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
#include <linux/gpio.h>
#include <linux/string.h>
#include "score-sysfs.h"
#include "score-device.h"
#include "score-dump.h"
#include "score-profiler.h"

#ifdef ENABLE_SYSFS_SYSTEM
ssize_t show_sysfs_system_dvfs_en(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sysfs_system.en_dvfs);
}

ssize_t store_sysfs_system_dvfs_en(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
#ifdef ENABLE_DVFS
	struct score_device *device =
		(struct score_device *)platform_get_drvdata(to_platform_device(dev));
	struct score_system *system;
	int i;

	BUG_ON(!core);

	system = &device->system;

	switch (buf[0]) {
		case '0':
			sysfs_system.en_dvfs = false;
			/* update dvfs lever to max */
			mutex_lock(&system->dvfs_ctrl.lock);
			for (i = 0; i < FIMC_IS_STREAM_COUNT; i++) {
				if (test_bit(SCORE_DEVICE_OPEN, &device.state))
					score_set_dvfs(system, SCORE_DVFS_MAX);
			}
			score_dvfs_init(system);
			system->dvfs_ctrl.static_ctrl->current_id = SCORE_DVFS_MAX;
			mutex_unlock(&system->dvfs_ctrl.lock);
			break;
		case '1':
			/* It can not re-define static scenario */
			sysfs_system.en_dvfs = true;
			break;
		default:
			pr_debug("%s: %c\n", __func__, buf[0]);
			break;
	}
#endif
	return count;
}

ssize_t show_sysfs_system_clk_gate_en(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sysfs_system.en_clk_gate);
}

ssize_t store_sysfs_system_clk_gate_en(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
#ifdef ENABLE_CLOCK_GATE
	switch (buf[0]) {
		case '0':
			sysfs_system.en_clk_gate = false;
			sysfs_system.clk_gate_mode = CLOCK_GATE_MODE_HOST;
			break;
		case '1':
			sysfs_system.en_clk_gate = true;
			sysfs_system.clk_gate_mode = CLOCK_GATE_MODE_HOST;
			break;
		default:
			pr_debug("%s: %c\n", __func__, buf[0]);
			break;
	}
#endif
	return count;
}

ssize_t show_sysfs_system_clk(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct score_device *device =
		(struct score_device *)platform_get_drvdata(to_platform_device(dev));
	struct score_system *system;
	ssize_t count = 0;

	system = &device->system;
	if (score_system_active(system)) {
		count += sprintf(buf + count, "clk %lu\n",
				CLK_OP(&system->exynos, clk_get));
	} else {
		count += sprintf(buf + count, "clk 0\n");
	}

	return count;
}

ssize_t store_sysfs_system_clk(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	return count;
}

ssize_t show_sysfs_system_sfr(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct score_device *device =
		(struct score_device *)platform_get_drvdata(to_platform_device(dev));
	struct score_system *system;
	ssize_t count = 0;

	system = &device->system;

	if (score_system_active(system)) {
		int i;
		count += sprintf(buf + count, "IDLE %08X\n",
				readl(system->regs + SCORE_IDLE));
		count += sprintf(buf + count, "CLOCK_STATUS %08X\n",
				readl(system->regs + SCORE_CLOCK_STATUS));
		count += sprintf(buf + count, "SW_RESET %08X\n",
				readl(system->regs + SCORE_SW_RESET));
		count += sprintf(buf + count, "DSP_INT %08X\n",
				readl(system->regs + SCORE_DSP_INT));
		count += sprintf(buf + count, "INT_CODE %08X\n",
				readl(system->regs + SCORE_INT_CODE));

		count += sprintf(buf + count, "RUN_STATE %08X\n",
				readl(system->regs + SCORE_RUN_STATE));
		count += sprintf(buf + count, "EXE_LOG %08X\n",
				readl(system->regs + SCORE_EXE_LOG));

		count += sprintf(buf + count, "INQ_TAIL %08X\n",
				readl(system->regs + SCORE_IN_QUEUE_TAIL));
		count += sprintf(buf + count, "INQ_HEAD %08X\n",
				readl(system->regs + SCORE_IN_QUEUE_HEAD));
		for (i = 0; i < SCORE_IN_QUEUE_SIZE; ++i) {
			count += sprintf(buf + count, "INQ[%2d] %08X\n",
					i, readl(system->regs +
						SCORE_IN_QUEUE_HEAD + ((i + 1) * 4)));
		}

		count += sprintf(buf + count, "OTQ_TAIL %08X\n",
				readl(system->regs + SCORE_OUT_QUEUE_TAIL));
		count += sprintf(buf + count, "OTQ_HEAD %08X\n",
				readl(system->regs + SCORE_OUT_QUEUE_HEAD));
		for (i = 0; i < SCORE_OUT_QUEUE_SIZE; ++i) {
			count += sprintf(buf + count, "OTQ[%2d] %08X\n",
					i, readl(system->regs +
						SCORE_OUT_QUEUE_HEAD + ((i + 1) * 4)));
		}

		count += sprintf(buf + count, "VERIFY_RESULT %08X\n",
				readl(system->regs + SCORE_VERIFY_RESULT));
		count += sprintf(buf + count, "VERIFY_OUTPUT %08X\n",
				readl(system->regs + SCORE_VERIFY_OUTPUT));
		count += sprintf(buf + count, "VERIFY_REFERENCE %08X\n",
				readl(system->regs + SCORE_VERIFY_REFERENCE));
		count += sprintf(buf + count, "VERIFY_X_VALUE %08X\n",
				readl(system->regs + SCORE_VERIFY_X_VALUE));
		count += sprintf(buf + count, "VERIFY_Y_VALUE %08X\n",
				readl(system->regs + SCORE_VERIFY_Y_VALUE));
	} else {
		count += sprintf(buf + count, "pm_runtime off\n");
	}

	return count;
}

