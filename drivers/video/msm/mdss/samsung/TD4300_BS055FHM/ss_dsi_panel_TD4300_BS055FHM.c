/*
 * =================================================================
 *
 *
 *	Description:  samsung display panel file
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
#include "ss_dsi_mdnie_TD4300_BS055FHM.h"
#include "ss_dsi_panel_TD4300_BS055FHM.h"

static int mdss_panel_on_pre(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

#if defined(CONFIG_SEC_INCELL)
	if (incell_data.tsp_enable)
		incell_data.tsp_enable(NULL);
#endif
	pr_info("%s %d\n", __func__, ctrl->ndx);

	mdss_panel_attach_set(ctrl, true);

	return true;
}

static int mdss_panel_on_post(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	pr_info("%s %d\n", __func__, ctrl->ndx);

	return true;
}


static int mdss_panel_off_pre(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}
	/* Disable PWM_EN */
	ssreg_enable_blic(false);

	pr_info("%s %d\n", __func__, ctrl->ndx);

	return true;
}

static int mdss_panel_off_post(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

#if defined(CONFIG_SEC_INCELL)
	if (incell_data.tsp_disable)
		incell_data.tsp_disable(NULL);
#endif
	pr_info("%s %d\n", __func__, ctrl->ndx);

	return true;
}

static void backlight_tft_late_on(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return;
	}

	if (mdss_panel_attached(ctrl->ndx))
		ssreg_enable_blic(true);
	else /* For PBA BOOTING */
		ssreg_enable_blic(false);
}

static int mdss_panel_revision(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	vdd->panel_revision = 0;

	return true;
}

static struct dsi_panel_cmds * mdss_brightness_tft_pwm(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	vdd->candela_level = get_candela_value(vdd, ctrl->ndx);

	vdd->dtsi_data[ctrl->ndx].tft_pwm_tx_cmds[vdd->panel_revision].cmds[0].payload[1] = vdd->candela_level;

	*level_key = PANEL_LEVE1_KEY;

	return &vdd->dtsi_data[ctrl->ndx].tft_pwm_tx_cmds[vdd->panel_revision];
}

static void mdss_panel_tft_outdoormode_update(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return;
	}
	pr_info("%s: tft panel autobrightness update\n", __func__);

}

static void mdnie_init(struct samsung_display_driver_data *vdd)
{
	mdnie_data = kzalloc(sizeof(struct mdnie_lite_tune_data), GFP_KERNEL);
	if (!mdnie_data) {
		LCD_ERR("fail to alloc mdnie_data..\n");
		return;
	}

	vdd->mdnie_tune_size1 = 47;
	vdd->mdnie_tune_size2 = 56;
	vdd->mdnie_tune_size3 = 2;

	if (mdnie_data) {
		/* Update mdnie command */
		mdnie_data[0].COLOR_BLIND_MDNIE_2 = NULL;
		mdnie_data[0].RGB_SENSOR_MDNIE_1 = NULL;
		mdnie_data[0].RGB_SENSOR_MDNIE_2 = NULL;
		mdnie_data[0].UI_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data[0].UI_STANDARD_MDNIE_2 = NULL;
		mdnie_data[0].UI_AUTO_MDNIE_2 = NULL;
		mdnie_data[0].VIDEO_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data[0].VIDEO_STANDARD_MDNIE_2 = NULL;
		mdnie_data[0].VIDEO_AUTO_MDNIE_2 = NULL;
		mdnie_data[0].CAMERA_MDNIE_2 = NULL;
		mdnie_data[0].CAMERA_AUTO_MDNIE_2 = NULL;
		mdnie_data[0].GALLERY_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data[0].GALLERY_STANDARD_MDNIE_2 = NULL;
		mdnie_data[0].GALLERY_AUTO_MDNIE_2 = NULL;
		mdnie_data[0].VT_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data[0].VT_STANDARD_MDNIE_2 = NULL;
		mdnie_data[0].VT_AUTO_MDNIE_2 = NULL;
		mdnie_data[0].BROWSER_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data[0].BROWSER_STANDARD_MDNIE_2 = NULL;
		mdnie_data[0].BROWSER_AUTO_MDNIE_2 = NULL;
		mdnie_data[0].EBOOK_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data[0].EBOOK_STANDARD_MDNIE_2 = NULL;
		mdnie_data[0].EBOOK_AUTO_MDNIE_2 = NULL;

		mdnie_data[0].BYPASS_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].NEGATIVE_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].GRAYSCALE_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].GRAYSCALE_NEGATIVE_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].COLOR_BLIND_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].HBM_CE_MDNIE = DSI0_HBM_CE_MDNIE;
		mdnie_data[0].RGB_SENSOR_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].CURTAIN = NULL;
		mdnie_data[0].UI_DYNAMIC_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].UI_STANDARD_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].UI_NATURAL_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].UI_MOVIE_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].UI_AUTO_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].VIDEO_OUTDOOR_MDNIE = NULL;
		mdnie_data[0].VIDEO_DYNAMIC_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data[0].VIDEO_STANDARD_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data[0].VIDEO_NATURAL_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data[0].VIDEO_MOVIE_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data[0].VIDEO_AUTO_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data[0].VIDEO_WARM_OUTDOOR_MDNIE = NULL;
		mdnie_data[0].VIDEO_WARM_MDNIE = NULL;
		mdnie_data[0].VIDEO_COLD_OUTDOOR_MDNIE = NULL;
		mdnie_data[0].VIDEO_COLD_MDNIE = NULL;
		mdnie_data[0].CAMERA_OUTDOOR_MDNIE = NULL;
		mdnie_data[0].CAMERA_MDNIE = DSI0_CAMERA_MDNIE;
		mdnie_data[0].CAMERA_AUTO_MDNIE = DSI0_CAMERA_MDNIE;
		mdnie_data[0].GALLERY_DYNAMIC_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].GALLERY_STANDARD_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].GALLERY_NATURAL_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].GALLERY_MOVIE_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].GALLERY_AUTO_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].VT_DYNAMIC_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].VT_STANDARD_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].VT_NATURAL_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].VT_MOVIE_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].VT_AUTO_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].BROWSER_DYNAMIC_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].BROWSER_STANDARD_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].BROWSER_NATURAL_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].BROWSER_MOVIE_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].BROWSER_AUTO_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].EBOOK_DYNAMIC_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EBOOK_STANDARD_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EBOOK_NATURAL_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EBOOK_MOVIE_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EBOOK_AUTO_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EMAIL_AUTO_MDNIE = DSI0_UI_MDNIE;
		mdnie_data[0].mdnie_tune_value = mdnie_tune_value_dsi0;
		mdnie_data[0].light_notification_tune_value = light_notification_tune_value;

		/* Update MDNIE data related with size, offset or index */
		mdnie_data[0].bypass_mdnie_size = ARRAY_SIZE(DSI0_UI_MDNIE);
		mdnie_data[0].mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
		mdnie_data[0].mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
		mdnie_data[0].mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
		mdnie_data[0].address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = 0;
		mdnie_data[0].address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = 0;
		mdnie_data[0].address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = 0;
		mdnie_data[0].rgb_sensor_mdnie_1_size = 0;
		mdnie_data[0].rgb_sensor_mdnie_2_size = 0;
		mdnie_data[0].white_default_r = 0xff;
		mdnie_data[0].white_default_g = 0xff;
		mdnie_data[0].white_default_b = 0xff;
		mdnie_data[0].white_balanced_r = 0;
		mdnie_data[0].white_balanced_g = 0;
		mdnie_data[0].white_balanced_b = 0;
	}
}

