/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * DDI operation : self clock, self mask, self icon.. etc.
 * Author: QC LCD driver <kr0124.cho@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ss_dsi_panel_common.h"
#include "ss_self_display_common.h"

/*
 * make dsi_panel_cmds using image data
 */
void make_self_dispaly_img_cmds(enum mipi_samsung_tx_cmd_list cmd, u32 op)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	struct samsung_display_dtsi_data *dtsi_data = NULL;
	static struct dsi_panel_cmds *pcmds;
	static struct dsi_cmd_desc *tcmds;
	u32 data_size = vdd->self_disp.operation[op].img_size;
	char *data = vdd->self_disp.operation[op].img_buf;
	int i, j;
	int data_idx = 0;
	u32 p_size = CMD_ALIGN;
	u32 payload_size = 0;
	u32 cmd_size = 0;

	if (!data) {
		LCD_ERR("data is null..\n");
		return;
	}

	if (!data_size) {
		LCD_ERR("data size is zero..\n");
		return;
	}

	/* payload size */
	while (p_size < MAX_PAYLOAD_SIZE) {
		if (data_size % p_size == 0)
			payload_size = p_size;
		p_size += CMD_ALIGN;
	}
	/* cmd size */
	cmd_size = data_size / payload_size;

	LCD_INFO("[%d] total data size [%d]\n", cmd, data_size);
	LCD_INFO("cmd size [%d] payload size [%d]\n", cmd_size, payload_size);

	/* only for revA */
	dtsi_data = &vdd->dtsi_data[0];
	pcmds = &dtsi_data->panel_tx_cmd_list[cmd][0];

	if (pcmds->cmds == NULL) {
		pcmds->cmds = kzalloc(cmd_size * sizeof(struct dsi_cmd_desc), GFP_KERNEL);
		if (pcmds->cmds == NULL) {
			LCD_ERR("fail to kzalloc for self_mask cmds \n");
			return;
		}
	}

	pcmds->link_state = DSI_HS_MODE;
	pcmds->cmd_cnt = cmd_size;

	tcmds = kzalloc(cmd_size * sizeof(struct dsi_cmd_desc), GFP_KERNEL);
	if (tcmds == NULL) {
		LCD_ERR("fail to kzalloc for tcmds \n");
		return;
	}

	for (i = 0; i < pcmds->cmd_cnt; i++) {
		tcmds[i].dchdr.dtype = DTYPE_GEN_LWRITE;
		tcmds[i].dchdr.last = 1;

		/* fill image data */
		tcmds[i].payload = kzalloc(payload_size + 1, GFP_KERNEL);
		if (tcmds[i].payload == NULL) {
			LCD_ERR("fail to kzalloc for self_mask cmds payload \n");
			return;
		}

		tcmds[i].payload[0] = (i == 0) ? 0x4C : 0x5C;

		for (j = 1; (j <= payload_size) && (data_idx < data_size); j++)
			tcmds[i].payload[j] = data[data_idx++];

		tcmds[i].dchdr.dlen = j;

		pcmds->cmds[i] = tcmds[i];

		LCD_DEBUG("dlen (%d), data_idx (%d)\n", j, data_idx);
	}

	return;
}

