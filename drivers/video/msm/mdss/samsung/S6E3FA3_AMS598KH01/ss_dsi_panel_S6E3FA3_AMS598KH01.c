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
#include "ss_dsi_panel_S6E3FA3_AMS598KH01.h"
#include "ss_dsi_mdnie_S6E3FA3_AMS598KH01.h"

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

//extern int poweroff_charging;

static int mdss_panel_on_post(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	pr_info("%s %d\n", __func__, ctrl->ndx);
/*
	if(poweroff_charging) {
		pr_info("%s LPM Mode Enable, Add More Delay\n", __func__);
		msleep(300);
	}
*/
	return true;
}

static int mdss_panel_revision(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	if (vdd->manufacture_id_dsi[ctrl->ndx] == PBA_ID)
		mdss_panel_attach_set(ctrl, false);
	else
		mdss_panel_attach_set(ctrl, true);

	vdd->panel_revision = 'B';/* revB can handle revA*/

	vdd->panel_revision -= 'A';

		/* AID Interpolation */
	vdd->aid_subdivision_enable = true;

	pr_debug("%s : panel_revision = %c %d \n", __func__,
					vdd->panel_revision + 'A', vdd->panel_revision);
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

	/* Read mtp (C8h 41,42th) for manufacture date */
	if (vdd->dtsi_data[ctrl->ndx].manufacture_date_rx_cmds[vdd->panel_revision].cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl,
			&vdd->dtsi_data[ctrl->ndx].manufacture_date_rx_cmds[vdd->panel_revision],
			date, PANEL_LEVE2_KEY);

		year = date[0] & 0xf0;
		year >>= 4;
		year += 2011; // 0 = 2011 year
		month = date[0] & 0x0f;
		day = date[1] & 0x1f;

		pr_info("%s DSI%d manufacture_date = %d", __func__, ctrl->ndx, year * 10000 + month * 100 + day);

		vdd->manufacture_date_dsi[ctrl->ndx]  =   year * 10000 + month * 100 + day;
	} else {
		pr_err("%s DSI%d error", __func__, ctrl->ndx);
		return false;
	}

	return true;
}

static int mdss_ddi_id_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char ddi_id[5];
	int loop;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read mtp (D6h 1~5th) for ddi id */
	if (vdd->dtsi_data[ctrl->ndx].ddi_id_rx_cmds[vdd->panel_revision].cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].ddi_id_rx_cmds[vdd->panel_revision]),
			ddi_id, PANEL_LEVE2_KEY);

		for(loop = 0; loop < 5; loop++)
			vdd->ddi_id_dsi[ctrl->ndx][loop] = ddi_id[loop];

		pr_info("%s DSI%d : %02x %02x %02x %02x %02x\n", __func__, ctrl->ndx,
			vdd->ddi_id_dsi[ctrl->ndx][0], vdd->ddi_id_dsi[ctrl->ndx][1],
			vdd->ddi_id_dsi[ctrl->ndx][2], vdd->ddi_id_dsi[ctrl->ndx][3],
			vdd->ddi_id_dsi[ctrl->ndx][4]);
	} else {
		pr_err("%s DSI%d error", __func__, ctrl->ndx);
		return false;
	}

	return true;
}

