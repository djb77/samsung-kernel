/*
 * =================================================================
 *
 *	Description:  samsung display sysfs common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2015, Samsung Electronics. All rights reserved.

*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/
#include "ss_dsi_panel_sysfs.h"

extern struct kset *devices_kset;

#define MAX_FILE_NAME 128
#define TUNING_FILE_PATH "/sdcard/"
static char tuning_file[MAX_FILE_NAME];

#if 0
/* 
 * Do not use below code but only for Image Quality Debug in Developing Precess.
 * Do not comment in this code Because there are contained vulnerability.
 */
static char char_to_dec(char data1, char data2)
{
	char dec;

	dec = 0;

	if (data1 >= 'a') {
		data1 -= 'a';
		data1 += 10;
	} else if (data1 >= 'A') {
		data1 -= 'A';
		data1 += 10;
	} else
		data1 -= '0';

	dec = data1 << 4;

	if (data2 >= 'a') {
		data2 -= 'a';
		data2 += 10;
	} else if (data2 >= 'A') {
		data2 -= 'A';
		data2 += 10;
	} else
		data2 -= '0';

	dec |= data2;

	return dec;
}

static void sending_tune_cmd(struct device *dev, char *src, int len)
{
	int i;
	int data_pos;
	int cmd_step;
	int cmd_pos;
	int cmd_cnt = 0;

	char *mdnie_tuning[MDNIE_TUNE_MAX_SIZE];
	struct dsi_cmd_desc *mdnie_tune_cmd;

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return;
	}

	for (i = 0; i < MDNIE_TUNE_MAX_SIZE; i++) {
		if (vdd->mdnie_tune_size[i]) {
			mdnie_tuning[i] = kzalloc(sizeof(char) * vdd->mdnie_tune_size[i], GFP_KERNEL);
			cmd_cnt++;
			LCD_INFO("mdnie_tune_size%d (%d) \n", i+1, vdd->mdnie_tune_size[i]);
		}
	}

	LCD_INFO("cmd_cnt : %d\n", cmd_cnt);
	if (!cmd_cnt) {
		LCD_ERR("No tuning cmds..\n");
		return;
	}

	mdnie_tune_cmd = kzalloc(cmd_cnt * sizeof(struct dsi_cmd_desc), GFP_KERNEL);

	cmd_step = 0;
	cmd_pos = 0;

	for (data_pos = 0; data_pos < len;) {
		if (*(src + data_pos) == '0') {
			if (*(src + data_pos + 1) == 'x') {
				if (!cmd_step)
					mdnie_tuning[0][cmd_pos] = char_to_dec(*(src + data_pos + 2), *(src + data_pos + 3));
				else if (cmd_step == 1)
					mdnie_tuning[1][cmd_pos] = char_to_dec(*(src + data_pos + 2), *(src + data_pos + 3));
				else if (cmd_step == 2 && vdd->mdnie_tuning_enable_tft)
					mdnie_tuning[2][cmd_pos] = char_to_dec(*(src + data_pos + 2), *(src + data_pos + 3));
				else if (cmd_step == 3 && vdd->mdnie_tuning_enable_tft)
					mdnie_tuning[3][cmd_pos] = char_to_dec(*(src + data_pos + 2), *(src + data_pos + 3));
				else if (cmd_step == 4 && vdd->mdnie_tuning_enable_tft)
					mdnie_tuning[4][cmd_pos] = char_to_dec(*(src + data_pos + 2), *(src + data_pos + 3));
				else if (cmd_step == 5 && vdd->mdnie_tuning_enable_tft)
					mdnie_tuning[5][cmd_pos] = char_to_dec(*(src + data_pos + 2), *(src + data_pos + 3));
				else
					mdnie_tuning[2][cmd_pos] = char_to_dec(*(src + data_pos + 2), *(src + data_pos + 3));

				data_pos += 3;
				cmd_pos++;

				if (cmd_pos == vdd->mdnie_tune_size[0] && !cmd_step) {
					cmd_pos = 0;
					cmd_step = 1;
				} else if ((cmd_pos == vdd->mdnie_tune_size[1]) && (cmd_step == 1)) {
					cmd_pos = 0;
					cmd_step = 2;
				} else if ((cmd_pos == vdd->mdnie_tune_size[2]) && (cmd_step == 2) && vdd->mdnie_tuning_enable_tft) {
					cmd_pos = 0;
					cmd_step = 3;
				} else if ((cmd_pos == vdd->mdnie_tune_size[3]) && (cmd_step == 3) && vdd->mdnie_tuning_enable_tft) {
					cmd_pos = 0;
					cmd_step = 4;
				} else if ((cmd_pos == vdd->mdnie_tune_size[4]) && (cmd_step == 4) && vdd->mdnie_tuning_enable_tft) {
					cmd_pos = 0;
					cmd_step = 5;
				}
			} else
				data_pos++;
		} else {
			data_pos++;
		}
	}

	for (i = 0; i < cmd_cnt; i++) {
		mdnie_tune_cmd[i].dchdr.dtype = DTYPE_DCS_LWRITE;
		mdnie_tune_cmd[i].dchdr.last = 1;
		mdnie_tune_cmd[i].dchdr.dlen = vdd->mdnie_tune_size[i];
		mdnie_tune_cmd[i].payload = mdnie_tuning[i];

		printk(KERN_ERR "mdnie_tuning%d (%d)\n", i, vdd->mdnie_tune_size[i]);
		for (data_pos = 0; data_pos < vdd->mdnie_tune_size[i] ; data_pos++)
			printk(KERN_ERR "0x%x \n", mdnie_tuning[i][data_pos]);
	}

	if ((vdd->ctrl_dsi[DSI_CTRL_0]->cmd_sync_wait_broadcast)
		&& (vdd->ctrl_dsi[DSI_CTRL_1]->cmd_sync_wait_trigger)) { /* Dual DSI & dsi 1 trigger */
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_1], TX_LEVEL1_KEY_ENABLE);
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_1], TX_LEVEL2_KEY_ENABLE);

		/* Set default link_stats as DSI_HS_MODE for mdnie tune data */
		vdd->dtsi_data[DSI_CTRL_1].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].link_state = DSI_HS_MODE;
		vdd->dtsi_data[DSI_CTRL_1].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmds = mdnie_tune_cmd;
		vdd->dtsi_data[DSI_CTRL_1].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmd_cnt = cmd_cnt;

		mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_1], TX_MDNIE_TUNE);

		mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_1], TX_LEVEL1_KEY_DISABLE);
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_1], TX_LEVEL2_KEY_DISABLE);
	} else { /* Single DSI, dsi 0 trigger */
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_0], TX_LEVEL1_KEY_ENABLE);

		/* Set default link_stats as DSI_HS_MODE for mdnie tune data */
		vdd->dtsi_data[DSI_CTRL_0].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].link_state = DSI_HS_MODE;
		vdd->dtsi_data[DSI_CTRL_0].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmds = mdnie_tune_cmd;
		vdd->dtsi_data[DSI_CTRL_0].panel_tx_cmd_list[TX_MDNIE_TUNE][vdd->panel_revision].cmd_cnt = cmd_cnt;

		mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_0], TX_MDNIE_TUNE);

		mdss_samsung_send_cmd(vdd->ctrl_dsi[DSI_CTRL_0], TX_LEVEL1_KEY_DISABLE);
	}

	for (i = 0; i < cmd_cnt; i++)
		kfree(mdnie_tuning[i]);
}


static void load_tuning_file(struct device *dev, char *filename)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs;

	LCD_INFO("called loading file name : [%s]\n",
	       filename);

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR "%s File open failed\n", __func__);
		goto err;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	LCD_INFO("Loading File Size : %ld(bytes)", l);

	dp = kmalloc(l + 10, GFP_KERNEL);
	if (dp == NULL) {
		LCD_INFO("Can't not alloc memory for tuning file load\n");
		filp_close(filp, current->files);
		goto err;
	}
	pos = 0;
	memset(dp, 0, l);

	LCD_INFO("before vfs_read()\n");
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	LCD_INFO("after vfs_read()\n");

	if (ret != l) {
		LCD_INFO("vfs_read() filed ret : %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		goto err;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	sending_tune_cmd(dev, dp, l);

	kfree(dp);

	return;
err:
	set_fs(fs);
}
#endif

static ssize_t tuning_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = snprintf(buf, MAX_FILE_NAME, "Tunned File Name : %s\n", tuning_file);
	return ret;
}

static ssize_t tuning_store(struct device *dev,
			    struct device_attribute *attr, const char *buf,
			    size_t size)
{
/* 
 * Do not use below code but only for Image Quality Debug in Developing Precess.
 * Do not comment in this code Because there are contained vulnerability.
 */
/*
	char *pt;

	if (buf == NULL || strchr(buf, '.') || strchr(buf, '/'))
		return size;

	memset(tuning_file, 0, sizeof(tuning_file));
	snprintf(tuning_file, MAX_FILE_NAME, "%s%s", TUNING_FILE_PATH, buf);

	pt = tuning_file;

	while (*pt) {
		if (*pt == '\r' || *pt == '\n') {
			*pt = 0;
			break;
		}
		pt++;
	}

	LCD_INFO("%s\n", tuning_file);

	load_tuning_file(dev, tuning_file);
*/
	return size;
}

