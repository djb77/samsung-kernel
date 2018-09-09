/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_watchdog.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifdef BIGDATA_LOGGING_ENABLE
#include <linux/sec_debug.h>
#endif

#include "s5p_mfc_watchdog.h"

#include "s5p_mfc_sync.h"

#include "s5p_mfc_pm.h"
#include "s5p_mfc_cmd.h"
#include "s5p_mfc_cal.h"
#include "s5p_mfc_reg.h"

#include "s5p_mfc_queue.h"
#include "s5p_mfc_utils.h"

#define MFC_SFR_AREA_COUNT	21
static void mfc_dump_regs(struct s5p_mfc_dev *dev)
{
	int i;
	struct s5p_mfc_buf_size_v6 *buf_size = NULL;
	int addr[MFC_SFR_AREA_COUNT][2] = {
		{ 0x0, 0x80 },
		{ 0x1000, 0xCD0 },
		{ 0xF000, 0xFF8 },
		{ 0x2000, 0xA00 },
		{ 0x2f00, 0x6C },
		{ 0x3000, 0x40 },
		{ 0x3094, 0x4 },
		{ 0x30b4, 0x8 },
		{ 0x3110, 0x10 },
		{ 0x5000, 0x100 },
		{ 0x5200, 0x300 },
		{ 0x5600, 0x100 },
		{ 0x5800, 0x100 },
		{ 0x5A00, 0x100 },
		{ 0x6000, 0xC4 },
		{ 0x7000, 0x21C },
		{ 0x8000, 0x20C },
		{ 0x9000, 0x10C },
		{ 0xA000, 0x20C },
		{ 0xB000, 0x444 },
		{ 0xC000, 0x84 },
	};

	pr_err("-----------dumping MFC registers (SFR base = %p, dev = %p)\n",
				dev->regs_base, dev);

	s5p_mfc_enable_all_clocks(dev);

	for (i = 0; i < MFC_SFR_AREA_COUNT; i++) {
		printk("[%04X .. %04X]\n", addr[i][0], addr[i][0] + addr[i][1]);
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4, dev->regs_base + addr[i][0],
				addr[i][1], false);
		printk("...\n");
	}

	if (dbg_enable) {
		buf_size = dev->variant->buf_size->buf;
		printk("[DBG INFO dump]\n");
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4, dev->dbg_info_buf.vaddr,
			buf_size->dbg_info_buf, false);
		printk("...\n");
	}
}

const u32 s5p_mfc_logging_sfr_set1[MFC_SFR_LOGGING_COUNT_SET1] = {
	0x1000, 0x1004, 0x100C, 0x1010
};

const u32 s5p_mfc_logging_sfr_set2[MFC_SFR_LOGGING_COUNT_SET2] = {
	0x0070, 0x10B4, 0x2020, 0x2028,
	0x204C, 0x20B4, 0x3000, 0x3004,
	0x3010, 0x301C, 0x3110, 0x5A54,
	0x5A80, 0x5A88, 0x5A94, 0x6038,
	0x6050, 0x6168, 0x7018, 0x7110,
	0x7114, 0xF088, 0xFFD0
};

int mfc_change_hex_to_ascii(u32 hex, u32 byte, char *ascii, int idx)
{
	int i;
	char tmp;

	for (i = 0; i < byte; i++) {
		if (idx >= MFC_LOGGING_DATA_SIZE) {
			pr_err("logging data size exceed: %d\n", idx);
			return idx;
		}

		tmp = (hex >> ((byte - 1 - i) * 4)) & 0xF;
		if (tmp > 9)
			ascii[idx] = tmp + 'a' - 0xa;
		else if (tmp <= 9)
			ascii[idx] = tmp + '0';
		idx++;
	}

	/* space */
	if (idx < MFC_LOGGING_DATA_SIZE)
		ascii[idx] = ' ';

	return ++idx;
}

