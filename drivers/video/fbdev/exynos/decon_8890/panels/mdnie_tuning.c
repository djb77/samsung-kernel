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
	char *fp, *p = NULL;
	int count = 0, size;

	size = mdnie_open_file(path, &fp);
	if (IS_ERR_OR_NULL(fp) || size <= 0) {
		pr_err("%s: file open skip %s\n", __func__, path);
		kfree(p);
		return -EPERM;
	}

	p = fp;
	while ((p = strstr(p, name)) != NULL) {
		count++;
		p++;
	}

	kfree(fp);

	pr_info("%s found %d times in %s\n", name, count, path);

	/* if count is 1, it means tuning header. if count is 0, it means tuning data */
	return count;
}

static int mdnie_request_firmware(char *path, char *name, unsigned int **buf)
{
	char *token, *ptr = NULL;
	int ret = 0, size, data[2];
	unsigned int count = 0, i;
	unsigned int *dp;

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

	while (!IS_ERR_OR_NULL(ptr)) {
		ptr = (name) ? strstr(ptr, name) : ptr;
		while ((token = strsep(&ptr, "\n")) != NULL) {
			ret = sscanf(token, "%i, %i", &data[0], &data[1]);
			pr_info("sscanf: %2d, strlen: %2d, %s\n", ret, (int)strlen(token), token);
			if (!ret && strlen(token) <= 1) {
				dp[count] = 0xffff;
				pr_info("stop at %d\n", count);
				if (count)
					count = (dp[count - 1] == dp[count]) ? count : count + 1;
				break;
			}
			for (i = 0; i < ret; count++, i++)
				dp[count] = data[i];
		}
	}

	*buf = dp;

	for (i = 0; i < count; i++)
		pr_info("[%4d] %04x\n", i, dp[i]);

	kfree(ptr);

	return count;
}

/* this function is not official one. its only purpose is for tuning */
uintptr_t mdnie_request_table(char *path, struct mdnie_table *org)
{
	unsigned int i, j = 0, k = 0, len;
	unsigned int *buf = NULL;
	mdnie_t *cmd = 0;
	int size, ret = 0, cmd_found = 0;

	ret = mdnie_check_firmware(path, org->name);
	if (ret < 0)
		goto exit;

	size = mdnie_request_firmware(path, ret ? org->name : NULL, &buf);
	if (size <= 0)
		goto exit;

	cmd = kzalloc(size * sizeof(mdnie_t), GFP_KERNEL);
	if (IS_ERR_OR_NULL(cmd))
		goto exit;

	for (i = 0; org->seq[i].len; i++) {
		if (!org->update_flag[i])
			continue;
		for (len = 0; k < size; len++, j++, k++) {
			if (buf[k] == 0xffff) {
				pr_info("stop at %d, %d, %d\n", k, j, len);
				k++;
				break;
			}
			cmd[j] = buf[k];
			pr_info("seq[%d].len[%3d], cmd[%3d]: %02x, buf[%3d]: %02x\n", i, len, j, cmd[j], k, buf[k]);
		}
		org->seq[i].cmd = &cmd[j - len];
		org->seq[i].len = len;
		cmd_found = 1;
		pr_info("seq[%d].cmd: &cmd[%3d], seq[%d].len: %d\n", i, j - len, i, len);
	}

	kfree(buf);

	if (!cmd_found) {
		kfree(cmd);
		cmd = 0;
	}

	for (i = 0; org->seq[i].len; i++) {
		pr_info("%d: size is %d\n", i, org->seq[i].len);
		for (j = 0; j < org->seq[i].len; j++)
			pr_info("%d: %03d: %02x\n", i, j, org->seq[i].cmd[j]);
	}

exit:
	/* return allocated address for prevent */
	return (uintptr_t)cmd;
}
