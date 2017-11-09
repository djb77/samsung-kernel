/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/random.h>
#include <linux/slab.h>

#include "vpu-config.h"
#include "vpu-interface.h"
#include "vpu-mailbox.h"
#include "vpu-io.h"
#include "vpu-framemgr.h"
#include "../vpu-graphmgr.h"
#include "../vpu-device.h"

#include "lib/vpul-errno.h"
#include "lib/vpul-translator.h"
#include "lib/vpu-fwif-mbox.h"

#define TURN_AROUND_TIME (HZ/20)

#define enter_request_barrier(frame) mutex_lock(frame->lock);
#define exit_request_barrier(frame) mutex_unlock(frame->lock);
#define init_process_barrier(itf) spin_lock_init(&itf->process_barrier);
#define enter_process_barrier(itf) spin_lock_irq(&itf->process_barrier);
#define exit_process_barrier(itf) spin_unlock_irq(&itf->process_barrier);

static void WORK_TO_FREE(struct vpu_work_list *work_list, struct vpu_work *work)
{
	unsigned long flags;

	BUG_ON(!work_list);
	BUG_ON(!work);

	spin_lock_irqsave(&work_list->slock, flags);
	list_add_tail(&work->list, &work_list->free_head);
	work_list->free_cnt++;
	spin_unlock_irqrestore(&work_list->slock, flags);
}

static void WORK_FR_FREE(struct vpu_work_list *work_list, struct vpu_work **work)
{
	unsigned long flags;

	BUG_ON(!work_list);
	BUG_ON(!work);

	spin_lock_irqsave(&work_list->slock, flags);
	if (work_list->free_cnt) {
		*work = container_of(work_list->free_head.next, struct vpu_work, list);
		list_del(&(*work)->list);
		work_list->free_cnt--;
	} else {
		*work = NULL;
	}
	spin_unlock_irqrestore(&work_list->slock, flags);
}

static void WORK_TO_REPLY(struct vpu_work_list *work_list, struct vpu_work *work)
{
	unsigned long flags;

	BUG_ON(!work_list);
	BUG_ON(!work);

	spin_lock_irqsave(&work_list->slock, flags);
	list_add_tail(&work->list, &work_list->reply_head);
	work_list->reply_cnt++;
	spin_unlock_irqrestore(&work_list->slock, flags);
}

static void WORK_FR_REPLY(struct vpu_work_list *work_list, struct vpu_work **work)
{
	unsigned long flags;

	BUG_ON(!work_list);
	BUG_ON(!work);

	spin_lock_irqsave(&work_list->slock, flags);
	if (work_list->reply_cnt) {
		*work = container_of(work_list->reply_head.next, struct vpu_work, list);
		list_del(&(*work)->list);
		work_list->reply_cnt--;
	} else {
		*work = NULL;
	}
	spin_unlock_irqrestore(&work_list->slock, flags);
}

static void INIT_WORK_LIST(struct vpu_work_list *work_list, u32 count)
{
	u32 i;

	work_list->free_cnt = 0;
	work_list->reply_cnt = 0;
	INIT_LIST_HEAD(&work_list->free_head);
	INIT_LIST_HEAD(&work_list->reply_head);
	spin_lock_init(&work_list->slock);
	init_waitqueue_head(&work_list->wait_queue);
	for (i = 0; i < count; ++i)
		WORK_TO_FREE(work_list, &work_list->work[i]);
}

static inline void __set_reply(struct vpu_interface *interface, u32 graph_id)
{
	BUG_ON(graph_id >= VPU_MAX_GRAPH);
	interface->reply[graph_id].valid = 1;
	wake_up(&interface->reply_queue);
}

static inline void __clr_reply(struct vpu_interface *interface, u32 graph_id)
{
	BUG_ON(graph_id >= VPU_MAX_GRAPH);
	interface->reply[graph_id].valid = 0;
}

static int __wait_reply(struct vpu_interface *interface, u32 graph_id)
{
	int ret = 0;
	int remain_time, try_count;
	struct vpu_mailbox_ctrl *mctrl;
	u16 *wptr_ofs16, *rptr_ofs16;
	u32 cmd, type;

	BUG_ON(!interface);
	BUG_ON(graph_id >= VPU_MAX_GRAPH);
	BUG_ON(!interface->request[graph_id]);
	BUG_ON(interface->request[graph_id]->param3 >= VPU_MTYPE_H2F_CNT);

	cmd = interface->request[graph_id]->message;
	type = interface->request[graph_id]->param3;
	mctrl = interface->private_data;
	wptr_ofs16 = &mctrl->stack->h2f[type].wptr_ofs16;
	rptr_ofs16 = &mctrl->stack->h2f[type].rptr_ofs16;

	try_count = 1000;
	while (--try_count && (*wptr_ofs16 != *rptr_ofs16)) {
		vpu_info("waiting vpu reply(%d, %d)...(%d)\n", *wptr_ofs16, *rptr_ofs16, try_count);
		msleep(1);
	}

	if (try_count <= 0) {
		vpu_err("waiting vpu reply is timeout\n");
		ret = -EINVAL;
		goto p_err;
	}

	remain_time = wait_event_timeout(interface->reply_queue,
		interface->reply[graph_id].valid, VPU_COMMAND_TIMEOUT);
	if (!remain_time) {
		vpu_err("%d command : reply is timeout\n", cmd);
		ret = -ETIME;
		goto p_err;
	}

p_err:
	return ret;
}

static void __send_interrupt(struct vpu_interface *interface)
{
	u32 try_count;
	u32 type, offset, val;

	BUG_ON(!interface);
	BUG_ON(!interface->process);
	BUG_ON(interface->process->param3 >= VPU_MTYPE_H2F_CNT);

	type = interface->process->param3;
	offset = (type == VPU_MTYPE_H2F_NORMAL) ? 0x10 : 0x14;

	try_count = 100;
	val = readl(interface->regs + offset);
	while (--try_count && val) {
		vpu_warn("waiting interrupt clear(%d)...(%d)\n", val, try_count);
		writel(0x0, interface->regs + offset);
		val = readl(interface->regs + offset);
	}

	writel(0x100, interface->regs + offset);
	udelay(1);
	writel(0x0, interface->regs + offset);
}

