/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-mcscaler-v2.h"
#include "api/fimc-is-hw-api-mcscaler-v2.h"
#include "../interface/fimc-is-interface-ischain.h"
#include "fimc-is-param.h"
#include "fimc-is-err.h"

struct semaphore	smp_mcsc_g_enable;
spinlock_t	shared_output_slock;
static ulong hw_mcsc_out_configured = 0xFFFF;
#define HW_MCSC_OUT_CLEARED_ALL (15)

static int fimc_is_hw_mcsc_rdma_cfg(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame);
static void fimc_is_hw_mcsc_wdma_cfg(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame);

static int fimc_is_hw_mcsc_handle_interrupt(u32 id, void *context)
{
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip = NULL;
	u32 status, intr_mask, intr_status;
	bool err_intr_flag = false;
	int ret = 0;
	u32 hl = 0, vl = 0;
	u32 instance;
	u32 hw_fcount, index;
	struct mcs_param *param;
	bool flag_clk_gate = false;

	hw_ip = (struct fimc_is_hw_ip *)context;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);
	instance = atomic_read(&hw_ip->instance);
	param = &hw_ip->region[instance]->parameter.mcs;

	if (!test_bit(HW_OPEN, &hw_ip->state)) {
		err_hw("[ID:%d][MCSC] invalid interrupt", hw_ip->id);
		return 0;
	}

	fimc_is_scaler_get_input_status(hw_ip->regs, hw_ip->id, &hl, &vl);
	/* read interrupt status register (sc_intr_status) */
	intr_mask = fimc_is_scaler_get_intr_mask(hw_ip->regs, hw_ip->id);
	intr_status = fimc_is_scaler_get_intr_status(hw_ip->regs, hw_ip->id);
	status = (~intr_mask) & intr_status;

	fimc_is_scaler_clear_intr_src(hw_ip->regs, hw_ip->id, status);

	if (!test_bit(HW_RUN, &hw_ip->state)) {
		err_hw("[MCSC]HW disabled!! interrupt(0x%x)", status);
		goto p_err;
	}

	if (status & (1 << INTR_MC_SCALER_OVERFLOW)) {
		err_hw("[%d][ID:%d][MCSC]Overflow!! (0x%x)", instance, hw_ip->id, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_OUTSTALL)) {
		err_hw("[%d][ID:%d][MCSC]Output Block BLOCKING!! (0x%x)", instance, hw_ip->id, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF)) {
		err_hw("[%d][ID:%d][MCSC]Input OTF Vertical Underflow!! (0x%x)", instance, hw_ip->id, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF)) {
		err_hw("[%d][ID:%d][MCSC]Input OTF Vertical Overflow!! (0x%x)", instance, hw_ip->id, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF)) {
		err_hw("[%d][ID:%d][MCSC]Input OTF Horizontal Underflow!! (0x%x)", instance, hw_ip->id, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF)) {
		err_hw("[%d][ID:%d][MCSC]Input OTF Horizontal Overflow!! (0x%x)", instance, hw_ip->id, status);
		err_intr_flag = true;
	}

	if (status & (1 << INTR_MC_SCALER_WDMA_FINISH))
		err_hw("[%d][ID:%d][MCSC]Disabeld interrupt occurred! WDAM FINISH!! (0x%x)", instance, hw_ip->id, status);

	if (status & (1 << INTR_MC_SCALER_FRAME_END)) {
		atomic_inc(&hw_ip->count.fe);
		hw_ip->cur_e_int++;
		if (hw_ip->cur_e_int >= hw_ip->num_buffers) {
			fimc_is_hw_mcsc_frame_done(hw_ip, NULL, IS_SHOT_SUCCESS);

			if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
				info_hw("[ID:%d][F:%d]F.E\n", hw_ip->id, hw_fcount);

			atomic_set(&hw_ip->status.Vvalid, V_BLANK);
			if (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.fe)) {
				err_hw("[%d][ID:%d][MCSC] fs(%d), fe(%d), dma(%d), status(0x%x)", instance, hw_ip->id,
					atomic_read(&hw_ip->count.fs),
					atomic_read(&hw_ip->count.fe),
					atomic_read(&hw_ip->count.dma), status);
			}

			wake_up(&hw_ip->status.wait_queue);
			flag_clk_gate = true;
			hw_ip->mframe = NULL;
		}
	}

	if (status & (1 << INTR_MC_SCALER_FRAME_START)) {
		atomic_inc(&hw_ip->count.fs);
		up(&smp_mcsc_g_enable);
		hw_ip->cur_s_int++;
		if (hw_ip->cur_s_int == 1) {
			hw_ip->debug_index[1] = hw_ip->debug_index[0] % DEBUG_FRAME_COUNT;
			index = hw_ip->debug_index[1];
			hw_ip->debug_info[index].fcount = hw_ip->debug_index[0];
			hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_START] = raw_smp_processor_id();
			hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START] = cpu_clock(raw_smp_processor_id());
			if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
				info_hw("[%d][ID:%d][F:%d]F.S\n", instance, hw_ip->id, hw_fcount);

			if (param->input.dma_cmd == DMA_INPUT_COMMAND_ENABLE)
				fimc_is_hardware_frame_start(hw_ip, instance);
		}

		if (hw_ip->cur_s_int < hw_ip->num_buffers) {
			if (hw_ip->mframe) {
				struct fimc_is_frame *mframe = hw_ip->mframe;
				mframe->cur_buf_index = hw_ip->cur_s_int;
				/* WDMA cfg */
				fimc_is_hw_mcsc_wdma_cfg(hw_ip, mframe);

				/* RDMA cfg */
				if (param->input.dma_cmd == DMA_INPUT_COMMAND_ENABLE) {
					ret = fimc_is_hw_mcsc_rdma_cfg(hw_ip, mframe);
					if (ret) {
						err_hw("[%d][ID:%d][F:%d]mcsc rdma_cfg failed\n",
							mframe->instance, hw_ip->id, mframe->fcount);
						return ret;
					}
				}
			} else {
				err_hw("[ID:%d]mframe is null(s:%d, e:%d, t:%d)", hw_ip->id,
					hw_ip->cur_s_int, hw_ip->cur_e_int, hw_ip->num_buffers);
			}
		} else {
			struct fimc_is_group *group;
			group = hw_ip->group[instance];
			/*
			 * In case of M2M mcsc, just supports only one buffering.
			 * So, in start irq, "setting to stop mcsc for N + 1" should be assigned.
			 *
			 * TODO: Don't touch global control, but we don't know how to be mapped
			 *       with group-id and scX_ctrl.
			 */
			if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state))
				fimc_is_scaler_stop(hw_ip->regs, hw_ip->id);
		}
	}

	if (err_intr_flag) {
		info_hw("[%d][ID:%d][MCSC][F:%d] Ocurred error interrupt (%d,%d) status(0x%x)\n",
			instance, hw_ip->id, hw_fcount, hl, vl, status);
		fimc_is_scaler_dump(hw_ip->regs);
		fimc_is_hardware_size_dump(hw_ip);
	}

	if (flag_clk_gate)
		CALL_HW_OPS(hw_ip, clk_gate, instance, false, false);

p_err:
	return ret;
}

void fimc_is_hw_mcsc_hw_info(struct fimc_is_hw_ip *hw_ip, struct fimc_is_hw_mcsc_cap *cap)
{
	int i = 0;

	if (!(hw_ip && cap))
		return;

	info_hw("[MCSC(%d)]==== h/w info(ver:0x%X) ====\n",
			hw_ip->id, cap->hw_ver);
	info_hw("[IN] max_out:%d, in(otf/dma):%d/%d, hwfc:%d\n",
			cap->max_output, cap->in_otf, cap->in_dma, cap->hwfc);

	for (i = MCSC_OUTPUT0; i < cap->max_output; i++)
		info_hw("[OUT%d] out(otf/dma):%d/%d, hwfc:%d\n", i,
			cap->out_otf[i], cap->out_dma[i], cap->out_hwfc[i]);

	info_hw("[MCSC]========================\n");
}

const struct fimc_is_hw_ip_ops fimc_is_hw_mcsc_ops = {
	.open			= fimc_is_hw_mcsc_open,
	.init			= fimc_is_hw_mcsc_init,
	.close			= fimc_is_hw_mcsc_close,
	.enable			= fimc_is_hw_mcsc_enable,
	.disable		= fimc_is_hw_mcsc_disable,
	.shot			= fimc_is_hw_mcsc_shot,
	.set_param		= fimc_is_hw_mcsc_set_param,
	.frame_ndone		= fimc_is_hw_mcsc_frame_ndone,
	.load_setfile		= fimc_is_hw_mcsc_load_setfile,
	.apply_setfile		= fimc_is_hw_mcsc_apply_setfile,
	.delete_setfile		= fimc_is_hw_mcsc_delete_setfile,
	.size_dump		= fimc_is_hw_mcsc_size_dump,
	.clk_gate		= fimc_is_hardware_clk_gate
};

