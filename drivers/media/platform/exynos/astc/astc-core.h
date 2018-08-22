/* linux/drivers/media/platform/astc-encoder/astc-core.h
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrea Bernabei <a.bernabei@samsung.com>
 * Author: Ramesh Munikrishnappa <ramesh.munikrishn@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ASTC_CORE_H_
#define ASTC_CORE_H_

#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/pm_qos.h>

#include <linux/hwastc.h>

#include <linux/videodev2.h>
#include <linux/videodev2_exynos_media.h>

#define MODULE_NAME		"exynos-astc"

#define NODE_NAME "astc"

#define ASTC_HEADER_BYTES               16
#define ASTC_BYTES_PER_BLOCK            16

/* astc hardware device state */
#define DEV_RUN		1
#define DEV_SUSPEND	2

#define ASTC_INPUT_IMAGE_MIN_SIZE 4
#define ASTC_INPUT_IMAGE_MAX_SIZE 16380

enum astc_mode {
	ENCODING,
};

/**
 * struct astc_dev - the abstraction for astc encoder device
 * @dev         : pointer to the astc encoder device
 * @clk_producer: the clock producer for this device
 * @regs        : the mapped hardware registers
 * @state       : device state flags
 * @slock       : spinlock for this data structure, mainly used for state var
 * @version     : IP version number
 *
 * @misc	: misc device descriptor for user-kernel interface
 * @tasks	: the list of the tasks that are to be scheduled
 * @contexts	: the list of the contexts that is created for this device
 * @lock_task	: lock to protect the consistency of @tasks list and
 *                @current_task
 * @lock_ctx	: lock to protect the consistency of @contexts list
 * @timeout_jiffies: timeout jiffoes for a task. If a task is not finished
 *                   until @timeout_jiffies elapsed,
 *                   timeout_task() is invoked an the task
 *                   is canced. The user will get an error.
 * @current_task: indicate the task that is currently being processed
 *                NOTE: this pointer is only valid while the task is being
 *                      PROCESSED, not while it's being prepared for processing.
 */
struct astc_dev {
	struct device                   *dev;
	struct clk                      *clk_producer;
	void __iomem                    *regs;
	unsigned long                    state;
	spinlock_t                       slock;
	u32                              version;
	struct pm_qos_request            qos_req;
	s32                              qos_req_level;

	struct miscdevice misc;
	struct list_head tasks;
	struct list_head contexts;
	spinlock_t lock_task;           /* lock with irqsave for tasks */
	spinlock_t lock_ctx;            /* lock for contexts */
	unsigned long timeout_jiffies;
	struct astc_task *current_task;
};

/**
 * astc_ctx - the device context data
 * @astc_dev:		ASTC ENCODER IP device for this context
 * @blocks_on_x         For the encoding process: the number of blocks
 *                      required on X axis.
 *                      This is computed based on the task and config received
 *                      from userspace and is then passed to the HW.
 * @blocks_on_y         For the encoding process: the number of blocks required
 *                      on Y axis
 *
 * @flags:		TBD: maybe not required
 *
 * @node	: node entry to astc_dev.contexts
 * @mutex	: lock to prevent racing between tasks between the same contexts
 * @kref	: usage count of the context not to release the context while a
 *              : task being processed.
 * @priv	: private data that is allowed to store client drivers' private
 *                data
 */
struct astc_ctx {
	struct astc_dev         *astc_dev;
	u16                      blocks_on_x;
	u16                      blocks_on_y;
	u32                      flags;

	struct list_head node;
	struct mutex mutex;
	struct kref kref;
	void *priv;
};


/**
 * enum astc_state - state of a task
 *
 * @ASTC_BUFSTATE_READY	: Task is verified and scheduled for processing
 * @ASTC_BUFSTATE_PROCESSING  : Task is being processed by H/W.
 * @ASTC_BUFSTATE_DONE	: Task is completed.
 * @ASTC_BUFSTATE_TIMEDOUT	: Task was not completed before timeout expired
 * @ASTC_BUFSTATE_ERROR:	: Task is not processed due to verification
 *                                failure
 */
enum astc_state {
	ASTC_BUFSTATE_READY,
	ASTC_BUFSTATE_PROCESSING,
	ASTC_BUFSTATE_DONE,
	ASTC_BUFSTATE_TIMEDOUT,
	ASTC_BUFSTATE_ERROR,
};

/**
 * struct astc_buffer_plane_dma - descriptions of a buffer
 *
 * @bytes_used	: the size of the buffer that is accessed by H/W. This is filled
 *                based on the task information received from userspace.
 * @dmabuf	: pointer to dmabuf descriptor if the buffer type is
 *		  HWASTC_BUFFER_DMABUF.
 *		  This is NULL when the buffer type is HWASTC_BUFFER_USERPTR and
 *		  the ptr points to user memory which did NOT already have a
 *		  dmabuf associated to it.
 * @attachment	: pointer to dmabuf attachment descriptor if the buffer type is
 *		  HWASTC_BUFFER_DMABUF (or if it's HWASTC_BUFFER_USERPTR and we
 *		  found and reused the dmabuf that was associated to that chunk
 *		  of memory, if there was any).
 * @sgt		: scatter-gather list that describes physical memory information
 *		: of the buffer. This is valid under the same condition as
 *		  @attachment and @dmabuf.
 * @dma_addr	: DMA address that is the address of the buffer in the H/W's
 *		: address space.
 *		  We use the IOMMU to map the input dma buffer or user memory
 *		  to a contiguous mapping in H/W's address space, so that H/W
 *		  can perform a DMA transfer on it.
 * @priv	: the client device driver's private data
 * @offset      : when buffer is of type HWASTC_BUFFER_USERPTR, this is the
 *		  offset of the buffer inside the VMA that holds it.
 */
struct astc_buffer_plane_dma {
	size_t bytes_used;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	void *priv;
	off_t offset;
};

/**
 * struct astc_buffer_dma - description of buffers for a task
 *
 * @buffers	: pointer to buffers received from userspace
 * @plane	: the corresponding internal buffer structure
 */
struct astc_buffer_dma {
	/* pointer to astc_task.task.buf_out/cap */
	const struct hwASTC_buffer *buffer;
	struct astc_buffer_plane_dma plane;
};

/**
 * struct astc_task - describes a task to process
 *
 * @user_task	: descriptions about the surfaces and format to process,
 *                provided by userspace.
 *                It also includes the encoding configuration (block size,
 *                dual plane, refinement iterations, partitioning, block mode)
 * @task_node	: list entry to astc_dev.tasks
 * @ctx		: pointer to astc_ctx that the task is valid under.
 * @complete	: waiter to finish the task
 * @dma_buf_out	: descriptions of the capture buffer (i.e. result from device)
 * @dma_buf_cap	: descriptions of the output buffer
 *                (i.e. what we're giving as input to device)
 * @state	: state of the task
 */
struct astc_task {
	struct hwASTC_task user_task;
	struct list_head task_node;
	struct astc_ctx *ctx;
	struct completion complete;
	struct astc_buffer_dma dma_buf_out;
	struct astc_buffer_dma dma_buf_cap;
	enum astc_state state;
};

#endif /* ASTC_CORE_H_ */
