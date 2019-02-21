/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is group manager functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
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
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/v4l2-mediabus.h>
#include <linux/bug.h>

#include "fimc-is-type.h"
#include "fimc-is-core.h"
#include "fimc-is-err.h"
#include "fimc-is-video.h"
#include "fimc-is-framemgr.h"
#include "fimc-is-groupmgr.h"
#include "fimc-is-devicemgr.h"
#include "fimc-is-cmd.h"
#include "fimc-is-dvfs.h"
#include "fimc-is-debug.h"
#include "fimc-is-hw.h"
#include "fimc-is-vender.h"
#ifdef CONFIG_USE_DIRECT_IS_CONTROL
#include "fimc-is-interface-wrap.h"
#endif
#ifdef CONFIG_COMPANION_DIRECT_USE
#include "fimc-is-interface-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#endif

/* sysfs variable for debug */
extern struct fimc_is_sysfs_debug sysfs_debug;

static inline void smp_shot_init(struct fimc_is_group *group, u32 value)
{
	sema_init(&group->smp_shot, value);
	atomic_set(&group->smp_shot_count, value);
}

static inline int smp_shot_get(struct fimc_is_group *group)
{
	return atomic_read(&group->smp_shot_count);
}

static inline int smp_shot_inc(struct fimc_is_group *group)
{
	atomic_inc(&group->smp_shot_count);
	up(&group->smp_shot);
	return 0;
}

static inline int smp_shot_dec(struct fimc_is_group *group)
{
	int ret = 0;

	ret = down_interruptible(&group->smp_shot);
	if (ret) {
		mgerr(" down fail(%d)", group, group, ret);
		goto p_err;
	}

	atomic_dec(&group->smp_shot_count);

p_err:
	return ret;
}

static void fimc_is_gframe_s_info(struct fimc_is_group_frame *gframe,
	u32 slot, struct fimc_is_frame *frame)
{
	BUG_ON(!gframe);
	BUG_ON(!frame);
	BUG_ON(!frame->shot_ext);
	BUG_ON(slot >= GROUP_SLOT_MAX);

	memcpy(&gframe->group_cfg[slot], &frame->shot_ext->node_group,
		sizeof(struct camera2_node_group));
}

static void fimc_is_gframe_free_head(struct fimc_is_group_framemgr *gframemgr,
	struct fimc_is_group_frame **gframe)
{
	if (gframemgr->gframe_cnt)
		*gframe = container_of(gframemgr->gframe_head.next, struct fimc_is_group_frame, list);
	else
		*gframe = NULL;
}

static void fimc_is_gframe_s_free(struct fimc_is_group_framemgr *gframemgr,
	struct fimc_is_group_frame *gframe)
{
	BUG_ON(!gframemgr);
	BUG_ON(!gframe);

	list_add_tail(&gframe->list, &gframemgr->gframe_head);
	gframemgr->gframe_cnt++;
}

static void fimc_is_gframe_print_free(struct fimc_is_group_framemgr *gframemgr)
{
	struct fimc_is_group_frame *gframe, *temp;

	BUG_ON(!gframemgr);

	printk(KERN_ERR "[GFM] fre(%d) :", gframemgr->gframe_cnt);

	list_for_each_entry_safe(gframe, temp, &gframemgr->gframe_head, list) {
		printk(KERN_CONT "%d->", gframe->fcount);
	}

	printk(KERN_CONT "X\n");
}

static void fimc_is_gframe_group_head(struct fimc_is_group *group,
	struct fimc_is_group_frame **gframe)
{
	if (group->gframe_cnt)
		*gframe = container_of(group->gframe_head.next, struct fimc_is_group_frame, list);
	else
		*gframe = NULL;
}

static void fimc_is_gframe_s_group(struct fimc_is_group *group,
	struct fimc_is_group_frame *gframe)
{
	BUG_ON(!group);
	BUG_ON(!gframe);

	list_add_tail(&gframe->list, &group->gframe_head);
	group->gframe_cnt++;
}

static void fimc_is_gframe_print_group(struct fimc_is_group *group)
{
	struct fimc_is_group_frame *gframe, *temp;

	while (group) {
		printk(KERN_ERR "[GP%d] req(%d) :", group->id, group->gframe_cnt);

		list_for_each_entry_safe(gframe, temp, &group->gframe_head, list) {
			printk(KERN_CONT "%d->", gframe->fcount);
		}

		printk(KERN_CONT "X\n");

		group = group->gnext;
	}
}

static int fimc_is_gframe_check(struct fimc_is_group *gprev,
	struct fimc_is_group *group,
	struct fimc_is_group *gnext,
	struct fimc_is_group_frame *gframe,
	struct fimc_is_frame *frame)
{
	int ret = 0;
	u32 capture_id;
	struct fimc_is_device_ischain *device;
	struct fimc_is_crop *incrop, *otcrop, *canv;
	struct fimc_is_subdev *subdev, *junction;
	struct camera2_node *node;

	BUG_ON(!group);
	BUG_ON(!group->device);
	BUG_ON(group->slot >= GROUP_SLOT_MAX);
	BUG_ON(gprev && (gprev->slot >= GROUP_SLOT_MAX));

	device = group->device;

	/*
	 * perframe check
	 * 1. perframe size can't exceed s_format size
	 */
	incrop = (struct fimc_is_crop *)gframe->group_cfg[group->slot].leader.input.cropRegion;
	subdev = &group->leader;
	if ((incrop->w * incrop->h) > (subdev->input.width * subdev->input.height)) {
		mrwarn("the input size is invalid(%dx%d > %dx%d)", group, gframe,
			incrop->w, incrop->h, subdev->input.width, subdev->input.height);
		incrop->w = subdev->input.width;
		incrop->h = subdev->input.height;
		frame->shot_ext->node_group.leader.input.cropRegion[2] = incrop->w;
		frame->shot_ext->node_group.leader.input.cropRegion[3] = incrop->h;
	}

	for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
		node = &gframe->group_cfg[group->slot].capture[capture_id];
		if (node->vid == 0) /* no effect */
			continue;

		otcrop = (struct fimc_is_crop *)node->output.cropRegion;
		subdev = video2subdev(FIMC_IS_ISCHAIN_SUBDEV, (void *)device, node->vid);
		if (!subdev) {
			mgerr("subdev is NULL", group, group);
			ret = -EINVAL;
			node->request = 0;
			node->vid = 0;
			goto p_err;
		}
/*
		mrwarn("[V%d][req:%d] the output size is invalid(perframe:%dx%d > subdev:%dx%d)", group, gframe,
			node->vid, node->request,
			otcrop->w, otcrop->h, subdev->output.width, subdev->output.height);
*/

		if ((otcrop->w * otcrop->h) > (subdev->output.width * subdev->output.height)) {
			mrwarn("[V%d][req:%d] the output size is invalid(perframe:%dx%d > subdev:%dx%d)", group, gframe,
				node->vid, node->request,
				otcrop->w, otcrop->h, subdev->output.width, subdev->output.height);
			otcrop->w = subdev->output.width;
			otcrop->h = subdev->output.height;
			frame->shot_ext->node_group.capture[capture_id].output.cropRegion[2] = otcrop->w;
			frame->shot_ext->node_group.capture[capture_id].output.cropRegion[3] = otcrop->h;
		}

		subdev->cid = capture_id;
	}

	/*
	 * junction check
	 * 1. skip if previous is empty
	 * 2. previous capture size should be bigger than current leader size
	 */
	if (!gprev)
		goto check_gnext;

	junction = gprev->junction;
	if (!junction) {
		mgerr("junction is NULL", gprev, gprev);
		ret = -EINVAL;
		goto p_err;
	}

	if (junction->cid >= CAPTURE_NODE_MAX) {
		mgerr("capture id(%d) is invalid", gprev, gprev, junction->cid);
		ret = -EFAULT;
		goto p_err;
	}

	canv = &gframe->canv;
	incrop = (struct fimc_is_crop *)gframe->group_cfg[group->slot].leader.input.cropRegion;

	if ((canv->w * canv->h) < (incrop->w * incrop->h)) {
		mrwarn("input crop size is bigger than output size of previous group(GP%d(%d,%d,%d,%d) < GP%d(%d,%d,%d,%d))",
			group, gframe,
			gprev->id, canv->x, canv->y, canv->w, canv->h,
			group->id, incrop->x, incrop->y, incrop->w, incrop->h);
		*incrop = *canv;
		frame->shot_ext->node_group.leader.input.cropRegion[0] = incrop->x;
		frame->shot_ext->node_group.leader.input.cropRegion[1] = incrop->y;
		frame->shot_ext->node_group.leader.input.cropRegion[2] = incrop->w;
		frame->shot_ext->node_group.leader.input.cropRegion[3] = incrop->h;
	}

	/* set input size of current group as output size of previous group */
	group->leader.input.canv = *canv;

check_gnext:
	/*
	 * junction check
	 * 1. skip if next is empty
	 * 2. current capture size should be smaller than next leader size.
	 */
	if (!gnext)
		goto p_err;

	junction = group->tail->junction;
	if (!junction) {
		mgerr("junction is NULL", group, group);
		ret = -EINVAL;
		goto p_err;
	}

	if (junction->cid >= CAPTURE_NODE_MAX) {
		mgerr("capture id(%d) is invalid", group, group, junction->cid);
		ret = -EFAULT;
		goto p_err;
	}

	/* When dma request for next group is empty, this size doesn`t have to be checked. */
	if (gframe->group_cfg[group->slot].capture[junction->cid].request == 0)
		goto p_err;

	otcrop = (struct fimc_is_crop *)gframe->group_cfg[group->slot].capture[junction->cid].output.cropRegion;
	subdev = &gnext->leader;
	if ((otcrop->w * otcrop->h) > (subdev->input.width * subdev->input.height)) {
		mrwarn("the output size bigger than input size of next group(GP%d(%dx%d) > GP%d(%dx%d))",
			group, gframe,
			group->id, otcrop->w, otcrop->h,
			gnext->id, subdev->input.width, subdev->input.height);
		otcrop->w = subdev->input.width;
		otcrop->h = subdev->input.height;
		frame->shot_ext->node_group.capture[junction->cid].output.cropRegion[2] = otcrop->w;
		frame->shot_ext->node_group.capture[junction->cid].output.cropRegion[3] = otcrop->h;
	}

	/* set canvas size of next group as output size of currnet group */
	gframe->canv = *otcrop;

p_err:
	return ret;
}

static int fimc_is_frame_check_without_gframe(struct fimc_is_group *group,
	struct fimc_is_frame *frame)
{
	int ret = 0;
	u32 capture_id;
	struct fimc_is_device_ischain *device;
	struct fimc_is_crop *incrop, *otcrop;
	struct fimc_is_subdev *subdev;
	struct camera2_node *node;

	BUG_ON(!group);
	BUG_ON(!group->device);
	BUG_ON(group->slot >= GROUP_SLOT_MAX);

	device = group->device;

	/*
	 * perframe check
	 * 1. perframe size can't exceed s_format size
	 */
	incrop = (struct fimc_is_crop *)frame->shot_ext->node_group.leader.input.cropRegion;
	subdev = &group->leader;
	if ((incrop->w * incrop->h) > (subdev->input.width * subdev->input.height)) {
		mrwarn("the input size is invalid(%dx%d > %dx%d)", group, frame,
			incrop->w, incrop->h, subdev->input.width, subdev->input.height);
		incrop->w = subdev->input.width;
		incrop->h = subdev->input.height;
	}

	for (capture_id = 0; capture_id < CAPTURE_NODE_MAX; ++capture_id) {
		node = &frame->shot_ext->node_group.capture[capture_id];
		if (node->vid == 0) /* no effect */
			continue;

		otcrop = (struct fimc_is_crop *)node->output.cropRegion;
		subdev = video2subdev(FIMC_IS_ISCHAIN_SUBDEV, (void *)device, node->vid);
		if (!subdev) {
			mgerr("subdev is NULL", group, group);
			ret = -EINVAL;
			node->request = 0;
			node->vid = 0;
			goto p_err;
		}

		if ((otcrop->w * otcrop->h) > (subdev->output.width * subdev->output.height)) {
			mrwarn("[V%d][req:%d] the output size is invalid(perframe:%dx%d > subdev:%dx%d)", group, frame,
				node->vid, node->request,
				otcrop->w, otcrop->h, subdev->output.width, subdev->output.height);
			otcrop->w = subdev->output.width;
			otcrop->h = subdev->output.height;
		}

		subdev->cid = capture_id;
	}

p_err:
	return ret;
}

