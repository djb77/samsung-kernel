/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_LIB_H
#define FIMC_IS_LIB_H

#include "fimc-is-lib-fd.h"
#include "fimc-is-core.h"
#include "fimc-is-param.h"
#include "fimc-is-metadata.h"

/* FD heap size is calculated by the appointed  formula  */
#define FD_HEAP_SIZE			((FD_MAP_WIDTH * FD_MAP_HEIGHT / 16) + (130 * 1024))
#define FD_LIB_NORMAL_THREAD
#define MAX_FIMC_IS_LIB_TASK		(1)
#define FD_ALIGN(x, alignbyte)		((((x)+(alignbyte)-1) / (alignbyte)) * (alignbyte))
#define SET32(addr, data)		(*(volatile u32 *)(addr) = (data))
#define SET64(addr, data)		(*(volatile ulong *)(addr) = (data))
#define FD_MAX_MAP_WIDTH		640
#define FD_MAX_MAP_HEIGHT		480
#define FD_MAP_WIDTH			640
#define FD_MAP_HEIGHT			480
#define FD_MAX_SCENARIO			(64)
#define FD_MAX_SETFILE			(16)
#define FD_SET_FILE_MAGIC_NUMBER	(0x12345679)

typedef struct {
	FD_INFO *(*fd_version_get_func)(void);
	FDSTATUS (*fd_create_heap_func)(void *, unsigned int, FD_HEAP * *);
	FDSTATUS (*fd_create_detector_func)(FD_HEAP *, FD_DETECTOR_CFG *, FD_DETECTOR * *);
	FDSTATUS (*fd_detect_face_func)(FD_DETECTOR *, FD_IMAGE *, void *, void *);
	FD_FACE *(*fd_enum_face_func)(FD_DETECTOR *, FD_FACE *);
	s32 (*fd_get_shift_func)(u32, u32);
	s32 (*lhfd_get_shift_func)(u32 width, u32 height);
	FDSTATUS (*clear_face_list_func)(FD_DETECTOR *det);
	void (*destroy_func)(FD_DETECTOR *det);
	FDSTATUS (*update_configure_func)(FD_DETECTOR *, const FD_DETECTOR_CFG * cfg);
} fd_lib_str;

typedef fd_lib_str * (*fd_lib_func_t)(void **);

enum fimc_is_fd_buffer_frame {
	PREV_FRAME = 0,
	DONE_FRAME,
	END_BUFFER
};

enum fimc_is_lib_fd_state {
	FD_LIB_CONFIG,
	FD_LIB_MAP_INIT,
	FD_LIB_RUN
};

struct fd_addr_str {
	ulong				kvaddr;
	u32				dvaddr;
};

struct fd_map_addr_str {
	struct fd_addr_str		map1;
	struct fd_addr_str		map2;
	struct fd_addr_str		map3;
	struct fd_addr_str		map4;
	struct fd_addr_str		map5;
	struct fd_addr_str		map6;
	struct fd_addr_str		map7;
	struct fd_addr_str		map8;
};

struct fimc_is_lib_fd_result {
	u32				detect_face_num;
	FD_FACE				rectangles[CAMERA2_MAX_FACES];
};

struct fd_lib_hw_data {
	u32				structSize;
	u32				width;
	u32				height;
	void				*map1;
	void				*map2;
	void				*map3;
	void				*map4;
	void				*map5;
	void				*map6;
	u32				sat;
	u32				shift;
	u32				k;
	u32				up;
	void				*map7;
};

struct fd_setfile_element {
	u32 addr;
	u32 size;
};

struct fd_setfile_info {
	int				version;
	/* the number of setfile each sub ip has */
	u32				num;
	/* which subindex is used at this scenario */
	u32				index[FD_MAX_SCENARIO];

	ulong				addr[FD_MAX_SETFILE];
	u32				size[FD_MAX_SETFILE];
};

enum fd_setfile_type {
	FD_SETFILE_V2 = 2,
	FD_SETFILE_V3 = 3,
	FD_SETFILE_MAX
};

enum fd_format_mode {
	FD_LIB_FORMAT_PREVIEW, /* OTF mode */
	FD_LIB_FORMAT_PLAYBACK, /* M2M mode */
	FD_LIB_FORMAT_END
};

struct fimc_is_lib_fd {
	u32				frame_count;
	u32				prev_map_width;
	u32				prev_map_height;

