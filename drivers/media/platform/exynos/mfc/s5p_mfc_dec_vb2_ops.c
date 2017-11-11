/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_dec_vb2_ops.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include  "s5p_mfc_common.h"

#include "s5p_mfc_hwlock.h"
#include "s5p_mfc_nal_q.h"
#include "s5p_mfc_watchdog.h"
#include "s5p_mfc_opr.h"
#include "s5p_mfc_sync.h"

#include "s5p_mfc_pm.h"

#include "s5p_mfc_queue.h"
#include "s5p_mfc_utils.h"
#include "s5p_mfc_buf.h"
#include "s5p_mfc_mem.h"

static int s5p_mfc_dec_queue_setup(struct vb2_queue *vq,
				const void *parg,
				unsigned int *buf_count, unsigned int *plane_count,
				unsigned int psize[], void *allocators[])
{
	struct s5p_mfc_ctx *ctx;
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_raw_info *raw;
	void *alloc_ctx;
	void *capture_ctx;
	int i;

	mfc_debug_enter();

	if (!vq) {
		mfc_err_dev("no vb2_queue info\n");
		return -EINVAL;
	}

	ctx = vq->drv_priv;
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

	raw = &ctx->raw_buf;
	alloc_ctx = ctx->dev->alloc_ctx;

	/* Video output for decoding (source)
	 * this can be set after getting an instance */
	if (ctx->state == MFCINST_GOT_INST &&
	    vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mfc_debug(2, "setting for VIDEO output\n");
		/* A single plane is required for input */
		*plane_count = 1;
		if (*buf_count < 1)
			*buf_count = 1;
		if (*buf_count > MFC_MAX_BUFFERS)
			*buf_count = MFC_MAX_BUFFERS;
	/* Video capture for decoding (destination)
	 * this can be set after the header was parsed */
	} else if (ctx->state == MFCINST_HEAD_PARSED &&
		   vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mfc_debug(2, "setting for VIDEO capture\n");
		/* Output plane count is different by the pixel format */
		*plane_count = ctx->dst_fmt->mem_planes;
		/* Setup buffer count */
		if (*buf_count < ctx->dpb_count)
			*buf_count = ctx->dpb_count;
		if (*buf_count > MFC_MAX_BUFFERS)
			*buf_count = MFC_MAX_BUFFERS;
	} else {
		mfc_err_ctx("State seems invalid. State = %d, vq->type = %d\n",
							ctx->state, vq->type);
		return -EINVAL;
	}
	mfc_debug(2, "buffer count=%d, plane count=%d type=0x%x\n",
					*buf_count, *plane_count, vq->type);

	if (ctx->state == MFCINST_HEAD_PARSED &&
	    vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (ctx->is_drm)
			capture_ctx = ctx->dev->alloc_ctx_drm;
		else
			capture_ctx = alloc_ctx;

		if (ctx->dst_fmt->mem_planes == 1) {
			psize[0] = raw->total_plane_size;
			allocators[0] = capture_ctx;
		} else {
			for (i = 0; i < ctx->dst_fmt->num_planes; i++) {
				psize[i] = raw->plane_size[i];
				allocators[i] = capture_ctx;
			}
		}

	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE &&
		   ctx->state == MFCINST_GOT_INST) {
		psize[0] = dec->src_buf_size;

		if (ctx->is_drm)
			allocators[0] = ctx->dev->alloc_ctx_drm;
		else
			allocators[0] = alloc_ctx;

	} else {
		mfc_err_ctx("Currently only decoding is supported. Decoding not initalised.\n");
		return -EINVAL;
	}

	mfc_debug(2, "plane=0, size=%d\n", psize[0]);
	mfc_debug(2, "plane=1, size=%d\n", psize[1]);

	mfc_debug_leave();

	return 0;
}

static void s5p_mfc_dec_unlock(struct vb2_queue *q)
{
	struct s5p_mfc_ctx *ctx = q->drv_priv;
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

	mutex_unlock(&dev->mfc_mutex);
}

