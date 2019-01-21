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
#include "ss_dsi_panel_EA8061S_AMS498QV01.h"
#include "ss_dsi_mdnie_EA8061S_AMS498QV01.h"

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

	vdd->panel_revision = 'A' - 'A';
	/* AID Interpolation */
	vdd->aid_subdivision_enable = true;
	return true;
}

static int mdss_manufacture_date_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	unsigned char date[2];
	int year, month, day;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read mtp (A1h 5,6 th) for manufacture date */
	if (get_panel_rx_cmds(ctrl, RX_MANUFACTURE_DATE)->cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl, get_panel_rx_cmds(ctrl, RX_MANUFACTURE_DATE),
			date, LEVEL1_KEY);

		year = date[0] & 0xf0;
		year >>= 4;
		year += 2011; // 0 = 2011 year
		month = date[0] & 0x0f;
		day = date[1] & 0x1f;

		LCD_ERR("manufacture_date DSI%d = %d",ctrl->ndx, year * 10000 + month * 100 + day);
		vdd->manufacture_date_dsi[ctrl->ndx]  =   year * 10000 + month * 100 + day;
	} else {
		LCD_ERR("DSI%d no manufacture_date_rx_cmds cmds(%d)", ctrl->ndx, vdd->panel_revision);
		return false;
	}
	return true;
}

static int mdss_cell_id_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char cell_id_buffer[MAX_CELL_ID] = {0,};
	int loop;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read Panel Unique Cell ID (A1h 1~11th) */
	if (get_panel_rx_cmds(ctrl, RX_CELL_ID)->cmd_cnt) {
		memset(cell_id_buffer, 0x00, MAX_CELL_ID);
		
		mdss_samsung_panel_data_read(ctrl, get_panel_rx_cmds(ctrl, RX_CELL_ID),
				cell_id_buffer, LEVEL1_KEY);

		/* manufacture date (A1h 5~11th)*/
		for(loop = 0; loop < 7; loop++)
			vdd->cell_id_dsi[ctrl->ndx][loop] = cell_id_buffer[4+loop];
		/* coordinate xy (A1h 1~4th) */
		for(loop = 7; loop < MAX_CELL_ID; loop++)
			vdd->cell_id_dsi[ctrl->ndx][loop] = cell_id_buffer[loop - 7];

		pr_info("%s DSI_%d cell_id: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			__func__, ctrl->ndx, vdd->cell_id_dsi[ctrl->ndx][0],
			vdd->cell_id_dsi[ctrl->ndx][1],	vdd->cell_id_dsi[ctrl->ndx][2],
			vdd->cell_id_dsi[ctrl->ndx][3],	vdd->cell_id_dsi[ctrl->ndx][4],
			vdd->cell_id_dsi[ctrl->ndx][5],	vdd->cell_id_dsi[ctrl->ndx][6],
			vdd->cell_id_dsi[ctrl->ndx][7],	vdd->cell_id_dsi[ctrl->ndx][8],
			vdd->cell_id_dsi[ctrl->ndx][9],	vdd->cell_id_dsi[ctrl->ndx][10]);

	} else {
		LCD_ERR("DSI%d no cell_id_rx_cmds cmd\n", ctrl->ndx);
		return false;
	}

	return true;
}

static struct dsi_panel_cmds *mdss_hbm_off(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	struct dsi_panel_cmds *hbm_off_cmds = get_panel_tx_cmds(ctrl, TX_HBM_OFF);

	pr_info("%s !!", __func__);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_off_cmds)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL1_KEY;
	return hbm_off_cmds;
}

#define COORDINATE_DATA_SIZE 6
#define MDNIE_SCR_WR_ADDR 	0x32

#define F1(x,y) ((y)-((547*(x))/503)+31)
#define F2(x,y) ((y)-((467*(x))/447)-25)
#define F3(x,y) ((y)+((201*(x))/39)-18718)
#define F4(x,y) ((y)+((523*(x))/173)-12111)

