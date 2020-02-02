/*
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

struct dl1750_pdata {
	int gpio_rst;
	int gpio_dpp3p3;
	int gpio_dp1p35;
	int gpio_dp1p1;
	int gpio_hdmi_en;

	const char *ldo_2p5;
	const char *ldo_1p8;
};

/*
* 2) Enable Power rails for DL-1750:
*  1.Pull “DL1750_RST_N_3P3” to low, to give RESET signal.
*  2.Pull “DP3P3_ON” to high to enable 3.3
*  3.Enable 2.5V power supply
*  4.Enable 1.8V power supply
*  5.Pull “DP1P35_ON” to high to enable 1.3.5V
*  6.Pull “DP1P1_ON” to high to enable 1.1V
*  7.Proceed step2 to 6 in 10ms
*  8.Pull “DL1750_RST_N_3P3” to low, for 50ms.
* 
* 3) Release RESET signal to enable DL-1750:
*  9.Enable HDMI_5P0
*  10.Pull “HDMI_EN_1P8” to high, to enable 5V at HDMI connector.
*  11.Pull “DL1750_RST_N_3P3” to high to enable HDP_3P3
*/

static int dl1750_enable(struct dl1750_pdata *pdrv)
{
	pr_info("%s\n", __func__);
	gpio_set_value(pdrv->gpio_hdmi_en, 1);
	gpio_set_value(pdrv->gpio_rst, 1);

	return 0;
}

static int dl1750_power_on(struct dl1750_pdata *pdrv)
{
	int ret = 0;
	struct regulator *ldo_2p5;
	struct regulator *ldo_1p8;

	pr_info("%s\n", __func__);
	ldo_2p5 = regulator_get(NULL, pdrv->ldo_2p5);
	if (IS_ERR_OR_NULL(ldo_2p5)) {
		pr_err("dl1750: failed to get ldo_2p5\n");
		return -ENODEV;
	}
	ldo_1p8 = regulator_get(NULL, pdrv->ldo_1p8);
	if (IS_ERR_OR_NULL(ldo_1p8)) {
		pr_err("dl1750: failed to get ldo_1p8\n");
		regulator_put(ldo_2p5);
		return -ENODEV;
	}

	gpio_set_value(pdrv->gpio_rst, 0);
	gpio_set_value(pdrv->gpio_dpp3p3, 1);

	ret = regulator_enable(ldo_2p5);
	if (ret) {
		pr_err("dl1750: failed to enable ldo_2p5\n");
		goto err_exit;
	}
	ret = regulator_enable(ldo_1p8);
	if (ret) {
		pr_err("dl1750: failed to enable ldo_1p8\n");
		goto err_exit;
	}

	gpio_set_value(pdrv->gpio_dp1p35, 1);
	gpio_set_value(pdrv->gpio_dp1p1, 1);

	msleep(50);

err_exit:
	regulator_put(ldo_2p5);
	regulator_put(ldo_1p8);

	return ret;
}

static int dl1750_parse_dt(struct device *dev, struct dl1750_pdata *pdrv)
{
	int ret = 0;
	struct device_node *np = dev->of_node;

	pr_info("%s\n", __func__);

	pdrv->gpio_rst = of_get_named_gpio(np, "dl,gpio-rst_n_1p8", 0);
	ret = gpio_request(pdrv->gpio_rst, "dl_gpio_rst");
	if (ret) {
		pr_err("dl1750: failed to get gpio dl_gpio_rst\n");
		return ret;
	}
	pdrv->gpio_dpp3p3 = of_get_named_gpio(np, "dl,gpio-dpp3p3_on", 0);
	ret = gpio_request(pdrv->gpio_dpp3p3, "dl_gpio_dpp3p3");
	if (ret) {
		pr_err("dl1750: failed to get gpio dl_gpio_dpp3p3\n");
		return ret;
	}
	pdrv->gpio_dp1p35 = of_get_named_gpio(np, "dl,gpio-dp1p35_on", 0);
	ret = gpio_request(pdrv->gpio_dp1p35, "dl_gpio_dp1p35");
	if (ret) {
		pr_err("dl1750: failed to get gpio dl_gpio_dp1p35\n");
		return ret;
	}
	pdrv->gpio_dp1p1 = of_get_named_gpio(np, "dl,gpio-dp1p1_on", 0);
	ret = gpio_request(pdrv->gpio_dp1p1, "dl_gpio_dp1p1");
	if (ret) {
		pr_err("dl1750: failed to get gpio dl_gpio_dp1p1\n");
		return ret;
	}
	pdrv->gpio_hdmi_en = of_get_named_gpio(np, "dl,gpio-hdmi_en_1p8", 0);
	ret = gpio_request(pdrv->gpio_hdmi_en, "dl_gpio_hdmi_en");
	if (ret) {
		pr_err("dl1750: failed to get gpio dl_gpio_hdmi_en\n");
		return ret;
	}

	ret = of_property_read_string(np, "dl,ldo-dp_pm_2p5", &pdrv->ldo_2p5);
	if (ret) {
		pr_err("dl1750: failed to get gpio ldo-dp_pm_2p5\n");
		return ret;
	}
	ret = of_property_read_string(np, "dl,ldo-dp_pm_1p8", &pdrv->ldo_1p8);
	if (ret) {
		pr_err("dl1750: failed to get gpio ldo-dp_pm_1p8\n");
		return ret;
	}

	return ret;
}

static int dl1750_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct dl1750_pdata *pdrv;

	pr_info("%s\n", __func__);
	pdrv = devm_kzalloc(dev, sizeof(struct dl1750_pdata), GFP_KERNEL);
	if (!pdrv) {
		pr_err("dl1750: failed to malloc\n");
		return -ENOMEM;
	}

	ret = dl1750_parse_dt(dev, pdrv);

	if (!ret) {
		ret = dl1750_power_on(pdrv);
	}

	if (!ret) {
		ret = dl1750_enable(pdrv);
	}

	return ret;
}

static int dl1750_remove(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);
	return 0;
}

static const struct of_device_id dl1750_of_match[] = {
	{ .compatible = "dl-1750", },
	{ },
};
MODULE_DEVICE_TABLE(of, dl1750_of_match);

static struct platform_driver dl1750_device_driver = {
	.probe		= dl1750_probe,
	.remove		= dl1750_remove,
	.driver		= {
		.name	= "dl-1750",
		.of_match_table = of_match_ptr(dl1750_of_match),
	}
};

static int __init dl1750_init(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&dl1750_device_driver);
}

static void __exit dl1750_exit(void)
{
	platform_driver_unregister(&dl1750_device_driver);
}

late_initcall(dl1750_init);
module_exit(dl1750_exit);