static ssize_t mdss_samsung_disp_cell_id_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	static int string_size = 50;
	char temp[string_size];
	int *cell_id;
	int ndx;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	ndx = display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]);

	cell_id = &vdd->cell_id_dsi[ndx][0];

	/*
	*	STANDARD FORMAT (Total is 11Byte)
	*	MAX_CELL_ID : 11Byte
	*	7byte(cell_id) + 2byte(Mdnie x_postion) + 2byte(Mdnie y_postion)
	*/

	snprintf((char *)temp, sizeof(temp),
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		cell_id[0], cell_id[1], cell_id[2], cell_id[3], cell_id[4],
		cell_id[5], cell_id[6],
		(vdd->mdnie_x[ndx] & 0xFF00) >> 8,
		vdd->mdnie_x[ndx] & 0xFF,
		(vdd->mdnie_y[ndx] & 0xFF00) >> 8,
		vdd->mdnie_y[ndx] & 0xFF);

	strlcat(buf, temp, string_size);

	LCD_INFO("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		cell_id[0], cell_id[1], cell_id[2], cell_id[3], cell_id[4],
		cell_id[5], cell_id[6],
		(vdd->mdnie_x[ndx] & 0xFF00) >> 8,
		vdd->mdnie_x[ndx] & 0xFF,
		(vdd->mdnie_y[ndx] & 0xFF00) >> 8,
		vdd->mdnie_y[ndx] & 0xFF);

	return strnlen(buf, string_size);
}

static ssize_t mdss_samsung_disp_octa_id_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	static int string_size = 50;
	char temp[string_size];
	int *octa_id;
	int ndx;
	int site, rework, poc, max_brightness;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	ndx = display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]);

	octa_id = &vdd->octa_id_dsi[ndx][0];

	site = octa_id[0] & 0xf0;
	site >>= 4;
	rework = octa_id[0] & 0x0f;
	poc = octa_id[1] & 0x0f;
	max_brightness = octa_id[2] * 256 + octa_id[3];

	snprintf((char *)temp, sizeof(temp),
			"%d%d%d%02x%02x%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
		site, rework, poc, octa_id[2], octa_id[3],
		octa_id[4] != 0 ? octa_id[4] : '0',
		octa_id[5] != 0 ? octa_id[5] : '0',
		octa_id[6] != 0 ? octa_id[6] : '0',
		octa_id[7] != 0 ? octa_id[7] : '0',
		octa_id[8] != 0 ? octa_id[8] : '0',
		octa_id[9] != 0 ? octa_id[9] : '0',
		octa_id[10] != 0 ? octa_id[10] : '0',
		octa_id[11] != 0 ? octa_id[11] : '0',
		octa_id[12] != 0 ? octa_id[12] : '0',
		octa_id[13] != 0 ? octa_id[13] : '0',
		octa_id[14] != 0 ? octa_id[14] : '0',
		octa_id[15] != 0 ? octa_id[15] : '0',
		octa_id[16] != 0 ? octa_id[16] : '0',
		octa_id[17] != 0 ? octa_id[17] : '0',
		octa_id[18] != 0 ? octa_id[18] : '0',
		octa_id[19] != 0 ? octa_id[19] : '0');

	strlcat(buf, temp, string_size);

	LCD_INFO("poc(%d)\n", poc);

	LCD_INFO("%d%d%d%02x%02x%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
		site, rework, poc, octa_id[2], octa_id[3],
		octa_id[4] != 0 ? octa_id[4] : '0',
		octa_id[5] != 0 ? octa_id[5] : '0',
		octa_id[6] != 0 ? octa_id[6] : '0',
		octa_id[7] != 0 ? octa_id[7] : '0',
		octa_id[8] != 0 ? octa_id[8] : '0',
		octa_id[9] != 0 ? octa_id[9] : '0',
		octa_id[10] != 0 ? octa_id[10] : '0',
		octa_id[11] != 0 ? octa_id[11] : '0',
		octa_id[12] != 0 ? octa_id[12] : '0',
		octa_id[13] != 0 ? octa_id[13] : '0',
		octa_id[14] != 0 ? octa_id[14] : '0',
		octa_id[15] != 0 ? octa_id[15] : '0',
		octa_id[16] != 0 ? octa_id[16] : '0',
		octa_id[17] != 0 ? octa_id[17] : '0',
		octa_id[18] != 0 ? octa_id[18] : '0',
		octa_id[19] != 0 ? octa_id[19] : '0');

	return strnlen(buf, string_size);
}

static ssize_t mdss_samsung_disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	static int string_size = 100;
	char temp[string_size];
	int ndx;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	ndx = display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]);

	if (vdd->dtsi_data[ndx].tft_common_support && vdd->dtsi_data[ndx].tft_module_name) {
		if (vdd->dtsi_data[ndx].panel_vendor)
			snprintf(temp, 20, "%s_%s\n", vdd->dtsi_data[ndx].panel_vendor, vdd->dtsi_data[ndx].tft_module_name);
		else
			snprintf(temp, 20, "SDC_%s\n", vdd->dtsi_data[ndx].tft_module_name);
	} else if (mdss_panel_attached(ndx)) {
		if (vdd->dtsi_data[ndx].panel_vendor)
			snprintf(temp, 20, "%s_%06x\n", vdd->dtsi_data[ndx].panel_vendor, vdd->manufacture_id_dsi[ndx]);
		else
			snprintf(temp, 20, "SDC_%06x\n", vdd->manufacture_id_dsi[ndx]);
	} else {
		LCD_INFO("no manufacture id\n");
		snprintf(temp, 20, "NULL\n");
	}

	strlcat(buf, temp, string_size);

	return strnlen(buf, string_size);
}

static ssize_t mdss_samsung_disp_windowtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static int string_size = 15;
	char temp[string_size];
	int id, id1, id2, id3;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	/* If LCD_ID is needed before splash done(Multi Color Boot Animation), we should get LCD_ID form LK */
	if (vdd->manufacture_id_dsi[display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0])] == PBA_ID)
		id = get_lcd_attached("GET");
	else
		id = vdd->manufacture_id_dsi[display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0])];

	id1 = (id & 0x00FF0000) >> 16;
	id2 = (id & 0x0000FF00) >> 8;
	id3 = id & 0xFF;

	LCD_INFO("ndx : %d %02x %02x %02x\n",
		display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]), id1, id2, id3);

	snprintf(temp, sizeof(temp), "%02x %02x %02x\n", id1, id2, id3);

	strlcat(buf, temp, string_size);

	return strnlen(buf, string_size);
}

static ssize_t mdss_samsung_disp_manufacture_date_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static int string_size = 30;
	char temp[string_size];
	int date;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	date = vdd->manufacture_date_dsi[display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0])];
	snprintf((char *)temp, sizeof(temp), "manufacture date : %d\n", date);

	strlcat(buf, temp, string_size);

	LCD_INFO("manufacture date : %d\n", date);

	return strnlen(buf, string_size);
}

static ssize_t mdss_samsung_disp_manufacture_code_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static int string_size = 30;
	char temp[string_size];
	int *ddi_id;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	ddi_id = &vdd->ddi_id_dsi[display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0])][0];

	snprintf((char *)temp, sizeof(temp), "%02x%02x%02x%02x%02x\n",
		ddi_id[0], ddi_id[1], ddi_id[2], ddi_id[3], ddi_id[4]);

	strlcat(buf, temp, string_size);

	LCD_INFO("%02x %02x %02x %02x %02x\n",
		ddi_id[0], ddi_id[1], ddi_id[2], ddi_id[3], ddi_id[4]);

	return strnlen(buf, string_size);
}

static ssize_t mdss_samsung_disp_acl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return rc;
	}

	rc = snprintf((char *)buf, sizeof(vdd->acl_status), "%d\n", vdd->acl_status);

	LCD_INFO("acl status: %d\n", *buf);

	return rc;
}

static ssize_t mdss_samsung_disp_acl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int acl_set = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_panel_data *pdata;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		return size;
	}

	if (sysfs_streq(buf, "1"))
		acl_set = true;
	else if (sysfs_streq(buf, "0"))
		acl_set = false;
	else
		LCD_INFO("Invalid argument!!");

	LCD_INFO("(%d)\n", acl_set);

	pdata = &vdd->ctrl_dsi[DISPLAY_1]->panel_data;
	mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);

	if (acl_set && !vdd->acl_status) {
		vdd->acl_status = acl_set;
		pdata->set_backlight(pdata, vdd->bl_level);
	} else if (!acl_set && vdd->acl_status) {
		vdd->acl_status = acl_set;
		pdata->set_backlight(pdata, vdd->bl_level);
	} else {
		vdd->acl_status = acl_set;
		LCD_INFO("skip acl update!! acl %d", vdd->acl_status);
	}

	mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);

	return size;
}

static ssize_t mdss_samsung_disp_siop_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return rc;
	}

	rc = snprintf((char *)buf, sizeof(vdd->siop_status), "%d\n", vdd->siop_status);

	LCD_INFO("siop status: %d\n", *buf);

	return rc;
}

static ssize_t mdss_samsung_disp_siop_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int siop_set = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_panel_data *pdata;


	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		return size;
	}

	pdata = &vdd->ctrl_dsi[DISPLAY_1]->panel_data;

	if (sysfs_streq(buf, "1"))
		siop_set = true;
	else if (sysfs_streq(buf, "0"))
		siop_set = false;
	else
		LCD_INFO("Invalid argument!!");

	LCD_INFO("(%d)\n", siop_set);

	mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);

	if (siop_set && !vdd->siop_status) {
		vdd->siop_status = siop_set;
		pdata->set_backlight(pdata, vdd->bl_level);
	} else if (!siop_set && vdd->siop_status) {
		vdd->siop_status = siop_set;
		pdata->set_backlight(pdata, vdd->bl_level);
	} else {
		vdd->siop_status = siop_set;
		LCD_INFO("skip siop update!! acl %d", vdd->acl_status);
	}

	mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);

	return size;
}

