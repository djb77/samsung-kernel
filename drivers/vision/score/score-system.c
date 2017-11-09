/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/pm_qos.h>

#include "score-config.h"
#include "score-device.h"
#include "score-system.h"
#include "score-interface.h"
#include "score-debug.h"
#include "score-regs-user-def.h"
#include "score-dump.h"
#include "score-profiler.h"

#define FLUSH_SIZE_SCORE_FW_DMB0	(300  * 1024)

struct score_run_state_info {
	unsigned int reset:			2;
	unsigned int queue_id:			2;
	unsigned int task_id:			7;
	unsigned int vctx_id:			8;
	unsigned int kernel_name:		12;
};

union score_run_state {
	struct score_run_state_info		info;
	unsigned int				reg;
};

static unsigned int score_fw_define_table[MAX_FW_COUNT][FW_TABLE_MAX] = {
/*	index, type, text size, data size, stack size, in stack size, copy on use flag */
	{0, FW_TYPE_CACHE, 0x00200000, 0x00100000, FW_STACK_SIZE, 0x00000000, SCORE_DM,}
/*	{1, FW_TYPE_SRAM,  0x00008000, 0x00008000, 0x00004000,    0x00002000, 0, } */
};

struct pm_qos_request exynos_score_qos_cam;
/* struct pm_qos_request exynos_score_qos_mem; */

static int __score_system_binary_load(struct score_system *system)
{
	int ret = 0;
	unsigned int fw_idx = system->fw_index;
	struct score_binary *binary_text;
	struct score_binary *binary_data;

	binary_text = &system->fw_info[fw_idx].binary_text;
	binary_data = &system->fw_info[fw_idx].binary_data;

	if (!binary_text->info.fw_loaded) {
		ret = score_binary_load(binary_text);
		if (ret) {
			goto p_err;
		}
	}
	if (!binary_data->info.fw_loaded) {
		ret = score_binary_load(binary_data);
		if (ret) {
			goto p_err;
		}
	}

	if (binary_text->info.fw_cou)
		score_binary_upload(binary_text);

	if (binary_data->info.fw_cou)
		score_binary_upload(binary_data);

	/* After upload score_fw_dmb0.bin to DRAM area,
	flushes CPU cache by 300KB for DRAM area of score_fw_dmb0.bin */
	vb2_ion_sync_for_device(system->memory.info.cookie,
			score_fw_define_table[0][FW_TABLE_TEXT_SIZE],
			FLUSH_SIZE_SCORE_FW_DMB0, DMA_TO_DEVICE);

	system->fw_info[fw_idx].fw_state = FW_LD_STAT_ONDRAM;

	return ret;
p_err:
	system->fw_info[fw_idx].fw_state = FW_LD_STAT_FAIL;
	return ret;
}

