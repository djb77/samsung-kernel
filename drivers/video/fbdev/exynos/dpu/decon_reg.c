/* linux/drivers/video/exynos/decon_8890/decon_reg_8890.c
 *
 * Copyright 2013-2015 Samsung Electronics
 *      Jiun Yu <jiun.yu@samsung.com>
 *
 * Jiun Yu <jiun.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "decon.h"

/******************* CAL raw functions implementation *************************/

u32 decon_reg_get_cam_status(void __iomem *cam_status)
{
	if (cam_status)
		return readl(cam_status);
	else
		return 0xF;
}

/* DECON2 only */
void decon_reg_set_vclk_freerun(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	if (id != 2)
		return;

	decon_write_mask(id, DISPIF_CONTROL, val, DISPIF_CLOCK_FREE_RUN_EN);
}

/* DECON2 only */
void decon_reg_set_underrun_scheme(u32 id, enum decon_hold_scheme mode)
{
	u32 val, mask;

	if (id != 2)
		return;

	val = DISPIF_CLOCK_UNDERRUN_SCHEME_F(mode);
	mask = DISPIF_CLOCK_UNDERRUN_SCHEME_MASK;
	decon_write_mask(id, DISPIF_CONTROL, val, mask);
}

/* DECON2 only */
void decon_reg_set_dispif_porch(u32 id, struct decon_lcd *lcd_info)
{
	u32 val;

	if (id != 2)
		return;

	val = DISPIF_VBPD_F(lcd_info->vbp) | DISPIF_VFPD_F(lcd_info->vfp);
	decon_write(id, DISPIF_TIMING_CONTROL_0, val);
	val = DISPIF_VSPD_F(lcd_info->vsa);
	decon_write(id, DISPIF_TIMING_CONTROL_1, val);

	val = DISPIF_HBPD_F(lcd_info->hbp) | DISPIF_HFPD_F(lcd_info->hfp);
	decon_write(id, DISPIF_TIMING_CONTROL_2, val);
	val = DISPIF_HSPD_F(lcd_info->hsa);
	decon_write(id, DISPIF_TIMING_CONTROL_3, val);
}

/* DECON2 only */
void decon_reg_set_dispif_size(u32 id, u32 width, u32 height)
{
	u32 val;

	if (id != 2)
		return;

	val = DISPIF_HEIGHT_F(height) | DISPIF_WIDTH_F(width);
	decon_write(id, DISPIF_SIZE_CONTROL_0, val);

	val = width * height;
	decon_write(id, DISPIF_SIZE_CONTROL_1, val);
}

/* DECON2 only */
void decon_reg_get_dispif_size(u32 id, u32 *w, u32 *h)
{
	u32 val;

	if (id != 2)
		return;

	val = decon_read(id, DISPIF_SIZE_CONTROL_0);
	*w = DISPIF_WIDTH_GET(val);
	*h = DISPIF_HEIGHT_GET(val);
}

/* DECON2 only */
void decon_reg_set_dispif_sync_pol(u32 id, u8 h_sync_pol, u8 v_sync_pol)
{
	if (id != 2)
		return;

	decon_write_mask(id, DISPIF_CONTROL,
			DISPIF_HSYNC_ACTIVE_VALUE(h_sync_pol) | DISPIF_VSYNC_ACTIVE_VALUE(v_sync_pol),			
			DISPIF_HSYNC_ACTIVE_VALUE_MASK | DISPIF_VSYNC_ACTIVE_VALUE_MASK);
}

int decon_reg_reset(u32 id)
{
	int tries;

	decon_write_mask(id, GLOBAL_CONTROL, ~0, GLOBAL_CONTROL_SRESET);
	for (tries = 2000; tries; --tries) {
		if (~decon_read(id, GLOBAL_CONTROL) & GLOBAL_CONTROL_SRESET)
			break;
		udelay(10);
	}

	if (!tries) {
		decon_err("failed to reset Decon\n");
		return -EBUSY;
	}

	return 0;
}

void decon_reg_set_operation_mode(u32 id, enum decon_psr_mode mode)
{
	u32 val, mask;

	mask = GLOBAL_CONTROL_OPERATION_MODE_F;
	if (mode == DECON_MIPI_COMMAND_MODE)
		val = GLOBAL_CONTROL_OPERATION_MODE_I80IF_F;
	else
		val = GLOBAL_CONTROL_OPERATION_MODE_RGBIF_F;
	decon_write_mask(id, GLOBAL_CONTROL, val, mask);
}

void decon_reg_direct_on_off(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	mask = (GLOBAL_CONTROL_DECON_EN | GLOBAL_CONTROL_DECON_EN_F);
	decon_write_mask(id, GLOBAL_CONTROL, val, mask);
}

void decon_reg_per_frame_off(u32 id)
{
	decon_write_mask(id, GLOBAL_CONTROL, 0, GLOBAL_CONTROL_DECON_EN_F);
}

void decon_reg_set_clkgate_mode(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;

	if (id == 1)
		mask = CLOCK_CONTROL_0_S_MASK;
	else if (id == 2)
		mask = CLOCK_CONTROL_0_T_MASK;
	else
		mask = CLOCK_CONTROL_0_F_MASK;
	decon_write_mask(id, CLOCK_CONTROL_0, val, mask);
}

/*
* Check the connection of DECON & DMA(DPP)
*
* CH0[5:4](vg0=2) - CH1[9:8](vgf0=4) - CH2[13:12](vgf1=5)
* - CH3[17:16](g1=1) - CH4[21:20](vg1=3)
*
* return : 0=decon0, 1=decon1, 2=decon2, 3=idle
*/
u32 decon_reg_get_rsc_dma_info(u32 id, u32 dpp_id)
{
	u32 val = 0;
	/* NA, G1-VG0-VG1-VGF0-VGF1 */
	u32 bit_pos[6] = {0, 16, 4, 20, 8, 12};
	u32 decon_id = 0;

	if (dpp_id == 0)
		val = 0;
	else
		val = decon_read(id, RESOURCE_OCCUPANCY_INFO_1);

	decon_id = (val >> bit_pos[dpp_id]) & 0x3;
	return decon_id;
}

int decon_reg_instant_stop(u32 id, unsigned long timeout)
{
	int ret = 0;

	decon_reg_direct_on_off(id, 0);
	decon_reg_update_req_global(id);
	ret = decon_reg_wait_idle_status_timeout(id, timeout);

	return ret;
}

/*
 * API is considering real possible Display Scenario
 * such as following examples
 *  < Single display >          D0=16K, D1=0K, D2=0K
 *  < Dual/Triple display >     D0=8K,  D1=8K, D2=0K
 *  < Single display + USB TV > D0=8K,  D1=8K, D2=0K
 *  < Dual display + DP >       D0=4K,  D1=4K, D2=8K
 *  < Dual display + WB-PRE >   D0=8K,  D1=8K, D2=0K
 *   --> Not necessary OutFIFO @preWB ( need @postWB )
 *
 * Current API does not configure various 8K case fully!
 * Therefore, modify/add configuration cases if necessary
 * "Resource Confliction" will happen if enabled simultaneously
*/
void decon_reg_set_sram_share(u32 id, enum decon_fifo_mode fifo_mode)
{
	u32 val = 0;

	switch (fifo_mode) {
	case DECON_FIFO_00K:
		val = ALL_SRAM_SHARE_DISABLE;
		break;
	case DECON_FIFO_04K:
		if (id == 0)
			val = FF0_SRAM_SHARE_ENABLE_F;
		else if (id == 1)
			val = FF1_SRAM_SHARE_ENABLE_F;
		else if (id == 2)
			val = FF2_SRAM_SHARE_ENABLE_F;
		break;
	case DECON_FIFO_08K:
		/* [HACK] only available for D0+D1 or D0+D2 */
		if (id == 0)
			val = (FLOATING_SRAM_SHARE_ENABLE_F
				| FF0_SRAM_SHARE_ENABLE_F);
		else if (id == 1)
			val = 0;
		else if (id == 2)
			val = (FF1_SRAM_SHARE_ENABLE_F
				| FF2_SRAM_SHARE_ENABLE_F);
		break;
	case DECON_FIFO_16K:
		val = ALL_SRAM_SHARE_ENABLE;
		break;
	default:
		break;
	}
	decon_write(id, SRAM_SHARE_ENABLE, val);
}

void decon_reg_set_splitter(u32 id, u32 width, u32 height,
	u32 split_idx, u32 overlap_w)
{
	u32 val;

	if (id != 0)
		return;

	val = SPLITTER_HEIGHT_F(height) | SPLITTER_WIDTH_F(width * 2);
	decon_write(id, SPLITTER_SIZE_CONTROL_0, val);

	val = height * width;
	decon_write(id, SPLITTER_SIZE_CONTROL_1, val);

	/* dual-DSI only */
	decon_write(id, SPLITTER_SPLIT_IDX_CONTROL, split_idx);
	val = SPLITTER_OVERLAP_F(overlap_w);
	decon_write(id, SPLITTER_OVERLAP_CONTROL, val);
}

void decon_reg_get_splitter_size(u32 id, u32 *w, u32 *h)
{
	u32 val;

	val = decon_read(id, SPLITTER_SIZE_CONTROL_0);
	*w = SPLITTER_WIDTH_GET(val);
	*h = SPLITTER_HEIGHT_GET(val);
}

void decon_reg_set_frame_fifo_threshold(u32 id, u32 ff0_th, u32 ff1_th)
{
	u32 val;

	if (id == 0)
		val = FRAME_FIFO_1_TH_F(ff1_th) | FRAME_FIFO_0_TH_F(ff0_th);
	else
		val = FRAME_FIFO_0_TH_F(ff0_th);

	decon_write(id, FRAME_FIFO_TH_CONTROL_0, val);
}

/*
* frame_fifo0 : must be set always
* frame_fifo1 : must be set for dual-dsi or dual-dsc
*/
void decon_reg_set_frame_fifo_size(u32 id, u32 ff_id, u32 width, u32 height)
{
	u32 val, cnt;
	u32 th, mask;

	val = FRAME_FIFO_HEIGHT_F(height) | FRAME_FIFO_WIDTH_F(width);
	cnt = height * width;

	if (ff_id == 0) {
		decon_write(id, FRAME_FIFO_0_SIZE_CONTROL_0, val);
		decon_write(id, FRAME_FIFO_0_SIZE_CONTROL_1, cnt);
		th = FRAME_FIFO_0_TH_F(width);
		mask = FRAME_FIFO_0_TH_MASK;
	} else {
		decon_write(id, FRAME_FIFO_1_SIZE_CONTROL_0, val);
		decon_write(id, FRAME_FIFO_1_SIZE_CONTROL_1, cnt);
		th = FRAME_FIFO_1_TH_F(width);
		mask = FRAME_FIFO_1_TH_MASK;
	}

	decon_write_mask(id, FRAME_FIFO_TH_CONTROL_0, th, mask);
}

void decon_reg_get_frame_fifo_size(u32 id, u32 ff_id, u32 *w, u32 *h)
{
	u32 val;

	if (ff_id == 0)
		val = decon_read(id, FRAME_FIFO_0_SIZE_CONTROL_0);
	else {
		if (id == 0)
			val = decon_read(id, FRAME_FIFO_1_SIZE_CONTROL_0);
		else
			val = 0;
	}

	*w = FRAME_FIFO_WIDTH_GET(val);
	*h = FRAME_FIFO_HEIGHT_GET(val);
}

void decon_reg_get_frame_fifo_threshold(u32 id, u32 *ff0_th, u32 *ff1_th)
{
	u32 val;

	val = decon_read(id, FRAME_FIFO_TH_CONTROL_0);
	*ff0_th = FRAME_FIFO_0_TH_GET(val);
	*ff1_th = FRAME_FIFO_1_TH_GET(val);
}

void decon_reg_get_frame_fifo_level(u32 id, u32 *ff0_lv, u32 *ff1_lv)
{
	if (id == 0) {
		*ff0_lv = decon_read(id, FRAME_FIFO_0_LEVEL);
		*ff1_lv = decon_read(id, FRAME_FIFO_1_LEVEL);
	} else {
		*ff1_lv = decon_read(id, FRAME_FIFO_0_LEVEL);
		*ff1_lv = 0;
	}
}

void decon_reg_set_hysteresis(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	decon_write_mask(id, HYSTERESIS_CONTROL, val, HYSTERESIS_ENABLE_F);
}

void decon_reg_set_hysteresis_threshold(u32 id, u32 high_th, u32 low_th)
{
	u32 val;

	val = HYSTERESIS_HIGH_TH_F(high_th) | HYSTERESIS_LOW_TH_F(low_th);
	decon_write(id, HYSTERESIS_THRESHOLD, val);
}

void decon_reg_set_formatter_pixel_order_swap(u32 id,
		u32 en_p0123, u32 en_p23, u32 en_p01)
{
	u32 val, mask;

	val = FORMATTER_PIXEL_ORDER_SWAP(en_p0123, en_p23, en_p01);
	mask = FORMATTER_PIXEL_ORDER_SWAP_MASK;
	decon_write_mask(id, FORMATTER_CONTROL, val, mask);
}

