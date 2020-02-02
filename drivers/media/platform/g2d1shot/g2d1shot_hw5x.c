/* linux/drivers/media/platform/exynos/fimg2d_v5/g2d1shot_hw5x.c
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

#include <linux/io.h>
#include <linux/sched.h>

#include "g2d1shot.h"
#include "g2d1shot_regs.h"

void g2d_hw_start(struct g2d1shot_dev *g2d_dev)
{
	/* start h/w */
	__raw_writel(G2D_BLIT_INT_ENABLE, g2d_dev->reg + G2D_INTEN_REG);
	__raw_writel(G2D_BLIT_INT_FLAG, g2d_dev->reg + G2D_INTC_PEND_REG);

	writel(G2D_START_BITBLT, g2d_dev->reg + G2D_BITBLT_START_REG);
}

/**
 * Four channels of the image are computed with:
 *	R = [ coeff(S)*Sc  + coeff(D)*Dc ]
 *	where
 *	Rc is result color or alpha
 *	Sc is source color or alpha
 *	Dc is destination color or alpha
 *
 * Caution: supposed that Sc and Dc are perpixel-alpha-premultiplied value
 *
 * MODE:             Formula
 * ----------------------------------------------------------------------------
 * FILL:
 * CLEAR:	     R = 0
 * SRC:		     R = Sc
 * DST:		     R = Dc
 * SRC_OVER:         R = Sc + (1-Sa)*Dc
 * DST_OVER:         R = (1-Da)*Sc + Dc
 * SRC_IN:	     R = Da*Sc
 * DST_IN:           R = Sa*Dc
 * SRC_OUT:          R = (1-Da)*Sc
 * DST_OUT:          R = (1-Sa)*Dc
 * SRC_ATOP:         R = Da*Sc + (1-Sa)*Dc
 * DST_ATOP:         R = (1-Da)*Sc + Sa*Dc
 * XOR:              R = (1-Da)*Sc + (1-Sa)*Dc
 * ADD:              R = Sc + Dc
 * MULTIPLY:         R = Sc*Dc
 * SCREEN:           R = Sc + (1-Sc)*Dc
 * DARKEN:           R = (Da*Sc<Sa*Dc)? Sc+(1-Sa)*Dc : (1-Da)*Sc+Dc
 * LIGHTEN:          R = (Da*Sc>Sa*Dc)? Sc+(1-Sa)*Dc : (1-Da)*Sc+Dc
 */
static struct g2d_blend_coeff const coeff_table[MAX_G2D_BLIT_OP] = {
	{ 0, COEFF_SA,		1, COEFF_SA },		/* None */
	{ 0, COEFF_ZERO,	0, COEFF_ZERO },	/* CLEAR */
	{ 0, COEFF_ONE,		0, COEFF_ZERO },	/* SRC */
	{ 0, COEFF_ZERO,	0, COEFF_ONE },		/* DST */
	{ 0, COEFF_ONE,		1, COEFF_SA },		/* SRC_OVER */
	{ 1, COEFF_DA,		0, COEFF_ONE },		/* DST_OVER */
	{ 0, COEFF_DA,		0, COEFF_ZERO },	/* SRC_IN */
	{ 0, COEFF_ZERO,	0, COEFF_SA },		/* DST_IN */
	{ 1, COEFF_DA,		0, COEFF_ZERO },	/* SRC_OUT */
	{ 0, COEFF_ZERO,	1, COEFF_SA },		/* DST_OUT */
	{ 0, COEFF_DA,		1, COEFF_SA },		/* SRC_ATOP */
	{ 1, COEFF_DA,		0, COEFF_SA },		/* DST_ATOP */
	{ 1, COEFF_DA,		1, COEFF_SA },		/* XOR */
	{ 0, COEFF_ONE,		0, COEFF_ONE },		/* ADD */
	{ 0, COEFF_DC,		0, COEFF_ZERO },	/* MULTIPLY */
	{ 0, COEFF_ONE,		1, COEFF_SC },		/* SCREEN */
	{ 0, 0, 0, 0 },					/* DARKEN */
	{ 0, 0, 0, 0 },					/* LIGHTEN */
};