static int mdss_cell_id_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char cell_id_buffer[MAX_CELL_ID];
	int loop;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read Panel Unique Cell ID (C8h 41~51th) */
	if (vdd->dtsi_data[ctrl->ndx].cell_id_rx_cmds[vdd->panel_revision].cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].cell_id_rx_cmds[vdd->panel_revision]),
			cell_id_buffer, PANEL_LEVE2_KEY);

		for(loop = 0; loop < MAX_CELL_ID; loop++)
			vdd->cell_id_dsi[ctrl->ndx][loop] = cell_id_buffer[loop];

		/* For AMS568HN01 exceptionally, MDNIE X&Y value(4bytes) should be get from A1h register read value */
		if(vdd->mdnie_x[ctrl->ndx] && vdd->mdnie_y[ctrl->ndx]) {
			vdd->cell_id_dsi[ctrl->ndx][7] = vdd->mdnie_x[ctrl->ndx] >> 8 & 0xff;
			vdd->cell_id_dsi[ctrl->ndx][8] = vdd->mdnie_x[ctrl->ndx] & 0xff;
			vdd->cell_id_dsi[ctrl->ndx][9] = vdd->mdnie_y[ctrl->ndx] >> 8 & 0xff;
			vdd->cell_id_dsi[ctrl->ndx][10] = vdd->mdnie_y[ctrl->ndx] & 0xff;
		} else
			pr_err("%s: MDNIE X,Y Value is Zero \n", __func__);

		pr_info("%s DSI_%d cell_id: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			__func__, ctrl->ndx, vdd->cell_id_dsi[ctrl->ndx][0],
			vdd->cell_id_dsi[ctrl->ndx][1],	vdd->cell_id_dsi[ctrl->ndx][2],
			vdd->cell_id_dsi[ctrl->ndx][3],	vdd->cell_id_dsi[ctrl->ndx][4],
			vdd->cell_id_dsi[ctrl->ndx][5],	vdd->cell_id_dsi[ctrl->ndx][6],
			vdd->cell_id_dsi[ctrl->ndx][7],	vdd->cell_id_dsi[ctrl->ndx][8],
			vdd->cell_id_dsi[ctrl->ndx][9],	vdd->cell_id_dsi[ctrl->ndx][10]);

	} else {
		pr_err("%s DSI%d error", __func__, ctrl->ndx);
		return false;
	}

	return true;
}

static int mdss_hbm_read(struct mdss_dsi_ctrl_pdata *ctrl)
{
	char hbm_buffer1[6] = {0,};
	char hbm_buffer2[29] = {0,};
	char hbm_elvss[2] ={0,};

	char V255R_0, V255R_1, V255G_0, V255G_1, V255B_0, V255B_1;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	if (vdd->dtsi_data[ctrl->ndx].hbm_rx_cmds[vdd->panel_revision].cmd_cnt) {
	/* Read mtp (C8h 51~65th) for hbm gamma */
		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].hbm_rx_cmds[vdd->panel_revision]),
			hbm_buffer1, PANEL_LEVE1_KEY);

		V255R_0 = (hbm_buffer1[0] & BIT(2))>>2;
		V255R_1 = hbm_buffer1[1];
		V255G_0 = (hbm_buffer1[0] & BIT(1))>>1;
		V255G_1 = hbm_buffer1[2];
		V255B_0 = hbm_buffer1[0] & BIT(0);
		V255B_1 = hbm_buffer1[3];
		hbm_buffer1[0] = V255R_0;
		hbm_buffer1[1] = V255R_1;
		hbm_buffer1[2] = V255G_0;
		hbm_buffer1[3] = V255G_1;
		hbm_buffer1[4] = V255B_0;
		hbm_buffer1[5] = V255B_1;


		pr_debug("mdss_hbm_read = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			hbm_buffer1[0], hbm_buffer1[1], hbm_buffer1[2], hbm_buffer1[3], hbm_buffer1[4], hbm_buffer1[5]);
		memcpy(&vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[1], hbm_buffer1, 6);

	} else {
		pr_err("%s DSI%d error", __func__, ctrl->ndx);
		return false;
	}

	if (vdd->dtsi_data[ctrl->ndx].hbm2_rx_cmds[vdd->panel_revision].cmd_cnt) {

		/* Read mtp (C8h 51~65th) for hbm gamma */
		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].hbm2_rx_cmds[vdd->panel_revision]),
			hbm_buffer2, PANEL_LEVE1_KEY);

		memcpy(&vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[7], hbm_buffer2, 29);
	} else {
		pr_err("%s DSI%d error", __func__, ctrl->ndx);
		return false;
	}

	if (vdd->dtsi_data[ctrl->ndx].elvss_rx_cmds[vdd->panel_revision].cmd_cnt) {
		/* Read mtp (B6h 22~23th) for hbm elvss */
		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].elvss_rx_cmds[vdd->panel_revision]),
			hbm_elvss, PANEL_LEVE1_KEY);
		vdd->display_status_dsi[ctrl->ndx].elvss_value1 = hbm_elvss[0];

		/*elvss B6 copy 23th's to 22th: it's used only in only interpolation, not in hbm(pure) mode*/
		memcpy(&vdd->dtsi_data[ctrl->ndx].hbm_etc_tx_cmds[vdd->panel_revision].cmds[2].payload[1], &hbm_elvss[1], 1);

	} else {
		pr_err("%s DSI%d error", __func__, ctrl->ndx);
		return false;
	}
	return true;
}
static struct dsi_panel_cmds *mdss_hbm_gamma(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	pr_debug("%s vdd->auto_brightness : %d\n", __func__, vdd->auto_brightness);

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi[ctrl->ndx]->generate_gamma)) {
		pr_err("%s generate_gamma is NULL error", __func__);
		return NULL;
	} else {
		vdd->smart_dimming_dsi[ctrl->ndx]->generate_hbm_gamma(
			vdd->smart_dimming_dsi[ctrl->ndx],
			vdd->auto_brightness,
			&vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[1]);

		*level_key = PANEL_LEVE1_KEY;

		return &vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision];
	}
}