ssize_t store_sysfs_system_sfr(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	return count;
}

#endif /* ENABLE_SYSFS_SYSTEM */
#ifdef ENABLE_SYSFS_STATE
ssize_t show_sysfs_state_val(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "du(%d) l/s(%d %d) lv/sv(%d %d) lr/sr(%d %d)\n",
			sysfs_state.frame_duration,
			sysfs_state.long_time,
			sysfs_state.short_time,
			sysfs_state.long_v_rank,
			sysfs_state.short_v_rank,
			sysfs_state.long_r_rank,
			sysfs_state.short_r_rank);
}

ssize_t store_sysfs_state_val(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret_count;
	int input_val[7];

	ret_count = sscanf(buf, "%d %d %d %d %d %d %d", &input_val[0], &input_val[1],
			&input_val[2], &input_val[3],
			&input_val[4], &input_val[5], &input_val[6]);
	if (ret_count != 7) {
		probe_err("%s: count should be 7 but %d \n", __func__, ret_count);
		return -EINVAL;
	}

	sysfs_state.frame_duration = input_val[0];
	sysfs_state.long_time = input_val[1];
	sysfs_state.short_time = input_val[2];
	sysfs_state.long_v_rank = input_val[3];
	sysfs_state.short_v_rank = input_val[4];
	sysfs_state.long_r_rank = input_val[5];
	sysfs_state.short_v_rank = input_val[6];

	return count;
}

ssize_t show_sysfs_state_en(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (sysfs_state.is_en)
		return snprintf(buf, PAGE_SIZE, "%s\n", "enabled");
	else
		return snprintf(buf, PAGE_SIZE, "%s\n", "disabled");
}

ssize_t store_sysfs_state_en(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	if (buf[0] == '1')
		sysfs_state.is_en = true;
	else
		sysfs_state.is_en = false;

	return count;
}

int mmap_sysfs_dump(struct file *flip,
		struct kobject *kobj,
		struct bin_attribute *bin_attr,
		struct vm_area_struct *vma)
{
	unsigned long score_dump_pfn;

	score_dump_pfn = vmalloc_to_pfn(score_dump_kvaddr);
	if (remap_pfn_range(vma, vma->vm_start, score_dump_pfn, DUMP_BUF_SIZE, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}
int mmap_sysfs_profile(struct file *flip,
                struct kobject *kobj,
                struct bin_attribute *bin_attr,
                struct vm_area_struct *vma)
{
        unsigned long score_profile_pfn;

        score_profile_pfn = vmalloc_to_pfn(score_profile_kvaddr);
        if (remap_pfn_range(vma, vma->vm_start, score_profile_pfn, PROFILE_BUF_SIZE, vma->vm_page_prot))
                return -EAGAIN;

        return 0;
}

#endif /* ENABLE_SYSFS_STATE */

#ifdef ENABLE_SYSFS_DEBUG
ssize_t show_sysfs_debug_count(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	ssize_t count = 0;

	if (score_sysfs_debug.qbuf_occur) {
		count += sprintf(buf + count, "qbuf %d\n",
				score_sysfs_debug.qbuf_cnt);
	}
	if (score_sysfs_debug.open_occur) {
		count += sprintf(buf + count, "open %d\n",
				score_sysfs_debug.open_cnt);
	}
	if (score_sysfs_debug.vctx_queue_occur) {
		count += sprintf(buf + count, "vctx_queue %d\n",
				score_sysfs_debug.vctx_queue_cnt);
	}
	if (score_sysfs_debug.timeout_occur) {
		count += sprintf(buf + count, "timeout %d\n",
				score_sysfs_debug.timeout_cnt);
	}
	if (score_sysfs_debug.fw_enq_occur) {
		count += sprintf(buf + count, "fw_enq %d\n",
				score_sysfs_debug.fw_enq_cnt);
	}
	if (score_sysfs_debug.ret_fail_occur) {
		count += sprintf(buf + count, "ret_fail %d\n",
				score_sysfs_debug.ret_fail_cnt);
	}

	return count;
}

ssize_t store_sysfs_debug_count(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	switch (buf[0]) {
		case '0':
			memset(&score_sysfs_debug, 0x0, sizeof(struct score_sysfs_debug));
			break;
		default:
			break;
	}

	return count;
}

void sysfs_debug_count_add(int sel)
{
	switch (sel) {
		case NORMAL_CNT_QBUF:
			score_sysfs_debug.qbuf_occur = 1;
			score_sysfs_debug.qbuf_cnt++;
			break;
		case ERROR_CNT_OPEN:
			score_sysfs_debug.open_occur = 1;
			score_sysfs_debug.open_cnt++;
			break;
		case ERROR_CNT_VCTX_QUEUE:
			score_sysfs_debug.vctx_queue_occur = 1;
			score_sysfs_debug.vctx_queue_cnt++;
			break;
		case ERROR_CNT_TIMEOUT:
			score_sysfs_debug.timeout_occur = 1;
			score_sysfs_debug.timeout_cnt++;
			break;
		case ERROR_CNT_FW_ENQ:
			score_sysfs_debug.fw_enq_occur = 1;
			score_sysfs_debug.fw_enq_cnt++;
			break;
		case ERROR_CNT_RET_FAIL:
			score_sysfs_debug.ret_fail_occur = 1;
			score_sysfs_debug.ret_fail_cnt++;
			break;
		default:
			break;
	}
}

#endif /* ENABLE_SYSFS_DEBUG */
