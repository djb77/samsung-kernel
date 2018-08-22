/*
 * MST drv Support
 *
 */
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/smc.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
//#include <mach/exynos-pm.h>
#include "mstdrv.h"

#define MST_LDO3_0 "MST_LEVEL_3.0V"
#define MST_NOT_SUPPORT		(0x1 << 3)
#define MST_MODE_ON			1
#define MST_MODE_OFF		0
static int mst_pwr_en;
static bool mst_power_on;
static struct class *mst_drv_class;
struct device *mst_drv_dev;
static int escape_loop = 1;
//static int rt;
static struct wake_lock mst_wakelock;
static int wpc_det;

EXPORT_SYMBOL_GPL(mst_drv_dev);

static inline struct power_supply *get_power_supply_by_name(char *name)
{
	if (!name)
		return (struct power_supply *)NULL;
	else
		return power_supply_get_by_name(name);
}

#define psy_do_property(name, function, property, value) \
{    \
	struct power_supply *psy;    \
	int ret;    \
	psy = get_power_supply_by_name((name));    \
	if (!psy) {    \
		pr_err("%s: Fail to "#function" psy (%s)\n",    \
				__func__, (name));    \
		value.intval = 0;    \
	} else {    \
		if (psy->desc->function##_property != NULL) { \
			ret = psy->desc->function##_property(psy, (property), &(value)); \
			if (ret < 0) {    \
				pr_err("%s: Fail to %s "#function" (%d=>%d)\n", \
						__func__, name, (property), ret);    \
				value.intval = 0;    \
			}    \
		}    \
	}    \
}

static void of_mst_hw_onoff(bool on)
{
	union power_supply_propval value;	/* power_supply prop */
	struct regulator *regulator3_0;
	int ret;
	int retry_cnt = 8;

	regulator3_0 = regulator_get(NULL, MST_LDO3_0);
	if (IS_ERR(regulator3_0)) {
		printk("%s : regulator 3.0 is not available\n", __func__);
		return;
	}

	if (mst_power_on == on) {
		printk("mst-drv : mst_power_onoff : already %d\n", on);
		regulator_put(regulator3_0);
		return;
	}

	mst_power_on = on;

	printk("mst-drv : mst_power_onoff : %d\n", on);

	if (regulator3_0 == NULL) {
		printk(KERN_ERR "%s: regulator3_0 is invalid(NULL)\n",
		       __func__);
		return;
	}

	if (on) {
		ret = regulator_enable(regulator3_0);
		if (ret < 0) {
			printk("%s : regulator 3.0 is not enable\n", __func__);
		}
		printk("%s : regulator 3.0 is enabled\n", __func__);

		printk("%s : MST_MODE_ON notify start\n", __func__);
		value.intval = MST_MODE_ON;

		psy_do_property("mfc-charger", set,
				POWER_SUPPLY_PROP_TECHNOLOGY, value);
		printk("%s : MST_MODE_ON notified : %d\n", __func__,
		       value.intval);

		gpio_set_value(mst_pwr_en, 1);
		usleep_range(3600, 4000);
		gpio_set_value(mst_pwr_en, 0);
		mdelay(50);

		gpio_set_value(mst_pwr_en, 1);
		printk("%s : mst_pwr_en gets the HIGH\n", __func__);

		mdelay(40);

		while (--retry_cnt) {
			psy_do_property("mfc-charger", get,
					POWER_SUPPLY_PROP_TECHNOLOGY, value);
			printk("%s : check mst mode status : %d\n", __func__, value.intval);
			if (value.intval == 0x02) {
				printk("%s : mst mode set!!! : %d\n", __func__,
				       value.intval);
				retry_cnt = 1;
				break;
			}
			usleep_range(3600, 4000);
		}

		if (!retry_cnt) {
			printk("%s : timeout !!! : %d\n", __func__,
			       value.intval);
		}

	} else {
		gpio_set_value(mst_pwr_en, 0);
		printk("%s : mst_pwr_en gets the LOW\n", __func__);

		usleep_range(800, 1000);
		printk("%s : msleep(1)\n", __func__);

		value.intval = MST_MODE_OFF;
		psy_do_property("mfc-charger", set,
				POWER_SUPPLY_PROP_TECHNOLOGY, value);
		printk("%s : MST_MODE_OFF notify : %d\n", __func__,
		       value.intval);

		ret = regulator_disable(regulator3_0);
		if (ret < 0) {
			printk("%s : regulator 3.0 is not disabled\n",
			       __func__);
		}
		printk("%s : regulator 3.0 is disabled\n", __func__);
	}

	regulator_put(regulator3_0);
}

