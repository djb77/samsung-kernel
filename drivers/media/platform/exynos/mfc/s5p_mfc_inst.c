/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_inst.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "s5p_mfc_inst.h"

#include "s5p_mfc_cmd.h"
#include "s5p_mfc_enc_param.h"
#include "s5p_mfc_reg.h"

#include "s5p_mfc_utils.h"

extern int nal_q_parallel_enable;
int s5p_mfc_open_inst(struct s5p_mfc_ctx *ctx)
{
	int ret;

	/* Preparing decoding - getting instance number */
	mfc_debug(2, "Getting instance number\n");
	s5p_mfc_clean_ctx_int_flags(ctx);
	ret = s5p_mfc_cmd_open_inst(ctx);
	if (ret) {
		mfc_err_ctx("Failed to create a new instance.\n");
		s5p_mfc_change_state(ctx, MFCINST_ERROR);
	}

	return ret;
}

int s5p_mfc_close_inst(struct s5p_mfc_ctx *ctx)
{
	int ret = -EINVAL;

	/* Closing decoding instance  */
	mfc_debug(2, "Returning instance number\n");
	s5p_mfc_clean_ctx_int_flags(ctx);
	if (ctx->state == MFCINST_FREE) {
		mfc_err_ctx("ctx already free status\n");
		return ret;
	}

	ret = s5p_mfc_cmd_close_inst(ctx);
	if (ret) {
		mfc_err_ctx("Failed to return an instance.\n");
		s5p_mfc_change_state(ctx, MFCINST_ERROR);
	}

	return ret;
}

int s5p_mfc_abort_inst(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	s5p_mfc_clean_ctx_int_flags(ctx);

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_NAL_ABORT);

	return 0;
}

/* Initialize decoding */
int s5p_mfc_init_decode(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	unsigned int reg = 0, pix_val;
	int fmo_aso_ctrl = 0;

	mfc_debug_enter();
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}
	mfc_debug(2, "InstNo: %d/%d\n", ctx->inst_no, S5P_FIMV_H2R_CMD_SEQ_HEADER);
	mfc_debug(2, "BUFs: %08x\n", MFC_READL(S5P_FIMV_D_CPB_BUFFER_ADDR));

	/* When user sets desplay_delay to 0,
	 * It works as "display_delay enable" and delay set to 0.
	 * If user wants display_delay disable, It should be
	 * set to negative value. */
	if (dec->display_delay >= 0) {
		reg |= (0x1 << S5P_FIMV_D_DEC_OPT_DISPLAY_DELAY_EN_SHIFT);
		MFC_WRITEL(dec->display_delay, S5P_FIMV_D_DISPLAY_DELAY);
	}

	/* FMO_ASO_CTRL - 0: Enable, 1: Disable */
	reg |= ((fmo_aso_ctrl & S5P_FIMV_D_DEC_OPT_FMO_ASO_CTRL_MASK)
			<< S5P_FIMV_D_DEC_OPT_FMO_ASO_CTRL_SHIFT);

	reg |= ((dec->idr_decoding & S5P_FIMV_D_DEC_OPT_IDR_DECODING_MASK)
			<< S5P_FIMV_D_DEC_OPT_IDR_DECODING_SHIFT);

	/* VC1 RCV: Discard to parse additional header as default */
	if (IS_VC1_RCV_DEC(ctx))
		reg |= (0x1 << S5P_FIMV_D_DEC_OPT_DISCARD_RCV_HEADER_SHIFT);

	/* conceal control to specific color */
	if (FW_HAS_CONCEAL_CONTROL(dev))
		reg |= (0x4 << S5P_FIMV_D_DEC_OPT_CONCEAL_CONTROL_SHIFT);

	/* Disable parallel processing in FW when using NAL-Q
	   It has to be enabled 11.0 */
	if (!nal_q_parallel_enable)
		reg |= (0x2 << S5P_FIMV_D_DEC_OPT_PARALLEL_PROCESSING_SHIFT);

	/* Realloc buffer for resolution decrease case in NAL QUEUE mode */
	reg |= (0x1 << S5P_FIMV_D_DEC_OPT_REALLOC_CONTROL_SHIFT);

	/* Parsing all including PPS */
	reg |= (0x1 << S5P_FIMV_D_DEC_OPT_SPECIAL_PARSING_SHIFT);

	MFC_WRITEL(reg, S5P_FIMV_D_DEC_OPTIONS);

	if (FW_HAS_CONCEAL_CONTROL(dev))
		MFC_WRITEL(MFC_CONCEAL_COLOR, S5P_FIMV_D_FORCE_PIXEL_VAL);

	if (IS_FIMV1_DEC(ctx)) {
		mfc_debug(2, "Setting FIMV1 resolution to %dx%d\n",
					ctx->img_width, ctx->img_height);
		MFC_WRITEL(ctx->img_width, S5P_FIMV_D_SET_FRAME_WIDTH);
		MFC_WRITEL(ctx->img_height, S5P_FIMV_D_SET_FRAME_HEIGHT);
	}

	switch (ctx->dst_fmt->fourcc) {
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV12N:
	case V4L2_PIX_FMT_NV12N_10B:
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
	default:
		pix_val = 0;
		break;
	}
	MFC_WRITEL(pix_val, S5P_FIMV_PIXEL_FORMAT);

	reg = 0;
	/* Enable realloc interface if SEI is enabled */
	if (dec->sei_parse)
		reg |= (0x1 << S5P_FIMV_D_SEI_ENABLE_NEED_INIT_BUFFER_SHIFT);
	if (FW_HAS_SEI_INFO_FOR_HDR(dev)) {
		reg |= (0x1 << S5P_FIMV_D_SEI_ENABLE_CONTENT_LIGHT_SHIFT);
		reg |= (0x1 << S5P_FIMV_D_SEI_ENABLE_MASTERING_DISPLAY_SHIFT);
	}
	reg |= (0x1 << S5P_FIMV_D_SEI_ENABLE_RECOVERY_PARSING_SHIFT);

	MFC_WRITEL(reg, S5P_FIMV_D_SEI_ENABLE);
	mfc_debug(2, "SEI enable was set, 0x%x\n", MFC_READL(S5P_FIMV_D_SEI_ENABLE));

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_SEQ_HEADER);

	mfc_debug_leave();
	return 0;
}

