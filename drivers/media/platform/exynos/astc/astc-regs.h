/* drivers/media/platform/exynos/astc/astc-regs.h
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
	/* The encoding flow on TRM reports this as the reset procedure */
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

	/*
	 * first zero-out the bits we want to overwrite,
	 * then write new bits into it
	 */
	u32 newReg = (reg & ~(ASTC_AXI_MODE_REG_WR_BURST_LENGTH_MASK))
			| ((len << ASTC_AXI_MODE_REG_WR_BURST_LENGTH_INDEX)
			   & ASTC_AXI_MODE_REG_WR_BURST_LENGTH_MASK);

	writel(newReg, base + ASTC_AXI_MODE_REG);
}

#ifdef DEBUG
/* Prints the encoding configuration register in readable format */
static inline void dump_config_register(struct astc_dev *device)
{
	u32 enc_config = readl(device->regs + ASTC_CTRL_REG);
	u32 color_fmt =   (enc_config >> ASTC_CTRL_REG_COLOR_FORMAT_INDEX) & (KBit0 | KBit1);
	bool dual_plane = (enc_config >> ASTC_CTRL_REG_DUAL_PLANE_INDEX)   &  KBit0;
	u32 refine_iter = (enc_config >> ASTC_CTRL_REG_REFINE_ITER_INDEX)  & (KBit0 | KBit1);
	u32 partitions =  (enc_config >> ASTC_CTRL_REG_PARTITION_EN_INDEX) &  KBit0;
	u32 blk_mode =    (enc_config >> ASTC_CTRL_REG_NUM_BLK_MODE_INDEX) &  KBit0;
	u32 blk_size =    (enc_config >> ASTC_CTRL_REG_ENC_BLK_SIZE_INDEX) & (KBit0 | KBit1);

	dev_dbg(device->dev, "ENCODING CONFIGURATION\n");
	dev_dbg(device->dev, "Color format reg val:      %u\n", color_fmt);
	dev_dbg(device->dev, "Dual plane reg val:        %d\n", dual_plane);
	dev_dbg(device->dev, "Refine iterations reg val: %u\n", refine_iter);
	dev_dbg(device->dev, "Partitions reg val:        %u\n", partitions);
	dev_dbg(device->dev, "Block mode:                %u\n", blk_mode);
	dev_dbg(device->dev, "Block size:                %u\n", blk_size);
}

/* Dump the content of all registers, for debugging purposes */
static inline void dump_regs(struct astc_dev *device)
{
	void __iomem *base = device->regs;
	u32 axi_mode = readl(base + ASTC_AXI_MODE_REG);
	dma_addr_t src_addr = readl(base + ASTC_SRC_BASE_ADDR_REG);
	dma_addr_t dst_addr = readl(base + ASTC_DST_BASE_ADDR_REG);

	dev_dbg(device->dev, "Soft reset:       %u\n"
		, readl(base + ASTC_SOFT_RESET_REG) & ASTC_SOFT_RESET_SOFT_RST);
	dump_config_register(device);
	dev_dbg(device->dev, "IRQ enabled:      %u\n"
		, readl(base + ASTC_INT_EN_REG) & ASTC_INT_EN_REG_IRQ_EN);
	dev_dbg(device->dev, "IRQ status:       %u\n"
		, readl(base + ASTC_INT_STAT_REG) & ASTC_INT_STAT_REG_INT_STATUS);

	dev_dbg(device->dev, "AXI write error:  %u\n"
		, readl(base + ASTC_ENC_STAT_REG) & ASTC_ENC_STAT_REG_WR_BUS_ERR);
	dev_dbg(device->dev, "AXI read error:   %u\n"
		, readl(base + ASTC_ENC_STAT_REG) & ASTC_ENC_STAT_REG_RD_BUS_ERR);

	//mask 14bits by &-ing with ASTC_NUM_BLK_REG_FIELD_MASK
	dev_dbg(device->dev, "Image width:      %u\n",
		(readl(base + ASTC_IMG_SIZE_REG) >> ASTC_IMG_SIZE_REG_IMG_SIZE_X_INDEX) & ASTC_NUM_BLK_REG_FIELD_MASK);
	dev_dbg(device->dev, "Image height:     %u\n",
		(readl(base + ASTC_IMG_SIZE_REG) >> ASTC_IMG_SIZE_REG_IMG_SIZE_Y_INDEX) & ASTC_NUM_BLK_REG_FIELD_MASK);

	dev_dbg(device->dev, "Blocks on X:      %u\n",
		(readl(base + ASTC_NUM_BLK_REG) >> ASTC_NUM_BLK_REG_NUM_BLK_X_INDEX) & 0xfff);
	dev_dbg(device->dev, "Blocks on Y:      %u\n",
		(readl(base + ASTC_NUM_BLK_REG) >> ASTC_NUM_BLK_REG_NUM_BLK_Y_INDEX) & 0xfff);

	dev_dbg(device->dev, "DMA Src address:  %pad\n", &src_addr);
	dev_dbg(device->dev, "DMA Dst address:  %pad\n", &dst_addr);
	dev_dbg(device->dev, "Magic number:     %x\n"
		, astc_get_magic_number(base));
	dev_dbg(device->dev, "Encoder started:  %u\n"
		, readl(base + ASTC_ENC_START_REG) & KBit0);
	dev_dbg(device->dev, "Encoder running:  %u\n"
		, astc_hw_is_enc_running(base));

	/*
	 * We do things a bit differently here, we first select the bits,
	 * then shift them to print them out. It's easier because
	 * they're small fields
	 */
	dev_dbg(device->dev, "AXI WR burst len: %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_WR_BURST_LENGTH_MASK) >> ASTC_AXI_MODE_REG_WR_BURST_LENGTH_INDEX);
	dev_dbg(device->dev, "AXI AWUSER:       %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_AWUSER) >> ASTC_AXI_MODE_REG_AWUSER_INDEX);
	dev_dbg(device->dev, "AXI ARUSER:       %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_ARUSER) >> ASTC_AXI_MODE_REG_ARUSER_INDEX);
	dev_dbg(device->dev, "AXI Write cache:  %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_AWCACHE) >> ASTC_AXI_MODE_REG_AWCACHE_INDEX);
	dev_dbg(device->dev, "AXI Read cache:   %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_ARCACHE) >> ASTC_AXI_MODE_REG_ARCACHE_INDEX);
	dev_dbg(device->dev
		, "HW version:       %x\n", astc_hw_get_version(base));
}
#else /* DEBUG */
static inline void dump_config_register(struct astc_dev *device)
{
}
static inline void dump_regs(struct astc_dev *device)
{
}
#endif /* DEBUG */

#endif /* ASTC_REGS_H_ */

