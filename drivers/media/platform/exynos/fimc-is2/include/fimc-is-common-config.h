/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_COMMON_CONFIG_H
#define FIMC_IS_COMMON_CONFIG_H

/*
 * =================================================================================================
 * CONFIG - GLOBAL OPTIONS
 * =================================================================================================
 */
#define FIMC_IS_SENSOR_COUNT	4
#define FIMC_IS_STREAM_COUNT	4

#define FIMC_IS_MAX_PRIO	(MAX_RT_PRIO)
/*
 * =================================================================================================
 * CONFIG - FEATURE ENABLE
 * =================================================================================================
 */

/* #define FW_SUSPEND_RESUME */
#define ENABLE_CLOCK_GATE
#define HAS_FW_CLOCK_GATE
/* #define ENABLE_CACHE */
#define ENABLE_FULL_BYPASS
#define ENABLE_ONE_SLOT

/* disable the Fast Shot because of AF fluctuating issue when touch af */
/* #define ENABLE_FAST_SHOT */

#define ENABLE_FAULT_HANDLER
#define ENABLE_PANIC_HANDLER
#define ENABLE_REBOOT_HANDLER
/* #define ENABLE_PANIC_SFR_PRINT */
/* #define ENABLE_MIF_400 */
#define ENABLE_DTP
#define ENABLE_FLITE_OVERFLOW_STOP
#define ENABLE_DBG_FS
/* #define ENABLE_DBG_EVENT */
#define ENABLE_DBG_STATE
#define FIXED_SENSOR_DEBUG
#define ENABLE_RESERVED_MEM

#if defined(CONFIG_PM_DEVFREQ)
#define ENABLE_DVFS
#define START_DVFS_LEVEL FIMC_IS_SN_MAX
#endif /* CONFIG_PM_DEVFREQ */

/* Config related to control HW directly */
#if defined(CONFIG_CAMERA_MC_SCALER_VER1_USE)
#define MCS_USE_SCP_PARAM
#else
#undef MCS_USE_SCP_PARAM
#endif

#if defined(CONFIG_USE_DIRECT_IS_CONTROL)
#undef ENABLE_IS_CORE
#define ENABLE_FPSIMD_FOR_USER
#else
#define ENABLE_IS_CORE
#define ENABLE_FW_SHARE_DUMP
#endif

#ifdef USE_FACE_UNLOCK_AE_AWB_INIT
/* Init AWB */
#define ENABLE_INIT_AWB
#define WB_GAIN_COUNT	(4)
#define INIT_AWB_COUNT_REAR	(3)
#define INIT_AWB_COUNT_FRONT	(7)
#define USE_FACE_UNLOCK_AE_AWB_INIT
#endif

/* notifier for MIF throttling */
#undef CONFIG_CPU_THERMAL_IPA
#if defined(CONFIG_CPU_THERMAL_IPA)
#define EXYNOS_MIF_ADD_NOTIFIER(nb) exynos_mif_add_notifier(nb)
#else
#define EXYNOS_MIF_ADD_NOTIFIER(nb)
#endif

#if defined(CONFIG_USE_HOST_FD_LIBRARY)
#ifndef ENABLE_FD_SW
#define ENABLE_FD_SW
#else
#undef ENABLE_FD_SW
#endif
#endif

/*
 * =================================================================================================
 * CONFIG - DEBUG OPTIONS
 * =================================================================================================
 */

#define DEBUG_LOG_MEMORY
/* #define DEBUG */
/* #define DBG_PSV */
#define DBG_VIDEO
#define DBG_DEVICE
#define DBG_PER_FRAME
/* #define DBG_STREAMING */
/* #define DBG_HW */
/* #define DEBUG_HW_SIZE */
#define DBG_STREAM_ID 0x3F
/* #define DBG_JITTER */
#define FW_PANIC_ENABLE
/* #define SENSOR_PANIC_ENABLE */
#define OVERFLOW_PANIC_ENABLE_ISCHAIN
#define OVERFLOW_PANIC_ENABLE_CSIS
#define ENABLE_KERNEL_LOG_DUMP
/* #define FIXED_FPS_DEBUG */

