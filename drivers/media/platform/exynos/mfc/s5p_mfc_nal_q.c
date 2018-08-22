/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_nal_q.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "s5p_mfc_nal_q.h"

#include "s5p_mfc_watchdog.h"
#include "s5p_mfc_sync.h"

#include "s5p_mfc_pm.h"
#include "s5p_mfc_cal.h"
#include "s5p_mfc_reg.h"

#include "s5p_mfc_qos.h"
#include "s5p_mfc_queue.h"
#include "s5p_mfc_utils.h"
#include "s5p_mfc_buf.h"
#include "s5p_mfc_mem.h"
#include "s5p_mfc_irq.h"

#ifdef NAL_Q_ENABLE
#define CBR_I_LIMIT_MAX			5
int s5p_mfc_nal_q_check_enable(struct s5p_mfc_dev *dev)
{
	struct s5p_mfc_ctx *temp_ctx;
	struct s5p_mfc_dec *dec = NULL;
	struct s5p_mfc_enc *enc = NULL;
	struct s5p_mfc_enc_params *p = NULL;
	int i;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	if (nal_q_disable)
		return 0;

	for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
		temp_ctx = dev->ctx[i];
		if (temp_ctx) {
			/* NAL-Q doesn't support drm */
			if (temp_ctx->is_drm) {
				mfc_debug(2, "There is a drm ctx. Can't start NAL-Q\n");
				return 0;
			}
			/* NAL-Q can be enabled when all ctx are in running state */
			if (temp_ctx->state != MFCINST_RUNNING &&
					temp_ctx->state != MFCINST_RUNNING_NO_OUTPUT) {
				mfc_debug(2, "There is a ctx which is not in running state. "
						"index: %d, state: %d\n", i, temp_ctx->state);
				return 0;
			}
			/* NAL-Q can't use the command about last frame */
			if (s5p_mfc_is_last_frame(temp_ctx) == 1) {
				mfc_debug(2, "There is a last frame. index: %d\n", i);
				return 0;
			}
			/* NAL-Q doesn't support OTF mode */
			if (temp_ctx->otf_handle) {
				mfc_debug(2, "There is a OTF node.\n");
				return 0;
			}
			/* NAL-Q doesn't support BPG */
			if (IS_BPG_DEC(temp_ctx) || IS_BPG_ENC(temp_ctx)) {
				mfc_debug(2, "BPG codec type\n");
				return 0;
			}
			/* NAL-Q doesn't support multi-frame, interlaced, black bar */
			if (temp_ctx->type == MFCINST_DECODER) {
				dec = temp_ctx->dec_priv;
				if (!dec) {
					mfc_debug(2, "There is no dec\n");
					return 0;
				}
				if ((dec->has_multiframe && CODEC_MULTIFRAME(temp_ctx)) || dec->consumed) {
					mfc_debug(2, "There is a multi frame or consumed header.\n");
					return 0;
				}
				if (dec->is_dpb_full) {
					mfc_debug(2, "All buffers are referenced\n");
					return 0;
				}
				if (dec->is_interlaced) {
					mfc_debug(2, "There is a interlaced stream\n");
					return 0;
				}
				if (dec->detect_black_bar) {
					mfc_debug(2, "black bar detection is enabled\n");
					return 0;
				}
			/* NAL-Q doesn't support fixed byte(slice mode), CBR_VT(rc mode) */
			} else if (temp_ctx->type == MFCINST_ENCODER) {
				enc = temp_ctx->enc_priv;
				if (!enc) {
					mfc_debug(2, "There is no enc\n");
					return 0;
				}
				if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_FIXED_BYTES) {
					mfc_debug(2, "There is fixed bytes option(slice mode)\n");
					return 0;
				}
				p = &enc->params;
				if (p->rc_reaction_coeff <= CBR_I_LIMIT_MAX) {
					mfc_debug(2, "There is CBR_VT option(rc mode)\n");
					return 0;
				}
			}
			mfc_debug(2, "There is a ctx in running state. index: %d\n", i);
		}
	}

	mfc_debug(2, "All working ctx are in running state!\n");

	mfc_debug_leave();

	return 1;
}

static int mfc_nal_q_find_ctx(struct s5p_mfc_dev *dev, EncoderOutputStr *pOutputStr)
{
	int i;

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return -EINVAL;
	}

	for(i = 0; i < MFC_NUM_CONTEXTS; i++) {
		if (dev->ctx[i] && dev->ctx[i]->inst_no == pOutputStr->InstanceId)
			return i;
	}
	return -1;
}

static nal_queue_in_handle* mfc_nal_q_create_in_q(struct s5p_mfc_dev *dev,
		nal_queue_handle *nal_q_handle)
{
	nal_queue_in_handle *nal_q_in_handle;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return NULL;
	}

	nal_q_in_handle = kzalloc(sizeof(*nal_q_in_handle), GFP_KERNEL);
	if (!nal_q_in_handle) {
		mfc_err_dev("NAL Q: Failed to get memory for nal_queue_in_handle\n");
		return NULL;
	}

	nal_q_in_handle->nal_q_handle = nal_q_handle;
	nal_q_in_handle->in_buf.buftype = MFCBUF_NORMAL;
	nal_q_in_handle->in_buf.size = NAL_Q_IN_ENTRY_SIZE * (NAL_Q_IN_QUEUE_SIZE + 2);
	if (s5p_mfc_mem_ion_alloc(dev, &nal_q_in_handle->in_buf)) {
		mfc_err_dev("NAL Q: failed to get memory\n");
		kfree(nal_q_in_handle);
		return NULL;
	}

	nal_q_in_handle->nal_q_in_addr
		= (nal_in_queue *)s5p_mfc_mem_get_vaddr(dev, &nal_q_in_handle->in_buf);
	if (!nal_q_in_handle->nal_q_in_addr) {
		mfc_err_dev("NAL Q: failed to get vaddr\n");
		s5p_mfc_mem_ion_free(dev, &nal_q_in_handle->in_buf);
		kfree(nal_q_in_handle);
		return NULL;
	}

	mfc_debug_leave();

	return nal_q_in_handle;
}

static nal_queue_out_handle* mfc_nal_q_create_out_q(struct s5p_mfc_dev *dev,
		nal_queue_handle *nal_q_handle)
{
	nal_queue_out_handle *nal_q_out_handle;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return NULL;
	}

	nal_q_out_handle = kzalloc(sizeof(*nal_q_out_handle), GFP_KERNEL);
	if (!nal_q_out_handle) {
		mfc_err_dev("NAL Q: failed to get memory for nal_queue_out_handle\n");
		return NULL;
	}

	nal_q_out_handle->nal_q_handle = nal_q_handle;
	nal_q_out_handle->out_buf.buftype = MFCBUF_NORMAL;
	nal_q_out_handle->out_buf.size = NAL_Q_OUT_ENTRY_SIZE * (NAL_Q_OUT_QUEUE_SIZE + 2);
	if (s5p_mfc_mem_ion_alloc(dev, &nal_q_out_handle->out_buf)) {
		mfc_err_dev("NAL Q: failed to get memory\n");
		kfree(nal_q_out_handle);
		return NULL;
	}

	nal_q_out_handle->nal_q_out_addr
		= (nal_out_queue *)s5p_mfc_mem_get_vaddr(dev, &nal_q_out_handle->out_buf);
	if (!nal_q_out_handle->nal_q_out_addr) {
		mfc_err_dev("NAL Q : failed to get vaddr\n");
		s5p_mfc_mem_ion_free(dev, &nal_q_out_handle->out_buf);
		kfree(nal_q_out_handle);
		return NULL;
	}

	mfc_debug_leave();

	return nal_q_out_handle;
}

static int mfc_nal_q_destroy_in_q(struct s5p_mfc_dev *dev,
			nal_queue_in_handle *nal_q_in_handle)
{
	mfc_debug_enter();

	if (!nal_q_in_handle)
		return -EINVAL;

	s5p_mfc_mem_ion_free(dev, &nal_q_in_handle->in_buf);
	if (nal_q_in_handle)
		kfree(nal_q_in_handle);

	mfc_debug_leave();

	return 0;
}

static int mfc_nal_q_destroy_out_q(struct s5p_mfc_dev *dev,
			nal_queue_out_handle *nal_q_out_handle)
{
	mfc_debug_enter();

	if (!nal_q_out_handle)
		return -EINVAL;

	s5p_mfc_mem_ion_free(dev, &nal_q_out_handle->out_buf);
	if (nal_q_out_handle)
		kfree(nal_q_out_handle);

	mfc_debug_leave();

	return 0;
}

/*
  * This function should be called after s5p_mfc_alloc_firmware() being called.
  */
nal_queue_handle *s5p_mfc_nal_q_create(struct s5p_mfc_dev *dev)
{
	nal_queue_handle *nal_q_handle;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return NULL;
	}

	nal_q_handle = kzalloc(sizeof(*nal_q_handle), GFP_KERNEL);
	if (!nal_q_handle) {
		mfc_err_dev("NAL Q: no nal_q_handle\n");
		return NULL;
	}

	nal_q_handle->nal_q_in_handle = mfc_nal_q_create_in_q(dev, nal_q_handle);
	if (!nal_q_handle->nal_q_in_handle) {
		kfree(nal_q_handle);
		mfc_err_dev("NAL Q: no nal_q_in_handle\n");
		return NULL;
	}

	spin_lock_init(&nal_q_handle->lock);

	nal_q_handle->nal_q_out_handle = mfc_nal_q_create_out_q(dev, nal_q_handle);
	if (!nal_q_handle->nal_q_out_handle) {
		mfc_nal_q_destroy_in_q(dev, nal_q_handle->nal_q_in_handle);
		kfree(nal_q_handle);
		mfc_err_dev("NAL Q: no nal_q_out_handle\n");
		return NULL;
	}

	nal_q_handle->nal_q_state = NAL_Q_STATE_CREATED;
	MFC_TRACE_DEV("** NAL Q state : %d\n", nal_q_handle->nal_q_state);
	mfc_debug(2, "NAL Q: handle created, state = %d\n", nal_q_handle->nal_q_state);

	mfc_debug_leave();

	return nal_q_handle;
}