int score_system_fw_start(struct score_system *system)
{
	int ret = 0;
	unsigned int fw_idx = system->fw_index;

	ret = __score_system_binary_load(system);
	if (ret) {
		goto p_err;
	}

	if (system->fw_info[fw_idx].fw_state != FW_LD_STAT_ONDRAM &&
			system->fw_info[fw_idx].fw_state != FW_LD_STAT_SUCCESS) {
		score_err("Binary not ready (FW:%d) \n", fw_idx);
		ret = -ENOENT;
		goto p_err;
	}

#ifdef ENABLE_MEM_OFFSET
	writel(system->mem_offset, system->regs + SCORE_MEMORY_OFFSET);
	#if 0
	printk("[SCORE] system->fw_info[fw_idx].fw_text_dvaddr = 0x%08X\n", (unsigned int)system->fw_info[fw_idx].fw_text_dvaddr);
	printk("[SCORE] system->fw_info[fw_idx].fw_data_dvaddr = 0x%08X\n", (unsigned int)system->fw_info[fw_idx].fw_data_dvaddr);
	printk("[SCORE] system->malloc_dvaddr = 0x%08X\n", (unsigned int)system->malloc_dvaddr);
	printk("[SCORE] system->fw_msg_dvaddr = 0x%08X\n", (unsigned int)system->fw_msg_dvaddr);
	printk("[SCORE] system->profile_dvaddr = 0x%08X\n", (unsigned int)system->profile_dvaddr);
	printk("[SCORE] system->dump_dvaddr = 0x%08X\n", (unsigned int)system->dump_dvaddr);
	#endif
#else
	writel((unsigned int)system->fw_msg_dvaddr,
			system->regs + SCORE_PRINT_BUF_START);
	writel(SCORE_LOG_BUF_SIZE, system->regs + SCORE_PRINT_BUF_SIZE);
#endif

	/* This is needed because some boards malfunction */
	/* during first write dma operation.              */
	writel(0x10, system->regs + SCORE_CH0_VDMA_CONTROL);
	writel(0x0, system->regs + SCORE_CH0_VDMA_CONTROL);

	/* SW reset */
	// Reg.Name : SCORE_SW_RESET (Offset 0x0040)
	// bit [0]: Score DSP&Cache Reset
	// bit [1]: Score DMA Reset
	writel(0x3, system->regs + SCORE_SW_RESET);
	udelay(1);

	/* Configure start */
	writel(0x1, system->regs + SCORE_AXIM1ARUSER);
	udelay(1);

	if (system->fw_info[fw_idx].fw_type == FW_TYPE_CACHE) {
		writel(system->fw_info[fw_idx].fw_text_dvaddr,
				system->regs + SCORE_CODE_START_ADDR);
		writel(system->fw_info[fw_idx].fw_data_dvaddr - FW_STACK_SIZE,
				system->regs + SCORE_DATA_START_ADDR);
		udelay(1);

		writel(0x3, system->regs + SCORE_CODE_MODE_ADDR);
		writel(0x3, system->regs + SCORE_DATA_MODE_ADDR);
	} else if (system->fw_info[fw_idx].fw_type == FW_TYPE_SRAM) {
		writel(0x1, system->regs + SCORE_CODE_MODE_ADDR);
		writel(0x1, system->regs + SCORE_DATA_MODE_ADDR);
		udelay(1);

		/* SW reset Clear */
		writel(0x0, system->regs + SCORE_SW_RESET);

		memcpy(system->regs + FW_SRAM_REGION_SIZE,
				system->fw_info[fw_idx].fw_text_kvaddr,
				system->fw_info[fw_idx].binary_text.image_size);
		memcpy(system->regs + (FW_SRAM_REGION_SIZE + FW_SRAM_REGION_SIZE +
					0x4000 - 0x2000),
				system->fw_info[fw_idx].fw_data_kvaddr,
				system->fw_info[fw_idx].binary_data.image_size);
		udelay(1);

		writel(0x21, system->regs + SCORE_CODE_MODE_ADDR);
		writel(0x21, system->regs + SCORE_DATA_MODE_ADDR);
	} else {
		score_err("fw_type is not valid(%d) \n",
				system->fw_info[fw_idx].fw_type);

		ret = -EINVAL;
		goto p_err;
	}
	udelay(1);

	/* CACHE signal */
	// Reg.Name : SCORE_AXIM1AWCACHE(Offset 0x0118), SCORE_AXIM1ARCACHE(Offset 0x011C)
	// bit [0]: Bufferable
	// bit [1]: Cacheable
	// bit [2]: Read allocate
	// bit [3]: Write allocate
	writel(0x2, system->regs + SCORE_AXIM1AWCACHE);
	writel(0x2, system->regs + SCORE_AXIM1ARCACHE);

	/* Configure end */
	writel(0x0, system->regs + SCORE_AXIM1ARUSER);
	udelay(1);

	writel(0x1, system->regs + SCORE_ENABLE);
	udelay(1);

	/* SW reset Clear */
	writel(0x0, system->regs + SCORE_SW_RESET);
	udelay(1);

	// Send SCore operation frequency (Hz)
	writel((u32)CLK_OP(&system->exynos, clk_get),
			system->regs + SCORE_CLK_FREQ_HZ);
p_err:
	return ret;
}