void ss_self_display_parse_dt_cmds(struct device_node *np, struct mdss_dsi_ctrl_pdata *ctrl,
							int panel_rev, struct samsung_display_dtsi_data *dtsi_data)
{
	/*for debug*/
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LDI_LOG_DISABLE][panel_rev],
			"samsung,ldi_log_disable_tx_cmds_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LDI_LOG_ENABLE][panel_rev],
			"samsung,ldi_log_enable_tx_cmds_rev", panel_rev, NULL);

	/* SELF DISPLAY ON/OFF */
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_DISP_ON][panel_rev],
			"samsung,self_dispaly_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_DISP_OFF][panel_rev],
			"samsung,self_dispaly_off_rev", panel_rev, NULL);

	/* SELF MASK OPERATION */
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_MASK_SIDE_MEM_SET][panel_rev],
			"samsung,self_mask_mem_setting_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_MASK_ON][panel_rev],
			"samsung,self_mask_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_MASK_OFF][panel_rev],
			"samsung,self_mask_off_rev", panel_rev, NULL);

	/* SELF MOVE OPERATION */
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_MOVE_SMALL_JUMP_ON][panel_rev],
			"samsung,self_move_small_jump_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_MOVE_MID_BIG_JUMP_ON][panel_rev],
			"samsung,self_move_mid_big_jump_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_MOVE_OFF][panel_rev],
			"samsung,self_move_off_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_MOVE_2C_SYNC_OFF][panel_rev],
			"samsung,self_move_2c_sync_off_rev", panel_rev, NULL);

	/* SELF ICON GRID OPERATION */
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ICON_SIDE_MEM_SET][panel_rev],
			"samsung,self_icon_mem_setting_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ICON_ON_GRID_ON][panel_rev],
			"samsung,self_icon_on_grid_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ICON_ON_GRID_OFF][panel_rev],
			"samsung,self_icon_on_grid_off_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ICON_OFF_GRID_ON][panel_rev],
			"samsung,self_icon_off_grid_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ICON_OFF_GRID_OFF][panel_rev],
			"samsung,self_icon_off_grid_off_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_BRIGHTNESS_ICON_ON][panel_rev],
			"samsung,self_brightness_icon_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_BRIGHTNESS_ICON_OFF][panel_rev],
			"samsung,self_brightness_icon_off_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ICON_GRID_2C_SYNC_OFF][panel_rev],
			"samsung,self_icon_grid_2c_sync_off_rev", panel_rev, NULL);

	/* SELF ANALOG CLOCK OPERATION */
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ACLOCK_SIDE_MEM_SET][panel_rev],
			"samsung,self_aclock_sidemem_setting_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ACLOCK_ON][panel_rev],
			"samsung,self_aclock_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ACLOCK_TIME_UPDATE][panel_rev],
			"samsung,self_aclock_time_update_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ACLOCK_ROTATION][panel_rev],
			"samsung,self_aclock_rotation_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_ACLOCK_OFF][panel_rev],
			"samsung,self_aclock_off_rev", panel_rev, NULL);

	/* SELF DIGITAL CLOCK OPERATION */
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_DCLOCK_SIDE_MEM_SET][panel_rev],
			"samsung,self_dclock_sidemem_setting_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_DCLOCK_ON][panel_rev],
			"samsung,self_dclock_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_DCLOCK_TIME_UPDATE][panel_rev],
			"samsung,self_dclock_time_update_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_DCLOCK_BLINKING_ON][panel_rev],
			"samsung,self_dclock_dlinking_on_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_DCLOCK_BLINKING_OFF][panel_rev],
			"samsung,self_dclock_dlinking_off_rev", panel_rev, NULL);
	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_DCLOCK_OFF][panel_rev],
			"samsung,self_dclock_off_rev", panel_rev, NULL);

	parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SELF_CLOCK_2C_SYNC_OFF][panel_rev],
			"samsung,self_clock_2c_sync_off_rev", panel_rev, NULL);

	/* SELF DISPLAY DEBUGGING */
	parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_SELF_DISP_DEBUG][panel_rev],
			"samsung,self_display_debug_rx_cmds_rev", panel_rev, NULL);

	parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_LOG][panel_rev],
			"samsung,ldi_log_rx_cmds_rev", panel_rev, NULL);
}

int ss_self_display_aod_enter(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_DEBUG("self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}

	LCD_ERR("++\n");

	ss_self_mask_on(ctrl, 0);

	vdd->self_disp.on = 1;

	LCD_ERR("--\n");

	return ret;
}

int ss_self_display_aod_exit(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR("self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}

	LCD_ERR("++\n");

	ss_self_mask_on(ctrl, 1);

	vdd->self_disp.on = 0;

	LCD_ERR("--\n");

	return ret;
}

void ss_self_move_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_move_lock);

	if (enable)
		mdss_samsung_send_cmd(ctrl, TX_SELF_MOVE_SMALL_JUMP_ON);
	else
		mdss_samsung_send_cmd(ctrl, TX_SELF_MOVE_OFF);

	mutex_unlock(&vdd->self_disp.vdd_self_move_lock);

	LCD_ERR("-- \n");

	return;
}

int ss_self_move_set(const void __user *argp)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	struct mdss_dsi_ctrl_pdata *ctrl = samsung_get_dsi_ctrl(vdd);
	struct samsung_display_dtsi_data *dtsi_data = NULL;
	static struct dsi_panel_cmds *pcmds;
	struct self_move_info sm_info;

	int sm_start = 8;
	int sm_end = 66;
	int sm_idx = 0;

	int sim_start = 67;
	int sim_end = 95;
	int sim_idx = 0;

	int ret, i;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (unlikely(!argp)) {
		LCD_ERR("invalid read buffer\n");
		return -EINVAL;
	}

	LCD_ERR("++\n");

	ret = copy_from_user(&sm_info, argp, sizeof(sm_info));
	if (ret) {
		LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
		return ret;
	}

	/* only for revA */
	dtsi_data = &vdd->dtsi_data[0];
	pcmds = &dtsi_data->panel_tx_cmd_list[TX_SELF_MOVE_SMALL_JUMP_ON][0];

	pcmds->cmds->payload[3] |= (sm_info.MOVE_DISP_X & 0x700) >> 8;
	pcmds->cmds->payload[4] |= sm_info.MOVE_DISP_X & 0xFF;

	pcmds->cmds->payload[5] |= (sm_info.MOVE_DISP_X & 0xF00) >> 8;
	pcmds->cmds->payload[6] |= sm_info.MOVE_DISP_X & 0xFF;

	/* Self Move for ICON parameter / i : 8 ~ 66 */
	for (i = sm_start; i <= sm_end; i++) {
		pcmds->cmds->payload[i] |= sm_info.SELF_MV_X[sm_idx] << 4;
		pcmds->cmds->payload[i] |= sm_info.SELF_MV_Y[sm_idx++];
	}

	/* To do..*/
	/* Self ICON Move for ICON parameter / i : 67 ~ 95 */
	for (i = sim_start; i <= sim_end; i++) {
		pcmds->cmds->payload[i] |= sm_info.SI_MOVE_X[sim_idx] << 6;
		pcmds->cmds->payload[i] |= sm_info.SI_MOVE_Y[sim_idx++] << 4;
		pcmds->cmds->payload[i] |= sm_info.SI_MOVE_X[sim_idx] << 2;
		pcmds->cmds->payload[i] |= sm_info.SI_MOVE_Y[sim_idx++];
	}
	/* i : 96*/
	pcmds->cmds->payload[i] |= sm_info.SI_MOVE_X[sim_idx] << 6;
	pcmds->cmds->payload[i] |= sm_info.SI_MOVE_X[sim_idx++] << 4;

	ss_self_move_on(ctrl, 1);

	return 0;
}