static void mfc_save_logging_sfr(struct s5p_mfc_dev *dev)
{
	char *errorinfo;
	int i, idx = 0;

	pr_err("-----------logging MFC error info-----------\n");
	errorinfo = dev->logging_data->errorinfo;
	for (i = 0; i < MFC_SFR_LOGGING_COUNT_SET1; i++)
		dev->logging_data->SFRs_set1[i] = MFC_READL(s5p_mfc_logging_sfr_set1[i]);
	for (i = 0; i < MFC_SFR_LOGGING_COUNT_SET2; i++)
		dev->logging_data->SFRs_set2[i] = MFC_READL(s5p_mfc_logging_sfr_set2[i]);

	idx = mfc_change_hex_to_ascii(dev->logging_data->cause, 8, errorinfo, idx);
	idx = mfc_change_hex_to_ascii(dev->logging_data->fault_status, 2, errorinfo, idx);
	idx = mfc_change_hex_to_ascii(dev->logging_data->fault_trans_info, 8, errorinfo, idx);
	idx = mfc_change_hex_to_ascii(dev->logging_data->fault_addr, 8, errorinfo, idx);
	for (i = 0; i < MFC_SFR_LOGGING_COUNT_SET1; i++)
		idx = mfc_change_hex_to_ascii(dev->logging_data->SFRs_set1[i], 2, errorinfo, idx);
	for (i = 0; i < MFC_SFR_LOGGING_COUNT_SET2; i++)
		idx = mfc_change_hex_to_ascii(dev->logging_data->SFRs_set2[i], 8, errorinfo, idx);

	pr_err("%s\n", errorinfo);

#ifdef BIGDATA_LOGGING_ENABLE
	sec_debug_set_extra_info_mfc_error(errorinfo);
#endif
}

static void mfc_display_state(struct s5p_mfc_dev *dev)
{
	nal_queue_handle *nal_q_handle = dev->nal_q_handle;
	int i;

	pr_err("-----------dumping MFC device info-----------\n");
	pr_err("power:%d, clock:%d, num_inst:%d, num_drm_inst:%d, fw_status:%d\n",
			s5p_mfc_pm_get_pwr_ref_cnt(dev), s5p_mfc_pm_get_clk_ref_cnt(dev),
			dev->num_inst, dev->num_drm_inst, dev->fw.status);
	pr_err("hwlock bits:%#lx / dev:%#lx, curr_ctx:%d (is_drm:%d),"
			" preempt_ctx:%d, work_bits:%#lx\n",
			dev->hwlock.bits, dev->hwlock.dev,
			dev->curr_ctx, dev->curr_ctx_is_drm,
			dev->preempt_ctx, s5p_mfc_get_bits(&dev->work_bits));
	pr_err("NAL-Q state:%d, exception:%d, in_exe_cnt: %d, out_exe_cnt: %d\n",
			nal_q_handle->nal_q_state, nal_q_handle->nal_q_exception,
			nal_q_handle->nal_q_in_handle->in_exe_count,
			nal_q_handle->nal_q_out_handle->out_exe_count);

	for (i = 0; i < MFC_NUM_CONTEXTS; i++)
		if (dev->ctx[i])
			pr_err("MFC ctx[%d] %s(%d) state:%d, queue_cnt(src:%d, dst:%d, ref:%d, qsrc:%d, qdst:%d)\n"
				"     interrupt(cond:%d, type:%d, err:%d)\n",
				dev->ctx[i]->num,
				dev->ctx[i]->type == MFCINST_DECODER ? "DEC" : "ENC",
				dev->ctx[i]->codec_mode, dev->ctx[i]->state,
				s5p_mfc_get_queue_count(&dev->ctx[i]->buf_queue_lock, &dev->ctx[i]->src_buf_queue),
				s5p_mfc_get_queue_count(&dev->ctx[i]->buf_queue_lock, &dev->ctx[i]->dst_buf_queue),
				s5p_mfc_get_queue_count(&dev->ctx[i]->buf_queue_lock, &dev->ctx[i]->ref_buf_queue),
				s5p_mfc_get_queue_count(&dev->ctx[i]->buf_queue_lock, &dev->ctx[i]->src_buf_nal_queue),
				s5p_mfc_get_queue_count(&dev->ctx[i]->buf_queue_lock, &dev->ctx[i]->dst_buf_nal_queue),
				dev->ctx[i]->int_condition, dev->ctx[i]->int_reason,
				dev->ctx[i]->int_err);
}

