/* linux/drivers/video/exynos_decon/panel/lcd_sysfs.c
 *
 * Header file for Samsung MIPI-DSI Panel SYSFS driver.
 *
 * Copyright (c) 2013 Samsung Electronics
 * JiHoon Kim <jihoonn.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/lcd.h>

#include "../dsim.h"
#include "dsim_panel.h"
#include "panel_info.h"
#include "dsim_backlight.h"
#include "../decon.h"

#ifdef CONFIG_PANEL_DDI_SPI
#include "ddi_spi.h"
#endif

#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_EXYNOS_DECON_LCD_MCD)
static struct lcd_seq_info SEQ_MCD_ON_SET[] = {
	{(u8 *)SEQ_MCD_ON_SET1, ARRAY_SIZE(SEQ_MCD_ON_SET1), 0},
	{(u8 *)SEQ_MCD_ON_SET2, ARRAY_SIZE(SEQ_MCD_ON_SET2), 0},
	{(u8 *)SEQ_MCD_ON_SET3, ARRAY_SIZE(SEQ_MCD_ON_SET3), 0},
	{(u8 *)SEQ_MCD_ON_SET4, ARRAY_SIZE(SEQ_MCD_ON_SET4), 0},
	{(u8 *)SEQ_MCD_ON_SET5, ARRAY_SIZE(SEQ_MCD_ON_SET5), 0},
	{(u8 *)SEQ_MCD_ON_SET6, ARRAY_SIZE(SEQ_MCD_ON_SET6), 0},
	{(u8 *)SEQ_MCD_ON_SET7, ARRAY_SIZE(SEQ_MCD_ON_SET7), 0},
	{(u8 *)SEQ_MCD_ON_SET8, ARRAY_SIZE(SEQ_MCD_ON_SET8), 100},
	{(u8 *)SEQ_MCD_ON_SET9, ARRAY_SIZE(SEQ_MCD_ON_SET9), 0},
	{(u8 *)SEQ_MCD_ON_SET10, ARRAY_SIZE(SEQ_MCD_ON_SET10), 0},
	{(u8 *)SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE), 100},
	{(u8 *)SEQ_MCD_ON_SET11, ARRAY_SIZE(SEQ_MCD_ON_SET11), 0},
	{(u8 *)SEQ_MCD_ON_SET12, ARRAY_SIZE(SEQ_MCD_ON_SET12), 0},
	{(u8 *)SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE), 100},
};

static struct lcd_seq_info SEQ_MCD_OFF_SET[] = {
	{(u8 *)SEQ_MCD_OFF_SET1, ARRAY_SIZE(SEQ_MCD_OFF_SET1), 0},
	{(u8 *)SEQ_MCD_OFF_SET2, ARRAY_SIZE(SEQ_MCD_OFF_SET2), 0},
	{(u8 *)SEQ_MCD_OFF_SET3, ARRAY_SIZE(SEQ_MCD_OFF_SET3), 0},
	{(u8 *)SEQ_MCD_OFF_SET4, ARRAY_SIZE(SEQ_MCD_OFF_SET4), 0},
	{(u8 *)SEQ_MCD_OFF_SET5, ARRAY_SIZE(SEQ_MCD_OFF_SET5), 0},
	{(u8 *)SEQ_MCD_OFF_SET6, ARRAY_SIZE(SEQ_MCD_OFF_SET6), 0},
	{(u8 *)SEQ_MCD_OFF_SET7, ARRAY_SIZE(SEQ_MCD_OFF_SET7), 0},
	{(u8 *)SEQ_MCD_OFF_SET8, ARRAY_SIZE(SEQ_MCD_OFF_SET8), 100},
	{(u8 *)SEQ_MCD_OFF_SET9, ARRAY_SIZE(SEQ_MCD_OFF_SET9), 0},
	{(u8 *)SEQ_MCD_OFF_SET10, ARRAY_SIZE(SEQ_MCD_OFF_SET10), 0},
	{(u8 *)SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE), 100},
	{(u8 *)SEQ_MCD_OFF_SET11, ARRAY_SIZE(SEQ_MCD_OFF_SET11), 0},
	{(u8 *)SEQ_MCD_OFF_SET12, ARRAY_SIZE(SEQ_MCD_OFF_SET12), 0},
	{(u8 *)SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE), 100},
};

static int mcd_write_set(struct dsim_device *dsim, struct lcd_seq_info *seq, u32 num)
{
	int ret = 0, i;

	for (i = 0; i < num; i++) {
		if (seq[i].cmd) {
			ret = dsim_write_hl_data(dsim, seq[i].cmd, seq[i].len);
			if (ret != 0) {
				dsim_err("%s failed.\n", __func__);
				return ret;
			}
		}
		if (seq[i].sleep)
			usleep_range(seq[i].sleep * 1000 , seq[i].sleep * 1000);
	}
	return ret;
}


void mcd_mode_set(struct dsim_device *dsim)
{
	struct panel_private *panel = &dsim->priv;
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F1, ARRAY_SIZE(SEQ_TEST_KEY_ON_F1));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
	if(panel->mcd_on == 0)	{	// mcd off
		pr_info("%s MCD off : %d\n", __func__, panel->mcd_on);
		mcd_write_set(dsim, SEQ_MCD_OFF_SET, ARRAY_SIZE(SEQ_MCD_OFF_SET));
	} else {					// mcd on
		pr_info("%s MCD on : %d\n", __func__, panel->mcd_on);
		mcd_write_set(dsim, SEQ_MCD_ON_SET, ARRAY_SIZE(SEQ_MCD_ON_SET));
	}
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F1, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F1));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
}
static ssize_t mcd_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%u\n", priv->mcd_on);

	return strlen(buf);
}

static ssize_t mcd_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int value;
	int rc;

	dsim = container_of(priv, struct dsim_device, priv);

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;

	if ((priv->state == PANEL_STATE_RESUMED) && (priv->mcd_on != value)) {
		priv->mcd_on = value;
		mcd_mode_set(dsim);
	}

	dev_info(dev, "%s: %d\n", __func__, priv->mcd_on);

	return size;
}
static DEVICE_ATTR(mcd_mode, 0664, mcd_mode_show, mcd_mode_store);

#endif

#ifdef CONFIG_LCD_HMT

static ssize_t hmt_brightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "index : %d, brightenss : %d\n", priv->hmt_br_index, priv->hmt_brightness);

	return strlen(buf);
}

static ssize_t hmt_brightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int value;
	int rc;

	dsim = container_of(priv, struct dsim_device, priv);

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;
	if (priv->state != PANEL_STATE_RESUMED) {
		dev_info(dev, "%s: panel is off\n", __func__);
		return -EINVAL;
	}

	if (priv->hmt_on == HMT_OFF) {
		dev_info(dev, "%s: hmt is not on\n", __func__);
		return -EINVAL;
	}

	if (priv->hmt_brightness != value) {
		mutex_lock(&priv->lock);
		priv->hmt_brightness = value;
		mutex_unlock(&priv->lock);
		dsim_panel_set_brightness_for_hmt(dsim, 0);
	}

	dev_info(dev, "%s: %d\n", __func__, value);
	return size;
}


int hmt_set_mode(struct dsim_device *dsim, bool wakeup)
{
	struct panel_private *priv = &(dsim->priv);
	const unsigned char* hmt_forward = SEQ_HMT_AID_FORWARD1;
 	const unsigned char* hmt_reverse = SEQ_HMT_AID_REVERSE1;
	unsigned int hmd_for_cmd_size = ARRAY_SIZE(SEQ_HMT_AID_FORWARD1);
	unsigned int hmd_rev_cmd_size = ARRAY_SIZE(SEQ_HMT_AID_REVERSE1);

#ifdef CONFIG_PANEL_S6E3HF4_WQHD
	if (priv->panel_material == LCD_MATERIAL_DAISY) {
		hmt_forward = SEQ_HMT_AID_FORWARD1_A3;
		hmt_reverse = SEQ_HMT_AID_REVERSE1_A3;
		hmd_for_cmd_size = ARRAY_SIZE(SEQ_HMT_AID_FORWARD1_A3);
		hmd_rev_cmd_size = ARRAY_SIZE(SEQ_HMT_AID_REVERSE1_A3);
	}
#endif

	mutex_lock(&priv->lock);
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));

	if(priv->hmt_on == HMT_ON) {
		// on set
		dsim_write_hl_data(dsim, SEQ_HMT_ON1, ARRAY_SIZE(SEQ_HMT_ON1));
		dsim_write_hl_data(dsim, hmt_reverse, hmd_rev_cmd_size);
		pr_info("%s on sysfs\n", __func__);
	} else if(priv->hmt_on == HMT_OFF) {
		// off set
		dsim_write_hl_data(dsim, SEQ_HMT_OFF1, ARRAY_SIZE(SEQ_HMT_OFF1));
		dsim_write_hl_data(dsim, hmt_forward, hmd_for_cmd_size);
		pr_info("%s off sysfs\n", __func__);

	} else {
		pr_info("hmt state is invalid %d !\n", priv->hmt_on);
		return 0;
	}
	dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE_L, ARRAY_SIZE(SEQ_GAMMA_UPDATE_L));
	if(wakeup)
		msleep(120);
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
	mutex_unlock(&priv->lock);

	if(priv->hmt_on == HMT_ON)
		dsim_panel_set_brightness_for_hmt(dsim, 1);
	else if (priv->hmt_on == HMT_OFF)
		dsim_panel_set_brightness(dsim, 1);
	else ;

	return 0;
}

static ssize_t hmt_on_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%u\n", priv->hmt_on);

	return strlen(buf);
}

static ssize_t hmt_on_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int value;
	int rc;
	dsim = container_of(priv, struct dsim_device, priv);

	rc = kstrtoint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (priv->state != PANEL_STATE_RESUMED) {
		dev_info(dev, "%s: panel is off\n", __func__);
		return -EINVAL;
	}

	if (priv->hmt_on != value) {
		mutex_lock(&priv->lock);
		priv->hmt_prev_status = priv->hmt_on;
		priv->hmt_on = value;
		mutex_unlock(&priv->lock);
		dev_info(dev, "++%s: %d %d\n", __func__, priv->hmt_on, priv->hmt_prev_status);
		hmt_set_mode(dsim, false);
		dev_info(dev, "--%s: %d %d\n", __func__, priv->hmt_on, priv->hmt_prev_status);
	} else
		dev_info(dev, "%s: hmt already %s\n", __func__, value ? "on" : "off");

	return size;
}

static DEVICE_ATTR(hmt_bright, 0664, hmt_brightness_show, hmt_brightness_store);
static DEVICE_ATTR(hmt_on, 0664, hmt_on_show, hmt_on_store);

#endif

#ifdef CONFIG_LCD_ALPM

int display_on_retry_cnt = 0;

#define RDDPM_ADDR	0x0A
#define RDDPM_REG_SIZE 3
#define RDDPM_REG_DISPLAY_ON 0x04
#define READ_STATUS_RETRY_CNT 3

static int read_panel_status(struct dsim_device *dsim)
{
	int ret = 0;
	unsigned char rddpm[RDDPM_REG_SIZE+1];
	int retry_cnt = READ_STATUS_RETRY_CNT;

read_rddpm_reg:
	ret = dsim_read_hl_data(dsim, RDDPM_ADDR, RDDPM_REG_SIZE, rddpm);
	if (ret != RDDPM_REG_SIZE) {
		dsim_err("%s : can't read RDDPM Reg\n", __func__);
		goto dump_exit;
	}

	dsim_info("%s : rddpm : %x\n", __func__, rddpm[0]);

	if ((rddpm[0] & RDDPM_REG_DISPLAY_ON) == 0)  {
		display_on_retry_cnt++;
		dsim_info("%s : display on was not setted retry : %d\n", __func__, retry_cnt);
		dsim_info("%s : display on was not setted total retry : %d\n", __func__, display_on_retry_cnt);
		if (retry_cnt) {
			dsim_write_hl_data(dsim, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
			retry_cnt--;
			goto read_rddpm_reg;
		}

		if (rddpm[0] & 0x80)
			dsim_info("Dump Panel : Status Booster Voltage Status : ON\n");
		else
			dsim_info("Dump Panel :  Booster Voltage Status : OFF\n");

		if (rddpm[0] & 0x40)
			dsim_info("Dump Panel :  Idle Mode : On\n");
		else
			dsim_info("Dump Panel :  Idle Mode : OFF\n");

		if (rddpm[0] & 0x20)
			dsim_info("Dump Panel :  Partial Mode : On\n");
		else
			dsim_info("Dump Panel :  Partial Mode : OFF\n");

		if (rddpm[0] & 0x10)
			dsim_info("Dump Panel :  Sleep OUT and Working Ok\n");
		else
			dsim_info("Dump Panel :  Sleep IN\n");

		if (rddpm[0] & 0x08)
			dsim_info("Dump Panel :  Normal Mode On and Working Ok\n");
		else
			dsim_info("Dump Panel :  Sleep IN\n");
	}
dump_exit:
	return ret;

}

static int alpm_write_set(struct dsim_device *dsim, struct lcd_seq_info *seq, u32 num)
{
	int ret = 0, i;

	for (i = 0; i < num; i++) {
		if (seq[i].cmd) {
			ret = dsim_write_hl_data(dsim, seq[i].cmd, seq[i].len);
			if (ret != 0) {
				dsim_err("%s failed.\n", __func__);
				return ret;
			}
		}
		if (seq[i].sleep)
			usleep_range(seq[i].sleep * 1000 , seq[i].sleep * 1000);
	}
	return ret;
}


#ifdef CONFIG_PANEL_S6E3HF4_WQHD
static struct lcd_seq_info SEQ_ALPM_2NIT_ON_SET[] = {
	{(u8 *)SEQ_SELECT_ALPM_2NIT_HF4, ARRAY_SIZE(SEQ_SELECT_ALPM_2NIT_HF4), 0},
	{(u8 *)SEQ_2NIT_MODE_ON, ARRAY_SIZE(SEQ_2NIT_MODE_ON), 0},
	{(u8 *)SEQ_MCLK_1SET_HF4, ARRAY_SIZE(SEQ_MCLK_1SET_HF4), 0},
};

static struct lcd_seq_info SEQ_ALPM_40NIT_ON_SET[] = {
	{(u8 *)SEQ_SELECT_ALPM_40NIT_HF4, ARRAY_SIZE(SEQ_SELECT_ALPM_40NIT_HF4), 0},
	{(u8 *)SEQ_40NIT_MODE_ON, ARRAY_SIZE(SEQ_40NIT_MODE_ON), 0},
	{(u8 *)SEQ_MCLK_1SET_HF4, ARRAY_SIZE(SEQ_MCLK_1SET_HF4), 0},
};

static struct lcd_seq_info SEQ_HLPM_2NIT_ON_SET[] = {
	{(u8 *)SEQ_SELECT_HLPM_2NIT_HF4, ARRAY_SIZE(SEQ_SELECT_HLPM_2NIT_HF4), 0},
	{(u8 *)SEQ_2NIT_MODE_ON, ARRAY_SIZE(SEQ_2NIT_MODE_ON), 0},
	{(u8 *)SEQ_MCLK_1SET_HF4, ARRAY_SIZE(SEQ_MCLK_1SET_HF4), 0},
};

static struct lcd_seq_info SEQ_HLPM_40NIT_ON_SET[] = {
	{(u8 *)SEQ_SELECT_HLPM_40NIT_HF4, ARRAY_SIZE(SEQ_SELECT_HLPM_40NIT_HF4), 0},
	{(u8 *)SEQ_40NIT_MODE_ON, ARRAY_SIZE(SEQ_40NIT_MODE_ON), 0},
	{(u8 *)SEQ_MCLK_1SET_HF4, ARRAY_SIZE(SEQ_MCLK_1SET_HF4), 0},
};

static struct lcd_seq_info SEQ_NORMAL_MODE_ON_SET[] = {
	{(u8 *)SEQ_MCLK_3SET_HF4, ARRAY_SIZE(SEQ_MCLK_3SET_HF4), 0},
	{(u8 *)SEQ_NORMAL_MODE_ON, ARRAY_SIZE(SEQ_NORMAL_MODE_ON), 0},
};

// old panel 18
static struct lcd_seq_info SEQ_ALPM_2NIT_ON_SET_OLD[] = {
	{(u8 *)SEQ_SELECT_ALPM_2NIT_HF4, ARRAY_SIZE(SEQ_SELECT_ALPM_2NIT_HF4), 0},
	{(u8 *)SEQ_2NIT_MODE_ON, ARRAY_SIZE(SEQ_2NIT_MODE_ON), 0},
};

static struct lcd_seq_info SEQ_ALPM_40NIT_ON_SET_OLD[] = {
	{(u8 *)SEQ_SELECT_ALPM_2NIT_HF4, ARRAY_SIZE(SEQ_SELECT_ALPM_2NIT_HF4), 0},
	{(u8 *)SEQ_40NIT_MODE_ON, ARRAY_SIZE(SEQ_40NIT_MODE_ON), 0},
};

static struct lcd_seq_info SEQ_HLPM_2NIT_ON_SET_OLD[] = {
	{(u8 *)SEQ_SELECT_HLPM_2NIT_HF4, ARRAY_SIZE(SEQ_SELECT_HLPM_2NIT_HF4), 0},
	{(u8 *)SEQ_2NIT_MODE_ON, ARRAY_SIZE(SEQ_2NIT_MODE_ON), 0},
};

static struct lcd_seq_info SEQ_HLPM_40NIT_ON_SET_OLD[] = {
	{(u8 *)SEQ_SELECT_HLPM_2NIT_HF4, ARRAY_SIZE(SEQ_SELECT_HLPM_2NIT_HF4), 0},
	{(u8 *)SEQ_40NIT_MODE_ON, ARRAY_SIZE(SEQ_40NIT_MODE_ON), 0},
};

static struct lcd_seq_info SEQ_NORMAL_MODE_ON_SET_OLD[] = {
	{(u8 *)SEQ_NORMAL_MODE_ON, ARRAY_SIZE(SEQ_NORMAL_MODE_ON), 0},
};


int alpm_set_mode(struct dsim_device *dsim, int enable)
{
	struct panel_private *priv = &(dsim->priv);

	if((enable < ALPM_OFF) && (enable > HLPM_ON_40NIT)) {
		pr_info("alpm state is invalid %d !\n", priv->alpm);
		return 0;
	}

	dsim_write_hl_data(dsim, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	usleep_range(17000, 17000);
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));

	if(enable == ALPM_OFF) {
		if(priv->panel_rev >= 5)
			alpm_write_set(dsim, SEQ_NORMAL_MODE_ON_SET, ARRAY_SIZE(SEQ_NORMAL_MODE_ON_SET));
		else
			alpm_write_set(dsim, SEQ_NORMAL_MODE_ON_SET_OLD, ARRAY_SIZE(SEQ_NORMAL_MODE_ON_SET_OLD));
		if((priv->alpm_support == SUPPORT_LOWHZALPM) && (priv->current_alpm == ALPM_ON_40NIT)){
			dsim_write_hl_data(dsim, SEQ_AOD_LOWHZ_OFF, ARRAY_SIZE(SEQ_AOD_LOWHZ_OFF));
			dsim_write_hl_data(dsim, SEQ_AID_MOD_OFF, ARRAY_SIZE(SEQ_AID_MOD_OFF));
			pr_info("%s : Low hz support !\n", __func__);
		}
	} else {
		switch(enable) {
		case HLPM_ON_2NIT:
			if(priv->panel_rev >= 5)
				alpm_write_set(dsim, SEQ_HLPM_2NIT_ON_SET, ARRAY_SIZE(SEQ_HLPM_2NIT_ON_SET));
			else
				alpm_write_set(dsim, SEQ_HLPM_2NIT_ON_SET_OLD, ARRAY_SIZE(SEQ_HLPM_2NIT_ON_SET_OLD));
			pr_info("%s : HLPM_ON_2NIT !\n", __func__);
			break;
		case ALPM_ON_2NIT:
			if(priv->panel_rev >= 5)
				alpm_write_set(dsim, SEQ_ALPM_2NIT_ON_SET, ARRAY_SIZE(SEQ_ALPM_2NIT_ON_SET));
			else
				alpm_write_set(dsim, SEQ_ALPM_2NIT_ON_SET_OLD, ARRAY_SIZE(SEQ_ALPM_2NIT_ON_SET_OLD));
			pr_info("%s : ALPM_ON_2NIT !\n", __func__);
			break;
		case HLPM_ON_40NIT:
			if(priv->panel_rev >= 5)
				alpm_write_set(dsim, SEQ_HLPM_40NIT_ON_SET, ARRAY_SIZE(SEQ_HLPM_40NIT_ON_SET));
			else
				alpm_write_set(dsim, SEQ_HLPM_40NIT_ON_SET_OLD, ARRAY_SIZE(SEQ_HLPM_40NIT_ON_SET_OLD));
			pr_info("%s : HLPM_ON_40NIT !\n", __func__);
			break;
		case ALPM_ON_40NIT:
			if(priv->alpm_support == SUPPORT_LOWHZALPM) {
				dsim_write_hl_data(dsim, SEQ_2HZ_GPARA, ARRAY_SIZE(SEQ_2HZ_GPARA));
				dsim_write_hl_data(dsim, SEQ_2HZ_SET, ARRAY_SIZE(SEQ_2HZ_SET));
				dsim_write_hl_data(dsim, SEQ_AID_MOD_ON, ARRAY_SIZE(SEQ_AID_MOD_ON));
				pr_info("%s : Low hz support !\n", __func__);
			}
			if(priv->panel_rev >= 5)
				alpm_write_set(dsim, SEQ_ALPM_40NIT_ON_SET, ARRAY_SIZE(SEQ_ALPM_40NIT_ON_SET));
			else
				alpm_write_set(dsim, SEQ_ALPM_40NIT_ON_SET_OLD, ARRAY_SIZE(SEQ_ALPM_40NIT_ON_SET_OLD));
			pr_info("%s : ALPM_ON_40NIT !\n", __func__);
			break;
		default:
			pr_info("%s: input is out of range : %d \n", __func__, enable);
			break;
		}
		dsim_write_hl_data(dsim, HF4_A3_IRC_off, ARRAY_SIZE(HF4_A3_IRC_off));
	}

	dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE_L, ARRAY_SIZE(SEQ_GAMMA_UPDATE_L));

	usleep_range(17000, 17000);

	dsim_write_hl_data(dsim, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
	read_panel_status(dsim);

	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));


	priv->current_alpm = dsim->alpm = enable;

	return 0;
}

#else		// HA3
static struct lcd_seq_info SEQ_ALPM_2NIT_ON_SET[] = {
	{(u8 *)SEQ_SELECT_ALPM_HA3, ARRAY_SIZE(SEQ_SELECT_ALPM_HA3), 0},
	{(u8 *)SEQ_2NIT_MODE_ON, ARRAY_SIZE(SEQ_2NIT_MODE_ON), 0},
};

static struct lcd_seq_info SEQ_ALPM_40NIT_ON_SET[] = {
	{(u8 *)SEQ_SELECT_ALPM_HA3, ARRAY_SIZE(SEQ_SELECT_ALPM_HA3), 0},
	{(u8 *)SEQ_40NIT_MODE_ON, ARRAY_SIZE(SEQ_40NIT_MODE_ON), 0},
};

static struct lcd_seq_info SEQ_HLPM_2NIT_ON_SET[] = {
	{(u8 *)SEQ_SELECT_HLPM_HA3, ARRAY_SIZE(SEQ_SELECT_HLPM_HA3), 0},
	{(u8 *)SEQ_2NIT_MODE_ON, ARRAY_SIZE(SEQ_2NIT_MODE_ON), 0},
};

static struct lcd_seq_info SEQ_HLPM_40NIT_ON_SET[] = {
	{(u8 *)SEQ_SELECT_HLPM_HA3, ARRAY_SIZE(SEQ_SELECT_HLPM_HA3), 0},
	{(u8 *)SEQ_40NIT_MODE_ON, ARRAY_SIZE(SEQ_40NIT_MODE_ON), 0},
};

static struct lcd_seq_info SEQ_NORMAL_MODE_ON_SET[] = {
	{(u8 *)SEQ_NORMAL_MODE_ON, ARRAY_SIZE(SEQ_NORMAL_MODE_ON), 0},
};


int alpm_set_mode(struct dsim_device *dsim, int enable)
{
	struct panel_private *priv = &(dsim->priv);

	if((enable < ALPM_OFF) && (enable > HLPM_ON_40NIT)) {
		pr_info("alpm state is invalid %d !\n", priv->alpm);
		return 0;
	}

	dsim_write_hl_data(dsim, SEQ_DISPLAY_OFF, ARRAY_SIZE(SEQ_DISPLAY_OFF));
	usleep_range(17000, 17000);
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));

	if(enable == ALPM_OFF) {
		alpm_write_set(dsim, SEQ_NORMAL_MODE_ON_SET, ARRAY_SIZE(SEQ_NORMAL_MODE_ON_SET));
	} else {
		switch(enable) {
		case HLPM_ON_2NIT:
			alpm_write_set(dsim, SEQ_HLPM_2NIT_ON_SET, ARRAY_SIZE(SEQ_HLPM_2NIT_ON_SET));
			pr_info("%s : HLPM_ON_2NIT !\n", __func__);
			break;
		case ALPM_ON_2NIT:
			alpm_write_set(dsim, SEQ_ALPM_2NIT_ON_SET, ARRAY_SIZE(SEQ_ALPM_2NIT_ON_SET));
			pr_info("%s : ALPM_ON_2NIT !\n", __func__);
			break;
		case HLPM_ON_40NIT:
			alpm_write_set(dsim, SEQ_HLPM_40NIT_ON_SET, ARRAY_SIZE(SEQ_HLPM_40NIT_ON_SET));
			pr_info("%s : HLPM_ON_40NIT !\n", __func__);
			break;
		case ALPM_ON_40NIT:
			alpm_write_set(dsim, SEQ_ALPM_40NIT_ON_SET, ARRAY_SIZE(SEQ_ALPM_40NIT_ON_SET));
			pr_info("%s : ALPM_ON_40NIT !\n", __func__);
			break;
		default:
			pr_info("%s: input is out of range : %d \n", __func__, enable);
			break;
		}
		dsim_write_hl_data(dsim, HF4_A3_IRC_off, ARRAY_SIZE(HF4_A3_IRC_off));
	}

	dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE, ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	dsim_write_hl_data(dsim, SEQ_GAMMA_UPDATE_L, ARRAY_SIZE(SEQ_GAMMA_UPDATE_L));

	usleep_range(17000, 17000);

	dsim_write_hl_data(dsim, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
	read_panel_status(dsim);

	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));

	priv->current_alpm = dsim->alpm = enable;

	return 0;
}

#endif


static ssize_t alpm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%d\n", priv->alpm);

	dev_info(dev, "%s: %d\n", __func__, priv->alpm);

	return strlen(buf);
}

#if defined (CONFIG_SEC_FACTORY)
static int prev_brightness = 0;
#endif

static ssize_t alpm_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int value;

	dsim = container_of(priv, struct dsim_device, priv);

	sscanf(buf, "%9d", &value);
	dev_info(dev, "%s: %d \n", __func__, value);

	if(priv->alpm_support == UNSUPPORT_ALPM) {
		pr_info("%s this panel does not support ALPM!\n", __func__);
		return size;
	}

	mutex_lock(&priv->alpm_lock);
	if(priv->state == PANEL_STATE_RESUMED) {
		switch(value) {
		case ALPM_OFF:
			if(priv->current_alpm) {
#ifdef CONFIG_SEC_FACTORY
				priv->bd->props.brightness = prev_brightness;
#endif
				alpm_set_mode(dsim, ALPM_OFF);
				usleep_range(17000, 17000);
				dsim_panel_set_brightness(dsim, 1);
			} else {
				dev_info(dev, "%s: alpm current state : %d input : %d\n",
					__func__, priv->current_alpm, value);
			}
			break;
		case ALPM_ON_2NIT:
		case HLPM_ON_2NIT:
		case ALPM_ON_40NIT:
		case HLPM_ON_40NIT:
#ifdef CONFIG_SEC_FACTORY
			if(priv->alpm == ALPM_OFF) {
				prev_brightness = priv->bd->props.brightness;
				priv->bd->props.brightness = UI_MIN_BRIGHTNESS;
				dsim_panel_set_brightness(dsim, 1);
			}
#endif
			alpm_set_mode(dsim, value);
			break;
		default:
			dev_info(dev, "%s: input is out of range : %d \n", __func__, value);
			break;
		}
	} else {
		dev_info(dev, "%s: panel state is not active\n", __func__);
	}
	priv->alpm = value;

	mutex_unlock(&priv->alpm_lock);

	dev_info(dev, "%s: %d\n", __func__, priv->alpm);

	return size;
}
static DEVICE_ATTR(alpm, 0664, alpm_show, alpm_store);
#endif

#ifdef CONFIG_LCD_DOZE_MODE

#if defined (CONFIG_SEC_FACTORY)
static int prev_brightness = 0;
static int current_alpm_mode = 0;

#endif

static ssize_t alpm_doze_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	struct decon_device *decon = NULL;
	struct mutex *output_lock = NULL;

	dsim = container_of(priv, struct dsim_device, priv);

	sscanf(buf, "%9d", &value);

	decon = (struct decon_device *)dsim->decon;
	if (decon != NULL)
		output_lock = &decon->output_lock;
#ifdef CONFIG_SEC_FACTORY
	dsim_info("%s: %d\n", __func__, value);
	current_alpm_mode = priv->alpm_mode;
	switch (value) {
		case ALPM_OFF:
			priv->alpm_mode = value;
			if ((dsim->dsim_doze == DSIM_DOZE_STATE_DOZE) ||
				(dsim->dsim_doze == DSIM_DOZE_STATE_NORMAL)) {

				call_panel_ops(dsim, exitalpm, dsim);
				usleep_range(17000, 17000);
				if (prev_brightness) {
					priv->bd->props.brightness = prev_brightness - 1;
					dsim_panel_set_brightness(dsim, 1);
					prev_brightness = 0;
				}
				call_panel_ops(dsim, displayon, dsim);
			}
			break;
		case ALPM_ON_2NIT:
		case HLPM_ON_2NIT:
		case ALPM_ON_40NIT:
		case HLPM_ON_40NIT:
			priv->alpm_mode = value;
			if ((dsim->dsim_doze == DSIM_DOZE_STATE_DOZE) ||
				(dsim->dsim_doze == DSIM_DOZE_STATE_NORMAL)) {
				if(current_alpm_mode == ALPM_OFF) {
					prev_brightness = priv->bd->props.brightness + 1;
					priv->bd->props.brightness = UI_MIN_BRIGHTNESS;
					dsim_panel_set_brightness(dsim, 1);
				}
				call_panel_ops(dsim, enteralpm, dsim);
				usleep_range(17000, 17000);
				dsim_write_hl_data(dsim, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
			}
			break;
		default:
			dsim_err("ERR:%s:undefined alpm mode : %d\n", __func__, value);
			break;
	}
#else
	switch (value) {
		case ALPM_ON_2NIT:
		case HLPM_ON_2NIT:
		case ALPM_ON_40NIT:
		case HLPM_ON_40NIT:
			if (output_lock != NULL)
				mutex_lock(output_lock);

			dsim_info("%s: %d\n", __func__, priv->alpm_mode);
			if (value != priv->alpm_mode) {
				priv->alpm_mode = value;
				if (dsim->dsim_doze == DSIM_DOZE_STATE_DOZE) {
					call_panel_ops(dsim, enteralpm, dsim);
				}
			}
			if (output_lock != NULL)
				mutex_unlock(output_lock);

			break;
		default:
			dsim_err("ERR:%s:undefined alpm mode : %d\n", __func__, value);
			break;
	}
#endif
	return size;
}

static ssize_t alpm_doze_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	dsim_info("%s: %d\n", __func__, priv->alpm_mode);
	sprintf(buf, "%d\n", priv->alpm_mode);
	return strlen(buf);
}

static DEVICE_ATTR(alpm, 0664, alpm_doze_show, alpm_doze_store);
#endif
static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "SDC_%02X%02X%02X\n", priv->id[0], priv->id[1], priv->id[2]);

	return strlen(buf);
}

static ssize_t window_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%02x %02x %02x\n", priv->id[0], priv->id[1], priv->id[2]);

	return strlen(buf);
}

static ssize_t brightness_table_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *panel = dev_get_drvdata(dev);

	char *pos = buf;
	int nit, i;
	for (i = 0; i <= UI_MAX_BRIGHTNESS; i++) {
		nit = panel->br_tbl[i];
		pos += sprintf(pos, "%3d %3d\n", i, nit);
	}
	return pos - buf;
}

static ssize_t auto_brightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%u\n", priv->auto_brightness);

	return strlen(buf);
}

static ssize_t auto_brightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int value;
	int rc;

	dsim = container_of(priv, struct dsim_device, priv);

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;
	else {
		if (priv->auto_brightness != value) {
			dev_info(dev, "%s: %d, %d\n", __func__, priv->auto_brightness, value);
			mutex_lock(&priv->lock);
			priv->auto_brightness = value;
			mutex_unlock(&priv->lock);
			dsim_panel_set_brightness(dsim, 1);
		}
	}
	return size;
}

static ssize_t siop_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%u\n", priv->siop_enable);

	return strlen(buf);
}

static ssize_t siop_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int value;
	int rc;

	dsim = container_of(priv, struct dsim_device, priv);

	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;
	else {
		if (priv->siop_enable != value) {
			dev_info(dev, "%s: %d, %d\n", __func__, priv->siop_enable, value);
			mutex_lock(&priv->lock);
			priv->siop_enable = value;
			mutex_unlock(&priv->lock);
#ifdef CONFIG_LCD_HMT
			if(priv->hmt_on == HMT_ON)
				dsim_panel_set_brightness_for_hmt(dsim, 1);
			else
#endif
				dsim_panel_set_brightness(dsim, 1);
		}
	}
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
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int value;
	int rc;

	dsim = container_of(priv, struct dsim_device, priv);

	rc = kstrtoint(buf, 10, &value);

	if (rc < 0)
		return rc;
	else {
		mutex_lock(&priv->lock);
		priv->temperature = value;
		mutex_unlock(&priv->lock);
#ifdef CONFIG_LCD_HMT
		if(priv->hmt_on == HMT_ON)
			dsim_panel_set_brightness_for_hmt(dsim, 1);
		else
#endif
			dsim_panel_set_brightness(dsim, 1);
		dev_info(dev, "%s: %d, %d\n", __func__, value, priv->temperature);
	}

	return size;
}

static ssize_t color_coordinate_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%u, %u\n", priv->coordinate[0], priv->coordinate[1]);
	return strlen(buf);
}

static ssize_t auto_brightness_level_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);
	sprintf(buf, "%u\n", priv->auto_brightness_level);

	return strlen(buf);
}


static ssize_t manufacture_date_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);
	u16 year;
	u8 month, day, hour, min;

	year = ((priv->date[0] & 0xF0) >> 4) + 2011;
	month = priv->date[0] & 0xF;
	day = priv->date[1] & 0x1F;
	hour = priv->date[2] & 0x1F;
	min = priv->date[3] & 0x3F;

	sprintf(buf, "%d, %d, %d, %d:%d\n", year, month, day, hour, min);
	return strlen(buf);
}

static ssize_t read_mtp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t read_mtp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	unsigned int reg, pos, length, i;
	unsigned char readbuf[256] = {0xff, };
	unsigned char writebuf[2] = {0xB0, };

	dsim = container_of(priv, struct dsim_device, priv);

	sscanf(buf, "%x %d %d", &reg, &pos, &length);

	if (!reg || !length || pos < 0 || reg > 0xff || length > 255 || pos > 255)
		return -EINVAL;
	if (priv->state != PANEL_STATE_RESUMED)
		return -EINVAL;

	writebuf[1] = pos;
	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) < 0)
		dsim_err("fail to write F0 on command.\n");

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC)) < 0)
		dsim_err("fail to write test on fc command.\n");

	if (dsim_write_hl_data(dsim, writebuf, ARRAY_SIZE(writebuf)) < 0)
		dsim_err("fail to write global command.\n");

	if (dsim_read_hl_data(dsim, reg, length, readbuf) < 0)
		dsim_err("fail to read id on command.\n");

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0)) < 0)
		dsim_err("fail to write F0 on command.\n");

	if (dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC)) < 0)
		dsim_err("fail to write test on fc command.\n");

	dsim_info("READ_Reg addr : %02x, start pos : %d len : %d \n", reg, pos, length);
	for (i = 0; i < length; i++)
		dsim_info("READ_Reg %dth : %02x \n", i + 1, readbuf[i]);

	return size;
}
#ifdef CONFIG_PANEL_S6E3HF4_WQHD
static int set_partial_display(struct dsim_device *dsim)
{
	u32 st, ed;
	u8 area_cmd[5] = {0x30,};
	u8 partial_mode[1] = {0x12};
	u8 normal_mode[1] = {0x13};

	st =	dsim->lcd_info.partial_range[0];
	ed = dsim->lcd_info.partial_range[1];

	if (st || ed) {
		area_cmd[1] = (st >> 8) & 0xFF;/*select msb 1byte*/
		area_cmd[2] = st & 0xFF;
		area_cmd[3] = (ed >> 8) & 0xFF;/*select msb 1byte*/
		area_cmd[4] = ed & 0xFF;

		dsim_write_hl_data(dsim, area_cmd, ARRAY_SIZE(area_cmd));
		dsim_write_hl_data(dsim, partial_mode, ARRAY_SIZE(partial_mode));
	} else
		dsim_write_hl_data(dsim, normal_mode, ARRAY_SIZE(normal_mode));

	return 0;
}