void ss_self_icon_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_icon_grid_lock);

	if (enable) {
		mdss_samsung_send_cmd(ctrl, TX_SELF_ICON_ON_GRID_ON);
/*		mdss_samsung_send_cmd(ctrl, TX_SELF_ICON_ON_GRID_OFF);*/
/*		mdss_samsung_send_cmd(ctrl, TX_SELF_ICON_OFF_GRID_ON);*/
	} else {
		mdss_samsung_send_cmd(ctrl, TX_SELF_ICON_OFF_GRID_OFF);
	}

	mutex_unlock(&vdd->self_disp.vdd_self_icon_grid_lock);

	LCD_ERR("--\n");

	return;
}

void ss_self_icon_grid_img_write(struct mdss_dsi_ctrl_pdata *ctrl)
{
	LCD_ERR("++\n");
	mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_ENABLE);
	mdss_samsung_send_cmd(ctrl, TX_SELF_ICON_SIDE_MEM_SET);
	mdss_samsung_send_cmd(ctrl, TX_SELF_ICON_IMAGE);
	mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_DISABLE);
	LCD_ERR("--\n");
}

int ss_self_icon_grid_set(const void __user *argp)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	struct mdss_dsi_ctrl_pdata *ctrl = samsung_get_dsi_ctrl(vdd);
	struct samsung_display_dtsi_data *dtsi_data = NULL;
	static struct dsi_panel_cmds *pcmds;
/*	enum mipi_samsung_tx_cmd_list cmd;*/
	int ret;
	struct self_icon_grid_info sig_info;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (unlikely(!argp)) {
		LCD_ERR("invalid read buffer\n");
		return -EINVAL;
	}

	LCD_ERR("++\n");


	ret = copy_from_user(&sig_info, argp, sizeof(sig_info));
	if (ret) {
		LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
		return ret;
	}

	/* only for revA */
	dtsi_data = &vdd->dtsi_data[0];

	/* TX_SELF_ICON_ON_GRID_ON*/
	if (sig_info.SI_ICON_EN && sig_info.SG_GRID_EN) {
		pcmds = &dtsi_data->panel_tx_cmd_list[TX_SELF_ICON_ON_GRID_ON][0];

		pcmds->cmds->payload[3] = (sig_info.SI_ST_X & 0x700) >> 8;
		pcmds->cmds->payload[4] = sig_info.SI_ST_X & 0xFF;

		pcmds->cmds->payload[5] = (sig_info.SI_ST_Y & 0xF00) >> 8;
		pcmds->cmds->payload[6] = sig_info.SI_ST_Y & 0xFF;

		pcmds->cmds->payload[7] = (sig_info.SI_IMG_WIGTH & 0x700) >> 8;
		pcmds->cmds->payload[8] = sig_info.SI_IMG_WIGTH & 0xFF;

		pcmds->cmds->payload[9] = (sig_info.SI_IMG_HEIGHT & 0xF00) >> 8;
		pcmds->cmds->payload[10] = sig_info.SI_IMG_HEIGHT & 0xFF;

		pcmds->cmds->payload[13] = (sig_info.SG_WIN_STP_X & 0x700) >> 8;
		pcmds->cmds->payload[14] = (sig_info.SG_WIN_STP_X & 0xFF) >> 8;

		pcmds->cmds->payload[15] = (sig_info.SG_WIN_STP_Y & 0xF00) >> 8;
		pcmds->cmds->payload[16] = (sig_info.SG_WIN_STP_Y & 0xFF) >> 8;

		pcmds->cmds->payload[17] = (sig_info.SG_WIN_END_X & 0x700) >> 8;
		pcmds->cmds->payload[18] = (sig_info.SG_WIN_END_X & 0xFF) >> 8;

		pcmds->cmds->payload[19] = (sig_info.SG_WIN_END_Y & 0xF00) >> 8;
		pcmds->cmds->payload[20] = (sig_info.SG_WIN_END_Y & 0xFF) >> 8;
	}
	/* TX_SELF_ICON_ON_GRID_OFF*/
	else if (sig_info.SI_ICON_EN && !sig_info.SG_GRID_EN) {
		pcmds = &dtsi_data->panel_tx_cmd_list[TX_SELF_ICON_ON_GRID_OFF][0];

		pcmds->cmds->payload[3] = (sig_info.SI_ST_X & 0x700) >> 8;
		pcmds->cmds->payload[4] = sig_info.SI_ST_X & 0xFF;

		pcmds->cmds->payload[5] = (sig_info.SI_ST_Y & 0xF00) >> 8;
		pcmds->cmds->payload[6] = sig_info.SI_ST_Y & 0xFF;

		pcmds->cmds->payload[7] = (sig_info.SI_IMG_WIGTH & 0x700) >> 8;
		pcmds->cmds->payload[8] = sig_info.SI_IMG_WIGTH & 0xFF;

		pcmds->cmds->payload[9] = (sig_info.SI_IMG_HEIGHT & 0xF00) >> 8;
		pcmds->cmds->payload[10] = sig_info.SI_IMG_HEIGHT & 0xFF;
	}
	/* TX_SELF_ICON_OFF_GRID_ON*/
	else if (!sig_info.SI_ICON_EN && sig_info.SG_GRID_EN) {
		pcmds = &dtsi_data->panel_tx_cmd_list[TX_SELF_ICON_OFF_GRID_ON][0];

		pcmds->cmds->payload[13] = (sig_info.SG_WIN_STP_X & 0x700) >> 8;
		pcmds->cmds->payload[14] = (sig_info.SG_WIN_STP_X & 0xFF) >> 8;

		pcmds->cmds->payload[15] = (sig_info.SG_WIN_STP_Y & 0xF00) >> 8;
		pcmds->cmds->payload[16] = (sig_info.SG_WIN_STP_Y & 0xFF) >> 8;

		pcmds->cmds->payload[17] = (sig_info.SG_WIN_END_X & 0x700) >> 8;
		pcmds->cmds->payload[18] = (sig_info.SG_WIN_END_X & 0xFF) >> 8;

		pcmds->cmds->payload[19] = (sig_info.SG_WIN_END_Y & 0xF00) >> 8;
		pcmds->cmds->payload[20] = (sig_info.SG_WIN_END_Y & 0xFF) >> 8;
	}

	ss_self_icon_on(ctrl, 1);

	LCD_ERR("--\n");

	return 0;
}

