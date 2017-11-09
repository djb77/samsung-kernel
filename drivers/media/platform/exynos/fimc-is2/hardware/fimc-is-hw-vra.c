/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-vra.h"
#include "../interface/fimc-is-interface-ischain.h"
#include "fimc-is-err.h"

void fimc_is_hw_vra_save_debug_info(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_vra *lib_vra, int debug_point)
{
	struct fimc_is_hardware *hardware;
	u32 hw_fcount, index, instance;

	BUG_ON(!hw_ip);

	hardware = hw_ip->hardware;
	instance = atomic_read(&hw_ip->instance);
	hw_fcount = atomic_read(&hw_ip->fcount);

	switch (debug_point) {
	case DEBUG_POINT_FRAME_START:
		hw_ip->debug_index[1] = hw_ip->debug_index[0] % DEBUG_FRAME_COUNT;
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].fcount = hw_ip->debug_index[0];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_START] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START] = local_clock();
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]])
			|| test_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state))
			info_hw("[ID:%d][F:%d]F.S\n", hw_ip->id, hw_fcount);
		break;
	case DEBUG_POINT_FRAME_END:
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_END] = local_clock();
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
			info_hw("[ID:%d][F:%d]F.E\n", hw_ip->id, hw_fcount);

		if (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.fe)) {
			err_hw("[VRA] fs(%d), fe(%d), dma(%d)",
				atomic_read(&hw_ip->count.fs),
				atomic_read(&hw_ip->count.fe),
				atomic_read(&hw_ip->count.dma));
		}
		break;
	default:
		break;
	}
	return;
}

static int fimc_is_hw_vra_ch0_handle_interrupt(u32 id, void *context)
{
	int ret = 0;
	u32 status, intr_mask, intr_status;
	struct fimc_is_hw_ip *hw_ip = NULL;
	struct fimc_is_hw_vra *hw_vra = NULL;
	struct fimc_is_lib_vra *lib_vra;
	u32 hw_fcount;

	BUG_ON(!context);
	hw_ip = (struct fimc_is_hw_ip *)context;
	hw_fcount = atomic_read(&hw_ip->fcount);

	BUG_ON(!hw_ip->priv_info);
	hw_vra = (struct fimc_is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		err_hw("hw_vra[ch0]: hw_vra is null");
		return -EINVAL;
	}
	lib_vra = &hw_vra->lib_vra;

	intr_status = fimc_is_vra_chain0_get_status_intr(hw_ip->regs);
	intr_mask   = fimc_is_vra_chain0_get_enable_intr(hw_ip->regs);
	status = (intr_mask) & intr_status;
	dbg_hw("vra_isr[ch0]: id(%d), status(0x%x), mask(0x%x), status(0x%x)\n",
		id, intr_status, intr_mask, status);

	ret = fimc_is_lib_vra_handle_interrupt(&hw_vra->lib_vra, id);
	if (ret) {
		err_hw("vra_isr[ch0]: lib_vra_handle_interrupt is fail (%d)\n" \
			"status(0x%x), mask(0x%x), status(0x%x)",
			ret, intr_status, intr_mask, status);
	}

	if (status & (1 << CH0INT_END_FRAME)) {
		status &= (~(1 << CH0INT_END_FRAME));
		dbg_hw("hw_vra[ch0]: END_FRAME\n");
		atomic_inc(&hw_ip->count.fe);
		hw_ip->cur_e_int++;
		if (hw_ip->cur_e_int >= hw_ip->num_buffers) {
			/* VRA stop register do not support shadowing */
			if (test_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state)) {
				info_hw("hw_vra[ch0]: END_FRAME: VRA_LIB_STOP [F:%d]\n",
					hw_fcount);
				ret = fimc_is_lib_vra_stop(lib_vra);
				if (ret)
					err_hw("lib_vra_stop is fail (%d)", ret);
				clear_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state);
				atomic_set(&hw_vra->ch1_count, 0);
			}
			atomic_set(&hw_ip->status.Vvalid, V_BLANK);
			fimc_is_hw_vra_save_debug_info(hw_ip, lib_vra, DEBUG_POINT_FRAME_END);
#if !defined(VRA_DMA_TEST_BY_IMAGE)
			fimc_is_hardware_frame_done(hw_ip, NULL, -1,
				FIMC_IS_HW_CORE_END, IS_SHOT_SUCCESS, true);
#endif
			wake_up(&hw_ip->status.wait_queue);
		}
	}

	if (status & (1 << CH0INT_CIN_FR_ST)) {
		status &= (~(1 << CH0INT_CIN_FR_ST));
		dbg_hw("hw_vra[ch0]: CIN_FR_ST\n");
		atomic_inc(&hw_ip->count.fs);
		hw_ip->cur_s_int++;
		if (hw_ip->cur_s_int == 1) {
			fimc_is_hw_vra_save_debug_info(hw_ip, lib_vra, DEBUG_POINT_FRAME_START);
			atomic_set(&hw_ip->status.Vvalid, V_VALID);
			clear_bit(HW_CONFIG, &hw_ip->state);
		}

		if (hw_ip->cur_s_int < hw_ip->num_buffers) {
			ret = fimc_is_lib_vra_new_frame(lib_vra, NULL, NULL, atomic_read(&hw_ip->instance));
			if (ret)
				err_hw("[%d]lib_vra_new_frame is fail (%d)", 0, ret);
		}
	}

	if (status)
		info_hw("hw_vra[ch0]: error! status(0x%x)\n", intr_status);

	return ret;
}

