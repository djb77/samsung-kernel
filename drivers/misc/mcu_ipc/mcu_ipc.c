/*
 * Copyright (C) 2014 Samsung Electronics Co.Ltd
 * http://www.samsung.com
 *
 * MCU IPC driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
*/

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

#include "regs-mcu_ipc.h"
#include "mcu_ipc.h"

static irqreturn_t mcu_ipc_handler(int irq, void *data)
{
	u32 irq_stat, i;

	if (!mcu_dat.ioaddr) {
		pr_err("%s: can't exec this func, probe failure\n", __func__);
		goto exit;
	}

	irq_stat = mcu_ipc_readl(EXYNOS_MCU_IPC_INTSR0) & 0xFFFF0000;

	for (i = 0; i < 16; i++) {
		if (irq_stat & (1 << (i + 16))) {
			if ((1 << (i + 16)) & mcu_dat.registered_irq)
				mcu_dat.hd[i].handler(mcu_dat.hd[i].data);
			else
				dev_err_ratelimited(mcu_dat.mcu_ipc_dev,
					"irq:0x%x, Unregistered INT received.\n", irq_stat);

			/* Interrupt Clear */
			mcu_ipc_writel(1 << (i + 16), EXYNOS_MCU_IPC_INTCR0);
			irq_stat &= ~(1 << (i + 16));
		}

		if (!irq_stat)
			break;
	}

exit:
	return IRQ_HANDLED;
}

int mbox_request_irq(u32 int_num, void (*handler)(void *), void *data)
{
	if ((!handler) || (int_num > 15) || !mcu_dat.ioaddr)
		return -EINVAL;

	mcu_dat.hd[int_num].data = data;
	mcu_dat.hd[int_num].handler = handler;
	mcu_dat.registered_irq |= 1 << (int_num + 16);

	return 0;
}
EXPORT_SYMBOL(mbox_request_irq);

int mcu_ipc_unregister_handler(u32 int_num, void (*handler)(void *))
{
	if (!handler || !mcu_dat.ioaddr ||
			(mcu_dat.hd[int_num].handler != handler))
		return -EINVAL;

	mcu_dat.hd[int_num].data = NULL;
	mcu_dat.hd[int_num].handler = NULL;
	mcu_dat.registered_irq &= ~(1 << (int_num + 16));

	return 0;
}
EXPORT_SYMBOL(mcu_ipc_unregister_handler);

void mbox_set_interrupt(u32 int_num)
{
	if (!mcu_dat.ioaddr) {
		pr_err("%s: can't exec this func, probe failure\n", __func__);
		return;
	}

	/* generate interrupt */
	if (int_num < 16)
		mcu_ipc_writel(0x1 << int_num, EXYNOS_MCU_IPC_INTGR1);
}
EXPORT_SYMBOL(mbox_set_interrupt);

void mcu_ipc_send_command(u32 int_num, u16 cmd)
{
	if (!mcu_dat.ioaddr) {
		pr_err("%s: can't exec this func, probe failure\n", __func__);
		return;
	}

	/* write command */
	if (int_num < 16)
		mcu_ipc_writel(cmd, EXYNOS_MCU_IPC_ISSR0 + (8 * int_num));

	/* generate interrupt */
	mbox_set_interrupt(int_num);
}
EXPORT_SYMBOL(mcu_ipc_send_command);

void mcu_ipc_clear_all_interrupt(void)
{
	if (!mcu_dat.ioaddr) {
		pr_err("%s: can't exec this func, probe failure\n", __func__);
		return;
	}

	mcu_ipc_writel(0xFFFF, EXYNOS_MCU_IPC_INTCR1);
}

u32 mbox_get_value(u32 mbx_num)
{
	if (!mcu_dat.ioaddr)
		return -EINVAL;

	if (mbx_num < 64)
		return mcu_ipc_readl(EXYNOS_MCU_IPC_ISSR0 + (4 * mbx_num));
	else
		return 0;
}
EXPORT_SYMBOL(mbox_get_value);

void mbox_set_value(u32 mbx_num, u32 msg)
{
	if (!mcu_dat.ioaddr) {
		pr_err("%s: can't exec this func, probe failure\n", __func__);
		return;
	}

	if (mbx_num < 64)
		mcu_ipc_writel(msg, EXYNOS_MCU_IPC_ISSR0 + (4 * mbx_num));
}
EXPORT_SYMBOL(mbox_set_value);

