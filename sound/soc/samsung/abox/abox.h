/* sound/soc/samsung/abox/abox.h
 *
 * ALSA SoC - Samsung Abox driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_H
#define __SND_SOC_ABOX_H

#include <linux/wakelock.h>
#include <sound/samsung/abox.h>

#define ABOX_MASK(name) (GENMASK(ABOX_##name##_H, ABOX_##name##_L))
#define ABOX_MASK_ARG(name, x) (GENMASK(ABOX_##name##_H(x), ABOX_##name##_L(x)))
#define ABOX_IDX_ARG(name, o, x) (ABOX_##name##_BASE + \
		(x * ABOX_##name##_INTERVAL) + o)
#define ABOX_L_ARG(name, o, x) ABOX_IDX_ARG(name, o, x)
#define ABOX_H_ARG(name, o, x) ABOX_IDX_ARG(name, o, x)

/* System */
#define ABOX_IP_INDEX			(0x0000)
#define ABOX_VERSION			(0x0004)
#define ABOX_SYSPOWER_CTRL		(0x0010)
#define ABOX_SYSPOWER_STATUS		(0x0014)
#define ABOX_SYSTEM_CONFIG0		(0x0020)
#define ABOX_REMAP_MASK			(0x0024)
#define ABOX_REMAP_ADDR			(0x0028)
#define ABOX_DYN_CLOCK_OFF		(0x0030)
#define ABOX_ROUTE_CTRL0		(0x0040)
#define ABOX_ROUTE_CTRL1		(0x0044)
#define ABOX_ROUTE_CTRL2		(0x0048)
/* ABOX_SYSPOWER_CTRL */
#define ABOX_SYSPOWER_CTRL_L		(0)
#define ABOX_SYSPOWER_CTRL_H		(0)
#define ABOX_SYSPOWER_CTRL_MASK		ABOX_MASK(SYSPOWER_CTRL)
/* ABOX_SYSPOWER_STATUS */
#define ABOX_SYSPOWER_STATUS_L		(0)
#define ABOX_SYSPOWER_STATUS_H		(0)
#define ABOX_SYSPOWER_STATUS_MASK	ABOX_MASK(SYSPOWER_STATUS)
/* ABOX_DYN_CLOCK_OFF */
#define ABOX_DYN_CLOCK_OFF_L		(0)
#define ABOX_DYN_CLOCK_OFF_H		(30)
#define ABOX_DYN_CLOCK_OFF_MASK		ABOX_MASK(DYN_CLOCK_OFF)
/* ABOX_ROUTE_CTRL0 */
#define ABOX_ROUTE_DSIF_L		(20)
#define ABOX_ROUTE_DSIF_H		(23)
#define ABOX_ROUTE_DSIF_MASK		ABOX_MASK(ROUTE_DSIF)
#define ABOX_ROUTE_UAIF_SPK_BASE	(0)
#define ABOX_ROUTE_UAIF_SPK_INTERVAL	(4)
#define ABOX_ROUTE_UAIF_SPK_L(x)	ABOX_L_ARG(ROUTE_UAIF_SPK, 0, x)
#define ABOX_ROUTE_UAIF_SPK_H(x)	ABOX_H_ARG(ROUTE_UAIF_SPK, 3, x)
#define ABOX_ROUTE_UAIF_SPK_MASK(x)	ABOX_MASK_ARG(ROUTE_UAIF_SPK, x)
/* ABOX_ROUTE_CTRL1 */
#define ABOX_ROUTE_SPUSM_L		(16)
#define ABOX_ROUTE_SPUSM_H		(19)
#define ABOX_ROUTE_SPUSM_MASK		ABOX_MASK(ROUTE_SPUSM)
#define ABOX_ROUTE_NSRC_BASE		(0)
#define ABOX_ROUTE_NSRC_INTERVAL	(4)
#define ABOX_ROUTE_NSRC_L(x)		ABOX_L_ARG(ROUTE_NSRC, 0, x)
#define ABOX_ROUTE_NSRC_H(x)		ABOX_H_ARG(ROUTE_NSRC, 3, x)
#define ABOX_ROUTE_NSRC_MASK(x)		ABOX_MASK_ARG(ROUTE_NSRC, x)
/* ABOX_ROUTE_CTRL2 */
#define ABOX_ROUTE_RSRC_BASE		(0)
#define ABOX_ROUTE_RSRC_INTERVAL	(4)
#define ABOX_ROUTE_RSRC_L(x)		ABOX_L_ARG(ROUTE_RSRC, 0, x)
#define ABOX_ROUTE_RSRC_H(x)		ABOX_H_ARG(ROUTE_RSRC, 3, x)
#define ABOX_ROUTE_RSRC_MASK(x)		ABOX_MASK_ARG(ROUTE_RSRC, x)

