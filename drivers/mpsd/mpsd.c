/*
 * Source file for mpsd framework.
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *	   Ravi Solanki <ravi.siso@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/syscore_ops.h>
#include <linux/mpsd/mpsd.h>

#include "mpsd_param.h"

#define MODULE_NAME	   "mpsd"

static atomic_t is_mpsd_enabled = ATOMIC_INIT(0);
static atomic_t wakeup_user = ATOMIC_INIT(0);
static atomic_t suspend = ATOMIC_INIT(0);

static struct mpsd_device mpsd_dev;
static DECLARE_WAIT_QUEUE_HEAD(queue_wait);

/**
 * struct mpsd_device - Contains the layout for shared memory.
 * @dev: Device driver structure.
 * @kref: Number of references kept to this buffer.
 * @mmap_buf: Shared buffer area contains the layout.
 * @uaddr: Virtual userspace start address.
 * @order: 2^order = number of pages allocated.
 * @len: Length of memory mapped to user
 * @app_field_mask: Bitmap for registration of app parameters.
 * @dev_field_mask: Bitmap for registration of device parameters.
 * @behavior: Behavior flag, Set 0 for specific params, 1 for all params.
 *
 * This structure provides the layout for the shared memory to be read from UMD
 */
struct mpsd_device {
	struct miscdevice dev;
	struct kref kref;
	struct memory_area *mmap_buf;
	uintptr_t uaddr;
	unsigned int order;
	unsigned int len;
	unsigned long long app_field_mask;
	unsigned long long dev_field_mask;
	unsigned int behavior;
};

bool get_mpsd_flag(void)
{
	return (bool)atomic_read(&is_mpsd_enabled);
}

static inline void set_mpsd_flag(int val)
{
	atomic_set(&is_mpsd_enabled, val);
}

bool get_wakeup_flag(void)
{
	return (bool)atomic_read(&wakeup_user);
}

static inline void set_wakeup_flag(int val)
{
	atomic_set(&wakeup_user, val);
}

static bool get_suspend_flag(void)
{
	return (bool)atomic_read(&suspend);
}

static inline void set_suspend_flag(int val)
{
	atomic_set(&suspend, val);
}

u32 get_behavior(void)
{
	return mpsd_dev.behavior;
}

u64 get_app_field_mask(void)
{
	return mpsd_dev.app_field_mask;
}

u64 get_dev_field_mask(void)
{
	return mpsd_dev.dev_field_mask;
}

struct memory_area *get_device_mmap(void)
{
	return mpsd_dev.mmap_buf;
}

#ifdef CONFIG_MPSD_DEBUG
void debug_set_mpsd_flag(int val)
{
	if (get_debug_level_driver() <= get_mpsd_debug_level())
		set_mpsd_flag(val);
}

void debug_set_mpsd_behavior(unsigned int val)
{
	if (get_debug_level_driver() <= get_mpsd_debug_level())
		mpsd_dev.behavior = val;
}

int debug_create_mmap_area(void)
{
	int ret = 0;

	if (get_debug_level_driver() <= get_mpsd_debug_level() &&
		get_mpsd_flag()) {
		if (!mpsd_dev.mmap_buf) {
			mpsd_dev.mmap_buf = kmalloc(
				sizeof(struct memory_area), GFP_KERNEL);
			if (!mpsd_dev.mmap_buf)
				ret = -ENOMEM;
		}
	}

	return ret;
}
#endif

/* This should be called from notifier code to update UMD */
void mpsd_notify_umd(void)
{
	set_wakeup_flag(1);
	wake_up(&queue_wait);

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s: Wakeup User, flag=%d\n", tag, __func__,
			atomic_read(&wakeup_user));
	}
#endif
}

static int mpsd_flush(struct file *file, fl_owner_t id)
{
	int ret = 0;

	pr_info("%s: %s:\n", tag, __func__);
	if (!get_mpsd_flag()) {
		pr_err("%s: %s: Open not done!\n", tag, __func__);
		return -ENODEV;
	}

	mpsd_timer_deregister_all();
	mpsd_notifier_release();
	mpsd_notify_umd();
	mpsd_read_stop_cpuload_work();
	set_mpsd_flag(0);

	pr_info("%s: %s: Done!\n", tag, __func__);
	return ret;
}

static int mpsd_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	pr_info("%s: %s:\n", tag, __func__);
	if (!get_mpsd_flag()) {
		pr_err("%s: %s: Open not done!\n", tag, __func__);
		return -ENODEV;
	}

	mpsd_timer_deregister_all();
	mpsd_notifier_release();
	mpsd_notify_umd();
	mpsd_read_stop_cpuload_work();
	set_mpsd_flag(0);

	pr_info("%s: %s: Done!\n", tag, __func__);
	return ret;
}

