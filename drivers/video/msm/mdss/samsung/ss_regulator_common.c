/*
 * ss_regulator_common.c
 *
 * Support for Qcomm BLIC configure and display reset seqence
 *
 * Copyright (C) 2016, Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/regulator/machine.h>

#include "ss_regulator_common.h"

/* Samsung fake regulator driver to support Qcomm display power on/off sequence.
 * Qcomm display driver uses "struct dss_module_power panel_power_data", which
 * is from "qcom,panel-supply-entries" in each board dtsi file,
 * to control display power on/off sequence.
 *
 * - Limitation of panel_power_data
 * It supports only regulator type power source.
 * But display power on/off sequence also includes "control of panel reset pin"
 * and "BLIC configuration", like ISL9xxx chip, via I2C.
 *
 * - Purpose of ss_regulator_common
 * This driver supports "panel reset control" and "BLIC configuaration via I2C"
 * with regulator type.
 * This driver registers "panel-rst" and "bl-conf" as regulator.
 * If you put these regulators into "qcom,panel-supply-entries" in your target
 * board dtsi file, panel_power_data get regulatgor instance of those, and it
 * calls regulator_enable(), then panelrst_enable() or blconf_enable() will be
 * called in result.
 *
 * - How to modify your target board dtsi file to use ss_regulator_common driver
 * There are two ways to put ss_regulator node in the baord dtsi.
 * 1) If you have I2C interface to BLIC
 *      put ss-regulator like below, to call ssreg_i2c_probe().
 * 2) If you don't have I2C interface to BLIC or you just want to use only
 * panel reset regulator
 *      put ss_regulator_platform like below, to call ssreg_platform_probe().
 *
 * - Example of ss_regulator and ss_regulator_platform node in board dtis

&soc {
i2c_22: i2c@22 {
	cell-index = <22>;
	compatible = "i2c-gpio";
	gpios = <&tlmm 21 0 // sda
		&tlmm 20 0 // scl
		>;
	i2c-gpio,delay-us = <2>;

	// If you have I2C interface to BLIC, use ss-regulator i2c driver
	ss_regulator@29 {
		compatible = "ss-regulator-common";
		reg = <0x29>;

		// BLIC enable pin
		bl_en_gpio = <&tlmm 94 GPIOF_OUT_INIT_HIGH>;

		fake_regulators {
			// control panel reset sequence
			ssreg_panelrst: ssreg_panelrst {
				// Do not put regulator-boot-on here.
				// It will call reset sequence in
				// kernel booting, and causes panel off.
				regulator-name = "ssreg-panelrst";
				regulator-type = <REGTYPE_PANEL_RESET>;

				// panel reset sequence: <level0 delay0>, <level1 delay1>, ...
				panel-rst-seq = <1 1>, <0 1>, <1 0>;
				rst-gpio = <&tlmm 60 GPIOF_OUT_INIT_HIGH>;
			};

			// configure BLIC initdata via I2C
			ssreg_blconf: ssreg_blconf {
				regulator-name = "ssreg-blconf";
				regulator-type = <REGTYPE_BL_CONFIG>;

				init_data = [
				// i2c:	addr		data
					01		00
					02		BF
				];
			};
		};
	};
};
};

&soc {
// If you don't have I2C interface to BLIC or you just want to use
// only panel reset regulator, use ss_regulator_platform platform driver.
ss_regulator_platform {
	compatible = "ss-regulator-platform-common";

	// BLIC enable pin
	bl_en_gpio = <&tlmm 94 GPIOF_OUT_INIT_HIGH>;

	fake_regulators {
		// control panel reset sequence
		ssreg_panelrst: ssreg_panelrst {
			// Do not put regulator-boot-on here.
			// It will call reset sequence in
			// kernel booting, and causes panel off.
			regulator-name = "ssreg-panelrst";
			regulator-type = <REGTYPE_PANEL_RESET>;

			// panel reset sequence: <level0 delay0>, <level1 delay1>, ...
			panel-rst-seq = <1 1>, <0 1>, <1 0>;
			rst-gpio = <&tlmm 60 GPIOF_OUT_INIT_HIGH>;
		};
	};
};
};

 *
 */

