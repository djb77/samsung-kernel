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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
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

struct mdnie_lite_tune_data* mdnie_data;

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
	struct dsi_cmd_desc *tune_data_cmds, struct mdnie_lite_tun_type *mdnie_tune_state)
{
	int idx = 0;

	if (vdd == NULL || !vdd->support_mdnie_lite)
		return;

	if (IS_ERR_OR_NULL(mdnie_tune_state) || IS_ERR_OR_NULL(mdnie_data))
		return;

	idx = mdnie_tune_state->index;

	if (tune_data_cmds && mdnie_data[idx].bypass_mdnie_size)	{
		if (vdd->support_hall_ic) {
			if (vdd->hall_ic_status == idx) {
				vdd->mdnie_tune_data[idx].mdnie_tune_packet_tx_cmds_dsi.cmds = tune_data_cmds;
				vdd->mdnie_tune_data[idx].mdnie_tune_packet_tx_cmds_dsi.cmd_cnt = mdnie_data[idx].bypass_mdnie_size;

				DPRINT("DUAL index(hall_ic) : %d hbm : %d mdnie_bypass : %d mdnie_accessibility : %d  mdnie_app: %d mdnie_mode : %d hdr : %d night_mode_enable : %d\n",
					vdd->hall_ic_status, mdnie_tune_state->hbm_enable, mdnie_tune_state->mdnie_bypass, mdnie_tune_state->mdnie_accessibility,
					mdnie_tune_state->mdnie_app, mdnie_tune_state->mdnie_mode, mdnie_tune_state->hdr, mdnie_tune_state->night_mode_enable);

				mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_0], PANEL_MDNIE_TUNE);
			}
		} else {
			DPRINT("SINGLE index : %d hbm : %d mdnie_bypass : %d mdnie_accessibility : %d  mdnie_app: %d mdnie_mode : %d hdr : %d night_mode_enable : %d\n",
				mdnie_tune_state->index, mdnie_tune_state->hbm_enable, mdnie_tune_state->mdnie_bypass, mdnie_tune_state->mdnie_accessibility,
				mdnie_tune_state->mdnie_app, mdnie_tune_state->mdnie_mode, mdnie_tune_state->hdr, mdnie_tune_state->night_mode_enable);

			if (vdd->ctrl_dsi[DSI_CTRL_0]->cmd_sync_wait_broadcast) { /* Dual DSI */
				vdd->mdnie_tune_data[DISPLAY_2].mdnie_tune_packet_tx_cmds_dsi.cmds = tune_data_cmds;
				vdd->mdnie_tune_data[DISPLAY_2].mdnie_tune_packet_tx_cmds_dsi.cmd_cnt = mdnie_data[idx].bypass_mdnie_size;
				mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_1], PANEL_MDNIE_TUNE);
			} else { /* Single DSI */
				vdd->mdnie_tune_data[DISPLAY_1].mdnie_tune_packet_tx_cmds_dsi.cmds = tune_data_cmds;
				vdd->mdnie_tune_data[DISPLAY_1].mdnie_tune_packet_tx_cmds_dsi.cmd_cnt = mdnie_data[idx].bypass_mdnie_size;
				mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_0], PANEL_MDNIE_TUNE);
			}
		}
	} else {
		DPRINT("No tune data cmds.. (%d)\n", mdnie_data[idx].bypass_mdnie_size);
	}
}