static ssize_t partial_disp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);

	dsim = container_of(priv, struct dsim_device, priv);

	sprintf((char *)buf, "%d, %d\n", dsim->lcd_info.partial_range[0], dsim->lcd_info.partial_range[1]);

	dev_info(dev, "%s: %d, %d\n", __func__, dsim->lcd_info.partial_range[0], dsim->lcd_info.partial_range[1]);

	return strlen(buf);
}

static ssize_t partial_disp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);

	u32 st, ed;

	dsim = container_of(priv, struct dsim_device, priv);

	sscanf(buf, "%9d %9d" , &st, &ed);

	dev_info(dev, "%s: %d, %d\n", __func__, st, ed);

	if ((st >= dsim->lcd_info.yres || ed >= dsim->lcd_info.yres) || (st > ed)) {
		pr_err("%s:Invalid Input\n", __func__);
		return size;
	}

	dsim->lcd_info.partial_range[0] = st;
	dsim->lcd_info.partial_range[1] = ed;

	set_partial_display(dsim);

	return size;
}
#endif


#ifdef CONFIG_PANEL_AID_DIMMING
static void show_aid_log(struct dsim_device *dsim)
{
	int i, j, k;
	struct dim_data *dim_data = NULL;
	struct SmtDimInfo *dimming_info = NULL;
#ifdef CONFIG_LCD_HMT
	struct SmtDimInfo *hmt_dimming_info = NULL;
#endif
	struct panel_private *panel = &dsim->priv;
	u8 temp[256];


	if (panel == NULL) {
		dsim_err("%s:panel is NULL\n", __func__);
		return;
	}

	dim_data = (struct dim_data*)(panel->dim_data);
	if (dim_data == NULL) {
		dsim_info("%s:dimming is NULL\n", __func__);
		return;
	}

	dimming_info = (struct SmtDimInfo*)(panel->dim_info);
	if (dimming_info == NULL) {
		dsim_info("%s:dimming is NULL\n", __func__);
		return;
	}

	dsim_info("MTP VT : %d %d %d\n",
			dim_data->vt_mtp[CI_RED], dim_data->vt_mtp[CI_GREEN], dim_data->vt_mtp[CI_BLUE] );

	for(i = 0; i < VMAX; i++) {
		dsim_info("MTP V%d : %4d %4d %4d \n",
			vref_index[i], dim_data->mtp[i][CI_RED], dim_data->mtp[i][CI_GREEN], dim_data->mtp[i][CI_BLUE] );
	}

	for(i = 0; i < MAX_BR_INFO; i++) {
		memset(temp, 0, sizeof(temp));
		for(j = 1; j < OLED_CMD_GAMMA_CNT; j++) {
			if (j == 1 || j == 3 || j == 5)
				k = dimming_info[i].gamma[j++] * 256;
			else
				k = 0;
			snprintf(temp + strnlen(temp, 256), 256, " %d", dimming_info[i].gamma[j] + k);
		}
		dsim_info("nit :%3d %s\n", dimming_info[i].br, temp);
	}
#ifdef CONFIG_LCD_HMT
	hmt_dimming_info = (struct SmtDimInfo*)(panel->hmt_dim_info);
	if (hmt_dimming_info == NULL) {
		dsim_info("%s:dimming is NULL\n", __func__);
		return;
	}
	for(i = 0; i < HMT_MAX_BR_INFO; i++) {
		memset(temp, 0, sizeof(temp));
		for(j = 1; j < OLED_CMD_GAMMA_CNT; j++) {
			if (j == 1 || j == 3 || j == 5)
				k = hmt_dimming_info[i].gamma[j++] * 256;
			else
				k = 0;
			snprintf(temp + strnlen(temp, 256), 256, " %d", hmt_dimming_info[i].gamma[j] + k);
		}
		dsim_info("nit :%3d %s\n", hmt_dimming_info[i].br, temp);
	}
#endif
}