int s5p_mfc_nal_q_destroy(struct s5p_mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	int ret = 0;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return -EINVAL;
	}

	if (!nal_q_handle) {
		mfc_err_dev("there isn't nal_q_handle\n");
		return -EINVAL;
	}

	ret = mfc_nal_q_destroy_out_q(dev, nal_q_handle->nal_q_out_handle);
	if (ret) {
		mfc_err_dev("failed nal_q_out_handle destroy\n");
		return ret;
	}

	ret = mfc_nal_q_destroy_in_q(dev, nal_q_handle->nal_q_in_handle);
	if (ret) {
		mfc_err_dev("failed nal_q_in_handle destroy\n");
		return ret;
	}

	kfree(nal_q_handle);
	dev->nal_q_handle = NULL;

	mfc_debug_leave();

	return ret;
}

void s5p_mfc_nal_q_init(struct s5p_mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return;
	}

	if (!nal_q_handle) {
		mfc_err_dev("NAL Q: There is no nal_q_handle\n");
		return;
	}

	if ((nal_q_handle->nal_q_state != NAL_Q_STATE_CREATED)
		&& (nal_q_handle->nal_q_state != NAL_Q_STATE_STOPPED)) {
		mfc_err_dev("NAL Q: State is wrong, state: %d\n", nal_q_handle->nal_q_state);
		return;
	}

	s5p_mfc_reset_nal_queue_registers(dev);

	nal_q_handle->nal_q_in_handle->in_exe_count = 0;
	nal_q_handle->nal_q_out_handle->out_exe_count = 0;

	mfc_debug(2, "NAL Q: S5P_FIMV_NAL_QUEUE_INPUT_COUNT=%d\n",
		s5p_mfc_get_nal_q_input_count());
	mfc_debug(2, "NAL Q: S5P_FIMV_NAL_QUEUE_OUTPUT_COUNT=%d\n",
		s5p_mfc_get_nal_q_output_count());
	mfc_debug(2, "NAL Q: S5P_FIMV_NAL_QUEUE_INPUT_EXE_COUNT=%d\n",
		s5p_mfc_get_nal_q_input_exe_count());
	mfc_debug(2, "NAL Q: S5P_FIMV_NAL_QUEUE_INFO=%d\n",
		s5p_mfc_get_nal_q_info());

	nal_q_handle->nal_q_exception = 0;
	nal_q_handle->nal_q_state = NAL_Q_STATE_INITIALIZED;
	MFC_TRACE_DEV("** NAL Q state : %d\n", nal_q_handle->nal_q_state);
	mfc_debug(2, "NAL Q: initialized, state = %d\n", nal_q_handle->nal_q_state);

	mfc_debug_leave();

	return;
}

void s5p_mfc_nal_q_start(struct s5p_mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	dma_addr_t addr;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return;
	}

	if (!nal_q_handle) {
		mfc_err_dev("NAL Q: There is no nal_q_handle\n");
		return;
	}

	if (nal_q_handle->nal_q_state != NAL_Q_STATE_INITIALIZED) {
		mfc_err_dev("NAL Q: State is wrong, state: %d\n", nal_q_handle->nal_q_state);
		return;
	}

	addr = nal_q_handle->nal_q_in_handle->in_buf.daddr;

	s5p_mfc_update_nal_queue_input(dev, addr, NAL_Q_IN_ENTRY_SIZE * NAL_Q_IN_QUEUE_SIZE);

	mfc_debug(2, "NAL Q: S5P_FIMV_NAL_QUEUE_INPUT_ADDR=0x%x\n",
		s5p_mfc_get_nal_q_input_addr());
	mfc_debug(2, "NAL Q: S5P_FIMV_NAL_QUEUE_INPUT_SIZE=%d\n",
		s5p_mfc_get_nal_q_input_size());

	addr = nal_q_handle->nal_q_out_handle->out_buf.daddr;

	s5p_mfc_update_nal_queue_output(dev, addr, NAL_Q_OUT_ENTRY_SIZE * NAL_Q_OUT_QUEUE_SIZE);

	mfc_debug(2, "NAL Q: S5P_FIMV_NAL_QUEUE_OUTPUT_ADDR=0x%x\n",
		s5p_mfc_get_nal_q_output_addr());
	mfc_debug(2, "S5P_FIMV_NAL_QUEUE_OUTPUT_SIZE=%d\n",
		s5p_mfc_get_nal_q_output_ize());

	nal_q_handle->nal_q_state = NAL_Q_STATE_STARTED;
	MFC_TRACE_DEV("** NAL Q state : %d\n", nal_q_handle->nal_q_state);
	mfc_debug(2, "NAL Q: started, state = %d\n", nal_q_handle->nal_q_state);

	MFC_WRITEL(MFC_TIMEOUT_VALUE, S5P_FIMV_DEC_TIMEOUT_VALUE);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_NAL_QUEUE);

	mfc_debug_leave();

	return;
}

void s5p_mfc_nal_q_stop(struct s5p_mfc_dev *dev, nal_queue_handle *nal_q_handle)
{
	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return;
	}

	if (!nal_q_handle) {
		mfc_err_dev("NAL Q: There is no nal_q_handle\n");
		return;
	}

	if (nal_q_handle->nal_q_state != NAL_Q_STATE_STARTED) {
		mfc_err_dev("NAL Q: State is wrong, state: %d\n", nal_q_handle->nal_q_state);
		return;
	}

	nal_q_handle->nal_q_state = NAL_Q_STATE_STOPPED;
	MFC_TRACE_DEV("** NAL Q state : %d\n", nal_q_handle->nal_q_state);
	mfc_debug(2, "NAL Q: stopped, state = %d\n", nal_q_handle->nal_q_state);

	s5p_mfc_clean_dev_int_flags(dev);

	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_STOP_QUEUE);

	mfc_debug_leave();

	return;
}

void s5p_mfc_nal_q_stop_if_started(struct s5p_mfc_dev *dev)
{
	nal_queue_handle *nal_q_handle;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return;
	}

	nal_q_handle = dev->nal_q_handle;
	if (!nal_q_handle) {
		mfc_err_dev("NAL Q: There is no nal_q_handle\n");
		return;
	}

	if (nal_q_handle->nal_q_state != NAL_Q_STATE_STARTED) {
		mfc_debug(2, "NAL Q: it is not running, state: %d\n",
				nal_q_handle->nal_q_state);
		return;
	}

	s5p_mfc_nal_q_stop(dev, nal_q_handle);
	mfc_info_dev("NAL Q: stop NAL QUEUE during get hwlock\n");
	if (s5p_mfc_wait_for_done_dev(dev,
				S5P_FIMV_R2H_CMD_COMPLETE_QUEUE_RET)) {
		mfc_err_dev("NAL Q: Failed to stop qeueue during get hwlock\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_STOP_NAL_Q_FOR_OTHER);
		s5p_mfc_dump_info_and_stop_hw(dev);
	}

	mfc_debug_leave();
	return;
}

void s5p_mfc_nal_q_cleanup_queue(struct s5p_mfc_dev *dev)
{
	struct s5p_mfc_ctx *ctx;
	int i;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return;
	}

	for(i = 0; i < MFC_NUM_CONTEXTS; i++) {
		ctx = dev->ctx[i];
		if (ctx) {
			s5p_mfc_cleanup_nal_queue(ctx);
			if (s5p_mfc_ctx_ready(ctx)) {
				s5p_mfc_set_bit(ctx->num, &dev->work_bits);
				mfc_debug(2, "NAL Q: set work_bits after cleanup,"
						" ctx: %d\n", ctx->num);
			}
		}
	}

	mfc_debug_leave();

	return;
}

static void mfc_nal_q_set_slice_mode(struct s5p_mfc_ctx *ctx, EncoderInputStr *pInStr)
{
	struct s5p_mfc_enc *enc = ctx->enc_priv;

	/* multi-slice control */
	if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES)
		pInStr->MsliceMode = enc->slice_mode + 0x4;
	else if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB_ROW)
		pInStr->MsliceMode = enc->slice_mode - 0x2;
	else if (enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_FIXED_BYTES)
		pInStr->MsliceMode = enc->slice_mode + 0x3;
	else
		pInStr->MsliceMode = enc->slice_mode;

	/* multi-slice MB number or bit size */
	if ((enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_MB) ||
			(enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB_ROW)) {
		pInStr->MsliceSizeMb = enc->slice_size.mb;
	} else if ((enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES) ||
			(enc->slice_mode == V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_FIXED_BYTES)){
		pInStr->MsliceSizeBits = enc->slice_size.bits;
	} else {
		pInStr->MsliceSizeMb = 0;
		pInStr->MsliceSizeBits = 0;
	}
}

