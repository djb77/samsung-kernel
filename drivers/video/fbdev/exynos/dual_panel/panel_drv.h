/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Samsung's Panel Driver
 * Author: Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_DRV_H__
#define __PANEL_DRV_H__

#include <linux/device.h>
#include <linux/fb.h>
#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <media/v4l2-subdev.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>

#include "../dual_dpu/decon_lcd.h"
#include "panel.h"
#include "mdnie.h"
#include "copr.h"

extern int panel_log_level;

#define panel_err(fmt, ...)							\
	do {									\
		if (panel_log_level >= 3) {					\
			pr_err(pr_fmt(fmt), ##__VA_ARGS__);			\
		}								\
	} while (0)

#define panel_warn(fmt, ...)							\
	do {									\
		if (panel_log_level >= 4) {					\
			pr_warn(pr_fmt(fmt), ##__VA_ARGS__);			\
		}								\
	} while (0)
#define panel_info(fmt, ...)							\
	do {									\
		if (panel_log_level >= 6)					\
			pr_info(pr_fmt(fmt), ##__VA_ARGS__);			\
	} while (0)
#define panel_dbg(fmt, ...)							\
	do {									\
		if (panel_log_level >= 6)					\
			pr_info(pr_fmt(fmt), ##__VA_ARGS__);			\
	} while (0)


enum {
	REGULATOR_3p0V = 0,
	REGULATOR_1p8V,
	REGULATOR_1p6V,
	REGULATOR_MAX
};

enum panel_gpio_lists {
	PANEL_GPIO_RESET = 0,
	PANEL_GPIO_DISP_DET,
	PANEL_GPIO_PCD,
	PANEL_GPIO_ERR_FG,
	PANEL_GPIO_MAX,
};

#define GPIO_NAME_RESET 	"gpio,lcd-reset"
#define GPIO_NAME_DISP_DET 	"gpio,disp-det"
#define GPIO_NAME_PCD		"gpio,pcd"
#define GPIO_NAME_ERR_FG	"gpio,err_fg"

#define REGULATOR_3p0_NAME "regulator,3p0"
#define REGULATOR_1p8_NAME "regulator,1p8"
#define REGULATOR_1p6_NAME "regulator,1p6"

struct panel_pad {
	int gpio_reset;
	int gpio_disp_det;
	int gpio_pcd;
	int gpio_err_fg;

	int irq_disp_det;
	int irq_pcd;
	int irq_err_fg;

	struct regulator *regulator[REGULATOR_MAX];

	void __iomem *pend_reg_disp_det;
	int pend_bit_disp_det;
};

struct mipi_drv_ops {
	int (*read)(u32 id, u8 addr, u8 *buf, int size);
	int (*write)(u32 id, u8 cmd_id, const u8 *cmd, int size);
	enum dsim_state(*get_state)(u32 id);
};

#define PANEL_INIT_KERNEL 		0
#define PANEL_INIT_BOOT 		1

#define PANEL_DISCONNECT		0
#define PANEL_CONNECT			1

enum panel_active_state {
	PANEL_STATE_OFF = 0,
	PANEL_STATE_ON,
	PANEL_STATE_NORMAL,
	PANEL_STATE_ALPM,
};

enum panel_power {
	PANEL_POWER_OFF = 0,
	PANEL_POWER_ON
};

enum {
	PANEL_DISPLAY_OFF = 0,
	PANEL_DISPLAY_ON,
};

enum {
	PANEL_HMD_OFF = 0,
	PANEL_HMD_ON,
};

struct panel_state {
	int init_at;
	int connect_panel;
	int cur_state;
	int power;
	int disp_on;
#ifdef CONFIG_SUPPORT_HMD
	int hmd_on;
#endif
};

#ifdef CONFIG_ACTIVE_CLOCK
enum {
	IMG_UPDATE_NEED = 0,
	IMG_UPDATE_DONE,
};

struct act_clk_info {
	u32 en;
	u32 interval;
	u32 time_hr;
	u32 time_min;
	u32 time_sec;
	u32 time_ms;
	u32 pos_x;
	u32 pos_y;
/* flag to check need to update side ram img */
	u32 update_img;
	char *img_buf;
	u32 img_buf_size;
	u32 update_time;
};

struct act_blink_info {
	u32 en;
	u32 interval;
	u32 radius;
	u32 color;
	u32 line_width;
	u32 pos1_x;
	u32 pos1_y;
	u32 pos2_x;
	u32 pos2_y;
};

struct act_drawer_info {
	u32 sd_line_color;
	u32 sd2_line_color;
	u32 sd_radius;
};

struct act_clock_dev {
	struct miscdevice dev;
	struct act_clk_info act_info;
	struct act_blink_info blink_info;
	struct act_drawer_info draw_info;
	u32 opened;
};
#endif


struct copr_spi_gpios {
	int gpio_sck;
	int gpio_miso;
	int gpio_mosi;
	int gpio_cs;
};
struct host_cb {
	int (*cb)(void *data);
	void *data;
};
struct panel_device {
	int id;
	int dsi_id;

	struct spi_device *spi;
	struct copr_info copr;
	struct copr_spi_gpios spi_gpio;

	/* mutex lock for panel operations */
	struct mutex op_lock;
	/* mutex lock for panel's data */
	struct mutex data_lock;
	/* mutex lock for panel's ioctl */
	struct mutex io_lock;

	struct device *dev;
	struct mdnie_info mdnie;

	struct lcd_device *lcd;
	struct panel_bl_device panel_bl;

	unsigned char panel_id[3];

	struct v4l2_subdev sd;

	struct panel_pad pad;

	struct decon_lcd lcd_info;

	struct mipi_drv_ops mipi_drv;

	struct panel_state state;

	struct workqueue_struct *disp_det_workqueue;
	struct work_struct disp_det_work;
	struct notifier_block fb_notif;
#ifdef CONFIG_DISPLAY_USE_INFO
	struct notifier_block panel_dpui_notif;
#endif
	struct panel_info panel_data;

#ifdef CONFIG_ACTIVE_CLOCK
	struct act_clock_dev act_clk_dev;
#endif
	struct host_cb reset_cb;

	ktime_t ktime_panel_on;
	ktime_t ktime_panel_off;
};


#define PANEL_DRV_NAME "panel-drv"

#define PANEL_IOC_BASE	'P'

#define PANEL_IOC_DSIM_PROBE			_IOW(PANEL_IOC_BASE, 1, struct decon_lcd *)
#define PANEL_IOC_DECON_PROBE			_IOW(PANEL_IOC_BASE, 2, struct decon_lcd *)
#define PANEL_IOC_GET_PANEL_STATE		_IOW(PANEL_IOC_BASE, 3, struct panel_state *)
#define PANEL_IOC_DSIM_PUT_MIPI_OPS		_IOR(PANEL_IOC_BASE, 4, struct mipi_ops *)
#define PANEL_IOC_PANEL_PROBE			_IO(PANEL_IOC_BASE, 5)

#define PANEL_IOC_SET_POWER				_IO(PANEL_IOC_BASE, 6)

#define PANEL_IOC_SLEEP_IN				_IO(PANEL_IOC_BASE, 7)
#define PANEL_IOC_SLEEP_OUT				_IO(PANEL_IOC_BASE, 8)

#define PANEL_IOC_DISP_ON				_IO(PANEL_IOC_BASE, 9)

#define PANEL_IOC_PANEL_DUMP			_IO(PANEL_IOC_BASE, 10)

#define PANEL_IOC_EVT_FRAME_DONE		_IOW(PANEL_IOC_BASE, 11, struct timespec *)
#define PANEL_IOC_EVT_VSYNC				_IOW(PANEL_IOC_BASE, 12, struct timespec *)

#ifdef CONFIG_SUPPORT_DOZE
#define PANEL_IOC_DOZE					_IO(PANEL_IOC_BASE, 13)
#endif

#ifdef CONFIG_SUPPORT_DSU
#define PANEL_IOC_SET_DSU				_IOW(PANEL_IOC_BASE, 14, struct dsu_info *)
#endif
#define PANEL_IOC_REG_RESET_CB			_IOR(PANEL_IOC_BASE, 15, struct host_cb *)

#define PANEL_IOC_SET_PANEL_RESET		_IO(PANEL_IOC_BASE, 16)

#endif //__PANEL_DRV_H__
