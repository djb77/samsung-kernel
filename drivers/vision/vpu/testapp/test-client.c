#include <stdio.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <vs4l.h>
#include <lib/vpul-ds.h>
#include "test-client.h"
#include "test-list.h"
#include "test-util.h"

struct vs4l_buffer * find_target_buffer(struct vs4l_container_list *clist, int target, int index)
{
	int i;
	struct vs4l_container *container;

	container = clist[index].containers;
	for (i = 0; i < clist[index].count; ++i) {
		if (container[i].target == target) {
			return container[i].buffers;
		}
	}

	return NULL;
}

struct vs4l_format * find_target_format(struct vs4l_format_list *flist, int target)
{
	int i;
	struct vs4l_format *formats;

	formats = flist->formats;
	for (i = 0; i < flist->count; ++i) {
		if (formats[i].target == target) {
			return &formats[i];
		}
	}

	return NULL;
}

struct vs4l_buffer * __print_buffer_list(struct vs4l_container_list *clist, int index)
{
	int i;
	struct vs4l_container *container;

	container = clist[index].containers;
	for (i = 0; i < clist[index].count; ++i) {
		printf("buffer[%d] : %X, %d\n", i, container[i].target, container[i].buffers[0].m.fd);
	}

	return NULL;
}

int client_s_ginfo(struct client *client, struct vpul_task *task)
{
	int i, j, k;
	int n_vertex, n_subchain, n_pu;
	int io_index, roi_index;
	struct client_graph_info *ginfo;
	struct client_graph_io *io, *in, *in1, *in2, *ot, *im, *t1, *t2;
	struct client_chain *curr, *next;
	struct vpul_vertex *vertex;
	struct vpul_subchain *subchain;
	struct vpul_pu *pu;
	char *ptr;
	unsigned int pu_base, chain_base;

	ginfo = &client->ginfo;
	INIT_LIST_HEAD(&ginfo->in_list);
	INIT_LIST_HEAD(&ginfo->ot_list);
	INIT_LIST_HEAD(&ginfo->im_list);
	ginfo->in_cnt = 0;
	ginfo->ot_cnt = 0;
	ginfo->im_cnt = 0;
	ginfo->io_cnt = 0;
	ginfo->chain_cnt = 0;
	ginfo->task = task;

	ptr = (char *)task;
	pu_base = task->pus_vec_ofs + (unsigned int)ptr;
	chain_base = task->sc_vec_ofs + (unsigned int)ptr;
	n_vertex = task->t_num_of_vertices;
	vertex = (struct vpul_vertex *)(ptr + task->vertices_vec_ofs);

	for (i = 0; i < n_vertex; ++i) {
		n_subchain = vertex[i].num_of_subchains;
		for (j = 0; j < n_subchain; ++j) {
			ginfo->chainset[ginfo->chain_cnt].in_cnt = 0;
			ginfo->chainset[ginfo->chain_cnt].ot_cnt = 0;
			INIT_LIST_HEAD(&ginfo->chainset[ginfo->chain_cnt].in_list);
			INIT_LIST_HEAD(&ginfo->chainset[ginfo->chain_cnt].ot_list);
			ginfo->chain_cnt++;
		}
	}

	for (i = 0; i < n_vertex; ++i) {
		n_subchain = vertex[i].num_of_subchains;
		subchain = (struct vpul_subchain *)(ptr + vertex[i].sc_ofs);
		for (j = 0; j < n_subchain; ++j) {
			n_pu = subchain[j].num_of_pus;
			pu = (struct vpul_pu *)(ptr + subchain[j].pus_ofs);
			for (k = 0; k < n_pu; ++k) {
				if ((pu[k].instance >= VPU_PU_DMAIN0) && (pu[k].instance <= VPU_PU_DMAIN_WIDE1)) {
					io = &ginfo->ioset[ginfo->io_cnt];
					io->flags = 0;
					io_index = pu[k].params.dma.inout_index;
					if (io_index >= VPUL_MAX_IN_OUT_TYPES) {
						printf("io_index is invalid(%d)\n", io_index);
						return -EINVAL;
					}

					if (vertex[i].vtype == VPUL_VERTEXT_PROC) {
						if (vertex[i].proc.io.inout_types[io_index].is_dynamic) {
							roi_index = vertex[i].proc.io.inout_types[io_index].roi_index;
							if (roi_index >= VPUL_MAX_DYN_MAP_ROI) {
								printf("roi_index is invalid(%d)\n", roi_index);
								return -EINVAL;
							}

							io->map_index[0] = vertex[i].proc.io.dynamic_map_roi[roi_index].memmap_idx;
						} else {
							roi_index = vertex[i].proc.io.inout_types[io_index].roi_index;
							if (roi_index >= VPUL_MAX_FIX_MAP_ROI) {
								printf("roi_index is invalid(%d)\n", roi_index);
								return -EINVAL;
							}

							io->map_index[0] = vertex[i].proc.io.fixed_map_roi[roi_index].memmap_idx;
						}
					} else {
						struct vpul_3dnn_process_base *process3_base, *process3;

						process3_base = (struct vpul_3dnn_process_base *)((char *)task + task->process_bases_3dnn_vec_ofs);
						process3 = &process3_base[vertex[i].proc3dnn.base_3dnn_ind];

						io->map_index[0] = process3->layers[0].inout_3dnn[io_index].mem_descr_index;
					}

					if (task->memmap_desc[io->map_index[0]].mtype != VPUL_MEM_EXTERNAL)
						continue;

					io->direction = VS4L_DIRECTION_IN;
					io->buffer_cnt = 1;
					io->pu_id = pu[k].instance;
					io->chain_id = subchain[j].id;
					io->pu_index = ((unsigned int)&pu[k] - pu_base) / sizeof(struct vpul_pu);
					io->chain_index = ((unsigned int)&subchain[j] - chain_base) / sizeof(struct vpul_subchain);
					io->map_desc = task->memmap_desc;
					io->ext_desc = task->external_mem_addr;
					io->target =  subchain[j].id << VS4L_TARGET_SC_SHIFT | io->pu_id;
					io->width = io->map_desc[io->map_index[0]].image_sizes.width;
					io->height = io->map_desc[io->map_index[0]].image_sizes.height;
					io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
					if (io->pixel_bytes == 1) {
						io->format = VS4L_DF_IMAGE_U8;
					} else if (io->pixel_bytes == 2) {
						io->format = VS4L_DF_IMAGE_U16;
					} else if (io->pixel_bytes == 3) {
						io->format = VS4L_DF_IMAGE_RGB;
					} else {
						io->format = VS4L_DF_IMAGE_U32;
					}

					list_add(&io->list, &ginfo->in_list);
					ginfo->in_cnt++;
					ginfo->io_cnt++;

					list_add(&io->chain_list, &ginfo->chainset[io->chain_index].in_list);
					ginfo->chainset[io->chain_index].in_cnt++;
				}
			}
		}
	}

	for (i = 0; i < n_vertex; ++i) {
		n_subchain = vertex[i].num_of_subchains;
		subchain = (struct vpul_subchain *)(ptr + vertex[i].sc_ofs);
		for (j = 0; j < n_subchain; ++j) {
			n_pu = subchain[j].num_of_pus;
			pu = (struct vpul_pu *)(ptr + subchain[j].pus_ofs);
			for (k = 0; k < n_pu; ++k) {
				if ((pu[k].instance >= VPU_PU_DMAOT0) && (pu[k].instance <= VPU_PU_DMAOT_WIDE1)) {
					io = &ginfo->ioset[ginfo->io_cnt];
					io->flags = 0;
					io_index = pu[k].params.dma.inout_index;
					if (io_index >= VPUL_MAX_IN_OUT_TYPES) {
						printf("io_index is invalid(%d)\n", io_index);
						return -EINVAL;
					}

					if (vertex[i].vtype == VPUL_VERTEXT_PROC) {
						if (vertex[i].proc.io.inout_types[io_index].is_dynamic) {
							roi_index = vertex[i].proc.io.inout_types[io_index].roi_index;
							if (roi_index >= VPUL_MAX_DYN_MAP_ROI) {
								printf("roi_index is invalid(%d)\n", roi_index);
								return -EINVAL;
							}

							io->map_index[0] = vertex[i].proc.io.dynamic_map_roi[roi_index].memmap_idx;
						} else {
							roi_index = vertex[i].proc.io.inout_types[io_index].roi_index;
							if (roi_index >= VPUL_MAX_FIX_MAP_ROI) {
								printf("roi_index is invalid(%d)\n", roi_index);
								return -EINVAL;
							}

							io->map_index[0] = vertex[i].proc.io.fixed_map_roi[roi_index].memmap_idx;
						}
					} else {
						struct vpul_3dnn_process_base *process3_base, *process3;

						process3_base = (struct vpul_3dnn_process_base *)((char *)task + task->process_bases_3dnn_vec_ofs);
						process3 = &process3_base[vertex[i].proc3dnn.base_3dnn_ind];

						io->map_index[0] = process3->layers[0].inout_3dnn[io_index].mem_descr_index;
					}

					if (task->memmap_desc[io->map_index[0]].mtype != VPUL_MEM_EXTERNAL)
						continue;

					io->direction = VS4L_DIRECTION_OT;
					io->buffer_cnt = 1;
					io->pu_id = pu[k].instance;
					io->chain_id = subchain[j].id;
					io->pu_index = ((unsigned int)&pu[k] - pu_base) / sizeof(struct vpul_pu);
					io->chain_index = ((unsigned int)&subchain[j] - chain_base) / sizeof(struct vpul_subchain);
					io->map_desc = task->memmap_desc;
					io->ext_desc = task->external_mem_addr;
					io->target =  subchain[j].id << VS4L_TARGET_SC_SHIFT | io->pu_id;
					io->width = io->map_desc[io->map_index[0]].image_sizes.width;
					io->height = io->map_desc[io->map_index[0]].image_sizes.height;
					io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
					if (io->pixel_bytes == 1) {
						io->format = VS4L_DF_IMAGE_U8;
					} else if (io->pixel_bytes == 2) {
						io->format = VS4L_DF_IMAGE_U16;
					} else if (io->pixel_bytes == 3) {
						io->format = VS4L_DF_IMAGE_RGB;
					} else {
						io->format = VS4L_DF_IMAGE_U32;
					}

					list_add(&io->list, &ginfo->ot_list);
					ginfo->ot_cnt++;
					ginfo->io_cnt++;

					list_add(&io->chain_list, &ginfo->chainset[io->chain_index].ot_list);
					ginfo->chainset[io->chain_index].ot_cnt++;
				} else if ((pu[k].instance >= VPU_PU_ROIS0) && (pu[k].instance <= VPU_PU_ROIS1)) {
					io = &ginfo->ioset[ginfo->io_cnt];

					io->direction = VS4L_DIRECTION_OT;
					io->buffer_cnt = 1;
					io->pu_id = pu[k].instance;
					io->chain_id = subchain[j].id;
					io->pu_index = ((unsigned int)&pu[k] - pu_base) / sizeof(struct vpul_pu);
					io->chain_index = ((unsigned int)&subchain[j] - chain_base) / sizeof(struct vpul_subchain);
					io->map_desc = NULL;
					io->ext_desc = NULL;
					io->target =  subchain[j].id << VS4L_TARGET_SC_SHIFT | io->pu_id;
					io->width = 1;
					io->height = 6;
					io->pixel_bytes = 4;
					io->format = VS4L_DF_IMAGE_U32;

					list_add(&io->list, &ginfo->ot_list);
					ginfo->ot_cnt++;
					ginfo->io_cnt++;
				}
			}
		}
	}

	for (i = 0; i < (ginfo->chain_cnt - 1); ++i) {
		curr = &ginfo->chainset[i];
		next = &ginfo->chainset[i + 1];

		list_for_each_entry_safe(ot, t1, &curr->ot_list, chain_list) {
			if (!ot->map_desc) {
				printf("%d pu map_desc is NULL\n", ot->pu_id);
				continue;
			}

			if (ot->map_desc[ot->map_index[0]].index >= task->n_external_mem_addresses) {
				printf("in ext index is invalid(%d, %d)\n", ot->map_desc[ot->map_index[0]].index, task->n_external_mem_addresses);
				continue;
			}

			if (ot->ext_desc[ot->map_desc[ot->map_index[0]].index] == 0)
				continue;

			list_for_each_entry_safe(in, t2, &next->in_list, chain_list) {
				if (!in->map_desc) {
					printf("%d pu map_desc is NULL\n", in->pu_id);
					continue;
				}

				if (in->map_desc[in->map_index[0]].index >= task->n_external_mem_addresses) {
					printf("ot ext index is invalid(%d, %d)\n", in->map_desc[in->map_index[0]].index, task->n_external_mem_addresses);
					continue;
				}

				if (in->ext_desc[in->map_desc[in->map_index[0]].index] ==
					ot->ext_desc[ot->map_desc[ot->map_index[0]].index]) {
					list_del(&in->list);
					ginfo->in_cnt--;
					list_del(&ot->list);
					ginfo->ot_cnt--;
					list_add_tail(&in->list, &ginfo->im_list);
					ginfo->im_cnt++;
					list_add_tail(&ot->list, &ginfo->im_list);
					ginfo->im_cnt++;
				}
			}
		}
	}

	list_for_each_entry_safe(in1, t1, &ginfo->in_list, list) {
		if (in1->ext_desc[in1->map_desc[in1->map_index[0]].index] == 0)
			continue;

		in2 = list_next_entry(in1, list);
		list_for_each_entry_safe_from(in2, t2, &ginfo->in_list, list) {
			if (in1->ext_desc[in1->map_desc[in1->map_index[0]].index] ==
					in2->ext_desc[in2->map_desc[in2->map_index[0]].index]) {
					in2->flags = 1;
					printf("checking~~ %d %d\n", in2->chain_id, in2->pu_id);
			}
		}
	}

	list_for_each_entry_safe(in, t1, &ginfo->in_list, list) {
		if (in->flags) {
			list_del(&in->list);
			ginfo->in_cnt--;
		}
	}

	TEST_LOG("in list\n");
	list_for_each_entry_safe(in, t1, &ginfo->in_list, list) {
		TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, %d, 0x%08X)\n", in->pu_id, in->target,
			in->chain_index, in->pu_index, in->chain_id,
			in->width, in->height, in->pixel_bytes,
			in->map_desc[in->map_index[0]].index,
			in->ext_desc[in->map_desc[in->map_index[0]].index]);
	}

	TEST_LOG("ot list\n");
	list_for_each_entry_safe(ot, t1, &ginfo->ot_list, list) {
		if (ot->map_desc) {
			TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, %d, 0x%08X)\n", ot->pu_id, ot->target,
				ot->chain_index, ot->pu_index, ot->chain_id,
				ot->width, ot->height, ot->pixel_bytes,
				ot->map_desc[ot->map_index[0]].index,
				ot->ext_desc[ot->map_desc[ot->map_index[0]].index]);
		} else {
			TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, F, 0xFFFFFFFF)\n", ot->pu_id, ot->target,
				ot->chain_index, ot->pu_index, ot->chain_id,
				ot->width, ot->height, ot->pixel_bytes);
		}
	}

	TEST_LOG("im list\n");
	list_for_each_entry_safe(im, t1, &ginfo->im_list, list) {
		TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, %d, 0x%08X)\n", im->pu_id, im->target,
			im->chain_index, im->pu_index, im->chain_id,
			im->width, im->height, im->pixel_bytes,
			im->map_desc[im->map_index[0]].index,
			im->ext_desc[im->map_desc[im->map_index[0]].index]);
	}

	/* trash goes to waste basket */
	for (i = 0; i < VPUL_MAX_TASK_EXTERNAL_RAMS; ++i)
		task->external_mem_addr[i] = 0;

	/* printf("%s()\n", __func__); */
	return 0;
}