int score_system_compare_run_state(struct score_system *system, struct score_frame *frame)
{
	int ret = 0;
	union score_run_state state;

	state.reg = readl(system->regs + SCORE_RUN_STATE);
	if ((state.info.task_id != frame->fid) ||
			(state.info.vctx_id != frame->vid)) {
		ret = -ENOSR;
	}

	return ret;
}

int score_system_get_run_state(struct score_system *system,
		int *qid, int *fid, int *vid, int *kn)
{
	int ret = 0;
	union score_run_state state;

	state.reg = readl(system->regs + SCORE_RUN_STATE);
	if (qid)
		*qid = state.info.queue_id;
	if (fid)
		*fid = state.info.task_id;
	if (vid)
		*vid = state.info.vctx_id;
	if (kn)
		*kn = state.info.kernel_name;

	return ret;
}

int score_system_core_halt(struct score_system *system)
{
	int ret = -EBUSY;
	int timeout = 0;

	writel(0x100, system->regs + SCORE_DSP_INT);
	while (timeout < 100) {
		if (readl(system->regs + SCORE_IDLE)) {
			timeout++;
			udelay(1);
		} else {
			ret = 0;
			break;
		}
	}

	return ret;
}

int score_system_check_resettable(struct score_system *system)
{
	int ret = 0;
	union score_run_state state;

	state.reg = readl(system->regs + SCORE_RUN_STATE);
	if (state.info.reset & 0x1) {
		ret = -EBUSY;
	}
	return ret;
}

static int __score_system_sfr_clean(struct score_system *system, struct score_fw_dev *fw_dev)
{
	int ret = 0;
	/* TODO
	 * IPC queue HEAD, TAIL init
	 * ...
	 */

	score_fw_queue_init(fw_dev);

	return ret;
}

static int __score_system_sfr_backup(struct score_system *system, struct score_fw_dev *fw_dev)
{
	int ret = 0;
	/* TODO
	 * IPC queue backup
	 * ...
	 */
	return ret;
}

static int __score_system_sfr_restore(struct score_system *system, struct score_fw_dev *fw_dev)
{
	int ret = 0;
	/* TODO
	 * IPC queue restore
	 * ...
	 */
	return ret;
}

static int __score_system_check_reset_complete(struct score_system *system)
{
	int core_ret = 0, dma_ret = 0;
	int reset = readl(system->regs + SCORE_SW_RESET);
	int timeout = 0;

	if (unlikely(reset & 0x10)) {
		while (timeout < 100) {
			reset = readl(system->regs + SCORE_SW_RESET);
			if (!(reset & 0x10)) {
				goto p_dma;
			}
			timeout++;
			udelay(1);
		}
		score_err("Core & Cache reset is failed. (SW_RESET SFR:%x)\n", reset);
		core_ret = -EBUSY;
	}
p_dma:
	timeout = 0;
	if (reset & 0x20) {
		while (timeout < 100) {
			reset = readl(system->regs + SCORE_SW_RESET);
			if (!(reset & 0x20)) {
				goto p_end;
			}
			timeout++;
			udelay(1);
		}
		score_err("DMA reset is failed. (SW_RESET SFR:%x)\n", reset);
		dma_ret = -EDEADLK;
	}
p_end:
	return core_ret + dma_ret;
}

static int __score_system_reset(struct score_system *system)
{
	int ret = 0;

	writel(0x3, system->regs + SCORE_SW_RESET);
	if (readl(system->regs + SCORE_SW_RESET) & 0x30) {
		ret = __score_system_check_reset_complete(system);
	}

	return ret;
}

