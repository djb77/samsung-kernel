/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "score_log.h"
#include "score_scq.h"
#include "score_regs.h"
#include "score_packet.h"
#include "score_system.h"
#include "score_context.h"
#include "score_frame.h"

#if defined(CONFIG_EXYNOS_SCORE)
#define MC_INIT_TIMEOUT		100
#else /* FPGA */
#define MC_INIT_TIMEOUT		300
#endif

/**
 * __score_scq_wait_for_mc
 *@brief : wait for mc_init_done before scq_write
 *
 */
static int __score_scq_wait_mc(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	int ready;
	int time_count = MC_INIT_TIMEOUT;
	int i;

	score_enter();

	for (i = 0; i < time_count; i++) {
		ready = readl(scq->sfr + MC_INIT_DONE_CHECK);
		if ((ready & 0xffff0000) != 0)
			return ret;
		mdelay(1);
	}

	score_err("Init time(%dms)is over\n", MC_INIT_TIMEOUT);
	score_leave();
	return -ETIMEDOUT;
}

static int __score_scq_pending_create(struct score_scq *scq,
		struct score_frame *frame)
{
	int ret = 0;
	struct score_host_packet *packet;
	unsigned int size;

	score_enter();
	if (frame->pending_packet)
		return 0;

	packet = frame->packet;
	size = packet->size;

	frame->pending_packet = kmalloc(size, GFP_ATOMIC);
	if (!frame->pending_packet) {
		ret = -ENOMEM;
		score_err("Fail to alloc packet for pending frame\n");
		goto p_err;
	}

	memcpy(frame->pending_packet, packet, size);
	score_leave();
p_err:
	return ret;
}

/**
 * __score_scq_create
 *@brief : copy command from packet to scq and setting basic scq parameter
 *
 */
static int __score_scq_create(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	struct score_host_packet *packet;
	struct score_host_packet_info *packet_info;
	unsigned char *target_packet;
	unsigned int *data;
	unsigned int size;
	int i;

	score_enter();
	if (frame->pending_packet)
		packet = frame->pending_packet;
	else
		packet = frame->packet;

	packet_info = (struct score_host_packet_info *)&packet->payload[0];
	target_packet = (unsigned char *)packet_info + packet->packet_offset;

	data = (unsigned int *)target_packet;
	size = packet_info->valid_size >> 2;
	if (size > MAX_SCQ_SIZE) {
		score_err("size(%d) is invalid (%d-%d)\n",
				size, frame->sctx->id, frame->frame_id);
		ret = -EINVAL;
		goto p_err;
	}

	for (i = 0; i < size; i++)
		scq->out_param[i] = data[i];

	scq->out_size = size;
	scq->slave_id = 2;  //MASTER 2
	scq->priority = 0;

	score_leave();
p_err:
	return ret;
}

/**
 * __score_scq_write
 *@brief : atomic scq write sequence
 */
