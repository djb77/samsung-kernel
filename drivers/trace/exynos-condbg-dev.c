/*
 * drivers/trace/exynos-condbg-dev.c
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

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/stacktrace.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/serial_s3c.h>

#include "exynos-condbg-dev.h"

struct ecd_dev {
	struct ecd_pdata	pdata;
	struct platform_device	*pdev;
	void __iomem 		*debug_port_base;
	int 			irq;
	u32 			baud;
	u32 			frac_baud;
	u32			usi_v2;
};

static inline struct ecd_dev *get_dbg(struct platform_device *pdev)
{
	struct ecd_pdata *pdata = dev_get_platdata(&pdev->dev);
	return container_of(pdata, struct ecd_dev, pdata);
}

static inline void exynos_write(struct ecd_dev *dbg,
			       unsigned int val, unsigned int off)
{
	__raw_writel(val, dbg->debug_port_base + off);
}

static inline unsigned int exynos_read(struct ecd_dev *dbg,
				      unsigned int off)
{
	return __raw_readl(dbg->debug_port_base + off);
}

static int debug_port_init(struct platform_device *pdev)
{
	struct ecd_dev *dbg = get_dbg(pdev);
	unsigned long timeout;

	if (dbg->usi_v2) {
		exynos_write(dbg, USI_RESET, USI_CON);
		exynos_write(dbg, USI_HWACG_CLKREQ_ON, USI_OPTION);
	}

	exynos_write(dbg, dbg->baud, S3C2410_UBRDIV);
	exynos_write(dbg, dbg->frac_baud, S3C2443_DIVSLOT);

	/* Mask and clear all interrupts */
	exynos_write(dbg, 0xF, S3C64XX_UINTM);
	exynos_write(dbg, 0xF, S3C64XX_UINTP);

	exynos_write(dbg, S3C2410_LCON_CS8, S3C2410_ULCON);
	exynos_write(dbg, S5PV210_UCON_DEFAULT, S3C2410_UCON);
	exynos_write(dbg, S5PV210_UFCON_DEFAULT, S3C2410_UFCON);
	exynos_write(dbg, 0, S3C2410_UMCON);

	/* Reset TX and RX fifos */
	exynos_write(dbg, S5PV210_UFCON_DEFAULT | S3C2410_UFCON_RESETBOTH,
		S3C2410_UFCON);

	timeout = jiffies + HZ;
	while (exynos_read(dbg, S3C2410_UFCON) & S3C2410_UFCON_RESETBOTH)
		if (time_after(jiffies, timeout))
			return -ETIMEDOUT;

	/* Enable all interrupts except TX */
	exynos_write(dbg, S3C64XX_UINTM_TXD_MSK, S3C64XX_UINTM);

	return 0;
}

static int debug_getc(struct platform_device *pdev)
{
	struct ecd_dev *dbg = get_dbg(pdev);
	u32 stat;
	int ret = DEBUGGER_NO_CHAR;

	/* Clear all pending interrupts */
	exynos_write(dbg, 0xF, S3C64XX_UINTP);

	stat = exynos_read(dbg, S3C2410_UERSTAT);
	if (stat & S3C2410_UERSTAT_BREAK)
		return DEBUGGER_BREAK;

	stat = exynos_read(dbg, S3C2410_UTRSTAT);
	if (stat & S3C2410_UTRSTAT_RXDR)
		ret = exynos_read(dbg, S3C2410_URXH);

	return ret;
}

static void debug_putc(struct platform_device *pdev, unsigned int c)
{
	struct ecd_dev *dbg = get_dbg(pdev);
	int count = loops_per_jiffy;

	if (exynos_read(dbg, S3C2410_ULCON) != S3C2410_LCON_CS8)
		debug_port_init(pdev);

	while (exynos_read(dbg, S3C2410_UFSTAT) & S5PV210_UFSTAT_TXFULL)
		if (--count == 0)
			return;

	exynos_write(dbg, c, S3C2410_UTXH);
}

static void debug_flush(struct platform_device *pdev)
{
	struct ecd_dev *dbg = get_dbg(pdev);
	int count = loops_per_jiffy * HZ;

	while (!(exynos_read(dbg, S3C2410_UTRSTAT) & S3C2410_UTRSTAT_TXE))
		if (--count == 0)
			return;
}

static void debug_clear_rxfifo(struct platform_device *pdev)
{
	struct ecd_dev *dbg = get_dbg(pdev);
	u32 ufcon;

	ufcon = exynos_read(dbg, S3C2410_UFCON);
	ufcon |= S3C2410_UFCON_RESETRX;
	exynos_write(dbg, ufcon, S3C2410_UFCON);
}

