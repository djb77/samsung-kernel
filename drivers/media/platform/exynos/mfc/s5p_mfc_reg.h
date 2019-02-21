/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_reg.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_REG_H
#define __S5P_MFC_REG_H __FILE__

#include <linux/io.h>

#include "s5p_mfc_common.h"

#include "s5p_mfc_utils.h"

#define MFC_READL(offset)		readl(dev->regs_base + (offset))
#define MFC_WRITEL(data, offset)	writel((data), dev->regs_base + (offset))

#define MFC_MMU0_READL(offset)		readl(dev->sysmmu0_base + (offset))
#define MFC_MMU1_READL(offset)		readl(dev->sysmmu1_base + (offset))

/* version */
#define s5p_mfc_get_fimv_info()		((MFC_READL(S5P_FIMV_FW_VERSION)		\
						>> S5P_FIMV_FW_VER_INFO_SHFT)		\
						& S5P_FIMV_FW_VER_INFO_MASK)
#define s5p_mfc_get_fw_ver_year()	((MFC_READL(S5P_FIMV_FW_VERSION)		\
						>> S5P_FIMV_FW_VER_YEAR_SHFT)		\
						& S5P_FIMV_FW_VER_YEAR_MASK)
#define s5p_mfc_get_fw_ver_month()	((MFC_READL(S5P_FIMV_FW_VERSION)		\
						>> S5P_FIMV_FW_VER_MONTH_SHFT)		\
						& S5P_FIMV_FW_VER_MONTH_MASK)
#define s5p_mfc_get_fw_ver_date()	((MFC_READL(S5P_FIMV_FW_VERSION)		\
						>> S5P_FIMV_FW_VER_DATE_SHFT)		\
						& S5P_FIMV_FW_VER_DATE_MASK)
#define s5p_mfc_get_fw_ver_all()	((MFC_READL(S5P_FIMV_FW_VERSION)		\
						>> S5P_FIMV_FW_VER_ALL_SHFT)		\
						& S5P_FIMV_FW_VER_ALL_MASK)
#define s5p_mfc_get_mfc_version()	((MFC_READL(S5P_FIMV_MFC_VERSION)		\
						>> S5P_FIMV_MFC_VER_SHFT)		\
						& S5P_FIMV_MFC_VER_MASK)


/* decoding & display information */
#define s5p_mfc_get_dec_status()	(MFC_READL(S5P_FIMV_D_DECODED_STATUS)		\
						& S5P_FIMV_DEC_STATUS_DECODED_STATUS_MASK)
#define s5p_mfc_get_disp_status()	(MFC_READL(S5P_FIMV_D_DISPLAY_STATUS)		\
						& S5P_FIMV_DISP_STATUS_DISPLAY_STATUS_MASK)
#define s5p_mfc_get_res_change()	((MFC_READL(S5P_FIMV_D_DISPLAY_STATUS)		\
						>> S5P_FIMV_DISP_STATUS_RES_CHANGE_SHIFT)	\
						& S5P_FIMV_DISP_STATUS_RES_CHANGE_MASK)
#define s5p_mfc_get_black_bar_detection()	((MFC_READL(S5P_FIMV_D_DISPLAY_STATUS)		\
						>> S5P_FIMV_DISP_STATUS_BLACK_BAR_DETECT_SHIFT)	\
						& S5P_FIMV_DISP_STATUS_BLACK_BAR_DETECT_MASK)
#define s5p_mfc_get_dpb_change()	((MFC_READL(S5P_FIMV_D_DISPLAY_STATUS)		\
						>> S5P_FIMV_DISP_STATUS_NEED_DPB_CHANGE_SHIFT)	\
						& S5P_FIMV_DISP_STATUS_NEED_DPB_CHANGE_MASK)
