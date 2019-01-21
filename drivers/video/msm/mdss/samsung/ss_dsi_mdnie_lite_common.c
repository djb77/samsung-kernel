/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: jb09.kim
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/
#include "ss_dsi_mdnie_lite_common.h"

#define MDNIE_LITE_TUN_DEBUG

#ifdef MDNIE_LITE_TUN_DEBUG
#define DPRINT(x...)	printk(KERN_ERR "[mdnie_mdss] " x)
#else
#define DPRINT(x...)
#endif

static struct class *mdnie_class;
struct device *tune_mdnie_dev;
struct list_head mdnie_list;
struct mdnie_lite_tune_data mdnie_data;

char mdnie_app_name[][NAME_STRING_MAX] = {
	"UI_APP",
	"VIDEO_APP",
	"VIDEO_WARM_APP",
	"VIDEO_COLD_APP",
	"CAMERA_APP",
	"NAVI_APP",
	"GALLERY_APP",
	"VT_APP",
	"BROWSER_APP",
	"eBOOK_APP",
	"EMAIL_APP",
	"GAME_LOW_APP",
	"GAME_MID_APP",
	"GAME_HIGH_APP",
	"VIDEO_ENHANCER",
	"VIDEO_ENHANCER_THIRD",
	"TDMB_APP",
};

char mdnie_mode_name[][NAME_STRING_MAX] = {
	"DYNAMIC_MODE",
	"STANDARD_MODE",
#if defined(NATURAL_MODE_ENABLE)
	"NATURAL_MODE",
#endif
	"MOVIE_MODE",
	"AUTO_MODE",
	"READING_MODE",
};

char outdoor_name[][NAME_STRING_MAX] = {
	"OUTDOOR_OFF_MODE",
	"OUTDOOR_ON_MODE",
};

char mdnie_hdr_name[][NAME_STRING_MAX] = {
	"HDR_OFF",
	"HDR_1",
	"HDR_2",
	"HDR_3"
};

char mdnie_light_notification_name[][NAME_STRING_MAX] = {
	"LIGHT_NOTIFICATION_OFF",
	"LIGHT_NOTIFICATION_ON"
};

void send_dsi_tcon_mdnie_register(struct samsung_display_driver_data *vdd,
	struct dsi_cmd_desc *tune_data_dsi0, struct dsi_cmd_desc *tune_data_dsi1, struct mdnie_lite_tun_type *mdnie_tune_state)
{
	if (vdd == NULL || !vdd->support_mdnie_lite)
		return;

	/* DUAL PANEL CHECK */
	if (!IS_ERR_OR_NULL(tune_data_dsi0) && !IS_ERR_OR_NULL(tune_data_dsi1)) {
		if (vdd->support_hall_ic) {
			if (tune_data_dsi0 && tune_data_dsi1 && mdnie_tune_state &&
				mdnie_data.dsi0_bypass_mdnie_size &&
				mdnie_data.dsi1_bypass_mdnie_size) {
				/* foder open : 0(primary panel), close : 1(secondary panel)*/
				if (!vdd->display_status_dsi[DSI_CTRL_0].hall_ic_status) {
					/* primary(internal) panel */
					vdd->dtsi_data[DSI_CTRL_0].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmds = tune_data_dsi0;
					vdd->dtsi_data[DSI_CTRL_0].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmd_cnt = mdnie_data.dsi0_bypass_mdnie_size;

				} else {
					/* secondary(external) panel */
					vdd->dtsi_data[DSI_CTRL_1].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmds = tune_data_dsi1;
					vdd->dtsi_data[DSI_CTRL_1].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmd_cnt = mdnie_data.dsi1_bypass_mdnie_size;
				}

				DPRINT("DUAL index : %d hbm : %d mdnie_bypass : %d mdnie_accessibility : %d  mdnie_app: %d mdnie_mode : %d hdr : %d night_mode_enable : %d\n",
					vdd->display_status_dsi[DSI_CTRL_0].hall_ic_status, mdnie_tune_state->hbm_enable, mdnie_tune_state->mdnie_bypass, mdnie_tune_state->mdnie_accessibility,
					mdnie_tune_state->mdnie_app, mdnie_tune_state->mdnie_mode, mdnie_tune_state->hdr, mdnie_tune_state->night_mode_enable);

				mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_0], TX_MDNIE_TUNE);
			} else
				DPRINT("DUAL Command Tx Fail, tune_data_dsi0=%p(%d), tune_data_dsi1=%p(%d), vdd=%p, mdnie_tune_state=%p\n",
					tune_data_dsi0, mdnie_data.dsi0_bypass_mdnie_size,
					tune_data_dsi0, mdnie_data.dsi1_bypass_mdnie_size, vdd, mdnie_tune_state);
		} else {
			if (tune_data_dsi0 && tune_data_dsi1 && mdnie_tune_state &&
				mdnie_data.dsi0_bypass_mdnie_size &&
				mdnie_data.dsi1_bypass_mdnie_size) {
				vdd->dtsi_data[DSI_CTRL_0].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmds = tune_data_dsi0;
				vdd->dtsi_data[DSI_CTRL_0].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmd_cnt = mdnie_data.dsi0_bypass_mdnie_size;
				vdd->dtsi_data[DSI_CTRL_1].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmds = tune_data_dsi1;
				vdd->dtsi_data[DSI_CTRL_1].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmd_cnt = mdnie_data.dsi1_bypass_mdnie_size;

				/* TODO: Tx command */
				DPRINT("DUAL Command Tx Fail(TODO  DUAL PANEL), tune_data_dsi0=%p(%d), tune_data_dsi1=%p(%d), vdd=%p, mdnie_tune_state=%p\n",
					tune_data_dsi0, mdnie_data.dsi0_bypass_mdnie_size,
					tune_data_dsi0, mdnie_data.dsi1_bypass_mdnie_size, vdd, mdnie_tune_state);
			} else
				DPRINT("DUAL Command Tx Fail, tune_data_dsi0=%p(%d), tune_data_dsi1=%p(%d), vdd=%p, mdnie_tune_state=%p\n",
					tune_data_dsi0, mdnie_data.dsi0_bypass_mdnie_size,
					tune_data_dsi0, mdnie_data.dsi1_bypass_mdnie_size, vdd, mdnie_tune_state);
		}
	} else {
		if (tune_data_dsi0 && mdnie_tune_state &&
			mdnie_data.dsi0_bypass_mdnie_size) {
			DPRINT("SINGLE index : %d hbm : %d mdnie_bypass : %d mdnie_accessibility : %d  mdnie_app: %d mdnie_mode : %d hdr : %d night_mode_enable : %d\n",
				mdnie_tune_state->index, mdnie_tune_state->hbm_enable, mdnie_tune_state->mdnie_bypass, mdnie_tune_state->mdnie_accessibility,
				mdnie_tune_state->mdnie_app, mdnie_tune_state->mdnie_mode, mdnie_tune_state->hdr, mdnie_tune_state->night_mode_enable);

			if (vdd->ctrl_dsi[DSI_CTRL_0]->cmd_sync_wait_broadcast) { /* Dual DSI */
				vdd->dtsi_data[DSI_CTRL_1].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmds = tune_data_dsi0;
				vdd->dtsi_data[DSI_CTRL_1].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmd_cnt = mdnie_data.dsi0_bypass_mdnie_size;

				mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_1], TX_MDNIE_TUNE);
			} else { /* Single DSI */
				vdd->dtsi_data[DSI_CTRL_0].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmds = tune_data_dsi0;
				vdd->dtsi_data[DSI_CTRL_0].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmd_cnt = mdnie_data.dsi0_bypass_mdnie_size;
				mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_0], TX_MDNIE_TUNE);
			}
		} else
			DPRINT("SINGLE Command Tx Fail, tune_data_dsi0=%p(%d), vdd=%p, mdnie_tune_state=%p\n",
					tune_data_dsi0, mdnie_data.dsi0_bypass_mdnie_size, vdd, mdnie_tune_state);
	}
}

