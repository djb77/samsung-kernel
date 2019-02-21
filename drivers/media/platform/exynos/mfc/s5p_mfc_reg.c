/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_reg.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>

#include "s5p_mfc_reg.h"

#include "s5p_mfc_mem.h"

/* Reset the device */
int s5p_mfc_reset_mfc(struct s5p_mfc_dev *dev)
{
	int i;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	/* Zero Initialization of MFC registers */
	MFC_WRITEL(0, S5P_FIMV_RISC2HOST_CMD);
	MFC_WRITEL(0, S5P_FIMV_HOST2RISC_CMD);
	MFC_WRITEL(0, S5P_FIMV_FW_VERSION);

	for (i = 0; i < S5P_FIMV_REG_CLEAR_COUNT; i++)
		MFC_WRITEL(0, S5P_FIMV_REG_CLEAR_BEGIN + (i*4));

	MFC_WRITEL(0x1FFF, S5P_FIMV_MFC_RESET);
	MFC_WRITEL(0, S5P_FIMV_MFC_RESET);

	mfc_debug_leave();

	return 0;
}

void s5p_mfc_dbg_enable(struct s5p_mfc_dev *dev)
{
	mfc_debug(2, "MFC debug info enable\n");
	MFC_WRITEL(0x1, S5P_FIMV_DBG_INFO_ENABLE);
}

void s5p_mfc_dbg_disable(struct s5p_mfc_dev *dev)
{
	mfc_debug(2, "MFC debug info disable\n");
	MFC_WRITEL(0x0, S5P_FIMV_DBG_INFO_ENABLE);
}

void s5p_mfc_dbg_set_addr(struct s5p_mfc_dev *dev)
{
	struct s5p_mfc_buf_size_v6 *buf_size = dev->variant->buf_size->buf;

	memset((void *)dev->dbg_info_buf.vaddr, 0, buf_size->dbg_info_buf);

	mfc_debug(2, "MFC debug info set addr(0x%08x), size(0x%08lx)\n",
			(unsigned int)dev->dbg_info_buf.daddr, buf_size->dbg_info_buf);
	MFC_WRITEL(dev->dbg_info_buf.daddr, S5P_FIMV_DBG_BUFFER_ADDR);
	MFC_WRITEL(buf_size->dbg_info_buf, S5P_FIMV_DBG_BUFFER_SIZE);
}