static int fimc_is_hw_vra_ch1_handle_interrupt(u32 id, void *context)
{
	int ret = 0;
	u32 status, intr_mask, intr_status;
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip = NULL;
	struct fimc_is_hw_vra *hw_vra = NULL;
	struct fimc_is_lib_vra *lib_vra;
	u32 instance;
	bool has_vra_ch1_only = false;
	u32 ch1_mode;
	u32 hw_fcount;

	BUG_ON(!context);
	hw_ip = (struct fimc_is_hw_ip *)context;
	hardware = hw_ip->hardware;
	instance = atomic_read(&hw_ip->instance);
	hw_fcount = atomic_read(&hw_ip->fcount);

	ret = fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);

	BUG_ON(!hw_ip->priv_info);
	hw_vra = (struct fimc_is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		err_hw("vra_isr[ch1]: hw_vra is null");
		return -EINVAL;
	}
	lib_vra = &hw_vra->lib_vra;

	ch1_mode = fimc_is_vra_chain1_get_image_mode(hw_ip->regs);
	intr_status = fimc_is_vra_chain1_get_status_intr(hw_ip->regs);
	intr_mask   = fimc_is_vra_chain1_get_enable_intr(hw_ip->regs);
	status = (intr_mask) & intr_status;
	dbg_hw("vra_isr[ch1]: id(%d), status(0x%x), mask(0x%x), status(0x%x)\n",
		id, intr_status, intr_mask, status);

	ret = fimc_is_lib_vra_handle_interrupt(&hw_vra->lib_vra, id);
	if (ret) {
		err_hw("hw_vra[ch1]: lib_vra_handle_interrupt is fail (%d)\n" \
			"status(0x%x), mask(0x%x), status(0x%x), ch1_count(%d)",
			ret, intr_status, intr_mask, status, atomic_read(&hw_vra->ch1_count));
	}

	if (status & (1 << CH1INT_IN_START_OF_CONTEXT)) {
		status &= (~(1 << CH1INT_IN_START_OF_CONTEXT));
		dbg_hw("hw_vra[ch1]: START_OF_CONTEXT\n");
		if (has_vra_ch1_only && test_bit(HW_VRA_CH1_START, &hw_ip->state)) {
			clear_bit(HW_VRA_CH1_START, &hw_ip->state);
			atomic_inc(&hw_ip->count.fs);
			hw_ip->cur_s_int++;
			if (hw_ip->cur_s_int == 1) {
				fimc_is_hardware_frame_start(hw_ip, instance);
				fimc_is_hw_vra_save_debug_info(hw_ip, lib_vra, DEBUG_POINT_FRAME_START);
				atomic_set(&hw_ip->status.Vvalid, V_VALID);
				clear_bit(HW_CONFIG, &hw_ip->state);
			}

			if (hw_ip->cur_s_int < hw_ip->num_buffers) {
				if (hw_ip->mframe) {
					struct fimc_is_frame *mframe = hw_ip->mframe;
					unsigned char *buffer_kva = NULL;
					unsigned char *buffer_dva = NULL;
					mframe->cur_buf_index = hw_ip->cur_s_int;
					/* TODO: It is required to support other YUV format */
					buffer_kva = (unsigned char *)(mframe->kvaddr_buffer \
						[mframe->cur_buf_index * 2]); /* Y-plane index of NV21 */
					buffer_dva = (unsigned char *)(ulong)(mframe->dvaddr_buffer \
						[mframe->cur_buf_index * 2]);
					ret = fimc_is_lib_vra_new_frame(lib_vra, buffer_kva, buffer_dva, atomic_read(&hw_ip->instance));
					if (ret)
						err_hw("[%d]lib_vra_new_frame is fail (%d)", 0, ret);
				} else {
					err_hw("mframe is null(s:%d, e:%d, t:%d)",
						hw_ip->cur_s_int, hw_ip->cur_e_int, hw_ip->num_buffers);
				}
			}
		}
	}

	if (status & (1 << CH1INT_IN_END_OF_CONTEXT)) {
		status &= (~(1 << CH1INT_IN_END_OF_CONTEXT));
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]])
			|| test_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state))
			info_hw("hw_vra[ch1]: END_OF_CONTEXT [F:%d](%d)\n",
				atomic_read(&hw_ip->fcount),
				(atomic_read(&hw_vra->ch1_count) % 4));
		atomic_inc(&hw_ip->count.dma);
		atomic_inc(&hw_vra->ch1_count);

		dbg_hw("hw_vra[ch1]: END_OF_CONTEXT\n");
		if (has_vra_ch1_only && (ch1_mode == VRA_CH1_DETECT_MODE)) {
			/* detect mode */
			set_bit(HW_VRA_CH1_START, &hw_ip->state);
			atomic_inc(&hw_ip->count.fe);
			hw_ip->cur_e_int++;
			if (hw_ip->cur_e_int >= hw_ip->num_buffers) {
				atomic_set(&hw_ip->status.Vvalid, V_BLANK);
				fimc_is_hw_vra_save_debug_info(hw_ip, lib_vra, DEBUG_POINT_FRAME_END);
				fimc_is_hardware_frame_done(hw_ip, NULL, -1,
					FIMC_IS_HW_CORE_END, IS_SHOT_SUCCESS, true);
				wake_up(&hw_ip->status.wait_queue);

				hw_ip->mframe = NULL;
			}
		}
	}

	if (status)
		info_hw("hw_vra[ch1]: error! status(0x%x)\n", intr_status);

	return ret;
}


