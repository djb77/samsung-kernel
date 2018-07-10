/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-srdz.h"
#include "api/fimc-is-hw-api-srdz-v1.h"
#include "../interface/fimc-is-interface-ischain.h"
#include "fimc-is-param.h"
#include "fimc-is-err.h"

static int fimc_is_hw_srdz_handle_interrupt(u32 id, void *context)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip = NULL;
	u32 status, intr_mask, intr_status;
	bool err_intr_flag = false;
	u32 instance;
	u32 hw_fcount, index;

	hw_ip = (struct fimc_is_hw_ip *)context;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);
	instance = atomic_read(&hw_ip->instance);

	intr_mask = fimc_is_srdz_get_intr_mask(hw_ip->regs);
	intr_status = fimc_is_srdz_get_intr_status(hw_ip->regs);
	status = (~intr_mask) & intr_status;

	fimc_is_srdz_clear_intr_src(hw_ip->regs, status);

	if (status & (1 << INTR_SRDZ_FRAME_START)) {
		atomic_inc(&hw_ip->count.fs);
		hw_ip->debug_index[1] = hw_ip->debug_index[0] % DEBUG_FRAME_COUNT;
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].fcount = hw_ip->debug_index[0];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_START] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START] = local_clock();
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
			info_hw("[ID:%d][F:%d]F.S\n", hw_ip->id, hw_fcount);

		fimc_is_hardware_frame_start(hw_ip, instance);
	}

	if (status & (1 << INTR_SRDZ_FRAME_END)) {
		if (fimc_is_hw_srdz_frame_done(hw_ip, NULL, IS_SHOT_SUCCESS)) {
			index = hw_ip->debug_index[1];
			hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_DMA_END] = raw_smp_processor_id();
			hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_DMA_END] = local_clock();
			if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
				info_hw("[ID:%d][F:%d]F.E DMA\n", hw_ip->id, atomic_read(&hw_ip->fcount));

			atomic_inc(&hw_ip->count.dma);
		} else {
			index = hw_ip->debug_index[1];
			hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_END] = raw_smp_processor_id();
			hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_END] = local_clock();
			if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
				info_hw("[ID:%d][F:%d]F.E\n", hw_ip->id, hw_fcount);

			fimc_is_hardware_frame_done(hw_ip, NULL, -1, FIMC_IS_HW_CORE_END,
				IS_SHOT_SUCCESS, true);
		}

		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		atomic_inc(&hw_ip->count.fe);
		if (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.fe)) {
			err_hw("[SRDZ] fs(%d), fe(%d), dma(%d)\n",
				atomic_read(&hw_ip->count.fs),
				atomic_read(&hw_ip->count.fe),
				atomic_read(&hw_ip->count.dma));
		}

		wake_up(&hw_ip->status.wait_queue);
	}

	if (err_intr_flag) {
		info_hw("[ID:%d][SRDZ][F:%d] Ocurred error interrupt status(0x%x)\n",
			hw_ip->id, hw_fcount, status);
		fimc_is_srdz_dump(hw_ip->regs);
		fimc_is_hardware_size_dump(hw_ip);
	}

	if (status & (1 << INTR_SRDZ_FRAME_END))
		CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);

	return 0;
}

const struct fimc_is_hw_ip_ops fimc_is_hw_srdz_ops = {
	.open			= fimc_is_hw_srdz_open,
	.init			= fimc_is_hw_srdz_init,
	.close			= fimc_is_hw_srdz_close,
	.enable			= fimc_is_hw_srdz_enable,
	.disable		= fimc_is_hw_srdz_disable,
	.shot			= fimc_is_hw_srdz_shot,
	.set_param		= fimc_is_hw_srdz_set_param,
	.frame_ndone		= fimc_is_hw_srdz_frame_ndone,
	.load_setfile		= fimc_is_hw_srdz_load_setfile,
	.apply_setfile		= fimc_is_hw_srdz_apply_setfile,
	.delete_setfile		= fimc_is_hw_srdz_delete_setfile,
	.size_dump		= fimc_is_hw_srdz_size_dump,
	.clk_gate		= fimc_is_hardware_clk_gate
};