static ssize_t mdss_samsung_aid_log_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;
	int loop = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return rc;
	}

	for (loop = 0; loop < vdd->support_panel_max; loop++) {
		if (vdd->smart_dimming_dsi[loop] && vdd->smart_dimming_dsi[loop]->print_aid_log)
			vdd->smart_dimming_dsi[loop]->print_aid_log(vdd->smart_dimming_dsi[loop]);
		else
			LCD_ERR("DSI%d smart dimming is not loaded\n", loop);
	}

	if (vdd->dtsi_data[0].hmt_enabled) {
		for (loop = 0; loop < vdd->support_panel_max; loop++) {
			if (vdd->smart_dimming_dsi_hmt[loop] && vdd->smart_dimming_dsi_hmt[loop]->print_aid_log)
				vdd->smart_dimming_dsi_hmt[loop]->print_aid_log(vdd->smart_dimming_dsi_hmt[loop]);
			else
				LCD_ERR("DSI%d smart dimming hmt is not loaded\n", loop);
		}
	}

	return rc;
}

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
static ssize_t mdss_samsung_disp_brightness_step(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	int ndx;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return rc;
	}

	ndx = display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]);

	rc = snprintf((char *)buf, 20, "%d\n", vdd->dtsi_data[ndx].candela_map_table[vdd->panel_revision].tab_size);

	LCD_INFO("brightness_step : %d", vdd->dtsi_data[ndx].candela_map_table[vdd->panel_revision].tab_size);

	return rc;
}

static ssize_t mdss_samsung_disp_color_weakness_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	int ndx;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return rc;
	}

	ndx = display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]);

	rc = snprintf((char *)buf, 20, "%d %d\n", vdd->color_weakness_mode, vdd->color_weakness_level);

	LCD_INFO("color weakness : %d %d", vdd->color_weakness_mode, vdd->color_weakness_level);

	return rc;
}

static ssize_t mdss_samsung_disp_color_weakness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_panel_data *pdata;
	struct mdss_dsi_ctrl_pdata *ctrl;
	unsigned int mode, level, value = 0;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		goto end;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("no ctrl");
		return size;
	}

	pdata = &vdd->ctrl_dsi[DISPLAY_1]->panel_data;

	sscanf(buf, "%x %x", &mode, &level);

	if (mode >= 0 && mode <= 9)
		vdd->color_weakness_mode = mode;
	else
		LCD_ERR("mode (%x) is not correct !\n", mode);

	if (level >= 0 && level <= 9)
		vdd->color_weakness_level = level;
	else
		LCD_ERR("level (%x) is not correct !\n", level);

	value = level << 4 | mode;

	LCD_ERR("level (%x) mode (%x) value (%x)\n", level, mode, value);

	get_panel_tx_cmds(ctrl, TX_COLOR_WEAKNESS_ENABLE)->cmds[1].payload[1] = value;

	if (mode)
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], TX_COLOR_WEAKNESS_ENABLE);
	else
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], TX_COLOR_WEAKNESS_DISABLE);

end:
	return size;
}

#if defined(CONFIG_BLIC_LM3632A)

#define get_bit(value, shift, width)	((value >> shift) & (GENMASK(width - 1, 0)))

static ssize_t mdss_brightness_12bit_pwm_tuning_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_dsi_ctrl_pdata *ctrl;
	struct dsi_cmd_desc *packet = NULL;
	struct dsi_panel_cmds *brightness_cmds = NULL;
	unsigned int candela_level = 0;
	int cmd_cnt = 0;
	int ndx;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		return size;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("no ctrl");
		return size;
	}
	
	brightness_cmds = get_panel_tx_cmds(ctrl, TX_BRIGHT_CTRL);
	
	ndx = display_ndx_check(ctrl);
	
	packet = &vdd->brightness[ndx]->brightness_packet[0];

	sscanf(buf, "%d", &candela_level);
	
	get_panel_tx_cmds(ctrl, TX_TFT_PWM)->cmds[0].payload[1] = get_bit(candela_level, 4, 8);
	get_panel_tx_cmds(ctrl, TX_TFT_PWM)->cmds[0].payload[2] = get_bit(candela_level, 0, 4) << 4 | BIT(0);

	mdss_samsung_update_brightness_packet(packet, &cmd_cnt, get_panel_tx_cmds(ctrl, TX_TFT_PWM));
	
	brightness_cmds->cmd_cnt = cmd_cnt;
	mdss_samsung_single_transmission_packet(brightness_cmds);
	mdss_samsung_send_cmd(ctrl, TX_BRIGHT_CTRL);
	LCD_INFO("DSI%d  candela : %dcd \n",
				ndx, candela_level);

	return size;
}

#endif


#endif

static ssize_t mdss_samsung_read_mtp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int addr, len, start;
	char *read_buf = NULL;
	char read_size, read_startoffset;
	struct dsi_panel_cmds *rx_cmds;
	struct mdss_dsi_ctrl_pdata *ctrl;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		return size;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("no ctrl");
		return size;
	}

	sscanf(buf, "%x %d %x", &addr, &len, &start);

	read_buf = kmalloc(len * sizeof(char), GFP_KERNEL);

	LCD_INFO("%x %d %x\n", addr, len, start);

	rx_cmds = get_panel_rx_cmds(ctrl, RX_MTP_READ_SYSFS);

	rx_cmds->cmds[0].payload[0] =  addr;
	rx_cmds->cmds[0].payload[1] =  len;
	rx_cmds->cmds[0].payload[2] =  start;

	read_size = len;
	read_startoffset = start;

	rx_cmds->read_size =  &read_size;
	rx_cmds->read_startoffset =  &read_startoffset;


	LCD_INFO("%x %x %x %x %x %x %x %x %x\n",
		rx_cmds->cmds[0].dchdr.dtype,
		rx_cmds->cmds[0].dchdr.last,
		rx_cmds->cmds[0].dchdr.vc,
		rx_cmds->cmds[0].dchdr.ack,
		rx_cmds->cmds[0].dchdr.wait,
		rx_cmds->cmds[0].dchdr.dlen,
		rx_cmds->cmds[0].payload[0],
		rx_cmds->cmds[0].payload[1],
		rx_cmds->cmds[0].payload[2]);

	mdss_samsung_panel_data_read(ctrl, rx_cmds, read_buf, LEVEL1_KEY);

	kfree(read_buf);

	return size;
}

static ssize_t mdss_samsung_temperature_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return rc;
	}

	if (vdd->elvss_interpolation_temperature == -15)
		rc = snprintf((char *)buf, 40, "-15, -14, 0, 1, 30, 40\n");
	else
		rc = snprintf((char *)buf, 40, "-20, -19, 0, 1, 30, 40\n");

	LCD_INFO("temperature : %d elvss_interpolation_temperature : %d\n", vdd->temperature, vdd->elvss_interpolation_temperature);

	return rc;
}

static ssize_t mdss_samsung_temperature_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_panel_data *pdata;
	int pre_temp = 0;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		return size;
	}

	pdata = &vdd->ctrl_dsi[DISPLAY_1]->panel_data;
	pre_temp = vdd->temperature;

	sscanf(buf, "%d", &vdd->temperature);

	/* When temperature changed, hbm_mode must setted 0 for EA8061 hbm setting. */
	if (pre_temp != vdd->temperature && vdd->display_status_dsi[DISPLAY_1].hbm_mode == 1)
		vdd->display_status_dsi[DISPLAY_1].hbm_mode = 0;

	mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
	pdata->set_backlight(pdata, vdd->bl_level);
	mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);

	LCD_INFO("temperature : %d", vdd->temperature);

	return size;
}

static ssize_t mdss_samsung_lux_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return rc;
	}

	rc = snprintf((char *)buf, 40, "%d\n", vdd->lux);

	LCD_INFO("lux : %d\n", vdd->lux);

	return rc;
}

static ssize_t mdss_samsung_lux_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_panel_data *pdata;
	int pre_lux = 0;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		return size;
	}

	pdata = &vdd->ctrl_dsi[DISPLAY_1]->panel_data;
	pre_lux = vdd->lux;

	sscanf(buf, "%d", &vdd->lux);

	if (vdd->support_mdnie_lite && pre_lux != vdd->lux)
		update_dsi_tcon_mdnie_register(vdd);

	LCD_INFO("lux : %d", vdd->lux);

	return size;
}

static ssize_t mdss_samsung_read_copr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return ret;
	}

	mutex_lock(&vdd->copr.copr_lock);

	if (vdd->copr.copr_on) {
		vdd->panel_func.samsung_copr_read(vdd);
		LCD_INFO("%d %d\n", vdd->copr.copr_data, vdd->bl_level);
		ret = snprintf((char *)buf, 20, "%d %d\n", vdd->copr.copr_data, vdd->bl_level);
	} else {
		ret = snprintf((char *)buf, 20, "-1 -1\n");
	}

	mutex_unlock(&vdd->copr.copr_lock);

	return ret;
}

static ssize_t mdss_samsung_copr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int ret = 0;
	s64 temp_avr = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return ret;
	}

	mutex_lock(&vdd->copr.copr_lock);

	if (vdd->copr.copr_on) {

		vdd->panel_func.samsung_set_copr_sum(vdd);

		temp_avr = vdd->copr.copr_sum;
		do_div(temp_avr, vdd->copr.total_t);
		vdd->copr.copr_avr = temp_avr;

		temp_avr = vdd->copr.cd_sum;
		do_div(temp_avr, vdd->copr.total_t);
		vdd->copr.cd_avr = temp_avr;

		LCD_INFO("copr_avr(%d) cd_avr (%d) copr_sum (%lld) cd_sum (%lld) total_t (%lld) \n",
			vdd->copr.copr_avr, vdd->copr.cd_avr,
			vdd->copr.copr_sum, vdd->copr.cd_sum, vdd->copr.total_t);

		vdd->copr.copr_sum = 0;
		vdd->copr.cd_sum = 0;
		vdd->copr.total_t = 0;

		ret = snprintf((char *)buf, 20, "%d %d\n", vdd->copr.copr_avr, vdd->copr.cd_avr);

		vdd->panel_func.samsung_copr_read(vdd);
	} else {
		ret = snprintf((char *)buf, 20, "-1 -1\n");
	}

	mutex_unlock(&vdd->copr.copr_lock);

	return ret;
}

