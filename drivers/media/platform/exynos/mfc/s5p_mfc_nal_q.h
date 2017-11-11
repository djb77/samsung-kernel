/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_nal_q.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_NAL_Q_H
#define __S5P_MFC_NAL_Q_H __FILE__

#include "s5p_mfc_common.h"

int s5p_mfc_nal_q_check_enable(struct s5p_mfc_dev *dev);

nal_queue_handle *s5p_mfc_nal_q_create(struct s5p_mfc_dev *dev);
int s5p_mfc_nal_q_destroy(struct s5p_mfc_dev *dev, nal_queue_handle *nal_q_handle);

void s5p_mfc_nal_q_init(struct s5p_mfc_dev *dev, nal_queue_handle *nal_q_handle);
void s5p_mfc_nal_q_start(struct s5p_mfc_dev *dev, nal_queue_handle *nal_q_handle);
void s5p_mfc_nal_q_stop(struct s5p_mfc_dev *dev, nal_queue_handle *nal_q_handle);
void s5p_mfc_nal_q_stop_if_started(struct s5p_mfc_dev *dev);
void s5p_mfc_nal_q_cleanup_queue(struct s5p_mfc_dev *dev);

int s5p_mfc_nal_q_handle_out_buf(struct s5p_mfc_dev *dev, EncoderOutputStr *pOutStr);
int s5p_mfc_nal_q_enqueue_in_buf(struct s5p_mfc_dev *dev, struct s5p_mfc_ctx *ctx,
			nal_queue_in_handle *nal_q_in_handle);
EncoderOutputStr *s5p_mfc_nal_q_dequeue_out_buf(struct s5p_mfc_dev *dev,
			nal_queue_out_handle *nal_q_out_handle, unsigned int *reason);

#endif /* __S5P_MFC_NAL_Q_H  */