static int mfc_nal_q_run_in_buf_enc(struct s5p_mfc_ctx *ctx, EncoderInputStr *pInStr)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *src_mb, *dst_mb;
	struct s5p_mfc_raw_info *raw = NULL;
	dma_addr_t src_addr[3] = {0, 0, 0};
	dma_addr_t addr_2bit[2];
	unsigned int index, i;

	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("NAL Q: no mfc context to run\n");
		return -EINVAL;
	}
	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return -EINVAL;
	}

	pInStr->StartCode = 0xBBBBBBBB;
	pInStr->CommandId = s5p_mfc_get_nal_q_input_count();
	pInStr->InstanceId = ctx->inst_no;

	raw = &ctx->raw_buf;

	/* move src_queue -> src_queue_nal_q */
	src_mb = s5p_mfc_get_move_buf(&ctx->buf_queue_lock,
		&ctx->src_buf_nal_queue, &ctx->src_buf_queue, MFC_BUF_SET_USED, MFC_QUEUE_ADD_BOTTOM);
	if (!src_mb) {
		mfc_err_dev("NAL Q: no src buffers\n");
		return -EAGAIN;
	}

	for (i = 0; i < raw->num_planes; i++) {
		src_addr[i] = s5p_mfc_mem_get_daddr_vb(&src_mb->vb.vb2_buf, i);
		mfc_debug(2, "NAL Q: enc src[%d] addr: 0x%08llx\n",
				i, src_addr[i]);
	}

	for (i = 0; i < raw->num_planes; i++)
		pInStr->FrameAddr[i] = src_addr[i];

	if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV12M_S10B ||
		ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV21M_S10B) {
		addr_2bit[0] = src_addr[0] + NV12N_Y_SIZE(ctx->img_width, ctx->img_height);
		addr_2bit[1] = src_addr[1] + NV12N_CBCR_SIZE(ctx->img_width, ctx->img_height);

		for (i = 0; i < raw->num_planes; i++)
			pInStr->Frame2bitAddr[i] = addr_2bit[i];
	} else if (ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV16M_S10B ||
		ctx->src_fmt->fourcc == V4L2_PIX_FMT_NV61M_S10B) {
		addr_2bit[0] = src_addr[0] + NV16M_Y_SIZE(ctx->img_width, ctx->img_height);
		addr_2bit[1] = src_addr[1] + NV16M_CBCR_SIZE(ctx->img_width, ctx->img_height);

		for (i = 0; i < raw->num_planes; i++)
			pInStr->Frame2bitAddr[i] = addr_2bit[i];
	}

	/* move dst_queue -> dst_queue_nal_q */
	dst_mb = s5p_mfc_get_move_buf(&ctx->buf_queue_lock,
		&ctx->dst_buf_nal_queue, &ctx->dst_buf_queue, MFC_BUF_SET_USED, MFC_QUEUE_ADD_BOTTOM);
	if (!dst_mb) {
		mfc_err_dev("NAL Q: no dst buffers\n");
		return -EAGAIN;
	}

	pInStr->StreamBufferAddr = s5p_mfc_mem_get_daddr_vb(&dst_mb->vb.vb2_buf, 0);
	pInStr->StreamBufferSize = (unsigned int)vb2_plane_size(&dst_mb->vb.vb2_buf, 0);
	pInStr->StreamBufferSize = ALIGN(pInStr->StreamBufferSize, 512);

	index = src_mb->vb.vb2_buf.index;
	if (call_cop(ctx, set_buf_ctrls_val_nal_q_enc, ctx, &ctx->src_ctrls[index], pInStr) < 0)
		mfc_err_ctx("NAL Q: failed in set_buf_ctrals_val in nal q\n");

	mfc_debug(2, "NAL Q: input queue, src_queue -> src_queue_nal_q, index:%d\n",
			src_mb->vb.vb2_buf.index);
	mfc_debug(2, "NAL Q: input queue, dst_buf_queue -> dst_buf_nal_queue, index:%d\n",
			dst_mb->vb.vb2_buf.index);

	mfc_nal_q_set_slice_mode(ctx, pInStr);

	mfc_debug_leave();

	return 0;
}

static int mfc_nal_q_run_in_buf_dec(struct s5p_mfc_ctx *ctx, DecoderInputStr *pInStr)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *src_mb, *dst_mb;
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_raw_info *raw = &ctx->raw_buf;
	dma_addr_t buf_addr;
	unsigned int strm_size;
	unsigned int cpb_buf_size;
	int src_index, dst_index;
	int i;

	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("NAL Q: no mfc context to run\n");
		return -EINVAL;
	}
	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return -EINVAL;
	}
	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("NAL Q: no mfc decoder to run\n");
		return -EINVAL;
	}

	if (s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0) &&
			s5p_mfc_is_queue_count_smaller(&ctx->buf_queue_lock,
				&ctx->ref_buf_queue, (ctx->dpb_count + 5))) {
		mfc_err_dev("NAL Q: no dst buffers\n");
		return -EAGAIN;
	}

	if (s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->src_buf_queue, 0)) {
		mfc_err_dev("NAL Q: no src buffers\n");
		return -EAGAIN;
	}

	pInStr->StartCode = 0xAAAAAAAA;
	pInStr->CommandId = s5p_mfc_get_nal_q_input_count();
	pInStr->InstanceId = ctx->inst_no;
	pInStr->NalStartOptions = 0;

	/* Try to use the non-referenced DPB on dst-queue */
	dst_mb = s5p_mfc_search_move_dpb_nal_q(ctx, dec->dynamic_used);
	if (!dst_mb) {
		mfc_debug(2, "NAL Q: no dst buffers.\n");
		return -EAGAIN;
	}

	/* move src_queue -> src_queue_nal_q */
	src_mb = s5p_mfc_get_move_buf(&ctx->buf_queue_lock,
			&ctx->src_buf_nal_queue, &ctx->src_buf_queue,
			MFC_BUF_SET_USED, MFC_QUEUE_ADD_BOTTOM);
	if (!src_mb) {
		mfc_err_dev("NAL Q: no src buffers\n");
		return -EAGAIN;
	}

	/* src buffer setting */
	buf_addr = s5p_mfc_mem_get_daddr_vb(&src_mb->vb.vb2_buf, 0);
	strm_size = src_mb->vb.vb2_buf.planes[0].bytesused;
	cpb_buf_size = ALIGN(dec->src_buf_size, STREAM_BUF_ALIGN);
	mfc_debug(2, "NAL Q: Src addr: 0x%08llx, size: %d\n", buf_addr, strm_size);

	if (strm_size > set_strm_size_max(cpb_buf_size)) {
		mfc_info_ctx("NAL Q: Decrease strm_size : %u -> %u, gap : %d\n",
				strm_size, set_strm_size_max(cpb_buf_size), STREAM_BUF_ALIGN);
		strm_size = set_strm_size_max(cpb_buf_size);
		src_mb->vb.vb2_buf.planes[0].bytesused = strm_size;
	}

	if (strm_size == 0)
		mfc_info_ctx("stream size is 0\n");

	pInStr->StreamDataSize = strm_size;
	pInStr->CpbBufferAddr = buf_addr;
	pInStr->CpbBufferSize = cpb_buf_size;
	pInStr->CpbBufferOffset = 0;

	MFC_TRACE_CTX("Set src[%d] fd: %d, %#llx\n",
			src_mb->vb.vb2_buf.index,
			src_mb->vb.vb2_buf.planes[0].m.fd,
			buf_addr);

	/* dst buffer setting */
	dst_index = dst_mb->vb.vb2_buf.index;
	set_bit(dst_index, &dec->available_dpb);
	dec->dynamic_set = 1 << dst_index;

	for (i = 0; i < raw->num_planes; i++) {
		pInStr->FrameSize[i] = raw->plane_size[i];
		pInStr->FrameAddr[i] = dst_mb->planes.raw[i];
	}
	mfc_debug(2, "NAL Q: dst addr[0]: 0x%08llx\n",
			dst_mb->planes.raw[0]);

	pInStr->ScratchBufAddr = ctx->codec_buf.daddr;
	pInStr->ScratchBufSize = ctx->scratch_buf_size;

	src_index = src_mb->vb.vb2_buf.index;
	if (call_cop(ctx, set_buf_ctrls_val_nal_q_dec, ctx,
				&ctx->src_ctrls[src_index], pInStr) < 0)
		mfc_err_ctx("NAL Q: failed in set_buf_ctrls_val\n");

	pInStr->DynamicDpbFlagLower = dec->dynamic_set;

	/* use dynamic_set value to available dpb in NAL Q */
	// pInStr->AvailableDpbFlagLower = dec->available_dpb;
	pInStr->AvailableDpbFlagLower = dec->dynamic_set;

	MFC_TRACE_CTX("Set dst[%d] fd: %d, %#llx / avail %#lx used %#x\n",
			dst_index, dst_mb->vb.vb2_buf.planes[0].m.fd, dst_mb->planes.raw[0],
			dec->available_dpb, dec->dynamic_used);

	mfc_debug_leave();

	return 0;
}

static void mfc_nal_q_get_enc_frame_buffer(struct s5p_mfc_ctx *ctx,
		dma_addr_t addr[], int num_planes, EncoderOutputStr *pOutStr)
{
	unsigned long enc_recon_y_addr, enc_recon_c_addr;
	int i;

	for (i = 0; i < num_planes; i++)
		addr[i] = pOutStr->EncodedFrameAddr[i];

	enc_recon_y_addr = pOutStr->ReconLumaDpbAddr;
	enc_recon_c_addr = pOutStr->ReconChromaDpbAddr;

	mfc_debug(2, "NAL Q: recon y addr: 0x%08lx\n", enc_recon_y_addr);
	mfc_debug(2, "NAL Q: recon c addr: 0x%08lx\n", enc_recon_c_addr);
}

