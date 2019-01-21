/*
 * drivers/debug/sec_debug_user_reset.c
 *
 * COPYRIGHT(C) 2006-2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <asm/stacktrace.h>

#include <linux/device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>

#include <linux/sec_debug.h>

#include <linux/user_reset/sec_debug_user_reset.h>
#include <linux/user_reset/sec_debug_partition.h>

#include <linux/sec_param.h>
#include <linux/sec_class.h>

#ifdef CONFIG_RKP_CFP_ROPP
#include <linux/rkp_cfp.h>
#endif

static char rr_str[][3] = {
	[USER_UPLOAD_CAUSE_SMPL] = "SP",
	[USER_UPLOAD_CAUSE_WTSR] = "WP",
	[USER_UPLOAD_CAUSE_WATCHDOG] = "DP",
	[USER_UPLOAD_CAUSE_PANIC] = "KP",
	[USER_UPLOAD_CAUSE_MANUAL_RESET] = "MP",
	[USER_UPLOAD_CAUSE_POWER_RESET] = "PP",
	[USER_UPLOAD_CAUSE_REBOOT] = "RP",
	[USER_UPLOAD_CAUSE_BOOTLOADER_REBOOT] = "BP",
	[USER_UPLOAD_CAUSE_POWER_ON] = "NP",
	[USER_UPLOAD_CAUSE_THERMAL] = "TP",
	[USER_UPLOAD_CAUSE_UNKNOWN] = "NP",
};

char *klog_buf;
uint32_t klog_size;
char *klog_read_buf;
struct debug_reset_header *klog_info;
static DEFINE_MUTEX(klog_mutex);

char *summary_buf;
struct debug_reset_header *summary_info;
static DEFINE_MUTEX(summary_mutex);
static unsigned reset_reason = 0xFFEEFFEE;

char *tzlog_buf;
struct debug_reset_header *tzlog_info;
static DEFINE_MUTEX(tzlog_mutex);

static int reset_write_cnt = -1;

uint32_t sec_debug_get_reset_reason(void)
{
	return reset_reason;
}
EXPORT_SYMBOL(sec_debug_get_reset_reason);

int sec_debug_get_reset_write_cnt(void)
{
	return reset_write_cnt;
}
EXPORT_SYMBOL(sec_debug_get_reset_write_cnt);

char *sec_debug_get_reset_reason_str(unsigned int reason)
{
	if (reason < USER_UPLOAD_CAUSE_MIN || reason > USER_UPLOAD_CAUSE_MAX)
		reason = USER_UPLOAD_CAUSE_UNKNOWN;

	return rr_str[reason];
}
EXPORT_SYMBOL(sec_debug_get_reset_reason_str);

#if defined(CONFIG_SUPPORT_DEBUG_PARTITION)
void sec_debug_update_reset_reason(uint32_t debug_partition_rr)
{
	static char updated = 0;

	if (!updated) {
		reset_reason = debug_partition_rr;
		updated = 1;
		pr_info("partition[%d] result[%s]\n",
			debug_partition_rr,
			sec_debug_get_reset_reason_str(reset_reason));
	}
}

void reset_reason_update_and_clear(void)
{
	ap_health_t *p_health;
	uint32_t rr_data;

	p_health = ap_health_data_read();
	if (p_health == NULL) {
		pr_err("p_health is NULL\n");
		return;
	}

	pr_info("done\n");
	rr_data = sec_debug_get_reset_reason();
	switch (rr_data) {
	case USER_UPLOAD_CAUSE_SMPL:
		p_health->daily_rr.sp++;
		p_health->rr.sp++;
		break;
	case USER_UPLOAD_CAUSE_WTSR:
		p_health->daily_rr.wp++;
		p_health->rr.wp++;
		break;
	case USER_UPLOAD_CAUSE_WATCHDOG:
		p_health->daily_rr.dp++;
		p_health->rr.dp++;
		break;
	case USER_UPLOAD_CAUSE_PANIC:
		p_health->daily_rr.kp++;
		p_health->rr.kp++;
		break;
	case USER_UPLOAD_CAUSE_MANUAL_RESET:
		p_health->daily_rr.mp++;
		p_health->rr.mp++;
		break;
	case USER_UPLOAD_CAUSE_POWER_RESET:
		p_health->daily_rr.pp++;
		p_health->rr.pp++;
		break;
	case USER_UPLOAD_CAUSE_REBOOT:
		p_health->daily_rr.rp++;
		p_health->rr.rp++;
		break;
	case USER_UPLOAD_CAUSE_THERMAL:
		p_health->daily_rr.tp++;
		p_health->rr.tp++;
		break;
	default:
		p_health->daily_rr.np++;
		p_health->rr.np++;
	}

	p_health->last_rst_reason = 0;
	ap_health_data_write(p_health);
}
#endif

static int set_reset_reason_proc_show(struct seq_file *m, void *v)
{
	uint32_t rr_data;
	static uint32_t rr_cnt_update = 1;

#if defined(CONFIG_SUPPORT_DEBUG_PARTITION)
	rr_data = sec_debug_get_reset_reason();

	seq_printf(m, "%sON\n", sec_debug_get_reset_reason_str(rr_data));
	if (rr_cnt_update) {
		reset_reason_update_and_clear();
		rr_cnt_update = 0;
	}
#else
	if (rr_cnt_update) {
		sec_get_param(param_index_last_reset_reason, &rr_data);
		reset_reason = rr_data;
		pr_info("partition[%d] result[%s]\n",
			rr_data, sec_debug_get_reset_reason_str(rr_data));

		rr_data = 0;
		sec_set_param(param_index_last_reset_reason, &rr_data);
		rr_cnt_update = 0;
	}

	rr_data = sec_debug_get_reset_reason();	
	seq_printf(m, "%sON\n", sec_debug_get_reset_reason_str(rr_data));
#endif

	return 0;
}

static int sec_reset_reason_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_reset_reason_proc_show, NULL);
}

static const struct file_operations sec_reset_reason_proc_fops = {
	.open = sec_reset_reason_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static phys_addr_t sec_debug_reset_ex_info_paddr;
static unsigned sec_debug_reset_ex_info_size;
rst_exinfo_t *sec_debug_reset_ex_info;
ex_info_fault_t ex_info_fault[NR_CPUS];

extern unsigned int get_sec_log_idx(void);
void sec_debug_store_extc_idx(bool prefix)
{
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		if (p_ex_info->extc_idx == 0) {
			p_ex_info->extc_idx = get_sec_log_idx();
			if (prefix)
				p_ex_info->extc_idx += SEC_DEBUG_RESET_EXTRC_SIZE;
		}
	}
}
EXPORT_SYMBOL(sec_debug_store_extc_idx);

void sec_debug_store_bug_string(const char *fmt, ...)
{
	va_list args;
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		va_start(args, fmt);
		vsnprintf(p_ex_info->bug_buf,
			sizeof(p_ex_info->bug_buf), fmt, args);
		va_end(args);
	}
}
EXPORT_SYMBOL(sec_debug_store_bug_string);

void sec_debug_store_additional_dbg(enum extra_info_dbg_type type,
				    unsigned int value, const char *fmt, ...)
{
	va_list args;
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		switch (type) {
		case DBG_0_GLAD_ERR:
			va_start(args, fmt);
			vsnprintf(p_ex_info->dbg0,
					sizeof(p_ex_info->dbg0), fmt, args);
			va_end(args);
			break;
		case DBG_1_UFS_ERR:
			va_start(args, fmt);
			vsnprintf(p_ex_info->ufs_err,
					sizeof(p_ex_info->ufs_err), fmt, args);
			va_end(args);
			break;
		case DBG_2_DISPLAY_ERR:
			va_start(args, fmt);
			vsnprintf(p_ex_info->display_err,
					sizeof(p_ex_info->display_err), fmt, args);
			va_end(args);
			break;
		case DBG_3_RESERVED ... DBG_5_RESERVED:
			break;
		default:
			break;
		}
	}
}
EXPORT_SYMBOL(sec_debug_store_additional_dbg);

void sec_debug_init_panic_extra_info(void)
{
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		memset((void *)&sec_debug_reset_ex_info->kern_ex_info, 0,
			sizeof(sec_debug_reset_ex_info->kern_ex_info));
		p_ex_info->cpu = -1;
#if defined(CONFIG_ARM64)
		pr_info("%s: ex_info memory initialized size[%ld]\n",
			__func__, sizeof(kern_exinfo_t));
#else
		pr_info("%s: ex_info memory initialized size[%d]\n",
			__func__, sizeof(kern_exinfo_t));
#endif
	}
}

int __init sec_debug_ex_info_setup(char *str)
{
	unsigned size = memparse(str, &str);
	int ret;

	if (size && (*str == '@')) {
		unsigned long long base = 0;

		ret = kstrtoull(++str, 0, &base);
		if (ret) {
			pr_err("failed to parse sec_dbg_ex_info\n");
			return ret;
		}

		sec_debug_reset_ex_info_paddr = base;
		sec_debug_reset_ex_info_size =
					(size + 0x1000 - 1) & ~(0x1000 - 1);

		pr_info("ex info phy=0x%llx, size=0x%x\n",
				(uint64_t)sec_debug_reset_ex_info_paddr,
				sec_debug_reset_ex_info_size);
	}
	return 1;
}
__setup("sec_dbg_ex_info=", sec_debug_ex_info_setup);

int __init sec_debug_get_extra_info_region(void)
{
	if (!sec_debug_reset_ex_info_paddr || !sec_debug_reset_ex_info_size)
		return -1;

	sec_debug_reset_ex_info = ioremap_cache(sec_debug_reset_ex_info_paddr,
						sec_debug_reset_ex_info_size);

	if (!sec_debug_reset_ex_info) {
		pr_err("Failed to remap nocache ex info region\n");
		return -1;
	}

	sec_debug_init_panic_extra_info();

	return 0;
}
arch_initcall_sync(sec_debug_get_extra_info_region);

#if defined(CONFIG_SUPPORT_DEBUG_PARTITION)
struct debug_reset_header *get_debug_reset_header(void)
{
	struct debug_reset_header *header = NULL;
	static int get_state = DRH_STATE_INIT;

	if (get_state == DRH_STATE_INVALID)
		return NULL;

	header = kmalloc(sizeof(struct debug_reset_header), GFP_KERNEL);
	if (!header) {
		pr_err("%s : fail - kmalloc for debug_reset_header\n", __func__);
		return NULL;
	}
	if (!read_debug_partition(debug_index_reset_summary_info, header)) {
		pr_err("%s : fail - get param!! debug_reset_header\n", __func__);
		kfree(header);
		header = NULL;
		return NULL;
	}

	if (get_state != DRH_STATE_VALID) {
		if (header->write_times == header->read_times) {
			pr_err("%s : untrustworthy debug_reset_header\n", __func__);
			get_state = DRH_STATE_INVALID;
			kfree(header);
			header = NULL;
			return NULL;
		}
		reset_write_cnt = header->write_times;
		get_state = DRH_STATE_VALID;
	}

	return header;
}

int set_debug_reset_header(struct debug_reset_header *header)
{
	int ret = 0;
	static int set_state = DRH_STATE_INIT;

	if (set_state == DRH_STATE_VALID) {
		pr_info("%s : debug_reset_header working well\n", __func__);
		return ret;
	}

	if ((header->write_times - 1) == header->read_times) {
		pr_info("%s : debug_reset_header working well\n", __func__);
		header->read_times++;
	} else {
		pr_info("%s : debug_reset_header read[%d] and write[%d] work sync error.\n",
				__func__, header->read_times, header->write_times);
		header->read_times = header->write_times;
	}

	if (!write_debug_partition(debug_index_reset_summary_info, header)) {
		pr_err("%s : fail - set param!! debug_reset_header\n", __func__);
		ret = -ENOENT;
	} else {
		set_state = DRH_STATE_VALID;
	}

	return ret;
}

int sec_reset_summary_info_init(void)
{
	int ret = 0;

	if (summary_buf != NULL)
		return true;

	if (summary_info != NULL) {
		pr_err("%s : already memory alloc for summary_info\n", __func__);
		return -EINVAL;
	}

	summary_info = get_debug_reset_header();
	if (summary_info == NULL)
		return -EINVAL;

	if (summary_info->summary_size > SEC_DEBUG_RESET_SUMMARY_SIZE) {
		pr_err("%s : summary_size has problem.\n", __func__);
		ret = -EINVAL;
		goto error_summary_info;
	}

	summary_buf = vmalloc(SEC_DEBUG_RESET_SUMMARY_SIZE);
	if (!summary_buf) {
		pr_err("%s : fail - kmalloc for summary_buf\n", __func__);
		ret = -ENOMEM;
		goto error_summary_info;
	}
	if (!read_debug_partition(debug_index_reset_summary, summary_buf)) {
		pr_err("%s : fail - get param!! summary data\n", __func__);
		ret = -ENOENT;
		goto error_summary_buf;
	}

	pr_info("%s : w[%d] r[%d] idx[%d] size[%d]\n",
			__func__, summary_info->write_times, summary_info->read_times,
			summary_info->ap_klog_idx, summary_info->summary_size);

	return ret;

error_summary_buf:
	vfree(summary_buf);
error_summary_info:
	kfree(summary_info);

	return ret;
}

int sec_reset_summary_completed(void)
{
	int ret = 0;

	ret = set_debug_reset_header(summary_info);

	vfree(summary_buf);
	kfree(summary_info);

	summary_info = NULL;
	summary_buf = NULL;
	pr_info("%s finish\n", __func__);

	return ret;
}

static ssize_t sec_reset_summary_info_proc_read(struct file *file,
			char __user *buf, size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	mutex_lock(&summary_mutex);
	if (sec_reset_summary_info_init() < 0) {
		mutex_unlock(&summary_mutex);
		return -ENOENT;
	}

	if ((pos >= summary_info->summary_size) || (pos >= SEC_DEBUG_RESET_SUMMARY_SIZE)) {
		pr_info("%s : pos %lld, size %d\n", __func__, pos, summary_info->summary_size);
		sec_reset_summary_completed();
		mutex_unlock(&summary_mutex);
		return 0;
	}

	count = min(len, (size_t)(summary_info->summary_size - pos));
	if (copy_to_user(buf, summary_buf + pos, count)) {
		mutex_unlock(&summary_mutex);
		return -EFAULT;
	}

	*offset += count;
	mutex_unlock(&summary_mutex);
	return count;
}

const struct file_operations sec_reset_summary_info_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_summary_info_proc_read,
};

int sec_reset_klog_init(void)
{
	int ret = 0;

	if ((klog_read_buf != NULL) && (klog_buf != NULL))
		return true;

	if (klog_info != NULL) {
		pr_err("%s : already memory alloc for klog_info\n", __func__);
		return -EINVAL;
	}

	klog_info = get_debug_reset_header();
	if (klog_info == NULL)
		return -EINVAL;

	klog_read_buf = vmalloc(SEC_DEBUG_RESET_KLOG_SIZE);
	if (!klog_read_buf) {
		pr_err("%s : fail - vmalloc for klog_read_buf\n", __func__);
		ret = -ENOMEM;
		goto error_klog_info;
	}
	if (!read_debug_partition(debug_index_reset_klog, klog_read_buf)) {
		pr_err("%s : fail - get param!! summary data\n", __func__);
		ret = -ENOENT;
		goto error_klog_read_buf;
	}

	pr_info("%s : idx[%d]\n", __func__, klog_info->ap_klog_idx);

	klog_size = min((uint32_t)SEC_DEBUG_RESET_KLOG_SIZE, (uint32_t)klog_info->ap_klog_idx);

	klog_buf = vmalloc(klog_size);
	if (!klog_buf) {
		pr_err("%s : fail - vmalloc for klog_buf\n", __func__);
		ret = -ENOMEM;
		goto error_klog_read_buf;
	}

	if (klog_size && klog_buf && klog_read_buf) {
		unsigned int i;
		for (i = 0; i < klog_size; i++)
			klog_buf[i] = klog_read_buf[(klog_info->ap_klog_idx - klog_size + i) % SEC_DEBUG_RESET_KLOG_SIZE];
	}

	return ret;

error_klog_read_buf:
	vfree(klog_read_buf);
error_klog_info:
	kfree(klog_info);

	return ret;
}

void sec_reset_klog_completed(void)
{
	set_debug_reset_header(klog_info);

	vfree(klog_buf);
	vfree(klog_read_buf);
	kfree(klog_info);

	klog_info = NULL;
	klog_buf = NULL;
	klog_read_buf = NULL;
	klog_size = 0;

	pr_info("%s finish\n", __func__);
}

ssize_t sec_reset_klog_proc_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	mutex_lock(&klog_mutex);
	if (sec_reset_klog_init() < 0) {
		mutex_unlock(&klog_mutex);
		return -ENOENT;
	}

	if (pos >= klog_size) {
		pr_info("%s : pos %lld, size %d\n", __func__, pos, klog_size);
		sec_reset_klog_completed();
		mutex_unlock(&klog_mutex);
		return 0;
	}

	count = min(len, (size_t)(klog_size - pos));
	if (copy_to_user(buf, klog_buf + pos, count)) {
		mutex_unlock(&klog_mutex);
		return -EFAULT;
	}

	*offset += count;
	mutex_unlock(&klog_mutex);
	return count;
}

const struct file_operations sec_reset_klog_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_klog_proc_read,
};

int sec_reset_tzlog_init(void)
{
	int ret = 0;

	if (tzlog_buf != NULL)
		return true;

	if (tzlog_info != NULL) {
		pr_err("%s : already memory alloc for tzlog_info\n", __func__);
		return -EINVAL;
	}

	tzlog_info = get_debug_reset_header();
	if (tzlog_info == NULL)
		return -EINVAL;

	if (tzlog_info->stored_tzlog == 0) {
		pr_err("%s : The target didn't run SDI operation\n", __func__);
		ret = -EINVAL;
		goto error_tzlog_info;
	}

	tzlog_buf = vmalloc(SEC_DEBUG_RESET_TZLOG_SIZE);
	if (!tzlog_buf) {
		pr_err("%s : fail - vmalloc for tzlog_read_buf\n", __func__);
		ret = -ENOMEM;
		goto error_tzlog_info;
	}
	if (!read_debug_partition(debug_index_reset_tzlog, tzlog_buf)) {
		pr_err("%s : fail - get param!! tzlog data\n", __func__);
		ret = -ENOENT;
		goto error_tzlog_buf;
	}

	return ret;

error_tzlog_buf:
	vfree(tzlog_buf);
error_tzlog_info:
	kfree(tzlog_info);

	return ret;
}

void sec_reset_tzlog_completed(void)
{
	set_debug_reset_header(tzlog_info);

	vfree(tzlog_buf);
	kfree(tzlog_info);

	tzlog_info = NULL;
	tzlog_buf = NULL;

	pr_info("%s finish\n", __func__);
}

ssize_t sec_reset_tzlog_proc_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	mutex_lock(&tzlog_mutex);
	if (sec_reset_tzlog_init() < 0) {
		mutex_unlock(&tzlog_mutex);
		return -ENOENT;
	}

	if (pos >= SEC_DEBUG_RESET_TZLOG_SIZE) {
		pr_info("%s : pos %lld, size %d\n", __func__, pos, SEC_DEBUG_RESET_TZLOG_SIZE);
		sec_reset_tzlog_completed();
		mutex_unlock(&tzlog_mutex);
		return 0;
	}

	count = min(len, (size_t)(SEC_DEBUG_RESET_TZLOG_SIZE - pos));
	if (copy_to_user(buf, tzlog_buf + pos, count)) {
		mutex_unlock(&tzlog_mutex);
		return -EFAULT;
	}

	*offset += count;
	mutex_unlock(&tzlog_mutex);
	return count;
}

const struct file_operations sec_reset_tzlog_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_tzlog_proc_read,
};

int set_store_lastkmsg_proc_show(struct seq_file *m, void *v)
{
	struct debug_reset_header *check_store = NULL;

	if (check_store != NULL) {
		pr_err("%s : already memory alloc for check_store\n", __func__);
		return -EINVAL;
	}

	check_store = get_debug_reset_header();
	if (check_store == NULL) {
		seq_printf(m, "0\n");
	} else {
		seq_printf(m, "1\n");
	}

	if (check_store != NULL) {
		kfree(check_store);
		check_store = NULL;
	}

	return 0;
}

int sec_store_lastkmsg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_store_lastkmsg_proc_show, NULL);
}

const struct file_operations sec_store_lastkmsg_proc_fops = {
	.open = sec_store_lastkmsg_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int sec_reset_reason_dbg_part_notifier_callback(
		struct notifier_block *nfb, unsigned long action, void *data)
{
	ap_health_t *p_health;
	uint32_t rr_data;

	switch (action) {
		case DBG_PART_DRV_INIT_DONE:
			p_health = ap_health_data_read();
			if (!p_health)
				return NOTIFY_DONE;

			sec_debug_update_reset_reason(
						p_health->last_rst_reason);
			rr_data = sec_debug_get_reset_reason();
			break;
		default:
			return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}


struct notifier_block sec_reset_reason_dbg_part_notifier = {
	.notifier_call = sec_reset_reason_dbg_part_notifier_callback,
};
#endif

static int __init sec_debug_reset_reason_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("reset_reason", S_IWUGO, NULL,
		&sec_reset_reason_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

#if defined(CONFIG_SUPPORT_DEBUG_PARTITION)
	entry = proc_create("reset_summary", S_IWUGO, NULL,
			&sec_reset_summary_info_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("reset_klog", S_IWUGO, NULL,
			&sec_reset_klog_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("reset_tzlog", S_IWUGO, NULL,
			&sec_reset_tzlog_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	entry = proc_create("store_lastkmsg", S_IWUGO, NULL,
			&sec_store_lastkmsg_proc_fops);
	if (unlikely(!entry))
		return -ENOMEM;

	dbg_partition_notifier_register(&sec_reset_reason_dbg_part_notifier);
#endif

	return 0;
}
device_initcall(sec_debug_reset_reason_init);

static ssize_t show_recovery_cause(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char recovery_cause[256];

	sec_get_param(param_index_reboot_recovery_cause, recovery_cause);
	pr_info("%s\n", recovery_cause);

	return scnprintf(buf, sizeof(recovery_cause), "%s", recovery_cause);
}

static ssize_t store_recovery_cause(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	char recovery_cause[256];

	if (count > sizeof(recovery_cause)) {
		pr_err("input buffer length is out of range.\n");
		return -EINVAL;
	}
	snprintf(recovery_cause, sizeof(recovery_cause), "%s:%d ", current->comm, task_pid_nr(current));
	if (strlen(recovery_cause) + strlen(buf) >= sizeof(recovery_cause)) {
		pr_err("input buffer length is out of range.\n");
		return -EINVAL;
	}
	strncat(recovery_cause, buf, strlen(buf));
	sec_set_param(param_index_reboot_recovery_cause, recovery_cause);
	pr_info("%s\n", recovery_cause);

	return count;
}

static DEVICE_ATTR(recovery_cause, 0660, show_recovery_cause, store_recovery_cause);

static struct device *sec_debug_dev;

static int __init sec_debug_recovery_reason_init(void)
{
	int ret;

	/* create sysfs for reboot_recovery_cause */
	sec_debug_dev = sec_device_create(0, NULL, "sec_debug");
	if (IS_ERR(sec_debug_dev)) {
		pr_err("Failed to create device for sec_debug\n");
		return PTR_ERR(sec_debug_dev);
	}

	ret = sysfs_create_file(&sec_debug_dev->kobj, &dev_attr_recovery_cause.attr);
	if (ret) {
		pr_err("Failed to create sysfs group for sec_debug\n");
		sec_device_destroy(sec_debug_dev->devt);
		sec_debug_dev = NULL;
		return ret;
	}

	return 0;
}
device_initcall(sec_debug_recovery_reason_init);

