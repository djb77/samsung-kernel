/* linux/drivers/media/platform/exynos/astc-regs.h
 *
 * Register definition file for Samsung ASTC encoder driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrea Bernabei <a.bernabei@samsung.com>
 * Author: Ramesh Munikrishnappa <ramesh.munikrishn@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ASTC_REGS_H_
#define ASTC_REGS_H_
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/module.h>
#include "astc-core.h"
#include "astc-regs-v1.h"

static inline void astc_hw_soft_reset(void __iomem *base)
{
	//The encoding flow on TRM reports this as the reset procedure
	writel(ASTC_SOFT_RESET_RESET, base + ASTC_SOFT_RESET_REG);
	writel(ASTC_SOFT_RESET_RELEASE, base + ASTC_SOFT_RESET_REG);
}

static inline void astc_hw_set_enc_dec_mode(void __iomem *base
					    , enum astc_mode mode)
{
	/* do nothing, currently HW only supports encode */
}

static inline void astc_hw_set_enc_mode(void __iomem *base, u32 mode)
{
	writel(mode, base + ASTC_CTRL_REG);
}

static inline void astc_hw_enable_interrupt(void __iomem *base)
{
	writel(ASTC_INT_EN_REG_IRQ_EN_ENABLED, base + ASTC_INT_EN_REG);
}

static inline void astc_hw_disable_interrupt(void __iomem *base)
{
	writel(ASTC_INT_EN_REG_IRQ_EN_DISABLED, base + ASTC_INT_EN_REG);
}

static inline void astc_hw_clear_interrupt(void __iomem *base)
{
	writel(0, base + ASTC_INT_STAT_REG);
}

static inline unsigned int astc_hw_get_int_status(void __iomem *base)
{
	unsigned int int_status;

	int_status = readl(base + ASTC_INT_STAT_REG)
			& ASTC_INT_STAT_REG_INT_STATUS;

	return int_status;
}

static inline void astc_hw_set_src_addr(void __iomem *base, dma_addr_t addr)
{
	writel(addr, base + ASTC_SRC_BASE_ADDR_REG);
}

static inline void astc_hw_set_image_size_num_blocks(void __iomem *base
						     , u32 width, u32 height
						     , u16 blocks_on_x
						     , u16 blocks_on_y)
{
	unsigned int image_size =
			((width << ASTC_IMG_SIZE_REG_IMG_SIZE_X_INDEX)
			 | (height << ASTC_IMG_SIZE_REG_IMG_SIZE_Y_INDEX));

	u32 reg = (((u32)blocks_on_x) << ASTC_NUM_BLK_REG_NUM_BLK_X_INDEX)
			| (((u32)blocks_on_y) << ASTC_NUM_BLK_REG_NUM_BLK_Y_INDEX);

	writel(image_size, base + ASTC_IMG_SIZE_REG);
	writel(reg, base + ASTC_NUM_BLK_REG);
}

static inline void astc_hw_set_dst_addr(void __iomem *base, dma_addr_t addr)
{
	writel(addr, base + ASTC_DST_BASE_ADDR_REG);
}

static inline unsigned int astc_hw_get_version(void __iomem *base)
{
	unsigned int version = 0;

	version = readl(base + ASTC_VERSION_INFO_REG);
	return version;
}

static inline bool astc_hw_is_enc_running(void __iomem *base)
{
	bool isIdle = false;

	isIdle = readl(base + ASTC_ENC_DONE_REG) & ASTC_ENC_DONE_REG_DONE;
	return !isIdle;
}


static inline void astc_hw_start_enc(void __iomem *base)
{
	writel(ASTC_ENC_START_REG_START, base + ASTC_ENC_START_REG);
}

static inline int get_hw_enc_status(void __iomem *base)
{
	unsigned int status = 0;

	status = readl(base + ASTC_ENC_STAT_REG) & (KBit0 | KBit1);
	return (status != 0 ? -1:0);
}

static inline unsigned int astc_get_magic_number(void __iomem *base)
{
	return readl(base + ASTC_MAGIC_NUM_REG);
}

static inline void astc_hw_set_axi_burst_len(void __iomem *base, u16 len)
{
	u32 reg = readl(base + ASTC_AXI_MODE_REG);

	//first zero-out the bits we want to overwrite,
	//then write new bits into it
	u32 newReg = (reg & ~(ASTC_AXI_MODE_REG_WR_BURST_LENGTH_MASK))
			| ((len << ASTC_AXI_MODE_REG_WR_BURST_LENGTH_INDEX)
			   & ASTC_AXI_MODE_REG_WR_BURST_LENGTH_MASK);

	writel(newReg, base + ASTC_AXI_MODE_REG);
}

#endif /* ASTC_REGS_H_ */