const struct fimc_is_hw_ip_ops fimc_is_hw_vra_ops = {
	.open			= fimc_is_hw_vra_open,
	.init			= fimc_is_hw_vra_init,
	.close			= fimc_is_hw_vra_close,
	.enable			= fimc_is_hw_vra_enable,
	.disable		= fimc_is_hw_vra_disable,
	.shot			= fimc_is_hw_vra_shot,
	.set_param		= fimc_is_hw_vra_set_param,
	.get_meta		= fimc_is_hw_vra_get_meta,
	.frame_ndone		= fimc_is_hw_vra_frame_ndone,
	.load_setfile		= fimc_is_hw_vra_load_setfile,
	.apply_setfile		= fimc_is_hw_vra_apply_setfile,
	.delete_setfile		= fimc_is_hw_vra_delete_setfile,
	.clk_gate		= fimc_is_hardware_clk_gate
};

int fimc_is_hw_vra_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc, int id)
{
	int ret = 0;
	int hw_slot = -1;
	bool has_vra_ch1_only = false;

	BUG_ON(!hw_ip);
	BUG_ON(!itf);
	BUG_ON(!itfc);

	/* initialize device hardware */
	hw_ip->id   = id;
	hw_ip->ops  = &fimc_is_hw_vra_ops;
	hw_ip->itf  = itf;
	hw_ip->itfc = itfc;
	atomic_set(&hw_ip->fcount, 0);
	hw_ip->internal_fcount = 0;
	hw_ip->is_leader = true;
	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	atomic_set(&hw_ip->rsccount, 0);
	init_waitqueue_head(&hw_ip->status.wait_queue);

	/* set fd sfr base address */
	hw_slot = fimc_is_hw_slot_id(id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid hw_slot (%d,%d)", id, hw_slot);
		return 0;
	}

	/* set vra interrupt handler */
	ret = fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);

	if (!has_vra_ch1_only)
		itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &fimc_is_hw_vra_ch0_handle_interrupt;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].handler = &fimc_is_hw_vra_ch1_handle_interrupt;

	clear_bit(HW_OPEN, &hw_ip->state);
	clear_bit(HW_INIT, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);
	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_TUNESET, &hw_ip->state);

	info_hw("[ID:%2d] probe done\n", id);

	return ret;
}