void _sec_debug_store_backtrace(unsigned long where)
{
	static int offset;
	unsigned int max_size = 0;
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;

#if defined(CONFIG_ARM64)
		max_size = (unsigned long long int)&sec_debug_reset_ex_info->rpm_ex_info.info
			- (unsigned long long int)p_ex_info->backtrace;
#else
		max_size = (unsigned int)&sec_debug_reset_ex_info->rpm_ex_info.info
			- (unsigned int)p_ex_info->backtrace;
#endif

		if (max_size <= offset)
			return;

		if (offset)
			offset += snprintf(p_ex_info->backtrace+offset,
					 max_size-offset, " > ");

		offset += snprintf(p_ex_info->backtrace+offset, max_size-offset,
					"%pS", (void *)where);
	}
}

void sec_debug_backtrace(void)
{
	static int once = 0;
	struct stackframe frame;
	int skip_callstack = 0;
#ifdef CONFIG_RKP_CFP_ROPP
	unsigned long where = 0x0;
#endif

	if (!once++) {
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.sp = current_stack_pointer;
		frame.pc = (unsigned long)sec_debug_backtrace;

		while (1) {
			int ret;

#ifdef CONFIG_RKP_CFP_ROPP
			ret = unwind_frame(current, &frame);
#else
			ret = unwind_frame(&frame);
#endif
			if (ret < 0)
				break;

			if (skip_callstack++ > 3) {
#ifdef CONFIG_RKP_CFP_ROPP
				where = frame.pc;
				if (where>>40 != 0xffffff){
					where = ropp_enable_backtrace(where, current);
				}
				_sec_debug_store_backtrace(where);
#else
				_sec_debug_store_backtrace(frame.pc);
#endif
			}
		}
	}
}
EXPORT_SYMBOL(sec_debug_backtrace);

