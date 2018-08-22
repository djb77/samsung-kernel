/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/freezer.h>

#include "score_log.h"
#include "score_system.h"
#include "score_mmu.h"

enum score_mmu_buffer_type {
	BUF_TYPE_WRONG = 0,
	BUF_TYPE_USERPTR,
	BUF_TYPE_DMABUF
};

enum score_mmu_buffer_state {
	BUF_STATE_NEW = 1,
	BUF_STATE_REGISTERED,
	BUF_STATE_REPLACE,
	BUF_STATE_END
};

#if defined(CONFIG_EXYNOS_SCORE)
static void __score_mmu_context_freelist_add_dmabuf(
		struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	struct score_mmu *mmu;

	score_enter();
	if (kbuf->mirror)
		return;

	mmu = ctx->mmu;
	mutex_lock(&ctx->dbuf_lock);
	if (list_empty(&kbuf->list)) {
		mutex_unlock(&ctx->dbuf_lock);
		score_err("buffer is not included in map list (fd:%d)\n",
				kbuf->m.fd);
		return;
	}

	list_del_init(&kbuf->list);
	ctx->dbuf_count--;
	mutex_unlock(&ctx->dbuf_lock);

	score_mmu_freelist_add(mmu, kbuf);
	score_leave();
}

static void __score_mmu_context_freelist_add(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
	switch (kbuf->type) {
	case BUF_TYPE_USERPTR:
		score_err("This type is not supported yet\n");
		break;
	case BUF_TYPE_DMABUF:
		__score_mmu_context_freelist_add_dmabuf(ctx, kbuf);
		break;
	default:
		score_err("memory type is invalid (%d)\n", kbuf->type);
		break;
	}
	score_leave();
}

#else
static void __score_mmu_context_del_list_dmabuf(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	struct score_mmu *mmu;

	mmu = ctx->mmu;
	mutex_lock(&ctx->dbuf_lock);
	if (list_empty(&kbuf->list)) {
		mutex_unlock(&ctx->dbuf_lock);
		score_err("buffer is not included in map list (fd:%d)\n",
				kbuf->m.fd);
		return;
	}

	list_del_init(&kbuf->list);
	ctx->dbuf_count--;
	mutex_unlock(&ctx->dbuf_lock);

	mmu->mmu_ops->unmap_dmabuf(mmu, kbuf);
}

static void __score_mmu_context_unmap_dmabuf(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
	if (!kbuf->mirror) {
		__score_mmu_context_del_list_dmabuf(ctx, kbuf);
		dma_buf_put(kbuf->dbuf);
	}
	score_leave();
}

static void __score_mmu_context_unmap_buffer(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
	switch (kbuf->type) {
	case BUF_TYPE_USERPTR:
		score_err("This type is not supported yet\n");
		break;
	case BUF_TYPE_DMABUF:
		__score_mmu_context_unmap_dmabuf(ctx, kbuf);
		break;
	default:
		score_err("memory type is invalid (%d)\n", kbuf->type);
		break;
	}
	score_leave();
}
#endif

void score_mmu_unmap_buffer(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
#if defined(CONFIG_EXYNOS_SCORE)
	__score_mmu_context_freelist_add(ctx, kbuf);
#else
	__score_mmu_context_unmap_buffer(ctx, kbuf);
	kfree(kbuf);
#endif
	score_leave();
}

static int __score_mmu_copy_dmabuf(struct score_mmu_buffer *cur_buf,
		struct score_mmu_buffer *list_buf)
{
	int retry = 100;

	score_check();
	while (retry) {
		if (!list_buf->dvaddr) {
			retry--;
			udelay(10);
			continue;
		}
		cur_buf->dbuf = list_buf->dbuf;
		cur_buf->attachment = list_buf->attachment;
		cur_buf->sgt = list_buf->sgt;
		cur_buf->dvaddr = list_buf->dvaddr;
		break;
	}

	if (cur_buf->dvaddr)
		return 0;
	else
		return -EFAULT;
}

static bool __score_mmu_compare_dmabuf(struct score_mmu_buffer *cur_buf,
		struct score_mmu_buffer *list_buf)
{
	struct dma_buf *dbuf, *list_dbuf;

