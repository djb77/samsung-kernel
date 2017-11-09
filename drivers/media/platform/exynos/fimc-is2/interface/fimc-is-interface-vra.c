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

#include "fimc-is-interface-vra.h"
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static struct fimc_is_lib_vra *g_lib_vra;

static void fimc_is_lib_vra_callback_frw_abort(void)
{
	dbg_lib("vra_callback_frw_abort\n");
}

static void fimc_is_lib_vra_callback_frw_hw_err(u32 ch_index, u32 err_mask)
{
	err_lib("callback_frw_hw_err: ch_index(%#x), err_mask(%#x)",
		ch_index, err_mask);
}

static void fimc_is_lib_vra_callback_output_ready(u32 instance,
	u32 num_all_faces, const struct api_vra_out_face *faces_ptr,
	const struct api_vra_out_list_info *out_list_info_ptr)
{
	int i, j;
	struct fimc_is_lib_vra *lib_vra = g_lib_vra;
	u32 face_rect[CAMERA2_MAX_FACES][4];
	u32 face_center[CAMERA2_MAX_FACES][2];
	bool debug_flag = false;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return;
	}

#if defined(VRA_DMA_TEST_BY_IMAGE)
	info_lib("------ num_all_faces(%d) -------\n", num_all_faces);
#endif

	memcpy(&lib_vra->out_list_info[instance], (void *)out_list_info_ptr,
		sizeof(lib_vra->out_list_info[instance]));
	lib_vra->all_face_num[instance] = (num_all_faces > lib_vra->max_face_num) ?
				0 : num_all_faces;

	for (i = 0; i < lib_vra->all_face_num[instance]; i++) {
		face_rect[i][0] = faces_ptr[i].base.rect.left;
		face_rect[i][1] = faces_ptr[i].base.rect.top;
		face_rect[i][2] = faces_ptr[i].base.rect.left + faces_ptr[i].base.rect.width;
		face_rect[i][3] = faces_ptr[i].base.rect.top  + faces_ptr[i].base.rect.height;
		face_center[i][0] = (face_rect[i][0] + face_rect[i][2]) >> 1;
		face_center[i][1] = (face_rect[i][1] + face_rect[i][3]) >> 1;
	}

	for (i = 0; i < lib_vra->all_face_num[instance]; i++) {
		for (j = 0; j < lib_vra->all_face_num[instance]; j++) {
			if (i == j)
				continue;
			if (((face_rect[j][0] <= face_center[i][0]) && (face_center[i][0] <= face_rect[j][2]))
				&& ((face_rect[j][1] <= face_center[i][1]) && (face_center[i][1] <= face_rect[j][3]))) {
				info_lib("lib_vra_callback_output_ready: debug_flag on\n");
				debug_flag = true;
				break;
			}
		}
	}

	if ((num_all_faces > lib_vra->max_face_num) || (debug_flag)) {
		info_lib("lib_vra_callback_output_ready: num_all_faces(%d) > MAX(%d)\n",
			num_all_faces, lib_vra->max_face_num);

		for (i = 0; i < lib_vra->all_face_num[instance]; i++) {
			info_lib("lib_vra: (%d), id[%d]; x,y,w,h,score; %d,%d,%d,%d,%d\n",
				lib_vra->all_face_num[instance],
				i, faces_ptr[i].base.rect.left,
				faces_ptr[i].base.rect.top,
				faces_ptr[i].base.rect.width,
				faces_ptr[i].base.rect.height,
				faces_ptr[i].base.score);
		}
	}

	for (i = 0; i < lib_vra->all_face_num[instance]; i++) {
		lib_vra->out_faces[instance][i] = faces_ptr[i];
#if defined(VRA_DMA_TEST_BY_IMAGE)
		info_lib("lib_vra: id[%d]; x,y,w,h,score; %d,%d,%d,%d,%d\n",
				i, faces_ptr[i].base.rect.left,
				faces_ptr[i].base.rect.top,
				faces_ptr[i].base.rect.width,
				faces_ptr[i].base.rect.height,
				faces_ptr[i].base.score);
#endif
	}
}

static void fimc_is_lib_vra_callback_end_input(u32 instance,
		u32 frame_index, unsigned char *base_address)
{
	dbg_lib("lib_vra_callback_end_input: base_addr(%p)\n", base_address);
}

static void fimc_is_lib_vra_callback_frame_error(u32 instance,
	enum api_vra_sen_err error_type, u32 additonal_info)
{
	struct fimc_is_lib_vra *lib_vra = g_lib_vra;

	if (VRA_ERR_FRAME_LOST) {
		lib_vra->debug.lost_frame_cnt[additonal_info]++;
	} else {
		lib_vra->debug.err_cnt++;
		lib_vra->debug.last_err_type = error_type;
		lib_vra->debug.last_err_info = additonal_info;
	}
}

void fimc_is_lib_vra_task_trigger(struct fimc_is_lib_vra *lib_vra,
	void *func)
{
	u32 work_index = 0;
	struct fimc_is_lib_task *task_vra;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return;
	}

	task_vra = &lib_vra->task_vra;

	spin_lock(&task_vra->work_lock);

	task_vra->work[task_vra->work_index % LIB_MAX_TASK].func = func;
	task_vra->work[task_vra->work_index % LIB_MAX_TASK].params = lib_vra;
	task_vra->work_index++;
	work_index = (task_vra->work_index - 1) % LIB_MAX_TASK;

	spin_unlock(&task_vra->work_lock);

	queue_kthread_work(&task_vra->worker, &task_vra->work[work_index].work);
}

int fimc_is_lib_vra_invoke_contol_event(struct fimc_is_lib_vra *lib_vra)
{
	enum api_vra_type status = VRA_NO_ERROR;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	dbg_lib("lib_vra_invoke_contol_event: type(%d)\n", lib_vra->ctl_task_type);

	if (in_interrupt()) {
		spin_lock(&lib_vra->ctl_lock);
		status = CALL_VRAOP(lib_vra, on_control_task_event,
					lib_vra->fr_work_heap);
		if (status) {
			err_lib("on_control_task_event is fail (%#x)", status);
			spin_unlock(&lib_vra->ctl_lock);
			return -EINVAL;
		}
		spin_unlock(&lib_vra->ctl_lock);
	} else {
		spin_lock_irqsave(&lib_vra->ctl_lock, lib_vra->ctl_irq_flag);
		status = CALL_VRAOP(lib_vra, on_control_task_event,
					lib_vra->fr_work_heap);
		if (status) {
			err_lib("on_control_task_event is fail (%#x)", status);
			spin_unlock_irqrestore(&lib_vra->ctl_lock, lib_vra->ctl_irq_flag);
			return -EINVAL;
		}
		spin_unlock_irqrestore(&lib_vra->ctl_lock, lib_vra->ctl_irq_flag);
	}

	return 0;
}

