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
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/bug.h>
#include <asm/uaccess.h>
#include <media/videobuf2-ion.h>
#include <linux/exynos_iovmm.h>

#include "vpu-config.h"
#include "vpu-binary.h"
#include "vpu-exynos.h"
#include "vpu-memory.h"
#include "vpu-debug.h"
#include "vpu-tv.h"

int vpu_tv_register_depth3(struct vpu_tvset *tvset)
{
	int ret = 0;
	struct vpu_tv *tv;

	tv = &tvset->tv[tvset->tv_cnt];
	tv->in_cnt = 3;
	tv->ot_cnt = 1;
	tv->id = tvset->tv_cnt;

	vpu_binary_init(&tv->regs, tvset->dev, VPU_FW_PATH1, NULL, "depth3_regs.bin");
	vpu_binary_init(&tv->in[0], tvset->dev, VPU_FW_PATH1, NULL, "depth3_disparity.bin");
	vpu_binary_init(&tv->in[1], tvset->dev, VPU_FW_PATH1, NULL, "depth3_left.bin");
	vpu_binary_init(&tv->in[2], tvset->dev, VPU_FW_PATH1, NULL, "depth3_right.bin");
	vpu_binary_init(&tv->ot[0], tvset->dev, VPU_FW_PATH1, NULL, "depth3_output.bin");
	vpu_binary_init(&tv->gd[0], tvset->dev, VPU_FW_PATH1, NULL, "depth3_golden.bin");

	tvset->tv_cnt++;

	return ret;
}