	score_check();
	dbuf = cur_buf->dbuf;
	list_dbuf = list_buf->dbuf;

	if ((dbuf == list_dbuf) &&
			(dbuf->size == list_dbuf->size) &&
			(dbuf->priv == list_dbuf->priv) &&
			(dbuf->file == list_dbuf->file))
		return true;
	else
		return false;
}

static int __score_mmu_context_add_list_dmabuf(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	int ret = BUF_STATE_END;
	struct score_mmu *mmu;
	struct score_mmu_buffer *list_buf, *tbuf;

	score_enter();
	mmu = ctx->mmu;
	mutex_lock(&ctx->dbuf_lock);
	if (!ctx->dbuf_count) {
		list_add_tail(&kbuf->list, &ctx->dbuf_list);
		ctx->dbuf_count++;
		kbuf->mirror = false;
		ret = BUF_STATE_NEW;
		goto p_end;
	}

	list_for_each_entry_safe(list_buf, tbuf, &ctx->dbuf_list, list) {
		if (kbuf->dbuf > list_buf->dbuf) {
			list_add_tail(&kbuf->list, &list_buf->list);
			ctx->dbuf_count++;
			kbuf->mirror = false;
			ret = BUF_STATE_NEW;
			goto p_end;
		} else if (kbuf->dbuf == list_buf->dbuf) {
			if (__score_mmu_compare_dmabuf(kbuf, list_buf)) {
				kbuf->mirror = true;
				ret = BUF_STATE_REGISTERED;
			} else {
				list_replace_init(&list_buf->list,
						&kbuf->list);
				kbuf->mirror = false;
				ret = BUF_STATE_REPLACE;
			}
			goto p_end;
		}
	}

	list_add_tail(&kbuf->list, &ctx->dbuf_list);
	ctx->dbuf_count++;
	kbuf->mirror = false;
	ret = BUF_STATE_NEW;
p_end:
	mutex_unlock(&ctx->dbuf_lock);

	if (ret == BUF_STATE_NEW) {
		ret = mmu->mmu_ops->map_dmabuf(mmu, kbuf);
	} else if (ret == BUF_STATE_REGISTERED) {
		dma_buf_put(kbuf->dbuf);
		ret = __score_mmu_copy_dmabuf(kbuf, list_buf);
	} else if (ret == BUF_STATE_REPLACE) {
		score_mmu_freelist_add(mmu, list_buf);
		ret = mmu->mmu_ops->map_dmabuf(mmu, kbuf);
	} else {
		score_err("state(%d) is invalid for mapping buffer\n", ret);
		ret = -EINVAL;
	}

	if (ret) {
		mutex_lock(&ctx->dbuf_lock);
		list_del_init(&kbuf->list);
		ctx->dbuf_count--;
		mutex_unlock(&ctx->dbuf_lock);
	}

	score_leave();
	return ret;
}

static int __score_mmu_context_map_dmabuf(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	int ret = 0;
	struct score_mmu *mmu;
	struct dma_buf *dbuf;

	score_enter();
	if (kbuf->m.fd <= 0) {
		score_err("fd(%d) is invalid\n", kbuf->m.fd);
		ret = -EINVAL;
		goto p_err;
	}

	dbuf = dma_buf_get(kbuf->m.fd);
	if (IS_ERR_OR_NULL(dbuf)) {
		score_err("dma_buf is invalid (fd:%d)\n", kbuf->m.fd);
		ret = -EINVAL;
		goto p_err;
	}
	kbuf->dbuf = dbuf;

	if (dbuf->size < kbuf->size) {
		score_err("user buffer size is small(%zd, %zd)\n",
				dbuf->size, kbuf->size);
		ret = -EINVAL;
		goto p_err_size;
	} else if (kbuf->size != dbuf->size) {
		kbuf->size = dbuf->size;
	}

	mmu = ctx->mmu;
	ret = __score_mmu_context_add_list_dmabuf(ctx, kbuf);
	if (ret)
		goto p_err_list;

	score_leave();
	return ret;
p_err_list:
p_err_size:
	dma_buf_put(dbuf);
p_err:
	return ret;
}

