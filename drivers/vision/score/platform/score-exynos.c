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
#include <linux/pinctrl/consumer.h>

#include "score-config.h"
#include "score-exynos.h"

extern const struct score_clk_ops score_clk_ops;
extern const struct score_ctl_ops score_ctl_ops;

int score_clk_set_rate(struct score_exynos *exynos, ulong frequency)
{
	int ret = 0;

	if (!IS_ERR_OR_NULL(exynos->clk)) {
		ret = clk_set_rate(exynos->clk, frequency);
		if (ret) {
			score_err("clk_set_rate is fail(%d)\n", ret);
		}
	}
	return ret;
}

ulong score_clk_get_rate(struct score_exynos *exynos)
{
	ulong frequency = 0;

	if (!IS_ERR_OR_NULL(exynos->clk)) {
		frequency = clk_get_rate(exynos->clk);
	}

	return frequency;
}

int score_clk_enable(struct score_exynos *exynos)
{
	int ret = 0;

	if (!IS_ERR_OR_NULL(exynos->clk)) {
		ret = clk_prepare_enable(exynos->clk);
		if (ret) {
			score_err("clk_prepare_enable is fail(%d)\n", ret);
		}
	}
	return ret;
}

int score_clk_disable(struct score_exynos *exynos)
{
	int ret = 0;

	if (!IS_ERR_OR_NULL(exynos->clk))
		clk_disable_unprepare(exynos->clk);
	return ret;
}

static int score_exynos_clk_init(struct score_exynos *exynos)
{
	int ret = 0;

	exynos->clk = devm_clk_get(exynos->dev, "dsp");
	if (IS_ERR_OR_NULL(exynos->clk)) {
		probe_err("Failed(%ld) to get 'dsp' clock\n", PTR_ERR(exynos->clk));
		ret = PTR_ERR(exynos->clk);
	}

	return ret;
}

int score_exynos_probe(struct score_exynos *exynos, struct device *dev, void *regs)
{
	exynos->dev = dev;
	exynos->regs = regs;
	exynos->clk_ops = &score_clk_ops;
	exynos->ctl_ops = &score_ctl_ops;
	exynos->pinctrl = devm_pinctrl_get(dev);
	exynos->clk = NULL;

	/*
	 * if (IS_ERR_OR_NULL(exynos->pinctrl)) {
	 *	probe_err("devm_pinctrl_get is fail");
	 *	ret = PTR_ERR(exynos->pinctrl);
	 *	goto p_err;
	 * }
	 */

	return score_exynos_clk_init(exynos);
}

void score_exynos_release(struct score_exynos *exynos)
{
	devm_pinctrl_put(exynos->pinctrl);
}

void score_readl(void __iomem *base_addr, struct score_reg *reg, u32 *val)
{
	*val = readl(base_addr + reg->offset);

#ifdef DBG_HW_SFR
	score_info("[REG][%s][0x%04X], val(R):[0x%08X]\n", reg->name, reg->offset, *val);
#endif
}

void score_writel(void __iomem *base_addr, struct score_reg *reg, u32 val)
{
#ifdef DBG_HW_SFR
	score_info("[REG][%s][0x%04X], val(W):[0x%08X]\n", reg->name, reg->offset, val);
#endif

	writel(val, base_addr + reg->offset);
}

void score_readf(void __iomem *base_addr, struct score_reg *reg, struct score_field *field, u32 *val)
{
	*val = (readl(base_addr + reg->offset) >> (field->bit_start)) & ((1 << (field->bit_width)) - 1);

#ifdef DBG_HW_SFR
	score_info("[REG][%s][%s][0x%04X], val(R):[0x%08X]\n", reg->name, field->name, reg->offset, *val);
#endif
}

void score_writef(void __iomem *base_addr, struct score_reg *reg, struct score_field *field, u32 val)
{
	u32 mask, temp;

	mask = ((1 << field->bit_width) - 1);
	temp = readl(base_addr + reg->offset) & ~(mask << field->bit_start);
	temp |= (val & mask) << (field->bit_start);

#ifdef DBG_HW_SFR
	score_info("[REG][%s][%s][0x%04X], val(W):[0x%08X]\n", reg->name, field->name, reg->offset, val);
#endif

	writel(temp, base_addr + reg->offset);
}
