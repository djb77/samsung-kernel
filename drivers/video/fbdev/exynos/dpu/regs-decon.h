/*
 * drivers/video/exynos_8890/decon/regs-decon.h
 *
 * Register definition file for Samsung DECON driver
 *
 * Copyright (c) 2014 Samsung Electronics
 * Sewoon Park <seuni.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#ifndef _REGS_DISP_SS_H
#define _REGS_DISP_SS_H

/* [2012-02-13] EVT0_ML3_DEV03 SFR */

/* SYSREG_DPU0 */
#define DISP_LPMUX_CFG					0x0000
/* _v : [0,1] */
#define SEL_DPHY2DSIM_MASK(_v)		(0x3 << (12 + (_v) * 4))
/* _v : [0,2] */
#define SEL_DSIM2DPHY_MASK(_v)		(0x3 << (0 + (_v) * 4))

#define DISP_DLMUX_CFG					0x0004
/* _v : [0,1] */
#define SEL_DISPIF2DSIM_MASK(_v)		(0x3 << (12 + (_v) * 4))
/* _x(dsim_if_id): [0,2] (0,1: for decon0, 2: for decon1)
   _y(dsim_id)   : [0,1]
*/
#define SEL_DISPIF2DSIM(_x, _y)		((_x) << (12 + (_y) * 4))

/* _v : [0,2] */
#define SEL_DSIM2DISPIF_MASK(_v)		(0x7 << (0 + (_v) * 4))
/* _x(dsim_if_id): [0,2] (0,1: for decon0, 2: for decon1)
   _y(dsim_id)   : [0,1] (0=id0, 1=id1)
*/
#define SEL_DSIM2DISPIF(_y, _x)		(((_y) + 1) << (0 + (_x) * 4))


#define DISP_DPU_MIPI_PHY_CON				0x0008
/* _v : [0,1] */
#define SEL_RESET_DPHY_MASK(_v)		(0x1 << (4 + (_v)))
#define M_RESETN_M1S2S2_MODULE_MASK		(0x1 << 2)
#define M_RESETN_M4S4_MODULE_MASK		(0x1 << 1)
#define M_RESETN_M4S4_TOP_MASK		(0x1 << 0)

#define DISP_DPU_TE_SEL				0x000C
/* _v : [0,2] */
#define SEL_TE_MASK(_n)			(0x1 << (_n))
/* _v : [0,1](0=speedy, 1=gpio),  _n (TE id) : [0,2] */
#define SEL_TE_PATH(_n, _v)			((_v) << (_n))
#define TE_FROM_SPEEDY				0
#define TE_FROM_GPIO				1

#endif /* _REGS_DISP_SS_H */


#ifndef _REGS_DECON_H
#define _REGS_DECON_H

/*
 * [ BLK_DPU BASE ADDRESS ]
 *
 * - CMU_DPU0		0x1280_0000
 * - PMU_DPU0		0x1281_0000
 * - SYSREG_DPU0	0x1282_0000 #
 * - DPP_SECURE		0x1283_0000
 * - DECON0_SECURE	0x1284_0000
 * - DPP		0x1285_0000
 * - DECON0		0x1286_0000 #
 * - MIPI_DSIM0		0x1287_0000
 * - MIPI_DSIM1		0x1288_0000
 * - DPU_WB_MUX		0x1289_0000
 * - DPU_DMA_SECURE	0x128A_0000
 * - DPU_DMA		0x128B_0000
 * - BTM_DPUD0		0x128C_0000
 * - BTM_DPUD1		0x128D_0000
 * - BTM_DPUD2		0x128E_0000
 * - SYSMMU_DPUD0	0x1290_0000
 * - SYSMMU_DPUD1	0x1291_0000
 * - SYSMMU_DPUD2	0x1292_0000
 * - SYSMMU_DPUD0_S	0x1293_0000
 * - SYSMMU_DPUD1_S	0x1294_0000
 * - SYSMMU_DPUD2_S	0x1295_0000
 * - PPMU_DPUD0		0x1296_0000
 * - PPMU_DPUD1		0x1297_0000
 * - PPMU_DPUD2		0x1298_0000
 * - CMU_DPU1		0x12A0_0000
 * - PMU_DPU1		0x12A1_0000
 * - SYSREG_DPU1	0x12A2_0000 #
 * - DECON1		0x12A3_0000 #
 * - DECON2		0x12A4_0000 #
*/