static struct dsi_panel_cmds *mdss_hbm_etc(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int B6_2ND_PARA = 0x00;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = PANEL_LEVE1_KEY;


	if (vdd->auto_brightness == 12) /* for HBM pure mode : 600nit */
		return &vdd->dtsi_data[ctrl->ndx].hbm_etc2_tx_cmds[vdd->panel_revision];

	else{ /* for HBM interpolation */
		switch(vdd->auto_brightness){

		case 6: /*465*/
			B6_2ND_PARA = 0x0E;
			break;
		case 7:	/*488*/
			B6_2ND_PARA = 0x0C;
			break;
		case 8:	/*510*/
			B6_2ND_PARA = 0x0A;
			break;
		case 9:	/*533*/
			B6_2ND_PARA = 0x09;
			break;
		case 10: /*555*/
			B6_2ND_PARA = 0x07;
			break;
		case 11: /*578*/
			B6_2ND_PARA = 0x06;
			break;

		}

		pr_info("%s B6_2ND_PARA = 0x%x, B6_22ND = 0x%x\n", __func__, B6_2ND_PARA,
			vdd->dtsi_data[ctrl->ndx].hbm_etc_tx_cmds[vdd->panel_revision].cmds[2].payload[1]);

		vdd->dtsi_data[ctrl->ndx].hbm_etc_tx_cmds[vdd->panel_revision].cmds[0].payload[2] = B6_2ND_PARA;

		return &vdd->dtsi_data[ctrl->ndx].hbm_etc_tx_cmds[vdd->panel_revision];
	}
}

static struct dsi_panel_cmds *mdss_hbm_off(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = PANEL_LEVE1_KEY;

	return &vdd->dtsi_data[ctrl->ndx].hbm_off_tx_cmds[vdd->panel_revision];
}
#define COORDINATE_DATA_SIZE 6

#define F1(x,y) ((y)-((43*(x))/40)+45)
#define F2(x,y) ((y)-((310*(x))/297)-3)
#define F3(x,y) ((y)+((367*(x))/84)-16305)
#define F4(x,y) ((y)+((333*(x))/107)-12396)

