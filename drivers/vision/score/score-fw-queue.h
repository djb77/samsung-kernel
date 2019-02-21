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

#ifndef _SCORE_CMD_QUEUE_H
#define _SCORE_CMD_QUEUE_H

#include <linux/spinlock.h>

#include "score-config.h"
#include "score-regs.h"
#include "score-regs-user-def.h"

/* queue informaition register count */
#define SCORE_QUEUE_INFO_SIZE			(2)
/* in:out = 44:4 */
/* queue input register count */
#define SCORE_IN_QUEUE_SIZE			(44)
/* queue output register count */
#define SCORE_OUT_QUEUE_SIZE			(4)
/* queue sfr register count */
#define SCORE_QUEUE_REG_SIZE			\
	(SCORE_QUEUE_INFO_SIZE + SCORE_IN_QUEUE_SIZE + \
	 SCORE_QUEUE_INFO_SIZE + SCORE_OUT_QUEUE_SIZE)

#define SCORE_IN_QUEUE_TAIL			(SCORE_USERDEFINED0)
#define SCORE_IN_QUEUE_HEAD			(SCORE_USERDEFINED1)
#define SCORE_OUT_QUEUE_TAIL			(SCORE_USERDEFINED46)
#define SCORE_OUT_QUEUE_HEAD			(SCORE_USERDEFINED47)

#define SCORE_IN_QUEUE_START			\
	((SCORE_IN_QUEUE_TAIL) + ((SCORE_QUEUE_INFO_SIZE)*(SCORE_REG_SIZE)))

#define SCORE_OUT_QUEUE_START			\
	((SCORE_OUT_QUEUE_TAIL) + ((SCORE_QUEUE_INFO_SIZE)*(SCORE_REG_SIZE)))

#define GET_MIRROR_STATE(X, SIZE)		(((X) >= (SIZE))? 1:0)
#define GET_QUEUE_IDX(X, SIZE)			\
	(GET_MIRROR_STATE((X), (SIZE))? ((X) - (SIZE)):(X))
#define GET_QUEUE_MIRROR_IDX(X, SIZE)		\
	(((X) < ((SIZE)<<1))? (X):((X) - ((SIZE)<<1)))

#define SCORE_MAX_PARAM				(42)
#define SCORE_MAX_RESULT			(3)
#define SCORE_MAX_GROUP				(2)
#define NUM_OF_GRP_PARAM			(26)

#define SCORE_FW_QUEUE_BLOCK			(1)
#define SCORE_FW_QUEUE_UNBLOCK			(0)

struct score_packet_group_data {
	unsigned int params[NUM_OF_GRP_PARAM];
};

struct score_packet_group_header {
	unsigned int valid_size : 6;
	unsigned int fd_bitmap  : 26;
};

struct score_packet_group {
	struct score_packet_group_header header;
	struct score_packet_group_data data;
};

struct score_packet_header {
	unsigned int queue_id    : 2;
	unsigned int kernel_name : 12;
	unsigned int task_id     : 7;
	unsigned int vctx_id     : 8;
	unsigned int worker_name : 3;
};
struct score_packet_size {
	unsigned int packet_size : 14;
	unsigned int reserved    : 10;
	unsigned int group_count : 8;
};

struct score_ipc_packet {
	struct score_packet_size size;
	struct score_packet_header header;
	struct score_packet_group group[0];
};

struct score_fw_queue {
	unsigned int	head_info;
	unsigned int	tail_info;
	unsigned int	start;
	unsigned int	size;
	spinlock_t	slock;
	unsigned int	block;

	void __iomem	*sfr;
};

struct score_fw_dev {
	void __iomem  *sfr;

	struct score_fw_queue *in_queue;
	struct score_fw_queue *out_queue;
};

struct sc_buffer {
	unsigned int width;
	unsigned int height;
	unsigned int type;
	unsigned int addr;
};

struct sc_host_buffer {
	int memory_type;
	int memory_size;
	union {
		int		fd;
		unsigned long	userptr;
	} m;
	int offset;
	int reserved;
};

struct sc_packet_buffer {
	struct sc_buffer      buf;
	struct sc_host_buffer host_buf;
};

int score_fw_queue_probe(struct score_fw_dev *dev);
void score_fw_queue_release(struct score_fw_dev *dev);
int score_fw_queue_init(struct score_fw_dev *dev);

int score_fw_queue_put(struct score_fw_queue *queue, struct score_ipc_packet *cmd);
int score_fw_queue_get(struct score_fw_queue *queue, struct score_ipc_packet *cmd);
int score_fw_queue_put_direct(struct score_fw_queue *queue,
		unsigned int head, unsigned int data);
int score_fw_queue_get_direct(struct score_fw_queue *queue,
		unsigned int tail, unsigned int *data);

void score_fw_queue_block(struct score_fw_dev *fw_dev);
void score_fw_queue_unblock(struct score_fw_dev *fw_dev);

void score_fw_queue_dump_packet_word(struct score_ipc_packet *cmd);
void score_fw_queue_dump_packet_group(struct score_ipc_packet *cmd);
void score_fw_queue_dump_regs(void);

#endif
