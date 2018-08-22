/*
 * Samsung Exynos SoC series SCore driver
 *
 * Definition of ioctl for controlling SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_IOCTL_H__
#define __SCORE_IOCTL_H__

#include <linux/fs.h>

/**
 * struct score_ioctl_request - Request data for communication with user space
 * @packet_fd: fd of user packet memory
 * @ret: return value of SCore device
 * @reserved: reserved parameter
 * @timestamp: time to measure performance
 */
struct score_ioctl_request {
	int			packet_fd;
	int                     ret;
	int                     reserved[4];
	struct timespec         timestamp[10];
};

/**
 * struct score_ioctl_get_dvfs - Data to get DVFS information
 * @ret: additional result value
 * @cmd: command type about DVFS
 * @default_qos: [out] default qos value
 * @current_qos: [out] current qos value
 * @max_qos: [out] maximum qos value
 * @min_qos: [out] minimum qos value
 * @reserved: reserved parameter
 */
struct score_ioctl_get_dvfs {
	int                     ret;
	int                     cmd;
	int                     qos_count;
	int                     default_qos;
	int                     current_qos;
	int                     max_qos;
	int                     min_qos;
	int                     reserved[4];
};

/**
 * struct score_ioctl_set_dvfs - Data to set DVFS request
 * @ret: additional result value
 * @cmd: command type about DVFS
 * @request_qos: [in] request qos value
 * @current_qos: [out] current qos value
 * @reserved: reserved parameter
 */
struct score_ioctl_set_dvfs {
	int                     ret;
	int                     cmd;
	int                     request_qos;
	int                     current_qos;
	int                     reserved[4];
};

/**
 * struct score_ioctl_secure - Data for executing at Secure O/S
 * @ret: result value about command
 * @cmd: command type about Secure O/S
 * @reserved: reserved parameter
 */
struct score_ioctl_secure {
	int                     ret;
	int                     cmd;
	int                     reserved[4];
};

/**
 * struct score_ioctl_request - Request data for communication with user space
 * @packet_fd: fd of user packet memory
 * @ret: return value of SCore device
 * @reserved: reserved parameter
 * @timestamp: time to measure performance
 */
struct score_ioctl_request_nonblock {
	int			packet_fd;
	int                     ret;
	int			kctx_id;
	int			task_id;
	int                     reserved[4];
	struct timespec         timestamp[10];
};

/**
 * struct score_ioctl_test - Data for verification
 * @ret: result value of test
 * @cmd: command type about test
 * @size: additional data size for test
 * @addr: start address of additional data for test
 */
struct score_ioctl_test {
	int                     ret;
	int                     cmd;
	int                     size;
	unsigned long           addr;
};

union score_ioctl_arg {
	struct score_ioctl_request req;
	struct score_ioctl_get_dvfs dvfs_info;
	struct score_ioctl_set_dvfs dvfs_req;
	struct score_ioctl_secure sec;
	struct score_ioctl_request_nonblock req_nb;
	struct score_ioctl_test test;
};

/**
 * struct score_ioctl_ops - Operations possible at score_ioctl
 * @score_ioctl_request: send command to SCore device, receive result
 *			from SCore device and send result to user
 * @score_ioctl_get_dvfs: get information of qos value for DVFS
 * @score_ioctl_set_dvfs: send command to control DVFS
 * @score_ioctl_secure: send command for execution at Secure O/S
 * @score_ioctl_test: test operation for verification
 */
struct score_ioctl_ops {
	int (*score_ioctl_request)(struct file *file,
			struct score_ioctl_request *arg);
	int (*score_ioctl_get_dvfs)(struct file *file,
			struct score_ioctl_get_dvfs *arg);
	int (*score_ioctl_set_dvfs)(struct file *file,
			struct score_ioctl_set_dvfs *arg);
	int (*score_ioctl_secure)(struct file *file,
			struct score_ioctl_secure *arg);
	int (*score_ioctl_request_nonblock)(struct file *file,
			struct score_ioctl_request_nonblock *arg);
	int (*score_ioctl_request_wait)(struct file *file,
			struct score_ioctl_request_nonblock *arg);
	int (*score_ioctl_test)(struct file *file,
			struct score_ioctl_test *arg);
};

long score_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#if defined(CONFIG_COMPAT)
long score_compat_ioctl32(struct file *file, unsigned int cmd,
		unsigned long arg);
#else
static inline long score_compat_ioctl32(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	return 0;
}
#endif

#endif
