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
 *       SUPPORT ALL THE SETTINGS).
 *
 *       FOR THAT REASON, YOU MUST NOT CHANGE THE ORDER OF VALUES,
 *       AND NEW VALUES MUST BE ADDED AT THE END OF THE ENUMS TO AVOID
 *       API BREAKAGES.
 */


/**
 * enum hwASTC_blk_size - The available block sizes
 *
 * @ASTC_ENCODE_MODE_BLK_SIZE_4x4	: Block size 4x4
 * @ASTC_ENCODE_MODE_BLK_SIZE_6x6	: Block size 6x6
 * @ASTC_ENCODE_MODE_BLK_SIZE_8x8	: Block size 8x8
 */
enum hwASTC_blk_size {
	ASTC_ENCODE_MODE_BLK_SIZE_4x4,
	ASTC_ENCODE_MODE_BLK_SIZE_6x6,
	ASTC_ENCODE_MODE_BLK_SIZE_8x8,
};

/**
 * enum hwASTC_wt_ref_iter - The number of weight refinement iterations
 *
 * @ASTC_ENCODE_MODE_REF_ITER_ZERO	: 0 iterations
 * @ASTC_ENCODE_MODE_REF_ITER_ONE	: 1 iteration
 * @ASTC_ENCODE_MODE_REF_ITER_TWO	: 2 iterations
 * @ASTC_ENCODE_MODE_REF_ITER_FOUR	: 4 iterations
 */
enum hwASTC_wt_ref_iter {
	ASTC_ENCODE_MODE_REF_ITER_ZERO,
	ASTC_ENCODE_MODE_REF_ITER_ONE,
	ASTC_ENCODE_MODE_REF_ITER_TWO,
	ASTC_ENCODE_MODE_REF_ITER_FOUR,
};

/**
 * enum hwASTC_partitioning - The available number of partitions
 *
 * @ASTC_ENCODE_MODE_PARTITION_ONE	: 1 partition
 * @ASTC_ENCODE_MODE_PARTITION_TWO	: 2 partitions
 */
enum hwASTC_partitioning {
	ASTC_ENCODE_MODE_PARTITION_ONE,
	ASTC_ENCODE_MODE_PARTITION_TWO,
};

/**
 * enum hwASTC_no_block_mode - The number of block modes to try
 *
 * @ASTC_ENCODE_MODE_NO_BLOCK_MODE_16EA	: 16 modes
 * @ASTC_ENCODE_MODE_NO_BLOCK_MODE_FULL	: All the available block modes
 */
enum hwASTC_no_block_mode {
	ASTC_ENCODE_MODE_NO_BLOCK_MODE_16EA,
	ASTC_ENCODE_MODE_NO_BLOCK_MODE_FULL,
};

/**
 * enum hwASTC_input_pixel_format - Available pixel formats for the input image
 *
 * @ASTC_ENCODE_INPUT_PIXEL_FORMAT_RGBA8888	: RGBA8888
 * @ASTC_ENCODE_INPUT_PIXEL_FORMAT_ARGB8888	: ARGB8888
 * @ASTC_ENCODE_INPUT_PIXEL_FORMAT_BGRA8888	: BGRA8888
 * @ASTC_ENCODE_INPUT_PIXEL_FORMAT_ABGR8888	: ABGR8888
 */
enum hwASTC_input_pixel_format {
	ASTC_ENCODE_INPUT_PIXEL_FORMAT_RGBA8888,
	ASTC_ENCODE_INPUT_PIXEL_FORMAT_ARGB8888,
	ASTC_ENCODE_INPUT_PIXEL_FORMAT_BGRA8888,
	ASTC_ENCODE_INPUT_PIXEL_FORMAT_ABGR8888,
};

/**
 * struct hwASTC_img_info - the image format and size
 *
 * @fmt	        : the pixel format, see @hwASTC_input_pixel_format
 * @width       : width of the image
 * @height      : height of the image
 */
struct hwASTC_img_info {
	__u32 fmt;
	__u32 width;
	__u32 height;
};

/**
 * enum hwASTC_buffer_type - the type of the buffer sent to the driver
 *
 * @HWASTC_BUFFER_NONE          : no buffer is set
 * @HWASTC_BUFFER_DMABUF        : DMA buffer
 * @HWASTC_BUFFER_USERPTR       : a pointer to user memory
 */
enum hwASTC_buffer_type {
	HWASTC_BUFFER_NONE,	/* no buffer is set */
	HWASTC_BUFFER_DMABUF,
	HWASTC_BUFFER_USERPTR,
};

/**
 * struct hwASTC_buffer - the struct holding the description of a buffer
 *
 * @fd		: the DMA file descriptor, if the buffer is a DMA buffer.
 *		  This is in a union with @userptr, only one should be set.
 * @userptr	: the ptr to user memory, if the buffer is in user memory.
 *		  This is in a union with @fd, only one should be set.
 * @len		: the declared buffer length, in bytes
 * @type	: the declared buffer type, see @hwASTC_buffer_type
 *
 * A buffer can either be represented by a DMA file descriptor OR by a pointer
 * to user memory. The @len must be initialized with the size of the buffer,
 * and the @type must be set to the correct type.
 */
struct hwASTC_buffer {
	union {
		/* the DMA file descriptor */
		__s32 fd;
		/* the pointer to userspace memory */
		unsigned long userptr;
	};
	size_t len;
	__u8 type;
};

/**
 * struct hwASTC_config - The encoding parameters
 *
 * @encodeBlockSize	: block size enum value
 * @intref_iterations	: number of weight refinement iterations to try
 * @partitions		: the partition schemes to try
 * @num_blk_mode	: number of block modes to try
 * @dual_plane_enable	: Enable/Disable dual plane mode
 *
 * The fields needs to be explicitly set by userspace to the desired values,
 * using the corresponding enumerations.
 *
 * This struct is crafted to be the same on 32/64bits platforms
 * REMEMBER to update ioctl32 handling if that stops being the case!
 */
struct hwASTC_config {
	__u8       encodeBlockSize;
	__u8       intref_iterations;
	__u8       partitions;
	__u8       num_blk_mode;
	__u8       dual_plane_enable;

	/*
	 * Explicit padding to align struct to 64bit,
	 * This way the memory layout should be the same on both 32 and 64bit,
	 * and will avoid the need for a 32-bit specific structure
	 * for compat_ioctl
	 */
	__u8       reserved[3];
};

/**
 * struct hwASTC_task - the task information to be sent to the driver
 *
 * @info_out	: info related to the image sent to the HW for processing
 * @info_cap	: info related to the image obtained as result from the HW
 *		  This is currently unused as the output is assumed to be an
 *		  ASTC image of the same size as the input image.
 *		  Reserved for future purposes.
 * @buf_out	: info related to the buffer sent to the HW for processing
 * @buf_cap	: info related to the buffer holding the encoded result
 * @enc_config	: the encoding configuration
 *
 * This is the struct holding all the task information that the driver needs
 * to process a task.
 *
 * The client needs to fill it in with the necessary information and send it
 * to the driver through the IOCTL.
 */
struct hwASTC_task {
	struct hwASTC_img_info info_out;
	struct hwASTC_img_info info_cap;
	struct hwASTC_buffer buf_out;
	struct hwASTC_buffer buf_cap;
	struct hwASTC_config enc_config;

	/* private: internal use only */
	unsigned long reserved[2];
};

/* The main IOCTL, used to request processing of a task */
#define HWASTC_IOC_PROCESS	_IOWR('M',   0, struct hwASTC_task)

#endif // HWASTC_API

