/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_intr.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "s5p_mfc_sync.h"

#include "s5p_mfc_queue.h"

#define R2H_BIT(x)	(((x) > 0) ? (1 << ((x) - 1)) : 0)

static inline unsigned int mfc_r2h_bit_mask(int cmd)
{
	unsigned int mask = R2H_BIT(cmd);

	if (cmd == S5P_FIMV_R2H_CMD_FRAME_DONE_RET)
		mask |= (R2H_BIT(S5P_FIMV_R2H_CMD_FIELD_DONE_RET) |
			 R2H_BIT(S5P_FIMV_R2H_CMD_COMPLETE_SEQ_RET) |
			 R2H_BIT(S5P_FIMV_R2H_CMD_SLICE_DONE_RET) |
			 R2H_BIT(S5P_FIMV_R2H_CMD_INIT_BUFFERS_RET) |
			 R2H_BIT(S5P_FIMV_R2H_CMD_ENC_BUFFER_FULL_RET));
	/* FIXME: Temporal mask for S3D SEI processing */
	else if (cmd == S5P_FIMV_R2H_CMD_INIT_BUFFERS_RET)
		mask |= (R2H_BIT(S5P_FIMV_R2H_CMD_FIELD_DONE_RET) |
			 R2H_BIT(S5P_FIMV_R2H_CMD_SLICE_DONE_RET) |
			 R2H_BIT(S5P_FIMV_R2H_CMD_FRAME_DONE_RET));

	return (mask |= R2H_BIT(S5P_FIMV_R2H_CMD_ERR_RET));
}

#define wait_condition(x, c) (x->int_condition &&		\
		(R2H_BIT(x->int_reason) & mfc_r2h_bit_mask(c)))
#define is_err_cond(x)	((x->int_condition) && (x->int_reason == S5P_FIMV_R2H_CMD_ERR_RET))

/*
 * Return value description
 * 0: waked up before timeout
 * 1: failed to get the response for the command before timeout
*/
int s5p_mfc_wait_for_done_dev(struct s5p_mfc_dev *dev, int command)
{
	int ret;

	ret = wait_event_timeout(dev->cmd_wq,
			wait_condition(dev, command),
			msecs_to_jiffies(MFC_INT_TIMEOUT));
	if (ret == 0) {
		mfc_err_dev("Interrupt (dev->int_reason:%d, command:%d) timed out.\n",
							dev->int_reason, command);
		return 1;
	}
	mfc_debug(2, "Finished waiting (dev->int_reason:%d, command: %d).\n",
							dev->int_reason, command);
	return 0;
}

/*
 * Return value description
 *  0: waked up before timeout
 *  1: failed to get the response for the command before timeout
 * -1: got the error response for the command before timeout
*/
int s5p_mfc_wait_for_done_ctx(struct s5p_mfc_ctx *ctx, int command)
{
	int ret;
	unsigned int timeout = MFC_INT_TIMEOUT;

	if (command == S5P_FIMV_R2H_CMD_CLOSE_INSTANCE_RET)
		timeout = MFC_INT_SHORT_TIMEOUT;

	ret = wait_event_timeout(ctx->cmd_wq,
			wait_condition(ctx, command),
			msecs_to_jiffies(timeout));
	if (ret == 0) {
		mfc_err_ctx("Interrupt (ctx->int_reason:%d, command:%d) timed out.\n",
							ctx->int_reason, command);
		return 1;
	} else if (ret > 0) {
		if (is_err_cond(ctx)) {
			mfc_err_ctx("Finished (ctx->int_reason:%d, command: %d).\n",
					ctx->int_reason, command);
			mfc_err_ctx("But error (ctx->int_err:%d).\n", ctx->int_err);
			return -1;
		}
	}
	mfc_debug(2, "Finished waiting (ctx->int_reason:%d, command: %d).\n",
							ctx->int_reason, command);
	return 0;
}

/* Wake up device wait_queue */
void s5p_mfc_wake_up_dev(struct s5p_mfc_dev *dev, unsigned int reason,
		unsigned int err)
{
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	dev->int_condition = 1;
	dev->int_reason = reason;
	dev->int_err = err;
	wake_up(&dev->cmd_wq);
}

/* Wake up context wait_queue */
void s5p_mfc_wake_up_ctx(struct s5p_mfc_ctx *ctx, unsigned int reason,
		unsigned int err)
{
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	ctx->int_condition = 1;
	ctx->int_reason = reason;
	ctx->int_err = err;
	wake_up(&ctx->cmd_wq);
}

int s5p_mfc_get_new_ctx(struct s5p_mfc_dev *dev)
{
	unsigned long wflags;
	int new_ctx_index = 0;
	int cnt = 0;
	int i;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&dev->work_bits.lock, wflags);

	mfc_debug(2, "Previous context: %d (bits %08lx)\n", dev->curr_ctx,
							dev->work_bits.bits);

	if (dev->preempt_ctx > MFC_NO_INSTANCE_SET) {
		new_ctx_index = dev->preempt_ctx;
		mfc_debug(2, "preempt_ctx is : %d\n", new_ctx_index);
	} else {
		for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
			if (dev->ctx[i] && dev->ctx[i]->otf_handle) {
				if (test_bit(i, &dev->work_bits.bits)) {
					spin_unlock_irqrestore(&dev->work_bits.lock, wflags);
					return i;
				}
				break;
			}
		}

		new_ctx_index = (dev->curr_ctx + 1) % MFC_NUM_CONTEXTS;
		while (!test_bit(new_ctx_index, &dev->work_bits.bits)) {
			new_ctx_index = (new_ctx_index + 1) % MFC_NUM_CONTEXTS;
			cnt++;
			if (cnt > MFC_NUM_CONTEXTS) {
				/* No contexts to run */
				spin_unlock_irqrestore(&dev->work_bits.lock, wflags);
				return -EAGAIN;
			}
		}
	}

	spin_unlock_irqrestore(&dev->work_bits.lock, wflags);
	return new_ctx_index;
}