static void s5p_mfc_dec_lock(struct vb2_queue *q)
{
	struct s5p_mfc_ctx *ctx = q->drv_priv;
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

	mutex_lock(&dev->mfc_mutex);
}

static int s5p_mfc_dec_buf_init(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct s5p_mfc_ctx *ctx = vq->drv_priv;
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_buf *buf = vb_to_mfc_buf(vb);
	dma_addr_t start_raw;
	int i, ret;

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

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = s5p_mfc_check_vb_with_fmt(ctx->dst_fmt, vb);
		if (ret < 0)
			return ret;

		start_raw = s5p_mfc_mem_get_daddr_vb(vb, 0);
		if (ctx->dst_fmt->fourcc == V4L2_PIX_FMT_NV12N) {
			buf->planes.raw[0] = start_raw;
			buf->planes.raw[1] = NV12N_CBCR_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
		} else if (ctx->dst_fmt->fourcc == V4L2_PIX_FMT_NV12N_10B) {
			buf->planes.raw[0] = start_raw;
			buf->planes.raw[1] = NV12N_10B_CBCR_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
		} else if (ctx->dst_fmt->fourcc == V4L2_PIX_FMT_YUV420N) {
			buf->planes.raw[0] = start_raw;
			buf->planes.raw[1] = YUV420N_CB_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
			buf->planes.raw[2] = YUV420N_CR_BASE(start_raw,
							ctx->img_width,
							ctx->img_height);
		} else {
			for (i = 0; i < ctx->dst_fmt->mem_planes; i++)
				buf->planes.raw[i] = s5p_mfc_mem_get_daddr_vb(vb, i);
		}

		if (call_cop(ctx, init_buf_ctrls, ctx, MFC_CTRL_TYPE_DST,
					vb->index) < 0)
			mfc_err_ctx("failed in init_buf_ctrls\n");
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = s5p_mfc_check_vb_with_fmt(ctx->src_fmt, vb);
		if (ret < 0)
			return ret;

		buf->planes.stream = s5p_mfc_mem_get_daddr_vb(vb, 0);

		if (call_cop(ctx, init_buf_ctrls, ctx, MFC_CTRL_TYPE_SRC,
					vb->index) < 0)
			mfc_err_ctx("failed in init_buf_ctrls\n");
	} else {
		mfc_err_ctx("s5p_mfc_dec_buf_init: unknown queue type.\n");
		return -EINVAL;
	}

	mfc_debug_leave();

	return 0;
}

static int s5p_mfc_dec_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct s5p_mfc_ctx *ctx = vq->drv_priv;
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_raw_info *raw;
	unsigned int index = vb->index;
	int i;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}
	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}
	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		raw = &ctx->raw_buf;
		/* check the size per plane */
		if (ctx->dst_fmt->mem_planes == 1) {
			mfc_debug(2, "Plane size = %lu, dst size:%d\n",
					vb2_plane_size(vb, 0),
					raw->total_plane_size);
			if (vb2_plane_size(vb, 0) < raw->total_plane_size) {
				mfc_err_ctx("Capture plane is too small\n");
				return -EINVAL;
			}
		} else {
			for (i = 0; i < ctx->dst_fmt->mem_planes; i++) {
				mfc_debug(2, "Plane[%d] size: %lu, dst[%d] size: %d\n",
						i, vb2_plane_size(vb, i),
						i, raw->plane_size[i]);
				if (vb2_plane_size(vb, i) < raw->plane_size[i]) {
					mfc_err_ctx("Capture plane[%d] is too small\n", i);
					return -EINVAL;
				}
			}
		}
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mfc_debug(2, "Plane size: %lu, ctx->dec_src_buf_size: %u\n",
				vb2_plane_size(vb, 0), dec->src_buf_size);

		if (vb2_plane_size(vb, 0) < dec->src_buf_size) {
			mfc_err_ctx("Plane buffer (OUTPUT) is too small.\n");
			return -EINVAL;
		}

		if (call_cop(ctx, to_buf_ctrls, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in to_buf_ctrls\n");
	}

	s5p_mfc_mem_buf_prepare(vb);

	return 0;
}

