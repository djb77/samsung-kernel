/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_enc.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "s5p_mfc_enc.h"
#include "s5p_mfc_enc_internal.h"

#include "s5p_mfc_hwlock.h"
#include "s5p_mfc_otf.h"
#include "s5p_mfc_opr.h"
#include "s5p_mfc_sync.h"

#include "s5p_mfc_qos.h"
#include "s5p_mfc_queue.h"
#include "s5p_mfc_utils.h"
#include "s5p_mfc_buf.h"
#include "s5p_mfc_mem.h"

static struct s5p_mfc_fmt *mfc_enc_find_format(struct v4l2_format *f, unsigned int t)
{
	unsigned long i;

	for (i = 0; i < NUM_FORMATS; i++) {
		if (enc_formats[i].fourcc == f->fmt.pix_mp.pixelformat &&
		    enc_formats[i].type == t)
			return (struct s5p_mfc_fmt *)&enc_formats[i];
	}

	return NULL;
}

static struct v4l2_queryctrl *mfc_enc_get_ctrl(int id)
{
	unsigned long i;

	for (i = 0; i < NUM_CTRLS; ++i)
		if (id == controls[i].id)
			return &controls[i];
	return NULL;
}

static int mfc_enc_check_ctrl_val(struct s5p_mfc_ctx *ctx, struct v4l2_control *ctrl)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct v4l2_queryctrl *c;

	c = mfc_enc_get_ctrl(ctrl->id);
	if (!c)
		return -EINVAL;

	if (ctrl->id == V4L2_CID_MPEG_VIDEO_GOP_SIZE
	    && ctrl->value > c->maximum) {
		mfc_info_ctx("GOP_SIZE is changed to max(%d -> %d)\n",
                                ctrl->value, c->maximum);
		ctrl->value = c->maximum;
	}

	if (ctrl->id == V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER) {
		if ((ctrl->value & ~(1 << 16)) < c->minimum || (ctrl->value & ~(1 << 16)) > c->maximum
		    || (c->step != 0 && (ctrl->value & ~(1 << 16)) % c->step != 0)) {
			mfc_err_ctx("Invalid control value for %#x (%#x)\n", ctrl->id, ctrl->value);
			return -ERANGE;
		} else {
			return 0;
		}
	}

	if (ctrl->value < c->minimum || ctrl->value > c->maximum
	    || (c->step != 0 && ctrl->value % c->step != 0)) {
		mfc_err_ctx("Invalid control value for %#x (%#x)\n", ctrl->id, ctrl->value);
		return -ERANGE;
	}

	if (!FW_HAS_ROI_CONTROL(dev) && ctrl->id == \
			V4L2_CID_MPEG_VIDEO_ROI_CONTROL) {
		mfc_err_ctx("Not support feature(0x%x) for F/W\n", ctrl->id);
		return -ERANGE;
	}

	return 0;
}

static inline int mfc_enc_h264_profile(struct s5p_mfc_ctx *ctx, int profile)
{
	int ret = 0;

	switch (profile) {
	case V4L2_MPEG_VIDEO_H264_PROFILE_MAIN:
		ret = S5P_FIMV_E_PROFILE_H264_MAIN;
		break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH:
		ret = S5P_FIMV_E_PROFILE_H264_HIGH;
		break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE:
		ret = S5P_FIMV_E_PROFILE_H264_BASELINE;
		break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE:
		ret = S5P_FIMV_E_PROFILE_H264_CONSTRAINED_BASELINE;
		break;
	case V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_HIGH:
		ret = S5P_FIMV_E_PROFILE_H264_CONSTRAINED_HIGH;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/* Query capabilities of the device */
static int vidioc_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	strncpy(cap->driver, "MFC", sizeof(cap->driver) - 1);
	strncpy(cap->card, "encoder", sizeof(cap->card) - 1);
	cap->bus_info[0] = 0;
	cap->version = KERNEL_VERSION(1, 0, 0);
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE
			| V4L2_CAP_VIDEO_OUTPUT
			| V4L2_CAP_VIDEO_CAPTURE_MPLANE
			| V4L2_CAP_VIDEO_OUTPUT_MPLANE
			| V4L2_CAP_STREAMING
			| V4L2_CAP_DEVICE_CAPS;

	cap->capabilities = cap->device_caps;

	return 0;
}

static int mfc_enc_enum_fmt(struct v4l2_fmtdesc *f, bool mplane, bool out)
{
	struct s5p_mfc_fmt *fmt;
	unsigned long i;
	int j = 0;

	for (i = 0; i < NUM_FORMATS; ++i) {
		if (mplane && enc_formats[i].mem_planes == 1)
			continue;
		else if (!mplane && enc_formats[i].mem_planes > 1)
			continue;
		if (out && enc_formats[i].type != MFC_FMT_RAW)
			continue;
		else if (!out && enc_formats[i].type != MFC_FMT_ENC)
			continue;

		if (j == f->index) {
			fmt = &enc_formats[i];
			strlcpy(f->description, fmt->name,
				sizeof(f->description));
			f->pixelformat = fmt->fourcc;

			return 0;
		}

		++j;
	}

	return -EINVAL;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void *pirv,
				   struct v4l2_fmtdesc *f)
{
	return mfc_enc_enum_fmt(f, false, false);
}

static int vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *pirv,
					  struct v4l2_fmtdesc *f)
{
	return mfc_enc_enum_fmt(f, true, false);
}

static int vidioc_enum_fmt_vid_out(struct file *file, void *prov,
				   struct v4l2_fmtdesc *f)
{
	return mfc_enc_enum_fmt(f, false, true);
}

static int vidioc_enum_fmt_vid_out_mplane(struct file *file, void *prov,
					  struct v4l2_fmtdesc *f)
{
	return mfc_enc_enum_fmt(f, true, true);
}

static int vidioc_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	struct s5p_mfc_raw_info *raw;
	int i;

	mfc_debug_enter();

	mfc_debug(2, "f->type = %d ctx->state = %d\n", f->type, ctx->state);

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		/* This is run on output (encoder dest) */
		pix_fmt_mp->width = 0;
		pix_fmt_mp->height = 0;
		pix_fmt_mp->field = V4L2_FIELD_NONE;
		pix_fmt_mp->pixelformat = ctx->dst_fmt->fourcc;
		pix_fmt_mp->num_planes = ctx->dst_fmt->mem_planes;

		pix_fmt_mp->plane_fmt[0].bytesperline = enc->dst_buf_size;
		pix_fmt_mp->plane_fmt[0].sizeimage = enc->dst_buf_size;
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		/* This is run on capture (encoder src) */
		raw = &ctx->raw_buf;

		pix_fmt_mp->width = ctx->img_width;
		pix_fmt_mp->height = ctx->img_height;
		pix_fmt_mp->field = V4L2_FIELD_NONE;
		pix_fmt_mp->pixelformat = ctx->src_fmt->fourcc;
		pix_fmt_mp->num_planes = ctx->src_fmt->mem_planes;
		for (i = 0; i < ctx->src_fmt->mem_planes; i++) {
			pix_fmt_mp->plane_fmt[i].bytesperline = raw->stride[i];
			pix_fmt_mp->plane_fmt[i].sizeimage = raw->plane_size[i];
		}
	} else {
		mfc_err_dev("invalid buf type (%d)\n", f->type);
		return -EINVAL;
	}

	mfc_debug_leave();

	return 0;
}

