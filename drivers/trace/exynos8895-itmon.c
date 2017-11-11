/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * IPs Traffic Monitor(ITMON) Driver for Samsung Exynos8895 SOC
 * By Hosung Kim (hosung0.kim@samsung.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/of_irq.h>
#include <soc/samsung/exynos-itmon.h>
#if defined(CONFIG_SEC_SIPC_MODEM_IF)
#include <soc/samsung/exynos-modem-ctrl.h>
#endif
#ifdef CONFIG_SEC_DEBUG
#include <linux/sec_debug.h>
#endif


#define OFFSET_TMOUT_REG		(0x2000)
#define OFFSET_REQ_R			(0x0)
#define OFFSET_REQ_W			(0x20)
#define OFFSET_RESP_R			(0x40)
#define OFFSET_RESP_W			(0x60)
#define OFFSET_ERR_REPT 		(0x20)
#define OFFSET_NUM			(0x4)

#define REG_INT_MASK			(0x0)
#define REG_INT_CLR			(0x4)
#define REG_INT_INFO			(0x8)
#define REG_EXT_INFO_0			(0x10)
#define REG_EXT_INFO_1			(0x14)
#define REG_EXT_INFO_2			(0x18)

#define REG_DBG_CTL			(0x10)
#define REG_TMOUT_INIT_VAL		(0x14)
#define REG_TMOUT_FRZ_EN		(0x18)
#define REG_TMOUT_BUF_WR_OFFSET 	(0x10)
#define REG_TMOUT_BUF_RD_ADDR		(0x20)
#define REG_TMOUT_BUF_RD_ID		(0x24)
#define REG_TMOUT_BUF_RD_PAYLOAD	(0x28)
#define REG_TMOUT_BUF_WR_ADDR		(0x30)
#define REG_TMOUT_BUF_WR_ID		(0x34)
#define REG_TMOUT_BUF_WR_PAYLOAD	(0x38)

#define BIT_ERR_CODE(x) 		(((x) & (0xF << 28)) >> 28)
#define BIT_ERR_OCCURRED(x)		(((x) & (0x1 << 27)) >> 27)
#define BIT_ERR_VALID(x)		(((x) & (0x1 << 26)) >> 26)
#define BIT_AXID(x)			(((x) & (0xFFFF)))
#define BIT_AXUSER(x)			(((x) & (0xFFFF << 16)) >> 16)
#define BIT_AXBURST(x)			(((x) & (0x3)))
#define BIT_AXPROT(x)			(((x) & (0x3 << 2)) >> 2)
#define BIT_AXLEN(x)			(((x) & (0xF << 16)) >> 16)
#define BIT_AXSIZE(x)			(((x) & (0x7 << 28)) >> 28)

#define M_NODE				(0)
#define T_S_NODE			(1)
#define T_M_NODE			(2)
#define S_NODE				(3)
#define NODE_TYPE			(4)

#define ERRCODE_SLVERR			(0)
#define ERRCODE_DECERR			(1)
#define ERRCODE_UNSUPORTED		(2)
#define ERRCODE_POWER_DOWN		(3)
#define ERRCODE_UNKNOWN_4		(4)
#define ERRCODE_UNKNOWN_5		(5)
#define ERRCODE_TMOUT			(6)

#define BUS_DATA			(0)
#define BUS_PERI			(1)
#define BUS_PATH_TYPE			(2)

#define TRANS_TYPE_WRITE 		(0)
#define TRANS_TYPE_READ 		(1)
#define TRANS_TYPE_NUM			(2)

#define FROM_CP_MEM 			(0)
#define FROM_CP_PERI 			(1)
#define FROM_CPU 			(2)
#define FROM_CPU_MAY 			(3)
#define FROM_M_NODE 			(4)

#define CP_COMMON_STR			"CP_"
#define CP_MEM_STR			"CP_MEM"
#define CP_PERI_STR			"CP_PERI"

#define TMOUT				(0xFFFFF)
#define TMOUT_TEST			(0x1)

#define PANIC_ALLOWED_THRESHOLD 	(0x2)
#define INVALID_REMAPPING		(0x08000000)

static bool initial_multi_irq_enable = false;
static struct itmon_dev *g_itmon = NULL;

struct itmon_rpathinfo {
	unsigned int id;
	char *port_name;
	char *dest_name;
	unsigned int bits;
	unsigned int shift_bits;
};

struct itmon_masterinfo {
	char *port_name;
	unsigned int user;
	char *master_name;
	unsigned int bits;
};

struct itmon_nodegroup;

struct itmon_traceinfo {
	char* port;
	char* master;
	char* dest;
	unsigned long target_addr;
	unsigned int errcode;
	bool read;
	bool path_dirty;
	bool snode_dirty;
	bool dirty;
	unsigned long from;
	char buf[SZ_32];
};

struct itmon_tracedata {
	unsigned int int_info;
	unsigned int ext_info_0;
	unsigned int ext_info_1;
	unsigned int ext_info_2;
	unsigned int offset;
	bool logging;
	bool read;
};

struct itmon_nodeinfo {
	unsigned int type;
	char *name;
	unsigned int phy_regs;
	void __iomem *regs;
	unsigned int time_val;
	bool tmout_enabled;
	bool tmout_frz_enabled;
	bool err_enabled;
	bool retention;
	struct itmon_tracedata tracedata;
	struct itmon_nodegroup *group;
	struct list_head list;
};

const static char *itmon_pathtype[] = {
	"DATA Path transaction (0x2000_0000 ~ 0xf_ffff_ffff)",
	"PERI(SFR) Path transaction (0x0 ~ 0x1fff_ffff)",
};

/* Error Code Description */
const static char *itmon_errcode[] = {
	"Error Detect by the Slave(SLVERR)",
	"Decode error(DECERR)",
	"Unsupported transaction error",
	"Power Down access error",
	"Unsupported transaction",
	"Unsupported transaction",
	"Timeout error - response timeout in timeout value",
	"Invalid errorcode",
};

const static char *itmon_nodestring[] = {
	"M_NODE",
	"TAXI_S_NODE",
	"TAXI_M_NODE",
	"S_NODE",
};

struct itmon_nodegroup {
	int irq;
	char *name;
	unsigned int phy_regs;
	void __iomem *regs;
	struct itmon_nodeinfo *nodeinfo;
	unsigned int nodesize;
	unsigned int bus_type;
};

struct itmon_platdata {
	const struct itmon_rpathinfo *rpathinfo;
	const struct itmon_masterinfo *masterinfo;
	struct itmon_nodegroup *nodegroup;
	struct itmon_traceinfo traceinfo[BUS_PATH_TYPE];
	struct list_head tracelist[BUS_PATH_TYPE];
	unsigned int err_cnt;
	bool panic_allowed;
	bool crash_in_progress;
	bool probed;
};

const static struct itmon_rpathinfo rpathinfo[] = {
	/* Target Address = 0x2000_0000 ~ 0xf_ffff_ffff */
	{0, "DPU0",		"DREX", GENMASK(5, 0), 0},
	{1, "DPU1",		"DREX", GENMASK(5, 0), 0},
	{2, "DPU2",		"DREX", GENMASK(5, 0), 0},
	{3, "CAM0",		"DREX", GENMASK(5, 0), 0},
	{4, "CAM1",		"DREX", GENMASK(5, 0), 0},
	{5, "ISPLP",		"DREX", GENMASK(5, 0), 0},
	{6, "SRDZ",		"DREX", GENMASK(5, 0), 0},
	{7, "IVA",		"DREX", GENMASK(5, 0), 0},
	{8, "DSP",		"DREX", GENMASK(5, 0), 0},
	{9, "VPU",		"DREX", GENMASK(5, 0), 0},
	{10, "MFC0",		"DREX", GENMASK(5, 0), 0},
	{11, "MFC1",		"DREX", GENMASK(5, 0), 0},
	{12, "G2D0",		"DREX", GENMASK(5, 0), 0},
	{13, "G2D1",		"DREX", GENMASK(5, 0), 0},
	{14, "G2D2",		"DREX", GENMASK(5, 0), 0},
	{15, "VTS",		"DREX", GENMASK(5, 0), 0},
	{16, "FSYS0",		"DREX", GENMASK(5, 0), 0},
	{17, "CORESIGHT",	"DREX", GENMASK(5, 0), 0},
	{18, "PDMA",		"DREX", GENMASK(5, 0), 0},
	{19, "SPDMA",		"DREX", GENMASK(5, 0), 0},
	{20, "FSYS1",		"DREX", GENMASK(5, 0), 0},
	{21, "GNSS",		"DREX", GENMASK(5, 0), 0},
	{22, "ALIVE",		"DREX", GENMASK(5, 0), 0},
	{23, "ABOX",		"DREX", GENMASK(5, 0), 0},
	{24, "DPU0",		"DREX", GENMASK(5, 0), 0},
	{25, "DPU1",		"DREX", GENMASK(5, 0), 0},
	{26, "DPU2",		"DREX", GENMASK(5, 0), 0},
	{27, "CAM0",		"DREX", GENMASK(5, 0), 0},
	{28, "CAM1",		"DREX", GENMASK(5, 0), 0},
	{29, "ISPLP",		"DREX", GENMASK(5, 0), 0},
	{30, "SRDZ",		"DREX", GENMASK(5, 0), 0},
	{31, "IVA",		"DREX", GENMASK(5, 0), 0},
	{32, "DSP",		"DREX", GENMASK(5, 0), 0},
	{33, "VPU",		"DREX", GENMASK(5, 0), 0},
	{34, "MFC0",		"DREX", GENMASK(5, 0), 0},
	{35, "MFC1",		"DREX", GENMASK(5, 0), 0},
	{36, "G2D0",		"DREX", GENMASK(5, 0), 0},
	{37, "G2D1",		"DREX", GENMASK(5, 0), 0},
	{38, "G2D2",		"DREX", GENMASK(5, 0), 0},
	{39, "VTS",		"DREX", GENMASK(5, 0), 0},
	{40, "FSYS0",		"DREX", GENMASK(5, 0), 0},
	{41, "CORESIGHT",	"DREX", GENMASK(5, 0), 0},
	{42, "PDMA",		"DREX", GENMASK(5, 0), 0},
	{43, "SPDMA",		"DREX", GENMASK(5, 0), 0},
	{44, "FSYS1",		"DREX", GENMASK(5, 0), 0},
	{45, "GNSS",		"DREX", GENMASK(5, 0), 0},
	{46, "ALIVE",		"DREX", GENMASK(5, 0), 0},
	{47, "ABOX",		"DREX", GENMASK(5, 0), 0},
	{48, "G3D0",		"DREX", GENMASK(5, 0), 0},
	{49, "G3D1",		"DREX", GENMASK(5, 0), 0},
	{50, "G3D2",		"DREX", GENMASK(5, 0), 0},
	{51, "G3D3",		"DREX", GENMASK(5, 0), 0},
	{52, "CP_MEM",		"DREX", GENMASK(5, 0), 0},
	/* Target Address = 0x1800_0000 ~ 0x1fff_ffff */
	{0, "DPU0",		"SP", GENMASK(4, 0), 0},
	{1, "DPU1",		"SP", GENMASK(4, 0), 0},
	{2, "DPU2",		"SP", GENMASK(4, 0), 0},
	{3, "CAM0",		"SP", GENMASK(4, 0), 0},
	{4, "CAM1",		"SP", GENMASK(4, 0), 0},
	{5, "ISPLP",		"SP", GENMASK(4, 0), 0},
	{6, "SRDZ",		"SP", GENMASK(4, 0), 0},
	{7, "IVA",		"SP", GENMASK(4, 0), 0},
	{8, "DSP",		"SP", GENMASK(4, 0), 0},
	{9, "VPU",		"SP", GENMASK(4, 0), 0},
	{10, "MFC0",		"SP", GENMASK(4, 0), 0},
	{11, "MFC1",		"SP", GENMASK(4, 0), 0},
	{12, "G2D0",		"SP", GENMASK(4, 0), 0},
	{13, "G2D1",		"SP", GENMASK(4, 0), 0},
	{14, "G2D2",		"SP", GENMASK(4, 0), 0},
	{15, "VTS",		"SP", GENMASK(4, 0), 0},
	{16, "FSYS0",		"SP", GENMASK(4, 0), 0},
	{17, "CORESIGHT",	"SP", GENMASK(4, 0), 0},
	{18, "PDMA",		"SP", GENMASK(4, 0), 0},
	{19, "SPDMA",		"SP", GENMASK(4, 0), 0},
	{20, "FSYS1",		"SP", GENMASK(4, 0), 0},
	{21, "GNSS",		"SP", GENMASK(4, 0), 0},
	{22, "ALIVE",		"SP", GENMASK(4, 0), 0},
	{23, "ABOX",		"SP", GENMASK(4, 0), 0},
	/* Target Address = 0x0000_0000 ~ 0x17ff_ffff */
	{0, "DPU0",		"PERI", GENMASK(4, 0), 0},
	{1, "DPU1",		"PERI", GENMASK(4, 0), 0},
	{2, "DPU2",		"PERI", GENMASK(4, 0), 0},
	{3, "CAM0",		"PERI", GENMASK(4, 0), 0},
	{4, "CAM1",		"PERI", GENMASK(4, 0), 0},
	{5, "ISPLP",		"PERI", GENMASK(4, 0), 0},
	{6, "SRDZ",		"PERI", GENMASK(4, 0), 0},
	{7, "IVA",		"PERI", GENMASK(4, 0), 0},
	{8, "DSP",		"PERI", GENMASK(4, 0), 0},
	{9, "VPU",		"PERI", GENMASK(4, 0), 0},
	{10, "MFC0",		"PERI", GENMASK(4, 0), 0},
	{11, "MFC1",		"PERI", GENMASK(4, 0), 0},
	{12, "G2D0",		"PERI", GENMASK(4, 0), 0},
	{13, "G2D1",		"PERI", GENMASK(4, 0), 0},
	{14, "G2D2",		"PERI", GENMASK(4, 0), 0},
	{15, "ABOX",		"PERI", GENMASK(4, 0), 0},
	{16, "VTS",		"PERI", GENMASK(4, 0), 0},
	{17, "FSYS0",		"PERI", GENMASK(4, 0), 0},
	{18, "CORESIGHT",	"PERI", GENMASK(4, 0), 0},
	{19, "PDMA",		"PERI", GENMASK(4, 0), 0},
	{20, "SPDMA",		"PERI", GENMASK(4, 0), 0},
	{21, "FSYS1",		"PERI", GENMASK(4, 0), 0},
	{22, "GNSS",		"PERI", GENMASK(4, 0), 0},
	{22, "ALIVE",		"PERI", GENMASK(4, 0), 0},
};

const static struct itmon_masterinfo masterinfo[] = {
	{"DPU0", 0,			/* 0XXXX0 */ "DPP/WBMUX",	BIT(5) | BIT(0)},
	{"DPU0", BIT(0),		/* 0XXXX1 */ "SYSMMU_DPU0",	BIT(5) | BIT(0)},

	{"DPU1", 0,			/* 0XXXX0 */ "DPP/WBMUX",	BIT(5) | BIT(0)},
	{"DPU1", BIT(0),		/* 0XXXX1 */ "SYSMMU_DPU1",	BIT(5) | BIT(0)},

	{"DPU2", 0,			/* 0XXXX0 */ "DPP/WBMUX",	BIT(5) | BIT(0)},
	{"DPU2", BIT(0),		/* 0XXXX1 */ "SYSMMU_DPU2",	BIT(5) | BIT(0)},

	{"CAM0", 0,			/* 0XX000 */ "CSISx4",		BIT(5) | GENMASK(2, 0)},
	{"CAM0", BIT(1),		/* 0XX010 */ "TPU0",		BIT(5) | GENMASK(2, 0)},
	{"CAM0", BIT(2),		/* 0XX100 */ "VRA",		BIT(5) | GENMASK(2, 0)},
	{"CAM0", GENMASK(2, 1),		/* 0XX110 */ "TPU1",		BIT(5) | GENMASK(2, 0)},
	{"CAM0", BIT(0),		/* 0XXXX1 */ "SYSMMU_CAM0",	BIT(5) | BIT(0)},

	{"CAM1", 0,			/* 0XXXX0 */ "MC_SCALER",	BIT(5) | BIT(0)},
	{"CAM1", BIT(0),		/* 0XXXX1 */ "SYSMMU_CAM1",	BIT(5) | BIT(0)},

	{"ISPLP", 0,			/* 0XX000 */ "3AAW",		BIT(5) | GENMASK(2, 0)},
	{"ISPLP", BIT(1),		/* 0XX010 */ "ISPLP",		BIT(5) | GENMASK(2, 0)},
	{"ISPLP", BIT(2),		/* 0XX100 */ "ISPHQ",		BIT(5) | GENMASK(2, 0)},
	{"ISPLP", BIT(0),		/* 0XXXX1 */ "SYSMMU_ISPLP",	BIT(5) | BIT(0)},

	{"FSYS0", 0,			/* 0XXX00 */ "ETR",		BIT(5) | GENMASK(1, 0)},
	{"FSYS0", BIT(1),		/* 0XXX10 */ "USB",		BIT(5) | GENMASK(1, 0)},
	{"FSYS0", BIT(0),		/* 0XXXX1 */ "UFS",		BIT(5) | BIT(0)},

	{"MFC0", 0,			/* 0XXXX0 */ "MFC0",		BIT(5) | BIT(0)},
	{"MFC0", BIT(0),		/* 0XXXX1 */ "SYSMMU_MFC0",	BIT(5) | BIT(0)},

	{"MFC1", 0,			/* 0XXXX0 */ "MFC1",		BIT(5) | BIT(0)},
	{"MFC1", BIT(0),		/* 0XXXX1 */ "SYSMMU_MFC1",	BIT(5) | BIT(0)},

	{"G2D0", 0,			/* 0XXXX0 */ "G2D0",		BIT(5) | BIT(0)},
	{"G2D0", BIT(0),		/* 0XXXX1 */ "SYSMMU_G2D0",	BIT(5) | BIT(0)},

	{"G2D1", 0,			/* 0XXXX0 */ "G2D1",		BIT(5) | BIT(0)},
	{"G2D1", BIT(0),		/* 0XXXX1 */ "SYSMMU_G2D1",	BIT(5) | BIT(0)},

	{"G2D2", 0,			/* 0XXX00 */ "JPEG",		BIT(5) | GENMASK(1, 0)},
	{"G2D2", BIT(1),		/* 0XXX10 */ "M2MSCALER",	BIT(5) | GENMASK(1, 0)},
	{"G2D2", BIT(0),		/* 0XXXX1 */ "SYSMMU_G2D2",	BIT(5) | BIT(0)},

	{"IVA", 0,			/* 0XXX00 */ "DSP0",		BIT(5) | GENMASK(1, 0)},
	{"IVA", BIT(1),			/* 0XXX10 */ "DSP1",		BIT(5) | GENMASK(1, 0)},
	{"IVA", BIT(0),			/* 0XXXX1 */ "SYSMMU_DSP",	BIT(5) | BIT(0)},

	{"VPU", 0,			/* 0XXXX0 */ "VPU",		BIT(5) | BIT(0)},
	{"VPU", BIT(0),			/* 0XXXX1 */ "SYSMMU_VPU",	BIT(5) | BIT(0)},

	{"FSYS1", 0,			/* 0XX000 */ "MMC2",		BIT(5) | GENMASK(2, 0)},
	{"FSYS1", BIT(0),		/* 0XX001 */ "PCIE0",		BIT(5) | GENMASK(2, 0)},
	{"FSYS1", BIT(1),		/* 0XX010 */ "PCIE1",		BIT(5) | GENMASK(2, 0)},
	{"FSYS1", GENMASK(1, 0),	/* 0XX011 */ "SSS",		BIT(5) | GENMASK(2, 0)},
	{"FSYS1", BIT(2),		/* 0XX100 */ "RTIC",		BIT(5) | GENMASK(2, 0)},
	{"FSYS1", BIT(2) | BIT(0),	/* 0XX101 */ "MCOMP",		BIT(5) | GENMASK(2, 0)},

	{"AUD", 0,			/* 0XXXX0 */ "SPUS/SPUM/CA7",	BIT(5) | BIT(0)},
	{"AUD", BIT(0),			/* 0XXXX1 */ "SYSMMU_AUC",	BIT(5) | BIT(0)},

	{"GNSS", 0,			/* 0XXX00 */ "CM0+",		BIT(5) | GENMASK(1, 0)},
	{"GNSS", BIT(0),		/* 0XXX01 */ "XDMA0",		BIT(5) | GENMASK(1, 0)},
	{"GNSS", BIT(1),		/* 0XXX10 */ "XDMA1",		BIT(5) | GENMASK(1, 0)},

	{"ALIVE", 0,			/* 0XXX00 */ "CM3",		BIT(5) | GENMASK(1, 0)},
	{"ALIVE", BIT(0),		/* 0XXX01 */ "SCAN2AXI",	BIT(5) | GENMASK(1, 0)},
	{"ALIVE", BIT(1),		/* 0XXX10 */ "CM4F/DMIC",	BIT(5) | GENMASK(1, 0)},

	{"CP_MEM", BIT(3),		/* X01XXX */ "CR7M",		GENMASK(4, 3)},
	{"CP_MEM", BIT(2),		/* X001XX */ "CR4MtoL2",	GENMASK(4, 2)},
	{"CP_MEM", GENMASK(4, 3),	/* X1100X */ "DMA",		GENMASK(4, 1)},
	{"CP_MEM", GENMASK(4, 3) | BIT(1),/* X1101X */ "MDMtoL2",	GENMASK(4, 1)},
	{"CP_MEM", BIT(4),		/* X1000X */ "LMACtoL2",	GENMASK(4, 1)},
	{"CP_MEM", BIT(1),		/* X00010 */ "CSXAP",		GENMASK(4, 0)},
	{"CP_MEM", BIT(4) | GENMASK(1, 0),/* X10011 */ "HARQMOVERtoL2",	GENMASK(4, 0)},

	{"CP_PERI", BIT(5),		/* 1XXXXX */ "CR7M",		BIT(5)},
	{"CP_PERI", BIT(3),		/* 001XXX */ "CR4MtoL2",	GENMASK(5, 3)},
	{"CP_PERI", BIT(4),		/* 0100XX */ "DMA",		GENMASK(5, 2)},
	{"CP_PERI", BIT(4) | BIT(2),	/* 0101XX */ "MDMtoL2",		GENMASK(5, 2)},
	{"CP_PERI", 0,			/* 00000X */ "LMACtoL2",	GENMASK(5, 1)},
	{"CP_PERI", BIT(2),		/* 000100 */ "CSXAP",		GENMASK(5, 0)},
	{"CP_PERI", BIT(2) | BIT(0),	/* 000101 */ "HARQMOVERtoL2",	GENMASK(5, 0)},

	/* Others */
	{"SRDZ", 0, "SRDZ", 0},
	{"DSP",  0, "DSP",  0},
	{"ABOX", 0, "ABOX", 0},
	{"VTS",  0, "VTS",  0},
	{"PDMA", 0, "PDMA", 0},
	{"SPDMA",0, "SPDMA",0},
};

static struct itmon_nodeinfo data_bus_1[] = {
	{M_NODE,	"ALIVE",	0x15423000, NULL, 0,	false, false, true, false},
	{M_NODE,	"FSYS1",	0x15403000, NULL, 0,	false, false, true, false},
	{M_NODE,	"GNSS",		0x15413000, NULL, 0,	false, false, true, false},
	{T_S_NODE,	"BUS1_B0",	0x15433000, NULL, TMOUT, true, false, true, false},
};

static struct itmon_nodeinfo data_bus_c[] = {
	{M_NODE,	"ABOX",		0x15143000, NULL, 0,	false, false, true, false},
	{T_M_NODE,	"BUS1_B0",	0x15003000, NULL, 0,	false, false, true, false},
	{M_NODE,	"CAM0",		0x15043000, NULL, 0,	false, false, true, false},
	{M_NODE,	"CAM1",		0x15053000, NULL, 0,	false, false, true, false},
	{M_NODE,	"CORESIGHT",	0x15113000, NULL, 0,	false, false, true, false},
	{M_NODE,	"DPU0",		0x15013000, NULL, 0,	false, false, true, false},
	{M_NODE,	"DPU1",		0x15023000, NULL, 0,	false, false, true, false},
	{M_NODE,	"DPU2",		0x15033000, NULL, 0,	false, false, true, false},
	{M_NODE,	"DSP",		0x15093000, NULL, 0,	false, false, true, false},
	{M_NODE,	"FSYS0",	0x15103000, NULL, 0,	false, false, true, false},
	{M_NODE,	"G2D0",		0x150D3000, NULL, 0,	false, false, true, false},
	{M_NODE,	"G2D1",		0x150E3000, NULL, 0,	false, false, true, false},
	{M_NODE,	"G2D2",		0x150F3000, NULL, 0,	false, false, true, false},
	{M_NODE,	"ISPLP",	0x15063000, NULL, 0,	false, false, true, false},
	{M_NODE,	"IVA",		0x15083000, NULL, 0,	false, false, true, false},
	{M_NODE,	"MFC0",		0x150B3000, NULL, 0,	false, false, true, false},
	{M_NODE,	"MFC1",		0x150C3000, NULL, 0,	false, false, true, false},
	{M_NODE,	"PDMA",		0x15123000, NULL, 0,	false, false, true, false},
	{M_NODE,	"SPDMA",	0x15133000, NULL, 0,	false, false, true, false},
	{M_NODE,	"SRDZ",		0x15073000, NULL, 0,	false, false, true, false},
	{M_NODE,	"VPU",		0x150A3000, NULL, 0,	false, false, true, false},
	{M_NODE,	"VTS",		0x15153000, NULL, 0,	false, false, true, false},
	{T_S_NODE,	"BUSC_M0",	0x15163000, NULL, 0,	false, false, true, false},
	{T_S_NODE,	"BUSC_M1",	0x15173000, NULL, 0,	false, false, true, false},
	{T_S_NODE,	"BUSC_M2",	0x15183000, NULL, 0,	false, false, true, false},
	{T_S_NODE,	"BUSC_M3",	0x15193000, NULL, 0,	false, false, true, false},
	{S_NODE,	"PERI",		0x151B3000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"SP",		0x151A3000, NULL, TMOUT, true, false, true, false},
};

static struct itmon_nodeinfo data_core[] = {
	{T_M_NODE,	"BUSC_M0",	0x14A03000, NULL, 0,	false, false, true, false},
	{T_M_NODE,	"BUSC_M1",	0x14A13000, NULL, 0,	false, false, true, false},
	{T_M_NODE,	"BUSC_M2",	0x14A23000, NULL, 0,	false, false, true, false},
	{T_M_NODE,	"BUSC_M3",	0x14A33000, NULL, 0,	false, false, true, false},
	{M_NODE,	"CP_MEM",	0x14A83000, NULL, 0,	false, false, true, false},
	{M_NODE,	"G3D0",		0x14A43000, NULL, 0,	false, false, true, false},
	{M_NODE,	"G3D1",		0x14A53000, NULL, 0,	false, false, true, false},
	{M_NODE,	"G3D2",		0x14A63000, NULL, 0,	false, false, true, false},
	{M_NODE,	"G3D3",		0x14A73000, NULL, 0,	false, false, true, false},
	{S_NODE,	"DREX",		0x14A93000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"DREX",		0x14AA3000, NULL, TMOUT, true, false, true, false},
};

static struct itmon_nodeinfo peri_core_0[] = {
	{M_NODE,	"CP_PERI",	0x14C63000, NULL, 0,	false, false, true, false},
	{M_NODE,	"SCI_CCM0",	0x14C33000, NULL, 0,	false, false, true, false},
	{M_NODE,	"SCI_CCM1",	0x14C43000, NULL, 0,	false, false, true, false},
	{M_NODE,	"SCI_IRPM",	0x14C53000, NULL, 0,	false, false, true, false},
	{T_S_NODE,	"CORE0_CORE1",	0x14C03000, NULL, 0,	true, false, true, false},
	{T_S_NODE,	"CORE_BUSC",	0x14C13000, NULL, 0,	true, false, true, false},
};

static struct itmon_nodeinfo peri_core_1[] = {
	{T_M_NODE,	"BUSC_CORE",	0x14E13000, NULL, 0,	false, false, true, false},
	{T_M_NODE,	"CORE0_CORE1",	0x14E03000, NULL, 0,	false, false, true, false},
	{S_NODE,	"CORESIGHT",	0x14E83000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"CORE_SFR",	0x14E33000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"CPUCL0",	0x14E53000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"CPUCL1",	0x14E63000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"G3D",		0x14E73000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"IMEM",		0x14E43000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"TREX_CORE",	0x14E23000, NULL, TMOUT, true, false, true, false},
};

static struct itmon_nodeinfo peri_bus_c[] = {
	{M_NODE,	"BUSC_PERI_M",	0x153A3000, NULL, 0,	false, false, true, false},
	{T_M_NODE,	"CORE_BUSC",	0x153B3000, NULL, 0,	false, false, true, false},
	{S_NODE,	"ABOX",		0x15383000, NULL, TMOUT, true, false, true, false},
	{T_S_NODE,	"BUSC_BUS1",	0x15243000, NULL, 0,	false, false, true, false},
	{T_S_NODE,	"BUSC_CORE",	0x15233000, NULL, 0,	false, false, true, false},
	{S_NODE,	"BUSC_SFR0",	0x15213000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"BUSC_SFR1",	0x15223000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"CAM",		0x152E3000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"DPU0",		0x152C3000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"DPU1",		0x152D3000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"DSP",		0x15363000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"FSYS0",	0x15333000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"G2D",		0x15343000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"ISPHQ",	0x15313000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"ISPLP",	0x15303000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"IVA",		0x15353000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"MFC",		0x15323000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"MIF0",		0x15253000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"MIF1",		0x15263000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"MIF2",		0x15273000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"MIF3",		0x15283000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"PERIC0",	0x152A3000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"PERIC1",	0x152B3000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"PERIS",	0x15293000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"SRDZ",		0x152F3000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"TREX_BUSC",	0x15203000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"VPU",		0x15373000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"VTS",		0x15393000, NULL, TMOUT, true, false, true, false},
};

static struct itmon_nodeinfo peri_bus_1[] = {
	{T_M_NODE,	"BUSC_BUS1",	0x15643000, NULL, 0,	false, false, true, false},
	{S_NODE,	"ALIVE",	0x15633000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"BUS1_SFR",	0x15613000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"FSYS1",	0x15623000, NULL, TMOUT, true, false, true, false},
	{S_NODE,	"TREX_BUS1",	0x15603000, NULL, TMOUT, true, false, true, false},
};

static struct itmon_nodegroup nodegroup[] = {
	{72,	"DATA_BUS_1",	0x155F3000, NULL, data_bus_1,	ARRAY_SIZE(data_bus_1), BUS_DATA},
	{311,	"DATA_CORE",	0x14AF3000, NULL, data_core,	ARRAY_SIZE(data_core),	BUS_DATA},
	{92,	"DATA_BUS_C",	0,	    NULL, data_bus_c,	ARRAY_SIZE(data_bus_c), BUS_DATA},
	{315,	"PERI_CORE_0",	0x14CF3000, NULL, peri_core_0,	ARRAY_SIZE(peri_core_0),BUS_PERI},
	{316,	"PERI_CORE_1",	0x14EF3000, NULL, peri_core_1,	ARRAY_SIZE(peri_core_1),BUS_PERI},
	{93,	"PERI_BUS_C",	0x153F3000, NULL, peri_bus_c,	ARRAY_SIZE(peri_bus_c), BUS_PERI},
	{77,	"PERI_BUS_1",	0x156F3000, NULL, peri_bus_1,	ARRAY_SIZE(peri_bus_1), BUS_PERI},
};

struct itmon_dev {
	struct device *dev;
	struct itmon_platdata *pdata;
	struct of_device_id *match;
	int irq;
	int id;
	void __iomem *regs;
	spinlock_t ctrl_lock;
	struct itmon_notifier notifier_info;
};

struct itmon_panic_block {
	struct notifier_block nb_panic_block;
	struct itmon_dev *pdev;
};

/* declare notifier_list */
static ATOMIC_NOTIFIER_HEAD(itmon_notifier_list);

static const struct of_device_id itmon_dt_match[] = {
	{.compatible = "samsung,exynos-itmon",
	 .data = NULL,},
	{},
};
MODULE_DEVICE_TABLE(of, itmon_dt_match);

static struct itmon_rpathinfo *itmon_get_rpathinfo(struct itmon_dev *itmon,
					       unsigned int id,
					       char *dest_name)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_rpathinfo *rpath = NULL;
	int i;

	if (!dest_name)
		return NULL;

	for (i = 0; i < ARRAY_SIZE(rpathinfo); i++) {
		if (pdata->rpathinfo[i].id == (id & pdata->rpathinfo[i].bits)) {
			if (dest_name && !strncmp(pdata->rpathinfo[i].dest_name,
						  dest_name,
						  strlen(pdata->rpathinfo[i].dest_name))) {
				rpath = (struct itmon_rpathinfo *)&pdata->rpathinfo[i];
				break;
			}
		}
	}
	return rpath;
}

