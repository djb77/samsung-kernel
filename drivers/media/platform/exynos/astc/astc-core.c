/* linux/drivers/media/platform/exynos/astc-core.c
 *
 * Core file for Samsung ASTC encoder driver
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

//#define DEBUG
//#define HWASTC_PROFILE_ENABLE

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/kmod.h>
#include <linux/time.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/exynos_iovmm.h>
#include <asm/page.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/time.h>

#include <linux/pm_runtime.h>

//This holds the enums used to communicate with userspace
#include <linux/hwastc.h>

#include "astc-core.h"
#include "astc-regs.h"
#include "astc-helper.h"

//The name of the gate clock. This is set in the device-tree entry
static const char *clk_producer_name = "gate";

#ifdef HWASTC_PROFILE_ENABLE
static struct timeval global_time_start;
static struct timeval global_time_end;
#define HWASTC_PROFILE(statement, msg, dev) \
	do { \
		/* Use local structs to support nesting HWASTC_PROFILE calls */\
		struct timeval time_start; \
		struct timeval time_end; \
		do_gettimeofday(&time_start); \
		statement; \
		do_gettimeofday(&time_end); \
		dev_info(dev, msg ": %ld microsecs\n", \
			1000000 * (time_end.tv_sec - time_start.tv_sec) + \
			(time_end.tv_usec - time_start.tv_usec)); \
	} while (0)
#else
#define HWASTC_PROFILE(statement, msg, dev) \
	do { \
		statement; \
	} while (0)
#endif

/**
 * astc_compute_buffer_size:
 *
 * Compute the size that the buffer(s) must have to
 * be able to respect that format. The result is saved in the variable
 * pointed to by bytes_used.
 *
 * The dma_data_direction is used to distinguish between output and capture
 * (in/out) buffers.
 *
 * srcBitsPerPixel is only used to compute the size of the buffer that
 * holds the source image we were asked to process.
 *
 * Returns 0 on success, otherwise < 0 err num
 */
static int astc_compute_buffer_size(struct astc_ctx *ctx,
				    struct astc_task *task,
				    enum dma_data_direction dir,
				    size_t *bytes_used,
				    int srcBitsPerPixel);

/**
 * astc_buffer_map:
 *
 * Finalize the necessary preparation to make the buffer
 * accessible to HW, e.g. dma attachment setup and iova mapping
 * (by using IOVMM API, which will use IOMMU).
 * The buffer will be made available to H/W as a contiguous chunk
 * of virtual memory which, thanks to the IOMMU, can be used for
 * DMA transfers even though it the mapping might refer to scattered
 * physical pages.
 *
 * Returns 0 on success, otherwise < 0 err num
 */
static int astc_buffer_map(struct astc_ctx *ctx,
			       struct astc_buffer_dma *dma_buffer,
			       enum dma_data_direction dir);
/**
 * astc_buffer_unmap:
 *
 * called after the task is finished to release all references
 * to the buffers. This is called for every buffer that
 * astc_buffer_map was previously called for.
 */
static void astc_buffer_unmap(struct astc_ctx *ctx,
			       struct astc_buffer_dma *dma_buffer,
			       enum dma_data_direction dir);
/**
 * astc_device_run:
 *
 * sets the HW registers and triggers the HW to start
 * processing a given task.
 *
 * Returns 0 on success, otherwise < 0 err num
 */
static int astc_device_run(struct astc_ctx *astc_context,
			   struct astc_task *task);

/**
 * astc_task_timeout:
 *
 * called when the processing of a given task times out.
 * Currently unused.
 */
static void astc_task_timeout(struct astc_ctx *ctx,
			      struct astc_task *task);


/**
 * astc_task_signal_completion - notify that the task is finished
 *
 * This function wakes up the process that is waiting for the completion of
 * @task. An IRQ handler is the best place to call this function. If @success
 * is false, the state will be updated to reflect that there was an error.
 *
 * Returns negative values if there is an error.
 */
static int astc_task_signal_completion(struct astc_dev *astc_device,
				       struct astc_task *task, bool success);

/**
 * astc_task_cancel - cancel a task and schedule the next task
 *
 * - task: The task that is being cancelled
 * - reason: the reason of canceling the task
 *
 * This function is called to cancel @task and schedule the next task.
 */
static void astc_task_cancel(struct astc_dev *astc_device,
			     struct astc_task *task,
			     enum astc_state reason);

/**
 * astc_deregister_device - deregister the device
 *
 * astc_device - pointer to struct astc_dev
 *
 * Deregister the device that was registered during the probe()
 */
static void astc_deregister_device(struct astc_dev *astc_device);


//let's keep the dump procedure close to the setting procedure, further down
static void dump_config_register(struct astc_dev *device);

static int enable_astc(struct astc_dev *astc);
static int disable_astc(struct astc_dev *astc);


static inline int block_size_enum_to_int(enum astc_encoder_blk_size blk_size)
{
	switch (blk_size) {
	case ASTC_ENCODE_MODE_BLK_SIZE_4x4: return 4;
	case ASTC_ENCODE_MODE_BLK_SIZE_6x6: return 6;
	case ASTC_ENCODE_MODE_BLK_SIZE_8x8: return 8;
	}

	//unsupported
	return -EINVAL;
}

//Compute the size of the buffer we need to fit the ASTC-compressed result,
//given the size of the source image and the size of the blocks used for
//compression
static int compute_num_blocks(struct astc_dev *device
			      , enum astc_encoder_blk_size blk_size
			      , int src_width, int src_height
			      , u16 *blocks_on_x, u16 *blocks_on_y)
{
	u8 x_blk_size = 0;
	u8 y_blk_size = 0;

	if (!blocks_on_x || !blocks_on_y) {
		dev_err(device->dev, "%s: invalid parameters\n", __func__);
		return -EINVAL;
	}

	if (src_width < 0 || src_height < 0) {
		dev_err(device->dev, "%s: invalid image size\n", __func__);
		return -EINVAL;
	}

	//here we assume the check for the max image size has already been
	//performed by the dedicated format validation fun

	x_blk_size = block_size_enum_to_int(blk_size);

	if (x_blk_size <= 0) {
		dev_err(device->dev
			, "%s: unsupported block size enum value %d\n"
			, __func__, blk_size);
		return x_blk_size;
	}

	y_blk_size = x_blk_size;

	//number of blocks on x and y sides, rounded up not to crop the img
	//(int division would yield floor otherwise)
	*blocks_on_x = (src_width  + x_blk_size - 1) / x_blk_size;
	*blocks_on_y = (src_height + y_blk_size - 1) / y_blk_size;

	dev_dbg(device->dev
		, "%s: REQUESTING %u x %u blks of sz %u x %u, img size %d x %d"
		, __func__, *blocks_on_x, *blocks_on_y
		, x_blk_size, y_blk_size
		, src_width, src_height);

	return 0;
}