/* Check whether a context should be run on hardware */
int s5p_mfc_dec_ctx_ready(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	int src_buf_queue_greater_than_0 = 0;
	int dst_buf_queue_greater_than_0 = 0;
	int ref_buf_queue_same_dpb_count_plus_5 = 0;

	mfc_debug(1, "[c:%d] src = %d, dst = %d, src_nal = %d, dst_nal = %d, ref = %d, state = %d, capstat = %d\n",
		  ctx->num, s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_queue),
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue),
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue),
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->ref_buf_queue),
		  ctx->state, ctx->capture_state);
	mfc_debug(2, "wait_state = %d\n", ctx->wait_state);

	src_buf_queue_greater_than_0
		= s5p_mfc_is_queue_count_greater(&ctx->buf_queue_lock, &ctx->src_buf_queue, 0);
	dst_buf_queue_greater_than_0
		= s5p_mfc_is_queue_count_greater(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0);
	ref_buf_queue_same_dpb_count_plus_5
		= s5p_mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->ref_buf_queue, (ctx->dpb_count + 5));

	/* If shutdown is called, do not try any cmd */
	if (dev->shutdown)
		return 0;

	/* Context is to parse header */
	if (ctx->state == MFCINST_GOT_INST &&
		src_buf_queue_greater_than_0)
		return 1;

	/* Context is to decode a frame */
	if (ctx->state == MFCINST_RUNNING &&
		ctx->wait_state == WAIT_NONE && src_buf_queue_greater_than_0 &&
		(dst_buf_queue_greater_than_0 || ref_buf_queue_same_dpb_count_plus_5))
		return 1;

	/* Context is to return last frame */
	if (ctx->state == MFCINST_FINISHING &&
		(dst_buf_queue_greater_than_0 || ref_buf_queue_same_dpb_count_plus_5))
		return 1;

	/* Context is to set buffers */
	if (ctx->state == MFCINST_HEAD_PARSED &&
		(dst_buf_queue_greater_than_0 && ctx->wait_state == WAIT_NONE))
		return 1;

	/* Resolution change */
	if ((ctx->state == MFCINST_RES_CHANGE_INIT || ctx->state == MFCINST_RES_CHANGE_FLUSH) &&
		dst_buf_queue_greater_than_0)
		return 1;

	if (ctx->state == MFCINST_RES_CHANGE_END &&
		src_buf_queue_greater_than_0)
		return 1;

	mfc_debug(2, "ctx is not ready.\n");

	return 0;
}

int s5p_mfc_enc_ctx_ready(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_enc_params *p = &enc->params;
	int src_buf_queue_greater_than_0 = 0;
	int dst_buf_queue_greater_than_0 = 0;

	mfc_debug(1, "[c:%d] src = %d, dst = %d, src_nal = %d, dst_nal = %d, ref = %d, state = %d\n",
		  ctx->num, s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_queue),
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue),
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue),
		  s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->ref_buf_queue),
		  ctx->state);

	src_buf_queue_greater_than_0
		= s5p_mfc_is_queue_count_greater(&ctx->buf_queue_lock, &ctx->src_buf_queue, 0);
	dst_buf_queue_greater_than_0
		= s5p_mfc_is_queue_count_greater(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0);

	/* If shutdown is called, do not try any cmd */
	if (dev->shutdown)
		return 0;

	/* context is ready to make header */
	if (ctx->state == MFCINST_GOT_INST &&
		dst_buf_queue_greater_than_0) {
		if (p->seq_hdr_mode == V4L2_MPEG_VIDEO_HEADER_MODE_AT_THE_READY) {
			if (src_buf_queue_greater_than_0)
				return 1;
		} else {
			return 1;
		}
	}

	/* context is ready to allocate DPB */
	if (ctx->state == MFCINST_HEAD_PARSED &&
		dst_buf_queue_greater_than_0)
		return 1;

	/* context is ready to encode a frame */
	if (ctx->state == MFCINST_RUNNING &&
		src_buf_queue_greater_than_0 && dst_buf_queue_greater_than_0)
		return 1;

	/* context is ready to encode a frame in case of B frame */
	if (ctx->state == MFCINST_RUNNING_NO_OUTPUT &&
		src_buf_queue_greater_than_0 && dst_buf_queue_greater_than_0)
		return 1;

	/* context is ready to encode a frame for NAL_ABORT command */
	if (ctx->state == MFCINST_ABORT_INST &&
		src_buf_queue_greater_than_0 && dst_buf_queue_greater_than_0)
		return 1;

	/* context is ready to encode remain frames */
	if (ctx->state == MFCINST_FINISHING &&
		src_buf_queue_greater_than_0 && dst_buf_queue_greater_than_0)
		return 1;

	mfc_debug(2, "ctx is not ready.\n");

	return 0;
}

int s5p_mfc_ctx_ready(struct s5p_mfc_ctx *ctx)
{
	if (ctx->type == MFCINST_DECODER)
		return s5p_mfc_dec_ctx_ready(ctx);
	else if (ctx->type == MFCINST_ENCODER)
		return s5p_mfc_enc_ctx_ready(ctx);

	return 0;
}
