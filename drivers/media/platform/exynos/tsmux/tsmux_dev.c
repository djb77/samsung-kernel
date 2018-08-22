/*
 * copyright (c) 2017 Samsung Electronics Co., Ltd.
 * http://www.samsung.com
 *
 * Core file for Samsung EXYNOS TSMUX driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/exynos_iovmm.h>

#include "tsmux_dev.h"
#include "tsmux_reg.h"
#include "tsmux_dbg.h"

#define MAX_JOB_DONE_WAIT_TIME		1000000
#define AUDIO_TIME_PERIOD_US		21333
#define ADD_NULL_TS_PACKET

#define RTP_HEADER_SIZE     12
#define TS_PACKET_SIZE      188

static struct tsmux_device *g_tsmux_dev;
int g_tsmux_debug_level;
module_param(g_tsmux_debug_level, int, 0600);

static inline int get_pes_len(int src_len, bool hdcp, bool audio)
{
	int pes_len = 0;

	pes_len = src_len;
	pes_len += 14;
	if (hdcp)
		pes_len += 17;
	if (audio)
		pes_len += 2;
	return pes_len;
}

static inline int get_ts_len(int pes_len, bool psi)
{
	int ts_len = 0;

	if (psi)
		ts_len += 3 * 188;
	ts_len += (pes_len + 183) / 184 * 188;
	return ts_len;
}

static inline int get_rtp_len(int ts_len, int num_ts_per_rtp)
{
	int rtp_len = 0;

	rtp_len = (ts_len / (num_ts_per_rtp * 188) + 1) * 12 + ts_len;
	return rtp_len;
}

static inline int get_m2m_buffer_idx(int job_id)
{
	return job_id - 1;
}

static inline int get_m2m_job_id(int buffer_idx)
{
	return buffer_idx + 1;
}

static inline bool is_m2m_job_done(struct tsmux_context *ctx)
{
	int i;
	bool ret = true;

	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		if (ctx->m2m_job_done[i] == false)
			ret = false;
	}

	return ret;
}

static inline bool is_otf_job_done(struct tsmux_context *ctx)
{
	int i;
	bool ret = false;
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		if (ctx->otf_outbuf_info[i].buf_state == BUF_DONE) {
			ret = true;
			break;
		}
	}

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	return ret;
}

static inline bool is_audio(u32 stream_type)
{
	bool is_audio = false;

	if (stream_type == TSMUX_AAC_STREAM_TYPE)
		is_audio = true;

	return is_audio;
}

static int tsmux_iommu_fault_handler(
	struct iommu_domain *domain, struct device *dev,
	unsigned long fault_addr, int fault_flags, void *token)
{
	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return 0;
}

int increment_ts_continuity_counter(
	int ts_continuity_counter, int rtp_size,
	int ts_packet_count_in_rtp, bool psi_enable)
{
	int rtp_packet_count = rtp_size / (TS_PACKET_SIZE * ts_packet_count_in_rtp + RTP_HEADER_SIZE);
	int ts_packet_count = rtp_packet_count * ts_packet_count_in_rtp;
	int rtp_remain_size = rtp_size % (TS_PACKET_SIZE * ts_packet_count_in_rtp + RTP_HEADER_SIZE);
	int ts_ramain_size = 0;

	if (rtp_remain_size > 0) {
		ts_ramain_size = rtp_remain_size - RTP_HEADER_SIZE;
		ts_packet_count += ts_ramain_size / TS_PACKET_SIZE;
	}
	if (psi_enable)
		ts_packet_count -= 3;

	ts_continuity_counter += ts_packet_count;
	ts_continuity_counter = ts_continuity_counter & 0xf;

	return ts_continuity_counter;
}

irqreturn_t tsmux_irq(int irq, void *priv)
{
	struct tsmux_device *tsmux_dev = priv;
	struct tsmux_context *ctx;
	int job_id;
	int dst_len;
	int i;
	ktime_t ktime;
	int64_t timestamp;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	tsmux_dev->irq = irq;
	ctx = tsmux_dev->ctx[tsmux_dev->ctx_cur];

	spin_lock(&tsmux_dev->device_spinlock);

	if (tsmux_is_job_done_id_0(tsmux_dev)) {
		job_id = 0;
		print_tsmux(TSMUX_COMMON, "Job ID %d is done\n", job_id);
		tsmux_clear_job_done(tsmux_dev, job_id);
		for (i = 0; i < TSMUX_OUT_BUF_CNT; i++)
			if (ctx->otf_outbuf_info[i].buf_state == BUF_Q)
				break;
		dst_len = tsmux_get_dst_len(tsmux_dev, job_id);
		print_tsmux(TSMUX_OTF, "otf outbuf num: %d, dst length: %d\n", i, dst_len);

		if (i < TSMUX_OUT_BUF_CNT) {
			ctx->otf_outbuf_info[i].buf_state = BUF_DONE;
			ctx->otf_cmd_queue.out_buf[i].actual_size = dst_len;

			ktime = ktime_get();
			timestamp = ktime_to_us(ktime);
			ctx->tsmux_end_stamp = timestamp;

			if (ctx->es_size != 0) {
				ctx->otf_cmd_queue.out_buf[i].es_size = ctx->es_size;
				wake_up_interruptible(&ctx->otf_wait_queue);
			}
		} else
			print_tsmux(TSMUX_ERR, "wrong index: %d\n", i);
	}

	if (tsmux_is_job_done_id_1(tsmux_dev)) {
		job_id = 1;
		print_tsmux(TSMUX_COMMON, "Job ID %d is done\n", job_id);
		tsmux_clear_job_done(tsmux_dev, job_id);
		ctx->m2m_job_done[get_m2m_buffer_idx(job_id)] = true;
	}

	if (tsmux_is_job_done_id_2(tsmux_dev)) {
		job_id = 2;
		print_tsmux(TSMUX_COMMON, "Job ID %d is done\n", job_id);
		tsmux_clear_job_done(tsmux_dev, job_id);
		ctx->m2m_job_done[get_m2m_buffer_idx(job_id)] = true;
	}

	if (tsmux_is_job_done_id_3(tsmux_dev)) {
		job_id = 3;
		print_tsmux(TSMUX_COMMON, "Job ID %d is done\n", job_id);
		tsmux_clear_job_done(tsmux_dev, job_id);
		ctx->m2m_job_done[get_m2m_buffer_idx(job_id)] = true;
	}

	spin_unlock(&tsmux_dev->device_spinlock);

	if (is_m2m_job_done(ctx)) {
		print_tsmux(TSMUX_COMMON, "wake_up_interruptible()\n");
		wake_up_interruptible(&ctx->m2m_wait_queue);
	}
	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return IRQ_HANDLED;
}

static int tsmux_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev = container_of(filp->private_data,
						struct tsmux_device, misc_dev);
	struct tsmux_context *ctx;
	unsigned long flags;
	int i;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (tsmux_dev->ctx_cnt >= TSMUX_MAX_CONTEXTS_NUM) {
		print_tsmux(TSMUX_ERR, "%s, too many context\n", __func__);
		ret = -ENOMEM;
		goto err_init;
	}

	ctx = kzalloc(sizeof(struct tsmux_context), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		goto err_init;
	}

	/* init ctx */
	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		ctx->m2m_cmd_queue.m2m_job[i].in_buf.ion_buf_fd = -1;
		ctx->m2m_cmd_queue.m2m_job[i].out_buf.ion_buf_fd = -1;
	}

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		ctx->otf_cmd_queue.out_buf[i].ion_buf_fd = -1;
	}

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	ctx->tsmux_dev = tsmux_dev;
	ctx->ctx_num = tsmux_dev->ctx_cnt;
	tsmux_dev->ctx[ctx->ctx_num] = ctx;
	tsmux_dev->ctx_cnt++;

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	pm_runtime_get_sync(tsmux_dev->dev);
	if (ret < 0) {
		print_tsmux(TSMUX_ERR, "pm_runtime_get_sync err(%d)\n", ret);
		return ret;
	}