//Dump the content of all registers, for debugging purposes
static void dump_regs(struct astc_dev *device)
{
	void __iomem *base = device->regs;
	u32 axi_mode = readl(base + ASTC_AXI_MODE_REG);
	dma_addr_t src_addr = readl(base + ASTC_SRC_BASE_ADDR_REG);
	dma_addr_t dst_addr = readl(base + ASTC_DST_BASE_ADDR_REG);

	dev_dbg(device->dev, "Soft reset:       %u\n"
		, readl(base + ASTC_SOFT_RESET_REG) & ASTC_SOFT_RESET_SOFT_RST);
	dump_config_register(device);
	dev_dbg(device->dev, "IRQ enabled:      %u\n"
		, readl(base + ASTC_INT_EN_REG) & ASTC_INT_EN_REG_IRQ_EN);
	dev_dbg(device->dev, "IRQ status:       %u\n"
		, readl(base + ASTC_INT_STAT_REG) & ASTC_INT_STAT_REG_INT_STATUS);

	dev_dbg(device->dev, "AXI write error:  %u\n"
		, readl(base + ASTC_ENC_STAT_REG) & ASTC_ENC_STAT_REG_WR_BUS_ERR);
	dev_dbg(device->dev, "AXI read error:   %u\n"
		, readl(base + ASTC_ENC_STAT_REG) & ASTC_ENC_STAT_REG_RD_BUS_ERR);

	//mask 14bits by &-ing with ASTC_NUM_BLK_REG_FIELD_MASK
	dev_dbg(device->dev, "Image width:      %u\n",
		(readl(base + ASTC_IMG_SIZE_REG) >> ASTC_IMG_SIZE_REG_IMG_SIZE_X_INDEX) & ASTC_NUM_BLK_REG_FIELD_MASK);
	dev_dbg(device->dev, "Image height:     %u\n",
		(readl(base + ASTC_IMG_SIZE_REG) >> ASTC_IMG_SIZE_REG_IMG_SIZE_Y_INDEX) & ASTC_NUM_BLK_REG_FIELD_MASK);

	dev_dbg(device->dev, "Blocks on X:      %u\n",
		(readl(base + ASTC_NUM_BLK_REG) >> ASTC_NUM_BLK_REG_NUM_BLK_X_INDEX) & 0xfff);
	dev_dbg(device->dev, "Blocks on Y:      %u\n",
		(readl(base + ASTC_NUM_BLK_REG) >> ASTC_NUM_BLK_REG_NUM_BLK_Y_INDEX) & 0xfff);

	dev_dbg(device->dev, "DMA Src address:  %pad\n", &src_addr);
	dev_dbg(device->dev, "DMA Dst address:  %pad\n", &dst_addr);
	dev_dbg(device->dev, "Magic number:     %x\n"
		, astc_get_magic_number(base));
	dev_dbg(device->dev, "Encoder started:  %u\n"
		, readl(base + ASTC_ENC_START_REG) & KBit0);
	dev_dbg(device->dev, "Encoder running:  %u\n"
		, astc_hw_is_enc_running(base));

	//We do things a bit differently here, we first select the bits,
	//then shift them to print them out. It's easier because
	//they're small fields
	dev_dbg(device->dev, "AXI WR burst len: %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_WR_BURST_LENGTH_MASK) >> ASTC_AXI_MODE_REG_WR_BURST_LENGTH_INDEX);
	dev_dbg(device->dev, "AXI AWUSER:       %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_AWUSER) >> ASTC_AXI_MODE_REG_AWUSER_INDEX);
	dev_dbg(device->dev, "AXI ARUSER:       %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_ARUSER) >> ASTC_AXI_MODE_REG_ARUSER_INDEX);
	dev_dbg(device->dev, "AXI Write cache:  %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_AWCACHE) >> ASTC_AXI_MODE_REG_AWCACHE_INDEX);
	dev_dbg(device->dev, "AXI Read cache:   %u\n",
		(axi_mode & ASTC_AXI_MODE_REG_ARCACHE) >> ASTC_AXI_MODE_REG_ARCACHE_INDEX);
	dev_dbg(device->dev
		, "HW version:       %x\n", astc_hw_get_version(base));
}

void astc_task_schedule(struct astc_dev *astc_device)
{
	struct astc_task *task;
	unsigned long flags;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

next_task:
	spin_lock_irqsave(&astc_device->lock_task, flags);

	if (list_empty(&astc_device->tasks)) {
		/* No task to run */
		dev_dbg(astc_device->dev
			, "%s: no tasks to run! Returning...\n", __func__);
		goto err;
	}

	dev_dbg(astc_device->dev
		, "%s: INTERRUPT FIRED AND WAITING TO BE HANDLED? %u\n"
		, __func__, astc_hw_get_int_status(astc_device->regs));
	dev_dbg(astc_device->dev, "%s: ENCODING STATUS? %d\n"
		, __func__, get_hw_enc_status(astc_device->regs));

	if (astc_device->current_task) {
		/* H/W is working */
		dev_dbg(astc_device->dev
			, "%s: hw is already processing a task! Returning...\n"
			, __func__);

		goto err;
	}

	if (astc_hw_is_enc_running(astc_device->regs)) {
		dev_dbg(astc_device->dev
			, "%s: hw is still processing! Returning...\n"
			, __func__);
		goto err;
	}

	dev_dbg(astc_device->dev
		, "%s: popping first task from queue...\n", __func__);

	task = list_first_entry(&astc_device->tasks,
				struct astc_task, task_node);
	list_del(&task->task_node);

	astc_device->current_task = task;

	spin_unlock_irqrestore(&astc_device->lock_task, flags);

	task->state = ASTC_BUFSTATE_PROCESSING;
	dev_dbg(astc_device->dev, "%s: about to run the task\n", __func__);

	if (astc_device_run(task->ctx, task)) {
		task->state = ASTC_BUFSTATE_ERROR;

		spin_lock_irqsave(&astc_device->lock_task, flags);
		astc_device->current_task = NULL;
		spin_unlock_irqrestore(&astc_device->lock_task, flags);

		complete(&task->complete);

		goto next_task;
	}

	dev_dbg(astc_device->dev, "%s: END\n", __func__);
	return;

err:
	spin_unlock_irqrestore(&astc_device->lock_task, flags);
}

static int astc_task_signal_completion(struct astc_dev *astc_device,
				       struct astc_task *task, bool success)
{
	unsigned long flags;

	//dereferencing the ptr will produce an oops and the program will be
	//killed, but at least we don't crash the kernel using BUG_ON
	if (!astc_device) {
		pr_err("hwastc: NULL astc device!\n");
		return -EINVAL;
	}
	if (!task) {
		dev_err(astc_device->dev
			, "%s; finishing a NULL task!\n", __func__);
		return -EINVAL;
	}

	spin_lock_irqsave(&astc_device->lock_task, flags);
	dev_dbg(astc_device->dev
		, "%s: resetting current task var...\n", __func__);

	if (!astc_device->current_task) {
		dev_warn(astc_device->dev
			 , "%s: invalid current task!\n", __func__);
	}
	if (astc_device->current_task != task) {
		dev_warn(astc_device->dev
			 , "%s: finishing an unexpected task!\n", __func__);
	}

	astc_device->current_task = NULL;
	spin_unlock_irqrestore(&astc_device->lock_task, flags);

	task->state = success ? ASTC_BUFSTATE_DONE : ASTC_BUFSTATE_ERROR;

	dev_dbg(astc_device->dev
		, "%s: signalling task completion...\n", __func__);

	//wake up the task processing function and let it return
	complete(&task->complete);

	return 0;
}

static void astc_task_cancel(struct astc_dev *astc_device,
			     struct astc_task *task,
			     enum astc_state reason)
{
	unsigned long flags;

	spin_lock_irqsave(&astc_device->lock_task, flags);
	dev_dbg(astc_device->dev
		, "%s: resetting current task var...\n", __func__);

	astc_device->current_task = NULL;
	spin_unlock_irqrestore(&astc_device->lock_task, flags);

	task->state = reason;

	dev_dbg(astc_device->dev
		, "%s: signalling task completion...\n", __func__);

	complete(&task->complete);

	dev_dbg(astc_device->dev, "%s: scheduling a new task...\n", __func__);

	astc_task_schedule(astc_device);
}

/**
 * Find the VMA that holds the input virtual address, and check
 * if it's associated to a fd representing a DMA buffer.
 * If it is, acquire that dma buffer (i.e. increase refcount on its fd).
 */
static struct dma_buf *astc_get_dmabuf_from_userptr(
		struct astc_dev *astc_device, unsigned long start, size_t len,
		off_t *out_offset)
{
	struct dma_buf *dmabuf = NULL;
	struct vm_area_struct *vma;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	down_read(&current->mm->mmap_sem);
	dev_dbg(astc_device->dev
		, "%s: virtual address space semaphore acquired\n", __func__);

	vma = find_vma(current->mm, start);

	if (!vma || (start < vma->vm_start)) {
		dev_err(astc_device->dev
			, "%s: Incorrect user buffer @ %#lx/%#zx\n"
			, __func__, start, len);
		dmabuf = ERR_PTR(-EINVAL);
		goto finish;
	}

	dev_dbg(astc_device->dev
		, "%s: VMA found, %p. Starting at %#lx within parent area\n"
		, __func__, vma, vma->vm_start);

	//vm_file is the file that this vma is a memory mapping of
	if (!vma->vm_file) {
		dev_dbg(astc_device->dev, "%s: this VMA does not map a file\n",
			__func__);
		goto finish;
	}

	dmabuf = get_dma_buf_file(vma->vm_file);
	dev_dbg(astc_device->dev
		, "%s: got dmabuf %p of vm_file %p\n", __func__, dmabuf
		, vma->vm_file);

	if (dmabuf != NULL)
		*out_offset = start - vma->vm_start;
finish:
	up_read(&current->mm->mmap_sem);
	dev_dbg(astc_device->dev
		, "%s: virtual address space semaphore released\n", __func__);

	return dmabuf;
}

/**
 * If the buffer we received from userspace is an fd representing a DMA buffer,
 * (HWASTC_BUFFER_DMABUF) then we attach to it.
 *
 * If it's a pointer to user memory (HWASTC_BUFFER_USERPTR), then we check if
 * that memory is associated to a dmabuf. If it is, we attach to it, thus
 * reusing that dmabuf.
 *
 * If it's just a pointer to user memory with no dmabuf associated to it, we
 * don't need to do anything here. We will use IOVMM later to get access to it.
 */
static int astc_buffer_get_and_attach(struct astc_dev *astc_device,
				      struct hwASTC_buffer *buffer,
				      struct astc_buffer_dma *dma_buffer)
{
	struct astc_buffer_plane_dma *plane;
	int ret = 0; //PTR_ERR returns a long
	off_t offset = 0;

	dev_dbg(astc_device->dev
		, "%s: BEGIN, acquiring plane from buf %p into dma buffer %p\n"
		, __func__, buffer, dma_buffer);

	plane = &dma_buffer->plane;

	if (buffer->type == HWASTC_BUFFER_DMABUF) {
		plane->dmabuf = dma_buf_get(buffer->fd);

		dev_dbg(astc_device->dev, "%s: dmabuf of fd %d is %p\n", __func__
			, buffer->fd, plane->dmabuf);
	} else if (buffer->type == HWASTC_BUFFER_USERPTR) {
		// Check if there's already a dmabuf associated to the
		// chunk of user memory our client is pointing us to
		plane->dmabuf = astc_get_dmabuf_from_userptr(astc_device,
							     buffer->userptr,
							     buffer->len,
							     &offset);
		dev_dbg(astc_device->dev, "%s: dmabuf of userptr %p is %p\n"
			, __func__, (void *) buffer->userptr, plane->dmabuf);
	}

	if (IS_ERR(plane->dmabuf)) {
		ret = PTR_ERR(plane->dmabuf);
		dev_err(astc_device->dev,
			"%s: failed to get dmabuf, err %d\n", __func__, ret);
		goto err;
	}

	// this is NULL when the buffer is of type HWASTC_BUFFER_USERPTR
	// but the memory it points to is not associated to a dmabuf
	if (plane->dmabuf) {
		if (plane->dmabuf->size < plane->bytes_used) {
			dev_err(astc_device->dev,
				"%s: driver requires %zu bytes but user gave %zu\n",
				__func__, plane->bytes_used,
				plane->dmabuf->size);
			ret = -EINVAL;
			goto err;
		}

		plane->attachment = dma_buf_attach(plane->dmabuf
						   , astc_device->dev);

		dev_dbg(astc_device->dev
			, "%s: attachment of dmabuf %p is %p\n", __func__
			, plane->dmabuf, plane->attachment);

		plane->offset = offset;

		if (IS_ERR(plane->attachment)) {
			dev_err(astc_device->dev,
				"%s: Failed to attach dmabuf\n", __func__);
			ret = PTR_ERR(plane->attachment);
			goto err;
		}
	}

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
err:
	dev_dbg(astc_device->dev
		, "%s: ERROR releasing dma resources\n", __func__);

	if (!IS_ERR(plane->dmabuf)) /* release dmabuf */
		dma_buf_put(plane->dmabuf);

	// NOTE: attach was not reached or did not complete,
	//       so no need to detach here

	return ret;
}

/**
 * This should be called after unmapping the dma buffer.
 *
 * We detach the device from the buffer to signal that we're not
 * interested in it anymore, and we use "put" to decrease the usage
 * refcount of the dmabuf struct.
 * Detach is at the end of the function name just to make it easier
 * to associate astc_buffer_get_* to astc_buffer_put_*, but  it's
 * actually called in the inverse order.
 */
static void astc_buffer_put_and_detach(struct astc_buffer_dma *dma_buffer)
{
	const struct hwASTC_buffer *user_buffer = dma_buffer->buffer;
	struct astc_buffer_plane_dma *plane = &dma_buffer->plane;

	// - buffer type DMABUF:
	//     in this case dmabuf should always be set
	// - buffer type USERPTR:
	//     in this case dmabuf is only set if we reuse any
	//     preexisting dmabuf (see astc_buffer_get_*)
	if (user_buffer->type == HWASTC_BUFFER_DMABUF
			|| (user_buffer->type == HWASTC_BUFFER_USERPTR
			    && plane->dmabuf)) {
		dma_buf_detach(plane->dmabuf, plane->attachment);
		dma_buf_put(plane->dmabuf);
		plane->dmabuf = NULL;
	}
}

/**
 * This is the entry point for buffer dismantling.
 *
 * It is used whenever we need to dismantle a buffer that had been fully
 * setup, for instance after we finish processing a task, independently of
 * whether the processing was successful or not.
 *
 * It unmaps the buffer, detaches the device from it, and releases it.
 */
static void astc_buffer_teardown(struct astc_dev *astc_device,
				       struct astc_ctx *ctx,
				       struct astc_buffer_dma *dma_buffer,
				       enum dma_data_direction dir)
{
	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	astc_buffer_unmap(ctx, dma_buffer, dir);

	astc_buffer_put_and_detach(dma_buffer);

	dev_dbg(astc_device->dev, "%s: END\n", __func__);
}

/**
 * This is the entry point for complete buffer setup.
 *
 * It does the necessary mapping and allocation of the DMA buffer that
 * the HW will access.
 *
 * At the end of this function, the buffer is ready for DMA transfers.
 *
 * Note: this is needed when the input is a HWASTC_BUFFER_DMABUF as well
 *       as when it is a HWASTC_BUFFER_USERPTR, because the H/W ultimately
 *       needs a DMA address to operate on.
 */
static int astc_buffer_setup(struct astc_ctx *ctx,
			     struct hwASTC_buffer *buffer,
			     struct astc_buffer_dma *dma_buffer,
			     enum dma_data_direction dir)
{
	struct astc_dev *astc_device = ctx->astc_dev;
	int ret = 0;
	struct astc_buffer_plane_dma *plane = &dma_buffer->plane;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	dev_dbg(astc_device->dev
		, "%s: computing bytes needed for dma buffer %p\n"
		, __func__, dma_buffer);

	if (plane->bytes_used == 0) {
		dev_err(astc_device->dev,
			"%s: BUG! driver expects 0 bytes! (user gave %zu)\n",
			__func__, buffer->len);
		return -EINVAL;

	} else if (buffer->len < plane->bytes_used) {
		dev_err(astc_device->dev,
			"%s: driver expects %zu bytes but user gave %zu\n",
			__func__, plane->bytes_used,
			buffer->len);
		return -EINVAL;
	}

	if ((buffer->type != HWASTC_BUFFER_USERPTR) &&
			(buffer->type != HWASTC_BUFFER_DMABUF)) {
		dev_err(astc_device->dev, "%s: unknown buffer type %u\n",
			__func__, buffer->type);
		return -EINVAL;
	}

	ret = astc_buffer_get_and_attach(astc_device, buffer, dma_buffer);

	if (ret)
		return ret;

	dma_buffer->buffer = buffer;

	dev_dbg(astc_device->dev
		, "%s: about to prepare buffer %p, dir DMA_TO_DEVICE? %d\n"
		, __func__, dma_buffer, dir == DMA_TO_DEVICE);

	/* the callback function should fill 'dma_addr' field */
	ret = astc_buffer_map(ctx, dma_buffer, dir == DMA_TO_DEVICE);
	if (ret) {
		dev_err(astc_device->dev, "%s: Failed to prepare plane"
			, __func__);

		dev_dbg(astc_device->dev, "%s: releasing buffer %p\n"
			, __func__, dma_buffer);

		astc_buffer_put_and_detach(dma_buffer);

		return ret;
	}

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
}


/**
 * Check validity of source/dest formats, i.e. image size and pixel format.
 *
 * dma_data_direction is used to distinguish between the source/dest formats.
 *
 * When the DMA direction is DMA_TO_DEVICE, we also check that the source
 * pixel format is something the H/W can handle, and we save the number of bits
 * per pixel needed to handle that format in the variable referenced by the
 * srcBitsPerPixel pointer.
 */
static int astc_validate_format(struct astc_ctx *ctx,
				struct astc_task *task,
				enum dma_data_direction dir,
				int *srcBitsPerPixel)
{
	struct hwASTC_pix_format *fmt = NULL;

	if (!ctx) {
		dev_err(ctx->astc_dev->dev, "%s: invalid context!\n", __func__);
		return -EINVAL;
	}
	if (!task) {
		dev_err(ctx->astc_dev->dev
			, "%s: invalid task ptr!\n", __func__);
		return -EINVAL;
	}

	//handling the buffer holding the encoding result
	if (dir == DMA_FROM_DEVICE) {
		fmt = &task->user_task.fmt_cap;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_FROM_DEVICE case, width %u height %u\n"
			, __func__, fmt->width, fmt->height);

		// no need to check the pixel format, we know the result
		// will be ASTC

	} else if (dir == DMA_TO_DEVICE) {
		fmt = &task->user_task.fmt_out;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_TO_DEVICE case, width %u height %u\n"
			, __func__, fmt->width, fmt->height);

		if (!srcBitsPerPixel) {
			dev_err(ctx->astc_dev->dev
				, "%s: invalid ptr to bits per pixel var!\n"
				, __func__);
			return -EINVAL;
		}

		//Check validity of the source pixel format
		switch (fmt->fmt) {
		case ASTC_ENCODE_INPUT_PIXEL_FORMAT_RGBA8888:  /* RGBA 8888 */
		case ASTC_ENCODE_INPUT_PIXEL_FORMAT_ARGB8888:  /* ARGB 8888 */
		case ASTC_ENCODE_INPUT_PIXEL_FORMAT_BGRA8888:  /* BGRA 8888 */
		case ASTC_ENCODE_INPUT_PIXEL_FORMAT_ABGR8888:  /* ABGR 8888 */
			*srcBitsPerPixel = 4;
			break;
		default:
			dev_err(ctx->astc_dev->dev
				, "invalid input color format (%u)", fmt->fmt);
			return -EINVAL;
		}

	} else {
		//this shouldn't happen
		dev_err(ctx->astc_dev->dev, "%s: invalid DMA direction\n",
			__func__);
		return -EINVAL;
	}

	if (fmt->width < ASTC_INPUT_IMAGE_MIN_SIZE ||
			fmt->height < ASTC_INPUT_IMAGE_MIN_SIZE ||
			fmt->width > ASTC_INPUT_IMAGE_MAX_SIZE ||
			fmt->height > ASTC_INPUT_IMAGE_MAX_SIZE) {
		dev_err(ctx->astc_dev->dev
			, "%s: invalid size of width or height (%u, %u)\n",
			__func__, fmt->width, fmt->height);
		return -EINVAL;
	}

	return 0;
}

static int astc_compute_buffer_size(struct astc_ctx *ctx,
				    struct astc_task *task,
				    enum dma_data_direction dir,
				    size_t *bytes_used,
				    int srcBitsPerPixel)
{
	dev_dbg(ctx->astc_dev->dev, "%s: BEGIN\n", __func__);

	if (!ctx) {
		dev_err(ctx->astc_dev->dev, "%s: invalid context!\n", __func__);
		return -EINVAL;
	}
	if (!task) {
		dev_err(ctx->astc_dev->dev
			, "%s: invalid task ptr!\n", __func__);
		return -EINVAL;
	}
	if (!bytes_used) {
		dev_err(ctx->astc_dev->dev
			, "%s: invalid ptr to requested buffer size!\n"
			, __func__);
		return -EINVAL;
	}


	//handling the buffer holding the encoding result
	if (dir == DMA_FROM_DEVICE) {
		int bytes_needed = 0;
		struct hwASTC_pix_format *fmt = &task->user_task.fmt_cap;

		int res = compute_num_blocks(ctx->astc_dev
					     , task->user_task.enc_config.encodeBlockSize
					     , fmt->width
					     , fmt->height
					     , &ctx->blocks_on_x
					     , &ctx->blocks_on_y);

		if (res) {
			dev_err(ctx->astc_dev->dev,
				"%s: failed to compute num of blocks needed\n"
				, __func__);
			return -EINVAL;
		}

		//For the output we need 16 bytes per block, + the astc header
		//NOTE: hw currently only supports 2d astc encoding,
		//      otherwise we'd need to take blocks on Z axis into
		//      account as well
		bytes_needed = ctx->blocks_on_x * ctx->blocks_on_y
				* ASTC_BYTES_PER_BLOCK
				+ ASTC_HEADER_BYTES;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_FROM_DEVICE, width %u height %u\n"
			, __func__, fmt->width, fmt->height);

		if (bytes_needed <= 0) {
			dev_err(ctx->astc_dev->dev,
				"%s: invalid output buffer size\n",
				__func__);
			return -EINVAL;
		}
		*bytes_used = bytes_needed;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_FROM_DEVICE, requesting %zu bytes\n"
			, __func__, *bytes_used);

	} else if (dir == DMA_TO_DEVICE) {
		u8 x_blk_size  = 0;
		u8 y_blk_size  = 0;
		uint newWidth  = 0;
		uint newHeight = 0;
		struct hwASTC_pix_format *fmt = &task->user_task.fmt_out;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_TO_DEVICE case, width %u height %u\n"
			, __func__, fmt->width, fmt->height);

		x_blk_size = block_size_enum_to_int(task->user_task.enc_config.encodeBlockSize);

		if (x_blk_size <= 0) {
			dev_err(ctx->astc_dev->dev, "Invalid block size %d\n",
				task->user_task.enc_config.encodeBlockSize);
			return x_blk_size;
		}

		//we only support same size x and y block sizes
		y_blk_size = x_blk_size;

		// !!!!! TODO REMOVE !!!!!
		//This is a workaround that will be not needed anymore with the
		//next version of the hardware (year 2018?)
		//
		//Explanation:
		//according to the HW engineer, the HW will access out of bounds when
		//the number of blocks on a side needs rounding up.
		//So we allocate a bigger buffer, but still pass the original
		//size of the image, so that the hw can access out of bounds,
		//even though it won't use that data.
		newWidth = ((fmt->width + (x_blk_size - 1)) / x_blk_size)
				* x_blk_size;
		newHeight = ((fmt->height + (y_blk_size - 1)) / y_blk_size)
				* y_blk_size;

		//If image size is not a multiple of the requested block size...
		if (newWidth != fmt->width || newHeight != fmt->height) {
			dev_dbg(ctx->astc_dev->dev
				, "!!!!! HACK: requesting a bigger buffer so that the HW can access out of bounds\n"
				  "Old size %d x %d, new size %d x %d\n"
				, fmt->width, fmt->height, newWidth, newHeight);
		}

		/* check that we can handle the input image */
		if (newWidth < ASTC_INPUT_IMAGE_MIN_SIZE ||
				newHeight < ASTC_INPUT_IMAGE_MIN_SIZE ||
				newWidth > ASTC_INPUT_IMAGE_MAX_SIZE ||
				newHeight > ASTC_INPUT_IMAGE_MAX_SIZE) {
			dev_err(ctx->astc_dev->dev,
				"Invalid size of width or height of the extended buffer (%u, %u)\n",
				newWidth, newHeight);
			return -EINVAL;
		}

		*bytes_used = newWidth * newHeight * srcBitsPerPixel;
		// *bytes_used = fmt->width * fmt->height * srcBitsPerPixel;

		dev_dbg(ctx->astc_dev->dev
			, "%s: DMA_TO_DEVICE, requesting %zu bytes\n"
			, __func__, *bytes_used);

	} else {
		//this shouldn't happen
		dev_err(ctx->astc_dev->dev,
			"%s: invalid DMA direction\n",
			__func__);
		return -EINVAL;
	}

	dev_dbg(ctx->astc_dev->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * Validates the image formats (size, pixel formats) provided by userspace and
 * computes the size of the DMA buffers required to be able to process the task.
 *
 * See description of astc_compute_buffer_size for more info.
 */
static int astc_prepare_formats(struct astc_dev *astc_device,
				struct astc_ctx *ctx,
				struct astc_task *task)
{
	int ret = 0;
	size_t out_size = 0;
	size_t cap_size = 0;
	int srcBitsPerPixel = 0;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	dev_dbg(ctx->astc_dev->dev, "%s: Validating format...\n", __func__);

	ret = astc_validate_format(ctx, task, DMA_TO_DEVICE, &srcBitsPerPixel);
	if (ret < 0)
		return ret;

	ret = astc_compute_buffer_size(ctx, task, DMA_TO_DEVICE, &out_size
				       , srcBitsPerPixel);
	if (ret < 0)
		return ret;

	dev_dbg(astc_device->dev, "%s: input buffer, plane requires %zu bytes\n"
		, __func__,  out_size);

	task->dma_buf_out.plane.bytes_used = out_size;

	dev_dbg(astc_device->dev
		, "%s: hardcoding result img height/width to be same as input\n"
		, __func__);

	//set the destination img width/height even though it's the same
	//as the input img size, as the other procedures need it to compute
	//the blocks-per-side and similar
	task->user_task.fmt_cap.width  = task->user_task.fmt_out.width;
	task->user_task.fmt_cap.height = task->user_task.fmt_out.height;

	ret = astc_validate_format(ctx, task, DMA_FROM_DEVICE, NULL);
	if (ret < 0)
		return ret;

	ret = astc_compute_buffer_size(ctx, task, DMA_FROM_DEVICE, &cap_size
				       , srcBitsPerPixel);
	if (ret < 0)
		return ret;

	dev_dbg(astc_device->dev, "%s: dest buffer plane requires %zu bytes\n"
		, __func__, cap_size);

	task->dma_buf_cap.plane.bytes_used = cap_size;

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * Main entry point for preparing a task before it can be processed.
 *
 * Checks the validity of requested formats/sizes and sets the buffers up.
 */
static int astc_task_setup(struct astc_dev *astc_device,
			     struct astc_ctx *ctx,
			     struct astc_task *task)
{
	int ret;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	HWASTC_PROFILE(ret = astc_prepare_formats(astc_device, ctx, task),
		       "PREPARE FORMATS TIME", astc_device->dev);
	if (ret)
		return ret;

	dev_dbg(astc_device->dev, "%s: OUT buf, USER GAVE %zu WE REQUIRE %zu\n"
		, __func__,  task->user_task.buf_out.len
		, task->dma_buf_out.plane.bytes_used);

	HWASTC_PROFILE(ret = astc_buffer_setup(ctx, &task->user_task.buf_out,
				      &task->dma_buf_out, DMA_TO_DEVICE),
		       "OUT BUFFER SETUP TIME", astc_device->dev);
	if (ret) {
		dev_err(astc_device->dev, "%s: Failed to get output buffer\n",
			__func__);
		return ret;
	}

	dev_dbg(astc_device->dev, "%s: CAP buf, USER GAVE %zu WE REQUIRE %zu\n"
		, __func__,  task->user_task.buf_cap.len
		, task->dma_buf_cap.plane.bytes_used);

	HWASTC_PROFILE(ret = astc_buffer_setup(ctx, &task->user_task.buf_cap,
				  &task->dma_buf_cap, DMA_FROM_DEVICE),
		       "CAP BUFFER SETUP TIME", astc_device->dev);
	if (ret) {
		astc_buffer_teardown(astc_device, ctx,
				     &task->dma_buf_out, DMA_TO_DEVICE);
		dev_err(astc_device->dev, "%s: Failed to get capture buffer\n",
			__func__);
		return ret;
	}

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * Main entry point for task teardown.
 *
 * It takes care of doing all that is needed to teardown the buffers
 * that were used to process the task.
 */
static void astc_task_teardown(struct astc_dev *astc_dev,
			     struct astc_ctx *ctx,
			     struct astc_task *task)
{
	dev_dbg(astc_dev->dev, "%s: BEGIN\n", __func__);

	astc_buffer_teardown(astc_dev, ctx, &task->dma_buf_cap,
			     DMA_FROM_DEVICE);

	astc_buffer_teardown(astc_dev, ctx, &task->dma_buf_out,
			     DMA_TO_DEVICE);

	dev_dbg(astc_dev->dev, "%s: END\n", __func__);
}

static void astc_destroy_context(struct kref *kreference)
{
	unsigned long flags;
	//^^ this function is called with ctx->kref as parameter,
	//so we can use ptr arithmetics to go back to ctx
	struct astc_ctx *ctx = container_of(kreference,
					    struct astc_ctx, kref);
	struct astc_dev *astc_device = ctx->astc_dev;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	spin_lock_irqsave(&astc_device->lock_ctx, flags);
	dev_dbg(astc_device->dev, "%s: deleting context %p from list\n"
		, __func__, ctx);
	list_del(&ctx->node);
	spin_unlock_irqrestore(&astc_device->lock_ctx, flags);

	kfree(ctx);

	dev_dbg(astc_device->dev, "%s: END\n", __func__);
}

/**
 * Main task processing function.
 * It takes care of processing a task, from setup to teardown.
 *
 * It performs the necessary validations and setup of buffers, then
 * wakes up the device, schedules the task for execution and waits
 * for the H/W to signal completion.
 *
 * After the task is finished (independently of whether it was
 * successful or not), it releases all the resources and powers
 * the device down.
 */
static int astc_process(struct astc_ctx *ctx,
			struct astc_task *task)
{
	struct astc_dev *astc_device = ctx->astc_dev;
	unsigned long flags;
	int ret = 0;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	//XXX: Disable DMA code paths until they're properly tested
	if (task->user_task.buf_out.type == HWASTC_BUFFER_DMABUF
			|| task->user_task.buf_cap.type == HWASTC_BUFFER_DMABUF) {
		dev_err(astc_device->dev,
			"%s: Failed to process task, the driver does not currently support DMA addresses.\n"
			, __func__);
		return -EINVAL;
	}

	INIT_LIST_HEAD(&task->task_node);
	init_completion(&task->complete);

	dev_dbg(astc_device->dev, "%s: acquiring kref of ctx %p\n",
		__func__, ctx);
	kref_get(&ctx->kref);

	mutex_lock(&ctx->mutex);

	dev_dbg(astc_device->dev, "%s: inside context lock (ctx %p task %p)\n"
		, __func__, ctx, task);

	ret = astc_task_setup(astc_device, ctx, task);

	if (ret)
		goto err_prepare_task;

	HWASTC_PROFILE(

	task->ctx = ctx;
	task->state = ASTC_BUFSTATE_READY;

	spin_lock_irqsave(&astc_device->slock, flags);
	if (test_bit(DEV_RUN, &astc_device->state)) {
		/* this will happen when multiple processes encode at the same
		 * time. They all get different contexts because they use
		 * different FDs, so they can get inside this section which is
		 * locked by a context-specific mutex, but when they get here
		 * they discover another process is already using the device.
		 */
		dev_dbg(astc_device->dev
			 , "%s: device state is marked as running before a task is run (ctx %p task %p)\n"
			 , __func__, ctx, task);
	}

	set_bit(DEV_RUN, &astc_device->state);
	spin_unlock_irqrestore(&astc_device->slock, flags);

	ret = enable_astc(astc_device);
	if (ret) {
		spin_lock_irqsave(&astc_device->slock, flags);
		set_bit(DEV_SUSPEND, &astc_device->state);
		spin_unlock_irqrestore(&astc_device->slock, flags);

		goto err_power;
	}

	spin_lock_irqsave(&astc_device->lock_task, flags);
	dev_dbg(astc_device->dev, "%s: adding task %p to list (ctx %p)\n"
		, __func__, task, ctx);
	list_add_tail(&task->task_node, &astc_device->tasks);
	spin_unlock_irqrestore(&astc_device->lock_task, flags);

	, "BETWEEN SETUP AND SCHEDULE", astc_device->dev);

	HWASTC_PROFILE(astc_task_schedule(astc_device),
		       "SCHEDULE TIME", astc_device->dev);

	if (astc_device->timeout_jiffies != -1) {
		unsigned long elapsed;

		dev_dbg(astc_device->dev
			, "%s: waiting for task to complete before timeout\n"
			, __func__);

		//perform an uninterruptible wait (i.e. ignore signals)
		//NOTE: this call can sleep, so it cannot be used in IRQ context
		elapsed = wait_for_completion_timeout(&task->complete,
						      astc_device->timeout_jiffies);
		if (!elapsed) { /* timed out */
			astc_task_cancel(astc_device, task,
					 ASTC_BUFSTATE_TIMEDOUT);

			astc_task_timeout(ctx, task);

			dev_notice(astc_device->dev, "%s: %u msecs timed out\n",
				   __func__,
				   jiffies_to_msecs(astc_device->timeout_jiffies));
			ret = -ETIMEDOUT;
		}
	} else {
		dev_dbg(astc_device->dev
			, "%s: waiting for task to complete, no timeout\n"
			, __func__);

		//perform an uninterruptible wait (i.e. ignore signals)
		//NOTE: this call can sleep, so it cannot be used in IRQ context
		wait_for_completion(&task->complete);
	}

	if (task->state == ASTC_BUFSTATE_READY) {
		dev_err(astc_device->dev
			, "%s: invalid task state after task completion\n"
			, __func__);
	}

	pm_runtime_mark_last_busy(astc_device->dev);

	HWASTC_PROFILE(disable_astc(astc_device), "DISABLE ASTC", astc_device->dev);

err_power:
	HWASTC_PROFILE(astc_task_teardown(astc_device, ctx, task),
		       "TASK TEARDOWN TIME",
		       astc_device->dev);

err_prepare_task:
	HWASTC_PROFILE(

	dev_dbg(astc_device->dev, "%s: releasing lock of ctx %p\n"
		, __func__, ctx);
	mutex_unlock(&ctx->mutex);

	dev_dbg(astc_device->dev, "%s: releasing kref of ctx %p\n"
		, __func__, ctx);

	kref_put(&ctx->kref, astc_destroy_context);

	dev_dbg(astc_device->dev, "%s: returning %d, task state %d\n"
		, __func__, ret, task->state);

	, "FINISH PROCESS", astc_device->dev);

	if (ret)
		return ret;
	else
		return (task->state == ASTC_BUFSTATE_DONE) ? 0 : -EINVAL;
}

/**
 * Handle dev node open calls.
 * Create a context and initialize context-related resources.
 *
 * A new context is created each time the device is opened.
 *
 * As a consequence, if a userspace client uses more threads to
 * enqueue separate encoding tasks using the SAME dev node fd, those
 * tasks will be sharing the same context (because threads in the same
 * process share the fd table, so only one open call will be needed).
 */
static int astc_open(struct inode *inode, struct file *filp)
{

	unsigned long flags;
	//filp->private_data holds astc_dev->misc (misc_open sets it)
	//this uses pointers arithmetics to go back to astc_dev
	struct astc_dev *astc_device = container_of(filp->private_data,
						    struct astc_dev, misc);
	struct astc_ctx *ctx;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);
	// GFP_KERNEL is used when kzalloc is allowed to sleep while
	// the memory we're requesting is not immediately available
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	INIT_LIST_HEAD(&ctx->node);
	kref_init(&ctx->kref);
	mutex_init(&ctx->mutex);

	ctx->astc_dev = astc_device;

	spin_lock_irqsave(&astc_device->lock_ctx, flags);
	dev_dbg(astc_device->dev, "%s: adding new context %p to contexts list\n"
		, __func__, ctx);
	list_add_tail(&ctx->node, &astc_device->contexts);
	spin_unlock_irqrestore(&astc_device->lock_ctx, flags);

	filp->private_data = ctx;

	dev_dbg(astc_device->dev, "%s: END\n", __func__);

	return 0;
}

static int astc_release(struct inode *inode, struct file *filp)
{
	struct astc_ctx *ctx = filp->private_data;

	dev_dbg(ctx->astc_dev->dev, "%s: releasing context %p\n"
		, __func__, ctx);

	//the release callback will not be invoked if counter
	//is already 0 before this line
	kref_put(&ctx->kref, astc_destroy_context);

	return 0;
}

/**
 * Handle ioctl calls coming from userspace.
 */
static long astc_ioctl(struct file *filp,
		       unsigned int cmd, unsigned long arg)
{
	struct astc_ctx *ctx = filp->private_data;
	struct astc_dev *astc_device = ctx->astc_dev;

	dev_dbg(astc_device->dev, "%s: BEGIN\n", __func__);

	switch (cmd) {
	case HWASTC_IOC_PROCESS:
	{
		struct astc_task data;
		int ret;

		dev_dbg(astc_device->dev, "%s: PROCESS ioctl received\n"
			, __func__);

		memset(&data, 0, sizeof(data));

		if (copy_from_user(&data.user_task
				   , (void __user *)arg
				   , sizeof(data.user_task))) {
			dev_err(astc_device->dev,
				"%s: Failed to read userdata\n", __func__);
			return -EFAULT;
		}

		//ensure the buffers are in userspace
		if (data.user_task.buf_out.type == HWASTC_BUFFER_USERPTR &&
				!access_ok(VERIFY_READ,
					      (void __user *) data.user_task.buf_out.userptr,
					      data.user_task.buf_out.len)) {
			dev_err(astc_device->dev,
				"%s: no access to src buffer\n", __func__);
			return -EFAULT;
		}

		if (data.user_task.buf_cap.type == HWASTC_BUFFER_USERPTR &&
				!access_ok(VERIFY_WRITE,
					      (void __user *) data.user_task.buf_cap.userptr,
					      data.user_task.buf_cap.len)) {
			dev_err(astc_device->dev,
				"%s: no access to result buffer\n", __func__);
			return -EFAULT;
		}

		dev_dbg(astc_device->dev
			, "%s: user data copied, now launching processing...\n"
			, __func__);

		/*
		 * astc_process() does not wake up
		 * until the given task finishes
		 */

		HWASTC_PROFILE(ret = astc_process(ctx, &data),
			       "WHOLE PROCESS TIME",
			       astc_device->dev);

		dev_dbg(astc_device->dev
			, "%s: processing done! Copying data back to user space...\n", __func__);

		if (ret) {
			dev_err(astc_device->dev,
				"%s: Failed to process task, error %d\n"
				, __func__, ret);
			return ret;
		}

		// No need to copy the task structure back to userspace,
		// as we did not modify it.
		// The encoding result is at the address we received from
		// the user.

		return ret;
	}
	default:
		dev_err(astc_device->dev, "%s: Unknown ioctl cmd %x\n",
			__func__, cmd);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_COMPAT

struct compat_astc_pix_format {
	compat_uint_t fmt;
	compat_uint_t width;
	compat_uint_t height;
};

struct compat_astc_buffer {
	union {
		compat_int_t fd;
		compat_ulong_t userptr;
	};
	compat_size_t len;
	__u8 type;
};

struct compat_astc {
	struct compat_astc_pix_format fmt_out;
	struct compat_astc_pix_format fmt_cap;
	struct compat_astc_buffer buf_out;
	struct compat_astc_buffer buf_cap;
	//layout of astc_config is crafted to be the same on 32/64bits,
	//so no need for a separate compat struct
	struct astc_config enc_config;
	compat_ulong_t reserved[2];
};

// astc_config struct  only uses elements with fixed width and it
// has explicit padding, so no compat structure should be needed

#define COMPAT_HWASTC_IOC_PROCESS	_IOWR('M',   0, struct compat_astc)

static long astc_compat_ioctl32(struct file *filp,
				unsigned int cmd, unsigned long arg)
{
	struct astc_ctx *ctx = filp->private_data;
	struct astc_dev *astc_device = ctx->astc_dev;

	switch (cmd) {
	case COMPAT_HWASTC_IOC_PROCESS:
	{
		struct compat_astc data;
		struct astc_task task;
		int ret;

		memset(&task, 0, sizeof(task));

		if (copy_from_user(&data, compat_ptr(arg), sizeof(data))) {
			dev_err(astc_device->dev,
				"%s: Failed to read userdata\n", __func__);
			return -EFAULT;
		}

		task.user_task.fmt_out.fmt = data.fmt_out.fmt;
		task.user_task.fmt_out.width = data.fmt_out.width;
		task.user_task.fmt_out.height = data.fmt_out.height;
		task.user_task.fmt_cap.fmt = data.fmt_cap.fmt;
		task.user_task.fmt_cap.width = data.fmt_cap.width;
		task.user_task.fmt_cap.height = data.fmt_cap.height;

		task.user_task.buf_out.len = data.buf_out.len;
		if (data.buf_out.type == HWASTC_BUFFER_DMABUF)
			task.user_task.buf_out.fd =
					data.buf_out.fd;
		else /* data.buf_out.type == HWASTC_BUFFER_USERPTR */
			task.user_task.buf_out.userptr =
					data.buf_out.userptr;
		task.user_task.buf_out.type = data.buf_out.type;

		task.user_task.buf_cap.len =
				data.buf_cap.len;
		if (data.buf_cap.type == HWASTC_BUFFER_DMABUF)
			task.user_task.buf_cap.fd =
					data.buf_cap.fd;
		else /* data.buf_cap.type == HWASTC_BUFFER_USERPTR */
			task.user_task.buf_cap.userptr =
					data.buf_cap.userptr;
		task.user_task.buf_cap.type = data.buf_cap.type;

		task.user_task.enc_config = data.enc_config;

		task.user_task.reserved[0] = data.reserved[0];
		task.user_task.reserved[1] = data.reserved[1];

		//ensure the buffers are in userspace
		if (task.user_task.buf_out.type == HWASTC_BUFFER_USERPTR
				&& !access_ok(VERIFY_READ,
					      (void __user *) task.user_task.buf_out.userptr,
					      task.user_task.buf_out.len)) {
			dev_err(astc_device->dev,
				"%s: no access to src buffer\n", __func__);
			return -EFAULT;
		}

		if (task.user_task.buf_cap.type == HWASTC_BUFFER_USERPTR
				&& !access_ok(VERIFY_WRITE,
					      (void __user *) task.user_task.buf_cap.userptr,
					      task.user_task.buf_cap.len)) {
			dev_err(astc_device->dev,
				"%s: no access to result buffer\n", __func__);
			return -EFAULT;
		}

		/*
		 * astc_process() does not wake up
		 * until the given task finishes
		 */
		ret = astc_process(ctx, &task);
		if (ret) {
			dev_err(astc_device->dev,
				"%s: Failed to process hwASTC_task task\n",
				__func__);
			return ret;
		}

		// No need to copy the task structure back to userspace,
		// as we did not modify it.
		// The encoding result is at the address we received from
		// the user.

		return 0;
	}
	default:
		dev_err(astc_device->dev, "%s: Unknown compat_ioctl cmd %x\n",
			__func__, cmd);
		return -EINVAL;
	}

	return 0;
}
#endif

static const struct file_operations astc_fops = {
	.owner          = THIS_MODULE,
	.open           = astc_open,
	.release        = astc_release,
	.unlocked_ioctl	= astc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= astc_compat_ioctl32,
#endif
};

static void astc_deregister_device(struct astc_dev *astc_device)
{
	dev_info(astc_device->dev, "%s: deregistering astc device\n", __func__);
	misc_deregister(&astc_device->misc);
}

/**
 * Set the H/W register representing the DMA address of the source buffer.
 */
static int astc_set_source(struct astc_dev *astc_dev,
			   struct astc_task *task)
{
	dma_addr_t addr = 0;

	if (!astc_dev) {
		pr_err("%s: invalid astc device!\n", __func__);
		return -EINVAL;
	}
	if (!task) {
		dev_err(astc_dev->dev, "%s: invalid task!\n", __func__);
		return -EINVAL;
	}

	addr = task->dma_buf_out.plane.dma_addr;

	dev_dbg(astc_dev->dev, "%s: BEGIN setting source\n", __func__);

	/* set source address */
	astc_hw_set_src_addr(astc_dev->regs, addr);

	dev_dbg(astc_dev->dev, "%s: END source set\n", __func__);

	return 0;
}

/**
 * Set the H/W register representing the DMA address of the target buffer
 * and the number of ASTC blocks alongside X and Y axes needed to encode the
 * input image with the given encoding settings.
 */
static int astc_set_target(struct astc_dev *astc_dev,
			   struct astc_task *task)
{
	dma_addr_t addr = 0;

	if (!astc_dev) {
		pr_err("%s: invalid astc device!\n", __func__);
		return -EINVAL;
	}
	if (!task) {
		dev_err(astc_dev->dev, "%s: invalid task!\n", __func__);
		return -EINVAL;
	}
	if (!task->ctx) {
		dev_err(astc_dev->dev, "%s: invalid task context!\n", __func__);
		return -EINVAL;
	}

	dev_dbg(astc_dev->dev, "%s: BEGIN setting target\n", __func__);

	addr = task->dma_buf_cap.plane.dma_addr;

	/* set source width, height and number of blocks */
	astc_hw_set_image_size_num_blocks(astc_dev->regs
					  , task->user_task.fmt_cap.width
					  , task->user_task.fmt_cap.height
					  , task->ctx->blocks_on_x
					  , task->ctx->blocks_on_y);

	/* set target address */
	astc_hw_set_dst_addr(astc_dev->regs, addr);

	dev_dbg(astc_dev->dev, "%s: END\n", __func__);

	return 0;
}

/**
 * Acquire and "prepare" the clock producer resource.
 * This is the setup needed before enabling the clock source.
 */
static int astc_clk_devm_get_prepare(struct astc_dev *astc)
{
	struct device *dev = astc->dev;
	int ret = 0;

	dev_dbg(astc->dev, "%s: acquiring clock with devm\n", __func__);

	astc->clk_producer = devm_clk_get(dev, clk_producer_name);

	if (IS_ERR(astc->clk_producer)) {
		dev_err(dev, "%s clock is not present\n",
			clk_producer_name);
		return PTR_ERR(astc->clk_producer);
	}

	dev_dbg(astc->dev, "%s: preparing the clock\n", __func__);

	if (!IS_ERR(astc->clk_producer)) {
		ret = clk_prepare(astc->clk_producer);
		if (ret) {
			dev_err(astc->dev, "%s: clk preparation failed (%d)\n",
				__func__, ret);

			//invalidate clk var so that none else will use it
			astc->clk_producer = ERR_PTR(-EINVAL);

			//no need to release anything as devm will take care of
			//if eventually
			return ret;
		}
	}

	return 0;
}

/**
 * Request the system to Enable/Disable the clock source.
 *
 * We cannot read/write to H/W registers or use the device in any way
 * without enabling the clock source first.
 */
static int astc_clock_gating(struct astc_dev *astc, bool on)
{
	int ret = 0;

	if (on) {
		ret = clk_enable(astc->clk_producer);
		if (ret) {
			dev_err(astc->dev, "%s failed to enable clock\n"
				, __func__);
			return ret;
		}
		dev_dbg(astc->dev, "astc clock enabled\n");
	} else {
		clk_disable(astc->clk_producer);
		dev_dbg(astc->dev, "astc clock disabled\n");
	}

	return 0;
}

/**
 * Use the Runtime Power Management framework to request device power on/off.
 * This will cause the Runtime PM handlers to be called when appropriate (i.e.
 * when the device is asleep and needs to be woken up or nothing is using it
 * and can be suspended).
 *
 * Returns 0 on success, < 0 err num otherwise.
 */
static int enable_astc(struct astc_dev *astc)
{
	int ret;

	dev_dbg(astc->dev, "%s: runtime pm, getting device\n", __func__);

	//we're assuming our runtime_resume callback will enable the clock
	ret = in_irq() ? pm_runtime_get(astc->dev) :
			 pm_runtime_get_sync(astc->dev);
	if (ret < 0) {
		dev_err(astc->dev
			, "%s: failed to increment Power Mgmt usage cnt (%d)\n"
			, __func__, ret);
		return ret;
	}

	//No need to enable the clock, our runtime pm callback will do it

	return 0;
}
static int disable_astc(struct astc_dev *astc)
{
	int ret;

	dev_dbg(astc->dev
		, "%s: disabling clock, decrementing runtime pm usage\n"
		, __func__);

	//No need to disable the clock, our runtime pm callback will do it

	//we're assuming our runtime_suspend callback will disable the clock
	if (in_irq()) {
		dev_dbg(astc->dev
			, "%s: IN IRQ! DELAYED autosuspend\n" , __func__);
		ret = pm_runtime_put_autosuspend(astc->dev);
	} else {
		dev_dbg(astc->dev
			, "%s: SYNC autosuspend\n" , __func__);
		ret = pm_runtime_put_sync_autosuspend(astc->dev);
	}
	//ret = in_irq() ? pm_runtime_put_autosuspend(astc->dev) :
	//		 pm_runtime_put_sync_autosuspend(astc->dev);

	if (ret < 0) {
		dev_err(astc->dev
			, "%s: failed to decrement Power Mgmt usage cnt (%d)\n"
			, __func__, ret);
		return ret;
	}
	return 0;
}

static int astc_sysmmu_fault_handler(struct iommu_domain *domain,
				     struct device *dev,
				     unsigned long fault_addr,
				     int fault_flags, void *p)
{
	dev_dbg(dev, "%s: sysmmu fault!\n", __func__);

	/* Dump BUS errors */
	return 0;
}


static irqreturn_t astc_irq_handler(int irq, void *priv)
{
	unsigned long flags;
	unsigned int int_status;
	struct astc_dev *astc = priv;
	struct astc_task *task;
	bool status = true;

#ifdef HWASTC_PROFILE_ENABLE
	do_gettimeofday(&global_time_end);
	dev_info(astc->dev, "ENCODING TIME: %ld millisec\n",
		 1000 * (global_time_end.tv_sec - global_time_start.tv_sec) +
		 (global_time_end.tv_usec - global_time_start.tv_usec) / 1000);
#endif

	dev_dbg(astc->dev, "%s: BEGIN\n", __func__);

	dev_dbg(astc->dev, "%s: getting irq status\n", __func__);

	int_status = astc_hw_get_int_status(astc->regs);
	//Check that the board intentionally triggered the interrupt
	if (!int_status) {
		dev_warn(astc->dev, "bogus interrupt\n");
		return IRQ_NONE;
	}

	spin_lock_irqsave(&astc->slock, flags);
	//HW is done processing
	clear_bit(DEV_RUN, &astc->state);
	spin_unlock_irqrestore(&astc->slock, flags);

	dev_dbg(astc->dev, "%s: clearing irq status\n", __func__);

	astc_hw_clear_interrupt(astc->regs);

	dev_dbg(astc->dev, "%s: getting hw status\n", __func__);

	/* IRQ handling */
	if (get_hw_enc_status(astc->regs) != 0) {
		status = false;
		dev_err(astc->dev, "astc encoder bus error\n");
	}

	spin_lock_irqsave(&astc->lock_task, flags);
	task = astc->current_task;
	if (!task) {
		spin_unlock_irqrestore(&astc->lock_task, flags);
		dev_err(astc->dev, "received null task in irq handler\n");
		return IRQ_HANDLED;
	}
	spin_unlock_irqrestore(&astc->lock_task, flags);

	dev_dbg(astc->dev, "%s: finishing task\n", __func__);

	//signal task completion and schedule next task
	if (astc_task_signal_completion(astc, task, status) < 0) {
		dev_err(astc->dev, "%s: error while finishing a task!\n"
			, __func__);

		return IRQ_HANDLED;
	}

	dev_dbg(astc->dev, "%s: scheduling a new task...\n", __func__);

	//TODO: surely this can be done a different way
	astc_task_schedule(astc);


	dev_dbg(astc->dev, "%s: END\n", __func__);

	return IRQ_HANDLED;
}

//Prints the encoding configuration register in readable format
static void dump_config_register(struct astc_dev *device)
{
	u32 enc_config = readl(device->regs + ASTC_CTRL_REG);
	u32 color_fmt =   (enc_config >> ASTC_CTRL_REG_COLOR_FORMAT_INDEX) & (KBit0 | KBit1);
	bool dual_plane = (enc_config >> ASTC_CTRL_REG_DUAL_PLANE_INDEX)   &  KBit0;
	u32 refine_iter = (enc_config >> ASTC_CTRL_REG_REFINE_ITER_INDEX)  & (KBit0 | KBit1);
	u32 partitions =  (enc_config >> ASTC_CTRL_REG_PARTITION_EN_INDEX) &  KBit0;
	u32 blk_mode =    (enc_config >> ASTC_CTRL_REG_NUM_BLK_MODE_INDEX) &  KBit0;
	u32 blk_size =    (enc_config >> ASTC_CTRL_REG_ENC_BLK_SIZE_INDEX) & (KBit0 | KBit1);

	dev_dbg(device->dev, "ENCODING CONFIGURATION\n");
	dev_dbg(device->dev, "Color format reg val:      %u\n", color_fmt);
	dev_dbg(device->dev, "Dual plane reg val:        %d\n", dual_plane);
	dev_dbg(device->dev, "Refine iterations reg val: %u\n", refine_iter);
	dev_dbg(device->dev, "Partitions reg val:        %u\n", partitions);
	dev_dbg(device->dev, "Block mode:                %u\n", blk_mode);
	dev_dbg(device->dev, "Block size:                %u\n", blk_size);
}

static int pack_config(struct astc_ctx *context, struct astc_task *task, u32 *ret)
{
	struct device *dev = NULL;
	struct astc_config *cfg   = NULL;

	if (!context) {
		pr_err("%s: invalid context!\n", __func__);
		return -EINVAL;
	}

	dev = context->astc_dev->dev;
	cfg = &task->user_task.enc_config;

	if (!ret) {
		dev_err(dev, "%s: invalid return val address!\n", __func__);
		return -EINVAL;
	}

	dev_dbg(dev, "Packing Configuration received from userspace:");
	dev_dbg(dev, "ENCODING CONFIGURATION\n");
	dev_dbg(dev, "Color format reg val:      %u\n", task->user_task.fmt_out.fmt);
	dev_dbg(dev, "Dual plane reg val:        %d\n", cfg->dual_plane_enable);
	dev_dbg(dev, "Refine iterations reg val: %u\n", cfg->intref_iterations);
	dev_dbg(dev, "Partitions reg val:        %u\n", cfg->partitions);
	dev_dbg(dev, "Block mode:                %u\n", cfg->num_blk_mode);
	dev_dbg(dev, "Block size:                %u\n", cfg->encodeBlockSize);

	*ret = 0 | ((task->user_task.fmt_out.fmt   & (KBit0 | KBit1)) << ASTC_CTRL_REG_COLOR_FORMAT_INDEX)
			| ((cfg->dual_plane_enable &  KBit0)          << ASTC_CTRL_REG_DUAL_PLANE_INDEX)
			| ((cfg->intref_iterations & (KBit0 | KBit1)) << ASTC_CTRL_REG_REFINE_ITER_INDEX)
			| ((cfg->partitions        &  KBit0)          << ASTC_CTRL_REG_PARTITION_EN_INDEX)
			| ((cfg->num_blk_mode      &  KBit0)          << ASTC_CTRL_REG_NUM_BLK_MODE_INDEX)
			| ((cfg->encodeBlockSize   & (KBit0 | KBit1)) << ASTC_CTRL_REG_ENC_BLK_SIZE_INDEX);

	return 0;
}

static void set_enc_config(struct astc_ctx *ctx, u32 packed_config)
{
	astc_hw_set_enc_mode(ctx->astc_dev->regs, packed_config);

	dump_config_register(ctx->astc_dev);
}

static int astc_device_run(struct astc_ctx *astc_context,
			   struct astc_task *task)
{
	struct astc_dev *astc = astc_context->astc_dev;
	u32 packed_config = 0;
	int ret = 0;
	unsigned long flags = 0;

	dev_dbg(astc_context->astc_dev->dev, "%s: BEGIN\n", __func__);

	if (!astc) {
		pr_err("astc is null\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&astc->slock, flags);
	if (test_bit(DEV_SUSPEND, &astc->state)) {
		spin_unlock_irqrestore(&astc->slock, flags);

		dev_err(astc->dev,
			"%s: unexpected suspended device state right before running a task\n", __func__);
		return -1;
	}
	spin_unlock_irqrestore(&astc->slock, flags);


	dev_dbg(astc->dev, "%s: resetting astc device\n", __func__);

	/* H/W initialization: this will reset all registers to reset values */
	astc_hw_soft_reset(astc->regs);

	dev_dbg(astc->dev, "%s: setting source/target addresses\n", __func__);

	/* setting for source */
	ret = astc_set_source(astc, task);
	if (ret) {
		dev_err(astc->dev, "%s: error setting source\n", __func__);
		return ret;
	}

	/* setting for destination */
	ret = astc_set_target(astc, task);
	if (ret) {
		dev_err(astc->dev, "%s: error setting target\n", __func__);
		return ret;
	}

	dev_dbg(astc->dev, "%s: enabling interrupts\n", __func__);
	astc_hw_enable_interrupt(astc->regs);

	dev_dbg(astc->dev, "%s: setting encoding configuration\n", __func__);

	//Pack the config received from userspace in a u32
	//and write it to the register
	ret = pack_config(astc_context
			  , task
			  , &packed_config);
	if (ret) {
		dev_err(astc->dev, "%s: error setting configuration\n"
			, __func__);
		return ret;
	}

	set_enc_config(astc_context, packed_config);

	//8 bursts (value 2) is supposed to be more stable than
	//the default 16, according to the HW firmware developer
	astc_hw_set_axi_burst_len(astc->regs, 2);

	dump_regs(astc_context->astc_dev);

	dev_dbg(astc->dev, "%s: starting actual encoding process\n", __func__);

	//ADD mdelay FOR ABOUT 2 SECS HERE TO GET DEBUGGING OUTPUT ON CONSOLE,
	//OR THE KERNEL BUFFER WILL NOT HAVE THE CHANCE TO PRINT TO CONSOLE
	//IN CASE OF A CRASH/REBOOT
	//mdelay(3000);

#ifdef HWASTC_PROFILE_ENABLE
	do_gettimeofday(&global_time_start);
#endif

	/* run H/W */
	astc_hw_start_enc(astc->regs);

	dev_dbg(astc->dev, "%s: END\n", __func__);

	return 0;
}


static int astc_buffer_map(struct astc_ctx *ctx,
			       struct astc_buffer_dma *dma_buffer,
			       enum dma_data_direction dir)
{
	int ret = 0;

	dev_dbg(ctx->astc_dev->dev, "%s: BEGIN\n", __func__);

	dev_dbg(ctx->astc_dev->dev
		, "%s: about to map dma buffer in device address space...\n"
		, __func__);

	//map dma attachment:
	//this is the point where the dma buffer is actually
	//allocated, if it wasn't there before.
	//It also means the calling driver now owns the buffer
	ret = astc_map_dma_buf(ctx->astc_dev->dev,
			       &dma_buffer->plane, dir);
	if (ret)
		return ret;

	dev_dbg(ctx->astc_dev->dev, "%s: getting dma address of the buffer...\n"
		, __func__);

	//use iommu to map and get the virtual memory address
	ret = astc_dma_addr_map(ctx->astc_dev->dev, dma_buffer, dir);
	if (ret) {
		dev_dbg(ctx->astc_dev->dev, "%s: mapping FAILED!\n", __func__);
		astc_unmap_dma_buf(ctx->astc_dev->dev,
				   &dma_buffer->plane, dir);
		return ret;
	}

	dev_dbg(ctx->astc_dev->dev, "%s: END\n", __func__);

	return 0;
}

static void astc_buffer_unmap(struct astc_ctx *ctx,
			       struct astc_buffer_dma *dma_buffer,
			       enum dma_data_direction dir)
{
	dev_dbg(ctx->astc_dev->dev, "%s: BEGIN\n", __func__);
	dev_dbg(ctx->astc_dev->dev, "%s: about to unmap the DMA address\n"
		, __func__);

	//use iommu to unmap the dma address
	astc_dma_addr_unmap(ctx->astc_dev->dev, dma_buffer);

	dev_dbg(ctx->astc_dev->dev, "%s: about to unmap the DMA buffer\n"
		, __func__);

	//unmap dma attachment, i.e. unmap the buffer so others can request it
	astc_unmap_dma_buf(ctx->astc_dev->dev,
			   &dma_buffer->plane, dir);

	//hwASTC_task will dma detach and decrease the refcount on the dma fd

	dev_dbg(ctx->astc_dev->dev, "%s: END\n", __func__);

}

static void astc_task_timeout(struct astc_ctx *ctx,
			      struct astc_task *task)
{
	dev_dbg(ctx->astc_dev->dev, "%s: BEGIN\n", __func__);

	dev_info(ctx->astc_dev->dev, "%s: Timeout occurred\n", __func__);

	//TODO: RESET HW AND SCHEDULE NEXT TASK (DISABLE CLOCKS TOO?)

	dev_dbg(ctx->astc_dev->dev, "%s: END\n", __func__);
}

/**
 * Initialization routine.
 *
 * This is called at boot time, when the system finds a device that this
 * driver requested to be a handler of (see platform_driver struct below).
 *
 * Set up access to H/W registers, register the device, register the IRQ
 * resource, IOVMM page fault handler, spinlocks, etc.
 */
static int astc_probe(struct platform_device *pdev)
{
	struct astc_dev *astc = NULL;
	struct resource *res = NULL;
	int ret, irq = -1;

	// This replaces the old pre-device-tree dev.platform_data
	// To be populated with data coming from device tree
	astc = devm_kzalloc(&pdev->dev, sizeof(struct astc_dev), GFP_KERNEL);
	if (!astc) {
		//not enough memory
		return -ENOMEM;
	}

	astc->dev = &pdev->dev;

	spin_lock_init(&astc->slock);

	/* Get the memory resource and map it to kernel space. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	astc->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(astc->regs)) {
		dev_err(&pdev->dev, "failed to claim/map register region\n");
		return PTR_ERR(astc->regs);
	}

	/* Get IRQ resource and register IRQ handler. */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		if (irq != -EPROBE_DEFER)
			dev_err(&pdev->dev, "cannot get irq\n");
		return irq;
	}

	// passing 0 as flags means the flags will be read from DT
	ret = devm_request_irq(&pdev->dev, irq,
			       (void *)astc_irq_handler, 0,
			       pdev->name, astc);

	if (ret) {
		dev_err(&pdev->dev, "failed to install irq\n");
		return ret;
	}

	astc->misc.minor = MISC_DYNAMIC_MINOR;
	astc->misc.name = NODE_NAME;
	astc->misc.fops = &astc_fops;
	astc->misc.parent = &pdev->dev;
	ret = misc_register(&astc->misc);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&astc->tasks);
	INIT_LIST_HEAD(&astc->contexts);

	spin_lock_init(&astc->lock_task);
	spin_lock_init(&astc->lock_ctx);

	astc->timeout_jiffies = -1; //msecs_to_jiffies(500);

	/* clock */
	ret = astc_clk_devm_get_prepare(astc);
	if (ret)
		goto err_clkget;

	platform_set_drvdata(pdev, astc);

#if defined(CONFIG_PM_DEVFREQ)
	if (!of_property_read_u32(pdev->dev.of_node, "astc,int_qos_minlock",
				(u32 *)&astc->qos_req_level)) {
		if (astc->qos_req_level > 0) {
			pm_qos_add_request(&astc->qos_req,
					   PM_QOS_DEVICE_THROUGHPUT, 0);

			dev_info(&pdev->dev, "INT Min.Lock Freq. = %u\n",
						astc->qos_req_level);
		}
	}
#endif
	pm_runtime_set_autosuspend_delay(&pdev->dev, 1000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	iovmm_set_fault_handler(&pdev->dev,
				astc_sysmmu_fault_handler, astc);

	ret = iovmm_activate(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: Failed to activate iommu (%d)\n",
			__func__, ret);
		goto err_iovmm;
	}

	//Power device on to access registers
	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev
			, "%s: failed to pwr on device with runtime pm (%d)\n",
			__func__, ret);
		goto err_pm;
	}

	// TODO: will this ever return <0 ?
	ret = astc_hw_get_version(astc->regs);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get H/W version\n");
		goto err_regs;
	} else {
		dev_info(&pdev->dev, "HW ver: %u", (unsigned int) ret);
		astc->version = ret;
	}

	//Doublecheck we're getting sensible values from register reads,
	//by reading the magic number reg
	if (astc_get_magic_number(astc->regs)
			!= ASTC_MAGIC_NUM_REG_RESETVALUE) {
		dev_err(&pdev->dev, "wrong magic number value.\n");
		goto err_regs;
	}

	//Make sure that the irq register says it has not fired any irq yet.
	//It cannot be otherwise, since we're in initialization phase.
	if (astc_hw_get_int_status(astc->regs)) {
		dev_err(&pdev->dev, "invalid interrupt status.\n");
		goto err_regs;
	}

	//Encoding status register must not report errors, as we have not
	//tried encoding anything yet
	if (get_hw_enc_status(astc->regs)) {
		dev_err(&pdev->dev, "wrong enc status at init stage.\n");
		goto err_regs;
	}

	pm_runtime_put_sync(&pdev->dev);

	dev_info(&pdev->dev, "hwastc probe completed successfully.\n");

	return 0;

err_regs:
	pm_runtime_put_sync(&pdev->dev);
err_pm:
	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	iovmm_deactivate(&pdev->dev);
err_iovmm:
	if (astc->qos_req_level > 0)
		pm_qos_remove_request(&astc->qos_req);
	if (!IS_ERR(astc->clk_producer)) /* devm will call clk_put */
		clk_unprepare(astc->clk_producer);
	platform_set_drvdata(pdev, NULL);
err_clkget:
	astc_deregister_device(astc);

	dev_err(&pdev->dev, "hwastc probe is failed.\n");

	return ret;
}

//Note: drv->remove() will not be called by the kernel framework because this
//      is a builtin driver. (see builtin_platform_driver at the bottom)
static int astc_remove(struct platform_device *pdev)
{
	struct astc_dev *astc = platform_get_drvdata(pdev);

	dev_dbg(astc->dev, "%s: BEGIN removing device\n", __func__);

	if (astc->qos_req_level > 0 && pm_qos_request_active(&astc->qos_req))
		pm_qos_remove_request(&astc->qos_req);

	//From the docs:
	//Drivers in ->remove() callback should undo the runtime PM changes done
	//in ->probe(). Usually this means calling pm_runtime_disable(),
	//pm_runtime_dont_use_autosuspend() etc.
	//disable runtime pm before releasing the clock,
	//or pm runtime could still use it
	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	iovmm_deactivate(&pdev->dev);

	if (!IS_ERR(astc->clk_producer)) /* devm will call clk_put */
		clk_unprepare(astc->clk_producer);

	platform_set_drvdata(pdev, NULL);

	astc_deregister_device(astc);

	dev_dbg(astc->dev, "%s: END removing device\n", __func__);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int astc_suspend(struct device *dev)
{
	struct astc_dev *astc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: BEGIN\n", __func__);

	set_bit(DEV_SUSPEND, &astc->state);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}

static int astc_resume(struct device *dev)
{
	struct astc_dev *astc = dev_get_drvdata(dev);

	dev_dbg(dev, "%s: BEGIN\n", __func__);

	clear_bit(DEV_SUSPEND, &astc->state);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}
#endif

#ifdef CONFIG_PM
static int astc_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct astc_dev *astc = platform_get_drvdata(pdev);
	unsigned long flags = 0;

	dev_dbg(dev, "%s: BEGIN\n", __func__);

	spin_lock_irqsave(&astc->slock, flags);
	//According to official runtime PM docs, when using runtime PM
	//autosuspend, it might happen that the autosuspend timer expires,
	//runtime_suspend is entered, and at the same time we receive new job.
	//In that case we return -EBUSY to tell runtime PM core that we're
	//not ready to go in low-power state.
	if (test_bit(DEV_RUN, &astc->state)) {
		dev_info(dev, "deferring suspend, device is still processing!");
		spin_unlock_irqrestore(&astc->slock, flags);

		return -EBUSY;
	}
	spin_unlock_irqrestore(&astc->slock, flags);

	dev_dbg(dev, "%s: gating clock, suspending\n", __func__);

	HWASTC_PROFILE(astc_clock_gating(astc, false), "SWITCHING OFF CLOCK",
		       astc->dev);

	HWASTC_PROFILE(
	if (astc->qos_req_level > 0)
		pm_qos_update_request(&astc->qos_req, 0);,
	"UPDATE QoS REQUEST", astc->dev
	);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}

static int astc_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct astc_dev *astc = platform_get_drvdata(pdev);

	dev_dbg(dev, "%s: BEGIN\n", __func__);
	dev_dbg(dev, "%s: ungating clock, resuming\n", __func__);

	HWASTC_PROFILE(astc_clock_gating(astc, true), "SWITCHING ON CLOCK",
		       astc->dev);

	HWASTC_PROFILE(
	if (astc->qos_req_level > 0)
		pm_qos_update_request(&astc->qos_req, astc->qos_req_level);,
	"UPDATE QoS REQUEST", astc->dev
	);

	dev_dbg(dev, "%s: END\n", __func__);

	return 0;
}
#endif


