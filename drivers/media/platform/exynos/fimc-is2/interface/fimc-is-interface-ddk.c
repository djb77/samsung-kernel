/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-interface-ddk.h"
#include "fimc-is-hw-control.h"
#include "sfr/fimc-is-sfr-isp-v310.h"
#include "fimc-is-err.h"
#include <soc/samsung/bcm.h>

unsigned int stop_tirgger_count = 0;
void fimc_is_stop_trigger(void);

bool check_dma_done(struct fimc_is_hw_ip *hw_ip, u32 instance_id, u32 fcount)
{
	bool ret = false;
	int expected_fcount;
	struct fimc_is_frame *frame;
	struct fimc_is_frame *list_frame;
	struct fimc_is_framemgr *framemgr;
	int wq_id0 = WORK_MAX_MAP, wq_id1 = WORK_MAX_MAP;
	int output_id0 = ENTRY_END, output_id1 = ENTRY_END;
	u32 hw_fcount;
	bool flag_get_meta = true;
	ulong flags = 0;

	BUG_ON(!hw_ip);

	framemgr = hw_ip->framemgr;
	hw_fcount = atomic_read(&hw_ip->fcount);

	BUG_ON(!framemgr);

	framemgr_e_barrier_common(framemgr, 0, flags);
	frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
	framemgr_x_barrier_common(framemgr, 0, flags);

	if (frame == NULL) {
flush_config_frame:
		framemgr_e_barrier_common(framemgr, 0, flags);
		frame = peek_frame(framemgr, FS_HW_CONFIGURE);
		if (frame) {
			if (unlikely(frame->fcount + frame->cur_buf_index < hw_fcount)) {
				trans_frame(framemgr, frame, FS_HW_WAIT_DONE);
				framemgr_x_barrier_common(framemgr, 0, flags);
				fimc_is_hardware_frame_ndone(hw_ip, frame, frame->instance, IS_SHOT_INVALID_FRAMENUMBER);
				goto flush_config_frame;
			} else if (unlikely(frame->fcount + frame->cur_buf_index == hw_fcount)) {
				trans_frame(framemgr, frame, FS_HW_WAIT_DONE);
				framemgr_x_barrier_common(framemgr, 0, flags);
				fimc_is_hardware_frame_ndone(hw_ip, frame, frame->instance, IS_SHOT_INVALID_FRAMENUMBER);
				return true;
			}
		}
		framemgr_x_barrier_common(framemgr, 0, flags);
		err_hw("[ID:%d][F:%d,HWF:%d]check_dma_done: frame(null)!!", hw_ip->id, fcount, hw_fcount);
		return true;
	}

	/*
	 * The expected_fcount in which num_buffers is considered.
	 * It would be matched with frame->fcount which has "HW_WAIT_DONE" state.
	 *  ex. frame->fcount : 1, num_buffer : 4, fcount : 4 => expected_fcount : 1
	 */
	expected_fcount = fcount - (frame->num_buffers - 1);

	dbg_hw("check_dma [ID:%d][ddk:%d,exp:%d,hw:%d] frame(F:%d,idx:%d,num_buffers:%d)\n", hw_ip->id,
			fcount, expected_fcount, hw_fcount,
			frame->fcount, frame->cur_buf_index, frame->num_buffers);

	if (((frame->num_buffers > 1) && (expected_fcount != frame->fcount))
		|| ((frame->num_buffers == 1) && (expected_fcount != (frame->fcount + frame->cur_buf_index)))) {
		framemgr_e_barrier_common(framemgr, 0, flags);
		list_frame = find_frame(framemgr, FS_HW_WAIT_DONE, frame_fcount,
					(void *)(ulong)expected_fcount);
		framemgr_x_barrier_common(framemgr, 0, flags);
		if (list_frame == NULL) {
			warn_hw("[ID:%d]queued_count(%d) [ddk:%d,exp:%d,hw:%d] invalid frame(F:%d,idx:%d)", hw_ip->id,
				framemgr->queued_count[FS_HW_WAIT_DONE],
				fcount, expected_fcount, hw_fcount,
				frame->fcount, frame->cur_buf_index);
flush_wait_done_frame:
			framemgr_e_barrier_common(framemgr, 0, flags);
			frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
			if (frame) {
				if (unlikely(frame->fcount < expected_fcount)) {
					framemgr_x_barrier_common(framemgr, 0, flags);
					fimc_is_hardware_frame_ndone(hw_ip, frame, frame->instance, IS_SHOT_INVALID_FRAMENUMBER);
					goto flush_wait_done_frame;
				}
			} else {
				framemgr_x_barrier_common(framemgr, 0, flags);
				return true;
			}
			framemgr_x_barrier_common(framemgr, 0, flags);
		} else {
			frame = list_frame;
		}
	}

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
		wq_id0 = WORK_30P_FDONE; /* after BDS */
		output_id0 = ENTRY_3AP;
		wq_id1 = WORK_30C_FDONE; /* before BDS */
		output_id1 = ENTRY_3AC;
		break;
	case DEV_HW_3AA1:
		wq_id0 = WORK_31P_FDONE;
		output_id0 = ENTRY_3AP;
		wq_id1 = WORK_31C_FDONE;
		output_id1 = ENTRY_3AC;
		break;
	case DEV_HW_ISP0:
		wq_id0 = WORK_I0P_FDONE; /* chunk output */
		output_id0 = ENTRY_IXP;
		wq_id1 = WORK_I0C_FDONE; /* yuv output */
		output_id1 = ENTRY_IXC;
		break;
	case DEV_HW_ISP1:
		wq_id0 = WORK_I1P_FDONE;
		output_id0 = ENTRY_IXP;
		wq_id1 = WORK_I1C_FDONE;
		output_id1 = ENTRY_IXC;
		break;
	case DEV_HW_TPU0:
		wq_id1 = WORK_D0C_FDONE;
		output_id1 = ENTRY_DXC;
		break;
	case DEV_HW_TPU1:
		wq_id1 = WORK_D1C_FDONE;
		output_id1 = ENTRY_DXC;
		break;
	default:
		err_hw("[ID:%d][F:%d] invalid hw ID!!", hw_ip->id, fcount);
		return ret;
		break;
	}

	if (test_bit(output_id0, &frame->out_flag)) {
		dbg_hw("[ID:%d][F:%d]output_id[0x%x],wq_id[0x%x]\n",
			hw_ip->id, frame->fcount, output_id0, wq_id0);
		fimc_is_hardware_frame_done(hw_ip, NULL, wq_id0, output_id0,
			IS_SHOT_SUCCESS, flag_get_meta);
		ret = true;
		flag_get_meta = false;
	}

	if (test_bit(output_id1, &frame->out_flag)) {
		dbg_hw("[ID:%d][F:%d]output_id[0x%x],wq_id[0x%x]\n",
			hw_ip->id, frame->fcount, output_id1, wq_id1);
		fimc_is_hardware_frame_done(hw_ip, NULL, wq_id1, output_id1,
			IS_SHOT_SUCCESS, flag_get_meta);
		ret = true;
		flag_get_meta = false;
	}

	return ret;
}

