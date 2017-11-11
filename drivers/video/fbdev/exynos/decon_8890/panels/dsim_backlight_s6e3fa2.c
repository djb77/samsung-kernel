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

unsigned dynamic_lcd_type;

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
#endif


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

	if(UNDER_MINUS_20(panel->temperature)) {
		retVal = dimming_info[panel->br_index].elvss_offset[UNDER_MINUS_TWENTY];
		pr_info("%s under -20 %d %d\n",
			__func__, retVal, dimming_info[panel->br_index].elvss_offset[UNDER_MINUS_TWENTY]);
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
	unsigned char SEQ_AID[AID_LEN_MAX + 1] = { 0, };
	struct panel_private *panel = &dsim->priv;
	int i;

#ifdef AID_INTERPOLATION
	u8 recv[3];
	if((panel->inter_aor_tbl!= NULL) && (panel->weakness_hbm_comp != HBM_COLORBLIND_ON)) {
		u8 recv[3];
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
	memcpy(&SEQ_AID[panel->aid_reg_offset], aid + 1, AID_LEN_MAX -1);

	if (dsim_write_hl_data(dsim, SEQ_AID, panel->aid_len) < 0)
		dsim_err("%s : failed to write aid \n", __func__);

	dsim_info("aid len = %d\n", panel->aid_len);
	dsim_info("(dsim) aid %dth = %02x %02x %02x %02x %02x\n", i, SEQ_AID[0], SEQ_AID[1], SEQ_AID[2], SEQ_AID[3], SEQ_AID[4]);

}

static void dsim_panel_set_elvss(struct dsim_device *dsim)
{
	u8 *elvss = NULL;
	unsigned char SEQ_ELVSS[ELVSS_LEN_MAX + 1] = {0, };
	struct panel_private *panel = &dsim->priv;
	int i;

	// kyNam_151104_
	return;

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


	dsim_info("elvss len = %d\n", panel->elvss_len);
	for(i = 0; i < panel->elvss_len; i++)
		dsim_info("elvss %dth = %x\n", i, SEQ_ELVSS[i]);
}


static int dsim_panel_set_acl(struct dsim_device *dsim, int force)
{
	int ret = 0, level = ACL_STATUS_8P;
	struct panel_private *panel = &dsim->priv;

	// kyNam_151104_
	return 0;

	if (panel == NULL) {
			dsim_err("%s : panel is NULL\n", __func__);
			goto exit;
	}

	if (panel->siop_enable || LEVEL_IS_HBM(panel->auto_brightness))  // auto acl or hbm is acl on
		goto acl_update;

	if (!panel->acl_enable)
		level = ACL_STATUS_0P;

acl_update:
	if(force || panel->current_acl != panel->acl_cutoff_tbl[level][1]) {
		if((ret = dsim_write_hl_data(dsim,  panel->acl_opr_tbl[level], 2)) < 0) {
			dsim_err("fail to write acl opr command.\n");
			goto exit;
		}
		if((ret = dsim_write_hl_data(dsim, panel->acl_cutoff_tbl[level], 2)) < 0) {
			dsim_err("fail to write acl command.\n");
			goto exit;
		}
		dsim_info("(dsim) acl : %x, opr: %x\n",
			panel->acl_cutoff_tbl[level][1], panel->acl_opr_tbl[level][1]);
		panel->current_acl = panel->acl_cutoff_tbl[level][1];
		dsim_info("(dsim) acl: %x, auto_brightness: %d\n", panel->current_acl, panel->auto_brightness);
	}
exit:
	if (!ret)
		ret = -EPERM;
	return ret;
}

static int dsim_panel_set_vint(struct dsim_device *dsim, int force)
{
	int ret = 0;
	int nit = 0;
	int i, level = 0;
	int arraySize = ARRAY_SIZE(VINT_DIM_TABLE);
	struct panel_private* panel = &(dsim->priv);
	unsigned char SEQ_VINT[VINT_LEN_MAX] = {0x00, };
	unsigned char *vint_tbl = panel->vint_table;

	return 0;	// kyNam_151104_
	
	level = arraySize - 1;

	memcpy(SEQ_VINT, panel->vint_set, panel->vint_len);

	if(UNDER_MINUS_20(panel->temperature))
		goto set_vint;
#ifdef CONFIG_LCD_HMT
	if(panel->hmt_on == HMT_ON)
		goto set_vint;
#endif
	nit = get_actual_br_value(dsim, panel->br_index);

	for (i = 0; i < arraySize; i++) {
		if (nit <= VINT_DIM_TABLE[i]) {
			level = i;
			goto set_vint;
		}
	}
set_vint:
	if(force || panel->current_vint != vint_tbl[level]) {
		SEQ_VINT[panel->vint_len] = vint_tbl[level];
		if ((ret = dsim_write_hl_data(dsim, SEQ_VINT, panel->vint_len)) < 0) {
			dsim_err("fail to write vint command.\n");
			ret = -EPERM;
		}
		panel->current_vint = vint_tbl[level];
		dsim_info("vint: %02x\n", panel->current_vint);
	}
	return ret;
}

static int low_level_set_brightness(struct dsim_device *dsim ,int force)
{
	dsim_info( "%s +\n", __func__ );

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) < 0)
		dsim_err("%s : fail to write F0 on command.\n", __func__);
	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC)) < 0)
		dsim_err("%s : fail to write F0 on command.\n", __func__);

	dsim_panel_gamma_ctrl(dsim);

	dsim_panel_aid_ctrl(dsim);

	dsim_panel_set_elvss(dsim);

	dsim_panel_set_vint(dsim, force);

	if (dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE)) < 0)
		dsim_err("%s : failed to write gamma \n", __func__);