int fimc_is_lib_vra_invoke_fwalgs_event(struct fimc_is_lib_vra *lib_vra)
{
	enum api_vra_type status = VRA_NO_ERROR;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	dbg_lib("lib_vra_invoke_fwalgs_event: type(%d)\n", lib_vra->algs_task_type);

	if (in_interrupt()) {
		spin_lock(&lib_vra->algs_lock);
		status = CALL_VRAOP(lib_vra, on_fw_algs_task_event,
					lib_vra->fr_work_heap);
		if (status) {
			err_lib("on_fw_algs_task_event is fail (%#x)", status);
			spin_unlock(&lib_vra->algs_lock);
			return -EINVAL;
		}
		spin_unlock(&lib_vra->algs_lock);
	} else {
		spin_lock_irqsave(&lib_vra->algs_lock, lib_vra->algs_irq_flag);
		status = CALL_VRAOP(lib_vra, on_fw_algs_task_event,
					lib_vra->fr_work_heap);
		if (status) {
			err_lib("on_fw_algs_task_event is fail (%#x)", status);
			spin_unlock_irqrestore(&lib_vra->algs_lock, lib_vra->algs_irq_flag);
			return -EINVAL;
		}
		spin_unlock_irqrestore(&lib_vra->algs_lock, lib_vra->algs_irq_flag);
	}

	return 0;
}

void fimc_is_lib_vra_task_work(struct kthread_work *work)
{
	struct fimc_is_task_work *cur_work;
	struct fimc_is_lib_vra *lib_vra;

	cur_work = container_of(work, struct fimc_is_task_work, work);
	lib_vra = (struct fimc_is_lib_vra *)cur_work->params;

	cur_work->func((void *)lib_vra);
}

int fimc_is_lib_vra_init_task(struct fimc_is_lib_vra *lib_vra)
{
	s32 ret = 0;
	u32 j;
#ifdef SET_CPU_AFFINITY
	u32 cpu = 0;
#endif
	struct sched_param param = { .sched_priority = FIMC_IS_MAX_PRIO - 3 };

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	spin_lock_init(&lib_vra->task_vra.work_lock);
	init_kthread_worker(&lib_vra->task_vra.worker);

	lib_vra->task_vra.task = kthread_run(kthread_worker_fn,
		&lib_vra->task_vra.worker, "fimc_is_lib_vra");
	if (IS_ERR(lib_vra->task_vra.task)) {
		err_lib("failed to create thread for VRA, err(%ld)",
			PTR_ERR(lib_vra->task_vra.task));
		return PTR_ERR(lib_vra->task_vra.task);
	}
#ifdef ENABLE_FPSIMD_FOR_USER
	fpsimd_set_task_using(lib_vra->task_vra.task);
#endif
	param.sched_priority = TASK_VRA_PRIORITY;
	ret = sched_setscheduler_nocheck(lib_vra->task_vra.task,
		SCHED_FIFO, &param);
	if (ret) {
		err("sched_setscheduler_nocheck is fail(%d)", ret);
		return ret;
	}

	lib_vra->task_vra.work_index = 0;
	for (j = 0; j < LIB_MAX_TASK; j++) {
		lib_vra->task_vra.work[j].func = NULL;
		lib_vra->task_vra.work[j].params = NULL;
		init_kthread_work(&lib_vra->task_vra.work[j].work,
			fimc_is_lib_vra_task_work);
	}

#ifdef SET_CPU_AFFINITY
	cpu = TASK_VRA_AFFINITY;
	ret = set_cpus_allowed_ptr(lib_vra->task_vra.task, cpumask_of(cpu));
	dbg_lib("lib_vra_task_init: affinity cpu(%d) (%d)\n", cpu, ret);
#endif
	return 0;
}

void fimc_is_lib_vra_set_event_control(u32 event_type)
{
	warn_lib("Invalid event_type (%d)", event_type);

	return;
}

void fimc_is_lib_vra_set_event_fw_algs(u32 event_type)
{
	struct fimc_is_lib_vra *lib_vra = g_lib_vra;

	switch (event_type) {
	case FWALGS_TASK_SET_ABORT:
		/* This event is processed in fimc_is_lib_vra_fwalgs_stop */
		info_lib("FWALGS_TASK_SET_ABORT (%d)\n", event_type);
		break;
	case FWALGS_TASK_SET_ALGS:
		lib_vra->algs_task_type = FWALGS_TASK_SET_ALGS;
		dbg_lib("lib_vra_set_event_fw_algs: type(%d)\n", event_type);
		fimc_is_lib_vra_task_trigger(lib_vra,
			fimc_is_lib_vra_invoke_fwalgs_event);
		break;
	default:
		err_lib("Invalid event_type (%d)", event_type);
		break;
	}

	return;
}

int fimc_is_lib_vra_alloc_memory(struct fimc_is_lib_vra *lib_vra, ulong dma_addr)
{
	int index;
	u32 size;
	enum api_vra_type status = VRA_NO_ERROR;
	struct api_vra_alloc_info *alloc_info;
#if !defined(CONFIG_FIMC_IS_V4_0_0)
#if !defined(VRA_DMA_TEST_BY_IMAGE)
	bool has_vra_ch1_only = false;
	int ret = 0;
#endif
#endif

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	alloc_info = &lib_vra->alloc_info;
	/* ToDo: make define */
	alloc_info->max_image.width  = 640;
	alloc_info->max_image.height = 480;
	alloc_info->track_faces      = 30;
	alloc_info->dt_faces_hw_res  = 2100;
	alloc_info->tr_hw_res_per_face = VRA_TR_HW_RES_PER_FACE;
	alloc_info->ff_hw_res_per_face = 500;
	alloc_info->ff_hw_res_per_list = VRA_FF_HW_RES_PER_LIST;
	alloc_info->cache_line_length  = 32;
#if defined(CONFIG_FIMC_IS_V4_0_0)
	alloc_info->pad_size = 0;
	alloc_info->allow_3planes = 0;
#else
	alloc_info->use_pad = 1;
	alloc_info->allow_ch0_2planes = 0;
#if defined(VRA_DMA_TEST_BY_IMAGE)
	alloc_info->using_ch0_input = 0;
#else
	ret = fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);
	if(has_vra_ch1_only)
		alloc_info->using_ch0_input = 0;
	else
		alloc_info->using_ch0_input = 1;
