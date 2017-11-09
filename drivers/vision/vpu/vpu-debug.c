/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <media/videobuf2-ion.h>

#include "vpu-config.h"
#include "vpu-debug.h"
#include "vpu-graphmgr.h"
#include "vpu-graph.h"
#include "vpu-system.h"
#include "vpu-mailbox.h"

#define DEBUG_FS_ROOT_NAME	"vpu"
#define DEBUG_FS_LOGFILE_NAME	"fw-msg"
#define DEBUG_FS_IMGFILE_NAME	"dump-img"
#define DEBUG_FS_GRPFILE_NAME	"graph"
#define DEBUG_FS_BUFFILE_NAME	"buffer"

#define CHUNKSZ			32

s32 atoi(const char *psz_buf)
{
        const char *pch = psz_buf;
        s32 base = 0;

        while (isspace(*pch))
                pch++;

        if (*pch == '-' || *pch == '+') {
                base = 10;
                pch++;
        } else if (*pch && tolower(pch[strlen(pch) - 1]) == 'h') {
                base = 16;
        }

        return simple_strtoul(pch, NULL, base);
}

int bitmap_scnprintf(char *buf, unsigned int buflen,
        const unsigned long *maskp, int nmaskbits)
{
        int i, word, bit, len = 0;
        unsigned long val;
        const char *sep = "";
        int chunksz;
        u32 chunkmask;

        chunksz = nmaskbits & (CHUNKSZ - 1);
        if (chunksz == 0)
                chunksz = CHUNKSZ;

        i = ALIGN(nmaskbits, CHUNKSZ) - CHUNKSZ;
        for (; i >= 0; i -= CHUNKSZ) {
                chunkmask = ((1ULL << chunksz) - 1);
                word = i / BITS_PER_LONG;
                bit = i % BITS_PER_LONG;
                val = (maskp[word] >> bit) & chunkmask;
                len += scnprintf(buf+len, buflen-len, "%s%0*lx", sep,
                        (chunksz+3)/4, val);
                chunksz = CHUNKSZ;
                sep = ",";
        }
        return len;
}

void vpu_dmsg_concate(struct vpu_debug_log *log, const char *fmt, ...)
{
	va_list ap;
	char term[50];
	u32 copy_len;

	va_start(ap, fmt);
	vsnprintf(term, sizeof(term), fmt, ap);
	va_end(ap);

	if (log->dsentence_pos >= DEBUG_SENTENCE_MAX) {
		vpu_err("debug message(%zd) over max\n", log->dsentence_pos);
		return;
	}

	copy_len = min((DEBUG_SENTENCE_MAX - log->dsentence_pos - 1), strlen(term));
	strncpy(log->dsentence + log->dsentence_pos, term, copy_len);
	log->dsentence_pos += copy_len;
	log->dsentence[log->dsentence_pos] = 0;
}

char * vpu_dmsg_print(struct vpu_debug_log *log)
{
	log->dsentence_pos = 0;
	return log->dsentence;
}

