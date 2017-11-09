/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/syscalls.h>
#include <linux/vmalloc.h>

#include "vpu-config.h"
#include "vpu-binary.h"

static noinline_for_stack long __get_file_size(struct file *file)
{
	struct kstat st;

	if (vfs_getattr(&file->f_path, &st))
		return -1;
	if (!S_ISREG(st.mode))
		return -1;
	if (st.size != (long)st.size)
		return -1;

	return st.size;
}

int vpu_binary_init(struct vpu_binary *binary,
	struct device *dev,
	char *fpath1,
	char *fpath2,
	char *fname)
{
	int ret = 0;

	BUG_ON(!binary);
	BUG_ON(!dev);

	binary->dev = dev;
	binary->image_size = 0;
	snprintf(binary->fpath1, sizeof(binary->fpath1), "%s%s", fpath1, fname);
	snprintf(binary->fpath2, sizeof(binary->fpath2), "%s%s", fpath2, fname);

	return ret;
}

int vpu_binary_g_size(struct vpu_binary *binary, size_t *size)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *fp;
	size_t fsize = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(binary->fpath1, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		set_fs(old_fs);
		ret = -EINVAL;
		goto p_err;
	}

	fsize = __get_file_size(fp);
	if (fsize <= 0) {
		vpu_err("__get_file_size is fail(%ld)\n", fsize);
		filp_close(fp, current->files);
		set_fs(old_fs);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	*size = fsize;
	return ret;
}

int vpu_binary_read(struct vpu_binary *binary,
	void *target,
	size_t target_size)
{
	int ret = 0;
	const struct firmware *fw_blob;
	u8 *buf;
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;

	BUG_ON(!binary);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(binary->fpath1, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		fp = filp_open(binary->fpath2, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(fp)) {
			set_fs(old_fs);
			goto request_fw;
		}
	}

	fsize = __get_file_size(fp);
	if (fsize <= 0) {
		vpu_err("__get_file_size is fail(%ld)\n", fsize);
		ret = -EBADF;
		goto p_err;
	}

	buf = vmalloc(fsize);
	if (!buf) {
		vpu_err("vmalloc is fail\n");
		ret = -ENOMEM;
		goto p_err;
	}

	nread = kernel_read(fp, 0, buf, fsize);
	if (nread != fsize) {
		vpu_err("kernel_read is fail(%ld != %ld)\n", nread, fsize);
		ret = -EIO;
		goto p_err;
	}

	if (fsize > target_size) {
		vpu_err("image size is over(%ld > %ld)\n", fsize, target_size);
		ret = -EIO;
		goto p_err;
	}

	binary->image_size = fsize;
	/* no cache operation, because target is sram of vpu */
#ifdef CONFIG_EXYNOS_VPU_HARDWARE
	memcpy(target, (void *)buf, fsize);
#endif
	vpu_info("FW(%s, %ld) were applied successfully.\n", binary->fpath1, fsize);

p_err:
	vfree(buf);
	filp_close(fp, current->files);
	set_fs(old_fs);

	return ret;

request_fw:
	ret = request_firmware(&fw_blob, VPU_FW_NAME, binary->dev);
	if (ret) {
		vpu_err("request_firmware(%s) is fail(%d)", binary->fpath2, ret);
		ret = -EINVAL;
		goto request_err;
	}

	if (!fw_blob) {
		vpu_err("fw_blob is NULL\n");
		ret = -EINVAL;
		goto request_err;
	}

	if (!fw_blob->data) {
		vpu_err("fw_blob->data is NULL\n");
		ret = -EINVAL;
		goto request_err;
	}

	if (fw_blob->size > target_size) {
		vpu_err("image size is over(%ld > %ld)\n", binary->image_size, target_size);
		ret = -EIO;
		goto p_err;
	}

	binary->image_size = fw_blob->size;
#ifdef CONFIG_EXYNOS_VPU_HARDWARE
	memcpy(target, fw_blob->data, fw_blob->size);
#endif
	vpu_info("Binay(%s, %ld) were applied successfully.\n", binary->fpath2, fw_blob->size);

request_err:
	release_firmware(fw_blob);
	return ret;
}

int vpu_binary_write(struct vpu_binary *binary,
	void *target,
	size_t target_size)
{
	int ret = 0;
	struct file *fp;
	mm_segment_t old_fs;
	long nwrite;

	BUG_ON(!binary);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(binary->fpath1, O_RDWR | O_CREAT, 0);
	if (IS_ERR_OR_NULL(fp)) {
		set_fs(old_fs);
		vpu_err("filp_open is fail(%p)\n", fp);
		ret = -EBADF;
		goto p_err;
	}

	nwrite = kernel_write(fp, target, target_size, 0);
	if (nwrite != target_size) {
		filp_close(fp, current->files);
		set_fs(old_fs);
		vpu_err("kernel_write is fail(%ld != %ld)\n", nwrite, target_size);
		ret = -EIO;
		goto p_err;
	}

	binary->image_size = target_size;

	vpu_info("Binay(%s, %ld) were applied successfully.\n", binary->fpath1, target_size);

	filp_close(fp, current->files);
	set_fs(old_fs);

p_err:
	return ret;
}