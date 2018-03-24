/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung SoC MIPI-DSIM driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/pm_runtime.h>
#include <linux/of_gpio.h>
#include <linux/device.h>
#include <linux/module.h>
#include <video/mipi_display.h>
#include <soc/samsung/cal-if.h>
#include <dt-bindings/clock/exynos8895.h>

#include "decon.h"
#include "dsim.h"

#if defined(CONFIG_SOC_EXYNOS8895)
#include "regs-dsim.h"
#else
#include "regs-dsim_8890.h"
#endif

#include "mipi_panel_drv.h"

int dsim_log_level = 6;

struct dsim_device *dsim_drvdata[MAX_DSIM_CNT];
EXPORT_SYMBOL(dsim_drvdata);

static int dsim_runtime_suspend(struct device *dev);
static int dsim_runtime_resume(struct device *dev);


static int dsim_ioctl_panel(struct dsim_device *dsim, u32 cmd, void *arg)
{
	int ret;


	if (unlikely(!dsim->panel_sd))
		return -EINVAL;

	ret = v4l2_subdev_call(dsim->panel_sd, core, ioctl, cmd, arg);

	return ret;
}

static void __dsim_dump(struct dsim_device *dsim)
{
	/* change to updated register read mode (meaning: SHADOW in DECON) */
	dsim_reg_enable_shadow_read(dsim->id, 0);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dsim->res.regs, 0xFC, false);
	/* restore to avoid size mismatch (possible config error at DECON) */
	dsim_reg_enable_shadow_read(dsim->id, 1);
}

static void dsim_dump_with_panel(struct dsim_device *dsim)
{
	dsim_info("=== DSIM SFR DUMP ===\n");
	__dsim_dump(dsim);

	if (dsim_ioctl_panel(dsim, PANEL_IOC_PANEL_DUMP, NULL))
		dsim_err("DSIM:ERR:%s:failed to panel dump\n", __func__);
}


static void dsim_dump(struct dsim_device *dsim)
{
	dsim_info("=== DSIM SFR DUMP ===\n");
	__dsim_dump(dsim);
#if 0
	if (dsim_ioctl_panel(dsim, PANEL_IOC_PANEL_DUMP, NULL))
		dsim_err("DSIM:ERR:%s:failed to panel dump\n", __func__);
#endif
}



static int dsim_long_data_wr(struct dsim_device *dsim, u8 *cmd, u32 size)
{
	int i = 0, rest = size;
	u32 tx_size, payload;

	while (rest > 0) {
		payload = 0;
		tx_size = min(rest, 4);
		memcpy(&payload, &cmd[i * 4], tx_size);
		dsim_reg_wr_tx_payload(dsim->id, payload);
		rest -= tx_size;
		i++;
	}

	return rest;
}

static int dsim_wait_for_cmd_fifo_empty(struct dsim_device *dsim, bool must_wait)
{
	int ret = 0;

	if (!must_wait) {
		/* timer is running, but already command is transferred */
		if (dsim_reg_header_fifo_is_empty(dsim->id))
			del_timer(&dsim->cmd_timer);

		dsim_dbg("%s Doesn't need to wait fifo_completion\n", __func__);
		return ret;
	} else {
		del_timer(&dsim->cmd_timer);
		dsim_dbg("%s Waiting for fifo_completion...\n", __func__);
	}

	if (!wait_for_completion_timeout(&dsim->ph_wr_comp, MIPI_WR_TIMEOUT)) {
		if (dsim_reg_header_fifo_is_empty(dsim->id)) {
			reinit_completion(&dsim->ph_wr_comp);
			dsim_reg_clear_int(dsim->id, DSIM_INTSRC_SFR_PH_FIFO_EMPTY);
			return 0;
		}
		ret = -ETIMEDOUT;
	}

	if ((dsim->state == DSIM_STATE_ON) && (ret == -ETIMEDOUT)) {
		dsim_err("%s have timed out\n", __func__);
		__dsim_dump(dsim);
#if !defined(CONFIG_SOC_EXYNOS8895)
		dsim_reg_set_fifo_ctrl(dsim->id, DSIM_FIFOCTRL_INIT_SFR);
#endif
	}
	return ret;
}

/* wait for until SFR fifo is empty */
int dsim_wait_for_cmd_done(struct dsim_device *dsim)
{
	int ret = 0;
	/* FIXME: hiber only support for DECON0 */
	struct decon_device *decon = get_decon_drvdata(0);

	decon_hiber_block_exit(decon);

	mutex_lock(&dsim->cmd_lock);
	ret = dsim_wait_for_cmd_fifo_empty(dsim, true);
	mutex_unlock(&dsim->cmd_lock);

	decon_hiber_unblock(decon);

	return ret;
}

static bool dsim_fifo_empty_needed(struct dsim_device *dsim, char data_id, char data0)
{
	/* read case or partial update command */
	if (data_id == MIPI_DSI_DCS_READ
			|| data0 == MIPI_DCS_SET_COLUMN_ADDRESS
			|| data0 == MIPI_DCS_SET_PAGE_ADDRESS) {
		dsim_dbg("%s: id:%x, data=%x\n", __func__, data_id, data0);
		return true;
	}

#if 1	/* Single Packet Transmission */
	/* W/A : It will be removed in EVT1 */
	return true;
#else	/* Multi-Packet Transmission */
	/* Check a FIFO level whether writable or not */
	if (!dsim_reg_is_writable_fifo_state(dsim->id))
		return true;

	return false;
#endif
}

int dsim_write_data(struct dsim_device *dsim, u8 id, u8 *cmd, u32 size)
{
	int i;
	int ret = 0;
	bool must_wait = true;
	u8 tmp_buf[2] = {0, 0};

	struct decon_device *decon = get_decon_drvdata(0);

	decon_hiber_block_exit(decon);

	mutex_lock(&dsim->cmd_lock);
	if (dsim->state != DSIM_STATE_ON) {
		dsim_err("DSIM is not ready. state(%d)\n", dsim->state);
		ret = -EINVAL;
		goto err_exit;
	}
	DPU_EVENT_LOG_CMD(&dsim->sd, id, cmd[0]);

	reinit_completion(&dsim->ph_wr_comp);
	dsim_reg_clear_int(dsim->id, DSIM_INTSRC_SFR_PH_FIFO_EMPTY);

	/* Run write-fail dectector */
	mod_timer(&dsim->cmd_timer, jiffies + MIPI_WR_TIMEOUT);

	switch (id) {
	/* short packet types of packet types for command. */
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
	case MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE:
	case MIPI_DSI_DSC_PRA:
	case MIPI_DSI_COLOR_MODE_OFF:
	case MIPI_DSI_COLOR_MODE_ON:
	case MIPI_DSI_SHUTDOWN_PERIPHERAL:
	case MIPI_DSI_TURN_ON_PERIPHERAL:
		if (size > 2) {
			dsim_err("ERR:%s:wrong size : %d\n", __func__, size);
			ret = -EINVAL;
			goto err_exit;
		}
		for (i = 0; i < size; i++)
			tmp_buf[i] = cmd[i];
		dsim_reg_wr_tx_header(dsim->id, id, tmp_buf[0], tmp_buf[1], false);
		must_wait = dsim_fifo_empty_needed(dsim, id, tmp_buf[0]);
		break;

	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
	case MIPI_DSI_DCS_READ:
		if (size > 2) {
			dsim_err("ERR:%s:wrong size : %d\n", __func__, size);
			ret = -EINVAL;
			goto err_exit;
		}
		for (i = 0; i < size; i++)
			tmp_buf[i] = cmd[i];
		dsim_reg_wr_tx_header(dsim->id, id, tmp_buf[0], tmp_buf[1], true);
		must_wait = dsim_fifo_empty_needed(dsim, id, tmp_buf[0]);
		break;

	/* long packet types of packet types for command. */
	case MIPI_DSI_GENERIC_LONG_WRITE:
	case MIPI_DSI_DCS_LONG_WRITE:
	case MIPI_DSI_DSC_PPS:
		dsim_long_data_wr(dsim, cmd, size);
		dsim_reg_wr_tx_header(dsim->id, id, (u8)(size & 0xff), (u8)((size & 0xff00) >> 8), false);
		must_wait = dsim_fifo_empty_needed(dsim, id, cmd[0]);
		break;

	default:
		dsim_info("data id %x is not supported.\n", id);
		ret = -EINVAL;
	}

	ret = dsim_wait_for_cmd_fifo_empty(dsim, must_wait);
	if (ret < 0) {
		dsim_err("ID(%d): DSIM cmd wr timeout 0x%x\n", id, cmd[0]);
		goto err_exit;
	}
	ret = size;
err_exit:
	mutex_unlock(&dsim->cmd_lock);
	decon_hiber_unblock(decon);

	return ret;
}

