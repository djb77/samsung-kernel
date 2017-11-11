/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_hwlock.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "s5p_mfc_hwlock.h"

#include "s5p_mfc_nal_q.h"
#include "s5p_mfc_watchdog.h"
#include "s5p_mfc_opr.h"
#include "s5p_mfc_sync.h"

#include "s5p_mfc_inst.h"
#include "s5p_mfc_pm.h"
#include "s5p_mfc_cmd.h"
#include "s5p_mfc_reg.h"

#include "s5p_mfc_queue.h"
#include "s5p_mfc_utils.h"

static inline void mfc_print_hwlock(struct s5p_mfc_dev *dev)
{
	mfc_debug(2, "dev.hwlock.dev = 0x%lx, bits = 0x%lx, owned_by_irq = %d, wl_count = %d, transfer_owner = %d\n",
		dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
		dev->hwlock.wl_count, dev->hwlock.transfer_owner);
}

void s5p_mfc_init_hwlock(struct s5p_mfc_dev *dev)
{
	unsigned long flags;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	spin_lock_init(&dev->hwlock.lock);
	spin_lock_irqsave(&dev->hwlock.lock, flags);

	INIT_LIST_HEAD(&dev->hwlock.waiting_list);
	dev->hwlock.wl_count = 0;
	dev->hwlock.bits = 0;
	dev->hwlock.dev = 0;
	dev->hwlock.owned_by_irq = 0;
	dev->hwlock.transfer_owner = 0;

	spin_unlock_irqrestore(&dev->hwlock.lock, flags);
}

static int mfc_remove_listable_wq_dev(struct s5p_mfc_dev *dev)
{
	struct s5p_mfc_listable_wq *listable_wq;
	unsigned long flags;
	int ret = -1;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	mfc_print_hwlock(dev);

	list_for_each_entry(listable_wq, &dev->hwlock.waiting_list, list) {
		if (!listable_wq->dev)
			continue;

		mfc_debug(2, "Found dev and will delete it!\n");

		list_del(&listable_wq->list);
		dev->hwlock.wl_count--;

		ret = 0;
		break;
	}

	mfc_print_hwlock(dev);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);

	return ret;
}

static int mfc_remove_listable_wq_ctx(struct s5p_mfc_ctx *curr_ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_listable_wq *listable_wq;
	unsigned long flags;
	int ret = -1;

	if (!curr_ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	dev = curr_ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	mfc_print_hwlock(dev);

	list_for_each_entry(listable_wq, &dev->hwlock.waiting_list, list) {
		if (!listable_wq->ctx)
			continue;

		if (listable_wq->ctx->num == curr_ctx->num) {
			mfc_debug(2, "Found ctx and will delete it (%d)!\n", curr_ctx->num);

			list_del(&listable_wq->list);
			dev->hwlock.wl_count--;
			ret = 0;
			break;
		}
	}

	mfc_print_hwlock(dev);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);

	return ret;
}

/*
 * Return value description
 *    0: succeeded to get hwlock
 * -EIO: failed to get hwlock (time out)
 */