static struct itmon_masterinfo *itmon_get_masterinfo(struct itmon_dev *itmon,
						 char *port_name,
						 unsigned int user)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_masterinfo *master = NULL;
	unsigned int val;
	int i;

	if (!port_name)
		return NULL;

	for (i = 0; i < ARRAY_SIZE(masterinfo); i++) {
		if (!strncmp(pdata->masterinfo[i].port_name, port_name, strlen(port_name))) {
			val = user & pdata->masterinfo[i].bits;
			if (val == pdata->masterinfo[i].user) {
				master = (struct itmon_masterinfo *)&pdata->masterinfo[i];
				break;
			}
		}
	}
	return master;
}

static void itmon_init(struct itmon_dev *itmon, bool enabled)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *node;
	unsigned int offset;
	int i, j;

	for (i = 0; i < ARRAY_SIZE(nodegroup); i++) {
		node = pdata->nodegroup[i].nodeinfo;
		for (j = 0; j < pdata->nodegroup[i].nodesize; j++) {
			if (node[j].type == S_NODE && node[j].tmout_enabled) {
				offset = OFFSET_TMOUT_REG;
				/* Enable Timeout setting */
				__raw_writel(enabled, node[j].regs + offset + REG_DBG_CTL);
				/* set tmout interval value */
				__raw_writel(node[j].time_val,
					     node[j].regs + offset + REG_TMOUT_INIT_VAL);
				pr_debug("Exynos IPM - %s timeout enabled\n", node[j].name);
				if (node[j].tmout_frz_enabled) {
					/* Enable freezing */
					__raw_writel(enabled,
						     node[j].regs + offset + REG_TMOUT_FRZ_EN);
				}
			}
			if (node[j].err_enabled) {
				/* clear previous interrupt of req_read */
				offset = OFFSET_REQ_R;
				if (!pdata->probed || !node->retention)
					__raw_writel(1, node[j].regs + offset + REG_INT_CLR);
				/* enable interrupt */
				__raw_writel(enabled, node[j].regs + offset + REG_INT_MASK);

				/* clear previous interrupt of req_write */
				offset = OFFSET_REQ_W;
				if (pdata->probed || !node->retention)
					__raw_writel(1, node[j].regs + offset + REG_INT_CLR);
				/* enable interrupt */
				__raw_writel(enabled, node[j].regs + offset + REG_INT_MASK);

				/* clear previous interrupt of response_read */
				offset = OFFSET_RESP_R;
				if (!pdata->probed || !node->retention)
					__raw_writel(1, node[j].regs + offset + REG_INT_CLR);
				/* enable interrupt */
				__raw_writel(enabled, node[j].regs + offset + REG_INT_MASK);

				/* clear previous interrupt of response_write */
				offset = OFFSET_RESP_W;
				if (!pdata->probed || !node->retention)
					__raw_writel(1, node[j].regs + offset + REG_INT_CLR);
				/* enable interrupt */
				__raw_writel(enabled, node[j].regs + offset + REG_INT_MASK);
				pr_debug("Exynos IPM - %s error reporting enabled\n", node[j].name);
			}
		}
	}
}

