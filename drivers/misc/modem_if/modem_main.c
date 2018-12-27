/* linux/drivers/modem/modem.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/if_arp.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_platform.h>
#endif
#include <linux/shm_ipc.h>

#include "modem_prj.h"
#include "modem_variation.h"
#include "modem_utils.h"

#define FMT_WAKE_TIME	(HZ/2)
#define RAW_WAKE_TIME	(HZ*6)

struct mif_log log_info;

static int set_log_info(char *str)
{
	log_info.debug_log = true;
	log_info.fmt_msg = strstr(str, "fmt") ? 1 : 0;
	log_info.boot_msg = strstr(str, "boot") ? 1 : 0;
	log_info.dump_msg = strstr(str, "dump") ? 1 : 0;
	log_info.rfs_msg = strstr(str, "rfs") ? 1 : 0;
	log_info.log_msg = strstr(str, "log") ? 1 : 0;
	log_info.ps_msg = strstr(str, "ps") ? 1 : 0;
	log_info.router_msg = strstr(str, "router") ? 1 : 0;
	log_info.rcs_msg = strstr(str, "rcs") ? 1 : 0;
	log_info.ppt_msg = strstr(str, "ppt") ? 1 : 0;

	mif_err("modemIF log info: %s\n", str);

	return 0;
}
__setup("log_info=", set_log_info);

static struct modem_shared *create_modem_shared_data(
				struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct modem_shared *msd;
	int size = MAX_MIF_BUFF_SIZE;

	msd = devm_kzalloc(dev, sizeof(struct modem_shared), GFP_KERNEL);
	if (msd == NULL)
		return NULL;

	/* initialize link device list */
	INIT_LIST_HEAD(&msd->link_dev_list);

	/* initialize tree of io devices */
	msd->iodevs_tree_chan = RB_ROOT;
	msd->iodevs_tree_fmt = RB_ROOT;

	msd->storage.cnt = 0;
	msd->storage.addr = devm_kzalloc(dev, MAX_MIF_BUFF_SIZE +
		(MAX_MIF_SEPA_SIZE * 2), GFP_KERNEL);
	if (msd->storage.addr == NULL) {
		mif_err("IPC logger buff alloc failed!!\n");
		return NULL;
	}
	memset(msd->storage.addr, 0, MAX_MIF_BUFF_SIZE +
			(MAX_MIF_SEPA_SIZE * 2));
	memcpy(msd->storage.addr, MIF_SEPARATOR, strlen(MIF_SEPARATOR));
	msd->storage.addr += MAX_MIF_SEPA_SIZE;
	memcpy(msd->storage.addr, &size, sizeof(int));
	msd->storage.addr += MAX_MIF_SEPA_SIZE;
	spin_lock_init(&msd->lock);

	return msd;
}

static struct modem_ctl *create_modemctl_device(struct platform_device *pdev,
		struct modem_shared *msd)
{
	struct device *dev = &pdev->dev;
	struct modem_data *pdata = pdev->dev.platform_data;
	struct modem_ctl *modemctl;
	int ret;

	/* create modem control device */
	modemctl = devm_kzalloc(dev, sizeof(struct modem_ctl), GFP_KERNEL);
	if (!modemctl) {
		mif_err("%s: modemctl devm_kzalloc fail\n", pdata->name);
		mif_err("%s: xxx\n", pdata->name);
		return NULL;
	}

	modemctl->msd = msd;
	modemctl->dev = dev;
	modemctl->phone_state = STATE_OFFLINE;

	modemctl->mdm_data = pdata;
	modemctl->name = pdata->name;

	/* init modemctl device for getting modemctl operations */

	ret = init_modemctl_device(modemctl, pdata);
	if (ret) {
		mif_err("%s: init_modemctl_device fail (err %d)\n",
			pdata->name, ret);
		mif_err("%s: xxx\n", pdata->name);
		kfree(modemctl);
		return NULL;
	}

	mif_info("%s is created!!!\n", pdata->name);

	return modemctl;
}