void ss_self_aclock_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_aclock_lock);

	if (enable)
		mdss_samsung_send_cmd(ctrl, TX_SELF_ACLOCK_ON);
	else
		mdss_samsung_send_cmd(ctrl, TX_SELF_ACLOCK_OFF);

	mutex_unlock(&vdd->self_disp.vdd_self_aclock_lock);

	LCD_ERR("--\n");

	return;
}

void ss_self_aclock_img_write(struct mdss_dsi_ctrl_pdata *ctrl)
{
	LCD_ERR("++\n");
	mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_ENABLE);
	mdss_samsung_send_cmd(ctrl, TX_SELF_ACLOCK_SIDE_MEM_SET);
	mdss_samsung_send_cmd(ctrl, TX_SELF_ACLOCK_IMAGE);
	mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_DISABLE);
	LCD_ERR("--\n");
}

int ss_self_aclock_set(const void __user *argp)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	struct mdss_dsi_ctrl_pdata *ctrl = samsung_get_dsi_ctrl(vdd);
	struct samsung_display_dtsi_data *dtsi_data = NULL;
	static struct dsi_panel_cmds *pcmds;
	int ret;
	struct self_analog_clock_info sac_info;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (unlikely(!argp)) {
		LCD_ERR("invalid read buffer\n");
		return -EINVAL;
	}

	LCD_ERR("++\n");

	ret = copy_from_user(&sac_info, argp, sizeof(sac_info));
	if (ret) {
		LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
		return ret;
	}

	/* only for revA */
	dtsi_data = &vdd->dtsi_data[0];
	pcmds = &dtsi_data->panel_tx_cmd_list[TX_SELF_ACLOCK_ON][0];

	pcmds->cmds->payload[4] = sac_info.SC_SET_HH;
	pcmds->cmds->payload[5] = sac_info.SC_SET_MM;
	pcmds->cmds->payload[6] = sac_info.SC_SET_SS;
	pcmds->cmds->payload[7] = sac_info.SC_SET_MSS;

	pcmds->cmds->payload[10] = (sac_info.SC_CENTER_X & 0x700) >> 8;
	pcmds->cmds->payload[11] = sac_info.SC_CENTER_X & 0xFF;
	pcmds->cmds->payload[12] = (sac_info.SC_CENTER_Y & 0xF00) >> 8;
	pcmds->cmds->payload[13] = sac_info.SC_CENTER_Y & 0xFF;

	if (sac_info.SC_ROTATE)
		pcmds->cmds->payload[14] |= 1;
	else
		pcmds->cmds->payload[14] &= ~1;

	pcmds->cmds->payload[15] |= sac_info.SC_HMS_LAYER << 4;
	pcmds->cmds->payload[15] |= sac_info.SC_HMS_MASK;

	ss_self_aclock_on(ctrl, 1);

	LCD_ERR("-- \n");

	return 0;
}

void ss_self_dclock_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_dclock_lock);

	if (enable) {
		mdss_samsung_send_cmd(ctrl, TX_SELF_DCLOCK_ON);
	} else {
		mdss_samsung_send_cmd(ctrl, TX_SELF_DCLOCK_OFF);
	}

	mutex_unlock(&vdd->self_disp.vdd_self_dclock_lock);

	LCD_ERR("--\n");

	return;
}

