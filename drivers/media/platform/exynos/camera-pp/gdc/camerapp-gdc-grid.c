/*
 * Samsung EXYNOS CAMERA PostProcessing GDC driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "camerapp-gdc.h"
#include "camerapp-gdc-grid.h"
#include "camerapp-hw-api-gdc.h"
#include <exynos-fimc-is-sensor.h>

#define UUFIXMULT(n,g,f)	(((n) * (g) + (1 << ((f) - 1))) / (1 << (f)))
#define DELTA_SUBPIX_MAX	((1 << 22) - 1)
#define DELTA_SUBPIX_MIN	(-(1 << 22))
#define delta_clamp(x)	((x > DELTA_SUBPIX_MAX)? DELTA_SUBPIX_MAX : (x < DELTA_SUBPIX_MIN) ? DELTA_SUBPIX_MIN : x)

typedef signed long long sint64_t;

/* supported number of fractional bits: 0 < f < 64 !!! */
sint64_t hwround64(sint64_t a, int f)
{
    sint64_t rnd, sum, out;

    rnd = (((sint64_t) 1) << (f - 1));
    sum = a + rnd;
    if (a >= 0) {
        out = sum >> f;
    } else {
        if (sum >= 0)
            out = 0;
        else {
            out = ((sint64_t)sum) >> f;		/* logic right shift (no sign extension)*/
            out = out - (((sint64_t) 1) << (64 - f));
        }
    }

    return out;
}

int gdc_grid_subpixfrac2subpixfrac(int input_value)
{
    int shifter = 1;

    return (input_value + shifter / 2) >> shifter;
}