/*
 *	IP			start_offset	end_offset
 *=================================================
 *	DECON0			0x0000		0x0304
 *	DECON0/WINx		0x1000		0x1110
 *-------------------------------------------------
 *	DQE			0x2000		0x2328
 *-------------------------------------------------
 *	DSC0			0x4000		0x4078
 *	DSC1			0x5000		0x5078
 *-------------------------------------------------
 *-------------------------------------------------
 *	SHD_DECON0		0x7000		0x7FFF
 *-------------------------------------------------
 *	SHD_DQE			0x9000		0x9FFF
 *-------------------------------------------------
 *	SHD_DSC0		0xB000		0xBFFF
 *	SHD_DSC1		0xC000		0xCFFF
 *-------------------------------------------------
 *	mDNIe			0xD000		0xDFFF
 *-------------------------------------------------
*/

/*
 * DECON_F registers
 * ->
 * updated by SHADOW_REG_UPDATE_REQ[31] : SHADOW_REG_UPDATE_REQ
 *	(0x0000~0x011C, 0x0230~0x209C, Dither/MIC/DSC)
*/

#define GLOBAL_CONTROL					0x0000
#define GLOBAL_CONTROL_SRESET				(1 << 28)
#define GLOBAL_CONTROL_OPERATION_MODE_F		(1 << 8)
#define GLOBAL_CONTROL_OPERATION_MODE_RGBIF_F	(0 << 8)
#define GLOBAL_CONTROL_OPERATION_MODE_I80IF_F	(1 << 8)
#define GLOBAL_CONTROL_IDLE_STATUS			(1 << 5)
#define GLOBAL_CONTROL_RUN_STATUS			(1 << 4)
#define GLOBAL_CONTROL_DECON_EN			(1 << 1)
#define GLOBAL_CONTROL_DECON_EN_F			(1 << 0)

#define RESOURCE_OCCUPANCY_INFO_0			0x0010
#define RESOURCE_OCCUPANCY_INFO_1			0x0014
#define RESOURCE_OCCUPANCY_INFO_2			0x0018

#define SRAM_SHARE_ENABLE				0x0030
#define FLOATING_SRAM_SHARE_ENABLE_F		(1 << 24)
#define FF2_SRAM_SHARE_ENABLE_F		(1 << 20)
#define FF1_SRAM_SHARE_ENABLE_F		(1 << 16)
#define FF0_SRAM_SHARE_ENABLE_F		(1 << 12)
#define ALL_SRAM_SHARE_ENABLE			(0x1111 << 12)
#define ALL_SRAM_SHARE_DISABLE		(0x0000 << 12)

#define INTERRUPT_ENABLE				0x0040
#define DPU_MDNIE_DIMMING_END_INT_EN		(1 << 25)
#define DPU_MDNIE_DIMMING_START_INT_EN	(1 << 24)
#define DPU_DQE_DIMMING_END_INT_EN		(1 << 21)
#define DPU_DQE_DIMMING_START_INT_EN		(1 << 20)
#define DPU_FRAME_DONE_INT_EN			(1 << 13)
#define DPU_FRAME_START_INT_EN		(1 << 12)
#define DPU_FIFO_SEL_FIFO0			(0 << 9)
#define DPU_FIFO_SEL_FIFO1			(1 << 9)
#define DPU_UNDER_FLOW_INT_EN			(1 << 8)
#define DPU_EXTRA_INT_EN			(1 << 4)
#define DPU_INT_EN				(1 << 0)
#define INTERRUPT_ENABLE_MASK			0x3303711

#define EXTRA_INTERRUPT_ENABLE			0x0044
#define DPU_RESOURCE_CONFLICT_INT_EN		(1 << 8)
#define DPU_TIME_OUT_INT_EN			(1 << 4)
#define DPU_ERROR_INT_EN			(1 << 0)

#define TIME_OUT_VALUE					0x0048

#define INTERRUPT_PENDING				0x004C
#define DPU_MDNIE_DIMMING_END_INT_PEND	(1 << 25)
#define DPU_MDNIE_DIMMING_START_INT_PEND	(1 << 24)
#define DPU_DQE_DIMMING_END_INT_PEND		(1 << 21)
#define DPU_DQE_DIMMING_START_INT_PEND	(1 << 20)
#define DPU_FRAME_DONE_INT_PEND		(1 << 13)
#define DPU_FRAME_START_INT_PEND		(1 << 12)
#define DPU_UNDER_FLOW_INT_PEND		(1 << 8)
#define DPU_EXTRA_INT_PEND			(1 << 4)

#define EXTRA_INTERRUPT_PENDING			0x0050
#define DPU_RESOURCE_CONFLICT_INT_PEND	(1 << 8)
#define DPU_TIME_OUT_INT_PEND			(1 << 4)
#define DPU_ERROR_INT_PEND			(1 << 0)

