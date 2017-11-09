/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/random.h>
#include <linux/slab.h>

#include "score-config.h"
#include "score-interface.h"
#include "score-regs.h"
#include "score-regs-user-def.h"
#include "../score-vertexmgr.h"
#include "../score-device.h"
#include "score-debug.h"
#include "score-lock.h"
#include "score-dump.h"
#include "score-time.h"

static void score_int_code_clear_bits(struct score_interface *interface,
		unsigned int bits)
{
	unsigned int temp;

	spin_lock(&interface->int_slock);
	SCORE_BAKERY_LOCK(interface->regs, CPU_ID);
	temp = readl(interface->regs + SCORE_INT_CODE);
	temp = (temp & (~bits)) & 0xFFFFFFFF;
	writel(temp, interface->regs + SCORE_INT_CODE);
	SCORE_BAKERY_UNLOCK(interface->regs, CPU_ID);
	spin_unlock(&interface->int_slock);
}

static void score_int_code_clear(struct score_interface *interface)
{
	spin_lock(&interface->int_slock);
	SCORE_BAKERY_LOCK(interface->regs, CPU_ID);
	writel(0x0, interface->regs + SCORE_INT_CODE);
	SCORE_BAKERY_UNLOCK(interface->regs, CPU_ID);
	spin_unlock(&interface->int_slock);
}

static irqreturn_t interface_isr(int irq, void *data)
{
	unsigned long flags;

	struct score_interface *interface = (struct score_interface *)data;
	struct score_system *system;
	struct score_device *device;
	unsigned int irq_code, irq_code_req;

	struct score_framemgr *iframemgr;
	struct score_frame *iframe;
	struct score_framemgr *framemgr;
	int fid, vid;

#ifdef ENABLE_TARGET_TIME
	struct timespec time;
	time = score_time_get();
#endif

	system = container_of(interface, struct score_system, interface);
	device = container_of(system, struct score_device, system);
	iframemgr = &interface->framemgr;

	spin_lock(&interface->int_slock);
	SCORE_BAKERY_LOCK(interface->regs, CPU_ID);
	irq_code = readl(interface->regs + SCORE_INT_CODE);
	SCORE_BAKERY_UNLOCK(interface->regs, CPU_ID);
	spin_unlock(&interface->int_slock);

	if (unlikely(irq_code & SCORE_INT_ABNORMAL_MASK)) {
		// Related to SCore dump and Profiler
		irq_code_req = irq_code & SCORE_INT_DUMP_PROFILER_MASK;
		if (irq_code_req) {
			switch (irq_code_req) {
			case SCORE_INT_CORE_DUMP_DONE:
				score_info("SCore dump done occurred (0x%08X)\n", irq_code);
				score_dump_queue_work(system);
				score_int_code_clear_bits(interface,
						SCORE_INT_CORE_DUMP_DONE);
				break;
			case SCORE_INT_PROFILER_START:
				score_info("SCore profiler started (0x%08X)\n", irq_code);
				//TODO: need next step
				score_int_code_clear_bits(interface,
						SCORE_INT_PROFILER_START);
				break;
			case SCORE_INT_PROFILER_END:
				score_info("SCore profiler ended (0x%08X)\n", irq_code);
				//TODO: need next step
				score_int_code_clear_bits(interface,
						SCORE_INT_PROFILER_END);
				break;
			default:
				score_info("Multiple SCore dump interrupt occurred (0x%08X)\n",
						irq_code);
				//TODO: need next step
				score_int_code_clear_bits(interface,
						SCORE_INT_DUMP_PROFILER_MASK);
				break;
			}
		}
		// HW exception case
		irq_code_req = irq_code & SCORE_INT_HW_EXCEPTION_MASK;
		if (irq_code_req) {
			switch (irq_code_req) {
			case SCORE_INT_BUS_ERROR:
				score_warn("Bus error occurred (0x%08X)\n", irq_code);
				break;
			case SCORE_INT_DMA_CONFLICT:
				score_warn("Dma conflict occurred (0x%08X)\n", irq_code);
				break;
			case SCORE_INT_CACHE_UNALIGNED:
				score_warn("Cache unaligned occurred (0x%08X)\n", irq_code);
				break;
			default:
				score_warn("Multiple hw exception occurred (0x%08X)\n", irq_code);
				break;
			}
			score_int_code_clear(interface);

			score_system_get_run_state(system, NULL, &fid, &vid, NULL);

			spin_lock_irqsave(&iframemgr->slock, flags);
			score_frame_g_entire(iframemgr, vid, fid, &iframe);
			if (!iframe) {
				score_warn("frame is already destroyed (vid:%d,fid:%d)\n",
						vid, fid);
			} else if (iframe->state == SCORE_FRAME_STATE_PROCESS) {
#ifdef ENABLE_TARGET_TIME
				iframe->frame->ts[1] = time;
#endif
				iframe->message = SCORE_FRAME_FAIL;
				score_frame_trans_pro_to_com(iframe);
				queue_kthread_work(&interface->worker, &iframe->work);
			} else {
#ifdef ENABLE_TARGET_TIME
				iframe->frame->ts[1] = time;
#endif
				iframe->frame->message = SCORE_FRAME_FAIL;
				framemgr = iframe->frame->owner;
				wake_up(&framemgr->done_wq);
			}

			score_system_reset(system, RESET_OPT_FORCE |
					RESET_OPT_RESTART);
			spin_unlock_irqrestore(&iframemgr->slock, flags);

			goto p_next;
		}

		// SCore sw assertion case
		irq_code_req = irq_code & SCORE_INT_SW_ASSERT;
		if (irq_code_req) {
			score_warn("SCore sw assertion occurred (0x%08X)\n", irq_code);
			//TODO: need next step
			score_int_code_clear_bits(interface, SCORE_INT_SW_ASSERT);
		}
	} else {
		struct score_fw_dev *score_fw_device;
		unsigned int packet[4];
		struct score_ipc_packet *result_packet;
		struct vb_buffer *buffer;

		score_fw_device = &device->fw_dev;
		result_packet = (struct score_ipc_packet *)packet;
		score_fw_queue_get(score_fw_device->out_queue, result_packet);

		spin_lock_irqsave(&iframemgr->slock, flags);
		score_frame_g_entire(iframemgr, result_packet->header.vctx_id,
				result_packet->header.task_id, &iframe);
		if (!iframe) {
			score_warn("frame is already destroyed (vid:%d,fid:%d)\n",
					result_packet->header.vctx_id,
					result_packet->header.task_id);
		} else if (iframe->state == SCORE_FRAME_STATE_PROCESS) {
#ifdef ENABLE_TARGET_TIME
			iframe->frame->ts[1] = time;
#endif
			score_frame_trans_pro_to_com(iframe);

			buffer = &iframe->frame->buffer;
			memcpy((void *)buffer->m.userptr, result_packet,
					(result_packet->size.packet_size + 1) << 2);

			queue_kthread_work(&interface->worker, &iframe->work);

		} else {
#ifdef ENABLE_TARGET_TIME
			iframe->frame->ts[1] = time;
#endif
			iframe->frame->message = SCORE_FRAME_FAIL;
			framemgr = iframe->frame->owner;
			wake_up(&framemgr->done_wq);
		}
		spin_unlock_irqrestore(&iframemgr->slock, flags);
#if defined (ENABLE_TARGET_TIME) && defined (DBG_SCV_KERNEL_TIME)
		printk("@@@SCORE [PERF][ID : %d][End] %ld.%09ld second \n",
				result_packet->header.kernel_name, time.tv_sec, time.tv_nsec);
#endif
	}
p_next:
	writel(0x0, interface->regs + SCORE_SCORE2HOST);

	return IRQ_HANDLED;
}