static void mfc_nal_q_handle_stream(struct s5p_mfc_ctx *ctx, EncoderOutputStr *pOutStr)
{
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_buf *src_mb, *dst_mb, *ref_mb;
	struct s5p_mfc_raw_info *raw;
	dma_addr_t enc_addr[3] = { 0, 0, 0 };
	int slice_type, i;
	unsigned int strm_size;
	unsigned int pic_count;
	unsigned int index;

	mfc_debug_enter();

	slice_type = pOutStr->SliceType;
	strm_size = pOutStr->StreamSize;
	pic_count = pOutStr->EncCnt;

	mfc_debug(2, "NAL Q: encoded slice type: %d\n", slice_type);
	mfc_debug(2, "NAL Q: encoded stream size: %d\n", strm_size);
	mfc_debug(2, "NAL Q: display order: %d\n", pic_count);
/*
	if (enc->buf_full) {
		ctx->state = MFCINST_ABORT_INST;
		return 0;
	}
*/
	/* set encoded frame type */
	enc->frame_type = slice_type;
	raw = &ctx->raw_buf;

	ctx->sequence++;
	if (strm_size > 0) {
		/* at least one more dest. buffers exist always  */
		dst_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue, MFC_BUF_NO_TOUCH_USED);
		if (!dst_mb) {
			mfc_err_dev("NAL Q: no dst buffers\n");
			return;
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
		mfc_debug(2, "NAL Q: Slice type : %d\n", dst_mb->vb.flags);

		vb2_set_plane_payload(&dst_mb->vb.vb2_buf, 0, strm_size);

		index = dst_mb->vb.vb2_buf.index;
		if (call_cop(ctx, get_buf_ctrls_val_nal_q_enc, ctx,
				&ctx->dst_ctrls[index], pOutStr) < 0)
			mfc_err_ctx("NAL Q: failed in get_buf_ctrls_val in nal q\n");

		vb2_buffer_done(&dst_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
	} else if (strm_size == 0) {
		dst_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue, MFC_BUF_NO_TOUCH_USED);
		if (!dst_mb)
			mfc_err_dev("NAL Q: no dst buffers\n");
		else
			vb2_buffer_done(&dst_mb->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}

	if (slice_type >= 0) {
		if (ctx->state == MFCINST_RUNNING_NO_OUTPUT ||
			ctx->state == MFCINST_RUNNING_BUF_FULL)
			ctx->state = MFCINST_RUNNING;

		mfc_nal_q_get_enc_frame_buffer(ctx, &enc_addr[0],
					raw->num_planes, pOutStr);

		for (i = 0; i < raw->num_planes; i++)
			mfc_debug(2, "NAL Q: encoded[%d] addr: 0x%08llx\n", i,
					enc_addr[i]);

		src_mb = s5p_mfc_find_del_buf_vb(&ctx->buf_queue_lock,
				&ctx->src_buf_nal_queue, enc_addr[0]);
		if (!src_mb) {
			mfc_err_dev("NAL Q: no src buffers\n");
			return;
		}

		vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);

		ref_mb = s5p_mfc_find_del_buf_vb(&ctx->buf_queue_lock,
				&ctx->ref_buf_queue, enc_addr[0]);
		if (ref_mb)
			vb2_buffer_done(&ref_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
	} else if (s5p_mfc_is_queue_count_greater(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue, 0)) {
		src_mb = s5p_mfc_get_move_buf_used(&ctx->buf_queue_lock,
				&ctx->ref_buf_queue, &ctx->src_buf_nal_queue);
		if (!src_mb) {
			mfc_err_dev("NAL Q: no src buffers\n");
			return;
		}

		if (src_mb->used) {
			mfc_debug(2, "NAL Q: no output, src_queue_nal_q -> ref_queue, index:%d\n",
					src_mb->vb.vb2_buf.index);
		}

		/* slice_type = 4 && strm_size = 0, skipped enable
		   should be considered */
		if ((slice_type == -1) && (strm_size == 0)) {
			ctx->state = MFCINST_RUNNING_NO_OUTPUT;

			dst_mb = s5p_mfc_get_move_buf(&ctx->buf_queue_lock,
				&ctx->dst_buf_queue, &ctx->dst_buf_nal_queue, MFC_BUF_RESET_USED, MFC_QUEUE_ADD_TOP);
			if (!dst_mb) {
				mfc_err_dev("NAL Q: no dst buffers\n");
				return;
			}

			mfc_debug(2, "NAL Q: no output, dst_buf_nal_queue -> dst_buf_queue, index:%d\n",
					dst_mb->vb.vb2_buf.index);
		}

		mfc_debug(2, "NAL Q: slice_type: %d, ctx->state: %d\n", slice_type, ctx->state);
		mfc_debug(2, "NAL Q: enc src count: %d, enc ref count: %d\n",
			  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
			  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->ref_buf_queue));
	}

	mfc_debug_leave();

	return;
}

static void mfc_nal_q_handle_reuse_buffer(struct s5p_mfc_ctx *ctx, DecoderOutputStr *pOutStr)
{
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	unsigned int prev_flag, released_flag = 0;
	struct s5p_mfc_buf *dst_mb;
	dma_addr_t disp_addr;
	int i;

	/* reuse not used buf: dst_buf_nal_queue -> dst_queue */
	disp_addr = pOutStr->DisplayAddr[0];
	if (disp_addr) {
		mfc_debug(2, "NAL Q: decoding only but there is disp addr: 0x%llx\n", disp_addr);
		dst_mb = s5p_mfc_get_move_buf_addr(&ctx->buf_queue_lock,
				&ctx->dst_buf_queue, &ctx->dst_buf_nal_queue,
				disp_addr);
		if (dst_mb) {
			mfc_debug(2, "NAL Q: buf[%d] will reused. addr: 0x%08llx\n",
					dst_mb->vb.vb2_buf.index, disp_addr);
			dst_mb->used = 0;
			clear_bit(dst_mb->vb.vb2_buf.index, &dec->available_dpb);
		}
	}

	/* reuse not referenced buf anymore: ref_queue -> dst_queue */
	prev_flag = dec->dynamic_used;
	dec->dynamic_used = pOutStr->UsedDpbFlagLower;
	released_flag = prev_flag & (~dec->dynamic_used);

	if (!released_flag)
		return;

	for (i = 0; i < MFC_MAX_DPBS; i++)
		if (released_flag & (1 << i))
			s5p_mfc_move_reuse_buffer(ctx, i);

}

static void mfc_nal_q_handle_ref_frame(struct s5p_mfc_ctx *ctx, DecoderOutputStr *pOutStr)
{
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	struct s5p_mfc_buf *dst_mb;
	dma_addr_t dec_addr, buf_addr, disp_addr;
	int index = 0;

	mfc_debug_enter();

	dec_addr = pOutStr->DecodedAddr[0];
	disp_addr = pOutStr->DisplayAddr[0];
	mfc_debug(2, "NAL Q: dec addr: 0x%08llx, disp addr: 0x%08llx\n",
			dec_addr, disp_addr);

	dst_mb = s5p_mfc_find_move_buf_vb_used(&ctx->buf_queue_lock,
		&ctx->ref_buf_queue, &ctx->dst_buf_nal_queue, dec_addr);
	if (dst_mb) {
		buf_addr = s5p_mfc_mem_get_daddr_vb(&dst_mb->vb.vb2_buf, 0);
		mfc_debug(2, "NAL Q: Found in dst queue, "
				"dec addr: 0x%08llx, buf addr: 0x%08llx, used: %d\n",
				dec_addr, buf_addr, dst_mb->used);

		index = dst_mb->vb.vb2_buf.index;
		if (!((1 << index) & pOutStr->UsedDpbFlagLower))
			dec->dynamic_used |= 1 << index;
	} else {
		mfc_debug(2, "NAL Q: Can't find buffer for addr: 0x%08llx\n", dec_addr);
	}

	mfc_debug_leave();
}

static void mfc_nal_q_handle_frame_copy_timestamp(struct s5p_mfc_ctx *ctx, DecoderOutputStr *pOutStr)
{
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_buf *ref_mb, *src_mb;
	dma_addr_t dec_y_addr;

	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dec = ctx->dec_priv;
	dev = ctx->dev;

	dec_y_addr = pOutStr->DecodedAddr[0];

	/* Get the next source buffer */
	src_mb = s5p_mfc_get_buf(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue, MFC_BUF_NO_TOUCH_USED);
	if (!src_mb) {
		mfc_err_dev("no src buffers.\n");
		return;
	}

	ref_mb = s5p_mfc_find_buf_vb(&ctx->buf_queue_lock,
			&ctx->ref_buf_queue, dec_y_addr);
	if (ref_mb)
		ref_mb->vb.vb2_buf.timestamp = src_mb->vb.vb2_buf.timestamp;

	mfc_debug_leave();
}

static void mfc_nal_q_handle_frame_output_move(struct s5p_mfc_ctx *ctx,
			dma_addr_t dspl_y_addr, unsigned int released_flag)
{
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_buf *dst_mb;
	unsigned int index;

	dst_mb = s5p_mfc_find_move_buf_vb(&ctx->buf_queue_lock,
			&ctx->dst_buf_queue, &ctx->ref_buf_queue, dspl_y_addr, released_flag);
	if (dst_mb) {
		mfc_debug(2, "NAL Q: find display buf, index: %d\n", dst_mb->vb.vb2_buf.index);
		/* Check if this is the buffer we're looking for */
		mfc_debug(2, "NAL Q: buf addr: 0x%08llx, disp addr: 0x%08llx\n",
				s5p_mfc_mem_get_daddr_vb(&dst_mb->vb.vb2_buf, 0), dspl_y_addr);

		index = dst_mb->vb.vb2_buf.index;

		if (released_flag & (1 << index)) {
			dec->available_dpb &= ~(1 << index);
			released_flag &= ~(1 << index);
			mfc_debug(2, "NAL Q: Corrupted frame(%d), it will be re-used(release)\n",
					s5p_mfc_get_warn(s5p_mfc_get_int_err()));
		} else {
			dec->err_reuse_flag |= 1 << index;
			mfc_debug(2, "NAL Q: Corrupted frame(%d), it will be re-used(not released)\n",
					s5p_mfc_get_warn(s5p_mfc_get_int_err()));
		}
		dec->dynamic_used |= released_flag;
	}
}

