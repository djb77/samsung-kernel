/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/slab.h>
#include <linux/uaccess.h>

#include "score_log.h"
#include "score_util.h"
#include "score_ioctl.h"
#include "score_device.h"
#include "score_mmu.h"
#include "score_context.h"
#include "score_core.h"

static struct score_dev *score_dev;

static struct score_core *score_get_coredata(void)
{
	return dev_get_drvdata(&score_dev->dev);
}

static void *__score_core_get_context(struct score_core *core,
		struct file *file)
{
	int ret;
	struct score_context *sctx;
	unsigned long flags;

	score_enter();
	spin_lock_irqsave(&core->ctx_slock, flags);
	sctx = file->private_data;
	if (!sctx) {
		score_err("context was already destroyed\n");
		ret = -ECONNREFUSED;
		goto p_err;
	}

	if (sctx->state == SCORE_CONTEXT_STOP) {
		score_err("context already stopped\n");
		ret = -ECONNREFUSED;
		goto p_err;
	}
	atomic_inc(&sctx->ref_count);
	spin_unlock_irqrestore(&core->ctx_slock, flags);
	score_leave();
	return sctx;
p_err:
	spin_unlock_irqrestore(&core->ctx_slock, flags);
	return ERR_PTR(ret);
}

static void __score_core_put_context(struct score_context *sctx)
{
	score_enter();
	atomic_dec(&sctx->ref_count);
	score_leave();
}

static int score_ioctl_request(struct file *file,
		struct score_ioctl_request *req)
{
	int ret = 0;
	struct score_core *core;
	struct score_context *sctx;
	struct score_system *system;
	struct score_frame *frame;
	struct score_mmu_packet packet;

	score_enter();
	core = score_get_coredata();
	sctx = __score_core_get_context(core, file);
	if (IS_ERR(sctx)) {
		req->ret = PTR_ERR(sctx);
		goto p_err;
	}

	system = sctx->system;
	frame = score_frame_create(&system->interface.framemgr, sctx, true);
	if (!frame) {
		req->ret = -ENOMEM;
		score_err("Fail to alloc frame (sctx id %d)\n", sctx->id);
		goto p_err_frame;
	}

	packet.fd = req->packet_fd;
	ret = score_mmu_packet_prepare(&system->mmu, &packet);
	if (ret) {
		req->ret = ret;
		goto p_err_prepare;
	}
	frame->packet = packet.kvaddr;

	ret = sctx->ctx_ops->queue(sctx, frame);
	if (ret)
		goto p_err_queue;

	ret = sctx->ctx_ops->deque(sctx, frame);
	if (ret)
		goto p_err_queue;

	score_mmu_packet_unprepare(&system->mmu, &packet);
	score_frame_done(frame, &req->ret);
	req->timestamp[SCORE_TIME_SCQ_WRITE] =
		frame->timestamp[SCORE_TIME_SCQ_WRITE];
	req->timestamp[SCORE_TIME_ISR] =
		frame->timestamp[SCORE_TIME_ISR];

	score_frame_destroy(frame);
	__score_core_put_context(sctx);

	score_leave();
	return 0;
p_err_queue:
	score_mmu_packet_unprepare(&system->mmu, &packet);
	score_frame_done(frame, &req->ret);
p_err_prepare:
	score_frame_destroy(frame);
p_err_frame:
	__score_core_put_context(sctx);
p_err:
	/* return value is included in req->ret */
	return 0;
}

static int score_ioctl_get_dvfs(struct file *file,
		struct score_ioctl_get_dvfs *info)
{
	int ret = 0;
	struct score_core *core;
	struct score_context *sctx;
	struct score_pm *pm;

	score_enter();
	core = score_get_coredata();
	sctx = __score_core_get_context(core, file);
	if (IS_ERR(sctx)) {
		ret = PTR_ERR(sctx);
		goto p_err;
	}

	pm = &sctx->core->device->pm;
	ret = score_pm_qos_get_info(pm,
			&info->qos_count, &info->min_qos, &info->max_qos,
			&info->default_qos, &info->current_qos);
	if (ret)
		score_warn("runtime pm is not supported (%d)\n", ret);

	__score_core_put_context(sctx);
	score_leave();
p_err:
	info->ret = ret;
	return 0;
}