static void fimc_is_lib_io_callback(void *this, enum lib_cb_event_type event_id,
	u32 instance_id)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip;
	int wq_id = WORK_MAX_MAP;
	int output_id = ENTRY_END;
	u32 hw_fcount, index;

	BUG_ON(!this);

	hw_ip = (struct fimc_is_hw_ip *)this;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);

	BUG_ON(!hw_ip->hardware);

	switch (event_id) {
	case LIB_EVENT_DMA_A_OUT_DONE:
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]]))
			info_hw("[%d][ID:%d][F:%d]DMA A\n", instance_id, hw_ip->id,
				hw_fcount);
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_DMA_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_DMA_END] = local_clock();
#if defined(SOC_3AAISP)
		wq_id = WORK_30C_FDONE;
		output_id = ENTRY_3AC;
#else
		switch (hw_ip->id) {
		case DEV_HW_3AA0: /* after BDS */
			wq_id = WORK_30P_FDONE;
			output_id = ENTRY_3AP;
			break;
		case DEV_HW_3AA1:
			wq_id = WORK_31P_FDONE;
			output_id = ENTRY_3AP;
			break;
		case DEV_HW_ISP0: /* chunk output */
			wq_id = WORK_I0P_FDONE;
			output_id = ENTRY_IXP;
			break;
		case DEV_HW_ISP1:
			wq_id = WORK_I1P_FDONE;
			output_id = ENTRY_IXP;
			break;
		default:
			break;
		}