static ssize_t aid_log_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);

	dsim = container_of(priv, struct dsim_device, priv);

	show_aid_log(dsim);
	return strlen(buf);
}

#endif

static ssize_t manufacture_code_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%02X%02X%02X%02X%02X\n",
		priv->code[0], priv->code[1], priv->code[2], priv->code[3], priv->code[4]);

	return strlen(buf);
}

static ssize_t cell_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
		priv->date[0] , priv->date[1], priv->date[2], priv->date[3], priv->date[4],
		priv->date[5],priv->date[6], (priv->coordinate[0] &0xFF00)>>8,priv->coordinate[0] &0x00FF,
		(priv->coordinate[1]&0xFF00)>>8,priv->coordinate[1]&0x00FF);

	return strlen(buf);
}
#if defined(CONFIG_LCD_HBM_INTERPOLATION) || defined(CONFIG_LCD_WEAKNESS_CCB)
static int find_hbm_table(struct dsim_device *dsim, int nit, int mode)
{
	int retVal = 0;
	int i;
	int current_gap;
	int minVal = 20000;
	for(i = 0; i < 256; i++) {
		if(mode == 1)				// setting
			current_gap = nit - dsim->priv.hbm_inter_br_tbl[i];
		else						// gallery
			break;
		if(current_gap < 0)
			current_gap *= -1;
		if(minVal > current_gap) {
			minVal = current_gap;
			retVal = i;
		}
	}
	return retVal;
}
static void hbm_set_brightness(struct dsim_device *dsim, int on)
{
	struct panel_private *priv = &(dsim->priv);
	int hbm_nit, origin_nit, p_br, gap_nit, i, prev_brightness = -1;
	const int step_max = 5;
	int index_array[6];	// array size is step_max +1

	if(priv->hbm_inter_br_tbl == NULL) {
		pr_info("this panel don't support hbm brightness\n");
		return ;
	}

	if(priv->panel_type == LCD_TYPE_S6E3HF4_WQHD) {
		if(on == 0) {				// off
			priv->br_tbl = priv->origin_br_tbl;
		} else {					// on
			priv->br_tbl = priv->hbm_inter_br_tbl;
		}
	} else {
		if(priv->interpolation) {
			if(on == 0) {				// off
				priv->br_tbl = priv->origin_br_tbl;
			} else {					// on
				priv->br_tbl = priv->hbm_inter_br_tbl;
			}
			return ;
		}
		if(on != 0) {				// off
			priv->br_tbl = priv->hbm_inter_br_tbl;
		}
		p_br = priv->bd->props.brightness;
		hbm_nit = priv->hbm_inter_br_tbl[p_br];
		origin_nit = priv->origin_br_tbl[p_br];
		gap_nit = (hbm_nit - origin_nit) / step_max;
		index_array[step_max] = priv->bd->props.brightness;
		pr_info("%s panel %d %d %d %d\n",
			__func__, p_br, hbm_nit, origin_nit, gap_nit);
		for(i = 0; i < step_max; i++) {
			index_array[i] = find_hbm_table(dsim, origin_nit, 1);
			origin_nit += gap_nit;
		}
		for(i = 0; i <= step_max; i++) {
			pr_info("%s panel %d\n", __func__, index_array[i]);
		}
		if(on) {
			for(i = 0; i < step_max; i++) {
				priv->bd->props.brightness = index_array[i];
				if(prev_brightness != priv->bd->props.brightness) {
					dsim_panel_set_brightness(dsim, 1);
					msleep(17);
				}
				prev_brightness = priv->bd->props.brightness;
			}
		} else {
			for(i = step_max; i >= 1; i--) {
				priv->bd->props.brightness = index_array[i];
				if(prev_brightness != priv->bd->props.brightness) {
					dsim_panel_set_brightness(dsim, 1);
					msleep(17);
				}
				prev_brightness = priv->bd->props.brightness;
			}
		}
		priv->bd->props.brightness = index_array[step_max];
		if(on == 0) {				// off
			priv->br_tbl = priv->origin_br_tbl;
		}
	}
	dsim_panel_set_brightness(dsim, 1);
}