#ifdef CLK_ENABLE
	ret = clk_enable(tsmux_dev->tsmux_clock);
	if (ret < 0) {
		print_tsmux(TSMUX_ERR, "clk_enable err (%d)\n", ret);
		return ret;
	}
#endif

	ctx->audio_frame_count = 0;
	ctx->video_frame_count = 0;
	ctx->set_hex_info = true;

	filp->private_data = ctx;
	print_tsmux(TSMUX_COMMON, "filp->private_data 0x%p\n",
		filp->private_data);

	g_tsmux_dev = tsmux_dev;

	init_waitqueue_head(&ctx->m2m_wait_queue);
	init_waitqueue_head(&ctx->otf_wait_queue);

	//print_tsmux_sfr(tsmux_dev);
	//print_dbg_info_all(tsmux_dev);

	tsmux_reset_pkt_ctrl(tsmux_dev);

	//print_tsmux_sfr(tsmux_dev);
	//print_dbg_info_all(tsmux_dev);

	ctx->otf_ts_continuity_counter = 0;

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return ret;

err_init:
	print_tsmux(TSMUX_COMMON, "%s--, err_init\n", __func__);

	return ret;
}

static int tsmux_release(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct tsmux_context *ctx = filp->private_data;
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);
#ifdef CLK_ENABLE
	clk_disable(tsmux_dev->tsmux_clock);
#endif
	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	ctx->tsmux_dev->ctx_cnt--;
	kfree(ctx);
	filp->private_data = NULL;

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	pm_runtime_put_sync(tsmux_dev->dev);
	if (ret < 0) {
		print_tsmux(TSMUX_ERR, "pm_runtime_put_sync err(%d)\n", ret);
		return ret;
	}

	g_tsmux_dev = NULL;

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
	return ret;
}

int tsmux_set_info(struct tsmux_context *ctx,
		struct tsmux_swp_ctrl *swp_ctrl,
		struct tsmux_hex_ctrl *hex_ctrl)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;

	/* set swap_ctrl reg*/
//	tsmux_set_swp_ctrl(tsmux_dev, swp_ctrl);

	/* secure OS will set hex regs */
	tsmux_set_hex_ctrl(ctx, hex_ctrl);

	/* enable interrupt */
	tsmux_enable_int_job_done(tsmux_dev);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return ret;
}