void decon_reg_set_rgb_order(u32 id, enum decon_rgb_order order)
{
	u32 val, mask;

	if (id == 2) {
		val = DISPIF_RGB_ORDER_F(order);
		mask = DISPIF_RGB_ORDER_MASK;
		decon_write_mask(id, DISPIF_CONTROL, val, mask);
	} else {
		val = FORMATTER_OUT_RGB_ORDER_F(order);
		mask = FORMATTER_OUT_RGB_ORDER_MASK;
		decon_write_mask(id, FORMATTER_CONTROL, val, mask);
	}
}

/*
* formatter0/1 setting depends on DSIM_IFx
* 1. dual-dsi      : both
* 2. dsim_if0 only : formatter0
* 3. dsim_if1 only : formatter1
*/
void decon_reg_set_formatter_size(u32 id, u32 fmt_id, u32 width, u32 height)
{
	u32 val, cnt;

	if (id && (fmt_id == 1))
		return;

	val = FORMATTER_HEIGHT_F(height) | FORMATTER_WIDTH_F(width);
	cnt = height * width;

	if (fmt_id == 0) {
		decon_write(id, FORMATTER0_SIZE_CONTROL_0, val);
		decon_write(id, FORMATTER0_SIZE_CONTROL_1, cnt);
	} else {
		decon_write(id, FORMATTER1_SIZE_CONTROL_0, val);
		decon_write(id, FORMATTER1_SIZE_CONTROL_1, cnt);
	}
}

void decon_reg_get_formatter_size(u32 id, u32 fmt_id, u32 *w, u32 *h)
{
	u32 val;

	if (fmt_id == 0)
		val = decon_read(id, FORMATTER0_SIZE_CONTROL_0);
	else {
		if (id == 0)
			val = decon_read(id, FORMATTER1_SIZE_CONTROL_0);
		else
			val = 0;
	}

	*w = FORMATTER_WIDTH_GET(val);
	*h = FORMATTER_HEIGHT_GET(val);
}

void decon_reg_set_blender_bg_image_size(u32 id,
		enum decon_dsi_mode dsi_mode, struct decon_lcd *lcd_info)
{
	u32 width, val, mask;

	width = lcd_info->xres;

	if (dsi_mode == DSI_MODE_DUAL_DSI)
		width = width * 2;

	val = BLENDER_BG_HEIGHT_F(lcd_info->yres) | BLENDER_BG_WIDTH_F(width);
	mask = BLENDER_BG_HEIGHT_MASK | BLENDER_BG_WIDTH_MASK;
	decon_write_mask(id, BLENDER_BG_IMAGE_SIZE_0, val, mask);

	val = (lcd_info->yres) * width;
	decon_write(id, BLENDER_BG_IMAGE_SIZE_1, val);
}

void decon_reg_get_blender_bg_image_size(u32 id, u32 *p_width, u32 *p_height)
{
	u32 val;

	val = decon_read(id, BLENDER_BG_IMAGE_SIZE_0);
	*p_width = BLENDER_BG_WIDTH_GET(val);
	*p_height = BLENDER_BG_HEIGHT_GET(val);
}

/*
 * argb_color : 32-bit
 * A[31:24] - R[23:16] - G[15:8] - B[7:0]
*/
void decon_reg_set_blender_bg_image_color(u32 id, u32 argb_color)
{
	u32 val, mask;
	u32 bg_alpha = 0, bg_red = 0;
	u32 bg_green = 0, bg_blue = 0;

	bg_alpha = (argb_color >> 24) & 0xFF;
	bg_red = (argb_color >> 16) & 0xFF;
	bg_green = (argb_color >> 8) & 0xFF;
	bg_blue = (argb_color >> 0) & 0xFF;

	val = BLENDER_BG_A_F(bg_alpha) | BLENDER_BG_R_F(bg_red);
	mask = BLENDER_BG_A_MASK | BLENDER_BG_R_MASK;
	decon_write_mask(id, BLENDER_BG_IMAGE_COLOR_0, val, mask);

	val = BLENDER_BG_G_F(bg_green) | BLENDER_BG_B_F(bg_blue);
	mask = BLENDER_BG_G_MASK | BLENDER_BG_B_MASK;
	decon_write_mask(id, BLENDER_BG_IMAGE_COLOR_1, val, mask);
}

void decon_reg_set_lrmerger_mode(u32 id, u32 win_idx,
		enum decon_merger_mode lrm_mode)
{
	u32 val, mask;

	if (win_idx == 0 || win_idx == 1) {
		val = LRM01_MODE_F(lrm_mode);
		mask = LRM01_MODE_MASK;
	} else if (win_idx == 2 || win_idx == 3) {
		val = LRM23_MODE_F(lrm_mode);
		mask = LRM23_MODE_MASK;
	}
	if  (win_idx < 4)
		decon_write_mask(id, LRMERGER_MODE_CONTROL, val, mask);
}

void decon_reg_set_dqe_lpd_exit_control(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	decon_write_mask(id, DATA_PATH_CONTROL_2, val, DQE_LPD_EXIT_CTRL);
}

void decon_reg_set_dqe_hsc_path(u32 id, enum decon_share_path hsc_path)
{
	u32 val;

	val = HSC_PATH_F(hsc_path);
	decon_write_mask(id, DATA_PATH_CONTROL_2, val, HSC_PATH_MASK);
}

void decon_reg_set_dqe_aps_path(u32 id, enum decon_share_path aps_path)
{
	u32 val;

	val = APS_PATH_F(aps_path);
	decon_write_mask(id, DATA_PATH_CONTROL_2, val, APS_PATH_MASK);
}

void decon_reg_set_data_path(u32 id, enum decon_data_path d_path,
		enum decon_enhance_path e_path)
{
	u32 val, mask;

	val = ENHANCE_LOGIC_PATH_F(e_path) | COMP_LINKIF_WB_PATH_F(d_path);
	mask = ENHANCE_LOGIC_PATH_MASK | COMP_LINKIF_WB_PATH_MASK;
	decon_write_mask(id, DATA_PATH_CONTROL_2, val, mask);
}

void decon_reg_get_data_path(u32 id, enum decon_data_path *d_path,
		enum decon_enhance_path *e_path)
{
	u32 val;

	val = decon_read(id, DATA_PATH_CONTROL_2);
	*d_path = COMP_LINKIF_WB_PATH_GET(val);
	*e_path = ENHANCE_LOGIC_PATH_GET(val);
}

/*
* Check major configuration of data_path_control
*    PRE-WB[8]
*    DSCC[7]
*    DSC_ENC1[5] DSC_ENC0[4]
*    POST_WB[2]
*    DSIM_IF1[1] DSIM_IF0[0]
*/
u32 decon_reg_get_data_path_cfg(u32 id, enum decon_path_cfg con_id)
{
	u32 val;
	u32 d_path;
	u32 bRet = 0;

	val = decon_read(id, DATA_PATH_CONTROL_2);
	d_path = COMP_LINKIF_WB_PATH_GET(val);

	switch (con_id) {
	case PATH_CON_ID_DSCC_EN:
		if (d_path & (0x1 << PATH_CON_ID_DSCC_EN))
			bRet = 1;
		break;
	case PATH_CON_ID_DUAL_DSC:
		if ((d_path & (0x3 << PATH_CON_ID_DUAL_DSC)) == 0x30)
			bRet = 1;
		break;
	case PATH_CON_ID_DSIM_IF0:
		if (d_path & (0x1 << PATH_CON_ID_DSIM_IF0))
			bRet = 1;
		break;
	case PATH_CON_ID_DSIM_IF1:
		if (d_path & (0x1 << PATH_CON_ID_DSIM_IF1))
			bRet = 1;
		break;
	default:
		break;
	}

	return bRet;
}

/*
* 'DATA_PATH_CONTROL_2' SFR must be set before calling this function!!
* [width]
* - no compression  : x-resolution
* - dsc compression : width_per_enc
*/
void decon_reg_config_data_path_size(u32 id,
	u32 width, u32 height, u32 overlap_w)
{
	u32 dual_dsc = 1;
	u32 dual_dsi = 0;
	u32 dsim_if0 = 1;
	u32 dsim_if1 = 0;
	struct decon_device *decon = get_decon_drvdata(id);

	dual_dsc = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DUAL_DSC);
	dsim_if0 = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSIM_IF0);
	dsim_if1 = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSIM_IF1);
	if (dsim_if0 && dsim_if1)
		dual_dsi = 1;

	/* 1. SPLITTER */
	if (dual_dsi && !dual_dsc)
		decon_reg_set_splitter(id, width*2, height, width, overlap_w);
	else
		decon_reg_set_splitter(id, width, height, width, 0);

	/* 2. FRAME_FIFO */
	decon_reg_set_frame_fifo_size(id, 0, width, height);
	if (dual_dsi || dual_dsc)
		decon_reg_set_frame_fifo_size(id, 1, width, height);

	/* 3. FORMATTER */
	if (dsim_if0 && !dsim_if1) {
		/* Single-DSI : DSIM0 */
		if (dual_dsc)
			decon_reg_set_formatter_size(id, 0, width*2, height);
		else
			decon_reg_set_formatter_size(id, 0, width, height);
	} else if (!dsim_if0 && dsim_if1) {
		/* Single-DSI : DSIM1 */
		if (dual_dsc)
			decon_reg_set_formatter_size(id, 1, width*2, height);
		else
			decon_reg_set_formatter_size(id, 1, width, height);
	} else if (dual_dsi) {
		/* Dual-DSI */
		decon_reg_set_formatter_size(id, 0, width, height);
		decon_reg_set_formatter_size(id, 1, width, height);
	} else {
		/* When Decon WrightBack, Remove Log. Too many logs*/
		if (decon->dt.out_type != DECON_OUT_WB)
			decon_err("decon%d : not specified case for FORMATTER\n", id);
		decon_reg_set_formatter_size(id, 0, width, height);
	}
}

void decon_reg_print_data_path_size(u32 id)
{
	u32 w, h;

	decon_reg_get_splitter_size(id, &w, &h);
	decon_dbg("SPLITTER size: w(%d), h(%d)\n", w, h);
	decon_reg_get_frame_fifo_size(id, 0, &w, &h);
	decon_dbg("FIFO0 size: w(%d), h(%d)\n", w, h);
	decon_reg_get_formatter_size(id, 0, &w, &h);
	decon_dbg("FORMATTER0 size: w(%d), h(%d)\n", w, h);

	if (id == 0) {
		decon_reg_get_frame_fifo_size(id, 1, &w, &h);
		decon_dbg("FIFO1 size: w(%d), h(%d)\n", w, h);
		decon_reg_get_formatter_size(id, 1, &w, &h);
		decon_dbg("FORMATTER1 size: w(%d), h(%d)\n", w, h);
	}
}

/* TODO: This will be modified in the future */
void dpu_sysreg_set_lpmux(void __iomem *sysreg)
{
	writel(0x1201, sysreg + DISP_LPMUX_CFG);
}

/*
* te_sel
*    0 = TE_FROM_SPEEDY
*    1 = TE_FROM_GPIO
*/

void dpu_sysreg_set_te_sel(u32 id, u32 te_sel)
{
	u32 val;

	/* TE_SEL : GPIO -> change SPEEDY if necessary */
	val = SEL_TE_PATH(0, te_sel)
		| SEL_TE_PATH(1, te_sel)
		| SEL_TE_PATH(2, te_sel);
	sysreg_write(id, DISP_DPU_TE_SEL, val);
	decon_dbg("%s: TE_SEL = 0x%x\n", __func__, val);
}