static int __score_scq_write(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	int elem;
	int scq_mset = 0;
	int scq_minfo;

	score_enter();
	//[17:8]	size of command msg to be send
	scq_mset |= ((scq->out_size) << 8);
	//[7:3]		sender ID
	scq_mset |= ((scq->slave_id) << 3);
	// [1] - priority / 0 : normal / 1: high
	scq_mset |= (scq->priority << 1);
	// [0] - queue_set / 1 : valid
	scq_mset |= 0x1;
	//SCQ_MSET write
	writel(scq_mset, scq->sfr + SCQ_MSET);
	//SCQ_MINFO read
	scq_minfo = readl(scq->sfr + SCQ_MINFO);

	frame->timestamp[SCORE_TIME_SCQ_WRITE] = score_util_get_timespec();
	// [31] - error / 0 : success / 1 : fail
	if ((scq_minfo & 0x80000000) == 0x80000000) {
		score_warn("Write queue of scq is full (%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = -EBUSY;
		goto p_err;
	} else {
#if defined(SCORE_EVT0)
		for (elem = 0; elem < scq->out_size; ++elem)
			writel(scq->out_param[elem], scq->sfr + SCQ_SRAM);
#else
		//SCQ out_param write to SRAM, SRAM1
		for (elem = 0; elem < (scq->out_size & 0x3fe); elem += 2) {
			writel(scq->out_param[elem], scq->sfr + SCQ_SRAM);
			writel(scq->out_param[elem + 1], scq->sfr + SCQ_SRAM1);
		}
		if (scq->out_size & 0x1)
			writel(scq->out_param[scq->out_size - 1],
					scq->sfr + SCQ_SRAM);
#endif
	}
	score_leave();
p_err:
	return ret;
}

/**
 * __score_scq_read
 *@brief : atomic scq read sequence
 */
static int __score_scq_read(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	int elem;
	int scq_sinfo;

	score_enter();
	//SCQ_TAKEN
	writel(0x1, scq->sfr + SCQ_STAKEN);
	//SCQ_SINFO read
	scq_sinfo = readl(scq->sfr + SCQ_SINFO);

	// [31] - error / 0 : success / 1 : fail
	if ((scq_sinfo & 0x80000000) == 0x80000000) {
		score_err("Read queue of scq is empty\n");
		ret = -ENOMEM;
		goto p_err;
	} else {
		// [17:8] - size of received command msg
		scq->in_size = (scq_sinfo >> 8) & 0x3ff;
		// [4:0] - master id
		scq->master_id = (scq_sinfo) & 0x1f;
#if defined(SCORE_EVT0)
		for (elem = 0; elem < scq->in_size; ++elem)
			scq->in_param[elem] = readl(scq->sfr + SCQ_SRAM);
#else
		//SCQ in_param read from SRAM, SRAM1
		for (elem = 0; elem < (scq->in_size & 0x3fe); elem += 2) {
			scq->in_param[elem] = readl(scq->sfr + SCQ_SRAM);
			scq->in_param[elem + 1] = readl(scq->sfr + SCQ_SRAM1);
		}
		if (scq->in_size & 0x1)
			scq->in_param[scq->in_size - 1] = readl(scq->sfr +
					SCQ_SRAM);
#endif
	}
	score_leave();
p_err:
	return ret;
}

int score_scq_read(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;

	score_enter();
	if (!test_bit(SCORE_SCQ_STATE_OPEN, &scq->state)) {
		score_warn("reading scq is already closed\n");
		ret = -ENOSTR;
		goto p_err;
	}

	ret = __score_scq_read(scq, frame);
	if (ret)
		goto p_err;

	// set return info from packet to frame
	frame->frame_id = (scq->in_param[1] >> 24);
	frame->ret = scq->in_param[2];

	scq->count--;

	score_leave();
p_err:
	return ret;
}

int score_scq_write(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;

	score_enter();
	if (!test_bit(SCORE_SCQ_STATE_OPEN, &scq->state)) {
		score_warn("writing scq is already closed\n");
		ret = -ENOSTR;
		goto p_err;
	}

	if (scq->count + 1 > MAX_HOST_TASK_COUNT) {
		score_warn("task is exceed, it will be pending (%d-%d)\n",
				frame->sctx->id, frame->frame_id);
		ret = __score_scq_pending_create(scq, frame);
		if (!ret)
			return -EBUSY;
		goto p_err;
	}

	ret = __score_scq_create(scq, frame);
	if (ret)
		goto p_err;

	if (scq->count == 0) {
		ret = __score_scq_wait_mc(scq, frame);
		if (ret)
			goto p_err;
	}

	ret = __score_scq_write(scq, frame);
	if (ret)
		goto p_err;

	scq->count++;

	score_leave();
p_err:
	kfree(frame->pending_packet);
	return ret;
}

void score_scq_init(struct score_scq *scq)
{
	score_enter();
	scq->count = 0;
	score_leave();
}

int score_scq_open(struct score_scq *scq)
{
	score_enter();
	score_scq_init(scq);
	set_bit(SCORE_SCQ_STATE_OPEN, &scq->state);
	score_leave();
	return 0;
}

int score_scq_close(struct score_scq *scq)
{
	score_enter();
	set_bit(SCORE_SCQ_STATE_CLOSE, &scq->state);
	score_leave();
	return 0;
}

int score_scq_probe(struct score_system *system)
{
	int ret = 0;
	struct score_scq *scq;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	score_enter();
	scq = &system->scq;
	scq->sfr = system->sfr;

	kthread_init_worker(&scq->write_worker);

	scq->write_task = kthread_run(kthread_worker_fn,
			&scq->write_worker, "score_scq");
	if (IS_ERR_OR_NULL(scq->write_task)) {
		ret = PTR_ERR(scq->write_task);
		score_err("scq_write kthread is not running (%d)\n", ret);
		goto p_err_run_write;
	}

	ret = sched_setscheduler_nocheck(scq->write_task, SCHED_FIFO, &param);
	if (ret) {
		score_err("scheduler setting of scq_write is fail(%d)\n", ret);
		goto p_err_sched_write;
	}

	set_bit(SCORE_SCQ_STATE_CLOSE, &scq->state);
	score_leave();
	return ret;
p_err_sched_write:
	kthread_stop(scq->write_task);
p_err_run_write:
	return ret;
}

int score_scq_remove(struct score_scq *scq)
{
	score_enter();
	kthread_stop(scq->write_task);

	scq->sfr = NULL;
	score_leave();
	return 0;
}
