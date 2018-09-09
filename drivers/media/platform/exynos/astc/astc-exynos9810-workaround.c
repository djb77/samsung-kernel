/* drivers/media/platform/exynos/astc/astc-exynos9810-workaround.c
 *
 * Implementation of functions needed for working around an Exynos9810 HW bug.
 *
 * This is part of the ASTC HW encoder kernel driver for Exynos SoCs.
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrea Bernabei <a.bernabei@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "astc-exynos9810-workaround.h"

// #define DEBUG

#include "../../../../iommu/exynos-iommu.h"
#include "astc-helper.h"


/*
 * WHAT IS THE PURPOSE OF THIS FILE?
 *
 * Exynos9810 HW ASTC chip will access memory out-of-bounds when img
 * height/width is not multiple of the blk size (although it will not
 * actually touch that memory).
 *
 * The size of the buffer that Exynos9810 ASTC HW encoder will access
 * is defined by the following formula:
 *
 * BUFF_SIZE = (IMG_WIDTH * ROUNDUP_TO_BLKSIZE(IMG_HEIGHT) + ROUNDUP_TO_BLKSIZE(IMG_WIDTH) - IMG_WIDTH)
 * 	              * BYTES_PER_PIXEL;
 *
 * This file includes all the helpers to workaround that bug.
 */


/*
 * Round-up a value to the next multiple of blk_size.
 * Used in the exynos9810 bug workaround.
 *
 * Most of the code in this function is to perform overflow/wraparound
 * checks on each intermediate arithmetic op.
 */
static inline u32 roundup_to_blksize(struct device *dev,
				u32 val_to_roundup, u32 blk_size, bool *okay) {
	u32 tmp_roundup_add = 0;
	u32 tmp_roundup_num_blocks = 0;

	// no need to roundup, it's already a multiple of blk size
	if (val_to_roundup % blk_size == 0) {
		*okay = true;
		return val_to_roundup;
	}

	if (!okay) {
		pr_err("%s: invalid result ptr!\n", __func__);
		goto err;
	}
	if (!dev) {
		pr_err("%s: null device!\n", __func__);
		goto err;
	}

	if (blk_size == 0) {
		dev_err(dev, "%s: blk size cannot be null.\n", __func__);
		goto err;
	}

	tmp_roundup_add = val_to_roundup + (blk_size - 1);
	/* wraparound check for the addition */
	if (tmp_roundup_add < val_to_roundup) {
		dev_err(dev, "%s: extended img size add wrapped around %u %u\n"
			, __func__, val_to_roundup, blk_size - 1);
		goto err;
	}

	tmp_roundup_num_blocks = tmp_roundup_add / blk_size;

	/* wraparound pre-check for the mul that we're about to do */
	if (tmp_roundup_num_blocks > (UINT_MAX / blk_size)) {
		dev_err(dev, "%s: extended img size mul wrapped around %u %u\n"
			, __func__, tmp_roundup_num_blocks, blk_size);
		goto err;
	}

	*okay = true;
	return tmp_roundup_num_blocks * blk_size;

err:
	if (okay) *okay = false;

	return val_to_roundup;
}

/*
 * Use the BUFF_SIZE formula reported at the top of this file to compute the
 * size of the buffer that the HW will try to access.
 *
 * Most of the code in this function is to perform overflow/wraparound
 * checks on each intermediate arithmetic op.
 *
 * TODO: move to GCC intrinsics once we move to GCC5+
 *
 * Return:
 *	0 on success,
 *	-ERANGE if the computation results triggers overflow or wraparound,
 *	-EINVAL otherwise.
 */
