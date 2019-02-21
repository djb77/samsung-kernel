/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _SCORE_REGS_H
#define _SCORE_REGS_H

/* SCore SFR */
#define SCORE_ENABLE				(0x0000)
/* HOST Command Check, 0x0 : No Command, 0x1 : New Command  */
#define SCORE_HOST2SCORE			(0x0004)
#define SCORE_SCORE2HOST			(0x0008)

#define SCORE_CH0_VDMA_CONTROL			(0x0010)
#define SCORE_CH1_VDMA_CONTROL			(0x0014)
#define SCORE_CH2_VDMA_CONTROL			(0x0018)
#define SCORE_CH3_VDMA_CONTROL			(0x001c)

#define SCORE_CONFIGURE				(0x003C)
/* score sw reset, 0x1 : Core Reset, 0x2 : DMA Reset, 0x4 : Cache Reset */
#define SCORE_SW_RESET				(0x0040)
#define SCORE_CLOCK_STATUS			(0x0044)
#define SCORE_IDLE				(0x0054)
/* CODE Firmware ADDRESS */
#define SCORE_CODE_START_ADDR			(0x1000)
/* DATA FW ADDRESS */
#define SCORE_DATA_START_ADDR			(0x2000)
/* CODE MODE */
#define SCORE_CODE_MODE_ADDR			(0x1004)
/* DATA MODE */
#define SCORE_DATA_MODE_ADDR			(0x2004)
/* CACHE CONTROL */
#define SCORE_DC_CCTRL				(0x2008)
#define SCORE_DC_RCTRL_MODE			(0x200C)
#define SCORE_DC_RCTRL_ADDR			(0x2010)
#define SCORE_DC_RCTRL_SIZE			(0x2014)

/* CACHE signal */
#define SCORE_AXIM1AWCACHE			(0x0118)
#define SCORE_AXIM1ARCACHE			(0x011c)

#define SCORE_AXIMAXLEN				(0x002C)
#define SCORE_AXIM0AWUSER			(0x0030)
#define SCORE_AXIM0ARUSER			(0x0034)
#define SCORE_AXIM1AWUSER			(0x0038)
#define SCORE_AXIM1ARUSER			(0x003C)

#define SCORE_INT_CLEAR				(0x004c)
#define SCORE_DSP_INT				(0x0050)

/* @brief SFR variable for indicating which interrupts come from SCore to HOST */
#define SCORE_INT_CODE				(0x0064)

/* @brief Information for SCORE_INT_CODE */
#define SCORE_INT_ABNORMAL_MASK			(0x0000FFFF)
#define SCORE_INT_HW_EXCEPTION_MASK		(0x0007 << 0)
#define SCORE_INT_BUS_ERROR			(0x0001 << 0)
#define SCORE_INT_DMA_CONFLICT			(0x0002 << 0)
#define SCORE_INT_CACHE_UNALIGNED		(0x0004 << 0)

#define SCORE_INT_DUMP_PROFILER_MASK		(0x000F << 4)
#define SCORE_INT_CORE_DUMP_HAPPEN		(0x0001 << 4)
#define SCORE_INT_CORE_DUMP_DONE		(0x0002 << 4)
#define SCORE_INT_PROFILER_START		(0x0004 << 4)
#define SCORE_INT_PROFILER_END			(0x0008 << 4)

#define SCORE_INT_SW_ASSERT			(0x0001 << 9)

#define SCORE_INT_NORMAL_MASK			(0xFFFF0000)
#define SCORE_INT_OUTPUT_QUEUE			(0x0001 << 16)

#ifdef ENABLE_TIME_STAMP
/* @brief  SFR variable for timer7 counter */
#define SCORE_TIMER_CNT7			(0x01F8)
#endif

/* register size = 32bit & 4bytes */
#define SCORE_REG_SIZE				(4)

#endif
