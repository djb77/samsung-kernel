/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_BINARY_H_
#define VPU_BINARY_H_

#include <linux/device.h>
#include <linux/firmware.h>

#define VPU_FW_PATH1 		"/data/"
#define VPU_FW_PATH2		"/system/vendor/firmware/"
#define VPU_FW_NAME		"vpu_fw.bin"
#define VPU_FW_NAME_LEN 	100
#define VPU_VERSION_SIZE	42

struct vpu_binary {
	struct device		*dev;
	char			fpath1[VPU_FW_NAME_LEN]; /* first try to load */
	char			fpath2[VPU_FW_NAME_LEN]; /* second try to load */
	size_t			image_size;
};

int vpu_binary_init(struct vpu_binary *binary,
	struct device *dev,
	char *fpath1,
	char *fpath2,
	char *fname);
int vpu_binary_read(struct vpu_binary *binary,
	void *target,
	size_t target_size);
int vpu_binary_write(struct vpu_binary *binary,
	void *target,
	size_t target_size);
int vpu_binary_g_size(struct vpu_binary *binary, size_t *size);

#endif