/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_cal.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_CAL_H
#define __S5P_MFC_CAL_H __FILE__

#include <linux/io.h>

#include "s5p_mfc_reg.h"

#include "s5p_mfc_common.h"

#include "s5p_mfc_utils.h"


#define s5p_mfc_get_int_reason()	(MFC_READL(S5P_FIMV_RISC2HOST_CMD)		\
						& S5P_FIMV_RISC2HOST_CMD_MASK)
#define s5p_mfc_clear_int_sfr()				\
		do {							\
			MFC_WRITEL(0, S5P_FIMV_RISC2HOST_CMD);	\
			MFC_WRITEL(0, S5P_FIMV_RISC2HOST_INT);	\
		} while (0)

static inline int s5p_mfc_check_int_cmd(struct s5p_mfc_dev *dev)
{
	if (MFC_READL(S5P_FIMV_RISC2HOST_INT))
		return MFC_READL(S5P_FIMV_RISC2HOST_CMD);
	else
		return 0;
}

static inline int s5p_mfc_stop_bus(struct s5p_mfc_dev *dev)
{
	unsigned int status;
	unsigned long timeout;

	/* Reset */
	MFC_WRITEL(0x1, S5P_FIMV_MFC_BUS_RESET_CTRL);

	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	/* Check bus status */
	do {
		if (time_after(jiffies, timeout)) {
			mfc_err_dev("Timeout while resetting MFC.\n");
			return -EIO;
		}
		status = MFC_READL(S5P_FIMV_MFC_BUS_RESET_CTRL);
	} while ((status & 0x2) == 0);

	return 0;
}

static inline void s5p_mfc_start_bus(struct s5p_mfc_dev *dev)
{
	int val;

	val = MFC_READL(S5P_FIMV_MFC_BUS_RESET_CTRL);
	val &= ~(0x1);
	MFC_WRITEL(val, S5P_FIMV_MFC_BUS_RESET_CTRL);
}

static inline void s5p_mfc_risc_on(struct s5p_mfc_dev *dev)
{
	s5p_mfc_clean_dev_int_flags(dev);

	MFC_WRITEL(0x1, S5P_FIMV_RISC_ON);
	if (FW_HAS_HWACG(dev))
		MFC_WRITEL(0x0, S5P_FIMV_MFC_OFF);
	mfc_debug(1, "RISC_ON\n");
	MFC_TRACE_DEV(">> RISC ON\n");
}

static inline void s5p_mfc_risc_off(struct s5p_mfc_dev *dev)
{
	unsigned int status;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	/* Check pending status */
	do {
		if (time_after(jiffies, timeout)) {
			mfc_err_dev("Timeout while pendng clear\n");
			mfc_err_dev("MFC access pending state: %#x\n", status);
			mfc_err_dev("MFC access pending R: %#x, W: %#x\n",
					MFC_READL(S5P_FIMV_MFC_RPEND),
					MFC_READL(S5P_FIMV_MFC_WPEND));
			break;
		}
		status = MFC_READL(S5P_FIMV_MFC_BUS_STATUS);
	} while (status != 0);

	MFC_WRITEL(0x0, S5P_FIMV_RISC_ON);
}

static inline void s5p_mfc_mfc_off(struct s5p_mfc_dev *dev)
{
	if (FW_HAS_HWACG(dev)) {
		mfc_info_dev("MFC h/w state: %d\n",
				MFC_READL(S5P_FIMV_MFC_STATE) & 0x7);
		MFC_WRITEL(0x1, S5P_FIMV_MFC_OFF);
	}
}

static inline void s5p_mfc_enable_all_clocks(struct s5p_mfc_dev *dev)
{
	/* Enable all FW clock gating */
	MFC_WRITEL(0xFFFFFFFF, S5P_FIMV_MFC_FW_CLOCK);
}

int s5p_mfc_reset_mfc(struct s5p_mfc_dev *dev);
void s5p_mfc_set_risc_base_addr(struct s5p_mfc_dev *dev,
				enum mfc_buf_usage_type buf_type);
void s5p_mfc_cmd_host2risc(struct s5p_mfc_dev *dev, int cmd);

#endif /* __S5P_MFC_CAL_H */
