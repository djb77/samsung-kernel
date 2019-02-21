/*
 * combo redriver for ptn36502.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/sec_sysfs.h>
#include <linux/combo_redriver/ptn36502.h>

/*#define PTN36502_DEBUG*/

#ifdef PTN36502_DEBUG
#define i2c_smbus_write_byte_data(fmt, ...)						\
	do {										\
		i2c_smbus_write_byte_data(fmt, ##__VA_ARGS__);				\
		pr_info("%s :: address : 0x%x value: 0x%x \n", __func__, ##__VA_ARGS__);	\
	} while (0)
#endif

struct ptn36502_data *redrv_data;
struct device *combo_redriver_device;

void ptn36502_config(int config, int is_DFP)
{
	int is_front = 0;
	u8 value;
	static int config_bak = INIT_MODE, is_DFP_bak;

	if (!redrv_data) {
		pr_err("redrv_data is NULL (config=%d, is_DFP=%d)\n", config, is_DFP);
		config_bak = config;
		is_DFP_bak = is_DFP;
		return;
	}

	is_front = !gpio_get_value(redrv_data->con_sel);
	pr_info("%s: config(%d)(%s)(%s)\n", __func__, config, (is_DFP ? "DFP":"UFP"), (is_front ? "FRONT":"REAR"));

	switch (config) {
	case BOOTING_INIT:
	case INIT_MODE:
		i2c_smbus_write_byte_data(redrv_data->i2c, Device_Control, 0x81);//chip reset
		i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, 0x48);
		i2c_smbus_write_byte_data(redrv_data->i2c, DP_Link_Control, 0x0e);
		i2c_smbus_write_byte_data(redrv_data->i2c, USB_TXRX_Control, redrv_data->usbControl_US.data);
		pr_info("%s: usbControl_US as (%x) \n", __func__, redrv_data->usbControl_US.data);
		i2c_smbus_write_byte_data(redrv_data->i2c, DS_TXRX_Control, redrv_data->usbControl_DS.data);
		i2c_smbus_write_byte_data(redrv_data->i2c, DP_Lane0_Control, 0x29);
		i2c_smbus_write_byte_data(redrv_data->i2c, DP_Lane1_Control, 0x29);
		i2c_smbus_write_byte_data(redrv_data->i2c, DP_Lane2_Control, 0x29);
		i2c_smbus_write_byte_data(redrv_data->i2c, DP_Lane3_Control, 0x29);

		if (config == BOOTING_INIT && config_bak != INIT_MODE)
			ptn36502_config(config_bak, is_DFP_bak);

		break;

	case USB3_ONLY_MODE:
		if (is_DFP)
			i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x61:0x41);
		else
			i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x81:0xa1);

		value = i2c_smbus_read_byte_data(redrv_data->i2c, Mode_Control);
		pr_info("%s: read 0x0b command as (%x) \n", __func__, value);
		break;

	case DP2_LANE_USB3_MODE:
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x7f, 0xd2);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x7f, 0x9f);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x7f, 0x1f);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x6e, is_front ? 0xd9:0xc7);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0xff, 0x00);
		if (is_DFP)
			i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x4a:0x6a);
		else
			i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x8a:0xaa);
		break;

	case DP4_LANE_MODE:
		i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x68:0x48);
		i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x6b:0x4b);
		
#if 0
		if (is_DFP)
			i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x48:0x68);
		else
			i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x88:0xa8);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x7f, 0xd2);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x7f, 0x9f);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x7f, 0x1f);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0x6e, is_front ? 0xd9:0xc7);
		i2c_smbus_write_byte_data(redrv_data->i2c, 0xff, 0x00);
		if (is_DFP)
			i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x4b:0x6b);
		else
			i2c_smbus_write_byte_data(redrv_data->i2c, Mode_Control, is_front ? 0x8b:0xab);
#endif
		break;
	}
}
EXPORT_SYMBOL(ptn36502_config);


