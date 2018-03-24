/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_ctrl.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "s5p_mfc_ctrl.h"

#include "s5p_mfc_hwlock.h"
#include "s5p_mfc_nal_q.h"
#include "s5p_mfc_watchdog.h"
#include "s5p_mfc_sync.h"

#include "s5p_mfc_pm.h"
#include "s5p_mfc_cmd.h"
#include "s5p_mfc_reg.h"

#include "s5p_mfc_utils.h"

/* Initialize hardware */
static int mfc_init_hw(struct s5p_mfc_dev *dev, enum mfc_buf_usage_type buf_type)
{
	char fimv_info;
	int fw_ver;
	int ret = 0;
	int curr_ctx_is_drm_backup;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	curr_ctx_is_drm_backup = dev->curr_ctx_is_drm;

	if (!dev->fw_buf.alloc)
		return -EINVAL;

	/* 0. MFC reset */
	mfc_debug(2, "MFC reset...\n");

	/* At init time, do not call secure API */
	if (buf_type == MFCBUF_NORMAL)
		dev->curr_ctx_is_drm = 0;
	else if (buf_type == MFCBUF_DRM)
		dev->curr_ctx_is_drm = 1;

	ret = s5p_mfc_pm_clock_on(dev);
	if (ret) {
		mfc_err_dev("Failed to enable clock before reset(%d)\n", ret);
		dev->curr_ctx_is_drm = curr_ctx_is_drm_backup;
		return ret;
	}

	ret = s5p_mfc_reset_mfc(dev);
	if (ret) {
		mfc_err_dev("Failed to reset MFC - timeout.\n");
		goto err_init_hw;
	}
	mfc_debug(2, "Done MFC reset...\n");

	/* 1. Set DRAM base Addr */
	s5p_mfc_set_risc_base_addr(dev, buf_type);

	/* 2. Release reset signal to the RISC */
	s5p_mfc_risc_on(dev);

	mfc_debug(2, "Will now wait for completion of firmware transfer.\n");
	if (s5p_mfc_wait_for_done_dev(dev, S5P_FIMV_R2H_CMD_FW_STATUS_RET)) {
		mfc_err_dev("Failed to RISC_ON\n");
		s5p_mfc_clean_dev_int_flags(dev);
		ret = -EIO;
		goto err_init_hw;
	}

	/* 3. Initialize firmware */
	ret = s5p_mfc_cmd_sys_init(dev, buf_type);
	if (ret) {
		mfc_err_dev("Failed to send command to MFC - timeout.\n");
		goto err_init_hw;
	}

	mfc_debug(2, "Ok, now will write a command to init the system\n");
	if (s5p_mfc_wait_for_done_dev(dev, S5P_FIMV_R2H_CMD_SYS_INIT_RET)) {
		mfc_err_dev("Failed to SYS_INIT\n");
		s5p_mfc_clean_dev_int_flags(dev);
		ret = -EIO;
		goto err_init_hw;
	}

	dev->int_condition = 0;
	if (dev->int_err != 0 || dev->int_reason !=
						S5P_FIMV_R2H_CMD_SYS_INIT_RET) {
		/* Failure. */
		mfc_err_dev("Failed to init firmware - error: %d"
				" int: %d.\n", dev->int_err, dev->int_reason);
		ret = -EIO;
		goto err_init_hw;
	}

	fimv_info = s5p_mfc_get_fimv_info();
	if (fimv_info != 'D' && fimv_info != 'E')
		fimv_info = 'N';

	mfc_info_dev("MFC v%x.%x, F/W: %02xyy, %02xmm, %02xdd (%c)\n",
		 MFC_VER_MAJOR(dev),
		 MFC_VER_MINOR(dev),
		 s5p_mfc_get_fw_ver_year(),
		 s5p_mfc_get_fw_ver_month(),
		 s5p_mfc_get_fw_ver_date(),
		 fimv_info);

	dev->fw.date = s5p_mfc_get_fw_ver_all();
	/* Check MFC version and F/W version */
	fw_ver = s5p_mfc_get_mfc_version();
	if (fw_ver != s5p_mfc_version(dev)) {
		mfc_err_dev("Invalid F/W version(0x%x) for MFC H/W(0x%x)\n",
				fw_ver, s5p_mfc_version(dev));
		ret = -EIO;
		goto err_init_hw;
	}

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	/* Cache flush for base address change */
	s5p_mfc_cmd_cache_flush(dev);
	if (s5p_mfc_wait_for_done_dev(dev, S5P_FIMV_R2H_CMD_CACHE_FLUSH_RET)) {
		mfc_err_dev("Failed to CACHE_FLUSH\n");
		s5p_mfc_clean_dev_int_flags(dev);
		ret = -EIO;
		goto err_init_hw;
	}

	if (buf_type == MFCBUF_DRM && !curr_ctx_is_drm_backup) {
		s5p_mfc_pm_clock_off(dev);
		dev->curr_ctx_is_drm = curr_ctx_is_drm_backup;
		s5p_mfc_pm_clock_on_with_base(dev, MFCBUF_NORMAL);
	} else if (buf_type == MFCBUF_NORMAL && curr_ctx_is_drm_backup) {
		s5p_mfc_set_risc_base_addr(dev, MFCBUF_DRM);
	}
#endif

err_init_hw:
	s5p_mfc_pm_clock_off(dev);
	dev->curr_ctx_is_drm = curr_ctx_is_drm_backup;
	mfc_debug_leave();

	return ret;
}