static void mfc_print_trace(struct s5p_mfc_dev *dev)
{
	int i, cnt, trace_cnt;

	pr_err("-----------dumping MFC trace info-----------\n");

	trace_cnt = atomic_read(&dev->trace_ref);
	for (i = MFC_TRACE_COUNT_PRINT - 1; i >= 0; i--) {
		cnt = ((trace_cnt + MFC_TRACE_COUNT_MAX) - i) % MFC_TRACE_COUNT_MAX;
		pr_err("MFC trace[%d]: time=%llu, str=%s", cnt,
				dev->mfc_trace[cnt].time, dev->mfc_trace[cnt].str);
	}
}

void s5p_mfc_dump_buffer_info(struct s5p_mfc_dev *dev, unsigned long addr)
{
	struct s5p_mfc_ctx *ctx;

	ctx = dev->ctx[dev->curr_ctx];
	if (ctx) {
		pr_err("-----------dumping MFC buffer info (fault at: %#lx)\n", addr);
		pr_err("common:%#llx~%#llx, instance:%#llx~%#llx, codec:%#llx~%#llx\n",
				dev->common_ctx_buf.daddr,
				dev->common_ctx_buf.daddr + PAGE_ALIGN(0x7800),
				ctx->instance_ctx_buf.daddr,
				ctx->instance_ctx_buf.daddr + ctx->instance_ctx_buf.size,
				ctx->codec_buf.daddr,
				ctx->codec_buf.daddr + ctx->codec_buf.size);
		if (ctx->type == MFCINST_DECODER) {
			pr_err("Decoder CPB:%#x++%#x, scratch:%#x++%#x, static(vp9):%#x++%#x\n",
					MFC_READL(S5P_FIMV_D_CPB_BUFFER_ADDR),
					MFC_READL(S5P_FIMV_D_CPB_BUFFER_SIZE),
					MFC_READL(S5P_FIMV_D_SCRATCH_BUFFER_ADDR),
					MFC_READL(S5P_FIMV_D_SCRATCH_BUFFER_SIZE),
					MFC_READL(S5P_FIMV_D_STATIC_BUFFER_ADDR),
					MFC_READL(S5P_FIMV_D_STATIC_BUFFER_SIZE));
			pr_err("DPB [0]plane:++%#x, [1]plane:++%#x, [2]plane:++%#x, MV buffer:++%#lx\n",
					ctx->raw_buf.plane_size[0],
					ctx->raw_buf.plane_size[1],
					ctx->raw_buf.plane_size[2],
					ctx->mv_size);
			print_hex_dump(KERN_ERR, "[0] plane ", DUMP_PREFIX_ADDRESS, 32, 4,
					dev->regs_base + S5P_FIMV_D_FIRST_PLANE_DPB0,
					0x100, false);
			print_hex_dump(KERN_ERR, "[1] plane ", DUMP_PREFIX_ADDRESS, 32, 4,
					dev->regs_base + S5P_FIMV_D_SECOND_PLANE_DPB0,
					0x100, false);
			if (ctx->dst_fmt->num_planes == 3)
				print_hex_dump(KERN_ERR, "[2] plane ", DUMP_PREFIX_ADDRESS, 32, 4,
						dev->regs_base + S5P_FIMV_D_THIRD_PLANE_DPB0,
						0x100, false);
			print_hex_dump(KERN_ERR, "MV buffer ", DUMP_PREFIX_ADDRESS, 32, 4,
					dev->regs_base + S5P_FIMV_D_MV_BUFFER0,
					0x100, false);
		} else if (ctx->type == MFCINST_ENCODER) {
			pr_err("Encoder SRC %dplane, [0]:%#x++%#x, [1]:%#x++%#x, [2]:%#x++%#x\n",
					ctx->src_fmt->num_planes,
					MFC_READL(S5P_FIMV_E_SOURCE_FIRST_ADDR),
					ctx->raw_buf.plane_size[0],
					MFC_READL(S5P_FIMV_E_SOURCE_SECOND_ADDR),
					ctx->raw_buf.plane_size[1],
					MFC_READL(S5P_FIMV_E_SOURCE_THIRD_ADDR),
					ctx->raw_buf.plane_size[2]);
			pr_err("DST:%#x++%#x, scratch:%#x++%#x\n",
					MFC_READL(S5P_FIMV_E_STREAM_BUFFER_ADDR),
					MFC_READL(S5P_FIMV_E_STREAM_BUFFER_SIZE),
					MFC_READL(S5P_FIMV_E_SCRATCH_BUFFER_ADDR),
					MFC_READL(S5P_FIMV_E_SCRATCH_BUFFER_SIZE));
			pr_err("DPB [0] plane:++%#lx, [1] plane:++%#lx, ME buffer:++%#lx\n",
					ctx->enc_priv->luma_dpb_size,
					ctx->enc_priv->chroma_dpb_size,
					ctx->enc_priv->me_buffer_size);
			print_hex_dump(KERN_ERR, "[0] plane ", DUMP_PREFIX_ADDRESS, 32, 4,
					dev->regs_base + S5P_FIMV_E_LUMA_DPB,
					0x44, false);
			print_hex_dump(KERN_ERR, "[1] plane ", DUMP_PREFIX_ADDRESS, 32, 4,
					dev->regs_base + S5P_FIMV_E_CHROMA_DPB,
					0x44, false);
			print_hex_dump(KERN_ERR, "ME buffer ", DUMP_PREFIX_ADDRESS, 32, 4,
					dev->regs_base + S5P_FIMV_E_ME_BUFFER,
					0x44, false);
		} else {
			pr_err("invalid MFC instnace type(%d)\n", ctx->type);
		}
	}
}

