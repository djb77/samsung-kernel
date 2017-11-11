/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_opr.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "s5p_mfc_opr.h"

#include "s5p_mfc_inst.h"
#include "s5p_mfc_reg.h"

#include "s5p_mfc_queue.h"
#include "s5p_mfc_utils.h"
#include "s5p_mfc_mem.h"

int s5p_mfc_run_dec_init(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *src_mb;
	struct s5p_mfc_dec *dec = NULL;

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
	/* Initializing decoding - parsing header */

	/* Get the next source buffer */
	src_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
	if (!src_mb) {
		mfc_err_dev("no src buffers.\n");
		return -EAGAIN;
	}

	mfc_debug(2, "Preparing to init decoding.\n");
	mfc_debug(2, "Header size: %d, (offset: %lu)\n",
		src_mb->vb.vb2_buf.planes[0].bytesused, dec->consumed);

	if (dec->consumed) {
		s5p_mfc_set_dec_stream_buffer(ctx, src_mb, dec->consumed, dec->remained_size);
	} else {
		/* decoder src buffer CFW PROT */
		if (ctx->is_drm) {
			int index = src_mb->vb.vb2_buf.index;

			if (!test_bit(index, &ctx->stream_protect_flag)) {
				if (s5p_mfc_stream_buf_prot(ctx, src_mb, true))
					mfc_err_ctx("failed to CFW_PROT\n");
				else
					set_bit(index, &ctx->stream_protect_flag);
			}
			mfc_debug(2, "[%d] dec src buf prot_flag: %#lx\n",
					index, ctx->stream_protect_flag);
		}

		s5p_mfc_set_dec_stream_buffer(ctx, src_mb,
			0, src_mb->vb.vb2_buf.planes[0].bytesused);
	}

	mfc_debug(2, "Header addr: 0x%08llx\n", s5p_mfc_mem_get_daddr_vb(&src_mb->vb.vb2_buf, 0));
	s5p_mfc_clean_ctx_int_flags(ctx);
	s5p_mfc_init_decode(ctx);

	return 0;
}

