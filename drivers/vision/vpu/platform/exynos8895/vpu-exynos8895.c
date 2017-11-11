/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/delay.h>

#include "vpu-config.h"
#include "vpu-exynos.h"
#include "vpu-exynos8895.h"
#include "vpu-debug.h"

#define CLK_INDEX(name) VPU_##name
#define REGISTER_CLK(name) [CLK_INDEX(name)] = {#name, NULL}

extern int vpu_clk_set_rate(struct device *dev, u32 index, ulong frequency);
extern ulong vpu_clk_get_rate(struct device *dev, u32 index);
extern int  vpu_clk_enable(struct device *dev, u32 index);
extern int vpu_clk_disable(struct device *dev, u32 index);

enum vpu_clk_index {
	CLK_INDEX(vpu)
};

struct vpu_clk vpu_clk_array[] = {
	REGISTER_CLK(vpu)
};

const u32 vpu_clk_array_size = ARRAY_SIZE(vpu_clk_array);

int vpu_exynos_clk_cfg(struct vpu_exynos *exynos)
{
	return 0;
}

int vpu_exynos_clk_on(struct vpu_exynos *exynos)
{
	vpu_clk_enable(exynos->dev, CLK_INDEX(vpu));
	vpu_clk_get_rate(exynos->dev, CLK_INDEX(vpu));
	return 0;
}

int vpu_exynos_clk_off(struct vpu_exynos *exynos)
{
	vpu_clk_disable(exynos->dev, CLK_INDEX(vpu));
	return 0;
}

int vpu_exynos_clk_dump(struct vpu_exynos *exynos)
{
	int ret = 0;
	struct vpu_reg *reg;
	void *vpucmu_base;
	void *vpu_base;
	u32 val;

	vpucmu_base = ioremap_nocache(0x15A80000, 0x2000);
	if (!vpucmu_base) {
		vpu_err("ioremap is fail(vpucmu_base)\n");
		ret = -EINVAL;
		goto p_err;
	}

	vpu_base = ioremap_nocache(0x13200000, 0x2000);
	if (!vpu_base) {
		vpu_err("ioremap is fail(vpu_base)\n");
		ret = -EINVAL;
		goto p_err;
	}

	reg = &vpu_cmuclk_regs[MUX_CLKCMU_VPU_BUS];
	vpu_readl(vpucmu_base, reg, &val);
	vpu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

	reg = &vpu_cmuclk_regs[DIV_CLKCMU_VPU_BUS];
	vpu_readl(vpucmu_base, reg, &val);
	vpu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

	reg = &vpu_clk_regs[MUX_CLKCMU_VPU_BUS_USER];
	vpu_readl(vpu_base, reg, &val);
	vpu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

	reg = &vpu_clk_regs[VPU_CMU_CONTROLLER_OPTION];
	vpu_readl(vpu_base, reg, &val);
	vpu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

	reg = &vpu_clk_regs[DIV_CLK_VPU_BUSP];
	vpu_readl(vpu_base, reg, &val);
	vpu_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, val);

p_err:
	if (vpucmu_base)
		iounmap(vpucmu_base);

	if (vpu_base)
		iounmap(vpu_base);

	return 0;
}

int dummy_vpu_exynos_clk_cfg(struct vpu_exynos *exynos) { return 0; }
int dummy_vpu_exynos_clk_on(struct vpu_exynos *exynos) { return 0; }
int dummy_vpu_exynos_clk_off(struct vpu_exynos *exynos) { return 0; }
int dummy_vpu_exynos_clk_dump(struct vpu_exynos *exynos) { return 0; }

const struct vpu_clk_ops vpu_clk_ops = {
#ifdef CONFIG_EXYNOS_VPU_HARDWARE
	.clk_cfg	= vpu_exynos_clk_cfg,
	.clk_on		= vpu_exynos_clk_on,
	.clk_off	= vpu_exynos_clk_off,
	.clk_dump	= vpu_exynos_clk_dump
#else
	.clk_cfg	= dummy_vpu_exynos_clk_cfg,
	.clk_on		= dummy_vpu_exynos_clk_on,
	.clk_off	= dummy_vpu_exynos_clk_off,
	.clk_dump	= dummy_vpu_exynos_clk_dump
#endif
};

int vpu_exynos_ctl_reset(struct vpu_exynos *exynos, bool hold)
{
	if (hold) {
		vpu_writel(exynos->regbase, &vpu_regs[CPU_CTRL], 0x3);
		vpu_writel(exynos->regbase, &vpu_regs[GLOBAL_CTRL], 0x7337);
	} else {
		vpu_writel(exynos->regbase, &vpu_regs[GLOBAL_CTRL], 0x0);
		vpu_writel(exynos->regbase, &vpu_regs[CPU_CTRL], 0x0);
	}

	return 0;
}