/*
 * coefficient table with global (constant) alpha
 * replace COEFF_ONE with COEFF_GA
 *
 * MODE:             Formula with Global Alpha
 *                   (Ga is multiplied to both Sc and Sa)
 * ----------------------------------------------------------------------------
 * FILL:
 * CLEAR:	     R = 0
 * SRC:		     R = Ga*Sc
 * DST:		     R = Dc
 * SRC_OVER:         R = Ga*Sc + (1-Sa*Ga)*Dc
 * DST_OVER:         R = (1-Da)*Ga*Sc + Dc --> (W/A) 1st:Ga*Sc, 2nd:DST_OVER
 * SRC_IN:	     R = Da*Ga*Sc
 * DST_IN:           R = Sa*Ga*Dc
 * SRC_OUT:          R = (1-Da)*Ga*Sc --> (W/A) 1st: Ga*Sc, 2nd:SRC_OUT
 * DST_OUT:          R = (1-Sa*Ga)*Dc
 * SRC_ATOP:         R = Da*Ga*Sc + (1-Sa*Ga)*Dc
 * DST_ATOP:         R = (1-Da)*Ga*Sc + Sa*Ga*Dc -->
 *					(W/A) 1st: Ga*Sc, 2nd:DST_ATOP
 * XOR:              R = (1-Da)*Ga*Sc + (1-Sa*Ga)*Dc -->
 *					(W/A) 1st: Ga*Sc, 2nd:XOR
 * ADD:              R = Ga*Sc + Dc
 * MULTIPLY:         R = Ga*Sc*Dc --> (W/A) 1st: Ga*Sc, 2nd: MULTIPLY
 * SCREEN:           R = Ga*Sc + (1-Ga*Sc)*Dc --> (W/A) 1st: Ga*Sc, 2nd: SCREEN
 * DARKEN:           R = (W/A) 1st: Ga*Sc, 2nd: OP
 * LIGHTEN:          R = (W/A) 1st: Ga*Sc, 2nd: OP
 */
static struct g2d_blend_coeff const ga_coeff_table[MAX_G2D_BLIT_OP] = {
	{ 0, COEFF_SA,		1, COEFF_SA },		/* None */
	{ 0, COEFF_ZERO,	0, COEFF_ZERO },	/* CLEAR */
	{ 0, COEFF_GA,		0, COEFF_ZERO },	/* SRC */
	{ 0, COEFF_ZERO,	0, COEFF_ONE },		/* DST */
	{ 0, COEFF_GA,		1, COEFF_SA },		/* SRC_OVER */
	{ 1, COEFF_DA,		0, COEFF_ONE },		/* DST_OVER (use W/A) */
	{ 0, COEFF_DA,		0, COEFF_ZERO },	/* SRC_IN */
	{ 0, COEFF_ZERO,	0, COEFF_SA },		/* DST_IN */
	{ 1, COEFF_DA,		0, COEFF_ZERO },	/* SRC_OUT (use W/A) */
	{ 0, COEFF_ZERO,	1, COEFF_SA },		/* DST_OUT */
	{ 0, COEFF_DA,		1, COEFF_SA },		/* SRC_ATOP */
	{ 1, COEFF_DA,		0, COEFF_SA },		/* DST_ATOP (use W/A) */
	{ 1, COEFF_DA,		1, COEFF_SA },		/* XOR (use W/A) */
	{ 0, COEFF_GA,		0, COEFF_ONE },		/* ADD */
	{ 0, COEFF_DC,		0, COEFF_ZERO },	/* MULTIPLY (use W/A) */
	{ 0, COEFF_ONE,		1, COEFF_SC },		/* SCREEN (use W/A) */
	{ 0, 0,	0, 0 },					/* DARKEN (use W/A) */
	{ 0, 0,	0, 0 },					/* LIGHTEN (use W/A) */
};

