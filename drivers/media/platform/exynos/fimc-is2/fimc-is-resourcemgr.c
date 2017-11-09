/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <soc/samsung/tmu.h>
#include <linux/isp_cooling.h>
#if defined(CONFIG_SOC_EXYNOS8895)
#include <linux/cpuidle.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <soc/samsung/bts.h>
#endif

#ifdef CONFIG_EXYNOS_BUSMONITOR
#include <linux/exynos-busmon.h>
#endif

#include "fimc-is-resourcemgr.h"
#include "fimc-is-hw.h"
#include "fimc-is-debug.h"
#include "fimc-is-core.h"
#include "fimc-is-dvfs.h"
#include "fimc-is-clk-gate.h"
#if !defined(ENABLE_IS_CORE)
#include "fimc-is-interface-library.h"
#include "hardware/fimc-is-hw-control.h"
#endif
#include <linux/memblock.h>
#include <asm/map.h>

#define CLUSTER_MIN_MASK			0x0000FFFF
#define CLUSTER_MIN_SHIFT			0
#define CLUSTER_MAX_MASK			0xFFFF0000
#define CLUSTER_MAX_SHIFT			16

#if CONFIG_SOC_EXYNOS8895
struct pm_qos_request exynos_isp_qos_int_cam;
#endif
struct pm_qos_request exynos_isp_qos_int;
struct pm_qos_request exynos_isp_qos_mem;
struct pm_qos_request exynos_isp_qos_cam;
struct pm_qos_request exynos_isp_qos_disp;
struct pm_qos_request exynos_isp_qos_hpg;
struct pm_qos_request exynos_isp_qos_cluster0_min;
struct pm_qos_request exynos_isp_qos_cluster0_max;
struct pm_qos_request exynos_isp_qos_cluster1_min;
struct pm_qos_request exynos_isp_qos_cluster1_max;
struct pm_qos_request exynos_isp_qos_cpu_online_min;

#define C0MIN_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster0_min, PM_QOS_CLUSTER0_FREQ_MIN, freq * 1000)
#define C0MIN_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster0_min)
#define C0MIN_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster0_min, freq * 1000)
#define C0MAX_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster0_max, PM_QOS_CLUSTER0_FREQ_MAX, freq * 1000)
#define C0MAX_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster0_max)
#define C0MAX_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster0_max, freq * 1000)
#define C1MIN_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster1_min, PM_QOS_CLUSTER1_FREQ_MIN, freq * 1000)
#define C1MIN_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster1_min)
#define C1MIN_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster1_min, freq * 1000)
#define C1MAX_QOS_ADD(freq) pm_qos_add_request(&exynos_isp_qos_cluster1_max, PM_QOS_CLUSTER1_FREQ_MAX, freq * 1000)
#define C1MAX_QOS_DEL() pm_qos_remove_request(&exynos_isp_qos_cluster1_max)
#define C1MAX_QOS_UPDATE(freq) pm_qos_update_request(&exynos_isp_qos_cluster1_max, freq * 1000)

extern struct fimc_is_sysfs_debug sysfs_debug;
extern int fimc_is_sensor_runtime_suspend(struct device *dev);
extern int fimc_is_sensor_runtime_resume(struct device *dev);

#ifdef ENABLE_IS_CORE
static int fimc_is_resourcemgr_allocmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_mem *mem = &resourcemgr->mem;
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
	int ret = 0;

	minfo->total_size = FW_MEM_SIZE;
#if defined (FW_SUSPEND_RESUME)
	minfo->total_size += FW_BACKUP_SIZE;
#endif
#if defined (ENABLE_ODC)
	minfo->total_size += SIZE_ODC_INTERNAL_BUF * NUM_ODC_INTERNAL_BUF;
#endif
#if defined (ENABLE_VDIS)
	minfo->total_size += SIZE_DIS_INTERNAL_BUF * NUM_DIS_INTERNAL_BUF;
#endif
#if defined (ENABLE_DNR)
	minfo->total_size += SIZE_DNR_INTERNAL_BUF * NUM_DNR_INTERNAL_BUF;
#endif
#if defined (ENABLE_FD_DMA_INPUT)
	minfo->total_size += SIZE_LHFD_SHOT_BUF * MAX_LHFD_SHOT_BUF;
#endif

	minfo->pb_fw = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, minfo->total_size, 16);
	if (IS_ERR(minfo->pb_fw)) {
		err("failed to allocate buffer for FW");
		return -ENOMEM;
	}

#if defined (ENABLE_FD_SW)
	minfo->pb_lhfd = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, LHFD_MAP_SIZE, 16);
	if (IS_ERR(minfo->pb_lhfd)) {
		err("failed to allocate buffer for LHFD_MAP");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_lhfd->size;
#endif

	probe_info("[RSC] Internal memory size (aligned) : %08lx\n", minfo->total_size);

	return ret;
}

static int fimc_is_resourcemgr_initmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = NULL;
	int ret = 0;
	u32 offset;

	dbg_core("fimc_is_init_mem - ION\n");

	ret = fimc_is_resourcemgr_allocmem(resourcemgr);
	if (ret) {
		err("Couldn't alloc for FIMC-IS firmware\n");
		ret = -ENOMEM;
		goto p_err;
	}

	minfo = &resourcemgr->minfo;
	/* set information */
	resourcemgr->minfo.dvaddr = CALL_BUFOP(minfo->pb_fw, dvaddr, minfo->pb_fw);
	resourcemgr->minfo.kvaddr = CALL_BUFOP(minfo->pb_fw, kvaddr, minfo->pb_fw);

	offset = SHARED_OFFSET;
	resourcemgr->minfo.dvaddr_fshared = resourcemgr->minfo.dvaddr + offset;
	resourcemgr->minfo.kvaddr_fshared = resourcemgr->minfo.kvaddr + offset;

	offset = FW_MEM_SIZE - PARAM_REGION_SIZE;
	resourcemgr->minfo.dvaddr_region = resourcemgr->minfo.dvaddr + offset;
	resourcemgr->minfo.kvaddr_region = resourcemgr->minfo.kvaddr + offset;

	offset = FW_MEM_SIZE;
#if defined (FW_SUSPEND_RESUME)
	offset += FW_BACKUP_SIZE;
