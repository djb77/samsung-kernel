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

/* dummy functions */
int vpu_mbox_ready(struct vpu_mailbox_ctrl *mctrl,
	size_t size, u32 type)
{
	return 0;
}

int vpu_mbox_write(struct vpu_mailbox_ctrl *mctrl,
	void *payload, size_t size, u32 type, u32 cid)
{
	struct vpu_mailbox_h2f_hdr *cmd_hdr;
	u16 size16;

	size16 = size >> 1;
	/* fill message header and copy message to Mail-box */
	cmd_hdr = (struct vpu_mailbox_h2f_hdr *)payload;
	cmd_hdr->signature = VPU_CMD_SIGNATURE;
	cmd_hdr->cid = cid;
	cmd_hdr->size16 = (__u16)size16;
#ifdef DBG_MAILBOX_CORE
	vpu_info("[MBOX] command(%d)\n", cmd_hdr->size16 << 1);
#endif

	return 0;
}

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
	int remain_time;
	u32 cmd;

	BUG_ON(!interface);
	BUG_ON(graph_id >= VPU_MAX_GRAPH);
	BUG_ON(!interface->request[graph_id]);

	cmd = interface->request[graph_id]->message;

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
}

static int __vpu_set_cmd(struct vpu_interface *interface,
	struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpu_mailbox_ctrl *mctrl;
	void *payload;
	u32 cmd, graph_id, size, type, cid;
	ulong flag;

	BUG_ON(!interface);
	BUG_ON(!iframe);
	BUG_ON(!iframe->lock);
	BUG_ON(iframe->param1 >= VPU_MAX_GRAPH);

	iframemgr = &interface->framemgr;
	mctrl = interface->private_data;
	cmd = iframe->message;
	payload = (void *)iframe->param0;
	graph_id = iframe->param1;
	size = iframe->param2;
	type = iframe->param3;
	cid = iframe->index;

	enter_request_barrier(iframe);
	interface->request[graph_id] = iframe;

	enter_process_barrier(interface);
	interface->process = iframe;

	ret = vpu_mbox_ready(mctrl, size, type);
	if (ret) {
		interface->process = NULL;
		exit_process_barrier(interface);
		interface->request[graph_id] = NULL;
		exit_request_barrier(iframe);
		vpu_err("vpu_mbox_ready is fail(%d)", ret);
		goto p_err;
	}

	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_req_to_pro(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	ret = vpu_mbox_write(mctrl, payload, size, type, cid);
	if (ret) {
		interface->process = NULL;
		exit_process_barrier(interface);
		interface->request[graph_id] = NULL;
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

	ret = __wait_reply(interface, graph_id);
	if (ret) {
		interface->request[graph_id] = NULL;
		exit_request_barrier(iframe);
		vpu_err("%d command is timeout", cmd);
		ret = -ETIME;
		goto p_err;
	}

	if (graph_id >= VPU_MAX_GRAPH) {
		interface->request[graph_id] = NULL;
		exit_request_barrier(iframe);
		vpu_err("graph id is invalid(%d)\n", graph_id);
		ret = -EINVAL;
		goto p_err;
	}

	if (interface->reply[graph_id].param1) {
		interface->request[graph_id] = NULL;
		exit_request_barrier(iframe);
		vpu_err("%d command is error(%d)\n", cmd, interface->reply[graph_id].param1);
		ret = interface->reply[graph_id].param1;
		goto p_err;
	}

	__clr_reply(interface, graph_id);

	interface->request[graph_id] = NULL;
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
	u32 cmd, size, type, cid;
	ulong flag;

	BUG_ON(!interface);
	BUG_ON(!iframe);

	iframemgr = &interface->framemgr;
	mctrl = interface->private_data;
	cmd = iframe->message;
	payload = (void *)iframe->param0;
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

	ret = vpu_mbox_write(mctrl, payload, size, type, cid);
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

	framemgr = &interface->framemgr;

	if (framemgr->req_cnt) {
		vpu_err("[ITF] request count is NOT zero(%d)\n", framemgr->req_cnt);
		ret = -EINVAL;
		goto p_err;
	}

	if (framemgr->pro_cnt) {
		vpu_err("[ITF] process count is NOT zero(%d)\n", framemgr->pro_cnt);
		ret = -EINVAL;
		goto p_err;
	}

	if (framemgr->com_cnt) {
		vpu_err("[ITF] complete count is NOT zero(%d)\n", framemgr->com_cnt);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

void vpu_interface_print(struct vpu_interface *interface)
{
	DLOG_INIT();
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *iframe, *itemp;

	BUG_ON(!interface);

	iframemgr = &interface->framemgr;

	DLOG("REQUEST LIST :");
	list_for_each_entry_safe(iframe, itemp, &iframemgr->req_list, list) {
		DLOG(" %d(%d, %d)", iframe->param1, iframe->index, iframe->message);
	}
	vpu_info("%s\n", DLOG_OUT());

	DLOG("PROCESS LIST :");
	list_for_each_entry_safe(iframe, itemp, &iframemgr->pro_list, list) {
		DLOG(" %d(%d, %d)", iframe->param1, iframe->index, iframe->message);
	}
	vpu_info("%s\n", DLOG_OUT());

	DLOG("COMPLETE LIST :");
	list_for_each_entry_safe(iframe, itemp, &iframemgr->com_list, list) {
		DLOG(" %d(%d, %d)", iframe->param1, iframe->index, iframe->message);
	}
	vpu_info("%s\n", DLOG_OUT());
}

static void interface_timer(unsigned long data)
{
	struct vpu_interface *interface = (struct vpu_interface *)data;
	struct work_struct *work_queue;
	struct vpu_work_list *work_list;
	struct vpu_work *work;
	struct vpu_framemgr *iframemgr;
	struct vpu_frame *iframe, *temp;
	u32 random;

	iframemgr = &interface->framemgr;
	work_queue = &interface->work_queue;
	work_list = &interface->work_list;

	if (!test_bit(VPU_ITF_STATE_START, &interface->state))
		return;

	list_for_each_entry_safe(iframe, temp, &iframemgr->pro_list, list) {
		/* if already get service */
		if (iframe->param3 == 0xFFFF)
			continue;

		switch (iframe->message) {
		case VPU_FRAME_INIT:
		case VPU_FRAME_CREATE:
		case VPU_FRAME_DESTROY:
		case VPU_FRAME_ALLOCATE:
			WORK_FR_FREE(work_list, &work);
			if (work) {
				iframe->param3 = 0xFFFF;
				work->id = iframe->index;
				work->message = VPU_MTYPE_F2H_NORMAL_RESPONSE;
				work->param0 = 0;
				work->param1 = 0;
				WORK_TO_REPLY(work_list, work);

				if (!work_pending(work_queue))
					schedule_work(work_queue);
			} else {
				vpu_err("free work is empty\n");
			}
			break;
		case VPU_FRAME_PROCESS:
			WORK_FR_FREE(work_list, &work);
			if (work) {
				iframe->param3 = 0xFFFF;
				work->id = iframe->index;
				work->message = VPU_MTYPE_F2H_REPORT;
				work->param0 = 0;
				work->param1 = 0;
				WORK_TO_REPLY(work_list, work);

				if (!work_pending(work_queue))
					schedule_work(work_queue);
			} else {
				vpu_err("free work is empty\n");
			}
			break;
		default:
			vpu_err("unresolved command(%d)\n", iframe->message);
			break;
		}
	}

	get_random_bytes(&random, sizeof(random));
	random = random % TURN_AROUND_TIME;
	mod_timer(&interface->timer, jiffies + random);
}

static irqreturn_t interface_isr0(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t interface_isr1(int irq, void *data)
{
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
			if (iframe->state != VPU_FRAME_STATE_PROCESS) {
				vpu_err("frame(%d, %d) state is invalid(%d)\n",
					iframe->message, iframe->index, iframe->state);
				vpu_interface_print(interface);
				BUG();
			}

			/* the response of process should be skipped */
			if (iframe->message == VPU_FRAME_PROCESS)
				break;

			if (iframe->message == VPU_FRAME_ALLOCATE) {
				framemgr_e_barrier_irqs(iframemgr, 0, flags);
				vpu_frame_trans_pro_to_com(iframemgr, iframe);
				framemgr_x_barrier_irqr(iframemgr, 0, flags);

				iframe->param2 = work->param1;
				iframe->param3 = 0;
				vpu_graphmgr_queue(interface->cookie, iframe);
				interface->done_cnt++;
			} else {
				graph_id = iframe->param1;
				interface->reply[graph_id] = *work;
				__set_reply(interface, graph_id);
			}
			break;
		case VPU_MTYPE_F2H_REPORT:
			if (work->id >= VPU_MAX_FRAME) {
				vpu_err("work id is invalid(%d)\n", work->id);
				break;
			}

			iframe = &iframemgr->frame[work->id];
			if (iframe->state != VPU_FRAME_STATE_PROCESS) {
				vpu_err("frame(%d, %d) state is invalid(%d)\n",
					iframe->message, iframe->index, iframe->state);
				vpu_interface_print(interface);
				BUG();
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
			vpu_info("notify - size interrupt\n");
			vpu_info("task id : %d\n", work->param0);
			vpu_info("base size : %d\n", work->param1);
			vpu_info("each size : %d\n", work->param2);
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
	INIT_WORK_LIST(&interface->work_list, MAX_WORK_COUNT);

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

	init_timer(&interface->timer);
	interface->timer.expires = jiffies + TURN_AROUND_TIME;
	interface->timer.data = (unsigned long)interface;
	interface->timer.function = interface_timer;
	add_timer(&interface->timer);

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
		errcnt++;
	}

	retry = VPU_STOP_WAIT_COUNT;
	while (--retry && iframemgr->pro_cnt) {
		vpu_warn("waiting %d process completion...(%d)\n", iframemgr->pro_cnt, retry);
		msleep(10);
	}

	if (!retry) {
		vpu_err("process completion is fail\n");
		errcnt++;
	}

	retry = VPU_STOP_WAIT_COUNT;
	while (--retry && iframemgr->com_cnt) {
		vpu_warn("waiting %d complete completion...(%d)\n", iframemgr->com_cnt, retry);
		msleep(10);
	}

	if (!retry) {
		vpu_err("complete completion is fail\n");
		errcnt++;
	}

	del_timer(&interface->timer);
	clear_bit(VPU_ITF_STATE_START, &interface->state);

	return errcnt;
}

int vpu_hw_enum(struct vpu_interface *interface)
{
	int ret = 0;

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

	set_bit(VPU_ITF_STATE_ENUM, &interface->state);

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
	size_t pl_size;
	ulong flag;

	mctrl = interface->private_data;
	iframemgr = &interface->framemgr;
	payload = NULL;
	pl_size = 0;

	core_init.time.isp_clock_mhz = 20;
	core_init.time.hw_timeout_msec = 0;
	core_init.time.process_timeout_msec = 0;
	core_init.time.task_timeout_msec = 0;

	core_init.mem.max_tasks = 1;
	core_init.mem.max_proc_vertices = 20;
	core_init.mem.max_control_vertices = 10;
	core_init.mem.min_alloc_size = 0;
	core_init.mem.max_valid_unused_size = 0;

	core_init.mbox.host_lowpr.size_in_u16 = 1000;
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

	ret = vpu_slab_alloc(&interface->slab, &payload, pl_size);
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

p_err:
	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_any_to_fre(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	vpu_slab_free(&interface->slab, payload, pl_size);
	return ret;
}

int vpu_hw_create(struct vpu_interface *interface, struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpul_task *task;
	void *payload;
	size_t pl_size;
	ulong flag;

	BUG_ON(!interface);
	BUG_ON(!iframe);

	iframemgr = &interface->framemgr;
	task = (struct vpul_task *)iframe->param0;
	payload = NULL;
	pl_size = 0;

	ret = vpu_translator_task_create(task, &pl_size, NULL);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_create1 fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_slab_alloc(&interface->slab, &payload, pl_size);
	if (ret) {
		vpu_err("vpu_slab_alloc is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_translator_task_create(task, &pl_size, payload);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_create2 fail(%d)\n", ret);
		goto p_err;
	}

	iframe->param0 = (ulong)payload;
	iframe->param2 = pl_size;
	iframe->param3 = VPU_MTYPE_H2F_LOW;

	ret = __vpu_set_cmd(interface, iframe);
	if (ret) {
		vpu_err("__vpu_set_cmd is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_any_to_fre(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	vpu_slab_free(&interface->slab, payload, pl_size);
	return ret;
}

int vpu_hw_destroy(struct vpu_interface *interface, struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpul_task *task;
	void *payload;
	size_t pl_size;
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

	ret = vpu_slab_alloc(&interface->slab, &payload, pl_size);
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
	iframe->param3 = VPU_MTYPE_H2F_LOW;

	ret = __vpu_set_cmd(interface, iframe);
	if (ret) {
		vpu_err("__vpu_set_cmd is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_any_to_fre(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	vpu_slab_free(&interface->slab, payload, pl_size);
	return ret;
}

int vpu_hw_allocate(struct vpu_interface *interface, struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_framemgr *iframemgr;
	struct vpul_task *task;
	void *payload;
	size_t pl_size;
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

	ret = vpu_slab_alloc(&interface->slab, &payload, pl_size);
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
	iframe->param3 = VPU_MTYPE_H2F_NORMAL;

	ret = __vpu_set_cmd_nblk(interface, iframe);
	if (ret) {
		vpu_err("__vpu_set_cmd is fail(%d)\n", ret);
		goto p_err;
	}

	vpu_slab_free(&interface->slab, payload, pl_size);
	return 0;

p_err:
	framemgr_e_barrier_irqs(iframemgr, 0, flag);
	vpu_frame_trans_any_to_fre(iframemgr, iframe);
	framemgr_x_barrier_irqr(iframemgr, 0, flag);

	vpu_slab_free(&interface->slab, payload, pl_size);
	return ret;
}

int vpu_hw_process(struct vpu_interface *interface, struct vpu_frame *iframe)
{
	int ret = 0;
	struct vpu_invoke_external_mem_vector_ds mvectors;
	struct vpu_framemgr *iframemgr;
	struct vpul_task *task;
	void *payload;
	size_t pl_size;
	ulong flag;
	u32 i;

	BUG_ON(!interface);
	BUG_ON(!iframe);

	iframemgr = &interface->framemgr;
	task = (struct vpul_task *)iframe->param0;
	payload = NULL;
	pl_size = 0;

	mvectors.num_of_buffers = task->n_external_mem_addresses;
	for (i = 0; i < mvectors.num_of_buffers; ++i)
		mvectors.addresses_vector[i] = task->external_mem_addr[i];

	ret = vpu_translator_task_invoke(task, iframe->index, NULL, &mvectors, &pl_size, NULL);
	if (ret != VPU_STATUS_SUCCESS) {
		vpu_err("vpu_translator_task_invoke1 is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_slab_alloc(&interface->slab, &payload, pl_size);
	if (ret) {
		vpu_err("vpu_slab_alloc is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_translator_task_invoke(task, iframe->index, NULL, &mvectors, &pl_size, payload);
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

	vpu_slab_free(&interface->slab, payload, pl_size);
	return ret;
}