void g2d_hw_set_alpha_composite(struct g2d1shot_dev *g2d_dev,
		int n, u16 composit_mode, unsigned char galpha)
{
	int alphamode;
	u32 cfg = 0;
	struct g2d_blend_coeff const *tbl;

	if (n == 1) {
		cfg = __raw_readl(g2d_dev->reg + G2D_LAYERn_COMMAND_REG(n));
		cfg |= G2D_LAYER_OPAQUE;

		if (galpha < 0xff)
			cfg |= G2D_PREMULT_GLOBAL_ALPHA;
		__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_COMMAND_REG(n));
		return;
	}

	switch (composit_mode) {
	case M2M1SHOT2_BLEND_DARKEN:
		cfg |= G2D_DARKEN;
		break;
	case M2M1SHOT2_BLEND_LIGHTEN:
		cfg |= G2D_LIGHTEN;
		break;
	default:
		if (galpha < 0xff) {	/* with global alpha */
			tbl = &ga_coeff_table[composit_mode];
			alphamode = ALPHA_PERPIXEL_MUL_GLOBAL;
		} else {
			tbl = &coeff_table[composit_mode];
			alphamode = ALPHA_PERPIXEL;
		}

		/* src coefficient */
		cfg |= tbl->s_coeff << G2D_SRC_COEFF_SHIFT;

		cfg |= alphamode << G2D_SRC_COEFF_SA_SHIFT;
		cfg |= alphamode << G2D_SRC_COEFF_DA_SHIFT;

		if (tbl->s_coeff_inv)
			cfg |= G2D_INV_SRC_COEFF;

		/* dst coefficient */
		cfg |= tbl->d_coeff << G2D_DST_COEFF_SHIFT;

		cfg |= alphamode << G2D_DST_COEFF_DA_SHIFT;
		cfg |= alphamode << G2D_DST_COEFF_SA_SHIFT;

		if (tbl->d_coeff_inv)
			cfg |= G2D_INV_DST_COEFF;

		break;
	}

	__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_BLEND_FUNCTION_REG(n));

	cfg = __raw_readl(g2d_dev->reg + G2D_LAYERn_COMMAND_REG(n));
	cfg |= G2D_ALPHA_BLEND_MODE;
	__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_COMMAND_REG(n));
}

void g2d_hw_set_source_blending(struct g2d1shot_dev *g2d_dev,
				int n, struct m2m1shot2_extra *ext)
{
	u32 cfg;

	cfg = ext->galpha;
	cfg |= ext->galpha << 8;
	cfg |= ext->galpha << 16;
	cfg |= ext->galpha << 24;

	__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_ALPHA_COLOR_REG(n));

	/* No more operations are needed for layer 0 */
	if (n == 0)
		return;

	g2d_hw_set_alpha_composite(g2d_dev, n, ext->composit_mode, ext->galpha);
}

void g2d_hw_set_source_premult(struct g2d1shot_dev *g2d_dev, int n, u32 flags)
{
	u32 cfg;

	/* set source premult */
	cfg = __raw_readl(g2d_dev->reg + G2D_LAYERn_COMMAND_REG(n));

	if (flags & M2M1SHOT2_IMGFLAG_PREMUL_ALPHA)
		cfg &= ~G2D_PREMULT_PER_PIXEL_MUL_GALPHA;
	else
		cfg |= G2D_PREMULT_PER_PIXEL_MUL_GALPHA;

	__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_COMMAND_REG(n));
}

void g2d_hw_set_source_format(struct g2d1shot_dev *g2d_dev, int n,
		struct m2m1shot2_context_format *ctx_fmt, bool compressed)
{
	struct v4l2_rect *s = &ctx_fmt->fmt.crop;
	struct v4l2_rect *d = &ctx_fmt->fmt.window;
	struct g2d1shot_fmt *fmt = ctx_fmt->priv;
	u32 cfg;

	/* set source rect */
	__raw_writel(s->left, g2d_dev->reg + G2D_LAYERn_LEFT_REG(n));
	__raw_writel(s->top, g2d_dev->reg + G2D_LAYERn_TOP_REG(n));
	__raw_writel(s->left + s->width,
			g2d_dev->reg + G2D_LAYERn_RIGHT_REG(n));
	__raw_writel(s->top + s->height,
			g2d_dev->reg + G2D_LAYERn_BOTTOM_REG(n));

	/* set dest clip */
	__raw_writel(d->left, g2d_dev->reg + G2D_LAYERn_DST_LEFT_REG(n));
	__raw_writel(d->top, g2d_dev->reg + G2D_LAYERn_DST_TOP_REG(n));
	__raw_writel(d->left + d->width,
			g2d_dev->reg + G2D_LAYERn_DST_RIGHT_REG(n));
	__raw_writel(d->top + d->height,
			g2d_dev->reg + G2D_LAYERn_DST_BOTTOM_REG(n));

	/* set pixel format and cbcr order */
	cfg = fmt->value;

