/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_dec.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "s5p_mfc_dec.h"
#include "s5p_mfc_dec_internal.h"

#include "s5p_mfc_hwlock.h"
#include "s5p_mfc_opr.h"
#include "s5p_mfc_sync.h"

#include "s5p_mfc_qos.h"
#include "s5p_mfc_queue.h"
#include "s5p_mfc_utils.h"
#include "s5p_mfc_buf.h"
#include "s5p_mfc_mem.h"

#define MAX_FRAME_SIZE		(2*1024*1024)

/* Find selected format description */
static struct s5p_mfc_fmt *mfc_dec_find_format(struct v4l2_format *f, unsigned int t)
{
	unsigned long i;

	for (i = 0; i < NUM_FORMATS; i++) {
		if (dec_formats[i].fourcc == f->fmt.pix_mp.pixelformat &&
		    dec_formats[i].type == t)
			return (struct s5p_mfc_fmt *)&dec_formats[i];
	}

	return NULL;
}

static struct v4l2_queryctrl *mfc_dec_get_ctrl(int id)
{
	unsigned long i;

	for (i = 0; i < NUM_CTRLS; ++i)
		if (id == controls[i].id)
			return &controls[i];

	return NULL;
}

/* Check whether a ctrl value if correct */
static int mfc_dec_check_ctrl_val(struct s5p_mfc_ctx *ctx, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl *c;

	c = mfc_dec_get_ctrl(ctrl->id);
	if (!c)
		return -EINVAL;

	if (ctrl->value < c->minimum || ctrl->value > c->maximum
		|| (c->step != 0 && ctrl->value % c->step != 0)) {
		mfc_err_ctx("Invalid control value (%#x)\n", ctrl->value);
		return -ERANGE;
	}

	return 0;
}

/* Query capabilities of the device */
static int vidioc_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	strncpy(cap->driver, "MFC", sizeof(cap->driver) - 1);
	strncpy(cap->card, "decoder", sizeof(cap->card) - 1);
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

/* Enumerate format */
static int mfc_dec_enum_fmt(struct v4l2_fmtdesc *f, bool mplane, bool out)
{
	struct s5p_mfc_fmt *fmt;
	unsigned long i, j = 0;

	for (i = 0; i < NUM_FORMATS; ++i) {
		if (mplane && dec_formats[i].mem_planes == 1)
			continue;
		else if (!mplane && dec_formats[i].mem_planes > 1)
			continue;
		if (out && dec_formats[i].type != MFC_FMT_DEC)
			continue;
		else if (!out && dec_formats[i].type != MFC_FMT_RAW)
			continue;

		if (j == f->index)
			break;
		++j;
	}
	if (i == NUM_FORMATS)
		return -EINVAL;

	fmt = &dec_formats[i];
	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;

	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void *pirv,
							struct v4l2_fmtdesc *f)
{
	return mfc_dec_enum_fmt(f, false, false);
}

static int vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *pirv,
							struct v4l2_fmtdesc *f)
{
	return mfc_dec_enum_fmt(f, true, false);
}

static int vidioc_enum_fmt_vid_out(struct file *file, void *prov,
							struct v4l2_fmtdesc *f)
{
	return mfc_dec_enum_fmt(f, false, true);
}

static int vidioc_enum_fmt_vid_out_mplane(struct file *file, void *prov,
							struct v4l2_fmtdesc *f)
{
	return mfc_dec_enum_fmt(f, true, true);
}

