/*
 * linux/drivers/gpu/exynos/g2d/g2d_fence.c
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fence.h>
#include <linux/sync_file.h>

#include "g2d.h"
#include "g2d_uapi.h"
#include "g2d_task.h"
#include "g2d_fence.h"
#include "g2d_debug.h"

void g2d_fence_timeout_handler(unsigned long arg)
{
	struct g2d_task *task = (struct g2d_task *)arg;
	struct g2d_device *g2d_dev = task->g2d_dev;
	struct fence *fence;
	unsigned long flags;
	char name[32];
	int i;
	s32 afbc = 0;

	for (i = 0; i < task->num_source; i++) {
		fence = task->source[i].fence;
		if (fence) {
			strlcpy(name, fence->ops->get_driver_name(fence),
			       sizeof(name));
			dev_err(g2d_dev->dev, "%s:  SOURCE[%d]:  %s #%d (%s)\n",
				__func__, i, name, fence->seqno,
				fence_is_signaled(fence) ?
					"signaled" : "active");
		}
	}

	fence = task->target.fence;
	if (fence) {
		strlcpy(name, fence->ops->get_driver_name(fence), sizeof(name));
		pr_err("%s:  TARGET:     %s #%d (%s)\n",
			__func__, name, fence->seqno,
			fence_is_signaled(fence) ? "signaled" : "active");
	}

	if (task->release_fence)
		pr_err("%s:    Pending g2d release fence: #%d\n",
			__func__, task->release_fence->fence->seqno);

	/*
	 * Give up waiting the acquire fences that are not currently signaled
	 * and force pushing this pending task to the H/W to avoid indefinite
	 * wait for the fences to be signaled.
	 * The reference count is required to prevent racing about the
	 * acqure fences between this time handler and the fence callback.
	 */
	spin_lock_irqsave(&task->fence_timeout_lock, flags);

	/*
	 * Make sure if there is really a unsignaled fences. task->starter is
	 * decremented under fence_timeout_lock held if it is done by fence
	 * signal.
	 */
	if (atomic_read(&task->starter.refcount) == 0) {
		spin_unlock_irqrestore(&task->fence_timeout_lock, flags);
		pr_err("All fences have been signaled. (work_busy? %d)\n",
			work_busy(&task->work));
		return;
	}

	pr_err("%s: %d Fence(s) timed out after %d msec.\n", __func__,
		atomic_read(&task->starter.refcount), G2D_FENCE_TIMEOUT_MSEC);

	/* Increase reference to prevent running the workqueue in callback */
	kref_get(&task->starter);

	spin_unlock_irqrestore(&task->fence_timeout_lock, flags);

	for (i = 0; i < task->num_source; i++) {
		fence = task->source[i].fence;
		if (fence)
			fence_remove_callback(fence, &task->source[i].fence_cb);
	}

	fence = task->target.fence;
	if (fence)
		fence_remove_callback(fence, &task->target.fence_cb);

	/* check compressed buffer because crashed buffer makes recovery */
	for (i = 0; i < task->num_source; i++) {
		if (IS_AFBC(
			task->source[i].commands[G2DSFR_IMG_COLORMODE].value))
			afbc |= 1 << i;
	}

	if (IS_AFBC(task->target.commands[G2DSFR_IMG_COLORMODE].value))
		afbc |= 1 << G2D_MAX_IMAGES;

	g2d_stamp_task(task, G2D_STAMP_STATE_TIMEOUT_FENCE, afbc);

	g2d_queuework_task(&task->starter);
};

static const char *g2d_fence_get_driver_name(struct fence *fence)
{
	return "g2d";
}

static bool g2d_fence_enable_signaling(struct fence *fence)
{
	/* nothing to do */
	return true;
}

static void g2d_fence_release(struct fence *fence)
{
	kfree(fence);
}

static void g2d_fence_value_str(struct fence *fence, char *str, int size)
{
	snprintf(str, size, "%d", fence->seqno);
}