static ssize_t mdss_samsung_copr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	int data = 0;

	sscanf(buf, "%d", &data);

	if (data == 0) {
		LCD_INFO("COPR disable! (%d)\n", data);
		vdd->panel_func.samsung_copr_enable(vdd, 0);

	} else {
		LCD_INFO("COPR enable! (%d)\n", data);
		vdd->panel_func.samsung_copr_enable(vdd, 1);
	}

	return size;
}

static ssize_t mdss_samsung_disp_partial_disp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	LCD_DEBUG("TDB");

	return 0;
}

static ssize_t mdss_samsung_disp_partial_disp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	LCD_DEBUG("TDB");

	return size;
}

/*
 * Panel LPM related functions
 */
static ssize_t mdss_samsung_panel_lpm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
	(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_dsi_ctrl_pdata *ctrl;
	struct panel_func *pfunc;
	u8 current_status = 0;

	pfunc = &vdd->panel_func;

	if (IS_ERR_OR_NULL(pfunc)) {
		LCD_ERR("no pfunc");
		return rc;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("no ctrl");
		return rc;
	}

	if (vdd->dtsi_data[DISPLAY_1].panel_lpm_enable)
		current_status = vdd->panel_lpm.mode;

	rc = snprintf((char *)buf, 30, "%d\n", current_status);
	LCD_INFO("[Panel LPM] Current status : %d\n", (int)current_status);

	return rc;
}

void mdss_samsung_panel_lpm_mode_store(struct mdss_panel_data *pdata, u8 mode)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	struct mdss_panel_info *pinfo = NULL;
	u8 req_mode = mode;

	if (IS_ERR_OR_NULL(pdata)) {
		LCD_ERR("Invalid input data\n");
		return;
	}

	LCD_DEBUG("[Panel LPM] ++\n");

	pinfo = &pdata->panel_info;
	if (IS_ERR_OR_NULL(pinfo)) {
		LCD_ERR("pinfo is null..\n");
		return;
	}

	vdd = (struct samsung_display_driver_data *)pdata->panel_private;
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null..\n");
		return;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..\n");
		return;
	}

	if (!vdd->dtsi_data[DISPLAY_1].panel_lpm_enable) {
		LCD_INFO("[Panel LPM] LPM(ALPM/HLPM) is not supported\n");
		return;
	}

	mutex_lock(&vdd->vdd_panel_lpm_lock);

	/*
	 * Get Panel LPM mode and update brightness and Hz setting
	 */
	if (vdd->panel_func.samsung_get_panel_lpm_mode)
		vdd->panel_func.samsung_get_panel_lpm_mode(ctrl, &req_mode);

	if ((req_mode >= MAX_LPM_MODE)) {
		LCD_INFO("[Panel LPM] Set req_mode to ALPM_MODE_ON\n");
		req_mode = ALPM_MODE_ON;
	}

	vdd->panel_lpm.mode = req_mode;

	mutex_unlock(&vdd->vdd_panel_lpm_lock);

	if (unlikely(vdd->is_factory_mode)) {
		if (vdd->panel_lpm.mode == MODE_OFF)
			mdss_samsung_panel_lpm_ctrl(pdata, false);
		else
			mdss_samsung_panel_lpm_ctrl(pdata, true);
	} else {
		if (vdd->vdd_blank_mode[DISPLAY_1] == FB_BLANK_NORMAL) {
			if (vdd->panel_lpm.mode == MODE_OFF)
				mdss_samsung_panel_lpm_ctrl(pdata, false);
			else
				mdss_samsung_panel_lpm_ctrl(pdata, true);
		}
	}

	LCD_DEBUG("[Panel LPM] --\n");
}

static ssize_t mdss_samsung_panel_lpm_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int mode = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_panel_data *pdata;
	struct mdss_panel_info *pinfo;

	pdata = &vdd->ctrl_dsi[DSI_CTRL_0]->panel_data;
	pinfo = &pdata->panel_info;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		return size;
	}

	sscanf(buf, "%d", &mode);
	LCD_INFO("[Panel LPM] Mode : %d\n", mode);

	mdss_samsung_panel_lpm_mode_store(pdata, mode + MAX_LPM_MODE);

	return size;
}

static ssize_t mipi_samsung_hmt_bright_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	if (!vdd->dtsi_data[0].hmt_enabled) {
		LCD_ERR("hmt is not supported..\n");
		return -ENODEV;
	}

	rc = snprintf((char *)buf, 30, "%d\n", vdd->hmt_stat.hmt_bl_level);
	LCD_INFO("[HMT] hmt bright : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_hmt_bright_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	struct mdss_panel_info *pinfo = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl;

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	if (!vdd->dtsi_data[0].hmt_enabled) {
		LCD_ERR("hmt is not supported..\n");
		return -ENODEV;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;;
	}

	pinfo = &ctrl->panel_data.panel_info;

	sscanf(buf, "%d", &input);
	LCD_INFO("[HMT] input (%d) ++\n", input);

	if (!vdd->hmt_stat.hmt_on) {
		LCD_INFO("[HMT] hmt is off!\n");
		return size;
	}

	if (!pinfo->blank_state) {
		LCD_ERR("[HMT] panel is off!\n");
		vdd->hmt_stat.hmt_bl_level = input;
		return size;
	}

	if (vdd->hmt_stat.hmt_bl_level == input) {
		LCD_ERR("[HMT] hmt bright already %d!\n", vdd->hmt_stat.hmt_bl_level);
		return size;
	}

	vdd->hmt_stat.hmt_bl_level = input;
	hmt_bright_update(ctrl);

	LCD_INFO("[HMT] input (%d) --\n", input);

	return size;
}

static ssize_t mipi_samsung_hmt_on_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	if (!vdd->dtsi_data[0].hmt_enabled) {
		LCD_ERR("hmt is not supported..\n");
		return -ENODEV;
	}

	rc = snprintf((char *)buf, 30, "%d\n", vdd->hmt_stat.hmt_on);
	LCD_INFO("[HMT] hmt on input : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_hmt_on_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	struct mdss_panel_info *pinfo = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl;

	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	if (!vdd->dtsi_data[0].hmt_enabled) {
		LCD_ERR("hmt is not supported..\n");
		return -ENODEV;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;;
	}

	pinfo = &ctrl->panel_data.panel_info;

	sscanf(buf, "%d", &input);
	LCD_INFO("[HMT] input (%d) ++\n", input);

	if (!pinfo->blank_state) {
		LCD_ERR("[HMT] panel is off!\n");
		vdd->hmt_stat.hmt_on = input;
		return size;
	}

	if (vdd->hmt_stat.hmt_on == input) {
		LCD_INFO("[HMT] hmt already %s !\n", vdd->hmt_stat.hmt_on?"ON":"OFF");
		return size;
	}

	vdd->hmt_stat.hmt_on = input;

	hmt_enable(ctrl, vdd);
	hmt_reverse_update(ctrl, vdd->hmt_stat.hmt_on);

	hmt_bright_update(ctrl);

	LCD_INFO("[HMT] input hmt (%d) --\n",
		vdd->hmt_stat.hmt_on);

	return size;
}

void mdss_samsung_cabc_update(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return;
	}

	if (vdd->auto_brightness) {
		LCD_INFO("auto brightness is on , cabc cmds are already sent--\n");
		return;
	}

	if (vdd->siop_status) {
		if (vdd->panel_func.samsung_lvds_write_reg)
			vdd->panel_func.samsung_brightness_tft_pwm(vdd->ctrl_dsi[DISPLAY_1], vdd->bl_level);
		else {
			mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], TX_CABC_OFF_DUTY);
			mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], TX_CABC_ON);
			if (vdd->dtsi_data[0].cabc_delay && !vdd->display_status_dsi[0].disp_on_pre)
				usleep_range(vdd->dtsi_data[0].cabc_delay, vdd->dtsi_data[0].cabc_delay);
			mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], TX_CABC_ON_DUTY);
		}
	} else {
		if (vdd->panel_func.samsung_lvds_write_reg)
			vdd->panel_func.samsung_brightness_tft_pwm(vdd->ctrl_dsi[DISPLAY_1], vdd->bl_level);
		else {
			mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], TX_CABC_OFF_DUTY);
			mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], TX_CABC_OFF);
		}
	}
}

int config_cabc(int value)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return value;
	}

	if (vdd->siop_status == value) {
		LCD_INFO("No change in cabc state, update not needed--\n");
		return value;
	}

	vdd->siop_status = value;
	mdss_samsung_cabc_update();
	return 0;
}

static ssize_t mipi_samsung_mcd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_dsi_ctrl_pdata *ctrl;
	int input;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		goto end;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;;
	}

	sscanf(buf, "%d", &input);

	LCD_INFO("(%d)\n", input);

	if (input)
		mdss_samsung_send_cmd(ctrl, TX_MCD_ON);
	else
		mdss_samsung_send_cmd(ctrl, TX_MCD_OFF);
end:
	return size;
}

