/*
 * sec_tsp_log.c
 *
 * driver supporting debug functions for Samsung touch device
 *
 * COPYRIGHT(C) Samsung Electronics Co., Ltd. 2006-2011 All Right Reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifdef CONFIG_SEC_DEBUG_TSP_LOG
#include <linux/input/sec_tsp_log.h>

static int sec_tsp_log_index;
static int sec_tsp_log_index_fix;
static char *sec_tsp_log_buf;
static unsigned int sec_tsp_log_size;

static int sec_tsp_raw_data_index;
static char *sec_tsp_raw_data_buf;
static unsigned int sec_tsp_raw_data_size;

static int sec_tsp_log_timestamp(unsigned long idx)
{
	/* Add the current time stamp */
	char tbuf[50];
	unsigned int tlen;
	unsigned long long t;
	unsigned long nanosec_rem;

	t = local_clock();
	nanosec_rem = do_div(t, 1000000000);
	tlen = snprintf(tbuf, sizeof(tbuf), "[%5lu.%06lu] ",
			(unsigned long)t,
			nanosec_rem / 1000);

	/* Overflow buffer size */
	if (idx + tlen > sec_tsp_log_size - 1) {
		if (sec_tsp_log_index_fix + tlen > sec_tsp_log_size - 1)
			return sec_tsp_log_index;
		tlen = scnprintf(&sec_tsp_log_buf[sec_tsp_log_index_fix],
				tlen + 1, "%s", tbuf);
		sec_tsp_log_index = sec_tsp_log_index_fix + tlen;
	} else {
		tlen = scnprintf(&sec_tsp_log_buf[idx], tlen + 1, "%s", tbuf);
		sec_tsp_log_index += tlen;
	}

	return sec_tsp_log_index;
}

static int sec_tsp_raw_data_timestamp(unsigned long idx)
{
	/* Add the current time stamp */
	char tbuf[50];
	unsigned int tlen;
	unsigned long long t;
	unsigned long nanosec_rem;

	t = local_clock();
	nanosec_rem = do_div(t, 1000000000);
	tlen = snprintf(tbuf, sizeof(tbuf), "[%5lu.%06lu] ",
			(unsigned long)t,
			nanosec_rem / 1000);

	/* Overflow buffer size */
	if (idx + tlen > sec_tsp_raw_data_size - 1) {
		tlen = scnprintf(&sec_tsp_raw_data_buf[0],
				tlen + 1, "%s", tbuf);
		sec_tsp_raw_data_index = tlen;
	} else {
		tlen = scnprintf(&sec_tsp_raw_data_buf[idx], tlen + 1, "%s", tbuf);
		sec_tsp_raw_data_index += tlen;
	}

	return sec_tsp_raw_data_index;
}

#define TSP_BUF_SIZE 512
void sec_debug_tsp_log(char *fmt, ...)
{
	va_list args;
	char buf[TSP_BUF_SIZE];
	int len = 0;
	unsigned int idx;
	unsigned long size;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_log_size)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = sec_tsp_log_index;
	size = strlen(buf);

	idx = sec_tsp_log_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size > sec_tsp_log_size - 1) {
		if (sec_tsp_log_index_fix + size > sec_tsp_log_size - 1)
			return;
		len = scnprintf(&sec_tsp_log_buf[sec_tsp_log_index_fix],
				size + 1, "%s\n", buf);
		sec_tsp_log_index = sec_tsp_log_index_fix + len;
	} else {
		len = scnprintf(&sec_tsp_log_buf[idx], size + 1, "%s\n", buf);
		sec_tsp_log_index += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_log);

void sec_debug_tsp_raw_data(char *fmt, ...)
{
	va_list args;
	char buf[TSP_BUF_SIZE];
	int len = 0;
	unsigned int idx;
	unsigned long size;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_raw_data_size || !sec_tsp_raw_data_buf)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = sec_tsp_raw_data_index;
	size = strlen(buf);

	idx = sec_tsp_raw_data_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size > sec_tsp_raw_data_size - 1) {
		len = scnprintf(&sec_tsp_raw_data_buf[0],
				size + 1, "%s\n", buf);
		sec_tsp_raw_data_index = len;
	} else {
		len = scnprintf(&sec_tsp_raw_data_buf[idx], size + 1, "%s\n", buf);
		sec_tsp_raw_data_index += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_raw_data);