static void mfc_dec_change_format(struct s5p_mfc_ctx *ctx)
{
	u32 org_fmt = ctx->dst_fmt->fourcc;

	if (ctx->is_10bit && ctx->is_422format) {
		switch (org_fmt) {
		case V4L2_PIX_FMT_NV16M_S10B:
		case V4L2_PIX_FMT_NV61M_S10B:
		case V4L2_PIX_FMT_NV16M_P210:
		case V4L2_PIX_FMT_NV61M_P210:
			break;
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_NV16M:
		case V4L2_PIX_FMT_NV12M_S10B:
		case V4L2_PIX_FMT_NV12M_P010:
			ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[14];
			break;
		case V4L2_PIX_FMT_NV21M:
		case V4L2_PIX_FMT_NV61M:
		case V4L2_PIX_FMT_NV21M_S10B:
		case V4L2_PIX_FMT_NV21M_P010:
			ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[17];
			break;
		default:
			ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[14];
			break;
		}
		ctx->raw_buf.num_planes = 2;
	} else if (ctx->is_10bit && !ctx->is_422format) {
		if (ctx->dst_fmt->mem_planes == 1) {
			/* YUV420 only supports the single plane */
			ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[8];
		} else {
			switch (org_fmt) {
			case V4L2_PIX_FMT_NV12M_S10B:
			case V4L2_PIX_FMT_NV21M_S10B:
			case V4L2_PIX_FMT_NV12M_P010:
			case V4L2_PIX_FMT_NV21M_P010:
				break;
			case V4L2_PIX_FMT_NV12M:
			case V4L2_PIX_FMT_NV16M:
			case V4L2_PIX_FMT_NV16M_S10B:
			case V4L2_PIX_FMT_NV16M_P210:
				ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[7];
				break;
			case V4L2_PIX_FMT_NV21M:
			case V4L2_PIX_FMT_NV61M:
			case V4L2_PIX_FMT_NV61M_S10B:
			case V4L2_PIX_FMT_NV61M_P210:
				ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[11];
				break;
			default:
				ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[7];
				break;
			}
		}
		ctx->raw_buf.num_planes = 2;
	} else if (!ctx->is_10bit && ctx->is_422format) {
		switch (org_fmt) {
		case V4L2_PIX_FMT_NV16M:
		case V4L2_PIX_FMT_NV61M:
			break;
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_NV12M_S10B:
		case V4L2_PIX_FMT_NV16M_S10B:
		case V4L2_PIX_FMT_NV12M_P010:
		case V4L2_PIX_FMT_NV16M_P210:
			ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[13];
			break;
		case V4L2_PIX_FMT_NV21M:
		case V4L2_PIX_FMT_NV21M_S10B:
		case V4L2_PIX_FMT_NV61M_S10B:
		case V4L2_PIX_FMT_NV21M_P010:
		case V4L2_PIX_FMT_NV61M_P210:
			ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[16];
			break;
		default:
			ctx->dst_fmt = (struct s5p_mfc_fmt *)&dec_formats[13];
			break;
		}
		ctx->raw_buf.num_planes = 2;
	}

	if (org_fmt != ctx->dst_fmt->fourcc)
		mfc_info_ctx("format is changed to %s\n", ctx->dst_fmt->name);
}

/* Get format */
static int vidioc_g_fmt_vid_cap_mplane(struct file *file, void *priv,
						struct v4l2_format *f)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_dec *dec;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	struct s5p_mfc_raw_info *raw;
	int i;

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}
	mfc_debug_enter();
	mfc_debug(2, "f->type = %d ctx->state = %d\n", f->type, ctx->state);

	if (ctx->state == MFCINST_GOT_INST ||
	    ctx->state == MFCINST_RES_CHANGE_FLUSH ||
	    ctx->state == MFCINST_RES_CHANGE_END) {
		/* If the MFC is parsing the header,
		 * so wait until it is finished */
		if (s5p_mfc_wait_for_done_ctx(ctx,
				S5P_FIMV_R2H_CMD_SEQ_DONE_RET)) {
				mfc_err_dev("header parsing failed\n");
				return -EAGAIN;
		}
	}

	if (ctx->state >= MFCINST_HEAD_PARSED &&
	    ctx->state < MFCINST_ABORT) {
		/* This is run on CAPTURE (deocde output) */

		/* only NV16(61) format is supported for 422 format */
		/* only 2 plane is supported for 10bit */
		mfc_dec_change_format(ctx);

		raw = &ctx->raw_buf;
		/* Width and height are set to the dimensions
		   of the movie, the buffer is bigger and
		   further processing stages should crop to this
		   rectangle. */
		s5p_mfc_dec_calc_dpb_size(ctx);

		pix_mp->width = ctx->img_width;
		pix_mp->height = ctx->img_height;
		pix_mp->num_planes = ctx->dst_fmt->mem_planes;

		if (dec->is_interlaced)
			pix_mp->field = V4L2_FIELD_INTERLACED;
		else
			pix_mp->field = V4L2_FIELD_NONE;

		/* Set pixelformat to the format in which MFC
		   outputs the decoded frame */
		pix_mp->pixelformat = ctx->dst_fmt->fourcc;
		for (i = 0; i < ctx->dst_fmt->mem_planes; i++) {
			pix_mp->plane_fmt[i].bytesperline = raw->stride[i];
			if (ctx->dst_fmt->mem_planes == 1) {
				pix_mp->plane_fmt[i].sizeimage = raw->total_plane_size;
			} else {
				if (ctx->is_10bit)
					pix_mp->plane_fmt[i].sizeimage = raw->plane_size[i]
						+ raw->plane_size_2bits[i];
				else
					pix_mp->plane_fmt[i].sizeimage = raw->plane_size[i];
			}
		}
	}

	mfc_debug_leave();

	return 0;
}

