/*
 * Exynos-SnapShot for Samsung's SoC's.
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef EXYNOS_SNAPSHOT_TABLE_H
#define EXYNOS_SNAPSHOT_TABLE_H

/************************************************************************
 * This definition is default settings.
 * We must use bootloader settings first.
*************************************************************************/

#define SZ_64K				0x00010000
#define SZ_1M				0x00100000

#define ESS_START_ADDR			0xFDA00000
#define ESS_HEADER_SIZE			SZ_64K
#define ESS_LOG_KERNEL_SIZE		(2 * SZ_1M)
#define ESS_LOG_PLATFORM_SIZE		(4 * SZ_1M)
#define ESS_LOG_SFR_SIZE		(2 * SZ_1M)
#define ESS_LOG_S2D_SIZE		(0)
#define ESS_LOG_CACHEDUMP_SIZE		(10 * SZ_1M)
#define ESS_LOG_ETM_SIZE		(1 * SZ_1M)
#define ESS_LOG_PSTORE_SIZE		(2 * SZ_1M)
#define ESS_LOG_KEVENTS_SIZE		(8 * SZ_1M)

#define ESS_HEADER_OFFSET		0
#define ESS_LOG_KERNEL_OFFSET		(ESS_HEADER_OFFSET + ESS_HEADER_SIZE)
#define ESS_LOG_PLATFORM_OFFSET		(ESS_LOG_KERNEL_OFFSET + ESS_LOG_KERNEL_SIZE)
#define ESS_LOG_SFR_OFFSET		(ESS_LOG_PLATFORM_OFFSET + ESS_LOG_PLATFORM_SIZE)
#define ESS_LOG_S2D_OFFSET		(ESS_LOG_SFR_OFFSET + ESS_LOG_SFR_SIZE)
#define ESS_LOG_CACHEDUMP_OFFSET	(ESS_LOG_S2D_OFFSET + ESS_LOG_S2D_SIZE)
#define ESS_LOG_ETM_OFFSET		(ESS_LOG_CACHEDUMP_OFFSET + ESS_LOG_CACHEDUMP_SIZE)
#define ESS_LOG_PSTORE_OFFSET		(ESS_LOG_ETM_OFFSET + ESS_LOG_ETM_SIZE)
#define ESS_LOG_KEVENTS_OFFSET		(ESS_LOG_PSTORE_OFFSET + ESS_LOG_PSTORE_SIZE)

#define ESS_HEADER_ADDR			(ESS_START_ADDR + ESS_HEADER_OFFSET)
#define ESS_LOG_KERNEL_ADDR		(ESS_START_ADDR + ESS_LOG_KERNEL_OFFSET)
#define ESS_LOG_PLATFORM_ADDR		(ESS_START_ADDR + ESS_LOG_PLATFORM_OFFSET)
#define ESS_LOG_SFR_ADDR		(ESS_START_ADDR + ESS_LOG_SFR_OFFSET)
#define ESS_LOG_S2D_ADDR		(ESS_START_ADDR + ESS_LOG_S2D_OFFSET)
#define ESS_LOG_CACHEDUMP_ADDR		(ESS_START_ADDR + ESS_LOG_CACHEDUMP_OFFSET)
#define ESS_LOG_ETM_ADDR		(ESS_START_ADDR + ESS_LOG_ETM_OFFSET)
#define ESS_LOG_PSTORE_ADDR		(ESS_START_ADDR + ESS_LOG_PSTORE_OFFSET)
#define ESS_LOG_KEVENTS_ADDR		(ESS_START_ADDR + ESS_LOG_KEVENTS_OFFSET)

/* Header size change 1MB for MB align. */
#define ESS_TOTAL_SIZE		       	(ESS_LOG_KERNEL_SIZE + ESS_LOG_PLATFORM_SIZE	\
					+  ESS_LOG_SFR_SIZE + ESS_LOG_S2D_SIZE		\
					+  ESS_LOG_CACHEDUMP_SIZE + ESS_LOG_ETM_SIZE	\
					+  ESS_LOG_PSTORE_SIZE + ESS_LOG_KEVENTS_SIZE	\
					+ SZ_1M)
#endif
