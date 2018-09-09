/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_otf.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef CONFIG_VIDEO_EXYNOS_REPEATER
#include <media/exynos_repeater.h>
#endif
#ifdef CONFIG_VIDEO_EXYNOS_TSMUX
#include <media/exynos_tsmux.h>
#endif
#include <media/s5p_mfc_hwfc.h>

#include "s5p_mfc_otf.h"
#include "s5p_mfc_hwfc_internal.h"

#include "s5p_mfc_watchdog.h"
#include "s5p_mfc_sync.h"

#include "s5p_mfc_inst.h"
#include "s5p_mfc_pm.h"
#include "s5p_mfc_cmd.h"
#include "s5p_mfc_reg.h"

#include "s5p_mfc_qos.h"
#include "s5p_mfc_queue.h"
#include "s5p_mfc_utils.h"
#include "s5p_mfc_buf.h"
#include "s5p_mfc_mem.h"

static struct s5p_mfc_fmt *mfc_enc_hwfc_find_format(unsigned int format, unsigned int t)
{
	unsigned long i;

	mfc_debug_enter();

	for (i = 0; i < NUM_FORMATS; i++) {
		if (enc_hwfc_formats[i].fourcc == format &&
				enc_hwfc_formats[i].type == t)
			return (struct s5p_mfc_fmt *)&enc_hwfc_formats[i];
	}

	mfc_debug_leave();

	return NULL;
}

static int mfc_otf_set_buf_info(struct s5p_mfc_ctx *ctx)
{
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_buf_info *buf_info = &handle->otf_buf_info;

	mfc_debug_enter();

	ctx->src_fmt = mfc_enc_hwfc_find_format(buf_info->pixel_format, MFC_FMT_RAW);
	if (!ctx->src_fmt) {
		mfc_err_ctx("OTF: failed to set source format\n");
		return -EINVAL;
	}

	/* set source information */
	ctx->raw_buf.num_planes = ctx->src_fmt->num_planes;
	ctx->img_width = buf_info->width;
	ctx->img_height = buf_info->height;
	ctx->buf_stride = ALIGN(ctx->img_width, 16);

	/* calculate source size */
	s5p_mfc_enc_calc_src_size(ctx);

	mfc_debug_leave();

	return 0;
}

static int mfc_otf_map_buf(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_buf_addr *buf_addr = &handle->otf_buf_addr;
	struct _otf_buf_info *buf_info = &handle->otf_buf_info;
	struct s5p_mfc_raw_info *raw = &ctx->raw_buf;
	int i;

	mfc_debug_enter();

	mfc_debug(2, "OTF: buffer count: %d\n", buf_info->buffer_count);
	/* map buffers */
	for (i = 0; i < buf_info->buffer_count; i++) {
		mfc_debug(2, "OTF: dma_buf: %p\n", buf_info->bufs[i]);
		buf_addr->otf_buf_attach[i] = dma_buf_attach(buf_info->bufs[i], dev->device);
		if (IS_ERR(buf_addr->otf_buf_attach[i])) {
			mfc_err_ctx("OTF: Failed to get attachment (err %ld)",
				PTR_ERR(buf_addr->otf_buf_attach[i]));
			buf_addr->otf_buf_attach[i] = 0;
			return -EINVAL;
		}
		buf_addr->otf_daddr[i][0] = ion_iovmm_map(buf_addr->otf_buf_attach[i], 0,
				raw->total_plane_size, DMA_BIDIRECTIONAL, 0);
		if (IS_ERR_VALUE(buf_addr->otf_daddr[i][0])) {
			mfc_err_ctx("OTF: Failed to get daddr (0x%08llx)",
					buf_addr->otf_daddr[i][0]);
			buf_addr->otf_daddr[i][0] = 0;
			return -EINVAL;
		}
		if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV12N) {
			buf_addr->otf_daddr[i][1] = NV12N_CBCR_BASE(buf_addr->otf_daddr[i][0],
					ctx->img_width, ctx->img_height);
		} else {
			mfc_err_ctx("OTF: not supported format(0x%x)\n", ctx->src_fmt->fourcc);
			return -EINVAL;
		}
		mfc_debug(2, "OTF: index: %d, addr[0]: 0x%08llx, addr[1]: 0x%08llx\n",
				i, buf_addr->otf_daddr[i][0], buf_addr->otf_daddr[i][1]);
	}

	mfc_debug_leave();

	return 0;
}

