/* drivers/video/fbdev/exynos/decon_8890/panels/s6e3hf4_lcd_ctrl.c
 *
 * Samsung SoC MIPI LCD CONTROL functions
 *
 * Copyright (c) 2015 Samsung Electronics
 *
 * Jiun Yu, <jiun.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "s6e3hf4_param.h"
#include "lcd_ctrl.h"

/* use FW_TEST definition when you test CAL on firmware */
/* #define FW_TEST */
#ifdef FW_TEST
#include "../dsim_fw.h"
#include "mipi_display.h"
#else
#include "../dsim.h"
#include <video/mipi_display.h>
#endif

/* Porch values. It depends on command or video mode */
#define S6E3HF4_CMD_VBP		15
#define S6E3HF4_CMD_VFP		3
#define S6E3HF4_CMD_VSA		1
#define S6E3HF4_CMD_HBP		2
#define S6E3HF4_CMD_HFP		2
#define S6E3HF4_CMD_HSA		2

#define S6E3HF4_HORIZONTAL	1440
#define S6E3HF4_VERTICAL	2560

#ifdef FW_TEST /* This information is moved to DT */
#define CONFIG_FB_I80_COMMAND_MODE

#define TN

struct decon_lcd s6e3ha3_lcd_info = {
	.mode = DECON_MIPI_COMMAND_MODE,
	.vfp = S6E3HF4_CMD_VFP,
	.vbp = S6E3HF4_CMD_VBP,
	.hfp = S6E3HF4_CMD_HFP,
	.hbp = S6E3HF4_CMD_HBP,
	.vsa = S6E3HF4_CMD_VSA,
	.hsa = S6E3HF4_CMD_HSA,
	.xres = S6E3HF4_HORIZONTAL,
	.yres = S6E3HF4_VERTICAL,

	/* Maybe, width and height will be removed */
	.width = 70,
	.height = 121,

	/* Mhz */
	.hs_clk = 1100,
	.esc_clk = 20,

	.p = 3,
	.m = 127,
	.s = 0,

	.fps = 60,
	.mic_enabled = 0,
	.mic_ver = MIC_VER_1_2,

	.dsc_enabled = 1,
	.dsc_cnt = 2,
	.dsc_slice_num = 4,
};
#endif

/*
 * S6E3HF4 lcd init sequence
 *
 * Parameters
 *	- mic_enabled : if mic is enabled, MIC_ENABLE command must be sent
 *	- mode : LCD init sequence depends on command or video mode
 *	- 1/3 DSC 4 Slices
 */
void lcd_init(int id, struct decon_lcd *lcd)
{
	/* DSC setting */
	if (dsim_wr_data(id, MIPI_DSI_DSC_PRA, (unsigned long)SEQ_DSC_EN[0],
				SEQ_DSC_EN[1]) < 0)
		dsim_err("fail to write SEQ_DSC_EN command.\n");

	switch (lcd->dsc_slice_num)
	{
	case 4:
		if (dsim_wr_data(id, MIPI_DSI_DSC_PPS, (unsigned long)SEQ_PPS_SLICE4,
					ARRAY_SIZE(SEQ_PPS_SLICE4)) < 0)
			dsim_err("fail to write SEQ_PPS_SLICE4 command.\n");
		break;
	case 2:
		if (dsim_wr_data(id, MIPI_DSI_DSC_PPS, (unsigned long)SEQ_PPS_SLICE2,
					ARRAY_SIZE(SEQ_PPS_SLICE2)) < 0)
			dsim_err("fail to write SEQ_PPS_SLICE2 command.\n");
		break;
	default:
		dsim_err("fail to set MIPI_DSI_DSC_PPS command(no slice).\n");
		break;
	}

	/* Sleep Out(11h) */
	if (dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE,
				(unsigned long)SEQ_SLEEP_OUT[0], 0) < 0)
		dsim_err("fail to send SEQ_SLEEP_OUT command.\n");

	dsim_wait_for_cmd_completion(id);
	msleep(120);

	/* Setting the TE timing to prevent LCD tearing */
	/* KEY_ON -> Setting -> KEY_OFF */
	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TEST_KEY_ON_F0,
				ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) < 0)
		dsim_err("fail to write KEY_ON init command.\n");
	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TE_START_SETTING,
				ARRAY_SIZE(SEQ_TE_START_SETTING)) < 0)
		dsim_err("fail to write TE_START_SETTING command.\n");
	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TEST_KEY_OFF_F0,
				ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0)) < 0)
		dsim_err("fail to write KEY_OFF init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE, SEQ_TE_ON[0], 0) < 0)
		dsim_err("fail to write SEQ_TE_ON init command.\n");
}