int fimc_is_hw_vra_open(struct fimc_is_hw_ip *hw_ip, u32 instance, u32 *size)
{
	int ret = 0;

	BUG_ON(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	*size = sizeof(struct fimc_is_hw_vra);

	frame_manager_probe(hw_ip->framemgr, FRAMEMGR_ID_HW | (1 << hw_ip->id));
	frame_manager_probe(hw_ip->framemgr_late, FRAMEMGR_ID_HW | (1 << hw_ip->id) | 0xF000);
	frame_manager_open(hw_ip->framemgr, FIMC_IS_MAX_HW_FRAME);
	frame_manager_open(hw_ip->framemgr_late, FIMC_IS_MAX_HW_FRAME_LATE);

	dbg_hw("[%d][ID:%d]open: [G:0x%x], framemgr[ID:0x%x]",
			instance, hw_ip->id, GROUP_ID(hw_ip->group[instance]->id), hw_ip->framemgr->id);

	return ret;
}

int fimc_is_hw_vra_init(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *group, bool flag, u32 module_id)
{
	u32 instance = 0;
	int ret = 0;
	ulong dma_addr;
	struct fimc_is_hw_vra *hw_vra = NULL;
	struct fimc_is_lib_vra *lib_vra;
	struct fimc_is_subdev *leader;
	enum fimc_is_lib_vra_input_type input_type;

	BUG_ON(!hw_ip);
	BUG_ON(!hw_ip->priv_info);
	BUG_ON(!group);

	hw_vra = (struct fimc_is_hw_vra *)hw_ip->priv_info;
	lib_vra = &hw_vra->lib_vra;
	instance = group->instance;

	leader = &hw_ip->group[instance]->leader;
	if (leader->id == ENTRY_VRA)
		input_type = VRA_INPUT_MEMORY;
	else
		input_type = VRA_INPUT_OTF;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		fimc_is_hw_vra_reset(hw_ip);

#ifdef ENABLE_FPSIMD_FOR_USER
		fpsimd_get();
		get_lib_vra_func((void *)&hw_vra->lib_vra.itf_func);
		fpsimd_put();
#else
		get_lib_vra_func((void *)&hw_vra->lib_vra.itf_func);
#endif
		dbg_hw("[%d][ID:%d] library interface was initialized\n", instance, hw_ip->id);

		dma_addr = hw_ip->group[instance]->device->minfo->kvaddr_vra;
		ret = fimc_is_lib_vra_alloc_memory(&hw_vra->lib_vra, dma_addr);
		if (ret) {
			err_hw("[%d][ID:%d] failed to alloc. memory", instance, hw_ip->id);
			return ret;
		}

		ret = fimc_is_lib_vra_frame_work_init(&hw_vra->lib_vra, hw_ip->regs, input_type);
		if (ret) {
			err_hw("[%d][ID:%d] failed to init. framework (%d)", instance, hw_ip->id, ret);
			return ret;
		}

		ret = fimc_is_lib_vra_init_task(&hw_vra->lib_vra);
		if (ret) {
			err_hw("[%d[ID:%d] failed to init. task(%d)", instance, hw_ip->id, ret);
			return ret;
		}

#if defined(VRA_DMA_TEST_BY_IMAGE)
		ret = fimc_is_lib_vra_test_image_load(lib_vra);
		if (ret) {
			err_hw("[%d[ID:%d] failed to load test image (%d)", instance, ret);
			return ret;
		}
#endif

		atomic_set(&hw_vra->ch1_count, 0);
	} else {
		if (input_type != lib_vra->fr_work_init.dram_input) {
			err_hw("[%d] input type is not matched - instance: %s, framework: %s",
					instance, input_type ? "M2M" : "OTF",
					lib_vra->fr_work_init.dram_input ? "M2M": "OTF");
			return -EINVAL;
		}
	}

	ret = fimc_is_lib_vra_frame_desc_init(&hw_vra->lib_vra, instance);
	if (ret) {
		err_hw("[%d][ID:%d] failed to init frame desc. (%d)", instance, hw_ip->id, ret);
		return ret;
	}

	dbg_hw("[%d][ID:%d] ready to start\n", instance, hw_ip->id);

	return 0;
}

int fimc_is_hw_vra_desc_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	return 0;
}

