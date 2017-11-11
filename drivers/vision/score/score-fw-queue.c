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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>

#include <linux/workqueue.h>

/* for proc */
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "score-fw-queue.h"
#include "score-device.h"
#include "score-system.h"

#include "score-regs-user-def.h"
#include "score-debug-print.h"

struct score_fw_dev *score_fw_device;

void score_fw_queue_dump_packet_group(struct score_ipc_packet *cmd)
{
	struct score_packet_size s = cmd->size;
	struct score_packet_header h = cmd->header;
	unsigned int group_count = s.group_count;
	int i, j;

	score_info("packet Size : %5d(packet_size), %3d(group_count)\n",
			s.packet_size, group_count);
	score_info("Packet Header : %d(queue_id), %4d(kernel_name), %3d(task_id), \
			%d(worker_name)\n", h.queue_id, h.kernel_name, h.task_id, h.worker_name);

	for (i = 0; i < group_count; ++i) {
		struct score_packet_group g = cmd->group[i];
		unsigned int valid_size = g.header.valid_size;
		score_info("Group[%3d] header : %2d(valid_size), %08X(fd_bitmap)\n",
				i, valid_size, g.header.fd_bitmap);
		for (j = 0; j < valid_size; ++j) {
			score_info("Group[%3d] params[%2d] : %08X\n", i, j, g.data.params[j]);
		}
	}
}

void score_fw_queue_dump_packet_word(struct score_ipc_packet *cmd)
{
	unsigned int size = cmd->size.packet_size;
	unsigned int *dump = (unsigned int *)cmd;
	int i;

	for (i = 0; i < size; ++i) {
		score_info("[Packet][%d] 0x%08X \n", i, dump[i]);
	}
}

void score_fw_queue_dump_regs(void)
{
	print_hex_dump(KERN_INFO, "[IPC-QUEUE]", DUMP_PREFIX_OFFSET, 32, 4,
			score_fw_device->sfr + SCORE_IN_QUEUE_TAIL, 64 * 4, false);
}

static void score_fw_queue_info_init(struct score_fw_queue *queue)
{
	writel(0, queue->sfr + queue->tail_info);
	writel(0, queue->sfr + queue->head_info);
}

static inline unsigned int score_fw_queue_get_head(struct score_fw_queue *queue)
{
	return readl(queue->sfr + queue->head_info);
}

static inline unsigned int score_fw_queue_get_tail(struct score_fw_queue *queue)
{
	return readl(queue->sfr + queue->tail_info);
}

static int score_fw_queue_put_data_direct(struct score_fw_queue *queue, unsigned int pos,
		unsigned int data)
{
	unsigned int reg_pos;

	if (pos >= queue->size)
		return -EINVAL;

	reg_pos = queue->start + (SCORE_REG_SIZE * pos);
	writel(data, queue->sfr + reg_pos);

	return 0;
}

static int score_fw_queue_get_data_direct(struct score_fw_queue *queue, unsigned int pos,
		unsigned int *data)
{
	unsigned int reg_pos;

	if (pos >= queue->size)
		return -EINVAL;

	reg_pos = queue->start + (SCORE_REG_SIZE * pos);
	*data = readl(queue->sfr + reg_pos);

	return 0;
}

static unsigned int score_fw_queue_get_used(struct score_fw_queue *queue)
{
	unsigned int used;
	unsigned int head, tail;
	unsigned int head_idx, tail_idx;

	head = score_fw_queue_get_head(queue);
	tail = score_fw_queue_get_tail(queue);
	head_idx = GET_QUEUE_IDX(head, queue->size);
	tail_idx = GET_QUEUE_IDX(tail, queue->size);

	if (head_idx > tail_idx)
		used = head_idx - tail_idx;
	else if (head_idx < tail_idx)
		used = (queue->size - tail_idx) + head_idx;
	else if (head != tail)
		used = queue->size;
	else
		used = 0;

	return used;
}