	if (compressed) {
		cfg |= G2D_LAYER_COMP_FORMAT;
		/*
		 * other hardware (support the compressed format) uses rgb565 of bgr ordering
		 * thus, g2d hardware always set bgr ordering of rgb565 format forcely
		 */
		if (is_rgb565(fmt->value)) {
			cfg &= ~0xFFFF;
			cfg |= G2D_SWIZZLING_BGR;
		}
	}
	__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_COLOR_MODE_REG(n));

	/* image width and height */
	if (compressed) {
		/* set the width - 1 and height -1 on destination only */
		cfg = ctx_fmt->fmt.width;
		cfg = G2D_COMP_SET_WH(cfg);
		__raw_writel(cfg,
			g2d_dev->reg + G2D_LAYERn_IMAGE_WIDTH_REG(n));

		cfg = ctx_fmt->fmt.height;
		cfg = G2D_COMP_SET_WH(cfg);
		__raw_writel(cfg,
			g2d_dev->reg + G2D_LAYERn_IMAGE_HEIGHT_REG(n));
	} else if (is_yuv(fmt->value)) {
		cfg = ctx_fmt->fmt.width;
		__raw_writel(cfg,
			g2d_dev->reg + G2D_LAYERn_IMAGE_WIDTH_REG(n));

		cfg = ctx_fmt->fmt.height;
		__raw_writel(cfg,
			g2d_dev->reg + G2D_LAYERn_IMAGE_HEIGHT_REG(n));
	} else if (is_rgb(fmt->value)) { /* only RGB format */
		cfg = (fmt->bpp[0] * ctx_fmt->fmt.width / 8);
		__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_STRIDE_REG(n));
	}
}

void g2d_hw_set_source_address(struct m2m1shot2_context *ctx,
		struct m2m1shot2_context_format *ctx_fmt,
		struct g2d1shot_dev *g2d_dev, int n, bool compressed)
{
	struct g2d1shot_fmt *fmt = ctx_fmt->priv;
	dma_addr_t addr = m2m1shot2_src_dma_addr(ctx, n, 0);

	if (compressed) {
		__raw_writel(addr,
			g2d_dev->reg + G2D_LAYERn_PAYLOAD_BASE_ADDR_REG(n));
		__raw_writel(addr,
			g2d_dev->reg + G2D_LAYERn_HEADER_BASE_ADDR_REG(n));
	} else {
		if (fmt->num_planes == 2) {
			dma_addr_t addr_cb;
			u32 w = ctx_fmt->fmt.width;
			u32 h = ctx_fmt->fmt.height;

			if (fmt->pixelformat == V4L2_PIX_FMT_NV12N) {
				addr_cb = NV12N_CBCR_BASE(addr, w, h);
			} else if (fmt->pixelformat == V4L2_PIX_FMT_NV12N_10B) {
				addr_cb = NV12N_10B_CBCR_BASE(addr, w, h);
			} else if (fmt->pixelformat == V4L2_PIX_FMT_NV12M ||
				(fmt->pixelformat == V4L2_PIX_FMT_NV21M)) {
				addr_cb = m2m1shot2_src_dma_addr(ctx, n, 1);
			} else { /* contiguous format */
				addr_cb = addr + w * h;
			}
			__raw_writel(addr_cb, g2d_dev->reg + G2D_LAYERn_PLANE2_BASE_ADDR_REG(n));
		}
		__raw_writel(addr, g2d_dev->reg + G2D_LAYERn_BASE_ADDR_REG(n));
	}

}

void g2d_hw_set_source_repeat(struct g2d1shot_dev *g2d_dev, int n,
		struct m2m1shot2_extra *ext)
{
	u32 cfg = 0;

	/* set repeat, or default CLAMP */
	if (ext->xrepeat)
		cfg |= G2D_LAYER_REPEAT_X_SET(ext->xrepeat - 1);
	else if (ext->xrepeat == M2M1SHOT2_REPEAT_NONE)
		cfg |= G2D_LAYER_REPEAT_X_CLAMP;

	if (ext->yrepeat)
		cfg |= G2D_LAYER_REPEAT_Y_SET(ext->yrepeat - 1);
	else if (ext->yrepeat == M2M1SHOT2_REPEAT_NONE)
		cfg |= G2D_LAYER_REPEAT_Y_CLAMP;

	__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_REPEAT_MODE_REG(n));

	/* repeat pad color */
	if (ext->xrepeat == M2M1SHOT2_REPEAT_PAD ||
			ext->yrepeat == M2M1SHOT2_REPEAT_PAD)
		__raw_writel(ext->fillcolor,
				g2d_dev->reg + G2D_LAYERn_PAD_VALUE_REG(n));
}