int vpu_debug_memdump8(u8 *start, u8 *end)
{
	int ret = 0;
	u8 *cur;
	u32 items, offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump8(%p ~ %p)", start, end);

	while (cur < end) {
		if ((items % 16) == 0) {
#ifdef DEBUG_LOG_MEMORY
			printk(KERN_DEBUG "%s\n", sentence);
#else
			printk(KERN_INFO "%s\n", sentence);
#endif
			offset = 0;
			snprintf(term, sizeof(term), "[V] %p:      ", cur);
			snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
			offset += strlen(term);
			items = 0;
		}

		snprintf(term, sizeof(term), "%02X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
#ifdef DEBUG_LOG_MEMORY
		printk(KERN_DEBUG "%s\n", sentence);
#else
		printk(KERN_INFO "%s\n", sentence);
#endif
	}

	ret = cur - end;

	return ret;
}


int vpu_debug_memdump16(u16 *start, u16 *end)
{
	int ret = 0;
	u16 *cur;
	u32 items, offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump16(%p ~ %p)", start, end);

	while (cur < end) {
		if ((items % 16) == 0) {
#ifdef DEBUG_LOG_MEMORY
			printk(KERN_DEBUG "%s\n", sentence);
#else
			printk(KERN_INFO "%s\n", sentence);
#endif
			offset = 0;
			snprintf(term, sizeof(term), "[V] %p:      ", cur);
			snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
			offset += strlen(term);
			items = 0;
		}

		snprintf(term, sizeof(term), "0x%04X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
#ifdef DEBUG_LOG_MEMORY
		printk(KERN_DEBUG "%s\n", sentence);
#else
		printk(KERN_INFO "%s\n", sentence);
#endif
	}

	ret = cur - end;

	return ret;
}

int vpu_debug_memdump32(u32 *start, u32 *end)
{
	int ret = 0;
	u32 *cur;
	u32 items, offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump32(%p ~ %p)", start, end);

	while (cur < end) {
		if ((items % 8) == 0) {
#ifdef DEBUG_LOG_MEMORY
			printk(KERN_DEBUG "%s\n", sentence);
#else
			printk(KERN_INFO "%s\n", sentence);
#endif
			offset = 0;
			snprintf(term, sizeof(term), "[V] %p:      ", cur);
			snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
			offset += strlen(term);
			items = 0;
		}

		snprintf(term, sizeof(term), "0x%08X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
#ifdef DEBUG_LOG_MEMORY
		printk(KERN_DEBUG "%s\n", sentence);
#else
		printk(KERN_INFO "%s\n", sentence);
#endif
	}

	ret = cur - end;

	return ret;
}

static int vpu_debug_log_open(struct inode *inode, struct file *file)
{
	if (inode->i_private)
		file->private_data = inode->i_private;

	return 0;
}

static ssize_t vpu_debug_log_read(struct file *file, char __user *user_buf,
	size_t buf_len, loff_t *ppos)
{
	const char *fwlog = "fwlog ...\n";

	memcpy(user_buf, fwlog, strlen(fwlog));

	return strlen(fwlog);
}

static int vpu_debug_img_open(struct inode *inode, struct file *file)
{
	if (inode->i_private)
		file->private_data = inode->i_private;

	return 0;
}

static ssize_t vpu_debug_img_read(struct file *file, char __user *user_buf,
	size_t len, loff_t *ppos)
{
	size_t size = 0;
	struct vpu_debug *debug;
	struct vpu_debug_imgdump *imgdump;

	debug = file->private_data;
	imgdump = &debug->imgdump;

	if (!imgdump->kvaddr) {
		vpu_err("kvaddr is NULL\n");
		return 0;
	}

	if (!imgdump->cookie) {
		vpu_err("cookie is NULL\n");
		return 0;
	}

	if (len <= imgdump->length)
		size = len;
	else
		size = imgdump->length;

	if (!size) {
		imgdump->cookie = NULL;
		imgdump->kvaddr = NULL;
		imgdump->length = 0;
		imgdump->offset = 0;
		goto p_err;
	}

	/* HACK for test */
	memset(imgdump->kvaddr, 0x88, size / 2);
	vb2_ion_sync_for_device(imgdump->cookie, imgdump->offset, size, DMA_FROM_DEVICE);
	memcpy(user_buf, imgdump->kvaddr, size);
	vpu_info("DUMP : %p, SIZE : %zd\n", imgdump->kvaddr, size);

	imgdump->offset += size;
	imgdump->length -= size;
	imgdump->kvaddr = (char *)imgdump->kvaddr + size;

p_err:
	return size;
}

static ssize_t vpu_debug_img_write(struct file *file, const char __user *user_buf,
	size_t len, loff_t *ppos)
{
	struct vpu_debug *debug;
	struct vpu_debug_imgdump *imgdump;
	struct vpu_graphmgr *graphmgr;
	struct vpu_graph *graph;
	struct vpuo_chain *chain;
	struct vpuo_pu *pu;
	struct vb_container *container;

	debug = file->private_data;
	graphmgr = debug->graphmgr_data;
	imgdump = &debug->imgdump;

	sscanf(user_buf, "%d %d %d %d", &imgdump->target_graph, &imgdump->target_chain, &imgdump->target_pu, &imgdump->target_index);
	vpu_info("target graph is %d\n", imgdump->target_graph);
	vpu_info("target chain is %d\n", imgdump->target_chain);
	vpu_info("target pu is %d\n", imgdump->target_pu);
	vpu_info("target index is %d\n", imgdump->target_index);

	graph = graphmgr->graph[imgdump->target_graph];
	if (!graph) {
		vpu_err("target graph %d is NULL\n", imgdump->target_graph);
		return len;
	}

	chain = vpu_graph_g_chain(graph, imgdump->target_chain);
	if (!chain) {
		vpu_err("target chain %d is NULL\n", imgdump->target_chain);
		return len;
	}

	pu = chain->pu_table[imgdump->target_pu];
	if (!pu) {
		vpu_err("target pu %d is NULL\n", imgdump->target_pu);
		return len;
	}

	container = pu->container[imgdump->target_index];
	if (!container) {
		vpu_err("target buffer %d is NULL\n", imgdump->target_index);
		return len;
	}

	if (!container->buffers[0].kvaddr) {
		vpu_err("kvaddr is NULL\n");
		return len;
	}

	imgdump->kvaddr = container->buffers[0].kvaddr;
	imgdump->cookie = container->buffers[0].cookie;
	imgdump->length = container->format->size[container->format->plane];
	imgdump->offset = len;

	return len;
}

static int vpu_debug_grp_show(struct seq_file *s, void *unused)
{
	u32 i;
	struct vpu_debug *debug = s->private;
	struct vpu_graphmgr *graphmgr = debug->graphmgr_data;
	struct vpu_graph *graph;

	seq_printf(s, "------------------------------------------"
			"----------------------------------------"
			"--------------------------------------\n");
	seq_printf(s, "%7.s %7.s %7.s %7.s %7.s %7.s %7.s\n",
			"graph", "prio", "period", "input", "done", "cancel", "recent");
	seq_printf(s, "------------------------------------------"
			"----------------------------------------"
			"--------------------------------------\n");

	mutex_lock(&graphmgr->mlock);
	for (i = 0; i < VPU_MAX_GRAPH; ++i) {
		graph = graphmgr->graph[i];

		if (!graph)
			continue;

		seq_printf(s, "%2d(%3d) %7d %7d %7d %7d %7d %7d\n",
			graph->id, graph->uid, graph->priority, graph->period_ticks,
			graph->input_cnt, graph->done_cnt, graph->cancel_cnt, graph->recent);
	}
	mutex_unlock(&graphmgr->mlock);

	seq_printf(s, "------------------------------------------"
			"----------------------------------------"
			"--------------------------------------\n");
	return 0;
}

static int vpu_debug_grp_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpu_debug_grp_show, inode->i_private);
}

static int vpu_debug_buf_show(struct seq_file *s, void *unused)
{
	u32 i, j, k;
	struct vpu_debug *debug = s->private;
	struct vpu_graphmgr *graphmgr = debug->graphmgr_data;
	struct vpu_graph *graph;
	struct vpuo_pu *pu, *temp;
	struct vb_container *container;
	struct vb_buffer *buffer;

	for (i = 0; i < VPU_MAX_GRAPH; ++i) {
		graph = graphmgr->graph[i];
		if (!graph)
			continue;

		seq_printf(s, "------------------------------------------"
			"----------------------------------------"
			"--------------------------------------\n");
		seq_printf(s, "GRAPH : %d(%d)\n", graph->id, graph->uid);
		seq_printf(s, "INPUT--------------------------------------------\n");
		list_for_each_entry_safe(pu, temp, &graph->inleaf_list, gleaf_entry) {
			seq_printf(s, "PU : %d\n", pu->id);
			seq_printf(s, "-------------------------------------------------\n");
			for (j = 0; j < VPU_MAX_BUFFER; ++j) {
				container = pu->container[j];
				if (!container)
					continue;

				if (j == 0) {
					seq_printf(s, "TYPE : %d\n", container->type);
					seq_printf(s, "RESOLUTION : %d x %d\n", container->format->width, container->format->height);
					seq_printf(s, "SIZE : %d\n", container->format->size[container->format->plane]);
					seq_printf(s, "-------------------------------------------------\n");
				}

				for (k = 0; k < container->count; ++k) {
					buffer = &container->buffers[k];
					seq_printf(s, "[%d][%d]DVADDR : %llx\n", j, k, buffer->dvaddr);
					seq_printf(s, "[%d][%d]KVADDR : %p\n", j, k, buffer->kvaddr);
				}

				seq_printf(s, "-------------------------------------------------\n");
			}
		}

		seq_printf(s, "OUTPUT-------------------------------------------\n");
		list_for_each_entry_safe(pu, temp, &graph->otleaf_list, gleaf_entry) {
			seq_printf(s, "PU : %d\n", pu->id);
			seq_printf(s, "-------------------------------------------------\n");
			for (j = 0; j < VPU_MAX_BUFFER; ++j) {
				container = pu->container[j];
				if (!container)
					continue;

				if (j == 0) {
					seq_printf(s, "TYPE : %d\n", container->type);
					seq_printf(s, "RESOLUTION : %d x %d\n", container->format->width, container->format->height);
					seq_printf(s, "SIZE : %d\n", container->format->size[container->format->plane]);
					seq_printf(s, "-------------------------------------------------\n");
				}

				for (k = 0; k < container->count; ++k) {
					buffer = &container->buffers[k];
					seq_printf(s, "[%d][%d]DVADDR : %llx\n", j, k, buffer->dvaddr);
					seq_printf(s, "[%d][%d]KVADDR : %p\n", j, k, buffer->kvaddr);
				}

				seq_printf(s, "-------------------------------------------------\n");
			}
		}
	}

	return 0;
}

static int vpu_debug_buf_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpu_debug_buf_show, inode->i_private);
}