int score_system_reset(struct score_system *system, unsigned long opt)
{
	int ret = 0;
	struct score_device *device;
	struct score_fw_dev *fw_dev;
	int retry;
	int sfr_clean = 0;

	device = container_of(system, struct score_device, system);
	fw_dev = &device->fw_dev;

	if (opt & RESET_OPT_FORCE) {
		if (score_system_core_halt(system)) {
			score_warn("Core is busy \n");
		}
		ret = __score_system_reset(system);
		if (ret) {
			goto p_err;
		}
		if (score_system_check_resettable(system)) {
			__score_system_sfr_clean(system, fw_dev);
			sfr_clean = 1;
		}
	} else if (opt & RESET_OPT_CHECK) {
		for (retry = 0; retry < 100; ++retry) {
			if (score_system_core_halt(system)) {
				score_warn("core is busy \n");
			}
			ret = score_system_check_resettable(system);
			if (!ret) {
				break;
			}
			writel(0x1, system->regs + SCORE_ENABLE);
			udelay(1);
		}
		ret = __score_system_reset(system);
		if (ret) {
			goto p_err;
		}
		if (score_system_check_resettable(system)) {
			__score_system_sfr_clean(system, fw_dev);
			sfr_clean = 1;
		}
	} else {
		score_err("option of sw_reset is unknown (0x%8lX) \n", opt);
		ret = -EINVAL;
		goto p_err;
	}

	/* SCore interrupt clear */
	writel(0xFFFFFFFF, system->regs + SCORE_INT_CLEAR);

	if (opt & RESET_SFR_BACKUP) {
		__score_system_sfr_backup(system, fw_dev);
	}
	if (opt & RESET_SFR_CLEAN) {
		__score_system_sfr_clean(system, fw_dev);
		sfr_clean = 1;
	}
	if (opt & RESET_SFR_RESTORE) {
		__score_system_sfr_restore(system, fw_dev);
	}

	if (sfr_clean) {
		struct score_framemgr *iframemgr = &system->interface.framemgr;
		score_frame_pro_flush(iframemgr);
	}

	if (opt & RESET_OPT_RESTART) {
		ret = score_system_fw_start(system);
	}
p_err:
	return ret;
}

int score_system_active(struct score_system *system)
{
	int active = 0;
#if defined(CONFIG_PM_DEVFREQ)
	if (pm_qos_request_active(&exynos_score_qos_cam)) {
		active = 1;
	}
#endif
	return active;
}

int score_system_resume(struct score_system *system)
{
	int ret = 0;

	score_system_fw_start(system);

	return ret;
}

int score_system_suspend(struct score_system *system)
{
	int ret = 0;
	struct score_interface *interface;
	struct score_framemgr *iframemgr;
	unsigned long flags;

	interface = &system->interface;
	iframemgr = &interface->framemgr;

	spin_lock_irqsave(&iframemgr->slock, flags);
	score_system_reset(system, RESET_OPT_FORCE | RESET_SFR_CLEAN);
	spin_unlock_irqrestore(&iframemgr->slock, flags);

#if defined(CONFIG_PM_DEVFREQ)
	if (system->cam_qos_current && pm_qos_request_active(&exynos_score_qos_cam)) {
		pm_qos_update_request(&exynos_score_qos_cam, 0);
		system->cam_qos_current = 0;
	}
#endif

	return ret;
}

int score_system_runtime_resume(struct score_system *system)
{
	int ret = 0;

#ifndef DISABLE_CLK_OP
	CLK_OP(&system->exynos, clk_cfg);
#endif

#if defined(CONFIG_PM_DEVFREQ)
	if (!system->cam_qos_current && !pm_qos_request_active(&exynos_score_qos_cam)) {
		pm_qos_add_request(&exynos_score_qos_cam,
				PM_QOS_CAM_THROUGHPUT, SCORE_CAM_L0); /* L0, 533MHz */
		system->cam_qos_request = SCORE_CAM_L0;
		system->cam_qos_current = SCORE_CAM_L0;
	}
	/* pm_qos_add_request(&exynos_score_qos_cam,
			PM_QOS_CAM_THROUGHPUT, SCORE_CAM_L5); *//* L5, 466MHz */
	/* pm_qos_add_request(&exynos_score_qos_mem,
			PM_QOS_BUS_THROUGHPUT, SCORE_MIF_L6); *//* L6 */
#endif

#ifndef DISABLE_CLK_OP
	CLK_OP(&system->exynos, clk_on);
#endif

#if defined(CONFIG_VIDEOBUF2_ION)
	ret = CALL_MEMOPS(&system->memory, resume);
	if (ret) {
		score_err("CALL_MEMOPS(resume) is fail(%d)\n", ret);
		goto p_err;
	} else {
		set_bit(0, &system->iommu_act);
	}
#endif

#ifndef DISABLE_CLK_OP
	CTL_OP(&system->exynos, ctl_reset);
#endif

p_err:
	return ret;
}