void itmon_enable(bool enabled)
{
	if (g_itmon) {
		itmon_init(g_itmon, enabled);
	}
}

void itmon_set_errcnt(int cnt)
{
	struct itmon_platdata *pdata;
	if (g_itmon) {
		pdata = g_itmon->pdata;
		pdata->err_cnt = cnt;
	}
}

static void itmon_post_handler_to_notifier(struct itmon_dev *itmon,
					   unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];

	/* After treatment by port */
	if (!traceinfo->port || strlen(traceinfo->port) < 1)
		return;

	itmon->notifier_info.port = traceinfo->port;
	itmon->notifier_info.master = traceinfo->master;
	itmon->notifier_info.dest = traceinfo->dest;
	itmon->notifier_info.read = traceinfo->read;
	itmon->notifier_info.target_addr = traceinfo->target_addr;
	itmon->notifier_info.errcode = traceinfo->errcode;

	/* call notifier_call_chain of itmon */
	atomic_notifier_call_chain(&itmon_notifier_list, 0, &itmon->notifier_info);
}

static void itmon_post_handler_by_master(struct itmon_dev *itmon,
					unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];

	/* After treatment by port */
	if (!traceinfo->port || strlen(traceinfo->port) < 1)
		return;

	if (!strncmp(traceinfo->port, "CPU", strlen("CPU"))) {
		/* if master is CPU, then we expect any exception */
		if (pdata->err_cnt > PANIC_ALLOWED_THRESHOLD) {
			pdata->err_cnt = 0;
			itmon_init(itmon, false);
			pr_info("ITMON is turn-off when CPU transaction is detected repeatly\n");
		} else {
			pr_info("ITMON skips CPU transaction detected\n");
		}
	} else if (!strncmp(traceinfo->port, CP_COMMON_STR, strlen(CP_COMMON_STR))) {
		/* if master is DSP and operation is read, we don't care this */
		if (traceinfo->master && traceinfo->target_addr == INVALID_REMAPPING &&
			!strncmp(traceinfo->master, "CR4MtoL2", strlen(traceinfo->master))) {
			pdata->err_cnt = 0;
			pr_info("ITMON skips CP's DSP(CR4MtoL2) detected\n");
		} else {
			/* Disable busmon all interrupts */
			itmon_init(itmon, false);
#if defined(CONFIG_SEC_SIPC_MODEM_IF)
			pdata->crash_in_progress = true;
			ss310ap_force_crash_exit_ext();
#endif
		}
	}
}

