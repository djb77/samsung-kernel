/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_irq.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "s5p_mfc_irq.h"

#include "s5p_mfc_hwlock.h"
#include "s5p_mfc_nal_q.h"
#include "s5p_mfc_watchdog.h"
#include "s5p_mfc_opr.h"
#include "s5p_mfc_sync.h"

#include "s5p_mfc_pm.h"
#include "s5p_mfc_reg.h"

#include "s5p_mfc_qos.h"
#include "s5p_mfc_queue.h"
#include "s5p_mfc_utils.h"
#include "s5p_mfc_buf.h"
#include "s5p_mfc_mem.h"

extern int dbg_enable;

static void mfc_handle_black_bar_info(struct s5p_mfc_dev *dev, struct s5p_mfc_ctx *ctx)
{
	struct v4l2_rect new_black_bar;
	int black_bar_info;
	struct s5p_mfc_dec *dec = ctx->dec_priv;

	black_bar_info = s5p_mfc_get_black_bar_detection();
	mfc_debug(3, "black bar type: %#x\n", black_bar_info);

	if (black_bar_info == S5P_FIMV_DISP_STATUS_BLACK_BAR) {
		new_black_bar.left = s5p_mfc_get_black_bar_pos_x();
		new_black_bar.top = s5p_mfc_get_black_bar_pos_y();
		new_black_bar.width = s5p_mfc_get_black_bar_image_w();
		new_black_bar.height = s5p_mfc_get_black_bar_image_h();
	} else if (black_bar_info == S5P_FIMV_DISP_STATUS_BLACK_SCREEN) {
		new_black_bar.left = -1;
		new_black_bar.top = -1;
		new_black_bar.width = ctx->img_width;
		new_black_bar.height = ctx->img_height;
	} else if (black_bar_info == S5P_FIMV_DISP_STATUS_NOT_DETECTED) {
		new_black_bar.left = 0;
		new_black_bar.top = 0;
		new_black_bar.width = ctx->img_width;
		new_black_bar.height = ctx->img_height;
	} else {
		mfc_err_ctx("Not supported black bar type: %#x\n", black_bar_info);
		dec->black_bar_updated = 0;
		return;
	}

	if ((new_black_bar.left == dec->black_bar.left) &&
			(new_black_bar.top == dec->black_bar.top) &&
			(new_black_bar.width == dec->black_bar.width) &&
			(new_black_bar.height == dec->black_bar.height)) {
		mfc_debug(3, "black bar info was not changed\n");
		dec->black_bar_updated = 0;
		return;
	}

	dec->black_bar = new_black_bar;
	dec->black_bar_updated = 1;
}

static void mfc_handle_frame_all_extracted(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *dst_mb;
	int index, i, is_first = 1;
	unsigned int interlace_type = 0, is_interlace = 0;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return;
	}

	mfc_debug(2, "Decided to finish\n");
	ctx->sequence++;

	while (1) {
		dst_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
		if (!dst_mb)
			break;

		mfc_debug(2, "Cleaning up buffer: %d\n",
					  dst_mb->vb.vb2_buf.index);
		if (CODEC_INTERLACED(ctx))
			is_interlace = s5p_mfc_is_interlace_picture();
		for (i = 0; i < ctx->dst_fmt->mem_planes; i++)
			vb2_set_plane_payload(&dst_mb->vb.vb2_buf, i, 0);

		dst_mb->vb.sequence = (ctx->sequence++);
		dst_mb->vb.reserved2 = 0;

		if (is_interlace) {
			interlace_type = s5p_mfc_get_interlace_type();
			if (interlace_type)
				dst_mb->vb.field = V4L2_FIELD_INTERLACED_TB;
			else
				dst_mb->vb.field = V4L2_FIELD_INTERLACED_BT;
		}
		else
			dst_mb->vb.field = V4L2_FIELD_NONE;
		clear_bit(dst_mb->vb.vb2_buf.index, &dec->available_dpb);

		index = dst_mb->vb.vb2_buf.index;
		if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->dst_ctrls[index]) < 0)
			mfc_err_ctx("failed in get_buf_ctrls_val\n");

		if (is_first) {
			call_cop(ctx, get_buf_update_val, ctx,
				&ctx->dst_ctrls[index],
				V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
				dec->stored_tag);
			is_first = 0;
		} else {
			call_cop(ctx, get_buf_update_val, ctx,
				&ctx->dst_ctrls[index],
				V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
				DEFAULT_TAG);
			call_cop(ctx, get_buf_update_val, ctx,
				&ctx->dst_ctrls[index],
				V4L2_CID_MPEG_VIDEO_H264_SEI_FP_AVAIL,
				0);
		}

		vb2_buffer_done(&dst_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

		/* decoder dst buffer CFW UNPROT */
		if (ctx->is_drm) {
			if (test_bit(index, &ctx->raw_protect_flag)) {
				if (s5p_mfc_raw_buf_prot(ctx, dst_mb, false))
					mfc_err_ctx("failed to CFW_UNPROT\n");
				else
					clear_bit(index, &ctx->raw_protect_flag);
			}
			mfc_debug(2, "[%d] dec dst buf un-prot_flag: %#lx\n",
					index, ctx->raw_protect_flag);
		}

		mfc_debug(2, "Cleaned up buffer: %d\n",
			  dst_mb->vb.vb2_buf.index);
	}

	if (ctx->state != MFCINST_ABORT && ctx->state != MFCINST_HEAD_PARSED &&
			ctx->state != MFCINST_RES_CHANGE_FLUSH)
		s5p_mfc_change_state(ctx, MFCINST_RUNNING);
	mfc_debug(2, "After cleanup\n");
}

static void mfc_handle_frame_copy_timestamp(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *ref_mb, *src_mb;
	dma_addr_t dec_y_addr;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no decoder context to run\n");
		return;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no device to run\n");
		return;
	}

	dec_y_addr = (dma_addr_t)s5p_mfc_get_dec_y_addr();

	/* Get the source buffer */
	src_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
	if (!src_mb) {
		mfc_err_dev("no src buffers.\n");
		return;
	}

	ref_mb = s5p_mfc_find_buf_vb(&ctx->buf_queue_lock,
			&ctx->ref_buf_queue, dec_y_addr);
	if (ref_mb) {
		memcpy(&ref_mb->vb.timestamp,
				&src_mb->vb.timestamp,
				sizeof(struct timeval));
	}
}