/* Normal Mode */
static char coordinate_data_1[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xf9, 0x00, 0xf9, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfa, 0x00, 0xfe, 0x00}, /* Tune_2 */
	{0xf9, 0x00, 0xf7, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfd, 0x00, 0xfa, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xf9, 0x00, 0xfb, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xfb, 0x00, 0xff, 0x00, 0xf8, 0x00}, /* Tune_7 */
	{0xfb, 0x00, 0xff, 0x00, 0xfb, 0x00}, /* Tune_8 */
	{0xf9, 0x00, 0xff, 0x00, 0xfe, 0x00}, /* Tune_9 */
};

/* sRGB/Adobe RGB Mode */
static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xf8, 0x00, 0xf0, 0x00}, /* Tune_9 */
};

static char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE] = {
	coordinate_data_2, /* DYNAMIC - DCI-P3 */
	coordinate_data_2, /* STANDARD - sRGB/Adobe RGB */
	coordinate_data_2, /* NATURAL - sRGB/Adobe RGB */
	coordinate_data_1, /* MOVIE - Normal */
	coordinate_data_1, /* AUTO - Normal */
	coordinate_data_1, /* READING - Normal */
};

static int mdnie_coordinate_index(int x, int y)
{
	int tune_number = 0;

	if (F1(x,y) > 0) {
		if (F3(x,y) > 0) {
			tune_number = 3;
		} else {
			if (F4(x,y) < 0)
				tune_number = 1;
			else
				tune_number = 2;
		}
	} else {
		if (F2(x,y) < 0) {
			if (F3(x,y) > 0) {
				tune_number = 9;
			} else {
				if (F4(x,y) < 0)
					tune_number = 7;
				else
					tune_number = 8;
			}
		} else {
			if (F3(x,y) > 0)
				tune_number = 6;
			else {
				if (F4(x,y) < 0)
					tune_number = 4;
				else
					tune_number = 5;
			}
		}
	}

	return tune_number;
}

static int mdss_mdnie_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char x_y_location[4];
	int mdnie_tune_index = 0;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read mtp (A1h 1~4th) for x,y co-ordinate*/
	if (get_panel_rx_cmds(ctrl, RX_MDNIE)->cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl, get_panel_rx_cmds(ctrl, RX_MDNIE),
				x_y_location, LEVEL1_KEY);

		vdd->mdnie_x[ctrl->ndx] = x_y_location[0] << 8 | x_y_location[1];	/* X */
		vdd->mdnie_y[ctrl->ndx] = x_y_location[2] << 8 | x_y_location[3];	/* Y */

		mdnie_tune_index = mdnie_coordinate_index(vdd->mdnie_x[ctrl->ndx], vdd->mdnie_y[ctrl->ndx]);
		coordinate_tunning_multi(ctrl->ndx, coordinate_data_multi, mdnie_tune_index,
			MDNIE_SCR_WR_ADDR, COORDINATE_DATA_SIZE);

		LCD_INFO("PANEL%d : X-%d Y-%d \n", ctrl->ndx,
			vdd->mdnie_x[ctrl->ndx], vdd->mdnie_y[ctrl->ndx]);
	} else {
		LCD_ERR("PANEL%d error\n", ctrl->ndx);
		return false;
	}

	return true;
}

static struct dsi_panel_cmds *mdss_hbm_etc(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	struct dsi_panel_cmds *hbm_etc_cmds = get_panel_tx_cmds(ctrl, TX_HBM_ETC);

	pr_info("%s !!", __func__);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(hbm_etc_cmds)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	/* ACL */
	if (!vdd->acl_status) {
		/* gallery app */
		hbm_etc_cmds->cmds[2].payload[1] 
			= 0x08; /* 16 Frame Avg */
		hbm_etc_cmds->cmds[2].payload[4] 
			= 0x00; /* ACL 0% */
		
	}
	else {
		/* not gallery app */
		hbm_etc_cmds->cmds[2].payload[1] 
			= 0x0A; /* 32 Frame Avg */
		hbm_etc_cmds->cmds[2].payload[4] 
			= 0x0A; /* ACL 8% */
		
	}

	*level_key = LEVEL1_KEY;
	return hbm_etc_cmds;
}