#endif


#ifdef CONFIG_LCD_HBM_INTERPOLATION

static ssize_t weakness_hbm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_private *priv = dev_get_drvdata(dev);

	sprintf(buf, "%d\n", priv->weakness_hbm_comp);

	return strlen(buf);
}

static ssize_t weakness_hbm_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;
	int value, i;
	int force = 0;
	const int step_max = 5;
	int old_acl, new_acl, p_br;
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);

	dsim = container_of(priv, struct dsim_device, priv);

	rc = kstrtouint(buf, (unsigned int)0, &value);

	if (rc < 0)
		return rc;
	else {
		if (priv->weakness_hbm_comp != value){
			if((priv->weakness_hbm_comp == HBM_COLORBLIND_ON) && (value == HBM_GALLERY_ON)) {
				pr_info("%s don't support color blind -> gallery\n", __func__);
				return size;
			}
			p_br = priv->bd->props.brightness;
			if((priv->weakness_hbm_comp == HBM_COLORBLIND_ON) || (value == HBM_COLORBLIND_ON)) {
				hbm_set_brightness(dsim, value);
				priv->weakness_hbm_comp = value;
			} else if((priv->weakness_hbm_comp == HBM_GALLERY_ON) || (value == HBM_GALLERY_ON)) {
				old_acl = get_panel_acl_on( p_br, priv->weakness_hbm_comp );
				new_acl = get_panel_acl_on( p_br, value );

				if( value == HBM_GALLERY_ON && old_acl != new_acl ) {
					for( i = 1; i <=step_max ; i++ )
					{
						if( priv->bd->props.brightness != p_br ) break;	// if brightness is changed in outside, stop ACL-step

						dsim_panel_set_acl_step(dsim, old_acl, new_acl, step_max+1, i );
						msleep( 20 );
					}
				}

				priv->weakness_hbm_comp = value;
				force = 1;
			} else {
				pr_info("%s invalid input %d \n", __func__, value);
				return size;
			}

			dsim_panel_set_brightness(dsim, force);

			dev_info(dev, "%s: %d, %d\n", __func__, priv->weakness_hbm_comp, value);
		}
	}
	return size;
}