int dsim_read_data(struct dsim_device *dsim, u8 id, u8 addr, u8 *buf, u16 size)
{
	int i, j;
	int ret = 0;
	char addr_buf;
	char size_buf[2] = {0, };
	u32 rx_fifo, rx_size = 0;
	u32 rx_fifo_depth = DSIM_RX_FIFO_MAX_DEPTH;
	struct decon_device *decon = get_decon_drvdata(0);

	decon_hiber_block_exit(decon);

	if (dsim->state != DSIM_STATE_ON) {
		dsim_err("DSIM is not ready. state(%d)\n", dsim->state);
		decon_hiber_unblock(decon);
		return -EINVAL;
	}
	addr_buf = addr;

	size_buf[0] = (u8)size & 0xff;
	size_buf[1] = (u8)(size >> 8) & 0xff;


	reinit_completion(&dsim->rd_comp);

	/* Init RX FIFO before read and clear DSIM_INTSRC */
#if !defined(CONFIG_SOC_EXYNOS8895)
	dsim_reg_set_fifo_ctrl(dsim->id, DSIM_FIFOCTRL_INIT_RX);
#endif
	dsim_reg_clear_int(dsim->id, DSIM_INTSRC_RX_DATA_DONE);

	/* Set the maximum packet size returned */
	ret = dsim_write_data(dsim, MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE, size_buf, 2);
	if (ret != 2) {
		dsim_err("ERR:%s:failed to write maxium read size\n", __func__);
		return -EIO;
	}

	ret = dsim_write_data(dsim, id, &addr_buf, 1);
	if (ret != 1) {
		dsim_err("ERR:%s:failed to write addreess\n", __func__);
		return -EIO;
	}

	if (!wait_for_completion_timeout(&dsim->rd_comp, MIPI_RD_TIMEOUT)) {
		dsim_err("ERR:%s:fifo empty timed out\n", __func__);
		return -ETIMEDOUT;
	}

	mutex_lock(&dsim->cmd_lock);

	DPU_EVENT_LOG_CMD(&dsim->sd, id, addr);
	do {
		rx_fifo = dsim_reg_get_rx_fifo(dsim->id);

		/* Parse the RX packet data types */
		switch (rx_fifo & 0xff) {
		case MIPI_DSI_RX_ACKNOWLEDGE_AND_ERROR_REPORT:
			ret = dsim_reg_rx_err_handler(dsim->id, rx_fifo);
			if (ret < 0) {
				__dsim_dump(dsim);
				goto exit_read_data;
			}
			break;
		case MIPI_DSI_RX_END_OF_TRANSMISSION:
			dsim_dbg("EoTp was received from LCD module.\n");
			break;
		case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_1BYTE:
		case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_2BYTE:
		case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_1BYTE:
		case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_2BYTE:
			dsim_dbg("Short Packet was received from LCD module.\n");
			for (i = 0; i <= size; i++)
				buf[i] = (rx_fifo >> (8 + i * 8)) & 0xff;
			rx_size = size;
			break;
		case MIPI_DSI_RX_DCS_LONG_READ_RESPONSE:
		case MIPI_DSI_RX_GENERIC_LONG_READ_RESPONSE:
			dsim_dbg("Long Packet was received from LCD module.\n");
			rx_size = (rx_fifo & 0x00ffff00) >> 8;
			dsim_dbg("rx fifo : %8x, response : %x, rx_size : %d\n",
					rx_fifo, rx_fifo & 0xff, rx_size);
			/* Read data from RX packet payload */
			for (i = 0; i < rx_size >> 2; i++) {
				rx_fifo = dsim_reg_get_rx_fifo(dsim->id);
				for (j = 0; j < 4; j++)
					buf[(i*4)+j] = (u8)(rx_fifo >> (j * 8)) & 0xff;
			}
			if (rx_size % 4) {
				rx_fifo = dsim_reg_get_rx_fifo(dsim->id);
				for (j = 0; j < rx_size % 4; j++)
					buf[4 * i + j] =
						(u8)(rx_fifo >> (j * 8)) & 0xff;
			}
			break;
		default:
			dsim_err("Packet format is invaild.\n");
			ret = -EBUSY;
			break;
		}
	} while (!dsim_reg_rx_fifo_is_empty(dsim->id) && --rx_fifo_depth);

	ret = rx_size;
	if (!rx_fifo_depth) {
		dsim_err("Check DPHY values about HS clk.\n");
		__dsim_dump(dsim);
		ret = -EBUSY;
	}
exit_read_data:
	mutex_unlock(&dsim->cmd_lock);
	decon_hiber_unblock(decon);

	return ret;
}

static int __mipi_write_data(struct dsim_device *dsim, u8 cmd_id, const u8 *cmd, int size)
{
	int ret = 0;
	int retry = 5;
	unsigned char dsi_cmd = 0;

	if (cmd_id == MIPI_DSI_WRITE) {
		if (size == 1)
			dsi_cmd = MIPI_DSI_DCS_SHORT_WRITE;
		/*
		   When send TE_ON(35h) command with 1 para type, DSI goes to BTA.
		   To prevent unintended BTA in that case, tx TE_ON(35h) command with 1 para
		   will be sent with long packet type.
		 */
		else if (size == 2 && cmd && cmd[0] != 0x35)
			dsi_cmd = MIPI_DSI_DCS_SHORT_WRITE_PARAM;
		else
			dsi_cmd = MIPI_DSI_DCS_LONG_WRITE;
	} else {
		dsi_cmd = cmd_id;
	}

write_command:
	ret = dsim_write_data(dsim, dsi_cmd, (u8 *)cmd, size);
	if (ret != size) {
		retry--;
		if (retry) {
			dsim_info("INFO:%s:retry dsim write command : %d\n", __func__, retry);
			goto write_command;
		} else {
			dsim_err("ERR:%s:failed to dsim write command\n", __func__);
		}
	}
	return ret;
}