int update_dsi_tcon_mdnie_register(struct samsung_display_driver_data *vdd)
{
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct mdnie_lite_tun_type *real_mdnie_tune_state = NULL;
	struct dsi_cmd_desc *tune_data_dsi0 = NULL;
	struct dsi_cmd_desc *tune_data_dsi1 = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	
	if (vdd == NULL || !vdd->support_mdnie_lite)
		return 0;

	ctrl = samsung_get_dsi_ctrl(vdd);
	
	if(vdd->mfd_dsi[DISPLAY_1]->panel_info->cont_splash_enabled ||
		vdd->mfd_dsi[DISPLAY_1]->panel_info->blank_state == MDSS_PANEL_BLANK_BLANK ||
		!(ctrl->ctrl_state & CTRL_STATE_MDP_ACTIVE)) {
		LCD_ERR("do not send mdnie data (%d) (%d)\n",
			vdd->mfd_dsi[DISPLAY_1]->panel_info->cont_splash_enabled,
			vdd->mfd_dsi[DISPLAY_1]->panel_info->blank_state);
		return 0;
	}

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		real_mdnie_tune_state = mdnie_tune_state;
		/*
		*	Checking HBM mode first.
		*/
		if (mdnie_tune_state->vdd->lux >= mdnie_tune_state->vdd->enter_hbm_lux)
			mdnie_tune_state->hbm_enable = true;
		else
			mdnie_tune_state->hbm_enable = false;

		/*
		* mDnie priority
		* Accessibility > HBM > Screen Mode
		*/
		if (mdnie_tune_state->mdnie_bypass == BYPASS_ENABLE) {
			if (mdnie_tune_state->index == DSI_CTRL_0)
				tune_data_dsi0 = mdnie_data.DSI0_BYPASS_MDNIE;
			else
				tune_data_dsi1 = mdnie_data.DSI1_BYPASS_MDNIE;
		} else if (mdnie_tune_state->light_notification) {
			if (mdnie_tune_state->index == DSI_CTRL_0 && mdnie_data.light_notification_tune_value_dsi0)
				tune_data_dsi0 = mdnie_data.light_notification_tune_value_dsi0[mdnie_tune_state->light_notification];
			else if (mdnie_tune_state->index == DSI_CTRL_1 && mdnie_data.light_notification_tune_value_dsi1)
				tune_data_dsi1 = mdnie_data.light_notification_tune_value_dsi1[mdnie_tune_state->light_notification];
		} else if (mdnie_tune_state->mdnie_accessibility == COLOR_BLIND || mdnie_tune_state->mdnie_accessibility == COLOR_BLIND_HBM) {
			if (mdnie_tune_state->index == DSI_CTRL_0)
				tune_data_dsi0  = mdnie_data.DSI0_COLOR_BLIND_MDNIE;
			else
				tune_data_dsi1  = mdnie_data.DSI1_COLOR_BLIND_MDNIE;
		} else if (mdnie_tune_state->mdnie_accessibility == NEGATIVE) {
			if (mdnie_tune_state->index == DSI_CTRL_0)
				tune_data_dsi0  = mdnie_data.DSI0_NEGATIVE_MDNIE;
			else
				tune_data_dsi1  = mdnie_data.DSI1_NEGATIVE_MDNIE;
		} else if (mdnie_tune_state->mdnie_accessibility == CURTAIN) {
			if (mdnie_tune_state->index == DSI_CTRL_0)
				tune_data_dsi0  = mdnie_data.DSI0_CURTAIN;
			else
				tune_data_dsi1  = mdnie_data.DSI1_CURTAIN;
		} else if (mdnie_tune_state->mdnie_accessibility == GRAYSCALE) {
			if (mdnie_tune_state->index == DSI_CTRL_0)
				tune_data_dsi0  = mdnie_data.DSI0_GRAYSCALE_MDNIE;
			else
				tune_data_dsi1  = mdnie_data.DSI1_GRAYSCALE_MDNIE;
		} else if (mdnie_tune_state->mdnie_accessibility == GRAYSCALE_NEGATIVE) {
			if (mdnie_tune_state->index == DSI_CTRL_0)
				tune_data_dsi0  = mdnie_data.DSI0_GRAYSCALE_NEGATIVE_MDNIE;
			else
				tune_data_dsi1  = mdnie_data.DSI1_GRAYSCALE_NEGATIVE_MDNIE;
		} else if (mdnie_tune_state->color_lens_enable == true) {
			if (mdnie_tune_state->index == DSI_CTRL_0)
				tune_data_dsi0  = mdnie_data.DSI0_COLOR_LENS_MDNIE;
			else
				tune_data_dsi1  = mdnie_data.DSI1_COLOR_LENS_MDNIE;
		} else if (mdnie_tune_state->hdr) {
			if (mdnie_tune_state->index == DSI_CTRL_0 && mdnie_data.hdr_tune_value_dsi0)
				tune_data_dsi0 = mdnie_data.hdr_tune_value_dsi0[mdnie_tune_state->hdr];
			else if (mdnie_tune_state->index == DSI_CTRL_1 && mdnie_data.hdr_tune_value_dsi1)
				tune_data_dsi1 = mdnie_data.hdr_tune_value_dsi1[mdnie_tune_state->hdr];
		} else if (mdnie_tune_state->hmt_color_temperature) {
			if (mdnie_tune_state->index == DSI_CTRL_0 && mdnie_data.hmt_color_temperature_tune_value_dsi0)
				tune_data_dsi0 = mdnie_data.hmt_color_temperature_tune_value_dsi0[mdnie_tune_state->hmt_color_temperature];
			else if (mdnie_tune_state->index == DSI_CTRL_1 && mdnie_data.hmt_color_temperature_tune_value_dsi1)
				tune_data_dsi1 = mdnie_data.hmt_color_temperature_tune_value_dsi1[mdnie_tune_state->hmt_color_temperature];
		} else if (mdnie_tune_state->night_mode_enable == true) {
				if (mdnie_tune_state->index == DSI_CTRL_0)
					tune_data_dsi0  = mdnie_data.DSI0_NIGHT_MODE_MDNIE;
				else
					tune_data_dsi1  = mdnie_data.DSI1_NIGHT_MODE_MDNIE;
		} else if (mdnie_tune_state->hbm_enable == true) {
			if (vdd->dtsi_data[mdnie_tune_state->index].hbm_ce_text_mode_support &&
				((mdnie_tune_state->mdnie_app == BROWSER_APP) || (mdnie_tune_state->mdnie_app == eBOOK_APP)))
					if (mdnie_tune_state->index == DSI_CTRL_0)
						tune_data_dsi0  = mdnie_data.DSI0_HBM_CE_TEXT_MDNIE;
					else
						tune_data_dsi1  = mdnie_data.DSI1_HBM_CE_TEXT_MDNIE;
			else {
				if (mdnie_tune_state->index == DSI_CTRL_0)
					tune_data_dsi0  = mdnie_data.DSI0_HBM_CE_MDNIE;
				else
					tune_data_dsi1  = mdnie_data.DSI1_HBM_CE_MDNIE;
			}
		} else if (mdnie_tune_state->mdnie_app == EMAIL_APP) {
			/*
				Some kind of panel doesn't suooprt EMAIL_APP mode, but SSRM module use same control logic.
				It means SSRM doesn't consider panel unique character.
				To support this issue eBOOK_APP used insted of EMAIL_APP under EMAIL_APP doesn't exist status..
			*/
			if (mdnie_tune_state->index == DSI_CTRL_0) {
				tune_data_dsi0 = mdnie_data.mdnie_tune_value_dsi0[mdnie_tune_state->mdnie_app][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];

				if (!tune_data_dsi0)
					tune_data_dsi0 = mdnie_data.mdnie_tune_value_dsi0[eBOOK_APP][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];
			} else {
				tune_data_dsi1 = mdnie_data.mdnie_tune_value_dsi1[mdnie_tune_state->mdnie_app][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];

				if (!tune_data_dsi1)
					tune_data_dsi1 = mdnie_data.mdnie_tune_value_dsi1[eBOOK_APP][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];
			}
		} else {
			if (mdnie_tune_state->index == DSI_CTRL_0)
				tune_data_dsi0 = mdnie_data.mdnie_tune_value_dsi0[mdnie_tune_state->mdnie_app][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];
			else
				tune_data_dsi1 = mdnie_data.mdnie_tune_value_dsi1[mdnie_tune_state->mdnie_app][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];
		}

		if (vdd->support_mdnie_trans_dimming && vdd->mdnie_disable_trans_dimming) {
			if (mdnie_tune_state->index == DSI_CTRL_0) {
				if (tune_data_dsi0) {
					memcpy(mdnie_data.DSI0_RGB_SENSOR_MDNIE_1, tune_data_dsi0[mdnie_data.mdnie_step_index[MDNIE_STEP1]].payload, mdnie_data.dsi0_rgb_sensor_mdnie_1_size);
					memcpy(mdnie_data.DSI0_RGB_SENSOR_MDNIE_2, tune_data_dsi0[mdnie_data.mdnie_step_index[MDNIE_STEP2]].payload, mdnie_data.dsi0_rgb_sensor_mdnie_2_size);
					memcpy(mdnie_data.DSI0_RGB_SENSOR_MDNIE_3, tune_data_dsi0[mdnie_data.mdnie_step_index[MDNIE_STEP3]].payload, mdnie_data.dsi0_rgb_sensor_mdnie_3_size);

					mdnie_data.DSI0_TRANS_DIMMING_MDNIE[mdnie_data.dsi0_trans_dimming_data_index] = 0x0;

					tune_data_dsi0 = mdnie_data.DSI0_RGB_SENSOR_MDNIE;
				}
			} else {
				if (tune_data_dsi1) {
					memcpy(mdnie_data.DSI1_RGB_SENSOR_MDNIE_1, tune_data_dsi1[mdnie_data.mdnie_step_index[MDNIE_STEP1]].payload, mdnie_data.dsi1_rgb_sensor_mdnie_1_size);
					memcpy(mdnie_data.DSI1_RGB_SENSOR_MDNIE_2, tune_data_dsi1[mdnie_data.mdnie_step_index[MDNIE_STEP2]].payload, mdnie_data.dsi1_rgb_sensor_mdnie_2_size);
					memcpy(mdnie_data.DSI1_RGB_SENSOR_MDNIE_3, tune_data_dsi1[mdnie_data.mdnie_step_index[MDNIE_STEP3]].payload, mdnie_data.dsi1_rgb_sensor_mdnie_3_size);

					mdnie_data.DSI1_TRANS_DIMMING_MDNIE[mdnie_data.dsi1_trans_dimming_data_index] = 0x0;

					tune_data_dsi1 = mdnie_data.DSI1_RGB_SENSOR_MDNIE;
				}
			}
		}

		if (!tune_data_dsi0 && (mdnie_tune_state->index == DSI_CTRL_0)) {
			DPRINT("%s index : %d tune_data is NULL hbm : %d mdnie_bypass : %d mdnie_accessibility : %d  mdnie_app: %d mdnie_mode : %d hdr : %d night_mode_enable : %d\n",
				__func__, mdnie_tune_state->index, mdnie_tune_state->hbm_enable, mdnie_tune_state->mdnie_bypass, mdnie_tune_state->mdnie_accessibility,
				mdnie_tune_state->mdnie_app, mdnie_tune_state->mdnie_mode, mdnie_tune_state->hdr, mdnie_tune_state->night_mode_enable);
			return -EFAULT;
		} else if (!tune_data_dsi1 && (mdnie_tune_state->index == DSI_CTRL_1)) {
			DPRINT("%s index : %d tune_data is NULL hbm : %d mdnie_bypass : %d mdnie_accessibility : %d  mdnie_app: %d mdnie_mode : %d hdr : %d night_mode_enable : %d\n",
				__func__, mdnie_tune_state->index, mdnie_tune_state->hbm_enable, mdnie_tune_state->mdnie_bypass, mdnie_tune_state->mdnie_accessibility,
				mdnie_tune_state->mdnie_app, mdnie_tune_state->mdnie_mode, mdnie_tune_state->hdr, mdnie_tune_state->night_mode_enable);
			return -EFAULT;
		} else if (likely(tune_data_dsi0)) {
			mdnie_tune_state->scr_white_red = tune_data_dsi0[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]];
			mdnie_tune_state->scr_white_green = tune_data_dsi0[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]];
			mdnie_tune_state->scr_white_blue = tune_data_dsi0[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]];
		} else if (likely(tune_data_dsi1)) {
			mdnie_tune_state->scr_white_red = tune_data_dsi1[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]];
			mdnie_tune_state->scr_white_green = tune_data_dsi1[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]];
			mdnie_tune_state->scr_white_blue = tune_data_dsi1[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]];
		}
	}

	send_dsi_tcon_mdnie_register(vdd, tune_data_dsi0, tune_data_dsi1, real_mdnie_tune_state);

	return 0;
}

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current Mode : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "DSI%d : %s ", mdnie_tune_state->index, mdnie_mode_name[mdnie_tune_state->mdnie_mode]);
	}
	buffer_pos += snprintf(buf + buffer_pos, 256, "\n");

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d", &value);

	if (value < DYNAMIC_MODE || value >= MAX_MODE) {
		DPRINT("[ERROR] wrong mode value : %d\n",
			value);
		return size;
	}

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;
		if (vdd->dtsi_data[0].tft_common_support && value >= NATURAL_MODE)
			value++;

		mdnie_tune_state->mdnie_mode = value;

		DPRINT("%s mode : %d\n", __func__, mdnie_tune_state->mdnie_mode);
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t scenario_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current APP : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "DSI%d : %s ", mdnie_tune_state->index, mdnie_app_name[mdnie_tune_state->mdnie_app]);
	}
	buffer_pos += snprintf(buf + buffer_pos, 256, "\n");

	DPRINT("%s \n", buf);

	return buffer_pos;
}