int s5p_mfc_get_hwlock_dev(struct s5p_mfc_dev *dev)
{
	int ret = 0;
	unsigned long flags;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	mutex_lock(&dev->hwlock_wq.wait_mutex);

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	mfc_print_hwlock(dev);

	if (dev->shutdown) {
		mfc_info_dev("Couldn't lock HW. Shutdown was called.\n");
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		mutex_unlock(&dev->hwlock_wq.wait_mutex);
		return -EINVAL;
	}

	if ((dev->hwlock.bits != 0) || (dev->hwlock.dev != 0)) {
		list_add_tail(&dev->hwlock_wq.list, &dev->hwlock.waiting_list);
		dev->hwlock.wl_count++;

		spin_unlock_irqrestore(&dev->hwlock.lock, flags);

		mfc_debug(2, "Waiting for hwlock to be released.\n");

		ret = wait_event_timeout(dev->hwlock_wq.wait_queue,
			((dev->hwlock.transfer_owner == 1) && (dev->hwlock.dev == 1)),
			msecs_to_jiffies(MFC_INT_TIMEOUT));

		MFC_TRACE_DEV_HWLOCK("get_hwlock_dev: before waiting\n");
		MFC_TRACE_DEV_HWLOCK(">>dev:0x%lx, bits:0x%lx, owned:%d, wl:%d, trans:%d\n",
				dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
				dev->hwlock.wl_count, dev->hwlock.transfer_owner);

		dev->hwlock.transfer_owner = 0;
		mfc_remove_listable_wq_dev(dev);
		if (ret == 0) {
			mfc_err_dev("Woken up but timed out\n");
			mfc_print_hwlock(dev);
			mutex_unlock(&dev->hwlock_wq.wait_mutex);
			return -EIO;
		} else {
			mfc_debug(2, "Woken up and got hwlock\n");
			mfc_print_hwlock(dev);
			mutex_unlock(&dev->hwlock_wq.wait_mutex);
		}
	} else {
		dev->hwlock.bits = 0;
		dev->hwlock.dev = 1;
		dev->hwlock.owned_by_irq = 0;

		MFC_TRACE_DEV_HWLOCK("get_hwlock_dev: no waiting\n");
		MFC_TRACE_DEV_HWLOCK(">>dev:0x%lx, bits:0x%lx, owned:%d, wl:%d, trans:%d\n",
				dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
				dev->hwlock.wl_count, dev->hwlock.transfer_owner);

		mfc_print_hwlock(dev);
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		mutex_unlock(&dev->hwlock_wq.wait_mutex);
	}

#ifdef NAL_Q_ENABLE
	/* Stop NAL-Q after getting hwlock */
	if (dev->nal_q_handle)
		s5p_mfc_nal_q_stop_if_started(dev);
#endif
	return 0;
}

/*
 * Return value description
 *    0: succeeded to get hwlock
 * -EIO: failed to get hwlock (time out)
 */