/* SPUS */
#define ABOX_SPUS_CTRL0			(0x0200)
#define ABOX_SPUS_CTRL1			(0x0204)
#define ABOX_SPUS_CTRL2			(0x0208)
#define ABOX_SPUS_CTRL3			(0x020C)
/* ABOX_SPUS_CTRL0 */
#define ABOX_FUNC_CHAIN_SRC_BASE	(0)
#define ABOX_FUNC_CHAIN_SRC_INTERVAL	(4)
#define ABOX_FUNC_CHAIN_SRC_IN_L(x)	ABOX_L_ARG(FUNC_CHAIN_SRC, 3, x)
#define ABOX_FUNC_CHAIN_SRC_IN_H(x)	ABOX_H_ARG(FUNC_CHAIN_SRC, 3, x)
#define ABOX_FUNC_CHAIN_SRC_IN_MASK(x)	ABOX_MASK_ARG(FUNC_CHAIN_SRC_IN, x)
#define ABOX_FUNC_CHAIN_SRC_OUT_L(x)	ABOX_L_ARG(FUNC_CHAIN_SRC, 1, x)
#define ABOX_FUNC_CHAIN_SRC_OUT_H(x)	ABOX_H_ARG(FUNC_CHAIN_SRC, 2, x)
#define ABOX_FUNC_CHAIN_SRC_OUT_MASK(x)	ABOX_MASK_ARG(FUNC_CHAIN_SRC_OUT, x)
#define ABOX_FUNC_CHAIN_SRC_ASRC_L(x)	ABOX_L_ARG(FUNC_CHAIN_SRC, 0, x)
#define ABOX_FUNC_CHAIN_SRC_ASRC_H(x)	ABOX_H_ARG(FUNC_CHAIN_SRC, 0, x)
#define ABOX_FUNC_CHAIN_SRC_ASRC_MASK(x)	ABOX_MASK_ARG(\
		FUNC_CHAIN_SRC_ASRC, x)
/* ABOX_SPUS_CTRL1 */
#define ABOX_SIFM_IN_SEL_L		(22)
#define ABOX_SIFM_IN_SEL_H		(24)
#define ABOX_SIFM_IN_SEL_MASK		(ABOX_MASK(SIFM_IN_SEL))
#define ABOX_SIFS_OUT2_SEL_L		(19)
#define ABOX_SIFS_OUT2_SEL_H		(21)
#define ABOX_SIFS_OUT2_SEL_MASK		(ABOX_MASK(SIFS_OUT2_SEL))
#define ABOX_SIFS_OUT1_SEL_L		(16)
#define ABOX_SIFS_OUT1_SEL_H		(18)
#define ABOX_SIFS_OUT1_SEL_MASK		(ABOX_MASK(SIFS_OUT1_SEL))
#define ABOX_SPUS_MIXP_FORMAT_L		(0)
#define ABOX_SPUS_MIXP_FORMAT_H		(4)
#define ABOX_SPUS_MIXP_FORMAT_MASK	(ABOX_MASK(SPUS_MIXP_FORMAT))
/* ABOX_SPUS_CTRL2 */
#define ABOX_SPUS_MIXP_FLUSH_L		(0)
#define ABOX_SPUS_MIXP_FLUSH_H		(0)
#define ABOX_SPUS_MIXP_FLUSH_MASK	(ABOX_MASK(SPUS_MIXP_FLUSH))
/* ABOX_SPUS_CTRL3 */
#define ABOX_SPUS_SIFM_FLUSH_L		(2)
#define ABOX_SPUS_SIFM_FLUSH_H		(2)
#define ABOX_SPUS_SIFM_FLUSH_MASK	(ABOX_MASK(SPUS_SIFM_FLUSH))
#define ABOX_SPUS_SIFS2_FLUSH_L		(1)
#define ABOX_SPUS_SIFS2_FLUSH_H		(1)
#define ABOX_SPUS_SIFS2_FLUSH_MASK	(ABOX_MASK(SPUS_SIFS2_FLUSH))
#define ABOX_SPUS_SIFS1_FLUSH_L		(0)
#define ABOX_SPUS_SIFS1_FLUSH_H		(0)
#define ABOX_SPUS_SIFS1_FLUSH_MASK	(ABOX_MASK(SPUS_SIFS1_FLUSH))

/* SPUM */
#define ABOX_SPUM_CTRL0			(0x0300)
#define ABOX_SPUM_CTRL1			(0x0304)
#define ABOX_SPUM_CTRL2			(0x0308)
#define ABOX_SPUM_CTRL3			(0x030C)

/* ABOX_SPUM_CTRL0 */
#define ABOX_FUNC_CHAIN_NSRC_BASE	(4)
#define ABOX_FUNC_CHAIN_NSRC_INTERVAL	(4)
#define ABOX_FUNC_CHAIN_NSRC_OUT_L(x)	ABOX_L_ARG(FUNC_CHAIN_NSRC, 3, x)
#define ABOX_FUNC_CHAIN_NSRC_OUT_H(x)	ABOX_H_ARG(FUNC_CHAIN_NSRC, 3, x)
#define ABOX_FUNC_CHAIN_NSRC_OUT_MASK(x)	ABOX_MASK_ARG(\
		FUNC_CHAIN_NSRC_OUT, x)