static struct fence_ops g2d_fence_ops = {
	.get_driver_name =	g2d_fence_get_driver_name,
	.get_timeline_name =	g2d_fence_get_driver_name,
	.enable_signaling =	g2d_fence_enable_signaling,
	.wait =			fence_default_wait,
	.release =		g2d_fence_release,
	.fence_value_str =	g2d_fence_value_str,
};

struct sync_file *g2d_create_release_fence(struct g2d_device *g2d_dev,
					   struct g2d_task *task,
					   struct g2d_task_data *data)
{
	struct fence *fence;
	struct sync_file *file;
	s32 release_fences[G2D_MAX_IMAGES + 1];
	int i;
	int ret = 0;

	if (!(task->flags & G2D_FLAG_NONBLOCK) || !data->num_release_fences)
		return NULL;

	if (data->num_release_fences > (task->num_source + 1)) {
		dev_err(g2d_dev->dev,
			"%s: Too many release fences %d required (src: %d)\n",
			__func__, data->num_release_fences, task->num_source);
		return ERR_PTR(-EINVAL);
	}

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence)
		return ERR_PTR(-ENOMEM);

	fence_init(fence, &g2d_fence_ops, &g2d_dev->fence_lock,
		   g2d_dev->fence_context,
		   atomic_inc_return(&g2d_dev->fence_timeline));

	file = sync_file_create(fence);
	fence_put(fence);
	if (!file)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < data->num_release_fences; i++) {
		release_fences[i] = get_unused_fd_flags(O_CLOEXEC);
		if (release_fences[i] < 0) {
			ret = release_fences[i];
			while (i-- > 0)
				put_unused_fd(release_fences[i]);
			goto err_fd;
		}
	}

	if (copy_to_user(data->release_fences, release_fences,
				sizeof(u32) * data->num_release_fences)) {
		ret = -EFAULT;
		dev_err(g2d_dev->dev,
			"%s: Failed to copy release fences to userspace\n",
			__func__);
		goto err_fd;
	}

	for (i = 0; i < data->num_release_fences; i++)
		fd_install(release_fences[i], get_file(file->file));

	return file;
err_fd:
	while (i-- > 0)
		put_unused_fd(release_fences[i]);
	fput(file->file);

	return ERR_PTR(ret);
}

static void g2d_check_valid_fence(struct fence *fence, s32 fence_fd)
{
	struct file *file;
	struct sync_file *sync_file;

	if (fence->magic_bit == 0xFECEFECE)
		return;
	/*
	 * Caution:
	 * This file of fence_fd doesn't have problem because this file's
	 * fops is sync_file_fops, and we get reference for that. However
	 * this fence of sync_file might be corrupted or released because
	 * magic_bit is not 0xFECEFECE that is initialized on fence_init.
	 *
	 * Print information to find fence owner.
	 */
	file = fget(fence_fd);
	sync_file = file->private_data;

	pr_err("%s : sync_file@%p : ref %d name %s\n", __func__,
	       sync_file, atomic_read(&sync_file->kref.refcount),
	       sync_file->name);
	pr_err("%s : fence@%p : ref %d lock %p context %lu\n", __func__,
	       fence, atomic_read(&fence->refcount.refcount),
	       fence->lock, (unsigned long)fence->context);
	pr_err(" timestamp %ld status %d magic_bit %#x\n",
	       (long)fence->timestamp.tv64, fence->status, fence->magic_bit);

	fput(file);
}

struct fence *g2d_get_acquire_fence(struct g2d_device *g2d_dev,
				    struct g2d_layer *layer, s32 fence_fd)
{
	struct fence *fence;
	int ret;

	if (!(layer->flags & G2D_LAYERFLAG_ACQUIRE_FENCE))
		return NULL;

	fence = sync_file_get_fence(fence_fd);
	if (!fence)
		return ERR_PTR(-EINVAL);

	g2d_check_valid_fence(fence, fence_fd);

	kref_get(&layer->task->starter);

	ret = fence_add_callback(fence, &layer->fence_cb, g2d_fence_callback);
	if (ret < 0) {
		kref_put(&layer->task->starter, g2d_queuework_task);
		fence_put(fence);
		return (ret == -ENOENT) ? NULL : ERR_PTR(ret);
	}

	return fence;
}
