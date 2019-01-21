/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * exynos fimc-is2 hw csi control functions
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-pdp-v1_0.h"
#include "fimc-is-hw-pdp.h"
#include "fimc-is-device-sensor.h"


#ifdef DEBUG_DUMP_STAT0
u32 *statBuf;
#endif

int pdp_hw_enable(u32 __iomem *base_reg, u32 pd_mode)
{
	u32 sensor_type;
	u32 enable;

	switch (pd_mode) {
	case PD_MSPD:
		sensor_type = SENSOR_TYPE_MSPD;
		enable = 1;
		break;
	case PD_MOD1:
		sensor_type = SENSOR_TYPE_MOD1;
		enable = 1;
		break;
	case PD_MOD2:
	case PD_NONE:
		sensor_type = SENSOR_TYPE_MOD2;
		enable = 0;
		break;
	case PD_MOD3:
		sensor_type = SENSOR_TYPE_MOD3;
		enable = 1;
		break;
	default:
		warn("PD MODE(%d) is invalid", pd_mode);
		sensor_type = SENSOR_TYPE_MOD2;
		enable = 0;
		break;
	}

	fimc_is_hw_set_field(base_reg, &pdp_regs[PDP_R_SENSOR_TYPE],
			&pdp_fields[PDP_F_SENSOR_TYPE], sensor_type);

	if (enable)
		pdp_hw_s_irq_msk(base_reg, 0x1F & ~PDP_INT_FRAME_PAF_STAT0);
	else
		pdp_hw_s_irq_msk(base_reg, 0x1F);

	fimc_is_hw_set_field(base_reg, &pdp_regs[PDP_R_PDP_CORE_ENABLE],
			&pdp_fields[PDP_F_PDP_CORE_ENABLE], enable);

	return enable;
}

void pdp_hw_s_config(u32 __iomem *base_reg)
{
	int i = 0;
	u32 index = 0;
	u32 value = 0;
	int count = sizeof(pdp_global_init_table)/sizeof(struct fimc_is_pdp_reg);
#ifdef DEBUG_DUMP_STAT0
	u32 bufSize = pdp_regs[PDP_R_PAF_STAT0_END].sfr_offset - pdp_regs[PDP_R_PAF_STAT0_START].sfr_offset;

	if (!statBuf) {
		statBuf = kmalloc(bufSize, GFP_KERNEL);
		memset(statBuf, 0x0, bufSize);
	}
#endif

	for (i = 0; i < count; i++) {
		index = pdp_global_init_table[i].index;
		value = pdp_global_init_table[i].init_value;

		//info("%s: Reg[%s] = %d\n", __func__, pdp_regs[index].reg_name, value);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[index], value);
	}
}

#ifdef DEBUG_DUMP_STAT0
void pdp_hw_g_reg_data_for_dump(void __iomem *base_addr,
			const struct fimc_is_reg *start_reg, const struct fimc_is_reg *end_reg)
{
	u32 reg_value;
	int i = 0;
	int buf_index = 0;

	for (i = 0; i < end_reg->sfr_offset - start_reg->sfr_offset; i += 4) {
		reg_value = readl(base_addr + start_reg->sfr_offset + i);
		*(statBuf+buf_index++) = reg_value;
	}
}

void pdp_hw_g_paf_single_window_stat(struct work_struct *data)
{
	struct fimc_is_pdp *pdp;
	loff_t pos = 0;
	int ret = 0;
	u32 size = pdp_regs[PDP_R_PAF_STAT0_END].sfr_offset - pdp_regs[PDP_R_PAF_STAT0_START].sfr_offset;

	pdp = container_of(data, struct fimc_is_pdp, wq_pdp_stat0);
	WARN_ON(!pdp);

	info("=====================PAF STAT0==============================\n");
	memset(statBuf, 0xFF, size);
	pdp_hw_g_reg_data_for_dump(pdp->base_reg, &pdp_regs[PDP_R_PAF_STAT0_START], &pdp_regs[PDP_R_PAF_STAT0_END]);

	ret = write_data_to_file("/data/media/0/pdp_stat0.dump", (char *)statBuf, size, &pos);
	if (ret < 0)
		info("Fail to dump pdp stat0.\n");
	else
		info("Done.\n");
}
#endif

