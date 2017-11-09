/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPU_MAILBOX_H_
#define VPU_MAILBOX_H_

#include "lib/vpu-fwif-hw-proj210.h"

#define VPU_MAILBOX_SIGNATURE1		0xCAFE
#define VPU_MAILBOX_SIGNATURE2		0xEFAC
#define VPU_MAILBOX_BASEOFFSET		16

struct vpu_mailbox_hdr {
	/* Set to VPUM_SIGNATURE_1_VALUE after entire CORE init is done */
	u16				signature1;
	/* Set to VPUM_SIGNATURE_2_VALUE after entire CORE init is done */
	u16				signature2;
	/* The negative offset from this structure base
	 * (VPUM_INTERFACE_BASE_ADR) to VPUM_HostAllMboxContStr structure
	 * in 16B units
	 */
	u16				stack_ofs16;
	/* 2100 for VPU_2_10_0. Other values for future HW implementations. */
	u16				hw_version;
	/* VPUI_CMND_FW_VERSION_ID */
	u16				fw_version;
	/* VPUM_MBOX_IF_VERSION_ID */
	u16				mbox_version;
	/* VPUI_CMND_IF_VERSION_ID */
	u16				cmd_version;
	/* The entire heap size in 16 bits units */
	u16				heap_size16;
	/* Total MBOXes sizes in 16 bits units (MBOXes are allocated as part of
	 * the heap, extra place in heap used for internal data)
	 */
	u16				mbox_totsize16;
	u8				reserved[14];
};

struct vpu_info_chain {
	/* VPUI_INVALID_TASK_INVOKE_ID when HW Chain isn't allocated */
	u8				task_id;
	/* The process index in the task the HW chain is allocated to */
	u8				vertex_index;
};

struct vpu_info {
	/* Which HW Chains are currently active. HW chain might be allocated
	 * allocated bit not currently active when a CPU chain is active and
	 * when control operations are invoked
	 */
	u16				active_chain_mask;
	struct vpu_info_chain		chains[VPUH_HW_CHAINS];
};

#define VPU_CMD_SIGNATURE		0x1234
#define VPU_DUM_SIGNATURE		0x5678

struct vpu_mailbox_h2f_hdr {
	/* either VPUM_REAL_CMND_SIGNATURE or VPUM_DUMMY_CMND_SIGNATURE */
	u16				signature;
	/* Used by commands response reports. Might be used to verify there are
	 * no lost commands (lost commands will be reported as system error -
	 * not command error). Different CommandInd for each HOST MBOX.
	 */
	u16				cid;
	/* Include the header. */
	u16				size16;
};

struct vpu_mailbox_h2f {
	/* The offset from the MBOX controller base to the MBOX data
	 * (up to 128K offset). Assumed to be positive.
	 */
	u16				data_ofs16;
	/* The size in 16 bits units of the entire MBOX. MBOX may include only
	 * DataSize16b-1 words (since NextWrMsgInd == NextRdMsgInd stands for
	 * empty MBOX)
	 */
	u16				data_size16;
	/* Incremented up to DataSize16b by the writer (HOST) after the entire
	 * message is written. If new NextWrPtr16bOfs == DataSize16b it is
	 * set to 0. Message must not be folded - if message size > DataSize16b
	 * NextWrPtr16bOfs dummy message should set NextWrPtr16bOfs to 0
	 */
	u16				wptr_ofs16;
	/* Incremented by the reader(CORE) after the entire message is read
	 * (if new NextRdtr16bOfs == DataSize16b it is set to 0)
	 * Notice - NextWrPtr16bOfs == NextRdPtr16bOfs stands for empty MBOX
	 */
	u16				rptr_ofs16;
	/* Initialized to TRUE and updated (by the CORE) on VPUI_CMND_INIT
	 * according to HOST request. Refers to core messages written to
	 * appropriate MBOX for commands - if set message is sent only on
	 * errors, if reset - message is sent on each command.
	 */
	u16				response_on_error_only;
};

struct vpu_mailbox_f2h {
	/* The offset from the MBOX controller base to the MBOX data
	 * (up to 128K offset). Assumed to be positive.
	 */
	u16				data_ofs16;
	/* The size in 16 bits units of each message */
	u16				msg_size16;
	/* The data size of the MBOX. MBOX may include only MsgNum - 1 messages
	 * (since NextWrMsgInd == NextRdMsgInd stands for empty MBOX)
	 */
	u16				emsg_idx;
	/* Incremented modulo emsg_idx by the writer (CORE) after the entire
	 * message is written.
	 */
	u16				wmsg_idx;
	/* Incremented modulo MsgNum by the reader (HOST) when message might be
	 * overwritten. In messages units.
	 * Notice - NextWrMsgInd == NextRdMsgInd stands for empty MBOX
	 */
	u16				rmsg_idx;
	/* Set by the Writer (CORE) when new message should be written and
	 * NextWrMsgInd + 1 % emsg_idx == NextRdMsgInd Released by the Reader
	 * (HOST) - after NextRdMsgInd is updated.
	 */
	u16				full;
	/* VPUM_MboxIntr. Initialized to VPUM_INTR_NONE and updated
	 * (by the CORE) on VPUI_CMND_INIT according to HOST request
	 */
	u16				intr_type;
};