static int mpsd_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	pr_info("%s: %s:\n", tag, __func__);
	if (get_mpsd_flag()) {
		pr_err("%s: %s: Already Enabled!\n", tag, __func__);
		return -EBUSY;
	}

	if (file->private_data == NULL)
		file->private_data = &mpsd_dev;

	mpsd_read_start_cpuload_work();
	mpsd_notifier_init();
	set_wakeup_flag(0);
	set_suspend_flag(0);
	set_mpsd_flag(1);

	pr_info("%s: %s: Done!\n", tag, __func__);
	return ret;
}

static long mpsd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct mpsd_req msg;
	struct mpsd_device *pmpsd = filp->private_data;

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
		pr_info("%s: %s: Enter!\n", tag, __func__);
#endif

	if (_IOC_SIZE(cmd) > sizeof(msg)) {
		pr_err("%s: %s: Failed! sizes => user=%u kernel=%lu\n",
		tag, __func__, _IOC_SIZE(cmd), sizeof(msg));
		return -EINVAL;
	}

	switch (cmd) {
	case MPSD_IOC_REGISTER:
	{
		if (copy_from_user(&msg, (void __user *)arg, _IOC_SIZE(cmd))) {
			pr_err("%s: %s: %d: MPSD_IOC_REGISTER: Copy Failed!\n",
				tag, __func__, __LINE__);
			return -EFAULT;
		}

		if (!is_valid_param(msg.param_id)) {
			pr_err("%s: %s: MPSD_IOC_REGISTER: Invalid Param!\n",
				tag, __func__);
			return -EINVAL;
		}

		if (is_valid_app_param(msg.param_id))
			set_param_bit(&mpsd_dev.app_field_mask, msg.param_id);
		else if (is_valid_dev_param(msg.param_id))
			set_param_bit(&mpsd_dev.dev_field_mask, msg.param_id);

		switch (msg.id) {
		case SYNC_READ:
			// Nothing is needed actually we are already
			// setting the flag so it will be checked while
			// read IOCTL
#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
				pr_info("%s: %s: MPSD_IOC_REGISTER:(%d): ReqID=%d\n", tag, __func__, msg.param_id, msg.id);
#endif
			break;
		case TIMED_UPDATE:
			// Need to register with timer subsystem
#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
				pr_info("%s: %s: MPSD_IOC_REGISTER:(%d): ReqID=%d Val=%d\n", tag, __func__, msg.param_id, msg.id, msg.data.val);
#endif
			ret = mpsd_timer_register(msg.param_id,
				(int)msg.data.val);
			if (ret)
				return ret;
			break;
		case NOTIFICATION_UPDATE:
			// Need to reigster with notifier subsystem
#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: MPSD_IOC_REGISTER:(%d): ReqID=%d P=%d E=%d (%lld %lld %lld)\n",
					tag, __func__, msg.param_id, msg.id,
					msg.data.info.param,
					msg.data.info.events,
					msg.data.info.delta,
					msg.data.info.max_threshold,
					msg.data.info.min_threshold);
			}
#endif
			mpsd_register_for_event_notify(&msg.data.info);
			break;
		default:
			pr_err("%s: %s: MPSD_IOC_REGISTER: Invalid ReqID\n",
				tag, __func__);
			return -EFAULT;
		}
		break;
	}
	case MPSD_IOC_UNREGISTER:
	{
		if (copy_from_user(&msg, (void __user *)arg, _IOC_SIZE(cmd))) {
			pr_err("%s: %s: MPSD_IOC_UNREGISTER: Copy Failed!\n",
				tag, __func__);
			return -EFAULT;
		}

		if (!is_valid_param(msg.param_id)) {
			pr_err("%s: MPSD_IOC_UNREGISTER: Invalid Param!\n",
				__func__);
			return -EINVAL;
		}

		if (is_valid_app_param(msg.param_id))
			clear_param_bit(&mpsd_dev.app_field_mask, msg.param_id);
		else if (is_valid_dev_param(msg.param_id))
			clear_param_bit(&mpsd_dev.dev_field_mask, msg.param_id);

		switch (msg.id) {
		case SYNC_READ:
			// Nothing is needed actually we are already setting
			// the flag so it will be checked while read IOCTL.
#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: MPSD_IOC_UNREGISTER:(%d): ReqID=%d\n",
					tag, __func__, msg.param_id, msg.id);
			}
