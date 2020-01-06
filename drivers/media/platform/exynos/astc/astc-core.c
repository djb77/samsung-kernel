/* drivers/media/platform/exynos/astc/astc-core.c
 *
 * Core file for Samsung ASTC encoder driver
 *
 * Copyright (c) 2017-2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrea Bernabei <a.bernabei@samsung.com>
 * Author: Ramesh Munikrishnappa <ramesh.munikrishn@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//#define DEBUG
//#define HWASTC_PROFILE_ENABLE

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/kmod.h>
#include <linux/time.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/exynos_iovmm.h>
#include <asm/page.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/time.h>
#include <linux/pm_runtime.h>

/* UAPI header */
#include <linux/hwastc.h>

#include "astc-core.h"
#include "astc-regs.h"
#include "astc-helper.h"

#include "astc-exynos9810-workaround.h"

/* The name of the gate clock. This is set in the device-tree entry */
static const char *clk_producer_name = "gate";

#ifdef HWASTC_PROFILE_ENABLE
static struct timeval global_time_start;
static struct timeval global_time_end;
#define HWASTC_PROFILE(statement, msg, dev) \
	do { \
		/* Use local structs to support nesting HWASTC_PROFILE calls */\
		struct timeval time_start; \
		struct timeval time_end; \
		do_gettimeofday(&time_start); \
		statement; \
		do_gettimeofday(&time_end); \
		dev_info(dev, msg ": %ld microsecs\n", \
			1000000 * (time_end.tv_sec - time_start.tv_sec) + \
			(time_end.tv_usec - time_start.tv_usec)); \
	} while (0)
#else
#define HWASTC_PROFILE(statement, msg, dev) \
	do { \
		statement; \
	} while (0)
#endif

/*
 * Here is a brief overview of the encoding flow to help new contributors
 * get started with understanding the code.
 * Start from these functions and get deeper as you progress.
 *
 * All functions are thoroughly documented, you will find more details at the
 * top of their implementation.
 *
 * Encoding flow:
 * astc_ioctl -> astc_process -> task_setup -> astc_buffer_setup -> ...
 *            -> queue task on workqueue
 *            -> workqueue worker calls astc_task_schedule when its turn comes
 *            -> astc_task_execute to actually execute the task
 *            -> wait for IRQ signalling encoding completion
 *            -> astc_irq_handler
 *            -> astc_task_signal_completion
 *            -> undo mappings, release resources...
 *            -> return to userspace, IOCTL call complete.
 */

/**
 * astc_set_source()
 * @astc_dev: the astc device struct
 * @task: the task that is about to be executed
 *
 * Set the H/W register representing the DMA address of the source buffer.
 */
static int astc_set_source(struct astc_dev *astc_dev,
			   struct astc_task *task)
{
	dma_addr_t addr = 0;

	if (!astc_dev) {
		pr_err("%s: invalid astc device!\n", __func__);
		return -EINVAL;
	}
	if (!task) {
		dev_err(astc_dev->dev, "%s: invalid task!\n", __func__);
		return -EINVAL;
	}

	addr = task->dma_buf_out.plane.dma_addr;

	dev_dbg(astc_dev->dev, "%s: BEGIN setting source\n", __func__);

	/* set source address */
	astc_hw_set_src_addr(astc_dev->regs, addr);

	dev_dbg(astc_dev->dev, "%s: END source set\n", __func__);

	return 0;
}

/**
 * astc_set_target() - Write info about the target buffer in the H/W registers
 * @astc_dev: the astc device struct
 * @task: the task to process
 *
 * Set the H/W register representing the DMA address of the target buffer
 * and the number of ASTC blocks alongside X and Y axes needed to encode
 * the input image with the given encoding settings.
 */
static int astc_set_target(struct astc_dev *astc_dev,
			   struct astc_task *task)
{
	dma_addr_t addr = 0;

	if (!astc_dev) {
		pr_err("%s: invalid astc device!\n", __func__);
		return -EINVAL;
	}
	if (!task) {
		dev_err(astc_dev->dev, "%s: invalid task!\n", __func__);
		return -EINVAL;
	}
	if (!task->ctx) {
		dev_err(astc_dev->dev, "%s: invalid task context!\n", __func__);
		return -EINVAL;
	}

	dev_dbg(astc_dev->dev, "%s: BEGIN setting target\n", __func__);

	addr = task->dma_buf_cap.plane.dma_addr;

	/* set source width, height and number of blocks */
	astc_hw_set_image_size_num_blocks(astc_dev->regs
					  , task->user_task.info_cap.width
					  , task->user_task.info_cap.height
					  , task->ctx->blocks_on_x
					  , task->ctx->blocks_on_y);

	/* set target address */
	astc_hw_set_dst_addr(astc_dev->regs, addr);

	dev_dbg(astc_dev->dev, "%s: END\n", __func__);

	return 0;
}

/* Pack the encoding configuration in a u32 that can be written to
 * the config reg */
static int astc_pack_config(struct astc_ctx *context,
		       struct astc_task *task, u32 *ret)
{
	struct device *dev = NULL;
	struct hwASTC_config *cfg   = NULL;

	if (!context) {
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}

	dev = context->astc_dev->dev;
	cfg = &task->user_task.enc_config;

	if (!ret) {
		dev_err(dev, "%s: invalid return val address!\n", __func__);
		return -EINVAL;
	}

	dev_dbg(dev, "Packing Configuration received from userspace:");
	dev_dbg(dev, "ENCODING CONFIGURATION\n");
	dev_dbg(dev, "Color format reg val:      %u\n", task->user_task.info_out.fmt);
	dev_dbg(dev, "Dual plane reg val:        %d\n", cfg->dual_plane_enable);
	dev_dbg(dev, "Refine iterations reg val: %u\n", cfg->intref_iterations);
	dev_dbg(dev, "Partitions reg val:        %u\n", cfg->partitions);
	dev_dbg(dev, "Block mode:                %u\n", cfg->num_blk_mode);
	dev_dbg(dev, "Block size:                %u\n", cfg->encodeBlockSize);

	*ret = 0 | ((task->user_task.info_out.fmt   & (KBit0 | KBit1)) << ASTC_CTRL_REG_COLOR_FORMAT_INDEX)
			| ((cfg->dual_plane_enable &  KBit0)          << ASTC_CTRL_REG_DUAL_PLANE_INDEX)
			| ((cfg->intref_iterations & (KBit0 | KBit1)) << ASTC_CTRL_REG_REFINE_ITER_INDEX)
			| ((cfg->partitions        &  KBit0)          << ASTC_CTRL_REG_PARTITION_EN_INDEX)
			| ((cfg->num_blk_mode      &  KBit0)          << ASTC_CTRL_REG_NUM_BLK_MODE_INDEX)
			| ((cfg->encodeBlockSize   & (KBit0 | KBit1)) << ASTC_CTRL_REG_ENC_BLK_SIZE_INDEX);

	return 0;
}

/**
 * set_enc_config() - Write the encoding configuration in the H/W register
 * @ctx: the context of the task that is being processed
 * @packed_config: the encoding configuration, packed in a u32
 */
static void astc_set_enc_config(struct astc_ctx *ctx, u32 packed_config)
{
	astc_hw_set_enc_mode(ctx->astc_dev->regs, packed_config);

	dump_config_register(ctx->astc_dev);
}

/**
 * astc_task_execute() - Set H/W registers and trigger H/W to execute a task
 * @astc_context: the context of the task to execute
 * @task: the task that is about to be executed
 *
 * Set H/W registers and trigger H/W to execute a task. The H/W will fire an
 * interrupt when it is done encoding.
 * See @astc_irq_handler.
 *
 * Return:
 *	 0 on success
 *	<0 error num otherwise
 */
static int astc_task_execute(struct astc_ctx *astc_context,
			   struct astc_task *task)
{
	struct astc_dev *astc = astc_context->astc_dev;
	u32 packed_config = 0;
	int ret = 0;

	if (!astc) {
		pr_err("hwastc: astc is null\n");
		return -EINVAL;
	}

	dev_dbg(astc_context->astc_dev->dev, "%s: BEGIN\n", __func__);

	/*
	 * This should theoretically never happen because this is called from
	 * a workqueue job function, and workqueues won't let the system go into
	 * sleep until their active job queue is empty.
	 * See workqueue official documentation for more details
	 */
	if (test_bit(DEV_SUSPEND, &astc->state)) {
		dev_err(astc->dev,
			"%s: unexpected suspended device state right before running a task\n", __func__);
		return -1;
	}

	dev_dbg(astc->dev, "%s: resetting astc device\n", __func__);

	/* Reset all registers to reset values */
	astc_hw_soft_reset(astc->regs);

	dev_dbg(astc->dev, "%s: setting source/target addresses\n", __func__);

	ret = astc_set_source(astc, task);
	if (ret) {
		dev_err(astc->dev, "%s: error setting source\n", __func__);
		return ret;
	}

	ret = astc_set_target(astc, task);
	if (ret) {
		dev_err(astc->dev, "%s: error setting target\n", __func__);
		return ret;
	}

	dev_dbg(astc->dev, "%s: enabling interrupts\n", __func__);
	astc_hw_enable_interrupt(astc->regs);

	dev_dbg(astc->dev, "%s: setting encoding configuration\n", __func__);

	/* Pack the config received from userspace in u32 as expected by H/W */
	ret = astc_pack_config(astc_context, task, &packed_config);
	if (ret) {
		dev_err(astc->dev, "%s: error setting configuration\n"
			, __func__);
		return ret;
	}

	astc_set_enc_config(astc_context, packed_config);

	/*
	 * 8 bursts (value 2) is supposed to be more stable than
	 * the default 16, according to the H/W firmware developer
	 */
	astc_hw_set_axi_burst_len(astc->regs, 2);

	dump_regs(astc_context->astc_dev);

	dev_dbg(astc->dev, "%s: starting actual encoding process\n", __func__);

#ifdef HWASTC_PROFILE_ENABLE
	do_gettimeofday(&global_time_start);
#endif

	astc_hw_start_enc(astc->regs);

