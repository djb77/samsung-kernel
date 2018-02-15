/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <media/videobuf2-ion.h>

#include "vision-config.h"
#include "vision-buffer.h"

#define DEBUG_SENTENCE_MAX	300

struct vision_debug_log {
	size_t			dsentence_pos;
	char			dsentence[DEBUG_SENTENCE_MAX];
};

void vision_dmsg_concate(struct vision_debug_log *log, const char *fmt, ...)
{
	va_list ap;
	char term[50];
	u32 copy_len;

	va_start(ap, fmt);
	vsnprintf(term, sizeof(term), fmt, ap);
	va_end(ap);

	if (log->dsentence_pos >= DEBUG_SENTENCE_MAX) {
		vision_err("debug message(%zd) over max\n", log->dsentence_pos);
		return;
	}

	copy_len = min((DEBUG_SENTENCE_MAX - log->dsentence_pos - 1), strlen(term));
	strncpy(log->dsentence + log->dsentence_pos, term, copy_len);
	log->dsentence_pos += copy_len;
	log->dsentence[log->dsentence_pos] = 0;
}

char * vision_dmsg_print(struct vision_debug_log *log)
{
	log->dsentence_pos = 0;
	return log->dsentence;
}

#define DLOG_INIT()		struct vision_debug_log vision_debug_log = { .dsentence_pos = 0 }
#define DLOG(fmt, ...)		vision_dmsg_concate(&vision_debug_log, fmt, ##__VA_ARGS__)
#define DLOG_OUT()		vision_dmsg_print(&vision_debug_log)

struct vb_fmt vb_fmts[] = {
	{
		.name		= "RGB",
		.colorspace	= VS4L_DF_IMAGE_RGB,
		.planes 	= 1,
		.bitsperpixel	= { 24 }
	}, {
		.name		= "ARGB",
		.colorspace	= VS4L_DF_IMAGE_RGBX,
		.planes 	= 1,
		.bitsperpixel	= { 32 }
	}, {
		.name		= "YUV 4:2:0 planar, Y/CbCr",
		.colorspace	= VS4L_DF_IMAGE_NV12,
		.planes 	= 2,
		.bitsperpixel	= { 8, 8 }
	}, {
		.name		= "YUV 4:2:0 planar, Y/CrCb",
		.colorspace	= VS4L_DF_IMAGE_NV21,
		.planes 	= 2,
		.bitsperpixel	= { 8, 8 }
	}, {
		.name		= "YUV 4:2:2 packed, YCbYCr",
		.colorspace	= VS4L_DF_IMAGE_YUYV,
		.planes 	= 1,
		.bitsperpixel	= { 16 }
	}, {
		.name		= "YUV 4:4:4 packed, YCbCr",
		.colorspace	= VS4L_DF_IMAGE_YUV4,
		.planes 	= 1,
		.bitsperpixel	= { 24 }
	}, {
		.name		= "VX unsigned 8 bit",
		.colorspace	= VS4L_DF_IMAGE_U8,
		.planes 	= 1,
		.bitsperpixel	= { 8 }
	}, {
		.name		= "VX unsigned 16 bit",
		.colorspace	= VS4L_DF_IMAGE_U16,
		.planes 	= 1,
		.bitsperpixel	= { 16 }
	}, {
		.name		= "VX unsigned 32 bit",
		.colorspace	= VS4L_DF_IMAGE_U32,
		.planes 	= 1,
		.bitsperpixel	= { 32 }
	}, {
		.name		= "VX signed 16 bit",
		.colorspace	= VS4L_DF_IMAGE_S16,
		.planes 	= 1,
		.bitsperpixel	= { 16 }
	}, {
		.name		= "VX signed 32 bit",
		.colorspace	= VS4L_DF_IMAGE_S32,
		.planes 	= 1,
		.bitsperpixel	= { 32 }
	}
};