int tsmux_job_queue(struct tsmux_context *ctx,
		struct tsmux_pkt_ctrl *pkt_ctrl,
		struct tsmux_pes_hdr *pes_hdr,
		struct tsmux_ts_hdr *ts_hdr,
		struct tsmux_rtp_hdr *rtp_hdr,
		int32_t src_len,
		struct tsmux_buffer_info *inbuf, struct tsmux_buffer_info *outbuf)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;

	/* set pck_ctrl */
	tsmux_set_pkt_ctrl(tsmux_dev, pkt_ctrl);

	/* set pes_hdr */
	tsmux_set_pes_hdr(tsmux_dev, pes_hdr);

	/* set ts_hdr */
	if (pkt_ctrl->mode == 0)
		tsmux_set_ts_hdr(tsmux_dev, ts_hdr, ts_hdr->continuity_counter);
	else
		tsmux_set_ts_hdr(tsmux_dev, ts_hdr, ctx->otf_ts_continuity_counter);

	/* set rtp_hdr */
	tsmux_set_rtp_hdr(tsmux_dev, rtp_hdr);

	/* set src_addr_reg */
	tsmux_set_src_addr(tsmux_dev, inbuf);

	/* set src_len_reg */
	tsmux_set_src_len(tsmux_dev, src_len);

	/* set dst_addr_reg */
	tsmux_set_dst_addr(tsmux_dev, outbuf);

	/* set pkt_ctrl_reg */
	tsmux_job_queue_pkt_ctrl(tsmux_dev);

	//print_tsmux_sfr(tsmux_dev);
	//print_dbg_info_all(tsmux_dev);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return ret;
}

int tsmux_ioctl_m2m_map_buf(struct tsmux_context *ctx, int buf_fd, int buf_size,
	struct tsmux_buffer_info *buf_info)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;

	print_tsmux(TSMUX_M2M, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;

	print_tsmux(TSMUX_M2M, "map m2m in_buf\n");

	buf_info->dmabuf = dma_buf_get(buf_fd);
	print_tsmux(TSMUX_M2M, "dma_buf_get(%d) ret dmabuf %p\n",
		buf_fd, buf_info->dmabuf);

	buf_info->dmabuf_att = dma_buf_attach(buf_info->dmabuf, tsmux_dev->dev);
	print_tsmux(TSMUX_M2M, "dma_buf_attach() ret dmabuf_att %p\n",
		buf_info->dmabuf_att);

	buf_info->dma_addr = ion_iovmm_map(buf_info->dmabuf_att, 0, buf_size,
				DMA_TO_DEVICE, 0);
	print_tsmux(TSMUX_M2M, "ion_iovmm_map() ret dma_addr_t 0x%llx\n",
		buf_info->dma_addr);

	print_tsmux(TSMUX_M2M, "%s--\n", __func__);

	return ret;
}

int tsmux_ioctl_m2m_unmap_buf(struct tsmux_context *ctx,
	struct tsmux_buffer_info *buf_info)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;
	struct ion_client *client;

	print_tsmux(TSMUX_M2M, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;
	client = tsmux_dev->tsmux_ion_client;

	print_tsmux(TSMUX_M2M, "unmap m2m in_buf\n");


	if (buf_info->dma_addr) {
		print_tsmux(TSMUX_M2M, "ion_iovmm_unmap(%p, 0x%llx)\n",
			buf_info->dmabuf_att, buf_info->dma_addr);
		ion_iovmm_unmap(buf_info->dmabuf_att, buf_info->dma_addr);
		buf_info->dma_addr = 0;
	}

	if (buf_info->dmabuf_att) {
		print_tsmux(TSMUX_M2M, "dma_buf_detach(%p, %p)\n",
			buf_info->dmabuf, buf_info->dmabuf_att);
		dma_buf_detach(buf_info->dmabuf, buf_info->dmabuf_att);
		buf_info->dmabuf_att = 0;
	}

	if (buf_info->dmabuf) {
		print_tsmux(TSMUX_M2M, "dma_buf_put(%p)\n", buf_info->dmabuf);
		dma_buf_put(buf_info->dmabuf);
		buf_info->dmabuf = 0;
	}

	print_tsmux(TSMUX_M2M, "%s--\n", __func__);

	return ret;
}

int tsmux_ioctl_m2m_run(struct tsmux_context *ctx)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;
	unsigned long flags;
	struct tsmux_job *m2m_job;
	int i = 0;
	int dst_len;
	int job_id;

	print_tsmux(TSMUX_M2M, "%s++\n", __func__);

	if (ctx == NULL)
		return -EFAULT;

	tsmux_dev = ctx->tsmux_dev;

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	tsmux_dev->ctx_cur = ctx->ctx_num;

	/* init job_done[] */
	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		m2m_job = &ctx->m2m_cmd_queue.m2m_job[i];
		if (m2m_job->pes_hdr.pts39_16 != -1)
			ctx->m2m_job_done[i] = false;
		else
			ctx->m2m_job_done[i] = true;
		print_tsmux(TSMUX_M2M, "ctx->m2m_job_done[%d] %d\n",
			i, ctx->m2m_job_done[i]);
	}

	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		m2m_job = &ctx->m2m_cmd_queue.m2m_job[i];
		if (m2m_job->pes_hdr.pts39_16 != -1) {
			tsmux_set_info(ctx, &m2m_job->swp_ctrl, &m2m_job->hex_ctrl);
			tsmux_job_queue(ctx,
					&m2m_job->pkt_ctrl,
					&m2m_job->pes_hdr,
					&m2m_job->ts_hdr,
					&m2m_job->rtp_hdr,
					m2m_job->in_buf.actual_size,
					&ctx->m2m_inbuf_info[i],
					&ctx->m2m_outbuf_info[i]);
		}
	}

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	wait_event_interruptible_timeout(ctx->m2m_wait_queue,
		is_m2m_job_done(ctx), usecs_to_jiffies(MAX_JOB_DONE_WAIT_TIME));

	for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
		m2m_job = &ctx->m2m_cmd_queue.m2m_job[i];
		if (m2m_job->pes_hdr.pts39_16 != -1) {
			m2m_job->out_buf.offset = 0;
			ctx->audio_frame_count++;
			m2m_job->out_buf.time_stamp =
				ctx->audio_frame_count * AUDIO_TIME_PERIOD_US;
			job_id = get_m2m_job_id(i);
			dst_len = tsmux_get_dst_len(tsmux_dev, job_id);
			m2m_job->out_buf.actual_size = dst_len;
			print_tsmux(TSMUX_M2M, "m2m %d, dst_len_reg %d",
				i, dst_len);
		}
	}

	print_tsmux(TSMUX_M2M, "%s--\n", __func__);

	return ret;
}

