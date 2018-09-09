/*
 * linux/drivers/video/fbdev/exynos/panel/sysfs.c
 *
 * Samsung MIPI-DSI Panel SYSFS driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 * JiHoon Kim <jihoonn.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/ctype.h>
#include <linux/lcd.h>

#include "panel.h"
#include "panel_drv.h"
#include "panel_bl.h"
#include "copr.h"
#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
#include "mdnie.h"
#endif
#ifdef CONFIG_PANEL_AID_DIMMING
#include "dimming.h"
#endif
#ifdef CONFIG_SUPPORT_DDI_FLASH
#include "panel_poc.h"
#endif
#ifdef CONFIG_EXTEND_LIVE_CLOCK
#include "./aod/aod_drv.h"
#endif

static DEFINE_MUTEX(sysfs_lock);

char *mcd_rs_name[MAX_MCD_RS] = {
	"MCD1_R", "MCD1_L", "MCD2_R", "MCD2_L",
};

extern struct kset *devices_kset;

//void g_tracing_mark_write( char id, char* str1, int value );
int fingerprint_value = -1;
static ssize_t fingerprint_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	snprintf(buf, PAGE_SIZE, "%u\n", fingerprint_value);

	return strlen(buf);
}

static ssize_t fingerprint_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;

	rc = kstrtouint(buf, (unsigned int)0, &fingerprint_value);
	if (rc < 0)
		return rc;

	//g_tracing_mark_write( 'C', "BCDS_hbm", fingerprint_value & 4);
	//g_tracing_mark_write( 'C', "BCDS_alpha", fingerprint_value & 2);

	dev_info(dev, "%s: %d\n", __func__, fingerprint_value);

	return size;
}

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "SDC_%02X%02X%02X\n",
			panel_data->id[0], panel_data->id[1], panel_data->id[2]);

	return strlen(buf);
}

static ssize_t window_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%02x %02x %02x\n",
			panel_data->id[0], panel_data->id[1], panel_data->id[2]);
	pr_info("%s %02x %02x %02x\n",
			__func__, panel_data->id[0], panel_data->id[1], panel_data->id[2]);
	return strlen(buf);
}

static ssize_t manufacture_code_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 code[5] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	resource_copy_by_name(panel_data, code, "code");

	snprintf(buf, PAGE_SIZE, "%02X%02X%02X%02X%02X\n",
		code[0], code[1], code[2], code[3], code[4]);

	return strlen(buf);
}

static ssize_t cell_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 date[PANEL_DATE_LEN] = { 0, }, coordinate[4] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	resource_copy_by_name(panel_data, date, "date");
	resource_copy_by_name(panel_data, coordinate, "coordinate");

	snprintf(buf, PAGE_SIZE, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
		date[0] , date[1], date[2], date[3], date[4], date[5], date[6],
		coordinate[0], coordinate[1], coordinate[2], coordinate[3]);

	return strlen(buf);
}

static ssize_t SVC_OCTA_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return cell_id_show(dev, attr, buf);
}

static ssize_t octa_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, site, rework, poc;
	u8 cell_id[16], octa_id[PANEL_OCTA_ID_LEN] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	int len = 0;
	bool cell_id_exist = true;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	resource_copy_by_name(panel_data, octa_id, "octa_id");

	site = (octa_id[0] >> 4) & 0x0F;
	rework = octa_id[0] & 0x0F;
	poc = octa_id[1] & 0x0F;

	pr_debug("site (%d), rework (%d), poc (%d)\n",
			site, rework, poc);

	pr_debug("<CELL ID>\n");
	for (i = 0; i < 16; i++) {
		cell_id[i] = isalnum(octa_id[i + 4]) ? octa_id[i + 4] : '\0';
		pr_debug("%x -> %c\n", octa_id[i + 4], cell_id[i]);
		if (cell_id[i] == '\0') {
			cell_id_exist = false;
			break;
		}
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "%d%d%d%02x%02x",
			site, rework, poc, octa_id[2], octa_id[3]);
	if (cell_id_exist) {
		for (i = 0; i < 16; i++)
			len += snprintf(buf + len, PAGE_SIZE - len, "%c", cell_id[i]);
	}
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	return strlen(buf);
}

static ssize_t SVC_OCTA_CHIPID_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return octa_id_show(dev, attr, buf);
}

static ssize_t color_coordinate_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 coordinate[4] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	resource_copy_by_name(panel_data, coordinate, "coordinate");

	snprintf(buf, PAGE_SIZE, "%u, %u\n", /* X, Y */
			coordinate[0] << 8 | coordinate[1],
			coordinate[2] << 8 | coordinate[3]);
	return strlen(buf);
}

static ssize_t manufacture_date_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 year;
	u8 month, day, hour, min, date[PANEL_DATE_LEN] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	resource_copy_by_name(panel_data, date, "date");

	year = ((date[0] & 0xF0) >> 4) + 2011;
	month = date[0] & 0xF;
	day = date[1] & 0x1F;
	hour = date[2] & 0x1F;
	min = date[3] & 0x3F;

	snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d:%d\n",
			year, month, day, hour, min);
	return strlen(buf);
}

static ssize_t brightness_table_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl;
	int br, len = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_bl = &panel->panel_bl;

	mutex_lock(&panel_bl->lock);
	for (br = 0; br <= get_max_brightness(panel_bl); br += BRT_SCALE)
		len += snprintf(buf + len, PAGE_SIZE - len, "%5d %3d\n",
				br, get_actual_brightness(panel_bl, br));
	mutex_unlock(&panel_bl->lock);

	return len;
}

static ssize_t adaptive_control_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n",
			panel_data->props.adaptive_control);

	return strlen(buf);
}

static ssize_t adaptive_control_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl;
	struct backlight_device *bd;
	int value, rc;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;
	bd = panel_bl->bd;

	if (bd == NULL) {
		panel_err("PANEL:ERR:%s:backlight device is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (value < 0 || value >= ACL_OPR_MAX) {
		panel_err("PANEL:ERR:%s:invalid adaptive_control %d\n",
				__func__, value);
		return -EINVAL;
	}

	if (panel_data->props.adaptive_control == value)
		return size;

	mutex_lock(&panel_bl->lock);
	panel_data->props.adaptive_control = value;
	mutex_unlock(&panel_bl->lock);
	backlight_update_status(bd);

	dev_info(dev, "%s, adaptive_control %d\n",
			__func__, panel_data->props.adaptive_control);

	return size;
}

static ssize_t siop_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.siop_enable);

	return strlen(buf);
}

static ssize_t siop_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct backlight_device *bd;
	int value, rc;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	bd = panel->panel_bl.bd;
	if (bd == NULL) {
		panel_err("PANEL:ERR:%s:backlight device is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.siop_enable == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.siop_enable = value;
	mutex_unlock(&panel->op_lock);
	backlight_update_status(bd);

	dev_info(dev, "%s, siop_enable %d\n",
			__func__, panel_data->props.siop_enable);
	return size;
}

static ssize_t temperature_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[] = "-15, -14, 0, 1\n";

	strcat(buf, temp);
	return strlen(buf);
}

static ssize_t temperature_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct backlight_device *bd;
	int value, rc;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	bd = panel->panel_bl.bd;
	if (bd == NULL) {
		panel_err("PANEL:ERR:%s:backlight device is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtoint(buf, 10, &value);
	if (rc < 0)
		return rc;

	mutex_lock(&panel->op_lock);
	panel_data->props.temperature = value;
	mutex_unlock(&panel->op_lock);
	backlight_update_status(bd);

	dev_info(dev, "%s, temperature %d\n",
			__func__, panel_data->props.temperature);

	return size;
}

unsigned char readbuf[256] = { 0xff, };
int readreg, readpos, readlen;

static ssize_t read_mtp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, len;

	mutex_lock(&sysfs_lock);
	if (readreg <= 0 || readreg > 0xFF || readlen <= 0 || readlen > 255 ||
			readpos < 0 || readpos > 255) {
		mutex_unlock(&sysfs_lock);
		return -EINVAL;
	}

	len = snprintf(buf, PAGE_SIZE,
			"addr:0x%02X pos:%d size:%d\n",
			readreg, readpos, readlen);
	for (i = 0; i < readlen; i++)
		len += snprintf(buf + len, PAGE_SIZE - len, "0x%02X%s", readbuf[i],
				(((i + 1) % 16) == 0) || (i == readlen - 1) ? "\n" : " ");

	readreg = 0;
	readpos = 0;
	readlen = 0;
	mutex_unlock(&sysfs_lock);

	return len;
}