/* Normal Mode */
static char coordinate_data_1[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xfb, 0x00, 0xfb, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfc, 0x00, 0xff, 0x00}, /* Tune_2 */
	{0xfb, 0x00, 0xfa, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfe, 0x00, 0xfc, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xfb, 0x00, 0xfc, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xfd, 0x00, 0xff, 0x00, 0xfa, 0x00}, /* Tune_7 */
	{0xfc, 0x00, 0xff, 0x00, 0xfc, 0x00}, /* Tune_8 */
	{0xfb, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_9 */
};

/* sRGB/Adobe RGB Mode */
static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xf6, 0x00, 0xec, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xf6, 0x00, 0xef, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xf8, 0x00, 0xf3, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xf8, 0x00, 0xec, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xf9, 0x00, 0xef, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfb, 0x00, 0xf3, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfb, 0x00, 0xec, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfc, 0x00, 0xef, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xfd, 0x00, 0xf3, 0x00}, /* Tune_9 */
};

static char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE] = {
	coordinate_data_1, /* DYNAMIC - Normal */
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
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* Read mtp (D6h 1~5th) for ddi id */
	if (vdd->dtsi_data[ctrl->ndx].mdnie_read_rx_cmds[vdd->panel_revision].cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl,
			&(vdd->dtsi_data[ctrl->ndx].mdnie_read_rx_cmds[vdd->panel_revision]),
			x_y_location, PANEL_LEVE2_KEY);

		vdd->mdnie_x[ctrl->ndx] = x_y_location[0] << 8 | x_y_location[1];	/* X */
		vdd->mdnie_y[ctrl->ndx] = x_y_location[2] << 8 | x_y_location[3];	/* Y */

		mdnie_tune_index = mdnie_coordinate_index(vdd->mdnie_x[ctrl->ndx], vdd->mdnie_y[ctrl->ndx]);
		coordinate_tunning_multi(ctrl->ndx, coordinate_data_multi, mdnie_tune_index,
			ADDRESS_SCR_WHITE_RED, COORDINATE_DATA_SIZE);

		pr_info("%s DSI%d : X-%d Y-%d \n", __func__, ctrl->ndx,
			vdd->mdnie_x[ctrl->ndx], vdd->mdnie_y[ctrl->ndx]);
	} else {
		pr_err("%s DSI%d error", __func__, ctrl->ndx);
		return false;
	}

	return true;
}

static int mdss_smart_dimming_init(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	vdd->smart_dimming_dsi[ctrl->ndx] = vdd->panel_func.samsung_smart_get_conf();

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi[ctrl->ndx])) {
		pr_err("%s DSI%d error", __func__, ctrl->ndx);
		return false;
	} else {

		if (vdd->dtsi_data[ctrl->ndx].smart_dimming_mtp_rx_cmds[vdd->panel_revision].cmd_cnt){
			mdss_samsung_panel_data_read(ctrl,
				&(vdd->dtsi_data[ctrl->ndx].smart_dimming_mtp_rx_cmds[vdd->panel_revision]),
				vdd->smart_dimming_dsi[ctrl->ndx]->mtp_buffer, PANEL_LEVE2_KEY);

			/* Initialize smart dimming related things here */
			/* lux_tab setting for 350cd */
			vdd->smart_dimming_dsi[ctrl->ndx]->lux_tab = vdd->dtsi_data[ctrl->ndx].candela_map_table[vdd->panel_revision].lux_tab;
			vdd->smart_dimming_dsi[ctrl->ndx]->lux_tabsize = vdd->dtsi_data[ctrl->ndx].candela_map_table[vdd->panel_revision].lux_tab_size;
			vdd->smart_dimming_dsi[ctrl->ndx]->man_id = vdd->manufacture_id_dsi[ctrl->ndx];
			vdd->smart_dimming_dsi[ctrl->ndx]->hbm_payload = &vdd->dtsi_data[ctrl->ndx].hbm_gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[1];

			/* Just a safety check to ensure smart dimming data is initialised well */
			vdd->smart_dimming_dsi[ctrl->ndx]->init(vdd->smart_dimming_dsi[ctrl->ndx]);

			vdd->temperature = 20; // default temperature

			vdd->smart_dimming_loaded_dsi[ctrl->ndx] = true;
		} else {
			pr_err("%s DSI%d error", __func__, ctrl->ndx);
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
	int cmd_idx = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	if (vdd->aid_subdivision_enable) {
		aid_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].aid_subdivision_tx_cmds[vdd->panel_revision].cmds[vdd->bl_level]);
		pr_err("%s : level(%d), aid(%x %x)\n", __func__, vdd->bl_level, aid_cmd.cmds->payload[1], aid_cmd.cmds->payload[2]);
	} else {
		cd_index = get_cmd_index(vdd, ctrl->ndx);

		if (!vdd->dtsi_data[ctrl->ndx].aid_map_table[vdd->panel_revision].size ||
			cd_index > vdd->dtsi_data[ctrl->ndx].aid_map_table[vdd->panel_revision].size)
			goto end;

		cmd_idx = vdd->dtsi_data[ctrl->ndx].aid_map_table[vdd->panel_revision].cmd_idx[cd_index];

		aid_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].aid_tx_cmds[vdd->panel_revision].cmds[cmd_idx]);
	}

	aid_cmd.cmd_cnt = 1;

	*level_key = PANEL_LEVE1_KEY;

	return &aid_cmd;