static int vidioc_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct s5p_mfc_fmt *fmt;
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		fmt = mfc_enc_find_format(f, MFC_FMT_ENC);
		if (!fmt) {
			mfc_err_dev("failed to try capture format\n");
			return -EINVAL;
		}

		if (pix_fmt_mp->plane_fmt[0].sizeimage == 0) {
			mfc_err_dev("must be set encoding dst size\n");
			return -EINVAL;
		}

		pix_fmt_mp->plane_fmt[0].bytesperline =
			pix_fmt_mp->plane_fmt[0].sizeimage;
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		fmt = mfc_enc_find_format(f, MFC_FMT_RAW);
		if (!fmt) {
			mfc_err_dev("failed to try output format\n");
			return -EINVAL;
		}

		if (fmt->mem_planes != pix_fmt_mp->num_planes) {
			mfc_err_dev("plane number is different (%d != %d)\n",
				fmt->mem_planes, pix_fmt_mp->num_planes);
			return -EINVAL;
		}
	} else {
		mfc_err_dev("invalid buf type (%d)\n", f->type);
		return -EINVAL;
	}

	return 0;
}

static void mfc_enc_check_format(struct s5p_mfc_ctx *ctx)
{
	switch (ctx->src_fmt->fourcc) {
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
	case V4L2_PIX_FMT_NV16M_P210:
	case V4L2_PIX_FMT_NV61M_P210:
		mfc_debug(2, "is 422 and 10bit format\n");
		ctx->is_10bit = 1;
		ctx->is_422format = 1;
		break;
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV61M:
		mfc_debug(2, "is 422 format\n");
		ctx->is_10bit = 0;
		ctx->is_422format = 1;
		break;
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV21M_S10B:
	case V4L2_PIX_FMT_NV21M_P010:
		mfc_debug(2, "is 10bit format\n");
		ctx->is_10bit = 1;
		ctx->is_422format = 0;
		break;
	default:
		ctx->is_10bit = 0;
		ctx->is_422format = 0;
		break;
	}
	mfc_debug(2, "10bit: %d, 422: %d\n", ctx->is_10bit, ctx->is_422format);
}

static int vidioc_s_fmt_vid_cap_mplane(struct file *file, void *priv,
							struct v4l2_format *f)
{
	struct s5p_mfc_dev *dev = video_drvdata(file);
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_fmt *fmt;
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	int ret = 0;

	mfc_debug_enter();

	ret = vidioc_try_fmt(file, priv, f);
	if (ret)
		return ret;

	if (ctx->vq_dst.streaming) {
		mfc_err_ctx("dst queue busy\n");
		return -EBUSY;
	}

	fmt = mfc_enc_find_format(f, MFC_FMT_ENC);
	if (!fmt) {
		mfc_err_ctx("failed to set capture format\n");
		return -EINVAL;
	}

	ctx->dst_fmt = fmt;
	ctx->codec_mode = ctx->dst_fmt->codec_mode;
	mfc_info_ctx("Enc output codec(%d) : %s\n",
			ctx->dst_fmt->codec_mode, ctx->dst_fmt->name);

	if (ctx->otf_handle && ctx->dst_fmt->fourcc != V4L2_PIX_FMT_H264 &&
			ctx->dst_fmt->fourcc != V4L2_PIX_FMT_HEVC) {
		mfc_err_ctx("OTF: only H.264 and HEVC is supported\n");
		return -EINVAL;
	}

	enc->dst_buf_size = pix_fmt_mp->plane_fmt[0].sizeimage;
	pix_fmt_mp->plane_fmt[0].bytesperline = 0;

	ret = s5p_mfc_alloc_instance_context(ctx);
	if (ret) {
		mfc_err_ctx("Failed to allocate enc instance[%d] buffers.\n",
				ctx->num);
		return -ENOMEM;
	}

	s5p_mfc_change_state(ctx, MFCINST_INIT);

	ctx->capture_state = QUEUE_FREE;

	if (FW_HAS_ROI_CONTROL(dev)) {
		ret = s5p_mfc_alloc_enc_roi_buffer(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate ROI buffers.\n");
			s5p_mfc_release_instance_context(ctx);
			return -ENOMEM;
		}
	}
	MFC_TRACE_CTX_HWLOCK("**ENC s_fmt\n");

	ret = s5p_mfc_get_hwlock_ctx(ctx);
	if (ret < 0) {
		mfc_err_dev("Failed to get hwlock.\n");
		s5p_mfc_release_instance_context(ctx);
		s5p_mfc_release_enc_roi_buffer(ctx);
		return -EBUSY;
	}

	s5p_mfc_set_bit(ctx->num, &dev->work_bits);
	ret = s5p_mfc_just_run(dev, ctx->num);
	if (ret) {
		mfc_err_ctx("Failed to run MFC.\n");
		s5p_mfc_release_hwlock_ctx(ctx);
		s5p_mfc_cleanup_work_bit_and_try_run(ctx);
		s5p_mfc_release_instance_context(ctx);
		s5p_mfc_release_enc_roi_buffer(ctx);
		return -EIO;
	}

	if (s5p_mfc_wait_for_done_ctx(ctx,
				S5P_FIMV_R2H_CMD_OPEN_INSTANCE_RET)) {
		mfc_err_ctx("time out during open instance\n");
		s5p_mfc_release_hwlock_ctx(ctx);
		s5p_mfc_cleanup_work_bit_and_try_run(ctx);
		s5p_mfc_release_instance_context(ctx);
		s5p_mfc_release_enc_roi_buffer(ctx);
		return -EIO;
	}
	s5p_mfc_release_hwlock_ctx(ctx);

	mfc_debug(2, "Got instance number: %d\n", ctx->inst_no);

	if (s5p_mfc_enc_ctx_ready(ctx))
		s5p_mfc_set_bit(ctx->num, &dev->work_bits);
	if (ctx->otf_handle && s5p_mfc_otf_ctx_ready(ctx))
		s5p_mfc_set_bit(ctx->num, &dev->work_bits);
	if (s5p_mfc_is_work_to_do(dev))
		queue_work(dev->butler_wq, &dev->butler_work);

	mfc_debug_leave();
	return 0;
}