#if defined(CONFIG_SUPPORT_POC_FLASH)
#define MAX_POC_SHOW_WAIT 500
static ssize_t mipi_samsung_poc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	static int string_size = 20;
	static char check_sum[5] = {0,};
	static char EB_value[4] = {0,};

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct dsi_panel_cmds *rx_cmds;
	struct mdss_dsi_ctrl_pdata *ctrl;
	char temp[string_size];
	int ndx;
	int poc;
	int *octa_id;
	int wait_cnt;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;
	}

	/* POC : From OCTA_ID*/
	ndx = display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]);
	octa_id = &vdd->octa_id_dsi[ndx][0];
	poc = octa_id[1] & 0x0f; /* S6E3HA6_AMS622MR01 poc offset is 1*/
	LCD_INFO("POC :(%d)\n", poc);

	/*
		This is temp code. Shoud be modified with locking or waiting logic.
		UNBLANK & RX operation should be protected mutually.
	*/
	for (wait_cnt = 0; wait_cnt < MAX_POC_SHOW_WAIT; wait_cnt++) {
		msleep(10);
		if ((vdd->vdd_blank_mode[ndx] ==  FB_BLANK_UNBLANK))/* &&
			(vdd->vdd_blank_mode_done[ndx] ==  FB_BLANK_UNBLANK))*/
			break;
	}

	if (wait_cnt < MAX_POC_SHOW_WAIT) {
		/*
			Retrun to App
			POC         : 0 or 1
			CHECKSUM    : 0 or 1   (0 : OK / 1 : Fail)
			EB 4th para : 33 or FF
		*/

		rx_cmds = get_panel_rx_cmds(ctrl, RX_POC_CHECKSUM);
		mdss_samsung_panel_data_read(ctrl, rx_cmds, check_sum, LEVEL1_KEY);

		rx_cmds = get_panel_rx_cmds(ctrl, RX_POC_STATUS);
		mdss_samsung_panel_data_read(ctrl, rx_cmds, EB_value, LEVEL1_KEY);
	} else
		LCD_INFO("MDSS FAIL TO RX -- wait_cnt : %d max_cnt : %d --\n", wait_cnt, MAX_POC_SHOW_WAIT);

	LCD_INFO("CHECKSUM : (%d)(%d)(%d)(%d)(%d)\n",
			check_sum[0], check_sum[1], check_sum[2], check_sum[3], check_sum[4]);
	LCD_INFO("POC STATUS : (%d)(%d)(%d)(%d)\n",
			EB_value[0], EB_value[1], EB_value[2], EB_value[3]);
	LCD_INFO("%d %d %02x\n", poc, check_sum[4], EB_value[3]);

	snprintf((char *)temp, sizeof(temp), "%d %d %02x\n", poc, check_sum[4], EB_value[3]);
	strlcat(buf, temp, string_size);

	return strnlen(buf, string_size);
}

static ssize_t mipi_samsung_poc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_dsi_ctrl_pdata *ctrl;
	int input;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;
	}

	sscanf(buf, "%d", &input);
	LCD_INFO("INPUT : (%d)\n", input);

	if (input == 1) {
		LCD_INFO("ERASE \n");
		if (vdd->panel_func.samsung_poc_ctrl) {
			ret = vdd->panel_func.samsung_poc_ctrl(vdd, POC_OP_ERASE);
		}
	} else if (input == 2) {
		LCD_INFO("WRITE \n");
	} else if (input == 3) {
		LCD_INFO("READ \n");
	} else if (input == 4) {
		LCD_INFO("STOP\n");
		atomic_set(&vdd->poc_driver.cancel, 1);
	} else {
		LCD_INFO("input check !! \n");
	}

	return size;
}
#endif

static ssize_t xtalk_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_dsi_ctrl_pdata *ctrl;
	struct mdss_panel_data *pdata;

	int input;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		goto end;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;;
	}

	pdata = &vdd->ctrl_dsi[DISPLAY_1]->panel_data;
	sscanf(buf, "%d", &input);
	LCD_INFO("(%d)\n", input);

	if (input) {
		vdd->xtalk_mode = 1;
	} else {
		vdd->xtalk_mode = 0;
	}
	pdata->set_backlight(pdata, vdd->bl_level);
end:
	return size;
}

static ssize_t mdss_samsung_irc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return rc;
	}

	rc = snprintf((char *)buf, 50, "irc_enable_status : %d irc_mode : %d\n",
		vdd->irc_info.irc_enable_status, vdd->irc_info.irc_mode);

	LCD_INFO("irc_enable_status : %d irc_mode : %d\n",
		vdd->irc_info.irc_enable_status, vdd->irc_info.irc_mode);

	return rc;
}

static ssize_t mdss_samsung_irc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int input_enable, input_mode;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		goto end;
	}

	sscanf(buf, "%d %x", &input_enable, &input_mode);

	if (input_enable != 0 && input_enable != 1) {
		LCD_INFO("Invalid argument %d 0x%x !!\n", input_enable, input_mode);
		return size;
	}

	if (input_mode >= IRC_MAX_MODE) {
		LCD_INFO("Invalid argument %d 0x%x !!\n", input_enable, input_mode);
		return size;
	}

	if (vdd->irc_info.irc_enable_status != input_enable ||
		vdd->irc_info.irc_mode != input_mode) {

		vdd->irc_info.irc_enable_status = input_enable;
		vdd->irc_info.irc_mode = input_mode;

		LCD_INFO("%d 0x%x\n", input_enable, input_mode);

		if (!vdd->dtsi_data[DISPLAY_1].tft_common_support) {
			mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
			mdss_samsung_brightness_dcs(vdd->ctrl_dsi[DISPLAY_1], vdd->bl_level);
			mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
		}
	}
end:
	return size;
}

static ssize_t mdss_samsung_ldu_correction_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t mdss_samsung_ldu_correction_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);

	if ((value < 0) || (value > 7)) {
		LCD_ERR("out of range %d\n", value);
		return -EINVAL;
	}

	vdd->ldu_correction_state = value;

	LCD_INFO("ldu_correction_state : %d\n", vdd->ldu_correction_state);

	return size;
}

static ssize_t mipi_samsung_hw_cursor_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_dsi_ctrl_pdata *ctrl;
	int input[10];

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		goto end;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;;
	}

	sscanf(buf, "%d %d %d %d %d %d %d %x %x %x", &input[0], &input[1],
			&input[2], &input[3], &input[4], &input[5], &input[6],
			&input[7], &input[8], &input[9]);

	if (!IS_ERR_OR_NULL(vdd->panel_func.ddi_hw_cursor))
		vdd->panel_func.ddi_hw_cursor(ctrl, input);
	else
		LCD_ERR("ddi_hw_cursor function is NULL\n");

end:
	return size;
}

static ssize_t mdss_samsung_adaptive_control_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);

	LCD_INFO("ACL value : %x\n", value);
	vdd->gradual_acl_val = value;

	if (!vdd->gradual_acl_val)
		vdd->acl_status = 0;
	else
		vdd->acl_status = 1;

	if (!vdd->dtsi_data[DISPLAY_1].tft_common_support) {
		mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
		mdss_samsung_brightness_dcs(vdd->ctrl_dsi[DISPLAY_1], vdd->bl_level);
		mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
	}

	return size;
}
static ssize_t mdss_samsung_multires_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int rc = 0;

	rc = snprintf((char *)buf, 30, "mode = %d\n", vdd->multires_stat.curr_mode);

	return rc;
}

static ssize_t mdss_samsung_multires_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	int value;

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;;
	}

	sscanf(buf, "%d", &value);

	vdd->multires_stat.curr_mode = value;
	if (vdd->panel_func.samsung_multires)
		vdd->panel_func.samsung_multires(vdd);
	LCD_INFO("ldu_correction_state : %d\n", vdd->multires_stat.curr_mode);

	return size;
}

static ssize_t mdss_samsung_cover_control_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int rc = 0;

	rc = snprintf((char *)buf, 30, "mode = %d\n", vdd->cover_control);

	return rc;
}

static ssize_t mdss_samsung_cover_control_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	int value;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		goto end;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;;
	}

	sscanf(buf, "%d", &value);
	vdd->cover_control = value;

	if (vdd->panel_func.samsung_cover_control)
		vdd->panel_func.samsung_cover_control(ctrl, vdd);
	else
		goto end;

	LCD_INFO("Cover Control Status = %d\n", vdd->cover_control);
end:
	return size;
}

static ssize_t mdss_samsung_disp_SVC_OCTA_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	static int string_size = 50;
	char temp[string_size];
	int *cell_id;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	cell_id = &vdd->cell_id_dsi[DISPLAY_1][0];

	/*
	*	STANDARD FORMAT (Total is 11Byte)
	*	MAX_CELL_ID : 11Byte
	*	7byte(cell_id) + 2byte(Mdnie x_postion) + 2byte(Mdnie y_postion)
	*/

	snprintf((char *)temp, sizeof(temp),
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		cell_id[0], cell_id[1], cell_id[2], cell_id[3], cell_id[4],
		cell_id[5], cell_id[6],
		(vdd->mdnie_x[DISPLAY_1] & 0xFF00) >> 8,
		vdd->mdnie_x[DISPLAY_1] & 0xFF,
		(vdd->mdnie_y[DISPLAY_1] & 0xFF00) >> 8,
		vdd->mdnie_y[DISPLAY_1] & 0xFF);

	strlcat(buf, temp, string_size);

	return strnlen(buf, string_size);
}

