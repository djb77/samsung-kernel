/*
 * Samsung Exynos SoC series FIMC-IS driver
 *
 * Exynos fimc-is PSV vender configuration
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_VENDER_CONFIG_H
#define FIMC_IS_VENDER_CONFIG_H

/* configurations */
#define USE_IOMMU
#undef USE_ION_DIRECTLY

/* version dependent */

/* 8895 */
#if defined(CONFIG_SOC_EXYNOS8895)
#define IO_MEM_BASE		0x12000000
#define IO_MEM_SIZE		0x02400000
#define IO_MEM_VECTOR_OFS	0x12000000

#define SYSMMU_CAM0_OFS		0x00D10000
#define SYSMMU_CAM1_OFS		0x00D20000
#define SYSMMU_ISP_OFS		0x01050000
#define SYSMMU_SRDZ_OFS		0x02230000

#define PDEV_IRQ_NUM_3AAW_0	0
#define PDEV_IRQ_NUM_3AAW_1	1
#define PDEV_IRQ_NUM_3AA_0	2
#define PDEV_IRQ_NUM_3AA_1	3
#define PDEV_IRQ_NUM_ISPLP_0	4
#define PDEV_IRQ_NUM_ISPLP_1	5
#define PDEV_IRQ_NUM_ISPHQ_0	6
#define PDEV_IRQ_NUM_ISPHQ_1	7
#define PDEV_IRQ_NUM_TPU0_0	8
#define PDEV_IRQ_NUM_TPU0_1	9
#define PDEV_IRQ_NUM_TPU1_0	10
#define PDEV_IRQ_NUM_TPU1_1	11
#define PDEV_IRQ_NUM_MCSC_0	12
#define PDEV_IRQ_NUM_MCSC_1	13
#define PDEV_IRQ_NUM_VRA_0	14
#define PDEV_IRQ_NUM_VRA_1	15
#define PDEV_IRQ_NUM_DCP_0	16
#define PDEV_IRQ_NUM_DCP_1	17
#define PDEV_IRQ_NUM_SRDZ	18

#define MAX_PDEV_IRQ_NUM	19

/* 7880 */
#elif defined(CONFIG_SOC_EXYNOS7880)
#define IO_MEM_BASE		0x14400000
#define IO_MEM_SIZE		0x00100000
#define IO_MEM_VECTOR_OFS	0x00400000

/* only for ver 5.10, should be flexible */
#define SYSMMU_ISP0_OFS		0x00070000	/* CSIS/BNA/3AA */
#define SYSMMU_ISP1_OFS		0x00090000	/* ISP/MCSC */
#define SYSMMU_ISP2_OFS		0x000A0000	/* VRA */

#define PDEV_IRQ_NUM_3AA0	0
#define PDEV_IRQ_NUM_3AA1	1
#define PDEV_IRQ_NUM_ISP0	2
#define PDEV_IRQ_NUM_ISP1	3
#define PDEV_IRQ_NUM_MCSC	4
#define PDEV_IRQ_NUM_VRA0	5
#define PDEV_IRQ_NUM_VRA1	6

#define MAX_PDEV_IRQ_NUM	7

/* 7570 */
#elif defined(CONFIG_SOC_EXYNOS7570)
#define IO_MEM_BASE		0x14400000
#define IO_MEM_SIZE		0x00100000
#define IO_MEM_VECTOR_OFS	0x00400000
#define SYSMMU_ISP0_OFS		0x00070000	/* FIMC */

/* need to match fimc_is interrupts number in 7570.dtsi */
#define PDEV_IRQ_NUM_ISP0	0
#define PDEV_IRQ_NUM_ISP1	1
#define PDEV_IRQ_NUM_ISP2	2
#define PDEV_IRQ_NUM_MCSC	3
#define PDEV_IRQ_NUM_VRA1	4
#define PDEV_IRQ_NUM_MFCMCSC	5

#define MAX_PDEV_IRQ_NUM	6
#endif

#endif