end :
	pr_err("%s error", __func__);
	return NULL;
}

static int mdss_force_acl_on(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int acl_on;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return false;
	}

	/* acl is always on */
	acl_on = 1;

	/* Gallery & Max brightness */
	if ((vdd->weakness_hbm_comp == 2) && (vdd->bl_level == 255))
		acl_on = 0;

	pr_err("%s : acl_on(%d)\n", __func__, acl_on);

	return acl_on;
}

static struct dsi_panel_cmds * mdss_acl_on(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = PANEL_LEVE1_KEY;

	return &(vdd->dtsi_data[ctrl->ndx].acl_on_tx_cmds[vdd->panel_revision]);
}

static struct dsi_panel_cmds * mdss_acl_off(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	*level_key = PANEL_LEVE1_KEY;

	return &(vdd->dtsi_data[ctrl->ndx].acl_off_tx_cmds[vdd->panel_revision]);
}

static struct dsi_panel_cmds acl_percent_cmd;
static struct dsi_panel_cmds * mdss_acl_precent(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int cd_index = 0;
	/* 0 : ACL OFF, 1: ACL 30%, 2: ACL 15%, 3: ACL 50% */
	int cmd_idx = ACL_15;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	cd_index = get_cmd_index(vdd, ctrl->ndx);

	if (!vdd->dtsi_data[ctrl->ndx].acl_map_table[vdd->panel_revision].size ||
		cd_index > vdd->dtsi_data[ctrl->ndx].acl_map_table[vdd->panel_revision].size)
		goto end;

	acl_percent_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].acl_percent_tx_cmds[vdd->panel_revision].cmds[cmd_idx]);
	acl_percent_cmd.cmd_cnt = 1;

	*level_key = PANEL_LEVE1_KEY;

	return &acl_percent_cmd;

end :
	pr_err("%s error", __func__);
	return NULL;

}

static struct dsi_panel_cmds elvss_cmd;
static struct dsi_panel_cmds * mdss_elvss(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int cd_index = 0;
	int cmd_idx = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	cd_index = get_cmd_index(vdd, ctrl->ndx);

	if (!vdd->dtsi_data[ctrl->ndx].smart_acl_elvss_map_table[vdd->panel_revision].size ||
		cd_index > vdd->dtsi_data[ctrl->ndx].smart_acl_elvss_map_table[vdd->panel_revision].size)
		goto end;

	cmd_idx = vdd->dtsi_data[ctrl->ndx].smart_acl_elvss_map_table[vdd->panel_revision].cmd_idx[cd_index];

	if (vdd->acl_status || vdd->siop_status) {
		elvss_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].smart_acl_elvss_tx_cmds[vdd->panel_revision].cmds[cmd_idx]);
	} else {
		elvss_cmd.cmds = &(vdd->dtsi_data[ctrl->ndx].elvss_tx_cmds[vdd->panel_revision].cmds[cmd_idx]);
	}

	elvss_cmd.cmd_cnt = 1;

	*level_key = PANEL_LEVE1_KEY;

	return &elvss_cmd;