int fimc_is_hw_srdz_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc,	int id)
{
	int ret = 0;
	int hw_slot = -1;

	BUG_ON(!hw_ip);
	BUG_ON(!itf);
	BUG_ON(!itfc);

	/* initialize device hardware */
	hw_ip->id   = id;
	hw_ip->ops  = &fimc_is_hw_srdz_ops;
	hw_ip->itf  = itf;
	hw_ip->itfc = itfc;
	atomic_set(&hw_ip->fcount, 0);
	hw_ip->internal_fcount = 0;
	hw_ip->is_leader = true;
	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	atomic_set(&hw_ip->rsccount, 0);
	init_waitqueue_head(&hw_ip->status.wait_queue);

	/* set srdz sfr base address */
	hw_slot = fimc_is_hw_slot_id(id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid hw_slot (%d, %d)", id, hw_slot);
		return -EINVAL;
	}

	/* set srdz interrupt handler */
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &fimc_is_hw_srdz_handle_interrupt;

	clear_bit(HW_OPEN, &hw_ip->state);
	clear_bit(HW_INIT, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);
	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_TUNESET, &hw_ip->state);

	info_hw("[ID:%2d] probe done\n", id);

	return ret;
}

int fimc_is_hw_srdz_open(struct fimc_is_hw_ip *hw_ip, u32 instance, u32 *size)
{
	int ret = 0;

	BUG_ON(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	*size = sizeof(struct fimc_is_hw_srdz);

	frame_manager_probe(hw_ip->framemgr, FRAMEMGR_ID_HW | hw_ip->id);
	frame_manager_open(hw_ip->framemgr, FIMC_IS_MAX_HW_FRAME);
	frame_manager_probe(hw_ip->framemgr_late, FRAMEMGR_ID_HW | hw_ip->id | 0xF0);
	frame_manager_open(hw_ip->framemgr_late, FIMC_IS_MAX_HW_FRAME_LATE);

	atomic_set(&hw_ip->status.Vvalid, V_BLANK);

	return ret;
}

int fimc_is_hw_srdz_init(struct fimc_is_hw_ip *hw_ip, struct fimc_is_group *group,
	bool flag, u32 module_id)
{
	int ret = 0;
	u32 instance = 0;
	struct fimc_is_hw_srdz *hw_srdz;

	BUG_ON(!hw_ip);

	instance = group->instance;
	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;

	hw_srdz->rep_flag[instance] = flag;

	/* skip duplicated h/w setting */
	if (test_bit(HW_INIT, &hw_ip->state))
		return 0;

	hw_srdz->instance = FIMC_IS_STREAM_COUNT;

	return ret;
}

int fimc_is_hw_srdz_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;

	BUG_ON(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	info_hw("[%d]close (%d)(%d)\n", instance, hw_ip->id, atomic_read(&hw_ip->rsccount));

	return ret;
}

int fimc_is_hw_srdz_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;

	BUG_ON(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		err_hw("[%d]not initialized!! (%d)", instance, hw_ip->id);
		return -EINVAL;
	}

	if (test_bit(HW_RUN, &hw_ip->state))
		return ret;

	ret = fimc_is_hw_srdz_reset(hw_ip);
	if (ret != 0) {
		err_hw("SRDZ sw reset fail");
		return -ENODEV;
	}

	ret = fimc_is_hw_srdz_clear_interrupt(hw_ip);
	if (ret != 0) {
		err_hw("SRDZ sw reset fail");
		return -ENODEV;
	}

	dbg_hw("[%d][ID:%d]srdz_enable: done\n", instance, hw_ip->id);

	set_bit(HW_RUN, &hw_ip->state);

	return ret;
}