/******************* Core report description *********************************/
/* Each message in CoreReport MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_CoreReport) / 2)
 */
#define VPUM_REP_VER_END_IND	0
#define VPUM_REP_ABORT_IND	0xffff

struct vpu_mailbox_f2h_report {
	/* The task Id that one of its vertices reports */
	u16				task_id;
	/* The vertex that reports - either the VPUI_TVER_END vertex or one of
	 * VPUI_TVER_HOST_REP vertices
	 */
	u16				vertex_idx;
	/* Unique (per task) invocation Id. Copied from Invoke command */
	u16				cid_lsb;
	u16				cid_msb;
	/* Place holder. Actual number is set by ReportDataSize
	 * on VPUM_MboxInitCmndInfo
	 */
	/* The actual data size on the message. Expected values are 0 .. ReportDataSize */
	u16				data_size;
	u16				data[1];
};

/**************** Core Create sizes  description******************************/
/* Each message in CoreCreateSizes MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_CoreCrSizes) / 2)
 */

struct vpu_mailbox_f2h_size {
	/* The task Id that is created */
	u16				task_id;
	/* The  size in 16b units of the basic task information */
	u16				base_size16;
	/* The  size in 16b units of each slot of the task  */
	u16				each_slot_size16;
};

/*************** Core system error description *******************************/
/* Each message in SystemErr MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_SysErr) / 2)
 */
struct vpu_mailbox_syserr {
	u16				err_type; /* enum VPUM_SysErrType value */
	u16				info1;	/* extra data for explicit error description */
	u16				info2;	/* extra data for explicit error description */
};

/*************** Core command error description ******************************/
/* Each message in CoreCmndErr MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_CoreCmndErr) / 2)
 */
struct vpu_mailbox_reply {
	/* copied from Command message */
	u16				cid;
	/* The offset in the appropriate command queue */
	u16				cmd_ptr_ofs16;
	/* VPUM_MboxErr value */
	u16				err_type;
};

/**************** Core debug info description ********************************/
/* Each message in Debug information MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_DebugInfo) / 2)
 */
struct vpu_mailbox_debug {
	__u16  Info[4];		/* enum VPUM_SysErrType value */
};

enum vpu_mailbox_h2f_type {
	/* All commands except those defined as "Low priority" */
	VPU_MTYPE_H2F_NORMAL,
	/* "Low priority" commands are Create & Allocate - long process time,
	 * low priority and long messages
	 */
	VPU_MTYPE_H2F_LOW,
	VPU_MTYPE_H2F_CNT
};

enum vpu_mailbox_f2h_type {
	/* Send the reports to HOST from each invoked task */
	VPU_MTYPE_F2H_REPORT,
	/* Send the task base size and each slot size for each created task */
	VPU_MTYPE_F2H_SIZE,
	/* General system errors to HOST */
	VPU_MTYPE_F2H_SYS_ERROR,
	/* report errors in HOST MBOX normal commands */
	VPU_MTYPE_F2H_NORMAL_RESPONSE,
	/* report errors in HOST MBOX low priority commands */
	VPU_MTYPE_F2H_LOW_RESPONSE,
	VPU_MTYPE_F2H_DEBUG,
	VPU_MTYPE_F2H_CNT
};

struct vpu_mailbox_stack {
	struct vpu_info			info;
	struct vpu_mailbox_h2f		h2f[VPU_MTYPE_H2F_CNT];
	struct vpu_mailbox_f2h		f2h[VPU_MTYPE_F2H_CNT];
};

struct vpu_mailbox_ctrl {
	struct vpu_mailbox_hdr		*hdr;
	struct vpu_mailbox_stack	*stack;
};

struct vpu_mailbox_hdr * vpu_mbox_g_hdr(void *code, u32 code_size);
struct vpu_mailbox_stack * vpu_mbox_g_stack(struct vpu_mailbox_hdr *hdr);
int vpu_mbox_ready(struct vpu_mailbox_ctrl *mctrl, size_t size, u32 type);
int vpu_mbox_write(struct vpu_mailbox_ctrl *mctrl,
	void *payload, size_t size, u32 type, u32 gid, u32 cmd, u32 cid);
int vpu_mbox_read(struct vpu_mailbox_ctrl *mctrl,
	void *payload, u32 type, void *debug_data);

#endif