static void itmon_report_timeout(struct itmon_dev *itmon,
				struct itmon_nodeinfo *node,
				unsigned int trans_type)
{
	unsigned int id, payload, axid, user, valid, timeout;
	char *master_name, *port_name;
	struct itmon_rpathinfo *port;
	struct itmon_masterinfo *master;
	int i, num = (trans_type == TRANS_TYPE_READ ? SZ_128 : SZ_64);
	int fz_offset = (trans_type == TRANS_TYPE_READ ? 0 : REG_TMOUT_BUF_WR_OFFSET);

	pr_info("      > NUM|   BLOCK|  MASTER|   VALID| TIMEOUT|      ID| PAYLOAD|\n");

	for (i = 0; i < num; i++) {
		writel(i, node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_RD_ADDR + fz_offset);
		id = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_RD_ID + fz_offset);
		payload = readl(node->regs + OFFSET_TMOUT_REG +
				REG_TMOUT_BUF_RD_PAYLOAD + fz_offset);

		/* ID[5:0] 6bit : R-PATH */
		axid = id & GENMASK(5, 0);
		/* PAYLOAD[15:8] : USER */
		user = (payload & GENMASK(15, 8)) >> 8;
		/* PAYLOAD[0] : Valid or Not valid */
		valid = payload & BIT(0);
		/* PAYLOAD[19:16] : Timeout */
		timeout = (payload & GENMASK(19, 16)) >> 16;

		port = (struct itmon_rpathinfo *)
				itmon_get_rpathinfo(itmon, axid, "DREX");
		if (port) {
			port_name = port->port_name;
			master = (struct itmon_masterinfo *)
				itmon_get_masterinfo(itmon, port_name, user);
			if (master)
				master_name = master->master_name;
			else
				master_name = "Unknown";
		} else {
			port_name = "Unknown";
			master_name = "Unknown";
		}
		pr_info("      > %03d|%8s|%8s|%8u|%8x|%08x|%08x|\n",
				i, port_name, master_name, valid, timeout, id, payload);
	}
	pr_info("--------------------------------------------------------------------------\n");
}