static int vidioc_g_fmt_vid_out_mplane(struct file *file, void *priv,
						struct v4l2_format *f)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_dec *dec;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;

	mfc_debug_enter();

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}
	mfc_debug(2, "f->type = %d ctx->state = %d\n", f->type, ctx->state);

	/* This is run on OUTPUT
	   The buffer contains compressed image
	   so width and height have no meaning */
	pix_mp->width = 0;
	pix_mp->height = 0;
	pix_mp->field = V4L2_FIELD_NONE;
	pix_mp->plane_fmt[0].bytesperline = dec->src_buf_size;
	pix_mp->plane_fmt[0].sizeimage = dec->src_buf_size;
	pix_mp->pixelformat = ctx->src_fmt->fourcc;
	pix_mp->num_planes = ctx->src_fmt->mem_planes;

	mfc_debug_leave();

	return 0;
}

/* Try format */
static int vidioc_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct s5p_mfc_dev *dev = video_drvdata(file);
	struct s5p_mfc_fmt *fmt;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}
	mfc_debug(2, "Type is %d\n", f->type);
	if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		fmt = mfc_dec_find_format(f, MFC_FMT_DEC);
		if (!fmt) {
			mfc_err_dev("Unsupported format for source.\n");
			return -EINVAL;
		}
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		fmt = mfc_dec_find_format(f, MFC_FMT_RAW);

		if (!fmt) {
			mfc_err_dev("Unsupported format for destination.\n");
			return -EINVAL;
		}

		if (fmt->fourcc == V4L2_PIX_FMT_NV12MT) {
			mfc_err_dev("Not supported format: NV12MT\n");
			return -EINVAL;
		}
		else if(fmt->fourcc == V4L2_PIX_FMT_NV12MT_16X16) {
			mfc_err_dev("Not supported format: NV12MT_16X16\n");
			return -EINVAL;
		}
	}

	return 0;
}

/* Set format */
static int vidioc_s_fmt_vid_cap_mplane(struct file *file, void *priv,
							struct v4l2_format *f)
{
	struct s5p_mfc_dev *dev = video_drvdata(file);
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret = 0;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	if (ctx->vq_dst.streaming) {
		mfc_err_ctx("queue busy\n");
		return -EBUSY;
	}

	ret = vidioc_try_fmt(file, priv, f);
	if (ret)
		return ret;

	ctx->dst_fmt = mfc_dec_find_format(f, MFC_FMT_RAW);
	if (!ctx->dst_fmt) {
		mfc_err_ctx("Unsupported format for destination.\n");
		return -EINVAL;
	}
	ctx->raw_buf.num_planes = ctx->dst_fmt->num_planes;
	mfc_info_ctx("Dec output pixelformat : %s\n", ctx->dst_fmt->name);

	mfc_debug_leave();

	return 0;
}

static int mfc_force_close_inst(struct s5p_mfc_dev *dev, struct s5p_mfc_ctx *ctx)
{
	if (ctx->inst_no == MFC_NO_INSTANCE_SET)
		return 0;

	s5p_mfc_change_state(ctx, MFCINST_RETURN_INST);
	s5p_mfc_set_bit(ctx->num, &dev->work_bits);
	s5p_mfc_clean_ctx_int_flags(ctx);
	if (s5p_mfc_just_run(dev, ctx->num)) {
		mfc_err_ctx("Failed to run MFC.\n");
		s5p_mfc_release_hwlock_ctx(ctx);
		s5p_mfc_cleanup_work_bit_and_try_run(ctx);
		return -EIO;
	}

	/* Wait until instance is returned or timeout occured */
	if (s5p_mfc_wait_for_done_ctx(ctx,
				S5P_FIMV_R2H_CMD_CLOSE_INSTANCE_RET)) {
		mfc_err_ctx("Waiting for CLOSE_INSTANCE timed out\n");
		s5p_mfc_release_hwlock_ctx(ctx);
		s5p_mfc_cleanup_work_bit_and_try_run(ctx);
		return -EIO;
	}

	/* Free resources */
	s5p_mfc_release_instance_context(ctx);
	s5p_mfc_change_state(ctx, MFCINST_INIT);

	return 0;
}