static void mfc_otf_unmap_buf(struct s5p_mfc_ctx *ctx)
{
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_buf_addr *buf_addr = &handle->otf_buf_addr;
	struct _otf_buf_info *buf_info = &handle->otf_buf_info;
	int i;

	mfc_debug_enter();

	for (i = 0; i < buf_info->buffer_count; i++) {
		if (buf_addr->otf_daddr[i][0]) {
			ion_iovmm_unmap(buf_addr->otf_buf_attach[i], buf_addr->otf_daddr[i][0]);
			buf_addr->otf_daddr[i][0] = 0;
		}
		if (buf_addr->otf_buf_attach[i]) {
			dma_buf_detach(buf_info->bufs[i], buf_addr->otf_buf_attach[i]);
			buf_addr->otf_buf_attach[i] = 0;
		}
	}

	mfc_debug_leave();
}

static void mfc_otf_put_buf(struct s5p_mfc_ctx *ctx)
{
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_buf_info *buf_info = &handle->otf_buf_info;
	int i;

	mfc_debug_enter();

	for (i = 0; i < buf_info->buffer_count; i++) {
		if (buf_info->bufs[i]) {
			dma_buf_put(buf_info->bufs[i]);
			buf_info->bufs[i] = NULL;
		}
	}

	mfc_debug_leave();

}

static int mfc_otf_init_hwfc_buf(struct s5p_mfc_ctx *ctx)
{
#ifdef CONFIG_VIDEO_EXYNOS_REPEATER
	struct shared_buffer_info *shared_buf_info;
#endif
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_buf_info *buf_info = &handle->otf_buf_info;

	mfc_debug_enter();

#ifdef CONFIG_VIDEO_EXYNOS_REPEATER
	shared_buf_info = (struct shared_buffer_info *)buf_info;
	/* request buffers */
	if (hwfc_request_buffer(shared_buf_info, 1)) {
		mfc_err_dev("OTF: request_buffer failed\n");
		return -EINVAL;
	}
#endif
	mfc_debug(2, "OTF: recieved buffer information\n");
	mfc_debug(2, "OTF: w(%d), h(%d), format(0x%x), bufcnt(%d)\n",
			buf_info->width, buf_info->height,
			buf_info->pixel_format,	buf_info->buffer_count);

	/* set buffer information to ctx, and calculate buffer size */
	if (mfc_otf_set_buf_info(ctx)) {
		mfc_err_ctx("OTF: failed to set buffer information\n");
		mfc_otf_put_buf(ctx);
		return -EINVAL;
	}

	if (mfc_otf_map_buf(ctx)) {
		mfc_err_ctx("OTF: failed to map buffers\n");
		mfc_otf_unmap_buf(ctx);
		mfc_otf_put_buf(ctx);
		return -EINVAL;
	}
	mfc_debug(2, "OTF: HWFC buffer initialized\n");

	mfc_debug_leave();

	return 0;
}

static void mfc_otf_deinit_hwfc_buf(struct s5p_mfc_ctx *ctx)
{
	mfc_debug_enter();

	mfc_otf_unmap_buf(ctx);
	mfc_otf_put_buf(ctx);
	mfc_debug(2, "OTF: HWFC buffer de-initialized\n");

	mfc_debug_leave();
}