	dev_dbg(astc->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * astc_task_signal_completion() - Notify that the task processing is complete
 * @astc_device: the astc device struct
 * @task: the task that whose processing has been completed
 * @success: whether the task processing was successful or not
 *
 * Update the task state according to the value of @success and wake up
 * the worker that is blocked waiting for the completion of @task.
 *
 * Return:
 *	 0 on success,
 *	<0 if there is an error.
 */
static int astc_task_signal_completion(struct astc_dev *astc_device,
				       struct astc_task *task, bool success)
{
	unsigned long flags;

	if (!astc_device) {
		pr_err("hwastc: NULL astc device!\n");
		return -EINVAL;
	}
	if (!task) {
		dev_err(astc_device->dev
			, "%s; finishing a NULL task!\n", __func__);
		return -EINVAL;
	}

	spin_lock_irqsave(&astc_device->lock_task, flags);
	dev_dbg(astc_device->dev
		, "%s: resetting current task var...\n", __func__);

	if (!astc_device->current_task) {
		dev_warn(astc_device->dev
			 , "%s: invalid current task!\n", __func__);
	}
	if (astc_device->current_task != task) {
		dev_warn(astc_device->dev
			 , "%s: finishing an unexpected task!\n", __func__);
	}

	astc_device->current_task = NULL;
	spin_unlock_irqrestore(&astc_device->lock_task, flags);

	task->state = success ? ASTC_BUFSTATE_DONE : ASTC_BUFSTATE_ERROR;

	dev_dbg(astc_device->dev
		, "%s: signalling task completion...\n", __func__);

	/* wake up the worker that was waiting for the task to be processed */
	complete(&task->complete);

	return 0;
}

/**
 * astc_irq_handler() - The handler of IRQ interrupts
 * @irq: the IRQ interrupt code
 * @priv: the astc device struct
 *
 * Handle the IRQ interrupt that the H/W sends when it is done processing
 * a task.
 *
 * Signal task completion and whether it was successful or not, and then
 * schedule the next task for execution.
 *
 * Return:
 *	IRQ_NONE if the IRQ was a spurious one,
 *	IRQ_HANDLED otherwise
 */
static irqreturn_t astc_irq_handler(int irq, void *priv)
{
	unsigned long flags;
	unsigned int int_status;
	struct astc_dev *astc = priv;
	struct astc_task *task;
	bool status = true;

#ifdef HWASTC_PROFILE_ENABLE
	do_gettimeofday(&global_time_end);
	dev_info(astc->dev, "ENCODING TIME: %ld millisec\n",
		 1000 * (global_time_end.tv_sec - global_time_start.tv_sec) +
		 (global_time_end.tv_usec - global_time_start.tv_usec) / 1000);
#endif

	dev_dbg(astc->dev, "%s: BEGIN\n", __func__);

	dev_dbg(astc->dev, "%s: getting irq status\n", __func__);

	int_status = astc_hw_get_int_status(astc->regs);
	//Check that the board intentionally triggered the interrupt
	if (!int_status) {
		dev_warn(astc->dev, "bogus interrupt\n");
		return IRQ_NONE;
	}

	dev_dbg(astc->dev, "%s: clearing irq status\n", __func__);

	astc_hw_clear_interrupt(astc->regs);

	dev_dbg(astc->dev, "%s: getting hw status\n", __func__);

	/* Was the processing successful? */
	if (get_hw_enc_status(astc->regs) != 0) {
		status = false;
		dev_err(astc->dev, "astc encoder bus error, resetting HW\n");
		astc_hw_soft_reset(astc->regs);
	}

	spin_lock_irqsave(&astc->lock_task, flags);
	task = astc->current_task;
	if (!task) {
		spin_unlock_irqrestore(&astc->lock_task, flags);
		dev_err(astc->dev, "received null task in irq handler\n");
		return IRQ_HANDLED;
	}
	spin_unlock_irqrestore(&astc->lock_task, flags);

	dev_dbg(astc->dev, "%s: signalling task completion\n", __func__);

	/* unblock the workqueue worker */
	if (astc_task_signal_completion(astc, task, status) < 0) {
		dev_err(astc->dev, "%s: error while signalling task completion!\n"
			, __func__);

		return IRQ_HANDLED;
	}

	dev_dbg(astc->dev, "%s: END\n", __func__);

	return IRQ_HANDLED;
}

/**
 * astc_cachesync_for_device() - Before encoding, sync CPU caches to make sure
 *                               device has access to the most recent data.
 * @dev: the device struct
 * @task: the struct holding information about the encoding task
 *
 * This is needed when the data transfers to/from the device are not coherent.
 * Mainly used before executing an encoding task on chip.
 *
 * On SoCs like Exynos9810 DMA transfers from/to this device are coherent, so
 * this is unused.
 *
 * It might be needed in different SoCs.
 */
static void astc_cachesync_for_device(struct device *dev,
				      struct astc_task *task) {
	if (device_get_dma_attr(dev) == DEV_DMA_NON_COHERENT) {
		dev_dbg(dev, "CACHESYNC OUTPUT CPU\n");
		astc_sync_for_device(dev, &task->dma_buf_out.plane,
				     DMA_TO_DEVICE);

		/*
		 * We need to also sync the cap buffer here.
		 * This will usually invalidate() the CPU cache.
		 *
		 * If we don't do this, the following can happen:
		 * 1. driver syncs the image buffer (the call just above
		 *    this comment)
		 * 2. H/W writes encoded result to the capture buffer
		 * 3. At this point the CPU can still have cache lines holding
		 *    the data of the capture buffer before it was sent to
		 *    the driver. If those lines are evicted, write-back caching
		 *    will write that (now stale) data on the output buffer,
		 *    overwriting (parts of) the encoded result.
		 *
		 * By calling sync_for_device on the capture buffer, we
		 * invalidate the CPU cache lines holding the capture buffer, if
		 * any. That will prevent point 3, and nothing will overwrite
		 * the encoded result.
		 *
		 * This has been tested to be required when the H/W is not
		 * DMA-coherent. On SoCs like Exynos9810, the ASTC H/W is
		 * coherent so this whole section does not apply.
		 */
		astc_sync_for_device(dev, &task->dma_buf_cap.plane,
				     DMA_FROM_DEVICE);
	}
}
/**
 * astc_cachesync_for_cpu() - After encoding, sync CPU caches to make sure the
 *                            CPU has access to the most recent data.
 * @dev: the device struct
 * @task: the struct holding information about the encoding task
 *
 * This is needed when the data transfers to/from the device are not coherent.
 * Mainly used after the chip is done encoding the image, to make sure the CPU
 * has access to the latest data and does not read stale data from its caches.
 *
 * On SoCs like Exynos9810 DMA transfers from/to this device are coherent, so
 * this is unused.
 *
 * It might be needed in different SoCs.
 */
static void astc_cachesync_for_cpu(struct device *dev,
				      struct astc_task *task) {
	if (device_get_dma_attr(dev) == DEV_DMA_NON_COHERENT) {
		dev_dbg(dev, "CACHESYNC CAPTURE CPU\n");
		/*
		 * No need for this because the device has not touched
		 * the buffer.
		 * In fact the underlying sync calls resolve to no-ops when
		 * the direction is DMA_TO_DEVICE.
		 */
		//astc_sync_for_cpu(dev, &task->dma_buf_out.plane, DMA_TO_DEVICE);

		astc_sync_for_cpu(dev, &task->dma_buf_cap.plane,
				  DMA_FROM_DEVICE);
	}
}

/**
 * enable_astc() - Request Runtime PM framework to enable the device
 * @astc: the astc device struct
 *
 * Signal Runtime Power Management framework that we need the device.
 * Runtime PM will eventually trigger the runtime_resume callback if needed
 * (i.e. when the device is asleep and needs to be woken up).
 *
 * Return:
 *	 0 on success,
 *	<0 err num otherwise.
 */
static int enable_astc(struct astc_dev *astc)
{
	int ret;

	dev_dbg(astc->dev, "%s: runtime pm, getting device\n", __func__);

	/* our runtime_resume callback will enable the clock */
	ret = in_irq() ? pm_runtime_get(astc->dev) :
			 pm_runtime_get_sync(astc->dev);
	if (ret < 0) {
		dev_err(astc->dev
			, "%s: failed to increment Power Mgmt usage cnt (%d)\n"
			, __func__, ret);
		return ret;
	}

	//No need to enable the clock, our runtime pm callback will do it

	return 0;
}
/**
 * disable_astc() - Request Runtime PM framework to disable the device
 * @astc: the astc device struct
 *
 * Signal Runtime Power Management framework that we don't need the device
 * anymore. As a consequence, runtime_suspend will be called when appropriate
 * (i.e. there are no other workers interested in the device, so it can
 * be suspended).
 *
 * The autosuspend feature of Runtime PM is used to delay the suspension
 * of the device. That allows us to save time switching the device off and
 * then back in case more requests are sent before the autosuspend timer
 * expires.
 *
 * Return:
 *	 0 on success,
 *	<0 err num otherwise.
 */
static int disable_astc(struct astc_dev *astc)
{
	int ret;

	dev_dbg(astc->dev
		, "%s: disabling clock, decrementing runtime pm usage\n"
		, __func__);

	/* we're assuming our runtime_suspend callback will disable the clock */
	if (in_irq()) {
		dev_dbg(astc->dev
			, "%s: IN IRQ! DELAYED autosuspend\n" , __func__);
		ret = pm_runtime_put_autosuspend(astc->dev);
	} else {
		dev_dbg(astc->dev
			, "%s: SYNC autosuspend\n" , __func__);
		ret = pm_runtime_put_sync_autosuspend(astc->dev);
	}

	if (ret < 0) {
		dev_err(astc->dev
			, "%s: failed to decrement Power Mgmt usage cnt (%d)\n"
			, __func__, ret);
		return ret;
	}
	return 0;
}

/**
 * astc_task_schedule() - schedule a task for execution and wait for completion
 * @task: the task to execute
 *
 * This will be called by the workqueue worker(s), it will setup whatever is
 * left to setup, and then execute the task and block waiting for its
 * completion.
 *
 * The IRQ handler (which is called when the HW is done encoding) will signal
 * completion, allowing this function to return.
 *
 * The workqueue framework will then take care of running any other job in the
 * queue once this is done.
 */
static void astc_task_schedule(struct astc_task *task)
{
	unsigned long flags;
	int ret = 0;
	struct astc_dev *astc_device = task->ctx->astc_dev;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	/*
	 * DEV_RUN at this point should be always unset, as long as
	 * we use a singlethread ordered workqueue.
	 */
	if (test_bit(DEV_RUN, &astc_device->state)) {
		dev_err(astc_device->dev
			, "%s: unexpected running state (ctx %pK task %pK)\n"
			, __func__, task->ctx, task);
	}

	/*
	 * NOTE: SEE DEV_SUSPEND doc to know when this can happen.
	 *
	 * This is to avoid the (unrealistic?) race where wait_event returns
	 * because DEV_SUSPEND has been cleared (i.e. there has just been a
	 * system resume), but another system suspend is triggered before
	 * we manage to take the spinlock and set DEV_RUN.
	 */
	while (true) {
		/*
		 * NOTE: SEE DEV_SUSPEND doc to know when this can happen.
		 *
		 * Using the SPINLOCK here and in suspend callback guarantees
		 * that thare no races with the suspend callback, otherwise the
		 * following might happen:
		 *
		 * T1. HERE:             test_bit(DEV_SUSPEND,...)
		 * T2. SUSPEND CALLBACK: set_bit(DEV_SUSPEND,...)
		 * T3. HERE:             set_bit(DEV_RUN,...)
		 * T4. SUSPEND CALLBACK: wait_event(!DEV_RUN)
		 * T5. HERE:             wait_event(!DEV_SUSPEND)
		 *
		 * Deadlock :)
		 */
		spin_lock_irqsave(&astc_device->slock, flags);
		if (!test_bit(DEV_SUSPEND, &astc_device->state)) {
			set_bit(DEV_RUN, &astc_device->state);
			dev_dbg(astc_device->dev,
				"%s: HW is active, continue.\n", __func__);
			spin_unlock_irqrestore(&astc_device->slock, flags);
			break;
		}
		spin_unlock_irqrestore(&astc_device->slock, flags);

		dev_dbg(astc_device->dev,
			"%s: waiting on system resume...\n", __func__);

		wait_event(astc_device->suspend_wait,
			   !test_bit(DEV_SUSPEND, &astc_device->state));
	}

	ret = enable_astc(astc_device);
	if (ret) {
		task->state = ASTC_BUFSTATE_RETRY;
		goto err;
	}

	dev_dbg(astc_device->dev
		, "%s: INTERRUPT FIRED AND WAITING TO BE HANDLED? %u\n"
		, __func__, astc_hw_get_int_status(astc_device->regs));
	dev_dbg(astc_device->dev, "%s: ENCODING STATUS? %d\n"
		, __func__, get_hw_enc_status(astc_device->regs));

	task->state = ASTC_BUFSTATE_PROCESSING;
	dev_dbg(astc_device->dev, "%s: about to run the task\n", __func__);

	/* make sure the device will see the latest data */
	astc_cachesync_for_device(astc_device->dev, task);

	spin_lock_irqsave(&astc_device->lock_task, flags);
	astc_device->current_task = task;
	spin_unlock_irqrestore(&astc_device->lock_task, flags);

	if (astc_task_execute(task->ctx, task)) {
		task->state = ASTC_BUFSTATE_ERROR;

		spin_lock_irqsave(&astc_device->lock_task, flags);
		astc_device->current_task = NULL;
		spin_unlock_irqrestore(&astc_device->lock_task, flags);

	} else {
		dev_dbg(astc_device->dev
			, "%s: waiting for task to complete, no timeout\n"
			, __func__);

		/*
		 * perform an uninterruptible wait (i.e. ignore signals)
		 * NOTE: this call can sleep, so it cannot be used in IRQ context
		 *
		 * NOTE2: When CONFIG_SUSPEND_FREEZER is set (usually that's the
		 *        default), THIS WILL PREVENT SYSTEM SLEEP UNTIL THE
		 *        TASK IS COMPLETED!
		 *        Take extra care if you are considering changing this
		 *        line.
		 */
		wait_for_completion(&task->complete);
	}

	astc_cachesync_for_cpu(astc_device->dev, task);

	pm_runtime_mark_last_busy(astc_device->dev);
	HWASTC_PROFILE(disable_astc(astc_device), "DISABLE ASTC", astc_device->dev);

err:
	clear_bit(DEV_RUN, &astc_device->state);
	/* unblock system suspend callback if it was waiting on us
	 * to complete */
	if (test_bit(DEV_SUSPEND, &astc_device->state)) {
		wake_up(&astc_device->suspend_wait);
	}

	dev_dbg(astc_device->dev, "%s: END\n", __func__);
	return;
}

/* helper function used to schedule tasks in a workqueue */
static void astc_task_schedule_work(struct work_struct *work)
{
	astc_task_schedule(container_of(work, struct astc_task, work));
}

/**
 * astc_get_dmabuf_from_userptr() - Get the dmabuf linked to a buffer, if any
 * @astc_device: the astc device struct
 * @start: the pointer to the user memory we received from userspace
 * @len: the length of the user memory region
 * @out_offset: the offset of the pointer inside the VMA that holds the
 *		memory region indicated by @start and @len.
 *
 * Find the VMA that holds the input virtual address, and check
 * if the VMA represents the mapping of a dmabuf.
 * If it does, acquire that dmabuf (i.e. increase refcount on its fd).
 * Otherwise, do nothing.
 *
 * Return:
 *	the pointer to the dma_buf struct if any is found,
 *	else return NULL if no dmabuf is associated to the VMA,
 *	else return ERR_PTR(-EINVAL) if no VMA is found for the user
 *	memory region indicated by @start and @len.
 */
static struct dma_buf *astc_get_dmabuf_from_userptr(
		struct astc_dev *astc_device, unsigned long start, size_t len,
		off_t *out_offset)
{
	struct dma_buf *dmabuf = NULL;
	struct vm_area_struct *vma;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	down_read(&current->mm->mmap_sem);
	dev_dbg(astc_device->dev
		, "%s: virtual address space semaphore acquired\n", __func__);

	vma = find_vma(current->mm, start);

	if (!vma || (start < vma->vm_start)) {
		dev_err(astc_device->dev
			, "%s: Incorrect user buffer @ %#lx/%#zx\n"
			, __func__, start, len);
		dmabuf = ERR_PTR(-EINVAL);
		goto finish;
	}

	dev_dbg(astc_device->dev
		, "%s: VMA found, %pK. Starting at %#lx within parent area\n"
		, __func__, vma, vma->vm_start);

	/* vm_file is the file that this vma is a memory mapping of, if any */
	if (!vma->vm_file) {
		dev_dbg(astc_device->dev, "%s: this VMA does not map a file\n",
			__func__);
		goto finish;
	}

	dmabuf = get_dma_buf_file(vma->vm_file);
	dev_dbg(astc_device->dev
		, "%s: got dmabuf %pK of vm_file %pK\n", __func__, dmabuf
		, vma->vm_file);

	/*
	 * no need to check size_t overflow (wrap-around), as we already check
	 * that (vm_start <= start) above
	 */
	if (dmabuf != NULL) {
		*out_offset = start - vma->vm_start;
	}
finish:
	up_read(&current->mm->mmap_sem);
	dev_dbg(astc_device->dev
		, "%s: virtual address space semaphore released\n", __func__);

	return dmabuf;
}

/**
 * astc_buffer_put_and_detach() - Detach the device from a DMA buffer
 * @dma_buffer: the internal struct holding info about a buffer
 *
 * This should be called after unmapping the dma buffer with
 * astc_buffer_unmap.
 *
 * Detach the device from the buffer to signal that we're not
 * interested in it anymore, and decrease the usage refcount of the dmabuf.
 *
 * It only makes sense to use this method for buffers that are backed
 * by a dmabuf, i.e. HWASTC_BUFFER_DMABUF or a HWASTC_BUFFER_USERPTR
 * in cases where the user pointer points to memory backed by a dmabuf
 * (see astc_get_dmabuf_from_userptr()).
 *
 * Detach is at the end of the function name just to make it easier
 * to associate astc_buffer_get_* to astc_buffer_put_*, but it's
 * actually called in the inverse order (i.e. first detach then put).
 */
static void astc_buffer_put_and_detach(struct astc_buffer_dma *dma_buffer)
{
	const struct hwASTC_buffer *user_buffer = dma_buffer->buffer;
	struct astc_buffer_plane_dma *plane = &dma_buffer->plane;

	/*
	 * - buffer type DMABUF:
	 *     in this case dmabuf should always be set
	 * - buffer type USERPTR:
	 *     in this case dmabuf is only set if we reuse any
	 *     preexisting dmabuf (see astc_buffer_get_*)
	 */
	if (user_buffer->type == HWASTC_BUFFER_DMABUF
			|| (user_buffer->type == HWASTC_BUFFER_USERPTR
			    && plane->dmabuf)) {
		dma_buf_detach(plane->dmabuf, plane->attachment);
		dma_buf_put(plane->dmabuf);
		plane->dmabuf = NULL;
	}
}

/**
 * astc_buffer_get_and_attach() - Acquire a buffer and attach the device to it
 * @astc_device: the astc device struct
 * @buffer: the buffer info received from userspace
 * @dma_buffer: the internal buffer struct
 *
 * If the buffer we received from userspace is an fd representing a
 * DMA buffer (HWASTC_BUFFER_DMABUF) then we acquire the DMA fd and attach
 * the device to it.
 *
 * If it's a pointer to user memory (HWASTC_BUFFER_USERPTR), then we check
 * if that memory is already associated to an existing dmabuf. If it is,
 * then it's just like a HWASTC_BUFFER_DMABUF, so we treat it the same.
 *
 * If, instead, it's just a pointer to user memory with no dmabuf
 * associated to it, we don't need to do anything here.
 * We will use IOVMM later to get access to it.
 *
 * Return:
 * 	 0 on success,
 *	<0 otherwise
 */
static int astc_buffer_get_and_attach(struct astc_dev *astc_device,
				      struct hwASTC_buffer *buffer,
				      struct astc_buffer_dma *dma_buffer)
{
	struct astc_buffer_plane_dma *plane;
	int ret = 0;
	off_t offset = 0;

	dev_dbg(astc_device->dev
		, "%s: BEGIN, acquiring plane from buf %pK into dma buffer %pK\n"
		, __func__, buffer, dma_buffer);

	plane = &dma_buffer->plane;

	if (buffer->type == HWASTC_BUFFER_DMABUF) {
		plane->dmabuf = dma_buf_get(buffer->fd);

		dev_dbg(astc_device->dev, "%s: dmabuf of fd %d is %pK\n", __func__
			, buffer->fd, plane->dmabuf);
	} else if (buffer->type == HWASTC_BUFFER_USERPTR) {
		/*
		 * Check if there's already a dmabuf associated to the
		 * chunk of user memory the client is pointing us to
		 */
		plane->dmabuf = astc_get_dmabuf_from_userptr(astc_device,
							     buffer->userptr,
							     buffer->len,
							     &offset);
		dev_dbg(astc_device->dev, "%s: dmabuf of userptr %p is %pK\n"
			, __func__, (void *) buffer->userptr, plane->dmabuf);
	}

	if (IS_ERR(plane->dmabuf)) {
		ret = PTR_ERR(plane->dmabuf);
		dev_err(astc_device->dev,
			"%s: failed to get dmabuf, err %d\n", __func__, ret);
		goto err;
	}

	/* If the buffer is backed by an existing dmabuf (else nothing to do) */
	if (plane->dmabuf) {
		if (plane->dmabuf->size < plane->bytes_used) {
			dev_err(astc_device->dev,
				"%s: driver requires %zu bytes but user gave %zu\n",
				__func__, plane->bytes_used,
				plane->dmabuf->size);
			ret = -EINVAL;
			goto err;
		}

		plane->attachment = dma_buf_attach(plane->dmabuf
						   , astc_device->dev);

		dev_dbg(astc_device->dev
			, "%s: attachment of dmabuf %pK is %pK\n", __func__
			, plane->dmabuf, plane->attachment);

		plane->offset = offset;

		if (IS_ERR(plane->attachment)) {
			dev_err(astc_device->dev,
				"%s: Failed to attach dmabuf\n", __func__);
			ret = PTR_ERR(plane->attachment);
			goto err;
		}
	}

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
err:
	dev_dbg(astc_device->dev
		, "%s: ERROR releasing dma resources\n", __func__);

	if (!IS_ERR(plane->dmabuf)) /* release dmabuf */
		dma_buf_put(plane->dmabuf);

	return ret;
}

/**
 * astc_buffer_unmap() - Unmap a buffer from device address space
 * @ctx: the context of the task that is being processed
 * @dma_buffer: the internal struct holding info about the buffer
 * @dir: the direction of the DMA transfer that involves the buffer
 *
 * This is called after the H/W has finished processing a task and,
 * if the userspace buffer is backed by a dmabuf, before
 * astc_buffer_put_and_detach.
 *
 * Use IOMMU to unmap the buffer from H/W address space and then
 * unmap the DMA attachment, if any.
 *
 * It is called for every buffer that astc_buffer_map was previously
 * called for.
 */
static void astc_buffer_unmap(struct astc_ctx *ctx,
			       struct astc_buffer_dma *dma_buffer,
			       enum dma_data_direction dir)
{
	dev_dbg(ctx->astc_dev->dev, "%s: BEGIN\n", __func__);
	dev_dbg(ctx->astc_dev->dev, "%s: about to unmap the DMA address\n"
		, __func__);

	/*
	 * use IOVMM (which will use IOMMU) to unmap the buffer
	 * from H/W address space
	 */
	if (astc_unmap_buf_from_device(ctx->astc_dev->dev, dma_buffer) != 0) {
		dev_dbg(ctx->astc_dev->dev, "%s: DMA address unampping failed\n"
			, __func__);
	}

	dev_dbg(ctx->astc_dev->dev, "%s: about to unmap the DMA buffer\n"
		, __func__);

	/* unmap the dmabuf, if any, so others can request it */
	astc_unmap_dma_attachment(ctx->astc_dev->dev,
			   &dma_buffer->plane, dir);

	dev_dbg(ctx->astc_dev->dev, "%s: END\n", __func__);
}

/**
 * astc_buffer_map() - Map a buffer to device address space
 * @ctx: the context of the task that is being processed
 * @task: the task that is being processed
 * @dma_buffer: the internal struct holding info about the buffer
 * @dir: the direction of the DMA transfer that involves the buffer
 *
 * Map a buffer to H/W address space to make it accessible by the H/W.
 * That includes things like dma attachment setup and iova mapping
 * (by using IOVMM API, which will use IOMMU).
 *
 * The buffer will be made available to H/W as a contiguous chunk
 * of virtual memory which, thanks to the IOMMU, can be used for
 * DMA transfers even though the physical pages referred by the mapping
 * might be scattered around.
 *
 * If the buffer is backed by a dmabuf, this is called after attaching
 * the device to the dmabuf. See astc_buffer_get_and_attach().
 *
 * Return:
 *	 0 on success
 *	<0 error num otherwise
 */
static int astc_buffer_map(struct astc_ctx *ctx,
			   struct astc_task *task,
			   struct astc_buffer_dma *dma_buffer,
			   enum dma_data_direction dir)
{
	int ret = 0;

	dev_dbg(ctx->astc_dev->dev, "%s: BEGIN\n", __func__);

	dev_dbg(ctx->astc_dev->dev
		, "%s: about to map dma buffer in device address space...\n"
		, __func__);

	/*
	 * map dma attachment:
	 * this is the point where the dma buffer is actually
	 * allocated, if it wasn't there before.
	 * It also means the driver now owns the buffer
	 */
	ret = astc_map_dma_attachment(ctx->astc_dev->dev,
			       &dma_buffer->plane, dir);
	if (ret)
		return ret;

	dev_dbg(ctx->astc_dev->dev, "%s: getting dma address of the buffer...\n"
		, __func__);

	/*
	 * use IOMMU to map the buffer to H/W address space, thus obtaining
	 * a I/O virtual address that can be used for DMA transfers
	 */
	ret = astc_map_buf_to_device(ctx->astc_dev, task, dma_buffer, dir);
	if (ret) {
		dev_dbg(ctx->astc_dev->dev, "%s: mapping FAILED!\n", __func__);
		astc_unmap_dma_attachment(ctx->astc_dev->dev,
				   &dma_buffer->plane, dir);
		return ret;
	}

	dev_dbg(ctx->astc_dev->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * astc_buffer_teardown() - Main procedure for buffer release
 * @astc_device: the astc device struct
 * @ctx: the context of the task that is being processed
 * @dma_buffer: the buffer that will be dismantled
 * @dir: the direction of the DMA transfer that the buffer was used for.
 *
 * This is the entry point for buffer dismantling.
 *
 * It is used whenever we need to dismantle a buffer and release all its
 * resources, for instance after we finish processing a task, independently
 * of whether the processing was successful or not.
 *
 * It unmaps the buffer from the device H/W address space and detaches
 * the device from it (if it's a DMABUF).
 */
static void astc_buffer_teardown(struct astc_dev *astc_device,
				       struct astc_ctx *ctx,
				       struct astc_buffer_dma *dma_buffer,
				       enum dma_data_direction dir)
{
	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	astc_buffer_unmap(ctx, dma_buffer, dir);

	astc_buffer_put_and_detach(dma_buffer);

	dev_dbg(astc_device->dev, "%s: END\n", __func__);
}

/**
 * astc_buffer_setup() - Procedure that takes care of the complete buffer setup
 * @ctx: the context of the task that is being setup
 * @task: the task that is being setup
 * @buffer: the userspace API struct representing the buffer
 * @dma_buffer: the internal struct representing the buffer
 * @dir: the direction of the DMA transfer this buffer will be used for.
 *
 * This is the entry point for complete buffer setup.
 *
 * It does the necessary mapping and allocation of the DMA buffer that
 * the H/W will access.
 *
 * At the end of this function, the buffer is ready for DMA transfers.
 *
 * NOTE: this is needed when the user buffer is a HWASTC_BUFFER_DMABUF as
 * well as when it is a HWASTC_BUFFER_USERPTR, because the H/W ultimately
 * needs a DMA address to operate on.
 *
 * Return:
 *	 0 on success,
 *	<0 error code on failure
 */
static int astc_buffer_setup(struct astc_ctx *ctx,
			     struct astc_task *task,
			     struct hwASTC_buffer *buffer,
			     struct astc_buffer_dma *dma_buffer,
			     enum dma_data_direction dir)
{
	struct astc_dev *astc_device = ctx->astc_dev;
	int ret = 0;
	struct astc_buffer_plane_dma *plane = &dma_buffer->plane;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	dev_dbg(astc_device->dev
		, "%s: computing bytes needed for dma buffer %pK\n"
		, __func__, dma_buffer);

	if (plane->bytes_used == 0) {
		dev_err(astc_device->dev,
			"%s: BUG! driver expects 0 bytes! (user gave %zu)\n",
			__func__, buffer->len);
		return -EINVAL;

	} else if (buffer->len < plane->bytes_used) {
		dev_err(astc_device->dev,
			"%s: driver expects %zu bytes but user gave %zu\n",
			__func__, plane->bytes_used,
			buffer->len);
		return -EINVAL;
	}

	if ((buffer->type != HWASTC_BUFFER_USERPTR) &&
			(buffer->type != HWASTC_BUFFER_DMABUF)) {
		dev_err(astc_device->dev, "%s: unknown buffer type %u\n",
			__func__, buffer->type);
		return -EINVAL;
	}

	ret = astc_buffer_get_and_attach(astc_device, buffer, dma_buffer);

	if (ret)
		return ret;

	dma_buffer->buffer = buffer;

	dev_dbg(astc_device->dev
		, "%s: about to prepare buffer %pK, dir DMA_TO_DEVICE? %d\n"
		, __func__, dma_buffer, dir == DMA_TO_DEVICE);

	/* the callback function should fill 'dma_addr' field */
	ret = astc_buffer_map(ctx, task, dma_buffer, dir == DMA_TO_DEVICE);
	if (ret) {
		dev_err(astc_device->dev, "%s: Failed to prepare plane\n"
			, __func__);

		dev_dbg(astc_device->dev, "%s: releasing buffer %pK\n"
			, __func__, dma_buffer);

		astc_buffer_put_and_detach(dma_buffer);

		return ret;
	}

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * astc_validate_buf_addresses() - Validate the memory regions of the buffers
 * @ctx: the context of the task that is being setup
 * @task: the task that is being processed
 *
 * Check that the user memory pointers or DMA file descriptors
 * representing the buffers received from userspace do not point to the
 * same memory.
 *
 * When both buffers are representd by ptrs to user memory, check that the
 * memory regions identifying the buffers don't overlap, and then check
 * that the memory they point to is in userspace and not in kernel space.
 *
 * When both buffers are represented by DMA file descriptors, check that
 * they're not the same fd.
 * Note: the check on DMA buffers is less restrictive.
 * TODO: improve that.
 *
 * TODO: also add checks for the case where one buffer is a ptr to user
 * memory and the other is a DMA buffer.
 *
 * Return:
 *	 0 when the check is successful,
 *	<0 when the check fails.
 */
static int astc_validate_buf_addresses(struct astc_ctx *ctx,
				       struct astc_task *task) {
	struct device *dev = NULL;
	struct hwASTC_buffer *buf_out = NULL;
	struct hwASTC_buffer *buf_cap = NULL;

	if (!ctx) {
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}
	dev = ctx->astc_dev->dev;

	if (!task) {
		dev_err(dev, "%s: invalid task!\n", __func__);
		return -EINVAL;
	}

	dev_dbg(dev, "%s: validating buffer addresses...\n", __func__);

	buf_out = &task->user_task.buf_out;
	buf_cap = &task->user_task.buf_cap;

	if (buf_out->type == HWASTC_BUFFER_NONE
			|| buf_cap->type == HWASTC_BUFFER_NONE) {
		dev_err(dev, "%s: invalid buffer type (out: %u, cap: %u)\n",
			__func__, buf_out->type, buf_cap->type);
		return -EINVAL;
	}

	if (buf_out->len == 0) {
		dev_err(dev, "%s: cannot process a null size out buffer\n",
			__func__);
		return -EINVAL;
	}
	if (buf_cap->len == 0) {
		dev_err(dev, "%s: cannot process a null size capture buffer\n",
			__func__);
		return -EINVAL;
	}

	if (buf_out->type == HWASTC_BUFFER_USERPTR
		   && buf_cap->type == HWASTC_BUFFER_USERPTR) {

		//pointers to the last cells of out and cap buffers
		unsigned long buf_out_end = buf_out->userptr + buf_out->len - 1;
		unsigned long buf_cap_end = buf_cap->userptr + buf_cap->len - 1;

		if (buf_out->userptr == buf_cap->userptr) {
			dev_err(dev, "%s: cannot use same ptr for both input and output buffers\n", __func__);
			return -EINVAL;
		}

		/*
		 * check the addition didn't wrap around. Data is coming from
		 * userspace, we can't be sure what it contains
		 * The check on len is a bit paranoid, we already check it above.
		 */
		if (buf_out->len != 0 && buf_out_end < buf_out->userptr) {
			dev_err(dev, "%s: overflow in buf out end\n",
				__func__);
			return -EINVAL;
		}
		if (buf_cap->len != 0 && buf_cap_end < buf_cap->userptr) {
			dev_err(dev, "%s: overflow in buf cap end\n",
				__func__);
			return -EINVAL;
		}

		/*
		 * Check that there is no overlap in the memory regions of the
		 * two buffers.
		 * Given [xStart, xEnd] [yStart, yEnd] ranges with xStart<=xEnd, yStart<=yEnd
		 * and xStart != yStart (we already checked it above),
		 * there is NO overlap if (xStart > yEnd || yStart > xEnd), so
		 * there IS overlap when (xStart <= yEnd && yStart <= xEnd)
		 */
		if (buf_out->userptr <= buf_cap_end
				&& buf_cap->userptr <= buf_out_end) {
			dev_err(dev, "%s: in/out buffers overlap detected (out: %lu/%zu cap: %lu/%zu)\n"
				, __func__
				, buf_out->userptr, buf_out->len
				, buf_cap->userptr, buf_cap->len);
			return -EINVAL;
		}

	} else if (buf_out->type == HWASTC_BUFFER_DMABUF
		   && buf_cap->type == HWASTC_BUFFER_DMABUF) {
		// in this case we can only check if they're using the same
		// DMA fd
		if (buf_out->fd == buf_cap->fd) {
			dev_err(dev, "%s: cannot use same DMA fd for both input and output buffers\n"
				, __func__);
			return -EINVAL;
		}
	} else {
		// TODO: can we do anything to check the other cases, such
		//       as when one is using DMA fd and the other user ptr?
	}

	//now ensure the buffers are in userspace and not kernel space
	if (buf_out->type == HWASTC_BUFFER_USERPTR &&
			!access_ok(VERIFY_READ,
				      (void __user *) buf_out->userptr,
				      buf_out->len)) {
		dev_err(dev, "%s: no access to src buffer\n", __func__);
		return -EFAULT;
	}
	if (buf_cap->type == HWASTC_BUFFER_USERPTR &&
			!access_ok(VERIFY_WRITE,
				      (void __user *) buf_cap->userptr,
				      buf_cap->len)) {
		dev_err(dev, "%s: no access to result buffer\n", __func__);
		return -EFAULT;
	}

	dev_dbg(dev, "%s: done.\n", __func__);

	return 0;
}

/**
 * astc_validate_config() - Validate the encoding configuration
 * @ctx: the context of the task that is being setup
 * @cfg: the encoding configuration to be checked
 *
 * Return:
 *	-1 if the input configuration is invalid or not supported by the H/W,
 *	0 otherwise.
 */
static int astc_validate_config(struct astc_ctx *ctx, struct hwASTC_config* cfg)
{
	struct device *dev = NULL;
	if (!ctx) {
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}
	dev = ctx->astc_dev->dev;

	dev_dbg(dev, "%s: validating configuration...\n", __func__);

	switch(cfg->encodeBlockSize) {
	case ASTC_ENCODE_MODE_BLK_SIZE_4x4:
	case ASTC_ENCODE_MODE_BLK_SIZE_6x6:
	case ASTC_ENCODE_MODE_BLK_SIZE_8x8:
		break;
	default:
		dev_err(dev, "%s: invalid encode block size cfg %u\n"
			, __func__
			, cfg->encodeBlockSize);
		return -1;

	}

	switch(cfg->intref_iterations) {
	case ASTC_ENCODE_MODE_REF_ITER_ZERO:
	case ASTC_ENCODE_MODE_REF_ITER_ONE:
	case ASTC_ENCODE_MODE_REF_ITER_TWO:
	case ASTC_ENCODE_MODE_REF_ITER_FOUR:
		break;
	default:
		dev_err(dev, "%s: invalid refine iterations cfg %u\n"
			, __func__
			, cfg->intref_iterations);
		return -1;
	}

	switch(cfg->partitions) {
	case ASTC_ENCODE_MODE_PARTITION_ONE:
	case ASTC_ENCODE_MODE_PARTITION_TWO:
		break;
	default:
		dev_err(dev, "%s: invalid partitions cfg %u\n"
			, __func__
			, cfg->partitions);
		return -1;
	}

	switch(cfg->num_blk_mode) {
	case ASTC_ENCODE_MODE_NO_BLOCK_MODE_16EA:
	case ASTC_ENCODE_MODE_NO_BLOCK_MODE_FULL:
		break;
	default:
		dev_err(dev, "%s: invalid block mode cfg %u\n"
			, __func__
			, cfg->num_blk_mode);
		return -1;
	}

	switch(cfg->dual_plane_enable) {
	case 1:
	case 0:
		break;
	default:
		dev_err(dev, "%s: invalid dual plane cfg %hu\n"
			, __func__
			, cfg->dual_plane_enable);
		return -1;
	}

	return 0;
}

/**
 * astc_validate_format() - Validate the input image size and pixel format.
 * @ctx: the context of the task that is being processed
 * @task: the task that is being processed
 * @dir: the direction of the DMA transfer
 * @srcBitsPerPixel: pointer where the function will store the bpp of the format
 *
 * Check validity of source/dest formats, i.e. image size and pixel format.
 *
 * dma_data_direction is used to distinguish between the source/dest
 * formats.
 *
 * When the DMA direction is DMA_TO_DEVICE, check that the source pixel
 * format is something the H/W can handle, and save the number of
 * bits per pixel needed for that format in the variable referenced by the
 * @srcBitsPerPixel.
 *
 * Return:
 *	 0 when the validation is successful,
 *	<0 on error.
 */
static int astc_validate_format(struct astc_ctx *ctx,
				struct astc_task *task,
				enum dma_data_direction dir,
				int *srcBitsPerPixel)
{
	struct hwASTC_img_info *img_info = NULL;

	if (!ctx) {
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}
	if (!task) {
		dev_err(ctx->astc_dev->dev
			, "%s: invalid task ptr!\n", __func__);
		return -EINVAL;
	}

	//handling the buffer holding the encoding result
	if (dir == DMA_FROM_DEVICE) {
		img_info = &task->user_task.info_cap;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_FROM_DEVICE case, width %u height %u\n"
			, __func__, img_info->width, img_info->height);

		/*
		 * no need to check the pixel format, we know the result
		 * will be ASTC
		 */

	} else if (dir == DMA_TO_DEVICE) {
		img_info = &task->user_task.info_out;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_TO_DEVICE case, width %u height %u\n"
			, __func__, img_info->width, img_info->height);

		if (!srcBitsPerPixel) {
			dev_err(ctx->astc_dev->dev
				, "%s: invalid ptr to bits per pixel var!\n"
				, __func__);
			return -EINVAL;
		}

		//Check validity of the source pixel format
		*srcBitsPerPixel = ftm_to_bpp(ctx->astc_dev->dev, img_info);
		if (*srcBitsPerPixel <= 0) {
			dev_err(ctx->astc_dev->dev, "%s: invalid pix format\n",
				__func__);
			return -EINVAL;
		}

	} else {
		//this shouldn't happen
		dev_err(ctx->astc_dev->dev, "%s: invalid DMA direction\n",
			__func__);
		return -EINVAL;
	}

	if (img_info->width < ASTC_INPUT_IMAGE_MIN_SIZE ||
			img_info->height < ASTC_INPUT_IMAGE_MIN_SIZE ||
			img_info->width > ASTC_INPUT_IMAGE_MAX_SIZE ||
			img_info->height > ASTC_INPUT_IMAGE_MAX_SIZE) {
		dev_err(ctx->astc_dev->dev
			, "%s: invalid size of width or height (%u, %u)\n",
			__func__, img_info->width, img_info->height);
		return -EINVAL;
	}

	return 0;
}


/**
 * compute_num_blocks() - Compute the number of ASTC blocks the H/W will output
 * @device: the platform device
 * @blk_size: block size used for encoding
 * @src_width: the width of the image we want to process
 * @src_height: the height of the image we want to process
 * @blocks_on_x: pointer to the var where the result will be stored
 * @blocks_on_y: pointer to the var where the result will be stored
 *
 * Given an image of size @src_width and @src_height, compute
 * the number of ASTC blocks (on the X/Y axes) required to encode that
 * image using block size @blk_size, rounded up to the next integer (if
 * fractional).
 *
 * The results are stored in @blocks_on_x and @blocks_on_y.
 *
 * Return:
 *	0 on success,
 *	-ERANGE on overflow/wraparound,
 *	-EINVAL otherwise.
 */
static int compute_num_blocks(struct astc_dev *device,
			      enum hwASTC_blk_size blk_size,
			      u32 src_width, u32 src_height,
			      u32 *blocks_on_x, u32 *blocks_on_y)
{
	u32 x_blk_size = 0;
	u32 y_blk_size = 0;

	if (!blocks_on_x || !blocks_on_y) {
		dev_err(device->dev, "%s: invalid parameters\n", __func__);
		return -EINVAL;
	}

	if (src_width == 0 || src_height == 0) {
		dev_err(device->dev, "%s: null image size\n", __func__);
		return -EINVAL;
	}

	/* we assume the check for image min/max size has already been done */

	x_blk_size = blksize_enum_to_x(blk_size);
	y_blk_size = blksize_enum_to_y(blk_size);

	if (x_blk_size == 0 || y_blk_size == 0) {
		dev_err(device->dev
			, "%s: unsupported block size enum value %u\n"
			, __func__, blk_size);
		return -EINVAL;
	}

	/*
	 * perform overflow (wraparound) checks before doing the sum below
	 * the division between unsigned does not need checks
	 */
	if ((UINT_MAX - src_width) < (x_blk_size - 1)) {
		dev_err(device->dev
			, "%s: wraparound while computing num of x blocks %u %u\n"
			, __func__, src_width, x_blk_size);
		return -ERANGE;
	}
	if ((UINT_MAX - src_height) < (y_blk_size - 1)) {
		dev_err(device->dev
			, "%s: wraparound while computing num of y blocks %u %u\n"
			, __func__, src_height, y_blk_size);
		return -ERANGE;
	}
	/* number of blocks on x and y sides, rounded up not to crop the img */
	*blocks_on_x = (src_width  + x_blk_size - 1) / x_blk_size;
	*blocks_on_y = (src_height + y_blk_size - 1) / y_blk_size;

	dev_dbg(device->dev
		, "%s: REQUESTING %u x %u blks of sz %u x %u, img size %d x %d"
		, __func__, *blocks_on_x, *blocks_on_y
		, x_blk_size, y_blk_size
		, src_width, src_height);

	return 0;
}

/**
 * astc_compute_buffer_size()
 * @ctx: the context of the task that is being processed
 * @task: the task that is being processed
 * @dir: the direction of the DMA transfer
 * @bytes_used: a pointer to the variable where the result will be stored.
 * @srcBitsPerPixel: the bpp of the input image
 *
 * Compute the size that the in/out buffer(s) must have to be able to hold
 * the in/out data, given the image format/size provided in the input task.
 * The result is saved in the variable pointed to by bytes_used.
 *
 * The dma_data_direction is used to distinguish between output and capture
 * (in/out) buffers.
 *
 * srcBitsPerPixel is only used to compute the size of the buffer that
 * holds the source image we were asked to process.
 *
 * Return:
 *	 0 on success,
 *	<0 error num otherwise
 */
static int astc_compute_buffer_size(struct astc_ctx *ctx,
				    struct astc_task *task,
				    enum dma_data_direction dir,
				    size_t *bytes_used,
				    int srcBitsPerPixel)
{
	if (!ctx) {
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}
	dev_dbg(ctx->astc_dev->dev, "%s: BEGIN\n", __func__);

	if (!task) {
		dev_err(ctx->astc_dev->dev
			, "%s: invalid task ptr!\n", __func__);
		return -EINVAL;
	}
	if (!bytes_used) {
		dev_err(ctx->astc_dev->dev
			, "%s: invalid ptr to requested buffer size!\n"
			, __func__);
		return -EINVAL;
	}

	/* handling the buffer holding the encoding result */
	if (dir == DMA_FROM_DEVICE) {
		size_t bytes_needed = 0;
		size_t tmp_num_blocks = 0;
		size_t tmp_bytes_without_header = 0;
		const struct hwASTC_img_info *img_info = &task->user_task.info_cap;
		const typeof(img_info->width) img_width  = img_info->width;
		const typeof(img_info->height) img_height = img_info->height;

		int res = compute_num_blocks(ctx->astc_dev
					     , task->user_task.enc_config.encodeBlockSize
					     , img_width
					     , img_height
					     , &ctx->blocks_on_x
					     , &ctx->blocks_on_y);

		if (res) {
			dev_err(ctx->astc_dev->dev,
				"%s: failed to compute num of blocks needed\n"
				, __func__);
			return -EINVAL;
		}

		/*
		 * For the output we need 16 bytes per block, + the astc header
		 * NOTE: hw currently only supports 2d astc encoding,
		 *       otherwise we'd need to take blocks on Z axis into
		 *       account as well
		 *
		 * Also perform wraparound checks before storing the value
		 */
		if ((size_t) ctx->blocks_on_x > (SIZE_MAX / ctx->blocks_on_y)) {
			dev_err(ctx->astc_dev->dev,
				"%s: computation of num of blocks needed (mul) wrapped around\n"
				, __func__);
			return -ERANGE;
		}
		tmp_num_blocks = (size_t) ctx->blocks_on_x * ctx->blocks_on_y;
		if (tmp_num_blocks > (SIZE_MAX / ASTC_BYTES_PER_BLOCK)) {
			dev_err(ctx->astc_dev->dev,
				"%s: computation of num of bytes needed (mul) wrapped around\n"
				, __func__);
			return -ERANGE;
		}
		tmp_bytes_without_header = tmp_num_blocks * ASTC_BYTES_PER_BLOCK;
		bytes_needed = tmp_bytes_without_header + ASTC_HEADER_BYTES;
		/* check that the addition did not wrap around */
		if (bytes_needed < tmp_bytes_without_header) {
			dev_err(ctx->astc_dev->dev,
				"%s: computation of num of blocks needed (add) wrapped around\n"
				, __func__);
			return -ERANGE;
		}

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_FROM_DEVICE, width %u height %u\n"
			, __func__, img_width, img_height);

		if (bytes_needed <= 0) {
			dev_err(ctx->astc_dev->dev,
				"%s: invalid output buffer size\n",
				__func__);
			return -EINVAL;
		}
		*bytes_used = bytes_needed;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_FROM_DEVICE, requesting %zu bytes\n"
			, __func__, *bytes_used);

	} else if (dir == DMA_TO_DEVICE) {
		struct hwASTC_img_info *img_info = &task->user_task.info_out;
		typeof(img_info->width) new_width  = img_info->width;
		typeof(img_info->height) new_height = img_info->height;
		size_t num_pixels = 0;

		/* for the Exynos9810 H/W bug workaround */
		int ret = 0;
		size_t buffer_extension_size = 0;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_TO_DEVICE case, width %u height %u\n"
			, __func__, new_width, new_height);

		ret = astc_exynos9810_setup_workaround(ctx->astc_dev->dev,
						       task,
						       &new_width,
						       &new_height,
						       &buffer_extension_size);
		if (ret < 0) {
			dev_err(ctx->astc_dev->dev,
				"%s: failed to compute extended buffer size (%d)\n",
				__func__, ret);
			return -EINVAL;
		}

		/* Compute number of bytes we expect from userspace, with
		 * overflow/wraparound checks
		 * NOTE: the casts to size_t area to avoid wraparounds and to
		 *       adjust the result to bytes_used, which is a size_t.
		 * TODO: move to GCC overflow intrinsics once we move to GCC5+
		 */
		if (((size_t)new_width) > (SIZE_MAX / new_height)) {
			dev_err(ctx->astc_dev->dev
				, "%s: new img size mul wrapped around %u %u\n"
				, __func__, new_width, new_height);
			return -ERANGE;
		}
		num_pixels = (size_t)new_width * new_height;
		if (num_pixels > (SIZE_MAX / srcBitsPerPixel)) {
			dev_err(ctx->astc_dev->dev
				, "%s: new img bytes mul wrapped around %zu %u\n"
				, __func__, num_pixels, srcBitsPerPixel);
			return -ERANGE;
		}
		*bytes_used = num_pixels * srcBitsPerPixel;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_TO_DEVICE, requesting %zu bytes\n"
			, __func__, *bytes_used);

	} else {
		/* this should never happen */
		dev_err(ctx->astc_dev->dev,
			"%s: invalid DMA direction\n",
			__func__);
		return -EINVAL;
	}