/* Set decoding frame buffer */
int s5p_mfc_set_dec_codec_buffers(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	unsigned int i, frame_size_mv;
	dma_addr_t buf_addr1;
	int buf_size1;
	int align_gap;
	struct s5p_mfc_raw_info *raw, *tiled_ref;
	unsigned int reg = 0;

	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}

	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}

	/* only 2 plane is supported for HEVC 10bit */
	if (ctx->is_10bit)
		MFC_WRITEL(0, S5P_FIMV_PIXEL_FORMAT);

	raw = &ctx->raw_buf;
	tiled_ref = &dec->tiled_ref;
	buf_addr1 = ctx->codec_buf.daddr;
	buf_size1 = ctx->codec_buf.size;

	mfc_debug(2, "Buf1: %p (%d)\n", (void *)buf_addr1, buf_size1);
	mfc_debug(2, "Total DPB COUNT: %d\n", dec->total_dpb_count);
	mfc_debug(2, "Setting display delay to %d\n", dec->display_delay);

	MFC_WRITEL(dec->total_dpb_count, S5P_FIMV_D_NUM_DPB);
	mfc_debug(2, "raw->num_planes %d\n", raw->num_planes);
	for (i = 0; i < raw->num_planes; i++) {
		mfc_debug(2, "raw->plane_size[%d]= %d\n", i, raw->plane_size[i]);
		MFC_WRITEL(raw->plane_size[i], S5P_FIMV_D_FIRST_PLANE_DPB_SIZE + (i * 4));
	}

	MFC_WRITEL(buf_addr1, S5P_FIMV_D_SCRATCH_BUFFER_ADDR);
	MFC_WRITEL(ctx->scratch_buf_size, S5P_FIMV_D_SCRATCH_BUFFER_SIZE);
	buf_addr1 += ctx->scratch_buf_size;
	buf_size1 -= ctx->scratch_buf_size;

	if (IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) || IS_HEVC_DEC(ctx))
		MFC_WRITEL(ctx->mv_size, S5P_FIMV_D_MV_BUFFER_SIZE);

	if (IS_VP9_DEC(ctx)){
		MFC_WRITEL(buf_addr1, S5P_FIMV_D_STATIC_BUFFER_ADDR);
		MFC_WRITEL(DEC_STATIC_BUFFER_SIZE, S5P_FIMV_D_STATIC_BUFFER_SIZE);
		buf_addr1 += DEC_STATIC_BUFFER_SIZE;
		buf_size1 -= DEC_STATIC_BUFFER_SIZE;
	}

	if (IS_MPEG4_DEC(ctx)) {
		if (dec->loop_filter_mpeg4) {
			if (dec->total_dpb_count >=
					(ctx->dpb_count + NUM_MPEG4_LF_BUF)) {
				if (!dec->internal_dpb) {
					dec->internal_dpb = NUM_MPEG4_LF_BUF;
					ctx->dpb_count += dec->internal_dpb;
				}
			} else {
				dec->loop_filter_mpeg4 = 0;
				mfc_debug(2, "failed to enable loop filter\n");
			}
		}
		reg |= ((dec->loop_filter_mpeg4 & S5P_FIMV_D_INIT_BUF_OPT_LF_CTRL_MASK)
				<< S5P_FIMV_D_INIT_BUF_OPT_LF_CTRL_SHIFT);
	}

	reg |= (0x1 << S5P_FIMV_D_INIT_BUF_OPT_DYNAMIC_DPB_SET_SHIFT);

	if (CODEC_NOT_CODED(ctx)) {
		reg |= (0x1 << S5P_FIMV_D_INIT_BUF_OPT_COPY_NOT_CODED_SHIFT);
		mfc_info_ctx("Notcoded frame copy mode start\n");
	}
	/* Enable 10bit Dithering */
	if (ctx->is_10bit)
		reg |= (0x1 << S5P_FIMV_D_INIT_BUF_OPT_DITHERING_EN_SHIFT);

	MFC_WRITEL(reg, S5P_FIMV_D_INIT_BUFFER_OPTIONS);

	frame_size_mv = ctx->mv_size;
	mfc_debug(2, "Frame size: %d, %d, %d, mv: %d\n",
			raw->plane_size[0], raw->plane_size[1],
			raw->plane_size[2], frame_size_mv);

	if (dec->dst_memtype != V4L2_MEMORY_USERPTR && dec->dst_memtype != V4L2_MEMORY_DMABUF)
		mfc_err_dev("Not supported memory type (%d)\n", dec->dst_memtype);

	/* set decoder stride size */
	for (i = 0; i < ctx->raw_buf.num_planes; i++) {
		MFC_WRITEL(ctx->raw_buf.stride[i],
			S5P_FIMV_D_FIRST_PLANE_DPB_STRIDE_SIZE + (i * 4));
		mfc_debug(2, "# plane%d.size = %d, stride = %d\n", i,
			ctx->raw_buf.plane_size[i], ctx->raw_buf.stride[i]);
	}

	if (ctx->is_10bit) {
		for (i = 0; i < ctx->raw_buf.num_planes; i++) {
			MFC_WRITEL(raw->stride_2bits[i], S5P_FIMV_D_FIRST_PLANE_2BIT_DPB_STRIDE_SIZE + (i * 4));
			MFC_WRITEL(raw->plane_size_2bits[i], S5P_FIMV_D_FIRST_PLANE_2BIT_DPB_SIZE + (i * 4));
			mfc_debug(2, "# HEVC 10bit : 2bits plane%d.size = %d, stride = %d\n", i,
				ctx->raw_buf.plane_size_2bits[i], ctx->raw_buf.stride_2bits[i]);
		}
	}

	MFC_WRITEL(dec->mv_count, S5P_FIMV_D_NUM_MV);
	if (IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) || IS_HEVC_DEC(ctx)) {
		for (i = 0; i < dec->mv_count; i++) {
			/* To test alignment */
			align_gap = buf_addr1;
			buf_addr1 = ALIGN(buf_addr1, 16);
			align_gap = buf_addr1 - align_gap;
			buf_size1 -= align_gap;

			mfc_debug(2, "\tBuf1: %p, size: %d\n", (void *)buf_addr1, buf_size1);
			MFC_WRITEL(buf_addr1, S5P_FIMV_D_MV_BUFFER0 + i * 4);
			buf_addr1 += frame_size_mv;
			buf_size1 -= frame_size_mv;
		}
	}

	mfc_debug(2, "Buf1: %p, buf_size1: %d (frames %d)\n",
			(void *)buf_addr1, buf_size1, dec->total_dpb_count);
	if (buf_size1 < 0) {
		mfc_debug(2, "Not enough memory has been allocated.\n");
		return -ENOMEM;
	}

	mfc_debug(2, "After setting buffers.\n");
	return 0;
}

