/*
 * Samsung Exynos SoC series SCore driver
 *
 * Definition of SCore packet (v2.0)
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_PACKET_H__
#define __SCORE_PACKET_H__

struct score_host_buffer {
	unsigned int				memory_type :3;
	unsigned int				offset      :29;
	unsigned int				memory_size;
	union {
		int				fd;
		unsigned long			userptr;
		unsigned long long		mem_info;
	} m;
	unsigned int				addr_offset :29;
	unsigned int				type        :3;
	unsigned int				reserved;
};

struct score_host_packet_info {
	unsigned int				valid_size;
	unsigned int				buf_count;
	unsigned int				payload[0];
};

struct score_host_packet {
	unsigned int				size;
	unsigned int				packet_offset;
	unsigned int				version     :24;
	unsigned int				reserved    :8;
	unsigned int				payload[0];
};

#endif
