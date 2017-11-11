/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_DCP_H
#define FIMC_IS_HW_DCP_H

#include "fimc-is-hw-control.h"
#include "fimc-is-interface-ddk.h"

struct fimc_is_hw_dcp {
	struct fimc_is_lib_isp		lib[FIMC_IS_STREAM_COUNT];
	struct fimc_is_lib_support	*lib_support;
	struct lib_interface_func	*lib_func;
	struct dcp_param_set		param_set[FIMC_IS_STREAM_COUNT];
};

int fimc_is_hw_dcp_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc, int id);
int fimc_is_hw_dcp_open(struct fimc_is_hw_ip *hw_ip, u32 instance, u32 *size);
int fimc_is_hw_dcp_init(struct fimc_is_hw_ip *hw_ip, struct fimc_is_group *group,
	bool flag, u32 module_id);
int fimc_is_hw_dcp_object_close(struct fimc_is_hw_ip *hw_ip, u32 instance);
int fimc_is_hw_dcp_close(struct fimc_is_hw_ip *hw_ip, u32 instance);
int fimc_is_hw_dcp_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map);
int fimc_is_hw_dcp_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map);
int fimc_is_hw_dcp_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map);
int fimc_is_hw_dcp_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map);
void fimc_is_hw_dcp_update_param(struct dcp_param *param,
	struct dcp_param_set *param_set, u32 lindex, u32 hindex);
int fimc_is_hw_dcp_get_meta(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map);
int fimc_is_hw_dcp_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type);
int fimc_is_hw_dcp_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 index,
	u32 instance, ulong hw_map);
int fimc_is_hw_dcp_apply_setfile(struct fimc_is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map);
int fimc_is_hw_dcp_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map);
void fimc_is_hw_dcp_size_dump(struct fimc_is_hw_ip *hw_ip);
#endif
