/* linux/drivers/video/exynos/decon/dpp/dpp_regs.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung EXYNOS5 SoC series G-scaler driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include "dpp.h"
#include "dpp_coef.h"

#define DPP_SC_RATIO_MAX	((1 << 20) * 8 / 8)
#define DPP_SC_RATIO_7_8	((1 << 20) * 8 / 7)
#define DPP_SC_RATIO_6_8	((1 << 20) * 8 / 6)
#define DPP_SC_RATIO_5_8	((1 << 20) * 8 / 5)
#define DPP_SC_RATIO_4_8	((1 << 20) * 8 / 4)
#define DPP_SC_RATIO_3_8	((1 << 20) * 8 / 3)


/* DPU_WB_MUX */

/* dpp_reg_get_op_status() function will be used */
u32 wb_reg_get_op_status(u32 id)
{
	u32 val;

	val = dpp_read(id, DPU_WB_ENABLE);
	if (val & WB_OP_STATUS)
		return OP_STATUS_BUSY;

	return OP_STATUS_IDLE;
}

int wb_reg_set_sw_reset(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	dpp_write_mask(id, DPU_WB_ENABLE, ~0, WB_SRSET);

	do {
		cfg = dpp_read(id, DPU_WB_ENABLE);
		if (!(cfg & (WB_SRSET)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("[wb] timeout sw-reset\n");

	return -EBUSY;
}

void wb_reg_set_clock_gate_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPU_WB_ENABLE, val, WB_ALL_CLOCK_GATE_EN_MASK);
}

void wb_reg_set_sfr_update_force(u32 id)
{
	dpp_write_mask(id, DPU_WB_ENABLE, ~0, WB_SFR_UPDATE_FORCE);
}

/* Setting value : 0=Qch-enable, 1=Qch-disable */
void wb_reg_set_qch(u32 id, u32 en)
{
	u32 val = en ? 0 : ~0;
	dpp_write_mask(id, DPU_WB_ENABLE, val, WB_QCHANNEL_EN);
}

/* rgb_type : {601, 709} x {narrow, wide} */
void wb_reg_set_rgb_type(u32 id, u32 rgb_type)
{
	u32 val, mask;

	val = WB_RGB_TYPE(rgb_type);
	mask = WB_RGB_TYPE_MASK;
	dpp_write_mask(id, DPU_WB_OUT_CON0, val, mask);
}

void wb_reg_set_csc_r2y_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPU_WB_OUT_CON0, val, WB_CSC_R2Y_MASK);
}

void wb_reg_set_out_frame_alpha(u32 id, u32 alpha)
{
	u32 val, mask;

	val = WB_OUT_FRAME_ALPHA(alpha);
	mask = WB_OUT_FRAME_ALPHA_MASK;
	dpp_write_mask(id, DPU_WB_OUT_CON1, val, mask);
}

void wb_reg_set_uv_offset(u32 id, u32 off_x, u32 off_y)
{
	u32 val, mask;

	val = WB_UV_OFFSET_Y(off_y) | WB_UV_OFFSET_X(off_x);
	mask = WB_UV_OFFSET_Y_MASK | WB_UV_OFFSET_X_MASK;
	dpp_write_mask(id, DPU_WB_OUT_CON1, val, mask);
}

void wb_reg_set_dst_size(u32 id, u32 w, u32 h)
{
	u32 val;
	val = (WB_DST_HEIGHT(h) | WB_DST_WIDTH(w));
	dpp_write(id, DPU_WB_DST_SIZE, val);
}

void wb_reg_set_usb_tv_wb_size(u32 id, u32 byte_cnt)
{
	u32 val, mask;

	val = WB_FRAME_BYTE_CNT(byte_cnt);
	mask = WB_FRAME_BYTE_CNT_MASK;
	dpp_write_mask(id, DPU_WB_USB_TV_WB_SIZE, val, mask);
}

void wb_reg_set_usb_tv_wb_en(u32 id, u32 en)
{
	u32 val, mask;

	val = WB_USB_WB_EN(en);
	mask = WB_USB_WB_EN_MASK;
	dpp_write_mask(id, DPU_WB_USB_TV_WB_CON, val, mask);
}

/*
* [opt]
* 0= 543210, 1= 345012, 2= 210543, 3= 012345
*/
void wb_reg_set_usb_tv_wb_swap(u32 id, u32 opt)
{
	u32 val, mask;

	val = WB_SWAP_OPTION(opt);
	mask = WB_SWAP_OPTION_MASK;
	dpp_write_mask(id, DPU_WB_USB_TV_WB_CON, val, mask);
}

void wb_reg_set_dynamic_gating_en_all(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	mask = WB_DG_EN_ALL;
	dpp_write_mask(id, DPU_WB_DYNAMIC_GATING_EN, val, mask);
}


/*
 * DPU_DMA APIs
 *
*/


/* DMA debug */
void dma_reg_set_debug(u32 id)
{
	u32 val;

	val = dma_read(id, IDMA_CHAN_CONTROL);
	val |= IDMA_CHAN_DEBUG_ENABLE;
	dma_write(id, IDMA_CHAN_CONTROL,val);
}

void dma_reg_set_common_debug(u32 id)
{
	u32 val;

	val = dma_com_read(id, DPU_DMA_GLB_CONTROL);
	val |= IDMA_CHAN_DEBUG_ENABLE;
	dma_com_write(id, DPU_DMA_GLB_CONTROL, val);
}

/* [to_pat] 0=dedicated, 1=pattern */
void dma_reg_set_ch_map(u32 id, u32 dpp_id, u32 to_pat)
{
	u32 val, mask;
	u32 ch = 3;
	u32 ch_bit[6] = {0, 2, 4, 6, 10, 8};

	if (!to_pat) {
		if (dpp_id == IDMA_G1 || dpp_id == IDMA_VG1)
			ch = 2;
		else if (dpp_id == IDMA_G0 || dpp_id == IDMA_VG0)
			ch = 1;
		else if (dpp_id == IDMA_VGF0 || dpp_id == IDMA_VGF1)
			ch = 0;
		else
			dpp_err("unknown DMA_ID\n");
	}
	val = DMA_CH_MAP(ch, ch_bit[dpp_id]);
	mask = DMA_CH_MAP_MASK(ch_bit[dpp_id]);
	dma_com_write_mask(id, DPU_DMA_CH_MAP, val, mask);
}

void dma_reg_set_lpi_period(u32 id, u32 period)
{
	u32 val = DMA_LPI_PERIOD(period);
	dma_com_write_mask(id, DPU_DMA_LPI_PERIOD, val, DMA_LPI_PERIOD_MASK);
}