static int debug_suspend(struct platform_device *pdev)
{
	struct ecd_dev *dbg = get_dbg(pdev);
#ifdef CONFIG_SERIAL_SAMSUNG_HWACG
	u32 ucon;
#endif

	exynos_write(dbg, 0xF, S3C64XX_UINTM);
	ucon = exynos_read(dbg, S3C2410_UTRSTAT);
	ucon |= S3C2410_UTRSTAT_TIMEOUT;
	exynos_write(dbg, ucon, S3C2410_UTRSTAT);

#ifdef CONFIG_SERIAL_SAMSUNG_HWACG
	ucon = exynos_read(dbg, S3C2410_UCON);
	ucon &= ~(S3C2410_UCON_RXIRQMODE | S3C2410_UCON_TXIRQMODE);
	exynos_write(dbg, ucon, S3C2410_UCON);
#endif
	return 0;
}

static int debug_resume(struct platform_device *pdev)
{
	debug_port_init(pdev);

	return 0;
}

static int ecd_dev_probe(struct platform_device *pdev)
{
	struct platform_device *ecd_pdev;
	struct device_node *np;
	int uart_irq;
	unsigned int uart_port;
	int ret = -ENOMEM;
	struct ecd_dev *dbg = NULL;
	struct resource *res;

	if (initial_no_firmware)
		return -EINVAL;

	dbg = devm_kzalloc(&pdev->dev, sizeof(struct ecd_dev), GFP_KERNEL);
	if (!dbg) {
		dev_err(&pdev->dev, "Failed to allocate dbg structure\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "No mem resource\n");
		return -EINVAL;
	}

	dbg->debug_port_base = devm_ioremap_resource(&pdev->dev, res);
	if (dbg->debug_port_base == NULL) {
		dev_err(&pdev->dev, "failed to claim register region\n");
		return -ENOENT;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res)
		return -ENXIO;

	uart_irq = platform_get_irq_byname(pdev, "uart_irq");
	if (uart_irq < 0) {
		dev_err(&pdev->dev, "No IRQ for uart_irq, error=%d\n", uart_irq);
		return uart_irq;
	}

	np = pdev->dev.of_node;
	of_property_read_u32(np, "uart_port", &uart_port);
	if (uart_port > CONFIG_SERIAL_SAMSUNG_UARTS) {
		dev_err(&pdev->dev, "wrong descriptor for uart_port, error=%d\n", uart_port);
		return -EINVAL;
	}

	dbg->pdata.uart_init = debug_port_init;
	dbg->pdata.uart_getc = debug_getc;
	dbg->pdata.uart_putc = debug_putc;
	dbg->pdata.uart_flush = debug_flush;
	dbg->pdata.uart_clear_rxfifo = debug_clear_rxfifo;
	dbg->pdata.uart_dev_suspend = debug_suspend;
	dbg->pdata.uart_dev_resume = debug_resume;
	dbg->baud = exynos_read(dbg, S3C2410_UBRDIV);
	dbg->frac_baud = exynos_read(dbg, S3C2443_DIVSLOT);
	dbg->pdev = pdev;
	pdev->dev.platform_data = &dbg->pdata;

	if (of_get_property(np, "samsung,usi-serial-v2", NULL))
		dbg->usi_v2 = 1;
	else
		dbg->usi_v2 = 0;

	debug_port_init(pdev);
	ecd_pdev = kzalloc(sizeof(struct platform_device), GFP_KERNEL);
	if(!ecd_pdev) {
		dev_err(&pdev->dev, "Failed to alloc ecd_dev platform device\n");
		goto err_free;
	}

	ecd_pdev->name = "console_debugger";
	ecd_pdev->id = uart_port;
	ecd_pdev->dev.platform_data = &dbg->pdata;
	ecd_pdev->resource = res;
	ecd_pdev->num_resources = 1;

	if (platform_device_register(ecd_pdev)) {
		dev_err(&pdev->dev, "Failed to register ecd_dev \n");
		goto err_pdev;
	}

	pr_info("Success to register ECD UART\n");
	return 0;

err_pdev:
	kfree(ecd_pdev);
err_free:
	kfree(dbg);
	return ret;
}

static const struct of_device_id ecd_dev_match[] = {
	{.compatible = "samsung,exynos_console_debugger",},
	{},
};

static struct platform_driver ecd_dev_driver = {
	.probe      = ecd_dev_probe,
	.driver     = {
		.name   = "exynos_console_debugger",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(ecd_dev_match),
	},
};

static int __init ecd_dev_init(void)
{
	return platform_driver_register(&ecd_dev_driver);
}
postcore_initcall_sync(ecd_dev_init);
