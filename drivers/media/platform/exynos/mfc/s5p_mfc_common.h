/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_common.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_COMMON_H
#define __S5P_MFC_COMMON_H __FILE__

#include <linux/exynos_iovmm.h>

#include "s5p_mfc_regs_v10.h"
#include "s5p_mfc_macros.h"
#include "s5p_mfc_debug.h"
#include "exynos_mfc_media.h"
#include "s5p_mfc_data_struct.h"

#define MFC_DRIVER_INFO		180315

#define MFC_MAX_REF_BUFS	2
#define MFC_FRAME_PLANES	2
#define MFC_INFO_INIT_FD	-1

#define MFC_MAX_DRM_CTX		2

/* Interrupt timeout */
#define MFC_INT_TIMEOUT		5000
/* Interrupt short timeout */
#define MFC_INT_SHORT_TIMEOUT	800
/* Busy wait timeout */
#define MFC_BW_TIMEOUT		500

/* This value guarantees 299.4msec ~ 2.25sec according to MFC clock (668MHz ~ 89MHz)
 * releated with S5P_FIMV_DEC_TIMEOUT_VALUE */
#define MFC_TIMEOUT_VALUE	200000000

#define NUM_MPEG4_LF_BUF	2

#define DEFAULT_TAG		(0xE05)

#define MFC_NO_INSTANCE_SET	-1

#define MFC_ENC_CAP_PLANE_COUNT	1
#define MFC_ENC_OUT_PLANE_COUNT	2

#define MFC_NAME_LEN		16
#define MFC_FW_NAME		"mfc_fw.bin"

#define STUFF_BYTE		4

#define MFC_BASE_MASK		((1 << 17) - 1)

#define FLAG_LAST_FRAME		0x80000000
#define FLAG_EMPTY_DATA		0x40000000
#define FLAG_CSD		0x20000000

/* MFC conceal color is black */
#define MFC_CONCEAL_COLOR	0x8020000

#define vb_to_mfc_buf(x)		\
	container_of(x, struct s5p_mfc_buf, vb.vb2_buf)

#define fh_to_mfc_ctx(x)		\
	container_of(x, struct s5p_mfc_ctx, fh)

#define call_bop(b, op, args...)	\
	(b->op ? b->op(args) : 0)

#define call_cop(c, op, args...)	\
	(((c)->c_ops->op) ?		\
		((c)->c_ops->op(args)) : 0)

#define	MFC_CTRL_TYPE_GET	(MFC_CTRL_TYPE_GET_SRC | MFC_CTRL_TYPE_GET_DST)
#define	MFC_CTRL_TYPE_SRC	(MFC_CTRL_TYPE_SET | MFC_CTRL_TYPE_GET_SRC)
#define	MFC_CTRL_TYPE_DST	(MFC_CTRL_TYPE_GET_DST)

#define MFC_FMT_DEC	0
#define MFC_FMT_ENC	1
#define MFC_FMT_RAW	2

/* node check */
#define IS_DEC_NODE(n)		((n == EXYNOS_VIDEONODE_MFC_DEC) ||	\
				(n == EXYNOS_VIDEONODE_MFC_DEC_DRM))
#define IS_ENC_NODE(n)		((n == EXYNOS_VIDEONODE_MFC_ENC) ||	\
				(n == EXYNOS_VIDEONODE_MFC_ENC_DRM) ||	\
				(n == EXYNOS_VIDEONODE_MFC_ENC_OTF) ||	\
				(n == EXYNOS_VIDEONODE_MFC_ENC_OTF_DRM))

/* Decoder codec mode check */
#define IS_H264_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_H264_DEC)
#define IS_H264_MVC_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_H264_MVC_DEC)
#define IS_MPEG4_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_MPEG4_DEC)
#define IS_FIMV1_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_FIMV1_DEC)
#define IS_FIMV2_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_FIMV2_DEC)
#define IS_FIMV3_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_FIMV3_DEC)
#define IS_FIMV4_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_FIMV4_DEC)
#define IS_VC1_DEC(ctx)		((ctx)->codec_mode == S5P_FIMV_CODEC_VC1_DEC)
#define IS_VC1_RCV_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_VC1_RCV_DEC)
#define IS_MPEG2_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_MPEG2_DEC)
#define IS_HEVC_DEC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_HEVC_DEC)
#define IS_VP9_DEC(ctx)		((ctx)->codec_mode == S5P_FIMV_CODEC_VP9_DEC)
#define IS_BPG_DEC(ctx)		((ctx)->codec_mode == S5P_FIMV_CODEC_BPG_DEC)