int g2d_blending_start(int32_t index)
{
	int ret = 0;
	struct tsmux_context *ctx = NULL;
	ktime_t ktime;
	int64_t timestamp;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	ctx->g2d_start_stamp[index] = timestamp;
	print_tsmux(TSMUX_COMMON, "g2d_blending_start() index %d, start timestamp %lld\n",
		index, timestamp);

	return ret;
}

int g2d_blending_end(int32_t index)
{
	int ret = 0;
	struct tsmux_context *ctx = NULL;
	ktime_t ktime;
	int64_t timestamp;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	ctx->g2d_end_stamp[index] = timestamp;
	print_tsmux(TSMUX_COMMON, "g2d_blending_end() index %d, end timestamp %lld\n",
		index, timestamp);

	return ret;
}

int mfc_encoding_start(int32_t index)
{
	int ret = 0;
	struct tsmux_context *ctx = NULL;
	ktime_t ktime;
	int64_t timestamp;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	ctx->mfc_start_stamp = timestamp;
	ctx->mfc_encoding_index = index;

	print_tsmux(TSMUX_COMMON, "mfc_encoding_start() index %d, start timestamp %lld\n",
		ctx->mfc_encoding_index, timestamp);

	return ret;
}

int mfc_encoding_end(void)
{
	int ret = 0;
	struct tsmux_context *ctx = NULL;
	ktime_t ktime;
	int64_t timestamp;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	ctx->mfc_end_stamp = timestamp;
	print_tsmux(TSMUX_COMMON, "mfc_encoding_end() end timestamp %lld\n",
		timestamp);

	return ret;
}

int packetize(struct packetizing_param *param)
{
	int ret = 0;
	int index = -1;
	int i = 0;
	unsigned long flags;
	struct tsmux_context *ctx = NULL;
	struct tsmux_buffer_info *out_buf_info = NULL;
	struct tsmux_otf_config *config = NULL;
	uint8_t *psi_data = NULL;
	uint64_t pcr;
	uint64_t pcr_base;
	uint32_t pcr_ext;
	uint64_t pts = 0;
	ktime_t ktime;
	int64_t timestamp;

	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);

	if (g_tsmux_dev == NULL) {
		print_tsmux(TSMUX_ERR, "tsmux was removed\n");
		ret = -1;
		return ret;
	}

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	spin_lock_irqsave(&g_tsmux_dev->device_spinlock, flags);

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];
		if (out_buf_info->buf_state == BUF_Q) {
			print_tsmux(TSMUX_ERR, "otf command queue is full\n");
			ret = -1;
			spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);
			return ret;
		}
	}

	if (ctx->otf_outbuf_info[0].dma_addr == 0) {
		print_tsmux(TSMUX_ERR, "otf_out_buf is NULL\n");
		ret = -1;
		spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);
		return ret;
	}

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];
		if (out_buf_info->buf_state == BUF_FREE) {
			index = i;
			print_tsmux(TSMUX_OTF, "otf buf index is %d\n", index);
			break;
		}
	}

	if (index == -1) {
		print_tsmux(TSMUX_ERR, "otf buf full\n");
		ret = -1;
		spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);
		return ret;
	}

	ctx->tsmux_start_stamp = timestamp;

	pts = (param->time_stamp * 9ll) / 100ll;
	config = &ctx->otf_cmd_queue.config;

	if (ctx->otf_cmd_queue.config.pkt_ctrl.psi_en == 1) {
		/* PCR should be set by tsmux device driver */
		pcr = param->time_stamp * 27;	// PCR based on a 27MHz clock
		pcr_base = pcr / 300;
		pcr_ext = pcr % 300;

		psi_data = (char *)ctx->psi_info.psi_data;
		psi_data += ctx->psi_info.pat_len;
		psi_data += ctx->psi_info.pmt_len;

		print_tsmux(TSMUX_OTF, "pcr header %.2x %.2x %.2x %.2x %.2x %.2x\n",
			psi_data[0], psi_data[1], psi_data[2],
			psi_data[3], psi_data[4], psi_data[5]);

		psi_data += 6;		// pcr ts packet header

		*psi_data++ = (pcr_base >> 25) & 0xff;
		*psi_data++ = (pcr_base >> 17) & 0xff;
		*psi_data++ = (pcr_base >> 9) & 0xff;
		*psi_data++ = ((pcr_base & 1) << 7) | 0x7e | ((pcr_ext >> 8) & 1);
		*psi_data++ = (pcr_ext & 0xff);
	}


	/* in case of otf, PTS should be set by tsmux device driver */
	config->pes_hdr.pts39_16 = (0x20 | (((pts >> 30) & 7) << 1) | 1) << 16;
	config->pes_hdr.pts39_16 |= ((pts >> 22) & 0xff) << 8;
	config->pes_hdr.pts39_16 |= ((((pts >> 15) & 0x7f) << 1) | 1);
	config->pes_hdr.pts15_0 = ((pts >> 7) & 0xff) << 8;
	config->pes_hdr.pts15_0 |= (((pts & 0x7f) << 1) | 1);

	tsmux_set_info(ctx, &config->swp_ctrl, &config->hex_ctrl);

	ret = tsmux_job_queue(ctx,
			&config->pkt_ctrl,
			&config->pes_hdr,
			&config->ts_hdr,
			&config->rtp_hdr,
			0, 0, &ctx->otf_outbuf_info[index]);
	if (ret == 0) {
		ctx->otf_cmd_queue.out_buf[index].time_stamp = param->time_stamp;
		ctx->otf_cmd_queue.out_buf[index].es_size = 0;
		ctx->otf_outbuf_info[index].buf_state = BUF_Q;
		ctx->es_size = 0;
		print_tsmux(TSMUX_OTF, "otf buf status: BUF_FREE -> BUF_Q, index: %d\n", index);
	}

	spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);

	return ret;
}

