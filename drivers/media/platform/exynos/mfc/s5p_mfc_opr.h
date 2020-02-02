/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_opr.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_OPR_H
#define __S5P_MFC_OPR_H __FILE__

#include "s5p_mfc_common.h"

int s5p_mfc_run_dec_init(struct s5p_mfc_ctx *ctx);
int s5p_mfc_run_dec_frame(struct s5p_mfc_ctx *ctx);
int s5p_mfc_run_dec_last_frames(struct s5p_mfc_ctx *ctx);
int s5p_mfc_run_enc_init(struct s5p_mfc_ctx *ctx);
int s5p_mfc_run_enc_frame(struct s5p_mfc_ctx *ctx);
int s5p_mfc_run_enc_last_frames(struct s5p_mfc_ctx *ctx);

#endif /* __S5P_MFC_OPR_H */