int fimc_is_hw_vra_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_vra *hw_vra = NULL;

	BUG_ON(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	hw_vra = (struct fimc_is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		err_hw("[%d]hw_vra is NULL", instance);
		return -EINVAL;
	}

	ret = fimc_is_lib_vra_frame_work_final(&hw_vra->lib_vra);
	if (ret) {
		err_hw("[%d]lib_vra_destory_object is fail (%d)", instance, ret);
		return ret;
	}

	ret = fimc_is_lib_vra_free_memory(&hw_vra->lib_vra);
	if (ret) {
		err_hw("[%d]lib_vra_free_memory is fail (%d)", instance, hw_ip->id);
		return ret;
	}

	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);

	info_hw("[%d]close (%d)(%d)\n", instance, hw_ip->id, atomic_read(&hw_ip->rsccount));

	return ret;
}

int fimc_is_hw_vra_enable(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	int ret = 0;

	BUG_ON(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		err_hw("[%d]not initialized!! (%d)", instance, hw_ip->id);
		return -EINVAL;
	}

	set_bit(HW_RUN, &hw_ip->state);
	set_bit(HW_VRA_CH1_START, &hw_ip->state);

	return ret;
}

int fimc_is_hw_vra_disable(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	int ret = 0;
	long timetowait;
	struct fimc_is_hw_vra *hw_vra = NULL;

	BUG_ON(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (atomic_read(&hw_ip->rsccount) > 1)
		return 0;

	info_hw("[%d][ID:%d]vra_disable: Vvalid(%d)\n", instance, hw_ip->id,
		atomic_read(&hw_ip->status.Vvalid));

	if (test_bit(HW_RUN, &hw_ip->state)) {
		timetowait = wait_event_timeout(hw_ip->status.wait_queue,
			!atomic_read(&hw_ip->status.Vvalid),
			FIMC_IS_HW_STOP_TIMEOUT);

		if (!timetowait)
			err_hw("[%d][ID:%d] wait FRAME_END timeout (%ld)", instance,
				hw_ip->id, timetowait);

		hw_vra = (struct fimc_is_hw_vra *)hw_ip->priv_info;
		if (unlikely(!hw_vra)) {
			err_hw("[%d]hw_vra is NULL", instance);
			return -EINVAL;
		}

		ret = fimc_is_lib_vra_stop(&hw_vra->lib_vra);
		if (ret) {
			err_hw("[%d]lib_vra_stop is fail (%d)", instance, ret);
			return ret;
		}

		fimc_is_hw_vra_reset(hw_ip);
		atomic_set(&hw_vra->ch1_count, 0);
		clear_bit(HW_RUN, &hw_ip->state);
		clear_bit(HW_CONFIG, &hw_ip->state);
	} else {
		dbg_hw("[%d]already disabled (%d)\n", instance, hw_ip->id);
	}

	return 0;
}

int fimc_is_hw_vra_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	struct fimc_is_group *head;
	struct fimc_is_hw_vra *hw_vra = NULL;
	struct vra_param *param;
	struct fimc_is_lib_vra *lib_vra;
	u32 instance, lindex, hindex;
	unsigned char *buffer_kva = NULL;
	unsigned char *buffer_dva = NULL;

	BUG_ON(!hw_ip);
	BUG_ON(!frame);

	/* multi-buffer */
	hw_ip->cur_s_int = 0;
	hw_ip->cur_e_int = 0;
	if (frame->num_buffers)
		hw_ip->num_buffers = frame->num_buffers;

	instance = frame->instance;
	dbg_hw("[%d][ID:%d]shot [F:%d]\n", instance, hw_ip->id,
		frame->fcount);

	hw_vra  = (struct fimc_is_hw_vra *)hw_ip->priv_info;
	param   = &hw_ip->region[instance]->parameter.vra;
	lib_vra = &hw_vra->lib_vra;

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		dbg_hw("[%d][ID:%d]hw_vra not initialized\n",
			instance, hw_ip->id);
		return -EINVAL;
	}

	if (frame->type == SHOT_TYPE_INTERNAL) {
		dbg_hw("[%d][ID:%d] request not exist\n", instance, hw_ip->id);
		goto new_frame;
	}

	BUG_ON(!frame->shot);
	/* per-frame control
	 * check & update size from region */
	lindex = frame->shot->ctl.vendor_entry.lowIndexParam;
	hindex = frame->shot->ctl.vendor_entry.highIndexParam;