/* 5fps */
#define FIXED_FPS_VALUE (30 / 6)
#define FIXED_EXPOSURE_VALUE (200000) /* 33.333 * 6 */
#define FIXED_AGAIN_VALUE (150 * 6)
#define FIXED_DGAIN_VALUE (150 * 6)
/* #define DBG_CSIISR */
/* #define DBG_FLITEISR */
/* #define DBG_IMAGE_KMAPPING */
/* #define DBG_DRAW_DIGIT */
/* #define DBG_IMAGE_DUMP */
/* #define DBG_META_DUMP */
#define DBG_HAL_DEAD_PANIC_DELAY (500) /* ms */
#define DBG_DMA_DUMP_PATH	"/data"
#define DBG_DMA_DUMP_INTEVAL	33	/* unit : frame */
#define DBG_DMA_DUMP_VID_COND(vid)	((vid == FIMC_IS_VIDEO_SS0_NUM) || \
					(vid == FIMC_IS_VIDEO_SS1_NUM) || \
					(vid == FIMC_IS_VIDEO_M0P_NUM))
/* #define DEBUG_HW_SFR */
/* #define DBG_DUMPREG */
/* #define USE_ADVANCED_DZOOM */
/* #define TASKLET_MSG */
/* #define PRINT_CAPABILITY */
/* #define PRINT_BUFADDR */
/* #define PRINT_DZOOM */
/* #define PRINT_PARAM */
/* #define PRINT_I2CCMD */
#define ISDRV_VERSION 244

#if defined(DBG_CSIISR) || !defined(ENABLE_IS_CORE)
#define ENABLE_CSIISR
#endif

/*
 * driver version extension
 */
#ifdef ENABLE_CLOCK_GATE
#define get_drv_clock_gate() 0x1
#else
#define get_drv_clock_gate() 0x0
#endif
#ifdef ENABLE_DVFS
#define get_drv_dvfs() 0x2
#else
#define get_drv_dvfs() 0x0
#endif

#define GET_SSX_ID(video) (video->id - FIMC_IS_VIDEO_SS0_NUM)
#define GET_3XS_ID(video) ((video->id < FIMC_IS_VIDEO_31S_NUM) ? 0 : 1)
#define GET_3XC_ID(video) ((video->id < FIMC_IS_VIDEO_31S_NUM) ? 0 : 1)
#define GET_3XP_ID(video) ((video->id < FIMC_IS_VIDEO_31S_NUM) ? 0 : 1)
#define GET_IXS_ID(video) ((video->id < FIMC_IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXC_ID(video) ((video->id < FIMC_IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXP_ID(video) ((video->id < FIMC_IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_DXS_ID(video) ((video->id < FIMC_IS_VIDEO_D1S_NUM) ? 0 : 1)
#define GET_DXC_ID(video) ((video->id < FIMC_IS_VIDEO_D1S_NUM) ? 0 : 1)
#define GET_MXS_ID(video) (video->id - FIMC_IS_VIDEO_M0S_NUM)
#define GET_MXP_ID(video) (video->id - FIMC_IS_VIDEO_M0P_NUM)

#define GET_STR(str) #str

/* sync log with HAL, FW */
#define log_sync(sync_id) info("FIMC_IS_SYNC %d\n", sync_id)

#define test_bit_variables(bit, var) \
	test_bit(((bit)% BITS_PER_LONG), (var))

