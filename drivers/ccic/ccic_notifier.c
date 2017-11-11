#include <linux/device.h>

#include <linux/notifier.h>
#include <linux/ccic/ccic_notifier.h>
#include <linux/sec_sysfs.h>

#define DEBUG
#define SET_CCIC_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_CCIC_NOTIFIER_BLOCK(nb)			\
		SET_CCIC_NOTIFIER_BLOCK(nb, NULL, -1)

static struct ccic_notifier_struct ccic_notifier;

struct device *ccic_device;

int ccic_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			ccic_notifier_device_t listener)
{
	int ret = 0;

	pr_info("%s: listener=%d register\n", __func__, listener);

	/* Check if CCIC Notifier is ready. */
	if (!ccic_device) {
		pr_err("%s: Not Initialized...\n", __func__);
		return -1;
	}

	SET_CCIC_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&(ccic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	/* current ccic's attached_device status notify */
	nb->notifier_call(nb, 0,
			&(ccic_notifier.ccic_template));

	return ret;
}

int ccic_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&(ccic_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_CCIC_NOTIFIER_BLOCK(nb);

	return ret;
}

static int ccic_notifier_notify(void)
{
	int ret = 0;

	CC_NOTI_TYPEDEF *p_noti =
		(CC_NOTI_TYPEDEF *)&ccic_notifier.ccic_template;

	switch (p_noti->id) {
	case CCIC_NOTIFY_ID_ATTACH:
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"attach:%02x cable_type:%02x rprd:%01x\n", __func__,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->cable_type,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->rprd);
		break;
	case CCIC_NOTIFY_ID_RID:
		pr_info("%s: src:%01x dest:%01x id:%02x rid:%02x\n", __func__,
			((CC_NOTI_RID_TYPEDEF *)p_noti)->src,
			((CC_NOTI_RID_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_RID_TYPEDEF *)p_noti)->id,
			((CC_NOTI_RID_TYPEDEF *)p_noti)->rid);
		break;
	case CCIC_NOTIFY_ID_POWER_STATUS:
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"status:%d max_voltage:%02x max_current:%02x\n", __func__,
			((CC_NOTI_PD_STATUS_TYPEDEF *)p_noti)->src,
			((CC_NOTI_PD_STATUS_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_PD_STATUS_TYPEDEF *)p_noti)->id,
			((CC_NOTI_PD_STATUS_TYPEDEF *)p_noti)->status,
			((CC_NOTI_PD_STATUS_TYPEDEF *)p_noti)->max_voltage,
			((CC_NOTI_PD_STATUS_TYPEDEF *)p_noti)->max_current);
		break;
	default:
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"sub1:%d sub2:%02x sub3:%02x\n", __func__,
			((CC_NOTI_TYPEDEF *)p_noti)->src,
			((CC_NOTI_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_TYPEDEF *)p_noti)->id,
			((CC_NOTI_TYPEDEF *)p_noti)->sub1,
			((CC_NOTI_TYPEDEF *)p_noti)->sub2,
			((CC_NOTI_TYPEDEF *)p_noti)->sub3);
		break;
	}

	ret = blocking_notifier_call_chain(&(ccic_notifier.notifier_call_chain),
			p_noti->id, &(ccic_notifier.ccic_template));


	switch (ret) {
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
		pr_err("%s: notify error occur(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s: notify done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}

void ccic_notifier_test(CC_NOTI_TYPEDEF *p_noti)
{
	/* ccic's attached_device attach broadcast */
	ccic_notifier.ccic_template = *p_noti;
	pr_info("%s:ID : %d, src : %d, dest :%d\n", __func__, p_noti->id, p_noti->src, p_noti->dest);

	ccic_notifier_notify();
}

void ccic_notifier_255K_test(void)
{
	ccic_notifier_rid_t rid_test;

	pr_err("%s:\n", __func__);
	rid_test = RID_301K;

	/* ccic's attached_device attach broadcast */
	ccic_notifier_notify();
}

static int __init ccic_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ccic_device = sec_device_create(NULL, "ccic");
	if (IS_ERR(ccic_device)) {
		pr_err("%s Failed to create device(switch)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	BLOCKING_INIT_NOTIFIER_HEAD(&(ccic_notifier.notifier_call_chain));

out:
	return ret;
}
device_initcall(ccic_notifier_init);