/* Wrapper : Initialize hardware */
int s5p_mfc_init_hw(struct s5p_mfc_dev *dev)
{
	int ret;

	ret = mfc_init_hw(dev, MFCBUF_NORMAL);
	if (ret)
		return ret;

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (dev->fw.drm_status) {
		ret = mfc_init_hw(dev, MFCBUF_DRM);
		if (ret)
			return ret;
	}
#endif

	return ret;
}

/* Deinitialize hardware */
void s5p_mfc_deinit_hw(struct s5p_mfc_dev *dev)
{
	mfc_debug(2, "mfc deinit start\n");

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return;
	}

	s5p_mfc_mfc_off(dev);

	mfc_debug(2, "mfc deinit completed\n");
}

int s5p_mfc_sleep(struct s5p_mfc_dev *dev)
{
	struct s5p_mfc_ctx *ctx;
	int ret;
	int old_state, i;
	int need_cache_flush = 0;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}

	ctx = dev->ctx[dev->curr_ctx];
	if (!ctx) {
		for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
			if (dev->ctx[i]) {
				ctx = dev->ctx[i];
				break;
			}
		}
		if (!ctx) {
			mfc_err_dev("no mfc context to run\n");
			return -EINVAL;
		} else {
			mfc_info_dev("ctx is changed %d -> %d\n",
					dev->curr_ctx, ctx->num);
			dev->curr_ctx = ctx->num;
			if (dev->curr_ctx_is_drm != ctx->is_drm) {
				need_cache_flush = 1;
				mfc_info_dev("DRM attribute is changed %d->%d\n",
						dev->curr_ctx_is_drm, ctx->is_drm);
			}
		}
	}
	old_state = ctx->state;
	s5p_mfc_change_state(ctx, MFCINST_ABORT);
	MFC_TRACE_DEV_HWLOCK("**sleep (ctx:%d)\n", ctx->num);
	ret = s5p_mfc_get_hwlock_dev(dev);
	if (ret < 0) {
		mfc_err_dev("Failed to get hwlock.\n");
		mfc_err_dev("dev.hwlock.dev = 0x%lx, bits = 0x%lx, owned_by_irq = %d, wl_count = %d, transfer_owner = %d\n",
				dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
				dev->hwlock.wl_count, dev->hwlock.transfer_owner);
		return -EBUSY;
	}

	mfc_info_dev("curr_ctx_is_drm:%d, hwlock.bits:%lu, hwlock.dev:%lu\n",
			dev->curr_ctx_is_drm, dev->hwlock.bits, dev->hwlock.dev);

	s5p_mfc_change_state(ctx, old_state);
	s5p_mfc_pm_clock_on(dev);

	if (need_cache_flush)
		s5p_mfc_cache_flush(dev, ctx->is_drm);

	s5p_mfc_cmd_sleep(dev);

	if (s5p_mfc_wait_for_done_dev(dev, S5P_FIMV_R2H_CMD_SLEEP_RET)) {
		mfc_err_dev("Failed to SLEEP\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_SLEEP);
		s5p_mfc_dump_info_and_stop_hw(dev);
		return -EIO;
	}

	dev->int_condition = 0;
	if (dev->int_err != 0 || dev->int_reason !=
						S5P_FIMV_R2H_CMD_SLEEP_RET) {
		/* Failure. */
		mfc_err_dev("Failed to sleep - error: %d"
				" int: %d.\n", dev->int_err, dev->int_reason);
		ret = -EIO;
		goto err_mfc_sleep;
	}

	dev->sleep = 1;

err_mfc_sleep:
	s5p_mfc_mfc_off(dev);
	s5p_mfc_pm_clock_off(dev);
	s5p_mfc_release_hwlock_dev(dev);
	mfc_debug_leave();

	return ret;
}