static int mdss_smart_dimming_init(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int ndx = display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}
	vdd->smart_dimming_dsi[ctrl->ndx] = vdd->panel_func.samsung_smart_get_conf();

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi[ctrl->ndx])) {
		pr_err("%s DSI%d error", __func__, ctrl->ndx);
		return false;
	} else {
			if (get_panel_rx_cmds(ctrl, RX_SMART_DIM_MTP)->cmd_cnt) {
					mdss_samsung_panel_data_read(ctrl, get_panel_rx_cmds(ctrl, RX_SMART_DIM_MTP),
					vdd->smart_dimming_dsi[ctrl->ndx]->mtp_buffer, LEVEL1_KEY);

				/* Initialize smart dimming related things here */
				/* lux_tab setting for 360cd */
				vdd->smart_dimming_dsi[ctrl->ndx]->lux_tab = vdd->dtsi_data[ctrl->ndx].candela_map_table[vdd->panel_revision].cd;
				vdd->smart_dimming_dsi[ctrl->ndx]->lux_tabsize = vdd->dtsi_data[ctrl->ndx].candela_map_table[vdd->panel_revision].tab_size;;
				vdd->smart_dimming_dsi[ctrl->ndx]->man_id = vdd->manufacture_id_dsi[ctrl->ndx];

				/* Just a safety check to ensure smart dimming data is initialised well */
				vdd->smart_dimming_dsi[ctrl->ndx]->init(vdd->smart_dimming_dsi[ctrl->ndx]);
				vdd->temperature = 20; // default temperature
				vdd->smart_dimming_loaded_dsi[ctrl->ndx] = true;
			} else {
					LCD_ERR("DSI%d error\n", ndx);
					return false;
			}
		}

	pr_info("%s DSI%d : --\n",__func__, ctrl->ndx);
	return true;
}

static struct dsi_panel_cmds aid_cmd;
static struct dsi_panel_cmds *mdss_aid(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int cd_index = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	cd_index = vdd->bl_level;

	aid_cmd.cmd_cnt = 1;
	aid_cmd.cmds = &(get_panel_tx_cmds(ctrl, TX_AID_SUBDIVISION)->cmds[cd_index]);
	pr_err("%s : level(%d), aid(%x %x)\n", __func__, vdd->bl_level, aid_cmd.cmds->payload[3], aid_cmd.cmds->payload[4]);

	*level_key = LEVEL1_KEY;

	return &aid_cmd;
}

static struct dsi_panel_cmds * mdss_acl_on(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL1_KEY;
	return get_panel_tx_cmds(ctrl, TX_ACL_ON);
}

static struct dsi_panel_cmds * mdss_acl_off(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = LEVEL1_KEY;
	return get_panel_tx_cmds(ctrl, TX_ACL_OFF);
}

static struct dsi_panel_cmds elvss_cmd;
static struct dsi_panel_cmds * mdss_elvss(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	struct dsi_panel_cmds *elvss_cmds = get_panel_tx_cmds(ctrl, TX_ELVSS);
	int cd_index = 0;
	int cmd_idx = 0;
	int candela_value = 0;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(elvss_cmds)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	cd_index = vdd->cd_idx;
	candela_value = vdd->candela_level;

	if (!vdd->dtsi_data[ctrl->ndx].elvss_map_table[vdd->panel_revision].size ||
		cd_index > vdd->dtsi_data[ctrl->ndx].elvss_map_table[vdd->panel_revision].size)
		goto end;

	cmd_idx = vdd->dtsi_data[ctrl->ndx].elvss_map_table[vdd->panel_revision].cmd_idx[cd_index];
	elvss_cmd.cmds = &(elvss_cmds->cmds[cmd_idx]);
	elvss_cmd.cmd_cnt = 1;
	
	if(candela_value <= 29)
	{
		if (vdd->temperature <= -15)
			elvss_cmd.cmds->payload[2] = 0x92;
		else if(vdd->temperature > -15 && vdd->temperature <= 0)
			elvss_cmd.cmds->payload[2] = 0x88;
		else
		{
			if(candela_value >=5 && candela_value <= 20)
				elvss_cmd.cmds->payload[2] = 0x87;
			else if(candela_value == 21)
				elvss_cmd.cmds->payload[2] = 0x89;
			else if(candela_value == 22)
				elvss_cmd.cmds->payload[2] = 0x8B;
			else if(candela_value == 24)
				elvss_cmd.cmds->payload[2] = 0x8D;
			else if(candela_value == 25)
				elvss_cmd.cmds->payload[2] = 0x8F;
			else if(candela_value == 27)
				elvss_cmd.cmds->payload[2] = 0x91;
			else if(candela_value == 29)
				elvss_cmd.cmds->payload[2] = 0x93;
		}
	}
	

	*level_key = LEVEL1_KEY;
	return &elvss_cmd;
end :
	pr_err("%s error", __func__);
	return NULL;
}