static struct io_device *create_io_device(struct platform_device *pdev,
		struct modem_io_t *io_t, struct modem_shared *msd,
		struct modem_ctl *modemctl,	struct modem_data *pdata)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct io_device *iod;

	iod = devm_kzalloc(dev, sizeof(struct io_device), GFP_KERNEL);
	if (!iod) {
		mif_err("iod == NULL\n");
		return NULL;
	}

	RB_CLEAR_NODE(&iod->node_chan);
	RB_CLEAR_NODE(&iod->node_fmt);

	iod->name = io_t->name;
	iod->id = io_t->id;
	iod->format = io_t->format;
	iod->io_typ = io_t->io_type;
	iod->link_types = io_t->links;
	iod->attrs = io_t->attrs;
	iod->app = io_t->app;
	iod->use_handover = pdata->use_handover;
	atomic_set(&iod->opened, 0);

	/* link between io device and modem control */
	iod->mc = modemctl;

	if (iod->format == IPC_FMT)
		modemctl->iod = iod;

	if (iod->format == IPC_BOOT) {
		modemctl->bootd = iod;
		mif_err("BOOT device = %s\n", iod->name);
	}

	/* link between io device and modem shared */
	iod->msd = msd;

	/* add iod to rb_tree */
	if (iod->format != IPC_RAW)
		insert_iod_with_format(msd, iod->format, iod);

	if (exynos_is_not_reserved_channel(iod->id))
		insert_iod_with_channel(msd, iod->id, iod);

	/* register misc device or net device */
	ret = exynos_init_io_device(iod);
	if (ret) {
		kfree(iod);
		mif_err("exynos_init_io_device fail (%d)\n", ret);
		return NULL;
	}

	mif_info("%s created\n", iod->name);
	return iod;
}