static int ptn36502_set_gpios(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret = 0;
	redrv_data->combo_redriver_en = -1;
	redrv_data->redriver_en = -1;
	redrv_data->con_sel = 0;

	redrv_data->combo_redriver_en = of_get_named_gpio(np, "combo,ptn_en", 0);
	redrv_data->redriver_en = of_get_named_gpio(np, "combo,redriver_en", 0);
	redrv_data->con_sel = of_get_named_gpio(np, "combo,con_sel", 0);

	np = of_find_all_nodes(NULL);

	if (gpio_is_valid(redrv_data->combo_redriver_en)) {
		ret = gpio_request(redrv_data->combo_redriver_en, "ptn36502_en");
		pr_info("%s: combo_redriver_en ret gpio_request (%d)\n", __func__, ret);
		ret = gpio_direction_input(redrv_data->combo_redriver_en);
		pr_info("%s: combo_redriver_en ret gpio_direction_input (%d)\n", __func__, ret);
	} else
		pr_info("%s: combo_redriver_en gpio is invalid\n", __func__);

	msleep(1000);

	if (gpio_is_valid(redrv_data->redriver_en)) {
		ret = gpio_request(redrv_data->redriver_en, "ap7341d_redriver_en");
		pr_info("%s: ap7341d_redriver_en ret gpio_request (%d)\n", __func__, ret);
		ret = gpio_direction_output(redrv_data->redriver_en, 1);
		pr_info("%s: ap7341d_redriver_en ret gpio_direction_output (%d)\n", __func__, ret);
	} else
		pr_info("%s: ap7341d_redriver_en gpio is invalid\n", __func__);

	msleep(1000);

	pr_info("%s: after set values (%d) (%d)\n",
		__func__, gpio_get_value(redrv_data->combo_redriver_en), gpio_get_value(redrv_data->redriver_en));

	return 0;
}

static void init_usb_control(void)
{
	redrv_data->usbControl_US.BITS.RxEq = 2;
	redrv_data->usbControl_US.BITS.Swing = 1;
	redrv_data->usbControl_US.BITS.DeEmpha = 1;
	pr_info("%s: usbControl_US (%x)\n", __func__, redrv_data->usbControl_US.data);

	redrv_data->usbControl_DS.BITS.RxEq = 2;
	redrv_data->usbControl_DS.BITS.Swing = 1;
	redrv_data->usbControl_DS.BITS.DeEmpha = 1;
	pr_info("%s: usbControl_DS (%x)\n", __func__, redrv_data->usbControl_DS.data);
}

static ssize_t ptn_us_tune_show(struct device *dev,
		struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "US: 0x%x 0x%x 0x%x Total(0x%x)\n",
		       redrv_data->usbControl_US.BITS.RxEq,
		       redrv_data->usbControl_US.BITS.Swing,
		       redrv_data->usbControl_US.BITS.DeEmpha,
		       redrv_data->usbControl_US.data
		       );
}

static ssize_t ptn_us_tune_store(struct device *dev,
		struct device_attribute *attr, const char *buff, size_t size)
{

	char *name;
	char *field;
	char *value;
	int	tmp = 0;
	char buf[256], *b, *c;

	printk(KERN_DEBUG "%s buff=%s\n", __func__, buff);
	strlcpy(buf, buff, sizeof(buf));
	b = strim(buf);

	while (b) {
		name = strsep(&b, ",");
		if (!name)
			continue;

		c =  strim(name);

		field = strsep(&c, "=");
		value = strsep(&c, "=");
		printk(KERN_DEBUG "usb: %s field=%s  value=%s\n", __func__, field, value);

		if (!strcmp(field, "eq")) {
			sscanf(value, "%d\n", &tmp);
			redrv_data->usbControl_US.BITS.RxEq = tmp;
			printk(KERN_DEBUG "RxEq value=%d\n", redrv_data->usbControl_US.BITS.RxEq);
			tmp = 0;
		} else if (!strcmp(field, "swing")) {
			sscanf(value, "%d\n", &tmp);
			redrv_data->usbControl_US.BITS.Swing = tmp;
			printk(KERN_DEBUG "Swing value=%d\n", redrv_data->usbControl_US.BITS.Swing);
			tmp = 0;
		} else if (!strcmp(field, "emp")) {
			sscanf(value, "%d\n", &tmp);
			redrv_data->usbControl_US.BITS.DeEmpha = tmp;
			printk(KERN_DEBUG "DeEmpha value=%d\n", redrv_data->usbControl_US.BITS.DeEmpha);
			tmp = 0;
		}
	}
	return size;
}

static DEVICE_ATTR(ptn_us_tune, S_IRUGO | S_IWUSR, ptn_us_tune_show,
					       ptn_us_tune_store);

static ssize_t ptn_ds_tune_show(struct device *dev,
		struct device_attribute *attr, char *buff)
{
	return sprintf(buff, "DS : 0x%x 0x%x 0x%x Total(0x%x)\n",
		       redrv_data->usbControl_DS.BITS.RxEq,
		       redrv_data->usbControl_DS.BITS.Swing,
		       redrv_data->usbControl_DS.BITS.DeEmpha,
		       redrv_data->usbControl_DS.data
		       );
}

static ssize_t ptn_ds_tune_store(struct device *dev,
		struct device_attribute *attr, const char *buff, size_t size)
{

	char *name;
	char *field;
	char *value;
	int	tmp = 0;
	char buf[256], *b, *c;

	printk(KERN_DEBUG "%s buff=%s\n", __func__, buff);
	strlcpy(buf, buff, sizeof(buf));
	b = strim(buf);