/* Set encoding ref & codec buffer */
int s5p_mfc_set_enc_codec_buffers(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_enc *enc = ctx->enc_priv;
	dma_addr_t buf_addr1;
	int buf_size1;
	int i;

	mfc_debug_enter();

	buf_addr1 = ctx->codec_buf.daddr;
	buf_size1 = ctx->codec_buf.size;

	mfc_debug(2, "Buf1: %p (%d)\n", (void *)buf_addr1, buf_size1);

	MFC_WRITEL(buf_addr1, S5P_FIMV_E_SCRATCH_BUFFER_ADDR);
	MFC_WRITEL(ctx->scratch_buf_size, S5P_FIMV_E_SCRATCH_BUFFER_SIZE);
	buf_addr1 += ctx->scratch_buf_size;
	buf_size1 -= ctx->scratch_buf_size;

	/* start address of per buffer is aligned */
	for (i = 0; i < ctx->dpb_count; i++) {
		MFC_WRITEL(buf_addr1, S5P_FIMV_E_LUMA_DPB + (4 * i));
		buf_addr1 += enc->luma_dpb_size;
		buf_size1 -= enc->luma_dpb_size;
	}
	for (i = 0; i < ctx->dpb_count; i++) {
		MFC_WRITEL(buf_addr1, S5P_FIMV_E_CHROMA_DPB + (4 * i));
		buf_addr1 += enc->chroma_dpb_size;
		buf_size1 -= enc->chroma_dpb_size;
	}
	for (i = 0; i < ctx->dpb_count; i++) {
		MFC_WRITEL(buf_addr1, S5P_FIMV_E_ME_BUFFER + (4 * i));
		buf_addr1 += enc->me_buffer_size;
		buf_size1 -= enc->me_buffer_size;
	}

	MFC_WRITEL(buf_addr1, S5P_FIMV_E_TMV_BUFFER0);
	buf_addr1 += enc->tmv_buffer_size >> 1;
	MFC_WRITEL(buf_addr1, S5P_FIMV_E_TMV_BUFFER1);
	buf_addr1 += enc->tmv_buffer_size >> 1;
	buf_size1 -= enc->tmv_buffer_size;

	mfc_debug(2, "Buf1: %p, buf_size1: %d (ref frames %d)\n",
			(void *)buf_addr1, buf_size1, ctx->dpb_count);
	if (buf_size1 < 0) {
		mfc_debug(2, "Not enough memory has been allocated.\n");
		return -ENOMEM;
	}

	mfc_debug_leave();

	return 0;
}