int mipi_write_gen_cmd(struct dsim_device *dsim, u8 dsi, const u8 *cmd, int size)
{
	u8 cmd_buf[DSIM_FIFO_SIZE];
	u8 g_param[2] = {0xB0, 0x00};
	int ret, remained, tx_size, data_offset, tx_data_size;
	int fifo_size = DSIM_FIFO_SIZE;

	if (dsim->version == 0)
		fifo_size = 40;

	data_offset = 1;
	remained = size - data_offset;

	do {
		if (g_param[1] != 0) {
			ret = __mipi_write_data(dsim, MIPI_DSI_WRITE, g_param, 2);
			if (ret != 2) {
				dsim_err("DISP:ERR:%s:failed to write global command\n", __func__);
				goto fail_write_command;
			}

			cmd_buf[0] = cmd[0];
			tx_size = min(remained + 1, fifo_size);

			tx_data_size = (tx_size - 1);
			memcpy((u8 *)cmd_buf + 1, (u8 *)cmd + data_offset + g_param[1], tx_data_size);
			ret = __mipi_write_data(dsim, MIPI_DSI_WRITE, cmd_buf, tx_size);
			if (ret != tx_size) {
				dsim_err("DISP:ERR:%s:failed to write command\n", __func__);
				goto fail_write_command;
			}

		} else {
			tx_size = min(remained + data_offset, fifo_size);
			tx_data_size = (tx_size - data_offset);
			ret = __mipi_write_data(dsim, dsi, cmd, tx_size);
			if (ret != tx_size) {
				dsim_err("DISP:ERR:%s:failed to write command\n", __func__);
				goto fail_write_command;
			}
		}

		g_param[1] += tx_data_size;
		remained -= tx_data_size;
	} while (remained > 0);

fail_write_command:
	return ret;

#if 0
	int ret;
	u8 cmd_buf[DSIM_FIFO_SIZE];
	u8 g_param[2] = {0xB0, 0x00};
	s32 t_tx_size, tx_size, remind_size;
	u32 fifo_size = DSIM_FIFO_SIZE;

	if (dsim->version == 0)
		fifo_size = 40;

	tx_size = (size > fifo_size) ? fifo_size : size;

	ret = __mipi_write_data(dsim, dsi, cmd, tx_size);
	if (ret != tx_size) {
		dsim_err("DISP:ERR:%s:failed to write command\n", __func__);
		goto err_write_gen_cmd;
	}
#if 0
	pr_info("%s, addr 0x%02X, offset %d, len %d\n",
			__func__, cmd[0], g_param[1], tx_size);
	print_data((u8 *)cmd, tx_size);
#endif
	t_tx_size = tx_size;
	remind_size = size - t_tx_size;

	 while (remind_size) {

		g_param[1] = (u8)t_tx_size + 1;
		ret = __mipi_write_data(dsim, MIPI_DSI_WRITE, g_param, 2);
		if (ret != 2) {
			dsim_err("DISP:ERR:%s:failed to write global command\n", __func__);
			goto err_write_gen_cmd;
		}

		cmd_buf[0] = cmd[0];

		//tx_size = min(remind_size, DSIM_FIFO_SIZE - 1);
		tx_size = (remind_size > fifo_size - 1) ? fifo_size - 1 : remind_size;
		memcpy((u8 *)cmd_buf + 1, (u8 *)cmd + t_tx_size, tx_size);

		ret = __mipi_write_data(dsim, MIPI_DSI_WRITE, cmd_buf, tx_size + 1);
		if (ret != tx_size + 1) {
			dsim_err("DISP:ERR:%s:failed to write command\n", __func__);
			goto err_write_gen_cmd;
		}
#if 0
		pr_info("%s, addr 0x%02X, offset %d, len %d\n",
				__func__, cmd_buf[0], g_param[1], tx_size);
		print_data(cmd_buf, tx_size);
#endif
		t_tx_size += tx_size;
		remind_size -= tx_size;
	}

	return size;

err_write_gen_cmd:
	return ret;
#endif
}


int mipi_write_pps_cmd(struct dsim_device *dsim, const u8 *cmd, int size)
{
	int ret;
	u8 cmd_buf[DSIM_FIFO_SIZE];
	u8 g_param[2] = {0xB0, 0x00};
	s32 t_tx_size, tx_size, remind_size;
	s32 fifo_size = DSIM_FIFO_SIZE;

	if (dsim->version == 0)
		fifo_size = 40;

	tx_size = (size > fifo_size) ? fifo_size : size;

	ret = __mipi_write_data(dsim, MIPI_DSI_DSC_PPS, cmd, tx_size);
	if (ret != tx_size) {
		dsim_err("DISP:ERR:%s:failed to write command\n", __func__);
		goto err_write_gen_cmd;
	}
#if 0
	pr_info("%s, addr 0x%02X, offset %d, len %d\n",
			__func__, cmd[0], g_param[1], tx_size);
	print_data((u8 *)cmd, tx_size);
#endif
	t_tx_size = tx_size;
	remind_size = size - t_tx_size;

	 while (remind_size) {

		g_param[1] = (u8)t_tx_size ;
		ret = __mipi_write_data(dsim, MIPI_DSI_WRITE, g_param, 2);
		if (ret != 2) {
			dsim_err("DISP:ERR:%s:failed to write global command\n", __func__);
			goto err_write_gen_cmd;
		}

		cmd_buf[0] = 0x9E;

		//tx_size = min(remind_size, DSIM_FIFO_SIZE - 1);
		tx_size = (remind_size > fifo_size - 1) ? fifo_size - 1 : remind_size;
		memcpy((u8 *)cmd_buf + 1, (u8 *)cmd + t_tx_size, tx_size);

		ret = __mipi_write_data(dsim, MIPI_DSI_WRITE, cmd_buf, tx_size + 1);
		if (ret != tx_size + 1) {
			dsim_err("DISP:ERR:%s:failed to write command\n", __func__);
			goto err_write_gen_cmd;
		}
#if 0
		pr_info("%s, addr 0x%02X, offset %d, len %d\n",
				__func__, cmd_buf[0], g_param[1], tx_size);
		print_data(cmd_buf, tx_size);
#endif
		t_tx_size += tx_size;
		remind_size -= tx_size;
	}

	return size;

err_write_gen_cmd:
	return ret;

}

#define SIDE_RAM_ALIGN_CNT 12


int mipi_write_side_ram(struct dsim_device *dsim, const u8 *cmd, int size)
{
	int ret;
	u8 cmd_buf[DSIM_FIFO_SIZE];
	u32 t_tx_size, tx_size, remind_size;
	u32 fifo_size, tmp;

	dsim_info("%s : was called : %d\n", __func__, size);

	fifo_size = DSIM_FIFO_SIZE;

	/* for only exynos8895 evt0 */
	if (dsim->version == 0)
		fifo_size = 40;

	/* for 12byte align */
	tmp = (fifo_size - 1) / SIDE_RAM_ALIGN_CNT;
	fifo_size = (tmp * SIDE_RAM_ALIGN_CNT) + 1;

	dsim_info("%s : fifo size : %d\n", __func__, fifo_size);

	t_tx_size = 0;
	remind_size = size;

	while (remind_size) {

		if (remind_size == size)
			cmd_buf[0] = MIPI_DSI_OEM1_WR_SIDE_RAM;
		else
			cmd_buf[0] = MIPI_DSI_OEM1_WR_SIDE_RAM2;

		tx_size = min(remind_size, fifo_size - 1);

		memcpy((u8 *)cmd_buf + 1, (u8 *)cmd + t_tx_size, tx_size);

		ret = __mipi_write_data(dsim, MIPI_DSI_WRITE, cmd_buf, tx_size + 1);
		if (ret != tx_size + 1) {
			dsim_err("DISP:ERR(2):%s:failed to write command : %d\n", __func__, ret);
			goto err_write_side_ram;
		}

		t_tx_size += tx_size;
		remind_size -= tx_size;
	}

	return size;

err_write_side_ram:
	return ret;

}