static int score_fw_queue_inc_head(struct score_fw_queue *queue,
		unsigned int head, unsigned int size)
{
	int ret = 0;
	unsigned int info;
	unsigned int real_info;

	info = GET_QUEUE_MIRROR_IDX((head + size), queue->size);
	writel(info, queue->sfr + queue->head_info);

	real_info = readl(queue->sfr + queue->head_info);
	if (unlikely(real_info != info)) {
		score_err("Head pos mismatch: info: %08X, real: %08X\n",
			info, real_info);
		ret = -EFAULT;
	}

	return ret;
}

static void score_fw_queue_inc_tail(struct score_fw_queue *queue,
		unsigned int tail, unsigned int size)
{
	unsigned int info;

	info = GET_QUEUE_MIRROR_IDX((tail + size), queue->size);
	writel(info, queue->sfr + queue->tail_info);
}

static inline unsigned int score_fw_queue_get_remain(struct score_fw_queue *queue)
{
	return (queue->size - score_fw_queue_get_used(queue));
}

static int score_fw_queue_get_total_valid_size(struct score_ipc_packet *cmd)
{
	struct score_packet_group *group = cmd->group;
	unsigned int group_count = cmd->size.group_count;
	unsigned int size = (sizeof(struct score_packet_size) +
			sizeof(struct score_packet_header)) >> 2;
	unsigned int valid_size;
	unsigned int fd_bitmap;
	int buf_count;
	int i, j;

	for (i = 0; i < group_count; ++i) {
		valid_size = group[i].header.valid_size;
		fd_bitmap  = group[i].header.fd_bitmap;
		buf_count  = 0;

		for (j = 0; j < valid_size; ++j) {
			if (fd_bitmap & (0x1 << j)) {
				buf_count++;
			}
		}

		size = (unsigned int)(size + valid_size -
			((sizeof(struct sc_host_buffer) >> 2) * buf_count));
	}

	return size;
}

int score_fw_queue_put_direct(struct score_fw_queue *queue, unsigned int head, unsigned int data)
{
	unsigned int head_idx = GET_QUEUE_IDX(head, queue->size);
#ifdef SCORE_FW_MSG_DEBUG
	unsigned int debug_tail =
		GET_QUEUE_IDX(score_fw_queue_get_tail(queue), queue->size);

	score_dbg("queue size:%d, head:%d, tail:%d\n",
			queue->size, head_idx, debug_tail);
#endif
	score_fw_queue_put_data_direct(queue, head_idx, data);

	return 0;
}

static int score_fw_queue_put_direct_params(struct score_fw_queue *queue,
		struct score_ipc_packet *cmd)
{
	int ret = 0;
	int i, j, k, offset = 0;
	unsigned int size = score_fw_queue_get_total_valid_size(cmd);
	unsigned int head = score_fw_queue_get_head(queue);

	unsigned int *size_word = (unsigned int *)&cmd->size;
	unsigned int *header_word = (unsigned int *)&cmd->header;

	unsigned int group_count = cmd->size.group_count;
	struct score_packet_group *group = cmd->group;
	unsigned int valid_size;
	unsigned int fd_bitmap;
	struct sc_packet_buffer *buffer;

	cmd->size.packet_size = size;
	score_fw_queue_put_direct(queue,
			GET_QUEUE_MIRROR_IDX(head, queue->size), *size_word);
	score_fw_queue_put_direct(queue,
			GET_QUEUE_MIRROR_IDX(head + 1, queue->size), *header_word);

