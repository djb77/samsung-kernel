/*
* Samsung debugging features for Samsung's SoC's.
*
* Copyright (c) 2016 Samsung Electronics Co., Ltd.
*      http://www.samsung.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*/

#include <asm/hw_breakpoint.h>
#include <linux/hw_breakpoint.h>

#ifndef SEC_KWATCHER_H
#define SEC_KWATCHER_H

#define	SEC_KWATCHER_UNIT_TEST			1

#define KWATCHER_MAX_NUM 4
#define	IRQ_MASK_BIT			0x80
#define	WATCHER_BIT_LSB_CLR(data, bits)			(data&((0xFFFFFFFFFFFFFFFF>>bits)<<bits))
#define	WATCHER_BIT_LSB_REMAIN(data, bits)		(data&((0xFFFFFFFFFFFFFFFF<<(64-bits))>>(64-bits)))


enum watch_type {
	ARM_KWATCHER_LOAD = 1,
	ARM_KWATCHER_STORE = 2,
	ARM_KWATCHER_BOTH = 3,
	ARM_KWATCHER_MAX
};

enum detect_type {
	TYPE_KWATCHER_PANIC = 1,
	TYPE_KWATCHER_HALT = 2,
	TYPE_KWATCHER_LOG = 3,
	TYPE_KWATCHER_VAR = 4,
	TYPE_KWATCHER_MAX
};

enum comp_op_type {
	COMP_NONE = 0,
	COMP_EQUAL,
	COMP_GREATER,
	COMP_LESS,
	COMP_MAX,
};

struct wpe_watcher {
	enum kwatcher_ops __percpu *updateNeeded;
	struct perf_event wp;
	int inUse;
	u64 base;
	int offset;
	int linked;
	int dtype;
	/*
		allow single condition now, comparison operation 'EQUAL' is only allowed
		operation now, too.
	*/
	struct {
		enum comp_op_type comp_op;
		unsigned long long comp_value;
	} comp_condition;
	int enabled;
	atomic_t processing;
};

struct kwatcher_group {
	struct list_head bypass_list;
	spinlock_t lock;
	struct wpe_watcher wpe_watcher[KWATCHER_MAX_NUM];
	struct perf_event __percpu *bp;
};


#ifdef CONFIG_SEC_KWATCHER
extern int sec_kwatcher_alloc(unsigned long base, unsigned long size, enum watch_type wType, enum detect_type dType, ...);
extern int sec_kwatcher_free(int index);
extern int sec_kwatcher_disable(int index);
extern int sec_kwatcher_enable(int index);
extern void sec_kwatcher_bypass_enter(void);
extern void sec_kwatcher_bypass_exit(void);

extern void kwatcher_exclusive_break_point(unsigned int *base);
extern void kwatcher_exclusive_break_point_uninstall(void);
extern void kwatcher_trace_stop(void);
#else
#define sec_kwatcher_debug(a, b, c, d, e) do { } while (0)
#endif /* CONFIG_SEC_KWATCHER */

#endif /* SEC_KWATCHER_H */