#ifdef err
#undef err
#endif
#define err(fmt, args...) \
	err_common("[@][ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)

/* multi-stream */
#define merr(fmt, object, args...) \
	merr_common("[@][%d][ERR]%s:%d:", fmt "\n", object->instance, __func__, __LINE__, ##args)

/* multi-stream & group error */
#define mgerr(fmt, object, group, args...) \
	merr_common("[@][%d][GP%d][ERR]%s:%d:", fmt "\n", object->instance, group->id, __func__, __LINE__, ##args)

/* multi-stream & subdev error */
#define mserr(fmt, object, subdev, args...) \
	merr_common("[@][%d][%s][ERR]%s:%d:", fmt "\n", object->instance, subdev->name, __func__, __LINE__, ##args)

/* multi-stream & video error */
#define mverr(fmt, object, video, args...) \
	merr_common("[@][%d][V%02d][ERR]%s:%d:", fmt "\n", object->instance, video->id, __func__, __LINE__, ##args)

/* multi-stream & runtime error */
#define mrerr(fmt, object, frame, args...) \
	merr_common("[@][%d][F%d][ERR]%s:%d:", fmt "\n", object->instance, frame->fcount, __func__, __LINE__, ##args)

/* multi-stream & group & runtime error */
#define mgrerr(fmt, object, group, frame, args...) \
	merr_common("[@][%d][GP%d][F%d][ERR]%s:%d:", fmt "\n", object->instance, group->id, frame->fcount, __func__, __LINE__, ##args)

/* multi-stream & pipe error */
#define mperr(fmt, object, pipe, video, args...) \
	merr_common("[%d][P%02d][V%02d]%s:%d:", fmt, object->instance, pipe->id, video->id, __func__, __LINE__, ##args)

#ifdef warn
#undef warn
#endif
#define warn(fmt, args...) \
	warn_common("[@][WRN]", fmt "\n", ##args)

#define mwarn(fmt, object, args...) \
	mwarn_common("[%d][WRN]", fmt "\n", object->instance, ##args)

#define mgwarn(fmt, object, group, args...) \
	mwarn_common("[%d][GP%d][WRN]", fmt "\n", object->instance, group->id, ##args)

#define mrwarn(fmt, object, frame, args...) \
	mwarn_common("[%d][F%d][WRN]", fmt "\n", object->instance, frame->fcount, ##args)

#define mswarn(fmt, object, subdev, args...) \
	mwarn_common("[%d][%s][WRN]", fmt "\n", object->instance, subdev->name, ##args)

#define mgrwarn(fmt, object, group, frame, args...) \
	mwarn_common("[%d][GP%d][F%d][WRN]", fmt, object->instance, group->id, frame->fcount, ##args)

#define msrwarn(fmt, object, subdev, frame, args...) \
	mwarn_common("[%d][%s][F%d][WRN]", fmt, object->instance, subdev->name, frame->fcount, ##args)

#define mpwarn(fmt, object, pipe, video, args...) \
	mwarn_common("[%d][P%02d][V%02d]", fmt, object->instance, pipe->id, video->id, ##args)

#define info(fmt, args...) \
	dbg_common("[@]", fmt, ##args)

#define sfrinfo(fmt, args...) \
	dbg_common("[@][SFR]", fmt, ##args)

#define minfo(fmt, object, args...) \
	minfo_common("[%d]", fmt, object->instance, ##args)

#define mvinfo(fmt, object, video, args...) \
	minfo_common("[%d][V%02d]", fmt, object->instance, video->id, ##args)

#define msinfo(fmt, object, subdev, args...) \
	minfo_common("[%d][%s]", fmt, object->instance, subdev->name, ##args)

#define msrinfo(fmt, object, subdev, frame, args...) \
	minfo_common("[%d][%s][F%d]", fmt, object->instance, subdev->name, frame->fcount, ##args)

#define mginfo(fmt, object, group, args...) \
	minfo_common("[%d][GP%d]", fmt, object->instance, group->id, ##args)

#define mrinfo(fmt, object, frame, args...) \
	minfo_common("[%d][F%d]", fmt, object->instance, frame->fcount, ##args)

#define mgrinfo(fmt, object, group, frame, args...) \
	minfo_common("[%d][GP%d][F%d]", fmt, object->instance, group->id, frame->fcount, ##args)

#define mpinfo(fmt, object, video, args...) \
	minfo_common("[%d][PV%02d]", fmt, object->instance, video->id, ##args)

#if (defined(DEBUG) && defined(DBG_VIDEO))
#define dbg(fmt, args...)

#define dbg_warning(fmt, args...) \
	dbg_common("%s[WAR] Warning! ", fmt, __func__, ##args)

/* debug message for video node */
#define mdbgv_vid(fmt, args...) \
	dbg_common("[@][VID:V] ", fmt, ##args)

#define mdbgv_pre(fmt, this, args...) \
	mdbg_common("[%d][PRE%d:V] ", fmt, ((struct fimc_is_device_preproc *)this->device)->instance, GET_SSX_ID(this->video), ##args)

#define dbg_sensor(fmt, args...) \
	pr_debug("[@][SSDRV] " fmt, ##args)

#define dbg_actuator(fmt, args...) \
	dbg_sensor(fmt, ##args)

#define dbg_flash(fmt, args...) \
	dbg_sensor(fmt, ##args)

#define dbg_preproc(fmt, args...) \
	dbg_sensor(fmt, ##args)

#define mdbgv_sensor(fmt, this, args...) \
	mdbg_common("[%d][SS%d:V] ", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, GET_SSX_ID(this->video), ##args)

#define mdbgv_3aa(fmt, this, args...) \
	mdbg_common("[%d][3%dS:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_3XS_ID(this->video), ##args)

#define mdbgv_3xc(fmt, this, args...) \
	mdbg_common("[%d][3%dC:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_3XC_ID(this->video), ##args)

#define mdbgv_3xp(fmt, this, args...) \
	mdbg_common("[%d][3%dP:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_3XP_ID(this->video), ##args)

#define mdbgv_isp(fmt, this, args...) \
	mdbg_common("[%d][I%dS:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_IXS_ID(this->video), ##args)

#define mdbgv_ixc(fmt, this, args...) \
	mdbg_common("[%d][I%dC:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_IXC_ID(this->video), ##args)

#define mdbgv_ixp(fmt, this, args...) \
	mdbg_common("[%d][I%dP:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_IXP_ID(this->video), ##args)

#define mdbgv_scp(fmt, this, args...) \
	mdbg_common("[%d][SCP:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_scc(fmt, this, args...) \
	mdbg_common("[%d][SCC:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_dis(fmt, this, args...) \
	mdbg_common("[%d][D%dS:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_DXS_ID(this->video), ##args)

#define mdbgv_dxc(fmt, this, args...) \
	mdbg_common("[%d][D%dC:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_DXC_ID(this->video), ##args)

#define mdbgv_mcs(fmt, this, args...) \
	mdbg_common("[%d][M%dS:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_MXS_ID(this->video), ##args)

#define mdbgv_mxp(fmt, this, args...) \
	mdbg_common("[%d][M%dP:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, GET_MXP_ID(this->video), ##args)

#define mdbgv_vra(fmt, this, args...) \
	mdbg_common("[%d][VRA:V] ", fmt, ((struct fimc_is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_ssxvc0(fmt, this, args...) \
	mdbg_common("[%d][SSXVC0:V] ", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_ssxvc1(fmt, this, args...) \
	mdbg_common("[%d][SSXVC1:V] ", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_ssxvc2(fmt, this, args...) \
	mdbg_common("[%d][SSXVC2:V] ", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_ssxvc3(fmt, this, args...) \
	mdbg_common("[%d][SSXVC3:V] ", fmt, ((struct fimc_is_device_sensor *)this->device)->instance, ##args)
#else
#define dbg(fmt, args...)

/* debug message for video node */
#define mdbgv_vid(fmt, this, args...)
#define dbg_sensor(fmt, args...)
#define dbg_actuator(fmt, args...)
#define dbg_flash(fmt, args...)
#define dbg_preproc(fmt, args...)
#define mdbgv_pre(fmt, this, args...)
#define mdbgv_sensor(fmt, this, args...)
#define mdbgv_3aa(fmt, this, args...)
#define mdbgv_3xc(fmt, this, args...)
#define mdbgv_3xp(fmt, this, args...)
#define mdbgv_isp(fmt, this, args...)
#define mdbgv_ixc(fmt, this, args...)
#define mdbgv_ixp(fmt, this, args...)
#define mdbgv_scp(fmt, this, args...)
#define mdbgv_scc(fmt, this, args...)
#define mdbgv_dis(fmt, this, args...)
#define mdbgv_dxc(fmt, this, args...)
#define mdbgv_mcs(fmt, this, args...)
#define mdbgv_mxp(fmt, this, args...)
#define mdbgv_vra(fmt, this, args...)
#define mdbgv_ssxvc0(fmt, this, args...)
#define mdbgv_ssxvc1(fmt, this, args...)
#define mdbgv_ssxvc2(fmt, this, args...)
#define mdbgv_ssxvc3(fmt, this, args...)
#endif

#if (defined(DEBUG) && defined(DBG_DEVICE))
/* debug message for device */
#define mdbgd_sensor(fmt, this, args...) \
	mdbg_common("[%d][SEN:D] ", fmt, this->instance, ##args)

#define mdbgd_front(fmt, this, args...) \
	mdbg_common("[%d][FRT:D] ", fmt, this->instance, ##args)

#define mdbgd_back(fmt, this, args...) \
	mdbg_common("[%d][BAK:D] ", fmt, this->instance, ##args)

#define mdbgd_3a0(fmt, this, args...) \
	mdbg_common("[%d][3A0:D] ", fmt, this->instance, ##args)

#define mdbgd_3a1(fmt, this, args...) \
	mdbg_common("[%d][3A1:D] ", fmt, this->instance, ##args)

#define mdbgd_isp(fmt, this, args...) \
	mdbg_common("[%d][ISP:D] ", fmt, this->instance, ##args)

#define mdbgd_ischain(fmt, this, args...) \
	mdbg_common("[%d][ISC:D] ", fmt, this->instance, ##args)

#define mdbgd_group(fmt, group, args...) \
	mdbg_common("[%d][GP%d:D] ", fmt, group->device->instance, group->id, ##args)

#define dbg_resource(fmt, args...) \
	dbg_common("[@][RSC] ", fmt, ##args)

#define dbg_core(fmt, args...) \
	dbg_common("[@][COR] ", fmt, ##args)
#else
/* debug message for device */
#define mdbgd_sensor(fmt, this, args...)
#define mdbgd_front(fmt, this, args...)
#define mdbgd_back(fmt, this, args...)
#define mdbgd_isp(fmt, this, args...)
#define mdbgd_ischain(fmt, this, args...)
#define mdbgd_group(fmt, group, args...)
#define dbg_resource(fmt, args...)
#define dbg_core(fmt, args...)
#endif

#if (defined(DEBUG) && defined(DBG_STREAMING))
#define dbg_interface(fmt, args...) \
	dbg_common("[@][ITF] ", fmt, ##args)
#define dbg_frame(fmt, args...) \
	dbg_common("[@][FRM] ", fmt, ##args)
#else
#define dbg_interface(fmt, args...)
#define dbg_frame(fmt, args...)
#endif

#if defined(DBG_PER_FRAME)
#define mdbg_pframe(fmt, object, subdev, frame, args...) \
	mdbg_common("[%d][%s][F%d]", fmt, object->instance, subdev->name, frame->fcount, ##args)
#else
#define mdbg_pframe(fmt, object, subdev, frame, args...)
#endif

/* log at probe */
#define probe_info(fmt, ...)		\
	pr_info("[@]" fmt, ##__VA_ARGS__)
#define probe_err(fmt, args...)		\
	pr_err("[@][ERR]%s:%d:" fmt "\n", __func__, __LINE__, ##args)
#define probe_warn(fmt, args...)	\
	pr_warning("[@][WRN]" fmt "\n", ##args)

#if defined(DEBUG_LOG_MEMORY)
#define fimc_is_err(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define fimc_is_warn(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define fimc_is_dbg(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define fimc_is_info(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define fimc_is_cont(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#else
#define fimc_is_err(fmt, ...)	pr_err(fmt, ##__VA_ARGS__)
#define fimc_is_warn(fmt, ...)	pr_warning(fmt, ##__VA_ARGS__)
#define fimc_is_dbg(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define fimc_is_info(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define fimc_is_cont(fmt, ...)	pr_cont(fmt, ##__VA_ARGS__)
#endif

#define merr_common(prefix, fmt, instance, args...)				\
	do {									\
		if ((1<<(instance)) & DBG_STREAM_ID)				\
			fimc_is_err("[@]" prefix fmt, instance, ##args);		\
	} while (0)

#define mwarn_common(prefix, fmt, instance, args...)				\
	do {									\
		if ((1<<(instance)) & DBG_STREAM_ID)				\
			fimc_is_warn("[@]" prefix fmt, instance, ##args);		\
	} while (0)

#define mdbg_common(prefix, fmt, instance, args...)				\
	do {									\
		if ((1<<(instance)) & DBG_STREAM_ID)				\
			fimc_is_dbg("[@]" prefix fmt, instance, ##args);		\
	} while (0)

#define minfo_common(prefix, fmt, instance, args...)				\
	do {									\
		if ((1<<(instance)) & DBG_STREAM_ID)				\
			fimc_is_info("[@]" prefix fmt, instance, ##args);		\
	} while (0)

#define err_common(prefix, fmt, args...)				\
	fimc_is_err(prefix fmt, ##args)

#define warn_common(prefix, fmt, args...)		\
	fimc_is_warn(prefix fmt, ##args)

#define dbg_common(prefix, fmt, args...)	\
	fimc_is_dbg(prefix fmt, ##args)

/* FIMC-BNS isr log */
#if (defined(DEBUG) && defined(DBG_FLITEISR))
#define dbg_fliteisr(fmt, args...)	\
	fimc_is_cont(fmt, args..)
#else
#define dbg_fliteisr(fmt, args...)
#endif

/* Tasklet Msg log */
#if (defined(DEBUG) && defined(TASKLET_MSG))
#define dbg_tasklet(fmt, args...)	\
	fimc_is_cont(fmt, args..)
#else
#define dbg_tasklet(fmt, args...)
#endif

#if defined(CONFIG_USE_DIRECT_IS_CONTROL)
#if defined(DBG_HW)
#define dbg_hw(fmt, args...) \
	dbg_common("[@][HW]", fmt, ##args)
#define dbg_itfc(fmt, args...) \
	dbg_common("[@][ITFC]", fmt, ##args)
#define dbg_lib(fmt, args...) \
	dbg_common("[@][LIB]", fmt, ##args)
#else
#define dbg_hw(fmt, args...)
#define dbg_itfc(fmt, args...)
#define dbg_lib(fmt, args...)
#endif
#define info_hw(fmt, args...) \
	info("[HW]" fmt, ##args)
#define info_itfc(fmt, args...) \
	info("[ITFC]" fmt, ##args)
#define info_lib(fmt, args...) \
	info("[LIB]" fmt, ##args)
#define warn_hw(fmt, args...) \
	warn_common("[@][HW][WRN]%d:", fmt "\n", __LINE__, ##args)
#define err_hw(fmt, args...) \
	err_common("[@][HW][ERR]%d:", fmt "\n", __LINE__, ##args)
#define err_itfc(fmt, args...) \
	err_common("[@][ITFC][ERR]%d:", fmt "\n", __LINE__, ##args)
#define err_lib(fmt, args...) \
	err_common("[@][LIB][ERR]%d:", fmt "\n", __LINE__, ##args)
#define warn_lib(fmt, args...) \
	warn_common("[@][LIB][WARN]%d:", fmt "\n", __LINE__, ##args)
#endif

#if defined(CONFIG_VENDER_PSV)
#if defined(DBG_PSV)
#define dbg_psv(fmt, args...) \
	dbg_common("[@][PSV] ", fmt, ##args)
#define dbg_vec(fmt, args...) \
	dbg_common("[@][VEC] ", fmt, ##args)
#define dbg_sfr(fmt, args...) \
	dbg_common("[@][SFR] ", fmt, ##args)
#else
#define dbg_psv(fmt, args...)
#define dbg_vec(fmt, args...)
#define dbg_sfr(fmt, args...)
#endif
#define info_psv(fmt, args...) \
	info("[PSV] " fmt, ##args)
#define info_vec(fmt, args...) \
	info("[VEC] " fmt, ##args)
#define info_sfr(fmt, args...) \
	info("[SFR] " fmt, ##args)
#define warn_psv(fmt, args...) \
	warn_common("[@][PSV][WRN]%d: ", fmt "\n", __LINE__, ##args)
#define err_psv(fmt, args...) \
	err_common("[@]PSV][ERR]%d: ", fmt "\n", __LINE__, ##args)
#define warn_vec(fmt, args...) \
	warn_common("[@][VEC][WRN]%d: ", fmt "\n", __LINE__, ##args)
#define err_vec(fmt, args...) \
	err_common("[@][VEC][ERR]%d: ", fmt "\n", __LINE__, ##args)
#define warn_sfr(fmt, args...) \
	warn_common("[@][SFR][WAN]%d: ", fmt "\n", __LINE__, ##args)
#define err_sfr(fmt, args...) \
	err_common("[@][SFR][ERR]%d: ", fmt "\n", __LINE__, ##args)
#endif

#endif
