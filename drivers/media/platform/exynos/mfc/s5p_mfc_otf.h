/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_otf.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_OTF_H
#define __S5P_MFC_OTF_H __FILE__

#include "s5p_mfc_common.h"

extern struct s5p_mfc_dev *g_mfc_dev;

int s5p_mfc_otf_init(struct s5p_mfc_ctx *ctx);
void s5p_mfc_otf_deinit(struct s5p_mfc_ctx *ctx);
int s5p_mfc_otf_ctx_ready(struct s5p_mfc_ctx *ctx);
int s5p_mfc_otf_run_enc_init(struct s5p_mfc_ctx *ctx);
int s5p_mfc_otf_run_enc_frame(struct s5p_mfc_ctx *ctx);
int s5p_mfc_otf_handle_seq(struct s5p_mfc_ctx *ctx);
int s5p_mfc_otf_handle_stream(struct s5p_mfc_ctx *ctx);
void s5p_mfc_otf_handle_error(struct s5p_mfc_ctx *ctx, unsigned int reason, unsigned int err);

#endif /* __S5P_MFC_OTF_H  */