static ssize_t read_mtp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	int ret, i;

	if (!IS_PANEL_ACTIVE(panel))
		return -EIO;

	mutex_lock(&sysfs_lock);
	ret = sscanf(buf, "%x %i %i", &readreg, &readpos, &readlen);
	if (ret != 3 || readreg <= 0 || readreg > 0xFF ||
			readlen <= 0 || readlen > 255 || readpos < 0 || readpos > 255) {
		ret = -EINVAL;
		goto store_err;
	}

	mutex_lock(&panel->op_lock);
	panel_set_key(panel, 3, true);
	ret = panel_rx_nbytes(panel, DSI_PKT_TYPE_RD, readbuf, readreg, readpos, readlen);
	panel_set_key(panel, 3, false);
	mutex_unlock(&panel->op_lock);

	if (unlikely(ret != readlen)) {
		pr_err("%s, failed to read reg %02Xh pos %d len %d\n",
				__func__, readreg, readpos, readlen);
		ret = -EIO;
		goto store_err;
	}

	pr_info("READ_Reg addr: %02x, pos : %d len : %d\n",
			readreg, readpos, readlen);
	for (i = 0; i < readlen; i++)
		pr_info("READ_Reg %dth : %02x \n", i + 1, readbuf[i]);
	mutex_unlock(&sysfs_lock);

	return size;

store_err:
	readreg = 0;
	readpos = 0;
	readlen = 0;
	mutex_unlock(&sysfs_lock);

	return ret;
}

static ssize_t mcd_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n", panel_data->props.mcd_on);

	return strlen(buf);
}

static ssize_t mcd_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.mcd_on == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.mcd_on = value;
	mutex_unlock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index_nolock(panel,
			value ? PANEL_MCD_ON_SEQ : PANEL_MCD_OFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write mcd seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s, mcd %s\n",
			__func__, panel_data->props.mcd_on ? "on" : "off");

	return size;
}

static void print_mcd_resistance(u8 *mcd_nok, int size)
{
	int code, len;
	char buf[1024];

	len = snprintf(buf, sizeof(buf),
			"MCD CHECK [b7:MCD1_R, b6:MCD2_R, b3:MCD1_L, b2:MCD2_L]\n");
	for (code = 0; code < size; code++) {
		if (!(code % 0x10))
			len += snprintf(buf + len, sizeof(buf) - len, "[%02X] ", code);
		len += snprintf(buf + len, sizeof(buf) - len, "%02X%s",
			   	mcd_nok[code], (!((code + 1) % 0x10)) ? "\n" : " ");
	}
	pr_info("%s\n", buf);
}

static int read_mcd_resistance(struct panel_device *panel)
{
	int i, ret, code;
	u8 mcd_nok[128];
	struct panel_info *panel_data = &panel->panel_data;
	int stt, end;
	u8 mcd_rs_mask[MAX_MCD_RS] = {
		(1U << 7),
		(1U << 3),
		(1U << 6),
		(1U << 2),
	};
	s64 elapsed_usec;
	struct timespec cur_ts, last_ts, delta_ts;

	ktime_get_ts(&last_ts);
	disable_irq(panel->pad.irq_disp_det);

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_MCD_RS_ON_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write mcd_3_0_on seq\n", __func__);
		goto out;
	}

	memset(mcd_nok, 0, sizeof(mcd_nok));
	for (code = 0; code < 0x80; code++) {
		panel_data->props.mcd_resistance = code;
		ret = panel_do_seqtbl_by_index_nolock(panel,
				PANEL_MCD_RS_READ_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to write mcd_rs_read seq\n", __func__);
			goto out;
		}

		ret = resource_copy_n_clear_by_name(panel_data,
				&mcd_nok[code], "mcd_resistance");
		if (unlikely(ret < 0)) {
			pr_err("%s failed to copy resource(mcd_resistance) (ret %d)\n",
					__func__, ret);
			goto out;
		}
		pr_debug("%s %02X : %02X\n", __func__, code, mcd_nok[code]);
	}
	print_mcd_resistance(mcd_nok, ARRAY_SIZE(mcd_nok));

	for (i = 0; i < MAX_MCD_RS; i++) {
		for (code = 0, stt = -1, end = -1; code < 0x80; code++) {
			if (mcd_nok[code] & mcd_rs_mask[i]) {
				if (stt == -1)
					stt = code;
				end = code;
			}
		}
		panel_data->props.mcd_rs_range[i][0] = stt;
		panel_data->props.mcd_rs_range[i][1] = end;
	}

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_MCD_RS_OFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write mcd_3_0_off seq\n", __func__);
		goto out;
	}

out:
	clear_disp_det_pend(panel);
	enable_irq(panel->pad.irq_disp_det);

	ktime_get_ts(&cur_ts);
	delta_ts = timespec_sub(cur_ts, last_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	pr_info("%s done (elapsed %2lld.%03lld msec)\n",
			__func__, elapsed_usec / 1000, elapsed_usec % 1000);

	return ret;
}

static ssize_t mcd_resistance_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	int i, len = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	mutex_lock(&panel->op_lock);
	for (i = 0; i < MAX_MCD_RS; i++)
		len += snprintf(buf + len, PAGE_SIZE - len,
				"SDC_%s:%d%s", mcd_rs_name[i],
				panel_data->props.mcd_rs_flash_range[i][1],
				(i != MAX_MCD_RS - 1) ? " " : "\n");

	for (i = 0; i < MAX_MCD_RS; i++)
		len += snprintf(buf + len, PAGE_SIZE - len,
				"%s:%d%s", mcd_rs_name[i],
				panel_data->props.mcd_rs_range[i][1],
				(i != MAX_MCD_RS - 1) ? " " : "\n");
	mutex_unlock(&panel->op_lock);

	return len;
}

static ssize_t mcd_resistance_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int i, value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
#ifdef CONFIG_SUPPORT_DDI_FLASH
	u8 flash_mcd[8];
#endif

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (!!value) {
		mutex_lock(&panel->op_lock);
		/* clear variable */
		memset(panel_data->props.mcd_rs_range, -1,
				sizeof(panel_data->props.mcd_rs_range));

		ret = read_mcd_resistance(panel);
		mutex_unlock(&panel->op_lock);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to check mcd resistance\n", __func__);
			return ret;
		}

		for (i = 0; i < MAX_MCD_RS; i++)
			pr_info("%s %s:(%d, %d)\n", __func__, mcd_rs_name[i],
					panel_data->props.mcd_rs_range[i][0],
					panel_data->props.mcd_rs_range[i][1]);

#ifdef CONFIG_SUPPORT_DDI_FLASH
		ret = set_panel_poc(&panel->poc_dev, POC_OP_MCD_READ);
		if (unlikely(ret)) {
			pr_err("%s, failed to read mcd(ret %d)\n",
					__func__, ret);
			return ret;
		}
		ret = panel_resource_update_by_name(panel, "flash_mcd");
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to update flash_mcd res (ret %d)\n",
					__func__, ret);
			return ret;
		}

		ret = resource_copy_by_name(&panel->panel_data, flash_mcd, "flash_mcd");
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to copy flash_mcd res (ret %d)\n",
					__func__, ret);
			return ret;
		}

		panel_data->props.mcd_rs_flash_range[MCD_RS_1_RIGHT][1] = flash_mcd[0];
		panel_data->props.mcd_rs_flash_range[MCD_RS_2_RIGHT][1] = flash_mcd[1];
		panel_data->props.mcd_rs_flash_range[MCD_RS_1_LEFT][1] = flash_mcd[4];
		panel_data->props.mcd_rs_flash_range[MCD_RS_2_LEFT][1] = flash_mcd[5];
		for (i = 0; i < MAX_MCD_RS; i++)
			pr_info("SDC_%s %s:(%d, %d)\n", __func__, mcd_rs_name[i],
					panel_data->props.mcd_rs_flash_range[i][0],
					panel_data->props.mcd_rs_flash_range[i][1]);
#endif
	}

	return size;
}

static ssize_t irc_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n", panel_data->props.irc_mode);

	return strlen(buf);
}

static ssize_t irc_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl;
	struct backlight_device *bd;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;
	bd = panel_bl->bd;

	if (bd == NULL) {
		panel_err("PANEL:ERR:%s:backlight device is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	mutex_lock(&panel->op_lock);
	panel_data->props.irc_mode = !!value;
	mutex_unlock(&panel->op_lock);
	backlight_update_status(bd);
	dev_info(dev, "%s, irc_mode %s\n",
			__func__, panel_data->props.irc_mode ? "on" : "off");

	return size;
}

#ifdef CONFIG_SUPPORT_MST
static ssize_t mst_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.mst_on);

	return strlen(buf);
}

static ssize_t mst_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.mst_on == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.mst_on = value;
	mutex_unlock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index(panel,
			value ? PANEL_MST_ON_SEQ : PANEL_MST_OFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write mcd seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s, mst %s\n",
			__func__, panel_data->props.mst_on ? "on" : "off");

	return size;
}
#endif


#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
u8 checksum[4] = { 0x12, 0x34, 0x56, 0x78 };
static bool gct_chksum_is_valid(struct panel_device *panel)
{
	int i;
	struct panel_info *panel_data;
	panel_data = &panel->panel_data;

	for(i = 0; i < 4; i++)
		if (checksum[i] != panel_data->props.gct_valid_chksum)
			return false;
	return true;
}

