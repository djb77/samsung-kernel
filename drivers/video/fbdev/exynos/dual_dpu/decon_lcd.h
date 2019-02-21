/* drivers/video/exynos_decon/lcd.h
 *
 * Copyright (c) 2011 Samsung Electronics
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __DECON_LCD__
#define __DECON_LCD__

enum decon_psr_mode {
	DECON_VIDEO_MODE = 0,
	DECON_DP_PSR_MODE = 1,
	DECON_MIPI_COMMAND_MODE = 2,
};

/* Mic ratio: 0: 1/2 ratio, 1: 1/3 ratio */
enum decon_mic_comp_ratio {
	MIC_COMP_RATIO_1_2 = 0,
	MIC_COMP_RATIO_1_3 = 1,
	MIC_COMP_BYPASS
};

enum mic_ver {
	MIC_VER_1_1,
	MIC_VER_1_2,
	MIC_VER_2_0,
};

enum type_of_ddi {
	TYPE_OF_SM_DDI = 0,
	TYPE_OF_MAGNA_DDI,
	TYPE_OF_NORMAL_DDI,
};

struct stdphy_pms {
	unsigned int p;
	unsigned int m;
	unsigned int s;
};

enum dsi_mode {
		/* support dsi_mode 
	0 : DSI_SDSD_MODE : Single Display + Single DSI 
	1 : DSI_DDDD_MODE : Dual Display + Dual DSI 
	2 : DSI_SDDD_MODE : Signle Display + Dual DSI */
	DSI_SDSD_MODE = 0,
	DSI_DDDD_MODE,
	DSI_SDDD_MODE,
};

#define LDI_NAME_LENGTH	32

#ifdef CONFIG_SUPPORT_DSU
#define MAX_DSU_RES_NUMBER 5

struct dsu_res_info {
	unsigned int width;
	unsigned int height;
	unsigned int dsc_en;
	unsigned int dsc_width;
	unsigned int dsc_height;
};

struct dsu_info_dt {
	unsigned int dsu_en;
	unsigned int dsu_number;
	struct dsu_res_info res_info[MAX_DSU_RES_NUMBER];
};
#endif
struct decon_lcd {
	enum decon_psr_mode mode;
	unsigned int vfp;
	unsigned int vbp;
	unsigned int hfp;
	unsigned int hbp;

	unsigned int vsa;
	unsigned int hsa;

	unsigned int xres;
	unsigned int yres;

	unsigned int width;
	unsigned int height;

	unsigned int hs_clk;
	struct stdphy_pms dphy_pms;
	unsigned int esc_clk;

	unsigned int fps;
	unsigned int mic_enabled;
	enum decon_mic_comp_ratio mic_ratio;
	unsigned int dsc_enabled;
	unsigned int dsc_cnt;
	unsigned int dsc_slice_num;
	unsigned int dsc_slice_h;
	enum mic_ver mic_ver;
	enum type_of_ddi ddi_type;
	unsigned int data_lane_cnt;
	unsigned int data_lane;
	unsigned int cmd_underrun_lp_ref;
	unsigned int vt_compensation;
	const char *ldi_name;
#ifdef CONFIG_SUPPORT_DSU
	unsigned int dsu_mode;
	struct dsu_info_dt dt_dsu_info;
#endif
	/* supporting operate_mode 
	0 : DSI_SDSD_MODE : Single Display + Single DSI 
	1 : DSI_DDDD_MODE : Dual Display + Dual DSI 
	2 : DSI_SDDD_MODE : Signle Display + Dual DSI */
	unsigned int op_mode;
	unsigned int disp_ss_path_swap;
};

struct decon_dsc {
/* 04 */	unsigned int comp_cfg;
/* 05 */	unsigned int bit_per_pixel;
/* 06-07 */	unsigned int pic_height;
/* 08-09 */	unsigned int pic_width;
/* 10-11 */	unsigned int slice_height;
/* 12-13 */	unsigned int slice_width;
/* 14-15 */	unsigned int chunk_size;
/* 16-17 */	unsigned int initial_xmit_delay;
/* 18-19 */	unsigned int initial_dec_delay;
/* 21 */	unsigned int initial_scale_value;
/* 22-23 */	unsigned int scale_increment_interval;
/* 24-25 */	unsigned int scale_decrement_interval;
/* 27 */	unsigned int first_line_bpg_offset;
/* 28-29 */	unsigned int nfl_bpg_offset;
/* 30-31 */	unsigned int slice_bpg_offset;
/* 32-33 */	unsigned int initial_offset;
/* 34-35 */	unsigned int final_offset;
/* 58-59 */	unsigned int rc_range_parameters;

		unsigned int overlap_w;
		unsigned int width_per_enc;
		unsigned char *dec_pps_t;
};

#endif