static int astc_exynos9810_img_buffer_size(struct device *dev,
					   struct astc_task *task,
					   u32 rounded_up_width,
					   u32 rounded_up_height,
					   size_t *expected_buffer_size)
{
	struct hwASTC_img_info *img_info = &task->user_task.info_out;
	int srcBitsPerPixel = 0;

	/* temp vars used in arithmetic ops wraparound checks */
	size_t num_pixels = 0;
	size_t tmp_add = 0;
	u32 roundedup_width_diff = 0;

	srcBitsPerPixel = ftm_to_bpp(dev, img_info);
	if (srcBitsPerPixel <= 0) {
		dev_err(dev, "%s: invalid pix format\n",
			__func__);
		return -EINVAL;
	}

	/*
	 * Compute the size of the buffer that the HW expects to be able to
	 * access.
	 * Most of the code is just overflow/wraparound checks for each
	 * intermediate arithmetic op.
	 */
	if (img_info->width > UINT_MAX / rounded_up_height) {
		dev_err(dev, "%s: extended buffer size mul wrapped around %u %u\n"
			, __func__, img_info->width, rounded_up_height);
		return -ERANGE;
	}
	num_pixels = (size_t) img_info->width * rounded_up_height;
	/* this is guaranteed to be positive, since the first op is roundup of the second */
	roundedup_width_diff = rounded_up_width - img_info->width;

	tmp_add = num_pixels + roundedup_width_diff;
	if (tmp_add < num_pixels) {
		dev_err(dev, "%s: extended buffer size add wrapped around %zu %u\n"
			, __func__, num_pixels, roundedup_width_diff);
		return -ERANGE;
	}
	if (tmp_add > SIZE_MAX / srcBitsPerPixel) {
		dev_err(dev, "%s: extended buffer size final mul wrapped around %zu %u\n"
			, __func__, tmp_add, srcBitsPerPixel);
	}
	*expected_buffer_size = tmp_add * srcBitsPerPixel;
	return 0;
}

/*
 * Check if the task requires the workaround to the Exynos9810 H/W bug.
 *
 * If the workaround is needed:
 * - Set the internal task state to signal that the workaround for that
 *   task is needed, so other functions can act accordingly
 * - Compute the rounded up width and height and store them in the vars
 *   passed in input
 * - Compute the size of the buffer extension that we need to map
 *   to workaround the H/W bug and store the size of the extension
 *   in extension_size.
 *
 * Return 0 on success, <0 on error.
 */
int astc_exynos9810_setup_workaround(struct device *dev,
				     struct astc_task *task,
				     u32 *rounded_up_width,
				     u32 *rounded_up_height,
				     size_t *extension_size) {
	struct hwASTC_img_info *img_info = &task->user_task.info_out;
	bool okay = false;
	int ret = 0;

	/*
	 * the internal repr of the output buffer, the one that we send to H/W
	 * and which it will read from
	 */
	struct astc_buffer_plane_dma *plane = &task->dma_buf_out.plane;

	/* the size of the buffer that Exynos9810 ASTC encoder expects to be
	 * able to access (which, due to the bug, is bigger than it should be)
	 */
	size_t expected_buffer_size = 0;

	u32 x_blk_size = blksize_enum_to_x(task->user_task.enc_config.encodeBlockSize);
	u32 y_blk_size = blksize_enum_to_y(task->user_task.enc_config.encodeBlockSize);

	if (x_blk_size == 0 || y_blk_size == 0) {
		dev_err(dev, "Invalid block size %u\n",
			task->user_task.enc_config.encodeBlockSize);
		return -EINVAL;
	}

	*rounded_up_width  = roundup_to_blksize(dev, img_info->width, x_blk_size, &okay);
	if (!okay) {
		dev_dbg(dev, "%s: img width roundup failed\n", __func__);
		return -ERANGE;
	}

	*rounded_up_height = roundup_to_blksize(dev, img_info->height, y_blk_size, &okay);
	if (!okay) {
		dev_dbg(dev, "%s: img height roundup failed\n", __func__);
		return -ERANGE;
	}

	if (*rounded_up_width != img_info->width
			|| *rounded_up_height != img_info->height) {
		dev_dbg(dev
			, "!!!!! EXYNOS9810 WORKAROUND: we might need to map "
			  "additional pages so that the HW can access out of bounds. "
			  "Original size %u x %u, extended size %u x %u\n"
			, img_info->width, img_info->height, *rounded_up_width, *rounded_up_height);

		/*
		 * store the fact that we're using the workaround, so the
		 * rest of the functions can act accordingly
		 */
		set_exynos9810bug_workaround_enable(task, true);
	} else {
		/* disable workaround and exit */
		set_exynos9810bug_workaround_enable(task, false);
		*extension_size = 0;
		return 0;
	}

	ret = astc_exynos9810_img_buffer_size(dev, task,
					      *rounded_up_width,
					      *rounded_up_height,
					      &expected_buffer_size);

	if (ret < 0) {
		dev_err(dev, "%s: exynos9810 workaround failed\n", __func__);
		return -1;
	}

	/*
	 * this should not happen, expected buffer size is bigger by definition,
	 * when the task requires the workaround.
	 */
	if (expected_buffer_size <= plane->bytes_used) {
		dev_err(dev, "%s: exynos9810 workaround failed, wrong buffer size\n", __func__);
		return -1;
	}

	*extension_size = expected_buffer_size - plane->bytes_used;

	return 0;
}
