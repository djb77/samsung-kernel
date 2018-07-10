/* iva_mcu.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/time.h>

#include "regs/iva_base_addr.h"
#include "iva_mcu.h"
#include "iva_pmu.h"
#include "iva_sh_mem.h"
#include "iva_ram_dump.h"
#include "iva_mbox.h"
#include "iva_ipc_queue.h"
#include "iva_vdma.h"

//#define MEASURE_MCU_BOOT_PERF
#define MCU_SRAM_VIA_MEMCPY

#define PRINT_CHARS_ALIGN_SIZE		(sizeof(uint32_t))
#define PRINT_CHARS_ALIGN_MASK		(PRINT_CHARS_ALIGN_SIZE - 1)
#define PRINT_CHARS_ALIGN_POS(a)	(a & (~PRINT_CHARS_ALIGN_MASK))

static inline void iva_mcu_print_putchar(struct device *dev, int ch)
{
	#define MCU_PRINT_BUF_SIZE	(512)
	static char	mcu_print_buf[MCU_PRINT_BUF_SIZE];
	static size_t	mcu_print_pos = 0;
	static const char *mcu_tag = "	[mcu] ";

	if (ch != 0 && ch != '\n') {
		if (mcu_print_pos == 0) {
			strncpy(mcu_print_buf, mcu_tag, MCU_PRINT_BUF_SIZE - 1);
			mcu_print_pos = strlen(mcu_tag);
		}
		mcu_print_buf[mcu_print_pos] = ch;
		mcu_print_pos++;
		if (mcu_print_pos == (sizeof(mcu_print_buf) - 1))/* full */
			goto print;
		return;
	}

	if (ch == 0) {
		if (mcu_print_pos == strlen(mcu_tag))
			return;
	}

	/* the case that: if (ch =='\n'): throw it. */
print:
	/* print out first */
	if (mcu_print_pos != 0) {
		mcu_print_buf[mcu_print_pos] = 0;
		dev_info(dev, "%s\n", mcu_print_buf);
	}
	strncpy(mcu_print_buf, mcu_tag, MCU_PRINT_BUF_SIZE - 1);
	mcu_print_pos = strlen(mcu_tag);
}

/* put max 4 bytes to output */
static int iva_mcu_put_uint_chars(struct device *dev, uint32_t chars,
		uint32_t pos_s/*0-3*/, uint32_t pos_e /*0-3*/)
{
	int put_num = 0;
	char ch;

	if (pos_s > sizeof(chars) - 1) {
		dev_err(dev, "%s() error in pos_s(%d), pos_e(%d)\n",
				__func__, pos_s, pos_e);
		return -1;
	}
	if (pos_e > sizeof(chars) - 1) {
		dev_err(dev, "%s() error in pos_s(%d), pos_e(%d)\n",
				__func__, pos_s, pos_e);
		return -1;
	}

	if (pos_e < pos_s)
		return 0;

	while (pos_s <= pos_e) {
		ch = (chars >> (pos_s << 3)) & 0xFF;
		iva_mcu_print_putchar(dev, ch);
		pos_s++;
		put_num++;
	}

	return put_num;
};


static void iva_mcu_print_emit_pending_chars_nolock(struct mcu_print_info *pi)
{
	u32		ch_vals;
	uint32_t	pos_s, pos_e;
	int		align_ch_pos;
	u32		head;

	if (!pi)
		return;

	head = readl(&pi->log_pos->head);
	if (head >= pi->log_buf_size) {	/* wrong value */
		struct device		*dev = pi->dev;
		struct iva_dev_data	*iva = dev_get_drvdata(dev);
		struct sh_mem_info	*sh_mem = sh_mem_get_sm_pointer(iva);
		dev_err(dev, "%s() magic3(0x%x), head(%d, %d), tail(%d), "
				"size(%d)\n", __func__,
				readl(&sh_mem->magic3),
				(int) readl(&sh_mem->log_pos.head),
				(int) head,
				(int) readl(&sh_mem->log_pos.tail),
				(int) readl(&sh_mem->log_buf_size));
		return;
	}
	if (pi->ch_pos == head)
		return;

	if (pi->ch_pos > head) {
		while (pi->ch_pos < pi->log_buf_size) {
			align_ch_pos = PRINT_CHARS_ALIGN_POS(pi->ch_pos);	/* 4 bytes */
			/* aligned access */
			ch_vals = readl(&pi->log_buf[align_ch_pos]);
			pos_s	= pi->ch_pos - align_ch_pos;
			pi->ch_pos += iva_mcu_put_uint_chars(pi->dev, ch_vals,
					pos_s, PRINT_CHARS_ALIGN_SIZE - 1);
		}
		pi->ch_pos = 0;
	}

	while (pi->ch_pos < head) {
		align_ch_pos = PRINT_CHARS_ALIGN_POS(pi->ch_pos);
		/* aligned access */
		ch_vals = readl(&pi->log_buf[align_ch_pos]);
		pos_s	= pi->ch_pos - align_ch_pos;
		pos_e	= (align_ch_pos + PRINT_CHARS_ALIGN_SIZE <= head) ?
				PRINT_CHARS_ALIGN_SIZE - 1 :
					head - align_ch_pos - 1;

		pi->ch_pos += iva_mcu_put_uint_chars(pi->dev, ch_vals, pos_s, pos_e);
	}

}

