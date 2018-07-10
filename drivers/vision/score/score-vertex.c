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
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <video/videonode.h>
#include <media/exynos_mc.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/bug.h>

#include "score-config.h"
#include "vision-ioctl.h"
#include "score-vertex.h"
#include "vision-dev.h"
#include "score-device.h"
#include "score-control.h"
#include "score-debug.h"
#include "score-utils.h"
#include "score-regs-user-def.h"

static int __vref_open(struct score_vertex *vertex)
{
	struct score_device *device = container_of(vertex, struct score_device, vertex);
	return score_device_open(device);
}

static int __vref_close(struct score_vertex *vertex)
{
	struct score_device *device = container_of(vertex, struct score_device, vertex);
	return score_device_close(device);
}

static inline void __vref_init(struct score_vertex_refcount *vref,
	struct score_vertex *vertex,
	int (*first)(struct score_vertex *vertex),
	int (*final)(struct score_vertex *vertex))
{
	vref->vertex = vertex;
	vref->first = first;
	vref->final = final;
	atomic_set(&vref->refcount, 0);
}

static inline int __vref_get(struct score_vertex_refcount *vref)
{
	return (atomic_inc_return(&vref->refcount) == 1) ? vref->first(vref->vertex) : 0;
}

static inline int __vref_put(struct score_vertex_refcount *vref)
{
	return (atomic_dec_return(&vref->refcount) == 0) ? vref->final(vref->vertex) : 0;
}

static int __score_buf_unmap_dmabuf(struct score_memory *mem,
		struct vb_buffer *kbuf)
{
	int ret = 0;
	const struct vb2_mem_ops *mem_ops = mem->vb2_mem_ops;

	/*
	if (kbuf->cookie) {
		int size = kbuf->roi.w * kbuf->roi.h;
		vb2_ion_sync_for_cpu(kbuf->cookie, 0, size, DMA_FROM_DEVICE);
	}
	*/

	if (kbuf->dvaddr)
		mem_ops->unmap_dmabuf(kbuf->mem_priv);

	if (kbuf->mem_priv)
		mem_ops->detach_dmabuf(kbuf->mem_priv);

	if (kbuf->dbuf)
		dma_buf_put(kbuf->dbuf);

	kbuf->dbuf = NULL;
	kbuf->mem_priv = NULL;
	kbuf->cookie = NULL;
	kbuf->kvaddr = NULL;
	kbuf->dvaddr = 0;

	return ret;
}

static int __score_buf_map_dmabuf(struct score_memory *mem,
		struct vb_buffer *kbuf)
{
	int ret = 0;
	struct dma_buf *dbuf;
	void *mem_priv, *cookie;
	dma_addr_t dvaddr;
	int size;
	const struct vb2_mem_ops *mem_ops;

	mem_ops = mem->vb2_mem_ops;

	if (kbuf->m.fd <= 0) {
		score_err("fd(%d) is invalid\n", kbuf->m.fd);
		ret = -EINVAL;
		goto p_err;
	}

	dbuf = dma_buf_get(kbuf->m.fd);
	if (IS_ERR_OR_NULL(dbuf)) {
		score_err("invalid dmabuf fd\n");
		ret = -EINVAL;
		goto p_err;
	}

	kbuf->dbuf = dbuf;

	size = kbuf->roi.w * kbuf->roi.h;
	if (dbuf->size < size) {
		score_err("user buffer size is small(%zd, %d)\n", dbuf->size, size);
		ret = -EINVAL;
		goto p_err;
	} else if (dbuf->size != size) {
		size = dbuf->size;
		kbuf->roi.w = size;
		kbuf->roi.h = 1;
	}