//	if (dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE_L, ARRAY_SIZE(SEQ_GAMMA_UPDATE_L)) < 0)
//			dsim_err("%s : failed to write gamma \n", __func__);

	dsim_panel_set_acl(dsim, force);

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC)) < 0)
			dsim_err("%s : fail to write F0 on command.\n", __func__);
	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0)) < 0)
		dsim_err("%s : fail to write F0 on command\n", __func__);

	dsim_info( "%s -\n", __func__ );
	return 0;
}

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
	bool bIsHbm = (LEVEL_IS_HBM(panel->auto_brightness) && (p_br == panel->bd->props.max_brightness));

	dsim_info( "%s +\n", __func__ );

#ifdef CONFIG_LCD_HMT
	if(panel->hmt_on == HMT_ON) {
		pr_info("%s hmt is enabled, plz set hmt brightness \n", __func__);
		goto set_br_exit;
	}
#endif

	dimming = (struct dim_data *)panel->dim_data;
	if ((dimming == NULL) || (panel->br_tbl == NULL)) {
		dsim_info("%s : this panel does not support dimming\n", __func__);
		return ret;
	}
	acutal_br = panel->br_tbl[p_br];
	panel->br_index = get_acutal_br_index(dsim, acutal_br);
	real_br = get_actual_br_value(dsim, panel->br_index);
	panel->caps_enable = CAPS_IS_ON(real_br);
	panel->acl_enable = ACL_IS_ON(real_br);

	if(bIsHbm) {
		panel->br_index = panel->hbm_index;
		panel->acl_enable = 1;				// hbm is acl on
		panel->caps_enable = 1; 			// hbm is caps on
		dsim_info("%s hbm %d\n", __func__, panel->br_index);
	}
	if(panel->siop_enable)					// check auto acl
		panel->acl_enable = 1;

	if (panel->state != PANEL_STATE_RESUMED) {
		dsim_info("%s : panel is not active state..\n", __func__);
		goto set_br_exit;
	}

	dsim_info("%s : platform : %d, : mapping : %d, real : %d, index : %d\n",
		__func__, p_br, acutal_br, real_br, panel->br_index);

	if (!force)
		goto set_br_exit;

	if ((acutal_br == 0) || (real_br == 0))
		goto set_br_exit;

	mutex_lock(&panel->lock);

	ret = low_level_set_brightness(dsim, force);
	if (ret) {
		dsim_err("%s failed to set brightness : %d\n", __func__, acutal_br);
	}
	mutex_unlock(&panel->lock);

set_br_exit:
	dsim_info( "%s -\n", __func__ );
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
	int i;

	return ;
	aid = get_aid_from_index_for_hmt(dsim, dsim->priv.br_index);

	if (aid == NULL) {
		dsim_err("%s : faield to get aid value\n", __func__);
		return;
	}
	memcpy(SEQ_AID, panel->aid_set, panel->aid_len);
	memcpy(&SEQ_AID[panel->aid_reg_offset], aid + 1, 2);

	if (dsim_write_hl_data(dsim, SEQ_AID, panel->aid_len) < 0)
		dsim_err("%s : failed to write aid \n", __func__);

	dsim_info("aid len = %d\n", panel->aid_len);
	for(i = 0; i < panel->aid_len; i++)
		dsim_info("aid %dth = %x\n", i, SEQ_AID[i]);
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
	panel->acl_enable = 0;
	panel->hmt_br_index = get_acutal_br_index_for_hmt(dsim, acutal_br);
	if(panel->siop_enable)					// check auto acl
		panel->acl_enable = 1;
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