int client_s_ginfo_saitfr1(struct client *client, struct vpul_task *task)
{
	int i;
	struct client_graph_info *ginfo;
	struct client_graph_io *io, *in, *ot, *im, *t1;

	ginfo = &client->ginfo;
	INIT_LIST_HEAD(&ginfo->in_list);
	INIT_LIST_HEAD(&ginfo->ot_list);
	INIT_LIST_HEAD(&ginfo->im_list);
	ginfo->in_cnt = 0;
	ginfo->ot_cnt = 0;
	ginfo->im_cnt = 0;
	ginfo->io_cnt = 0;
	ginfo->chain_cnt = 0;
	ginfo->task = task;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 1;
	io->chain_id = 4660;
	io->pu_index = 0;
	io->chain_index = 0;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 0;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 4;
	io->chain_id = 4661;
	io->pu_index = 2;
	io->chain_index = 1;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 2;
	io->map_index[0] = 1;
	io->map_index[1] = 6;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 5;
	io->chain_id = 4661;
	io->pu_index = 3;
	io->chain_index = 1;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 2;
	io->map_index[0] = 2;
	io->map_index[1] = 7;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 3;
	io->chain_id = 4661;
	io->pu_index = 4;
	io->chain_index = 1;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 2;
	io->map_index[0] = 3;
	io->map_index[1] = 8;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 2;
	io->chain_id = 4661;
	io->pu_index = 5;
	io->chain_index = 1;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 2;
	io->map_index[0] = 4;
	io->map_index[1] = 9;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 4;
	io->chain_id = 4666;
	io->pu_index = 25;
	io->chain_index = 5;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 11;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 5;
	io->chain_id = 4666;
	io->pu_index = 26;
	io->chain_index = 5;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 12;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 3;
	io->chain_id = 4666;
	io->pu_index = 27;
	io->chain_index = 5;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 13;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 2;
	io->chain_id = 4666;
	io->pu_index = 28;
	io->chain_index = 5;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 14;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 4;
	io->chain_id = 4670;
	io->pu_index = 41;
	io->chain_index = 8;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 16;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 5;
	io->chain_id = 4670;
	io->pu_index = 42;
	io->chain_index = 8;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 17;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 3;
	io->chain_id = 4670;
	io->pu_index = 43;
	io->chain_index = 8;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 18;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 2;
	io->chain_id = 4670;
	io->pu_index = 44;
	io->chain_index = 8;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 19;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->in_list);
	ginfo->in_cnt++;
	ginfo->io_cnt++;

	/*
	 * O
	 * U
	 * T
	 */

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_OT;
	io->pu_id = 9;
	io->chain_id = 4670;
	io->pu_index = 46;
	io->chain_index = 8;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 20;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->ot_list);
	ginfo->ot_cnt++;
	ginfo->io_cnt++;

	/*
	 * I
	 * M
	 */

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 1;
	io->chain_id = 4663;
	io->pu_index = 10;
	io->chain_index = 2;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 5;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->im_list);
	ginfo->im_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_OT;
	io->pu_id = 1;
	io->chain_id = 4663;
	io->pu_index = 10;
	io->chain_index = 2;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 5;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	list_add_tail(&io->list, &ginfo->im_list);
	ginfo->im_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 1;
	io->chain_id = 4663;
	io->pu_index = 10;
	io->chain_index = 2;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 10;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->im_list);
	ginfo->im_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_OT;
	io->pu_id = 1;
	io->chain_id = 4663;
	io->pu_index = 10;
	io->chain_index = 2;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 10;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	list_add_tail(&io->list, &ginfo->im_list);
	ginfo->im_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_IN;
	io->pu_id = 1;
	io->chain_id = 4668;
	io->pu_index = 33;
	io->chain_index = 6;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 15;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	if (io->pixel_bytes == 1) {
		io->format = VS4L_DF_IMAGE_U8;
	} else if (io->pixel_bytes == 2) {
		io->format = VS4L_DF_IMAGE_U16;
	} else if (io->pixel_bytes == 3) {
		io->format = VS4L_DF_IMAGE_RGB;
	} else {
		io->format = VS4L_DF_IMAGE_U32;
	}
	list_add_tail(&io->list, &ginfo->im_list);
	ginfo->im_cnt++;
	ginfo->io_cnt++;

	io = &ginfo->ioset[ginfo->io_cnt];
	io->direction = VS4L_DIRECTION_OT;
	io->pu_id = 1;
	io->chain_id = 4663;
	io->pu_index = 10;
	io->chain_index = 2;
	io->map_desc = task->memmap_desc;
	io->ext_desc = task->external_mem_addr;
	io->target =  io->chain_id << VS4L_TARGET_SC_SHIFT | io->pu_id;
	io->buffer_cnt = 1;
	io->map_index[0] = 15;
	io->width = io->map_desc[io->map_index[0]].image_sizes.width;
	io->height = io->map_desc[io->map_index[0]].image_sizes.height;
	io->pixel_bytes = io->map_desc[io->map_index[0]].image_sizes.pixel_bytes;
	list_add_tail(&io->list, &ginfo->im_list);
	ginfo->im_cnt++;
	ginfo->io_cnt++;

	TEST_LOG("in list\n");
	list_for_each_entry_safe(in, t1, &ginfo->in_list, list) {
		TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, %d, 0x%08X)\n", in->pu_id, in->target,
			in->chain_index, in->pu_index, in->chain_id,
			in->width, in->height, in->pixel_bytes,
			in->map_desc[in->map_index[0]].index,
			in->ext_desc[in->map_desc[in->map_index[0]].index]);
	}

	TEST_LOG("ot list\n");
	list_for_each_entry_safe(ot, t1, &ginfo->ot_list, list) {
		if (ot->map_desc) {
			TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, %d, 0x%08X)\n", ot->pu_id, ot->target,
				ot->chain_index, ot->pu_index, ot->chain_id,
				ot->width, ot->height, ot->pixel_bytes,
				ot->map_desc[ot->map_index[0]].index,
				ot->ext_desc[ot->map_desc[ot->map_index[0]].index]);
		} else {
			TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, F, 0xFFFFFFFF)\n", ot->pu_id, ot->target,
				ot->chain_index, ot->pu_index, ot->chain_id,
				ot->width, ot->height, ot->pixel_bytes);
		}
	}

	TEST_LOG("im list\n");
	list_for_each_entry_safe(im, t1, &ginfo->im_list, list) {
		TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, %d, 0x%08X)\n", im->pu_id, im->target,
			im->chain_index, im->pu_index, im->chain_id,
			im->width, im->height, im->pixel_bytes,
			im->map_desc[im->map_index[0]].index,
			im->ext_desc[im->map_desc[im->map_index[0]].index]);
	}

	/* trash goes to waste basket */
	for (i = 0; i < VPUL_MAX_TASK_EXTERNAL_RAMS; ++i)
		task->external_mem_addr[i] = 0;

	/* printf("%s()\n", __func__); */
	return 0;
}