#endif

#if defined(ENABLE_ODC) || defined(ENABLE_VDIS) || defined(ENABLE_DNR)
	resourcemgr->minfo.dvaddr_tpu = resourcemgr->minfo.dvaddr + offset;
	resourcemgr->minfo.kvaddr_tpu = resourcemgr->minfo.kvaddr + offset;
#if defined (ENABLE_ODC)
	offset += (SIZE_ODC_INTERNAL_BUF * NUM_ODC_INTERNAL_BUF);
#endif
#if defined (ENABLE_VDIS)
	offset += (SIZE_DIS_INTERNAL_BUF * NUM_DIS_INTERNAL_BUF);
#endif
#if defined (ENABLE_DNR)
	offset += (SIZE_DNR_INTERNAL_BUF * NUM_DNR_INTERNAL_BUF);
#endif
#else
	resourcemgr->minfo.dvaddr_tpu = 0;
	resourcemgr->minfo.kvaddr_tpu = 0;
#endif

#if defined(ENABLE_FD_SW)
	resourcemgr->minfo.dvaddr_lhfd = CALL_BUFOP(minfo->pb_lhfd, dvaddr, minfo->pb_lhfd);
	resourcemgr->minfo.kvaddr_lhfd = CALL_BUFOP(minfo->pb_lhfd, kvaddr, minfo->pb_lhfd);
#else
	resourcemgr->minfo.dvaddr_lhfd = 0;
	resourcemgr->minfo.kvaddr_lhfd = 0;
#endif
	resourcemgr->minfo.kvaddr_debug_cnt =  resourcemgr->minfo.kvaddr +
					DEBUG_REGION_OFFSET + DEBUG_REGION_SIZE;

	probe_info("[RSC] Kernel virtual for internal: 0x%lx\n", resourcemgr->minfo.kvaddr);
	probe_info("[RSC] Device virtual for internal: 0x%llx\n", resourcemgr->minfo.dvaddr);
	probe_info("[RSC] fimc_is_init_mem done\n");

p_err:
	return ret;
}

#ifndef ENABLE_RESERVED_MEM
static int fimc_is_resourcemgr_deinitmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
	int ret = 0;

	CALL_VOID_BUFOP(minfo->pb_fw, free, minfo->pb_setfile);
#if defined (ENABLE_FD_SW)
	CALL_VOID_BUFOP(minfo->pb_lhfd, free, minfo->pb_lhfd);
#endif
	return ret;
}
#endif

#else /* #ifdef ENABLE_IS_CORE */
static int fimc_is_resourcemgr_allocmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_mem *mem = &resourcemgr->mem;
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
	size_t tpu_size = 0;

	minfo->total_size = 0;
	/* setfile */
	minfo->pb_setfile = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, SETFILE_SIZE, 16);
	if (IS_ERR_OR_NULL(minfo->pb_setfile)) {
		err("failed to allocate buffer for SETFILE");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_setfile->size;

	/* calibration data for rear lens */
	minfo->pb_rear_cal = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, REAR_CALDATA_SIZE, 16);
	if (IS_ERR_OR_NULL(minfo->pb_rear_cal)) {
		err("failed to allocate buffer for REAR_CALDATA");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_rear_cal->size;

	/* calibration data for front lens */
	minfo->pb_front_cal = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, FRONT_CALDATA_SIZE, 16);
	if (IS_ERR_OR_NULL(minfo->pb_front_cal)) {
		err("failed to allocate buffer for FRONT_CALDATA");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_front_cal->size;

	/* library logging */
	minfo->pb_debug = mem->kmalloc(DEBUG_REGION_SIZE + 0x10, 16);
	if (IS_ERR(minfo->pb_debug)) {
		/* retry by ION */
		minfo->pb_debug = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, DEBUG_REGION_SIZE + 0x10, 16);
		if (IS_ERR_OR_NULL(minfo->pb_debug)) {
			err("failed to allocate buffer for DEBUG_REGION");
			return -ENOMEM;
		}
	}
	minfo->total_size += minfo->pb_debug->size;

	/* data region */
	minfo->pb_dregion = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, DATA_REGION_SIZE, 16);
	if (IS_ERR_OR_NULL(minfo->pb_dregion)) {
		err("failed to allocate buffer for DATA_REGION");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_dregion->size;

	/* parameter region */
	minfo->pb_pregion = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx,
						(FIMC_IS_SENSOR_COUNT*PARAM_REGION_SIZE), 16);
	if (IS_ERR_OR_NULL(minfo->pb_pregion)) {
		err("failed to allocate buffer for PARAM_REGION");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_pregion->size;

	/* fshared data region */
	minfo->pb_fshared = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, FSHARED_REGION_SIZE, 16);
	if (IS_ERR_OR_NULL(minfo->pb_fshared)) {
		err("failed to allocate buffer for FSHARED_REGION");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_fshared->size;

	/* reserved memory for library */
	minfo->pb_lib = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, RESERVE_LIB_SIZE, 16);
	if (IS_ERR_OR_NULL(minfo->pb_lib)) {
		err("failed to allocate buffer for library");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_lib->size;

	/* 3aa/isp internal DMA buffer */
	minfo->pb_taaisp = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, TAAISP_DMA_SIZE, 16);
	if (IS_ERR_OR_NULL(minfo->pb_taaisp)) {
		err("failed to allocate buffer for TAAISP_DMAE");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_taaisp->size;

#if defined (ENABLE_FD_SW)
	minfo->pb_lhfd = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, LHFD_MAP_SIZE, 16);
	if (IS_ERR_OR_NULL(minfo->pb_lhfd)) {
		err("failed to allocate buffer for LHFD_MAP");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_lhfd->size;
#endif

#if defined (ENABLE_VRA)
	minfo->pb_vra = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, VRA_DMA_SIZE, 16);
	if (IS_ERR_OR_NULL(minfo->pb_vra)) {
		err("failed to allocate buffer for VRA");
		return -ENOMEM;
	}
	minfo->total_size += minfo->pb_vra->size;
#endif

#if defined (ENABLE_ODC)
	tpu_size += (SIZE_ODC_INTERNAL_BUF * NUM_ODC_INTERNAL_BUF);
