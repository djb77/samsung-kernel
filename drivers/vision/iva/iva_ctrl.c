/* iva_ctrl.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/ion.h>
#include <linux/notifier.h>
#include <linux/pm_runtime.h>
#include <linux/clk-provider.h>
#ifdef CONFIG_EXT_BOOTMEM
#include <linux/ext_bootmem.h>
#endif
#ifdef CONFIG_EXYNOS_IOVMM
#include <linux/exynos_iovmm.h>
#endif
#ifdef CONFIG_EXYNOS_IMA
#include <exynos_ima.h>
#endif
#include "iva_ctrl_ioctl.h"
#include "regs/iva_base_addr.h"
#include "iva_ipc_queue.h"
#include "iva_ctrl.h"
#include "iva_mbox.h"
#include "iva_mcu.h"
#include "iva_pmu.h"
#include "iva_sh_mem.h"
#include "iva_mem.h"
#include "iva_sysfs.h"
#include "iva_vdma.h"

#undef ENABLE_MMAP_US
#undef ENABLE_FPGA_REG_ACCESS
#undef ENABLE_LOCAL_PLAT_DEVICE
#undef ENABLE_MEASURE_IVA_PERF
#undef ENABLE_MBOX_SEND_IOCTL

#ifdef MODULE
#define IVA_CTRL_NAME			"iva_ctrl"
#define IVA_CTRL_MAJOR			(63)
#endif
#define IVA_MBOX_IRQ_RSC_NAME		"iva_mbox_irq"
#define IVA_PM_QOS_CAM_REQ		(690000)

static struct iva_dev_data *g_iva_data;

static int32_t __iva_init(struct device *dev)
{
	int ret;
	struct iva_dev_data *iva = dev_get_drvdata(dev);
	struct clk *iva_clk = iva->iva_clk;

#ifdef ENABLE_FPGA_REG_ACCESS
	uint32_t	val;
	void __iomem	*peri_regs;

	/* TO DO : check again later */
	/* enable the Qrequest otherwise IVA will start going into low-power mode.
	 * this is required for all FPGA >= v29
	 * Also write 1 to bit 4 which is Qchannel_en (CMU model in FPGA).
	 * Avail in FPGA >= 42b
	 */
	peri_regs = devm_ioremap(dev, PERI_PHY_BASE_ADDR, PERI_SIZE);
	if (!peri_regs) {
		dev_err(dev, "%s() fail to ioremap. peri phys(0x%x), size(0x%x)\n",
			__func__, PERI_PHY_BASE_ADDR, PERI_SIZE);
		return -ENOMEM;
	}
	writel(0x12, peri_regs + 0x1008);

	/* version register. if upper 8-bits are non-zero then "assume" to be in
	* 2-FPGA build environment where SCore FPGA version is contained in bits
	* 31:24 and IVA FPGA in 23:16 */
	val = readl(peri_regs + 0x1000);
	if (val & 0xff000000)
		dev_info(dev, "%s() FPGA ver: IVA=%d SCore=%d\n",
			__func__, (val >> 16) & val, val >> 24);
	else
		dev_info(dev, "%s() FPGA ver: %d\n", __func__, val >> 16);

	devm_iounmap(dev, peri_regs);
#endif

	dev_dbg(dev, "%s() init iva state(0x%lx) with hwa en(%d)\n",
			__func__, iva->state, iva->en_hwa_req);

	if (test_and_set_bit(IVA_ST_INIT_DONE, &iva->state)) {
		dev_warn(dev, "%s() already iva inited(0x%lx)\n",
				__func__, iva->state);
		return 0;
	}

 	if (iva_clk) {
		ret = clk_prepare_enable(iva_clk);
		if (ret) {
			dev_err(dev, "fail to enable iva_clk(%d)\n", ret);
			return ret;
		}
	#if 0
		/* confirm */
		if (!__clk_is_enabled(iva_clk)) {
			dev_err(dev, "%s() req iva_clk on but off\n", __func__);
			return -EINVAL;
		}
	#endif
		dev_dbg(dev, "%s() success to enable iva_clk with rate(%ld)\n",
			__func__, clk_get_rate(iva_clk));
 	}

#ifdef CONFIG_EXYNOS_IMA
	ima_host_begin();
#endif
	iva_pmu_init(iva, iva->en_hwa_req);

	/* iva mbox supports both of iva and score. */
	iva_mbox_init(iva);
	return 0;

}