/* app_id : App give self_app_id to mdnie driver.
* ret_id : app_id for mdnie data structure.
* example. TDMB app tell mdnie-driver that my app_id is 20. but mdnie driver will change it to TDMB_APP value.
*/
static int fake_id(int app_id)
{
	int ret_id;

	switch (app_id) {
#ifdef CONFIG_TDMB
	case APP_ID_TDMB:
		ret_id = TDMB_APP;
		DPRINT("%s : change app_id(%d) to mdnie_app(%d)\n", __func__, app_id, ret_id);
		break;
#endif
	default:
		ret_id = app_id;
		break;
	}

	return ret_id;
}


static ssize_t scenario_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d", &value);
	value = fake_id(value);

	if (value < UI_APP || value >= MAX_APP_MODE) {
		DPRINT("[ERROR] wrong Scenario mode value : %d\n",
			value);
		return size;
	}

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		mdnie_tune_state->mdnie_app = value;
		DPRINT("%s APP : %d\n", __func__, mdnie_tune_state->mdnie_app);
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t outdoor_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current outdoor Mode : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "DSI%d : %s ", mdnie_tune_state->index, outdoor_name[mdnie_tune_state->outdoor]);
	}
	buffer_pos += snprintf(buf + buffer_pos, 256, "\n");

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t outdoor_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t size)
{
	int value;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d", &value);

	if (value < OUTDOOR_OFF_MODE || value >= MAX_OUTDOOR_MODE) {
		DPRINT("[ERROR] : wrong outdoor mode value : %d\n", value);
		return size;
	}

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		mdnie_tune_state->outdoor = value;

		DPRINT("outdoor value = %d, APP = %d\n", value, mdnie_tune_state->mdnie_app);
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t bypass_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current MDNIE bypass : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "DSI%d : %s ", mdnie_tune_state->index, mdnie_tune_state->mdnie_bypass ? "ENABLE" : "DISABLE");
	}
	buffer_pos += snprintf(buf + buffer_pos, 256, "\n");

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t bypass_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	int value;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d", &value);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		if (value)
			mdnie_tune_state->mdnie_bypass = BYPASS_ENABLE;
		else
			mdnie_tune_state->mdnie_bypass = BYPASS_DISABLE;

		DPRINT("%s bypass : %s value : %d\n", __func__, mdnie_tune_state->mdnie_bypass ? "ENABLE" : "DISABLE", value);
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t accessibility_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current accessibility : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "DSI%d : %s ", mdnie_tune_state->index,
		mdnie_tune_state->mdnie_accessibility ?
		mdnie_tune_state->mdnie_accessibility == 1 ? "NEGATIVE" :
		mdnie_tune_state->mdnie_accessibility == 2 ? "COLOR_BLIND" :
		mdnie_tune_state->mdnie_accessibility == 3 ? "CURTAIN" :
		mdnie_tune_state->mdnie_accessibility == 4 ? "GRAYSCALE" :
		mdnie_tune_state->mdnie_accessibility == 5 ? "GRAYSCALE_NEGATIVE" : "COLOR_BLIND_HBM" :
		"ACCESSIBILITY_OFF");
	}
	buffer_pos += snprintf(buf + buffer_pos, 256, "\n");

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t accessibility_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	int cmd_value;
	char buffer[MDNIE_COLOR_BLINDE_HBM_CMD_SIZE] = {0,};
	int buffer2[MDNIE_COLOR_BLINDE_HBM_CMD_SIZE/2] = {0,};
	int loop;
	char temp;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d %x %x %x %x %x %x %x %x %x %x %x %x", &cmd_value,
		&buffer2[0], &buffer2[1], &buffer2[2], &buffer2[3], &buffer2[4],
		&buffer2[5], &buffer2[6], &buffer2[7], &buffer2[8], &buffer2[9],
		&buffer2[10], &buffer2[11]);

	for (loop = 0; loop < MDNIE_COLOR_BLINDE_HBM_CMD_SIZE/2; loop++) {
		buffer2[loop] = buffer2[loop] & 0xFFFF;
		buffer[loop * 2] = (buffer2[loop] & 0xFF00) >> 8;
		buffer[loop * 2 + 1] = buffer2[loop] & 0xFF;
	}

	for (loop = 0; loop < MDNIE_COLOR_BLINDE_HBM_CMD_SIZE; loop += 2) {
		temp = buffer[loop];
		buffer[loop] = buffer[loop + 1];
		buffer[loop + 1] = temp;
	}

	/*
	* mDnie priority
	* Accessibility > HBM > Screen Mode
	*/
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		if (cmd_value == NEGATIVE) {
			mdnie_tune_state->mdnie_accessibility = NEGATIVE;
		} else if (cmd_value == COLOR_BLIND) {
			mdnie_tune_state->mdnie_accessibility = COLOR_BLIND;

			if (!IS_ERR_OR_NULL(mdnie_data.DSI0_COLOR_BLIND_MDNIE_SCR))
				memcpy(&mdnie_data.DSI0_COLOR_BLIND_MDNIE_SCR[mdnie_data.mdnie_color_blinde_cmd_offset],
						buffer, MDNIE_COLOR_BLINDE_CMD_SIZE);
			if (!IS_ERR_OR_NULL(mdnie_data.DSI1_COLOR_BLIND_MDNIE_SCR))
				memcpy(&mdnie_data.DSI1_COLOR_BLIND_MDNIE_SCR[mdnie_data.mdnie_color_blinde_cmd_offset],
						buffer, MDNIE_COLOR_BLINDE_CMD_SIZE);
		} else if (cmd_value == COLOR_BLIND_HBM) {
			mdnie_tune_state->mdnie_accessibility = COLOR_BLIND_HBM;

			if (!IS_ERR_OR_NULL(mdnie_data.DSI0_COLOR_BLIND_MDNIE_SCR))
				memcpy(&mdnie_data.DSI0_COLOR_BLIND_MDNIE_SCR[mdnie_data.mdnie_color_blinde_cmd_offset],
						buffer, MDNIE_COLOR_BLINDE_HBM_CMD_SIZE);
			if (!IS_ERR_OR_NULL(mdnie_data.DSI1_COLOR_BLIND_MDNIE_SCR))
				memcpy(&mdnie_data.DSI1_COLOR_BLIND_MDNIE_SCR[mdnie_data.mdnie_color_blinde_cmd_offset],
						buffer, MDNIE_COLOR_BLINDE_HBM_CMD_SIZE);

		} else if (cmd_value == CURTAIN) {
			mdnie_tune_state->mdnie_accessibility = CURTAIN;
		} else if (cmd_value == GRAYSCALE) {
			mdnie_tune_state->mdnie_accessibility = GRAYSCALE;
		} else if (cmd_value == GRAYSCALE_NEGATIVE) {
			mdnie_tune_state->mdnie_accessibility = GRAYSCALE_NEGATIVE;
		} else if (cmd_value == ACCESSIBILITY_OFF) {
			mdnie_tune_state->mdnie_accessibility = ACCESSIBILITY_OFF;
		}  else
			DPRINT("%s ACCESSIBILITY_MAX", __func__);
	}