#endif
#if defined (ENABLE_VDIS)
	tpu_size += (SIZE_DIS_INTERNAL_BUF * NUM_DIS_INTERNAL_BUF);
#endif
#if defined (ENABLE_DNR)
	tpu_size += (SIZE_DNR_INTERNAL_BUF * NUM_DNR_INTERNAL_BUF);
#endif
	if (tpu_size > 0) {
		minfo->pb_tpu = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, tpu_size, 16);
		if (IS_ERR_OR_NULL(minfo->pb_tpu)) {
			err("failed to allocate buffer for TPU");
			return -ENOMEM;
		}
		minfo->total_size += minfo->pb_tpu->size;
	}

	probe_info("[RSC] Internal memory size (aligned) : %08lx\n", minfo->total_size);

	return 0;
}

static int fimc_is_resourcemgr_initmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = NULL;
	int ret = 0;

	probe_info("fimc_is_init_mem - ION\n");

	ret = fimc_is_resourcemgr_allocmem(resourcemgr);
	if (ret) {
		err("Couldn't alloc for FIMC-IS\n");
		ret = -ENOMEM;
		goto p_err;
	}

	minfo = &resourcemgr->minfo;
	/* set information */
	resourcemgr->minfo.dvaddr = 0;
	resourcemgr->minfo.kvaddr = 0;

	resourcemgr->minfo.dvaddr_lib = CALL_BUFOP(minfo->pb_lib, dvaddr, minfo->pb_lib);
	resourcemgr->minfo.kvaddr_lib = CALL_BUFOP(minfo->pb_lib, kvaddr, minfo->pb_lib);

	resourcemgr->minfo.dvaddr_debug = CALL_BUFOP(minfo->pb_debug, dvaddr, minfo->pb_debug);
	resourcemgr->minfo.kvaddr_debug = CALL_BUFOP(minfo->pb_debug, kvaddr, minfo->pb_debug);
	resourcemgr->minfo.phaddr_debug = CALL_BUFOP(minfo->pb_debug, phaddr, minfo->pb_debug);

	resourcemgr->minfo.dvaddr_fshared = CALL_BUFOP(minfo->pb_fshared, dvaddr, minfo->pb_fshared);
	resourcemgr->minfo.kvaddr_fshared = CALL_BUFOP(minfo->pb_fshared, kvaddr, minfo->pb_fshared);

	resourcemgr->minfo.dvaddr_region = CALL_BUFOP(minfo->pb_pregion, dvaddr, minfo->pb_pregion);
	resourcemgr->minfo.kvaddr_region = CALL_BUFOP(minfo->pb_pregion, kvaddr, minfo->pb_pregion);

	resourcemgr->minfo.dvaddr_taaisp = CALL_BUFOP(minfo->pb_taaisp, dvaddr, minfo->pb_taaisp);
	resourcemgr->minfo.kvaddr_taaisp = CALL_BUFOP(minfo->pb_taaisp, kvaddr, minfo->pb_taaisp);
#if defined(ENABLE_FD_SW)
	resourcemgr->minfo.dvaddr_lhfd = CALL_BUFOP(minfo->pb_lhfd, dvaddr, minfo->pb_lhfd);
	resourcemgr->minfo.kvaddr_lhfd = CALL_BUFOP(minfo->pb_lhfd, kvaddr, minfo->pb_lhfd);
#else
	resourcemgr->minfo.dvaddr_lhfd = 0;
	resourcemgr->minfo.kvaddr_lhfd = 0;
#endif

#if defined(ENABLE_VRA)
	resourcemgr->minfo.dvaddr_vra = CALL_BUFOP(minfo->pb_vra, dvaddr, minfo->pb_vra);
	resourcemgr->minfo.kvaddr_vra = CALL_BUFOP(minfo->pb_vra, kvaddr, minfo->pb_vra);
#else
	resourcemgr->minfo.dvaddr_vra = 0;
	resourcemgr->minfo.kvaddr_vra = 0;
#endif

#if defined(ENABLE_ODC) || defined(ENABLE_VDIS) || defined(ENABLE_DNR)
	resourcemgr->minfo.dvaddr_tpu = CALL_BUFOP(minfo->pb_tpu, dvaddr, minfo->pb_tpu);
	resourcemgr->minfo.kvaddr_tpu = CALL_BUFOP(minfo->pb_tpu, kvaddr, minfo->pb_tpu);
#else
	resourcemgr->minfo.dvaddr_tpu = 0;
	resourcemgr->minfo.kvaddr_tpu = 0;
#endif
	resourcemgr->minfo.kvaddr_debug_cnt =  resourcemgr->minfo.kvaddr_debug
						+ DEBUG_REGION_SIZE;
	resourcemgr->minfo.kvaddr_setfile = CALL_BUFOP(minfo->pb_setfile, kvaddr, minfo->pb_setfile);
	resourcemgr->minfo.kvaddr_rear_cal = CALL_BUFOP(minfo->pb_rear_cal, kvaddr, minfo->pb_rear_cal);
	resourcemgr->minfo.kvaddr_front_cal = CALL_BUFOP(minfo->pb_front_cal, kvaddr, minfo->pb_front_cal);

	probe_info("[RSC] Kernel virtual for library: %08lx\n", resourcemgr->minfo.kvaddr);
	probe_info("[RSC] Kernel virtual for debug: %08lx\n", resourcemgr->minfo.kvaddr_debug);
	probe_info("[RSC] fimc_is_init_mem done\n");
p_err:
	return ret;
}

static int __init fimc_is_init_static_mem(char *str)
{
	ulong addr = 0;
	struct fimc_is_static_mem static_mem[] = {{0, LIB_START, LIB_SIZE}};
	struct map_desc fimc_iodesc[ARRAY_SIZE(static_mem)];
	size_t i;

	if (kstrtoul(str, 0, (ulong *)&addr) || !addr) {
		probe_warn("[RSC] fimc_is_init_static_mem input address(0x%lx) invalid\n", addr);
		addr = LIB_START;
	}

	if (addr != LIB_START)
		probe_warn("[RSC] reserve-fimc=0x%lx is not equal to LIB_START:0x%lx", addr, LIB_START);

	for (i = 0; i < ARRAY_SIZE(static_mem); i++) {
		static_mem[i].paddr = memblock_alloc(static_mem[i].size, SZ_4K);

		fimc_iodesc[i].virtual = addr;
		fimc_iodesc[i].pfn = __phys_to_pfn(static_mem[i].paddr);
		fimc_iodesc[i].length = static_mem[i].size;
		fimc_iodesc[i].type = MT_MEMORY;
	}
	iotable_init_exec(fimc_iodesc, ARRAY_SIZE(static_mem));

	probe_info("[RSC] fimc_is_init_static_mem done\n");
	return 0;
}
__setup("reserve-fimc=", fimc_is_init_static_mem);