int s5p_mfc_run_dec_frame(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *src_mb, *dst_mb;
	struct s5p_mfc_dec *dec;
	int last_frame = 0;
	unsigned int index;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}
	dec = ctx->dec_priv;
	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	if (s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0) &&
			s5p_mfc_is_queue_count_smaller(&ctx->buf_queue_lock,
				&dec->ref_buf_queue, (ctx->dpb_count + 5))) {
		return -EAGAIN;
	}

	/* Get the next source buffer */
	src_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_SET_USED);
	if (!src_mb) {
		mfc_debug(2, "no src buffers.\n");
		return -EAGAIN;
	}

	/* decoder src buffer CFW PROT */
	if (ctx->is_drm) {
		if (!dec->consumed) {
			index = src_mb->vb.vb2_buf.index;
			if (!test_bit(index, &ctx->stream_protect_flag)) {
				if (s5p_mfc_stream_buf_prot(ctx, src_mb, true))
					mfc_err_ctx("failed to CFW_PROT\n");
				else
					set_bit(index, &ctx->stream_protect_flag);
			}
			mfc_debug(2, "[%d] dec src buf prot_flag: %#lx\n",
					index, ctx->stream_protect_flag);
		}
	}

	if (src_mb->vb.reserved2 & FLAG_EMPTY_DATA)
		src_mb->vb.vb2_buf.planes[0].bytesused = 0;

	if (dec->consumed)
		s5p_mfc_set_dec_stream_buffer(ctx, src_mb, dec->consumed, dec->remained_size);
	else
		s5p_mfc_set_dec_stream_buffer(ctx, src_mb, 0, src_mb->vb.vb2_buf.planes[0].bytesused);

	/* Try to use the non-referenced DPB on dst-queue */
	dst_mb = s5p_mfc_search_for_dpb(ctx, dec->dynamic_used);
	if (!dst_mb) {
		mfc_debug(2, "no dst buffers.\n");
		return -EAGAIN;
	}

	index = src_mb->vb.vb2_buf.index;
	if (call_cop(ctx, set_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
		mfc_err_ctx("failed in set_buf_ctrls_val\n");

	s5p_mfc_set_dynamic_dpb(ctx, dst_mb);

	s5p_mfc_clean_ctx_int_flags(ctx);

	if (src_mb->vb.reserved2 & FLAG_LAST_FRAME) {
		last_frame = 1;
		mfc_debug(2, "Setting ctx->state to FINISHING\n");
		s5p_mfc_change_state(ctx, MFCINST_FINISHING);
	}
	s5p_mfc_decode_one_frame(ctx, last_frame);

	return 0;
}

int s5p_mfc_run_dec_last_frames(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *src_mb, *dst_mb;
	struct s5p_mfc_dec *dec;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no decoder context to run\n");
		return -EINVAL;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	if (s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0)) {
		mfc_debug(2, "no dst buffer\n");
		return -EAGAIN;
	}

	/* Get the next source buffer */
	src_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_SET_USED);

	/* Frames are being decoded */
	if (!src_mb) {
		mfc_debug(2, "no src buffers.\n");
		s5p_mfc_set_dec_stream_buffer(ctx, 0, 0, 0);
	} else {
		if (dec->consumed) {
			s5p_mfc_set_dec_stream_buffer(ctx, src_mb, dec->consumed, dec->remained_size);
		} else {
			/* decoder src buffer CFW PROT */
			if (ctx->is_drm) {
				int index = src_mb->vb.vb2_buf.index;

				if (!test_bit(index, &ctx->stream_protect_flag)) {
					if (s5p_mfc_stream_buf_prot(ctx, src_mb, true))
						mfc_err_ctx("failed to CFW_PROT\n");
					else
						set_bit(index, &ctx->stream_protect_flag);
				}
				mfc_debug(2, "[%d] dec src buf prot_flag: %#lx\n",
						index, ctx->stream_protect_flag);
			}

			s5p_mfc_set_dec_stream_buffer(ctx, src_mb, 0, 0);
		}
	}

	/* Try to use the non-referenced DPB on dst-queue */
	dst_mb = s5p_mfc_search_for_dpb(ctx, dec->dynamic_used);
	if (!dst_mb) {
		mfc_debug(2, "no dst buffers.\n");
		return -EAGAIN;
	}

	s5p_mfc_set_dynamic_dpb(ctx, dst_mb);

	s5p_mfc_clean_ctx_int_flags(ctx);
	s5p_mfc_decode_one_frame(ctx, 1);

	return 0;
}

int s5p_mfc_run_enc_init(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_buf *dst_mb;
	int ret;

	dst_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
	if (!dst_mb) {
		mfc_debug(2, "no dst buffers.\n");
		return -EAGAIN;
	}

	/* encoder dst buffer CFW PROT */
	if (ctx->is_drm) {
		int index = dst_mb->vb.vb2_buf.index;

		if (!test_bit(index, &ctx->stream_protect_flag)) {
			if (s5p_mfc_stream_buf_prot(ctx, dst_mb, true))
				mfc_err_ctx("failed to CFW_PROT\n");
			else
				set_bit(index, &ctx->stream_protect_flag);
		}
		mfc_debug(2, "[%d] enc dst buf prot_flag: %#lx\n",
				index, ctx->stream_protect_flag);
	}
	s5p_mfc_set_enc_stream_buffer(ctx, dst_mb);

	s5p_mfc_set_enc_stride(ctx);

	mfc_debug(2, "Header addr: 0x%08llx\n", s5p_mfc_mem_get_daddr_vb(&dst_mb->vb.vb2_buf, 0));
	s5p_mfc_clean_ctx_int_flags(ctx);

	ret = s5p_mfc_init_encode(ctx);
	return ret;
}