int mipi_write_cmd(u32 id, u8 cmd_id, const u8 *cmd, int size)
{
	int ret = 0;
	struct dsim_device *dsim = NULL;
	struct decon_device *decon = get_decon_drvdata(0);

	if (id >= MAX_DSIM_CNT) {
		dsim_err("ERR:DSIM:%s:invalid id : %d\n", __func__, id);
		return -EINVAL;
	}

	dsim = dsim_drvdata[id];
	if (dsim == NULL) {
		dsim_err("ERR:DSIM:%s:dsim is NULL\n", __func__);
		return -ENODEV;
	}

	decon_hiber_block_exit(decon);

	if (dsim->state != DSIM_STATE_ON) {
		dsim_warn("DSIM:WARN:%s:dsim status : %d\n",
			__func__, dsim->state);
		ret = -EINVAL;
		goto err_write_cmd;
	}

	switch (cmd_id) {
		case MIPI_DSI_DSC_PPS:
			ret = mipi_write_pps_cmd(dsim, cmd, size);
			if (ret != size) {
				dsim_err("DSIM:ERR:%s:failed to write pps cmd\n", __func__);
				goto err_write_cmd;
			}
			break;
		case MIPI_DSI_DSC_PRA:
		case MIPI_DSI_WRITE:
			ret = mipi_write_gen_cmd(dsim, cmd_id, cmd, size);
			if (ret != size) {
				dsim_err("DSIM:ERR:%s:failed to write gen cmd\n", __func__);
				goto err_write_cmd;
			}
			break;
		case MIPI_DSI_OEM1_WR_SIDE_RAM:
			ret = mipi_write_side_ram(dsim, cmd, size);
			if (ret != size) {
				dsim_err("DSIM:ERR:%s:failed to write side ram\n", __func__);
				goto err_write_cmd;
			}
			break;
		default:
			dsim_err("DSIM:ERR:%s:invalid id : %d\n", __func__, cmd_id);
			goto err_write_cmd;
	}
	ret = size;

err_write_cmd:
	decon_hiber_unblock(decon);
	return ret;
}

int mipi_read_cmd(u32 id, u8 addr, u8 *buf, int size)
{
	int ret = 0;
	int retry = 5;
	struct dsim_device *dsim = NULL;

	if (id >= MAX_DSIM_CNT) {
		dsim_err("ERR:DSIM:%s:invalid id : %d\n", __func__, id);
		return -EINVAL;
	}

	dsim = dsim_drvdata[id];
	if (dsim == NULL) {
		dsim_err("ERR:DSIM:%s:dsim is NULL\n", __func__);
		return -ENODEV;
	}

read_command:
	ret = dsim_read_data(dsim, MIPI_DSI_DCS_READ, addr, buf, size);
	if (ret != size) {
		if (retry == 0) {
			dsim_err("ERR:%s:failed to read addr : %x\n", __func__, addr);
		} else {
			retry--;
			goto read_command;
		}
	}
	return ret;
}

enum dsim_state get_dsim_state(u32 id)
{
	struct dsim_device *dsim = NULL;

	dsim = dsim_drvdata[id];
	if (dsim == NULL) {
		dsim_err("ERR:DSIM:%s:dsim is NULL\n", __func__);
		return -ENODEV;
	}
	return dsim->state;
}


static void dsim_cmd_fail_detector(unsigned long arg)
{
	struct dsim_device *dsim = (struct dsim_device *)arg;
	struct decon_device *decon = get_decon_drvdata(0);

	decon_hiber_block(decon);

	dsim_dbg("%s +\n", __func__);
	if (dsim->state != DSIM_STATE_ON) {
		dsim_err("%s: DSIM is not ready. state(%d)\n", __func__,
				dsim->state);
		goto exit;
	}

	/* If already FIFO empty even though the timer is no pending */
	if (!timer_pending(&dsim->cmd_timer)
			&& dsim_reg_header_fifo_is_empty(dsim->id)) {
		reinit_completion(&dsim->ph_wr_comp);
		dsim_reg_clear_int(dsim->id, DSIM_INTSRC_SFR_PH_FIFO_EMPTY);
		goto exit;
	}

	__dsim_dump(dsim);

exit:
	decon_hiber_unblock(decon);
	dsim_dbg("%s -\n", __func__);
	return;
}

static void dsim_underrun_info(struct dsim_device *dsim)
{
	struct decon_device *decon = get_decon_drvdata(0);

	dsim_info("dsim%d: MIF(%lu), INT(%lu), DISP(%lu), total bw(%u, %u)\n",
			dsim->id,
			cal_dfs_get_rate(ACPM_DVFS_MIF),
			cal_dfs_get_rate(ACPM_DVFS_INT),
			cal_dfs_get_rate(ACPM_DVFS_DISP),
			decon->bts.prev_total_bw,
			decon->bts.total_bw);
}

static irqreturn_t dsim_irq_handler(int irq, void *dev_id)
{
	unsigned int int_src;
	struct dsim_device *dsim = dev_id;
	struct decon_device *decon = get_decon_drvdata(0);
	int active;

	spin_lock(&dsim->slock);

	active = pm_runtime_active(dsim->dev);
	if (!active) {
		dsim_info("dsim power(%d), state(%d)\n", active, dsim->state);
		spin_unlock(&dsim->slock);
		return IRQ_HANDLED;
	}

	int_src = readl(dsim->res.regs + DSIM_INTSRC);

	if (int_src & DSIM_INTSRC_SFR_PH_FIFO_EMPTY) {
		del_timer(&dsim->cmd_timer);
		complete(&dsim->ph_wr_comp);
		dsim_dbg("dsim%d PH_FIFO_EMPTY irq occurs\n", dsim->id);
	}
	if (int_src & DSIM_INTSRC_RX_DATA_DONE)
		complete(&dsim->rd_comp);
	if (int_src & DSIM_INTSRC_FRAME_DONE)
		dsim_dbg("dsim%d framedone irq occurs\n", dsim->id);
	if (int_src & DSIM_INTSRC_ERR_RX_ECC)
		dsim_err("RX ECC Multibit error was detected!\n");
#if defined(CONFIG_SOC_EXYNOS8895)
	if (int_src & DSIM_INTSRC_UNDER_RUN) {
		dsim->total_underrun_cnt++;
		dsim_info("dsim%d underrun irq occurs(%d)\n", dsim->id,
				dsim->total_underrun_cnt);
		dsim_underrun_info(dsim);
	}
	if (int_src & DSIM_INTSRC_VT_STATUS) {
		dsim_dbg("dsim%d vt_status(vsync) irq occurs\n", dsim->id);
		decon->vsync.timestamp = ktime_get();
		wake_up_interruptible_all(&decon->vsync.wait);
	}
#endif

	dsim_reg_clear_int(dsim->id, int_src);

	spin_unlock(&dsim->slock);

	return IRQ_HANDLED;
}

static void dsim_clocks_info(struct dsim_device *dsim)
{
#if !defined(CONFIG_SOC_EXYNOS8895)
	dsim_info("%s: %ld Mhz\n", __clk_get_name(dsim->res.pclk),
				clk_get_rate(dsim->res.pclk) / MHZ);
	dsim_info("%s: %ld Mhz\n", __clk_get_name(dsim->res.dphy_esc),
				clk_get_rate(dsim->res.dphy_esc) / MHZ);
	dsim_info("%s: %ld Mhz\n", __clk_get_name(dsim->res.dphy_byte),
				clk_get_rate(dsim->res.dphy_byte) / MHZ);
#endif
}

