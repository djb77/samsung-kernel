/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_EXYNOS_H_
#define VPU_EXYNOS_H_

#include <linux/clk.h>
#include <linux/device.h>

struct vpu_exynos;

struct vpu_regblock {
	unsigned int		offset;
	unsigned int		blocks;
	char			*name;
};

enum  regdata_type {
	/* read write */
	RW			= 0,
	/* read only */
	RO			= 1,
	/* write only */
	WO			= 2,
	/* write input */
	WI			= 2,
	/* clear after read */
	RAC			= 3,
	/* write 1 -> clear */
	W1C			= 4,
	/* write read input */
	WRI			= 5,
	/* write input */
	RWI			= 5,
	/* only scaler */
	R_W			= 6,
	/* read & write for clear */
	RWC			= 7,
	/* read & write as dual setting */
	RWS
};

struct vpu_reg {
	unsigned int		offset;
	char			*name;
};

struct vpu_field {
	char			*name;
	unsigned int		bit_start;
	unsigned int		bit_width;
	enum regdata_type	type;
};

struct vpu_clk {
	const char	*name;
	struct clk	*clk;
};

struct vpu_clk_ops {
	int (*clk_cfg)(struct vpu_exynos *exynos);
	int (*clk_on)(struct vpu_exynos *exynos);
	int (*clk_off)(struct vpu_exynos *exynos);
	int (*clk_dump)(struct vpu_exynos *exynos);
};

struct vpu_ctl_ops {
	int (*ctl_reset)(struct vpu_exynos *exynos, bool hold);
	int (*ctl_dump)(struct vpu_exynos *exynos, u32 instance);
	void * (*ctl_remap)(struct vpu_exynos *exynos, u32 instance);
	int (*ctl_unmap)(struct vpu_exynos *exynos, u32 instance, void *base);
	int (*ctl_trigger)(struct vpu_exynos *exynos, u32 chain_id);
	int (*ctl_start)(struct vpu_exynos *exynos, u32 chain_id);
};

struct vpu_exynos {
	struct device			*dev;
	void				*regbase;
	void				*ram0base;
	void				*ram1base;
	struct pinctrl			*pinctrl;
	const struct vpu_clk_ops	*clk_ops;
	const struct vpu_ctl_ops	*ctl_ops;
};

int vpu_exynos_probe(struct vpu_exynos *exynos, struct device *dev,
	void *regs, void *ram0, void *ram1);

void vpu_readl(void __iomem *base_addr, struct vpu_reg *reg, u32 *val);
void vpu_writel(void __iomem *base_addr, struct vpu_reg *reg, u32 val);

#define CLK_OP(exynos, op) (exynos->clk_ops ? exynos->clk_ops->op(exynos) : 0)
#define CTL_OP(exynos, op, ...) (exynos->ctl_ops ? exynos->ctl_ops->op(exynos, ##__VA_ARGS__) : 0)

#endif