/* Set registers for decoding stream buffer */
int s5p_mfc_set_dec_stream_buffer(struct s5p_mfc_ctx *ctx, struct s5p_mfc_buf *mfc_buf,
		  unsigned int start_num_byte, unsigned int strm_size)
{
	struct s5p_mfc_dev *dev;
	struct s5p_mfc_dec *dec;
	unsigned int cpb_buf_size;
	dma_addr_t addr;

	mfc_debug_enter();
	if (!ctx) {
		mfc_err_dev("no mfc context to run\n");
		return -EINVAL;
	}
	dev = ctx->dev;
	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}
	dec = ctx->dec_priv;
	if (!dec) {
		mfc_err_dev("no mfc decoder to run\n");
		return -EINVAL;
	}

	cpb_buf_size = ALIGN(dec->src_buf_size, STREAM_BUF_ALIGN);

	if (mfc_buf) {
		addr = s5p_mfc_mem_get_daddr_vb(&mfc_buf->vb.vb2_buf, 0);
		if (strm_size > set_strm_size_max(cpb_buf_size)) {
			mfc_info_ctx("Decrease strm_size : %u -> %u, gap : %d\n",
					strm_size, set_strm_size_max(cpb_buf_size),
					STREAM_BUF_ALIGN);
			strm_size = set_strm_size_max(cpb_buf_size);
			mfc_buf->vb.vb2_buf.planes[0].bytesused = strm_size;
		}
	} else {
		addr = 0;
	}

	mfc_debug(2, "inst_no: %d, buf_addr: 0x%08llx\n", ctx->inst_no, addr);
	mfc_debug(2, "strm_size: %u cpb_buf_size: %u offset: %u\n",
			strm_size, cpb_buf_size, start_num_byte);

	if (strm_size == 0)
		mfc_info_ctx("stream size is 0\n");

	MFC_WRITEL(strm_size, S5P_FIMV_D_STREAM_DATA_SIZE);
	MFC_WRITEL(addr, S5P_FIMV_D_CPB_BUFFER_ADDR);
	MFC_WRITEL(cpb_buf_size, S5P_FIMV_D_CPB_BUFFER_SIZE);
	MFC_WRITEL(start_num_byte, S5P_FIMV_D_CPB_BUFFER_OFFSET);
	ctx->last_src_addr = addr;

	if (mfc_buf)
		MFC_TRACE_CTX("Set src[%d] fd: %d, %#llx\n",
				mfc_buf->vb.vb2_buf.index,
				mfc_buf->vb.vb2_buf.planes[0].m.fd,
				addr);

	mfc_debug_leave();
	return 0;
}

void s5p_mfc_set_enc_frame_buffer(struct s5p_mfc_ctx *ctx,
		dma_addr_t addr[], int num_planes)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	int i;

	for (i = 0; i < num_planes; i++)
		MFC_WRITEL(addr[i], S5P_FIMV_E_SOURCE_FIRST_ADDR + (i*4));
}

/* Set registers for encoding stream buffer */
int s5p_mfc_set_enc_stream_buffer(struct s5p_mfc_ctx *ctx,
		struct s5p_mfc_buf *mfc_buf)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	dma_addr_t addr;
	unsigned int size;

	addr = s5p_mfc_mem_get_daddr_vb(&mfc_buf->vb.vb2_buf, 0);
	size = (unsigned int)vb2_plane_size(&mfc_buf->vb.vb2_buf, 0);
	size = ALIGN(size, 512);

	MFC_WRITEL(addr, S5P_FIMV_E_STREAM_BUFFER_ADDR); /* 16B align */
	MFC_WRITEL(size, S5P_FIMV_E_STREAM_BUFFER_SIZE);

	mfc_debug(2, "stream buf addr: 0x%08llx, size: 0x%08x(%d)\n", addr, size, size);

	return 0;
}

void s5p_mfc_get_enc_frame_buffer(struct s5p_mfc_ctx *ctx,
		dma_addr_t addr[], int num_planes)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	unsigned long enc_recon_y_addr, enc_recon_c_addr;
	int i, addr_offset;

	addr_offset = S5P_FIMV_E_ENCODED_SOURCE_FIRST_ADDR;

	for (i = 0; i < num_planes; i++)
		addr[i] = MFC_READL(addr_offset + (i * 4));

	enc_recon_y_addr = MFC_READL(S5P_FIMV_E_RECON_LUMA_DPB_ADDR);
	enc_recon_c_addr = MFC_READL(S5P_FIMV_E_RECON_CHROMA_DPB_ADDR);

	mfc_debug(2, "recon y addr: 0x%08lx\n", enc_recon_y_addr);
	mfc_debug(2, "recon c addr: 0x%08lx\n", enc_recon_c_addr);
}