#define ABOX_FUNC_CHAIN_NSRC_ASRC_L(x)	ABOX_L_ARG(FUNC_CHAIN_NSRC, 0, x)
#define ABOX_FUNC_CHAIN_NSRC_ASRC_H(x)	ABOX_H_ARG(FUNC_CHAIN_NSRC, 0, x)
#define ABOX_FUNC_CHAIN_NSRC_ASRC_MASK(x)	ABOX_MASK_ARG(\
		FUNC_CHAIN_NSRC_ASRC, x)
#define ABOX_FUNC_CHAIN_RSRC_RECP_L	(1)
#define ABOX_FUNC_CHAIN_RSRC_RECP_H	(1)
#define ABOX_FUNC_CHAIN_RSRC_RECP_MASK	ABOX_MASK(FUNC_CHAIN_RSRC_RECP)
#define ABOX_FUNC_CHAIN_RSRC_ASRC_L	(0)
#define ABOX_FUNC_CHAIN_RSRC_ASRC_H	(0)
#define ABOX_FUNC_CHAIN_RSRC_ASRC_MASK	ABOX_MASK(FUNC_CHAIN_RSRC_ASRC)
/* ABOX_SPUM_CTRL1 */
#define ABOX_SIFS_OUT_SEL_L		(16)
#define ABOX_SIFS_OUT_SEL_H		(18)
#define ABOX_SIFS_OUT_SEL_MASK		(ABOX_MASK(SIFS_OUT_SEL))
#define ABOX_SPUM_MIXP_FORMAT_L		(8)
#define ABOX_SPUM_MIXP_FORMAT_H		(12)
#define ABOX_SPUM_MIXP_FORMAT_MASK	(ABOX_MASK(SPUM_MIXP_FORMAT))
#define ABOX_RECP_SRC_VALID_L		(0)
#define ABOX_RECP_SRC_VALID_H		(1)
#define ABOX_RECP_SRC_VALID_MASK	(ABOX_MASK(RECP_SRC_VALID))
/* ABOX_SPUM_CTRL2 */
#define ABOX_SPUM_RECP_FLUSH_L		(0)
#define ABOX_SPUM_RECP_FLUSH_H		(0)
#define ABOX_SPUM_RECP_FLUSH_MASK	(ABOX_MASK(SPUM_RECP_FLUSH))
/* ABOX_SPUM_CTRL3 */
#define ABOX_SPUM_SIFM3_FLUSH_L		(3)
#define ABOX_SPUM_SIFM3_FLUSH_H		(3)
#define ABOX_SPUM_SIFM3_FLUSH_MASK	(ABOX_MASK(SPUM_SIFM3_FLUSH))
#define ABOX_SPUM_SIFM2_FLUSH_L		(2)
#define ABOX_SPUM_SIFM2_FLUSH_H		(2)
#define ABOX_SPUM_SIFM2_FLUSH_MASK	(ABOX_MASK(SPUM_SIFM2_FLUSH))
#define ABOX_SPUM_SIFM1_FLUSH_L		(1)
#define ABOX_SPUM_SIFM1_FLUSH_H		(1)
#define ABOX_SPUM_SIFM1_FLUSH_MASK	(ABOX_MASK(SPUM_SIFM1_FLUSH))
#define ABOX_SPUM_SIFM0_FLUSH_L		(0)
#define ABOX_SPUM_SIFM0_FLUSH_H		(0)
#define ABOX_SPUM_SIFM0_FLUSH_MASK	(ABOX_MASK(SPUM_SIFM0_FLUSH))