static int fimc_is_gframe_trans_fre_to_grp(struct fimc_is_group_framemgr *gframemgr,
	struct fimc_is_group_frame *gframe,
	struct fimc_is_group *group,
	struct fimc_is_group *gnext)
{
	int ret = 0;

	BUG_ON(!gframemgr);
	BUG_ON(!gframe);
	BUG_ON(!group);
	BUG_ON(!gnext);
	BUG_ON(!group->tail);
	BUG_ON(!group->tail->junction);

	if (unlikely(!gframemgr->gframe_cnt)) {
		merr("gframe_cnt is zero", group);
		ret = -EFAULT;
		goto p_err;
	}

	if (gframe->group_cfg[group->slot].capture[group->tail->junction->cid].request) {
		list_del(&gframe->list);
		gframemgr->gframe_cnt--;
		fimc_is_gframe_s_group(gnext, gframe);
	}

p_err:
	return ret;
}

static int fimc_is_gframe_trans_grp_to_grp(struct fimc_is_group_framemgr *gframemgr,
	struct fimc_is_group_frame *gframe,
	struct fimc_is_group *group,
	struct fimc_is_group *gnext)
{
	int ret = 0;

	BUG_ON(!gframemgr);
	BUG_ON(!gframe);
	BUG_ON(!group);
	BUG_ON(!gnext);
	BUG_ON(!group->tail);
	BUG_ON(!group->tail->junction);

	if (unlikely(!group->gframe_cnt)) {
		merr("gframe_cnt is zero", group);
		ret = -EFAULT;
		goto p_err;
	}

	if (gframe->group_cfg[group->slot].capture[group->tail->junction->cid].request) {
		list_del(&gframe->list);
		group->gframe_cnt--;
		fimc_is_gframe_s_group(gnext, gframe);
	} else {
		list_del(&gframe->list);
		group->gframe_cnt--;
		fimc_is_gframe_s_free(gframemgr, gframe);
	}

p_err:
	return ret;
}

static int fimc_is_gframe_trans_grp_to_fre(struct fimc_is_group_framemgr *gframemgr,
	struct fimc_is_group_frame *gframe,
	struct fimc_is_group *group)
{
	int ret = 0;

	BUG_ON(!gframemgr);
	BUG_ON(!gframe);
	BUG_ON(!group);

	if (!group->gframe_cnt) {
		merr("gframe_cnt is zero", group);
		ret = -EFAULT;
		goto p_err;
	}

	list_del(&gframe->list);
	group->gframe_cnt--;
	fimc_is_gframe_s_free(gframemgr, gframe);

p_err:
	return ret;
}

int fimc_is_gframe_cancel(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group, u32 target_fcount)
{
	int ret = -EINVAL;
	struct fimc_is_group_framemgr *gframemgr;
	struct fimc_is_group_frame *gframe, *temp;
	ulong flags;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);

	gframemgr = &groupmgr->gframemgr[group->instance];

	spin_lock_irqsave(&gframemgr->gframe_slock, flags);

	list_for_each_entry_safe(gframe, temp, &group->gframe_head, list) {
		if (gframe->fcount == target_fcount) {
			list_del(&gframe->list);
			group->gframe_cnt--;
			mwarn("gframe%d is cancelled", group, target_fcount);
			fimc_is_gframe_s_free(gframemgr, gframe);
			ret = 0;
			break;
		}
	}

	spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);

	return ret;
}

void * fimc_is_gframe_rewind(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group, u32 target_fcount)
{
	struct fimc_is_group_framemgr *gframemgr;
	struct fimc_is_group_frame *gframe, *temp;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);

	gframemgr = &groupmgr->gframemgr[group->instance];

	list_for_each_entry_safe(gframe, temp, &group->gframe_head, list) {
		if (gframe->fcount == target_fcount)
			break;

		if (gframe->fcount > target_fcount) {
			mgwarn("qbuf fcount(%d) is smaller than expect fcount(%d)", group, group,
				target_fcount, gframe->fcount);
			break;
		}

		list_del(&gframe->list);
		group->gframe_cnt--;
		mgwarn("gframe%d is cancel(count : %d)", group, group, gframe->fcount, group->gframe_cnt);
		fimc_is_gframe_s_free(gframemgr, gframe);
	}

	if (!group->gframe_cnt) {
		merr("gframe%d can't be found", group, target_fcount);
		gframe = NULL;
	}

	return gframe;
}

int fimc_is_gframe_flush_all(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group)
{
	int ret = 0;
	unsigned long flag;
	struct fimc_is_group_framemgr *gframemgr;
	struct fimc_is_group_frame *gframe, *temp;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);

	gframemgr = &groupmgr->gframemgr[group->instance];

	spin_lock_irqsave(&gframemgr->gframe_slock, flag);

	list_for_each_entry_safe(gframe, temp, &group->gframe_head, list) {
		list_del(&gframe->list);
		group->gframe_cnt--;
		fimc_is_gframe_s_free(gframemgr, gframe);
	}

	spin_unlock_irqrestore(&gframemgr->gframe_slock, flag);

	return ret;
}

int fimc_is_gframe_flush(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group, int remain)
{
	int ret = 0;
	unsigned long flag;
	struct fimc_is_group_framemgr *gframemgr;
	struct fimc_is_group_frame *gframe, *temp;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);

	gframemgr = &groupmgr->gframemgr[group->instance];

	spin_lock_irqsave(&gframemgr->gframe_slock, flag);
	if (group->gframe_cnt <= remain) {
		spin_unlock_irqrestore(&gframemgr->gframe_slock, flag);
		return 0;
	}

	list_for_each_entry_safe(gframe, temp, &group->gframe_head, list) {
		list_del(&gframe->list);
		group->gframe_cnt--;
		fimc_is_gframe_s_free(gframemgr, gframe);

		if (group->gframe_cnt == remain)
			break;

	}
	spin_unlock_irqrestore(&gframemgr->gframe_slock, flag);

	return ret;
}
unsigned long fimc_is_group_lock(struct fimc_is_group *group,
		enum fimc_is_device_type device_type,
		bool leader_lock)
{
	u32 entry;
	unsigned long flags = 0;
	struct fimc_is_subdev *subdev;
	struct fimc_is_framemgr *ldr_framemgr, *sub_framemgr;

	BUG_ON(!group);

	if (leader_lock) {
		ldr_framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
		if (!ldr_framemgr) {
			mgerr("ldr_framemgr is NULL", group, group);
			BUG();
		}
		framemgr_e_barrier_irqs(ldr_framemgr, FMGR_IDX_20, flags);
	}

	while (group) {
		/* skip the lock by device type */
		if (group->device_type != device_type &&
				device_type != FIMC_IS_DEVICE_MAX)
			break;

		for (entry = 0; entry < ENTRY_END; ++entry) {
			subdev = group->subdev[entry];
			group->locked_sub_framemgr[entry] = NULL;

			if (!subdev)
				continue;

			if (&group->leader == subdev)
				continue;

			sub_framemgr = GET_SUBDEV_FRAMEMGR(subdev);
			if (!sub_framemgr)
				continue;

			if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state))
				continue;

			framemgr_e_barrier(sub_framemgr, FMGR_IDX_19);
			group->locked_sub_framemgr[entry] = sub_framemgr;
		}

		group = group->child;
	}

	return flags;
}

void fimc_is_group_unlock(struct fimc_is_group *group, unsigned long flags,
		enum fimc_is_device_type device_type,
		bool leader_lock)
{
	u32 entry;
	struct fimc_is_framemgr *ldr_framemgr;

	BUG_ON(!group);

	if (leader_lock) {
		ldr_framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
		if (!ldr_framemgr) {
			mgerr("ldr_framemgr is NULL", group, group);
			BUG();
		}
	}

	while (group) {
		/* skip the unlock by device type */
		if (group->device_type != device_type &&
				device_type != FIMC_IS_DEVICE_MAX)
			break;

		for (entry = 0; entry < ENTRY_END; ++entry) {
			if (group->locked_sub_framemgr[entry]) {
				framemgr_x_barrier(group->locked_sub_framemgr[entry],
							FMGR_IDX_19);
				group->locked_sub_framemgr[entry] = NULL;
			}
		}

		group = group->child;
	}

	if (leader_lock)
		framemgr_x_barrier_irqr(ldr_framemgr, FMGR_IDX_20, flags);
}

void fimc_is_group_subdev_cancel(struct fimc_is_group *group,
		struct fimc_is_frame *ldr_frame,
		enum fimc_is_device_type device_type,
		enum fimc_is_frame_state frame_state,
		bool flush)
{
	u32 entry;
	struct fimc_is_subdev *subdev;
	struct fimc_is_video_ctx *sub_vctx;
	struct fimc_is_framemgr *sub_framemgr;
	struct fimc_is_frame *sub_frame;

	while (group) {
		/* skip the subdev device by device type */
		if (group->device_type != device_type &&
				device_type != FIMC_IS_DEVICE_MAX)
			break;

		for (entry = 0; entry < ENTRY_END; ++entry) {
			subdev = group->subdev[entry];

			if (!subdev)
				continue;

			sub_vctx = subdev->vctx;
			if (!sub_vctx)
				continue;

			sub_framemgr = GET_FRAMEMGR(sub_vctx);
			if (!sub_framemgr)
				continue;

			if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state))
				continue;

			if (subdev->cid >= CAPTURE_NODE_MAX)
				continue;

			if (!flush && ldr_frame->shot_ext->node_group.capture[subdev->cid].request == 0)
				continue;

			do {
				sub_frame = peek_frame(sub_framemgr, frame_state);
				if (sub_frame) {
					sub_frame->stream->fvalid = 0;
					sub_frame->stream->fcount = ldr_frame->fcount;
					sub_frame->stream->rcount = ldr_frame->rcount;
					clear_bit(subdev->id, &ldr_frame->out_flag);
					trans_frame(sub_framemgr, sub_frame, FS_COMPLETE);
					msrinfo("[ERR] CANCEL(%d, %d)\n", group, subdev, ldr_frame, sub_frame->index, frame_state);
					CALL_VOPS(sub_vctx, done, sub_frame->index, VB2_BUF_STATE_ERROR);
				} else {
					msrinfo("[ERR] There's no frame\n", group, subdev, ldr_frame);
#ifdef DEBUG
					frame_manager_print_queues(sub_framemgr);
#endif
				}
			} while (sub_frame && flush);
		}

		group = group->child;
	}
}

static void fimc_is_group_cancel(struct fimc_is_group *group,
	struct fimc_is_frame *ldr_frame)
{
	u32 wait_count = 300;
	unsigned long flags;
	struct fimc_is_video_ctx *ldr_vctx;
	struct fimc_is_framemgr *ldr_framemgr;
	struct fimc_is_frame *prev_frame, *next_frame;

	BUG_ON(!group);
	BUG_ON(!ldr_frame);

	ldr_vctx = group->head->leader.vctx;
	ldr_framemgr = GET_FRAMEMGR(ldr_vctx);
	if (!ldr_framemgr) {
		mgerr("ldr_framemgr is NULL", group, group);
		BUG();
	}

p_retry:
	flags = fimc_is_group_lock(group, FIMC_IS_DEVICE_MAX, true);

	next_frame = peek_frame_tail(ldr_framemgr, FS_FREE);
	if (wait_count && next_frame && next_frame->out_flag) {
		mginfo("next frame(F%d) is on process1(%lX %lX), waiting...\n", group, group,
			next_frame->fcount, next_frame->bak_flag, next_frame->out_flag);
		fimc_is_group_unlock(group, flags, FIMC_IS_DEVICE_MAX, true);
		usleep_range(1000, 1000);
		wait_count--;
		goto p_retry;
	}

	next_frame = peek_frame_tail(ldr_framemgr, FS_COMPLETE);
	if (wait_count && next_frame && next_frame->out_flag) {
		mginfo("next frame(F%d) is on process2(%lX %lX), waiting...\n", group, group,
			next_frame->fcount, next_frame->bak_flag, next_frame->out_flag);
		fimc_is_group_unlock(group, flags, FIMC_IS_DEVICE_MAX, true);
		usleep_range(1000, 1000);
		wait_count--;
		goto p_retry;
	}

	prev_frame = peek_frame(ldr_framemgr, FS_PROCESS);
	if (wait_count && prev_frame && prev_frame->bak_flag != prev_frame->out_flag) {
		mginfo("prev frame(F%d) is on process(%lX %lX), waiting...\n", group, group,
			prev_frame->fcount, prev_frame->bak_flag, prev_frame->out_flag);
		fimc_is_group_unlock(group, flags, FIMC_IS_DEVICE_MAX, true);
		usleep_range(1000, 1000);
		wait_count--;
		goto p_retry;
	}

