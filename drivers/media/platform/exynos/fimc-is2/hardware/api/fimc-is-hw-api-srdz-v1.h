/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_API_SRDZ_V1_H
#define FIMC_IS_HW_API_SRDZ_V1_H

#include "fimc-is-hw-api-common.h"

#define SRDZ_SETFILE_VERSION	0x14027431

u32 fimc_is_srdz_sw_reset(void __iomem *base_addr, u32 hw_id);
void fimc_is_srdz_clear_intr_all(void __iomem *base_addr, u32 hw_id);
void fimc_is_srdz_disable_intr(void __iomem *base_addr, u32 hw_id);
void fimc_is_srdz_mask_intr(void __iomem *base_addr, u32 hw_id, u32 intr_mask);

void fimc_is_srdz_set_dma_out_enable(void __iomem *base_addr, u32 output_id, bool dma_out_en);
void fimc_is_srdz_set_otf_out_enable(void __iomem *base_addr, u32 output_id, bool otf_out_en);

void fimc_is_srdz_set_input_img_size(void __iomem *base_addr, u32 hw_id, u32 width, u32 height);
void fimc_is_srdz_set_rdma_format(void __iomem *base_addr, u32 dma_in_format);
void fimc_is_srdz_set_rdma_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_srdz_set_rdma_stride(void __iomem *base_addr, u32 y_stride, u32 uv_stride);

void fimc_is_srdz_set_wdma_format(void __iomem *base_addr, u32 output_id, u32 dma_out_format);
void fimc_is_srdz_set_wdma_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height);
void fimc_is_srdz_set_wdma_stride(void __iomem *base_addr, u32 output_id, u32 y_stride, u32 uv_stride);

void fimc_is_srdz_clear_intr_src(void __iomem *base_addr, u32 status);
u32 fimc_is_srdz_get_intr_mask(void __iomem *base_addr);
u32 fimc_is_srdz_get_intr_status(void __iomem *base_addr);
u32 fimc_is_srdz_get_version(void __iomem *base_addr);
void fimc_is_srdz_dump(void __iomem *base_addr);
#endif
