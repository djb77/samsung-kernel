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

#ifndef _SCORE_REGS_USER_DEF_H
#define _SCORE_REGS_USER_DEF_H

#include <linux/kernel.h>
#include "score-regs.h"
#include "score-config.h"

/*
 * Special Purpose Define
 */
#define SCORE_CORE_HALT_CHK1			(0x0020)
#define SCORE_CORE_HALT_CHK2			(0x0024)
#define SCORE_SCORE2CPU_INT_STAT		(0x0064)
#define SCORE_COREDUMP0				(0x305C)
#define SCORE_COREDUMP1				(0x405C)
#define SCORE_RUN_STATE				(0x505C)
#define SCORE_CLK_FREQ_HZ			(0x605C)

#ifdef ENABLE_MEM_OFFSET
#define SCORE_MEMORY_OFFSET			SCORE_USERDEFINED52
#endif
#ifdef ENABLE_TIME_STAMP
#define SCORE_CLK_FREQ				SCORE_USERDEFINED53
#define SCORE_TIME_FROM_LINUX			SCORE_USERDEFINED54
#define SCORE_TEMP_TIMER_CNT7			SCORE_USERDEFINED55
#endif

#define SCORE_EXE_LOG				SCORE_USERDEFINED56

/*
 * Userdefined register Define
 * Used register 0 ~ 63
 * 0 ~ 51 : IPC queue
 * 52 : Memory offset
 * 53 : CLK info
 * 54 ~ 55 : Time
 * 56 : Exe log
 * 57 : TCM2DMA_CH_MAP
 * 58 ~ 63 : Bakery lock
 */
#define SCORE_USERDEFINED0			(0x7000)
#define SCORE_USERDEFINED1			(0x7004)
#define SCORE_USERDEFINED2			(0x7008)
#define SCORE_USERDEFINED3			(0x700C)
#define SCORE_USERDEFINED4			(0x7010)
#define SCORE_USERDEFINED5			(0x7014)
#define SCORE_USERDEFINED6			(0x7018)
#define SCORE_USERDEFINED7			(0x701C)
#define SCORE_USERDEFINED8			(0x7020)
#define SCORE_USERDEFINED9			(0x7024)

#define SCORE_USERDEFINED10			(0x7028)
#define SCORE_USERDEFINED11			(0x702C)
#define SCORE_USERDEFINED12			(0x7030)
#define SCORE_USERDEFINED13			(0x7034)
#define SCORE_USERDEFINED14			(0x7038)
#define SCORE_USERDEFINED15			(0x703C)
#define SCORE_USERDEFINED16			(0x7040)
#define SCORE_USERDEFINED17			(0x7044)
#define SCORE_USERDEFINED18			(0x7048)
#define SCORE_USERDEFINED19			(0x704C)

#define SCORE_USERDEFINED20			(0x7050)
#define SCORE_USERDEFINED21			(0x7054)
#define SCORE_USERDEFINED22			(0x7058)
#define SCORE_USERDEFINED23			(0x705C)
#define SCORE_USERDEFINED24			(0x7060)
#define SCORE_USERDEFINED25			(0x7064)
#define SCORE_USERDEFINED26			(0x7068)
#define SCORE_USERDEFINED27			(0x706C)
#define SCORE_USERDEFINED28			(0x7070)
#define SCORE_USERDEFINED29			(0x7074)

#define SCORE_USERDEFINED30			(0x7078)
#define SCORE_USERDEFINED31			(0x707C)
#define SCORE_USERDEFINED32			(0x7080)
#define SCORE_USERDEFINED33			(0x7084)
#define SCORE_USERDEFINED34			(0x7088)
#define SCORE_USERDEFINED35			(0x708C)
#define SCORE_USERDEFINED36			(0x7090)
#define SCORE_USERDEFINED37			(0x7094)
#define SCORE_USERDEFINED38			(0x7098)
#define SCORE_USERDEFINED39			(0x709C)

#define SCORE_USERDEFINED40			(0x70A0)
#define SCORE_USERDEFINED41			(0x70A4)
#define SCORE_USERDEFINED42			(0x70A8)
#define SCORE_USERDEFINED43			(0x70AC)
#define SCORE_USERDEFINED44			(0x70B0)
#define SCORE_USERDEFINED45			(0x70B4)
#define SCORE_USERDEFINED46			(0x70B8)
#define SCORE_USERDEFINED47			(0x70BC)
#define SCORE_USERDEFINED48			(0x70C0)
#define SCORE_USERDEFINED49			(0x70C4)

#define SCORE_USERDEFINED50			(0x70C8)
#define SCORE_USERDEFINED51			(0x70CC)
#define SCORE_USERDEFINED52			(0x70D0)
#define SCORE_USERDEFINED53			(0x70D4)
#define SCORE_USERDEFINED54			(0x70D8)
#define SCORE_USERDEFINED55			(0x70DC)
#define SCORE_USERDEFINED56			(0x70E0)
#define SCORE_USERDEFINED57			(0x70E4)
#define SCORE_USERDEFINED58			(0x70E8)
#define SCORE_USERDEFINED59			(0x70EC)

#define SCORE_USERDEFINED60			(0x70F0)
#define SCORE_USERDEFINED61			(0x70F4)
#define SCORE_USERDEFINED62			(0x70F8)
#define SCORE_USERDEFINED63			(0x70FC)

/*
 * Debugging Purpose Define
 */
#define SCORE_VERIFY_RESULT			(0x0300)
#define SCORE_VERIFY_OUTPUT			(0x0304)
#define SCORE_VERIFY_REFERENCE			(0x0308)
#define SCORE_VERIFY_X_VALUE			(0x030c)
#define SCORE_VERIFY_Y_VALUE			(0x0310)

static inline void score_dump_sfr(void __iomem *regs)
{
	score_err("[SFR] SCORE_IDLE:   %08X \n", readl(regs + SCORE_IDLE));
	score_err("[SFR] CLOCK_STATUS: %08X \n", readl(regs + SCORE_CLOCK_STATUS) & 0x1);
	score_err("[SFR] DEBUG1:	    %08X \n", readl(regs + SCORE_VERIFY_RESULT));
	score_err("[SFR] DEBUG2:       %08X \n", readl(regs + SCORE_VERIFY_OUTPUT));
	score_err("[SFR] DEBUG3:       %08X \n", readl(regs + SCORE_VERIFY_REFERENCE));
	score_err("[SFR] DEBUG4:       %08X \n", readl(regs + SCORE_VERIFY_X_VALUE));
	score_err("[SFR] DEBUG5:       %08X \n", readl(regs + SCORE_VERIFY_Y_VALUE));
	score_err("[SFR] RUN_STATE:    %08X \n", readl(regs + SCORE_RUN_STATE));
	score_err("[SFR] EXE_LOG:      %08X \n", readl(regs + SCORE_EXE_LOG));
	print_hex_dump(KERN_INFO, "SCORE [IPC-QUEUE]", DUMP_PREFIX_OFFSET, 32, 4,
			regs + 0x7000, 56 * 4, false);
}

#endif /* SCORE_REGS_USER_DEF_H */
