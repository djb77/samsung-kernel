/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_irq.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_IRQ_H
#define __S5P_MFC_IRQ_H __FILE__

#include <linux/interrupt.h>

#include "s5p_mfc_common.h"
#include "s5p_mfc_reg.h"

irqreturn_t s5p_mfc_top_half_irq(int irq, void *priv);
irqreturn_t s5p_mfc_irq(int irq, void *priv);

static inline int s5p_mfc_dec_status_decoding(unsigned int dst_frame_status)
{
	if (dst_frame_status == S5P_FIMV_DEC_STATUS_DECODING_DISPLAY ||
	    dst_frame_status == S5P_FIMV_DEC_STATUS_DECODING_ONLY)
		return 1;
	return 0;
}

static inline int s5p_mfc_dec_status_display(unsigned int dst_frame_status)
{
	if (dst_frame_status == S5P_FIMV_DEC_STATUS_DISPLAY_ONLY ||
	    dst_frame_status == S5P_FIMV_DEC_STATUS_DECODING_DISPLAY)
		return 1;

	return 0;
}

static inline void s5p_mfc_handle_force_change_status(struct s5p_mfc_ctx *ctx)
{
	if (ctx->state != MFCINST_ABORT && ctx->state != MFCINST_HEAD_PARSED &&
			ctx->state != MFCINST_RES_CHANGE_FLUSH)
		s5p_mfc_change_state(ctx, MFCINST_RUNNING);
}
#endif /* __S5P_MFC_IRQ_H */