int score_interface_open(struct score_interface *interface)
{
	int ret = 0;
	struct score_system *system;
	struct score_device *device;

	system = container_of(interface, struct score_system, interface);
	device = container_of(system, struct score_device, system);
	score_vertexmgr_itf_register(&device->vertexmgr, interface);

	return ret;
}

int score_interface_close(struct score_interface *interface)
{
	int ret = 0;

	score_vertexmgr_itf_unregister(interface->vertexmgr, interface);

	return ret;
}

int score_interface_probe(struct score_interface *interface,
		struct device *dev, void __iomem *regs,
		resource_size_t regs_size, u32 irq0)
{
	int ret = 0;
	char name[30];
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	ret = devm_request_irq(dev, irq0, interface_isr, 0, dev_name(dev), interface);
	if (ret) {
		probe_err("devm_request_irq(0) is fail(%d)\n", ret);
		goto p_err_irq;
	}

	init_kthread_worker(&interface->worker);
	snprintf(name, sizeof(name), "score_interface");
	interface->task_itf = kthread_run(kthread_worker_fn, &interface->worker, name);
	if (IS_ERR_OR_NULL(interface->task_itf)) {
		score_err("interface kthread is not running \n");
		ret = -EINVAL;
		goto p_err_kthread;
	}

	ret = sched_setscheduler_nocheck(interface->task_itf, SCHED_FIFO, &param);
	if (ret) {
		score_err("sched_setscheduler_nocheck is fail(%d)\n", ret);
		goto p_err_sched;
	}

	spin_lock_init(&interface->int_slock);
	interface->regs = regs;
	interface->regs_size = regs_size;

	return ret;
p_err_sched:
	kthread_stop(interface->task_itf);
p_err_kthread:
	devm_free_irq(dev, irq0, interface);
p_err_irq:
	return ret;
}

void score_interface_release(struct score_interface *interface,
		struct device *dev, u32 irq0)
{
	kthread_stop(interface->task_itf);
	devm_free_irq(dev, irq0, interface);
}
