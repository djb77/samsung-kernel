/* linux/drivers/video/exynos_decon/panel/dsim_backlight.c
 *
 * Header file for Samsung MIPI-DSI Backlight driver.
 *
 * Copyright (c) 2013 Samsung Electronics
 * Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#include <linux/backlight.h>

#include "../dsim.h"
#include "dsim_backlight.h"
#include "panel_info.h"

#ifdef CONFIG_PANEL_AID_DIMMING
#include "aid_dimming.h"
#endif


#ifdef CONFIG_PANEL_AID_DIMMING

static unsigned char *get_aid_from_index(struct dsim_device *dsim, int index)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_aid_err;
	}

	if (index > MAX_BR_INFO)
		index = MAX_BR_INFO;

	return (u8 *)dimming_info[index].aid;

get_aid_err:
	return NULL;
}


#ifdef AID_INTERPOLATION

static void get_aid_interpolation(struct dsim_device *dsim, u8* aid)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;
	int current_pbr = panel->bd->props.brightness;


	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_aid_err;
	}

	if (current_pbr > UI_MAX_BRIGHTNESS)
		current_pbr = UI_MAX_BRIGHTNESS;

	if (panel->inter_aor_tbl == NULL)
		goto get_aid_err;
	aid[0] = 0xB1;
	aid[1] = panel->inter_aor_tbl[current_pbr * 2];
	aid[2] = panel->inter_aor_tbl[current_pbr * 2 + 1];

	dsim_info("%s %d aid : %x : %x : %x\n", __func__, current_pbr, aid[0], aid[1], aid[2]);

get_aid_err:
	aid = NULL;

}
static int is_panel_aid_interpolation(struct dsim_device *dsim)
{
	struct panel_private *panel = &dsim->priv;
	if(panel->inter_aor_tbl == NULL) {
		dsim_info("%s panel does not support detailed aid\n", __func__);
		return 0;
	}
	if(panel->weakness_hbm_comp == HBM_COLORBLIND_ON) {
		dsim_info("%s color blind old version\n", __func__);
		return 0;
	}
	if(panel->interpolation == 1) {
		dsim_info("%s hbm brightness\n", __func__);
		return 0;
	}
#ifdef CONFIG_LCD_WEAKNESS_CCB
	if(panel->current_ccb != 0) {
		dsim_info("%s new color blind\n", __func__);
		return 0;
	}
#endif
#ifdef CONFIG_LCD_BURNIN_CORRECTION
	if(panel->ldu_correction_state != 0) {
		dsim_info("%s ldu correction\n", __func__);
		return 0;
	}
#endif
	return 1;
}
#endif


static int get_acutal_br_index(struct dsim_device *dsim, int br)
{
	int i;
	int min;
	int gap;
	int index = 0;
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming_info is NULL\n", __func__);
		return 0;
	}

	min = MAX_BRIGHTNESS;

	for (i = 0; i < MAX_BR_INFO; i++) {
		if (br > dimming_info[i].br)
			gap = br - dimming_info[i].br;
		else
			gap = dimming_info[i].br - br;

		if (gap == 0) {
			index = i;
			break;
		}

		if (gap < min) {
			min = gap;
			index = i;
		}
	}
	return index;
}

static unsigned int get_actual_br_value(struct dsim_device *dsim, int index)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_br_err;
	}

	if (index > MAX_BR_INFO)
		index = MAX_BR_INFO;

	return dimming_info[index].br;

get_br_err:
	return 0;
}
static unsigned char *get_gamma_from_index(struct dsim_device *dsim, int index)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_gamma_err;
	}

	if (index > MAX_BR_INFO)
		index = MAX_BR_INFO;

	return (unsigned char *)dimming_info[index].gamma;

get_gamma_err:
	return NULL;
}


static unsigned char *get_elvss_from_index(struct dsim_device *dsim, int index, int caps)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_elvess_err;
	}

	if(caps)
		return (unsigned char *)dimming_info[index].elvCaps;
	else
		return (unsigned char *)dimming_info[index].elv;

get_elvess_err:
	return NULL;
}


static void dsim_panel_gamma_ctrl(struct dsim_device *dsim)
{
	u8 *gamma = NULL;
	gamma = get_gamma_from_index(dsim, dsim->priv.br_index);
	if (gamma == NULL) {
		dsim_err("%s :faied to get gamma\n", __func__);
		return;
	}

	if (dsim_write_hl_data(dsim, gamma, GAMMA_CMD_CNT) < 0)
		dsim_err("%s : failed to write gamma \n", __func__);
}

static char dsim_panel_get_elvssoffset(struct dsim_device *dsim)
{
	char retVal = 0x00;
	struct panel_private* panel = &(dsim->priv);
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->dim_info;

	if(UNDER_MINUS_15(panel->temperature)) {
		retVal = dimming_info[panel->br_index].elvss_offset[UNDER_MINUS_FIFTEEN];
		pr_info("%s under -20 %d %d\n",
			__func__, retVal, dimming_info[panel->br_index].elvss_offset[UNDER_MINUS_FIFTEEN]);
	} else if(UNDER_0(panel->temperature)) {
		retVal = dimming_info[panel->br_index].elvss_offset[UNDER_ZERO];
		pr_info("%s under 0 %d %d\n",
			__func__, retVal, dimming_info[panel->br_index].elvss_offset[UNDER_ZERO]);
	} else {
		retVal = dimming_info[panel->br_index].elvss_offset[OVER_ZERO];
		pr_info("%s over 0 %d %d\n",
			__func__, retVal, dimming_info[panel->br_index].elvss_offset[OVER_ZERO]);
	}

	return retVal;

}

static void dsim_panel_aid_ctrl(struct dsim_device *dsim)
{
	u8 *aid = NULL;
	u8 recv[3];
	unsigned char SEQ_AID[AID_LEN_MAX + 1] = { 0, };
	struct panel_private *panel = &dsim->priv;

#ifdef AID_INTERPOLATION
	if(is_panel_aid_interpolation(dsim)) {
		get_aid_interpolation(dsim, recv);
		aid = recv;
	}
	else
		aid = get_aid_from_index(dsim, dsim->priv.br_index);
#else
	aid = get_aid_from_index(dsim, dsim->priv.br_index);
#endif


	if (aid == NULL) {
		dsim_err("%s : faield to get aid value\n", __func__);
		return;
	}

	memcpy(SEQ_AID, panel->aid_set, panel->aid_len);
	memcpy(&SEQ_AID[panel->aid_reg_offset], aid + 1, 2);

	if (dsim_write_hl_data(dsim, SEQ_AID, panel->aid_len) < 0)
		dsim_err("%s : failed to write aid \n", __func__);
}

static void dsim_panel_set_elvss(struct dsim_device *dsim)
{
	u8 *elvss = NULL;
	unsigned char SEQ_ELVSS[ELVSS_LEN_MAX + 1] = {0, };
	struct panel_private *panel = &dsim->priv;

	memcpy(SEQ_ELVSS, panel->elvss_set, panel->elvss_len);
	elvss = get_elvss_from_index(dsim, dsim->priv.br_index, dsim->priv.caps_enable);
	if (elvss == NULL) {
		dsim_err("%s : failed to get elvss value\n", __func__);
		return;
	}
	memcpy(&SEQ_ELVSS[panel->elvss_start_offset], elvss, 2);

	SEQ_ELVSS[panel->elvss_temperature_offset] += dsim_panel_get_elvssoffset(dsim);

	if(panel->elvss_tset_offset != 0xff) {
		SEQ_ELVSS[panel->elvss_tset_offset] = (panel->temperature < 0) ? BIT(7) | abs(panel->temperature) : panel->temperature;
	}
	if (dsim_write_hl_data(dsim, SEQ_ELVSS, panel->elvss_len) < 0)
		dsim_err("%s : failed to write elvss \n", __func__);
}


static int dsim_panel_set_acl(struct dsim_device *dsim, int force)
{
	int ret = 0;
	int level = ACL_OPR_15P, enabled = ACL_STATUS_ON;
	struct panel_private *panel = &dsim->priv;

	if (panel == NULL) {
		dsim_err("%s : panel is NULL\n", __func__);
		goto set_acl_exit;
	}
	level = panel->acl_enable;
	if(!panel->acl_enable)
		enabled = ACL_STATUS_OFF;

	if(force || panel->current_acl != level) {
		if((ret = dsim_write_hl_data(dsim,  panel->acl_opr_tbl[level], ACL_OPR_LEN)) < 0) {
			dsim_err("fail to write acl opr command.\n");
			goto set_acl_exit;
		}
		if((ret = dsim_write_hl_data(dsim, panel->acl_cutoff_tbl[enabled], ACL_CMD_LEN)) < 0) {
			dsim_err("fail to write acl command.\n");
			goto set_acl_exit;
		}
		panel->current_acl = level;
		dsim_info("acl : %x, opr: %x\n",
			panel->acl_cutoff_tbl[enabled][1], panel->acl_opr_tbl[level][ACL_OPR_LEN - 1]);
		dsim_info("acl: %d(%x), auto_brightness: %d\n", panel->current_acl, panel->acl_opr_tbl[level][ACL_OPR_LEN - 1], panel->auto_brightness);
	}
set_acl_exit:
	if (!ret)
		ret = -EPERM;
	return ret;
}

int dsim_panel_set_acl_step(struct dsim_device *dsim, int old_level, int new_level, int step, int pos )
{
	int ret;
	int old_param, new_param;
	unsigned char acl_opr[ ACL_OPR_LEN ];
	int value;
	struct panel_private *panel = &dsim->priv;

	memcpy( acl_opr, &(panel->acl_opr_tbl[ACL_STATUS_MAX-1][0]), ACL_OPR_LEN );
	new_param = panel->acl_opr_tbl[new_level][ACL_OPR_LEN - 1];
	old_param = panel->acl_opr_tbl[old_level][ACL_OPR_LEN - 1];

	value = old_param +((new_param -old_param) *pos /step);
	ret = value;

	acl_opr[ACL_OPR_LEN -1] = value;
	{
		if((ret = dsim_write_hl_data(dsim,  acl_opr, ACL_OPR_LEN)) < 0) {
			dsim_err("fail to write acl opr command.\n");
			goto set_acl_stop_exit;
		}
		if((ret = dsim_write_hl_data(dsim, panel->acl_cutoff_tbl[1], ACL_CMD_LEN)) < 0) {	// 1 = acl_enabled
			dsim_err("fail to write acl command.\n");
			goto set_acl_stop_exit;
		}
		dsim_info("acl(step) : %x, opr: %x, from %x to %x, %d/%d\n",
			panel->acl_cutoff_tbl[1][1], acl_opr[ACL_OPR_LEN - 1], old_param, new_param, pos, step );
	}
set_acl_stop_exit:
	if (!ret)
		ret = -EPERM;
	return ret;
}

static int is_panel_irc_ctrl(struct dsim_device *dsim)
{
	struct panel_private *panel = &dsim->priv;
	if(panel->weakness_hbm_comp == HBM_COLORBLIND_ON) {
		dsim_info("%s color blind old version\n", __func__);
		return 0;
	}
#ifdef CONFIG_LCD_WEAKNESS_CCB
	if(panel->current_ccb != 0) {
		dsim_info("%s new color blind\n", __func__);
		return 0;
	}
#endif

#ifdef CONFIG_LCD_BURNIN_CORRECTION
	if(panel->ldu_correction_state != 0) {
		dsim_info("%s ldu correction\n", __func__);
		return 0;
	}
#endif
	return 1;
}


static void dsim_panel_irc_ctrl(struct dsim_device *dsim, int on)
{
	struct panel_private* panel = &(dsim->priv);
	int irc_cmd_cnt = IRC_CMD_CNT_HF4;
	int p_br = panel->bd->props.brightness;
	int max_br_index = get_acutal_br_index(dsim, panel->br_tbl[255]);
	int hbm_irc_index = 0;

	if (panel->irc_table[0][0] == 0x00) {
		dsim_info("%s : do not support irc\n", __func__);
		return;
	}

	if(on) {
		if(panel->interpolation == 0) {
			if (dsim_write_hl_data(dsim, panel->irc_table[p_br], irc_cmd_cnt) < 0)
				dsim_err("%s : failed to write gamma \n", __func__);
			pr_info("%s : p_br : %d\n", __func__, p_br);
		} else {
			hbm_irc_index = panel->br_index - max_br_index - 1;
			if (dsim_write_hl_data(dsim, panel->irc_table_hbm[hbm_irc_index], irc_cmd_cnt) < 0)
				dsim_err("%s : failed to write gamma \n", __func__);
			pr_info("%s : hbm_irc : %d\n", __func__, hbm_irc_index);
		}
	} else {
		if (dsim_write_hl_data(dsim, HF4_A3_IRC_off, ARRAY_SIZE(HF4_A3_IRC_off)) < 0)
			dsim_err("%s : failed to write gamma \n", __func__);
	}
}


static int dsim_panel_set_vint(struct dsim_device *dsim, int force)
{
	int ret = 0;
	int nit = 0;
	int i, level = 0;
	struct panel_private* panel = &(dsim->priv);
	unsigned char SEQ_VINT[VINT_LEN_MAX] = {0x00, };
	unsigned char *vint_tbl = panel->vint_table;
	unsigned int *vint_dim_tbl = panel->vint_dim_table;

	level = panel->vint_table_len - 1;

	memcpy(SEQ_VINT, panel->vint_set, panel->vint_len);

	if(UNDER_MINUS_15(panel->temperature))
		goto set_vint;
#ifdef CONFIG_LCD_HMT
	if(panel->hmt_on == HMT_ON)
		goto set_vint;
#endif
	nit = get_actual_br_value(dsim, panel->br_index);

	for (i = 0; i < panel->vint_table_len; i++) {
		if (nit <= vint_dim_tbl[i]) {
			level = i;
			goto set_vint;
		}
	}
set_vint:
	if(force || panel->current_vint != vint_tbl[level]) {
		SEQ_VINT[panel->vint_len - 1] = vint_tbl[level];
		if ((ret = dsim_write_hl_data(dsim, SEQ_VINT, panel->vint_len)) < 0) {
			dsim_err("fail to write vint command.\n");
			ret = -EPERM;
		}
		panel->current_vint = vint_tbl[level];
		dsim_info("nit:%d level:%d vint:%02x",
			nit, level, panel->current_vint);
	}
	return ret;
}

static int low_level_set_brightness(struct dsim_device *dsim ,int force)
{

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) < 0)
		dsim_err("%s : fail to write F0 on command.\n", __func__);

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC)) < 0)
		dsim_err("%s : fail to write F0 on command.\n", __func__);

	dsim_panel_gamma_ctrl(dsim);

	dsim_panel_aid_ctrl(dsim);

	dsim_panel_set_elvss(dsim);

	dsim_panel_set_vint(dsim, force);

	dsim_panel_irc_ctrl(dsim, is_panel_irc_ctrl(dsim));

	if (dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE)) < 0)
		dsim_err("%s : failed to write gamma \n", __func__);

	if (dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE_L, ARRAY_SIZE(SEQ_GAMMA_UPDATE_L)) < 0)
			dsim_err("%s : failed to write gamma \n", __func__);

	dsim_panel_set_acl(dsim, force);

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC)) < 0)
			dsim_err("%s : fail to write F0 on command.\n", __func__);

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0)) < 0)
		dsim_err("%s : fail to write F0 on command\n", __func__);

	return 0;
}

static int is_panel_hbm_on(struct dsim_device *dsim)
{
	struct panel_private *panel = &dsim->priv;
	int auto_offset = 0;
	int retVal = 0;
	int cur_br = 0;
	bool bIsHbmArea = (LEVEL_IS_HBM_AREA(panel->auto_brightness)
		&& (panel->bd->props.brightness == panel->bd->props.max_brightness));

	if(bIsHbmArea) {
		auto_offset = 13 - panel->auto_brightness;
		panel->br_index = MAX_BR_INFO - auto_offset;
		cur_br = get_actual_br_value(dsim, panel->br_index);
		panel->caps_enable = 1;
		panel->acl_enable = ACL_OPR_8P;
		dsim_info("%s cur_br : %d br_index : %d auto : %d\n",
			__func__, cur_br, panel->br_index, panel->auto_brightness);
		retVal = 1;
	} else {
		retVal = 0;
	}

	return retVal;
}

int get_panel_acl_on( int p_br, int weakness_hbm_comp )
{
	int retVal = ACL_OPR_15P;

	if(p_br == 255) {
		retVal = ACL_OPR_15P;
		if(weakness_hbm_comp == HBM_GALLERY_ON)
			retVal = ACL_OPR_OFF;
	}
	return retVal;
}

static int is_panel_acl_on(struct dsim_device *dsim, int p_br)
{
	struct panel_private *panel = &dsim->priv;

	return get_panel_acl_on( p_br, panel->weakness_hbm_comp );
}

#endif


int dsim_panel_set_brightness(struct dsim_device *dsim, int force)
{
	int ret = 0;
#ifndef CONFIG_PANEL_AID_DIMMING
	dsim_info("%s:this panel does not support dimming \n", __func__);
#else
	struct dim_data *dimming;
	struct panel_private *panel = &dsim->priv;
	int p_br = panel->bd->props.brightness;
	int acutal_br = 0;
	int real_br = 0;

	dimming = (struct dim_data *)panel->dim_data;
	if ((dimming == NULL) || (panel->br_tbl == NULL)) {
		dsim_info("%s : this panel does not support dimming\n", __func__);
		goto set_br_exit;
	}
	panel->interpolation = is_panel_hbm_on(dsim);
	if(panel->interpolation) {
		if(panel->current_ccb) {
			dsim_info("%s:color blind > auto_brightness  \n", __func__);
			goto set_br_exit;
		}
		goto set_brightness;
	}
	acutal_br = panel->br_tbl[p_br];
	panel->br_index = get_acutal_br_index(dsim, acutal_br);
	real_br = get_actual_br_value(dsim, panel->br_index);
	panel->caps_enable = CAPS_IS_ON(real_br);
	panel->acl_enable = is_panel_acl_on(dsim, p_br);

	dsim_info("%s : platform : %d, : mapping : %d, real : %d, index : %d\n",
		__func__, p_br, acutal_br, real_br, panel->br_index);

	if (!force)
		goto set_br_exit;

	if ((acutal_br == 0) || (real_br == 0))
		goto set_br_exit;

#ifdef CONFIG_LCD_HMT
	if(panel->hmt_on == HMT_ON) {
		pr_info("%s hmt is enabled, plz set hmt brightness \n", __func__);
		goto set_br_exit;
	}
#endif

set_brightness:
	if (panel->state != PANEL_STATE_RESUMED) {
		dsim_info("%s : panel is not active state..\n", __func__);
		goto set_br_exit;
	}

	mutex_lock(&panel->lock);

	ret = low_level_set_brightness(dsim, force);
	if (ret) {
		dsim_err("%s failed to set brightness : %d\n", __func__, acutal_br);
	}
	mutex_unlock(&panel->lock);

set_br_exit:
#endif
	return ret;

}

#ifdef CONFIG_LCD_HMT
#ifdef CONFIG_PANEL_AID_DIMMING
static unsigned char *get_gamma_from_index_for_hmt(struct dsim_device *dsim, int index)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->hmt_dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_gamma_err;
	}

	if (index > HMT_MAX_BR_INFO - 1)
		index = HMT_MAX_BR_INFO - 1;

	return (unsigned char *)dimming_info[index].gamma;

get_gamma_err:
	return NULL;
}

static unsigned char *get_aid_from_index_for_hmt(struct dsim_device *dsim, int index)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->hmt_dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_aid_err;
	}

	if (index > HMT_MAX_BR_INFO - 1)
		index = HMT_MAX_BR_INFO - 1;

	return (u8 *)dimming_info[index].aid;

get_aid_err:
	return NULL;
}

static unsigned char *get_elvss_from_index_for_hmt(struct dsim_device *dsim, int index, int caps)
{
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = (struct SmtDimInfo *)panel->hmt_dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming info is NULL\n", __func__);
		goto get_elvess_err;
	}

	if(caps)
		return (unsigned char *)dimming_info[index].elvCaps;
	else
		return (unsigned char *)dimming_info[index].elv;

get_elvess_err:
	return NULL;
}


static void dsim_panel_gamma_ctrl_for_hmt(struct dsim_device *dsim)
{
	u8 *gamma = NULL;
	gamma = get_gamma_from_index_for_hmt(dsim, dsim->priv.hmt_br_index);
	if (gamma == NULL) {
		dsim_err("%s :faied to get gamma\n", __func__);
		return;
	}

	if (dsim_write_hl_data(dsim, gamma, GAMMA_CMD_CNT) < 0)
		dsim_err("%s : failed to write hmt gamma \n", __func__);
}


static void dsim_panel_aid_ctrl_for_hmt(struct dsim_device *dsim)
{
	u8 *aid = NULL;
	unsigned char SEQ_AID[AID_LEN_MAX + 1] = { 0, };
	struct panel_private *panel = &dsim->priv;

	aid = get_aid_from_index_for_hmt(dsim, dsim->priv.hmt_br_index);

	if (aid == NULL) {
		dsim_err("%s : faield to get aid value\n", __func__);
		return;
	}
	memcpy(SEQ_AID, panel->aid_set, panel->aid_len);
	memcpy(&SEQ_AID[panel->aid_reg_offset], aid + 1, 2);

	if (dsim_write_hl_data(dsim, SEQ_AID, panel->aid_len) < 0)
		dsim_err("%s : failed to write aid \n", __func__);
}

static void dsim_panel_set_elvss_for_hmt(struct dsim_device *dsim)
{
	u8 *elvss = NULL;
	unsigned char SEQ_ELVSS[ELVSS_LEN_MAX + 1] = {0, };
	struct panel_private *panel = &dsim->priv;
	int i;

	memcpy(SEQ_ELVSS, panel->elvss_set, panel->elvss_len);
	elvss = get_elvss_from_index_for_hmt(dsim, dsim->priv.hmt_br_index, dsim->priv.caps_enable);
	if (elvss == NULL) {
		dsim_err("%s : failed to get elvss value\n", __func__);
		return;
	}
	memcpy(&SEQ_ELVSS[panel->elvss_start_offset], elvss, 2);

	SEQ_ELVSS[panel->elvss_temperature_offset] += dsim_panel_get_elvssoffset(dsim);

	if(panel->elvss_tset_offset != 0xff) {
		SEQ_ELVSS[panel->elvss_tset_offset] = (panel->temperature < 0) ? BIT(7) | abs(panel->temperature) : panel->temperature;
	}
	if (dsim_write_hl_data(dsim, SEQ_ELVSS, panel->elvss_len) < 0)
		dsim_err("%s : failed to write elvss \n", __func__);

	dsim_info("elvss len = %d\n", panel->elvss_len);
	for(i = 0; i < panel->elvss_len; i++)
		dsim_info("elvss %dth = %x\n", i, SEQ_ELVSS[i]);
}

static int low_level_set_brightness_for_hmt(struct dsim_device *dsim ,int force)
{
	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) < 0)
		dsim_err("%s : fail to write F0 on command.\n", __func__);
	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC)) < 0)
		dsim_err("%s : fail to write F0 on command.\n", __func__);

	dsim_panel_gamma_ctrl_for_hmt(dsim);

	dsim_panel_aid_ctrl_for_hmt(dsim);

	dsim_panel_set_elvss_for_hmt(dsim);

	dsim_panel_set_vint(dsim, force);
	dsim_panel_irc_ctrl(dsim, 0);

	if (dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE)) < 0)
		dsim_err("%s : failed to write gamma \n", __func__);
	if (dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE_L, ARRAY_SIZE(SEQ_GAMMA_UPDATE_L)) < 0)
		dsim_err("%s : failed to write gamma \n", __func__);

	dsim_panel_set_acl(dsim, force);

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC)) < 0)
		dsim_err("%s : fail to write F0 on command.\n", __func__);
	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0)) < 0)
		dsim_err("%s : fail to write F0 on command\n", __func__);

	return 0;
}

static int get_acutal_br_index_for_hmt(struct dsim_device *dsim, int br)
{
	int i;
	int min;
	int gap;
	int index = 0;
	struct panel_private *panel = &dsim->priv;
	struct SmtDimInfo *dimming_info = panel->hmt_dim_info;

	if (dimming_info == NULL) {
		dsim_err("%s : dimming_info is NULL\n", __func__);
		return 0;
	}

	min = HMT_MAX_BRIGHTNESS;

	for (i = 0; i < MAX_BR_INFO; i++) {
		if (br > dimming_info[i].br)
			gap = br - dimming_info[i].br;
		else
			gap = dimming_info[i].br - br;

		if (gap == 0) {
			index = i;
			break;
		}

		if (gap < min) {
			min = gap;
			index = i;
		}
	}
	return index;
}

#endif
int dsim_panel_set_brightness_for_hmt(struct dsim_device *dsim, int force)
{
	int ret = 0;
#ifndef CONFIG_PANEL_AID_DIMMING
	dsim_info("%s:this panel does not support dimming \n", __func__);
#else
	struct dim_data *dimming;
	struct panel_private *panel = &dsim->priv;
	int p_br = panel->hmt_brightness;
	int acutal_br = 0;
	int prev_index = panel->hmt_br_index;

	dimming = (struct dim_data *)panel->hmt_dim_data;
	if ((dimming == NULL) || (panel->hmt_br_tbl == NULL)) {
		dsim_info("%s : this panel does not support dimming\n", __func__);
		return ret;
	}

	acutal_br = panel->hmt_br_tbl[p_br];
	panel->acl_enable = ACL_OPR_OFF;
	panel->hmt_br_index = get_acutal_br_index_for_hmt(dsim, acutal_br);
	if(panel->siop_enable)					// check auto acl
		panel->acl_enable = ACL_OPR_15P;
	if (panel->state != PANEL_STATE_RESUMED) {
		dsim_info("%s : panel is not active state..\n", __func__);
		goto set_br_exit;
	}
	if (!force && panel->hmt_br_index == prev_index)
		goto set_br_exit;

	dsim_info("%s : req : %d : nit : %d index : %d\n",
		__func__, p_br, acutal_br, panel->hmt_br_index);

	if (acutal_br == 0)
		goto set_br_exit;

	mutex_lock(&panel->lock);

	ret = low_level_set_brightness_for_hmt(dsim, force);
	if (ret) {
		dsim_err("%s failed to hmt set brightness : %d\n", __func__, acutal_br);
	}
	mutex_unlock(&panel->lock);

set_br_exit:
#endif
	return ret;
}
#endif





static int panel_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}


static int panel_set_brightness(struct backlight_device *bd)
{
	int ret = 0;
	int brightness = bd->props.brightness;
	struct panel_private *priv = bl_get_data(bd);
	struct dsim_device *dsim;

	dsim = container_of(priv, struct dsim_device, priv);

	if (brightness < UI_MIN_BRIGHTNESS || brightness > UI_MAX_BRIGHTNESS) {
		printk(KERN_ALERT "Brightness should be in the range of 0 ~ 255\n");
		ret = -EINVAL;
		goto exit_set;
	}

	ret = dsim_panel_set_brightness(dsim, 1);
	if (ret) {
		dsim_err("%s : fail to set brightness\n", __func__);
		goto exit_set;
	}
exit_set:
	return ret;

}

static const struct backlight_ops panel_backlight_ops = {
	.get_brightness = panel_get_brightness,
	.update_status = panel_set_brightness,
};


int dsim_backlight_probe(struct dsim_device *dsim)
{
	int ret = 0;
	struct panel_private *panel = &dsim->priv;

	panel->bd = backlight_device_register("panel", dsim->dev, &dsim->priv,
					&panel_backlight_ops, NULL);
	if (IS_ERR(panel->bd)) {
		dsim_err("%s:failed register backlight\n", __func__);
		ret = PTR_ERR(panel->bd);
	}

	panel->bd->props.max_brightness = UI_MAX_BRIGHTNESS;
	panel->bd->props.brightness = UI_DEFAULT_BRIGHTNESS;

#ifdef CONFIG_LCD_HMT
	panel->hmt_on = HMT_OFF;
	panel->hmt_brightness = DEFAULT_HMT_BRIGHTNESS;
	panel->hmt_br_index = 0;
#endif
	return ret;
}