int s5p_mfc_get_hwlock_ctx(struct s5p_mfc_ctx *curr_ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_ctx *ctx = curr_ctx;
	int ret = 0;
	unsigned long flags;

	if (!curr_ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	dev = curr_ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	mutex_lock(&curr_ctx->hwlock_wq.wait_mutex);

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	mfc_print_hwlock(dev);

	if (dev->shutdown) {
		mfc_info_dev("Couldn't lock HW. Shutdown was called.\n");
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		mutex_unlock(&curr_ctx->hwlock_wq.wait_mutex);
		return -EINVAL;
	}

	if ((dev->hwlock.bits != 0) || (dev->hwlock.dev != 0)) {
		list_add_tail(&curr_ctx->hwlock_wq.list, &dev->hwlock.waiting_list);
		dev->hwlock.wl_count++;

		spin_unlock_irqrestore(&dev->hwlock.lock, flags);

		MFC_TRACE_CTX_HWLOCK("get_hwlock_ctx: before waiting\n");
		MFC_TRACE_CTX_HWLOCK(">>dev:0x%lx, bits:0x%lx, owned:%d, wl:%d, trans:%d\n",
				dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
				dev->hwlock.wl_count, dev->hwlock.transfer_owner);

		mfc_debug(2, "Waiting for hwlock to be released.\n");

		ret = wait_event_timeout(curr_ctx->hwlock_wq.wait_queue,
			((dev->hwlock.transfer_owner == 1) && (test_bit(curr_ctx->num, &dev->hwlock.bits))),
			msecs_to_jiffies(MFC_INT_TIMEOUT));

		MFC_TRACE_CTX_HWLOCK("get_hwlock_ctx: after waiting, ret:%d\n", ret);
		MFC_TRACE_CTX_HWLOCK(">>dev:0x%lx, bits:0x%lx, owned:%d, wl:%d, trans:%d\n",
				dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
				dev->hwlock.wl_count, dev->hwlock.transfer_owner);

		dev->hwlock.transfer_owner = 0;
		mfc_remove_listable_wq_ctx(curr_ctx);
		if (ret == 0) {
			mfc_err_dev("Woken up but timed out\n");
			mfc_print_hwlock(dev);
			mutex_unlock(&curr_ctx->hwlock_wq.wait_mutex);
			return -EIO;
		} else {
			mfc_debug(2, "Woken up and got hwlock\n");
			mfc_print_hwlock(dev);
			mutex_unlock(&curr_ctx->hwlock_wq.wait_mutex);
		}
	} else {
		dev->hwlock.bits = 0;
		dev->hwlock.dev = 0;
		set_bit(curr_ctx->num, &dev->hwlock.bits);
		dev->hwlock.owned_by_irq = 0;

		MFC_TRACE_CTX_HWLOCK("get_hwlock_ctx: no waiting\n");
		MFC_TRACE_CTX_HWLOCK(">>dev:0x%lx, bits:0x%lx, owned:%d, wl:%d, trans:%d\n",
				dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
				dev->hwlock.wl_count, dev->hwlock.transfer_owner);

		mfc_print_hwlock(dev);
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		mutex_unlock(&curr_ctx->hwlock_wq.wait_mutex);
	}

#ifdef NAL_Q_ENABLE
	/* Stop NAL-Q after getting hwlock */
	if (dev->nal_q_handle)
		s5p_mfc_nal_q_stop_if_started(dev);
#endif
	return 0;
}

/*
 * Return value description
 *  0: succeeded to release hwlock
 *  1: succeeded to release hwlock, hwlock is captured by another module
 * -1: error since device is waiting again.
 */
int s5p_mfc_release_hwlock_dev(struct s5p_mfc_dev *dev)
{
	struct s5p_mfc_listable_wq *listable_wq;
	unsigned long flags;
	int ret = -1;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	mfc_print_hwlock(dev);

	dev->hwlock.dev = 0;
	dev->hwlock.owned_by_irq = 0;

	if (dev->shutdown) {
		mfc_debug(2, "Couldn't wakeup module. Shutdown was called.\n");
		ret = 0;
	} else if (list_empty(&dev->hwlock.waiting_list)) {
		mfc_debug(2, "No waiting module.\n");
		ret = 0;
	} else {
		mfc_debug(2, "There is a waiting module.\n");
		listable_wq = list_entry(dev->hwlock.waiting_list.next, struct s5p_mfc_listable_wq, list);
		list_del(&listable_wq->list);
		dev->hwlock.wl_count--;

		if (listable_wq->dev) {
			mfc_err_dev("Device with hwlock is waiting for hwlock agian!\n");
			ret = -1;
		} else {
			mfc_debug(2, "Waking up another ctx\n");
			set_bit(listable_wq->ctx->num, &dev->hwlock.bits);

			dev->hwlock.transfer_owner = 1;

			MFC_TRACE_DEV_HWLOCK("release_hwlock_dev: wakeup.\n");
			MFC_TRACE_DEV_HWLOCK(">>dev:0x%lx, bits:0x%lx, owned:%d, wl:%d, trans:%d\n",
					dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
					dev->hwlock.wl_count, dev->hwlock.transfer_owner);

			wake_up(&listable_wq->wait_queue);
			ret = 1;
		}
	}

	mfc_print_hwlock(dev);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);
	return ret;
}

/*
 * Should be called with hwlock.lock
 *
 * Return value description
 * 0: succeeded to release hwlock
 * 1: succeeded to release hwlock, hwlock is captured by another module
 */
