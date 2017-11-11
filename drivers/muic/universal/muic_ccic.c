/*
 * muic_ccic.c
 *
 * Copyright (C) 2014 Samsung Electronics
 * Thomas Ryu <smilesr.ryu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/host_notify.h>
#include <linux/string.h>
#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif

#include <linux/muic/muic.h>
#if defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic_notifier.h>
#endif
#include "muic-internal.h"
#include "muic_apis.h"

#if defined(CONFIG_MUIC_SUPPORT_CCIC)
#include <linux/ccic/ccic_notifier.h>
#endif

struct mdev_rid_desc_t {
	char *name;
	int mdev;
};

static struct mdev_desc_t {
	int ccic_evt_attached; /* 1: attached, 0: detached */
	int ccic_evt_rid; /* the last rid */

	int mdev; /* attached dev */
}mdev_desc;

static int __ccic_info = 0;

static struct mdev_rid_desc_t mdev_rid_tbl[] = {
	[RID_UNDEFINED] = {"UNDEFINED", ATTACHED_DEV_NONE_MUIC},
	[RID_000K] = {"000K", ATTACHED_DEV_OTG_MUIC},
	[RID_001K] = {"001K", ATTACHED_DEV_MHL_MUIC},
	[RID_255K] = {"255K", ATTACHED_DEV_JIG_USB_OFF_MUIC},
	[RID_301K] = {"301K", ATTACHED_DEV_JIG_USB_ON_MUIC},
	[RID_523K] = {"523K", ATTACHED_DEV_JIG_UART_OFF_MUIC},
	[RID_619K] = {"619K", ATTACHED_DEV_JIG_UART_ON_MUIC},
	[RID_OPEN] = {"OPEN", ATTACHED_DEV_NONE_MUIC},
};

/*
 * __ccic_info :
 * b'0: 1 if an active ccic is present,
 *        0 when muic works without ccic chip or
 *              no ccic Noti. registration is needed
 *              even though a ccic chip is present.
 */
static int set_ccic_info(char *str)
{
	get_option(&str, &__ccic_info);

	pr_info("%s: ccic_info: 0x%04x\n", __func__, __ccic_info);

	return __ccic_info;
}
__setup("ccic_info=", set_ccic_info);

int get_ccic_info(void)
{
	return __ccic_info;
}

int muic_is_ccic_supported_jig(muic_data_t *pmuic, muic_attached_dev_t mdev)
{
	switch (mdev) {
	/* JIG */
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
		pr_info("%s: Supported JIG(%d).\n", __func__, mdev);
		return 1;
	default:
		pr_info("%s: mdev:%d Unsupported.\n", __func__, mdev);
	}

	return 0;
}

int muic_is_ccic_supported_dev(muic_data_t *pmuic, muic_attached_dev_t new_dev)
{
	switch (new_dev) {
	/* Legacy TA/USB. Noti. will be sent when ATTACH is received from CCIC. */
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_TA_MUIC:
		return 1;
	default:
		break;
	}

	return 0;
}

static bool mdev_is_supported(muic_data_t *pmuic, int mdev)
{
	switch (mdev) {
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_TA_MUIC:
		return true;
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		return true;
	default:
		break;
	}

	return false;
}

static int mdev_com_to(muic_data_t *pmuic, int path)
{
	switch (path) {
	case MUIC_PATH_OPEN:
		com_to_open_with_vbus(pmuic);
		break;

	case MUIC_PATH_USB_AP:
	case MUIC_PATH_USB_CP:
		switch_to_ap_usb(pmuic);
		break;
	case MUIC_PATH_UART_AP:
	case MUIC_PATH_UART_CP:
		if (pmuic->pdata->uart_path == MUIC_PATH_UART_AP)
			switch_to_ap_uart(pmuic);
		else
			switch_to_cp_uart(pmuic);
		break;

	default:
		pr_err("%s:A wrong com path!\n", __func__);
		return -1;
	}

	return 0;
}

static int mdev_get_vbus(muic_data_t *pmuic)
{
	return pmuic->vps.t.vbvolt;
}

