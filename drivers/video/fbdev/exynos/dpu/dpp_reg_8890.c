/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * SFR access functions for Exynos DPP driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

int dpp_reg_wait_op_status(u32 id)
{
	u32 cfg = 0;

	unsigned long cnt = 100000;

	do {
		cfg = dpp_read(id, VG_ENABLE);
		if (!(cfg & (VG_ENABLE_OP_STATUS)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("timeout op_status to idle\n");

	return -EBUSY;
}

void dpp_reg_wait_idle(u32 id)
{
	u32 cfg = 0;

	unsigned long cnt = 100000;

	do {
		cfg = dpp_read(id, VG_ENABLE);
		if (!(cfg & (VG_ENABLE_OP_STATUS)))
			return;
		dpp_info("dpp%d is operating...\n", id);
		udelay(10);
	} while (--cnt);

	dpp_err("timeout op_status to idle\n");
}

int dpp_reg_set_sw_reset(u32 id)
{
	u32 cfg = 0;

	unsigned long cnt = 100000;
	dpp_write_mask(id, VG_ENABLE, ~0, VG_ENABLE_SRESET);

	do {
		cfg = dpp_read(id, VG_ENABLE);
		if (!(cfg & (VG_ENABLE_SRESET)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("timeout sw reset\n");

	return -EBUSY;
}

void dpp_reg_wait_pingpong_clear(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 1700;

	do {
		cfg = dpp_read(id, VG_PINGPONG_UPDATE);
		if (!(cfg & (VG_ADDR_PINGPONG_UPDATE)))
			return;
		udelay(10);
	} while (--cnt);
	dpp_err("timeout of DPP(%d) pingpong_clear\n", id);
}

void dpp_reg_set_realtime_path(u32 id)
{
	dpp_write_mask(id, VG_ENABLE, ~0, VG_ENABLE_RT_PATH_EN);
}

void dpp_reg_set_framedone_irq(u32 id, u32 enable)
{
	u32 val = enable ? ~0 : 0;
	dpp_write_mask(id, VG_IRQ, val, VG_IRQ_FRAMEDONE_MASK);
}

void dpp_reg_set_deadlock_irq(u32 id, u32 enable)
{
	u32 val = enable ? ~0 : 0;
	dpp_write_mask(id, VG_IRQ, val, VG_IRQ_DEADLOCK_STATUS_MASK);
}

void dpp_reg_set_read_slave_err_irq(u32 id, u32 enable)
{
	u32 val = enable ? ~0 : 0;
	dpp_write_mask(id, VG_IRQ, val, VG_IRQ_READ_SLAVE_ERROR_MASK);
}

void dpp_reg_set_sfr_update_force(u32 id)
{
	dpp_write_mask(id, VG_ENABLE, ~0, VG_ENABLE_SFR_UPDATE_FORCE);
}

void dpp_reg_set_enable_interrupt(u32 id)
{
	dpp_write_mask(id, VG_IRQ, ~0, VG_IRQ_ENABLE);
}

void dpp_reg_set_hw_reset_done_mask(u32 id, u32 enable)
{
	u32 val = enable ? ~0 : 0;
	dpp_write_mask(id, VG_IRQ, val, VG_IRQ_HW_RESET_DONE_MASK);
}

void dpp_reg_set_in_afbc_en(u32 id, u32 enable)
{
	u32 val = enable ? ~0 : 0;
	dpp_write_mask(id, VG_IN_CON, val, VG_IN_CON_IN_AFBC_EN);
}

void dpp_reg_set_format(u32 id, struct dpp_params_info *p)
{
	u32 cfg = dpp_read(id, VG_IN_CON);

	cfg &= ~(VG_IN_CON_IMG_FORMAT_MASK | VG_IN_CON_CHROMINANCE_STRIDE_EN);

	switch (p->format) {
	case DECON_PIXEL_FORMAT_ARGB_8888:
		cfg |= VG_IN_CON_IMG_FORMAT_ARGB8888;
		break;
	case DECON_PIXEL_FORMAT_ABGR_8888:
		cfg |= VG_IN_CON_IMG_FORMAT_ABGR8888;
		break;
	case DECON_PIXEL_FORMAT_RGBA_8888:
		cfg |= VG_IN_CON_IMG_FORMAT_RGBA8888;
		break;
	case DECON_PIXEL_FORMAT_BGRA_8888:
		cfg |= VG_IN_CON_IMG_FORMAT_BGRA8888;
		break;
	case DECON_PIXEL_FORMAT_XRGB_8888:
		cfg |= VG_IN_CON_IMG_FORMAT_XRGB8888;
		break;
	case DECON_PIXEL_FORMAT_XBGR_8888:
		cfg |= VG_IN_CON_IMG_FORMAT_XBGR8888;
		break;
	case DECON_PIXEL_FORMAT_RGBX_8888:
		cfg |= VG_IN_CON_IMG_FORMAT_RGBX8888;
		break;
	case DECON_PIXEL_FORMAT_BGRX_8888:
		cfg |= VG_IN_CON_IMG_FORMAT_BGRX8888;
		break;
	case DECON_PIXEL_FORMAT_RGB_565:
		cfg |= VG_IN_CON_IMG_FORMAT_RGB565;
		break;
	case DECON_PIXEL_FORMAT_NV16:
		cfg |= VG_IN_CON_IMG_FORMAT_YUV422_2P;
		break;
	case DECON_PIXEL_FORMAT_NV61:
		cfg |= VG_IN_CON_IMG_FORMAT_YVU422_2P;
		break;
	case DECON_PIXEL_FORMAT_NV12:
	case DECON_PIXEL_FORMAT_NV12M:
		cfg |= VG_IN_CON_IMG_FORMAT_YUV420_2P;
		break;
	case DECON_PIXEL_FORMAT_NV21:
	case DECON_PIXEL_FORMAT_NV21M:
	case DECON_PIXEL_FORMAT_NV12N:
		cfg |= VG_IN_CON_IMG_FORMAT_YVU420_2P;
		break;
	default:
		break;
	}

	dpp_write(id, VG_IN_CON, cfg);

	dpp_reg_set_in_afbc_en(id, p->is_comp);
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
				dpp_write(id, VG_H_COEF(i, j, k),
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
				dpp_write(id, VG_V_COEF(i, j, k),
						v_coef_4t[sc_ratio][i][j]);
			}
		}
	}
}

int dpp_reg_set_rotation(u32 id, struct dpp_params_info *p)
{
	dpp_write_mask(id, VG_IN_CON, p->is_rot << 8, VG_IN_CON_IN_ROTATION_MASK);
	if (p->is_rot > 0)
		dpp_write_mask(id, VG_IN_CON, VG_IN_CON_IN_IC_MAX, VG_IN_CON_IN_IC_MAX);
	else
		dpp_write_mask(id, VG_IN_CON, VG_IN_CON_IN_IC_MAX_DEFAULT, VG_IN_CON_IN_IC_MAX);

	return 0;
}

void dpp_reg_set_scale_ratio(u32 id, struct dpp_params_info *p)
{
	dpp_write(id, VG_H_RATIO, p->h_ratio);
	dpp_reg_set_h_coef(id, p->h_ratio);
	dpp_write(id, VG_V_RATIO, p->v_ratio);
	dpp_reg_set_v_coef(id, p->v_ratio);

	dpp_dbg("h_ratio : %#x, v_ratio : %#x\n", p->h_ratio, p->v_ratio);
}

void dpp_reg_set_buf_addr(u32 id, struct dpp_params_info *p)
{
	dpp_dbg("y : %llu, cb : %llu, cr : %llu\n",
			p->addr[0], p->addr[1], p->addr[2]);

	dpp_write(id, VG_BASE_ADDR_Y(0), p->addr[0]);
	/* When processing the AFBC data, BASE_ADDR_Y and
	 * BASE_ADDR_CB should be set to the same address.
	 */
	if (p->is_comp == 0)
		dpp_write(id, VG_BASE_ADDR_CB(0), p->addr[1]);
	else
		dpp_write(id, VG_BASE_ADDR_CB(0), p->addr[0]);

	dpp_write(id, VG_PINGPONG_UPDATE, VG_ADDR_PINGPONG_UPDATE);
}

void dpp_reg_set_size(u32 id, struct dpp_params_info *p)
{
	u32 cfg = 0;

	/* source offset */
	cfg = VG_SRC_OFFSET_X(p->src.x) | VG_SRC_OFFSET_Y(p->src.y);
	dpp_write(id, VG_SRC_OFFSET, cfg);

	/* source full(alloc) size */
	cfg = VG_SRC_SIZE_WIDTH(p->src.f_w) | VG_SRC_SIZE_HEIGHT(p->src.f_h);
	dpp_write(id, VG_SRC_SIZE, cfg);

	/* source cropped size */
	cfg = VG_IMG_SIZE_WIDTH(p->src.w) | VG_IMG_SIZE_HEIGHT(p->src.h);
	dpp_write(id, VG_IMG_SIZE, cfg);

	cfg = VG_SCALED_SIZE_WIDTH(p->dst.w) | VG_SCALED_SIZE_HEIGHT(p->dst.h);
	dpp_write(id, VG_SCALED_SIZE, cfg);

	dpp_write(id, VG_SMART_IF_PIXEL_NUM, p->dst.w * p->dst.h);
}

void dpp_reg_set_block_area(u32 id, struct dpp_params_info *p)
{
	u32 cfg = 0;

	if (!p->is_block) {
		dpp_write_mask(id, VG_IN_CON, 0, VG_IN_CON_BLOCKING_FEATURE_EN);
		return;
	}

	/* blocking area offset */
	cfg = VG_BLK_OFFSET_X(p->block.x) | VG_BLK_OFFSET_Y(p->block.y);
	dpp_write(id, VG_BLK_OFFSET, cfg);

	/* blocking area size */
	cfg = VG_BLK_SIZE_WIDTH(p->block.w) | VG_BLK_SIZE_HEIGHT(p->block.h);
	dpp_write(id, VG_BLK_SIZE, cfg);

	dpp_write_mask(id, VG_IN_CON, ~0, VG_IN_CON_BLOCKING_FEATURE_EN);

	dpp_dbg("block x : %d, y : %d, w : %d, h : %d\n",
			p->block.x, p->block.y, p->block.w, p->block.h);
}

void dpp_reg_set_out_size(u32 id, u32 dst_w, u32 dst_h)
{
	u32 cfg = 0;

	/* destination scaled size */
	cfg = VG_SCALED_SIZE_WIDTH(dst_w) | VG_SCALED_SIZE_HEIGHT(dst_h);
	dpp_write(id, VG_SCALED_SIZE, cfg);
}

void dpp_reg_set_rgb_type(u32 id, u32 type)
{
	u32 csc_eq = 0;

	switch (type) {
	case BT_601_NARROW:
		csc_eq = VG_OUT_CON_RGB_TYPE_601_NARROW;
		break;
	case BT_601_WIDE:
		csc_eq = VG_OUT_CON_RGB_TYPE_601_WIDE;
		break;
	case BT_709_NARROW:
		csc_eq = VG_OUT_CON_RGB_TYPE_709_NARROW;
		break;
	case BT_709_WIDE:
		csc_eq = VG_OUT_CON_RGB_TYPE_709_WIDE;
		break;
	default:
		dpp_err("Unsupported CSC Equation\n");
	}

	dpp_write(id, VG_OUT_CON, csc_eq);
}

void dpp_reg_set_plane_alpha(u32 id, u32 plane_alpha)
{
	if (plane_alpha > 0xFF)
		dpp_info("%d is too much value\n", plane_alpha);
	dpp_write_mask(id, VG_OUT_CON, VG_OUT_CON_FRAME_ALPHA(plane_alpha),
			VG_OUT_CON_FRAME_ALPHA_MASK);
}

void dpp_reg_set_plane_alpha_fixed(u32 id)
{
	dpp_write_mask(id, VG_OUT_CON, VG_OUT_CON_FRAME_ALPHA(0xFF),
			VG_OUT_CON_FRAME_ALPHA_MASK);
}

void dpp_reg_set_smart_if_pix_num(u32 id, u32 dst_w, u32 dst_h)
{
	dpp_write(id, VG_SMART_IF_PIXEL_NUM, dst_w * dst_h);
}

void dpp_reg_set_lookup_table(u32 id)
{
	dpp_write(id, VG_QOS_LUT07_00, 0x44444444);
	dpp_write(id, VG_QOS_LUT15_08, 0x44444444);
}

void dpp_reg_set_dynamic_clock_gating(u32 id)
{
	dpp_write(id, VG_DYNAMIC_GATING_ENABLE, 0x3F);
}

u32 dpp_reg_get_irq_status(u32 id)
{
	u32 cfg = dpp_read(id, VG_IRQ);
	cfg &= (VG_IRQ_HW_RESET_DONE | VG_IRQ_READ_SLAVE_ERROR |
		       VG_IRQ_DEADLOCK_STATUS |	VG_IRQ_FRAMEDONE);
	return cfg;
}

void dpp_reg_clear_irq(u32 id, u32 irq)
{
	dpp_write_mask(id, VG_IRQ, ~0, irq);
}

void dpp_constraints_params(struct dpp_size_constraints *vc,
		struct dpp_img_format *vi)
{
	if (!vi->wb && !vi->vgr) {
		if (vi->yuv) {
			vc->src_mul_w = YUV_SRC_SIZE_MULTIPLE;
			vc->src_mul_h = YUV_SRC_SIZE_MULTIPLE;
			vc->src_w_min = YUV_SRC_WIDTH_MIN;
			vc->src_w_max = YUV_SRC_WIDTH_MAX;
			vc->src_h_min = YUV_SRC_HEIGHT_MIN;
			vc->src_h_max = YUV_SRC_HEIGHT_MAX;
			vc->img_mul_w = YUV_IMG_SIZE_MULTIPLE;
			vc->img_mul_h = YUV_IMG_SIZE_MULTIPLE;
			vc->img_w_min = YUV_IMG_WIDTH_MIN;
			vc->img_w_max = IMG_WIDTH_MAX;
			vc->img_h_min = YUV_IMG_HEIGHT_MIN;
			vc->img_h_max = IMG_WIDTH_MAX;
			vc->src_mul_x = YUV_SRC_OFFSET_MULTIPLE;
			vc->src_mul_y = YUV_SRC_OFFSET_MULTIPLE;
			vc->sca_w_min = SCALED_WIDTH_MIN;
			vc->sca_w_max = SCALED_WIDTH_MAX;
			vc->sca_h_min = SCALED_HEIGHT_MIN;
			vc->sca_h_max = SCALED_HEIGHT_MAX;
			vc->sca_mul_w = SCALED_SIZE_MULTIPLE;
			vc->sca_mul_h = SCALED_SIZE_MULTIPLE;
		} else {
			vc->src_mul_w = RGB_SRC_SIZE_MULTIPLE;
			vc->src_mul_h = RGB_SRC_SIZE_MULTIPLE;
			vc->src_w_min = RGB_SRC_WIDTH_MIN;
			vc->src_w_max = RGB_SRC_WIDTH_MAX;
			vc->src_h_min = RGB_SRC_HEIGHT_MIN;
			vc->src_h_max = RGB_SRC_HEIGHT_MAX;
			vc->img_mul_w = RGB_IMG_SIZE_MULTIPLE;
			vc->img_mul_h = RGB_IMG_SIZE_MULTIPLE;
			vc->img_w_min = RGB_IMG_WIDTH_MIN;
			vc->img_w_max = IMG_WIDTH_MAX;
			vc->img_h_min = RGB_IMG_HEIGHT_MIN;
			vc->img_h_max = IMG_WIDTH_MAX;
			vc->src_mul_x = RGB_SRC_OFFSET_MULTIPLE;
			vc->src_mul_y = RGB_SRC_OFFSET_MULTIPLE;
			vc->sca_w_min = SCALED_WIDTH_MIN;
			vc->sca_w_max = SCALED_WIDTH_MAX;
			vc->sca_h_min = SCALED_HEIGHT_MIN;
			vc->sca_h_max = SCALED_HEIGHT_MAX;
			vc->sca_mul_w = SCALED_SIZE_MULTIPLE;
			vc->sca_mul_h = SCALED_SIZE_MULTIPLE;
		}
	} else if (!vi->wb && vi->vgr) {
		if (!vi->yuv) {
			vc->src_mul_w = RGB_SRC_SIZE_MULTIPLE;
			vc->src_mul_h = RGB_SRC_SIZE_MULTIPLE;
			vc->src_w_max = RGB_SRC_WIDTH_MAX;
			vc->src_h_max = RGB_SRC_HEIGHT_MAX;
			vc->sca_w_min = SCALED_WIDTH_MIN;
			vc->sca_w_max = SCALED_WIDTH_MAX;
			vc->sca_h_min = SCALED_HEIGHT_MIN;
			vc->sca_h_max = SCALED_HEIGHT_MAX;
			vc->src_mul_x = RGB_SRC_OFFSET_MULTIPLE;
			vc->src_mul_y = RGB_SRC_OFFSET_MULTIPLE;
			vc->sca_mul_w = SCALED_SIZE_MULTIPLE;
			vc->img_mul_w = PRE_RGB_WIDTH;
			vc->img_mul_h = PRE_RGB_HEIGHT;

			if (!vi->rot) {
				vc->src_w_min = ROT1_RGB_SRC_WIDTH_MIN;
				vc->src_h_min = ROT1_RGB_SRC_HEIGHT_MIN;
				vc->img_w_min = ROT1_RGB_IMG_WIDTH_MIN;
				vc->img_h_min = ROT1_RGB_IMG_HEIGHT_MIN;
				vc->sca_mul_h = SCALED_SIZE_MULTIPLE;
			} else {
				vc->src_w_min = ROT2_RGB_SRC_WIDTH_MIN;
				vc->src_h_min = ROT2_RGB_SRC_HEIGHT_MIN;
				vc->img_w_min = ROT2_RGB_IMG_WIDTH_MIN;
				vc->img_h_min = ROT2_RGB_IMG_HEIGHT_MIN;
				vc->sca_mul_h = SCALED_SIZE_MULTIPLE;
			}
			if (vi->normal) {
				vc->img_w_max = ROT3_RGB_IMG_WIDTH_MAX;
				vc->img_h_max = ROT3_RGB_IMG_HEIGHT_MAX;
			} else {
				vc->img_w_max = ROT4_RGB_IMG_WIDTH_MAX;
				vc->img_h_max = ROT4_RGB_IMG_HEIGHT_MAX;
				vc->blk_w_min = ROT3_RGB_BLK_WIDTH_MIN;
				vc->blk_w_max = ROT3_RGB_BLK_WIDTH_MAX;
				vc->blk_h_min = ROT3_RGB_BLK_HEIGHT_MIN;
				vc->blk_h_max = ROT3_RGB_BLK_HEIGHT_MAX;
			}
		} else {
			vc->src_mul_w = YUV_SRC_SIZE_MULTIPLE;
			vc->src_w_max = YUV_SRC_WIDTH_MAX;
			vc->src_h_max = YUV_SRC_HEIGHT_MAX;
			vc->sca_w_min = SCALED_WIDTH_MIN;
			vc->sca_w_max = SCALED_WIDTH_MAX;
			vc->sca_h_min = SCALED_HEIGHT_MIN;
			vc->sca_h_max = SCALED_HEIGHT_MAX;
			vc->src_mul_x = SRC_SIZE_MULTIPLE;
			vc->sca_mul_w = SCALED_SIZE_MULTIPLE;
			vc->img_mul_w = PRE_YUV_WIDTH;

			if (!vi->rot) {
				vc->src_w_min = ROT1_YUV_SRC_WIDTH_MIN;
				vc->src_h_min = ROT1_YUV_SRC_HEIGHT_MIN;
				vc->img_w_min = ROT1_YUV_IMG_WIDTH_MIN;
				vc->img_h_min = ROT1_YUV_IMG_HEIGHT_MIN;
			} else {
				vc->src_w_min = ROT2_YUV_SRC_WIDTH_MIN;
				vc->src_h_min = ROT2_YUV_SRC_HEIGHT_MIN;
				vc->img_w_min = ROT2_YUV_IMG_WIDTH_MIN;
				vc->img_h_min = ROT2_YUV_IMG_HEIGHT_MIN;
			}
			if (vi->normal) {
				vc->img_w_max = ROT3_YUV_IMG_WIDTH_MAX;
				vc->img_h_max = ROT3_YUV_IMG_HEIGHT_MAX;
			} else {
				vc->img_w_max = ROT4_YUV_IMG_WIDTH_MAX;
				vc->img_h_max = ROT4_YUV_IMG_HEIGHT_MAX;
			}
			if (vi->yuv422) {
				vc->src_mul_h = YUV_SRC_SIZE_MUL_HEIGHT;
				vc->img_mul_h = PRE_YUV_HEIGHT;
				if (!vi->rot) {
					vc->img_mul_h = PRE_ROT1_YUV_HEIGHT;
					vc->src_mul_y = YUV_SRC_SIZE_MUL_HEIGHT;
					vc->sca_mul_h = SCALED_SIZE_MULTIPLE;
				} else {
					vc->src_mul_y = YUV_SRC_OFFSET_MULTIPLE;
					vc->img_mul_h = PRE_YUV_HEIGHT;
					vc->sca_mul_h = SCALED_SIZE_MULTIPLE;
				}

			} else {
				vc->src_mul_h = YUV_SRC_SIZE_MULTIPLE;
				vc->img_mul_h = PRE_YUV_HEIGHT;
				vc->src_mul_y = YUV_SRC_OFFSET_MULTIPLE;
				vc->sca_mul_h = SCALED_SIZE_MULTIPLE;
			}
		}
	} else { /* write-back case */
		vc->src_mul_w = DST_SIZE_MULTIPLE;
		vc->src_mul_h = DST_SIZE_MULTIPLE;
		vc->src_w_min = DST_SIZE_WIDTH_MIN;
		vc->src_w_max = DST_SIZE_WIDTH_MAX;
		vc->src_h_min = DST_SIZE_HEIGHT_MIN;
		vc->src_h_max = DST_SIZE_HEIGHT_MAX;
		vc->img_mul_w = DST_IMGAGE_MULTIPLE;
		vc->img_mul_h = DST_IMGAGE_MULTIPLE;
		vc->img_w_min = DST_IMG_WIDTH_MIN;
		vc->img_w_max = DST_IMG_MAX;
		vc->img_h_min = DST_IMG_HEIGHT_MIN;
		vc->img_h_max = DST_IMG_MAX;
		vc->sca_w_min = DST_SIZE_WIDTH_MIN;
		vc->sca_w_max = DST_SIZE_WIDTH_MAX;
		vc->sca_h_min = DST_SIZE_HEIGHT_MIN;
		vc->sca_h_max = DST_SIZE_HEIGHT_MAX;
		vc->src_mul_x = DST_OFFSET_MULTIPLE;
		vc->src_mul_y = DST_OFFSET_MULTIPLE;
		vc->sca_mul_w = DST_OFFSET_MULTIPLE;
		vc->sca_mul_h = DST_OFFSET_MULTIPLE;
	}
}

void dpp_reg_init(u32 id)
{
	dpp_reg_set_realtime_path(id);
	dpp_reg_set_framedone_irq(id, false);
	dpp_reg_set_deadlock_irq(id, false);
	dpp_reg_set_read_slave_err_irq(id, false);
	dpp_reg_set_hw_reset_done_mask(id, false);
	dpp_reg_set_enable_interrupt(id);
	dpp_reg_set_lookup_table(id);
	dpp_reg_set_dynamic_clock_gating(id);
	dpp_reg_set_plane_alpha_fixed(id);
}

int dpp_reg_deinit(u32 id, bool reset)
{
	int ret;
	u32 dpp_irq = 0;

	if (!reset) {
		ret = dpp_reg_wait_op_status(id);
		if (ret) {
			dpp_err("dpp-%d is working\n", id);
			return -EBUSY;
		}
	}

	dpp_irq = dpp_reg_get_irq_status(id);
	dpp_reg_clear_irq(id, dpp_irq);

	dpp_reg_set_framedone_irq(id, true);
	dpp_reg_set_deadlock_irq(id, true);
	dpp_reg_set_read_slave_err_irq(id, true);
	dpp_reg_set_hw_reset_done_mask(id, true);
	if (reset)
		dpp_reg_set_sw_reset(id);

	return 0;
}

void dpp_reg_configure_params(u32 id, struct dpp_params_info *p)
{
	dpp_reg_wait_pingpong_clear(id);
	dpp_reg_set_rgb_type(id, p->eq_mode);
	dpp_reg_set_scale_ratio(id, p);
	dpp_reg_set_size(id, p);
	dpp_reg_set_rotation(id, p);
	dpp_reg_set_buf_addr(id, p);
	dpp_reg_set_block_area(id, p);
	dpp_reg_set_format(id, p);
}