static int mfc_release_hwlock_ctx_protected(struct s5p_mfc_ctx *curr_ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_ctx *ctx = curr_ctx;
	struct s5p_mfc_listable_wq *listable_wq;
	int ret = -1;

	if (!curr_ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	dev = curr_ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	mfc_print_hwlock(dev);
	clear_bit(curr_ctx->num, &dev->hwlock.bits);
	dev->hwlock.owned_by_irq = 0;

	if (dev->shutdown) {
		mfc_debug(2, "Couldn't wakeup module. Shutdown was called.\n");
		ret = 0;
	} else if (list_empty(&dev->hwlock.waiting_list)) {
		mfc_debug(2, "No waiting module.\n");
		ret = 0;
	} else {
		mfc_debug(2, "There is a waiting module.\n");
		listable_wq = list_entry(dev->hwlock.waiting_list.next, struct s5p_mfc_listable_wq, list);
		list_del(&listable_wq->list);
		dev->hwlock.wl_count--;

		if (listable_wq->dev) {
			mfc_debug(2, "Waking up dev\n");
			dev->hwlock.dev = 1;
		} else {
			mfc_debug(2, "Waking up another ctx\n");
			set_bit(listable_wq->ctx->num, &dev->hwlock.bits);
		}

		dev->hwlock.transfer_owner = 1;

		MFC_TRACE_CTX_HWLOCK("release_hwlock_ctx: wakeup.\n");
		MFC_TRACE_CTX_HWLOCK(">>dev:0x%lx, bits:0x%lx, owned:%d, wl:%d, trans:%d\n",
				dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
				dev->hwlock.wl_count, dev->hwlock.transfer_owner);

		wake_up(&listable_wq->wait_queue);
		ret = 1;
	}

	mfc_print_hwlock(dev);
	return ret;
}

/*
 * Return value description
 * 0: succeeded to release hwlock
 * 1: succeeded to release hwlock, hwlock is captured by another module
 */
int s5p_mfc_release_hwlock_ctx(struct s5p_mfc_ctx *curr_ctx)
{
	struct s5p_mfc_dev *dev;
	unsigned long flags;
	int ret = -1;

	if (!curr_ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	dev = curr_ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	ret = mfc_release_hwlock_ctx_protected(curr_ctx);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);
	return ret;
}

static inline int mfc_yield_hwlock(struct s5p_mfc_dev *dev, struct s5p_mfc_ctx *curr_ctx)
{
	unsigned long flags;

	if (!curr_ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&dev->hwlock.lock, flags);

	mfc_release_hwlock_ctx_protected(curr_ctx);

	spin_unlock_irqrestore(&dev->hwlock.lock, flags);

	/* Trigger again if other instance's work is waiting */
	if (s5p_mfc_is_work_to_do(dev))
		queue_work(dev->butler_wq, &dev->butler_work);

	return 0;
}

/*
 * Should be called with hwlock.lock
 */
static inline void mfc_transfer_hwlock_ctx_protected(struct s5p_mfc_dev *dev, int curr_ctx_index)
{
	dev->hwlock.dev = 0;
	dev->hwlock.bits = 0;
	set_bit(curr_ctx_index, &dev->hwlock.bits);
}

/*
 * Should be called with hwlock.lock
 *
 * Return value description
 *   >=0: succeeded to get hwlock_bit for the context, index of new context
 *   -1, -EINVAL: failed to get hwlock_bit for a context
 */
static int mfc_try_to_get_new_ctx_protected(struct s5p_mfc_dev *dev)
{
	int ret = 0;
	int index;
	struct s5p_mfc_ctx *new_ctx;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	if (dev->shutdown) {
		mfc_info_dev("Couldn't lock HW. Shutdown was called.\n");
		return -EINVAL;
	}

	if (dev->sleep) {
		mfc_info_dev("Couldn't lock HW. Sleep was called.\n");
		return -EINVAL;
	}

	/* Check whether hardware is not running */
	if ((dev->hwlock.bits != 0) || (dev->hwlock.dev != 0)) {
		/* This is perfectly ok, the scheduled ctx should wait */
		mfc_debug(2, "Couldn't lock HW.\n");
		return -1;
	}

	/* Choose the context to run */
	index = s5p_mfc_get_new_ctx(dev);
	if (index < 0) {
		/* This is perfectly ok, the scheduled ctx should wait
		 * No contexts to run
		 */
		mfc_debug(2, "No ctx is scheduled to be run.\n");
		ret = -1;
		return ret;
	}

	new_ctx = dev->ctx[index];
	if (!new_ctx) {
		mfc_err_dev("no mfc context to run\n");
		ret = -1;
		return ret;
	}

	set_bit(new_ctx->num, &dev->hwlock.bits);
	ret = index;

	return ret;
}

/*
 * Should be called without hwlock holding
 *
 * Try to run an operation on hardware
 */
void s5p_mfc_try_run(struct s5p_mfc_dev *dev)
{
	int new_ctx_index;
	int ret;
	unsigned long flags;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}
	mfc_debug(2, "Try run dev: %p\n", dev);

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	mfc_print_hwlock(dev);

	new_ctx_index = mfc_try_to_get_new_ctx_protected(dev);
	if (new_ctx_index < 0) {
		mfc_debug(2, "Failed to get new context to run.\n");
		mfc_print_hwlock(dev);
		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
		return;
	}

	dev->hwlock.owned_by_irq = 1;

	mfc_print_hwlock(dev);
	spin_unlock_irqrestore(&dev->hwlock.lock, flags);

	ret = s5p_mfc_just_run(dev, new_ctx_index);
	if (ret)
		mfc_yield_hwlock(dev, dev->ctx[new_ctx_index]);
}