static DEVICE_ATTR(weakness_hbm_comp, 0664, weakness_hbm_show, weakness_hbm_store);
#endif

#ifdef CONFIG_LCD_WEAKNESS_CCB

void ccb_set_mode(struct dsim_device *dsim, u8 ccb, int stepping)
{
	u8 ccb_cmd[3] = {0xB7, 0x00, 0x00};
	u8 secondval = 0;

	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if(dsim->priv.panel_type == LCD_TYPE_S6E3HF4_WQHD) {
		ccb_cmd[1] = ccb;
		ccb_cmd[2] = 0x2A;
		dsim_write_hl_data(dsim, ccb_cmd, ARRAY_SIZE(ccb_cmd));
	} else if(dsim->priv.panel_type == LCD_TYPE_S6E3HA3_WQHD) {
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
	} else {
		pr_info("%s unknown panel \n", __func__);
	}

	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
}

static ssize_t weakness_ccb_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t weakness_ccb_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int type, serverity, bChangeBrightness = true;
	unsigned char set_ccb = 0x00;
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);

	dsim = container_of(priv, struct dsim_device, priv);

	ret = sscanf(buf, "%d %d", &type, &serverity);

	if (ret < 0)
		return ret;

	dev_info(dev, "%s: type = %d serverity = %d\n", __func__, type, serverity);

	if(priv->ccb_support == UNSUPPORT_CCB) {
		pr_info("%s this panel does not support CCB!\n", __func__);
		return size;
	}

	if((type < 0) || (type > 3)) {
		dev_info(dev, "%s: type is out of range\n", __func__);
		ret = -EINVAL;
	} else if((serverity < 0) || (serverity > 9)) {
		dev_info(dev, "%s: serverity is out of range\n", __func__);
		ret = -EINVAL;
	} else {
		set_ccb = ((u8)(serverity) << 4);
		switch(type) {
		case 0:
			if(priv->current_ccb == 0)
				bChangeBrightness = false;
			set_ccb = 0;
			dev_info(dev, "%s: disable ccb\n", __func__);
			break;
		case 1:
			if(priv->current_ccb != 0)
				bChangeBrightness = false;
			set_ccb += 1;
			dev_info(dev, "%s: enable red\n", __func__);
			break;
		case 2:
			if(priv->current_ccb != 0)
				bChangeBrightness = false;
			set_ccb += 5;
			dev_info(dev, "%s: enable green\n", __func__);
			break;
		case 3:
			if(priv->current_ccb != 0)
				bChangeBrightness = false;
			if(serverity == 0) {
				set_ccb += 9;
				dev_info(dev, "%s: enable blue\n", __func__);
			} else {
				set_ccb = 0;
				set_ccb += 9;
				dev_info(dev, "%s: serverity is out of range, blue only support 0\n", __func__);
			}
			break;
		default:
			set_ccb = 0;
			break;
		}
		if(priv->current_ccb == set_ccb) {
			dev_info(dev, "%s: aleady set same ccb\n", __func__);
		} else {
			ccb_set_mode(dsim, set_ccb, 1);
			if(bChangeBrightness)
				hbm_set_brightness(dsim, set_ccb);
			priv->current_ccb = set_ccb;
		}
	}
	dev_info(dev, "%s: --\n", __func__);

	return size;
}
static DEVICE_ATTR(weakness_ccb, 0664, weakness_ccb_show, weakness_ccb_store);

