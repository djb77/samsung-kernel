/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_buf.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/smc.h>
#include <linux/firmware.h>
#include <trace/events/mfc.h>

#include "s5p_mfc_buf.h"

#include "s5p_mfc_mem.h"

static int mfc_alloc_common_context(struct s5p_mfc_dev *dev,
					enum mfc_buf_usage_type buf_type)
{
	struct s5p_mfc_special_buf *ctx_buf;
	int firmware_size;
	unsigned long fw_daddr;

	mfc_debug_enter();
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	ctx_buf = &dev->common_ctx_buf;
	fw_daddr = dev->fw_buf.daddr;

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (buf_type == MFCBUF_DRM) {
		ctx_buf = &dev->drm_common_ctx_buf;
		fw_daddr = dev->drm_fw_buf.daddr;
	}
#endif

	firmware_size = dev->variant->buf_size->firmware_code;

	ctx_buf->handle = NULL;
	ctx_buf->vaddr = NULL;
	ctx_buf->daddr = fw_daddr + firmware_size;

	mfc_debug_leave();

	return 0;
}

/* Wrapper : allocate context buffers for SYS_INIT */
int s5p_mfc_alloc_common_context(struct s5p_mfc_dev *dev)
{
	int ret = 0;

	ret = mfc_alloc_common_context(dev, MFCBUF_NORMAL);
	if (ret)
		return ret;
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (dev->fw.drm_status) {
		ret = mfc_alloc_common_context(dev, MFCBUF_DRM);
		if (ret)
			return ret;
	}
#endif

	return ret;
}

/* Release context buffers for SYS_INIT */
static void mfc_release_common_context(struct s5p_mfc_dev *dev,
					enum mfc_buf_usage_type buf_type)
{
	struct s5p_mfc_special_buf *ctx_buf;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	ctx_buf = &dev->common_ctx_buf;
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (buf_type == MFCBUF_DRM)
		ctx_buf = &dev->drm_common_ctx_buf;
#endif

	ctx_buf->handle = NULL;
	ctx_buf->vaddr = NULL;
	ctx_buf->daddr = 0;
}

/* Release context buffers for SYS_INIT */
void s5p_mfc_release_common_context(struct s5p_mfc_dev *dev)
{
	mfc_release_common_context(dev, MFCBUF_NORMAL);

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	mfc_release_common_context(dev, MFCBUF_DRM);
#endif
}

/* Allocate memory for instance data buffer */
int s5p_mfc_alloc_instance_context(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf_size_v6 *buf_size;

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
	buf_size = dev->variant->buf_size->buf;

	switch (ctx->codec_mode) {
	case S5P_FIMV_CODEC_H264_DEC:
	case S5P_FIMV_CODEC_H264_MVC_DEC:
	case S5P_FIMV_CODEC_HEVC_DEC:
	case S5P_FIMV_CODEC_BPG_DEC:
		ctx->instance_ctx_buf.size = buf_size->h264_dec_ctx;
		break;
	case S5P_FIMV_CODEC_MPEG4_DEC:
	case S5P_FIMV_CODEC_H263_DEC:
	case S5P_FIMV_CODEC_VC1_RCV_DEC:
	case S5P_FIMV_CODEC_VC1_DEC:
	case S5P_FIMV_CODEC_MPEG2_DEC:
	case S5P_FIMV_CODEC_VP8_DEC:
	case S5P_FIMV_CODEC_VP9_DEC:
	case S5P_FIMV_CODEC_FIMV1_DEC:
	case S5P_FIMV_CODEC_FIMV2_DEC:
	case S5P_FIMV_CODEC_FIMV3_DEC:
	case S5P_FIMV_CODEC_FIMV4_DEC:
		ctx->instance_ctx_buf.size = buf_size->other_dec_ctx;
		break;
	case S5P_FIMV_CODEC_H264_ENC:
		ctx->instance_ctx_buf.size = buf_size->h264_enc_ctx;
		break;
	case S5P_FIMV_CODEC_HEVC_ENC:
	case S5P_FIMV_CODEC_BPG_ENC:
		ctx->instance_ctx_buf.size = buf_size->hevc_enc_ctx;
		break;
	case S5P_FIMV_CODEC_MPEG4_ENC:
	case S5P_FIMV_CODEC_H263_ENC:
	case S5P_FIMV_CODEC_VP8_ENC:
	case S5P_FIMV_CODEC_VP9_ENC:
		ctx->instance_ctx_buf.size = buf_size->other_enc_ctx;
		break;
	default:
		ctx->instance_ctx_buf.size = 0;
		mfc_err_ctx("Codec type(%d) should be checked!\n", ctx->codec_mode);
		return -ENOMEM;
	}

	if (ctx->is_drm)
		ctx->instance_ctx_buf.buftype = MFCBUF_DRM;
	else
		ctx->instance_ctx_buf.buftype = MFCBUF_NORMAL;

	if (s5p_mfc_mem_ion_alloc(dev, &ctx->instance_ctx_buf)) {
		mfc_err_ctx("Allocating context buffer failed\n");
		return -ENOMEM;
	}

	ctx->instance_ctx_buf.vaddr = s5p_mfc_mem_get_vaddr(dev, &ctx->instance_ctx_buf);

	if (!ctx->instance_ctx_buf.vaddr) {
		mfc_err_dev("failed to get instance ctx buffer vaddr\n");
		s5p_mfc_mem_ion_free(dev, &ctx->instance_ctx_buf);
		return -ENOMEM;
	}

	mfc_debug(2, "Instance buf alloc, ctx: %d, size: %ld, addr: 0x%08llx\n",
			ctx->num, ctx->instance_ctx_buf.size, ctx->instance_ctx_buf.daddr);

	return 0;
}