int gdc_grid_bicubicinterp(int virt_x, int virt_y,
				int posit_x, int posit_y, int full_matrix_grid[GRID_Y_SIZE][GRID_X_SIZE])
{
	/* uniform grid */
	int virt_uni_grid_x[GRID_X_SIZE] = {
		0, 1024, 2048, 3072, 4096, 5120, 6144, 7168, 8192,
	};
	int virt_uni_grid_y[GRID_Y_SIZE] = {
		0, 1024, 2048, 3072, 4096, 5120, 6144,
	};

	/* vertical interpolation variables: u:up, d:dn, l:left, r: right*/
	int ver_uull = 0, ver_uul = 0, ver_uur = 0, ver_uurr = 0;
	int ver_ull = 0, ver_ul = 0, ver_ur = 0, ver_urr = 0;
	int ver_dll = 0, ver_dl = 0, ver_dr = 0, ver_drr = 0;
	int ver_ddll = 0, ver_ddl = 0, ver_ddr = 0, ver_ddrr = 0;

	int tan_ver_ull = 0, tan_ver_ul = 0, tan_ver_ur = 0, tan_ver_urr = 0;
	int tan_ver_cll = 0, tan_ver_cl = 0, tan_ver_cr = 0, tan_ver_crr = 0;
	int tan_ver_dll = 0, tan_ver_dl = 0, tan_ver_dr = 0, tan_ver_drr = 0;

	/* vertical interpolation result: */
	int ver_rst_ll = 0, ver_rst_l = 0, ver_rst_r = 0, ver_rst_rr = 0;

	/* horisontal interpolation variables: */
	int tan_hor_l, tan_hor_c, tan_hor_r;

	/* interpolation result: */
	int v_out;

	/* 4 closest nodes on the virtual grid with the same units as x,y ( m_feature_virt_scale_frac_bits fract. bit): */
	int virt_uni_grid_xl = virt_uni_grid_x[posit_x - 1] << 1;
	int virt_uni_grid_xr = virt_uni_grid_x[posit_x] << 1;
	int	virt_uni_grid_yu = virt_uni_grid_y[posit_y - 1] << 1;
	int virt_uni_grid_yd = virt_uni_grid_y[posit_y] << 1;

	int hat, cur_ll, cur_l, cur, cur_r, cur_rr;
	int virt_step_fracbitsy_fracshift = 12;

	ver_ul = full_matrix_grid[posit_y - 1][(posit_x - 1)]; /* bittage already increased by GDC_SUBPIX_FRAC */
	ver_ur = full_matrix_grid[posit_y - 1][posit_x];
	ver_dl = full_matrix_grid[posit_y][(posit_x - 1)];
	ver_dr = full_matrix_grid[posit_y][posit_x];

	if (posit_y < (GRID_Y_SIZE - 1)) {
		ver_ddl = full_matrix_grid[posit_y + 1][(posit_x - 1)];
		ver_ddr = full_matrix_grid[posit_y + 1][posit_x];
	}
	if (posit_y > 1) {
		ver_uul = full_matrix_grid[posit_y - 2][(posit_x - 1)];
		ver_uur = full_matrix_grid[posit_y - 2][posit_x];
	}
	if (posit_x > 1) {
		ver_ull = full_matrix_grid[posit_y - 1][(posit_x - 2)];
		ver_dll = full_matrix_grid[posit_y][(posit_x-2)];
		if (posit_y < (GRID_Y_SIZE - 1))
			ver_ddll = full_matrix_grid[posit_y + 1][(posit_x - 2)];
		if (posit_y > 1)
			ver_uull = full_matrix_grid[posit_y - 2][(posit_x - 2)];
	}
	if (posit_x < (GRID_X_SIZE - 1)) {
		ver_urr = full_matrix_grid[posit_y - 1][(posit_x + 1)];
		ver_drr = full_matrix_grid[posit_y][(posit_x + 1)];
		if (posit_y < (GRID_Y_SIZE - 1))
			ver_ddrr = full_matrix_grid[posit_y + 1][(posit_x + 1)];
		if (posit_y > 1)
			ver_uurr = full_matrix_grid[posit_y - 2][(posit_x + 1)];
	}

	/* ^^^^^^^^ Calc vertical tangents at the 4 horizontally adjacent nodes ^^^^^^^^*/
	tan_ver_cl = ver_dl - ver_ul;			/* multiplied by (VIRT_STEP_Y)*/
	tan_ver_cr = ver_dr - ver_ur;			/* multiplied by (VIRT_STEP_Y) */
	if (posit_y == 1) {
		tan_ver_ul = tan_ver_cl * 4 - (ver_ddl - ver_ul);	/* multiplied by (VIRT_STEP_Y * 2) */
		tan_ver_ur = tan_ver_cr * 4 - (ver_ddr - ver_ur);	/* multiplied by (VIRT_STEP_Y * 2) */
	} else {
		tan_ver_ul = ver_dl - ver_uul;			/* multiplied by (VIRT_STEP_Y * 2) */
		tan_ver_ur = ver_dr - ver_uur;			/* multiplied by (VIRT_STEP_Y * 2) */
	}
	if (posit_y == (GRID_Y_SIZE - 1)) {
		tan_ver_dl = tan_ver_cl * 4 - (ver_dl - ver_uul);	/* multiplied by (VIRT_STEP_Y * 2) */
		tan_ver_dr = tan_ver_cr * 4 - (ver_dr - ver_uur);	/* multiplied by (VIRT_STEP_Y * 2) */
	} else {
		tan_ver_dl = ver_ddl - ver_ul;			/* multiplied by (VIRT_STEP_Y * 2) */
		tan_ver_dr = ver_ddr - ver_ur;			/* multiplied by (VIRT_STEP_Y * 2) */
	}

	if (posit_x > 1) {
		tan_ver_cll = ver_dll - ver_ull;		/* multiplied by (VIRT_STEP_Y) */
		if (posit_y == 1)
			tan_ver_ull = tan_ver_cll * 4 - (ver_ddll - ver_ull);	/* multiplied by (VIRT_STEP_Y * 2) */
		else
			tan_ver_ull = ver_dll - ver_uull;		/* multiplied by (VIRT_STEP_Y * 2) */
		if (posit_y == (GRID_Y_SIZE - 1))
			tan_ver_dll = tan_ver_cll * 4 - (ver_dll - ver_uull);	/* multiplied by (VIRT_STEP_Y * 2) */
		else
			tan_ver_dll = ver_ddll - ver_ull;					/* multiplied by (VIRT_STEP_Y * 2) */
	}

	if (posit_x < (GRID_X_SIZE - 1)) {
		tan_ver_crr = ver_drr - ver_urr;		/* multiplied by (VIRT_STEP_Y) */
		if (posit_y == 1)
			tan_ver_urr = tan_ver_crr * 4 - (ver_ddrr - ver_urr);	/* multiplied by (VIRT_STEP_Y * 2) */
		else
			tan_ver_urr = ver_drr 	- ver_uurr;			/* multiplied by (VIRT_STEP_Y * 2) */
		if (posit_y == (GRID_Y_SIZE - 1))
			tan_ver_drr = tan_ver_crr * 4 - (ver_drr - ver_uurr);	/* multiplied by (VIRT_STEP_Y * 2) */
		else
			tan_ver_drr = ver_ddrr - ver_urr;			/* multiplied by (VIRT_STEP_Y * 2) */
	}

	/* bittage: full_matrix_grid_BITTAGE + GDC_SUBPIX_FRAC (16) = 24 (signed 23) = INT15.8 */
	ver_rst_ll = gdc_grid_subpixfrac2subpixfrac(ver_ull);
	ver_rst_rr = gdc_grid_subpixfrac2subpixfrac(ver_urr);

	hat = UUFIXMULT(virt_y - virt_uni_grid_yu, virt_uni_grid_yd - virt_y, 11);		/* 2^11 = 2048 */

	cur_l = (int)(hwround64((virt_uni_grid_yd + virt_uni_grid_yu - virt_y * 2)
			* ((sint64_t)(tan_ver_cl * 4 - tan_ver_dl - tan_ver_ul)), 12));	/* multiplied by (VIRT_STEP_Y * 4) */
	cur_r = (int)(hwround64((virt_uni_grid_yd + virt_uni_grid_yu - virt_y * 2)
			* ((sint64_t)(tan_ver_cr * 4 - tan_ver_dr - tan_ver_ur)), 12));	/* multiplied by (VIRT_STEP_Y * 4) */

	ver_rst_l = gdc_grid_subpixfrac2subpixfrac(ver_ul)
			+ (int)(hwround64((virt_y - virt_uni_grid_yu) * ((sint64_t)(ver_dl - ver_ul)), 12))
			- (int)(hwround64(hat * ((sint64_t)(gdc_grid_subpixfrac2subpixfrac(tan_ver_dl - tan_ver_ul) + cur_l)), 13));	/* with factor 4 compensation */

	ver_rst_r = gdc_grid_subpixfrac2subpixfrac(ver_ur)
			+ (int)(hwround64((virt_y - virt_uni_grid_yu) * ((sint64_t)(ver_dr - ver_ur)), 12))
			- (int)(hwround64(hat * ((sint64_t)(gdc_grid_subpixfrac2subpixfrac(tan_ver_dr - tan_ver_ur) + cur_r)), 13));	/* with factor 4 compensation */

	if (posit_x < 8) {
		cur_rr = (int)(hwround64((virt_uni_grid_yd + virt_uni_grid_yu - virt_y * 2)
				* ((sint64_t)(tan_ver_crr * 4 - tan_ver_drr - tan_ver_urr)), virt_step_fracbitsy_fracshift));	/* multiplied by (VIRT_STEP_Y * 4) */
		ver_rst_rr = gdc_grid_subpixfrac2subpixfrac(ver_urr)
				+ (int)(hwround64((virt_y - virt_uni_grid_yu) * ((sint64_t)(ver_drr - ver_urr)), 12))
				- (int)(hwround64(hat * ((sint64_t)(gdc_grid_subpixfrac2subpixfrac(tan_ver_drr - tan_ver_urr) + cur_rr)), 13));	/* with factor 4 compensation */
	}
	if (posit_x > 1) {
		cur_ll = (int)(hwround64((virt_uni_grid_yd + virt_uni_grid_yu - virt_y * 2)
				* ((sint64_t)(tan_ver_cll * 4 - tan_ver_dll - tan_ver_ull)), virt_step_fracbitsy_fracshift));	/* multiplied by (VIRT_STEP_Y * 4) */
		ver_rst_ll = gdc_grid_subpixfrac2subpixfrac(ver_ull)
				+ (int)(hwround64((virt_y - virt_uni_grid_yu) * ((sint64_t)(ver_dll - ver_ull)), 12))
				- (int)(hwround64(hat * ((sint64_t)(gdc_grid_subpixfrac2subpixfrac(tan_ver_dll - tan_ver_ull) + cur_ll)), 13));	/* with factor 4 compensation */
	}

	ver_rst_ll = delta_clamp(ver_rst_ll);
	ver_rst_l = delta_clamp(ver_rst_l);
	ver_rst_r = delta_clamp(ver_rst_r);
	ver_rst_rr = delta_clamp(ver_rst_rr);

	/* ======== Calc horizontal tangents for the y point ======== */
	tan_hor_c = ver_rst_r - ver_rst_l;		/* multiplied by (VIRT_STEP_X) */
	if (posit_x == 1)
		tan_hor_l = tan_hor_c * 4 - (ver_rst_rr - ver_rst_l);		/* multiplied by (VIRT_STEP_Y * 2) */
	else
		tan_hor_l = ver_rst_r - ver_rst_ll;			/* multiplied by (VIRT_STEP_X * 2) */
	if (posit_x == GRID_X_SIZE - 1)
		tan_hor_r = tan_hor_c * 4 - (ver_rst_r - ver_rst_ll);		/* multiplied by (VIRT_STEP_Y * 2) */
	else
		tan_hor_r = ver_rst_rr - ver_rst_l;			/* multiplied by (VIRT_STEP_X * 2) */

	hat = UUFIXMULT(virt_x - virt_uni_grid_xl, virt_uni_grid_xr - virt_x, 11);
	cur = (int)(hwround64((virt_uni_grid_xl + virt_uni_grid_xr - virt_x * 2)
			* ((sint64_t)(tan_hor_c * 4 - tan_hor_l - tan_hor_r)), 11));		/* multiplied by (VIRT_STEP_X * 4) */
	v_out = ver_rst_l + (int)(hwround64((virt_x - virt_uni_grid_xl) * ((sint64_t)(ver_rst_r - ver_rst_l)), 11))
			- (int)(hwround64(hat * ((sint64_t)(tan_hor_r - tan_hor_l + cur)), 13));		/* with factor 4 compensation */
	v_out = delta_clamp(v_out);

	return v_out;
}

