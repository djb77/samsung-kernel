/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DMABUF_CONTAINER_UAPI_H__
#define __DMABUF_CONTAINER_UAPI_H__

#define ION_IOC_CUSTOM_MAGIC	'I'

#define MAX_BUFCON_BUFS 32
/*
 * struct dmabuf_container_data - container description
 *
 * @fd:				a file descriptor representing each buffers
 * @container_fd:	a file descriptor representing container
 * @count:			the number of the buffers
 */
struct dmabuf_container_data {
	__s32 fds[MAX_BUFCON_BUFS];
	__u32 count;
	__s32 container_fd;
};

#define ION_IOC_CUSTOM_CONTAINER_CREATE \
		_IOWR(ION_IOC_CUSTOM_MAGIC, 0, struct dmabuf_container_data)

#endif
