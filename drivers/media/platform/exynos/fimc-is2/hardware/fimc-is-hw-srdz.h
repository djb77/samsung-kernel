/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_SUBDEV_SRDZ_H
#define FIMC_IS_SUBDEV_SRDZ_H

#include "fimc-is-hw-control.h"
#include "fimc-is-interface-library.h"
#include "fimc-is-param.h"

#define SRDZ_ROUND_UP(x, d) \
	((d) * (((x) + ((d) - 1)) / (d)))

#define GET_SRDZ_HW_CAP(hwip) \
	((hwip->priv_info) ? &((struct fimc_is_hw_srdz *)hw_ip->priv_info)->cap : NULL)
#define GET_ENTRY_FROM_OUTPUT_ID(output_id) \
	(output_id + ENTRY_M0P)

enum srdz_img_format {
	SRDZ_YUV422_1P_YUYV = 0,
	SRDZ_YUV422_1P_YVYU,
	SRDZ_YUV422_1P_UYVY,
	SRDZ_YUV422_1P_VYUY,
	SRDZ_YUV422_2P_UFIRST,
	SRDZ_YUV422_2P_VFIRST,
	SRDZ_YUV422_3P,
	SRDZ_YUV420_2P_UFIRST,
	SRDZ_YUV420_2P_VFIRST,
	SRDZ_YUV420_3P
};

enum srdz_io_type {
	HW_SRDZ_OTF_OUTPUT,
	HW_SRDZ_DMA_INPUT,
	HW_SRDZ_DMA_OUTPUT,
};

enum srdz_cap_enum {
	SRDZ_CAP_NOT_SUPPORT = 0,
	SRDZ_CAP_SUPPORT,
};

typedef struct {
	/* TODO */
} srdz_setfile_contents;

struct hw_api_srdz_setfile {
	u32 setfile_version;

	srdz_setfile_contents contents[2];
};

struct fimc_is_hw_srdz {
	struct	hw_api_srdz_setfile *setfile;

	bool	rep_flag[FIMC_IS_STREAM_COUNT];
	u32	instance;
};

int fimc_is_hw_srdz_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc, int id);
int fimc_is_hw_srdz_open(struct fimc_is_hw_ip *hw_ip, u32 instance, u32 *size);
int fimc_is_hw_srdz_init(struct fimc_is_hw_ip *hw_ip, struct fimc_is_group *group,
	bool flag, u32 module_id);
int fimc_is_hw_srdz_close(struct fimc_is_hw_ip *hw_ip, u32 instance);
int fimc_is_hw_srdz_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map);
int fimc_is_hw_srdz_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map);
int fimc_is_hw_srdz_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map);

int fimc_is_hw_srdz_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map);
int fimc_is_hw_srdz_update_param(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *param, u32 lindex, u32 hindex, u32 instance);
bool fimc_is_hw_srdz_frame_done(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	int done_type);
int fimc_is_hw_srdz_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type);
int fimc_is_hw_srdz_reset(struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_srdz_clear_interrupt(struct fimc_is_hw_ip *hw_ip);
int fimc_is_hw_srdz_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 index,
	u32 instance, ulong hw_map);
int fimc_is_hw_srdz_apply_setfile(struct fimc_is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map);
int fimc_is_hw_srdz_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map);

int fimc_is_hw_srdz_dma_input(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance);
int fimc_is_hw_srdz_dma_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);
int fimc_is_hw_srdz_adjust_input_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format);
int fimc_is_hw_srdz_adjust_output_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format);
int fimc_is_hw_srdz_check_format(enum srdz_io_type type, u32 format, u32 bit_width,
	u32 width, u32 height);
void fimc_is_hw_srdz_size_dump(struct fimc_is_hw_ip *hw_ip);
#endif