void ss_self_dclock_img_write(struct mdss_dsi_ctrl_pdata *ctrl)
{
	LCD_ERR("++\n");
	mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_ENABLE);
	mdss_samsung_send_cmd(ctrl, TX_SELF_DCLOCK_SIDE_MEM_SET);
	mdss_samsung_send_cmd(ctrl, TX_SELF_DCLOCK_IMAGE);
	mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_DISABLE);
	LCD_ERR("--\n");
}

int ss_self_dclock_set(const void __user *argp)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	struct mdss_dsi_ctrl_pdata *ctrl = samsung_get_dsi_ctrl(vdd);
	struct samsung_display_dtsi_data *dtsi_data = NULL;
	static struct dsi_panel_cmds *pcmds;
	int ret;
	struct self_digital_clock_info sdc_info;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (unlikely(!argp)) {
		LCD_ERR("invalid read buffer\n");
	/*	return -EINVAL;*/
	}

	LCD_ERR("++\n");

	ret = copy_from_user(&sdc_info, argp, sizeof(sdc_info));
	if (ret) {
		LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
		return ret;
	}

	/* only for revA */
	dtsi_data = &vdd->dtsi_data[0];
	pcmds = &dtsi_data->panel_tx_cmd_list[TX_SELF_DCLOCK_ON][0];

	pcmds->cmds->payload[2] |= sdc_info.SC_D_MIN_LOCK_EN << 6;
	pcmds->cmds->payload[2] |= sdc_info.SC_24H_EN << 4;

	pcmds->cmds->payload[3] |= sdc_info.SC_D_EN_HH << 2;
	pcmds->cmds->payload[3] |= sdc_info.SC_D_EN_MM;

	pcmds->cmds->payload[4] = sdc_info.SC_SET_HH;
	pcmds->cmds->payload[5] = sdc_info.SC_SET_MM;
	pcmds->cmds->payload[6] = sdc_info.SC_SET_SS;
	pcmds->cmds->payload[7] = sdc_info.SC_SET_MSS;

	pcmds->cmds->payload[22] = (sdc_info.SC_D_ST_HH_X10 & 0x700) >> 8;
	pcmds->cmds->payload[23] = sdc_info.SC_D_ST_HH_X10 & 0xFF;

	pcmds->cmds->payload[24] = (sdc_info.SC_D_ST_HH_Y10 & 0xF00) >> 8;
	pcmds->cmds->payload[25] = sdc_info.SC_D_ST_HH_Y10 & 0xFF;

	pcmds->cmds->payload[26] = (sdc_info.SC_D_ST_HH_X01 & 0x700) >> 8;
	pcmds->cmds->payload[27] = sdc_info.SC_D_ST_HH_X01 & 0xFF;

	pcmds->cmds->payload[28] = (sdc_info.SC_D_ST_HH_Y01 & 0xF00) >> 8;
	pcmds->cmds->payload[29] = sdc_info.SC_D_ST_HH_Y01 & 0xFF;


	pcmds->cmds->payload[30] = (sdc_info.SC_D_ST_MM_X10 & 0x700) >> 8;
	pcmds->cmds->payload[31] = sdc_info.SC_D_ST_MM_X10 & 0xFF;

	pcmds->cmds->payload[32] = (sdc_info.SC_D_ST_MM_Y10 & 0xF00) >> 8;
	pcmds->cmds->payload[33] = sdc_info.SC_D_ST_MM_Y10 & 0xFF;

	pcmds->cmds->payload[34] = (sdc_info.SC_D_ST_MM_X01 & 0x700) >> 8;
	pcmds->cmds->payload[35] = sdc_info.SC_D_ST_MM_X01 & 0xFF;

	pcmds->cmds->payload[36] = (sdc_info.SC_D_ST_MM_Y01 & 0xF00) >> 8;
	pcmds->cmds->payload[37] = sdc_info.SC_D_ST_MM_Y01 & 0xFF;

	pcmds->cmds->payload[38] = (sdc_info.SC_D_IMG_WIDTH & 0x700) >> 8;
	pcmds->cmds->payload[39] = sdc_info.SC_D_IMG_WIDTH & 0xFF;

	pcmds->cmds->payload[40] = (sdc_info.SC_D_IMG_HEIGHT & 0xF00) >> 8;
	pcmds->cmds->payload[41] = sdc_info.SC_D_IMG_HEIGHT & 0xFF;

	ss_self_dclock_on(ctrl, 1);

	LCD_ERR("--\n");

	return 0;
}

void ss_self_blinking_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_dclock_lock);

	if (enable)
		mdss_samsung_send_cmd(ctrl, TX_SELF_DCLOCK_BLINKING_ON);
	else
		mdss_samsung_send_cmd(ctrl, TX_SELF_DCLOCK_BLINKING_OFF);

	mutex_unlock(&vdd->self_disp.vdd_self_dclock_lock);

	LCD_ERR("--\n");

	return;
}

