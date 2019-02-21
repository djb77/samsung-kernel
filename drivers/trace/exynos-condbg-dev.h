/*
 * drivers/trace/exynos-condbg.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Exynos-Console-Debugger for Exynos SoC
 * This codes are based on fiq_debugger of google
 * /driver/staging/android/fiq_debugger
 *
 * Author: Hosung Kim <hosung0.kim@samsung.com>
 *         Changki Kim <changki.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _CONSOLE_DEBUGGER_H_
#define _CONSOLE_DEBUGGER_H_

#include <linux/serial_core.h>

#define DEBUGGER_NO_CHAR NO_POLL_CHAR
#define DEBUGGER_BREAK 0x00ff0100

/**
 * struct ecd_pdata - exynos console debugger platform data
 * @uart_resume:	used to restore uart state right
 * @uart_enable:	Do the work necessary to communicate with the uart
 *			hw (enable clocks, etc.). This must be ref-counted.
 * @uart_disable:	Do the work necessary to disable the uart hw
 *			(disable clocks, etc.). This must be ref-counted.
 * @uart_dev_suspend:	called during PM suspend, generally not needed
 *			in debug state.
 * @uart_dev_resume:	called during PM resume, generally not needed
 *			in debug state
 */
struct ecd_pdata {
	int (*uart_init)(struct platform_device *pdev);
	void (*uart_free)(struct platform_device *pdev);
	int (*uart_resume)(struct platform_device *pdev);
	int (*uart_getc)(struct platform_device *pdev);
	void (*uart_putc)(struct platform_device *pdev, unsigned int c);
	void (*uart_flush)(struct platform_device *pdev);
	void (*uart_enable)(struct platform_device *pdev);
	void (*uart_disable)(struct platform_device *pdev);

	int (*uart_dev_suspend)(struct platform_device *pdev);
	int (*uart_dev_resume)(struct platform_device *pdev);

	void (*force_irq)(struct platform_device *pdev, unsigned int irq);
	void (*force_irq_ack)(struct platform_device *pdev, unsigned int irq);
};
#endif
