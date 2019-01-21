/* linux/drivers/media/platform/exynos/fimg2d_v5/g2d1shot.h
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * Samsung Graphics 2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __EXYNOS_G2D1SHOT_H_
#define __EXYNOS_G2D1SHOT_H_

#include <linux/videodev2.h>
#include <linux/videodev2_exynos_media.h>
#include <linux/pm_qos.h>
#include <media/m2m1shot2.h>

#include "g2d1shot_regs.h"

#define MODULE_NAME	"exynos-g2d"
#define NODE_NAME	"fimg2d"

#define	G2D_MAX_PLANES	3
#define	G2D_MAX_SOURCES 8
#define	G2D_MAX_CSC_FMT 5
#define	G2D_CSC_HW_SUPPORT 4

#define G2D_MAX_SIZE	8192

struct g2d_qos_entry {
	unsigned int freq_mif;
	unsigned int freq_cpu;
	unsigned int freq_kfc;
};

struct g2d1shot_platdata {
	int num_qos_steps;
	struct g2d_qos_entry *g2d_qos_table;
};

#define G2D_UORDER_ALIGN		16

#define G2D_COMP_ALIGN_SRC_ADDR	16
#define G2D_COMP_ALIGN_DST_ADDR	64
#define G2D_COMP_ALIGN_WIDTH		16
#define G2D_COMP_ALIGN_SRC_HEIGHT	4
#define G2D_COMP_ALIGN_DST_HEIGHT	16
#define G2D_COMP_BLOCK_SIZE 16
#define G2D_COMP_META_LEN 16

#define G2D_YUV_ALIGN_WIDTH		2
#define G2D_YUV420_ALIGN_HEIGHT	2

#define G2D_COMP_DEBUG_DATA_COUNT 16

/* flags for G2D supported format */
#define G2D_FMT_FLAG_SUPPORT_COMP	(1 << 0)
#define G2D_DATAFORMAT_RGB8888		(0 << 16)
#define G2D_DATAFORMAT_RGB565		(1 << 16)
#define G2D_DATAFORMAT_RGB4444		(2 << 16)
#define G2D_DATAFORMAT_RGB1555		(4 << 16)
#define G2D_DATAFORMAT_YCBCR420_2P	(8 << 16)
#define G2D_DATAFORMAT_YCBCR420_3P	(9 << 16)
#define G2D_DATAFORMAT_YCBCR422_1P	(10 << 16)
#define G2D_DATAFORMAT_YCBCR422_2P	(11 << 16)

/* flags for G2D supported CbCr order */
#define G2D_ORDER_CBYCRY		(0 << 24)
#define G2D_ORDER_CRYCBY		(1 << 24)
#define G2D_ORDER_YCBYCR		(2 << 24)
#define G2D_ORDER_YCRYCB		(3 << 24)
#define G2D_ORDER_CBCR			(0 << 24)
#define G2D_ORDER_CRCB			(1 << 24)

/* [31:0] swizzling order */
#define G2D_SWIZZLING_ABGR		(0x3012)
#define G2D_SWIZZLING_XBGR		(0x5012)
#define G2D_SWIZZLING_ARGB		(0x3210)
#define G2D_SWIZZLING_RGB		(0x5210)
#define G2D_SWIZZLING_BGR		(0x5012)

#define is_rgb565(x) ((x & 0xF0000) == G2D_DATAFORMAT_RGB565)
#define is_rgb(x) (((x & 0xF0000) == G2D_DATAFORMAT_RGB8888) ||	\
			((x & 0xF0000) == G2D_DATAFORMAT_RGB565) || \
			((x & 0xF0000) == G2D_DATAFORMAT_RGB4444) || \
			((x & 0xF0000) == G2D_DATAFORMAT_RGB1555))
#define is_yuv422(x)	(((x & 0xF0000) == G2D_DATAFORMAT_YCBCR422_1P) || \
			((x & 0xF0000) == G2D_DATAFORMAT_YCBCR422_2P))
#define is_yuv420(x)	((x & 0xF0000) == G2D_DATAFORMAT_YCBCR420_2P) || \
			((x & 0xF0000) == G2D_DATAFORMAT_YCBCR420_3P)
#define is_yuv420_3p(x)	((x & 0xF0000) == G2D_DATAFORMAT_YCBCR420_3P)
#define is_yuv(x)	(is_yuv422(x) || is_yuv420(x))

