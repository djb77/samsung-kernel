/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_cmd.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_CMD_H
#define __S5P_MFC_CMD_H __FILE__

#include "s5p_mfc_common.h"

int s5p_mfc_cmd_sys_init(struct s5p_mfc_dev *dev,
				enum mfc_buf_usage_type buf_type);
void s5p_mfc_cmd_sleep(struct s5p_mfc_dev *dev);
void s5p_mfc_cmd_wakeup(struct s5p_mfc_dev *dev);
int s5p_mfc_cmd_open_inst(struct s5p_mfc_ctx *ctx);
int s5p_mfc_cmd_close_inst(struct s5p_mfc_ctx *ctx);
int s5p_mfc_cmd_dpb_flush(struct s5p_mfc_ctx *ctx);
int s5p_mfc_cmd_cache_flush(struct s5p_mfc_dev *dev);
int s5p_mfc_cmd_dec_init_buffers(struct s5p_mfc_ctx *ctx);
int s5p_mfc_cmd_enc_init_buffers(struct s5p_mfc_ctx *ctx);

#endif /* __S5P_MFC_CMD_H */