int mdev_continue_for_TA_USB(muic_data_t *pmuic, int mdev)
{
	struct mdev_desc_t *pdesc = &mdev_desc;

	if (!muic_is_ccic_supported_dev(pmuic, mdev)) {
		pr_info("%s:%s: NOT supported(%d).\n", __func__, MUIC_DEV_NAME, mdev);
		return 0;
	}

	pmuic->legacy_dev = mdev;
	pr_info("%s:%s: A legacy TA or USB updated(%d).\n",
				MUIC_DEV_NAME,__func__, mdev);

	/* Some delay for CCIC's Noti. When VBUS comes in to MUIC */
	msleep(200);

	if (!muic_is_ccic_supported_dev(pmuic, pmuic->legacy_dev)) {
		pr_info("%s:%s: Legacy Device Updated to %d.\n",
				MUIC_DEV_NAME,__func__, pmuic->legacy_dev);
		return 0;
	}

	/* We will go even though CCIC is not working until it becomes stable. */
	if (!pdesc->ccic_evt_attached)
		pr_info("%s:%s: NOT attached.\n", MUIC_DEV_NAME,__func__);

	return 1;
}

static void mdev_show_status(muic_data_t *pmuic)
{
	struct mdev_desc_t *pdesc = &mdev_desc;

	pr_info("%s: mdev:%d rid:%d attached:%d legacy_dev:%d\n", __func__,
			pdesc->mdev, pdesc->ccic_evt_rid, pdesc->ccic_evt_attached,
			pmuic->legacy_dev);
}

int mdev_noti_attached(muic_data_t *pmuic, int mdev)
{
	muic_notifier_attach_attached_dev(mdev);
	return 0;
}

int mdev_noti_detached(muic_data_t *pmuic, int mdev)
{
	muic_notifier_detach_attached_dev(mdev);
	return 0;
}

/* Get the charger type from muic interrupt or by reading the register directly */
static int muic_get_chgtyp_to_mdev(muic_data_t *pmuic)
{
	return pmuic->legacy_dev;
}


static int mdev_handle_legacy_TA_USB(muic_data_t *pmuic)
{
	struct mdev_desc_t *pdesc = &mdev_desc;
	int mdev = 0;

	pr_info("%s: vbvolt:%d legacy_dev:%d\n", __func__,
			pmuic->vps.t.vbvolt, pmuic->legacy_dev);

	/* 1. Run a charger detection algorithm manually if necessary. */
	msleep(200);

	/* 2. Get the result by polling or via an interrupt */
	mdev = muic_get_chgtyp_to_mdev(pmuic);
	pr_info("%s: detected legacy_dev=%d\n", __func__, mdev);

	/* 3. Noti. if supported. */
	if (!muic_is_ccic_supported_dev(pmuic, mdev)) {
		pr_info("%s: Unsupported legacy_dev=%d\n", __func__, mdev);
		return 0;
	}

	if (mdev_is_supported(pmuic, pdesc->mdev)) {
		mdev_noti_detached(pmuic, pdesc->mdev);
		pdesc->mdev = 0;
	}

	pdesc->mdev = mdev;
	mdev_noti_attached(pmuic, mdev);

	return 0;
}


void init_mdev_desc(muic_data_t *pmuic)
{
	struct mdev_desc_t *pdesc = &mdev_desc;

	pr_info("%s\n", __func__);
	pdesc->mdev = 0;
	pdesc->ccic_evt_rid = 0;
	pdesc->ccic_evt_attached = false;
}


static int rid_to_mdev_with_vbus(muic_data_t *pmuic, int rid, int vbus)
{
	int mdev = 0;

	if (rid < 0 || rid >  RID_OPEN) {
		pr_err("%s:Out of RID range: %d\n", __func__, rid);
		return 0;
	}

	if ((rid == RID_523K) && vbus)
		mdev = ATTACHED_DEV_JIG_UART_OFF_VB_MUIC;
	else
		mdev = mdev_rid_tbl[rid].mdev;

	return mdev;
}

static bool mdev_is_valid_RID_OPEN(muic_data_t *pmuic, int vbus)
{
	int i, retry = 5;

	if (vbus)
		return true;

	for (i = 0; i < retry; i++) {
		pr_info("%s: %dth ...\n", __func__, i);
		msleep(10);
		if (mdev_get_vbus(pmuic))
			return 1;
	}

	return 0;
}