/*
 * Should be called without hwlock holding
 *
 */
void s5p_mfc_cleanup_work_bit_and_try_run(struct s5p_mfc_ctx *curr_ctx)
{
	struct s5p_mfc_dev *dev;

	if (!curr_ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	dev = curr_ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	s5p_mfc_clear_bit(curr_ctx->num, &dev->work_bits);

	s5p_mfc_try_run(dev);
}

void s5p_mfc_cache_flush(struct s5p_mfc_dev *dev, int is_drm)
{
	s5p_mfc_cmd_cache_flush(dev);
	if (s5p_mfc_wait_for_done_dev(dev, S5P_FIMV_R2H_CMD_CACHE_FLUSH_RET)) {
		mfc_err_dev("Failed to CACHE_FLUSH\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_CHACHE_FLUSH);
		s5p_mfc_dump_info_and_stop_hw(dev);
	}

	s5p_mfc_set_risc_base_addr(dev, (is_drm ? MFCBUF_DRM : MFCBUF_NORMAL));
	s5p_mfc_pm_clock_off(dev);

	dev->curr_ctx_is_drm = is_drm;
	s5p_mfc_pm_clock_on(dev);
}

#ifdef NAL_Q_ENABLE
/*
 * Return value description
 *  0: NAL-Q is handled successfully
 *  1: NAL_START command should be handled
 * -1: Error
*/
static int mfc_nal_q_just_run(struct s5p_mfc_ctx *ctx, int need_cache_flush)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	unsigned int ret = -1;

	nal_queue_handle *nal_q_handle = dev->nal_q_handle;

	if (!nal_q_handle) {
		mfc_err_dev("nal_q_handle is NULL\n");
		return ret;
	}

	if (nal_q_handle->nal_q_state != NAL_Q_STATE_STARTED
			&& nal_q_handle->nal_q_state != NAL_Q_STATE_STOPPED) {
		mfc_debug(2, "continue_clock_on = %d\n", dev->continue_clock_on);
		if (!dev->continue_clock_on) {
			s5p_mfc_pm_clock_on(dev);
		} else {
			dev->continue_clock_on = false;
		}
	}

	switch (nal_q_handle->nal_q_state) {
	case NAL_Q_STATE_CREATED:
		s5p_mfc_nal_q_init(dev, nal_q_handle);
	case NAL_Q_STATE_INITIALIZED:
		if (s5p_mfc_nal_q_check_enable(dev) == 0) {
			/* NAL START */
			ret = 1;
		} else {
			/* enable NAL QUEUE */
			if (need_cache_flush)
				s5p_mfc_cache_flush(dev, ctx->is_drm);

			mfc_info_ctx("NAL Q: start NAL QUEUE\n");
			s5p_mfc_nal_q_start(dev, nal_q_handle);

			if (s5p_mfc_nal_q_enqueue_in_buf(dev, ctx, nal_q_handle->nal_q_in_handle)) {
				mfc_debug(2, "NAL Q: Failed to enqueue input data\n");
				if (ctx->clear_work_bit) {
					s5p_mfc_clear_bit(ctx->num, &dev->work_bits);
					ctx->clear_work_bit = 0;
				}
				s5p_mfc_release_hwlock_ctx(ctx);
				ret = 0;
				break;
			}

			if (!nal_q_handle->nal_q_exception)
				s5p_mfc_clear_bit(ctx->num, &dev->work_bits);

			s5p_mfc_release_hwlock_ctx(ctx);

			if (s5p_mfc_ctx_ready(ctx))
				s5p_mfc_set_bit(ctx->num, &dev->work_bits);
			if (s5p_mfc_is_work_to_do(dev))
				queue_work(dev->butler_wq, &dev->butler_work);

			ret = 0;
		}
		break;
	case NAL_Q_STATE_STARTED:
		if (s5p_mfc_nal_q_check_enable(dev) == 0 ||
				nal_q_handle->nal_q_exception) {
			/* disable NAL QUEUE */
			s5p_mfc_nal_q_stop(dev, nal_q_handle);
			mfc_info_ctx("NAL Q: stop NAL QUEUE\n");
			if (s5p_mfc_wait_for_done_dev(dev,
					S5P_FIMV_R2H_CMD_COMPLETE_QUEUE_RET)) {
				mfc_err_dev("NAL Q: Failed to stop queue.\n");
				dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_STOP_NAL_Q);
				s5p_mfc_dump_info_and_stop_hw(dev);
	                }
			nal_q_handle->nal_q_exception = 0;
			ret = 1;
			s5p_mfc_pm_clock_on(dev);
			break;
		} else {
			/* NAL QUEUE */
			if (s5p_mfc_nal_q_enqueue_in_buf(dev, ctx, nal_q_handle->nal_q_in_handle)) {
				mfc_debug(2, "NAL Q: Failed to enqueue input data\n");
				if (ctx->clear_work_bit) {
					s5p_mfc_clear_bit(ctx->num, &dev->work_bits);
					ctx->clear_work_bit = 0;
				}
				s5p_mfc_release_hwlock_ctx(ctx);
				ret = 0;
				break;
			}

			if (!nal_q_handle->nal_q_exception)
				s5p_mfc_clear_bit(ctx->num, &dev->work_bits);

			s5p_mfc_release_hwlock_ctx(ctx);

			if (s5p_mfc_ctx_ready(ctx))
				s5p_mfc_set_bit(ctx->num, &dev->work_bits);
			if (s5p_mfc_is_work_to_do(dev))
				queue_work(dev->butler_wq, &dev->butler_work);
			ret = 0;
		}
		break;
	default:
		mfc_info_ctx("NAL Q: can't try command, nal_q_state : %d\n",
				nal_q_handle->nal_q_state);
		ret = -1;
		break;
	}

	return ret;
}
#endif

