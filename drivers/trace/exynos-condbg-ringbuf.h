/*
 * drivers/trace/exynos-condbg-ringbuf.h
 * simple lockless ringbuffer
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com

 * Exynos-Console-Debugger for Exynos SoC
 * This codes are based on fiq_debugger of google
 * /driver/staging/android/fiq_debugger
 *
 * Author: Hosung Kim <hosung0.kim@samsung.com>
 *         Changki Kim <changki.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/slab.h>

struct ecd_ringbuf {
	int len;
	int head;
	int tail;
	u8 buf[];
};


static inline struct ecd_ringbuf *ecd_ringbuf_alloc(int len)
{
	struct ecd_ringbuf *rbuf;

	rbuf = kzalloc(sizeof(*rbuf) + len, GFP_KERNEL);
	if (rbuf == NULL)
		return NULL;

	rbuf->len = len;
	rbuf->head = 0;
	rbuf->tail = 0;
	smp_mb();

	return rbuf;
}

static inline void ecd_ringbuf_free(struct ecd_ringbuf *rbuf)
{
	kfree(rbuf);
}

static inline int ecd_ringbuf_level(struct ecd_ringbuf *rbuf)
{
	int level = rbuf->head - rbuf->tail;

	if (level < 0)
		level = rbuf->len + level;

	return level;
}

static inline int ecd_ringbuf_room(struct ecd_ringbuf *rbuf)
{
	return rbuf->len - ecd_ringbuf_level(rbuf) - 1;
}

static inline u8
ecd_ringbuf_peek(struct ecd_ringbuf *rbuf, int i)
{
	return rbuf->buf[(rbuf->tail + i) % rbuf->len];
}

static inline int
ecd_ringbuf_consume(struct ecd_ringbuf *rbuf, int count)
{
	count = min(count, ecd_ringbuf_level(rbuf));

	rbuf->tail = (rbuf->tail + count) % rbuf->len;
	smp_mb();

	return count;
}

static inline int
ecd_ringbuf_push(struct ecd_ringbuf *rbuf, u8 datum)
{
	if (ecd_ringbuf_room(rbuf) == 0)
		return 0;

	rbuf->buf[rbuf->head] = datum;
	smp_mb();
	rbuf->head = (rbuf->head + 1) % rbuf->len;
	smp_mb();

	return 1;
}