#endif
#endif
	alloc_info->image_slots = VRA_IMAGE_SLOTS;
	alloc_info->max_sensors = VRA_TOTAL_SENSORS;
	alloc_info->max_tr_res_frames = 5;

	status = CALL_VRAOP(lib_vra, ex_get_memory_sizes,
				&lib_vra->alloc_info,
				&lib_vra->fr_work_size,
				&lib_vra->frame_desc_size,
				&lib_vra->dma_out_size);
	if (status) {
		err_lib("ex_get_memory_sizes is fail (%d)", status);
		return -ENOMEM;
	}

	if (SIZE_VRA_INTERNEL_BUF < lib_vra->dma_out_size) {
		err_lib("SIZE_VRA_INTERNEL_BUF(%d) < Request dma size(%d)",
			SIZE_VRA_INTERNEL_BUF, lib_vra->dma_out_size);
		return -ENOMEM;
	}

	dbg_lib("lib_vra_alloc_memory: dma_out_size(%d), frame_desc_size(%d),"
		" fr_work_size(%d)\n",
		lib_vra->dma_out_size, lib_vra->frame_desc_size,
		lib_vra->fr_work_size);

	size = lib_vra->dma_out_size;
	lib_vra->dma_out_heap = fimc_is_alloc_reserved_vra_dma_buffer(size);
	memset(lib_vra->dma_out_heap, 0, size);

	size = lib_vra->fr_work_size;
	lib_vra->fr_work_heap = fimc_is_alloc_reserved_vra_dma_buffer(size);
	memset(lib_vra->fr_work_heap, 0, size);

	dbg_lib("lib_vra_alloc_memory: dma_out_heap(0x%p), fr_work_heap(0x%p)\n",
		lib_vra->dma_out_heap, lib_vra->fr_work_heap);

	for (index = 0; index < VRA_TOTAL_SENSORS; index++) {
		size = lib_vra->frame_desc_size;
		lib_vra->frame_desc_heap[index] = fimc_is_alloc_reserved_vra_dma_buffer(size);
		memset(lib_vra->frame_desc_heap[index], 0, size);
		dbg_lib("lib_vra_alloc_memory: frame_desc_heap[%d]=(0x%p)\n",
			index, lib_vra->frame_desc_heap[index]);
	}

	return 0;
}

int fimc_is_lib_vra_free_memory(struct fimc_is_lib_vra *lib_vra)
{
	u32 index;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	fimc_is_free_reserved_vra_dma_buffer(lib_vra->dma_out_heap);

	fimc_is_free_reserved_vra_dma_buffer(lib_vra->fr_work_heap);

	for (index = 0; index < VRA_TOTAL_SENSORS; index++)
		fimc_is_free_reserved_vra_dma_buffer(lib_vra->frame_desc_heap[index]);

	return 0;
}

int fimc_is_lib_vra_init_frame_work(struct fimc_is_lib_vra *lib_vra,
	void __iomem *base_addr, enum fimc_is_lib_vra_input_type input_type)
{
	int ret;
	enum api_vra_type status = VRA_NO_ERROR;
	struct vra_call_backs_str *callbacks;
	struct lib_vra_fr_work_info fr_work_info;
	int i;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	/* Connected vra library to global value */
	g_lib_vra = lib_vra;

	lib_vra->fr_index = 0;
#if defined(VRA_DMA_TEST_BY_IMAGE)
	lib_vra->image_load = false;
#endif

	spin_lock_init(&lib_vra->ctl_lock);
	spin_lock_init(&lib_vra->algs_lock);
	spin_lock_init(&lib_vra->intr_lock);

	lib_vra->dma_out.total_size = lib_vra->dma_out_size;
	lib_vra->dma_out.base_adr   = lib_vra->dma_out_heap;
	lib_vra->dma_out.is_cached  = true;

	lib_vra->fr_work_init.hw_regs_base_adr = (uintptr_t)base_addr;
	lib_vra->fr_work_init.int_type = VRA_INT_LEVEL;

#if defined(VRA_DMA_TEST_BY_IMAGE)
	lib_vra->fr_work_init.dram_input = true;
#else
	if (input_type == VRA_INPUT_OTF) {
		lib_vra->fr_work_init.dram_input = false;
	} else if (input_type == VRA_INPUT_MEMORY) {
		lib_vra->fr_work_init.dram_input = true;
	} else {
		err_lib("input type is unknown(%d)", input_type);
		return -EINVAL;
	}
#endif

	dbg_lib("lib_vra_init_frame_work: hw_regs_base_adr(%#lx)\n",
		lib_vra->fr_work_init.hw_regs_base_adr);

	callbacks = &lib_vra->fr_work_init.call_backs;
	callbacks->frw_abort_func_ptr     = fimc_is_lib_vra_callback_frw_abort;
	callbacks->frw_hw_err_func_ptr    = fimc_is_lib_vra_callback_frw_hw_err;
	callbacks->sen_out_ready_func_ptr = fimc_is_lib_vra_callback_output_ready;
	callbacks->sen_end_input_proc_ptr = fimc_is_lib_vra_callback_end_input;
	callbacks->sen_error_ptr          = fimc_is_lib_vra_callback_frame_error;
	callbacks->sen_stat_collected_ptr = NULL;

	lib_vra->fr_work_init.hw_clock_freq_mhz = 533; /* Not used */
	lib_vra->fr_work_init.sw_clock_freq_mhz = 400;
	lib_vra->fr_work_init.block_new_fr_on_transaction = false;
	lib_vra->fr_work_init.block_new_fr_on_input_set   = false;
	lib_vra->fr_work_init.wait_on_lock = true;
	lib_vra->fr_work_init.reset_uniqu_id_on_reset_list   = true;
	lib_vra->fr_work_init.crop_faces_out_of_image_pixels = true;

	fr_work_info.fr_work_init = lib_vra->fr_work_init;
	fr_work_info.fr_work_heap = lib_vra->fr_work_heap;
	fr_work_info.fr_work_size = lib_vra->fr_work_size;

	status = CALL_VRAOP(lib_vra, vra_frame_work_init,
				&fr_work_info,
				&lib_vra->alloc_info,
				&lib_vra->dma_out,
				VRA_DICO_API_VERSION);
	if (status) {
		err_lib("vra_frame_work_init is fail(0x%x)", status);
		ret = -EINVAL;
		goto free;
	}

	for (i = 0; i < VRA_TOTAL_SENSORS; i++)
		clear_bit(VRA_INST_FRAME_DESC_INIT, &lib_vra->inst_state[i]);

	return 0;
free:
	ret = fimc_is_lib_vra_free_memory(lib_vra);
	if (ret) {
		err_lib("lib_vra_free_memory is fail");
		return ret;
	}

	return ret;
}