#define s5p_mfc_get_scratch_change()	((MFC_READL(S5P_FIMV_D_DISPLAY_STATUS)		\
						>> S5P_FIMV_DISP_STATUS_NEED_SCRATCH_CHANGE_SHIFT)	\
						& S5P_FIMV_DISP_STATUS_NEED_SCRATCH_CHANGE_MASK)
#define s5p_mfc_get_disp_frame_type()	(MFC_READL(S5P_FIMV_D_DISPLAY_FRAME_TYPE)	\
						& S5P_FIMV_DISPLAY_FRAME_MASK)
#define s5p_mfc_get_dec_frame_type()	(MFC_READL(S5P_FIMV_D_DECODED_FRAME_TYPE)	\
						& S5P_FIMV_DECODED_FRAME_MASK)
#define s5p_mfc_get_interlace_type()	((MFC_READL(S5P_FIMV_D_DISPLAY_FRAME_TYPE)	\
						>> S5P_FIMV_DISPLAY_TEMP_INFO_SHIFT)	\
						& S5P_FIMV_DISPLAY_TEMP_INFO_MASK)
#define s5p_mfc_is_interlace_picture()	((MFC_READL(S5P_FIMV_D_DISPLAY_STATUS)		\
						>> S5P_FIMV_DISP_STATUS_INTERLACE_SHIFT)\
						& S5P_FIMV_DISP_STATUS_INTERLACE_MASK)
#define s5p_mfc_get_img_width()		MFC_READL(S5P_FIMV_D_DISPLAY_FRAME_WIDTH)
#define s5p_mfc_get_img_height()	MFC_READL(S5P_FIMV_D_DISPLAY_FRAME_HEIGHT)
#define s5p_mfc_get_disp_y_addr()	MFC_READL(S5P_FIMV_D_DISPLAY_LUMA_ADDR)
#define s5p_mfc_get_dec_y_addr()	MFC_READL(S5P_FIMV_D_DECODED_LUMA_ADDR)


/* kind of interrupt */
#define s5p_mfc_get_int_reason()	(MFC_READL(S5P_FIMV_RISC2HOST_CMD)		\
						& S5P_FIMV_RISC2HOST_CMD_MASK)
#define s5p_mfc_get_int_err()		MFC_READL(S5P_FIMV_ERROR_CODE)
#define s5p_mfc_get_err(x)		(((x) >> S5P_FIMV_ERR_STATUS_SHIFT)		\
						& S5P_FIMV_ERR_STATUS_MASK)
#define s5p_mfc_get_warn(x)		(((x) >> S5P_FIMV_WARN_STATUS_SHIFT)		\
						& S5P_FIMV_WARN_STATUS_MASK)