static void mfc_nal_q_handle_frame_output_del(struct s5p_mfc_ctx *ctx,
		DecoderOutputStr *pOutStr, unsigned int err, unsigned int released_flag)
{
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_raw_info *raw = &ctx->raw_buf;
	struct s5p_mfc_buf *ref_mb;
	dma_addr_t dspl_y_addr;
	unsigned int frame_type;
	unsigned int dst_frame_status;
	unsigned int is_video_signal_type = 0, is_colour_description = 0;
	unsigned int is_content_light = 0, is_display_colour = 0;
	unsigned int disp_err;
	int i, index;

	if (FW_HAS_VIDEO_SIGNAL_TYPE(dev)) {
		is_video_signal_type = ((pOutStr->VideoSignalType
					>> S5P_FIMV_D_VIDEO_SIGNAL_TYPE_FLAG_SHIFT)
					& S5P_FIMV_D_VIDEO_SIGNAL_TYPE_FLAG_MASK);
		is_colour_description = ((pOutStr->VideoSignalType
					>> S5P_FIMV_D_COLOUR_DESCRIPTION_FLAG_SHIFT)
					& S5P_FIMV_D_COLOUR_DESCRIPTION_FLAG_MASK);
	}

	if (FW_HAS_SEI_INFO_FOR_HDR(dev)) {
		is_content_light = ((pOutStr->SeiAvail >> S5P_FIMV_D_SEI_AVAIL_CONTENT_LIGHT_SHIFT)
					& S5P_FIMV_D_SEI_AVAIL_CONTENT_LIGHT_MASK);
		is_display_colour = ((pOutStr->SeiAvail >> S5P_FIMV_D_SEI_AVAIL_MASTERING_DISPLAY_SHIFT)
					& S5P_FIMV_D_SEI_AVAIL_MASTERING_DISPLAY_MASK);
	}

	if (dec->immediate_display == 1) {
		dspl_y_addr = pOutStr->DecodedAddr[0];
		frame_type = pOutStr->DecodedFrameType & S5P_FIMV_DECODED_FRAME_MASK;
	} else {
		dspl_y_addr = pOutStr->DisplayAddr[0];
		frame_type = pOutStr->DisplayFrameType & S5P_FIMV_DISPLAY_FRAME_MASK;
	}

	ref_mb = s5p_mfc_find_del_buf_vb(&ctx->buf_queue_lock,
			&ctx->ref_buf_queue, dspl_y_addr);
	if (ref_mb) {
		mfc_debug(2, "NAL Q: find display buf, index: %d\n", ref_mb->vb.vb2_buf.index);
		/* Check if this is the buffer we're looking for */
		mfc_debug(2, "NAL Q: buf addr: 0x%08llx, disp addr: 0x%08llx\n",
				s5p_mfc_mem_get_daddr_vb(&ref_mb->vb.vb2_buf, 0), dspl_y_addr);

		index = ref_mb->vb.vb2_buf.index;

		ref_mb->vb.sequence = ctx->sequence;

		/* Set reserved2 bits in order to inform SEI information */
		ref_mb->vb.reserved2 = 0;

		if (is_content_light) {
			ref_mb->vb.reserved2 |= (1 << 0);
			mfc_debug(2, "NAL Q: content light level parsed\n");
		}
		if (is_display_colour) {
			ref_mb->vb.reserved2 |= (1 << 1);
			mfc_debug(2, "NAL Q: mastering display colour parsed\n");
		}
		if (is_video_signal_type) {
			ref_mb->vb.reserved2 |= (1 << 4);
			mfc_debug(2, "NAL Q: video signal type parsed\n");
			if (is_colour_description) {
				ref_mb->vb.reserved2 |= (1 << 2);
				mfc_debug(2, "NAL Q: matrix coefficients parsed\n");
				ref_mb->vb.reserved2 |= (1 << 3);
				mfc_debug(2, "NAL Q: colour description parsed\n");
			}
		}

		if (IS_VP9_DEC(ctx) && FW_HAS_VP9_HDR(dev)) {
			if (dec->color_space != S5P_FIMV_D_COLOR_UNKNOWN) {
				ref_mb->vb.reserved2 |= (1 << 3);
				mfc_debug(2, "NAL Q: color space parsed\n");
			}
			ref_mb->vb.reserved2 |= (1 << 4);
			mfc_debug(2, "NAL Q: color range parsed\n");
		}

		for (i = 0; i < raw->num_planes; i++)
			vb2_set_plane_payload(&ref_mb->vb.vb2_buf, i,
					raw->plane_size[i]);

		clear_bit(index, &dec->available_dpb);

		ref_mb->vb.flags &= ~(V4L2_BUF_FLAG_KEYFRAME |
					V4L2_BUF_FLAG_PFRAME |
					V4L2_BUF_FLAG_BFRAME |
					V4L2_BUF_FLAG_ERROR);

		switch (frame_type) {
			case S5P_FIMV_DISPLAY_FRAME_I:
				ref_mb->vb.flags |= V4L2_BUF_FLAG_KEYFRAME;
				break;
			case S5P_FIMV_DISPLAY_FRAME_P:
				ref_mb->vb.flags |= V4L2_BUF_FLAG_PFRAME;
				break;
			case S5P_FIMV_DISPLAY_FRAME_B:
				ref_mb->vb.flags |= V4L2_BUF_FLAG_BFRAME;
				break;
			default:
				break;
		}

		disp_err = s5p_mfc_get_warn(pOutStr->ErrorCode);
		if (disp_err) {
			mfc_err_ctx("NAL Q: Warning for displayed frame: %d\n",
					disp_err);
			ref_mb->vb.flags |= V4L2_BUF_FLAG_ERROR;
		}

		if (call_cop(ctx, get_buf_ctrls_val_nal_q_dec, ctx,
					&ctx->dst_ctrls[index], pOutStr) < 0)
			mfc_err_ctx("NAL Q: failed in get_buf_ctrls_val\n");

		s5p_mfc_handle_released_info(ctx, released_flag, index);

		if (dec->immediate_display == 1) {
			dst_frame_status = pOutStr->DecodedStatus
				& S5P_FIMV_DEC_STATUS_DECODED_STATUS_MASK;

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

		s5p_mfc_qos_update_last_framerate(ctx, ref_mb->vb.vb2_buf.timestamp);
		vb2_buffer_done(&ref_mb->vb.vb2_buf, disp_err ?
				VB2_BUF_STATE_ERROR : VB2_BUF_STATE_DONE);
	}
}

static void mfc_nal_q_handle_frame_new(struct s5p_mfc_ctx *ctx, unsigned int err,
					DecoderOutputStr *pOutStr)
{
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	dma_addr_t dspl_y_addr;
	unsigned int frame_type;
	unsigned int prev_flag, released_flag = 0;
	unsigned int disp_err;

	mfc_debug_enter();

	frame_type = pOutStr->DisplayFrameType & S5P_FIMV_DISPLAY_FRAME_MASK;
	disp_err = s5p_mfc_get_warn(pOutStr->ErrorCode);

	mfc_debug(2, "NAL_Q: frame_type in nal q : %d\n", frame_type);

	ctx->sequence++;

	dspl_y_addr = pOutStr->DisplayAddr[0];

	if (dec->immediate_display == 1) {
		dspl_y_addr = pOutStr->DecodedAddr[0];
		frame_type = pOutStr->DecodedFrameType & S5P_FIMV_DECODED_FRAME_MASK;
	}

	/* If frame is same as previous then skip and do not dequeue */
	if (frame_type == S5P_FIMV_DISPLAY_FRAME_NOT_CODED) {
		if (!CODEC_NOT_CODED(ctx))
			return;
	}

	prev_flag = dec->dynamic_used;
	dec->dynamic_used = pOutStr->UsedDpbFlagLower;
	released_flag = prev_flag & (~dec->dynamic_used);

	mfc_debug(2, "NAL Q: Used flag = %08x, Released Buffer = %08x\n",
			dec->dynamic_used, released_flag);

	if ((IS_VC1_RCV_DEC(ctx) &&
			(disp_err == S5P_FIMV_ERR_SYNC_POINT_NOT_RECEIVED)) ||
			(disp_err == S5P_FIMV_ERR_BROKEN_LINK))
		mfc_nal_q_handle_frame_output_move(ctx, dspl_y_addr, released_flag);
	else
		mfc_nal_q_handle_frame_output_del(ctx, pOutStr, err, released_flag);

	mfc_debug_leave();
}

static void mfc_nal_q_handle_frame_input(struct s5p_mfc_ctx *ctx, unsigned int err,
					DecoderOutputStr *pOutStr)
{
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_buf *src_mb;
	unsigned int index;
	int deleted = 0;
	unsigned long consumed;

	/* If there is consumed byte, it is abnormal status,
	 * We have to return remained stream buffer
	 */
	if (dec->consumed) {
		mfc_err_dev("NAL Q: previous buffer was not fully consumed\n");
		src_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue,
				MFC_BUF_NO_TOUCH_USED);
		if (src_mb)
			vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
	}

	/* Check multi-frame */
	consumed = pOutStr->DecodedNalSize;
	src_mb = s5p_mfc_get_del_if_consumed(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue,
			consumed, STUFF_BYTE, err, &deleted);
	if (!src_mb) {
		mfc_err_dev("no src buffers.\n");
		return;
	}

	if (!deleted) {
		/* Run MFC again on the same buffer */
		mfc_debug(2, "NAL Q: Running again the same buffer.\n");

		if (CODEC_MULTIFRAME(ctx))
			dec->y_addr_for_pb = (dma_addr_t)pOutStr->DecodedAddr[0];

		dec->consumed = consumed;
		dec->remained_size = src_mb->vb.vb2_buf.planes[0].bytesused
			- dec->consumed;
		dec->has_multiframe = 1;

		MFC_TRACE_CTX("** consumed:%ld, remained:%ld, addr:0x%08llx\n",
			dec->consumed, dec->remained_size, dec->y_addr_for_pb);
		/* Do not move src buffer to done_list */
		return;
	}

	index = src_mb->vb.vb2_buf.index;
	if (call_cop(ctx, get_buf_ctrls_val_nal_q_dec, ctx,
				&ctx->src_ctrls[index], pOutStr) < 0)
		mfc_err_ctx("NAL Q: failed in get_buf_ctrls_val\n");

	mfc_debug(2, "NAL Q: MFC needs next buffer.\n");
	dec->consumed = 0;
	dec->remained_size = 0;

	vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_DONE);
}