int fimc_is_hw_mcsc_probe(struct fimc_is_hw_ip *hw_ip, struct fimc_is_interface *itf,
	struct fimc_is_interface_ischain *itfc,	int id)
{
	int ret = 0;
	int hw_slot = -1;

	BUG_ON(!hw_ip);
	BUG_ON(!itf);
	BUG_ON(!itfc);

	/* initialize device hardware */
	hw_ip->id   = id;
	hw_ip->ops  = &fimc_is_hw_mcsc_ops;
	hw_ip->itf  = itf;
	hw_ip->itfc = itfc;
	atomic_set(&hw_ip->fcount, 0);
	hw_ip->internal_fcount = 0;
	hw_ip->is_leader = true;
	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	atomic_set(&hw_ip->rsccount, 0);
	init_waitqueue_head(&hw_ip->status.wait_queue);

	/* set mcsc sfr base address */
	hw_slot = fimc_is_hw_slot_id(id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_hw("invalid hw_slot (%d, %d)", id, hw_slot);
		return -EINVAL;
	}

	/* set mcsc interrupt handler */
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &fimc_is_hw_mcsc_handle_interrupt;

	clear_bit(HW_OPEN, &hw_ip->state);
	clear_bit(HW_INIT, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);
	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_TUNESET, &hw_ip->state);
	spin_lock_init(&shared_output_slock);
	sema_init(&smp_mcsc_g_enable, 1);

	info_hw("[ID:%2d] probe done\n", id);

	return ret;
}

int fimc_is_hw_mcsc_open(struct fimc_is_hw_ip *hw_ip, u32 instance, u32 *size)
{
	int ret = 0;

	BUG_ON(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	*size = sizeof(struct fimc_is_hw_mcsc);

	frame_manager_probe(hw_ip->framemgr, FRAMEMGR_ID_HW | (1 << hw_ip->id));
	frame_manager_probe(hw_ip->framemgr_late, FRAMEMGR_ID_HW | (1 << hw_ip->id) | 0xF000);
	frame_manager_open(hw_ip->framemgr, FIMC_IS_MAX_HW_FRAME);
	frame_manager_open(hw_ip->framemgr_late, FIMC_IS_MAX_HW_FRAME_LATE);

	dbg_hw("[%d][ID:%d]open: [G:0x%x], framemgr[ID:0x%x]",
			instance, hw_ip->id, GROUP_ID(hw_ip->group[instance]->id), hw_ip->framemgr->id);

	hw_ip->mframe = NULL;
	atomic_set(&hw_ip->status.Vvalid, V_BLANK);

	return ret;
}

int fimc_is_hw_mcsc_init(struct fimc_is_hw_ip *hw_ip, struct fimc_is_group *group,
	bool flag, u32 module_id)
{
	int ret = 0;
	u32 instance = 0, entry, output_id;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap;
	struct fimc_is_subdev *subdev;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_video *video;

	BUG_ON(!hw_ip);

	instance = group->instance;
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	cap = GET_MCSC_HW_CAP(hw_ip);
	if (!cap) {
		err_hw("failed to get hw_mcsc_cap(%p)", cap);
		return -EINVAL;
	}

	/* get the mcsc hw info */
	ret = fimc_is_hw_query_cap((void *)&hw_mcsc->cap, hw_ip->id);
	if (ret) {
		err_hw("failed to get hw info(%d)", hw_ip->id);
		return -EINVAL;
	}

	hw_mcsc->rep_flag[instance] = flag;

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		entry = GET_ENTRY_FROM_OUTPUT_ID(output_id);
		subdev = group->subdev[entry];
		if (!subdev)
			continue;

		vctx = subdev->vctx;
		if (!vctx) {
			continue;
		}

		video = vctx->video;
		if (!video) {
			err_hw("video is NULL. entry(%d)", entry);
			BUG();
		}
		set_bit(output_id, &hw_mcsc->out_en);
	}

	/* skip duplicated h/w setting */
	if (test_bit(HW_INIT, &hw_ip->state))
		return 0;

	/* print hw info */
	fimc_is_hw_mcsc_hw_info(hw_ip, &hw_mcsc->cap);

	hw_mcsc->instance = FIMC_IS_STREAM_COUNT;

	dbg_hw("[%d][ID:%d]hw_mcsc_init: out_en[0x%lx]\n", instance, hw_ip->id, hw_mcsc->out_en);

	return ret;
}

int fimc_is_hw_mcsc_close(struct fimc_is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;

	BUG_ON(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_close(hw_ip->framemgr);
	frame_manager_close(hw_ip->framemgr_late);

	info_hw("[%d]close (%d)(%d)\n", instance, hw_ip->id, atomic_read(&hw_ip->rsccount));

	return ret;
}

int fimc_is_hw_mcsc_enable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	ulong flag = 0;

	BUG_ON(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		err_hw("[%d]not initialized!! (%d)", instance, hw_ip->id);
		return -EINVAL;
	}

	atomic_inc(&hw_ip->run_rsccount);

	if (test_bit(HW_RUN, &hw_ip->state))
		return ret;

	dbg_hw("[ID:%d]mcsc_enable: start, (0x%lx)\n", hw_ip->id, hw_mcsc_out_configured);

	spin_lock_irqsave(&shared_output_slock, flag);
	ret = fimc_is_hw_mcsc_reset(hw_ip);
	if (ret != 0) {
		err_hw("MCSC sw reset fail");
		spin_unlock_irqrestore(&shared_output_slock, flag);
		return -ENODEV;
	}

	/* input source select 0: otf, 1:rdma */
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &hw_ip->group[instance]->state))
		fimc_is_scaler_set_input_source(hw_ip->regs, hw_ip->id, 0);
	else
		fimc_is_scaler_set_input_source(hw_ip->regs, hw_ip->id, 1);

	ret = fimc_is_hw_mcsc_clear_interrupt(hw_ip);
	if (ret != 0) {
		err_hw("MCSC sw clear_interrupt fail");
		spin_unlock_irqrestore(&shared_output_slock, flag);
		return -ENODEV;
	}
	dbg_hw("[%d][ID:%d]mcsc_enable: done, (0x%lx)\n", instance, hw_ip->id, hw_mcsc_out_configured);

	set_bit(HW_RUN, &hw_ip->state);
	spin_unlock_irqrestore(&shared_output_slock, flag);

	return ret;
}

int fimc_is_hw_mcsc_disable(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	int hw_slot = -1;
	u32 input_id, output_id;
	long timetowait;
	bool config = true;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip0 = NULL, *hw_ip1 = NULL;

	BUG_ON(!hw_ip);
	BUG_ON(!cap);
	BUG_ON(!hw_ip->priv_info);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (atomic_dec_return(&hw_ip->run_rsccount) > 0)
		return 0;

	info_hw("[%d][ID:%d]mcsc_disable: Vvalid(%d)\n", instance, hw_ip->id,
		atomic_read(&hw_ip->status.Vvalid));

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (test_bit(HW_RUN, &hw_ip->state)) {
		timetowait = wait_event_timeout(hw_ip->status.wait_queue,
			!atomic_read(&hw_ip->status.Vvalid),
			FIMC_IS_HW_STOP_TIMEOUT);

		if (!timetowait) {
			err_hw("[%d][ID:%d] wait FRAME_END timeout (%ld)", instance,
				hw_ip->id, timetowait);
			ret = -ETIME;
		}

		ret = fimc_is_hw_mcsc_clear_interrupt(hw_ip);
		if (ret != 0) {
			err_hw("MCSC sw clear_interrupt fail");
			return -ENODEV;
		}

		clear_bit(HW_RUN, &hw_ip->state);
		clear_bit(HW_CONFIG, &hw_ip->state);
	} else {
		dbg_hw("[%d]already disabled (%d)\n", instance, hw_ip->id);
	}

	hw_ip->mframe = NULL;

	hardware = hw_ip->hardware;
	hw_slot = fimc_is_hw_slot_id(DEV_HW_MCSC0);
	if (valid_hw_slot_id(hw_slot))
		hw_ip0 = &hardware->hw_ip[hw_slot];

	hw_slot = fimc_is_hw_slot_id(DEV_HW_MCSC1);
	if (valid_hw_slot_id(hw_slot))
		hw_ip1 = &hardware->hw_ip[hw_slot];

	if (hw_ip0 && test_bit(HW_RUN, &hw_ip0->state))
		return 0;

	if (hw_ip1 && test_bit(HW_RUN, &hw_ip1->state))
		return 0;


	if (hw_ip0)
		fimc_is_scaler_stop(hw_ip0->regs, hw_ip0->id);

	if (hw_ip1)
		fimc_is_scaler_stop(hw_ip1->regs, hw_ip1->id);

	/* disable MCSC */
	if (cap->in_dma == MCSC_CAP_SUPPORT)
		fimc_is_scaler_clear_rdma_addr(hw_ip->regs);

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
		config = (input_id == hw_ip->id ? true: false);
		if (cap->enable_shared_output == false || !test_bit(output_id, &hw_mcsc_out_configured)) {
			info_hw("[%d][ID:%d][OUT:%d]hw_mcsc_disable: clear_wdma_addr\n", instance, hw_ip->id, output_id);
			fimc_is_scaler_clear_wdma_addr(hw_ip->regs, output_id);
		}
	}

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++)
		set_bit(output_id, &hw_mcsc_out_configured);
	clear_bit(HW_MCSC_OUT_CLEARED_ALL, &hw_mcsc_out_configured);

	info_hw("[%d][ID:%d]mcsc_disable: done, (0x%lx)\n", instance, hw_ip->id, hw_mcsc_out_configured);

	return ret;
}