#if defined(CONFIG_64BIT)
	DPRINT("%s cmd_value : %d size : %lu", __func__, cmd_value, size);
#else
	DPRINT("%s cmd_value : %d size : %u", __func__, cmd_value, size);
#endif

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t sensorRGB_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf, 256, "%d %d %d ", mdnie_tune_state->scr_white_red, mdnie_tune_state->scr_white_green, mdnie_tune_state->scr_white_blue);
	}
	return buffer_pos;
}

static ssize_t sensorRGB_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int white_red, white_green, white_blue;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct mdnie_lite_tun_type *real_mdnie_tune_state = NULL;
	struct dsi_cmd_desc *data_dsi0 = NULL;
	struct dsi_cmd_desc *tune_data_dsi0 = NULL;
	struct dsi_cmd_desc *data_dsi1 = NULL;
	struct dsi_cmd_desc *tune_data_dsi1 = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d %d %d", &white_red, &white_green, &white_blue);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		real_mdnie_tune_state = mdnie_tune_state;

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		if (vdd->support_mdnie_lite){
			if(mdnie_tune_state->ldu_mode_index == 0) {
				if ((mdnie_tune_state->mdnie_accessibility == ACCESSIBILITY_OFF) && (mdnie_tune_state->mdnie_mode == AUTO_MODE) &&
					((mdnie_tune_state->mdnie_app == BROWSER_APP) || (mdnie_tune_state->mdnie_app == eBOOK_APP))) {

					mdnie_tune_state->scr_white_red = (char)white_red;
					mdnie_tune_state->scr_white_green = (char)white_green;
					mdnie_tune_state->scr_white_blue = (char)white_blue;

					DPRINT("%s: white_red = %d, white_green = %d, white_blue = %d %u %u %u %u\n", __func__, white_red, white_green, white_blue,
						mdnie_data.dsi0_rgb_sensor_mdnie_1_size,
						mdnie_data.dsi0_rgb_sensor_mdnie_2_size,
						mdnie_data.dsi1_rgb_sensor_mdnie_1_size,
						mdnie_data.dsi1_rgb_sensor_mdnie_2_size);

					if (mdnie_tune_state->index == DSI_CTRL_0) {
						tune_data_dsi0 = mdnie_data.DSI0_RGB_SENSOR_MDNIE;

						data_dsi0 = mdnie_data.mdnie_tune_value_dsi0[mdnie_tune_state->mdnie_app][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];

						if (data_dsi0) {
							memcpy(mdnie_data.DSI0_RGB_SENSOR_MDNIE_1, data_dsi0[mdnie_data.mdnie_step_index[MDNIE_STEP1]].payload, mdnie_data.dsi0_rgb_sensor_mdnie_1_size);
							memcpy(mdnie_data.DSI0_RGB_SENSOR_MDNIE_2, data_dsi0[mdnie_data.mdnie_step_index[MDNIE_STEP2]].payload, mdnie_data.dsi0_rgb_sensor_mdnie_2_size);
							memcpy(mdnie_data.DSI0_RGB_SENSOR_MDNIE_3, data_dsi0[mdnie_data.mdnie_step_index[MDNIE_STEP3]].payload, mdnie_data.dsi0_rgb_sensor_mdnie_3_size);

							mdnie_data.DSI0_RGB_SENSOR_MDNIE_SCR[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = white_red;
							mdnie_data.DSI0_RGB_SENSOR_MDNIE_SCR[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = white_green;
							mdnie_data.DSI0_RGB_SENSOR_MDNIE_SCR[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = white_blue;
						}
					} else {
						tune_data_dsi1 = mdnie_data.DSI1_RGB_SENSOR_MDNIE;

						data_dsi1 = mdnie_data.mdnie_tune_value_dsi1[mdnie_tune_state->mdnie_app][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];

						if (data_dsi1) {
							memcpy(mdnie_data.DSI1_RGB_SENSOR_MDNIE_1, data_dsi1[mdnie_data.mdnie_step_index[MDNIE_STEP1]].payload, mdnie_data.dsi1_rgb_sensor_mdnie_1_size);
							memcpy(mdnie_data.DSI1_RGB_SENSOR_MDNIE_2, data_dsi1[mdnie_data.mdnie_step_index[MDNIE_STEP2]].payload, mdnie_data.dsi1_rgb_sensor_mdnie_2_size);
							memcpy(mdnie_data.DSI1_RGB_SENSOR_MDNIE_3, data_dsi1[mdnie_data.mdnie_step_index[MDNIE_STEP3]].payload, mdnie_data.dsi1_rgb_sensor_mdnie_3_size);

							mdnie_data.DSI1_RGB_SENSOR_MDNIE_SCR[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = white_red;
							mdnie_data.DSI1_RGB_SENSOR_MDNIE_SCR[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = white_green;
							mdnie_data.DSI1_RGB_SENSOR_MDNIE_SCR[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = white_blue;
						}
					}
				}
			}
		}
	}

	send_dsi_tcon_mdnie_register(vdd, tune_data_dsi0, tune_data_dsi1, real_mdnie_tune_state);

	return size;
}

static ssize_t whiteRGB_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current whiteRGB SETTING : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "DSI%d : %d %d %d ", mdnie_tune_state->index, mdnie_tune_state->scr_white_balanced_red,
			mdnie_tune_state->scr_white_balanced_green, mdnie_tune_state->scr_white_balanced_blue);
	}
	buffer_pos += snprintf(buf + buffer_pos, 256, "\n");

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t whiteRGB_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int i;
	int white_red, white_green, white_blue;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct mdnie_lite_tun_type *real_mdnie_tune_state = NULL;
	struct dsi_cmd_desc *white_tunning_data = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d %d %d", &white_red, &white_green, &white_blue);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		real_mdnie_tune_state = mdnie_tune_state;

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		DPRINT("%s: white_red = %d, white_green = %d, white_blue = %d\n", __func__, white_red, white_green, white_blue);

		if (vdd->support_mdnie_lite){
		if (mdnie_tune_state->mdnie_mode == AUTO_MODE) {
			if (mdnie_tune_state->index == DSI_CTRL_0) {
				if ((white_red <= 0 && white_red >= -40) && (white_green <= 0 && white_green >= -40) && (white_blue <= 0 && white_blue >= -40)) {
						if(mdnie_tune_state->ldu_mode_index == 0) {
							mdnie_data.dsi0_white_ldu_r = mdnie_data.dsi0_white_default_r;
							mdnie_data.dsi0_white_ldu_g = mdnie_data.dsi0_white_default_g;
							mdnie_data.dsi0_white_ldu_b = mdnie_data.dsi0_white_default_b;
						}
					for (i = 0; i < MAX_APP_MODE; i++) {
						if ((mdnie_data.mdnie_tune_value_dsi0[i][AUTO_MODE][0] != NULL) && (i != eBOOK_APP)) {
							white_tunning_data = mdnie_data.mdnie_tune_value_dsi0[i][AUTO_MODE][0];
							white_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = (char)(mdnie_data.dsi0_white_ldu_r + white_red);
							white_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = (char)(mdnie_data.dsi0_white_ldu_g + white_green);
							white_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = (char)(mdnie_data.dsi0_white_ldu_b + white_blue);
							mdnie_tune_state->scr_white_balanced_red = white_red;
							mdnie_tune_state->scr_white_balanced_green = white_green;
							mdnie_tune_state->scr_white_balanced_blue = white_blue;
						}
					}
					mdnie_data.dsi0_white_rgb_enabled = 1;
				}
			} else {
				if ((white_red <= 0 && white_red >= -40) && (white_green <= 0 && white_green >= -40) && (white_blue <= 0 && white_blue >= -40)) {
						if(mdnie_tune_state->ldu_mode_index == 0) {
							mdnie_data.dsi0_white_ldu_r = mdnie_data.dsi1_white_default_r;
							mdnie_data.dsi0_white_ldu_g = mdnie_data.dsi1_white_default_g;
							mdnie_data.dsi0_white_ldu_b = mdnie_data.dsi1_white_default_b;
						}
					for (i = 0; i < MAX_APP_MODE; i++) {
						if ((mdnie_data.mdnie_tune_value_dsi1[i][AUTO_MODE][0] != NULL) && (i != eBOOK_APP)) {
							white_tunning_data = mdnie_data.mdnie_tune_value_dsi1[i][AUTO_MODE][0];
							white_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = (char)(mdnie_data.dsi1_white_ldu_r + white_red);
							white_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = (char)(mdnie_data.dsi1_white_ldu_g + white_green);
							white_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = (char)(mdnie_data.dsi1_white_ldu_b + white_blue);
							mdnie_tune_state->scr_white_balanced_red = white_red;
							mdnie_tune_state->scr_white_balanced_green = white_green;
							mdnie_tune_state->scr_white_balanced_blue = white_blue;
						}
					}
					mdnie_data.dsi1_white_rgb_enabled = 1;
				}
			}
		}
	}
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t mdnie_ldu_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf, 256, "%d %d %d ", mdnie_tune_state->scr_white_red, mdnie_tune_state->scr_white_green, mdnie_tune_state->scr_white_blue);
	}
	return buffer_pos;
}