static unsigned int power(unsigned int param, unsigned int num)
{
	if (num == 0)
		return 1;
	return param * (power(param, num - 1));
}

static void itmon_report_traceinfo(struct itmon_dev *itmon,
				struct itmon_nodeinfo *node,
				unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];
	struct itmon_nodegroup *group = NULL;
#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
	char temp_buf[SZ_128];
#endif

	if (!traceinfo->dirty)
		return;

	pr_auto(ASL3, 
		"--------------------------------------------------------------------------\n"
		"      Transaction Information\n\n"
		"      > Master         : %s %s\n"
		"      > Target         : %s\n"
		"      > Target Address : 0x%lX %s\n"
		"      > Type           : %s\n"
		"      > Error code     : %s\n",
		traceinfo->port, traceinfo->master ? traceinfo->master : "",
		traceinfo->dest ? traceinfo->dest : "Unknown",
		traceinfo->target_addr,
		(unsigned int)traceinfo->target_addr == INVALID_REMAPPING ?
		"(Violation Access by CP maybe)" : "",
		trans_type == TRANS_TYPE_READ ? "READ" : "WRITE",
		itmon_errcode[traceinfo->errcode]);

#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
	snprintf(temp_buf, SZ_128, "%s %s/ %s/ 0x%zx %s/ %s/ %s",
		traceinfo->port, traceinfo->master ? traceinfo->master : "",
		traceinfo->dest ? traceinfo->dest : "Unknown",
		traceinfo->target_addr,
		traceinfo->target_addr == INVALID_REMAPPING ?
		"(by CP maybe)" : "",
		trans_type == TRANS_TYPE_READ ? "READ" : "WRITE",
		itmon_errcode[traceinfo->errcode]);
	sec_debug_set_extra_info_busmon(temp_buf);