void set_es_size(unsigned int size) {
	int i;
	unsigned long flags;
	struct tsmux_context *ctx = NULL;

	ctx = g_tsmux_dev->ctx[g_tsmux_dev->ctx_cur];

	print_tsmux(TSMUX_OTF, "es_size: %d\n", size);

	spin_lock_irqsave(&g_tsmux_dev->device_spinlock, flags);

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++)
		if (ctx->otf_outbuf_info[i].buf_state == BUF_DONE)
			break;

	if (i == TSMUX_OUT_BUF_CNT)
		ctx->es_size = size;
	else
	    ctx->otf_cmd_queue.out_buf[i].es_size = size;

	spin_unlock_irqrestore(&g_tsmux_dev->device_spinlock, flags);

	if (i != TSMUX_OUT_BUF_CNT)
	    wake_up_interruptible(&ctx->otf_wait_queue);
}

static int get_job_done_buf(struct tsmux_context *ctx)
{
	struct tsmux_buffer_info *out_buf_info = NULL;
	struct tsmux_buffer *user_info = NULL;
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;
	int index = -1;
	int i = 0;

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];
		user_info = &ctx->otf_cmd_queue.out_buf[i];

		if (out_buf_info->buf_state == BUF_DONE) {
			if (index == -1)
				index = i;
			else {
				if (ctx->otf_cmd_queue.out_buf[index].time_stamp
						> user_info->time_stamp)
					index = i;
			}
		}
	}

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	return index;
}

static bool tsmux_ioctl_otf_dq_buf(struct tsmux_context *ctx)
{
	unsigned long wait_time = 0;
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;
	int index = -1;
	int out_size = 0;
	int rtp_size = 0;
	int psi_en = 0;
	ktime_t ktime;
	int64_t timestamp;

	while ((index = get_job_done_buf(ctx)) == -1) {
		wait_time = wait_event_interruptible_timeout(ctx->otf_wait_queue,
				is_otf_job_done(ctx), HZ / 10);
		print_tsmux(TSMUX_OTF, "dq buf wait_time: %lu\n", wait_time);
		if (wait_time <= 0)
			break;
	}

	if (wait_time > 0 || index != -1) {
		spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

		out_size = ctx->otf_cmd_queue.out_buf[index].actual_size;
		rtp_size = ctx->otf_cmd_queue.config.pkt_ctrl.rtp_size;
		psi_en = ctx->otf_cmd_queue.config.pkt_ctrl.psi_en;

		ctx->otf_ts_continuity_counter =
			increment_ts_continuity_counter(
					ctx->otf_ts_continuity_counter, out_size, rtp_size, psi_en);
#ifdef ADD_NULL_TS_PACKET
		// TSMUX_HAL will add null ts packet after end of frame
		ctx->otf_ts_continuity_counter++;
		if (ctx->otf_ts_continuity_counter == 16)
			ctx->otf_ts_continuity_counter = 0;
#endif
		ctx->otf_cmd_queue.cur_buf_num = index;
		ctx->otf_outbuf_info[index].buf_state = BUF_DQ;
		print_tsmux(TSMUX_OTF, "dq buf index: %d\n", index);

		spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);
	} else {
		print_tsmux(TSMUX_ERR, "time out: wait_time: %lu\n", wait_time);
		return false;
	}

	ctx->otf_cmd_queue.out_buf[index].g2d_start_stamp = ctx->g2d_start_stamp[ctx->mfc_encoding_index];
	ctx->otf_cmd_queue.out_buf[index].g2d_end_stamp = ctx->g2d_end_stamp[ctx->mfc_encoding_index];
	ctx->otf_cmd_queue.out_buf[index].mfc_start_stamp = ctx->mfc_start_stamp;
	ctx->otf_cmd_queue.out_buf[index].mfc_end_stamp = ctx->mfc_end_stamp;
	ctx->otf_cmd_queue.out_buf[index].tsmux_start_stamp = ctx->tsmux_start_stamp;
	ctx->otf_cmd_queue.out_buf[index].tsmux_end_stamp = ctx->tsmux_end_stamp;
	ktime = ktime_get();
	timestamp = ktime_to_us(ktime);
	ctx->otf_cmd_queue.out_buf[index].kernel_end_stamp = timestamp;

	print_tsmux(TSMUX_COMMON, "mfc_encoding_index %d, g2d %lld %lld\n",
		ctx->mfc_encoding_index,
		ctx->otf_cmd_queue.out_buf[index].g2d_start_stamp,
		ctx->otf_cmd_queue.out_buf[index].g2d_end_stamp);

	return true;
}