/* UAIF */
#define ABOX_UAIF_BASE			(0x0500)
#define ABOX_UAIF_INTERVAL		(0x0010)
#define ABOX_UAIF_CTRL0(x)		ABOX_IDX_ARG(UAIF, 0x0, x)
#define ABOX_UAIF_CTRL1(x)		ABOX_IDX_ARG(UAIF, 0x4, x)
#define ABOX_UAIF_STATUS(x)		ABOX_IDX_ARG(UAIF, 0xC, x)
/* ABOX_UAIF?_CTRL0 */
#define ABOX_START_FIFO_DIFF_MIC_L	(28)
#define ABOX_START_FIFO_DIFF_MIC_H	(31)
#define ABOX_START_FIFO_DIFF_MIC_MASK	(ABOX_MASK(START_FIFO_DIFF_MIC))
#define ABOX_START_FIFO_DIFF_SPK_L	(24)
#define ABOX_START_FIFO_DIFF_SPK_H	(27)
#define ABOX_START_FIFO_DIFF_SPK_MASK	(ABOX_MASK(START_FIFO_DIFF_SPK))
#define ABOX_MODE_L			(2)
#define ABOX_MODE_H			(2)
#define ABOX_MODE_MASK			(ABOX_MASK(MODE))
#define ABOX_MIC_ENABLE_L		(1)
#define ABOX_MIC_ENABLE_H		(1)
#define ABOX_MIC_ENABLE_MASK		(ABOX_MASK(MIC_ENABLE))
#define ABOX_SPK_ENABLE_L		(0)
#define ABOX_SPK_ENABLE_H		(0)
#define ABOX_SPK_ENABLE_MASK		(ABOX_MASK(SPK_ENABLE))
/* ABOX_UAIF?_CTRL1 */
#define ABOX_FORMAT_L			(24)
#define ABOX_FORMAT_H			(28)
#define ABOX_FORMAT_MASK		(ABOX_MASK(FORMAT))
#define ABOX_BCLK_POLARITY_L		(23)
#define ABOX_BCLK_POLARITY_H		(23)
#define ABOX_BCLK_POLARITY_MASK		(ABOX_MASK(BCLK_POLARITY))
#define ABOX_WS_MODE_L			(22)
#define ABOX_WS_MODE_H			(22)
#define ABOX_WS_MODE_MASK		(ABOX_MASK(WS_MODE))
#define ABOX_WS_POLAR_L			(21)
#define ABOX_WS_POLAR_H			(21)
#define ABOX_WS_POLAR_MASK		(ABOX_MASK(WS_POLAR))
#define ABOX_SLOT_MAX_L			(18)
#define ABOX_SLOT_MAX_H			(20)
#define ABOX_SLOT_MAX_MASK		(ABOX_MASK(SLOT_MAX))
#define ABOX_SBIT_MAX_L			(12)
#define ABOX_SBIT_MAX_H			(17)
#define ABOX_SBIT_MAX_MASK		(ABOX_MASK(SBIT_MAX))
#define ABOX_VALID_STR_L		(6)
#define ABOX_VALID_STR_H		(11)
#define ABOX_VALID_STR_MASK		(ABOX_MASK(VALID_STR))
#define ABOX_VALID_END_L		(0)
#define ABOX_VALID_END_H		(5)
#define ABOX_VALID_END_MASK		(ABOX_MASK(VALID_END))
/* ABOX_UAIF?_STATUS */
#define ABOX_ERROR_OF_MIC_L		(1)
#define ABOX_ERROR_OF_MIC_H		(1)
#define ABOX_ERROR_OF_MIC_MASK		(ABOX_MASK(ERROR_OF_MIC))
#define ABOX_ERROR_OF_SPK_L		(0)
#define ABOX_ERROR_OF_SPK_H		(0)
#define ABOX_ERROR_OF_SPK_MASK		(ABOX_MASK(ERROR_OF_SPK))

/* DSIF */
#define ABOX_DSIF_CTRL			(0x0550)
#define ABOX_DSIF_STATUS		(0x0554)
/* ABOX_DSIF_CTRL */
#define ABOX_DSIF_BCLK_POLARITY_L	(2)
#define ABOX_DSIF_BCLK_POLARITY_H	(2)
#define ABOX_DSIF_BCLK_POLARITY_MASK	(ABOX_MASK(DSIF_BCLK_POLARITY))
#define ABOX_ORDER_L			(1)
#define ABOX_ORDER_H			(1)
#define ABOX_ORDER_MASK			(ABOX_MASK(ORDER))
#define ABOX_ENABLE_L			(0)
#define ABOX_ENABLE_H			(0)
#define ABOX_ENABLE_MASK		(ABOX_MASK(ENABLE))
/* ABOX_DSIF_STATUS */
#define ABOX_ERROR_L			(0)
#define ABOX_ERROR_H			(0)
#define ABOX_ERROR_MASK			(ABOX_MASK(ERROR))

/* RDMA */
#define ABOX_RDMA_BASE			(0x1000)
#define ABOX_RDMA_INTERVAL		(0x0100)
#define ABOX_RDMA_CTRL0			(0x00)
#define ABOX_RDMA_CTRL1			(0x04)
#define ABOX_RDMA_BUF_STR		(0x08)
#define ABOX_RDMA_BUF_END		(0x0C)
#define ABOX_RDMA_BUF_OFFSET		(0x10)
#define ABOX_RDMA_STR_POINT		(0x14)
#define ABOX_RDMA_VOL_FACTOR		(0x18)
#define ABOX_RDMA_VOL_CHANGE		(0x1C)
#define ABOX_RDMA_STATUS		(0x20)
/* ABOX_RDMA_CTRL0 */
#define ABOX_RDMA_ENABLE_L		(0)
#define ABOX_RDMA_ENABLE_H		(0)
#define ABOX_RDMA_ENABLE_MASK		(ABOX_MASK(RDMA_ENABLE))
/* ABOX_RDMA_STATUS */
#define ABOX_RDMA_PROGRESS_L		(31)
#define ABOX_RDMA_PROGRESS_H		(31)
#define ABOX_RDMA_PROGRESS_MASK		(ABOX_MASK(RDMA_PROGRESS))
#define ABOX_RDMA_RBUF_OFFSET_L		(16)
#define ABOX_RDMA_RBUF_OFFSET_H		(28)
#define ABOX_RDMA_RBUF_OFFSET_MASK	(ABOX_MASK(RDMA_RBUF_OFFSET))
#define ABOX_RDMA_RBUF_CNT_L		(0)
#define ABOX_RDMA_RBUF_CNT_H		(12)
#define ABOX_RDMA_RBUF_CNT_MASK		(ABOX_MASK(RDMA_RBUF_CNT))