static int attach_devices(struct io_device *iod, enum modem_link tx_link)
{
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;

	/* find link type for this io device */
	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld)) {
			/* The count 1 bits of iod->link_types is count
			 * of link devices of this iod.
			 * If use one link device,
			 * or, 2+ link devices and this link is tx_link,
			 * set iod's link device with ld
			 */

			if ((countbits(iod->link_types) <= 1) ||
					(tx_link == ld->link_type)) {
				mif_debug("set %s->%s\n", iod->name, ld->name);
				set_current_link(iod, ld);
			}
		}
	}

	/* if use rx dynamic switch, set tx_link at modem_io_t of
	 * board-*-modems.c
	 */
	if (!get_current_link(iod)) {
		mif_err("%s->link == NULL\n", iod->name);
		BUG();
	}

	switch (iod->format) {
	case IPC_FMT:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = FMT_WAKE_TIME;
		break;

	case IPC_RAW:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RAW_WAKE_TIME;
		break;

	case IPC_RFS:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RAW_WAKE_TIME;
		break;

	case IPC_MULTI_RAW:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RAW_WAKE_TIME;
		break;

	case IPC_BOOT:
		wake_lock_init(&iod->wakelock, WAKE_LOCK_SUSPEND, iod->name);
		iod->waketime = RAW_WAKE_TIME;
		break;

	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_OF
static int parse_dt_common_pdata(struct device_node *np,
					struct modem_data *pdata)
{
	mif_dt_read_string(np, "mif,name", pdata->name);
	mif_dt_read_bool(np, "mif,use_handover", pdata->use_handover);
	mif_dt_read_u32(np, "mif,link_types", pdata->link_types);
	mif_dt_read_string(np, "mif,link_name", pdata->link_name);
	mif_dt_read_u32(np, "mif,num_iodevs", pdata->num_iodevs);

	mif_dt_read_u32(np, "shmem,dump_offset", pdata->dump_offset);
	return 0;
}

static int parse_dt_mbox_pdata(struct device *dev, struct device_node *np,
					struct modem_data *pdata)
{
	struct modem_mbox *mbox = pdata->mbx;

	mbox = devm_kzalloc(dev, sizeof(struct modem_mbox), GFP_KERNEL);
	if (!mbox) {
		mif_err("mbox: failed to alloc memory\n");
		return -ENOMEM;
	}
	pdata->mbx = mbox;

#ifdef CONFIG_PCI_EXYNOS
	mif_dt_read_u32 (np, "mif,irq_cp2ap_pcie_l1ss_disable",
		mbox->irq_cp2ap_pcie_l1ss_disable);
	mif_dt_read_u32 (np, "mbx_cp2ap_pcie_l1ss_disable",
				mbox->mbx_cp2ap_pcie_l1ss_disable);
#endif

	mif_dt_read_u32 (np, "mbx_ap2cp_msg", mbox->mbx_ap2cp_msg);
	mif_dt_read_u32 (np, "mbx_cp2ap_msg", mbox->mbx_cp2ap_msg);
	mif_dt_read_u32 (np, "mbx_ap2cp_united_status",
				mbox->mbx_ap2cp_status);
	mif_dt_read_u32 (np, "mbx_cp2ap_united_status",
				mbox->mbx_cp2ap_status);

	mif_dt_read_u32 (np, "mif,int_ap2cp_msg", mbox->int_ap2cp_msg);
	mif_dt_read_u32 (np, "mif,int_ap2cp_wakeup", mbox->int_ap2cp_wakeup);
	mif_dt_read_u32 (np, "mif,int_ap2cp_status", mbox->int_ap2cp_status);
	mif_dt_read_u32 (np, "mif,int_ap2cp_active", mbox->int_ap2cp_active);

	mif_dt_read_u32 (np, "mif,irq_cp2ap_msg", mbox->irq_cp2ap_msg);
	mif_dt_read_u32 (np, "mif,irq_cp2ap_status", mbox->irq_cp2ap_status);
	mif_dt_read_u32 (np, "mif,irq_cp2ap_active", mbox->irq_cp2ap_active);
	mif_dt_read_u32 (np, "mif,irq_cp2ap_wake_lock",
				mbox->irq_cp2ap_wake_lock);

	/* Value for Performance Request */
	mif_dt_read_u32 (np, "mbx_cp2ap_dvfsreq", mbox->mbx_cp2ap_perf_req);
	mif_dt_read_u32 (np, "mbx_cp2ap_dvfsreq_cpu",
				mbox->mbx_cp2ap_perf_req_cpu);
	mif_dt_read_u32 (np, "mbx_cp2ap_dvfsreq_mif",
				mbox->mbx_cp2ap_perf_req_mif);
	mif_dt_read_u32 (np, "mbx_cp2ap_dvfsreq_int",
				mbox->mbx_cp2ap_perf_req_int);

	mif_dt_read_u32 (np, "mif,irq_cp2ap_perf_req_cpu",
				mbox->irq_cp2ap_perf_req_cpu);
	mif_dt_read_u32 (np, "mif,irq_cp2ap_perf_req_mif",
				mbox->irq_cp2ap_perf_req_mif);
	mif_dt_read_u32 (np, "mif,irq_cp2ap_perf_req_int",
				mbox->irq_cp2ap_perf_req_int);

	/* Status Bit Info */
	mif_dt_read_u32 (np, "sbi_lte_active_mask", mbox->sbi_lte_active_mask);
	mif_dt_read_u32 (np, "sbi_lte_active_pos", mbox->sbi_lte_active_pos);
	mif_dt_read_u32 (np, "sbi_wake_lock_mask", mbox->sbi_wake_lock_mask);
	mif_dt_read_u32 (np, "sbi_wake_lock_pos", mbox->sbi_wake_lock_pos);
	mif_dt_read_u32 (np, "sbi_cp_status_mask", mbox->sbi_cp_status_mask);
	mif_dt_read_u32 (np, "sbi_cp_status_pos", mbox->sbi_cp_status_pos);
	mif_dt_read_u32 (np, "sbi_pda_active_mask", mbox->sbi_pda_active_mask);
	mif_dt_read_u32 (np, "sbi_pda_active_pos", mbox->sbi_pda_active_pos);
	mif_dt_read_u32 (np, "sbi_ap_status_mask", mbox->sbi_ap_status_mask);
	mif_dt_read_u32 (np, "sbi_ap_status_pos", mbox->sbi_ap_status_pos);

	return 0;
}

static int parse_dt_iodevs_pdata(struct device *dev, struct device_node *np,
				struct modem_data *pdata)
{
	struct device_node *child = NULL;
	struct modem_io_t *iod = NULL;
	size_t size = sizeof(struct modem_io_t) * pdata->num_iodevs;
	int i = 0;

	pdata->iodevs = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!pdata->iodevs) {
		mif_err("iodevs: failed to alloc memory\n");
		return -ENOMEM;
	}

	for_each_child_of_node(np, child) {
		iod = &pdata->iodevs[i];
		mif_dt_read_string(child, "iod,name", iod->name);
		mif_dt_read_u32(child, "iod,id", iod->id);
		mif_dt_read_enum(child, "iod,format", iod->format);
		mif_dt_read_enum(child, "iod,io_type", iod->io_type);
		mif_dt_read_u32(child, "iod,links", iod->links);
		if (countbits(iod->links) > 1)
			mif_dt_read_enum(child, "iod,tx_link", iod->tx_link);
		mif_dt_read_u32(child, "iod,attrs", iod->attrs);
		mif_dt_read_string(child, "iod,app", iod->app);

		i++;
	}
	return 0;
}