#define MAX_PRECISION		16
#define DEFAULT_SCALE_RATIO	0x10000

/**
 * scale_factor_to_fixed16 - convert scale factor to fixed point 16
 * @n: numerator
 * @d: denominator
 */
static inline u32 scale_factor_to_fixed16(u32 n, u32 d)
{
	u64 value;

	value = (u64)n << MAX_PRECISION;
	value /= d;

	return (u32)(value & 0xffffffff);
}

void g2d_hw_set_source_scale(struct g2d1shot_dev *g2d_dev, int n,
		struct m2m1shot2_extra *ext, u32 flags,
		struct m2m1shot2_context_format *ctx_fmt)
{
	struct v4l2_rect *s = &ctx_fmt->fmt.crop;
	struct v4l2_rect *d = &ctx_fmt->fmt.window;
	u32 wcfg, hcfg;
	u32 mode;

	/* set scaling ratio, default NONE */
	/*
	 * scaling ratio in pixels
	 * e.g scale-up: src(1,1)-->dst(2,2), src factor: 0.5 (0x000080000)
	 *     scale-down: src(2,2)-->dst(1,1), src factor: 2.0 (0x000200000)
	 */

	/* inversed scaling factor: src is numerator */

	if (flags & M2M1SHOT2_IMGFLAG_XSCALE_FACTOR) {
		wcfg = ext->horizontal_factor;
	} else {
		if (ext->transform & M2M1SHOT2_IMGTRFORM_ROT90)
			wcfg = scale_factor_to_fixed16(s->width, d->height);
		else
			wcfg = scale_factor_to_fixed16(s->width, d->width);
	}

	if (flags & M2M1SHOT2_IMGFLAG_YSCALE_FACTOR) {
		hcfg = ext->vertical_factor;
	} else {
		if (ext->transform & M2M1SHOT2_IMGTRFORM_ROT90)
			hcfg = scale_factor_to_fixed16(s->height, d->width);
		else
			hcfg = scale_factor_to_fixed16(s->height, d->height);
	}

	if (wcfg == DEFAULT_SCALE_RATIO && hcfg == DEFAULT_SCALE_RATIO)
		return;

	__raw_writel(wcfg, g2d_dev->reg + G2D_LAYERn_XSCALE_REG(n));
	__raw_writel(hcfg, g2d_dev->reg + G2D_LAYERn_YSCALE_REG(n));

	/* scaling algorithm */
	if (ext->scaler_filter == M2M1SHOT2_SCFILTER_BILINEAR)
		mode = G2D_LAYER_SCALE_MODE_BILINEAR;
	else if (ext->scaler_filter == M2M1SHOT2_SCFILTER_NEAREST)
		mode = G2D_LAYER_SCALE_MODE_NEAREST;
	else
		mode = 0x0;

	__raw_writel(mode, g2d_dev->reg + G2D_LAYERn_SCALE_CTRL_REG(n));
}

void g2d_hw_set_source_rotate(struct g2d1shot_dev *g2d_dev, int n,
		struct m2m1shot2_extra *ext)
{
	u32 cfg = 0;
	u32 rot_mask = 0x3;

	if (ext->transform & M2M1SHOT2_IMGTRFORM_XFLIP)
		cfg |= G2D_LAYER_Y_DIR_NEGATIVE;
	if (ext->transform & M2M1SHOT2_IMGTRFORM_YFLIP)
		cfg |= G2D_LAYER_X_DIR_NEGATIVE;

	switch (ext->transform & rot_mask) {
	case M2M1SHOT2_IMGTRFORM_ROT90:
		cfg ^= G2D_LAYER_Y_DIR_NEGATIVE | G2D_LAYER_X_DIR_NEGATIVE | G2D_LAYER_ROTATE;
		break;
	case M2M1SHOT2_IMGTRFORM_ROT180:
		cfg ^= G2D_LAYER_Y_DIR_NEGATIVE | G2D_LAYER_X_DIR_NEGATIVE;
		break;
	case M2M1SHOT2_IMGTRFORM_ROT270:
		cfg ^= G2D_LAYER_ROTATE;
		break;
	}

