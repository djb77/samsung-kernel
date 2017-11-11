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

#ifndef USE_PANEL_PARAMETER_MAX_SIZE
#define ELVSS_LEN_MAX ELVSS_LEN
#define TSET_LEN_MAX TSET_LEN
#endif

#ifdef CONFIG_PANEL_AID_DIMMING

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
	int nit = 0;
	char retVal = 0x00;
	unsigned char panelline;
	struct panel_private* panel = &(dsim->priv);
	bool bIsHbm = (LEVEL_IS_HBM(panel->auto_brightness) && (panel->bd->props.brightness == panel->bd->props.max_brightness));

	nit = panel->br_tbl[panel->bd->props.brightness];

	if((!bIsHbm) && (panel->interpolation)) {
		if(panel->weakness_hbm_comp == HBM_COLORBLIND_ON)
			retVal = -panel->hbm_elvss_comp;
		else if(panel->weakness_hbm_comp == HBM_GALLERY_ON)
			retVal = -HBM_INTER_22TH_OFFSET[panel->br_index - 65];
		else
			pr_info("%s invaid weakness_hbm_comp:%d\n", __func__, panel->weakness_hbm_comp);
		goto exit_get_elvss;
	}

	panelline = panel->id[0] & 0xc0;

#if defined(CONFIG_PANEL_S6E3HF2_DYNAMIC)		  // edge
	if (panel->a3_elvss_updated && panelline == S6E3HF2_A3_LINE_ID) {
		if (nit > 5)
			nit = 6;
		if (UNDER_MINUS_20(panel->temperature))
			retVal = panel->a3_elvss_temp_20[nit - 2];
		else if (UNDER_0(panel->temperature))
			retVal = panel->a3_elvss_temp_0[nit - 2];
		else
			retVal = 0x00;

		goto exit_get_elvss;
	}

#endif
	if(UNDER_MINUS_20(panel->temperature)) {
		switch(nit)
		{
#if defined(CONFIG_PANEL_S6E3HA2_DYNAMIC) || defined(CONFIG_PANEL_S6E3HA3_DYNAMIC)			// flat
		case 2:
			retVal = 0x00;
			break;
		case 3:
			retVal = -0x01;
			break;
		case 4:
			retVal = -0x02;
			break;
		case 5:
			retVal = -0x03;
			break;
		default:
			retVal = -0x04;
			break;
#elif defined(CONFIG_PANEL_S6E3HF2_DYNAMIC)		  // edge
		case 2:
			retVal = -0x01;
			break;
		case 3:
			retVal = -0x02;
			break;
		case 4:
			retVal = -0x03;
			break;
		case 5:
			retVal = -0x04;
			break;
		default:
			retVal = -0x05;
			break;
#endif
		}
	} else if(UNDER_0(panel->temperature)) {
		switch(nit)
		{
		case 2:
			retVal = 0x04;
			break;
		case 3:
			retVal = 0x03;
			break;
		case 4:
			retVal = 0x02;
			break;
		case 5:
			retVal = 0x01;
			break;
		default:
			retVal = 0x00;
			break;
		}
	} else {
		retVal = 0x00;
	}

exit_get_elvss:
	pr_info("%s %d\n", __func__, retVal);
	return retVal;
}

static void dsim_panel_aid_ctrl(struct dsim_device *dsim)
{
	u8 *aid = NULL;
	aid = get_aid_from_index(dsim, dsim->priv.br_index);
	if (aid == NULL) {
		dsim_err("%s : faield to get aid value\n", __func__);
		return;
	}
	if (dsim_write_hl_data(dsim, aid, AID_CMD_CNT) < 0)
		dsim_err("%s : failed to write gamma \n", __func__);
}

static void dsim_panel_set_elvss(struct dsim_device *dsim)
{
	u8 *elvss = NULL;
	unsigned char SEQ_ELVSS[ELVSS_LEN_MAX] = {0, };
	struct panel_private *panel = &dsim->priv;
	bool bIsHbm = (LEVEL_IS_HBM(panel->auto_brightness) && (panel->bd->props.brightness == panel->bd->props.max_brightness));

	SEQ_ELVSS[0] = ELVSS_REG;
	elvss = get_elvss_from_index(dsim, dsim->priv.br_index, dsim->priv.caps_enable);
	if (elvss == NULL) {
		dsim_err("%s : failed to get elvss value\n", __func__);
		return;
	}
	memcpy(&SEQ_ELVSS[1], dsim->priv.elvss_set, ELVSS_LEN - 1);
	memcpy(SEQ_ELVSS, elvss, ELVSS_CMD_CNT);

	SEQ_ELVSS[ELVSS_LEN - 1] += dsim_panel_get_elvssoffset(dsim);
	if(bIsHbm ||
		((panel->interpolation) && (panel->weakness_hbm_comp == HBM_COLORBLIND_ON)))
		SEQ_ELVSS[2] = 0x0A;
	if (dsim_write_hl_data(dsim, SEQ_ELVSS, ELVSS_LEN) < 0)
		dsim_err("%s : failed to write elvss \n", __func__);
}