static ssize_t show_mst_drv(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	if (!dev)
		return -ENODEV;
	// todo
	if (escape_loop == 0) {
		return sprintf(buf, "%s\n", "activating");
	} else {
		return sprintf(buf, "%s\n", "waiting");
	}
}

static ssize_t store_mst_drv(struct device *dev,
			     struct device_attribute *attr, const char *buf,
			     size_t count)
{
	u64 r0 = 0, r1 = 0, r2 = 0, r3 = 0;
	char test_result[256] = { 0, };
	int result = 0;

	sscanf(buf, "%20s\n", test_result);
	printk(KERN_ERR "MST Store test result : %s\n", test_result);

	if (wpc_det && (gpio_get_value(wpc_det) == 1)) {
		printk("[MST] Wireless charging is ongoing, do not proceed MST work\n");
		return count;
	}

	switch (test_result[0]) {
	case '1':
		of_mst_hw_onoff(1);
		break;

	case '0':
		of_mst_hw_onoff(0);
		break;

	case '2':
		//of_mst_hw_onoff(1);
		printk(KERN_INFO "%s\n", __func__);
		printk(KERN_INFO "MST_LDO_DRV]]] Track1 data transmit\n");
		//Will Add here
		r0 = (0x8300000f);
		r1 = 1;
		result = exynos_smc(r0, r1, r2, r3);
		printk(KERN_INFO "MST_LDO_DRV]]] Track1 data sent : %d\n",
		       result);
		//of_mst_hw_onoff(0);
		break;

	case '3':
		//of_mst_hw_onoff(1);
		printk(KERN_INFO "%s\n", __func__);
		printk(KERN_INFO "MST_LDO_DRV]]] Track2 data transmit\n");
		//Will Add here
		r0 = (0x8300000f);
		r1 = 2;
		result = exynos_smc(r0, r1, r2, r3);
		printk(KERN_INFO "MST_LDO_DRV]]] Track2 data sent : %d\n",
		       result);
		//of_mst_hw_onoff(0);
		break;

	case '4':
		if (escape_loop) {
			wake_lock_init(&mst_wakelock, WAKE_LOCK_SUSPEND,
				       "mst_wakelock");
			wake_lock(&mst_wakelock);
		}
		escape_loop = 0;
		while (1) {
			if (escape_loop == 1)
				break;
			of_mst_hw_onoff(1);
			mdelay(10);
			printk
			    ("MST_LDO_DRV]]] Track2 data transmit to infinity until stop button pushed\n");
			r0 = (0x8300000f);
			r1 = 2;
			result = exynos_smc(r0, r1, r2, r3);
			printk(KERN_INFO
			       "MST_LDO_DRV]]] Track2 data transmit to infinity after smc : %d\n",
			       result);
			of_mst_hw_onoff(0);
			mdelay(1000);
		}
		break;

	case '5':
		if (!escape_loop)
			wake_lock_destroy(&mst_wakelock);
		escape_loop = 1;
		printk("MST escape_loop value = 1\n");
		break;

	default:
		printk(KERN_ERR "MST invalid value : %s\n", test_result);
		break;
	}
	return count;
}

static DEVICE_ATTR(transmit, 0770, show_mst_drv, store_mst_drv);

#if defined(CONFIG_MFC_CHARGER)
static ssize_t show_mfc(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	if (!dev)
		return -ENODEV;

	return sprintf(buf, "%s\n", "mfc_charger");
}

static ssize_t store_mfc(struct device *dev,
			 struct device_attribute *attr, const char *buf,
			 size_t count)
{
	return count;
}

static DEVICE_ATTR(mfc, 0770, show_mfc, store_mfc);
#endif

static int sec_mst_gpio_init(struct device *dev)
{
	int ret = 0;
	struct device_node *np;
	enum of_gpio_flags irq_gpio_flags;

	/* get wireless chraging check gpio */
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
	} else {
		/* wpc_det */
		wpc_det = of_get_named_gpio_flags(np, "battery,wpc_det",
			0, &irq_gpio_flags);
		if (ret < 0) {
			dev_err(dev, "%s : can't get wpc_det\r\n", __FUNCTION__);
		}
	}

	mst_pwr_en = of_get_named_gpio(dev->of_node, "sec-mst,mst-pwr-gpio", 0);

	printk("[MST] Data Value : %d\n", mst_pwr_en);

	/* check if gpio pin is inited */
	if (mst_pwr_en < 0) {
		printk(KERN_ERR "%s : Cannot create the gpio\n", __func__);
		return 1;
	}
	printk(KERN_ERR "MST_DRV]]] gpio pwr en inited\n");

	/* gpio request */
	ret = gpio_request(mst_pwr_en, "sec-mst,mst-pwr-gpio");
	if (ret) {
		printk(KERN_ERR "[MST] failed to get en gpio : %d, %d\n", ret,
		       mst_pwr_en);
	}

	/* set gpio direction */
	if (!(ret < 0) && (mst_pwr_en > 0)) {
		gpio_direction_output(mst_pwr_en, 0);
		printk(KERN_ERR "%s : Send Output\n", __func__);
	}

	return 0;
}

