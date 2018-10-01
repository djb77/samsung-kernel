#include <linux/device.h>

#include <linux/notifier.h>
#include <linux/ccic/ccic_notifier.h>
#include <linux/sec_sysfs.h>
#include <linux/ccic/ccic_sysfs.h>
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
#include <linux/battery/battery_notifier.h>
#endif
#include <linux/usb_notify.h>

#define DEBUG
#define SET_CCIC_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_CCIC_NOTIFIER_BLOCK(nb)			\
		SET_CCIC_NOTIFIER_BLOCK(nb, NULL, -1)

static struct ccic_notifier_struct ccic_notifier;

struct device *ccic_device;

char CCIC_NOTI_DEST_Print[8][10] =
{
    {"INITIAL"},
    {"USB"},
    {"BATTERY"},
    {"PDIC"},
    {"MUIC"},
    {"CCIC"},
    {"MANAGER"},
    {"ALL"},
};

char CCIC_NOTI_ID_Print[7][20] =
{
    {"ID_INITIAL"},
    {"ID_ATTACH"},
    {"ID_RID"},
    {"ID_USB"},
    {"ID_POWER_STATUS"},
    {"ID_WATER"},
    {"ID_VCONN"},
};

char CCIC_NOTI_RID_Print[8][15] =
{
    {"RID_UNDEFINED"},
    {"RID_000K"},
    {"RID_001K"},
    {"RID_255K"},
    {"RID_301K"},
    {"RID_523K"},
    {"RID_619K"},
    {"RID_OPEN"},
};

char CCIC_NOTI_USB_STATUS_Print[5][20] =
{
    {"USB_DETACH"},
    {"USB_ATTACH_DFP"},
    {"USB_ATTACH_UFP"},
    {"USB_ATTACH_DRP"},
    {"USB_ATTACH_NO_USB"},
};

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

static void ccic_uevent_work(int id)
{
	char *water[2] = { "CCIC=WATER", NULL };
	char *vconn[2] = { "CCIC=VCONN", NULL };

	pr_info("usb: %s: id=%s \n", __func__, CCIC_NOTI_ID_Print[id]);
	if (id == CCIC_NOTIFY_ID_WATER) {
		kobject_uevent_env(&ccic_device->kobj, KOBJ_CHANGE, water);
	} else if (id == CCIC_NOTIFY_ID_VCONN) {
		kobject_uevent_env(&ccic_device->kobj, KOBJ_CHANGE, vconn);
	}
}

/* ccic's attached_device attach broadcast */
int ccic_notifier_notify(CC_NOTI_TYPEDEF *p_noti, void *pd, int pdic_attach)
{
	int ret = 0;
	ccic_notifier.ccic_template = *p_noti;

	switch (p_noti->id) {
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	case CCIC_NOTIFY_ID_POWER_STATUS:		// PDIC_NOTIFY_EVENT_PD_SINK
		pr_info("%s: src:%01x dest:%01x id:%02x "
			"attach:%02x cable_type:%02x rprd:%01x\n", __func__,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->src,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->dest,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->id,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->cable_type,
			((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->rprd);

		if (pd != NULL) {
			if(((CC_NOTI_ATTACH_TYPEDEF *)p_noti)->attach) {
				((struct pdic_notifier_struct *)pd)->event = PDIC_NOTIFY_EVENT_PD_SINK;
			} else {
				if (((struct pdic_notifier_struct *)pd)->event != PDIC_NOTIFY_EVENT_CCIC_ATTACH)
					((struct pdic_notifier_struct *)pd)->event = PDIC_NOTIFY_EVENT_DETACH;
			}
			ccic_notifier.ccic_template.pd = pd;

			pr_info("%s: PD event:%d, num:%d, sel:%d \n", __func__,
				((struct pdic_notifier_struct *)pd)->event,
				((struct pdic_notifier_struct *)pd)->sink_status.available_pdo_num,
				((struct pdic_notifier_struct *)pd)->sink_status.selected_pdo_num);
		}
		break;
#endif
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
	case CCIC_NOTIFY_ID_WATER:
		ccic_uevent_work(CCIC_NOTIFY_ID_WATER);
		break;
	case CCIC_NOTIFY_ID_VCONN:
		ccic_uevent_work(CCIC_NOTIFY_ID_VCONN);
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
	store_usblog_notify(NOTIFY_CCIC_EVENT, (void*)p_noti , NULL);
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

#if 0
void ccic_notifier_255K_test(void)
{
	ccic_notifier_rid_t rid_test;

	pr_err("%s:\n", __func__);
	rid_test = RID_301K;

	/* ccic's attached_device attach broadcast */
	ccic_notifier_notify();
}
#endif

int ccic_notifier_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ccic_device = sec_device_create(NULL, "ccic");
	if (IS_ERR(ccic_device)) {
		pr_err("%s Failed to create device(switch)!\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	/* create sysfs group */
	ret = sysfs_create_group(&ccic_device->kobj, &ccic_sysfs_group);
	if (ret) {
		pr_err("%s: ccic sysfs fail, ret %d", __func__, ret);
		goto out;
	}


	BLOCKING_INIT_NOTIFIER_HEAD(&(ccic_notifier.notifier_call_chain));

out:
	return ret;
}
//device_initcall(ccic_notifier_init);

