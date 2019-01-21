/* Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.
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

#ifndef QPNP_PON_H
#define QPNP_PON_H

#include <linux/errno.h>

/**
 * enum pon_trigger_source: List of PON trigger sources
 * %PON_SMPL:		PON triggered by SMPL - Sudden Momentary Power Loss
 * %PON_RTC:		PON triggered by RTC alarm
 * %PON_DC_CHG:		PON triggered by insertion of DC charger
 * %PON_USB_CHG:	PON triggered by insertion of USB
 * %PON_PON1:		PON triggered by other PMIC (multi-PMIC option)
 * %PON_CBLPWR_N:	PON triggered by power-cable insertion
 * %PON_KPDPWR_N:	PON triggered by long press of the power-key
 */
enum pon_trigger_source {
	PON_SMPL = 1,
	PON_RTC,
	PON_DC_CHG,
	PON_USB_CHG,
	PON_PON1,
	PON_CBLPWR_N,
	PON_KPDPWR_N,
};

/**
 * enum pon_power_off_type: Possible power off actions to perform
 * %PON_POWER_OFF_RESERVED:          Reserved, not used
 * %PON_POWER_OFF_WARM_RESET:        Reset the MSM but not all PMIC peripherals
 * %PON_POWER_OFF_SHUTDOWN:          Shutdown the MSM and PMIC completely
 * %PON_POWER_OFF_HARD_RESET:        Reset the MSM and all PMIC peripherals
 */
enum pon_power_off_type {
	PON_POWER_OFF_RESERVED		= 0x00,
	PON_POWER_OFF_WARM_RESET	= 0x01,
	PON_POWER_OFF_SHUTDOWN		= 0x04,
	PON_POWER_OFF_HARD_RESET	= 0x07,
	PON_POWER_OFF_DVDD_HARD_RESET	= 0x08,
	PON_POWER_OFF_MAX_TYPE		= 0x10,
};

enum pon_restart_reason {
	PON_RESTART_REASON_UNKNOWN		= 0x00,
	PON_RESTART_REASON_RECOVERY		= 0x01,
	PON_RESTART_REASON_BOOTLOADER		= 0x02,
	PON_RESTART_REASON_RTC			= 0x03,
	PON_RESTART_REASON_DMVERITY_CORRUPTED	= 0x04,
	PON_RESTART_REASON_DMVERITY_ENFORCE	= 0x05,
	PON_RESTART_REASON_KEYS_CLEAR		= 0x06,
/* CONFIG_SEC_BSP */
	PON_RESTART_REASON_NORMALBOOT		= 0x14,
	PON_RESTART_REASON_DOWNLOAD		= 0x15,
	PON_RESTART_REASON_NVBACKUP		= 0x16,
	PON_RESTART_REASON_NVRESTORE		= 0x17,
	PON_RESTART_REASON_NVERASE		= 0x18,
	PON_RESTART_REASON_NVRECOVERY		= 0x19,
#ifdef CONFIG_SEC_PERIPHERAL_SECURE_CHK
	PON_RESTART_REASON_SECURE_CHECK_FAIL	= 0x1A,
#endif
	PON_RESTART_REASON_WATCH_DOG		= 0x1B,
	PON_RESTART_REASON_KERNEL_PANIC		= 0x1C,
	PON_RESTART_REASON_THERMAL		= 0x1D,
	PON_RESTART_REASON_POWER_RESET		= 0x1E,
	PON_RESTART_REASON_WTSR			= 0x1F,
/***********************************************/
	PON_RESTART_REASON_RORY_START		= 0x20,
   /* here is reserved for rory download. */
   /* don't use betwwen PON_RESTART_REASON_RORY_START */
   /*   & PON_RESTART_REASON_RORY_END */
	PON_RESTART_REASON_RORY_END		= 0x2A,
/***********************************************/
	PON_RESTART_REASON_DBG_LOW		= 0x30,
	PON_RESTART_REASON_DBG_MID		= 0x31,
	PON_RESTART_REASON_DBG_HIGH		= 0x32,
	PON_RESTART_REASON_CP_DBG_ON		= 0x33,
	PON_RESTART_REASON_CP_DBG_OFF		= 0x34,
	PON_RESTART_REASON_CP_MEM_RESERVE_ON	= 0x35,
	PON_RESTART_REASON_CP_MEM_RESERVE_OFF	= 0x36,
	PON_RESTART_REASON_FIRMWAREUPDATE	= 0x37,
#ifdef CONFIG_MUIC_SUPPORT_RUSTPROOF
/* SWITCHSEL_MODE use 0x38 ~ 0x3F */
	PON_RESTART_REASON_SWITCHSEL		= 0x38, /* SET_SWITCHSEL_MODE */
	PON_RESTART_REASON_RUSTPROOF		= 0x3F, /* RUSTPROOF_MODE */
#endif
	PON_RESTART_REASON_MAX			= 0x40
};

#ifdef CONFIG_QPNP_POWER_ON
int qpnp_pon_system_pwr_off(enum pon_power_off_type type);
int qpnp_pon_is_warm_reset(void);
int qpnp_pon_trigger_config(enum pon_trigger_source pon_src, bool enable);
int qpnp_pon_wd_config(bool enable);
int qpnp_pon_set_restart_reason(enum pon_restart_reason reason);
bool qpnp_pon_check_hard_reset_stored(void);

#else
static int qpnp_pon_system_pwr_off(enum pon_power_off_type type)
{
	return -ENODEV;
}
static inline int qpnp_pon_is_warm_reset(void) { return -ENODEV; }
static inline int qpnp_pon_trigger_config(enum pon_trigger_source pon_src,
							bool enable)
{
	return -ENODEV;
}
int qpnp_pon_wd_config(bool enable)
{
	return -ENODEV;
}
static inline int qpnp_pon_set_restart_reason(enum pon_restart_reason reason)
{
	return -ENODEV;
}
static inline bool qpnp_pon_check_hard_reset_stored(void)
{
	return false;
}
#endif

#if defined(CONFIG_SEC_PM)
int get_pkey_press(void);
int get_vdkey_press(void);
#endif

#endif
