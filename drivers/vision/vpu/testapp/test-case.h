/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_TEST_CASE_H_
#define VPU_TEST_CASE_H_

#define VPU_MAX_TEST_CASES	100

struct vpu_test_case {
	int			id;
	char			name[30];
	void *			(*func)(void *);
	unsigned int		flags;
};

struct vpu_test_set {
	struct vpu_test_case	tc[VPU_MAX_TEST_CASES];
	int			tc_cnt;
};

void vpu_test_verify_init(struct vpu_test_set *test_set);
void vpu_test_unit_init(struct vpu_test_set *test_set);

#endif