#define RST_SEQ_LEN	10
#define MAX_RDATA_LDO	10

enum {
	REGTYPE_PANEL_RESET = 0, /* display reset sequence */
	REGTYPE_BL_CONFIG, /* BLIC initial configure data via I2c */
	REGTYPE_GPIO_REGULATOR, /* GPIO regulator */
};

struct ssreg_rdata_panelrst {
	int rst_gpio;
	u32 gpio_rst_flags;
	u32 rst_seq[RST_SEQ_LEN];
	u32 rst_seq_len;
};

struct ssreg_rdata_blconf {
	char *addr;
	char *data;
	int size;
};

struct ssreg_rdata_gpioreg {
	int enable_gpio;
	u32 enable_gpio_flags;
};

struct ssreg_regulator_data {
	u32 reg_type;
	struct regulator_init_data *initdata;
	struct device_node *of_node;

	union {
		struct ssreg_rdata_panelrst *r_panelrst; /* REGTYPE_PANEL_RESET */
		struct ssreg_rdata_blconf *r_blconf; /*REGTYPE_BL_CONFIG*/
		struct ssreg_rdata_gpioreg *r_gpioreg; /*REGTYPE_GPIO_REGULATOR*/
	};
};

struct ssreg_pdata {
	int num_regulators;
	struct ssreg_regulator_data *regulators;

	int enable_bl_gpio; /* gpio to enable BLIC */
	u32 gpio_bl_flags;
};

struct ssreg_pmic {
	struct i2c_client	*client;
	struct regulator_dev	**rdev;
	struct mutex		mtx;

	struct ssreg_pdata *pdata;
};

/* TODO: remove gPmic and find the way to call ssreg_enable_blic() from
 * other drivers, like notifier...
 */
struct ssreg_pmic *gPmic;

static int panelrst_enable(struct regulator_dev *rdev)
{

	struct ssreg_pmic *pmic = rdev_get_drvdata(rdev);
	struct ssreg_pdata *pdata = pmic->pdata;
	int id = rdev_get_id(rdev);
	struct ssreg_regulator_data *rdata = &pdata->regulators[id];
	struct ssreg_rdata_panelrst *r_panelrst = rdata->r_panelrst;
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	int i;

	dev_info(&rdev->dev, "enable reset pin(%d)\n", r_panelrst->rst_gpio);
	WARN(rdata->reg_type != REGTYPE_PANEL_RESET,
			"reg_type=%d", rdata->reg_type);

	for (i = 0; i < r_panelrst->rst_seq_len; ++i) {
		gpio_set_value((r_panelrst->rst_gpio), r_panelrst->rst_seq[i]);
		if (r_panelrst->rst_seq[++i])
			usleep_range(r_panelrst->rst_seq[i] * 1000,
					r_panelrst->rst_seq[i] * 1000);
	}

	vdd->reset_time_64 = ktime_to_ms(ktime_get());

	return 0;
}

static int panelrst_disable(struct regulator_dev *rdev)
{
	struct ssreg_pmic *pmic = rdev_get_drvdata(rdev);
	struct ssreg_pdata *pdata = pmic->pdata;
	int id = rdev_get_id(rdev);
	struct ssreg_regulator_data *rdata = &pdata->regulators[id];
	struct ssreg_rdata_panelrst *r_panelrst = rdata->r_panelrst;

	dev_info(&rdev->dev, "disable reset pin(%d)\n", r_panelrst->rst_gpio);
	WARN(rdata->reg_type != REGTYPE_PANEL_RESET,
			"reg_type=%d", rdata->reg_type);

	gpio_set_value((r_panelrst->rst_gpio), 0);

	return 0;
}