	while (b) {
		name = strsep(&b, ",");
		if (!name)
			continue;

		c =  strim(name);

		field = strsep(&c, "=");
		value = strsep(&c, "=");
		printk(KERN_DEBUG "usb: %s field=%s  value=%s\n", __func__, field, value);

		if (!strcmp(field, "eq")) {
			sscanf(value, "%d\n", &tmp);
			redrv_data->usbControl_DS.BITS.RxEq = tmp;
			printk(KERN_DEBUG "RxEq value=%d\n", redrv_data->usbControl_DS.BITS.RxEq);
			tmp = 0;
		} else if (!strcmp(field, "swing")) {
			sscanf(value, "%d\n", &tmp);
			redrv_data->usbControl_DS.BITS.Swing = tmp;
			printk(KERN_DEBUG "Swing value=%d\n", redrv_data->usbControl_DS.BITS.Swing);
			tmp = 0;
		} else if (!strcmp(field, "emp")) {
			sscanf(value, "%d\n", &tmp);
			redrv_data->usbControl_DS.BITS.DeEmpha = tmp;
			printk(KERN_DEBUG "DeEmpha value=%d\n", redrv_data->usbControl_DS.BITS.DeEmpha);
			tmp = 0;
		}
	}
	return size;
}

static DEVICE_ATTR(ptn_ds_tune, S_IRUGO | S_IWUSR, ptn_ds_tune_show,
					       ptn_ds_tune_store);

static int ptn36502_probe(struct i2c_client *i2c,
			       const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(i2c->dev.parent);
	int ret = 0;

	pr_info("%s\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&i2c->dev, "i2c functionality check error\n");
		return -EIO;
	}

	redrv_data = devm_kzalloc(&i2c->dev, sizeof(*redrv_data), GFP_KERNEL);

	if (!redrv_data) {
		dev_err(&i2c->dev, "Failed to allocate driver data\n");
		return -ENOMEM;
	}

	if (i2c->dev.of_node)
		ptn36502_set_gpios(&i2c->dev);
	else
		dev_err(&i2c->dev, "not found ptn36502 dt ret:%d\n", ret);

	redrv_data->dev = &i2c->dev;
	redrv_data->i2c = i2c;
	i2c_set_clientdata(i2c, redrv_data);

	mutex_init(&redrv_data->i2c_mutex);

	init_usb_control();
	ptn36502_config(BOOTING_INIT, 0);

	return ret;
}

static int ptn36502_remove(struct i2c_client *i2c)
{
	struct ptn36502_data *redrv_data = i2c_get_clientdata(i2c);

	if (redrv_data->i2c) {
		mutex_destroy(&redrv_data->i2c_mutex);
		i2c_set_clientdata(redrv_data->i2c, NULL);
	}
	return 0;
}


static void ptn36502_shutdown(struct i2c_client *i2c)
{
	;
}

#define PTN36502_DEV_NAME  "ptn36502"


static const struct i2c_device_id ptn36502_id[] = {
	{ PTN36502_DEV_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ptn36502_id);


static struct of_device_id ptn36502_i2c_dt_ids[] = {
	{ .compatible = "ptn36502_driver" },
	{ }
};


static struct i2c_driver ptn36502_driver = {
	.driver = {
		.name	= PTN36502_DEV_NAME,
		.of_match_table	= ptn36502_i2c_dt_ids,
	},
	.probe		= ptn36502_probe,
	.remove		= ptn36502_remove,
	.shutdown	= ptn36502_shutdown,
	.id_table	= ptn36502_id,
};

static struct attribute *ptn36502_attributes[] = {
	&dev_attr_ptn_us_tune.attr,
	&dev_attr_ptn_ds_tune.attr,
	NULL
};

const struct attribute_group ptn36502_sysfs_group = {
	.attrs = ptn36502_attributes,
};

static int __init ptn36502_init(void)
{
	int ret = 0;
	ret = i2c_add_driver(&ptn36502_driver);
	pr_info("%s ret is %d\n", __func__, ret);

	combo_redriver_device = sec_device_create(NULL, "combo");
	if (IS_ERR(combo_redriver_device))
		pr_err("%s Failed to create device(combo)!\n", __func__);

	ret = sysfs_create_group(&combo_redriver_device->kobj, &ptn36502_sysfs_group);
	if (ret)
		pr_err("%s: sysfs_create_group fail, ret %d", __func__, ret);

	return ret;
}
module_init(ptn36502_init);

static void __exit ptn36502_exit(void)
{
	i2c_del_driver(&ptn36502_driver);
}
module_exit(ptn36502_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ptn36502 combo redriver driver");
MODULE_AUTHOR("lucky29.park <lucky29.park@samsung.com>");