void __vb_queue_print(struct vb_queue *q)
{
	DLOG_INIT();
	struct vb_bundle *bundle, *temp;

	DLOG("[VB] queued(%d) :", atomic_read(&q->queued_count));
	list_for_each_entry_safe(bundle, temp, &q->queued_list, queued_entry) {
		DLOG("%d->", bundle->clist.index);
	}
	DLOG("X");

	vision_info("%s\n", DLOG_OUT());

	DLOG("[VB] process(%d) :", atomic_read(&q->process_count));
	list_for_each_entry_safe(bundle, temp, &q->process_list, process_entry) {
		DLOG("%d->", bundle->clist.index);
	}
	DLOG("X");

	vision_info("%s\n", DLOG_OUT());

	DLOG("[VB] done(%d) :", atomic_read(&q->done_count));
	list_for_each_entry_safe(bundle, temp, &q->done_list, done_entry) {
		DLOG("%d->", bundle->clist.index);
	}
	DLOG("X");

	vision_info("%s\n", DLOG_OUT());
}

void __vb_buffer_print(struct vs4l_container_list *c)
{
	vision_info("[VB] c->direction : %d", c->direction);
	vision_info("[VB] c->id        : %d", c->id);
	vision_info("[VB] c->index     : %d", c->count);
}

static struct vb_fmt * __vb_find_format(u32 colorspace)
{
	u32 i;
	struct vb_fmt *fmt = NULL;

	for (i = 0; i < ARRAY_SIZE(vb_fmts); ++i) {
		if (vb_fmts[i].colorspace == colorspace) {
			fmt = &vb_fmts[i];
			break;
		}
	}

	return fmt;
}

static int __vb_plane_size(struct vb_format *format)
{
	int ret = 0;
	u32 plane;
	struct vb_fmt *fmt;

	BUG_ON(!format);

	fmt = format->fmt;

	if (fmt->planes > VB_MAX_PLANES) {
		vision_err("planes(%d) is invalid\n", fmt->planes);
		ret = -EINVAL;
		goto p_err;
	}

	for (plane = 0; plane < fmt->planes; ++plane)
		format->size[plane] = (fmt->bitsperpixel[plane] / 8) * format->width * format->height;

p_err:
	return ret;
}

static int __vb_unmap_dmabuf(struct vb_queue *q, struct vb_buffer *buffer)
{
	int ret = 0;

	if (buffer->dvaddr)
		call_memop(q, unmap_dmabuf, buffer->mem_priv);

	if (buffer->mem_priv)
		call_memop(q, detach_dmabuf, buffer->mem_priv);

	if (buffer->dbuf)
		dma_buf_put(buffer->dbuf);

	buffer->mem_priv = NULL;
	buffer->cookie = NULL;
	buffer->kvaddr = NULL;
	buffer->dbuf = NULL;
	buffer->dvaddr = 0;

	return ret;
}

static int __vb_unmap_virtptr(struct vb_queue *q, struct vb_buffer *buffer)
{
	int ret = 0;

	if (buffer->reserved)
		kfree((void *)buffer->m.userptr);

	buffer->mem_priv = NULL;
	buffer->cookie = NULL;
	buffer->kvaddr = NULL;
	buffer->dbuf = NULL;
	buffer->dvaddr = 0;
	buffer->reserved = 0;

	return ret;
}