#ifndef ENABLE_RESERVED_MEM
static int fimc_is_resourcemgr_deinitmem(struct fimc_is_resourcemgr *resourcemgr)
{
	struct fimc_is_minfo *minfo = &resourcemgr->minfo;
	int ret = 0;

	CALL_VOID_BUFOP(minfo->pb_setfile, free, minfo->pb_setfile);
	CALL_VOID_BUFOP(minfo->pb_rear_cal, free, minfo->pb_rear_cal);
	CALL_VOID_BUFOP(minfo->pb_front_cal, free, minfo->pb_front_cal);
	CALL_VOID_BUFOP(minfo->pb_debug, free, minfo->pb_debug);
	CALL_VOID_BUFOP(minfo->pb_dregion, free, minfo->pb_dregion);
	CALL_VOID_BUFOP(minfo->pb_pregion, free, minfo->pb_pregion);
	CALL_VOID_BUFOP(minfo->pb_fshared, free, minfo->pb_fshared);
#if defined (ENABLE_FD_SW)
	CALL_VOID_BUFOP(minfo->pb_lhfd, free, minfo->pb_lhfd);
#endif
#if defined (ENABLE_VRA)
	CALL_VOID_BUFOP(minfo->pb_vra, free, minfo->pb_vra);
#endif
#if defined (ENABLE_ODC) || defined (ENABLE_VDIS) || defined (ENABLE_DNR)
	CALL_VOID_BUFOP(minfo->pb_tpu, free, minfo->pb_tpu);
#endif
	return ret;
}
#endif
#endif /* #ifdef ENABLE_IS_CORE */

static int fimc_is_tmu_notifier(struct notifier_block *nb,
	unsigned long state, void *data)
{
#ifdef CONFIG_EXYNOS_THERMAL
	int ret = 0, fps = 0;
	struct fimc_is_resourcemgr *resourcemgr;
#ifdef CONFIG_EXYNOS_SNAPSHOT_THERMAL
	char *cooling_device_name = "ISP";
#endif
	resourcemgr = container_of(nb, struct fimc_is_resourcemgr, tmu_notifier);

	switch (state) {
	case ISP_NORMAL:
		resourcemgr->tmu_state = ISP_NORMAL;
		resourcemgr->limited_fps = 0;
		break;
	case ISP_COLD:
		resourcemgr->tmu_state = ISP_COLD;
		resourcemgr->limited_fps = 0;
		break;
	case ISP_THROTTLING:
		resourcemgr->tmu_state = ISP_THROTTLING;
		fps = isp_cooling_get_fps(0, *(unsigned long *)data);

		/* The FPS can be defined to any specific value. */
		if (fps >= 60) {
			resourcemgr->limited_fps = 0;
			warn("[RSC] THROTTLING : Unlimited FPS");
		} else {
			resourcemgr->limited_fps = fps;
			warn("[RSC] THROTTLING : Limited %dFPS", fps);
		}
		break;
	case ISP_TRIPPING:
		resourcemgr->tmu_state = ISP_TRIPPING;
		resourcemgr->limited_fps = 5;
		warn("[RSC] TRIPPING : Limited 5FPS");
		break;
	default:
		err("[RSC] invalid tmu state(%ld)", state);
		break;
	}

#ifdef CONFIG_EXYNOS_SNAPSHOT_THERMAL
	exynos_ss_thermal(NULL, 0, cooling_device_name, resourcemgr->limited_fps);
#endif
	return ret;
#else
	return 0;
#endif
}

#ifdef CONFIG_EXYNOS_BUSMONITOR
static int due_to_fimc_is(const char *desc)
{
	if (desc && (strstr((char *)desc, "CAM")
				|| strstr((char *)desc, "ISP")))
			return 1;

	return 0;
}

static int fimc_is_bm_notifier(struct notifier_block *nb,
	unsigned long state, void *data)
{
	int i;
	struct fimc_is_core *core;
	struct fimc_is_resourcemgr *resourcemgr;
	struct busmon_notifier *busmon;

	resourcemgr = container_of(nb, struct fimc_is_resourcemgr, bm_notifier);
	core = container_of(resourcemgr, struct fimc_is_core, resourcemgr);
	busmon = (struct busmon_notifier *)data;

	if (!busmon)
		return 0;

	if (due_to_fimc_is(busmon->init_desc)
			|| due_to_fimc_is(busmon->target_desc)
			|| due_to_fimc_is(busmon->masterip_desc)) {
		info("1. NOC info.\n");
		info("%s: init description : %s\n", __func__, busmon->init_desc);
		info("%s: target descrition: %s\n", __func__, busmon->target_desc);
		info("%s: user description : %s\n", __func__, busmon->masterip_desc);
		info("%s: user id          : %u\n", __func__, busmon->masterip_idx);
		info("%s: target address   : %lx\n",__func__, busmon->target_addr);

		for (i = 0; i < FIMC_IS_STREAM_COUNT; ++i) {
			if (!test_bit(FIMC_IS_ISCHAIN_POWER_ON, &core->ischain[i].state))
				continue;

			info("2. FW log dump\n");
			fimc_is_hw_logdump(&core->interface);

			info("3. Clock info.\n");
			CALL_POPS(core, print_clk);
		}
	}

	return 0;
}
#endif /* CONFIG_EXYNOS_BUSMONITOR */