/*
* [ CAUTION ]
* 'DATA_PATH_CONTROL_2' SFR must be set before calling this function!!
*
* [ Implementation Info about CONNECTION ]
* 1) dsim_if0@DECON0 - DSIM0
* 2) dsim_if1@DECON0 - DSIM1
* 3) dsim_if0@DECON1 - DSIM1 (description for 'dsim_if2')
* --> modify code if you want to change connection between dsim_if and dsim
*/
void decon_reg_set_disp_ss_cfg(u32 id, struct decon_mode_info *psr)
{
	u32 val;
	u32 dsim_if0 = 0;
	u32 dsim_if1 = 0;

	if (psr->out_type != DECON_OUT_DSI) {
		decon_err("%s: [decon%d] output is not DSI\n",
			__func__, id);
		return;
	}

	/* dsim_if2 is equal to dsim_if0 at DECON1 */
	dsim_if0 = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSIM_IF0);
	dsim_if1 = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSIM_IF1);
	if (!dsim_if0 && !dsim_if1) {
		decon_err("%s: [decon%d] both dsim_if are not used\n",
			__func__, id);
		return;
	}

	/* DPMUX_CFG configuration : DECON(DISPIF) - DSIM connnection */
	val = sysreg_read(id, DISP_DLMUX_CFG);

	/*
	* [Caution]
	* - SEL_DSIM2DISPIF must be set as 'Reserved Value(=4)'
	*   when connection is not necessary.
	*   Otherwise, abnormal operation will happen!
	*/
	switch (id) {
	case 0:
		/* [ dsim_if0-dsim0 ] + [ dsim_if1-dsim1 ] */
		val = val & (~(SEL_DISPIF2DSIM_MASK(0) | SEL_DSIM2DISPIF_MASK(0)
			| SEL_DISPIF2DSIM_MASK(1) | SEL_DSIM2DISPIF_MASK(1)));
		if (dsim_if0)
			val |= SEL_DISPIF2DSIM(0, 0) | SEL_DSIM2DISPIF(0, 0);
		else
			/* for setting Reserved Value */
			val |= SEL_DISPIF2DSIM(0, 0) | SEL_DSIM2DISPIF(3, 0);

		if (dsim_if1)
			val |= SEL_DISPIF2DSIM(1, 1) | SEL_DSIM2DISPIF(1, 1);
		else
			/* for setting Reserved Value */
			val |= SEL_DISPIF2DSIM(1, 1) | SEL_DSIM2DISPIF(3, 1);
		break;

	case 1:
		/* dsim_if2-dsim1 */
		val = val & (~(SEL_DISPIF2DSIM_MASK(1)
			| SEL_DSIM2DISPIF_MASK(2)));
		if (dsim_if0)
			val |= SEL_DISPIF2DSIM(2, 1) | SEL_DSIM2DISPIF(1, 2);
		else
			/* for setting Reserved Value */
			val |= SEL_DISPIF2DSIM(2, 1) | SEL_DSIM2DISPIF(3, 2);
		break;

	case 2:
		break;
	}

	sysreg_write(id, DISP_DLMUX_CFG, val);
	decon_dbg("%s: DLMUX_CFG = 0x%x\n", __func__, val);

	/* TE_SEL : GPIO -> change SPEEDY if necessary */
	val = SEL_TE_PATH(0, TE_FROM_GPIO)
		| SEL_TE_PATH(1, TE_FROM_GPIO)
		| SEL_TE_PATH(2, TE_FROM_GPIO);
	sysreg_write(id, DISP_DPU_TE_SEL, val);
	decon_dbg("%s: TE_SEL = 0x%x\n", __func__, val);

	/*
	* LPMUX_CFG configuration : DSIM - MIPI_DPHY connection
	* -> will be configured at DSIM
	*/
}

void decon_reg_set_start_crc(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	decon_write_mask(id, CRC_CONTROL, val, CRC_START);
}

/* bit_sel : 0=B, 1=G, 2=R */
void decon_reg_set_select_crc_bits(u32 id, u32 bit_sel)
{
	u32 val;

	val = CRC_BITS_SEL(bit_sel);
	decon_write_mask(id, CRC_CONTROL, val, CRC_BITS_SEL_MASK);
}

void decon_reg_get_crc_data(u32 id, u32 *w0_data, u32 *w1_data)
{
	u32 val;

	val = decon_read(id, CRC_DATA);
	*w0_data = CRC_DATA_WCLK0_GET(val);
	*w1_data = CRC_DATA_WCLK1_GET(val);
}

void decon_reg_clear_all_update_req(u32 id)
{
	decon_write(id, SHADOW_REG_UPDATE_REQ, 0);
}

void decon_reg_update_req_global(u32 id)
{
	u32 mask;

	mask = SHADOW_REG_UPDATE_REQ_GLOBAL;
	decon_write_mask(id, SHADOW_REG_UPDATE_REQ, ~0, mask);
}

void decon_reg_update_req_window(u32 id, u32 win_idx)
{
	u32 mask;

	mask = SHADOW_REG_UPDATE_REQ_WIN(win_idx);
	decon_write_mask(id, SHADOW_REG_UPDATE_REQ, ~0, mask);
}

void decon_reg_all_win_shadow_update_req(u32 id)
{
	u32 mask;

	/* DECON0 has 6 windows */
	if (id == 0)
		mask = SHADOW_REG_UPDATE_REQ_FOR_DECON0;
	/* DECON1/2 has 5 windows */
	else
		mask = SHADOW_REG_UPDATE_REQ_FOR_DECON2;

	decon_write_mask(id, SHADOW_REG_UPDATE_REQ, ~0, mask);
}


u32 decon_reg_get_idle_status(u32 id)
{
	u32 val;

	val = decon_read(id, GLOBAL_CONTROL);
	if (val & GLOBAL_CONTROL_IDLE_STATUS)
		return 1;

	return 0;
}

int decon_reg_wait_idle_status_timeout(u32 id, unsigned long timeout)
{
	unsigned long delay_time = 10;
	unsigned long cnt = timeout / delay_time;
	u32 status;

	do {
		status = decon_reg_get_idle_status(id);
		cnt--;
		udelay(delay_time);
	} while (!status && cnt);

	if (!cnt) {
		decon_err("decon%d wait timeout decon idle status(%u)\n", id, status);
		return -EBUSY;
	}

	return 0;
}

u32 decon_reg_get_run_status(u32 id)
{
	u32 val;

	val = decon_read(id, GLOBAL_CONTROL);
	if (val & GLOBAL_CONTROL_RUN_STATUS)
		return 1;

	return 0;
}

/* Determine that DECON is perfectly shuttled off through checking this * function */
int decon_reg_wait_run_is_off_timeout(u32 id, unsigned long timeout)
{
	unsigned long delay_time = 10;
	unsigned long cnt = timeout / delay_time;
	u32 status;

	do {
		status = decon_reg_get_run_status(id);
		cnt--;
		udelay(delay_time);
	} while (status && cnt);

	if (!cnt) {
		decon_err("decon%d wait timeout decon run is shut-off(%u)\n", id, status);
		return -EBUSY;
	}

	return 0;
}

int decon_reg_wait_run_status_timeout(u32 id, unsigned long timeout)
{
	unsigned long delay_time = 10;
	unsigned long cnt = timeout / delay_time;
	u32 status;

	do {
		status = decon_reg_get_run_status(id);
		cnt--;
		udelay(delay_time);
	} while (!status && cnt);

	if (!cnt) {
		decon_err("decon%d wait timeout decon run status(%u)\n", id, status);
		return -EBUSY;
	}

	return 0;
}

void decon_reg_config_win_channel(u32 id, u32 win_idx,
		enum decon_idma_type type)
{
	u32 ch_id;
	u32 val, mask;

	ch_id = dpu_dma_type_to_channel(type);

	val = WIN_CHMAP_F(win_idx, ch_id);
	mask = WIN_CHMAP_MASK(win_idx);
	decon_write_mask(id, DATA_PATH_CONTROL_1, val, mask);
}


/* wait until shadow update is finished */
int decon_reg_wait_for_update_timeout(u32 id, unsigned long timeout)
{
	unsigned long delay_time = 100;
	unsigned long cnt = timeout / delay_time;

	while (decon_read(id, SHADOW_REG_UPDATE_REQ) && --cnt)
		udelay(delay_time);

	if (!cnt) {
		decon_err("decon%d timeout of updating decon registers\n", id);
		return -EBUSY;
	}

	return 0;
}


/* wait until shadow update is finished */
int decon_reg_wait_for_window_update_timeout(u32 id, u32 win_idx,
		unsigned long timeout)
{
	unsigned long delay_time = 100;
	unsigned long cnt = timeout / delay_time;

	while ((decon_read(id, SHADOW_REG_UPDATE_REQ) & SHADOW_REG_UPDATE_REQ_WIN(win_idx)) && --cnt)
		udelay(delay_time);

	if (!cnt) {
		decon_err("decon%d timeout of updating decon window registers\n", id);
		return -EBUSY;
	}

	return 0;
}

void decon_reg_set_hw_trig_sel(u32 id, enum decon_te_src te_src)
{
	u32 val, mask;

	val = HW_TRIG_SEL(te_src);
	mask = HW_TRIG_SEL_MASK;
	decon_write_mask(id, HW_SW_TRIG_CONTROL, val, mask);
}

void decon_reg_set_hw_trig_skip(u32 id, u32 cnt)
{
	u32 val, mask;

	val = HW_TRIG_SKIP(cnt);
	mask = HW_TRIG_SKIP_MASK;
	decon_write_mask(id, HW_SW_TRIG_CONTROL, val, mask);
}

void decon_reg_configure_trigger(u32 id, enum decon_trig_mode mode)
{
	u32 val, mask;

	mask = HW_TRIG_EN;
	/* Using TRIG_AUTO_MASK_EN=1'b0 is strictly prohibited! (SPEC-OUT) */

	if (mode == DECON_SW_TRIG)
		val = 0;
	else
		val = ~0;

	decon_write_mask(id, HW_SW_TRIG_CONTROL, val, mask);
}

void decon_reg_set_trigger(u32 id, struct decon_mode_info *psr,
		enum decon_set_trig en)
{
	u32 val, mask;

	if (psr->psr_mode == DECON_VIDEO_MODE)
		return;

	if (psr->trig_mode == DECON_SW_TRIG) {
		val = (en == DECON_TRIG_ENABLE) ? SW_TRIG_EN : 0;
		mask = HW_TRIG_EN | SW_TRIG_EN;
	} else {
		val = (en == DECON_TRIG_ENABLE) ?
			HW_TRIG_EN : HW_TRIG_MASK_DECON;
		mask = HW_TRIG_EN | HW_TRIG_MASK_DECON;
	}

	decon_write_mask(id, HW_SW_TRIG_CONTROL, val, mask);
}

void decon_reg_set_timeout_value(u32 id, u32 to_val)
{
	decon_write(id, TIME_OUT_VALUE, to_val);
}

/****************** DSC related functions ********************/
void dsc_reg_swreset(u32 dsc_id)
{
	dsc_write_mask(dsc_id, DSC_CONTROL0, 1, DSC_SW_RESET);
}

void dsc_reg_set_dcg_all(u32 dsc_id, u32 en)
{
	u32 val = 0;

	if (en)
		val = DSC_DCG_EN_ALL_MASK;
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_DCG_EN_ALL_MASK);
}

void dsc_reg_set_swap(u32 dsc_id, u32 bit_s, u32 byte_s, u32 word_s)
{
	u32 val;

	val = DSC_SWAP(bit_s, byte_s, word_s);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_SWAP_MASK);
}

void dsc_reg_set_flatness_det_th(u32 dsc_id, u32 th)
{
	u32 val;

	val = DSC_FLATNESS_DET_TH_F(th);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_FLATNESS_DET_TH_MASK);
}

void dsc_reg_set_slice_mode_change(u32 dsc_id, u32 en)
{
	u32 val;

	val = DSC_SLICE_MODE_CH_F(en);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_SLICE_MODE_CH_MASK);
}

void dsc_reg_set_encoder_bypass(u32 dsc_id, u32 en)
{
	u32 val;

	val = DSC_BYPASS_F(en);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_BYPASS_MASK);
}

void dsc_reg_set_auto_clock_gate(u32 dsc_id, u32 en)
{
	u32 val;

	val = DSC_CG_EN_F(en);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_CG_EN_MASK);
}

void dsc_reg_set_dual_slice(u32 dsc_id, u32 en)
{
	u32 val;

	val = DSC_DUAL_SLICE_EN_F(en);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_DUAL_SLICE_EN_MASK);
}

/* V-porch setting is unnecessary : use reset values */
void dsc_reg_set_vertical_porch(u32 dsc_id, u32 vfp, u32 vsw, u32 vbp)
{
	u32 val;

	val = (DSC_V_FRONT_F(vfp) | DSC_V_SYNC_F(vsw) | DSC_V_BACK_F(vbp));
	dsc_write_mask(dsc_id, DSC_CONTROL1, val, DSC_V_BLANK_MASK);
}

/* H-porch setting is unnecessary : use reset values */
void dsc_reg_set_horizontal_porch(u32 dsc_id, u32 hfp, u32 hsw, u32 hbp)
{
	u32 val;

	val = (DSC_H_FRONT_F(hfp) | DSC_H_SYNC_F(hsw) | DSC_H_BACK_F(hbp));
	dsc_write_mask(dsc_id, DSC_CONTROL2, val, DSC_H_BLANK_MASK);
}


/*
###############################################################################
				dsc PPS Configuration
###############################################################################
*/

/*
 * APIs which user setting or calculation is required are implemented
 * - PPS04 ~ PPS35 except reserved
 * - PPS58 ~ PPS59
*/
void dsc_reg_set_pps_04_comp_cfg(u32 dsc_id, u32 comp_cfg)
{
	u32 val, mask;

	val = PPS04_COMP_CFG(comp_cfg);
	mask = PPS04_COMP_CFG_MASK;
	dsc_write_mask(dsc_id, DSC_PPS04_07, val, mask);
}

void dsc_reg_set_pps_05_bit_per_pixel(u32 dsc_id, u32 bpp)
{
	u32 val, mask;

	val = PPS05_BPP(bpp);
	mask = PPS05_BPP_MASK;
	dsc_write_mask(dsc_id, DSC_PPS04_07, val, mask);
}

void dsc_reg_set_pps_06_07_picture_height(u32 dsc_id, u32 height)
{
	u32 val, mask;

	val = PPS06_07_PIC_HEIGHT(height);
	mask = PPS06_07_PIC_HEIGHT_MASK;
	dsc_write_mask(dsc_id, DSC_PPS04_07, val, mask);
}

