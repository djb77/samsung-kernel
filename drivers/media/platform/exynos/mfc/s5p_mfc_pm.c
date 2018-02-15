/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_pm.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/smc.h>

#include "s5p_mfc_pm.h"

#include "s5p_mfc_reg.h"

void s5p_mfc_pm_init(struct s5p_mfc_dev *dev)
{
	spin_lock_init(&dev->pm.clklock);
	atomic_set(&dev->pm.pwr_ref, 0);
	atomic_set(&dev->clk_ref, 0);

	dev->pm.device = dev->device;
	dev->pm.clock_on_steps = 0;
	dev->pm.clock_off_steps = 0;
	pm_runtime_enable(dev->pm.device);
}

void s5p_mfc_pm_final(struct s5p_mfc_dev *dev)
{
	pm_runtime_disable(dev->pm.device);
}

int s5p_mfc_pm_clock_on(struct s5p_mfc_dev *dev)
{
	int ret = 0;
	int state;
	unsigned long flags;

	dev->pm.clock_on_steps = 1;
	state = atomic_read(&dev->clk_ref);

	ret = clk_enable(dev->pm.clock);
	if (ret < 0) {
		mfc_err_dev("clk_enable failed (%d)\n", ret);
		return ret;
	}
	dev->pm.clock_on_steps |= 0x1 << 1;

	if (dev->pm.base_type != MFCBUF_INVALID)
		s5p_mfc_set_risc_base_addr(dev, dev->pm.base_type);

	dev->pm.clock_on_steps |= 0x1 << 2;
	if (dev->curr_ctx_is_drm) {
		spin_lock_irqsave(&dev->pm.clklock, flags);
		mfc_debug(3, "Begin: enable protection\n");
		ret = exynos_smc(SMC_PROTECTION_SET, 0,
					dev->id, SMC_PROTECTION_ENABLE);
		dev->pm.clock_on_steps |= 0x1 << 3;
		if (ret != DRMDRV_OK) {
			mfc_err_dev("Protection Enable failed! ret(%u)\n", ret);
			spin_unlock_irqrestore(&dev->pm.clklock, flags);
			clk_disable(dev->pm.clock);
			return -EACCES;
		}
		mfc_debug(3, "End: enable protection\n");
		spin_unlock_irqrestore(&dev->pm.clklock, flags);
	}

	dev->pm.clock_on_steps |= 0x1 << 4;
	atomic_inc_return(&dev->clk_ref);

	dev->pm.clock_on_steps |= 0x1 << 6;
	state = atomic_read(&dev->clk_ref);
	mfc_debug(2, "+ %d\n", state);
	MFC_TRACE_DEV("** clock_on : ref state(%d)\n", state);
	MFC_TRACE_LOG_DEV("c+%d", state);

	return 0;
}

/* Use only in functions that first instance is guaranteed, like mfc_init_hw() */
int s5p_mfc_pm_clock_on_with_base(struct s5p_mfc_dev *dev,
				enum mfc_buf_usage_type buf_type)
{
	int ret;
	dev->pm.base_type = buf_type;
	ret = s5p_mfc_pm_clock_on(dev);
	dev->pm.base_type = MFCBUF_INVALID;

	return ret;
}

void s5p_mfc_pm_clock_off(struct s5p_mfc_dev *dev)
{
	int state;
	unsigned long flags;
	int ret = 0;

	dev->pm.clock_off_steps = 1;
	state = atomic_read(&dev->clk_ref);

	dev->pm.clock_off_steps |= 0x1 << 1;
	atomic_dec_return(&dev->clk_ref);

	dev->pm.clock_off_steps |= 0x1 << 2;
	state = atomic_read(&dev->clk_ref);
	if (state < 0) {
		mfc_err_dev("Clock state is wrong(%d)\n", state);
		atomic_set(&dev->clk_ref, 0);
		dev->pm.clock_off_steps |= 0x1 << 3;
	} else {
		if (dev->curr_ctx_is_drm) {
			mfc_debug(3, "Begin: disable protection\n");
			spin_lock_irqsave(&dev->pm.clklock, flags);
			dev->pm.clock_off_steps |= 0x1 << 4;
			ret = exynos_smc(SMC_PROTECTION_SET, 0,
					dev->id, SMC_PROTECTION_DISABLE);
			if (ret != DRMDRV_OK) {
				mfc_err_dev("Protection Disable failed! ret(%u)\n", ret);
				spin_unlock_irqrestore(&dev->pm.clklock, flags);
				clk_disable(dev->pm.clock);
				return;
			}
			mfc_debug(3, "End: disable protection\n");
			dev->pm.clock_off_steps |= 0x1 << 5;
			spin_unlock_irqrestore(&dev->pm.clklock, flags);
		}
		dev->pm.clock_off_steps |= 0x1 << 6;
		clk_disable(dev->pm.clock);
	}
	dev->pm.clock_off_steps |= 0x1 << 7;
	state = atomic_read(&dev->clk_ref);
	mfc_debug(2, "- %d\n", state);
	MFC_TRACE_DEV("** clock_off: ref state(%d)\n", state);
	MFC_TRACE_LOG_DEV("c-%d", state);
}

int s5p_mfc_pm_power_on(struct s5p_mfc_dev *dev)
{
	int ret;

	MFC_TRACE_DEV("++ Power on\n");
	ret = pm_runtime_get_sync(dev->pm.device);
	if (ret < 0) {
		mfc_err_dev("Failed to get power: ret(%d)\n", ret);
		goto err_power_on;
	}

	dev->pm.clock = clk_get(dev->device, "aclk_mfc");
	if (IS_ERR(dev->pm.clock)) {
		mfc_err_dev("failed to get parent clock: ret(%d)\n", ret);
		ret = -ENOENT;
		goto err_clk_get;
	}

	ret = clk_prepare(dev->pm.clock);
	if (ret) {
		mfc_err_dev("clk_prepare() failed: ret(%d)\n", ret);
		goto err_clk_prepare;
	}

	atomic_inc(&dev->pm.pwr_ref);

	MFC_TRACE_DEV("-- Power on: ret(%d)\n", ret);
	MFC_TRACE_LOG_DEV("p+%d", s5p_mfc_pm_get_pwr_ref_cnt(dev));

	return 0;

err_clk_prepare:
	clk_put(dev->pm.clock);

err_clk_get:
	pm_runtime_put_sync(dev->pm.device);

err_power_on:
	return ret;
}

int s5p_mfc_pm_power_off(struct s5p_mfc_dev *dev)
{
	int ret;

	MFC_TRACE_DEV("++ Power off\n");

	clk_unprepare(dev->pm.clock);
	clk_put(dev->pm.clock);

	ret = pm_runtime_put_sync(dev->pm.device);
	if (ret < 0) {
		mfc_err_dev("Failed to put power: ret(%d)\n", ret);
		return ret;
	}

	atomic_dec(&dev->pm.pwr_ref);

	MFC_TRACE_DEV("-- Power off: ret(%d)\n", ret);
	MFC_TRACE_LOG_DEV("p-%d", s5p_mfc_pm_get_pwr_ref_cnt(dev));

	return ret;
}
