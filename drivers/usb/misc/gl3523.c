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
#include <linux/workqueue.h>
#include <linux/usb_notify.h>

struct gl3523_chip {
	struct delayed_work gl3523_work;
	int host_step;

	int hub_on;
	int hub_reset;
	int hub_gang;

	struct regulator *hub_3p3;
	struct regulator *hub_1p2;
};

enum host_state {
	GL_NONE,
	GL_HOST,
	GL_HUB,
};

static int gl3523_power_on(struct gl3523_chip *chip)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	gpio_set_value(chip->hub_on, 0);
	gpio_set_value(chip->hub_reset, 0);
	msleep(1);

	ret = regulator_enable(chip->hub_3p3);
	if (ret) {
		pr_err("gl3523: failed to enable hub_3p3\n");
		return -1;
	}
	msleep(1);

	ret = regulator_enable(chip->hub_1p2);
	if (ret) {
		pr_err("gl3523: failed to enable hub_1p2\n");
		return -1;
	}
	msleep(1);

	gpio_set_value(chip->hub_on, 1);
	gpio_set_value(chip->hub_reset, 1);

	return ret;
}

static void gl3523_work_func (struct work_struct *work)
{
	struct otg_notify *o_notify = get_otg_notify();
	struct delayed_work *gl_work;
	struct gl3523_chip *chip;

	gl_work = container_of(work, struct delayed_work, work);
	chip = container_of(gl_work, struct gl3523_chip, gl3523_work);

	pr_info("%s %d\n", __func__, chip->host_step);

	switch (chip->host_step) {
	case GL_NONE:
		send_otg_notify(o_notify, NOTIFY_EVENT_HOST, 1);
		chip->host_step = GL_HOST;
		schedule_delayed_work(&chip->gl3523_work, HZ * 2);
		break;
	case GL_HOST:
		gl3523_power_on(chip);
		break;
	default:
		break;
	}
}

static int hub_request_gpio(struct device_node *np, char *name) {
	int gpio = 0;
	int ret = 0;

	gpio = of_get_named_gpio(np, name, 0);
	ret = gpio_request(gpio, name);
	if (ret) {
		pr_err("gl3523: failed to get gpio %s : %d\n", name, ret);
		return ret;
	}
	return gpio;
}	

static struct regulator *hub_request_ldo(struct device_node *np,
		char *name) {
	struct regulator *ldo = NULL;
	const char *power;
	int ret = 0;

	ret = of_property_read_string(np, name, &power);

	if (ret)
		pr_err("gl3523: failed to get %s: %d\n", name, ret);
	else {
		ldo = regulator_get(NULL, power);
		if (IS_ERR_OR_NULL(ldo)) {
			pr_err("gl3523: failed to get ldo %s\n", name);
			ldo = NULL;
		}
	}
	return ldo;
}	

static int gl3523_parse_dt(struct device *dev, struct gl3523_chip *chip)
{
	int ret = 0;
	struct device_node *np = dev->of_node;

	pr_info("%s\n", __func__);

	chip->hub_on = hub_request_gpio(np, "gl,hub_on");
	chip->hub_reset = hub_request_gpio(np, "gl,hub_reset");
	chip->hub_gang = hub_request_gpio(np, "gl,hub_gang");

	if (chip->hub_on < 0
		|| chip->hub_reset < 0
		|| chip->hub_gang < 0)
		return -1;

	chip->hub_3p3 = hub_request_ldo(np, "gl,hub_pm_3p3");
	chip->hub_1p2 = hub_request_ldo(np, "gl,hub_pm_1p2");

	if (!chip->hub_3p3 || !chip->hub_1p2)
		return -1;
	return ret;
}

static int gl3523_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct gl3523_chip *chip;

	pr_info("%s\n", __func__);
	chip = devm_kzalloc(dev, sizeof(struct gl3523_chip), GFP_KERNEL);
	if (!chip) {
		pr_err("gl3523: failed to malloc\n");
		return -ENOMEM;
	}

	chip->host_step = GL_NONE;
	ret = gl3523_parse_dt(dev, chip);
	INIT_DELAYED_WORK(&chip->gl3523_work, gl3523_work_func);
	schedule_delayed_work(&chip->gl3523_work, HZ * 10);
	platform_set_drvdata(pdev, chip);
	return ret;
}

static int gl3523_remove(struct platform_device *pdev)
{
	struct gl3523_chip *chip = platform_get_drvdata(pdev);

	pr_info("%s\n", __func__);
	regulator_put(chip->hub_3p3);
	regulator_put(chip->hub_1p2);
	return 0;
}

static const struct of_device_id gl3523_of_match[] = {
	{ .compatible = "sec,gl3523", },
	{ },
};
MODULE_DEVICE_TABLE(of, gl3523_of_match);

static struct platform_driver gl3523_device_driver = {
	.probe		= gl3523_probe,
	.remove		= gl3523_remove,
	.driver		= {
		.name	= "gl3523",
		.of_match_table = of_match_ptr(gl3523_of_match),
	}
};

static int __init gl3523_init(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&gl3523_device_driver);
}

static void __exit gl3523_exit(void)
{
	platform_driver_unregister(&gl3523_device_driver);
}

late_initcall(gl3523_init);
module_exit(gl3523_exit);