static void s5p_mfc_dec_buf_finish(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct s5p_mfc_ctx *ctx = vq->drv_priv;
	unsigned int index = vb->index;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (call_cop(ctx, to_ctx_ctrls, ctx, &ctx->dst_ctrls[index]) < 0)
			mfc_err_ctx("failed in to_ctx_ctrls\n");
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (call_cop(ctx, to_ctx_ctrls, ctx, &ctx->src_ctrls[index]) < 0)
			mfc_err_ctx("failed in to_ctx_ctrls\n");
	}

	s5p_mfc_mem_buf_finish(vb);
}

static void s5p_mfc_dec_buf_cleanup(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct s5p_mfc_ctx *ctx = vq->drv_priv;
	unsigned int index = vb->index;

	mfc_debug_enter();
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (call_cop(ctx, cleanup_buf_ctrls, ctx,
					MFC_CTRL_TYPE_DST, index) < 0)
			mfc_err_ctx("failed in cleanup_buf_ctrls\n");
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (call_cop(ctx, cleanup_buf_ctrls, ctx,
					MFC_CTRL_TYPE_SRC, index) < 0)
			mfc_err_ctx("failed in cleanup_buf_ctrls\n");
	} else {
		mfc_err_ctx("s5p_mfc_dec_buf_cleanup: unknown queue type.\n");
	}

	mfc_debug_leave();
}

static int s5p_mfc_dec_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct s5p_mfc_ctx *ctx = q->drv_priv;
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

	if (ctx->state == MFCINST_FINISHING)
		s5p_mfc_change_state(ctx, MFCINST_RUNNING);

	/* If context is ready then dev = work->data;schedule it to run */
	if (s5p_mfc_dec_ctx_ready(ctx)) {
		s5p_mfc_set_bit(ctx->num, &dev->work_bits);
	}

	s5p_mfc_try_run(dev);

	return 0;
}