static void mfc_handle_frame_new(struct s5p_mfc_ctx *ctx, unsigned int err)
{
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *ref_mb;
	struct s5p_mfc_raw_info *raw;
	dma_addr_t dspl_y_addr;
	unsigned int index;
	unsigned int frame_type;
	unsigned int interlace_type = 0, is_interlace = 0;
	unsigned int is_video_signal_type = 0, is_colour_description = 0;
	unsigned int is_content_light = 0, is_display_colour = 0;
	int mvc_view_id;
	unsigned int dst_frame_status;
	unsigned int prev_flag, released_flag = 0;
	int i;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return;
	}

	raw = &ctx->raw_buf;
	frame_type = s5p_mfc_get_disp_frame_type();
	mvc_view_id = s5p_mfc_get_mvc_disp_view_id();

	mfc_debug(2, "frame_type : %d\n", frame_type);

	if (IS_H264_MVC_DEC(ctx)) {
		if (mvc_view_id == 0)
			ctx->sequence++;
	} else {
		ctx->sequence++;
	}

	dspl_y_addr = s5p_mfc_get_disp_y_addr();

	if (dec->immediate_display == 1) {
		dspl_y_addr = (dma_addr_t)s5p_mfc_get_dec_y_addr();
		frame_type = s5p_mfc_get_dec_frame_type();
	}

	/* If frame is same as previous then skip and do not dequeue */
	if (frame_type == S5P_FIMV_DISPLAY_FRAME_NOT_CODED) {
		if (!CODEC_NOT_CODED(ctx))
			return;
	}

	if (CODEC_INTERLACED(ctx))
		is_interlace = s5p_mfc_is_interlace_picture();

	if (FW_HAS_VIDEO_SIGNAL_TYPE(dev)) {
		is_video_signal_type = s5p_mfc_get_video_signal_type();
		is_colour_description = s5p_mfc_get_colour_description();
	}

	if (FW_HAS_SEI_INFO_FOR_HDR(dev)) {
		is_content_light = s5p_mfc_get_sei_avail_content_light();
		is_display_colour = s5p_mfc_get_sei_avail_mastering_display();
	}

	if (FW_HAS_BLACK_BAR_DETECT(dev) && dec->detect_black_bar)
		mfc_handle_black_bar_info(dev, ctx);
	else
		dec->black_bar_updated = 0;

	prev_flag = dec->dynamic_used;
	dec->dynamic_used = s5p_mfc_get_dec_used_flag();
	released_flag = prev_flag & (~dec->dynamic_used);

	mfc_debug(2, "Used flag = %08x, Released Buffer = %08x\n",
			dec->dynamic_used, released_flag);

	/* decoder dst buffer CFW UNPROT */
	s5p_mfc_unprotect_released_dpb(ctx, released_flag);

	if ((IS_VC1_RCV_DEC(ctx) &&
		s5p_mfc_get_warn(err) == S5P_FIMV_ERR_SYNC_POINT_NOT_RECEIVED) ||
		(s5p_mfc_get_warn(err) == S5P_FIMV_ERR_BROKEN_LINK)) {

		ref_mb = s5p_mfc_find_move_buf_vb(&ctx->buf_queue_lock,
				&ctx->dst_buf_queue, &ctx->ref_buf_queue, dspl_y_addr, released_flag);
		if (ref_mb) {
			mfc_debug(2, "Listing: %d\n", ref_mb->vb.vb2_buf.index);
			/* Check if this is the buffer we're looking for */
			mfc_debug(2, "Found 0x%08llx, looking for 0x%08llx\n",
				s5p_mfc_mem_get_daddr_vb(&ref_mb->vb.vb2_buf, 0), dspl_y_addr);

			index = ref_mb->vb.vb2_buf.index;

			if (released_flag & (1 << index)) {
				dec->available_dpb &= ~(1 << index);
				released_flag &= ~(1 << index);
				mfc_debug(2, "Corrupted frame(%d), it will be re-used(release)\n",
						s5p_mfc_get_warn(err));
			} else {
				dec->err_reuse_flag |= 1 << index;
				mfc_debug(2, "Corrupted frame(%d), it will be re-used(not released)\n",
						s5p_mfc_get_warn(err));
			}
			dec->dynamic_used |= released_flag;
		}
	} else {
		ref_mb = s5p_mfc_find_del_buf_vb(&ctx->buf_queue_lock,
				&ctx->ref_buf_queue, dspl_y_addr);
		if (ref_mb) {
			mfc_debug(2, "Listing: %d\n", ref_mb->vb.vb2_buf.index);
			/* Check if this is the buffer we're looking for */
			mfc_debug(2, "Found 0x%08llx, looking for 0x%08llx\n",
				s5p_mfc_mem_get_daddr_vb(&ref_mb->vb.vb2_buf, 0), dspl_y_addr);

			index = ref_mb->vb.vb2_buf.index;

			ref_mb->vb.sequence = ctx->sequence;

			if (is_interlace) {
				interlace_type = s5p_mfc_get_interlace_type();
				if (interlace_type)
					ref_mb->vb.field = V4L2_FIELD_INTERLACED_TB;
				else
					ref_mb->vb.field = V4L2_FIELD_INTERLACED_BT;
			}
			else
				ref_mb->vb.field = V4L2_FIELD_NONE;
			mfc_debug(2, "is_interlace : %d interlace_type : %d\n",
				is_interlace, interlace_type);

			/* Set reserved2 bits in order to inform SEI information */
			ref_mb->vb.reserved2 = 0;

			if (is_content_light) {
				ref_mb->vb.reserved2 |= (1 << 0);
				mfc_debug(2, "content light level parsed\n");
			}

			if (is_display_colour) {
				ref_mb->vb.reserved2 |= (1 << 1);
				mfc_debug(2, "mastering display colour parsed\n");
			}

			if (is_video_signal_type) {
				ref_mb->vb.reserved2 |= (1 << 4);
				mfc_debug(2, "video signal type parsed\n");
				if (is_colour_description) {
					ref_mb->vb.reserved2 |= (1 << 2);
					mfc_debug(5, "matrix coefficients parsed\n");
					ref_mb->vb.reserved2 |= (1 << 3);
					mfc_debug(2, "colour description parsed\n");
				}
			}

			if (dec->black_bar_updated) {
				ref_mb->vb.reserved2 |= (1 << 5);
				mfc_debug(3, "black bar detected\n");
			}

			if (ctx->src_fmt->mem_planes == 1) {
				vb2_set_plane_payload(&ref_mb->vb.vb2_buf, 0,
							raw->total_plane_size);
				mfc_debug(5, "single plane payload: %d\n",
							raw->total_plane_size);
			} else {
				for (i = 0; i < ctx->src_fmt->mem_planes; i++) {
					vb2_set_plane_payload(&ref_mb->vb.vb2_buf, i,
							raw->plane_size[i]);
				}
			}

			clear_bit(index, &dec->available_dpb);

			ref_mb->vb.flags &=
					~(V4L2_BUF_FLAG_KEYFRAME |
					V4L2_BUF_FLAG_PFRAME |
					V4L2_BUF_FLAG_BFRAME |
					V4L2_BUF_FLAG_ERROR);

			switch (frame_type) {
			case S5P_FIMV_DISPLAY_FRAME_I:
				ref_mb->vb.flags |=
					V4L2_BUF_FLAG_KEYFRAME;
				break;
			case S5P_FIMV_DISPLAY_FRAME_P:
				ref_mb->vb.flags |=
					V4L2_BUF_FLAG_PFRAME;
				break;
			case S5P_FIMV_DISPLAY_FRAME_B:
				ref_mb->vb.flags |=
					V4L2_BUF_FLAG_BFRAME;
				break;
			default:
				break;
			}

			if (s5p_mfc_get_warn(err)) {
				mfc_err_ctx("Warning for displayed frame: %d\n",
							s5p_mfc_get_warn(err));
				ref_mb->vb.flags |=
					V4L2_BUF_FLAG_ERROR;
			}

			if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->dst_ctrls[index]) < 0)
				mfc_err_ctx("failed in get_buf_ctrls_val\n");

			s5p_mfc_handle_released_info(ctx, released_flag, index);

			if (dec->immediate_display == 1) {
				dst_frame_status = s5p_mfc_get_dec_status();

				call_cop(ctx, get_buf_update_val, ctx,
						&ctx->dst_ctrls[index],
						V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS,
						dst_frame_status);

				call_cop(ctx, get_buf_update_val, ctx,
					&ctx->dst_ctrls[index],
					V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
					dec->stored_tag);

				dec->immediate_display = 0;
			}

			/* Update frame tag for packed PB */
			if (CODEC_MULTIFRAME(ctx) &&
					(dec->y_addr_for_pb == dspl_y_addr)) {
				call_cop(ctx, get_buf_update_val, ctx,
					&ctx->dst_ctrls[index],
					V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
					dec->stored_tag);
				dec->y_addr_for_pb = 0;
			}

			s5p_mfc_qos_update_last_framerate(ctx, &ref_mb->vb);
			vb2_buffer_done(&ref_mb->vb.vb2_buf,
				s5p_mfc_get_warn(err) ?
				VB2_BUF_STATE_ERROR : VB2_BUF_STATE_DONE);
		}
	}
}