static void prepare_gct_mode(struct panel_device *panel)
{
	decon_bypass_on_global(0);
	usleep_range(90000, 100000);
	disable_irq(panel->pad.irq_disp_det);
}

static void clear_gct_mode(struct panel_device *panel)
{
	struct panel_info *panel_data = &panel->panel_data;
	int ret;

	ret = __set_panel_power(panel, PANEL_POWER_OFF);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed to set power off\n", __func__);

	ret = __set_panel_power(panel, PANEL_POWER_ON);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed to set power on\n", __func__);

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_INIT_SEQ);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed init-seq\n", __func__);

	clear_disp_det_pend(panel);
	enable_irq(panel->pad.irq_disp_det);
	panel_data->props.gct_on = GRAM_TEST_OFF;
	decon_bypass_off_global(0);
	usleep_range(90000, 100000);
}

static ssize_t gct_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u 0x%x\n",
			gct_chksum_is_valid(panel) ? 1 : 0,
			*(u32 *)checksum);

	return strlen(buf);
}

static ssize_t gct_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret, result = 0;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct seqinfo *seqtbl;
	int index = 0, vddm = 0, pattern = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (value != GRAM_TEST_ON)
		return -EINVAL;

	/* clear checksum buffer */
	checksum[0] = 0x12;
	checksum[1] = 0x34;
	checksum[2] = 0x56;
	checksum[3] = 0x78;

	mutex_lock(&panel->io_lock);
	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		mutex_unlock(&panel->io_lock);
		return -EAGAIN;
	}

	if (panel->state.cur_state == PANEL_STATE_ALPM) {
		pr_warn("%s gct not supported on LPM\n", __func__);
		mutex_unlock(&panel->io_lock);
		return -EINVAL;
	}

	copr_disable(&panel->copr);
	mdnie_disable(&panel->mdnie);

	mutex_lock(&panel->mdnie.lock);
	mutex_lock(&panel->op_lock);
	prepare_gct_mode(panel);
	panel_data->props.gct_on = value;

#ifdef CONFIG_SUPPORT_AFC
	if (panel->mdnie.props.afc_on &&
			panel->mdnie.nr_seqtbl > MDNIE_AFC_OFF_SEQ) {
		pr_info("%s afc off\n", __func__);
		ret = panel_do_seqtbl(panel, &panel->mdnie.seqtbl[MDNIE_AFC_OFF_SEQ]);
		if (unlikely(ret < 0))
			pr_err("%s, failed to write afc off seqtbl\n", __func__);
	}
#endif

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_GCT_ENTER_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write gram-checksum-test-enter seq\n", __func__);
		result = ret;
		goto out;
	}

	for (vddm = VDDM_LV; vddm < MAX_VDDM; vddm++) {
		panel_data->props.gct_vddm = vddm;
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_GCT_VDDM_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to write gram-checksum-on seq\n", __func__);
			result = ret;
			goto out;
		}

		for (pattern = GCT_PATTERN_1; pattern < MAX_GCT_PATTERN; pattern++) {
			panel_data->props.gct_pattern = pattern;
			seqtbl = find_index_seqtbl(&panel->panel_data,
					PANEL_GCT_IMG_UPDATE_SEQ);
			ret = panel_do_seqtbl_by_index_nolock(panel,
					(seqtbl && seqtbl->cmdtbl) ? PANEL_GCT_IMG_UPDATE_SEQ :
					((pattern == GCT_PATTERN_1) ?
					 PANEL_GCT_IMG_0_UPDATE_SEQ : PANEL_GCT_IMG_1_UPDATE_SEQ));
			if (unlikely(ret < 0)) {
				pr_err("%s, failed to write gram-img-update seq\n", __func__);
				result = ret;
				goto out;
			}

			ret = resource_copy_n_clear_by_name(panel_data,
					&checksum[index], "gram_checksum");
			if (unlikely(ret < 0)) {
				pr_err("%s failed to copy gram_checksum[%d] (ret %d)\n",
						__func__, index, ret);
				result = ret;
				goto out;
			}
			pr_info("%s gram_checksum[%d] 0x%x\n",
					__func__, index, checksum[index]);
			index++;
		}
	}

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_GCT_EXIT_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write gram-checksum-off seq\n", __func__);
		result = ret;
	}
out:
	clear_gct_mode(panel);
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel->mdnie.lock);
#ifdef CONFIG_EXTEND_LIVE_CLOCK
	ret = panel_aod_init_panel(panel);
	if (ret)
		panel_err("PANEL:ERR:%s:failed to aod init_panel\n", __func__);
#endif
	mutex_unlock(&panel->io_lock);

	ret = panel_display_on(panel);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed to display on\n", __func__);


	if (result < 0)
		return result;

	pr_info("%s chksum %s 0x%0x\n", __func__,
			gct_chksum_is_valid(panel) ? "ok" : "nok",
			*(u32 *)checksum);

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_XTALK_MODE
static ssize_t xtalk_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.xtalk_mode);

	return strlen(buf);
}

static ssize_t xtalk_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_info *panel_data;
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct backlight_device *bd;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;
	bd = panel_bl->bd;

	if (bd == NULL) {
		panel_err("PANEL:ERR:%s:backlight device is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.xtalk_mode == value)
		return size;

	mutex_lock(&panel_bl->lock);
	panel_data->props.xtalk_mode = value;
	mutex_unlock(&panel_bl->lock);
	backlight_update_status(bd);

	dev_info(dev, "%s, xtalk_mode %d\n",
			__func__, panel_data->props.xtalk_mode);

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_POC_FLASH
static ssize_t poc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_poc_device *poc_dev;
	struct panel_poc_info *poc_info;
	int ret;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return -EAGAIN;
	}

	poc_dev = &panel->poc_dev;
	poc_info = &poc_dev->poc_info;

	ret = set_panel_poc(poc_dev, POC_OP_CHECKPOC);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to chkpoc (ret %d)\n", __func__, ret);
		return ret;
	}

	ret = set_panel_poc(poc_dev, POC_OP_CHECKSUM);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to chksum (ret %d)\n", __func__, ret);
		return ret;
	}

	snprintf(buf, PAGE_SIZE, "%d %d %02x\n", poc_info->poc,
			poc_info->poc_chksum[4], poc_info->poc_ctrl[3]);

	dev_info(dev, "%s poc:%d chk:%d gray:%02x\n", __func__, poc_info->poc,
			poc_info->poc_chksum[4], poc_info->poc_ctrl[3]);

	return strlen(buf);
}

static ssize_t poc_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_info *panel_data;
	struct panel_poc_device *poc_dev;
	struct panel_poc_info *poc_info;
	int rc, ret;
	unsigned int value;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	poc_dev = &panel->poc_dev;
	poc_info = &poc_dev->poc_info;

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (!IS_VALID_POC_OP(value)) {
		pr_warn("%s invalid poc_op %d\n", __func__, value);
		return -EINVAL;
	}

	if (value == POC_OP_WRITE || value == POC_OP_READ) {
		pr_warn("%s unsupported poc_op %d\n", __func__, value);
		return size;
	}

	if (value == POC_OP_CANCEL) {
		atomic_set(&poc_dev->cancel, 1);
	} else {
		mutex_lock(&panel->io_lock);
		ret = set_panel_poc(poc_dev, value);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to poc_op %d(ret %d)\n", __func__, value, ret);
			mutex_unlock(&panel->io_lock);
			return -EINVAL;
		}
		mutex_unlock(&panel->io_lock);
	}

	mutex_lock(&panel->op_lock);
	panel_data->props.poc_op = value;
	mutex_unlock(&panel->op_lock);

	dev_info(dev, "%s poc_op %d\n",
			__func__, panel_data->props.poc_op);

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_DIM_FLASH
static u16 calc_checksum_16bit(u8 *arr, int size)
{
	u16 chksum = 0;
	int i;

	for (i = 0; i < size; i++)
		chksum += arr[i];

	return chksum;
}

/*
 * gamma_flash_show() function returns read state and checksum.
 * @read_state(-1 : FAILED, 0 : PROGRESS, 1 : DONE)
 * @checksum(XXXXXXXX XXXXXXXX)
 * The first checksum is by calculation of parameters
 * and the other one is by reading checksum parameter.
 */