static int muic_handle_ccic_ATTACH(muic_data_t *pmuic, CC_NOTI_ATTACH_TYPEDEF *pnoti)
{
	struct mdev_desc_t *pdesc = &mdev_desc;

	pr_info("%s: src:%d dest:%d id:%d attach:%d cable_type:%d reserved:%d\n", __func__,
		pnoti->src, pnoti->dest, pnoti->id, pnoti->attach, pnoti->cable_type, pnoti->reserved);

	pdesc->ccic_evt_attached = pnoti->attach ? true: false;

	/* Attached */
	if (pnoti->attach) {
		int vbus = mdev_get_vbus(pmuic);

		if (mdev_is_valid_RID_OPEN(pmuic, vbus))
			pr_info("%s: Valid VBUS-> handled in irq handler\n", __func__);
		else
			pr_info("%s: No VBUS-> doing nothing.\n", __func__);

		return 0;
	}

	/* Detached */
	mdev_com_to(pmuic, MUIC_PATH_OPEN);
	if (mdev_is_supported(pmuic, pdesc->mdev))
		mdev_noti_detached(pmuic, pdesc->mdev);

	/* Reset status & flags */
	pdesc->mdev = 0;
	pdesc->ccic_evt_rid = 0;
	pdesc->ccic_evt_attached = false;

	pmuic->legacy_dev = 0;

	return 0;
}

static int mdev_handle_factory_jig(muic_data_t *pmuic, int rid, int vbus)
{
	struct mdev_desc_t *pdesc = &mdev_desc;
	int mdev = 0;

	pr_info("%s: rid:%d vbus:%d\n", __func__, rid, vbus);

	switch (rid) {
	case RID_255K:
	case RID_301K:
		mdev_com_to(pmuic, MUIC_PATH_USB_AP);
		break;
	case RID_523K:
	case RID_619K:
		mdev_com_to(pmuic, MUIC_PATH_UART_AP);
		break;
	default:
		pr_info("%s: Unsupported rid\n", __func__);
		return 0;
	}

	if (mdev_is_supported(pmuic, pdesc->mdev)) {
		mdev_noti_detached(pmuic, pdesc->mdev);
		pdesc->mdev = 0;
	}

	mdev = rid_to_mdev_with_vbus(pmuic, rid, vbus);
	pdesc->mdev = mdev;
	mdev_noti_attached(pmuic, mdev);

	return 0;
}

static int muic_handle_ccic_RID(muic_data_t *pmuic, CC_NOTI_RID_TYPEDEF *pnoti)
{
	struct mdev_desc_t *pdesc = &mdev_desc;
	int rid, vbus;

	pr_info("%s: src:%d dest:%d id:%d rid:%d sub2:%d sub3:%d\n", __func__,
		pnoti->src, pnoti->dest, pnoti->id, pnoti->rid, pnoti->sub2, pnoti->sub3);

	rid = pnoti->rid;

	if (rid > RID_OPEN) {
		pr_info("%s: Out of range of RID\n", __func__);
		return 0;
	}

	if (!pdesc->ccic_evt_attached) {
		pr_info("%s: RID but No ATTACH->discarded\n", __func__);
		return 0;
	}

	pdesc->ccic_evt_rid = rid;

	switch (rid) {
	case RID_000K:
		pr_info("%s: OTG -> discarded.\n", __func__);
		return 0;
	case RID_001K:
		pr_info("%s: MHL -> discarded.\n", __func__);
		return 0;
	case RID_255K:
	case RID_301K:
	case RID_523K:
	case RID_619K:
		vbus = mdev_get_vbus(pmuic);
		mdev_handle_factory_jig(pmuic, rid, vbus);
		break;
	case RID_OPEN:
	case RID_UNDEFINED:
		vbus = mdev_get_vbus(pmuic);
		if (pdesc->ccic_evt_attached &&
			mdev_is_valid_RID_OPEN(pmuic, vbus)) {
				/*
				 * USB team's requirement.
				 * Set AP USB for enumerations.
				 */
				mdev_com_to(pmuic, MUIC_PATH_USB_AP);

				mdev_handle_legacy_TA_USB(pmuic);
		}
		break;
	default:
		pr_err("%s:Undefined RID\n", __func__);
		return 0;
	}

	return 0;
}