static int vidioc_s_fmt_vid_out_mplane(struct file *file, void *priv,
							struct v4l2_format *f)
{
	struct s5p_mfc_dev *dev = video_drvdata(file);
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_dec *dec;
	int ret = 0;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}

	if (ctx->vq_src.streaming) {
		mfc_err_ctx("queue busy\n");
		return -EBUSY;
	}

	ret = vidioc_try_fmt(file, priv, f);
	if (ret)
		return ret;

	ctx->src_fmt = mfc_dec_find_format(f, MFC_FMT_DEC);
	ctx->codec_mode = ctx->src_fmt->codec_mode;
	mfc_info_ctx("Dec input codec(%d): %s\n",
			ctx->codec_mode, ctx->src_fmt->name);
	ctx->pix_format = pix_mp->pixelformat;
	if ((pix_mp->width > 0) && (pix_mp->height > 0)) {
		ctx->img_height = pix_mp->height;
		ctx->img_width = pix_mp->width;
	}
	/* As this buffer will contain compressed data, the size is set
	 * to the maximum size. */
	if (pix_mp->plane_fmt[0].sizeimage)
		dec->src_buf_size = pix_mp->plane_fmt[0].sizeimage;
	else
		dec->src_buf_size = MAX_FRAME_SIZE;
	mfc_debug(2, "sizeimage: %d\n", pix_mp->plane_fmt[0].sizeimage);
	pix_mp->plane_fmt[0].bytesperline = 0;
	MFC_TRACE_CTX_HWLOCK("**DEC s_fmt\n");
	ret = s5p_mfc_get_hwlock_ctx(ctx);
	if (ret < 0) {
		mfc_err_ctx("Failed to get hwlock.\n");
		return -EBUSY;
	}

	/* In case of calling s_fmt twice or more */
	ret = mfc_force_close_inst(dev, ctx);
	if (ret) {
		mfc_err_ctx("Failed to close already opening instance\n");
		return -EIO;
	}

	ret = s5p_mfc_alloc_instance_context(ctx);
	if (ret) {
		mfc_err_ctx("Failed to allocate dec instance[%d] buffers.\n",
				ctx->num);
		s5p_mfc_release_hwlock_ctx(ctx);
		return -ENOMEM;
	}

	s5p_mfc_set_bit(ctx->num, &dev->work_bits);
	ret = s5p_mfc_just_run(dev, ctx->num);
	if (ret) {
		mfc_err_ctx("Failed to run MFC.\n");
		s5p_mfc_release_hwlock_ctx(ctx);
		s5p_mfc_cleanup_work_bit_and_try_run(ctx);
		s5p_mfc_release_instance_context(ctx);
		return -EIO;
	}

	if (s5p_mfc_wait_for_done_ctx(ctx,
			S5P_FIMV_R2H_CMD_OPEN_INSTANCE_RET)) {
		s5p_mfc_release_hwlock_ctx(ctx);
		s5p_mfc_cleanup_work_bit_and_try_run(ctx);
		s5p_mfc_release_instance_context(ctx);
		return -EIO;
	}

	s5p_mfc_release_hwlock_ctx(ctx);

	mfc_debug(2, "Got instance number: %d\n", ctx->inst_no);

	if (s5p_mfc_dec_ctx_ready(ctx))
		s5p_mfc_set_bit(ctx->num, &dev->work_bits);
	if (s5p_mfc_is_work_to_do(dev))
		queue_work(dev->butler_wq, &dev->butler_work);

	mfc_debug_leave();

	return 0;
}

/* Reqeust buffers */
static int vidioc_reqbufs(struct file *file, void *priv,
		struct v4l2_requestbuffers *reqbufs)
{
	struct s5p_mfc_dev *dev = video_drvdata(file);
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_dec *dec;
	int ret = 0;