static int fimc_is_hw_mcsc_rdma_cfg(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame)
{
	int ret = 0;
	int i;
	u32 rdma_addr[4] = {0};
	struct mcs_param *param;
	u32 plane;

	param = &hw_ip->region[frame->instance]->parameter.mcs;

	plane = param->input.plane;
	for (i = 0; i < plane; i++)
		rdma_addr[i] = frame->shot->uctl.scalerUd.sourceAddress[plane * frame->cur_buf_index + i];

	/* DMA in */
	dbg_hw("[%d][ID:%d]mcsc_rdma_cfg [F:%d][addr: %x]\n",
		frame->instance, hw_ip->id, frame->fcount, rdma_addr[0]);

	if ((rdma_addr[0] == 0)
		&& (param->input.dma_cmd == DMA_INPUT_COMMAND_ENABLE)) {
		err_hw("Wrong rdma_addr(%x)\n", rdma_addr[0]);
		fimc_is_scaler_clear_rdma_addr(hw_ip->regs);
		ret = -EINVAL;
		return ret;
	}

	/* use only one buffer (per-frame) */
	fimc_is_scaler_set_rdma_frame_seq(hw_ip->regs,
		0x1 << USE_DMA_BUFFER_INDEX);
	fimc_is_scaler_set_rdma_addr(hw_ip->regs,
		rdma_addr[0], rdma_addr[1], rdma_addr[2],
		USE_DMA_BUFFER_INDEX);

	return ret;
}

static void fimc_is_hw_mcsc_wdma_cfg(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame)
{
	int i;
	struct mcs_param *param;
	u32 wdma_addr[MCSC_OUTPUT_MAX][4] = {{0}};
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	struct fimc_is_hw_mcsc *hw_mcsc;
	u32 plane;
	ulong flag;

	BUG_ON(!cap);
	BUG_ON(!hw_ip->priv_info);

	param = &hw_ip->region[frame->instance]->parameter.mcs;
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (frame->type == SHOT_TYPE_INTERNAL)
		goto skip_addr;

	plane = param->output[MCSC_OUTPUT0].plane;
	for (i = 0; i < plane; i++) {
		wdma_addr[MCSC_OUTPUT0][i] =
			frame->shot->uctl.scalerUd.sc0TargetAddress[plane * frame->cur_buf_index + i];
		dbg_hw("M0P(P:%d)(A:0x%X)\n", i, wdma_addr[MCSC_OUTPUT0][i]);
	}

	plane = param->output[MCSC_OUTPUT1].plane;
	for (i = 0; i < plane; i++) {
		wdma_addr[MCSC_OUTPUT1][i] =
			frame->shot->uctl.scalerUd.sc1TargetAddress[plane * frame->cur_buf_index + i];
		dbg_hw("M1P(P:%d)(A:0x%X)\n", i, wdma_addr[MCSC_OUTPUT1][i]);
	}

	plane = param->output[MCSC_OUTPUT2].plane;
	for (i = 0; i < plane; i++) {
		wdma_addr[MCSC_OUTPUT2][i] =
			frame->shot->uctl.scalerUd.sc2TargetAddress[plane * frame->cur_buf_index + i];
		dbg_hw("M2P(P:%d)(A:0x%X)\n", i, wdma_addr[MCSC_OUTPUT2][i]);
	}

	plane = param->output[MCSC_OUTPUT3].plane;
	for (i = 0; i < plane; i++) {
		wdma_addr[MCSC_OUTPUT3][i] =
			frame->shot->uctl.scalerUd.sc3TargetAddress[plane * frame->cur_buf_index + i];
		dbg_hw("M3P(P:%d)(A:0x%X)\n", i, wdma_addr[MCSC_OUTPUT3][i]);
	}

	plane = param->output[MCSC_OUTPUT4].plane;
	for (i = 0; i < plane; i++) {
		wdma_addr[MCSC_OUTPUT4][i] =
			frame->shot->uctl.scalerUd.sc4TargetAddress[plane * frame->cur_buf_index + i];
		dbg_hw("M4P(P:%d)(A:0x%X)\n", i, wdma_addr[MCSC_OUTPUT4][i]);
	}

skip_addr:

	/* DMA out */
	for (i = MCSC_OUTPUT0; i < cap->max_output; i++) {
		if ((cap->out_dma[i] != MCSC_CAP_SUPPORT) || !test_bit(i, &hw_mcsc->out_en))
			continue;

		dbg_hw("[%d][ID:%d]mcsc_wdma_cfg [F:%d][T:%d][addr%d: %x]\n",
			frame->instance, hw_ip->id, frame->fcount, frame->type, i, wdma_addr[i][0]);

		if (param->output[i].dma_cmd != DMA_OUTPUT_COMMAND_DISABLE
			&& wdma_addr[i][0] != 0
			&& frame->type != SHOT_TYPE_INTERNAL) {

			spin_lock_irqsave(&shared_output_slock, flag);
			if (cap->enable_shared_output && test_bit(i, &hw_mcsc_out_configured)
				&& frame->type != SHOT_TYPE_MULTI) {
				warn_hw("[%d][ID:%d][OUT:%d]DMA_OUTPUT in running state[F:%d]\n",
					frame->instance, hw_ip->id, i, frame->fcount);
				spin_unlock_irqrestore(&shared_output_slock, flag);
				return;
			}
			set_bit(i, &hw_mcsc_out_configured);
			spin_unlock_irqrestore(&shared_output_slock, flag);

			dbg_hw("[%d][ID:%d][OUT:%d]dma_out enabled\n",
				frame->instance, hw_ip->id, i);
			fimc_is_scaler_set_dma_out_enable(hw_ip->regs, i, true);

			/* use only one buffer (per-frame) */
			fimc_is_scaler_set_wdma_frame_seq(hw_ip->regs, i,
				0x1 << USE_DMA_BUFFER_INDEX);
			fimc_is_scaler_set_wdma_addr(hw_ip->regs, i,
				wdma_addr[i][0], wdma_addr[i][1], wdma_addr[i][2],
				USE_DMA_BUFFER_INDEX);
		} else {
			u32 wdma_enable = 0;

			wdma_enable = fimc_is_scaler_get_dma_out_enable(hw_ip->regs, i);
			spin_lock_irqsave(&shared_output_slock, flag);
			if (wdma_enable && (cap->enable_shared_output == false || !test_bit(i, &hw_mcsc_out_configured))) {
				fimc_is_scaler_set_dma_out_enable(hw_ip->regs, i, false);
				fimc_is_scaler_clear_wdma_addr(hw_ip->regs, i);
				dbg_hw("[%d][ID:%d][OUT:%d]mcsc_shot: dma_out disabled\n",
						frame->instance, hw_ip->id, i);
			}
			spin_unlock_irqrestore(&shared_output_slock, flag);
			dbg_hw("[%d][ID:%d][OUT:%d]mcsc_wdma_cfg:wmda_enable(%d)[F:%d][T:%d][cmd:%d][addr:0x%x]\n",
				frame->instance, hw_ip->id, i, wdma_enable, frame->fcount, frame->type,
				param->output[i].dma_cmd, wdma_addr[i][0]);
		}
	}
}