#endif
			break;
		case TIMED_UPDATE:
			// Need to register with timer subsystem
#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: MPSD_IOC_UNREGISTER:(%d): ReqID=%d Val=%d\n",
					tag, __func__, msg.param_id, msg.id,
					msg.data.val);
			}
#endif
			mpsd_timer_deregister(msg.param_id);
			break;
		case NOTIFICATION_UPDATE:
			// Need to reigster with notifier subsystem
#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_info("%s: %s: MPSD_IOC_UNREGISTER:(%d): ReqID=%d P=%d E=%d D=%lld Max=%lld Min=%lld\n",
					tag, __func__, msg.param_id, msg.id,
					msg.data.info.param,
					msg.data.info.events,
					msg.data.info.delta,
					msg.data.info.max_threshold,
					msg.data.info.min_threshold);
			}
#endif
			mpsd_unregister_for_event_notify(&msg.data.info);
			break;
		default:
			pr_err("%s: %s: MPSD_IOC_UNREGISTER: Invalid ReqID\n",
				tag, __func__);
			return -EFAULT;
		}
		break;
	}
	case MPSD_IOC_READ:
	{
		/* Syncrnous read */
		/* Populate the MMAPed area */
		unsigned int pid = DEFAULT_PID;
#ifdef CONFIG_MPSD_DEBUG
		struct timespec ts_b, ts_a, latency;
#endif

		if (copy_from_user(&pid, (void __user *)arg, _IOC_SIZE(cmd))) {
			pr_err("%s: %s: MPSD_IOC_READ: Copy Failed!\n",
				tag, __func__);
			return -EFAULT;
		}

		if (!pmpsd->uaddr) {
			pr_err("%s: %s: MPSD_IOC_READ: memory not mapped!\n",
				tag, __func__);
			return -ENOMEM;
		}

#ifdef CONFIG_MPSD_DEBUG
		if (get_debug_level_driver() <= get_mpsd_debug_level())
			getnstimeofday(&ts_b);
#endif

		if (pid <= 0)
			pid = DEFAULT_PID;

		mpsd_populate_mmap(pmpsd->app_field_mask,
			pmpsd->dev_field_mask, pid, SYNC_READ, DEFAULT_EVENTS);

#ifdef CONFIG_MPSD_DEBUG
		if (get_debug_level_driver() <= get_mpsd_debug_level()) {
			getnstimeofday(&ts_a);
			latency.tv_sec = ts_a.tv_sec - ts_b.tv_sec;
			latency.tv_nsec = ts_a.tv_nsec - ts_b.tv_nsec;
			pr_info("%s: %s: MPSD_IOC_READ: PID[%u] CONFIG[%u]  LATENCY[%lld.%.9ld]\n",
				tag, __func__, pid, mpsd_dev.behavior,
				(long long)latency.tv_sec, latency.tv_nsec);
		}

		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
			pr_info("%s: %s: MPSD_IOC_READ: PID=%u\n",
				tag, __func__, pid);
		}
#endif
		break;
	}
	case MPSD_IOC_VERSION:
	{
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
			pr_info("%s: %s: MPSD_IOC_VERSION: VER=%d\n",
				tag, __func__, MPSD_KMD_VERSION);
		}
#endif
		return MPSD_KMD_VERSION;
	}
	case MPSD_IOC_CONFIG:
	{
		unsigned int config = DEFAULT_CONFIG;

		if (copy_from_user(&config, (void __user *)arg,
			_IOC_SIZE(cmd))) {
			pr_err("%s: %s: MPSD_IOC_CONFIG: Copy Failed!\n",
				tag, __func__);
			return -EFAULT;
		}

		if (config == 1 || config == 0)
			mpsd_dev.behavior = config;

#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
			pr_info("%s: %s: MPSD_IOC_CONFIG: B[%d]\n",
				tag, __func__, config);
		}
#endif
		break;
	}
	case MPSD_IOC_SET_PID:
	{
		unsigned int pid = DEFAULT_PID;

		if (copy_from_user(&pid, (void __user *)arg, _IOC_SIZE(cmd))) {
			pr_err("%s: %s: MPSD_IOC_SET_PID: Copy Failed!\n",
				tag, __func__);
			return -EFAULT;
		}

#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
			pr_info("%s: %s: MPSD_IOC_SET_PID: PID=%u\n", tag,
				__func__, pid);
		}
#endif

		if (!is_task_valid(pid)) {
#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_err("%s: %s: MPSD_IOC_SET_PID: Task not found, setting default pid!\n",
					tag, __func__);
			}