int fimc_is_lib_vra_init_frame_desc(struct fimc_is_lib_vra *lib_vra, u32 instance)
{
	enum api_vra_type status;
	int ret = 0;
	bool has_vra_ch1_only = false;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	ret = fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);

	/*
	 * Default set for create frame descript.
	 * The value are changed in the set param
	 */
	if (lib_vra->fr_work_init.dram_input) {
		lib_vra->frame_desc[instance].sizes.width = 640;
		lib_vra->frame_desc[instance].sizes.height = 480;
		lib_vra->frame_desc[instance].hdr_lines = 0;
		lib_vra->frame_desc[instance].yuv_format = VRA_YUV_FMT_422;
		lib_vra->frame_desc[instance].u_before_v = true;
		lib_vra->frame_desc[instance].dram.pix_component_store_bits = 8;
		lib_vra->frame_desc[instance].dram.pix_component_data_bits = 8;
		if(has_vra_ch1_only)
			lib_vra->frame_desc[instance].dram.planes_num = 2;
		else
			lib_vra->frame_desc[instance].dram.planes_num = 1;
		lib_vra->frame_desc[instance].dram.un_pack_data = 0;
		lib_vra->frame_desc[instance].dram.line_ofs_fst_plane =
			lib_vra->frame_desc[instance].sizes.width * 2;
		lib_vra->frame_desc[instance].dram.line_ofs_other_planes = 0;
		lib_vra->frame_desc[instance].dram.adr_ofs_bet_planes =
			lib_vra->frame_desc[instance].sizes.height *
			lib_vra->frame_desc[instance].dram.line_ofs_fst_plane;
	} else {
		lib_vra->frame_desc[instance].sizes.width = 640;
		lib_vra->frame_desc[instance].sizes.height = 480;
		lib_vra->frame_desc[instance].hdr_lines = 0;
		lib_vra->frame_desc[instance].yuv_format = VRA_OTF_INPUT_FORMAT;
		lib_vra->frame_desc[instance].u_before_v = true;
	}

	status = CALL_VRAOP(lib_vra, vra_sensor_init,
				lib_vra->frame_desc_heap[instance], instance,
				&lib_vra->frame_desc[instance],
				VRA_TRM_ROI_TRACK);
	if (status) {
		err_lib("[%d]vra_sensor_init is fail(%#x)", instance, status);
		return -EINVAL;
	}

	return 0;
}

int fimc_is_lib_vra_frame_work_init(struct fimc_is_lib_vra *lib_vra,
	void __iomem *base_addr, enum fimc_is_lib_vra_input_type input_type)
{
	int ret;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	if (!test_bit(VRA_LIB_FRAME_WORK_INIT, &lib_vra->state)) {
		ret = fimc_is_lib_vra_init_frame_work(lib_vra, base_addr, input_type);
		if (ret) {
			err_lib("lib_vra_init_frame_work is fail (%d)", ret);
			return ret;
		}
		set_bit(VRA_LIB_FRAME_WORK_INIT, &lib_vra->state);
	}

	set_bit(VRA_LIB_FWALGS_ABORT, &lib_vra->state);

	return 0;
}

int fimc_is_lib_vra_frame_desc_init(struct fimc_is_lib_vra *lib_vra,
	u32 instance)
{
	int ret;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	if (!test_bit(VRA_INST_FRAME_DESC_INIT, &lib_vra->inst_state[instance])) {
		ret = fimc_is_lib_vra_init_frame_desc(lib_vra, instance);
		if (ret) {
			err_lib("[%d]lib_vra_init_frame_desc is fail (%d)", instance, ret);
			return ret;
		}
		set_bit(VRA_INST_FRAME_DESC_INIT, &lib_vra->inst_state[instance]);
	}

	return 0;
}

int fimc_is_lib_vra_set_orientation(struct fimc_is_lib_vra *lib_vra,
	u32 scaler_orientation, u32 instance)
{
	enum api_vra_type status = VRA_NO_ERROR;
	enum api_vra_orientation vra_orientation;
	enum fimc_is_lib_vra_dir dir;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	dir = lib_vra->orientation[instance];

	if (dir == VRA_REAR_ORIENTATION) {
		switch (scaler_orientation) {
		case 0:
			vra_orientation = VRA_ORIENT_TOP_LEFT_TO_RIGHT;
			break;
		case 90:
			vra_orientation = VRA_ORIENT_TOP_RIGHT_TO_BTM;
			break;
		case 180:
			vra_orientation = VRA_ORIENT_BTM_RIGHT_TO_LEFT;
			break;
		case 270:
			vra_orientation = VRA_ORIENT_BTM_LEFT_TO_TOP;
			break;
		default:
			warn_lib("REAR: unknown scaler_orientation(%d)", scaler_orientation);
			vra_orientation = VRA_ORIENT_TOP_LEFT_TO_RIGHT;
			break;
		}
	} else if (dir == VRA_FRONT_ORIENTATION) {
		switch (scaler_orientation) {
		case 0:
			vra_orientation = VRA_ORIENT_TOP_LEFT_TO_RIGHT;
			break;
		case 90:
			vra_orientation = VRA_ORIENT_BTM_LEFT_TO_TOP;
			break;
		case 180:
			vra_orientation = VRA_ORIENT_BTM_RIGHT_TO_LEFT;
			break;
		case 270:
			vra_orientation = VRA_ORIENT_TOP_RIGHT_TO_BTM;
			break;
		default:
			warn_lib("REAR: unknown scaler_orientation(%d)", scaler_orientation);
			vra_orientation = VRA_ORIENT_TOP_LEFT_TO_RIGHT;
			break;
		}
	}

	dbg_lib("[%d]scaler_orientation(%d), vra_orientation(%d)\n", instance,
		scaler_orientation, vra_orientation);

	status = CALL_VRAOP(lib_vra, set_orientation,
				lib_vra->frame_desc_heap[instance],
				vra_orientation);
	if (status) {
		err_lib("[%d]set_orientation fail (%#x)", instance, status);
		return -EINVAL;
	}

	return 0;
}