	mfc_debug_enter();
	mfc_info_ctx("Memory type: %d\n", reqbufs->memory);

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}

	if (reqbufs->memory == V4L2_MEMORY_MMAP) {
		mfc_err_ctx("Not supported memory type (%d).\n", reqbufs->memory);
		return -EINVAL;
	}

	if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		/* Can only request buffers after
		   an instance has been opened.*/
		if (ctx->state == MFCINST_GOT_INST) {
			if (reqbufs->count == 0) {
				mfc_debug(2, "Freeing buffers.\n");
				ret = vb2_reqbufs(&ctx->vq_src, reqbufs);
				ctx->output_state = QUEUE_FREE;
				return ret;
			}

			/* Decoding */
			if (ctx->output_state != QUEUE_FREE) {
				mfc_err_ctx("Bufs have already been requested.\n");
				return -EINVAL;
			}

			ret = vb2_reqbufs(&ctx->vq_src, reqbufs);
			if (ret) {
				mfc_err_ctx("vb2_reqbufs on output failed.\n");
				return ret;
			}

			ctx->output_state = QUEUE_BUFS_REQUESTED;
		}
	} else if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (reqbufs->count == 0) {
			mfc_debug(2, "Freeing buffers.\n");
			ret = vb2_reqbufs(&ctx->vq_dst, reqbufs);
			s5p_mfc_release_codec_buffers(ctx);
			ctx->capture_state = QUEUE_FREE;
			return ret;
		}

		dec->dst_memtype = reqbufs->memory;

		if (ctx->capture_state != QUEUE_FREE) {
			mfc_err_ctx("Bufs have already been requested.\n");
			return -EINVAL;
		}

		ret = vb2_reqbufs(&ctx->vq_dst, reqbufs);
		if (ret) {
			mfc_err_ctx("vb2_reqbufs on capture failed.\n");
			return ret;
		}

		if (reqbufs->count < ctx->dpb_count) {
			mfc_err_ctx("Not enough buffers allocated.\n");
			reqbufs->count = 0;
			vb2_reqbufs(&ctx->vq_dst, reqbufs);
			return -ENOMEM;
		}

		dec->total_dpb_count = reqbufs->count;

		ret = s5p_mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate decoding buffers.\n");
			reqbufs->count = 0;
			vb2_reqbufs(&ctx->vq_dst, reqbufs);
			return -ENOMEM;
		}

		ctx->capture_state = QUEUE_BUFS_REQUESTED;

		if (s5p_mfc_dec_ctx_ready(ctx))
			s5p_mfc_set_bit(ctx->num, &dev->work_bits);

		s5p_mfc_try_run(dev);
	}

	mfc_debug_leave();

	return ret;
}

/* Query buffer */
static int vidioc_querybuf(struct file *file, void *priv,
						   struct v4l2_buffer *buf)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret;

	mfc_debug_enter();

	mfc_debug(2, "State: %d, buf->type: %d\n", ctx->state, buf->type);
	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = vb2_querybuf(&ctx->vq_dst, buf);
		if (ret != 0) {
			mfc_err_dev("dec dst: error in vb2_querybuf()\n");
			return ret;
		}
	} else if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = vb2_querybuf(&ctx->vq_src, buf);
		if (ret != 0) {
			mfc_err_dev("dec src: error in vb2_querybuf()\n");
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
	int ret = -EINVAL;

	mfc_debug_enter();

	mfc_debug(2, "Enqueued buf: %d, (type = %d)\n", buf->index, buf->type);
	if (ctx->state == MFCINST_ERROR) {
		mfc_err_ctx("Call on QBUF after unrecoverable error.\n");
		return -EIO;
	}

	if (V4L2_TYPE_IS_MULTIPLANAR(buf->type) && !buf->length) {
		mfc_err_ctx("multiplanar but length is zero\n");
		return -EIO;
	}

	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (buf->m.planes[0].bytesused > buf->m.planes[0].length) {
			mfc_err_ctx("data size (%d) must be less than "
					"buffer size(%d)\n",
					buf->m.planes[0].bytesused,
					buf->m.planes[0].length);
			return -EIO;
		}

		s5p_mfc_qos_update_framerate(ctx);

		if (!buf->m.planes[0].bytesused) {
			buf->m.planes[0].bytesused = buf->m.planes[0].length;
			mfc_debug(2, "Src input size zero, changed to buf size %d\n",
					buf->m.planes[0].bytesused);
		} else {
			mfc_debug(2, "Src input size = %d\n", buf->m.planes[0].bytesused);
		}
		ret = vb2_qbuf(&ctx->vq_src, buf);
	} else {
		ret = vb2_qbuf(&ctx->vq_dst, buf);
		mfc_debug(2, "End of enqueue(%d) : %d\n", buf->index, ret);
	}

	mfc_debug_leave();
	return ret;
}