int fimc_is_hw_mcsc_shot(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	ulong hw_map)
{
	int ret = 0, ret_smp;
	struct fimc_is_group *head;
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct mcs_param *param;
	bool start_flag = true;
	u32 lindex, hindex, instance;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!frame);
	BUG_ON(!cap);

	hardware = hw_ip->hardware;
	instance = frame->instance;

	dbg_hw("[%d][ID:%d]shot [F:%d]\n", instance, hw_ip->id, frame->fcount);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		dbg_hw("[%d][ID:%d] hw_mcsc not initialized\n", instance, hw_ip->id);
		return -EINVAL;
	}

	if ((!test_bit(ENTRY_M0P, &frame->out_flag))
		&& (!test_bit(ENTRY_M1P, &frame->out_flag))
		&& (!test_bit(ENTRY_M2P, &frame->out_flag))
		&& (!test_bit(ENTRY_M3P, &frame->out_flag))
		&& (!test_bit(ENTRY_M4P, &frame->out_flag)))
		set_bit(hw_ip->id, &frame->core_flag);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	param = &hw_ip->region[instance]->parameter.mcs;

	head = hw_ip->group[frame->instance]->head;

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state)) {
		if (!test_bit(HW_CONFIG, &hw_ip->state)
			&& !atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
			start_flag = true;
		else
			start_flag = false;
	} else {
		start_flag = true;
	}

	ret_smp = down_interruptible(&smp_mcsc_g_enable);

	if (frame->type == SHOT_TYPE_INTERNAL) {
		dbg_hw("[%d][ID:%d] request not exist\n", instance, hw_ip->id);
		goto config;
	}

	lindex = frame->shot->ctl.vendor_entry.lowIndexParam;
	hindex = frame->shot->ctl.vendor_entry.highIndexParam;

	fimc_is_hw_mcsc_update_param(hw_ip, param,
		lindex, hindex, instance);

	dbg_hw("[%d]mcsc_shot [F:%d][T:%d]\n", instance, frame->fcount, frame->type);

config:
	/* multi-buffer */
	hw_ip->cur_s_int = 0;
	hw_ip->cur_e_int = 0;
	if (frame->num_buffers)
		hw_ip->num_buffers = frame->num_buffers;
	if (frame->num_buffers > 1)
		hw_ip->mframe = frame;

	/* RDMA cfg */
	if (param->input.dma_cmd == DMA_INPUT_COMMAND_ENABLE
		&& cap->in_dma == MCSC_CAP_SUPPORT) {
		ret = fimc_is_hw_mcsc_rdma_cfg(hw_ip, frame);
		if (ret) {
			err_hw("[%d][ID:%d][F:%d]mcsc rdma_cfg failed\n",
				instance, hw_ip->id, frame->fcount);
			return ret;
		}
	}

	/* WDMA cfg */
	fimc_is_hw_mcsc_wdma_cfg(hw_ip, frame);

	if (start_flag) {
		dbg_hw("[%d][ID:%d]mcsc_start [F:%d]\n", instance, hw_ip->id, frame->fcount);
		fimc_is_scaler_start(hw_ip->regs, hw_ip->id);
	}

	dbg_hw("[%d][ID:%d]mcsc_shot: hw_mcsc_out_configured[0x%lx]\n", instance, hw_ip->id,
		hw_mcsc_out_configured);

	clear_bit(HW_MCSC_OUT_CLEARED_ALL, &hw_mcsc_out_configured);
	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

int fimc_is_hw_mcsc_set_param(struct fimc_is_hw_ip *hw_ip, struct is_region *region,
	u32 lindex, u32 hindex, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;

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

	/* To execute update_register function in hw_mcsc_shot(),
	 * the value of hw_mcsc->instance is set as default.
	 */
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	hw_mcsc->instance = FIMC_IS_STREAM_COUNT;

	return ret;
}

void fimc_is_hw_mcsc_check_size(struct fimc_is_hw_ip *hw_ip, struct mcs_param *param,
	u32 instance, u32 output_id)
{
	struct param_mcs_input *input = &param->input;
	struct param_mcs_output *output = &param->output[output_id];

	info_hw("[%d][OUT:%d]>>> hw_mcsc_check_size >>>\n", instance, output_id);
	info_hw("otf_input: format(%d),size(%dx%d)\n",
		input->otf_format, input->width, input->height);
	info_hw("dma_input: format(%d),crop_size(%dx%d)\n",
		input->dma_format, input->dma_crop_width, input->dma_crop_height);
	info_hw("output: otf_cmd(%d),dma_cmd(%d),format(%d),stride(y:%d,c:%d)\n",
		output->otf_cmd, output->dma_cmd, output->dma_format,
		output->dma_stride_y, output->dma_stride_c);
	info_hw("output: pos(%d,%d),crop%dx%d),size(%dx%d)\n",
		output->crop_offset_x, output->crop_offset_y,
		output->crop_width, output->crop_height,
		output->width, output->height);
	info_hw("[%d]<<< hw_mcsc_check_size <<<\n", instance);
}

int fimc_is_hw_mcsc_update_register(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *param, u32 output_id, u32 instance)
{
	int ret = 0;

	hw_mcsc_check_size(hw_ip, param, instance, output_id);
	ret = fimc_is_hw_mcsc_poly_phase(hw_ip, &param->input,
			&param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_post_chain(hw_ip, &param->input,
			&param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_flip(hw_ip, &param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_otf_output(hw_ip, &param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_dma_output(hw_ip, &param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_output_yuvrange(hw_ip, &param->output[output_id], output_id, instance);
	ret = fimc_is_hw_mcsc_hwfc_output(hw_ip, &param->output[output_id], output_id, instance);

	return ret;
}

int fimc_is_hw_mcsc_update_param(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *param, u32 lindex, u32 hindex, u32 instance)
{
	int i = 0;
	int ret = 0;
	bool control_cmd = false;
	struct fimc_is_hw_mcsc *hw_mcsc;
	u32 hwfc_output_ids = 0;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!param);
	BUG_ON(!cap);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (hw_mcsc->instance != instance) {
		control_cmd = true;
		dbg_hw("[%d][ID:%d]hw_mcsc_update_param: hw_ip->instance(%d), control_cmd(%d)\n",
			instance, hw_ip->id, hw_mcsc->instance, control_cmd);
		hw_mcsc->instance = instance;
	}

	if (control_cmd || (lindex & LOWBIT_OF(PARAM_MCS_INPUT))
		|| (hindex & HIGHBIT_OF(PARAM_MCS_INPUT))
		|| (test_bit(HW_MCSC_OUT_CLEARED_ALL, &hw_mcsc_out_configured))) {
		ret = fimc_is_hw_mcsc_otf_input(hw_ip, &param->input, instance);
		ret = fimc_is_hw_mcsc_dma_input(hw_ip, &param->input, instance);
	}

	for (i = MCSC_OUTPUT0; i < cap->max_output; i++) {
		if (control_cmd || (lindex & LOWBIT_OF((i + PARAM_MCS_OUTPUT0)))
				|| (hindex & HIGHBIT_OF((i + PARAM_MCS_OUTPUT0)))
				|| (test_bit(HW_MCSC_OUT_CLEARED_ALL, &hw_mcsc_out_configured))) {
			ret = fimc_is_hw_mcsc_update_register(hw_ip, param, i, instance);
			fimc_is_scaler_set_wdma_pri(hw_ip->regs, i, param->output[i].plane);	/* FIXME: */

		}
		/* check the hwfc enable in all output */
		if (param->output[i].hwfc)
			hwfc_output_ids |= (1 << i);
	}

	/* setting for hwfc */
	if (hwfc_output_ids)
		ret = fimc_is_hw_mcsc_hwfc_mode(hw_ip, &param->input, hwfc_output_ids,
				instance);

	if (ret)
		fimc_is_hw_mcsc_size_dump(hw_ip);

	return ret;
}

int fimc_is_hw_mcsc_reset(struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0, hw_slot = -1;
	u32 output_id;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	struct fimc_is_hardware *hardware;
	struct fimc_is_hw_ip *hw_ip0 = NULL, *hw_ip1 = NULL;

	BUG_ON(!hw_ip);
	BUG_ON(!cap);

	hardware = hw_ip->hardware;
	hw_slot = fimc_is_hw_slot_id(DEV_HW_MCSC0);
	if (valid_hw_slot_id(hw_slot))
		hw_ip0 = &hardware->hw_ip[hw_slot];

	hw_slot = fimc_is_hw_slot_id(DEV_HW_MCSC1);
	if (valid_hw_slot_id(hw_slot))
		hw_ip1 = &hardware->hw_ip[hw_slot];

	if (cap->enable_shared_output) {
		if (hw_ip0 && test_bit(HW_RUN, &hw_ip0->state))
			return 0;

		if (hw_ip1 && test_bit(HW_RUN, &hw_ip1->state))
			return 0;

		info_hw("[ID:%d] %s: sema_init (g_enable, 1)\n", hw_ip->id, __func__);
		sema_init(&smp_mcsc_g_enable, 1);
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (hw_ip0) {
		info_hw("[ID:%d]hw_mcsc_reset: out_en[0x%lx]\n", hw_ip0->id, hw_mcsc->out_en);
		ret = fimc_is_scaler_sw_reset(hw_ip0->regs, hw_ip0->id, 0, 0);
		if (ret != 0) {
			err_hw("MCSC sw reset fail");
			return -ENODEV;
		}
	}

	if (hw_ip1) {
		info_hw("[ID:%d]hw_mcsc_reset: out_en[0x%lx]\n", hw_ip1->id, hw_mcsc->out_en);
		ret = fimc_is_scaler_sw_reset(hw_ip1->regs, hw_ip1->id, 0, 0);
		if (ret != 0) {
			err_hw("MCSC sw reset fail");
			return -ENODEV;
		}
	}

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		if (cap->enable_shared_output == false || test_bit(output_id, &hw_mcsc_out_configured)) {
			info_hw("[ID:%d][OUT:%d]set output clear\n", hw_ip->id, output_id);
			fimc_is_scaler_set_poly_scaler_enable(hw_ip->regs, hw_ip->id, output_id, 0);
			fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, output_id, 0);
			fimc_is_scaler_set_otf_out_enable(hw_ip->regs, output_id, false);
			fimc_is_scaler_set_dma_out_enable(hw_ip->regs, output_id, false);

			fimc_is_scaler_set_wdma_pri(hw_ip->regs, output_id, 0);	/* FIXME: */
			fimc_is_scaler_set_wdma_axi_pri(hw_ip->regs);		/* FIXME: */
			fimc_is_scaler_set_wdma_sram_base(hw_ip->regs, output_id);
			clear_bit(output_id, &hw_mcsc_out_configured);
		}
	}
	fimc_is_scaler_set_wdma_sram_base(hw_ip->regs, MCSC_OUTPUT_SSB);	/* FIXME: */

	set_bit(HW_MCSC_OUT_CLEARED_ALL, &hw_mcsc_out_configured);

	if (cap->in_otf == MCSC_CAP_SUPPORT) {
		if (hw_ip0)
			fimc_is_scaler_set_stop_req_post_en_ctrl(hw_ip0->regs, hw_ip0->id, 0);

		if (hw_ip1)
			fimc_is_scaler_set_stop_req_post_en_ctrl(hw_ip1->regs, hw_ip1->id, 0);
	}

	return ret;
}

int fimc_is_hw_mcsc_clear_interrupt(struct fimc_is_hw_ip *hw_ip)
{
	int ret = 0;

	BUG_ON(!hw_ip);

	info_hw("[ID:%d]hw_mcsc_clear_interrupt\n", hw_ip->id);

	fimc_is_scaler_clear_intr_all(hw_ip->regs, hw_ip->id);
	fimc_is_scaler_disable_intr(hw_ip->regs, hw_ip->id);
	fimc_is_scaler_mask_intr(hw_ip->regs, hw_ip->id, MCSC_INTR_MASK);

	return ret;
}

int fimc_is_hw_mcsc_load_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;
	u32 index;

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
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
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
	}

	for (index = 0; index < setfile->using_count; index++) {
		hw_mcsc->setfile = (struct hw_api_scaler_setfile *)setfile->table[index].addr;
		if (hw_mcsc->setfile->setfile_version != MCSC_SETFILE_VERSION) {
			err_hw("[%d][ID:%d] setfile version(0x%x) is incorrect",
				instance, hw_ip->id, hw_mcsc->setfile->setfile_version);
			return -EINVAL;
		}
	}

	set_bit(HW_TUNESET, &hw_ip->state);

	return ret;
}

int fimc_is_hw_mcsc_apply_setfile(struct fimc_is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	struct fimc_is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;
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
		err_hw("MCSC priv info is NULL");
		return -EINVAL;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	if (!hw_mcsc->setfile)
		return 0;

	setfile_index = setfile->index[scenario];
	if (setfile_index >= setfile->using_count) {
		err_hw("[%d][ID:%d] setfile index is out-of-range, [%d:%d]",
				instance, hw_ip->id, scenario, setfile_index);
		return -EINVAL;
	}

	info_hw("[%d][ID:%d] setfile (%d) scenario (%d)\n", instance, hw_ip->id,
		setfile_index, scenario);

	return ret;
}

int fimc_is_hw_mcsc_delete_setfile(struct fimc_is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;

	BUG_ON(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		dbg_hw("[%d]%s: hw_map(0x%lx)\n", instance, __func__, hw_map);
		return 0;
	}

	if (test_bit(HW_TUNESET, &hw_ip->state)) {
		dbg_hw("[%d][ID:%d] setfile already deleted", instance, hw_ip->id);
		return 0;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	hw_mcsc->setfile = NULL;
	clear_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

void fimc_is_hw_mcsc_frame_done(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	int done_type)
{
	int ret = 0;
	bool fdone_flag = false;
	struct fimc_is_frame *done_frame;
	struct fimc_is_framemgr *framemgr;
	u32 index;
	ulong flags = 0;

	switch (done_type) {
	case IS_SHOT_SUCCESS:
		framemgr = hw_ip->framemgr;
		framemgr_e_barrier_common(framemgr, 0, flags);
		done_frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
		framemgr_x_barrier_common(framemgr, 0, flags);
		if (done_frame == NULL) {
			err_hw("[ID:%d][MCSC][F:%d] frame(null) @FS_HW_WAIT_DONE!!",
				hw_ip->id, atomic_read(&hw_ip->fcount));
			framemgr_e_barrier_common(framemgr, 0, flags);
			print_all_hw_frame_count(hw_ip->hardware);
			framemgr_x_barrier_common(framemgr, 0, flags);
			return;
		}
		break;
	case IS_SHOT_UNPROCESSED:
	case IS_SHOT_LATE_FRAME:
	case IS_SHOT_OVERFLOW:
	case IS_SHOT_INVALID_FRAMENUMBER:
	case IS_SHOT_TIMEOUT:
		done_frame = frame;
		if (done_frame == NULL) {
			err_hw("[ID:%d][MCSC][F:%d] frame(null) type(%d)",
				hw_ip->id, atomic_read(&hw_ip->fcount), done_type);
			return;
		}
		break;
	default:
		err_hw("[%d][F:%d] invalid done type(%d)\n", atomic_read(&hw_ip->instance),
			atomic_read(&hw_ip->fcount), done_type);
		return;
	}

	if (test_bit(ENTRY_M0P, &done_frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_M0P_FDONE, ENTRY_M0P, done_type, false);
		fdone_flag = true;
		clear_bit(MCSC_OUTPUT0, &hw_mcsc_out_configured);
		dbg_hw("[OUT:0] cleared[F:%d]\n", done_frame->fcount);
	}

	if (test_bit(ENTRY_M1P, &done_frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_M1P_FDONE, ENTRY_M1P, done_type, false);
		fdone_flag = true;
		clear_bit(MCSC_OUTPUT1, &hw_mcsc_out_configured);
		dbg_hw("[OUT:1] cleared[F:%d]\n", done_frame->fcount);
	}

	if (test_bit(ENTRY_M2P, &done_frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_M2P_FDONE, ENTRY_M2P, done_type, false);
		fdone_flag = true;
		clear_bit(MCSC_OUTPUT2, &hw_mcsc_out_configured);
		dbg_hw("[OUT:2] cleared[F:%d]\n", done_frame->fcount);
	}

	if (test_bit(ENTRY_M3P, &done_frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_M3P_FDONE, ENTRY_M3P, done_type, false);
		fdone_flag = true;
		clear_bit(MCSC_OUTPUT3, &hw_mcsc_out_configured);
		dbg_hw("[OUT:3] cleared[F:%d]\n", done_frame->fcount);
	}

	if (test_bit(ENTRY_M4P, &done_frame->out_flag)) {
		ret = fimc_is_hardware_frame_done(hw_ip, frame,
			WORK_M4P_FDONE, ENTRY_M4P, done_type, false);
		fdone_flag = true;
		clear_bit(MCSC_OUTPUT4, &hw_mcsc_out_configured);
		dbg_hw("[OUT:4] cleared[F:%d]\n", done_frame->fcount);
	}

	if (fdone_flag) {
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_DMA_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_DMA_END] = cpu_clock(raw_smp_processor_id());
		atomic_inc(&hw_ip->count.dma);
	} else {
		index = hw_ip->debug_index[1];
		hw_ip->debug_info[index].cpuid[DEBUG_POINT_FRAME_END] = raw_smp_processor_id();
		hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_END] = cpu_clock(raw_smp_processor_id());

		fimc_is_hardware_frame_done(hw_ip, NULL, -1, FIMC_IS_HW_CORE_END,
				IS_SHOT_SUCCESS, false);
	}

	return;
}

int fimc_is_hw_mcsc_frame_ndone(struct fimc_is_hw_ip *hw_ip, struct fimc_is_frame *frame,
	u32 instance, enum ShotErrorType done_type)
{
	int ret = 0;

	fimc_is_hw_mcsc_frame_done(hw_ip, frame, done_type);

	if (test_bit_variables(hw_ip->id, &frame->core_flag))
		ret = fimc_is_hardware_frame_done(hw_ip, frame, -1, FIMC_IS_HW_CORE_END,
				done_type, false);

	if (test_bit(HW_CONFIG, &hw_ip->state))
		up(&smp_mcsc_g_enable);

	return ret;
}

int fimc_is_hw_mcsc_otf_input(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance)
{
	int ret = 0;
	u32 width, height;
	u32 format, bit_width;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!input);
	BUG_ON(!cap);

	/* can't support this function */
	if (cap->in_otf != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	dbg_hw("[%d][ID:%d]hw_mcsc_otf_input_setting cmd(%d)\n", instance, hw_ip->id, input->otf_cmd);
	width  = input->width;
	height = input->height;
	format = input->otf_format;
	bit_width = input->otf_bitwidth;

	if (input->otf_cmd == OTF_INPUT_COMMAND_DISABLE)
		return ret;

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_OTF_INPUT,
		format, bit_width, width, height);
	if (ret) {
		err_hw("[%d]Invalid MCSC OTF Input format: fmt(%d),bit(%d),size(%dx%d)",
			instance, format, bit_width, width, height);
		return ret;
	}

	fimc_is_scaler_set_input_img_size(hw_ip->regs, hw_ip->id, width, height);
	fimc_is_scaler_set_dither(hw_ip->regs, hw_ip->id, 0);

	return ret;
}

int fimc_is_hw_mcsc_dma_input(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance)
{
	int ret = 0;
	u32 width, height;
	u32 format, bit_width, plane, order;
	u32 y_stride, uv_stride;
	u32 img_format;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!input);
	BUG_ON(!cap);

	/* can't support this function */
	if (cap->in_dma != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	dbg_hw("[%d][ID:%d]hw_mcsc_dma_input_setting cmd(%d)\n", instance, hw_ip->id, input->dma_cmd);
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

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_DMA_INPUT,
		format, bit_width, width, height);
	if (ret) {
		err_hw("[%d]Invalid MCSC DMA Input format: fmt(%d),bit(%d),size(%dx%d)",
			instance, format, bit_width, width, height);
		return ret;
	}

	fimc_is_scaler_set_input_img_size(hw_ip->regs, hw_ip->id, width, height);
	fimc_is_scaler_set_dither(hw_ip->regs, hw_ip->id, 0);

	fimc_is_scaler_set_rdma_stride(hw_ip->regs, y_stride, uv_stride);

	ret = fimc_is_hw_mcsc_adjust_input_img_fmt(format, plane, order, &img_format);
	if (ret < 0) {
		warn_hw("[%d][ID:%d] Invalid rdma image format\n", instance, hw_ip->id);
		img_format = hw_mcsc->in_img_format;
	} else {
		hw_mcsc->in_img_format = img_format;
	}

	fimc_is_scaler_set_rdma_size(hw_ip->regs, width, height);
	fimc_is_scaler_set_rdma_format(hw_ip->regs, img_format);

	return ret;
}