end :
	pr_err("%s error", __func__);
	return NULL;
}

static struct dsi_panel_cmds * mdss_elvss_temperature1(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	if (vdd->temperature >= 0)
		vdd->dtsi_data[ctrl->ndx].elvss_lowtemp_tx_cmds[vdd->panel_revision].cmds[0].payload[1] = vdd->temperature;
	else {
		vdd->dtsi_data[ctrl->ndx].elvss_lowtemp_tx_cmds[vdd->panel_revision].cmds[0].payload[1] = (vdd->temperature*-1) | 0x80;
	}

	pr_debug("%s temp : %d 0xB8 : 0x%x\n", __func__, vdd->temperature,
		vdd->dtsi_data[ctrl->ndx].elvss_lowtemp_tx_cmds[vdd->panel_revision].cmds[0].payload[1] );

	*level_key = PANEL_LEVE1_KEY;

	return &(vdd->dtsi_data[ctrl->ndx].elvss_lowtemp_tx_cmds[vdd->panel_revision]);
}

static struct dsi_panel_cmds * mdss_elvss_temperature2(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	int candela_value = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	candela_value = get_candela_value(vdd, ctrl->ndx);

	if (vdd->temperature > 0) {
		if (candela_value == 2)	/* 2 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x15;
		else if (candela_value < 15) /* 3 ~ 14 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x15;
		else /* 15 ~ 420 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= vdd->display_status_dsi[ctrl->ndx].elvss_value1;
	} else if (vdd->temperature <= 0 && vdd->temperature > -20) {
		if (candela_value == 2)	/* 2 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1F;
		else if (candela_value == 3) /* 3 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1E;
		else if (candela_value == 4) /* 4 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1D;
		else if (candela_value == 5) /* 5 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1C;
		else if (candela_value == 6) /* 6 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1B;
		else if (candela_value == 7) /* 7 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1A;
		else if (candela_value == 8) /* 8 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x19;
		else if (candela_value < 11) /* 9~10 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x18;
		else if (candela_value < 13) /* 11~12 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x17;
		else if (candela_value < 15) /* 13~14 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x16;
		else /* 15 ~ 420 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= vdd->display_status_dsi[ctrl->ndx].elvss_value1;
	} else {
		if (candela_value == 2)	/* 2 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1F;
		else if (candela_value == 3) /* 3 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1E;
		else if (candela_value == 4) /* 4 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1D;
		else if (candela_value == 5) /* 5 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1C;
		else if (candela_value == 6) /* 6 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1B;
		else if (candela_value == 7) /* 7 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x1A;
		else if (candela_value == 8) /* 8 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x19;
		else if (candela_value < 11) /* 9~10 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x18;
		else if (candela_value < 13) /* 11~12 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x17;
		else if (candela_value < 15) /* 13~14 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= 0x16;
		else /* 15 ~ 420 nit */
			vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]
				= vdd->display_status_dsi[ctrl->ndx].elvss_value1;
	}


	pr_debug("%s temp : %d 0xB0 : 0x%x 0xB6 : 0x%x\n", __func__, vdd->temperature,
		vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[0].payload[1],
		vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision].cmds[1].payload[1]);

	*level_key = PANEL_LEVE1_KEY;

	return &(vdd->dtsi_data[ctrl->ndx].elvss_lowtemp2_tx_cmds[vdd->panel_revision]);
}


