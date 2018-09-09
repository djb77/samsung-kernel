/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_cal.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <trace/events/mfc.h>

#include "s5p_mfc_cal.h"

/* Reset the device */
int s5p_mfc_reset_mfc(struct s5p_mfc_dev *dev)
{
	int i;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	/* Zero Initialization of MFC registers */
	MFC_WRITEL(0, S5P_FIMV_RISC2HOST_CMD);
	MFC_WRITEL(0, S5P_FIMV_HOST2RISC_CMD);
	MFC_WRITEL(0, S5P_FIMV_FW_VERSION);

	for (i = 0; i < S5P_FIMV_REG_CLEAR_COUNT; i++)
		MFC_WRITEL(0, S5P_FIMV_REG_CLEAR_BEGIN + (i*4));

	MFC_WRITEL(0x1FFF, S5P_FIMV_MFC_RESET);
	MFC_WRITEL(0, S5P_FIMV_MFC_RESET);

	mfc_debug_leave();

	return 0;
}

void s5p_mfc_set_risc_base_addr(struct s5p_mfc_dev *dev,
					enum mfc_buf_usage_type buf_type)
{
	struct s5p_mfc_special_buf *fw_buf;

	fw_buf = &dev->fw_buf;

	if (buf_type == MFCBUF_DRM)
		fw_buf = &dev->drm_fw_buf;

	MFC_WRITEL(fw_buf->daddr, S5P_FIMV_RISC_BASE_ADDRESS);
	mfc_debug(2, "%s F/W Base Address : %08llx\n",
			buf_type == MFCBUF_DRM ? "DRM" : "NORMAL", fw_buf->daddr);
	MFC_TRACE_DEV("%s F/W Base Address : %08llx\n",
			buf_type == MFCBUF_DRM ? "DRM" : "NORMAL", fw_buf->daddr);
}

void s5p_mfc_cmd_host2risc(struct s5p_mfc_dev *dev, int cmd)
{
	mfc_debug(1, "Issue the command: %d\n", cmd);
	MFC_TRACE_DEV(">> CMD : %d, (dev:0x%lx, bits:%lx, owned:%d, wl:%d, trans:%d)\n",
			cmd, dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
			dev->hwlock.wl_count, dev->hwlock.transfer_owner);

	trace_mfc_frame_start(dev->curr_ctx, cmd, 0, 0);
	/* Reset RISC2HOST command except nal q stop command */
	if (cmd != S5P_FIMV_H2R_CMD_STOP_QUEUE)
		MFC_WRITEL(0x0, S5P_FIMV_RISC2HOST_CMD);

	/* Start the timeout watchdog */
	if ((cmd != S5P_FIMV_H2R_CMD_NAL_QUEUE) && (cmd != S5P_FIMV_H2R_CMD_STOP_QUEUE))
		s5p_mfc_watchdog_start_tick(dev);

	if (dbg_enable) {
		/* For FW debugging */
		s5p_mfc_dbg_set_addr(dev);
		s5p_mfc_dbg_enable(dev);
	}

	/* Issue the command */
	MFC_WRITEL(cmd, S5P_FIMV_HOST2RISC_CMD);
	MFC_WRITEL(0x1, S5P_FIMV_HOST2RISC_INT);
}
