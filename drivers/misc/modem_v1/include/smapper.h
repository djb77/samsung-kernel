/*
 * linux/drivers/misc/modem_v1/smapper.h
 *
 * Register definition file for Samsung SMAPPER
 *
 * Copyright (c) 2018 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SMAPPER_H__
#define __SMAPPER_H__

/* base address */
#define SMAPPER_REG_BASE_ADDR	(0x14900000)
#define SMAPPER_CP_BASE_ADDR	(0x5C000000)

/* registers */
#define ADDR_MAP_EN_0		0x0000
#define SRAM_WRITE_CTRL_0	0x0004
#define START_ADDR_0		0x0008
#define ADDR_GRANULATY_0	0x000C
#define APB_STATUS		0x0090
#define CTRL_REG_OFFSET		(0x10)

#define ADDR_MAP_STATUS		0x0100
#define AW_START_ADDR		0x0104
#define AW_END_ADDR		0x0108
#define SMAPPER_Q_CH_DISABLE	0x010C

#define SRAM_BANK0_INDEX	0x1000
#define SRAM_BANK1_INDEX	0x1400
#define SRAM_BANK2_INDEX	0x1800
#define SRAM_BANK3_INDEX	0x1C00
#define SRAM_BANK4_INDEX	0x2000
#define SRAM_BANK5_INDEX	0x2400
#define SRAM_BANK6_INDEX	0x2800
#define SRAM_BANK7_INDEX	0x2C00
#define SRAM_BANK_INDEX_OFFSET	(0x400)

/* numbers */
#define TOTAL_BANK_NUM		(8)
#define MAX_PREALLOC_BANK_NUM	(TOTAL_BANK_NUM-1)
#define ENRTY_PER_BANK		(0x100)
#define MAX_SMAPPER_ENTRY	(TOTAL_BANK_NUM*ENRTY_PER_BANK)
#define SMAPPER_SRAM_OFFSET	(0x00100000)

/* granul */
#define SMAPPER_PKT_SIZE	(SZ_2K)
#define GRANUL_2K		0x0
#define SHIFT_2K		(11)
#define GRANUL_4K		0x1
#define SHIFT_4K		(12)
#define GRANUL_8K		0x2
#define SHIFT_8K		(13)

/* access window */
#define SMAPPER_AW_START	(0x08000000) /*0x8000_0000 >> 4 */
#define SMAPPER_AW_END		(0xffffffff) /*0xF_FFFF_FFFF >> 4 */

#endif