static int panelrst_is_enabled(struct regulator_dev *rdev)
{
	struct ssreg_pmic *pmic = rdev_get_drvdata(rdev);
	struct ssreg_pdata *pdata = pmic->pdata;
	int id = rdev_get_id(rdev);
	struct ssreg_regulator_data *rdata = &pdata->regulators[id];
	struct ssreg_rdata_panelrst *r_panelrst = rdata->r_panelrst;
	int val;

	WARN(rdata->reg_type != REGTYPE_PANEL_RESET,
			"reg_type=%d", rdata->reg_type);

	val = gpio_get_value(r_panelrst->rst_gpio);

	dev_dbg(&rdev->dev, "reset is %s\n", val ? "enabled" : "disabled");

	return val;
}

static struct regulator_ops panelrst_ops = {
	.enable		= panelrst_enable,
	.disable	= panelrst_disable,
	.is_enabled	= panelrst_is_enabled,
};

static int blconf_i2c_write(struct i2c_client *client,
		u8 reg,  u8 val, unsigned int len)
{
	int err = 0;
	int retry = 3;
	u8 temp_val = val;

	while (retry--) {
		err = i2c_smbus_write_i2c_block_data(client,
				reg, len, &temp_val);
		if (err >= 0)
			return err;

		LCD_ERR("i2c transfer error. %d\n", err);
		usleep_range(1000,1000);
	}
	return err;
}

static int blconf_enable(struct regulator_dev *rdev)
{

	struct ssreg_pmic *pmic = rdev_get_drvdata(rdev);
	struct i2c_client *client = pmic->client;
	struct ssreg_pdata *pdata = pmic->pdata;
	int id = rdev_get_id(rdev);
	struct ssreg_regulator_data *rdata = &pdata->regulators[id];
	struct ssreg_rdata_blconf *r_blconf = rdata->r_blconf;
	int ret = 0;
	int i;

	dev_info(&rdev->dev, "enable blconf, len=%d\n", r_blconf->size);
	WARN(rdata->reg_type != REGTYPE_BL_CONFIG,
			"reg_type=%d", rdata->reg_type);
	WARN(!client, "no I2C client");

	mutex_lock(&pmic->mtx);
	for (i = 0 ; i < r_blconf->size; i++) {
		ret = blconf_i2c_write(client,
				r_blconf->addr[i], r_blconf->data[i],1);
		if (ret < 0) {
			dev_err(&rdev->dev, "i2c err(%d): reg=%x, val=%x\n",
				ret, r_blconf->addr[i], r_blconf->data[i]);
		}
	}
	mutex_unlock(&pmic->mtx);

	/* 
		To execute wake-up sequence under i2c-fail,
		we return only 0.
	*/

	return 0;
}

static int blconf_disable(struct regulator_dev *rdev)
{
	return 0;
}


static int blconf_is_enabled(struct regulator_dev *rdev)
{
	/* Regulator core always recognize that it is disabled,
	 * so it always skip to disable and not to skip to enable.
	 */
	return 0;
}

static struct regulator_ops blconf_ops = {
	.enable		= blconf_enable,
	.disable	= blconf_disable,
	.is_enabled	= blconf_is_enabled,
};

static struct regulator_desc ssreg_reg_desc[] = {
	{
		.name		= "panel-rst",
		.id		= REGTYPE_PANEL_RESET,
		.ops		= &panelrst_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	},
	{
		.name		= "bl-conf",
		.id		= REGTYPE_BL_CONFIG,
		.ops		= &blconf_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	},
};

int ssreg_enable_blic(int enable)
{
	struct ssreg_pdata *pdata = gPmic->pdata;

	if (IS_ERR_OR_NULL(pdata))
		goto err;

	pr_info("%s: %s enable_bl pin(%d)\n", __func__,
			enable ? "enable" : "disable", pdata->enable_bl_gpio);

	/* For PBA BOOTING */
	if (!mdss_panel_attached(DISPLAY_1))
		enable = 0;

	if (gpio_is_valid(pdata->enable_bl_gpio))
		gpio_set_value(pdata->enable_bl_gpio, !!enable);

err:
	return 0;
}