/* additional information */
#define s5p_mfc_get_consumed_stream()		MFC_READL(S5P_FIMV_D_DECODED_NAL_SIZE)
#define s5p_mfc_get_dpb_count()			MFC_READL(S5P_FIMV_D_MIN_NUM_DPB)
#define s5p_mfc_get_min_dpb_size(x)		MFC_READL(S5P_FIMV_D_MIN_FIRST_PLANE_DPB_SIZE + (x * 4))
#define s5p_mfc_get_scratch_size()		MFC_READL(S5P_FIMV_D_MIN_SCRATCH_BUFFER_SIZE)
#define s5p_mfc_get_mv_count()			MFC_READL(S5P_FIMV_D_MIN_NUM_MV)
#define s5p_mfc_get_inst_no()			MFC_READL(S5P_FIMV_RET_INSTANCE_ID)
#define s5p_mfc_get_enc_dpb_count()		MFC_READL(S5P_FIMV_E_NUM_DPB)
#define s5p_mfc_get_enc_scratch_size()		MFC_READL(S5P_FIMV_E_MIN_SCRATCH_BUFFER_SIZE)
#define s5p_mfc_get_enc_strm_size()		MFC_READL(S5P_FIMV_E_STREAM_SIZE)
#define s5p_mfc_get_enc_slice_type()		MFC_READL(S5P_FIMV_E_SLICE_TYPE)
#define s5p_mfc_get_enc_pic_count()		MFC_READL(S5P_FIMV_E_PICTURE_COUNT)
#define s5p_mfc_get_sei_avail()			MFC_READL(S5P_FIMV_D_SEI_AVAIL)
#define s5p_mfc_get_sei_content_light()		MFC_READL(S5P_FIMV_D_CONTENT_LIGHT_LEVEL_INFO_SEI)
#define s5p_mfc_get_sei_mastering0()		MFC_READL(S5P_FIMV_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_0)
#define s5p_mfc_get_sei_mastering1()		MFC_READL(S5P_FIMV_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_1)
#define s5p_mfc_get_sei_mastering2()		MFC_READL(S5P_FIMV_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_2)
#define s5p_mfc_get_sei_mastering3()		MFC_READL(S5P_FIMV_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_3)
#define s5p_mfc_get_sei_mastering4()		MFC_READL(S5P_FIMV_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_4)
#define s5p_mfc_get_sei_mastering5()		MFC_READL(S5P_FIMV_D_MASTERING_DISPLAY_COLOUR_VOLUME_SEI_5)
#define s5p_mfc_get_sei_avail_frame_pack()	(MFC_READL(S5P_FIMV_D_SEI_AVAIL)	\
						& S5P_FIMV_D_SEI_AVAIL_FRAME_PACK_MASK)
#define s5p_mfc_get_sei_avail_content_light()	((MFC_READL(S5P_FIMV_D_SEI_AVAIL)	\
						>> S5P_FIMV_D_SEI_AVAIL_CONTENT_LIGHT_SHIFT)	\
						& S5P_FIMV_D_SEI_AVAIL_CONTENT_LIGHT_MASK)
#define s5p_mfc_get_sei_avail_mastering_display()	((MFC_READL(S5P_FIMV_D_SEI_AVAIL)	\
						>> S5P_FIMV_D_SEI_AVAIL_MASTERING_DISPLAY_SHIFT)	\
						& S5P_FIMV_D_SEI_AVAIL_MASTERING_DISPLAY_MASK)
#define s5p_mfc_get_video_signal_type()		((MFC_READL(S5P_FIMV_D_VIDEO_SIGNAL_TYPE)	\
						>> S5P_FIMV_D_VIDEO_SIGNAL_TYPE_FLAG_SHIFT)	\
						& S5P_FIMV_D_VIDEO_SIGNAL_TYPE_FLAG_MASK)
#define s5p_mfc_get_colour_description()	((MFC_READL(S5P_FIMV_D_VIDEO_SIGNAL_TYPE)	\
						>> S5P_FIMV_D_COLOUR_DESCRIPTION_FLAG_SHIFT)	\
						& S5P_FIMV_D_COLOUR_DESCRIPTION_FLAG_MASK)
#define s5p_mfc_get_black_bar_pos_x()		((MFC_READL(S5P_FIMV_D_BLACK_BAR_START_POS)	\
						>> S5P_FIMV_D_BLACK_BAR_START_X_SHIFT)		\
						& S5P_FIMV_D_BLACK_BAR_START_X_MASK)
#define s5p_mfc_get_black_bar_pos_y()		((MFC_READL(S5P_FIMV_D_BLACK_BAR_START_POS)	\
						>> S5P_FIMV_D_BLACK_BAR_START_Y_SHIFT)		\
						& S5P_FIMV_D_BLACK_BAR_START_Y_MASK)
#define s5p_mfc_get_black_bar_image_w()		((MFC_READL(S5P_FIMV_D_BLACK_BAR_IMAGE_SIZE)	\
						>> S5P_FIMV_D_BLACK_BAR_IMAGE_W_SHIFT)	\
						& S5P_FIMV_D_BLACK_BAR_IMAGE_W_MASK)
