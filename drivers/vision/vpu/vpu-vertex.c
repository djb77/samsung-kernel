/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

#include "vpu-config.h"
#include "vision-ioctl.h"
#include "vpu-vertex.h"
#include "vision-dev.h"
#include "vpu-device.h"
#include "vpu-graphmgr.h"
#include "vpu-graph.h"
#include "vpu-queue.h"
#include "vpu-control.h"

const struct vision_file_ops vpu_vertex_fops;
const const struct vertex_ioctl_ops vpu_vertex_ioctl_ops;

static int __vref_open(struct vpu_vertex *vertex)
{
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	atomic_set(&vertex->start_cnt.refcount, 0);
	return vpu_device_open(device);
}

static int __vref_close(struct vpu_vertex *vertex)
{
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	return vpu_device_close(device);
}

static int __vref_start(struct vpu_vertex *vertex)
{
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	return vpu_device_start(device);
}

static int __vref_stop(struct vpu_vertex *vertex)
{
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	return vpu_device_stop(device);
}

static inline void __vref_init(struct vpu_vertex_refcount *vref,
	struct vpu_vertex *vertex, int (*first)(struct vpu_vertex *vertex), int (*final)(struct vpu_vertex *vertex))
{
	vref->vertex = vertex;
	vref->first = first;
	vref->final = final;
	atomic_set(&vref->refcount, 0);
}

static inline int __vref_get(struct vpu_vertex_refcount *vref)
{
	return (atomic_inc_return(&vref->refcount) == 1) ? vref->first(vref->vertex) : 0;
}

static inline int __vref_put(struct vpu_vertex_refcount *vref)
{
	return (atomic_dec_return(&vref->refcount) == 0) ? vref->final(vref->vertex) : 0;
}

int vpu_vertex_probe(struct vpu_vertex *vertex, struct device *parent)
{
	int ret = 0;
	struct vision_device *vdev;

	BUG_ON(!vertex);
	BUG_ON(!parent);

	get_device(parent);
	mutex_init(&vertex->lock);
	__vref_init(&vertex->open_cnt, vertex, __vref_open, __vref_close);
	__vref_init(&vertex->start_cnt, vertex, __vref_start, __vref_stop);

	vdev = &vertex->vd;
	snprintf(vdev->name, sizeof(vdev->name), "%s", VPU_VERTEX_NAME);
	vdev->fops		= &vpu_vertex_fops;
	vdev->ioctl_ops		= &vpu_vertex_ioctl_ops;
	vdev->release		= NULL;
	vdev->lock		= NULL;
	vdev->parent		= parent;
	vdev->type		= VISION_DEVICE_TYPE_VERTEX;
	dev_set_drvdata(&vdev->dev, vertex);

	ret = vision_register_device(vdev, VPU_VERTEX_MINOR, vpu_vertex_fops.owner);
	if (ret) {
		probe_err("vision_register_device is fail(%d)\n", ret);
		goto p_err;
	}

	probe_info("%s():%d\n", __func__, ret);

p_err:
	return ret;
}

/*
 * =============================================================================
 * Video File Opertation
 * =============================================================================
 */

static int vpu_vertex_open(struct file *file)
{
	int ret = 0;
	struct vpu_vertex *vertex = dev_get_drvdata(&vision_devdata(file)->dev);
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	struct mutex *lock = &vertex->lock;
	struct vpu_vertex_ctx *vctx;
	struct vpu_graph *graph;

	if (mutex_lock_interruptible(lock)) {
		vpu_err("mutex_lock_interruptible is fail\n");
		return -ERESTARTSYS;
	}

	ret = __vref_get(&vertex->open_cnt);
	if (ret) {
		vpu_err("vref_get is fail(%d)", ret);
		goto p_err;
	}

	ret = vpu_graph_create(&graph,
		&device->graphmgr,
		&device->resource,
		&device->system.memory,
		&device->system.exynos);
	if (ret) {
		vpu_err("vpu_graph_create is fail(%d)", ret);
		goto p_err;
	}

	vctx			= &graph->vctx;
	vctx->id		= graph->id;
	vctx->vertex		= vertex;
	mutex_init(&vctx->lock);

	ret = vpu_queue_open(&vctx->queue, &device->system.memory, &vctx->lock);
	if (ret) {
		vpu_err("vpu_queue_open is fail(%d)", ret);
		goto p_err;
	}

	file->private_data = vctx;
	vctx->state = BIT(VPU_VERTEX_OPEN);

p_err:
	vpu_iinfo("%s():%d\n", vctx, __func__, ret);
	mutex_unlock(lock);
	return ret;
}

static int vpu_vertex_close(struct file *file)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_graph *graph = container_of(vctx, struct vpu_graph, vctx);
	struct vpu_vertex *vertex = vctx->vertex;
	struct mutex *lock = &vertex->lock;
	int id = vctx->id;

	if (mutex_lock_interruptible(lock)) {
		vpu_ierr("mutex_lock_interruptible is fail\n", vctx);
		return -ERESTARTSYS;
	}

	ret = vpu_graph_destroy(graph);
	if (ret) {
		vpu_err("vpu_graph_destroy is fail(%d)", ret);
		goto p_err;
	}

	ret = __vref_put(&vertex->open_cnt);
	if (ret) {
		vpu_err("vref_put is fail(%d)", ret);
		goto p_err;
	}