#endif

		if (wq_id != WORK_MAX_MAP)
			fimc_is_hardware_frame_done(hw_ip, NULL, wq_id,
					output_id, IS_SHOT_SUCCESS, true);
		break;
	case LIB_EVENT_DMA_B_OUT_DONE:
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]]))
			info_hw("[%d][ID:%d][F:%d]DMA B\n", instance_id, hw_ip->id,
				hw_fcount);
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_DMA_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_DMA_END] = local_clock();
		switch (hw_ip->id) {
		case DEV_HW_3AA0: /* before BDS */
			wq_id = WORK_30C_FDONE;
			output_id = ENTRY_3AC;
			break;
		case DEV_HW_3AA1:
			wq_id = WORK_31C_FDONE;
			output_id = ENTRY_3AC;
			break;
		case DEV_HW_ISP0: /* yuv output */
			wq_id = WORK_I0C_FDONE;
			output_id = ENTRY_IXC;
			break;
		case DEV_HW_ISP1:
			wq_id = WORK_I1C_FDONE;
			output_id = ENTRY_IXC;
			break;
		default:
			break;
		}

		if (wq_id != WORK_MAX_MAP)
			fimc_is_hardware_frame_done(hw_ip, NULL, wq_id,
					output_id, IS_SHOT_SUCCESS, true);
		break;
	case LIB_EVENT_ERROR_CIN_OVERFLOW:
		bcm_stop(NULL);
		info_hw("[%d][ID:%d]LIB_EVENT_ERROR_CIN_OVERFLOW\n", instance_id, hw_ip->id);
		fimc_is_hardware_flush_frame(hw_ip, FS_HW_CONFIGURE, IS_SHOT_OVERFLOW);

#ifdef OVERFLOW_PANIC_ENABLE_ISCHAIN
		panic("CIN OVERFLOW!!");
#endif
		break;
	default:
		info_hw("[%d][ID:%d] event_id(%d)\n", instance_id, hw_ip->id, event_id);
		break;
	}

	return;
};

static void fimc_is_lib_camera_callback(void *this, enum lib_cb_event_type event_id,
	u32 instance_id, void *data)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip;
	ulong fcount;
	u32 hw_fcount, index;
	bool ret = false;
	bool frame_done = false;
	struct fimc_is_device_sensor *sensor;

	BUG_ON(!this);

	hw_ip = (struct fimc_is_hw_ip *)this;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);

	BUG_ON(!hw_ip->hardware);

	switch (event_id) {
	case LIB_EVENT_CONFIG_LOCK:
		fcount = (ulong)data;
		atomic_add(hw_ip->num_buffers, &hw_ip->count.cl);
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]]))
			info_hw("[%d][ID:%d][F:%d]C.L %d\n", instance_id, hw_ip->id,
				hw_fcount, (u32)fcount);

		if ((hw_ip->id == DEV_HW_3AA0)
		    && (hardware->sensor_position[instance_id] == SENSOR_POSITION_REAR)) {
		    sensor = hw_ip->group[instance_id]->device->sensor;
		    if (sensor && (sensor->max_target_fps < 60)
		        && (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.cl))) {
				info_hw("[%d][ID:%d]C.L fs(%d), cl(%d), fe(%d)\n",
						instance_id, hw_ip->id,
						atomic_read(&hw_ip->count.fs),
						atomic_read(&hw_ip->count.cl),
						atomic_read(&hw_ip->count.fe));

				stop_tirgger_count++;
				if (stop_tirgger_count > 1)
					fimc_is_stop_trigger();
			} else {
				stop_tirgger_count = 0;
			}
		}

		/* fcount : frame number of current frame in Vvalid */
		fimc_is_hardware_config_lock(hw_ip, instance_id, (u32)fcount);
		break;
	case LIB_EVENT_FRAME_START_ISR:
		hw_ip->debug_index[1] = hw_ip->debug_index[0] % DEBUG_FRAME_COUNT;
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].fcount = hw_ip->debug_index[0];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_START] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START] = local_clock();
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]]))
			info_hw("[%d][ID:%d][F:%d]F.S\n", instance_id, hw_ip->id,
				hw_fcount);

		fimc_is_hardware_frame_start(hw_ip, instance_id);
		atomic_add(hw_ip->num_buffers, &hw_ip->count.fs);
		break;
	case LIB_EVENT_FRAME_END:
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_END] = local_clock();
		atomic_add(hw_ip->num_buffers, &hw_ip->count.fe);
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance_id]]))
			info_hw("[%d][ID:%d][F:%d]F.E\n", instance_id, hw_ip->id,
				hw_fcount);

		fcount = (ulong)data;
		fimc_is_hw_g_ctrl(hw_ip, hw_ip->id, HW_G_CTRL_FRM_DONE_WITH_DMA, (void *)&frame_done);
		if (frame_done)
			ret = check_dma_done(hw_ip, instance_id, (u32)fcount);
		else
			dbg_hw("dma done interupt separate\n");

		if (!ret) {
			fimc_is_hardware_frame_done(hw_ip, NULL, -1, FIMC_IS_HW_CORE_END,
				IS_SHOT_SUCCESS, true);
		}
		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		wake_up(&hw_ip->status.wait_queue);

		CALL_HW_OPS(hw_ip, clk_gate, instance_id, false, false);
		break;
	default:
		break;
	}

	return;
};