static void iva_mcu_print_worker(struct work_struct *work)
{
	struct mcu_print_info *pi = container_of(work, struct mcu_print_info,
					  dwork.work);
	/* emit all pending log */
	iva_mcu_print_emit_pending_chars_nolock(pi);

	queue_delayed_work(system_wq, &pi->dwork, pi->delay_jiffies);
}

static void iva_mcu_print_start(struct iva_dev_data *iva)
{
	struct sh_mem_info	*sh_mem = sh_mem_get_sm_pointer(iva);
	struct buf_pos __iomem	*log_pos = &sh_mem->log_pos;
	struct mcu_print_info	*pi;
	struct device		*dev = iva->dev;
	u32			tail;
	int			lb_sz;

	if (iva->mcu_print_delay == 0) {
		dev_dbg(dev, "%s() will not print out iva log\n", __func__);
		return;
	}

	lb_sz = readl(&sh_mem->log_buf_size);
	if (lb_sz & PRINT_CHARS_ALIGN_MASK) {
		dev_err(dev, "%s() log_buf_size(0x%x) is not aligned to %d. "
				"check the mcu shmem.\n",
				__func__, lb_sz, (int) PRINT_CHARS_ALIGN_SIZE);
		return;
	}

	if (((long) sh_mem->log_buf) & PRINT_CHARS_ALIGN_MASK) {
		dev_err(dev, "%s() log_buf(%p) is not aligned to %d. "
				"check the mcu shmem.\n",
				__func__, sh_mem->log_buf,
				(int) PRINT_CHARS_ALIGN_SIZE);
		return;
	}

	pi = devm_kmalloc(dev, sizeof(*pi), GFP_KERNEL);
	if (!pi) {
		dev_err(dev, "%s() fail to allocate memory\n", __func__);
		return;
	}

	pi->dev		= iva->dev;
	pi->delay_jiffies= usecs_to_jiffies(iva->mcu_print_delay);
	pi->log_buf_size= lb_sz;
	pi->log_buf	= &sh_mem->log_buf[0];
	pi->log_pos	= log_pos;
	tail = readl(&log_pos->tail);
	if (tail >= pi->log_buf_size)
		pi->ch_pos = 0;
	else
		pi->ch_pos = tail;

	INIT_DELAYED_WORK(&pi->dwork, iva_mcu_print_worker);
	queue_delayed_work(system_wq, &pi->dwork, msecs_to_jiffies(1));

	iva->mcu_print = pi;

	dev_dbg(dev, "%s() delay_jiffies(%ld) success!!!\n", __func__,
				pi->delay_jiffies);

}

static void iva_mcu_print_stop(struct iva_dev_data *iva)
{
	struct device *dev = iva->dev;

	if (iva->mcu_print) {
		struct mcu_print_info *pi = iva->mcu_print;
		cancel_delayed_work_sync(&pi->dwork);
		devm_kfree(dev, pi);
		iva->mcu_print = NULL;
	}
}

static noinline_for_stack uint32_t iva_mcu_file_size(struct file *file)
{
	struct kstat st;

	if (vfs_getattr(&file->f_path, &st))
		return 0;
	if (!S_ISREG(st.mode))
		return 0;

	return (uint32_t) st.size;
}

void iva_mcu_handle_panic_k(struct iva_dev_data *iva)
{
#if 0	/* entrusts the log to work queue */
	iva_mcu_print_emit_pending_chars_nolock(iva->mcu_print);
#endif
}

