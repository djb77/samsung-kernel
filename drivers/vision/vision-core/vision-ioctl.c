/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#include "vision-config.h"
#include "vision-dev.h"
#include "vision-ioctl.h"

long vertex_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct vision_device *vdev = vision_devdata(file);
	const struct vertex_ioctl_ops *ops = vdev->ioctl_ops;
	char sbuf[128];
	void *mbuf = NULL;
	void *parg = (void *)arg;

	if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
		parg = sbuf;
	} else {
		/* too big to allocate from stack */
		mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
		if (NULL == mbuf) {
			vision_err("kmalloc is fail\n");
			ret = -ENOMEM;
			goto p_err;
		}

		parg = mbuf;
	}

	ret = copy_from_user(parg, (void __user *)arg, _IOC_SIZE(cmd));
	if (ret) {
		vision_err("copy_from_user is fail(%d)\n", ret);
		goto p_err;
	}

	switch (cmd) {
	case VS4L_VERTEXIOC_S_GRAPH:
		{
			ret = ops->vertexioc_s_graph(file, parg);
			if (ret) {
				vision_err("vertexioc_s_graph is fail(%d)\n", ret);
				goto p_err;
			}
		}
		break;
	case VS4L_VERTEXIOC_S_FORMAT:
		{
			ret = ops->vertexioc_s_format(file, parg);
			if (ret) {
				vision_err("vertexioc_s_format is fail(%d)\n", ret);
				goto p_err;
			}
		}
		break;
	case VS4L_VERTEXIOC_S_PARAM:
		{
			ret = ops->vertexioc_s_param(file, parg);
			if (ret) {
				vision_err("vertexioc_s_param is fail(%d)\n", ret);
				goto p_err;
			}
		}
		break;
	case VS4L_VERTEXIOC_S_CTRL:
		{
			ret = ops->vertexioc_s_ctrl(file, parg);
			if (ret) {
				vision_err("vertexioc_s_ctrl is fail(%d)\n", ret);
				goto p_err;
			}
		}
		break;
	case VS4L_VERTEXIOC_STREAM_ON:
		{
			ret = ops->vertexioc_streamon(file);
			if (ret) {
				vision_err("vertexioc_streamon is fail(%d)\n", ret);
				goto p_err;
			}
		}
		break;
	case VS4L_VERTEXIOC_STREAM_OFF:
		{
			ret = ops->vertexioc_streamoff(file);
			if (ret) {
				vision_err("vertexioc_streamoff is fail(%d)\n", ret);
				goto p_err;
			}
		}
		break;
	case VS4L_VERTEXIOC_QBUF:
		{
#if 0
			ret = ops->vertexioc_qbuf(file, parg);
			if (ret) {
				vision_err("vertexioc_qbuf is fail(%d)\n", ret);
				goto p_err;
			}
#endif
			ret = -EINVAL;
			vision_info("SCore:qbuf is not supported at 64-bit\n");
		}
		break;
	case VS4L_VERTEXIOC_DQBUF:
		{
			ret = ops->vertexioc_dqbuf(file, parg);
			if (ret) {
				vision_err("vertexioc_dqbuf is fail(%d)\n", ret);
				goto p_err;
			}
		}
		break;
	default:
		vision_err("%x iocontrol is not supported(%lx, %zd)\n", cmd, VS4L_VERTEXIOC_S_FORMAT, sizeof(struct vs4l_format_list));
		break;
	}

p_err:
	kfree(mbuf);
	return ret;
}