void camerapp_gdc_grid_linear_interpoltion(struct gdc_dev *gdc)
{
	unsigned int i, j;
	unsigned int ori_width, ori_height;
	unsigned int scaled_width, scaled_height;
	struct gdc_ctx *ctx;

	ctx = gdc->current_ctx;

	if (ctx->crop_param.is_crop_dzoom) {
		ori_width = ctx->crop_param.crop_width;
		ori_height = ctx->crop_param.crop_height;
		scaled_width = ctx->s_frame.width;
		scaled_height = ctx->s_frame.height;

		for (i = 0; i < GRID_Y_SIZE; i++) {
			for (j = 0; j < GRID_X_SIZE; j++) {
				ctx->grid_param.dx[i][j] = (ctx->grid_param.dx[i][j] * (int)scaled_width) / (int)ori_width;
				ctx->grid_param.dy[i][j] = (ctx->grid_param.dy[i][j] * (int)scaled_height) / (int)ori_height;
			}
		}
	} else {
		ori_width = ctx->crop_param.sensor_width;
		ori_height = ctx->crop_param.sensor_height;
		scaled_width = ctx->s_frame.width;
		scaled_height = ctx->s_frame.height;

		for (i = 0; i < GRID_Y_SIZE; i++) {
			for (j = 0; j < GRID_X_SIZE; j++) {
				ctx->grid_param.dx[i][j] = (ODC_STEREO_GRID_DX[i][j] * (int)scaled_width) / (int)ori_width;
				ctx->grid_param.dy[i][j] = (ODC_STEREO_GRID_DY[i][j] * (int)scaled_height) / (int)ori_height;
			}
		}
	}

	return;
}

