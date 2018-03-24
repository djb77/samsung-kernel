/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/**** Describe Initialization & MBOX interface between VPU driver and CORE ****
 * The interface between HOST and CORE is based on few MBOXes.
 * There are 2 MBOXes from Host to CORE and 4 MBOXes from CORE to HOST.
 * Both HOST MBOXes are commands MBOXes - one includes "normal" commands and
 * the other "low priority" commands. The "low priority" commands will be
 * handled as BG tasks.
 * CreateTask and AllocateTask are defined as "low priority" commands.
 * The MBOXes from CORE are reports, system errors and commands errors MBOX for
 * each of the 2 HOST commands MBOXes. The reports MBOX will include one or
 * more reports for each task invocation - a report after the VPUI_TVER_END
 * vertex is finished and a report for each VPUI_TVER_HOST_REP vertex
 * (if such vertices exist).
 * Each MBOX each has controller and data. The MBOXEs controllers are
 * initialized on CORE initialization. The only valid MBOXes on this stage is
 * the HOST normal commands MBOX and the CORE normal commands errors MBOX.
 * As part of VPUI_CMND_INIT all the MBOXes sizes are set, MBOXes controllers
 * are updated and all MBOXes become valid. The only MBOX which its size might
 * be dynamically updated is the "low priority" commands MBOX
 * (using VPUI_CMND_REALLOC_LOWPR_CMD_MBX command).
 * On initialization command the HOST also describe which CORE reports it would
 * like to receive as interrupts and whether it would like to get interrupts
 * for CORE updates of read pointers in commands MBOXes.
 * The Initialization interface between VPU driver and CORE is based on single
 * predefined address that is located very near to end of the CORE SRAM. It is
 * assumed to be a base of an array of 16 bits values. The CORE sets these
 * values on initialization. One of those values is an offset to a structure of
 * controllers of all the MBOXes. The last values CORE writes on initialization
 * are 2 signatures that are written to this array. HOST may read the offset to
 * MBOXes controllers only after both signatures are set.
 * All the Initialization & MBOX interface between VPU driver and CORE is based
 * on 16 bits values to avoid compiler mismatches potential problems.
 * The only assumption is that little endianity is used.
 */

#ifndef VPU_FWIF_MBOX_H
#define VPU_FWIF_MBOX_H

#ifndef VPU_PROJ_HW_VER
#define VPU_PROJ_HW_VER	210
	/* While VPU 2.10 is the only HW supported - defined by default */
#endif

#if (VPU_PROJ_HW_VER == 210)		/* VPU 2.10 */
#include "vpu-fwif-hw-proj210.h"	/* required for VPUH_HW_CHAINS */
#endif

/* VPUM_MBOX_IF_VERSION_ID 1.0.00 delivered on 17/04/16, SVN version 59264
 * Updated on each delivery when current file has any changes from the
 * previous delivery
 */
#define VPUM_MBOX_IF_VERSION_ID		1001	/* 1.0.01 */

/* HOST_TO_CPU and CPU_TO_HOST interrupts description */
/* Address offsets of HOST_TO_CPU and CPU_TO_HOST are relative to this base */
#define VPUM_APB_BASE_ADR		0xF0000000

/* The HOST should set and clear the appropriate interrupt bit after it updates
 * the appropriate MBOX Write Pointer.
 */
#define HOST_TO_CPU_INTERRUPT_BIT	0x100	/* Both interrupts */
/* HOST Interrupt 0 should be set for each new "normal" command */
#define VPUM_HOST_TO_CPU_INTR_0_ADR_OFS	0x10
/* HOST Interrupt 1 should be set for each new "low priority" command */
#define VPUM_HOST_TO_CPU_INTR_1_ADR_OFS	0x14