static ssize_t mdss_samsung_disp_SVC_OCTA2_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	static int string_size = 50;
	char temp[string_size];
	int *cell_id;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	cell_id = &vdd->cell_id_dsi[DISPLAY_2][0];

	/*
	*	STANDARD FORMAT (Total is 11Byte)
	*	MAX_CELL_ID : 11Byte
	*	7byte(cell_id) + 2byte(Mdnie x_postion) + 2byte(Mdnie y_postion)
	*/

	snprintf((char *)temp, sizeof(temp),
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		cell_id[0], cell_id[1], cell_id[2], cell_id[3], cell_id[4],
		cell_id[5], cell_id[6],
		(vdd->mdnie_x[DISPLAY_2] & 0xFF00) >> 8,
		vdd->mdnie_x[DISPLAY_2] & 0xFF,
		(vdd->mdnie_y[DISPLAY_2] & 0xFF00) >> 8,
		vdd->mdnie_y[DISPLAY_2] & 0xFF);

	strlcat(buf, temp, string_size);

	return strnlen(buf, string_size);
}

static ssize_t mdss_samsung_disp_SVC_OCTA_CHIPID_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	static int string_size = 50;
	char temp[string_size];
	int *octa_id;
	int site, rework, poc, max_brightness;

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	octa_id = &vdd->octa_id_dsi[DISPLAY_1][0];

	site = octa_id[0] & 0xf0;
	site >>= 4;
	rework = octa_id[0] & 0x0f;
	poc = octa_id[1] & 0x0f;
	max_brightness = octa_id[2] * 256 + octa_id[3];

	snprintf((char *)temp, sizeof(temp),
			"%d%d%d%02x%02x%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
		site, rework, poc, octa_id[2], octa_id[3],
		octa_id[4] != 0 ? octa_id[4] : '0',
		octa_id[5] != 0 ? octa_id[5] : '0',
		octa_id[6] != 0 ? octa_id[6] : '0',
		octa_id[7] != 0 ? octa_id[7] : '0',
		octa_id[8] != 0 ? octa_id[8] : '0',
		octa_id[9] != 0 ? octa_id[9] : '0',
		octa_id[10] != 0 ? octa_id[10] : '0',
		octa_id[11] != 0 ? octa_id[11] : '0',
		octa_id[12] != 0 ? octa_id[12] : '0',
		octa_id[13] != 0 ? octa_id[13] : '0',
		octa_id[14] != 0 ? octa_id[14] : '0',
		octa_id[15] != 0 ? octa_id[15] : '0',
		octa_id[16] != 0 ? octa_id[16] : '0',
		octa_id[17] != 0 ? octa_id[17] : '0',
		octa_id[18] != 0 ? octa_id[18] : '0',
		octa_id[19] != 0 ? octa_id[19] : '0');

	strlcat(buf, temp, string_size);

	LCD_INFO("%d%d%d%02x%02x%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
		site, rework, poc, octa_id[2], octa_id[3],
		octa_id[4] != 0 ? octa_id[4] : '0',
		octa_id[5] != 0 ? octa_id[5] : '0',
		octa_id[6] != 0 ? octa_id[6] : '0',
		octa_id[7] != 0 ? octa_id[7] : '0',
		octa_id[8] != 0 ? octa_id[8] : '0',
		octa_id[9] != 0 ? octa_id[9] : '0',
		octa_id[10] != 0 ? octa_id[10] : '0',
		octa_id[11] != 0 ? octa_id[11] : '0',
		octa_id[12] != 0 ? octa_id[12] : '0',
		octa_id[13] != 0 ? octa_id[13] : '0',
		octa_id[14] != 0 ? octa_id[14] : '0',
		octa_id[15] != 0 ? octa_id[15] : '0',
		octa_id[16] != 0 ? octa_id[16] : '0',
		octa_id[17] != 0 ? octa_id[17] : '0',
		octa_id[18] != 0 ? octa_id[18] : '0',
		octa_id[19] != 0 ? octa_id[19] : '0');

	return strnlen(buf, string_size);
}

static ssize_t mdss_samsung_disp_SVC_OCTA2_CHIPID_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	static int string_size = 50;
	char temp[string_size];
	int *octa_id;
	int site, rework, poc, max_brightness;

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return strnlen(buf, string_size);
	}

	octa_id = &vdd->octa_id_dsi[DISPLAY_2][0];

	site = octa_id[0] & 0xf0;
	site >>= 4;
	rework = octa_id[0] & 0x0f;
	poc = octa_id[1] & 0x0f;
	max_brightness = octa_id[2] * 256 + octa_id[3];

	snprintf((char *)temp, sizeof(temp),
			"%d%d%d%02x%02x%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
		site, rework, poc, octa_id[2], octa_id[3],
		octa_id[4] != 0 ? octa_id[4] : '0',
		octa_id[5] != 0 ? octa_id[5] : '0',
		octa_id[6] != 0 ? octa_id[6] : '0',
		octa_id[7] != 0 ? octa_id[7] : '0',
		octa_id[8] != 0 ? octa_id[8] : '0',
		octa_id[9] != 0 ? octa_id[9] : '0',
		octa_id[10] != 0 ? octa_id[10] : '0',
		octa_id[11] != 0 ? octa_id[11] : '0',
		octa_id[12] != 0 ? octa_id[12] : '0',
		octa_id[13] != 0 ? octa_id[13] : '0',
		octa_id[14] != 0 ? octa_id[14] : '0',
		octa_id[15] != 0 ? octa_id[15] : '0',
		octa_id[16] != 0 ? octa_id[16] : '0',
		octa_id[17] != 0 ? octa_id[17] : '0',
		octa_id[18] != 0 ? octa_id[18] : '0',
		octa_id[19] != 0 ? octa_id[19] : '0');

	strlcat(buf, temp, string_size);

	LCD_INFO("%d%d%d%02x%02x%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
		site, rework, poc, octa_id[2], octa_id[3],
		octa_id[4] != 0 ? octa_id[4] : '0',
		octa_id[5] != 0 ? octa_id[5] : '0',
		octa_id[6] != 0 ? octa_id[6] : '0',
		octa_id[7] != 0 ? octa_id[7] : '0',
		octa_id[8] != 0 ? octa_id[8] : '0',
		octa_id[9] != 0 ? octa_id[9] : '0',
		octa_id[10] != 0 ? octa_id[10] : '0',
		octa_id[11] != 0 ? octa_id[11] : '0',
		octa_id[12] != 0 ? octa_id[12] : '0',
		octa_id[13] != 0 ? octa_id[13] : '0',
		octa_id[14] != 0 ? octa_id[14] : '0',
		octa_id[15] != 0 ? octa_id[15] : '0',
		octa_id[16] != 0 ? octa_id[16] : '0',
		octa_id[17] != 0 ? octa_id[17] : '0',
		octa_id[18] != 0 ? octa_id[18] : '0',
		octa_id[19] != 0 ? octa_id[19] : '0');

	return strnlen(buf, string_size);
}

static ssize_t mipi_samsung_esd_check_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct irqaction *action = NULL;
	struct irq_desc *desc = NULL;
	int rc = 0;
	int i;
	int irq;

	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data vdd : 0x%zx\n", __func__, (size_t)vdd);
		return 0;
	}

	LCD_INFO("num gpio (%d) \n", vdd->esd_recovery.num_of_gpio);

	for (i = 0; i < vdd->esd_recovery.num_of_gpio; i++) {
		irq = gpio_to_irq(vdd->esd_recovery.esd_gpio[i]);
		LCD_INFO("irq : %d\n", irq);

		desc = irq_to_desc(irq);
		if (desc) {
			action = desc->action;
			if (action->thread_fn) {
				LCD_ERR("[%d] gpio (%d) irq (%d) handler (%s)\n",
					i, vdd->esd_recovery.esd_gpio[i], irq, action->name);

				generic_handle_irq(irq);
			} else {
				LCD_ERR("No handler for irq(%d)\n", irq);
			}
		} else
			LCD_ERR("No desc..\n");

	}

	return rc;
}

#if 0
static ssize_t mdss_samsung_active_clock_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len = 0;
	struct act_clk_info *info;
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	info = &vdd->act_clock.clk_info;

	len = snprintf(buf, 30, "active en : %d\n", info->en);
	len += snprintf(buf+len, 30, "interval : %d ms\n", info->interval);
	len += snprintf(buf+len, 30, "time %d:%d:%d:%d\n",
		info->time_hour, info->time_min, info->time_second, info->time_ms);
	len += snprintf(buf+len, 30, "pos : %d, %d\n", info->pos_x, info->pos_y);

	return strlen(buf);
}