static ssize_t mdnie_ldu_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int i, j, idx;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct mdnie_lite_tun_type *real_mdnie_tune_state = NULL;
	struct dsi_cmd_desc *ldu_tunning_data = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d", &idx);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		real_mdnie_tune_state = mdnie_tune_state;

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		if (mdnie_tune_state->index == DSI_CTRL_0) {
			if ((idx >= 0) && (idx < mdnie_data.dsi0_max_adjust_ldu)) {
				mdnie_tune_state->ldu_mode_index = idx;
				for (i = 0; i < MAX_APP_MODE; i++) {
					for (j = 0; j < MAX_MODE; j++) {
						if ((mdnie_data.mdnie_tune_value_dsi0[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
							ldu_tunning_data = mdnie_data.mdnie_tune_value_dsi0[i][j][0];
							if(j == AUTO_MODE) {
								ldu_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] 
									= mdnie_data.dsi0_adjust_ldu_table[j][idx * 3 + 0] + mdnie_tune_state->scr_white_balanced_red;
								ldu_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] 
									= mdnie_data.dsi0_adjust_ldu_table[j][idx * 3 + 1] + mdnie_tune_state->scr_white_balanced_green;
								ldu_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] 
									= mdnie_data.dsi0_adjust_ldu_table[j][idx * 3 + 2] + mdnie_tune_state->scr_white_balanced_blue;
								mdnie_data.dsi0_white_ldu_r = mdnie_data.dsi0_adjust_ldu_table[j][idx * 3 + 0];
								mdnie_data.dsi0_white_ldu_g = mdnie_data.dsi0_adjust_ldu_table[j][idx * 3 + 1];
								mdnie_data.dsi0_white_ldu_b = mdnie_data.dsi0_adjust_ldu_table[j][idx * 3 + 2];
							}
							else {
								ldu_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = mdnie_data.dsi0_adjust_ldu_table[j][idx * 3 + 0];
								ldu_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = mdnie_data.dsi0_adjust_ldu_table[j][idx * 3 + 1];
								ldu_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = mdnie_data.dsi0_adjust_ldu_table[j][idx * 3 + 2];
							
							}
						}
					}
				}
			}
		} else {
			if ((idx >= 0) && (idx < mdnie_data.dsi1_max_adjust_ldu)) {
				mdnie_tune_state->ldu_mode_index = idx;
				for (i = 0; i < MAX_APP_MODE; i++) {
					for (j = 0; j < MAX_MODE; j++) {
						if ((mdnie_data.mdnie_tune_value_dsi1[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
							ldu_tunning_data = mdnie_data.mdnie_tune_value_dsi1[i][j][0];
							if(j == AUTO_MODE) {
								ldu_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] 
									= mdnie_data.dsi1_adjust_ldu_table[j][idx * 3 + 0] + mdnie_tune_state->scr_white_balanced_red;
								ldu_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] 
									= mdnie_data.dsi1_adjust_ldu_table[j][idx * 3 + 1] + mdnie_tune_state->scr_white_balanced_green;
								ldu_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] 
									= mdnie_data.dsi1_adjust_ldu_table[j][idx * 3 + 2] + mdnie_tune_state->scr_white_balanced_blue;
								mdnie_data.dsi1_white_ldu_r = mdnie_data.dsi1_adjust_ldu_table[j][idx * 3 + 0];
								mdnie_data.dsi1_white_ldu_g = mdnie_data.dsi1_adjust_ldu_table[j][idx * 3 + 1];
								mdnie_data.dsi1_white_ldu_b = mdnie_data.dsi1_adjust_ldu_table[j][idx * 3 + 2];
							}
							else {
								ldu_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = mdnie_data.dsi1_adjust_ldu_table[j][idx * 3 + 0];
								ldu_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = mdnie_data.dsi1_adjust_ldu_table[j][idx * 3 + 1];
								ldu_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = mdnie_data.dsi1_adjust_ldu_table[j][idx * 3 + 2];
							
							}
						}
					}
				}
			}
		}
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t night_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf, 256, "%d %d", mdnie_tune_state->night_mode_enable, mdnie_tune_state->night_mode_index);
	}
	return buffer_pos;
}