static struct modem_data *modem_if_parse_dt_pdata(struct device *dev)
{
	struct modem_data *pdata;
	struct device_node *iodevs = NULL;

	pdata = devm_kzalloc(dev, sizeof(struct modem_data), GFP_KERNEL);

	if (!pdata) {
		mif_err("modem_data: alloc fail\n");
		return ERR_PTR(-ENOMEM);
	}

	if (parse_dt_common_pdata(dev->of_node, pdata)) {
		mif_err("DT error: failed to parse common\n");
		goto error;
	}

	if (parse_dt_mbox_pdata(dev, dev->of_node, pdata)) {
		mif_err("DT error: failed to parse mbox\n");
		goto error;
	}

	iodevs = of_get_child_by_name(dev->of_node, "iodevs");
	if (!iodevs) {
		mif_err("DT error: failed to get child node\n");
		goto error;
	}

	if (parse_dt_iodevs_pdata(dev, iodevs, pdata)) {
		mif_err("DT error: failed to parse iodevs\n");
		goto error;
	}

	dev->platform_data = pdata;
	mif_info("DT parse complete!\n");
	return pdata;

error:
	if (pdata) {
		if (pdata->iodevs)
			devm_kfree(dev, pdata->iodevs);
		devm_kfree(dev, pdata);
	}

	return ERR_PTR(-EINVAL);
}

static const struct of_device_id sec_modem_match[] = {
	{ .compatible = "sec_modem,modem_pdata", },
	{},
};
MODULE_DEVICE_TABLE(of, sec_modem_match);
#else
static struct modem_data *modem_if_parse_dt_pdata(struct device *dev)
{
	return ERR_PTR(-ENODEV);
}
#endif

static void modem_get_shmem_base(struct modem_data *pdata)
{
	pdata->shmem_base = shm_get_phys_base();
	pdata->ipcmem_offset = shm_get_ipc_rgn_offset();
	pdata->ipc_size = shm_get_ipc_rgn_size();
	mif_err("shmem_base: 0x%x ipcmem_offset: 0x%x, ipc_size: 0x%x\n",
		pdata->shmem_base, pdata->ipcmem_offset, pdata->ipc_size);
}

struct io_device *iod_test;

static ssize_t do_cp_crash_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	unsigned int val = 0;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	if (mc->bootd)
		mc->bootd->modem_state_changed(mc->bootd, val);
	return count;
}

static ssize_t modem_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", cp_state_str[mc->phone_state]);
}
static DEVICE_ATTR_WO(do_cp_crash);
static DEVICE_ATTR_RO(modem_state);

static struct attribute *modem_attrs[] = {
	&dev_attr_do_cp_crash.attr,
	&dev_attr_modem_state.attr,
	NULL,
};
ATTRIBUTE_GROUPS(modem);