int fimc_is_hw_mcsc_poly_phase(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, u32 output_id, u32 instance)
{
	int ret = 0;
	u32 src_pos_x, src_pos_y, src_width, src_height;
	u32 poly_dst_width, poly_dst_height;
	u32 out_width, out_height;
	ulong temp_width, temp_height;
	u32 hratio, vratio;
	u32 input_id = 0;
	bool config = true;
	bool post_en = false;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!input);
	BUG_ON(!output);
	BUG_ON(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	dbg_hw("[%d][OUT:%d]hw_mcsc_poly_phase_setting cmd(O:%d,D:%d)\n",
		instance, output_id, output->otf_cmd, output->dma_cmd);

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->otf_cmd == OTF_OUTPUT_COMMAND_DISABLE
		&& output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &hw_mcsc_out_configured)))
			fimc_is_scaler_set_poly_scaler_enable(hw_ip->regs, hw_ip->id, output_id, 0);
		return ret;
	}

	fimc_is_scaler_set_poly_scaler_enable(hw_ip->regs, hw_ip->id, output_id, 1);

	src_pos_x = output->crop_offset_x;
	src_pos_y = output->crop_offset_y;
	src_width = output->crop_width;
	src_height = output->crop_height;

	out_width = output->width;
	out_height = output->height;

	fimc_is_scaler_set_poly_src_size(hw_ip->regs, output_id, src_pos_x, src_pos_y,
		src_width, src_height);

	if ((src_width <= (out_width * MCSC_POLY_RATIO_DOWN))
		&& (out_width <= (src_width * MCSC_POLY_RATIO_UP))) {
		poly_dst_width = out_width;
		post_en = false;
	} else if ((src_width <= (out_width * MCSC_POLY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((out_width * MCSC_POLY_RATIO_DOWN) < src_width)) {
		poly_dst_width = MCSC_ROUND_UP(src_width / MCSC_POLY_RATIO_DOWN, 2);
		post_en = true;
	} else {
		err_hw("[%d]hw_mcsc_poly_phase: Unsupported H ratio, (%dx%d)->(%dx%d)\n",
			instance, src_width, src_height, out_width, out_height);
		poly_dst_width = MCSC_ROUND_UP(src_width / MCSC_POLY_RATIO_DOWN, 2);
		post_en = true;
	}

	if ((src_height <= (out_height * MCSC_POLY_RATIO_DOWN))
		&& (out_height <= (src_height * MCSC_POLY_RATIO_UP))) {
		poly_dst_height = out_height;
		post_en = false;
	} else if ((src_height <= (out_height * MCSC_POLY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((out_height * MCSC_POLY_RATIO_DOWN) < src_height)) {
		poly_dst_height = (src_height / MCSC_POLY_RATIO_DOWN);
		post_en = true;
	} else {
		err_hw("[%d]hw_mcsc_poly_phase: Unsupported H ratio, (%dx%d)->(%dx%d)\n",
			instance, src_width, src_height, out_width, out_height);
		poly_dst_height = (src_height / MCSC_POLY_RATIO_DOWN);
		post_en = true;
	}
#if defined(MCSC_POST_WA)
	/* The post scaler guarantee the quality of image          */
	/*  in case the scaling ratio equals to multiple of x1/256 */
	if ((post_en && ((poly_dst_width << MCSC_POST_WA_SHIFT) % out_width))
		|| (post_en && ((poly_dst_height << MCSC_POST_WA_SHIFT) % out_height))) {
		u32 multiple_hratio = 1;
		u32 multiple_vratio = 1;
		u32 shift = 0;
		for (shift = 0; shift <= MCSC_POST_WA_SHIFT; shift++) {
			if (((out_width % (1 << (MCSC_POST_WA_SHIFT-shift))) == 0)
				&& (out_height % (1 << (MCSC_POST_WA_SHIFT-shift)) == 0)) {
				multiple_hratio = out_width  / (1 << (MCSC_POST_WA_SHIFT-shift));
				multiple_vratio = out_height / (1 << (MCSC_POST_WA_SHIFT-shift));
				break;
			}
		}
		dbg_hw("[%d][OUT:%d]hw_mcsc_poly_phase: shift(%d), ratio(%d,%d), "
			"size(%dx%d) before calculation\n",
			instance, output_id, shift, multiple_hratio, multiple_hratio,
			poly_dst_width, poly_dst_height);
		poly_dst_width  = MCSC_ROUND_UP(poly_dst_width, multiple_hratio);
		poly_dst_height = MCSC_ROUND_UP(poly_dst_height, multiple_vratio);
		if (poly_dst_width % 2) {
			poly_dst_width  = poly_dst_width  + multiple_hratio;
			poly_dst_height = poly_dst_height + multiple_vratio;
		}
		dbg_hw("[%d][OUT:%d]hw_mcsc_poly_phase: size(%dx%d) after  calculation\n",
			instance, output_id, poly_dst_width, poly_dst_height);
	}
#endif

	fimc_is_scaler_set_poly_dst_size(hw_ip->regs, output_id,
		poly_dst_width, poly_dst_height);

	temp_width  = (ulong)src_width;
	temp_height = (ulong)src_height;
	hratio = (u32)((temp_width  << MCSC_PRECISION) / poly_dst_width);
	vratio = (u32)((temp_height << MCSC_PRECISION) / poly_dst_height);

	fimc_is_scaler_set_poly_scaling_ratio(hw_ip->regs, output_id, hratio, vratio);
	fimc_is_scaler_set_poly_scaler_coef(hw_ip->regs, output_id, hratio, vratio);

	return ret;
}

int fimc_is_hw_mcsc_post_chain(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, u32 output_id, u32 instance)
{
	int ret = 0;
	u32 img_width, img_height;
	u32 dst_width, dst_height;
	ulong temp_width, temp_height;
	u32 hratio, vratio;
	u32 input_id = 0;
	bool config = true;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!input);
	BUG_ON(!output);
	BUG_ON(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	dbg_hw("[%d][OUT:%d]hw_mcsc_post_chain_setting cmd(O:%d,D:%d)\n",
		instance, output_id, output->otf_cmd, output->dma_cmd);

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->otf_cmd == OTF_OUTPUT_COMMAND_DISABLE
		&& output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &hw_mcsc_out_configured)))
			fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, output_id, 0);
		return ret;
	}

	fimc_is_scaler_get_poly_dst_size(hw_ip->regs, output_id, &img_width, &img_height);

	dst_width = output->width;
	dst_height = output->height;

	/* if x1 ~ x1/4 scaling, post scaler bypassed */
	if ((img_width == dst_width) && (img_height == dst_height)) {
		fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, output_id, 0);
	} else {
		fimc_is_scaler_set_post_scaler_enable(hw_ip->regs, output_id, 1);
	}

	fimc_is_scaler_set_post_img_size(hw_ip->regs, output_id, img_width, img_height);
	fimc_is_scaler_set_post_dst_size(hw_ip->regs, output_id, dst_width, dst_height);

	temp_width  = (ulong)img_width;
	temp_height = (ulong)img_height;
	hratio = (u32)((temp_width  << MCSC_PRECISION) / dst_width);
	vratio = (u32)((temp_height << MCSC_PRECISION) / dst_height);

	fimc_is_scaler_set_post_scaling_ratio(hw_ip->regs, output_id, hratio, vratio);

	return ret;
}

