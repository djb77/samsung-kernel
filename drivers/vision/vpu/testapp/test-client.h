/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VPU_TEST_CLIENT_H_
#define VPU_TEST_CLIENT_H_

#include <stdio.h>
#include <vs4l.h>
#include <pthread.h>

#include "ion.h"
#include "test-list.h"

#define GRAPH_IOSET_MAX_CNT		100
/* #define GRAPH_ION_CACHE */

struct client_config {
	struct vs4l_format_list		flist;
	struct vs4l_container_list	*clist;
	int				blist_cnt[50];
};

struct client_graph_io {
	struct list_head		list;
	struct list_head		chain_list;
	int				direction;
	int				pu_id;
	int				chain_id;
	int				pu_index;
	int				chain_index;
	int				target;
	int				width;
	int				height;
	int				pixel_bytes;
	int				format;
	int				flags;

	int				buffer_cnt;
	int				map_index[6];
	struct vpul_memory_map_desc     *map_desc;
	unsigned int			*ext_desc;

	int				fd;
	unsigned long			va;
};

struct client_chain {
	int				in_cnt;
	struct list_head		in_list;
	int				ot_cnt;
	struct list_head		ot_list;
};

struct client_graph_info {
	struct vpul_task		*task;

	int				in_cnt;
	struct list_head		in_list;
	int				ot_cnt;
	struct list_head		ot_list;
	int				im_cnt;
	struct list_head		im_list;
	int				io_cnt;
	struct client_graph_io		ioset[GRAPH_IOSET_MAX_CNT];

	int				chain_cnt;
	struct client_chain		chainset[GRAPH_IOSET_MAX_CNT];
};

struct client {
	int 				fd;
	int				id;
	int				fcount;
	int				str_loop;
	int				end_loop;

	int				chain_cnt;
	int				pu_cnt;
	struct vpul_vertex		*vertex_array;
	struct vpul_subchain		*chain_array;
	struct vpul_pu			*pu_array;
	short				*td_binary;
	int				td_size;

	pthread_t			thread;
	pthread_mutex_t 		mutex;
	ion_client			iclient;

	int				buffer_cnt;
	struct vs4l_graph		graph;
	struct client_config		incfg;
	struct client_config		otcfg;

	struct client_graph_info	ginfo;
};

int vpu_open(struct client *client);
int vpu_close(struct client *client);
int vpu_s_graph(struct client *client, struct vs4l_graph *graph);
int vpu_s_format(struct client *client, struct vs4l_format_list *flist);
int vpu_s_param(struct client *client, struct vs4l_param_list *plist);
int vpu_s_ctrl(struct client *client, struct vs4l_ctrl *ctrl);
int vpu_stream_on(struct client *client);
int vpu_stream_off(struct client *client);
int vpu_poll(struct client *client);
int vpu_qbuf(struct client *client, struct vs4l_container_list *c);
int vpu_dqbuf(struct client *client, struct vs4l_container_list *c);

int client_g_size(struct vs4l_format *format);
int client_compare_io(char *in, char *ot, int size);

int client_s_ginfo(struct client *client, struct vpul_task *task);
int client_s_format(struct client *client);
int client_s_buffer(struct client *client);
int client_p_buffer(struct client *client);
int client_s_intermediate(struct client *client);

struct vs4l_buffer * find_target_buffer(struct vs4l_container_list *clist, int target, int index);
struct vs4l_format * find_target_format(struct vs4l_format_list *flist, int target);
#endif