void lcd_enable(int id)
{
	if (dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE, SEQ_DISPLAY_ON[0], 0) < 0)
		dsim_err("fail to send SEQ_DISPLAY_ON command.\n");
}

void lcd_disable(int id)
{
	/* This function needs to implement */
}

int dsim_write_by_panel(int id, const u8 *cmd, u32 cmd_size)
{
	int ret;

	if (cmd_size == 1)
		ret = dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE,
				cmd[0], 0);
	else if (cmd_size == 2)
		ret = dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE_PARAM,
				cmd[0], cmd[1]);
	else
		ret = dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE,
				(unsigned long)cmd, cmd_size);

	return ret;
}

int dsim_read_from_panel(int id, u8 addr, u32 size, u8 *buf)
{
	int ret;

	ret = dsim_rd_data(id, MIPI_DSI_DCS_READ, (u32)addr, size, buf);

	return ret;
}

static int s6e3hf4_wqhd_dump(int dsim)
{
	int ret = 0;
	unsigned char rx_buf[DSIM_DDI_ID_LEN + 1];

	dsim_info(" + %s\n", __func__);
	ret = dsim_write_by_panel(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0)
		dsim_err("%s : fail to write CMD : KEY_ON_F0\n", __func__);

	ret = dsim_write_by_panel(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
	if (ret < 0)
		dsim_err("%s : fail to write CMD : KEY_ON_FC\n", __func__);

	ret = dsim_read_from_panel(dsim, MIPI_DCS_GET_POWER_MODE, DSIM_DDI_ID_LEN, rx_buf);
	if (ret != DSIM_DDI_ID_LEN) {
		dsim_err("%s : can't read POWER_MODE Reg\n",__func__);
		goto dump_exit;
	}

	dsim_info("=== Panel's POWER_MODE Reg Value : %x ===\n", rx_buf[0]);

	if (rx_buf[0] & 0x80)
		dsim_info("* Booster Voltage Status : ON\n");
	else
		dsim_info("* Booster Voltage Status : OFF\n");

	if (rx_buf[0] & 0x40)
		dsim_info("* Idle Mode : On\n");
	else
		dsim_info("* Idle Mode : OFF\n");

	if (rx_buf[0] & 0x20)
		dsim_info("* Partial Mode : On\n");
	else
		dsim_info("* Partial Mode : OFF\n");

	if (rx_buf[0] & 0x10)
		dsim_info("* Sleep OUT and Working Ok\n");
	else
		dsim_info("* Sleep IN\n");

	if (rx_buf[0] & 0x08)
		dsim_info("* Normal Mode On and Working Ok\n");
	else
		dsim_info("* Sleep IN\n");

	if (rx_buf[0] & 0x04)
		dsim_info("* Display On and Working Ok\n");
	else
		dsim_info("* Display Off\n");

	ret = dsim_read_from_panel(dsim, MIPI_DCS_GET_SIGNAL_MODE, DSIM_DDI_ID_LEN, rx_buf);
	if (ret != DSIM_DDI_ID_LEN) {
		dsim_err("%s : can't read SIGNAL_MODE Reg\n",__func__);
		goto dump_exit;
	}

	dsim_info("=== Panel's SIGNAL_MODE Reg Value : %x ===\n", rx_buf[0]);

	if (rx_buf[0] & 0x80)
		dsim_info("* TE On\n");
	else
		dsim_info("* TE OFF\n");

	if (rx_buf[0] & 0x40)
		dsim_info("* TE MODE on\n");

	if (rx_buf[0] & 0x01) {
		/* get a value of protocol violation error */
		ret = dsim_read_from_panel(dsim, 0xEA, DSIM_DDI_ID_LEN, rx_buf);
		if (ret != DSIM_DDI_ID_LEN) {
			dsim_err("%s : can't read Panel's Protocol\n",__func__);
			goto dump_exit;
		}

		dsim_err("* Protocol violation: buf[0] = %x\n", rx_buf[0]);
		dsim_err("* Protocol violation: buf[1] = %x\n", rx_buf[1]);
	}

	ret = dsim_write_by_panel(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
	if (ret < 0)
		dsim_err("%s : fail to write CMD : KEY_OFF_FC\n", __func__);

	ret = dsim_write_by_panel(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	if (ret < 0)
		dsim_err("%s : fail to write CMD : KEY_OFF_F0\n", __func__);
dump_exit:
	dsim_info(" - %s\n", __func__);
	return ret;

}

int lcd_dump(int id)
{
	s6e3hf4_wqhd_dump(id);
	return 0;
}