static ssize_t gamma_flash_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct dim_flash_result *result = &panel->dim_flash_work.result;
	int ret;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	/* thread is running */
	if (atomic_read(&panel->dim_flash_work.running))
		ret = GAMMA_FLASH_PROGRESS;
	else
		ret = panel->dim_flash_work.ret;

	pr_info("%s result %d, dim chksum(calc:%04X read:%04X), "
			"mtp chksum(reg:%04X, calc:%04X, read:%04X)\n", __func__,
			ret, result->dim_chksum_by_calc, result->dim_chksum_by_read,
			calc_checksum_16bit(result->mtp_reg, sizeof(result->mtp_reg)),
			result->mtp_chksum_by_calc, result->mtp_chksum_by_read);

	return snprintf(buf, PAGE_SIZE, "%d %08X %08X %08X %08X %08X\n",
			ret, result->dim_chksum_by_calc, result->dim_chksum_by_read,
			calc_checksum_16bit(result->mtp_reg, sizeof(result->mtp_reg)),
			result->mtp_chksum_by_calc, result->mtp_chksum_by_read);
}

static ssize_t gamma_flash_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct backlight_device *bd = panel->panel_bl.bd;
	int rc;
	unsigned int value;

	if (!IS_PANEL_ACTIVE(panel))
		return -ENODEV;

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (atomic_read(&panel->dim_flash_work.running))
		return -EBUSY;

	panel->dim_flash_work.ret = GAMMA_FLASH_ERROR_NOT_EXIST;
	memset(&panel->dim_flash_work.result, 0, sizeof(struct dim_flash_result));
	if (value == 0) {
		panel_update_dim_type(panel, DIM_TYPE_AID_DIMMING);
		backlight_update_status(bd);
	} else if (value == 1) {
		queue_delayed_work(panel->dim_flash_work.wq,
				&panel->dim_flash_work.dwork, msecs_to_jiffies(0));
	}

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
static ssize_t grayspot_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.grayspot);

	return strlen(buf);
}

static ssize_t grayspot_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.grayspot == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.grayspot = value;
	mutex_unlock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index(panel,
			value ? PANEL_GRAYSPOT_ON_SEQ : PANEL_GRAYSPOT_OFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write grayspot on/off seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s, grayspot %s\n",
			__func__, panel_data->props.grayspot ? "on" : "off");

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_HMD
static ssize_t hmt_bright_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = dev_get_drvdata(dev);
	int size;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_bl = &panel->panel_bl;
	mutex_lock(&panel_bl->lock);
	if (panel_bl->props.id == PANEL_BL_SUBDEV_TYPE_DISP) {
		size = snprintf(buf, 30, "HMD off state\n");
	} else {
		size = snprintf(buf, PAGE_SIZE, "index : %d, brightenss : %d\n",
				panel_bl->props.actual_brightness_index,
				BRT_USER(panel_bl->props.brightness));
	}
	mutex_unlock(&panel_bl->lock);
	return size;
}

static ssize_t hmt_bright_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int value, rc;
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_bl = &panel->panel_bl;

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	panel_info("PANEL:INFO:%s: brightness : %d\n",
			__func__, value);

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);

	if (panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_HMD].brightness != BRT(value)) {
		panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_HMD].brightness = BRT(value);
	}

	if (panel->state.hmd_on != PANEL_HMD_ON) {
		panel_info("PANEL:WARN:%s: hmd off\n", __func__);
		goto exit_store;
	}

	ret = panel_bl_set_brightness(panel_bl, PANEL_BL_SUBDEV_TYPE_HMD, 1);
	if (ret) {
		pr_err("%s : fail to set brightness\n", __func__);
		goto exit_store;
	}

exit_store:
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);
	return size;
}

static ssize_t hmt_on_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_state *state;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	state = &panel->state;

	snprintf(buf, PAGE_SIZE, "%u\n", state->hmd_on);

	return strlen(buf);
}

static ssize_t hmt_on_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int value, rc;
	struct backlight_device *bd;
	struct panel_state *state;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_bl = &panel->panel_bl;
	bd = panel_bl->bd;
	state = &panel->state;

	rc = kstrtoint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (value != PANEL_HMD_OFF && value != PANEL_HMD_ON) {
		pr_err("%s invalid parameter %d\n", __func__, value);
		return -EINVAL;
	}

	panel_info("PANEL:INFO:%s: hmt %s\n",
			__func__, (value == PANEL_HMD_ON) ? "on" : "off");

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);

	if (value == state->hmd_on) {
		panel_warn("PANEL:WARN:%s:already set : %d\n",
			__func__, value);
	} else {
		ret = panel_do_seqtbl_by_index_nolock(panel,
				(value == PANEL_HMD_ON) ? PANEL_HMD_ON_SEQ : PANEL_HMD_OFF_SEQ);
		if (ret < 0)
			pr_err("%s failed to set hmd %s seq\n", __func__,
					(value == PANEL_HMD_ON) ? "on" : "off");
	}

	ret = panel_bl_set_brightness(panel_bl, (value == PANEL_HMD_ON) ?
			PANEL_BL_SUBDEV_TYPE_HMD : PANEL_BL_SUBDEV_TYPE_DISP, 1);
	if (ret < 0)
		pr_err("%s failed to set %s brightness\n", __func__,
					(value == PANEL_HMD_ON) ? "hmd" : "normal");
	state->hmd_on = value;

	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);

	return size;
}
#endif /* CONFIG_SUPPORT_HMD */

#ifdef CONFIG_SUPPORT_DOZE
static int set_alpm_mode(struct panel_device *panel, int mode)
{
	int ret = 0;
#ifdef CONFIG_SUPPORT_AOD_BL
	int lpm_ver = (mode & 0x00FF0000) >> 16;
#endif
	int lpm_mode = (mode & 0xFF);
#ifdef CONFIG_SEC_FACTORY
	static int backup_br;
#endif
	struct panel_info *panel_data = &panel->panel_data;
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct backlight_device *bd = panel_bl->bd;

	switch (lpm_mode) {
	case ALPM_OFF:
		mutex_lock(&panel_bl->lock);
		mutex_lock(&panel->op_lock);
#ifdef CONFIG_SEC_FACTORY
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_ALPM_EXIT_SEQ);
		if (unlikely(ret < 0))
			panel_err("PANEL:ERR:%s, failed to alpm-exit\n", __func__);
#endif
		panel_data->props.alpm_mode = lpm_mode;
#ifdef CONFIG_SUPPORT_AOD_BL
		panel->panel_data.props.lpm_brightness = -1;
		panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP);
#endif
#ifdef CONFIG_SEC_FACTORY
		if (backup_br)
			bd->props.brightness = backup_br;
#endif
		mutex_unlock(&panel->op_lock);
		mutex_unlock(&panel_bl->lock);
		backlight_update_status(bd);
		break;
	case ALPM_LOW_BR:
	case HLPM_LOW_BR:
	case ALPM_HIGH_BR:
	case HLPM_HIGH_BR:
		mutex_lock(&panel_bl->lock);
#ifdef CONFIG_SEC_FACTORY
		if (panel_data->props.alpm_mode == ALPM_OFF) {
			backup_br = bd->props.brightness;
			bd->props.brightness = UI_MIN_BRIGHTNESS;
			panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_DISP].brightness =
				UI_MIN_BRIGHTNESS;
			ret = panel_bl_set_brightness(panel_bl,
					PANEL_BL_SUBDEV_TYPE_DISP, 1);
			if (ret)
				pr_err("%s, failed to set_brightness (ret %d)\n",
						__func__, ret);
		}
#endif
		mutex_lock(&panel->op_lock);
		panel_data->props.alpm_mode = lpm_mode;

#ifndef CONFIG_SEC_FACTORY
		if (panel->state.cur_state != PANEL_STATE_ALPM) {
			panel_info("PANEL:INFO:%s:panel state(%d) is not lpm mode\n",
					__func__, panel->state.cur_state);
			mutex_unlock(&panel->op_lock);
			mutex_unlock(&panel_bl->lock);
			return ret;
		}
#endif
#ifdef CONFIG_SUPPORT_AOD_BL
		if (lpm_ver == 0) {
			bd->props.brightness =
				(lpm_mode <= HLPM_LOW_BR) ? BRT(0) : BRT(94);
			panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_AOD].brightness =
				(lpm_mode <= HLPM_LOW_BR) ? BRT(0) : BRT(94);
		}
#endif
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_ALPM_ENTER_SEQ);
		if (ret)
			panel_err("PANEL:ERR:%s, failed to alpm-enter\n", __func__);
#ifdef CONFIG_SUPPORT_AOD_BL
		panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_AOD);
#endif
		mutex_unlock(&panel->op_lock);
		mutex_unlock(&panel_bl->lock);
		break;
	default:
		panel_err("PANEL:ERR:%s:invalid alpm_mode: %d\n",
				__func__, lpm_mode);
		break;
	}

	return ret;
}
#endif

static ssize_t alpm_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_info *panel_data = &panel->panel_data;

	mutex_lock(&panel->io_lock);
	rc = kstrtoint(buf, 0, &value);
	if (rc < 0) {
		pr_warn("%s invalid param (ret %d)\n",
				__func__, rc);
		mutex_unlock(&panel->io_lock);
		return rc;
	}