/* Decode a single frame */
int s5p_mfc_decode_one_frame(struct s5p_mfc_ctx *ctx, int last_frame)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	u32 reg = 0;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}

	mfc_debug(2, "Dynamic:0x%08x, Available:0x%lx\n",
			dec->dynamic_set, dec->available_dpb);

	reg = MFC_READL(S5P_FIMV_D_NAL_START_OPTIONS);
	reg &= ~(0x1 << S5P_FIMV_D_NAL_START_OPT_BLACK_BAR_SHIFT);
	reg |= ((dec->detect_black_bar & 0x1) << S5P_FIMV_D_NAL_START_OPT_BLACK_BAR_SHIFT);
	MFC_WRITEL(reg, S5P_FIMV_D_NAL_START_OPTIONS);
	mfc_debug(3, "black bar detect set: %#x\n", reg);

	MFC_WRITEL(dec->dynamic_set, S5P_FIMV_D_DYNAMIC_DPB_FLAG_LOWER);
	MFC_WRITEL(0x0, S5P_FIMV_D_DYNAMIC_DPB_FLAG_UPPER);
	MFC_WRITEL(dec->available_dpb, S5P_FIMV_D_AVAILABLE_DPB_FLAG_LOWER);
	MFC_WRITEL(0x0, S5P_FIMV_D_AVAILABLE_DPB_FLAG_UPPER);
	MFC_WRITEL(dec->slice_enable, S5P_FIMV_D_SLICE_IF_ENABLE);
	MFC_WRITEL(MFC_TIMEOUT_VALUE, S5P_FIMV_DEC_TIMEOUT_VALUE);

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	/* Issue different commands to instance basing on whether it
	 * is the last frame or not. */
	switch (last_frame) {
	case 0:
		s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_NAL_START);
		break;
	case 1:
		s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_LAST_FRAME);
		break;
	}

	mfc_debug(2, "Decoding a usual frame.\n");
	return 0;
}

int s5p_mfc_init_encode(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;

	mfc_debug(2, "++\n");

	if (IS_H264_ENC(ctx))
		s5p_mfc_set_enc_params_h264(ctx);
	else if (IS_MPEG4_ENC(ctx))
		s5p_mfc_set_enc_params_mpeg4(ctx);
	else if (IS_H263_ENC(ctx))
		s5p_mfc_set_enc_params_h263(ctx);
	else if (IS_VP8_ENC(ctx))
		s5p_mfc_set_enc_params_vp8(ctx);
	else if (IS_VP9_ENC(ctx))
		s5p_mfc_set_enc_params_vp9(ctx);
	else if (IS_HEVC_ENC(ctx))
		s5p_mfc_set_enc_params_hevc(ctx);
	else {
		mfc_err_ctx("Unknown codec for encoding (%x).\n",
			ctx->codec_mode);
		return -EINVAL;
	}

	mfc_debug(5, "RC) Bitrate: %d / framerate: %#x / config %#x / mode %#x\n",
			MFC_READL(S5P_FIMV_E_RC_BIT_RATE),
			MFC_READL(S5P_FIMV_E_RC_FRAME_RATE),
			MFC_READL(S5P_FIMV_E_RC_CONFIG),
			MFC_READL(S5P_FIMV_E_RC_MODE));

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_SEQ_HEADER);

	mfc_debug(2, "--\n");

	return 0;
}