struct lib_callback_func fimc_is_lib_cb_func = {
	.camera_callback	= fimc_is_lib_camera_callback,
	.io_callback		= fimc_is_lib_io_callback,
};

int fimc_is_lib_isp_chain_create(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id)
{
	int ret = 0;
	u32 chain_id;
	ulong base_addr, base_addr_b, set_b_offset;
	struct lib_system_config config;

	BUG_ON(!hw_ip);
	BUG_ON(!this);
	BUG_ON(!this->func);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
		chain_id = 0;
		break;
	case DEV_HW_3AA1:
		chain_id = 1;
		break;
	case DEV_HW_ISP0:
		chain_id = 0;
		break;
	case DEV_HW_ISP1:
		chain_id = 1;
		break;
	case DEV_HW_TPU0:
		chain_id = 0;
		break;
	case DEV_HW_TPU1:
		chain_id = 1;
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		return -EINVAL;
		break;
	}

	base_addr    = (ulong)hw_ip->regs;
	base_addr_b  = (ulong)hw_ip->regs_b;
	set_b_offset = (base_addr_b < base_addr) ? 0 : base_addr_b - base_addr;

	ret = CALL_LIBOP(this, chain_create, chain_id, base_addr, set_b_offset,
				&fimc_is_lib_cb_func);
	if (ret) {
		err_lib("chain_create fail (%d)", hw_ip->id);
		return -EINVAL;
	}

	/* set system config */
	memset(&config, 0, sizeof(struct lib_system_config));
	config.overflow_recovery = DDK_OVERFLOW_RECOVERY;

	ret = CALL_LIBOP(this, set_system_config, &config);
	if (ret) {
		err_lib("set_system_config fail (%d)", hw_ip->id);
		return -EINVAL;
	}

	info_lib("[%d]chain_create done [ID:%d][reg_base:0x%lx][b_offset:0x%lx](%d)\n",
		instance_id, hw_ip->id, base_addr, set_b_offset, config.overflow_recovery);

	return ret;
}

