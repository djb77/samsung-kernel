/* linux/drivers/media/platform/exynos/astc-regs-v1.h
 *
 * Register definition file for Samsung ASTC encoder driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrea Bernabei <a.bernabei@samsung.com>
 * Author: Ramesh Munikrishnappa <ramesh.munikrishn@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ASTC_REGS_V1_H_
#define ASTC_REGS_V1_H_

#define KBit0 (1 << 0)
#define KBit1 (1 << 1)
#define KBit2 (1 << 2)
#define KBit3 (1 << 3)
#define KBit4 (1 << 4)
#define KBit5 (1 << 5)
#define KBit6 (1 << 6)
#define KBit7 (1 << 7)
#define KBit8 (1 << 8)
#define KBit9 (1 << 9)
#define KBit10 (1 << 10)
#define KBit11 (1 << 11)
#define KBit12 (1 << 12)
#define KBit13 (1 << 13)
#define KBit14 (1 << 14)
#define KBit15 (1 << 15)
#define KBit16 (1 << 16)
#define KBit17 (1 << 17)
#define KBit18 (1 << 18)
#define KBit19 (1 << 19)

/* Register and bit definitions for ASTC encoder */

/* Software Reset Register */
#define ASTC_SOFT_RESET_REG                        0x00
#define ASTC_SOFT_RESET_SOFT_RST                   KBit0  // Bit mask
#define ASTC_SOFT_RESET_RELEASE                    0x00
#define ASTC_SOFT_RESET_RESET                      0x01

/* Control Register */
#define ASTC_CTRL_REG                              0x04
#define ASTC_CTRL_REG_COLOR_FORMAT_INDEX           7
#define ASTC_CTRL_REG_DUAL_PLANE_INDEX             6
#define ASTC_CTRL_REG_REFINE_ITER_INDEX            4
#define ASTC_CTRL_REG_PARTITION_EN_INDEX           3
#define ASTC_CTRL_REG_NUM_BLK_MODE_INDEX           2
#define ASTC_CTRL_REG_ENC_BLK_SIZE_INDEX           0


/* Interrupt Enable Register */
#define ASTC_INT_EN_REG                             0x08
#define ASTC_INT_EN_REG_IRQ_EN                      KBit0
#define ASTC_INT_EN_REG_IRQ_EN_ENABLED              0x01
#define ASTC_INT_EN_REG_IRQ_EN_DISABLED             0x00


/* Interrupt Status Register */
#define ASTC_INT_STAT_REG                          0x0C
#define ASTC_INT_STAT_REG_INT_STATUS               KBit0
#define ASTC_INT_STAT_REG_INT_STATUS_CLEAR         0x00
#define ASTC_INT_STAT_REG_INT_STATUS_ISSUED        0x01


/* Encoder Status Register */
#define ASTC_ENC_STAT_REG                          0x10
#define ASTC_ENC_STAT_REG_WR_BUS_ERR               KBit1
#define ASTC_ENC_STAT_REG_WR_BUS_ERR_NO_ERROR      0x00
#define ASTC_ENC_STAT_REG_WR_BUS_ERR_BUS_ERROR     0x01
#define ASTC_ENC_STAT_REG_RD_BUS_ERR               KBit0
#define ASTC_ENC_STAT_REG_RD_BUS_ERR_NO_ERROR      0x00
#define ASTC_ENC_STAT_REG_RD_BUS_ERR_BUS_ERROR     0x01


/* Image Size Register */
#define ASTC_IMG_SIZE_REG                          0x14
#define ASTC_IMG_SIZE_REG_IMG_SIZE_X_INDEX         16
#define ASTC_IMG_SIZE_REG_IMG_SIZE_Y_INDEX         0

/* The Number of Blocks (on x/y axis) Register */
#define ASTC_NUM_BLK_REG                           0x18
#define ASTC_NUM_BLK_REG_FIELD_MASK                0x3fff
#define ASTC_NUM_BLK_REG_NUM_BLK_X_INDEX           16
#define ASTC_NUM_BLK_REG_NUM_BLK_Y_INDEX           0

/* Source Base Address Register */
#define ASTC_SRC_BASE_ADDR_REG                     0x1C

/* Destination Base Address Register */
#define ASTC_DST_BASE_ADDR_REG                     0x20

/* ASTC Magic Number Register */
#define ASTC_MAGIC_NUM_REG                         0x24
#define ASTC_MAGIC_NUM_REG_RESETVALUE              0x5CA1AB13

/* Encoding Start Register */
#define ASTC_ENC_START_REG                         0x28
#define ASTC_ENC_START_REG_START                   0x01
#define ASTC_ENC_START_REG_NOTUSED                 0x00

/* Encoding Done Registerr */
#define ASTC_ENC_DONE_REG                          0x2C
#define ASTC_ENC_DONE_REG_DONE                     KBit0


/* AXI mode register */
#define ASTC_AXI_MODE_REG                          0x30
#define ASTC_AXI_MODE_REG_WR_BURST_LENGTH_MASK     (KBit10 | KBit11)
#define ASTC_AXI_MODE_REG_WR_BURST_LENGTH_INDEX    10
#define ASTC_AXI_MODE_REG_AWUSER                   KBit9
#define ASTC_AXI_MODE_REG_AWUSER_INDEX             9
#define ASTC_AXI_MODE_REG_ARUSER                   KBit8
#define ASTC_AXI_MODE_REG_ARUSER_INDEX             8
#define ASTC_AXI_MODE_REG_AWCACHE                  0xF0
#define ASTC_AXI_MODE_REG_AWCACHE_INDEX            4
#define ASTC_AXI_MODE_REG_ARCACHE                  0x0F
#define ASTC_AXI_MODE_REG_ARCACHE_INDEX            0

/* Version Information Register */
#define ASTC_VERSION_INFO_REG                      0x34

#endif /* ASTC_REGS_V1_H_ */

