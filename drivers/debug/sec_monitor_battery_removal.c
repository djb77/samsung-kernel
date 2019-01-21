/*
 * drivers/debug/sec_monitor_battery_removal.c
 *
 * COPYRIGHT(C) 2016 Samsung Electronics Co., Ltd. All Right Reserved.
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

#include <linux/fs.h>
#include <linux/sec_param.h>
#include <linux/debugfs.h>
#include <linux/sec_monitor_battery_removal.h>

static unsigned long long normal_off;

static int __init power_normal_off(char *val)
{
	normal_off = strncmp(val, "1", 1) ? ABNORMAL_POWEROFF : NORMAL_POWEROFF;
	pr_info("%s, normal_off:%llu\n", __func__, normal_off);
	return 1;
}
__setup("normal_off=", power_normal_off);

void sec_set_normal_pwroff(int value)
{
	int normal_poweroff = value;

	pr_info("%s, value :%d\n", __func__, value);
	sec_set_param(param_index_normal_poweroff, &normal_poweroff);
}
EXPORT_SYMBOL(sec_set_normal_pwroff);

static int sec_get_normal_pwroff(void *data, u64 *val)
{
	*val = normal_off;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(normal_off_fops, sec_get_normal_pwroff, NULL, "%llu\n");

#ifdef CONFIG_DEBUG_FS
static int __init sec_logger_init(void)
{
	struct dentry *dent;
	struct dentry *dbgfs_file;

	dent = debugfs_create_dir("sec_logger", NULL);
	if (IS_ERR_OR_NULL(dent)) {
		pr_err("Failed to create debugfs dir of sec_logger\n");
		return PTR_ERR(dent);
	}

	dbgfs_file = debugfs_create_file("normal_off",
				 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, dent,
				 NULL, &normal_off_fops);
	if (IS_ERR_OR_NULL(dbgfs_file)) {
		pr_err("Failed to create debugfs file of normal_off file\n");
		debugfs_remove_recursive(dent);
		return PTR_ERR(dbgfs_file);
	}

	return 0;
}
late_initcall(sec_logger_init);
#endif