int fimc_is_lib_isp_object_create(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id, u32 rep_flag, u32 module_id)
{
	int ret = 0;
	u32 chain_id, input_type, obj_info = 0;
	struct fimc_is_group *head;

	BUG_ON(!hw_ip);
	BUG_ON(!this);
	BUG_ON(!this->func);
	BUG_ON(!hw_ip->group[instance_id]);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
		chain_id = 0;
		break;
	case DEV_HW_3AA1:
		chain_id = 1;
		break;
	case DEV_HW_ISP0:
		chain_id = 0;
		break;
	case DEV_HW_ISP1:
		chain_id = 1;
		break;
	case DEV_HW_TPU0:
		chain_id = 0;
		break;
	case DEV_HW_TPU1:
		chain_id = 1;
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		return -EINVAL;
		break;
	}

	head = GET_HEAD_GROUP_IN_DEVICE(FIMC_IS_DEVICE_ISCHAIN, hw_ip->group[instance_id]);

	BUG_ON(!head);

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state))
		input_type = 0; /* default */
	else
		input_type = 1;

	obj_info |= chain_id << CHAIN_ID_SHIFT;
	obj_info |= instance_id << INSTANCE_ID_SHIFT;
	obj_info |= rep_flag << REPROCESSING_FLAG_SHIFT;
	obj_info |= (input_type) << INPUT_TYPE_SHIFT;
	obj_info |= (module_id) << MODULE_ID_SHIFT;

	info_lib("obj_create: chain(%d), instance(%d), rep(%d), in_type(%d),"
		" obj_info(0x%08x), module_id(%d)\n",
		chain_id, instance_id, rep_flag, input_type, obj_info, module_id);

	ret = CALL_LIBOP(this, object_create, &this->object, obj_info, hw_ip);
	if (ret) {
		err_lib("object_create fail (%d)", hw_ip->id);
		return -EINVAL;
	}

	info_lib("[%d]object_create done [ID:%d]\n", instance_id, hw_ip->id);

	return ret;
}

void fimc_is_lib_isp_chain_destroy(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id)
{
	int ret = 0;
	u32 chain_id;

	BUG_ON(!hw_ip);
	BUG_ON(!this->func);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
		chain_id = 0;
		break;
	case DEV_HW_3AA1:
		chain_id = 1;
		break;
	case DEV_HW_ISP0:
		chain_id = 0;
		break;
	case DEV_HW_ISP1:
		chain_id = 1;
		break;
	case DEV_HW_TPU0:
		chain_id = 0;
		break;
	case DEV_HW_TPU1:
		chain_id = 1;
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		return;
		break;
	}

	ret = CALL_LIBOP(this, chain_destroy, chain_id);
	if (ret) {
		err_lib("chain_destroy fail (%d)", hw_ip->id);
		return;
	}
	info_lib("[%d]chain_destroy done [ID:%d]\n", instance_id, hw_ip->id);

	return;
}

void fimc_is_lib_isp_object_destroy(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id)
{
	int ret = 0;

	BUG_ON(!hw_ip);
	BUG_ON(!this);
	BUG_ON(!this->func);

	if (!this->object) {
		err_lib("object(NULL) destroy fail (%d)", hw_ip->id);
		return;
	}

	ret = CALL_LIBOP(this, object_destroy, this->object, instance_id);
	if (ret) {
		err_lib("object_destroy fail (%d)", hw_ip->id);
		return;
	}

	info_lib("[%d]object_destroy done [ID:%d]\n", instance_id, hw_ip->id);

	return;
}

int fimc_is_lib_isp_set_param(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, void *param)
{
	int ret;

	BUG_ON(!hw_ip);
	BUG_ON(!this);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_3AA1:
		ret = CALL_LIBOP(this, set_param, this->object, param);
		if (ret)
			err_lib("3aa set_param fail (%d)", hw_ip->id);
		break;
	case DEV_HW_ISP0:
	case DEV_HW_ISP1:
		ret = CALL_LIBOP(this, set_param, this->object, param);
		if (ret)
			err_lib("isp set_param fail (%d)", hw_ip->id);
		break;
	case DEV_HW_TPU0:
	case DEV_HW_TPU1:
		ret = CALL_LIBOP(this, set_param, this->object, param);
		if (ret)
			err_lib("tpu set_param fail (%d)", hw_ip->id);
		break;
	default:
		ret = -EINVAL;
		err_lib("invalid hw (%d)", hw_ip->id);
		break;
	}

	return ret;
}