void dsc_reg_set_pps_08_09_picture_width(u32 dsc_id, u32 width)
{
	u32 val, mask;

	val = PPS08_09_PIC_WIDHT(width);
	mask = PPS08_09_PIC_WIDHT_MASK;
	dsc_write_mask(dsc_id, DSC_PPS08_11, val, mask);
}
void dsc_reg_set_pps_10_11_slice_height(u32 dsc_id, u32 slice_height)
{
	u32 val, mask;

	val = PPS10_11_SLICE_HEIGHT(slice_height);
	mask = PPS10_11_SLICE_HEIGHT_MASK;
	dsc_write_mask(dsc_id, DSC_PPS08_11, val, mask);
}

void dsc_reg_set_pps_12_13_slice_width(u32 dsc_id, u32 slice_width)
{
	u32 val, mask;

	val = PPS12_13_SLICE_WIDTH(slice_width);
	mask = PPS12_13_SLICE_WIDTH_MASK;
	dsc_write_mask(dsc_id, DSC_PPS12_15, val, mask);
}

/* chunk_size = slice_width */
void dsc_reg_set_pps_14_15_chunk_size(u32 dsc_id, u32 chunk_size)
{
	u32 val, mask;

	val = PPS14_15_CHUNK_SIZE(chunk_size);
	mask = PPS14_15_CHUNK_SIZE_MASK;
	dsc_write_mask(dsc_id, DSC_PPS12_15, val, mask);
}

void dsc_reg_set_pps_16_17_init_xmit_delay(u32 dsc_id, u32 xmit_delay)
{
	u32 val, mask;

	val = PPS16_17_INIT_XMIT_DELAY(xmit_delay);
	mask = PPS16_17_INIT_XMIT_DELAY_MASK;
	dsc_write_mask(dsc_id, DSC_PPS16_19, val, mask);
}

void dsc_reg_set_pps_18_19_init_dec_delay(u32 dsc_id, u32 dec_delay)
{
	u32 val, mask;

	val = PPS18_19_INIT_DEC_DELAY(dec_delay);
	mask = PPS18_19_INIT_DEC_DELAY_MASK;
	dsc_write_mask(dsc_id, DSC_PPS16_19, val, mask);
}

void dsc_reg_set_pps_21_initial_scale_value(u32 dsc_id, u32 scale_value)
{
	u32 val, mask;

	val = PPS21_INIT_SCALE_VALUE(scale_value);
	mask = PPS21_INIT_SCALE_VALUE_MASK;
	dsc_write_mask(dsc_id, DSC_PPS20_23, val, mask);
}

void dsc_reg_set_pps_22_23_scale_increment_interval(u32 dsc_id, u32 sc_inc)
{
	u32 val, mask;

	val = PPS22_23_SCALE_INC_INTERVAL(sc_inc);
	mask = PPS22_23_SCALE_INC_INTERVAL_MASK;
	dsc_write_mask(dsc_id, DSC_PPS20_23, val, mask);
}

void dsc_reg_set_pps_24_25_scale_decrement_interval(u32 dsc_id, u32 sc_dec)
{
	u32 val, mask;

	val = PPS24_25_SCALE_DEC_INTERVAL(sc_dec);
	mask = PPS24_25_SCALE_DEC_INTERVAL_MASK;
	dsc_write_mask(dsc_id, DSC_PPS24_27, val, mask);
}

void dsc_reg_set_pps_27_first_line_bpg_offset(u32 dsc_id, u32 fl_bpg_off)
{
	u32 val, mask;

	val = PPS27_FL_BPG_OFFSET(fl_bpg_off);
	mask = PPS27_FL_BPG_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS24_27, val, mask);
}

void dsc_reg_set_pps_28_29_nfl_bpg_offset(u32 dsc_id, u32 nfl_bpg_off)
{
	u32 val, mask;

	val = PPS28_29_NFL_BPG_OFFSET(nfl_bpg_off);
	mask = PPS28_29_NFL_BPG_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS28_31, val, mask);
}

void dsc_reg_set_pps_30_31_slice_bpg_offset(u32 dsc_id, u32 slice_bpg_off)
{
	u32 val, mask;

	val = PPS30_31_SLICE_BPG_OFFSET(slice_bpg_off);
	mask = PPS30_31_SLICE_BPG_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS28_31, val, mask);
}

void dsc_reg_set_pps_32_33_initial_offset(u32 dsc_id, u32 init_off)
{
	u32 val, mask;

	val = PPS32_33_INIT_OFFSET(init_off);
	mask = PPS32_33_INIT_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS32_35, val, mask);
}

void dsc_reg_set_pps_34_35_final_offset(u32 dsc_id, u32 fin_off)
{
	u32 val, mask;

	val = PPS34_35_FINAL_OFFSET(fin_off);
	mask = PPS34_35_FINAL_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS32_35, val, mask);
}

void dsc_reg_set_pps_58_59_rc_range_param0(u32 dsc_id, u32 rc_range_param)
{
	u32 val, mask;

	val = PPS58_59_RC_RANGE_PARAM(rc_range_param);
	mask = PPS58_59_RC_RANGE_PARAM_MASK;
	dsc_write_mask(dsc_id, DSC_PPS56_59, val, mask);
}

static inline u32 dsc_round_up(u32 x, u32 a)
{
	u32 remained = x % a;

	if (!remained)
		return x;

	return x + a - remained;
}

static inline u32 ceil_div(u32 a, u32 b)
{
	return (a + (b - 1)) / b;
}

/* full size default value */
u32 dsc_get_dual_slice_mode(struct decon_lcd *lcd_info)
{
	u32 dual_slice_en = 0;

	if (lcd_info->dsc_cnt == 1) {
		if (lcd_info->dsc_slice_num == 2)
			dual_slice_en = 1;
	} else if(lcd_info->dsc_cnt == 2) {
		if (lcd_info->dsc_slice_num == 4)
			dual_slice_en = 1;
	} else {
		dual_slice_en = 0;
	}

	return dual_slice_en;
}

/* full size default value */
u32 dsc_get_slice_mode_change(struct decon_lcd *lcd_info)
{
	u32 slice_mode_ch = 0;

	if (lcd_info->dsc_cnt == 2) {
		if (lcd_info->dsc_slice_num == 2)
			slice_mode_ch = 1;
	}

	return slice_mode_ch;
}

void dsc_get_partial_update_info(struct decon_lcd *lcd,
	bool in_slice[4], u32 ds_en[2], u32 sm_ch[2])
{
	switch (lcd->dsc_slice_num) {
	case 4:
		if ((in_slice[0] + in_slice[1]) % 2) {
			ds_en[DECON_DSC_ENC0] = 0;
			sm_ch[DECON_DSC_ENC0] = 1;
		} else {
			ds_en[DECON_DSC_ENC0] = 1;
			sm_ch[DECON_DSC_ENC0] = 0;
		}

		if ((in_slice[2] + in_slice[3]) % 2) {
			ds_en[DECON_DSC_ENC1] = 0;
			sm_ch[DECON_DSC_ENC1] = 1;
		} else {
			ds_en[DECON_DSC_ENC1] = 1;
			sm_ch[DECON_DSC_ENC1] = 0;
		}

		break;
	case 2:
		if (lcd->dsc_cnt == 2) {
			ds_en[DECON_DSC_ENC0] = 0;
			sm_ch[DECON_DSC_ENC0] = 1;

			ds_en[DECON_DSC_ENC1] = 0;
			sm_ch[DECON_DSC_ENC1] = 1;
		}else {
			if (in_slice[0]) {
				ds_en[DECON_DSC_ENC0] = 0;
				sm_ch[DECON_DSC_ENC0] = 1;
			} else if (in_slice[1]) {
				ds_en[DECON_DSC_ENC0] = 0;
				sm_ch[DECON_DSC_ENC0] = 1;
			} else {
				ds_en[DECON_DSC_ENC0] = 1;
				sm_ch[DECON_DSC_ENC0] = 0;
			}

			ds_en[DECON_DSC_ENC1] = ds_en[DECON_DSC_ENC0];
			sm_ch[DECON_DSC_ENC1] = sm_ch[DECON_DSC_ENC0];
		}
		break;
	case 1:
		ds_en[DECON_DSC_ENC0] = 0;
		sm_ch[DECON_DSC_ENC0] = 0;

		ds_en[DECON_DSC_ENC1] = 0;
		sm_ch[DECON_DSC_ENC1] = 0;
		break;
	default:
		decon_err("Not specified case for Partial Update in DSC!\n");
		break;
	}

}

void dsc_reg_config_control(u32 dsc_id, u32 ds_en, u32 sm_ch)
{
	dsc_reg_set_dcg_all(dsc_id, 1);
	dsc_reg_set_swap(dsc_id, 0x0, 0x1, 0x0);
	/* flatness detection is fixed 2@8bpc / 8@10bpc / 32@12bpc */
	dsc_reg_set_flatness_det_th(dsc_id, 0x2);
	dsc_reg_set_auto_clock_gate(dsc_id, 1);
	dsc_reg_set_dual_slice(dsc_id, ds_en);
	dsc_reg_set_encoder_bypass(dsc_id, 0);
	dsc_reg_set_slice_mode_change(dsc_id, sm_ch);
}

/*
* overlap_w
* - default : 0
* - range : [0, 32] & (multiples of 2)
*    if non-zero value is applied, this means slice_w increasing.
*    therefore, DECON & DSIM setting must also be aligned.
*    --> must check if DDI module is supporting this feature !!!
*/
void dsc_calc_pps_info(struct decon_lcd *lcd_info, u32 dscc_en,
	struct decon_dsc *dsc_enc)
{
	u32 width, height;
	u32 slice_width, slice_height;
	u32 pic_width, pic_height;
	u32 width_eff;
	u32 dual_slice_en = 0;
	u32 bpp, chunk_size;
	u32 slice_bits;
	u32 groups_per_line, groups_total;

	/* initial values, also used for other pps calcualtion */
	u32 rc_model_size = 0x2000;
	u32 num_extra_mux_bits = 246;
	u32 initial_xmit_delay = 0x200;
	u32 initial_dec_delay = 0x4c0;
	/* when 'slice_w >= 70' */
	u32 initial_scale_value = 0x20;
	u32 first_line_bpg_offset = 0x0c;
	u32 initial_offset = 0x1800;
	u32 rc_range_parameters = 0x0102;

	u32 final_offset, final_scale;
	u32 flag, nfl_bpg_offset, slice_bpg_offset;
	u32 scale_increment_interval, scale_decrement_interval;
	u32 slice_width_byte_unit, comp_slice_width_byte_unit;
	u32 comp_slice_width_pixel_unit;
	u32 overlap_w = 0;
	u32 dsc_enc0_w = 0, dsc_enc0_h;
	u32 dsc_enc1_w = 0, dsc_enc1_h;
	u32 i, j;

	width = lcd_info->xres;
	height = lcd_info->yres;

	overlap_w = dsc_enc->overlap_w;

	if (dscc_en)
		/* OVERLAP can be used in the dual-slice case (if one ENC) */
		width_eff = (width >> 1) + overlap_w;
	else
		width_eff = width + overlap_w;

	pic_width = width_eff;
	dual_slice_en = dsc_get_dual_slice_mode(lcd_info);
	if (dual_slice_en)
		slice_width = width_eff >> 1;
	else
		slice_width = width_eff;

	pic_height = height;
	slice_height = lcd_info->dsc_slice_h;

	bpp = 8;
	chunk_size = slice_width;
	slice_bits = 8 * chunk_size * slice_height;

	while ((slice_bits - num_extra_mux_bits) % 48) {
		num_extra_mux_bits--;
	}

	groups_per_line = (slice_width + 2) / 3;
	groups_total = groups_per_line * slice_height;

	final_offset = rc_model_size - ((initial_xmit_delay * (8<<4) + 8)>>4)
		+ num_extra_mux_bits;
	final_scale = 8 * rc_model_size / (rc_model_size - final_offset);

	flag = (first_line_bpg_offset * 2048) % (slice_height - 1);
	nfl_bpg_offset = (first_line_bpg_offset * 2048) / (slice_height - 1);
	if (flag)
		nfl_bpg_offset = nfl_bpg_offset + 1;

	flag = 2048 * (rc_model_size - initial_offset + num_extra_mux_bits)
		% groups_total;
	slice_bpg_offset = 2048
		* (rc_model_size - initial_offset + num_extra_mux_bits)
		/ groups_total;
	if (flag)
		slice_bpg_offset = slice_bpg_offset + 1;

	scale_increment_interval = (2048 * final_offset) / ((final_scale - 9)
		* (nfl_bpg_offset + slice_bpg_offset));
	scale_decrement_interval = groups_per_line / (initial_scale_value - 8);

	/* 3bytes per pixel */
	slice_width_byte_unit = slice_width * 3;
	/* integer value, /3 for 1/3 compression */
	comp_slice_width_byte_unit = slice_width_byte_unit / 3;
	/* integer value, /3 for pixel unit */
	comp_slice_width_pixel_unit = comp_slice_width_byte_unit / 3;

	i = comp_slice_width_byte_unit % 3;
	j = comp_slice_width_pixel_unit % 2;