int fimc_is_hw_srdz_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	u32 timetowait;
	struct fimc_is_hw_srdz *hw_srdz;

	BUG_ON(!hw_ip);
	BUG_ON(!hw_ip->priv_info);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (atomic_read(&hw_ip->rsccount) > 1)
		return 0;

	info_hw("[%d][ID:%d]srdz_disable: Vvalid(%d)\n", instance, hw_ip->id,
		atomic_read(&hw_ip->status.Vvalid));

	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;

	if (test_bit(HW_RUN, &hw_ip->state)) {
		timetowait = wait_event_timeout(hw_ip->status.wait_queue,
			!atomic_read(&hw_ip->status.Vvalid),
			FIMC_IS_HW_STOP_TIMEOUT);

		if (!timetowait) {
			err_hw("[%d][ID:%d] wait FRAME_END timeout (%u)", instance,
				hw_ip->id, timetowait);
			ret = -ETIME;
		}

		ret = fimc_is_hw_srdz_clear_interrupt(hw_ip);
		if (ret != 0) {
			err_hw("SRDZ clear interupt fail");
			return -ENODEV;
		}

		clear_bit(HW_RUN, &hw_ip->state);
		clear_bit(HW_CONFIG, &hw_ip->state);
	} else {
		dbg_hw("[%d]already disabled (%d)\n", instance, hw_ip->id);
	}

	info_hw("[%d][ID:%d]srdz_disable: done \n", instance, hw_ip->id);

	return ret;
}

int fimc_is_hw_srdz_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	struct fimc_is_group *head;
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_srdz *hw_srdz;
	struct mcs_param *param;
	u32 lindex, hindex, instance;

	BUG_ON(!hw_ip);
	BUG_ON(!frame);

	hardware = hw_ip->hardware;
	instance = frame->instance;

	dbg_hw("[%d][ID:%d]shot [F:%d]\n", instance, hw_ip->id, frame->fcount);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		dbg_hw("[%d][ID:%d] hw_srdz not initialized\n", instance, hw_ip->id);
		return -EINVAL;
	}

	if ((!test_bit(ENTRY_M0P, &frame->out_flag))
		&& (!test_bit(ENTRY_M1P, &frame->out_flag))
		&& (!test_bit(ENTRY_M2P, &frame->out_flag))
		&& (!test_bit(ENTRY_M3P, &frame->out_flag))
		&& (!test_bit(ENTRY_M4P, &frame->out_flag)))
		set_bit(hw_ip->id, &frame->core_flag);

	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;
	param = &hw_ip->region[instance]->parameter.srdz;

	head = hw_ip->group[frame->instance]->head;

	if (frame->type == SHOT_TYPE_INTERNAL) {
		dbg_hw("[%d][ID:%d] request not exist\n", instance, hw_ip->id);
		goto config;
	}

	lindex = frame->shot->ctl.vendor_entry.lowIndexParam;
	hindex = frame->shot->ctl.vendor_entry.highIndexParam;

	fimc_is_hw_srdz_update_param(hw_ip, param,
		lindex, hindex, instance);

	dbg_hw("[%d]srdz_shot [F:%d][T:%d]\n", instance, frame->fcount, frame->type);

config:
	if (param->input.dma_cmd == DMA_INPUT_COMMAND_ENABLE) {
		/* TODO */
	}

	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

int fimc_is_hw_srdz_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_srdz *hw_srdz;
	struct mcs_param *param;

	BUG_ON(!hw_ip);
	BUG_ON(!region);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		err_hw("[%d]not initialized!!", instance);
		return -EINVAL;
	}

	hw_ip->region[instance] = region;
	hw_ip->lindex[instance] = lindex;
	hw_ip->hindex[instance] = hindex;

	param = &region->parameter.srdz;
	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;

	hw_srdz->instance = FIMC_IS_STREAM_COUNT;
	if (hw_srdz->rep_flag[instance]) {
		dbg_hw("[%d][ID:%d] skip srdz set_param(rep_flag(%d))\n",
			instance, hw_ip->id, hw_srdz->rep_flag[instance]);
		return 0;
	}

	fimc_is_hw_srdz_update_param(hw_ip, param,
		lindex, hindex, instance);

	return ret;
}