static bool tsmux_ioctl_otf_q_buf(struct tsmux_context *ctx)
{
	struct tsmux_device *tsmux_dev = ctx->tsmux_dev;
	unsigned long flags;
	int32_t index;

	spin_lock_irqsave(&tsmux_dev->device_spinlock, flags);

	index = ctx->otf_cmd_queue.cur_buf_num;
	if (ctx->otf_outbuf_info[index].buf_state == BUF_DQ) {
		ctx->otf_outbuf_info[index].buf_state = BUF_FREE;
		print_tsmux(TSMUX_OTF, "otf buf status: BUF_DQ -> BUF_FREE, index: %d\n", index);
	} else {
		print_tsmux(TSMUX_ERR, "otf buf unexpected state: %d\n",
				ctx->otf_outbuf_info[index].buf_state);
	}

	spin_unlock_irqrestore(&tsmux_dev->device_spinlock, flags);

	return true;
}

static bool tsmux_ioctl_otf_map_buf(struct tsmux_context *ctx)
{
	int i = 0;
	struct tsmux_buffer_info *out_buf_info = NULL;
	struct tsmux_buffer *user_info = NULL;
	struct tsmux_device *tsmux_dev = NULL;

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	tsmux_dev = ctx->tsmux_dev;

	print_tsmux(TSMUX_OTF, "map otf out_buf\n");

	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];
		user_info = &ctx->otf_cmd_queue.out_buf[i];

		if (user_info->ion_buf_fd > 0) {
			out_buf_info->dmabuf =
				dma_buf_get(user_info->ion_buf_fd);
			print_tsmux(TSMUX_OTF, "dma_buf_get(%d) ret dmabuf %p\n",
					user_info->ion_buf_fd,
					out_buf_info->dmabuf);

			out_buf_info->dmabuf_att =
				dma_buf_attach(out_buf_info->dmabuf,
						tsmux_dev->dev);
			print_tsmux(TSMUX_OTF, "dma_buf_attach() ret dmabuf_att %p\n",
					out_buf_info->dmabuf_att);

			out_buf_info->dma_addr =
				ion_iovmm_map(out_buf_info->dmabuf_att,
						0, user_info->buffer_size,
						DMA_TO_DEVICE, 0);
			print_tsmux(TSMUX_OTF, "ion_iovmm_map() ret dma_addr_t 0x%llx\n",
					out_buf_info->dma_addr);

			out_buf_info->buf_state = BUF_FREE;
		}
	}

	return true;
}

int tsmux_ioctl_otf_unmap_buf(struct tsmux_context *ctx)
{
	int i = 0;
	struct tsmux_buffer_info *out_buf_info = NULL;
	int ret = 0;

	print_tsmux(TSMUX_OTF, "%s++\n", __func__);

	if (ctx == NULL || ctx->tsmux_dev == NULL)
		return -ENOMEM;

	/* free otf buffer */
	print_tsmux(TSMUX_OTF, "unmap otf out_buf\n");
	for (i = 0; i < TSMUX_OUT_BUF_CNT; i++) {
		out_buf_info = &ctx->otf_outbuf_info[i];

		if (out_buf_info->dma_addr) {
			print_tsmux(TSMUX_OTF, "ion_iovmm_unmmap(%p, 0x%llx)\n",
					out_buf_info->dmabuf_att,
					out_buf_info->dma_addr);
			ion_iovmm_unmap(out_buf_info->dmabuf_att,
					out_buf_info->dma_addr);
			out_buf_info->dma_addr = 0;
		}

		print_tsmux(TSMUX_OTF, "ion_iovmm_unmap() ok\n");

		if (out_buf_info->dmabuf_att) {
			print_tsmux(TSMUX_OTF, "dma_buf_detach(%p, %p)\n",
					out_buf_info->dmabuf,
					out_buf_info->dmabuf_att);
			dma_buf_detach(out_buf_info->dmabuf,
					out_buf_info->dmabuf_att);
			out_buf_info->dmabuf_att = 0;
		}

		print_tsmux(TSMUX_OTF, "dma_buf_detach() ok\n");

		if (out_buf_info->dmabuf) {
			print_tsmux(TSMUX_OTF, "dma_buf_put(%p)\n",
					out_buf_info->dmabuf);
			dma_buf_put(out_buf_info->dmabuf);
			out_buf_info->dmabuf = 0;
		}

		print_tsmux(TSMUX_OTF, "dma_buf_put() ok\n");
	}

	print_tsmux(TSMUX_OTF, "%s--\n", __func__);

	return ret;
}