#if !defined(VRA_DMA_TEST_BY_IMAGE)
	ret = fimc_is_lib_vra_set_orientation(lib_vra,
			frame->shot->uctl.scalerUd.orientation, instance);
	if (ret) {
		err_hw("[%d]lib_vra_set_orientation is fail (%d)",
				instance, ret);
		return ret;
	}

	ret = fimc_is_hw_vra_update_param(hw_ip, param, lindex, hindex,
			instance, frame->fcount);
	if (ret) {
		err_hw("[%d]lib_vra_update_param is fail (%d)", instance, ret);
		return ret;
	}
#endif

new_frame:
	head = hw_ip->group[frame->instance]->head;

	if (param->control.cmd == CONTROL_COMMAND_STOP) {
		if (!test_bit(VRA_LIB_FWALGS_ABORT, &lib_vra->state)) {
			if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state)) {
				info_hw("[%d]shot: VRA_LIB_BYPASS_REQUESTED [F:%d](%d)\n",
					instance, frame->fcount, param->control.cmd);
				set_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state);
			} else {
				info_hw("[%d]shot: lib_vra_stop [F:%d](%d)\n",
					instance, frame->fcount, param->control.cmd);
				ret = fimc_is_lib_vra_stop(lib_vra);
				if (ret)
					err_hw("lib_vra_stop is fail (%d)", ret);

				atomic_set(&hw_vra->ch1_count, 0);
			}
		}

		return ret;
	}
	/*
	 * OTF mode: the buffer value is null.
	 * DMA mode: the buffer value is VRA input DMA address.
	 * ToDo: DMA input buffer set by buffer hiding
	 */
	/* Add for CH1 DMA input */
	if (lib_vra->fr_work_init.dram_input) {
		/* TODO: It is required to support other YUV format */
		buffer_kva = (unsigned char *)(frame->kvaddr_buffer[frame->cur_buf_index * 2]); /* Y-plane index of NV21 */
		buffer_dva = (unsigned char *)(u64)(frame->dvaddr_buffer[frame->cur_buf_index * 2]);
		hw_ip->mframe = frame;
	}
	dbg_hw("[%d]lib_vra_new_frame [F:%d]\n", instance, frame->fcount);
	ret = fimc_is_lib_vra_new_frame(lib_vra, buffer_kva, buffer_dva, instance);
	if (ret) {
		err_hw("[%d]lib_vra_new_frame is fail (%d)", instance, ret);
		info_hw("count[ID:%d]fs(%d), fe(%d), dma(%d)", hw_ip->id,
			atomic_read(&hw_ip->count.fs),
			atomic_read(&hw_ip->count.fe),
			atomic_read(&hw_ip->count.dma));
		return ret;
	}

#if !defined(VRA_DMA_TEST_BY_IMAGE)
	set_bit(hw_ip->id, &frame->core_flag);
#endif
	set_bit(HW_CONFIG, &hw_ip->state);

	return 0;
}

int fimc_is_hw_vra_set_param(struct fimc_is_hw_ip *hw_ip,
	struct is_region *region, u32 lindex, u32 hindex, u32 instance,
	ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_vra *hw_vra;
	struct vra_param *param;
	struct fimc_is_lib_vra *lib_vra;
	u32 fcount;

	BUG_ON(!hw_ip);
	BUG_ON(!region);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		err_hw("[%d]not initialized!!", instance);
		return -EINVAL;
	}

	fcount = atomic_read(&hw_ip->fcount);

	hw_ip->region[instance] = region;
	hw_ip->lindex[instance] = lindex;
	hw_ip->hindex[instance] = hindex;

	param   = &region->parameter.vra;
	hw_vra  = (struct fimc_is_hw_vra *)hw_ip->priv_info;
	lib_vra = &hw_vra->lib_vra;

	if (!test_bit(VRA_INST_APPLY_TUNE_SET, &lib_vra->inst_state[instance])) {
		ret = fimc_is_lib_vra_apply_tune(lib_vra, NULL, instance);
		if (ret) {
			err_hw("[%d]lib_vra_set_tune is fail (%d)", instance, ret);
			return ret;
		}
	}

	info_hw("[%d][ID:%d]set_param \n", instance, hw_ip->id);
	ret = fimc_is_hw_vra_update_param(hw_ip, param,
			lindex, hindex, instance, fcount);
	if (ret) {
		err_hw("[%d]lib_vra_set_param is fail (%d)", instance, ret);
		return ret;
	}

	return ret;
}

