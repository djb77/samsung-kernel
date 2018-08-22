/* sound/soc/samsung/abox/abox_vss.c
 *
 * ALSA SoC Audio Layer - Samsung Abox VSS driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/shm_ipc.h>
#include <linux/io.h>

#include "abox.h"

static unsigned int VSS_MAGIC_OFFSET = 0x500000;
static const int E9810_INT_FREQ = 178000;
static const int E9810_INT_FREQ_SPK = 400000;

int abox_vss_notify_call(struct device *dev, struct abox_data *data, int en)
{
	int ret = 0;

	dev_info(dev, "%s(%d)\n", __func__, en);

	if (en) {
		if (IS_ENABLED(CONFIG_SOC_EXYNOS9810)) {
			if (data->sound_type == SOUND_TYPE_SPEAKER)
				ret = abox_request_int_freq(dev, data, dev,
						E9810_INT_FREQ_SPK);
			else
				ret = abox_request_int_freq(dev, data, dev,
						E9810_INT_FREQ);
		}
	} else {
		if (IS_ENABLED(CONFIG_SOC_EXYNOS9810))
			ret = abox_request_int_freq(dev, data, dev, 0);
	}

	return ret;
}

static int samsung_abox_vss_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	void __iomem *magic_addr;

	dev_dbg(dev, "%s\n", __func__);

	of_property_read_u32(np, "magic_offset", &VSS_MAGIC_OFFSET);
	dev_info(dev, "magic_offset = 0x%08X\n", VSS_MAGIC_OFFSET);
	magic_addr = phys_to_virt(shm_get_vss_base() + VSS_MAGIC_OFFSET);
	writel(0, magic_addr);
	return 0;
}

static int samsung_abox_vss_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "%s\n", __func__);
	return 0;
}

static const struct of_device_id samsung_abox_vss_match[] = {
	{
		.compatible = "samsung,abox-vss",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_vss_match);

static struct platform_driver samsung_abox_vss_driver = {
	.probe  = samsung_abox_vss_probe,
	.remove = samsung_abox_vss_remove,
	.driver = {
		.name = "samsung-abox-vss",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_vss_match),
	},
};

module_platform_driver(samsung_abox_vss_driver);

MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box VSS Driver");
MODULE_ALIAS("platform:samsung-abox-vss");
MODULE_LICENSE("GPL");
