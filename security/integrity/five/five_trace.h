/*
 * five_trace.h
 *
 * Copyright (C) 2017 Samsung Electronics, Inc.
 * Ivan Vorobiov, <i.vorobiov@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM five

#if !defined(__FIVE_TRACE_H)
enum {
	FIVE_TRACE_UNDEFINED,
	FIVE_TRACE_MEASUREMENT_OP_CACHE,
	FIVE_TRACE_MEASUREMENT_OP_CALC,
	FIVE_TRACE_MEASUREMENT_OP_FILTEROUT,
	FIVE_TRACE_MEASUREMENT_TYPE_RSA,
	FIVE_TRACE_MEASUREMENT_TYPE_HMAC
};

enum {
	FIVE_TRACE_ENTRY_MMAP_CHECK,
	FIVE_TRACE_ENTRY_BPRM_CHECK,
	FIVE_TRACE_ENTRY_FILE_CHECK,
	FIVE_TRACE_ENTRY_VERIFY,
	FIVE_TRACE_ENTRY_FORK,
};

enum {
	FIVE_TRACE_FILTEROUT_NONREG,
	FIVE_TRACE_FILTEROUT_NONACTION,
	FIVE_TRACE_FILTEROUT_NONEINTEGRITY,
	FIVE_TRACE_FILTEROUT_ERROR,
	FIVE_TRACE_FILTEROUT_NOXATTRS,
};

#endif

#if !defined(__FIVE_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define __FIVE_TRACE_H

#include "five.h"

#include <linux/tracepoint.h>

TRACE_EVENT(five_measurement_enter,
	TP_PROTO(struct file *file, unsigned long pid),
	TP_ARGS(file, pid),
	TP_STRUCT__entry(
		__field(unsigned long, inode)
		__field(struct super_block  *, sb)
		__field(unsigned long, pid)
	),
	TP_fast_assign(
		__entry->inode = file ? file_inode(file)->i_ino : 0;
		__entry->sb = file_inode(file)->i_sb;
		__entry->pid = pid;
	),
	TP_printk("measurement_enter %lu (%s) %lu",
		__entry->inode, __entry->sb->s_id, __entry->pid)
);

TRACE_EVENT(five_measurement_exit,
	TP_PROTO(struct file *file, unsigned int op, unsigned int type),
	TP_ARGS(file, op, type),
	TP_STRUCT__entry(
		__field(unsigned long, inode)
		__field(unsigned int, op)
		__field(unsigned int, type)
	),
	TP_fast_assign(
		__entry->inode = file ? file_inode(file)->i_ino : 0;
		__entry->op = op;
		__entry->type = type;
	),
	TP_printk("measurement_exit(%d,%d) %lu",
		__entry->op, __entry->type, __entry->inode)
);

TRACE_EVENT(five_push_workqueue,
	TP_PROTO(struct file *file, unsigned int function),
	TP_ARGS(file, function),
	TP_STRUCT__entry(
		__field(unsigned long, inode)
		__field(unsigned int, function)
	),
	TP_fast_assign(
		__entry->inode = file ? file_inode(file)->i_ino : 0;
		__entry->function = function;
	),
	TP_printk("push_workqueue %lu %u", __entry->inode, __entry->function)
);

TRACE_EVENT(five_entry,
	TP_PROTO(struct file *file, unsigned int function),
	TP_ARGS(file, function),
	TP_STRUCT__entry(
		__field(unsigned long, inode)
		__field(unsigned int, function)
	),
	TP_fast_assign(
		__entry->inode = file ? file_inode(file)->i_ino : 0;
		__entry->function = function;
	),
	TP_printk("entry %lu %u", __entry->inode, __entry->function)
);

TRACE_EVENT(five_sign_enter,
	TP_PROTO(struct file *file),
	TP_ARGS(file),
	TP_STRUCT__entry(
		__field(unsigned long, inode)
		__field(struct super_block  *, sb)
	),
	TP_fast_assign(
		__entry->inode = file ? file_inode(file)->i_ino : 0;
		__entry->sb = file_inode(file)->i_sb;
	),
	TP_printk("sign_enter %lu (%s)", __entry->inode, __entry->sb->s_id)
);

TRACE_EVENT(five_sign_exit,
	TP_PROTO(struct file *file, unsigned int op, unsigned int type),
	TP_ARGS(file, op, type),
	TP_STRUCT__entry(
		__field(unsigned long, inode)
		__field(unsigned int, op)
		__field(unsigned int, type)
	),
	TP_fast_assign(
		__entry->inode = file ? file_inode(file)->i_ino : 0;
		__entry->op = op;
		__entry->type = type;
	),
	TP_printk("sign_exit(%d,%d) %lu",
			__entry->op, __entry->type, __entry->inode)
);

#endif /* __FIVE_TRACE_H */

/* this part must be outside the header guard */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE five_trace

#include <trace/define_trace.h>