int update_dsi_tcon_mdnie_register(struct samsung_display_driver_data *vdd)
{
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;
	struct mdnie_lite_tun_type *real_mdnie_tune_state = NULL;
	struct dsi_cmd_desc *tune_data_cmds = NULL;

	int idx = 0;

	if (vdd == NULL || !vdd->support_mdnie_lite)
		return 0;

	if (IS_ERR_OR_NULL(mdnie_data))
		return 0;

	if (vdd->mfd_dsi[DISPLAY_1]->panel_info->cont_splash_enabled ||
		vdd->mfd_dsi[DISPLAY_1]->panel_info->blank_state != MDSS_PANEL_BLANK_UNBLANK) {
		LCD_ERR("do not send mdnie data (%d) (%d)\n",
			vdd->mfd_dsi[DISPLAY_1]->panel_info->cont_splash_enabled,
			vdd->mfd_dsi[DISPLAY_1]->panel_info->blank_state);
		return 0;
	}

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		real_mdnie_tune_state = mdnie_tune_state;
		/*
		*	Checking HBM mode first.
		*/
		if (mdnie_tune_state->vdd->lux >= mdnie_tune_state->vdd->enter_hbm_lux)
			mdnie_tune_state->hbm_enable = true;
		else
			mdnie_tune_state->hbm_enable = false;

		idx = mdnie_tune_state->index;

		/*
		* mDnie priority
		* Accessibility > HBM > Screen Mode
		*/
		if (mdnie_tune_state->mdnie_bypass == BYPASS_ENABLE) {
			tune_data_cmds = mdnie_data[idx].BYPASS_MDNIE;
		} else if (mdnie_tune_state->light_notification) {
			tune_data_cmds  = mdnie_data[idx].light_notification_tune_value[mdnie_tune_state->light_notification];
		} else if (mdnie_tune_state->mdnie_accessibility == COLOR_BLIND) {
			tune_data_cmds  = mdnie_data[idx].COLOR_BLIND_MDNIE;
		} else if (mdnie_tune_state->mdnie_accessibility == NEGATIVE) {
			tune_data_cmds  = mdnie_data[idx].NEGATIVE_MDNIE;
		} else if (mdnie_tune_state->mdnie_accessibility == CURTAIN) {
			tune_data_cmds  = mdnie_data[idx].CURTAIN;
		} else if (mdnie_tune_state->mdnie_accessibility == GRAYSCALE) {
			tune_data_cmds  = mdnie_data[idx].GRAYSCALE_MDNIE;
		} else if (mdnie_tune_state->mdnie_accessibility == GRAYSCALE_NEGATIVE) {
			tune_data_cmds  = mdnie_data[idx].GRAYSCALE_NEGATIVE_MDNIE;
		} else if (mdnie_tune_state->hdr) {
			tune_data_cmds = mdnie_data[idx].hdr_tune_value[mdnie_tune_state->hdr];
		} else if (mdnie_tune_state->hmt_color_temperature) {
			tune_data_cmds = mdnie_data[idx].hmt_color_temperature_tune_value[mdnie_tune_state->hmt_color_temperature];
		} else if (mdnie_tune_state->night_mode_enable == true) {
			tune_data_cmds  = mdnie_data[idx].NIGHT_MODE_MDNIE;
		} else if (mdnie_tune_state->hbm_enable == true) {
			if (vdd->dtsi_data[idx].hbm_ce_text_mode_support &&
				((mdnie_tune_state->mdnie_app == BROWSER_APP) || (mdnie_tune_state->mdnie_app == eBOOK_APP)))
					tune_data_cmds  = mdnie_data[idx].HBM_CE_TEXT_MDNIE;
			else {
					tune_data_cmds  = mdnie_data[idx].HBM_CE_MDNIE;
			}
		} else if (mdnie_tune_state->mdnie_app == EMAIL_APP) {
			/*
				Some kind of panel doesn't suooprt EMAIL_APP mode, but SSRM module use same control logic.
				It means SSRM doesn't consider panel unique character.
				To support this issue eBOOK_APP used insted of EMAIL_APP under EMAIL_APP doesn't exist status..
			*/
			tune_data_cmds = mdnie_data[idx].mdnie_tune_value[mdnie_tune_state->mdnie_app][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];
			if (!tune_data_cmds)
				tune_data_cmds = mdnie_data[idx].mdnie_tune_value[eBOOK_APP][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];

		} else {
			if (!mdnie_data[idx].mdnie_tune_value)
				pr_err("mdnie_tune_value is null..\n");

			tune_data_cmds = mdnie_data[idx].mdnie_tune_value[mdnie_tune_state->mdnie_app][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];
		}

		if (vdd->support_mdnie_trans_dimming && vdd->mdnie_disable_trans_dimming) {
			if (tune_data_cmds) {
				memcpy(mdnie_data[idx].RGB_SENSOR_MDNIE_1, tune_data_cmds[mdnie_data[idx].mdnie_step_index[MDNIE_STEP1]].payload, mdnie_data[idx].rgb_sensor_mdnie_1_size);
				memcpy(mdnie_data[idx].RGB_SENSOR_MDNIE_2, tune_data_cmds[mdnie_data[idx].mdnie_step_index[MDNIE_STEP2]].payload, mdnie_data[idx].rgb_sensor_mdnie_2_size);
				memcpy(mdnie_data[idx].RGB_SENSOR_MDNIE_3, tune_data_cmds[mdnie_data[idx].mdnie_step_index[MDNIE_STEP3]].payload, mdnie_data[idx].rgb_sensor_mdnie_3_size);

				mdnie_data[idx].TRANS_DIMMING_MDNIE[mdnie_data[idx].trans_dimming_data_index] = 0x0;

				tune_data_cmds = mdnie_data[idx].RGB_SENSOR_MDNIE;
			}
		}

		if (likely(tune_data_cmds)) {
			mdnie_tune_state->scr_white_red = tune_data_cmds[mdnie_data[idx].scr_step_index].payload[mdnie_data[idx].address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]];
			mdnie_tune_state->scr_white_green = tune_data_cmds[mdnie_data[idx].scr_step_index].payload[mdnie_data[idx].address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]];
			mdnie_tune_state->scr_white_blue = tune_data_cmds[mdnie_data[idx].scr_step_index].payload[mdnie_data[idx].address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]];
		} else {
			DPRINT("%s index : %d tune_data is NULL hbm : %d mdnie_bypass : %d mdnie_accessibility : %d  mdnie_app: %d mdnie_mode : %d hdr : %d night_mode_enable : %d\n",
				__func__, mdnie_tune_state->index, mdnie_tune_state->hbm_enable, mdnie_tune_state->mdnie_bypass, mdnie_tune_state->mdnie_accessibility,
				mdnie_tune_state->mdnie_app, mdnie_tune_state->mdnie_mode, mdnie_tune_state->hdr, mdnie_tune_state->night_mode_enable);
			return -EFAULT;
		}

		send_dsi_tcon_mdnie_register(vdd, tune_data_cmds, real_mdnie_tune_state);
	}

	return 0;
}

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	buffer_pos += snprintf(buf, 256, "Current Mode : ");
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
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

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		mdnie_tune_state->mdnie_mode = value;

		DPRINT("%s[%d] mode : %d\n", __func__, mdnie_tune_state->index, mdnie_tune_state->mdnie_mode);
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
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "PANEL%d : %s ", mdnie_tune_state->index, mdnie_app_name[mdnie_tune_state->mdnie_app]);
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

	switch(app_id) {
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

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		mdnie_tune_state->mdnie_app = value;
		DPRINT("%s[%d] APP : %d\n", __func__, mdnie_tune_state->index, mdnie_tune_state->mdnie_app);
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
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "PANEL%d : %s ", mdnie_tune_state->index, outdoor_name[mdnie_tune_state->outdoor]);
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

	if (value < OUTDOOR_OFF_MODE || value >= MAX_OUTDOOR_MODE)
		DPRINT("[ERROR] : wrong outdoor mode value : %d\n", value);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		mdnie_tune_state->outdoor = value;

		DPRINT("outdoor[%d] value = %d, APP = %d\n", mdnie_tune_state->index, value, mdnie_tune_state->mdnie_app);
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
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "PANEL%d : %s ", mdnie_tune_state->index, mdnie_tune_state->mdnie_bypass ? "ENABLE" : "DISABLE");
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

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		if (value)
			mdnie_tune_state->mdnie_bypass = BYPASS_ENABLE;
		else
			mdnie_tune_state->mdnie_bypass = BYPASS_DISABLE;

		DPRINT("%s[%d] bypass : %s value : %d\n", __func__, mdnie_tune_state->index,
			mdnie_tune_state->mdnie_bypass ? "ENABLE" : "DISABLE", value);
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
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "PANEL%d : %s ", mdnie_tune_state->index,
		mdnie_tune_state->mdnie_accessibility ?
		mdnie_tune_state->mdnie_accessibility == 1 ? "NEGATIVE" :
		mdnie_tune_state->mdnie_accessibility == 2 ? "COLOR_BLIND" :
		mdnie_tune_state->mdnie_accessibility == 3 ? "CURTAIN" :
		mdnie_tune_state->mdnie_accessibility == 4 ? "GRAYSCALE" : "GRAYSCALE_NEGATIVE" :
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
	char buffer[MDNIE_COLOR_BLINDE_CMD_SIZE] = {0,};
	int buffer2[MDNIE_COLOR_BLINDE_CMD_SIZE/2] = {0,};
	int loop;
	char temp;
	struct samsung_display_driver_data *vdd = NULL;
	int idx = 0;

	if (IS_ERR_OR_NULL(mdnie_data))
		return size;

	sscanf(buf, "%d %x %x %x %x %x %x %x %x %x", &cmd_value,
		&buffer2[0], &buffer2[1], &buffer2[2], &buffer2[3], &buffer2[4],
		&buffer2[5], &buffer2[6], &buffer2[7], &buffer2[8]);

	for (loop = 0; loop < MDNIE_COLOR_BLINDE_CMD_SIZE/2; loop++) {
		buffer2[loop] = buffer2[loop] & 0xFFFF;
		buffer[loop * 2] = (buffer2[loop] & 0xFF00) >> 8;
		buffer[loop * 2 + 1] = buffer2[loop] & 0xFF;
	}

	for (loop = 0; loop < MDNIE_COLOR_BLINDE_CMD_SIZE; loop+=2) {
		temp = buffer[loop];
		buffer[loop] = buffer[loop + 1];
		buffer[loop + 1] = temp;
	}

	/*
	* mDnie priority
	* Accessibility > HBM > Screen Mode
	*/
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {

		idx = mdnie_tune_state->index;

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		if (vdd->mdnie_reverse_scr == true){
			for (loop = 0; loop < (MDNIE_COLOR_BLINDE_CMD_SIZE/2); loop++) {
				temp = buffer[loop];
				buffer[loop] = buffer[MDNIE_COLOR_BLINDE_CMD_SIZE  - loop - 1];
				buffer[MDNIE_COLOR_BLINDE_CMD_SIZE	- loop - 1] = temp;
			}

			for (loop = 0; loop < MDNIE_COLOR_BLINDE_CMD_SIZE; loop++) {
				DPRINT("%s buffer[loop] - - %d - %0x", __func__, loop, buffer[loop]);
			}
		}

		if (cmd_value == NEGATIVE) {
			mdnie_tune_state->mdnie_accessibility = NEGATIVE;
		} else if (cmd_value == COLOR_BLIND) {
			mdnie_tune_state->mdnie_accessibility = COLOR_BLIND;

			if (!IS_ERR_OR_NULL(mdnie_data[idx].COLOR_BLIND_MDNIE_SCR))
				memcpy(&mdnie_data[idx].COLOR_BLIND_MDNIE_SCR[mdnie_data[idx].mdnie_color_blinde_cmd_offset],
						buffer, MDNIE_COLOR_BLINDE_CMD_SIZE);

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

#if defined(CONFIG_64BIT)
		DPRINT("%s[%d] cmd_value : %d size : %lu", __func__, mdnie_tune_state->index, cmd_value, size);
#else
		DPRINT("%s[%d] cmd_value : %d size : %u", __func__, mdnie_tune_state->index, cmd_value, size);
#endif
	}

	update_dsi_tcon_mdnie_register(vdd);
	return size;
}

