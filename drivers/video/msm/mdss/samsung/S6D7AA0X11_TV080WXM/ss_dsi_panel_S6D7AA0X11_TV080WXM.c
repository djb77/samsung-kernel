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
#if defined(CONFIG_SEC_GTA2SLTE_PROJECT) || defined(CONFIG_SEC_GTA2SWIFI_PROJECT)
#include "ss_dsi_mdnie_S6D7AA0X11_TV080WXM_gtab2s.h"
#else
#include "ss_dsi_mdnie_S6D7AA0X11_TV080WXM.h"
#endif
#include "ss_dsi_panel_S6D7AA0X11_TV080WXM.h"

static int mdss_panel_on_pre(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

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
	/* ssreg_enable_blic(false); */

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
	
	/* if (mdss_panel_attached(ctrl->ndx))		
		ssreg_enable_blic(true);
	else 
		ssreg_enable_blic(false);// For PBA BOOTING */
}


static char mdss_panel_revision(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int ndx = display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	if (vdd->manufacture_id_dsi[ndx] == PBA_ID)
		mdss_panel_attach_set(ctrl, false);
	else
		mdss_panel_attach_set(ctrl, true);

	vdd->aid_subdivision_enable = true;

	switch (mdss_panel_rev_get(ctrl)) {
	case 0x00:
		vdd->panel_revision = 'A';
		break;
	case 0x01:
		vdd->panel_revision = 'A';
		break;
	default:
		vdd->panel_revision = 'A';
		LCD_ERR("Invalid panel_rev(default rev : %c)\n",
				vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';

	LCD_INFO_ONCE("panel_revision = %c %d \n",
					vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision);
}

static struct dsi_panel_cmds * mdss_brightness_tft_pwm(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	get_panel_tx_cmds(ctrl, TX_TFT_PWM)->cmds[0].payload[1] = vdd->candela_level;

	*level_key = LEVEL1_KEY;

	return get_panel_tx_cmds(ctrl, TX_TFT_PWM);
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

static void dsi_update_mdnie_data(void)
{
	/* mdnie_data = kzalloc(sizeof(struct mdnie_lite_tune_data), GFP_KERNEL); 
	if (!mdnie_data) {
		LCD_ERR("fail to alloc mdnie_data..\n");
		return;
	}

	vdd->support_mdnie_trans_dimming = false;

	vdd->mdnie_tune_size1 = 17;
	vdd->mdnie_tune_size2 = 25;
	vdd->mdnie_tune_size3 = 25;
	vdd->mdnie_tune_size4 = 25;
	vdd->mdnie_tune_size5 = 19;
	vdd->mdnie_tune_size6 = 8; */


		/* Update mdnie command */
		mdnie_data.DSI0_COLOR_BLIND_MDNIE_2 = COLOR_BLIND_2;
		mdnie_data.DSI0_RGB_SENSOR_MDNIE_1 = RGB_SENSOR_1;
		mdnie_data.DSI0_RGB_SENSOR_MDNIE_2 = RGB_SENSOR_2;
		mdnie_data.DSI0_NIGHT_MODE_MDNIE_SCR = NIGHT_MODE_2;
		mdnie_data.DSI0_UI_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data.DSI0_UI_STANDARD_MDNIE_2 = NULL;
		mdnie_data.DSI0_UI_AUTO_MDNIE_2 = NULL;
		mdnie_data.DSI0_VIDEO_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data.DSI0_VIDEO_STANDARD_MDNIE_2 = NULL;
		mdnie_data.DSI0_VIDEO_AUTO_MDNIE_2 = NULL;
		mdnie_data.DSI0_CAMERA_MDNIE_2 = NULL;
		mdnie_data.DSI0_CAMERA_AUTO_MDNIE_2 = NULL;
		mdnie_data.DSI0_GALLERY_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data.DSI0_GALLERY_STANDARD_MDNIE_2 = NULL;
		mdnie_data.DSI0_GALLERY_AUTO_MDNIE_2 = NULL;
		mdnie_data.DSI0_VT_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data.DSI0_VT_STANDARD_MDNIE_2 = NULL;
		mdnie_data.DSI0_VT_AUTO_MDNIE_2 = NULL;
		mdnie_data.DSI0_BROWSER_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data.DSI0_BROWSER_STANDARD_MDNIE_2 = NULL;
		mdnie_data.DSI0_BROWSER_AUTO_MDNIE_2 = NULL;
		mdnie_data.DSI0_EBOOK_DYNAMIC_MDNIE_2 = NULL;
		mdnie_data.DSI0_EBOOK_STANDARD_MDNIE_2 = NULL;
		mdnie_data.DSI0_EBOOK_AUTO_MDNIE_2 = NULL;

		mdnie_data.DSI0_BYPASS_MDNIE = DSI0_BYPASS_MDNIE;
		mdnie_data.DSI0_NEGATIVE_MDNIE = DSI0_NEGATIVE_MDNIE;
		mdnie_data.DSI0_GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
		mdnie_data.DSI0_GRAYSCALE_NEGATIVE_MDNIE = DSI0_GRAYSCALE_NEGATIVE_MDNIE;
		mdnie_data.DSI0_COLOR_BLIND_MDNIE = DSI0_COLOR_BLIND_MDNIE;
		mdnie_data.DSI0_COLOR_BLIND_MDNIE_SCR = COLOR_BLIND_2;
		mdnie_data.DSI0_HBM_CE_MDNIE = DSI0_HBM_CE_MDNIE;
		mdnie_data.DSI0_RGB_SENSOR_MDNIE = DSI0_RGB_SENSOR_MDNIE;
		mdnie_data.DSI0_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
		mdnie_data.DSI0_COLOR_LENS_MDNIE = DSI0_COLOR_LENS_MDNIE;
		mdnie_data.DSI0_COLOR_LENS_MDNIE_SCR = COLOR_LENS_2;
		mdnie_data.DSI0_CURTAIN = NULL;
		mdnie_data.DSI0_UI_DYNAMIC_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_UI_STANDARD_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_UI_NATURAL_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_UI_MOVIE_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_UI_AUTO_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_VIDEO_OUTDOOR_MDNIE = NULL;
		mdnie_data.DSI0_VIDEO_DYNAMIC_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data.DSI0_VIDEO_STANDARD_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data.DSI0_VIDEO_NATURAL_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data.DSI0_VIDEO_MOVIE_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data.DSI0_VIDEO_AUTO_MDNIE = DSI0_VIDEO_MDNIE;
		mdnie_data.DSI0_VIDEO_WARM_OUTDOOR_MDNIE = NULL;
		mdnie_data.DSI0_VIDEO_WARM_MDNIE = NULL;
		mdnie_data.DSI0_VIDEO_COLD_OUTDOOR_MDNIE = NULL;
		mdnie_data.DSI0_VIDEO_COLD_MDNIE = NULL;
		mdnie_data.DSI0_CAMERA_OUTDOOR_MDNIE = NULL;
		mdnie_data.DSI0_CAMERA_MDNIE = DSI0_CAMERA_MDNIE;
		mdnie_data.DSI0_CAMERA_AUTO_MDNIE = DSI0_CAMERA_MDNIE;
		mdnie_data.DSI0_GALLERY_DYNAMIC_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data.DSI0_GALLERY_STANDARD_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data.DSI0_GALLERY_NATURAL_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data.DSI0_GALLERY_MOVIE_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data.DSI0_GALLERY_AUTO_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data.DSI0_VT_DYNAMIC_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_VT_STANDARD_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_VT_NATURAL_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_VT_MOVIE_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_VT_AUTO_MDNIE = DSI0_UI_MDNIE;
		mdnie_data.DSI0_BROWSER_DYNAMIC_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data.DSI0_BROWSER_STANDARD_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data.DSI0_BROWSER_NATURAL_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data.DSI0_BROWSER_MOVIE_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data.DSI0_BROWSER_AUTO_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data.DSI0_EBOOK_DYNAMIC_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data.DSI0_EBOOK_STANDARD_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data.DSI0_EBOOK_NATURAL_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data.DSI0_EBOOK_MOVIE_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data.DSI0_EBOOK_AUTO_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data.DSI0_EMAIL_AUTO_MDNIE = NULL;
		mdnie_data.DSI0_RGB_SENSOR_MDNIE_SCR = RGB_SENSOR_2;
		mdnie_data.mdnie_tune_value_dsi0 = mdnie_tune_value_dsi0;
		mdnie_data.light_notification_tune_value_dsi0 = light_notification_tune_value;
		
		/* Update MDNIE data related with size, offset or index */
		mdnie_data.dsi0_bypass_mdnie_size = ARRAY_SIZE(DSI0_BYPASS_MDNIE);
		mdnie_data.mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
		mdnie_data.mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
		mdnie_data.mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
		mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = ADDRESS_SCR_WHITE_RED;
		mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = ADDRESS_SCR_WHITE_GREEN;
		mdnie_data.address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = ADDRESS_SCR_WHITE_BLUE;
		mdnie_data.dsi0_rgb_sensor_mdnie_1_size = DSI0_RGB_SENSOR_MDNIE_1_SIZE;
		mdnie_data.dsi0_rgb_sensor_mdnie_2_size = DSI0_RGB_SENSOR_MDNIE_2_SIZE;
		mdnie_data.dsi0_night_mode_table = night_mode_data;
		mdnie_data.dsi1_night_mode_table = night_mode_data;
		mdnie_data.dsi0_color_lens_table = color_lens_data;
		mdnie_data.dsi0_max_night_mode_index = 11;
		mdnie_data.dsi1_max_night_mode_index = 11;
		mdnie_data.dsi0_white_default_r = 0xff;
		mdnie_data.dsi0_white_default_g = 0xff;
		mdnie_data.dsi0_white_default_b = 0xff;
		mdnie_data.dsi0_white_rgb_enabled = 0;
		mdnie_data.dsi1_white_default_r = 0xff;
		mdnie_data.dsi1_white_default_g = 0xff;
		mdnie_data.dsi1_white_default_b = 0xff;
		mdnie_data.dsi1_white_rgb_enabled = 0;
		mdnie_data.dsi0_scr_step_index = MDNIE_STEP1_INDEX;
		mdnie_data.dsi1_scr_step_index = MDNIE_STEP1_INDEX;
}

static void mdss_panel_init(struct samsung_display_driver_data *vdd)
{
	pr_info("%s : %s", __func__, vdd->panel_name);

	vdd->support_panel_max = S6D7AA0X11_TV080WXM_SUPPORT_PANEL_COUNT;
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

	vdd->manufacture_id_dsi[0] = get_lcd_attached("GET");

	//vdd->brightness[0].brightness_packet_tx_cmds_dsi.link_state = DSI_HS_MODE;
	vdd->mdss_panel_tft_outdoormode_update=mdss_panel_tft_outdoormode_update;
	
	vdd->bl_level = LCD_DEFAUL_BL_LEVEL;

	/* MDNIE */
	vdd->support_mdnie_trans_dimming = false;
	vdd->support_mdnie_lite = true;
	vdd->mdnie_tune_size[0] = 17;
	vdd->mdnie_tune_size[1] = 25;
	vdd->mdnie_tune_size[2] = 25;
	vdd->mdnie_tune_size[3] = 25;
	vdd->mdnie_tune_size[4] = 19;
	vdd->mdnie_tune_size[5] = 8;
	dsi_update_mdnie_data();

	vdd->panel_revision = mdss_panel_revision(vdd->ctrl_dsi[DISPLAY_1]);
	mdss_panel_attach_set(vdd->ctrl_dsi[DISPLAY_1], true);
}

static int __init samsung_panel_init(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	char panel_string[] = "ss_dsi_panel_S6D7AA0X11_TV080WXM_WXGA";


	vdd->panel_name = mdss_mdp_panel + 8;
	pr_info("%s : %s\n", __func__, vdd->panel_name);

	if (!strncmp(vdd->panel_name, panel_string, strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = mdss_panel_init;

	return 0;
}
early_initcall(samsung_panel_init);