void camerapp_gdc_grid_bicubic_interpolation(struct gdc_dev *gdc)
{
	u32 ori_width, ori_height;
	u32 temp_width, temp_height;
	u32 crop_start_x, crop_start_y;
	u32 croped_width, croped_height;
	u32 scaled_x, scaled_y;
	u32 scale_shifter_x, scale_shifter_y;
	u32 position_x, position_y;
	u32 temp_x, temp_y;

	u32 crop_matrix_x[GRID_Y_SIZE][GRID_X_SIZE];
	u32 crop_matrix_y[GRID_Y_SIZE][GRID_X_SIZE];
	u32 virtual_matrix_x[GRID_Y_SIZE][GRID_X_SIZE];
	u32 virtual_matrix_y[GRID_Y_SIZE][GRID_X_SIZE];
	u32 add_x, add_y;
	u32 i = 0, j = 0;

	struct gdc_ctx *ctx;

	ctx = gdc->current_ctx;

	crop_start_x = gdc->current_ctx->crop_param.crop_start_x;
	crop_start_y = gdc->current_ctx->crop_param.crop_start_y;
	ori_width = gdc->current_ctx->crop_param.sensor_width;
	ori_height = gdc->current_ctx->crop_param.sensor_height;
	croped_width = gdc->current_ctx->crop_param.crop_width;
	croped_height = gdc->current_ctx->crop_param.crop_height;

	scale_shifter_x = DS_SHIFTER_MAX;
	temp_width = ori_width * 2;
	while ((temp_width <= MAX_VIRTUAL_GRID_X) && (scale_shifter_x > 0)) {
		temp_width *= 2;
		scale_shifter_x--;
	}
	scale_shifter_y = DS_SHIFTER_MAX;
	temp_height = ori_height * 2;
	while ((temp_height <= MAX_VIRTUAL_GRID_Y) && (scale_shifter_y > 0)) {
		temp_height *= 2;
		scale_shifter_y--;
	}
	scaled_x = MIN(65535, ((MAX_VIRTUAL_GRID_X <<(DS_FRACTION_BITS + scale_shifter_x)) + ori_width / 2) / ori_width);
	scaled_y = MIN(65535, ((MAX_VIRTUAL_GRID_Y <<(DS_FRACTION_BITS + scale_shifter_y)) + ori_height / 2) / ori_height);
	gdc_dbg("scaled x,y = %d, %d / shift_x, y = %d, %d\n", scaled_x, scaled_y, scale_shifter_x, scale_shifter_y);

	/* real croped image matrix*/
	add_x = croped_width / (GRID_X_SIZE - 1);
	add_y = croped_height / (GRID_Y_SIZE - 1);
	for (i = 0; i < GRID_Y_SIZE; i++) {
		for (j = 0; j < GRID_X_SIZE; j++) {
			crop_matrix_x[i][j] = crop_start_x + add_x * j;
			crop_matrix_y[i][j] = crop_start_y + add_y * i;
		}
	}

	for (i = 0; i < GRID_Y_SIZE; i++) {
		for (j = 0; j < GRID_X_SIZE; j++) {
			/* convert to Virtual matrix */
			temp_x = (crop_matrix_x[i][j] << 1) + 1;
			temp_y = (crop_matrix_y[i][j] << 1) + 1;

			/* Use mirrorX, Y
			virtual_matrix_x[i][j] = (8192 << 1) - UUFIXMULT(scaled_x, (crop_start_x << 1) + temp_x, scale_shifter_x + DS_FRACTION_BITS);
			virtual_matrix_y[i][j] = (6144 << 1) - UUFIXMULT(scaled_y, (crop_start_y << 1) + temp_y, scale_shifter_y + DS_FRACTION_BITS);
			*/
			virtual_matrix_x[i][j] = UUFIXMULT(scaled_x, (crop_start_x << 1) + temp_x, scale_shifter_x + DS_FRACTION_BITS);
			virtual_matrix_y[i][j] = UUFIXMULT(scaled_y, (crop_start_y << 1) + temp_y, scale_shifter_y + DS_FRACTION_BITS);
		}
	}

	for (i = 0; i < GRID_Y_SIZE; i++) {
		for (j = 0; j < GRID_X_SIZE; j++) {
			/* find virtual grid cell indices*/
			position_x = (virtual_matrix_x[i][j] >> 11) + 1;	/* 9*7 grid location */
			position_y = (virtual_matrix_y[i][j] >> 11) + 1;

			ctx->grid_param.dx[i][j] = gdc_grid_bicubicinterp(virtual_matrix_x[i][j], virtual_matrix_y[i][j],
				position_x, position_y, ODC_STEREO_GRID_DX);
			ctx->grid_param.dy[i][j] = gdc_grid_bicubicinterp(virtual_matrix_x[i][j], virtual_matrix_y[i][j],
				position_x, position_y, ODC_STEREO_GRID_DY);
		}
	}

	return;
}