int fimc_is_lib_isp_set_ctrl(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, struct fimc_is_frame *frame)
{
	int ret = 0;

	BUG_ON(!hw_ip);
	BUG_ON(!this);
	BUG_ON(!frame);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_3AA1:
		ret = CALL_LIBOP(this, set_ctrl, this->object, frame->instance,
					frame->fcount, frame->shot);
		if (ret)
			err_lib("3aa set_ctrl fail (%d)", hw_ip->id);
		break;
	case DEV_HW_ISP0:
	case DEV_HW_ISP1:
		ret = CALL_LIBOP(this, set_ctrl, this->object, frame->instance,
					frame->fcount, frame->shot);
		if (ret)
			err_lib("isp set_ctrl fail (%d)", hw_ip->id);
		break;
	case DEV_HW_TPU0:
	case DEV_HW_TPU1:
		ret = CALL_LIBOP(this, set_ctrl, this->object, frame->instance,
					frame->fcount, frame->shot);
		if (ret)
			err_lib("tpu set_ctrl fail (%d)", hw_ip->id);
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		break;
	}

	return 0;
}

void fimc_is_lib_isp_shot(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, void *param_set, struct camera2_shot *shot)
{
	int ret = 0;

	BUG_ON(!hw_ip);
	BUG_ON(!this);
	BUG_ON(!param_set);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_3AA1:
		ret = CALL_LIBOP(this, shot, this->object,
					(struct taa_param_set *)param_set,
					shot, hw_ip->num_buffers);
		if (ret)
			err_lib("3aa shot fail (%d)", hw_ip->id);

		break;
	case DEV_HW_ISP0:
	case DEV_HW_ISP1:
		ret = CALL_LIBOP(this, shot, this->object,
					(struct isp_param_set *)param_set,
					shot, hw_ip->num_buffers);
		if (ret)
			err_lib("isp shot fail (%d)", hw_ip->id);
		break;
	case DEV_HW_TPU0:
	case DEV_HW_TPU1:
		ret = CALL_LIBOP(this, shot, this->object,
					(struct tpu_param_set *)param_set,
					shot, hw_ip->num_buffers);
		if (ret)
			err_lib("tpu shot fail (%d)", hw_ip->id);
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		break;
	}
}

int fimc_is_lib_isp_get_meta(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, struct fimc_is_frame *frame)

{
	int ret = 0;

	BUG_ON(!hw_ip);
	BUG_ON(!this);
	BUG_ON(!frame);
	BUG_ON(!this->func);
	BUG_ON(!this->object);
	BUG_ON(!frame->shot);

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_3AA1:
		ret = CALL_LIBOP(this, get_meta, this->object, frame->instance,
					frame->fcount, frame->shot);
		if (ret)
			err_lib("3aa get_meta fail (%d)", hw_ip->id);
		break;
	case DEV_HW_ISP0:
	case DEV_HW_ISP1:
		ret = CALL_LIBOP(this, get_meta, this->object, frame->instance,
					frame->fcount, frame->shot);
		if (ret)
			err_lib("isp get_meta fail (%d)", hw_ip->id);
		break;
	case DEV_HW_TPU0:
	case DEV_HW_TPU1:
		ret = CALL_LIBOP(this, get_meta, this->object, frame->instance,
					frame->fcount, frame->shot);
		if (ret)
			err_lib("tpu get_meta fail (%d)", hw_ip->id);
		break;
	default:
		err_lib("invalid hw (%d)", hw_ip->id);
		break;
	}

	return ret;
}

void fimc_is_lib_isp_stop(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_lib_isp *this, u32 instance_id)
{
	int ret = 0;

	BUG_ON(!hw_ip);
	BUG_ON(!this);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	ret = CALL_LIBOP(this, stop, this->object, instance_id);
	if (ret) {
		err_lib("object_suspend fail (%d)", hw_ip->id);
		return;
	}
	info_lib("[%d]object_suspend done [ID:%d]\n", instance_id, hw_ip->id);

	return;
}