static const struct file_operations vpu_debug_log_fops = {
	.open	= vpu_debug_log_open,
	.read	= vpu_debug_log_read,
	.llseek	= default_llseek
};

static const struct file_operations vpu_debug_img_fops = {
	.open	= vpu_debug_img_open,
	.read	= vpu_debug_img_read,
	.write	= vpu_debug_img_write,
	.llseek	= default_llseek
};

static const struct file_operations vpu_debug_grp_fops = {
	.open	= vpu_debug_grp_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations vpu_debug_buf_fops = {
	.open	= vpu_debug_buf_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static void vpu_debug_monitor(unsigned long data)
{
	struct vpu_debug_monitor *monitor = (struct vpu_debug_monitor *)data;
	struct vpu_debug *debug = container_of(monitor, struct vpu_debug, monitor);
	struct vpu_graphmgr *graphmgr = debug->graphmgr_data;
	struct vpu_system *system = debug->system_data;
	struct vpu_interface *interface = &system->interface;

	if (!test_bit(VPU_DEBUG_STATE_START, &debug->state))
		return;

	if (monitor->tick_cnt == graphmgr->tick_cnt)
		vpu_err("timer thread is stuck(%d, %d)\n", monitor->tick_cnt, graphmgr->tick_pos);

	if (monitor->sched_cnt == graphmgr->sched_cnt) {
		struct vpu_graph *graph;
		u32 i;

		vpu_info("GRAPH--------------------------------------------------------------\n");
		for (i = 0; i < VPU_MAX_GRAPH; i++) {
			graph = graphmgr->graph[i];
			if (!graph)
				continue;

			vpu_graph_print(graph);
		}

		vpu_info("GRAPH-MGR----------------------------------------------------------\n");
		vpu_gframe_print(graphmgr);

		vpu_info("INTERFACE-MGR------------------------------------------------------\n");
		vpu_interface_print(interface);

		vpu_info("-------------------------------------------------------------------\n");
		vpu_err("graph thread is stuck(%d, %d)\n", monitor->sched_cnt, graphmgr->sched_pos);
	}

	if (interface->framemgr.pro_cnt && (monitor->done_cnt == interface->done_cnt)) {
		struct vpu_mailbox_ctrl *mctrl;
		struct vpu_mailbox_h2f *h2f;
		struct vpu_mailbox_f2h *f2h;

		if (!test_bit(VPU_ITF_STATE_ENUM, &interface->state))
			goto p_err;

		mctrl = interface->private_data;
		if (!mctrl) {
			vpu_err("mctrl is NULL\n");
			goto p_err;
		}

		if (!mctrl->stack) {
			vpu_err("stack is NULL\n");
			goto p_err;
		}

		h2f = &mctrl->stack->h2f[0];
		vpu_info("H2F MBOX[0] : %4d %4d %4d\n", h2f->wptr_ofs16, h2f->rptr_ofs16, h2f->data_size16);

		h2f = &mctrl->stack->h2f[1];
		vpu_info("H2F MBOX[1] : %4d %4d %4d\n", h2f->wptr_ofs16, h2f->rptr_ofs16, h2f->data_size16);

		f2h = &mctrl->stack->f2h[0];
		vpu_info("F2H MBOX[0] : %4d %4d %4d\n", f2h->wmsg_idx, f2h->rmsg_idx, f2h->emsg_idx);

		f2h = &mctrl->stack->f2h[1];
		vpu_info("F2H MBOX[1] : %4d %4d %4d\n", f2h->wmsg_idx, f2h->rmsg_idx, f2h->emsg_idx);

		f2h = &mctrl->stack->f2h[2];
		vpu_info("F2H MBOX[2] : %4d %4d %4d\n", f2h->wmsg_idx, f2h->rmsg_idx, f2h->emsg_idx);

		f2h = &mctrl->stack->f2h[3];
		vpu_info("F2H MBOX[3] : %4d %4d %4d\n", f2h->wmsg_idx, f2h->rmsg_idx, f2h->emsg_idx);

		f2h = &mctrl->stack->f2h[4];
		vpu_info("F2H MBOX[4] : %4d %4d %4d\n", f2h->wmsg_idx, f2h->rmsg_idx, f2h->emsg_idx);

		f2h = &mctrl->stack->f2h[5];
		vpu_info("F2H MBOX[5] : %4d %4d %4d\n", f2h->wmsg_idx, f2h->rmsg_idx, f2h->emsg_idx);

		vpu_err("firmware is stuck(%d, %d)\n", monitor->done_cnt, interface->framemgr.req_cnt);
	}

p_err:
	monitor->tick_cnt = graphmgr->tick_cnt;
	monitor->sched_cnt = graphmgr->sched_cnt;
	monitor->done_cnt = interface->done_cnt;
	vpu_info("TIME %d(%d, %d, %d)\n", monitor->time_cnt, monitor->sched_cnt, monitor->tick_cnt, monitor->done_cnt);

	monitor->time_cnt++;
	mod_timer(&monitor->timer, jiffies + DEBUG_MONITORING_PERIOD);
}

int __vpu_debug_stop(struct vpu_debug *debug)
{
	int ret = 0;
	struct vpu_debug_monitor *monitor = &debug->monitor;

	if (!test_bit(VPU_DEBUG_STATE_START, &debug->state))
		goto p_err;

	del_timer(&monitor->timer);
	clear_bit(VPU_DEBUG_STATE_START, &debug->state);

p_err:
	return ret;
}

int vpu_debug_probe(struct vpu_debug *debug, void *graphmgr_data, void *system_data)
{
	debug->graphmgr_data = graphmgr_data;
	debug->system_data = system_data;

	debug->root = debugfs_create_dir(DEBUG_FS_ROOT_NAME, NULL);
	if (debug->root)
		probe_info("%s is created\n", DEBUG_FS_ROOT_NAME);

	debug->logfile = debugfs_create_file(DEBUG_FS_LOGFILE_NAME, S_IRUSR,
		debug->root, debug, &vpu_debug_log_fops);
	if (debug->logfile)
		probe_info("%s is created\n", DEBUG_FS_LOGFILE_NAME);

	debug->imgdump.file = debugfs_create_file(DEBUG_FS_IMGFILE_NAME, S_IRUSR,
		debug->root, debug, &vpu_debug_img_fops);
	if (debug->imgdump.file)
		probe_info("%s is created\n", DEBUG_FS_IMGFILE_NAME);

	debug->grpfile = debugfs_create_file(DEBUG_FS_GRPFILE_NAME, S_IRUSR,
		debug->root, debug, &vpu_debug_grp_fops);
	if (debug->grpfile)
		probe_info("%s is created\n", DEBUG_FS_GRPFILE_NAME);

	debug->grpfile = debugfs_create_file(DEBUG_FS_BUFFILE_NAME, S_IRUSR,
		debug->root, debug, &vpu_debug_buf_fops);
	if (debug->buffile)
		probe_info("%s is created\n", DEBUG_FS_BUFFILE_NAME);

	clear_bit(VPU_DEBUG_STATE_START, &debug->state);

	return 0;
}

int vpu_debug_open(struct vpu_debug *debug)
{
	return 0;
}

int vpu_debug_close(struct vpu_debug *debug)
{
	int ret = 0;

	ret = __vpu_debug_stop(debug);
	if (ret)
		vpu_err("__vpu_debug_stop is fail(%d)\n", ret);

	return ret;
}

int vpu_debug_start(struct vpu_debug *debug)
{
	int ret = 0;
	struct vpu_debug_monitor *monitor = &debug->monitor;
	struct vpu_graphmgr *graphmgr = debug->graphmgr_data;
	struct vpu_system *system = debug->system_data;
	struct vpu_interface *interface = &system->interface;

	monitor->tick_cnt = graphmgr->tick_cnt;
	monitor->sched_cnt = graphmgr->sched_cnt;
	monitor->done_cnt = interface->done_cnt;
	monitor->time_cnt = 0;

	set_bit(VPU_DEBUG_STATE_START, &debug->state);

	init_timer(&monitor->timer);
	monitor->timer.expires = jiffies + DEBUG_MONITORING_PERIOD;
	monitor->timer.data = (unsigned long)monitor;
	monitor->timer.function = vpu_debug_monitor;
	add_timer(&monitor->timer);

	return ret;
}

int vpu_debug_stop(struct vpu_debug *debug)
{
	int ret = 0;

	ret = __vpu_debug_stop(debug);
	if (ret)
		vpu_err("__vpu_debug_stop is fail(%d)\n", ret);

	return ret;
}