p_err:
	vpu_info("[I%d]%s():%d\n", id, __func__, ret);
	mutex_unlock(lock);
	return ret;
}

static unsigned int vpu_vertex_poll(struct file *file,
	poll_table *poll)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_queue *queue = &vctx->queue;

	if (!(vctx->state & BIT(VPU_VERTEX_START))) {
		vpu_ierr("invalid state(%X)", vctx, vctx->state);
		ret |= POLLERR;
		goto p_err;
	}

	ret = vpu_queue_poll(queue, file, poll);

p_err:
	return ret;
}

const struct vision_file_ops vpu_vertex_fops = {
	.owner		= THIS_MODULE,
	.open		= vpu_vertex_open,
	.release	= vpu_vertex_close,
	.poll		= vpu_vertex_poll,
	.ioctl		= vertex_ioctl,
	.compat_ioctl	= vertex_compat_ioctl32
};

/*
 * =============================================================================
 * Video Ioctl Opertation
 * =============================================================================
 */

static int vpu_vertex_s_graph(struct file *file, struct vs4l_graph *ginfo)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_graph *graph = container_of(vctx, struct vpu_graph, vctx);
	struct mutex *lock = &vctx->lock;

	if (mutex_lock_interruptible(lock)) {
		vpu_ierr("mutex_lock_interruptible is fail\n", vctx);
		return -ERESTARTSYS;
	}

	if (!(vctx->state & BIT(VPU_VERTEX_OPEN))) {
		vpu_ierr("invalid state(%X)\n", vctx, vctx->state);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_graph_config(graph, ginfo);
	if (ret) {
		vpu_err("vpu_graph_config is fail(%d)\n", ret);
		goto p_err;
	}

	vctx->state = BIT(VPU_VERTEX_GRAPH);

p_err:
	mutex_unlock(lock);
	return ret;
}

static int vpu_vertex_s_format(struct file *file, struct vs4l_format_list *flist)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_queue *queue = &vctx->queue;
	struct mutex *lock = &vctx->lock;

	if (mutex_lock_interruptible(lock)) {
		vpu_ierr("mutex_lock_interruptible is fail\n", vctx);
		return -ERESTARTSYS;
	}

	if (!(vctx->state & (BIT(VPU_VERTEX_GRAPH) | BIT(VPU_VERTEX_FORMAT)))) {
		vpu_ierr("invalid state(%X)\n", vctx, vctx->state);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_queue_s_format(queue, flist);
	if (ret) {
		vpu_ierr("vpu_queue_s_format is fail(%d)\n", vctx, ret);
		goto p_err;
	}

	vctx->state = BIT(VPU_VERTEX_FORMAT);

p_err:
	vpu_iinfo("%s():%d\n", vctx, __func__, ret);
	mutex_unlock(lock);
	return ret;
}

