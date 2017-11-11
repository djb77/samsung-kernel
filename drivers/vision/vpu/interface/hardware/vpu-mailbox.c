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
#include <linux/kernel.h>

#include "vpu-config.h"
#include "vpu-io.h"
#include "vpu-mailbox.h"
#include "vpu-interface.h"

struct vpu_mailbox_hdr * vpu_mbox_g_hdr(void *code, u32 code_size)
{
	return (struct vpu_mailbox_hdr *)((char *)code + code_size - sizeof(struct vpu_mailbox_hdr));
}

struct vpu_mailbox_stack * vpu_mbox_g_stack(struct vpu_mailbox_hdr *hdr)
{
	return (struct vpu_mailbox_stack *)((char *)hdr + sizeof(struct vpu_mailbox_hdr)
		- VPU_MAILBOX_BASEOFFSET * 2 - (hdr->stack_ofs16 * 2));
}

inline u16 * vpu_h2f_g_mboxdata(struct vpu_mailbox_h2f *mbox)
{
	return ((u16 *)mbox) + mbox->data_ofs16;
}

inline u16 * vpu_f2h_g_mboxdata(struct vpu_mailbox_f2h *mbox)
{
	return ((u16 *)mbox) + mbox->data_ofs16;
}

static u16 __vpu_mbox_g_freesize(struct vpu_mailbox_h2f *mbox)
{
	u16 size16;
	u16 wptr_ofs16;
	u16 rptr_ofs16;

	wptr_ofs16 = mbox->wptr_ofs16;
	rptr_ofs16 = mbox->rptr_ofs16;

	size16 = wptr_ofs16 >= rptr_ofs16 ?
		max((u16)(mbox->data_size16 - wptr_ofs16), rptr_ofs16) :
		rptr_ofs16 - wptr_ofs16;

	return size16;
}

int vpu_mbox_ready(struct vpu_mailbox_ctrl *mctrl,
	size_t size, u32 type)
{
	u32 try_count;
	struct vpu_mailbox_h2f *mbox;
	u16 size16, free_size16;

	BUG_ON(!mctrl);
	BUG_ON(type >= VPU_MTYPE_H2F_CNT);
	BUG_ON(!IS_ALIGNED(size, 2));

	mbox = &mctrl->stack->h2f[type];
	size16 = size >> 1;
	try_count = 1000;

	free_size16 = __vpu_mbox_g_freesize(mbox);
	while (--try_count && (size16 > free_size16)) {
		vpu_warn("mbox(%d) is not ready(%d > %d)...(%d)\n", type,
			size16, free_size16, try_count);
		udelay(10);
		free_size16 = __vpu_mbox_g_freesize(mbox);
	}

	if (try_count)
		return 0;
	else
		return -EBUSY;
}

int vpu_mbox_write(struct vpu_mailbox_ctrl *mctrl,
	void *payload, size_t size, u32 type, u32 gid, u32 cmd, u32 cid)
{
	struct vpu_mailbox_h2f *mbox;
	struct vpu_mailbox_h2f_hdr *cmd_hdr;
	u16 size16;
	u16 wptr_ofs16;
	u16 rptr_ofs16;
	u16 *data, *wptr;

	BUG_ON(!mctrl);
	BUG_ON(!payload);
	BUG_ON(type >= VPU_MTYPE_H2F_CNT);
	BUG_ON(!IS_ALIGNED(size, 2));

	mbox = &mctrl->stack->h2f[type];
	data = vpu_h2f_g_mboxdata(mbox);
	wptr_ofs16 = mbox->wptr_ofs16;
	rptr_ofs16 = mbox->rptr_ofs16;
	size16 = size >> 1;

	wptr = data + wptr_ofs16;
	if ((wptr_ofs16 >= rptr_ofs16) && (mbox->data_size16 - wptr_ofs16 < size16)) {
		/* Write dummy message header */
		cmd_hdr = (struct vpu_mailbox_h2f_hdr *)wptr;
		cmd_hdr->signature = VPU_DUM_SIGNATURE;
		cmd_hdr->cid = VPU_DUM_SIGNATURE;
		cmd_hdr->size16 = mbox->data_size16 - wptr_ofs16;

#ifdef DBG_MAILBOX_CORE
		vpu_info("[I%d][MBOX][H2F%d] dummy(%d, %d, %d, %d)\n", gid, type,
			cid, wptr_ofs16, mbox->rptr_ofs16, cmd_hdr->size16);
#endif

		wptr = data;
		wptr_ofs16 = 0;
	}

	/* fill message header and copy message to Mail-box */
	cmd_hdr = (struct vpu_mailbox_h2f_hdr *)payload;
	cmd_hdr->signature = VPU_CMD_SIGNATURE;
	cmd_hdr->cid = cid;
	cmd_hdr->size16 = (__u16)size16;
	mem2iocpy(wptr, payload, size);

#ifdef DBG_MAILBOX_CORE
	vpu_info("[I%d][MBOX][H2F%d] %d command(%d, %d, %d, %d)\n", gid, type,
		cmd, cid, wptr_ofs16, mbox->rptr_ofs16, cmd_hdr->size16);
#endif

	wptr_ofs16 += size16;
	if (wptr_ofs16 == mbox->data_size16)
		wptr_ofs16 = 0;

	/* increment actual write pointer */
	mbox->wptr_ofs16 = wptr_ofs16;

	return 0;
}

int vpu_mbox_read(struct vpu_mailbox_ctrl *mctrl,
	void *payload, u32 type, void *debug_data)
{
	int ret = 0;
	struct vpu_mailbox_f2h *mbox;
	u16 *data, *rptr;

	BUG_ON(!mctrl);
	BUG_ON(!payload);
	BUG_ON(type >= VPU_MTYPE_F2H_CNT);

	mbox = &mctrl->stack->f2h[type];
	data = vpu_f2h_g_mboxdata(mbox);
	rptr = data + (mbox->msg_size16 * mbox->rmsg_idx);

	io2memcpy(payload, rptr, mbox->msg_size16 << 1);

#ifdef DBG_MAILBOX_CORE
	switch (type) {
	case VPU_MTYPE_F2H_NORMAL_RESPONSE:
	case VPU_MTYPE_F2H_LOW_RESPONSE:
	{
		struct vpu_interface *interface = debug_data;
		struct vpu_mailbox_reply *reply = payload;
		struct vpu_framemgr *iframemgr = &interface->framemgr;
		struct vpu_frame *iframe;
		u32 cid = reply->cid;

		iframe = &iframemgr->frame[cid];

		vpu_info("[I%ld][MBOX][F2H%d] %d reply(%d, %d, %d, %d)\n", iframe->param1, type,
			iframe->message, cid, reply->err_type, mbox->wmsg_idx, mbox->rmsg_idx);
		break;
	}
	case VPU_MTYPE_F2H_REPORT:
	{
		struct vpu_interface *interface = debug_data;
		struct vpu_mailbox_f2h_report *reply = payload;
		struct vpu_framemgr *iframemgr = &interface->framemgr;
		struct vpu_frame *iframe;
		u32 cid = reply->cid_lsb;

		iframe = &iframemgr->frame[cid];

		vpu_info("[I%ld][MBOX][F2H%d] %d reply(%d, %d, %d, %d)\n", iframe->param1, type,
			iframe->message, cid, reply->task_id, mbox->wmsg_idx, mbox->rmsg_idx);
		break;
	}
	default:
		break;
	}
#endif

	mbox->rmsg_idx = (mbox->rmsg_idx + 1) % mbox->emsg_idx;

	return ret;
}