static int mfc_otf_create_handle(struct s5p_mfc_ctx *ctx)
{
	struct _otf_handle *otf_handle;

	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("OTF: no mfc context to run\n");
		return -EINVAL;
	}

	ctx->otf_handle = kzalloc(sizeof(*otf_handle), GFP_KERNEL);
	if (!ctx->otf_handle) {
		mfc_err_dev("OTF: no otf_handle\n");
		return -EINVAL;
	}
	mfc_debug(2, "OTF: otf_handle created\n");

	mfc_debug_leave();

	return 0;
}

static void mfc_otf_destroy_handle(struct s5p_mfc_ctx *ctx)
{
	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("OTF: no mfc context to run\n");
		return;
	}

	kfree(ctx->otf_handle);
	ctx->otf_handle = NULL;
	mfc_debug(2, "OTF: otf_handle destroyed\n");

	mfc_debug_leave();
}

int s5p_mfc_otf_init(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	int i;

	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("OTF: no mfc context to run\n");
		return -EINVAL;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("OTF: no mfc device to run\n");
		return -EINVAL;
	}

	for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
		if (dev->ctx[i] && dev->ctx[i]->otf_handle) {
			mfc_err_dev("OTF: otf_handle is already created, ctx: %d\n", i);
			return -EINVAL;
		}
	}

	if (mfc_otf_create_handle(ctx)) {
		mfc_err_dev("OTF: otf_handle is not created\n");
		return -EINVAL;
	}

	if (mfc_otf_init_hwfc_buf(ctx)) {
		mfc_err_dev("OTF: HWFC init failed\n");
		mfc_otf_destroy_handle(ctx);
		return -EINVAL;
	}

	if (otf_dump) {
		/* It is for debugging. Do not return error */
		if (s5p_mfc_otf_alloc_stream_buf(ctx)) {
			mfc_err_dev("OTF: stream buffer allocation failed\n");
			s5p_mfc_otf_release_stream_buf(ctx);
		}
	}

	mfc_debug(2, "OTF: otf_init is completed\n");

	mfc_debug_leave();

	return 0;
}

void s5p_mfc_otf_deinit(struct s5p_mfc_ctx *ctx)
{
	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("OTF: no mfc context to run\n");
		return;
	}

	s5p_mfc_otf_release_stream_buf(ctx);
	mfc_otf_deinit_hwfc_buf(ctx);
	mfc_otf_destroy_handle(ctx);
	s5p_mfc_qos_off(ctx);
	mfc_debug(2, "OTF: deinit_otf is completed\n");

	mfc_debug_leave();
}

int s5p_mfc_otf_ctx_ready(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct _otf_handle *handle;

	mfc_debug_enter();

	if (!ctx->otf_handle)
		return 0;

	handle = ctx->otf_handle;

	mfc_debug(1, "OTF: [c:%d] state = %d, otf_work_bit = %d\n",
			ctx->num, ctx->state, handle->otf_work_bit);
	/* If shutdown is called, do not try any cmd */
	if (dev->shutdown)
		return 0;

	/* Context is to parse header */
	if (ctx->state == MFCINST_GOT_INST)
		return 1;

	/* Context is to set buffers */
	if (ctx->state == MFCINST_HEAD_PARSED)
		return 1;

	if (ctx->state == MFCINST_RUNNING && handle->otf_work_bit)
		return 1;
	mfc_debug(2, "OTF: ctx is not ready.\n");

	mfc_debug_leave();

	return 0;
}

int s5p_mfc_otf_run_enc_init(struct s5p_mfc_ctx *ctx)
{
	int ret;

	mfc_debug_enter();

	s5p_mfc_set_enc_stride(ctx);
	s5p_mfc_clean_ctx_int_flags(ctx);
	ret = s5p_mfc_init_encode(ctx);

	mfc_debug_leave();

	return ret;
}