/* HOST defines on "init" command per each MBOX control whether CPU should send
 * interrupt0 / interrupt1 / no interrupt per each MBOX update.
 * CPU sets the appropriate interrupt after it updates the MBOX Write Pointer.
 * Since HOST need a LEVEL interrupt CPU is not allowed to clear the interrupt.
 * HOST should clear the appropriate interrupt and than check the MBOX Write
 * Pointer.Clear interrupt 0 is done by writing 0 to the register while clear
 * interrupt 1 is done by writing the appropriate bit to the register.
 */
#define VPUM_CPU_TO_HOST_INTR_0_ADR_OFS	0x18
#define HOST_CLR_CPU_INTERRUPT_1_BIT	(1 << 13)
#define VPUM_CPU_TO_HOST_INTR_1_ADR_OFS	0x1B4

/* The Entire size of CORE code & data memory */
#define VPUM_CORE_SRAM_SIZE		0x20000
/* The offset to predefined data in 16 bit units from CORE SRAM end */
#define VPUM_BASE_OFS_FROM_SRAM_END_16B	16
#define VPUM_INTERFACE_BASE_ADR\
	(VPUM_CORE_SRAM_SIZE - VPUM_BASE_OFS_FROM_SRAM_END_16B * 2)

#define VPUM_SIGNATURE_1_VALUE		0xcafe
#define VPUM_SIGNATURE_2_VALUE		0xefac

/* Bellow structure is located in VPUM_INTERFACE_BASE_ADR. It may include up to
 * 16 (VPUM_BASE_OFS_FROM_SRAM_END_16B) __u16 values
 */
struct VPUM_FixAdrHostIntr {
	/* Set to VPUM_SIGNATURE_1_VALUE after entire CORE init is done */
	__u16 Signature1;
	/* Set to VPUM_SIGNATURE_2_VALUE after entire CORE init is done */
	__u16 Signature2;
	/* The negative offset from this structure base
	 * (VPUM_INTERFACE_BASE_ADR) to VPUM_DbgAndAllMboxCont structure
	 * in 16B units
	 */
	__u16 Neg16bOfsToMbCont;
	/* 2100 for VPU_2_10_0. Other values for future HW implementations. */
	__u16 HwVersion;
	/* VPUI_CMND_FW_VERSION_ID */
	__u16 FwVersion;
	/* VPUM_MBOX_IF_VERSION_ID */
	__u16 MboxIfVersion;
	/* VPUI_CMND_IF_VERSION_ID */
	__u16 CmndIfVersion;
	/* The entire heap size in 16 bits units */
	__u16 MemHeapSize16b;
	/* Total MBOXes sizes in 16 bits units (MBOXes are allocated as part of
	 * the heap, extra place in heap used for internal data)
	 */
	__u16 MboxesTotSize16b;
};

union VPUM_FixAdrHostIntrUn {
	struct VPUM_FixAdrHostIntr	Str;
	__u16			Dummy[VPUM_BASE_OFS_FROM_SRAM_END_16B];
};