static ssize_t night_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int enable, idx;
	char *buffer;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct mdnie_lite_tun_type *real_mdnie_tune_state = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d %d", &enable, &idx);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		real_mdnie_tune_state = mdnie_tune_state;

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		mdnie_tune_state->night_mode_enable = enable;

		DPRINT("%s: enable = %d, idx = %d\n", __func__, enable, idx);

		if (mdnie_tune_state->index == DSI_CTRL_0) {
			if (((idx >= 0) && (idx < mdnie_data.dsi0_max_night_mode_index)) && (enable == true)) {
				if (!IS_ERR_OR_NULL(mdnie_data.dsi0_night_mode_table)) {
					buffer = &mdnie_data.dsi0_night_mode_table[(MDNIE_SCR_CMD_SIZE * idx)];
					if (!IS_ERR_OR_NULL(mdnie_data.DSI0_NIGHT_MODE_MDNIE_SCR)) {
						memcpy(&mdnie_data.DSI0_NIGHT_MODE_MDNIE_SCR[mdnie_data.mdnie_color_blinde_cmd_offset],
							buffer, MDNIE_SCR_CMD_SIZE);
						mdnie_tune_state->night_mode_index = idx;
					}
				}
			}
		} else {
			if (((idx >= 0) && (idx < mdnie_data.dsi1_max_night_mode_index)) && (enable == true)) {
				if (!IS_ERR_OR_NULL(mdnie_data.dsi1_night_mode_table)) {
					buffer = &mdnie_data.dsi1_night_mode_table[(MDNIE_SCR_CMD_SIZE * idx)];
					if (!IS_ERR_OR_NULL(mdnie_data.DSI1_NIGHT_MODE_MDNIE_SCR)) {
						memcpy(&mdnie_data.DSI1_NIGHT_MODE_MDNIE_SCR[mdnie_data.mdnie_color_blinde_cmd_offset],
							buffer, MDNIE_SCR_CMD_SIZE);
						mdnie_tune_state->night_mode_index = idx;
					}
				}
			}
		}
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t color_lens_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf, 256, "%d %d %d", mdnie_tune_state->color_lens_enable, mdnie_tune_state->color_lens_color, mdnie_tune_state->color_lens_level);
	}
	return buffer_pos;
}

static ssize_t color_lens_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int enable, color, level;
	char *buffer;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct mdnie_lite_tun_type *real_mdnie_tune_state = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d %d %d", &enable, &color, &level);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		real_mdnie_tune_state = mdnie_tune_state;

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		mdnie_tune_state->color_lens_enable = enable;

		DPRINT("%s: enable = %d, color = %d, level = %d\n", __func__, enable, color, level);

		if (mdnie_tune_state->index == DSI_CTRL_0) {
			if ((enable == true) && ((color >= 0) && (color < COLOR_LENS_COLOR_MAX)) && ((level >= 0) && (level < COLOR_LENS_LEVEL_MAX))) {
				if (!IS_ERR_OR_NULL(mdnie_data.dsi0_color_lens_table)) {
					buffer = &mdnie_data.dsi0_color_lens_table[(color * MDNIE_SCR_CMD_SIZE * COLOR_LENS_LEVEL_MAX) + (MDNIE_SCR_CMD_SIZE * level)];
					if (!IS_ERR_OR_NULL(mdnie_data.DSI0_COLOR_LENS_MDNIE_SCR)) {
						memcpy(&mdnie_data.DSI0_COLOR_LENS_MDNIE_SCR[mdnie_data.mdnie_color_blinde_cmd_offset],
							buffer, MDNIE_SCR_CMD_SIZE);
						mdnie_tune_state->color_lens_color = color;
						mdnie_tune_state->color_lens_level = level;
					}
				}
			}
		} else {
			if ((enable == true) && ((color >= 0) && (color < COLOR_LENS_COLOR_MAX)) && ((level >= 0) && (level < COLOR_LENS_LEVEL_MAX))) {
				if (!IS_ERR_OR_NULL(mdnie_data.dsi1_color_lens_table)) {
					buffer = &mdnie_data.dsi1_color_lens_table[(color * MDNIE_SCR_CMD_SIZE * COLOR_LENS_LEVEL_MAX) + (MDNIE_SCR_CMD_SIZE * level)];
					if (!IS_ERR_OR_NULL(mdnie_data.DSI1_COLOR_LENS_MDNIE_SCR)) {
						memcpy(&mdnie_data.DSI1_COLOR_LENS_MDNIE_SCR[mdnie_data.mdnie_color_blinde_cmd_offset],
							buffer, MDNIE_SCR_CMD_SIZE);
						mdnie_tune_state->color_lens_color = color;
						mdnie_tune_state->color_lens_level = level;
					}
				}
			}
		}
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t hdr_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current HDR SETTING : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "DSI%d : %s ", mdnie_tune_state->index, mdnie_hdr_name[mdnie_tune_state->hdr]);
	}
	buffer_pos += snprintf(buf + buffer_pos, 256, "\n");

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t hdr_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;
	int backup;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d", &value);

	if (value < HDR_OFF || value >= HDR_MAX) {
		DPRINT("[ERROR] wrong hdr value : %d\n", value);
		return size;
	}

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		backup = mdnie_tune_state->hdr;
		mdnie_tune_state->hdr = value;

		DPRINT("%s : (%d) -> (%d)\n", __func__, backup, value);
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t light_notification_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current LIGHT NOTIFICATION SETTING : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "DSI%d : %s ", mdnie_tune_state->index, mdnie_light_notification_name[mdnie_tune_state->light_notification]);
	}
	buffer_pos += snprintf(buf + buffer_pos, 256, "\n");

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t light_notification_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;
	int backup;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d", &value);

	if (value < LIGHT_NOTIFICATION_OFF || value >= LIGHT_NOTIFICATION_MAX) {
		DPRINT("[ERROR] wrong light notification value : %d\n", value);
		return size;
	}

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		backup = mdnie_tune_state->light_notification;
		mdnie_tune_state->light_notification = value;

		DPRINT("%s : (%d) -> (%d)\n", __func__, backup, value);
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static ssize_t cabc_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current CABC bypass : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "DSI%d : %s ", mdnie_tune_state->index, mdnie_tune_state->cabc_bypass ? "ENABLE" : "DISABLE");
	}
	buffer_pos += snprintf(buf + buffer_pos, 256, "\n");

	DPRINT("%s\n", buf);

	return buffer_pos;
}

static ssize_t cabc_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	int value;

	sscanf(buf, "%d", &value);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		if (value)
			mdnie_tune_state->cabc_bypass = BYPASS_DISABLE;
		else
			mdnie_tune_state->cabc_bypass = BYPASS_ENABLE;

		DPRINT("%s bypass : %s value : %d\n", __func__, mdnie_tune_state->cabc_bypass ? "ENABLE" : "DISABLE", value);
	}

	config_cabc(value);

	return size;
}

static ssize_t hmt_color_temperature_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		DPRINT("Current color temperature : %d\n", mdnie_tune_state->hmt_color_temperature);
	}

	return snprintf(buf, 256, "Current color temperature : %d\n", mdnie_tune_state->hmt_color_temperature);
}

static ssize_t hmt_color_temperature_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;
	int backup;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	sscanf(buf, "%d", &value);

	if (value < HMT_COLOR_TEMP_OFF || value >= HMT_COLOR_TEMP_MAX) {
		DPRINT("[ERROR] wrong color temperature value : %d\n", value);
		return size;
	}

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		if (mdnie_tune_state->mdnie_accessibility == NEGATIVE) {
			DPRINT("already negative mode(%d), do not update color temperature(%d)\n",
				mdnie_tune_state->mdnie_accessibility, value);
			return size;
		}

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		backup = mdnie_tune_state->hmt_color_temperature;
		mdnie_tune_state->hmt_color_temperature = value;

		DPRINT("%s : (%d) -> (%d)\n", __func__, backup, value);
	}

	update_dsi_tcon_mdnie_register(vdd);

	return size;
}