static int vidioc_s_fmt_vid_out_mplane(struct file *file, void *priv,
							struct v4l2_format *f)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_fmt *fmt;
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	int ret = 0;

	mfc_debug_enter();

	ret = vidioc_try_fmt(file, priv, f);
	if (ret)
		return ret;

	if (ctx->vq_src.streaming) {
		mfc_err_ctx("src queue busy\n");
		return -EBUSY;
	}

	if (ctx->otf_handle) {
		mfc_info_ctx("OTF: skip source s_fmt\n");
		return 0;
	}

	fmt = mfc_enc_find_format(f, MFC_FMT_RAW);
	if (!fmt) {
		mfc_err_ctx("failed to set output format\n");
		return -EINVAL;
	}
	if (fmt->fourcc == V4L2_PIX_FMT_NV12MT) {
		mfc_err_ctx("Not supported format: NV12MT\n");
		return -EINVAL;
	} else if (fmt->fourcc == V4L2_PIX_FMT_NV12MT_16X16) {
		mfc_err_ctx("Not supported format: NV12MT_16X16\n");
		return -EINVAL;
	}

	if (fmt->mem_planes != pix_fmt_mp->num_planes) {
		mfc_err_ctx("plane number is different (%d != %d)\n",
				fmt->mem_planes, pix_fmt_mp->num_planes);
		return -EINVAL;
	}

	ctx->src_fmt = fmt;
	ctx->raw_buf.num_planes = ctx->src_fmt->num_planes;
	ctx->img_width = pix_fmt_mp->width;
	ctx->img_height = pix_fmt_mp->height;
	ctx->buf_stride = pix_fmt_mp->plane_fmt[0].bytesperline;

	mfc_enc_check_format(ctx);

	if ((ctx->state == MFCINST_RUNNING)
		&& (((ctx->old_img_width != 0) && (ctx->old_img_width != ctx->img_width))
		|| ((ctx->old_img_height != 0) && (ctx->old_img_height != ctx->img_height)))) {
		ctx->enc_drc_flag = 1;
	}

	mfc_info_ctx("Enc input pixelformat : %s\n", ctx->src_fmt->name);
	mfc_info_ctx("fmt - w: %d, h: %d, stride: %d\n",
			pix_fmt_mp->width, pix_fmt_mp->height, ctx->buf_stride);

	s5p_mfc_enc_calc_src_size(ctx);

	ctx->output_state = QUEUE_FREE;

	mfc_debug_leave();
	return 0;
}

static int vidioc_reqbufs(struct file *file, void *priv,
					  struct v4l2_requestbuffers *reqbufs)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret = 0;

	mfc_debug_enter();

	mfc_debug(2, "type: %d\n", reqbufs->memory);

	if (reqbufs->memory == V4L2_MEMORY_MMAP) {
		mfc_err_ctx("Not supported memory type (%d).\n", reqbufs->memory);
		return -EINVAL;
	}

	if (ctx->otf_handle) {
		mfc_info_ctx("OTF: skip reqbufs\n");
		return 0;
	}

	if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (reqbufs->count == 0) {
			mfc_debug(2, "Freeing buffers.\n");
			ret = vb2_reqbufs(&ctx->vq_dst, reqbufs);
			ctx->capture_state = QUEUE_FREE;
			return ret;
		}

		if (ctx->capture_state != QUEUE_FREE) {
			mfc_err_ctx("invalid capture state: %d\n", ctx->capture_state);
			return -EINVAL;
		}

		ret = vb2_reqbufs(&ctx->vq_dst, reqbufs);
		if (ret) {
			mfc_err_ctx("error in vb2_reqbufs() for E(D)\n");
			return ret;
		}

		ctx->capture_state = QUEUE_BUFS_REQUESTED;
	} else if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (reqbufs->count == 0) {
			mfc_debug(2, "Freeing buffers.\n");
			ret = vb2_reqbufs(&ctx->vq_src, reqbufs);
			ctx->output_state = QUEUE_FREE;
			return ret;
		}

		if (ctx->output_state != QUEUE_FREE) {
			mfc_err_ctx("invalid output state: %d\n", ctx->output_state);
			return -EINVAL;
		}

		ret = vb2_reqbufs(&ctx->vq_src, reqbufs);
		if (ret) {
			mfc_err_ctx("error in vb2_reqbufs() for E(S)\n");
			return ret;
		}

		ctx->output_state = QUEUE_BUFS_REQUESTED;
	} else {
		mfc_err_ctx("invalid buf type (%d)\n", reqbufs->type);
		return -EINVAL;
	}

	mfc_debug_leave();

	return ret;
}

static int vidioc_querybuf(struct file *file, void *priv,
						   struct v4l2_buffer *buf)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret = 0;

	mfc_debug_enter();

	if (ctx->otf_handle) {
		mfc_info_ctx("OTF: skip source querybuf\n");
		return 0;
	}

	mfc_debug(2, "state: %d, buf->type: %d\n", ctx->state, buf->type);
	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = vb2_querybuf(&ctx->vq_dst, buf);
		if (ret != 0) {
			mfc_err_dev("enc dst: error in vb2_querybuf()\n");
			return ret;
		}
	} else if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = vb2_querybuf(&ctx->vq_src, buf);
		if (ret != 0) {
			mfc_err_dev("enc src: error in vb2_querybuf()\n");
			return ret;
		}
	} else {
		mfc_err_dev("invalid buf type (%d)\n", buf->type);
		return -EINVAL;
	}

	mfc_debug_leave();

	return ret;
}

/* Queue a buffer */
static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int i, ret = -EINVAL;

	mfc_debug_enter();

	if (ctx->otf_handle) {
		mfc_info_ctx("OTF: skip qbuf\n");
		return 0;
	}

	mfc_debug(2, "Enqueued buf: %d (type = %d)\n", buf->index, buf->type);
	if (ctx->state == MFCINST_ERROR) {
		mfc_err_ctx("Call on QBUF after unrecoverable error.\n");
		return -EIO;
	}

	if (V4L2_TYPE_IS_MULTIPLANAR(buf->type) && !buf->length) {
		mfc_err_ctx("multiplanar but length is zero\n");
		return -EIO;
	}

	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		for (i = 0; i < ctx->src_fmt->num_planes; i++) {
			if (!buf->m.planes[i].bytesused) {
				mfc_debug(2, "Src input[%d] size zero, "
						"changed to buf size %d\n",
						i, buf->m.planes[i].length);
				buf->m.planes[i].bytesused = buf->m.planes[i].length;
			} else {
				mfc_debug(2, "Src input[%d] size %d\n",
						i, buf->m.planes[i].bytesused);
			}
		}
		ret = vb2_qbuf(&ctx->vq_src, buf);
	} else {
		ret = vb2_qbuf(&ctx->vq_dst, buf);
	}

	mfc_debug_leave();
	return ret;
}

/* Dequeue a buffer */
static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret;

	mfc_debug_enter();

	if (ctx->otf_handle) {
		mfc_info_ctx("OTF: skip dqbuf\n");
		return 0;
	}

	mfc_debug(2, "Addr: %p %p %p Type: %d\n", &ctx->vq_src, buf, buf->m.planes,
								buf->type);
	if (ctx->state == MFCINST_ERROR) {
		mfc_err_ctx("Call on DQBUF after unrecoverable error.\n");
		return -EIO;
	}
	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		ret = vb2_dqbuf(&ctx->vq_src, buf, file->f_flags & O_NONBLOCK);
	else
		ret = vb2_dqbuf(&ctx->vq_dst, buf, file->f_flags & O_NONBLOCK);
	mfc_debug_leave();
	return ret;
}

/* Stream on */
static int vidioc_streamon(struct file *file, void *priv,
			   enum v4l2_buf_type type)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret = -EINVAL;

	mfc_debug_enter();

	if (ctx->otf_handle) {
		mfc_info_ctx("OTF: skip streamon\n");
		return 0;
	}

	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = vb2_streamon(&ctx->vq_src, type);

		if (!ret) {
			s5p_mfc_qos_on(ctx);
		}
	} else {
		ret = vb2_streamon(&ctx->vq_dst, type);
	}
	mfc_debug(2, "src: %d, dst: %d, state = %d, dpb_count = %d\n",
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_queue),
		  ctx->state, ctx->dpb_count);
	mfc_debug_leave();
	return ret;
}