int fimc_is_lib_isp_create_tune_set(struct fimc_is_lib_isp *this,
	ulong addr, u32 size, u32 index, int flag, u32 instance_id)
{
	int ret = 0;
	struct lib_tune_set tune_set;

	BUG_ON(!this);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	tune_set.index = index;
	tune_set.addr = addr;
	tune_set.size = size;
	tune_set.decrypt_flag = flag;

	ret = CALL_LIBOP(this, create_tune_set, this->object, instance_id,
				&tune_set);
	if (ret) {
		err_lib("create_tune_set fail (%d)", ret);
		return ret;
	}

	info_lib("[%d]create_tune_set index(%d)\n", instance_id, index);

	return ret;
}

int fimc_is_lib_isp_apply_tune_set(struct fimc_is_lib_isp *this,
	u32 index, u32 instance_id)
{
	int ret = 0;

	BUG_ON(!this);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	ret = CALL_LIBOP(this, apply_tune_set, this->object, instance_id, index);
	if (ret) {
		err_lib("apply_tune_set fail (%d)", ret);
		return ret;
	}

	return ret;
}

int fimc_is_lib_isp_delete_tune_set(struct fimc_is_lib_isp *this,
	u32 index, u32 instance_id)
{
	int ret = 0;

	BUG_ON(!this);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	ret = CALL_LIBOP(this, delete_tune_set, this->object, instance_id, index);
	if (ret) {
		err_lib("delete_tune_set fail (%d)", ret);
		return ret;
	}

	info_lib("[%d]delete_tune_set index(%d)\n", instance_id, index);

	return ret;
}

int fimc_is_lib_isp_load_cal_data(struct fimc_is_lib_isp *this,
	u32 instance_id, ulong addr)
{
	char version[32];
	int ret = 0;

	BUG_ON(!this);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	memcpy(version, (void *)(addr + 0x20), (FIMC_IS_CAL_VER_SIZE - 1));
	version[FIMC_IS_CAL_VER_SIZE] = '\0';
	info_lib("CAL version: %s\n", version);

	ret = CALL_LIBOP(this, load_cal_data, this->object, instance_id, addr);
	if (ret) {
		err_lib("apply_tune_set fail (%d)", ret);
		return ret;
	}

	return ret;
}

int fimc_is_lib_isp_get_cal_data(struct fimc_is_lib_isp *this,
	u32 instance_id, struct cal_info *data, int type)
{
	int ret = 0;

	BUG_ON(!this);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	ret = CALL_LIBOP(this, get_cal_data, this->object, instance_id,
				data, type);
	if (ret) {
		err_lib("apply_tune_set fail (%d)", ret);
		return ret;
	}

	return ret;
}

int fimc_is_lib_isp_sensor_info_mode_chg(struct fimc_is_lib_isp *this,
	u32 instance_id, struct camera2_shot *shot)
{
	int ret = 0;

	BUG_ON(!this);
	BUG_ON(!shot);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	ret = CALL_LIBOP(this, sensor_info_mode_chg, this->object, instance_id,
				shot);
	if (ret) {
		err_lib("sensor_info_mode_chg fail (%d)", ret);
		return ret;
	}

	return ret;
}

int fimc_is_lib_isp_sensor_update_control(struct fimc_is_lib_isp *this,
	u32 instance_id, u32 frame_count, struct camera2_shot *shot)
{
	int ret = 0;

	BUG_ON(!this);
	BUG_ON(!this->func);
	BUG_ON(!this->object);

	ret = CALL_LIBOP(this, sensor_update_ctl, this->object, instance_id,
				frame_count, shot);
	if (ret) {
		err_lib("sensor_update_ctl fail (%d)", ret);
		return ret;
	}

	return ret;
}

int fimc_is_lib_isp_convert_face_map(struct fimc_is_hardware *hardware,
	struct taa_param_set *param_set, struct fimc_is_frame *frame)
{
	int ret = 0;
#ifdef ENABLE_VRA
	int i;
	u32 fd_width = 0, fd_height = 0;
	u32 bayer_crop_width, bayer_crop_height;
	struct fimc_is_group *group = NULL;
	struct fimc_is_group *group_vra;
	struct camera2_shot *shot = NULL;
	struct fimc_is_device_ischain *device = NULL;
	struct param_otf_input *fd_otf_input;
	struct param_dma_input *fd_dma_input;

	BUG_ON(!hardware);
	BUG_ON(!param_set);
	BUG_ON(!frame);