void pdp_hw_g_reg_data(void __iomem *base_reg, u32 *buf,
			const struct fimc_is_reg *start_reg, const struct fimc_is_reg *end_reg)
{
	u32 reg_value;
	int i = 0;
	int buf_index = 0;

	for (i = 0; i < end_reg->sfr_offset - start_reg->sfr_offset; i += 4) {
		reg_value = readl(base_reg + start_reg->sfr_offset + i);
		*(buf+buf_index++) = reg_value;
	}
}

void pdp_hw_g_pdp_reg(void __iomem *base_reg, struct pdp_read_reg_setting_t *reg_param)
{
	int i = 0;

	WARN_ON(!reg_param->buf_array);
	WARN_ON(!reg_param->addr_array);

	for (i = 0; i < reg_param->buf_size; i++) {
		int addr = *(reg_param->addr_array + i);

		*(reg_param->buf_array + i) = readl(base_reg + addr);
	}
}

void pdp_hw_g_paf_sfr_stats(void __iomem *base_reg, u32 *buf)
{
	pdp_hw_g_reg_data(base_reg, buf, &pdp_regs[PDP_R_PAF_STAT0_START], &pdp_regs[PDP_R_PAF_STAT0_END]);
}

void pdp_hw_s_paf_mpd_bin_shift(u32 __iomem *base_reg, u32 h, u32 v, u32 vsht)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MPD_HBIN], h);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MPD_VBIN], v);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MPD_VSFT], vsht);
}

void pdp_hw_s_paf_col(u32 __iomem *base_reg, u32 value)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_COL], value);
}

void pdp_hw_s_paf_mode(u32 __iomem *base_reg, bool onoff)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_PAF_ON], onoff);
}

void pdp_hw_s_paf_active_size(u32 __iomem *base_reg, u32 width, u32 height)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ACTIVE_X], width);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ACTIVE_Y], height);
}

void pdp_hw_s_paf_af_size(u32 __iomem *base_reg, u32 width, u32 height)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_AF_SIZE_X], width);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_AF_SIZE_Y], height);
}

void pdp_hw_s_paf_crop_size(u32 __iomem *base_reg, struct pdp_point_info *size)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CROP_ON], 0);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CROP_SX], size->sx);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CROP_SY], size->sy);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CROP_EX], size->ex);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CROP_EY], size->ey);
}
void pdp_hw_s_paf_setting(u32 __iomem *base_reg,
			struct pdp_paf_setting_t *pafInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_XCOR_ON], pafInfo->xcor_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_AF_CROSS], pafInfo->af_cross);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MPD_ON], pafInfo->mpd_on);
#if 0	/* Move this code to pdp_set_sensor_info function.*/
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MPD_HBIN], pafInfo->mpd_hbin);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MPD_VBIN], pafInfo->mpd_vbin);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MPD_VSFT], pafInfo->mpd_vsft);
#endif
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MPD_DP], pafInfo->mpd_dp);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MPD_DP_TH], pafInfo->mpd_dp_th);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_PHASE_RANGE], pafInfo->phase_range);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_DPC_ON], pafInfo->dpc_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_LMV_ON], pafInfo->lmv_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_LMV_SHIFT], pafInfo->lmv_shift);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ALC_ON], pafInfo->alc_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ALC_GAP], pafInfo->alc_gap);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ALC_CLIP_ON], pafInfo->alc_clip_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ALC_FIT_ON], pafInfo->alc_fit_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_B2_EN], pafInfo->b2_en);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CROP_ON], pafInfo->crop_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_AF_DEBUG_MODE], pafInfo->af_debug_mode);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_LF_SHIFT], pafInfo->lf_shift);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_PAFSAT_ON], pafInfo->pafsat_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_SAT_LV], pafInfo->sat_lv);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_SAT_LV1], pafInfo->sat_lv1);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_SAT_LV2], pafInfo->sat_lv2);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_SAT_LV3], pafInfo->sat_lv3);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_SAT_SRC], pafInfo->sat_src);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_COR_TYPE], pafInfo->cor_type);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_G_SSD], pafInfo->g_ssd);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_OB_VALUE], pafInfo->ob_value);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_AF_LAYOUT], pafInfo->af_layout);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_AF_PATTERN], pafInfo->af_pattern);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ROI_ZG1], pafInfo->roi_zg1);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ROI_ZG2], pafInfo->roi_zg2);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ROI_ZG3], pafInfo->roi_zg3);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ROI_ZG4], pafInfo->roi_zg4);
}