/* Setting value : 0=Qch-enable, 1=Qch-disable */
void dma_reg_set_qch(u32 id, u32 en)
{
	u32 val = en ? 0 : ~0;
	dma_com_write_mask(id, DPU_DMA_QCH_EN, val, DMA_QCH_EN);
}

/* [ch] 0~2, 3=all */
void dma_reg_set_swrst(u32 id, u32 ch)
{
	u32 mask;

	if (ch == 3)
		mask = DMA_ALL_SWRST;
	else
		mask = DMA_CH_SWRST(ch);
	dma_com_write_mask(id, DPU_DMA_SWRST, ~0, mask);
}

void dma_reg_set_glb_clock_gate_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_com_write_mask(id, DPU_DMA_GLB_CGEN, val, DMA_ALL_CGEN_MASK);
}

/* pat_id: [0,1] */
void dma_reg_set_test_pattern(u32 id, u32 pat_id, u32 pat_dat[4])
{
	if (pat_id == 0) {
		dma_com_write(id, DPU_DMA_TEST_PATTERN0_0, pat_dat[0]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN0_1, pat_dat[1]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN0_2, pat_dat[2]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN0_3, pat_dat[3]);
	} else {
		dma_com_write(id, DPU_DMA_TEST_PATTERN1_0, pat_dat[0]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN1_1, pat_dat[1]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN1_2, pat_dat[2]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN1_3, pat_dat[3]);
	}
}

int dma_reg_wait_op_status(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	do {
		cfg = dma_read(id, IDMA_ENABLE);
		if (!(cfg & (IDMA_OP_STATUS)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("[dma] timeout op_status to idle\n");

	return -EBUSY;
}

u32 dma_reg_get_op_status(u32 id)
{
	u32 val;

	/**
	 * IDMA_OP_STATUS was commonly used at ODMA_WB
	 *   because bit-field is same with ODMA_OP_STATUS
	 */
	if (id == ODMA_WB)
		val = dma_read(id, ODMA_ENABLE);
	else
		val = dma_read(id, IDMA_ENABLE);
	if (val & IDMA_OP_STATUS)
		return OP_STATUS_BUSY;

	return OP_STATUS_IDLE;
}

void dma_reg_set_sw_reset(u32 id)
{
	dma_write_mask(id, IDMA_ENABLE, ~0, IDMA_SRSET);
}

int dma_reg_wait_sw_reset_status(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	do {
		cfg = dma_read(id, IDMA_ENABLE);
		if (!(cfg & (IDMA_SRSET)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("[dma] timeout sw-reset\n");

	return -EBUSY;
}

void dma_reg_set_clock_gate_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_ENABLE, val, IDMA_ALL_CLOCK_GATE_EN_MASK);
}

void dma_reg_set_sfr_update_force(u32 id)
{
	dma_write_mask(id, IDMA_ENABLE, ~0, IDMA_SFR_UPDATE_FORCE);
}

/* G0_S only */
void dma_reg_set_secure_mode(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_ENABLE, val, IDMA_SECURE_MODE);
}

u32 dma_reg_get_irq_status(u32 id)
{
	u32 val, mask;

	if (id == ODMA_WB) {
		val = dma_read(id, ODMA_IRQ);
		mask = ODMA_ALL_IRQ_CLEAR;
	} else {
		val = dma_read(id, IDMA_IRQ);
		mask = IDMA_ALL_IRQ_CLEAR;
	}

	return val & mask;
}

void dma_reg_clear_irq(u32 id, u32 irq)
{
	if (id == ODMA_WB)
		dma_write_mask(id, ODMA_IRQ, ~0, irq);
	else
		dma_write_mask(id, IDMA_IRQ, ~0, irq);
}

void dma_reg_clear_irq_all(u32 id)
{
	u32 val = IDMA_ALL_IRQ_CLEAR;

	if (id == ODMA_WB)
		val = ODMA_ALL_IRQ_CLEAR;
	dma_write_mask(id, IDMA_IRQ, val, val);
}

void dma_reg_set_irq_mask_afbc_timeout(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_IRQ, val, IDMA_AFBC_TIMEOUT_MASK);
}

void dma_reg_set_irq_mask_recovery_start(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_IRQ, val, IDMA_RECOVERY_START_MASK);
}

void dma_reg_set_irq_mask_config_err(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_IRQ, val, IDMA_CONFIG_ERROR_MASK);
}

void dma_reg_set_irq_mask_local_hw_reset_done(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_IRQ, val, IDMA_LOCAL_HW_RESET_DONE_MASK);
}

void dma_reg_set_irq_mask_read_slave_err(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_IRQ, val, IDMA_READ_SLAVE_ERROR_MASK);
}

void dma_reg_set_irq_mask_deadlock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_IRQ, val, IDMA_IRQ_DEADLOCK_MASK);
}

void dma_reg_set_irq_mask_framedone(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_IRQ, val, IDMA_IRQ_FRAMEDONE_MASK);
}

/* for ODMA SLICE_DONE */
void dma_reg_set_irq_mask_slice_done_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, ODMA_IRQ, val, ODMA_ALL_SLICE_DONE_MASK);
}

void dma_reg_set_irq_mask_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_IRQ, val, IDMA_ALL_IRQ_MASK);
}

void dma_reg_set_irq_enable(u32 id)
{
	dma_write_mask(id, IDMA_IRQ, ~0, IDMA_IRQ_ENABLE);
}

void dma_reg_set_in_ic_max(u32 id, u32 ic_num)
{
	u32 val, mask;

	val = IDMA_IN_IC_MAX(ic_num);
	mask = IDMA_IN_IC_MAX_MASK;
	if (id == ODMA_WB)
		dma_write_mask(id, ODMA_OUT_CON0, val, mask);
	else
		dma_write_mask(id, IDMA_IN_CON, val, mask);
}

void dma_reg_set_img_format(u32 id, u32 fmt)
{
	u32 val, mask;

	val = IDMA_IMG_FORMAT(fmt);
	mask = IDMA_IMG_FORMAT_MASK;
	if (id == ODMA_WB)
		dma_write_mask(id, ODMA_OUT_CON0, val, mask);
	else
		dma_write_mask(id, IDMA_IN_CON, val, mask);
}

void dma_reg_set_in_flip(u32 id, u32 flip)
{
	u32 val, mask;

	if (id == ODMA_WB)
		return;
	val = IDMA_IN_FLIP(flip);
	mask = IDMA_IN_FLIP_MASK;
	dma_write_mask(id, IDMA_IN_CON, val, mask);
}