/* Run an operation on hardware */
int s5p_mfc_just_run(struct s5p_mfc_dev *dev, int new_ctx_index)
{
	struct s5p_mfc_ctx *ctx;
	unsigned int ret = 0;
	int need_cache_flush = 0;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	ctx = dev->ctx[new_ctx_index];
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	if (ctx->state == MFCINST_RUNNING)
		s5p_mfc_clean_ctx_int_flags(ctx);

	mfc_debug(2, "New context: %d\n", new_ctx_index);
	dev->curr_ctx = ctx->num;

	/* Got context to run in ctx */
	mfc_debug(2, "src: %d, dst: %d, state: %d, dpb_count = %d\n",
		s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
		s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_queue),
		ctx->state, ctx->dpb_count);
	mfc_debug(2, "ctx->state=%d\n", ctx->state);
	/* Last frame has already been sent to MFC
	 * Now obtaining frames from MFC buffer */

	/* Check if cache flush command is needed */
	if (dev->curr_ctx_is_drm != ctx->is_drm)
		need_cache_flush = 1;
	else
		dev->curr_ctx_is_drm = ctx->is_drm;

	mfc_debug(2, "need_cache_flush = %d, is_drm = %d\n", need_cache_flush, ctx->is_drm);

#ifdef NAL_Q_ENABLE
	if (dev->nal_q_handle) {
		ret = mfc_nal_q_just_run(ctx, need_cache_flush);
		if (ret == 0) {
			mfc_debug(2, "NAL_Q was handled\n");
			return ret;
		} else if (ret == 1){
			/* Path through */
			mfc_debug(2, "NAL_START will be handled\n");
		} else {
			return ret;
		}
	}