int ss_self_blinking_set(const void __user *argp)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	struct mdss_dsi_ctrl_pdata *ctrl = samsung_get_dsi_ctrl(vdd);
	struct samsung_display_dtsi_data *dtsi_data = NULL;
	static struct dsi_panel_cmds *pcmds;
	int ret;
	struct self_blinking_info sb_info;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (unlikely(!argp)) {
		LCD_ERR("invalid read buffer\n");
	/*	return -EINVAL;*/
	}

	LCD_ERR("++\n");

	ret = copy_from_user(&sb_info, argp, sizeof(sb_info));
	if (ret) {
		LCD_ERR("fail to copy_from_user.. (%d) \n", ret);
		return ret;
	}

	/* only for revA */
	dtsi_data = &vdd->dtsi_data[0];
	pcmds = &dtsi_data->panel_tx_cmd_list[TX_SELF_DCLOCK_BLINKING_ON][0];

	pcmds->cmds->payload[1] |= sb_info.SD_BLC1_EN << 7;
	pcmds->cmds->payload[1] |= sb_info.SD_BLC2_EN << 6;

	pcmds->cmds->payload[7] |= sb_info.SD_BLEND_RATE & 0x07;

	pcmds->cmds->payload[9] = sb_info.SD_RADIUS1;
	pcmds->cmds->payload[10] = sb_info.SD_RADIUS2;

	pcmds->cmds->payload[11] = (sb_info.SD_LINE_COLOR & 0xFF0000) >> 16;
	pcmds->cmds->payload[12] = (sb_info.SD_LINE_COLOR & 0xFF00) >> 8;
	pcmds->cmds->payload[13] = sb_info.SD_LINE_COLOR & 0xFF;

	pcmds->cmds->payload[14] = (sb_info.SD_LINE2_COLOR & 0xFF0000) >> 16;
	pcmds->cmds->payload[15] = (sb_info.SD_LINE2_COLOR & 0xFF00) >> 8;
	pcmds->cmds->payload[16] = sb_info.SD_LINE2_COLOR & 0xFF;

	pcmds->cmds->payload[17] = (sb_info.SD_BB_PT_X1 & 0x700) >> 8;
	pcmds->cmds->payload[18] = sb_info.SD_BB_PT_X1 & 0xFF;

	pcmds->cmds->payload[19] = (sb_info.SD_BB_PT_Y1 & 0xF00) >> 8;
	pcmds->cmds->payload[20] = sb_info.SD_BB_PT_Y1 & 0xFF;

	pcmds->cmds->payload[21] = (sb_info.SD_BB_PT_X2 & 0x700) >> 8;
	pcmds->cmds->payload[22] = sb_info.SD_BB_PT_X2 & 0xFF;

	pcmds->cmds->payload[23] = (sb_info.SD_BB_PT_Y2 & 0xF00) >> 8;
	pcmds->cmds->payload[24] = sb_info.SD_BB_PT_Y2 & 0xFF;

	ss_self_blinking_on(ctrl, 1);

	LCD_ERR("- \n");

	return 0;
}

#define DDI_LOG_SIZE 50
int read_ddi_log(struct samsung_display_driver_data *vdd)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	int loop;
	unsigned char ddi_log[DDI_LOG_SIZE] = {0,};
	char print_buf[400] = {0,};
	int print_pos = 0;

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (!ctrl) {
		LCD_ERR("dsi_ctrl is null.. \n");
		return 0;
	}

	LCD_ERR("========== DDI Command History Start ==========\n");

	if (get_panel_rx_cmds(ctrl, RX_LDI_LOG)->cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl, get_panel_rx_cmds(ctrl, RX_LDI_LOG),
			ddi_log, LEVEL1_KEY);

		for (loop = 0 ; loop < DDI_LOG_SIZE ; loop++) {
			print_pos += snprintf(print_buf + print_pos, 400, "0x%x ", ddi_log[loop]);
		}
		LCD_ERR("%s\n", print_buf);
		LCD_ERR("========== DDI Command History Finish ==========\n");

		mdss_samsung_send_cmd(ctrl, TX_LDI_LOG_DISABLE);
	} else
		LCD_ERR("No LDI LOG cmds..\n");

	return 0;
}

int panic_on;

void ss_self_mask_on(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
/*	char buf[2];*/

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR("self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return;
	}

	LCD_ERR("++ (%d)\n", enable);

	mutex_lock(&vdd->self_disp.vdd_self_mask_lock);

	if (enable) {
		mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_ENABLE);
		mdss_samsung_send_cmd(ctrl, TX_SELF_MASK_SIDE_MEM_SET);
		mdss_samsung_send_cmd(ctrl, TX_SELF_MASK_IMAGE);
		mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_DISABLE);
		mdss_samsung_send_cmd(ctrl, TX_SELF_MASK_ON);

#ifdef SELF_DISPLAY_DEBUG
		mdss_samsung_panel_data_read(ctrl, get_panel_rx_cmds(ctrl, RX_LDI_ERROR),
				buf, LEVEL1_KEY);

		if (ss_self_disp_debug(ctrl) == 0) { /*check checksum*/
			mdss_samsung_send_cmd(ctrl, TX_LDI_LOG_DISABLE);
			read_ddi_log(vdd);
			mdss_samsung_panel_data_read(ctrl, get_panel_rx_cmds(ctrl, RX_LDI_ERROR),
				buf, LEVEL1_KEY);
			panic_on = 1;
		}