void dma_reg_set_afbc_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	if ((id != IDMA_VGF0) && (id != IDMA_VGF1))
		return;
	dma_write_mask(id, IDMA_IN_CON, val, IDMA_AFBC_EN);
}

void dma_reg_set_afbc_timeout_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	if ((id != IDMA_VGF0) && (id != IDMA_VGF1))
		return;
	dma_write_mask(id, IDMA_IN_CON, val, IDMA_AFBC_TO_EN);
}

void dma_reg_set_in_chroma_stride_sel(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	mask = IDMA_IN_CHROMINANCE_STRIDE_SEL;
	if (id == ODMA_WB)
		dma_write_mask(id, ODMA_OUT_CON0, val, mask);
	else
		dma_write_mask(id, IDMA_IN_CON, val, mask);
}

void dma_reg_set_block_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	if (id == ODMA_WB)
		return;
	dma_write_mask(id, IDMA_IN_CON, val, IDMA_BLOCK_EN);
}

void dma_reg_set_out_frame_alpha(u32 id, u32 alpha)
{
	u32 val, mask;

	val = IDMA_OUT_FRAME_ALPHA(alpha);
	mask = IDMA_OUT_FRAME_ALPHA_MASK;
	if (id == ODMA_WB)
		dma_write_mask(id, ODMA_OUT_CON1, val, mask);
	else
		dma_write_mask(id, IDMA_OUT_CON, val, mask);
}

/* IDMA : SRC_SIZE, ODMA : DST_SIZE */
void dma_reg_set_buf_size(u32 id, u32 w, u32 h)
{
	u32 val;
	val = (IDMA_SRC_HEIGHT(h) | IDMA_SRC_WIDTH(w));
	if (id == ODMA_WB)
		dma_write(id, ODMA_DST_SIZE, val);
	else
		dma_write(id, IDMA_SRC_SIZE, val);
}

/* IDMA : SRC_OFFSET, ODMA : DST_OFFSET */
void dma_reg_set_buf_offset(u32 id, u32 x, u32 y)
{
	u32 val;
	val = (IDMA_SRC_OFFSET_Y(y) | IDMA_SRC_OFFSET_X(x));
	if (id == ODMA_WB)
		dma_write(id, ODMA_DST_OFFSET, val);
	else
		dma_write(id, IDMA_SRC_OFFSET, val);
}

void dma_reg_set_img_size(u32 id, u32 w, u32 h)
{
	u32 val;
	val = (IDMA_IMG_HEIGHT(h) | IDMA_IMG_WIDTH(w));
	if (id == ODMA_WB) {
		dma_write(id, ODMA_OUT_IMG_SIZE, val);
		wb_reg_set_dst_size(id, w, h);
	} else {
		dma_write(id, IDMA_IMG_SIZE, val);
	}
}

void dma_reg_set_chroma_stride(u32 id, u32 stride)
{
	u32 val, mask;

	if (id == ODMA_WB)
		return;
	val = IDMA_CHROMA_STRIDE(stride);
	mask = IDMA_CHROMA_STRIDE_MASK;
	dma_write_mask(id, IDMA_CHROMINANCE_STRIDE, val, mask);
}

void dma_reg_set_block_offset(u32 id, u32 x, u32 y)
{
	u32 val;

	if (id == ODMA_WB)
		return;
	val = (IDMA_BLK_OFFSET_Y(y) | IDMA_BLK_OFFSET_X(x));
	dma_write(id, IDMA_BLOCK_OFFSET, val);
}

void dma_reg_set_block_size(u32 id, u32 w, u32 h)
{
	u32 val;

	if (id == ODMA_WB)
		return;
	val = (IDMA_BLK_HEIGHT(h) | IDMA_BLK_WIDTH(w));
	dma_write(id, IDMA_BLOCK_SIZE, val);
}

void dma_reg_set_perf_degradation_cfg(u32 id, u32 time, u32 ic_num)
{
	u32 val, mask;

	val = (IDMA_DEGRADATION_TIME(time) | IDMA_IN_IC_MAX_DEG(ic_num));
	mask = (IDMA_DEGRADATION_TIME_MASK | IDMA_IN_IC_MAX_DEG_MASK);
	dma_write_mask(id, IDMA_PERFORMANCE_CON0, val, mask);
}

void dma_reg_set_perf_degradation_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_PERFORMANCE_CON0, val, IDMA_DEGRADATION_EN);
}

void dma_reg_set_in_qos_lut(u32 id, u32 lut_id, u32 qos_t)
{
	u32 reg_id;

	if (id == ODMA_WB) {
		if (lut_id == 0)
			reg_id = ODMA_OUT_QOS_LUT07_00;
		else
			reg_id = ODMA_OUT_QOS_LUT15_08;
	} else {
		if (lut_id == 0)
			reg_id = IDMA_IN_QOS_LUT07_00;
		else
			reg_id = IDMA_IN_QOS_LUT15_08;
	}
	dma_write(id, reg_id, qos_t);
}

void dma_reg_set_in_base_addr(u32 id, u32 addr_y, u32 addr_c)
{
	if (id == ODMA_WB) {
		dma_write(id, ODMA_IN_BASE_ADDR_Y, addr_y);
		dma_write(id, ODMA_IN_BASE_ADDR_C, addr_c);
	} else {
		dma_write(id, IDMA_IN_BASE_ADDR_Y, addr_y);
		dma_write(id, IDMA_IN_BASE_ADDR_C, addr_c);
	}
}

/* (ODMA only) for SLICE_BYTE(n) */
void dma_reg_set_slice_byte_cnt(u32 id, u32 s_id, u32 b_cnt)
{
	if (id != ODMA_WB)
		return;
	dma_write(id, ODMA_SLICE_BYTE_CNT(s_id), b_cnt);
}

/* (ODMA only) b_cnt[8] : 0=slice0_cnt, ..., 6=slice6_cnt, 7=frame_cnt */
void dma_reg_set_slice_byte_cnt_all(u32 id, u32 b_cnt[8])
{
	u32 i;

	if (id != ODMA_WB)
		return;
	for (i = 0; i < 8; i++)
		dma_write(id, ODMA_SLICE_BYTE_CNT(i), b_cnt[i]);
}

/* ODMA only */
void dma_reg_set_usb_wb_path_sel(u32 id, u32 p_sel)
{
	u32 val;

	if (id != ODMA_WB)
		return;

	if (p_sel == USB_WB_PATH_OTF)
		val = ~0;
	else
		val = 0;
	dma_write_mask(id, ODMA_USB_TV_WB_CON, val, ODMA_USB_WB_PATH_SEL);
}