	dev_dbg(ctx->astc_dev->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * astc_process_formats() - Inspect and validate the image formats
 * @astc_device: the astc device struct
 * @ctx: the context of the task that is being processed
 * @task: the task that is being processed
 *
 * Validate the image formats (size, pixel formats) provided by userspace
 * and compute the size of the DMA buffers required to be able to process
 * the task.
 *
 * Basically a wrapper around astc_compute_buffer_size() and
 * astc_validate_format().
 *
 * Return:
 *	 0 on success,
 *	<0 error num otherwise
 */
static int astc_process_formats(struct astc_dev *astc_device,
				struct astc_ctx *ctx,
				struct astc_task *task)
{
	int ret = 0;
	size_t out_size = 0;
	size_t cap_size = 0;
	int srcBitsPerPixel = 0;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	dev_dbg(ctx->astc_dev->dev, "%s: Validating format...\n", __func__);

	ret = astc_validate_format(ctx, task, DMA_TO_DEVICE, &srcBitsPerPixel);
	if (ret < 0)
		return ret;

	ret = astc_compute_buffer_size(ctx, task, DMA_TO_DEVICE, &out_size
				       , srcBitsPerPixel);
	if (ret < 0)
		return ret;

	dev_dbg(astc_device->dev, "%s: input buffer, plane requires %zu bytes\n"
		, __func__,  out_size);

	task->dma_buf_out.plane.bytes_used = out_size;

	dev_dbg(astc_device->dev
		, "%s: hardcoding result img height/width to be same as input\n"
		, __func__);

	//set the destination img width/height even though it's the same
	//as the input img size, as the other procedures need it to compute
	//the blocks-per-side and similar
	task->user_task.info_cap.width  = task->user_task.info_out.width;
	task->user_task.info_cap.height = task->user_task.info_out.height;

	ret = astc_validate_format(ctx, task, DMA_FROM_DEVICE, NULL);
	if (ret < 0)
		return ret;

	ret = astc_compute_buffer_size(ctx, task, DMA_FROM_DEVICE, &cap_size
				       , srcBitsPerPixel);
	if (ret < 0)
		return ret;

	dev_dbg(astc_device->dev, "%s: dest buffer plane requires %zu bytes\n"
		, __func__, cap_size);

	task->dma_buf_cap.plane.bytes_used = cap_size;

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * astc_task_teardown() - Main entry point for task teardown.
 * @astc_dev: the astc device struct
 * @ctx: the context of the task that is being torn down
 * @task: the task that is being torn down
 *
 * Call auxiliary procedures to dismantle the buffers used for the
 * processing of @task.
 */
static void astc_task_teardown(struct astc_dev *astc_dev,
			     struct astc_ctx *ctx,
			     struct astc_task *task)
{
	dev_dbg(astc_dev->dev, "%s: BEGIN\n", __func__);

	astc_buffer_teardown(astc_dev, ctx, &task->dma_buf_cap,
			     DMA_FROM_DEVICE);

	astc_buffer_teardown(astc_dev, ctx, &task->dma_buf_out,
			     DMA_TO_DEVICE);

	dev_dbg(astc_dev->dev, "%s: END\n", __func__);
}

/**
 * astc_task_setup() - Main procedure that handles the full setup of a task
 * @astc_device: the astc device struct
 * @ctx: the context of the task that is being setup
 * @task: the task that is being setup
 *
 * Main entry point for preparing a task before it can be processed by
 * the H/W.
 *
 * Check the validity of requested formats/sizes and do the setup of the
 * buffers.
 *
 * Basically a wrapper around astc_process_formats() and astc_buffer_setup().
 *
 * Return:
 *	0 on success,
 *	an error code otherwise.
 */
static int astc_task_setup(struct astc_dev *astc_device,
			     struct astc_ctx *ctx,
			     struct astc_task *task)
{
	int ret;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	HWASTC_PROFILE(ret = astc_process_formats(astc_device, ctx, task),
		       "PREPARE FORMATS TIME", astc_device->dev);
	if (ret)
		return ret;

	dev_dbg(astc_device->dev, "%s: OUT buf, USER GAVE %zu WE REQUIRE %zu\n"
		, __func__,  task->user_task.buf_out.len
		, task->dma_buf_out.plane.bytes_used);

	HWASTC_PROFILE(ret = astc_buffer_setup(ctx, task, &task->user_task.buf_out,
				      &task->dma_buf_out, DMA_TO_DEVICE),
		       "OUT BUFFER SETUP TIME", astc_device->dev);
	if (ret) {
		dev_err(astc_device->dev, "%s: Failed to get output buffer\n",
			__func__);
		return ret;
	}

	dev_dbg(astc_device->dev, "%s: CAP buf, USER GAVE %zu WE REQUIRE %zu\n"
		, __func__,  task->user_task.buf_cap.len
		, task->dma_buf_cap.plane.bytes_used);

	HWASTC_PROFILE(ret = astc_buffer_setup(ctx, task, &task->user_task.buf_cap,
				  &task->dma_buf_cap, DMA_FROM_DEVICE),
		       "CAP BUFFER SETUP TIME", astc_device->dev);
	if (ret) {
		astc_buffer_teardown(astc_device, ctx,
				     &task->dma_buf_out, DMA_TO_DEVICE);
		dev_err(astc_device->dev, "%s: Failed to get capture buffer\n",
			__func__);
		return ret;
	}

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * astc_destroy_context() - Remove a context from the contexts list and free it.
 * @kreference: the kref object for the context
 */
static void astc_destroy_context(struct kref *kreference)
{
	/*
	 * this function is called with ctx->kref as parameter,
	 * so we can use ptr arithmetics to go back to ctx
	 */
	struct astc_ctx *ctx = container_of(kreference,
					    struct astc_ctx, kref);
	struct astc_dev *astc_device = ctx->astc_dev;
	unsigned long flags;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	spin_lock_irqsave(&astc_device->lock_ctx, flags);
	dev_dbg(astc_device->dev, "%s: deleting context %pK from list\n"
		, __func__, ctx);
	list_del(&ctx->node);
	spin_unlock_irqrestore(&astc_device->lock_ctx, flags);

	kfree(ctx);

	dev_dbg(astc_device->dev, "%s: END\n", __func__);
}

/**
 * astc_process() - Main entry point for task processing
 * @ctx: context of the task that is about to be processed
 * @task: the task to process
 *
 * Handle all steps needed to process a task, from setup to teardown.
 *
 * First perform the necessary validations and setup of buffers, then
 * wake the H/W up, add the task to the queue, schedule it for execution
 * and wait for the H/W to signal completion.
 *
 * After the @task has been processed (independently of whether it was
 * successful or not), release all the resources and power the H/W down.
 *
 * Return:
 *	 0 on success,
 *	<0 error code on error.
 */
static int astc_process(struct astc_ctx *ctx,
			struct astc_task *task)
{
	struct astc_dev *astc_device = NULL;
	int ret = 0;
	int num_retries = -1;

	if (!ctx) {
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}

	astc_device = ctx->astc_dev;
	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	if (astc_validate_config(ctx, &task->user_task.enc_config) < 0) {
		dev_err(astc_device->dev,
			"%s: invalid encoding configuration!\n", __func__);
		return -EINVAL;
	}

	if (astc_validate_buf_addresses(ctx, task) < 0) {
		dev_err(astc_device->dev,
			"%s: invalid buffer addresses!\n", __func__);
		return -EINVAL;
	}

	init_completion(&task->complete);

	dev_dbg(astc_device->dev, "%s: acquiring kref of ctx %pK\n",
		__func__, ctx);

	/*
	 * no need to protect this with a mutex, because the ptr it is the
	 * refcount of (i.e. ctx) is not released until release() is called,
	 * which cannot happen while an ioctl is in progress
	 */
	kref_get(&ctx->kref);

	mutex_lock(&ctx->mutex);

	dev_dbg(astc_device->dev, "%s: inside context lock (ctx %pK task %pK)\n"
		, __func__, ctx, task);

	/*
	 * NOTE: this CANNOT be moved inside the workqueue worker, because that
	 * is executed in a kernel thread, which by definition does not have
	 * access to process memory so you can't do mapping of USERPTR
	 * buffers there, at the very least (i.e. exynos_iovmm_map_userptr,
	 * find_vma and other calls that require access to current->mm).
	 * It might be possible for DMABUF buffers.
	 */
	ret = astc_task_setup(astc_device, ctx, task);

	if (ret)
		goto err_prepare_task;

	task->ctx = ctx;
	task->state = ASTC_BUFSTATE_READY;

	INIT_WORK(&task->work, astc_task_schedule_work);

	do {
		num_retries++;

		if (!queue_work(astc_device->workqueue, &task->work)) {
			dev_err(astc_device->dev
				, "%s: work was already on a workqueue!\n"
				, __func__);
		}

		/* wait for the job to be completed, in UNINTERRUPTIBLE mode */
		flush_work(&task->work);

	} while (num_retries < MAX_NUM_RETRIES
		 && task->state == ASTC_BUFSTATE_RETRY);

	if (task->state == ASTC_BUFSTATE_READY) {
		dev_err(astc_device->dev
			, "%s: invalid task state after task completion\n"
			, __func__);
	}

	HWASTC_PROFILE(astc_task_teardown(astc_device, ctx, task),
		       "TASK TEARDOWN TIME",
		       astc_device->dev);
err_prepare_task:
	HWASTC_PROFILE(

	dev_dbg(astc_device->dev, "%s: releasing lock of ctx %pK\n"
		, __func__, ctx);
	mutex_unlock(&ctx->mutex);

	dev_dbg(astc_device->dev, "%s: releasing kref of ctx %pK\n"
		, __func__, ctx);

	kref_put(&ctx->kref, astc_destroy_context);

	dev_dbg(astc_device->dev, "%s: returning %d, task state %d\n"
		, __func__, ret, task->state);

	, "FINISH PROCESS", astc_device->dev);

	if (ret)
		return ret;
	else
		return (task->state == ASTC_BUFSTATE_DONE) ? 0 : -EINVAL;
}

/**
 * astc_open() - Handle device file open calls
 * @inode: the inode struct representing the device file on disk
 * @filp: the kernel file struct representing the abstract device file
 *
 * Function passed as "open" member of the file_operations struct of
 * the device in order to handle dev node open calls.
 *
 * Create a structure to hold common data related to tasks which are
 * processed through this device fd (i.e. the "context") and initialize
 * context-related resources.
 * A new context is created each time the device is opened.
 *
 * As a consequence, if a userspace client uses more threads to
 * enqueue separate encoding tasks using the SAME device fd, those
 * tasks will be sharing the same context (because threads in the same
 * process share the fd table).
 *
 * Return:
 *	0 on success,
 *	-ENOMEM if there is not enough memory to allocate the context
 */
static int astc_open(struct inode *inode, struct file *filp)
{
	unsigned long flags;
	/*
	 * filp->private_data holds astc_dev->misc (misc_open sets it)
	 * this uses pointers arithmetics to go back to astc_dev
	 */
	struct astc_dev *astc_device = container_of(filp->private_data,
						    struct astc_dev, misc);
	struct astc_ctx *ctx;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);
	// GFP_KERNEL is used when kzalloc is allowed to sleep while
	// the memory we're requesting is not immediately available
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	INIT_LIST_HEAD(&ctx->node);
	kref_init(&ctx->kref);
	mutex_init(&ctx->mutex);

	ctx->astc_dev = astc_device;

	spin_lock_irqsave(&astc_device->lock_ctx, flags);
	dev_dbg(astc_device->dev, "%s: adding new context %pK to contexts list\n"
		, __func__, ctx);
	list_add_tail(&ctx->node, &astc_device->contexts);
	spin_unlock_irqrestore(&astc_device->lock_ctx, flags);

	filp->private_data = ctx;

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * astc_release() - Handle device file close calls
 * @inode: the inode struct representing the device file on disk
 * @filp: the kernel file struct representing the abstract device file
 *
 * Function passed as "close" member of the file_operations struct of
 * the device in order to handle dev node close calls.
 *
 * Decrease the refcount of the context associated to the given
 * device node.
 */
static int astc_release(struct inode *inode, struct file *filp)
{
	struct astc_ctx *ctx = filp->private_data;

	dev_dbg(ctx->astc_dev->dev, "%s: releasing context %pK\n"
		, __func__, ctx);

	/*
	 * the release callback will not be invoked if counter
	 * is already 0
	 */
	kref_put(&ctx->kref, astc_destroy_context);

	return 0;
}

/**
 * astc_ioctl() - Handle ioctl calls coming from userspace.
 * @filp: the kernel file struct representing the abstract device file
 * @cmd: the ioctl command
 * @arg: the argument passed to the ioctl sys call
 *
 * Receive the task information from userspace, setup and execute the task.
 * Wait for the task to be completed and then return control to userspace.
 *
 * Return:
 *	 0 on success,
 *	<0 error codes otherwise
 */
static long astc_ioctl(struct file *filp,
		       unsigned int cmd, unsigned long arg)
{
	struct astc_ctx *ctx = filp->private_data;
	struct astc_dev *astc_device = ctx->astc_dev;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	switch (cmd) {
	case HWASTC_IOC_PROCESS:
	{
		struct astc_task data;
		int ret;

		dev_dbg(astc_device->dev, "%s: PROCESS ioctl received\n"
			, __func__);

		memset(&data, 0, sizeof(data));

		if (copy_from_user(&data.user_task
				   , (void __user *)arg
				   , sizeof(struct hwASTC_task))) {
			dev_err(astc_device->dev,
				"%s: Failed to read userdata\n", __func__);
			return -EFAULT;
		}

		dev_dbg(astc_device->dev
			, "%s: user data copied, now launching processing...\n"
			, __func__);

		/*
		 * astc_process() does not wake up
		 * until the given task finishes
		 */
		HWASTC_PROFILE(ret = astc_process(ctx, &data),
			       "WHOLE PROCESS TIME",
			       astc_device->dev);

		dev_dbg(astc_device->dev
			, "%s: processing done! Copying data back to user space...\n", __func__);

		if (ret) {
			dev_err(astc_device->dev,
				"%s: Failed to process task, error %d\n"
				, __func__, ret);
			return ret;
		}

		/* No need to copy the task structure back to userspace,
		 * as we did not modify it.
		 * The encoded data is at the address we received from
		 * userspace.
		 */

		return ret;
	}
	default:
		dev_err(astc_device->dev, "%s: Unknown ioctl cmd %x\n",
			__func__, cmd);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_COMPAT

struct compat_astc_img_info {
	compat_uint_t fmt;
	compat_uint_t width;
	compat_uint_t height;
};

struct compat_astc_buffer {
	union {
		compat_int_t fd;
		compat_ulong_t userptr;
	};
	compat_size_t len;
	__u8 type;
};

struct compat_astc {
	struct compat_astc_img_info info_out;
	struct compat_astc_img_info info_cap;
	struct compat_astc_buffer buf_out;
	struct compat_astc_buffer buf_cap;
	//layout of hwASTC_config is crafted to be the same on 32/64bits,
	//so no need for a separate compat struct
	struct hwASTC_config enc_config;
	compat_ulong_t reserved[2];
};

/*
 * hwASTC_config struct only uses elements with fixed width and it
 * has explicit padding, so no compat structure should be needed
 */

#define COMPAT_HWASTC_IOC_PROCESS	_IOWR('M',   0, struct compat_astc)

static long astc_compat_ioctl32(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	struct astc_ctx *ctx = filp->private_data;
	struct astc_dev *astc_device = ctx->astc_dev;

	switch (cmd) {
	case COMPAT_HWASTC_IOC_PROCESS:
	{
		struct compat_astc data;
		struct astc_task task;
		int ret;

		memset(&task, 0, sizeof(task));

		if (copy_from_user(&data, compat_ptr(arg), sizeof(struct compat_astc))) {
			dev_err(astc_device->dev,
				"%s: Failed to read userdata\n", __func__);
			return -EFAULT;
		}

		task.user_task.info_out.fmt = data.info_out.fmt;
		task.user_task.info_out.width = data.info_out.width;
		task.user_task.info_out.height = data.info_out.height;
		task.user_task.info_cap.fmt = data.info_cap.fmt;
		task.user_task.info_cap.width = data.info_cap.width;
		task.user_task.info_cap.height = data.info_cap.height;

		task.user_task.buf_out.len = data.buf_out.len;
		if (data.buf_out.type == HWASTC_BUFFER_DMABUF)
			task.user_task.buf_out.fd = data.buf_out.fd;
		else /* data.buf_out.type == HWASTC_BUFFER_USERPTR */
			task.user_task.buf_out.userptr = data.buf_out.userptr;
		task.user_task.buf_out.type = data.buf_out.type;

		task.user_task.buf_cap.len = data.buf_cap.len;
		if (data.buf_cap.type == HWASTC_BUFFER_DMABUF)
			task.user_task.buf_cap.fd = data.buf_cap.fd;
		else /* data.buf_cap.type == HWASTC_BUFFER_USERPTR */
			task.user_task.buf_cap.userptr = data.buf_cap.userptr;
		task.user_task.buf_cap.type = data.buf_cap.type;

		task.user_task.enc_config = data.enc_config;

		task.user_task.reserved[0] = data.reserved[0];
		task.user_task.reserved[1] = data.reserved[1];

		/*
		 * astc_process() does not wake up
		 * until the given task finishes
		 */
		ret = astc_process(ctx, &task);
		if (ret) {
			dev_err(astc_device->dev,
				"%s: Failed to process hwASTC_task task\n",
				__func__);
			return ret;
		}

		/*
		 * No need to copy the task structure back to userspace,
		 * as we did not modify it.
		 * The encoded data is at the address we received from
		 * userspace.
		 */

		return 0;
	}
	default:
		dev_err(astc_device->dev, "%s: Unknown compat_ioctl cmd %x\n",
			__func__, cmd);
		return -EINVAL;
	}

	return 0;
}
#endif

static const struct file_operations astc_fops = {
	.owner          = THIS_MODULE,
	.open           = astc_open,
	.release        = astc_release,
	.unlocked_ioctl	= astc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= astc_compat_ioctl32,
#endif
};

/* The handler of IOVMM page faults */
static int astc_sysmmu_fault_handler(struct iommu_domain *domain,
				     struct device *dev,
				     unsigned long fault_addr,
				     int fault_flags, void *p)
{
	dev_dbg(dev, "%s: sysmmu fault!\n", __func__);

	return 0;
}

/*
 * Acquire and "prepare" the clock producer resource.
 * This is the setup needed before enabling the clock source.
 *
 * Only called once in the driver initialization routine, probe.
 */
static int astc_clk_devm_get_prepare(struct astc_dev *astc)
{
	struct device *dev = astc->dev;
	int ret = 0;

	dev_dbg(astc->dev, "%s: acquiring clock with devm\n", __func__);

	astc->clk_producer = devm_clk_get(dev, clk_producer_name);

	if (IS_ERR(astc->clk_producer)) {
		dev_err(dev, "%s clock is not present\n",
			clk_producer_name);
		return PTR_ERR(astc->clk_producer);
	}

	dev_dbg(astc->dev, "%s: preparing the clock\n", __func__);

	if (!IS_ERR(astc->clk_producer)) {
		ret = clk_prepare(astc->clk_producer);
		if (ret) {
			dev_err(astc->dev, "%s: clk preparation failed (%d)\n",
				__func__, ret);

			//invalidate clk var so that none else will use it
			astc->clk_producer = ERR_PTR(-EINVAL);

			//no need to release anything as devm will take care of
			//if eventually
			return ret;
		}
	}

	return 0;
}

/**
 * astc_deregister_device() - Deregister the device
 * @astc_device: the astc device struct
 */
static void astc_deregister_device(struct astc_dev *astc_device)
{
	dev_info(astc_device->dev, "%s: deregistering astc device\n", __func__);
	misc_deregister(&astc_device->misc);
}

/**
 * astc_probe() - Driver initialization routine
 * @pdev: the platform device that needs to be initialized
 *
 * This is the driver initialization routine.
 *
 * It is called at boot time, when the system finds a device that this
 * driver requested to be a handler of (see &platform_driver struct below).
 *
 * Set up access to H/W registers, register the device, register the IRQ
 * resource, IOVMM page fault handler, spinlocks, etc.
 */
static int astc_probe(struct platform_device *pdev)
{
	struct astc_dev *astc = NULL;
	struct resource *res = NULL;
	int ret, irq = -1;

	/*
	 * This replaces the old pre-device-tree dev.platform_data
	 * To be populated with data coming from device tree
	 */
	astc = devm_kzalloc(&pdev->dev, sizeof(struct astc_dev), GFP_KERNEL);
	if (!astc) {
		//not enough memory
		return -ENOMEM;
	}

	astc->dev = &pdev->dev;

	spin_lock_init(&astc->slock);

	/* Get the memory resource and map it to kernel space. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	astc->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(astc->regs)) {
		dev_err(&pdev->dev, "failed to claim/map register region\n");
		return PTR_ERR(astc->regs);
	}

	/* Get IRQ resource and register IRQ handler. */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		if (irq != -EPROBE_DEFER)
			dev_err(&pdev->dev, "cannot get irq\n");
		return irq;
	}

	/* passing 0 as flags means the flags will be read from device-tree */
	ret = devm_request_irq(&pdev->dev, irq,
			       (void *)astc_irq_handler, 0,
			       pdev->name, astc);

	if (ret) {
		dev_err(&pdev->dev, "failed to install irq\n");
		return ret;
	}

	astc->misc.minor = MISC_DYNAMIC_MINOR;
	astc->misc.name = NODE_NAME;
	astc->misc.fops = &astc_fops;
	astc->misc.parent = &pdev->dev;
	ret = misc_register(&astc->misc);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&astc->contexts);

	spin_lock_init(&astc->lock_task);
	spin_lock_init(&astc->lock_ctx);
	init_waitqueue_head(&astc->suspend_wait);

	/* clock */
	ret = astc_clk_devm_get_prepare(astc);
	if (ret)
		goto err_clkget;

	platform_set_drvdata(pdev, astc);

#if defined(CONFIG_PM_DEVFREQ)
	if (!of_property_read_u32(pdev->dev.of_node, "astc,int_qos_minlock",
				  &astc->qos_req_level)) {
		if (astc->qos_req_level > 0) {
			pm_qos_add_request(&astc->qos_req,
					   PM_QOS_DEVICE_THROUGHPUT, 0);

			dev_info(&pdev->dev, "INT Min.Lock Freq. = %u\n",
				 astc->qos_req_level);
		}
	}
#endif
	pm_runtime_set_autosuspend_delay(&pdev->dev, 1000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	iovmm_set_fault_handler(&pdev->dev,
				astc_sysmmu_fault_handler, astc);

	ret = iovmm_activate(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: Failed to activate iommu (%d)\n",
			__func__, ret);
		goto err_iovmm;
	}

	//Power device on to access registers
	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev
			, "%s: failed to pwr on device with runtime pm (%d)\n",
			__func__, ret);
		goto err_pm;
	}

	// TODO: will this ever return <0 ?
	ret = astc_hw_get_version(astc->regs);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get H/W version\n");
		goto err_regs;
	} else {
		dev_info(&pdev->dev, "HW ver: %u", (unsigned int) ret);
		astc->version = ret;
	}

	/* Doublecheck we're getting sensible values from register reads,
	 * by reading the magic number reg
	 */
	if (astc_get_magic_number(astc->regs)
			!= ASTC_MAGIC_NUM_REG_RESETVALUE) {
		dev_err(&pdev->dev, "wrong magic number value.\n");
		goto err_regs;
	}

	/* Make sure that the irq register says it has not fired any irq yet.
	 * It cannot be otherwise, since we're in initialization phase.
	 */
	if (astc_hw_get_int_status(astc->regs)) {
		dev_err(&pdev->dev, "invalid interrupt status.\n");
		goto err_regs;
	}

	/* Encoding status register must not report errors, as we have not
	 * tried encoding anything yet
	 */
	if (get_hw_enc_status(astc->regs)) {
		dev_err(&pdev->dev, "wrong enc status at init stage.\n");
		goto err_regs;
	}

	pm_runtime_put_sync(&pdev->dev);

	/* create the workqueue scheduling the tasks for execution */
	astc->workqueue = alloc_ordered_workqueue(NODE_NAME, WQ_MEM_RECLAIM);
	if (!astc->workqueue) {
		ret = -ENOMEM;
		goto err_pm; /* put_sync has already been called */
	}

	dev_info(&pdev->dev, "hwastc probe completed successfully.\n");

	return 0;

err_regs:
	pm_runtime_put_sync(&pdev->dev);
err_pm:
	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	iovmm_deactivate(&pdev->dev);
err_iovmm:
	if (astc->qos_req_level > 0)
		pm_qos_remove_request(&astc->qos_req);
	if (!IS_ERR(astc->clk_producer)) /* devm will call clk_put */
		clk_unprepare(astc->clk_producer);
	platform_set_drvdata(pdev, NULL);
err_clkget:
	astc_deregister_device(astc);

	dev_err(&pdev->dev, "hwastc probe is failed.\n");

	return ret;
}

#ifdef CONFIG_PM_SLEEP
/*
 * System Power Management procedures (for whole-system suspension/resume)
 */
static int astc_suspend(struct device *dev)
{
	struct astc_dev *astc = dev_get_drvdata(dev);
	long ret = 0;
	unsigned long flags = 0;

	dev_dbg(dev, "%s: BEGIN\n", __func__);

	/*
	 * Lock needed to avoid racing with the worker, which has to do more
	 * than one atomic op. See astc_task_schedule for more details.
	 */
	spin_lock_irqsave(&astc->slock, flags);
	/*
	 * This will stop the next workqueue job from accessing the device
	 * until we are out of system suspend
	 */
	set_bit(DEV_SUSPEND, &astc->state);
	spin_unlock_irqrestore(&astc->slock, flags);

	dev_dbg(dev, "%s: wait for the device to allow suspension\n", __func__);

	/*
	 * NOTE: SEE DEV_SUSPEND doc to know when this can happen.
	 *
	 * wait for the current encoding job to done accessing the device
	 */
	ret = wait_event_timeout(astc->suspend_wait,
				 !test_bit(DEV_RUN, &astc->state),
				 ASTC_SUSPEND_TIMEOUT);

	dev_dbg(dev, "%s: suspension continuing...\n", __func__);

	/*
	 * The chance to hit the timeout is only != 0 when the conditions
	 * described in DEV_SUSPEND doc are verified.
	 * See DEV_SUSPEND doc for more details.
	 */
	if (!ret) {
		dev_err(dev, "%s: SUSPEND TIMEOUT EXPIRED\n", __func__);
	}

	dev_dbg(dev, "%s: END\n", __func__);

	return !ret ? -EAGAIN : 0;
}
/*
 * NOTE: the PM core will run this AFTER running pm_runtime_barrier and
 *       pm_disable_runtime, so we can assume that all runtime PM requests
 *       are either completed or canceled, at this point.
 *       Feel free to doublecheck that by reading runtime_pm.txt official doc.
 *
 *       Best practice seems to be to call pm_runtime_force_suspend here to
 *       reuse the runtime_suspend callback in drivers where that makes sense,
 *       like this one.
 */
static int astc_suspend_late(struct device *dev)
{
	struct astc_dev *astc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: BEGIN\n", __func__);
	pm_runtime_force_suspend(astc->dev);
	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}

static int astc_resume(struct device *dev)
{
	struct astc_dev *astc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: BEGIN\n", __func__);

	clear_bit(DEV_SUSPEND, &astc->state);

	/*
	 * wake the workqueue worker that was waiting for the device to come
	 * out of suspend, if any.
	 */
	wake_up(&astc->suspend_wait);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}
/*
 *       Best practice seems to be to call pm_runtime_force_resume here to
 *       reuse the runtime_resume callback in drivers where that makes sense,
 *       like this one.
 *
 *       NOTE: at this point Runtime PM is disabled (it was disabled by PM core
 *             in suspend phase).
 *             The PM core will call pm_runtime_enable() and pm_runtime_put()
 *             AFTER the resume_early callback.
 *             Feel free to doublecheck that by reading runtime_pm.txt doc.
 *
 *       If the device was already suspended before system sleep, then it will
 *       be resumed and then suspended again after this call, due to the
 *       pm_runtime_put() call mentioned at the beginning of this comment.
 *       This inefficiency might be fixed once the following is merged
 *       https://patchwork.kernel.org/patch/8496081/
 *
 *       If it was active before system sleep started, this will guarantee that
 *       state is restored.
 */
static int astc_resume_early(struct device *dev)
{
	struct astc_dev *astc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: BEGIN\n", __func__);
	pm_runtime_force_resume(astc->dev);
	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}
#endif


/**
 * astc_clock_gating() - Enable/Disable the clock source
 * @astc: the astc device struct
 * @on: the requestest gating status
 *
 * Request the system to Enable/Disable the clock source.
 *
 * We cannot read/write to H/W registers or use the device in any way
 * without enabling the clock source first.
 *
 * Return:
 *	0 on success,
 *	< 0 error code on error
 */
static int astc_clock_gating(struct astc_dev *astc, bool on)
{
	int ret = 0;

	if (on) {
		ret = clk_enable(astc->clk_producer);
		if (ret < 0) {
			dev_err(astc->dev, "%s failed to enable clock\n"
				, __func__);
			return ret;
		}
		dev_dbg(astc->dev, "astc clock enabled\n");
	} else {
		clk_disable(astc->clk_producer);
		dev_dbg(astc->dev, "astc clock disabled\n");
	}

	return 0;
}

#ifdef CONFIG_PM
/**
 * astc_runtime_suspend() - Runtime PM callback which suspends the device
 * @dev: the platform device to suspend
 *
 * Called by the Runtime PM framework when the device needs to be
 * suspended.
 * Disable the clock and update the QoS request.
 *
 * Return:
 *	0 on success,
 *	-EBUSY if the device can't be suspended because it's still busy
 */
static int astc_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct astc_dev *astc = platform_get_drvdata(pdev);
	int ret = 0;