static struct dsi_panel_cmds * mdss_elvss_temperature1(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	struct dsi_panel_cmds *low_temp_cmds = get_panel_tx_cmds(ctrl, TX_ELVSS_LOWTEMP);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(low_temp_cmds)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	if (vdd->temperature >= 0)
		low_temp_cmds->cmds[0].payload[1] = vdd->temperature;
	else
		low_temp_cmds->cmds[0].payload[1] = (vdd->temperature*-1) | 0x80;

	pr_debug("%s temp : %d 0xB8 : 0x%x\n", __func__, vdd->temperature,
		low_temp_cmds->cmds[0].payload[1]);

	*level_key = LEVEL1_KEY;

	return low_temp_cmds;
}

static struct dsi_panel_cmds * mdss_gamma(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	struct dsi_panel_cmds  *gamma_cmds = get_panel_tx_cmds(ctrl, TX_GAMMA);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(gamma_cmds)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	pr_debug("%s bl_level : %d candela : %dCD\n", __func__, vdd->bl_level, vdd->candela_level);

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi[ctrl->ndx]->generate_gamma)) {
		pr_err("%s generate_gamma is NULL error", __func__);
		return NULL;
	} else {
		vdd->smart_dimming_dsi[ctrl->ndx]->generate_gamma(
			vdd->smart_dimming_dsi[ctrl->ndx],
			vdd->candela_level,
			&gamma_cmds->cmds[0].payload[1]);

		*level_key = LEVEL1_KEY;
		return gamma_cmds;
	}
}