#ifdef ENABLE_FW_SHARE_DUMP
static int fimc_is_fw_share_dump(void)
{
	int ret = 0;
	u8 *buf;
	struct fimc_is_core *core = NULL;
	struct fimc_is_resourcemgr *resourcemgr;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		goto p_err;

	resourcemgr = &core->resourcemgr;
	buf = (u8 *)resourcemgr->fw_share_dump_buf;

	/* dump share region in fw area */
	if (IS_ERR_OR_NULL(buf)) {
		err("%s: fail to alloc", __func__);
		ret = -ENOMEM;
		goto p_err;
	}

	/* sync with fw for memory */
	vb2_ion_sync_for_cpu(resourcemgr->minfo.fw_cookie, 0,
			SHARED_OFFSET, DMA_BIDIRECTIONAL);

	memcpy(buf, (u8 *)resourcemgr->minfo.kvaddr_fshared, SHARED_SIZE);

	info("%s: dumped ramdump addr(virt/phys/size): (%p/%p/0x%X)", __func__, buf,
			(void *)virt_to_phys(buf), SHARED_SIZE);
p_err:
	return ret;
}
#endif

int fimc_is_resource_dump(void)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_group *group;
	struct fimc_is_subdev *subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_device_ischain *device = NULL;
	int i, j;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		goto exit;

	info("### %s dump start ###\n", __func__);

	groupmgr = &core->groupmgr;

	/* dump per core */
	for (i = 0; i < FIMC_IS_STREAM_COUNT; ++i) {
		device = &core->ischain[i];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state))
			continue;

		if (test_bit(FIMC_IS_ISCHAIN_CLOSING, &device->state))
			continue;

		/* clock & gpio dump */
		CALL_POPS(device, print_clk);
#ifdef ENABLE_IS_CORE
		/* fw log, mcuctl dump */
		fimc_is_hw_logdump(&core->interface);
		fimc_is_hw_regdump(&core->interface);
#else
		/* ddk log dump */
		fimc_is_lib_logdump();
		fimc_is_hardware_clk_gate_dump(&core->hardware);
		fimc_is_hardware_sfr_dump(&core->hardware);
#endif
		break;
	}

	/* dump per ischain */
	for (i = 0; i < FIMC_IS_STREAM_COUNT; ++i) {
		device = &core->ischain[i];
		if (!test_bit(FIMC_IS_ISCHAIN_OPEN_STREAM, &device->state))
			continue;

		if (test_bit(FIMC_IS_ISCHAIN_CLOSING, &device->state))
			continue;

		/* dump all framemgr */
		group = groupmgr->leader[i];
		while (group) {
			if (!test_bit(FIMC_IS_GROUP_OPEN, &group->state))
				break;

			for (j = 0; j < ENTRY_END; j++) {
				subdev = group->subdev[j];
				if (subdev && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
					framemgr = GET_SUBDEV_FRAMEMGR(subdev);
					if (framemgr) {
						unsigned long flags;

						mserr(" dump framemgr..", subdev, subdev);
						framemgr_e_barrier_irqs(framemgr, 0, flags);
						frame_manager_print_queues(framemgr);
						framemgr_x_barrier_irqr(framemgr, 0, flags);
					}
				}
			}

			group = group->next;
		}
	}

	info("### %s dump end ###\n", __func__);

exit:
	return 0;
}

#ifdef ENABLE_PANIC_HANDLER
static int fimc_is_panic_handler(struct notifier_block *nb, ulong l,
	void *buf)
{
#if !defined(ENABLE_IS_CORE)
	fimc_is_resource_dump();
#endif
#ifdef ENABLE_FW_SHARE_DUMP
	/* dump share area in fw region */
	fimc_is_fw_share_dump();
#endif
	return 0;
}

static struct notifier_block notify_panic_block = {
	.notifier_call = fimc_is_panic_handler,
};
#endif

#if defined(ENABLE_REBOOT_HANDLER) && !defined(ENABLE_IS_CORE)
static int fimc_is_reboot_handler(struct notifier_block *nb, ulong l,
	void *buf)
{
	struct fimc_is_core *core = NULL;

	info("%s:++\n", __func__);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core)
		goto exit;

	fimc_is_cleanup(core);
exit:
	info("%s:--\n", __func__);
	return 0;
}

static struct notifier_block notify_reboot_block = {
	.notifier_call = fimc_is_reboot_handler,
};
#endif

int fimc_is_resourcemgr_probe(struct fimc_is_resourcemgr *resourcemgr,
	void *private_data)
{
	int ret = 0;

	BUG_ON(!resourcemgr);
	BUG_ON(!private_data);

	resourcemgr->private_data = private_data;

	clear_bit(FIMC_IS_RM_COM_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS0_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS1_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS2_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS3_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS4_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_SS5_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_ISC_POWER_ON, &resourcemgr->state);
	clear_bit(FIMC_IS_RM_POWER_ON, &resourcemgr->state);
	atomic_set(&resourcemgr->rsccount, 0);
	atomic_set(&resourcemgr->resource_sensor0.rsccount, 0);
	atomic_set(&resourcemgr->resource_sensor1.rsccount, 0);
	atomic_set(&resourcemgr->resource_sensor2.rsccount, 0);
	atomic_set(&resourcemgr->resource_sensor3.rsccount, 0);
	atomic_set(&resourcemgr->resource_ischain.rsccount, 0);
	atomic_set(&resourcemgr->resource_preproc.rsccount, 0);

	resourcemgr->cluster0 = 0;
	resourcemgr->cluster1 = 0;
	resourcemgr->hal_version = IS_HAL_VER_1_0;

	/* temperature monitor unit */
	resourcemgr->tmu_notifier.notifier_call = fimc_is_tmu_notifier;
	resourcemgr->tmu_notifier.priority = 0;
	resourcemgr->tmu_state = ISP_NORMAL;
	resourcemgr->limited_fps = 0;

	/* bus monitor unit */

#ifdef CONFIG_EXYNOS_BUSMONITOR
	resourcemgr->bm_notifier.notifier_call = fimc_is_bm_notifier;
	resourcemgr->bm_notifier.priority = 0;

	busmon_notifier_chain_register(&resourcemgr->bm_notifier);
#endif

	ret = exynos_tmu_isp_add_notifier(&resourcemgr->tmu_notifier);
	if (ret) {
		probe_err("exynos_tmu_isp_add_notifier is fail(%d)", ret);
		goto p_err;
	}

#ifdef ENABLE_RESERVED_MEM
	ret = fimc_is_resourcemgr_initmem(resourcemgr);
	if (ret) {
		probe_err("fimc_is_resourcemgr_initmem is fail(%d)", ret);
		goto p_err;
	}
#endif

#ifdef ENABLE_DVFS
	/* dvfs controller init */
	ret = fimc_is_dvfs_init(resourcemgr);
	if (ret) {
		probe_err("%s: fimc_is_dvfs_init failed!\n", __func__);
		goto p_err;
	}
#endif

#ifdef ENABLE_PANIC_HANDLER
	atomic_notifier_chain_register(&panic_notifier_list, &notify_panic_block);
#endif
#if defined(ENABLE_REBOOT_HANDLER) && !defined(ENABLE_IS_CORE)
	register_reboot_notifier(&notify_reboot_block);
#endif
#ifdef ENABLE_SHARED_METADATA
	spin_lock_init(&resourcemgr->shared_meta_lock);
#endif
#ifdef ENABLE_FW_SHARE_DUMP
	/* to dump share region in fw area */
	resourcemgr->fw_share_dump_buf = (ulong)kzalloc(SHARED_SIZE, GFP_KERNEL);
#endif

p_err:
	probe_info("[RSC] %s(%d)\n", __func__, ret);
	return ret;
}