#endif

	if (node) {
		struct itmon_tracedata *tracedata = &node->tracedata;

		pr_auto(ASL3, 
			" > Size           : %u bytes x %u burst => %u bytes\n"
			"      > Burst Type     : %u (0:FIXED, 1:INCR, 2:WRAP)\n"
			"      > Level          : %s\n"
			"      > Protection     : %s\n",
			power(BIT_AXSIZE(tracedata->ext_info_1), 2), BIT_AXLEN(tracedata->ext_info_1) + 1,
			power(BIT_AXSIZE(tracedata->ext_info_1), 2) * (BIT_AXLEN(tracedata->ext_info_1) + 1),
			BIT_AXBURST(tracedata->ext_info_2),
			(BIT_AXPROT(tracedata->ext_info_2) & 0x1) ? "Privileged access" : "Unprivileged access",
			(BIT_AXPROT(tracedata->ext_info_2) & 0x2) ? "Non-secure access" : "Secure access");

		group = node->group;
		pr_auto(ASL3, " > Path Type      : %s\n"
			"--------------------------------------------------------------------------\n",
			itmon_pathtype[group->bus_type]);

	} else {
		pr_auto(ASL3, "--------------------------------------------------------------------------\n");
	}
}

static void itmon_report_pathinfo(struct itmon_dev *itmon,
				  struct itmon_nodeinfo *node,
				  unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_tracedata *tracedata = &node->tracedata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];

	if (!traceinfo->path_dirty) {
		pr_auto(ASL3,
			"--------------------------------------------------------------------------\n"
			"      ITMON Report (%s)\n"
			"--------------------------------------------------------------------------\n"
			"      PATH Information\n",
			trans_type == TRANS_TYPE_READ ? "READ" : "WRITE");
		traceinfo->path_dirty = true;
	}
	switch (node->type) {
	case M_NODE:
		pr_auto(ASL3, " > %14s, %8s(0x%08X)\n",
			node->name, "M_NODE", node->phy_regs + tracedata->offset);
		break;
	case T_S_NODE:
		pr_auto(ASL3, " > %14s, %8s(0x%08X)\n",
			node->name, "T_S_NODE", node->phy_regs + tracedata->offset);
		break;
	case T_M_NODE:
		pr_auto(ASL3, " > %14s, %8s(0x%08X)\n",
			node->name, "T_M_NODE", node->phy_regs + tracedata->offset);
		break;
	case S_NODE:
		pr_auto(ASL3, " > %14s, %8s(0x%08X)\n",
			node->name, "S_NODE", node->phy_regs + tracedata->offset);
		break;
	}
}

static void itmon_report_tracedata(struct itmon_dev *itmon,
				   struct itmon_nodeinfo *node,
				   unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_tracedata *tracedata = &node->tracedata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];
	struct itmon_masterinfo *master;
	struct itmon_rpathinfo *port;
	unsigned int errcode, axid;
	unsigned long userbit;

	errcode = BIT_ERR_CODE(tracedata->int_info);
	axid = BIT_AXID(tracedata->int_info);
	userbit = BIT_AXUSER(tracedata->ext_info_2);

	switch (node->type) {
	case M_NODE:
		/* In this case, we can get information from M_NODE
		 * Fill traceinfo->port / target_addr / read / master */
		if (BIT_ERR_VALID(tracedata->int_info) && tracedata->ext_info_2) {
			/* If only detecting M_NODE only */
			if (!strncmp(node->name, "SCI_IRPM", strlen(node->name))) {
				int cluster_num, core_num;
				core_num = axid & GENMASK(1, 0);
				cluster_num = (axid & BIT(2)) >> 2;
				snprintf(traceinfo->buf, SZ_32 - 1, "CPU%d Cluster%d", core_num, cluster_num);
				traceinfo->port = traceinfo->buf;
			} else {
				traceinfo->port = node->name;
				master = (struct itmon_masterinfo *)
					itmon_get_masterinfo(itmon, node->name, userbit);
				if (master)
					traceinfo->master = master->master_name;
				else
					traceinfo->master = NULL;
			}
			traceinfo->target_addr =
				(((unsigned long)node->tracedata.ext_info_1
				& GENMASK(3, 0)) << 32ULL);
			traceinfo->target_addr |= node->tracedata.ext_info_0;
			traceinfo->read = tracedata->read;
			traceinfo->errcode = errcode;
			traceinfo->dirty = true;
		}
		/* If we excepts following M_NODE with S_NODE */
		if (!strncmp(node->name, "SCI_IRPM", strlen(node->name))) {
			set_bit(FROM_CPU, &traceinfo->from);
		} else if (!strncmp(node->name, "SCI_CCM0", strlen(node->name)) ||
			!strncmp(node->name, "SCI_CCM1", strlen(node->name))) {
			set_bit(FROM_CPU_MAY, &traceinfo->from);
		} else if (!strncmp(node->name, CP_MEM_STR, strlen(node->name))) {
			set_bit(FROM_CP_MEM, &traceinfo->from);
		} else if (!strncmp(node->name, CP_PERI_STR, strlen(node->name))) {
			set_bit(FROM_CP_PERI, &traceinfo->from);
		} else if (!strncmp(node->name, "BUSC_PERI_M", strlen(node->name))) {
			set_bit(FROM_M_NODE, &traceinfo->from);
		}
		/* Pure M_NODE and it doesn't have any information */
		if (!traceinfo->dirty) {
			traceinfo->master = NULL;
			traceinfo->target_addr = 0;
			traceinfo->read = tracedata->read;
			traceinfo->port = node->name;
			traceinfo->errcode = errcode;
			traceinfo->dirty = true;
		}
		itmon_report_pathinfo(itmon, node, trans_type);
		break;
	case S_NODE:
		if (test_bit(FROM_CPU, &traceinfo->from)) {
			/*
			 * This case is only for PERI Path
			 * Master is CPU cluster
			 * user & GENMASK(1, 0) = core number
			 */
			int cluster_num, core_num;
			core_num = axid & GENMASK(1, 0);
			cluster_num = (axid & BIT(2)) >> 2;
			snprintf(traceinfo->buf, SZ_32 - 1, "CPU%d Cluster%d", core_num, cluster_num);
			traceinfo->port = traceinfo->buf;
		} else if (test_bit(FROM_CPU_MAY, &traceinfo->from)) {
			snprintf(traceinfo->buf, SZ_32 - 1, "CPU maybe");
			traceinfo->port = traceinfo->buf;
		} else {
			/*
			* we expect traceinfo->port is always true in this case
			* In DECERR case, the follow information was filled.
			*/
			if (test_bit(FROM_CP_MEM, &traceinfo->from) && (!traceinfo->port))
				traceinfo->port = CP_MEM_STR;
			else if (test_bit(FROM_CP_PERI, &traceinfo->from) && (!traceinfo->port))
				traceinfo->port = CP_PERI_STR;

			/* In DREX Path, it should parsed again to find the master */
			if (!strncmp(node->name, "DREX", strlen(node->name))) {
				port = (struct itmon_rpathinfo *)
					itmon_get_rpathinfo(itmon, axid, node->name);
				/* If it couldn't find port, keep previous information */
				if (port) {
					traceinfo->port = port->port_name;
					master = (struct itmon_masterinfo *)
						itmon_get_masterinfo(itmon, traceinfo->port,
							userbit & GENMASK(2, 0));
					if (master)
						traceinfo->master = master->master_name;
				}
			} else {
				/* If it has traceinfo->port, keep previous information */
				if (!traceinfo->port) {
					port = (struct itmon_rpathinfo *)
						itmon_get_rpathinfo(itmon, axid, node->name);
					if (port)
						traceinfo->port = port->port_name;
				}
				if (!traceinfo->master && traceinfo->port) {
					master = (struct itmon_masterinfo *)
					itmon_get_masterinfo(itmon, traceinfo->port,
								userbit & GENMASK(2, 0));
					if (master)
						traceinfo->master = master->master_name;
				}
			}
		}
		traceinfo->target_addr =
			(((unsigned long)node->tracedata.ext_info_1
			& GENMASK(3, 0)) << 32ULL);
		traceinfo->target_addr |= node->tracedata.ext_info_0;
		traceinfo->errcode = errcode;
		traceinfo->dest = node->name;
		traceinfo->snode_dirty = true;
		itmon_report_pathinfo(itmon, node, trans_type);
		itmon_report_traceinfo(itmon, node, trans_type);
		break;
	case T_S_NODE:
	case T_M_NODE:
		itmon_report_pathinfo(itmon, node, trans_type);
		break;
	default:
		pr_info("Unknown Error - offset:%u\n", tracedata->offset);
		break;
	}
}