enum VPUM_SysErrType {
	VPUM_SYS_NO_ERR					= 0,
	VPUM_SYS_WRONG_HW_VERSION		= 1,
	/* AddInfo1 is MBOXes total default size, AddInfo2 is heap size */
	VPUM_SYS_TOO_SMALL_HEAP			= 2,
	VPUM_SYS_FULL_REPORTS_MBOX		= 3,
	VPUM_SYS_FULL_NORM_CMD_RESPONSE_MBOX	= 4,
	VPUM_SYS_FULL_LOWP_CMD_RESPONSE_MBOX	= 5,
	VPUM_SYS_FULL_DBG_MBOX			= 6,
	/* AddInfo1 is 1 for Process Queue / 0 for control queue,
	 * AddInfo2 is the vertex index in the task
	 */
	VPUM_SYS_FULL_VERTICES_QUEUE	= 7,
	/* AddInfo1 is Task Id */
	VPUM_SYS_TASK_TIME_OUT			= 8,
	/* AddInfo1 is Task Id, AddInfo2 is Vertex Id */
	VPUM_SYS_PROCESS_TIME_OUT		= 9,
	/* AddInfo1 is Task Id, AddInfo2 is Vertex Id */
	VPUM_SYS_HW_CHAIN_TIME_OUT		= 10,
	/* AddInfo1 is actual number of report parameters,
	 * AddInfo2 is maximal number of report parameters
	 */
	VPUM_SYS_TOO_MANY_REP_PARAMS	= 11,
	/* Got AXI error on DRAM access */
	VPUM_SYS_DRAM_AXI_ERROR			= 12,
	/* HOST try to access forbidden SRAM area / HW register */
	VPUM_SYS_HOST_FORBIDEN_ACCESS	= 13,
	/* MBOX Write pointer >= MBOX total size.
	 * AddInfo1 is NextWrPtr16bOfs and AddInfo2 is DataSize16b
	 */
	VPUM_SYS_HOST_MBOX_BAD_WR_PTR	= 14,
	/* Set only in case the HOST request to get such errors on CmndInit
	 *	AddInfo1 is Command Id, AddInfo2 is expected Command Ind
	 */
	VPUM_SYS_UNEXPECTED_CMND_IND	= 15,
	/* AddInfo1 is FwErrType, AddInfo2 is uAdd */
	VPUM_SYS_INTERNAL_ERROR			= 100
	/* TBD - more system errors */
};

enum VPUM_MboxIntr {
	/* Core should not set any IRQ to HOST on MBOX updates */
	VPUM_INTR_NONE	= 0,
	/* Core should set IRQ0 to HOST on each CORE MBOX update */
	VPUM_INTR_IRQ0	= 1,
	/* Core should set IRQ1 to HOST on each CORE MBOX update */
	VPUM_INTR_IRQ1	= 2
};

#define VPUM_MBOX_HOST_NORMAL_CMND_DEF_16B_SIZE	500
#define VPUM_MBOX_HOST_LOW_PR_CMND_DEF_16B_SIZE	5000

struct VPUM_HostMboxCont {
	/* The offset from the MBOX controller base to the MBOX data
	 * (up to 128K offset). Assumed to be positive.
	 */
	__u16	DataOfs16b;
	/* The size in 16 bits units of the entire MBOX. MBOX may include only
	 * DataSize16b-1 words (since NextWrMsgInd == NextRdMsgInd stands for
	 * empty MBOX)
	 */
	__u16	DataSize16b;
	/* Incremented up to DataSize16b by the writer (HOST) after the entire
	 * message is written. If new NextWrPtr16bOfs == DataSize16b it is
	 * set to 0. Message must not be folded - if message size > DataSize16b
	 * NextWrPtr16bOfs dummy message should set NextWrPtr16bOfs to 0
	 */
	__u16	NextWrPtr16bOfs;
	/* Incremented by the reader(CORE) after the entire message is read
	 * (if new NextRdtr16bOfs == DataSize16b it is set to 0)
	 * Notice - NextWrPtr16bOfs == NextRdPtr16bOfs stands for empty MBOX
	 */
	__u16	NextRdPtr16bOfs;
	/* Initialized to TRUE and updated (by the CORE) on VPUI_CMND_INIT
	 * according to HOST request. Refers to core messages written to
	 * appropriate MBOX for commands - if set message is sent only on
	 * errors, if reset - message is sent on each command.
	 */
	__u16	ResponseOnErrorOnly;
};

#define VPUM_MBOX_CORE_REPORTS_DEF_MSG_SIZE			20
#define VPUM_MBOX_CORE_CR_SIZES_DEF_MSG_SIZE		4
#define VPUM_MBOX_CORE_SYS_ERRORS_DEF_MSG_SIZE		4
#define VPUM_MBOX_CORE_NORMAL_CMND_RESPONSE_DEF_MGS_SIZE	8
#define VPUM_MBOX_CORE_LOW_PR_CMND_RESPONSE_DEF_MSG_SIZE	4
/* Used only when FW compiled with SEND_DEBUG_TO_HOST macro defined */
#define VPUM_MBOX_CORE_DBG_DEF_MSG_SIZE				200