void mfc_nal_q_handle_frame(struct s5p_mfc_ctx *ctx, DecoderOutputStr *pOutStr)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	unsigned int dst_frame_status, sei_avail_status, need_empty_dpb;
	unsigned int res_change, need_dpb_change, need_scratch_change;
	unsigned int is_interlaced, err;

	mfc_debug_enter();

	dst_frame_status = pOutStr->DisplayStatus
				& S5P_FIMV_DISP_STATUS_DISPLAY_STATUS_MASK;
	need_empty_dpb = (pOutStr->DisplayStatus
				>> S5P_FIMV_DISP_STATUS_NEED_EMPTY_DPB_SHIFT)
				& S5P_FIMV_DISP_STATUS_NEED_EMPTY_DPB_MASK;
	res_change = (pOutStr->DisplayStatus
				>> S5P_FIMV_DISP_STATUS_RES_CHANGE_SHIFT)
				& S5P_FIMV_DISP_STATUS_RES_CHANGE_MASK;
	need_dpb_change = (pOutStr->DisplayStatus
				>> S5P_FIMV_DISP_STATUS_NEED_DPB_CHANGE_SHIFT)
				& S5P_FIMV_DISP_STATUS_NEED_DPB_CHANGE_MASK;
	need_scratch_change = (pOutStr->DisplayStatus
				 >> S5P_FIMV_DISP_STATUS_NEED_SCRATCH_CHANGE_SHIFT)
				& S5P_FIMV_DISP_STATUS_NEED_SCRATCH_CHANGE_MASK;
	is_interlaced = (pOutStr->DisplayStatus
				>> S5P_FIMV_DISP_STATUS_INTERLACE_SHIFT)
				& S5P_FIMV_DISP_STATUS_INTERLACE_MASK;
	sei_avail_status = pOutStr->SeiAvail;
	err = pOutStr->ErrorCode;

	if (dec->immediate_display == 1)
		dst_frame_status = pOutStr->DecodedStatus
				& S5P_FIMV_DEC_STATUS_DECODED_STATUS_MASK;

	mfc_debug(2, "NAL Q: Frame Status: %x\n", dst_frame_status);
	mfc_debug(2, "NAL Q: SEI available status: %x\n", sei_avail_status);
	mfc_debug(2, "NAL Q: Used flag: old = %08x, new = %08x\n",
				dec->dynamic_used, pOutStr->UsedDpbFlagLower);

	if (ctx->state == MFCINST_RES_CHANGE_INIT) {
		mfc_debug(2, "NAL Q: return until NAL-Q stopped in try_run\n");
		goto leave_handle_frame;
	}
	if (res_change) {
		mfc_debug(2, "NAL Q: Resolution change set to %d\n", res_change);
		s5p_mfc_change_state(ctx, MFCINST_RES_CHANGE_INIT);
		ctx->wait_state = WAIT_DECODING;
		dev->nal_q_handle->nal_q_exception = 1;
		mfc_info_ctx("NAL Q: nal_q_exception is set (res change)\n");
		goto leave_handle_frame;
	}
	if (need_empty_dpb) {
		mfc_debug(2, "NAL Q: There is multi-frame. consumed:%ld\n", dec->consumed);
		dec->has_multiframe = 1;
		dev->nal_q_handle->nal_q_exception = 1;
		mfc_info_ctx("NAL Q: nal_q_exception is set (multi-frame)\n");
		goto leave_handle_frame;
	}
	if (need_dpb_change || need_scratch_change) {
		mfc_debug(2, "NAL Q: Interframe resolution change is not supported\n");
		dev->nal_q_handle->nal_q_exception = 1;
		mfc_info_ctx("NAL Q: nal_q_exception is set (interframe res change)\n");
		goto leave_handle_frame;
	}
	if (is_interlaced) {
		mfc_debug(2, "NAL Q: Progressive -> Interlaced\n");
		dec->is_interlaced = is_interlaced;
		dev->nal_q_handle->nal_q_exception = 1;
		mfc_info_ctx("NAL Q: nal_q_exception is set (interlaced)\n");
		goto leave_handle_frame;
	}

	if (s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue, 0) &&
		s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue, 0)) {
		mfc_err_dev("NAL Q: Queue count is zero for src/dst\n");
		goto leave_handle_frame;
	}

	switch (dst_frame_status) {
	case S5P_FIMV_DEC_STATUS_DECODING_DISPLAY:
		mfc_nal_q_handle_ref_frame(ctx, pOutStr);
		mfc_nal_q_handle_frame_copy_timestamp(ctx, pOutStr);
		break;
	case S5P_FIMV_DEC_STATUS_DECODING_ONLY:
		mfc_nal_q_handle_ref_frame(ctx, pOutStr);
		mfc_nal_q_handle_reuse_buffer(ctx, pOutStr);
		break;
	default:
		break;
	}

	/* A frame has been decoded and is in the buffer  */
	if (s5p_mfc_dec_status_display(dst_frame_status))
		mfc_nal_q_handle_frame_new(ctx, err, pOutStr);
	else
		mfc_debug(2, "NAL Q: No display frame.\n");

	/* Mark source buffer as complete */
	if (dst_frame_status != S5P_FIMV_DEC_STATUS_DISPLAY_ONLY)
		mfc_nal_q_handle_frame_input(ctx, err, pOutStr);
	else
		mfc_debug(2, "NAL Q: can't support display only in NAL-Q, is_dpb_full: %d\n",
				dec->is_dpb_full);

leave_handle_frame:
	mfc_debug_leave();
}

void mfc_nal_q_handle_error(struct s5p_mfc_ctx *ctx, EncoderOutputStr *pOutStr)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	struct s5p_mfc_buf *src_mb;

	mfc_debug_enter();

	if (!ctx) {
		mfc_err_dev("NAL Q: no mfc context to run\n");
		return;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return;
	}

	mfc_err_ctx("NAL Q: Interrupt Error: %d\n", pOutStr->ErrorCode);

	dev->nal_q_handle->nal_q_exception = 1;
	mfc_info_ctx("NAl Q: nal_q_exception is set (error)\n");

	if (ctx->type == MFCINST_DECODER) {
		dec = ctx->dec_priv;
		if (!dec) {
			mfc_err_dev("NAL Q: no mfc decoder to run\n");
			return;
		}

		src_mb = s5p_mfc_get_del_buf(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue, MFC_BUF_NO_TOUCH_USED);

		if (!src_mb) {
			mfc_err_dev("NAL Q: no src buffers.\n");
		} else {
			dec->consumed = 0;
			vb2_buffer_done(&src_mb->vb.vb2_buf, VB2_BUF_STATE_ERROR);
		}
	}

	mfc_debug_leave();
}

int s5p_mfc_nal_q_handle_out_buf(struct s5p_mfc_dev *dev, EncoderOutputStr *pOutStr)
{
	struct s5p_mfc_ctx *ctx;
	struct s5p_mfc_enc *enc;
	struct s5p_mfc_dec *dec;
	int ctx_num;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("NAL Q: no mfc device to run\n");
		return -EINVAL;
	}

	ctx_num = dev->nal_q_handle->nal_q_out_handle->nal_q_ctx;
	if (ctx_num < 0) {
		mfc_err_dev("NAL Q: Can't find ctx in nal q\n");
		return -EINVAL;
	}

	ctx = dev->ctx[ctx_num];
	if (!ctx) {
		mfc_err_dev("NAL Q: no mfc context to run\n");
		return -EINVAL;
	}

	mfc_debug(2, "NAL Q: Int ctx is %d(%s)\n", ctx_num,
			 ctx->type == MFCINST_ENCODER ? "enc" : "dec");

	if (s5p_mfc_get_err(pOutStr->ErrorCode) &&
			(s5p_mfc_get_err(pOutStr->ErrorCode) < S5P_FIMV_ERR_FRAME_CONCEAL)) {
		mfc_nal_q_handle_error(ctx, pOutStr);
		return 0;
	}

	if (ctx->type == MFCINST_ENCODER) {
		enc = ctx->enc_priv;
		if (!enc) {
			mfc_err_dev("NAL Q: no mfc encoder to run\n");
			return -EINVAL;
		}
		mfc_nal_q_handle_stream(ctx, pOutStr);
	} else if (ctx->type == MFCINST_DECODER) {
		dec = ctx->dec_priv;
		if (!dec) {
			mfc_err_dev("NAL Q: no mfc decoder to run\n");
			return -EINVAL;
		}
		mfc_nal_q_handle_frame(ctx, (DecoderOutputStr *)pOutStr);
	}

	mfc_debug_leave();

	return 0;
}

/*
  * This function should be called in NAL_Q_STATE_INITIALIZED or NAL_Q_STATE_STARTED state.
  */