/* Release instance buffer */
void s5p_mfc_release_instance_context(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;

	mfc_debug_enter();
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	s5p_mfc_mem_ion_free(dev, &ctx->instance_ctx_buf);
	mfc_debug(2, "Release the instance ctx buffer\n");

	mfc_debug_leave();
}

static void mfc_calc_dec_codec_buffer_size(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dec *dec;
	int i;

	dec = ctx->dec_priv;

	/* Codecs have different memory requirements */
	switch (ctx->codec_mode) {
	case S5P_FIMV_CODEC_H264_DEC:
	case S5P_FIMV_CODEC_H264_MVC_DEC:
		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size =
			ctx->scratch_buf_size +
			(dec->mv_count * ctx->mv_size);
		break;
	case S5P_FIMV_CODEC_MPEG4_DEC:
	case S5P_FIMV_CODEC_FIMV1_DEC:
	case S5P_FIMV_CODEC_FIMV2_DEC:
	case S5P_FIMV_CODEC_FIMV3_DEC:
	case S5P_FIMV_CODEC_FIMV4_DEC:
		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		if (dec->loop_filter_mpeg4) {
			ctx->loopfilter_luma_size = ALIGN(ctx->raw_buf.plane_size[0], 256);
			ctx->loopfilter_chroma_size = ALIGN(ctx->raw_buf.plane_size[1] +
							ctx->raw_buf.plane_size[2], 256);
			ctx->codec_buf.size = ctx->scratch_buf_size +
				(NUM_MPEG4_LF_BUF * (ctx->loopfilter_luma_size +
						     ctx->loopfilter_chroma_size));
		} else {
			ctx->codec_buf.size = ctx->scratch_buf_size;
		}
		break;
	case S5P_FIMV_CODEC_VC1_RCV_DEC:
	case S5P_FIMV_CODEC_VC1_DEC:
		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size = ctx->scratch_buf_size;
		break;
	case S5P_FIMV_CODEC_MPEG2_DEC:
		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size = ctx->scratch_buf_size;
		break;
	case S5P_FIMV_CODEC_H263_DEC:
		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size = ctx->scratch_buf_size;
		break;
	case S5P_FIMV_CODEC_VP8_DEC:
		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size = ctx->scratch_buf_size;
		break;
	case S5P_FIMV_CODEC_VP9_DEC:
		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size =
			ctx->scratch_buf_size +
			DEC_STATIC_BUFFER_SIZE;
		break;
	case S5P_FIMV_CODEC_HEVC_DEC:
	case S5P_FIMV_CODEC_BPG_DEC:
		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size =
			ctx->scratch_buf_size +
			(dec->mv_count * ctx->mv_size);
		break;
	default:
		ctx->codec_buf.size = 0;
		mfc_err_ctx("invalid codec type: %d\n", ctx->codec_mode);
		break;
	}

	for (i = 0; i < ctx->raw_buf.num_planes; i++)
		mfc_debug(2, "Plane[%d] size:%d\n",
				i, ctx->raw_buf.plane_size[i]);
	mfc_debug(2, "MV size: %zu, Totals bufs: %d\n",
			ctx->mv_size, dec->total_dpb_count);
}