/* WDMA */
#define ABOX_WDMA_BASE			(0x2000)
#define ABOX_WDMA_INTERVAL		(0x0100)
#define ABOX_WDMA_CTRL			(0x00)
#define ABOX_WDMA_BUF_STR		(0x08)
#define ABOX_WDMA_BUF_END		(0x0C)
#define ABOX_WDMA_BUF_OFFSET		(0x10)
#define ABOX_WDMA_STR_POINT		(0x14)
#define ABOX_WDMA_VOL_FACTOR		(0x18)
#define ABOX_WDMA_VOL_CHANGE		(0x1C)
#define ABOX_WDMA_STATUS		(0x20)
/* ABOX_WDMA_CTRL */
#define ABOX_WDMA_ENABLE_L		(0)
#define ABOX_WDMA_ENABLE_H		(0)
#define ABOX_WDMA_ENABLE_MASK		(ABOX_MASK(WDMA_ENABLE))
/* ABOX_WDMA_STATUS */
#define ABOX_WDMA_PROGRESS_L		(31)
#define ABOX_WDMA_PROGRESS_H		(31)
#define ABOX_WDMA_PROGRESS_MASK		(ABOX_MASK(WDMA_PROGRESS))
#define ABOX_WDMA_RBUF_OFFSET_L		(16)
#define ABOX_WDMA_RBUF_OFFSET_H		(28)
#define ABOX_WDMA_RBUF_OFFSET_MASK	(ABOX_MASK(WDMA_RBUF_OFFSET))
#define ABOX_WDMA_RBUF_CNT_L		(0)
#define ABOX_WDMA_RBUF_CNT_H		(12)
#define ABOX_WDMA_RBUF_CNT_MASK		(ABOX_MASK(WDMA_RBUF_CNT))

/* CA7 */
#define ABOX_CA7_R(x)			(0x2C00 + (x * 0x4))
#define ABOX_CA7_PC			(0x2C3C)
#define ABOX_CA7_L2C_STATUS		(0x2C40)

#define ABOX_MAX_REGISTERS		(0x2D0C)

/* SYSREG */
#define ABOX_SYSREG_L2_CACHE_CON	(0x0328)
#define ABOX_SYSREG_MISC_CON		(0x032C)

#define BUFFER_BYTES_MAX		(SZ_128K)
#define PERIOD_BYTES_MIN		(SZ_128)
#define PERIOD_BYTES_MAX		(BUFFER_BYTES_MAX / 2)

#define DRAM_FIRMWARE_SIZE		(SZ_8M + SZ_4M)
#define IOVA_DRAM_FIRMWARE		(0x80000000)
#define IOVA_RDMA_BUFFER_BASE		(0x81000000)
#define IOVA_RDMA_BUFFER(x)		(IOVA_RDMA_BUFFER_BASE + (SZ_1M * x))
#define IOVA_WDMA_BUFFER_BASE		(0x82000000)
#define IOVA_WDMA_BUFFER(x)		(IOVA_WDMA_BUFFER_BASE + (SZ_1M * x))
#define IOVA_COMPR_BUFFER_BASE		(0x83000000)
#define IOVA_COMPR_BUFFER(x)		(IOVA_COMPR_BUFFER_BASE + (SZ_1M * x))
#define IVA_FIRMWARE_SIZE		(SZ_512K)
#define IOVA_IVA_FIRMWARE		(0x90000000)
#define IOVA_IVA_BASE			(IOVA_IVA_FIRMWARE)
#define IOVA_IVA(x)			(IOVA_IVA_BASE + (SZ_16M * x))
#define IOVA_VSS_FIRMWARE		(0xA0000000)
#define IOVA_DUMP_BUFFER		(0xD0000000)
#define PHSY_VSS_FIRMWARE		(0xFEE00000)
#define PHSY_VSS_SIZE			(SZ_4M + SZ_2M)

#define LIMIT_IN_JIFFIES		(msecs_to_jiffies(1000))

#define ABOX_CPU_GEAR_CALL_VSS		(0xCA11)
#define ABOX_CPU_GEAR_CALL_KERNEL	(0xCA12)
#define ABOX_CPU_GEAR_CALL		ABOX_CPU_GEAR_CALL_VSS
#define ABOX_CPU_GEAR_BOOT		(0xB00D)
#define ABOX_CPU_GEAR_LOWER_LIMIT	(12)

#define ABOX_DMA_TIMEOUT_US		(40000)

#define ABOX_SAMPLING_RATES (SNDRV_PCM_RATE_KNOT)
#define ABOX_SAMPLE_FORMATS (SNDRV_PCM_FMTBIT_S16\
		| SNDRV_PCM_FMTBIT_S24\
		| SNDRV_PCM_FMTBIT_S32)