int fimc_is_lib_vra_new_frame(struct fimc_is_lib_vra *lib_vra,
	unsigned char *buffer_kva, unsigned char *buffer_dva, u32 instance)
{
	enum api_vra_type status = VRA_NO_ERROR;
	unsigned char *input_dma_buf_kva = NULL;
	ulong input_dma_buf_dva;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

#if defined(VRA_DMA_TEST_BY_IMAGE)
	input_dma_buf_kva = lib_vra->test_input_buffer;
	fimc_is_translate_vra_kva_to_dva((ulong)lib_vra->test_input_buffer, (u32 *)&input_dma_buf_dva);
#else
	input_dma_buf_kva = buffer_kva;
	input_dma_buf_dva = (ulong)buffer_dva;
#endif

	status = CALL_VRAOP(lib_vra, on_new_frame,
				lib_vra->frame_desc_heap[instance],
				lib_vra->fr_index, input_dma_buf_kva,
				(unsigned char *)input_dma_buf_dva);
	if (status == VRA_ERR_NEW_FR_PREV_REQ_NOT_HANDLED ||
		status == VRA_ERR_NEW_FR_NEXT_EXIST ||
		status == VRA_BUSY ||
		status == VRA_ERR_FRWORK_ABORTING) {
		err_lib("[%d]on_new_frame is fail(%#x)", instance, status);
		return -EINVAL;
	}
	clear_bit(VRA_LIB_FWALGS_ABORT, &lib_vra->state);

	return 0;
}

int fimc_is_lib_vra_handle_interrupt(struct fimc_is_lib_vra *lib_vra, u32 id)
{
	enum api_vra_type result;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	spin_lock(&lib_vra->intr_lock);
	result = CALL_VRAOP(lib_vra, on_interrupt, lib_vra->fr_work_heap, id);
	if (result) {
		err_lib("on_interrupt is fail (%#x)", result);
		spin_unlock(&lib_vra->intr_lock);
		return -EINVAL;
	}
	spin_unlock(&lib_vra->intr_lock);

	return 0;
}

static int fimc_is_lib_vra_fwalgs_stop(struct fimc_is_lib_vra *lib_vra)
{
	int ret;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	lib_vra->algs_task_type = FWALGS_TASK_SET_ABORT;
	dbg_lib("lib_vra_fwalgs_stop: type(%d)\n", lib_vra->algs_task_type);

	ret = fimc_is_lib_vra_invoke_fwalgs_event(lib_vra);
	if (ret) {
		err_lib("lib_vra_invoke_fwalgs_event(SET_ABORT) is fail(%#x)", ret);
		return ret;
	}
	set_bit(VRA_LIB_FWALGS_ABORT, &lib_vra->state);

	return 0;
}

int fimc_is_lib_vra_stop(struct fimc_is_lib_vra *lib_vra)
{
	int ret;
	enum api_vra_type result;
	int i;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	result = CALL_VRAOP(lib_vra, frame_work_abort,
				lib_vra->fr_work_heap, true);
	if (result) {
		err_lib("frame_work_abort is fail (%#x)", result);
		return -EINVAL;
	}

	ret = fimc_is_lib_vra_fwalgs_stop(lib_vra);
	if (ret) {
		err_lib("lib_vra_fwalgs_stop is fail(%d)", ret);
		return ret;
	}

	for (i = 0; i < VRA_TOTAL_SENSORS; i++)
		clear_bit(VRA_INST_APPLY_TUNE_SET, &lib_vra->inst_state[i]);

	return 0;
}

int fimc_is_lib_vra_frame_work_final(struct fimc_is_lib_vra *lib_vra)
{
	enum api_vra_type result;
	int ret;
	int i;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	result = CALL_VRAOP(lib_vra, frame_work_terminate, lib_vra->fr_work_heap);
	if (result) {
		err_lib("frame_work_terminate is fail (%#x)", result);
		return -EINVAL;
	}

	ret = fimc_is_lib_vra_fwalgs_stop(lib_vra);
	if (ret) {
		err_lib("lib_vra_fwalgs_stop is fail(%d)", ret);
		return ret;
	}

	if (lib_vra->task_vra.task != NULL) {
		ret = kthread_stop(lib_vra->task_vra.task);
		if (ret)
			err_lib("kthread_stop fail (%d)", ret);

		lib_vra->task_vra.task = NULL;
	}

	clear_bit(VRA_LIB_FRAME_WORK_INIT, &lib_vra->state);
	clear_bit(VRA_LIB_FWALGS_ABORT, &lib_vra->state);
	clear_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state);

	for (i = 0; i < VRA_TOTAL_SENSORS; i++) {
		clear_bit(VRA_INST_FRAME_DESC_INIT, &lib_vra->inst_state[i]);
		clear_bit(VRA_INST_APPLY_TUNE_SET, &lib_vra->inst_state[i]);
	}

	return 0;
}

int fimc_is_lib_vra_update_dm(struct fimc_is_lib_vra *lib_vra, u32 instance,
	enum facedetect_mode *faceDetectMode, struct camera2_stats_dm *dm)
{
	int face_num;
	struct api_vra_face_base_str *base;
	struct api_vra_facial_str *facial;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	if (unlikely(!faceDetectMode)) {
		err_lib("faceDetectMode is NULL");
		return -EINVAL;
	}

	if (unlikely(!dm)) {
		err_lib("camera2_stats_dm is NULL");
		return -EINVAL;
	}

	if (*faceDetectMode == FACEDETECT_MODE_OFF) {
		dm->faceDetectMode = FACEDETECT_MODE_OFF;
		return 0;
	}