	dev_dbg(dev, "%s: BEGIN\n", __func__);

	/*
	 * This can theoretically happen if the autosuspend timer started by
	 * a previous job expires after DEV_RUN is set (by a new job) but before
	 * the device usage counter is increased in enable_astc.
	 * In that case we return -EBUSY to tell runtime PM core that we're
	 * not ready to go in low-power state, as a new task is being executed.
	 */
	if (test_bit(DEV_RUN, &astc->state)) {
		dev_info(dev, "deferring suspend, device is still processing!");
		return -EBUSY;
	}

	dev_dbg(dev, "%s: gating clock, suspending\n", __func__);

	HWASTC_PROFILE(ret = astc_clock_gating(astc, false), "SWITCHING OFF CLOCK",
		       astc->dev);
	if (ret < 0)
		return ret;

	HWASTC_PROFILE(
	if (astc->qos_req_level > 0)
		pm_qos_update_request(&astc->qos_req, 0);,
	"UPDATE QoS REQUEST", astc->dev
	);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}
/**
 * astc_runtime_resume() - Runtime PM callback which resumes the device
 * @dev: the platform device to resume
 *
 * Called by the Runtime PM framework when the device needs to be
 * resumed.
 * Enable the clock and update the QoS request to set the frequency
 * at the desired level.
 *
 * Return:
 *	0
 */