static void itmon_report_rawdata(struct itmon_dev *itmon,
				 struct itmon_nodeinfo *node,
				 unsigned int trans_type)

{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo = &pdata->traceinfo[trans_type];
	struct itmon_tracedata *tracedata = &node->tracedata;

	/* Output Raw register information */
	pr_info("      > %s(%s, 0x%08X)\n"
		"      > REG(0x08~0x18) : 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
		node->name, itmon_nodestring[node->type],
		node->phy_regs + tracedata->offset,
		tracedata->int_info,
		tracedata->ext_info_0,
		tracedata->ext_info_1,
		tracedata->ext_info_2);

	/* If node is to DREX S_NODE, Outputing timeout freezing result */
	if (!strncmp(node->name, "DREX", strlen(node->name)) && node->type == S_NODE
		&& traceinfo->errcode == ERRCODE_TMOUT)
		itmon_report_timeout(itmon, node, trans_type);
}

static void itmon_route_tracedata(struct itmon_dev *itmon)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *traceinfo;
	struct itmon_nodeinfo *node, *next_node;
	unsigned int trans_type;
	int i;

	/* To call function is sorted by declaration */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		for (i = M_NODE; i < NODE_TYPE; i++) {
			list_for_each_entry(node, &pdata->tracelist[trans_type], list) {
				if (i == node->type)
					itmon_report_tracedata(itmon, node, trans_type);
			}
		}
		/* If there is no S_NODE information, check one more */
		traceinfo = &pdata->traceinfo[trans_type];
		if (!traceinfo->snode_dirty)
			itmon_report_traceinfo(itmon, NULL, trans_type);
	}

	if (pdata->traceinfo[TRANS_TYPE_READ].dirty ||
		pdata->traceinfo[TRANS_TYPE_WRITE].dirty)
		pr_auto(ASL3, " Raw Register Information(ITMON Internal Information)\n\n");

	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		for (i = M_NODE; i < NODE_TYPE; i++) {
			list_for_each_entry_safe(node, next_node, &pdata->tracelist[trans_type], list) {
				if (i == node->type) {
					itmon_report_rawdata(itmon, node, trans_type);
					/* clean up */
					list_del(&node->list);
					kfree(node);
				}
			}
		}
	}

	if (pdata->traceinfo[TRANS_TYPE_READ].dirty ||
		pdata->traceinfo[TRANS_TYPE_WRITE].dirty)
		pr_auto(ASL3, "--------------------------------------------------------------------------\n");

	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		itmon_post_handler_to_notifier(itmon, trans_type);
		itmon_post_handler_by_master(itmon, trans_type);
	}
}

static void itmon_trace_data(struct itmon_dev *itmon,
			    struct itmon_nodegroup *group,
			    struct itmon_nodeinfo *node,
			    unsigned int offset)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *new_node = NULL;
	unsigned int int_info, info0, info1, info2;
	bool read = TRANS_TYPE_WRITE;
	bool req = false;

	int_info = __raw_readl(node->regs + offset + REG_INT_INFO);
	info0 = __raw_readl(node->regs + offset + REG_EXT_INFO_0);
	info1 = __raw_readl(node->regs + offset + REG_EXT_INFO_1);
	info2 = __raw_readl(node->regs + offset + REG_EXT_INFO_2);

	switch (offset) {
	case OFFSET_REQ_R:
		read = TRANS_TYPE_READ;
		/* fall down */
	case OFFSET_REQ_W:
		req = true;
		/* Only S-Node is able to make log to registers */
		break;
	case OFFSET_RESP_R:
		read = TRANS_TYPE_READ;
		/* fall down */
	case OFFSET_RESP_W:
		req = false;
		/* Only NOT S-Node is able to make log to registers */
		break;
	default:
		pr_auto(ASL3, "Unknown Error - node:%s offset:%u\n", node->name, offset);
		break;
	}

	new_node = kmalloc(sizeof(struct itmon_nodeinfo), GFP_ATOMIC);
	if (new_node) {
		/* Fill detected node information to tracedata's list */
		memcpy(new_node, node, sizeof(struct itmon_nodeinfo));
		new_node->tracedata.int_info = int_info;
		new_node->tracedata.ext_info_0 = info0;
		new_node->tracedata.ext_info_1 = info1;
		new_node->tracedata.ext_info_2 = info2;
		new_node->tracedata.offset = offset;
		new_node->tracedata.read = read;
		new_node->group = group;
		if (BIT_ERR_VALID(int_info))
			node->tracedata.logging = true;
		else
			node->tracedata.logging = false;

		list_add(&new_node->list, &pdata->tracelist[read]);
	} else {
		pr_auto(ASL3, "failed to kmalloc for %s node %x offset\n",
			node->name, offset);
	}
}