	if ( i == 0 && j == 0) {
		dsc_enc0_w = comp_slice_width_pixel_unit;
		dsc_enc0_h = pic_height;
		if (dscc_en) {
			dsc_enc1_w = comp_slice_width_pixel_unit;
			dsc_enc1_h = pic_height;
		}
	} else if (i == 0 && j != 0) {
		dsc_enc0_w = comp_slice_width_pixel_unit + 1;
		dsc_enc0_h = pic_height;
		if (dscc_en) {
			dsc_enc1_w = comp_slice_width_pixel_unit + 1;
			dsc_enc1_h = pic_height;
		}
	} else if (i != 0) {
		while (1) {
			comp_slice_width_pixel_unit++;
			j = comp_slice_width_pixel_unit % 2;
			if (j == 0)
				break;
		}
		dsc_enc0_w = comp_slice_width_pixel_unit;
		dsc_enc0_h = pic_height;
		if (dscc_en) {
			dsc_enc1_w = comp_slice_width_pixel_unit;
			dsc_enc1_h = pic_height;
		}
	}

	if (dual_slice_en) {
		dsc_enc0_w = dsc_enc0_w * 2;
		if (dscc_en)
			dsc_enc1_w = dsc_enc1_w * 2;
	}

	/* Save information to structure variable */
	dsc_enc->comp_cfg = 0x30;
	dsc_enc->bit_per_pixel = bpp << 4;
	dsc_enc->pic_height = pic_height;
	dsc_enc->pic_width = pic_width;
	dsc_enc->slice_height = slice_height;
	dsc_enc->slice_width = slice_width;
	dsc_enc->chunk_size = chunk_size;
	dsc_enc->initial_xmit_delay = initial_xmit_delay;
	dsc_enc->initial_dec_delay = initial_dec_delay;
	dsc_enc->initial_scale_value = initial_scale_value;
	dsc_enc->scale_increment_interval = scale_increment_interval;
	dsc_enc->scale_decrement_interval = scale_decrement_interval;
	dsc_enc->first_line_bpg_offset = first_line_bpg_offset;
	dsc_enc->nfl_bpg_offset = nfl_bpg_offset;
	dsc_enc->slice_bpg_offset = slice_bpg_offset;
	dsc_enc->initial_offset = initial_offset;
	dsc_enc->final_offset = final_offset;
	dsc_enc->rc_range_parameters = rc_range_parameters;

	dsc_enc->width_per_enc = dsc_enc0_w;
}

u32 dsc_get_compressed_slice_width(u32 x_resol, u32 dscc_en, u32 ds_en)
{
	u32 slice_width;
	u32 width_eff;
	u32 slice_width_byte_unit, comp_slice_width_byte_unit;
	u32 comp_slice_width_pixel_unit;
	u32 overlap_w = 0;
	u32 comp_slice_w = 0;
	u32 i, j;

	/* check if two encoders are used */
	if (dscc_en)
		width_eff = (x_resol >> 1) + overlap_w;
	else
		width_eff = x_resol + overlap_w;

	/* check if dual slice is enabled */
	if (ds_en)
		slice_width = width_eff >> 1;
	else
		slice_width = width_eff;

	/* 3bytes per pixel */
	slice_width_byte_unit = slice_width * 3;
	/* integer value, /3 for 1/3 compression */
	comp_slice_width_byte_unit = slice_width_byte_unit / 3;
	/* integer value, /3 for pixel unit */
	comp_slice_width_pixel_unit = comp_slice_width_byte_unit / 3;

	i = comp_slice_width_byte_unit % 3;
	j = comp_slice_width_pixel_unit % 2;

	if ( i == 0 && j == 0) {
		comp_slice_w = comp_slice_width_pixel_unit;
	} else if (i == 0 && j != 0) {
		comp_slice_w = comp_slice_width_pixel_unit + 1;
	} else if (i != 0) {
		while (1) {
			comp_slice_width_pixel_unit++;
			j = comp_slice_width_pixel_unit % 2;
			if (j == 0)
				break;
		}
		comp_slice_w = comp_slice_width_pixel_unit;
	}

	return comp_slice_w;

}

void dsc_reg_set_pps(u32 dsc_id, struct decon_dsc *dsc_enc)
{
	dsc_reg_set_pps_04_comp_cfg(dsc_id, dsc_enc->comp_cfg);
	dsc_reg_set_pps_05_bit_per_pixel(dsc_id, dsc_enc->bit_per_pixel);
	dsc_reg_set_pps_06_07_picture_height(dsc_id, dsc_enc->pic_height);

	dsc_reg_set_pps_08_09_picture_width(dsc_id, dsc_enc->pic_width);
	dsc_reg_set_pps_10_11_slice_height(dsc_id, dsc_enc->slice_height);
	dsc_reg_set_pps_12_13_slice_width(dsc_id, dsc_enc->slice_width);
	dsc_reg_set_pps_14_15_chunk_size(dsc_id, dsc_enc->chunk_size);

	dsc_reg_set_pps_16_17_init_xmit_delay(dsc_id,
		dsc_enc->initial_xmit_delay);

#ifndef VESA_SCR_V4
	dsc_reg_set_pps_18_19_init_dec_delay(dsc_id, 0x01B4);
#else
	dsc_reg_set_pps_18_19_init_dec_delay(dsc_id,
		dsc_enc->initial_dec_delay);
#endif
	dsc_reg_set_pps_21_initial_scale_value(dsc_id,
		dsc_enc->initial_scale_value);

	dsc_reg_set_pps_22_23_scale_increment_interval(dsc_id,
		dsc_enc->scale_increment_interval);
	dsc_reg_set_pps_24_25_scale_decrement_interval(dsc_id,
		dsc_enc->scale_decrement_interval);

	dsc_reg_set_pps_27_first_line_bpg_offset(dsc_id,
		dsc_enc->first_line_bpg_offset);
	dsc_reg_set_pps_28_29_nfl_bpg_offset(dsc_id, dsc_enc->nfl_bpg_offset);

	dsc_reg_set_pps_30_31_slice_bpg_offset(dsc_id,
		dsc_enc->slice_bpg_offset);
	dsc_reg_set_pps_32_33_initial_offset(dsc_id, dsc_enc->initial_offset);
	dsc_reg_set_pps_34_35_final_offset(dsc_id, dsc_enc->final_offset);

	/* min_qp0 = 0 , max_qp0 = 4 , bpg_off0 = 2 */
	dsc_reg_set_pps_58_59_rc_range_param0(dsc_id,
		dsc_enc->rc_range_parameters);
#ifndef VESA_SCR_V4
	/* PPS79 ~ PPS87 : 3HF4 is different with VESA SCR v4 */
	dsc_write(dsc_id, 0x006C, 0x1AB62AF6);
	dsc_write(dsc_id, 0x0070, 0x2B342B74);
	dsc_write(dsc_id, 0x0074, 0x3B746BF4);
#endif

}

/*
* Following PPS SFRs will be set from DDI PPS Table (DSC Decoder)
* : not 'fix' type
*   - PPS04 ~ PPS35
*   - PPS58 ~ PPS59
*   <PPS Table e.g.> SEQ_PPS_SLICE4[] @ s6e3hf4_param.h
*/
void dsc_get_decoder_pps_info(struct decon_dsc *dsc_dec,
	const unsigned char pps_t[90])
{
	dsc_dec->comp_cfg = (u32) pps_t[4];
	dsc_dec->bit_per_pixel = (u32) pps_t[5];
	dsc_dec->pic_height = (u32) (pps_t[6] << 8 | pps_t[7]);
	dsc_dec->pic_width = (u32) (pps_t[8] << 8 | pps_t[9]);
	dsc_dec->slice_height = (u32) (pps_t[10] << 8 | pps_t[11]);
	dsc_dec->slice_width = (u32) (pps_t[12] << 8 | pps_t[13]);
	dsc_dec->chunk_size = (u32) (pps_t[14] << 8 | pps_t[15]);
	dsc_dec->initial_xmit_delay = (u32) (pps_t[16] << 8 | pps_t[17]);
	dsc_dec->initial_dec_delay = (u32) (pps_t[18] << 8 | pps_t[19]);
	dsc_dec->initial_scale_value = (u32) pps_t[21];
	dsc_dec->scale_increment_interval = (u32) (pps_t[22] << 8 | pps_t[23]);
	dsc_dec->scale_decrement_interval = (u32) (pps_t[24] << 8 | pps_t[25]);
	dsc_dec->first_line_bpg_offset = (u32) pps_t[27];
	dsc_dec->nfl_bpg_offset = (u32) (pps_t[28] << 8 | pps_t[29]);
	dsc_dec->slice_bpg_offset = (u32) (pps_t[30] << 8 | pps_t[31]);
	dsc_dec->initial_offset = (u32) (pps_t[32] << 8 | pps_t[33]);
	dsc_dec->final_offset = (u32) (pps_t[34] << 8 | pps_t[35]);
	dsc_dec->rc_range_parameters = (u32) (pps_t[58] << 8 | pps_t[59]);
}

u32 dsc_cmp_pps_enc_dec(struct decon_dsc *p_enc, struct decon_dsc *p_dec)
{
	u32 diff_cnt = 0;

	if (p_enc->comp_cfg != p_dec->comp_cfg) {
		diff_cnt++;
		decon_dbg("[dsc_pps] comp_cfg (enc:dec = %d:%d)\n",
			p_enc->comp_cfg, p_dec->comp_cfg);
	}
	if (p_enc->bit_per_pixel != p_dec->bit_per_pixel) {
		diff_cnt++;
		decon_dbg("[dsc_pps] bit_per_pixel (enc:dec = %d:%d)\n",
			p_enc->bit_per_pixel, p_dec->bit_per_pixel);
	}
	if (p_enc->pic_height != p_dec->pic_height) {
		diff_cnt++;
		decon_dbg("[dsc_pps] pic_height (enc:dec = %d:%d)\n",
			p_enc->pic_height, p_dec->pic_height);
	}
	if (p_enc->pic_width != p_dec->pic_width) {
		diff_cnt++;
		decon_dbg("[dsc_pps] pic_width (enc:dec = %d:%d)\n",
			p_enc->pic_width, p_dec->pic_width);
	}
	if (p_enc->slice_height != p_dec->slice_height) {
		diff_cnt++;
		decon_dbg("[dsc_pps] slice_height (enc:dec = %d:%d)\n",
			p_enc->slice_height, p_dec->slice_height);
	}
	if (p_enc->slice_width != p_dec->slice_width) {
		diff_cnt++;
		decon_dbg("[dsc_pps] slice_width (enc:dec = %d:%d)\n",
			p_enc->slice_width, p_dec->slice_width);
	}
	if (p_enc->chunk_size != p_dec->chunk_size) {
		diff_cnt++;
		decon_dbg("[dsc_pps] chunk_size (enc:dec = %d:%d)\n",
			p_enc->chunk_size, p_dec->chunk_size);
	}
	if (p_enc->initial_xmit_delay != p_dec->initial_xmit_delay) {
		diff_cnt++;
		decon_dbg("[dsc_pps] initial_xmit_delay (enc:dec = %d:%d)\n",
			p_enc->initial_xmit_delay, p_dec->initial_xmit_delay);
	}
	if (p_enc->initial_dec_delay != p_dec->initial_dec_delay) {
		diff_cnt++;
		decon_dbg("[dsc_pps] initial_dec_delay (enc:dec = %d:%d)\n",
			p_enc->initial_dec_delay, p_dec->initial_dec_delay);
	}
	if (p_enc->initial_scale_value != p_dec->initial_scale_value) {
		diff_cnt++;
		decon_dbg("[dsc_pps] initial_scale_value (enc:dec = %d:%d)\n",
			p_enc->initial_scale_value, p_dec->initial_scale_value);
	}
	if (p_enc->scale_increment_interval != p_dec->scale_increment_interval) {
		diff_cnt++;
		decon_dbg("[dsc_pps] scale_increment_interval (enc:dec = %d:%d)\n",
			p_enc->scale_increment_interval, p_dec->scale_increment_interval);
	}
	if (p_enc->scale_decrement_interval != p_dec->scale_decrement_interval) {
		diff_cnt++;
		decon_dbg("[dsc_pps] scale_decrement_interval (enc:dec = %d:%d)\n",
			p_enc->scale_decrement_interval, p_dec->scale_decrement_interval);
	}
	if (p_enc->first_line_bpg_offset != p_dec->first_line_bpg_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] first_line_bpg_offset (enc:dec = %d:%d)\n",
			p_enc->first_line_bpg_offset, p_dec->first_line_bpg_offset);
	}
	if (p_enc->nfl_bpg_offset != p_dec->nfl_bpg_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] nfl_bpg_offset (enc:dec = %d:%d)\n",
			p_enc->nfl_bpg_offset, p_dec->nfl_bpg_offset);
	}
	if (p_enc->slice_bpg_offset != p_dec->slice_bpg_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] slice_bpg_offset (enc:dec = %d:%d)\n",
			p_enc->slice_bpg_offset, p_dec->slice_bpg_offset);
	}
	if (p_enc->initial_offset != p_dec->initial_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] initial_offset (enc:dec = %d:%d)\n",
			p_enc->initial_offset, p_dec->initial_offset);
	}
	if (p_enc->final_offset != p_dec->final_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] final_offset (enc:dec = %d:%d)\n",
			p_enc->final_offset, p_dec->final_offset);
	}
	if (p_enc->rc_range_parameters != p_dec->rc_range_parameters) {
		diff_cnt++;
		decon_dbg("[dsc_pps] rc_range_parameters (enc:dec = %d:%d)\n",
			p_enc->rc_range_parameters, p_dec->rc_range_parameters);
	}

	decon_dbg("[dsc_pps] total different count : %d\n", diff_cnt);

	return diff_cnt;
}