#define SHADOW_REG_UPDATE_REQ				0x0060
#define SHADOW_REG_UPDATE_REQ_GLOBAL		(1 << 31)
#define SHADOW_REG_UPDATE_REQ_DQE		(1 << 28)
#define SHADOW_REG_UPDATE_REQ_MDNIE		(1 << 24)
#define SHADOW_REG_UPDATE_REQ_WIN(_win)	(1 << (_win))
#define SHADOW_REG_UPDATE_REQ_FOR_DECON0	(0x3f)
#define SHADOW_REG_UPDATE_REQ_FOR_DECON2	(0x1f)

#define HW_SW_TRIG_CONTROL				0x0070
#define HW_TRIG_SEL(_v)			((_v) << 24)
#define HW_TRIG_SEL_MASK			(0x3 << 24)
#define HW_TRIG_SEL_FROM_USB			(3 << 24)
#define HW_TRIG_SEL_FROM_DDI2			(2 << 24)
#define HW_TRIG_SEL_FROM_DDI1			(1 << 24)
#define HW_TRIG_SEL_FROM_DDI0			(0 << 24)
#define HW_TRIG_SKIP(_v)			((_v) << 16)
#define HW_TRIG_SKIP_MASK			(0xff << 16)
#define TRIG_AUTO_MASK_EN			(1 << 12)
#define SW_TRIG_EN				(1 << 8)
#define HW_TRIG_EDGE_POLARITY			(1 << 7)
#define HW_TRIG_MASK_DECON			(1 << 4)
#define HW_SW_TRIG_TIMER_CLEAR		(1 << 3)
#define HW_SW_TRIG_TIMER_EN			(1 << 2)
#define HW_TRIG_EN				(1 << 0)

#define HW_SW_TRIG_TIMER				0x0074

#define CLOCK_CONTROL_0				0x00F0
/* [24] QACTIVE_VALUE = 0
 * 0: QACTIVE is dynamically changed by DECON h/w,
 * 1: QACTIVE is stuck to 1'b1
 * [22][20][16][12]+[8][0] AUTO_CG_EN_xxx
*/
#define CLOCK_CONTROL_0_F_MASK		(0x00511101)
#define CLOCK_CONTROL_0_S_MASK		(0x00511000)
#define CLOCK_CONTROL_0_T_MASK		(0x00511000)


#define SPLITTER_SIZE_CONTROL_0			0x0100
#define SPLITTER_HEIGHT_F(_v)			((_v) << 16)
#define SPLITTER_HEIGHT_MASK			(0x3fff << 16)
#define SPLITTER_HEIGHT_GET(_v)		(((_v) >> 16) & 0x3fff)
#define SPLITTER_WIDTH_F(_v)			((_v) << 0)
#define SPLITTER_WIDTH_MASK			(0x3fff << 0)
#define SPLITTER_WIDTH_GET(_v)		(((_v) >> 0) & 0x3fff)

#define SPLITTER_SIZE_CONTROL_1			0x0104

#define SPLITTER_SPLIT_IDX_CONTROL			0x0108
#define SPLITTER_SPLIT_IDX_F(_v)		((_v) << 0)
#define SPLITTER_SPLIT_IDX_MASK		(0x3fff << 0)

#define SPLITTER_OVERLAP_CONTROL			0x010C
#define SPLITTER_OVERLAP_F(_v)		((_v) << 0)
#define SPLITTER_OVERLAP_MASK			(0x3fff << 0)

#define FRAME_FIFO_0_MONITOR_CONTROL			0x0114
#define FRAME_FIFO_1_MONITOR_CONTROL			0x0118

#define FRAME_FIFO_0_SIZE_CONTROL_0			0x0120
#define FRAME_FIFO_HEIGHT_F(_v)		((_v) << 16)
#define FRAME_FIFO_HEIGHT_MASK		(0x3fff << 16)
#define FRAME_FIFO_HEIGHT_GET(_v)		(((_v) >> 16) & 0x3fff)
#define FRAME_FIFO_WIDTH_F(_v)		((_v) << 0)
#define FRAME_FIFO_WIDTH_MASK			(0x3fff << 0)
#define FRAME_FIFO_WIDTH_GET(_v)		(((_v) >> 0) & 0x3fff)

#define FRAME_FIFO_0_SIZE_CONTROL_1			0x0124
/* same field with FRAME_FIFO_0_SIZE_CONTROL_0 */
#define FRAME_FIFO_1_SIZE_CONTROL_0			0x0128
#define FRAME_FIFO_1_SIZE_CONTROL_1			0x012C

