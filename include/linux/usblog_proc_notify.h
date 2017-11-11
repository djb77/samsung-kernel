/*
 * Copyright (C) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

  /* usb notify layer v2.0 */

#ifndef __LINUX_USBLOG_PROC_NOTIFY_H__
#define __LINUX_USBLOG_PROC_NOTIFY_H__

enum usblog_type {
	NOTIFY_FUNCSTATE,
	NOTIFY_CCIC_EVENT,
	NOTIFY_MANAGER,
	NOTIFY_USBMODE,
	NOTIFY_USBMODE_FUNC,
	NOTIFY_USBSTATE,
	NOTIFY_EVENT,
};

enum usblog_state {
	NOTIFY_CONFIGURED = 1,
	NOTIFY_CONNECTED,
	NOTIFY_DISCONNECTED,
	NOTIFY_RESET,
	NOTIFY_RESET_FULL,
	NOTIFY_RESET_HIGH,
	NOTIFY_RESET_SUPER,
	NOTIFY_ACCSTART,
	NOTIFY_PULLUP,
	NOTIFY_PULLUP_ENABLE,
	NOTIFY_PULLUP_EN_SUCCESS,
	NOTIFY_PULLUP_EN_FAIL,
	NOTIFY_PULLUP_DISABLE,
	NOTIFY_PULLUP_DIS_SUCCESS,
	NOTIFY_PULLUP_DIS_FAIL,
	NOTIFY_VBUS_SESSION,
	NOTIFY_VBUS_SESSION_ENABLE,
	NOTIFY_VBUS_EN_SUCCESS,
	NOTIFY_VBUS_EN_FAIL,
	NOTIFY_VBUS_SESSION_DISABLE,
	NOTIFY_VBUS_DIS_SUCCESS,
	NOTIFY_VBUS_DIS_FAIL,
	NOTIFY_HIGH,
	NOTIFY_SUPER,
};

enum usblog_status {
	NOTIFY_DETACH = 0,
	NOTIFY_ATTACH_DFP,
	NOTIFY_ATTACH_UFP,
	NOTIFY_ATTACH_DRP,
};

enum ccic_device {
	NOTIFY_DEV_INITIAL = 0,
	NOTIFY_DEV_USB,
	NOTIFY_DEV_BATTERY,
	NOTIFY_DEV_PDIC,
	NOTIFY_DEV_MUIC,
	NOTIFY_DEV_CCIC,
	NOTIFY_DEV_MANAGER,
};

enum ccic_id {
	NOTIFY_ID_INITIAL = 0,
	NOTIFY_ID_ATTACH,
	NOTIFY_ID_RID,
	NOTIFY_ID_USB,
	NOTIFY_ID_POWER_STATUS,
	NOTIFY_ID_WATER,
	NOTIFY_ID_VCONN,
};

enum ccic_rid {
	NOTIFY_RID_UNDEFINED = 0,
	NOTIFY_RID_000K,
	NOTIFY_RID_001K,
	NOTIFY_RID_255K,
	NOTIFY_RID_301K,
	NOTIFY_RID_523K,
	NOTIFY_RID_619K,
	NOTIFY_RID_OPEN,
};

enum ccic_con {
	NOTIFY_CON_DETACH = 0,
	NOTIFY_CON_ATTACH,
};

enum ccic_rprd {
	NOTIFY_RD = 0,
	NOTIFY_RP,
};

#ifdef CONFIG_USB_NOTIFY_PROC_LOG
extern void store_usblog_notify(int type, void *param1, void *parma2);
extern void store_ccic_version(unsigned char *hw, unsigned char *sw_main,
			unsigned char *sw_boot);
extern int register_usblog_proc(void);
extern void unregister_usblog_proc(void);
#else
static inline void store_ccic_version(unsigned char *hw, unsigned char *sw_main,
			unsigned char *sw_boot) {}
static inline int register_usblog_proc(void)
			{return 0; }
static inline void unregister_usblog_proc(void) {}
#endif
#endif