static int dsim_panel_set_acl(struct dsim_device *dsim, int force)
{
	int ret = 0, level = ACL_STATUS_8P;
	struct panel_private *panel = &dsim->priv;

	if (panel == NULL) {
			dsim_err("%s : panel is NULL\n", __func__);
			goto exit;
	}

	if (dsim->priv.siop_enable || LEVEL_IS_HBM(dsim->priv.auto_brightness))  // auto acl or hbm is acl on
		goto acl_update;

	if (!dsim->priv.acl_enable)
		level = ACL_STATUS_0P;

acl_update:
	if(force || dsim->priv.current_acl != panel->acl_cutoff_tbl[level][1]) {
		if((ret = dsim_write_hl_data(dsim,  panel->acl_opr_tbl[level], 2)) < 0) {
			dsim_err("fail to write acl opr command.\n");
			goto exit;
		}
		if((ret = dsim_write_hl_data(dsim, panel->acl_cutoff_tbl[level], 2)) < 0) {
			dsim_err("fail to write acl command.\n");
			goto exit;
		}
		dsim->priv.current_acl = panel->acl_cutoff_tbl[level][1];
		dsim_info("acl: %d, auto_brightness: %d\n", dsim->priv.current_acl, dsim->priv.auto_brightness);
	}
exit:
	if (!ret)
		ret = -EPERM;
	return ret;
}


static int dsim_panel_set_tset(struct dsim_device *dsim, int force)
{
	int ret = 0;
	int tset = 0;
	unsigned char SEQ_TSET[TSET_LEN_MAX] = {0, };

	SEQ_TSET[0] = TSET_REG;
	tset = (dsim->priv.temperature < 0) ? BIT(7) | abs(dsim->priv.temperature) : dsim->priv.temperature;

	if(force || dsim->priv.tset[TSET_LEN - 2] != tset) {
		memcpy(&SEQ_TSET[1], dsim->priv.tset, TSET_LEN - 1);
		dsim->priv.tset[TSET_LEN - 2] = SEQ_TSET[TSET_LEN - 1] = tset;
		if ((ret = dsim_write_hl_data(dsim, SEQ_TSET, ARRAY_SIZE(SEQ_TSET))) < 0) {
			dsim_err("fail to write tset command.\n");
			ret = -EPERM;
		}
		dsim_info("%s temperature: %d, tset: %d\n",
			__func__, dsim->priv.temperature, SEQ_TSET[TSET_LEN - 1]);
	}
	return ret;
}

static int dsim_panel_set_vint(struct dsim_device *dsim, int force)
{
	int ret = 0;
	int nit = 0;
	int i, level = 0;
	int arraySize = ARRAY_SIZE(VINT_DIM_TABLE);
	struct panel_private* panel = &(dsim->priv);
	unsigned char SEQ_VINT[VINT_LEN] = {VINT_REG, 0x8B, 0x21};
	unsigned char *vint_tbl = (unsigned char *)VINT_TABLE;

#if defined(CONFIG_PANEL_S6E3HF2_DYNAMIC)		  // edge
	unsigned char panelline = panel->id[0] & 0xc0;
	if (panel->a3_vint_updated && panelline == S6E3HF2_A3_LINE_ID) {
		vint_tbl = panel->a3_vint;
	}
#endif

	level = arraySize - 1;

	if(UNDER_MINUS_20(panel->temperature))
		goto set_vint;
#ifdef CONFIG_LCD_HMT
	if(panel->hmt_on == HMT_ON)
		goto set_vint;
#endif
	nit = get_actual_br_value(dsim, panel->br_index);

	level = arraySize - 1;

	for (i = 0; i < arraySize; i++) {
		if (nit <= VINT_DIM_TABLE[i]) {
			level = i;
			goto set_vint;
		}
	}
set_vint:
	if(force || panel->current_vint != vint_tbl[level]) {
		SEQ_VINT[VINT_LEN - 1] = vint_tbl[level];
		if ((ret = dsim_write_hl_data(dsim, SEQ_VINT, ARRAY_SIZE(SEQ_VINT))) < 0) {
			dsim_err("fail to write vint command.\n");
			ret = -EPERM;
		}
		panel->current_vint = vint_tbl[level];
		dsim_info("vint: %02x\n", panel->current_vint);
	}
	return ret;
}

