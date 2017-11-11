/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_macros.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_MACROS_H
#define __S5P_MFC_MACROS_H __FILE__

#define WIDTH_MB(x_size)	((x_size + 15) / 16)
#define HEIGHT_MB(y_size)	((y_size + 15) / 16)

/*
   Note that lcu_width and lcu_height are defined as follows :
   lcu_width = (frame_width + lcu_size - 1)/lcu_size
   lcu_height = (frame_height + lcu_size - 1)/lcu_size.
   (lcu_size is 32(encoder) or 64(decoder))
*/
#define DEC_LCU_WIDTH(x_size)	((x_size + 63) / 64)
#define ENC_LCU_WIDTH(x_size)	((x_size + 31) / 32)
#define DEC_LCU_HEIGHT(y_size)	((y_size + 63) / 64)
#define ENC_LCU_HEIGHT(y_size)	((y_size + 31) / 32)

#define DEC_MV_SIZE_MB(x, y)	(WIDTH_MB(x) * (((HEIGHT_MB(y)+1)/2)*2) * 64 + 1024)
#define DEC_HEVC_MV_SIZE(x, y)	(DEC_LCU_WIDTH(x) * DEC_LCU_HEIGHT(y) * 256 + 512)

/* Encoder buffer size for MFC v10.0 */
#define ENC_V100_H264_ME_SIZE(x, y)				\
	(((x + 3) * (y + 3) * 8) + ((((x * y) + 63) / 64) * 32) + (((y * 64) + 1280) * (x + 7) / 8))
#define ENC_V100_MPEG4_ME_SIZE(x, y)				\
	(((x + 3) * (y + 3) * 8) + ((((x * y) + 127) / 128) * 16) + (((y * 64) + 1280) * (x + 7) / 8))
#define ENC_V100_VP8_ME_SIZE(x, y)				\
	(((x + 3) * (y + 3) * 8) + (((y * 64) + 1280) * (x + 7) / 8))
#define ENC_V100_VP9_ME_SIZE(x, y)				\
	((((x * 2) + 3) * ((y * 2) + 3) * 128) + (((y * 256) + 1280) * (x+1) / 2))
#define ENC_V100_HEVC_ME_SIZE(x, y)				\
	(((x + 3) * (y + 3) * 32) + (((y * 128) + 1280) * (x + 3) / 4))

#endif /* __S5P_MFC_MACROS_H */
