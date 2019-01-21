/*
 * drivers/usb/core/otg_whitelist.h
 *
 * Copyright (C) 2004 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * This OTG and Embedded Host Whitelist is "Targeted Peripheral List".
 * It should mostly use of USB_DEVICE() or USB_DEVICE_VER() entries..
 *
 * YOU _SHOULD_ CHANGE THIS LIST TO MATCH YOUR PRODUCT AND ITS TESTING!
 */
#include <linux/usb_notify.h>

static int is_targeted_for_samsung_mdm(struct usb_device *dev)
{
	/* NOTE: can't use usb_match_id() since interface caches
	 * aren't set up yet. this is cut/paste from that code.
	 */
	return usb_check_whitelist_for_mdm(dev);
}