int s5p_mfc_run_enc_frame(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_buf *dst_mb;
	struct s5p_mfc_buf *src_mb;
	struct s5p_mfc_raw_info *raw;
	dma_addr_t src_addr[3] = { 0, 0, 0 };
	unsigned int index, i;
	int last_frame = 0;

	raw = &ctx->raw_buf;

	/* Get the next source buffer */
	src_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_SET_USED);
	if (!src_mb) {
		mfc_debug(2, "no src buffers.\n");
		return -EAGAIN;
	}

	if (src_mb->vb.reserved2 & FLAG_LAST_FRAME) {
		last_frame = 1;
		mfc_debug(2, "Setting ctx->state to FINISHING\n");
		s5p_mfc_change_state(ctx, MFCINST_FINISHING);
	}

	for (i = 0; i < raw->num_planes; i++) {
		src_addr[i] = src_mb->planes.raw[i];
		mfc_debug(2, "enc src[%d] addr: 0x%08llx\n", i, src_addr[i]);
	}
	if (src_mb->planes.raw[0] != s5p_mfc_mem_get_daddr_vb(&src_mb->vb.vb2_buf, 0))
		mfc_err_ctx("enc src yaddr: 0x%08llx != vb2 yaddr: 0x%08llx\n",
				src_mb->planes.raw[i],
				s5p_mfc_mem_get_daddr_vb(&src_mb->vb.vb2_buf, i));

	index = src_mb->vb.vb2_buf.index;

	/* encoder src buffer CFW PROT */
	if (ctx->is_drm) {
		if (!test_bit(index, &ctx->raw_protect_flag)) {
			if (s5p_mfc_raw_buf_prot(ctx, src_mb, true))
				mfc_err_ctx("failed to CFW_PROT\n");
			else
				set_bit(index, &ctx->raw_protect_flag);
		}
		mfc_debug(2, "[%d] enc src buf prot_flag: %#lx\n",
				index, ctx->raw_protect_flag);
	}

	s5p_mfc_set_enc_frame_buffer(ctx, &src_addr[0], raw->num_planes);

	dst_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_SET_USED);
	if (!dst_mb) {
		mfc_debug(2, "no dst buffers.\n");
		return -EAGAIN;
	}

	/* encoder dst buffer CFW PROT */
	if (ctx->is_drm) {
		i = dst_mb->vb.vb2_buf.index;
		if (!test_bit(i, &ctx->stream_protect_flag)) {
			if (s5p_mfc_stream_buf_prot(ctx, dst_mb, true))
				mfc_err_ctx("failed to CFW_PROT\n");
			else
				set_bit(i, &ctx->stream_protect_flag);
		}
		mfc_debug(2, "[%d] enc dst buf prot_flag: %#lx\n",
				i, ctx->stream_protect_flag);
	}
	mfc_debug(2, "nal start : src index from src_buf_queue:%d\n",
		src_mb->vb.vb2_buf.index);
	mfc_debug(2, "nal start : dst index from dst_buf_queue:%d\n",
		dst_mb->vb.vb2_buf.index);

	s5p_mfc_set_enc_stream_buffer(ctx, dst_mb);

	if (call_cop(ctx, set_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
		mfc_err_ctx("failed in set_buf_ctrls_val\n");

	s5p_mfc_clean_ctx_int_flags(ctx);
	s5p_mfc_encode_one_frame(ctx, last_frame);

	return 0;
}

int s5p_mfc_run_enc_last_frames(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_buf *src_mb, *dst_mb;
	struct s5p_mfc_raw_info *raw;
	dma_addr_t src_addr[3] = { 0, 0, 0 };

	raw = &ctx->raw_buf;

	/* Get the next source buffer */
	src_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
	if (!src_mb) {
		mfc_debug(2, "no src buffers.\n");
		return -EAGAIN;
	}

	dst_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_SET_USED);
	if (!dst_mb) {
		mfc_debug(2, "no dst buffers.\n");
		return -EAGAIN;
	}

	mfc_debug(2, "Set address zero for all planes\n");
	s5p_mfc_set_enc_frame_buffer(ctx, &src_addr[0], raw->num_planes);

	/* encoder dst buffer CFW PROT */
	if (ctx->is_drm) {
		int index = dst_mb->vb.vb2_buf.index;

		if (!test_bit(index, &ctx->stream_protect_flag)) {
			if (s5p_mfc_stream_buf_prot(ctx, dst_mb, true))
				mfc_err_ctx("failed to CFW_PROT\n");
			else
				set_bit(index, &ctx->stream_protect_flag);
		}
		mfc_debug(2, "[%d] enc dst buf prot_flag: %#lx\n",
				index, ctx->stream_protect_flag);
	}

	s5p_mfc_set_enc_stream_buffer(ctx, dst_mb);

	s5p_mfc_clean_ctx_int_flags(ctx);
	s5p_mfc_encode_one_frame(ctx, 1);

	return 0;
}