int s5p_mfc_otf_run_enc_frame(struct s5p_mfc_ctx *ctx)
{
	struct _otf_handle *handle = ctx->otf_handle;
	struct s5p_mfc_raw_info *raw;

	mfc_debug_enter();

	raw = &ctx->raw_buf;

	if (!handle) {
		mfc_err_ctx("OTF: There is no otf_handle, handle: %p\n", handle);
		return -EINVAL;
	}

	if (!handle->otf_work_bit) {
		mfc_err_ctx("OTF: Can't run OTF encoder, otf_work_bit: %d\n",
				handle->otf_work_bit);
		return -EINVAL;
	}

	s5p_mfc_otf_set_frame_addr(ctx, raw->num_planes);
	s5p_mfc_otf_set_stream_size(ctx, raw->total_plane_size);
	s5p_mfc_otf_set_hwfc_index(ctx, handle->otf_job_id);

	if (call_cop(ctx, init_buf_ctrls, ctx, MFC_CTRL_TYPE_SRC, handle->otf_buf_index) < 0)
		mfc_err_ctx("failed in init_buf_ctrls\n");
	if (call_cop(ctx, to_buf_ctrls, ctx, &ctx->src_ctrls[handle->otf_buf_index]) < 0)
		mfc_err_ctx("failed in to_buf_ctrls\n");
	if (call_cop(ctx, set_buf_ctrls_val, ctx, &ctx->src_ctrls[handle->otf_buf_index]) < 0)
		mfc_err_ctx("OTF: failed in set_buf_ctrls_val\n");

	/* Change timestamp usec -> nsec */
	s5p_mfc_qos_update_last_framerate(ctx, handle->otf_time_stamp * 1000);
	s5p_mfc_qos_update_framerate(ctx);

	/* Set stream buffer size to handle buffer full */
	s5p_mfc_clean_ctx_int_flags(ctx);
	s5p_mfc_encode_one_frame(ctx, 0);

	mfc_debug_leave();

	return 0;
}

int s5p_mfc_otf_handle_seq(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_enc *enc = ctx->enc_priv;

	mfc_debug_enter();

	enc->header_size = s5p_mfc_get_enc_strm_size();
#ifdef CONFIG_VIDEO_EXYNOS_TSMUX
	if (enc->header_size)
		set_es_size(enc->header_size);
#endif

	ctx->dpb_count = s5p_mfc_get_enc_dpb_count();
	ctx->scratch_buf_size = s5p_mfc_get_enc_scratch_size();
	mfc_debug(2, "OTF: header size: %d, cpb_count: %d, scratch size: %zu\n",
			enc->header_size, ctx->dpb_count, ctx->scratch_buf_size);

	s5p_mfc_change_state(ctx, MFCINST_HEAD_PARSED);

	if (s5p_mfc_alloc_codec_buffers(ctx)) {
		mfc_err_ctx("OTF: Failed to allocate encoding buffers.\n");
		return -EINVAL;
	}

	mfc_debug_leave();

	return 0;
}