void s5p_mfc_dump_info_and_stop_hw(struct s5p_mfc_dev *dev)
{
	MFC_TRACE_DEV("** mfc will stop!!!\n");
	mfc_display_state(dev);
	mfc_print_trace(dev);
	mfc_save_logging_sfr(dev);
	mfc_dump_regs(dev);
	exynos_sysmmu_show_status(dev->device);
	BUG();
}

void s5p_mfc_watchdog_worker(struct work_struct *work)
{
	struct s5p_mfc_dev *dev;
	int cmd = 0;

	dev = container_of(work, struct s5p_mfc_dev, watchdog_work);

	if (atomic_read(&dev->watchdog_run)) {
		mfc_err_dev("watchdog already running???\n");
		return;
	}

	if (!atomic_read(&dev->watchdog_tick_cnt)) {
		mfc_err_dev("interrupt handler is called\n");
		return;
	}

	/* Check whether HW interrupt has occured or not */
	if (s5p_mfc_pm_get_pwr_ref_cnt(dev) && s5p_mfc_pm_get_clk_ref_cnt(dev))
		cmd = s5p_mfc_check_int_cmd(dev);
	if (cmd) {
		if (atomic_read(&dev->watchdog_tick_cnt) == (3 * WATCHDOG_TICK_CNT_TO_START_WATCHDOG)) {
			mfc_err_dev("MFC driver waited for upward of %dsec\n",
						3 * WATCHDOG_TICK_CNT_TO_START_WATCHDOG);
			dev->logging_data->cause |= (1 << MFC_CAUSE_NO_SCHEDULING);
		} else {
			mfc_err_dev("interrupt(%d) is occured, wait scheduling\n", cmd);
			return;
		}
	} else {
		dev->logging_data->cause |= (1 << MFC_CAUSE_NO_INTERRUPT);
		mfc_err_dev("Driver timeout error handling\n");
	}

	/* Run watchdog worker */
	atomic_set(&dev->watchdog_run, 1);

	/* Reset the timeout watchdog */
	atomic_set(&dev->watchdog_tick_cnt, 0);

	/* Stop after dumping information */
	s5p_mfc_dump_info_and_stop_hw(dev);
}