	for (i = 0; i < group_count; ++i) {
		valid_size = group[i].header.valid_size;
		fd_bitmap  = group[i].header.fd_bitmap;

		for (j = 0; j < valid_size; ++j, ++offset) {
			if (fd_bitmap & (0x1 << j)) {
				buffer = (struct sc_packet_buffer*)(&group[i].data.params[j]);
				for (k = 0; k < (sizeof(struct sc_buffer) >> 2); ++k) {
					score_fw_queue_put_direct(queue,
						GET_QUEUE_MIRROR_IDX(head + 2 + offset + k, queue->size),
						group[i].data.params[j + k]);
				}
				j += ((sizeof(struct sc_packet_buffer) >> 2) - 1);
				offset += ((sizeof(struct sc_buffer) >> 2) - 1);
			} else {
				score_fw_queue_put_direct(queue,
					GET_QUEUE_MIRROR_IDX(head + 2 + offset, queue->size),
					group[i].data.params[j]);
			}
		}
	}
	ret = score_fw_queue_inc_head(queue, head, size);
	if (unlikely(ret < 0)) {
		goto p_err;
	}
	ret = score_system_enable(score_fw_device);

p_err:
	if (unlikely(ret < 0)) {
		score_dump_sfr(score_fw_device->sfr);
	}
	return ret;
}

int score_fw_queue_put(struct score_fw_queue *queue, struct score_ipc_packet *cmd)
{
	int ret;
	unsigned long flags;
	unsigned int remain;
	unsigned int size;

	spin_lock_irqsave(&queue->slock, flags);
	if (unlikely(queue->block == SCORE_FW_QUEUE_BLOCK)) {
		ret = -ENODEV;
		goto p_err;
	}

	remain = score_fw_queue_get_remain(queue);
	if (unlikely(remain > queue->size)) {
		ret = -EFAULT;
		score_fw_queue_info_init(queue);
		goto p_err;
	}

	size = score_fw_queue_get_total_valid_size(cmd);
	if (unlikely(size > queue->size)) {
		ret = -E2BIG;
		goto p_err;
	}

	if (remain >= size) {
		ret = score_fw_queue_put_direct_params(queue, cmd);
	} else {
		score_dbg("remain[%d] need[%d]\n", remain, size);
		ret = -EBUSY;
		goto p_err;
	}

p_err:
	spin_unlock_irqrestore(&queue->slock, flags);
	return ret;
}

int score_fw_queue_get_direct(struct score_fw_queue *queue, unsigned int tail, unsigned int *data)
{
	unsigned int tail_idx = GET_QUEUE_IDX(tail, queue->size);
#ifdef SCORE_FW_MSG_DEBUG
	unsigned int debug_head =
		GET_QUEUE_IDX(score_fw_queue_get_head(queue), queue->size);

	score_dbg("Queue size:%d, head:%d, tail:%d\n",
			queue->size, debug_head, tail_idx);
#endif
	score_fw_queue_get_data_direct(queue, tail_idx, data);

	return 0;
}

int score_fw_queue_get(struct score_fw_queue *queue, struct score_ipc_packet *cmd)
{
	int ret = 0;
	unsigned long flags;
	unsigned int tail = score_fw_queue_get_tail(queue);
	int size;

	unsigned int *size_word = (unsigned int *)&cmd->size;
	unsigned int *header_word = (unsigned int *)&cmd->header;

	unsigned int used;
	int i;

	spin_lock_irqsave(&queue->slock, flags);

	used = score_fw_queue_get_used(queue);
	if (used < 2) {
		ret = -ENODATA;
		score_fw_queue_info_init(queue);
		goto p_err;
	}

	score_fw_queue_get_direct(queue,
			GET_QUEUE_MIRROR_IDX(tail, queue->size), size_word);
	score_fw_queue_get_direct(queue,
			GET_QUEUE_MIRROR_IDX(tail + 1, queue->size), header_word);

	size = cmd->size.packet_size;
	if (used < size) {
		ret = -ENODATA;
		score_fw_queue_info_init(queue);
		goto p_err;
	}

	for (i = 0; i < size - 2; ++i) {
		score_fw_queue_get_direct(queue,
				GET_QUEUE_MIRROR_IDX((tail + 2 + i), queue->size),
				&cmd->group[0].data.params[i]);
	}

	score_fw_queue_inc_tail(queue, tail, size);

p_err:
	spin_unlock_irqrestore(&queue->slock, flags);
	return ret;
}