int client_s_ginfo_saitfr2(struct client *client, struct vpul_task *task)
{
#if 0
	TEST_LOG("in list\n");
	list_for_each_entry_safe(in, t1, &ginfo->in_list, list) {
		TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, %d, 0x%08X)\n", in->pu_id, in->target,
			in->chain_index, in->pu_index, in->chain_id,
			in->width, in->height, in->pixel_bytes,
			in->map_desc[in->map_index[0]].index,
			in->ext_desc[in->map_desc[in->map_index[0]].index]);
	}

	TEST_LOG("ot list\n");
	list_for_each_entry_safe(ot, t1, &ginfo->ot_list, list) {
		if (ot->map_desc) {
			TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, %d, 0x%08X)\n", ot->pu_id, ot->target,
				ot->chain_index, ot->pu_index, ot->chain_id,
				ot->width, ot->height, ot->pixel_bytes,
				ot->map_desc[ot->map_index[0]].index,
				ot->ext_desc[ot->map_desc[ot->map_index[0]].index]);
		} else {
			TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, F, 0xFFFFFFFF)\n", ot->pu_id, ot->target,
				ot->chain_index, ot->pu_index, ot->chain_id,
				ot->width, ot->height, ot->pixel_bytes);
		}
	}

	TEST_LOG("im list\n");
	list_for_each_entry_safe(im, t1, &ginfo->im_list, list) {
		TEST_LOG("%02d(%08X, %02d, %02d, %04d, %03dx%03d@%d, %d, 0x%08X)\n", im->pu_id, im->target,
			im->chain_index, im->pu_index, im->chain_id,
			im->width, im->height, im->pixel_bytes,
			im->map_desc[im->map_index[0]].index,
			im->ext_desc[im->map_desc[im->map_index[0]].index]);
	}

	/* trash goes to waste basket */
	for (i = 0; i < VPUL_MAX_TASK_EXTERNAL_RAMS; ++i)
		task->external_mem_addr[i] = 0;