static int32_t __iva_deinit(struct device *dev)
{
	struct iva_dev_data *iva = dev_get_drvdata(dev);
	struct clk *iva_clk = iva->iva_clk;

	if (!test_and_clear_bit(IVA_ST_INIT_DONE, &iva->state)) {
		dev_warn(dev, "%s() already iva deinited(0x%lx)\n",
				__func__, iva->state);
		return 0;
	}

	dev_dbg(dev, "%s()\n", __func__);

	iva_mbox_deinit(iva);
	iva_pmu_deinit(iva);

#ifdef CONFIG_EXYNOS_IMA
	ima_host_end();
#endif
	if (iva_clk) {
		clk_disable_unprepare(iva_clk);
	#if 1	/* check */
		if (__clk_is_enabled(iva_clk)) {
			dev_err(dev, "%s() req iva_clk off but on\n", __func__);
			return -EINVAL;
		}
	#endif
	}

	return 0;
}

static int32_t iva_init(struct iva_dev_data *iva, bool en_hwa)
{
	int ret;
	struct device *dev = iva->dev;

	iva->en_hwa_req = en_hwa;

#ifdef CONFIG_PM
	ret = pm_runtime_get_sync(dev);
#else
	ret = __iva_init(dev);
#endif
	if (ret < 0) {
		dev_err(dev, "%s() fail to enable iva clk/pwr. ret(%d)\n",
				__func__, ret);
		return ret;
	}

	dev_dbg(dev, "%s() rt ret(%d) iva clk rate(%ld)\n", __func__,
			ret, clk_get_rate(iva->iva_clk));
	return ret;
}

static inline int32_t iva_init_usr(struct iva_dev_data *iva,
		uint32_t __user *en_hwa_usr)
{
	uint32_t en_hwa;

	get_user(en_hwa, en_hwa_usr);

	return iva_init(iva, en_hwa);
}

static int32_t iva_deinit(struct iva_dev_data *iva)
{
	int ret;
	struct device *dev = iva->dev;

	if (!test_bit(IVA_ST_INIT_DONE, &iva->state)) {
		dev_info(dev, "%s() already iva deinited(0x%lx)."
			"plz check the pair of init/deinit\n",
			__func__, iva->state);
		return 0;
	}

#ifdef CONFIG_PM
	ret = pm_runtime_put_sync(dev);
#else
	ret = __iva_deinit(dev);
#endif
	if (ret < 0) {
		dev_err(dev, "%s() fail to deinit iva(%d)\n", __func__, ret);
		return ret;
	}

	dev_dbg(dev, "%s() ret(%d)\n", __func__, ret);
	return ret;
}


static int32_t iva_ctrl_mcu_boot_file(struct iva_dev_data *iva,
			const char __user *mcu_file_u, int32_t wait_ready)
{
	struct device *dev = iva->dev;
	int	name_len;
	char	mcu_file[IVA_CTRL_BOOT_FILE_LEN];
	int32_t ret;
	bool	forced_iva_init = false;

	name_len = strncpy_from_user(mcu_file, mcu_file_u,
			sizeof(mcu_file) - 1);
	mcu_file[sizeof(mcu_file) - 1] = 0x0;
	if (name_len <= 0) {
		dev_err(dev, "%s() unexpected name_len(%d)\n",
				__func__, name_len);
		return name_len;
	}

	if (test_and_set_bit(IVA_ST_MCU_BOOT_DONE, &iva->state)) {
		dev_warn(dev, "%s() already iva booted(0x%lx)\n",
					__func__, iva->state);
		return 0;
	}

	if (!test_bit(IVA_ST_INIT_DONE, &iva->state)) {
		dev_dbg(dev, "%s() mcu boot request w/o iva init(0x%lx)\n",
				__func__, iva->state);
		ret = iva_init(iva, false);
		if (ret < 0) {
			dev_err(dev, "%s() fail to init iva from "
					"mcu boot(0x%lx)\n",
					__func__, iva->state);
			return ret;
		}

		forced_iva_init = true;
		set_bit(IVA_ST_INIT_DONE, &iva->state);
	}

#ifdef CONFIG_PM_SLEEP
	/* prevent the system to suspend */
	wake_lock(&iva->iva_wake_lock);
#endif

#ifdef CONFIG_SOC_EXYNOS8895
	if (!test_and_set_bit(IVA_ST_PM_QOS_CAM, &iva->state)) {
		dev_dbg(dev, "%s() request cam pm_qos(0x%lx)\n",
					__func__, iva->state);
		pm_qos_update_request(&iva->iva_qos_cam, IVA_PM_QOS_CAM_REQ);
	}
#endif
	dev_dbg(dev, "%s() iva_clk rate(%ld)\n", __func__,
			clk_get_rate(iva->iva_clk));

	ret = iva_mcu_boot_file(iva, mcu_file, wait_ready);
	if (!ret)	/* on success */
		return 0;

	/* on failure */
#ifdef CONFIG_SOC_EXYNOS8895
	pm_qos_update_request(&iva->iva_qos_cam, 0);
	clear_bit(IVA_ST_PM_QOS_CAM, &iva->state);
#endif

#ifdef CONFIG_PM_SLEEP
	wake_unlock(&iva->iva_wake_lock);
#endif
	if (forced_iva_init) {
		iva_deinit(iva);
		clear_bit(IVA_ST_INIT_DONE, &iva->state);
	}

	clear_bit(IVA_ST_MCU_BOOT_DONE, &iva->state);
	return ret;
}