#endif


#ifdef CONFIG_PANEL_DDI_SPI
static int ddi_spi_enablespi_send(struct dsim_device *dsim)
{
	int ret =0;

	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
	dsim_write_hl_data(dsim, SEQ_B0_13, ARRAY_SIZE(SEQ_B0_13));
	dsim_write_hl_data(dsim, SEQ_D8_02, ARRAY_SIZE(SEQ_D8_02));
	dsim_write_hl_data(dsim, SEQ_B0_1F, ARRAY_SIZE(SEQ_B0_1F));
	dsim_write_hl_data(dsim, SEQ_FE_06, ARRAY_SIZE(SEQ_FE_06));
	msleep(30);
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	dsim_write_hl_data(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));

	return ret;
}

static ssize_t read_copr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static int string_size = 70;
	char temp[string_size];
	int* copr;

	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	struct panel_private *panel;

	dsim = container_of(priv, struct dsim_device, priv);
	panel = &dsim->priv;

	if(panel->state == PANEL_STATE_RESUMED)
	{
		ddi_spi_enablespi_send(dsim);

		mutex_lock(&panel->lock);
		copr = ddi_spi_read_opr();
		mutex_unlock(&panel->lock);

		pr_info("%s %x %x %x %x %x %x %x %x %x %x \n", __func__,
				copr[0], copr[1], copr[2], copr[3], copr[4],
				copr[5], copr[6], copr[7], copr[8], copr[9]);

		snprintf((char *)temp, sizeof(temp), "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			copr[0], copr[1], copr[2], copr[3], copr[4], copr[5], copr[6], copr[7], copr[8], copr[9]);
	}
	else
	{
		snprintf((char *)temp, sizeof(temp), "00 00 00 00 00 00 00 00 00 00\n");
	}

	strlcat(buf, temp, string_size);

	return strnlen(buf, string_size);

}