void camerapp_gdc_grid_nochange_setting(struct gdc_dev *gdc)
{
	unsigned int i, j;

	for (i = 0; i < GRID_Y_SIZE; i++) {
		for (j = 0; j < GRID_X_SIZE; j++) {
			gdc->current_ctx->grid_param.dx[i][j] = ODC_STEREO_GRID_DX[i][j];
			gdc->current_ctx->grid_param.dy[i][j] = ODC_STEREO_GRID_DY[i][j];
		}
	}

	return;
}

int camerapp_hw_gdc_size_compare(struct gdc_ctx *ctx)
{
	int ret = 0;
	u32 prev_width, prev_height;
	u32 curr_width, curr_height;

	curr_width = ctx->s_frame.width;
	curr_height = ctx->s_frame.height;
	prev_width = ctx->grid_param.prev_width;
	prev_height = ctx->grid_param.prev_height;

	gdc_dbg("compare : prev(%d, %d) <-> current(%d, %d)\n",
		prev_width, prev_height, curr_width, curr_height);

	if ((curr_width != prev_width) || (curr_height != prev_height))
		ret = true;

	ctx->grid_param.prev_width = curr_width;
	ctx->grid_param.prev_height = curr_height;

	return ret;
}

void camerapp_gdc_grid_get_default_table(u32 sensor_number)
{
	int i, j;

	gdc_dbg("grid_sensor_number = %d\n", sensor_number);

	if (sensor_number == SENSOR_NAME_S5K2L7) {
		for (i = 0; i < GRID_Y_SIZE; i++) {
			for (j = 0; j < GRID_X_SIZE; j++) {
				ODC_STEREO_GRID_DX[i][j] = ODC_STEREO_GRID_DX_2L7[i][j];
				ODC_STEREO_GRID_DY[i][j] = ODC_STEREO_GRID_DY_2L7[i][j];
			}
		}
	} else if (sensor_number == SENSOR_NAME_S5K2P8) {
		for (i = 0; i < GRID_Y_SIZE; i++) {
			for (j = 0; j < GRID_X_SIZE; j++) {
				ODC_STEREO_GRID_DX[i][j] = ODC_STEREO_GRID_DX_2P8[i][j];
				ODC_STEREO_GRID_DY[i][j] = ODC_STEREO_GRID_DY_2P8[i][j];
			}
		}
	} else if (sensor_number == SENSOR_NAME_S5K6B2) {
		for (i = 0; i < GRID_Y_SIZE; i++) {
			for (j = 0; j < GRID_X_SIZE; j++) {
				ODC_STEREO_GRID_DX[i][j] = ODC_STEREO_GRID_DX_6B2[i][j];
				ODC_STEREO_GRID_DY[i][j] = ODC_STEREO_GRID_DY_6B2[i][j];
			}
		}
	}

}