#endif
	/* printf("%s()\n", __func__); */
	return 0;
}

int client_s_format(struct client *client)
{
	int i;
	struct client_graph_info *ginfo;
	struct client_graph_io *in, *ot, *temp;
	struct vs4l_format_list *flist;
	struct vs4l_format *formats;

	ginfo = &client->ginfo;

	flist = &client->incfg.flist;
	flist->direction = VS4L_DIRECTION_IN;
	flist->count = ginfo->in_cnt;
	flist->formats = formats = malloc(sizeof(struct vs4l_format) * flist->count);

	i = 0;
	list_for_each_entry_safe(in, temp, &ginfo->in_list, list) {
		formats[i].target = in->target;
		formats[i].width = in->width;
		formats[i].height = in->height;
		formats[i].plane = 0;
		formats[i].format = in->format;
		client->incfg.blist_cnt[i] = in->buffer_cnt;
		++i;
	}

	flist = &client->otcfg.flist;
	flist->direction = VS4L_DIRECTION_OT;
	flist->count = ginfo->ot_cnt;
	flist->formats = formats = malloc(sizeof(struct vs4l_format) * flist->count);

	i = 0;
	list_for_each_entry_safe(ot, temp, &ginfo->ot_list, list) {
		formats[i].target = ot->target;
		formats[i].width = ot->width;
		formats[i].height = ot->height;
		formats[i].plane = 0;
		formats[i].format = ot->format;
		client->otcfg.blist_cnt[i] = ot->buffer_cnt;
		++i;
	}

	/* printf("%s()\n", __func__); */
	return 0;
}