int fimc_is_hw_srdz_update_param(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *param, u32 lindex, u32 hindex, u32 instance)
{
	int ret = 0;
	bool control_cmd = false;
	struct fimc_is_hw_srdz *hw_srdz;

	BUG_ON(!hw_ip);
	BUG_ON(!param);

	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;

	if (hw_srdz->instance != instance) {
		control_cmd = true;
		info_hw("[%d][ID:%d]hw_srdz_update_param: hw_ip->instance(%d), control_cmd(%d)\n",
			instance, hw_ip->id, hw_srdz->instance, control_cmd);
		hw_srdz->instance = instance;
	}

	if (control_cmd || (lindex & LOWBIT_OF(PARAM_SRDZ_INPUT))
			|| (hindex & HIGHBIT_OF(PARAM_SRDZ_INPUT))) {
		ret = fimc_is_hw_srdz_dma_input(hw_ip, &param->input, instance);
	}

	if (control_cmd || (lindex & LOWBIT_OF(PARAM_SRDZ_OUTPUT))
			|| (hindex & HIGHBIT_OF(PARAM_SRDZ_OUTPUT))) {
		ret = fimc_is_hw_srdz_dma_output(hw_ip, &param->output[0], 0, instance);
	}

	if (ret)
		fimc_is_hw_srdz_size_dump(hw_ip);

	return ret;
}

int fimc_is_hw_srdz_reset(struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;
	struct fimc_is_hw_srdz *hw_srdz;

	BUG_ON(!hw_ip);

	if (test_bit(HW_RUN, &hw_ip->state))
		return 0;

	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;

	info_hw("[ID:%d]hw_srdz_reset:\n", hw_ip->id);

	ret = fimc_is_srdz_sw_reset(hw_ip->regs, hw_ip->id);
	if (ret != 0) {
		err_hw("SRDZ sw reset fail");
		return -ENODEV;
	}

	return ret;
}

int fimc_is_hw_srdz_clear_interrupt(struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;

	BUG_ON(!hw_ip);

	info_hw("[ID:%d]hw_srdz_clear_interrupt\n", hw_ip->id);

	fimc_is_srdz_clear_intr_all(hw_ip->regs, hw_ip->id);
	fimc_is_srdz_disable_intr(hw_ip->regs, hw_ip->id);
	fimc_is_srdz_mask_intr(hw_ip->regs, hw_ip->id, SRDZ_INTR_MASK);

	return ret;
}

int fimc_is_hw_srdz_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 index,
	u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_srdz *hw_srdz;
	struct fimc_is_hw_ip_setfile *info;

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
	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;
	info = &hw_ip->setfile;

	switch (info->version) {
	case SETFILE_V2:
		break;
	case SETFILE_V3:
		break;
	default:
		err_hw("[%d][ID:%d] invalid version (%d)", instance, hw_ip->id,
			info->version);
		return -EINVAL;
	}

	hw_srdz->setfile = (struct hw_api_srdz_setfile *)info->table[index].addr;
	if (hw_srdz->setfile->setfile_version != SRDZ_SETFILE_VERSION) {
		err_hw("[%d][ID:%d] setfile version(0x%x) is incorrect",
			instance, hw_ip->id, hw_srdz->setfile->setfile_version);
		return -EINVAL;
	}

	set_bit(HW_TUNESET, &hw_ip->state);

	return ret;
}

int fimc_is_hw_srdz_apply_setfile(struct fimc_is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_srdz *hw_srdz = NULL;
	struct fimc_is_hw_ip_setfile *info;
	u32 setfile_index = 0;

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
		err_hw("SRDZ priv info is NULL");
		return -EINVAL;
	}

	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;
	info = &hw_ip->setfile;

	if (!hw_srdz->setfile)
		return 0;

	setfile_index = info->index[scenario];
	if (setfile_index >= hw_ip->setfile.using_count) {
		err_hw("[%d][ID:%d] setfile index is out-of-range, [%d:%d]",
				instance, hw_ip->id, scenario, setfile_index);
		return -EINVAL;
	}

	info_hw("[%d][ID:%d] setfile (%d) scenario (%d)\n", instance, hw_ip->id,
		setfile_index, scenario);

	return ret;
}

int fimc_is_hw_srdz_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	struct fimc_is_hw_srdz *hw_srdz = NULL;

	BUG_ON(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		dbg_hw("[%d]%s: hw_map(0x%lx)\n", instance, __func__, hw_map);
		return 0;
	}

	if (test_bit(HW_TUNESET, &hw_ip->state)) {
		dbg_hw("[%d][ID:%d] setfile already deleted", instance, hw_ip->id);
		return 0;
	}

	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;
	hw_srdz->setfile = NULL;
	clear_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

