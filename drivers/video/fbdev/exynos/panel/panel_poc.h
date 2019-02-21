/*
 * linux/drivers/video/fbdev/exynos/panel/panel_misc.h
 *
 * Samsung Common LCD Driver.
 *
 * Copyright (c) 2017 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_POC_H__
#define __PANEL_POC_H__

#include <linux/types.h>
#include <linux/kernel.h>

#include "panel.h"
#include "panel_drv.h"
#include "panel_poc.h"
#ifdef CONFIG_PANEL_POC_2_0
#define POC_IMG_SIZE	(546008)
#else
#define POC_IMG_SIZE	(532816)
#endif
#define POC_IMG_ADDR	(0x000000)
#define POC_PAGE		(4096)
#define POC_TEST_PATTERN_SIZE	(4096)
#define POC_IMG_WR_SIZE	(POC_IMG_SIZE)
#define POC_IMG_PATH "/data/poc_img"
#define POC_CHKSUM_DATA_LEN		(4)
#define POC_CHKSUM_RES_LEN		(1)
#define POC_CHKSUM_LEN		(POC_CHKSUM_DATA_LEN + POC_CHKSUM_RES_LEN)
#ifdef CONFIG_DISPLAY_USE_INFO
#ifdef CONFIG_SEC_FACTORY
#define POC_TOTAL_TRY_COUNT_FILE_PATH	("/efs/FactoryApp/poc_totaltrycount")
#define POC_TOTAL_FAIL_COUNT_FILE_PATH	("/efs/FactoryApp/poc_totalfailcount")
#else
#define POC_TOTAL_TRY_COUNT_FILE_PATH	("/efs/etc/poc/totaltrycount")
#define POC_TOTAL_FAIL_COUNT_FILE_PATH	("/efs/etc/poc/totalfailcount")
#endif
#define POC_INFO_FILE_PATH	("/efs/FactoryApp/poc_info")
#define POC_USER_FILE_PATH	("/efs/FactoryApp/poc_user")
#endif

#if defined(CONFIG_POC_DREAM)
#define ERASE_WAIT_COUNT	(180)
#define WR_DONE_UDELAY		(4000)
#define QD_DONE_MDELAY		(30)
#define RD_DONE_UDELAY		(200)
#elif defined(CONFIG_POC_DREAM2)
#define ERASE_WAIT_COUNT	(41)
#define WR_DONE_UDELAY		(800)
#define QD_DONE_MDELAY		(10)
#define RD_DONE_UDELAY		(200)
#else
#define ERASE_WAIT_COUNT	(180)
#define WR_DONE_UDELAY		(4000)
#define QD_DONE_MDELAY		(30)
#define RD_DONE_UDELAY		(200)
#endif

enum poc_flash_state {
	POC_FLASH_STATE_UNKNOWN = -1,
	POC_FLASH_STATE_NOT_USE = 0,
	POC_FLASH_STATE_USE = 1,
	MAX_POC_FLASH_STATE,
};

enum {
	POC_OP_NONE = 0,
	POC_OP_ERASE = 1,
	POC_OP_WRITE = 2,
	POC_OP_READ = 3,
	POC_OP_CANCEL = 4,
	POC_OP_CHECKSUM = 5,
	POC_OP_CHECKPOC = 6,
	POC_OP_BACKUP = 7,
	POC_OP_SELF_TEST = 8,
	POC_OP_READ_TEST = 9,
	POC_OP_WRITE_TEST = 10,
	MAX_POC_OP,
};

#define IS_VALID_POC_OP(_op_)	\
	((_op_) > POC_OP_NONE && (_op_) < MAX_POC_OP)

#define IS_POC_OP_ACCESS_FLASH(_op_)	\
	((_op_) == POC_OP_ERASE ||\
	 (_op_) == POC_OP_WRITE ||\
	 (_op_) == POC_OP_READ ||\
	 (_op_) == POC_OP_BACKUP ||\
	 (_op_) == POC_OP_SELF_TEST)

enum poc_state {
	POC_STATE_NONE,
	POC_STATE_FLASH_EMPTY,
	POC_STATE_FLASH_FILLED,
	POC_STATE_ER_START,
	POC_STATE_ER_PROGRESS,
	POC_STATE_ER_COMPLETE,
	POC_STATE_ER_FAILED,
	POC_STATE_WR_START,
	POC_STATE_WR_PROGRESS,
	POC_STATE_WR_COMPLETE,
	POC_STATE_WR_FAILED,
	MAX_POC_STATE,
};

struct panel_poc_info {
	bool enabled;
	bool erased;

	enum poc_state state;

	u8 poc;
	u8 poc_chksum[POC_CHKSUM_LEN];
	u8 poc_ctrl[PANEL_POC_CTRL_LEN];

	u8 *wbuf;
	u32 wpos;
	u32 wsize;

	u8 *rbuf;
	u32 rpos;
	u32 rsize;

#ifdef CONFIG_DISPLAY_USE_INFO
	int total_failcount;
	int total_trycount;
#endif
};

struct panel_poc_device {
	wait_queue_head_t wqh;
	struct miscdevice dev;
	__u64 count;
	unsigned int flags;
	struct mutex op_lock;
	atomic_t cancel;
	bool opened;
	struct panel_poc_info poc_info;
#ifdef CONFIG_DISPLAY_USE_INFO
	struct notifier_block poc_notif;
#endif
};

#define IOC_GET_POC_STATUS	_IOR('A', 100, __u32)		/* 0:NONE, 1:ERASED, 2:WROTE, 3:READ */
#define IOC_GET_POC_CHKSUM	_IOR('A', 101, __u32)		/* 0:CHKSUM ERROR, 1:CHKSUM SUCCESS */
#define IOC_GET_POC_CSDATA	_IOR('A', 102, __u32)		/* CHKSUM DATA 4 Bytes */
#define IOC_GET_POC_ERASED	_IOR('A', 103, __u32)		/* 0:NONE, 1:NOT ERASED, 2:ERASED */
#define IOC_GET_POC_FLASHED	_IOR('A', 104, __u32)		/* 0:NOT POC FLASHED(0x53), 1:POC FLAHSED(0x52) */

#define IOC_SET_POC_ERASE	_IOR('A', 110, __u32)		/* ERASE POC FLASH */
#define IOC_SET_POC_TEST	_IOR('A', 112, __u32)		/* POC FLASH TEST - ERASE/WRITE/READ/COMPARE */

extern int panel_poc_probe(struct panel_poc_device *poc_dev);
extern int set_panel_poc(struct panel_poc_device *poc_dev, u32 cmd);


#endif /* __PANEL_POC_H__ */