struct g2d1shot_fmt {
	char	*name;
	u32	pixelformat;
	u8	bpp[G2D_MAX_PLANES];
	u8	num_planes;
	u8	num_buffer;
	u8	flag;
	u32	value;
};

struct g2d_csc_fmt {
	u32 v4l2_colorspace;
	u8 colorspace;
	u8 range;
	u8 index;
};

struct g2d_dummy_buf {
	struct page *page;
	dma_addr_t dma_addr;
};

#define G2D_TIMEOUT_INTERVAL		300	/* 300 msec */
#define G2D_STATE_RUNNING		(1 << 0)
#define G2D_STATE_SUSPENDING		(1 << 1)
#define G2D_STATE_TIMEOUT		(1 << 2)

struct g2d1shot_dev {
	struct m2m1shot2_device *oneshot2_dev;
	struct device *dev;
	struct clk *clock;
	void __iomem *reg;

	u32 version;
	unsigned long state;
	spinlock_t state_lock;
	struct timer_list timer;

	wait_queue_head_t suspend_wait;
	struct m2m1shot2_context *suspend_ctx;
	bool suspend_ctx_success;
	unsigned long long t_start;

	struct g2d1shot_platdata *pdata;

	struct pm_qos_request g2d_cluster1_qos;
	struct pm_qos_request g2d_cluster0_qos;
	struct pm_qos_request g2d_mif_qos;
	struct pm_qos_request g2d_int_qos;
	unsigned int qos_table_idx;
	unsigned int *qos_stats;
	unsigned int num_rec;
	spinlock_t g2d_qos_lock;

	struct g2d_dummy_buf d_buf;
};

struct g2d1shot_ctx {
	struct g2d1shot_dev *g2d_dev;
	u32 src_fmt_value[G2D_MAX_SOURCES];
	u32 dst_fmt_value;

	u32 qos_lv;

	s8 src_csc_value[G2D_MAX_SOURCES];
	s8 csc_idx_map[G2D_MAX_CSC_FMT];
	u8 num_req_coeff;
};

/**
 * @ALPHA_PERPIXEL: perpixel alpha
 * @ALPHA_PERPIXEL_SUM_GLOBAL: perpixel + global
 * @ALPHA_PERPIXEL_MUL_GLOBAL: perpixel x global
 *
 * DO NOT CHANGE THIS ORDER
 */
enum alpha_opr {
	ALPHA_PERPIXEL = 0,	/* initial value */
	ALPHA_PERPIXEL_SUM_GLOBAL,
	ALPHA_PERPIXEL_MUL_GLOBAL,
};

#define MAX_G2D_BLIT_OP		(M2M1SHOT2_BLEND_LIGHTEN + 1)

/**
 * @COEFF_ONE: 1
 * @COEFF_ZERO: 0
 * @COEFF_SA: src alpha
 * @COEFF_SC: src color
 * @COEFF_DA: dst alpha
 * @COEFF_DC: dst color
 * @COEFF_GA: global(constant) alpha
 * @COEFF_GC: global(constant) color
 * @COEFF_DISJ_S:
 * @COEFF_DISJ_D:
 * @COEFF_CONJ_S:
 * @COEFF_CONJ_D:
 *
 * DO NOT CHANGE THIS ORDER
 */
enum g2d_coeff {
	COEFF_ONE = 0,
	COEFF_ZERO,
	COEFF_SA,
	COEFF_SC,
	COEFF_DA,
	COEFF_DC,
	COEFF_GA,
	COEFF_GC,
	COEFF_DISJ_S,
	COEFF_DISJ_D,
	COEFF_CONJ_S,
	COEFF_CONJ_D,
};

struct g2d_blend_coeff {
	bool s_coeff_inv;
	u8 s_coeff;
	bool d_coeff_inv;
	u8 d_coeff;
};

enum g2d_colorspace {
	G2D_COLORSPACE_601 = 0,
	G2D_COLORSPACE_709,
	G2D_COLORSPACE_2020,
	G2D_COLORSPACE_END,
};

enum g2d_cbcr_offset {
	G2D_CBCROFFSET_0_00 = 0,
	G2D_CBCROFFSET_0_25,
	G2D_CBCROFFSET_0_50,
	G2D_CBCROFFSET_0_75,
	G2D_CBCROFFSET_1_00,
};