static int __score_mmu_context_map_buffer(struct score_mmu_context *ctx,
		struct score_mmu_buffer *kbuf)
{
	int ret;

	score_enter();
	switch (kbuf->type) {
	case BUF_TYPE_USERPTR:
		ret = -EINVAL;
		score_err("This type is not supported yet\n");
		goto p_err;
	case BUF_TYPE_DMABUF:
		ret = __score_mmu_context_map_dmabuf(ctx, kbuf);
		break;
	default:
		ret = -EINVAL;
		score_err("memory type is invalid (%d)\n", kbuf->type);
		goto p_err;
	}

	score_leave();
p_err:
	return ret;
}

void *score_mmu_map_buffer(struct score_mmu_context *ctx,
		struct score_host_buffer *buf)
{
	int ret;
	struct score_mmu_buffer *kbuf;

	score_enter();
	kbuf = kzalloc(sizeof(*kbuf), GFP_KERNEL);
	if (!kbuf) {
		ret = -ENOMEM;
		score_err("Fail to alloc mmu buffer\n");
		goto p_err;
	}

	kbuf->type = buf->memory_type;
	kbuf->size = buf->memory_size;
	kbuf->m.mem_info = buf->m.mem_info;

	ret = __score_mmu_context_map_buffer(ctx, kbuf);
	if (ret)
		goto p_err_map;

	score_leave();
	return kbuf;
p_err_map:
	kfree(kbuf);
p_err:
	return ERR_PTR(ret);
}

void *score_mmu_create_context(struct score_mmu *mmu)
{
	int ret;
	struct score_mmu_context *ctx;

	score_enter();
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		score_err("Fail to alloc mmu context\n");
		goto p_err;
	}
	ctx->mmu = mmu;

	INIT_LIST_HEAD(&ctx->dbuf_list);
	ctx->dbuf_count = 0;
	mutex_init(&ctx->dbuf_lock);

	INIT_LIST_HEAD(&ctx->userptr_list);
	ctx->userptr_count = 0;
	mutex_init(&ctx->userptr_lock);

	score_leave();
	return ctx;
p_err:
	return ERR_PTR(ret);
}

void score_mmu_destroy_context(void *alloc_ctx)
{
	struct score_mmu_context *ctx;
	struct score_mmu_buffer *buf, *tbuf;

	score_enter();
	ctx = alloc_ctx;

	list_for_each_entry_safe(buf, tbuf, &ctx->userptr_list, list) {
		score_mmu_unmap_buffer(ctx, buf);
	}
	list_for_each_entry_safe(buf, tbuf, &ctx->dbuf_list, list) {
		score_mmu_unmap_buffer(ctx, buf);
	}

	mutex_destroy(&ctx->userptr_lock);
	mutex_destroy(&ctx->dbuf_lock);
	kfree(ctx);
	score_leave();
}

int score_mmu_packet_prepare(struct score_mmu *mmu,
		struct score_mmu_packet *packet)
{
	return score_memory_kmap_packet(&mmu->mem, packet);
}

void score_mmu_packet_unprepare(struct score_mmu *mmu,
		struct score_mmu_packet *packet)
{
	return score_memory_kunmap_packet(&mmu->mem, packet);
}

void score_mmu_freelist_add(struct score_mmu *mmu,
		struct score_mmu_buffer *kbuf)
{
	score_enter();
	spin_lock(&mmu->free_slock);
	list_add_tail(&kbuf->list, &mmu->free_list);
	mmu->free_count++;
	spin_unlock(&mmu->free_slock);
	wake_up(&mmu->waitq);
	score_leave();
}

/**
 * score_mmu_get_internal_mem_kvaddr - Get kvaddr of
 *	kernel space memory allocated for SCore device
 * @mem:	[in]	object about score_memory structure
 * @kvaddr:	[out]	kernel virtual address
 */
void *score_mmu_get_internal_mem_kvaddr(struct score_mmu *mmu)
{
	score_check();
	return score_memory_get_internal_mem_kvaddr(&mmu->mem);
}

/**
 * score_mmu_get_internal_mem_dvaddr - Get  dvaddr of
 *	kernel space memory allocated for SCore device
 * @mem:	[in]	object about score_memory structure
 * @dvaddr:	[out]	device virtual address
 */