#define FRAME_FIFO_TH_CONTROL_0			0x0130
#define FRAME_FIFO_1_TH_F(_v)			((_v) << 16)
#define FRAME_FIFO_1_TH_MASK			(0xffff << 16)
#define FRAME_FIFO_1_TH_GET(_v)		(((_v) >> 16) & 0xffff)
#define FRAME_FIFO_0_TH_F(_v)			((_v) << 0)
#define FRAME_FIFO_0_TH_MASK			(0xffff << 0)
#define FRAME_FIFO_0_TH_GET(_v)		(((_v) >> 0) & 0xffff)

#define FRAME_FIFO_0_LEVEL				0x0134
#define FRAME_FIFO_1_LEVEL				0x0138

/*
 * DECON2 Only
*/
#define DPU_DISPIF_VSTATUS_INT_SEL_MASK		(3 << 17)	/* INTERRUPT_ENABLE */
#define DPU_DISPIF_VSTATUS_INT_SEL(_v)		((_v) << 17)
#define DPU_DISPIF_VSTATUS_INT_EN		(1 << 16)
#define DPU_FIFO_SEL_DISPIF			(2 << 9)
#define DPU_FIFO_SEL_MASK			(3 << 9)
#define DPU_VERTICAL_VSYNC_START		0x01
#define INTERRUPT_ENABLE_MASK_DECON2		0x00073711

#define DPU_DISPIF_VSTATUS_INT_PEND		(1 << 16)	/* INTERRUPT_PENDING */

#define DISPIF_CONTROL				0x0140
#define DISPIF_CLOCK_UNDERRUN_SCHEME_F(_v)	((_v) << 12)
#define DISPIF_CLOCK_UNDERRUN_SCHEME_MASK	(0x3 << 12)
#define DISPIF_RGB_ORDER_F(_v)		((_v) << 8)
#define DISPIF_RGB_ORDER_MASK			(0x7 << 8)
#define DISPIF_CLOCK_FREE_RUN_EN		(1 << 4)
#define DISPIF_VSYNC_ACTIVE_VALUE(_v)		((_v) << 2)
#define DISPIF_VSYNC_ACTIVE_VALUE_MASK		(1 << 2)
#define DISPIF_HSYNC_ACTIVE_VALUE(_v)		((_v) << 1)
#define DISPIF_HSYNC_ACTIVE_VALUE_MASK		(1 << 1)
#define DISPIF_VDEN_ACTIVE_VALUE		(1 << 0)

#define DISPIF_TIMING_CONTROL_0			0x0150
#define DISPIF_VBPD_F(_v)			((_v) << 16)
#define DISPIF_VBPD_MASK			(0xffff << 16)
#define DISPIF_VBPD_GET(_v)			(((_v) >> 16) & 0xffff)
#define DISPIF_VFPD_F(_v)			((_v) << 0)
#define DISPIF_VFPD_MASK			(0xffff << 0)
#define DISPIF_VFPD_GET(_v)			(((_v) >> 0) & 0xffff)

#define DISPIF_TIMING_CONTROL_1			0x0154
#define DISPIF_VSPD_F(_v)			((_v) << 0)
#define DISPIF_VSPD_MASK			(0xffff << 0)
#define DISPIF_VSPD_GET(_v)			(((_v) >> 0) & 0xffff)

#define DISPIF_TIMING_CONTROL_2			0x0158
#define DISPIF_HBPD_F(_v)			((_v) << 16)
#define DISPIF_HBPD_MASK			(0xffff << 16)
#define DISPIF_HBPD_GET(_v)			(((_v) >> 16) & 0xffff)
#define DISPIF_HFPD_F(_v)			((_v) << 0)
#define DISPIF_HFPD_MASK			(0xffff << 0)
#define DISPIF_HFPD_GET(_v)			(((_v) >> 0) & 0xffff)

#define DISPIF_TIMING_CONTROL_3			0x015C
#define DISPIF_HSPD_F(_v)			((_v) << 0)
#define DISPIF_HSPD_MASK			(0xffff << 0)
#define DISPIF_HSPD_GET(_v)			(((_v) >> 0) & 0xffff)

#define DISPIF_SIZE_CONTROL_0				0x0160
#define DISPIF_HEIGHT_F(_v)			((_v) << 16)
#define DISPIF_HEIGHT_MASK			(0xffff << 16)
#define DISPIF_HEIGHT_GET(_v)			(((_v) >> 16) & 0x3fff)
#define DISPIF_WIDTH_F(_v)			((_v) << 0)
#define DISPIF_WIDTH_MASK			(0xffff << 0)
#define DISPIF_WIDTH_GET(_v)			(((_v) >> 0) & 0x3fff)

#define DISPIF_SIZE_CONTROL_1				0x0164
#define DISPIF_PIXEL_COUNT_F(_v)		((_v) << 0)

/*************************************************************/

#define HYSTERESIS_CONTROL				0x0180
#define HYSTERESIS_ENABLE_F			(1 << 0)