int s5p_mfc_otf_handle_stream(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct _otf_handle *handle = ctx->otf_handle;
	struct _otf_debug *debug = &handle->otf_debug;
	struct s5p_mfc_special_buf *buf;
	struct _otf_buf_addr *buf_addr = &handle->otf_buf_addr;
	struct s5p_mfc_raw_info *raw;
	dma_addr_t enc_addr[3] = { 0, 0, 0 };
	int slice_type, i;
	unsigned int strm_size;
	unsigned int pic_count;
	int enc_ret = HWFC_ERR_NONE;
	unsigned int print_size;

	mfc_debug_enter();

#ifdef CONFIG_VIDEO_EXYNOS_TSMUX
	mfc_encoding_end();
#endif

	slice_type = s5p_mfc_get_enc_slice_type();
	pic_count = s5p_mfc_get_enc_pic_count();
	strm_size = s5p_mfc_get_enc_strm_size();
#ifdef CONFIG_VIDEO_EXYNOS_TSMUX
	if (strm_size)
		set_es_size(strm_size);
#endif

	mfc_debug(2, "OTF: encoded slice type: %d\n", slice_type);
	mfc_debug(2, "OTF: encoded stream size: %d\n", strm_size);
	mfc_debug(2, "OTF: display order: %d\n", pic_count);

	/* set encoded frame type */
	enc->frame_type = slice_type;
	raw = &ctx->raw_buf;

	if (strm_size > 0) {
		s5p_mfc_get_enc_frame_buffer(ctx, &enc_addr[0], raw->num_planes);

		for (i = 0; i < raw->num_planes; i++)
			mfc_debug(2, "OTF: encoded[%d] addr: 0x%08llx\n",
						i, enc_addr[i]);
		if (enc_addr[0] !=  buf_addr->otf_daddr[handle->otf_buf_index][0]) {
			mfc_err_ctx("OTF: address is not matched. 0x%08llx != 0x%08llx\n",
					enc_addr[0], buf_addr->otf_daddr[handle->otf_buf_index][0]);
			enc_ret = -HWFC_ERR_MFC;
		}
	} else {
		mfc_err_ctx("OTF: stream size is zero\n");
		enc_ret = -HWFC_ERR_MFC;
	}

	if (otf_dump && !ctx->is_drm) {
		buf = &debug->stream_buf[debug->frame_cnt];
		debug->stream_size[debug->frame_cnt] = strm_size;
		debug->frame_cnt++;
		if (debug->frame_cnt >= OTF_MAX_BUF)
			debug->frame_cnt = 0;
		/* print stream dump */
		print_size = (strm_size * 2) + 64;
		if (print_size > 512 && otf_dump == 1)
			print_size = 512;

		if (buf->vaddr)
			print_hex_dump(KERN_ERR, "OTF dump: ",
					DUMP_PREFIX_ADDRESS, print_size, 0,
					buf->vaddr, print_size, false);
	}

	if (call_cop(ctx, recover_buf_ctrls_val, ctx,
				&ctx->src_ctrls[handle->otf_buf_index]) < 0)
		mfc_err_ctx("OTF: failed in recover_buf_ctrls_val\n");
	if (call_cop(ctx, cleanup_buf_ctrls, ctx,
				MFC_CTRL_TYPE_SRC, handle->otf_buf_index) < 0)
		mfc_err_ctx("OTF: failed in cleanup_buf_ctrls\n");

	handle->otf_work_bit = 0;
	handle->otf_buf_index = 0;
	handle->otf_job_id = 0;

#ifdef CONFIG_VIDEO_EXYNOS_REPEATER
	hwfc_encoding_done(enc_ret);
#endif

	mfc_debug_leave();

	return 0;
}

void s5p_mfc_otf_handle_error(struct s5p_mfc_ctx *ctx,
		unsigned int reason, unsigned int err)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct _otf_handle *handle = ctx->otf_handle;
	int enc_ret = -HWFC_ERR_MFC;

	mfc_debug_enter();

	mfc_err_ctx("OTF: Interrupt Error: display: %d, decoded: %d\n",
			s5p_mfc_get_warn(err), s5p_mfc_get_err(err));
	err = s5p_mfc_get_err(err);

	/* Error recovery is dependent on the state of context */
	switch (ctx->state) {
	case MFCINST_GOT_INST:
	case MFCINST_INIT:
	case MFCINST_RETURN_INST:
	case MFCINST_HEAD_PARSED:
		mfc_err_ctx("OTF: error happened during init/de-init\n");
		break;
	case MFCINST_RUNNING:
		if (err == S5P_FIMV_ERR_MFC_TIMEOUT) {
			mfc_err_ctx("OTF: MFC TIMEOUT. go to error state\n");
			s5p_mfc_change_state(ctx, MFCINST_ERROR);
			enc_ret = -HWFC_ERR_MFC_TIMEOUT;
		} else if (err == S5P_FIMV_ERR_TS_MUX_TIMEOUT ||
				err == S5P_FIMV_ERR_G2D_TIMEOUT) {
			mfc_err_ctx("OTF: TS-MUX or G2D TIMEOUT. skip this frame\n");
			enc_ret = -HWFC_ERR_MFC_TIMEOUT;
		} else {
			mfc_err_ctx("OTF: MFC ERROR. skip this frame\n");
			enc_ret = -HWFC_ERR_MFC;
		}

		handle->otf_work_bit = 0;
		handle->otf_buf_index = 0;
		handle->otf_job_id = 0;

		if (call_cop(ctx, recover_buf_ctrls_val, ctx,
					&ctx->src_ctrls[handle->otf_buf_index]) < 0)
			mfc_err_ctx("OTF: failed in recover_buf_ctrls_val\n");
		if (call_cop(ctx, cleanup_buf_ctrls, ctx,
					MFC_CTRL_TYPE_SRC, handle->otf_buf_index) < 0)
			mfc_err_ctx("OTF: failed in cleanup_buf_ctrls\n");

#ifdef CONFIG_VIDEO_EXYNOS_REPEATER
		hwfc_encoding_done(enc_ret);
#endif
		break;
	default:
		mfc_err_ctx("Encountered an error interrupt which had not been handled.\n");
		mfc_err_ctx("ctx->state = %d, ctx->inst_no = %d\n",
						ctx->state, ctx->inst_no);
		break;
	}

	s5p_mfc_wake_up_dev(dev, reason, err);

	mfc_debug_leave();
}