static int32_t iva_ctrl_mcu_exit(struct iva_dev_data *iva)
{
	struct device *dev = iva->dev;

	if (!test_and_clear_bit(IVA_ST_MCU_BOOT_DONE, &iva->state)) {
		dev_info(dev, "%s() already mcu exited(0x%lx)\n",
				__func__, iva->state);
		return 0;
	}

	iva_mcu_exit(iva);

#ifdef CONFIG_PM_SLEEP
	wake_unlock(&iva->iva_wake_lock);
#endif
#ifdef CONFIG_SOC_EXYNOS8895
	if (test_and_clear_bit(IVA_ST_PM_QOS_CAM, &iva->state)) {
		pm_qos_update_request(&iva->iva_qos_cam, 0);

		dev_dbg(dev, "%s() iva_clk rate(%ld)\n", __func__,
				clk_get_rate(iva->iva_clk));
	}
#endif
	return 0;
}

const char *iva_ctrl_get_ioctl_str(unsigned int cmd)
{
	#define IOCTL_STR_ENTRY(cmd_nr, cmd_str)	[cmd_nr] = cmd_str
	static const char *iva_ioctl_str[] = {
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_INIT_IVA),		"INIT_IVA"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_DEINIT_IVA),		"DEINIT_IVA"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_BOOT_MCU),		"BOOT_MCU"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_EXIT_MCU),		"EXIT_MCU"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_IPC_QUEUE_SEND_CMD),	"IPCQ_SEND"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_IPC_QUEUE_RECEIVE_RSP),	"IPCQ_RECEIVE"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_MBOX_SEND_MSG),		"MBOX_SEND"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_GET_IOVA),		"ION_GET_IOVA"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_PUT_IOVA),		"ION_PUT_IOVA"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_ALLOC), 		"ION_ALLOC"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_FREE),			"ION_FREE"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_SYNC_FOR_CPU),		"ION_SYNC_FOR_CPU"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_SYNC_FOR_DEVICE),	"ION_SYNC_FOR_DEVICE"),
	};

	unsigned int cmd_nr = _IOC_NR(cmd);

	if (cmd_nr >= ARRAY_SIZE(iva_ioctl_str))
		return "UNKNOWN IOCTL";

	return iva_ioctl_str[cmd_nr];
}

static inline s32 __maybe_unused iva_timeval_diff_us(struct timeval *a,
		struct timeval *b)
{
	return (a->tv_sec - b->tv_sec) * USEC_PER_SEC +
			(a->tv_usec - b->tv_usec);
}

static long iva_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user		*p = (void __user *)arg;
	struct device		*dev;
	int			ret;
	struct iva_dev_data	*iva;
	struct iva_proc		*proc;

#ifdef ENABLE_MEASURE_IVA_PERF
	struct timeval before, after;

	do_gettimeofday(&before);