/* Dequeue a buffer */
static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	struct dec_dpb_ref_info *dstBuf, *srcBuf;
	int ret;
	int ncount = 0;

	mfc_debug_enter();
	mfc_debug(2, "Addr: %p %p %p Type: %d\n", &ctx->vq_src, buf, buf->m.planes,
								buf->type);
	if (ctx->state == MFCINST_ERROR) {
		mfc_err_ctx("Call on DQBUF after unrecoverable error.\n");
		return -EIO;
	}
	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = vb2_dqbuf(&ctx->vq_src, buf, file->f_flags & O_NONBLOCK);
	} else {
		ret = vb2_dqbuf(&ctx->vq_dst, buf, file->f_flags & O_NONBLOCK);

		if (buf->index >= MFC_MAX_DPBS) {
			mfc_err_ctx("buffer index[%d] range over\n", buf->index);
			return -EINVAL;
		}

		/* Memcpy from dec->ref_info to shared memory */
		srcBuf = &dec->ref_info[buf->index];
		for (ncount = 0; ncount < MFC_MAX_DPBS; ncount++) {
			if (srcBuf->dpb[ncount].fd[0] == MFC_INFO_INIT_FD)
				break;
			mfc_debug(2, "DQ index[%d] Released FD = %d\n",
					buf->index, srcBuf->dpb[ncount].fd[0]);
		}

		if (dec->sh_handle.vaddr != NULL) {
			dstBuf = (struct dec_dpb_ref_info *)
					dec->sh_handle.vaddr + buf->index;
			memcpy(dstBuf, srcBuf, sizeof(struct dec_dpb_ref_info));
			dstBuf->index = buf->index;
		}
	}
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

	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = vb2_streamon(&ctx->vq_src, type);
	} else if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = vb2_streamon(&ctx->vq_dst, type);

		if (!ret)
			s5p_mfc_qos_on(ctx);
	} else {
		mfc_err_ctx("unknown v4l2 buffer type\n");
	}

	mfc_debug(2, "src: %d, dst: %d,  state = %d, dpb_count = %d\n",
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
	int ret = -EINVAL;

	mfc_debug_enter();

	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		s5p_mfc_qos_reset_last_framerate(ctx);

		ret = vb2_streamoff(&ctx->vq_src, type);
	} else if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = vb2_streamoff(&ctx->vq_dst, type);
		if (!ret)
			s5p_mfc_qos_off(ctx);
	} else {
		mfc_err_ctx("unknown v4l2 buffer type\n");
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

	c = mfc_dec_get_ctrl(qc->id);
	if (!c)
		return -EINVAL;
	*qc = *c;
	return 0;
}

static int mfc_dec_ext_info(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	int val = 0;

	val |= DEC_SET_DYNAMIC_DPB;
	if (FW_SUPPORT_SKYPE(dev))
		val |= DEC_SET_SKYPE_FLAG;

	return val;
}

