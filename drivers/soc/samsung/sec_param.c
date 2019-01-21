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

bool sec_open_param(void)
{
	pr_info("%s start\n", __func__);

	if (!param_data)
		param_data = kmalloc(sizeof(struct sec_param_data), GFP_KERNEL);

	if (unlikely(!param_data)) {
		pr_err("failed to alloc for param_data\n");
		return false;
	}

	sched_sec_param_data.value = param_data;
	sched_sec_param_data.offset = SEC_PARAM_FILE_OFFSET;
	sched_sec_param_data.size = sizeof(struct sec_param_data);
	sched_sec_param_data.direction = PARAM_RD;

	schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
	wait_for_completion(&sched_sec_param_data.work);

	pr_info("%s end\n", __func__);

	return true;
}

bool sec_write_param(void)
{
	pr_info("%s start\n", __func__);

	sched_sec_param_data.value = param_data;
	sched_sec_param_data.offset = SEC_PARAM_FILE_OFFSET;
	sched_sec_param_data.size = sizeof(struct sec_param_data);
	sched_sec_param_data.direction = PARAM_WR;

	schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
	wait_for_completion(&sched_sec_param_data.work);

	pr_info("%s end\n", __func__);

	return true;
}

bool sec_get_param(enum sec_param_index index, void *value)
{
	int ret;

	mutex_lock(&sec_param_mutex);

	ret = sec_open_param();
	if (!ret)
		goto out;

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
	case param_index_last_reset_reason:
		sched_sec_param_data.value = value;
		sched_sec_param_data.offset = SEC_PARAM_LAST_RESET_REASON;
		sched_sec_param_data.size = sizeof(uint32_t);
		sched_sec_param_data.direction = PARAM_RD;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		break;
#endif
#ifdef CONFIG_SEC_NAD
	case param_index_qnad:
		sched_sec_param_data.value=value;
		sched_sec_param_data.offset=SEC_PARAM_NAD_OFFSET;
		sched_sec_param_data.size=sizeof(struct param_qnad);
		sched_sec_param_data.direction=PARAM_RD;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		break;
	case param_index_qnad_ddr_result:
		sched_sec_param_data.value=value;
		sched_sec_param_data.offset=SEC_PARAM_NAD_DDR_RESULT_OFFSET;
		sched_sec_param_data.size=sizeof(struct param_qnad_ddr_result);
		sched_sec_param_data.direction=PARAM_RD;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		break;
#endif
	case param_index_apigpiotest:
		memcpy(value, &(param_data->apigpiotest),
			sizeof(param_data->apigpiotest));
		break;
	case param_index_apigpiotestresult:
		memcpy(value, param_data->apigpiotestresult,
			sizeof(param_data->apigpiotestresult));
		break;
	case param_index_reboot_recovery_cause:
		memcpy(value, param_data->reboot_recovery_cause,
				sizeof(param_data->reboot_recovery_cause));
		break;
#if defined (CONFIG_KEEP_JIG_LOW)
	case param_index_keep_jig_low:
		memcpy(value, &(param_data->keep_jig_low), sizeof(unsigned int));
		break;
#endif
	default:
		ret = false;
	}

out:
	mutex_unlock(&sec_param_mutex);
	return ret;
}
EXPORT_SYMBOL(sec_get_param);

bool sec_set_param(enum sec_param_index index, void *value)
{
	bool ret;

	mutex_lock(&sec_param_mutex);

	ret = sec_open_param();
	if (!ret)
		goto out;

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
	case param_index_last_reset_reason:
		sched_sec_param_data.value=(uint32_t *)value;
		sched_sec_param_data.offset=SEC_PARAM_LAST_RESET_REASON;
		sched_sec_param_data.size=sizeof(uint32_t);
		sched_sec_param_data.direction=PARAM_WR;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		break;
#endif
#ifdef CONFIG_SEC_NAD
	case param_index_qnad:
		sched_sec_param_data.value=(struct param_qnad *)value;
		sched_sec_param_data.offset=SEC_PARAM_NAD_OFFSET;
		sched_sec_param_data.size=sizeof(struct param_qnad);
		sched_sec_param_data.direction=PARAM_WR;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		break;
	case param_index_qnad_ddr_result:
		sched_sec_param_data.value=(struct param_qnad_ddr_result *)value;
		sched_sec_param_data.offset=SEC_PARAM_NAD_DDR_RESULT_OFFSET;
		sched_sec_param_data.size=sizeof(struct param_qnad_ddr_result);
		sched_sec_param_data.direction=PARAM_WR;
		schedule_delayed_work(&sched_sec_param_data.sec_param_work, 0);
		wait_for_completion(&sched_sec_param_data.work);
		break;
#endif
	case param_index_apigpiotest:
		memcpy(&(param_data->apigpiotest),
				value, sizeof(param_data->apigpiotest));
		break;
	case param_index_apigpiotestresult:
		memcpy(&(param_data->apigpiotestresult),
				value, sizeof(param_data->apigpiotestresult));
		break;
	case param_index_reboot_recovery_cause:
		memcpy(param_data->reboot_recovery_cause,
				value, sizeof(param_data->reboot_recovery_cause));
	break;
#if defined (CONFIG_KEEP_JIG_LOW)
	case param_index_keep_jig_low:
		memcpy(&(param_data->keep_jig_low),
				value, sizeof(unsigned int));
	break;
#endif
	default:
		ret = false;
		goto out;
	}

	ret = sec_write_param();

out:
	mutex_unlock(&sec_param_mutex);
	return ret;
}
EXPORT_SYMBOL(sec_set_param);

static int __init sec_param_work_init(void)
{
	pr_info("%s: start\n", __func__);

	sched_sec_param_data.offset = 0;
	sched_sec_param_data.direction = 0;
	sched_sec_param_data.size = 0;
	sched_sec_param_data.value = NULL;

	init_completion(&sched_sec_param_data.work);
	INIT_DELAYED_WORK(&sched_sec_param_data.sec_param_work, param_sec_operation);

	pr_info("%s: end\n", __func__);

	return 0;
}

static void __exit sec_param_work_exit(void)
{
	cancel_delayed_work_sync(&sched_sec_param_data.sec_param_work);
	pr_info("%s: exit\n", __func__);
}

module_init(sec_param_work_init);
module_exit(sec_param_work_exit);
