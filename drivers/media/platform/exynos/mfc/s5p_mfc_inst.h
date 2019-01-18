/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_inst.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_INST_H
#define __S5P_MFC_INST_H __FILE__

#include "s5p_mfc_common.h"

void s5p_mfc_open_inst(struct s5p_mfc_ctx *ctx);
int s5p_mfc_close_inst(struct s5p_mfc_ctx *ctx);
int s5p_mfc_abort_inst(struct s5p_mfc_ctx *ctx);

int s5p_mfc_init_decode(struct s5p_mfc_ctx *ctx);
int s5p_mfc_decode_one_frame(struct s5p_mfc_ctx *ctx, int last_frame);

int s5p_mfc_init_encode(struct s5p_mfc_ctx *ctx);
int s5p_mfc_encode_one_frame(struct s5p_mfc_ctx *ctx, int last_frame);

#endif /* __S5P_MFC_INST_H  */