#define s5p_mfc_get_black_bar_image_h()		((MFC_READL(S5P_FIMV_D_BLACK_BAR_IMAGE_SIZE)	\
						>> S5P_FIMV_D_BLACK_BAR_IMAGE_H_SHIFT)	\
						& S5P_FIMV_D_BLACK_BAR_IMAGE_H_MASK)
#define s5p_mfc_get_mvc_disp_view_id()		(MFC_READL(S5P_FIMV_D_MVC_VIEW_ID)		\
						& S5P_FIMV_D_MVC_VIEW_ID_DISP_MASK)
#define s5p_mfc_get_profile()			(MFC_READL(S5P_FIMV_D_DECODED_PICTURE_PROFILE)	\
						& S5P_FIMV_D_DECODED_PIC_PROFILE_MASK)
#define s5p_mfc_get_bit_depth_minus8()		((MFC_READL(S5P_FIMV_D_DECODED_PICTURE_PROFILE)	\
						>> S5P_FIMV_D_BIT_DEPTH_LUMA_MINUS8_SHIFT)	\
						& S5P_FIMV_D_BIT_DEPTH_LUMA_MINUS8_MASK)
#define s5p_mfc_get_dec_used_flag()		MFC_READL(S5P_FIMV_D_USED_DPB_FLAG_LOWER)
#define s5p_mfc_get_enc_nal_done_info()		((MFC_READL(S5P_FIMV_E_NAL_DONE_INFO) & (0x3 << 4)) >> 4)
#define s5p_mfc_get_hevc_main_12()		(((MFC_READL(S5P_FIMV_D_HEVC_INFO)	\
						>> S5P_FIMV_D_HEVC_INFO_SHIFT)		\
						& S5P_FIMV_D_HEVC_INFO_MAIN_12_MASK)	\
						== S5P_FIMV_D_HEVC_INFO_MAIN_12_VAL)
#define s5p_mfc_get_hevc_422_10_intra()		(((MFC_READL(S5P_FIMV_D_HEVC_INFO)	\
						>> S5P_FIMV_D_HEVC_INFO_SHIFT)		\
						& S5P_FIMV_D_HEVC_INFO_MAIN_422_10_INTRA_MASK)	\
						== S5P_FIMV_D_HEVC_INFO_MAIN_422_10_INTRA_VAL)
#define s5p_mfc_get_lcu_size()			(MFC_READL(S5P_FIMV_D_HEVC_INFO)		\
						& S5P_FIMV_D_HEVC_INFO_LCU_SIZE_MASK)


/* nal queue information */
#define s5p_mfc_get_nal_q_input_count()		MFC_READL(S5P_FIMV_NAL_QUEUE_INPUT_COUNT)
#define s5p_mfc_get_nal_q_output_count()	MFC_READL(S5P_FIMV_NAL_QUEUE_OUTPUT_COUNT)
#define s5p_mfc_get_nal_q_input_exe_count()	MFC_READL(S5P_FIMV_NAL_QUEUE_INPUT_EXE_COUNT)
#define s5p_mfc_get_nal_q_info()		MFC_READL(S5P_FIMV_NAL_QUEUE_INFO)
#define s5p_mfc_get_nal_q_input_addr()		MFC_READL(S5P_FIMV_NAL_QUEUE_INPUT_ADDR)
#define s5p_mfc_get_nal_q_input_size()		MFC_READL(S5P_FIMV_NAL_QUEUE_INPUT_SIZE)
#define s5p_mfc_get_nal_q_output_addr()		MFC_READL(S5P_FIMV_NAL_QUEUE_OUTPUT_ADDR)
#define s5p_mfc_get_nal_q_output_ize()		MFC_READL(S5P_FIMV_NAL_QUEUE_OUTPUT_SIZE)

#define s5p_mfc_clear_int_sfr()				\
	do {							\
		MFC_WRITEL(0, S5P_FIMV_RISC2HOST_CMD);	\
		MFC_WRITEL(0, S5P_FIMV_RISC2HOST_INT);	\
	} while (0)

