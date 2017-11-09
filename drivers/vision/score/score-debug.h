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

#ifndef SCORE_DEBUG_H
#define SCORE_DEBUG_H

#define DEBUG_SENTENCE_MAX			300

#define MAX_EVENT_LOG_NUM			128
#define SCORE_EVENT_MAX_NUM			SZ_4K
#define SCORE_EVENT_PRINT_MAX			512

#include "../score-system.h"

enum score_debug_state {
	SCORE_DEBUG_OPEN
};

enum dbg_dma_dump_type {
	DBG_DMA_DUMP_IMAGE,
	DBG_DMA_DUMP_META,
};

typedef enum score_event_type {
	/* INTERUPPT  */
	SCORE_EVENT_INT_START			= 0x0,
	SCORE_EVENT_INT_END,

	/* SHOT */
	SCORE_EVENT_PACKET_SEND			= 0x10,
	SCORE_EVENT_PACKET_DONE,

	/* MEM ALLOC */
	SCORE_EVENT_FW_MEM_ALLOC		= 0x300,
	SCORE_EVENT_FW_MEM_FREE,

	SCORE_EVENT_MSG				= 0x9990,
	SCORE_EVENT_SIZE			= 0x1000,

	SCORE_EVENT_MAX,
} score_event_type_t;

struct score_debug_event_size {
	unsigned int id;
	unsigned int total_width;
	unsigned int total_height;
	unsigned int width;
	unsigned int height;
	unsigned int position_x;
	unsigned int position_y;
};

struct score_debug_event_msg {
	char text[MAX_EVENT_LOG_NUM];
};

struct score_debug_event_lib_memory {
	char		comm[TASK_COMM_LEN];
	u32		size;
	ulong		kvaddr;
	dma_addr_t	dvaddr;
};

struct score_debug_event_overflow {
	unsigned int recovered_count;
};

struct score_debug_event {
	unsigned int num;
	ktime_t time;
	score_event_type_t type;
	union {
		struct score_debug_event_size size;
		struct score_debug_event_msg msg;
		struct score_debug_event_lib_memory lib_mem;
		struct score_debug_event_overflow overflow;
	} event_data;
};

struct score_debug {
	volatile long unsigned int	state;
	struct dentry			*root;
	struct dentry			*logfile;
	struct dentry			*imgfile;
	struct dentry			*event;

	void				*system;

	/* log dump */
	size_t				read_vptr;

	/* debug message */
	size_t				dsentence_pos;
	char				dsentence[DEBUG_SENTENCE_MAX];

	/* event message */
	struct score_debug_event	event_log[SCORE_EVENT_MAX_NUM];
	atomic_t			event_index;

	/* test */
	char				build_date[MAX_EVENT_LOG_NUM];
	unsigned int			iknownothing;
};

extern struct score_debug score_debug;

int score_debug_probe(void);
int score_debug_open(struct score_system *system);
int score_debug_close(void);

void score_dmsg_init(void);
void score_dmsg_concate(const char *fmt, ...);
char *score_dmsg_print(void);
void score_print_buffer(char *buffer, size_t len);

#ifdef ENABLE_DBG_EVENT
void score_debug_event_add(struct score_debug *info,
		struct score_debug_event *event);
int score_debug_info_dump(struct seq_file *s, struct score_debug *info);
int score_debug_info_dump2(void);
void score_debug_info_dump_reset(void);
void score_event_test(struct score_debug *info);
void score_event_add_fw_mem(bool type, char* comm, u32 size, ulong kvaddr,
		dma_addr_t dvaddr);
void score_event_add_ofy_recovered_count(unsigned int recovered_count);
void score_event_add_dram_msg(char *msg);

void score_event_concate_dram_msg(const char *fmt, ...);

long timer_end(struct timespec start_time);
struct timespec timer_start(void);

void score_dump_regs(void __iomem *base_addr, u32 size);

#define score_event_msg(fmt, args...) \
	score_event_concate_dram_msg("[%s:%d]" fmt, __func__, __LINE__, ##args)

#endif
#endif
