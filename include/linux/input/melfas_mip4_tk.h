/*
 * MELFAS MIP4 Touchkey
 *
 * Copyright (C) 2016 MELFAS Inc.
 *
 * melfas_mip4_tk.h : Platform data
 *
 * Default path : linux/input/melfas_mip4_tk.h
 */

#ifndef _MIP4_TOUCHKEY_H
#define _MIP4_TOUCHKEY_H

#define MIP_USE_CALLBACK	0 // 0 or 1 : Callback for inform charger, display, power, etc...

#define MIP_DEV_NAME	"mip4_tk"

/*
 * Platform Data
 */
struct mip4_tk_platform_data {
	u32 gpio_intr;
	u32 gpio_pwr_en;
	u32 gpio_bus_en;
	const char *pwr_reg_name;
	const char *bus_reg_name;
	const char *firmware_name;
#if MIP_USE_CALLBACK
	void (*register_callback) (void *);
	/* ... */
#endif
};

#endif
