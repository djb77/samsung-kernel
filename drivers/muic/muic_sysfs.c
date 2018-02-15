#include <linux/device.h>
#include <linux/err.h>

static struct class *muic_class;
static atomic_t muic_dev;

static int __init muic_class_create(void)
{
	muic_class = class_create(THIS_MODULE, "sec");
	if (IS_ERR(muic_class)) {
		pr_err("Failed to create class(muic) %ld\n", PTR_ERR(muic_class));
		return PTR_ERR(muic_class);
	}
	return 0;
}

struct device *muic_device_create(void *drvdata, const char *fmt)
{
	struct device *dev;

	if (IS_ERR(muic_class)) {
		pr_err("Failed to create class(muic) %ld\n", PTR_ERR(muic_class));
		BUG();
	}

	if (!muic_class) {
		pr_err("Not yet created class(muic)!\n");
		BUG();
	}

	dev = device_create(muic_class, NULL, atomic_inc_return(&muic_dev),
			drvdata, fmt);
	if (IS_ERR(dev))
		pr_err("Failed to create device %s %ld\n", fmt, PTR_ERR(dev));
	else
		pr_debug("%s : %s : %d\n", __func__, fmt, dev->devt);

	return dev;
}
EXPORT_SYMBOL(muic_device_create);

void muic_device_destroy(dev_t devt)
{
	pr_info("%s : %d\n", __func__, devt);
	device_destroy(muic_class, devt);
}
EXPORT_SYMBOL(muic_device_destroy);

subsys_initcall(muic_class_create);