static inline void g2d_hw_init(struct g2d1shot_dev *g2d_dev)
{
	/* sfr clear */
	__raw_writel(G2D_SFR_CLEAR | G2D_SOFT_RESET,
			g2d_dev->reg + G2D_SOFT_RESET_REG);
}

static inline u32 g2d_int_status(struct g2d1shot_dev *g2d_dev)
{
	return readl_relaxed(g2d_dev->reg + G2D_INTC_PEND_REG);
}

static inline void g2d_hw_stop(struct g2d1shot_dev *g2d_dev)
{
	/* clear irq */
	__raw_writel(G2D_BLIT_INT_FLAG, g2d_dev->reg + G2D_INTC_PEND_REG);
	/* disable irq */
	__raw_writel(0, g2d_dev->reg + G2D_INTEN_REG);
}

static inline u32 g2d_hw_read_version(struct g2d1shot_dev *g2d_dev)
{
	return __raw_readl(g2d_dev->reg + G2D_VERSION_INFO_REG);
}

static inline void g2d_hw_set_source_color(struct g2d1shot_dev *g2d_dev,
							int n, u32 color)
{
	/* set constant color */
	__raw_writel(color, g2d_dev->reg + G2D_LAYERn_COLOR_REG(n));
}

static inline void g2d_hw_set_source_type(struct g2d1shot_dev *g2d_dev,
							int n, u32 select)
{
	__raw_writel(select, g2d_dev->reg + G2D_LAYERn_SELECT_REG(n));
}

static inline void g2d_hw_set_dither(struct g2d1shot_dev *g2d_dev)
{
	u32 cfg;

	/* set dithering */
	cfg = __raw_readl(g2d_dev->reg + G2D_BITBLT_COMMAND_REG);
	cfg |= G2D_ENABLE_DITHER;

	__raw_writel(cfg, g2d_dev->reg + G2D_BITBLT_COMMAND_REG);
}

const struct g2d_csc_fmt *find_colorspace(u32 v4l2_colorspace);
void g2d_hw_start(struct g2d1shot_dev *g2d_dev);
void g2d_hw_set_source_blending(struct g2d1shot_dev *g2d_dev, int n,
						struct m2m1shot2_extra *ext);
void g2d_hw_set_source_premult(struct g2d1shot_dev *g2d_dev, int n, u32 flags);
void g2d_hw_set_source_format(struct g2d1shot_dev *g2d_dev, int n,
		struct m2m1shot2_context_format *ctx_fmt, bool compressed);
void g2d_hw_set_dest_addr(struct m2m1shot2_context *ctx,
		struct m2m1shot2_context_format *ctx_fmt,
		struct g2d1shot_dev *g2d_dev, bool compressed);
void g2d_hw_set_source_address(struct m2m1shot2_context *ctx,
		struct m2m1shot2_context_format *ctx_fmt,
		struct g2d1shot_dev *g2d_dev, int n, bool compressed);
void g2d_hw_set_source_repeat(struct g2d1shot_dev *g2d_dev, int n,
		struct m2m1shot2_extra *ext);
void g2d_hw_set_source_scale(struct g2d1shot_dev *g2d_dev, int n,
		struct m2m1shot2_extra *ext, u32 flags,
		struct m2m1shot2_context_format *ctx_fmt);
void g2d_hw_set_source_rotate(struct g2d1shot_dev *g2d_dev, int n,
		struct m2m1shot2_extra *ext);
void g2d_hw_set_source_valid(struct g2d1shot_dev *g2d_dev, int n);
void g2d_hw_set_dest_format(struct g2d1shot_dev *g2d_dev,
		struct m2m1shot2_context_format *ctx_fmt, u32 flags);
void g2d_hw_set_dest_premult(struct g2d1shot_dev *g2d_dev, u32 flags);
void g2d_hw_set_source_ycbcr(struct g2d1shot_ctx *g2d_ctx,
	int n, struct m2m1shot2_context_format *ctx_fmt);
void g2d_hw_set_dest_ycbcr(struct g2d1shot_dev *g2d_dev,
	struct m2m1shot2_context_format *ctx_fmt);
void g2d_hw_set_csc_coeff(struct g2d1shot_ctx *g2d_ctx);
void g2d_hw_set_tile_direction(struct g2d1shot_dev *g2d_dev,
				struct m2m1shot2_context *ctx);
#endif /* __EXYNOS_G2D1SHOT_H_ */