static DEVICE_ATTR(read_copr, 0444, read_copr_show, NULL);

#endif

#ifdef CONFIG_LCD_BURNIN_CORRECTION
static ssize_t ldu_correction_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t ldu_correction_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct dsim_device *dsim;
	struct panel_private *priv = dev_get_drvdata(dev);
	int rc;
	int value;

	dsim = container_of(priv, struct dsim_device, priv);
	rc = kstrtouint(buf, (unsigned int)0, &value);
	if (rc < 0)
		return rc;
	else {
		if((value < 0) || (value > 7)) {
			pr_info("%s out of range %d\n", __func__, value);
			return -EINVAL;
		}
		if(priv->ldu_tbl[0] == NULL) {
			pr_info("%s do not support ldu correction %d\n", __func__, value);
			return -EINVAL;
		}

		priv->ldu_correction_state = value;
		priv->br_tbl = priv->ldu_tbl[priv->ldu_correction_state];
		dev_info(dev, "%s: %d\n", __func__, priv->ldu_correction_state);
		dsim_panel_set_brightness(dsim, 1);
	}
	return size;
}
static DEVICE_ATTR(ldu_correction, 0664, ldu_correction_show, ldu_correction_store);

#endif


static DEVICE_ATTR(auto_brightness_level, 0444, auto_brightness_level_show, NULL);
static DEVICE_ATTR(lcd_type, 0444, lcd_type_show, NULL);
static DEVICE_ATTR(window_type, 0444, window_type_show, NULL);
static DEVICE_ATTR(manufacture_code, 0444, manufacture_code_show, NULL);
static DEVICE_ATTR(cell_id, 0444, cell_id_show, NULL);
static DEVICE_ATTR(brightness_table, 0444, brightness_table_show, NULL);
static DEVICE_ATTR(auto_brightness, 0664, auto_brightness_show, auto_brightness_store);
static DEVICE_ATTR(siop_enable, 0664, siop_enable_show, siop_enable_store);
static DEVICE_ATTR(temperature, 0664, temperature_show, temperature_store);
static DEVICE_ATTR(color_coordinate, 0444, color_coordinate_show, NULL);
static DEVICE_ATTR(manufacture_date, 0444, manufacture_date_show, NULL);
static DEVICE_ATTR(read_mtp, 0664, read_mtp_show, read_mtp_store);
#ifdef CONFIG_PANEL_S6E3HF4_WQHD
static DEVICE_ATTR(partial_disp, 0664, partial_disp_show, partial_disp_store);
#endif
#ifdef CONFIG_PANEL_AID_DIMMING
static DEVICE_ATTR(aid_log, 0444, aid_log_show, NULL);
#endif

