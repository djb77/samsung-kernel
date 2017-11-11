/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPUL_HWMAPPER_H_
#define VPUL_HWMAPPER_H_


#include "vpu-hardware.h"
#include "vpul-ds.h"



/** \addtogroup VPUL-HwMapper
 *  @{
 */

/**
 * \name Resource Get Bit Flags definitions
 * \brief Bit flags for "flags" argument to vpu_resource_get.
 * \li various combinations of those flag values are possible
 * \li each flag is represented here by its bit-number the corresponding
 *     mask value would be 1 << bit-number
 *     @{
 */

/**
 * \brief Specifies that PU will not be allocated, and PU sepcified in
 *        task will remain.
 */
#define VPUL_GRAPH_FLAG_FIXED					0
/**
 * \brief
 * Specifies that a resource (PU instance / MPRB) can be re-used in
 * different sub-chains of same task.
 * \details
 * Specifies that a resource (PU instance / MPRB) can be re-used in
 * different sub-chains of same task.
 * If this option is NOT selected, when trying to allocate resources for a
 * specific subchain, resources already
 * allocated to other subchains in task are not available for allocation.
 */
#define VPUL_GRAPH_FLAG_SHARED_AMONG_SUBCHAINS			1
/**
 * \brief
 * Should HW mapper prefer allocate while sharing between subchains.
 * \note
 * This flag applies only if VPUL_GRAPH_FLAG_SHARED_AMONG_SUBCHAINS is set
 * \details
 * Should HW mapper prefer allocate while sharing between subchains.
 * \li flag set : will first try to allocate resources already allocated to
 *	      other subchains in task, and only if this fails - try to
 *	      allocate resources not allocated to other subchains
 * \li flag cleared : will first try to allocate resources not allocated to
 *		  other subchains in task, and only if this fails - try to
 *		  allocate resources already allocated to other subchains
 */
#define VPUL_GRAPH_FLAG_SHARING_AMONG_SUBCHAINS_PREFERRED	2
/**
 * \brief
 * Specifies that a resource (PU instance / MPRB) can be concurrently
 * allocated to different tasks.
 * \details
 * Specifies that a resource (PU instance / MPRB) can be concurrently
 * allocated to different tasks.
 * If this option is NOT selected, resources
 * already allocated to other tasks are not available for allocation in this
 * task.
 */
#define VPUL_GRAPH_FLAG_SHARED_AMONG_TASKS			3
/**
 * \brief Specifies sharing of resources with other tasks
 * \note
 * this flag applies only if VPUL_GRAPH_FLAG_SHARED_AMONG_TASKS is set
 * \details
 * Specifies sharing of resources with other tasks
 * \li flag set : will first try to allocate resources already allocated to
 *	      other tasks, and only if this fails - try to allocate
 *	      resources not allocated to other tasks
 * \li flag cleared : will first try to allocate resources not allocated to
 *		  other tasks, and only if this fails - try to allocate
 *		  resources already allocated to other tasks
 */
#define VPUL_GRAPH_FLAG_SHARING_AMONG_TASKS_PREFERRED		4

/**
 * \brief Specifies that HW mapper should not try to balance
 * sub-chains latency.
 * \details Specifies that HW mapper should not try to balance
 * sub-chains latency
 */
#define VPUL_GRAPH_FLAG_DSBL_LATENCY_BALANCING			5

/**
 * \brief Flag to change small MPRB to large
 */
#define VPUL_STATIC_ALLOC_LARGE_MPRB_INSTEAD_SMALL_FLAG		(6)
#define VPUL_STATIC_ALLOC_LARGE_INSTEAD_SMALL_MPRB_MASK		(0x40)

/**
 * \brief Flag to set which pu need change
 *        MPRB from small to large
 */
#define VPUL_STATIC_ALLOC_PU_INSTANCE_LSB					(7)
#define VPUL_STATIC_ALLOC_PU_INSTANCE_MASK					(0xFF)


/** @}*/

extern const enum VPU_PU_TYPES pu_inst2type[];

/**
 *  \brief Allocates resource to a task according to availability in system.
 *  \param vpu_hw VPU HW resource availability representation
 *  \param task VPU task for which resources are required
 *  \param flags indicates mode of operation
 *		as defined in Resource Get Bit Flags definitions
 *
 */
__s32 __vpu_resource_get(struct vpu_hardware *vpu_hw,
				struct vpul_task *task, __u32 flags);

/**
 *  \brief De-Allocates resource from task back to VPU system representation.
 *  \param vpu_hw VPU HW resource availability representation
 *  \param task VPU task for which resources are required
 */
__s32 __vpu_resource_put(struct vpu_hardware *vpu_hw,
				struct vpul_task *task);

/** @}*/

#endif
