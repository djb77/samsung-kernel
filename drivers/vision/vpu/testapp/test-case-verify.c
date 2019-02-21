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

#include "test-case.h"
#include "test-client.h"
#include "test-util.h"
#include <vs4l.h>
#include <vpu-control.h>

#include "test-binary-info.h"

#include "test-binary0.h"
#include "test-binary1.h"
#include "test-binary2.h"
#include "test-binary3.h"

#include "test-binary-absdiff.h"
#include "test-binary-acc.h"
#include "test-binary-accsquared.h"
#include "test-binary-accweighted.h"
#include "test-binary-add.h"
#include "test-binary-sub.h"
#include "test-binary-and.h"
#include "test-binary-xor.h"
#include "test-binary-or.h"
#include "test-binary-not.h"
#include "test-binary-boxfilter.h"
#include "test-binary-canny.h"
#include "test-binary-chcombine.h"
#include "test-binary-chextract.h"
#include "test-binary-colorconvert.h"
#include "test-binary-convertbitdepth.h"
#include "test-binary-convolution.h"
#include "test-binary-dilate.h"
#include "test-binary-equalize.h"
#include "test-binary-erode.h"
#include "test-binary-fastcorner.h"
#include "test-binary-gaussianfilter.h"
#include "test-binary-harris.h"
#include "test-binary-histogram.h"
#include "test-binary-gaussianpyramid.h"
#include "test-binary-integral.h"
#include "test-binary-magnitude.h"
#include "test-binary-meanstddev.h"
#include "test-binary-median.h"
#include "test-binary-minmaxloc.h"
#include "test-binary-phase.h"
#include "test-binary-multiply.h"
#include "test-binary-scale.h"
#include "test-binary-sobel.h"
#include "test-binary-lut.h"
#include "test-binary-threshold.h"
#include "test-binary-halfscalegaussian.h"
#include "test-binary-saitfr0.h"
#include "test-binary-saitfr1.h"
#include "test-binary-saitfr2.h"
#include "test-binary-flamorb.h"

#include "test-ds-absdiff.h"
#include "test-ds-acc.h"
#include "test-ds-accsquared.h"
#include "test-ds-accweighted.h"
#include "test-ds-add.h"
#include "test-ds-sub.h"
#include "test-ds-and.h"
#include "test-ds-xor.h"
#include "test-ds-or.h"
#include "test-ds-not.h"
#include "test-ds-boxfilter.h"
#include "test-ds-canny.h"
#include "test-ds-chcombine.h"
#include "test-ds-chextract.h"
#include "test-ds-colorconvert.h"
#include "test-ds-convertbitdepth.h"
#include "test-ds-convolution.h"
#include "test-ds-dilate.h"
#include "test-ds-equalize.h"
#include "test-ds-erode.h"
#include "test-ds-fastcorner.h"
#include "test-ds-gaussianfilter.h"
#include "test-ds-harris.h"
#include "test-ds-histogram.h"
#include "test-ds-gaussianpyramid.h"
#include "test-ds-integral.h"
#include "test-ds-magnitude.h"
#include "test-ds-meanstddev.h"
#include "test-ds-median.h"
#include "test-ds-minmaxloc.h"
#include "test-ds-phase.h"
#include "test-ds-multiply.h"
#include "test-ds-scale.h"
#include "test-ds-sobel.h"
#include "test-ds-lut.h"
#include "test-ds-threshold.h"
#include "test-ds-halfscalegaussian.h"
#include "test-ds-saitfr0.h"
#include "test-ds-saitfr1.h"
#include "test-ds-saitfr2.h"
#include "test-ds-flamorb.h"

int client_s_ginfo_saitfr1(struct client *client, struct vpul_task *task);
int client_s_ginfo_saitfr2(struct client *client, struct vpul_task *task);