int client_s_buffer(struct client *client)
{

	int i, j, k;
	struct vs4l_format_list *flist;
	struct vs4l_format *formats;
	struct vs4l_container_list *clist;
	struct vs4l_container *container;
	struct vs4l_buffer *buffer;
	int size;

	flist = &client->incfg.flist;
	formats = flist->formats;

	client->incfg.clist = clist = malloc(sizeof(struct vs4l_container_list) * client->buffer_cnt);
	for (i = 0; i < client->buffer_cnt; ++i) {
		clist[i].direction = VS4L_DIRECTION_IN;
		clist[i].id = 0;
		clist[i].index = i;
		clist[i].flags = 0;
		clist[i].count = flist->count;
		clist[i].containers = container =malloc(sizeof(struct vs4l_container) * clist[i].count);

		for (j = 0; j < clist[i].count; ++j) {
			container[j].type = VS4L_BUFFER_LIST;
			container[j].target = formats[j].target;
			container[j].count = client->incfg.blist_cnt[j];
			container[j].memory = VS4L_MEMORY_DMABUF;
			container[j].buffers = buffer = malloc(sizeof(struct vs4l_buffer) * container[j].count);

			for (k = 0; k < container[j].count; ++k) {
				size = util_g_size_from(&formats[j]);
				buffer[k].roi.x = 0;
				buffer[k].roi.y = 0;
				buffer[k].roi.w = formats[j].width;
				buffer[k].roi.h = formats[j].height;
#ifdef GRAPH_ION_CACHE
				buffer[k].m.fd = ion_alloc(client->iclient, size, 0, ION_HEAP_SYSTEM_MASK,
					ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC);
#else
				buffer[k].m.fd = ion_alloc(client->iclient, size, 0, ION_HEAP_SYSTEM_MASK, 0);
#endif
				if (buffer[k].m.fd < 0)
					printf("failed to alloc ion buffer\n");
				buffer[k].reserved = (unsigned long)ion_map(buffer[k].m.fd, size, 0);
				if (buffer[k].reserved == 0)
					printf("failed to map ion buffer\n");
			}
		}
	}

	flist = &client->otcfg.flist;
	formats = flist->formats;

	client->otcfg.clist = clist = malloc(sizeof(struct vs4l_container_list) * client->buffer_cnt);
	for (i = 0; i < client->buffer_cnt; ++i) {
		clist[i].direction = VS4L_DIRECTION_OT;
		clist[i].id = 0;
		clist[i].index = i;
		clist[i].flags = 0;
		clist[i].count = flist->count;
		clist[i].containers = container = malloc(sizeof(struct vs4l_container) * clist[i].count);

		for (j = 0; j < clist[i].count; ++j) {
			container[j].type = VS4L_BUFFER_LIST;
			container[j].target = formats[j].target;
			container[j].count = client->otcfg.blist_cnt[j];
			container[j].memory = VS4L_MEMORY_DMABUF;
			container[j].buffers = buffer = malloc(sizeof(struct vs4l_buffer) * container[j].count);

			for (k = 0; k < container[j].count; ++k) {
				size = util_g_size_from(&formats[j]);
				buffer[k].roi.x = 0;
				buffer[k].roi.y = 0;
				buffer[k].roi.w = formats[j].width;
				buffer[k].roi.h = formats[j].height;
#ifdef GRAPH_ION_CACHE
				buffer[k].m.fd = ion_alloc(client->iclient, size, 0, ION_HEAP_SYSTEM_MASK,
					ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC);
#else
				buffer[k].m.fd = ion_alloc(client->iclient, size, 0, ION_HEAP_SYSTEM_MASK, 0);
#endif
				if (buffer[k].m.fd < 0)
					printf("failed to alloc ion buffer\n");
				buffer[k].reserved = (unsigned long)ion_map(buffer[k].m.fd, size, 0);
				if (buffer[k].reserved == 0)
					printf("failed to map ion buffer\n");
			}
		}
	}

	/* printf("%s()\n", __func__); */
	return 0;
}

