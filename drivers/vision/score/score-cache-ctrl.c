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

#include <linux/io.h>
#include <linux/types.h>
#include "score-cache-ctrl.h"

// This function flush all data cache
void score_dcache_flush(struct score_system *system)
{
	// Reg.Name : SCORE_DC_CCTRL (Offset 0x2008)
	// Range      Description
	// bit[1]     write 1 : run flush
	//            read 0  : flush done
	// bit[0]     write 1 : run invalidate
	//            read 0  : invalidate done
	writel(0x3, system->regs + SCORE_DC_CCTRL);
	while (readl(system->regs + SCORE_DC_CCTRL) != 0x0);
}

/// Region control operates for given address range.
/// Address range is set by start address(SCORE_DC_RCTRL_ADDR) and
/// number of cache lines(SCORE_DC_RCTRL_SIZE).
/// Data size of one cache line is 0x20 bytes. Start address should be aligned
/// to 0x20(cache line’s size). Real number of cache line is register value + 1.
/// For example, if you want to control 4 cache lines,
/// you should set SCORE_DC_RCTRL_SIZE to 3.
void score_dcache_flush_region(struct score_system *system, void *addr, unsigned int size)
{
	u64 temp = (u64)addr;
	u32 addr_32 = (u32)temp;

	// Set region of data cache flush
	writel(addr_32, system->regs + SCORE_DC_RCTRL_ADDR);
	writel((size >> 5) + 1, system->regs + SCORE_DC_RCTRL_SIZE);

	// Reg.Name : SCORE_DC_RCTRL_MODE (Offset 0x200c)
	// Range      Description
	// bit[2:0]   write 6 : clean & invalidate
	//            read 0  : done
	writel(0x6, system->regs + SCORE_DC_RCTRL_MODE);
	while (readl(system->regs + SCORE_DC_RCTRL_MODE) != 0x0);
}