#else
	mfc_debug(2, "continue_clock_on = %d\n", dev->continue_clock_on);
	if (!dev->continue_clock_on) {
		s5p_mfc_pm_clock_on(dev);
	} else {
		dev->continue_clock_on = false;
	}
#endif

	if (need_cache_flush)
		s5p_mfc_cache_flush(dev, ctx->is_drm);

	if (ctx->type == MFCINST_DECODER) {
		switch (ctx->state) {
		case MFCINST_FINISHING:
			ret = s5p_mfc_run_dec_last_frames(ctx);
			break;
		case MFCINST_RUNNING:
		case MFCINST_SPECIAL_PARSING_NAL:
			ret = s5p_mfc_run_dec_frame(ctx);
			break;
		case MFCINST_INIT:
			ret = s5p_mfc_open_inst(ctx);
			break;
		case MFCINST_RETURN_INST:
			ret = s5p_mfc_close_inst(ctx);
			break;
		case MFCINST_GOT_INST:
		case MFCINST_SPECIAL_PARSING:
			ret = s5p_mfc_run_dec_init(ctx);
			break;
		case MFCINST_HEAD_PARSED:
			if (ctx->codec_buffer_allocated == 0) {
				ctx->clear_work_bit = 1;
				mfc_err_ctx("codec buffer is not allocated\n");
				ret = -EAGAIN;
				break;
			}
			ret = s5p_mfc_cmd_dec_init_buffers(ctx);
			break;
		case MFCINST_RES_CHANGE_INIT:
			ret = s5p_mfc_run_dec_last_frames(ctx);
			break;
		case MFCINST_RES_CHANGE_FLUSH:
			ret = s5p_mfc_run_dec_last_frames(ctx);
			break;
		case MFCINST_RES_CHANGE_END:
			mfc_debug(2, "Finished remaining frames after resolution change.\n");
			ctx->capture_state = QUEUE_FREE;
			mfc_debug(2, "Will re-init the codec.\n");
			ret = s5p_mfc_run_dec_init(ctx);
			break;
		case MFCINST_DPB_FLUSHING:
			ret = s5p_mfc_cmd_dpb_flush(ctx);
			break;
		default:
			mfc_info_ctx("can't try command(decoder just_run), state : %d\n", ctx->state);
			ret = -EAGAIN;
		}
	} else if (ctx->type == MFCINST_ENCODER) {
		switch (ctx->state) {
		case MFCINST_FINISHING:
			ret = s5p_mfc_run_enc_last_frames(ctx);
			break;
		case MFCINST_RUNNING:
		case MFCINST_RUNNING_NO_OUTPUT:
			ret = s5p_mfc_run_enc_frame(ctx);
			break;
		case MFCINST_INIT:
			ret = s5p_mfc_open_inst(ctx);
			break;
		case MFCINST_RETURN_INST:
			ret = s5p_mfc_close_inst(ctx);
			break;
		case MFCINST_GOT_INST:
			ret = s5p_mfc_run_enc_init(ctx);
			break;
		case MFCINST_HEAD_PARSED:
			ret = s5p_mfc_cmd_enc_init_buffers(ctx);
			break;
		case MFCINST_ABORT_INST:
			ret = s5p_mfc_abort_inst(ctx);
			break;
		default:
			mfc_info_ctx("can't try command(encoder just_run), state : %d\n", ctx->state);
			ret = -EAGAIN;
		}
	} else {
		mfc_err_ctx("invalid context type: %d\n", ctx->type);
		ret = -EAGAIN;
	}

	if (ret) {
		/* Check again the ctx condition and clear work bits
		 * if ctx is not available. */
		if (s5p_mfc_ctx_ready(ctx) == 0 || ctx->clear_work_bit) {
			s5p_mfc_clear_bit(ctx->num, &dev->work_bits);
			ctx->clear_work_bit = 0;
		}

		s5p_mfc_pm_clock_off(dev);
	}

	return ret;
}