static void mfc_handle_frame_error(struct s5p_mfc_ctx *ctx,
		unsigned int reason, unsigned int err)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_buf *src_mb;
	unsigned int index;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	if (ctx->type == MFCINST_ENCODER) {
		mfc_err_ctx("Encoder Interrupt Error: %d\n", err);
		return;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return;
	}

	mfc_err_ctx("Interrupt Error: %d\n", err);

	dec->dpb_flush = 0;

	/* Get the source buffer */
	src_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);

	if (!src_mb) {
		mfc_err_dev("no src buffers.\n");
	} else {
		index = src_mb->vb.vb2_buf.index;
		if (call_cop(ctx, recover_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in recover_buf_ctrls_val\n");

		mfc_debug(2, "MFC needs next buffer.\n");
		dec->consumed = 0;

		if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in get_buf_ctrls_val\n");

		/* decoder src buffer CFW UNPROT */
		if (ctx->is_drm) {
			if (test_bit(index, &ctx->stream_protect_flag)) {
				if (s5p_mfc_stream_buf_prot(ctx, src_mb, false))
					mfc_err_ctx("failed to CFW_UNPROT\n");
				else
					clear_bit(index, &ctx->stream_protect_flag);
			}
			mfc_debug(2, "[%d] dec src buf un-prot_flag: %#lx\n",
					index, ctx->stream_protect_flag);
		}

		vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	mfc_debug(2, "Assesing whether this context should be run again.\n");
}

static void mfc_handle_ref_frame(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	struct s5p_mfc_buf *dst_mb;
	dma_addr_t dec_addr;

	dec_addr = (dma_addr_t)s5p_mfc_get_dec_y_addr();

	/* Try to search decoded address in whole dst queue */
	dst_mb = s5p_mfc_find_move_buf_vb_used(&ctx->buf_queue_lock,
			&ctx->ref_buf_queue, &ctx->dst_buf_queue, dec_addr);
	if (dst_mb) {
		mfc_debug(2, "Found in dst queue = 0x%08llx, buf = 0x%08llx\n",
				dec_addr, s5p_mfc_mem_get_daddr_vb(&dst_mb->vb.vb2_buf, 0));

		if (!(dec->dynamic_set & s5p_mfc_get_dec_used_flag()))
			dec->dynamic_used |= dec->dynamic_set;
	} else {
		mfc_debug(2, "Can't find buffer for addr = 0x%08llx\n", dec_addr);
	}
}

static void mfc_handle_reuse_buffer(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	unsigned int prev_flag, released_flag = 0;
	int i;

	prev_flag = dec->dynamic_used;
	dec->dynamic_used = s5p_mfc_get_dec_used_flag();
	released_flag = prev_flag & (~dec->dynamic_used);

	if (!released_flag)
		return;

	/* Reuse not referenced buf anymore */
	for (i = 0; i < MFC_MAX_DPBS; i++)
		if (released_flag & (1 << i))
			if (s5p_mfc_move_reuse_buffer(ctx, i))
				released_flag &= ~(1 << i);

	/* Not reused buffer should be released when there is a display frame */
	dec->dec_only_release_flag |= released_flag;
	for (i = 0; i < MFC_MAX_DPBS; i++)
		if (released_flag & (1 << i))
			clear_bit(i, &dec->available_dpb);
}

/* Handle frame decoding interrupt */
static void mfc_handle_frame(struct s5p_mfc_ctx *ctx,
			unsigned int reason, unsigned int err)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	unsigned int dst_frame_status, sei_avail_frame_pack;
	struct s5p_mfc_buf *src_mb;
	unsigned int res_change, need_dpb_change, need_scratch_change;
	unsigned int index;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return;
	}

	dst_frame_status = s5p_mfc_get_disp_status();
	res_change = s5p_mfc_get_res_change();
	need_dpb_change = s5p_mfc_get_dpb_change();
	need_scratch_change = s5p_mfc_get_scratch_change();
	sei_avail_frame_pack = s5p_mfc_get_sei_avail_frame_pack();

	if (dec->immediate_display == 1)
		dst_frame_status = s5p_mfc_get_dec_status();

	mfc_debug(2, "Frame Status: %x\n", dst_frame_status);
	mfc_debug(5, "SEI available status: 0x%08x\n", s5p_mfc_get_sei_avail());
	mfc_debug(5, "SEI content light: 0x%08x\n", s5p_mfc_get_sei_content_light());
	mfc_debug(5, "SEI luminance: 0x%08x, 0x%08x white point: 0x%08x\n",
			s5p_mfc_get_sei_mastering0(), s5p_mfc_get_sei_mastering1(),
			s5p_mfc_get_sei_mastering2());
	mfc_debug(5, "SEI display primaries: 0x%08x, 0x%08x, 0x%08x\n",
			s5p_mfc_get_sei_mastering3(), s5p_mfc_get_sei_mastering4(),
			s5p_mfc_get_sei_mastering5());
	mfc_debug(2, "Used flag: old = %08x, new = %08x\n",
				dec->dynamic_used, s5p_mfc_get_dec_used_flag());

	if (ctx->state == MFCINST_RES_CHANGE_INIT)
		s5p_mfc_change_state(ctx, MFCINST_RES_CHANGE_FLUSH);

	if (res_change) {
		mfc_debug(2, "Resolution change set to %d\n", res_change);
		s5p_mfc_change_state(ctx, MFCINST_RES_CHANGE_INIT);
		ctx->wait_state = WAIT_DECODING;
		mfc_debug(2, "Decoding waiting! : %d\n", ctx->wait_state);
		return;
	}

	if (need_dpb_change || need_scratch_change)
		mfc_debug(2, "Interframe resolution change is not supported\n");

	if (dec->dpb_flush)
		dec->dpb_flush = 0;

	if (s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->src_buf_queue, 0) &&
		s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0)) {
		mfc_err_dev("Queue count is zero for src and dst\n");
		goto leave_handle_frame;
	}

	if (IS_H264_DEC(ctx) && sei_avail_frame_pack &&
		dst_frame_status == S5P_FIMV_DEC_STATUS_DECODING_ONLY) {
		mfc_debug(2, "Frame packing SEI exists for a frame.\n");
		mfc_debug(2, "Reallocate DPBs and issue init_buffer.\n");
		ctx->is_dpb_realloc = 1;
		s5p_mfc_change_state(ctx, MFCINST_HEAD_PARSED);
		ctx->capture_state = QUEUE_FREE;
		ctx->wait_state = WAIT_DECODING;
		mfc_handle_frame_all_extracted(ctx);
		goto leave_handle_frame;
	}

	/* All frames remaining in the buffer have been extracted  */
	if (dst_frame_status == S5P_FIMV_DEC_STATUS_DECODING_EMPTY) {
		if (ctx->state == MFCINST_RES_CHANGE_FLUSH) {
			struct mfc_timestamp *temp_ts = NULL;

			mfc_debug(2, "Last frame received after resolution change.\n");
			mfc_handle_frame_all_extracted(ctx);
			s5p_mfc_change_state(ctx, MFCINST_RES_CHANGE_END);
			/* If there is no display frame after resolution change,
			 * Some released frames can't be unprotected.
			 * So, check and request unprotection in the end of DRC.
			 */
			s5p_mfc_cleanup_assigned_dpb(ctx);

			/* empty the timestamp queue */
			while (!list_empty(&ctx->ts_list)) {
				temp_ts = list_entry((&ctx->ts_list)->next,
						struct mfc_timestamp, list);
				list_del(&temp_ts->list);
			}
			ctx->ts_count = 0;
			ctx->ts_is_full = 0;
			s5p_mfc_qos_reset_last_framerate(ctx);
			s5p_mfc_qos_set_framerate(ctx, DEC_MAX_FPS);

			goto leave_handle_frame;
		} else {
			mfc_handle_frame_all_extracted(ctx);
		}
	}

	/* Detection for QoS weight */
	if (!dec->super64_bframe && IS_SUPER64_BFRAME(ctx, s5p_mfc_get_lcu_size(),
				s5p_mfc_get_dec_frame_type())) {
		dec->super64_bframe = 1;
		s5p_mfc_qos_on(ctx);
	}

	switch (dst_frame_status) {
	case S5P_FIMV_DEC_STATUS_DECODING_DISPLAY:
		mfc_handle_ref_frame(ctx);
		break;
	case S5P_FIMV_DEC_STATUS_DECODING_ONLY:
		mfc_handle_ref_frame(ctx);
		/*
		 * Some cases can have many decoding only frames like VP9
		 * alt-ref frame. So need handling release buffer
		 * because of DPB full.
		 */
		mfc_handle_reuse_buffer(ctx);
		break;
	default:
		break;
	}

	if (dst_frame_status == S5P_FIMV_DEC_STATUS_DECODING_DISPLAY ||
	    dst_frame_status == S5P_FIMV_DEC_STATUS_DECODING_ONLY)
		mfc_handle_frame_copy_timestamp(ctx);

	/* A frame has been decoded and is in the buffer  */
	if (dst_frame_status == S5P_FIMV_DEC_STATUS_DISPLAY_ONLY ||
	    dst_frame_status == S5P_FIMV_DEC_STATUS_DECODING_DISPLAY) {
		mfc_handle_frame_new(ctx, err);
	} else {
		mfc_debug(2, "No frame decode.\n");
	}

	/* Mark source buffer as complete */
	if (dst_frame_status != S5P_FIMV_DEC_STATUS_DISPLAY_ONLY) {
		int deleted = 0;
		unsigned long consumed;

		consumed = dec->consumed + s5p_mfc_get_consumed_stream();

		if (s5p_mfc_get_err(err) == S5P_FIMV_ERR_NON_PAIRED_FIELD) {
			/*
			 * For non-paired field, the same buffer need to be
			 * resubmitted and the consumed stream will be 0
			 */
			mfc_debug(2, "Not paired field. Running again the same buffer.\n");
			goto leave_handle_frame;
		}

		/* Get the source buffer */
		src_mb = s5p_mfc_get_del_if_consumed(&ctx->buf_queue_lock, &ctx->src_buf_queue,
				consumed, STUFF_BYTE, err, &deleted);
		if (!src_mb) {
			mfc_err_dev("no src buffers.\n");
			goto leave_handle_frame;
		}

		if (!deleted) {
			/* Run MFC again on the same buffer */
			mfc_debug(2, "Running again the same buffer.\n");

			if (CODEC_MULTIFRAME(ctx))
				dec->y_addr_for_pb = (dma_addr_t)s5p_mfc_get_dec_y_addr();

			dec->consumed = consumed;
			dec->remained_size = src_mb->vb.vb2_buf.planes[0].bytesused
					- dec->consumed;
			dec->has_multiframe = 1;

			MFC_TRACE_CTX("** consumed:%ld, remained:%ld, addr:0x%08llx\n",
				dec->consumed, dec->remained_size, dec->y_addr_for_pb);
			/* Do not move src buffer to done_list */
			goto leave_handle_frame;
		}

		index = src_mb->vb.vb2_buf.index;
		if (call_cop(ctx, recover_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in recover_buf_ctrls_val\n");

		mfc_debug(2, "MFC needs next buffer.\n");
		dec->consumed = 0;
		dec->remained_size = 0;

		if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in get_buf_ctrls_val\n");

		/* decoder src buffer CFW UNPROT */
		if (ctx->is_drm) {
			if (test_bit(index, &ctx->stream_protect_flag)) {
				if (s5p_mfc_stream_buf_prot(ctx, src_mb, false))
					mfc_err_ctx("failed to CFW_UNPROT\n");
				else
					clear_bit(index, &ctx->stream_protect_flag);
			}
			mfc_debug(2, "[%d] dec src buf un-prot_flag: %#lx\n",
					index, ctx->stream_protect_flag);
		}

		vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
	}

leave_handle_frame:
	mfc_debug(2, "Assesing whether this context should be run again.\n");
}

/* Handle encoder resolution change */
static void mfc_enc_res_change(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	unsigned int reg = 0;

	/*
	 * Right after the first NAL_START finished with the new resolution,
	 * We need to reset the fields
	 */
	if (ctx->enc_res_change) {
		/* clear resolution change bits */
		s5p_mfc_clear_enc_res_change(dev);

		ctx->enc_res_change_state = ctx->enc_res_change;
		ctx->enc_res_change = 0;
	}

	if (ctx->enc_res_change_state == 1) { /* resolution swap */
		ctx->enc_res_change_state = 0;
	} else if (ctx->enc_res_change_state == 2) { /* resolution change */
		reg = s5p_mfc_get_enc_nal_done_info();

		/*
		 * Encoding resolution status
		 * 0: Normal encoding
		 * 1: Resolution Change for B-frame
		 *    (Encode with previous resolution)
		 * 2: Resolution Change for B-frame
		 *    (Last encoding with previous resolution)
		 * 3: Resolution Change for only P-frame
		 *    (No encode, as all frames with previous resolution are encoded)
		 */
		mfc_debug(2, "Encoding Resolution Status : %d\n", reg);

		if (reg == 2 || reg == 3) {
			s5p_mfc_release_codec_buffers(ctx);
			/* for INIT_BUFFER cmd */
			s5p_mfc_change_state(ctx, MFCINST_HEAD_PARSED);

			ctx->enc_res_change_state = 0;
			ctx->min_scratch_buf_size = s5p_mfc_get_enc_scratch_size();
			mfc_debug(2, "S5P_FIMV_E_MIN_SCRATCH_BUFFER_SIZE = 0x%x\n",
					(unsigned int)ctx->min_scratch_buf_size);
			if (reg == 3)
				ctx->enc_res_change_re_input = 1;
		}
	}
}

/* Handle frame encoding interrupt */
static int mfc_handle_stream(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_buf *ref_mb, *dst_mb, *src_mb;
	struct s5p_mfc_raw_info *raw;
	dma_addr_t enc_addr[3] = { 0, 0, 0 };
	int slice_type, i;
	unsigned int strm_size;
	unsigned int pic_count;
	unsigned int index;

	slice_type = s5p_mfc_get_enc_slice_type();
	strm_size = s5p_mfc_get_enc_strm_size();
	pic_count = s5p_mfc_get_enc_pic_count();

	mfc_debug(2, "encoded slice type: %d\n", slice_type);
	mfc_debug(2, "encoded stream size: %d\n", strm_size);
	mfc_debug(2, "display order: %d\n", pic_count);

	if (enc->buf_full) {
		s5p_mfc_change_state(ctx, MFCINST_ABORT_INST);
		return 0;
	}

	if (ctx->enc_res_change || ctx->enc_res_change_state)
		mfc_enc_res_change(ctx);

	/* set encoded frame type */
	enc->frame_type = slice_type;
	raw = &ctx->raw_buf;

	ctx->sequence++;
	if (strm_size > 0 || ctx->state == MFCINST_FINISHING
			  || ctx->state == MFCINST_RUNNING_BUF_FULL) {
		/* at least one more dest. buffers exist always  */
		dst_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
		if (!dst_mb) {
			mfc_err_dev("no dst buffers.\n");
			return -EINVAL;
		}

		dst_mb->vb.flags &=
			~(V4L2_BUF_FLAG_KEYFRAME |
			  V4L2_BUF_FLAG_PFRAME |
			  V4L2_BUF_FLAG_BFRAME);

		switch (slice_type) {
		case S5P_FIMV_E_SLICE_TYPE_I:
			dst_mb->vb.flags |=
				V4L2_BUF_FLAG_KEYFRAME;
			break;
		case S5P_FIMV_E_SLICE_TYPE_P:
			dst_mb->vb.flags |=
				V4L2_BUF_FLAG_PFRAME;
			break;
		case S5P_FIMV_E_SLICE_TYPE_B:
			dst_mb->vb.flags |=
				V4L2_BUF_FLAG_BFRAME;
			break;
		default:
			dst_mb->vb.flags |=
				V4L2_BUF_FLAG_KEYFRAME;
			break;
		}
		mfc_debug(2, "Slice type : %d\n", dst_mb->vb.flags);

		vb2_set_plane_payload(&dst_mb->vb.vb2_buf, 0, strm_size);

		index = dst_mb->vb.vb2_buf.index;
		if (call_cop(ctx, get_buf_ctrls_val, ctx, &ctx->dst_ctrls[index]) < 0)
			mfc_err_ctx("failed in get_buf_ctrls_val\n");

		if (strm_size == 0 && ctx->state == MFCINST_FINISHING)
			call_cop(ctx, get_buf_update_val, ctx,
				&ctx->dst_ctrls[index],
				V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
				enc->stored_tag);

		vb2_buffer_done(&dst_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

		/* encoder dst buffer CFW UNPROT */
		if (ctx->is_drm) {
			if (test_bit(index, &ctx->stream_protect_flag)) {
				if (s5p_mfc_stream_buf_prot(ctx, dst_mb, false))
					mfc_err_ctx("failed to CFW_PROT\n");
				else
					clear_bit(index, &ctx->stream_protect_flag);
			}
			mfc_debug(2, "[%d] enc dst buf un-prot_flag: %#lx\n",
					index, ctx->stream_protect_flag);
		}
	} else if (strm_size == 0 && slice_type == S5P_FIMV_E_SLICE_TYPE_SKIPPED) {
		/* TO-DO: skipped frame should be handled */
	}

	if (enc->in_slice) {
		if (s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0)) {
			s5p_mfc_clear_bit(ctx->num, &dev->work_bits);
		}

		return 0;
	}

	if (!ctx->enc_res_change_re_input && slice_type >= 0 &&
			ctx->state != MFCINST_FINISHING) {
		if (ctx->state == MFCINST_RUNNING_NO_OUTPUT ||
			ctx->state == MFCINST_RUNNING_BUF_FULL)
			s5p_mfc_change_state(ctx, MFCINST_RUNNING);

		s5p_mfc_get_enc_frame_buffer(ctx, &enc_addr[0], raw->num_planes);

		for (i = 0; i < raw->num_planes; i++)
			mfc_debug(2, "encoded[%d] addr: 0x%08llx\n",
						i, enc_addr[i]);

		src_mb = s5p_mfc_find_del_buf_raw(&ctx->buf_queue_lock,
			&ctx->src_buf_queue, enc_addr[0]);
		if (src_mb) {
			index = src_mb->vb.vb2_buf.index;
			if (call_cop(ctx, recover_buf_ctrls_val, ctx,
					&ctx->src_ctrls[index]) < 0)
				mfc_err_ctx("failed in recover_buf_ctrls_val\n");

			vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

			/* encoder src buffer CFW UNPROT */
			if (ctx->is_drm) {
				if (test_bit(index, &ctx->raw_protect_flag)) {
					if (s5p_mfc_raw_buf_prot(ctx, src_mb, false))
						mfc_err_ctx("failed to CFW_PROT\n");
					else
						clear_bit(index, &ctx->raw_protect_flag);
				}
				mfc_debug(2, "[%d] enc src buf un-prot_flag: %#lx\n",
						index, ctx->raw_protect_flag);
			}
		}

		ref_mb = s5p_mfc_find_del_buf_raw(&ctx->buf_queue_lock,
			&ctx->ref_buf_queue, enc_addr[0]);
		if (ref_mb) {
			vb2_buffer_done(&ref_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

			/* encoder src buffer CFW UNPROT */
			if (ctx->is_drm) {
				index = ref_mb->vb.vb2_buf.index;
				if (test_bit(index, &ctx->raw_protect_flag)) {
					if (s5p_mfc_raw_buf_prot(ctx, ref_mb, false))
						mfc_err_ctx("failed to CFW_PROT\n");
					else
						clear_bit(index, &ctx->raw_protect_flag);
				}
				mfc_debug(2, "[%d] enc src buf un-prot_flag: %#lx\n",
						index, ctx->raw_protect_flag);
			}
		}
	} else if (ctx->state == MFCINST_FINISHING) {
		src_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
		if (!src_mb) {
			mfc_err_dev("no src buffers.\n");
			return -EAGAIN;
		}

		vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

		/* encoder src buffer CFW UNPROT */
		if (ctx->is_drm) {
			index = src_mb->vb.vb2_buf.index;
			if (test_bit(index, &ctx->raw_protect_flag)) {
				if (s5p_mfc_raw_buf_prot(ctx, src_mb, false))
					mfc_err_ctx("failed to CFW_PROT\n");
				else
					clear_bit(index, &ctx->raw_protect_flag);
			}
			mfc_debug(2, "[%d] enc src buf un-prot_flag: %#lx\n",
					index, ctx->raw_protect_flag);
		}
	}

	if (ctx->enc_res_change_re_input)
		ctx->enc_res_change_re_input = 0;

	if (s5p_mfc_is_queue_count_greater(&ctx->buf_queue_lock, &ctx->src_buf_queue, 0) &&
		((ctx->state == MFCINST_RUNNING) ||
		 (ctx->state == MFCINST_RUNNING_NO_OUTPUT) ||
		 (ctx->state == MFCINST_RUNNING_BUF_FULL))) {

		s5p_mfc_move_first_buf_used(&ctx->buf_queue_lock,
			&ctx->ref_buf_queue, &ctx->src_buf_queue, MFC_QUEUE_ADD_BOTTOM);

		/* slice_type = 4 && strm_size = 0, skipped enable
		   should be considered */
		if ((slice_type == -1) && (strm_size == 0))
			s5p_mfc_change_state(ctx, MFCINST_RUNNING_NO_OUTPUT);

		mfc_debug(2, "slice_type: %d, ctx->state: %d\n", slice_type, ctx->state);
		mfc_debug(2, "enc src count: %d, enc ref count: %d\n",
			  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
			  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->ref_buf_queue));
	}

	return 0;
}

/* Error handling for interrupt */
static inline void mfc_handle_error(struct s5p_mfc_ctx *ctx,
	unsigned int reason, unsigned int err)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *src_mb;
	int index;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	mfc_err_ctx("Interrupt Error: display: %d, decoded: %d\n",
			s5p_mfc_get_warn(err), s5p_mfc_get_err(err));
	err = s5p_mfc_get_err(err);

	/* Error recovery is dependent on the state of context */
	switch (ctx->state) {
	case MFCINST_RES_CHANGE_END:
	case MFCINST_GOT_INST:
		/* This error had to happen while parsing the header */
		if (!ctx->is_drm) {
			unsigned char *stream_vir = NULL;
			unsigned int strm_size = 0;

			src_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
			if (src_mb) {
				stream_vir = src_mb->vir_addr;
				strm_size = src_mb->vb.vb2_buf.planes[0].bytesused;
				if (strm_size > 640)
					strm_size = 640;

				if (stream_vir && strm_size)
					print_hex_dump(KERN_ERR, "No header: ",
							DUMP_PREFIX_ADDRESS, 32, 4,
							stream_vir, strm_size, false);

				vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
			}
		} else {
			src_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
			if (src_mb) {
				index = src_mb->vb.vb2_buf.index;

				/* decoder src buffer CFW UNPROT */
				if (test_bit(index, &ctx->stream_protect_flag)) {
					if (s5p_mfc_stream_buf_prot(ctx, src_mb, false))
						mfc_err_ctx("failed to CFW_UNPROT\n");
					else
						clear_bit(index, &ctx->stream_protect_flag);
				}
				mfc_debug(2, "[%d] dec src buf un-prot_flag: %#lx\n",
						index, ctx->stream_protect_flag);
				vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
			}
		}
		break;
	case MFCINST_INIT:
		/* This error had to happen while acquireing instance */
	case MFCINST_RETURN_INST:
		/* This error had to happen while releasing instance */
	case MFCINST_DPB_FLUSHING:
		/* This error had to happen while flushing DPB */
	case MFCINST_SPECIAL_PARSING:
	case MFCINST_SPECIAL_PARSING_NAL:
		/* This error had to happen while special parsing */
		break;
	case MFCINST_HEAD_PARSED:
		/* This error had to happen while setting dst buffers */
	case MFCINST_RES_CHANGE_INIT:
	case MFCINST_RES_CHANGE_FLUSH:
		/* This error has to happen while resolution change */
	case MFCINST_ABORT_INST:
		/* This error has to happen while buffer full handling */
	case MFCINST_FINISHING:
		/* It is higly probable that an error occured
		 * while decoding a frame */
		s5p_mfc_change_state(ctx, MFCINST_ERROR);
		/* Mark all dst buffers as having an error */
		s5p_mfc_cleanup_queue(&ctx->buf_queue_lock, &ctx->dst_buf_queue);
		/* Mark all src buffers as having an error */
		s5p_mfc_cleanup_queue(&ctx->buf_queue_lock, &ctx->src_buf_queue);
		break;
	default:
		mfc_err_ctx("Encountered an error interrupt which had not been handled.\n");
		mfc_err_ctx("ctx->state = %d, ctx->inst_no = %d\n",
						ctx->state, ctx->inst_no);
		break;
	}

	s5p_mfc_wake_up_dev(dev, reason, err);

	return;
}