static void dsim_check_version(struct dsim_device *dsim)
{
	int version;
	version = readl(dsim->res.ver_regs);
	version = (version >> 20) & 0xf;
	dsim->version = version;
	dsim_info("DSIM:INFO:%s:disp ver : %d\n", __func__, version);
}


static int dsim_get_clocks(struct dsim_device *dsim)
{
#if !defined(CONFIG_SOC_EXYNOS8895)
	struct device *dev = dsim->dev;

	dsim->res.pclk = devm_clk_get(dev, "dsim_pclk");
	if (IS_ERR_OR_NULL(dsim->res.pclk)) {
		dsim_err("failed to get dsim_pclk\n");
		return -ENODEV;
	}
	dsim->res.dphy_esc = devm_clk_get(dev, "dphy_escclk");
	if (IS_ERR_OR_NULL(dsim->res.dphy_esc)) {
		dsim_err("failed to get dphy_escclk\n");
		return -ENODEV;
	}

	dsim->res.dphy_byte = devm_clk_get(dev, "dphy_byteclk");
	if (IS_ERR_OR_NULL(dsim->res.dphy_byte)) {
		dsim_err("failed to get dphy_byteclk\n");
		return -ENODEV;
	}
#endif
	return 0;
}

int dsim_reg_ready(struct dsim_device *dsim)
{
	int ret = 0;

#if defined(CONFIG_PM)
	pm_runtime_get_sync(dsim->dev);
#else
	dsim_runtime_resume(dsim->dev);
#endif
	/* Config link to DPHY configuration */
	dpu_sysreg_set_lpmux(dsim->res.ss_regs);

	enable_irq(dsim->res.irq);

	/* DPHY power on : iso release */
	phy_power_on(dsim->phy);

	/* check whether the bootloader init has been done */
	if (dsim->state == DSIM_STATE_INIT) {
		if (dsim_reg_is_pll_stable(dsim->id)) {
			dsim_info("dsim%d PLL is stabled in bootloader, so skip DSIM link/DPHY init.\n", dsim->id);
			ret = 1;
			goto ready_end;
		}
	}

	/* Enable DPHY reset : DPHY reset start */
	dsim_reg_dphy_resetn(dsim->id, 1);

	dsim_reg_sw_reset(dsim->id);

	ret = dsim_reg_set_clocks(dsim->id, &dsim->clks, &dsim->lcd_info->dphy_pms, 1);
	if (ret) {
		dsim_err("ERR:%s:failed to set dsim clock\n", __func__);
		//goto ready_err;
	}
	dsim_reg_set_lanes(dsim->id, dsim->data_lane, 1);

	/* Wait for 200us~ for Dphy stable time and slave acknowlegement */
 	udelay(300);

	dsim_reg_set_esc_clk_on_lane(dsim->id, 1, dsim->data_lane_cnt);

 	dsim_reg_enable_word_clock(dsim->id, 1);

	dsim_reg_dphy_resetn(dsim->id, 0); /* Release DPHY reset */

	ret = dsim_reg_init(dsim->id, dsim->lcd_info, dsim->data_lane_cnt, &dsim->clks);
	if (ret < 0) {
		dsim_info("dsim_%d already enabled\n", dsim->id);
		goto ready_err;
	}

ready_end:
	dsim->state = DSIM_STATE_ON;
	return ret;

ready_err:
	return ret;
}

static int dsim_init_enable(struct dsim_device *dsim)
{
	int ret = 0;
	int power_on = 1;

	if (dsim->panel_state->power == PANEL_POWER_ON) {
		ret = dsim_ioctl_panel(dsim, PANEL_IOC_SET_POWER, (void *)&power_on);
		if (ret) {
			dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
		}
	}

	ret = dsim_reg_ready(dsim);

	if (dsim->state != DSIM_STATE_ON) {
		dsim_err("DSIM:ERR:%s:failed to dsim_reg_ready : wrong state : %d\n"
			, __func__, dsim->state);
		goto init_err;
	}
	if (dsim->panel_state->power == PANEL_POWER_ON) {
		// panel reset
		ret = dsim_ioctl_panel(dsim, PANEL_IOC_SET_PANEL_RESET, NULL);
		if (ret)
			dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
	}

	dsim_reg_start(dsim->id);

#if 0 // minwoo
	if (ret == 0) {
		dsim_info("DSIM:INFO:dsim ready @ kernel side\n");
		ret = dsim_ioctl_panel(dsim, PANEL_IOC_SLEEP_OUT, NULL);
		if (ret) {
			dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
		}
	} else {
		dsim_info("DSIM:INFO:dsim ready @ bootloader side\n");
	}
#endif
	ret = 0;

init_err:
	return ret;
}

static int dsim_enable(struct dsim_device *dsim)
{
	int ret = 0;
	int power_on = 1;
	bool skip_poweron = false;
	if (dsim->state == DSIM_STATE_ON) {
#ifdef CONFIG_SUPPORT_DOZE
		ret = dsim_ioctl_panel(dsim, PANEL_IOC_SLEEP_OUT, NULL);
		if (ret) {
			dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
		}
#endif
		return 0;
	}
	if ((dsim->lcd_info->op_mode == 2) && (dsim->id != 0)) {
		skip_poweron = true;
	}

	if (!skip_poweron) {
		ret = dsim_ioctl_panel(dsim, PANEL_IOC_SET_POWER, (void *)&power_on);
		if (ret)
			dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
	}
	ret = dsim_reg_ready(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to dsim reg ready\n", __func__);
		goto enable_err;
	}
	if (!skip_poweron) {
		if (dsim->panel_state->power == PANEL_POWER_ON) {
			// panel reset
			ret = dsim_ioctl_panel(dsim, PANEL_IOC_SET_PANEL_RESET, NULL);
			if (ret)
				dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
		}
	}
	dsim->state = DSIM_STATE_ON;
	dsim_reg_start(dsim->id);

	if (skip_poweron)
		goto skip_sleep_out;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_SLEEP_OUT, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
	}
skip_sleep_out:
enable_err:
	return ret;
}



static int dsim_disable(struct dsim_device *dsim)
{
	int ret;
	int power_off = 0;

#if  0
	if ((dsim->id != 0) && (dsim->lcd_info->op_mode == DSI_SDDD_MODE)) {
		dsim_info("DSIM:INFO:%s:skip :dual dsi mode id : %d\n",
			__func__, dsim->id);
		return 0;
	}
#endif
	if (dsim->state == DSIM_STATE_OFF)
		return 0;

	if ((dsim->lcd_info->op_mode == 2) && (dsim->id != 0)) {
		goto skip_sleep_in;
	}

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_SLEEP_IN, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel off\n", __func__);
	}
skip_sleep_in:
	/* Wait for current read & write CMDs. */
	mutex_lock(&dsim->cmd_lock);
	del_timer(&dsim->cmd_timer);
	dsim->state = DSIM_STATE_OFF;
	mutex_unlock(&dsim->cmd_lock);

	dsim_reg_stop(dsim->id, dsim->data_lane);
	disable_irq(dsim->res.irq);

	/* HACK */
	phy_power_off(dsim->phy);

	if ((dsim->lcd_info->op_mode == 2) && (dsim->id != 0)) {
		goto skip_power_off;
	}

	power_off = 0;
	ret = dsim_ioctl_panel(dsim, PANEL_IOC_SET_POWER, (void*)&power_off);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel off\n", __func__);
	}
skip_power_off:
#if defined(CONFIG_PM)
	pm_runtime_put_sync(dsim->dev);
#else
	dsim_runtime_suspend(dsim->dev);
