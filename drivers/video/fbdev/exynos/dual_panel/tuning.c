/*
 * linux/drivers/video/fbdev/exynos/panel/tuning.c
 *
 * Samsung Common LCD Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
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

#include "panel_drv.h"
#include "mdnie.h"
#include "tuning.h"

static int tfile_open(const char *path, char **fp)
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

	dp = kzalloc(length * sizeof(u32), GFP_KERNEL);
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
			pr_err("%s: read size = %d, length= %ld\n", __func__, ret, length);
			kfree(dp);
			filp_close(filp, current->files);
			return -EPERM;
		}
	}

	filp_close(filp, current->files);

	*fp = dp;

	return ret;
}

static int tfile_get_type(const char *path, char *name)
{
	char *fp, *p = NULL;
	int count = 0, size;

	size = tfile_open(path, &fp);
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

int tfile_load(char *path, char *name, unsigned int **buf)
{
	char *token, *ptr = NULL;
	int ret = 0, size, data[2];
	unsigned int count = 0, i;
	unsigned int *dp;

	size = tfile_open(path, &ptr);
	if (IS_ERR_OR_NULL(ptr) || size <= 0) {
		pr_err("%s: file open skip %s\n", __func__, path);
		kfree(ptr);
		return -EPERM;
	}

	dp = kzalloc(size * sizeof(u32), GFP_KERNEL);
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
			if (ret < 0 || ret > size - count)
				break;
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

int tfile_do_seqtbl(struct panel_device *panel, char *path)
{
	unsigned int *buf = NULL;
	unsigned int i, stt, pos, dlen;
	int ret, size;
	u8 *data;
	struct pktinfo *pktinfo;
	char *name;
	static void *tuning_cmdtbl[128];
	struct seqinfo seqtbl = SEQINFO_INIT("tuning-seq", tuning_cmdtbl);

	if (!IS_PANEL_ACTIVE(panel))
		return 0;

	ret = tfile_get_type(path, seqtbl.name);
	if (ret < 0) {
		pr_err("%s, invalid type %d\n", __func__, ret);
		return -EINVAL;
	}

	size = tfile_load(path, ret ? seqtbl.name : NULL, &buf);
	if (size <= 0) {
		if (buf)
			kfree(buf);
		return -EINVAL;
	}

	data = kmalloc(size * sizeof(u8), GFP_KERNEL);
	for (i = 0, stt = 0, pos = 0; pos < size; pos++) {
		data[pos] = (u8)buf[pos];
		if (pos + 1 == size || buf[pos + 1] == 0xFFFF) {
			pos++;
			dlen = pos - stt;
			name = kasprintf(GFP_KERNEL, "tuning-packet-%d", i);
			pktinfo = alloc_static_packet(name, DSI_PKT_TYPE_WR, &data[stt], dlen);
			if (!pktinfo) {
				pr_err("%s, failed to alloc static packet\n", __func__);
				return -ENOMEM;
			}
			stt = pos + 1;
			seqtbl.cmdtbl[i++] = pktinfo;
		}
	}
	seqtbl.cmdtbl[i] = NULL;
	kfree(buf);

	mutex_lock(&panel->op_lock);

	panel_set_key(panel, 3, true);
	panel_do_seqtbl(panel, &seqtbl);
	panel_set_key(panel, 3, false);

	mutex_unlock(&panel->op_lock);
	i = 0;
	while (seqtbl.cmdtbl[i]) {
		kfree(((struct cmdinfo *)seqtbl.cmdtbl[i])->name);
		kfree(seqtbl.cmdtbl[i]);
		i++;
	}
	kfree(data);

	return 0;
}