void s5p_mfc_hwlock_handler_irq(struct s5p_mfc_dev *dev, struct s5p_mfc_ctx *curr_ctx,
		unsigned int reason, unsigned int err)
{
	int new_ctx_index;
	unsigned long flags;
	int ret;

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	if (!curr_ctx) {
		mfc_err_dev("no mfc context to run\n");
		return;
	}

	spin_lock_irqsave(&dev->hwlock.lock, flags);
	mfc_print_hwlock(dev);

	if (dev->hwlock.owned_by_irq) {
		if (dev->preempt_ctx > MFC_NO_INSTANCE_SET) {
			mfc_debug(2, "There is a preempt_ctx\n");
			dev->continue_clock_on = true;
			s5p_mfc_wake_up_ctx(curr_ctx, reason, err);
			new_ctx_index = dev->preempt_ctx;
			mfc_debug(2, "preempt_ctx is : %d\n", new_ctx_index);

			spin_unlock_irqrestore(&dev->hwlock.lock, flags);

			ret = s5p_mfc_just_run(dev, new_ctx_index);
			if (ret) {
				dev->continue_clock_on = false;
				mfc_yield_hwlock(dev, dev->ctx[new_ctx_index]);
			}
		} else if (!list_empty(&dev->hwlock.waiting_list)) {
			mfc_debug(2, "There is a waiting module for hwlock\n");
			dev->continue_clock_on = false;
			s5p_mfc_pm_clock_off(dev);

			spin_unlock_irqrestore(&dev->hwlock.lock, flags);

			s5p_mfc_release_hwlock_ctx(curr_ctx);
			s5p_mfc_wake_up_ctx(curr_ctx, reason, err);
			queue_work(dev->butler_wq, &dev->butler_work);
		} else {
			mfc_debug(2, "No preempt_ctx and no waiting module\n");
			new_ctx_index = s5p_mfc_get_new_ctx(dev);
			if (new_ctx_index < 0) {
				mfc_debug(2, "No ctx to run\n");
				/* No contexts to run */
				dev->continue_clock_on = false;
				s5p_mfc_pm_clock_off(dev);

				spin_unlock_irqrestore(&dev->hwlock.lock, flags);

				s5p_mfc_release_hwlock_ctx(curr_ctx);
				s5p_mfc_wake_up_ctx(curr_ctx, reason, err);
				queue_work(dev->butler_wq, &dev->butler_work);
			} else {
				mfc_debug(2, "There is a ctx to run\n");
				dev->continue_clock_on = true;
				s5p_mfc_wake_up_ctx(curr_ctx, reason, err);

				/* If cache flush command is needed, hander should stop */
				if (dev->curr_ctx_is_drm != dev->ctx[new_ctx_index]->is_drm) {
					mfc_debug(2, "Secure and nomal switching\n");
					mfc_debug(2, "DRM attribute is changed %d->%d\n",
							dev->curr_ctx_is_drm, dev->ctx[new_ctx_index]->is_drm);

					spin_unlock_irqrestore(&dev->hwlock.lock, flags);

					s5p_mfc_release_hwlock_ctx(curr_ctx);
					queue_work(dev->butler_wq, &dev->butler_work);
				} else {
					mfc_debug(2, "Work to do successively (next ctx: %d)\n", new_ctx_index);
					mfc_transfer_hwlock_ctx_protected(dev, new_ctx_index);

					spin_unlock_irqrestore(&dev->hwlock.lock, flags);

					ret = s5p_mfc_just_run(dev, new_ctx_index);
					if (ret) {
						dev->continue_clock_on = false;
						mfc_yield_hwlock(dev, dev->ctx[new_ctx_index]);
					}
				}
			}
		}
	} else {
		mfc_debug(2, "hwlock is NOT owned by irq\n");
		dev->continue_clock_on = false;
		s5p_mfc_pm_clock_off(dev);
		s5p_mfc_wake_up_ctx(curr_ctx, reason, err);
		queue_work(dev->butler_wq, &dev->butler_work);

		spin_unlock_irqrestore(&dev->hwlock.lock, flags);
	}
	mfc_print_hwlock(dev);
}