	/* TODO: FACEDETECT_MODE_FULL*/
	dm->faceDetectMode = FACEDETECT_MODE_SIMPLE;

	if (!lib_vra->all_face_num[instance]) {
		memset(&dm->faceIds, 0, sizeof(dm->faceIds));
		memset(&dm->faceRectangles, 0, sizeof(dm->faceRectangles));
		memset(&dm->faceScores, 0, sizeof(dm->faceScores));
		memset(&dm->faceLandmarks, 0, sizeof(dm->faceIds));
	}

	dm->faceSrcImageSize[0] = lib_vra->out_list_info[instance].in_sizes.width;
	dm->faceSrcImageSize[1] = lib_vra->out_list_info[instance].in_sizes.height;

	for (face_num = 0; face_num < lib_vra->all_face_num[instance]; face_num++) {
		base = &lib_vra->out_faces[instance][face_num].base;
		/* X min */
		dm->faceRectangles[face_num][0] = base->rect.left;
		/* Y min */
		dm->faceRectangles[face_num][1] = base->rect.top;
		/* X max */
		dm->faceRectangles[face_num][2] = base->rect.left + base->rect.width;
		/* Y max */
		dm->faceRectangles[face_num][3] = base->rect.top + base->rect.height;
		/* Score, scale down for RTA (0 ~ 1000) -> (0 ~ 255) */
		dm->faceScores[face_num] = ((base->score * 0xff) / 0x3e8) > 0xff ? 0xff : ((base->score * 0xff) / 0x3e8);
		/* ID */
		dm->faceIds[face_num] = base->unique_id;

		dbg_lib("lib_vra_update_dm: face position(%d,%d),size(%dx%d),scores(%d),id(%d))\n",
			dm->faceRectangles[face_num][0],
			dm->faceRectangles[face_num][1],
			dm->faceRectangles[face_num][2],
			dm->faceRectangles[face_num][3],
			dm->faceScores[face_num],
			dm->faceIds[face_num]);

		facial = &lib_vra->out_faces[instance][face_num].facial;
		dm->faceLandmarks[face_num][0] = facial->locations[VRA_FF_LOCATION_LEFT_EYE].left;
		dm->faceLandmarks[face_num][1] = facial->locations[VRA_FF_LOCATION_LEFT_EYE].top;
		dm->faceLandmarks[face_num][2] = facial->locations[VRA_FF_LOCATION_RIGHT_EYE].left;
		dm->faceLandmarks[face_num][3] = facial->locations[VRA_FF_LOCATION_RIGHT_EYE].top;
		dm->faceLandmarks[face_num][4] = facial->locations[VRA_FF_LOCATION_MOUTH].left;
		dm->faceLandmarks[face_num][5] = facial->locations[VRA_FF_LOCATION_MOUTH].top;

		dbg_lib("lib_vra_update_dm: face locations(%d,%d,%d,%d,%d,%d)\n",
			dm->faceLandmarks[face_num][0],
			dm->faceLandmarks[face_num][1],
			dm->faceLandmarks[face_num][2],
			dm->faceLandmarks[face_num][3],
			dm->faceLandmarks[face_num][4],
			dm->faceLandmarks[face_num][5]);
	}

	/* ToDo: Add error handler for detected face range */

	return 0;
}

int fimc_is_lib_vra_update_sm(struct fimc_is_lib_vra *lib_vra)
{
	/* ToDo */
	return 0;
}

int fimc_is_lib_vra_get_meta(struct fimc_is_lib_vra *lib_vra,
	struct fimc_is_frame *frame)
{
	int ret = 0;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	if (unlikely(!frame)) {
		err_lib("frame is NULL");
		return -EINVAL;
	}

	ret = fimc_is_lib_vra_update_dm(lib_vra, frame->instance,
			&frame->shot->ctl.stats.faceDetectMode,
			&frame->shot->dm.stats);
	if (ret) {
		err_lib("lib_vra_update_dm is fail (%#x)", ret);
		return -EINVAL;
	}

	/* ToDo : fimc_is_lib_vra_update_sm */

	return 0;
}