/* Stream off, which equals to a pause */
static int vidioc_streamoff(struct file *file, void *priv,
			    enum v4l2_buf_type type)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret;

	mfc_debug_enter();

	if (ctx->otf_handle) {
		mfc_info_ctx("OTF: skip streamoff\n");
		return 0;
	}

	ret = -EINVAL;
	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		s5p_mfc_qos_reset_last_framerate(ctx);

		ctx->old_img_width = ctx->img_width;
		ctx->old_img_height = ctx->img_height;

		mfc_debug(2, "vidioc_streamoff ctx->old_img_width = %d\n", ctx->old_img_width);
		mfc_debug(2, "vidioc_streamoff ctx->old_img_height = %d\n", ctx->old_img_height);

		ret = vb2_streamoff(&ctx->vq_src, type);
		if (!ret)
			s5p_mfc_qos_off(ctx);
	} else {
		ret = vb2_streamoff(&ctx->vq_dst, type);
	}

	mfc_debug(2, "streamoff\n");
	mfc_debug_leave();

	return ret;
}

/* Query a ctrl */
static int vidioc_queryctrl(struct file *file, void *priv,
			    struct v4l2_queryctrl *qc)
{
	struct v4l2_queryctrl *c;

	c = mfc_enc_get_ctrl(qc->id);
	if (!c)
		return -EINVAL;
	*qc = *c;
	return 0;
}

static int mfc_enc_ext_info(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	int val = 0;

	val |= ENC_SET_SPARE_SIZE;
	val |= ENC_SET_TEMP_SVC_CH;

	if (FW_SUPPORT_SKYPE(dev))
		val |= ENC_SET_SKYPE_FLAG;

	if (FW_HAS_ROI_CONTROL(dev))
		val |= ENC_SET_ROI_CONTROL;

	val |= ENC_SET_QP_BOUND_PB;
	val |= ENC_SET_FIXED_SLICE;
	val |= ENC_SET_PVC_MODE;

	if (FW_HAS_RATIO_INTRA_CTRL(dev))
		val |= ENC_SET_RATIO_OF_INTRA;

	if (FW_HAS_ENC_COLOR_ASPECT(dev))
		val |= ENC_SET_COLOR_ASPECT;

	return val;
}