#ifdef CONFIG_SUPPORT_DOZE
	if (set_alpm_mode(panel, value)) {
		panel_err("PANEL:ERR:%s:failed to set alpm (value %d, ret %d)\n",
				__func__, value, ret);
		goto exit_store;
	}
#endif
	panel_info("PANEL:INFO:%s:value %d, alpm_mode %d\n",
			__func__, value, panel_data->props.alpm_mode);

exit_store:
	mutex_unlock(&panel->io_lock);
	return size;
}

static ssize_t alpm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n", panel_data->props.alpm_mode);
	panel_dbg("%s: %d\n", __func__, panel_data->props.alpm_mode);

	return strlen(buf);
}

static ssize_t lpm_opr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_info *panel_data = &panel->panel_data;

	mutex_lock(&panel->io_lock);
	rc = kstrtoint(buf, 0, &value);
	if (rc < 0) {
		pr_warn("%s invalid param (ret %d)\n",
				__func__, rc);
		mutex_unlock(&panel->io_lock);
		return rc;
	}

	mutex_lock(&panel->op_lock);
	panel_data->props.lpm_opr = value;
	mutex_unlock(&panel->op_lock);
	backlight_update_status(panel->panel_bl.bd);

	panel_info("PANEL:INFO:%s:value %d, lpm_opr %d\n",
			__func__, value, panel_data->props.lpm_opr);

	mutex_unlock(&panel->io_lock);
	return size;
}

static ssize_t lpm_opr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n", panel_data->props.lpm_opr);

	return strlen(buf);
}

#ifdef CONFIG_PANEL_AID_DIMMING_DEBUG
char *normal_tbl_names[] = {
	"gamma_table",
	"aor_table",
	"vint_table",
	"elvss_table",
	"irc_table",
};
char *hmd_tbl_names[] = {
	"hmd_gamma_table",
	"hmd_aor_table",
};

char *brt_res_names[] = {
	"gamma",
	"aor",
	"vint",
	"elvss_t",
	"irc",
};
char *line[4096];

static int buffer_backup(u8 *buf, int size, char *name)
{
	struct file *fp;
	mm_segment_t old_fs;

	if (!name)
		return -1;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pr_info("%s filename %s size %d\n", __func__, name, size);
	fp = filp_open(name, O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0660);
	if (IS_ERR(fp)) {
		pr_err("%s, fail to open %s file\n", __func__, name);
		goto open_err;
	}

	vfs_write(fp, (u8 __user *)buf, size, &fp->f_pos);
	pr_info("%s filename %s write %d bytes done!!\n",
			__func__, name, size);

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;

 open_err:
	set_fs(old_fs);
	return -1;
}