static int mfc_h264_set_aso_slice_order(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_enc_params *p = &enc->params;
	struct s5p_mfc_h264_enc_params *p_264 = &p->codec.h264;
	int i;

	if (p_264->aso_enable) {
		for (i = 0; i < 8; i++)
			MFC_WRITEL(p_264->aso_slice_order[i],
				S5P_FIMV_E_H264_ASO_SLICE_ORDER_0 + i * 4);
	}
	return 0;
}

/*
	When the resolution is changed,
	s5p_mfc_start_change_resol_enc() should be called right before NAL_START.
	return value
	0: no resolution change
	1: resolution swap
	2: resolution change
*/
static int mfc_start_change_resol_enc(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_enc_params *p = &enc->params;
	unsigned int reg = 0;
	int old_img_width;
	int old_img_height;
	int new_img_width;
	int new_img_height;
	int ret = 0;

	if ((ctx->img_width == 0) || (ctx->img_height == 0)) {
		mfc_err_dev("new_img_width = %d, new_img_height = %d\n",
			ctx->img_width, ctx->img_height);
		return 0;
	}

	old_img_width = ctx->old_img_width;
	old_img_height = ctx->old_img_height;

	new_img_width = ctx->img_width;
	new_img_height = ctx->img_height;

	if ((old_img_width == new_img_width) && (old_img_height == new_img_height)) {
		mfc_err_dev("Resolution is not changed. new_img_width = %d, new_img_height = %d\n",
			new_img_width, new_img_height);
		return 0;
	}

	mfc_info_ctx("Resolution Change : (%d x %d) -> (%d x %d)\n",
		old_img_width, old_img_height, new_img_width, new_img_height);

	if ((old_img_width == new_img_height) && (old_img_height == new_img_width)) {
		reg = MFC_READL(S5P_FIMV_E_PARAM_CHANGE);
		reg &= ~(0x1 << 6);
		reg |= (0x1 << 6); /* resolution swap */
		MFC_WRITEL(reg, S5P_FIMV_E_PARAM_CHANGE);
		ret = 1;
	} else {
		reg = MFC_READL(S5P_FIMV_E_PARAM_CHANGE);
		reg &= ~(0x3 << 7);
		/* For now, FW does not care S5P_FIMV_E_PARAM_CHANGE is 1 or 2.
		 * It cares S5P_FIMV_E_PARAM_CHANGE is NOT zero.
		 */
		if ((old_img_width*old_img_height) < (new_img_width*new_img_height)) {
			reg |= (0x1 << 7); /* resolution increased */
			mfc_info_ctx("Resolution Increased\n");
		} else {
			reg |= (0x2 << 7); /* resolution decreased */
			mfc_info_ctx("Resolution Decreased\n");
		}
		MFC_WRITEL(reg, S5P_FIMV_E_PARAM_CHANGE);

		/** set cropped width */
		MFC_WRITEL(ctx->img_width, S5P_FIMV_E_CROPPED_FRAME_WIDTH);
		/** set cropped height */
		MFC_WRITEL(ctx->img_height, S5P_FIMV_E_CROPPED_FRAME_HEIGHT);

		/* bit rate */
		if (p->rc_frame)
			MFC_WRITEL(p->rc_bitrate, S5P_FIMV_E_RC_BIT_RATE);
		else
			MFC_WRITEL(1, S5P_FIMV_E_RC_BIT_RATE);
		ret = 2;
	}

	/** set new stride */
	s5p_mfc_set_enc_stride(ctx);

	/** set cropped offset */
	MFC_WRITEL(0x0, S5P_FIMV_E_FRAME_CROP_OFFSET);

	return ret;
}

/* Encode a single frame */
int s5p_mfc_encode_one_frame(struct s5p_mfc_ctx *ctx, int last_frame)
{
	struct s5p_mfc_dev *dev = ctx->dev;

	mfc_debug(2, "++\n");

	if (IS_H264_ENC(ctx))
		mfc_h264_set_aso_slice_order(ctx);

	s5p_mfc_set_slice_mode(ctx);

	if (ctx->enc_drc_flag) {
		ctx->enc_res_change = mfc_start_change_resol_enc(ctx);
		ctx->enc_drc_flag = 0;
	}

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	/* Issue different commands to instance basing on whether it
	 * is the last frame or not. */
	switch (last_frame) {
	case 0:
		s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_NAL_START);
		break;
	case 1:
		s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_LAST_FRAME);
		break;
	}

	mfc_debug(2, "--\n");

	return 0;
}