static int __vb_map_dmabuf(struct vb_queue *q, struct vb_buffer *buffer, u32 size)
{
	int ret = 0, write;
	void *mem_priv, *cookie;
	dma_addr_t dvaddr;
	struct dma_buf *dbuf;

	buffer->dbuf = NULL;
	buffer->mem_priv = NULL;
	buffer->cookie = NULL;
	buffer->kvaddr = NULL;
	buffer->dvaddr = 0;

	if (q->direction == VS4L_DIRECTION_OT)
		write = 1;
	else
		write = 0;

	dbuf = dma_buf_get(buffer->m.fd);
	if (IS_ERR_OR_NULL(dbuf)) {
		vision_err("qbuf: invalid dmabuf fd\n");
		ret = -EINVAL;
		goto p_err;
	}

	buffer->dbuf = dbuf;

	if (dbuf->size < size) {
		vision_err("user buffer size is small(%zd, %d)\n", dbuf->size, size);
		ret = -EINVAL;
		goto p_err;
	}

	/* Acquire each plane's memory */
	mem_priv = call_memop(q, attach_dmabuf, q->alloc_ctx, dbuf, size, write);
	if (IS_ERR(mem_priv)) {
		vision_err("call_memop(attach_dmabuf) is fail(%p, %d, %d)\n", q->alloc_ctx, size, write);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	buffer->mem_priv = mem_priv;

	cookie = call_memop(q, cookie, mem_priv);
	if (IS_ERR_OR_NULL(cookie)) {
		vision_err("call_memop(cookie) is fail(%p, %p)\n", mem_priv, cookie);
		ret = PTR_ERR(mem_priv);
		goto p_err;
	}

	buffer->cookie = cookie;

	ret = call_memop(q, map_dmabuf, mem_priv);
	if (ret) {
		vision_err("call_memop(map_dmabuf) is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vb2_ion_dma_address(cookie, &dvaddr);
	if (ret) {
		vision_err("vb2_ion_dma_address is fail(%d)\n", ret);
		goto p_err;
	}

	buffer->dvaddr = dvaddr;

#ifdef VISION_MAP_KVADDR
	{
		void *kvaddr;

		kvaddr = call_memop(q, vaddr, mem_priv);
		if (IS_ERR_OR_NULL(kvaddr)) {
			vision_err("call_memop(vaddr) is fail(%p, %p)\n", mem_priv, kvaddr);
			ret = PTR_ERR(kvaddr);
			goto p_err;
		}

		buffer->kvaddr = kvaddr;
	}
#endif

	return 0;

p_err:
	__vb_unmap_dmabuf(q, buffer);
	return ret;
}

static int __vb_map_virtptr(struct vb_queue *q, struct vb_buffer *buffer, u32 size)
{
	int ret = 0;
	unsigned long tmp_buffer;
	void *mbuf = NULL;

	mbuf = kmalloc(size, GFP_KERNEL);
	if (!mbuf) {
		vision_err("kmalloc is fail\n");
		ret = -ENOMEM;
		goto p_err;
	}

	tmp_buffer = (unsigned long)mbuf;

	ret = copy_from_user((void *)tmp_buffer, (void *)buffer->m.userptr, size);
	if (ret) {
		vision_err("copy_from_user() is fail(%d)\n", ret);
		goto p_err;
	}

	/* back up - userptr */
	buffer->reserved = buffer->m.userptr;
	buffer->m.userptr = (unsigned long)tmp_buffer;

p_err:
	return ret;
}

static int __vb_queue_alloc(struct vb_queue *q,
	struct vs4l_container_list *c)
{
	DLOG_INIT();
	int ret = 0;
	u32 alloc_size, i, j;
	u8 *mapped_ptr;
	struct vb_format_list *flist;
	struct vb_bundle *bundle;
	struct vb_container_list *clist;
	struct vb_container *container;
	struct vb_buffer *buffer;

	BUG_ON(!q);
	BUG_ON(!c);
	BUG_ON(c->index >= VB_MAX_BUFFER);

	flist = &q->format;

	/* allocation */
	alloc_size = sizeof(struct vb_bundle);
	alloc_size += sizeof(struct vb_container) * c->count;
	for (i = 0; i < c->count; ++i)
		alloc_size += sizeof(struct vb_buffer) * c->containers[i].count;

	bundle = kzalloc(alloc_size, GFP_KERNEL);
	if (!bundle) {
		vision_err("Memory alloc for buffer struct failed\n");
		ret = -ENOMEM;
		goto p_err;
	}

	/* mapping */
	mapped_ptr = (u8 *)bundle + sizeof(struct vb_bundle);
	bundle->clist.containers = (struct vb_container *)mapped_ptr;
	mapped_ptr += sizeof(struct vb_container) * c->count;
	for (i = 0; i < c->count; ++i) {
		bundle->clist.containers[i].buffers = (struct vb_buffer *)mapped_ptr;
		mapped_ptr += sizeof(struct vb_buffer) * c->containers[i].count;
	}

	/* fill */
	bundle->state = VB_BUF_STATE_DEQUEUED;
	clear_bit(VS4L_CL_FLAG_PREPARE, &bundle->flags);
	clear_bit(VS4L_CL_FLAG_INVALID, &bundle->flags);
	clear_bit(VS4L_CL_FLAG_DONE, &bundle->flags);

	clist = &bundle->clist;
	clist->index = c->index;
	clist->id = c->id;
	clist->direction = c->direction;
	clist->count = c->count;
	clist->flags = c->flags;

	for (i = 0; i < clist->count; ++i) {
		container = &clist->containers[i];
		container->count = c->containers[i].count;
		container->memory = c->containers[i].memory;
		container->reserved[0] = c->containers[i].reserved[0];
		container->reserved[1] = c->containers[i].reserved[1];
		container->reserved[2] = c->containers[i].reserved[2];
		container->reserved[3] = c->containers[i].reserved[3];
		container->target = c->containers[i].target;
		container->type = c->containers[i].type;

		for (j = 0; j < flist->count; ++j) {
			DLOG("c%d t%d == f%d t%d\n", i, container->target, j, flist->formats[j].target);
			if (container->target == flist->formats[j].target) {
				container->format = &flist->formats[j];
				break;
			}
		}

		if (!container->format) {
			vision_err("format is not found\n");
			vision_err("%s", DLOG_OUT());
			kfree(bundle);
			ret = -EINVAL;
			goto p_err;
		}

		for (j = 0; j < container->count; ++j) {
			buffer = &container->buffers[j];
			buffer->roi = c->containers[i].buffers[j].roi;
			buffer->m.userptr = c->containers[i].buffers[j].m.userptr;
			buffer->dbuf = NULL;
			buffer->mem_priv = NULL;
			buffer->cookie = NULL;
			buffer->kvaddr = NULL;
			buffer->dvaddr = 0;
		}
	}

	q->bufs[c->index] = bundle;
	q->num_buffers++;

p_err:
	return ret;
}

static int __vb_queue_free(struct vb_queue *q,
	struct vb_bundle *bundle)
{
	int ret = 0;

	BUG_ON(!bundle);
	BUG_ON(bundle->clist.index >= VB_MAX_BUFFER);

	q->bufs[bundle->clist.index] = NULL;
	kfree(bundle);
	q->num_buffers--;

	return ret;
}

static int __vb_queue_check(struct vb_bundle *bundle,
	struct vs4l_container_list *c)
{
	int ret = 0;
	u32 i, j;
	struct vb_container_list *clist;
	struct vb_container *container;
	struct vb_buffer *buffer;

	BUG_ON(!bundle);
	BUG_ON(!c);

	clist = &bundle->clist;

	if (clist->index != c->index) {
		vision_err("index is conflict(%d != %d)\n", clist->index, c->index);
		goto p_err;
	}

	if (clist->direction != c->direction) {
		vision_err("direction is conflict(%d != %d)\n", clist->direction, c->direction);
		goto p_err;
	}

	if (clist->count != c->count) {
		vision_err("count is conflict(%d != %d)\n", clist->count, c->count);
		goto p_err;
	}

	clist->flags = c->flags;
	clist->id = c->id;

	for (i = 0; i < clist->count; ++i) {
		container = &clist->containers[i];

		if (container->target != c->containers[i].target) {
			vision_err("target is conflict(%d != %d)\n", container->target, c->containers[i].target);
			goto p_err;
		}

		if (container->count != c->containers[i].count) {
			vision_err("count is conflict(%d != %d)\n", container->count, c->containers[i].count);
			goto p_err;
		}

		for (j = 0; j < container->count; ++j) {
			buffer = &container->buffers[j];
			buffer->roi = c->containers[i].buffers[j].roi;

			if (buffer->m.fd != c->containers[i].buffers[j].m.fd) {
				vision_err("buffer is conflict(%d != %d)\n", buffer->m.fd, c->containers[i].buffers[j].m.fd);
				goto p_err;
			}
		}
	}

p_err:
	return ret;
}

static int __vb_buf_prepare(struct vb_queue *q, struct vb_bundle *bundle)
{
	int ret = 0;
	u32 i, j, k;
	struct vb_format *format;
	struct vb_container *container;
	struct vb_buffer *buffer;

	BUG_ON(!q);
	BUG_ON(!bundle);

	if (test_bit(VS4L_CL_FLAG_PREPARE, &bundle->flags))
		goto p_err;

	for (i = 0; i < bundle->clist.count; ++i) {
		container = &bundle->clist.containers[i];
		format = container->format;

		switch (container->type) {
		case VS4L_BUFFER_LIST:
			k = container->count;
			break;
		case VS4L_BUFFER_ROI:
			k = container->count;
			break;
		case VS4L_BUFFER_PYRAMID:
			k = container->count;
			break;
		default:
			vision_err("unsupported container type\n");
			goto p_err;
		}

		switch (container->memory) {
		case VS4L_MEMORY_DMABUF:
			for (j = 0; j < k; ++j) {
				buffer = &container->buffers[j];
				ret = __vb_map_dmabuf(q, buffer, format->size[format->plane]);
				if (ret) {
					vision_err("__vb_qbuf_dmabuf is fail(%d)\n", ret);
					goto p_err;
				}
			}
			break;
		case VS4L_MEMORY_VIRTPTR:
			for (j = 0; j < k; ++j) {
				buffer = &container->buffers[j];
				ret = __vb_map_virtptr(q, buffer, format->size[format->plane]);
				if (ret) {
					vision_err("__vb_map_virtptr is fail(%d)\n", ret);
					goto p_err;
				}
			}
			break;
		default:
			vision_err("unsupported container memory type\n");
			goto p_err;
		}
	}

	ret = call_op(q, buf_prepare, q, &bundle->clist);
	if (ret) {
		vision_err("call_op(buf_prepare) is fail(%d)\n", ret);
		goto p_err;
	}

	set_bit(VS4L_CL_FLAG_PREPARE, &bundle->flags);

p_err:
	return ret;
}

static int __vb_buf_unprepare(struct vb_queue *q, struct vb_bundle *bundle)
{
	int ret = 0;
	u32 i, j, k;
	struct vb_format *format;
	struct vb_container *container;
	struct vb_buffer *buffer;

	BUG_ON(!q);
	BUG_ON(!bundle);


	if (!test_bit(VS4L_CL_FLAG_PREPARE, &bundle->flags))
		goto p_err;

	for (i = 0; i < bundle->clist.count; ++i) {
		container = &bundle->clist.containers[i];
		format = container->format;

		switch (container->type) {
		case VS4L_BUFFER_LIST:
			k = container->count;
			break;
		case VS4L_BUFFER_ROI:
			k = container->count;
			break;
		case VS4L_BUFFER_PYRAMID:
			k = container->count;
			break;
		default:
			vision_err("unsupported container type\n");
			goto p_err;
		}

		switch (container->memory) {
		case VS4L_MEMORY_DMABUF:
			for (j = 0; j < k; ++j) {
				buffer = &container->buffers[j];
				ret = __vb_unmap_dmabuf(q, buffer);
				if (ret) {
					vision_err("__vb_qbuf_dmabuf is fail(%d)\n", ret);
					goto p_err;
				}
			}
			break;
		case VS4L_MEMORY_VIRTPTR:
			for (j = 0; j < k; ++j) {
				buffer = &container->buffers[j];
				ret = __vb_unmap_virtptr(q, buffer);
				if (ret) {
					vision_err("__vb_unmap_virtptr is fail(%d)\n", ret);
					goto p_err;
				}
			}
			break;
		default:
			vision_err("unsupported container memory type\n");
			goto p_err;
		}
	}

	ret = call_op(q, buf_unprepare, q, &bundle->clist);
	if (ret) {
		vision_err("call_op(buf_unprepare) is fail(%d)\n", ret);
		goto p_err;
	}

	clear_bit(VS4L_CL_FLAG_PREPARE, &bundle->flags);

p_err:
	return ret;
}

static int __vb_wait_for_done_vb(struct vb_queue *q, int nonblocking)
{
	int ret = 0;

	if (!q->streaming) {
		vision_err("Streaming off, will not wait for buffers\n");
		ret = -EINVAL;
		goto p_err;
	}

	if (!list_empty(&q->done_list))
		goto p_err;

	if (nonblocking) {
		vision_err("Nonblocking and no buffers to dequeue, will not wait\n");
		ret = -EINVAL;
		goto p_err;
	}

	mutex_unlock(q->lock);

	ret = wait_event_interruptible(q->done_wq, !list_empty(&q->done_list) || !q->streaming);

	mutex_lock(q->lock);

p_err:
	return ret;
}

static int __vb_get_done_vb(struct vb_queue *q,
	struct vb_bundle **bundle,
	struct vs4l_container_list *c,
	int nonblocking)
{
	unsigned long flags;
	int ret;

	/*
	 * Wait for at least one buffer to become available on the done_list.
	 */
	ret = __vb_wait_for_done_vb(q, nonblocking);
	if (ret)
		return ret;

	/*
	 * Driver's lock has been held since we last verified that done_list
	 * is not empty, so no need for another list_empty(done_list) check.
	 */

	spin_lock_irqsave(&q->done_lock, flags);

	*bundle = list_first_entry(&q->done_list, struct vb_bundle, done_entry);
	list_del(&(*bundle)->done_entry);
	atomic_dec(&q->done_count);

	spin_unlock_irqrestore(&q->done_lock, flags);

	return ret;
}

static void __fill_vs4l_buffer(struct vb_bundle *bundle,
	struct vs4l_container_list *c)
{
	struct vb_container_list *clist;

	clist = &bundle->clist;
	c->flags &= ~(1 << VS4L_CL_FLAG_TIMESTAMP);
	c->flags &= ~(1 << VS4L_CL_FLAG_PREPARE);
	c->flags &= ~(1 << VS4L_CL_FLAG_INVALID);
	c->flags &= ~(1 << VS4L_CL_FLAG_DONE);

	if (test_bit(VS4L_CL_FLAG_TIMESTAMP, &clist->flags)) {
		c->flags |= (1 << VS4L_CL_FLAG_TIMESTAMP);
		memcpy(c->timestamp, clist->timestamp, sizeof(clist->timestamp));
	}

	if (test_bit(VS4L_CL_FLAG_PREPARE, &bundle->flags))
		c->flags |= (1 << VS4L_CL_FLAG_PREPARE);

	if (test_bit(VS4L_CL_FLAG_INVALID, &bundle->flags))
		c->flags |= (1 << VS4L_CL_FLAG_INVALID);

	if (test_bit(VS4L_CL_FLAG_DONE, &bundle->flags))
		c->flags |= (1 << VS4L_CL_FLAG_DONE);

	c->index = clist->index;
	c->id = clist->id;
}

static void __vb_dqbuf(struct vb_bundle *bundle)
{
	if (bundle->state == VB_BUF_STATE_DEQUEUED)
		return;

	bundle->state = VB_BUF_STATE_DEQUEUED;
}

int vb_queue_init(struct vb_queue *q,
	void *alloc_ctx,
	const struct vb2_mem_ops *mem_ops,
	const struct vb_ops *ops,
	struct mutex *lock,
	u32 direction)
{
	int ret = 0;

	INIT_LIST_HEAD(&q->queued_list);
	atomic_set(&q->queued_count, 0);

	INIT_LIST_HEAD(&q->process_list);
	atomic_set(&q->process_count, 0);

	INIT_LIST_HEAD(&q->done_list);
	atomic_set(&q->done_count, 0);

	spin_lock_init(&q->done_lock);
	init_waitqueue_head(&q->done_wq);

	q->num_buffers = 0;
	q->direction = direction;
	q->lock	= lock;
	q->streaming = 0;
	q->alloc_ctx = alloc_ctx;
	q->mem_ops = mem_ops;
	q->ops = ops;

	clear_bit(VB_QUEUE_STATE_FORMAT, &q->state);
	q->format.count = 0;
	q->format.formats = NULL;

	return ret;
}

int vb_queue_s_format(struct vb_queue *q, struct vs4l_format_list *flist)
{
	int ret = 0;
	u32 i;
	struct vs4l_format *f;
	struct vb_fmt *fmt;

	q->format.count = flist->count;
	q->format.formats = kzalloc(sizeof(struct vb_format) * flist->count, GFP_KERNEL);

	for (i = 0; i < flist->count; ++i) {
		f = &flist->formats[i];

		fmt = __vb_find_format(f->format);
		if (!fmt) {
			vision_err("__vb_find_format is fail\n");
			kfree(q->format.formats);
			ret = -EINVAL;
			goto p_err;
		}

		q->format.formats[i].fmt = fmt;
		q->format.formats[i].colorspace = f->format;
		q->format.formats[i].target = f->target;
		q->format.formats[i].plane = f->plane;
		q->format.formats[i].width = f->width;
		q->format.formats[i].height = f->height;

		ret = __vb_plane_size(&q->format.formats[i]);
		if (ret) {
			vision_err("__vb_plane_size is fail(%d)\n", ret);
			kfree(q->format.formats);
			goto p_err;
		}
	}

	set_bit(VB_QUEUE_STATE_FORMAT, &q->state);

p_err:
	return ret;
}

int vb_queue_start(struct vb_queue *q)
{
	int ret = 0;

	if (!test_bit(VB_QUEUE_STATE_FORMAT, &q->state)) {
		vision_err("format is not configured\n");
		ret = -EINVAL;
		goto p_err;
	}

	q->streaming = 1;
	set_bit(VB_QUEUE_STATE_START, &q->state);

p_err:
	return ret;
}

int vb_queue_stop(struct vb_queue *q)
{
	int ret = 0;
	u32 i;
	struct vb_bundle *bundle;

	q->streaming = 0;

	wake_up_all(&q->done_wq);

	if (atomic_read(&q->queued_count) > 0) {
		vision_err("queued list is not empty\n");
		ret = -EINVAL;
		goto p_err;
	}

	if (atomic_read(&q->process_count) > 0) {
		vision_err("process list is not empty\n");
		ret = -EINVAL;
		goto p_err;
	}

	if (atomic_read(&q->done_count) > 0) {
		vision_err("done list is not empty\n");
		ret = -EINVAL;
		goto p_err;
	}

	INIT_LIST_HEAD(&q->queued_list);
	INIT_LIST_HEAD(&q->process_list);
	INIT_LIST_HEAD(&q->done_list);

	for (i = 0; i < VB_MAX_BUFFER; ++i) {
		bundle = q->bufs[i];
		if (!bundle)
			continue;

		ret = __vb_buf_unprepare(q, bundle);
		if (ret) {
			vision_err("__vb_buf_unprepare is fail(%d)\n", ret);
			goto p_err;
		}

		ret = __vb_queue_free(q, bundle);
		if (ret) {
			vision_err("__vb_queue_free is fail(%d)\n", ret);
			goto p_err;
		}
	}

	if (q->num_buffers != 0) {
		vision_err("memroy leakage is issued(%d)\n", q->num_buffers);
		BUG();
	}

	clear_bit(VB_QUEUE_STATE_START, &q->state);

p_err:
	return ret;
}

int vb_queue_qbuf(struct vb_queue *q, struct vs4l_container_list *c)
{
	int ret = 0;
	struct vb_bundle *bundle;
	struct vb_container *container;
	u32 direction, size;
	u32 i, j, k;

	if (q->direction != c->direction) {
		vision_err("qbuf: invalid buffer direction\n");
		ret = -EINVAL;
		goto p_err;
	}

	if (c->index >= VB_MAX_BUFFER) {
		vision_err("qbuf: buffer index out of range\n");
		ret = -EINVAL;
		goto p_err;
	}

	bundle = q->bufs[c->index];
	if (bundle) {
		ret = __vb_queue_check(bundle, c);
		if (ret) {
			vision_err("__vb_queue_check is fail(%d)\n", ret);
			goto p_err;
		}
	} else {
		ret = __vb_queue_alloc(q, c);
		if (ret) {
			vision_err("__vb_queue_alloc is fail(%d)\n", ret);
			goto p_err;
		}

		bundle = q->bufs[c->index];
	}

	if (bundle->state != VB_BUF_STATE_DEQUEUED) {
		vision_err("qbuf: buffer already in use\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = __vb_buf_prepare(q, bundle);
	if (ret) {
		vision_err("__vb_buf_prepare is fail(%d)\n", ret);
		goto p_err;
	}

	if (q->direction == VS4L_DIRECTION_OT)
		direction = DMA_TO_DEVICE;
	else
		direction = DMA_FROM_DEVICE;

	/* sync buffers */
	for (i = 0; i < bundle->clist.count; ++i) {
		container = &bundle->clist.containers[i];
		BUG_ON(!container->format);
		if (container->memory != VS4L_MEMORY_VIRTPTR) {
			k = container->count;
			size = container->format->size[container->format->plane];
			for (j = 0; j < k; ++j)
				vb2_ion_sync_for_device(container->buffers[j].cookie, 0, size, direction);
		}
	}

	/*
	 * Add to the queued buffers list, a buffer will stay on it until
	 * dequeued in dqbuf.
	 */
	list_add_tail(&bundle->queued_entry, &q->queued_list);
	bundle->state = VB_BUF_STATE_QUEUED;
	atomic_inc(&q->queued_count);

	/* vb_buffer_print(&vb->buffer); */

p_err:
	return ret;
}

int vb_queue_dqbuf(struct vb_queue *q,
	struct vs4l_container_list *c,
	bool nonblocking)
{
	int ret = 0;
	struct vb_bundle *bundle;

	if (q->direction != c->direction) {
		vision_err("qbuf: invalid buffer direction\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = __vb_get_done_vb(q, &bundle, c, nonblocking);
	if (ret < 0) {
		vision_err("__vb2_get_done_vb is fail(%d)\n", ret);
		return ret;
	}

	if (bundle->state != VB_BUF_STATE_DONE) {
		vision_err("dqbuf: Invalid buffer state(%X)\n", bundle->state);
		return -EINVAL;
	}

	/* Fill buffer information for the userspace */
	__fill_vs4l_buffer(bundle, c);
	/* Remove from videobuf queue */
	/* go back to dequeued state */
	__vb_dqbuf(bundle);

	list_del(&bundle->queued_entry);
	atomic_dec(&q->queued_count);

p_err:
	return ret;
}

void vb_queue_process(struct vb_queue *q, struct vb_bundle *bundle)
{
	BUG_ON(!q);
	BUG_ON(!bundle);
	BUG_ON(q->direction != bundle->clist.direction);

	bundle->state = VB_BUF_STATE_PROCESS;
	list_add_tail(&bundle->process_entry, &q->process_list);
	atomic_inc(&q->process_count);
}

void vb_queue_done(struct vb_queue *q, struct vb_bundle *bundle)
{
	struct vb_container *container;
	unsigned long flag;
	u32 direction, size;
	u32 i, j, k;

	BUG_ON(!q);
	BUG_ON(!bundle);
	BUG_ON(q->direction != bundle->clist.direction);

	if (q->direction == VS4L_DIRECTION_OT)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	/* sync buffers */
	for (i = 0; i < bundle->clist.count; ++i) {
		container = &bundle->clist.containers[i];
		BUG_ON(!container->format);
		if (container->memory != VS4L_MEMORY_VIRTPTR) {
			k = container->count;
			size = container->format->size[container->format->plane];
			for (j = 0; j < k; ++j)
				vb2_ion_sync_for_cpu(container->buffers[j].cookie, 0, size, direction);
		}
	}

	spin_lock_irqsave(&q->done_lock, flag);

	list_del(&bundle->process_entry);
	atomic_dec(&q->process_count);

	bundle->state = VB_BUF_STATE_DONE;
	list_add_tail(&bundle->done_entry, &q->done_list);
	atomic_inc(&q->done_count);

	spin_unlock_irqrestore(&q->done_lock, flag);

	wake_up(&q->done_wq);
}