	fimc_is_group_subdev_cancel(group, ldr_frame, FIMC_IS_DEVICE_MAX, FS_REQUEST, false);

	clear_bit(group->leader.id, &ldr_frame->out_flag);
	trans_frame(ldr_framemgr, ldr_frame, FS_COMPLETE);
	mgrinfo("[ERR] CANCEL(%d)\n", group, group, ldr_frame, ldr_frame->index);
	CALL_VOPS(ldr_vctx, done, ldr_frame->index, VB2_BUF_STATE_ERROR);

	fimc_is_group_unlock(group, flags, FIMC_IS_DEVICE_MAX, true);
}

static void fimc_is_group_s_leader(struct fimc_is_group *group,
	struct fimc_is_subdev *leader)
{
	u32 entry;
	struct fimc_is_subdev *subdev;

	BUG_ON(!group);
	BUG_ON(!leader);

	subdev = &group->leader;
	subdev->leader = leader;

	for (entry = 0; entry < ENTRY_END; ++entry) {
		subdev = group->subdev[entry];
		if (subdev) {
			if (leader->vctx && subdev->vctx &&
				(leader->vctx->refcount < subdev->vctx->refcount)) {
				mgwarn("Invalide subdev instance (%s(%u) < %s(%u))",
					group, group,
					leader->name, leader->vctx->refcount,
					subdev->name, subdev->vctx->refcount);
			}
			subdev->leader = leader;
		}
	}
}

static void fimc_is_stream_status(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group)
{
	unsigned long flags;
	struct fimc_is_queue *queue;
	struct fimc_is_framemgr *framemgr;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(group->id >= GROUP_ID_MAX);

	while (group) {
		BUG_ON(!group->leader.vctx);

		queue = GET_SUBDEV_QUEUE(&group->leader);
		framemgr = &queue->framemgr;

		mginfo(" ginfo(res %d, rcnt %d, pos %d)\n", group, group,
			groupmgr->gtask[group->id].smp_resource.count,
			atomic_read(&group->rcount),
			group->pcount);
		mginfo(" vinfo(req %d, pre %d, que %d, com %d, dqe %d)\n", group, group,
			queue->buf_req,
			queue->buf_pre,
			queue->buf_que,
			queue->buf_com,
			queue->buf_dqe);

		/* print framemgr's frame info */
		framemgr_e_barrier_irqs(framemgr, 0, flags);
		frame_manager_print_queues(framemgr);
		framemgr_x_barrier_irqr(framemgr, 0, flags);

		group = group->gnext;
	}
}

#ifdef DEBUG_AA
static void fimc_is_group_debug_aa_shot(struct fimc_is_group *group,
	struct fimc_is_frame *ldr_frame)
{
	if (group->prev)
		return;

#ifdef DEBUG_FLASH
	if (ldr_frame->shot->ctl.aa.vendor_aeflashMode != group->flashmode) {
		group->flashmode = ldr_frame->shot->ctl.aa.vendor_aeflashMode;
		info("flash ctl : %d(%d)\n", group->flashmode, ldr_frame->fcount);
	}
#endif
}

static void fimc_is_group_debug_aa_done(struct fimc_is_group *group,
	struct fimc_is_frame *ldr_frame)
{
	if (group->prev)
		return;

#ifdef DEBUG_FLASH
	if (ldr_frame->shot->dm.flash.firingStable != group->flash.firingStable) {
		group->flash.firingStable = ldr_frame->shot->dm.flash.firingStable;
		info("flash stable : %d(%d)\n", group->flash.firingStable, ldr_frame->fcount);
	}

	if (ldr_frame->shot->dm.flash.flashReady!= group->flash.flashReady) {
		group->flash.flashReady = ldr_frame->shot->dm.flash.flashReady;
		info("flash ready : %d(%d)\n", group->flash.flashReady, ldr_frame->fcount);
	}

	if (ldr_frame->shot->dm.flash.flashOffReady!= group->flash.flashOffReady) {
		group->flash.flashOffReady = ldr_frame->shot->dm.flash.flashOffReady;
		info("flash off : %d(%d)\n", group->flash.flashOffReady, ldr_frame->fcount);
	}
#endif
}
#endif

static void fimc_is_group_set_torch(struct fimc_is_group *group,
	struct fimc_is_frame *ldr_frame)
{
	if (group->prev)
		return;

	if (group->aeflashMode != ldr_frame->shot->ctl.aa.vendor_aeflashMode) {
		group->aeflashMode = ldr_frame->shot->ctl.aa.vendor_aeflashMode;
		fimc_is_vender_set_torch(group->aeflashMode);
	}

	return;
}

static void fimc_is_group_start_trigger(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group,
	struct fimc_is_frame *frame)
{
	struct fimc_is_group_task *gtask;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(!frame);
	BUG_ON(group->id >= GROUP_ID_MAX);

	atomic_inc(&group->rcount);

	gtask = &groupmgr->gtask[group->id];
#ifdef ENABLE_SYNC_REPROCESSING
	if ((atomic_read(&gtask->refcount) > 1)
		&& (group->device->resourcemgr->hal_version == IS_HAL_VER_1_0)
		&& test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &group->device->state)) {
		if (atomic_read(&gtask->rep_tick) != 0) {
			atomic_set(&gtask->rep_tick, REPROCESSING_TICK_COUNT);
			return;
		} else {
			/* for distinguish first async shot */
			atomic_set(&gtask->rep_tick, REPROCESSING_TICK_COUNT - 1);
		}
	}
#endif

	queue_kthread_work(&gtask->worker, &frame->work);
}

static int fimc_is_group_task_probe(struct fimc_is_group_task *gtask,
	u32 id)
{
	int ret = 0;

	BUG_ON(!gtask);
	BUG_ON(id >= GROUP_ID_MAX);

	gtask->id = id;
	atomic_set(&gtask->refcount, 0);
	clear_bit(FIMC_IS_GTASK_START, &gtask->state);
	clear_bit(FIMC_IS_GTASK_REQUEST_STOP, &gtask->state);

	return ret;
}

static int fimc_is_group_task_start(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group_task *gtask, struct fimc_is_framemgr *framemgr)
{
	int ret = 0;
	char name[30];
	struct sched_param param = { .sched_priority = FIMC_IS_MAX_PRIO - 1 };

	BUG_ON(!groupmgr);
	BUG_ON(!gtask);
	BUG_ON(!framemgr);

	if (test_bit(FIMC_IS_GTASK_START, &gtask->state))
		goto p_work;

	init_kthread_worker(&gtask->worker);
	snprintf(name, sizeof(name), "fimc_is_gw%d", gtask->id);
	gtask->task = kthread_run(kthread_worker_fn, &gtask->worker, name);
	if (IS_ERR(gtask->task)) {
		err("failed to create group_task%d, err(%ld)\n",
			gtask->id, PTR_ERR(gtask->task));
		ret = PTR_ERR(gtask->task);
		goto p_err;
	}

	ret = sched_setscheduler_nocheck(gtask->task, SCHED_FIFO, &param);
	if (ret) {
		err("sched_setscheduler_nocheck is fail(%d)", ret);
		goto p_err;
	}

#ifndef ENABLE_IS_CORE
#ifdef ENABLE_FPSIMD_FOR_USER
	fpsimd_set_task_using(gtask->task);
#endif
#ifdef SET_CPU_AFFINITY
	ret = set_cpus_allowed_ptr(gtask->task, cpumask_of(2));
#endif
#endif

	sema_init(&gtask->smp_resource, MIN_OF_SHOT_RSC);
	set_bit(FIMC_IS_GTASK_START, &gtask->state);

p_work:
	atomic_inc(&gtask->refcount);

p_err:
	return ret;
}

static int fimc_is_group_task_stop(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group_task *gtask, u32 slot)
{
	int ret = 0;
	u32 stream, refcount;
	struct fimc_is_group *group;

	BUG_ON(!groupmgr);
	BUG_ON(!gtask);
	BUG_ON(slot >= GROUP_SLOT_MAX);

	if (!test_bit(FIMC_IS_GTASK_START, &gtask->state)) {
		err("gtask(%d) is not started", gtask->id);
		ret = -EINVAL;
		goto p_err;
	}

	if (IS_ERR_OR_NULL(gtask->task)) {
		err("task of gtask(%d) is invalid(%p)", gtask->id, gtask->task);
		ret = -EINVAL;
		goto p_err;
	}

	refcount = atomic_dec_return(&gtask->refcount);
	if (refcount > 0)
		goto p_err;

	set_bit(FIMC_IS_GTASK_REQUEST_STOP, &gtask->state);

	for (stream = 0; stream < FIMC_IS_STREAM_COUNT; ++stream) {
		group = groupmgr->group[stream][slot];
		if (group && (group->id == gtask->id) &&
			test_bit(FIMC_IS_GROUP_SHOT, &group->state)) {
			smp_shot_inc(group);
			up(&gtask->smp_resource);
			up(&group->smp_trigger);
		}
	}

	/*
	 * flush kthread wait until all work is complete
	 * it's dangerous if all is not finished
	 * so it's commented currently
	 * flush_kthread_worker(&groupmgr->group_worker[slot]);
	 */
	kthread_stop(gtask->task);

	clear_bit(FIMC_IS_GTASK_REQUEST_STOP, &gtask->state);
	clear_bit(FIMC_IS_GTASK_START, &gtask->state);

p_err:
	return ret;
}

