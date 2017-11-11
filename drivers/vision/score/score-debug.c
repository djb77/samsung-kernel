/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/debugfs.h>
#include <linux/delay.h>

#include "score-config.h"
#include "score-debug.h"

struct score_debug score_debug;
EXPORT_SYMBOL(score_debug);

#define DEBUG_FS_ROOT_NAME	"score"
#define DEBUG_FS_LOGFILE_NAME	"msg"
#define DEBUG_FS_EVENTFILE_NAME	"event"

static int score_debug_event_open(struct inode *inode, struct file *file);

static const struct file_operations debug_log_fops;
static const struct file_operations debug_img_fops;
static const struct file_operations debug_event_fops = {
	.open = score_debug_event_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

void score_dump_regs(void __iomem *base_addr, u32 size)
{
	print_hex_dump(KERN_INFO, "[SCORE_DUMP]", DUMP_PREFIX_OFFSET, 32, 4,
			base_addr, size, false);
}

long timer_end(struct timespec start_time) {
	struct timespec end_time;
	getrawmonotonic(&end_time);
	return (end_time.tv_nsec - start_time.tv_nsec);
}

struct timespec timer_start(void) {
	struct timespec start_time;
	getrawmonotonic(&start_time);
	return start_time;
}

void score_dmsg_init(void)
{
	score_debug.dsentence_pos = 0;
	memset(score_debug.dsentence, 0x0, DEBUG_SENTENCE_MAX);
}

void score_dmsg_concate(const char *fmt, ...)
{
	va_list ap;
	char term[50];
	u32 copy_len;

	va_start(ap, fmt);
	vsnprintf(term, sizeof(term), fmt, ap);
	va_end(ap);

	copy_len = min((DEBUG_SENTENCE_MAX - score_debug.dsentence_pos), strlen(term));
	strncpy(score_debug.dsentence + score_debug.dsentence_pos, term, copy_len);
	score_debug.dsentence_pos += copy_len;
}

char *score_dmsg_print(void)
{
	return score_debug.dsentence;
}

void score_print_buffer(char *buffer, size_t len)
{
	u32 read_cnt, sentence_i;
	char sentence[250];
	char letter;

	BUG_ON(!buffer);

	sentence_i = 0;

	for (read_cnt = 0; read_cnt < len; read_cnt++) {
		letter = buffer[read_cnt];
		if (letter) {
			sentence[sentence_i] = letter;
			if (sentence_i >= 247) {
				sentence[sentence_i + 1] = '\n';
				sentence[sentence_i + 2] = 0;
				printk(KERN_DEBUG "%s", sentence);
				sentence_i = 0;
			} else if (letter == '\n') {
				sentence[sentence_i + 1] = 0;
				printk(KERN_DEBUG "%s", sentence);
				sentence_i = 0;
			} else {
				sentence_i++;
			}
		}
	}
}

int score_debug_probe(void)
{
	score_debug.system = NULL;

	score_debug.read_vptr = 0;

	score_debug.dsentence_pos = 0;
	memset(score_debug.dsentence, 0x0, DEBUG_SENTENCE_MAX);

	memset(score_debug.event_log, 0x0,
		sizeof(struct score_debug_event) * SCORE_EVENT_MAX_NUM);

#ifdef ENABLE_DBG_FS
	score_debug.root = debugfs_create_dir(DEBUG_FS_ROOT_NAME, NULL);

	score_debug.logfile = debugfs_create_file(DEBUG_FS_LOGFILE_NAME, S_IRUSR,
					score_debug.root,
					&score_debug,
					&debug_log_fops);

#ifdef ENABLE_DBG_EVENT
	atomic_set(&score_debug.event_index, -1);

	score_debug.event = debugfs_create_file(DEBUG_FS_EVENTFILE_NAME, 0444,
					score_debug.root,
					&score_debug,
					&debug_event_fops);

	snprintf(score_debug.build_date, MAX_EVENT_LOG_NUM, "(VER_%s_%s)", __func__, __func__);
	score_debug.iknownothing = 160219;

	/* score_debug_info_dump(debug); */
	/* score_event_test(debug); */

#endif
#endif
	clear_bit(SCORE_DEBUG_OPEN, &score_debug.state);

	return 0;
}

int score_debug_open(struct score_system *system)
{
	score_debug.system = system;

	/*
	 * debug control should be reset on camera entrance
	 * because firmware doesn't update this area after reset
	 */
	score_debug.read_vptr = 0;

	set_bit(SCORE_DEBUG_OPEN, &score_debug.state);

	return 0;
}

int score_debug_close(void)
{
	clear_bit(SCORE_DEBUG_OPEN, &score_debug.state);

	return 0;
}

int score_debug_start(void)
{
	clear_bit(SCORE_DEBUG_OPEN, &score_debug.state);

	return 0;
}

int score_debug_stop(void)
{
	clear_bit(SCORE_DEBUG_OPEN, &score_debug.state);

	return 0;
}

/**
  * score_debug_dma_dump: dump buffer by score_queue.
  *                         should be enable DBG_IMAGE_KMAPPING for kernel addr
  * @queue: buffer info
  * @index: buffer index
  * @vid: video node id for filename
  * @type: enum dbg_dma_dump_type
  **/
#if 0
int score_debug_dma_dump(struct score_queue *queue, u32 index, u32 vid, u32 type)
{
	int i = 0;
	int ret = 0;
	u32 flags = 0;
	int total_size = 0;
	u32 framecount = 0;
	char *filename = NULL;
	struct vb2_buffer *buf;
	struct score_binary bin;
	buf = queue->vbq->bufs[index];
	framecount = queue->framemgr.frames[index].fcount;

#ifdef DBG_DMA_DUMP_VID_COND
	if (!DBG_DMA_DUMP_VID_COND(vid))
		return 0;
#endif
	/* manipulateed the count for dump */
	if (((framecount - 1) % DBG_DMA_DUMP_INTEVAL) != 0)
		return 0;

	switch (type) {
	case DBG_DMA_DUMP_IMAGE:
		filename = __getname();

		if (unlikely(!filename))
			return -ENOMEM;

		snprintf(filename, PATH_MAX, "%s/V%02d_F%08d_I%02d.raw",
				DBG_DMA_DUMP_PATH, vid, framecount, index);

		for (i = 0; i < (buf->num_planes - 1); i++) {
			bin.data = (void *)queue->buf_kva[index][i];
			bin.size = queue->framecfg.size[i];

			if (!i) {
				/* first plane for image */
				flags = O_TRUNC | O_CREAT | O_WRONLY | O_APPEND;
				total_size += bin.size;
			} else {
				/* after first plane for image */
				flags = O_WRONLY | O_APPEND;
				total_size += bin.size;
			}

			ret = put_filesystem_binary(filename, &bin, flags);
			if (ret) {
				err("failed to dump %s (%d)", filename, ret);
				ret = -EINVAL;
				goto p_err;
			}
		}

		info("[V%d][F%d] img dumped..(%s, %d)\n", vid, framecount, filename, total_size);
		break;
	case DBG_DMA_DUMP_META:
		filename = __getname();

		if (unlikely(!filename))
			return -ENOMEM;

		snprintf(filename, PATH_MAX, "%s/V%02d_F%08d_I%02d.meta",
				DBG_DMA_DUMP_PATH, vid, framecount, index);

		bin.data = (void *)queue->buf_kva[index][buf->num_planes - 1];
		bin.size = queue->framecfg.size[buf->num_planes - 1];

		/* last plane for meta */
		flags = O_TRUNC | O_CREAT | O_WRONLY;
		total_size = bin.size;

		ret = put_filesystem_binary(filename, &bin, flags);
		if (ret) {
			err("failed to dump %s (%d)", filename, ret);
			ret = -EINVAL;
			goto p_err;
		}

		info("[V%d][F%d] meta dumped..(%s, %d)\n", vid, framecount, filename, total_size);
		break;
	default:
		err("invalid type(%d)", type);
		break;
	}

p_err:
	__putname(filename);

	return ret;
}
#endif

static int isfw_debug_open(struct inode *inode, struct file *file)
{
	if (inode->i_private)
		file->private_data = inode->i_private;

	return 0;
}

static ssize_t isfw_debug_read(struct file *file, char __user *user_buf,
	size_t buf_len, loff_t *ppos)
{
#if 0
	void *read_ptr;
	size_t write_vptr, read_vptr, buf_vptr;
	size_t read_cnt, read_cnt1, read_cnt2;
	struct score_memory_info *minfo;

	while (!test_bit(SCORE_DEBUG_OPEN, debug.state))
		msleep(500);

	minfo = score_debug.minfo;

retry:
	CALL_BUFOP(minfo->pb_fw, sync_for_cpu, minfo->pb_fw,
		DEBUG_REGION_OFFSET, DEBUG_REGION_SIZE + 4, DMA_FROM_DEVICE);

	write_vptr = *((int *)(minfo->kvaddr + DEBUGCTL_OFFSET)) - DEBUG_REGION_OFFSET;
	read_vptr = score_debug.read_vptr;
	buf_vptr = buf_len;

	if (write_vptr >= read_vptr) {
		read_cnt1 = write_vptr - read_vptr;
		read_cnt2 = 0;
	} else {
		read_cnt1 = DEBUG_REGION_SIZE - read_vptr;
		read_cnt2 = write_vptr;
	}

	if (buf_vptr && read_cnt1) {
		read_ptr = (void *)(minfo->kvaddr + DEBUG_REGION_OFFSET + score_debug.read_vptr);

		if (read_cnt1 > buf_vptr)
			read_cnt1 = buf_vptr;

		memcpy(user_buf, read_ptr, read_cnt1);
		score_debug.read_vptr += read_cnt1;
		buf_vptr -= read_cnt1;
	}

	if (score_debug.read_vptr >= DEBUG_REGION_SIZE) {
		if (score_debug.read_vptr > DEBUG_REGION_SIZE)
			err("[DBG] read_vptr(%zd) is invalid", score_debug.read_vptr);
		score_debug.read_vptr = 0;
	}

	if (buf_vptr && read_cnt2) {
		read_ptr = (void *)(minfo->kvaddr + DEBUG_REGION_OFFSET + score_debug.read_vptr);

		if (read_cnt2 > buf_vptr)
			read_cnt2 = buf_vptr;

		memcpy(user_buf, read_ptr, read_cnt2);
		score_debug.read_vptr += read_cnt2;
		buf_vptr -= read_cnt2;
	}

	read_cnt = buf_len - buf_vptr;

	/* info("[DBG] FW_READ : read_vptr(%zd), write_vptr(%zd) - dump(%zd)\n", read_vptr, write_vptr, read_cnt); */

	if (read_cnt == 0) {
		msleep(500);
		goto retry;
	}

	return read_cnt;
#else
	return 0;
#endif
}

static const struct file_operations debug_log_fops = {
	.open	= isfw_debug_open,
	.read	= isfw_debug_read,
	.llseek	= default_llseek
};

#ifdef ENABLE_DBG_EVENT
void score_event_test(struct score_debug *info)
{
	int kk = 0;
	struct score_debug_event dummy_msg;

	memset(&dummy_msg, 0, sizeof(struct score_debug_event));

	for (kk = 0; kk < SCORE_EVENT_MAX_NUM; kk ++) {
		if (kk % 3 != 2) {
			dummy_msg.type = SCORE_EVENT_INT_START;
			snprintf(dummy_msg.event_data.msg.text, MAX_EVENT_LOG_NUM,
				"(index %d)",kk);
			score_debug_event_add(info, &dummy_msg);
		} else {
			dummy_msg.type = SCORE_EVENT_SIZE;
			dummy_msg.event_data.size.id = 8;
			dummy_msg.event_data.size.total_width = 1920;
			dummy_msg.event_data.size.total_height = 1080;
			dummy_msg.event_data.size.width = 1280;
			dummy_msg.event_data.size.height = 720;
			dummy_msg.event_data.size.position_x = (1920 - 1280) / 2;
			dummy_msg.event_data.size.position_y = (1080 - 720) / 2;

			score_debug_event_add(info, &dummy_msg);
		}
	}

	return;
}

void score_event_add_lib_mem(bool type, char* comm, u32 size, ulong kvaddr,
		dma_addr_t dvaddr)
{
	struct score_debug_event lib_mem_info;

	memset(&lib_mem_info, 0x0, sizeof(struct score_debug_event));

	if (type)
		lib_mem_info.type = SCORE_EVENT_FW_MEM_ALLOC;
	else
		lib_mem_info.type = SCORE_EVENT_FW_MEM_FREE;

	memcpy(&lib_mem_info.event_data.lib_mem.comm, comm, TASK_COMM_LEN);
	lib_mem_info.event_data.lib_mem.size = size;
	lib_mem_info.event_data.lib_mem.kvaddr = kvaddr;
	lib_mem_info.event_data.lib_mem.dvaddr = dvaddr;

	score_debug_event_add(&score_debug, &lib_mem_info);

	return;
}

void score_event_add_ofy_recovered_count(unsigned int recovered_count)
{
	struct score_debug_event ofy_recovered_info;

	memset(&ofy_recovered_info, 0x0, sizeof(struct score_debug_event));

	ofy_recovered_info.event_data.overflow.recovered_count = recovered_count;

	score_debug_event_add(&score_debug, &ofy_recovered_info);

	return;
}

void score_event_concate_dram_msg(const char *fmt, ...)
{
	va_list ap;
	char term[MAX_EVENT_LOG_NUM];
	struct score_debug_event event_msg;

	memset(&event_msg, 0x0, sizeof(struct score_debug_event));
	event_msg.type = SCORE_EVENT_MSG;
	va_start(ap, fmt);
	vsnprintf(term, sizeof(term), fmt, ap);
	va_end(ap);

	memcpy(&event_msg.event_data.msg.text, term, MAX_EVENT_LOG_NUM);

	score_debug_event_add(&score_debug, &event_msg);
}

void score_event_add_dram_msg(char *msg)
{
	struct score_debug_event event_msg;

	memset(&event_msg, 0x0, sizeof(struct score_debug_event));

	memcpy(&event_msg.event_data.msg.text, msg, MAX_EVENT_LOG_NUM);

	score_debug_event_add(&score_debug, &event_msg);

	return;
}

static int score_event_show(struct seq_file *s, void *unused)
{
	struct score_debug *debug_info = s->private;

	score_debug_info_dump(s, debug_info);

	return 0;
}

static int score_debug_event_open(struct inode *inode, struct file *file)
{
	return single_open(file, score_event_show, inode->i_private);
}

void score_debug_event_add(struct score_debug *info, struct score_debug_event *event)
{
	int index = atomic_inc_return(&info->event_index) % SCORE_EVENT_MAX_NUM;
	struct score_debug_event *log = &info->event_log[index];

	memcpy(log, event, sizeof(struct score_debug_event));

	log->num = atomic_read(&info->event_index);
	log->time = ktime_get();
	/* log->type = SCORE_EVENT_INT_START; */

	return;
}

void score_debug_info_dump_reset(void)
{
	memset(score_debug.event_log, 0x0,
		sizeof(struct score_debug_event) * SCORE_EVENT_MAX_NUM);

	return;
}

int score_debug_info_dump2(void)
{
	struct score_debug *info = &score_debug;
	int index = atomic_read(&info->event_index) % SCORE_EVENT_MAX_NUM;
	int latest = index;
	struct timeval tv;
	ktime_t prev_ktime;
	struct score_debug_event *log ;

	printk("------------------- SCORE EVENT LOGGER - START --------------\n");

	if (index < 0)
		return -1;

	index = (index + SCORE_EVENT_MAX_NUM - SCORE_EVENT_PRINT_MAX) % SCORE_EVENT_MAX_NUM;
	prev_ktime = ktime_set(0, 0);

	do {
		if (++index >= SCORE_EVENT_MAX_NUM)
			index = 0;

		log = &info->event_log[index];
		tv = ktime_to_timeval(log->time);
		/* printk("[%6ld.%06ld] num(%d) ", tv.tv_sec, tv.tv_usec, log->num); */
		if (log->num == 0)
			continue;

		/* if (!tv.tv_sec) */
		/*	break; */

		switch (log->type) {
		case SCORE_EVENT_MSG:
			printk("%s", log->event_data.msg.text);
			break;
		case SCORE_EVENT_INT_START:
			printk("%20s %32s %20s", "INT_START",
					log->event_data.msg.text, "-\n");
			break;
		case SCORE_EVENT_INT_END:
			printk("%20s %32s %20s", "INT_END",
					log->event_data.msg.text, "-\n");
			break;
		case SCORE_EVENT_PACKET_SEND:
			printk("%20s %32s %20s", "PACKET_SEND",
					log->event_data.msg.text, "-\n");
			break;
		case SCORE_EVENT_PACKET_DONE:
			printk("%20s %32s %20s", "PACKET_DONE",
					log->event_data.msg.text, "-\n");
			break;
		case SCORE_EVENT_FW_MEM_ALLOC:
			printk("%20s %16s %16d 0x%16lx 0x%8lx %20s", "FW_MEM_ALLOC",
					log->event_data.lib_mem.comm,
					log->event_data.lib_mem.size,
					log->event_data.lib_mem.kvaddr,
					(unsigned long)log->event_data.lib_mem.dvaddr,
					"\n");
			break;
		case SCORE_EVENT_FW_MEM_FREE:
			printk("%20s %16s %16d 0x%16lx 0x%8lx %20s", "FW_MEM_FREE ",
					log->event_data.lib_mem.comm,
					log->event_data.lib_mem.size,
					log->event_data.lib_mem.kvaddr,
					(unsigned long)log->event_data.lib_mem.dvaddr,
					"-\n");
			break;
		case SCORE_EVENT_SIZE:
			printk("%20s %d %dx%d %dx%d %d %d %20s", "SIZE",
					log->event_data.size.id,
					log->event_data.size.total_width,
					log->event_data.size.total_height,
					log->event_data.size.width,
					log->event_data.size.height,
					log->event_data.size.position_x,
					log->event_data.size.position_y,
					"-\n");
			break;
		default :
			printk("%20s (%2d)\n", "NO_DEFINED", log->type);
			break;
		}
	} while (latest != index);

	printk("------------------- SCORE EVENT LOGGER - END ----------------\n");

	score_dbg(" %d, %s \n", info->iknownothing, info->build_date);
	score_dbg(" %ld %ld \n", sizeof(info->build_date), sizeof(info->event_log));

	return 0;
}

int score_debug_info_dump(struct seq_file *s, struct score_debug *info)
{
	int index = atomic_read(&info->event_index) % SCORE_EVENT_MAX_NUM;
	int latest = index;
	struct timeval tv;
	ktime_t prev_ktime;
	struct score_debug_event *log ;

	seq_printf(s, "------------------- SCORE EVENT LOGGER - START --------------\n");

	if (index < 0)
		return -1;

	index = (index + SCORE_EVENT_MAX_NUM - SCORE_EVENT_PRINT_MAX) % SCORE_EVENT_MAX_NUM;
	prev_ktime = ktime_set(0, 0);

	do {
		if (++index >= SCORE_EVENT_MAX_NUM)
			index = 0;

		log = &info->event_log[index];
		tv = ktime_to_timeval(log->time);
		seq_printf(s, "[%6ld.%06ld] num(%d) ", tv.tv_sec, tv.tv_usec, log->num);

		/* if (!tv.tv_sec) */
		/*	break; */

		switch (log->type) {
		case SCORE_EVENT_INT_START:
			seq_printf(s, "%20s %32s %20s", "INT_START",
					log->event_data.msg.text, "-\n");
			break;
		case SCORE_EVENT_INT_END:
			seq_printf(s, "%20s %32s %20s", "INT_END",
					log->event_data.msg.text, "-\n");
			break;
		case SCORE_EVENT_PACKET_SEND:
			seq_printf(s, "%20s %32s %20s", "PACKET_SEND",
					log->event_data.msg.text, "-\n");
			break;
		case SCORE_EVENT_PACKET_DONE:
			seq_printf(s, "%20s %32s %20s", "PACKET_DONE",
					log->event_data.msg.text, "-\n");
			break;
		case SCORE_EVENT_FW_MEM_ALLOC:
			seq_printf(s, "%20s %16s %16d 0x%16lx 0x%8lx %20s", "FW_MEM_ALLOC",
					log->event_data.lib_mem.comm,
					log->event_data.lib_mem.size,
					log->event_data.lib_mem.kvaddr,
					(unsigned long)log->event_data.lib_mem.dvaddr,
					"\n");
			break;
		case SCORE_EVENT_FW_MEM_FREE:
			seq_printf(s, "%20s %16s %16d 0x%16lx 0x%8lx %20s", "FW_MEM_FREE ",
					log->event_data.lib_mem.comm,
					log->event_data.lib_mem.size,
					log->event_data.lib_mem.kvaddr,
					(unsigned long)log->event_data.lib_mem.dvaddr,
					"-\n");
			break;
		case SCORE_EVENT_SIZE:
			seq_printf(s, "%20s %d %dx%d %dx%d %d %d %20s", "SIZE",
					log->event_data.size.id,
					log->event_data.size.total_width,
					log->event_data.size.total_height,
					log->event_data.size.width,
					log->event_data.size.height,
					log->event_data.size.position_x,
					log->event_data.size.position_y,
					"-\n");
			break;
		default :
			seq_printf(s, "%20s (%2d)\n", "NO_DEFINED", log->type);
			break;
		}
	} while (latest != index);

	seq_printf(s, "------------------- SCORE EVENT LOGGER - END ----------------\n");

	score_info(" %d, %s \n", info->iknownothing, info->build_date);
	score_info(" %ld %ld \n", sizeof(info->build_date), sizeof(info->event_log));

	return 0;
}
#endif
