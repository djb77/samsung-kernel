/*
 * drivers/soc/samsung/sec_param.c
 *
 * COPYRIGHT(C) 2011-2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sec_param.h>
#include <linux/file.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/sec_debug.h>
#ifdef CONFIG_SEC_NAD
#include <linux/sec_nad.h>
#endif

#define PARAM_RD	0
#define PARAM_WR	1

#define MAX_PARAM_BUFFER	128

static DEFINE_MUTEX(sec_param_mutex);

/* single global instance */
struct sec_param_data *param_data;
struct sec_param_data_s sched_sec_param_data;

static unsigned int param_file_size;

#ifdef CONFIG_SEC_AP_HEALTH
static int driver_initialized;
static struct delayed_work sec_param_notify_work;
static struct workqueue_struct *sec_param_wq;
static struct delayed_work ap_health_work;
static ap_health_t ap_health_data;
static int ap_health_initialized;
static int in_panic;
static DEFINE_MUTEX(ap_health_work_lock);
#endif /* CONFIG_SEC_AP_HEALTH */

static char param_name[MAX_PARAM_BUFFER];
static __init int get_param_name(char *str)
{
	if (likely(str))
		return snprintf(param_name, sizeof(param_name),
			"/dev/block/platform/soc/%s/by-name/param", str);

	pr_err("sec_param: failed to get path from androidboot.bootdevice\n");
	return 0;

}
__setup("androidboot.bootdevice=", get_param_name);

static int __init sec_get_param_file_size_setup(char *str)
{
	int ret;
	unsigned long long size = 0;

	ret = kstrtoull(str, 0, &size);
	if (ret)
		pr_err("%s str:%s\n", __func__, str);
	else
		param_file_size = (unsigned int)size;

	return 1;
}
__setup("sec_param_file_size=", sec_get_param_file_size_setup);

static void param_sec_operation(struct work_struct *work)
{
	/* Read from PARAM(parameter) partition  */
	struct file *filp;
	mm_segment_t fs;
	int ret;
	struct sec_param_data_s *sched_param_data =
		container_of(to_delayed_work(work), struct sec_param_data_s, sec_param_work);

	int flag = (sched_param_data->direction == PARAM_WR)
			? (O_RDWR | O_SYNC) : O_RDONLY;
	unsigned long delay = 5 * HZ;

	pr_info("%s - start.\n", __func__);
	pr_debug("%s %p %x %d %d\n", __func__, sched_param_data->value,
			sched_param_data->offset, sched_param_data->size,
			sched_param_data->direction);

	if (param_name[0] == '\0') {
		pr_err("sec_param: failed to find cmdline\n");
		return;
	}
	filp = filp_open(param_name, flag, 0);

	if (IS_ERR(filp)) {
		pr_err("%s: filp_open failed. (%ld)\n",
				__func__, PTR_ERR(filp));
		goto openfail_retry;
		return;
	}

	fs = get_fs();
	set_fs(get_ds());

	ret = filp->f_op->llseek(filp, sched_param_data->offset, SEEK_SET);
	if (unlikely(ret < 0)) {
		pr_err("%s FAIL LLSEEK\n", __func__);
		goto seekfail_retry;
	}

	if (sched_param_data->direction == PARAM_RD)
		filp->f_op->read(filp,
				(char __user *)sched_param_data->value,
				sched_param_data->size, &filp->f_pos);
	else if (sched_param_data->direction == PARAM_WR)
		filp->f_op->write(filp,
				(char __user *)sched_param_data->value,
				sched_param_data->size, &filp->f_pos);

	set_fs(fs);
	filp_close(filp, NULL);
	complete(&sched_sec_param_data.work);
	pr_info("%s - end.\n", __func__);
	return;

seekfail_retry:
	set_fs(fs);
	filp_close(filp, NULL);
openfail_retry:
	schedule_delayed_work(&sched_sec_param_data.sec_param_work, delay);
	pr_info("%s - end, will retry.\n", __func__);
}