#ifdef CONFIG_ARGOS
static int mcu_ipc_set_affinity(struct device *dev, int irq)
{
	struct device_node *np = dev->of_node;
	u32 irq_affinity_mask = 0;

	if (!np)	{
		dev_err(dev, "non-DT project, can't set irq affinity\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "mcu,irq_affinity_mask",
			&irq_affinity_mask))	{
		dev_err(dev, "Failed to get affinity mask\n");
		return -ENODEV;
	}

	dev_info(dev, "irq_affinity_mask = 0x%x\n", irq_affinity_mask);

	if (!zalloc_cpumask_var(&mcu_dat.dmask, GFP_KERNEL))
		return -ENOMEM;
	if (!zalloc_cpumask_var(&mcu_dat.imask, GFP_KERNEL))
		return -ENOMEM;

	cpumask_or(mcu_dat.imask, mcu_dat.imask, cpumask_of(irq_affinity_mask));
	cpumask_copy(mcu_dat.dmask, get_default_cpu_mask());

	return argos_irq_affinity_setup_label(irq, "IPC", mcu_dat.imask,
			mcu_dat.dmask);
}
#else
static inline int mcu_ipc_set_affinity(struct device *dev, int irq)
{
	return 0;
}
#endif

#ifdef CONFIG_MCU_IPC_TEST
static void test_without_cp(void)
{
	int i;

	for (i = 0; i < 64; i++) {
		mbox_set_value(i, 64 - i);
		mdelay(50);
		dev_err(mcu_dat.mcu_ipc_dev, "Test without CP: Read mbox value[%d]: %d\n",
				i, mbox_get_value(i));
	}
}
#endif

static int mcu_ipc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res = NULL;
	int mcu_ipc_irq;
	int err = 0;

	dev_err(dev, "%s: mcu init\n", __func__);
	mcu_dat.mcu_ipc_dev = dev;

	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;
	if (!dev->coherent_dma_mask)
		dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (dev->of_node) {
		mcu_dt_read_string(dev->of_node, "mcu,name", mcu_dat.name);
		if (IS_ERR(&mcu_dat)) {
			dev_err(dev, "MCU IPC parse error!\n");
			return PTR_ERR(&mcu_dat);
		}
	}

	/* resource for mcu_ipc SFR region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "no memory resource defined\n");
		return -ENOENT;
	}

	res = request_mem_region(res->start, resource_size(res), mcu_dat.name);
	if (!res) {
		dev_err(dev, "failded to request memory resource\n");
		return -ENOENT;
	}

	mcu_dat.ioaddr = ioremap(res->start, resource_size(res));
	if (!mcu_dat.ioaddr) {
		dev_err(dev, "failded to request memory resource\n");
		err = -ENXIO;
		goto release_mcu_ipc;
	}

	/* Request IRQ */
	mcu_ipc_irq = platform_get_irq(pdev, 0);
	dev_err(dev, "%s: mcu interrupt num: %d\n", __func__, mcu_ipc_irq);
	if (mcu_ipc_irq < 0) {
		dev_err(dev, "no irq specified\n");
		err = mcu_ipc_irq;
		goto unmap_ioaddr;
	}

	err = request_irq(mcu_ipc_irq, mcu_ipc_handler, 0, pdev->name, pdev);
	if (err) {
		dev_err(dev, "Can't request MCU_IPC IRQ\n");
		goto unmap_ioaddr;
	}

	mcu_ipc_clear_all_interrupt();

	/* set argos irq affinity */
	err = mcu_ipc_set_affinity(dev, mcu_ipc_irq);
	if (err)
		dev_err(dev, "Can't set IRQ affinity with(%d)\n", err);

#ifdef CONFIG_MCU_IPC_TEST
	test_without_cp();
#endif
	return 0;

unmap_ioaddr:
	iounmap(mcu_dat.ioaddr);
	mcu_dat.ioaddr = NULL;

release_mcu_ipc:
	release_mem_region(res->start, resource_size(res));
	mcu_dat.mcu_ipc_dev = NULL;

	return err;
}

static int __exit mcu_ipc_remove(struct platform_device *pdev)
{
	/* TODO */
	return 0;
}

#ifdef CONFIG_PM
static int mcu_ipc_suspend(struct device *dev)
{
	/* TODO */
	return 0;
}

static int mcu_ipc_resume(struct device *dev)
{
	/* TODO */
	return 0;
}
#else
#define mcu_ipc_suspend NULL
#define mcu_ipc_resume NULL
#endif

static const struct dev_pm_ops mcu_ipc_pm_ops = {
	.suspend = mcu_ipc_suspend,
	.resume = mcu_ipc_resume,
};

static const struct of_device_id exynos_mcu_ipc_dt_match[] = {
		{ .compatible = "samsung,exynos7580-mailbox", },
		{ .compatible = "samsung,exynos7890-mailbox", },
		{ .compatible = "samsung,exynos8890-mailbox", },
		{},
};
MODULE_DEVICE_TABLE(of, exynos_mcu_ipc_dt_match);

static struct platform_driver mcu_ipc_driver = {
	.probe		= mcu_ipc_probe,
	.remove		= mcu_ipc_remove,
	.driver		= {
		.name = "mcu_ipc",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_mcu_ipc_dt_match),
		.pm = &mcu_ipc_pm_ops,
	},
};
module_platform_driver(mcu_ipc_driver);

MODULE_DESCRIPTION("MCU IPC driver");
MODULE_AUTHOR("<hy50.seo@samsung.com>");
MODULE_LICENSE("GPL");