static int dsim_panel_set_hbm(struct dsim_device *dsim, int force)
{
	int ret = 0, level = LEVEL_IS_HBM(dsim->priv.auto_brightness);
	struct panel_private *panel = &dsim->priv;

	if (panel == NULL) {
		dsim_err("%s : panel is NULL\n", __func__);
		goto exit;
	}

	if(force || dsim->priv.current_hbm != panel->hbm_tbl[level][1]) {
		dsim->priv.current_hbm = panel->hbm_tbl[level][1];
		if((ret = dsim_write_hl_data(dsim, panel->hbm_tbl[level], ARRAY_SIZE(SEQ_HBM_OFF))) < 0) {
			dsim_err("fail to write hbm command.\n");
			ret = -EPERM;
		}
		dsim_info("hbm: %d, auto_brightness: %d\n", dsim->priv.current_hbm, dsim->priv.auto_brightness);
	}
exit:
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

	if (dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE)) < 0)
		dsim_err("%s : failed to write gamma \n", __func__);
	if (dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE_L, ARRAY_SIZE(SEQ_GAMMA_UPDATE_L)) < 0)
		dsim_err("%s : failed to write gamma \n", __func__);

	dsim_panel_set_acl(dsim, force);

	dsim_panel_set_tset(dsim, force);

#ifdef CONFIG_LCD_ALPM
	if (!(dsim->priv.current_alpm && dsim->priv.alpm))
#endif
		dsim_panel_set_hbm(dsim, force);

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC)) < 0)
			dsim_err("%s : fail to write F0 on command.\n", __func__);
	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0)) < 0)
		dsim_err("%s : fail to write F0 on command\n", __func__);

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
	int prev_index = panel->br_index;
	bool bIsHbm = (LEVEL_IS_HBM(panel->auto_brightness) && (p_br == panel->bd->props.max_brightness));
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
	if (panel->weakness_hbm_comp == 1)
		acutal_br = panel->hbm_inter_br_tbl[p_br];
	else if(panel->weakness_hbm_comp == 2)
		acutal_br = panel->gallery_br_tbl[p_br];
	else
		acutal_br = panel->br_tbl[p_br];
	panel->br_index = get_acutal_br_index(dsim, acutal_br);
	real_br = get_actual_br_value(dsim, panel->br_index);
	panel->caps_enable = CAPS_IS_ON(real_br);
	panel->acl_enable = ACL_IS_ON(real_br);

	if(bIsHbm) {
		panel->br_index = panel->hbm_index;
		panel->acl_enable = 1;				// hbm is acl on
		panel->caps_enable = 1;				// hbm is caps on
	}
	if(panel->siop_enable)					// check auto acl
		panel->acl_enable = 1;

	if (real_br > MAX_BRIGHTNESS) {
		panel->interpolation = 1;
	} else {
		panel->interpolation = 0;
	}
	if (panel->weakness_hbm_comp) {
		panel->acl_enable = 1;
		if((!bIsHbm) && (p_br == 255))
			panel->acl_enable = 0;
	}
	if (panel->state != PANEL_STATE_RESUMED) {
		dsim_info("%s : panel is not active state..\n", __func__);
		goto set_br_exit;
	}

	dsim_info("%s : platform : %d, : mapping : %d, real : %d, index : %d, interpolation : %d\n",
		__func__, p_br, acutal_br, real_br, panel->br_index, panel->interpolation);

	if (!force && panel->br_index == prev_index)
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
	aid = get_aid_from_index_for_hmt(dsim, dsim->priv.hmt_br_index);
	if (aid == NULL) {
		dsim_err("%s : faield to get aid value\n", __func__);
		return;
	}
	if (dsim_write_hl_data(dsim, aid, AID_CMD_CNT + 1) < 0)
		dsim_err("%s : failed to write hmt aid \n", __func__);
}

static void dsim_panel_set_elvss_for_hmt(struct dsim_device *dsim)
{
	u8 *elvss = NULL;
	unsigned char SEQ_ELVSS[ELVSS_LEN_MAX] = {ELVSS_REG, };

	SEQ_ELVSS[0] = ELVSS_REG;
	elvss = get_elvss_from_index_for_hmt(dsim, dsim->priv.hmt_br_index, dsim->priv.acl_enable);
	if (elvss == NULL) {
		dsim_err("%s : failed to get elvss value\n", __func__);
		return;
	}
	memcpy(&SEQ_ELVSS[1], dsim->priv.elvss_set, ELVSS_LEN - 1);
	memcpy(SEQ_ELVSS, elvss, ELVSS_CMD_CNT);

	if(UNDER_MINUS_20(dsim->priv.temperature))				// tset
		SEQ_ELVSS[ELVSS_LEN - 1] -= TSET_MINUS_OFFSET;

	if (dsim_write_hl_data(dsim, SEQ_ELVSS, ELVSS_LEN) < 0)
		dsim_err("%s : failed to write elvss \n", __func__);
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

	dsim_panel_set_tset(dsim, force);

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

	ret = dsim_panel_set_brightness(dsim, 0);
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
