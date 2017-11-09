/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos Scaler driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __EXYNOS_IMA_H_
#define __EXYNOS_IMA_H_

#include <linux/delay.h>
#include <linux/genalloc.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/types.h>

struct ima_client;

typedef int (*ima_reclaim_callback_t)(struct ima_client *,
					struct device *, void *);

struct ima_client {
	struct list_head node;
	struct device *dev;
	struct ima_dev *ima_dev;
	ima_reclaim_callback_t reclaim_callback;
	struct list_head buffer_list;
	spinlock_t lock;
	void *priv;
	atomic_t refcount;
};

struct ima_buffer {
	struct list_head node;
	void *addr;
	unsigned long size;
	struct ima_client *client;
};

enum ima_state {
	IMA_STATE_IDLE = 0,
	IMA_STATE_HOST,
	IMA_STATE_CLIENT,
	IMA_STATE_RECLAIMING,
};

struct ima_dev {
	struct list_head client_list;
	enum ima_state state;
	struct device *dev;
	struct gen_pool *pool;
	spinlock_t lock;
	spinlock_t statelock;
	struct clk *clock;
	atomic_t refcount;
	void __iomem *sysreg_base;
	void __iomem *pre_base;
};

struct ima_client *ima_create_client(struct device *dev,
		ima_reclaim_callback_t reclaim_callback, void *priv);
void ima_destroy_client(struct ima_client *client);
void *ima_alloc(struct ima_client *client, unsigned long size,
						unsigned long flags);
void ima_free(struct ima_client *client, void *vaddr);
phys_addr_t ima_get_dma_addr(struct ima_client *client, void *vaddr);
void ima_host_begin(void);
void ima_host_end(void);
#endif /* __EXYNOS_IMA_H_ */
