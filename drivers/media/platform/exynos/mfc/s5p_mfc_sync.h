/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_intr.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_INTR_H
#define __S5P_MFC_INTR_H __FILE__

#include "s5p_mfc_common.h"

#define need_to_dpb_flush(ctx)		\
	((ctx->state == MFCINST_FINISHING) ||	\
	  (ctx->state == MFCINST_RUNNING))
#define need_to_wait_nal_abort(ctx)		 \
	(ctx->state == MFCINST_ABORT_INST)
#define need_to_continue(ctx)			\
	((ctx->state == MFCINST_DPB_FLUSHING) ||\
	(ctx->state == MFCINST_ABORT_INST) ||	\
	(ctx->state == MFCINST_RETURN_INST) ||	\
	(ctx->state == MFCINST_SPECIAL_PARSING) ||	\
	(ctx->state == MFCINST_SPECIAL_PARSING_NAL))
#define need_to_special_parsing(ctx)		\
	((ctx->state == MFCINST_GOT_INST) ||	\
	 (ctx->state == MFCINST_HEAD_PARSED))
#define need_to_special_parsing_nal(ctx)	\
	(ctx->state == MFCINST_RUNNING)
#define ready_to_get_crop(ctx)			\
	((ctx->state == MFCINST_HEAD_PARSED) || \
	 (ctx->state == MFCINST_RUNNING) || \
	 (ctx->state == MFCINST_FINISHING))

int s5p_mfc_wait_for_done_dev(struct s5p_mfc_dev *dev, int command);
int s5p_mfc_wait_for_done_ctx(struct s5p_mfc_ctx *ctx, int command);
void s5p_mfc_wake_up_dev(struct s5p_mfc_dev *dev, unsigned int reason,
		unsigned int err);
void s5p_mfc_wake_up_ctx(struct s5p_mfc_ctx *ctx, unsigned int reason,
		unsigned int err);

int s5p_mfc_get_new_ctx(struct s5p_mfc_dev *dev);
int s5p_mfc_dec_ctx_ready(struct s5p_mfc_ctx *ctx);
int s5p_mfc_enc_ctx_ready(struct s5p_mfc_ctx *ctx);
int s5p_mfc_ctx_ready(struct s5p_mfc_ctx *ctx);


static inline void s5p_mfc_set_bit(int num, struct s5p_mfc_bits *data)
{
	unsigned long flags;
	spin_lock_irqsave(&data->lock, flags);
	__set_bit(num, &data->bits);
	spin_unlock_irqrestore(&data->lock, flags);
}

static inline void s5p_mfc_clear_bit(int num, struct s5p_mfc_bits *data)
{
	unsigned long flags;
	spin_lock_irqsave(&data->lock, flags);
	__clear_bit(num, &data->bits);
	spin_unlock_irqrestore(&data->lock, flags);
}

static inline int s5p_mfc_test_bit(int num, struct s5p_mfc_bits *data)
{
	unsigned long flags;
	int ret;
	spin_lock_irqsave(&data->lock, flags);
	ret = test_bit(num, &data->bits);
	spin_unlock_irqrestore(&data->lock, flags);
	return ret;
}

static inline int s5p_mfc_is_all_bits_cleared(struct s5p_mfc_bits *data)
{
	unsigned long flags;
	int ret;
	spin_lock_irqsave(&data->lock, flags);
	ret = ((data->bits) == 0) ? 1 : 0;
	spin_unlock_irqrestore(&data->lock, flags);
	return ret;
}

static inline void s5p_mfc_clear_all_bits(struct s5p_mfc_bits *data)
{
	unsigned long flags;
	spin_lock_irqsave(&data->lock, flags);
	data->bits = 0;
	spin_unlock_irqrestore(&data->lock, flags);
}

static inline unsigned long s5p_mfc_get_bits(struct s5p_mfc_bits *data)
{
	unsigned long flags;
	unsigned long ret;
	spin_lock_irqsave(&data->lock, flags);
	ret = data->bits;
	spin_unlock_irqrestore(&data->lock, flags);
	return ret;
}

static inline void s5p_mfc_create_bits(struct s5p_mfc_bits *data)
{
	spin_lock_init(&data->lock);
	s5p_mfc_clear_all_bits(data);
}

static inline void s5p_mfc_delete_bits(struct s5p_mfc_bits *data)
{
	s5p_mfc_clear_all_bits(data);
}

static inline int s5p_mfc_is_work_to_do(struct s5p_mfc_dev *dev)
{
	return (!s5p_mfc_is_all_bits_cleared(&dev->work_bits));
}

#endif /* __S5P_MFC_INTR_H */