/* ODMA only */
void dma_reg_set_usb_wb_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	if (id != ODMA_WB)
		return;
	dma_write_mask(id, ODMA_USB_TV_WB_CON, val, ODMA_USB_WB_EN);
}

void dma_reg_set_deadlock_num(u32 id, u32 dl_num)
{
	u32 val, mask;

	val = IDMA_DEADLOCK_VAL(dl_num);
	mask = IDMA_DEADLOCK_VAL_MASK;
	if (id == ODMA_WB)
		dma_write_mask(id, ODMA_DEADLOCK_NUM, val, mask);
	else
		dma_write_mask(id, IDMA_DEADLOCK_NUM, val, mask);
}

void dma_reg_set_deadlock_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	if (id == ODMA_WB)
		dma_write_mask(id, ODMA_DEADLOCK_NUM, val, IDMA_DEADLOCK_EN);
	else
		dma_write_mask(id, IDMA_DEADLOCK_NUM, val, IDMA_DEADLOCK_EN);
}

void dma_reg_set_dynamic_gating_en_all(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;

	if (id == ODMA_WB) {
		mask = ODMA_DG_EN_ALL;
		dma_write_mask(id, ODMA_DYNAMIC_GATING_EN, val, mask);
	} else {
		mask = IDMA_DG_EN_ALL;
		dma_write_mask(id, IDMA_DYNAMIC_GATING_EN, val, mask);
	}
}

void dma_reg_set_recovery_num(u32 id, u32 rcv_num)
{
	u32 val, mask;

	val = IDMA_RECOVERY_NUM(rcv_num);
	mask = IDMA_RECOVERY_NUM_MASK;
	dma_write_mask(id, IDMA_RECOVERY_CTRL, val, mask);
}

void dma_reg_set_recovery_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dma_write_mask(id, IDMA_RECOVERY_CTRL, val, IDMA_RECOVERY_EN);
}

u32 dma_reg_get_cfg_err_state(u32 id)
{
	u32 val;

	if (id == ODMA_WB) {
		val = dma_read(id, ODMA_CFG_ERR_STATE);
		return ODMA_CFG_ERR_GET(val);
	} else {
		val = dma_read(id, IDMA_CFG_ERR_STATE);
		return IDMA_CFG_ERR_GET(val);
	}
}

int dpp_reg_wait_op_status(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	do {
		cfg = dpp_read(id, DPP_ENABLE);
		if (!(cfg & (DPP_OP_STATUS)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("[dpp] timeout op_status to idle\n");

	return -EBUSY;
}

u32 dpp_reg_get_op_status(u32 id)
{
	u32 val;

	/**
	 * DPP_OP_STATUS is commonly used at WB_MUX
	 *   because bit-field is same with WB_OP_STATUS
	 */
	if (id == ODMA_WB)
		val = dpp_read(id, DPU_WB_ENABLE);
	else
		val = dpp_read(id, DPP_ENABLE);
	if (val & DPP_OP_STATUS)
		return OP_STATUS_BUSY;

	return OP_STATUS_IDLE;
}

void dpp_reg_set_sw_reset(u32 id)
{
	dpp_write_mask(id, DPP_ENABLE, ~0, DPP_SRSET);

}

int dpp_reg_wait_sw_reset_status(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	do {
		cfg = dpp_read(id, DPP_ENABLE);
		if (!(cfg & (DPP_SRSET)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("[dpp] timeout sw reset\n");

	return -EBUSY;
}

void dpp_reg_set_clock_gate_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_ENABLE, val, DPP_ALL_CLOCK_GATE_EN_MASK);
}

void dpp_reg_set_sfr_update_force(u32 id)
{
	dpp_write_mask(id, DPP_ENABLE, ~0, DPP_SFR_UPDATE_FORCE);
}

/* Setting value : 0=Qch-enable, 1=Qch-disable */
void dpp_reg_set_qchannel_en(u32 id, u32 en)
{
	u32 val = en ? 0 : ~0;
	dpp_write_mask(id, DPP_ENABLE, val, DPP_QCHANNEL_EN);
}

/* G0_S only */
void dpp_reg_set_secure_mode(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_ENABLE, val, DPP_SECURE_MODE);
}

void dpp_reg_wait_pingpong_clear(u32 id)
{
	/* removed */
}

void dpp_reg_set_irq_clear_all(u32 id)
{
	dpp_write_mask(id, DPP_IRQ, DPP_ALL_IRQ_CLEAR, DPP_ALL_IRQ_CLEAR);
}

void dpp_reg_set_irq_mask_config_err(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_IRQ, val, DPP_CONFIG_ERROR_MASK);
}

void dpp_reg_set_irq_mask_framedone(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_IRQ, val, DPP_IRQ_FRAMEDONE_MASK);
}

void dpp_reg_set_irq_mask_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_IRQ, val, DPP_ALL_IRQ_MASK);
}

void dpp_reg_set_irq_enable(u32 id)
{
	dpp_write_mask(id, DPP_IRQ, ~0, DPP_IRQ_ENABLE);
}

void dpp_reg_set_csc_type(u32 id, u32 type, u32 range, u32 mode)
{
	u32 val, mask;

	val = (DPP_CSC_TYPE(type) | DPP_CSC_RANGE(range) | DPP_CSC_MODE(mode));
	mask = (DPP_CSC_TYPE_MASK | DPP_CSC_RANGE_MASK | DPP_CSC_MODE_MASK);
	dpp_write_mask(id, DPP_IN_CON, val, mask);
}