static ssize_t mdss_samsung_active_clock_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;
	char *pload = NULL;
	char *pload_d = NULL;
	struct act_clk_info *c_info;
	struct act_drawer_info *d_info;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null or error\n");
		return -ENODEV;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;;
	}

	if (vdd->act_clock.act_clock_control_tx_cmds.cmds->payload) {
		pload = vdd->act_clock.act_clock_control_tx_cmds.cmds[1].payload;
		pload_d = vdd->act_clock.act_clock_control_tx_cmds.cmds[2].payload;
	} else {
		LCD_ERR("Clock Control Command Payload is NULL\n");
		return -ENODEV;
	}

	LCD_INFO("called\n");

	c_info = &vdd->act_clock.clk_info;
	d_info = &vdd->act_clock.draw_info;

	rc = sscanf(buf, "%d %d %d %d %d %d %d %d\n",
			&c_info->en, &c_info->interval, &c_info->time_hour, &c_info->time_min,
			&c_info->time_second, &c_info->time_ms, &c_info->pos_x, &c_info->pos_y);

	pload[1] = 0;

	if (c_info->en) {
		pload[2] = 0x10;

		if (c_info->interval == 100) {
			/* INC_STEP = 1(0.6 degree), UPDATE_RATE=3 */
			pload[9] = 0x03; /* UPDATE_RATE */
			pload[10] = 0x01; /* INC_STEP */
		} else {
			/* Default Value */
			/* INC_STEP = 10(6 degree), UPDATE_RATE=30 */
			pload[9] = 0x1E; /* UPDATE_RATE */
			pload[10] = 0x0A; /* INC_STEP */
		}

		/* Enable TIME_UPDATE */
		pload[10] |= BIT(7);

		pload[16] = c_info->time_hour % 12;
		pload[17] = c_info->time_min % 60;
		pload[18] = c_info->time_second % 60;
		pload[19] = c_info->time_ms;

		pload[20] = (c_info->pos_x >> 8) & 0xff;
		pload[21] = c_info->pos_x & 0xff;

		pload[22] = (c_info->pos_y >> 8) & 0xff;
		pload[23] = c_info->pos_y & 0xff;

		/* Update Self Drawer Command also */
		pload_d[2] = 0x01;

		pload_d[30] = (c_info->pos_x >> 8) & 0xff;
		pload_d[31] = c_info->pos_x & 0xff;
		pload_d[32] = (c_info->pos_y >> 8) & 0xff;
		pload_d[33] = c_info->pos_y & 0xff;

		pload_d[40] = (unsigned char)(d_info->sd_line_color >> 16) & 0xff; /* Inner Circle Color (R)*/
		pload_d[41] = (unsigned char)(d_info->sd_line_color >> 8) & 0xff; /* Inner Circle Color (G)*/
		pload_d[42] = (unsigned char)(d_info->sd_line_color & 0xff); /* Inner Circle Color (B)*/
		pload_d[43] = (unsigned char)(d_info->sd2_line_color >> 16) & 0xff; /* Outer Circle Color (R)*/
		pload_d[44] = (unsigned char)(d_info->sd2_line_color >> 8) & 0xff; /* Outer Circle Color (G)*/
		pload_d[45] = (unsigned char)(d_info->sd2_line_color & 0xff); /* Outer Circle Color (B)*/
	}

	LCD_INFO("[Active_Clock] Send Image Data ++ \n");
	mutex_lock(&vdd->vdd_act_clock_lock);
	mdss_samsung_send_cmd(ctrl, TX_ACT_CLOCK_IMG_DATA);
	mutex_unlock(&vdd->vdd_act_clock_lock);
	LCD_INFO("[Active_Clock] Send Image Data -- \n");
	mdss_samsung_send_cmd(ctrl, TX_ACT_CLOCK_ENABLE);
	msleep(35);
	/* Disable TIME_UPDATE */
	pload[10] &= ~BIT(7);
	/* Enable COMP_EN */
	pload[10] |= BIT(6);
	mdss_samsung_send_cmd(ctrl, TX_ACT_CLOCK_ENABLE);

	return size;
}
#endif

static ssize_t mdss_samsung_brightness_table_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;
	int i, ndx;
	struct candela_map_table *table = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	char *pos = buf;

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return rc;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return -ENODEV;;
	}

	ndx = display_ndx_check(ctrl);

	if (vdd->pac)
		table = &vdd->dtsi_data[ndx].pac_candela_map_table[vdd->panel_revision];
	else
		table = &vdd->dtsi_data[ndx].candela_map_table[vdd->panel_revision];

	LCD_INFO("tab_size (%d)\n", table->tab_size);

	for (i = 0; i < table->tab_size; i++) {
		if (vdd->pac)
			pos += snprintf((char *)pos, 30, "%5d %5d %3d %3d\n",
				table->from[i], table->end[i], table->cd[i], table->interpolation_cd[i]);
		else
			pos += snprintf((char *)pos, 30, "%3d %3d %3d\n",
				table->from[i], table->end[i], table->cd[i]);
	}
	return pos-buf;
}

#ifdef CONFIG_DISPLAY_USE_INFO
static int dpui_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct samsung_display_driver_data *vdd = container_of(self,
			struct samsung_display_driver_data, dpui_notif);
	struct dpui_info *dpui = data;
	char tbuf[MAX_DPUI_VAL_LEN];
	int ndx;
	int *cell_id;
	int year, mon, day, hour, min, sec;
	int lcd_id;
	int size;

	if (dpui == NULL) {
		LCD_ERR("err: dpui is null\n");
		return 0;
	}

	if (vdd == NULL) {
		LCD_ERR("err: vdd is null\n");
		return 0;
	}

	ndx = display_ndx_check(vdd->ctrl_dsi[DSI_CTRL_0]);

	/* Vendor and DDI model name */
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%s_%s",
			vdd->dtsi_data[ndx].panel_vendor, vdd->dtsi_data[ndx].disp_model);
	set_dpui_field(DPUI_KEY_DISP_MODEL, tbuf, size);

	/* manufacture date and time */
	cell_id = &vdd->cell_id_dsi[ndx][0];
	year = ((cell_id[0] >> 4) & 0xF) + 2011;
	mon = cell_id[0] & 0xF;
	day = cell_id[1] & 0x1F;
	hour = cell_id[2] & 0x1F;
	min = cell_id[3] & 0x3F;
	sec = cell_id[4] & 0x3F;

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%04d%02d%02d %02d%02d%02d", year, mon, day, hour, min, sec);
	set_dpui_field(DPUI_KEY_MAID_DATE, tbuf, size);

	/* lcd id */
	lcd_id = vdd->manufacture_id_dsi[ndx];

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", ((lcd_id  & 0xFF0000) >> 16));
	set_dpui_field(DPUI_KEY_LCDID1, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", ((lcd_id  & 0xFF00) >> 8));
	set_dpui_field(DPUI_KEY_LCDID2, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", (lcd_id  & 0xFF));
	set_dpui_field(DPUI_KEY_LCDID3, tbuf, size);

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "0x%02X%02X%02X%02X%02X",
					vdd->ddi_id_dsi[ndx][0], vdd->ddi_id_dsi[ndx][1], vdd->ddi_id_dsi[ndx][2],
					vdd->ddi_id_dsi[ndx][3], vdd->ddi_id_dsi[ndx][4]);

	set_dpui_field(DPUI_KEY_CHIPID, tbuf, size);

	/* cell id */
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			cell_id[0], cell_id[1], cell_id[2], cell_id[3], cell_id[4],
			cell_id[5], cell_id[6],
			(vdd->mdnie_x[ndx] & 0xFF00) >> 8,
			vdd->mdnie_x[ndx] & 0xFF,
			(vdd->mdnie_y[ndx] & 0xFF00) >> 8,
			vdd->mdnie_y[ndx] & 0xFF);

	set_dpui_field(DPUI_KEY_CELLID, tbuf, size);

	return 0;
}

static int mdss_samsung_register_dpui(struct samsung_display_driver_data *vdd)
{
	memset(&vdd->dpui_notif, 0,
			sizeof(vdd->dpui_notif));
	vdd->dpui_notif.notifier_call = dpui_notifier_callback;

	return dpui_logging_register(&vdd->dpui_notif, DPUI_TYPE_PANEL);
}