#endif
	proc	= (struct iva_proc *) filp->private_data;
	iva	= proc->iva_data;
	dev	= iva->dev;

	dev_dbg(dev, "%s() cmd(%s) arg(0x%p)\n", __func__,
			iva_ctrl_get_ioctl_str(cmd), p);

	if (_IOC_TYPE(cmd) != IVA_CTRL_MAGIC || !p) {
		dev_warn(dev, "%s() check the your ioctl cmd(0x%x) arg(0x%p)\n",
				__func__, cmd, p);
		return -EINVAL;
	}

	switch (cmd) {
	case IVA_CTRL_INIT_IVA:
		ret = iva_init_usr(iva, (uint32_t __user *) p);
		break;
	case IVA_CTRL_DEINIT_IVA:
		ret = iva_deinit(iva);
		break;
	case IVA_CTRL_BOOT_MCU:
		ret = iva_ctrl_mcu_boot_file(iva,
				(const char __user *) p, false);
		break;
	case IVA_CTRL_EXIT_MCU:
		ret = iva_ctrl_mcu_exit(iva);
		break;
	case IVA_IPC_QUEUE_SEND_CMD:
		ret = iva_ipcq_send_cmd_usr(&iva->mcu_ipcq,
				(struct ipc_cmd_param __user *) p);
		break;
	case IVA_IPC_QUEUE_RECEIVE_RSP:
		ret = iva_ipcq_wait_res_usr(&iva->mcu_ipcq,
				(struct ipc_res_param __user *) p);
		break;
#ifdef ENABLE_MBOX_SEND_IOCTL
	case IVA_MBOX_SEND_MSG:
		ret = iva_mbox_send_mail_to_mcu_usr(iva, (uint32_t __user *) p);
		break;
#endif
	case IVA_ION_GET_IOVA:
	case IVA_ION_PUT_IOVA:
	case IVA_ION_ALLOC:
	case IVA_ION_FREE: /* This may not be used */
	case IVA_ION_SYNC_FOR_CPU:
	case IVA_ION_SYNC_FOR_DEVICE:
		ret = iva_mem_ioctl(proc, cmd, p);
		break;
	default:
		dev_warn(dev, "%s() unhandled cmd 0x%x",
				__func__, cmd);
		ret = -ENOTTY;
		break;
	}

#ifdef ENABLE_MEASURE_IVA_PERF
	do_gettimeofday(&after);
	dev_info(dev, "%s(%d usec)\n", iva_ctrl_get_ioctl_str(cmd),
			iva_timeval_diff_us(&after, &before));
#endif
	return ret;
}


static int iva_ctrl_mmap(struct file *filp, struct vm_area_struct *vma)
{
#ifndef MODULE
	struct iva_proc		*proc = (struct iva_proc *) filp->private_data;
	struct iva_dev_data	*iva = proc->iva_data;
	struct device		*dev = iva->dev;
#else
	struct iva_dev_data	*iva = filp->private_data;
	struct device		*dev = iva->dev;
#endif

#ifdef ENABLE_MMAP_US
	int		ret;
	unsigned long	req_size = vma->vm_end - vma->vm_start;
	unsigned long	phys_iva = iva->iva_res->start;
	unsigned long	req_phys_iva = vma->vm_pgoff << PAGE_SHIFT;

	if ((req_phys_iva < phys_iva) || ((req_phys_iva + req_size) >
			(phys_iva + resource_size(iva->iva_res)))) {
		dev_warn(dev, "%s() not allowed to map the region(0x%lx-0x%lx)",
				__func__,
				req_phys_iva, req_phys_iva + req_size);
		return -EPERM;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			  req_size, vma->vm_page_prot);
	if (ret) {
		dev_err(dev, "%s() fail to remap, ret(%d), start(0x%lx), "
				"end(0x%lx), pgoff(0x%lx)\n", __func__, ret,
				vma->vm_start, vma->vm_end, vma->vm_pgoff);
		return ret;
	}

	dev_dbg(dev, "%s() success!!! start(0x%lx), end(0x%lx), "
			"pgoff(0x%lx)\n", __func__,
			vma->vm_start, vma->vm_end, vma->vm_pgoff);
	return 0;
#else
	dev_err(dev, "%s() not allowed to map with start(0x%lx), end(0x%lx), "
			"pgoff(0x%lx)\n", __func__,
			vma->vm_start, vma->vm_end, vma->vm_pgoff);
	return -EPERM;	/* error: not allowed */
#endif
}