static int mst_ldo_device_probe(struct platform_device *pdev)
{
	int retval = 0;

	printk("%s init start\n", __func__);

	if (sec_mst_gpio_init(&pdev->dev))
		return -1;

	printk(KERN_ALERT "%s\n", __func__);

	mst_drv_class = class_create(THIS_MODULE, "mstldo");
	if (IS_ERR(mst_drv_class)) {
		retval = PTR_ERR(mst_drv_class);
		goto error;
	}

	mst_drv_dev = device_create(mst_drv_class,
				    NULL /* parent */, 0 /* dev_t */,
				    NULL /* drvdata */,
				    MST_DRV_DEV);
	if (IS_ERR(mst_drv_dev)) {
		retval = PTR_ERR(mst_drv_dev);
		goto error_destroy;
	}

	/* register this mst device with the driver core */
	retval = device_create_file(mst_drv_dev, &dev_attr_transmit);
	if (retval)
		goto error_destroy;

#if defined(CONFIG_MFC_CHARGER)
	retval = device_create_file(mst_drv_dev, &dev_attr_mfc);
	if (retval)
		goto error_destroy;
#endif

	printk(KERN_DEBUG "MST drv driver (%s) is initialized.\n", MST_DRV_DEV);
	return 0;

error_destroy:
	kfree(mst_drv_dev);
	device_destroy(mst_drv_class, 0);
error:
	printk(KERN_ERR "%s: MST drv driver initialization failed\n", __FILE__);
	return retval;
}

static struct of_device_id mst_match_ldo_table[] = {
	{.compatible = "sec-mst",},
	{},
};

#if 0
static int mst_ldo_device_suspend(struct platform_device *dev,
				  pm_message_t state)
{
	u64 r0 = 0, r1 = 0, r2 = 0, r3 = 0;
	int result = 0;

	printk(KERN_INFO "%s\n", __func__);
	printk(KERN_INFO "MST_LDO_DRV]]] suspend");
	//Will Add here
	r0 = (0x8300000c);
	result = exynos_smc(r0, r1, r2, r3);
	if (result == MST_NOT_SUPPORT) {
		printk(KERN_INFO
		       "MST_LDO_DRV]]] suspend do nothing after smc : %x\n",
		       result);
	} else {
		printk(KERN_INFO
		       "MST_LDO_DRV]]] suspend success after smc : %x\n",
		       result);
	}

	return 0;
}

static int mst_ldo_device_resume(struct platform_device *dev)
{
	u64 r0 = 0, r1 = 0, r2 = 0, r3 = 0;
	int result = 0;

	printk(KERN_INFO "%s\n", __func__);
	printk(KERN_INFO "MST_LDO_DRV]]] resume");
	//Will Add here
	r0 = (0x8300000d);
	result = exynos_smc(r0, r1, r2, r3);
	if (result == MST_NOT_SUPPORT) {
		printk(KERN_INFO
		       "MST_LDO_DRV]]] resume do nothing after smc : %x\n",
		       result);
	} else {
		printk(KERN_INFO
		       "MST_LDO_DRV]]] resume success after smc : %x\n",
		       result);
	}

	rt = result;
	return 0;
}
#endif

static struct platform_driver sec_mst_ldo_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "mstldo",
		   .of_match_table = mst_match_ldo_table,
		   },
	.probe = mst_ldo_device_probe,
	//.suspend = mst_ldo_device_suspend,
	//.resume = mst_ldo_device_resume,
};

static int __init mst_drv_init(void)
{
	int ret = 0;
	printk(KERN_ERR "%s\n", __func__);

	ret = platform_driver_register(&sec_mst_ldo_driver);
	printk(KERN_ERR "MST_LDO_DRV]]] init , ret val : %d\n", ret);
	return ret;
}

static void __exit mst_drv_exit(void)
{
	class_destroy(mst_drv_class);
	printk(KERN_ALERT "%s\n", __func__);
}

MODULE_AUTHOR("JASON KANG, j_seok.kang@samsung.com");
MODULE_DESCRIPTION("MST drv driver");
MODULE_VERSION("0.1");
module_init(mst_drv_init);
module_exit(mst_drv_exit);
