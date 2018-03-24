/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *		http://www.samsung.com/
 *
 * Defines phy types for samsung usb phy controllers - HOST or DEIVCE.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

typedef struct usb_dp_noti {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t is_connect:16;
	uint64_t hs_connect:16;
	uint64_t conn_port1:8;
	uint64_t conn_port2:8;
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	void *pd;
#endif
} USB_DP_NOTI_TYPEDEF;