static inline void s5p_mfc_reset_nal_queue_registers(struct s5p_mfc_dev *dev)
{
	MFC_WRITEL(0x0, S5P_FIMV_NAL_QUEUE_INPUT_COUNT);
	MFC_WRITEL(0x0, S5P_FIMV_NAL_QUEUE_OUTPUT_COUNT);
	MFC_WRITEL(0x0, S5P_FIMV_NAL_QUEUE_INPUT_EXE_COUNT);
	MFC_WRITEL(0x0, S5P_FIMV_NAL_QUEUE_INFO);
}

static inline void s5p_mfc_update_nal_queue_input(struct s5p_mfc_dev *dev,
	dma_addr_t addr, unsigned int size)
{
	MFC_WRITEL(addr, S5P_FIMV_NAL_QUEUE_INPUT_ADDR);
	MFC_WRITEL(size, S5P_FIMV_NAL_QUEUE_INPUT_SIZE);
}

static inline void s5p_mfc_update_nal_queue_output(struct s5p_mfc_dev *dev,
	dma_addr_t addr, unsigned int size)
{
	MFC_WRITEL(addr, S5P_FIMV_NAL_QUEUE_OUTPUT_ADDR);
	MFC_WRITEL(size, S5P_FIMV_NAL_QUEUE_OUTPUT_SIZE);
}

static inline void s5p_mfc_update_nal_queue_input_count(struct s5p_mfc_dev *dev,
	unsigned int input_count)
{
	MFC_WRITEL(input_count, S5P_FIMV_NAL_QUEUE_INPUT_COUNT);
}

static inline void s5p_mfc_dec_store_crop_info(struct s5p_mfc_ctx *ctx)
{
	struct s5p_mfc_dev *dev = ctx->dev;
	struct s5p_mfc_dec *dec = ctx->dec_priv;
	u32 left, right, top, bottom;

	left = MFC_READL(S5P_FIMV_D_DISPLAY_CROP_INFO1);
	right = left >> S5P_FIMV_D_SHARED_CROP_RIGHT_SHIFT;
	left = left & S5P_FIMV_D_SHARED_CROP_LEFT_MASK;
	top = MFC_READL(S5P_FIMV_D_DISPLAY_CROP_INFO2);
	bottom = top >> S5P_FIMV_D_SHARED_CROP_BOTTOM_SHIFT;
	top = top & S5P_FIMV_D_SHARED_CROP_TOP_MASK;

	dec->cr_left = left;
	dec->cr_right = right;
	dec->cr_top = top;
	dec->cr_bot = bottom;
}

static inline int s5p_mfc_check_int_cmd(struct s5p_mfc_dev *dev)
{
	if (MFC_READL(S5P_FIMV_RISC2HOST_INT))
		return MFC_READL(S5P_FIMV_RISC2HOST_CMD);
	else
		return 0;
}

static inline int s5p_mfc_stop_bus(struct s5p_mfc_dev *dev)
{
	unsigned int status;
	unsigned long timeout;

	/* Reset */
	MFC_WRITEL(0x1, S5P_FIMV_MFC_BUS_RESET_CTRL);

	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	/* Check bus status */
	do {
		if (time_after(jiffies, timeout)) {
			mfc_err_dev("Timeout while resetting MFC.\n");
			return -EIO;
		}
		status = MFC_READL(S5P_FIMV_MFC_BUS_RESET_CTRL);
	} while ((status & 0x2) == 0);

	return 0;
}

static inline void s5p_mfc_start_bus(struct s5p_mfc_dev *dev)
{
	int val;

	val = MFC_READL(S5P_FIMV_MFC_BUS_RESET_CTRL);
	val &= ~(0x1);
	MFC_WRITEL(val, S5P_FIMV_MFC_BUS_RESET_CTRL);
}