int32_t iva_mcu_wait_ready(struct iva_dev_data *iva)
{
	#define WAIT_DELAY_US	(1000)

	int ret = sh_mem_wait_mcu_ready(iva, WAIT_DELAY_US);
	if (ret) {
		dev_err(iva->dev, "%s() mcu is not ready within %d us.\n",
				__func__, WAIT_DELAY_US);
		return ret;
	}

	return 0;
}

static inline s32 __maybe_unused timeval_diff_us(struct timeval *a,
		struct timeval *b)
{
	return (a->tv_sec - b->tv_sec) * USEC_PER_SEC +
			(a->tv_usec - b->tv_usec);
}

static inline void iva_mcu_update_iva_sram(struct iva_dev_data *iva,
		struct mcu_binary *mcu_bin)
{
#ifdef CONFIG_ION_EXYNOS
	uint32_t vdma_hd = ID_CH_TO_HANDLE(vdma_id_1, vdma_ch_2);

	dev_dbg(iva->dev, "%s() prepare vdma iova(0x%x) size(0x%x, 0x%x)\n",
			__func__, (uint32_t) mcu_bin->io_va,
			mcu_bin->file_size, ALIGN(mcu_bin->file_size, 16));

	iva_vdma_enable(iva, vdma_id_1, true);

	iva_vdma_config_dram_to_mcu(iva, vdma_hd,
			mcu_bin->io_va, 0x0, ALIGN(mcu_bin->file_size, 16),
			true);
	iva_vdma_wait_done(iva, vdma_hd);
	iva_vdma_enable(iva, vdma_id_1, false);
#else
	void __iomem *mcu_va = iva->iva_va + IVA_VMCU_MEM_BASE_ADDR;

	memcpy(mcu_va, mcu_bin->bin, mcu_bin->bin_size);
#endif
}


static int32_t iva_mcu_boot(struct iva_dev_data *iva, struct mcu_binary *mcu_bin,
		int32_t wait_ready)
{
	int ret;
	struct device	*dev = iva->dev;

	if (!mcu_bin) {
		dev_err(dev, "%s() check null pointer\n", __func__);
		return -EINVAL;
	}
#if 0
	if (mcu_bin_size != VMCU_MEM_SIZE) {
		dev_err(dev, "%s() mcu binary seems to have wrong size(%d)\n",
				__func__, mcu_bin_size);
		return -EINVAL;
	}
#endif
	iva_pmu_prepare_mcu_sram(iva);

	iva_mcu_update_iva_sram(iva, mcu_bin);

	/* start to boot */
	iva_pmu_reset_mcu(iva);

	/* monitor shared memory initialization from mcu */
	ret = sh_mem_init(iva);
	if (ret) {
		dev_err(dev, "%s() mcu seems to fail to boot.\n", __func__);
		return ret;
	}

	/* enable mcu print */
	iva_mcu_print_start(iva);

	if (wait_ready)
		iva_mcu_wait_ready(iva);

	/* enable ipq queue */
	ret = iva_ipcq_init(&iva->mcu_ipcq,
			&sh_mem_get_sm_pointer(iva)->ap_cmd_q,
			&sh_mem_get_sm_pointer(iva)->ap_rsp_q);
	if (ret < 0) {
		dev_err(dev, "%s() fail to init ipcq (%d)\n", __func__, ret);
		goto err_ipcq;
	}

	return 0;
err_ipcq:
	sh_mem_deinit(iva);
	iva_mcu_print_stop(iva);
	return ret;
}