int fimc_is_resource_open(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type, void **device)
{
	int ret = 0;
	u32 stream;
	void *result;
	struct fimc_is_resource *resource;
	struct fimc_is_core *core;
	struct fimc_is_device_ischain *ischain;

	BUG_ON(!resourcemgr);
	BUG_ON(!resourcemgr->private_data);
	BUG_ON(rsc_type >= RESOURCE_TYPE_MAX);

	result = NULL;
	core = (struct fimc_is_core *)resourcemgr->private_data;
	resource = GET_RESOURCE(resourcemgr, rsc_type);
	if (!resource) {
		err("[RSC] resource is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	switch (rsc_type) {
	case RESOURCE_TYPE_PREPROC:
		result = &core->preproc;
		resource->pdev = core->preproc.pdev;
		break;
	case RESOURCE_TYPE_SENSOR0:
		result = &core->sensor[RESOURCE_TYPE_SENSOR0];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR0].pdev;
		break;
	case RESOURCE_TYPE_SENSOR1:
		result = &core->sensor[RESOURCE_TYPE_SENSOR1];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR1].pdev;
		break;
	case RESOURCE_TYPE_SENSOR2:
		result = &core->sensor[RESOURCE_TYPE_SENSOR2];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR2].pdev;
		break;
	case RESOURCE_TYPE_SENSOR3:
		result = &core->sensor[RESOURCE_TYPE_SENSOR3];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR3].pdev;
		break;
#if (FIMC_IS_SENSOR_COUNT > 4)
	case RESOURCE_TYPE_SENSOR4:
		result = &core->sensor[RESOURCE_TYPE_SENSOR4];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR4].pdev;
		break;
	case RESOURCE_TYPE_SENSOR5:
		result = &core->sensor[RESOURCE_TYPE_SENSOR5];
		resource->pdev = core->sensor[RESOURCE_TYPE_SENSOR5].pdev;
		break;
#endif
	case RESOURCE_TYPE_ISCHAIN:
		for (stream = 0; stream < FIMC_IS_STREAM_COUNT; ++stream) {
			ischain = &core->ischain[stream];
			if (!test_bit(FIMC_IS_ISCHAIN_OPEN, &ischain->state)) {
				result = ischain;
				resource->pdev = ischain->pdev;
				break;
			}
		}
		break;
	}

	if (device)
		*device = result;

p_err:
	dbg_resource("%s\n", __func__);
	return ret;
}