void score_fw_queue_block(struct score_fw_dev *fw_dev)
{
	struct score_fw_queue *inq;
	unsigned long flags;

	inq = fw_dev->in_queue;

	spin_lock_irqsave(&inq->slock, flags);
	inq->block = SCORE_FW_QUEUE_BLOCK;
	spin_unlock_irqrestore(&inq->slock, flags);
}

void score_fw_queue_unblock(struct score_fw_dev *fw_dev)
{
	struct score_fw_queue *inq;
	unsigned long flags;

	inq = fw_dev->in_queue;

	spin_lock_irqsave(&inq->slock, flags);
	inq->block = SCORE_FW_QUEUE_UNBLOCK;
	spin_unlock_irqrestore(&inq->slock, flags);
}

int score_fw_queue_init(struct score_fw_dev *fw_dev)
{
	int ret = 0;
	struct score_fw_queue *inq, *otq;
	unsigned long flags;

	inq = fw_dev->in_queue;
	otq = fw_dev->out_queue;

	spin_lock_irqsave(&inq->slock, flags);
	score_fw_queue_info_init(inq);
	spin_unlock_irqrestore(&inq->slock, flags);

	spin_lock_irqsave(&otq->slock, flags);
	score_fw_queue_info_init(otq);
	spin_unlock_irqrestore(&otq->slock, flags);

	return ret;
}

int score_fw_queue_probe(struct score_fw_dev *dev)
{
	struct score_fw_queue *in_queue;
	struct score_fw_queue *out_queue;
	struct score_device *device;
	struct score_system *system;

	device = container_of(dev, struct score_device, fw_dev);
	system = &device->system;

	dev->sfr = system->regs;

	dev->in_queue = kzalloc(sizeof(struct score_fw_queue), GFP_KERNEL);
	if (!dev->in_queue) {
		probe_err("Failed to allocate memory for in_queue\n");
		return -ENOMEM;
	}

	dev->out_queue = kzalloc(sizeof(struct score_fw_queue), GFP_KERNEL);
	if (!dev->out_queue) {
		probe_err("Failed to allocate memory for out_queue\n");
		kfree(dev->in_queue);
		return -ENOMEM;
	}

	in_queue = dev->in_queue;
	out_queue = dev->out_queue;

	in_queue->head_info = SCORE_IN_QUEUE_HEAD;
	in_queue->tail_info = SCORE_IN_QUEUE_TAIL;
	in_queue->start = SCORE_IN_QUEUE_START;
	in_queue->size = SCORE_IN_QUEUE_SIZE;
	in_queue->sfr = system->regs;
	spin_lock_init(&in_queue->slock);
	in_queue->block = SCORE_FW_QUEUE_UNBLOCK;

	out_queue->head_info = SCORE_OUT_QUEUE_HEAD;
	out_queue->tail_info = SCORE_OUT_QUEUE_TAIL;
	out_queue->start = SCORE_OUT_QUEUE_START;
	out_queue->size = SCORE_OUT_QUEUE_SIZE;
	out_queue->sfr = system->regs;
	spin_lock_init(&out_queue->slock);
	out_queue->block = SCORE_FW_QUEUE_UNBLOCK;

	score_fw_device = dev;
	return 0;
}

void score_fw_queue_release(struct score_fw_dev *dev)
{
	struct score_fw_queue *in_queue;
	struct score_fw_queue *out_queue;

	in_queue = dev->in_queue;
	out_queue = dev->out_queue;

	if (in_queue != NULL) {
		kfree(in_queue);
		in_queue = NULL;
	}

	if (out_queue != NULL) {
		kfree(out_queue);
		out_queue = NULL;
	}
}