static struct dsi_panel_cmds * mdss_gamma(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data ctrl : 0x%zx vdd : 0x%zx", __func__, (size_t)ctrl, (size_t)vdd);
		return NULL;
	}

	vdd->candela_level = get_candela_value(vdd, ctrl->ndx);

	pr_debug("%s bl_level : %d candela : %dCD\n", __func__, vdd->bl_level, vdd->candela_level);

	if (IS_ERR_OR_NULL(vdd->smart_dimming_dsi[ctrl->ndx]->generate_gamma)) {
		pr_err("%s generate_gamma is NULL error", __func__);
		return NULL;
	} else {
		vdd->smart_dimming_dsi[ctrl->ndx]->generate_gamma(
			vdd->smart_dimming_dsi[ctrl->ndx],
			vdd->candela_level,
			&vdd->dtsi_data[ctrl->ndx].gamma_tx_cmds[vdd->panel_revision].cmds[0].payload[1]);

		*level_key = PANEL_LEVE1_KEY;

		return &vdd->dtsi_data[ctrl->ndx].gamma_tx_cmds[vdd->panel_revision];
	}
}
#if 0
static int samsung_osc_te_fitting_get_cmd(struct te_fitting_lut *lut, long long te_duration)
{
	int ret;
	int i = 0;

	if (IS_ERR_OR_NULL(lut)) {
		pr_err("%s: Invalid te fitting lut\n", __func__);
		return -EINVAL;
	}

	do {
		if (te_duration >= lut[i].te_duration ||
				lut[i].te_duration == 0) {
			ret = lut[i].value;
			break;
		}
		i++;
	} while(true);

	return ret;

}
#endif

