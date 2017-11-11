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

#include <linux/io.h>
#include "score-config.h"
#include "score-exynos.h"
#include "score-exynos8895.h"

#define CLK_INDEX(name) name
#define REGISTER_CLK(name) {name, NULL}

extern int score_clk_set_rate(struct score_exynos *exynos, ulong frequency);
extern ulong score_clk_get_rate(struct score_exynos *exynos);
extern int score_clk_enable(struct score_exynos *exynos);
extern int score_clk_disable(struct score_exynos *exynos);

int score_exynos_clk_cfg(struct score_exynos *exynos)
{
	return 0;
}

int score_exynos_clk_on(struct score_exynos *exynos)
{
	return score_clk_enable(exynos);
}

int score_exynos_clk_off(struct score_exynos *exynos)
{
	return score_clk_disable(exynos);
}

int score_exynos_clk_set(struct score_exynos *exynos, ulong frequency)
{
	/* return score_clk_set_rate(exynos, frequency); */
	return 0;
}

ulong score_exynos_clk_get(struct score_exynos *exynos)
{
	return score_clk_get_rate(exynos);
}

const struct score_clk_ops score_clk_ops = {
	.clk_cfg	= score_exynos_clk_cfg,
	.clk_on		= score_exynos_clk_on,
	.clk_off	= score_exynos_clk_off,
	.clk_set	= score_exynos_clk_set,
	.clk_get	= score_exynos_clk_get
};

int score_exynos_ctl_reset(struct score_exynos *exynos)
{
#ifdef CONFIG_EXYNOS_SCORE_HARDWARE
	writel(0, exynos->regs + 0x18);
#endif
	return 0;
}

const struct score_ctl_ops score_ctl_ops = {
	.ctl_reset	= score_exynos_ctl_reset,
};