static long tsmux_ioctl(struct file *filp,
			    unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct tsmux_context *ctx;
	struct tsmux_device *tsmux_dev;
	int i = 0;
	int buf_fd = 0;
	int buf_size = 0;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	ctx = filp->private_data;
	if (!ctx) {
		ret = -ENOTTY;
		return ret;
	}

	tsmux_dev = ctx->tsmux_dev;

	switch (cmd) {
	case TSMUX_IOCTL_SET_INFO:
		print_tsmux(TSMUX_COMMON, "TSMUX_IOCTL_SET_PSI\n");

		if (copy_from_user(&(ctx->psi_info),
			(struct tsmux_psi_info __user *)arg,
			sizeof(struct tsmux_psi_info))) {
			ret = -EFAULT;
			break;
		}

		tsmux_set_psi_info(ctx->tsmux_dev, &ctx->psi_info);
	break;

	case TSMUX_IOCTL_M2M_MAP_BUF:
		print_tsmux(TSMUX_M2M, "TSMUX_IOCTL_M2M_MAP_BUF\n");

		if (copy_from_user(&ctx->m2m_cmd_queue,
			(struct tsmux_m2m_cmd_queue __user *)arg,
			sizeof(struct tsmux_m2m_cmd_queue))) {
			ret = -EFAULT;
			break;
		}

		for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
			buf_fd = ctx->m2m_cmd_queue.m2m_job[i].in_buf.ion_buf_fd;
			buf_size = ctx->m2m_cmd_queue.m2m_job[i].in_buf.buffer_size;
			if (buf_fd > 0 && buf_size > 0) {
				tsmux_ioctl_m2m_map_buf(ctx, buf_fd, buf_size,
					&ctx->m2m_inbuf_info[i]);
			}
		}

		for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
			buf_fd = ctx->m2m_cmd_queue.m2m_job[i].out_buf.ion_buf_fd;
			buf_size = ctx->m2m_cmd_queue.m2m_job[i].out_buf.buffer_size;
			if (buf_fd > 0 && buf_size > 0) {
				tsmux_ioctl_m2m_map_buf(ctx, buf_fd, buf_size,
					&ctx->m2m_outbuf_info[i]);
			}
		}

		if (copy_to_user((struct tsmux_m2m_cmd_queue __user *)arg,
			&ctx->m2m_cmd_queue,
			sizeof(struct tsmux_m2m_cmd_queue))) {
			ret = -EFAULT;
			break;
		}
	break;

	case TSMUX_IOCTL_M2M_UNMAP_BUF:
		print_tsmux(TSMUX_M2M, "TSMUX_IOCTL_M2M_UNMAP_BUF\n");

		for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
			buf_fd = ctx->m2m_cmd_queue.m2m_job[i].in_buf.ion_buf_fd;
			buf_size = ctx->m2m_cmd_queue.m2m_job[i].in_buf.buffer_size;
			if (buf_fd > 0 && buf_size > 0) {
				tsmux_ioctl_m2m_unmap_buf(ctx,
					&ctx->m2m_inbuf_info[i]);
			}
		}

		for (i = 0; i < TSMUX_MAX_M2M_CMD_QUEUE_NUM; i++) {
			buf_fd = ctx->m2m_cmd_queue.m2m_job[i].out_buf.ion_buf_fd;
			buf_size = ctx->m2m_cmd_queue.m2m_job[i].out_buf.buffer_size;
			if (buf_fd > 0 && buf_size > 0) {
				tsmux_ioctl_m2m_unmap_buf(ctx,
					&ctx->m2m_outbuf_info[i]);
			}
		}

	break;

	case TSMUX_IOCTL_M2M_RUN:
		print_tsmux(TSMUX_M2M, "TSMUX_IOCTL_M2M_RUN\n");

		if (copy_from_user(&ctx->m2m_cmd_queue,
			(struct tsmux_m2m_cmd_queue __user *)arg,
			sizeof(struct tsmux_m2m_cmd_queue))) {
			ret = -EFAULT;
			break;
		}

		ret = tsmux_ioctl_m2m_run(ctx);

		if (copy_to_user((struct tsmux_m2m_cmd_queue __user *)arg,
			&ctx->m2m_cmd_queue,
			sizeof(struct tsmux_m2m_cmd_queue))) {
			ret = -EFAULT;
			break;
		}
	break;

	case TSMUX_IOCTL_OTF_MAP_BUF:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_MAP_BUF\n");
		if (copy_from_user(&ctx->otf_cmd_queue,
					(struct tsmux_otf_cmd_queue __user *)arg,
					sizeof(struct tsmux_otf_cmd_queue))) {
			ret = -EFAULT;
			break;
		}

		if (!tsmux_ioctl_otf_map_buf(ctx)) {
			print_tsmux(TSMUX_ERR, "map fail for dst buf\n");
			ret = -EFAULT;
			break;
		}

		if (copy_to_user((struct tsmux_otf_cmd_queue __user *) arg,
					&ctx->otf_cmd_queue, sizeof(struct tsmux_otf_cmd_queue))) {
			print_tsmux(TSMUX_ERR, "TSMUX_IOCTL_OTF_OTF_BUF: fail to copy_to_user\n");
			ret = -EFAULT;
			break;
		}
	break;

	case TSMUX_IOCTL_OTF_UNMAP_BUF:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_REL_BUF\n");
		tsmux_ioctl_otf_unmap_buf(ctx);
	break;

	case TSMUX_IOCTL_OTF_DQ_BUF:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_DQ_BUF\n");
		if (!tsmux_ioctl_otf_dq_buf(ctx)) {
			print_tsmux(TSMUX_ERR, "dq buf fail\n");
			ret = -EFAULT;
			break;
		}

		if (copy_to_user((struct tsmux_otf_cmd_queue __user *) arg,
					&ctx->otf_cmd_queue, sizeof(struct tsmux_otf_cmd_queue))) {
			print_tsmux(TSMUX_ERR, "TSMUX_IOCTL_OTF_DQ_BUF: fail to copy_to_user\n");
			ret = -EFAULT;
			break;
		}
	break;

	case TSMUX_IOCTL_OTF_Q_BUF:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_Q_BUF\n");
		if (copy_from_user(&ctx->otf_cmd_queue.cur_buf_num,
					(int32_t *)arg,
					sizeof(int32_t))) {
			ret = -EFAULT;
			break;
		}

		if (!tsmux_ioctl_otf_q_buf(ctx))
			ret = -EFAULT;
	break;

	case TSMUX_IOCTL_OTF_SET_CONFIG:
		print_tsmux(TSMUX_OTF, "TSMUX_IOCTL_OTF_SET_CONFIG\n");
		if (copy_from_user(&ctx->otf_cmd_queue.config,
					(struct tsmux_otf_config __user *)arg,
					sizeof(struct tsmux_otf_config))) {
			ret = -EFAULT;
			break;
		}
	break;

	default:
		ret = -ENOTTY;
	}

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
	return ret;
}