int fimc_is_hw_vra_update_param(struct fimc_is_hw_ip *hw_ip,
	struct vra_param *param, u32 lindex, u32 hindex, u32 instance, u32 fcount)
{
	int ret = 0;
	struct fimc_is_hw_vra *hw_vra;
	struct fimc_is_lib_vra *lib_vra;

	hw_vra = (struct fimc_is_hw_vra *)hw_ip->priv_info;

	BUG_ON(!hw_ip);
	BUG_ON(!param);

	lib_vra = &hw_vra->lib_vra;

#ifdef VRA_DMA_TEST_BY_IMAGE
	ret = fimc_is_lib_vra_test_input(lib_vra, instance);
	if (ret) {
		err_hw("[%d]lib_vra_test_input is fail (%d)", instance, ret);
		return ret;
	}
	return 0;
#endif

	if (param->otf_input.cmd == OTF_INPUT_COMMAND_ENABLE) {
		if ((lindex & LOWBIT_OF(PARAM_FD_OTF_INPUT))
			|| (hindex & HIGHBIT_OF(PARAM_FD_OTF_INPUT))) {
			ret = fimc_is_lib_vra_otf_input(lib_vra, param, instance, fcount);
			if (ret) {
				err_hw("[%d]lib_vra_otf_input is fail (%d)", instance, ret);
				return ret;
			}
		}
	} else if (param->dma_input.cmd == DMA_INPUT_COMMAND_ENABLE) {
		if ((lindex & LOWBIT_OF(PARAM_FD_DMA_INPUT))
			|| (hindex & HIGHBIT_OF(PARAM_FD_DMA_INPUT))) {
			ret = fimc_is_lib_vra_dma_input(lib_vra, param, instance, fcount);
			if (ret) {
				err_hw("[%d]lib_vra_dma_input is fail (%d)", instance, ret);
				return ret;
			}
		}
	} else {
		err_hw("VRA param setting is wrong! otf_input.cmd(%d), dma_input.cmd(%d)",
			param->otf_input.cmd, param->dma_input.cmd);
		return -EINVAL;
	}

	return ret;
}