static void s5p_mfc_dec_stop_streaming(struct vb2_queue *q)
{
	struct s5p_mfc_ctx *ctx = q->drv_priv;
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_dev *dev;
	int index = 0;
	int ret = 0;
	int prev_state;

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

	mfc_info_ctx("dec stop_streaming is called, hwlock : %d, type : %d\n",
				test_bit(ctx->num, &dev->hwlock.bits), q->type);
	MFC_TRACE_CTX("** DEC streamoff(type:%d)\n", q->type);

	MFC_TRACE_CTX_HWLOCK("**DEC streamoff(type:%d)\n", q->type);
	/* If a H/W operation is in progress, wait for it complete */
	ret = s5p_mfc_get_hwlock_ctx(ctx);
	if (ret < 0) {
		mfc_err_ctx("Failed to get hwlock.\n");
		return;
	}

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		s5p_mfc_cleanup_assigned_fd(ctx);
		s5p_mfc_cleanup_queue(&ctx->buf_queue_lock, &dec->ref_buf_queue);

		dec->dynamic_used = 0;
		dec->err_reuse_flag = 0;

		s5p_mfc_cleanup_queue(&ctx->buf_queue_lock, &ctx->dst_buf_queue);

		ctx->is_dpb_realloc = 0;
		dec->dpb_flush = 1;
		dec->available_dpb = 0;

		dec->y_addr_for_pb = 0;

		s5p_mfc_cleanup_assigned_dpb(ctx);

		while (index < MFC_MAX_BUFFERS) {
			index = find_next_bit(&ctx->dst_ctrls_avail,
					MFC_MAX_BUFFERS, index);
			if (index < MFC_MAX_BUFFERS)
				call_cop(ctx, reset_buf_ctrls, &ctx->dst_ctrls[index]);
			index++;
		}

		if (ctx->wait_state == WAIT_INITBUF_DONE ||
					ctx->wait_state == WAIT_DECODING) {
			ctx->wait_state = WAIT_NONE;
			mfc_debug(2, "Decoding can be started now\n");
		}
	} else if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		while (1) {
			struct s5p_mfc_buf *src_mb;
			int index, csd, condition = 0;

			csd = s5p_mfc_peek_buf_csd(&ctx->buf_queue_lock, &ctx->src_buf_queue);

			if (FW_HAS_SPECIAL_PARSING(dev) && (csd == 1)) {
				s5p_mfc_clean_ctx_int_flags(ctx);
				if (need_to_special_parsing(ctx)) {
					s5p_mfc_change_state(ctx, MFCINST_SPECIAL_PARSING);
					condition = S5P_FIMV_R2H_CMD_SEQ_DONE_RET;
					mfc_info_ctx("try to special parsing! (before NAL_START)\n");
				} else if (need_to_special_parsing_nal(ctx)) {
					s5p_mfc_change_state(ctx, MFCINST_SPECIAL_PARSING_NAL);
					condition = S5P_FIMV_R2H_CMD_FRAME_DONE_RET;
					mfc_info_ctx("try to special parsing! (after NAL_START)\n");
				} else {
					mfc_info_ctx("can't parsing CSD!, state = %d\n", ctx->state);
				}

				if (condition) {
					s5p_mfc_set_bit(ctx->num, &dev->work_bits);

					ret = s5p_mfc_just_run(dev, ctx->num);
					if (ret) {
						mfc_err_ctx("Failed to run MFC.\n");
					} else {
						if (s5p_mfc_wait_for_done_ctx(ctx, condition))
							mfc_err_ctx("special parsing time out\n");
					}
				}
			}

			src_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, MFC_BUF_NO_TOUCH_USED);
			if (!src_mb)
				break;

			index = src_mb->vb.vb2_buf.index;

			if (ctx->is_drm && test_bit(index, &ctx->stream_protect_flag)) {
				if (s5p_mfc_stream_buf_prot(ctx, src_mb, false))
					mfc_err_ctx("failed to CFW_UNPROT\n");
				else
					clear_bit(index, &ctx->stream_protect_flag);
				mfc_debug(2, "[%d] dec src buf un-prot flag: %#lx\n",
						index, ctx->stream_protect_flag);
			}
			vb2_set_plane_payload(&src_mb->vb.vb2_buf, 0, 0);
			vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_ERROR);
		}

		dec->consumed = 0;
		dec->remained_size = 0;

		s5p_mfc_init_queue(&ctx->src_buf_queue);

		while (index < MFC_MAX_BUFFERS) {
			index = find_next_bit(&ctx->src_ctrls_avail,
					MFC_MAX_BUFFERS, index);
			if (index < MFC_MAX_BUFFERS)
				call_cop(ctx, reset_buf_ctrls, &ctx->src_ctrls[index]);
			index++;
		}
	}

	if (ctx->state == MFCINST_FINISHING)
		s5p_mfc_change_state(ctx, MFCINST_RUNNING);

	if (q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE && need_to_dpb_flush(ctx)) {
		prev_state = ctx->state;
		s5p_mfc_change_state(ctx, MFCINST_DPB_FLUSHING);
		s5p_mfc_set_bit(ctx->num, &dev->work_bits);
		s5p_mfc_clean_ctx_int_flags(ctx);
		mfc_info_ctx("try to DPB flush\n");
		ret = s5p_mfc_just_run(dev, ctx->num);
		if (ret) {
			mfc_err_ctx("Failed to run MFC.\n");
			s5p_mfc_release_hwlock_ctx(ctx);
			s5p_mfc_cleanup_work_bit_and_try_run(ctx);
			return;
		}

		if (s5p_mfc_wait_for_done_ctx(ctx, S5P_FIMV_R2H_CMD_DPB_FLUSH_RET)) {
			mfc_err_ctx("time out during DPB flush\n");
			dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_DPB_FLUSH);
			s5p_mfc_dump_info_and_stop_hw(dev);
		}

		s5p_mfc_change_state(ctx, prev_state);
	}

	mfc_debug(2, "buffer cleanup & flush is done in stop_streaming, type : %d\n", q->type);

	s5p_mfc_clear_bit(ctx->num, &dev->work_bits);
	s5p_mfc_release_hwlock_ctx(ctx);

	if (s5p_mfc_dec_ctx_ready(ctx))
		s5p_mfc_set_bit(ctx->num, &dev->work_bits);
	if (s5p_mfc_is_work_to_do(dev))
		queue_work(dev->butler_wq, &dev->butler_work);
}