#endif

	return 0;
}


#ifdef CONFIG_SUPPORT_DOZE
static int dsim_doze(struct dsim_device *dsim)
{
	int ret = 0;
	int power_on = 1;

	if (dsim->state == DSIM_STATE_ON) {
		ret = dsim_ioctl_panel(dsim, PANEL_IOC_DOZE, NULL);
		if (ret) {
			dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
		}
		return 0;
	}

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_SET_POWER, (void *)&power_on);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
	}

	ret = dsim_reg_ready(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to dsim reg ready\n", __func__);
		goto doze_err;
	}
	dsim->state = DSIM_STATE_ON;

	dsim_reg_start(dsim->id);

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DOZE, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
	}

doze_err:
	return ret;
}


static int dsim_doze_suspend(struct dsim_device *dsim)
{
	int ret;

	if (dsim->state == DSIM_STATE_OFF)
		return 0;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DOZE, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel off\n", __func__);
	}

	/* Wait for current read & write CMDs. */
	mutex_lock(&dsim->cmd_lock);
	del_timer(&dsim->cmd_timer);

	dsim->state = DSIM_STATE_OFF;

	mutex_unlock(&dsim->cmd_lock);

	dsim_reg_stop(dsim->id, dsim->data_lane);
	disable_irq(dsim->res.irq);

	/* HACK */
	phy_power_off(dsim->phy);

#if defined(CONFIG_PM)
	pm_runtime_put_sync(dsim->dev);
#else
	dsim_runtime_suspend(dsim->dev);
#endif

	return 0;
}

#endif
static int dsim_enter_ulps(struct dsim_device *dsim)
{
	int ret = 0;

	DPU_EVENT_START();
	dsim_dbg("%s +\n", __func__);
	exynos_ss_printk("%s:state %d: active %d:+\n", __func__,
				dsim->state, pm_runtime_active(dsim->dev));

	if (dsim->state != DSIM_STATE_ON) {
		ret = -EBUSY;
		goto err;
	}

	/* Wait for current read & write CMDs. */
	mutex_lock(&dsim->cmd_lock);
	dsim->state = DSIM_STATE_ULPS;
	mutex_unlock(&dsim->cmd_lock);

	/* disable interrupts */
	dsim_reg_set_int(dsim->id, 0);

	disable_irq(dsim->res.irq);
	ret = dsim_reg_stop_and_enter_ulps(dsim->id, dsim->lcd_info->ddi_type,
			dsim->data_lane);
	if (ret < 0)
		dsim_dump(dsim);

	phy_power_off(dsim->phy);

#if defined(CONFIG_PM)
	pm_runtime_put_sync(dsim->dev);
#else
	dsim_runtime_suspend(dsim->dev);
#endif

	DPU_EVENT_LOG(DPU_EVT_ENTER_ULPS, &dsim->sd, start);
err:
	dsim_dbg("%s -\n", __func__);
	exynos_ss_printk("%s:state %d: active %d:-\n", __func__,
				dsim->state, pm_runtime_active(dsim->dev));

	return ret;
}

static int dsim_exit_ulps(struct dsim_device *dsim)
{
	int ret = 0;

	DPU_EVENT_START();
	dsim_dbg("%s +\n", __func__);
	exynos_ss_printk("%s:state %d: active %d:+\n", __func__,
				dsim->state, pm_runtime_active(dsim->dev));

	if (dsim->state != DSIM_STATE_ULPS) {
		ret = -EBUSY;
		goto exit_ulps_err;
	}

	ret = dsim_reg_ready(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to dsim reg ready\n", __func__);
		goto exit_ulps_err;
	}

	ret = dsim_reg_exit_ulps_and_start(dsim->id, dsim->lcd_info->ddi_type,
			dsim->data_lane);
	if (ret < 0) {
		dsim_err("DSIM:ERR:%s:failed to exit_ulps_and_start\n", __func__);
		dsim_dump(dsim);
	}

	DPU_EVENT_LOG(DPU_EVT_EXIT_ULPS, &dsim->sd, start);
exit_ulps_err:
	dsim_dbg("%s -\n", __func__);
	exynos_ss_printk("%s:state %d: active %d:-\n", __func__,
				dsim->state, pm_runtime_active(dsim->dev));

	return 0;
}

static int dsim_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct dsim_device *dsim = container_of(sd, struct dsim_device, sd);

	if (enable)
		return dsim_enable(dsim);
	else
		return dsim_disable(dsim);
}

static long dsim_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
#ifdef CONFIG_SUPPORT_DSU
	struct dsu_info *dsu;
#endif
	struct dsim_device *dsim = container_of(sd, struct dsim_device, sd);

	switch (cmd) {

	case DSIM_IOC_ENTER_ULPS:
		if ((unsigned long)arg)
			ret = dsim_enter_ulps(dsim);
		else
			ret = dsim_exit_ulps(dsim);
		break;

	case DSIM_IOC_DUMP:
		dsim_dump_with_panel(dsim);
		break;

	case DSIM_IOC_GET_WCLK:
		v4l2_set_subdev_hostdata(sd, &dsim->clks.word_clk);
		break;
#ifdef CONFIG_SUPPORT_DOZE
	case DSIM_IOC_DOZE:
		ret = dsim_doze(dsim);
		if (ret) {
			dsim_err("DSIM:ERR:%s:failed to doze\n", __func__);
		}
		break;

	case DSIM_IOC_DOZE_SUSPEND:
		ret = dsim_doze_suspend(dsim);
		if (ret) {
			dsim_err("DSIM:ERR:%s:failed to doze suspend\n", __func__);
		}
		break;

#endif

#ifdef CONFIG_SUPPORT_DSU
		case DSIM_IOC_SET_DSU:
			dsim_info("DSIM:INFO:%s:DSIM_IOC_SET_DSU\n",__func__);
			dsu = (struct dsu_info*)v4l2_get_subdev_hostdata(sd);
			if (dsu == NULL) {
				dsim_err("DSIN:ERR:%s:dsu info is null\n", __func__);
				return -EINVAL;
			}
			dsim_info("dsu mode : %d, %d:%d-%d:%d", dsu->mode,
				dsu->left, dsu->top, dsu->right, dsu->bottom);
			dsim_set_dsu_update(dsim->id, dsim->lcd_info);
			break;
#endif
	default:
		dsim_err("unsupported ioctl");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_subdev_core_ops dsim_sd_core_ops = {
	.ioctl = dsim_ioctl,
};

static const struct v4l2_subdev_video_ops dsim_sd_video_ops = {
	.s_stream = dsim_s_stream,
};

static const struct v4l2_subdev_ops dsim_subdev_ops = {
	.core = &dsim_sd_core_ops,
	.video = &dsim_sd_video_ops,
};

static void dsim_init_subdev(struct dsim_device *dsim)
{
	struct v4l2_subdev *sd = &dsim->sd;

	v4l2_subdev_init(sd, &dsim_subdev_ops);
	sd->owner = THIS_MODULE;
	sd->grp_id = dsim->id;
	snprintf(sd->name, sizeof(sd->name), "%s.%d", "dsim-sd", dsim->id);
	v4l2_set_subdevdata(sd, dsim);
}

#if 0
static int __match_panel_v4l2_subdev(struct device *dev, void *data)
{
	struct panel_device *panel;
	struct dsim_device *dsim = (struct dsim_device *)data;

	dsim_info("DSIM:INFO:matched panel drv name : %s\n", dev->driver->name);

	panel = (struct panel_device *)dev_get_drvdata(dev);
	if (panel == NULL) {
		dsim_err("DSIM:ERR:%s:failed to get panel's v4l2 sub dev\n", __func__);
	}
	dsim->panel_sd = &panel->sd;

	return 0;
}
#endif

static int dsim_panel_probe(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DSIM_PROBE, (void *)&dsim->id);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel dsim probe\n", __func__);
		return ret;
	}

	dsim->lcd_info = (struct decon_lcd *)v4l2_get_subdev_hostdata(dsim->panel_sd);
	if (IS_ERR_OR_NULL(dsim->lcd_info)) {
		dsim_err("DSIM:ERR:%s:failed to get lcd information\n", __func__);
		return -EINVAL;
	}
	dsim_info("get ldi name : %s\n", dsim->lcd_info->ldi_name);

	dsim->clks.hs_clk = dsim->lcd_info->hs_clk;
	dsim->clks.esc_clk = dsim->lcd_info->esc_clk;
	dsim->data_lane_cnt = dsim->lcd_info->data_lane_cnt;

	return ret;
}