#define ABOX_WDMA_SAMPLE_FORMATS (SNDRV_PCM_FMTBIT_S16\
		| SNDRV_PCM_FMTBIT_S24)

#define set_mask_value(id, mask, value) \
		(id = (typeof(id))((id & ~mask) | (value & mask)))

#define ABOX_SUPPLEMENT_SIZE (SZ_128)
#define ABOX_IPC_QUEUE_SIZE (SZ_8)

#define CALLIOPE_VERSION(class, year, month, minor) \
		((class << 24) | \
		((year - 1 + 'A') << 16) | \
		((month - 1 + 'A') << 8) | \
		((minor + '0') << 0))

enum abox_sram {
	ABOX_IVA_MEMORY,
	ABOX_IVA_MEMORY_PREPARE,
};

enum abox_dai {
	ABOX_UAIF0,
	ABOX_UAIF1,
	ABOX_UAIF2,
	ABOX_UAIF3,
	ABOX_UAIF4,
	ABOX_DSIF,
	ABOX_RDMA0,
	ABOX_RDMA1,
	ABOX_RDMA2,
	ABOX_RDMA3,
	ABOX_RDMA4,
	ABOX_RDMA5,
	ABOX_RDMA6,
	ABOX_RDMA7,
	ABOX_WDMA0,
	ABOX_WDMA1,
	ABOX_WDMA2,
	ABOX_WDMA3,
	ABOX_WDMA4,
};

#define ABOX_DAI_COUNT (ABOX_DSIF + 1)

enum ipc_state {
	IDLE,
	SEND_MSG,
	SEND_MSG_OK,
	SEND_MSG_FAIL,
};

enum calliope_state {
	CALLIOPE_DISABLED,
	CALLIOPE_DISABLING,
	CALLIOPE_ENABLING,
	CALLIOPE_ENABLED,
	CALLIOPE_STATE_COUNT,
};

enum audio_mode {
	MODE_NORMAL,
	MODE_RINGTONE,
	MODE_IN_CALL,
	MODE_IN_COMMUNICATION,
	MODE_IN_VIDEOCALL,
};

#define ABOX_QUIRK_BIT_TRY_TO_ASRC_OFF (1 << 0)
#define ABOX_QUIRK_STR_TRY_TO_ASRC_OFF "try to asrc off"

/**
 * parse and return quirk bitmap
 * @param[in]	np		pointer to device node
 * @return	quirk bitmap
 */
static inline unsigned long abox_probe_quirks(struct device_node *np)
{
	#define QUIRKS "quirks"
	#define DEC_MAP(id) {ABOX_QUIRK_STR_##id, ABOX_QUIRK_BIT_##id}

	static const struct {
		const char *str;
		unsigned int bit;
	} map[] = {
		DEC_MAP(TRY_TO_ASRC_OFF),
	};

	int i, ret;
	unsigned long quirks = 0;

	while (np) {
		if (!of_find_property(np, QUIRKS, NULL)) {
			/* fallback to parent */
			np = np->parent;
			continue;
		}
		for (i = 0; i < ARRAY_SIZE(map); i++) {
			ret = of_property_match_string(np, QUIRKS, map[i].str);
			if (!IS_ERR_VALUE(ret))
				quirks |= map[i].bit;
		}
		break;
	}

	return quirks;
}

/**
 * test quirk bitmap
 * @param[in]	quirks		quirk bitmap
 * @param[in]	quirk		quirk to test
 * @return	true if quirk is in quirks, false on otherwise.
 */
static inline bool abox_test_quirk(unsigned long quirks, unsigned long quirk)
{
	return !!(quirks & quirk);
}

struct abox_ipc {
	struct device *dev;
	int hw_irq;
	unsigned char supplement[ABOX_SUPPLEMENT_SIZE];
	size_t size;
};

struct abox_irq_action {
	struct list_head list;
	int irq;
	abox_irq_handler_t irq_handler;
	void *dev_id;
};

struct abox_qos_request {
	void *id;
	unsigned int value;
};

struct abox_dram_request {
	void *id;
	bool on;
};

struct abox_l2c_request {
	void *id;
	bool on;
};

struct abox_extra_firmware {
	const struct firmware *firmware;
	const char *name;
	u32 area;
	u32 offset;
};

struct abox_component {
	struct ABOX_COMPONENT_DESCRIPTIOR *desc;
	bool registered;
};

struct abox_component_kcontrol_value {
	struct ABOX_COMPONENT_DESCRIPTIOR *desc;
	struct ABOX_COMPONENT_CONTROL *control;
};