#define HYSTERESIS_THRESHOLD				0x0184
#define HYSTERESIS_HIGH_TH_F(_v)		((_v) << 16)
#define HYSTERESIS_HIGH_TH_MASK		(0xffff << 16)
#define HYSTERESIS_HIGH_TH_GET(_v)		(((_v) >> 16) & 0xffff)
#define HYSTERESIS_LOW_TH_F(_v)		((_v) << 0)
#define HYSTERESIS_LOW_TH_MASK		(0xffff << 0)
#define HYSTERESIS_LOW_TH_GET(_v)		(((_v) >> 0) & 0xffff)


#define FORMATTER_CONTROL				0x0190
#define FORMATTER_PIXEL0123_ORDER_SWAP_F	(1 << 16)
#define FORMATTER_PIXEL23_ORDER_SWAP_F	(1 << 12)
#define FORMATTER_PIXEL01_ORDER_SWAP_F	(1 << 8)
#define FORMATTER_PIXEL_ORDER_SWAP(_a, _b, _c)	(_a << 16) | (_b << 12) | (_c << 8)
#define FORMATTER_PIXEL_ORDER_SWAP_MASK	((1 << 16) | (1 << 12) | (1 << 8))
#define FORMATTER_OUT_RGB_ORDER_F(_v)	((_v) << 4)
#define FORMATTER_OUT_RGB_ORDER_MASK		(0x7 << 4)

#define FORMATTER0_SIZE_CONTROL_0			0x01A0
#define FORMATTER_HEIGHT_F(_v)		((_v) << 16)
#define FORMATTER_HEIGHT_MASK		(0x3fff << 16)
#define FORMATTER_HEIGHT_GET(_v)		(((_v) >> 16) & 0x3fff)
#define FORMATTER_WIDTH_F(_v)		((_v) << 0)
#define FORMATTER_WIDTH_MASK			(0x3fff << 0)
#define FORMATTER_WIDTH_GET(_v)		(((_v) >> 0) & 0x3fff)

#define FORMATTER0_SIZE_CONTROL_1			0x01A4
/* same field with FORMATTER0_SIZE_CONTROL_0 */
#define FORMATTER1_SIZE_CONTROL_0			0x01B0
#define FORMATTER1_SIZE_CONTROL_1			0x01B4


#define BLENDER_BG_IMAGE_SIZE_0			0x0200
#define BLENDER_BG_HEIGHT_F(_v)		((_v) << 16)
#define BLENDER_BG_HEIGHT_MASK		(0x3fff << 16)
#define BLENDER_BG_HEIGHT_GET(_v)		(((_v) >> 16) & 0x3fff)
#define BLENDER_BG_WIDTH_F(_v)		((_v) << 0)
#define BLENDER_BG_WIDTH_MASK			(0x3fff << 0)
#define BLENDER_BG_WIDTH_GET(_v)		(((_v) >> 0) & 0x3fff)

#define BLENDER_BG_IMAGE_SIZE_1			0x0204

#define BLENDER_BG_IMAGE_COLOR_0			0x0208
#define BLENDER_BG_A_F(_v)			((_v) << 16)
#define BLENDER_BG_A_MASK			(0xff << 16)
#define BLENDER_BG_A_GET(_v)			(((_v) >> 16) & 0xff)
#define BLENDER_BG_R_F(_v)			((_v) << 0)
#define BLENDER_BG_R_MASK			(0xff << 0)
#define BLENDER_BG_R_GET(_v)			(((_v) >> 0) & 0xff)

#define BLENDER_BG_IMAGE_COLOR_1			0x020C
#define BLENDER_BG_G_F(_v)			((_v) << 16)
#define BLENDER_BG_G_MASK			(0xff << 16)
#define BLENDER_BG_G_GET(_v)			(((_v) >> 16) & 0xff)
#define BLENDER_BG_B_F(_v)			((_v) << 0)
#define BLENDER_BG_B_MASK			(0xff << 0)
#define BLENDER_BG_B_GET(_v)			(((_v) >> 0) & 0xff)

#define LRMERGER_MODE_CONTROL				0x0210
#define LRM23_MODE_F(_v)			((_v) << 16)
#define LRM23_MODE_MASK			(0x7 << 16)
#define LRM01_MODE_F(_v)			((_v) << 0)
#define LRM01_MODE_MASK			(0x7 << 0)


#define DATA_PATH_CONTROL_0				0x0214
#define WIN_MAPCOLOR_EN_F(_win)		(1 << (4*_win + 1))
#define WIN_EN_F(_win)				(1 << (4*_win + 0))