int fimc_is_hw_mcsc_flip(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;

	BUG_ON(!hw_ip);
	BUG_ON(!output);
	BUG_ON(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	dbg_hw("[%d][OUT:%d]hw_mcsc_flip_setting flip(%d),cmd(O:%d,D:%d)\n",
		instance, output_id, output->flip, output->otf_cmd, output->dma_cmd);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE)
		return ret;

	fimc_is_scaler_set_flip_mode(hw_ip->regs, output_id, output->flip);

	return ret;
}

int fimc_is_hw_mcsc_otf_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	u32 out_width, out_height;
	u32 format, bitwidth;
	u32 input_id = 0;
	bool config = true;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!output);
	BUG_ON(!cap);
	BUG_ON(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->out_otf[output_id] != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	dbg_hw("[%d][OUT:%d]hw_mcsc_otf_output_setting cmd(O:%d,D:%d)\n",
		instance, output_id, output->otf_cmd, output->dma_cmd);

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->otf_cmd == OTF_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &hw_mcsc_out_configured)))
			fimc_is_scaler_set_otf_out_enable(hw_ip->regs, output_id, false);
		return ret;
	}

	out_width  = output->width;
	out_height = output->height;
	format     = output->otf_format;
	bitwidth  = output->otf_bitwidth;

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_OTF_OUTPUT,
		format, bitwidth, out_width, out_height);
	if (ret) {
		err_hw("[%d][OUT:%d]Invalid MCSC OTF Output format: fmt(%d),bit(%d),size(%dx%d)",
			instance, output_id, format, bitwidth, out_width, out_height);
		return ret;
	}

	fimc_is_scaler_set_otf_out_enable(hw_ip->regs, output_id, true);
	fimc_is_scaler_set_otf_out_path(hw_ip->regs, output_id);

	return ret;
}