void dpp_reg_set_csc_coef(u32 id, u32 csc_std, u32 csc_rng)
{
	u32 val, mask;
	u32 csc_id = DPP_CSC_ID_BT_2020 + CSC_RANGE_LIMITED;
	u32 c00, c01, c02;
	u32 c10, c11, c12;
	u32 c20, c21, c22;

	if (csc_std == CSC_BT_2020)
		csc_id = DPP_CSC_ID_BT_2020 + csc_rng;
	else if (csc_std == CSC_DCI_P3)
		csc_id = DPP_CSC_ID_DCI_P3 + csc_rng;
	else
		dpp_err("Undefined CSC Type!!!\n");

	c00 = csc_3x3_t[csc_id][0][0];
	c01 = csc_3x3_t[csc_id][0][1];
	c02 = csc_3x3_t[csc_id][0][2];

	c10 = csc_3x3_t[csc_id][1][0];
	c11 = csc_3x3_t[csc_id][1][1];
	c12 = csc_3x3_t[csc_id][1][2];

	c20 = csc_3x3_t[csc_id][2][0];
	c21 = csc_3x3_t[csc_id][2][1];
	c22 = csc_3x3_t[csc_id][2][2];

	mask = (DPP_CSC_COEF_H_MASK | DPP_CSC_COEF_L_MASK);
	val = (DPP_CSC_COEF_H(c01) | DPP_CSC_COEF_L(c00));
	dpp_write_mask(id, DPP_CSC_COEF0, val, mask);

	val = (DPP_CSC_COEF_H(c10) | DPP_CSC_COEF_L(c02));
	dpp_write_mask(id, DPP_CSC_COEF1, val, mask);

	val = (DPP_CSC_COEF_H(c12) | DPP_CSC_COEF_L(c11));
	dpp_write_mask(id, DPP_CSC_COEF2, val, mask);

	val = (DPP_CSC_COEF_H(c21) | DPP_CSC_COEF_L(c20));
	dpp_write_mask(id, DPP_CSC_COEF3, val, mask);

	mask = DPP_CSC_COEF_L_MASK;
	val = DPP_CSC_COEF_L(c22);
	dpp_write_mask(id, DPP_CSC_COEF4, val, mask);

	dpp_dbg("---[CSC Type = %s_%s]---\n",
		csc_std == 3 ? "DCI_P3" : "BT_2020",
		csc_rng == 0 ? "LTD" : "FULL");
	dpp_dbg("0x%3x  0x%3x  0x%3x\n", c00, c01, c02);
	dpp_dbg("0x%3x  0x%3x  0x%3x\n", c10, c11, c12);
	dpp_dbg("0x%3x  0x%3x  0x%3x\n", c20, c21, c22);

}

void dpp_reg_set_csc_config(u32 id, u32 type)
{
	u32 csc_std = CSC_BT_601;
	u32 csc_rng = CSC_RANGE_FULL;
	u32 csc_type = (CSC_BT_601 | CSC_RANGE_LIMITED);
	u32 coef_mode = CSC_COEF_HARDWIRED;

	if ((id == IDMA_G0) || (id == IDMA_G1) || (id == IDMA_G0_S))
		return;

	csc_std = (type >> CSC_STANDARD_SHIFT) & 0x3F;
	csc_rng = (type >> CSC_RANGE_SHIFT) & 0x7;

	if (csc_std < CSC_BT_2020)
		coef_mode = CSC_COEF_HARDWIRED;
	else
		coef_mode = CSC_COEF_CUSTOMIZED;

	if (id == ODMA_WB) {
		/* only support {601, 709, N, W} */
		csc_type = (csc_std << 1) | (csc_rng << 0);
		if (csc_type > 3) {
			dpp_info("[WB] Unsupported CSC(%d) !\n", csc_type);
			dpp_info("[WB] -> forcing BT_601_LIMITTED\n");
			csc_type = ((CSC_BT_601 << 1) | CSC_RANGE_LIMITED);
		}
		wb_reg_set_rgb_type(id, csc_type);
		return;
	}

	dpp_reg_set_csc_type(id, csc_std, csc_rng, coef_mode);
	if (coef_mode != CSC_COEF_HARDWIRED)
		dpp_reg_set_csc_coef(id, csc_std, csc_rng);

}

/* [fmt] 0=ARGB8888, 1=NV12/NV21 */
void dpp_reg_set_img_format(u32 id, u32 fmt)
{
	u32 val;
	val = DPP_IMG_FORMAT(fmt);
	dpp_write_mask(id, DPP_IN_CON, val, DPP_IMG_FORMAT_MASK);
}

/* [type] 0=per-frame, 1=per-pixel */
void dpp_reg_set_alpha_sel(u32 id, u32 type)
{
	u32 val;
	val = DPP_ALPHA_SEL(type);
	dpp_write_mask(id, DPP_IN_CON, val, DPP_ALPHA_SEL_MASK);
}

void dpp_reg_set_img_size(u32 id, u32 w, u32 h)
{
	u32 val;
	val = (DPP_IMG_HEIGHT(h) | DPP_IMG_WIDTH(w));
	dpp_write(id, DPP_IMG_SIZE, val);
}

void dpp_reg_set_scaled_img_size(u32 id, u32 w, u32 h)
{
	u32 val;
	val = (DPP_SCALED_IMG_HEIGHT(h) | DPP_SCALED_IMG_WIDTH(w));
	dpp_write(id, DPP_SCALED_IMG_SIZE, val);
}

void dpp_reg_set_h_ratio(u32 id, u32 h_ratio)
{
	u32 val;

	val = DPP_H_RATIO(h_ratio);
	dpp_write_mask(id, DPP_MAIN_H_RATIO, val, DPP_H_RATIO_MASK);
}

void dpp_reg_set_v_ratio(u32 id, u32 v_ratio)
{
	u32 val;

	val = DPP_V_RATIO(v_ratio);
	dpp_write_mask(id, DPP_MAIN_V_RATIO, val, DPP_V_RATIO_MASK);
}

void dpp_reg_set_h_coef(u32 id, u32 h_ratio)
{
	int i, j, k, sc_ratio;

	if (h_ratio <= DPP_SC_RATIO_MAX)
		sc_ratio = 0;
	else if (h_ratio <= DPP_SC_RATIO_7_8)
		sc_ratio = 1;
	else if (h_ratio <= DPP_SC_RATIO_6_8)
		sc_ratio = 2;
	else if (h_ratio <= DPP_SC_RATIO_5_8)
		sc_ratio = 3;
	else if (h_ratio <= DPP_SC_RATIO_4_8)
		sc_ratio = 4;
	else if (h_ratio <= DPP_SC_RATIO_3_8)
		sc_ratio = 5;
	else
		sc_ratio = 6;

	for (i = 0; i < 9; i++) {
		for (j = 0; j < 8; j++) {
			for (k = 0; k < 2; k++) {
				dpp_write(id, DPP_H_COEF(i, j, k),
						h_coef_8t[sc_ratio][i][j]);
			}
		}
	}
}

void dpp_reg_set_v_coef(u32 id, u32 v_ratio)
{
	int i, j, k, sc_ratio;

	if (v_ratio <= DPP_SC_RATIO_MAX)
		sc_ratio = 0;
	else if (v_ratio <= DPP_SC_RATIO_7_8)
		sc_ratio = 1;
	else if (v_ratio <= DPP_SC_RATIO_6_8)
		sc_ratio = 2;
	else if (v_ratio <= DPP_SC_RATIO_5_8)
		sc_ratio = 3;
	else if (v_ratio <= DPP_SC_RATIO_4_8)
		sc_ratio = 4;
	else if (v_ratio <= DPP_SC_RATIO_3_8)
		sc_ratio = 5;
	else
		sc_ratio = 6;

	for (i = 0; i < 9; i++) {
		for (j = 0; j < 4; j++) {
			for (k = 0; k < 2; k++) {
				dpp_write(id, DPP_V_COEF(i, j, k),
						v_coef_4t[sc_ratio][i][j]);
			}
		}
	}
}

