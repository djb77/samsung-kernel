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
	u32 id;

	id = ((struct mcu_ipc_drv_data *)data)->id;
	irq_stat = mcu_ipc_readl(id, EXYNOS_MCU_IPC_INTSR0) & 0xFFFF0000;

	/* Interrupt Clear */
	mcu_ipc_writel(id, irq_stat, EXYNOS_MCU_IPC_INTCR0);

	for (i = 0; i < 16; i++) {
		if (irq_stat & (1 << (i + 16))) {
			if ((1 << (i + 16)) & mcu_dat[id].registered_irq)
				mcu_dat[id].hd[i].handler(mcu_dat[id].hd[i].data);
			else
				dev_err(mcu_dat[id].mcu_ipc_dev,
					"Unregistered INT received.\n");

			irq_stat &= ~(1 << (i + 16));
		}

		if (!irq_stat)
			break;
	}

	return IRQ_HANDLED;
}

int mbox_request_irq(enum mcu_ipc_region id, u32 int_num,
					void (*handler)(void *), void *data)
{
	if ((!handler) || (int_num > 15))
		return -EINVAL;

	mcu_dat[id].hd[int_num].data = data;
	mcu_dat[id].hd[int_num].handler = handler;
	mcu_dat[id].registered_irq |= 1 << (int_num + 16);

	return 0;
}
EXPORT_SYMBOL(mbox_request_irq);

int mcu_ipc_unregister_handler(enum mcu_ipc_region id, u32 int_num,
								void (*handler)(void *))
{
	if (!handler || (mcu_dat[id].hd[int_num].handler != handler))
		return -EINVAL;

	mcu_dat[id].hd[int_num].data = NULL;
	mcu_dat[id].hd[int_num].handler = NULL;
	mcu_dat[id].registered_irq &= ~(1 << (int_num + 16));

	return 0;
}
EXPORT_SYMBOL(mcu_ipc_unregister_handler);

void mbox_set_interrupt(enum mcu_ipc_region id, u32 int_num)
{
	/* generate interrupt */
	if (int_num < 16)
		mcu_ipc_writel(id, 0x1 << int_num, EXYNOS_MCU_IPC_INTGR1);
}
EXPORT_SYMBOL(mbox_set_interrupt);

void mcu_ipc_send_command(enum mcu_ipc_region id, u32 int_num, u16 cmd)
{
	/* write command */
	if (int_num < 16)
		mcu_ipc_writel(id, cmd, EXYNOS_MCU_IPC_ISSR0 + (8 * int_num));

	/* generate interrupt */
	mbox_set_interrupt(id, int_num);
}
EXPORT_SYMBOL(mcu_ipc_send_command);

u32 mbox_get_value(enum mcu_ipc_region id, u32 mbx_num)
{
	if (mbx_num < 64)
		return mcu_ipc_readl(id, EXYNOS_MCU_IPC_ISSR0 + (4 * mbx_num));
	else
		return 0;
}
EXPORT_SYMBOL(mbox_get_value);

void mbox_set_value(enum mcu_ipc_region id, u32 mbx_num, u32 msg)
{
	if (mbx_num < 64)
		mcu_ipc_writel(id, msg, EXYNOS_MCU_IPC_ISSR0 + (4 * mbx_num));
}
EXPORT_SYMBOL(mbox_set_value);

u32 mbox_extract_value(enum mcu_ipc_region id, u32 mbx_num, u32 mask, u32 pos)
{
	if (mbx_num < 64)
		return (mbox_get_value(id, mbx_num) >> pos) & mask;
	else
		return 0;
}
EXPORT_SYMBOL(mbox_extract_value);

void mbox_update_value(enum mcu_ipc_region id, u32 mbx_num,
				u32 msg, u32 mask, u32 pos)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&mcu_dat[id].lock, flags);

	if (mbx_num < 64) {
		val = mbox_get_value(id, mbx_num);
		val &= ~(mask << pos);
		val |= (msg & mask) << pos;
		mbox_set_value(id, mbx_num, val);
	}

	spin_unlock_irqrestore(&mcu_dat[id].lock, flags);
}
EXPORT_SYMBOL(mbox_update_value);

void mbox_sw_reset(enum mcu_ipc_region id)
{
	u32 reg_val;

	printk("Reset Mailbox registers\n");

	reg_val = mcu_ipc_readl(id, EXYNOS_MCU_IPC_MCUCTLR);
	reg_val |= (0x1 << MCU_IPC_MCUCTLR_MSWRST);

	mcu_ipc_writel(id, reg_val, EXYNOS_MCU_IPC_MCUCTLR) ;

	udelay(5);
}
EXPORT_SYMBOL(mbox_sw_reset);