int fimc_is_groupmgr_probe(struct fimc_is_groupmgr *groupmgr)
{
	int ret = 0;
	u32 stream, slot, id, gframe;
	struct fimc_is_group_framemgr *gframemgr;

	for (stream = 0; stream < FIMC_IS_STREAM_COUNT; ++stream) {
		gframemgr = &groupmgr->gframemgr[stream];
		spin_lock_init(&groupmgr->gframemgr[stream].gframe_slock);
		INIT_LIST_HEAD(&groupmgr->gframemgr[stream].gframe_head);
		groupmgr->gframemgr[stream].gframe_cnt = 0;

		for (gframe = 0; gframe < FIMC_IS_MAX_GFRAME; ++gframe) {
			groupmgr->gframemgr[stream].gframe[gframe].fcount = 0;
			fimc_is_gframe_s_free(&groupmgr->gframemgr[stream],
				&groupmgr->gframemgr[stream].gframe[gframe]);
		}

		groupmgr->leader[stream] = NULL;
		for (slot = 0; slot < GROUP_SLOT_MAX; ++slot)
			groupmgr->group[stream][slot] = NULL;
	}

	for (id = 0; id < GROUP_ID_MAX; ++id) {
		ret = fimc_is_group_task_probe(&groupmgr->gtask[id], id);
		if (ret) {
			err("fimc_is_group_task_probe is fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	return ret;
}

int fimc_is_groupmgr_init(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_device_ischain *device)
{
	int ret = 0;
	struct fimc_is_path_info *path;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_group *group, *prev, *next, *sibling;
	struct fimc_is_group *leader_group;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_video *video;
	u32 slot, source_vid;
	u32 instance;
	u32 entry;

	BUG_ON(!groupmgr);
	BUG_ON(!device);

	group = NULL;
	prev = NULL;
	next = NULL;
	instance = device->instance;
	path = &device->path;

	for (slot = 0; slot < GROUP_SLOT_MAX; ++slot) {
		path->group[slot] = GROUP_ID_INVALID;
	}

	leader_group = groupmgr->leader[instance];
	if (!leader_group) {
		err("stream leader is not selected");
		ret = -EINVAL;
		goto p_err;
	}

	for (slot = 0; slot < GROUP_SLOT_MAX; ++slot) {
		group = groupmgr->group[instance][slot];
		if (!group)
			continue;

		group->prev = NULL;
		group->next = NULL;
		group->gprev = NULL;
		group->gnext = NULL;
		group->parent = NULL;
		group->child = NULL;
		group->head = group;
		group->tail = group;
		group->junction = NULL;
		group->pipe_shot_callback = NULL;

		/* A group should be initialized, only if the group was placed at the front of leader group */
		if (slot < leader_group->slot)
			continue;

		source_vid = group->source_vid;
		mdbgd_group("source vid : %02d\n", group, source_vid);
		if (source_vid) {
			leader = &group->leader;
			fimc_is_group_s_leader(group, leader);

			if (prev) {
				group->prev = prev;
				prev->next = group;
			}

			prev = group;
		}
	}

	fimc_is_dmsg_init();

	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
		fimc_is_dmsg_concate("STM(R) PH:");
	else
		fimc_is_dmsg_concate("STM(N) PH:");

	/*
	 * The Meaning of Symbols.
	 *  1) -> : HAL(OTF), Driver(OTF)
	 *  2) => : HAL(M2M), Driver(M2M)
	 *  3) ~> : HAL(OTF), Driver(M2M)
	 *  4) >> : HAL(OTF), Driver(M2M)
	 *          It's Same with 3).
	 *          But HAL q/dq junction node between the groups.
	 */
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &leader_group->state))
		fimc_is_dmsg_concate(" %02d -> ", leader_group->source_vid);
	else if (test_bit(FIMC_IS_GROUP_PIPE_INPUT, &leader_group->state))
		fimc_is_dmsg_concate(" %02d ~> ", leader_group->source_vid);
	else if (test_bit(FIMC_IS_GROUP_SEMI_PIPE_INPUT, &leader_group->state))
		fimc_is_dmsg_concate(" %02d >> ", leader_group->source_vid);
	else
		fimc_is_dmsg_concate(" %02d => ", leader_group->source_vid);

	group = leader_group;
	while(group) {
		next = group->next;
		if (next) {
			source_vid = next->source_vid;
			BUG_ON(group->slot >= GROUP_SLOT_MAX);
			BUG_ON(next->slot >= GROUP_SLOT_MAX);
		} else {
			if (group->id == GROUP_ID_MCS0 || group->id == GROUP_ID_MCS1)
				source_vid = device->vid_to_vra;
			else
			source_vid = 0;
			path->group[group->slot] = group->id;
		}

		fimc_is_dmsg_concate("GP%d ( ", group->id);
		for (entry = 0; entry < ENTRY_END; ++entry) {
			subdev = group->subdev[entry];
			if (!subdev)
				continue;

			vctx = subdev->vctx;
			if (!vctx)
				continue;

			video = vctx->video;
			if (!video) {
				merr("video is NULL", device);
				BUG();
			}

			/* groupping check */
			switch (group->id) {
#ifdef CONFIG_USE_SENSOR_GROUP
			case GROUP_ID_SS0:
			case GROUP_ID_SS1:
			case GROUP_ID_SS2:
			case GROUP_ID_SS3:
			case GROUP_ID_SS4:
			case GROUP_ID_SS5:
				if ( !(((video->id >= FIMC_IS_VIDEO_SS0_NUM) &&		/* SS0 <= video_id <= BNS */
					(video->id <= FIMC_IS_VIDEO_BNS_NUM)) ||	/* or */
					((video->id >= FIMC_IS_VIDEO_SS0VC0_NUM) &&	/* SS0VC0 <= video_id <= SS5VC3 */
					(video->id <= FIMC_IS_VIDEO_SS5VC3_NUM)))) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}
				break;
#endif
			case GROUP_ID_3AA0:
				if ((video->id >= FIMC_IS_VIDEO_31S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_31P_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}
				break;
			case GROUP_ID_3AA1:
				if ((video->id >= FIMC_IS_VIDEO_30S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_30P_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}
				break;
			case GROUP_ID_ISP0:
				if ((video->id >= FIMC_IS_VIDEO_I1S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_I1P_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}

				if ((video->id >= FIMC_IS_VIDEO_30S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_31P_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}
				break;
			case GROUP_ID_ISP1:
				if ((video->id >= FIMC_IS_VIDEO_I0S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_I0P_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}

				if ((video->id >= FIMC_IS_VIDEO_30S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_31P_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}
				break;
			case GROUP_ID_DIS0:
			case GROUP_ID_DIS1:
				if ((video->id >= FIMC_IS_VIDEO_30S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_I1P_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}
				break;
			case GROUP_ID_MCS0:
			case GROUP_ID_MCS1:
				if ((video->id >= FIMC_IS_VIDEO_30S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_SCP_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}

				if ((video->id >= FIMC_IS_VIDEO_D0S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_D1C_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}
				break;
			case GROUP_ID_VRA0:
				if ((video->id >= FIMC_IS_VIDEO_30S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_SCP_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}

				if ((video->id >= FIMC_IS_VIDEO_D0S_NUM) &&
					(video->id <= FIMC_IS_VIDEO_D1C_NUM)) {
					merr("invalid video group(G%d -> V%02d)", device, group->id, video->id);
					BUG();
				}
				break;
			default:
				merr("invalid group(%d)", device, group->id);
				BUG();
				break;
			}

			/* default connection */
			if ((!group->junction) && (subdev != &group->leader))
				group->junction = group->subdev[entry];

			/* connection check */
			if (video->id == source_vid) {
				fimc_is_dmsg_concate("*%02d ", video->id);
				group->junction = group->subdev[entry];
				path->group[group->slot] = group->id;
				if (next)
					path->group[next->slot] = next->id;
			} else {
				fimc_is_dmsg_concate("%02d ", video->id);
			}
		}
		fimc_is_dmsg_concate(")");

		if (next && !group->junction) {
			mgerr("junction subdev can NOT be found", device, group);
			ret = -EINVAL;
			goto p_err;
		}

		if (next) {
			if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &next->state)) {
				set_bit(FIMC_IS_GROUP_OTF_OUTPUT, &group->state);
				fimc_is_dmsg_concate(" -> ");
			} else if (test_bit(FIMC_IS_GROUP_PIPE_INPUT, &next->state)) {
				set_bit(FIMC_IS_GROUP_PIPE_OUTPUT, &group->state);
				fimc_is_dmsg_concate(" ~> ");
			} else if (test_bit(FIMC_IS_GROUP_SEMI_PIPE_INPUT, &next->state)) {
				set_bit(FIMC_IS_GROUP_SEMI_PIPE_OUTPUT, &group->state);
				fimc_is_dmsg_concate(" >> ");
			} else {
				fimc_is_dmsg_concate(" => ");
			}
		}

		group = next;
	}
	fimc_is_dmsg_concate("\n");

	group = leader_group;
	sibling = leader_group;
	next = group->next;
	while (next) {
		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &next->state)) {
			group->child = next;
			next->parent = next->prev;
			sibling->tail = next;
			next->head = sibling;
			leader = &sibling->leader;
			fimc_is_group_s_leader(next, leader);
		} else if (test_bit(FIMC_IS_GROUP_PIPE_INPUT, &next->state)) {
			sibling->gnext = next;
			next->gprev = sibling;
			sibling = next;

			fimc_is_pipe_create(&device->pipe, next->gprev, next);
		} else if (test_bit(FIMC_IS_GROUP_SEMI_PIPE_INPUT, &next->state)) {
			sibling->gnext = next;
			next->gprev = sibling;
			sibling = next;

			fimc_is_pipe_create(&device->pipe, next->gprev, next);
		} else {
			sibling->gnext = next;
			next->gprev = sibling;
			sibling = next;
		}

		group = next;
		next = group->next;
	}

	/* each group tail setting */
	group = leader_group;
	sibling = leader_group;
	next = group->next;
	while (next) {
		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &next->state))
			next->tail = sibling->tail;
		else
			sibling = next;

		group = next;
		next = group->next;
	}

p_err:
	minfo(" =STM CFG===============\n", device);
	minfo(" %s", device, fimc_is_dmsg_print());
	minfo(" DEVICE GRAPH :", device);
	for (slot = 0; slot < GROUP_SLOT_MAX; ++slot)
		printk(KERN_CONT " %X", path->group[slot]);
	printk(KERN_CONT "\n");
	minfo(" =======================\n", device);
	return ret;
}

int fimc_is_groupmgr_start(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_device_ischain *device)
{
	int ret = 0;
	u32 subdev_id;
	u32 instance;
	u32 width, height;
	u32 lindex, hindex, indexes;
	struct fimc_is_group *group, *prev;
	struct fimc_is_subdev *leader, *subdev;
	struct fimc_is_crop incrop, otcrop;

	BUG_ON(!groupmgr);
	BUG_ON(!device);

	width = 0;
	height = 0;
	instance = device->instance;
	group = groupmgr->leader[instance];
	if (!group) {
		merr("stream leader is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	minfo(" =GRP CFG===============\n", device);
	while(group) {
		leader = &group->leader;
		prev = group->prev;

		if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state) &&
			!test_bit(FIMC_IS_GROUP_START, &group->state)) {
			merr("GP%d is NOT started", device, group->id);
			ret = -EINVAL;
			goto p_err;
		}

		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
#ifdef CONFIG_USE_SENSOR_GROUP
			if (group->slot == GROUP_SLOT_SENSOR) {
				width = fimc_is_sensor_g_width(device->sensor);
				height = fimc_is_sensor_g_height(device->sensor);
				leader->input.width = width;
				leader->input.height = height;
			} else if (group->slot == GROUP_SLOT_3AA)
#else
			if (group->slot == GROUP_SLOT_3AA)
#endif
			{
				width = fimc_is_sensor_g_bns_width(device->sensor);
				height = fimc_is_sensor_g_bns_height(device->sensor);
				leader->input.width = width;
				leader->input.height = height;
			} else {
				if (prev && prev->junction) {
					/* FIXME, Max size constrains */
					if (width > leader->constraints_width) {
						mgwarn(" width(%d) > constraints_width(%d), set constraints width",
							device, group, width, leader->constraints_width);
						width = leader->constraints_width;
					}

					if (height > leader->constraints_height) {
						mgwarn(" height(%d) > constraints_height(%d), set constraints height",
							device, group, height, leader->constraints_height);
						height = leader->constraints_height;
					}

					leader->input.width = width;
					leader->input.height = height;
					prev->junction->output.width = width;
					prev->junction->output.height = height;
				} else {
					mgerr("previous group is NULL", group, group);
					BUG();
				}
			}
		} else {
			if (group->slot == GROUP_SLOT_3AA) {
				width = leader->input.width;
				height = leader->input.height;
			} else {
				width = leader->input.width;
				height = leader->input.height;
				leader->input.canv.x = 0;
				leader->input.canv.y = 0;
				leader->input.canv.w = leader->input.width;
				leader->input.canv.h = leader->input.height;
			}
		}

		mginfo(" SRC%02d:%04dx%04d\n", device, group, leader->vid,
			leader->input.width, leader->input.height);
		for (subdev_id = 0; subdev_id < ENTRY_END; ++subdev_id) {
			subdev = group->subdev[subdev_id];
			if (subdev && subdev->vctx && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				mginfo(" CAP%2d:%04dx%04d\n", device, group, subdev->vid,
					subdev->output.width, subdev->output.height);
			}
		}

		if (prev && !test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
			if (!prev->junction) {
				mgerr("prev group is existed but junction is NULL", device, group);
				ret = -EINVAL;
				goto p_err;
			}

			if ((prev->junction->output.width != group->leader.input.width) ||
				(prev->junction->output.height != group->leader.input.height)) {
				merr("%s(%d x %d) != %s(%d x %d)", device,
					prev->junction->name,
					prev->junction->output.width,
					prev->junction->output.height,
					group->leader.name,
					group->leader.input.width,
					group->leader.input.height);
				ret = -EINVAL;
				goto p_err;
			}
		}

		incrop.x = 0;
		incrop.y = 0;
		incrop.w = width;
		incrop.h = height;

		otcrop.x = 0;
		otcrop.y = 0;
		otcrop.w = width;
		otcrop.h = height;

		/* subdev cfg callback for initialization */
		ret = CALL_SOPS(&group->leader, cfg, device, NULL, &incrop, &otcrop, &lindex, &hindex, &indexes);
		if (ret) {
			mgerr("tag callback is fail(%d)", group, group, ret);
			goto p_err;
		}

		group = group->next;
	}
	minfo(" =======================\n", device);

p_err:
	return ret;
}

int fimc_is_groupmgr_stop(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_device_ischain *device)
{
	int ret = 0;
	u32 instance;
	struct fimc_is_group *group;

	BUG_ON(!groupmgr);
	BUG_ON(!device);

	instance = device->instance;
	group = groupmgr->leader[instance];
	if (!group) {
		merr("stream leader is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	if (test_bit(FIMC_IS_GROUP_START, &group->state)) {
		merr("stream leader is NOT stopped", device);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_group_probe(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group,
	struct fimc_is_device_sensor *sensor,
	struct fimc_is_device_ischain *device,
	fimc_is_shot_callback shot_callback,
	u32 slot,
	u32 id,
	char *name,
	const struct fimc_is_subdev_ops *sops)
{
	int ret = 0;
	struct fimc_is_subdev *leader;

	BUG_ON(!groupmgr);
	BUG_ON(!group);

	leader = &group->leader;
	group->id = GROUP_ID_INVALID;
	group->slot = slot;
	group->shot_callback = shot_callback;
	group->device = device;
	group->sensor = sensor;
	if (group->device)
		group->instance = device->instance;
	else
		group->instance = FIMC_IS_STREAM_COUNT;

	ret = fimc_is_hw_group_cfg(group);
	if (ret) {
		merr("fimc_is_hw_group_cfg is fail(%d)", group, ret);
		goto p_err;
	}

	clear_bit(FIMC_IS_GROUP_OPEN, &group->state);
	clear_bit(FIMC_IS_GROUP_INIT, &group->state);
	clear_bit(FIMC_IS_GROUP_START, &group->state);
	clear_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
	clear_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state);
	clear_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_OTF_OUTPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_PIPE_INPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_SEMI_PIPE_INPUT, &group->state);

	if (group->device) {
		group->device_type = FIMC_IS_DEVICE_ISCHAIN;
		fimc_is_subdev_probe(leader, device->instance, id, name, sops);
	} else if (group->sensor) {
		group->device_type = FIMC_IS_DEVICE_SENSOR;
		fimc_is_subdev_probe(leader, sensor->instance, id, name, sops);
	} else {
		err("device and sensor are NULL(%d)", ret);
	}

p_err:
	return ret;
}

int fimc_is_group_open(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group, u32 id,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	u32 stream, slot;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_group_task *gtask;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(!vctx);
	BUG_ON(id >= GROUP_ID_MAX);

	group->id = id;
	stream = group->instance;
	slot = group->slot;
	gtask = &groupmgr->gtask[id];

	mdbgd_group("%s(%d) S\n", group, __func__, id);

	if (test_bit(FIMC_IS_GROUP_OPEN, &group->state)) {
		mgerr("already open", group, group);
		ret = -EMFILE;
		goto p_err;
	}

	framemgr = GET_FRAMEMGR(vctx);
	if (!framemgr) {
		mgerr("framemgr is NULL", group, group);
		ret = -EINVAL;
		goto p_err;
	}

	/* 1. Init Group */
	clear_bit(FIMC_IS_GROUP_INIT, &group->state);
	clear_bit(FIMC_IS_GROUP_START, &group->state);
	clear_bit(FIMC_IS_GROUP_SHOT, &group->state);
	clear_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
	clear_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state);
	clear_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_OTF_OUTPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_PIPE_INPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_SEMI_PIPE_INPUT, &group->state);

	group->prev = NULL;
	group->next = NULL;
	group->gprev = NULL;
	group->gnext = NULL;
	group->parent = NULL;
	group->child = NULL;
	group->head = NULL;
	group->tail = NULL;
	group->junction = NULL;
	group->source_vid = 0;
	group->fcount = 0;
	group->pcount = 0;
	group->aeflashMode = 0; /* Flash Mode Control */
	atomic_set(&group->scount, 0);
	atomic_set(&group->rcount, 0);
	atomic_set(&group->backup_fcount, 0);
	atomic_set(&group->sensor_fcount, 1);
	sema_init(&group->smp_trigger, 0);

	INIT_LIST_HEAD(&group->gframe_head);
	group->gframe_cnt = 0;

#ifdef MEASURE_TIME
#ifdef MONITOR_TIME
	monitor_init(&group->time);
#endif
#endif

	/* 2. start kthread */
	ret = fimc_is_group_task_start(groupmgr, gtask, framemgr);
	if (ret) {
		mgerr("fimc_is_group_task_start is fail(%d)", group, group, ret);
		goto p_err;
	}

	/* 3. Subdev Init */
	ret = fimc_is_subdev_open(&group->leader, vctx, NULL);
	if (ret) {
		mgerr("fimc_is_subdev_open is fail(%d)", group, group, ret);
		goto p_err;
	}

	/* 4. group hw Init */
	ret = fimc_is_hw_group_open(group);
	if (ret) {
		mgerr("fimc_is_hw_group_open is fail(%d)", group, group, ret);
		goto p_err;
	}

	/* 5. Update Group Manager */
	groupmgr->group[stream][slot] = group;
	set_bit(FIMC_IS_GROUP_OPEN, &group->state);

p_err:
	mdbgd_group("%s(%d) E\n", group, __func__, ret);
	return ret;
}

int fimc_is_group_close(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group)
{
	int ret = 0;
	bool all_slot_empty = true;
	u32 stream, slot, i;
	struct fimc_is_group_task *gtask;
	struct fimc_is_group_framemgr *gframemgr;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);
	BUG_ON(group->id >= GROUP_ID_MAX);

	slot = group->slot;
	stream = group->instance;
	gtask = &groupmgr->gtask[group->id];

	if (!test_bit(FIMC_IS_GROUP_OPEN, &group->state)) {
		mgerr("already close", group, group);
		return -EMFILE;
	}

	ret = fimc_is_group_task_stop(groupmgr, gtask, slot);
	if (ret)
		mgerr("fimc_is_group_task_stop is fail(%d)", group, group, ret);

	ret = fimc_is_subdev_close(&group->leader);
	if (ret)
		mgerr("fimc_is_subdev_close is fail(%d)", group, group, ret);

	group->prev = NULL;
	group->next = NULL;
	group->gprev = NULL;
	group->gnext = NULL;
	group->parent = NULL;
	group->child = NULL;
	group->head = NULL;
	group->tail = NULL;
	group->junction = NULL;
	clear_bit(FIMC_IS_GROUP_INIT, &group->state);
	clear_bit(FIMC_IS_GROUP_OPEN, &group->state);
	groupmgr->group[stream][slot] = NULL;

	for (i = 0; i < GROUP_SLOT_MAX; i++) {
		if (groupmgr->group[stream][i]) {
			all_slot_empty = false;
			break;
		}
	}

	if (all_slot_empty) {
		gframemgr = &groupmgr->gframemgr[stream];

		if (gframemgr->gframe_cnt != FIMC_IS_MAX_GFRAME) {
			mwarn("gframemgr free count is invalid(%d)", group, gframemgr->gframe_cnt);
			INIT_LIST_HEAD(&gframemgr->gframe_head);
			gframemgr->gframe_cnt = 0;
			for (i = 0; i < FIMC_IS_MAX_GFRAME; ++i) {
				gframemgr->gframe[i].fcount = 0;
				fimc_is_gframe_s_free(gframemgr, &gframemgr->gframe[i]);
			}
		}
	}

	mdbgd_group("%s(ref %d, %d)", group, __func__, atomic_read(&gtask->refcount), ret);

	/* reset after using it */
	group->id = GROUP_ID_INVALID;

	return ret;
}

int fimc_is_group_init(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group,
	u32 input_type,
	u32 video_id,
	u32 stream_leader)
{
	int ret = 0;
	u32 slot;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);
	BUG_ON(group->id >= GROUP_ID_MAX);
	BUG_ON(video_id >= FIMC_IS_VIDEO_MAX_NUM);

	if (!test_bit(FIMC_IS_GROUP_OPEN, &group->state)) {
		mgerr("group is NOT open", group, group);
		ret = -EINVAL;
		goto p_err;
	}

	slot = group->slot;
	group->source_vid = video_id;
	clear_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_OTF_OUTPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_PIPE_INPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_PIPE_OUTPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_SEMI_PIPE_INPUT, &group->state);
	clear_bit(FIMC_IS_GROUP_SEMI_PIPE_OUTPUT, &group->state);

	if (stream_leader)
		groupmgr->leader[group->instance] = group;

	switch (input_type) {
	case GROUP_INPUT_MEMORY:
		smp_shot_init(group, 1);
		group->asyn_shots = 0;
		group->skip_shots = 0;
		group->init_shots = 0;
		group->sync_shots = 1;
		break;
	case GROUP_INPUT_OTF:
		set_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state);
		smp_shot_init(group, MIN_OF_SHOT_RSC);
		group->asyn_shots = MIN_OF_ASYNC_SHOTS;
		group->skip_shots = MIN_OF_ASYNC_SHOTS;
		group->init_shots = MIN_OF_ASYNC_SHOTS;
		group->sync_shots = MIN_OF_SYNC_SHOTS;
		break;
	case GROUP_INPUT_PIPE:
		set_bit(FIMC_IS_GROUP_PIPE_INPUT, &group->state);
		smp_shot_init(group, 1);
		group->asyn_shots = 0;
		group->skip_shots = 0;
		group->init_shots = 0;
		group->sync_shots = 1;
		break;
	case GROUP_INPUT_SEMI_PIPE:
		set_bit(FIMC_IS_GROUP_SEMI_PIPE_INPUT, &group->state);
		smp_shot_init(group, 1);
		group->asyn_shots = 0;
		group->skip_shots = 0;
		group->init_shots = 0;
		group->sync_shots = 1;
		break;
	default:
		mgerr("input type is invalid(%d)", group, group, input_type);
		ret = -EINVAL;
		goto p_err;
		break;
	}

	set_bit(FIMC_IS_GROUP_INIT, &group->state);

