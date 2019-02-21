/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_TEST_UTIL_H_
#define VPU_TEST_UTIL_H_

#include <stdio.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <vs4l.h>
#include <lib/vpul-ds.h>

int util_g_size_from(struct vs4l_format *format);
int util_memdump16(unsigned short *start, unsigned short *end);

struct vpul_pu * get_pu_by_index(
	const struct vpul_task * task,
	__u32 proc_idx,
	__u32 sc_idx,
	__u32 pu_idx);

__u32 vpu_translator_get_task_ds_size(
	__u32 t_num_of_vertices,
	__u32 t_num_of_subchains,
	__u32 t_num_of_pus,
	__u32 t_num_of_updatable_pus

	);

__s32 vpu_translator_create_task_ds(
	struct vpul_task *task,
	__u32 size_allocated,
	__u32 t_num_of_vertices,
	__u32 t_num_of_subchains,
	__u32 t_num_of_pus,
	__u32 t_num_of_updatable_pus,
	...);

__s32 vpu_translator_create_task_ds_from_array(
	struct vpul_task *task,
	__u32 size_allocated,
	__u32 t_num_of_vertices,
	__u32 t_num_of_subchains,
	__u32 t_num_of_pus,
	__u32 t_num_of_updatable_pus,
	const __u32 *subchains_for_vertices,
	const __u32 *pus_for_subchains);

extern int loglevel;
#define TEST_LOG(fmt, ...) do { \
		if (loglevel) printf(fmt, ##__VA_ARGS__); \
	} while(0)

#endif
