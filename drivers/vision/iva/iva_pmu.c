/* iva_pmu.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>

#include "regs/iva_base_addr.h"
#include "regs/iva_pmu_reg.h"
#include "iva_pmu.h"

//#define PMU_CLK_ENABLE_WITH_BIT_SET		/* clock polarity */


/* return masked old values */
static uint32_t __iva_pmu_write_mask(void __iomem *pmu_reg, uint32_t mask, bool set)
{
	uint32_t old_val, val;

	old_val= readl(pmu_reg);
	if (set)
		val = old_val | mask;
	else
		val = old_val & ~mask;
	writel(val, pmu_reg);

	return old_val & mask;
}

uint32_t iva_pmu_ctrl_q_channel(struct iva_dev_data *iva,
			enum iva_pmu_q_ch_ctrl q_ctrl, bool set)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_CTRL_ADDR, q_ctrl, set);
}

uint32_t iva_pmu_reset_hwa(struct iva_dev_data *iva,
			enum iva_pmu_ip_id ip, bool release)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_SOFT_RESET_ADDR,
			ip, release);
}

uint32_t iva_pmu_enable_clk(struct iva_dev_data *iva,
		enum iva_pmu_ip_id ip, bool enable)
{
#ifndef PMU_CLK_ENABLE_WITH_BIT_SET
	enable = !enable;
#endif
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_CLOCK_CONTROL_ADDR,
			ip, enable);
}

uint32_t iva_pmu_ctrl_mcu(struct iva_dev_data *iva,
		enum iva_pmu_mcu_ctrl mcu_ctrl, bool set)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_VMCU_CTRL_ADDR,
			mcu_ctrl, set);
}

void iva_pmu_prepare_mcu_sram(struct iva_dev_data *iva)
{
	/*
	 * Mark PMU_VMCU as active to prevent any QChannel deadlocks
	 * that can happen when resetting the MCU
	 */
	iva_pmu_ctrl_q_channel(iva, pmu_q_ch_vmcu_active, true);

	/* Put MCU into reset and hold boot*/
	iva_pmu_ctrl_mcu(iva, pmu_mcu_reset_n | pmu_en_sleep, false);
	iva_pmu_ctrl_mcu(iva, pmu_boothold, true);

	/* Release MCU reset */
	iva_pmu_ctrl_mcu(iva, pmu_mcu_reset_n, true);
}

void iva_pmu_reset_mcu(struct iva_dev_data *iva)
{
	iva_pmu_ctrl_mcu(iva, pmu_boothold, false);

	/*
	 * Remove VMCU as active
	 * so that CMU can put us in low-power if and when appropriate
	 */
	iva_pmu_ctrl_q_channel(iva, pmu_q_ch_vmcu_active, false);
}

uint32_t iva_pmu_enable_auto_clock_gating(struct iva_dev_data *iva,
		enum iva_pmu_ip_id ip, bool enable)
{
	return __iva_pmu_write_mask(iva->pmu_va + IVA_PMU_HWACG_EN_ADDR,
			ip, enable);
}

int iva_pmu_init(struct iva_dev_data *iva, bool en_hwa)
{
	if (en_hwa) {		/* unreset and enable clocks for all blocks */
		iva_pmu_enable_all_clks(iva);
		iva_pmu_reset_all(iva);
		iva_pmu_release_all(iva);
	} else {		/* explicitly disable all ips */
		iva_pmu_reset_all(iva);
		iva_pmu_disable_all_clks(iva);
	}

	return 0;
}

void iva_pmu_deinit(struct iva_dev_data *iva)
{
	iva_pmu_reset_all(iva);
	iva_pmu_disable_all_clks(iva);
}

int iva_pmu_probe(struct iva_dev_data *iva)
{
	struct device *dev = iva->dev;

	if (iva->pmu_va) {
		dev_warn(dev, "%s() already mapped into pmu_va(%p)\n", __func__,
				iva->pmu_va);
		return 0;
	}

	if (!iva->iva_va) {
		dev_err(dev, "%s() null iva_va\n", __func__);
		return -EINVAL;
	}

	iva->pmu_va = iva->iva_va + IVA_PMU_BASE_ADDR;

	dev_dbg(iva->dev, "%s() succeed to get pmu va\n", __func__);
	return 0;
}

void iva_pmu_remove(struct iva_dev_data *iva)
{
	if (iva->pmu_va) {
		iva->pmu_va = NULL;
	}

	dev_dbg(iva->dev, "%s() succeed to release pmu va\n", __func__);
}