	/*
	 * H/W rotate the image before flip,
	 * but user requests to rotate the image after flip.
	 * we reverse the direction to fix the diffrence when rotation.
	 */
	if (cfg & G2D_LAYER_ROTATE) {
		bool y_flip = !!(cfg & G2D_LAYER_X_DIR_NEGATIVE);
		bool x_flip = !!(cfg & G2D_LAYER_Y_DIR_NEGATIVE);

		if (x_flip != y_flip) {
			cfg ^= (G2D_LAYER_Y_DIR_NEGATIVE | G2D_LAYER_X_DIR_NEGATIVE);
		}
	}

	__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_DIRECT_REG(n));
}

void g2d_hw_set_tile_direction(struct g2d1shot_dev *g2d_dev,
				struct m2m1shot2_context *ctx)
{
	struct m2m1shot2_context_format *ctx_fmt;
	struct m2m1shot2_extra *ext;
	struct v4l2_rect *s;
	unsigned int non_rotate = 0;
	unsigned int rotate = 0;
	int i;

	for (i = 0; i < ctx->num_sources; i++) {
		ext = &ctx->source[i].ext;
		ctx_fmt = m2m1shot2_src_format(ctx, i);
		s = &ctx_fmt->fmt.crop;

		if (ext->transform & 0x1)
			rotate += s->width * s->height;
		else
			non_rotate += s->width * s->height;
	}

	if (rotate > non_rotate)
		__raw_writel(0x1, g2d_dev->reg + G2D_TILE_DIRECTION_ORDER_REG);
}

void g2d_hw_set_source_valid(struct g2d1shot_dev *g2d_dev, int n)
{
	u32 cfg;

	/* set valid flag */
	cfg = __raw_readl(g2d_dev->reg + G2D_LAYERn_COMMAND_REG(n));
	cfg |= G2D_LAYER_VALID;

	__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_COMMAND_REG(n));

	/* set update layer flag */
	cfg = __raw_readl(g2d_dev->reg + G2D_LAYER_UPDATE_REG);
	cfg |= (1 << n);

	__raw_writel(cfg, g2d_dev->reg + G2D_LAYER_UPDATE_REG);
}

void g2d_hw_set_dest_addr(struct m2m1shot2_context *ctx,
		struct m2m1shot2_context_format *ctx_fmt,
		struct g2d1shot_dev *g2d_dev, bool compressed)
{
	struct g2d1shot_fmt *fmt = ctx_fmt->priv;
	u32 addr = (u32)m2m1shot2_dst_dma_addr(ctx, 0);

	if (compressed) {
		/* payload = header +
		 * ceil(width/ 16) * ceil(height / 16) * 16
		 * the value of payload must be 64byte-aligned
		 */
		u32 header_len = DIV_ROUND_UP(ctx_fmt->fmt.width, G2D_COMP_BLOCK_SIZE)
				 * DIV_ROUND_UP(ctx_fmt->fmt.height, G2D_COMP_BLOCK_SIZE)
				 * G2D_COMP_META_LEN;
		u32 pld = addr + ALIGN(header_len, G2D_COMP_ALIGN_DST_ADDR);

		__raw_writel(addr, g2d_dev->reg + G2D_DST_HEADER_BASE_ADDR_REG);
		__raw_writel(pld, g2d_dev->reg + G2D_DST_PAYLOAD_BASE_ADDR_REG);
	} else {
		if (fmt->num_planes == 2) {
			dma_addr_t addr_cb;
			u32 w = ctx_fmt->fmt.width;
			u32 h = ctx_fmt->fmt.height;

			if (fmt->pixelformat == V4L2_PIX_FMT_NV12N) {
				addr_cb = NV12N_CBCR_BASE(addr, w, h);
			} else if (fmt->pixelformat == V4L2_PIX_FMT_NV12N_10B) {
				addr_cb = NV12N_10B_CBCR_BASE(addr, w, h);
			} else if (fmt->pixelformat == V4L2_PIX_FMT_NV12M ||
				(fmt->pixelformat == V4L2_PIX_FMT_NV21M)) {
				addr_cb = m2m1shot2_dst_dma_addr(ctx, 1);
			} else { /* contiguous format */
				addr_cb = addr + (u32)(w * h);
			}
			__raw_writel(addr_cb, g2d_dev->reg + G2D_DST_PLANE2_BASE_ADDR_REG);
		}
		__raw_writel(addr, g2d_dev->reg + G2D_DST_BASE_ADDR_REG);
	}
}