#ifdef CONFIG_SEC_AP_HEALTH
static void ap_health_work_write_fn(struct work_struct *work)
{
	struct file *filp;
	mm_segment_t fs;
	int ret = true;
	unsigned long delay = 5 * HZ;

	pr_info("%s - start.\n", __func__);
	if (!mutex_trylock(&ap_health_work_lock)) {
		pr_err("%s - already locked.\n", __func__);
		delay = 2 * HZ;
		goto occupied_retry;
	}

	if (param_name[0] == '\0') {
		pr_err("sec_param: failed to find cmdline\n");
		return;
	}

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(param_name, (O_RDWR | O_SYNC), 0);

	if (IS_ERR(filp)) {
		pr_err("%s: filp_open failed. (%ld)\n",
				__func__, PTR_ERR(filp));
		goto openfail_retry;
	}

	ret = filp->f_op->llseek(filp, SEC_DEBUG_AP_HEALTH_OFFSET, SEEK_SET);
	if (unlikely(ret < 0)) {
		pr_err("%s FAIL LLSEEK\n", __func__);
		goto seekfail_retry;
	}

	filp->f_op->write(filp, (char __user *)&ap_health_data,
			SEC_DEBUG_AP_HEALTH_SIZE, &filp->f_pos);

	if (--ap_health_data.header.need_write) {
		goto remained;
	}

	filp_close(filp, NULL);
	set_fs(fs);

	mutex_unlock(&ap_health_work_lock);
	pr_info("%s - end.\n", __func__);
	return;

remained:
seekfail_retry:
	filp_close(filp, NULL);
openfail_retry:
	set_fs(fs);
	mutex_unlock(&ap_health_work_lock);
occupied_retry:
	queue_delayed_work(sec_param_wq, &ap_health_work, delay);
	pr_info("%s - end, will retry, wr(%u).\n", __func__,
		ap_health_data.header.need_write);
}
#endif /* CONFIG_SEC_AP_HEALTH */

bool sec_open_param(void)
{
	pr_info("%s start\n", __func__);

	if (param_data != NULL)
		return true;

	mutex_lock(&sec_param_mutex);

	param_data = kmalloc(sizeof(struct sec_param_data), GFP_KERNEL);

	sched_sec_param_data.value = param_data;
	sched_sec_param_data.offset = SEC_PARAM_FILE_OFFSET;
	sched_sec_param_data.size = sizeof(struct sec_param_data);
	sched_sec_param_data.direction = PARAM_RD;

	schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
	wait_for_completion(&sched_sec_param_data.work);

	mutex_unlock(&sec_param_mutex);

	pr_info("%s end\n", __func__);

	return true;
}

bool sec_write_param(void)
{
	pr_info("%s start\n", __func__);

	mutex_lock(&sec_param_mutex);

	sched_sec_param_data.value = param_data;
	sched_sec_param_data.offset = SEC_PARAM_FILE_OFFSET;
	sched_sec_param_data.size = sizeof(struct sec_param_data);
	sched_sec_param_data.direction = PARAM_WR;

	schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
	wait_for_completion(&sched_sec_param_data.work);

	mutex_unlock(&sec_param_mutex);

	pr_info("%s end\n", __func__);

	return true;
}