int fimc_is_hw_mcsc_dma_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	u32 out_width, out_height, scaled_width, scaled_height;
	u32 format, plane, order,bitwidth;
	u32 y_stride, uv_stride;
	u32 img_format;
	u32 input_id = 0;
	bool config = true;
	bool conv420_en = false;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!output);
	BUG_ON(!cap);
	BUG_ON(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->out_dma[output_id] != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	dbg_hw("[%d][OUT:%d]hw_mcsc_dma_output_setting cmd(O:%d,D:%d)\n",
		instance, output_id, output->otf_cmd, output->dma_cmd);

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &hw_mcsc_out_configured)))
			fimc_is_scaler_set_dma_out_enable(hw_ip->regs, output_id, false);
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

	ret = fimc_is_hw_mcsc_check_format(HW_MCSC_DMA_OUTPUT,
		format, bitwidth, out_width, out_height);
	if (ret) {
		err_hw("[%d][OUT:%d]Invalid MCSC DMA Output format: fmt(%d),bit(%d),size(%dx%d)",
			instance, output_id, format, bitwidth, out_width, out_height);
		return ret;
	}

	ret = fimc_is_hw_mcsc_adjust_output_img_fmt(format, plane, order,
			&img_format, &conv420_en);
	if (ret < 0) {
		warn_hw("[%d][ID:%d] Invalid dma image format\n", instance, hw_ip->id);
		img_format = hw_mcsc->out_img_format[output_id];
		conv420_en = hw_mcsc->conv420_en[output_id];
	} else {
		hw_mcsc->out_img_format[output_id] = img_format;
		hw_mcsc->conv420_en[output_id] = conv420_en;
	}

	fimc_is_scaler_set_wdma_format(hw_ip->regs, output_id, img_format);
	fimc_is_scaler_set_420_conversion(hw_ip->regs, output_id, 0, conv420_en);

	fimc_is_scaler_get_post_dst_size(hw_ip->regs, output_id, &scaled_width, &scaled_height);
	if ((scaled_width != out_width) || (scaled_height != out_height)) {
		dbg_hw("[%d][ID:%d] Invalid output[%d] scaled size (%d/%d)(%d/%d)\n",
			instance, hw_ip->id, output_id, scaled_width, scaled_height,
			out_width, out_height);
		return -EINVAL;
	}
	fimc_is_scaler_set_wdma_size(hw_ip->regs, output_id, out_width, out_height);

	fimc_is_scaler_set_wdma_stride(hw_ip->regs, output_id, y_stride, uv_stride);

	return ret;
}

int fimc_is_hw_mcsc_hwfc_mode(struct fimc_is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 hwfc_output_ids, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!input);
	BUG_ON(!cap);
	BUG_ON(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->hwfc != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	dbg_hw("[%d]hw_mcsc_hwfc_mode_setting output[0x%08X]\n", instance, hwfc_output_ids);

	fimc_is_scaler_set_hwfc_mode(hw_ip->regs, hwfc_output_ids);

	return ret;
}

int fimc_is_hw_mcsc_hwfc_output(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	u32 width, height, format, plane;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!output);
	BUG_ON(!cap);
	BUG_ON(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->out_hwfc[output_id] != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	width  = output->width;
	height = output->height;
	format = output->dma_format;
	plane = output->plane;

	dbg_hw("[%d]hw_mcsc_hwfc_config_setting mode(%dx%d, %d, %d)\n", instance,
			width, height, format, plane);

	if (output->hwfc)
		fimc_is_scaler_set_hwfc_config(hw_ip->regs, output_id, format, plane,
			(output_id * 3), width, height);

	return ret;
}


int fimc_is_hw_mcsc_output_yuvrange(struct fimc_is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	int yuv_range = 0;
	u32 input_id = 0;
	bool config = true;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
#if !defined(USE_YUV_RANGE_BY_ISP)
	scaler_setfile_contents contents;
#endif
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	BUG_ON(!hw_ip);
	BUG_ON(!output);
	BUG_ON(!hw_ip->priv_info);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	input_id = fimc_is_scaler_get_scaler_path(hw_ip->regs, hw_ip->id, output_id);
	config = (input_id == hw_ip->id ? true: false);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		if (cap->enable_shared_output == false
			|| (config && !test_bit(output_id, &hw_mcsc_out_configured)))
			fimc_is_scaler_set_bchs_enable(hw_ip->regs, output_id, 0);
		return ret;
	}

	yuv_range = output->yuv_range;
	hw_mcsc->yuv_range = yuv_range; /* save for ISP */

	fimc_is_scaler_set_bchs_enable(hw_ip->regs, output_id, 1);
#if !defined(USE_YUV_RANGE_BY_ISP)
	if (test_bit(HW_TUNESET, &hw_ip->state)) {
		/* set yuv range config value by scaler_param yuv_range mode */
		contents = hw_mcsc->setfile->contents[yuv_range];
		fimc_is_scaler_set_b_c(hw_ip->regs, output_id,
			contents.y_offset, contents.y_gain);
		fimc_is_scaler_set_h_s(hw_ip->regs, output_id,
			contents.c_gain00, contents.c_gain01,
			contents.c_gain10, contents.c_gain11);
		dbg_hw("[%d][ID:%d] set YUV range(%d) by setfile parameter\n",
			instance, hw_ip->id, yuv_range);
		dbg_hw("[%d][OUT:%d]hw_mcsc_output_yuv_setting: yuv_range(%d), cmd(O:%d,D:%d)\n",
			instance, output_id, yuv_range, output->otf_cmd, output->dma_cmd);
		dbg_hw("[Y:offset(%d),gain(%d)][C:gain00(%d),01(%d),10(%d),11(%d)]\n",
			contents.y_offset, contents.y_gain,
			contents.c_gain00, contents.c_gain01,
			contents.c_gain10, contents.c_gain11);
	} else {
		if (yuv_range == SCALER_OUTPUT_YUV_RANGE_FULL) {
			/* Y range - [0:255], U/V range - [0:255] */
			fimc_is_scaler_set_b_c(hw_ip->regs, output_id, 0, 256);
			fimc_is_scaler_set_h_s(hw_ip->regs, output_id, 256, 0, 0, 256);
		} else if (yuv_range == SCALER_OUTPUT_YUV_RANGE_NARROW) {
			/* Y range - [16:235], U/V range - [16:239] */
			fimc_is_scaler_set_b_c(hw_ip->regs, output_id, 16, 220);
			fimc_is_scaler_set_h_s(hw_ip->regs, output_id, 224, 0, 0, 224);
		}
		dbg_hw("[%d][ID:%d] YUV range set default settings\n", instance,
			hw_ip->id);
	}