static int mfc_enc_get_ctrl_val(struct s5p_mfc_ctx *ctx, struct v4l2_control *ctrl)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_ctx_ctrl *ctx_ctrl;
	int ret = 0;
	int found = 0;

	switch (ctrl->id) {
	case V4L2_CID_CACHEABLE:
		mfc_debug(5, "it is supported only V4L2_MEMORY_MMAP\n");
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_STREAM_SIZE:
		ctrl->value = enc->dst_buf_size;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TYPE:
		ctrl->value = enc->frame_type;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE:
		if (ctx->state == MFCINST_RUNNING_NO_OUTPUT)
			ctrl->value = MFCSTATE_ENC_NO_OUTPUT;
		else
			ctrl->value = MFCSTATE_PROCESSING;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG:
	case V4L2_CID_MPEG_MFC51_VIDEO_LUMA_ADDR:
	case V4L2_CID_MPEG_MFC51_VIDEO_CHROMA_ADDR:
	case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_STATUS:
		list_for_each_entry(ctx_ctrl, &ctx->ctrls, list) {
			if (!(ctx_ctrl->type & MFC_CTRL_TYPE_GET))
				continue;

			if (ctx_ctrl->id == ctrl->id) {
				if (ctx_ctrl->has_new) {
					ctx_ctrl->has_new = 0;
					ctrl->value = ctx_ctrl->val;
				} else {
					mfc_debug(8, "Control value "\
							"is not up to date: "\
							"0x%08x\n", ctrl->id);
					return -EINVAL;
				}

				found = 1;
				break;
			}
		}

		if (!found) {
			mfc_err_ctx("Invalid control: 0x%08x\n", ctrl->id);
			return -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_MFC_GET_VERSION_INFO:
		ctrl->value = s5p_mfc_version(dev);
		break;
	case V4L2_CID_MPEG_MFC_GET_DRIVER_INFO:
		ctrl->value = MFC_DRIVER_INFO;
		break;
	case V4L2_CID_MPEG_MFC_GET_EXTRA_BUFFER_SIZE:
		ctrl->value = MFC_LINEAR_BUF_SIZE;
		break;
	case V4L2_CID_MPEG_VIDEO_QOS_RATIO:
		ctrl->value = ctx->qos_ratio;
		break;
	case V4L2_CID_MPEG_MFC_GET_EXT_INFO:
		ctrl->value = mfc_enc_ext_info(ctx);
		break;
	case V4L2_CID_MPEG_VIDEO_BPG_HEADER_SIZE:
		ctrl->value = enc->header_size;
		break;
	default:
		mfc_err_ctx("Invalid control: 0x%08x\n", ctrl->id);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int vidioc_g_ctrl(struct file *file, void *priv,
			 struct v4l2_control *ctrl)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret = 0;

	mfc_debug_enter();
	ret = mfc_enc_get_ctrl_val(ctx, ctrl);
	mfc_debug_leave();

	return ret;
}

static inline int mfc_enc_h264_level(enum v4l2_mpeg_video_h264_level lvl)
{
	static unsigned int t[V4L2_MPEG_VIDEO_H264_LEVEL_5_1 + 1] = {
		/* V4L2_MPEG_VIDEO_H264_LEVEL_1_0   */ 10,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_1B    */ 9,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_1_1   */ 11,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_1_2   */ 12,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_1_3   */ 13,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_2_0   */ 20,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_2_1   */ 21,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_2_2   */ 22,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_3_0   */ 30,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_3_1   */ 31,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_3_2   */ 32,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_4_0   */ 40,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_4_1   */ 41,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_4_2   */ 42,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_5_0   */ 50,
		/* V4L2_MPEG_VIDEO_H264_LEVEL_5_1   */ 51,
	};
	return t[lvl];
}

static inline int mfc_enc_mpeg4_level(enum v4l2_mpeg_video_mpeg4_level lvl)
{
	static unsigned int t[V4L2_MPEG_VIDEO_MPEG4_LEVEL_6 + 1] = {
		/* V4L2_MPEG_VIDEO_MPEG4_LEVEL_0	     */ 0,
		/* V4L2_MPEG_VIDEO_MPEG4_LEVEL_0B, Simple    */ 9,
		/* V4L2_MPEG_VIDEO_MPEG4_LEVEL_1	     */ 1,
		/* V4L2_MPEG_VIDEO_MPEG4_LEVEL_2	     */ 2,
		/* V4L2_MPEG_VIDEO_MPEG4_LEVEL_3	     */ 3,
		/* V4L2_MPEG_VIDEO_MPEG4_LEVEL_3B, Advanced  */ 7,
		/* V4L2_MPEG_VIDEO_MPEG4_LEVEL_4	     */ 4,
		/* V4L2_MPEG_VIDEO_MPEG4_LEVEL_5	     */ 5,
		/* V4L2_MPEG_VIDEO_MPEG4_LEVEL_6,  Simple    */ 6,
	};
	return t[lvl];
}

static inline int mfc_enc_vui_sar_idc(enum v4l2_mpeg_video_h264_vui_sar_idc sar)
{
	static unsigned int t[V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_EXTENDED + 1] = {
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_UNSPECIFIED     */ 0,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_1x1             */ 1,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_12x11           */ 2,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_10x11           */ 3,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_16x11           */ 4,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_40x33           */ 5,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_24x11           */ 6,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_20x11           */ 7,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_32x11           */ 8,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_80x33           */ 9,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_18x11           */ 10,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_15x11           */ 11,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_64x33           */ 12,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_160x99          */ 13,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_4x3             */ 14,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_3x2             */ 15,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_2x1             */ 16,
		/* V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_EXTENDED        */ 255,
	};
	return t[sar];
}

static int mfc_enc_set_param(struct s5p_mfc_ctx *ctx, struct v4l2_control *ctrl)
{
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_enc_params *p = &enc->params;
	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		p->gop_size = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE:
		p->slice_mode =
			(enum v4l2_mpeg_video_multi_slice_mode)ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB:
		p->slice_mb = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES:
		p->slice_bit = ctrl->value * 8;
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB_ROW:
		p->slice_mb_row = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB:
		p->intra_refresh_mb = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_PADDING:
		p->pad = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_PADDING_YUV:
		p->pad_luma = (ctrl->value >> 16) & 0xff;
		p->pad_cb = (ctrl->value >> 8) & 0xff;
		p->pad_cr = (ctrl->value >> 0) & 0xff;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		p->rc_frame = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		p->rc_bitrate = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF:
		p->rc_reaction_coeff = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE:
		enc->force_frame_type = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VBV_SIZE:
		p->vbv_buf_size = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEADER_MODE:
		p->seq_hdr_mode =
			(enum v4l2_mpeg_video_header_mode)(ctrl->value);
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE:
		p->frame_skip_mode =
			(enum v4l2_mpeg_mfc51_video_frame_skip_mode)
			(ctrl->value);
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT:
		p->fixed_target_bit = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_B_FRAMES:
		p->num_b_frame = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
		p->codec.h264.profile =
		mfc_enc_h264_profile(ctx, (enum v4l2_mpeg_video_h264_profile)(ctrl->value));
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
		p->codec.h264.level =
		mfc_enc_h264_level((enum v4l2_mpeg_video_h264_level)(ctrl->value));
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_H264_INTERLACE:
		p->codec.h264.interlace = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE:
		p->codec.h264.loop_filter_mode =
		(enum v4l2_mpeg_video_h264_loop_filter_mode)(ctrl->value);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA:
		p->codec.h264.loop_filter_alpha = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA:
		p->codec.h264.loop_filter_beta = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE:
		p->codec.h264.entropy_mode =
			(enum v4l2_mpeg_video_h264_entropy_mode)(ctrl->value);
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_H264_NUM_REF_PIC_FOR_P:
		p->num_refs_for_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM:
		p->codec.h264._8x8_transform = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE:
		p->rc_mb = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_H264_RC_FRAME_RATE:
		p->codec.h264.rc_framerate = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
		p->codec.h264.rc_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
		p->codec.h264.rc_min_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
		p->codec.h264.rc_max_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP_P:
		p->codec.h264.rc_min_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP_P:
		p->codec.h264.rc_max_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP_B:
		p->codec.h264.rc_min_qp_b = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP_B:
		p->codec.h264.rc_max_qp_b = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_DARK:
		p->codec.h264.rc_mb_dark = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_SMOOTH:
		p->codec.h264.rc_mb_smooth = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_STATIC:
		p->codec.h264.rc_mb_static = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_ACTIVITY:
		p->codec.h264.rc_mb_activity = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
		p->codec.h264.rc_p_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP:
		p->codec.h264.rc_b_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE:
		p->codec.h264.ar_vui = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC:
		p->codec.h264.ar_vui_idc =
		mfc_enc_vui_sar_idc((enum v4l2_mpeg_video_h264_vui_sar_idc)(ctrl->value));
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH:
		p->codec.h264.ext_sar_width = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT:
		p->codec.h264.ext_sar_height = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_CLOSURE:
		p->codec.h264.open_gop = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_PERIOD:
		p->codec.h264.open_gop_size = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING:
		p->codec.h264.hier_qp_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE:
		p->codec.h264.hier_qp_type =
		(enum v4l2_mpeg_video_h264_hierarchical_coding_type)(ctrl->value);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER:
		p->codec.h264.num_hier_layer = ctrl->value & 0x7;
		p->codec.h264.hier_ref_type = (ctrl->value >> 16) & 0x1;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP:
		p->codec.h264.hier_qp_layer[(ctrl->value >> 16) & 0x7]
			= ctrl->value & 0xFF;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT0:
		p->codec.h264.hier_bit_layer[0] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT1:
		p->codec.h264.hier_bit_layer[1] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT2:
		p->codec.h264.hier_bit_layer[2] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT3:
		p->codec.h264.hier_bit_layer[3] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT4:
		p->codec.h264.hier_bit_layer[4] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT5:
		p->codec.h264.hier_bit_layer[5] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT6:
		p->codec.h264.hier_bit_layer[6] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING:
		p->codec.h264.sei_gen_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_SEI_FP_CURRENT_FRAME_0:
		p->codec.h264.sei_fp_curr_frame_0 = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE:
		p->codec.h264.sei_fp_arrangement_type = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_FMO:
		p->codec.h264.fmo_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_FMO_MAP_TYPE:
		switch ((enum v4l2_mpeg_video_h264_fmo_map_type)(ctrl->value)) {
		case V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_INTERLEAVED_SLICES:
		case V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_SCATTERED_SLICES:
		case V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_RASTER_SCAN:
		case V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_WIPE_SCAN:
			p->codec.h264.fmo_slice_map_type = ctrl->value;
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_FMO_SLICE_GROUP:
		p->codec.h264.fmo_slice_num_grp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH:
		p->codec.h264.fmo_run_length[(ctrl->value >> 30) & 0x3]
			= ctrl->value & 0x3FFFFFFF;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_DIRECTION:
		p->codec.h264.fmo_sg_dir = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_RATE:
		p->codec.h264.fmo_sg_rate = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_ASO:
		p->codec.h264.aso_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_ASO_SLICE_ORDER:
		p->codec.h264.aso_slice_order[(ctrl->value >> 18) & 0x7]
			&= ~(0xFF << (((ctrl->value >> 16) & 0x3) << 3));
		p->codec.h264.aso_slice_order[(ctrl->value >> 18) & 0x7]
			|= (ctrl->value & 0xFF) << \
				(((ctrl->value >> 16) & 0x3) << 3);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PREPEND_SPSPPS_TO_IDR:
		p->codec.h264.prepend_sps_pps_to_idr = ctrl->value ? 1 : 0;
		break;
	case V4L2_CID_MPEG_MFC_H264_ENABLE_LTR:
		p->codec.h264.enable_ltr = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC_H264_NUM_OF_LTR:
		p->codec.h264.num_of_ltr = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC_H264_BASE_PRIORITY:
		p->codec.h264.base_priority = ctrl->value;
		p->codec.h264.set_priority = 1;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_PROFILE:
		switch ((enum v4l2_mpeg_video_mpeg4_profile)(ctrl->value)) {
		case V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE:
			p->codec.mpeg4.profile =
				S5P_FIMV_E_PROFILE_MPEG4_SIMPLE;
			break;
		case V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE:
			p->codec.mpeg4.profile =
			S5P_FIMV_E_PROFILE_MPEG4_ADVANCED_SIMPLE;
			break;
		default:
			ret = -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_LEVEL:
		p->codec.mpeg4.level =
		mfc_enc_mpeg4_level((enum v4l2_mpeg_video_mpeg4_level)(ctrl->value));
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP:
		p->codec.mpeg4.rc_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP:
		p->codec.mpeg4.rc_min_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP:
		p->codec.mpeg4.rc_max_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_P:
		p->codec.mpeg4.rc_min_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_P:
		p->codec.mpeg4.rc_max_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_B:
		p->codec.mpeg4.rc_min_qp_b = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_B:
		p->codec.mpeg4.rc_max_qp_b = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_QPEL:
		p->codec.mpeg4.quarter_pixel = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP:
		p->codec.mpeg4.rc_p_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MPEG4_B_FRAME_QP:
		p->codec.mpeg4.rc_b_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_TIME_RES:
		p->codec.mpeg4.vop_time_res = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_FRM_DELTA:
		p->codec.mpeg4.vop_frm_delta = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_H263_RC_FRAME_RATE:
		p->codec.mpeg4.rc_framerate = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP:
		p->codec.mpeg4.rc_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H263_MIN_QP:
		p->codec.mpeg4.rc_min_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H263_MAX_QP:
		p->codec.mpeg4.rc_max_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H263_MIN_QP_P:
		p->codec.mpeg4.rc_min_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H263_MAX_QP_P:
		p->codec.mpeg4.rc_max_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP:
		p->codec.mpeg4.rc_p_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_VERSION:
		p->codec.vp8.vp8_version = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_RC_FRAME_RATE:
		p->codec.vp8.rc_framerate = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_MIN_QP:
		p->codec.vp8.rc_min_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_MAX_QP:
		p->codec.vp8.rc_max_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_MIN_QP_P:
		p->codec.vp8.rc_min_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_MAX_QP_P:
		p->codec.vp8.rc_max_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_I_FRAME_QP:
		p->codec.vp8.rc_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_P_FRAME_QP:
		p->codec.vp8.rc_p_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_NUM_OF_PARTITIONS:
		p->codec.vp8.vp8_numberofpartitions = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_FILTER_LEVEL:
		p->codec.vp8.vp8_filterlevel = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_FILTER_SHARPNESS:
		p->codec.vp8.vp8_filtersharpness = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_GOLDEN_FRAMESEL:
		p->codec.vp8.vp8_goldenframesel = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_GF_REFRESH_PERIOD:
		p->codec.vp8.vp8_gfrefreshperiod = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_ENABLE:
		p->codec.vp8.hier_qp_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_LAYER0:
		p->codec.vp8.hier_qp_layer[(ctrl->value >> 16) & 0x3]
			= ctrl->value & 0xFF;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_LAYER1:
		p->codec.vp8.hier_qp_layer[(ctrl->value >> 16) & 0x3]
			= ctrl->value & 0xFF;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_LAYER2:
		p->codec.vp8.hier_qp_layer[(ctrl->value >> 16) & 0x3]
			= ctrl->value & 0xFF;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_BIT0:
		p->codec.vp8.hier_bit_layer[0] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_BIT1:
		p->codec.vp8.hier_bit_layer[1] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_BIT2:
		p->codec.vp8.hier_bit_layer[2] = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_REF_NUMBER_FOR_PFRAMES:
		p->num_refs_for_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_DISABLE_INTRA_MD4X4:
		p->codec.vp8.intra_4x4mode_disable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC70_VIDEO_VP8_NUM_TEMPORAL_LAYER:
		p->codec.vp8.num_hier_layer = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_VERSION:
		p->codec.vp9.vp9_version = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_RC_FRAME_RATE:
		p->codec.vp9.rc_framerate = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_MIN_QP:
		p->codec.vp9.rc_min_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_MAX_QP:
		p->codec.vp9.rc_max_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_MIN_QP_P:
		p->codec.vp9.rc_min_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_MAX_QP_P:
		p->codec.vp9.rc_max_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_I_FRAME_QP:
		p->codec.vp9.rc_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_P_FRAME_QP:
		p->codec.vp9.rc_p_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_GOLDEN_FRAMESEL:
		p->codec.vp9.vp9_goldenframesel = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_GF_REFRESH_PERIOD:
		p->codec.vp9.vp9_gfrefreshperiod = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_HIERARCHY_QP_ENABLE:
		p->codec.vp9.hier_qp_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_QP:
		p->codec.vp9.hier_qp_layer[(ctrl->value >> 16) & 0x3]
			= ctrl->value & 0xFF;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_BIT0:
		p->codec.vp9.hier_bit_layer[0] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_BIT1:
		p->codec.vp9.hier_bit_layer[1] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_BIT2:
		p->codec.vp9.hier_bit_layer[2] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_REF_NUMBER_FOR_PFRAMES:
		p->num_refs_for_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER:
		p->codec.vp9.num_hier_layer = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_MAX_PARTITION_DEPTH:
		p->codec.vp9.max_partition_depth = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_DISABLE_INTRA_PU_SPLIT:
		p->codec.vp9.intra_pu_split_disable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_DISABLE_IVF_HEADER:
		p->ivf_header_disable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_PROFILE:
		p->codec.vp9.profile = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP:
		p->codec.hevc.rc_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_QP:
		p->codec.hevc.rc_p_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_QP:
		p->codec.hevc.rc_b_frame_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_RC_FRAME_RATE:
		p->codec.hevc.rc_framerate = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP:
		p->codec.hevc.rc_min_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP:
		p->codec.hevc.rc_max_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_P:
		p->codec.hevc.rc_min_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_P:
		p->codec.hevc.rc_max_qp_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_B:
		p->codec.hevc.rc_min_qp_b = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_B:
		p->codec.hevc.rc_max_qp_b = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_LEVEL:
		p->codec.hevc.level = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_PROFILE:
		p->codec.hevc.profile = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_DARK:
		p->codec.hevc.rc_lcu_dark = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_SMOOTH:
		p->codec.hevc.rc_lcu_smooth = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_STATIC:
		p->codec.hevc.rc_lcu_static = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_ACTIVITY:
		p->codec.hevc.rc_lcu_activity = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_TIER_FLAG:
		p->codec.hevc.tier_flag = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_MAX_PARTITION_DEPTH:
		p->codec.hevc.max_partition_depth = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_REF_NUMBER_FOR_PFRAMES:
		p->num_refs_for_p = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_REFRESH_TYPE:
		p->codec.hevc.refreshtype = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_CONST_INTRA_PRED_ENABLE:
		p->codec.hevc.const_intra_period_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LOSSLESS_CU_ENABLE:
		p->codec.hevc.lossless_cu_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_WAVEFRONT_ENABLE:
		p->codec.hevc.wavefront_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_DISABLE:
		p->codec.hevc.loopfilter_disable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_SLICE_BOUNDARY:
		p->codec.hevc.loopfilter_across = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LTR_ENABLE:
		p->codec.hevc.enable_ltr = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_QP_ENABLE:
		p->codec.hevc.hier_qp_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_TYPE:
		p->codec.hevc.hier_qp_type =
		(enum v4l2_mpeg_video_hevc_hierarchical_coding_type)(ctrl->value);
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER:
		p->codec.hevc.num_hier_layer = ctrl->value & 0x7;
		p->codec.hevc.hier_ref_type = (ctrl->value >> 16) & 0x1;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_QP:
		p->codec.hevc.hier_qp_layer[(ctrl->value >> 16) & 0x7]
			= ctrl->value & 0xFF;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT0:
		p->codec.hevc.hier_bit_layer[0] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT1:
		p->codec.hevc.hier_bit_layer[1] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT2:
		p->codec.hevc.hier_bit_layer[2] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT3:
		p->codec.hevc.hier_bit_layer[3] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT4:
		p->codec.hevc.hier_bit_layer[4] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT5:
		p->codec.hevc.hier_bit_layer[5] = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT6:
		p->codec.hevc.hier_bit_layer[6] = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_GENERAL_PB_ENABLE:
		p->codec.hevc.general_pb_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_TEMPORAL_ID_ENABLE:
		p->codec.hevc.temporal_id_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_STRONG_SMOTHING_FLAG:
		p->codec.hevc.strong_intra_smooth = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_DISABLE_INTRA_PU_SPLIT:
		p->codec.hevc.intra_pu_split_disable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_DISABLE_TMV_PREDICTION:
		p->codec.hevc.tmv_prediction_disable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_MAX_NUM_MERGE_MV_MINUS1:
		p->codec.hevc.max_num_merge_mv = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_WITHOUT_STARTCODE_ENABLE:
		p->codec.hevc.encoding_nostartcode_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_REFRESH_PERIOD:
		p->codec.hevc.refreshperiod = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_BETA_OFFSET_DIV2:
		p->codec.hevc.lf_beta_offset_div2 = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_TC_OFFSET_DIV2:
		p->codec.hevc.lf_tc_offset_div2 = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_SIZE_OF_LENGTH_FIELD:
		p->codec.hevc.size_of_length_field = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_USER_REF:
		p->codec.hevc.user_ref = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_STORE_REF:
		p->codec.hevc.store_ref = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_ROI_ENABLE:
		p->roi_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC_H264_VUI_RESTRICTION_ENABLE:
		p->codec.h264.vui_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_PREPEND_SPSPPS_TO_IDR:
		p->codec.hevc.prepend_sps_pps_to_idr = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC_CONFIG_QP_ENABLE:
		p->dynamic_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC_CONFIG_QP:
		p->config_qp = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_RC_PVC_ENABLE:
		/* It is valid for H.264, HEVC, VP8, VP9 */
		p->rc_pvc = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_TEMPORAL_SHORTTERM_MAX_LAYER:
		p->num_hier_max_layer = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC90_VIDEO_HEVC_SIGN_DATA_HIDING:
		break;
	case V4L2_CID_MPEG_VIDEO_WEIGHTED_ENABLE:
		p->weighted_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_BPG_THUMBNAIL_SIZE:
		/* It should be included when NAL_START */
		p->codec.bpg.thumb_size = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_BPG_EXIF_SIZE:
		/* It should be included when NAL_START */
		p->codec.bpg.exif_size = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_RATIO_OF_INTRA:
		p->ratio_intra = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_FULL_RANGE_FLAG:
		p->color_range = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_COLOUR_PRIMARIES:
		p->colour_primaries = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_TRANSFER_CHARACTERISTICS:
		p->transfer_characteristics = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_MATRIX_COEFFICIENTS:
		p->matrix_coefficients = ctrl->value;
		break;
	default:
		mfc_err_ctx("Invalid control: 0x%08x\n", ctrl->id);
		ret = -EINVAL;
	}

	return ret;
}

static int mfc_enc_set_ctrl_val(struct s5p_mfc_ctx *ctx, struct v4l2_control *ctrl)
{
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_enc_params *p = &enc->params;
	struct s5p_mfc_ctx_ctrl *ctx_ctrl;
	int ret = 0;
	int found = 0;
	int index = 0;

	switch (ctrl->id) {
	case V4L2_CID_CACHEABLE:
		mfc_debug(5, "it is supported only V4L2_MEMORY_MMAP\n");
		break;
	case V4L2_CID_MPEG_VIDEO_QOS_RATIO:
		ctx->qos_ratio = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_H263_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_VP8_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_VP9_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_H263_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_VP8_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_VP9_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP_P:
	case V4L2_CID_MPEG_VIDEO_H263_MAX_QP_P:
	case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_P:
	case V4L2_CID_MPEG_VIDEO_VP8_MAX_QP_P:
	case V4L2_CID_MPEG_VIDEO_VP9_MAX_QP_P:
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_P:
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP_P:
	case V4L2_CID_MPEG_VIDEO_H263_MIN_QP_P:
	case V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_P:
	case V4L2_CID_MPEG_VIDEO_VP8_MIN_QP_P:
	case V4L2_CID_MPEG_VIDEO_VP9_MIN_QP_P:
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_P:
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP_B:
	case V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_B:
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_B:
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP_B:
	case V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_B:
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_B:
	case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG:
	case V4L2_CID_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE:
	case V4L2_CID_MPEG_MFC51_VIDEO_I_PERIOD_CH:
	case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH:
	case V4L2_CID_MPEG_MFC51_VIDEO_BIT_RATE_CH:
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_CH:
	case V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_CH:
	case V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_CH:
	case V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_CH:
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
	case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
	case V4L2_CID_MPEG_MFC_H264_MARK_LTR:
	case V4L2_CID_MPEG_MFC_H264_USE_LTR:
	case V4L2_CID_MPEG_MFC_H264_BASE_PRIORITY:
	case V4L2_CID_MPEG_MFC_CONFIG_QP:
	case V4L2_CID_MPEG_VIDEO_ROI_CONTROL:
	case V4L2_CID_MPEG_VIDEO_YSUM:
	case V4L2_CID_MPEG_VIDEO_RATIO_OF_INTRA:
		list_for_each_entry(ctx_ctrl, &ctx->ctrls, list) {
			if (!(ctx_ctrl->type & MFC_CTRL_TYPE_SET))
				continue;

			if (ctx_ctrl->id == ctrl->id) {
				ctx_ctrl->has_new = 1;
				ctx_ctrl->val = ctrl->value;
				if (ctx_ctrl->id == \
					V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH) {
					ctx_ctrl->val &= ~(0xFFFF << 16);
					ctx_ctrl->val |= ctx_ctrl->val << 16;
					ctx_ctrl->val &= ~(0xFFFF);
					ctx_ctrl->val |= p->rc_frame_delta & 0xFFFF;
				}
				if (((ctx_ctrl->id == \
					V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_CH) ||
					(ctx_ctrl->id == \
					V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_CH) ||
					(ctx_ctrl->id == \
					V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_CH) ||
					(ctx_ctrl->id == \
					V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_CH)) &&
					(enc->sh_handle_svc.fd == -1)) {
						enc->sh_handle_svc.fd = ctrl->value;
						if (s5p_mfc_mem_get_user_shared_handle(ctx,
									&enc->sh_handle_svc)) {
							enc->sh_handle_svc.fd = -1;
							return -EINVAL;
						}
				}
				if (ctx_ctrl->id == V4L2_CID_MPEG_MFC51_VIDEO_I_PERIOD_CH &&
						p->i_frm_ctrl_mode) {
					ctx_ctrl->val = ctx_ctrl->val * (p->num_b_frame + 1);
					if (ctx_ctrl->val >= 0x3FFFFFFF) {
						mfc_info_ctx("I frame interval is bigger than max: %d\n",
								ctx_ctrl->val);
						ctx_ctrl->val = 0x3FFFFFFF;
					}
				}
				if (ctx_ctrl->id == V4L2_CID_MPEG_VIDEO_H264_LEVEL)
					ctx_ctrl->val = mfc_enc_h264_level((enum v4l2_mpeg_video_h264_level)(ctrl->value));
				if (ctx_ctrl->id == V4L2_CID_MPEG_VIDEO_H264_PROFILE)
					ctx_ctrl->val = mfc_enc_h264_profile(ctx, (enum v4l2_mpeg_video_h264_profile)(ctrl->value));
				if (ctx_ctrl->id == \
					V4L2_CID_MPEG_VIDEO_ROI_CONTROL) {
					enc->sh_handle_roi.fd = ctrl->value;
					if (s5p_mfc_mem_get_user_shared_handle(ctx,
								&enc->sh_handle_roi)) {
						enc->sh_handle_roi.fd = -1;
						return -EINVAL;
					}
					index = enc->roi_index;
					memcpy(&enc->roi_info[index],
							enc->sh_handle_roi.vaddr,
							sizeof(struct mfc_enc_roi_info));
					if (copy_from_user(enc->roi_buf[index].vaddr,
							enc->roi_info[index].addr,
							enc->roi_info[index].size))
						return -EFAULT;
				}

				found = 1;
				break;
			}
		}

		if (!found) {
			mfc_err_ctx("Invalid control: 0x%08x\n", ctrl->id);
			return -EINVAL;
		}
		break;
	default:
		ret = mfc_enc_set_param(ctx, ctrl);
		break;
	}

	return ret;
}

static int vidioc_s_ctrl(struct file *file, void *priv,
			 struct v4l2_control *ctrl)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret = 0;

	mfc_debug_enter();

	ret = mfc_enc_check_ctrl_val(ctx, ctrl);
	if (ret != 0)
		return ret;

	ret = mfc_enc_set_ctrl_val(ctx, ctrl);

	mfc_debug_leave();

	return ret;
}

static int vidioc_g_ext_ctrls(struct file *file, void *priv,
			      struct v4l2_ext_controls *f)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct v4l2_ext_control *ext_ctrl;
	struct v4l2_control ctrl;
	int i;
	int ret = 0;

	if (f->which != V4L2_CTRL_CLASS_MPEG)
		return -EINVAL;

	for (i = 0; i < f->count; i++) {
		ext_ctrl = (f->controls + i);

		ctrl.id = ext_ctrl->id;

		ret = mfc_enc_get_ctrl_val(ctx, &ctrl);
		if (ret == 0) {
			ext_ctrl->value = ctrl.value;
		} else {
			f->error_idx = i;
			break;
		}

		mfc_debug(2, "[%d] id: 0x%08x, value: %d\n", i, ext_ctrl->id, ext_ctrl->value);
	}

	return ret;
}