static void mcu_ipc_clear_all_interrupt(enum mcu_ipc_region id)
{
	mcu_ipc_writel(id, 0xFFFF, EXYNOS_MCU_IPC_INTCR1);
}

#ifdef CONFIG_ARGOS
static int mcu_ipc_set_affinity(enum mcu_ipc_region id, struct device *dev, int irq)
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

	if (!zalloc_cpumask_var(&mcu_dat[id].dmask, GFP_KERNEL))
		return -ENOMEM;
	if (!zalloc_cpumask_var(&mcu_dat[id].imask, GFP_KERNEL))
		return -ENOMEM;

	cpumask_or(mcu_dat[id].imask, mcu_dat[id].imask, cpumask_of(irq_affinity_mask));
	cpumask_copy(mcu_dat[id].dmask, get_default_cpu_mask());

	return argos_irq_affinity_setup_label(irq, "IPC", mcu_dat[id].imask,
			mcu_dat[id].dmask);
}
#else
static int mcu_ipc_set_affinity(enum mcu_ipc_region id, struct device *dev, int irq)
{
	return 0;
}
#endif

#ifdef CONFIG_MCU_IPC_TEST
static void test_without_dev(enum mcu_ipc_region id)
{
	int i;
	char *region;

	switch(id) {
	case MCU_CP:
		region = "CP";
		break;
	case MCU_GNSS:
		region = "GNSS";
		break;
	default:
		region = NULL;
	}

	for (i = 0; i < 64; i++) {
		mbox_set_value(id, i, 64 - i);
		mdelay(50);
		dev_err(mcu_dat[id].mcu_ipc_dev,
				"Test without %s(%d): Read mbox value[%d]: %d\n",
				region, id, i, mbox_get_value(i));
	}
}
#endif

static int mcu_ipc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res = NULL;
	int mcu_ipc_irq;
	int err = 0;
	u32 id = 0;

	dev_err(&pdev->dev, "%s: mcu_ipc probe start.\n", __func__);

	err = of_property_read_u32(dev->of_node, "mcu,id", &id);
	if (err) {
		dev_err(&pdev->dev, "MCU IPC parse error! [id]\n");
		return err;
	}

	if (id >= MCU_MAX) {
		dev_err(&pdev->dev, "MCU IPC Invalid ID [%d]\n", id);
		return -EINVAL;
	}

	mcu_dat[id].id = id;
	mcu_dat[id].mcu_ipc_dev = &pdev->dev;

	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	if (!pdev->dev.coherent_dma_mask)
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	if (dev->of_node) {
		mcu_dt_read_string(dev->of_node, "mcu,name", mcu_dat[id].name);
		if (IS_ERR(&mcu_dat[id])) {
			dev_err(&pdev->dev, "MCU IPC parse error!\n");
			return PTR_ERR(&mcu_dat[id]);
		}
	}

	/* resource for mcu_ipc SFR region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mcu_dat[id].ioaddr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mcu_dat[id].ioaddr)) {
		dev_err(&pdev->dev, "failded to request memory resource\n");
		return PTR_ERR(mcu_dat[id].ioaddr);
	}

	/* Request IRQ */
	mcu_ipc_irq = platform_get_irq(pdev, 0);
	err = devm_request_irq(&pdev->dev, mcu_ipc_irq, mcu_ipc_handler, 0,
			pdev->name, &mcu_dat[id]);
	if (err) {
		dev_err(&pdev->dev, "Can't request MCU_IPC IRQ\n");
		return err;
	}

	mcu_ipc_clear_all_interrupt(id);

	/* set argos irq affinity */
	err = mcu_ipc_set_affinity(id, dev, mcu_ipc_irq);
	if (err)
		dev_err(dev, "Can't set IRQ affinity with(%d)\n", err);

#ifdef CONFIG_MCU_IPC_TEST
	test_without_dev(id);
#endif

	spin_lock_init(&mcu_dat[id].lock);

	dev_err(&pdev->dev, "%s: mcu_ipc probe done.\n", __func__);

	return 0;
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
		{ .compatible = "samsung,exynos7870-mailbox", },
		{ .compatible = "samsung,exynos-shd-ipc-mailbox", },
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
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(mcu_ipc_driver);

MODULE_DESCRIPTION("MCU IPC driver");
MODULE_AUTHOR("<hy50.seo@samsung.com>");
MODULE_LICENSE("GPL");