static int __vpu_set_cmd(struct vpu_interface *interface,
	struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpu_mailbox_ctrl *mctrl;
	void *payload;
	u32 cmd, gid, cid, size, type;
	ulong flag;

	BUG_ON(!interface);
	BUG_ON(!iframe);
	BUG_ON(!iframe->lock);
	BUG_ON(iframe->param1 >= VPU_MAX_GRAPH);

	iframemgr = &interface->framemgr;
	mctrl = interface->private_data;
	cmd = iframe->message;
	payload = (void *)iframe->param0;
	gid = iframe->param1;
	size = iframe->param2;
	type = iframe->param3;
	cid = iframe->index;

	enter_request_barrier(iframe);
	interface->request[gid] = iframe;

	enter_process_barrier(interface);
	interface->process = iframe;

	ret = vpu_mbox_ready(mctrl, size, type);
	if (ret) {
		interface->process = NULL;
		exit_process_barrier(interface);
		interface->request[gid] = NULL;
		exit_request_barrier(iframe);
		vpu_err("vpu_mbox_ready is fail(%d)", ret);
		goto p_err;
	}

	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_req_to_pro(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	ret = vpu_mbox_write(mctrl, payload, size, type, gid, cmd, cid);
	if (ret) {
		interface->process = NULL;
		exit_process_barrier(interface);
		interface->request[gid] = NULL;
		exit_request_barrier(iframe);
		pr_err("vpu_mbox_write_request fail (%d)\n", ret);
		goto p_err;
	}

	__send_interrupt(interface);

#ifdef DBG_MAILBOX_DATA
	vpu_info("[I%ld][MBOX] CMD : %d, ID : %d\n", iframe->param1, cmd, cid);
	vpu_debug_memdump16(payload, (u16 *)(payload + size));
#endif

	interface->process = NULL;
	exit_process_barrier(interface);

	ret = __wait_reply(interface, gid);
	if (ret) {
		interface->request[gid] = NULL;
		exit_request_barrier(iframe);
		vpu_err("%d command is timeout", cmd);
		ret = -ETIME;
		goto p_err;
	}

	if (gid >= VPU_MAX_GRAPH) {
		interface->request[gid] = NULL;
		exit_request_barrier(iframe);
		vpu_err("graph id is invalid(%d)\n", gid);
		ret = -EINVAL;
		goto p_err;
	}

	if (interface->reply[gid].param1) {
		interface->request[gid] = NULL;
		exit_request_barrier(iframe);
		vpu_err("%d command is error(%d)\n", cmd, interface->reply[gid].param1);
		ret = interface->reply[gid].param1;
		goto p_err;
	}

	__clr_reply(interface, gid);

	interface->request[gid] = NULL;
	exit_request_barrier(iframe);

p_err:
	return ret;
}

static int __vpu_set_cmd_nblk(struct vpu_interface *interface,
	struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpu_mailbox_ctrl *mctrl;
	void *payload;
	u32 cmd, gid, cid, size, type;
	ulong flag;

	BUG_ON(!interface);
	BUG_ON(!iframe);

	iframemgr = &interface->framemgr;
	mctrl = interface->private_data;
	cmd = iframe->message;
	payload = (void *)iframe->param0;
	gid = iframe->param1;
	size = iframe->param2;
	type = iframe->param3;
	cid = iframe->index;

	enter_process_barrier(interface);
	interface->process = iframe;

	ret = vpu_mbox_ready(mctrl, size, type);
	if (ret) {
		interface->process = NULL;
		exit_process_barrier(interface);
		vpu_err("vpu_mbox_ready is fail(%d)", ret);
		goto p_err;
	}

	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_req_to_pro(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	ret = vpu_mbox_write(mctrl, payload, size, type, gid, cmd, cid);
	if (ret) {
		interface->process = NULL;
		exit_process_barrier(interface);
		pr_err("vpu_mbox_write_request fail (%d)\n", ret);
		goto p_err;
	}

	__send_interrupt(interface);

#ifdef DBG_MAILBOX_DATA
	vpu_info("[I%ld][MBOX] CMD : %d, ID : %d\n", iframe->param1, cmd, cid);
	vpu_debug_memdump16(payload, (u16 *)(payload + size));
#endif

	interface->process = NULL;
	exit_process_barrier(interface);

p_err:
	return ret;
}

static int __vpu_interface_cleanup(struct vpu_interface *interface)
{
	int ret = 0;
	struct vpu_framemgr *framemgr;
	struct vpu_frame *frame;
	ulong flags;
	u32 i;

	framemgr = &interface->framemgr;

	if (framemgr->req_cnt) {
		vpu_err("[ITF] request count is NOT zero(%d)\n", framemgr->req_cnt);
		ret += framemgr->req_cnt;
	}

	if (framemgr->pro_cnt) {
		vpu_err("[ITF] process count is NOT zero(%d)\n", framemgr->pro_cnt);
		ret += framemgr->pro_cnt;
	}

	if (framemgr->com_cnt) {
		vpu_err("[ITF] complete count is NOT zero(%d)\n", framemgr->com_cnt);
		ret += framemgr->com_cnt;
	}

	if (ret) {
		framemgr_e_barrier_irqs(framemgr, 0, flags);

		for (i = 1; i < framemgr->tot_cnt; ++i) {
			frame = &framemgr->frame[i];
			if (frame->state == VPU_FRAME_STATE_FREE)
				continue;

			vpu_frame_trans_any_to_fre(framemgr, frame);
			ret--;
		}

		framemgr_x_barrier_irqr(framemgr, 0, flags);
	}

	return ret;
}

void vpu_interface_print(struct vpu_interface *interface)
{
	DLOG_INIT();
	u32 i;
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *iframe, *itemp;
	struct vpu_mailbox_ctrl *mctrl;

	BUG_ON(!interface);

	iframemgr = &interface->framemgr;

	DLOG("REQUEST LIST(%d) :", iframemgr->req_cnt);
	list_for_each_entry_safe(iframe, itemp, &iframemgr->req_list, list) {
		DLOG(" %d(%d, %d)", iframe->index, iframe->param1, iframe->message);
	}
	vpu_info("%s\n", DLOG_OUT());

	DLOG("PROCESS LIST(%d) :", iframemgr->pro_cnt);
	list_for_each_entry_safe(iframe, itemp, &iframemgr->pro_list, list) {
		DLOG(" %d(%d, %d)", iframe->index, iframe->param1, iframe->message);
	}
	vpu_info("%s\n", DLOG_OUT());

	DLOG("COMPLETE LIST(%d) :", iframemgr->com_cnt);
	list_for_each_entry_safe(iframe, itemp, &iframemgr->com_list, list) {
		DLOG(" %d(%d, %d)", iframe->index, iframe->param1, iframe->message);
	}
	vpu_info("%s\n", DLOG_OUT());

	mctrl = interface->private_data;
	vpu_info("active chain mask : %X\n", mctrl->stack->info.active_chain_mask);
	for (i = 0; i < VPUH_HW_CHAINS; ++i)
		vpu_info("chain[%d] : %d\n", i, mctrl->stack->info.chains[i].task_id);
}

static irqreturn_t interface_isr(int irq, void *data)
{
	int ret = 0;
	struct vpu_interface *interface;
	struct vpu_mailbox_ctrl *mctrl;
	struct vpu_mailbox_f2h *mbox;
	struct work_struct *work_queue;
	struct vpu_work_list *work_list;
	struct vpu_work *work;
	char payload[100];
	u16 wmsg_idx;

	interface = (struct vpu_interface *)data;
	mctrl = interface->private_data;
	work_queue = &interface->work_queue;
	work_list = &interface->work_list;

	mbox = &mctrl->stack->f2h[VPU_MTYPE_F2H_SYS_ERROR];
	wmsg_idx = mbox->wmsg_idx;
	while (wmsg_idx != mbox->rmsg_idx) {
		struct vpu_mailbox_syserr *reply;

		ret = vpu_mbox_read(mctrl, payload, VPU_MTYPE_F2H_SYS_ERROR, data);
		if (ret) {
			vpu_err("vpu_mbox_read is fail(%d)\n", ret);
			break;
		}

		WORK_FR_FREE(work_list, &work);
		if (work) {
			reply = (struct vpu_mailbox_syserr *)payload;
			work->message = VPU_MTYPE_F2H_SYS_ERROR;
			work->param0 = reply->err_type;
			work->param1 = reply->info1;
			work->param2 = reply->info2;
			WORK_TO_REPLY(work_list, work);

			if (!work_pending(work_queue))
				schedule_work(work_queue);
		} else {
			vpu_err("free work is empty\n");
		}
	}

	mbox = &mctrl->stack->f2h[VPU_MTYPE_F2H_NORMAL_RESPONSE];
	wmsg_idx = mbox->wmsg_idx;
	while (wmsg_idx != mbox->rmsg_idx) {
		struct vpu_mailbox_reply *reply;

		ret = vpu_mbox_read(mctrl, payload, VPU_MTYPE_F2H_NORMAL_RESPONSE, data);
		if (ret) {
			vpu_err("vpu_mbox_read is fail(%d)\n", ret);
			break;
		}

		reply = (struct vpu_mailbox_reply *)payload;
		if (reply->cid == VPU_DUM_SIGNATURE)
			continue;

		WORK_FR_FREE(work_list, &work);
		if (work) {
			work->id = reply->cid;
			work->message = VPU_MTYPE_F2H_NORMAL_RESPONSE;
			work->param0 = reply->cmd_ptr_ofs16;
			work->param1 = reply->err_type;
			WORK_TO_REPLY(work_list, work);

			if (!work_pending(work_queue))
				schedule_work(work_queue);
		} else {
			vpu_err("free work is empty\n");
		}
	}

	mbox = &mctrl->stack->f2h[VPU_MTYPE_F2H_LOW_RESPONSE];
	wmsg_idx = mbox->wmsg_idx;
	while (wmsg_idx != mbox->rmsg_idx) {
		struct vpu_mailbox_reply *reply;

		ret = vpu_mbox_read(mctrl, payload, VPU_MTYPE_F2H_LOW_RESPONSE, data);
		if (ret) {
			vpu_err("vpu_mbox_read is fail(%d)\n", ret);
			break;
		}

		reply = (struct vpu_mailbox_reply *)payload;
		if (reply->cid == VPU_DUM_SIGNATURE)
			continue;

		WORK_FR_FREE(work_list, &work);
		if (work) {
			work->id = reply->cid;
			work->message = VPU_MTYPE_F2H_LOW_RESPONSE;
			work->param0 = reply->cmd_ptr_ofs16;
			work->param1 = reply->err_type;
			WORK_TO_REPLY(work_list, work);

			if (!work_pending(work_queue))
				schedule_work(work_queue);
		} else {
			vpu_err("free work is empty\n");
		}
	}

	mbox = &mctrl->stack->f2h[VPU_MTYPE_F2H_SIZE];
	wmsg_idx = mbox->wmsg_idx;
	while (wmsg_idx != mbox->rmsg_idx) {
		struct vpu_mailbox_f2h_size *reply;

		ret = vpu_mbox_read(mctrl, payload, VPU_MTYPE_F2H_SIZE, data);
		if (ret) {
			vpu_err("vpu_mbox_read is fail(%d)\n", ret);
			break;
		}

		WORK_FR_FREE(work_list, &work);
		if (work) {
			reply = (struct vpu_mailbox_f2h_size *)payload;
			work->message = VPU_MTYPE_F2H_SIZE;
			work->param0 = reply->task_id;
			work->param1 = reply->base_size16;
			work->param2 = reply->each_slot_size16;
			WORK_TO_REPLY(work_list, work);

			if (!work_pending(work_queue))
				schedule_work(work_queue);
		} else {
			vpu_err("free work is empty\n");
		}
	}

	mbox = &mctrl->stack->f2h[VPU_MTYPE_F2H_REPORT];
	wmsg_idx = mbox->wmsg_idx;
	while (wmsg_idx != mbox->rmsg_idx) {
		struct vpu_mailbox_f2h_report *reply;

		ret = vpu_mbox_read(mctrl, payload, VPU_MTYPE_F2H_REPORT, data);
		if (ret) {
			vpu_err("vpu_mbox_read is fail(%d)\n", ret);
			break;
		}

		WORK_FR_FREE(work_list, &work);
		if (work) {
			reply = (struct vpu_mailbox_f2h_report *)payload;

			if (reply->data_size > VPU_WORK_MAX_DATA) {
				vpu_err("report data size is invalid(%d)\n", reply->data_size);
				BUG();
			}

			work->id = (reply->cid_msb << 16) | reply->cid_lsb;
			work->message = VPU_MTYPE_F2H_REPORT;
			work->param0 = reply->task_id;
			work->param1 = reply->vertex_idx;
			work->param2 = reply->data_size;
			memcpy(work->data, reply->data, reply->data_size);
			WORK_TO_REPLY(work_list, work);

			if (!work_pending(work_queue))
				schedule_work(work_queue);
		} else {
			vpu_err("free work is empty\n");
		}
	}

	return IRQ_HANDLED;
}

static irqreturn_t interface_isr0(int irq, void *data)
{
	struct vpu_interface *interface = (struct vpu_interface *)data;

	writel(0, interface->regs + 0xC);
	interface_isr(irq, data);

	return IRQ_HANDLED;
}

static irqreturn_t interface_isr1(int irq, void *data)
{
	struct vpu_interface *interface = (struct vpu_interface *)data;
	u32 val;

	val = readl(interface->regs + 0x1B4);
	if (val & 0x2000) {
		writel(0x2000, interface->regs + 0x1B4);
		interface_isr(irq, data);
	}

	return IRQ_HANDLED;
}

static void vpu_wq_func(struct work_struct *data)
{
	struct vpu_interface *interface;
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *iframe;
	struct vpu_work_list *work_list;
	struct vpu_work *work;
	u32 graph_id;
	ulong flags;

	interface = container_of(data, struct vpu_interface, work_queue);
	iframemgr = &interface->framemgr;
	work_list = &interface->work_list;

	WORK_FR_REPLY(work_list, &work);
	while (work) {
		switch (work->message) {
		case VPU_MTYPE_F2H_NORMAL_RESPONSE:
		case VPU_MTYPE_F2H_LOW_RESPONSE:
			if (work->id >= VPU_MAX_FRAME) {
				vpu_err("work id is invalid(%d)\n", work->id);
				break;
			}

			iframe = &iframemgr->frame[work->id];

			/* the response of process should be skipped */
			if (iframe->message == VPU_FRAME_PROCESS) {
				if (work->param1) {
					vpu_err("frame(%d, %ld, %d), work(%d, %d, %d)\n",
						iframe->index, iframe->param1, iframe->state,
						work->id, work->param0, work->param1);
					vpu_interface_print(interface);
					BUG();
				}
				break;
			}

			if (iframe->state != VPU_FRAME_STATE_PROCESS) {
				vpu_err("frame(%d, %d, %ld) state is invalid(%d), work(%d, %d, %d)\n",
					iframe->message, iframe->index, iframe->param1, iframe->state,
					work->id, work->param0, work->param1);
				vpu_interface_print(interface);
				BUG();
			}

			graph_id = iframe->param1;
			interface->reply[graph_id] = *work;
			__set_reply(interface, graph_id);
			break;
		case VPU_MTYPE_F2H_REPORT:
			if (work->id >= VPU_MAX_FRAME) {
				vpu_err("work id is invalid(%d)\n", work->id);
				break;
			}

			iframe = &iframemgr->frame[work->id];
			if (iframe->state != VPU_FRAME_STATE_PROCESS) {
				vpu_err("frame(%d, %d, %ld) state is invalid(%d), work(%d, %d, %d)\n",
					iframe->message, iframe->index, iframe->param1, iframe->state,
					work->id, work->param0, work->param1);
				vpu_interface_print(interface);
				BUG();
			}

			if (test_bit(VPU_FRAME_FLAG_IOCPY, &iframe->flags)) {
				/* HACK : iocpy update only first roi output buffer */
				u32 i, instance;

				for (i = 0; i < iframe->otcl->count; ++i) {
					instance = iframe->otcl->containers[i].target & VS4L_TARGET_PU;
					if ((instance >= VPU_PU_ROIS0) && (instance <= VPU_PU_ROIS1)) {
						memcpy(iframe->otcl->containers[i].buffers[0].kvaddr,
							work->data, work->param2);
					}
				}
			}

			framemgr_e_barrier_irqs(iframemgr, 0, flags);
			vpu_frame_trans_pro_to_com(iframemgr, iframe);
			framemgr_x_barrier_irqr(iframemgr, 0, flags);

			iframe->param2 = 0;
			iframe->param3 = 0;
			vpu_graphmgr_queue(interface->cookie, iframe);
			interface->done_cnt++;
			break;
		case VPU_MTYPE_F2H_SYS_ERROR:
			vpu_info("notify - syserr interrupt\n");
			vpu_info("error type : %d\n", work->param0);
			vpu_info("info1 : %d\n", work->param1);
			vpu_info("info2 : %d\n", work->param2);
			break;
		case VPU_MTYPE_F2H_SIZE:
			vpu_info("notify - size(%d %d %d)\n", work->param0, work->param1, work->param2);
			break;
		default:
			vpu_err("unresolved message(%d) have arrived\n", work->message);
			break;
		}

		WORK_TO_FREE(work_list, work);
		WORK_FR_REPLY(work_list, &work);
	}
}

int vpu_interface_probe(struct vpu_interface *interface,
	struct device *dev,
	void __iomem *code,
	resource_size_t code_size,
	void __iomem *regs,
	resource_size_t regs_size,
	u32 irq0, u32 irq1)
{
	int ret = 0;
	struct vpu_device *device;
	struct vpu_system *system;
	struct vpu_framemgr *framemgr;

	BUG_ON(!interface);
	BUG_ON(!dev);
	BUG_ON(!code);
	BUG_ON(!regs);

	system = container_of(interface, struct vpu_system, interface);
	device = container_of(system, struct vpu_device, system);

	init_process_barrier(interface);
	init_waitqueue_head(&interface->reply_queue);

	interface->code = code;
	interface->code_size = code_size;
	interface->regs = regs;
	interface->regs_size = regs_size;
	interface->cookie = (void *)&device->graphmgr;
	clear_bit(VPU_ITF_STATE_OPEN, &interface->state);
	clear_bit(VPU_ITF_STATE_START, &interface->state);
	interface->private_data = kmalloc(sizeof(struct vpu_mailbox_ctrl), GFP_KERNEL);
	if (!interface->private_data) {
		probe_err("kmalloc is fail\n");
		ret = -ENOMEM;
		goto p_err;
	}

	ret = devm_request_irq(dev, irq0, interface_isr0, 0, dev_name(dev), interface);
	if (ret) {
		probe_err("devm_request_irq(0) is fail(%d)\n", ret);
		goto p_err;
	}

	ret = devm_request_irq(dev, irq1, interface_isr1, 0, dev_name(dev), interface);
	if (ret) {
		probe_err("devm_request_irq(1) is fail(%d)\n", ret);
		goto p_err;
	}

	framemgr = &interface->framemgr;
	framemgr->id = VPU_MAX_GRAPH;
	framemgr->sindex = 0;
	spin_lock_init(&framemgr->slock);

	ret = vpu_frame_init(framemgr, interface);
	if (ret) {
		probe_err("vpu_frame_init is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_slab_init(&interface->slab);
	if (ret) {
		probe_err("vpu_slab_init is fail(%d)\n", ret);
		goto p_err;
	}

	INIT_WORK(&interface->work_queue, vpu_wq_func);
	INIT_WORK_LIST(&interface->work_list, VPU_WORK_MAX_COUNT);

p_err:
	return ret;
}

int vpu_interface_open(struct vpu_interface *interface)
{
	int ret = 0;
	u32 i;

	BUG_ON(!interface);

	ret = vpu_graphmgr_itf_register(interface->cookie, interface);
	if (ret) {
		vpu_err("vpu_graphmgr_itf_register is fail(%d)\n", ret);
		goto p_err;
	}

	interface->process = NULL;
	for (i = 0; i < VPU_MAX_GRAPH; ++i) {
		interface->request[i] = NULL;
		interface->reply[i].valid = 0;
	}

	interface->done_cnt = 0;
	set_bit(VPU_ITF_STATE_OPEN, &interface->state);
	clear_bit(VPU_ITF_STATE_ENUM, &interface->state);
	clear_bit(VPU_ITF_STATE_START, &interface->state);

p_err:
	return ret;
}

int vpu_interface_close(struct vpu_interface *interface)
{
	int ret = 0;

	BUG_ON(!interface);

	ret = __vpu_interface_cleanup(interface);
	if (ret)
		vpu_err("__vpu_interface_cleanup is fail(%d)\n", ret);

	ret = vpu_graphmgr_itf_unregister(interface->cookie, interface);
	if (ret)
		vpu_err("vpu_graphmgr_itf_unregister is fail(%d)\n", ret);

	clear_bit(VPU_ITF_STATE_OPEN, &interface->state);

	return ret;
}

int vpu_interface_start(struct vpu_interface *interface)
{
	int ret = 0;

	set_bit(VPU_ITF_STATE_START, &interface->state);

	return ret;
}

int vpu_interface_stop(struct vpu_interface *interface)
{
	int errcnt = 0;
	struct vpu_framemgr *iframemgr;
	u32 retry;

	iframemgr = &interface->framemgr;

	retry = VPU_STOP_WAIT_COUNT;
	while (--retry && iframemgr->req_cnt) {
		vpu_warn("waiting %d request completion...(%d)\n", iframemgr->req_cnt, retry);
		msleep(10);
	}

	if (!retry) {
		vpu_err("request completion is fail\n");
		vpu_interface_print(interface);
		errcnt++;
	}

	retry = VPU_STOP_WAIT_COUNT;
	while (--retry && iframemgr->pro_cnt) {
		vpu_warn("waiting %d process completion...(%d)\n", iframemgr->pro_cnt, retry);
		msleep(10);
	}

	if (!retry) {
		vpu_err("process completion is fail\n");
		vpu_interface_print(interface);
		errcnt++;
	}

	retry = VPU_STOP_WAIT_COUNT;
	while (--retry && iframemgr->com_cnt) {
		vpu_warn("waiting %d complete completion...(%d)\n", iframemgr->com_cnt, retry);
		msleep(10);
	}

	if (!retry) {
		vpu_err("complete completion is fail\n");
		vpu_interface_print(interface);
		errcnt++;
	}

	clear_bit(VPU_ITF_STATE_START, &interface->state);

	return 0;
}

int vpu_hw_enum(struct vpu_interface *interface)
{
	int ret = 0;
	struct vpu_mailbox_ctrl *mctrl;
	struct vpu_mailbox_hdr *hdr;
	void __iomem *code;
	resource_size_t	code_size;
	u32 try_count;

	BUG_ON(!interface);

	/* check mailbox size */
	if (sizeof(struct vpu_mailbox_hdr) != sizeof(union VPUM_FixAdrHostIntrUn)) {
		vpu_err("mailbox hdr is NOT same(%ld, %ld)\n", sizeof(struct vpu_mailbox_hdr),
			sizeof(union VPUM_FixAdrHostIntrUn));
		BUG();
	}

	if (sizeof(struct vpu_mailbox_hdr) != (sizeof(u16) * VPU_MAILBOX_BASEOFFSET)) {
		vpu_err("base offset is NOT same(%ld, %ld)\n", sizeof(struct vpu_mailbox_hdr),
			sizeof(u16) * VPU_MAILBOX_BASEOFFSET);
		BUG();
	}

	if (sizeof(struct vpu_mailbox_stack) != sizeof(struct VPUM_DbgAndAllMboxCont)) {
		vpu_err("mailbox stack is NOT same(%ld, %ld)\n", sizeof(struct vpu_mailbox_stack),
			sizeof(struct VPUM_DbgAndAllMboxCont));
		BUG();
	}

	if (sizeof(struct vpu_mailbox_h2f) != sizeof(struct VPUM_HostMboxCont)) {
		vpu_err("mailbox h2f is NOT same(%ld, %ld)\n", sizeof(struct vpu_mailbox_h2f),
			sizeof(struct VPUM_HostMboxCont));
		BUG();
	}

	if (sizeof(struct vpu_mailbox_f2h) != sizeof(struct VPUM_CoreMboxCont)) {
		vpu_err("mailbox h2f is NOT same(%ld, %ld)\n", sizeof(struct vpu_mailbox_f2h),
			sizeof(struct VPUM_CoreMboxCont));
		BUG();
	}

	code = interface->code;
	code_size = interface->code_size;
	mctrl = interface->private_data;
	hdr = vpu_mbox_g_hdr(code, code_size);

	try_count = 1000;
	while (try_count && (IOR16(hdr->signature1) != VPU_MAILBOX_SIGNATURE1)) {
		vpu_info("waiting sig1(%X)...%d\n", IOR16(hdr->signature1), try_count);
		try_count--;
		msleep(1);
	}

	if (!try_count) {
		vpu_err("try count is 0(s1)\n");
		ret = -EINVAL;
		goto p_err;
	}

	IOW16(hdr->signature1, 0x0);

	try_count = 1000;
	while (try_count && (IOR16(hdr->signature2) != VPU_MAILBOX_SIGNATURE2)) {
		vpu_info("waiting sig2(%X)...%d\n", IOR16(hdr->signature2), try_count);
		try_count--;
		msleep(1);
	}

	if (!try_count) {
		vpu_err("try count is 0(s2)\n");
		ret = -EINVAL;
		goto p_err;
	}

	IOW16(hdr->signature2, 0x0);

	mctrl->hdr = hdr;
	mctrl->stack = vpu_mbox_g_stack(hdr);

	vpu_info("<mailbox info>\n");
	vpu_info("stack_ofs16    : %d\n", hdr->stack_ofs16);
	vpu_info("hw_version     : %d\n", hdr->hw_version);
	vpu_info("fw_version     : %d\n", hdr->fw_version);
	vpu_info("mbox_version   : %d\n", hdr->mbox_version);
	vpu_info("cmd_version    : %d\n", hdr->cmd_version);
	vpu_info("heap_size16    : %d\n", hdr->heap_size16);
	vpu_info("mbox_totsize16 : %d\n", hdr->mbox_totsize16);

#ifdef DBG_MAILBOX_INIT
	vpu_info("h2f(0) info\n");
	vpu_info("data_ofs16     : %d\n", mctrl->stack->h2f[0].data_ofs16);
	vpu_info("data_size16    : %d\n", mctrl->stack->h2f[0].data_size16);
	vpu_info("wptr_ofs16     : %d\n", mctrl->stack->h2f[0].wptr_ofs16);
	vpu_info("rptr_ofs16     : %d\n", mctrl->stack->h2f[0].rptr_ofs16);
	vpu_info("resonse        : %d\n", mctrl->stack->h2f[0].response_on_error_only);
#endif

	set_bit(VPU_ITF_STATE_ENUM, &interface->state);

p_err:
	vpu_info("%s():%d\n", __func__, ret);
	return ret;
}

int vpu_hw_init(struct vpu_interface *interface, struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpu_mailbox_ctrl *mctrl;
	struct vpul_core_init_command_params core_init;
	void *payload;
	size_t alloc_size, pl_size;
	ulong flag;

	mctrl = interface->private_data;
	iframemgr = &interface->framemgr;
	payload = NULL;
	pl_size = 0;

	core_init.time.isp_clock_mhz = 533;
	core_init.time.hw_timeout_msec = 0;
	core_init.time.process_timeout_msec = 0;
	core_init.time.task_timeout_msec = 0;

	core_init.mem.max_tasks = VPU_MAX_GRAPH;
	core_init.mem.max_proc_vertices = 20;
	core_init.mem.max_control_vertices = 10;
	core_init.mem.min_alloc_size = 0;
	core_init.mem.max_valid_unused_size = 0;

	core_init.mbox.host_lowpr.size_in_u16 = 2000;
	core_init.mbox.host_lowpr.send_response_on_error_only = 0;
	core_init.mbox.host_normal.size_in_u16 = 0;
	core_init.mbox.host_normal.send_response_on_error_only = 0;

	core_init.mbox.core_lowpr.n_messages = 0;
	core_init.mbox.core_lowpr.interrupt_type = 2;
	core_init.mbox.core_debug.n_messages = 0;
	core_init.mbox.core_debug.interrupt_type = 0;
	core_init.mbox.core_normal.n_messages = 0;
	core_init.mbox.core_normal.interrupt_type = 2;
	core_init.mbox.core_reports.n_messages = 0;
	core_init.mbox.core_reports.interrupt_type = 1;
	core_init.mbox.core_syserr.n_messages = 0;
	core_init.mbox.core_syserr.interrupt_type = 2;
	core_init.mbox.core_create_sizes.n_messages = 0;
	core_init.mbox.core_create_sizes.interrupt_type = 0;
	core_init.mbox.update_sizes = 1;
	core_init.mbox.report_data_size = VPU_WORK_MAX_DATA / 2; /* host report data size : 24 bytes */
	core_init.mbox.verify_inc_cmd_id = 0;

	ret = vpu_translator_init_core(NULL, &pl_size, NULL);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_init_core1 is fail(%d)\n", ret);
		goto p_err;
	}

	alloc_size = pl_size;
	ret = vpu_slab_alloc(&interface->slab, &payload, alloc_size);
	if (ret) {
		vpu_err("vpu_slab_alloc is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_translator_init_core(&core_init, &pl_size, payload);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_init_core2 is  fail(%d)\n", ret);
		goto p_err;
	}

	iframe->param0 = (ulong)payload;
	iframe->param2 = pl_size;
	iframe->param3 = VPU_MTYPE_H2F_NORMAL;

	ret = __vpu_set_cmd(interface, iframe);
	if (ret) {
		vpu_err("__vpu_set_cmd is fail(%d)\n", ret);
		goto p_err;
	}

#ifdef DBG_MAILBOX_INIT
	vpu_info("h2f(0) info\n");
	vpu_info("data_ofs16     : %d\n", mctrl->stack->h2f[0].data_ofs16);
	vpu_info("data_size16    : %d\n", mctrl->stack->h2f[0].data_size16);
	vpu_info("wptr_ofs16	 : %d\n", mctrl->stack->h2f[0].wptr_ofs16);
	vpu_info("rptr_ofs16	 : %d\n", mctrl->stack->h2f[0].rptr_ofs16);
	vpu_info("resonse        : %d\n", mctrl->stack->h2f[0].response_on_error_only);
	vpu_info("h2f(1) info\n");
	vpu_info("data_ofs16     : %d\n", mctrl->stack->h2f[1].data_ofs16);
	vpu_info("data_size16    : %d\n", mctrl->stack->h2f[1].data_size16);
	vpu_info("wptr_ofs16	 : %d\n", mctrl->stack->h2f[1].wptr_ofs16);
	vpu_info("rptr_ofs16	 : %d\n", mctrl->stack->h2f[1].rptr_ofs16);
	vpu_info("resonse        : %d\n", mctrl->stack->h2f[1].response_on_error_only);
	vpu_info("f2h(0) info\n");
	vpu_info("data_ofs16     : %d\n", mctrl->stack->f2h[0].data_ofs16);
	vpu_info("msg_size16     : %d\n", mctrl->stack->f2h[0].msg_size16);
	vpu_info("emsg_idx	 : %d\n", mctrl->stack->f2h[0].emsg_idx);
	vpu_info("wmsg_idx	 : %d\n", mctrl->stack->f2h[0].wmsg_idx);
	vpu_info("rmsg_idx	 : %d\n", mctrl->stack->f2h[0].rmsg_idx);
	vpu_info("full           : %d\n", mctrl->stack->f2h[0].full);
	vpu_info("intr_type      : %d\n", mctrl->stack->f2h[0].intr_type);
	vpu_info("f2h(1) info\n");
	vpu_info("data_ofs16     : %d\n", mctrl->stack->f2h[1].data_ofs16);
	vpu_info("msg_size16     : %d\n", mctrl->stack->f2h[1].msg_size16);
	vpu_info("emsg_idx	 : %d\n", mctrl->stack->f2h[1].emsg_idx);
	vpu_info("wmsg_idx	 : %d\n", mctrl->stack->f2h[1].wmsg_idx);
	vpu_info("rmsg_idx	 : %d\n", mctrl->stack->f2h[1].rmsg_idx);
	vpu_info("full           : %d\n", mctrl->stack->f2h[1].full);
	vpu_info("intr_type      : %d\n", mctrl->stack->f2h[1].intr_type);
	vpu_info("f2h(2) info\n");
	vpu_info("data_ofs16     : %d\n", mctrl->stack->f2h[2].data_ofs16);
	vpu_info("msg_size16     : %d\n", mctrl->stack->f2h[2].msg_size16);
	vpu_info("emsg_idx	 : %d\n", mctrl->stack->f2h[2].emsg_idx);
	vpu_info("wmsg_idx	 : %d\n", mctrl->stack->f2h[2].wmsg_idx);
	vpu_info("rmsg_idx	 : %d\n", mctrl->stack->f2h[2].rmsg_idx);
	vpu_info("full           : %d\n", mctrl->stack->f2h[2].full);
	vpu_info("intr_type      : %d\n", mctrl->stack->f2h[2].intr_type);
	vpu_info("f2h(3) info\n");
	vpu_info("data_ofs16     : %d\n", mctrl->stack->f2h[3].data_ofs16);
	vpu_info("msg_size16     : %d\n", mctrl->stack->f2h[3].msg_size16);
	vpu_info("emsg_idx	 : %d\n", mctrl->stack->f2h[3].emsg_idx);
	vpu_info("wmsg_idx	 : %d\n", mctrl->stack->f2h[3].wmsg_idx);
	vpu_info("rmsg_idx 	 : %d\n", mctrl->stack->f2h[3].rmsg_idx);
	vpu_info("full           : %d\n", mctrl->stack->f2h[3].full);
	vpu_info("intr_type      : %d\n", mctrl->stack->f2h[3].intr_type);
	vpu_info("f2h(4) info\n");
	vpu_info("data_ofs16     : %d\n", mctrl->stack->f2h[4].data_ofs16);
	vpu_info("msg_size16     : %d\n", mctrl->stack->f2h[4].msg_size16);
	vpu_info("emsg_idx	 : %d\n", mctrl->stack->f2h[4].emsg_idx);
	vpu_info("wmsg_idx	 : %d\n", mctrl->stack->f2h[4].wmsg_idx);
	vpu_info("rmsg_idx	 : %d\n", mctrl->stack->f2h[4].rmsg_idx);
	vpu_info("full           : %d\n", mctrl->stack->f2h[4].full);
	vpu_info("intr_type      : %d\n", mctrl->stack->f2h[4].intr_type);
#endif

p_err:
	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_any_to_fre(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	vpu_slab_free(&interface->slab, payload, alloc_size);
	return ret;
}

int vpu_hw_create(struct vpu_interface *interface, struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpul_task *task;
	struct vpu_mark_for_invoke_ext_mem_vec_ds *mem_mark;
	void *payload;
	size_t alloc_size, pl_size;
	ulong flag;

	BUG_ON(!interface);
	BUG_ON(!iframe);

	iframemgr = &interface->framemgr;
	task = (struct vpul_task *)iframe->param0;
	mem_mark = (struct vpu_mark_for_invoke_ext_mem_vec_ds *)iframe->param2;
	payload = NULL;
	pl_size = 0;

	ret = vpu_translator_task_create_w_mem_mark(task, mem_mark, &pl_size, NULL);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_create1 fail(%d)\n", ret);
		goto p_err;
	}

	alloc_size = pl_size;
	ret = vpu_slab_alloc(&interface->slab, &payload, alloc_size);
	if (ret) {
		vpu_err("vpu_slab_alloc is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_translator_task_create_w_mem_mark(task, mem_mark, &pl_size, payload);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_create2 fail(%d)\n", ret);
		goto p_err;
	}

	iframe->param0 = (ulong)payload;
	iframe->param2 = pl_size;
	iframe->param3 = VPU_MTYPE_H2F_LOW;

#ifdef DBG_TIMEMEASURE
	vpu_get_timestamp(&iframe->time[VPU_TMP_PROCESS]);
#endif

	ret = __vpu_set_cmd(interface, iframe);
	if (ret) {
		vpu_err("__vpu_set_cmd is fail(%d)\n", ret);
		goto p_err;
	}


#ifdef DBG_TIMEMEASURE
	vpu_get_timestamp(&iframe->time[VPU_TMP_DONE]);
#endif

p_err:
	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_any_to_fre(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	vpu_slab_free(&interface->slab, payload, alloc_size);
	return ret;
}

int vpu_hw_destroy(struct vpu_interface *interface, struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpul_task *task;
	void *payload;
	size_t alloc_size, pl_size;
	ulong flag;

	BUG_ON(!interface);
	BUG_ON(!iframe);

	iframemgr = &interface->framemgr;
	task = (struct vpul_task *)iframe->param0;
	payload = NULL;
	pl_size = 0;

	ret = vpu_translator_task_destroy(task, &pl_size, NULL);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_destroy1 is fail(%d)\n", ret);
		goto p_err;
	}

	alloc_size = pl_size;
	ret = vpu_slab_alloc(&interface->slab, &payload, alloc_size);
	if (ret) {
		vpu_err("vpu_slab_alloc is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_translator_task_destroy(task, &pl_size, payload);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_destroy2 is fail(%d)\n", ret);
		goto p_err;
	}

	iframe->param0 = (ulong)payload;
	iframe->param2 = pl_size;
	iframe->param3 = VPU_MTYPE_H2F_NORMAL;

#ifdef DBG_TIMEMEASURE
	vpu_get_timestamp(&iframe->time[VPU_TMP_PROCESS]);
#endif

	ret = __vpu_set_cmd(interface, iframe);
	if (ret) {
		vpu_err("__vpu_set_cmd is fail(%d)\n", ret);
		goto p_err;
	}

#ifdef DBG_TIMEMEASURE
	vpu_get_timestamp(&iframe->time[VPU_TMP_DONE]);
#endif

p_err:
	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_any_to_fre(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	vpu_slab_free(&interface->slab, payload, alloc_size);
	return ret;
}

int vpu_hw_allocate(struct vpu_interface *interface, struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpul_task *task;
	void *payload;
	size_t alloc_size, pl_size;
	ulong flag;

	BUG_ON(!interface);
	BUG_ON(!iframe);

	iframemgr = &interface->framemgr;
	task = (struct vpul_task *)iframe->param0;
	payload = NULL;
	pl_size = 0;

	ret = vpu_translator_task_allocate(task, 1, &pl_size, NULL);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_allocate1 is fail(%d)\n", ret);
		goto p_err;
	}

	alloc_size = pl_size;
	ret = vpu_slab_alloc(&interface->slab, &payload, alloc_size);
	if (ret) {
		vpu_err("vpu_slab_alloc is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_translator_task_allocate(task, 1, &pl_size, payload);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_allocate2 is fail(%d)\n", ret);
		goto p_err;
	}

	iframe->param0 = (ulong)payload;
	iframe->param2 = pl_size;
	iframe->param3 = VPU_MTYPE_H2F_LOW;

#ifdef DBG_TIMEMEASURE
	vpu_get_timestamp(&iframe->time[VPU_TMP_PROCESS]);
#endif

	ret = __vpu_set_cmd(interface, iframe);
	if (ret) {
		vpu_err("__vpu_set_cmd is fail(%d)\n", ret);
		goto p_err;
	}

#ifdef DBG_TIMEMEASURE
	vpu_get_timestamp(&iframe->time[VPU_TMP_DONE]);
#endif

p_err:
	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_any_to_fre(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	vpu_slab_free(&interface->slab, payload, alloc_size);
	return ret;
}

int vpu_hw_process(struct vpu_interface *interface, struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpul_task *task;
	struct vpu_invoke_external_mem_vector_ds *mvectors;
	union vpul_pu_parameters *update_array;
	void *payload;
	size_t alloc_size, pl_size;
	ulong flag;

	BUG_ON(!interface);
	BUG_ON(!iframe);

	iframemgr = &interface->framemgr;
	task = (struct vpul_task *)iframe->param0;
	mvectors = (struct vpu_invoke_external_mem_vector_ds *)iframe->param2;
	update_array = (union vpul_pu_parameters *)iframe->param3;
	payload = NULL;
	pl_size = 0;

	ret = vpu_translator_task_invoke(task, iframe->index, update_array, mvectors, &pl_size, NULL);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_invoke1 is fail(%d)\n", ret);
		goto p_err;
	}

	alloc_size = pl_size;
	ret = vpu_slab_alloc(&interface->slab, &payload, alloc_size);
	if (ret) {
		vpu_err("vpu_slab_alloc is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_translator_task_invoke(task, iframe->index, update_array, mvectors, &pl_size, payload);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_invoke2 is fail(%d)\n", ret);
		goto p_err;
	}

	iframe->param0 = (ulong)payload;
	iframe->param2 = pl_size;
	iframe->param3 = VPU_MTYPE_H2F_NORMAL;

	ret = __vpu_set_cmd_nblk(interface, iframe);
	if (ret) {
		vpu_err("__vpu_set_cmd_nblk is fail(%d)\n", ret);
		goto p_err;
	}

	vpu_slab_free(&interface->slab, payload, pl_size);
	return 0;

p_err:
	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_any_to_fre(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	vpu_slab_free(&interface->slab, payload, alloc_size);
	return ret;
}
