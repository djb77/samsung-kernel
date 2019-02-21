/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - mapping cpu onto physical core
 * Author: Park Bumgyu <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/smc.h>
#include <asm/cputype.h>

static char lot_id[6];

static u32 CHIPID_ReverseValue(u32 uValue, u32 nBitCnt)
{
	u32 uTemp, uRetValue = 0;
	u32 i;

	for (i = 0; i < nBitCnt; i++) {
		uTemp = (uValue >> i) & 0x1;
		uRetValue += uTemp << ((nBitCnt - 1) - i);
	}

	return uRetValue;
}

static void CHIPID_DecTo36(u32 in, char *p)
{
	u32 nMod;
	u32 i;

	for (i = 4; i >= 1; i--) {
		nMod = in % 36;
		in /= 36;
		p[i] = (nMod < 10) ? (nMod + '0') : (nMod - 10 + 'A');
	}

	p[0] = 'N';
	p[5] = 0;
}

void __init get_lot_id(void)
{
	void __iomem *reg;
	int value;

	reg = early_ioremap(0x10000004, SZ_4K);
	value = __raw_readl(reg);
	early_iounmap(reg, SZ_4K);

	value = CHIPID_ReverseValue(value, 32);
	value = (value >> 11) & 0x1fffff;
	CHIPID_DecTo36(value, lot_id);

	pr_info("Lot ID = %s\n", lot_id);
}

u64 get_auto_cpu_hwid(u32 cpu)
{
	int ret = exynos_smc(SMC_CMD_CPUMAP, cpu, 0, 0);

	if (ret < 0)
		return INVALID_HWID;

	if (cpu >= 4) {
		if (strcmp(lot_id, "NZNP4") == 0 ||
				strcmp(lot_id, "NZNP5") == 0 ||
				strcmp(lot_id, "NZNP7") == 0 ||
				strcmp(lot_id, "NZNRH") == 0 ||
				strcmp(lot_id, "NZNVK") == 0 ||
				strcmp(lot_id, "NZNV2") == 0)
			return INVALID_HWID;
	}

	return ret;
}
