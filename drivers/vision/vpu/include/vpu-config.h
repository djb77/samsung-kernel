/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPU_CONFIG_H_
#define VPU_CONFIG_H_

/*
 * =================================================================================================
 * CONFIG - GLOBAL OPTIONS
 * =================================================================================================
 */

#define VPU_MAX_BUFFER			16
#define VPU_MAX_PLANE			3

#define VPU_MAX_GRAPH			32
#define VPU_MAX_FRAME			VPU_MAX_BUFFER

/* this macro determines schedule period, the unit is mile second */
#define VPU_TIME_TICK			5
/*
 * =================================================================================================
 * CONFIG -PLATFORM CONFIG
 * =================================================================================================
 */
#ifdef CONFIG_SOC_EXYNOS8890
#define VPU_AHB_BASE_ADDR 0x0
#define VPU_STOP_WAIT_COUNT 200 /* 1 unit : 10ms */
#else
#define VPU_AHB_BASE_ADDR 0x20200000
#define VPU_STOP_WAIT_COUNT 200
#endif


/*
 * =================================================================================================
 * CONFIG - FEATURE ENABLE
 * =================================================================================================
 */

/* #define VPU_DYNAMIC_RESOURCE */

/*
 * =================================================================================================
 * CONFIG - DEBUG OPTIONS
 * =================================================================================================
 */

/* #define DBG_STREAMING */
#define DBG_HISTORY
/* #define DBG_MAILBOX_INIT */
/* #define DBG_MAILBOX_CORE */
/* #define DBG_MAILBOX_DATA */
/* #define DBG_TIMEMEASURE */
#define DBG_MAP_KVADDR
/* #define DBG_RESOURCE */
#define DBG_MARKING
/* #define DBG_HW_SFR */
/* #define DBG_PRINT_TASK */
/* #define DBG_VERBOSE_IO */

#define DEBUG_LOG_MEMORY

#define probe_info(fmt, ...)		pr_info("[V]" fmt, ##__VA_ARGS__)
#define probe_warn(fmt, args...)	pr_warning("[V][WRN]" fmt, ##args)
#define probe_err(fmt, args...) 	pr_err("[V][ERR]%s:%d:" fmt, __func__, __LINE__, ##args)

#ifdef DEBUG_LOG_MEMORY
#define vpu_err_target(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define vpu_warn_target(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define vpu_info_target(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define vpu_dbg_target(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#else
#define vpu_err_target(fmt, ...)	pr_err(fmt, ##__VA_ARGS__)
#define vpu_warn_target(fmt, ...)	pr_warning(fmt, ##__VA_ARGS__)
#define vpu_info_target(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define vpu_dbg_target(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#endif

#define vpu_err(fmt, args...) \
	vpu_err_target("[V][ERR]%s:%d:" fmt, __func__, __LINE__, ##args)

#define vpu_ierr(fmt, vctx, args...) \
	vpu_err_target("[V][I%d][ERR]%s:%d:" fmt, vctx->id, __func__, __LINE__, ##args)

#define vpu_irerr(fmt, vctx, frame, args...) \
	vpu_err_target("[V][I%d][F%d][ERR]%s:%d:" fmt, vctx->id, frame->id, __func__, __LINE__, ##args)

#define vpu_warn(fmt, args...) \
	vpu_warn_target("[V][WRN]%s:%d:" fmt, __func__, __LINE__, ##args)

#define vpu_iwarn(fmt, vctx, args...) \
	vpu_warn_target("[V][I%d][WRN]%s:%d:" fmt, vctx->id, __func__, __LINE__, ##args)

#define vpu_irwarn(fmt, vctx, frame, args...) \
	vpu_warn_target("[V][I%d][F%d][WRN]%s:%d:" fmt, vctx->id, frame->id, __func__, __LINE__, ##args)

#define vpu_info(fmt, args...) \
	vpu_info_target("[V]" fmt, ##args)

#define vpu_iinfo(fmt, vctx, args...) \
	vpu_info_target("[V][I%d]" fmt, vctx->id, ##args)

#define vpu_irinfo(fmt, vctx, frame, args...) \
	vpu_info_target("[V][I%d][F%d]" fmt, vctx->id, frame->id, ##args)

#define vpu_dbg(fmt, args...) \
	vpu_dbg_target("[V]" fmt, ##args)

#define vpu_idbg(fmt, vctx, args...) \
	vpu_dbg_target("[V][I%d]" fmt, vctx->id, ##args)

#define vpu_irdbg(fmt, vctx, frame, args...) \
	vpu_dbg_target("[V][I%d][F%d]" fmt, vctx->id, frame->id, ##args)

#endif