static DEVICE_ATTR(mode, 0664, mode_show, mode_store);
static DEVICE_ATTR(scenario, 0664, scenario_show, scenario_store);
static DEVICE_ATTR(outdoor, 0664, outdoor_show, outdoor_store);
static DEVICE_ATTR(bypass, 0664, bypass_show, bypass_store);
static DEVICE_ATTR(accessibility, 0664, accessibility_show, accessibility_store);
static DEVICE_ATTR(sensorRGB, 0664, sensorRGB_show, sensorRGB_store);
static DEVICE_ATTR(whiteRGB, 0664, whiteRGB_show, whiteRGB_store);
static DEVICE_ATTR(mdnie_ldu, 0664, mdnie_ldu_show, mdnie_ldu_store);
static DEVICE_ATTR(night_mode, 0664, night_mode_show, night_mode_store);
static DEVICE_ATTR(color_lens, 0664, color_lens_show, color_lens_store);
static DEVICE_ATTR(hdr, 0664, hdr_show, hdr_store);
static DEVICE_ATTR(light_notification, 0664, light_notification_show, light_notification_store);
static DEVICE_ATTR(cabc, 0664, cabc_show, cabc_store);
static DEVICE_ATTR(hmt_color_temperature, 0664, hmt_color_temperature_show, hmt_color_temperature_store);

#ifdef CONFIG_DISPLAY_USE_INFO
static int dpui_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	struct dpui_info *dpui = data;
	char tbuf[MAX_DPUI_VAL_LEN];
	int size;

	if (dpui == NULL) {
		DPRINT("err: dpui is null\n");
		return 0;
	}

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list, used_list) {
		vdd = mdnie_tune_state->vdd;
		size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
				mdnie_tune_state->scr_white_balanced_red);
		set_dpui_field(DPUI_KEY_WOFS_R, tbuf, size);
		size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
				mdnie_tune_state->scr_white_balanced_green);
		set_dpui_field(DPUI_KEY_WOFS_G, tbuf, size);
		size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
				mdnie_tune_state->scr_white_balanced_blue);
		set_dpui_field(DPUI_KEY_WOFS_B, tbuf, size);
	}

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", vdd->mdnie_x[DSI_CTRL_0]);
	set_dpui_field(DPUI_KEY_WCRD_X, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", vdd->mdnie_y[DSI_CTRL_0]);
	set_dpui_field(DPUI_KEY_WCRD_Y, tbuf, size);

	return 0;
}

static int mdnie_register_dpui(struct mdnie_lite_tun_type *mdnie_tune_state)
{
	memset(&mdnie_tune_state->dpui_notif, 0,
			sizeof(mdnie_tune_state->dpui_notif));
	mdnie_tune_state->dpui_notif.notifier_call = dpui_notifier_callback;

	return dpui_logging_register(&mdnie_tune_state->dpui_notif,
			DPUI_TYPE_MDNIE);
}
#endif /* CONFIG_DISPLAY_USE_INFO */

void create_tcon_mdnie_node(void)

{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		DPRINT("%s vdd is error", __func__);
	};

	tune_mdnie_dev = device_create(mdnie_class, NULL, 0, NULL,  "mdnie");
	if (IS_ERR(tune_mdnie_dev))
		DPRINT("Failed to create device(mdnie)!\n");

	/* APP */
	if (device_create_file(tune_mdnie_dev, &dev_attr_scenario) < 0)
		DPRINT("Failed to create device file(%s)!\n", dev_attr_scenario.attr.name);

	/* MODE */
	if (device_create_file(tune_mdnie_dev, &dev_attr_mode) < 0)
		DPRINT("Failed to create device file(%s)!\n", dev_attr_mode.attr.name);

	/* OUTDOOR */
	if (device_create_file(tune_mdnie_dev, &dev_attr_outdoor) < 0)
		DPRINT("Failed to create device file(%s)!\n", dev_attr_outdoor.attr.name);

	/* MDNIE ON/OFF */
	if (device_create_file(tune_mdnie_dev, &dev_attr_bypass) < 0)
		DPRINT("Failed to create device file(%s)!\n", dev_attr_bypass.attr.name);
#if 0
	/* NEGATIVE */
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_negative) < 0)
		DPRINT("Failed to create device file(%s)!\n",
			dev_attr_negative.attr.name);
#endif
	/* COLOR BLIND */
	if (device_create_file(tune_mdnie_dev, &dev_attr_accessibility) < 0)
		DPRINT("Failed to create device file(%s)!=n", dev_attr_accessibility.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_sensorRGB) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_sensorRGB.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_whiteRGB) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_whiteRGB.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_mdnie_ldu) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_mdnie_ldu.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_night_mode) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_night_mode.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_color_lens) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_color_lens.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_hdr) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_hdr.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_light_notification) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_hdr.attr.name);

	/* hmt_color_temperature */
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_hmt_color_temperature) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_hmt_color_temperature.attr.name);

	/* CABC ON/OFF */
	if (vdd->support_cabc)
		if (device_create_file(tune_mdnie_dev, &dev_attr_cabc) < 0)
			DPRINT("Failed to create device file(%s)!\n", dev_attr_cabc.attr.name);
}

struct mdnie_lite_tun_type *init_dsi_tcon_mdnie_class(int index, struct samsung_display_driver_data *vdd_data)
{
	struct mdnie_lite_tun_type *mdnie_tune_state;

	if (mdnie_class == NULL) {
		mdnie_class = class_create(THIS_MODULE, "mdnie");

		if (IS_ERR(mdnie_class)) {
			DPRINT("Failed to create class(mdnie)!\n");
			goto out;
		} else
			create_tcon_mdnie_node();

		INIT_LIST_HEAD(&mdnie_list);
	}

	mdnie_tune_state =
		kzalloc(sizeof(struct mdnie_lite_tun_type), GFP_KERNEL);

	if (!mdnie_tune_state) {
		DPRINT("%s allocation fail", __func__);
		goto out;
	} else {
		mdnie_tune_state->vdd = vdd_data;

		mdnie_tune_state->index = index;
		mdnie_tune_state->mdnie_bypass = BYPASS_DISABLE;
		if (mdnie_tune_state->vdd->support_cabc)
			mdnie_tune_state->cabc_bypass = BYPASS_DISABLE;
		mdnie_tune_state->hbm_enable = false;

		mdnie_tune_state->mdnie_app = UI_APP;
		mdnie_tune_state->mdnie_mode = AUTO_MODE;
		mdnie_tune_state->outdoor = OUTDOOR_OFF_MODE;
		mdnie_tune_state->hdr = HDR_OFF;
		mdnie_tune_state->light_notification = LIGHT_NOTIFICATION_OFF;

		mdnie_tune_state->mdnie_accessibility = ACCESSIBILITY_OFF;

		mdnie_tune_state->scr_white_red = 0xff;
		mdnie_tune_state->scr_white_green = 0xff;
		mdnie_tune_state->scr_white_blue = 0xff;

		mdnie_tune_state->scr_white_balanced_red = 0;
		mdnie_tune_state->scr_white_balanced_green = 0;
		mdnie_tune_state->scr_white_balanced_blue = 0;

		mdnie_tune_state->night_mode_enable = false;
		mdnie_tune_state->night_mode_index = 0;

		mdnie_tune_state->ldu_mode_index = 0;

		mdnie_tune_state->color_lens_enable = false;
		mdnie_tune_state->color_lens_color = 0;
		mdnie_tune_state->color_lens_level = 0;

		INIT_LIST_HEAD(&mdnie_tune_state->used_list);

		list_add(&mdnie_tune_state->used_list, &mdnie_list);
	}

#ifdef CONFIG_DISPLAY_USE_INFO
	mdnie_register_dpui(mdnie_tune_state);
#endif

	/* Set default link_stats as DSI_HS_MODE for mdnie tune data */
	vdd_data->dtsi_data[index].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd_data->panel_revision].link_state = DSI_HS_MODE;

	return mdnie_tune_state;

out:
	return 0;
}

