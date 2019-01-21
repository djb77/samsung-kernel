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
#include "ss_dsi_panel_SC7798D_BV038WVM.h"
#include "ss_dsi_mdnie_SC7798D_BV038WVM.h"

static int is_first_boot = 1;

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
static void update_mdnie_tft_cmds(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return;
	}

	if (!mdss_panel_attach_get(ctrl)) {
		pr_err("%s: mdss_panel_attach_get(%d) : %d\n",__func__, ctrl->ndx, mdss_panel_attach_get(ctrl));
		return;
	}

	if (vdd->support_mdnie_lite)
		update_dsi_tcon_mdnie_register(vdd);

}
static int mdss_panel_on_post(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	pr_info("%s %d\n", __func__, ctrl->ndx);

	mdss_samsung_cabc_update();

	if(is_first_boot){
		if (ctrl->panel_data.set_backlight)
			ctrl->panel_data.set_backlight(&ctrl->panel_data, LCD_DEFAUL_BL_LEVEL);
		is_first_boot = 0;
	}

	return true;
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

#define reg_outdoor 237
#define reg_max 186
#define reg_default 77 /*170cd*/
#define reg_min 4

#define bl_outdoor 324
#define bl_max 255
#define bl_default 127
#define bl_min 1

static struct dsi_panel_cmds * mdss_brightness_tft_pwm(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int reg_value;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	if (vdd->bl_level > bl_max)
		reg_value = (vdd->bl_level - bl_max)*(reg_outdoor-reg_max)/(bl_outdoor- bl_max) + reg_max;
	else if (vdd->bl_level >= bl_default)
		reg_value = (vdd->bl_level - bl_default)*(reg_max-reg_default)/(bl_max- bl_default) + reg_default;
	else if (vdd->bl_level >= bl_min)
		reg_value = (vdd->bl_level - bl_min)*(reg_default-reg_min)/(bl_default- bl_min) + reg_min;
	else reg_value = 0;

	vdd->dtsi_data[ctrl->ndx].tft_pwm_tx_cmds[vdd->panel_revision].cmds[0].payload[1] = reg_value;
	vdd->candela_level = reg_value;/*pwm level: regitster value*/

	/* MDNIE Outdoor Mode Setting :temp. solution for tft project without light sensor,
	*  In the case of no light sensor, pms should ? be set lux as over 0 value.
	*  This will be removed once PMS set specific lux on lux node for common way.
	*/
	if (vdd->bl_level > bl_max){
		if (vdd->lux!= ENTER_HBM_LUX){
			vdd->lux = ENTER_HBM_LUX;
			update_mdnie_tft_cmds(ctrl);
			pr_info("%s : MDNIE Outdoor Mode : %d\n", __func__, vdd->lux);
		}
	}else{
		if(vdd->lux!= 0) {
			vdd->lux = 0;
			update_mdnie_tft_cmds(ctrl);
			pr_info("%s : MDNIE Outdoor Mode : %d\n", __func__, vdd->lux);
		}
	}

	*level_key = PANEL_LEVE1_KEY;

	return &vdd->dtsi_data[ctrl->ndx].tft_pwm_tx_cmds[vdd->panel_revision];
}

static void mdnie_init(struct samsung_display_driver_data *vdd)
{
	mdnie_data = kzalloc(sizeof(struct mdnie_lite_tune_data), GFP_KERNEL);
	if (!mdnie_data) {
		LCD_ERR("fail to alloc mdnie_data..\n");
		return;
	}

	vdd->mdnie_tune_size1 = 113;
	vdd->mdnie_tune_size2 = 0;
	vdd->mdnie_reverse_scr = true;

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

		mdnie_data[0].BYPASS_MDNIE = DSI0_BYPASS_MDNIE;
		mdnie_data[0].NEGATIVE_MDNIE = DSI0_NEGATIVE_MDNIE;
		mdnie_data[0].GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
		mdnie_data[0].GRAYSCALE_NEGATIVE_MDNIE = DSI0_GRAYSCALE_NEGATIVE_MDNIE;
		mdnie_data[0].COLOR_BLIND_MDNIE = DSI0_COLOR_BLIND_MDNIE;
		mdnie_data[0].HBM_CE_MDNIE = DSI0_OUTDOOR_MDNIE;
		mdnie_data[0].RGB_SENSOR_MDNIE = NULL;
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
		mdnie_data[0].VIDEO_WARM_MDNIE = DSI0_VIDEO_WARM_MDNIE;
		mdnie_data[0].VIDEO_COLD_OUTDOOR_MDNIE = NULL;
		mdnie_data[0].VIDEO_COLD_MDNIE = DSI0_VIDEO_COLD_MDNIE;
		mdnie_data[0].CAMERA_OUTDOOR_MDNIE = NULL;
		mdnie_data[0].CAMERA_MDNIE = DSI0_CAMERA_MDNIE;
		mdnie_data[0].CAMERA_AUTO_MDNIE = DSI0_CAMERA_MDNIE;
		mdnie_data[0].GALLERY_DYNAMIC_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data[0].GALLERY_STANDARD_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data[0].GALLERY_NATURAL_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data[0].GALLERY_MOVIE_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data[0].GALLERY_AUTO_MDNIE = DSI0_GALLERY_MDNIE;
		mdnie_data[0].VT_DYNAMIC_MDNIE = DSI0_VT_MDNIE;
		mdnie_data[0].VT_STANDARD_MDNIE = DSI0_VT_MDNIE;
		mdnie_data[0].VT_NATURAL_MDNIE = DSI0_VT_MDNIE;
		mdnie_data[0].VT_MOVIE_MDNIE = DSI0_VT_MDNIE;
		mdnie_data[0].VT_AUTO_MDNIE = DSI0_VT_MDNIE;
		mdnie_data[0].BROWSER_DYNAMIC_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data[0].BROWSER_STANDARD_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data[0].BROWSER_NATURAL_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data[0].BROWSER_MOVIE_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data[0].BROWSER_AUTO_MDNIE = DSI0_BROWSER_MDNIE;
		mdnie_data[0].EBOOK_DYNAMIC_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EBOOK_STANDARD_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EBOOK_NATURAL_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EBOOK_MOVIE_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EBOOK_AUTO_MDNIE = DSI0_EBOOK_MDNIE;
		mdnie_data[0].EMAIL_AUTO_MDNIE = DSI0_EMAIL_MDNIE;
		mdnie_data[0].mdnie_tune_value = mdnie_tune_value_dsi0;
		mdnie_data[0].COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_CMDS;
		mdnie_data[0].light_notification_tune_value = light_notification_tune_value;
	/*	mdnie_data[0].RGB_SENSOR_MDNIE_SCR = DSI0_RGB_SENSOR_MDNIE_2;*/

		/* Update MDNIE data related with size, offset or index */
		mdnie_data[0].bypass_mdnie_size = ARRAY_SIZE(DSI0_BYPASS_MDNIE);
		mdnie_data[0].mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
		mdnie_data[0].mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
		mdnie_data[0].mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
		mdnie_data[0].address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = 0;
		mdnie_data[0].address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = 0;
		mdnie_data[0].address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = 0;
		mdnie_data[0].rgb_sensor_mdnie_1_size = 0;
		mdnie_data[0].rgb_sensor_mdnie_2_size = 0;
		mdnie_data[0].scr_step_index = MDNIE_STEP2_INDEX;
		mdnie_data[0].white_default_r = 0xff;
		mdnie_data[0].white_default_g = 0xff;
		mdnie_data[0].white_default_b = 0xff;
		mdnie_data[0].white_balanced_r = 0;
		mdnie_data[0].white_balanced_g = 0;
		mdnie_data[0].white_balanced_b = 0;
	}
}

static void backlight_tft_late_on(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return;
	}
	update_mdnie_tft_cmds(ctrl);/*It dose not work updating mdnie in mdss_panel_on_post func.*/
}

static void mdss_panel_init(struct samsung_display_driver_data *vdd)
{
	pr_info("%s : %s", __func__, vdd->panel_name);

	vdd->support_panel_max = SC7798D_BV038WVM_SUPPORT_PANEL_COUNT;
	vdd->support_cabc = true;

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = mdss_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = mdss_panel_on_post;
	vdd->panel_func.samsung_panel_off_pre = NULL;
	vdd->panel_func.samsung_panel_off_post = NULL;
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

	/* MDNIE */
	vdd->panel_func.samsung_mdnie_init = mdnie_init;

	mdss_panel_attach_set(vdd->ctrl_dsi[DISPLAY_1], true);
}

static int __init samsung_panel_init(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	char panel_string[] = "ss_dsi_panel_SC7798D_BV038WVM_WVGA";

	vdd->panel_name = mdss_mdp_panel + 8;
	pr_info("%s : %s\n", __func__, vdd->panel_name);

	if (!strncmp(vdd->panel_name, panel_string, strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = mdss_panel_init;

	return 0;
}
early_initcall(samsung_panel_init);