static ssize_t sensorRGB_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
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
	struct dsi_cmd_desc *rgb_sensor_cmds = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	int idx;

	if (IS_ERR_OR_NULL(mdnie_data))
		return size;

	sscanf(buf, "%d %d %d", &white_red, &white_green, &white_blue);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		real_mdnie_tune_state = mdnie_tune_state;

		idx = mdnie_tune_state->index;

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		if ((mdnie_tune_state->mdnie_accessibility == ACCESSIBILITY_OFF) && (mdnie_tune_state->mdnie_mode == AUTO_MODE) &&
			((mdnie_tune_state->mdnie_app == BROWSER_APP) || (mdnie_tune_state->mdnie_app == eBOOK_APP))) {

			mdnie_tune_state->scr_white_red = (char)white_red;
			mdnie_tune_state->scr_white_green = (char)white_green;
			mdnie_tune_state->scr_white_blue = (char)white_blue;

			DPRINT("%s[%d]: white_red = %d, white_green = %d, white_blue = %d %u %u \n", __func__, idx, white_red, white_green, white_blue,
				mdnie_data[idx].rgb_sensor_mdnie_1_size,
				mdnie_data[idx].rgb_sensor_mdnie_2_size);

			rgb_sensor_cmds = mdnie_data[idx].RGB_SENSOR_MDNIE;

			data_dsi0 = mdnie_data[idx].mdnie_tune_value[mdnie_tune_state->mdnie_app][mdnie_tune_state->mdnie_mode][mdnie_tune_state->outdoor];

			if (data_dsi0) {
				memcpy(mdnie_data[idx].RGB_SENSOR_MDNIE_1, data_dsi0[mdnie_data[idx].mdnie_step_index[MDNIE_STEP1]].payload, mdnie_data[idx].rgb_sensor_mdnie_1_size);
				memcpy(mdnie_data[idx].RGB_SENSOR_MDNIE_2, data_dsi0[mdnie_data[idx].mdnie_step_index[MDNIE_STEP2]].payload, mdnie_data[idx].rgb_sensor_mdnie_2_size);
				memcpy(mdnie_data[idx].RGB_SENSOR_MDNIE_3, data_dsi0[mdnie_data[idx].mdnie_step_index[MDNIE_STEP3]].payload, mdnie_data[idx].rgb_sensor_mdnie_3_size);

				mdnie_data[idx].RGB_SENSOR_MDNIE_SCR[mdnie_data[idx].address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = white_red;
				mdnie_data[idx].RGB_SENSOR_MDNIE_SCR[mdnie_data[idx].address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = white_green;
				mdnie_data[idx].RGB_SENSOR_MDNIE_SCR[mdnie_data[idx].address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = white_blue;
			}

			send_dsi_tcon_mdnie_register(vdd, rgb_sensor_cmds, real_mdnie_tune_state);

		}
	}

	return size;
}

static ssize_t mdnie_ldu_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int buffer_pos = 0;
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
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
	int midx = 0;

	if (IS_ERR_OR_NULL(mdnie_data))
		return 0;

	sscanf(buf, "%d", &idx);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		real_mdnie_tune_state = mdnie_tune_state;

		midx = mdnie_tune_state->index;

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		if ((idx >=0) && (idx < mdnie_data[midx].max_adjust_ldu)) {
			for (i = 0; i < MAX_APP_MODE; i++) {
				for (j = 0; j < MAX_MODE; j++) {
					if ((mdnie_data[midx].mdnie_tune_value[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
						ldu_tunning_data = mdnie_data[midx].mdnie_tune_value[i][j][0];
						ldu_tunning_data[mdnie_data[midx].scr_step_index].payload[mdnie_data[midx].address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = mdnie_data[idx].adjust_ldu_table[j][idx * 3 + 0];
						ldu_tunning_data[mdnie_data[midx].scr_step_index].payload[mdnie_data[midx].address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = mdnie_data[idx].adjust_ldu_table[j][idx * 3 + 1];
						ldu_tunning_data[mdnie_data[midx].scr_step_index].payload[mdnie_data[midx].address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = mdnie_data[idx].adjust_ldu_table[j][idx * 3 + 2];
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

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
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
	int midx = 0;

	if (IS_ERR_OR_NULL(mdnie_data))
		return size;

	sscanf(buf, "%d %d", &enable, &idx);

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		real_mdnie_tune_state = mdnie_tune_state;

		midx = mdnie_tune_state->index;

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		mdnie_tune_state->night_mode_enable = enable;

		DPRINT("%s[%d]: enable = %d, idx = %d\n", __func__, midx, enable, idx);

		if (((idx >=0) && (idx < mdnie_data[midx].max_night_mode_index)) && (enable == true)) {
			if (!IS_ERR_OR_NULL(mdnie_data[midx].night_mode_table)) {
				buffer = &mdnie_data[midx].night_mode_table[(MDNIE_NIGHT_MODE_CMD_SIZE * idx)];
				if (!IS_ERR_OR_NULL(mdnie_data[midx].NIGHT_MODE_MDNIE_1)) {
					memcpy(&mdnie_data[midx].NIGHT_MODE_MDNIE_1[mdnie_data[midx].mdnie_color_blinde_cmd_offset],
						buffer, MDNIE_NIGHT_MODE_CMD_SIZE);
					mdnie_tune_state->night_mode_index = idx;
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
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "PANEL%d : %s ", mdnie_tune_state->index, mdnie_hdr_name[mdnie_tune_state->hdr]);
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

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		backup = mdnie_tune_state->hdr;
		mdnie_tune_state->hdr = value;

		DPRINT("%s[%d] : (%d) -> (%d)\n", __func__, mdnie_tune_state->index, backup, value);
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
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "PANEL%d : %s ", mdnie_tune_state->index, mdnie_light_notification_name[mdnie_tune_state->light_notification]);
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

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
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
	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		buffer_pos += snprintf(buf + buffer_pos, 256, "PANEL%d : %s ", mdnie_tune_state->index, mdnie_tune_state->cabc_bypass ? "ENABLE" : "DISABLE");
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

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		if (value)
			mdnie_tune_state->cabc_bypass = BYPASS_DISABLE;
		else
			mdnie_tune_state->cabc_bypass = BYPASS_ENABLE;

		DPRINT("%s[%d] bypass : %s value : %d\n", __func__, mdnie_tune_state->index,
			mdnie_tune_state->cabc_bypass ? "ENABLE" : "DISABLE", value);
	}

	config_cabc(value);

	return size;
}

static ssize_t hmt_color_temperature_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct mdnie_lite_tun_type *mdnie_tune_state = NULL;

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		DPRINT("[%d] Current color temperature : %d\n", mdnie_tune_state->index, mdnie_tune_state->hmt_color_temperature);
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

	list_for_each_entry_reverse(mdnie_tune_state, &mdnie_list , used_list) {
		if (mdnie_tune_state->mdnie_accessibility == NEGATIVE) {
			DPRINT("already negative mode(%d), do not update color temperature(%d)\n",
				mdnie_tune_state->mdnie_accessibility, value);
			return size;
		}

		if (!vdd)
			vdd = mdnie_tune_state->vdd;

		backup = mdnie_tune_state->hmt_color_temperature;
		mdnie_tune_state->hmt_color_temperature = value;

		DPRINT("%s[%d] : (%d) -> (%d)\n", __func__, mdnie_tune_state->index, backup, value);
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
static DEVICE_ATTR(mdnie_ldu, 0664, mdnie_ldu_show, mdnie_ldu_store);
static DEVICE_ATTR(night_mode, 0664, night_mode_show, night_mode_store);
static DEVICE_ATTR(hdr, 0664, hdr_show, hdr_store);
static DEVICE_ATTR(light_notification, 0664, light_notification_show, light_notification_store);
static DEVICE_ATTR(cabc, 0664, cabc_show, cabc_store);
static DEVICE_ATTR(hmt_color_temperature, 0664, hmt_color_temperature_show, hmt_color_temperature_store);

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
		(tune_mdnie_dev, &dev_attr_mdnie_ldu) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_mdnie_ldu.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_night_mode) < 0)
		DPRINT("Failed to create device file(%s)!=n",
			dev_attr_night_mode.attr.name);

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

		mdnie_tune_state->night_mode_enable = false;
		mdnie_tune_state->night_mode_index = 0;

		INIT_LIST_HEAD(&mdnie_tune_state->used_list);

		list_add(&mdnie_tune_state->used_list, &mdnie_list);
	}

	/* Set default link_stats as DSI_HS_MODE for mdnie tune data */
	vdd_data->mdnie_tune_data[index].mdnie_tune_packet_tx_cmds_dsi.link_state = DSI_HS_MODE;

	DPRINT("%s : done.. (%d)\n", __func__, mdnie_tune_state->index);

	return mdnie_tune_state;

out:
	return 0;
}

void coordinate_tunning_multi(int index, char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE], int mdnie_tune_index, int scr_wr_addr, int data_size)
{
	int i, j;
	struct dsi_cmd_desc *coordinate_tunning_data = NULL;

	if (IS_ERR_OR_NULL(mdnie_data))
		return;

	for (i = 0; i < MAX_APP_MODE; i++) {
		for (j = 0; j < MAX_MODE; j++) {
			if ((mdnie_data[index].mdnie_tune_value[i][j][0] != NULL) && (i != eBOOK_APP) && (j != READING_MODE)) {
				coordinate_tunning_data = mdnie_data[index].mdnie_tune_value[i][j][0];
				coordinate_tunning_data[mdnie_data[index].scr_step_index].payload[mdnie_data[index].address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][0];
				coordinate_tunning_data[mdnie_data[index].scr_step_index].payload[mdnie_data[index].address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][2];
				coordinate_tunning_data[mdnie_data[index].scr_step_index].payload[mdnie_data[index].address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = coordinate_data_multi[j][mdnie_tune_index][4];
			}
		}
	}
}

void coordinate_tunning(int index, char *coordinate_data, int scr_wr_addr, int data_size)
{
	int i, j;
	char white_r, white_g, white_b;
	struct dsi_cmd_desc *coordinate_tunning_data = NULL;

	if (IS_ERR_OR_NULL(mdnie_data))
		return;

	for (i = 0; i < MAX_APP_MODE; i++) {
		for (j = 0; j < MAX_MODE; j++) {
			if (mdnie_data[index].mdnie_tune_value[i][j][0] != NULL) {
				coordinate_tunning_data = mdnie_data[index].mdnie_tune_value[i][j][0];
				white_r = coordinate_tunning_data[mdnie_data[index].scr_step_index].payload[mdnie_data[index].address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]];
				white_g = coordinate_tunning_data[mdnie_data[index].scr_step_index].payload[mdnie_data[index].address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]];
				white_b = coordinate_tunning_data[mdnie_data[index].scr_step_index].payload[mdnie_data[index].address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]];
				if ((white_r == 0xff) && (white_g == 0xff) && (white_b == 0xff)) {
					coordinate_tunning_data[mdnie_data[index].scr_step_index].payload[mdnie_data[index].address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET]] = coordinate_data[0];
					coordinate_tunning_data[mdnie_data[index].scr_step_index].payload[mdnie_data[index].address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET]] = coordinate_data[2];
					coordinate_tunning_data[mdnie_data[index].scr_step_index].payload[mdnie_data[index].address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET]] = coordinate_data[4];
				}
			}
		}
	}
}
