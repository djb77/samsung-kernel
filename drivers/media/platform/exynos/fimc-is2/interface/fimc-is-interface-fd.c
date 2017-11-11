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

#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/of_reserved_mem.h>
#include <linux/memblock.h>

#include "fimc-is-interface-fd.h"
#include "fimc-is-config.h"
#include "fimc-is-regs.h"
#include "fimc-is-hw.h"

#ifndef ENABLE_IS_CORE
#include "fimc-is-interface-library.h"
#endif

#ifdef FD_USE_SHARED_REGION
/* FD reserved memory address */
void *fd_vaddr;
#endif

/* default FD lock angle */
static short lock_angle_default[32] = {0, 90, -90, 180, FD_ANGLE_END, };

#ifdef FD_USE_SHARED_REGION
static int __init fimc_is_fd_reserved_mem_setup(struct reserved_mem *rmem)
{
	int ret = 0;

	pr_info("fimc_is_fd reserved memory: %s=%#lx@%pa\n",
			rmem->name, (unsigned long) rmem->size, &rmem->base);

	ret = memblock_is_region_reserved(rmem->base, rmem->size);
	if (!ret) {
		err("fd memblock fail");
		ret = -EINVAL;
	}

	fd_vaddr = phys_to_virt(rmem->base);
	info("[FD] vaddr: %#lx\n", (ulong)fd_vaddr);

	return ret;
}
RESERVEDMEM_OF_DECLARE(fimc_is_fd, "fd-lib-shmem", fimc_is_fd_reserved_mem_setup);
#endif

int fimc_is_lib_fd_map_copy(struct file *fd_map, FDD_DATA *data, int map_cnt)
{
	int ret = 0;
	long fsize, nread;
	u32 size;
	u8 *buf = NULL;

	BUG_ON(!fd_map);
	BUG_ON(!data);

	size = data->map_height * data->map_width;

	fsize = fd_map->f_path.dentry->d_inode->i_size;
	fsize -= 1;

	info("%s: start, file path: size %ld Bytes\n", __func__, fsize);
	buf = vmalloc(fsize);
	if (!buf) {
		err("failed to allocate memory\n");
		ret = -ENOMEM;
		goto p_err;
	}

	nread = vfs_read(fd_map, (char __user *)buf, fsize, &fd_map->f_pos);
	if (nread != fsize) {
		err("failed to read firmware file, %ld Bytes\n", nread);
		ret = -EINVAL;
		goto p_err;
	}

	info("[FD] fd_lib_map %d read", map_cnt);
	switch (map_cnt) {
	case 0:
		memcpy(data->map2, (void *)buf, fsize);
		break;
	case 1:
		memcpy(data->map3, (void *)buf, fsize);
		break;
	case 2:
		memcpy(data->map4, (void *)buf, fsize);
		break;
	case 3:
		memcpy(data->map5, (void *)buf, fsize);
		break;
	case 4:
		memcpy(data->map1, (void *)buf, fsize);
		break;
	case 5:
		memcpy((data->map1) + (size), (void *)buf, fsize);
		break;
	case 6:
		memcpy((data->map1) + (size) + (size/4), (void *)buf, fsize);
		break;
	case 7:
		memcpy(data->map6, (void *)buf, fsize);
		break;
	default:
		break;
	}

p_err:
	return ret;
}