	mem_priv = mem_ops->attach_dmabuf(mem->alloc_ctx, dbuf, size, DMA_TO_DEVICE);
	if (IS_ERR(mem_priv)) {
		score_err("attach_dmabuf is fail(%p, %d)\n", mem->alloc_ctx, size);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	kbuf->mem_priv = mem_priv;

	cookie = mem_ops->cookie(mem_priv);
	if (IS_ERR_OR_NULL(cookie)) {
		score_err("cookie is fail(%p, %p)\n", mem_priv, cookie);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	kbuf->cookie = cookie;

	ret = mem_ops->map_dmabuf(mem_priv);
	if (ret) {
		score_err("map_dmabuf is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vb2_ion_dma_address(cookie, &dvaddr);
	if (ret) {
		score_err("vb2_ion_dma_address is fail(%d)\n", ret);
		goto p_err;
	}

	kbuf->dvaddr = dvaddr;

#ifdef VISION_MAP_KVADDR
	{
		void *kvaddr = NULL;

		kvaddr = mem_ops->vaddr(mem_priv);
		if (IS_ERR_OR_NULL(kvaddr)) {
			score_err("vaddr is fail(%p)\n", mem_priv);
			ret = PTR_ERR(kvaddr);
			goto p_err;
		}

		kbuf->kvaddr = kvaddr;
	}
#endif
	//vb2_ion_sync_for_device(cookie, 0, size, DMA_TO_DEVICE);

	return 0;
p_err:
	__score_buf_unmap_dmabuf(mem, kbuf);
	return ret;
}

static int __score_buf_unmap_virtptr(struct score_memory *mem,
		struct vb_buffer *kbuf)
{
	int ret = 0;
	struct score_ipc_packet *packet = NULL;
	int size;

	size = kbuf->roi.w * kbuf->roi.h;
	packet = (struct score_ipc_packet *)kbuf->m.userptr;
	kbuf->m.userptr = kbuf->reserved;
	if (!packet) {
		score_err("packet was not allocated\n");
		ret = -ENOMEM;
		goto p_err;
	}

	ret = copy_to_user((void *)kbuf->m.userptr, packet, size);
	if (ret) {
		score_err("copy_to_user() is fail(%d)\n", ret);
	}
	kbuf->reserved = 0;

p_err:
	if (packet) {
		kfree(packet);
		kbuf->m.userptr = 0;
	}
	return ret;
}

static int __score_buf_map_virtptr(struct score_memory *mem,
		struct vb_buffer *kbuf)
{
	int ret = 0;
	void *buf = NULL;
	int size;

	size = kbuf->roi.w * kbuf->roi.h;
	buf = kmalloc(size, GFP_KERNEL);
	if (!buf) {
		score_err("kmalloc is fail\n");
		ret = -ENOMEM;
		goto p_err;
	}

	ret = copy_from_user(buf, (void *)kbuf->m.userptr, size);
	if (ret) {
		score_err("copy_from_user() is fail(%d)\n", ret);
		goto p_err;
	}

	/* back up - userptr */
	kbuf->reserved = kbuf->m.userptr;
	kbuf->m.userptr = (unsigned long)buf;

	return ret;
p_err:
	if (buf) {
		kfree(buf);
	}
	return ret;
}

static int __score_buf_prepare(struct score_vertex_ctx *vctx,
		struct score_frame *frame, struct vs4l_container_list *clist)
{
	int ret = 0;
	struct score_memory *memory;
	struct vs4l_container *container;
	struct vs4l_buffer *userbuf;
	struct vb_buffer *kbuf;

	memory = vctx->memory;
	clist->index = frame->fid;
	container = clist->containers;
	userbuf = &container[0].buffers[0];
	kbuf = &frame->buffer;

	kbuf->roi = userbuf->roi;
	kbuf->m.userptr = userbuf->m.userptr;
	kbuf->dbuf = NULL;
	kbuf->mem_priv = NULL;
	kbuf->cookie = NULL;
	kbuf->kvaddr = NULL;
	kbuf->dvaddr = 0;
	kbuf->reserved = 0;

	switch (container->memory) {
		case VS4L_MEMORY_DMABUF:
			ret = __score_buf_map_dmabuf(memory, kbuf);
			if (ret) {
				goto p_err;
			}
			break;
		case VS4L_MEMORY_VIRTPTR:
			ret = __score_buf_map_virtptr(memory, kbuf);
			if (ret) {
				goto p_err;
			}
			break;
		default:
			score_err("unsupported memory type\n");
			ret = -EINVAL;
			goto p_err;
	}
	return ret;
p_err:
	kbuf->m.userptr = 0;
	return ret;
}

static int __score_buf_unprepare(struct score_vertex_ctx *vctx,
		struct score_frame *frame, struct vs4l_container_list *clist)
{
	int ret = 0;
	struct score_memory *memory;
	struct vs4l_container *container;
	struct vs4l_buffer *userbuf;
	struct vb_buffer *kbuf;
	struct score_ipc_packet *packet;

	memory = vctx->memory;
	container = clist->containers;
	userbuf = &container[0].buffers[0];
	kbuf = &frame->buffer;

	switch (container->memory) {
		case VS4L_MEMORY_DMABUF:
			ret = __score_buf_unmap_dmabuf(memory, kbuf);
			if (ret) {
				goto p_err;
			}
			break;
		case VS4L_MEMORY_VIRTPTR:
			packet = (struct score_ipc_packet *)kbuf->m.userptr;
			if (frame->message != SCORE_FRAME_DONE) {
				packet->group[0].data.params[0] = frame->message;
			}
#ifdef ENABLE_TARGET_TIME
			packet->group[0].data.params[1] = (unsigned int)frame->ts[0].tv_sec;
			packet->group[0].data.params[2] = (unsigned int)frame->ts[0].tv_nsec;
			packet->group[0].data.params[3] = (unsigned int)frame->ts[1].tv_sec;
			packet->group[0].data.params[4] = (unsigned int)frame->ts[1].tv_nsec;
#else
			packet->group[0].data.params[1] = 0;
			packet->group[0].data.params[2] = 0;
			packet->group[0].data.params[3] = 0;
			packet->group[0].data.params[4] = 0;
#endif

			ret = __score_buf_unmap_virtptr(memory, kbuf);
			if (ret) {
				goto p_err;
			}
			break;
		default:
			score_err("unsupported memory type\n");
			ret = -EINVAL;
			goto p_err;
	}

p_err:
	return ret;
}

static int score_vctx_get(struct score_vertex_ctx *vctx)
{
	return (atomic_inc_return(&vctx->refcount) == 1) ? CALL_VOPS(vctx, start) : 0;
}

static int score_vctx_put(struct score_vertex_ctx *vctx)
{
	return (atomic_dec_return(&vctx->refcount) == 0) ? CALL_VOPS(vctx, stop) : 0;
}

static int score_vctx_start(struct score_vertex_ctx *vctx)
{
	int ret = 0;

	vctx->state = BIT(SCORE_VERTEX_START);

	return ret;
}

static int score_vctx_stop(struct score_vertex_ctx *vctx)
{
	int ret = 0;
	unsigned long flags;
	struct score_framemgr *framemgr;
	struct score_framemgr *iframemgr;
	struct score_buftracker *buftracker;

	framemgr = &vctx->framemgr;
	iframemgr = vctx->iframemgr;

	spin_lock_irqsave(&iframemgr->slock, flags);
	score_frame_all_flush(framemgr);
	spin_unlock_irqrestore(&iframemgr->slock, flags);

	wake_up_all(&framemgr->done_wq);

	buftracker = &vctx->buftracker;
	score_buftracker_remove_userptr_all(buftracker);

	vctx->state = BIT(SCORE_VERTEX_STOP);

	return ret;
}

static int score_vctx_queue(struct score_vertex_ctx *vctx,
		struct vs4l_container_list *clist)
{
	int ret = 0;
	struct score_framemgr *framemgr;
	struct score_frame *frame;
	struct score_framemgr *iframemgr;
	struct score_frame *iframe;

	framemgr = &vctx->framemgr;
	iframemgr = vctx->iframemgr;

	frame = score_frame_create(framemgr);
	if (!frame) {
		score_verr("frame is not created \n", vctx);
		ret = -ENOMEM;
		goto p_err;
	} else {
		iframe = score_frame_create(iframemgr);
		if (!iframe) {
			score_ferr("iframe is not created \n", frame);
			ret = -ENOMEM;
			goto p_err_frame;
		}
		frame->frame = iframe;
		iframe->frame = frame;
		iframe->vid = frame->vid;
		iframe->fid = frame->fid;
	}

	ret = __score_buf_prepare(vctx, frame, clist);
	if (ret)
		goto p_err_iframe;

	ret = score_vertexmgr_queue(vctx->vertexmgr, frame);
	if (ret)
		goto p_err_iframe;

	return ret;
p_err_iframe:
	score_frame_destroy(iframe);
p_err_frame:
	score_frame_destroy(frame);
p_err:
	return ret;
}

static int __score_vctx_wait_for_done(struct score_vertex_ctx *vctx,
		struct vs4l_container_list *clist, struct score_frame **frame)
{
	int ret = 0;
	struct score_framemgr *framemgr;
	struct score_frame *cur_frame;
	unsigned int retry = 10, idx;

	framemgr = &vctx->framemgr;
	score_frame_g_entire(framemgr, vctx->id, clist->index, &cur_frame);
	if (!cur_frame) {
		score_verr("No frame matching with id (clist:%d)\n",
				vctx, clist->index);
		return -ENODATA;
	}

	if (vctx->blocking) {
		score_vwarn("Will not wait as nonblocking. \n", vctx);
		ret = -EINVAL;
		goto p_err;
	}

	mutex_unlock(&vctx->lock);

	for (idx = 0; idx < retry; ++idx) {
		if (!score_frame_done(cur_frame)) {
			ret = wait_event_interruptible_timeout(framemgr->done_wq,
					score_frame_done(cur_frame),
					msecs_to_jiffies(SCORE_TIMEOUT));
			if (!ret) {
				struct score_vertex *vertex;
				struct score_device *device;
				struct score_system *system;

				vertex = vctx->vertex;
				device = container_of(vertex, struct score_device, vertex);
				system = &device->system;
				score_verr("Wait time is over (limit:%d ms)\n",
						vctx, SCORE_TIMEOUT);
				score_dump_sfr(system->regs);
				ret = -ETIMEDOUT;
				break;
			} else if (ret > 0) {
				ret = 0;
				break;
			} else {
				udelay(1);
			}
		} else {
			ret = 0;
			break;
		}
	}

	mutex_lock(&vctx->lock);
p_err:
	*frame = cur_frame;
	return ret;
}

static int score_vctx_deque(struct score_vertex_ctx *vctx,
		struct vs4l_container_list *clist)
{
	int ret = 0;
	unsigned long flags;
	struct score_frame *frame = NULL;
	struct score_frame *iframe = NULL;

	struct score_framemgr *iframemgr;
	struct score_vertex *vertex;
	struct score_device *device;
	struct score_system *system;

	iframemgr = vctx->iframemgr;
	ret = __score_vctx_wait_for_done(vctx, clist, &frame);
	if (!frame) {
		goto p_err;
	} else if (ret) {
		vertex = vctx->vertex;
		device = container_of(vertex, struct score_device, vertex);
		system = &device->system;
		iframe = frame->frame;

		spin_lock_irqsave(&iframemgr->slock, flags);
		if (iframe->state == SCORE_FRAME_STATE_READY) {
			score_frame_trans_rdy_to_com(iframe);
		} else if (iframe->state == SCORE_FRAME_STATE_PENDING) {
			score_frame_trans_pend_to_com(iframe);
		} else if (iframe->state == SCORE_FRAME_STATE_PROCESS) {
			if (!score_system_compare_run_state(system, frame)) {
				score_frame_trans_pro_to_com(iframe);
				score_system_reset(system, RESET_OPT_CHECK |
						RESET_OPT_RESTART);
			} else {
				score_system_reset(system, RESET_OPT_FORCE |
						RESET_SFR_CLEAN | RESET_OPT_RESTART);
			}
		}
		iframe->message = SCORE_FRAME_FAIL;
		frame->message = ret;
		spin_unlock_irqrestore(&iframemgr->slock, flags);
		sysfs_debug_count_add(ERROR_CNT_TIMEOUT);
	} else {
		iframe = frame->frame;
		if (frame->message == SCORE_FRAME_FAIL) {
			frame->message = -ENODATA;
			sysfs_debug_count_add(ERROR_CNT_RET_FAIL);
		}
	}

	ret = __score_buf_unprepare(vctx, frame, clist);

	score_vertexmgr_queue_pending_frame(vctx->vertexmgr);

	score_frame_destroy(iframe);
	score_frame_destroy(frame);
p_err:
	return ret;
}

const struct score_vctx_ops score_vctx_ops = {
	.get		= score_vctx_get,
	.put		= score_vctx_put,
	.start		= score_vctx_start,
	.stop		= score_vctx_stop,
	.queue		= score_vctx_queue,
	.deque		= score_vctx_deque,
};

static struct score_vertex_ctx *score_vctx_create(struct score_vertex *vertex,
		struct score_memory *memory)
{
	struct score_vertex_ctx *vctx;
	struct score_buftracker *buftracker;

	vctx = kzalloc(sizeof(struct score_vertex_ctx), GFP_KERNEL);
	if (!vctx) {
		return NULL;
	}

	vctx->vertex = vertex;
	vctx->memory = memory;
	vctx->vops = &score_vctx_ops;
	mutex_init(&vctx->lock);
	atomic_set(&vctx->refcount, 0);

	buftracker = &vctx->buftracker;
	score_buftracker_init(buftracker);

	return vctx;
}

static int score_vctx_destroy(struct score_vertex_ctx *vctx)
{
	int ret = 0;
	struct score_buftracker *buftracker;
	int timeout = 0;

	while (timeout < 1000) {
		if (vctx->state & BIT(SCORE_VERTEX_STOP)) {
			break;
		}
		timeout++;
		udelay(1);
	}
	if (vctx->state & BIT(SCORE_VERTEX_START)) {
		CALL_VOPS(vctx, stop);
	}

	buftracker = &vctx->buftracker;
	score_buftracker_deinit(buftracker);

	kfree(vctx);

	return ret;
}

static int score_vertex_s_graph(struct file *file, struct vs4l_graph *graph)
{
	int ret = 0;
	return ret;
}

static int score_vertex_s_format(struct file *file, struct vs4l_format_list *flist)
{
	int ret = 0;
	return ret;
}

static int score_vertex_s_param(struct file *file, struct vs4l_param_list *plist)
{
	int ret = 0;
	return ret;
}

static int score_vertex_s_ctrl(struct file *file, struct vs4l_ctrl *ctrl)
{
	int ret = 0;

	struct score_vertex_ctx *vctx = file->private_data;
	struct mutex *lock = &vctx->lock;

	struct score_vertex *vertex = vctx->vertex;
	struct score_device *device = container_of(vertex, struct score_device, vertex);
	struct score_system *system = &device->system;

	if (mutex_lock_interruptible(lock)) {
		score_verr("mutex_lock_interruptible is fail \n", vctx);
		return -ERESTARTSYS;
	}

	ret = score_system_runtime_update(system);

	mutex_unlock(lock);
	return ret;
}

static int score_vertex_qbuf(struct file *file, struct vs4l_container_list *clist)
{
	int ret = 0;
	struct score_vertex_ctx *vctx = file->private_data;
	struct score_vertex *vertex = vctx->vertex;
	struct score_device *device = container_of(vertex, struct score_device, vertex);
	struct mutex *lock = &vctx->lock;

	sysfs_debug_count_add(NORMAL_CNT_QBUF);
	if (mutex_lock_interruptible(lock)) {
		score_verr("mutex_lock_interruptible is fail \n", vctx);
		return -ERESTARTSYS;
	}

	ret = score_device_start(device);
	if (ret)
		goto p_err_start;

	CALL_VOPS(vctx, get);

	ret = CALL_VOPS(vctx, queue, clist);
	if (ret) {
		sysfs_debug_count_add(ERROR_CNT_VCTX_QUEUE);
		goto p_err_queue;
	}

	vctx->blocking = file->f_flags & O_NONBLOCK;
	CALL_VOPS(vctx, deque, clist);
	CALL_VOPS(vctx, put);
	score_device_stop(device);

	mutex_unlock(lock);

	return ret;
p_err_queue:
	CALL_VOPS(vctx, put);
	score_device_stop(device);
p_err_start:
	mutex_unlock(lock);

	return ret;
}

static int score_vertex_dqbuf(struct file *file, struct vs4l_container_list *clist)
{
	int ret = 0;
	return ret;
}

static int score_vertex_streamon(struct file *file)
{
	int ret = 0;
	return ret;
}

static int score_vertex_streamoff(struct file *file)
{
	int ret = 0;
	return ret;
}

const struct vertex_ioctl_ops score_vertex_ioctl_ops = {
	.vertexioc_s_graph	= score_vertex_s_graph,
	.vertexioc_s_format	= score_vertex_s_format,
	.vertexioc_s_param	= score_vertex_s_param,
	.vertexioc_s_ctrl	= score_vertex_s_ctrl,
	.vertexioc_qbuf		= score_vertex_qbuf,
	.vertexioc_dqbuf	= score_vertex_dqbuf,
	.vertexioc_streamon	= score_vertex_streamon,
	.vertexioc_streamoff	= score_vertex_streamoff
};

static int score_vertex_open(struct file *file)
{
	int ret = 0;
	struct score_vertex *vertex = dev_get_drvdata(&vision_devdata(file)->dev);
	struct score_device *device = container_of(vertex, struct score_device, vertex);
	struct mutex *lock = &vertex->lock;
	struct score_vertex_ctx *vctx;

	if (mutex_lock_interruptible(lock)) {
		score_err("mutex_lock_interruptible is fail\n");
		return -ERESTARTSYS;
	}

	ret = __vref_get(&vertex->open_cnt);
	if (ret) {
		atomic_set(&vertex->open_cnt.refcount, 0);
		goto p_err_vref;
	}

	vctx = score_vctx_create(vertex, &device->system.memory);
	if (!vctx)
		goto p_err_vctx;

	ret = score_vertexmgr_vctx_register(&device->vertexmgr, vctx);
	if (ret)
		goto p_err_vertexmgr;

	file->private_data = vctx;
	mutex_unlock(lock);

	return ret;
p_err_vertexmgr:
	score_vctx_destroy(vctx);
p_err_vctx:
	__vref_put(&vertex->open_cnt);
p_err_vref:
	file->private_data = NULL;
	sysfs_debug_count_add(ERROR_CNT_OPEN);
	mutex_unlock(lock);

	return ret;
}

static int score_vertex_close(struct file *file)
{
	int ret = 0;
	struct score_vertex_ctx *vctx = file->private_data;
	struct score_vertex *vertex = vctx->vertex;
	struct score_device *device = container_of(vertex, struct score_device, vertex);
	struct mutex *lock = &vertex->lock;

	if (mutex_lock_interruptible(lock)) {
		score_verr("mutex_lock_interruptible is fail \n", vctx);
		return -ERESTARTSYS;
	}

	score_vertexmgr_vctx_unregister(&device->vertexmgr, vctx);
	score_vctx_destroy(vctx);
	__vref_put(&vertex->open_cnt);

	mutex_unlock(lock);

	return ret;
}

static unsigned int score_vertex_poll(struct file *file,
	poll_table *poll)
{
	int ret = 0;
	struct score_vertex_ctx *vctx = file->private_data;
	struct score_framemgr *framemgr = &vctx->framemgr;
	unsigned long events;

	if (!(vctx->state & BIT(SCORE_VERTEX_START))) {
		score_verr("invalid state\n", vctx);
		ret |= POLLERR;
		goto p_err;
	}

	poll_wait(file, &framemgr->done_wq, poll);

	events = poll_requested_events(poll);
	if (events & POLLIN) {
		if (!score_bitmap_full(framemgr->fid_map, SCORE_MAX_FRAME)) {
			ret |= (POLLIN | POLLRDNORM);
		}
	}

	if (events & POLLOUT) {
		if (!score_bitmap_empty(framemgr->fid_map, SCORE_MAX_FRAME)) {
			ret |= (POLLOUT | POLLWRNORM);
		}
	}

p_err:
	return ret;
}

const struct vision_file_ops score_vertex_fops = {
	.owner		= THIS_MODULE,
	.open		= score_vertex_open,
	.release	= score_vertex_close,
	.poll		= score_vertex_poll,
	.ioctl		= vertex_ioctl,
	.compat_ioctl	= vertex_compat_ioctl32
};

int score_vertex_probe(struct score_vertex *vertex, struct device *parent)
{
	int ret = 0;
	struct vision_device *vdev;

	get_device(parent);
	mutex_init(&vertex->lock);
	__vref_init(&vertex->open_cnt, vertex, __vref_open, __vref_close);

	vdev = &vertex->vd;
	snprintf(vdev->name, sizeof(vdev->name), "%s", SCORE_VERTEX_NAME);
	vdev->fops		= &score_vertex_fops;
	vdev->ioctl_ops		= &score_vertex_ioctl_ops;
	vdev->release		= NULL;
	vdev->lock		= NULL;
	vdev->parent		= parent;
	vdev->type		= VISION_DEVICE_TYPE_VERTEX;
	dev_set_drvdata(&vdev->dev, vertex);

	ret = vision_register_device(vdev, SCORE_VERTEX_MINOR, score_vertex_fops.owner);
	if (ret) {
		probe_err("vision_register_device is fail(%d)\n", ret);
		put_device(parent);
	}

	return ret;
}

void score_vertex_release(struct score_vertex *vertex)
{
	struct score_device *device = container_of(vertex, struct score_device, vertex);

	put_device(device->dev);
}