/* Encoder codec mode check */
#define IS_H264_ENC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_H264_ENC)
#define IS_MPEG4_ENC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_MPEG4_ENC)
#define IS_H263_ENC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_H263_ENC)
#define IS_VP8_ENC(ctx)		((ctx)->codec_mode == S5P_FIMV_CODEC_VP8_ENC)
#define IS_HEVC_ENC(ctx)	((ctx)->codec_mode == S5P_FIMV_CODEC_HEVC_ENC)
#define IS_VP9_ENC(ctx)		((ctx)->codec_mode == S5P_FIMV_CODEC_VP9_ENC)
#define IS_BPG_ENC(ctx)		((ctx)->codec_mode == S5P_FIMV_CODEC_BPG_ENC)

#define CODEC_NOT_CODED(ctx)	(IS_MPEG4_DEC(ctx) || IS_VC1_DEC(ctx) || IS_VC1_RCV_DEC(ctx))
#define CODEC_INTERLACED(ctx)	(IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) ||	\
				IS_MPEG2_DEC(ctx) || IS_MPEG4_DEC(ctx) ||	\
				IS_VC1_DEC(ctx) || IS_VC1_RCV_DEC(ctx))
#define CODEC_MBAFF(ctx)	(IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx))
#define CODEC_MULTIFRAME(ctx)	(IS_MPEG4_DEC(ctx) || IS_VP9_DEC(ctx) ||	\
				IS_FIMV2_DEC(ctx) || IS_FIMV3_DEC(ctx) || IS_FIMV4_DEC(ctx))
#define CODEC_10BIT(ctx)	(IS_HEVC_DEC(ctx) || IS_HEVC_ENC(ctx) ||	\
				IS_VP9_DEC(ctx) || IS_VP9_ENC(ctx) ||		\
				IS_BPG_DEC(ctx) || IS_BPG_ENC(ctx))
#define CODEC_422FORMAT(ctx)	(IS_HEVC_DEC(ctx) || IS_HEVC_ENC(ctx) ||	\
				IS_BPG_DEC(ctx) || IS_BPG_ENC(ctx))
#define ON_RES_CHANGE(ctx)	(((ctx)->state >= MFCINST_RES_CHANGE_INIT) &&	\
				 ((ctx)->state <= MFCINST_RES_CHANGE_END))

/* UHD resoluition */
#define MFC_UHD_RES		(3840 * 2160)
#define IS_UHD_RES(ctx)		(((ctx)->img_width * (ctx)->img_height) == MFC_UHD_RES)
#define OVER_UHD_ENC60(ctx)	((((ctx)->img_width * (ctx)->img_height) == MFC_UHD_RES) && \
				((ctx)->type == MFCINST_ENCODER) &&	\
				((ctx)->framerate / 1000) >= 60)

/* Extra information for Decoder */
#define	DEC_SET_DUAL_DPB		(1 << 0)
#define	DEC_SET_DYNAMIC_DPB		(1 << 1)
#define	DEC_SET_LAST_FRAME_INFO		(1 << 2)
#define	DEC_SET_SKYPE_FLAG		(1 << 3)

/* Extra information for Encoder */
#define	ENC_SET_RGB_INPUT		(1 << 0)
#define	ENC_SET_SPARE_SIZE		(1 << 1)
#define	ENC_SET_TEMP_SVC_CH		(1 << 2)
#define	ENC_SET_SKYPE_FLAG		(1 << 3)
#define	ENC_SET_ROI_CONTROL		(1 << 4)
#define	ENC_SET_QP_BOUND_PB		(1 << 5)
#define	ENC_SET_FIXED_SLICE		(1 << 6)
#define	ENC_SET_PVC_MODE		(1 << 7)
#define	ENC_SET_RATIO_OF_INTRA		(1 << 8)
#define	ENC_SET_COLOR_ASPECT		(1 << 9)
#define	ENC_SET_HP_BITRATE_CONTROL	(1 << 10)
#define	ENC_SET_STATIC_INFO		(1 << 11)