p_err:
	mdbgd_group("%s(input : %d):%d\n", group, __func__, input_type, ret);
	return ret;
}

void set_group_shots(struct fimc_is_group *group, u32 hal_version, u32 framerate)
{
#ifdef ENABLE_IS_CORE
	if (hal_version == IS_HAL_VER_3_2) {
		if (framerate <= 30) {
			group->asyn_shots = 0;
			group->skip_shots = 1;
			group->init_shots = 1;
			group->sync_shots = 3;
		} else {
			group->asyn_shots = 0;
			group->skip_shots = 3;
			group->init_shots = 3;
			group->sync_shots = 3;
		}
	} else {
		if (framerate <= 30) {
#ifdef REDUCE_COMMAND_DELAY
			group->asyn_shots = MIN_OF_ASYNC_SHOTS + 1;
			group->sync_shots = 0;
#else
			group->asyn_shots = MIN_OF_ASYNC_SHOTS + 0;
			group->sync_shots = MIN_OF_SYNC_SHOTS;
#endif
		} else if (framerate <= 60) {
			group->asyn_shots = MIN_OF_ASYNC_SHOTS + 1;
			group->sync_shots = MIN_OF_SYNC_SHOTS;
		} else if (framerate <= 120) {
			group->asyn_shots = MIN_OF_ASYNC_SHOTS + 2;
			group->sync_shots = MIN_OF_SYNC_SHOTS;
		} else if (framerate <= 240) {
			group->asyn_shots = MIN_OF_ASYNC_SHOTS + 2;
			group->sync_shots = MIN_OF_SYNC_SHOTS;
		} else { /* 300fps */
			group->asyn_shots = MIN_OF_ASYNC_SHOTS + 3;
			group->sync_shots = MIN_OF_SYNC_SHOTS;
		}
		group->init_shots = group->asyn_shots;
		group->skip_shots = group->asyn_shots;
	}
#else
#ifdef REDUCE_COMMAND_DELAY
	group->asyn_shots = MIN_OF_ASYNC_SHOTS + 1;
	group->sync_shots = 0;
#else
	group->asyn_shots = MIN_OF_ASYNC_SHOTS;
	group->sync_shots = MIN_OF_SYNC_SHOTS;
#endif
	group->init_shots = group->asyn_shots;
	group->skip_shots = group->asyn_shots;
#endif
	return;
}

int fimc_is_group_start(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group)
{
	int ret = 0;
	struct fimc_is_device_ischain *device;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_framemgr *framemgr = NULL;
	struct fimc_is_group_task *gtask;
	u32 sensor_fcount;
	u32 framerate;
	u32 shot_inc;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(!group->device);
	BUG_ON(!group->device->sensor);
	BUG_ON(!group->device->resourcemgr);
	BUG_ON(!group->leader.vctx);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);
	BUG_ON(group->id >= GROUP_ID_MAX);

	if (!test_bit(FIMC_IS_GROUP_INIT, &group->state)) {
		merr("group is NOT initialized", group);
		ret = -EINVAL;
		goto p_err;
	}

	if (test_bit(FIMC_IS_GROUP_START, &group->state)) {
		warn("already group start");
		ret = -EINVAL;
		goto p_err;
	}

	device = group->device;
	gtask = &groupmgr->gtask[group->id];
	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		mgerr("framemgr is NULL", group, group);
		goto p_err;
	}

	atomic_set(&group->scount, 0);
	sema_init(&group->smp_trigger, 0);

	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
		group->asyn_shots = 1;
		group->skip_shots = 0;
		group->init_shots = 0;
		group->sync_shots = 0;
		smp_shot_init(group, group->asyn_shots + group->sync_shots);
	} else {
		sensor = device->sensor;
		framerate = fimc_is_sensor_g_framerate(sensor);

		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
			resourcemgr = device->resourcemgr;
			set_group_shots(group, resourcemgr->hal_version, framerate);

			/* frame count */
			sensor_fcount = fimc_is_sensor_g_fcount(sensor) + 1;
			atomic_set(&group->sensor_fcount, sensor_fcount);
			atomic_set(&group->backup_fcount, sensor_fcount - 1);
			group->fcount = sensor_fcount - 1;

			memset(&group->intent_ctl, 0, sizeof(struct camera2_aa_ctl));

			/* shot resource */
			if ((group->init_shots + group->sync_shots) > smp_shot_get(group)) {
				shot_inc = (group->init_shots + group->sync_shots) - smp_shot_get(group);
				while (shot_inc > 0) {
					up(&gtask->smp_resource);
					smp_shot_inc(group);
					shot_inc--;
				}
			}
		} else {
			if (framerate > 120)
				group->asyn_shots = MIN_OF_ASYNC_SHOTS_240FPS;
			else
				group->asyn_shots = MIN_OF_ASYNC_SHOTS;
			group->skip_shots = 0;
			group->init_shots = 0;
			group->sync_shots = 0;
			smp_shot_init(group, group->asyn_shots + group->sync_shots);
		}
	}

	set_bit(FIMC_IS_SUBDEV_START, &group->leader.state);
	set_bit(FIMC_IS_GROUP_START, &group->state);