#define DATA_PATH_CONTROL_1				0x0218
/* WIN5_CHMAP_F [22:20]=5 :  dedicated to channel5 */
#define WIN_CHMAP_F(_win, _ch)		(((_ch) & 0x7) << (4*_win))
#define WIN_CHMAP_MASK(_win)			(0x7 << (4*_win))

#define DATA_PATH_CONTROL_2				0x0230
#define DQE_LPD_EXIT_CTRL			(1 << 24)
#define HSC_PATH_F(_v)				((_v) << 20)
#define HSC_PATH_MASK				(0x7 << 20)
#define APS_PATH_F(_v)				((_v) << 16)
#define APS_PATH_MASK				(0x7 << 16)
#define ENHANCE_LOGIC_PATH_F(_v)		((_v) << 12)
#define ENHANCE_LOGIC_PATH_MASK		(0x7 << 12)
#define ENHANCE_LOGIC_PATH_GET(_v)		(((_v) >> 12) & 0x7)
#define COMP_LINKIF_WB_PATH_F(_v)		((_v) << 0)
#define COMP_LINKIF_WB_PATH_MASK		(0x1ff << 0)
#define COMP_LINKIF_WB_PATH_GET(_v)		(((_v) >> 0) & 0x1ff)


#define CRC_DATA					0x0280
#define CRC_DATA_WCLK1_GET(_v)		(((_v) >> 16) & 0xffff)
#define CRC_DATA_WCLK0_GET(_v)		(((_v) >> 0) & 0xffff)

#define CRC_CONTROL					0x0288
#define CRC_BITS_SEL(_v)			((_v) << 16)
#define CRC_BITS_SEL_MASK			(0xf << 16)
#define CRC_START				(1 << 0)


#define FRAME_ID					0x02A0

/* hidden for customer */
#define WAIT_CYCLE_AFTER_SFR_UPDATE			0x02A4

#define DITHER_CONTROL					0x0300
#define DITHER_CONTROL_F(_v)			((_v) << 0)
#define DITHER_CONTROL_MASK			(0x3 << 0)


#define WIN_CONTROL_0(_win)			(0x1000 + ((_win) * 0x30))
#define WIN_ALPHA1_F(_v)			(((_v) & 0xFF) << 24)
#define WIN_ALPHA1_MASK			(0xFF << 24)
#define WIN_ALPHA0_F(_v)			(((_v) & 0xFF) << 16)
#define WIN_ALPHA0_MASK			(0xFF << 16)
#define WIN_ALPHA_GET(_v, _n)		(((_v) >> (16 + 8 * (_n))) & 0xFF)
#define WIN_FUNC_F(_v)				(((_v) & 0xF) << 8)
#define WIN_FUNC_MASK				(0xF << 8)
#define WIN_FUNC_GET(_v)			(((_v) >> 8) & 0xf)
#define WIN_SRESET				(1 << 4)
#define WIN_ALPHA_MULT_SRC_SEL_F(_v)		(((_v) & 0x3) << 0)
#define WIN_ALPHA_MULT_SRC_SEL_MASK		(0x3 << 0)

#define WIN_CONTROL_1(_win)			(0x1004 + ((_win) * 0x30))
#define WIN_FG_ALPHA_D_SEL_F(_v)		(((_v) & 0xF) << 24)
#define WIN_FG_ALPHA_D_SEL_MASK		(0xF << 24)
#define WIN_BG_ALPHA_D_SEL_F(_v)		(((_v) & 0xF) << 16)
#define WIN_BG_ALPHA_D_SEL_MASK		(0xF << 16)
#define WIN_FG_ALPHA_A_SEL_F(_v)		(((_v) & 0xF) << 8)
#define WIN_FG_ALPHA_A_SEL_MASK		(0xF << 8)
#define WIN_BG_ALPHA_A_SEL_F(_v)		(((_v) & 0xF) << 0)
#define WIN_BG_ALPHA_A_SEL_MASK		(0xF << 0)

#define WIN_START_POSITION(_win)		(0x1008 + ((_win) * 0x30))
#define WIN_STRPTR_Y_F(_v)			(((_v) & 0x3FFF) << 16)
#define WIN_STRPTR_X_F(_v)			(((_v) & 0x3FFF) << 0)

#define WIN_END_POSITION(_win)		(0x100C + ((_win) * 0x30))
#define WIN_ENDPTR_Y_F(_v)			(((_v) & 0x3FFF) << 16)
#define WIN_ENDPTR_X_F(_v)			(((_v) & 0x3FFF) << 0)

#define WIN_COLORMAP_0(_win)			(0x1010 + ((_win) * 0x30))
#define WIN_MAPCOLOR_A_F(_v)			((_v) << 16)
#define WIN_MAPCOLOR_A_MASK			(0xff << 16)
#define WIN_MAPCOLOR_R_F(_v)			((_v) << 0)
#define WIN_MAPCOLOR_R_MASK			(0xff << 0)