	FD_HEAP				*heap;
	FD_DETECTOR			*det;
	FD_DETECTOR_CFG			cfg_user;
	FDD_DATA			data_a;
	FDD_DATA			data_b;
	FDD_DATA			data_c;
	FDD_DATA			*data_fd_lib;
	bool				update_cfg;
	u32				num_detect;
	u32				prev_num_detect;
	s32				shift;
	s32				orientation;
	struct fd_map_addr_str		map_addr_a;
	struct fd_map_addr_str		map_addr_b;
	struct fd_map_addr_str		map_addr_c;

	unsigned long			fd_dma_select;
	unsigned long			fd_lib_select;

	unsigned long			map_history[END_BUFFER];

	u8				play_mode;
	enum facedetect_mode		fd_detect_mode;
	fd_lib_str			*fd_lib_func;

	struct camera2_fd_uctl		fd_uctl;
	struct camera2_fd_uctl		prev_fd_uctl;
	struct camera2_fd_udm		fd_udm;

	struct fd_setfile_info		setfile;
	enum fd_format_mode		format_mode;
};

struct fimc_is_lib_work {
	struct kthread_work		work;
	void				*data;
};


struct fimc_is_lib {
	u32				id;
	u32				work_index;
	spinlock_t			work_lock;
	struct task_struct		*task;
	struct kthread_worker		worker;
	struct fimc_is_lib_work		work[MAX_FIMC_IS_LIB_TASK];
	unsigned long			state;

	void				*lib_data;
	struct fimc_is_device_ischain   *device;
};

enum fd_lib_play_mode {
	FD_PLAY_STATIC = 0,
	FD_PLAY_TRACKING
};

enum fd_sel_data {
	FD_SEL_DATA_A = 1,
	FD_SEL_DATA_B,
	FD_SEL_DATA_C
};

struct fd_setfile_header_v2 {
	u32	magic_number;
	u32	scenario_num;
	u32	subip_num;
	u32	setfile_offset;
};

struct fd_setfile_header_v3 {
	u32	magic_number;
	u32	designed_bit;
	char	version_code[4];
	char	revision_code[4];
	u32	scenario_num;
	u32	subip_num;
	u32	setfile_offset;
};

struct fd_setfile_half {
	u32 setfile_version;
	u32 skip_frames;
	u32 flag;
	u32 min_face;
	u32 max_face;
	u32 central_lock_area_percent_w;
	u32 central_lock_area_percent_h;
	u32 central_max_face_num_limit;
	u32 frame_per_lock;
	u32 smooth;
	s32 boost_fd_vs_fd;
	u32 max_face_cnt;
	s32 boost_fd_vs_speed;
	u32 min_face_size_feature_detect;
	u32 max_face_size_feature_detect;
	s32 keep_faces_over_frame;
	s32 keep_limit_no_face;
	u32 frame_per_lock_no_face;
	s32 lock_angle[16];
};

struct fd_setfile {
	struct fd_setfile_half preview;
	struct fd_setfile_half playback;
};

int fimc_is_lib_fd_load(void);
int fimc_is_lib_fd_open(struct fimc_is_lib *fd_lib);
int fimc_is_lib_fd_close(struct fimc_is_lib *fd_lib);
int fimc_is_lib_fd_create_detector(struct fimc_is_lib *fd_lib, struct vra_param *fd_param);
int fimc_is_lib_fd_thread_init(struct fimc_is_lib *fd_lib);
int fimc_is_lib_fd_map_init(struct fimc_is_lib *fd_lib, ulong *kvaddr_fd, u32 *dvaddr_fd,
		ulong kvaddr_fshared, struct vra_param *fd_param);
int fimc_is_lib_fd_map_size(FDD_DATA *m_data, struct fd_map_addr_str *m_addr,
		ulong kbase, ulong dbase, ulong shared);
int fimc_is_lib_fd_select_buf(struct fimc_is_lib_fd *lib_data, struct camera2_fd_uctl *fdUd,
		ulong kfshared, ulong dfshared);
int fimc_is_lib_fd_load_setfile(struct fimc_is_lib *fd_lib, ulong setfile_addr);
int fimc_is_lib_fd_apply_setfile(struct fimc_is_lib *fd_lib, struct vra_param *fd_param);
int fimc_is_lib_fd_convert_orientation(u32 src_orientation, s32 *fd_orientation);

ulong fimc_is_lib_fd_m_align(ulong size_in_byte, ulong align, ulong *alloc_addr);

void fimc_is_lib_fd_run(struct fimc_is_lib *fd_lib);
void fimc_is_lib_fd_fn(struct kthread_work *work);
void fimc_is_lib_fd_trigger(struct fimc_is_lib *fd_lib);
void fimc_is_lib_fd_size_check(struct fimc_is_lib *fd_lib, struct param_fd_config *lib_input,
		struct fimc_is_subdev_path *preview, u32 hw_width, u32 hw_height);
#endif
