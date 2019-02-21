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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/platform_device.h>

#include "score-debug-print.h"
#include "score-fw-queue.h"
#ifdef ENABLE_MEM_OFFSET
#include "score-memory.h"
#endif
#include "score-cache-ctrl.h"

#define ENABLE_DEBUG_TIMER

static inline int score_print_buf_full(struct score_print_buf *buf)
{
	return ((*(buf->rear) + 1) % buf->size) == *(buf->front);
}

static inline int score_print_buf_empty(struct score_print_buf *buf)
{
	return (*(buf->rear) == *(buf->front));
}

static char* score_strcpy(char *dest, const volatile unsigned char* src)
{
	char* tmp = dest;

	while ((*dest++ = *src++) != '\0') {;}

	return tmp;
}

static int score_print_dequeue(struct score_print_buf *buf, char *log)
{
	int front;
	volatile unsigned char *print_buf;

	if (buf->init == 0) {
		return -EINVAL;
	}

	if (score_print_buf_empty(buf)) {
		return -ENODATA;
	}
	front = (*(buf->front) + 1) % buf->size;
	if (front < 0) {
		return -EFAULT;
	}
	print_buf = (buf->kva) + (MAX_PRINT_BUF_SIZE * front);

	score_strcpy(log, (const unsigned char*)print_buf);
	*(buf->front) = front;

	return 0;
}

#ifdef ENABLE_DEBUG_TIMER
static void score_print_timer(unsigned long data)
{
	struct score_print_buf *buf = (struct score_print_buf *)data;
	char log[MAX_PRINT_BUF_SIZE];

	memset(log, '\0', MAX_PRINT_BUF_SIZE);
	while (!score_print_dequeue(buf, log)) {
		printk("[SCORE-TARGET] %s", log);
	}

	mod_timer(&buf->timer, jiffies + SCORE_PRINT_TIME_STEP);
}
#endif

void score_print_flush(struct score_system *system)
{
	char log[MAX_PRINT_BUF_SIZE];

	memset(log, '\0', MAX_PRINT_BUF_SIZE);
	while (!score_print_dequeue(&system->print_buf, log)) {
		printk("[SCORE-TARGET] %s", log);
	}
}

static void score_print_buf_init(struct score_print_buf *buf,
		unsigned char *start, int size)
{

	buf->front = (volatile int *)start;
	buf->rear  = (volatile int *)(buf->front + 1);
	buf->kva   = start + MAX_PRINT_BUF_SIZE;
	buf->size  = (size / MAX_PRINT_BUF_SIZE) - 1;

#ifdef ENABLE_DEBUG_TIMER
	add_timer(&buf->timer);
#endif
	buf->init = 1;
}

int score_print_open(struct score_system *system)
{
	struct score_print_buf *buf;
	unsigned char *score_print_va;
	unsigned int size;

	buf = &system->print_buf;
	score_print_va = (unsigned char *)system->fw_msg_kvaddr;

#ifdef ENABLE_MEM_OFFSET
	size = SCORE_PROFILE_OFFSET - SCORE_PRINT_OFFSET;
#else
	size = SCORE_LOG_BUF_SIZE;
#endif
	memset(score_print_va, 0x0, size);
	score_print_buf_init(buf, score_print_va, size);

	return 0;
}

void score_print_close(struct score_system *system)
{
	struct score_print_buf *buf;

	buf = &system->print_buf;

#ifdef ENABLE_DEBUG_TIMER
	del_timer_sync(&buf->timer);
#endif

#ifdef ENABLE_SC_PRINT_DCACHE_FLUSH
	score_dcache_flush(system);
#endif
	score_print_flush(system);

	buf->init = 0;
}

void score_print_probe(struct score_system *system)
{
	struct score_print_buf *buf;
	buf = &system->print_buf;

#ifdef ENABLE_DEBUG_TIMER
	init_timer(&buf->timer);
	buf->timer.expires = jiffies + SCORE_PRINT_TIME_STEP;
	buf->timer.data = (unsigned long)buf;
	buf->timer.function = score_print_timer;
#endif
}

void score_print_release(struct score_system *system)
{
	struct score_print_buf *buf;

	buf = &system->print_buf;
#ifdef ENABLE_DEBUG_TIMER
	del_timer_sync(&buf->timer);
#endif
}