void g2d_hw_set_dest_format(struct g2d1shot_dev *g2d_dev,
				struct m2m1shot2_context_format *ctx_fmt, u32 flags)
{
	struct g2d1shot_fmt *fmt = ctx_fmt->priv;
	struct v4l2_rect *d = &ctx_fmt->fmt.crop;
	u32 cfg;
	bool u_order = flags & M2M1SHOT2_IMGFLAG_UORDER_ADDR;
	bool compressed = flags & M2M1SHOT2_IMGFLAG_COMPRESSED;

	/* set dest rect */
	__raw_writel(d->left, g2d_dev->reg + G2D_DST_LEFT_REG);
	__raw_writel(d->top, g2d_dev->reg + G2D_DST_TOP_REG);
	__raw_writel(d->left + d->width, g2d_dev->reg + G2D_DST_RIGHT_REG);
	__raw_writel(d->top + d->height, g2d_dev->reg + G2D_DST_BOTTOM_REG);

	/* set dest pixelformat */
	cfg = fmt->value;
	if (compressed) {
		cfg |= G2D_LAYER_COMP_FORMAT;
		/*
		 * other hardware (support the compressed format) uses rgb565 of bgr ordering
		 * thus, g2d hardware always set bgr ordering of rgb565 format forcely
		 */
		if (is_rgb565(fmt->value)) {
			cfg &= ~0xFFFF;
			cfg |= G2D_SWIZZLING_BGR;
		}
	}
	if (u_order)
		cfg |= G2D_LAYER_UORDER_ADDR;

	__raw_writel(cfg, g2d_dev->reg + G2D_DST_COLOR_MODE_REG);

	if (compressed) {
		cfg = ctx_fmt->fmt.width;
		__raw_writel(cfg,
			g2d_dev->reg + G2D_DST_IMAGE_WIDTH_REG);

		cfg = ctx_fmt->fmt.height;
		__raw_writel(cfg,
			g2d_dev->reg + G2D_DST_IMAGE_HEIGHT_REG);
	} else if (is_yuv(fmt->value)) {
		cfg = ctx_fmt->fmt.width;
		__raw_writel(cfg,
			g2d_dev->reg + G2D_DST_IMAGE_WIDTH_REG);

		cfg = ctx_fmt->fmt.height;
		__raw_writel(cfg,
			g2d_dev->reg + G2D_DST_IMAGE_HEIGHT_REG);
	} else if (is_rgb(fmt->value)) {
		cfg = (fmt->bpp[0] * ctx_fmt->fmt.width / 8);
		__raw_writel(cfg, g2d_dev->reg + G2D_DST_STRIDE_REG);
	}

	/* set the [13:4] of the half of image for parallel processing */
	cfg = ((int)(d->width / 2)) >> 4;
	cfg |= G2D_DST_SPLIT_TILE_IDX_VFLAG;
	__raw_writel(cfg, g2d_dev->reg + G2D_DST_SPLIT_TILE_IDX_REG);
}

void g2d_hw_set_dest_premult(struct g2d1shot_dev *g2d_dev, u32 flags)
{
	u32 cfg;

	/* set dest premult if flag is set */
	cfg = __raw_readl(g2d_dev->reg + G2D_BITBLT_COMMAND_REG);

	if (!(flags & M2M1SHOT2_IMGFLAG_PREMUL_ALPHA))
		cfg |= G2D_DST_DE_PREMULT;

	__raw_writel(cfg, g2d_dev->reg + G2D_BITBLT_COMMAND_REG);
}

static unsigned short csc_y2r[G2D_MAX_CSC_FMT][9] = {
	/* REC.601 Narrow */
	{ 0x254, 0x000, 0x331, 0x254, 0xF38, 0xE60, 0x254, 0x409, 0x000 },
	/* REC.601 Wide */
	{ 0x200, 0x000, 0x2BE, 0x200, 0xF54, 0xE9B, 0x200, 0x377, 0x000 },
	/* REC.709 Narrow */
	{ 0x254, 0x000, 0x396, 0x254, 0xF93, 0xEEF, 0x254, 0x43B, 0x000 },
	/* REC.709 Wide */
	{ 0x200, 0x000, 0x314, 0x200, 0xFA2, 0xF15, 0x200, 0x3A2, 0x000 },
	/* BT.2020 Narrow */
	{ 0x254, 0x000, 0x36F, 0x254, 0xF9E, 0xEAC, 0x254, 0x461, 0x000 },
};