/* Handle header decoder interrupt */
static int mfc_handle_seq_dec(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	int i;

	if (ctx->src_fmt->fourcc != V4L2_PIX_FMT_FIMV1) {
		ctx->img_width = s5p_mfc_get_img_width();
		ctx->img_height = s5p_mfc_get_img_height();
		mfc_info_ctx("width: %d, height: %d\n", ctx->img_width, ctx->img_height);
	}

	ctx->dpb_count = s5p_mfc_get_dpb_count();
	ctx->scratch_buf_size = s5p_mfc_get_scratch_size();
	for (i = 0; i < ctx->dst_fmt->num_planes; i++)
		ctx->min_dpb_size[i] = s5p_mfc_get_min_dpb_size(i);

	dec->internal_dpb = 0;
	s5p_mfc_dec_store_crop_info(ctx);
	dec->mv_count = s5p_mfc_get_mv_count();
	if (IS_HEVC_DEC(ctx)) {
		dec->profile = s5p_mfc_get_profile();
		if (s5p_mfc_get_bit_depth_minus8() || IS_MAIN_10_PROFILE(dec) ||
			(IS_RANGE_EXT_PROFILE(dec) && s5p_mfc_get_hevc_main_12())) {
			ctx->is_10bit = 1;
			mfc_info_ctx("HEVC 10bit contents, profile: %d, depth: %d\n",
					s5p_mfc_get_profile(),
					s5p_mfc_get_bit_depth_minus8() + 8);
		} else if (IS_RANGE_EXT_PROFILE(dec) && s5p_mfc_get_hevc_422_10_intra()) {
			ctx->is_422_10_intra = 1;
			mfc_info_ctx("HEVC 422 contents\n");
		}
	}

	if (ctx->img_width == 0 || ctx->img_height == 0)
		s5p_mfc_change_state(ctx, MFCINST_ERROR);
	else
		s5p_mfc_change_state(ctx, MFCINST_HEAD_PARSED);

	if (ctx->state == MFCINST_HEAD_PARSED)
		dec->is_interlaced =
			s5p_mfc_is_interlace_picture();

	if (IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) || IS_HEVC_DEC(ctx)) {
		struct s5p_mfc_buf *src_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
		if (src_mb) {
			dec->consumed += s5p_mfc_get_consumed_stream();
			mfc_debug(2, "Check consumed size of header. ");
			mfc_debug(2, "total size : %d, consumed : %lu\n",
					src_mb->vb.vb2_buf.planes[0].bytesused, dec->consumed);
			if ((dec->consumed > 0) &&
					(src_mb->vb.vb2_buf.planes[0].bytesused > dec->consumed)) {
				dec->remained_size = src_mb->vb.vb2_buf.planes[0].bytesused -
					dec->consumed;
				mfc_debug(2, "there is remained bytes after header parsing\n");
				mfc_debug(2, "remained_size: %lu\n", dec->remained_size);
			} else {
				dec->consumed = 0;
				dec->remained_size = 0;
			}
		}
	}

	return 0;
}