#if defined(VRA_DMA_TEST_BY_IMAGE)
int fimc_is_lib_vra_test_image_load(struct fimc_is_lib_vra *lib_vra)
{
	int ret = 0;
	struct file *vra_dma_image = NULL;
	long fsize, nread;
	mm_segment_t old_fs;

	if (lib_vra->image_load)
		return 0;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	vra_dma_image = filp_open(VRA_DMA_TEST_IMAGE_PATH, O_RDONLY, 0);
	if (unlikely(!vra_dma_image)) {
		err("filp_open(%s) fail!!\n", VRA_DMA_TEST_IMAGE_PATH);
		return -EEXIST;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fsize = vra_dma_image->f_path.dentry->d_inode->i_size;
	fsize -= 1;

	info_lib("lib_vra_test_image_load: size(%ld)Bytes\n", fsize);
	lib_vra->test_input_buffer = fimc_is_alloc_reserved_vra_dma_buffer(fsize);
	nread = vfs_read(vra_dma_image,
			(char __user *)lib_vra->test_input_buffer,
			fsize, &vra_dma_image->f_pos);
	if (nread != fsize) {
		err_lib("failed to read firmware file (%ld)Bytes", nread);
		ret = -EINVAL;
		goto buf_free;
	}

	lib_vra->image_load = true;

	set_fs(old_fs);
	return 0;

buf_free:
	fimc_is_free_reserved_vra_dma_buffer(lib_vra->test_input_buffer);
	set_fs(old_fs);

	return ret;
}
#endif

void fimc_is_lib_vra_assert(void)
{
	BUG_ON(1);
}

void fimc_is_lib_vra_os_funcs(void)
{
	struct fimc_is_lib_vra_os_system_funcs funcs;

	funcs.control_task_set_event = fimc_is_lib_vra_set_event_control;
	funcs.fw_algs_task_set_event = fimc_is_lib_vra_set_event_fw_algs;
	funcs.set_dram_adr_from_core_to_vdma = fimc_is_translate_vra_kva_to_dva;
	funcs.clean_cache_region             = fimc_is_vra_cache_invalid;
	funcs.invalidate_cache_region        = fimc_is_vra_cache_invalid;
	funcs.data_write_back_cache_region   = fimc_is_vra_cache_flush;
	funcs.log_write_console              = fimc_is_log_write_console;
	funcs.log_write                      = fimc_is_log_write;

	funcs.spin_lock_init 	     = fimc_is_spin_lock_init;
	funcs.spin_lock_finish 	     = fimc_is_spin_lock_finish;
	funcs.spin_lock              = fimc_is_spin_lock;
	funcs.spin_unlock            = fimc_is_spin_unlock;
	funcs.spin_lock_irq          = fimc_is_spin_lock_irq;
	funcs.spin_unlock_irq        = fimc_is_spin_unlock_irq;
	funcs.spin_lock_irqsave      = fimc_is_spin_lock_irqsave;
	funcs.spin_unlock_irqrestore = fimc_is_spin_unlock_irqrestore;
	funcs.lib_assert       = fimc_is_lib_vra_assert;
	funcs.lib_in_interrupt = fimc_is_lib_in_interrupt;

#ifdef ENABLE_FPSIMD_FOR_USER
	fpsimd_get();
	((vra_set_os_funcs_t)VRA_LIB_ADDR)((void *)&funcs);
	fpsimd_put();
#else
	((vra_set_os_funcs_t)VRA_LIB_ADDR)((void *)&funcs);
#endif
}

void fimc_is_lib_vra_check_size(struct api_vra_input_desc *frame_desc, struct vra_param *param, u32 fcount)
{
	if ((frame_desc->sizes.width != param->otf_input.width)
		||(frame_desc->sizes.height != param->otf_input.height)) {
		info_lib("===== VRA input setting =====\n"
			"\t cmd(OTF:%d, DMA:%d), old_sizes(%dx%d) new_sizes(%dx%d) [F:%d]\n",
			param->otf_input.cmd, param->dma_input.cmd,
			frame_desc->sizes.width, frame_desc->sizes.height,
			param->otf_input.width, param->otf_input.height, fcount);
	}
	return;
}

int fimc_is_lib_vra_test_input(struct fimc_is_lib_vra *lib_vra, u32 instance)
{
	enum api_vra_type status;
	struct api_vra_input_desc *frame_desc;
	bool has_vra_ch1_only = false;
	int ret = 0;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	ret = fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);

	frame_desc = &lib_vra->frame_desc[instance];

	frame_desc->sizes.width  = 640;
	frame_desc->sizes.height = 480;
	frame_desc->hdr_lines  = 0;
	frame_desc->yuv_format = VRA_YUV_FMT_422;
	frame_desc->u_before_v = true;
	frame_desc->dram.pix_component_store_bits = 8;
	frame_desc->dram.pix_component_data_bits  = 8;
	if (has_vra_ch1_only) {
		frame_desc->dram.planes_num   = 2;
		frame_desc->dram.line_ofs_fst_plane = frame_desc->sizes.width;
		frame_desc->dram.line_ofs_other_planes = frame_desc->sizes.width;
	} else {
		frame_desc->dram.planes_num   = 1;
		frame_desc->dram.line_ofs_fst_plane = frame_desc->sizes.width * 2;
		frame_desc->dram.line_ofs_other_planes = 0;
	}
	frame_desc->dram.un_pack_data = 0;
	frame_desc->dram.adr_ofs_bet_planes
		= frame_desc->sizes.height * frame_desc->dram.line_ofs_fst_plane;

	info_lib("lib_vra_test_input: DMA_TEST_BY_IMAGE\n");

	status = CALL_VRAOP(lib_vra, set_input,
				lib_vra->frame_desc_heap[instance],
				&lib_vra->frame_desc[instance],
				VRA_KEEP_TR_DATA_BASE);
	if (status) {
		err_lib("[%d]set_input is fail(%#x)", instance, status);
		return -EINVAL;
	}

	return 0;
}

int fimc_is_lib_vra_otf_input(struct fimc_is_lib_vra *lib_vra,
	struct vra_param *param, u32 instance, u32 fcount)
{
	enum api_vra_type status;
	struct api_vra_input_desc *frame_desc;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	if (unlikely(!param)) {
		err_lib("vra_param is NULL");
		return -EINVAL;
	}

	frame_desc = &lib_vra->frame_desc[instance];
	lib_vra_check_size(frame_desc, param, fcount);

	frame_desc->sizes.width  = param->otf_input.width;
	frame_desc->sizes.height = param->otf_input.height;
	frame_desc->hdr_lines = 0;

	switch (param->otf_input.format) {
	case OTF_OUTPUT_FORMAT_YUV444:
		frame_desc->yuv_format = VRA_YUV_FMT_444;
		frame_desc->u_before_v = true;
		break;
	case OTF_OUTPUT_FORMAT_YUV422:
		frame_desc->yuv_format = VRA_YUV_FMT_422;
		frame_desc->u_before_v = true;
		break;
	case OTF_OUTPUT_FORMAT_YUV420:
		frame_desc->yuv_format = VRA_YUV_FMT_420;
		frame_desc->u_before_v = true;
		break;
	default:
		err_lib("[%d]Invalid otf_input.format(%d)", instance,
			param->otf_input.format);
		break;
	}

	status = CALL_VRAOP(lib_vra, set_input,
				lib_vra->frame_desc_heap[instance],
				&lib_vra->frame_desc[instance],
				VRA_KEEP_TR_DATA_BASE);
	if (status) {
		err_lib("[%d]set_input is fail(%#x)", instance, status);
		return -EINVAL;
	}

	return 0;
}

int fimc_is_lib_vra_dma_input(struct fimc_is_lib_vra *lib_vra,
	struct vra_param *param, u32 instance, u32 fcount)
{
	enum api_vra_type status;
	struct api_vra_input_desc *frame_desc;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	if (unlikely(!param)) {
		err_lib("vra_param is NULL");
		return -EINVAL;
	}

	frame_desc = &lib_vra->frame_desc[instance];
	lib_vra_check_size(frame_desc, param, fcount);

	frame_desc->sizes.width  = param->dma_input.width;
	frame_desc->sizes.height = param->dma_input.height;
	frame_desc->hdr_lines = 0;

