/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _VPU_ERRNO_H_
#define _VPU_ERRNO_H_

#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/errno.h>

#elif defined(__linux__) || defined(_WIN32)
#include <errno.h>
#include <stdio.h>
/*! Win32 / non kernel mode */
#define KERN_ALERT      "[ALERT] " /* action must be taken immediately */
#define KERN_CRIT       "[CRITICAL] "    /* critical conditions */
#define KERN_ERR        "[ERROR] "    /* error conditions */
#define KERN_WARNING    "[WARNNING]"    /* warning conditions */
#define KERN_NOTICE     "[NOTE]"    /* normal but significant condition */
#define KERN_INFO       ""    /* informational */
#define KERN_DEBUG      "[DBG]"    /* debug-level messages */

#define pr_err(fmt, ...) \
	printf(KERN_ERR fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...) \
	printf(KERN_WARNING fmt, ##__VA_ARGS__)
#define pr_notice(fmt, ...) \
	printf(KERN_NOTICE fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...) \
	printf(KERN_INFO fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...) \
	printf(KERN_DEBUG fmt, ##__VA_ARGS__)


#define __must_check

#define MAX_ERRNO       4095

#define NK_INLINE __inline

#define IS_ERR_VALUE(x) ((x) >= (unsigned long)-MAX_ERRNO)

static NK_INLINE void * __must_check ERR_PTR(long error)
{
	return (void *) error;
}

static NK_INLINE long __must_check PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static NK_INLINE long __must_check IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static NK_INLINE long __must_check IS_ERR_OR_NULL(const void *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}
#else
#error Platform not supported
#endif /* !__KERNEL__ */

/* Start of VPU unique error codes */
#define VPU_ERRNO_BASE			200

/*! \brief error/status codes returned from VPU Driver functions. */

/*!< Successful */
/* TODO: consider return 0 in case of success */
#define VPU_STATUS_SUCCESS		(VPU_ERRNO_BASE + 0)
/*! The function had no effect. */
#define VPU_STATUS_NOP			(VPU_ERRNO_BASE + 1)
/*! The operation was successfully canceled */
#define VPU_STATUS_CANCELED		(VPU_ERRNO_BASE + 2)
/*! Main process function is not finished (should be called again) */
#define VPU_STATUS_IN_PROGRESS		(VPU_ERRNO_BASE + 3)

/*! Unexplained failure */
#define VPU_STATUS_FAILURE		(VPU_ERRNO_BASE + 4)
/*! Operation would block in a forbidden context */
#define VPU_STATUS_WOULD_BLOCK		(VPU_ERRNO_BASE + 5)
/*! Failure due to insufficient resources */
#define VPU_STATUS_INSUFFICIENT_RESOURCES	(VPU_ERRNO_BASE + 6)
/*! Failure due to insufficient space */
#define VPU_STATUS_INSUFFICIENT_SPACE	(VPU_ERRNO_BASE + 7)
/*! The resource is already used */
#define VPU_STATUS_IN_USE		(VPU_ERRNO_BASE + 8)
/*! The object doesn't exist */
#define VPU_STATUS_NOT_EXIST		(VPU_ERRNO_BASE + 9)
/*! The object already exists */
#define VPU_STATUS_DUPLICATED		(VPU_ERRNO_BASE + 10)
/*! Bad parameters values. */
#define VPU_STATUS_BAD_PARAMS		(VPU_ERRNO_BASE + 11)
/*! Inappropriate state for this action to take place. */
#define VPU_STATUS_INAPPROPRIATE_STATE	(VPU_ERRNO_BASE + 12)
/*! Queue is empty.  */
#define VPU_STATUS_QUEUE_EMPTY		(VPU_ERRNO_BASE + 13)
/*! Queue is Full  */
#define VPU_STATUS_QUEUE_FULL		(VPU_ERRNO_BASE + 14)
/*! An underflow occurred */
#define VPU_STATUS_UNDERFLOW		(VPU_ERRNO_BASE + 15)
/*! Function is not supported */
#define VPU_STATUS_NOT_IMPLEMENTED	(VPU_ERRNO_BASE + 16)
/*! Unexplained failure */
#define VPU_STATUS_IO_FAILURE		(VPU_ERRNO_BASE + 17)
/*! Illegal task data structure */
#define VPU_STATUS_ILLEGAL_TASK		(VPU_ERRNO_BASE + 18)
/*! Illegal VPU Configuration */
#define VPU_STATUS_ILLEGAL_VPU_CONFIG		(VPU_ERRNO_BASE + 19)
/*! CPU operation not supported */
#define VPU_STATUS_CPU_OPER_NOT_SUPORTED	(VPU_ERRNO_BASE + 20)
/*! Last status value marker */
/*! PreLoad chain configuration mismatch with PU configuration */
#define VPU_STATUS_PRELOAD_MISMATCH		(VPU_ERRNO_BASE + 21)


/*!< Determines whether a certain return code represent a success. */
#define VPU_STATUS_IS_SUCCESSFUL(x) ((x) < VPU_STATUS_FAILURE)
/*!< Determines whether a certain return code represent a failure. */
#define VPU_STATUS_IS_FAILURE(x)    ((x) >= VPU_STATUS_FAILURE)

/* print debug functions */
#define VPU_LIB_DEBUG

#define VPU_ERRO(fmt, ...) pr_err("vpu lib: " fmt, ##__VA_ARGS__)
#define VPU_WARN(fmt, ...) pr_warn("vpu lib: " fmt, ##__VA_ARGS__)
#define VPU_NOTE(fmt, ...) pr_notice("vpu lib: " fmt, ##__VA_ARGS__)
#define VPU_INFO(fmt, ...) pr_info("vpu lib: " fmt, ##__VA_ARGS__)
#ifdef VPU_LIB_DEBUG
/* Consider replace pr_debug with dev_dbg */
#define VPU_DEBG(fmt, ...) pr_debug("vpu lib: " fmt, ##__VA_ARGS__)
#else
#define VPU_DEBG(fmt, ...)
#endif

#define VPU_DEBUG_LEVEL_MAX	7

#endif /* _VPU_ERRNO_H_ */