static int dsim_panel_get_state(struct dsim_device *dsim)
{
	int ret = 0;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_GET_PANEL_STATE, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to get panel staten", __func__);
		return ret;
	}

	dsim->panel_state = (struct panel_state *)v4l2_get_subdev_hostdata(dsim->panel_sd);
	if (IS_ERR_OR_NULL(dsim->panel_state)) {
		dsim_err("DSIM:ERR:%s:failed to get lcd information\n", __func__);
		return -EINVAL;
	}

	return ret;
}

static int dsim_panel_put_ops(struct dsim_device *dsim)
{
	int ret = 0;
	struct mipi_drv_ops mipi_ops;

	mipi_ops.read = mipi_read_cmd;
	mipi_ops.write = mipi_write_cmd;
	mipi_ops.get_state = get_dsim_state;

	v4l2_set_subdev_hostdata(dsim->panel_sd, &mipi_ops);

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DSIM_PUT_MIPI_OPS, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to set mipi ops\n", __func__);
		return ret;
	}

	return ret;
}


#define MAX_PANEL_SD 3
struct v4l2_subdev *panel_sd[MAX_PANEL_SD];

static int __find_panel_v4l2_subdev(struct device *dev, void *data)
{
	char sd_name[30];
	struct panel_device *panel;
	struct dsim_device *dsim = (struct dsim_device *)data;

	dsim_info("DSIM:INFO:matched panel drv name : %s\n", dev->driver->name);

	panel = (struct panel_device *)dev_get_drvdata(dev);
	if (panel == NULL) {
		dsim_err("DSIM:ERR:%s:failed to get panel's v4l2 sub dev\n", __func__);
	}
	sprintf(sd_name, "panel-sd.%d", dsim->id);

	dsim_err("sd_name : %s\n", sd_name);
	if (!strcmp(sd_name, panel->sd.name)) {
		dsim->panel_sd = &panel->sd;
		dsim_info("DSIM:INFO:matched panel drv name : %s\n", dsim->panel_sd->name);
	}
	return 0;
}



static int dsim_get_sd_by_drvname(struct dsim_device *dsim, char *drvname)
{
	struct device_driver *drv;
	struct device *dev;

	drv = driver_find(drvname, &platform_bus_type);
	if (IS_ERR_OR_NULL(drv)) {
		decon_err("failed to find driver\n");
		return -ENODEV;
	}

	dev = driver_find_device(drv, NULL, dsim, __find_panel_v4l2_subdev);

	return 0;
}
static int dsim_probe_panel(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_get_sd_by_drvname(dsim, PANEL_DRV_NAME);
	if (ret) {
		dsim_err("DSIM:ERR:%sfailed to find driver\n", __func__);
		return -ENODEV;
	}

	ret = dsim_panel_probe(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to probe panel", __func__);
		goto do_exit;
	}

	ret = dsim_panel_get_state(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to get panel state\n", __func__);
		goto do_exit;
	}

	ret = dsim_panel_put_ops(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to put ops\n", __func__);
		goto do_exit;
	}

	return ret;

do_exit:
	return ret;
}

static int dsim_cmd_sysfs_write(struct dsim_device *dsim, bool on)
{
	int ret = 0;
	char write_buf;

	if (on)
		write_buf = MIPI_DCS_SET_DISPLAY_ON;
	else
		write_buf = MIPI_DCS_SET_DISPLAY_OFF;

	ret = dsim_write_data(dsim, MIPI_DSI_DCS_SHORT_WRITE, &write_buf, 1);
	if (ret < 0)
		dsim_err("Failed to write test data!\n");
	else
		dsim_dbg("Succeeded to write test data!\n");

	return ret;
}

static int dsim_cmd_sysfs_read(struct dsim_device *dsim)
{
	int ret = 0;
	u8 buf[4];
	unsigned int id;

	/* dsim sends the request for the lcd id and gets it buffer */
	ret = dsim_read_data(dsim, MIPI_DSI_DCS_READ, MIPI_DCS_GET_DISPLAY_ID, buf, DSIM_DDI_ID_LEN);
	id = *(unsigned int *)buf;
	if (ret < 0)
		dsim_err("Failed to read panel id!\n");
	else
		dsim_info("Suceeded to read panel id : 0x%08x\n", id);

	return ret;
}

static ssize_t dsim_cmd_sysfs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t dsim_cmd_sysfs_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long cmd;
	struct dsim_device *dsim = dev_get_drvdata(dev);

	ret = kstrtoul(buf, 0, &cmd);
	if (ret)
		return ret;

	switch (cmd) {
	case 1:
		ret = dsim_cmd_sysfs_read(dsim);
		if (dsim_ioctl_panel(dsim, PANEL_IOC_PANEL_DUMP, NULL))
			dsim_err("DSIM:ERR:%s:failed to panel dump\n", __func__);

		if (ret)
			return ret;
		break;
	case 2:
		ret = dsim_cmd_sysfs_write(dsim, true);
		dsim_info("Dsim write command, display on!!\n");
		if (ret)
			return ret;
		break;
	case 3:
		ret = dsim_cmd_sysfs_write(dsim, false);
		dsim_info("Dsim write command, display off!!\n");
		if (ret)
			return ret;
		break;
	default :
		dsim_info("unsupportable command\n");
		break;
	}

	return count;
}
static DEVICE_ATTR(cmd_rw, 0644, dsim_cmd_sysfs_show, dsim_cmd_sysfs_store);

int dsim_create_cmd_rw_sysfs(struct dsim_device *dsim)
{
	int ret = 0;

	ret = device_create_file(dsim->dev, &dev_attr_cmd_rw);
	if (ret)
		dsim_err("failed to create command read & write sysfs\n");

	return ret;
}

static int dsim_parse_dt(struct dsim_device *dsim, struct device *dev)
{
	if (IS_ERR_OR_NULL(dev->of_node)) {
		dsim_err("no device tree information\n");
		return -EINVAL;
	}

	dsim->id = of_alias_get_id(dev->of_node, "dsim");
	dsim_info("dsim(%d) probe start..\n", dsim->id);

	dsim->phy = devm_phy_get(dev, "dsim_dphy");
	if (IS_ERR_OR_NULL(dsim->phy)) {
		dsim_err("failed to get phy\n");
		return PTR_ERR(dsim->phy);
	}

	dsim->dev = dev;
#if 0
	dsim_get_gpios(dsim);

	dsim_parse_lcd_info(dsim);
#endif
	return 0;
}