static void show_brt_param(struct panel_info *panel_data, int id, int type)
{
	char reg_val[256];
	char *buf;
	int i, num, size, count = 0, len = 0, ret = 0;
	size_t ires, itemp;
	struct resinfo *res;
	int sz_tbl;
	u32 *tbl;
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct panel_bl_sub_dev *subdev;
	int orig_temperature, temperatures[] = { 23, 0, -15 };
	const char * const path[] = {
		"/data/brightness.csv",
		"/data/hmd_brightness.csv",
		"/data/aod_brightness.csv"
	};

	if (unlikely(!panel_data)) {
		panel_err("%s:panel is NULL\n", __func__);
		return;
	}

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s invalid bl-%d\n", __func__, id);
		return;
	}

	subdev = &panel_bl->subdev[id];

	if (type == 1) {
		/* brightness mode table */
		tbl = subdev->brt_tbl.brt;
		sz_tbl = subdev->brt_tbl.sz_brt;
	} else if (type == 2) {
		/* detail brightness mode table */
		tbl = subdev->brt_tbl.brt_to_step;
		sz_tbl = subdev->brt_tbl.sz_brt_to_step;
	}

	/* columns */
	buf = kmalloc(SZ_1K, GFP_KERNEL);
	if (!buf)
		return;

	line[count++] = buf;
	len = snprintf(buf, SZ_1K, "Brightness");
	for (ires = 0; ires < ARRAY_SIZE(brt_res_names); ires++) {
		res = find_panel_resource(panel_data, brt_res_names[ires]);
		size = get_panel_resource_size(res);
		if (!strncmp(brt_res_names[ires], "elvss_t", 8))
			for (itemp = 0; itemp < ARRAY_SIZE(temperatures); itemp++)
				len += snprintf(buf + len, SZ_1K - len, ",%selvss(T:%d)",
						(id == PANEL_BL_SUBDEV_TYPE_HMD) ? "hmd_" : "",
						temperatures[itemp]);
		else
			for (num = 0; num < size; num++)
				len += snprintf(buf + len, SZ_1K - len, ",%s%s_%d",
						(id == PANEL_BL_SUBDEV_TYPE_HMD) ? "hmd_" : "",
						brt_res_names[ires], num);
	}

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);
	if (id == PANEL_BL_SUBDEV_TYPE_HMD)
		panel_do_seqtbl_by_index_nolock(panel, PANEL_HMD_ON_SEQ);

	for (i = 0; i < sz_tbl; i++) {
		panel_bl->bd->props.brightness = tbl[i];
		subdev->brightness = tbl[i];
		ret = panel_bl_set_brightness(panel_bl, id, 1);
		if (ret < 0) {
			pr_err("%s, failed to set_brightness (ret %d)\n",
					__func__, ret);
			break;
		}

		buf = kmalloc(SZ_1K, GFP_KERNEL);
		if (!buf)
			break;

		line[count++] = buf;
		len = snprintf(buf, SZ_1K, "%3d", tbl[i]);

		panel_set_key(panel, 3, true);
		/* store temperature */
		orig_temperature = panel_data->props.temperature;
		for (ires = 0; ires < ARRAY_SIZE(brt_res_names); ires++) {
			res = find_panel_resource(panel_data, brt_res_names[ires]);
			size = get_panel_resource_size(res);
			if (!strncmp(brt_res_names[ires], "elvss_t", 8)) {
				for (itemp = 0; itemp < ARRAY_SIZE(temperatures); itemp++) {
					/* update temperature */
					panel_data->props.temperature = temperatures[itemp];
					ret = panel_bl_set_brightness(panel_bl, id, 1);
					if (ret < 0) {
						pr_err("%s, failed to set_brightness (ret %d)\n",
								__func__, ret);
						break;
					}

					ret = panel_resource_update(panel, res);
					if (ret < 0) {
						pr_err("%s, failed to resource_update (ret %d)\n",
								__func__, ret);
						break;
					}

					resource_copy(reg_val, res);
					len += snprintf(buf + len, SZ_1K - len, ",0x%02X",
							reg_val[0]);
				}
			} else {
				ret = panel_resource_update(panel, res);
				if (ret < 0) {
					pr_err("%s, failed to resource_update (ret %d)\n",
							__func__, ret);
					break;
				}
				resource_copy(reg_val, res);
				for (num = 0; num < size; num++)
					len += snprintf(buf + len, SZ_1K - len, ",0x%02X",
							reg_val[num]);
			}
		}
		/* restore temperature */
		panel_data->props.temperature = orig_temperature;
		panel_set_key(panel, 3, false);

		if (ret < 0)
			break;
	}

	if (id == PANEL_BL_SUBDEV_TYPE_HMD) {
		panel_do_seqtbl_by_index_nolock(panel, PANEL_HMD_OFF_SEQ);
		panel_bl_set_brightness(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP, 1);
	}

	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);

	buf = kmalloc(SZ_1K * count, GFP_KERNEL);
	if (buf) {
		len = 0;
		for (i = 0; i < count; i++)
			len += snprintf(buf + len,
					SZ_1K * count - len, "%s\n", line[i]);
		buffer_backup(buf, len, (char *)path[id]);
		kfree(buf);
	}

	for (i = 0; i < count; i++) {
		pr_info("brt_param:%s\n", line[i]);
		kfree(line[i]);
	}
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
static void show_aid_log(struct panel_info *panel_data, int id)
{
	struct dimming_info *dim_info;
	struct maptbl *tbl = NULL;
	int layer, row, col, i, len = 0;
	char *buf;
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct panel_bl_sub_dev *subdev;
	char **tbl_names;
	int count = 0, nr_tbl_names;

	if (unlikely(!panel_data)) {
		panel_err("%s:panel is NULL\n", __func__);
		return;
	}

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s invalid bl-%d\n", __func__, id);
		return;
	}

	subdev = &panel_bl->subdev[id];

	dim_info = panel_data->dim_info[id];
	if (!dim_info) {
		panel_warn("%s bl-%d dim_info is null\n", __func__, id);
		return;
	}

	pr_info("[====================== [%s] ======================]\n",
			(id == PANEL_BL_SUBDEV_TYPE_HMD ? "HMD" : "DISP"));
	print_dimming_info(dim_info, TAG_MTP_OFFSET_START);
	print_dimming_info(dim_info, TAG_GAMMA_CENTER_START);

	if (id == PANEL_BL_SUBDEV_TYPE_HMD) {
		tbl_names = hmd_tbl_names;
		nr_tbl_names = ARRAY_SIZE(hmd_tbl_names);
	} else {
		tbl_names = normal_tbl_names;
		nr_tbl_names = ARRAY_SIZE(normal_tbl_names);
	}

	for (i = 0; i < nr_tbl_names; i++) {
		tbl = find_panel_maptbl_by_name(panel_data, tbl_names[i]);
		buf = kmalloc(SZ_1K, GFP_KERNEL);
		line[count++] = buf;
		len = snprintf(buf, SZ_1K, "[MAPTBL:%s]", tbl_names[i]);
		for_each_layer(tbl, layer) {
			for_each_row(tbl, row) {
				buf = kmalloc(SZ_1K, GFP_KERNEL);
				line[count++] = buf;
				len = snprintf(buf, SZ_1K, "lum[%3d] : ",
						subdev->brt_tbl.lum[row]);
				for_each_col(tbl, col)
					len += snprintf(buf + len, SZ_1K - len, "%02X ",
							tbl->arr[row * sizeof_row(tbl) + col]);
			}
		}
	}

	for (i = 0; i < count; i++) {
		pr_info("%s\n", line[i]);
		kfree(line[i]);
	}
}
#else
static void show_aid_log(struct panel_info *panel_data, int id)
{
	struct dimming_info *dim_info;
	struct maptbl *tbl = NULL, *aor_tbl = NULL;
	struct maptbl *irc_tbl = NULL, *elvss_tbl = NULL, *vint_tbl = NULL;
	int layer, row, col, i, len = 0;
	char buf[1024];
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct panel_bl_sub_dev *subdev;

	if (unlikely(!panel_data)) {
		panel_err("%s:panel is NULL\n", __func__);
		return;
	}

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s invalid bl-%d\n", __func__, id);
		return;
	}

	subdev = &panel_bl->subdev[id];

	pr_info("[====================== [%s] ======================]\n",
			(id == PANEL_BL_SUBDEV_TYPE_HMD ? "HMD" : "DISP"));
	dim_info = panel_data->dim_info[id];
	if (!dim_info) {
		panel_warn("%s bl-%d dim_info is null\n", __func__, id);
	} else {
		print_dimming_info(dim_info, TAG_MTP_OFFSET_START);
		print_dimming_info(dim_info, TAG_TP_VOLT_START);
		print_dimming_info(dim_info, TAG_GRAY_SCALE_VOLT_START);
		print_dimming_info(dim_info, TAG_GAMMA_CENTER_START);
	}

	/* TODO : 0 means GAMMA_MAPTBL.
	 * To use commonly in panel driver some maptbl index should be same
	 */
	if (id == PANEL_BL_SUBDEV_TYPE_HMD)
		tbl = find_panel_maptbl_by_name(panel_data, "hmd_gamma_table");
	else
		tbl = find_panel_maptbl_by_name(panel_data, "gamma_table");

	if (tbl) {
		pr_info("[GAMMA MAPTBL] HEXA-DECIMAL\n");
		for_each_layer(tbl, layer) {
			for_each_row(tbl, row) {
				len = snprintf(buf, sizeof(buf), "gamma[%3d] : ",
						subdev->brt_tbl.lum[row]);
				for_each_col(tbl, col)
					len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
							tbl->arr[row * sizeof_row(tbl) + col]);
				pr_info("%s\n", buf);
			}
		}
	}

	if (id == PANEL_BL_SUBDEV_TYPE_HMD) {
		aor_tbl = find_panel_maptbl_by_name(panel_data, "hmd_aor_table");
	} else {
		aor_tbl = find_panel_maptbl_by_name(panel_data, "aor_table");
		vint_tbl = find_panel_maptbl_by_name(panel_data, "vint_table");
		elvss_tbl = find_panel_maptbl_by_name(panel_data, "elvss_table");
		irc_tbl = find_panel_maptbl_by_name(panel_data, "irc_table");
	}

	panel_info("\n[BRIGHTNESS, %s %s %s %s table]\n",
			aor_tbl ? "AOR" : "", vint_tbl ? "VINT" : "",
			elvss_tbl ? "ELVSS" : "", irc_tbl ? "IRC" : "");
	len = snprintf(buf, sizeof(buf), "[idx] platform   luminance  ");
	if (aor_tbl)
		len += snprintf(buf + len, sizeof(buf) - len, "|  aor  ");
	if (vint_tbl)
		len += snprintf(buf + len, sizeof(buf) - len,
				"| vint ");
	if (elvss_tbl)
		len += snprintf(buf + len, sizeof(buf) - len,
				"|  elvss   ");
	if (irc_tbl)
		len += snprintf(buf + len, sizeof(buf) - len,
				"| ====================== irc =======================");
	pr_info("%s\n", buf);

	for (i = 0; i < subdev->brt_tbl.sz_brt_to_step; i++) {
		len = snprintf(buf, sizeof(buf),
				"[%3d]   %-5d   %3d(%3d.%02d) ",
				i, subdev->brt_tbl.brt_to_step[i],
				get_subdev_actual_brightness(panel_bl, id,
					subdev->brt_tbl.brt_to_step[i]),
				get_subdev_actual_brightness_interpolation(panel_bl, id,
					subdev->brt_tbl.brt_to_step[i]) / 100,
				get_subdev_actual_brightness_interpolation(panel_bl, id,
					subdev->brt_tbl.brt_to_step[i]) % 100);
		if (aor_tbl) {
			len += snprintf(buf + len, sizeof(buf) - len, "| ");
			for_each_col(aor_tbl, col)
				len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
						aor_tbl->arr[i * sizeof_row(aor_tbl) + col]);
		}

		if (vint_tbl) {
			len += snprintf(buf + len, sizeof(buf) - len, "| ");
			for_each_col(vint_tbl, col)
				len += snprintf(buf + len, sizeof(buf) - len, " %02X  ",
						vint_tbl->arr[i * sizeof_row(vint_tbl) + col]);
		}

		if (elvss_tbl) {
			len += snprintf(buf + len, sizeof(buf) - len, "| ");
			for_each_layer(elvss_tbl, layer)
				for_each_col(elvss_tbl, col)
				len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
						elvss_tbl->arr[maptbl_index(elvss_tbl, layer, i, col)]);
		}

		if (irc_tbl) {
			len += snprintf(buf + len, sizeof(buf) - len, "| ");
			for_each_col(irc_tbl, col)
				len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
						irc_tbl->arr[i * sizeof_row(irc_tbl) + col]);
		}

		pr_info("%s\n", buf);
	}
}
#endif /* CONFIG_SUPPORT_DIM_FLASH */

static ssize_t aid_log_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	print_panel_resource(panel);

	show_aid_log(panel_data, PANEL_BL_SUBDEV_TYPE_DISP);
#ifdef CONFIG_SUPPORT_HMD
	show_aid_log(panel_data, PANEL_BL_SUBDEV_TYPE_HMD);
#endif

	return strlen(buf);
}

static ssize_t aid_log_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_device *panel = dev_get_drvdata(dev);

	rc = kstrtoint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (value == 1 || value == 2) {
		/* read brightness & make csv */
		show_brt_param(&panel->panel_data, PANEL_BL_SUBDEV_TYPE_DISP, value);
#ifdef CONFIG_SUPPORT_HMD
		show_brt_param(&panel->panel_data, PANEL_BL_SUBDEV_TYPE_HMD, value);
#endif
	}

	return size;
}
#endif /* CONFIG_PANEL_AID_DIMMING_DEBUG */

#ifdef CONFIG_LCD_WEAKNESS_CCB
void ccb_set_mode(struct panel_device *panel, u8 ccb, int stepping)
{
#if 0
	u8 ccb_cmd[3] = {0xB7, 0x00, 0x00};
	u8 secondval = 0;

	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	switch (dsim->priv.panel_type) {

	case LCD_TYPE_S6E3HF4_WQHD:
		ccb_cmd[1] = ccb;
		ccb_cmd[2] = 0x2A;
		dsim_write_hl_data(dsim, ccb_cmd, ARRAY_SIZE(ccb_cmd));
		break;
	case LCD_TYPE_S6E3HA3_WQHD:
		if((ccb & 0x0F) == 0x00) {		// off
			if(stepping) {
				ccb_cmd[1] = dsim->priv.current_ccb;
				for(secondval = 0x2A; secondval <= 0x3F; secondval += 1) {
					ccb_cmd[2] = secondval;
					dsim_write_hl_data(dsim, ccb_cmd, ARRAY_SIZE(ccb_cmd));
					msleep(17);
				}
			}
			ccb_cmd[1] = 0x00;
			ccb_cmd[2] = 0x3F;
			dsim_write_hl_data(dsim, ccb_cmd, ARRAY_SIZE(ccb_cmd));
		} else {						// on
			ccb_cmd[1] = ccb;
			if(stepping) {
				for(secondval = 0x3F; secondval >= 0x2A; secondval -= 1) {
					ccb_cmd[2] = secondval;
					dsim_write_hl_data(dsim, ccb_cmd, ARRAY_SIZE(ccb_cmd));
					if(secondval != 0x2A)
						msleep(17);
				}
			} else {
				ccb_cmd[2] = 0x2A;
				dsim_write_hl_data(dsim, ccb_cmd, ARRAY_SIZE(ccb_cmd));
			}
		}
		msleep(17);
		break;
	default:
		pr_info("%s unknown panel \n", __func__);
		break;
	}

	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
#endif
}