static void mfc_calc_enc_codec_buffer_size(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_enc *enc;
	unsigned int mb_width, mb_height;
	unsigned int lcu_width = 0, lcu_height = 0;

	enc = ctx->enc_priv;
	enc->tmv_buffer_size = 0;

	mb_width = WIDTH_MB(ctx->img_width);
	mb_height = HEIGHT_MB(ctx->img_height);

	lcu_width = ENC_LCU_WIDTH(ctx->img_width);
	lcu_height = ENC_LCU_HEIGHT(ctx->img_height);

	/* default recon buffer size, it can be changed in case of 422, 10bit */
	enc->luma_dpb_size =
		ALIGN(ENC_LUMA_DPB_SIZE(ctx->img_width, ctx->img_height), 64);
	enc->chroma_dpb_size =
		ALIGN(ENC_CHROMA_DPB_SIZE(ctx->img_width, ctx->img_height), 64);
	mfc_debug(2, "recon luma size: %zu chroma size: %zu\n",
			enc->luma_dpb_size, enc->chroma_dpb_size);

	/* Codecs have different memory requirements */
	switch (ctx->codec_mode) {
	case S5P_FIMV_CODEC_H264_ENC:
		enc->me_buffer_size =
			ALIGN(ENC_V100_H264_ME_SIZE(mb_width, mb_height), 256);

		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size =
			ctx->scratch_buf_size + enc->tmv_buffer_size +
			(ctx->dpb_count * (enc->luma_dpb_size +
			enc->chroma_dpb_size + enc->me_buffer_size));

		ctx->scratch_buf_size = max(ctx->scratch_buf_size, ctx->min_scratch_buf_size);
		ctx->min_scratch_buf_size = 0;
		break;
	case S5P_FIMV_CODEC_MPEG4_ENC:
	case S5P_FIMV_CODEC_H263_ENC:
		enc->me_buffer_size =
			ALIGN(ENC_V100_MPEG4_ME_SIZE(mb_width, mb_height), 256);

		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size =
			ctx->scratch_buf_size + enc->tmv_buffer_size +
			(ctx->dpb_count * (enc->luma_dpb_size +
			enc->chroma_dpb_size + enc->me_buffer_size));
		break;
	case S5P_FIMV_CODEC_VP8_ENC:
		enc->me_buffer_size =
			ALIGN(ENC_V100_VP8_ME_SIZE(mb_width, mb_height), 256);

		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size =
			ctx->scratch_buf_size + enc->tmv_buffer_size +
			(ctx->dpb_count * (enc->luma_dpb_size +
			enc->chroma_dpb_size + enc->me_buffer_size));
		break;
	case S5P_FIMV_CODEC_VP9_ENC:
		if (ctx->is_10bit) {
			enc->luma_dpb_size =
				ALIGN(ENC_VP9_LUMA_DPB_10B_SIZE(ctx->img_width, ctx->img_height), 64);
			enc->chroma_dpb_size =
				ALIGN(ENC_VP9_CHROMA_DPB_10B_SIZE(ctx->img_width, ctx->img_height), 64);
			mfc_debug(2, "VP9 10bit recon luma size: %zu chroma size: %zu\n",
					enc->luma_dpb_size, enc->chroma_dpb_size);
		}
		enc->me_buffer_size =
			ALIGN(ENC_V100_VP9_ME_SIZE(lcu_width, lcu_height), 256);

		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size =
			ctx->scratch_buf_size + enc->tmv_buffer_size +
			(ctx->dpb_count * (enc->luma_dpb_size +
					   enc->chroma_dpb_size + enc->me_buffer_size));
		break;
	case S5P_FIMV_CODEC_HEVC_ENC:
	case S5P_FIMV_CODEC_BPG_ENC:
		if (ctx->is_10bit || ctx->is_422format) {
			enc->luma_dpb_size =
				ALIGN(ENC_HEVC_LUMA_DPB_10B_SIZE(ctx->img_width, ctx->img_height), 64);
			enc->chroma_dpb_size =
				ALIGN(ENC_HEVC_CHROMA_DPB_10B_SIZE(ctx->img_width, ctx->img_height), 64);
			mfc_debug(2, "HEVC 10bit or 422 recon luma size: %zu chroma size: %zu\n",
					enc->luma_dpb_size, enc->chroma_dpb_size);
		}
		enc->me_buffer_size =
			ALIGN(ENC_V100_HEVC_ME_SIZE(lcu_width, lcu_height), 256);

		ctx->scratch_buf_size = ALIGN(ctx->scratch_buf_size, 256);
		ctx->codec_buf.size =
			ctx->scratch_buf_size + enc->tmv_buffer_size +
			(ctx->dpb_count * (enc->luma_dpb_size +
					   enc->chroma_dpb_size + enc->me_buffer_size));
		break;
	default:
		ctx->codec_buf.size = 0;
		mfc_err_ctx("invalid codec type: %d\n", ctx->codec_mode);
		break;
	}
}