void dsc_reg_set_partial_update(u32 dsc_id, u32 dual_slice_en,
	u32 slice_mode_ch, u32 pic_h)
{
	/*
	* Following SFRs must be considered
	* - dual_slice_en
	* - slice_mode_change
	* - picture_height
	* - picture_width (don't care @KC) : decided by DSI (-> dual: /2)
	*/
	dsc_reg_set_dual_slice(dsc_id, dual_slice_en);
	dsc_reg_set_slice_mode_change(dsc_id, slice_mode_ch);
	dsc_reg_set_pps_06_07_picture_height(dsc_id, pic_h);
}

/*
 * This table is only used to check DSC setting value when debugging
 * Copy or Replace table's data from current using LCD information
 * ( e.g. : SEQ_PPS_SLICE4 @ s6e3hf4_param.h )
*/
static const unsigned char DDI_PPS_INFO[] = {
	0x11, 0x00, 0x00, 0x89, 0x30,
	0x80, 0x0A, 0x00, 0x05, 0xA0,
	0x00, 0x40, 0x01, 0x68, 0x01,
	0x68, 0x02, 0x00, 0x01, 0xB4,

	0x00, 0x20, 0x04, 0xF2, 0x00,
	0x05, 0x00, 0x0C, 0x01, 0x87,
	0x02, 0x63, 0x18, 0x00, 0x10,
	0xF0, 0x03, 0x0C, 0x20, 0x00,

	0x06, 0x0B, 0x0B, 0x33, 0x0E,
	0x1C, 0x2A, 0x38, 0x46, 0x54,
	0x62, 0x69, 0x70, 0x77, 0x79,
	0x7B, 0x7D, 0x7E, 0x01, 0x02,

	0x01, 0x00, 0x09, 0x40, 0x09,
	0xBE, 0x19, 0xFC, 0x19, 0xFA,
	0x19, 0xF8, 0x1A, 0x38, 0x1A,
	0x78, 0x1A, 0xB6, 0x2A, 0xF6,

	0x2B, 0x34, 0x2B, 0x74, 0x3B,
	0x74, 0x6B, 0xF4, 0x00, 0x00
};

void dsc_reg_set_encoder(u32 id, struct decon_param *p,
	struct decon_dsc *dsc_enc, u32 chk_en)
{
	u32 dsc_id;
	u32 dscc_en = 1;
	u32 ds_en = 0;
	u32 sm_ch = 0;
	struct decon_lcd *lcd_info = p->lcd_info;
	/* DDI PPS table : for compare with ENC PPS value */
	struct decon_dsc dsc_dec;
	/* set corresponding table like 'SEQ_PPS_SLICE4' */
	const unsigned char *pps_t = DDI_PPS_INFO;

	ds_en = dsc_get_dual_slice_mode(lcd_info);
	decon_dbg("dual slice(%d)\n", ds_en);

	sm_ch = dsc_get_slice_mode_change(lcd_info);
	decon_dbg("slice mode change(%d)\n", sm_ch);

	dscc_en = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSCC_EN);
	dsc_calc_pps_info(lcd_info, dscc_en, dsc_enc);

	if (id == 1) {
		dsc_reg_config_control(DECON_DSC_ENC1, ds_en, sm_ch);
		dsc_reg_set_pps(DECON_DSC_ENC1, dsc_enc);
	} else {
		for (dsc_id = 0; dsc_id < lcd_info->dsc_cnt; dsc_id++) {
			dsc_reg_config_control(dsc_id, ds_en, sm_ch);
			dsc_reg_set_pps(dsc_id, dsc_enc);
		}
	}

	if (chk_en) {
		dsc_get_decoder_pps_info(&dsc_dec, pps_t);
		if (dsc_cmp_pps_enc_dec(dsc_enc, &dsc_dec))
			decon_dbg("[WARNING] Check PPS value!!\n");
	}

}

int dsc_reg_init(u32 id, struct decon_param *p, u32 overlap_w, u32 swrst)
{
	u32 dsc_id;
	struct decon_lcd *lcd_info = p->lcd_info;
	struct decon_dsc dsc_enc;

	/* Basically, all SW-resets in DPU are not necessary ! */
	if (swrst) {
		for (dsc_id = 0; dsc_id < lcd_info->dsc_cnt; dsc_id++)
			dsc_reg_swreset(dsc_id);
	}

	dsc_enc.overlap_w = overlap_w;
	dsc_reg_set_encoder(id, p, &dsc_enc, 0);
	decon_reg_config_data_path_size(id,
		dsc_enc.width_per_enc, lcd_info->yres, overlap_w);

	/* To check SFR size configurations */
	decon_reg_print_data_path_size(id);

	return 0;
}

void decon_reg_configure_lcd(u32 id, struct decon_param *p)
{
	/*
	* overlap
	* 1. available when DDI is supporting
	* 1. non-zero value is only valid at dual-DSI extension display
	* 2. default=0 : range=[0, 32] & (multiples of 2)
	*/
	u32 overlap_w = 0;
	enum decon_data_path d_path = DPATH_DSCC_DSCENC01_FF01_FORMATTER0_DSIMIF0;
	enum decon_enhance_path e_path = ENHANCEPATH_ENHANCE_ALL_OFF;
	enum decon_share_path aps_path = SHAREPATH_DQE_USE;
	enum decon_share_path hsc_path = SHAREPATH_DQE_USE;

	struct decon_lcd *lcd_info = p->lcd_info;
	struct decon_mode_info *psr = &p->psr;
	enum decon_dsi_mode dsi_mode = psr->dsi_mode;
	enum decon_rgb_order rgb_order = DECON_RGB;

	if (lcd_info->dsc_enabled || (id == 2))
		rgb_order = DECON_RGB;
	else
		rgb_order = DECON_BGR;
	decon_reg_set_rgb_order(id, rgb_order);
	decon_reg_set_dispif_porch(id, lcd_info);

	/*
	* Following setting sequence must be kept
	* 1. decon_reg_set_data_path
	* 2. decon_reg_config_data_path_size
	* --> because fn2 refer to SFR of fn1
	*/
	if (lcd_info->dsc_enabled) {
		if (lcd_info->dsc_cnt == 1)
			d_path = DPATH_DSCENC0_FF0_FORMATTER0_DSIMIF0;
		else if (lcd_info->dsc_cnt == 2)
			d_path = DPATH_DSCC_DSCENC01_FF01_FORMATTER0_DSIMIF0;
		else
			decon_err("[decon%d] dsc_cnt=%d : not supported\n",
				id, lcd_info->dsc_cnt);

		decon_reg_set_data_path(id, d_path, e_path);

		dsc_reg_init(id, p, overlap_w, 0);
	} else {
		if (dsi_mode == DSI_MODE_DUAL_DSI)
			d_path = DPATH_NOCOMP_SPLITTER_FF0FF1_FORMATTER01_DSIMIF01;
		else if (id == 2)
			d_path = DECON2_NOCOMP_FF0_FORMATTER0_DISPIF;
		else
			d_path = DPATH_NOCOMP_FF0_FORMATTER0_DSIMIF0;
		decon_reg_set_data_path(id, d_path, e_path);

		decon_reg_config_data_path_size(id,
			lcd_info->xres, lcd_info->yres, overlap_w);

		decon_reg_set_dispif_size(id, lcd_info->xres, lcd_info->yres);
	}

	decon_reg_set_dqe_aps_path(id, aps_path);
	decon_reg_set_dqe_hsc_path(id, hsc_path);

	decon_reg_per_frame_off(id);
}

/***************** CAL APIs implementation *******************/
static void decon_reg_init_probe(u32 id, u32 dsi_idx, struct decon_param *p)
{
	struct decon_lcd *lcd_info = p->lcd_info;
	struct decon_mode_info *psr = &p->psr;
	enum decon_data_path d_path = DPATH_NOCOMP_FF0_FORMATTER0_DSIMIF0;
	enum decon_enhance_path e_path = ENHANCEPATH_ENHANCE_ALL_OFF;
	u32 overlap_w = 0; /* default=0 : range=[0, 32] & (multiples of 2) */
	enum decon_rgb_order rgb_order = DECON_RGB;

	if (lcd_info->dsc_enabled || (id == 2))
		rgb_order = DECON_RGB;
	else
		rgb_order = DECON_BGR;

	decon_reg_set_clkgate_mode(id, 1);

	decon_reg_set_sram_share(id, DECON_FIFO_08K);

	decon_reg_set_operation_mode(id, psr->psr_mode);

	decon_reg_set_blender_bg_image_size(id, psr->dsi_mode, lcd_info);

	/* valid only for DECON2-DP */
	decon_reg_set_underrun_scheme(id, DECON_VCLK_NOT_AFFECTED);

	/*
	 * same as decon_reg_configure_lcd(...) function
	 * except using decon_reg_update_req_global(id)
	 * instead of decon_reg_direct_on_off(id, 0)
	 */
	decon_reg_set_rgb_order(id, rgb_order);
	decon_reg_set_dispif_porch(id, lcd_info);

	if (lcd_info->dsc_enabled) {
		if (lcd_info->dsc_cnt == 1)
			d_path = DPATH_DSCENC0_FF0_FORMATTER0_DSIMIF0;
		else if (lcd_info->dsc_cnt == 2)
			d_path = DPATH_DSCC_DSCENC01_FF01_FORMATTER0_DSIMIF0;
		else
			decon_err("[decon%d] dsc_cnt=%d : not supported\n",
				id, lcd_info->dsc_cnt);

		decon_reg_set_data_path(id, d_path, e_path);

		dsc_reg_init(id, p, overlap_w, 0);
	} else {
		if (psr->dsi_mode == DSI_MODE_DUAL_DSI)
			d_path = DPATH_NOCOMP_SPLITTER_FF0FF1_FORMATTER01_DSIMIF01;
		else
			d_path = DPATH_NOCOMP_FF0_FORMATTER0_DSIMIF0;
		decon_reg_set_data_path(id, d_path, e_path);

		decon_reg_config_data_path_size(id,
			lcd_info->xres, lcd_info->yres, overlap_w);

		decon_reg_set_dispif_size(id, lcd_info->xres, lcd_info->yres);
	}

	decon_reg_get_data_path(id, &d_path, &e_path);
	decon_dbg("%s: decon%d, enhance path(0x%x), data path(0x%x)\n",
			__func__, id, e_path, d_path);
}

int decon_reg_init(u32 id, u32 dsi_idx, struct decon_param *p)
{
	struct decon_lcd *lcd_info = p->lcd_info;
	struct decon_mode_info *psr = &p->psr;
	enum decon_enhance_path e_path = ENHANCEPATH_ENHANCE_ALL_OFF;

	/* DECON does not need to start, if DECON is already
	 * running(enabled in LCD_ON_UBOOT) */
	if (decon_reg_get_run_status(id)) {
		decon_info("decon_reg_init already called by BOOTLOADER\n");
		decon_reg_init_probe(id, dsi_idx, p);
		if (psr->psr_mode == DECON_MIPI_COMMAND_MODE)
			decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);
		return -EBUSY;
	}

	dpu_sysreg_set_te_sel(id, TE_FROM_GPIO);

	//decon_reg_reset(id);

	decon_reg_set_clkgate_mode(id, 1);

	decon_reg_set_sram_share(id, DECON_FIFO_08K);

	decon_reg_set_operation_mode(id, psr->psr_mode);

	decon_reg_set_blender_bg_image_size(id, psr->dsi_mode, lcd_info);

	if (id == 2) {
		/* Set a TRIG mode */
		decon_reg_configure_trigger(id, psr->trig_mode);

		/*
		 * Interrupt of DECON-T should be set to video mode,
		 * because of malfunction of I80 frame done interrupt.
		 */
		if (psr->out_type == DECON_OUT_WB)
			psr->psr_mode = DECON_MIPI_COMMAND_MODE;
		else if (psr->out_type == DECON_OUT_DP)
			psr->psr_mode = DECON_VIDEO_MODE;
		decon_reg_set_int(id, psr, 1);
		decon_reg_set_underrun_scheme(id, DECON_VCLK_NOT_AFFECTED);
		decon_reg_set_vclk_freerun(id, 1);

		decon_reg_configure_lcd(id, p);
	} else {
		decon_reg_configure_lcd(id, p);

		if (psr->psr_mode == DECON_MIPI_COMMAND_MODE) {
			decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);
		}
	}

	/* FIXME: DECON_T dedicated to PRE_WB */
	if (p->psr.out_type == DECON_OUT_WB)
		decon_reg_set_data_path(id, DPATH_WBPRE_ONLY, e_path);

	/* asserted interrupt should be cleared before initializing decon hw */
	decon_reg_clear_int_all(id);

	/* Configure DISP_SS : 'data_path' setting is required */
	decon_reg_set_disp_ss_cfg(id, psr);

	return 0;
}