int32_t iva_mcu_boot_file(struct iva_dev_data *iva,
			const char *mcu_file, int32_t wait_ready)
{
	struct device	*dev = iva->dev;
	struct file	*mcu_fp;
	uint32_t	mcu_size;
	struct mcu_binary *mcu_bin = iva->mcu_bin;
	int		ret = 0;
	int		read_bytes;

	dev_dbg(dev, "%s() try to boot with %s", __func__, mcu_file);

	mcu_fp = filp_open(mcu_file, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(mcu_fp)) {
		dev_err(dev, "%s() unable to open mcu binary: %s\n",
			__func__, mcu_file);
		return -EINVAL;
	}

	mcu_size = iva_mcu_file_size(mcu_fp);
	if (mcu_size <= 0 || mcu_size > VMCU_MEM_SIZE) {
		dev_err(dev, "%s() file size(0x%x) is larger that expected %d\n",
			__func__, mcu_size, VMCU_MEM_SIZE);
		goto err_mcu_file;
	}

	/* only one case is not covered, fix it */
	if (mcu_size == mcu_bin->file_size &&
			strcmp(mcu_file, mcu_bin->file) == 0) {
		dev_dbg(dev, "%s() regard the same as previous(%s, 0x%x)\n",
				__func__, mcu_file, mcu_size);
		goto skip_read;

	}

	if (mcu_bin->flag & MCU_BIN_ION) {
		if (dma_buf_begin_cpu_access(mcu_bin->dmabuf,
				0, VMCU_MEM_SIZE, 0)) {
			ret = -ENOMEM;
			goto err_mcu_file;
		}

		mcu_bin->bin = dma_buf_kmap(mcu_bin->dmabuf, 0);
		if (!mcu_bin->bin) {
			dev_err(dev, "%s() fail to kmap ion\n", __func__);
			dma_buf_end_cpu_access(mcu_bin->dmabuf, 0,
					VMCU_MEM_SIZE, 0);
			ret = -ENOMEM;
			goto err_mcu_file;
		}
	}

	read_bytes = kernel_read(mcu_fp, 0, mcu_bin->bin,
			(unsigned long) mcu_size);
	if ((read_bytes <= 0) || ((uint32_t) read_bytes != mcu_size)) {
		dev_err(dev, "%s() unable to read %s sucessfully. "
				"read(%d) file size(%d)\n",
				__func__, mcu_file, read_bytes, mcu_size);
		ret = -EIO;
		goto err_ion_kmap;
	}

	mcu_bin->file_size = mcu_size;
	strncpy(mcu_bin->file, mcu_file, sizeof(mcu_bin->file) - 1);
	mcu_bin->file[sizeof(mcu_bin->file) - 1] = 0x0;
skip_read:
	ret = iva_mcu_boot(iva, mcu_bin, wait_ready);
	if (ret) {
		dev_err(dev, "%s() fail to boot mcu. ret(%d)\n",
				__func__, ret);
	}

err_ion_kmap:
	if (mcu_bin->flag & MCU_BIN_ION) {
		if (mcu_bin->bin) {
			dma_buf_kunmap(mcu_bin->dmabuf, 0, mcu_bin->bin);
			dma_buf_end_cpu_access(mcu_bin->dmabuf, 0, VMCU_MEM_SIZE, 0);
			mcu_bin->bin = NULL;
		}
	}

err_mcu_file:
	filp_close(mcu_fp, NULL);

	return ret;
}


int32_t iva_mcu_exit(struct iva_dev_data *iva)
{
	iva_ipcq_deinit(&iva->mcu_ipcq);

	iva_mcu_print_stop(iva);

	sh_mem_deinit(iva);

	return 0;
}

#ifdef CONFIG_ION_EXYNOS
#include <linux/exynos_ion.h>
#define REQ_ION_HEAP_ID		EXYNOS_ION_HEAP_SYSTEM_MASK
#endif

static int32_t iva_mcu_boot_mem_alloc(struct iva_dev_data *iva,
			struct mcu_binary *mcu_bin)
{
	int32_t			ret = 0;
	struct device		*dev = iva->dev;
#ifdef CONFIG_ION_EXYNOS
	struct ion_client	*client = iva->ion_client;
	struct ion_handle	*handle;
	struct dma_buf		*dmabuf;
	struct dma_buf_attachment *attachment;
	dma_addr_t		io_va;

	handle = ion_alloc(client, VMCU_MEM_SIZE, 16, REQ_ION_HEAP_ID,
			/*ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC*/ 0);
	if (IS_ERR(handle)) {
		dev_err(dev, "%s() fail to alloc ion with id(0x%x), "
				"size(0x%x),ret(%ld)\n",
				__func__, REQ_ION_HEAP_ID,
				VMCU_MEM_SIZE, PTR_ERR(handle));
		return PTR_ERR(handle);
	}

	dmabuf = ion_share_dma_buf(client, handle);
	if (!dmabuf) {
		dev_err(dev, "%s() ion with dma_buf sharing failed.\n",
				__func__);
		ret = -ENOMEM;
		goto err_dmabuf;
	}

	attachment = dma_buf_attach(dmabuf, dev);
	if (!attachment) {
		dev_err(dev, "%s() fail to attach dma buf, dmabuf(%p).\n",
				__func__, dmabuf);
		goto err_attach;
	}

	io_va = ion_iovmm_map(attachment, 0, VMCU_MEM_SIZE, DMA_TO_DEVICE, 0);
	if (!io_va || IS_ERR_VALUE(io_va)) {
		dev_err(dev, "%s() fail to map iovm (%ld)\n", __func__,
				(unsigned long) io_va);
		if (!io_va)
			ret = -ENOMEM;
		else
			ret = (int) io_va;
		goto err_get_iova;
	}