bool fimc_is_hw_srdz_frame_done(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	int done_type)
{
	int ret = 0;
	bool fdone_flag = false;
	struct fimc_is_frame *done_frame;
	struct fimc_is_framemgr *framemgr;

	switch (done_type) {
	case IS_SHOT_SUCCESS:
		framemgr = hw_ip->framemgr;
		framemgr_e_barrier(framemgr, 0);
		done_frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
		framemgr_x_barrier(framemgr, 0);
		if (done_frame == NULL) {
			err_hw("[SRDZ][F:%d] frame(null)!!", atomic_read(&hw_ip->fcount));
			BUG_ON(1);
		}
		break;
	case IS_SHOT_UNPROCESSED:
	case IS_SHOT_LATE_FRAME:
	case IS_SHOT_UNKNOWN:
	case IS_SHOT_BAD_FRAME:
	case IS_SHOT_GROUP_PROCESSSTOP:
	case IS_SHOT_INVALID_FRAMENUMBER:
	case IS_SHOT_OVERFLOW:
	case IS_SHOT_TIMEOUT:
		done_frame = frame;
		break;
	default:
		err_hw("[%d][F:%d] invalid done type(%d)\n", atomic_read(&hw_ip->instance),
			atomic_read(&hw_ip->fcount), done_type);
		return false;
	}

	if (test_bit(ENTRY_SRDZ, &done_frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_SRDZ_FDONE, ENTRY_SRDZ, done_type, true);
		fdone_flag = true;
		dbg_hw("[OUT:0] cleared[F:%d]\n", done_frame->fcount);
	}

	return fdone_flag;
}

int fimc_is_hw_srdz_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type)
{
	int ret = 0;
	bool is_fdone = false;

	is_fdone = fimc_is_hw_srdz_frame_done(hw_ip, frame, done_type);

	if (test_bit_variables(hw_ip->id, &frame->core_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, -1, FIMC_IS_HW_CORE_END,
				done_type, true);

	return ret;
}

int fimc_is_hw_srdz_dma_input(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance)
{
	int ret = 0;
	u32 width, height;
	u32 format, bit_width, plane, order;
	u32 y_stride, uv_stride;
	u32 img_format;
	struct fimc_is_hw_srdz *hw_srdz;

	BUG_ON(!hw_ip);
	BUG_ON(!input);

	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;

	dbg_hw("[%d][ID:%d]hw_srdz_dma_input_setting cmd(%d)\n", instance, hw_ip->id, input->dma_cmd);
	width  = input->dma_crop_width;
	height = input->dma_crop_height;
	format = input->dma_format;
	bit_width = input->dma_bitwidth;
	plane = input->plane;
	order = input->dma_order;
	y_stride = input->dma_stride_y;
	uv_stride = input->dma_stride_c;

	if (input->dma_cmd == DMA_INPUT_COMMAND_DISABLE)
		return ret;

	ret = fimc_is_hw_srdz_check_format(HW_SRDZ_DMA_INPUT,
		format, bit_width, width, height);
	if (ret) {
		err_hw("[%d]Invalid SRDZ DMA Input format: fmt(%d),bit(%d),size(%dx%d)",
			instance, format, bit_width, width, height);
		return ret;
	}

	fimc_is_srdz_set_input_img_size(hw_ip->regs, hw_ip->id, width, height);
	fimc_is_srdz_set_rdma_stride(hw_ip->regs, y_stride, uv_stride);

	ret = fimc_is_hw_srdz_adjust_input_img_fmt(format, plane, order, &img_format);
	if (ret < 0)
		warn_hw("[%d][ID:%d] Invalid rdma image format\n", instance, hw_ip->id);

	fimc_is_srdz_set_rdma_size(hw_ip->regs, width, height);
	fimc_is_srdz_set_rdma_format(hw_ip->regs, img_format);

	return ret;
}

int fimc_is_hw_srdz_otf_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_srdz *hw_srdz;
	u32 out_width, out_height;
	u32 format, bitwidth;

	BUG_ON(!hw_ip);
	BUG_ON(!output);
	BUG_ON(!hw_ip->priv_info);

	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;

	dbg_hw("[%d][OUT:%d]hw_srdz_otf_output_setting cmd(O:%d,D:%d)\n",
		instance, output_id, output->otf_cmd, output->dma_cmd);

	if (output->otf_cmd == OTF_OUTPUT_COMMAND_DISABLE) {
		fimc_is_srdz_set_otf_out_enable(hw_ip->regs, output_id, false);
		return ret;
	}

	out_width  = output->width;
	out_height = output->height;
	format     = output->otf_format;
	bitwidth  = output->otf_bitwidth;

	ret = fimc_is_hw_srdz_check_format(HW_SRDZ_OTF_OUTPUT,
		format, bitwidth, out_width, out_height);
	if (ret) {
		err_hw("[%d][OUT:%d]Invalid SRDZ OTF Output format: fmt(%d),bit(%d),size(%dx%d)",
			instance, output_id, format, bitwidth, out_width, out_height);
		return ret;
	}

	fimc_is_srdz_set_otf_out_enable(hw_ip->regs, output_id, true);

	return ret;
}