static int astc_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct astc_dev *astc = platform_get_drvdata(pdev);
	int ret = 0;

	dev_dbg(dev, "%s: BEGIN\n", __func__);
	dev_dbg(dev, "%s: ungating clock, resuming\n", __func__);

	HWASTC_PROFILE(ret = astc_clock_gating(astc, true),
		       "SWITCHING ON CLOCK",
		       astc->dev);
	if (ret < 0)
		return ret;

	HWASTC_PROFILE(
		if (astc->qos_req_level > 0)
			pm_qos_update_request(&astc->qos_req,
					      astc->qos_req_level),
		"UPDATE QoS REQUEST", astc->dev
	);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}
#endif

static const struct dev_pm_ops astc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(astc_suspend, astc_resume)
	SET_LATE_SYSTEM_SLEEP_PM_OPS(astc_suspend_late, astc_resume_early)
	SET_RUNTIME_PM_OPS(astc_runtime_suspend, astc_runtime_resume, NULL)
};

/*
 * GCC will fire a warning because of the empty "end of list sentinel".
 * Suppress that.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
/*
 * of == OpenFirmware, aka DeviceTree
 * This allows matching devices which are specified as "vendorid,deviceid"
 * in the device tree to this driver
 */
static const struct of_device_id exynos_astc_match[] = {
{
	.compatible = "samsung,exynos-astc",
},
{ /* this is intentional: it's the end of list sentinel */ },
};
#pragma GCC diagnostic pop

static struct platform_driver astc_driver = {
	.driver = {
		/* This was used in pre-device-tree era.
		 * Any device advertising itself as MODULE_NAME would be matched
		 * to this driver.
		 * No ID table needed.
		 */
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &astc_pm_ops,
		/*
		 * Tell the kernel what dev tree names this driver can handle.
		 *
		 * At boot time, when the device-tree is instantiated, if a match occurs,
		 * probe() will called with the device that was matched as parameter.
		 *
		 * This is needed because device trees are meant to work with
		 * more than one OS, so the driver name might not match the name
		 * which is in the device tree.
		 */
		.of_match_table = of_match_ptr(exynos_astc_match),
		/* disable driver binding/unbinding, as this device is not
		 * hot-pluggable. */
		.suppress_bind_attrs = true,
	}
};

/*
 * Macro that will declare module init function that will register the driver
 * and set its probe function to be only called once.
 * This is to decrease runtime memory usage for devices which are not
 * hot-pluggable.
 */
builtin_platform_driver_probe(astc_driver, astc_probe);

MODULE_AUTHOR("Andrea Bernabei <a.bernabei@samsung.com>");
MODULE_DESCRIPTION("Samsung ASTC encoder driver");
MODULE_LICENSE("GPL");