static int vpu_vertex_s_param(struct file *file, struct vs4l_param_list *plist)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_graph *graph = container_of(vctx, struct vpu_graph, vctx);
	struct mutex *lock = &vctx->lock;

	if (mutex_lock_interruptible(lock)) {
		vpu_ierr("mutex_lock_interruptible is fail\n", vctx);
		return -ERESTARTSYS;
	}

	if (!(vctx->state & BIT(VPU_VERTEX_START))) {
		vpu_ierr("invalid state(%X)\n", vctx, vctx->state);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_graph_param(graph, plist);
	if (ret) {
		vpu_err("vpu_graph_param is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	vpu_iinfo("%s():%d\n", vctx, __func__, ret);
	mutex_unlock(lock);
	return ret;
}

static int vpu_vertex_s_ctrl(struct file *file, struct vs4l_ctrl *ctrl)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_vertex *vertex = dev_get_drvdata(&vision_devdata(file)->dev);
	struct vpu_device *device = container_of(vertex, struct vpu_device, vertex);
	struct vpu_graph *graph = container_of(vctx, struct vpu_graph, vctx);
	struct mutex *lock = &vctx->lock;

	if (mutex_lock_interruptible(lock)) {
		vpu_ierr("mutex_lock_interruptible is fail\n", vctx);
		return -ERESTARTSYS;
	}

	switch (ctrl->ctrl) {
	case VPU_CTRL_DUMP:
		vpu_graph_print(graph);
		break;
	case VPU_CTRL_MODE:
		device->mode = ctrl->value;
		break;
	case VPU_CTRL_TEST:
		if (ctrl->value == 1) {
			ret = vpu_tv_do_depth3(&device->system.tvset);
			if (ret) {
				vpu_err("vpu_tv_do_depth3 is fail(%d)\n", ret);
				break;
			}
		} else {
			ret = vpu_tv_do_flamorb(&device->system.tvset);
			if (ret) {
				vpu_err("vpu_tv_do_flamorb is fail(%d)\n", ret);
				break;
			}
		}
		break;
	default:
		vpu_ierr("request control is invalid(%d)\n", vctx, ctrl->ctrl);
		ret = -EINVAL;
		break;
	}

	vpu_iinfo("%s(%d):%d\n", vctx, __func__, ctrl->ctrl, ret);
	mutex_unlock(lock);
	return ret;
}

static int vpu_vertex_qbuf(struct file *file, struct vs4l_container_list *clist)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_queue *queue = &vctx->queue;
	struct mutex *lock = &vctx->lock;

	if (mutex_lock_interruptible(lock)) {
		vpu_ierr("mutex_lock_interruptible is fail\n", vctx);
		return -ERESTARTSYS;
	}

	if (!(vctx->state & BIT(VPU_VERTEX_START))) {
		vpu_ierr("(%d) invalid state(%X)\n", vctx, clist->direction, vctx->state);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_queue_qbuf(queue, clist);
	if (ret) {
		vpu_ierr("(%d) vpu_queue_qbuf is fail(%d)\n", vctx, clist->direction, ret);
		goto p_err;
	}

p_err:
	mutex_unlock(lock);
	return ret;
}

static int vpu_vertex_dqbuf(struct file *file, struct vs4l_container_list *clist)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_queue *queue = &vctx->queue;
	struct mutex *lock = &vctx->lock;
	bool nonblocking = file->f_flags & O_NONBLOCK;

	if (mutex_lock_interruptible(lock)) {
		vpu_ierr("mutex_lock_interruptible is fail\n", vctx);
		return -ERESTARTSYS;
	}

	if (!(vctx->state & BIT(VPU_VERTEX_START))) {
		vpu_ierr("(%d) invalid state(%X)\n", vctx, clist->direction, vctx->state);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_queue_dqbuf(queue, clist, nonblocking);
	if (ret) {
		vpu_ierr("(%d) vpu_queue_dqbuf is fail(%d)\n", vctx, clist->direction, ret);
		goto p_err;
	}

p_err:
	mutex_unlock(lock);
	return ret;
}

static int vpu_vertex_streamon(struct file *file)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_vertex *vertex = vctx->vertex;
	struct vpu_queue *queue = &vctx->queue;
	struct mutex *lock = &vctx->lock;

	if (mutex_lock_interruptible(lock)) {
		vpu_ierr("mutex_lock_interruptible is fail\n", vctx);
		return -ERESTARTSYS;
	}

	if (!(vctx->state & (BIT(VPU_VERTEX_FORMAT) | BIT(VPU_VERTEX_STOP)))) {
		vpu_ierr("invalid state(%X)\n", vctx, vctx->state);
		ret = -EINVAL;
		goto p_err;
	}

	ret = __vref_get(&vertex->start_cnt);
	if (ret) {
		vpu_err("vref_get is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_queue_start(queue);
	if (ret) {
		vpu_ierr("vpu_queue_start is fail(%d)\n", vctx, ret);
		goto p_err;
	}

	vctx->state = BIT(VPU_VERTEX_START);

p_err:
	vpu_iinfo("%s():%d\n", vctx, __func__, ret);
	mutex_unlock(lock);
	return ret;
}

static int vpu_vertex_streamoff(struct file *file)
{
	int ret = 0;
	struct vpu_vertex_ctx *vctx = file->private_data;
	struct vpu_vertex *vertex = vctx->vertex;
	struct vpu_queue *queue = &vctx->queue;
	struct mutex *lock = &vctx->lock;

	if (mutex_lock_interruptible(lock)) {
		vpu_ierr("mutex_lock_interruptible is fail\n", vctx);
		return -ERESTARTSYS;
	}

	if (!(vctx->state & BIT(VPU_VERTEX_START))) {
		vpu_ierr("invalid state(%X)\n", vctx, vctx->state);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_queue_stop(queue);
	if (ret) {
		vpu_ierr("vpu_queue_stop is fail(%d)\n", vctx, ret);
		goto p_err;
	}

	ret = __vref_put(&vertex->start_cnt);
	if (ret) {
		vpu_err("vref_put is fail(%d)\n", ret);
		goto p_err;
	}

	vctx->state = BIT(VPU_VERTEX_STOP);

p_err:
	vpu_iinfo("%s():%d\n", vctx, __func__, ret);
	mutex_unlock(lock);
	return ret;
}

const struct vertex_ioctl_ops vpu_vertex_ioctl_ops = {
	.vertexioc_s_graph	= vpu_vertex_s_graph,
	.vertexioc_s_format	= vpu_vertex_s_format,
	.vertexioc_s_param	= vpu_vertex_s_param,
	.vertexioc_s_ctrl	= vpu_vertex_s_ctrl,
	.vertexioc_qbuf		= vpu_vertex_qbuf,
	.vertexioc_dqbuf	= vpu_vertex_dqbuf,
	.vertexioc_streamon	= vpu_vertex_streamon,
	.vertexioc_streamoff	= vpu_vertex_streamoff
};