#define WIN_COLORMAP_1(_win)			(0x1014 + ((_win) * 0x30))
#define WIN_MAPCOLOR_G_F(_v)			((_v) << 16)
#define WIN_MAPCOLOR_G_MASK			(0xff << 16)
#define WIN_MAPCOLOR_B_F(_v)			((_v) << 0)
#define WIN_MAPCOLOR_B_MASK			(0xff << 0)

#define WIN_START_TIME_CONTROL(_win)		(0x1018 + ((_win) * 0x30))
#define WIN_START_TIME_CONTROL_F(_v)		((_v) << 0)
#define WIN_START_TIME_CONTROL_MASK		(0x3fff << 0)

#define WIN_PIXEL_COUNT(_win)			(0x101C + ((_win) * 0x30))


/*
* DQE registers
* ->
* 0x2000 ~
*  updated by SHADOW_REG_UPDATE_REQ[28] : SHADOW_REG_UPDATE_REQ_DQE
*
* @ regs-dqe.h
*
* <-
* DQE registers
*/


/*
* DSC registers
* ->
* 0x4000 ~
*
* @ regs-dsc.h
*
* <-
* DSC registers
*/
#define DSC0_OFFSET					0x4000
#define DSC1_OFFSET					0x5000
#define DSC_NUM_EXTRA_MUX_BIT				246
#define DSC_MIN_SLICE_SIZE				15000
#define DSC_INIT_TRANSMIT_DELAY				0x200
#define DSC_INIT_SCALE_VALUE				0x20
#define DSC_BIT_PER_PIXEL				0x80
#define DSC_FIRST_LINE_BPG_OFFSET			0xC
#define DSC_INIT_OFFSET					0x1800
#define DSC_RC_MODE_SIZE				0x2000


#define DSC_CONTROL0					0x0000
#define DSC_SW_RESET				(0x1 << 28)
#define DSC_DCG_EN_REF(_v)			((_v) << 19)
#define DSC_DCG_EN_SSM(_v)			((_v) << 18)
#define DSC_DCG_EN_ICH(_v)			((_v) << 17)
#define DSC_DCG_EN_BYPASS(_v)			((_v) << 16)
#define DSC_DCG_EN_ALL_OFF			(0x0 << 16)
#define DSC_DCG_EN_ALL_MASK			(0xf << 16)
#define DSC_BIT_SWAP(_v)			((_v) << 10)
#define DSC_BYTE_SWAP(_v)			((_v) << 9)
#define DSC_WORD_SWAP(_v)			((_v) << 8)
#define DSC_SWAP(_b, _c, _w)			(_b << 10) | (_c << 9) | (_w << 8)
#define DSC_SWAP_MASK				((1 << 10) | (1 << 9) | (1 << 8))
#define DSC_FLATNESS_DET_TH_MASK		(0xf << 4)
#define DSC_FLATNESS_DET_TH_F(_v)		((_v) << 4)
#define DSC_SLICE_MODE_CH_MASK		(0x1 << 3)
#define DSC_SLICE_MODE_CH_F(_v)		((_v) << 3)
#define DSC_BYPASS_MASK			(0x1 << 2)
#define DSC_BYPASS_F(_v)			((_v) << 2)
#define DSC_CG_EN_MASK				(0x1 << 1)
#define DSC_CG_EN_F(_v)			((_v) << 1)
#define DSC_DUAL_SLICE_EN_MASK		(0x1 << 0)
#define DSC_DUAL_SLICE_EN_F(_v)		((_v) << 0)

#define DSC_CONTROL1					0x0004
#define DSC_V_FRONT_F(_v)			((_v) << 16)
#define DSC_V_SYNC_F(_v)			((_v) << 8)
#define DSC_V_BACK_F(_v)			((_v) << 0)
#define DSC_V_BLANK_MASK			(0xFFFFFF)

#define DSC_CONTROL2					0x0008
#define DSC_H_FRONT_F(_v)			((_v) << 16)
#define DSC_H_SYNC_F(_v)			((_v) << 8)
#define DSC_H_BACK_F(_v)			((_v) << 0)
#define DSC_H_BLANK_MASK			(0xFFFFFF)


#define DSC_PPS00_03					0x0020

#define DSC_PPS04_07					0x0024
#define PPS04_COMP_CFG(_v)			(_v << 24)
#define PPS04_COMP_CFG_MASK			(0x3F << 24)
#define PPS05_BPP(_v)				(_v << 16)
#define PPS05_BPP_MASK				(0xFF << 16)
#define PPS06_07_PIC_HEIGHT_MASK		(0xFFFF << 0)
#define PPS06_07_PIC_HEIGHT(_v)		(_v << 0)