static void dsi_update_mdnie_data(void)
{

	/* Update mdnie command */
	mdnie_data.DSI0_RGB_SENSOR_MDNIE_1 = DSI0_RGB_SENSOR_MDNIE_1;
	mdnie_data.DSI0_RGB_SENSOR_MDNIE_2 = DSI0_RGB_SENSOR_MDNIE_2;

	mdnie_data.DSI0_BYPASS_MDNIE = DSI0_BYPASS_MDNIE;
	mdnie_data.DSI0_NEGATIVE_MDNIE = DSI0_NEGATIVE_MDNIE;
	mdnie_data.DSI0_GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
	mdnie_data.DSI0_GRAYSCALE_NEGATIVE_MDNIE = DSI0_GRAYSCALE_NEGATIVE_MDNIE;
	mdnie_data.DSI0_COLOR_BLIND_MDNIE = DSI0_COLOR_BLIND_MDNIE;
	mdnie_data.DSI0_COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_2;
	mdnie_data.DSI0_HBM_CE_MDNIE = DSI0_HBM_CE_MDNIE;
	mdnie_data.DSI0_HBM_CE_TEXT_MDNIE = DSI0_HBM_CE_TEXT_MDNIE;
	mdnie_data.DSI0_RGB_SENSOR_MDNIE = DSI0_RGB_SENSOR_MDNIE;
	mdnie_data.DSI0_CURTAIN = DSI0_CURTAIN;

	mdnie_data.DSI0_UI_DYNAMIC_MDNIE = DSI0_UI_DYNAMIC_MDNIE;
	mdnie_data.DSI0_UI_STANDARD_MDNIE = DSI0_UI_STANDARD_MDNIE;
	mdnie_data.DSI0_UI_NATURAL_MDNIE = DSI0_UI_NATURAL_MDNIE;
	mdnie_data.DSI0_UI_MOVIE_MDNIE = DSI0_UI_MOVIE_MDNIE;
	mdnie_data.DSI0_UI_AUTO_MDNIE = DSI0_UI_AUTO_MDNIE;
	mdnie_data.DSI0_VIDEO_OUTDOOR_MDNIE = DSI0_VIDEO_OUTDOOR_MDNIE;
	mdnie_data.DSI0_VIDEO_DYNAMIC_MDNIE = DSI0_VIDEO_DYNAMIC_MDNIE;
	mdnie_data.DSI0_VIDEO_STANDARD_MDNIE = DSI0_VIDEO_STANDARD_MDNIE;
	mdnie_data.DSI0_VIDEO_NATURAL_MDNIE = DSI0_VIDEO_NATURAL_MDNIE;
	mdnie_data.DSI0_VIDEO_MOVIE_MDNIE = DSI0_VIDEO_MOVIE_MDNIE;
	mdnie_data.DSI0_VIDEO_AUTO_MDNIE = DSI0_VIDEO_AUTO_MDNIE;
	mdnie_data.DSI0_VIDEO_WARM_OUTDOOR_MDNIE = DSI0_VIDEO_WARM_OUTDOOR_MDNIE;
	mdnie_data.DSI0_VIDEO_WARM_MDNIE = DSI0_VIDEO_WARM_MDNIE;
	mdnie_data.DSI0_VIDEO_COLD_OUTDOOR_MDNIE = DSI0_VIDEO_COLD_OUTDOOR_MDNIE;
	mdnie_data.DSI0_VIDEO_COLD_MDNIE = DSI0_VIDEO_COLD_MDNIE;
	mdnie_data.DSI0_CAMERA_OUTDOOR_MDNIE = DSI0_CAMERA_OUTDOOR_MDNIE;
	mdnie_data.DSI0_CAMERA_AUTO_MDNIE = DSI0_CAMERA_AUTO_MDNIE;
	mdnie_data.DSI0_GALLERY_DYNAMIC_MDNIE = DSI0_GALLERY_DYNAMIC_MDNIE;
	mdnie_data.DSI0_GALLERY_STANDARD_MDNIE = DSI0_GALLERY_STANDARD_MDNIE;
	mdnie_data.DSI0_GALLERY_NATURAL_MDNIE = DSI0_GALLERY_NATURAL_MDNIE;
	mdnie_data.DSI0_GALLERY_MOVIE_MDNIE = DSI0_GALLERY_MOVIE_MDNIE;
	mdnie_data.DSI0_GALLERY_AUTO_MDNIE = DSI0_GALLERY_AUTO_MDNIE;
	mdnie_data.DSI0_VT_DYNAMIC_MDNIE = DSI0_VT_DYNAMIC_MDNIE;
	mdnie_data.DSI0_VT_STANDARD_MDNIE = DSI0_VT_STANDARD_MDNIE;
	mdnie_data.DSI0_VT_NATURAL_MDNIE = DSI0_VT_NATURAL_MDNIE;
	mdnie_data.DSI0_VT_MOVIE_MDNIE = DSI0_VT_MOVIE_MDNIE;
	mdnie_data.DSI0_VT_AUTO_MDNIE = DSI0_VT_AUTO_MDNIE;
	mdnie_data.DSI0_BROWSER_DYNAMIC_MDNIE = DSI0_BROWSER_DYNAMIC_MDNIE;
	mdnie_data.DSI0_BROWSER_STANDARD_MDNIE = DSI0_BROWSER_STANDARD_MDNIE;
	mdnie_data.DSI0_BROWSER_NATURAL_MDNIE = DSI0_BROWSER_NATURAL_MDNIE;
	mdnie_data.DSI0_BROWSER_MOVIE_MDNIE = DSI0_BROWSER_MOVIE_MDNIE;
	mdnie_data.DSI0_BROWSER_AUTO_MDNIE = DSI0_BROWSER_AUTO_MDNIE;
	mdnie_data.DSI0_EBOOK_DYNAMIC_MDNIE = DSI0_EBOOK_DYNAMIC_MDNIE;
	mdnie_data.DSI0_EBOOK_STANDARD_MDNIE = DSI0_EBOOK_STANDARD_MDNIE;
	mdnie_data.DSI0_EBOOK_NATURAL_MDNIE = DSI0_EBOOK_NATURAL_MDNIE;
	mdnie_data.DSI0_EBOOK_MOVIE_MDNIE = DSI0_EBOOK_MOVIE_MDNIE;
	mdnie_data.DSI0_EBOOK_AUTO_MDNIE = DSI0_EBOOK_AUTO_MDNIE;
	mdnie_data.DSI0_EMAIL_AUTO_MDNIE = DSI0_EMAIL_AUTO_MDNIE;
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
	mdnie_data.dsi0_scr_step_index = MDNIE_STEP2_INDEX;
	mdnie_data.DSI0_NIGHT_MODE_MDNIE = DSI0_NIGHT_MODE_MDNIE;
	mdnie_data.DSI0_NIGHT_MODE_MDNIE_1 = DSI0_NIGHT_MODE_MDNIE_2;
	mdnie_data.dsi0_night_mode_table = night_mode_data;
	mdnie_data.dsi0_max_night_mode_index = 11;
	
	mdnie_data.dsi0_adjust_ldu_table = adjust_ldu_data;
	mdnie_data.dsi0_max_adjust_ldu = 6;
	mdnie_data.dsi0_white_default_r = 0xff;
	mdnie_data.dsi0_white_default_g = 0xff;
	mdnie_data.dsi0_white_default_b = 0xff;

}