void sec_debug_tsp_log_msg(char *msg, char *fmt, ...)
{
	va_list args;
	char buf[TSP_BUF_SIZE];
	int len = 0;
	unsigned int idx;
	unsigned int size;
	unsigned int size_dev_name;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_log_size)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = sec_tsp_log_index;
	size = strlen(buf);
	size_dev_name = strlen(msg);

	idx = sec_tsp_log_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size + size_dev_name + 3 + 1 > sec_tsp_log_size) {
		if (sec_tsp_log_index_fix + size + size_dev_name
				> sec_tsp_log_size - 1)
			return;
		len = scnprintf(&sec_tsp_log_buf[sec_tsp_log_index_fix],
			size + size_dev_name + 3 + 1, "%s : %s", msg, buf);
		sec_tsp_log_index = sec_tsp_log_index_fix + len;
	} else {
		len = scnprintf(&sec_tsp_log_buf[idx],
			size + size_dev_name + 3 + 1, "%s : %s", msg, buf);
		sec_tsp_log_index += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_log_msg);

void sec_debug_tsp_raw_data_msg(char *msg, char *fmt, ...)
{
	va_list args;
	char buf[TSP_BUF_SIZE];
	int len = 0;
	unsigned int idx;
	unsigned int size;
	unsigned int size_dev_name;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_raw_data_size || !sec_tsp_raw_data_buf)
		return;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	idx = sec_tsp_raw_data_index;
	size = strlen(buf);
	size_dev_name = strlen(msg);

	idx = sec_tsp_raw_data_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size + size_dev_name + 3 + 1 > sec_tsp_raw_data_size) {
		len = scnprintf(&sec_tsp_raw_data_buf[0],
			size + size_dev_name + 3 + 1, "%s : %s", msg, buf);
		sec_tsp_raw_data_index = len;
	} else {
		len = scnprintf(&sec_tsp_raw_data_buf[idx],
			size + size_dev_name + 3 + 1, "%s : %s", msg, buf);
		sec_tsp_raw_data_index += len;
	}
}
EXPORT_SYMBOL(sec_debug_tsp_raw_data_msg);

void sec_tsp_raw_data_clear(void)
{
	if (!sec_tsp_raw_data_size || !sec_tsp_raw_data_buf)
		return;

	sec_tsp_raw_data_index = 0;
	memset(sec_tsp_raw_data_buf, 0x00, sec_tsp_raw_data_size);
}
EXPORT_SYMBOL(sec_tsp_raw_data_clear);

void sec_tsp_log_fix(void)
{
	char *buf = "FIX LOG!\n";
	int len = 0;
	unsigned int idx;
	unsigned int size;

	/* In case of sec_tsp_log_setup is failed */
	if (!sec_tsp_log_size)
		return;

	idx = sec_tsp_log_index;
	size = strlen(buf);

	idx = sec_tsp_log_timestamp(idx);

	/* Overflow buffer size */
	if (idx + size > sec_tsp_log_size - 1) {
		if (sec_tsp_log_index_fix + size > sec_tsp_log_size - 1)
			return;
		len = scnprintf(&sec_tsp_log_buf[sec_tsp_log_index_fix],
				size + 1, "%s", buf);
		sec_tsp_log_index = sec_tsp_log_index_fix + len;
	} else {
		len = scnprintf(&sec_tsp_log_buf[idx], size + 1, "%s", buf);
		sec_tsp_log_index += len;
	}
	sec_tsp_log_index_fix = sec_tsp_log_index;
}
EXPORT_SYMBOL(sec_tsp_log_fix);