void pdp_hw_s_paf_roi(u32 __iomem *base_reg,
			struct pdp_paf_roi_setting_t *roiInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ROI_SX], roiInfo->roi_start_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ROI_SY], roiInfo->roi_start_y);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ROI_EX], roiInfo->roi_end_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_ROI_EY], roiInfo->roi_end_y);
}

void pdp_hw_s_paf_knee(u32 __iomem *base_reg,
			struct pdp_knee_setting_t *kneeInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KNEE_ON], kneeInfo->knee_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_INC0], kneeInfo->kn_inc_0);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_INC1], kneeInfo->kn_inc_1);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_INC2], kneeInfo->kn_inc_2);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_INC3], kneeInfo->kn_inc_3);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_INC4], kneeInfo->kn_inc_4);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_INC5], kneeInfo->kn_inc_5);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_INC6], kneeInfo->kn_inc_6);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_INC7], kneeInfo->kn_inc_7);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_OFFSET1], kneeInfo->kn_offset_1);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_OFFSET2], kneeInfo->kn_offset_2);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_OFFSET3], kneeInfo->kn_offset_3);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_OFFSET4], kneeInfo->kn_offset_4);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_OFFSET5], kneeInfo->kn_offset_5);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_OFFSET6], kneeInfo->kn_offset_6);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_KN_OFFSET7], kneeInfo->kn_offset_7);
}

void pdp_hw_s_paf_filter_cor(u32 __iomem *base_reg,
			struct pdp_filterCor_setting_t *corInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CORING_TY], corInfo->coring_ty);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CORING_TH], corInfo->coring_th);
}

void pdp_hw_s_paf_filter_bin(u32 __iomem *base_reg,
			struct pdp_filterBin_setting_t *binInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BIN_FIRST], binInfo->bin_first);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BINNING_NUM_LMV_H], binInfo->binning_num_lmv_h);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BINNING_NUM_LMV_V], binInfo->binning_num_lmv_v);
}

void pdp_hw_s_paf_filter_band(u32 __iomem *base_reg,
			struct pdp_filterBand_setting_t *iirInfo, int filter_no)
{
	switch (filter_no) {
	case FILTER_BAND_0:
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_G0], iirInfo->gain0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_K01], iirInfo->k01);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_K02], iirInfo->k02);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_FTYPE0], iirInfo->type0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_G1], iirInfo->gain1);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_K11], iirInfo->k11);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_K12], iirInfo->k12);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_C11], iirInfo->c11);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_C12], iirInfo->c12);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_G2], iirInfo->gain2);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_K21], iirInfo->k21);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_K22], iirInfo->k22);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_C21], iirInfo->c21);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_C22], iirInfo->c22);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_BY0], iirInfo->bypass0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_BY1], iirInfo->bypass1);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_BY2], iirInfo->bypass2);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_COR_TYPE_B0], iirInfo->cor_type_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CORING_TY_B0], iirInfo->coring_ty_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CORING_TH_B0], iirInfo->coring_th_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I0_CORING], iirInfo->coring_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BIN_FIRST_B0], iirInfo->bin_first_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BINNING_NUM_B0], iirInfo->binning_num_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BIN0_SKIP], iirInfo->bin_skip_b);
		break;
	case FILTER_BAND_1:
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_G0], iirInfo->gain0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_K01], iirInfo->k01);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_K02], iirInfo->k02);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_FTYPE0], iirInfo->type0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_G1], iirInfo->gain1);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_K11], iirInfo->k11);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_K12], iirInfo->k12);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_C11], iirInfo->c11);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_C12], iirInfo->c12);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_G2], iirInfo->gain2);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_K21], iirInfo->k21);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_K22], iirInfo->k22);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_C21], iirInfo->c21);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_C22], iirInfo->c22);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_BY0], iirInfo->bypass0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_BY1], iirInfo->bypass1);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_BY2], iirInfo->bypass2);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_COR_TYPE_B1], iirInfo->cor_type_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CORING_TY_B1], iirInfo->coring_ty_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CORING_TH_B1], iirInfo->coring_th_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I1_CORING], iirInfo->coring_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BIN_FIRST_B1], iirInfo->bin_first_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BINNING_NUM_B1], iirInfo->binning_num_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BIN1_SKIP], iirInfo->bin_skip_b);
		break;
	case FILTER_BAND_2:
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_G0], iirInfo->gain0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_K01], iirInfo->k01);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_K02], iirInfo->k02);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_FTYPE0], iirInfo->type0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_G1], iirInfo->gain1);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_K11], iirInfo->k11);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_K12], iirInfo->k12);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_C11], iirInfo->c11);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_C12], iirInfo->c12);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_G2], iirInfo->gain2);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_K21], iirInfo->k21);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_K22], iirInfo->k22);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_C21], iirInfo->c21);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_C22], iirInfo->c22);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_BY0], iirInfo->bypass0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_BY1], iirInfo->bypass1);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_BY2], iirInfo->bypass2);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_COR_TYPE_B2], iirInfo->cor_type_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CORING_TY_B2], iirInfo->coring_ty_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_CORING_TH_B2], iirInfo->coring_th_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_I2_CORING], iirInfo->coring_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BIN_FIRST_B2], iirInfo->bin_first_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BINNING_NUM_B2], iirInfo->binning_num_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BIN2_SKIP], iirInfo->bin_skip_b);
		break;
	case FILTER_BAND_L:
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_G0], iirInfo->gain0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_K01], iirInfo->k01);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_K02], iirInfo->k02);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_FTYPE0], iirInfo->type0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_G1], iirInfo->gain1);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_K11], iirInfo->k11);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_K12], iirInfo->k12);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_C11], iirInfo->c11);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_C12], iirInfo->c12);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_G2], iirInfo->gain2);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_K21], iirInfo->k21);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_K22], iirInfo->k22);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_C21], iirInfo->c21);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_C22], iirInfo->c22);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_BY0], iirInfo->bypass0);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_BY1], iirInfo->bypass1);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_BY2], iirInfo->bypass2);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_IL_CORING], iirInfo->coring_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BINNING_NUM_LMV], iirInfo->binning_num_b);
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_BINL_SKIP], iirInfo->bin_skip_b);
		break;
	default:
		err("invalid IIR filter number(%d)\n", filter_no);
		break;
	}
}

