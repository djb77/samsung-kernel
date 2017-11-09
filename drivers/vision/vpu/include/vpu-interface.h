/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VPU_INTERFACE_H_
#define VPU_INTERFACE_H_

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

#include "vpu-framemgr.h"
#include "vpu-slab.h"

#define VPU_WORK_MAX_COUNT		20
#define VPU_WORK_MAX_DATA		24
#define VPU_COMMAND_TIMEOUT		(3 * HZ)

struct vpu_work {
	struct list_head		list;
	u32				valid:1;
	u32				id;
	u32				message;
	u32				param0;
	u32				param1;
	u32				param2;
	u32				param3;
	u8				data[VPU_WORK_MAX_DATA];
};

struct vpu_work_list {
	struct vpu_work			work[VPU_WORK_MAX_COUNT];
	spinlock_t			slock;
	struct list_head		free_head;
	u32				free_cnt;
	struct list_head		reply_head;
	u32				reply_cnt;
	wait_queue_head_t		wait_queue;
};

enum vpu_interface_state {
	VPU_ITF_STATE_OPEN,
	VPU_ITF_STATE_ENUM,
	VPU_ITF_STATE_START
};

struct vpu_interface {
	void __iomem			*code;
	resource_size_t			code_size;
	void __iomem			*regs;
	resource_size_t			regs_size;
	unsigned long			state;

	struct vpu_slab_allocator	slab;
	struct vpu_framemgr		framemgr;
	void				*cookie;
	u32				done_cnt;

	struct vpu_frame		*request[VPU_MAX_GRAPH];
	struct mutex			request_barrier;
	struct vpu_work			reply[VPU_MAX_GRAPH];
	wait_queue_head_t		reply_queue;
	struct vpu_frame		*process;
	spinlock_t			process_barrier;

	struct vpu_work_list		work_list;
	struct work_struct		work_queue;

	struct timer_list		timer;
	void				*private_data;
};

int vpu_interface_probe(struct vpu_interface *interface,
	struct device *dev,
	void __iomem *code,
	resource_size_t code_size,
	void __iomem *regs,
	resource_size_t regs_size,
	u32 irq0, u32 irq1);
int vpu_interface_open(struct vpu_interface *interface);
int vpu_interface_close(struct vpu_interface *interface);
int vpu_interface_start(struct vpu_interface *interface);
int vpu_interface_stop(struct vpu_interface *interface);
void vpu_interface_print(struct vpu_interface *interface);

int vpu_hw_enum(struct vpu_interface *interface);
int vpu_hw_init(struct vpu_interface *interface, struct vpu_frame *iframe);
int vpu_hw_create(struct vpu_interface *interface, struct vpu_frame *iframe);
int vpu_hw_destroy(struct vpu_interface *interface, struct vpu_frame *iframe);
int vpu_hw_allocate(struct vpu_interface *interface, struct vpu_frame *iframe);
int vpu_hw_process(struct vpu_interface *interface, struct vpu_frame *iframe);
int vpu_hw_power_down(struct vpu_interface *interface);

#endif