int s5p_mfc_nal_q_enqueue_in_buf(struct s5p_mfc_dev *dev, struct s5p_mfc_ctx *ctx,
	nal_queue_in_handle *nal_q_in_handle)
{
	unsigned long flags;
	unsigned int input_count = 0;
	unsigned int input_exe_count = 0;
	int input_diff = 0;
	unsigned int index = 0;
	EncoderInputStr *pStr = NULL;
	int ret = 0;

	mfc_debug_enter();

	if (!nal_q_in_handle) {
		mfc_err_dev("NAL Q: There is no nal_q_handle\n");
		return -EINVAL;
	}

	if ((nal_q_in_handle->nal_q_handle->nal_q_state != NAL_Q_STATE_INITIALIZED)
		&& (nal_q_in_handle->nal_q_handle->nal_q_state != NAL_Q_STATE_STARTED)) {
		mfc_err_dev("NAL Q: State is wrong, state: %d\n",
				nal_q_in_handle->nal_q_handle->nal_q_state);
		return -EINVAL;
	}

	spin_lock_irqsave(&nal_q_in_handle->nal_q_handle->lock, flags);

	input_count = s5p_mfc_get_nal_q_input_count();
	input_exe_count = s5p_mfc_get_nal_q_input_exe_count();
	input_diff = input_count - input_exe_count;

	/*
	 * meaning of the variable input_diff
	 * 0:				number of available slots = NAL_Q_IN_QUEUE_SIZE
	 * 1:				number of available slots = NAL_Q_IN_QUEUE_SIZE - 1
	 * ...
	 * NAL_Q_IN_QUEUE_SIZE-1:	number of available slots = 1
	 * NAL_Q_IN_QUEUE_SIZE:		number of available slots = 0
	 */

	mfc_debug(2, "NAL Q: input_diff = %d(in: %d, exe: %d)\n",
			input_diff, input_count, input_exe_count);

	if ((input_diff < 0) || (input_diff >= NAL_Q_IN_QUEUE_SIZE)) {
		mfc_err_dev("NAL Q: No available input slot(%d)\n", input_diff);
		spin_unlock_irqrestore(&nal_q_in_handle->nal_q_handle->lock, flags);
		return -EINVAL;
	}

	index = input_count % NAL_Q_IN_QUEUE_SIZE;
	pStr = &(nal_q_in_handle->nal_q_in_addr->entry[index].enc);

	memset(pStr, 0, NAL_Q_IN_ENTRY_SIZE);

	if (ctx->type == MFCINST_ENCODER)
		ret = mfc_nal_q_run_in_buf_enc(ctx, pStr);
	else if (ctx->type == MFCINST_DECODER)
		ret = mfc_nal_q_run_in_buf_dec(ctx, (DecoderInputStr *)pStr);

	if (ret != 0) {
		mfc_debug(2, "NAL Q: Failed to set input queue\n");
		spin_unlock_irqrestore(&nal_q_in_handle->nal_q_handle->lock, flags);
		return ret;
	}

	if (nal_q_dump == 1) {
		mfc_err_dev("[NAL-Q][DUMP][%s INPUT][c: %d] diff: %d, count: %d, exe: %d\n",
				ctx->type == MFCINST_ENCODER ? "ENC" : "DEC", dev->curr_ctx,
				input_diff, input_count, input_exe_count);
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4, (int *)pStr, 256, false);
		printk("...\n");
	}
	input_count++;

	s5p_mfc_update_nal_queue_input_count(dev, input_count);

	if (input_diff == 0)
		s5p_mfc_watchdog_start_tick(dev);

	spin_unlock_irqrestore(&nal_q_in_handle->nal_q_handle->lock, flags);

	MFC_TRACE_CTX("NAL %s in: diff %d count %d exe %d\n",
			ctx->type == MFCINST_ENCODER ? "ENC" : "DEC",
			input_diff, input_count, input_exe_count);

	mfc_debug_leave();

	return ret;
}

/*
  * This function should be called in NAL_Q_STATE_STARTED state.
  */
EncoderOutputStr *s5p_mfc_nal_q_dequeue_out_buf(struct s5p_mfc_dev *dev,
	nal_queue_out_handle *nal_q_out_handle, unsigned int *reason)
{
	struct s5p_mfc_ctx *ctx;
	unsigned long flags;
	unsigned int output_count = 0;
	unsigned int output_exe_count = 0;
	int output_diff = 0;
	unsigned int index = 0;
	EncoderOutputStr *pStr = NULL;

	int input_diff = 0;

	mfc_debug_enter();

	if (!nal_q_out_handle || !nal_q_out_handle->nal_q_out_addr) {
		mfc_err_dev("NAL Q: There is no handle\n");
		return pStr;
	}

	spin_lock_irqsave(&nal_q_out_handle->nal_q_handle->lock, flags);

	output_count = s5p_mfc_get_nal_q_output_count();
	output_exe_count = nal_q_out_handle->out_exe_count;
	output_diff = output_count - output_exe_count;

	/*
	 * meaning of the variable output_diff
	 * 0:				number of output slots = 0
	 * 1:				number of output slots = 1
	 * ...
	 * NAL_Q_OUT_QUEUE_SIZE-1:	number of output slots = NAL_Q_OUT_QUEUE_SIZE - 1
	 * NAL_Q_OUT_QUEUE_SIZE:	number of output slots = NAL_Q_OUT_QUEUE_SIZE
	 */

	mfc_debug(2, "NAL Q: output_diff = %d(out: %d, exe: %d)\n",
			output_diff, output_count, output_exe_count);
	if ((output_diff <= 0) || (output_diff > NAL_Q_OUT_QUEUE_SIZE)) {
		spin_unlock_irqrestore(&nal_q_out_handle->nal_q_handle->lock, flags);
		mfc_debug(2, "NAL Q: No available output slot(%d)\n", output_diff);
		return pStr;
	}

	index = output_exe_count % NAL_Q_OUT_QUEUE_SIZE;
	pStr = &(nal_q_out_handle->nal_q_out_addr->entry[index].enc);

	nal_q_out_handle->nal_q_ctx = mfc_nal_q_find_ctx(dev, pStr);
	if (nal_q_out_handle->nal_q_ctx < 0) {
		mfc_err_dev("NAl Q: Can't find ctx in nal q\n");
		pStr = NULL;
		return pStr;
	}

	ctx = dev->ctx[nal_q_out_handle->nal_q_ctx];
	if (nal_q_dump == 1) {
		mfc_err_dev("[NAL-Q][DUMP][%s OUTPUT][c: %d] diff: %d, count: %d, exe: %d\n",
				ctx->type == MFCINST_ENCODER ? "ENC" : "DEC",
				nal_q_out_handle->nal_q_ctx,
				output_diff, output_count, output_exe_count);
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4, (int *)pStr, 256, false);
		printk("...\n");
	}
	nal_q_out_handle->out_exe_count++;

	if (pStr->ErrorCode) {
		*reason = S5P_FIMV_R2H_CMD_ERR_RET;
		mfc_err_dev("NAL Q: Error : %d\n", pStr->ErrorCode);
	}

	input_diff = s5p_mfc_get_nal_q_input_count() - s5p_mfc_get_nal_q_input_exe_count();
	if (input_diff == 0) {
		s5p_mfc_watchdog_stop_tick(dev);
	} else if (input_diff > 0) {
		s5p_mfc_watchdog_reset_tick(dev);
	}

	spin_unlock_irqrestore(&nal_q_out_handle->nal_q_handle->lock, flags);

	MFC_TRACE_CTX("NAL %s out: diff %d count %d exe %d\n",
			ctx->type == MFCINST_ENCODER ? "ENC" : "DEC",
			output_diff, output_count, output_exe_count);

	mfc_debug_leave();

	return pStr;
}

#if 0
/* not used function - only for reference sfr <-> structure */
void s5p_mfc_nal_q_fill_DecoderInputStr(struct s5p_mfc_dev *dev, DecoderInputStr *pStr)
{
	pStr->StartCode			= 0xAAAAAAAA; // Decoder input start
//	pStr->CommandId			= MFC_READL(S5P_FIMV_HOST2RISC_CMD);		// 0x1100
	pStr->InstanceId		= MFC_READL(S5P_FIMV_INSTANCE_ID);		// 0xF008
	pStr->PictureTag		= MFC_READL(S5P_FIMV_D_PICTURE_TAG);		// 0xF5C8
	pStr->CpbBufferAddr		= MFC_READL(S5P_FIMV_D_CPB_BUFFER_ADDR);	// 0xF5B0
	pStr->CpbBufferSize		= MFC_READL(S5P_FIMV_D_CPB_BUFFER_SIZE);	// 0xF5B4
	pStr->CpbBufferOffset		= MFC_READL(S5P_FIMV_D_CPB_BUFFER_OFFSET);	// 0xF5C0
	pStr->StreamDataSize		= MFC_READL(S5P_FIMV_D_STREAM_DATA_SIZE);	// 0xF5D0
	pStr->AvailableDpbFlagUpper	= MFC_READL(S5P_FIMV_D_AVAILABLE_DPB_FLAG_UPPER);// 0xF5B8
	pStr->AvailableDpbFlagLower	= MFC_READL(S5P_FIMV_D_AVAILABLE_DPB_FLAG_LOWER);// 0xF5BC
	pStr->DynamicDpbFlagUpper	= MFC_READL(S5P_FIMV_D_DYNAMIC_DPB_FLAG_UPPER);	// 0xF5D4
	pStr->DynamicDpbFlagLower	= MFC_READL(S5P_FIMV_D_DYNAMIC_DPB_FLAG_LOWER);	// 0xF5D8
	pStr->FirstPlaneDpb		= MFC_READL(S5P_FIMV_D_FIRST_PLANE_DPB0);	// 0xF160+(index*4)
	pStr->SecondPlaneDpb		= MFC_READL(S5P_FIMV_D_SECOND_PLANE_DPB0);	// 0xF260+(index*4)
	pStr->ThirdPlaneDpb		= MFC_READL(S5P_FIMV_D_THIRD_PLANE_DPB0);	// 0xF360+(index*4)
	pStr->FirstPlaneDpbSize		= MFC_READL(S5P_FIMV_D_FIRST_PLANE_DPB_SIZE);	// 0xF144
	pStr->SecondPlaneDpbSize	= MFC_READL(S5P_FIMV_D_SECOND_PLANE_DPB_SIZE);	// 0xF148
	pStr->ThirdPlaneDpbSize		= MFC_READL(S5P_FIMV_D_THIRD_PLANE_DPB_SIZE);	// 0xF14C
	pStr->NalStartOptions		= MFC_READL(0xF5AC);// S5P_FIMV_D_NAL_START_OPTIONS 0xF5AC
	pStr->FirstPlaneStrideSize	= MFC_READL(S5P_FIMV_D_FIRST_PLANE_DPB_STRIDE_SIZE);// 0xF138
	pStr->SecondPlaneStrideSize	= MFC_READL(S5P_FIMV_D_SECOND_PLANE_DPB_STRIDE_SIZE);// 0xF13C
	pStr->ThirdPlaneStrideSize	= MFC_READL(S5P_FIMV_D_THIRD_PLANE_DPB_STRIDE_SIZE);// 0xF140
	pStr->FirstPlane2BitDpbSize	= MFC_READL(S5P_FIMV_D_FIRST_PLANE_2BIT_DPB_SIZE);// 0xF578
	pStr->SecondPlane2BitDpbSize	= MFC_READL(S5P_FIMV_D_SECOND_PLANE_2BIT_DPB_SIZE);// 0xF57C
	pStr->FirstPlane2BitStrideSize	= MFC_READL(S5P_FIMV_D_FIRST_PLANE_2BIT_DPB_STRIDE_SIZE);// 0xF580
	pStr->SecondPlane2BitStrideSize	= MFC_READL(S5P_FIMV_D_SECOND_PLANE_2BIT_DPB_STRIDE_SIZE);// 0xF584
	pStr->ScratchBufAddr		= MFC_READL(S5P_FIMV_D_SCRATCH_BUFFER_ADDR);	// 0xF560
	pStr->ScratchBufSize		= MFC_READL(S5P_FIMV_D_SCRATCH_BUFFER_SIZE);	// 0xF564
}