static int muic_handle_ccic_notification(struct notifier_block *nb,
				unsigned long action, void *data)
{
	CC_NOTI_TYPEDEF *pnoti = (CC_NOTI_TYPEDEF *)data;
	muic_data_t *pmuic =
		container_of(nb, muic_data_t, ccic_nb);

	pr_info("%s: Rcvd Noti=> action: %d src:%d dest:%d id:%d sub[%d %d %d]\n", __func__,
		(int)action, pnoti->src, pnoti->dest, pnoti->id, pnoti->sub1, pnoti->sub2, pnoti->sub3);

	mdev_show_status(pmuic);

	switch (pnoti->id) {
	case CCIC_NOTIFY_ID_ATTACH:
		pr_info("%s: CCIC_NOTIFY_ID_ATTACH: %s\n", __func__,
				pnoti->sub1 ? "Attached": "Detached");
		muic_handle_ccic_ATTACH(pmuic, (CC_NOTI_ATTACH_TYPEDEF *)pnoti);
		break;
	case CCIC_NOTIFY_ID_RID:
		pr_info("%s: CCIC_NOTIFY_ID_RID\n", __func__);
		muic_handle_ccic_RID(pmuic, (CC_NOTI_RID_TYPEDEF *)pnoti);
		break;
	case CCIC_NOTIFY_ID_POWER_STATUS:
		pr_info("%s: CCIC_NOTIFY_ID_POWER_STATUS -> discarded.\n", __func__);
		return NOTIFY_DONE;
	default:
		pr_info("%s: Undefined Noti. ID\n", __func__);
		return NOTIFY_DONE;
	}

	mdev_show_status(pmuic);

	return NOTIFY_DONE;
}


void __delayed_ccic_notifier(struct work_struct *work)
{
	muic_data_t *pmuic;
	int ret = 0;

	pr_info("%s\n", __func__);

	pmuic = container_of(work, muic_data_t, ccic_work.work);

	ret = ccic_notifier_register(&pmuic->ccic_nb,
		muic_handle_ccic_notification, CCIC_NOTIFY_DEV_MUIC);
	if (ret < 0) {
		pr_info("%s: CCIC Noti. is not ready. Try again in 4sec...\n", __func__);
		schedule_delayed_work(&pmuic->ccic_work, msecs_to_jiffies(4000));
		return;
	}

	pr_info("%s: done.\n", __func__);
}

void muic_register_ccic_notifier(muic_data_t *pmuic)
{
	int ret = 0;

	pr_info("%s: Registering CCIC_NOTIFY_DEV_MUIC.\n", __func__);

	init_mdev_desc(pmuic);

	ret = ccic_notifier_register(&pmuic->ccic_nb,
		muic_handle_ccic_notification, CCIC_NOTIFY_DEV_MUIC);
	if (ret < 0) {
		pr_info("%s: CCIC Noti. is not ready. Try again in 8sec...\n", __func__);
		INIT_DELAYED_WORK(&pmuic->ccic_work, __delayed_ccic_notifier);
		schedule_delayed_work(&pmuic->ccic_work, msecs_to_jiffies(8000));
		return;
	}

	pr_info("%s: done.\n", __func__);
}

static void muic_pseudo_ATTACH(int attached)
{
	CC_NOTI_ATTACH_TYPEDEF attach_noti;

	pr_info("%s: attached=%d\n", __func__, attached);

	attach_noti.src = CCIC_NOTIFY_DEV_CCIC;
	attach_noti.dest = CCIC_NOTIFY_DEV_MUIC;
	attach_noti.id = CCIC_NOTIFY_ID_ATTACH;
	attach_noti.attach = attached;
	attach_noti.cable_type = 0;
	attach_noti.reserved = 0;

	ccic_notifier_test((CC_NOTI_TYPEDEF*)&attach_noti);
}

static void muic_pseudo_RID(int rid)
{
	CC_NOTI_RID_TYPEDEF rid_noti;

	pr_info("%s: rid=%d\n", __func__, rid);

	rid_noti.src = CCIC_NOTIFY_DEV_CCIC;
	rid_noti.dest = CCIC_NOTIFY_DEV_MUIC;
	rid_noti.id = CCIC_NOTIFY_ID_RID;
	rid_noti.rid = rid;
	rid_noti.sub2 = 0;
	rid_noti.sub3 = 0;
	ccic_notifier_test((CC_NOTI_TYPEDEF*)&rid_noti);
}

void muic_ccic_pseudo_noti(int mid, int rid)
{

	pr_info("%s: Rcvd => mid=%d, rid=%d\n", __func__, mid, rid);

	switch (mid) {
	case CCIC_NOTIFY_ID_ATTACH:
		muic_pseudo_ATTACH(rid);
		break;
	case CCIC_NOTIFY_ID_RID:
		muic_pseudo_RID(rid);
		break;
	case CCIC_NOTIFY_ID_POWER_STATUS:
	default:
		break;
	}

	pr_info("%s: done.\n", __func__);
}