#endif
	} else {
		mdss_samsung_send_cmd(ctrl, TX_SELF_MASK_OFF);

#ifdef SELF_DISPLAY_DEBUG
		if (panic_on)
			panic("Self mask side memory checksum fail!!!");
#endif
	}

	vdd->self_disp.operation[FLAG_SELF_MASK].on = enable;

	mutex_unlock(&vdd->self_disp.vdd_self_mask_lock);

	LCD_ERR("--\n");

	return;
}

int ss_self_sync_off(const void __user *argp)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	struct mdss_dsi_ctrl_pdata *ctrl = samsung_get_dsi_ctrl(vdd);
	enum self_display_sync_off sync_off;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	if (unlikely(!argp)) {
		LCD_ERR("invalid read buffer\n");
		return -EINVAL;
	}

	LCD_ERR("++\n");

	ret = copy_from_user(&sync_off, argp, sizeof(sync_off));
	if (ret) {
		LCD_ERR("fail to copy_from_user.. (%d)\n", ret);
		return ret;
	}

	switch (sync_off) {
	case SELF_MOVE_SYNC_OFF:
		mdss_samsung_send_cmd(ctrl, TX_SELF_MOVE_2C_SYNC_OFF);
		break;
	case SELF_ICON_GRID_SYNC_OFF:
		mdss_samsung_send_cmd(ctrl, TX_SELF_ICON_GRID_2C_SYNC_OFF);
		break;
	case SELF_CLOCK_SYNC_OFF:
		mdss_samsung_send_cmd(ctrl, TX_SELF_CLOCK_2C_SYNC_OFF);
		break;
	default:
		LCD_ERR("invalid sync_off : %u\n", sync_off);
		break;
	}

	LCD_ERR("--\n");

	return ret;
}

int ss_self_disp_debug(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	char buf[32];

	if (get_panel_rx_cmds(ctrl, RX_SELF_DISP_DEBUG)->cmd_cnt) {
		mdss_samsung_panel_data_read(ctrl, get_panel_rx_cmds(ctrl, RX_SELF_DISP_DEBUG),
			buf, LEVEL1_KEY);

		vdd->self_disp.debug.SI_X_O = ((buf[14] & 0x07) << 8);
		vdd->self_disp.debug.SI_X_O |= (buf[15] & 0xFF);

		vdd->self_disp.debug.SI_Y_O = ((buf[16] & 0x0F) << 8);
		vdd->self_disp.debug.SI_Y_O |= (buf[17] & 0xFF);

		vdd->self_disp.debug.SM_SUM_O = ((buf[6] & 0xFF) << 24);
		vdd->self_disp.debug.SM_SUM_O |= ((buf[7] & 0xFF) << 16);
		vdd->self_disp.debug.SM_SUM_O |= ((buf[8] & 0xFF) << 8);
		vdd->self_disp.debug.SM_SUM_O |= (buf[9] & 0xFF);

		vdd->self_disp.debug.MEM_SUM_O = ((buf[10] & 0xFF) << 24);
		vdd->self_disp.debug.MEM_SUM_O |= ((buf[11] & 0xFF) << 16);
		vdd->self_disp.debug.MEM_SUM_O |= ((buf[12] & 0xFF) << 8);
		vdd->self_disp.debug.MEM_SUM_O |= (buf[13] & 0xFF);

		LCD_DEBUG("SI_X_O(%u) SI_Y_O(%u) MEM_SUM_O(%u) SM_SUM_O(%u)\n",
			vdd->self_disp.debug.SI_X_O,
			vdd->self_disp.debug.SI_Y_O,
			vdd->self_disp.debug.MEM_SUM_O,
			vdd->self_disp.debug.SM_SUM_O);

		if (vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum != vdd->self_disp.debug.SM_SUM_O) {
			LCD_ERR("Self mask side memory fail! (%x) != %x\n",
				vdd->self_disp.debug.SM_SUM_O, vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum);
			return 0;
		}

	} else {
		LCD_ERR("DSI%d no self_disp_debug cmds", ctrl->ndx);
		return 1;
	}

	return 1;
}
/*
 * ss_self_display_ioctl() : get ioctl from aod framework.
 *                           set self display related registers.
 */