/* Get ctrl */
static int mfc_dec_get_ctrl_val(struct s5p_mfc_ctx *ctx, struct v4l2_control *ctrl)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_ctx_ctrl *ctx_ctrl;
	int found = 0;

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

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_DECODER_MPEG4_DEBLOCK_FILTER:
		ctrl->value = dec->loop_filter_mpeg4;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_DECODER_H264_DISPLAY_DELAY:
		ctrl->value = dec->display_delay;
		break;
	case V4L2_CID_CACHEABLE:
		mfc_debug(5, "it is supported only V4L2_MEMORY_MMAP\n");
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		if (ctx->state >= MFCINST_HEAD_PARSED &&
		    ctx->state < MFCINST_ABORT) {
			ctrl->value = ctx->dpb_count;
			break;
		} else if (ctx->state != MFCINST_INIT) {
			mfc_err_ctx("Decoding not initialised.\n");
			return -EINVAL;
		}

		/* Should wait for the header to be parsed */
		if (s5p_mfc_wait_for_done_ctx(ctx,
				S5P_FIMV_R2H_CMD_SEQ_DONE_RET)) {
			s5p_mfc_cleanup_work_bit_and_try_run(ctx);
			return -EIO;
		}

		if (ctx->state >= MFCINST_HEAD_PARSED &&
		    ctx->state < MFCINST_ABORT) {
			ctrl->value = ctx->dpb_count;
		} else {
			mfc_err_ctx("Decoding not initialised.\n");
			return -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_VIDEO_DECODER_SLICE_INTERFACE:
		ctrl->value = dec->slice_enable;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_PACKED_PB:
		/* Not used */
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_CRC_ENABLE:
		ctrl->value = dec->crc_enable;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE:
		if (ctx->is_dpb_realloc && ctx->state == MFCINST_HEAD_PARSED)
			ctrl->value = MFCSTATE_DEC_S3D_REALLOC;
		else if (ctx->state == MFCINST_RES_CHANGE_FLUSH
				|| ctx->state == MFCINST_RES_CHANGE_END
				|| ctx->state == MFCINST_HEAD_PARSED)
			ctrl->value = MFCSTATE_DEC_RES_DETECT;
		else if (ctx->state == MFCINST_FINISHING)
			ctrl->value = MFCSTATE_DEC_TERMINATING;
		else
			ctrl->value = MFCSTATE_PROCESSING;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING:
		ctrl->value = dec->sei_parse;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_I_FRAME_DECODING:
		ctrl->value = dec->idr_decoding;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE:
		ctrl->value = s5p_mfc_qos_get_framerate(ctx);
		break;
	case V4L2_CID_MPEG_MFC_GET_VERSION_INFO:
		ctrl->value = s5p_mfc_version(dev);
		break;
	case V4L2_CID_MPEG_VIDEO_QOS_RATIO:
		ctrl->value = ctx->qos_ratio;
		break;
	case V4L2_CID_MPEG_MFC_SET_DYNAMIC_DPB_MODE:
		ctrl->value = dec->is_dynamic_dpb;
		break;
	case V4L2_CID_MPEG_MFC_GET_EXT_INFO:
		ctrl->value = mfc_dec_ext_info(ctx);
		break;
	case V4L2_CID_MPEG_MFC_GET_10BIT_INFO:
		ctrl->value = ctx->is_10bit;
		break;
	case V4L2_CID_MPEG_MFC_GET_DRIVER_INFO:
		ctrl->value = MFC_DRIVER_INFO;
		break;
	default:
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
	}

	mfc_debug_leave();

	return 0;
}

/* Get a ctrl */
static int vidioc_g_ctrl(struct file *file, void *priv,
			struct v4l2_control *ctrl)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	int ret = 0;

	mfc_debug_enter();
	ret = mfc_dec_get_ctrl_val(ctx, ctrl);
	mfc_debug_leave();

	return ret;
}

/* Set a ctrl */
static int vidioc_s_ctrl(struct file *file, void *priv,
			 struct v4l2_control *ctrl)
{
	struct s5p_mfc_dev *dev = video_drvdata(file);
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_ctx_ctrl *ctx_ctrl;
	int ret = 0;
	int found = 0;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}

	ret = mfc_dec_check_ctrl_val(ctx, ctrl);
	if (ret)
		return ret;

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_DECODER_MPEG4_DEBLOCK_FILTER:
		dec->loop_filter_mpeg4 = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_DECODER_H264_DISPLAY_DELAY:
		dec->display_delay = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_DECODER_SLICE_INTERFACE:
		dec->slice_enable = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_PACKED_PB:
		/* Not used */
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_CRC_ENABLE:
		dec->crc_enable = ctrl->value;
		break;
	case V4L2_CID_CACHEABLE:
		mfc_debug(5, "it is supported only V4L2_MEMORY_MMAP\n");
		break;
	case V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING:
		dec->sei_parse = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC51_VIDEO_I_FRAME_DECODING:
		dec->idr_decoding = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_DECODER_IMMEDIATE_DISPLAY:
		dec->immediate_display = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_DECODER_DECODING_TIMESTAMP_MODE:
		dec->is_dts_mode = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_DECODER_WAIT_DECODING_START:
		ctx->wait_state = ctrl->value;
		break;
	case V4L2_CID_MPEG_MFC_SET_DUAL_DPB_MODE:
		mfc_err_dev("not supported CID: 0x%x\n",ctrl->id);
		break;
	case V4L2_CID_MPEG_VIDEO_QOS_RATIO:
		ctx->qos_ratio = ctrl->value;
		mfc_info_ctx("set %d qos_ratio.\n", ctrl->value);
		break;
	case V4L2_CID_MPEG_MFC_SET_DYNAMIC_DPB_MODE:
		dec->is_dynamic_dpb = ctrl->value;
		if (dec->is_dynamic_dpb == 0)
			mfc_err_dev("is_dynamic_dpb is 0. it has to be enabled.\n");
		break;
	case V4L2_CID_MPEG_MFC_SET_USER_SHARED_HANDLE:
		dec->sh_handle.fd = ctrl->value;
		if (s5p_mfc_mem_get_user_shared_handle(ctx, &dec->sh_handle)) {
			dec->sh_handle.fd = -1;
			return -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_MFC_SET_BUF_PROCESS_TYPE:
		ctx->buf_process_type = ctrl->value;
		break;
	case V4L2_CID_MPEG_VIDEO_BLACK_BAR_DETECT:
		dec->detect_black_bar = ctrl->value;
		break;
	default:
		list_for_each_entry(ctx_ctrl, &ctx->ctrls, list) {
			if (!(ctx_ctrl->type & MFC_CTRL_TYPE_SET))
				continue;

			if (ctx_ctrl->id == ctrl->id) {
				ctx_ctrl->has_new = 1;
				ctx_ctrl->val = ctrl->value;

				found = 1;
				break;
			}
		}

		if (!found) {
			mfc_err_ctx("Invalid control: 0x%08x\n", ctrl->id);
			return -EINVAL;
		}
		break;
	}

	mfc_debug_leave();

	return 0;
}