int decon_reg_start(u32 id, struct decon_mode_info *psr)
{
	int ret = 0;

	decon_reg_direct_on_off(id, 1);
	decon_reg_update_req_global(id);

	/* DECON goes to run-status as soon as request shadow update without HW_TE */
	ret = decon_reg_wait_run_status_timeout(id, 20 * 1000);

	/* wait until run-status, then trigger */
	if (psr->psr_mode == DECON_MIPI_COMMAND_MODE)
		decon_reg_set_trigger(id, psr, DECON_TRIG_ENABLE);
	return ret;
}

void decon_reg_set_partial_update(u32 id, enum decon_dsi_mode dsi_mode,
		struct decon_lcd *lcd_info, bool in_slice[])
{
	u32 overlap_w = 0; /* default=0 : range=[0, 32] & (multiples of 2) */
	u32 dual_slice_en[2] = {1, 1};
	u32 slice_mode_ch[2] = {0, 0};
	u32 width = lcd_info->xres;
	u32 dscc_en = 1;
	u32 ds_en = 1;
	u32 comp_slice_w = 0;

	/* Here, lcd_info contains the size to be updated */
	decon_reg_set_blender_bg_image_size(id, dsi_mode, lcd_info);
	decon_reg_set_dispif_porch(id, lcd_info);

	if (lcd_info->dsc_enabled) {
		/* for DECON SFR setting */
		ds_en = dsc_get_dual_slice_mode(lcd_info);
		dscc_en = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSCC_EN);
		comp_slice_w = dsc_get_compressed_slice_width(width,
			dscc_en, ds_en);

		/* get correct DSC configuration */
		dsc_get_partial_update_info(lcd_info, in_slice,
			dual_slice_en, slice_mode_ch);

		/* To support dual-display : DECON1 have to set DSC1 */
		dsc_reg_set_partial_update(id, dual_slice_en[0],
			slice_mode_ch[0], lcd_info->yres);
		if (lcd_info->dsc_cnt == 2)
			dsc_reg_set_partial_update(1, dual_slice_en[1],
				slice_mode_ch[1], lcd_info->yres);

		/* for full width update */
		if (ds_en)
			width = comp_slice_w * 2;
		else
			width = comp_slice_w;
	}

	decon_reg_config_data_path_size(id, width, lcd_info->yres, overlap_w);
	decon_reg_set_dispif_size(id, width, lcd_info->yres);
}

int decon_reg_stop_nreset(u32 id, struct decon_mode_info *psr)
{
	int ret = 0;

	if ((psr->psr_mode == DECON_MIPI_COMMAND_MODE) &&
			(psr->trig_mode == DECON_HW_TRIG)) {
		decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);
	}

	decon_reg_per_frame_off(id);
	decon_reg_update_req_global(id);

	/* timeout : 20ms */
	ret = decon_reg_wait_run_is_off_timeout(id, 20 * 1000);

	if (ret) {
		decon_err("decon_reg_stop_nreset failed!\n");
		ret = -EBUSY;
	}

	return ret;
}


int decon_reg_stop(u32 id, u32 dsi_idx, struct decon_mode_info *psr)
{
	int ret = 0;
	//int ret1 = 0, ret2 = 0;
	int timeout_value = 0;
	struct decon_device *decon = get_decon_drvdata(id);
        
	if ((psr->psr_mode == DECON_MIPI_COMMAND_MODE) &&
			(psr->trig_mode == DECON_HW_TRIG)) {
		decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);
	}

	decon_reg_per_frame_off(id);

	decon_reg_update_req_global(id);

	if (id != 2 || psr->out_type != DECON_OUT_DP) {
		/* timeout : 20ms @ 60Hz */
		ret = decon_reg_wait_run_is_off_timeout(id, 20 * 1000);
	} else {
		/* timeout : 1 / fps + 20% margin */
		timeout_value = 1000 / decon->lcd_info->fps * 12 / 10;
		ret = decon_reg_wait_run_is_off_timeout(id, timeout_value * 1000);
	}

#if 0 
	ret2 = decon_reg_reset(id);

	if (ret1 || ret2)
		ret = -EBUSY;
#endif
	return ret;
}

void decon_reg_release_resource(u32 id, struct decon_mode_info *psr)
{
	decon_reg_per_frame_off(id);
	decon_reg_update_req_global(id);
	decon_reg_set_trigger(id, psr, DECON_TRIG_ENABLE);
}

void decon_reg_clear_int_all(u32 id)
{
	u32 mask;

	if (id != 2) {
		mask = (DPU_MDNIE_DIMMING_END_INT_EN
			| DPU_MDNIE_DIMMING_START_INT_EN
			| DPU_DQE_DIMMING_END_INT_EN
			| DPU_DQE_DIMMING_START_INT_EN
			| DPU_FRAME_DONE_INT_EN
			| DPU_FRAME_START_INT_EN
			| DPU_FIFO_SEL_FIFO0
			| DPU_UNDER_FLOW_INT_EN);
	} else {
		mask = (DPU_DISPIF_VSTATUS_INT_SEL_MASK
			| DPU_DISPIF_VSTATUS_INT_EN
			| DPU_FRAME_DONE_INT_EN
			| DPU_FRAME_START_INT_EN
			| DPU_FIFO_SEL_MASK
			| DPU_UNDER_FLOW_INT_EN);
	}
	mask |= DPU_EXTRA_INT_EN;
	decon_write_mask(id, INTERRUPT_PENDING, ~0, mask);

	mask = (DPU_RESOURCE_CONFLICT_INT_EN
		| DPU_TIME_OUT_INT_EN
		| DPU_ERROR_INT_EN);
	decon_write_mask(id, EXTRA_INTERRUPT_PENDING, ~0, mask);
}

void decon_reg_set_int(u32 id, struct decon_mode_info *psr, u32 en)
{
	u32 val, mask;

	decon_reg_clear_int_all(id);

	if (en) {
		val = (DPU_DQE_DIMMING_END_INT_EN
			| DPU_DQE_DIMMING_START_INT_EN
			| DPU_FRAME_DONE_INT_EN
			| DPU_FRAME_START_INT_EN
			| DPU_EXTRA_INT_EN
			| DPU_INT_EN);
		/* DPU_UNDER_FLOW_INT_EN : don't use when DSIM and WB-POST */
		if (psr->psr_mode == DECON_VIDEO_MODE)
			val |= DPU_UNDER_FLOW_INT_EN;

		if (psr->psr_mode == DECON_VIDEO_MODE && psr->out_type == DECON_OUT_DP) {
			val = (DPU_FIFO_SEL_DISPIF
				| DPU_UNDER_FLOW_INT_EN
				| DPU_FRAME_DONE_INT_EN
				| DPU_DISPIF_VSTATUS_INT_EN
				| DPU_DISPIF_VSTATUS_INT_SEL(DPU_VERTICAL_VSYNC_START)
				| DPU_EXTRA_INT_EN
				| DPU_INT_EN);
		}

		if (id != 2)
			decon_write_mask(id, INTERRUPT_ENABLE, val, INTERRUPT_ENABLE_MASK);
		else
			decon_write_mask(id, INTERRUPT_ENABLE, val, INTERRUPT_ENABLE_MASK_DECON2);

		decon_dbg("decon %d, interrupt val = %x\n", id, val);

		val = (DPU_RESOURCE_CONFLICT_INT_EN
			| DPU_TIME_OUT_INT_EN
			| DPU_ERROR_INT_EN);
		decon_write(id, EXTRA_INTERRUPT_ENABLE, val);
	} else {
		mask = (DPU_EXTRA_INT_EN | DPU_INT_EN);
		decon_write_mask(id, INTERRUPT_ENABLE, 0, mask);
	}
}

void decon_reg_set_win_enable(u32 id, u32 win_idx, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	mask = WIN_EN_F(win_idx);
	decon_write_mask(id, DATA_PATH_CONTROL_0, val, mask);
}

/*
 * argb_color : 32-bit
 * A[31:24] - R[23:16] - G[15:8] - B[7:0]
*/
void decon_reg_set_win_mapcolor(u32 id, u32 win_idx, u32 argb_color)
{
	u32 val, mask;
	u32 mc_alpha = 0, mc_red = 0;
	u32 mc_green = 0, mc_blue = 0;

	mc_alpha = (argb_color >> 24) & 0xFF;
	mc_red = (argb_color >> 16) & 0xFF;
	mc_green = (argb_color >> 8) & 0xFF;
	mc_blue = (argb_color >> 0) & 0xFF;

	val = WIN_MAPCOLOR_A_F(mc_alpha) | WIN_MAPCOLOR_R_F(mc_red);
	mask = WIN_MAPCOLOR_A_MASK | WIN_MAPCOLOR_R_MASK;
	decon_write_mask(id, WIN_COLORMAP_0(win_idx), val, mask);

	val = WIN_MAPCOLOR_G_F(mc_green) | WIN_MAPCOLOR_B_F(mc_blue);
	mask = WIN_MAPCOLOR_G_MASK | WIN_MAPCOLOR_B_MASK;
	decon_write_mask(id, WIN_COLORMAP_1(win_idx), val, mask);
}

void decon_reg_set_winmap(u32 id, u32 win_idx, u32 color, u32 en)
{
	u32 val, mask;

	/* Enable */
	val = en ? ~0 : 0;
	mask = WIN_MAPCOLOR_EN_F(win_idx);
	decon_write_mask(id, DATA_PATH_CONTROL_0, val, mask);

	/* Color Set */
	decon_reg_set_win_mapcolor(0, win_idx, color);
}

/* ALPHA_MULT selection used in (a',b',c',d') coefficient */
void decon_reg_set_win_alpha_mult(u32 id, u32 win_idx, u32 a_sel)
{
	u32 val, mask;

	val = WIN_ALPHA_MULT_SRC_SEL_F(a_sel);
	mask = WIN_ALPHA_MULT_SRC_SEL_MASK;
	decon_write_mask(id, WIN_CONTROL_0(win_idx), val, mask);
}

void decon_reg_set_win_sub_coeff(u32 id, u32 win_idx,
		u32 fgd, u32 bgd, u32 fga, u32 bga)
{
	u32 val, mask;

	/*
	 * [ Blending Equation ]
	 * Color : Cr = (a x Cf) + (b x Cb)  <Cf=FG pxl_C, Cb=BG pxl_C>
	 * Alpha : Ar = (c x Af) + (d x Ab)  <Af=FG pxl_A, Ab=BG pxl_A>
	 *
	 * [ User-defined ]
	 * a' = WINx_FG_ALPHA_D_SEL : Af' that is multiplied by FG Pixel Color
	 * b' = WINx_BG_ALPHA_D_SEL : Ab' that is multiplied by BG Pixel Color
	 * c' = WINx_FG_ALPHA_A_SEL : Af' that is multiplied by FG Pixel Alpha
	 * d' = WINx_BG_ALPHA_A_SEL : Ab' that is multiplied by BG Pixel Alpha
	*/

	val = ( WIN_FG_ALPHA_D_SEL_F(fgd)
		| WIN_BG_ALPHA_D_SEL_F(bgd)
		| WIN_FG_ALPHA_A_SEL_F(fga)
		| WIN_BG_ALPHA_A_SEL_F(bga) );
	mask = ( WIN_FG_ALPHA_D_SEL_MASK
		| WIN_BG_ALPHA_D_SEL_MASK
		| WIN_FG_ALPHA_A_SEL_MASK
		| WIN_BG_ALPHA_A_SEL_MASK );
	decon_write_mask(id, WIN_CONTROL_1(win_idx), val, mask);
}

void decon_reg_set_win_plane_alpha(u32 id, u32 win_idx, u32 a0, u32 a1)
{
	u32 val, mask;

	val = WIN_ALPHA1_F(a1) | WIN_ALPHA0_F(a0);
	mask = WIN_ALPHA1_MASK | WIN_ALPHA0_MASK;
	decon_write_mask(id, WIN_CONTROL_0(win_idx), val, mask);
}

void decon_reg_get_win_plane_alpha(u32 id, u32 win_idx, u32 *a0, u32 *a1)
{
	u32 val;

	val = decon_read(id, WIN_CONTROL_0(win_idx));
	*a0 = WIN_ALPHA_GET(val, 0);
	*a1 = WIN_ALPHA_GET(val, 1);
}

void decon_reg_set_win_func(u32 id, u32 win_idx, enum decon_win_func pd_func)
{
	u32 val, mask;

	val = WIN_FUNC_F(pd_func);
	mask = WIN_FUNC_MASK;
	decon_write_mask(id, WIN_CONTROL_0(win_idx), val, mask);
}

void decon_reg_set_win_bnd_function(u32 id, u32 win_idx,
		struct decon_window_regs *regs)
{
	int plane_a = regs->plane_alpha;
	enum decon_blending blend = regs->blend;
	enum decon_win_func pd_func = PD_FUNC_USER_DEFINED;
	u8 alpha0 = 0xff;
	u8 alpha1 = 0xff;
	bool is_plane_a = false;
	u32 af_d = BND_COEF_ONE, ab_d = BND_COEF_ZERO,
		af_a = BND_COEF_ONE, ab_a = BND_COEF_ZERO;

