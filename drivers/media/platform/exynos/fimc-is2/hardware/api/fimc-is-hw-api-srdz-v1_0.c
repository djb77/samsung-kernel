/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-api-srdz-v1.h"
#include "sfr/fimc-is-sfr-srdz-v1_0.h"
#include "fimc-is-hw.h"
#include "fimc-is-hw-control.h"
#include "fimc-is-param.h"

u32 fimc_is_srdz_sw_reset(void __iomem *base_addr, u32 hw_id)
{
	return 0;
}

void fimc_is_srdz_clear_intr_all(void __iomem *base_addr, u32 hw_id)
{
}

void fimc_is_srdz_disable_intr(void __iomem *base_addr, u32 hw_id)
{
}

void fimc_is_srdz_mask_intr(void __iomem *base_addr, u32 hw_id, u32 intr_mask)
{
}

void fimc_is_srdz_set_dma_out_enable(void __iomem *base_addr, u32 output_id, bool dma_out_en)
{
}

void fimc_is_srdz_set_otf_out_enable(void __iomem *base_addr, u32 output_id, bool otf_out_en)
{
}

void fimc_is_srdz_set_input_img_size(void __iomem *base_addr, u32 hw_id, u32 width, u32 height)
{
}

void fimc_is_srdz_set_rdma_format(void __iomem *base_addr, u32 dma_in_format)
{
}

void fimc_is_srdz_set_rdma_size(void __iomem *base_addr, u32 width, u32 height)
{
}

void fimc_is_srdz_set_rdma_stride(void __iomem *base_addr, u32 y_stride, u32 uv_stride)
{
}

void fimc_is_srdz_set_wdma_format(void __iomem *base_addr, u32 output_id, u32 dma_out_format)
{
}

void fimc_is_srdz_set_wdma_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
}

void fimc_is_srdz_set_wdma_stride(void __iomem *base_addr, u32 output_id, u32 y_stride, u32 uv_stride)
{
}

void fimc_is_srdz_clear_intr_src(void __iomem *base_addr, u32 status)
{
}

u32 fimc_is_srdz_get_intr_mask(void __iomem *base_addr)
{
	return fimc_is_hw_get_reg(base_addr, &srdz_regs[SRDZ_R_COM_INTERRUPT_MASK]);
}

u32 fimc_is_srdz_get_intr_status(void __iomem *base_addr)
{
	return fimc_is_hw_get_reg(base_addr, &srdz_regs[SRDZ_R_COM_INTERRUPT_SRC]);
}

u32 fimc_is_srdz_get_version(void __iomem *base_addr)
{
	return 0;
}

void fimc_is_srdz_dump(void __iomem *base_addr)
{
	u32 i = 0;
	u32 reg_value = 0;

	info_hw("SRDZ ver 1.0");

	for(i = 0; i < SRDZ_REG_CNT; i++) {
		reg_value = readl(base_addr + srdz_regs[i].sfr_offset);
		sfrinfo("[DUMP] reg:[%s][0x%04X], value:[0x%08X]\n",
			srdz_regs[i].reg_name, srdz_regs[i].sfr_offset, reg_value);
	}
}