static int iva_ctrl_open(struct inode *inode, struct file *filp)
{
	struct device *dev;
	struct iva_dev_data	*iva;
	struct iva_proc		*proc;
#ifndef MODULE
	struct miscdevice *miscdev;

	miscdev = filp->private_data;
	dev = miscdev->parent;
	iva = dev_get_drvdata(dev);
#else
	/* smarter */
	iva = filp->private_data = g_iva_data;
	dev = iva->dev;
#endif
	proc = devm_kmalloc(dev, sizeof(*proc), GFP_KERNEL);
	if (!proc) {
		dev_err(dev, "%s() fail to alloc mem for struct iva_proc\n",
				__func__);
		return -ENOMEM;
	}
	/* assume only one open() per process */
	get_task_struct(current);
	proc->tsk	= current;
	proc->pid	= current->group_leader->pid;
	proc->tid	= current->pid;
	proc->iva_data	= iva;
	proc->state	= 0;

	mutex_init(&proc->proc_mem_lock);
	hash_init(proc->h_mem_map);

	mutex_lock(&iva->proc_mutex);
	list_add(&proc->proc_node, &iva->proc_head);
	mutex_unlock(&iva->proc_mutex);

	/* update file->private to hold struct iva_proc */
	filp->private_data = (void *) proc;

	dev_dbg(dev, "%s() succeed to open from (%s, %d, %d)\n", __func__,
			proc->tsk->comm, proc->pid, proc->tid);
	return 0;
}

static int iva_ctrl_release(struct inode *inode, struct file *filp)
{
	struct device *dev;
	struct iva_dev_data	*iva;
	struct iva_proc		*proc;

	proc	= (struct iva_proc *) filp->private_data;
	iva	= proc->iva_data;
	dev	= iva->dev;

	mutex_lock(&iva->proc_mutex);
	list_del(&proc->proc_node);
	mutex_unlock(&iva->proc_mutex);

	dev_dbg(dev, "%s() try to close from (%s, %d, %d)\n", __func__,
			proc->tsk->comm, proc->pid, proc->tid);

	/* to do : per process */
	dev_dbg(dev, "%s() state(0x%lx)\n", __func__, iva->state);

	/* one more check */
#ifdef CONFIG_SOC_EXYNOS8895
	if (test_bit(IVA_ST_PM_QOS_CAM, &iva->state))
		pm_qos_update_request(&iva->iva_qos_cam, 0);
#endif
	if (test_bit(IVA_ST_MCU_BOOT_DONE, &iva->state))
		iva_ctrl_mcu_exit(iva);
	if (test_bit(IVA_ST_INIT_DONE, &iva->state))
		iva_deinit(iva);

	iva_mem_force_to_free_proc_mapped_list(proc);
	mutex_destroy(&proc->proc_mem_lock);

	put_task_struct(proc->tsk);

	devm_kfree(dev, proc);
	return 0;
}

static struct file_operations iva_ctl_fops =
{
	.owner		= THIS_MODULE,
	.open           = iva_ctrl_open,
	.release	= iva_ctrl_release,
	.unlocked_ioctl	= iva_ctrl_ioctl,
	.compat_ioctl	= iva_ctrl_ioctl,
	.mmap		= iva_ctrl_mmap
};

#ifdef CONFIG_EXYNOS_IOVMM
static int iva_iommu_fault_handler(struct iommu_domain *domain,
		struct device *dev, unsigned long fault_addr,
		int fault_flags, void *token)
{
	struct iva_dev_data *iva = token;

	dev_err(dev, "%s() is called with fault addr(0x%lx)\n",
			__func__, fault_addr);

	iva_vdma_show_status(iva);

	return 0;
}
#endif

static int iva_ctrl_catch_panic_event(struct notifier_block *nb,
		unsigned long event, void *data)
{
	struct iva_dev_data *iva = container_of(nb,
			struct iva_dev_data, k_panic_nb);

	dev_err(iva->dev, "%s() iva state(0x%lx)\n", __func__, iva->state);

	if (test_bit(IVA_ST_MCU_BOOT_DONE, &iva->state))
		iva_mcu_handle_panic_k(iva);

	/* called twice if iva sysmmu fault happens */
	iva_mem_show_global_mapped_list_nolock(iva);

	return NOTIFY_DONE;
}

static int iva_ctrl_register_panic_handler(struct iva_dev_data *iva)
{
	struct notifier_block *iva_nb = &iva->k_panic_nb;

	iva_nb->notifier_call	= iva_ctrl_catch_panic_event;
	iva_nb->next		= NULL;
	iva_nb->priority	= 0;

	return atomic_notifier_chain_register(&panic_notifier_list, iva_nb);
}

static int iva_ctrl_unregister_panic_handler(struct iva_dev_data *iva)
{
	struct notifier_block *iva_nb = &iva->k_panic_nb;

	atomic_notifier_chain_unregister(&panic_notifier_list, iva_nb);

	iva_nb->notifier_call	= NULL;
	iva_nb->next		= NULL;
	iva_nb->priority	= 0;

	return 0;
}