	if (blend == DECON_BLENDING_NONE)
		pd_func = PD_FUNC_COPY;

	if ((plane_a >= 0) && (plane_a <= 0xff)) {
		alpha0 = plane_a;
		alpha1 = 0;
		is_plane_a = true;
	}

	if ((blend == DECON_BLENDING_COVERAGE) && !is_plane_a) {
		af_d = BND_COEF_AF;
		ab_d = BND_COEF_1_M_AF;
		af_a = BND_COEF_AF;
		ab_a = BND_COEF_1_M_AF;
	} else if ((blend == DECON_BLENDING_COVERAGE) && is_plane_a) {
		af_d = BND_COEF_ALPHA_MULT;
		ab_d = BND_COEF_1_M_ALPHA_MULT;
		af_a = BND_COEF_ALPHA_MULT;
		ab_a = BND_COEF_1_M_ALPHA_MULT;
	} else if ((blend == DECON_BLENDING_PREMULT) && !is_plane_a) {
		af_d = BND_COEF_ONE;
		ab_d = BND_COEF_1_M_AF;
		af_a = BND_COEF_ONE;
		ab_a = BND_COEF_1_M_AF;
	} else if ((blend == DECON_BLENDING_PREMULT) && is_plane_a) {
		af_d = BND_COEF_PLNAE_ALPHA0;
		ab_d = BND_COEF_1_M_ALPHA_MULT;
		af_a = BND_COEF_PLNAE_ALPHA0;
		ab_a = BND_COEF_1_M_ALPHA_MULT;
	} else if (blend == DECON_BLENDING_NONE) {
		decon_dbg("%s:%d none blending mode\n", __func__, __LINE__);
	} else {
 		decon_warn("%s:%d undefined blending mode\n", __func__, __LINE__);
	}

	decon_reg_set_win_plane_alpha(id, win_idx, alpha0, alpha1);
	decon_reg_set_win_alpha_mult(id, win_idx, ALPHA_MULT_SRC_SEL_AF);
	decon_reg_set_win_func(id, win_idx, pd_func);
	if (pd_func == PD_FUNC_USER_DEFINED)
		decon_reg_set_win_sub_coeff(id, win_idx, af_d, ab_d, af_a, ab_a);
}

void decon_reg_set_window_control(u32 id, int win_idx,
		struct decon_window_regs *regs, u32 winmap_en)
{
	u32 win_en = regs->wincon & WIN_EN_F(win_idx) ? 1 : 0;

	if (win_en) {
		decon_reg_set_win_bnd_function(0, win_idx, regs);
		decon_write(0, WIN_START_POSITION(win_idx), regs->start_pos);
		decon_write(0, WIN_END_POSITION(win_idx), regs->end_pos);
		decon_write(0, WIN_START_TIME_CONTROL(win_idx), regs->start_time);
		decon_write(0, WIN_PIXEL_COUNT(win_idx), regs->pixel_count);
	}

	decon_reg_set_win_enable(id, win_idx, win_en);
	if (win_en)
		decon_reg_set_winmap(id, win_idx, regs->colormap, winmap_en);
	decon_reg_config_win_channel(id, win_idx, regs->type);

	decon_dbg("%s: regs->type(%d)\n", __func__, regs->type);
}


void decon_reg_update_req_and_unmask(u32 id, struct decon_mode_info *psr)
{
	decon_reg_update_req_global(id);

	if (psr->psr_mode == DECON_MIPI_COMMAND_MODE)
		decon_reg_set_trigger(id, psr, DECON_TRIG_ENABLE);
}


int decon_reg_wait_update_done_and_mask(u32 id, struct decon_mode_info *psr, u32 timeout)
{
	int result;

	result = decon_reg_wait_for_update_timeout(id, timeout);

	if (psr->psr_mode == DECON_MIPI_COMMAND_MODE)
		decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);

	return result;
}

int decon_reg_get_interrupt_and_clear(u32 id, u32 *ext_irq)
{
	u32 val, val1;
	u32 reg_id;

	reg_id = INTERRUPT_PENDING;
	val = decon_read(id, reg_id);

	if (val & DPU_UNDER_FLOW_INT_PEND)
		decon_write(id, reg_id, DPU_UNDER_FLOW_INT_PEND);

	if (val & DPU_DISPIF_VSTATUS_INT_PEND)
		decon_write(id, reg_id, DPU_DISPIF_VSTATUS_INT_PEND);

	if (val & DPU_FRAME_START_INT_PEND)
		decon_write(id, reg_id, DPU_FRAME_START_INT_PEND);

	if (val & DPU_FRAME_DONE_INT_PEND)
		decon_write(id, reg_id, DPU_FRAME_DONE_INT_PEND);

	if (val & DPU_DQE_DIMMING_START_INT_PEND)
		decon_write(id, reg_id, DPU_DQE_DIMMING_START_INT_PEND);

	if (val & DPU_DQE_DIMMING_END_INT_PEND)
		decon_write(id, reg_id, DPU_DQE_DIMMING_END_INT_PEND);

	if (val & DPU_MDNIE_DIMMING_START_INT_PEND)
		decon_write(id, reg_id, DPU_MDNIE_DIMMING_START_INT_PEND);

	if (val & DPU_MDNIE_DIMMING_END_INT_PEND)
		decon_write(id, reg_id, DPU_MDNIE_DIMMING_END_INT_PEND);

	if (val & DPU_EXTRA_INT_PEND) {
		decon_write(id, reg_id, DPU_EXTRA_INT_PEND);

		reg_id = EXTRA_INTERRUPT_PENDING;
		val1 = decon_read(id, reg_id);
		*ext_irq = val1;

		if (val1 & DPU_RESOURCE_CONFLICT_INT_PEND) {
			decon_write(id, reg_id, DPU_RESOURCE_CONFLICT_INT_PEND);
			decon_warn("decon%d INFO0: SRAM_RSC= 0x%x\n",
				id,
				decon_read(id, RESOURCE_OCCUPANCY_INFO_0));
			decon_warn("decon%d INFO1: DMA_CH_RSC= 0x%x\n",
				id,
				decon_read(id, RESOURCE_OCCUPANCY_INFO_1));
			decon_warn("decon%d INFO2: WIN_RSC= 0x%x\n",
				id,
				decon_read(id, RESOURCE_OCCUPANCY_INFO_2));
		}

		if (val1 & DPU_TIME_OUT_INT_PEND)
			decon_write(id, reg_id, DPU_TIME_OUT_INT_PEND);

		if (val1 & DPU_ERROR_INT_PEND)
			decon_write(id, reg_id, DPU_ERROR_INT_PEND);
	}

	return val;
}

/******************  OS Only ******************/
int decon_reg_is_win_enabled(u32 id, int win_idx)
{
	if (decon_read(id, DATA_PATH_CONTROL_0) & WIN_EN_F(win_idx))
		return 1;

	return 0;
}

u32 decon_reg_get_width(u32 id, int dsi_mode)
{
	u32 val = 0;

	val = decon_read(id, FORMATTER0_SIZE_CONTROL_0);
	return (FORMATTER_WIDTH_GET(val));
}

u32 decon_reg_get_height(u32 id, int dsi_mode)
{
	u32 val = 0;

	val = decon_read(id, FORMATTER0_SIZE_CONTROL_0);
	return (FORMATTER_HEIGHT_GET(val));
}

const unsigned long decon_clocks_table[][CLK_ID_MAX] = {
	/* VCLK,  ECLK,  ACLK,  PCLK,  DISP_PLL,  resolution,            MIC_ratio, DSC count */
	{    71,   168,   400,    66,        71, 1080 * 1920,     MIC_COMP_BYPASS,         0},
	{    63,   168,   400,    66,        63, 1440 * 2560,  MIC_COMP_RATIO_1_2,         0},
	{  41.7, 137.5,   400,    66,      62.5, 1440 * 2560,  MIC_COMP_RATIO_1_3,         0},
	{   141, 137.5,   400,    66,       141, 1440 * 2560,     MIC_COMP_BYPASS,         0},
	{    42,   337,   400,    66,        42, 1440 * 2560,     MIC_COMP_BYPASS,         1},
	{    42,   168,   400,    66,        42, 1440 * 2560,     MIC_COMP_BYPASS,         2},
};

void decon_reg_get_clock_ratio(struct decon_clocks *clks, struct decon_lcd *lcd_info)
{
	int i = sizeof(decon_clocks_table) / sizeof(decon_clocks_table[0]) - 1;

	/* set reset value */
	clks->decon[CLK_ID_VCLK] = decon_clocks_table[0][CLK_ID_VCLK];
	clks->decon[CLK_ID_ECLK] = decon_clocks_table[0][CLK_ID_ECLK];
	clks->decon[CLK_ID_ACLK] = decon_clocks_table[0][CLK_ID_ACLK];
	clks->decon[CLK_ID_PCLK] = decon_clocks_table[0][CLK_ID_PCLK];
	clks->decon[CLK_ID_DPLL] = decon_clocks_table[0][CLK_ID_DPLL];

	for (; i >= 0; i--) {
		if (decon_clocks_table[i][CLK_ID_RESOLUTION]
				!= lcd_info->xres * lcd_info->yres) {
			continue;
		}

		if (!lcd_info->mic_enabled && !lcd_info->dsc_enabled) {
			if (decon_clocks_table[i][CLK_ID_MIC_RATIO]
					!= MIC_COMP_BYPASS)
				continue;
		}

		if (lcd_info->mic_enabled) {
			if (decon_clocks_table[i][CLK_ID_MIC_RATIO]
					!= lcd_info->mic_ratio)
				continue;
		}

		if (lcd_info->dsc_enabled) {
			if (decon_clocks_table[i][CLK_ID_DSC_RATIO]
					!= lcd_info->dsc_cnt)
				continue;
		}

		clks->decon[CLK_ID_VCLK] = decon_clocks_table[i][CLK_ID_VCLK];
		clks->decon[CLK_ID_ECLK] = decon_clocks_table[i][CLK_ID_ECLK];
		clks->decon[CLK_ID_ACLK] = decon_clocks_table[i][CLK_ID_ACLK];
		clks->decon[CLK_ID_PCLK] = decon_clocks_table[i][CLK_ID_PCLK];
		clks->decon[CLK_ID_DPLL] = decon_clocks_table[i][CLK_ID_DPLL];
		break;
	}

	decon_dbg("%s: VCLK %ld ECLK %ld ACLK %ld PCLK %ld DPLL %ld\n",
		__func__,
		clks->decon[CLK_ID_VCLK],
		clks->decon[CLK_ID_ECLK],
		clks->decon[CLK_ID_ACLK],
		clks->decon[CLK_ID_PCLK],
		clks->decon[CLK_ID_DPLL]);
}
#ifdef CONFIG_SUPPORT_DSU
void decon_set_dsu_update(u32 id, enum decon_dsi_mode dsi_mode, 
		struct decon_param *p)
{
	u32 width;
	u32 overlap_w = 0;
	enum decon_data_path d_path = DPATH_DSCC_DSCENC01_FF01_FORMATTER0_DSIMIF0;
	enum decon_rgb_order rgb = DECON_RGB;
	enum decon_enhance_path e_path = ENHANCEPATH_ENHANCE_ALL_OFF;
	struct decon_lcd *lcd_info = p->lcd_info;
	
	width = lcd_info->xres;

	decon_reg_set_blender_bg_image_size(id, dsi_mode, lcd_info);
	decon_reg_set_dispif_porch(id, lcd_info);

	if (lcd_info->dsc_enabled) {
		if (lcd_info->dsc_cnt == 1)
			d_path = DPATH_DSCENC0_FF0_FORMATTER0_DSIMIF0;
		else if (lcd_info->dsc_cnt == 2)
			d_path = DPATH_DSCC_DSCENC01_FF01_FORMATTER0_DSIMIF0;
		else
			decon_err("[decon%d] dsc_cnt=%d : not supported\n",
				id, lcd_info->dsc_cnt);
		decon_reg_set_data_path(id, d_path, e_path);
		dsc_reg_init(id, p, overlap_w, 0);

	} else {
		rgb = DECON_BGR;
		if (dsi_mode == DSI_MODE_DUAL_DSI)
			d_path = DPATH_NOCOMP_SPLITTER_FF0FF1_FORMATTER01_DSIMIF01;
		else if (id == 2)
			d_path = DECON2_NOCOMP_FF0_FORMATTER0_DISPIF;
		else
			d_path = DPATH_NOCOMP_FF0_FORMATTER0_DSIMIF0;

		decon_reg_set_data_path(id, d_path, e_path);
		decon_reg_config_data_path_size(id,
			lcd_info->xres, lcd_info->yres, overlap_w);
		decon_reg_set_dispif_size(id, lcd_info->xres, lcd_info->yres);
	}

	decon_reg_set_rgb_order(id, rgb);
	//decon_reg_config_data_path_size(id, width, lcd_info->yres, overlap_w);
	//decon_reg_set_dispif_size(id, width, lcd_info->yres);
}
#endif