int vpu_tv_do_depth3(struct vpu_tvset *tvset)
{
	int ret = 0;
	struct vpu_tv *tv;
	struct vpu_exynos *exynos;
	struct vpu_memory *memory;
	void *cookie, *kvaddr;
	dma_addr_t dvaddr;
	u32 *reg_set, offset;
	u32 i;

	exynos = tvset->exynos;
	memory = tvset->memory;
	tv = &tvset->tv[0];

	ret = vpu_binary_g_size(&tv->regs, &tv->regs_size);
	if (ret) {
		vpu_err("vpu_binary_g_size is fail(%d)\n", ret);
		goto p_err;
	}

	tv->regs_buffer = kmalloc(tv->regs_size, GFP_KERNEL);
	if (!tv->regs_buffer) {
		vpu_err("kmalloc is fail\n");
		goto p_err;
	}

	ret = vpu_binary_read(&tv->regs, tv->regs_buffer, tv->regs_size);
	if (ret) {
		vpu_err("vpu_binary_read is fail(%d)\n", ret);
		goto p_err;
	}

	for (i = 0; i < tv->in_cnt; ++i) {
		ret = vpu_binary_g_size(&tv->in[i], &tv->in_size[i]);
		if (ret) {
			vpu_err("vpu_binary_g_size is fail(%d)\n", ret);
			goto p_err;
		}

		cookie = vb2_ion_private_alloc(memory->alloc_ctx, tv->in_size[i]);
		if (IS_ERR(cookie)) {
			vpu_err("vb2_ion_private_alloc is failed");
			ret = PTR_ERR(cookie);
			goto p_err;
		}

		ret = vb2_ion_dma_address(cookie, &dvaddr);
		if (ret) {
			vpu_err("vb2_ion_dma_address is fail(%d)\n", ret);
			vb2_ion_private_free(cookie);
			goto p_err;
		}

		kvaddr = vb2_ion_private_vaddr(cookie);
		if (IS_ERR_OR_NULL(kvaddr)) {
			vpu_err("vb2_ion_private_vaddr is failed");
			vb2_ion_private_free(cookie);
			ret = PTR_ERR(kvaddr);
			goto p_err;
		}

		tv->in_cookie[i] = cookie;
		tv->in_dvaddr[i] = dvaddr;
		tv->in_kvaddr[i] = kvaddr;

		ret = vpu_binary_read(&tv->in[i], tv->in_kvaddr[i], tv->in_size[i]);
		if (ret) {
			vpu_err("vpu_binary_read is fail(%d)\n", ret);
			goto p_err;
		}
	}

	for (i = 0; i < tv->ot_cnt; ++i) {
		ret = vpu_binary_g_size(&tv->gd[i], &tv->ot_size[i]);
		if (ret) {
			vpu_err("vpu_binary_g_size is fail(%d)\n", ret);
			goto p_err;
		}

		cookie = vb2_ion_private_alloc(memory->alloc_ctx, tv->ot_size[i]);
		if (IS_ERR(cookie)) {
			vpu_err("vb2_ion_private_alloc is failed");
			ret = PTR_ERR(cookie);
			goto p_err;
		}

		ret = vb2_ion_dma_address(cookie, &dvaddr);
		if (ret) {
			vpu_err("vb2_ion_dma_address is fail(%d)\n", ret);
			vb2_ion_private_free(cookie);
			goto p_err;
		}

		kvaddr = vb2_ion_private_vaddr(cookie);
		if (IS_ERR_OR_NULL(kvaddr)) {
			vpu_err("vb2_ion_private_vaddr is failed");
			vb2_ion_private_free(cookie);
			ret = PTR_ERR(kvaddr);
			goto p_err;
		}

		tv->ot_cookie[i] = cookie;
		tv->ot_dvaddr[i] = dvaddr;
		tv->ot_kvaddr[i] = kvaddr;

		memset(kvaddr, 0, tv->ot_size[i]);

		tv->ot_golden[i] = kmalloc(tv->ot_size[i], GFP_KERNEL);
		if (!tv->ot_golden[i]) {
			vpu_err("kmalloc is fail\n");
			goto p_err;
		}

		ret = vpu_binary_read(&tv->gd[i], tv->ot_golden[i], tv->ot_size[i]);
		if (ret) {
			vpu_err("vpu_binary_read is fail(%d)\n", ret);
			goto p_err;
		}
	}

	reg_set = tv->regs_buffer;

	for (i = 0; i < (tv->regs_size / 8); ++i) {
		switch (reg_set[i * 2]) {
		case 0xf0021004: /* 0x34000000 : left(1) */
		case 0xf0021204:
		case 0xf0031004:
		case 0xf0031204:
			reg_set[i * 2 + 1] = tv->in_dvaddr[1];
			break;
		case 0xf0021104: /* 0x33500000 : right(2) */
		case 0xf0031104:
			reg_set[i * 2 + 1] = tv->in_dvaddr[2];
			break;
		case 0xf0021304: /* 0x34500000 : disparity(0) */
		case 0xf0031304:
			reg_set[i * 2 + 1] = tv->in_dvaddr[0];
			break;
		case 0xf0022304: /* 0x35000000 : output(0) */
		case 0xf0032304:
			reg_set[i * 2 + 1] = tv->ot_dvaddr[0];
			break;
		default:
			break;
		}

		offset = reg_set[i * 2] & 0xFFFFF;
#ifdef CONFIG_EXYNOS_VPU_HARDWARE
		writel(reg_set[i * 2 + 1], exynos->regbase + offset);
#endif
	}

	ret = CTL_OP(exynos, ctl_trigger, 0xF);
	if (ret) {
		vpu_err("ctl_trigger is fail(%d)\n", ret);
		goto p_err;
	}

	ret = CTL_OP(exynos, ctl_start, 0xF);
	if (ret) {
		vpu_err("ctl_start is fail(%d)\n", ret);
		goto p_err;
	}

	for (i = 0; i < (tv->regs_size / 8); ++i) {
		offset = reg_set[i * 2] & 0xFFFFF;
#ifdef CONFIG_EXYNOS_VPU_HARDWARE
		writel(reg_set[i * 2 + 1], exynos->regbase + offset);
#endif
	}

	ret = CTL_OP(exynos, ctl_trigger, 0xF);
	if (ret) {
		vpu_err("ctl_trigger is fail(%d)\n", ret);
		goto p_err;
	}

	ret = CTL_OP(exynos, ctl_start, 0xF);
	if (ret) {
		vpu_err("ctl_start is fail(%d)\n", ret);
		goto p_err;
	}

	for (i = 0; i < tv->ot_cnt; ++i) {
		ret = memcmp(tv->ot_golden[i], tv->ot_kvaddr[i], tv->ot_size[i]);
		if (ret) {
			vpu_info("memcmp is fail(ret : %d, size : %zd, target : %p)\n",
				ret, tv->ot_size[i], tv->ot_kvaddr[i]);
			vpu_binary_write(&tv->ot[i], tv->ot_kvaddr[i], tv->ot_size[i]);
		}
	}

	kfree(tv->regs_buffer);

	for (i = 0; i < tv->in_cnt; ++i)
		vb2_ion_private_free(tv->in_cookie[i]);

	for (i = 0; i < tv->ot_cnt; ++i) {
		vb2_ion_private_free(tv->ot_cookie[i]);
		kfree(tv->ot_golden[i]);
	}

p_err:
	return ret;
}