int score_system_runtime_suspend(struct score_system *system)
{
	int ret = 0;

	score_system_reset(system, RESET_OPT_FORCE | RESET_SFR_CLEAN);

#if defined(CONFIG_VIDEOBUF2_ION)
	if (test_bit(0, &system->iommu_act)) {
		CALL_MEMOPS(&system->memory, suspend);
		clear_bit(0, &system->iommu_act);
	}
#endif

#ifndef DISABLE_CLK_OP
	CLK_OP(&system->exynos, clk_off);
#if defined(CONFIG_PM_DEVFREQ)
	if (pm_qos_request_active(&exynos_score_qos_cam)) {
		pm_qos_remove_request(&exynos_score_qos_cam);
		system->cam_qos_current = 0;
	}
	/* pm_qos_remove_request(&exynos_score_qos_mem); */
#endif
#endif

	return ret;
}

int score_system_runtime_update(struct score_system *system)
{
	int ret = 0;
#if defined(CONFIG_PM_DEVFREQ)
	if (system->cam_qos_current == SCORE_CAM_L0 &&
			pm_qos_request_active(&exynos_score_qos_cam)) {
		pm_qos_update_request(&exynos_score_qos_cam, SCORE_CAM_L2);
		system->cam_qos_request = SCORE_CAM_L2;
		system->cam_qos_current = SCORE_CAM_L2;
	}
#endif
	return ret;
}

#define CORE_HALT_WAIT 100
int score_system_enable(struct score_fw_dev *score_fw_device)
{
	int ret = 0;
	int halt_check;
	int clock_check;
	int timeout = 0;
	void __iomem *regs = score_fw_device->sfr;

	writel(0x1, regs + SCORE_ENABLE);
	while(timeout < CORE_HALT_WAIT) {
		halt_check = readl(regs + SCORE_CORE_HALT_CHK1);
		clock_check = (readl(regs + SCORE_CLOCK_STATUS) & 0x1);
		if (clock_check == 1 && halt_check == 1) {
			break;
		} else {
			writel(0x1, regs + SCORE_ENABLE);
			timeout++;
			udelay(10);
		}
	}

	if (unlikely(timeout == CORE_HALT_WAIT)) {
		score_err("core_halt timeout halt: %08X clk: %08X\n",
			halt_check, clock_check);
		ret = -EFAULT;
	}
	return ret;
}

#ifdef ENABLE_TIME_STAMP
void score_ts_timeover(unsigned long arg)
{
	struct score_time_stamp *pdata = (struct score_time_stamp *)arg;
	struct score_system *system;
	system = container_of(pdata, struct score_system, score_ts);

	spin_lock(&pdata->lock);

	// Get clock frequency of SCore
	pdata->freq_mh = (u32)CLK_OP(&system->exynos, clk_get) / 1000000;
	writel(pdata->freq_mh, system->regs + SCORE_CLK_FREQ);

	// Get linux time value and send it to SCore
	pdata->nsec = local_clock();
	pdata->usec = pdata->nsec;
	do_div(pdata->usec, 1000);
	writel((unsigned int)pdata->usec, system->regs + SCORE_TIME_FROM_LINUX);

	// Get SCore timer7 counter value
	pdata->cnt = readl(system->regs + SCORE_TIMER_CNT7);
	writel(pdata->cnt, system->regs + SCORE_TEMP_TIMER_CNT7);

	spin_unlock(&pdata->lock);
	mod_timer(&pdata->timer, jiffies + SCORE_TS_STEP);
#if 0
	printk("score_clk_freq_mh = %d\n", pdata->freq_mh);
	printk("score_ts_nsec = %llu\n", pdata->nsec);
	printk("score_ts_usec = %d\n", (unsigned int)pdata->usec);
	printk("score_timer7_cnt = %d\n", (unsigned int)pdata->cnt);
#endif
}