/* Handle header encoder interrupt */
static int mfc_handle_seq_enc(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_enc_params *p = &enc->params;
	struct s5p_mfc_buf *dst_mb;
	int ret;

	mfc_debug(2, "seq header size: %d\n", s5p_mfc_get_enc_strm_size());

	if ((p->seq_hdr_mode == V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE) ||
	    (p->seq_hdr_mode == V4L2_MPEG_VIDEO_HEADER_MODE_AT_THE_READY)) {

		dst_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->dst_buf_queue, MFC_BUF_NO_TOUCH_USED);
		if (!dst_mb) {
			mfc_err_dev("no dst buffers.\n");
			return -EAGAIN;
		}

		vb2_set_plane_payload(&dst_mb->vb.vb2_buf, 0, s5p_mfc_get_enc_strm_size());
		vb2_buffer_done(&dst_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

		/* encoder dst buffer CFW UNPROT */
		if (ctx->is_drm) {
			int index = dst_mb->vb.vb2_buf.index;

			if (test_bit(index, &ctx->stream_protect_flag)) {
				if (s5p_mfc_stream_buf_prot(ctx, dst_mb, false))
					mfc_err_ctx("failed to CFW_PROT\n");
				else
					clear_bit(index, &ctx->stream_protect_flag);
			}
			mfc_debug(2, "[%d] enc dst buf un-prot_flag: %#lx\n",
					index, ctx->stream_protect_flag);
		}

	}

	ctx->dpb_count = s5p_mfc_get_enc_dpb_count();
	ctx->scratch_buf_size = s5p_mfc_get_enc_scratch_size();

	s5p_mfc_change_state(ctx, MFCINST_HEAD_PARSED);

	ret = s5p_mfc_alloc_codec_buffers(ctx);
	if (ret) {
		mfc_err_ctx("Failed to allocate encoding buffers.\n");
		return ret;
	}

	return 0;
}