#define DSC_PPS08_11					0x0028
#define PPS08_09_PIC_WIDHT_MASK		(0xFFFF << 16)
#define PPS08_09_PIC_WIDHT(_v)		((_v) << 16)
#define PPS10_11_SLICE_HEIGHT_MASK		(0xFFFF << 0)
#define PPS10_11_SLICE_HEIGHT(_v)		(_v << 0)

#define DSC_PPS12_15					0x002C
#define PPS12_13_SLICE_WIDTH_MASK		(0xFFFF << 16)
#define PPS12_13_SLICE_WIDTH(_v)		((_v) << 16)
#define PPS14_15_CHUNK_SIZE_MASK		(0xFFFF << 0)
#define PPS14_15_CHUNK_SIZE(_v)		(_v << 0)

#define DSC_PPS16_19					0x0030
#define PPS16_17_INIT_XMIT_DELAY_MASK	(0x3FF << 16)
#define PPS16_17_INIT_XMIT_DELAY(_v)		((_v) << 16)
#define PPS18_19_INIT_DEC_DELAY_MASK		(0xFFFF << 0)
#define PPS18_19_INIT_DEC_DELAY(_v)		((_v) << 0)


#define DSC_PPS20_23					0x0034
#define PPS21_INIT_SCALE_VALUE_MASK		(0x3F << 16)
#define PPS21_INIT_SCALE_VALUE(_v)		((_v) << 16)
#define PPS22_23_SCALE_INC_INTERVAL_MASK	(0xFFFF << 0)
#define PPS22_23_SCALE_INC_INTERVAL(_v)	(_v << 0)

#define DSC_PPS24_27					0x0038
#define PPS24_25_SCALE_DEC_INTERVAL_MASK	(0xFFF << 16)
#define PPS24_25_SCALE_DEC_INTERVAL(_v)	((_v) << 16)
/* FL : First Line */
#define PPS27_FL_BPG_OFFSET_MASK		(0x1F << 0)
#define PPS27_FL_BPG_OFFSET(_v)		(_v << 0)

#define DSC_PPS28_31					0x003C
/* NFL : Not First Line */
#define PPS28_29_NFL_BPG_OFFSET_MASK		(0xFFFF << 16)
#define PPS28_29_NFL_BPG_OFFSET(_v)		((_v) << 16)
#define PPS30_31_SLICE_BPG_OFFSET_MASK	(0xFFFF << 0)
#define PPS30_31_SLICE_BPG_OFFSET(_v)	(_v << 0)

#define DSC_PPS32_35					0x0040
#define PPS32_33_INIT_OFFSET_MASK		(0xFFFF << 16)
#define PPS32_33_INIT_OFFSET(_v)		((_v) << 16)
#define PPS34_35_FINAL_OFFSET_MASK		(0xFFFF << 0)
#define PPS34_35_FINAL_OFFSET(_v)		(_v << 0)

#define DSC_PPS56_59					0x0058
#define PPS56_RC_BUF_THRESH_C_MASK		(0xFF << 24)
#define PPS56_RC_BUF_THRESH_C(_v)		((_v) << 24)
#define PPS57_RC_BUF_THRESH_D_MASK		(0xFF << 16)
#define PPS57_RC_BUF_THRESH_D(_v)		((_v) << 16)
#define PPS58_RC_RANGE_PARAM_MASK		(0xFF << 8)
#define PPS58_RC_RANGE_PARAM(_v)		(_v << 8)
#define PPS59_RC_RANGE_PARAM_MASK		(0xFF << 0)
#define PPS59_RC_RANGE_PARAM(_v)		(_v << 0)
#define PPS58_59_RC_RANGE_PARAM_MASK		(0xFFFF << 0)
#define PPS58_59_RC_RANGE_PARAM(_v)		(_v << 0)


/*
 * [ DSC SFR related info ]
 *  1) end SFR : DSC_PPS84_87(offset: 0x0074)
 *  2) all other PPSs except listed SFRs above are "fix" type
 *     : this means SFR is RW type but reset value is recommended
 *
 *      # Followings are re-guided values from VESA (about end of 2015)
 *        - PPS18_19 : initial_dec_delay
 *        - PPS22_23 : scale_increment_interval
 *        - PPS27    : first_line_bpg_offset
 *        - PPS28_29 : not_first_line_bpg_offset
 *        - PPS_78 ~ PPS_87 : rc_range_parameters_A~E
 */




#define SHADOW_OFFSET					0x7000

#endif /* _REGS_DECON_H */
