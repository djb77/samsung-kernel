/* drivers/media/platform/exynos/astc/astc-core.h
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
#include <linux/wait.h>

#include <linux/hwastc.h>

#define MODULE_NAME		"exynos-astc"

#define NODE_NAME "astc"

#define ASTC_HEADER_BYTES               16
#define ASTC_BYTES_PER_BLOCK            16

/* astc hardware device state
 * @DEV_RUN     : the driver is accessing the device. Do not suspend the device
 *                until this is unset.
 * @DEV_SUSPEND : the device should not be accessed as it's suspended or in the
 *                process of being suspended.
 *
 * FUTUREPROOFING ALERT: DEV_RUN/DEV_SUSPEND are used to futureproof the driver
 * for future usecases and environment configurations it might need to handle.
 * They are not needed by default on Exynos9810. Read more below.
 *
 * These flags are mainly used to prevent conflicts between the encoding job
 * worker and the system suspend callback.
 *
 * For that race to happen, at least one of the 2 conditions has to verify:
 * - CONFIG_SUSPEND_FREEZER is unset.
 *   If it were set, in fact, there would no race because userspace processes
 *   are frozen before the devices are asked to suspend. Freezing the
 *   userspace client implies waiting for the ioctl to return, and that implies
 *   waiting for the workqueue worker to be done encoding the task it was
 *   assigned (as we use flush_work, uninterruptible).
 *   See freezing-of-tasks and workqueue kernel docs.
 * - This driver is modified to add support for NONBLOCKing ioctl. In that case
 *   even though the userspace process is suspended, the workqueue worker could
 *   be still accessing the device (or waiting for HW to signal completion) at
 *   the point where the system suspend callback is invoked.
 *
 * NEITHER OF THE CONDITION HOLDS ON EXYNOS9810 at the time of writing (March18)
 * as CONFIG_SUSPEND_FREEZER is enabled by default and the driver does not yet
 * provide a NON blocking ioctl.
 * Yet it's good practice to increase robustness by supporting multiple kernel
 * configurations.
 *
 * Checking for DEV_RUN is better than checking Runtime PM device state, because
 * this way if we get a system sleep request while autosuspend timer has not
 * expired yet, we can still put the device to sleep without having to wait for
 * the autosuspend timer to expire.
 */
#define DEV_RUN		1
#define DEV_SUSPEND	2

#define ASTC_INPUT_IMAGE_MIN_SIZE 4
#define ASTC_INPUT_IMAGE_MAX_SIZE 16380

/* number of times to re-enqueue a job that failed for possibly temporary
 * issues */
#define MAX_NUM_RETRIES 2
/*
 * Block suspend if the device is still processing an image.
 * 200msecs should be enough to wait for the encoding of small-ish images.
 * If the device is still processing when the timeout expires,
 */
#define ASTC_SUSPEND_TIMEOUT HZ/5

#ifdef CONFIG_SOC_EXYNOS9810
/*
 * use a separate define from the generic soc_exynos9810, as it's more convenient
 * to undefine when we want to test both defined/not-defined builds
 */
#define EXYNOS9810_ASTC_WORKAROUND
#endif

enum astc_mode {
	ENCODING,
};

/**
 * struct astc_dev - the abstraction for astc encoder device
 * @dev           : pointer to the astc encoder device
 * @clk_producer  : the clock producer for this device
 * @regs          : the base of the mem region where H/W registers are mapped to
 * @state         : device state
 * @slock         : spinlock for this data structure, mainly used for state var
 * @version       : H/W IP version number
 * @qos_req       : handle to the QoS request
 * @qos_req_level : the frequency we want QoS framework to run our HW at
 *
 * @misc          : misc dev descriptor, the device is registered as type misc
 * @contexts      : the list of the contexts that are created for this device
 * @lock_task     : lock to protect the consistency of @current_task
 * @lock_ctx      : lock to protect the consistency of @contexts list
 * @current_task  : the task that is currently being executed.
 *                  NOTE: this pointer is only valid while the task is being
 *                  EXECUTED, not while it's being setup.
 * @workqueue     : the workqueue handling the executing of encoding tasks.
 * @suspend_wait  : the waitqueue used to handle power management, blocking
 *                  suspend until the device is ready to be suspended.
 *                  See DEV_SUSPEND doc to know when this is used/needed.
 *                  Make sure you read kernel doc related to freezing of tasks.
 */
struct astc_dev {
	struct device                   *dev;
	struct clk                      *clk_producer;
	void __iomem                    *regs;
	unsigned long                    state;
	spinlock_t                       slock;
	u32                              version;
	struct pm_qos_request            qos_req;
	u32                              qos_req_level;

	struct miscdevice                misc;
	struct list_head                 contexts;
	spinlock_t                       lock_task;
	spinlock_t                       lock_ctx;
	struct astc_task                *current_task;
	struct workqueue_struct         *workqueue;
	wait_queue_head_t                suspend_wait;
};