bool sec_get_param(enum sec_param_index index, void *value)
{
	int ret;

	ret = sec_open_param();
	if (!ret)
		return ret;

	switch (index) {
	case param_index_debuglevel:
		memcpy(value, &(param_data->debuglevel), sizeof(unsigned int));
		break;
	case param_index_uartsel:
		memcpy(value, &(param_data->uartsel), sizeof(unsigned int));
		break;
	case param_rory_control:
		memcpy(value, &(param_data->rory_control),
				sizeof(unsigned int));
		break;
	case param_index_movinand_checksum_done:
		memcpy(value, &(param_data->movinand_checksum_done),
				sizeof(unsigned int));
		break;
	case param_index_movinand_checksum_pass:
		memcpy(value, &(param_data->movinand_checksum_pass),
				sizeof(unsigned int));
		break;
#ifdef CONFIG_GSM_MODEM_SPRD6500
	case param_update_cp_bin:
		memcpy(value, &(param_data->update_cp_bin),
				sizeof(unsigned int));
		printk(KERN_INFO "param_data.update_cp_bin :[%d]!!",
				param_data->update_cp_bin);
		break;
#endif
#ifdef CONFIG_SEC_MONITOR_BATTERY_REMOVAL
	case param_index_normal_poweroff:
		memcpy(&(param_data->normal_poweroff), value,
				sizeof(unsigned int));
		break;
#endif
#ifdef CONFIG_BARCODE_PAINTER
	case param_index_barcode_info:
		memcpy(value, param_data->param_barcode_info,
				sizeof(param_data->param_barcode_info));
		break;
#endif
#ifdef CONFIG_WIRELESS_CHARGER_HIGH_VOLTAGE
	case param_index_wireless_charging_mode:
		memcpy(value, &(param_data->wireless_charging_mode),
				sizeof(unsigned int));
		break;
#endif
#ifdef CONFIG_MUIC_HV
	case param_index_afc_disable:
		memcpy(value, &(param_data->afc_disable), sizeof(unsigned int));
		break;
#endif
	case param_index_cp_reserved_mem:
		memcpy(value, &(param_data->cp_reserved_mem),
				sizeof(unsigned int));
		break;
#ifdef CONFIG_USER_RESET_DEBUG
	case param_index_reset_ex_info:
		mutex_lock(&sec_param_mutex);
		sched_sec_param_data.value = value;
		sched_sec_param_data.offset = SEC_PARAM_EX_INFO_OFFSET;
		sched_sec_param_data.size = SEC_DEBUG_EX_INFO_SIZE;
		sched_sec_param_data.direction = PARAM_RD;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		mutex_unlock(&sec_param_mutex);
		break;
#endif
#ifdef CONFIG_SEC_NAD
	case param_index_qnad:
		mutex_lock(&sec_param_mutex);
		sched_sec_param_data.value=value;
		sched_sec_param_data.offset=SEC_PARAM_NAD_OFFSET;
		sched_sec_param_data.size=sizeof(struct param_qnad);
		sched_sec_param_data.direction=PARAM_RD;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		mutex_unlock(&sec_param_mutex);
		break;
	case param_index_qnad_ddr_result:
		mutex_lock(&sec_param_mutex);
		sched_sec_param_data.value=value;
		sched_sec_param_data.offset=SEC_PARAM_NAD_DDR_RESULT_OFFSET;
		sched_sec_param_data.size=sizeof(struct param_qnad_ddr_result);
		sched_sec_param_data.direction=PARAM_RD;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		mutex_unlock(&sec_param_mutex);
		break;
#endif
	default:
		return false;
	}

	return true;
}
EXPORT_SYMBOL(sec_get_param);

