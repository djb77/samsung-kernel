/* include/uapi/linux/hwastc.h
 *
 * Userspace API for Samsung ASTC encoder driver
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

#ifndef HWASTC_API
#define HWASTC_API

#include <linux/ioctl.h>
#include <linux/types.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

/*
 * NOTE: THESE ENUMS ARE ALSO USED BY USERSPACE TO SPECIFY SETTINGS AND THEY'RE
 *       SUPPOSED TO WORK WITH FUTURE HARDWARE AS WELL (WHICH MAY OR MAY NOT
 *       SUPPORT ALL THE SETTINGS.
 *
 *       FOR THAT REASON, YOU MUST NOT CHANGE THE ORDER OF VALUES,
 *       AND NEW VALUES MUST BE ADDED AT THE END OF THE ENUMS TO AVOID
 *       API BREAKAGES.
 */


/* blockSize:	        Block size – 4x4 / 6x6 / 8x8 */
enum astc_encoder_blk_size {
	ASTC_ENCODE_MODE_BLK_SIZE_4x4,
	ASTC_ENCODE_MODE_BLK_SIZE_6x6,
	ASTC_ENCODE_MODE_BLK_SIZE_8x8,
};

/* wt_ref_iterations:
 * Number of refinement iterations- 1 time / 2 times / 4 times
 */
enum astc_encoder_wt_ref_iter {
	ASTC_ENCODE_MODE_REF_ITER_ZERO,
	ASTC_ENCODE_MODE_REF_ITER_ONE,
	ASTC_ENCODE_MODE_REF_ITER_TWO,
	ASTC_ENCODE_MODE_REF_ITER_FOUR, //not a typo
};

/* partitions:	        Single partition / Two partition */
enum astc_encoder_partitioning {
	ASTC_ENCODE_MODE_PARTITION_ONE,
	ASTC_ENCODE_MODE_PARTITION_TWO,
};

/* enc_blk_mode:	The number of encoding block mode */
enum astc_encoder_no_block_mode {
	ASTC_ENCODE_MODE_NO_BLOCK_MODE_16EA,
	ASTC_ENCODE_MODE_NO_BLOCK_MODE_FULL,
};

/* enc_blk_mode:	The number of encoding block mode */
enum astc_encoder_input_pixel_format {
	ASTC_ENCODE_INPUT_PIXEL_FORMAT_RGBA8888,
	ASTC_ENCODE_INPUT_PIXEL_FORMAT_ARGB8888,
	ASTC_ENCODE_INPUT_PIXEL_FORMAT_BGRA8888,
	ASTC_ENCODE_INPUT_PIXEL_FORMAT_ABGR8888,
};

struct hwASTC_pix_format {
	__u32 fmt;
	__u32 width;
	__u32 height;
};

enum hwASTC_buffer_type {
	HWASTC_BUFFER_NONE,	/* no buffer is set */
	HWASTC_BUFFER_DMABUF,
	HWASTC_BUFFER_USERPTR,
};

//A buffer can either be backed by a DMA file descriptor OR by userspace memory
struct hwASTC_buffer {
	union {
		//the DMA file descriptor
		__s32 fd;
		//the pointer to userspace memory
		unsigned long userptr;
	};
	size_t len;
	__u8 type;
};

//This is crafted to be the same on 32/64bits platforms
//REMEMBER to update ioctl32 handling if that stops being the case!
struct astc_config {
	/* Block size – 4x4 / 6x6 / 8x8 */
	__u8       encodeBlockSize;
	/* Number of refinement iterations- 1 time / 2 times / 4 times */
	__u8       intref_iterations;
	/* Single partition / Two partition */
	__u8       partitions;
	/* The number of encoding block mode */
	__u8       num_blk_mode;
	/* Enable(true) / Disable(false) */
	__u8       dual_plane_enable;

	/*
	 * Explicit padding to align struct to 64bit,
	 * This way the memory layout should be the same on both 32 and 64bit,
	 * and will avoid the need for a 32-bit specific structure
	 * for compat_ioctl
	 */
	__u8       reserved[3];
};

struct hwASTC_task {
	//What we're giving as input to the device
	struct hwASTC_pix_format fmt_out;
	/*
	 * What we're capturing from the device as result
	 * Please note the pixel format is not always used in the code
	 * It is there to have a more abstract API that can accommodate
	 * more usecases
	 */
	struct hwASTC_pix_format fmt_cap;
	struct hwASTC_buffer buf_out;
	struct hwASTC_buffer buf_cap;
	struct astc_config enc_config;

	unsigned long reserved[2];
};


#define HWASTC_IOC_PROCESS	_IOWR('M',   0, struct hwASTC_task)


#endif // HWASTC_API