struct VPUM_CoreMboxCont {
	/* The offset from the MBOX controller base to the MBOX data
	 * (up to 128K offset). Assumed to be positive.
	 */
	__u16	DataOfs16b;
	/* The size in 16 bits units of each message */
	__u16	MsgSize16b;
	/* The data size of the MBOX. MBOX may include only MsgNum - 1 messages
	 * (since NextWrMsgInd == NextRdMsgInd stands for empty MBOX)
	 */
	__u16	MsgNum;
	/* Incremented modulo MsgNum by the writer (CORE) after the entire
	 * message is written.
	 */
	__u16	NextWrMsgInd;
	/* Incremented modulo MsgNum by the reader (HOST) when message might be
	 * overwritten. In messages units.
	 * Notice - NextWrMsgInd == NextRdMsgInd stands for empty MBOX
	 */
	__u16	NextRdMsgInd;
	/* Set by the Writer (CORE) when new message should be written and
	 * NextWrMsgInd + 1 % MsgNum == NextRdMsgInd Released by the Reader
	 * (HOST) - after NextRdMsgInd is updated.
	 */
	__u16	FlagFull;
	/* VPUM_MboxIntr. Initialized to VPUM_INTR_NONE and updated
	 * (by the CORE) on VPUI_CMND_INIT according to HOST request
	 */
	__u16	HostIntrType;
};

enum VPUM_HostMbox {
	/* All commands except those defined as "Low priority" */
	VPUM_HOST_MBOX_NORMAL_CMND	= 0,
	/* "Low priority" commands are Create & Allocate - long process time,
	 * low priority and long messages
	 */
	VPUM_HOST_MBOX_LOW_PR_CMND	= 1,
	VPUM_HOST_MBOX_ALL		= 2
};

enum VPUM_CoreMbox {
	/* Send the reports to HOST from each invoked task */
	VPUM_CORE_MBOX_REPORTS			= 0,
	/* Send the task base size and each slot size for each created task */
	VPUM_CORE_MBOX_CREATE_SIZES		= 1,
	/* General system errors to HOST */
	VPUM_CORE_MBOX_SYS_ERRORS		= 2,
	/* Commands responses for HOST MBOX normal commands.
	 * A response is sent for each command or on errors only according to
	 * HOST configuration
	 */
	VPUM_CORE_MBOX_NORMAL_CMND_RESPONSE	= 3,
	/* Commands responses for HOST MBOX low priority commands.
	 * A response is sent for each command or on errors only according to
	 * HOST configuration
	 */
	VPUM_CORE_MBOX_LOW_PR_CMND_RESPONSE	= 4,
	/* Send Debug info when FW is compiled with SEND_DEBUG_TO_HOST
	 * MBOX is always empty when SEND_DEBUG_TO_HOST not defined
	 */
	VPUM_CORE_MBOX_DEBUG_INFO		= 5,
	VPUM_CORE_MBOX_ALL			= 6
};

struct VPUM_DbgHwChain {
	/* VPUI_INVALID_TASK_INVOKE_ID when HW Chain isn't allocated */
	__u8	TaskId;
	/* The process index in the task the HW chain is allocated to */
	__u8	VertexInd;
};

struct VPUM_DbgData {
	/* Which HW Chains are currently active. HW chain might be allocated
	 * allocated bit not currently active when a CPU chain is active and
	 * when control operations are invoked
	 */
	__u16 ActiveHwChMask;
	struct VPUM_DbgHwChain HwChain[VPUH_HW_CHAINS];
};