/**
 * struct astc_ctx - the device context data
 * @astc_dev	: ASTC ENCODER IP device for this context
 * @blocks_on_x	: For the encoding process: the number of blocks
 *                required on X axis.
 *                This is computed based on the task and config received
 *                from userspace and is then passed to the HW.
 * @blocks_on_y	: For the encoding process: the number of blocks required
 *                on Y axis
 *
 * @node	: node entry to astc_dev.contexts
 * @mutex	: lock to prevent racing between tasks of the same context
 * @kref	: usage count of the context not to release the context while a
 *              : task being processed.
 *
 * A context is used to serialize tasks of the same client (with a mutex) and
 * to provide a task with a place to store intermediate computations needed
 * for setting up the processing task (such as the number of ASTC blocks needed
 * to encode an image).
 * A task has exclusive access to its context while it is being setup/executed.
 * That allows us to reuse the same memory for all tasks of the same context
 * instead of having to add those intermediate variables to each task struct.
 *
 * A new context is created each time file_operation's open() callback is
 * triggered.
 * That usually means there is one context for each client process.
 *
 * Multi-threaded clients:
 * If the client runs multiple threads in the same process, and sends requests
 * to the driver using the same file descriptor, those tasks will share the
 * same context, as the FD table is shared across threads of the same process.
 */
struct astc_ctx {
	struct astc_dev         *astc_dev;
	u32                      blocks_on_x;
	u32                      blocks_on_y;

	struct list_head         node;
	struct mutex             mutex;
	struct kref              kref;
};


/**
 * enum astc_state - state of a task
 *
 * @ASTC_BUFSTATE_READY         : Task is verified and scheduled for processing
 * @ASTC_BUFSTATE_PROCESSING    : Task is being processed by H/W.
 * @ASTC_BUFSTATE_DONE          : Task is completed.
 * @ASTC_BUFSTATE_TIMEDOUT      : Task was not completed before timeout expired.
 *				  Currently unused.
 * @ASTC_BUFSTATE_RETRY         : Task failed to complete and needs to be
 *                                readded to the queue
 * @ASTC_BUFSTATE_ERROR         : Task is not processed due to verification
 *                                failure
 */
enum astc_state {
	ASTC_BUFSTATE_READY,
	ASTC_BUFSTATE_PROCESSING,
	ASTC_BUFSTATE_DONE,
	ASTC_BUFSTATE_TIMEDOUT,
	ASTC_BUFSTATE_RETRY,
	ASTC_BUFSTATE_ERROR,
};

/**
 * struct astc_buffer_plane_dma - data related to a DMA (H/W accessible) buffer
 *
 * @bytes_used	: the buffer size that the H/W requires to be able to process
 *                the data, based on the task info sent by userspace.
 *                This will be used as size of the mapping that maps the buffer
 *                received from userspace to the H/W address-space.
 * @dmabuf	: pointer to dmabuf descriptor.
 *                This is only valid if the buffer type is HWASTC_BUFFER_DMABUF
 *                or if it's of type HWASTC_BUFFER_USERPTR but it is backed
 *                by a dma_buf.
 *		  This is NULL when the buffer type is HWASTC_BUFFER_USERPTR and
 *		  the ptr points to user memory which did NOT already have a
 *		  dmabuf associated to it.
 * @attachment	: pointer to dmabuf attachment descriptor. This is valid under
 *                the same conditions as @dmabuf.
 * @sgt		: scatter-gather list that describes physical memory information
 *		: of the buffer. This is valid under the same condition as
 *		  @attachment and @dmabuf.
 * @dma_addr	: DMA address of the buffer after it has been mapped in the
 *                H/W's address space.
 *		  We use the IOVMM/IOMMU to map the buffer received from
 *                userspace to a contiguous mapping in H/W's address space, so
 *                that H/W can perform a DMA transfer on it.
 * @offset      : when buffer is of type HWASTC_BUFFER_USERPTR, this is the
 *		  offset of the buffer inside the VMA that holds it.
 */
struct astc_buffer_plane_dma {
	size_t                     bytes_used;
	struct dma_buf            *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table           *sgt;
	dma_addr_t                 dma_addr;
	off_t                      offset;
};

/**
 * struct astc_buffer_dma - wrapper around the internal buffer struct and
 *                          its userspace representation
 *
 * @buffer	: pointer to the userspace API buffer struct
 * @plane	: the corresponding internal buffer structure
 */
struct astc_buffer_dma {
	const struct hwASTC_buffer  *buffer;
	struct astc_buffer_plane_dma plane;
};

/**
 * struct astc_task - describes a task to process
 *
 * @user_task	: task information provided received from userspace.
 *                It includes info regarding buffers, encoding config, image
 *                formats, etc.
 * @ctx		: pointer to astc_ctx that the task is valid under.
 * @complete	: completion object used to wait for task completion
 * @dma_buf_out	: internal repr of the output buffer
 *                (i.e. what we're sending as input to H/W)
 * @dma_buf_cap	: internal repr of the capture buffer
 *                (i.e. the encoded result we get from the device)
 * @state	: state of the task
 * @work	: the work_struct used to schedule add this task to a workqueue
 */
struct astc_task {
	struct hwASTC_task     user_task;
	struct astc_ctx        *ctx;
	struct completion      complete;
	struct astc_buffer_dma dma_buf_out;
	struct astc_buffer_dma dma_buf_cap;
	enum astc_state        state;
	struct work_struct     work;
};

#endif /* ASTC_CORE_H_ */