int dpp_reg_set_rotation(u32 id, struct dpp_params_info *p)
{
	/* Not support */
	return 0;
}

void dpp_reg_set_scale_ratio(u32 id, struct dpp_params_info *p)
{
	dpp_reg_set_h_ratio(id, p->h_ratio);
	dpp_reg_set_h_coef(id, p->h_ratio);

	dpp_reg_set_v_ratio(id, p->v_ratio);
	dpp_reg_set_v_coef(id, p->v_ratio);

	dpp_dbg("h_ratio : %#x, v_ratio : %#x\n", p->h_ratio, p->v_ratio);
}

void dpp_reg_set_yh_pos(u32 id, u32 i_part, u32 f_part)
{
	u32 val;
	val = (DPP_POS_I(i_part) | DPP_POS_F(f_part));
	dpp_write(id, DPP_YHPOSITION, val);
}

void dpp_reg_set_yv_pos(u32 id, u32 i_part, u32 f_part)
{
	u32 val;
	val = (DPP_POS_I(i_part) | DPP_POS_F(f_part));
	dpp_write(id, DPP_YVPOSITION, val);
}

void dpp_reg_set_ch_pos(u32 id, u32 i_part, u32 f_part)
{
	u32 val;
	val = (DPP_POS_I(i_part) | DPP_POS_F(f_part));
	dpp_write(id, DPP_CHPOSITION, val);
}

void dpp_reg_set_cv_pos(u32 id, u32 i_part, u32 f_part)
{
	u32 val;
	val = (DPP_POS_I(i_part) | DPP_POS_F(f_part));
	dpp_write(id, DPP_CVPOSITION, val);
}

void dpp_reg_set_scale_start_position(u32 id, u32 yh, u32 yv, u32 ch, u32 cv)
{
	dpp_reg_set_yh_pos(id, DPP_POS_I_GET(yh), DPP_POS_F_GET(yh));
	dpp_reg_set_yv_pos(id, DPP_POS_I_GET(yv), DPP_POS_F_GET(yv));
	dpp_reg_set_ch_pos(id, DPP_POS_I_GET(ch), DPP_POS_F_GET(ch));
	dpp_reg_set_cv_pos(id, DPP_POS_I_GET(cv), DPP_POS_F_GET(cv));
}

/*
 * This function calculates initial phase for scaling.
 * range = (-1, 4)
 *
 * <EQUATION for RGB>
 *        (S - D) / (2*D)
 *
 * [fmt_id]
 *  0 = RGB
 *  1 = YUV420 : x2 for both chroma_width and chroma_height (to 444)
 *
 * [return]
 *  calculated_phase (12.20 format)
 *   yc_phase[0] : YH
 *   yc_phase[1] : YV
 *   yc_phase[2] : CH
 *   yc_phase[3] : CV
*/
void dpp_calc_recommend_initial_phase(u32 fmt_id,
		u32 sw, u32 sh, u32 dw, u32 dh, u32 yc_phase[4])
{
	switch (fmt_id) {
	/* RGB */
	case 0:
		yc_phase[0] = (((s64)sw - (s64)dw) * 0x100000) / (2 * dw);
		yc_phase[1] = (((s64)sh - (s64)dh) * 0x100000) / (2 * dh);
		yc_phase[2] = (((s64)sw - (s64)dw) * 0x100000) / (2 * dw);
		yc_phase[3] = (((s64)sh - (s64)dh) * 0x100000) / (2 * dh);
		break;
	/* 420 */
	case 1:
		yc_phase[0] = (((s64)sw - (s64)dw) * 0x100000) / (2 * dw);
		yc_phase[1] = (((s64)sh - (s64)dh) * 0x100000) / (2 * dh);
		yc_phase[2] = (((s64)sw - (s64)(2*dw)) * 0x100000) / (4 * dw);
		yc_phase[3] = (((s64)sh - (s64)(2*dh)) * 0x100000) / (4 * dh);
		break;
	/* unknown format */
	default:
		break;
	}
}

void dpp_reg_set_she_on(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_SHE_CON0, val, DPP_SHE_ON_MASK);
}

void dpp_reg_set_she_gain(u32 id, u32 val)
{
	dpp_write_mask(id, DPP_SHE_CON0, DPP_GAIN(val), DPP_GAIN_MASK);
}

void dpp_reg_set_she_ec_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_SHE_CON0, val, DPP_EC_EN_MASK);
}

void dpp_reg_set_she_ec_threshold(u32 id, u32 n32, u32 n16,
		u32 zr, u32 p16, u32 p32)
{
	u32 val;

	val = DPP_EC_TH_N32(n32);
	dpp_write_mask(id, DPP_SHE_CON0, val, DPP_EC_TH_N32_MASK);

	val = (DPP_EC_TH_N16(n16) | DPP_EC_TH_0(zr)
		| DPP_EC_TH_P16(p16) | DPP_EC_TH_P32(p32));
	dpp_write(id, DPP_SHE_CON1, val);
}

void dpp_reg_set_she_sc_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_SHE_CON2, val, DPP_SC_EN_MASK);
}

void dpp_reg_set_she_sc_threshold(u32 id, u32 th1, u32 th2, u32 th3)
{
	u32 val, mask;

	val = (DPP_SC_TH1(th1) | DPP_SC_TH2(th2) | DPP_SC_TH3(th3));
	mask = (DPP_SC_TH1_MASK | DPP_SC_TH2_MASK | DPP_SC_TH3_MASK);
	dpp_write(id, DPP_SHE_CON2, val);
}

void dpp_reg_set_dynamic_gating_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_DYNAMIC_GATING_EN, val, DPP_DG_EN_ALL);
}

void dpp_reg_set_linecnt_capture(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_LINECNT_CON, val, DPP_LC_CAPTURE_MASK);
}

void dpp_reg_set_linecnt_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;
	dpp_write_mask(id, DPP_LINECNT_CON, val, DPP_LC_ENABLE_MASK);
}

void dpp_reg_set_linecnt_counter(u32 id, u32 cnt)
{
	u32 val;

	val = DPP_LC_COUNTER(cnt);
	dpp_write_mask(id, DPP_LINECNT_VAL, val, DPP_LC_COUNTER_MASK);
}