int mfc_hwfc_check_run(struct s5p_mfc_ctx *ctx)
{
	struct _otf_handle *handle = ctx->otf_handle;

	mfc_debug_enter();

	if (!handle) {
		mfc_err_ctx("OTF: there is no handle for OTF\n");
		return -EINVAL;
	}
	if (handle->otf_work_bit) {
		mfc_err_ctx("OTF: OTF is already working\n");
		return -EINVAL;
	}
	if (ctx->state != MFCINST_RUNNING) {
		mfc_err_ctx("OTF: mfc is not running state\n");
		return -EINVAL;
	}

	mfc_debug_leave();

	return 0;
}

int s5p_mfc_hwfc_encode(int buf_index, int job_id, struct encoding_param *param)
{
	struct s5p_mfc_dev *dev = g_mfc_dev;
	struct _otf_handle *handle;
	struct s5p_mfc_ctx *ctx = NULL;
#ifdef CONFIG_VIDEO_EXYNOS_TSMUX
	struct packetizing_param packet_param;
#endif
	int i;

	mfc_debug_enter();

#ifdef CONFIG_VIDEO_EXYNOS_TSMUX
	mfc_encoding_start(buf_index);
#endif

	for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
		if (dev->ctx[i] && dev->ctx[i]->otf_handle) {
			ctx = dev->ctx[i];
			break;
		}
	}

	if (!ctx) {
		mfc_err_dev("OTF: there is no context to run\n");
		return -HWFC_ERR_MFC_NOT_PREPARED;
	}

	if (mfc_hwfc_check_run(ctx)) {
		mfc_err_dev("OTF: mfc is not prepared\n");
		return -HWFC_ERR_MFC_NOT_PREPARED;
	}

#ifdef CONFIG_VIDEO_EXYNOS_TSMUX
	packet_param.time_stamp = param->time_stamp;
	mfc_debug(2, "OTF: timestamp: %llu\n", param->time_stamp);
	if (packetize(&packet_param)) {
		mfc_err_dev("OTF: packetize failed\n");
		return -HWFC_ERR_TSMUX;
	}
#endif

	handle = ctx->otf_handle;
	handle->otf_work_bit = 1;
	handle->otf_buf_index = buf_index;
	handle->otf_job_id = job_id;
	handle->otf_time_stamp = param->time_stamp;

	if (s5p_mfc_otf_ctx_ready(ctx))
		s5p_mfc_set_bit(ctx->num, &dev->work_bits);
	if (s5p_mfc_is_work_to_do(dev))
		queue_work(dev->butler_wq, &dev->butler_work);

	mfc_debug_leave();

	return HWFC_ERR_NONE;
}
