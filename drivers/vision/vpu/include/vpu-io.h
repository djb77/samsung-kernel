/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __VPU_IO_H__
#define __VPU_IO_H__

#include <linux/io.h>
#include "vpu-config.h"

#define IOR8(port)            readb((const volatile void *)&port)
#define IOR16(port)           readw((const volatile void *)&port)
#define IOR32(port)           readl((const volatile void *)&port)
#define IOR64(port)           readq((const volatile void *)&port)

#ifdef DBG_VERBOSE_IO
#define IOW8(port, val) \
	do { pr_info("ADDR: %p, VAL: 0x%02x\r\n", \
		&port, val); writeb(val, &port);\
	} while (0)
#define IOW16(port, val) \
	do { pr_info("ADDR: %p, VAL: 0x%04x\r\n", \
		&port, val); writew(val, &port);\
	} while (0)
#define IOW32(port, val) \
	do { pr_info("ADDR: %p, VAL: 0x%08x\r\n", \
		&port, val); writel(val, &port);\
	} while (0)
#define IOW64(port, val) \
	do { pr_info("ADDR: %p, VAL: 0x%016llx\r\n", \
		&port, val); writeq(val, &port);\
	} while (0)

#else  /* not VERBOSE_WRITE */
#define IOW8(port, val)      writeb(val, &port)
#define IOW16(port, val)     writew(val, &port)
#define IOW32(port, val)     writel(val, &port)
#define IOW64(port, val)     writeq(val, &port)
#endif /* not VERBOSE_WRITE */

void *mem2iocpy(void *dst,void *src, u32 size);
void *io2memcpy(void *dst,void *src, u32 size);

#endif /* __VPUL_IO_H__ */