u32 dpp_reg_get_linecnt_counter(u32 id)
{
	u32 val = dpp_read(id, DPP_LINECNT_VAL);
	return DPP_LC_COUNTER_GET(val);
}

u32 dpp_reg_get_cfg_err_state(u32 id)
{
	u32 val = dpp_read(id, DPP_CFG_ERR_STATE);
	return DPP_CFG_ERR_GET(val);
}

bool dpp_reg_check_cfg_err(u32 id, enum dpp_cfg_err err_id)
{
	u32 val = dpp_read(id, DPP_CFG_ERR_STATE);

	if (DPP_CFG_ERR_GET(val) & err_id)
		return 1;
	else
		return 0;
}

/* [mode] 0=runtime changed, 1=captured */
void dpp_reg_control_linecnt(u32 id, u32 en, u32 mode)
{
	dpp_reg_set_linecnt_capture(id, mode);
	dpp_reg_set_linecnt_en(id, en);
}

int dpp_reg_set_format(u32 id, struct dpp_params_info *p)
{
	u32 fmt;
	/* 0=per-frame, 1=per-pixel */
	u32 a_sel = 0;
	/* 0=ARGB, 1=YUV */
	u32 fmt_type = 0;
	/* [WB] chroma x-/y-offset when R2Y : [0, 4] */
	u32 off_x = 0;
	u32 off_y = 0;

	switch (p->format) {
	case DECON_PIXEL_FORMAT_ARGB_8888:
		fmt = IDMA_IMG_FORMAT_ARGB8888;
		a_sel = 1;
		break;
	case DECON_PIXEL_FORMAT_ABGR_8888:
		fmt = IDMA_IMG_FORMAT_ABGR8888;
		a_sel = 1;
		break;
	case DECON_PIXEL_FORMAT_RGBA_8888:
		fmt = IDMA_IMG_FORMAT_RGBA8888;
		a_sel = 1;
		break;
	case DECON_PIXEL_FORMAT_BGRA_8888:
		fmt = IDMA_IMG_FORMAT_BGRA8888;
		a_sel = 1;
		break;
	case DECON_PIXEL_FORMAT_XRGB_8888:
		fmt = IDMA_IMG_FORMAT_XRGB8888;
		break;
	case DECON_PIXEL_FORMAT_XBGR_8888:
		fmt = IDMA_IMG_FORMAT_XBGR8888;
		break;
	case DECON_PIXEL_FORMAT_RGBX_8888:
		fmt = IDMA_IMG_FORMAT_RGBX8888;
		break;
	case DECON_PIXEL_FORMAT_BGRX_8888:
		fmt = IDMA_IMG_FORMAT_BGRX8888;
		break;
	case DECON_PIXEL_FORMAT_RGB_565:
		fmt = IDMA_IMG_FORMAT_RGB565;
		break;
	/* TODO: add ARGB1555 & ARGB4444 */
	case DECON_PIXEL_FORMAT_NV12:
	case DECON_PIXEL_FORMAT_NV12M:
		fmt = IDMA_IMG_FORMAT_YUV420_2P;
		fmt_type = 1;
		break;
	case DECON_PIXEL_FORMAT_NV21:
	case DECON_PIXEL_FORMAT_NV21M:
	case DECON_PIXEL_FORMAT_NV12N:
		fmt = IDMA_IMG_FORMAT_YVU420_2P;
		fmt_type = 1;
		break;
	default:
		dpp_err("Unsupported Format\n");
		return -EINVAL;
	}

	dma_reg_set_img_format(id, fmt);
	dpp_reg_set_alpha_sel(id, a_sel);
	dpp_reg_set_img_format(id, fmt_type);
	if (id == ODMA_WB) {
		if (fmt_type) {
			wb_reg_set_csc_r2y_en(id, 1);
			wb_reg_set_uv_offset(id, off_x, off_y);
		} else {
			wb_reg_set_csc_r2y_en(id, 0);
			wb_reg_set_uv_offset(id, 0, 0);
		}
	}

	dma_reg_set_afbc_en(id, p->is_comp);

	dma_reg_set_recovery_en(id, 0);

	return 0;
}

void dpp_reg_set_buf_addr(u32 id, struct dpp_params_info *p)
{
	dpp_dbg("y : %llu, cb : %llu, cr : %llu\n",
			p->addr[0], p->addr[1], p->addr[2]);

	/* For AFBC stream, BASE_ADDR_C must be same with BASE_ADDR_Y */
	if (p->is_comp == 0)
		dma_reg_set_in_base_addr(id, p->addr[0], p->addr[1]);
	else
		dma_reg_set_in_base_addr(id, p->addr[0], p->addr[0]);
}

void dpp_reg_set_size(u32 id, struct dpp_params_info *p)
{
	/* source offset */
	dma_reg_set_buf_offset(id, p->src.x, p->src.y);

	/* source full(alloc) size */
	dma_reg_set_buf_size(id, p->src.f_w, p->src.f_h);

	/* source cropped size */
	dma_reg_set_img_size(id, p->src.w, p->src.h);
	dpp_reg_set_img_size(id, p->src.w, p->src.h);

	if ((id == IDMA_VGF0) || (id == IDMA_VGF1))
		dpp_reg_set_scaled_img_size(id, p->dst.w, p->dst.h);
}

void dpp_reg_set_block_area(u32 id, struct dpp_params_info *p)
{
	if (!p->is_block) {
		dma_reg_set_block_en(id, 0);
		return;
	}

	dma_reg_set_block_offset(id, p->block.x, p->block.y);
	dma_reg_set_block_size(id, p->block.w, p->block.h);
	dma_reg_set_block_en(id, 1);

	dpp_dbg("block x : %d, y : %d, w : %d, h : %d\n",
			p->block.x, p->block.y, p->block.w, p->block.h);
}

void dpp_reg_set_plane_alpha(u32 id, u32 plane_alpha)
{
	if (plane_alpha > 0xFF)
		dpp_info("%d is too big value\n", plane_alpha);
	dma_reg_set_out_frame_alpha(id, plane_alpha);
	if (id == ODMA_WB)
		wb_reg_set_out_frame_alpha(id, plane_alpha);
}

void dpp_reg_set_plane_alpha_fixed(u32 id)
{
	dma_reg_set_out_frame_alpha(id, 0xFF);
	if (id == ODMA_WB)
		wb_reg_set_out_frame_alpha(id, 0xFF);
}

void dpp_reg_set_lookup_table(u32 id)
{
	dma_reg_set_in_qos_lut(id, 0, 0x44444444);
	dma_reg_set_in_qos_lut(id, 1, 0x44444444);
}