struct abox_data {
	struct platform_device *pdev;
	struct snd_soc_codec *codec;
	struct regmap *regmap;
	void __iomem *sfr_base;
	void __iomem *sysreg_base;
	void __iomem *sram_base;
	phys_addr_t sram_base_phys;
	size_t sram_size;
	void *dram_base;
	dma_addr_t dram_base_phys;
	void *iva_base;
	dma_addr_t iva_base_phys;
	void *dump_base;
	phys_addr_t dump_base_phys;
	struct iommu_domain *iommu_domain;
	unsigned int ipc_tx_offset;
	unsigned int ipc_rx_offset;
	unsigned int ipc_tx_ack_offset;
	unsigned int ipc_rx_ack_offset;
	unsigned int mailbox_offset;
	unsigned int rdma_count;
	unsigned int wdma_count;
	unsigned int calliope_version;
	const struct firmware *firmware_sram;
	const struct firmware *firmware_dram;
	const struct firmware *firmware_iva;
	struct abox_extra_firmware firmware_extra[8];
	struct platform_device *pdev_gic;
	struct platform_device *pdev_rdma[8];
	struct platform_device *pdev_wdma[5];
	struct platform_device *pdev_vts;
	enum ipc_state ipc_state;
	struct work_struct ipc_work;
	struct abox_ipc ipc_queue[ABOX_IPC_QUEUE_SIZE];
	unsigned int ipc_queue_start;
	unsigned int ipc_queue_end;
	wait_queue_head_t ipc_wait_queue;
	spinlock_t ipc_spinlock;
	struct mutex ipc_mutex;
	struct clk *clk_pll;
	struct clk *clk_audif;
	struct clk *clk_ca7;
	struct clk *clk_uaif;
	struct clk *clk_bclk[ABOX_DAI_COUNT];
	struct clk *clk_bclk_gate[ABOX_DAI_COUNT];
	struct clk *clk_dmic;
	struct pinctrl *pinctrl;
	unsigned int clk_ca7_gear;
	struct abox_qos_request ca7_gear_requests[32];
	struct work_struct change_cpu_gear_work;
	unsigned int mif_freq;
	struct work_struct change_mif_freq_work;
	unsigned int lit_freq;
	struct abox_qos_request lit_requests[16];
	struct work_struct change_lit_freq_work;
	unsigned int big_freq;
	struct abox_qos_request big_requests[16];
	struct work_struct change_big_freq_work;
	unsigned int hmp_boost;
	struct abox_qos_request hmp_requests[16];
	struct work_struct change_hmp_boost_work;
	struct abox_dram_request dram_requests[16];
	unsigned int uaif_fmt[ABOX_DAI_COUNT];
	unsigned long audif_rates[ABOX_DAI_COUNT];
	unsigned int out_rate[SET_INMUX4_SAMPLE_RATE + 1];
	bool out_rate_auto[SET_INMUX4_SAMPLE_RATE + 1];
	unsigned char rdma_synchronizer[8];
	unsigned int erap_status[ERAP_TYPE_COUNT];
	struct work_struct register_component_work;
	struct abox_component components[16];
	struct list_head irq_actions;
	bool enabled;
	enum calliope_state calliope_state;
	bool l2c_controlled;
	bool l2c_enabled;
	struct abox_l2c_request l2c_requests[8];
	struct work_struct l2c_work;
	struct notifier_block pm_nb;
	struct notifier_block modem_nb;
	struct notifier_block itmon_nb;
	int pm_qos_int[5];
	struct ima_client *ima_client;
	void *ima_vaddr;
	bool ima_claimed;
	struct mutex ima_lock;
	struct work_struct boot_done_work;
	struct wake_lock wake_lock;
	enum audio_mode audio_mode;
};

struct abox_compr_data {
	/* compress offload */
	struct snd_compr_stream *cstream;

	void *dma_area;
	size_t dma_size;
	dma_addr_t dma_addr;

	unsigned int block_num;
	unsigned int handle_id;
	unsigned int codec_id;
	unsigned int channels;
	unsigned int sample_rate;

	unsigned int byte_offset;
	u64 copied_total;
	u64 received_total;

	bool start;
	bool eos;
	bool created;

	bool effect_on;

	wait_queue_head_t flush_wait;
	wait_queue_head_t exit_wait;
	wait_queue_head_t ipc_wait;

	uint32_t stop_ack;
	uint32_t exit_ack;

	spinlock_t lock;
	spinlock_t cmd_lock;

	int (*isr_handler)(void *data);

	struct snd_compr_params codec_param;

	/* effect offload */
	unsigned int out_sample_rate;
};

enum abox_platform_type {
	PLATFORM_NORMAL,
	PLATFORM_CALL,
	PLATFORM_COMPRESS,
	PLATFORM_REALTIME,
	PLATFORM_VI_SENSING,
	PLATFORM_SYNC,
};

enum abox_rate {
	RATE_SUHQA,
	RATE_UHQA,
	RATE_NORMAL,
	RATE_COUNT,
};

/**
 * Get sampling rate type
 * @param[in]	rate		sampling rate in Hz
 * @return	rate type in enum abox_rate
 */
static inline enum abox_rate abox_get_rate_type(unsigned int rate)
{
	if (rate < 176400)
		return RATE_NORMAL;
	else if (rate >= 176400 && rate <= 192000)
		return RATE_UHQA;
	else
		return RATE_SUHQA;
}

