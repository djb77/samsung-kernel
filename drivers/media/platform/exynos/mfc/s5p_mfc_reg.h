/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_reg.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
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

#include "s5p_mfc_data_struct.h"

#if defined(CONFIG_EXYNOS_MFC_V10)
#include "regs-mfc-v10.h"
#endif

#define MFC_READL(offset)		readl(dev->regs_base + (offset))
#define MFC_WRITEL(data, offset)	writel((data), dev->regs_base + (offset))


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
#define s5p_mfc_get_dspl_status()	MFC_READL(S5P_FIMV_D_DISPLAY_STATUS)
#define s5p_mfc_get_dec_status()	(MFC_READL(S5P_FIMV_D_DECODED_STATUS)		\
						& S5P_FIMV_DEC_STATUS_DECODING_STATUS_MASK)
#define s5p_mfc_get_disp_frame_type()	(MFC_READL(S5P_FIMV_D_DISPLAY_FRAME_TYPE)	\
						& S5P_FIMV_DISPLAY_FRAME_MASK)
#define s5p_mfc_get_dec_frame_type()	(MFC_READL(S5P_FIMV_D_DECODED_FRAME_TYPE)	\
						& S5P_FIMV_DECODED_FRAME_MASK)
#define s5p_mfc_get_interlace_type()	((MFC_READL(S5P_FIMV_D_DISPLAY_FRAME_TYPE)	\
						>> S5P_FIMV_DISPLAY_TEMP_INFO_SHIFT)	\
						& S5P_FIMV_DISPLAY_TEMP_INFO_MASK)
#define s5p_mfc_is_interlace_picture()	((MFC_READL(S5P_FIMV_D_DISPLAY_STATUS)		\
						& S5P_FIMV_DEC_STATUS_INTERLACE_MASK))	\
						>> S5P_FIMV_DEC_STATUS_INTERLACE_SHIFT
#define mfc_get_last_disp_info()	((MFC_READL(S5P_FIMV_D_DISPLAY_STATUS)		\
						>> S5P_FIMV_DISPLAY_LAST_INFO_SHIFT)	\
						& S5P_FIMV_DISPLAY_LAST_INFO_MASK)
#define s5p_mfc_get_img_width()		MFC_READL(S5P_FIMV_D_DISPLAY_FRAME_WIDTH)
#define s5p_mfc_get_img_height()	MFC_READL(S5P_FIMV_D_DISPLAY_FRAME_HEIGHT)
#define mfc_get_disp_first_addr()	-1
#define mfc_get_dec_first_addr()	-1
#define s5p_mfc_get_disp_y_addr()	MFC_READL(S5P_FIMV_D_DISPLAY_LUMA_ADDR)
#define s5p_mfc_get_dec_y_addr()	MFC_READL(S5P_FIMV_D_DECODED_LUMA_ADDR)


/* kind of interrupt */
#define s5p_mfc_get_int_reason()	(MFC_READL(S5P_FIMV_RISC2HOST_CMD)		\
						& S5P_FIMV_RISC2HOST_CMD_MASK)
#define s5p_mfc_get_int_err()		MFC_READL(S5P_FIMV_ERROR_CODE)
#define s5p_mfc_err_dec(x)		(((x) & S5P_FIMV_ERR_DEC_MASK)			\
						>> S5P_FIMV_ERR_DEC_SHIFT)
#define s5p_mfc_err_dspl(x)		(((x) & S5P_FIMV_ERR_DSPL_MASK)			\
						>> S5P_FIMV_ERR_DSPL_SHIFT)


/* additional information */
#define s5p_mfc_get_consumed_stream()	MFC_READL(S5P_FIMV_D_DECODED_NAL_SIZE)
#define s5p_mfc_get_dpb_count()		MFC_READL(S5P_FIMV_D_MIN_NUM_DPB)
#define s5p_mfc_get_scratch_size()	MFC_READL(S5P_FIMV_D_MIN_SCRATCH_BUFFER_SIZE)
#define s5p_mfc_get_dis_count()		0
#define s5p_mfc_get_mv_count()		MFC_READL(S5P_FIMV_D_MIN_NUM_MV)
#define s5p_mfc_get_inst_no()		MFC_READL(S5P_FIMV_RET_INSTANCE_ID)
#define s5p_mfc_get_enc_dpb_count()	MFC_READL(S5P_FIMV_E_NUM_DPB)
#define s5p_mfc_get_enc_scratch_size()	MFC_READL(S5P_FIMV_E_MIN_SCRATCH_BUFFER_SIZE)
#define s5p_mfc_get_enc_strm_size()	MFC_READL(S5P_FIMV_E_STREAM_SIZE)
#define s5p_mfc_get_enc_slice_type()	MFC_READL(S5P_FIMV_E_SLICE_TYPE)
#define s5p_mfc_get_enc_pic_count()	MFC_READL(S5P_FIMV_E_PICTURE_COUNT)
#define s5p_mfc_get_sei_avail_status()	MFC_READL(S5P_FIMV_D_FRAME_PACK_SEI_AVAIL)
#define s5p_mfc_get_mvc_num_views()	MFC_READL(S5P_FIMV_D_MVC_NUM_VIEWS)
#define s5p_mfc_get_mvc_disp_view_id()	(MFC_READL(S5P_FIMV_D_MVC_VIEW_ID)		\
					& S5P_FIMV_D_MVC_VIEW_ID_DISP_MASK)
#define s5p_mfc_get_profile()		(MFC_READL(S5P_FIMV_D_DECODED_PICTURE_PROFILE)	\
						& S5P_FIMV_DECODED_PIC_PROFILE_MASK)
#define mfc_get_dec_used_flag()		MFC_READL(S5P_FIMV_D_USED_DPB_FLAG_LOWER)


#endif /* __S5P_MFC_REG_H */
