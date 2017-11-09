/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_buf.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_BUF_H
#define __S5P_MFC_BUF_H __FILE__

#include "s5p_mfc_common.h"

/* Memory allocation */
int s5p_mfc_alloc_common_context(struct s5p_mfc_dev *dev);
void s5p_mfc_release_common_context(struct s5p_mfc_dev *dev);

int s5p_mfc_alloc_instance_context(struct s5p_mfc_ctx *ctx);
void s5p_mfc_release_instance_context(struct s5p_mfc_ctx *ctx);

int s5p_mfc_alloc_codec_buffers(struct s5p_mfc_ctx *ctx);
void s5p_mfc_release_codec_buffers(struct s5p_mfc_ctx *ctx);

int s5p_mfc_alloc_enc_roi_buffer(struct s5p_mfc_ctx *ctx);
void s5p_mfc_release_enc_roi_buffer(struct s5p_mfc_ctx *ctx);

int s5p_mfc_alloc_firmware(struct s5p_mfc_dev *dev);
int s5p_mfc_load_firmware(struct s5p_mfc_dev *dev);
int s5p_mfc_release_firmware(struct s5p_mfc_dev *dev);

int s5p_mfc_alloc_dbg_info_buffer(struct s5p_mfc_dev *dev);
int s5p_mfc_release_dbg_info_buffer(struct s5p_mfc_dev *dev);

#endif /* __S5P_MFC_BUF_H */