#else
	/* SCALER_OUTPUT_YUV_RANGE_FULL */
	/* Y range - [0:255], U/V range - [0:255] */
	fimc_is_scaler_set_b_c(hw_ip->regs, output_id, 0, 256);
	fimc_is_scaler_set_h_s(hw_ip->regs, output_id, 256, 0, 0, 256);
	if (yuv_range == SCALER_OUTPUT_YUV_RANGE_NARROW)
		fimc_is_scaler_set_bchs_clamp(hw_ip->regs, output_id, 235, 16, 240, 16);
	else
		fimc_is_scaler_set_bchs_clamp(hw_ip->regs, output_id, 255, 0, 255, 0);
#endif
	return ret;
}

int fimc_is_hw_mcsc_adjust_input_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format)
{
	int ret = 0;

	switch (format) {
	case DMA_INPUT_FORMAT_YUV420:
		switch (plane) {
		case 2:
			switch (order) {
			case DMA_INPUT_ORDER_CbCr:
				*img_format = MCSC_YUV420_2P_UFIRST;
				break;
			case DMA_INPUT_ORDER_CrCb:
				* img_format = MCSC_YUV420_2P_VFIRST;
				break;
			default:
				err_hw("input order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV420_3P;
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
				*img_format = MCSC_YUV422_1P_VYUY;
				break;
			case DMA_INPUT_ORDER_CbYCrY:
				*img_format = MCSC_YUV422_1P_UYVY;
				break;
			case DMA_INPUT_ORDER_YCrYCb:
				*img_format = MCSC_YUV422_1P_YVYU;
				break;
			case DMA_INPUT_ORDER_YCbYCr:
				*img_format = MCSC_YUV422_1P_YUYV;
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
				*img_format = MCSC_YUV422_2P_UFIRST;
				break;
			case DMA_INPUT_ORDER_CrCb:
				*img_format = MCSC_YUV422_2P_VFIRST;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV422_3P;
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


int fimc_is_hw_mcsc_adjust_output_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format,
	bool *conv420_flag)
{
	int ret = 0;

	switch (format) {
	case DMA_OUTPUT_FORMAT_YUV420:
		if (conv420_flag)
			*conv420_flag = true;
		switch (plane) {
		case 2:
			switch (order) {
			case DMA_OUTPUT_ORDER_CbCr:
				*img_format = MCSC_YUV420_2P_UFIRST;
				break;
			case DMA_OUTPUT_ORDER_CrCb:
				* img_format = MCSC_YUV420_2P_VFIRST;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV420_3P;
			break;
		default:
			err_hw("output plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	case DMA_OUTPUT_FORMAT_YUV422:
		if (conv420_flag)
			*conv420_flag = false;
		switch (plane) {
		case 1:
			switch (order) {
			case DMA_OUTPUT_ORDER_CrYCbY:
				*img_format = MCSC_YUV422_1P_VYUY;
				break;
			case DMA_OUTPUT_ORDER_CbYCrY:
				*img_format = MCSC_YUV422_1P_UYVY;
				break;
			case DMA_OUTPUT_ORDER_YCrYCb:
				*img_format = MCSC_YUV422_1P_YVYU;
				break;
			case DMA_OUTPUT_ORDER_YCbYCr:
				*img_format = MCSC_YUV422_1P_YUYV;
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
				*img_format = MCSC_YUV422_2P_UFIRST;
				break;
			case DMA_OUTPUT_ORDER_CrCb:
				*img_format = MCSC_YUV422_2P_VFIRST;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV422_3P;
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

int fimc_is_hw_mcsc_check_format(enum mcsc_io_type type, u32 format, u32 bit_width,
	u32 width, u32 height)
{
	int ret = 0;

	switch (type) {
	case HW_MCSC_OTF_INPUT:
		/* check otf input */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input height(%d)", height);
		}

		if (format != OTF_INPUT_FORMAT_YUV422) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input format(%d)", format);
		}

		if (bit_width != OTF_INPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input format(%d)", bit_width);
		}
		break;
	case HW_MCSC_OTF_OUTPUT:
		/* check otf output */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output height(%d)", height);
		}

		if (format != OTF_OUTPUT_FORMAT_YUV422) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output format(%d)", format);
		}

		if (bit_width != OTF_OUTPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output format(%d)", bit_width);
		}
		break;
	case HW_MCSC_DMA_INPUT:
		/* check dma input */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input height(%d)", height);
		}

		if (format != DMA_INPUT_FORMAT_YUV422) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input format(%d)", format);
		}

		if (bit_width != DMA_INPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input format(%d)", bit_width);
		}
		break;
	case HW_MCSC_DMA_OUTPUT:
		/* check dma output */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output width(%d)", width);
		}

		if (height < 16 || height > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output height(%d)", height);
		}

		if (!(format == DMA_OUTPUT_FORMAT_YUV422 ||
			format == DMA_OUTPUT_FORMAT_YUV420)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output format(%d)", format);
		}

		if (bit_width != DMA_OUTPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output format(%d)", bit_width);
		}
		break;
	default:
		err_hw("Invalid MCSC type(%d)", type);
		break;
	}

	return ret;
}

void fimc_is_hw_mcsc_set_shadow_ctrl(struct fimc_is_hw_ip *hw_ip, u32 val)
{
	info("[ID:%d]%s: val(%d)\n", hw_ip->id, __func__, val);
	fimc_is_scaler_set_shadow_reg_ctrl(hw_ip->regs, val);
}

void fimc_is_hw_mcsc_size_dump(struct fimc_is_hw_ip *hw_ip)
{
	int i;
	u32 input_src = 0;
	u32 in_w, in_h = 0;
	u32 rdma_w, rdma_h = 0;
	u32 poly_src_w, poly_src_h = 0;
	u32 poly_dst_w, poly_dst_h = 0;
	u32 post_in_w, post_in_h = 0;
	u32 post_out_w, post_out_h = 0;
	u32 wdma_enable = 0;
	u32 wdma_w, wdma_h = 0;
	u32 rdma_y_stride, rdma_uv_stride = 0;
	u32 wdma_y_stride, wdma_uv_stride = 0;
	struct fimc_is_hw_mcsc_cap *cap;

	BUG_ON(!hw_ip);

	/* skip size dump, if hw_ip wasn't opened */
	if (!test_bit(HW_OPEN, &hw_ip->state))
		return;

	cap = GET_MCSC_HW_CAP(hw_ip);
	if (!cap) {
		err_hw("failed to get hw_mcsc_cap(%p)", cap);
		return;
	}

	input_src = fimc_is_scaler_get_input_source(hw_ip->regs, hw_ip->id);
	fimc_is_scaler_get_input_img_size(hw_ip->regs, hw_ip->id, &in_w, &in_h);
	fimc_is_scaler_get_rdma_size(hw_ip->regs, &rdma_w, &rdma_h);
	fimc_is_scaler_get_rdma_stride(hw_ip->regs, &rdma_y_stride, &rdma_uv_stride);

	dbg_hw("[MCSC]=SIZE=====================================\n"
		"[INPUT] SRC:%d(0:OTF, 1:DMA), SIZE:%dx%d\n"
		"[RDMA] SIZE:%dx%d, STRIDE: Y:%d, UV:%d\n",
		input_src, in_w, in_h,
		rdma_w, rdma_h, rdma_y_stride, rdma_uv_stride);

	for (i = MCSC_OUTPUT0; i < cap->max_output; i++) {
		fimc_is_scaler_get_poly_src_size(hw_ip->regs, i, &poly_src_w, &poly_src_h);
		fimc_is_scaler_get_poly_dst_size(hw_ip->regs, i, &poly_dst_w, &poly_dst_h);
		fimc_is_scaler_get_post_img_size(hw_ip->regs, i, &post_in_w, &post_in_h);
		fimc_is_scaler_get_post_dst_size(hw_ip->regs, i, &post_out_w, &post_out_h);
		fimc_is_scaler_get_wdma_size(hw_ip->regs, i, &wdma_w, &wdma_h);
		fimc_is_scaler_get_wdma_stride(hw_ip->regs, i, &wdma_y_stride, &wdma_uv_stride);
		wdma_enable = fimc_is_scaler_get_dma_out_enable(hw_ip->regs, i);

		dbg_hw("[POLY%d] SRC:%dx%d, DST:%dx%d\n"
			"[POST%d] SRC:%dx%d, DST:%dx%d\n"
			"[WDMA%d] ENABLE:%d, SIZE:%dx%d, STRIDE: Y:%d, UV:%d\n",
			i, poly_src_w, poly_src_h, poly_dst_w, poly_dst_h,
			i, post_in_w, post_in_h, post_out_w, post_out_h,
			i, wdma_enable, wdma_w, wdma_h, wdma_y_stride, wdma_uv_stride);
	}
	dbg_hw("[MCSC]==========================================\n");

	return;
}