static const struct file_operations tsmux_fops = {
	.owner          = THIS_MODULE,
	.open           = tsmux_open,
	.release        = tsmux_release,
	.unlocked_ioctl	= tsmux_ioctl,
	.compat_ioctl = tsmux_ioctl,
};

static int tsmux_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct tsmux_device *tsmux_dev;
	struct resource *res;

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	g_tsmux_debug_level = TSMUX_ERR;

	tsmux_dev = devm_kzalloc(&pdev->dev, sizeof(struct tsmux_device),
			GFP_KERNEL);
	if (!tsmux_dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		print_tsmux(TSMUX_ERR, "failed to get memory region resource\n");
		ret = -ENOENT;
		goto err_res_mem;
	}

	tsmux_dev->tsmux_mem = request_mem_region(res->start,
					resource_size(res), pdev->name);
	if (tsmux_dev->tsmux_mem == NULL) {
		print_tsmux(TSMUX_ERR, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_req_mem;
	}

	tsmux_dev->regs_base = ioremap(tsmux_dev->tsmux_mem->start,
				resource_size(tsmux_dev->tsmux_mem));
	if (tsmux_dev->regs_base == NULL) {
		print_tsmux(TSMUX_ERR, "failed to ioremap address region\n");
		ret = -ENOENT;
		goto err_ioremap;
	}

	pm_runtime_enable(&pdev->dev);
	if (ret < 0) {
		print_tsmux(TSMUX_ERR, "Failed to pm_runtime_enable (%d)\n", ret);
		return ret;
	}

	iovmm_set_fault_handler(&pdev->dev, tsmux_iommu_fault_handler,
		tsmux_dev);

	ret = iovmm_activate(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to activate iommu\n");
		goto err_iovmm_act;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		print_tsmux(TSMUX_ERR, "failed to get irq resource\n");
		ret = -ENOENT;
		goto err_res_irq;
	}

	tsmux_dev->irq = res->start;
	ret = devm_request_irq(&pdev->dev, res->start, tsmux_irq,
				0, pdev->name, tsmux_dev);
	if (ret != 0) {
		print_tsmux(TSMUX_ERR, "failed to install irq (%d)\n", ret);
		goto err_req_irq;
	}
#ifdef CLK_ENABLE
	tsmux_dev->tsmux_clock = devm_clk_get(tsmux_dev->dev, "gate");
	if (IS_ERR(tsmux_dev->tsmux_clock)) {
		dev_err(tsmux_dev->dev, "Failed to get clock (%ld)\n",
		PTR_ERR(tsmux_dev->tsmux_clock));
		return PTR_ERR(tsmux_dev->tsmux_clock);
	}
#endif

	spin_lock_init(&tsmux_dev->device_spinlock);

	tsmux_dev->tsmux_ion_client = exynos_ion_client_create("tsmux");
	if (tsmux_dev->tsmux_ion_client == NULL)
		print_tsmux(TSMUX_ERR, "exynos_ion_client_create failed\n");

	tsmux_dev->ctx_cnt = 0;

	tsmux_dev->dev = &pdev->dev;
	tsmux_dev->misc_dev.minor = MISC_DYNAMIC_MINOR;
	tsmux_dev->misc_dev.fops = &tsmux_fops;
	tsmux_dev->misc_dev.name = NODE_NAME;
	ret = misc_register(&tsmux_dev->misc_dev);
	if (ret)
		goto err_misc_register;

	platform_set_drvdata(pdev, tsmux_dev);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return ret;

err_misc_register:
	print_tsmux(TSMUX_ERR, "err_misc_dev\n");

err_req_irq:
err_res_irq:
err_iovmm_act:
err_ioremap:
err_req_mem:
err_res_mem:

	return ret;
}

static int tsmux_remove(struct platform_device *pdev)
{
	struct tsmux_device *tsmux_dev = platform_get_drvdata(pdev);

	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	if (tsmux_dev == NULL)
		return -EFAULT;

	iovmm_deactivate(tsmux_dev->dev);

	if (tsmux_dev->tsmux_ion_client)
		ion_client_destroy(tsmux_dev->tsmux_ion_client);

	free_irq(tsmux_dev->irq, tsmux_dev);
	iounmap(tsmux_dev->regs_base);

	if (tsmux_dev) {
		misc_deregister(&tsmux_dev->misc_dev);
		kfree(tsmux_dev);
	}

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);

	return 0;
}

static void tsmux_shutdown(struct platform_device *pdev)
{
	print_tsmux(TSMUX_COMMON, "%s++\n", __func__);

	print_tsmux(TSMUX_COMMON, "%s--\n", __func__);
}

static const struct of_device_id exynos_tsmux_match[] = {
	{
		.compatible = "samsung,exynos-tsmux",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_tsmux_match);

static struct platform_driver tsmux_driver = {
	.probe		= tsmux_probe,
	.remove		= tsmux_remove,
	.shutdown	= tsmux_shutdown,
	.driver = {
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_tsmux_match),
	}
};

module_platform_driver(tsmux_driver);

MODULE_AUTHOR("Shinwon Lee <shinwon.lee@samsung.com>");
MODULE_DESCRIPTION("EXYNOS tsmux driver");
MODULE_LICENSE("GPL");