	shot = frame->shot;
	BUG_ON(!shot);

	group = (struct fimc_is_group *)frame->group;
	BUG_ON(!group);

	device = group->device;
	BUG_ON(!device);

	if (shot->uctl.fdUd.faceDetectMode == FACEDETECT_MODE_OFF)
		return 0;

	/*
	 * The face size which an algorithm uses is determined
	 * by the input bayer crop size of 3aa.
	 */
	if (param_set->otf_input.cmd == OTF_INPUT_COMMAND_ENABLE) {
		bayer_crop_width = param_set->otf_input.bayer_crop_width;
		bayer_crop_height = param_set->otf_input.bayer_crop_height;
	} else if (param_set->dma_input.cmd == DMA_INPUT_COMMAND_ENABLE) {
		bayer_crop_width = param_set->dma_input.bayer_crop_width;
		bayer_crop_height = param_set->dma_input.bayer_crop_height;
	} else {
		err_hw("invalid hw input!!\n");
		return -EINVAL;
	}

	if (bayer_crop_width == 0 || bayer_crop_height == 0) {
		warn_hw("%s: invalid crop size (%d * %d)!!\n",
			__func__, bayer_crop_width, bayer_crop_height);
		return 0;
	}

	/* The face size is determined by the fd input size */
	group_vra = &device->group_vra;
	if (test_bit(FIMC_IS_GROUP_INIT, &group_vra->state)
		&& (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &group_vra->state))) {

		fd_dma_input = fimc_is_itf_g_param(device, frame, PARAM_FD_DMA_INPUT);
		if (fd_dma_input->cmd == DMA_INPUT_COMMAND_ENABLE) {
			fd_width = fd_dma_input->width;
			fd_height = fd_dma_input->height;
		}
	} else {
		fd_otf_input = fimc_is_itf_g_param(device, frame, PARAM_FD_OTF_INPUT);
		if (fd_otf_input->cmd == OTF_INPUT_COMMAND_ENABLE) {
			fd_width = fd_otf_input->width;
			fd_height = fd_otf_input->height;
		}
	}

	if (fd_width == 0 || fd_height == 0) {
		warn_hw("%s: invalid fd size (%d * %d)!!\n",
			__func__, fd_width, fd_height);
		return 0;
	}

	/* Convert face size */
	for (i = 0; i < CAMERA2_MAX_FACES; i++) {
		if (shot->uctl.fdUd.faceScores[i] == 0)
			continue;

		shot->uctl.fdUd.faceRectangles[i][0] =
				CONVRES(shot->uctl.fdUd.faceRectangles[i][0],
				fd_width, bayer_crop_width);
		shot->uctl.fdUd.faceRectangles[i][1] =
				CONVRES(shot->uctl.fdUd.faceRectangles[i][1],
				fd_height, bayer_crop_height);
		shot->uctl.fdUd.faceRectangles[i][2] =
				CONVRES(shot->uctl.fdUd.faceRectangles[i][2],
				fd_width, bayer_crop_width);
		shot->uctl.fdUd.faceRectangles[i][3] =
				CONVRES(shot->uctl.fdUd.faceRectangles[i][3],
				fd_height, bayer_crop_height);

		dbg_lib("%s: ID(%d), x_min(%d), y_min(%d), x_max(%d), y_max(%d)\n",
			__func__, shot->uctl.fdUd.faceIds[i],
			shot->uctl.fdUd.faceRectangles[i][0],
			shot->uctl.fdUd.faceRectangles[i][1],
			shot->uctl.fdUd.faceRectangles[i][2],
			shot->uctl.fdUd.faceRectangles[i][3]);
	}

#endif
	return ret;
}

void fimc_is_isp_get_bcrop1_size(void __iomem *base_addr, u32 *width, u32 *height)
{
	*width  = fimc_is_hw_get_field(base_addr, &isp_regs[ISP_R_BCROP1_SIZE_X], &isp_fields[ISP_F_BCROP1_SIZE_X]);
	*height = fimc_is_hw_get_field(base_addr, &isp_regs[ISP_R_BCROP1_SIZE_Y], &isp_fields[ISP_F_BCROP1_SIZE_Y]);
}
