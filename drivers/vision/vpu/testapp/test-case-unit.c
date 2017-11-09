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

#include "test-binary-info.h"
#include "test-binary-harris.h"
#include "test-binary-minmaxloc.h"

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
#include "test-ds-flamorb.h"

void vpu_graph_S0(struct client *client);
void vpu_graph_RAW(struct client *client, const short *raw, int raw_size);

inline int vpu_get_time(struct timeval *a, struct timeval *b)
{
	return (b->tv_sec - a->tv_sec)*1000000 + (b->tv_usec - a->tv_usec);
}

static void * test_BASIC(void *user_data)
{
	int ret = 0;
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist;
	struct vs4l_container_list *otclist;

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_graph_S0(user);

	inclist = user->incfg.clist;
	otclist = user->otcfg.clist;

	ret += vpu_open(user);
	ret += vpu_close(user);

	if (ret) {
		printf("%s(%d)-1 fail\n", __func__, user->id);
		goto p_err;
	} else {
		printf("%s(%d)-1 pass\n", __func__, user->id);
	}

	ret += vpu_open(user);
	ret += vpu_s_graph(user, &user->graph);
	ret += vpu_close(user);

	if (ret) {
		printf("%s(%d)-2 fail\n", __func__, user->id);
		goto p_err;
	} else {
		printf("%s(%d)-2 pass\n", __func__, user->id);
	}

	ret += vpu_open(user);
	ret += vpu_s_graph(user, &user->graph);
	ret += vpu_s_format(user, &user->incfg.flist);
	ret += vpu_s_format(user, &user->otcfg.flist);
	ret += vpu_close(user);

	if (ret) {
		printf("%s(%d)-3 fail\n", __func__, user->id);
		goto p_err;
	} else {
		printf("%s(%d)-3 pass\n", __func__, user->id);
	}

	ret += vpu_open(user);
	ret += vpu_s_graph(user, &user->graph);
	ret += vpu_s_format(user, &user->incfg.flist);
	ret += vpu_s_format(user, &user->otcfg.flist);
	ret += vpu_stream_on(user);
	ret += vpu_close(user);

	if (ret) {
		printf("%s(%d)-4 fail\n", __func__, user->id);
		goto p_err;
	} else {
		printf("%s(%d)-4 pass\n", __func__, user->id);
	}

	ret += vpu_open(user);
	ret += vpu_s_graph(user, &user->graph);
	ret += vpu_s_format(user, &user->incfg.flist);
	ret += vpu_s_format(user, &user->otcfg.flist);
	ret += vpu_stream_on(user);
	ret += vpu_stream_off(user);
	ret += vpu_close(user);

	if (ret) {
		printf("%s(%d)-5 fail\n", __func__, user->id);
		goto p_err;
	} else {
		printf("%s(%d)-5 pass\n", __func__, user->id);
	}

p_err:
	client_p_buffer(user);
	return (void *)ret;
}