int vpu_exynos_ctl_dump(struct vpu_exynos *exynos, u32 instance)
{
	u32 i, offset;

	vpu_info("DUMP PU : %s\n", vpu_reg_blk[instance].name);
	for (i = 0; i < vpu_reg_blk[instance].blocks; ++i) {
		offset = 0x100 * i;
		vpu_debug_memdump32((u32 *)(exynos->regbase + vpu_reg_blk[instance].offset + offset),
			(u32 *)(exynos->regbase + vpu_reg_blk[instance].offset + offset + 0xFF));
	}

	return 0;
}

void * vpu_exynos_ctl_remap(struct vpu_exynos *exynos, u32 instance)
{
	BUG_ON(!exynos);
	BUG_ON(instance >= ARRAY_SIZE(vpu_reg_blk));

	return exynos->regbase + vpu_reg_blk[instance].offset;
}

int vpu_exynos_ctl_unmap(struct vpu_exynos *exynos, u32 instance, void *base)
{
	BUG_ON(!exynos);
	BUG_ON(!base);
	BUG_ON(instance >= ARRAY_SIZE(vpu_reg_blk));

	return 0;
}

int vpu_exynos_ctl_trigger(struct vpu_exynos *exynos, u32 chain_id)
{
	vpu_writel(exynos->regbase, &vpu_regs[GCBCTRL_CHAIN_ID], chain_id);
	vpu_writel(exynos->regbase, &vpu_regs[GCBCTRL_COMMAND_CODE], 0x2);
	vpu_writel(exynos->regbase, &vpu_regs[GCBCTRL_COMMAND_EXECUTE], 0x1);

	return 0;
}

int vpu_exynos_ctl_start(struct vpu_exynos *exynos, u32 chain_id)
{
	u32 busy;

	vpu_writel(exynos->regbase, &vpu_regs[GCBCTRL_CHAIN_ID], chain_id);
	vpu_writel(exynos->regbase, &vpu_regs[GCBCTRL_COMMAND_CODE], 0x3);
	vpu_writel(exynos->regbase, &vpu_regs[GCBCTRL_COMMAND_EXECUTE], 0x1);

	vpu_readl(exynos->regbase, &vpu_regs[GCBCTRL_BUSY_STATE], &busy);
	while (busy) {
		vpu_info("waiting busy is clear(%X)\n", busy);
		vpu_readl(exynos->regbase, &vpu_regs[GCBCTRL_BUSY_STATE], &busy);
		msleep(100);
	}

	vpu_info("%d test done(%X)\n", chain_id, busy);

	return 0;
}

int dummy_vpu_exynos_ctl_reset(struct vpu_exynos *exynos, bool hold) { return 0; }
int dummy_vpu_exynos_ctl_dump(struct vpu_exynos *exynos, u32 instance) { return 0; }
void * dummy_vpu_exynos_ctl_remap(struct vpu_exynos *exynos, u32 instance) { return NULL; }
int dummy_vpu_exynos_ctl_unmap(struct vpu_exynos *exynos, u32 instance, void *base) { return 0; }
int dummy_vpu_exynos_ctl_trigger(struct vpu_exynos *exynos, u32 chain_id) { return 0; }
int dummy_vpu_exynos_ctl_start(struct vpu_exynos *exynos, u32 chain_id) { return 0; }

const struct vpu_ctl_ops vpu_ctl_ops = {
#ifdef CONFIG_EXYNOS_VPU_HARDWARE
	.ctl_reset	= vpu_exynos_ctl_reset,
	.ctl_dump	= vpu_exynos_ctl_dump,
	.ctl_remap	= vpu_exynos_ctl_remap,
	.ctl_unmap	= vpu_exynos_ctl_unmap,
	.ctl_trigger	= vpu_exynos_ctl_trigger,
	.ctl_start	= vpu_exynos_ctl_start,
#else
	.ctl_reset	= dummy_vpu_exynos_ctl_reset,
	.ctl_dump	= dummy_vpu_exynos_ctl_dump,
	.ctl_remap	= dummy_vpu_exynos_ctl_remap,
	.ctl_unmap	= dummy_vpu_exynos_ctl_unmap,
	.ctl_trigger	= dummy_vpu_exynos_ctl_trigger,
	.ctl_start	= dummy_vpu_exynos_ctl_start,
#endif
};