/*
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t mdss_samsung_dpui_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	update_dpui_log(DPUI_LOG_LEVEL_INFO);
	get_dpui_log(buf, DPUI_LOG_LEVEL_INFO);

	pr_info("%s\n", buf);

	return strlen(buf);
}

static ssize_t mdss_samsung_dpui_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_INFO);

	return size;
}

/*
 * [DEV ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t mdss_samsung_dpui_dbg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	update_dpui_log(DPUI_LOG_LEVEL_DEBUG);
	get_dpui_log(buf, DPUI_LOG_LEVEL_DEBUG);

	pr_info("%s\n", buf);

	return strlen(buf);
}

static ssize_t mdss_samsung_dpui_dbg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_DEBUG);

	return size;
}

#endif
u8 csc_update = 1;
u8 csc_change;

static ssize_t csc_read_cfg(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", csc_update);
	return ret;
}

static ssize_t csc_write_cfg(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int err;
	int mode;

	err =  kstrtoint(buf, 0, &mode);
	if (err)
		return ret;

	csc_update = (u8)mode;
	csc_change = 1;
	LCD_INFO("csc ctrl set to csc_update(%d)\n", csc_update);

	return ret;
}

static DEVICE_ATTR(lcd_type, S_IRUGO, mdss_samsung_disp_lcdtype_show, NULL);
static DEVICE_ATTR(cell_id, S_IRUGO, mdss_samsung_disp_cell_id_show, NULL);
static DEVICE_ATTR(octa_id, S_IRUGO, mdss_samsung_disp_octa_id_show, NULL);
static DEVICE_ATTR(window_type, S_IRUGO, mdss_samsung_disp_windowtype_show, NULL);
static DEVICE_ATTR(manufacture_date, S_IRUGO, mdss_samsung_disp_manufacture_date_show, NULL);
static DEVICE_ATTR(manufacture_code, S_IRUGO, mdss_samsung_disp_manufacture_code_show, NULL);
static DEVICE_ATTR(power_reduce, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_disp_acl_show, mdss_samsung_disp_acl_store);
static DEVICE_ATTR(siop_enable, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_disp_siop_show, mdss_samsung_disp_siop_store);
static DEVICE_ATTR(read_mtp, S_IRUGO | S_IWUSR | S_IWGRP, NULL, mdss_samsung_read_mtp_store);
static DEVICE_ATTR(temperature, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_temperature_show, mdss_samsung_temperature_store);
static DEVICE_ATTR(lux, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_lux_show, mdss_samsung_lux_store);
static DEVICE_ATTR(copr, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_copr_show, mdss_samsung_copr_store);
static DEVICE_ATTR(read_copr, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_read_copr_show, NULL);
static DEVICE_ATTR(aid_log, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_aid_log_show, NULL);
static DEVICE_ATTR(partial_disp, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_disp_partial_disp_show, mdss_samsung_disp_partial_disp_store);
static DEVICE_ATTR(alpm, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_panel_lpm_show, mdss_samsung_panel_lpm_store);
static DEVICE_ATTR(hmt_bright, S_IRUGO | S_IWUSR | S_IWGRP, mipi_samsung_hmt_bright_show, mipi_samsung_hmt_bright_store);
static DEVICE_ATTR(hmt_on, S_IRUGO | S_IWUSR | S_IWGRP,	mipi_samsung_hmt_on_show, mipi_samsung_hmt_on_store);
static DEVICE_ATTR(mcd_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL, mipi_samsung_mcd_store);
#if defined(CONFIG_SUPPORT_POC_FLASH)
static DEVICE_ATTR(poc, S_IRUGO | S_IWUSR | S_IWGRP, mipi_samsung_poc_show, mipi_samsung_poc_store);
#endif
static DEVICE_ATTR(irc, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_irc_show, mdss_samsung_irc_store);
static DEVICE_ATTR(ldu_correction, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_ldu_correction_show, mdss_samsung_ldu_correction_store);
static DEVICE_ATTR(adaptive_control, S_IRUGO | S_IWUSR | S_IWGRP, NULL, mdss_samsung_adaptive_control_store);
static DEVICE_ATTR(hw_cursor, S_IRUGO | S_IWUSR | S_IWGRP, NULL, mipi_samsung_hw_cursor_store);
static DEVICE_ATTR(multires, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_multires_show, mdss_samsung_multires_store);
static DEVICE_ATTR(cover_control, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_cover_control_show, mdss_samsung_cover_control_store);
static DEVICE_ATTR(SVC_OCTA, S_IRUGO, mdss_samsung_disp_SVC_OCTA_show, NULL);
static DEVICE_ATTR(SVC_OCTA2, S_IRUGO, mdss_samsung_disp_SVC_OCTA2_show, NULL);
static DEVICE_ATTR(SVC_OCTA_CHIPID, S_IRUGO, mdss_samsung_disp_SVC_OCTA_CHIPID_show, NULL);
static DEVICE_ATTR(SVC_OCTA2_CHIPID, S_IRUGO, mdss_samsung_disp_SVC_OCTA2_CHIPID_show, NULL);
static DEVICE_ATTR(esd_check, S_IRUGO, mipi_samsung_esd_check_show, NULL);
/*static DEVICE_ATTR(act_clock_test, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_active_clock_show, mdss_samsung_active_clock_store);*/
static DEVICE_ATTR(tuning, 0664, tuning_show, tuning_store);
static DEVICE_ATTR(csc_cfg, S_IRUGO | S_IWUSR, csc_read_cfg, csc_write_cfg);
static DEVICE_ATTR(xtalk_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL, xtalk_store);
static DEVICE_ATTR(brightness_table, S_IRUGO | S_IWUSR | S_IWGRP, mdss_samsung_brightness_table_show, NULL);
#ifdef CONFIG_DISPLAY_USE_INFO
static DEVICE_ATTR(dpui, S_IRUSR|S_IRGRP, mdss_samsung_dpui_show, mdss_samsung_dpui_store);
static DEVICE_ATTR(dpui_dbg, S_IRUSR|S_IRGRP, mdss_samsung_dpui_dbg_show, mdss_samsung_dpui_dbg_store);
#endif

static struct attribute *panel_sysfs_attributes[] = {
	&dev_attr_lcd_type.attr,
	&dev_attr_cell_id.attr,
	&dev_attr_octa_id.attr,
	&dev_attr_window_type.attr,
	&dev_attr_manufacture_date.attr,
	&dev_attr_manufacture_code.attr,
	&dev_attr_power_reduce.attr,
	&dev_attr_siop_enable.attr,
	&dev_attr_aid_log.attr,
	&dev_attr_read_mtp.attr,
	&dev_attr_read_copr.attr,
	&dev_attr_copr.attr,
	&dev_attr_temperature.attr,
	&dev_attr_lux.attr,
	&dev_attr_partial_disp.attr,
	&dev_attr_alpm.attr,
	&dev_attr_hmt_bright.attr,
	&dev_attr_hmt_on.attr,
	&dev_attr_mcd_mode.attr,
	&dev_attr_irc.attr,
	&dev_attr_ldu_correction.attr,
	&dev_attr_adaptive_control.attr,
	&dev_attr_hw_cursor.attr,
	&dev_attr_multires.attr,
	&dev_attr_cover_control.attr,
	&dev_attr_SVC_OCTA.attr,
	&dev_attr_SVC_OCTA2.attr,
	&dev_attr_SVC_OCTA_CHIPID.attr,
	&dev_attr_SVC_OCTA2_CHIPID.attr,
	&dev_attr_esd_check.attr,
/*	&dev_attr_act_clock_test.attr,*/
	&dev_attr_xtalk_mode.attr,
	&dev_attr_brightness_table.attr,
#if defined(CONFIG_SUPPORT_POC_FLASH)
	&dev_attr_poc.attr,
#endif
#ifdef CONFIG_DISPLAY_USE_INFO
	&dev_attr_dpui.attr,
	&dev_attr_dpui_dbg.attr,
#endif
	NULL
};
static const struct attribute_group panel_sysfs_group = {
	.attrs = panel_sysfs_attributes,
};

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
static DEVICE_ATTR(brightness_step, S_IRUGO | S_IWUSR | S_IWGRP,
			mdss_samsung_disp_brightness_step,
			NULL);
#if defined(CONFIG_BLIC_LM3632A)
static DEVICE_ATTR(pwm_tuning, S_IRUGO | S_IWUSR | S_IWGRP,
			NULL,
			mdss_brightness_12bit_pwm_tuning_store);
#endif

static DEVICE_ATTR(weakness_ccb, S_IRUGO | S_IWUSR | S_IWGRP,
			mdss_samsung_disp_color_weakness_show,
			mdss_samsung_disp_color_weakness_store);


static struct attribute *bl_sysfs_attributes[] = {
	&dev_attr_brightness_step.attr,
	&dev_attr_weakness_ccb.attr,
#if defined(CONFIG_BLIC_LM3632A)
	&dev_attr_pwm_tuning.attr,
#endif
	NULL
};

static const struct attribute_group bl_sysfs_group = {
	.attrs = bl_sysfs_attributes,
};
#endif /* END CONFIG_LCD_CLASS_DEVICE*/

int mdss_samsung_create_sysfs(void *data)
{
	static int sysfs_enable;
	int rc = 0;
	struct lcd_device *lcd_device;
#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	struct backlight_device *bd = NULL;
#endif
	struct samsung_display_driver_data *vdd = (struct samsung_display_driver_data *)data;
	struct device *csc_dev;
	struct kernfs_node *SVC_sd;
	struct kobject *SVC;

	/* sysfs creat func should be called one time in dual dsi mode */
	if (sysfs_enable)
		return 0;

	csc_dev = vdd->mfd_dsi[0]->fbi->dev;

	lcd_device = lcd_device_register("panel", NULL, data, NULL);

	if (IS_ERR_OR_NULL(lcd_device)) {
		rc = PTR_ERR(lcd_device);
		LCD_ERR("Failed to register lcd device..\n");
		return rc;
	}

	rc = sysfs_create_group(&lcd_device->dev.kobj, &panel_sysfs_group);
	if (rc) {
		LCD_ERR("Failed to create panel sysfs group..\n");
		sysfs_remove_group(&lcd_device->dev.kobj, &panel_sysfs_group);
		return rc;
	}

	/* To find SVC kobject */
	SVC_sd = sysfs_get_dirent(devices_kset->kobj.sd, "svc");
	if (IS_ERR_OR_NULL(SVC_sd)) {
		/* try to create SVC kobject */
		SVC = kobject_create_and_add("svc", &devices_kset->kobj);
		if (IS_ERR_OR_NULL(SVC))
			LCD_ERR("Failed to create sys/devices/svc already exist svc : 0x%p\n", SVC);
		else
			LCD_INFO("Success to create sys/devices/svc svc : 0x%p\n", SVC);
	} else {
		SVC = (struct kobject *)SVC_sd->priv;
		LCD_INFO("Success to find SVC_sd : 0x%p svc : 0x%p\n", SVC_sd, SVC);
	}

	if (!IS_ERR_OR_NULL(SVC)) {
		rc = sysfs_create_link(SVC, &lcd_device->dev.kobj, "OCTA");
		if (rc)
			LCD_ERR("Failed to create panel sysfs svc/OCTA..\n");
		else
			LCD_INFO("Success to create panel sysfs svc/OCTA..\n");
	} else
		LCD_ERR("Failed to find svc kobject\n");

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	bd = backlight_device_register("panel", &lcd_device->dev,
						data, NULL, NULL);
	if (IS_ERR(bd)) {
		rc = PTR_ERR(bd);
		LCD_ERR("backlight : failed to register device\n");
		return rc;
	}

	rc = sysfs_create_group(&bd->dev.kobj, &bl_sysfs_group);
	if (rc) {
		LCD_ERR("Failed to create backlight sysfs group..\n");
		sysfs_remove_group(&bd->dev.kobj, &bl_sysfs_group);
		return rc;
	}
#endif

	rc = sysfs_create_file(&lcd_device->dev.kobj, &dev_attr_tuning.attr);
	if (rc) {
		LCD_ERR("sysfs create fail-%s\n", dev_attr_tuning.attr.name);
		return rc;
	}

	rc = sysfs_create_file(&csc_dev->kobj, &dev_attr_csc_cfg.attr);
	if (rc) {
		LCD_ERR("sysfs create fail-%s\n", dev_attr_csc_cfg.attr.name);
		return rc;
	}

#ifdef CONFIG_DISPLAY_USE_INFO
	mdss_samsung_register_dpui(data);
#endif

	sysfs_enable = 1;

	LCD_INFO("done!!\n");

	return rc;
}