static int vidioc_s_ext_ctrls(struct file *file, void *priv,
				struct v4l2_ext_controls *f)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct v4l2_ext_control *ext_ctrl;
	struct v4l2_control ctrl;
	int i;
	int ret = 0;

	if (f->which != V4L2_CTRL_CLASS_MPEG)
		return -EINVAL;

	for (i = 0; i < f->count; i++) {
		ext_ctrl = (f->controls + i);

		ctrl.id = ext_ctrl->id;
		ctrl.value = ext_ctrl->value;

		ret = mfc_enc_check_ctrl_val(ctx, &ctrl);
		if (ret != 0) {
			f->error_idx = i;
			break;
		}

		ret = mfc_enc_set_param(ctx, &ctrl);
		if (ret != 0) {
			f->error_idx = i;
			break;
		}

		mfc_debug(2, "[%d] id: 0x%08x, value: %d\n", i, ext_ctrl->id, ext_ctrl->value);
	}

	return ret;
}

static int vidioc_try_ext_ctrls(struct file *file, void *priv,
				struct v4l2_ext_controls *f)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct v4l2_ext_control *ext_ctrl;
	struct v4l2_control ctrl;
	int i;
	int ret = 0;

	if (f->which != V4L2_CTRL_CLASS_MPEG)
		return -EINVAL;

	for (i = 0; i < f->count; i++) {
		ext_ctrl = (f->controls + i);

		ctrl.id = ext_ctrl->id;
		ctrl.value = ext_ctrl->value;

		ret = mfc_enc_check_ctrl_val(ctx, &ctrl);
		if (ret != 0) {
			f->error_idx = i;
			break;
		}

		mfc_debug(2, "[%d] id: 0x%08x, value: %d\n", i, ext_ctrl->id, ext_ctrl->value);
	}

	return ret;
}