int fimc_is_lib_fd_map_load(FDD_DATA *data)
{
	int ret = 0;
	int m_cnt;
	char map_dir[20];
	mm_segment_t old_fs;
	struct file *fd_map[8] = {NULL};

	for (m_cnt = 0; m_cnt < 8; m_cnt++) {
		snprintf(map_dir, sizeof(map_dir), "/data/map%d.bin", m_cnt);
		fd_map[m_cnt] = filp_open(map_dir, O_RDONLY, 0);
		if (IS_ERR(fd_map[m_cnt])) {
			err("filp_open map%d fail!!\n",m_cnt);
			ret = -ENOENT;
			goto p_err;
		}
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	for (m_cnt = 0; m_cnt < 8; m_cnt++) {
		if (fimc_is_lib_fd_map_copy(fd_map[m_cnt], data, m_cnt)) {
			err("filp_open fail. status: (%d)\n", ret);
			ret = -EINVAL;
			goto p_err;
		}
	}

	set_fs(old_fs);

p_err:
	for (m_cnt = 0; m_cnt < 8; m_cnt++)
		if (!IS_ERR(fd_map[m_cnt]))
			filp_close(fd_map[m_cnt], NULL);

	return ret;
}

int fimc_is_lib_fd_open(struct fimc_is_lib *fd_lib)
{
	int ret = 0;
	struct fimc_is_lib_fd *fd_data = NULL;

	fd_data = kzalloc(sizeof(struct fimc_is_lib_fd), GFP_KERNEL);
	if (unlikely(!fd_data)) {
		err("fimc_is_fd_lib is NULL");
		ret = -ENOMEM;
		goto err;
	}
	fd_lib->lib_data = fd_data;

	memset(fd_data, 0, sizeof(*fd_data));
	memset(&fd_data->setfile, 0, sizeof(struct fd_setfile_info));
	memset(&fd_data->cfg_user, 0, sizeof(FD_DETECTOR_CFG));
	memset(&(fd_data->data_a), 0, sizeof(FDD_DATA));
	memset(&(fd_data->data_b), 0, sizeof(FDD_DATA));
	memset(&(fd_data->data_c), 0, sizeof(FDD_DATA));
	memset(&(fd_data->map_addr_a), 0, sizeof(fd_data->map_addr_a));
	memset(&(fd_data->map_addr_b), 0, sizeof(fd_data->map_addr_b));
	memset(&(fd_data->map_addr_c), 0, sizeof(fd_data->map_addr_c));

	fd_data->det = NULL;
	fd_data->heap = NULL;
	fd_data->format_mode = FD_LIB_FORMAT_END;
	fd_data->prev_map_width = FD_MAP_WIDTH;
	fd_data->prev_map_height = FD_MAP_HEIGHT;
	fd_data->cfg_user.structSize = sizeof(FD_DETECTOR_CFG);

	/* FD library function call before certainly library load. */
#ifdef FD_USE_SHARED_REGION
	fd_data->fd_lib_func = ((fd_lib_func_t)(fd_vaddr))((void **)NULL);
#else
	fd_data->fd_lib_func = ((fd_lib_func_t)(FD_LIB_BASE))((void **)NULL);
#endif

	/* initial buffer is set A */
	set_bit(FD_SEL_DATA_A, &fd_data->fd_dma_select);
	set_bit(FD_SEL_DATA_A, &fd_data->fd_lib_select);
	fd_data->data_fd_lib = &fd_data->data_b;

	clear_bit(FD_LIB_CONFIG, &fd_lib->state);
	clear_bit(FD_LIB_MAP_INIT, &fd_lib->state);
	clear_bit(FD_LIB_RUN, &fd_lib->state);

	return ret;
err:
	if (fd_data)
		kfree(fd_data);

	return ret;
}

int fimc_is_lib_fd_close(struct fimc_is_lib *fd_lib)
{
	FDSTATUS fd_status = FDS_OK;
	struct fimc_is_lib_fd *fd_data;
	fd_lib_str *lib_func;

	BUG_ON(!fd_lib);

	fd_data = fd_lib->lib_data;
	if (unlikely(!fd_data))
		return fd_status;

	lib_func = fd_data->fd_lib_func;

	if (fd_lib->task)
		kthread_stop(fd_lib->task);

	if (fd_data->det) {
		fd_status = lib_func->clear_face_list_func(fd_data->det);
		if (fd_status != FDS_OK)
			err("[FD] Clear face fail");
		lib_func->destroy_func(fd_data->det);
		fd_data->det = NULL;
	}

	/* This pointer equals to dummy_heap */
	if (fd_data->heap != NULL) {
		vfree(fd_data->heap);
		fd_data->heap = NULL;
	}

	kfree(fd_data);
	fd_lib->lib_data = NULL;

	clear_bit(FD_LIB_CONFIG, &fd_lib->state);
	clear_bit(FD_LIB_MAP_INIT, &fd_lib->state);
	clear_bit(FD_LIB_RUN, &fd_lib->state);

	return fd_status;
}

int fimc_is_lib_fd_create_detector(struct fimc_is_lib *fd_lib, struct vra_param *fd_param)
{
	int ret = 0;
	void *dummy_heap = NULL;
	struct fimc_is_lib_fd *data = NULL;

	BUG_ON(!fd_lib);
	BUG_ON(!fd_lib->lib_data);
	BUG_ON(!fd_param);

	if (test_bit(FD_LIB_CONFIG, &fd_lib->state))
		goto exit;

	data = fd_lib->lib_data;

	dummy_heap = vmalloc(FD_HEAP_SIZE);
	if (!dummy_heap) {
		err("fimc_is_fd_lib_fd_heap is NULL");
		ret = -ENOMEM;
		goto exit;
	}

	ret = fimc_is_lib_fd_thread_init(fd_lib);
	if (ret) {
		err("failed to fimc_is_lib_fd_thread_init (%d)=n", ret);
		goto err_heap;
	}

	/* FD lib assigned heap memory like fd_data->heap = dummy_heap */
	ret = (int)data->fd_lib_func->fd_create_heap_func(dummy_heap,
			FD_HEAP_SIZE, &data->heap);
	if (ret != (int)FDS_OK) {
		err("[FD] FD heap create fail: %d\n", ret);
		ret = -ENOMEM;
		goto err_heap;
	}

	ret = fimc_is_lib_fd_apply_setfile(fd_lib, fd_param);
	if (ret) {
		err("[FD] FD apply setfile fail: %d\n", ret);
		goto err_heap;
	}

	ret = (int)data->fd_lib_func->fd_create_detector_func(data->heap,
			&data->cfg_user, &data->det);
	if (ret != (int)FDS_OK) {
		err("[FD] Couldn't create FD Detector: %d\n", ret);
		ret = -ENOMEM;
		goto err_heap;
	}

	clear_bit(FD_LIB_RUN, &fd_lib->state);
	set_bit(FD_LIB_CONFIG, &fd_lib->state);

	info("[FD] FD Detector create successe\n");

	return 0;

err_heap:
	vfree(dummy_heap);
exit:
	return ret;
}

void fimc_is_lib_fd_run(struct fimc_is_lib *fd_lib)
{
	FD_IMAGE image;
	FD_FACE *face = NULL;
	FDSTATUS status = FDS_FAIL;
	FD_DETECTOR *det = NULL;
	FDD_DATA *data_fd_lib;
	struct fd_lib_hw_data hw_data;
	struct fimc_is_lib_fd *lib_data = NULL;
	u32 detect_num = 0;
	u32 scaled_left, scaled_top, scaled_width, scaled_height;
#ifdef ENABLE_FD_LIB_DIRECT_MAP
	u32 i;
#endif

	BUG_ON(!fd_lib);

	lib_data = fd_lib->lib_data;
	if (!lib_data) {
		err("[FD] lib: fd library is NULL!!");
		return;
	}

	data_fd_lib = lib_data->data_fd_lib;
	if (!data_fd_lib) {
		err("[FD] lib: fd data is NULL!!\n");
		return;
	}

	det = lib_data->det;
	if (!det) {
		err("[FD] lib: fd detector is NULL!!\n");
		return;
	}

	set_bit(lib_data->map_history[DONE_FRAME], &lib_data->fd_lib_select);
	if (test_bit(FD_SEL_DATA_A, &lib_data->fd_lib_select)) {
		data_fd_lib = &lib_data->data_a;
	} else if (test_bit(FD_SEL_DATA_B, &lib_data->fd_lib_select)) {
		data_fd_lib = &lib_data->data_b;
	} else if (test_bit(FD_SEL_DATA_C, &lib_data->fd_lib_select)) {
		data_fd_lib = &lib_data->data_c;
	} else {
		err("[FD] Library map data error\n");
		return;
	}

#ifdef ENABLE_FD_LIB_DIRECT_MAP
	data_fd_lib = &lib_data->data_c;
#endif
	memset(&image, 0, sizeof(image));
	image.structSize = sizeof(image);
	image.width = data_fd_lib->map_width;
	image.height = data_fd_lib->map_height;
	image.stride = image.width;
	image.format = FD_FMT_YUV_422P;
	image.data = (void *)(data_fd_lib);
	image.hwData = (void *)&hw_data;
	image.imageOrientation = lib_data->orientation;

	memset(&hw_data, 0, sizeof(hw_data));
	hw_data.structSize = sizeof(hw_data);
	hw_data.width = image.width;
	hw_data.height = image.height;
	hw_data.map1 = data_fd_lib->map1;
	hw_data.map2 = data_fd_lib->map2;
	hw_data.map3 = data_fd_lib->map3;
	hw_data.map4 = data_fd_lib->map4;
	hw_data.map5 = data_fd_lib->map5;
	hw_data.map6 = data_fd_lib->map6;
	hw_data.map7 = data_fd_lib->map7;
	hw_data.sat = data_fd_lib->sat;
	hw_data.shift = lib_data->fd_lib_func->fd_get_shift_func(hw_data.width, hw_data.height);

#ifdef ENABLE_FD_LIB_DIRECT_MAP
	for (i = 0; i < 256; i++)
		((u8 *)hw_data.map7)[i] = i;
#endif

#ifdef FD_MEASURE_TIME
	TIME_STR(0);
	status = lib_data->fd_lib_func->fd_detect_face_func(det, &image, NULL, NULL);
	TIME_END(0, "fd measure time");

	if (status != FDS_OK)
		info("[FD] detect face fail : %d", status);
#else
	status = lib_data->fd_lib_func->fd_detect_face_func(det, &image, NULL, NULL);
#endif

	data_fd_lib->k = hw_data.k;
	data_fd_lib->up = hw_data.up;
	detect_num = 0;
	while (status == FDS_OK) {
		face = lib_data->fd_lib_func->fd_enum_face_func(lib_data->det, face);
		if (face == NULL)
			break;
		scaled_left = (u32)face->boundRect.left * data_fd_lib->input_width;
		scaled_left = ((scaled_left << 10) / data_fd_lib->map_width) >> 10;
		scaled_top = (u32)face->boundRect.top * data_fd_lib->input_height;
		scaled_top = ((scaled_top << 10) / data_fd_lib->map_height) >> 10;
		scaled_width = (u32)face->boundRect.width * data_fd_lib->input_width;
		scaled_width = ((scaled_width << 10) / data_fd_lib->map_width) >> 10;
		scaled_height = (u32)face->boundRect.height * data_fd_lib->input_height;
		scaled_height = ((scaled_height << 10) / data_fd_lib->map_height) >> 10;
		lib_data->fd_uctl.faceRectangles[detect_num][0] = scaled_left;
		lib_data->fd_uctl.faceRectangles[detect_num][1] = scaled_top;
		lib_data->fd_uctl.faceRectangles[detect_num][2] = scaled_left + scaled_width;
		lib_data->fd_uctl.faceRectangles[detect_num][3] = scaled_top + scaled_height;
		lib_data->fd_uctl.faceScores[detect_num] = face->confidence > 0xff ? 0xff : face->confidence;
		lib_data->fd_uctl.faceIds[detect_num] = face->nTrackedFaceID;
#ifdef ENABLE_FD_LIB_DIRECT_MAP
		info("[FD] lib face position <%d, %d>\n",
				lib_data->fd_uctl.faceRectangles[detect_num][0],
				lib_data->fd_uctl.faceRectangles[detect_num][1]);
		info("[FD] lib face size <%d, %d>\n",
				lib_data->fd_uctl.faceRectangles[detect_num][1],
				lib_data->fd_uctl.faceRectangles[detect_num][2]);
		info("[FD] lib face confidance: %d\n",
				lib_data->fd_uctl.faceScores[detect_num]);
		info("[FD] lib face id: %d\n",
				lib_data->fd_uctl.faceIds[detect_num]);
#endif
		detect_num++;
	}
	lib_data->num_detect = detect_num;

#ifndef ENABLE_FD_LIB_DIRECT_MAP
	clear_bit(FD_SEL_DATA_A, &lib_data->fd_lib_select);
	clear_bit(FD_SEL_DATA_B, &lib_data->fd_lib_select);
	clear_bit(FD_SEL_DATA_C, &lib_data->fd_lib_select);
#endif
}

int fimc_is_lib_fd_load_setfile(struct fimc_is_lib *fd_lib, ulong setfile_addr)
{
	int ret = 0, i;
	int scenario_index = 0, setfile_index = 0;
	u32 scenario_num, subip_num, fd_offset;
	ulong setfile_offset, setfile_base = 0;
	ulong scenario_table = 0;
	ulong setfile_table = 0;
	struct fd_setfile_element *setfile_index_table = NULL;
	struct fd_setfile_header_v3 *header_v3 = NULL;
	struct fd_setfile_header_v2 *header_v2 = NULL;
	struct fimc_is_lib_fd *lib_data = NULL;
	struct fd_setfile *fd_setfile = NULL;

	BUG_ON(!fd_lib);

	if (unlikely(!fd_lib->lib_data)) {
		err("Lib data is NULL in FD library\n");
		ret = -EINVAL;
		goto exit;
	}
	lib_data = (struct fimc_is_lib_fd *)fd_lib->lib_data;

	/* 1. load header information */
	header_v3 = (struct fd_setfile_header_v3 *)setfile_addr;
	if (header_v3->magic_number < FD_SET_FILE_MAGIC_NUMBER) {
		header_v2 = (struct fd_setfile_header_v2 *)setfile_addr;
		if (header_v2->magic_number != (FD_SET_FILE_MAGIC_NUMBER - 1)) {
			err("invalid magic number[0x%08x]", header_v2->magic_number);
			ret = -EINVAL;
			goto exit;
		}

		lib_data->setfile.version = FD_SETFILE_V2;
		scenario_num = header_v2->scenario_num;
		subip_num = header_v2->subip_num;
		setfile_offset = header_v2->setfile_offset;
		scenario_table = (setfile_addr + sizeof(struct fd_setfile_header_v2));
		setfile_base = setfile_offset + setfile_addr;
		setfile_table = ((ulong)scenario_table + (subip_num * scenario_num * sizeof(u32)));
		setfile_index_table = (struct fd_setfile_element *)(setfile_table + (subip_num * sizeof(u32)));
	} else {

		lib_data->setfile.version = FD_SETFILE_V3;
		scenario_num = header_v3->scenario_num;
		subip_num = header_v3->subip_num;
		setfile_offset = header_v3->setfile_offset;
		scenario_table = (setfile_addr + sizeof(struct fd_setfile_header_v3));
		setfile_base = setfile_offset + setfile_addr;
		setfile_table = (scenario_table + (subip_num * scenario_num * sizeof(u32)));
		setfile_index_table = (struct fd_setfile_element *)(setfile_table + (subip_num * sizeof(u32)));
	}
	fd_offset = subip_num - 1;

	for (i = 0; i < subip_num; i++) {
		/* 2. set what setfile index is used at FD scenario */
		for (scenario_index = 0; scenario_index < scenario_num; scenario_index++, scenario_table++) {
			if (i != fd_offset)
				continue;
			else
				lib_data->setfile.index[scenario_index] = scenario_table;
		}

		/* 3. set the number of setfile at FD */
		lib_data->setfile.num = setfile_table;
		setfile_table++;

		/* 4. set FD setfile address and size */
		for (setfile_index = 0; setfile_index < lib_data->setfile.num; setfile_index++) {
			if (i != fd_offset) {
				continue;
			} else {
				lib_data->setfile.addr[setfile_index] = setfile_index_table->addr + setfile_base;
				lib_data->setfile.size[setfile_index] = setfile_index_table->size;
				fd_setfile = (struct fd_setfile *)lib_data->setfile.addr[setfile_index];

				dbg_lib("set_file index: %d\n", setfile_index);
				dbg_lib("setfile_version(%#x), flag(%#x), min_face(%d), max_face(%d)\n",
						fd_setfile->preview.setfile_version, fd_setfile->preview.flag,
						fd_setfile->preview.min_face, fd_setfile->preview.max_face);
				dbg_lib("central_lock_w(%d), central_lock_h(%d), central_max_face(%d)\n",
						fd_setfile->preview.central_lock_area_percent_w,
						fd_setfile->preview.central_lock_area_percent_h,
						fd_setfile->preview.central_max_face_num_limit);
				dbg_lib("frame_per_lock(%d), smooth(%d), max_face_cnt(%d), keep_faces_over_frame(%d)\n",
						fd_setfile->preview.frame_per_lock, fd_setfile->preview.smooth,
						fd_setfile->preview.max_face_cnt, fd_setfile->preview.keep_faces_over_frame);
				dbg_lib("boost_fd_vs_fd(%d), boost_fd_vs_speed(%d), frame_per_lock_no_face(%d)\n",
						fd_setfile->preview.boost_fd_vs_fd,
						fd_setfile->preview.boost_fd_vs_speed,
						fd_setfile->preview.frame_per_lock_no_face);
				dbg_lib("keep_limit_no_face(%d), min_face_size_feature_detect(%d), max_face_size_feature_detect(%d)\n",
						fd_setfile->preview.keep_limit_no_face,
						fd_setfile->preview.min_face_size_feature_detect,
						fd_setfile->preview.max_face_size_feature_detect);
			}
		}
	}

exit:
	return ret;
}

int fimc_is_lib_fd_apply_setfile(struct fimc_is_lib *fd_lib, struct vra_param *fd_param)
{
	int ret = 0;
	struct fd_setfile *setfile = NULL;
	struct fd_setfile_half *setfile_half = NULL;
	struct fimc_is_lib_fd *fd_data = NULL;

	BUG_ON(!fd_lib);
	BUG_ON(!fd_lib->lib_data);
	BUG_ON(!fd_param);

	fd_data = fd_lib->lib_data;

	if (fd_param->otf_input.cmd == OTF_INPUT_COMMAND_ENABLE)
		fd_data->format_mode = FD_LIB_FORMAT_PREVIEW;
	else if (fd_param->dma_input.cmd == OTF_INPUT_COMMAND_ENABLE)
		fd_data->format_mode = FD_LIB_FORMAT_PLAYBACK;
	else {
		err("FD format is undefine\n");
		fd_data->format_mode = FD_LIB_FORMAT_END;
		ret = -EINVAL;
		goto exit;
	}

	if ((fd_data->setfile.version == FD_SETFILE_V2) || (fd_data->setfile.version == FD_SETFILE_V3)) {
		setfile = (struct fd_setfile *)fd_data->setfile.addr[fd_data->setfile.num - 1];
		if (fd_data->format_mode == FD_LIB_FORMAT_PREVIEW) {
			setfile_half = &setfile->preview;
		} else if (fd_data->format_mode == FD_LIB_FORMAT_PLAYBACK) {
			setfile_half = &setfile->playback;
		} else {
			err("setfile format is undefine\n");
			ret = -EINVAL;
			goto exit;
		}

		/* fd library mode: 1. static, 2. tracking */
		if (setfile_half->flag & FDD_TRACKING_MODE)
			fd_data->play_mode = FD_PLAY_TRACKING;
		else
			fd_data->play_mode = FD_PLAY_STATIC;

		fd_data->cfg_user.flags = setfile_half->flag;
		fd_data->cfg_user.minFace = setfile_half->min_face;
		fd_data->cfg_user.maxFace = setfile_half->max_face;
		fd_data->cfg_user.centralLockAreaPercentageW = setfile_half->central_lock_area_percent_w;
		fd_data->cfg_user.centralLockAreaPercentageH = setfile_half->central_lock_area_percent_h;
		fd_data->cfg_user.centralMaxFaceNumberLimitation = setfile_half->central_max_face_num_limit;
		fd_data->cfg_user.framesPerLock = setfile_half->frame_per_lock;
		fd_data->cfg_user.smoothing = setfile_half->smooth;
		fd_data->cfg_user.boostFDvsFP = setfile_half->boost_fd_vs_fd;
		fd_data->cfg_user.maxFaceCount = setfile_half->max_face_cnt;
		fd_data->cfg_user.boostFDvsSPEED = setfile_half->boost_fd_vs_speed;
		fd_data->cfg_user.minFaceSizeFeaturesDetect = setfile_half->min_face_size_feature_detect;
		fd_data->cfg_user.maxFaceSizeFeaturesDetect = setfile_half->max_face_size_feature_detect;
		fd_data->cfg_user.keepFacesOverMoreFrames = setfile_half->keep_faces_over_frame;
		fd_data->cfg_user.keepLimitingWhenNoFace = setfile_half->keep_limit_no_face;
		fd_data->cfg_user.framesPerLockWhenNoFaces = setfile_half->frame_per_lock_no_face;

		if (*(s16 *)(setfile_half->lock_angle) == FD_ANGLE_END)
			fd_data->cfg_user.lockAngles = NULL;
		else
			fd_data->cfg_user.lockAngles = (s16 *)(setfile_half->lock_angle);
	} else {
		/* set default setfile settings */
		fd_data->play_mode = FD_PLAY_TRACKING;
		fd_data->cfg_user.flags = FDD_TRACKING_MODE |
			FDD_COLOR_FILTER |
			FDD_FAST_LOCK |
			FDD_DISABLE_FACE_CONFIRMATION |
			FDD_ENABLE_IMAGE_ORIENTATION |
			FDD_DISABLE_AUTO_ORIENTATION;

		fd_data->cfg_user.minFace = 16;
		fd_data->cfg_user.maxFace = 0;
		fd_data->cfg_user.centralLockAreaPercentageW = 95;
		fd_data->cfg_user.centralLockAreaPercentageH = 90;
		fd_data->cfg_user.centralMaxFaceNumberLimitation = 0;
		fd_data->cfg_user.lockAngles = (s16 *)lock_angle_default;
		fd_data->cfg_user.framesPerLock = 5;
		fd_data->cfg_user.smoothing = 50;
		fd_data->cfg_user.boostFDvsFP = 0;
		fd_data->cfg_user.maxFaceCount = 10;
		fd_data->cfg_user.boostFDvsSPEED = -20;
		fd_data->cfg_user.minFaceSizeFeaturesDetect = 0;
		fd_data->cfg_user.maxFaceSizeFeaturesDetect = 0;
		fd_data->cfg_user.keepFacesOverMoreFrames = 5;
		fd_data->cfg_user.keepLimitingWhenNoFace = 0;
		fd_data->cfg_user.framesPerLockWhenNoFaces = 0;
	}

exit:
	return ret;
}

int fimc_is_lib_fd_map_init(struct fimc_is_lib *fd_lib, ulong *kvaddr_fd, u32 *dvaddr_fd,
		ulong kvaddr_fshared, struct vra_param *fd_param)
{
	int ret = 0, index = 0;
	FDD_DATA *data = NULL;
	struct fimc_is_lib_fd *fd_data;
	struct fd_map_addr_str *map_addr = NULL;

	BUG_ON(!fd_lib);
	BUG_ON(!fd_lib->lib_data);
	BUG_ON(!kvaddr_fd);
	BUG_ON(!dvaddr_fd);
	BUG_ON(!fd_param);

	if (test_bit(FD_LIB_MAP_INIT, &fd_lib->state))
		goto exit;

	fd_data = (struct fimc_is_lib_fd *)fd_lib->lib_data;

#ifdef ENABLE_FD_LIB_DIRECT_MAP
	fd_data->data_a.map_width = fd_data->data_b.map_width =
		fd_data->data_c.map_width = FD_MAX_MAP_WIDTH;
	fd_data->data_a.map_height = fd_data->data_b.map_height =
		fd_data->data_c.map_height = FD_MAX_MAP_HEIGHT;
#else
	fd_data->data_a.map_width = fd_data->data_b.map_width =
		fd_data->data_c.map_width = fd_param->config.map_width;
	fd_data->data_a.map_height = fd_data->data_b.map_height =
		fd_data->data_c.map_height = fd_param->config.map_height;
#endif

	/* init settings for set A */
	data = &fd_data->data_a;
	map_addr = &fd_data->map_addr_a;
	ret = fimc_is_lib_fd_map_size(data, map_addr, kvaddr_fd[0], (ulong)dvaddr_fd[0],
			kvaddr_fshared + (0x200 * (index++)));
	if (ret) {
		err("fimc_is_fd Map A alloc fail!!\n");
		ret = -ENOMEM;
		goto exit;
	}

	/* init settings for set B */
	data = &fd_data->data_b;
	map_addr = &fd_data->map_addr_b;
	ret = fimc_is_lib_fd_map_size(data, map_addr, kvaddr_fd[1], (ulong)dvaddr_fd[1],
			kvaddr_fshared + (0x200 * (index++)));
	if (ret) {
		err("fimc_is_fd Map B alloc fail!!\n");
		ret = -ENOMEM;
		goto exit;
	}

	/* init settings for set C */
	data = &fd_data->data_c;
	map_addr = &fd_data->map_addr_c;
	ret = fimc_is_lib_fd_map_size(data, map_addr, kvaddr_fd[2], (ulong)dvaddr_fd[2],
			kvaddr_fshared + (0x200 * (index++)));
	if (ret) {
		err("fimc_is_fd Map C alloc fail!!\n");
		ret = -ENOMEM;
		goto exit;
	}

	/* Set to shift value for HW FD */
	fd_data->shift = fd_data->fd_lib_func->lhfd_get_shift_func(fd_data->data_fd_lib->map_width,
			fd_data->data_fd_lib->map_height);

#ifdef ENABLE_FD_LIB_DIRECT_MAP
	ret = fimc_is_lib_fd_map_load(&fd_data->data_c);
	if (ret) {
		err("fimc_is_fd Map load fail. status(%d)\n");
		ret = -EINAVL;
		goto exit;
	}
#endif

	set_bit(FD_LIB_MAP_INIT, &fd_lib->state);
exit:
	return ret;
}

int fimc_is_lib_fd_map_size(FDD_DATA *m_data, struct fd_map_addr_str *m_addr,
		ulong kbase, ulong dbase, ulong fw_share)
{
	int ret = 0;
	ulong align = 0, map7_cnt = 0;
	u32 m_size = 0;

	BUG_ON(!m_data);
	BUG_ON(!m_addr);

	if (!kbase || !dbase) {
		err("[FD] invalid base addr = %#lx, %#lx", kbase, dbase);
		return -ENOMEM;
	}

	m_size = m_data->map_width * m_data->map_height;
	align = 0x100 * 2;

	/* Set kernel virtual address */
	m_addr->map1.kvaddr = kbase;
	m_addr->map2.kvaddr = m_addr->map1.kvaddr + (m_size * 2 + align);
	m_addr->map3.kvaddr = m_addr->map2.kvaddr + (m_size * 4 + align);
	m_addr->map4.kvaddr = m_addr->map3.kvaddr + (m_size * 4 + align);
	m_addr->map5.kvaddr = m_addr->map4.kvaddr + (m_size / 4 + align);
	m_addr->map6.kvaddr = m_addr->map5.kvaddr + (m_size + align);
	m_addr->map7.kvaddr = fw_share;

	m_data->map1 = (void *)fimc_is_lib_fd_m_align((ulong)m_size * 2, 0x100, &m_addr->map1.kvaddr);
	if (m_data->map1 == (void *)-ENOMEM) {
		ret = -ENOMEM;
		goto err;
	}

	m_data->map2 = (void *)fimc_is_lib_fd_m_align((ulong)m_size * 4, 0x100, &m_addr->map2.kvaddr);
	if (m_data->map2 == (void *)-ENOMEM) {
		ret = -ENOMEM;
		goto err;
	}

	m_data->map3 = (void *)fimc_is_lib_fd_m_align((ulong)m_size * 4, 0x100, &m_addr->map3.kvaddr);
	if (m_data->map3 == (void *)-ENOMEM) {
		ret = -ENOMEM;
		goto err;
	}

	m_data->map4 = (void *)fimc_is_lib_fd_m_align((ulong)m_size / 4, 0x100, &m_addr->map4.kvaddr);
	if (m_data->map4 == (void *)-ENOMEM) {
		ret = -ENOMEM;
		goto err;
	}

	m_data->map5 = (void *)fimc_is_lib_fd_m_align((ulong)m_size, 0x100, &m_addr->map5.kvaddr);
	if (m_data->map5 == (void *)-ENOMEM) {
		ret = -ENOMEM;
		goto err;
	}

	m_data->map6 = (void *)fimc_is_lib_fd_m_align((ulong)1024, 0x100, &m_addr->map6.kvaddr);
	if (m_data->map6 == (void *)-ENOMEM) {
		ret = -ENOMEM;
		goto err;
	}

	m_data->map7 = (void *)fimc_is_lib_fd_m_align((ulong)256, 0x100, &m_addr->map7.kvaddr);
	if (m_data->map7 == (void *)-ENOMEM) {
		ret = -ENOMEM;
		goto err;
	}

	m_data->k = 0;
	m_data->up = 0;

	for (map7_cnt = 0; map7_cnt < 256; map7_cnt++)
		((u8 *)m_data->map7)[map7_cnt] = map7_cnt;

	/* Set device virtual address */
	m_addr->map1.dvaddr = (u32)((ulong)m_data->map2 - kbase) + dbase;
	m_addr->map2.dvaddr = (u32)((ulong)m_data->map3 - kbase) + dbase;
	m_addr->map3.dvaddr = (u32)((ulong)m_data->map4 - kbase) + dbase;
	m_addr->map4.dvaddr = (u32)((ulong)m_data->map5 - kbase) + dbase;
	m_addr->map5.dvaddr = (u32)((ulong)m_data->map1 - kbase) + dbase;
	m_addr->map6.dvaddr = m_addr->map5.dvaddr + m_size;
	m_addr->map7.dvaddr = m_addr->map5.dvaddr + (m_size * 5 / 4);
	m_addr->map8.dvaddr = (u32)((ulong)m_data->map6 - kbase) + dbase;

err:
	return ret;
}

ulong fimc_is_lib_fd_m_align(ulong size_in_byte, ulong align, ulong *alloc_addr)
{
	ulong alloc_size, aligned_alloc_addr;

	if (!(((align % 64) == 0) && (align != 0))) {
		err("fimc_is_fd align wrong(%ld)!!\n", align);
		return -ENOMEM;
	}

	alloc_size = (size_in_byte + (align * 2));

	aligned_alloc_addr = FD_ALIGN((*alloc_addr + 64), align);
	if ((*alloc_addr + 64) == aligned_alloc_addr)
		aligned_alloc_addr += align;

	SET64((aligned_alloc_addr - 4), *alloc_addr);
	SET64((aligned_alloc_addr - 8), alloc_size);

	return aligned_alloc_addr;
}

int fimc_is_lib_fd_thread_init(struct fimc_is_lib *fd_lib)
{
	s32 ret = 0, cnt = 0;

#ifdef FD_LIB_NORMAL_THREAD
	struct sched_param param = {.sched_priority = 0};
#else
	/* RT thread */
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 70};
	/* cpus allow */
	u32 cpu = 0;
#endif

	BUG_ON(!fd_lib);

	spin_lock_init(&fd_lib->work_lock);
	init_kthread_worker(&fd_lib->worker);

	fd_lib->task = kthread_run(kthread_worker_fn, &fd_lib->worker, "fimc_is_lib_fd");

	if (IS_ERR(fd_lib->task)) {
		err("failed to create fd_task\n");
		fd_lib->task = NULL;
		ret = -ENOMEM;
		goto p_err;
	}
#ifdef FD_LIB_NORMAL_THREAD
	ret = sched_setscheduler_nocheck(fd_lib->task, SCHED_NORMAL, &param);
#else
	/* RT thread */
	ret = set_cpus_allowed_ptr(fd_lib->task, cpumask_of(cpu);
#endif
	if (ret) {
		err("sched_setscheduler_nocheck is fail(%d)", ret);
		goto p_err;
	}

	for (cnt = 0; cnt < MAX_FIMC_IS_LIB_TASK; cnt++) {
		fd_lib->work[cnt].data = NULL;
		init_kthread_work(&fd_lib->work[cnt].work, fimc_is_lib_fd_fn);
	}

	fd_lib->work_index = 0;

p_err:
return ret;
}

void fimc_is_lib_fd_fn(struct kthread_work *work)
{
struct fimc_is_lib_work *cur_work = NULL;
struct fimc_is_lib *fd_lib = NULL;

cur_work = container_of(work, struct fimc_is_lib_work, work);
fd_lib = (struct fimc_is_lib *)cur_work->data;

set_bit(FD_LIB_RUN, &fd_lib->state);

fimc_is_lib_fd_run(fd_lib);

clear_bit(FD_LIB_RUN, &fd_lib->state);
}

void fimc_is_lib_fd_trigger(struct fimc_is_lib *fd_lib)
{
	u32 work_index = 0;

	BUG_ON(!fd_lib);

	spin_lock(&fd_lib->work_lock);

	/* TODO: fix this coarse routine */
	fd_lib->work[fd_lib->work_index%MAX_FIMC_IS_LIB_TASK].data = fd_lib;
	fd_lib->work_index++;

	spin_unlock(&fd_lib->work_lock);

	work_index = (fd_lib->work_index - 1) % MAX_FIMC_IS_LIB_TASK;
	queue_kthread_work(&fd_lib->worker, &fd_lib->work[work_index].work);
}

int fimc_is_lib_fd_select_buf(struct fimc_is_lib_fd *lib_data,
		struct camera2_fd_uctl *fdUd, ulong kdshared, ulong ddshared)
{
	struct fd_map_addr_str *map_addr = NULL;
	ulong y_map_addr = 0;

	BUG_ON(!lib_data);
	BUG_ON(!fdUd);

	/*
	 * The logic to select map data.
	 * 3 bufferring method was used by FD_SEL_DATA_A/B/C state
	 * There are two type of state variables for selection.
	 * It indicated that selected slot means..
	 * - fd_dma_select : This slot should be used by FD lib.
	 *                   The selected slot will be used at next shot.
	 * - fd_lib_select : This slot is being used by FD lib.
	 */
	if (test_bit(FD_SEL_DATA_A, &lib_data->fd_dma_select)) {
		if (test_bit(FD_SEL_DATA_A, &lib_data->fd_lib_select)) {
			map_addr = &lib_data->map_addr_b;
			y_map_addr = (ulong)lib_data->data_b.map7 - kdshared + ddshared;
			set_bit(FD_SEL_DATA_C, &lib_data->fd_dma_select);
		} else {
			map_addr = &lib_data->map_addr_a;
			y_map_addr = (ulong)lib_data->data_a.map7 - kdshared + ddshared;
			set_bit(FD_SEL_DATA_B, &lib_data->fd_dma_select);
		}
		clear_bit(FD_SEL_DATA_A, &lib_data->fd_dma_select);
	} else if (test_bit(FD_SEL_DATA_B, &lib_data->fd_dma_select)) {
		if (test_bit(FD_SEL_DATA_B, &lib_data->fd_lib_select)) {
			map_addr = &lib_data->map_addr_c;
			y_map_addr = (ulong)lib_data->data_c.map7 - kdshared + ddshared;
			set_bit(FD_SEL_DATA_A, &lib_data->fd_dma_select);
		} else {
			map_addr = &lib_data->map_addr_b;
			y_map_addr = (ulong)lib_data->data_b.map7 - kdshared + ddshared;
			set_bit(FD_SEL_DATA_C, &lib_data->fd_dma_select);
		}
		clear_bit(FD_SEL_DATA_B, &lib_data->fd_dma_select);
	} else if (test_bit(FD_SEL_DATA_C, &lib_data->fd_dma_select)) {
		if (test_bit(FD_SEL_DATA_C, &lib_data->fd_lib_select)) {
			map_addr = &lib_data->map_addr_a;
			y_map_addr = (ulong)lib_data->data_a.map7 - kdshared + ddshared;
			set_bit(FD_SEL_DATA_B, &lib_data->fd_dma_select);
		} else {
			map_addr = &lib_data->map_addr_c;
			y_map_addr = (ulong)lib_data->data_c.map7 - kdshared + ddshared;
			set_bit(FD_SEL_DATA_A, &lib_data->fd_dma_select);
		}
		clear_bit(FD_SEL_DATA_C, &lib_data->fd_dma_select);
	} else {
		err("[FD] DMA SEL ERROR\n");
		return -1;
	}

	if (map_addr == &lib_data->map_addr_a) {
		lib_data->map_history[DONE_FRAME] = FD_SEL_DATA_A;
	} else if (map_addr == &lib_data->map_addr_b) {
		lib_data->map_history[DONE_FRAME] = FD_SEL_DATA_B;
	} else if (map_addr == &lib_data->map_addr_c) {
		lib_data->map_history[DONE_FRAME] = FD_SEL_DATA_C;
#ifdef ENABLE_FD_LIB_DIRECT_MAP
		map_addr = &lib_data->map_addr_a;
#endif
	} else {
		err("[FD] SEL_ERROR\n");
		return -1;
	}

	/* Parameter copy for F/W */
	fdUd->vendorSpecific[0] = map_addr->map1.dvaddr;
	fdUd->vendorSpecific[1] = map_addr->map2.dvaddr;
	fdUd->vendorSpecific[2] = map_addr->map3.dvaddr;
	fdUd->vendorSpecific[3] = map_addr->map4.dvaddr;
	fdUd->vendorSpecific[4] = map_addr->map5.dvaddr;
	fdUd->vendorSpecific[5] = map_addr->map6.dvaddr;
	fdUd->vendorSpecific[6] = map_addr->map7.dvaddr;
	fdUd->vendorSpecific[7] = map_addr->map8.dvaddr;

	if ((y_map_addr < ddshared) || (y_map_addr >= (ddshared + 0x40000))) {
		err("[FD] Y_map address error : %lx\n", y_map_addr);
		fdUd->vendorSpecific[8] = 0;
	} else {
		fdUd->vendorSpecific[8] = y_map_addr;
	}

	fdUd->vendorSpecific[9] = lib_data->data_fd_lib->k;
	fdUd->vendorSpecific[10] = lib_data->data_fd_lib->up;
	fdUd->vendorSpecific[11] = lib_data->shift;

	return 0;
}

void fimc_is_lib_fd_size_check(struct fimc_is_lib *fd_lib,
		struct param_fd_config *lib_input,
		struct fimc_is_subdev_path *preview,
		u32 hw_width, u32 hw_height)
{
	u32 ratio;
	struct fimc_is_lib_fd *fd_data = NULL;

	BUG_ON(!fd_lib);
	BUG_ON(!fd_lib->lib_data);
	BUG_ON(!lib_input);
	BUG_ON(!preview);

	fd_data = (struct fimc_is_lib_fd *)fd_lib->lib_data;

	ratio = SIZE_RATIO(preview->width, preview->height);
	switch (ratio) {
	case RATIO_4_3:
	case RATIO_3_2:
		lib_input->map_width = 640;
		lib_input->map_height = 480;
		break;
	case RATIO_16_9:
	case RATIO_5_3:
		lib_input->map_width = 640;
		lib_input->map_height = 360;
		break;
	default:
		lib_input->map_width = 480;
		lib_input->map_height = 480;
		break;
	}

	/* When the preview size is smaller than the map size */
	if (hw_width <= (lib_input->map_width))
		lib_input->map_width = hw_width;

	if (hw_height <= (lib_input->map_height))
		lib_input->map_height = hw_height;

	/* This size is preview size. using to FD sircle */
	fd_data->data_a.input_width = fd_data->data_b.input_width =
		fd_data->data_c.input_width = preview->width;
	fd_data->data_a.input_height = fd_data->data_b.input_height =
		fd_data->data_c.input_height = preview->height;

#ifdef FIXED_FD_MAPDATA
	lib_input->map_width = FD_MAP_WIDTH;
	lib_input->map_height = FD_MAP_HEIGHT;
#endif

	if ((fd_data->prev_map_width == lib_input->map_width) &&
			(fd_data->prev_map_height == lib_input->map_height))
		return;

	/* This size is hw fd output size. using to FD sircle and library intput */
	fd_data->data_a.map_width = fd_data->data_b.map_width =
		fd_data->data_c.map_width = lib_input->map_width;
	fd_data->data_a.map_height = fd_data->data_b.map_height =
		fd_data->data_c.map_height = lib_input->map_height;

	fd_data->prev_map_width = lib_input->map_width;
	fd_data->prev_map_height = lib_input->map_height;
	clear_bit(FD_LIB_MAP_INIT, &fd_lib->state);

}

int fimc_is_lib_fd_convert_orientation(u32 src_orientation, s32 *fd_orientation)
{
	int ret = 0;

	BUG_ON(!fd_orientation);

	/*
	* The device orientation type is signed integer.
	* This price is dependent in the direction of the camera sensor.
	* But, LHFD orientation type is unsigned integer.
	* Accordingly the expression mode of two values is different.
	*      Device          LHFD
	*      90              -90
	*      0       ->      0
	*      270             90
	*      180             180
	*/

	if (((s32)src_orientation < 0) || (src_orientation >= 360)) {
		err("%s: Wrong orientation value(%d)\n",
				__func__, src_orientation);
		ret = -EINVAL;
		return ret;
	}

	if (src_orientation == 180 || src_orientation == 0)
		*fd_orientation = src_orientation;
	else
		*fd_orientation = src_orientation - 180;

	dbg_lib("%s: source(%d), FD(%d)\n",
			__func__,
			src_orientation, *fd_orientation);

	return ret;
}
