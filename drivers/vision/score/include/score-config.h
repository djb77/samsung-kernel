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

#ifndef SCORE_CONFIG_H_
#define SCORE_CONFIG_H_

/*
 * =======================================================================================
 * CONFIG - GLOBAL OPTIONS
 * =======================================================================================
 */
#define SCORE_MAX_VERTEX			(256)
#define SCORE_MAX_FRAME				(128)

#define SCORE_MAX_BUFFER			(16)
#define SCORE_MAX_PLANE				(3)

#define SCORE_TIME_TICK				(5)

#define SCORE_CAM_L0				(690000)
#define SCORE_CAM_L1				(680000)
#define SCORE_CAM_L2				(670000)
#define SCORE_CAM_L3				(660000)
#define SCORE_CAM_L4				(650000)
#define SCORE_CAM_L5				(640000)
#define SCORE_CAM_L6				(630000)

#define SCORE_MIF_L6				(845000)

#define SCORE_TIMEOUT				(10000)

/*
 * =======================================================================================
 * CONFIG - FEATURE ENABLE
 * =======================================================================================
 */
/* #define DISABLE_CMU */
/* #define DISABLE_CLK_OP */

#define ENABLE_SYSFS_SYSTEM
#define ENABLE_SYSFS_STATE
#define ENABLE_SYSFS_DEBUG

#define ENABLE_BAKERY_LOCK
#define ENABLE_MEM_OFFSET
//#define ENABLE_TIME_STAMP
//#define ENABLE_SC_PRINT_DCACHE_FLUSH

/*
 * =======================================================================================
 * CONFIG - DEBUG OPTIONS
 * =======================================================================================
 */
#define ENABLE_DBG_EVENT
#define ENABLE_DBG_FS

/* #define BUF_MAP_KVADDR */
/* #define DBG_CALL_PATH_LOG */
/* #define DBG_SCV_KERNEL_TIME */
#define ENABLE_TARGET_TIME
/* #define ENABLE_DBG_LOG */
/* #define SCORE_FW_MSG_DEBUG */

#define probe_info(fmt, ...)		pr_info("[V]" fmt, ##__VA_ARGS__)
#define probe_warn(fmt, args...)	pr_warning("[V][WRN]" fmt, ##args)
#define probe_err(fmt, args...)		pr_err("[V][ERR]%s:%d:" fmt, __func__, __LINE__, ##args)

#define score_err_log(fmt, ...)		pr_err(fmt, ##__VA_ARGS__)
#define score_warn_log(fmt, ...)	pr_warning(fmt, ##__VA_ARGS__)
#define score_info_log(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define score_dbg_log(fmt, ...)		printk(KERN_DEBUG fmt, ##__VA_ARGS__)

#define score_err(fmt, args...) \
	score_err_log("[V][ERR][%s:%d]:" fmt, __func__, __LINE__, ##args)

#define score_verr(fmt, vctx, args...) \
	score_err_log("[V][ERR][%s:%d][vctx(%d):%d]:" fmt, \
		__func__, __LINE__, vctx->id, vctx->state, ##args)

#define score_ferr(fmt, frame, args...) \
	score_err_log("[V][ERR][%s:%d][frame(%d,%d):%d]:" fmt, \
		__func__, __LINE__, frame->fid, frame->vid, frame->state, ##args)

#define score_warn(fmt, args...) \
	score_warn_log("[V][WRN][%s:%d]:" fmt, __func__, __LINE__, ##args)

#define score_vwarn(fmt, vctx, args...) \
	score_warn_log("[V][WRN][%s:%d][vctx(%d):%d]:" fmt, \
		__func__, __LINE__, vctx->id, vctx->state, ##args)

#define score_fwarn(fmt, frame, args...) \
	score_warn_log("[V][WRN][%s:%d][frame(%d,%d):%d]:" fmt, \
		__func__, __LINE__, frame->fid, frame->vid, frame->state, ##args)

#define score_info(fmt, args...) \
	score_info_log("[V][INF][%s:%d]:" fmt, __func__, __LINE__, ##args)

#define score_note(fmt, args...) \
	score_info_log("[V][SCore]:" fmt, ##args)

#ifdef ENABLE_DBG_LOG
#define score_dbg(fmt, args...) \
	score_dbg_log("[V][DBG][%s:%d]:" fmt, __func__, __LINE__, ##args)
#else
#define score_dbg(fmt, args...)
#endif

#ifdef DBG_CALL_PATH_LOG
#define score_enter()			score_dbg("enter\n")
#define score_leave()			score_dbg("leave\n")
#else
#define score_enter()
#define score_leave()
#endif

#endif
