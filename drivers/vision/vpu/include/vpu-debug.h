/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_DEBUG_H_
#define VPU_DEBUG_H_

#include <linux/types.h>
#include <linux/timer.h>

#include "vpu-config.h"

#define DEBUG_SENTENCE_MAX		300
#define DEBUG_MONITORING_PERIOD 	(HZ * 5)

struct vpu_debug_imgdump {
	struct dentry			*file;
	u32				target_graph;
	u32				target_chain;
	u32				target_pu;
	u32				target_index;
	void				*kvaddr;
	void				*cookie;
	size_t				length;
	size_t				offset;
};

struct vpu_debug_monitor {
	struct timer_list		timer;
	u32				time_cnt;
	u32				tick_cnt;
	u32				sched_cnt;
	u32				done_cnt;
};

enum vpu_debug_state {
	VPU_DEBUG_STATE_START
};

struct vpu_debug {
	unsigned long			state;

	struct dentry			*root;
	struct dentry			*logfile;
	struct dentry			*grpfile;
	struct dentry			*buffile;

	/* graph */
	void				*graphmgr_data;
	void				*system_data;

	struct vpu_debug_imgdump	imgdump;
	struct vpu_debug_monitor	monitor;
};

struct vpu_debug_log {
	size_t			dsentence_pos;
	char			dsentence[DEBUG_SENTENCE_MAX];
};

s32 atoi(const char *psz_buf);
int bitmap_scnprintf(char *buf, unsigned int buflen,
        const unsigned long *maskp, int nmaskbits);

int vpu_debug_probe(struct vpu_debug *debug, void *graphmgr_data, void *interface_data);
int vpu_debug_open(struct vpu_debug *debug);
int vpu_debug_close(struct vpu_debug *debug);
int vpu_debug_start(struct vpu_debug *debug);
int vpu_debug_stop(struct vpu_debug *debug);

void vpu_dmsg_concate(struct vpu_debug_log *log, const char *fmt, ...);
char * vpu_dmsg_print(struct vpu_debug_log *log);
int vpu_debug_memdump8(u8 *start, u8 *end);
int vpu_debug_memdump16(u16 *start, u16 *end);
int vpu_debug_memdump32(u32 *start, u32 *end);

#ifdef DBG_HISTORY
#define DLOG_INIT()		struct vpu_debug_log vpu_debug_log = { .dsentence_pos = 0 }
#define DLOG(fmt, ...)		vpu_dmsg_concate(&vpu_debug_log, fmt, ##__VA_ARGS__)
#define DLOG_OUT()		vpu_dmsg_print(&vpu_debug_log)
#else
#define DLOG_INIT()
#define DLOG(fmt, ...)
#define DLOG_OUT()		"FORBIDDEN HISTORY"
#endif

#endif