/* The data of all MBOXes controllers (except the low priority HOST commands
 * controller) are initialized on core initialization. In case a HW version
 * error found on core initialization it is set on CORE System Error MBOX.
 * On this case the HOST normal commands MBOX is disabled. Otherwise - the Init
 * command should be set on Normal HOST commands MBOX. In case an error found
 * in the Init command it will be reported to Normal commands Core errors MBOX.
 * As part of the Init command the sizes of the MBOXes might be updated from
 * their default values. Notice that on this case the data offsets and total
 * sizes in controllers will be updated (only if command succeeded). The only
 * exception is that the data offset of HOST Normal commands will be kept such
 * that the init command will be kept in the MBOX. The HOST should be very
 * careful when reading the MBOX controllers and data on this case.
 * All MBOXes sizes are fixed along the entire system operation
 * (after set on init command). The only exception is the Low priority HOST
 * commands. Its size might be updated by the VPUI_CMND_REALLOC_LOWPR_CMD_MBX
 * command. It is allowed only when the queue is empty. Notice that on this
 * case not only the MBOX size but also the other fields in the MBOX controller
 *  might be updated.
 */
struct VPUM_DbgAndAllMboxCont {
	struct VPUM_DbgData	Dbg;
	struct VPUM_HostMboxCont HostMboxCont[VPUM_HOST_MBOX_ALL];
	struct VPUM_CoreMboxCont CoreMboxCont[VPUM_CORE_MBOX_ALL];
};

/****************** HOST command description *********************************/
/* Each message in HostCmnd MBOX has the below header */
#define VPUM_REAL_CMND_SIGNATURE	0x1234
/* Dummy command is used in order to set the NextWrPtr16bOfs to 0 to avoid next
 * message from being folded
 */
#define VPUM_DUMMY_CMND_SIGNATURE	0x5678

struct VPUM_HostCmndHdr {
	/* either VPUM_REAL_CMND_SIGNATURE or VPUM_DUMMY_CMND_SIGNATURE */
	__u16	Signature;
	/* Used by commands response reports. Might be used to verify there are
	 * no lost commands (lost commands will be reported as system error -
	 * not command error). Different CommandInd for each HOST MBOX.
	 */
	__u16	CommandInd;
	/* Include the header. */
	__u16	MsgSize16B;
};

/******************* Core report description *********************************/
/* Each message in CoreReport MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_CoreReport) / 2)
 */
#define VPUM_REP_VER_END_IND			0
/* Either bad input or OVERFLOW found along the task */
#define VPUM_REP_VER_END_BAD_DATA_IND	0x8000
#define VPUM_REP_ABORT_IND				0xffff

struct VPUM_CoreReport {
	/* The task Id that one of its vertices reports */
	__u16  TaskId;
	/* The vertex that reports - either the VPUI_TVER_END vertex or one of
	 * VPUI_TVER_HOST_REP vertices
	 */
	__u16  VertexInd;
	/* Unique (per task) invocation Id. Copied from Invoke command */
	__u16	InvokeIdLsb;
	__u16	InvokeIdMsb;
	/* The actual data size on the message. Expected values are 0 .. ReportDataSize */
	__u16   DataSize;
	/* Place holder. Actual number is set by ReportDataSize
	 * on VPUM_MboxInitCmndInfo
	 */
	__u16	Data[1];
};

/**************** Core Create sizes  description******************************/
/* Each message in CoreCreateSizes MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_CoreCrSizes) / 2)
 */

struct VPUM_CoreCrSizes {
	/* The task Id that is created */
	__u16	TaskId;
	/* The  size in 16b units of the basic task information */
	__u16	BaseSize16b;
	/* The  size in 16b units of each slot of the task  */
	__u16	EachSlotSize16b;
};