void s5p_mfc_nal_q_flush_DecoderOutputStr(struct s5p_mfc_dev *dev, DecoderOutputStr *pStr)
{
	//pStr->StartCode; // 0xAAAAAAAA; // Decoder output start
//	MFC_WRITEL(pStr->CommandId, S5P_FIMV_RISC2HOST_CMD);				// 0x1104
	MFC_WRITEL(pStr->InstanceId, S5P_FIMV_RET_INSTANCE_ID);				// 0xF070
	MFC_WRITEL(pStr->ErrorCode, S5P_FIMV_ERROR_CODE);				// 0xF074
	MFC_WRITEL(pStr->PictureTagTop, S5P_FIMV_D_RET_PICTURE_TAG_TOP);		// 0xF674
	MFC_WRITEL(pStr->PictureTimeTop, S5P_FIMV_D_RET_PICTURE_TIME_TOP);		// 0xF67C
	MFC_WRITEL(pStr->DisplayFrameWidth, S5P_FIMV_D_DISPLAY_FRAME_WIDTH);		// 0xF600
	MFC_WRITEL(pStr->DisplayFrameHeight, S5P_FIMV_D_DISPLAY_FRAME_HEIGHT);		// 0xF604
	MFC_WRITEL(pStr->DisplayStatus, S5P_FIMV_D_DISPLAY_STATUS);			// 0xF608
	MFC_WRITEL(pStr->DisplayFirstPlaneAddr, S5P_FIMV_D_DISPLAY_FIRST_PLANE_ADDR);	// 0xF60C
	MFC_WRITEL(pStr->DisplaySecondPlaneAddr, S5P_FIMV_D_DISPLAY_SECOND_PLANE_ADDR);	// 0xF610
	MFC_WRITEL(pStr->DisplayThirdPlaneAddr,S5P_FIMV_D_DISPLAY_THIRD_PLANE_ADDR);	// 0xF614
	MFC_WRITEL(pStr->DisplayFrameType, S5P_FIMV_D_DISPLAY_FRAME_TYPE);		// 0xF618
	MFC_WRITEL(pStr->DisplayCropInfo1, S5P_FIMV_D_DISPLAY_CROP_INFO1);		// 0xF61C
	MFC_WRITEL(pStr->DisplayCropInfo2, S5P_FIMV_D_DISPLAY_CROP_INFO2);		// 0xF620
	MFC_WRITEL(pStr->DisplayPictureProfile, S5P_FIMV_D_DISPLAY_PICTURE_PROFILE);	// 0xF624
	MFC_WRITEL(pStr->DisplayAspectRatio, S5P_FIMV_D_DISPLAY_ASPECT_RATIO);		// 0xF634
	MFC_WRITEL(pStr->DisplayExtendedAr, S5P_FIMV_D_DISPLAY_EXTENDED_AR);		// 0xF638
	MFC_WRITEL(pStr->DecodedNalSize, S5P_FIMV_D_DECODED_NAL_SIZE);			// 0xF664
	MFC_WRITEL(pStr->UsedDpbFlagUpper, S5P_FIMV_D_USED_DPB_FLAG_UPPER);		// 0xF720
	MFC_WRITEL(pStr->UsedDpbFlagLower, S5P_FIMV_D_USED_DPB_FLAG_LOWER);		// 0xF724
	MFC_WRITEL(pStr->SeiAvail, S5P_FIMV_D_SEI_AVAIL);				// 0xF6DC
	MFC_WRITEL(pStr->FramePackArrgmentId, S5P_FIMV_D_FRAME_PACK_ARRGMENT_ID);	// 0xF6E0
	MFC_WRITEL(pStr->FramePackSeiInfo, S5P_FIMV_D_FRAME_PACK_SEI_INFO);		// 0xF6E4
	MFC_WRITEL(pStr->FramePackGridPos, S5P_FIMV_D_FRAME_PACK_GRID_POS);		// 0xF6E8
	MFC_WRITEL(pStr->DisplayRecoverySeiInfo, S5P_FIMV_D_DISPLAY_RECOVERY_SEI_INFO);	// 0xF6EC
	MFC_WRITEL(pStr->H264Info, S5P_FIMV_D_H264_INFO);				// 0xF690
	MFC_WRITEL(pStr->DisplayFirstCrc, S5P_FIMV_D_DISPLAY_FIRST_PLANE_CRC);		// 0xF628
	MFC_WRITEL(pStr->DisplaySecondCrc, S5P_FIMV_D_DISPLAY_SECOND_PLANE_CRC);	// 0xF62C
	MFC_WRITEL(pStr->DisplayThirdCrc, S5P_FIMV_D_DISPLAY_THIRD_PLANE_CRC);		// 0xF630
	MFC_WRITEL(pStr->DisplayFirst2BitCrc, S5P_FIMV_D_DISPLAY_FIRST_PLANE_2BIT_CRC);	// 0xF6FC
	MFC_WRITEL(pStr->DisplaySecond2BitCrc, S5P_FIMV_D_DISPLAY_SECOND_PLANE_2BIT_CRC);// 0xF700
	MFC_WRITEL(pStr->DecodedFrameWidth, S5P_FIMV_D_DECODED_FRAME_WIDTH);		// 0xF63C
	MFC_WRITEL(pStr->DecodedFrameHeight, S5P_FIMV_D_DECODED_FRAME_HEIGHT);		// 0xF640
	MFC_WRITEL(pStr->DecodedStatus, S5P_FIMV_D_DECODED_STATUS);			// 0xF644
	MFC_WRITEL(pStr->DecodedFirstPlaneAddr, S5P_FIMV_D_DECODED_FIRST_PLANE_ADDR);	// 0xF648
	MFC_WRITEL(pStr->DecodedSecondPlaneAddr, S5P_FIMV_D_DECODED_SECOND_PLANE_ADDR);	// 0xF64C
	MFC_WRITEL(pStr->DecodedThirdPlaneAddr, S5P_FIMV_D_DECODED_THIRD_PLANE_ADDR);	// 0xF650
	MFC_WRITEL(pStr->DecodedFrameType, S5P_FIMV_D_DECODED_FRAME_TYPE);		// 0xF654
	MFC_WRITEL(pStr->DecodedCropInfo1, S5P_FIMV_D_DECODED_CROP_INFO1);		// 0xF658
	MFC_WRITEL(pStr->DecodedCropInfo2, S5P_FIMV_D_DECODED_CROP_INFO2);		// 0xF65C
	MFC_WRITEL(pStr->DecodedPictureProfile, S5P_FIMV_D_DECODED_PICTURE_PROFILE);	// 0xF660
	MFC_WRITEL(pStr->DecodedRecoverySeiInfo, S5P_FIMV_D_DECODED_RECOVERY_SEI_INFO);	// 0xF6F0
	MFC_WRITEL(pStr->DecodedFirstCrc, S5P_FIMV_D_DECODED_FIRST_PLANE_CRC);		// 0xF668
	MFC_WRITEL(pStr->DecodedSecondCrc, S5P_FIMV_D_DECODED_SECOND_PLANE_CRC);	// 0xF66C
	MFC_WRITEL(pStr->DecodedThirdCrc, S5P_FIMV_D_DECODED_THIRD_PLANE_CRC);		// 0xF670
	MFC_WRITEL(pStr->DecodedFirst2BitCrc, S5P_FIMV_D_DECODED_FIRST_PLANE_2BIT_CRC);	// 0xF704
	MFC_WRITEL(pStr->DecodedSecond2BitCrc, S5P_FIMV_D_DECODED_SECOND_PLANE_2BIT_CRC);// 0xF708
	MFC_WRITEL(pStr->PictureTagBot, S5P_FIMV_D_RET_PICTURE_TAG_BOT);		// 0xF678
	MFC_WRITEL(pStr->PictureTimeBot, S5P_FIMV_D_RET_PICTURE_TIME_BOT);		// 0xF680
}
#endif
#endif