int client_p_buffer(struct client *client)
{
	int ret = 0;
	struct vs4l_format_list *flist;
	struct vs4l_format *formats;
	struct client_graph_io *im, *temp;
	int i, j, k;
	int size;

	flist = &client->incfg.flist;
	formats = flist->formats;

	for (i = 0; i < client->buffer_cnt; ++i) {
		for (j = 0; j < client->incfg.clist[i].count; ++j) {
			for (k = 0; k < client->incfg.clist[i].containers[j].count; ++k) {
				size = util_g_size_from(&formats[j]);
				ret = ion_unmap((void *)client->incfg.clist[i].containers[j].buffers[k].reserved, size);
				if (ret)
					printf("ionumap is fail(%d)\n", ret);
				ion_free(client->incfg.clist[i].containers[j].buffers[k].m.fd);
			}
			free(client->incfg.clist[i].containers[j].buffers);
		}
		free(client->incfg.clist[i].containers);
	}
	free(client->incfg.clist);

	flist = &client->otcfg.flist;
	formats = flist->formats;

	for (i = 0; i < client->buffer_cnt; ++i) {
		for (j = 0; j < client->otcfg.clist[i].count; ++j) {
			for (k = 0; k < client->otcfg.clist[i].containers[j].count; ++k) {
				size = util_g_size_from(&formats[j]);
				ret = ion_unmap((void *)client->otcfg.clist[i].containers[j].buffers[k].reserved, size);
				if (ret)
					printf("ionumap is fail(%d)\n", ret);
				ion_free(client->otcfg.clist[i].containers[j].buffers[k].m.fd);
			}
			free(client->otcfg.clist[i].containers[j].buffers);
		}
		free(client->otcfg.clist[i].containers);
	}
	free(client->otcfg.clist);

	list_for_each_entry_safe(im, temp, &client->ginfo.im_list, list) {
		if (im->direction != VS4L_DIRECTION_IN)
			continue;

		size = im->width * im->height * im->pixel_bytes;
		ret = ion_unmap((void *)im->va, size);
		if (ret)
			printf("ionumap is fail(%d)\n", ret);
		ion_free(im->fd);
	}

	free(client->incfg.flist.formats);
	free(client->otcfg.flist.formats);
	free((void *)client->graph.addr);

	return 0;
}