irqreturn_t s5p_mfc_top_half_irq(int irq, void *priv)
{
	struct s5p_mfc_dev *dev = priv;
	struct s5p_mfc_ctx *ctx;
	unsigned int err;
	unsigned int reason;

	ctx = dev->ctx[dev->curr_ctx];
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return IRQ_WAKE_THREAD;
	}

	reason = s5p_mfc_get_int_reason();
	err = s5p_mfc_get_int_err();

	dev->last_int = reason;
	dev->last_int_time = ktime_to_timeval(ktime_get());

	if ((reason == S5P_FIMV_R2H_CMD_SEQ_DONE_RET) ||
		(reason == S5P_FIMV_R2H_CMD_INIT_BUFFERS_RET) ||
		(reason == S5P_FIMV_R2H_CMD_FRAME_DONE_RET) ||
		(reason == S5P_FIMV_R2H_CMD_QUEUE_DONE_RET))
		ctx->frame_cnt++;

	mfc_debug(2, "[c:%d] Int reason: %d (err: %d)\n",
			dev->curr_ctx, reason, err);
	MFC_TRACE_CTX("<< INT(top): %d\n", reason);
	MFC_TRACE_LOG_CTX("I%d", reason);

	return IRQ_WAKE_THREAD;
}

#ifdef NAL_Q_ENABLE
/*
 * Return value description
 *  0: NAL-Q is handled successfully
 *  1: NAL_START command
 * -1: Error
*/
static inline int mfc_nal_q_irq(struct s5p_mfc_dev *dev,
		unsigned int reason, unsigned int err)
{
	int ret = -1;
	unsigned int errcode;

	nal_queue_handle *nal_q_handle = dev->nal_q_handle;
	EncoderOutputStr *pOutStr;

	switch (reason) {
	case S5P_FIMV_R2H_CMD_QUEUE_DONE_RET:
		pOutStr = s5p_mfc_nal_q_dequeue_out_buf(dev,
			nal_q_handle->nal_q_out_handle, &errcode);
		if (pOutStr) {
			if (s5p_mfc_nal_q_handle_out_buf(dev, pOutStr))
				mfc_err_dev("NAL Q: Failed to handle out buf\n");
		} else {
			mfc_err_dev("NAL Q: pOutStr is NULL\n");
		}

		if (nal_q_handle->nal_q_exception)
			s5p_mfc_set_bit(nal_q_handle->nal_q_out_handle->nal_q_ctx,
					&dev->work_bits);
		s5p_mfc_clear_int_sfr();

		ret = 0;
		break;
	case S5P_FIMV_R2H_CMD_COMPLETE_QUEUE_RET:
		s5p_mfc_watchdog_stop_tick(dev);
		s5p_mfc_nal_q_cleanup_queue(dev);
		s5p_mfc_pm_clock_off(dev);
		nal_q_handle->nal_q_state = NAL_Q_STATE_CREATED;
		MFC_TRACE_DEV("** NAL Q state : %d\n", nal_q_handle->nal_q_state);
		mfc_debug(2, "NAL Q: return to created state\n");
		s5p_mfc_clear_int_sfr();
		s5p_mfc_wake_up_dev(dev, reason, err);

		ret = 0;
		break;
	default:
		if (nal_q_handle->nal_q_state == NAL_Q_STATE_STARTED ||
			nal_q_handle->nal_q_state == NAL_Q_STATE_STOPPED) {
			mfc_err_dev("NAL Q: Should not be here! state: %d, int reason : %d\n",
				nal_q_handle->nal_q_state, reason);
			s5p_mfc_clear_int_sfr();

			ret = -1;
		} else {
			/* NAL START */
			ret = 1;
		}

		break;
	}

	if (ret == 0)
		queue_work(dev->butler_wq, &dev->butler_work);

	return ret;
}
#endif