int s5p_mfc_wakeup(struct s5p_mfc_dev *dev)
{
	enum mfc_buf_usage_type buf_type;
	int ret = 0;

	mfc_debug_enter();

	if (!dev) {
		mfc_err_dev("no mfc device to run\n");
		return -EINVAL;
	}
	mfc_info_dev("curr_ctx_is_drm:%d\n", dev->curr_ctx_is_drm);

	MFC_TRACE_DEV_HWLOCK("**wakeup\n");
	ret = s5p_mfc_get_hwlock_dev(dev);
	if (ret < 0) {
		mfc_err_dev("Failed to get hwlock.\n");
		mfc_err_dev("dev.hwlock.dev = 0x%lx, bits = 0x%lx, owned_by_irq = %d, wl_count = %d, transfer_owner = %d\n",
				dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
				dev->hwlock.wl_count, dev->hwlock.transfer_owner);
		return -EBUSY;
	}

	dev->sleep = 0;

	/* 0. MFC reset */
	mfc_debug(2, "MFC reset...\n");

	s5p_mfc_pm_clock_on(dev);

	ret = s5p_mfc_reset_mfc(dev);
	if (ret) {
		mfc_err_dev("Failed to reset MFC - timeout.\n");
		goto err_mfc_wakeup;
	}
	mfc_debug(2, "Done MFC reset...\n");
	if (dev->curr_ctx_is_drm)
		buf_type = MFCBUF_DRM;
	else
		buf_type = MFCBUF_NORMAL;

	/* 1. Set DRAM base Addr */
	s5p_mfc_set_risc_base_addr(dev, buf_type);

	/* 2. Release reset signal to the RISC */
	s5p_mfc_risc_on(dev);

	mfc_debug(2, "Will now wait for completion of firmware transfer.\n");
	if (s5p_mfc_wait_for_done_dev(dev, S5P_FIMV_R2H_CMD_FW_STATUS_RET)) {
		mfc_err_dev("Failed to RISC_ON\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_RISC_ON);
		s5p_mfc_dump_info_and_stop_hw(dev);
		return -EIO;
	}

	mfc_debug(2, "Ok, now will write a command to wakeup the system\n");
	s5p_mfc_cmd_wakeup(dev);

	mfc_debug(2, "Will now wait for completion of firmware wake up.\n");
	if (s5p_mfc_wait_for_done_dev(dev, S5P_FIMV_R2H_CMD_WAKEUP_RET)) {
		mfc_err_dev("Failed to WAKEUP\n");
		dev->logging_data->cause |= (1 << MFC_CAUSE_FAIL_WAKEUP);
		s5p_mfc_dump_info_and_stop_hw(dev);
		return -EIO;
	}

	dev->int_condition = 0;
	if (dev->int_err != 0 || dev->int_reason !=
						S5P_FIMV_R2H_CMD_WAKEUP_RET) {
		/* Failure. */
		mfc_err_dev("Failed to wakeup - error: %d"
				" int: %d.\n", dev->int_err, dev->int_reason);
		ret = -EIO;
		goto err_mfc_wakeup;
	}

err_mfc_wakeup:
	s5p_mfc_pm_clock_off(dev);

	s5p_mfc_release_hwlock_dev(dev);

	mfc_debug_leave();

	return ret;
}
