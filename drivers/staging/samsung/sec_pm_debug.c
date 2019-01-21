/*
 * sec_pm_debug.c
 *
 *  Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *  Minsung Kim <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>


#ifdef CONFIG_REGULATOR_S2MPS18
extern void s2mps18_get_pwr_onoffsrc(u8 *onsrc, u8 *offsrc);
#endif

enum OCP_INFO_TYPE {
	OCP_INFO_NO_OCP,
	OCP_INFO_OCP,
	OCP_INFO_OCP_OTG,
	OCP_INFO_MAX
};

static int ocp_info;

static int __init sec_pm_debug_ocp_info(char *buf)
{
	get_option(&buf, &ocp_info);
	return 0;
}

early_param("sec_pm_debug.ocp_info", sec_pm_debug_ocp_info);

static int sec_pm_debug_ocp_info_show(struct seq_file *m, void *v)
{
	u8 onsrc = 0, offsrc = 0;

#ifdef CONFIG_REGULATOR_S2MPS18
	s2mps18_get_pwr_onoffsrc(&onsrc, &offsrc);
#endif

	switch (ocp_info) {
	case OCP_INFO_OCP:
		seq_printf(m, "OCP(ON:0x%02X OFF:0x%02X)\n", onsrc, offsrc);
		break;
	case OCP_INFO_OCP_OTG:
		seq_printf(m, "OCP-OTG(ON:0x%02X OFF:0x%02X)\n", onsrc, offsrc);
		break;
	default:
		seq_printf(m, "No OCP(ON:0x%02X OFF:0x%02X)\n", onsrc, offsrc);
		break;
	}

	return 0;
}

static int ocp_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_pm_debug_ocp_info_show, inode->i_private);
}

const static struct file_operations ocp_info_fops = {
	.open		= ocp_info_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int sec_pm_debug_on_offsrc_show(struct seq_file *m, void *v)
{
	u8 onsrc = 0, offsrc = 0;

#ifdef CONFIG_REGULATOR_S2MPS18
	s2mps18_get_pwr_onoffsrc(&onsrc, &offsrc);
#endif
	seq_printf(m, "PWRONSRC:0x%02X OFFSRC:0x%02X\n", onsrc, offsrc);
	return 0;
}

static int on_offsrc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_pm_debug_on_offsrc_show, inode->i_private);
}

const static struct file_operations on_offsrc_fops = {
	.open		= on_offsrc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init sec_pm_debug_init(void)
{
	debugfs_create_file("ocp_info", 0444, NULL, NULL, &ocp_info_fops);
	debugfs_create_file("pwr_on_offsrc", 0444, NULL, NULL, &on_offsrc_fops);

	return 0;
}
late_initcall(sec_pm_debug_init);