/* Interrupt processing */
irqreturn_t s5p_mfc_irq(int irq, void *priv)
{
	struct s5p_mfc_dev *dev = priv;
	struct s5p_mfc_ctx *ctx;
	struct s5p_mfc_dec *dec = NULL;
	struct s5p_mfc_enc *enc = NULL;
	unsigned int reason;
	unsigned int err;

#ifdef NAL_Q_ENABLE
	int ret = -1;
#endif
	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		goto irq_end;
	}

	if (s5p_mfc_pm_get_pwr_ref_cnt(dev) == 0) {
		mfc_err_dev("no mfc power on\n");
		goto irq_end;
	}

	/* Get the reason of interrupt and the error code */
	reason = s5p_mfc_get_int_reason();
	err = s5p_mfc_get_int_err();
	mfc_debug(1, "Int reason: %d (err: %d)\n", reason, err);
	MFC_TRACE_DEV("<< INT: %d\n", reason);

	dev->preempt_ctx = MFC_NO_INSTANCE_SET;

	if (dbg_enable && (reason != S5P_FIMV_R2H_CMD_QUEUE_DONE_RET))
		s5p_mfc_dbg_disable(dev);

#ifdef NAL_Q_ENABLE
	if (dev->nal_q_handle) {
		ret = mfc_nal_q_irq(dev, reason, err);
		if (ret == 0) {
			mfc_debug(2, "NAL_Q command was handled\n");
			goto irq_end;
		} else if (ret == 1){
			/* Path through */
			mfc_debug(2, "NAL_START command will be handled\n");
		} else {
			mfc_debug(2, "Error.\n");
			goto irq_end;
		}
	}