static void mdnie_init(struct samsung_display_driver_data *vdd)
{
	mdnie_data = kzalloc(sizeof(struct mdnie_lite_tune_data), GFP_KERNEL);
	if (!mdnie_data) {
		LCD_ERR("fail to alloc mdnie_data..\n");
		return;
	}

	vdd->mdnie_tune_size1 = 4;
	vdd->mdnie_tune_size2 = 155;

	if (mdnie_data) {
		/* Update mdnie command */
		mdnie_data[0].COLOR_BLIND_MDNIE_2 = DSI0_COLOR_BLIND_MDNIE_2;
		mdnie_data[0].RGB_SENSOR_MDNIE_1 = DSI0_RGB_SENSOR_MDNIE_1;
		mdnie_data[0].RGB_SENSOR_MDNIE_2 = DSI0_RGB_SENSOR_MDNIE_2;

		mdnie_data[0].BYPASS_MDNIE = DSI0_BYPASS_MDNIE;
		mdnie_data[0].NEGATIVE_MDNIE = DSI0_NEGATIVE_MDNIE;
		mdnie_data[0].COLOR_BLIND_MDNIE = DSI0_COLOR_BLIND_MDNIE;
		mdnie_data[0].HBM_CE_MDNIE = DSI0_HBM_CE_MDNIE;
		mdnie_data[0].HBM_CE_TEXT_MDNIE = DSI0_HBM_CE_TEXT_MDNIE;
		mdnie_data[0].RGB_SENSOR_MDNIE = DSI0_RGB_SENSOR_MDNIE;
		mdnie_data[0].CURTAIN = DSI0_CURTAIN;
		mdnie_data[0].GRAYSCALE_MDNIE = DSI0_GRAYSCALE_MDNIE;
		mdnie_data[0].GRAYSCALE_NEGATIVE_MDNIE = DSI0_GRAYSCALE_NEGATIVE_MDNIE;
		mdnie_data[0].COLOR_BLIND_MDNIE_SCR = DSI0_COLOR_BLIND_MDNIE_2;
		mdnie_data[0].RGB_SENSOR_MDNIE_SCR = DSI0_RGB_SENSOR_MDNIE_2;

		mdnie_data[0].mdnie_tune_value = mdnie_tune_value_dsi0;
		mdnie_data[0].light_notification_tune_value = light_notification_tune_value;

		/* Update MDNIE data related with size, offset or index */
		mdnie_data[0].bypass_mdnie_size = ARRAY_SIZE(DSI0_BYPASS_MDNIE);
		mdnie_data[0].mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
		mdnie_data[0].mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
		mdnie_data[0].mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
		mdnie_data[0].address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = ADDRESS_SCR_WHITE_RED;
		mdnie_data[0].address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = ADDRESS_SCR_WHITE_GREEN;
		mdnie_data[0].address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = ADDRESS_SCR_WHITE_BLUE;
		mdnie_data[0].rgb_sensor_mdnie_1_size = DSI0_RGB_SENSOR_MDNIE_1_SIZE;
		mdnie_data[0].rgb_sensor_mdnie_2_size = DSI0_RGB_SENSOR_MDNIE_2_SIZE;
		mdnie_data[0].adjust_ldu_table = adjust_ldu_data;
		mdnie_data[0].max_adjust_ldu = 6;
		mdnie_data[0].scr_step_index = MDNIE_STEP2_INDEX;
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
	pr_info("%s\n", __func__);

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = mdss_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = mdss_panel_on_post;

	/* DDI RX */
	vdd->panel_func.samsung_panel_revision = mdss_panel_revision;
	vdd->panel_func.samsung_manufacture_date_read = mdss_manufacture_date_read;
	vdd->panel_func.samsung_ddi_id_read = mdss_ddi_id_read;
	vdd->panel_func.samsung_cell_id_read = mdss_cell_id_read;
	vdd->panel_func.samsung_hbm_read = mdss_hbm_read;
	vdd->panel_func.samsung_mdnie_read = mdss_mdnie_read;
	vdd->panel_func.samsung_smart_dimming_init = mdss_smart_dimming_init;
	vdd->panel_func.samsung_smart_get_conf = smart_get_conf_S6E3FA3_AMS598KH01;

	/* Brightness */
	vdd->panel_func.samsung_brightness_hbm_off = mdss_hbm_off;
	vdd->panel_func.samsung_brightness_aid = mdss_aid;
	vdd->panel_func.samsung_brightness_acl_on = mdss_acl_on;
	vdd->panel_func.samsung_brightness_acl_percent = mdss_acl_precent;
	vdd->panel_func.samsung_brightness_acl_off = mdss_acl_off;
	vdd->panel_func.samsung_brightness_elvss = mdss_elvss;
	vdd->panel_func.samsung_brightness_elvss_temperature1 = mdss_elvss_temperature1;
	vdd->panel_func.samsung_brightness_elvss_temperature2 = mdss_elvss_temperature2;
	vdd->panel_func.samsung_brightness_vint = NULL;
	vdd->panel_func.samsung_brightness_gamma = mdss_gamma;

	/* HBM */
	vdd->panel_func.samsung_hbm_gamma = mdss_hbm_gamma;
	vdd->panel_func.samsung_hbm_etc = mdss_hbm_etc;

	vdd->manufacture_id_dsi[0] = PBA_ID;

	vdd->auto_brightness_level = 12;

	/* MDNIE */
	vdd->panel_func.samsung_mdnie_init = mdnie_init;

	/* force ACL */
	vdd->panel_func.samsung_force_acl_on = mdss_force_acl_on;
}

static int __init samsung_panel_init(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	char panel_string[] = "ss_dsi_panel_S6E3FA3_AMS598KH01_FHD";

	vdd->panel_name = mdss_mdp_panel + 8;
	pr_info("%s : %s\n", __func__, vdd->panel_name);

	if (!strncmp(vdd->panel_name, panel_string, strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = mdss_panel_init;

	/* TODO: Remove below line */
	vdd->panel_func.samsung_panel_init = mdss_panel_init;

	return 0;
}
early_initcall(samsung_panel_init);
