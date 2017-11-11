/* linux/drivers/video/mdnie_tuning.c
 *
 * Register interface file for Samsung mDNIe driver
 *
 * Copyright (c) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/rtc.h>
#include <linux/fb.h>
//#include <mach/gpio.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/magic.h>

#include "mdnie.h"

int mdnie_calibration(int *r)
{
	int ret = 0;

	if (r[1] > 0) {
		if (r[3] > 0)
			ret = 3;
		else
			ret = (r[4] < 0) ? 1 : 2;
	} else {
		if (r[2] < 0) {
			if (r[3] > 0)
				ret = 9;
			else
				ret = (r[4] < 0) ? 7 : 8;
		} else {
			if (r[3] > 0)
				ret = 6;
			else
				ret = (r[4] < 0) ? 4 : 5;
		}
	}

	pr_info("%d, %d, %d, %d, tune%d\n", r[1], r[2], r[3], r[4], ret);

	return ret;
}

int mdnie_open_file(const char *path, char **fp)
{
	struct file *filp;
	char	*dp;
	long	length;
	int     ret;
	struct super_block *sb;
	loff_t  pos = 0;

	if (!path) {
		pr_err("%s: path is invalid\n", __func__);
		return -EPERM;
	}

	filp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("%s: filp_open skip: %s\n", __func__, path);
		return -EPERM;
	}

	length = i_size_read(filp->f_path.dentry->d_inode);
	if (length <= 0) {
		pr_err("%s: file length is %ld\n", __func__, length);
		return -EPERM;
	}

	dp = kzalloc(length, GFP_KERNEL);
	if (dp == NULL) {
		pr_err("%s: fail to alloc size %ld\n", __func__, length);
		filp_close(filp, current->files);
		return -EPERM;
	}

	ret = kernel_read(filp, pos, dp, length);
	if (ret != length) {
		/* check node is sysfs, bus this way is not safe */
		sb = filp->f_path.dentry->d_inode->i_sb;
		if ((sb->s_magic != SYSFS_MAGIC) && (length != sb->s_blocksize)) {
			pr_err("%s: read size= %d, length= %ld\n", __func__, ret, length);
			kfree(dp);
			filp_close(filp, current->files);
			return -EPERM;
		}
	}

	filp_close(filp, current->files);

	*fp = dp;

	return ret;
}

int mdnie_check_firmware(const char *path, char *name)
{
	char *ptr = NULL;
	int ret = 0, size;

	size = mdnie_open_file(path, &ptr);
	if (IS_ERR_OR_NULL(ptr) || size <= 0) {
		pr_err("%s: file open skip %s\n", __func__, path);
		kfree(ptr);
		return -EPERM;
	}

	ret = (strstr(ptr, name) != NULL) ? 1 : 0;

	kfree(ptr);

	return ret;
}

static int mdnie_request_firmware(char *path, char *name, mdnie_t **buf)
{
	char *token, *ptr = NULL;
	int ret = 0, size;
	unsigned int data[2], i = 0, j;
	mdnie_t *dp;

	size = mdnie_open_file(path, &ptr);
	if (IS_ERR_OR_NULL(ptr) || size <= 0) {
		pr_err("%s: file open skip %s\n", __func__, path);
		kfree(ptr);
		return -EPERM;
	}

	dp = kzalloc(size, GFP_KERNEL);
	if (dp == NULL) {
		pr_err("%s: fail to alloc size %d\n", __func__, size);
		kfree(ptr);
		return -ENOMEM;
	}

	if (name) {
		if (strstr(ptr, name) != NULL) {
			pr_info("found %s in %s\n", name, path);
			ptr = strstr(ptr, name);
		}
	}

	while ((token = strsep(&ptr, "\r\n")) != NULL) {
		if (name && !strncmp(token, "};", 2)) {
			pr_info("found %s end in local, stop searching\n", name);
			break;
		}
		ret = sscanf(token, "%x, %x", &data[0], &data[1]);
		for (j = 0; j < ret; i++, j++)
			dp[i] = data[j];
	}

	*buf = dp;

	kfree(ptr);

	return i;
}

struct mdnie_table *mdnie_request_table(char *path, struct mdnie_table *s)
{
	char string[50];
	unsigned int i, j, size;
	mdnie_t *buf = NULL;
	struct mdnie_table *t;
	int ret = 0;

	ret = mdnie_check_firmware(path, s->name);
	if (ret < 0)
		return NULL;

	t = kzalloc(sizeof(struct mdnie_table), GFP_KERNEL);
	t->name = kzalloc(strlen(s->name) + 1, GFP_KERNEL);
	strcpy(t->name, s->name);
	memcpy(t, s, sizeof(struct mdnie_table));

	if (ret > 0) {
		for (j = 1, i = MDNIE_CMD1; i <= MDNIE_CMD2; i++, j++) {
			memset(string, 0, sizeof(string));
			sprintf(string, "%s_%d", t->name, j);
			size = mdnie_request_firmware(path, string, &buf);
			t->tune[i].sequence = buf;
			t->tune[i].size = size;
			pr_info("%s: size is %d\n", string, t->tune[i].size);
		}
	} else if (ret == 0) {
		size = mdnie_request_firmware(path, NULL, &buf);
#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
		for (i = 0; i < size; i++) {
			if (buf[i] == 0xEC)
				break;
		}
		t->tune[MDNIE_CMD1].sequence = &buf[0];
		t->tune[MDNIE_CMD1].size = i;
#endif
		t->tune[MDNIE_CMD2].sequence = &buf[i];
		t->tune[MDNIE_CMD2].size = size - i + 1;
	}

	/* for (i = 0; i < MDNIE_CMD_MAX; i++) {
		pr_info("%d: size is %d\n", i, t->tune[i].size);
		for (j = 0; j < t->tune[i].size; j++)
			pr_info("%d: %03d: %02x\n", i, j, t->tune[i].sequence[j]);
	} */

	return t;
}