void vpu_graph_RAW(struct client *client, const short *raw, int raw_size)
{
	int ret = 0;
	struct vs4l_graph *graph;
	struct vpul_task *task;
	struct vpul_vertex *vertex;
	struct vpul_subchain *subchain;
	struct vpul_pu *pu;
	char *ptr;
	int size;

	size = raw_size + 4;
	ptr = malloc(size);
	memcpy(ptr, raw, raw_size);
	ptr[size - 4] = 'V';
	ptr[size - 3] = 'S';
	ptr[size - 2] = '4';
	ptr[size - 1] = 'L';

	task = (struct vpul_task *)ptr;
	vertex = (struct vpul_vertex *)(ptr + task->vertices_vec_ofs);
	subchain = (struct vpul_subchain *)(ptr + task->sc_vec_ofs);
	pu = (struct vpul_pu *)(ptr + task->pus_vec_ofs);

	/* print_td_info(task); */

	client->chain_cnt = task->t_num_of_subchains;
	client->pu_cnt = task->t_num_of_pus;
	client->td_binary = (short *)task;
	client->vertex_array = vertex;
	client->chain_array = subchain;
	client->pu_array = pu;
	graph = &client->graph;
	graph->addr = (unsigned long)ptr;
	graph->size = size;

	switch (client->id) {
	case 43:
		ret = client_s_ginfo_saitfr1(client, task);
		break;
/*	case 44:
		ret = client_s_ginfo_saitfr2(client, task);
		break;*/
	default:
		ret = client_s_ginfo(client, task);
		break;
	}

	if (ret)
		printf("client_s_ginfo is fail(%d)\n", ret);

	ret = client_s_format(client);
	if (ret)
		printf("client_s_format is fail(%d)\n", ret);

	ret = client_s_buffer(client);
	if (ret)
		printf("client_s_buffer is fail(%d)\n", ret);

	ret = client_s_intermediate(client);
	if (ret)
		printf("client_s_intermediate is fail(%d)\n", ret);
}

int test_V_Common(struct client *user,
	struct vpu_binary_io_info *in_info,
	struct vpu_binary_io_info *ot_info,
	short *raw_input,
	short *raw_otput)
{
	int ret, errcnt, size;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist, *otclist;
	struct vs4l_format_list *inflist, *otflist;
	struct vs4l_buffer *buffer;
	struct vs4l_format *format;
	struct client_graph_io *im, *t;
	unsigned int i, j, index, addr;

	inclist = user->incfg.clist;
	otclist = user->otcfg.clist;
	inflist = &user->incfg.flist;
	otflist = &user->otcfg.flist;

	addr = (unsigned int)raw_input;
	for (i = 0; i < inflist->count; ++i) {
		in_info[i].target = user->chain_array[in_info[i].chain_idx].id << VS4L_TARGET_SC_SHIFT | user->pu_array[in_info[i].pu_idx].instance;
		format = find_target_format(inflist, in_info[i].target);
		if (!format) {
			printf("in format is NOT found(%X)\n", in_info[i].target);
			break;
		}

		in_info[i].blist_cnt = user->incfg.blist_cnt[i];
		for (j = 0; j < in_info[i].blist_cnt; ++j) {
			in_info[i].size[j] = util_g_size_from(format);
			in_info[i].addr[j] = addr;
			addr += in_info[i].size[j];
		}
	}

	addr = (unsigned int)raw_otput;
	for (i = 0; i < otflist->count; ++i) {
		ot_info[i].target = user->chain_array[ot_info[i].chain_idx].id << VS4L_TARGET_SC_SHIFT | user->pu_array[ot_info[i].pu_idx].instance;
		format = find_target_format(otflist, ot_info[i].target);
		if (!format) {
			printf("ot format is NOT found(%X)\n", ot_info[i].target);
			break;
		}

		ot_info[i].blist_cnt = user->otcfg.blist_cnt[i];
		for (j = 0; j < ot_info[i].blist_cnt; ++j) {
			ot_info[i].size[j] = util_g_size_from(format);
			ot_info[i].addr[j] = addr;
			addr += ot_info[i].size[j];
		}
	}

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_open(user);
	vpu_s_graph(user, &user->graph);
	vpu_s_format(user, inflist);
	vpu_s_format(user, otflist);
	vpu_stream_on(user);

	errcnt = 0;
	while (user->str_loop < user->end_loop) {
		index = user->str_loop % user->buffer_cnt;
		inclist[index].id = user->str_loop;
		otclist[index].id = user->str_loop;
		inclist[index].flags = 0;
		otclist[index].flags = 0;

		for (i = 0; i < inflist->count; ++i) {
			for (j = 0; j < in_info[i].blist_cnt; ++j) {
				buffer = find_target_buffer(inclist, in_info[i].target, index);
				if (!buffer) {
					printf("buffer is NOT found(%X)\n", in_info[i].target);
					break;
				}

				memcpy((void *)buffer[j].reserved, (void *)in_info[i].addr[j], in_info[i].size[j]);
			}
		}

		for (i = 0; i < otflist->count; ++i) {
			for (j = 0; j < ot_info[i].blist_cnt; ++j) {
				buffer = find_target_buffer(otclist, ot_info[i].target, index);
				if (!buffer) {
					printf("buffer is NOT found(%X)\n", ot_info[i].target);
					break;
				}

				memset((void *)buffer[j].reserved, 0x5B, ot_info[i].size[j]);
			}
		}

		vpu_qbuf(user, &inclist[index]);
		vpu_qbuf(user, &otclist[index]);
		vpu_dqbuf(user, &inresult);
		vpu_dqbuf(user, &otresult);

		for (i = 0; i < otflist->count; ++i) {
			for (j = 0; j < ot_info[i].blist_cnt; ++j) {
				buffer = find_target_buffer(otclist, ot_info[i].target, index);
				if (!buffer) {
					printf("buffer is NOT found(%X)\n", ot_info[i].target);
					break;
				}

				ret = memcmp((char *)buffer[j].reserved, (char *)ot_info[i].addr[j], ot_info[i].size[j]);
				if (ret) {
					printf("memcmp is fail(ret : %d, size : %d, target : %X)\n",
						ret, ot_info[i].size[j], ot_info[i].target);
					util_memdump16((void *)buffer[j].reserved, (void *)((char *)buffer[j].reserved + ot_info[i].size[j]));
					errcnt++;
				}
			}
		}

		if (errcnt) {
			struct vs4l_ctrl ctrl;

			list_for_each_entry_safe(im, t, &user->ginfo.im_list, list) {
				printf("%d(%08X, %d, %d, %04d, %dx%d@%d, %d, 0x%08X)\n", im->pu_id, im->target,
					im->chain_index, im->pu_index, im->chain_id,
					im->width, im->height, im->pixel_bytes,
					im->map_desc[im->map_index[0]].index,
					im->ext_desc[im->map_desc[im->map_index[0]].index]);

				size = im->width * im->height * im->pixel_bytes;
				//util_memdump16((void *)im->va, (void *)((char *)im->va + size));
			}

			ctrl.ctrl = VPU_CTRL_DUMP;
			ctrl.value = 1;
			vpu_s_ctrl(user, &ctrl);
			break;
		}

		user->str_loop++;
	}

	vpu_stream_off(user);
	vpu_close(user);
	client_p_buffer(user);

	return errcnt;
}