static void iva_ctl_parse_of_dt(struct device *dev, struct iva_dev_data	*iva)
{
	struct device_node *of_node = dev->of_node;
	struct device_node *child_node;

	/* overwrite parameters from of */
	if (!of_node) {
		dev_info(dev, "of node not found.\n");
		return;
	}

	child_node = of_find_node_by_name(of_node, "mcu-info");
	if (child_node) {
		of_property_read_u32(child_node, "mem_size",
				&iva->mcu_mem_size);
		of_property_read_u32(child_node, "shmem_size",
				&iva->mcu_shmem_size);
		of_property_read_u32(child_node, "print_delay",
				&iva->mcu_print_delay);
		dev_dbg(dev, "%s() of mem_size(0x%x) shmem_size(0x%x) "
				"print_delay(%d)\n",
				__func__,
				iva->mcu_mem_size, iva->mcu_shmem_size,
				iva->mcu_print_delay);
	}
}


static int iva_ctl_probe(struct platform_device *pdev)
{
	int	ret = 0;
	struct device		*dev = &pdev->dev;
	struct iva_dev_data	*iva;
	struct resource		*res_iva;
#ifdef USE_NAMED_IRQ_RSC
	struct resource		*res_mbox_irq;
#endif
	int	irq_nr = IVA_MBOX_IRQ_NOT_DEFINED;

	/* iva */
	res_iva = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_iva) {
		dev_err(dev, "failed to find memory resource for iva sfr\n");
		return -ENXIO;
	}
	dev_dbg(dev, "resource 0 %p (%x..%x)\n", res_iva,
			(uint32_t) res_iva->start, (uint32_t) res_iva->end);

	/* mbox irq */
#ifdef USE_NAMED_IRQ_RSC
	res_mbox_irq = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
			IVA_MBOX_IRQ_RSC_NAME);
	if (res_mbox_irq) {
		irq_nr = res_mbox_irq->start;
	} else
		dev_warn(dev, "not defined irq resource for iva mbox\n");
#else
	irq_nr = platform_get_irq(pdev, 0);
#endif
	dev_dbg(dev, "irq nr(%d)\n", irq_nr);

	iva = dev_get_drvdata(dev);
	if (!iva) {
	 	iva = devm_kzalloc(dev, sizeof(struct iva_dev_data), GFP_KERNEL);
		if (!iva) {
			dev_err(dev, "%s() fail to alloc mem for plat data\n",
					__func__);
			return -ENOMEM;
		}
		dev_dbg(dev, "%s() success to alloc mem for iva ctrl(%p)\n",
					__func__, iva);
		iva->flags = IVA_CTL_ALLOC_LOCALLY_F;
		dev_set_drvdata(dev, iva);
	}

	/* fill data */
#ifndef MODULE
	iva->iva_clk = devm_clk_get(dev, "clk_iva");
	if (IS_ERR(iva->iva_clk)) {
		dev_err(dev, "%s() fail to get clk_iva\n", __func__);
		ret = PTR_ERR(iva->iva_clk);
		goto err_iva_clk;
	}
#endif
	iva->dev	= dev;
	iva->misc.minor	= MISC_DYNAMIC_MINOR;
	iva->misc.name	= "iva_ctl";
	iva->misc.fops	= &iva_ctl_fops;
	iva->misc.parent= dev;

	mutex_init(&iva->proc_mutex);
	INIT_LIST_HEAD(&iva->proc_head);

	iva_ctl_parse_of_dt(dev, iva);

	if (!iva->mcu_mem_size || !iva->mcu_shmem_size) {
		dev_warn(dev, "%s() forced set to mcu dft value\n", __func__);
		iva->mcu_mem_size	= VMCU_MEM_SIZE;
		iva->mcu_shmem_size	= SHMEM_SIZE;
	}

	/* reserve the resources */
	iva->mbox_irq_nr= irq_nr;
	iva->iva_res = devm_request_mem_region(dev, res_iva->start,
			resource_size(res_iva), res_iva->name ? : dev_name(dev));
	if (!iva->iva_res) {
		dev_err(dev, "%s() fail to request resources. (%p)\n",
				__func__,iva->iva_res);
		ret = -EINVAL;
		goto err_req_iva_res;
	}

	iva->iva_va = devm_ioremap(dev, iva->iva_res->start,
			resource_size(iva->iva_res));
	if (!iva->iva_va) {
		dev_err(dev, "%s() fail to ioremap resources.(0x%llx--0x%llx)\n",
				__func__,
				(u64) iva->iva_res->start,
				(u64) iva->iva_res->end);
		ret = -EINVAL;
		goto err_io_remap;

	}

	ret = iva_mem_create_ion_client(iva);
	if (ret) {
		dev_err(dev, "%s() fail to create ion client ret(%d)\n",
			__func__,  ret);
		goto err_ion_client;
	}

	ret = iva_pmu_probe(iva);
	if (ret) {
		dev_err(dev, "%s() fail to probe iva pmu. ret(%d)\n",
			__func__,  ret);
		goto err_pmu_prb;
	}

	ret = iva_mcu_probe(iva);
	if (ret) {
		dev_err(dev, "%s() fail to probe iva mcu. ret(%d)\n",
			__func__,  ret);
		goto err_mcu_prb;
	}