p_err:
	if (framemgr != NULL) {
		mginfo("bufs: %02d, init : %d, asyn: %d, skip: %d, sync : %d\n", group, group,
			framemgr->num_frames, group->init_shots, group->asyn_shots, group->skip_shots, group->sync_shots);
	}
	return ret;
}

void wait_subdev_flush_work(struct fimc_is_device_ischain *device,
	struct fimc_is_group *group, u32 entry)
{
#ifdef ENABLE_IS_CORE
	return;
#else
	struct fimc_is_interface *itf;
	u32 wq_id = WORK_MAX_MAP;
	bool ret;

	BUG_ON(!group);
	BUG_ON(!device);

	itf = device->interface;
	BUG_ON(!itf);

	switch (entry) {
	case ENTRY_3AC:
		wq_id = (group->id == GROUP_ID_3AA0) ? WORK_30C_FDONE: WORK_31C_FDONE;
		break;
	case ENTRY_3AP:
		wq_id = (group->id == GROUP_ID_3AA0) ? WORK_30P_FDONE: WORK_31P_FDONE;
		break;
	case ENTRY_IXC:
		wq_id = (group->id == GROUP_ID_ISP0) ? WORK_I0C_FDONE: WORK_I1C_FDONE;
		break;
	case ENTRY_IXP:
		wq_id = (group->id == GROUP_ID_ISP0) ? WORK_I0P_FDONE: WORK_I1P_FDONE;
		break;
	case ENTRY_SCC:
		wq_id = WORK_SCC_FDONE;
		break;
	case ENTRY_SCP:
		wq_id = WORK_SCP_FDONE;
		break;
	case ENTRY_M0P:
		wq_id = WORK_M0P_FDONE;
		break;
	case ENTRY_M1P:
		wq_id = WORK_M1P_FDONE;
		break;
	case ENTRY_M2P:
		wq_id = WORK_M2P_FDONE;
		break;
	case ENTRY_M3P:
		wq_id = WORK_M3P_FDONE;
		break;
	case ENTRY_M4P:
		wq_id = WORK_M4P_FDONE;
		break;
	case ENTRY_SENSOR:	/* Falls Through */
	case ENTRY_3AA:		/* Falls Through */
	case ENTRY_ISP:		/* Falls Through */
	case ENTRY_DIS:		/* Falls Through */
	case ENTRY_MCS:		/* Falls Through */
	case ENTRY_VRA:
		mginfo("flush SHOT_DONE wq, subdev[%d]", device, group, entry);
		wq_id = WORK_SHOT_DONE;
		break;
	default:
		mgerr(" invalid subdev[%d]", device, group, entry);
		return;
		break;
	}

	ret = flush_work(&itf->work_wq[wq_id]);
	if (ret)
		mginfo(" flush_work executed! wq_id(%d)\n", device, group, wq_id);
#endif
}

int fimc_is_group_stop(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group)
{
	int ret = 0;
	int errcnt = 0;
	int retry;
	u32 rcount, entry;
	unsigned long flags;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_device_ischain *device;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_group *child;
	struct fimc_is_subdev *subdev;
	struct fimc_is_group_task *gtask;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(!group->device);
	BUG_ON(!group->leader.vctx);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);
	BUG_ON(group->id >= GROUP_ID_MAX);

	device = group->device;
	sensor = device->sensor;
	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	gtask = &groupmgr->gtask[group->id];
	if (!framemgr) {
		mgerr("framemgr is NULL", group, group);
		goto p_err;
	}

	if (!test_bit(FIMC_IS_GROUP_START, &group->state) &&
		!test_bit(FIMC_IS_GROUP_START, &group->head->state)) {
		mwarn("already group stop", group);
		goto p_err;
	}

	if (test_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state)) {
		set_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state);
		clear_bit(FIMC_IS_GROUP_REQUEST_FSTOP, &group->state);
	}

	retry = 150;
	while (--retry && framemgr->queued_count[FS_REQUEST]) {
		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state) &&
			!list_empty(&group->smp_trigger.wait_list)) {

			if (!sensor) {
				mwarn(" sensor is NULL, forcely trigger(pc %d)", device, group->pcount);
				set_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state);
				up(&group->smp_trigger);
			} else if (!test_bit(FIMC_IS_SENSOR_OPEN, &sensor->state)) {
				mwarn(" sensor is closed, forcely trigger(pc %d)", device, group->pcount);
				set_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state);
				up(&group->smp_trigger);
			} else if (!test_bit(FIMC_IS_SENSOR_FRONT_START, &sensor->state)) {
				mwarn(" front is stopped, forcely trigger(pc %d)", device, group->pcount);
				set_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state);
				up(&group->smp_trigger);
			} else if (!test_bit(FIMC_IS_SENSOR_BACK_START, &sensor->state)) {
				mwarn(" back is stopped, forcely trigger(pc %d)", device, group->pcount);
				set_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state);
				up(&group->smp_trigger);
			} else if (retry < 100) {
				merr(" sensor is working but no trigger(pc %d)", device, group->pcount);
				set_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state);
				up(&group->smp_trigger);
			} else {
				mwarn(" wating for sensor trigger(pc %d)", device, group->pcount);
			}
		}

		mgwarn(" %d reqs waiting...(pc %d) smp_shot(%d) smp_resource(%d)", device, group,
				framemgr->queued_count[FS_REQUEST], group->pcount,
				list_empty(&group->smp_shot.wait_list), list_empty(&gtask->smp_resource.wait_list));
		msleep(20);
	}

	if (!retry) {
		mgerr(" waiting(until request empty) is fail(pc %d)", device, group, group->pcount);
		errcnt++;
	}

	/* ensure that request cancel work is complete fully */
	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_21, flags);
	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_21, flags);

	retry = 150;
	while (--retry && test_bit(FIMC_IS_GROUP_SHOT, &group->state)) {
		mgwarn(" thread stop waiting...(pc %d)", device, group, group->pcount);
		msleep(20);
	}

	if (!retry) {
		mgerr(" waiting(until thread stop) is fail(pc %d)", device, group, group->pcount);
		errcnt++;
	}

	child = group;
	while (child) {
		if (test_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state)) {
			ret = fimc_is_itf_force_stop(device, GROUP_ID(child->id));
			if (ret) {
				mgerr(" fimc_is_itf_force_stop is fail(%d)", device, child, ret);
				errcnt++;
			}
		} else {
			ret = fimc_is_itf_process_stop(device, GROUP_ID(child->id));
			if (ret) {
				mgerr(" fimc_is_itf_process_stop is fail(%d)", device, child, ret);
				errcnt++;
			}
		}
		child = child->child;
	}

	retry = 150;
	while (--retry && framemgr->queued_count[FS_PROCESS]) {
		mgwarn(" %d pros waiting...(pc %d)", device, group, framemgr->queued_count[FS_PROCESS], group->pcount);
		msleep(20);
	}

	if (!retry) {
		mgerr(" waiting(until process empty) is fail(pc %d)", device, group, group->pcount);
		errcnt++;
	}

	rcount = atomic_read(&group->rcount);
	if (rcount) {
		mgerr(" request is NOT empty(%d) (pc %d)", device, group, rcount, group->pcount);
		errcnt++;
	}
	/* the count of request should be clear for next streaming */
	atomic_set(&group->rcount, 0);

	child = group;
	while(child) {
		for (entry = 0; entry < ENTRY_END; ++entry) {
			subdev = child->subdev[entry];
			if (subdev && subdev->vctx && test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
				framemgr = GET_SUBDEV_FRAMEMGR(subdev);
				if (!framemgr) {
					mgerr("framemgr is NULL", group, group);
					goto p_err;
				}

				retry = 150;
				while (--retry && framemgr->queued_count[FS_PROCESS]) {
					mgwarn(" subdev[%d] stop waiting...", device, group, subdev->vid);
					msleep(20);
				}

				if (!retry) {
					wait_subdev_flush_work(device, child, entry);
					mgerr(" waiting(subdev stop) is fail", device, group);
					errcnt++;
				}

				clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
			}
				/*
				 *
				 * For subdev only to be control by driver (no video node)
				 * In "process-stop" state of a group which have these subdevs,
				 * subdev's state can be invalid like tdnr, odc or drc etc.
				 * The wrong state problem can be happened in just stream-on/off ->
				 * stream-on case.
				 * ex.  previous stream on : state = FIMC_IS_SUBDEV_RUN && bypass = 0
				 *      next stream on     : state = FIMC_IS_SUBDEV_RUN && bypass = 0
				 * In this case, the subdev's function can't be worked.
				 * Because driver skips the function if it's state is FIMC_IS_SUBDEV_RUN.
				 */
			if (subdev)
				clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
			}
		child = child->child;
	}

	fimc_is_gframe_flush_all(groupmgr, group);

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state))
		mginfo(" sensor fcount: %d, fcount: %d\n", device, group,
			atomic_read(&group->sensor_fcount), group->fcount);

	clear_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state);
	clear_bit(FIMC_IS_SUBDEV_START, &group->leader.state);
	clear_bit(FIMC_IS_GROUP_START, &group->state);
	clear_bit(FIMC_IS_SUBDEV_RUN, &group->leader.state);

p_err:
	return -errcnt;
}

int fimc_is_group_buffer_queue(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group,
	struct fimc_is_queue *queue,
	u32 index)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_device_ischain *device;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
#ifdef CONFIG_COMPANION_DIRECT_USE
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor *sensor = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;
#endif
	u32 orientation;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(!group->device);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);
	BUG_ON(group->id >= GROUP_ID_MAX);
	BUG_ON(!queue);

	device = group->device;
	resourcemgr = device->resourcemgr;
	framemgr = &queue->framemgr;

	BUG_ON(index >= framemgr->num_frames);
#ifdef CONFIG_COMPANION_DIRECT_USE
	sensor = device->sensor;
	BUG_ON(!sensor);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(sensor->subdev_module);
	BUG_ON(!module);

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
	BUG_ON(!sensor_peri);

	cis_data = sensor_peri->cis.cis_data;
	BUG_ON(!cis_data);
#endif

	/* 1. check frame validation */
	frame = &framemgr->frames[index];

	if (unlikely(!test_bit(FRAME_MEM_INIT, &frame->mem_state))) {
		err("frame %d is NOT init", index);
		ret = EINVAL;
		goto p_err;
	}

	PROGRAM_COUNT(0);

	/* 2. update frame manager */
	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_22, flags);

	if (frame->state == FS_FREE) {
		if (unlikely(frame->out_flag)) {
			mgwarn("output(0x%lX) is NOT completed", device, group, frame->out_flag);
			frame->out_flag = 0;
		}

		if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state) &&
			(framemgr->queued_count[FS_REQUEST] >= DIV_ROUND_UP(framemgr->num_frames, 2))) {
				mgwarn(" request bufs : %d", device, group, framemgr->queued_count[FS_REQUEST]);
				frame_manager_print_queues(framemgr);
				if (test_bit(FIMC_IS_HAL_DEBUG_PILE_REQ_BUF, &sysfs_debug.hal_debug_mode)) {
					mdelay(sysfs_debug.hal_debug_delay);
					panic("HAL panic for request bufs");
				}
		}

		/* orientation is set by user */
		orientation = frame->shot->uctl.scalerUd.orientation;
		memset(&frame->shot->uctl.scalerUd, 0, sizeof(struct camera2_scaler_uctl));
		frame->shot->uctl.scalerUd.orientation = orientation;

		frame->lindex = 0;
		frame->hindex = 0;
		frame->result = 0;
		frame->fcount = frame->shot->dm.request.frameCount;
		frame->rcount = frame->shot->ctl.request.frameCount;
		frame->groupmgr = groupmgr;
		frame->group    = group;