#endif
			pid = DEFAULT_PID;
		}

		mpsd_set_timer_pid(pid);
		mpsd_set_notifier_pid(pid);

		break;
	}
	default:
		pr_err("%s: %s: default: Invalid IOCTL!\n", tag, __func__);
		return -ENOENT;
	}

	return ret;
}

/* Wait for the event when there is update this thread will be woken up */
static unsigned int mpsd_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
		pr_info("%s: %s: Waiting\n", tag, __func__);
#endif
	poll_wait(filp, &queue_wait, wait);

	if (atomic_cmpxchg(&wakeup_user, 1, 0))
		mask = POLLOUT | POLLWRNORM;
	else
		mask = 0;

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s: Wakeup Flag=%d\n", tag, __func__,
			atomic_read(&wakeup_user));
	}
#endif

	return mask;
}

/* Must only be called by tee_cbuf_put */
static void mpsd_buf_release(struct kref *kref)
{
	struct mpsd_device *mpsd = container_of(kref, struct mpsd_device, kref);

	free_pages((unsigned long long)mpsd->mmap_buf, mpsd->order);
	pr_info("%s: %s: Free pages and release!\n", tag, __func__);
}

static inline void mpsd_buf_get(struct mpsd_device *mpsd)
{
	kref_get(&mpsd->kref);
}

static inline void mpsd_buf_put(struct mpsd_device *mpsd)
{
	kref_put(&mpsd->kref, mpsd_buf_release);
}

/*
 * This callback is called on remap
 */
static void mpsd_vm_open(struct vm_area_struct *vmarea)
{
	struct mpsd_device *mpsd = vmarea->vm_private_data;

	mpsd_buf_get(mpsd);
}

/*
 * This callback is called on unmap
 */
static void mpsd_vm_close(struct vm_area_struct *vmarea)
{
	struct mpsd_device *mpsd = vmarea->vm_private_data;

	mpsd_buf_put(mpsd);
}

static const struct vm_operations_struct vm_ops = {
	.close = mpsd_vm_close,
	.open = mpsd_vm_open,
};

static int mpsd_map(struct vm_area_struct *vmarea, uintptr_t addr, u32 len,
	uintptr_t *uaddr)
{
	int ret;

	if (!uaddr) {
		pr_err("%s: %s: uaddr is Null!\n", tag, __func__);
		return -EINVAL;
	}

	if (!vmarea) {
		pr_err("%s: %s: vmarea is Null!\n", tag, __func__);
		return -EINVAL;
	}

	if (!addr) {
		pr_err("%s: %s: addr is Null!\n", tag, __func__);
		return -EINVAL;
	}

	if (len != (u32)(vmarea->vm_end - vmarea->vm_start)) {
		pr_err("%s: %s: Buffer incompatible!\n", tag, __func__);
		return -EINVAL;
	}

	vmarea->vm_flags = (vmarea->vm_flags) | (VM_IO | VM_DONTEXPAND |
		(VM_READ & ~VM_WRITE & ~VM_EXEC));

	ret = remap_pfn_range(vmarea, vmarea->vm_start,
		page_to_pfn(virt_to_page(addr)),
		vmarea->vm_end - vmarea->vm_start,
		vmarea->vm_page_prot);

	if (ret) {
		*uaddr = 0;
		pr_err("%s: %s: User mapping Failed!\n", tag, __func__);
		return ret;
	}
	*uaddr = vmarea->vm_start;

	return 0;
}

int mpsd_buffer_create(struct mpsd_device *mpsd, u32 len,
	struct memory_area *addr, struct vm_area_struct *vmarea)
{
	int err = 0;
	unsigned int order;

	if (!mpsd) {
		pr_err("%s: %s: mpsd device is Null!\n", tag, __func__);
		return -EINVAL;
	}

	if (!len || (len > BUFFER_LENGTH_MAX)) {
		pr_err("%s: %s:size missmatch len=%d A=%lu VM(%p %lu %lu)\n",
			tag, __func__, len, sizeof(struct memory_area),
			vmarea, vmarea->vm_end, vmarea->vm_start);
		return -EINVAL;
	}

	order = get_order(len);
	if (order > MAX_ORDER) {
		pr_err("%s: %s: Invalid mem order!%d\n", tag, __func__, order);
		return -ENOMEM;
	}

	mpsd->mmap_buf = (struct memory_area *)__get_free_pages(GFP_USER |
		__GFP_ZERO, order);
	if (!mpsd->mmap_buf) {
		pr_err("%s: %s: Memory not available!\n", tag, __func__);
		return -ENOMEM;
	}