int score_time_stamp_init(struct score_system *system)
{
	int ret = 0;

	spin_lock(&system->score_ts.lock);

	system->score_ts.freq_mh = (u32)CLK_OP(&system->exynos, clk_get) / 1000000;
	writel(system->score_ts.freq_mh, system->regs + SCORE_CLK_FREQ);

	// Get linux time value and send it to SCore
	system->score_ts.nsec = local_clock();
	system->score_ts.usec = system->score_ts.nsec;
	do_div(system->score_ts.usec, 1000);
	writel((unsigned int)system->score_ts.usec, system->regs + SCORE_TIME_FROM_LINUX);

	// Get SCore timer7 counter value
	system->score_ts.cnt = readl(system->regs + SCORE_TIMER_CNT7);
	writel(system->score_ts.cnt, system->regs + SCORE_TEMP_TIMER_CNT7);

	spin_unlock(&system->score_ts.lock);

	init_timer(&(system->score_ts.timer));
	system->score_ts.timer.function = score_ts_timeover;
	system->score_ts.timer.data = (unsigned long)(&system->score_ts);
	system->score_ts.timer.expires = jiffies + SCORE_TS_STEP;
	add_timer(&system->score_ts.timer);
#if 0
	printk("score_clk_freq_mh = %d\n", system->score_ts.freq_mh);
	printk("score_ts_nsec = %llu\n", system->score_ts.nsec);
	printk("score_ts_usec = %d\n", (unsigned int)system->score_ts.usec);
	printk("score_timer7_cnt = %d\n", (unsigned int)system->score_ts.cnt);
#endif
	return ret;
}
#endif

int score_system_start(struct score_system *system)
{
	int ret = 0;

#if defined(CONFIG_PM_DEVFREQ)
	if (!system->cam_qos_current && pm_qos_request_active(&exynos_score_qos_cam)) {
		pm_qos_update_request(&exynos_score_qos_cam, system->cam_qos_request);
		system->cam_qos_current = system->cam_qos_request;
	}
#endif
	return ret;
}

int score_system_stop(struct score_system *system)
{
	int ret = 0;
	return ret;
}

#if 0
static void __score_system_fw_ld_cb(const struct firmware *fw_blob, void *context)
{
	int ret;
	struct score_binary *binary = (struct score_binary *)context;

	struct score_device *device;
	struct score_system *system;

	int idx = binary->info.fw_index;

	device = dev_get_drvdata(binary->dev);
	system = &device->system;

	if (!fw_blob) {
		score_err("fw_ld_cb failed(%s)\n", binary->fpath);
		system->fw_info[idx].fw_state = FW_LD_STAT_FAIL;
		goto p_exit;
	}

	ret = score_binary_copy(fw_blob, binary);
	if (ret) {
		binary->info.fw_loaded = false;
		goto p_failed;
	}

	binary->info.fw_loaded = true;
	goto p_ret;

p_failed:
	system->fw_info[idx].fw_state = FW_LD_STAT_FAIL;
p_ret:
	release_firmware(fw_blob);
p_exit:
	return;
}
#endif

