/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_cmd_v6.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <trace/events/mfc.h>

#include "s5p_mfc_cmd.h"

#include "s5p_mfc_reg.h"

#include "s5p_mfc_utils.h"
#include "s5p_mfc_buf.h"

extern int dbg_enable;

void s5p_mfc_cmd_host2risc(struct s5p_mfc_dev *dev, int cmd)
{
	mfc_debug(1, "Issue the command: %d\n", cmd);
	MFC_TRACE_DEV(">> CMD : %d, (dev:0x%lx, bits:%lx, owned:%d, wl:%d, trans:%d)\n",
			cmd, dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
			dev->hwlock.wl_count, dev->hwlock.transfer_owner);
	MFC_TRACE_LOG_DEV("C%d", cmd);

	trace_mfc_frame_start(dev->curr_ctx, cmd, 0, 0);
	/* Reset RISC2HOST command except nal q stop command */
	if (cmd != S5P_FIMV_H2R_CMD_STOP_QUEUE)
		MFC_WRITEL(0x0, S5P_FIMV_RISC2HOST_CMD);

	/* Start the timeout watchdog */
	if ((cmd != S5P_FIMV_H2R_CMD_NAL_QUEUE) && (cmd != S5P_FIMV_H2R_CMD_STOP_QUEUE))
		s5p_mfc_watchdog_start_tick(dev);

	if (dbg_enable) {
		/* For FW debugging */
		s5p_mfc_dbg_set_addr(dev);
		s5p_mfc_dbg_enable(dev);
	}

	dev->last_cmd = cmd;
	dev->last_cmd_time = ktime_to_timeval(ktime_get());

	/* Issue the command */
	MFC_WRITEL(cmd, S5P_FIMV_HOST2RISC_CMD);
	MFC_WRITEL(0x1, S5P_FIMV_HOST2RISC_INT);
}

int s5p_mfc_cmd_sys_init(struct s5p_mfc_dev *dev,
					enum mfc_buf_usage_type buf_type)
{
	struct s5p_mfc_buf_size_v6 *buf_size;
	struct s5p_mfc_special_buf *ctx_buf;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	s5p_mfc_clean_dev_int_flags(dev);

	buf_size = dev->variant->buf_size->buf;
	ctx_buf = &dev->common_ctx_buf;
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (buf_type == MFCBUF_DRM)
		ctx_buf = &dev->drm_common_ctx_buf;
#endif
	MFC_WRITEL(ctx_buf->daddr, S5P_FIMV_CONTEXT_MEM_ADDR);
	MFC_WRITEL(buf_size->dev_ctx, S5P_FIMV_CONTEXT_MEM_SIZE);

	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_SYS_INIT);

	mfc_debug_leave();

	return 0;
}

void s5p_mfc_cmd_sleep(struct s5p_mfc_dev *dev)
{
	mfc_debug_enter();

	s5p_mfc_clean_dev_int_flags(dev);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_SLEEP);

	mfc_debug_leave();
}

void s5p_mfc_cmd_wakeup(struct s5p_mfc_dev *dev)
{
	mfc_debug_enter();

	s5p_mfc_clean_dev_int_flags(dev);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_WAKEUP);

	mfc_debug_leave();
}

/* Open a new instance and get its number */
void s5p_mfc_cmd_open_inst(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;

	mfc_debug_enter();

	mfc_debug(2, "Requested codec mode: %d\n", ctx->codec_mode);

	MFC_WRITEL(ctx->codec_mode, S5P_FIMV_CODEC_TYPE);
	MFC_WRITEL(ctx->instance_ctx_buf.daddr, S5P_FIMV_CONTEXT_MEM_ADDR);
	MFC_WRITEL(ctx->instance_ctx_buf.size, S5P_FIMV_CONTEXT_MEM_SIZE);
	if (ctx->type == MFCINST_DECODER)
		MFC_WRITEL(ctx->dec_priv->crc_enable, S5P_FIMV_D_CRC_CTRL);

	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_OPEN_INSTANCE);

	mfc_debug_leave();
}

/* Close instance */
void s5p_mfc_cmd_close_inst(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;

	mfc_debug_enter();

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_CLOSE_INSTANCE);

	mfc_debug_leave();
}

int s5p_mfc_cmd_dpb_flush(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;

	if (ON_RES_CHANGE(ctx))
		mfc_err_ctx("dpb flush on res change(state:%d)\n", ctx->state);

	s5p_mfc_clean_ctx_int_flags(ctx);

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_DPB_FLUSH);

	return 0;
}

int s5p_mfc_cmd_cache_flush(struct s5p_mfc_dev *dev)
{
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	s5p_mfc_clean_dev_int_flags(dev);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_CACHE_FLUSH);

	return 0;
}

int s5p_mfc_cmd_dec_init_buffers(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	int ret;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}
	dec = ctx->dec_priv;
	dev = ctx->dev;
	if (!dev) {
		mfc_err_ctx("no mfc device to run\n");
		return -EINVAL;
	}
	/* Initializing decoding - parsing header */
	/* Header was parsed now starting processing
	 * First set the output frame buffers
	 * s5p_mfc_alloc_dec_buffers(ctx); */

	s5p_mfc_clean_ctx_int_flags(ctx);
	ret = s5p_mfc_set_dec_codec_buffers(ctx);
	if (ret) {
		mfc_info_ctx("isn't enough codec buffer size, re-alloc!\n");
		s5p_mfc_release_codec_buffers(ctx);
		ret = s5p_mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate decoding buffers.\n");
			return ret;
		}
		ret = s5p_mfc_set_dec_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to alloc frame mem.\n");
			return ret;
		}
	}

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_INIT_BUFFERS);

	return ret;
}

int s5p_mfc_cmd_enc_init_buffers(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	int ret;

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	/* Header was generated now starting processing
	 * First set the reference frame buffers
	 */
	if (ctx->capture_state != QUEUE_BUFS_REQUESTED) {
		mfc_err_ctx("It seems that destionation buffers were not "
			"requested.\nMFC requires that header should be generated "
			"before allocating codec buffer.\n");
		return -EAGAIN;
	}

	if (!ctx->codec_buf.alloc) {
		mfc_info_ctx("there isn't codec buffer, re-alloc!\n");
		ret = s5p_mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate encoding buffers.\n");
			return ret;
		}
	}

	s5p_mfc_clean_ctx_int_flags(ctx);
	ret = s5p_mfc_set_enc_codec_buffers(ctx);
	if (ret) {
		mfc_info_ctx("isn't enough codec buffer size, re-alloc!\n");
		s5p_mfc_release_codec_buffers(ctx);
		ret = s5p_mfc_alloc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to allocate encoding buffers.\n");
			return ret;
		}
		ret = s5p_mfc_set_enc_codec_buffers(ctx);
		if (ret) {
			mfc_err_ctx("Failed to set enc codec buffers.\n");
			return ret;
		}
	}

	MFC_WRITEL(ctx->inst_no, S5P_FIMV_INSTANCE_ID);
	s5p_mfc_cmd_host2risc(dev, S5P_FIMV_H2R_CMD_INIT_BUFFERS);

	return ret;
}