void s5p_mfc_set_enc_stride(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	int i;

	for (i = 0; i < ctx->raw_buf.num_planes; i++) {
		MFC_WRITEL(ctx->raw_buf.stride[i],
				S5P_FIMV_E_SOURCE_FIRST_STRIDE + (i * 4));
		mfc_debug(2, "enc src[%d] stride: 0x%08lx\n",
				i, (unsigned long)ctx->raw_buf.stride[i]);
	}
}

void s5p_mfc_set_risc_base_addr(struct s5p_mfc_dev *dev,
					enum mfc_buf_usage_type buf_type)
{
	struct s5p_mfc_special_buf *fw_buf;

	fw_buf = &dev->fw_buf;

	if (buf_type == MFCBUF_DRM)
		fw_buf = &dev->drm_fw_buf;

	MFC_WRITEL(fw_buf->daddr, S5P_FIMV_RISC_BASE_ADDRESS);
	mfc_debug(2, "[%d] Base Address : %08llx\n", buf_type, fw_buf->daddr);
}

int s5p_mfc_set_dynamic_dpb(struct s5p_mfc_ctx *ctx, struct s5p_mfc_buf *dst_mb)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	struct s5p_mfc_raw_info *raw = &ctx->raw_buf;
	int dst_index;
	int i;

	dst_index = dst_mb->vb.vb2_buf.index;
	set_bit(dst_index, &dec->available_dpb);
	dec->dynamic_set = 1 << dst_index;
	mfc_debug(2, "ADDING Flag after: 0x%lx\n", dec->available_dpb);
	mfc_debug(2, "Dst addr [%d] = 0x%08llx\n", dst_index, dst_mb->planes.raw[0]);

	/* for debugging about black bar detection */
	if (FW_HAS_BLACK_BAR_DETECT(dev) && dec->detect_black_bar) {
		for (i = 0; i < raw->num_planes; i++) {
			dec->frame_vaddr[i][dec->frame_cnt] = vb2_plane_vaddr(&dst_mb->vb.vb2_buf, i);
			dec->frame_daddr[i][dec->frame_cnt] = dst_mb->planes.raw[i];
			dec->frame_size[i][dec->frame_cnt] = raw->plane_size[i];
			dec->index[i][dec->frame_cnt] = dst_index;
			dec->fd[i][dec->frame_cnt] = dst_mb->vb.vb2_buf.planes[0].m.fd;
		}
		dec->frame_cnt++;
		if (dec->frame_cnt >= 30)
			dec->frame_cnt = 0;
	}

	/* decoder dst buffer CFW PROT */
	s5p_mfc_protect_dpb(ctx, dst_mb);

	for (i = 0; i < raw->num_planes; i++) {
		MFC_WRITEL(raw->plane_size[i],
				S5P_FIMV_D_FIRST_PLANE_DPB_SIZE + i * 4);
		MFC_WRITEL(dst_mb->planes.raw[i],
				S5P_FIMV_D_FIRST_PLANE_DPB0 + (i * 0x100 + dst_index * 4));
		if (ctx->is_10bit)
			MFC_WRITEL(raw->plane_size_2bits[i],
					S5P_FIMV_D_FIRST_PLANE_2BIT_DPB_SIZE + (i * 4));
		ctx->last_dst_addr[i] = dst_mb->planes.raw[i];
	}

	MFC_TRACE_CTX("Set dst[%d] fd: %d, %#llx / avail %#lx used %#x\n",
			dst_index, dst_mb->vb.vb2_buf.planes[0].m.fd, dst_mb->planes.raw[0],
			dec->available_dpb, dec->dynamic_used);

	return 0;
}