#define MFC_VER_MAJOR(dev)	((s5p_mfc_version(dev) >> 8) & 0xFF)
#define MFC_VER_MINOR(dev)	(s5p_mfc_version(dev) & 0xFF)

/*
 * Version Description
 */
#define IS_MFCV10X(dev)		((s5p_mfc_version(dev) == 0xA0) || \
				(s5p_mfc_version(dev) == 0xA01))
#define IS_MFCV11X(dev)		(s5p_mfc_version(dev) == 0x1100)
#define IS_MFCV12X(dev)		(s5p_mfc_version(dev) == 0x1200)
#define FROM_MFCV11X(dev)	(IS_MFCV11X(dev) || IS_MFCV12X(dev))
#define FROM_MFCV10X(dev)	(IS_MFCV10X(dev) || IS_MFCV11X(dev) || \
				IS_MFCV12X(dev))

/* supported feature macros by F/W version */
#define FW_HAS_CONCEAL_CONTROL(dev)	(FROM_MFCV10X(dev))
#define FW_HAS_ROI_CONTROL(dev)		(FROM_MFCV10X(dev))
#define FW_HAS_HWACG(dev)		(FROM_MFCV10X(dev))
#define FW_HAS_SPECIAL_PARSING(dev)	(FROM_MFCV10X(dev))
#define FW_SUPPORT_SKYPE(dev)		(FROM_MFCV10X(dev) &&		\
					(dev->fw.date >= 0x150901))
#define FW_HAS_VIDEO_SIGNAL_TYPE(dev)	(FROM_MFCV10X(dev) &&		\
					(dev->fw.date >= 0x151223))
#define FW_HAS_SEI_INFO_FOR_HDR(dev)	(FROM_MFCV10X(dev) &&		\
					(dev->fw.date >= 0x160415))
#define FW_HAS_BLACK_BAR_DETECT(dev)	(FROM_MFCV11X(dev) &&		\
					(dev->fw.date >= 0x161017))
#define FW_HAS_VP9_HDR(dev)		(IS_MFCV12X(dev) &&		\
					(dev->fw.date >= 0x171023))
#define FW_HAS_RATIO_INTRA_CTRL(dev)	(FROM_MFCV11X(dev) &&		\
					(dev->fw.date >= 0x171113))
#define FW_HAS_ENC_COLOR_ASPECT(dev)	(FROM_MFCV11X(dev) &&		\
					(dev->fw.date >= 0x171023))
#define FW_HAS_ENC_STATIC_INFO(dev)	(IS_MFCV12X(dev) &&		\
					(dev->fw.date >= 0x180314))

static inline unsigned int s5p_mfc_version(struct s5p_mfc_dev *dev)
{
	unsigned int version = 0;

	switch (dev->pdata->ip_ver) {
	case IP_VER_MFC_4P_0:
	case IP_VER_MFC_4P_1:
	case IP_VER_MFC_4P_2:
		version = 0x51;
		break;
	case IP_VER_MFC_5G_0:
		version = 0x61;
		break;
	case IP_VER_MFC_5G_1:
	case IP_VER_MFC_5A_0:
	case IP_VER_MFC_5A_1:
		version = 0x65;
		break;
	case IP_VER_MFC_6A_0:
	case IP_VER_MFC_6A_1:
		version = 0x72;
		break;
	case IP_VER_MFC_6A_2:
		version = 0x723;
		break;
	case IP_VER_MFC_7A_0:
		version = 0x80;
		break;
	case IP_VER_MFC_8I_0:
		version = 0x90;
		break;
	case IP_VER_MFC_6I_0:
		version = 0x78;
		break;
	case IP_VER_MFC_8J_0:
		version = 0xA0;
		break;
	case IP_VER_MFC_8J_1:
		version = 0xA01;
		break;
	case IP_VER_MFC_8K_0:
		version = 0x1100;
		break;
	case IP_VER_MFC_7K_0:
		version = 0x1120;
		break;
	case IP_VER_MFC_9L_0:
		version = 0x1200;
		break;
	}

	return version;
}

#endif /* __S5P_MFC_COMMON_H */
