/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_cmd_v6.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <trace/events/mfc.h>

#include "s5p_mfc_cmd.h"

#include "s5p_mfc_cal.h"
#include "s5p_mfc_reg.h"

#include "s5p_mfc_utils.h"
#include "s5p_mfc_buf.h"

int s5p_mfc_cmd_sys_init(struct s5p_mfc_dev *dev,
					enum mfc_buf_usage_type buf_type)
{
	struct s5p_mfc_buf_size_v6 *buf_size;
	struct s5p_mfc_special_buf *ctx_buf;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	s5p_mfc_clean_dev_int_flags(dev);

	buf_size = dev->variant->buf_size->buf;
	ctx_buf = &dev->common_ctx_buf;
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (buf_type == MFCBUF_DRM)
		ctx_buf = &dev->drm_common_ctx_buf;
#endif
	MFC_WRITEL(ctx_buf->daddr, S5P_FIMV_CONTEXT_MEM_ADDR);
	MFC_WRITEL(buf_size->dev_ctx, S5P_FIMV_CONTEXT_MEM_SIZE);

	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_SYS_INIT);

	mfc_debug_leave();

	return 0;
}

void s5p_mfc_cmd_sleep(struct s5p_mfc_dev *dev)
{
	mfc_debug_enter();

	s5p_mfc_clean_dev_int_flags(dev);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_SLEEP);

	mfc_debug_leave();
}

void s5p_mfc_cmd_wakeup(struct s5p_mfc_dev *dev)
{
	mfc_debug_enter();

	s5p_mfc_clean_dev_int_flags(dev);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_WAKEUP);

	mfc_debug_leave();
}

/* Open a new instance and get its number */
int s5p_mfc_cmd_open_inst(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;

	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}
	dev = ctx->dev;
	mfc_debug(2, "Requested codec mode: %d\n", ctx->codec_mode);

	MFC_WRITEL(ctx->codec_mode, S5P_FIMV_CODEC_TYPE);
	MFC_WRITEL(ctx->instance_ctx_buf.daddr, S5P_FIMV_CONTEXT_MEM_ADDR);
	MFC_WRITEL(ctx->instance_ctx_buf.size, S5P_FIMV_CONTEXT_MEM_SIZE);
	if (ctx->type == MFCINST_DECODER)
		MFC_WRITEL(ctx->dec_priv->crc_enable, S5P_FIMV_D_CRC_CTRL);

	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_OPEN_INSTANCE);

	mfc_debug_leave();

	return 0;
}

/* Close instance */
int s5p_mfc_cmd_close_inst(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;

	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}
	dev = ctx->dev;

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);

	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_CLOSE_INSTANCE);

	mfc_debug_leave();

	return 0;
}

int s5p_mfc_cmd_dpb_flush(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;

	if (ON_RES_CHANGE(ctx))
		mfc_err_ctx("dpb flush on res change(state:%d)\n", ctx->state);

	s5p_mfc_clean_ctx_int_flags(ctx);

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_DPB_FLUSH);

	return 0;
}

int s5p_mfc_cmd_cache_flush(struct s5p_mfc_dev *dev)
{
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	s5p_mfc_clean_dev_int_flags(dev);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_CACHE_FLUSH);

	return 0;
}

int s5p_mfc_cmd_dec_init_buffers(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	unsigned int reg = 0, pix_val, mem_type = 0;
	int ret;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}
	dec = ctx->dec_priv;
	dev = ctx->dev;
	if (!dev) {
		mfc_err_ctx("no mfc device to run\n");
		return -EINVAL;
	}
	/* Initializing decoding - parsing header */
	/* Header was parsed now starting processing
	 * First set the output frame buffers
	 * s5p_mfc_alloc_dec_buffers(ctx); */

	switch (ctx->dst_fmt->fourcc) {
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV12N:
	case V4L2_PIX_FMT_NV12MT_16X16:
	case V4L2_PIX_FMT_NV16M:
		pix_val = 0;
		break;
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV61M:
		pix_val = 1;
		break;
	case V4L2_PIX_FMT_YVU420M:
		pix_val = 2;
		break;
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YUV420N:
		pix_val = 3;
		break;
	/* 10bit */
	case V4L2_PIX_FMT_NV12N_10B:
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV16M_S10B:
		mem_type = 0;
		pix_val = 0;
		break;
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV16M_P210:
		mem_type = 1;
		pix_val = 0;
		break;
	case V4L2_PIX_FMT_NV21M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		mem_type = 0;
		pix_val = 1;
		break;
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV61M_P210:
		mem_type = 1;
		pix_val = 1;
		break;
	default:
		pix_val = 0;
		break;
	}
	reg = 0;
	reg |= pix_val;
	reg |= (mem_type << 4);
	MFC_WRITEL(reg, S5P_FIMV_PIXEL_FORMAT);
	mfc_debug(2, "pixel format: %d, mem_type for 10bit: %d (reg: %#x)\n",
			pix_val, mem_type, reg);

	s5p_mfc_clean_ctx_int_flags(ctx);
	ret = s5p_mfc_set_dec_codec_buffers(ctx);
	if (ret) {
		mfc_info_ctx("isn't enough codec buffer size, re-alloc!\n");
		s5p_mfc_release_codec_buffers(ctx);
		ret = s5p_mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate decoding buffers.\n");
			return ret;
		}
		ret = s5p_mfc_set_dec_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to alloc frame mem.\n");
			return ret;
		}
	}

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_INIT_BUFFERS);

	return ret;
}

int s5p_mfc_cmd_enc_init_buffers(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	int ret;

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	/* Header was generated now starting processing
	 * First set the reference frame buffers
	 */
	if (!ctx->codec_buf.handle) {
		mfc_info_ctx("there isn't codec buffer, re-alloc!\n");
		ret = s5p_mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate encoding buffers.\n");
			return ret;
		}
	}

	s5p_mfc_clean_ctx_int_flags(ctx);
	ret = s5p_mfc_set_enc_codec_buffers(ctx);
	if (ret) {
		mfc_info_ctx("isn't enough codec buffer size, re-alloc!\n");
		s5p_mfc_release_codec_buffers(ctx);
		ret = s5p_mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate encoding buffers.\n");
			return ret;
		}
		ret = s5p_mfc_set_enc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to set enc codec buffers.\n");
			return ret;
		}
	}

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_INIT_BUFFERS);

	return ret;
}