void coordinate_tunning_multi(int index, char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE], int mdnie_tune_index, int scr_wr_addr, int data_size)
{
	int i, j;
	struct dsi_cmd_desc *coordinate_tunning_data = NULL;

	if (index == DSI_CTRL_0) {
		if (mdnie_data.dsi0_white_rgb_enabled == 0) {
			for (i = 0; i < MAX_APP_MODE; i++) {
				for (j = 0; j < MAX_MODE; j++) {
					if ((mdnie_data.mdnie_tune_value_dsi0[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
						coordinate_tunning_data = mdnie_data.mdnie_tune_value_dsi0[i][j][0];
						coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][0];
						coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][2];
						coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][4];
					}
					if ((i == UI_APP) && (j == AUTO_MODE)) {
						mdnie_data.dsi0_white_default_r = coordinate_data_multi[j][mdnie_tune_index][0];
						mdnie_data.dsi0_white_default_g = coordinate_data_multi[j][mdnie_tune_index][2];
						mdnie_data.dsi0_white_default_b = coordinate_data_multi[j][mdnie_tune_index][4];
					}
				}
			}
		}
	} else {
		if (mdnie_data.dsi1_white_rgb_enabled == 0) {
			for (i = 0; i < MAX_APP_MODE; i++) {
				for (j = 0; j < MAX_MODE; j++) {
					if ((mdnie_data.mdnie_tune_value_dsi1[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
						coordinate_tunning_data = mdnie_data.mdnie_tune_value_dsi1[i][j][0];
						coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][0];
						coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][2];
						coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][4];
					}
					if ((i == UI_APP) && (j == AUTO_MODE)) {
						mdnie_data.dsi1_white_default_r = coordinate_data_multi[j][mdnie_tune_index][0];
						mdnie_data.dsi1_white_default_g = coordinate_data_multi[j][mdnie_tune_index][2];
						mdnie_data.dsi1_white_default_b = coordinate_data_multi[j][mdnie_tune_index][4];
					}
				}
			}
		}
	}
}

void coordinate_tunning_calculate(int index, int x, int y, char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE],  int *rgb_index, int scr_wr_addr, int data_size)
{
	int i, j;
	int r, g, b;
	int r_00, r_01, r_10, r_11;
	int g_00, g_01, g_10, g_11;
	int b_00, b_01, b_10, b_11;
	struct dsi_cmd_desc *coordinate_tunning_data = NULL;

	DPRINT("coordinate_tunning_calculate index_0 : %d, index_1 : %d, index_2 : %d, index_3 : %d, x : %d, y : %d\n", rgb_index[0], rgb_index[1], rgb_index[2], rgb_index[3], x, y);

	if (index == DSI_CTRL_0) {
		for (i = 0; i < MAX_APP_MODE; i++) {
			for (j = 0; j < MAX_MODE; j++) {
				if ((mdnie_data.mdnie_tune_value_dsi0[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
					coordinate_tunning_data = mdnie_data.mdnie_tune_value_dsi0[i][j][0];

					r_00 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[0]][0];
					r_01 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[1]][0];
					r_10 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[2]][0];
					r_11 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[3]][0];

					g_00 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[0]][2];
					g_01 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[1]][2];
					g_10 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[2]][2];
					g_11 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[3]][2];

					b_00 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[0]][4];
					b_01 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[1]][4];
					b_10 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[2]][4];
					b_11 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[3]][4];

					r = ((r_00 * (1024 - x) + r_10 * x) * (1024 - y) + (r_01 * (1024 - x) + r_11 * x) * y) + 524288;
					r = r >> 20;
					g = ((g_00 * (1024 - x) + g_10 * x) * (1024 - y) + (g_01 * (1024 - x) + g_11 * x) * y) + 524288;
					g = g >> 20;
					b = ((b_00 * (1024 - x) + b_10 * x) * (1024 - y) + (b_01 * (1024 - x) + b_11 * x) * y) + 524288;
					b = b >> 20;

					if (i == 0 && j == 4)
						DPRINT("coordinate_tunning_calculate_Adaptive r : %d, g : %d, b : %d\n", r, g, b);
					if (i == 0 && j == 2)
						DPRINT("coordinate_tunning_calculate_D65 r : %d, g : %d, b : %d\n", r, g, b);

					coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = (char)r;
					coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = (char)g;
					coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = (char)b;

					if ((i == UI_APP) && (j == AUTO_MODE)) {
						mdnie_data.dsi0_white_default_r = (char)r;
						mdnie_data.dsi0_white_default_g = (char)g;
						mdnie_data.dsi0_white_default_b = (char)b;
					}
				}
			}
		}
	} else {
		for (i = 0; i < MAX_APP_MODE; i++) {
			for (j = 0; j < MAX_MODE; j++) {
				if ((mdnie_data.mdnie_tune_value_dsi1[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
					coordinate_tunning_data = mdnie_data.mdnie_tune_value_dsi1[i][j][0];

					r_00 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[0]][0];
					r_01 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[1]][0];
					r_10 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[2]][0];
					r_11 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[3]][0];


					g_00 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[0]][2];
					g_01 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[1]][2];
					g_10 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[2]][2];
					g_11 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[3]][2];

					b_00 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[0]][4];
					b_01 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[1]][4];
					b_10 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[2]][4];
					b_11 = (int)(unsigned char)coordinate_data_multi[j][rgb_index[3]][4];

					r = ((r_00 * (1024 - x) + r_10 * x) * (1024 - y) + (r_01 * (1024 - x) + r_11 * x) * y) + 524288;
					r = r >> 20;
					g = ((g_00 * (1024 - x) + g_10 * x) * (1024 - y) + (g_01 * (1024 - x) + g_11 * x) * y) + 524288;
					g = g >> 20;
					b = ((b_00 * (1024 - x) + b_10 * x) * (1024 - y) + (b_01 * (1024 - x) + b_11 * x) * y) + 524288;
					b = b >> 20;

					coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = (char)r;
					coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = (char)g;
					coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = (char)b;

					if ((i == UI_APP) && (j == AUTO_MODE)) {
						mdnie_data.dsi1_white_default_r = (char)r;
						mdnie_data.dsi1_white_default_g = (char)g;
						mdnie_data.dsi1_white_default_b = (char)b;
					}
				}
			}
		}
	}
}

void coordinate_tunning(int index, char *coordinate_data, int scr_wr_addr, int data_size)
{
	int i, j;
	char white_r, white_g, white_b;
	struct dsi_cmd_desc *coordinate_tunning_data = NULL;

	if (index == DSI_CTRL_0) {
		if (mdnie_data.dsi0_white_rgb_enabled == 0) {
			for (i = 0; i < MAX_APP_MODE; i++) {
				for (j = 0; j < MAX_MODE; j++) {
					if (mdnie_data.mdnie_tune_value_dsi0[i][j][0] != NULL) {
						coordinate_tunning_data = mdnie_data.mdnie_tune_value_dsi0[i][j][0];
						white_r = coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]];
						white_g = coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]];
						white_b = coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]];
						if ((white_r == 0xff) && (white_g == 0xff) && (white_b == 0xff)) {
							coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = coordinate_data[0];
							coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = coordinate_data[2];
							coordinate_tunning_data[mdnie_data.dsi0_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = coordinate_data[4];
						}
						if ((i == UI_APP) && (j == AUTO_MODE)) {
							mdnie_data.dsi0_white_default_r = coordinate_data[0];
							mdnie_data.dsi0_white_default_g = coordinate_data[2];
							mdnie_data.dsi0_white_default_b = coordinate_data[4];
						}
					}
				}
			}
		}
	} else {
		if (mdnie_data.dsi1_white_rgb_enabled == 0) {
			for (i = 0; i < MAX_APP_MODE; i++) {
				for (j = 0; j < MAX_MODE; j++) {
					if (mdnie_data.mdnie_tune_value_dsi1[i][j][0] != NULL) {
						coordinate_tunning_data = mdnie_data.mdnie_tune_value_dsi1[i][j][0];
						white_r = coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]];
						white_g = coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]];
						white_b = coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]];
						if ((white_r == 0xff) && (white_g == 0xff) && (white_b == 0xff)) {
							coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = coordinate_data[0];
							coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = coordinate_data[2];
							coordinate_tunning_data[mdnie_data.dsi1_scr_step_index].payload[mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = coordinate_data[4];
						}
						if ((i == UI_APP) && (j == AUTO_MODE)) {
							mdnie_data.dsi1_white_default_r = coordinate_data[0];
							mdnie_data.dsi1_white_default_g = coordinate_data[2];
							mdnie_data.dsi1_white_default_b = coordinate_data[4];
						}
					}
				}
			}
		}
	}
}