int fimc_is_hw_srdz_dma_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	u32 out_width, out_height;
	u32 format, plane, order, bitwidth;
	u32 y_stride, uv_stride;
	u32 img_format;
	struct fimc_is_hw_srdz *hw_srdz;

	BUG_ON(!hw_ip);
	BUG_ON(!output);
	BUG_ON(!hw_ip->priv_info);

	hw_srdz = (struct fimc_is_hw_srdz *)hw_ip->priv_info;

	dbg_hw("[%d][OUT:%d]hw_srdz_dma_output_setting cmd(O:%d,D:%d)\n",
		instance, output_id, output->otf_cmd, output->dma_cmd);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		fimc_is_srdz_set_dma_out_enable(hw_ip->regs, output_id, false);
		return ret;
	}

	out_width  = output->width;
	out_height = output->height;
	format     = output->dma_format;
	plane      = output->plane;
	order      = output->dma_order;
	bitwidth   = output->dma_bitwidth;
	y_stride   = output->dma_stride_y;
	uv_stride  = output->dma_stride_c;

	ret = fimc_is_hw_srdz_check_format(HW_SRDZ_DMA_OUTPUT,
		format, bitwidth, out_width, out_height);
	if (ret) {
		err_hw("[%d][OUT:%d]Invalid SRDZ DMA Output format: fmt(%d),bit(%d),size(%dx%d)",
			instance, output_id, format, bitwidth, out_width, out_height);
		return ret;
	}

	ret = fimc_is_hw_srdz_adjust_output_img_fmt(format, plane, order, &img_format);
	if (ret < 0)
		warn_hw("[%d][ID:%d] Invalid dma image format\n", instance, hw_ip->id);

	fimc_is_srdz_set_wdma_format(hw_ip->regs, output_id, img_format);
	fimc_is_srdz_set_wdma_size(hw_ip->regs, output_id, out_width, out_height);
	fimc_is_srdz_set_wdma_stride(hw_ip->regs, output_id, y_stride, uv_stride);

	return ret;
}