dma_addr_t score_mmu_get_internal_mem_dvaddr(struct score_mmu *mmu)
{
	score_check();
	return score_memory_get_internal_mem_dvaddr(&mmu->mem);
}

/**
 * score_mmu_get_profiler_kvaddr - Get kvaddr of
 *	kernel space memory allocated for SCore device
 * @mem:	[in]	object about score_memory structure
 * @id:         [in]    id of requested core memory
 * @kvaddr:	[out]	kernel virtual address
 */
void *score_mmu_get_profiler_kvaddr(struct score_mmu *mmu, unsigned int id)
{
	score_check();
	return score_memory_get_profiler_kvaddr(&mmu->mem, id);
}

/**
 * score_mmu_get_print_kvaddr - Get kvaddr and dvaddr of
 *	kernel space memory allocated for SCore device
 * @mem:	[in]	object about score_memory structure
 * @kvaddr:	[out]	kernel virtual address
 */
void *score_mmu_get_print_kvaddr(struct score_mmu *mmu)
{
	score_check();
	return score_memory_get_print_kvaddr(&mmu->mem);
}

void score_mmu_init(struct score_mmu *mmu)
{
	score_memory_init(&mmu->mem);
}

int score_mmu_open(struct score_mmu *mmu)
{
	int ret = 0;

	score_enter();
	ret = score_memory_open(&mmu->mem);
	score_leave();
	return ret;
}

void score_mmu_close(struct score_mmu *mmu)
{
	score_enter();
	score_memory_close(&mmu->mem);
	score_leave();
}

static int score_mmu_free(void *data)
{
	struct score_mmu *mmu;
	struct score_mmu_buffer *buffer;

	score_enter();
	mmu = data;
	set_freezable();
	while (true) {
		wait_event_freezable(mmu->waitq,
				mmu->free_count > 0 || kthread_should_stop());
		if (kthread_should_stop())
			break;

		spin_lock(&mmu->free_slock);
		if (list_empty(&mmu->free_list)) {
			spin_unlock(&mmu->free_slock);
			continue;
		}

		buffer = list_first_entry(&mmu->free_list,
				struct score_mmu_buffer,
				list);
		list_del(&buffer->list);
		mmu->free_count--;
		spin_unlock(&mmu->free_slock);

		mmu->mmu_ops->unmap_dmabuf(mmu, buffer);
		dma_buf_put(buffer->dbuf);
		kfree(buffer);
	}

	score_leave();
	return 0;
}

static int __score_mmu_freelist_init(struct score_mmu *mmu)
{
	int ret = 0;
	struct sched_param param = { .sched_priority = 0 };

	INIT_LIST_HEAD(&mmu->free_list);
	spin_lock_init(&mmu->free_slock);
	mmu->free_count = 0;

	init_waitqueue_head(&mmu->waitq);
	mmu->task = kthread_run(score_mmu_free, mmu, "score_mmu_free");
	if (IS_ERR_OR_NULL(mmu->task)) {
		ret = PTR_ERR(mmu->task);
		score_err("creating thread for mmu free failed(%d)\n", ret);
		goto p_end;
	}
	sched_setscheduler(mmu->task, SCHED_IDLE, &param);
p_end:
	return ret;
}

int score_mmu_probe(struct score_system *system)
{
	int ret = 0;
	struct score_mmu *mmu;

	score_enter();
	mmu = &system->mmu;
	mmu->system = system;

	ret = score_memory_probe(mmu);
	if (ret)
		goto p_end;

	ret = __score_mmu_freelist_init(mmu);
	if (ret)
		goto p_err_freelist;

	score_leave();
	return ret;
p_err_freelist:
	score_memory_remove(&mmu->mem);
p_end:
	return ret;
}

static void __score_mmu_freelist_deinit(struct score_mmu *mmu)
{
	wake_up(&mmu->waitq);
	kthread_stop(mmu->task);
}

void score_mmu_remove(struct score_mmu *mmu)
{
	score_enter();
	__score_mmu_freelist_deinit(mmu);
	score_memory_remove(&mmu->mem);
	score_leave();
}