static int dsim_get_data_lanes(struct dsim_device *dsim)
{
	int i;

	if (dsim->data_lane_cnt > MAX_DSIM_DATALANE_CNT) {
		dsim_err("%d data lane couldn't be supported\n",
				dsim->data_lane_cnt);
		return -EINVAL;
	}

	dsim->data_lane = DSIM_LANE_CLOCK;
	for (i = 1; i < dsim->data_lane_cnt + 1; ++i)
		dsim->data_lane |= 1 << i;

	dsim_info("%s: lanes(0x%x)\n", __func__, dsim->data_lane);

	return 0;
}

static int dsim_init_resources(struct dsim_device *dsim, struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dsim_err("failed to get mem resource\n");
		return -ENOENT;
	}
	dsim_info("res: start(0x%x), end(0x%x)\n", (u32)res->start, (u32)res->end);

	dsim->res.regs = devm_ioremap_resource(dsim->dev, res);
	if (!dsim->res.regs) {
		dsim_err("failed to remap DSIM SFR region\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dsim_err("failed to get irq resource\n");
		return -ENOENT;
	}

	dsim->res.irq = res->start;
	ret = devm_request_irq(dsim->dev, res->start,
			dsim_irq_handler, 0, pdev->name, dsim);
	if (ret) {
		dsim_err("failed to install DSIM irq\n");
		return -EINVAL;
	}
	disable_irq(dsim->res.irq);

	dsim->res.ss_regs = dpu_get_sysreg_addr();
	if (IS_ERR_OR_NULL(dsim->res.ss_regs)) {
		dsim_err("failed to get sysreg addr\n");
		return -EINVAL;
	}

	dsim->res.ver_regs = dpu_get_version_addr();
	if (IS_ERR_OR_NULL(dsim->res.ver_regs)) {
		dsim_err("failed to get version addr\n");
		return -EINVAL;
	}

	return 0;
}

static int dsim_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct dsim_device *dsim = NULL;

	dsim = devm_kzalloc(dev, sizeof(struct dsim_device), GFP_KERNEL);
	if (!dsim) {
		dsim_err("failed to allocate dsim device.\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = dsim_parse_dt(dsim, dev);
	if (ret)
		goto err_dt;

	dsim_drvdata[dsim->id] = dsim;
	ret = dsim_get_clocks(dsim);
	if (ret)
		goto err_dt;

	spin_lock_init(&dsim->slock);
	mutex_init(&dsim->cmd_lock);
	init_completion(&dsim->ph_wr_comp);
	init_completion(&dsim->rd_comp);

	ret = dsim_init_resources(dsim, pdev);
	if (ret)
		goto err_dt;

	dsim_check_version(dsim);

	dsim_init_subdev(dsim);
	platform_set_drvdata(pdev, dsim);
	dsim_probe_panel(dsim);
#if 0
	ret = probe_mipi_panel_drv(dsim);
	if (ret) {
		dsim_err("ERR:%s:failed to register panel driver\n", __func__);
		goto err_dt;
	}
#endif
	setup_timer(&dsim->cmd_timer, dsim_cmd_fail_detector,
			(unsigned long)dsim);

	pm_runtime_enable(dev);

	ret = dsim_get_data_lanes(dsim);
	if (ret)
		goto err_dt;

	/* HACK */
	phy_init(dsim->phy);
	dsim->state = DSIM_STATE_INIT;

	dsim_init_enable(dsim);

	/* TODO: If you want to enable DSIM BIST mode. you must turn on LCD here */
#if 0
	ret = call_panel_ops(dsim, probe, dsim);
	/* call_panel_ops(dsim, displayon, dsim); */
#else
	ret = v4l2_subdev_call(dsim->panel_sd, core, ioctl, PANEL_IOC_PANEL_PROBE, &dsim->id);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to get panel info from panel's v4l2 subdev\n", __func__);
		return ret;
	}
#endif
	/* TODO: This is for dsim BIST mode in zebu emulator. only for test*/
	dsim_set_bist(dsim->id, false);

	dsim_clocks_info(dsim);
	dsim_create_cmd_rw_sysfs(dsim);

	dsim_info("dsim%d driver(%s mode) has been probed.\n", dsim->id,
		dsim->lcd_info->mode == DECON_MIPI_COMMAND_MODE ? "cmd" : "video");

	return 0;

err_dt:
	kfree(dsim);
err:
	panic("dsim probe failed\n");
	return ret;
}

static int dsim_remove(struct platform_device *pdev)
{
	struct dsim_device *dsim = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	mutex_destroy(&dsim->cmd_lock);
	dsim_info("dsim%d driver removed\n", dsim->id);

	return 0;
}

static void dsim_shutdown(struct platform_device *pdev)
{
	struct dsim_device *dsim = platform_get_drvdata(pdev);

	DPU_EVENT_LOG(DPU_EVT_DSIM_SHUTDOWN, &dsim->sd, ktime_set(0, 0));
	dsim_info("%s + state:%d\n", __func__, dsim->state);

	dsim_disable(dsim);

	dsim_info("%s -\n", __func__);
}

static int dsim_runtime_suspend(struct device *dev)
{
	struct dsim_device *dsim = dev_get_drvdata(dev);

	DPU_EVENT_LOG(DPU_EVT_DSIM_SUSPEND, &dsim->sd, ktime_set(0, 0));
	dsim_dbg("%s +\n", __func__);

#if !defined(CONFIG_SOC_EXYNOS8895)
	clk_disable_unprepare(dsim->res.pclk);
	clk_disable_unprepare(dsim->res.dphy_esc);
	clk_disable_unprepare(dsim->res.dphy_byte);
#endif
	dsim_dbg("%s -\n", __func__);
	return 0;
}

static int dsim_runtime_resume(struct device *dev)
{
	struct dsim_device *dsim = dev_get_drvdata(dev);

	DPU_EVENT_LOG(DPU_EVT_DSIM_RESUME, &dsim->sd, ktime_set(0, 0));
	dsim_dbg("%s: +\n", __func__);

#if !defined(CONFIG_SOC_EXYNOS8895)
	clk_prepare_enable(dsim->res.pclk);
	clk_prepare_enable(dsim->res.dphy_esc);
	clk_prepare_enable(dsim->res.dphy_byte);
#endif
	dsim_dbg("%s -\n", __func__);
	return 0;
}

static const struct of_device_id dsim_of_match[] = {
	{ .compatible = "samsung,exynos8-dsim" },
	{},
};
MODULE_DEVICE_TABLE(of, dsim_of_match);

static const struct dev_pm_ops dsim_pm_ops = {
	.runtime_suspend	= dsim_runtime_suspend,
	.runtime_resume		= dsim_runtime_resume,
};

static struct platform_driver dsim_driver __refdata = {
	.probe			= dsim_probe,
	.remove			= dsim_remove,
	.shutdown		= dsim_shutdown,
	.driver = {
		.name		= DSIM_MODULE_NAME,
		.owner		= THIS_MODULE,
		.pm		= &dsim_pm_ops,
		.of_match_table	= of_match_ptr(dsim_of_match),
		.suppress_bind_attrs = true,
	}
};

static int __init dsim_init(void)
{
	int ret = platform_driver_register(&dsim_driver);
	if (ret)
		pr_err("dsim driver register failed\n");

	return ret;
}
late_initcall(dsim_init);

static void __exit dsim_exit(void)
{
	platform_driver_unregister(&dsim_driver);
}

module_exit(dsim_exit);
MODULE_AUTHOR("Yeongran Shin <yr613.shin@samsung.com>");
MODULE_DESCRIPTION("Samusung EXYNOS DSIM driver");
MODULE_LICENSE("GPL");