int fimc_is_hw_srdz_adjust_input_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format)
{
	int ret = 0;

	switch (format) {
	case DMA_INPUT_FORMAT_YUV420:
		switch (plane) {
		case 2:
			switch (order) {
			case DMA_INPUT_ORDER_CbCr:
				*img_format = SRDZ_YUV420_2P_UFIRST;
				break;
			case DMA_INPUT_ORDER_CrCb:
				* img_format = SRDZ_YUV420_2P_VFIRST;
				break;
			default:
				err_hw("input order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = SRDZ_YUV420_3P;
			break;
		default:
			err_hw("input plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	case DMA_INPUT_FORMAT_YUV422:
		switch (plane) {
		case 1:
			switch (order) {
			case DMA_INPUT_ORDER_CrYCbY:
				*img_format = SRDZ_YUV422_1P_VYUY;
				break;
			case DMA_INPUT_ORDER_CbYCrY:
				*img_format = SRDZ_YUV422_1P_UYVY;
				break;
			case DMA_INPUT_ORDER_YCrYCb:
				*img_format = SRDZ_YUV422_1P_YVYU;
				break;
			case DMA_INPUT_ORDER_YCbYCr:
				*img_format = SRDZ_YUV422_1P_YUYV;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 2:
			switch (order) {
			case DMA_INPUT_ORDER_CbCr:
				*img_format = SRDZ_YUV422_2P_UFIRST;
				break;
			case DMA_INPUT_ORDER_CrCb:
				*img_format = SRDZ_YUV422_2P_VFIRST;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = SRDZ_YUV422_3P;
			break;
		default:
			err_hw("img plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	default:
		err_hw("img format error - (%d/%d/%d)", format, order, plane);
		ret = -EINVAL;
		break;
	}

	return ret;
}


int fimc_is_hw_srdz_adjust_output_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format)
{
	int ret = 0;

	switch (format) {
	case DMA_OUTPUT_FORMAT_YUV420:
		switch (plane) {
		case 2:
			switch (order) {
			case DMA_OUTPUT_ORDER_CbCr:
				*img_format = SRDZ_YUV420_2P_UFIRST;
				break;
			case DMA_OUTPUT_ORDER_CrCb:
				* img_format = SRDZ_YUV420_2P_VFIRST;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = SRDZ_YUV420_3P;
			break;
		default:
			err_hw("output plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	case DMA_OUTPUT_FORMAT_YUV422:
		switch (plane) {
		case 1:
			switch (order) {
			case DMA_OUTPUT_ORDER_CrYCbY:
				*img_format = SRDZ_YUV422_1P_VYUY;
				break;
			case DMA_OUTPUT_ORDER_CbYCrY:
				*img_format = SRDZ_YUV422_1P_UYVY;
				break;
			case DMA_OUTPUT_ORDER_YCrYCb:
				*img_format = SRDZ_YUV422_1P_YVYU;
				break;
			case DMA_OUTPUT_ORDER_YCbYCr:
				*img_format = SRDZ_YUV422_1P_YUYV;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 2:
			switch (order) {
			case DMA_OUTPUT_ORDER_CbCr:
				*img_format = SRDZ_YUV422_2P_UFIRST;
				break;
			case DMA_OUTPUT_ORDER_CrCb:
				*img_format = SRDZ_YUV422_2P_VFIRST;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = SRDZ_YUV422_3P;
			break;
		default:
			err_hw("img plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	default:
		err_hw("img format error - (%d/%d/%d)", format, order, plane);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int fimc_is_hw_srdz_check_format(enum srdz_io_type type, u32 format, u32 bit_width,
	u32 width, u32 height)
{
	int ret = 0;

	switch (type) {
	case HW_SRDZ_OTF_OUTPUT:
		/* check otf output */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ OTF Output width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ OTF Output height(%d)", height);
		}

		if (format != OTF_OUTPUT_FORMAT_YUV422) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ OTF Output format(%d)", format);
		}

		if (bit_width != OTF_OUTPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ OTF Output format(%d)", bit_width);
		}
		break;
	case HW_SRDZ_DMA_INPUT:
		/* check dma input */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ DMA Input width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ DMA Input height(%d)", height);
		}

		if (format != DMA_INPUT_FORMAT_YUV422) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ DMA Input format(%d)", format);
		}

		if (bit_width != DMA_INPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ DMA Input format(%d)", bit_width);
		}
		break;
	case HW_SRDZ_DMA_OUTPUT:
		/* check dma output */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ DMA Output width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ DMA Output height(%d)", height);
		}

		if (!(format == DMA_OUTPUT_FORMAT_YUV422 ||
			format == DMA_OUTPUT_FORMAT_YUV420)) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ DMA Output format(%d)", format);
		}

		if (bit_width != DMA_OUTPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid SRDZ DMA Output format(%d)", bit_width);
		}
		break;
	default:
		err_hw("Invalid SRDZ type(%d)", type);
		break;
	}

	return ret;
}

void fimc_is_hw_srdz_size_dump(struct fimc_is_hw_ip *hw_ip)
{
	return;
}

