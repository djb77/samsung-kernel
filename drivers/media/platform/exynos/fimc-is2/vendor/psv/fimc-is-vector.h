/*
 * Samsung Exynos SoC series FIMC-IS driver
 *
 * Exynos fimc-is PSV vector
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_VECTOR_H
#define FIMC_IS_VECTOR_H

#include "fimc-is-core.h"
#include "fimc-is-vender-config.h"

/* configurations */
#undef INPUT_CHKSUM

#define NUM_OF_COL_CONFIG	1
#define NUM_OF_COL_DMA_CFG	4	/* ofs size dir filenam */
#define DMA_DIR_OUTPUT		0
#define DMA_DIR_INPUT		1
#define DMA_DIR_IGNORE		2
#define NUM_OF_COL_CRC_CFG	3	/* addr value mask */
#define NUM_OF_COL_SFR_CFG	2	/* addr value */
#define MIN_LEN_OF_SFR_CFG	17	/* XXXXXXXX XXXXXXXX */

#define VERIFICATION_CRC	0x1
#define VERIFICATION_CHKSUM	0x2

#define SIZE_OF_NAME	128

#define vector_writel(val, base, ofs) \
	__raw_writel(val, base + (ofs - IO_MEM_VECTOR_OFS))
#define vector_readl(base, ofs)	\
	__raw_readl(base + (ofs - IO_MEM_VECTOR_OFS))

struct vector_dma {
	struct list_head list;
	unsigned int sfr_ofs;
	unsigned int size;
	unsigned int dir;
	char filename[SIZE_OF_NAME];

#ifdef USE_ION_DIRECTLY
	struct ion_handle ion_hdl;
	unsigned long dva;
	void *kva;
#else
	struct fimc_is_priv_buf *pbuf;
#endif

#ifdef INPUT_CHKSUM
	ulong chksum_input;
#endif
	ulong chksum_expect;
	ulong chksum_result;
};

struct vector_crc {
	struct list_head list;
	unsigned int sfr_addr;
	unsigned int value;
	unsigned int sfr_mask;
};

struct vector_time {
	unsigned long long start_time;
	unsigned long long end_time;
};

#define MAX_NUMERIC_CFGS	32
#define NUM_OF_NUMERIC_CFGS	8
#define OFS_OF_NUMERIC_CFGS	1 /* name */
static const char * const cfg_name[] = {
	"name",
	"frame number",
	"verification",
	"DMA dump",
	"sync. mode",
	"execution timeout",
	"compressed DMA",
	"no fimc-is",
	"no fimc-is baseaddr",
	NULL,
};

struct vector_cfg {
	char name[SIZE_OF_NAME];
	union {
		struct {
			unsigned int framenum;		/* requested frame count */
			unsigned int verification;	/* 1: CRC, 2: checksum */
			unsigned int dump_dma;		/* dump output DMAs to files */
			unsigned int sync;		/* 0: Async, 1: Sync */
			unsigned int timeout;		/* execution timeout(ms) */
			unsigned int compressed;	/* compressed DMA files */
			unsigned int no_fimcis;		/* no fimc-is block */
			unsigned int no_fimcis_baseaddr;	/* no fimc-is block baseaddress */
		} item;
		unsigned int items[MAX_NUMERIC_CFGS];
	};

#ifdef USE_ION_DIRECTLY
	struct ion_client *client;
#endif
	void __iomem *baseaddr;
	unsigned int framecnt;	/* current frame count */
	struct list_head dma;
	struct list_head crc;

	/* done */
	atomic_t done_3aa0;
	atomic_t done_3aa1;
	atomic_t done_isp0;
	atomic_t done_isp1;
	atomic_t done_mcsc0;
	atomic_t done_mcsc1;
	atomic_t done_vra0;
	atomic_t done_vra1;
	atomic_t done_dcp;
	atomic_t done_srdz;

	/* irqs */
	int irq_3aa0;
	int irq_3aa1;
	int irq_isp0;
	int irq_isp1;
	int irq_mcsc0;
	int irq_mcsc1;
	int irq_vra0;
	int irq_vra1;
	int irq_dcp;
	int irq_srdz;

	wait_queue_head_t wait;

	int measure_time_enable;
	struct vector_time time[MAX_PDEV_IRQ_NUM];

	int async_id;
};

int fimc_is_vector_set(struct fimc_is_core *core, int id);
int fimc_is_vector_get(struct fimc_is_core *core);

#endif
