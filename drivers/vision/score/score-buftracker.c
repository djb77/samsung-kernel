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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/exynos_iovmm.h>
#include <linux/pm_runtime.h>

#include "score-buftracker.h"
#include "score-vertex.h"
#include "score-device.h"

void score_buftracker_dump(struct score_buftracker *buftracker)
{
	struct score_memory_buffer *buf, *temp_buf;

	if (buftracker->buf_cnt > 0) {
		list_for_each_entry_safe(buf, temp_buf,
				&buftracker->buf_list, list) {
			score_info("buf_cnt:%d fd(%d) dvaddr(0x%llx) (%p, %p, %p) size(%ld) \n",
					buftracker->buf_cnt, buf->m.fd, buf->dvaddr,
					buf->mem_priv, buf->cookie, buf->dbuf,
					buf->size);
#ifdef BUF_MAP_KVADDR
			score_info("fd(%d) - kvaddr(%p) \n", buf->m.fd, buf->kvaddr);
#endif
		}
	}
	if (buftracker->userptr_cnt > 0) {
		list_for_each_entry_safe(buf, temp_buf,
				&buftracker->userptr_list, list) {
			score_info("buf_cnt:%d userptr(%lx) dvaddr(0x%llx) (%p, %p, %p) size(%ld) \n",
					buftracker->userptr_cnt, buf->m.userptr, buf->dvaddr,
					buf->mem_priv, buf->cookie, buf->dbuf,
					buf->size);
#ifdef BUF_MAP_KVADDR
			score_info("fd(%d) - kvaddr(%p) \n", buf->m.fd, buf->kvaddr);
#endif
		}
	}
}

static int __score_buftracker_add_list_dmabuf(struct score_buftracker *buftracker,
		struct score_memory_buffer *buf, struct score_memory_buffer **listed_buf)
{
	unsigned long flag;
	struct score_memory_buffer *target_buf, *temp_buf, *prev_buf;
	int ret = 0;

	spin_lock_irqsave(&buftracker->buf_lock, flag);
	if (buftracker->buf_cnt > 0) {
		list_for_each_entry_safe(target_buf, temp_buf, &buftracker->buf_list, list) {
			if (target_buf->m.fd < buf->m.fd) {
				prev_buf = container_of(target_buf->list.prev,
						struct score_memory_buffer, list);
				list_add(&buf->list, &prev_buf->list);
				buftracker->buf_cnt++;
				ret = SCORE_BUFTRACKER_SEARCH_STATE_INSERT;
				goto p_err;
			} else if (target_buf->m.fd == buf->m.fd) {
				*listed_buf = target_buf;
				ret = SCORE_BUFTRACKER_SEARCH_STATE_ALREADY_REGISTERED;
				goto p_err;
			}
		}
		list_add_tail(&buf->list, &buftracker->buf_list);
		buftracker->buf_cnt++;
		ret = SCORE_BUFTRACKER_SEARCH_STATE_INSERT;
	} else {
		list_add_tail(&buf->list, &buftracker->buf_list);
		buftracker->buf_cnt++;
		ret = SCORE_BUFTRACKER_SEARCH_STATE_NEW;
	}
p_err:
	spin_unlock_irqrestore(&buftracker->buf_lock, flag);

	return ret;
}

int score_buftracker_add_dmabuf(struct score_buftracker *buftracker,
		struct score_memory_buffer *buf)
{
	int ret = 0;
	int buf_ret;
	struct score_vertex_ctx *vctx;
	struct score_memory *memory;
	struct score_memory_buffer *listed_buf;
	unsigned long flag;

	vctx = container_of(buftracker, struct score_vertex_ctx, buftracker);
	memory = vctx->memory;

	/* add list to compare */
	buf_ret = __score_buftracker_add_list_dmabuf(buftracker, buf, &listed_buf);

	/* check buffer when fd is equal */
	if (buf_ret == SCORE_BUFTRACKER_SEARCH_STATE_ALREADY_REGISTERED) {
		if (score_memory_compare_dmabuf(buf, listed_buf)) {
			score_memory_unmap_dmabuf(memory, listed_buf);
			buf_ret = SCORE_BUFTRACKER_SEARCH_STATE_INSERT;
		} else {
			memcpy(buf, listed_buf, sizeof(struct score_memory_buffer));
		}
		spin_lock_irqsave(&buftracker->buf_lock, flag);
		list_replace(&listed_buf->list, &buf->list);
		spin_unlock_irqrestore(&buftracker->buf_lock, flag);
		kfree(listed_buf);
	}