bool sec_set_param(enum sec_param_index index, void *value)
{
	int ret;

	ret = sec_open_param();
	if (!ret)
		return ret;

	switch (index) {
	case param_index_debuglevel:
		memcpy(&(param_data->debuglevel),
				value, sizeof(unsigned int));
		break;
	case param_index_uartsel:
		memcpy(&(param_data->uartsel),
				value, sizeof(unsigned int));
		break;
	case param_rory_control:
		memcpy(&(param_data->rory_control),
				value, sizeof(unsigned int));
		break;
	case param_index_movinand_checksum_done:
		memcpy(&(param_data->movinand_checksum_done),
				value, sizeof(unsigned int));
		break;
	case param_index_movinand_checksum_pass:
		memcpy(&(param_data->movinand_checksum_pass),
				value, sizeof(unsigned int));
		break;
#ifdef CONFIG_GSM_MODEM_SPRD6500
	case param_update_cp_bin:
		memcpy(&(param_data->update_cp_bin),
				value, sizeof(unsigned int));
		break;
#endif
#ifdef CONFIG_SEC_MONITOR_BATTERY_REMOVAL
	case param_index_normal_poweroff:
		memcpy(&(param_data->normal_poweroff),
				value, sizeof(unsigned int));
		break;
#endif
#ifdef CONFIG_BARCODE_PAINTER
	case param_index_barcode_info:
		memcpy(param_data->param_barcode_info,
				value, sizeof(param_data->param_barcode_info));
		break;
#endif
#ifdef CONFIG_WIRELESS_CHARGER_HIGH_VOLTAGE
	case param_index_wireless_charging_mode:
		memcpy(&(param_data->wireless_charging_mode),
				value, sizeof(unsigned int));
		break;
#endif
#ifdef CONFIG_MUIC_HV
	case param_index_afc_disable:
		memcpy(&(param_data->afc_disable),
				value, sizeof(unsigned int));
		break;
#endif
	case param_index_cp_reserved_mem:
		memcpy(&(param_data->cp_reserved_mem),
				value, sizeof(unsigned int));
		break;
#ifdef CONFIG_USER_RESET_DEBUG
	case param_index_reset_ex_info:
		/* do nothing */
		break;
#endif
#ifdef CONFIG_SEC_AP_HEALTH
	case param_index_ap_health:
		/* do nothing */
		break;
#endif /* CONFIG_SEC_AP_HEALTH */
#ifdef CONFIG_SEC_NAD
	case param_index_qnad:
		mutex_lock(&sec_param_mutex);
		sched_sec_param_data.value=(struct param_qnad *)value;
		sched_sec_param_data.offset=SEC_PARAM_NAD_OFFSET;
		sched_sec_param_data.size=sizeof(struct param_qnad);
		sched_sec_param_data.direction=PARAM_WR;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		mutex_unlock(&sec_param_mutex);
		break;
	case param_index_qnad_ddr_result:
		mutex_lock(&sec_param_mutex);
		sched_sec_param_data.value=(struct param_qnad_ddr_result *)value;
		sched_sec_param_data.offset=SEC_PARAM_NAD_DDR_RESULT_OFFSET;
		sched_sec_param_data.size=sizeof(struct param_qnad_ddr_result);
		sched_sec_param_data.direction=PARAM_WR;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		mutex_unlock(&sec_param_mutex);
		break;
#endif
	default:
		return false;
	}

	ret = sec_write_param();

	return ret;
}
EXPORT_SYMBOL(sec_set_param);

#ifdef CONFIG_SEC_AP_HEALTH
ap_health_t* ap_health_data_read(void)
{
	if (!driver_initialized)
		return NULL;

	if (ap_health_initialized)
		return &ap_health_data;
	else
		return NULL;
}

EXPORT_SYMBOL(ap_health_data_read);

int ap_health_data_write(ap_health_t *data)
{
	if (!driver_initialized || !data || !ap_health_initialized)
		return -1;

	data->header.need_write++;

	if (!in_panic) {
		queue_delayed_work(sec_param_wq, &ap_health_work, 0);
	}

	return 0;
}
EXPORT_SYMBOL(ap_health_data_write);


static BLOCKING_NOTIFIER_HEAD(sec_param_notifier_list);

int sec_param_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&sec_param_notifier_list, nb);
}
EXPORT_SYMBOL(sec_param_notifier_register);