static ssize_t sec_tsp_log_write(struct file *file,
				const char __user *buf,
				size_t count, loff_t *ppos)
{
	char *page = NULL;
	ssize_t ret;
	int new_value;

	if (!sec_tsp_log_buf)
		return 0;

	ret = -EINVAL;
	if (count >= PAGE_SIZE)
		return ret;

	ret = -ENOMEM;
	page = (char *)get_zeroed_page(GFP_KERNEL);
	if (!page)
		return ret;

	ret = -EFAULT;
	if (copy_from_user(page, buf, count))
		goto out;

	ret = -EINVAL;
	if (sscanf(page, "%u", &new_value) != 1) {
		pr_info("%s\n", page);
		/* print tsp_log to sec_tsp_log_buf */
		sec_debug_tsp_log("%s", page);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

static ssize_t sec_tsp_raw_data_write(struct file *file,
				const char __user *buf,
				size_t count, loff_t *ppos)
{
	char *page = NULL;
	ssize_t ret;
	int new_value;

	if (!sec_tsp_raw_data_buf)
		return 0;

	ret = -EINVAL;
	if (count >= PAGE_SIZE)
		return ret;

	ret = -ENOMEM;
	page = (char *)get_zeroed_page(GFP_KERNEL);
	if (!page)
		return ret;

	ret = -EFAULT;
	if (copy_from_user(page, buf, count))
		goto out;

	ret = -EINVAL;
	if (sscanf(page, "%u", &new_value) != 1) {
		pr_info("%s\n", page);
		sec_debug_tsp_raw_data("%s", page);
	}
	ret = count;
out:
	free_page((unsigned long)page);
	return ret;
}

static ssize_t sec_tsp_log_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (!sec_tsp_log_buf)
		return 0;

	if (pos >= sec_tsp_log_index)
		return 0;

	count = min(len, (size_t)(sec_tsp_log_index - pos));
	if (copy_to_user(buf, sec_tsp_log_buf + pos, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static ssize_t sec_tsp_raw_data_read(struct file *file, char __user *buf,
					size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (!sec_tsp_raw_data_buf)
		return 0;

	if (pos >= sec_tsp_raw_data_index)
		return 0;

	count = min(len, (size_t)(sec_tsp_raw_data_index - pos));
	if (copy_to_user(buf, sec_tsp_raw_data_buf + pos, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static const struct file_operations tsp_msg_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_tsp_log_read,
	.write = sec_tsp_log_write,
	.llseek = generic_file_llseek,
};

static const struct file_operations tsp_raw_data_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_tsp_raw_data_read,
	.write = sec_tsp_raw_data_write,
	.llseek = generic_file_llseek,
};

static int __init sec_tsp_log_late_init(void)
{
	struct proc_dir_entry *entry;

	if (!sec_tsp_log_buf)
		return 0;

	entry = proc_create("tsp_msg", S_IFREG | S_IRUSR | S_IRGRP,
			NULL, &tsp_msg_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry of tsp_msg\n", __func__);
		return 0;
	}

	proc_set_size(entry, sec_tsp_log_size);
	return 0;
}
late_initcall(sec_tsp_log_late_init);

static int __init sec_tsp_raw_data_late_init(void)
{
	struct proc_dir_entry *entry;

	if (!sec_tsp_raw_data_buf)
		return 0;

	entry = proc_create("tsp_raw_data", S_IFREG | 0444,
			NULL, &tsp_raw_data_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry of tsp_raw_data\n", __func__);
		return 0;
	}

	proc_set_size(entry, sec_tsp_raw_data_size);

	return 0;
}
late_initcall(sec_tsp_raw_data_late_init);

static int __init __init_sec_tsp_log(void)
{
	char *vaddr;

	sec_tsp_log_size = SEC_TSP_LOG_BUF_SIZE;
	vaddr = kmalloc(sec_tsp_log_size, GFP_KERNEL);

	pr_info("%s: vaddr=0x%lx size=0x%x\n", __func__,
		(unsigned long)vaddr, sec_tsp_log_size);

	if (!vaddr) {
		pr_info("%s: ERROR! init failed!\n", __func__);
		return -ENOMEM;
	}

	sec_tsp_log_buf = vaddr;

	pr_info("%s: init done\n", __func__);

	return 0;
}
fs_initcall(__init_sec_tsp_log);	/* earlier than device_initcall */

static int __init __init_sec_tsp_raw_data(void)
{
	char *vaddr;

	sec_tsp_raw_data_size = SEC_TSP_RAW_DATA_BUF_SIZE;
	vaddr = kmalloc(sec_tsp_raw_data_size, GFP_KERNEL);

	pr_info("%s: vaddr=0x%lx size=0x%x\n", __func__,
		(unsigned long)vaddr, sec_tsp_raw_data_size);

	if (!vaddr) {
		pr_info("%s: ERROR! init failed!\n", __func__);
		return -ENOMEM;
	}

	sec_tsp_raw_data_buf = vaddr;

	pr_info("%s: init done\n", __func__);

	return 0;
}
fs_initcall(__init_sec_tsp_raw_data);	/* earlier than device_initcall */
#endif /* CONFIG_SEC_DEBUG_TSP_LOG */