static inline void s5p_mfc_risc_on(struct s5p_mfc_dev *dev)
{
	s5p_mfc_clean_dev_int_flags(dev);

	MFC_WRITEL(0x1, S5P_FIMV_RISC_ON);
	if (FW_HAS_HWACG(dev))
		MFC_WRITEL(0x0, S5P_FIMV_MFC_OFF);
	mfc_debug(1, "RISC ON\n");
	MFC_TRACE_DEV("RISC ON\n");
}

static inline void s5p_mfc_risc_off(struct s5p_mfc_dev *dev)
{
	unsigned int status;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	/* Check pending status */
	do {
		if (time_after(jiffies, timeout)) {
			mfc_err_dev("Timeout while pendng clear\n");
			mfc_err_dev("MFC access pending state: %#x\n", status);
			mfc_err_dev("MFC access pending R: %#x, W: %#x\n",
					MFC_READL(S5P_FIMV_MFC_RPEND),
					MFC_READL(S5P_FIMV_MFC_WPEND));
			break;
		}
		status = MFC_READL(S5P_FIMV_MFC_BUS_STATUS);
	} while (status != 0);

	MFC_WRITEL(0x0, S5P_FIMV_RISC_ON);
}

static inline void s5p_mfc_mfc_off(struct s5p_mfc_dev *dev)
{
	if (FW_HAS_HWACG(dev)) {
		mfc_info_dev("MFC h/w state: %d\n",
				MFC_READL(S5P_FIMV_MFC_STATE) & 0x7);
		MFC_WRITEL(0x1, S5P_FIMV_MFC_OFF);
	}
}

static inline void s5p_mfc_enable_all_clocks(struct s5p_mfc_dev *dev)
{
	/* Enable all FW clock gating */
	MFC_WRITEL(0xFFFFFFFF, S5P_FIMV_MFC_FW_CLOCK);
}

static inline void s5p_mfc_clear_enc_res_change(struct s5p_mfc_dev *dev)
{
	unsigned int reg = 0;

	reg = MFC_READL(S5P_FIMV_E_PARAM_CHANGE);
	reg &= ~(0x7 << 6);
	MFC_WRITEL(reg, S5P_FIMV_E_PARAM_CHANGE);
}

int s5p_mfc_reset_mfc(struct s5p_mfc_dev *dev);

void s5p_mfc_dbg_enable(struct s5p_mfc_dev *dev);
void s5p_mfc_dbg_disable(struct s5p_mfc_dev *dev);
void s5p_mfc_dbg_set_addr(struct s5p_mfc_dev *dev);

int s5p_mfc_set_dec_codec_buffers(struct s5p_mfc_ctx *ctx);
int s5p_mfc_set_enc_codec_buffers(struct s5p_mfc_ctx *mfc_ctx);

int s5p_mfc_set_dec_stream_buffer(struct s5p_mfc_ctx *ctx,
		struct s5p_mfc_buf *mfc_buf,
		unsigned int start_num_byte,
		unsigned int buf_size);

void s5p_mfc_set_enc_frame_buffer(struct s5p_mfc_ctx *ctx,
		dma_addr_t addr[], int num_planes);
int s5p_mfc_set_enc_stream_buffer(struct s5p_mfc_ctx *ctx,
		struct s5p_mfc_buf *mfc_buf);

void s5p_mfc_get_enc_frame_buffer(struct s5p_mfc_ctx *ctx,
		dma_addr_t addr[], int num_planes);
void s5p_mfc_set_enc_stride(struct s5p_mfc_ctx *ctx);

void s5p_mfc_set_risc_base_addr(struct s5p_mfc_dev *dev,
				enum mfc_buf_usage_type buf_type);

int s5p_mfc_set_dynamic_dpb(struct s5p_mfc_ctx *ctx, struct s5p_mfc_buf *dst_vb);

#endif /* __S5P_MFC_REG_H */