static void sec_param_do_notify(struct work_struct *work)
{
	pr_info("%s: start\n", __func__);

	mutex_lock(&sec_param_mutex);
	sched_sec_param_data.value = &ap_health_data;
	sched_sec_param_data.offset = SEC_DEBUG_AP_HEALTH_OFFSET;
	sched_sec_param_data.size = SEC_DEBUG_AP_HEALTH_SIZE;
	sched_sec_param_data.direction = PARAM_RD;
	schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
	wait_for_completion(&sched_sec_param_data.work);

	if (ap_health_data.header.magic != AP_HEALTH_MAGIC ||
		ap_health_data.header.version != AP_HEALTH_VER ||
		ap_health_data.header.size != SEC_DEBUG_AP_HEALTH_SIZE) {
		pr_err("%s: ap_health_data magic(0x%016llx, 0x%016llx) ver(%u, %u) size(%u, %zu)\n",
			__func__, ap_health_data.header.magic, (long long unsigned)AP_HEALTH_MAGIC,
			ap_health_data.header.version, AP_HEALTH_VER,
			ap_health_data.header.size, SEC_DEBUG_AP_HEALTH_SIZE);

		memset((void *)&ap_health_data, 0, SEC_DEBUG_AP_HEALTH_SIZE);

		ap_health_data.header.magic = AP_HEALTH_MAGIC;
		ap_health_data.header.version = AP_HEALTH_VER;
		ap_health_data.header.size = SEC_DEBUG_AP_HEALTH_SIZE;

		sched_sec_param_data.value = &ap_health_data;
		sched_sec_param_data.offset = SEC_DEBUG_AP_HEALTH_OFFSET;
		sched_sec_param_data.size = SEC_DEBUG_AP_HEALTH_SIZE;
		sched_sec_param_data.direction = PARAM_WR;

		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
	}
	mutex_unlock(&sec_param_mutex);

	ap_health_initialized=1;

	blocking_notifier_call_chain(&sec_param_notifier_list, SEC_PARAM_DRV_INIT_DONE, NULL);

	pr_info("%s: end\n", __func__);
	return;
}

static int sec_param_panic_prepare(struct notifier_block *nb,
		unsigned long event, void *data)
{
	in_panic = 1;
	return NOTIFY_DONE;
}

static struct notifier_block sec_param_panic_notifier_block = {
	.notifier_call = sec_param_panic_prepare,
};
#endif /* CONFIG_SEC_AP_HEALTH */

static int __init sec_param_work_init(void)
{
	pr_info("%s: start\n", __func__);

	sched_sec_param_data.offset = 0;
	sched_sec_param_data.direction = 0;
	sched_sec_param_data.size = 0;
	sched_sec_param_data.value = NULL;

	init_completion(&sched_sec_param_data.work);
	INIT_DELAYED_WORK(&sched_sec_param_data.sec_param_work, param_sec_operation);

#ifdef CONFIG_SEC_AP_HEALTH
	INIT_DELAYED_WORK(&sec_param_notify_work, sec_param_do_notify);
	INIT_DELAYED_WORK(&ap_health_work, ap_health_work_write_fn);

	sec_param_wq = create_singlethread_workqueue("ap_health_writer");
	if (!sec_param_wq) {
		pr_err("%s: fail to create sec_param_wq!\n", __func__);
		return -EFAULT; 
	}
	atomic_notifier_chain_register(&panic_notifier_list, &sec_param_panic_notifier_block);
	
	driver_initialized = 1;
	schedule_delayed_work(&sec_param_notify_work, 2 * HZ);
#endif /* CONFIG_SEC_AP_HEALTH */
	pr_info("%s: end\n", __func__);

	return 0;
}

static void __exit sec_param_work_exit(void)
{
#ifdef CONFIG_SEC_AP_HEALTH
	driver_initialized = 0;
#endif
	cancel_delayed_work_sync(&sched_sec_param_data.sec_param_work);
#ifdef CONFIG_SEC_AP_HEALTH
	cancel_delayed_work_sync(&sec_param_notify_work);
#endif
	pr_info("%s: exit\n", __func__);
}

module_init(sec_param_work_init);
module_exit(sec_param_work_exit);