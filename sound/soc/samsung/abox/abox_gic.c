/* sound/soc/samsung/abox/abox_gic.c
 *
 * ALSA SoC - Samsung ABOX driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/smc.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/delay.h>

#include "abox_util.h"
#include "abox_gic.h"

#define GIC_IS_SECURE_FREE

void abox_gic_generate_interrupt(struct platform_device *pdev, int hw_irq)
{
#ifdef GIC_IS_SECURE_FREE
	struct abox_gic_data *data = platform_get_drvdata(pdev);
#endif
	dev_dbg(&pdev->dev, "%s(%d)\n", __func__, hw_irq);
#ifdef GIC_IS_SECURE_FREE
	writel((0x1 << 16) | (hw_irq & 0xF),
			data->gicd_base + GIC_DIST_SOFTINT);
#else
	dev_dbg(&pdev->dev, "exynos_smc() is called\n");
	exynos_smc(SMC_CMD_REG,
			SMC_REG_ID_SFR_W(0x13EF1000 + GIC_DIST_SOFTINT),
			(0x1 << 16) | (hw_irq & 0xF), 0);
#endif
}
EXPORT_SYMBOL(abox_gic_generate_interrupt);

void abox_gic_register_irq_handler(struct platform_device *pdev,
		irq_handler_t irq_handler, void *dev_id)
{
	struct abox_gic_data *data = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s\n", __func__);

	data->abox_gic_irq_handler.irq_handler = irq_handler;
	data->abox_gic_irq_handler.dev_id = dev_id;
}
EXPORT_SYMBOL(abox_gic_register_irq_handler);

void abox_gic_unregister_irq_handler(struct platform_device *pdev)
{
	struct abox_gic_data *data = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s\n", __func__);

	data->abox_gic_irq_handler.irq_handler = NULL;
	data->abox_gic_irq_handler.dev_id = NULL;
}
EXPORT_SYMBOL(abox_gic_unregister_irq_handler);

static irqreturn_t abox_gic_irq_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct abox_gic_data *data = platform_get_drvdata(pdev);
	struct abox_gic_irq_handler_t *irq_handler =
			&data->abox_gic_irq_handler;
	irqreturn_t result = IRQ_HANDLED;
	u32 irqstat, irqnr;

	dev_dbg(&pdev->dev, "%s\n", __func__);

	do {
		irqstat = readl(data->gicc_base + GIC_CPU_INTACK);
		irqnr = irqstat & GICC_IAR_INT_ID_MASK;
		dev_dbg(&pdev->dev, "IAR: %08X\n", irqstat);

		if (likely(irqnr < 16)) {
			writel(irqstat, data->gicc_base + GIC_CPU_EOI);
			writel(irqstat, data->gicc_base + GIC_CPU_DEACTIVATE);
			if (irq_handler->irq_handler) {
				result = irq_handler->irq_handler(irqnr,
						irq_handler->dev_id);
			}
			continue;
		} else if (unlikely(irqnr > 15 && irqnr < 1021)) {
			writel(irqstat, data->gicc_base + GIC_CPU_EOI);
			continue;
		}
		break;
	} while (1);

	return result;
}

void abox_gic_init_gic(struct platform_device *pdev)
{
	struct abox_gic_data *data = platform_get_drvdata(pdev);
	unsigned long arg;
	int i, result;

	dev_info(&pdev->dev, "%s\n", __func__);

	arg = SMC_REG_ID_SFR_W(data->gicc_base_phys + GIC_CPU_PRIMASK);
	result = exynos_smc(SMC_CMD_REG, arg, 0x000000FF, 0);

	arg = SMC_REG_ID_SFR_W(data->gicd_base_phys + GIC_DIST_CTRL);
	result = exynos_smc(SMC_CMD_REG, arg, 0x3, 0);
	if (is_secure_gic()) {
		for (i = 0; i < 1; i++) {
			arg = SMC_REG_ID_SFR_W(data->gicd_base_phys +
					GIC_DIST_IGROUP + (i * 4));
			result = exynos_smc(SMC_CMD_REG, arg, 0xFFFFFFFF, 0);
		}
	}
	for (i = 0; i < 40; i++) {
		arg = SMC_REG_ID_SFR_W(data->gicd_base_phys +
				GIC_DIST_PRI + (i * 4));
		result = exynos_smc(SMC_CMD_REG, arg, 0x10101010, 0);
	}

	writel(0x3, data->gicc_base + GIC_CPU_CTRL);
}
EXPORT_SYMBOL(abox_gic_init_gic);

int abox_gic_enable_irq(struct device *dev)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);

	if (likely(data->disabled)) {
		dev_info(dev, "%s\n", __func__);

		data->disabled = false;
		enable_irq(data->irq);
	}
	return 0;
}

int abox_gic_disable_irq(struct device *dev)
{
	struct abox_gic_data *data = dev_get_drvdata(dev);

	if (likely(!data->disabled)) {
		dev_info(dev, "%s\n", __func__);

		data->disabled = true;
		disable_irq(data->irq);
	}

	return 0;
}

static int samsung_abox_gic_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct abox_gic_data *data;
	int result;

	dev_info(dev, "%s\n", __func__);

	data = devm_kzalloc(dev, sizeof(struct abox_gic_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	platform_set_drvdata(pdev, data);

	data->gicd_base = devm_request_and_map_byname(pdev, "gicd",
			&data->gicd_base_phys, NULL);
	if (IS_ERR(data->gicd_base))
		return PTR_ERR(data->gicd_base);

	data->gicc_base = devm_request_and_map_byname(pdev, "gicc",
			&data->gicc_base_phys, NULL);
	if (IS_ERR(data->gicc_base))
		return PTR_ERR(data->gicc_base);

	data->irq = platform_get_irq(pdev, 0);
	if (IS_ERR_VALUE(data->irq)) {
		dev_err(dev, "Failed to get irq\n");
		return data->irq;
	}

	result = devm_request_irq(dev, data->irq, abox_gic_irq_handler,
		IRQF_TRIGGER_RISING, pdev->name, pdev);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Failed to request irq\n");
		return result;
	}

#ifndef CONFIG_PM
	abox_gic_resume(dev);
#endif
	dev_info(dev, "%s: probe complete\n", __func__);

	return 0;
}

static int samsung_abox_gic_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "%s\n", __func__);

	return 0;
}

static const struct of_device_id samsung_abox_gic_of_match[] = {
	{
		.compatible = "samsung,abox_gic",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_gic_of_match);

static struct platform_driver samsung_abox_gic_driver = {
	.probe  = samsung_abox_gic_probe,
	.remove = samsung_abox_gic_remove,
	.driver = {
		.name = "samsung-abox-gic",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_gic_of_match),
	},
};

module_platform_driver(samsung_abox_gic_driver);

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box GIC Driver");
MODULE_ALIAS("platform:samsung-abox-gic");
MODULE_LICENSE("GPL");