static int modem_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct modem_data *pdata = dev->platform_data;
	struct modem_shared *msd;
	struct modem_ctl *modemctl;
	struct io_device **iod;
	struct link_device *ld;
	unsigned size;
	int i;

	mif_err("%s: +++\n", pdev->name);

	if (dev->of_node) {
		pdata = modem_if_parse_dt_pdata(dev);
		if (IS_ERR(pdata)) {
			mif_err("MIF DT pasrse error!\n");
			return PTR_ERR(pdata);
		}
	}
	modem_get_shmem_base(pdata);

	msd = create_modem_shared_data(pdev);
	if (!msd) {
		mif_err("%s: msd == NULL\n", pdata->name);
		return -ENOMEM;
	}

	modemctl = create_modemctl_device(pdev, msd);
	if (!modemctl) {
		mif_err("%s: modemctl == NULL\n", pdata->name);
		kfree(msd);
		return -ENOMEM;
	}

	/* create link device */
	/* support multi-link device */
	for (i = 0; i < LINKDEV_MAX; i++) {
		/* find matching link type */
		if (pdata->link_types & LINKTYPE(i)) {
			ld = call_link_init_func(pdev, i);
			if (!ld)
				goto free_mc;

			mif_err("%s: %s link created\n", pdata->name, ld->name);
			ld->link_type = i;
			ld->mc = modemctl;
			ld->msd = msd;
			list_add(&ld->list, &msd->link_dev_list);
		}
	}

	/* create io deivces and connect to modemctl device */
	size = sizeof(struct io_device *) * pdata->num_iodevs;
	iod = (struct io_device **)devm_kzalloc(dev, size, GFP_KERNEL);
	for (i = 0; i < pdata->num_iodevs; i++) {
		iod[i] = create_io_device(pdev, &pdata->iodevs[i], msd,
					modemctl, pdata);
		if (!iod[i]) {
			mif_err("%s: iod[%d] == NULL\n", pdata->name, i);
			goto free_iod;
		}

		attach_devices(iod[i], pdata->iodevs[i].tx_link);
	}

	platform_set_drvdata(pdev, modemctl);

	if (sysfs_create_groups(&dev->kobj, modem_groups))
		mif_err("failed to create modem groups node\n");

	mif_err("%s: ---\n", pdata->name);

	return 0;

free_iod:
	for (i = 0; i < pdata->num_iodevs; i++)
		kfree(iod[i]);

free_mc:
	kfree(modemctl);
	kfree(msd);

	mif_err("%s: xxx\n", pdata->name);

	return -ENOMEM;
}

static void modem_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct modem_ctl *mc = dev_get_drvdata(dev);
	struct utc_time utc;

	mc->phone_state = STATE_OFFLINE;

	get_utc_time(&utc);
	mif_info("%s: at [%02d:%02d:%02d.%03d]\n",
		mc->name, utc.hour, utc.min, utc.sec, utc.msec);
}

#ifdef CONFIG_OF
static int modem_suspend(struct device *pdev)
{
	struct modem_ctl *mc = dev_get_drvdata(pdev);

	if (mc->ops.suspend_modem_ctrl != NULL) {
		mif_err("%s: pd_active:0\n", mc->name);
		mc->ops.suspend_modem_ctrl(mc);
	}

	return 0;
}

static int modem_resume(struct device *pdev)
{
	struct modem_ctl *mc = dev_get_drvdata(pdev);

	if (mc->ops.suspend_modem_ctrl != NULL) {
		mif_err("%s: pd_active:1\n", mc->name);
		mc->ops.resume_modem_ctrl(mc);
	}

	return 0;
}
#else
#define modem_suspend	NULL
#define modem_resume	NULL
#endif

static const struct dev_pm_ops modem_pm_ops = {
	.suspend = modem_suspend,
	.resume = modem_resume,
};

static struct platform_driver modem_driver = {
	.probe = modem_probe,
	.shutdown = modem_shutdown,
	.driver = {
		.name = "mif_exynos",
		.owner = THIS_MODULE,
		.pm = &modem_pm_ops,

#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(sec_modem_match),
#endif
	},
};

module_platform_driver(modem_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung Modem Interface Driver");