/* Allocate codec buffers */
int s5p_mfc_alloc_codec_buffers(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;

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

	if (ctx->type == MFCINST_DECODER) {
		mfc_calc_dec_codec_buffer_size(ctx);
	} else if (ctx->type == MFCINST_ENCODER) {
		mfc_calc_enc_codec_buffer_size(ctx);
	} else {
		mfc_err_ctx("invalid type: %d\n", ctx->type);
		return -EINVAL;
	}

	if (ctx->is_drm)
		ctx->codec_buf.buftype = MFCBUF_DRM;
	else
		ctx->codec_buf.buftype = MFCBUF_NORMAL;

	if (ctx->codec_buf.size > 0) {
		if (s5p_mfc_mem_ion_alloc(dev, &ctx->codec_buf)) {
			mfc_err_ctx("Allocating codec buffer failed\n");
			return -ENOMEM;
		}

		ctx->codec_buf.vaddr = s5p_mfc_mem_get_vaddr(dev, &ctx->codec_buf);
		if (!ctx->codec_buf.vaddr) {
			mfc_err_dev("failed to get codec buffer vaddr\n");
			s5p_mfc_mem_ion_free(dev, &ctx->codec_buf);
			return -ENOMEM;
		}
		ctx->codec_buffer_allocated = 1;
	} else if (ctx->codec_mode == S5P_FIMV_CODEC_MPEG2_DEC) {
		ctx->codec_buffer_allocated = 1;
	}

	mfc_debug(2, "Codec buf alloc, ctx: %d, size: %ld, addr: 0x%08llx\n",
			ctx->num, ctx->codec_buf.size, ctx->codec_buf.daddr);

	return 0;
}

/* Release buffers allocated for codec */
void s5p_mfc_release_codec_buffers(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	s5p_mfc_mem_ion_free(dev, &ctx->codec_buf);
	mfc_debug(2, "Release the codec buffer\n");
}

/* Allocation buffer of debug infor memory for FW debugging */
int s5p_mfc_alloc_dbg_info_buffer(struct s5p_mfc_dev *dev)
{
	struct s5p_mfc_buf_size_v6 *buf_size = dev->variant->buf_size->buf;

	mfc_debug(2, "Allocate a debug-info buffer.\n");

	dev->dbg_info_buf.buftype = MFCBUF_NORMAL;
	dev->dbg_info_buf.size = buf_size->dbg_info_buf;
	if (s5p_mfc_mem_ion_alloc(dev, &dev->dbg_info_buf)) {
		mfc_err_dev("Allocating debug info buffer failed\n");
		return -ENOMEM;
	}
	mfc_debug(2, "dev->dbg_info_buf.daddr = 0x%08llx\n",
			dev->dbg_info_buf.daddr);

	dev->dbg_info_buf.vaddr = s5p_mfc_mem_get_vaddr(dev, &dev->dbg_info_buf);
	if (!dev->dbg_info_buf.vaddr) {
		mfc_err_dev("failed to get debug info buffer vaddr\n");
		s5p_mfc_mem_ion_free(dev, &dev->dbg_info_buf);
		return -ENOMEM;
	}
	mfc_debug(2, "dev->dbg_info_buf.vaddr = 0x%p\n",
					dev->dbg_info_buf.vaddr);

	return 0;
}

/* Release buffer of debug infor memory for FW debugging */
int s5p_mfc_release_dbg_info_buffer(struct s5p_mfc_dev *dev)
{
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	if (!dev->dbg_info_buf.handle) {
		mfc_debug(2, "debug info buffer is already freed\n");
		return 0;
	}

	s5p_mfc_mem_ion_free(dev, &dev->dbg_info_buf);
	mfc_debug(2, "Release the debug-info buffer\n");

	return 0;
}