void * test_V_0(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_0_DS;
	struct vpu_binary_io_info in_info[] = TASK_0_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_0_OT_INFO;
	short raw_input[] = TASK_0_IN;
	short raw_otput[] = TASK_0_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_1(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_1_DS;
	struct vpu_binary_io_info in_info[] = TASK_1_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_1_OT_INFO;
	short raw_input[] = TASK_1_IN;
	short raw_otput[] = TASK_1_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_2(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_2_DS;
	struct vpu_binary_io_info in_info[] = TASK_2_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_2_OT_INFO;
	short raw_input[] = TASK_2_IN;
	short raw_otput[] = TASK_2_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_3(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_3_DS;
	struct vpu_binary_io_info in_info[] = TASK_3_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_3_OT_INFO;
	short raw_input[] = TASK_3_IN;
	short raw_otput[] = TASK_3_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_ABSDIFF(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_ABSDIFF_DS;
	struct vpu_binary_io_info in_info[] = TASK_ABSDIFF_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_ABSDIFF_OT_INFO;
	short raw_input[] = TASK_ABSDIFF_IN;
	short raw_otput[] = TASK_ABSDIFF_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_ACC(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_ACC_DS;
	struct vpu_binary_io_info in_info[] = TASK_ACC_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_ACC_OT_INFO;
	short raw_input[] = TASK_ACC_IN;
	short raw_otput[] = TASK_ACC_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_ACCSQUARED(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_ACCSQUARED_DS;
	struct vpu_binary_io_info in_info[] = TASK_ACCSQUARED_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_ACCSQUARED_OT_INFO;
	short raw_input[] = TASK_ACCSQUARED_IN;
	short raw_otput[] = TASK_ACCSQUARED_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_ACCWEIGHTED(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_ACCWEIGHTED_DS;
	struct vpu_binary_io_info in_info[] = TASK_ACCWEIGHTED_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_ACCWEIGHTED_OT_INFO;
	short raw_input[] = TASK_ACCWEIGHTED_IN;
	short raw_otput[] = TASK_ACCWEIGHTED_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_ADD(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_ADD_DS;
	struct vpu_binary_io_info in_info[] = TASK_ADD_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_ADD_OT_INFO;
	short raw_input[] = TASK_ADD_IN;
	short raw_otput[] = TASK_ADD_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_SUB(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_SUB_DS;
	struct vpu_binary_io_info in_info[] = TASK_SUB_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_SUB_OT_INFO;
	short raw_input[] = TASK_SUB_IN;
	short raw_otput[] = TASK_SUB_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_AND(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_AND_DS;
	struct vpu_binary_io_info in_info[] = TASK_AND_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_AND_OT_INFO;
	short raw_input[] = TASK_AND_IN;
	short raw_otput[] = TASK_AND_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_XOR(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_XOR_DS;
	struct vpu_binary_io_info in_info[] = TASK_XOR_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_XOR_OT_INFO;
	short raw_input[] = TASK_XOR_IN;
	short raw_otput[] = TASK_XOR_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_OR(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_OR_DS;
	struct vpu_binary_io_info in_info[] = TASK_OR_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_OR_OT_INFO;
	short raw_input[] = TASK_OR_IN;
	short raw_otput[] = TASK_OR_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_NOT(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_NOT_DS;
	struct vpu_binary_io_info in_info[] = TASK_NOT_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_NOT_OT_INFO;
	short raw_input[] = TASK_NOT_IN;
	short raw_otput[] = TASK_NOT_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_BOXFILTER(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_BOXFILTER_DS;
	struct vpu_binary_io_info in_info[] = TASK_BOXFILTER_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_BOXFILTER_OT_INFO;
	short raw_input[] = TASK_BOXFILTER_IN;
	short raw_otput[] = TASK_BOXFILTER_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_CANNY(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_CANNY_DS;
	struct vpu_binary_io_info in_info[] = TASK_CANNY_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_CANNY_OT_INFO;
	short raw_input[] = TASK_CANNY_IN;
	short raw_otput[] = TASK_CANNY_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_CHCOMBINE(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_CHCOMBINE_DS;
	struct vpu_binary_io_info in_info[] = TASK_CHCOMBINE_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_CHCOMBINE_OT_INFO;
	short raw_input[] = TASK_CHCOMBINE_IN;
	short raw_otput[] = TASK_CHCOMBINE_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_CHEXTRACT(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_CHEXTRACT_DS;
	struct vpu_binary_io_info in_info[] = TASK_CHEXTRACT_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_CHEXTRACT_OT_INFO;
	short raw_input[] = TASK_CHEXTRACT_IN;
	short raw_otput[] = TASK_CHEXTRACT_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_COLORCONVERT(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_COLORCONVERT_DS;
	struct vpu_binary_io_info in_info[] = TASK_COLORCONVERT_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_COLORCONVERT_OT_INFO;
	short raw_input[] = TASK_COLORCONVERT_IN;
	short raw_otput[] = TASK_COLORCONVERT_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_CONVERTBITDEPTH(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_CONVERTBITDEPTH_DS;
	struct vpu_binary_io_info in_info[] = TASK_CONVERTBITDEPTH_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_CONVERTBITDEPTH_OT_INFO;
	short raw_input[] = TASK_CONVERTBITDEPTH_IN;
	short raw_otput[] = TASK_CONVERTBITDEPTH_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_CONVOLUTION(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_CONVOLUTION_DS;
	struct vpu_binary_io_info in_info[] = TASK_CONVOLUTION_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_CONVOLUTION_OT_INFO;
	short raw_input[] = TASK_CONVOLUTION_IN;
	short raw_otput[] = TASK_CONVOLUTION_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_DILATE(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_DILATE_DS;
	struct vpu_binary_io_info in_info[] = TASK_DILATE_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_DILATE_OT_INFO;
	short raw_input[] = TASK_DILATE_IN;
	short raw_otput[] = TASK_DILATE_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_EQUALIZE(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_EQUALIZE_DS;
	struct vpu_binary_io_info in_info[] = TASK_EQUALIZE_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_EQUALIZE_OT_INFO;
	short raw_input[] = TASK_EQUALIZE_IN;
	short raw_otput[] = TASK_EQUALIZE_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_ERODE(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_ERODE_DS;
	struct vpu_binary_io_info in_info[] = TASK_ERODE_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_ERODE_OT_INFO;
	short raw_input[] = TASK_ERODE_IN;
	short raw_otput[] = TASK_ERODE_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_FASTCORNER(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_FASTCORNER_DS;
	struct vpu_binary_io_info in_info[] = TASK_FASTCORNER_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_FASTCORNER_OT_INFO;
	short raw_input[] = TASK_FASTCORNER_IN;
	short raw_otput[] = TASK_FASTCORNER_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_GAUSSIANFILTER(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_GAUSSIANFILTER_DS;
	struct vpu_binary_io_info in_info[] = TASK_GAUSSIANFILTER_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_GAUSSIANFILTER_OT_INFO;
	short raw_input[] = TASK_GAUSSIANFILTER_IN;
	short raw_otput[] = TASK_GAUSSIANFILTER_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_HARRIS(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_HARRIS_DS;
	struct vpu_binary_io_info in_info[] = TASK_HARRIS_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_HARRIS_OT_INFO;
	short raw_input[] = TASK_HARRIS_IN;
	short raw_otput[] = TASK_HARRIS_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_HISTOGRAM(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_HISTOGRAM_DS;
	struct vpu_binary_io_info in_info[] = TASK_HISTOGRAM_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_HISTOGRAM_OT_INFO;
	short raw_input[] = TASK_HISTOGRAM_IN;
	short raw_otput[] = TASK_HISTOGRAM_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_GAUSSIANPYRAMID(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_GAUSSIANPYRAMID_DS;
	struct vpu_binary_io_info in_info[] = TASK_GAUSSIANPYRAMID_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_GAUSSIANPYRAMID_OT_INFO;
	short raw_input[] = TASK_GAUSSIANPYRAMID_IN;
	short raw_otput[] = TASK_GAUSSIANPYRAMID_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_INTEGRAL(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_INTEGRAL_DS;
	struct vpu_binary_io_info in_info[] = TASK_INTEGRAL_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_INTEGRAL_OT_INFO;
	short raw_input[] = TASK_INTEGRAL_IN;
	short raw_otput[] = TASK_INTEGRAL_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_MAGNITUDE(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_MAGNITUDE_DS;
	struct vpu_binary_io_info in_info[] = TASK_MAGNITUDE_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_MAGNITUDE_OT_INFO;
	short raw_input[] = TASK_MAGNITUDE_IN;
	short raw_otput[] = TASK_MAGNITUDE_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_MEANSTDDEV(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_MEANSTDDEV_DS;
	struct vpu_binary_io_info in_info[] = TASK_MEANSTDDEV_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_MEANSTDDEV_OT_INFO;
	short raw_input[] = TASK_MEANSTDDEV_IN;
	short raw_otput[] = TASK_MEANSTDDEV_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_MEDIAN(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_MEDIAN_DS;
	struct vpu_binary_io_info in_info[] = TASK_MEDIAN_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_MEDIAN_OT_INFO;
	short raw_input[] = TASK_MEDIAN_IN;
	short raw_otput[] = TASK_MEDIAN_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_MINMAXLOC(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_MINMAXLOC_DS;
	struct vpu_binary_io_info in_info[] = TASK_MINMAXLOC_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_MINMAXLOC_OT_INFO;
	short raw_input[] = TASK_MINMAXLOC_IN;
	short raw_otput[] = TASK_MINMAXLOC_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_PHASE(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_PHASE_DS;
	struct vpu_binary_io_info in_info[] = TASK_PHASE_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_PHASE_OT_INFO;
	short raw_input[] = TASK_PHASE_IN;
	short raw_otput[] = TASK_PHASE_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_MULTIPLY(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_MULTIPLY_DS;
	struct vpu_binary_io_info in_info[] = TASK_MULTIPLY_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_MULTIPLY_OT_INFO;
	short raw_input[] = TASK_MULTIPLY_IN;
	short raw_otput[] = TASK_MULTIPLY_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_SCALE(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_SCALE_DS;
	struct vpu_binary_io_info in_info[] = TASK_SCALE_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_SCALE_OT_INFO;
	short raw_input[] = TASK_SCALE_IN;
	short raw_otput[] = TASK_SCALE_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_SOBEL(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_SOBEL_DS;
	struct vpu_binary_io_info in_info[] = TASK_SOBEL_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_SOBEL_OT_INFO;
	short raw_input[] = TASK_SOBEL_IN;
	short raw_otput[] = TASK_SOBEL_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_LUT(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_LUT_DS;
	struct vpu_binary_io_info in_info[] = TASK_LUT_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_LUT_OT_INFO;
	short raw_input[] = TASK_LUT_IN;
	short raw_otput[] = TASK_LUT_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_THRESHOLD(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_THRESHOLD_DS;
	struct vpu_binary_io_info in_info[] = TASK_THRESHOLD_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_THRESHOLD_OT_INFO;
	short raw_input[] = TASK_THRESHOLD_IN;
	short raw_otput[] = TASK_THRESHOLD_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_HALFSCALEGAUSSIAN(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_HALFSCALEGAUSSIAN_DS;
	struct vpu_binary_io_info in_info[] = TASK_HALFSCALEGAUSSIAN_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_HALFSCALEGAUSSIAN_OT_INFO;
	short raw_input[] = TASK_HALFSCALEGAUSSIAN_IN;
	short raw_otput[] = TASK_HALFSCALEGAUSSIAN_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_SAITFR0(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_SAITFR0_DS;
	struct vpu_binary_io_info in_info[] = TASK_SAITFR0_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_SAITFR0_OT_INFO;
	short raw_input[] = TASK_SAITFR0_IN;
	short raw_otput[] = TASK_SAITFR0_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_SAITFR1(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_SAITFR1_DS;
	struct vpu_binary_io_info in_info[] = TASK_SAITFR1_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_SAITFR1_OT_INFO;
	short raw_input[] = TASK_SAITFR1_IN;
	short raw_otput[] = TASK_SAITFR1_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_SAITFR2(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_SAITFR2_DS;
	struct vpu_binary_io_info in_info[] = TASK_SAITFR2_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_SAITFR2_OT_INFO;
	short raw_input[] = TASK_SAITFR2_IN;
	short raw_otput[] = TASK_SAITFR2_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

void * test_V_FLAMORB(void *user_data)
{
	int ret;
	struct client *user = user_data;
	const short raw[] = TASK_FLAMORB_DS;
	struct vpu_binary_io_info in_info[] = TASK_FLAMORB_IN_INFO;
	struct vpu_binary_io_info ot_info[] = TASK_FLAMORB_OT_INFO;
	short raw_input[] = TASK_FLAMORB_IN;
	short raw_otput[] = TASK_FLAMORB_OT;

	vpu_graph_RAW(user, raw, sizeof(raw));
	ret = test_V_Common(user, in_info, ot_info, raw_input, raw_otput);

	return (void *)ret;
}

#define DECLARE_VERIFY_TEST(test_set, n, _name, _flags) do { \
		test_set->tc[n].id = n; \
		snprintf(test_set->tc[n].name, 20, "verify-%s", #_name); \
		test_set->tc[n].func = test_V_##_name; \
		test_set->tc[n].flags = _flags; \
		verify_cnt++; \
	} while (0)

void vpu_test_verify_init(struct vpu_test_set *test_set)
{
	int i, verify_cnt = 0;
	unsigned int flags, cnn_flags;

	for (i = 0; i < VPU_MAX_TEST_CASES; ++i) {
		test_set->tc[i].id = 0;
		test_set->tc[i].func = NULL;
		test_set->tc[i].flags = 0;
	}

	flags = (1 << VS4L_GRAPH_FLAG_SHARED_AMONG_SUBCHAINS) |
		(1 << VS4L_GRAPH_FLAG_DSBL_LATENCY_BALANCING);

	cnn_flags = (1 << VS4L_GRAPH_FLAG_SHARED_AMONG_SUBCHAINS) |
		(1 << VS4L_GRAPH_FLAG_DSBL_LATENCY_BALANCING) |
		(1 << VS4L_STATIC_ALLOC_LARGE_MPRB_INSTEAD_SMALL_FLAG) |
		(VPU_PU_SLF50 << VS4L_STATIC_ALLOC_PU_INSTANCE_LSB);

	DECLARE_VERIFY_TEST(test_set, 1, ABSDIFF, flags);
	DECLARE_VERIFY_TEST(test_set, 2, ACC, flags);
	DECLARE_VERIFY_TEST(test_set, 3, ACCSQUARED, flags);
	DECLARE_VERIFY_TEST(test_set, 4, ACCWEIGHTED, flags);
	DECLARE_VERIFY_TEST(test_set, 5, ADD, flags);
	DECLARE_VERIFY_TEST(test_set, 6, SUB, flags);
	DECLARE_VERIFY_TEST(test_set, 7, AND, flags);
	DECLARE_VERIFY_TEST(test_set, 8, XOR, flags); /* 10 */
	DECLARE_VERIFY_TEST(test_set, 9, OR, flags);
	DECLARE_VERIFY_TEST(test_set, 10, NOT, flags);
	DECLARE_VERIFY_TEST(test_set, 11, BOXFILTER, flags);
	DECLARE_VERIFY_TEST(test_set, 12, CANNY, flags);
	DECLARE_VERIFY_TEST(test_set, 13, CHCOMBINE, flags);
	DECLARE_VERIFY_TEST(test_set, 14, CHEXTRACT, flags);
	DECLARE_VERIFY_TEST(test_set, 15, COLORCONVERT, flags);
	DECLARE_VERIFY_TEST(test_set, 16, CONVERTBITDEPTH, flags);
	DECLARE_VERIFY_TEST(test_set, 17, CONVOLUTION, flags);
	DECLARE_VERIFY_TEST(test_set, 18, DILATE, flags);
	DECLARE_VERIFY_TEST(test_set, 19, EQUALIZE, flags); /* 20 */
	DECLARE_VERIFY_TEST(test_set, 20, ERODE, flags);
	DECLARE_VERIFY_TEST(test_set, 21, FASTCORNER, flags);
	DECLARE_VERIFY_TEST(test_set, 22, GAUSSIANFILTER, flags);
	DECLARE_VERIFY_TEST(test_set, 23, HARRIS, flags);
	DECLARE_VERIFY_TEST(test_set, 24, HISTOGRAM, flags);
	DECLARE_VERIFY_TEST(test_set, 25, GAUSSIANPYRAMID, flags);
	DECLARE_VERIFY_TEST(test_set, 26, INTEGRAL, flags);
	DECLARE_VERIFY_TEST(test_set, 27, MAGNITUDE, flags);
	DECLARE_VERIFY_TEST(test_set, 28, MEANSTDDEV, flags);
	DECLARE_VERIFY_TEST(test_set, 29, MEDIAN, flags);
	DECLARE_VERIFY_TEST(test_set, 30, MINMAXLOC, flags);
	DECLARE_VERIFY_TEST(test_set, 32, PHASE, flags);
	DECLARE_VERIFY_TEST(test_set, 33, MULTIPLY, flags);
	DECLARE_VERIFY_TEST(test_set, 35, SCALE, flags);
	DECLARE_VERIFY_TEST(test_set, 36, SOBEL, flags); /* 30 */
	DECLARE_VERIFY_TEST(test_set, 37, LUT, flags);
	DECLARE_VERIFY_TEST(test_set, 38, THRESHOLD, flags);
	DECLARE_VERIFY_TEST(test_set, 41, HALFSCALEGAUSSIAN, flags);
	DECLARE_VERIFY_TEST(test_set, 42, SAITFR0, flags);
	DECLARE_VERIFY_TEST(test_set, 43, SAITFR1, cnn_flags);
	DECLARE_VERIFY_TEST(test_set, 44, SAITFR2, cnn_flags);
	DECLARE_VERIFY_TEST(test_set, 45, FLAMORB, flags);

	DECLARE_VERIFY_TEST(test_set, 50, 0, flags);
	DECLARE_VERIFY_TEST(test_set, 51, 1, flags);
	DECLARE_VERIFY_TEST(test_set, 52, 2, flags);
	DECLARE_VERIFY_TEST(test_set, 53, 3, flags);

	test_set->tc_cnt = verify_cnt;
}