static const struct dev_pm_ops astc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(astc_suspend, astc_resume)
	SET_RUNTIME_PM_OPS(astc_runtime_suspend, astc_runtime_resume, NULL)
};

// of == OpenFirmware, aka DeviceTree
// This allows matching devices which are specified as "vendorid,deviceid"
// in the device tree to this driver
static const struct of_device_id exynos_astc_match[] = {
{
	.compatible = "samsung,exynos-astc",
},
{},
};

// Load the kernel module when the device is matched against this driver
MODULE_DEVICE_TABLE(of, exynos_astc_match);

static struct platform_driver astc_driver = {
	.probe		= astc_probe,
	.remove		= astc_remove,
	.driver = {
		// Any device advertising itself as MODULE_NAME will be given
		// this driver.
		// No ID table needed (this was more relevant in the past,
		// when platform_device struct was used instead of device-tree)
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &astc_pm_ops,
		// Tell the kernel what dev tree names this driver can handle.
		// This is needed because device trees are meant to work with
		// more than one OS, so the driver name might not match the name
		// which is in the device tree.
		.of_match_table = of_match_ptr(exynos_astc_match),
	}
};

// Macro that will declare module init function that will register the driver
// i.e platform_driver_register
builtin_platform_driver(astc_driver);

MODULE_AUTHOR("Andrea Bernabei <a.bernabei@samsung.com>");
MODULE_DESCRIPTION("Samsung ASTC encoder driver");
MODULE_LICENSE("GPL");