static const struct g2d_csc_fmt csc_fmt[] = {
	{
		.v4l2_colorspace = V4L2_COLORSPACE_SMPTE170M,
		.range = 0,
		.colorspace = G2D_COLORSPACE_601,
		.index = 0,
	}, {
		.v4l2_colorspace = V4L2_COLORSPACE_JPEG,
		.range = 1,
		.colorspace = G2D_COLORSPACE_601,
		.index = 1,
	}, {
		.v4l2_colorspace = V4L2_COLORSPACE_REC709,
		.range = 0,
		.colorspace = G2D_COLORSPACE_709,
		.index = 2,
	}, {
		.v4l2_colorspace = V4L2_COLORSPACE_SRGB,
		.range = 1,
		.colorspace = G2D_COLORSPACE_709,
		.index = 3,
	}, {
		.v4l2_colorspace = V4L2_COLORSPACE_BT2020,
		.range = 0,
		.colorspace = G2D_COLORSPACE_2020,
		.index = 4,
	},
};

const struct g2d_csc_fmt *find_colorspace(u32 v4l2_colorspace)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(csc_fmt); i++) {
		if (v4l2_colorspace == csc_fmt[i].v4l2_colorspace) {
			return &csc_fmt[i];
		}
	}
	return NULL;
}


void g2d_hw_set_csc_coeff(struct g2d1shot_ctx *g2d_ctx)
{
	struct g2d1shot_dev *g2d_dev = g2d_ctx->g2d_dev;
	unsigned char offset = G2D_LAYER_CSC0_COEFF10_REG - G2D_LAYER_CSC0_COEFF00_REG;
	int idx = 0;
	int i, j, m;

	for (m = 0; m < G2D_MAX_CSC_FMT; m++) {
		idx = g2d_ctx->csc_idx_map[m];
		if (idx < 0)
			continue;

		for (i = 0, j = 0; i < ARRAY_SIZE(csc_y2r[0]); i++, j += offset) {
			writel_relaxed(csc_y2r[m][i], g2d_dev->reg +
				G2D_LAYER_CSCn_COEFF00_REG(idx) + j);
		}
	}
}

void g2d_hw_set_source_ycbcr(struct g2d1shot_ctx *g2d_ctx, int n,
	struct m2m1shot2_context_format *ctx_fmt)
{
	u32 cfg = 0;
	struct g2d1shot_dev *g2d_dev = g2d_ctx->g2d_dev;
	const struct g2d_csc_fmt *g2d_csc_fmt;
	struct g2d1shot_fmt *fmt = ctx_fmt->priv;

	if (!is_yuv(fmt->value))
		return;

	g2d_csc_fmt = find_colorspace(ctx_fmt->colorspace);
	cfg = g2d_csc_fmt->range << G2D_LAYER_YCBCR_RANGE_SHIFT |
			g2d_ctx->src_csc_value[n];

	__raw_writel(cfg, g2d_dev->reg + G2D_LAYERn_YCBCR_MODE_REG(n));
}

void g2d_hw_set_dest_ycbcr(struct g2d1shot_dev *g2d_dev,
	struct m2m1shot2_context_format *ctx_fmt)
{
	u32 cfg = 0;
	const struct g2d_csc_fmt *g2d_csc_fmt;
	struct g2d1shot_fmt *fmt = ctx_fmt->priv;

	if (!is_yuv(fmt->value))
		return;

	g2d_csc_fmt = find_colorspace(ctx_fmt->colorspace);

	if (g2d_csc_fmt->colorspace == G2D_COLORSPACE_601) {
		cfg = 0;
	} else if (g2d_csc_fmt->colorspace == G2D_COLORSPACE_709) {
		cfg = 1;
	}

	cfg |= g2d_csc_fmt->range << G2D_LAYER_YCBCR_RANGE_SHIFT;
	cfg |= G2D_CBCROFFSET_0_50 << G2D_LAYER_YCBCR_RANGE_OFFSET_X;
	cfg |= G2D_CBCROFFSET_0_50 << G2D_LAYER_YCBCR_RANGE_OFFSET_Y;

	__raw_writel(cfg, g2d_dev->reg + G2D_DST_YCBCR_MODE_REG);
}
