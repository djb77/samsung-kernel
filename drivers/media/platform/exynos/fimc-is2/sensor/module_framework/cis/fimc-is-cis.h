/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CIS_H
#define FIMC_IS_CIS_H

#define CIS_TEST_PATTERN_MODE 0
#define CIS_STREAM_OFF_WAIT_TIME 4	/* 4ms */

struct sensor_pll_info_compact {
	u32 ext_clk;
	u32 mipi_datarate;
	u32 pclk;
	u32 frame_length_lines;
	u32 line_length_pck;
};

struct sensor_pll_info {
	u32 ext_clk;
	u32 vt_pix_clk_div;
	u32 vt_sys_clk_div;
	u32 pre_pll_clk_div;
	u32 pll_multiplier;
	u32 op_pix_clk_div;
	u32 op_sys_clk_div;

	u32 secnd_pre_pll_clk_div;
	u32 secnd_pll_multiplier;
	u32 frame_length_lines;
	u32 line_length_pck;
};

int sensor_cis_set_registers(struct v4l2_subdev *subdev, const u32 *regs, const u32 size);
int sensor_cis_check_rev(struct fimc_is_cis *cis);

u32 sensor_cis_calc_again_code(u32 permile);
u32 sensor_cis_calc_again_permile(u32 code);

u32 sensor_cis_calc_dgain_code(u32 permile);
u32 sensor_cis_calc_dgain_permile(u32 code);

int sensor_cis_compensate_gain_for_extremely_br(struct v4l2_subdev *subdev, u32 expo, u32 *again, u32 *dgain);

int sensor_cis_dump_registers(struct v4l2_subdev *subdev, const u32 *regs, const u32 size);

u32 sensor_cis_do_div64(u64 num, u32 den);

int sensor_cis_wait_streamoff(struct v4l2_subdev *subdev);

#ifdef USE_FACE_UNLOCK_AE_AWB_INIT
int sensor_cis_set_initial_exposure(struct v4l2_subdev *subdev);
#endif

#endif