static int __score_system_binary_init(struct score_system *system, struct device *dev)
{
	int ret = 0;
	struct score_fw_load_info *fw_info;
	struct score_binary *binary_text;
	struct score_binary *binary_data;

	unsigned int loop;
	unsigned int offset;
	unsigned int org_fw_offset;

	offset = 0;
	org_fw_offset = SCORE_ORGMEM_OFFSET;

	for (loop = 0; loop < MAX_FW_COUNT; loop++) {
		fw_info = &system->fw_info[loop];

		fw_info->fw_state = FW_LD_STAT_INIT;
		fw_info->fw_text_kvaddr = (void *)system->memory.info.kvaddr + offset;
		fw_info->fw_text_dvaddr = system->memory.info.dvaddr + offset;
		fw_info->fw_text_size = score_fw_define_table[loop][FW_TABLE_TEXT_SIZE];

		offset = offset +
			score_fw_define_table[loop][FW_TABLE_TEXT_SIZE] +
			score_fw_define_table[loop][FW_TABLE_STACK_SIZE] -
			score_fw_define_table[loop][FW_TABLE_IN_STACK_SIZE];
		if (score_fw_define_table[loop][FW_TABLE_COU_FLAGS] & SCORE_PM) {
			if ((org_fw_offset - SCORE_ORGMEM_OFFSET) < SCORE_ORGMEM_SIZE) {
				fw_info->fw_org_text_kvaddr =
					(void *)system->memory.info.kvaddr + org_fw_offset;
				org_fw_offset += score_fw_define_table[loop][FW_TABLE_TEXT_SIZE];
			} else {
				score_err("%dth fw(text) exceeds buffer size(%d)\n",
						loop, org_fw_offset);
				fw_info->fw_org_text_kvaddr = NULL;
			}
		} else {
			fw_info->fw_org_text_kvaddr = NULL;
		}

		fw_info->fw_data_kvaddr = (void *)system->memory.info.kvaddr + offset;
		fw_info->fw_data_dvaddr = system->memory.info.dvaddr + offset;
		fw_info->fw_data_size = score_fw_define_table[loop][FW_TABLE_DATA_SIZE];

		fw_info->fw_type = score_fw_define_table[loop][FW_TABLE_TYPE];

		offset = offset + score_fw_define_table[loop][FW_TABLE_DATA_SIZE];

		if (score_fw_define_table[loop][FW_TABLE_COU_FLAGS] & SCORE_DM) {
			if ((org_fw_offset - SCORE_ORGMEM_OFFSET) < SCORE_ORGMEM_SIZE) {
				fw_info->fw_org_data_kvaddr =
					(void *)system->memory.info.kvaddr + org_fw_offset;
				org_fw_offset += score_fw_define_table[loop][FW_TABLE_DATA_SIZE];
			} else {
				score_err("%dth fw(data) exceeds buffer size(%d)\n",
						loop, org_fw_offset);
				fw_info->fw_org_data_kvaddr = NULL;
			}
		} else {
			fw_info->fw_org_data_kvaddr = NULL;
		}

		binary_text = &fw_info->binary_text;
		binary_data = &fw_info->binary_data;

		score_binary_init(binary_text, dev, loop, fw_info->fw_text_kvaddr,
				fw_info->fw_org_text_kvaddr, fw_info->fw_text_size);
		score_binary_set_text_path(binary_text, loop, dev);

		ret = score_binary_load(binary_text);
		if (ret) {
			score_err("%s is not loaded (%d)\n", binary_text->fpath, ret);
			fw_info->fw_state = FW_LD_STAT_FAIL;
			goto p_err;
		}

		score_binary_init(binary_data, dev, loop, fw_info->fw_data_kvaddr,
				fw_info->fw_org_data_kvaddr, fw_info->fw_data_size);
		score_binary_set_data_path(binary_data, loop, dev);

		ret = score_binary_load(binary_data);
		if (ret) {
			score_err("%s is not loaded (%d)\n", binary_data->fpath, ret);
			fw_info->fw_state = FW_LD_STAT_FAIL;
			goto p_err;
		}
	}

p_err:
	// default mode : cache mode
	system->fw_index = FW_TYPE_CACHE;
	return ret;
}