/* Allocation buffer of ROI macroblock information */
static int mfc_alloc_enc_roi_buffer(struct s5p_mfc_ctx *ctx, struct s5p_mfc_special_buf *roi_buf)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_buf_size_v6 *buf_size = dev->variant->buf_size->buf;

	roi_buf->buftype = MFCBUF_NORMAL;
	roi_buf->size = buf_size->shared_buf;
	if (s5p_mfc_mem_ion_alloc(dev, roi_buf)) {
		mfc_err_ctx("Allocating ROI buffer failed\n");
		return -ENOMEM;
	}
	mfc_debug(2, "roi_buf.daddr = 0x%08llx\n", roi_buf->daddr);

	roi_buf->vaddr = s5p_mfc_mem_get_vaddr(dev, roi_buf);
	if (!roi_buf->vaddr) {
		mfc_err_dev("failed to get ROI buffer vaddr\n");
		s5p_mfc_mem_ion_free(dev, roi_buf);
		return -ENOMEM;
	}

	memset(roi_buf->vaddr, 0, buf_size->shared_buf);
	s5p_mfc_mem_clean(dev, roi_buf, 0, roi_buf->size);

	return 0;
}

/* Wrapper : allocation ROI buffers */
int s5p_mfc_alloc_enc_roi_buffer(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	int i;

	for (i = 0; i < MFC_MAX_EXTRA_BUF; i++) {
		if (mfc_alloc_enc_roi_buffer(ctx, &enc->roi_buf[i]) < 0) {
			mfc_err_dev("Remapping shared mem buffer failed.\n");
			return -ENOMEM;
		}
	}

	return 0;
}

/* Release buffer of ROI macroblock information */
void s5p_mfc_release_enc_roi_buffer(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	int i;

	for (i = 0; i < MFC_MAX_EXTRA_BUF; i++)
		if (enc->roi_buf[i].handle)
			s5p_mfc_mem_ion_free(ctx->dev, &enc->roi_buf[i]);
}

int s5p_mfc_otf_alloc_stream_buf(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_debug *debug = &handle->otf_debug;
	struct s5p_mfc_special_buf *buf;
	struct s5p_mfc_raw_info *raw = &ctx->raw_buf;
	int i;

	mfc_debug_enter();

	for (i = 0; i < OTF_MAX_BUF; i++) {
		buf = &debug->stream_buf[i];
		buf->buftype = MFCBUF_NORMAL;
		buf->size = raw->total_plane_size;
		if (s5p_mfc_mem_ion_alloc(dev, buf)) {
			mfc_err_ctx("OTF: Allocating stream buffer failed\n");
			return -EINVAL;
		}
		buf->vaddr = s5p_mfc_mem_get_vaddr(dev, buf);
		if (!buf->vaddr) {
			mfc_err_dev("OTF: failed to get stream buffer vaddr\n");
			s5p_mfc_mem_ion_free(dev, buf);
			return -EINVAL;
		}
		memset(buf->vaddr, 0, raw->total_plane_size);
		s5p_mfc_mem_clean(dev, buf, 0, buf->size);
	}

	mfc_debug_leave();

	return 0;
}

void s5p_mfc_otf_release_stream_buf(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_debug *debug = &handle->otf_debug;
	struct s5p_mfc_special_buf *buf;
	int i;

	mfc_debug_enter();

	for (i = 0; i < OTF_MAX_BUF; i++) {
		buf = &debug->stream_buf[i];
		if (buf->handle)
			s5p_mfc_mem_ion_free(dev, buf);
	}

	mfc_debug_leave();
}