static void mdss_panel_init(struct samsung_display_driver_data *vdd)
{
	pr_info("%s : %s", __func__, vdd->panel_name);

	vdd->support_panel_max = TD4300_BS055FHM_SUPPORT_PANEL_COUNT;
	vdd->support_cabc = false;

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = mdss_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = mdss_panel_on_post;
	vdd->panel_func.samsung_panel_off_pre = mdss_panel_off_pre;
	vdd->panel_func.samsung_panel_off_post = mdss_panel_off_post;
	vdd->panel_func.samsung_backlight_late_on = backlight_tft_late_on;

	/* DDI RX */
	vdd->panel_func.samsung_panel_revision = mdss_panel_revision;
	vdd->panel_func.samsung_manufacture_date_read = NULL;
	vdd->panel_func.samsung_ddi_id_read = NULL;
	vdd->panel_func.samsung_hbm_read = NULL;
	vdd->panel_func.samsung_mdnie_read = NULL;
	vdd->panel_func.samsung_smart_dimming_init = NULL;

	/* Brightness */
	vdd->panel_func.samsung_brightness_tft_pwm_ldi = mdss_brightness_tft_pwm;
	vdd->panel_func.samsung_brightness_hbm_off = NULL;
	vdd->panel_func.samsung_brightness_aid = NULL;
	vdd->panel_func.samsung_brightness_acl_on = NULL;
	vdd->panel_func.samsung_brightness_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_acl_off = NULL;
	vdd->panel_func.samsung_brightness_elvss = NULL;
	vdd->panel_func.samsung_brightness_elvss_temperature1 = NULL;
	vdd->panel_func.samsung_brightness_elvss_temperature2 = NULL;
	vdd->panel_func.samsung_brightness_vint = NULL;
	vdd->panel_func.samsung_brightness_gamma = NULL;
	vdd->brightness[0].brightness_packet_tx_cmds_dsi.link_state = DSI_HS_MODE;
	vdd->mdss_panel_tft_outdoormode_update=mdss_panel_tft_outdoormode_update;

	/* MDNIE */
	vdd->panel_func.samsung_mdnie_init = mdnie_init;

	mdss_panel_attach_set(vdd->ctrl_dsi[DISPLAY_1], true);
}

static int __init samsung_panel_init(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	char panel_string[] = "ss_dsi_panel_TD4300_BS055FHM_FHD";


	vdd->panel_name = mdss_mdp_panel + 8;
	pr_info("%s : %s\n", __func__, vdd->panel_name);

	if (!strncmp(vdd->panel_name, panel_string, strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = mdss_panel_init;

	return 0;
}
early_initcall(samsung_panel_init);