void pdp_hw_s_paf_main_window(u32 __iomem *base_reg,
			struct pdp_main_wininfo *winInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWM_CX], winInfo->center_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWM_CY], winInfo->center_y);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWM_SX], winInfo->start_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWM_SY], winInfo->start_y);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWM_EX], winInfo->end_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWM_EY], winInfo->end_y);
}

void pdp_hw_s_paf_single_window(u32 __iomem *base_reg,
			struct pdp_single_wininfo *winInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_SROI], winInfo->sroi);

	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S1SX], winInfo->single_win[0].start_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S1SY], winInfo->single_win[0].start_y);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S1EX], winInfo->single_win[0].end_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S1EY], winInfo->single_win[0].end_y);

	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S2SX], winInfo->single_win[1].start_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S2SY], winInfo->single_win[1].start_y);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S2EX], winInfo->single_win[1].end_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S2EY], winInfo->single_win[1].end_y);

	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S3SX], winInfo->single_win[2].start_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S3SY], winInfo->single_win[2].start_y);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S3EX], winInfo->single_win[2].end_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S3EY], winInfo->single_win[2].end_y);

	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S4SX], winInfo->single_win[3].start_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S4SY], winInfo->single_win[3].start_y);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S4EX], winInfo->single_win[3].end_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_S4EY], winInfo->single_win[3].end_y);
}

void pdp_hw_s_paf_multi_window(u32 __iomem *base_reg,
			struct pdp_multi_wininfo *winInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWS_ON], winInfo->mode_on);

	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWS_SX], winInfo->start_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWS_SY], winInfo->start_y);

	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWS_SIZE_X], winInfo->size_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWS_SIZE_Y], winInfo->size_y);

	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWS_GAP_X], winInfo->gap_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWS_GAP_Y], winInfo->gap_y);

	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWS_NO_X], winInfo->no_x);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_MWS_NO_Y], winInfo->no_y);
}

void pdp_hw_s_wdr_setting(u32 __iomem *base_reg,
			struct pdp_wdr_setting_t *wdrInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_WDR_ON], wdrInfo->wdr_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_WDR_COEF_LONG], wdrInfo->wdr_coef_long);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_WDR_COEF_SHORT], wdrInfo->wdr_coef_short);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_WDR_SHFT_LONG], wdrInfo->wdr_shft_long);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PAF_I_WDR_SHFT_SHORT], wdrInfo->wdr_shft_short);
}