#ifdef FIXED_FPS_DEBUG
		frame->shot->ctl.aa.aeTargetFpsRange[0] = FIXED_FPS_VALUE;
		frame->shot->ctl.aa.aeTargetFpsRange[1] = FIXED_FPS_VALUE;
		frame->shot->ctl.sensor.frameDuration = 1000000000/FIXED_FPS_VALUE;
#endif

		if (resourcemgr->limited_fps) {
			frame->shot->ctl.aa.aeTargetFpsRange[0] = resourcemgr->limited_fps;
			frame->shot->ctl.aa.aeTargetFpsRange[1] = resourcemgr->limited_fps;
			frame->shot->ctl.sensor.frameDuration = 1000000000/resourcemgr->limited_fps;
		}

#ifdef ENABLE_FAST_SHOT
		/* only fast shot can be enabled in case hal 1.0 */
		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state) &&
			(resourcemgr->hal_version == IS_HAL_VER_1_0)) {
			memcpy(&group->fast_ctl.aa, &frame->shot->ctl.aa,
				sizeof(struct camera2_aa_ctl));
			memcpy(&group->fast_ctl.scaler, &frame->shot->ctl.scaler,
				sizeof(struct camera2_scaler_ctl));
		}
#endif

		trans_frame(framemgr, frame, FS_REQUEST);
	} else {
		err("frame(%d) is invalid state(%d)\n", index, frame->state);
		frame_manager_print_queues(framemgr);
		ret = -EINVAL;
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_22, flags);

#ifdef CONFIG_COMPANION_DIRECT_USE
	if (test_bit(FIMC_IS_SENSOR_PREPROCESSOR_AVAILABLE, &sensor_peri->peri_state)) {
		/* PAF */
		if (cis_data->companion_data.paf_stat_enable == false) {
			frame->shot->uctl.companionUd.paf_mode = COMPANION_PAF_OFF;
		}

		/* CAF */
		if (cis_data->companion_data.caf_stat_enable == false) {
			frame->shot->uctl.companionUd.caf_mode = COMPANION_CAF_OFF;
		}

		/* WDR */
		if (cis_data->companion_data.wdr_enable == false) {
			frame->shot->uctl.companionUd.wdr_mode = COMPANION_WDR_OFF;
		}
	}
#endif
	fimc_is_group_start_trigger(groupmgr, group, frame);

	PROGRAM_COUNT(1);

p_err:
	return ret;
}

int fimc_is_group_buffer_finish(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group, u32 index)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_device_ischain *device;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(!group->device);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);

	device = group->device;

	if (unlikely(!test_bit(FIMC_IS_GROUP_OPEN, &group->state))) {
		warn("group was closed..(%d)", index);
		return ret;
	}

	if (unlikely(!group->leader.vctx)) {
		mgerr("leder vctx is null(%d)", device, group, index);
		ret = -EINVAL;
		return ret;
	}

	if (unlikely(group->id >= GROUP_ID_MAX)) {
		mgerr("group id is invalid(%d)", device, group, index);
		ret = -EINVAL;
		return ret;
	}

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	BUG_ON(index >= framemgr->num_frames);

	frame = &framemgr->frames[index];
	/* 2. update frame manager */
	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_23, flags);

	if (frame->state == FS_COMPLETE) {
		trans_frame(framemgr, frame, FS_FREE);

		frame->shot_ext->free_cnt = framemgr->queued_count[FS_FREE];
		frame->shot_ext->request_cnt = framemgr->queued_count[FS_REQUEST];
		frame->shot_ext->process_cnt = framemgr->queued_count[FS_PROCESS];
		frame->shot_ext->complete_cnt = framemgr->queued_count[FS_COMPLETE];
	} else {
		mgerr("frame(%d) is not com state(%d)", device, group, index, frame->state);
		frame_manager_print_queues(framemgr);
		ret = -EINVAL;
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_23, flags);

	PROGRAM_COUNT(15);

	return ret;
}

static int fimc_is_group_check_pre(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_device_ischain *device,
	struct fimc_is_group *gprev,
	struct fimc_is_group *group,
	struct fimc_is_group *gnext,
	struct fimc_is_frame *frame,
	struct fimc_is_group_frame **result,
	bool skip_chk_gframe)
{
	int ret = 0;
	struct fimc_is_group *group_leader;
	struct fimc_is_group_framemgr *gframemgr;
	struct fimc_is_group_frame *gframe;
	ulong flags;

	BUG_ON(!groupmgr);
	BUG_ON(!device);
	BUG_ON(!group);
	BUG_ON(!frame);
	BUG_ON(!frame->shot_ext);

	gframemgr = &groupmgr->gframemgr[device->instance];
	group_leader = groupmgr->leader[device->instance];

	/* invalid shot can be processed only on memory input */
	if (unlikely(!test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state) &&
		frame->shot_ext->invalid)) {
		mgrerr("invalid shot", device, group, frame);
		ret = -EINVAL;
		goto p_err;
	}

	/* skip the gframe check */
	if (skip_chk_gframe)
		return fimc_is_frame_check_without_gframe(group, frame);

	spin_lock_irqsave(&gframemgr->gframe_slock, flags);

	if (gprev && !gnext) {
		/* tailer */
		fimc_is_gframe_group_head(group, &gframe);
		if (unlikely(!gframe)) {
			mgrerr("gframe is NULL3", device, group, frame);
			fimc_is_stream_status(groupmgr, group_leader);
			fimc_is_gframe_print_free(gframemgr);
			fimc_is_gframe_print_group(group_leader);
			spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);
			ret = -EINVAL;
			goto p_err;
		}

		if (unlikely(frame->fcount != gframe->fcount)) {
			mgwarn("shot mismatch(%d != %d)", device, group,
				frame->fcount, gframe->fcount);
			gframe = fimc_is_gframe_rewind(groupmgr, group, frame->fcount);
			if (!gframe) {
				spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);
				merr("rewinding is fail,can't recovery", group);
				goto p_err;
			}
		}

		fimc_is_gframe_s_info(gframe, group->slot, frame);
		fimc_is_gframe_check(gprev, group, gnext, gframe, frame);
	} else if (!gprev && gnext) {
		/* leader */
		if (atomic_read(&group->scount))
			group->fcount += frame->num_buffers;
		else
			group->fcount++;

		fimc_is_gframe_free_head(gframemgr, &gframe);
		if (unlikely(!gframe)) {
			mgerr("gframe is NULL2", device, group);
			fimc_is_stream_status(groupmgr, group_leader);
			fimc_is_gframe_print_free(gframemgr);
			fimc_is_gframe_print_group(group_leader);
			spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);
			group->fcount -= frame->num_buffers;
			ret = -EINVAL;
			goto p_err;
		}

		if (unlikely(!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) &&
			(frame->fcount != group->fcount))) {
			if (frame->fcount > group->fcount) {
				mgwarn("shot mismatch(%d, %d)", device, group,
					frame->fcount, group->fcount);
				group->fcount = frame->fcount;
			} else {
				spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);
				mgerr("shot mismatch(%d, %d)", device, group,
					frame->fcount, group->fcount);
				group->fcount -= frame->num_buffers;
				ret = -EINVAL;
				goto p_err;
			}
		}

		gframe->fcount = frame->fcount;
		fimc_is_gframe_s_info(gframe, group->slot, frame);
		fimc_is_gframe_check(gprev, group, gnext, gframe, frame);
	} else if (gprev && gnext) {
		/* middler */
		fimc_is_gframe_group_head(group, &gframe);
		if (unlikely(!gframe)) {
			mgrerr("gframe is NULL3", device, group, frame);
			fimc_is_stream_status(groupmgr, group_leader);
			fimc_is_gframe_print_free(gframemgr);
			fimc_is_gframe_print_group(group_leader);
			spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);
			ret = -EINVAL;
			goto p_err;
		}

		if (unlikely(frame->fcount != gframe->fcount)) {
			mgwarn("shot mismatch(%d != %d)", device, group,
				frame->fcount, gframe->fcount);
			gframe = fimc_is_gframe_rewind(groupmgr, group, frame->fcount);
			if (!gframe) {
				spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);
				merr("rewinding is fail,can't recovery", group);
				goto p_err;
			}
		}

		fimc_is_gframe_s_info(gframe, group->slot, frame);
		fimc_is_gframe_check(gprev, group, gnext, gframe, frame);
	} else {
		/* single */
		if (atomic_read(&group->scount))
			group->fcount += frame->num_buffers;
		else
			group->fcount++;

		fimc_is_gframe_free_head(gframemgr, &gframe);
		if (unlikely(!gframe)) {
			mgerr("gframe is NULL4", device, group);
			fimc_is_stream_status(groupmgr, group_leader);
			fimc_is_gframe_print_free(gframemgr);
			fimc_is_gframe_print_group(group_leader);
			spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);
			ret = -EINVAL;
			goto p_err;
		}

		if (unlikely(!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) &&
			(frame->fcount != group->fcount))) {
			if (frame->fcount > group->fcount) {
				mgwarn("shot mismatch(%d != %d)", device, group,
					frame->fcount, group->fcount);
				group->fcount = frame->fcount;
			} else {
				spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);
				mgerr("shot mismatch(%d, %d)", device, group,
					frame->fcount, group->fcount);
				group->fcount -= frame->num_buffers;
				ret = -EINVAL;
				goto p_err;
			}
		}

		fimc_is_gframe_s_info(gframe, group->slot, frame);
		fimc_is_gframe_check(gprev, group, gnext, gframe, frame);
	}

	*result = gframe;

	spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);

p_err:
	return ret;
}

static int fimc_is_group_check_post(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_device_ischain *device,
	struct fimc_is_group *gprev,
	struct fimc_is_group *group,
	struct fimc_is_group *gnext,
	struct fimc_is_frame *frame,
	struct fimc_is_group_frame *gframe)
{
	int ret = 0;
	struct fimc_is_group *tail;
	struct fimc_is_group_framemgr *gframemgr;
	ulong flags;

	BUG_ON(!groupmgr);
	BUG_ON(!device);
	BUG_ON(!group);
	BUG_ON(!frame);
	BUG_ON(!gframe);
	BUG_ON(group->slot >= GROUP_SLOT_MAX);

	tail = group->tail;
	gframemgr = &groupmgr->gframemgr[group->instance];

	spin_lock_irqsave(&gframemgr->gframe_slock, flags);

	if (gprev && !gnext) {
		/* tailer */
		ret = fimc_is_gframe_trans_grp_to_fre(gframemgr, gframe, group);
		if (ret) {
			mgerr("fimc_is_gframe_trans_grp_to_fre is fail(%d)", device, group, ret);
			BUG();
		}
	} else if (!gprev && gnext) {
		/* leader */
		if (!tail || !tail->junction) {
			mgerr("junction is NULL", device, group);
			BUG();
		}

		if (gframe->group_cfg[group->slot].capture[tail->junction->cid].request) {
			ret = fimc_is_gframe_trans_fre_to_grp(gframemgr, gframe, group, gnext);
			if (ret) {
				mgerr("fimc_is_gframe_trans_fre_to_grp is fail(%d)", device, group, ret);
				BUG();
			}
		}
	} else if (gprev && gnext) {
		/* middler */
		if (!tail || !group->junction) {
			mgerr("junction is NULL", device, group);
			BUG();
		}

		/* gframe should be destroyed if the request of junction is zero, so need to check first */
		if (gframe->group_cfg[group->slot].capture[tail->junction->cid].request) {
			ret = fimc_is_gframe_trans_grp_to_grp(gframemgr, gframe, group, gnext);
			if (ret) {
				mgerr("fimc_is_gframe_trans_grp_to_grp is fail(%d)", device, group, ret);
				BUG();
			}
		} else {
			ret = fimc_is_gframe_trans_grp_to_fre(gframemgr, gframe, group);
			if (ret) {
				mgerr("fimc_is_gframe_trans_grp_to_fre is fail(%d)", device, group, ret);
				BUG();
			}
		}
	} else {
		/* single */
		gframe->fcount = frame->fcount;
	}

	spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);

	return ret;
}