void dpp_reg_set_dynamic_clock_gating(u32 id)
{
	dma_reg_set_dynamic_gating_en_all(id, 1);
	dpp_reg_set_dynamic_gating_en_all(id, 1);
	if (id == ODMA_WB)
		wb_reg_set_dynamic_gating_en_all(id, 1);
}

u32 dpp_reg_get_irq_status(u32 id)
{
	u32 cfg = dpp_read(id, DPP_IRQ);
	cfg &= DPP_ALL_IRQ_CLEAR;
	return cfg;
}

void dpp_reg_clear_irq(u32 id, u32 irq)
{
	dpp_write_mask(id, DPP_IRQ, ~0, irq);
}

void dpp_constraints_params(struct dpp_size_constraints *vc,
		struct dpp_img_format *vi)
{
	u32 sz_align = 1;

	if (vi->yuv)
		sz_align = 2;

	vc->src_mul_w = SRC_SIZE_MULTIPLE * sz_align;
	vc->src_mul_h = SRC_SIZE_MULTIPLE * sz_align;
	vc->src_w_min = SRC_WIDTH_MIN * sz_align;
	vc->src_w_max = SRC_WIDTH_MAX;
	vc->src_h_min = SRC_HEIGHT_MIN;
	vc->src_h_max = SRC_HEIGHT_MAX;
	vc->img_mul_w = IMG_SIZE_MULTIPLE * sz_align;
	vc->img_mul_h = IMG_SIZE_MULTIPLE * sz_align;
	vc->img_w_min = IMG_WIDTH_MIN * sz_align;
	vc->img_w_max = IMG_WIDTH_MAX;
	vc->img_h_min = IMG_HEIGHT_MIN;
	vc->img_h_max = IMG_HEIGHT_MAX;
	vc->src_mul_x = SRC_OFFSET_MULTIPLE * sz_align;
	vc->src_mul_y = SRC_OFFSET_MULTIPLE * sz_align;

	vc->sca_w_min = SCALED_WIDTH_MIN;
	vc->sca_w_max = SCALED_WIDTH_MAX;
	vc->sca_h_min = SCALED_HEIGHT_MIN;
	vc->sca_h_max = SCALED_HEIGHT_MAX;
	vc->sca_mul_w = SCALED_SIZE_MULTIPLE;
	vc->sca_mul_h = SCALED_SIZE_MULTIPLE;

	vc->blk_w_min = BLK_WIDTH_MIN;
	vc->blk_w_max = BLK_WIDTH_MAX;
	vc->blk_h_min = BLK_HEIGHT_MIN;
	vc->blk_h_max = BLK_HEIGHT_MAX;
	vc->blk_mul_w = BLK_SIZE_MULTIPLE;
	vc->blk_mul_h = BLK_SIZE_MULTIPLE;

	if (vi->wb) {
		vc->src_mul_w = DST_SIZE_MULTIPLE * sz_align;
		vc->src_mul_h = DST_SIZE_MULTIPLE * sz_align;
		vc->src_w_min = DST_SIZE_WIDTH_MIN;
		vc->src_w_max = DST_SIZE_WIDTH_MAX;
		vc->src_h_min = DST_SIZE_HEIGHT_MIN;
		vc->src_h_max = DST_SIZE_HEIGHT_MAX;
		vc->img_mul_w = DST_IMG_MULTIPLE * sz_align;
		vc->img_mul_h = DST_IMG_MULTIPLE * sz_align;
		vc->img_w_min = DST_IMG_WIDTH_MIN;
		vc->img_w_max = DST_IMG_WIDTH_MAX;
		vc->img_h_min = DST_IMG_HEIGHT_MIN;
		vc->img_h_max = DST_IMG_HEIGHT_MAX;
		vc->src_mul_x = DST_OFFSET_MULTIPLE * sz_align;
		vc->src_mul_y = DST_OFFSET_MULTIPLE * sz_align;
	}

}

int dpp_reg_wait_idle_status(int id, unsigned long timeout)
{
	u32 dpp_status = 0;
	u32 dma_status = 0;
	unsigned long delay_time = 10;
	unsigned long cnt = timeout / delay_time;

	while (cnt) {
		dpp_status = dpp_reg_get_op_status(id);
		dma_status = dma_reg_get_op_status(id);

		if ((dpp_status == OP_STATUS_IDLE)
			&& (dma_status == OP_STATUS_IDLE))
			break;

		cnt--;
		udelay(delay_time);
	};

	if (!cnt) {
		if (dpp_status)
			dpp_err("[dpp%d] timeout op_status to idle\n", id);
		if (dma_status)
			dpp_err("[dma%d] timeout op_status to idle\n", id);
		return -EBUSY;
	}

	return 0;
}

void dpp_reg_init(u32 id)
{
	dma_reg_set_irq_mask_all(id, 0);
	dpp_reg_set_irq_mask_all(id, 0);
	dma_reg_set_irq_enable(id);
	dpp_reg_set_irq_enable(id);

	dma_reg_set_clock_gate_en_all(id, 1);
	if (id == ODMA_WB)
		wb_reg_set_clock_gate_en_all(id, 1);
	dpp_reg_set_clock_gate_en_all(id, 1);

	dpp_reg_set_lookup_table(id);
	dpp_reg_set_dynamic_clock_gating(id);
	dpp_reg_set_plane_alpha_fixed(id);

	dpp_reg_control_linecnt(id, 1, 0);
	dma_reg_set_recovery_num(id, INIT_RCV_NUM);
}

int dpp_reg_deinit(u32 id, bool reset)
{
	dma_reg_clear_irq_all(id);
	dpp_reg_set_irq_clear_all(id);

	dma_reg_set_irq_mask_all(id, 1);
	dpp_reg_set_irq_mask_all(id, 1);

	if (reset) {
		dma_reg_set_sw_reset(id);
		dpp_reg_set_sw_reset(id);

		if (dpp_reg_wait_sw_reset_status(id)
			&& dma_reg_wait_sw_reset_status(id))
			return -EBUSY;
		if (id == ODMA_WB)
			wb_reg_set_sw_reset(id);
	}

	return 0;
}


void dpp_reg_configure_params(u32 id, struct dpp_params_info *p)
{
	dpp_reg_set_csc_config(id, p->eq_mode);
	dpp_reg_set_scale_ratio(id, p);
	dpp_reg_set_size(id, p);
	dma_reg_set_in_flip(id, p->flip);
	dpp_reg_set_buf_addr(id, p);
	dpp_reg_set_block_area(id, p);
	dpp_reg_set_format(id, p);
}