	switch (param->dma_input.format) {
	case DMA_OUTPUT_FORMAT_YUV444:
		frame_desc->yuv_format = VRA_YUV_FMT_444;
		frame_desc->u_before_v = true;
		frame_desc->dram.line_ofs_fst_plane = param->dma_input.width * 3;
		break;
	case DMA_OUTPUT_FORMAT_YUV422:
		frame_desc->yuv_format = VRA_YUV_FMT_422;
		frame_desc->u_before_v = param->dma_input.plane == 2 ? false : true;
		frame_desc->dram.line_ofs_fst_plane = param->dma_input.width * 2;
		break;
	case DMA_OUTPUT_FORMAT_YUV420:
		frame_desc->yuv_format = VRA_YUV_FMT_420;
		frame_desc->u_before_v = true;
		frame_desc->dram.line_ofs_fst_plane = param->dma_input.width;
		break;
	default:
		err_lib("[%d]Invalid dma_input.format(%d)", instance,
			param->dma_input.format);
		break;
	}

	frame_desc->dram.pix_component_store_bits = param->dma_input.bitwidth;
	frame_desc->dram.pix_component_data_bits  = param->dma_input.bitwidth;
	frame_desc->dram.planes_num = param->dma_input.plane;
	frame_desc->dram.un_pack_data = 0;
	frame_desc->dram.line_ofs_other_planes = 0;
	frame_desc->dram.adr_ofs_bet_planes
			= param->dma_input.height *
			frame_desc->dram.line_ofs_fst_plane;

	status = CALL_VRAOP(lib_vra, set_input,
				lib_vra->frame_desc_heap[instance],
				&lib_vra->frame_desc[instance],
				VRA_KEEP_TR_DATA_BASE);
	if (status) {
		err_lib("[%d]set_input is fail(%#x)", instance, status);
		return -EINVAL;
	}

	return 0;
}

int fimc_is_lib_vra_apply_tune(struct fimc_is_lib_vra *lib_vra,
	struct fimc_is_lib_vra_tune_data *vra_tune, u32 instance)
{
	struct fimc_is_lib_vra_tune_data tune;
	struct api_vra_tune_data *info_tune, dbg_tune;
	struct fimc_is_lib_vra_frame_lock *info_frame;
	enum api_vra_orientation dbg_orientation;
	bool dma_test = false;
	int cnt;
	int ret;
	bool has_vra_ch1_only = false;

	if (unlikely(!lib_vra)) {
		err_lib("lib_vra is NULL");
		return -EINVAL;
	}

	dbg_lib("lib_vra_set_param: vra_tune(%p)\n", vra_tune);

#if defined(VRA_DMA_TEST_BY_IMAGE)
	dma_test = true;
#endif

	ret = fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);

	if (!vra_tune || dma_test) {
		dbg_lib("lib_vra_apply_tune: vra_tune use default setting\n");
		if (has_vra_ch1_only) {
			tune.api_tune.tracking_mode = VRA_TRM_SINGLE_FRAME;
			tune.api_tune.min_face_size = 24;
		} else {
			tune.api_tune.tracking_mode = VRA_TUNE_TRACKING_MODE;
			tune.api_tune.min_face_size = 40;
		}
		tune.api_tune.enable_features = 0;
		tune.api_tune.full_frame_detection_freq = 1;
		tune.api_tune.max_face_count = 10;
		tune.api_tune.face_priority = VRA_TUNE_FACE_PRIORITY;
		tune.api_tune.disable_frontal_rot_mask = VRA_TUNE_DISABLE_FRONTAL_ROT_MASK;
		tune.api_tune.disable_profile_rot_mask = 0xFE;
		tune.api_tune.working_point = VRA_TUNE_WORKING_POINT;
		tune.api_tune.tracking_smoothness = 10;
		tune.api_tune.selfie_working_point = 0;
		tune.api_tune.sensor_position = SENSOR_POSITION_REAR;

		tune.frame_lock.lock_frame_num = 0;
		tune.frame_lock.init_frames_per_lock = 1;
		tune.frame_lock.normal_frames_per_lock = 1;
		tune.dir = VRA_REAR_ORIENTATION;
	} else {
		dbg_lib("lib_vra_apply_tune: vra_tune use setfile\n");
		memcpy(&tune, vra_tune, sizeof(struct fimc_is_lib_vra_tune_data));
		set_bit(VRA_INST_APPLY_TUNE_SET, &lib_vra->inst_state[instance]);
	}

	lib_vra->max_face_num = tune.api_tune.max_face_count;
	lib_vra->orientation[instance] = tune.dir;
	info_tune  = &tune.api_tune;
	info_frame = &tune.frame_lock;

	cnt = CALL_VRAOP(lib_vra, set_parameter, lib_vra->fr_work_heap,
				lib_vra->frame_desc_heap[instance],
				&tune.api_tune);
	if (cnt) {
		err_lib("[%d]set_parameter is fail, cnt(%d)", instance, cnt);
		ret = -EINVAL;
		goto debug_info;
	}

	cnt = CALL_VRAOP(lib_vra, get_parameter, lib_vra->fr_work_heap,
				lib_vra->frame_desc_heap[instance],
				&dbg_tune, &dbg_orientation);
	if (cnt) {
		err_lib("[%d]get_parameter is fail, cnt(%d)", instance, cnt);
		info_tune = &dbg_tune;
		ret = -EINVAL;
		goto debug_info;
	}

	return 0;

debug_info:
	info_lib("===== VRA set parameter =====\n"
		"\t tracking_mode(%#x), enable_features(%#x)\n"
		"\t min_face_size(%#x), max_face_count(%#x)\n"
		"\t full_frame_detection_freq(%#x), face_priority(%#x)\n"
		"\t disable_frontal_rot_mask(%#x), disable_profile_rot_mask(%#x)\n"
		"\t working_point(%#x), tracking_smoothness(%#x)\n"
		"\t frame_lock: lock_frame_num(%d), init_frames_per_lock(%d), "
		"normal_frames_per_lock(%d)\n",
		info_tune->tracking_mode, info_tune->enable_features,
		info_tune->min_face_size, info_tune->max_face_count,
		info_tune->full_frame_detection_freq, info_tune->face_priority,
		info_tune->disable_frontal_rot_mask, info_tune->disable_profile_rot_mask,
		info_tune->working_point, info_tune->tracking_smoothness,
		info_frame->lock_frame_num, info_frame->init_frames_per_lock,
		info_frame->normal_frames_per_lock);

	return ret;
}