/* Get cropping information */
static int vidioc_g_crop(struct file *file, void *priv,
		struct v4l2_crop *cr)
{
	struct s5p_mfc_ctx *ctx = fh_to_mfc_ctx(file->private_data);
	struct s5p_mfc_dec *dec = ctx->dec_priv;

	mfc_debug_enter();

	if (!ready_to_get_crop(ctx)) {
		mfc_debug(2, "ready to get crop failed\n");
		return -EINVAL;
	}

	if (ctx->state == MFCINST_RUNNING && dec->detect_black_bar
			&& dec->black_bar_updated) {
		cr->c.left = dec->black_bar.left;
		cr->c.top = dec->black_bar.top;
		cr->c.width = dec->black_bar.width;
		cr->c.height = dec->black_bar.height;
		mfc_debug(2, "black bar info: l=%d t=%d w=%d h=%d\n",
				dec->black_bar.left,
				dec->black_bar.top,
				dec->black_bar.width,
				dec->black_bar.height);
	} else {
		if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_H264 ||
			ctx->src_fmt->fourcc == V4L2_PIX_FMT_HEVC ||
			ctx->src_fmt->fourcc == V4L2_PIX_FMT_BPG) {
			cr->c.left = dec->cr_left;
			cr->c.top = dec->cr_top;
			cr->c.width = ctx->img_width - dec->cr_left - dec->cr_right;
			cr->c.height = ctx->img_height - dec->cr_top - dec->cr_bot;
			mfc_debug(2, "Cropping info: l=%d t=%d " \
					"w=%d h=%d (r=%d b=%d fw=%d fh=%d)\n",
					dec->cr_left, dec->cr_top,
					cr->c.width, cr->c.height,
					dec->cr_right, dec->cr_bot,
					ctx->img_width, ctx->img_height);
		} else {
			cr->c.left = 0;
			cr->c.top = 0;
			cr->c.width = ctx->img_width;
			cr->c.height = ctx->img_height;
			mfc_debug(2, "Cropping info: w=%d h=%d fw=%d fh=%d\n",
					cr->c.width, cr->c.height,
					ctx->img_width, ctx->img_height);
		}
	}

	mfc_debug_leave();
	return 0;
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

		ret = mfc_dec_get_ctrl_val(ctx, &ctrl);
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

/* v4l2_ioctl_ops */
static const struct v4l2_ioctl_ops s5p_mfc_dec_ioctl_ops = {
	.vidioc_querycap		= vidioc_querycap,
	.vidioc_enum_fmt_vid_cap	= vidioc_enum_fmt_vid_cap,
	.vidioc_enum_fmt_vid_cap_mplane	= vidioc_enum_fmt_vid_cap_mplane,
	.vidioc_enum_fmt_vid_out	= vidioc_enum_fmt_vid_out,
	.vidioc_enum_fmt_vid_out_mplane	= vidioc_enum_fmt_vid_out_mplane,
	.vidioc_g_fmt_vid_cap_mplane	= vidioc_g_fmt_vid_cap_mplane,
	.vidioc_g_fmt_vid_out_mplane	= vidioc_g_fmt_vid_out_mplane,
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
	.vidioc_g_crop			= vidioc_g_crop,
	.vidioc_g_ext_ctrls		= vidioc_g_ext_ctrls,
};

const struct v4l2_ioctl_ops *s5p_mfc_get_dec_v4l2_ioctl_ops(void)
{
	return &s5p_mfc_dec_ioctl_ops;
}