struct abox_platform_data {
	void __iomem *sfr_base;
	unsigned int id;
	unsigned int pointer;
	unsigned long quirks;
	int pm_qos_lit[RATE_COUNT];
	int pm_qos_big[RATE_COUNT];
	int pm_qos_hmp[RATE_COUNT];
	struct platform_device *pdev_abox;
	struct abox_data *abox_data;
	struct snd_pcm_substream *substream;
	enum abox_platform_type type;
	struct abox_compr_data compr_data;
};

/**
 * get pointer to abox_data (internal use only)
 * @return	pointer to abox_data
 */
extern struct abox_data *abox_get_abox_data(void);

/**
 * Read from mailbox
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	index		index from mailbox start
 * @return	data which is read from mailbox
 */
extern u32 abox_mailbox_read(struct device *dev, struct abox_data *data,
		unsigned int index);

/**
 * Wrtie to mailbox
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	index		index from mailbox start
 * @param[in]	value		data which will be written to mailbox
 */
extern void abox_mailbox_write(struct device *dev, struct abox_data *data,
		u32 index, u32 value);

/**
 * Schedule abox IPC
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	hw_irq		hardware IRQ number
 * @param[in]	supplement	pointer to data
 * @param[in]	size		size of data which are pointed by supplement
 * @return	error code if any
 */
extern int abox_schedule_ipc(struct device *dev, struct abox_data *data,
		int hw_irq, const void *supplement, size_t size);

/**
 * check specific cpu gear request is idle
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @return	true if it is idle or not has been requested, false on otherwise
 */
extern bool abox_cpu_gear_idle(struct device *dev, struct abox_data *data,
		void *id);

/**
 * Request abox cpu clock level
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @param[in]	gear		gear level (cpu clock = aud pll rate / gear)
 * @return	error code if any
 */
extern int abox_request_cpu_gear(struct device *dev, struct abox_data *data,
		void *id, unsigned int gear);

/**
 * Request abox cpu clock level synchronously
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @param[in]	gear		gear level (cpu clock = aud pll rate / gear)
 * @return	error code if any
 */
extern int abox_request_cpu_gear_sync(struct device *dev,
		struct abox_data *data, void *id, unsigned int gear);

/**
 * Request LITTLE cluster clock level
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @param[in]	freq		frequency in kHz
 * @return	error code if any
 */
extern int abox_request_lit_freq(struct device *dev, struct abox_data *data,
		void *id, unsigned int freq);

/**
 * Request big cluster clock level
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @param[in]	freq		frequency in kHz
 * @return	error code if any
 */
extern int abox_request_big_freq(struct device *dev, struct abox_data *data,
		void *id, unsigned int freq);

/**
 * Request hmp boost
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @param[in]	on		1 on boost, 0 on otherwise.
 * @return	error code if any
 */
extern int abox_request_hmp_boost(struct device *dev, struct abox_data *data,
		void *id, unsigned int on);

/**
 * Try to turn off ASRC when sampling rate auto control is enabled
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	dai		snd_soc_dai pointer which invokes this API
 * @return	error code if any
 */
extern int abox_try_to_asrc_off(struct device *dev, struct abox_data *data,
		struct snd_soc_dai *dai);

/**
 * Register rdma to abox
 * @param[in]	pdev_abox	pointer to abox platform device
 * @param[in]	pdev_rdma	pointer to abox rdma platform device
 * @param[in]	id		number
 * @return	error code if any
 */
extern void abox_register_rdma(struct platform_device *pdev_abox,
		struct platform_device *pdev_rdma, unsigned int id);

/**
 * Register wdma to abox
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	ipcid		id of ipc
 * @param[in]	irq_handler	irq handler to register
 * @param[in]	dev_id		cookie which would be summitted with irq_handler
 * @return	error code if any
 */
extern void abox_register_wdma(struct platform_device *pdev_abox,
		struct platform_device *pdev_wdma, unsigned int id);

/**
 * Request or release l2 cache
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @param[in]	on		true for requesting, false on otherwise
 * @return	error code if any
 */
extern int abox_request_l2c(struct device *dev, struct abox_data *data,
		void *id, bool on);

/**
 * Request or release l2 cache synchronously
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @param[in]	on		true for requesting, false on otherwise
 * @return	error code if any
 */
extern int abox_request_l2c_sync(struct device *dev, struct abox_data *data,
		void *id, bool on);

/**
 * Request or release dram during cpuidle (count based API)
 * @param[in]	pdev_abox	pointer to abox platform device
 * @param[in]	id		key which is used as unique handle
 * @param[in]	on		true for requesting, false on otherwise
 */
extern void abox_request_dram_on(struct platform_device *pdev_abox, void *id,
		bool on);

/**
 * claim IVA memory
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[out]	addr		optional argument to get physical address
 */
extern int abox_ima_claim(struct device *dev, struct abox_data *data,
		phys_addr_t *addr);
#endif /* __SND_SOC_ABOX_H */
