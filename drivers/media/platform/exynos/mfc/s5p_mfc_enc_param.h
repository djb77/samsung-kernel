/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_enc_param.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_ENC_PARAM_H
#define __S5P_MFC_ENC_PARAM_H __FILE__

#include "s5p_mfc_common.h"

int s5p_mfc_set_slice_mode(struct s5p_mfc_ctx *ctx);
int s5p_mfc_set_enc_params_h264(struct s5p_mfc_ctx *ctx);
int s5p_mfc_set_enc_params_mpeg4(struct s5p_mfc_ctx *ctx);
int s5p_mfc_set_enc_params_h263(struct s5p_mfc_ctx *ctx);
int s5p_mfc_set_enc_params_vp8(struct s5p_mfc_ctx *ctx);
int s5p_mfc_set_enc_params_vp9(struct s5p_mfc_ctx *ctx);
int s5p_mfc_set_enc_params_hevc(struct s5p_mfc_ctx *ctx);

#endif /* __S5P_MFC_ENC_PARAM_H */