int fimc_is_group_shot(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group,
	struct fimc_is_frame *frame)
{
	int ret = 0;
	struct fimc_is_device_ischain *device;
	struct fimc_is_resourcemgr *resourcemgr;
	struct fimc_is_group *gprev, *gnext;
	struct fimc_is_group_frame *gframe;
	struct fimc_is_group_task *gtask;
	bool try_sdown = false;
	bool try_rdown = false;
	struct fimc_is_core *core;
	int resol, stream_cnt;
	bool skip_chk_gframe = false;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(!group->shot_callback);
	BUG_ON(!group->device);
	BUG_ON(!frame);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);
	BUG_ON(group->id >= GROUP_ID_MAX);

	set_bit(FIMC_IS_GROUP_SHOT, &group->state);
	atomic_dec(&group->rcount);
	device = group->device;
	gtask = &groupmgr->gtask[group->id];

	if (unlikely(test_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state))) {
		mgwarn(" cancel by fstop1", group, group);
		ret = -EINVAL;
		goto p_err_cancel;
	}

	if (unlikely(test_bit(FIMC_IS_GTASK_REQUEST_STOP, &gtask->state))) {
		mgerr(" cancel by gstop1", group, group);
		ret = -EINVAL;
		goto p_err_ignore;
	}

	PROGRAM_COUNT(2);
	ret = smp_shot_dec(group);
	if (ret) {
		mgerr(" down fail(%d) #1", group, group, ret);
		goto p_err_ignore;
	}
	try_sdown = true;

	PROGRAM_COUNT(3);
	ret = down_interruptible(&gtask->smp_resource);
	if (ret) {
		mgerr(" down fail(%d) #2", group, group, ret);
		goto p_err_ignore;
	}
	try_rdown = true;

	if (device->sensor && !test_bit(FIMC_IS_SENSOR_FRONT_START, &device->sensor->state)) {
		/*
		 * this statement is execued only at initial.
		 * automatic increase the frame count of sensor
		 * for next shot without real frame start
		 */
		if (group->init_shots > atomic_read(&group->scount)) {
			frame->fcount = atomic_read(&group->sensor_fcount);
			atomic_set(&group->backup_fcount, frame->fcount);
			/* for multi-buffer */
			if (frame->num_buffers)
				atomic_set(&group->sensor_fcount, frame->fcount + frame->num_buffers);
			else
				atomic_inc(&group->sensor_fcount);

			goto p_skip_sync;
		}
	}

	if (group->sync_shots) {
		bool try_sync_shot = false;

		if (group->asyn_shots == 0) {
			try_sync_shot = true;
		} else {
			if ((smp_shot_get(group) < MIN_OF_SYNC_SHOTS))
				try_sync_shot = true;
			else
				if (atomic_read(&group->backup_fcount) >=
					atomic_read(&group->sensor_fcount))
					try_sync_shot = true;
		}

		if (try_sync_shot) {
			PROGRAM_COUNT(4);
			ret = down_interruptible(&group->smp_trigger);
			if (ret) {
				mgerr(" down fail(%d) #4", group, group, ret);
				goto p_err_ignore;
			}
		}

		/* for multi-buffer */
		if (frame->num_buffers) {
			if (atomic_read(&group->sensor_fcount) <= (atomic_read(&group->backup_fcount) + frame->num_buffers)) {
				frame->fcount = atomic_read(&group->backup_fcount) + frame->num_buffers;
			} else {
				frame->fcount = atomic_read(&group->sensor_fcount);
				mgwarn(" frame count reset by sensor focunt(%d->%d)", group, group,
					atomic_read(&group->backup_fcount) + frame->num_buffers,
					frame->fcount);
			}
		} else {
			frame->fcount = atomic_read(&group->sensor_fcount);
		}
		atomic_set(&group->backup_fcount, frame->fcount);

		/* real automatic increase */
		if (!try_sync_shot && (smp_shot_get(group) > MIN_OF_SYNC_SHOTS)) {
			/* for multi-buffer */
			if (frame->num_buffers)
				atomic_add(frame->num_buffers, &group->sensor_fcount);
			else
				atomic_inc(&group->sensor_fcount);
		}
	} else {
		if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
			frame->fcount = fimc_is_sensor_g_fcount(device->sensor) + 1;
			if (frame->fcount <= atomic_read(&group->backup_fcount))
				frame->fcount = atomic_read(&group->backup_fcount) + 1;
			atomic_set(&group->backup_fcount, frame->fcount);
		}
	}

p_skip_sync:
	if (unlikely(test_bit(FIMC_IS_GROUP_FORCE_STOP, &group->state))) {
		mgwarn(" cancel by fstop2", group, group);
		ret = -EINVAL;
		goto p_err_cancel;
	}

	if (unlikely(test_bit(FIMC_IS_GTASK_REQUEST_STOP, &gtask->state))) {
		mgerr(" cancel by gstop2", group, group);
		ret = -EINVAL;
		goto p_err_ignore;
	}

	core = (struct fimc_is_core *)device->interface->core;
	resol = fimc_is_get_target_resol(device);
	stream_cnt = fimc_is_get_start_sensor_cnt(core);

	/* HACK: try to flush all remained gframes in Dual UHD */
	if ((stream_cnt > 1) && (resol >= SIZE_UHD))
		skip_chk_gframe = true;

	PROGRAM_COUNT(6);
	gnext = group->gnext;
	gprev = group->gprev;
	resourcemgr = device->resourcemgr;
	gframe = NULL;

	/*
	 * pipe shot callback
	 * For example, junction qbuf was processed in this callback
	 */
	if (group->pipe_shot_callback)
		group->pipe_shot_callback(device, group, frame);

	ret = fimc_is_group_check_pre(groupmgr, device, gprev, group, gnext, frame, &gframe, skip_chk_gframe);
	if (unlikely(ret)) {
		merr(" fimc_is_group_check_pre is fail(%d)", device, ret);
		goto p_err_cancel;
	}

	if (!skip_chk_gframe && unlikely(!gframe)) {
		merr(" gframe is NULL", device);
		goto p_err_cancel;
	}

#ifdef DEBUG_AA
	fimc_is_group_debug_aa_shot(group, frame);
#endif

	fimc_is_group_set_torch(group, frame);

#ifdef ENABLE_SHARED_METADATA
	fimc_is_hw_shared_meta_update(device, group, frame, SHARED_META_SHOT);
#endif

	ret = group->shot_callback(device, frame);
	if (unlikely(ret)) {
		mgerr(" shot_callback is fail(%d)", group, group, ret);
		goto p_err_cancel;
	}

	fimc_is_dual_mode_update(device, group, frame);

#ifdef ENABLE_DVFS
	if ((!pm_qos_request_active(&device->user_qos)) && (sysfs_debug.en_dvfs)) {
		int scenario_id;

		mutex_lock(&resourcemgr->dvfs_ctrl.lock);

		/* try to find dynamic scenario to apply */
		fimc_is_dual_dvfs_update(device, group, frame);

		scenario_id = fimc_is_dvfs_sel_dynamic(device, group);
		if (scenario_id > 0) {
			struct fimc_is_dvfs_scenario_ctrl *dynamic_ctrl = resourcemgr->dvfs_ctrl.dynamic_ctrl;
			mgrinfo("tbl[%d] dynamic scenario(%d)-[%s]\n", device, group, frame,
				resourcemgr->dvfs_ctrl.dvfs_table_idx,
				scenario_id,
				dynamic_ctrl->scenarios[dynamic_ctrl->cur_scenario_idx].scenario_nm);
			fimc_is_set_dvfs((struct fimc_is_core *)device->interface->core, device, scenario_id);
		}

		if ((scenario_id < 0) && (resourcemgr->dvfs_ctrl.dynamic_ctrl->cur_frame_tick == 0)) {
			struct fimc_is_dvfs_scenario_ctrl *static_ctrl = resourcemgr->dvfs_ctrl.static_ctrl;
			mgrinfo("tbl[%d] restore scenario(%d)-[%s]\n", device, group, frame,
				resourcemgr->dvfs_ctrl.dvfs_table_idx,
				static_ctrl->cur_scenario_id,
				static_ctrl->scenarios[static_ctrl->cur_scenario_idx].scenario_nm);
			fimc_is_set_dvfs((struct fimc_is_core *)device->interface->core, device, static_ctrl->cur_scenario_id);
		}

		mutex_unlock(&resourcemgr->dvfs_ctrl.lock);
	}
#endif

	PROGRAM_COUNT(7);

	if (!skip_chk_gframe) {
		ret = fimc_is_group_check_post(groupmgr, device, gprev, group, gnext, frame, gframe);
		if (unlikely(ret)) {
			merr(" fimc_is_group_check_post is fail(%d)", device, ret);
			goto p_err_cancel;
		}
	}

	fimc_is_itf_grp_shot(device, group, frame);
	atomic_inc(&group->scount);

	clear_bit(FIMC_IS_GROUP_SHOT, &group->state);
	PROGRAM_COUNT(12);

	return ret;

p_err_ignore:
	if (try_sdown)
		smp_shot_inc(group);

	if (try_rdown)
		up(&gtask->smp_resource);

	clear_bit(FIMC_IS_GROUP_SHOT, &group->state);
	PROGRAM_COUNT(12);

	return ret;

p_err_cancel:
	fimc_is_group_cancel(group, frame);

	if (try_sdown)
		smp_shot_inc(group);

	if (try_rdown)
		up(&gtask->smp_resource);

	clear_bit(FIMC_IS_GROUP_SHOT, &group->state);
	PROGRAM_COUNT(12);

	return ret;
}

int fimc_is_group_done(struct fimc_is_groupmgr *groupmgr,
	struct fimc_is_group *group,
	struct fimc_is_frame *frame,
	u32 done_state)
{
	int ret = 0;
	struct fimc_is_device_ischain *device;
	struct fimc_is_group_framemgr *gframemgr;
	struct fimc_is_group_frame *gframe;
	struct fimc_is_group *gnext;
	struct fimc_is_group_task *gtask;
#if !defined(ENABLE_SHARED_METADATA)
	struct fimc_is_group *child;
#endif
	ulong flags;

	BUG_ON(!groupmgr);
	BUG_ON(!group);
	BUG_ON(!frame);
	BUG_ON(group->instance >= FIMC_IS_STREAM_COUNT);
	BUG_ON(group->id >= GROUP_ID_MAX);
	BUG_ON(!group->device);

	/* check shot & resource count validation */
	device = group->device;
	gnext = group->gnext;
	gframemgr = &groupmgr->gframemgr[group->instance];
	gtask = &groupmgr->gtask[group->id];

	if (unlikely(test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state) &&
		(done_state != VB2_BUF_STATE_DONE))) {
		merr("G%d NOT DONE(reprocessing)\n", group, group->id);
		fimc_is_hw_logdump(device->interface);
	}

#ifdef DEBUG_AA
	fimc_is_group_debug_aa_done(group, frame);
#endif

	/* sensor tagging */
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state))
		fimc_is_sensor_dm_tag(device->sensor, frame);

#ifdef ENABLE_SHARED_METADATA
	fimc_is_hw_shared_meta_update(device, group, frame, SHARED_META_SHOT_DONE);
#else
	child = group;
	while(child) {
#ifdef ENABLE_INIT_AWB
		/* wb gain backup for initial AWB */
		if (device->sensor && ((child == &device->group_isp) || (child->subdev[ENTRY_ISP])))
			memcpy(device->sensor->last_wb, frame->shot->dm.color.gains,
					sizeof(float) * WB_GAIN_COUNT);
#endif
		if ((child == &device->group_vra) || (child->subdev[ENTRY_VRA])) {
#ifdef ENABLE_FD_SW
			fimc_is_vra_trigger(device, &group->leader, frame);
#endif
			/* fd information backup */
			memcpy(&device->cur_peri_ctl.fdUd, &frame->shot->dm.stats,
				sizeof(struct camera2_fd_uctl));
		}
		child = child->child;
	}
#endif

	/* gframe should move to free list next group is existed and not done is oocured */
	if (unlikely((done_state != VB2_BUF_STATE_DONE) && gnext)) {
		spin_lock_irqsave(&gframemgr->gframe_slock, flags);

		fimc_is_gframe_group_head(gnext, &gframe);
		if (gframe && (gframe->fcount == frame->fcount)) {
			ret = fimc_is_gframe_trans_grp_to_fre(gframemgr, gframe, gnext);
			if (ret) {
				mgerr("fimc_is_gframe_trans_grp_to_fre is fail(%d)", device, gnext, ret);
				BUG();
			}
		}

		spin_unlock_irqrestore(&gframemgr->gframe_slock, flags);
	}

	smp_shot_inc(group);
	up(&gtask->smp_resource);

	if (unlikely(frame->result)) {
		ret = fimc_is_devicemgr_shot_done(group, frame, frame->result);
		if (ret) {
			mgerr("fimc_is_devicemgr_shot_done is fail(%d)", device, group, ret);
			return ret;
		}
	}

	return ret;
}