static void __score_system_memory_init(struct score_system *system)
{
	struct score_memory_info *mem_info = &system->memory.info;
#ifdef ENABLE_MEM_OFFSET
	system->malloc_kvaddr = (void *)mem_info->kvaddr + SCORE_MALLOC_OFFSET;
	system->malloc_dvaddr = mem_info->dvaddr + SCORE_MALLOC_OFFSET;

	system->profile_kvaddr = (void *)mem_info->kvaddr + SCORE_PROFILE_OFFSET;
	system->profile_dvaddr = mem_info->dvaddr + SCORE_PROFILE_OFFSET;

	system->dump_kvaddr = (void *)mem_info->kvaddr + SCORE_DUMP_OFFSET;
	system->dump_dvaddr = mem_info->dvaddr + SCORE_DUMP_OFFSET;

	system->fw_msg_kvaddr = (void *)mem_info->kvaddr + SCORE_PRINT_OFFSET;
	system->fw_msg_dvaddr = mem_info->dvaddr + SCORE_PRINT_OFFSET;

	system->org_fw_kvaddr = (void *)mem_info->kvaddr + SCORE_ORGMEM_OFFSET;

	system->mem_offset = (SCORE_MALLOC_OFFSET << SCORE_MALLOC_OFFSET_SHIFT) + \
			     (SCORE_PRINT_OFFSET >> SCORE_PRINT_OFFSET_SHIFT) + \
			     (SCORE_PROFILE_OFFSET >> SCORE_PROFILE_OFFSET_SHIFT) + \
			     (SCORE_DUMP_OFFSET >> SCORE_DUMP_OFFSET_SHIFT) + \
			     (SCORE_DUMP_END >> SCORE_DUMP_END_SHIFT);
#else
	system->fw_msg_kvaddr = (void *)mem_info->kvaddr + \
				SCORE_MEMORY_INTERNAL_SIZE - SCORE_MEMORY_FW_MSG_SIZE;
	system->fw_msg_dvaddr = mem_info->dvaddr + \
				SCORE_MEMORY_INTERNAL_SIZE - SCORE_MEMORY_FW_MSG_SIZE;
#endif
}

int score_system_open(struct score_system *system)
{
	int ret = 0;

	ret = score_memory_open(&system->memory);
	if (ret) {
		goto p_err;
	}

	ret = __score_system_binary_init(system, &system->pdev->dev);
	if (ret) {
		goto p_err_binary;
	}

	__score_system_memory_init(system);

	score_interface_open(&system->interface);
	score_dump_open(system);
	score_profile_open(system);

	return ret;
p_err_binary:
	score_memory_close(&system->memory);
p_err:
	return ret;
}

void score_system_close(struct score_system *system)
{
	score_profile_close();
	score_dump_close(system);
	score_interface_close(&system->interface);
	score_memory_close(&system->memory);
}

int score_system_probe(struct score_system *system, struct platform_device *pdev)
{
	int ret = 0;

	struct device *dev;
	struct resource *res;
	void __iomem *iomem;
	int irq;

	system->pdev = pdev;
	dev = &pdev->dev;
	system->cam_qos_request = SCORE_CAM_L0;
	system->cam_qos_current = 0;
	system->mif_qos = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		probe_err("platform_get_resource(0) is fail(%p)", res);
		ret = PTR_ERR(res);
		goto p_err_resource;
	}

	iomem = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(iomem)) {
		probe_err("devm_ioremap_resource(0) is fail(%p)", iomem);
		ret = PTR_ERR(iomem);
		goto p_err_ioremap;
	}

	system->regs = iomem;
	system->regs_size = resource_size(res);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		probe_err("platform_get_irq(0) is fail(%d)", irq);
		ret = -EINVAL;
		goto p_err_irq;
	}

	system->irq0 = irq;

	ret = score_memory_probe(&system->memory, dev);
	if (ret)
		goto p_err_memory;

	ret = score_interface_probe(&system->interface, dev,
		system->regs, system->regs_size,
		system->irq0);
	if (ret)
		goto p_err_interface;

	ret = score_exynos_probe(&system->exynos, dev, system->regs);
	if (ret)
		goto p_err_exynos;

	score_print_probe(system);

	ret = score_dump_probe(system);
	if (ret)
		goto p_err_dump;

	clear_bit(0, &system->iommu_act);
	return 0;
p_err_dump:
p_err_exynos:
	score_interface_release(&system->interface, dev, system->irq0);
p_err_interface:
	score_memory_release(&system->memory);
p_err_memory:
p_err_irq:
	devm_iounmap(dev, system->regs);
p_err_ioremap:
p_err_resource:
	return ret;
}

void score_system_release(struct score_system *system)
{
	struct device *dev = &system->pdev->dev;

	score_dump_release();
	score_print_release(system);
	score_exynos_release(&system->exynos);
	score_interface_release(&system->interface, dev, system->irq0);
	score_memory_release(&system->memory);
	devm_iounmap(dev, system->regs);
}