static int parse_panelrst(struct device *dev, struct device_node *np,
			struct ssreg_regulator_data *rdata)
{
	struct ssreg_rdata_panelrst *r_panelrst;
	struct property *data;
	u32 tmp[RST_SEQ_LEN];
	int i;
	int num = 0;
	int ret = 0;

	/* For PBA BOOTING */
	if (!mdss_panel_attached(DISPLAY_1))
		return ret;

	r_panelrst = devm_kzalloc(dev,
			sizeof(struct ssreg_rdata_panelrst), GFP_KERNEL);
	if (!r_panelrst)
		return -ENOMEM;

	r_panelrst->rst_gpio = of_get_named_gpio_flags(np, "rst-gpio", 0,
				&r_panelrst->gpio_rst_flags);

	ret = gpio_request_one(r_panelrst->rst_gpio,
			r_panelrst->gpio_rst_flags, "panel-rst");
	if (ret) {
		dev_err(dev, "fail to request rst gpio(%d, f=%x), ret=%d\n",
			r_panelrst->rst_gpio, r_panelrst->gpio_rst_flags, ret);
		return ret;
	}

	r_panelrst->rst_seq_len = 0;


	data = of_find_property(np, "panel-rst-seq", &num);
	num /= sizeof(u32);
	if (!data || !num || num > RST_SEQ_LEN || num % 2) {
		dev_err(dev, "fail to find panel-rst-seq: num=%d\n", num);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "panel-rst-seq", tmp, num);
	if (ret) {
		dev_err(dev, "fail to read panel-rst-seq: ret=%d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < num; ++i)
		r_panelrst->rst_seq[i] = tmp[i];

	r_panelrst->rst_seq_len = num;

	rdata->r_panelrst = r_panelrst;

	dev_info(dev, "rst_gpio=%d, 0x%x, rst_seq_len=%d, boot_on=%d",
			r_panelrst->rst_gpio,
			r_panelrst->gpio_rst_flags,
			r_panelrst->rst_seq_len,
			rdata->initdata->constraints.boot_on);

	return 0;
}

static int parse_blconf(struct device *dev, struct device_node *np,
			struct ssreg_regulator_data *rdata)
{
	struct ssreg_rdata_blconf *r_blconf;
	const char *data;
	int  data_offset;
	int len;
	int i;
	int ret = 0;

	/* For PBA BOOTING */
	if (!mdss_panel_attached(DISPLAY_1))
		return ret;

	r_blconf = devm_kzalloc(dev,
			sizeof(struct ssreg_rdata_blconf), GFP_KERNEL);
	if (!r_blconf)
		return -ENOMEM;

	data = of_get_property(np, "init_data", &len);
	if (!data) {
		dev_info(dev, "no bl init_data");
		r_blconf->size = 0;
		return -EINVAL;
	}

	if ((len % 2) != 0) {
		dev_err(dev, "err: table entries len=%d (blic_init_data)", len);
		return -EINVAL;
	}

	r_blconf->size = len / 2;

	r_blconf->addr = devm_kzalloc(dev, r_blconf->size, GFP_KERNEL);
	if (!r_blconf->addr)
		return -ENOMEM;

	r_blconf->data = devm_kzalloc(dev, r_blconf->size, GFP_KERNEL);
	if (!r_blconf->data) {
		ret = -ENOMEM;
		goto error;
	}

	data_offset = 0;

	for (i = 0 ; i < r_blconf->size; i++) {
		r_blconf->addr[i] = data[data_offset++];
		r_blconf->data[i] = data[data_offset++];

		dev_info(dev, "BLIC inidata: 0x%x 0x%x\n",
				r_blconf->addr[i], r_blconf->data[i]);
	}

	rdata->r_blconf = r_blconf;

	return 0;

error:
	r_blconf->size = 0;
	devm_kfree(dev, r_blconf->addr);

	return ret;
}

static int parse_gpioreg(struct device *dev, struct device_node *np,
			struct ssreg_regulator_data *rdata)
{
	struct ssreg_rdata_gpioreg *r_gpioreg;
	int ret = 0;
	bool enable_high;

	r_gpioreg = devm_kzalloc(dev,
			sizeof(struct ssreg_rdata_gpioreg), GFP_KERNEL);
	if (!r_gpioreg)
		return -ENOMEM;

	r_gpioreg->enable_gpio = of_get_named_gpio(np, "gpio", 0);

	enable_high = of_property_read_bool(np, "enable-active-high");
	if (rdata->initdata->constraints.boot_on) {
		if (enable_high)
			r_gpioreg->enable_gpio_flags = GPIOF_OUT_INIT_HIGH;
		else
			r_gpioreg->enable_gpio_flags = GPIOF_OUT_INIT_LOW;
	} else {
		if (enable_high)
			r_gpioreg->enable_gpio_flags = GPIOF_OUT_INIT_LOW;
		else
			r_gpioreg->enable_gpio_flags = GPIOF_OUT_INIT_HIGH;
	}

	rdata->r_gpioreg = r_gpioreg;

	return ret;
}

static struct ssreg_pdata *ssreg_parse_dt(struct device *dev)
{
	struct ssreg_pdata *pdata;
	struct device_node *regulators_np, *reg_np;
	struct ssreg_regulator_data *rdata;
	int ret;

	regulators_np = of_find_node_by_name(dev->of_node, "fake_regulators");
	if (unlikely(regulators_np == NULL)) {
		dev_err(dev, "regulators node not found\n");
		return ERR_PTR(-EINVAL);
	}

	pdata = devm_kzalloc(dev,
			sizeof(struct ssreg_pdata), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->num_regulators = of_get_child_count(regulators_np);
	rdata = devm_kzalloc(dev, sizeof(*rdata) *
				pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		of_node_put(regulators_np);
		dev_err(dev, "could not allocate memory for regulator data\n");
		return ERR_PTR(-ENOMEM);
	}

	pdata->regulators = rdata;
	for_each_child_of_node(regulators_np, reg_np) {
		rdata->initdata = of_get_regulator_init_data(dev, reg_np);
		rdata->of_node = reg_np;

		of_property_read_u32(reg_np, "regulator-type",
				&rdata->reg_type);

		if (rdata->reg_type == REGTYPE_PANEL_RESET)
			ret = parse_panelrst(dev, reg_np, rdata);
		else if (rdata->reg_type == REGTYPE_BL_CONFIG)
			ret = parse_blconf(dev, reg_np, rdata);
		else if (rdata->reg_type == REGTYPE_GPIO_REGULATOR)
			ret = parse_gpioreg(dev, reg_np, rdata);
		else
			dev_err(dev, "wrong reg_type(%d)\n", rdata->reg_type);

		if (ret)
			return ERR_PTR(ret);

		rdata++;
	}
	of_node_put(regulators_np);

	pdata->enable_bl_gpio = of_get_named_gpio_flags(dev->of_node,
			"bl_en_gpio", 0, &pdata->gpio_bl_flags);

	if (gpio_is_valid(pdata->enable_bl_gpio)) {
		ret = gpio_request_one(pdata->enable_bl_gpio,
				pdata->gpio_bl_flags, "ss_reg_bl_en");
		if (ret) {
			dev_err(dev, "fail to request bl en gpio(%d), ret=%d\n",
					pdata->enable_bl_gpio, ret);
			return ERR_PTR(ret);
		}

		dev_info(dev, "enable_bl_gpio=%d, 0x%x",
				pdata->enable_bl_gpio, pdata->gpio_bl_flags);
	}

	return pdata;
}

static struct regulator_ops fixed_voltage_ops = {
};

static int _ssreg_probe(struct ssreg_pmic *pmic, struct device *dev)
{
	struct ssreg_pdata *pdata;
	struct regulator_config config = { };
	struct ssreg_regulator_data *rdata;
	int size;
	int i;

	dev_info(dev, "+\n");

	if (!mdss_panel_attached(DISPLAY_1)) {
		dev_info(dev, "PBA booting... stop to support ssreg\n");
		return 0;
	}

	if (dev->of_node) {
		pdata = ssreg_parse_dt(dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	} else {
		pdata = dev_get_platdata(dev);
	}

	pmic->pdata = pdata;

	size = sizeof(struct regulator_dev *) * pdata->num_regulators;
	pmic->rdev = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!pmic->rdev)
		return -ENOMEM;

	for (i = 0; i < pdata->num_regulators; i++) {
		struct regulator_desc *desc;

		rdata = &pdata->regulators[i];

		config.dev = dev;
		config.driver_data = pmic;
		config.init_data = rdata->initdata;
		config.of_node = rdata->of_node;

		if (rdata->reg_type == REGTYPE_GPIO_REGULATOR) {
			config.ena_gpio = rdata->r_gpioreg->enable_gpio;
			config.ena_gpio_flags = rdata->r_gpioreg->enable_gpio_flags;

			desc = devm_kzalloc(dev, sizeof(struct regulator_desc),
					GFP_KERNEL);
			if (!desc)
				return -ENOMEM;
			desc->name = devm_kstrdup(dev,
					config.init_data->constraints.name,
					GFP_KERNEL);
			desc->type = REGULATOR_VOLTAGE;
			desc->owner = THIS_MODULE;
			desc->ops = &fixed_voltage_ops;
		} else {
			desc = &ssreg_reg_desc[rdata->reg_type];
		}

		pmic->rdev[i] = regulator_register(desc, &config);
		if (IS_ERR(pmic->rdev)) {
			dev_err(dev, "faild to register %s\n",
					ssreg_reg_desc[rdata->reg_type].name);
			return PTR_ERR(pmic->rdev);
		}
	}

	gPmic = pmic;
	dev_info(dev, "-\n");

	return 0;
}

static int ssreg_i2c_probe(struct i2c_client *client,
				     const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct ssreg_pmic *pmic;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	pmic = devm_kzalloc(dev, sizeof(struct ssreg_pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	mutex_init(&pmic->mtx);

	pmic->client = client;
	i2c_set_clientdata(client, pmic);

	return _ssreg_probe(pmic, dev);
}

#ifdef CONFIG_OF
static const struct of_device_id ssreg_dt_ids[] = {
	{ .compatible = "ss-regulator-common" },
	{},
};
#endif

static const struct i2c_device_id ssreg_id[] = {
	{.name = "ss-reg", 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, ssreg_id);

static struct i2c_driver ssreg_i2c_driver = {
	.driver = {
		.name = "ss-reg",
		.owner = THIS_MODULE,
		.of_match_table	= of_match_ptr(ssreg_dt_ids),
	},
	.probe = ssreg_i2c_probe,
	.id_table = ssreg_id,
};

static int __init ssreg_init(void)
{
	return i2c_add_driver(&ssreg_i2c_driver);
}

static void __exit ssreg_cleanup(void)
{
	i2c_del_driver(&ssreg_i2c_driver);
}

subsys_initcall(ssreg_init);
module_exit(ssreg_cleanup);

static int ssreg_platform_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ssreg_pmic *pmic;

	pmic = devm_kzalloc(dev, sizeof(struct ssreg_pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	platform_set_drvdata(pdev, pmic);

	return _ssreg_probe(pmic, dev);
}

#ifdef CONFIG_OF
static const struct of_device_id ssreg_dt_platform_ids[] = {
	{ .compatible = "ss-regulator-platform-common" },
	{},
};
#endif

static struct platform_driver ssreg_platform_driver = {
	.probe		= ssreg_platform_probe,
	.driver		= {
		.name		= "ss-reg-platform",
		.owner		= THIS_MODULE,
		.of_match_table = of_match_ptr(ssreg_dt_platform_ids),
	},
};

static int __init ssreg_platform_init(void)
{
	return platform_driver_register(&ssreg_platform_driver);
}

static void __exit ssreg_platform_cleanup(void)
{
	platform_driver_unregister(&ssreg_platform_driver);
}

subsys_initcall(ssreg_platform_init);
module_exit(ssreg_platform_cleanup);