static int score_ioctl_set_dvfs(struct file *file,
		struct score_ioctl_set_dvfs *req)
{
	int ret = 0;
	struct score_core *core;
	struct score_context *sctx;
	struct score_pm *pm;
	struct score_ioctl_get_dvfs info;

	score_enter();
	core = score_get_coredata();
	sctx = __score_core_get_context(core, file);
	if (IS_ERR(sctx)) {
		ret = PTR_ERR(sctx);
		goto p_err;
	}

	pm = &sctx->core->device->pm;
	score_pm_qos_update(pm, req->request_qos);
	ret = score_pm_qos_get_info(pm,
			&info.qos_count, &info.min_qos, &info.max_qos,
			&info.default_qos, &info.current_qos);
	if (ret) {
		score_warn("runtime pm is not supported (%d)\n", ret);
		goto p_err_put;
	}
	req->current_qos = info.current_qos;

	if (req->request_qos > info.min_qos ||
			req->request_qos < info.max_qos) {
		ret = -EINVAL;
		goto p_err_put;
	}

	score_leave();
p_err_put:
	__score_core_put_context(sctx);
p_err:
	req->ret = ret;
	return 0;
}

static int score_ioctl_secure(struct file *file, struct score_ioctl_secure *sec)
{
	/* for secure mode change */
	return 0;
}

static int score_ioctl_request_nonblock(struct file *file,
		struct score_ioctl_request_nonblock *req)
{
	int ret = 0;
	struct score_core *core;
	struct score_context *sctx;
	struct score_system *system;
	struct score_frame *frame;
	struct score_mmu_packet packet;

	score_enter();
	core = score_get_coredata();
	sctx = __score_core_get_context(core, file);
	if (IS_ERR(sctx)) {
		ret = PTR_ERR(sctx);
		goto p_err;
	}

	system = sctx->system;
	frame = score_frame_create(&system->interface.framemgr, sctx, false);
	if (!frame) {
		ret = -ENOMEM;
		score_err("Fail to alloc frame (sctx id %d)\n", sctx->id);
		goto p_err_frame;
	}

	packet.fd = req->packet_fd;
	ret = score_mmu_packet_prepare(&system->mmu, &packet);
	if (ret)
		goto p_err_prepare;

	frame->packet = packet.kvaddr;

	ret = sctx->ctx_ops->queue(sctx, frame);
	if (ret)
		goto p_err_queue;

	score_mmu_packet_unprepare(&system->mmu, &packet);
	__score_core_put_context(sctx);

	req->ret = 0;
	req->kctx_id = frame->sctx->id;
	req->task_id = frame->frame_id;
	score_leave();
	return 0;
p_err_queue:
	score_mmu_packet_unprepare(&system->mmu, &packet);
p_err_prepare:
	score_frame_destroy(frame);
p_err_frame:
	__score_core_put_context(sctx);
p_err:
	req->ret = ret;
	return 0;
}

static int score_ioctl_request_wait(struct file *file,
		struct score_ioctl_request_nonblock *req)
{
	int ret = 0;
	struct score_core *core;
	struct score_context *sctx;
	struct score_frame_manager *framemgr;
	struct score_frame *frame;
	unsigned long flags;

	score_enter();
	core = score_get_coredata();
	sctx = __score_core_get_context(core, file);
	if (IS_ERR(sctx)) {
		req->ret = PTR_ERR(sctx);
		goto p_err;
	}

	framemgr = &sctx->system->interface.framemgr;
	spin_lock_irqsave(&framemgr->slock, flags);
	frame = score_frame_get_by_id(framemgr, req->task_id);
	if (!frame) {
		spin_unlock_irqrestore(&framemgr->slock, flags);
		req->ret = -EINVAL;
		score_warn("frame is already completed (task_id:%d)\n",
				req->task_id);
		goto p_err_frame;
	} else {
		if (score_frame_is_nonblock(frame)) {
			score_frame_set_block(frame);
		} else {
			spin_unlock_irqrestore(&framemgr->slock, flags);
			req->ret = -EINVAL;
			score_warn("frame isn't nonblocking (task_id:%d)\n",
					req->task_id);
			goto p_err_frame;
		}
	}
	spin_unlock_irqrestore(&framemgr->slock, flags);

	ret = sctx->ctx_ops->deque(sctx, frame);
	if (ret)
		goto p_err_queue;

