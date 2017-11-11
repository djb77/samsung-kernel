/* arch/arm/mach-exynos/cal_bts.h
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * EXYNOS - BTS CAL code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BTSCAL_H__
#define __BTSCAL_H__

#include <linux/io.h>
#include <linux/debugfs.h>

/* for BTS Through SYSTEM Register */
#define EXYNOS8895_PA_CP			0x14A80000
#define EXYNOS8895_PA_DPU0			0x15010000
#define EXYNOS8895_PA_DPU1			0x15020000
#define EXYNOS8895_PA_DPU2			0x15030000
#define EXYNOS8895_PA_CAM0			0x15040000
#define EXYNOS8895_PA_CAM1			0x15050000
#define EXYNOS8895_PA_ISPLP			0x15060000
#define EXYNOS8895_PA_SRDZ			0x15070000
#define EXYNOS8895_PA_IVA			0x15080000
#define EXYNOS8895_PA_DSP			0x15090000
#define EXYNOS8895_PA_VPU			0x150a0000
#define EXYNOS8895_PA_MFC0			0x150b0000
#define EXYNOS8895_PA_MFC1			0x150c0000
#define EXYNOS8895_PA_G2D0			0x150d0000
#define EXYNOS8895_PA_G2D1			0x150e0000
#define EXYNOS8895_PA_G2D2			0x150f0000
#define EXYNOS8895_PA_FSYS0			0x15100000
#define EXYNOS8895_PA_PDMA			0x15120000
#define EXYNOS8895_PA_SPDMA			0x15130000
#define EXYNOS8895_PA_ABOX			0x15140000
#define EXYNOS8895_PA_VTS			0x15150000
#define EXYNOS8895_PA_FSYS1			0x15400000
#define EXYNOS8895_PA_GNSS			0x15410000
#define EXYNOS8895_PA_ALIVE			0x15420000
#define EXYNOS8895_PA_G3D0			0x14A40000
#define EXYNOS8895_PA_G3D1			0x14A50000
#define EXYNOS8895_PA_G3D2			0x14A60000
#define EXYNOS8895_PA_G3D3			0x14A70000

#define EXYNOS8895_PA_ACEMUX_SYSREG_CORE	0x15832800
#define EXYNOS8895_PA_SCI			0x15800000
#define EXYNOS8895_PA_SCI_IRPS0			0x14a94000
#define EXYNOS8895_PA_SCI_IRPS1			0x14aa4000
#define EXYNOS8895_PA_SMC0			0x16030000
#define EXYNOS8895_PA_SMC1			0x16130000
#define EXYNOS8895_PA_SMC2			0x16230000
#define EXYNOS8895_PA_SMC3			0x16330000

#define EXYNOS8895_PA_SN_IRPS0			0x14A92000
#define EXYNOS8895_PA_SN_IRPS1			0x14AA2000
#define EXYNOS8895_PA_SN_BUSC_M0		0x15162000
#define EXYNOS8895_PA_SN_BUSC_M1		0x15172000
#define EXYNOS8895_PA_SN_BUSC_M2		0x15182000
#define EXYNOS8895_PA_SN_BUSC_M3		0x15192000

#define BTS_MAX_MO				0xffff

struct bts_status {
	bool scen_en;
	unsigned int priority;
	bool disable;
	bool bypass_en;
	bool timeout_en;
	unsigned int rmo;
	unsigned int wmo;
	unsigned int full_rmo;
	unsigned int full_wmo;
	unsigned int busy_rmo;
	unsigned int busy_wmo;
	unsigned int max_rmo;
	unsigned int max_wmo;
	unsigned int timeout_r;
	unsigned int timeout_w;
};

void bts_setqos(void __iomem *base, struct bts_status *stat);
void bts_showqos(void __iomem *base, struct seq_file *buf);
void bts_trex_init(void __iomem *base);
void bts_set_qmax(void __iomem *base, unsigned int rmo, unsigned int wmo);
void bts_set_tq(void __iomem *base, unsigned int tq);
void bts_set_qbusy(void __iomem *base, unsigned int qbusy);
void bts_set_qfull(void __iomem *base, unsigned int qfull_low,
		   unsigned int qfull_high);


#endif