	/* if not exist buffer in list, completing this job */
	if ((buf_ret == SCORE_BUFTRACKER_SEARCH_STATE_NEW) ||
			(buf_ret == SCORE_BUFTRACKER_SEARCH_STATE_INSERT)) {
		ret = score_memory_map_dmabuf(memory, buf);
		if (ret) {
			struct score_memory_buffer *target_buf, *temp_buf;
			spin_lock_irqsave(&buftracker->buf_lock, flag);
			list_for_each_entry_safe(target_buf, temp_buf,
					&buftracker->buf_list, list) {
				if (target_buf->m.fd == buf->m.fd) {
					list_del(&buf->list);
					buftracker->buf_cnt--;
					break;
				}
			}
			spin_unlock_irqrestore(&buftracker->buf_lock, flag);
		}
	} else if (buf_ret != SCORE_BUFTRACKER_SEARCH_STATE_ALREADY_REGISTERED) {
		ret = -EINVAL;
		score_err("buf_ret about buftracker list is invalid (%d) \n", buf_ret);
	}

	return ret;
}

int score_buftracker_remove_dmabuf(struct score_buftracker *buftracker,
	struct score_memory_buffer *buf)
{
	int ret = 0;
	struct score_vertex_ctx *vctx;
	struct score_memory *memory;
	unsigned long flag;

	vctx = container_of(buftracker, struct score_vertex_ctx, buftracker);
	memory = vctx->memory;

	score_memory_unmap_dmabuf(memory, buf);

	spin_lock_irqsave(&buftracker->buf_lock, flag);
	list_del(&buf->list);
	buftracker->buf_cnt--;
	spin_unlock_irqrestore(&buftracker->buf_lock, flag);
	kfree(buf);

	return ret;
}

int score_buftracker_remove_dmabuf_all(struct score_buftracker *buftracker)
{
	int ret = 0;
	struct score_memory_buffer *buf;

	while (buftracker->buf_cnt > 0) {
		buf = container_of(buftracker->buf_list.next,
				struct score_memory_buffer, list);
		score_buftracker_remove_dmabuf(buftracker, buf);
	}

	return ret;
}

int score_buftracker_add_userptr(struct score_buftracker *buftracker,
		struct score_memory_buffer *buf)
{
	int ret = 0;
	unsigned long flags;
	struct score_vertex_ctx *vctx;
	struct score_memory *memory;

	vctx = container_of(buftracker, struct score_vertex_ctx, buftracker);
	memory = vctx->memory;

	ret = score_memory_map_userptr(memory, buf);
	if (ret) {
		goto p_err;
	}

	spin_lock_irqsave(&buftracker->userptr_lock, flags);
	list_add_tail(&buf->list, &buftracker->userptr_list);
	buftracker->userptr_cnt++;
	spin_unlock_irqrestore(&buftracker->userptr_lock, flags);

p_err:
	return ret;
}

int score_buftracker_remove_userptr(struct score_buftracker *buftracker,
	struct score_memory_buffer *buf)
{
	int ret = 0;
	struct score_vertex_ctx *vctx;
	struct score_memory *memory;
	unsigned long flags;

	vctx = container_of(buftracker, struct score_vertex_ctx, buftracker);
	memory = vctx->memory;

	score_memory_unmap_userptr(memory, buf);

	spin_lock_irqsave(&buftracker->userptr_lock, flags);
	list_del(&buf->list);
	buftracker->userptr_cnt--;
	spin_unlock_irqrestore(&buftracker->userptr_lock, flags);
	kfree(buf);

	return ret;
}

int score_buftracker_remove_userptr_all(struct score_buftracker *buftracker)
{
	int ret = 0;
	struct score_memory_buffer *buf;

	while (buftracker->userptr_cnt > 0) {
		buf = container_of(buftracker->userptr_list.next,
				struct score_memory_buffer, list);
		score_buftracker_remove_userptr(buftracker, buf);
	}

	return ret;
}

void score_buftracker_invalid_or_flush_userptr_fid_all(struct score_buftracker *buftracker,
		int fid, int sync_for, enum dma_data_direction dir)
{
	struct score_memory_buffer *buf;
	struct score_memory_buffer *temp_buf;

	list_for_each_entry_safe(buf, temp_buf,
			&buftracker->userptr_list, list) {
		if (buf->fid == fid) {
			score_memory_invalid_or_flush_userptr(buf, sync_for, dir);
		}
	}
}

void score_buftracker_invalid_or_flush_userptr_all(struct score_buftracker *buftracker,
		int sync_for, enum dma_data_direction dir)
{
	struct score_memory_buffer *buf;
	struct score_memory_buffer *temp_buf;

	list_for_each_entry_safe(buf, temp_buf,
			&buftracker->userptr_list, list) {
		score_memory_invalid_or_flush_userptr(buf, sync_for, dir);
	}
}

int score_buftracker_init(struct score_buftracker *buftracker)
{
	int ret = 0;

	buftracker->buf_cnt = 0;
	INIT_LIST_HEAD(&buftracker->buf_list);
	spin_lock_init(&buftracker->buf_lock);

	buftracker->userptr_cnt = 0;
	INIT_LIST_HEAD(&buftracker->userptr_list);
	spin_lock_init(&buftracker->userptr_lock);

	return ret;
}

int score_buftracker_deinit(struct score_buftracker *buftracker)
{
	int ret = 0;

	score_buftracker_remove_dmabuf_all(buftracker);
	score_buftracker_remove_userptr_all(buftracker);

	return ret;
}