#endif

	/* Stop the timeout watchdog */
	if (reason != S5P_FIMV_R2H_CMD_FW_STATUS_RET)
		s5p_mfc_watchdog_stop_tick(dev);

	switch (reason) {
	case S5P_FIMV_R2H_CMD_CACHE_FLUSH_RET:
	case S5P_FIMV_R2H_CMD_SYS_INIT_RET:
	case S5P_FIMV_R2H_CMD_FW_STATUS_RET:
	case S5P_FIMV_R2H_CMD_SLEEP_RET:
	case S5P_FIMV_R2H_CMD_WAKEUP_RET:
		s5p_mfc_clear_int_sfr();
		s5p_mfc_wake_up_dev(dev, reason, err);
		goto irq_end;
	}

	ctx = dev->ctx[dev->curr_ctx];
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		s5p_mfc_clear_int_sfr();
		s5p_mfc_pm_clock_off(dev);
		goto irq_end;
	}

	if (ctx->type == MFCINST_DECODER)
		dec = ctx->dec_priv;
	else if (ctx->type == MFCINST_ENCODER)
		enc = ctx->enc_priv;

	switch (reason) {
	case S5P_FIMV_R2H_CMD_ERR_RET:
		/* An error has occured */
		if (ctx->state == MFCINST_RUNNING || ctx->state == MFCINST_ABORT ||
				ctx->state == MFCINST_RUNNING_NO_OUTPUT) {
			if ((s5p_mfc_get_err(err) >= S5P_FIMV_ERR_WARNINGS_START) &&
				(s5p_mfc_get_err(err) <= S5P_FIMV_ERR_WARNINGS_END))
				mfc_handle_frame(ctx, reason, err);
			else
				mfc_handle_frame_error(ctx, reason, err);
		} else {
			mfc_handle_error(ctx, reason, err);
		}
		break;
	case S5P_FIMV_R2H_CMD_SLICE_DONE_RET:
	case S5P_FIMV_R2H_CMD_FIELD_DONE_RET:
	case S5P_FIMV_R2H_CMD_FRAME_DONE_RET:
	case S5P_FIMV_R2H_CMD_COMPLETE_SEQ_RET:
	case S5P_FIMV_R2H_CMD_ENC_BUFFER_FULL_RET:
		if (ctx->type == MFCINST_DECODER) {
			if (ctx->state == MFCINST_SPECIAL_PARSING_NAL) {
				s5p_mfc_clear_int_sfr();
				s5p_mfc_pm_clock_off(dev);
				s5p_mfc_clear_bit(ctx->num, &dev->work_bits);
				s5p_mfc_change_state(ctx, MFCINST_RUNNING);
				s5p_mfc_wake_up_ctx(ctx, reason, err);
				goto irq_end;
			}
			mfc_handle_frame(ctx, reason, err);
		} else if (ctx->type == MFCINST_ENCODER) {
			if (reason == S5P_FIMV_R2H_CMD_SLICE_DONE_RET) {
				dev->preempt_ctx = ctx->num;
				enc->buf_full = 0;
				enc->in_slice = 1;
			} else if (reason == S5P_FIMV_R2H_CMD_ENC_BUFFER_FULL_RET) {
				mfc_err_ctx("stream buffer size(%d) isn't enough\n",
						s5p_mfc_get_enc_strm_size());
				dev->preempt_ctx = ctx->num;
				enc->buf_full = 1;
				enc->in_slice = 0;
			} else {
				enc->buf_full = 0;
				enc->in_slice = 0;
			}
			mfc_handle_stream(ctx);
		}
		break;
	case S5P_FIMV_R2H_CMD_SEQ_DONE_RET:
		if (ctx->type == MFCINST_ENCODER)
			mfc_handle_seq_enc(ctx);
		else if (ctx->type == MFCINST_DECODER)
			mfc_handle_seq_dec(ctx);
		break;
	case S5P_FIMV_R2H_CMD_OPEN_INSTANCE_RET:
		ctx->inst_no = s5p_mfc_get_inst_no();
		s5p_mfc_change_state(ctx, MFCINST_GOT_INST);
		break;
	case S5P_FIMV_R2H_CMD_CLOSE_INSTANCE_RET:
		s5p_mfc_change_state(ctx, MFCINST_FREE);
		break;
	case S5P_FIMV_R2H_CMD_NAL_ABORT_RET:
		if (ctx->type == MFCINST_ENCODER) {
			s5p_mfc_change_state(ctx, MFCINST_RUNNING_BUF_FULL);
			enc->buf_full = 0;
			if (IS_VP8_ENC(ctx))
				mfc_err_ctx("stream buffer size isn't enough\n");
			mfc_handle_stream(ctx);
		} else {
			s5p_mfc_change_state(ctx, MFCINST_ABORT);
		}
		break;
	case S5P_FIMV_R2H_CMD_DPB_FLUSH_RET:
		s5p_mfc_change_state(ctx, MFCINST_ABORT);
		break;
	case S5P_FIMV_R2H_CMD_INIT_BUFFERS_RET:
		if (err != 0) {
			mfc_err_ctx("INIT_BUFFERS_RET error: %d\n", err);
			break;
		}

		s5p_mfc_change_state(ctx, MFCINST_RUNNING);
		if (ctx->type == MFCINST_DECODER) {
			if (dec->dst_memtype == V4L2_MEMORY_MMAP)
				mfc_err_ctx("Not supported memory type (%d).\n", dec->dst_memtype);

			if (ctx->wait_state == WAIT_DECODING) {
				ctx->wait_state = WAIT_INITBUF_DONE;
				mfc_debug(2, "INIT_BUFFER has done, but can't start decoding\n");
			}
			if (ctx->is_dpb_realloc)
				ctx->is_dpb_realloc = 0;
		}
		break;
	default:
		mfc_err_ctx("Unknown int reason: %d\n", reason);
	}

	/* clean-up interrupt */
	s5p_mfc_clear_int_sfr();

	if ((ctx->state != MFCINST_RES_CHANGE_INIT) && (s5p_mfc_ctx_ready(ctx) == 0))
		s5p_mfc_clear_bit(ctx->num, &dev->work_bits);

	s5p_mfc_hwlock_handler_irq(dev, ctx, reason, err);

irq_end:
	mfc_debug_leave();
	return IRQ_HANDLED;
}