int fimc_is_hw_vra_frame_ndone(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_frame *frame, u32 instance, enum ShotErrorType done_type)
{
	int ret = 0;
	int wq_id;
	int output_id;

	BUG_ON(!hw_ip);
	BUG_ON(!frame);

	wq_id     = -1;
	output_id = FIMC_IS_HW_CORE_END;
	if (test_bit_variables(hw_ip->id, &frame->core_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, wq_id, output_id,
				done_type, true);

	return ret;
}

int fimc_is_hw_vra_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct fimc_is_hw_vra *hw_vra = NULL;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;

	BUG_ON(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		dbg_hw("[%d]%s: hw_map(0x%lx)\n", instance, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		err_hw("[%d] not initialized!!", instance);
		return -ESRCH;
	}

	if (!unlikely(hw_ip->priv_info)) {
		err_hw("[%d][ID:%d] priv_info is NULL", instance, hw_ip->id);
		return -EINVAL;
	}
	hw_vra = (struct fimc_is_hw_vra *)hw_ip->priv_info;
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	switch (setfile->version) {
	case SETFILE_V2:
		break;
	case SETFILE_V3:
		break;
	default:
		err_hw("[%d][ID:%d] invalid version (%d)", instance, hw_ip->id,
				 setfile->version);
		return -EINVAL;
		break;
	}

	set_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

int fimc_is_hw_vra_apply_setfile(struct fimc_is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	struct fimc_is_hw_ip_setfile *setfile;
	struct fimc_is_hw_vra_setfile *setfile_vra;
	struct fimc_is_hw_vra *hw_vra;
	struct fimc_is_lib_vra *lib_vra;
	struct fimc_is_lib_vra_tune_data tune;
	enum exynos_sensor_position sensor_position;
	u32 setfile_index;
	u32 p_rot_mask;
	int ret = 0;

	BUG_ON(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		dbg_hw("[%d]%s: hw_map(0x%lx)\n", instance, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		err_hw("[%d] not initialized!!", instance);
		return -ESRCH;
	}

	hw_vra  = (struct fimc_is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		err_hw("[%d]hw_vra is NULL", instance);
		return -EINVAL;
	}

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile    = &hw_ip->setfile[sensor_position];
	lib_vra = &hw_vra->lib_vra;
	clear_bit(VRA_INST_APPLY_TUNE_SET, &lib_vra->inst_state[instance]);

	setfile_index = setfile->index[scenario];
	if (setfile_index >= setfile->using_count) {
		err_hw("[%d][ID:%d] setfile index is out-of-range, [%d:%d]",
				instance, hw_ip->id, scenario, setfile_index);
		return -EINVAL;
	}

	info_hw("[%d][ID:%d] setfile (%d) scenario (%d)\n", instance, hw_ip->id,
		setfile_index, scenario);

	hw_vra->setfile = *(struct fimc_is_hw_vra_setfile *)setfile->table[setfile_index].addr;
	setfile_vra = &hw_vra->setfile;

	if (setfile_vra->setfile_version != VRA_SETFILE_VERSION)
		err_hw("[%d][ID:%d]setfile version is wrong:(%#x) expected version (%#x)",
			instance, hw_ip->id, setfile_vra->setfile_version, VRA_SETFILE_VERSION);

	tune.api_tune.tracking_mode   = setfile_vra->tracking_mode;
	tune.api_tune.enable_features = setfile_vra->enable_features;
	tune.api_tune.min_face_size   = setfile_vra->min_face_size;
	tune.api_tune.max_face_count  = setfile_vra->max_face_count;
	tune.api_tune.face_priority   = setfile_vra->face_priority;
	tune.api_tune.disable_frontal_rot_mask =
		(setfile_vra->limit_rotation_angles & 0xFF);

	if (setfile_vra->disable_profile_detection)
		p_rot_mask = (setfile_vra->limit_rotation_angles >> 8) | (0xFF);
	else
		p_rot_mask = (setfile_vra->limit_rotation_angles >> 8) | (0xFE);

	tune.api_tune.disable_profile_rot_mask = p_rot_mask;
	tune.api_tune.working_point       = setfile_vra->boost_dr_vs_fpr;
	tune.api_tune.tracking_smoothness = setfile_vra->tracking_smoothness;
	tune.api_tune.selfie_working_point = setfile_vra->lock_frame_number;
	tune.api_tune.sensor_position      = sensor_position;

	tune.dir = setfile_vra->front_orientation;

	tune.frame_lock.lock_frame_num         = setfile_vra->lock_frame_number;
	tune.frame_lock.init_frames_per_lock   = setfile_vra->init_frames_per_lock;
	tune.frame_lock.normal_frames_per_lock = setfile_vra->normal_frames_per_lock;

	if (lib_vra->fr_work_init.dram_input)
		tune.api_tune.full_frame_detection_freq =
				setfile_vra->init_frames_per_lock;
	else
		tune.api_tune.full_frame_detection_freq =
				setfile_vra->normal_frames_per_lock;

	ret = fimc_is_lib_vra_apply_tune(lib_vra, &tune, instance);
	if (ret) {
		err_hw("[%d]lib_vra_apply_tune is fail (%d)", instance, ret);
		return ret;
	}

	return 0;
}

int fimc_is_hw_vra_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
		ulong hw_map)
{
	clear_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

void fimc_is_hw_vra_reset(struct fimc_is_hw_ip *hw_ip)
{
	u32 all_intr;
	bool has_vra_ch1_only = false;
	int ret = 0;

	/* Interrupt clear */
	ret = fimc_is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);
	if(!has_vra_ch1_only) {
		all_intr = fimc_is_vra_chain0_get_all_intr(hw_ip->regs);
		fimc_is_vra_chain0_set_clear_intr(hw_ip->regs, all_intr);
	}

	all_intr = fimc_is_vra_chain1_get_all_intr(hw_ip->regs);
	fimc_is_vra_chain1_set_clear_intr(hw_ip->regs, all_intr);
}

int fimc_is_hw_vra_get_meta(struct fimc_is_hw_ip *hw_ip,
		struct fimc_is_frame *frame, unsigned long hw_map)
{
	int ret = 0;
	struct fimc_is_hw_vra *hw_vra;

	if (unlikely(!frame)) {
		err_hw("[%d]get_meta: frame is null", atomic_read(&hw_ip->instance));
		return 0;
	}

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	hw_vra = (struct fimc_is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		err_hw("[%d]hw_vra is NULL", frame->instance);
		return -EINVAL;
	}

	ret = fimc_is_lib_vra_get_meta(&hw_vra->lib_vra, frame);
	if (ret)
		err_hw("[%d]lib_vra_get_meta is fail (%d)", frame->instance, ret);

	return ret;
}