static void mdss_panel_init(struct samsung_display_driver_data *vdd)
{
	pr_info("%s\n", __func__);

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = mdss_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = mdss_panel_on_post;
	vdd->panel_func.samsung_panel_off_pre = NULL;
	vdd->panel_func.samsung_panel_off_post = NULL;
	vdd->panel_func.samsung_backlight_late_on = NULL;

	/* DDI RX */
	vdd->panel_func.samsung_panel_revision = mdss_panel_revision;
	vdd->panel_func.samsung_manufacture_date_read = mdss_manufacture_date_read;
	vdd->panel_func.samsung_ddi_id_read = NULL;
	vdd->panel_func.samsung_cell_id_read = mdss_cell_id_read;
	vdd->panel_func.samsung_hbm_read = NULL;
	vdd->panel_func.samsung_mdnie_read = mdss_mdnie_read;
	vdd->panel_func.samsung_smart_dimming_init = mdss_smart_dimming_init;
	vdd->panel_func.samsung_smart_get_conf = smart_get_conf_EA8061S_AMS498QV01;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = NULL;
	vdd->panel_func.samsung_hbm_etc = mdss_hbm_etc;
	vdd->panel_func.samsung_brightness_hbm_off = mdss_hbm_off;

	vdd->manufacture_id_dsi[0] = get_lcd_attached("GET");
	
	/* Brightness */
	vdd->panel_func.samsung_brightness_aid = mdss_aid;
	vdd->panel_func.samsung_brightness_acl_on = mdss_acl_on;
	vdd->panel_func.samsung_brightness_acl_percent = NULL;
	vdd->panel_func.samsung_brightness_acl_off = mdss_acl_off;
	vdd->panel_func.samsung_brightness_elvss = mdss_elvss;
	vdd->panel_func.samsung_brightness_elvss_temperature1 = mdss_elvss_temperature1;
	vdd->panel_func.samsung_brightness_elvss_temperature2 = NULL;
	vdd->panel_func.samsung_brightness_vint = NULL;
	vdd->panel_func.samsung_brightness_gamma = mdss_gamma;

	/* default brightness */
	vdd->bl_level = 255;

	/* ACL default 15% ON, */
	vdd->acl_status = 1;

	/* MDNIE */
	vdd->support_mdnie_lite = true;
	vdd->support_mdnie_trans_dimming = false;
	/* for mdnie tuning */
	vdd->mdnie_tune_size[0] = 4;
	vdd->mdnie_tune_size[1] = 113;
	vdd->mdnie_tune_size[2] = 0;
	dsi_update_mdnie_data();
	/* Initialize the panel before first commit on bootup */
	vdd->init_panel_before_commit = true;
}

static int __init samsung_panel_init(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	char panel_string[] = "ss_dsi_panel_EA8061S_AMS498QV01_QHD";

	vdd->panel_name = mdss_mdp_panel + 8;
	pr_info("%s : %s\n", __func__, vdd->panel_name);

	if (!strncmp(vdd->panel_name, panel_string, strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = mdss_panel_init;

	return 0;
}

early_initcall(samsung_panel_init);