int vpu_tv_register_flamorb(struct vpu_tvset *tvset)
{
	int ret = 0;
	struct vpu_tv *tv;

	tv = &tvset->tv[tvset->tv_cnt];
	tv->in_cnt = 2;
	tv->ot_cnt = 1;
	tv->id = tvset->tv_cnt;

	vpu_binary_init(&tv->regs, tvset->dev, VPU_FW_PATH1, NULL, "flamorb_regs.bin");
	vpu_binary_init(&tv->in[0], tvset->dev, VPU_FW_PATH1, NULL, "flamorb_input0.bin");
	vpu_binary_init(&tv->in[1], tvset->dev, VPU_FW_PATH1, NULL, "flamorb_input1.bin");
	vpu_binary_init(&tv->ot[0], tvset->dev, VPU_FW_PATH1, NULL, "flamorb_output0.bin");
	vpu_binary_init(&tv->gd[0], tvset->dev, VPU_FW_PATH1, NULL, "flamorb_golden0.bin");

	tvset->tv_cnt++;

	return ret;
}

int vpu_tv_do_flamorb(struct vpu_tvset *tvset)
{
	int ret = 0;
	struct vpu_tv *tv;
	struct vpu_exynos *exynos;
	struct vpu_memory *memory;
	void *cookie, *kvaddr;
	dma_addr_t dvaddr;
	u32 *reg_set, *value;
	u32 i, offset;

	exynos = tvset->exynos;
	memory = tvset->memory;
	tv = &tvset->tv[1];

	ret = vpu_binary_g_size(&tv->regs, &tv->regs_size);
	if (ret) {
		vpu_err("vpu_binary_g_size is fail(%d)\n", ret);
		goto p_err;
	}

	tv->regs_buffer = kmalloc(tv->regs_size, GFP_KERNEL);
	if (!tv->regs_buffer) {
		vpu_err("kmalloc is fail\n");
		goto p_err;
	}

	ret = vpu_binary_read(&tv->regs, tv->regs_buffer, tv->regs_size);
	if (ret) {
		vpu_err("vpu_binary_read is fail(%d)\n", ret);
		goto p_err;
	}

	for (i = 0; i < tv->in_cnt; ++i) {
		ret = vpu_binary_g_size(&tv->in[i], &tv->in_size[i]);
		if (ret) {
			vpu_err("vpu_binary_g_size is fail(%d)\n", ret);
			goto p_err;
		}

		cookie = vb2_ion_private_alloc(memory->alloc_ctx, tv->in_size[i]);
		if (IS_ERR(cookie)) {
			vpu_err("vb2_ion_private_alloc is failed");
			ret = PTR_ERR(cookie);
			goto p_err;
		}

		ret = vb2_ion_dma_address(cookie, &dvaddr);
		if (ret) {
			vpu_err("vb2_ion_dma_address is fail(%d)\n", ret);
			vb2_ion_private_free(cookie);
			goto p_err;
		}

		kvaddr = vb2_ion_private_vaddr(cookie);
		if (IS_ERR_OR_NULL(kvaddr)) {
			vpu_err("vb2_ion_private_vaddr is failed");
			vb2_ion_private_free(cookie);
			ret = PTR_ERR(kvaddr);
			goto p_err;
		}

		tv->in_cookie[i] = cookie;
		tv->in_dvaddr[i] = dvaddr;
		tv->in_kvaddr[i] = kvaddr;

		ret = vpu_binary_read(&tv->in[i], tv->in_kvaddr[i], tv->in_size[i]);
		if (ret) {
			vpu_err("vpu_binary_read is fail(%d)\n", ret);
			goto p_err;
		}

		vb2_ion_sync_for_cpu(cookie, 0, tv->in_size[i], DMA_TO_DEVICE);
	}

	for (i = 0; i < tv->ot_cnt; ++i) {
		ret = vpu_binary_g_size(&tv->gd[i], &tv->ot_size[i]);
		if (ret) {
			vpu_err("vpu_binary_g_size is fail(%d)\n", ret);
			goto p_err;
		}

		cookie = vb2_ion_private_alloc(memory->alloc_ctx, tv->ot_size[i]);
		if (IS_ERR(cookie)) {
			vpu_err("vb2_ion_private_alloc is failed");
			ret = PTR_ERR(cookie);
			goto p_err;
		}

		ret = vb2_ion_dma_address(cookie, &dvaddr);
		if (ret) {
			vpu_err("vb2_ion_dma_address is fail(%d)\n", ret);
			vb2_ion_private_free(cookie);
			goto p_err;
		}

		kvaddr = vb2_ion_private_vaddr(cookie);
		if (IS_ERR_OR_NULL(kvaddr)) {
			vpu_err("vb2_ion_private_vaddr is failed");
			vb2_ion_private_free(cookie);
			ret = PTR_ERR(kvaddr);
			goto p_err;
		}

		tv->ot_cookie[i] = cookie;
		tv->ot_dvaddr[i] = dvaddr;
		tv->ot_kvaddr[i] = kvaddr;

		memset(kvaddr, 0, tv->ot_size[i]);
		vb2_ion_sync_for_cpu(cookie, 0, tv->ot_size[i], DMA_TO_DEVICE);

		tv->ot_golden[i] = kmalloc(tv->ot_size[i], GFP_KERNEL);
		if (!tv->ot_golden[i]) {
			vpu_err("kmalloc is fail\n");
			goto p_err;
		}

		ret = vpu_binary_read(&tv->gd[i], tv->ot_golden[i], tv->ot_size[i]);
		if (ret) {
			vpu_err("vpu_binary_read is fail(%d)\n", ret);
			goto p_err;
		}
	}

	reg_set = tv->regs_buffer;

	writel(1, exynos->regbase + 0x410e0);
	writel(1, exynos->regbase + 0x410e4);
	writel(1, exynos->regbase + 0x410e8);
	writel(0xE0, exynos->regbase + 0x410ec);

	ret = CTL_OP(exynos, ctl_trigger, 1);
	if (ret) {
		vpu_err("ctl_trigger is fail(%d)\n", ret);
		goto p_err;
	}

	/* MPRB loading */
	value = (u32 *)tv->in_kvaddr[1];
	for (i = 0; i < (tv->in_size[1] / 4); ++i) {
		offset = i << 3;
		writel(value[i], exynos->ram0base + offset);
	}

	for (i = 0; i < (tv->regs_size / 8); ++i) {
		switch (reg_set[i * 2]) {
		case 0xf0021004: /* 0x30000000 : input0 */
		case 0xf0031004:
			reg_set[i * 2 + 1] = tv->in_dvaddr[0];
			break;
		case 0xf0022004: /* 0x30989680 : output(0) */
		case 0xf0032004:
			reg_set[i * 2 + 1] = tv->ot_dvaddr[0];
			break;
		default:
			break;
		}

		offset = reg_set[i * 2] & 0xFFFFF;
#ifdef CONFIG_EXYNOS_VPU_HARDWARE
		writel(reg_set[i * 2 + 1], exynos->regbase + offset);
#endif
	}

	ret = CTL_OP(exynos, ctl_trigger, 0xF);
	if (ret) {
		vpu_err("ctl_trigger is fail(%d)\n", ret);
		goto p_err;
	}

	ret = CTL_OP(exynos, ctl_start, 0xF);
	if (ret) {
		vpu_err("ctl_start is fail(%d)\n", ret);
		goto p_err;
	}

	for (i = 0; i < tv->ot_cnt; ++i) {
		ret = memcmp(tv->ot_golden[i], tv->ot_kvaddr[i], tv->ot_size[i]);
		if (ret) {
			vpu_info("memcmp is fail(ret : %d, size : %zd, target : %p)\n",
				ret, tv->ot_size[i], tv->ot_kvaddr[i]);
			vpu_binary_write(&tv->ot[i], tv->ot_kvaddr[i], tv->ot_size[i]);
		}
	}

	kfree(tv->regs_buffer);

	for (i = 0; i < tv->in_cnt; ++i)
		vb2_ion_private_free(tv->in_cookie[i]);

	for (i = 0; i < tv->ot_cnt; ++i) {
		vb2_ion_private_free(tv->ot_cookie[i]);
		kfree(tv->ot_golden[i]);
	}

p_err:
	return ret;
}