#ifdef MODULE
	ret = register_chrdev(IVA_CTRL_MAJOR, IVA_CTRL_NAME, &iva_ctl_fops);
	if (ret < 0) {
		dev_err(dev, "%s() fail to register. ret(%d)\n",
			__func__,  ret);
		goto err_misc_reg;
	}
#else
	ret = misc_register(&iva->misc);
	if (ret) {
		dev_err(dev, "%s() failed to register misc device\n",
				__func__);
		goto err_misc_reg;
	}
#endif
	/* enable runtime pm */
	pm_runtime_enable(dev);

#ifdef CONFIG_EXYNOS_IOVMM
	iovmm_set_fault_handler(dev, iva_iommu_fault_handler, iva);

	/* automatically controlled by rpm */
	ret = iovmm_activate(dev);
	if (ret) {
		dev_err(dev, "%s() fail to activate iovmm ret(%d)\n",
				__func__, ret);
		goto err_iovmm;
	}
#endif

	/* set sysfs */
	ret = iva_sysfs_init(dev);
	if (ret) {
		dev_err(dev, "%s() sysfs registeration is failed(%d)\n",
				__func__, ret);
	}

	/* register panic handler for debugging */
	iva_ctrl_register_panic_handler(iva);

#ifdef CONFIG_SOC_EXYNOS8895
	pm_qos_add_request(&iva->iva_qos_cam, PM_QOS_CAM_THROUGHPUT, 0);
#endif

#ifdef CONFIG_PM_SLEEP
	/* initialize the iva wake lock */
	wake_lock_init(&iva->iva_wake_lock, WAKE_LOCK_SUSPEND, "iva_run_wlock");
#endif
	/* just for easy debug */
	g_iva_data = iva;

	dev_dbg(dev, "%s() probed successfully\n", __func__);
	return 0;

#ifdef CONFIG_EXYNOS_IOVMM
err_iovmm:
	pm_runtime_disable(dev);
#endif
err_misc_reg:
	iva_mcu_remove(iva);
err_mcu_prb:
	iva_pmu_remove(iva);
err_pmu_prb:
	iva_mem_destroy_ion_client(iva);
err_ion_client:
	devm_iounmap(dev, iva->iva_va);
err_io_remap:
	if (iva->iva_res) {
		devm_release_mem_region(dev, iva->iva_res->start,
				resource_size(iva->iva_res));
	}

err_req_iva_res:
	if (iva->iva_clk)
		devm_clk_put(dev, iva->iva_clk);

#ifndef MODULE
err_iva_clk:
#endif
	if (iva->flags & IVA_CTL_ALLOC_LOCALLY_F)
		devm_kfree(dev, iva);
	dev_set_drvdata(dev, NULL);
	return ret;
}


static int iva_ctl_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct device		*dev = &pdev->dev;
	struct iva_dev_data	*iva = platform_get_drvdata(pdev);

	dev_info(dev, "%s()\n", __func__);

	g_iva_data = NULL;

#ifdef CONFIG_PM_SLEEP
	wake_lock_destroy(&iva->iva_wake_lock);
#endif
	iva_ctrl_unregister_panic_handler(iva);

	iva_sysfs_deinit(dev);

#ifdef CONFIG_EXYNOS_IOVMM
	iovmm_deactivate(dev);
#endif
	pm_runtime_disable(dev);

#ifdef MODULE
	unregister_chrdev(IVA_CTRL_MAJOR, IVA_CTRL_NAME);
#else
	misc_deregister(&iva->misc);
