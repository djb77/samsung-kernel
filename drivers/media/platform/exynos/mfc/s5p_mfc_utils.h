/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_utils.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_UTILS_H
#define __S5P_MFC_UTILS_H __FILE__

#include <linux/time.h>

#include "s5p_mfc_common.h"

static inline void s5p_mfc_clean_dev_int_flags(struct s5p_mfc_dev *dev)
{
	dev->int_condition = 0;
	dev->int_reason = 0;
	dev->int_err = 0;
}

static inline void s5p_mfc_clean_ctx_int_flags(struct s5p_mfc_ctx *ctx)
{
	ctx->int_condition = 0;
	ctx->int_reason = 0;
	ctx->int_err = 0;
}

static inline void s5p_mfc_change_state(struct s5p_mfc_ctx *ctx, enum s5p_mfc_inst_state state)
{
	struct s5p_mfc_dev *dev = ctx->dev;

	MFC_TRACE_CTX("** state : %d\n", state);
	ctx->state = state;
}

static inline enum s5p_mfc_node_type s5p_mfc_get_node_type(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	enum s5p_mfc_node_type node_type;

	if (!vdev) {
		mfc_err_dev("failed to get video_device\n");
		return MFCNODE_INVALID;
	}

	mfc_debug(2, "video_device index: %d\n", vdev->index);

	switch (vdev->index) {
	case 0:
		node_type = MFCNODE_DECODER;
		break;
	case 1:
		node_type = MFCNODE_ENCODER;
		break;
	case 2:
		node_type = MFCNODE_DECODER_DRM;
		break;
	case 3:
		node_type = MFCNODE_ENCODER_DRM;
		break;
	default:
		node_type = MFCNODE_INVALID;
		break;
	}

	return node_type;
}

static inline int s5p_mfc_is_decoder_node(enum s5p_mfc_node_type node)
{
	if (node == MFCNODE_DECODER || node == MFCNODE_DECODER_DRM)
		return 1;

	return 0;
}

static inline int s5p_mfc_is_drm_node(enum s5p_mfc_node_type node)
{
	if (node == MFCNODE_DECODER_DRM || node == MFCNODE_ENCODER_DRM)
		return 1;

	return 0;
}

int s5p_mfc_check_vb_with_fmt(struct s5p_mfc_fmt *fmt, struct vb2_buffer *vb);

int s5p_mfc_stream_buf_prot(struct s5p_mfc_ctx *ctx,
				struct s5p_mfc_buf *buf, bool en);
int s5p_mfc_raw_buf_prot(struct s5p_mfc_ctx *ctx,
				struct s5p_mfc_buf *buf, bool en);

void s5p_mfc_dec_calc_dpb_size(struct s5p_mfc_ctx *ctx);
void s5p_mfc_enc_calc_src_size(struct s5p_mfc_ctx *ctx);

static inline void s5p_mfc_cleanup_assigned_fd(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dec *dec;
	int i;

	dec = ctx->dec_priv;

	for (i = 0; i < MFC_MAX_DPBS; i++)
		dec->assigned_fd[i] = MFC_INFO_INIT_FD;
}

static inline void s5p_mfc_clear_assigned_dpb(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dec *dec;
	int i;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return;
	}

	for (i = 0; i < MFC_MAX_DPBS; i++)
		dec->assigned_dpb[i] = NULL;
}

void s5p_mfc_cleanup_assigned_dpb(struct s5p_mfc_ctx *ctx);
void s5p_mfc_unprotect_released_dpb(struct s5p_mfc_ctx *ctx, unsigned int released_flag);
void s5p_mfc_protect_dpb(struct s5p_mfc_ctx *ctx, struct s5p_mfc_buf *dst_mb);

/* Watchdog interval */
#define WATCHDOG_TICK_INTERVAL   1000
/* After how many executions watchdog should assume lock up */
#define WATCHDOG_TICK_CNT_TO_START_WATCHDOG        5

void s5p_mfc_watchdog_tick(unsigned long arg);
void s5p_mfc_watchdog_start_tick(struct s5p_mfc_dev *dev);
void s5p_mfc_watchdog_stop_tick(struct s5p_mfc_dev *dev);
void s5p_mfc_watchdog_reset_tick(struct s5p_mfc_dev *dev);

#endif /* __S5P_MFC_UTILS_H */