int fimc_is_resource_get(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type)
{
	int ret = 0;
	u32 rsccount;
	struct fimc_is_resource *resource;
	struct fimc_is_core *core;
	int i;

	BUG_ON(!resourcemgr);
	BUG_ON(!resourcemgr->private_data);
	BUG_ON(rsc_type >= RESOURCE_TYPE_MAX);

	core = (struct fimc_is_core *)resourcemgr->private_data;
	rsccount = atomic_read(&core->rsccount);
	resource = GET_RESOURCE(resourcemgr, rsc_type);
	if (!resource) {
		err("[RSC] resource is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!core->pdev) {
		err("[RSC] pdev is NULL");
		ret = -EMFILE;
		goto p_err;
	}

	if (rsccount >= (FIMC_IS_STREAM_COUNT + FIMC_IS_VIDEO_SS5_NUM)) {
		err("[RSC] Invalid rsccount(%d)", rsccount);
		ret = -EMFILE;
		goto p_err;
	}

	if (rsccount == 0) {
		pm_stay_awake(&core->pdev->dev);

		resourcemgr->cluster0 = 0;
		resourcemgr->cluster1 = 0;

#ifdef ENABLE_DVFS
		/* dvfs controller init */
		ret = fimc_is_dvfs_init(resourcemgr);
		if (ret) {
			err("%s: fimc_is_dvfs_init failed!\n", __func__);
			goto p_err;
		}
#endif
#if defined(CONFIG_EXYNOS_DEVICE_MIPI_CSIS_VER3)
		/* CSIS common DMA rcount set */
		atomic_set(&core->csi_dma.rcount, 0);
#endif
#if defined(CONFIG_SECURE_CAMERA_USE)
		mutex_init(&core->secure_state_lock);
		core->secure_state = FIMC_IS_STATE_UNSECURE;

		dbg_resource("%s: fimc-is secure state has reset\n", __func__);
#endif
		core->dual_info.mode = FIMC_IS_DUAL_MODE_NOTHING;
		core->dual_info.pre_mode = FIMC_IS_DUAL_MODE_NOTHING;
		core->dual_info.tick_count = 0;

		for (i = 0; i < MAX_SENSOR_SHARED_RSC; i++) {
			spin_lock_init(&core->shared_rsc_slock[i]);
			atomic_set(&core->shared_rsc_count[i], 0);
		}
	}

	if (atomic_read(&resource->rsccount) == 0) {
		switch (rsc_type) {
		case RESOURCE_TYPE_PREPROC:
#if defined(CONFIG_PM)
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_preproc_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_COM_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR0:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS0_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR1:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS1_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR2:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS2_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR3:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS3_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR4:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS4_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR5:
#ifdef CONFIG_PM
			pm_runtime_get_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_resume(&resource->pdev->dev);
#endif
			set_bit(FIMC_IS_RM_SS5_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_ISCHAIN:
			if (test_bit(FIMC_IS_RM_POWER_ON, &resourcemgr->state)) {
				err("all resource is not power off(%lX)", resourcemgr->state);
				ret = -EINVAL;
				goto p_err;
			}

#ifndef ENABLE_RESERVED_MEM
			ret = fimc_is_resourcemgr_initmem(resourcemgr);
			if (ret) {
				err("fimc_is_resourcemgr_initmem is fail(%d)\n", ret);
				goto p_err;
			}
#endif

			ret = fimc_is_debug_open(&resourcemgr->minfo);
			if (ret) {
				err("fimc_is_debug_open is fail(%d)", ret);
				goto p_err;
			}

			ret = fimc_is_interface_open(&core->interface);
			if (ret) {
				err("fimc_is_interface_open is fail(%d)", ret);
				goto p_err;
			}

			ret = fimc_is_ischain_power(&core->ischain[0], 1);
			if (ret) {
				err("fimc_is_ischain_power is fail(%d)", ret);
				fimc_is_ischain_power(&core->ischain[0], 0);
				goto p_err;
			}

			/* W/A for a lower version MCUCTL */
			fimc_is_interface_reset(&core->interface);

#if !defined(ENABLE_IS_CORE) && defined(USE_MCUCTL)
			fimc_is_hw_s_ctrl(&core->interface, 0, HW_S_CTRL_CHAIN_IRQ, 0);
#endif

#ifdef ENABLE_CLOCK_GATE
			if (sysfs_debug.en_clk_gate &&
					sysfs_debug.clk_gate_mode == CLOCK_GATE_MODE_HOST)
				fimc_is_clk_gate_init(core);
#endif

			set_bit(FIMC_IS_RM_ISC_POWER_ON, &resourcemgr->state);
			set_bit(FIMC_IS_RM_POWER_ON, &resourcemgr->state);

#if defined(CONFIG_SOC_EXYNOS8895)
			info("%s: call cpuidle_pause()\n", __func__);
			cpuidle_pause();

			info("%s: call bts_update_scen(1)\n", __func__);
			bts_update_scen(BS_CAMERA_DEFAULT, 1);
			fimc_is_hw_ischain_qe_cfg();
#endif
			break;
		default:
			err("[RSC] resource type(%d) is invalid", rsc_type);
			BUG();
			break;
		}

#if !defined(ENABLE_IS_CORE) && !defined(DISABLE_LIB)
		if ((rsc_type != RESOURCE_TYPE_PREPROC)
			&& (rsc_type != RESOURCE_TYPE_SENSOR3)
			&& (!test_and_set_bit(FIMC_IS_BINARY_LOADED, &resourcemgr->binary_state))) {
			ret = fimc_is_load_bin();
			if (ret < 0) {
				err("fimc_is_load_bin() is fail(%d)", ret);
				clear_bit(FIMC_IS_BINARY_LOADED, &resourcemgr->binary_state);
				goto p_err;
			}
			fimc_is_hardware_clear(&core->hardware);

			info("fimc_is_load_bin() done\n");
		}
#endif
	}
	atomic_inc(&resource->rsccount);
	atomic_inc(&core->rsccount);

p_err:
	info("[RSC] rsctype: %d, rsccount: device[%d], core[%d]\n", rsc_type,
			atomic_read(&resource->rsccount), rsccount + 1);
	return ret;
}

int fimc_is_resource_put(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type)
{
	int ret = 0;
	u32 rsccount;
	struct fimc_is_resource *resource;
	struct fimc_is_core *core;

	BUG_ON(!resourcemgr);
	BUG_ON(!resourcemgr->private_data);
	BUG_ON(rsc_type >= RESOURCE_TYPE_MAX);

	core = (struct fimc_is_core *)resourcemgr->private_data;
	rsccount = atomic_read(&core->rsccount);
	resource = GET_RESOURCE(resourcemgr, rsc_type);
	if (!resource) {
		err("[RSC] resource is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!core->pdev) {
		err("[RSC] pdev is NULL");
		ret = -EMFILE;
		goto p_err;
	}

	if (rsccount == 0) {
		err("[RSC] Invalid rsccount(%d)\n", rsccount);
		ret = -EMFILE;
		goto p_err;
	}

	/* local update */
	if (atomic_read(&resource->rsccount) == 1) {
#if !defined(ENABLE_IS_CORE) && !defined(DISABLE_LIB)
		if ((rsc_type != RESOURCE_TYPE_PREPROC)
			&& (rsc_type != RESOURCE_TYPE_SENSOR3)
			&& (test_and_clear_bit(FIMC_IS_BINARY_LOADED, &resourcemgr->binary_state))) {
			fimc_is_load_clear();

			info("fimc_is_load_clear() done\n");
		}
#endif
		/* clear hal version, default 1.0 */
		resourcemgr->hal_version = IS_HAL_VER_1_0;

		switch (rsc_type) {
		case RESOURCE_TYPE_PREPROC:
#if defined(CONFIG_PM)
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_preproc_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_COM_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR0:
#if defined(CONFIG_PM)
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS0_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR1:
#if defined(CONFIG_PM)
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS1_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR2:
#ifdef CONFIG_PM
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS2_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR3:
#ifdef CONFIG_PM
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS3_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR4:
#ifdef CONFIG_PM
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS4_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_SENSOR5:
#ifdef CONFIG_PM
			pm_runtime_put_sync(&resource->pdev->dev);
#else
			fimc_is_sensor_runtime_suspend(&resource->pdev->dev);
#endif
			clear_bit(FIMC_IS_RM_SS5_POWER_ON, &resourcemgr->state);
			break;
		case RESOURCE_TYPE_ISCHAIN:
			ret = fimc_is_itf_power_down(&core->interface);
			if (ret)
				err("power down cmd is fail(%d)", ret);

			ret = fimc_is_ischain_power(&core->ischain[0], 0);
			if (ret)
				err("fimc_is_ischain_power is fail(%d)", ret);

			ret = fimc_is_interface_close(&core->interface);
			if (ret)
				err("fimc_is_interface_close is fail(%d)", ret);

			ret = fimc_is_debug_close();
			if (ret)
				err("fimc_is_debug_close is fail(%d)", ret);

#ifndef ENABLE_RESERVED_MEM
			ret = fimc_is_resourcemgr_deinitmem(resourcemgr);
			if (ret)
				err("fimc_is_resourcemgr_deinitmem is fail(%d)", ret);
#endif

			clear_bit(FIMC_IS_RM_ISC_POWER_ON, &resourcemgr->state);

#if defined(CONFIG_SOC_EXYNOS8895)
			info("%s: call cpuidle_resume()\n", __func__);
			cpuidle_resume();

			info("%s: call bts_update_scen(0)\n", __func__);
			bts_update_scen(BS_CAMERA_DEFAULT, 0);
#endif
			break;
		default:
			err("[RSC] resource type(%d) is invalid", rsc_type);
			BUG();
			break;
		}
	}

	/* global update */
	if (atomic_read(&core->rsccount) == 1) {
		u32 current_min, current_max;

		current_min = (resourcemgr->cluster0 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
		current_max = (resourcemgr->cluster0 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
		if (current_min) {
			C0MIN_QOS_DEL();
			warn("[RSC] cluster0 minfreq is not removed(%dMhz)\n", current_min);
		}

		if (current_max) {
			C0MAX_QOS_DEL();
			warn("[RSC] cluster0 maxfreq is not removed(%dMhz)\n", current_max);
		}

		current_min = (resourcemgr->cluster1 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
		current_max = (resourcemgr->cluster1 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
		if (current_min) {
			C1MIN_QOS_DEL();
			warn("[RSC] cluster1 minfreq is not removed(%dMhz)\n", current_min);
		}

		if (current_max) {
			C1MAX_QOS_DEL();
			warn("[RSC] cluster1 maxfreq is not removed(%dMhz)\n", current_max);
		}

		resourcemgr->cluster0 = 0;
		resourcemgr->cluster1 = 0;

		ret = fimc_is_runtime_suspend_post(&resource->pdev->dev);
		if (ret)
			err("fimc_is_runtime_suspend_post is fail(%d)", ret);

		pm_relax(&core->pdev->dev);

		clear_bit(FIMC_IS_RM_POWER_ON, &resourcemgr->state);
	}

	atomic_dec(&resource->rsccount);
	atomic_dec(&core->rsccount);

p_err:
	info("[RSC] rsctype: %d, rsccount: device[%d], core[%d]\n", rsc_type,
			atomic_read(&resource->rsccount), rsccount - 1);
	return ret;
}

int fimc_is_resource_count(struct fimc_is_resourcemgr *resourcemgr, u32 rsc_type)
{
	struct fimc_is_resource *resource;

	BUG_ON(!resourcemgr);
	BUG_ON(rsc_type >= RESOURCE_TYPE_MAX);

	resource = GET_RESOURCE(resourcemgr, rsc_type);
	if (!resource) {
		err("[RSC] resource is NULL");
		return -EINVAL;
	}

	return atomic_read(&resource->rsccount);
}

int fimc_is_resource_ioctl(struct fimc_is_resourcemgr *resourcemgr, struct v4l2_control *ctrl)
{
	int ret = 0;

	BUG_ON(!resourcemgr);
	BUG_ON(!ctrl);

	switch (ctrl->id) {
	/* APOLLO CPU0~3 */
	case V4L2_CID_IS_DVFS_CLUSTER0:
		{
			u32 current_min, current_max;
			u32 request_min, request_max;

			current_min = (resourcemgr->cluster0 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			current_max = (resourcemgr->cluster0 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
			request_min = (ctrl->value & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			request_max = (ctrl->value & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;

			if (current_min) {
				if (request_min)
					C0MIN_QOS_UPDATE(request_min);
				else
					C0MIN_QOS_DEL();
			} else {
				if (request_min)
					C0MIN_QOS_ADD(request_min);
			}

			if (current_max) {
				if (request_max)
					C0MAX_QOS_UPDATE(request_max);
				else
					C0MAX_QOS_DEL();
			} else {
				if (request_max)
					C0MAX_QOS_ADD(request_max);
			}

			info("[RSC] cluster0 minfreq : %dMhz\n", request_min);
			info("[RSC] cluster0 maxfreq : %dMhz\n", request_max);
			resourcemgr->cluster0 = (request_max << CLUSTER_MAX_SHIFT) | request_min;
		}
		break;
	/* ATLAS CPU4~7 */
	case V4L2_CID_IS_DVFS_CLUSTER1:
		{
			u32 current_min, current_max;
			u32 request_min, request_max;

			current_min = (resourcemgr->cluster1 & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			current_max = (resourcemgr->cluster1 & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;
			request_min = (ctrl->value & CLUSTER_MIN_MASK) >> CLUSTER_MIN_SHIFT;
			request_max = (ctrl->value & CLUSTER_MAX_MASK) >> CLUSTER_MAX_SHIFT;

			if (current_min) {
				if (request_min)
					C1MIN_QOS_UPDATE(request_min);
				else
					C1MIN_QOS_DEL();
			} else {
				if (request_min)
					C1MIN_QOS_ADD(request_min);
			}

			if (current_max) {
				if (request_max)
					C1MAX_QOS_UPDATE(request_max);
				else
					C1MAX_QOS_DEL();
			} else {
				if (request_max)
					C1MAX_QOS_ADD(request_max);
			}

			info("[RSC] cluster1 minfreq : %dMhz\n", request_min);
			info("[RSC] cluster1 maxfreq : %dMhz\n", request_max);
			resourcemgr->cluster1 = (request_max << CLUSTER_MAX_SHIFT) | request_min;
		}
		break;
	}

	return ret;
}

int fimc_is_logsync(struct fimc_is_interface *itf, u32 sync_id, u32 msg_test_id)
{
	int ret = 0;

	/* print kernel sync log */
	log_sync(sync_id);

#ifdef ENABLE_FW_SYNC_LOG
	ret = fimc_is_hw_msg_test(itf, sync_id, msg_test_id);
	if (ret)
	err("fimc_is_hw_msg_test(%d)", ret);
#endif
	return ret;
}