#endif
	iva_mcu_remove(iva);
	iva_pmu_remove(iva);

	iva_mem_destroy_ion_client(iva);

	devm_iounmap(dev, iva->iva_va);

	devm_release_mem_region(dev, iva->iva_res->start,
				resource_size(iva->iva_res));

	mutex_destroy(&iva->mem_map_lock);

	if (iva->iva_clk) {
		devm_clk_put(dev, iva->iva_clk);
		iva->iva_clk = NULL;
	}

	if (iva->flags & IVA_CTL_ALLOC_LOCALLY_F) {
		devm_kfree(dev, iva);
		platform_set_drvdata(pdev, NULL);
	}

	return ret;
}

static void iva_ctl_shutdown(struct platform_device *pdev)
{
	struct device		*dev = &pdev->dev;
	struct iva_dev_data	*iva = platform_get_drvdata(pdev);
	unsigned long		state_org = iva->state;

	if (test_bit(IVA_ST_MCU_BOOT_DONE, &iva->state)) {
		iva_mcu_shutdown(iva);
#ifdef CONFIG_PM_SLEEP
		wake_unlock(&iva->iva_wake_lock);
#endif
		clear_bit(IVA_ST_MCU_BOOT_DONE, &iva->state);
	}

	if (test_bit(IVA_ST_INIT_DONE, &iva->state)) {
		iva_deinit(iva);
		clear_bit(IVA_ST_INIT_DONE, &iva->state);
	}

	pm_runtime_disable(dev);

	dev_info(dev, "%s() iva state(0x%lx -> 0x%lx)\n", __func__,
			state_org, iva->state);
	return;
}

#ifdef CONFIG_PM_SLEEP
static int iva_ctl_suspend(struct device *dev)
{
	/* to do */
	dev_info(dev, "%s()\n", __func__);
	return 0;
}

static int iva_ctl_resume(struct device *dev)
{
	/* to do */
	dev_info(dev, "%s()\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_PM
static int iva_ctl_runtime_suspend(struct device *dev)
{
	return __iva_deinit(dev);
}

static int iva_ctl_runtime_resume(struct device *dev)
{
	return __iva_init(dev);
}
#endif

static const struct dev_pm_ops iva_ctl_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(iva_ctl_suspend, iva_ctl_resume)
	SET_RUNTIME_PM_OPS(iva_ctl_runtime_suspend, iva_ctl_runtime_resume, NULL)
};


#ifdef CONFIG_OF
static const struct of_device_id iva_ctl_match[] = {
	{ .compatible = "samsung,iva", .data = (void *)0 },
	{},
};
MODULE_DEVICE_TABLE(of, iva_ctl_match);
#endif

static struct platform_driver iva_ctl_driver = {
	.probe		= iva_ctl_probe,
	.remove		= iva_ctl_remove,
	.shutdown	= iva_ctl_shutdown,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "iva_ctl",
		.pm	= &iva_ctl_pm_ops,
		.of_match_table = of_match_ptr(iva_ctl_match),
	},
};

#ifdef ENABLE_LOCAL_PLAT_DEVICE
static struct resource iva_res[] = {
	{
		/* mcu mbox irq */
		.name	= IVA_MBOX_IRQ_RSC_NAME,
		.start	= IVA_MBOX_IRQ_NOT_DEFINED,
		.end	= IVA_MBOX_IRQ_NOT_DEFINED,
		.flags	= IORESOURCE_IRQ,
	}, {
		.name	= "iva_sfr",
		.start	= IVA_CFG_PHY_BASE_ADDR
		.end	= IVA_CFG_PHY_BASE_ADDR + IVA_CFG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};


static struct iva_dev_data iva_pdata = {
	.mcu_mem_size	= VMCU_MEM_SIZE,
	.mcu_shmem_size	= SHMEM_SIZE,
	.mcu_print_delay= 50,
};

static struct platform_device *iva_ctl_pdev;
#endif

static int __init iva_ctrl_init(void)
{
#ifdef ENABLE_LOCAL_PLAT_DEVICE
	iva_ctl_pdev = platform_device_register_resndata(NULL,
			"iva_ctl", -1, iva_res, ARRAY_SIZE(iva_res),
			&iva_pdata, sizeof(iva_pdata));
#endif
	return platform_driver_register(&iva_ctl_driver);
}

static void __exit iva_ctrl_exit(void)
{
	platform_driver_unregister(&iva_ctl_driver);
#ifdef ENABLE_LOCAL_PLAT_DEVICE
	platform_device_unregister(iva_ctl_pdev);
#endif
}

#ifdef MODULE
module_init(iva_ctrl_init);
#else	/* dummy ion */
device_initcall_sync(iva_ctrl_init);
#endif
module_exit(iva_ctrl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ilkwan.kim@samsung.com");