/* Allocate firmware */
int s5p_mfc_alloc_firmware(struct s5p_mfc_dev *dev)
{
	unsigned int base_align;
	size_t firmware_size;
	struct s5p_mfc_buf_size_v6 *buf_size;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	buf_size = dev->variant->buf_size->buf;
	base_align = dev->variant->buf_align->mfc_base_align;
	firmware_size = dev->variant->buf_size->firmware_code;
	dev->fw.size = firmware_size + buf_size->dev_ctx;

	if (dev->fw_buf.handle)
		return 0;

	mfc_debug(2, "Allocating memory for firmware.\n");
	trace_mfc_loadfw_start(dev->fw.size, firmware_size);

	dev->fw_buf.buftype = MFCBUF_NORMAL_FW;
	dev->fw_buf.size = dev->fw.size;
	if (s5p_mfc_mem_ion_alloc(dev, &dev->fw_buf)) {
		mfc_err_dev("Allocating normal firmware buffer failed\n");
		return -ENOMEM;
	}

	dev->fw_buf.vaddr = s5p_mfc_mem_get_vaddr(dev, &dev->fw_buf);
	if (!dev->fw_buf.vaddr) {
		mfc_err_dev("failed to get normal firmware buffer vaddr\n");
		s5p_mfc_mem_ion_free(dev, &dev->fw_buf);
		return -EIO;
	}
	mfc_debug(2, "FW normal: 0x%08llx (vaddr: 0x%p), size: %08zu\n",
			dev->fw_buf.daddr, dev->fw_buf.vaddr,
			dev->fw_buf.size);

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	dev->drm_fw_buf.buftype = MFCBUF_DRM_FW;
	dev->drm_fw_buf.size = dev->fw.size;
	if (s5p_mfc_mem_ion_alloc(dev, &dev->drm_fw_buf)) {
		mfc_err_dev("Allocating DRM firmware buffer failed\n");
		return -ENOMEM;
	}

	dev->drm_fw_buf.vaddr = s5p_mfc_mem_get_vaddr(dev, &dev->drm_fw_buf);
	if (!dev->drm_fw_buf.vaddr) {
		mfc_err_dev("failed to get DRM firmware buffer vaddr\n");
		s5p_mfc_mem_ion_free(dev, &dev->fw_buf);
		s5p_mfc_mem_ion_free(dev, &dev->drm_fw_buf);
		return -EIO;
	}

	mfc_debug(2, "FW DRM: 0x%08llx (vaddr: 0x%p), size: %08zu\n",
			dev->drm_fw_buf.daddr, dev->drm_fw_buf.vaddr,
			dev->drm_fw_buf.size);
#endif

	mfc_debug_leave();

	return 0;
}

/* Load firmware to MFC */
int s5p_mfc_load_firmware(struct s5p_mfc_dev *dev)
{
	struct firmware *fw_blob;
	size_t firmware_size;
	int err;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	firmware_size = dev->variant->buf_size->firmware_code;

	/* Firmare has to be present as a separate file or compiled
	 * into kernel. */
	mfc_debug_enter();
	mfc_debug(2, "Requesting fw\n");
	err = request_firmware((const struct firmware **)&fw_blob,
					MFC_FW_NAME, dev->v4l2_dev.dev);

	if (err != 0) {
		mfc_err_dev("Firmware is not present in the /lib/firmware directory nor compiled in kernel.\n");
		return -EINVAL;
	}

	mfc_debug(2, "Ret of request_firmware: %d Size: %zu\n", err, fw_blob->size);

	if (fw_blob->size > firmware_size) {
		mfc_err_dev("MFC firmware is too big to be loaded.\n");
		release_firmware(fw_blob);
		return -ENOMEM;
	}

	if (dev->fw_buf.handle == NULL || dev->fw_buf.daddr == 0) {
		mfc_err_dev("MFC firmware is not allocated or was not mapped correctly.\n");
		release_firmware(fw_blob);
		return -EINVAL;
	}

	memcpy(dev->fw_buf.vaddr, fw_blob->data, fw_blob->size);
	s5p_mfc_mem_clean(dev, &dev->fw_buf, 0, fw_blob->size);
	s5p_mfc_mem_invalidate(dev, &dev->fw_buf, 0, fw_blob->size);
	if (dev->drm_fw_buf.vaddr) {
		memcpy(dev->drm_fw_buf.vaddr, fw_blob->data, fw_blob->size);
		mfc_debug(2, "copy firmware to secure region\n");
		s5p_mfc_mem_clean(dev, &dev->drm_fw_buf, 0, fw_blob->size);
		s5p_mfc_mem_invalidate(dev, &dev->drm_fw_buf, 0, fw_blob->size);
	}
	release_firmware(fw_blob);
	trace_mfc_loadfw_end(dev->fw.size, firmware_size);
	mfc_debug_leave();
	return 0;
}

/* Release firmware memory */
int s5p_mfc_release_firmware(struct s5p_mfc_dev *dev)
{
	/* Before calling this function one has to make sure
	 * that MFC is no longer processing */
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	if (!dev->fw_buf.handle) {
		mfc_err_dev("firmware memory is already freed\n");
		return -EINVAL;
	}

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	s5p_mfc_mem_ion_free(dev, &dev->drm_fw_buf);
#endif

	s5p_mfc_mem_ion_free(dev, &dev->fw_buf);

	return 0;
}