void camerapp_gdc_grid_setting(struct gdc_dev *gdc)
{
	struct gdc_ctx *ctx = gdc->current_ctx;

#ifdef ENABLE_GDC_CONSTANT_DISTORTION_CORRECTION		/* Correct Distortion lens*/
	gdc_dbg("GDC sensor(%d, %d), crop_s(%d, %d), crop(%d, %d), s_frame = (%d, %d)\n",
		ctx->crop_param.sensor_width, ctx->crop_param.sensor_height,
		ctx->crop_param.crop_start_x, ctx->crop_param.crop_start_y,
		ctx->crop_param.crop_width, ctx->crop_param.crop_height,
		ctx->s_frame.width, ctx->s_frame.height);

	ctx->crop_param.is_scaled = false;

	camerapp_gdc_grid_get_default_table(ctx->crop_param.sensor_num);

	if (camerapp_hw_gdc_size_compare(ctx)) {
		gdc_dbg("gdc_size is changed\n");
		ctx->grid_param.is_valid = false;
	}

	if (!ctx->grid_param.is_valid) {
		if ((ctx->s_frame.width != ctx->crop_param.sensor_width)
			|| (ctx->s_frame.height != ctx->crop_param.sensor_height)) {
			ctx->crop_param.is_scaled = true;
		}

		if ((ctx->crop_param.is_scaled)
			&& (!ctx->crop_param.is_crop_dzoom)) {
			camerapp_gdc_grid_linear_interpoltion(gdc);
			gdc_dbg("grid_linear_scaled\n");
		} else if (ctx->crop_param.is_crop_dzoom) {
			camerapp_gdc_grid_bicubic_interpolation(gdc);
			camerapp_gdc_grid_linear_interpoltion(gdc);
			gdc_dbg("grid_interpolated\n");
		} else {
			camerapp_gdc_grid_nochange_setting(gdc);
			gdc_dbg("Do not need grid change.\n");
		}

		ctx->grid_param.is_valid = true;
	}
#else		/* no correction*/
	int i, j;

	for (i = 0; i < GRID_Y_SIZE; i++) {
		for (j = 0; j < GRID_X_SIZE; j++) {
			ctx->grid_param.dx[i][j] = 0;
			ctx->grid_param.dy[i][j] = 0;
		}
	}
	ctx->grid_param.is_valid = true;

#endif
	return;
}