int client_s_intermediate(struct client *client)
{
	int fd, size;
	unsigned long va;
	struct client_graph_info *ginfo;
	struct client_graph_io *im, *in, *ot, *temp;
	struct vs4l_container_list *clist;
	struct vpul_task *task;

	ginfo = &client->ginfo;
	task = client->ginfo.task;

	clist = client->incfg.clist;
	list_for_each_entry_safe(in, temp, &ginfo->in_list, list) {
		if (!in->map_desc)
			continue;

		if (in->map_desc[in->map_index[0]].mtype == VPUL_MEM_EXTERNAL) {
			in->ext_desc[in->map_desc[in->map_index[0]].index] = 0;
		}
	}

	clist = client->otcfg.clist;
	list_for_each_entry_safe(ot, temp, &ginfo->ot_list, list) {
		if (!ot->map_desc)
			continue;

		if (ot->map_desc[ot->map_index[0]].mtype == VPUL_MEM_EXTERNAL) {
			ot->ext_desc[ot->map_desc[ot->map_index[0]].index] = 0;
		}
	}

	in = ot = NULL;
	list_for_each_entry_safe(im, temp, &ginfo->im_list, list) {
		if (im->direction == VS4L_DIRECTION_IN) {
			in = im;
			continue;
		} else {
			ot = im;
		}

		if (!in) {
			printf("in is NULL\n");
			continue;
		}

		if (!ot) {
			printf("ot is NULL\n");
			continue;
		}

		if (in->map_index[0] >= ginfo->task->n_memmap_desc) {
			printf("map_index is invalid (%d > %d)\n", in->map_index[0], ginfo->task->n_memmap_desc);
			continue;
		}

		if (in->map_desc[in->map_index[0]].mtype != VPUL_MEM_EXTERNAL) {
			printf("mtype is invalid (%d)\n", in->map_desc[in->map_index[0]].mtype);
			continue;
		}

		size = in->width * in->height * in->pixel_bytes;
		fd = ion_alloc(client->iclient, size, 0, ION_HEAP_SYSTEM_MASK, 0);
		if (fd < 0)
			printf("failed to alloc ion buffer\n");
		in->fd = fd;
		ot->fd = fd;
		in->ext_desc[in->map_desc[in->map_index[0]].index] = fd;
		ot->ext_desc[ot->map_desc[ot->map_index[0]].index] = fd;

		va = (unsigned long)ion_map(fd, size, 0);
		if (va == 0)
			printf("failed to map ion buffer\n");
		in->va = va;
		ot->va = va;

		in = ot = NULL;
	}

	/* printf("%s()\n", __func__); */
	return 0;
}