static void s5p_mfc_dec_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct s5p_mfc_ctx *ctx = vq->drv_priv;
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_buf *buf = vb_to_mfc_buf(vb);
	int index, i;
	unsigned char *stream_vir = NULL;

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

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return;
	}

	if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mfc_debug(2, "Src queue: %p\n", &ctx->src_buf_queue);
		mfc_debug(2, "Adding to src: %p (0x%08llx, 0x%08llx)\n", vb,
				s5p_mfc_mem_get_daddr_vb(vb, 0),
				buf->planes.stream);
		if (ctx->state < MFCINST_HEAD_PARSED && !ctx->is_drm) {
			stream_vir = vb2_plane_vaddr(vb, 0);
			s5p_mfc_mem_inv_vb(vb, 1);
		}
		buf->vir_addr = stream_vir;

		s5p_mfc_add_tail_buf(&ctx->buf_queue_lock, &ctx->src_buf_queue, buf);

		MFC_TRACE_CTX("Q src[%d] fd: %d, %#llx\n",
				vb->index, vb->planes[0].m.fd, buf->planes.stream);
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		index = vb->index;
		mfc_debug(2, "Dst queue: %p\n", &ctx->dst_buf_queue);
		mfc_debug(2, "Adding to dst: %p (0x%08llx)\n", vb,
				s5p_mfc_mem_get_daddr_vb(vb, 0));
		for (i = 0; i < ctx->dst_fmt->num_planes; i++)
			mfc_debug(2, "dec dst plane[%d]: %08llx\n",
					i, buf->planes.raw[i]);
		s5p_mfc_store_dpb(ctx, vb);

		if ((dec->dst_memtype == V4L2_MEMORY_USERPTR || dec->dst_memtype == V4L2_MEMORY_DMABUF) &&
				s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock,
					&ctx->dst_buf_queue, dec->total_dpb_count))
			ctx->capture_state = QUEUE_BUFS_MMAPED;

		MFC_TRACE_CTX("Q dst[%d] fd: %d, %#llx / avail %#lx used %#x\n",
				index, vb->planes[0].m.fd, buf->planes.raw[0],
				dec->available_dpb, dec->dynamic_used);
	} else {
		mfc_err_ctx("Unsupported buffer type (%d)\n", vq->type);
	}

	if (s5p_mfc_dec_ctx_ready(ctx)) {
		s5p_mfc_set_bit(ctx->num, &dev->work_bits);
		s5p_mfc_try_run(dev);
	}

	mfc_debug_leave();
}

struct vb2_ops s5p_mfc_dec_qops = {
	.queue_setup		= s5p_mfc_dec_queue_setup,
	.wait_prepare		= s5p_mfc_dec_unlock,
	.wait_finish		= s5p_mfc_dec_lock,
	.buf_init		= s5p_mfc_dec_buf_init,
	.buf_prepare		= s5p_mfc_dec_buf_prepare,
	.buf_finish		= s5p_mfc_dec_buf_finish,
	.buf_cleanup		= s5p_mfc_dec_buf_cleanup,
	.start_streaming	= s5p_mfc_dec_start_streaming,
	.stop_streaming		= s5p_mfc_dec_stop_streaming,
	.buf_queue		= s5p_mfc_dec_buf_queue,
};