static const struct v4l2_ioctl_ops s5p_mfc_enc_ioctl_ops = {
	.vidioc_querycap		= vidioc_querycap,
	.vidioc_enum_fmt_vid_cap	= vidioc_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_cap_mplane	= vidioc_enum_fmt_vid_cap_mplane,
	.vidioc_enum_fmt_vid_out	= vidioc_enum_fmt_vid_out,
	.vidioc_enum_fmt_vid_out_mplane	= vidioc_enum_fmt_vid_out_mplane,
	.vidioc_g_fmt_vid_cap_mplane	= vidioc_g_fmt,
	.vidioc_g_fmt_vid_out_mplane	= vidioc_g_fmt,
	.vidioc_try_fmt_vid_cap_mplane	= vidioc_try_fmt,
	.vidioc_try_fmt_vid_out_mplane	= vidioc_try_fmt,
	.vidioc_s_fmt_vid_cap_mplane	= vidioc_s_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_out_mplane	= vidioc_s_fmt_vid_out_mplane,
	.vidioc_reqbufs			= vidioc_reqbufs,
	.vidioc_querybuf		= vidioc_querybuf,
	.vidioc_qbuf			= vidioc_qbuf,
	.vidioc_dqbuf			= vidioc_dqbuf,
	.vidioc_streamon		= vidioc_streamon,
	.vidioc_streamoff		= vidioc_streamoff,
	.vidioc_queryctrl		= vidioc_queryctrl,
	.vidioc_g_ctrl			= vidioc_g_ctrl,
	.vidioc_s_ctrl			= vidioc_s_ctrl,
	.vidioc_g_ext_ctrls		= vidioc_g_ext_ctrls,
	.vidioc_s_ext_ctrls		= vidioc_s_ext_ctrls,
	.vidioc_try_ext_ctrls		= vidioc_try_ext_ctrls,
};

const struct v4l2_ioctl_ops *s5p_mfc_get_enc_v4l2_ioctl_ops(void)
{
	return &s5p_mfc_enc_ioctl_ops;
}