	mcu_bin->flag		= MCU_BIN_ION;
	mcu_bin->bin_size	= VMCU_MEM_SIZE;
	mcu_bin->handle		= handle;
	mcu_bin->dmabuf 	= dmabuf;
	mcu_bin->attachment	= attachment;
	mcu_bin->io_va		= io_va;

	return 0;
err_get_iova:
	dma_buf_detach(dmabuf, attachment);
err_attach:
	dma_buf_put(dmabuf);
err_dmabuf:
	ion_free(client, handle);
#else
	mcu_bin->bin = kmalloc(VMCU_MEM_SIZE, GFP_KERNEL);
	if (!mcu_bin->bin) {
		dev_err(dev, "%s() unable to alloc mem for mcu boot\n",
				__func__);
		return -ENOMEM;
	}
	mcu_bin->flag		= MCU_BIN_MALLOC;
	mcu_bin->bin_size	= VMCU_MEM_SIZE;
#endif
	return ret;
}

static void iva_mcu_boot_mem_free(struct iva_dev_data *iva,
		struct mcu_binary *mcu_bin)
{
#ifdef CONFIG_ION_EXYNOS
	struct ion_client	*client = iva->ion_client;

	if (mcu_bin->attachment) {
		ion_iovmm_unmap(mcu_bin->attachment, mcu_bin->io_va);
		if (mcu_bin->dmabuf)
			dma_buf_detach(mcu_bin->dmabuf, mcu_bin->attachment);
		mcu_bin->io_va		= 0x0;
		mcu_bin->attachment	= NULL;
	}

	if (mcu_bin->dmabuf) {
		dma_buf_put(mcu_bin->dmabuf);
		mcu_bin->dmabuf = NULL;
	}

	if (mcu_bin->handle) {
		ion_free(client, mcu_bin->handle);
		mcu_bin->handle = NULL;
		mcu_bin->bin_size = 0;
	}
#else
	struct device		*dev = iva->dev;
	if (mcu_bin->bin) {
		devm_kfree(dev, mcu_bin->bin);
		mcu_bin->bin = NULL;
		mcu_bin->bin_size = 0;
	}
#endif
	mcu_bin->flag = 0x0;

	return;
}

int32_t iva_mcu_probe(struct iva_dev_data *iva)
{
	int32_t		ret;
	struct device	*dev = iva->dev;

	iva->mcu_bin = kzalloc(sizeof(*iva->mcu_bin), GFP_KERNEL);
	if (!iva->mcu_bin) {
		dev_err(iva->dev, "%s() fail to alloc mem for mcu_bin struct\n",
				__func__);
		return -ENOMEM;
	}

	ret = iva_mcu_boot_mem_alloc(iva, iva->mcu_bin);
	if (ret) {
		dev_err(dev, "%s() fail to prepare mcu boot mem ret(%d)\n",
				__func__,  ret);
		goto err_boot_mem;

	}

	ret = iva_mbox_probe(iva);
	if (ret) {
		dev_err(dev, "%s() fail to probe iva mbox. ret(%d)\n",
			__func__,  ret);
		goto err_mbox_prb;
	}

	ret = iva_ipcq_probe(iva);
	if (ret) {
		dev_err(dev, "%s() fail to probe iva mbox. ret(%d)\n",
			__func__,  ret);
		goto err_ipcq_prb;
	}

	/* enable ram dump, just registration*/
	iva_ram_dump_init(iva);

	return 0;
err_ipcq_prb:
	iva_mbox_remove(iva);
err_mbox_prb:
	iva_mcu_boot_mem_free(iva, iva->mcu_bin);
err_boot_mem:
	devm_kfree(iva->dev, iva->mcu_bin);
	iva->mcu_bin = NULL;
	return ret;
}

void iva_mcu_remove(struct iva_dev_data *iva)
{
	iva_ram_dump_deinit(iva);

	iva_ipcq_remove(iva);

	iva_mbox_remove(iva);

	if (iva->mcu_bin) {
		iva_mcu_boot_mem_free(iva, iva->mcu_bin);

		devm_kfree(iva->dev, iva->mcu_bin);
		iva->mcu_bin = NULL;
	}
}

void iva_mcu_shutdown(struct iva_dev_data *iva)
{
	/* forced to block safely at minimum: - report to mcu? */
	iva_mcu_print_stop(iva);

	/* forced to block mbox interrupt */
	iva_mbox_remove(iva);
}
