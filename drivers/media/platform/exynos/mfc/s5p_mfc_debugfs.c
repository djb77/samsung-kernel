/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_debug.c
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include "s5p_mfc_debugfs.h"
#include "s5p_mfc_sync.h"

#include "s5p_mfc_pm.h"

#include "s5p_mfc_queue.h"

unsigned int debug_level;
unsigned int debug_ts;
unsigned int dbg_enable;
unsigned int nal_q_dump;
unsigned int nal_q_disable;
unsigned int nal_q_parallel_disable;
unsigned int otf_dump;

static int mfc_info_show(struct seq_file *s, void *unused)
{
	struct s5p_mfc_dev *dev = s->private;
	struct s5p_mfc_ctx *ctx = NULL;
	int i;
	char *codec_name = NULL;

	seq_puts(s, ">> MFC device information(common)\n");
	seq_printf(s, "[VERSION] H/W: v%x.%x, F/W: %06x(%c), DRV: %d\n",
		 MFC_VER_MAJOR(dev), MFC_VER_MINOR(dev), dev->fw.date,
		 dev->fw.fimv_info, MFC_DRIVER_INFO);
	seq_printf(s, "[PM] power: %d, clock: %d\n",
			s5p_mfc_pm_get_pwr_ref_cnt(dev), s5p_mfc_pm_get_clk_ref_cnt(dev));
	seq_printf(s, "[CTX] num_inst: %d, num_drm_inst: %d, curr_ctx: %d(is_drm: %d)\n",
			dev->num_inst, dev->num_drm_inst, dev->curr_ctx, dev->curr_ctx_is_drm);
	seq_printf(s, "[HWLOCK] bits: %#lx, dev: %#lx, owned_by_irq = %d, wl_count = %d\n",
			dev->hwlock.bits, dev->hwlock.dev,
			dev->hwlock.owned_by_irq, dev->hwlock.wl_count);
	if (dev->nal_q_handle)
		seq_printf(s, "[NAL-Q] state: %d\n", dev->nal_q_handle->nal_q_state);

	seq_puts(s, ">> MFC device information(instance)\n");
	for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
		ctx = dev->ctx[i];
		if (ctx) {
			if (ctx->type == MFCINST_DECODER)
				codec_name = ctx->src_fmt->name;
			else
				codec_name = ctx->dst_fmt->name;

			seq_printf(s, "[CTX:%d] codec: %s(%s), width: %d, height: %d, state: %d\n",
				ctx->num, ctx->type == MFCINST_DECODER ? "DEC" : "ENC", codec_name,
				ctx->img_width, ctx->img_height, ctx->state);
			seq_printf(s, "        queue(src: %d, dst: %d, src_nal: %d, dst_nal: %d, ref: %d)\n",
				s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_queue),
				s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_queue),
				s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->src_buf_nal_queue),
				s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->dst_buf_nal_queue),
				s5p_mfc_get_queue_count(&ctx->buf_queue_lock, &ctx->ref_buf_queue));
		}
	}

	return 0;
}

static int mfc_debug_info_show(struct seq_file *s, void *unused)
{
	seq_puts(s, ">> MFC debug information\n");
	return 0;
}

static int mfc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, mfc_info_show, inode->i_private);
}

static int mfc_debug_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, mfc_debug_info_show, inode->i_private);
}

static const struct file_operations mfc_info_fops = {
	.open = mfc_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations debug_info_fops = {
	.open = mfc_debug_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void s5p_mfc_init_debugfs(struct s5p_mfc_dev *dev)
{
	struct s5p_mfc_debugfs *debugfs = &dev->debugfs;

	debugfs->root = debugfs_create_dir("mfc", NULL);
	if (!debugfs->root) {
		mfc_err_dev("debugfs: failed to create root derectory.\n");
		return;
	}

	debugfs->mfc_info = debugfs_create_file("mfc_info",
			0444, debugfs->root, dev, &mfc_info_fops);
	debugfs->debug_info = debugfs_create_file("debug_info",
			0444, debugfs->root, dev, &debug_info_fops);
	debugfs->debug_level = debugfs_create_u32("debug",
			0644, debugfs->root, &debug_level);
	debugfs->debug_ts = debugfs_create_u32("debug_ts",
			0644, debugfs->root, &debug_ts);
	debugfs->dbg_enable = debugfs_create_u32("dbg_enable",
			0644, debugfs->root, &dbg_enable);
	debugfs->nal_q_dump = debugfs_create_u32("nal_q_dump",
			0644, debugfs->root, &nal_q_dump);
	debugfs->nal_q_disable = debugfs_create_u32("nal_q_disable",
			0644, debugfs->root, &nal_q_disable);
	debugfs->nal_q_parallel_disable = debugfs_create_u32("nal_q_parallel_disable",
			0644, debugfs->root, &nal_q_parallel_disable);
	debugfs->otf_dump = debugfs_create_u32("otf_dump",
			0644, debugfs->root, &otf_dump);
}