void lcd_init_sysfs(struct dsim_device *dsim)
{
	int ret = -1;

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_lcd_type);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_window_type);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_manufacture_code);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_cell_id);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_brightness_table);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->priv.bd->dev, &dev_attr_auto_brightness);
	if (ret < 0)
		dev_err(&dsim->priv.bd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_siop_enable);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_temperature);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_color_coordinate);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->lcd->dev, &dev_attr_manufacture_date);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

	ret = device_create_file(&dsim->priv.bd->dev, &dev_attr_auto_brightness_level);
	if (ret < 0)
		dev_err(&dsim->priv.bd->dev, "failed to add sysfs entries, %d\n", __LINE__);

#ifdef CONFIG_PANEL_AID_DIMMING
	ret = device_create_file(&dsim->lcd->dev, &dev_attr_aid_log);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif
	ret = device_create_file(&dsim->lcd->dev, &dev_attr_read_mtp);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);

#ifdef CONFIG_PANEL_S6E3HF4_WQHD
	ret = device_create_file(&dsim->lcd->dev, &dev_attr_partial_disp);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif

#if defined(CONFIG_SEC_FACTORY) && defined(CONFIG_EXYNOS_DECON_LCD_MCD)
	ret = device_create_file(&dsim->lcd->dev, &dev_attr_mcd_mode);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif

#ifdef CONFIG_LCD_HMT
#ifdef CONFIG_PANEL_AID_DIMMING
	ret = device_create_file(&dsim->lcd->dev, &dev_attr_hmt_bright);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif
	ret = device_create_file(&dsim->lcd->dev, &dev_attr_hmt_on);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif

#ifdef CONFIG_LCD_ALPM
	ret = device_create_file(&dsim->lcd->dev, &dev_attr_alpm);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif

#ifdef CONFIG_LCD_DOZE_MODE
	ret = device_create_file(&dsim->lcd->dev, &dev_attr_alpm);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif
#ifdef CONFIG_LCD_HBM_INTERPOLATION
	ret = device_create_file(&dsim->priv.bd->dev ,&dev_attr_weakness_hbm_comp);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif

#ifdef CONFIG_LCD_WEAKNESS_CCB
	ret = device_create_file(&dsim->priv.bd->dev ,&dev_attr_weakness_ccb);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif
#ifdef CONFIG_PANEL_DDI_SPI
	ret = device_create_file(&dsim->lcd->dev ,&dev_attr_read_copr);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif

#ifdef CONFIG_LCD_BURNIN_CORRECTION
	ret = device_create_file(&dsim->lcd->dev, &dev_attr_ldu_correction);
	if (ret < 0)
		dev_err(&dsim->lcd->dev, "failed to add sysfs entries, %d\n", __LINE__);
#endif

}