static ssize_t weakness_ccb_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t weakness_ccb_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, type, serverity;
	unsigned char set_ccb = 0x00;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	ret = sscanf(buf, "%d %d", &type, &serverity);
	if (ret < 0)
		return ret;

	dev_info(dev, "%s: type = %d serverity = %d\n", __func__, type, serverity);

	if (panel_data->ccb_support == UNSUPPORT_CCB) {
		pr_info("%s, CCB unsupported\n", __func__);
		return size;
	}

	if ((type < 0) || (type > 3)) {
		dev_info(dev, "%s: type is out of range\n", __func__);
		return -EINVAL;
	}

	if ((serverity < 0) || (serverity > 9)) {
		dev_info(dev, "%s: serverity is out of range\n", __func__);
		return -EINVAL;
	}

	set_ccb = ((u8)(serverity) << 4);
	switch (type) {
		case 0:
			set_ccb = 0;
			panel_dbg("%s: disable ccb\n", __func__);
			break;
		case 1:
			set_ccb += 1;
			panel_dbg("%s: enable red\n", __func__);
			break;
		case 2:
			set_ccb += 5;
			panel_dbg( "%s: enable green\n", __func__);
			break;
		case 3:
			if (serverity == 0) {
				set_ccb += 9;
				panel_dbg("%s: enable blue\n", __func__);
			} else {
				set_ccb = 0;
				set_ccb += 9;
				panel_dbg("%s, serverity is out of range, blue only support 0\n", __func__);
			}
			break;
		default:
			set_ccb = 0;
			break;
	}
	if (panel->current_ccb == set_ccb) {
		panel_dbg("%s: aleady set same ccb\n", __func__);
	} else {
		ccb_set_mode(panel, set_ccb, 1);
		panel_data->current_ccb = set_ccb;
	}
	dev_info(dev, "%s: --\n", __func__);

	return size;
}
#endif /* CONFIG_LCD_WEAKNESS_CCB */

#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
static ssize_t lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n", panel_data->props.lux);

	return strlen(buf);
}

static ssize_t lux_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;
	int value;
	struct mdnie_info *mdnie;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	mdnie = &panel->mdnie;
	panel_data = &panel->panel_data;

	rc = kstrtoint(buf, 0, &value);

	if (rc < 0)
		return rc;

	if (panel_data->props.lux != value) {
		mutex_lock(&panel->op_lock);
		panel_data->props.lux = value;
		mutex_unlock(&panel->op_lock);
		attr_store_for_each(mdnie->class, attr->attr.name, buf, size);
	}

	return size;
}
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */

#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
static ssize_t copr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct copr_info *copr = &panel->copr;
	int copr_avg, brt_avg;

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	if (!copr_is_enabled(copr)) {
		panel_err("%s copr is off state\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	copr_avg = copr_get_average_and_clear(copr);
	if (copr_avg < 0) {
		panel_err("%s failed to get average copr\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	brt_avg = panel_bl_get_average_and_clear(panel_bl, 0);
	if (brt_avg < 0) {
		panel_err("%s failed to get average brt0\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	return snprintf(buf, PAGE_SIZE, "%d %d\n", copr_avg, brt_avg);
}

static ssize_t copr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct copr_info *copr = &panel->copr;
	int copr_en, copr_gamma, copr_er, copr_eg, copr_eb;
	int roi_on, roi_xs, roi_ys, roi_xe, roi_ye;
	int rc;

	rc = sscanf(buf, "%i %i %i %i %i %i %i %i %i %i",
			&copr_en, &copr_gamma, &copr_er, &copr_eg, &copr_eb,
			&roi_on, &roi_xs, &roi_ys, &roi_xe, &roi_ye);
	if (rc < 0)
		return rc;

	mutex_lock(&copr->lock);
	if (copr->props.reg.copr_en != !!copr_en) {
		copr->props.reg.copr_en = !!copr_en;
		copr->props.state = COPR_UNINITIALIZED;
		pr_info("%s copr %s\n", __func__, copr_en ? "enable" : "disable");
	}

	if ((copr_gamma == 0 || copr_gamma == 1) &&
			(roi_on == 0 || roi_on == 1)) {
		copr->props.reg.copr_gamma = !!copr_gamma;
		copr->props.reg.copr_er = copr_er;
		copr->props.reg.copr_eg = copr_eg;
		copr->props.reg.copr_eb = copr_eb;
		copr->props.reg.roi_on = !!roi_on;

		if (roi_on && (roi_xs % 4 == 0) && (roi_xe % 4 == 3) && (roi_xe - roi_xs + 1 >= 12) &&
				(roi_ys % 4 == 0) && (roi_ye % 4 == 3) && (roi_ye - roi_ys + 1 >= 12)) {
			copr->props.reg.roi_xs = roi_xs;
			copr->props.reg.roi_ys = roi_ys;
			copr->props.reg.roi_xe = roi_xe;
			copr->props.reg.roi_ye = roi_ye;
		}
		pr_info("%s copr en %d gamma %d er 0x%02X eg 0x%02X eb 0x%02X roi on:%d xs:%d ys:%d xe:%d ye:%d\n",
				__func__, copr_en, copr_gamma, copr_er, copr_eg, copr_eb, roi_on, roi_xs, roi_ys, roi_xe, roi_ye);
		copr->props.state = COPR_UNINITIALIZED;
	}
	mutex_unlock(&copr->lock);
	copr_update_start(&panel->copr, 1);

	return size;
}

static ssize_t read_copr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct copr_info *copr = &panel->copr;
	struct copr_properties *props = &copr->props;
	int cur_copr;

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	if (!copr_is_enabled(copr)) {
		panel_err("%s copr is off state\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	cur_copr = copr_get_value(copr);
	if (cur_copr < 0) {
		panel_err("%s failed to get copr\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	if (props->version == 2)
		panel_info("read_copr : cur_cnt %d, cur_copr %d, avg_copr %d, "
				"s_cur_cnt %d, s_avg_copr %d, copr_ready %d, comp_copr %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr,
				props->copr_ready, props->comp_copr);

	return snprintf(buf, PAGE_SIZE, "%d\n", cur_copr);
}

static ssize_t copr_roi_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct copr_info *copr = &panel->copr;
	struct copr_roi roi[4];
	int nr_roi, rc, i;

	rc = sscanf(buf, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
			&roi[0].roi_xs, &roi[0].roi_ys, &roi[0].roi_xe, &roi[0].roi_ye,
			&roi[1].roi_xs, &roi[1].roi_ys, &roi[1].roi_xe, &roi[1].roi_ye,
			&roi[2].roi_xs, &roi[2].roi_ys, &roi[2].roi_xe, &roi[2].roi_ye,
			&roi[3].roi_xs, &roi[3].roi_ys, &roi[3].roi_xe, &roi[3].roi_ye);
	if (rc < 0) {
		pr_err("%s invalid roi input(rc %d)\n", __func__, rc);
		return -EINVAL;
	}
	nr_roi = rc / 4;

	mutex_lock(&copr->lock);
	memcpy(copr->props.roi, roi, sizeof(struct copr_roi) * nr_roi);
	copr->props.nr_roi = nr_roi;
	mutex_unlock(&copr->lock);

	for (i = 0; i < nr_roi; i++)
		pr_info("%s roi[%d] %d %d %d %d\n", __func__, i,
				roi[i].roi_xs, roi[i].roi_ys, roi[i].roi_xe, roi[i].roi_ye);

	return size;
}

static ssize_t copr_roi_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct copr_info *copr = &panel->copr;
	int i, c, ret, len = 0;
	u32 out[4 * 3] = { 0, };

	if (copr->props.nr_roi == 0) {
		panel_warn("%s copr roi disabled\n", __func__);
		return -ENODEV;
	}

	if (!copr_is_enabled(copr)) {
		panel_err("%s copr is off state\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	ret = copr_roi_get_value(copr, copr->props.roi,
			copr->props.nr_roi, out);
	if (ret < 0) {
		panel_err("%s failed to get copr\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	for (i = 0; i < copr->props.nr_roi; i++) {
		for (c = 0; c < 3; c++) {
			len += snprintf(buf + len, PAGE_SIZE - len,
					"%d%s", out[i * 3 + c],
					((i == copr->props.nr_roi -1) && c == 2) ? "\n" : " ");
		}
	}

	return len;
}

static ssize_t brt_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	int brt_avg;

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	brt_avg = panel_bl_get_average_and_clear(panel_bl, 1);
	if (brt_avg < 0) {
		panel_err("%s failed to get average brt1\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", brt_avg);
}
#endif

#ifdef CONFIG_DISPLAY_USE_INFO
/*
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t dpui_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);
	return ret;
}

static ssize_t dpui_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL);

	return size;
}

/*
 * [DEV ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t dpui_dbg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);
	return ret;
}

static ssize_t dpui_dbg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL);

	return size;
}

/*
 * [AP DEPENDENT ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t dpci_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);
	return ret;
}

static ssize_t dpci_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL);

	return size;
}

/*
 * [AP DEPENDENT DEV ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t dpci_dbg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_CTRL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_CTRL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);
	return ret;
}

static ssize_t dpci_dbg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_CTRL);

	return size;
}
#endif

static ssize_t poc_onoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n", panel_data->props.poc_onoff);

	return strlen(buf);
}

static ssize_t poc_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;
	int value;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtoint(buf, 0, &value);

	if (rc < 0)
		return rc;

	pr_info("%s: %d -> %d\n", __func__, panel_data->props.poc_onoff, value);

	if (panel_data->props.poc_onoff != value) {
		mutex_lock(&panel->panel_bl.lock);
		panel_data->props.poc_onoff = value;
		mutex_unlock(&panel->panel_bl.lock);
		backlight_update_status(panel->panel_bl.bd);
	}

	return size;
}


#ifdef CONFIG_EXTEND_LIVE_CLOCK

static ssize_t self_mask_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aod_dev_info *aod;
	struct aod_ioctl_props *props;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	aod = &panel->aod;
	props = &aod->props;

	snprintf(buf, PAGE_SIZE, "%d\n", props->self_mask_en);

	return strlen(buf);
}

static ssize_t self_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;
	int value;
	struct aod_dev_info *aod;
	struct aod_ioctl_props *props;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	aod = &panel->aod;
	props = &aod->props;
	if ((aod == NULL) || (props == NULL)) {
		panel_err("PANEL:ERR:%s:aod is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtoint(buf, 0, &value);

	if (rc < 0)
		return rc;

	pr_info("%s: %d -> %d\n", __func__, props->self_mask_en, value);

	if (props->self_mask_en != value) {
		if (value == 0) {
			panel_do_aod_seqtbl_by_index(aod, SELF_MASK_DIS_SEQ);
		} else {
			panel_do_aod_seqtbl_by_index(aod, SELF_MASK_IMG_SEQ);
			panel_do_aod_seqtbl_by_index(aod, SELF_MASK_ENA_SEQ);
		}
		props->self_mask_en = value;
	}

	return size;
}

#endif


#ifdef CONFIG_SUPPORT_ISC_DEFECT
static ssize_t isc_defect_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	snprintf(buf, PAGE_SIZE, "support isc defect checkd\n");

	return 0;
}

static ssize_t isc_defect_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc, ret;
	int value;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtoint(buf, 0, &value);

	if (rc < 0)
		return rc;

	panel_info("PANEL:INFO:%s: %d\n", __func__, value);

	mutex_lock(&panel->op_lock);

	if (value) {
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_CHECK_ISC_DEFECT_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("PANEL:ERR:%s:failed to write ics defect seq\n", __func__);
		}
	}

	mutex_unlock(&panel->op_lock);
	return size;
}

#endif


struct device_attribute panel_attrs[] = {
	__PANEL_ATTR_RO(lcd_type, 0444),
	__PANEL_ATTR_RO(window_type, 0444),
	__PANEL_ATTR_RO(manufacture_code, 0444),
	__PANEL_ATTR_RO(cell_id, 0444),
	__PANEL_ATTR_RO(octa_id, 0444),
	__PANEL_ATTR_RO(SVC_OCTA, 0444),
	__PANEL_ATTR_RO(SVC_OCTA_CHIPID, 0444),
#ifdef CONFIG_SUPPORT_XTALK_MODE
	__PANEL_ATTR_RW(xtalk_mode, 0664),
#endif
#ifdef CONFIG_SUPPORT_MST
	__PANEL_ATTR_RW(mst, 0664),
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
	__PANEL_ATTR_RW(poc, 0660),
#endif
#ifdef CONFIG_SUPPORT_DIM_FLASH
	__PANEL_ATTR_RW(gamma_flash, 0660),
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
	__PANEL_ATTR_RW(gct, 0664),
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	__PANEL_ATTR_RW(grayspot, 0664),
#endif
	__PANEL_ATTR_RW(irc_mode, 0664),
	__PANEL_ATTR_RO(color_coordinate, 0444),
	__PANEL_ATTR_RO(manufacture_date, 0444),
	__PANEL_ATTR_RO(brightness_table, 0444),
	__PANEL_ATTR_RW(adaptive_control, 0664),
	__PANEL_ATTR_RW(siop_enable, 0664),
	__PANEL_ATTR_RW(temperature, 0664),
	__PANEL_ATTR_RW(read_mtp, 0644),
	__PANEL_ATTR_RW(mcd_mode, 0664),
	__PANEL_ATTR_RW(mcd_resistance, 0664),
#ifdef CONFIG_PANEL_AID_DIMMING_DEBUG
	__PANEL_ATTR_RW(aid_log, 0660),
#endif
#ifdef CONFIG_LCD_WEAKNESS_CCB
	__PANEL_ATTR_RO(weakness_ccb, 0644),
#endif
#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
	__PANEL_ATTR_RW(lux, 0644),
#endif
#if defined(CONFIG_EXYNOS_DECON_LCD_COPR)
	__PANEL_ATTR_RW(copr, 0600),
	__PANEL_ATTR_RO(read_copr, 0440),
	__PANEL_ATTR_RW(copr_roi, 0600),
	__PANEL_ATTR_RO(brt_avg, 0440),
#endif
	__PANEL_ATTR_RW(alpm, 0664),
	__PANEL_ATTR_RW(lpm_opr, 0664),
	__PANEL_ATTR_RW(fingerprint, 0644),
#if 0
#ifdef CONFIG_ACTIVE_CLOCK
	__PANEL_ATTR_RW(active_clock, 0644),
	__PANEL_ATTR_RW(active_blink, 0644),
#endif
#endif
#ifdef CONFIG_SUPPORT_HMD
	__PANEL_ATTR_RW(hmt_bright, 0664),
	__PANEL_ATTR_RW(hmt_on, 0664),
#endif
#ifdef CONFIG_DISPLAY_USE_INFO
	__PANEL_ATTR_RW(dpui, 0660),
	__PANEL_ATTR_RW(dpui_dbg, 0660),
	__PANEL_ATTR_RW(dpci, 0660),
	__PANEL_ATTR_RW(dpci_dbg, 0660),
#endif
	__PANEL_ATTR_RW(poc_onoff, 0664),
#ifdef CONFIG_EXTEND_LIVE_CLOCK
	__PANEL_ATTR_RW(self_mask, 0664),
#endif
#ifdef CONFIG_SUPPORT_ISC_DEFECT
	__PANEL_ATTR_RW(isc_defect, 0664),
#endif
};

int panel_sysfs_probe(struct panel_device *panel)
{
	struct lcd_device *lcd;
	size_t i;
	int ret;
	struct kernfs_node *svc_sd;
	struct kobject *svc;

	lcd = panel->lcd;
	if (unlikely(!lcd)) {
		pr_err("%s, lcd device not exist\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(panel_attrs); i++) {
		ret = device_create_file(&lcd->dev, &panel_attrs[i]);
		if (ret < 0) {
			dev_err(&lcd->dev, "%s, failed to add %s sysfs entries, %d\n",
					__func__, panel_attrs[i].attr.name, ret);
			return -ENODEV;
		}
	}

	/* to /sys/devices/svc/ */
	svc_sd = sysfs_get_dirent(devices_kset->kobj.sd, "svc");
	if (IS_ERR_OR_NULL(svc_sd)) {
		svc = kobject_create_and_add("svc", &devices_kset->kobj);
		if (IS_ERR_OR_NULL(svc))
			pr_err("failed to create /sys/devices/svc already exist svc : 0x%p\n", svc);
		else
			pr_err("success to create /sys/devices/svc svc : 0x%p\n", svc);
	} else {
		svc = (struct kobject*)svc_sd->priv;
		pr_info("success to find svc_sd : 0x%p  svc : 0x%p\n", svc_sd, svc);
	}

	if (!IS_ERR_OR_NULL(svc)) {
		ret = sysfs_create_link(svc, &lcd->dev.kobj, "OCTA");
		if (ret)
			pr_err("failed to create svc/OCTA/ \n");
		else
			pr_info("success to create svc/OCTA/ \n");
	} else {
		pr_err("failed to find svc kobject\n");
	}

	return 0;
}
