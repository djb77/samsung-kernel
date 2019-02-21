/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SCORE_EXYNOS_H_
#define SCORE_EXYNOS_H_

#include <linux/clk.h>
#include <linux/device.h>

struct score_exynos;

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

struct score_reg {
	unsigned int		offset;
	char			*name;
};

struct score_field {
	char			*name;
	unsigned int		bit_start;
	unsigned int		bit_width;
	enum regdata_type	type;
};

struct score_clk {
	const char	*name;
	struct clk	*clk;
};

struct score_clk_ops {
	int (*clk_cfg)(struct score_exynos *exynos);
	int (*clk_on)(struct score_exynos *exynos);
	int (*clk_off)(struct score_exynos *exynos);
	int (*clk_set)(struct score_exynos *exynos, ulong frequency);
	ulong (*clk_get)(struct score_exynos *exynos);
};

struct score_ctl_ops {
	int (*ctl_reset)(struct score_exynos *exynos);
};

struct score_exynos {
	struct device			*dev;
	void				*regs;
	struct pinctrl			*pinctrl;

	struct clk			*clk;
	const struct score_clk_ops	*clk_ops;
	const struct score_ctl_ops	*ctl_ops;
};

int score_exynos_probe(struct score_exynos *exynos, struct device *dev, void *regs);
void score_exynos_release(struct score_exynos *exynos);

void score_readl(void __iomem *base_addr, struct score_reg *reg, u32 *val);
void score_writel(void __iomem *base_addr, struct score_reg *reg, u32 val);

#define CLK_OP(exynos, op) ((exynos)->clk_ops ? (exynos)->clk_ops->op(exynos) : 0)
#define CTL_OP(exynos, op) ((exynos)->ctl_ops ? (exynos)->ctl_ops->op(exynos) : 0)

#endif
