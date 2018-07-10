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

#ifndef SCORE_SYSTEM_H_
#define SCORE_SYSTEM_H_

#include <linux/platform_device.h>
#include <linux/delay.h>

#include "score-interface.h"
#include "score-exynos.h"
#include "score-memory.h"
#include "score-binary.h"
#include "score-config.h"
#include "score-fw-queue.h"
#include "score-debug-print.h"

#ifdef ENABLE_TIME_STAMP
#define SCORE_TS_STEP				(2 * HZ) /* 2 second */
#endif

#define RESET_OPT_FORCE				(1UL << 0)
#define RESET_OPT_CHECK				(1UL << 1)
#define RESET_OPT_RESTART			(1UL << 2)

#define RESET_SFR_NOBACKUP			(1UL << 10)
#define RESET_SFR_BACKUP			(1UL << 11)
#define RESET_SFR_RESTORE			(1UL << 12)
#define RESET_SFR_CLEAN				(1UL << 13)

#ifdef ENABLE_TIME_STAMP
struct score_time_stamp {
	struct timer_list			timer;
	spinlock_t				lock;

	u32					freq_mh;
	u64					nsec;
	u64					usec;
	u32					cnt;
};
#endif

struct score_system {
	struct platform_device                  *pdev;
	struct score_fw_load_info		fw_info[MAX_FW_COUNT];
	u32					fw_index;
	void __iomem				*regs;
	resource_size_t				regs_size;
	int					irq0;

	u32					cam_qos_current;
	u32					cam_qos_request;
	u32					mif_qos;
	unsigned long				iommu_act;

	struct score_interface			interface;
	struct score_exynos			exynos;
	struct score_memory			memory;

	struct score_print_buf			print_buf;
#ifdef ENABLE_MEM_OFFSET
	u32					mem_offset;

	void					*malloc_kvaddr;
	dma_addr_t				malloc_dvaddr;

	void					*profile_kvaddr;
	dma_addr_t				profile_dvaddr;

	void					*dump_kvaddr;
	dma_addr_t				dump_dvaddr;

	void					*fw_msg_kvaddr;
	dma_addr_t				fw_msg_dvaddr;

	void					*org_fw_kvaddr;
#else
	void					*fw_msg_kvaddr;
	dma_addr_t				fw_msg_dvaddr;
#endif
#ifdef ENABLE_TIME_STAMP
	struct score_time_stamp			score_ts;
#endif
};

int score_system_probe(struct score_system *system, struct platform_device *pdev);
void score_system_release(struct score_system *system);
int score_system_open(struct score_system *system);
void score_system_close(struct score_system *system);
int score_system_start(struct score_system *system);
int score_system_stop(struct score_system *system);

int score_system_active(struct score_system *system);
int score_system_resume(struct score_system *system);
int score_system_suspend(struct score_system *system);
int score_system_runtime_resume(struct score_system *system);
int score_system_runtime_suspend(struct score_system *system);
int score_system_runtime_update(struct score_system *system);
int score_system_fw_start(struct score_system *system);

int score_system_check_resettable(struct score_system *system);
int score_system_reset(struct score_system *system, unsigned long opt);
int score_system_core_halt(struct score_system *system);

int score_system_compare_run_state(struct score_system *system, struct score_frame *frame);
int score_system_get_run_state(struct score_system *system,
		int *qid, int *fid, int *vid, int *kn);
int score_system_enable(struct score_fw_dev *score_fw_device);

#ifdef ENABLE_TIME_STAMP
int score_time_stamp_init(struct score_system *system);
#endif

# endif