static long ss_self_display_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	void __user *argp = (void __user *)arg;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}
/*
	if (vdd->vdd_blank_mode[DISPLAY_1] == FB_BLANK_UNBLANK) {
		LCD_ERR("self_display IOCTL is blocked in Normal On(Unblank)\n");
		return -EPERM;
	}
*/
	if ((_IOC_TYPE(cmd) != SELF_DISPLAY_IOCTL_MAGIC) || (_IOC_NR(cmd) >= IOCTL_SELF_MAX)) {
		LCD_ERR("TYPE(%u) NR(%u) is wrong..\n", _IOC_TYPE(cmd), _IOC_NR(cmd));
		return -EINVAL;
	}

	LCD_INFO("cmd = %u\n", cmd);

	switch (cmd) {
	case IOCTL_SELF_MOVE:
		ret = ss_self_move_set(argp);
		break;
	case IOCTL_SELF_MASK:
		/* write Setting and image to side memory every panel on time.*/
		break;
	case IOCTL_SELF_ICON_GRID:
		ret = ss_self_icon_grid_set(argp);
		break;
	case IOCTL_SELF_ANALOG_CLOCK:
		ret = ss_self_aclock_set(argp);
		break;
	case IOCTL_SELF_DIGITAL_CLOCK:
		ret = ss_self_dclock_set(argp);
		break;
	case IOCTL_SELF_BLINGKING:
		ret = ss_self_blinking_set(argp);
		break;
	case IOCTL_SELF_SYNC_OFF:
		ret = ss_self_sync_off(argp);
		break;
	default:
		LCD_ERR("invalid cmd : %u\n", cmd);
		break;
	}

	return ret;
}

/*
 * ss_self_display_write() : get image data from aod framework.
 *                           prepare for dsi_panel_cmds.
 */
static ssize_t ss_self_display_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	u32 op;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	if (unlikely(!buf)) {
		LCD_ERR("invalid read buffer\n");
		return -EINVAL;
	}

	/*
	 * get 1byte flas to distinguish what operation is passing
	 */
	ret = simple_write_to_buffer(&op, 1, ppos, buf, count);
	if (unlikely(ret < 0)) {
		LCD_ERR("failed to simple_write_to_buffer (flag \n");
		return ret;
	}

	LCD_INFO("flag (%d \n", op);

	if (op >= FLAG_SELF_DISP_MAX) {
		LCD_ERR("invalid data flag : %d\n", op);
		return -EINVAL;
	}

	if (count != vdd->self_disp.operation[op].img_size) {
		LCD_ERR("img_size (%d) size is wrong.. count (%d)\n",
			vdd->self_disp.operation[op].img_size, (int)count);
		return -EINVAL;
	}

	vdd->self_disp.operation[op].wpos = *ppos;
	vdd->self_disp.operation[op].wsize = count;
	ret = simple_write_to_buffer(vdd->self_disp.operation[op].img_buf,
					vdd->self_disp.operation[op].img_size, ppos+1, buf, count);
	if (unlikely(ret < 0)) {
		LCD_ERR("failed to simple_write_to_buffer (data)\n");
		return ret;
	}

	switch (op) {
	case FLAG_SELF_MOVE:
		/* MOVE has no image data..*/
		break;
	case FLAG_SELF_MASK:
		make_self_dispaly_img_cmds(TX_SELF_MASK_IMAGE, op);
		vdd->self_disp.operation[op].on = 1;
		break;
	case FLAG_SELF_ICON:
		make_self_dispaly_img_cmds(TX_SELF_ICON_IMAGE, op);
		vdd->self_disp.operation[op].on = 1;
		break;
	case FLAG_SELF_GRID:
		/* GRID has no image data..*/
		break;
	case FLAG_SELF_ACLK:
		make_self_dispaly_img_cmds(TX_SELF_ACLOCK_IMAGE, op);
		vdd->self_disp.operation[op].on = 1;
		vdd->self_disp.operation[FLAG_SELF_DCLK].on = 0;
		break;
	case FLAG_SELF_DCLK:
		make_self_dispaly_img_cmds(TX_SELF_DCLOCK_IMAGE, op);
		vdd->self_disp.operation[op].on = 1;
		vdd->self_disp.operation[FLAG_SELF_ACLK].on = 0;
		break;
	default:
		LCD_ERR("invalid data flag %d\n", op);
		break;
	}


	return ret;
}

static int ss_self_display_open(struct inode *inode, struct file *file)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	vdd->self_disp.file_open = 1;

	LCD_DEBUG("[open]\n");

	return 0;
}

static int ss_self_display_release(struct inode *inode, struct file *file)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	vdd->self_disp.file_open = 0;

	LCD_DEBUG("[release]\n");

	return 0;
}

static const struct file_operations self_display_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ss_self_display_ioctl,
	.open = ss_self_display_open,
	.release = ss_self_display_release,
	.write = ss_self_display_write,
};

int ss_self_display_init(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	LCD_INFO("++\n");

	mutex_init(&vdd->self_disp.vdd_self_move_lock);
	mutex_init(&vdd->self_disp.vdd_self_mask_lock);
	mutex_init(&vdd->self_disp.vdd_self_aclock_lock);
	mutex_init(&vdd->self_disp.vdd_self_dclock_lock);
	mutex_init(&vdd->self_disp.vdd_self_icon_grid_lock);

	vdd->self_disp.dev.minor = MISC_DYNAMIC_MINOR;
	vdd->self_disp.dev.name = "self_display";
	vdd->self_disp.dev.fops = &self_display_fops;
	vdd->self_disp.dev.parent = NULL;

	ret = misc_register(&vdd->self_disp.dev);
	if (ret) {
		LCD_ERR("failed to register driver : %d\n", ret);
		return ret;
	}

	vdd->self_disp.is_support = 1;

	LCD_INFO("Success to register self_disp device..(%d)\n", ret);

	return ret;
}