static void * test_STREAMING(void *user_data)
{
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist;
	struct vs4l_container_list *otclist;
	int index;

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	switch (user->id) {
	case 1:
	{
		const short raw[] = TASK_ABSDIFF_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 2:
	{
		const short raw[] = TASK_ACC_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 3:
	{
		const short raw[] = TASK_ACCSQUARED_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 4:
	{
		const short raw[] = TASK_ACCWEIGHTED_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 5:
	{
		const short raw[] = TASK_ADD_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 6:
	{
		const short raw[] = TASK_SUB_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 7:
	{
		const short raw[] = TASK_AND_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 8:
	{
		const short raw[] = TASK_XOR_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 9:
	{
		const short raw[] = TASK_OR_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 10:
	{
		const short raw[] = TASK_NOT_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 11:
	{
		const short raw[] = TASK_BOXFILTER_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 12:
	{
		const short raw[] = TASK_CANNY_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 13:
	{
		const short raw[] = TASK_CHCOMBINE_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 14:
	{
		const short raw[] = TASK_CHEXTRACT_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 15:
	{
		const short raw[] = TASK_COLORCONVERT_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 16:
	{
		const short raw[] = TASK_CONVERTBITDEPTH_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 17:
	{
		const short raw[] = TASK_CONVOLUTION_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 18:
	{
		const short raw[] = TASK_DILATE_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 19:
	{
		const short raw[] = TASK_EQUALIZE_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 20:
	{
		const short raw[] = TASK_ERODE_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 21:
	{
		const short raw[] = TASK_FASTCORNER_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 22:
	{
		const short raw[] = TASK_GAUSSIANFILTER_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 23:
	{
		const short raw[] = TASK_HARRIS_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 24:
	{
		const short raw[] = TASK_HISTOGRAM_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 25:
	{
		const short raw[] = TASK_GAUSSIANPYRAMID_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 26:
	{
		const short raw[] = TASK_INTEGRAL_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 27:
	{
		const short raw[] = TASK_MAGNITUDE_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 28:
	{
		const short raw[] = TASK_MEANSTDDEV_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 29:
	{
		const short raw[] = TASK_MEDIAN_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 30:
	{
		const short raw[] = TASK_MINMAXLOC_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 32:
	{
		const short raw[] = TASK_PHASE_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 33:
	{
		const short raw[] = TASK_MULTIPLY_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 35:
	{
		const short raw[] = TASK_SCALE_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 36:
	{
		const short raw[] = TASK_SOBEL_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 37:
	{
		const short raw[] = TASK_LUT_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 38:
	{
		const short raw[] = TASK_THRESHOLD_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 41:
	{
		const short raw[] = TASK_HALFSCALEGAUSSIAN_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	case 44:
	{
		const short raw[] = TASK_FLAMORB_DS;
		vpu_graph_RAW(user, raw, sizeof(raw));
		break;
	}
	default:
		vpu_graph_S0(user);
		break;
	}

	inclist = user->incfg.clist;
	otclist = user->otcfg.clist;

	vpu_open(user);
	vpu_s_graph(user, &user->graph);
	vpu_s_format(user, &user->incfg.flist);
	vpu_s_format(user, &user->otcfg.flist);
	vpu_stream_on(user);

	while (user->str_loop < user->end_loop) {
		index = user->str_loop % user->buffer_cnt;
		inclist[index].id = user->str_loop;
		otclist[index].id = user->str_loop;

		vpu_qbuf(user, &inclist[index]);
		vpu_qbuf(user, &otclist[index]);

		vpu_dqbuf(user, &inresult);
		vpu_dqbuf(user, &otresult);

		user->str_loop++;
	}

	vpu_stream_off(user);
	vpu_close(user);
	client_p_buffer(user);

	return NULL;
}

static void * test_OPENCLOSE(void *user_data)
{
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist;
	struct vs4l_container_list *otclist;
	int repeat = 10;

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_graph_S0(user);

	inclist = user->incfg.clist;
	otclist = user->otcfg.clist;

	while (repeat--) {
		vpu_open(user);
		vpu_s_graph(user, &user->graph);
		vpu_s_format(user, &user->incfg.flist);
		vpu_s_format(user, &user->otcfg.flist);
		vpu_stream_on(user);

		while (user->str_loop < user->end_loop) {
			inclist[0].id = user->str_loop;
			otclist[0].id = user->str_loop;

			vpu_qbuf(user, &inclist[0]);
			vpu_qbuf(user, &otclist[0]);
			vpu_dqbuf(user, &inresult);
			vpu_dqbuf(user, &otresult);

			user->str_loop++;
		}

		vpu_stream_off(user);
		vpu_close(user);

		printf("%s(%d)-%d pass\n", __func__, user->id, repeat);
		user->str_loop = 0;
	}

	client_p_buffer(user);

	return NULL;
}

static void * test_ONOFF(void *user_data)
{
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist[2];
	struct vs4l_container_list *otclist[2];
	int index, user_id, repeat;
	const short raw[] = TASK_HARRIS_DS;

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_graph_S0(&user[0]);

	inclist[0] = user[0].incfg.clist;
	otclist[0] = user[0].otcfg.clist;

	vpu_open(&user[0]);
	vpu_s_graph(&user[0], &user[0].graph);
	vpu_s_format(&user[0], &user[0].incfg.flist);
	vpu_s_format(&user[0], &user[0].otcfg.flist);

	repeat = 10;
	while (repeat--) {
		user[0].str_loop = 0;
		vpu_stream_on(&user[0]);

		while (user[0].str_loop < user[0].end_loop) {
			index = user[0].str_loop % user[0].buffer_cnt;
			inclist[0][index].id = user[0].str_loop;
			otclist[0][index].id = user[0].str_loop;

			vpu_qbuf(&user[0], &inclist[0][index]);
			vpu_qbuf(&user[0], &otclist[0][index]);
			vpu_dqbuf(&user[0], &inresult);
			vpu_dqbuf(&user[0], &otresult);

			user[0].str_loop++;
		}

		vpu_stream_off(&user[0]);

		printf("%s(%d)-%d pass\n", __func__, user[0].id, repeat);
	}

	vpu_close(&user[0]);

	vpu_graph_RAW(&user[1], raw, sizeof(raw));

	inclist[1] = user[1].incfg.clist;
	otclist[1] = user[1].otcfg.clist;

	vpu_open(&user[1]);
	vpu_s_graph(&user[1], &user[1].graph);
	vpu_s_format(&user[1], &user[1].incfg.flist);
	vpu_s_format(&user[1], &user[1].otcfg.flist);

	repeat = 10;
	while (repeat--) {
		user[1].str_loop = 0;
		vpu_stream_on(&user[1]);

		while (user[1].str_loop < user[1].end_loop) {
			index = user[1].str_loop % user[1].buffer_cnt;
			inclist[1][index].id = user[1].str_loop;
			otclist[1][index].id = user[1].str_loop;

			vpu_qbuf(&user[1], &inclist[1][index]);
			vpu_qbuf(&user[1], &otclist[1][index]);
			vpu_dqbuf(&user[1], &inresult);
			vpu_dqbuf(&user[1], &otresult);

			user[1].str_loop++;
		}

		vpu_stream_off(&user[1]);

		printf("%s(%d)-%d pass\n", __func__, user[1].id, repeat);
	}

	vpu_close(&user[1]);

	vpu_open(&user[0]);
	vpu_s_graph(&user[0], &user[0].graph);
	vpu_s_format(&user[0], &user[0].incfg.flist);
	vpu_s_format(&user[0], &user[0].otcfg.flist);
	vpu_open(&user[1]);
	vpu_s_graph(&user[1], &user[1].graph);
	vpu_s_format(&user[1], &user[1].incfg.flist);
	vpu_s_format(&user[1], &user[1].otcfg.flist);

	repeat = 10;
	user[0].str_loop = 0;
	user[1].str_loop = 0;
	while (repeat--) {
		user_id = repeat % 2;
		vpu_stream_on(&user[user_id]);

		index = repeat % user[user_id].buffer_cnt;
		inclist[user_id][index].id = repeat;
		otclist[user_id][index].id = repeat;

		vpu_qbuf(&user[user_id], &inclist[user_id][index]);
		vpu_qbuf(&user[user_id], &otclist[user_id][index]);
		vpu_dqbuf(&user[user_id], &inresult);
		vpu_dqbuf(&user[user_id], &otresult);

		vpu_stream_off(&user[user_id]);

		printf("%s(%d)-%d pass\n", __func__, user[user_id].id, repeat);
	}

	vpu_close(&user[0]);
	vpu_close(&user[1]);

	client_p_buffer(&user[0]);
	client_p_buffer(&user[1]);

	return NULL;
}

static void * test_PARAM(void *user_data)
{
	int ret = 0;
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist;
	struct vs4l_container_list *otclist;
	struct vs4l_format_list *inflist, *otflist;
	struct vs4l_format *format;
	struct vs4l_buffer *buffer;
	struct vs4l_param params[3];
	struct vs4l_param_list plist;
	int i, index, addr;
	unsigned short *data;

	const short raw[] = TASK_MINMAXLOC_DS;
	struct vpu_binary_io_info in_info[] = TASK_MINMAXLOC_IN_INFO;
	short raw_input[] = TASK_MINMAXLOC_IN;

	vpu_graph_RAW(user, raw, sizeof(raw));

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

		in_info[i].size[0] = util_g_size_from(format);
		in_info[i].addr[0] = addr;
		addr += in_info[i].size[0];
	}

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_open(user);
	vpu_s_graph(user, &user->graph);
	vpu_s_format(user, &user->incfg.flist);
	vpu_s_format(user, &user->otcfg.flist);
	vpu_stream_on(user);

	while (user->str_loop < user->end_loop) {
		index = user->str_loop % user->buffer_cnt;
		inclist[index].id = user->str_loop;
		otclist[index].id = user->str_loop;

		for (i = 0; i < inflist->count; ++i) {
			buffer = find_target_buffer(inclist, in_info[i].target, index);
			if (!buffer) {
				printf("buffer is NOT found(%X)\n", in_info[i].target);
				break;
			}

			memcpy((void *)buffer->reserved, (void *)in_info[i].addr, in_info[i].size);
		}

		vpu_qbuf(user, &inclist[index]);
		vpu_qbuf(user, &otclist[index]);

		vpu_dqbuf(user, &inresult);
		vpu_dqbuf(user, &otresult);

		data = (unsigned short *)otclist[index].containers[0].buffers[0].reserved;
		if ((data[0] != 1) || (data[3] != user->str_loop)) {
			ret = 1;
			break;
		}

		user->pu_array[1].params.map2list.threshold_low = user->str_loop + 1;
		user->pu_array[1].params.map2list.threshold_high = user->str_loop + 1;

		params[0].target = user->chain_array[0].id << VS4L_TARGET_SC_SHIFT | user->pu_array[1].instance;
		params[0].offset = 0;
		params[0].size = sizeof(union vpul_pu_parameters);
		params[0].addr = (unsigned long)&user->pu_array[1].params;

		plist.count = 1;
		plist.params = params;
		vpu_s_param(user, &plist);

		user->str_loop++;
	}

	vpu_stream_off(user);
	vpu_close(user);
	client_p_buffer(user);

	return (void *)ret;
}

/* poll test */
static void * test_POLL(void *user_data)
{
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist;
	struct vs4l_container_list *otclist;
	int index;

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_graph_S0(user);

	inclist = user->incfg.clist;
	otclist = user->otcfg.clist;

	vpu_open(user);
	vpu_s_graph(user, &user->graph);
	vpu_s_format(user, &user->incfg.flist);
	vpu_s_format(user, &user->otcfg.flist);
	vpu_stream_on(user);

	while (user->str_loop < user->end_loop) {
		index = user->str_loop % user->buffer_cnt;
		inclist[index].id = user->str_loop;
		otclist[index].id = user->str_loop;
		inclist[index].flags = 0;
		otclist[index].flags = 0;

		vpu_qbuf(user, &inclist[index]);
		vpu_qbuf(user, &otclist[index]);

		vpu_poll(user);

		vpu_dqbuf(user, &inresult);
		vpu_dqbuf(user, &otresult);

		user->str_loop++;
	}

	vpu_stream_off(user);
	vpu_close(user);
	client_p_buffer(user);

	return NULL;
}

static void * test_TIMESTAMP(void *user_data)
{
	struct client *user = user_data;
	struct vs4l_container_list inresult, otresult;
	struct vs4l_container_list *inclist;
	struct vs4l_container_list *otclist;
	struct timeval qtime, dqime;
	int index;

	inresult.direction = VS4L_DIRECTION_IN;
	inresult.count = 0;
	inresult.containers = NULL;
	otresult.direction = VS4L_DIRECTION_OT;
	otresult.count = 0;
	otresult.containers = NULL;

	vpu_graph_S0(user);

	inclist = user->incfg.clist;
	otclist = user->otcfg.clist;

	vpu_open(user);
	vpu_s_graph(user, &user->graph);
	vpu_s_format(user, &user->incfg.flist);
	vpu_s_format(user, &user->otcfg.flist);
	vpu_stream_on(user);

	while (user->str_loop < user->end_loop) {
		index = user->str_loop % user->buffer_cnt;
		inclist[index].id = user->str_loop;
		otclist[index].id = user->str_loop;

		inclist[index].flags = 1 << VS4L_CL_FLAG_TIMESTAMP;
		otclist[index].flags = 1 << VS4L_CL_FLAG_TIMESTAMP;

		gettimeofday(&qtime, NULL);

		vpu_qbuf(user, &inclist[index]);
		vpu_qbuf(user, &otclist[index]);

		vpu_dqbuf(user, &inresult);
		vpu_dqbuf(user, &otresult);

		gettimeofday(&dqime, NULL);

		if (inresult.flags & (1 << VS4L_CL_FLAG_TIMESTAMP)) {
			printf("[%d][%d] UQ~Q(%d), Q~R(%d), R~P(%d), P~D(%d), D~UD(%d)\n", user->graph.id, inresult.id,
				vpu_get_time(&qtime, &inresult.timestamp[0]),
				vpu_get_time(&inresult.timestamp[0], &inresult.timestamp[1]),
				vpu_get_time(&inresult.timestamp[1], &inresult.timestamp[3]),
				vpu_get_time(&inresult.timestamp[3], &inresult.timestamp[4]),
				vpu_get_time(&inresult.timestamp[4], &dqime));
		}

		user->str_loop++;
	}

	vpu_stream_off(user);
	vpu_close(user);
	client_p_buffer(user);

	return NULL;
}

void * test_T_BASIC(void *tc_data)
{
	int ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client user;
	ion_client iclient;

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	user.iclient = iclient;
	user.id = tc->id;
	user.graph.flags = tc->flags;
	user.graph.priority = 10;
	user.str_loop = 0;
	user.end_loop = 1;
	user.buffer_cnt = 1;

	ret = (int)test_BASIC(&user);

	ion_client_destroy(iclient);

	return (void *)ret;
}

void * test_T_STREAMING(void *tc_data)
{
	int ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client user;
	ion_client iclient;

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	user.iclient = iclient;
	user.id = tc->id;
	user.graph.flags = tc->flags;
	user.graph.priority = 0;
	user.str_loop = 0;
	user.end_loop = 50000;
	user.buffer_cnt = 5;

	ret = (int)test_STREAMING(&user);

	ion_client_destroy(iclient);

	return (void *)ret;
}

void * test_T_OPENCLOSE(void *tc_data)
{
	int ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client user;
	ion_client iclient;

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	user.iclient = iclient;
	user.id = tc->id;
	user.graph.priority = 1;
	user.graph.flags = tc->flags;
	user.str_loop = 0;
	user.end_loop = 100;
	user.buffer_cnt = 5;

	ret = (int)test_OPENCLOSE(&user);

	ion_client_destroy(iclient);

	return (void *)ret;
}

void * test_T_ONOFF(void *tc_data)
{
	int ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client user[2];
	ion_client iclient;

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	user[0].iclient = iclient;
	user[0].id = tc->id;
	user[0].graph.flags = tc->flags;
	user[0].graph.priority = 10;
	user[0].str_loop = 0;
	user[0].end_loop = 100;
	user[0].buffer_cnt = 5;

	user[1].iclient = iclient;
	user[1].id = tc->id + 1;
	user[1].graph.flags = tc->flags;
	user[1].graph.priority = 10;
	user[1].str_loop = 0;
	user[1].end_loop = 100;
	user[1].buffer_cnt = 5;

	ret = (int)test_ONOFF(&user);

	ion_client_destroy(iclient);

	return (void *)ret;
}

void * test_T_PARAM(void *tc_data)
{
	int ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client user;
	ion_client iclient;

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	user.iclient = iclient;
	user.id = tc->id;
	user.graph.flags = tc->flags;
	user.graph.priority = 10;
	user.str_loop = 0;
	user.end_loop = 20;
	user.buffer_cnt = 5;

	ret = (int)test_PARAM(&user);

	ion_client_destroy(iclient);

	return (void *)ret;
}

void * test_T_POLL(void *tc_data)
{
	int ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client user;
	ion_client iclient;

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	user.iclient = iclient;
	user.id = tc->id;
	user.graph.flags = tc->flags;
	user.graph.priority = 10;
	user.str_loop = 0;
	user.end_loop = 20;
	user.buffer_cnt = 5;

	ret = (int)test_POLL(&user);

	ion_client_destroy(iclient);

	return (void *)ret;
}

void * test_T_TIMESTAMP(void *tc_data)
{
	int ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client user;
	ion_client iclient;

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	user.iclient = iclient;
	user.id = tc->id;
	user.graph.flags = tc->flags;
	user.graph.priority = 10;
	user.str_loop = 0;
	user.end_loop = 20;
	user.buffer_cnt = 5;

	ret = (int)test_TIMESTAMP(&user);

	ion_client_destroy(iclient);

	return (void *)ret;
}

void * test_T_MULTIGRAPH(void *tc_data)
{
	int i, ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client *user;
	ion_client iclient;
	int *completes;
	int complete_cnt;
	int thread_num = 3;

	user = malloc(sizeof(struct client) * thread_num);
	completes = malloc(sizeof(int) * thread_num);

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	for (i = 0; i < thread_num; ++i) {
		user[i].iclient = iclient;
		user[i].id = tc->id + i;
		user[i].graph.priority = 1;
		user[i].graph.flags = tc->flags;
		user[i].str_loop = 0;
		user[i].end_loop = 100;
		user[i].buffer_cnt = 5;
		completes[i] = 0;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_create(&user[i].thread, NULL, test_STREAMING, (void *)&user[i]);
		if (ret) {
			printf("thread creation fail\n");
			abort();
		}

		printf("%s(%d) start\n", __func__, i);
	}

	i = 0;
	complete_cnt = 0;
	while (1) {
		if (!completes[i] && (user[i].str_loop == user[i].end_loop)) {
			completes[i] = 1;
			complete_cnt++;
			printf("%s(%d) done\n", __func__, i);
		}

		if (complete_cnt == thread_num)
			break;

		i = (i + 1) % thread_num;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_join(user[i].thread, NULL);
		if (ret) {
			printf("thread join fail\n");
			abort();
		}

		printf("%s(%d) end\n", __func__, i);
	}

	ion_client_destroy(iclient);
	free(user);

	return (void *)ret;
}

void *  test_T_PRIORITY(void *tc_data)
{
	int i, ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client *user;
	ion_client iclient;
	int *completes;
	int complete_cnt;
	int thread_num = 10;

	user = malloc(sizeof(struct client) * thread_num);
	completes = malloc(sizeof(int) * thread_num);

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	for (i = 0; i < thread_num; ++i) {
		user[i].iclient = iclient;
		user[i].id = tc->id + i;
		user[i].graph.priority = i + 1;
		user[i].graph.flags = tc->flags;
		user[i].str_loop = 0;
		user[i].end_loop = 50;
		user[i].buffer_cnt = 5;
		completes[i] = 0;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_create(&user[i].thread, NULL, test_STREAMING, (void *)&user[i]);
		if (ret) {
			printf("thread creation fail\n");
			abort();
		}

		printf("%s(%d) start\n", __func__, i);
	}

	i = 0;
	complete_cnt = 0;
	while (1) {
		if (!completes[i] && (user[i].str_loop == user[i].end_loop)) {
			completes[i] = 1;
			complete_cnt++;
			printf("%s(%d) done\n", __func__, i);
		}

		if (complete_cnt == thread_num)
			break;

		i = (i + 1) % thread_num;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_join(user[i].thread, NULL);
		if (ret) {
			printf("thread join fail\n");
			abort();
		}

		printf("%s(%d) end\n", __func__, i);
	}

	ion_client_destroy(iclient);
	free(user);

	return (void *)ret;
}

void * test_T_PERIODIC(void *tc_data)
{
	int i, ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client *user;
	ion_client iclient;
	int *completes;
	int complete_cnt;
	int thread_num = 10;

	user = malloc(sizeof(struct client) * thread_num);
	completes = malloc(sizeof(int) * thread_num);

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	for (i = 0; i < thread_num; ++i) {
		user[i].iclient = iclient;
		user[i].id = tc->id + i;
		user[i].graph.priority = i + 1;
		user[i].graph.flags = tc->flags;
		user[i].str_loop = 0;
		user[i].end_loop = 50;
		user[i].buffer_cnt = 5;
		completes[i] = 0;
	}

	user[0].graph.flags |= (1 << VS4L_GRAPH_FLAG_PERIODIC);
	user[0].graph.time = 20;

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_create(&user[i].thread, NULL, test_STREAMING, (void *)&user[i]);
		if (ret) {
			printf("thread creation fail\n");
			abort();
		}

		printf("%s(%d) start\n", __func__, i);
	}

	i = 0;
	complete_cnt = 0;
	while (1) {
		if (!completes[i] && (user[i].str_loop == user[i].end_loop)) {
			completes[i] = 1;
			complete_cnt++;
			printf("%s(%d) done\n", __func__, i);
		}

		if (complete_cnt == thread_num)
			break;

		i = (i + 1) % thread_num;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_join(user[i].thread, NULL);
		if (ret) {
			printf("thread join fail\n");
			abort();
		}

		printf("%s(%d) end\n", __func__, i);
	}

	ion_client_destroy(iclient);
	free(user);

	return (void *)ret;
}

void * test_T_PARRALLEL(void *tc_data)
{
	int i, ret = 0;
	struct vpu_test_case *tc = tc_data;
	struct client *user;
	ion_client iclient;
	int *completes;
	int complete_cnt;
	int thread_num = 10;

	user = malloc(sizeof(struct client) * thread_num);
	completes = malloc(sizeof(int) * thread_num);

	iclient = ion_client_create();
	if (iclient < 0) {
		printf("failed to open ion node\n");
		return (void *)-EINVAL;
	}

	for (i = 0; i < thread_num; ++i) {
		user[i].iclient = iclient;
		user[i].id = tc->id + i;
		user[i].graph.priority = 1;
		user[i].graph.flags = tc->flags;
		user[i].str_loop = 0;
		user[i].end_loop = 50;
		user[i].buffer_cnt = 5;
		completes[i] = 0;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_create(&user[i].thread, NULL, test_STREAMING, (void *)&user[i]);
		if (ret) {
			printf("thread creation fail\n");
			abort();
		}

		printf("%s(%d) start\n", __func__, i);
	}

	i = 0;
	complete_cnt = 0;
	while (1) {
		if (!completes[i] && (user[i].str_loop == user[i].end_loop)) {
			completes[i] = 1;
			complete_cnt++;
			printf("%s(%d) done\n", __func__, i);
		}

		if (complete_cnt == thread_num)
			break;

		i = (i + 1) % thread_num;
	}

	for (i = 0; i < thread_num; ++i) {
		ret = pthread_join(user[i].thread, NULL);
		if (ret) {
			printf("thread join fail\n");
			abort();
		}

		printf("%s(%d) end\n", __func__, i);
	}

	ion_client_destroy(iclient);
	free(user);

	return (void *)ret;
}

#define DECLARE_UNIT_TEST(test_set, n, _name, _flags) do { \
		test_set->tc[n].id = n; \
		snprintf(test_set->tc[n].name, 20, "unit-%s", #_name); \
		test_set->tc[n].func = test_T_##_name; \
		test_set->tc[n].flags = _flags; \
		unit_cnt++; \
	} while (0)

void vpu_test_unit_init(struct vpu_test_set *test_set)
{
	int i, unit_cnt = 0;
	unsigned int flags;

	for (i = 0; i < VPU_MAX_TEST_CASES; ++i) {
		test_set->tc[i].func = NULL;
		test_set->tc[i].flags = 0;
	}

	flags = (1 << VS4L_GRAPH_FLAG_SHARED_AMONG_SUBCHAINS) |
		(1 << VS4L_GRAPH_FLAG_DSBL_LATENCY_BALANCING);

	DECLARE_UNIT_TEST(test_set, 1, BASIC, flags);
	DECLARE_UNIT_TEST(test_set, 2, STREAMING, flags);
	DECLARE_UNIT_TEST(test_set, 3, OPENCLOSE, flags);
	DECLARE_UNIT_TEST(test_set, 4, ONOFF, flags);
	DECLARE_UNIT_TEST(test_set, 5, PARAM, flags);
	DECLARE_UNIT_TEST(test_set, 6, POLL, flags);
	DECLARE_UNIT_TEST(test_set, 7, TIMESTAMP, flags);
	DECLARE_UNIT_TEST(test_set, 8, MULTIGRAPH, flags);
	DECLARE_UNIT_TEST(test_set, 9, PRIORITY, flags);
	DECLARE_UNIT_TEST(test_set, 10, PARRALLEL, flags);

	test_set->tc_cnt = unit_cnt;
}