	score_frame_done(frame, &req->ret);
	req->timestamp[SCORE_TIME_SCQ_WRITE] =
		frame->timestamp[SCORE_TIME_SCQ_WRITE];
	req->timestamp[SCORE_TIME_ISR] =
		frame->timestamp[SCORE_TIME_ISR];

	score_frame_destroy(frame);
	__score_core_put_context(sctx);

	score_leave();
	return 0;
p_err_queue:
	score_frame_done(frame, &req->ret);
	score_frame_destroy(frame);
p_err_frame:
	__score_core_put_context(sctx);
p_err:
	return 0;
}

static int score_ioctl_test(struct file *file, struct score_ioctl_test *info)
{
	/* verification */
	return 0;
}

const struct score_ioctl_ops score_ioctl_ops = {
	.score_ioctl_request		= score_ioctl_request,
	.score_ioctl_get_dvfs		= score_ioctl_get_dvfs,
	.score_ioctl_set_dvfs		= score_ioctl_set_dvfs,
	.score_ioctl_secure		= score_ioctl_secure,
	.score_ioctl_request_nonblock	= score_ioctl_request_nonblock,
	.score_ioctl_request_wait	= score_ioctl_request_wait,
	.score_ioctl_test		= score_ioctl_test
};

static int score_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct score_core *core;
	struct score_context *sctx;

	score_enter();
	core = score_get_coredata();

	if (mutex_lock_interruptible(&core->device_lock)) {
		score_err("mutex_lock_interruptible is fail\n");
		return -ERESTARTSYS;
	}

	ret = score_device_open(core->device);
	if (ret)
		goto p_err;

	mutex_unlock(&core->device_lock);

	sctx = score_context_create(core);
	if (IS_ERR_OR_NULL(sctx)) {
		ret = PTR_ERR(sctx);
		mutex_lock(&core->device_lock);
		goto p_err_create_context;
	}
	file->private_data = sctx;

	score_leave();
	return ret;
p_err_create_context:
	score_device_close(core->device);
p_err:
	mutex_unlock(&core->device_lock);
	return ret;
}

static int score_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct score_core *core;
	struct score_context *sctx;
	unsigned long flags;

	score_enter();
	core = score_get_coredata();

	spin_lock_irqsave(&core->ctx_slock, flags);
	sctx = file->private_data;
	file->private_data = NULL;
	sctx->state = SCORE_CONTEXT_STOP;
	spin_unlock_irqrestore(&core->ctx_slock, flags);

	score_context_destroy(sctx);

	if (mutex_lock_interruptible(&core->device_lock)) {
		score_err("mutex_lock_interruptible is fail\n");
		return -ERESTARTSYS;
	}

	score_device_close(core->device);

	mutex_unlock(&core->device_lock);

	score_leave();
	return ret;
}

static const struct file_operations score_file_ops = {
	.owner		= THIS_MODULE,
	.open		= score_open,
	.release	= score_release,
	.unlocked_ioctl = score_ioctl,
	.compat_ioctl	= score_compat_ioctl32,
};

int score_core_probe(struct score_device *device)
{
	int ret = 0;
	struct score_core *core;

	score_enter();
	core = &device->core;
	score_dev = &core->sdev;
	core->device = device;
	core->system = &device->system;

	INIT_LIST_HEAD(&core->ctx_list);
	core->ctx_count = 0;
	spin_lock_init(&core->ctx_slock);
#if defined(CONFIG_EXYNOS_SCORE)
	core->wait_time = 10000;
#else /* FPGA */
	core->wait_time = 50000;
#endif
	score_util_bitmap_init(core->ctx_map, SCORE_MAX_CONTEXT);
	mutex_init(&core->device_lock);
	core->ioctl_ops = &score_ioctl_ops;
	core->dpmu_print = false;

	score_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	score_dev->miscdev.name = "score";
	score_dev->miscdev.fops = &score_file_ops;
	score_dev->miscdev.parent = device->dev;
	dev_set_drvdata(&score_dev->dev, core);

	ret = misc_register(&score_dev->miscdev);
	if (ret)
		score_err("miscdevice is not registered (%d)\n", ret);

	score_leave();
	return ret;
}

void score_core_remove(struct score_core *core)
{
	score_enter();
	mutex_destroy(&core->device_lock);
	misc_deregister(&core->sdev.miscdev);
	score_leave();
}