void pdp_hw_s_depth_setting(u32 __iomem *base_reg,
			struct pdp_depth_setting_t *depthInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DMODE], depthInfo->depth_dmode);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DSHIFT], depthInfo->depth_dshift);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DKSIZE1], depthInfo->depth_dsize1);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DKSIZE2], depthInfo->depth_dsize2);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DBIN_UP], depthInfo->depth_dbin_up);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DSCALE_UP], depthInfo->depth_dscale_up);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DSMOOTHE_ON], depthInfo->depth_dsmoothe_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DSCAN_ON], depthInfo->depth_dscan_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DMEDIAN_ON], depthInfo->depth_dmedian_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DSLOPE_ON], depthInfo->depth_dslope_on);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DARM_EDGE_H], depthInfo->depth_darm_edge_h);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DMEDIAN_SIZE], depthInfo->depth_dmedian_size);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DSCAN_EDGE], depthInfo->depth_dscan_edge);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DSCAN_PENALTY], depthInfo->depth_dscan_penalty);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DOUT_STAT_MODE], depthInfo->depth_dout_stat_mode);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DINVERSE_SNR], depthInfo->depth_dinverse_snr);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DCOEF_SNR], depthInfo->depth_dcoef_snr);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DCOEF_EDGEH], depthInfo->depth_dcoef_edgeh);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DCOEF_EDGEV], depthInfo->depth_dcoef_edgev);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DSHIFT_FILTER], depthInfo->depth_dshift_filter);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DADD_FILTER], depthInfo->depth_dadd_filter);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DMASK_FILTER1], depthInfo->depth_dmask_filter_1);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DMASK_FILTER2], depthInfo->depth_dmask_filter_2);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DMASK_FILTER3], depthInfo->depth_dmask_filter_3);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DMASK_FILTER4], depthInfo->depth_dmask_filter_4);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_DEPTH_I_DMASK_FILTER5], depthInfo->depth_dmask_filter_5);
}

void pdp_hw_s_y_ext_setting(u32 __iomem *base_reg,
			struct pdp_YextParam_setting_t *yExtInfo)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_MAX_NO_SKIP_PXG], yExtInfo->max_no_skippxg);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_SKIP_LEVEL_TH], yExtInfo->skip_levelth);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_SAT_VALUE_G], yExtInfo->px_sat_g);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_SAT_VALUE_R], yExtInfo->px_sat_r);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_SAT_VALUE_B], yExtInfo->px_sat_b);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_NUM_OF_SAT_G], yExtInfo->sat_no_g);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_NUM_OF_SAT_R], yExtInfo->sat_no_r);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_NUM_OF_SAT_B], yExtInfo->sat_no_b);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_SAT_VAL_SUM_OF_G], yExtInfo->sat_g);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_SAT_VAL_SUM_OF_R], yExtInfo->sat_r);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_SAT_VAL_SUM_OF_B], yExtInfo->sat_b);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_CLIP_VAL_LEFT], yExtInfo->clip_val_left);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_CLIP_VAL_RIGHT], yExtInfo->clip_val_right);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_COEF_R], yExtInfo->coef_r_short);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_COEF_G], yExtInfo->coef_g_short);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_COEF_B], yExtInfo->coef_b_short);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_YSHIFT], yExtInfo->y_shift);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_COEF_G_SHORT], yExtInfo->coef_r_long);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_COEF_R_SHORT], yExtInfo->coef_g_long);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_YEXTR_I_COEF_B_SHORT], yExtInfo->coef_b_long);
}

int pdp_hw_s_irq_msk(u32 __iomem *base_reg, u32 msk)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PDP_INT], 0);
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PDP_INT_MASK], msk);

	return 0;
}

void pdp_hw_s_reset(u32 __iomem *base_reg, u32 val)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PDP_SWRESET0], val);
}

int pdp_hw_g_irq_state(u32 __iomem *base_reg, bool clear)
{
	u32 src = 0;

	src = fimc_is_hw_get_reg(base_reg, &pdp_regs[PDP_R_PDP_INT]);
	if (clear)
		fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PDP_INT], src);

	return src;
}

void pdp_hw_clr_irq_state(u32 __iomem *base_reg, int state)
{
	fimc_is_hw_set_reg(base_reg, &pdp_regs[PDP_R_PDP_INT], state);
}

int pdp_hw_dump(u32 __iomem *base_reg)
{
	info("PDP SFR DUMP (v1.0)\n");
	fimc_is_hw_dump_regs(base_reg, pdp_regs, PDP_REG_CNT);

	return 0;
}