static int itmon_search_node(struct itmon_dev *itmon, struct itmon_nodegroup *group, bool clear)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *node = NULL;
	unsigned int val, offset;
	unsigned long vec, flags, bit = 0;
	int i, j, ret = 0;

	spin_lock_irqsave(&itmon->ctrl_lock, flags);
	memset(pdata->traceinfo, 0, sizeof(struct itmon_traceinfo) * 2);
	if (group) {
		/* Processing only this group and select detected node */
		vec = (unsigned long)__raw_readl(group->regs);
		node = group->nodeinfo;
		if (!vec)
			goto exit;

		for_each_set_bit(bit, &vec, group->nodesize) {
			/* exist array */
			for (i = 0; i < OFFSET_NUM; i++) {
				offset = i * OFFSET_ERR_REPT;
				/* Check Request information */
				val = __raw_readl(node[bit].regs + offset + REG_INT_INFO);
				if (BIT_ERR_OCCURRED(val)) {
					/* This node occurs the error */
					itmon_trace_data(itmon, group, &node[bit], offset);
					if (clear)
						__raw_writel(1, node[bit].regs
								+ offset + REG_INT_CLR);
					ret = true;
				}
			}

		}
	} else {
		/* Processing all group & nodes */
		for (i = 0; i < ARRAY_SIZE(nodegroup); i++) {
			group = &nodegroup[i];
			if (group->phy_regs)
				vec = (unsigned long)__raw_readl(group->regs);
			else
				vec = GENMASK(group->nodesize, 0);

			node = group->nodeinfo;
			bit = 0;

			for_each_set_bit(bit, &vec, group->nodesize) {
				for (j = 0; j < OFFSET_NUM; j++) {
					offset = j * OFFSET_ERR_REPT;
					/* Check Request information */
					val = __raw_readl(node[bit].regs + offset + REG_INT_INFO);
					if (BIT_ERR_OCCURRED(val)) {
						/* This node occurs the error */
						itmon_trace_data(itmon, group, &node[bit], offset);
						if (clear)
							__raw_writel(1, node[bit].regs
									+ offset + REG_INT_CLR);
						ret = true;
					}
				}
			}
		}
	}
	itmon_route_tracedata(itmon);
 exit:
	spin_unlock_irqrestore(&itmon->ctrl_lock, flags);
	return ret;
}

static irqreturn_t itmon_irq_handler(int irq, void *data)
{
	struct itmon_dev *itmon = (struct itmon_dev *)data;
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodegroup *group = NULL;
	bool ret;
	int i;

	/* Search itmon group */
	for (i = 0; i < ARRAY_SIZE(nodegroup); i++) {
		if (irq == nodegroup[i].irq) {
			group = &pdata->nodegroup[i];
			if (group->phy_regs != 0) {
				pr_info("\nITMON Detected: %d irq, %s group, 0x%x vec, err_cnt:%u\n",
					irq, group->name, __raw_readl(group->regs), pdata->err_cnt);
			} else {
				pr_info("\nITMON Detected: %d irq, %s group, err_cnt:%u\n",
					irq, group->name, pdata->err_cnt);
			}
			break;
		}
	}

	ret = itmon_search_node(itmon, NULL, true);
	if (!ret) {
		pr_info("ITMON could not detect any error\n");
	} else {
		if (pdata->err_cnt++ > PANIC_ALLOWED_THRESHOLD)
			pdata->panic_allowed = true;
	}

	if (pdata->panic_allowed)
		panic("ITMON occurs panic to prevent endless log");

	return IRQ_HANDLED;
}

void itmon_notifier_chain_register(struct notifier_block *block)
{
	atomic_notifier_chain_register(&itmon_notifier_list, block);
}

static int itmon_logging_panic_handler(struct notifier_block *nb,
				     unsigned long l, void *buf)
{
	struct itmon_panic_block *itmon_panic = (struct itmon_panic_block *)nb;
	struct itmon_dev *itmon = itmon_panic->pdev;
	int ret;

	if (!IS_ERR_OR_NULL(itmon)) {
		/* Check error has been logged */
		ret = itmon_search_node(itmon, NULL, false);
		if (!ret)
			pr_info("No found error in %s\n", __func__);
		else
			pr_info("Found errors in %s\n", __func__);
	}
	return 0;
}

static int itmon_probe(struct platform_device *pdev)
{
	struct itmon_dev *itmon;
	struct itmon_panic_block *itmon_panic = NULL;
	struct itmon_platdata *pdata;
	struct itmon_nodeinfo *node;
	unsigned int irq_option = 0, irq;
	char *dev_name;
	int ret, i, j;

	itmon = devm_kzalloc(&pdev->dev, sizeof(struct itmon_dev), GFP_KERNEL);
	if (!itmon) {
		dev_err(&pdev->dev, "failed to allocate memory for driver's "
				    "private data\n");
		return -ENOMEM;
	}
	itmon->dev = &pdev->dev;

	spin_lock_init(&itmon->ctrl_lock);

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct itmon_platdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(&pdev->dev, "failed to allocate memory for driver's "
				    "platform data\n");
		return -ENOMEM;
	}
	itmon->pdata = pdata;
	itmon->pdata->masterinfo = masterinfo;
	itmon->pdata->rpathinfo = rpathinfo;
	itmon->pdata->nodegroup = nodegroup;

	for (i = 0; i < ARRAY_SIZE(nodegroup); i++) {
		dev_name = nodegroup[i].name;
		node = nodegroup[i].nodeinfo;

		if (nodegroup[i].phy_regs) {
			nodegroup[i].regs = devm_ioremap_nocache(&pdev->dev,
							 nodegroup[i].phy_regs, SZ_16K);
			if (nodegroup[i].regs == NULL) {
				dev_err(&pdev->dev, "failed to claim register region - %s\n",
					dev_name);
				return -ENOENT;
			}
		}

		if (initial_multi_irq_enable)
			irq_option = IRQF_GIC_MULTI_TARGET;

		irq = irq_of_parse_and_map(pdev->dev.of_node, i);
		nodegroup[i].irq = irq;

		ret = devm_request_irq(&pdev->dev, irq,
				       itmon_irq_handler, irq_option, dev_name, itmon);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request irq - %s\n", dev_name);
			return -ENOENT;
		} else {
			dev_err(&pdev->dev, "success to register request irq%u - %s\n", irq, dev_name);
		}

		for (j = 0; j < nodegroup[i].nodesize; j++) {
			node[j].regs = devm_ioremap_nocache(&pdev->dev, node[j].phy_regs, SZ_16K);
			if (node[j].regs == NULL) {
				dev_err(&pdev->dev, "failed to claim register region - %s\n",
					dev_name);
				return -ENOENT;
			}
		}
	}

	itmon_panic = devm_kzalloc(&pdev->dev, sizeof(struct itmon_panic_block),
				 GFP_KERNEL);

	if (!itmon_panic) {
		dev_err(&pdev->dev, "failed to allocate memory for driver's "
				    "panic handler data\n");
	} else {
		itmon_panic->nb_panic_block.notifier_call = itmon_logging_panic_handler;
		itmon_panic->pdev = itmon;
		atomic_notifier_chain_register(&panic_notifier_list,
					       &itmon_panic->nb_panic_block);
	}

	platform_set_drvdata(pdev, itmon);

	INIT_LIST_HEAD(&pdata->tracelist[BUS_DATA]);
	INIT_LIST_HEAD(&pdata->tracelist[BUS_PERI]);

	pdata->crash_in_progress = false;
	itmon_init(itmon, true);

	g_itmon = itmon;
	pdata->probed = true;

	dev_info(&pdev->dev, "success to probe Exynos ITMON driver\n");

	return 0;
}

static int itmon_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int itmon_suspend(struct device *dev)
{
	return 0;
}

static int itmon_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct itmon_dev *itmon = platform_get_drvdata(pdev);
	struct itmon_platdata *pdata = itmon->pdata;

	/* re-enable ITMON if cp-crash progress is not starting */
	if (!pdata->crash_in_progress)
		itmon_init(itmon, true);

	return 0;
}

static SIMPLE_DEV_PM_OPS(itmon_pm_ops, itmon_suspend, itmon_resume);
#define ITMON_PM	(itmon_pm_ops)
#else
#define ITM_ONPM	NULL
#endif

static struct platform_driver exynos_itmon_driver = {
	.probe = itmon_probe,
	.remove = itmon_remove,
	.driver = {
		   .name = "exynos-itmon",
		   .of_match_table = itmon_dt_match,
		   .pm = &itmon_pm_ops,
		   },
};

module_platform_driver(exynos_itmon_driver);

MODULE_DESCRIPTION("Samsung Exynos ITMON DRIVER");
MODULE_AUTHOR("Hosung Kim <hosung0.kim@samsung.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:exynos-itmon");