	err = mpsd_map(vmarea, (uintptr_t)mpsd->mmap_buf, len, &mpsd->uaddr);
	if (err) {
		pr_err("%s: %s: mpsd_map failed! error:%d\n",
			tag, __func__, err);
		free_pages((uintptr_t)mpsd->mmap_buf, order);
		return err;
	}

	kref_init(&mpsd->kref);
	mpsd->len = len;
	mpsd->order = order;

	if (vmarea) {
		pr_info("%s: %s: Mmap Done!\n", tag, __func__);
		vmarea->vm_private_data = mpsd;
		vmarea->vm_ops = &vm_ops;
	}

	return err;
}

static int mpsd_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct mpsd_device *pmpsd = filp->private_data;

	if ((vma->vm_end - vma->vm_start) > BUFFER_LENGTH_MAX) {
		pr_err("%s: %s: Invalid vma buf size!\n", tag, __func__);
		return -EINVAL;
	}

	pr_info("%s: %s: Mmap Called\n", tag, __func__);

	/* Alloc contiguous buffer for this client */
	return mpsd_buffer_create(pmpsd, (u32)(vma->vm_end - vma->vm_start),
		mpsd_dev.mmap_buf, vma);
}

#ifdef CONFIG_PM_SLEEP
static int mpsd_suspend(void)
{
	if (!get_mpsd_flag()) {
		pr_info("%s: %s: MPSD not running\n", tag, __func__);
		return 0;
	}

	mpsd_timer_suspend();
	atomic_cmpxchg(&suspend, 0, 1);

	pr_info("%s: %s: Done!\n", tag, __func__);
	return 0;
}

static void mpsd_resume(void)
{
	if (!get_suspend_flag()) {
		pr_info("%s: %s: Suspend Not Done!\n", tag, __func__);
		return;
	}

	mpsd_timer_resume();
	atomic_cmpxchg(&suspend, 1, 0);

	pr_info("%s: %s: Done!\n", tag, __func__);
}
#else
#define mpsd_suspend NULL
#define mpsd_resume NULL
#endif

static struct syscore_ops mpsd_pm_ops = {
	.suspend = mpsd_suspend,
	.resume = mpsd_resume,
};

static const struct file_operations mpsd_fops = {
	.owner = THIS_MODULE,
	.open = mpsd_open,
	.release = mpsd_release,
	.unlocked_ioctl = mpsd_ioctl,
	.mmap = mpsd_mmap,
	.poll = mpsd_poll,
	.flush = mpsd_flush,
};

static int __init mpsd_init(void)
{
	int ret = 0;

	set_mpsd_flag(0);
	set_wakeup_flag(0);
	set_suspend_flag(0);

	memset((void *)&mpsd_dev, 0, sizeof(struct mpsd_device));
	mpsd_dev.dev.minor = MISC_DYNAMIC_MINOR;
	mpsd_dev.dev.name = MODULE_NAME;
	mpsd_dev.dev.fops = &mpsd_fops;
	mpsd_dev.dev.parent = NULL;
	mpsd_dev.app_field_mask = DEFAULT_APP_FIELD_MASK;
	mpsd_dev.dev_field_mask = DEFAULT_DEV_FIELD_MASK;
	mpsd_dev.behavior = DEFAULT_CONFIG;

	ret = misc_register(&mpsd_dev.dev);
	if (ret) {
		pr_err("%s: %s: Reg Failed!\n", tag, __func__);
		return ret;
	}

#ifdef CONFIG_MPSD_DEBUG
	ret = mpsd_debug_init();
	if (ret) {
		misc_deregister(&mpsd_dev.dev);
		pr_err("%s: %s: debug init failed!\n", tag, __func__);
		return ret;
	}
#endif
	mpsd_read_init();
	mpsd_timer_init();

	register_syscore_ops(&mpsd_pm_ops);

	pr_info("%s: %s: Done!\n", tag, __func__);
	return ret;
}

static void __exit mpsd_exit(void)
{
#ifdef CONFIG_MPSD_DEBUG
	mpsd_debug_exit();
#endif
	mpsd_read_exit();
	mpsd_timer_exit();
	unregister_syscore_ops(&mpsd_pm_ops);
	misc_deregister(&mpsd_dev.dev);
	pr_info("%s: %s: Done!\n", tag, __func__);
}
module_init(mpsd_init);
module_exit(mpsd_exit);
MODULE_DESCRIPTION("MPSD Muliparameter Single Decision Module");
MODULE_LICENSE("GPL");