enum VPUM_MboxErr {
	VPUM_MBOX_NO_ERR				= 0,
	/* The signature is nor VPUM_REAL_CMND_SIGNATURE neither
	 * VPUM_DUMMY_CMND_SIGNATURE
	 */
	VPUM_MBOX_WRONG_SIGNATURE		= 1,
	/* RdPtr16bOfs + MsgSize16B > DataSize16b */
	VPUM_MBOX_MSG_FOLDED			= 2,
	/* RdPtr16bOfs < WrPtr16bOfs and RdPtr16bOfs+MsgSize16B>WrPtr16bOfs  */
	VPUM_MBOX_MSG_PASS_WR_PTR		= 3,
	/* The signature is VPUM_DUMMY_CMND_SIGNATURE but the message
	 * RdPtr16bOfs + MsgSize16B != DataSize16b
	 */
	VPUM_MBOX_DUMMY_NOT_END			= 4,
	/* Dummy message (read until MBOX end) and RdPtr16bOfs < WrPtr16bOfs */
	VPUM_MBOX_DUMMY_PASS_WR_PTR		= 5,
	/* MsgSize16B <= (sizeof(VPUM_HostCmndHdr)+sizeof(VPUI_CmndHdr))/2*/
	VPUM_MBOX_TOO_SMALL_MSG_SIZE	= 6,
	/* Any VPUI_CmndErrType found for the command.The explicit
	 * VPUI_CmndErrType is added to VPUI_MBOX_CMND_ERR_BASE.
	 */
	VPUM_MBOX_CMND_ERR_BASE			= 0x100
	/* HOST may conclude whether either MBOX error happened
	 * (Error < VPUI_MBOX_CMND_ERR_BASE) or Command error happend.
	 */
};

/*************** Core command error description ******************************/
/* Each message in CoreCmndResponse MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_CoreCmndResponse) / 2)
 */
struct VPUM_CoreCmndResponse {
	/* copied from Command message */
	__u16  CommandInd;
	/* The offset in the appropriate command queue */
	__u16  CmndPtr16bOfs;
	/* VPUM_MboxErr value */
	__u16  ErrType;
};

/*************** Core system error description *******************************/
/* Each message in SystemErr MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_SysErr) / 2)
 */
struct VPUM_SysErr {
	__u16  ErrType;		/* enum VPUM_SysErrType value */
	__u16  AddInfo1;	/* extra data for explicit error description */
	__u16  AddInfo2;	/* extra data for explicit error description */
};

/**************** Core debug info description ********************************/
/* Each message in Debug information MBOX has the below structure
 * (MsgSize16b == sizeof(struct VPUM_DebugInfo) / 2)
 */
struct VPUM_DebugInfo {
	__u16  Info[4];		/* enum VPUM_SysErrType value */
};

struct VPUM_HostMboxData {
	__u16  Size16b;
	/* If set - Core message is written to appropriate MBOX only when
	 * error found in MBOX message / command. If reset - Core message is
	 * written to appropriate MBOX on each command.
	 */
	__u16  SendResponseOnErrorOnly;
};

struct VPUM_CoreMboxData {
	__u16  MsgNum;
	/* VPUM_MboxIntr. interrupt after CORE write message and updates
	 * appropriate MBBOX NextWrMsgInd
	 */
	__u16  IntrType;
};

/* The default number of Data parameters on each CORE report */
#define VPUM_DEF_REPORT_DATA_SIZE	8

struct VPUM_MboxInitCmndInfo {
	struct VPUM_HostMboxData HostMboxData[VPUM_HOST_MBOX_ALL];
	struct VPUM_CoreMboxData CoreMboxData[VPUM_CORE_MBOX_ALL];
	/* If set CORE should verify for both HOST MBOXes that CmndInd is
	 * increased by 1 between each couple of commands
	 */
	__u16	VerifyIncCmndInd;
	/* If set - should update the MBOXes sizes according to Sizes in
	* MBOXes array. If reset - MBOXes sizes are kept as is.
	*/
	__u16	UpdateMboxSizes;
	/* The number of Data parameters on each VPUM_CoreReportStr.
	 * At least 1 - if set to 0 - previous value is kept as is.
	 */
	__u16	ReportDataSize;
};

#endif
