/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *
 *	Author: jb09.kim
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/
#ifndef SAMSUNG_DSI_PANEL_COMMON_H
#define SAMSUNG_DSI_PANEL_COMMON_H

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/err.h>
#include <linux/lcd.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <asm/div64.h>
#include <linux/interrupt.h>
#include <linux/msm-bus.h>
/*#include <linux/hall.h>*/
#include <linux/sync.h>
#include <linux/sw_sync.h>
#include <linux/sched.h>
#include <linux/dma-buf.h>
#include <linux/debugfs.h>
#include <linux/wakelock.h>
#include <linux/miscdevice.h>

#include "../mdss.h"
#include "../mdss_panel.h"
#include "../mdss_dsi.h"
#include "../mdss_debug.h"

#include "ss_ddi_spi_common.h"
#include "ss_dpui_common.h"

#include "ss_dsi_panel_sysfs.h"
#include "ss_dsi_panel_debug.h"

#include "ss_self_display_common.h"

#include <linux/sec_debug.h>

#define LCD_DEBUG(X, ...) pr_debug("[MDSS] %s : "X, __func__, ## __VA_ARGS__)
#define LCD_INFO(X, ...) pr_info("[MDSS] %s : "X, __func__, ## __VA_ARGS__)
#define LCD_INFO_ONCE(X, ...) pr_info_once("[MDSS] %s : "X, __func__, ## __VA_ARGS__)
#define LCD_ERR(X, ...) pr_err("[MDSS] %s : "X, __func__, ## __VA_ARGS__)

#define MAX_PANEL_NAME_SIZE 100

#define SUPPORT_PANEL_COUNT 2
#define SUPPORT_PANEL_REVISION 20
#define PARSE_STRING 64
#define MAX_EXTRA_POWER_GPIO 4
#define MAX_BACKLIGHT_TFT_GPIO 4

/* OSC TE FITTING */
#define OSC_TE_FITTING_LUT_MAX 2

/* Register dump info */
#define MAX_INTF_NUM 2

/* Panel Unique Cell ID Byte count */
#define MAX_CELL_ID 11

/* Panel Unique OCTA ID Byte count */
#define MAX_OCTA_ID 20

/* PBA booting ID */
#define PBA_ID 0xFFFFFF

/* Default elvss_interpolation_lowest_temperature */
#define ELVSS_INTERPOLATION_TEMPERATURE -20

/* Default lux value for entering mdnie HBM */
#define ENTER_HBM_LUX 40000

/* MAX ESD Recovery gpio */
#define MAX_ESD_GPIO 2

#define BASIC_FB_PANLE_TYPE 0x01
#define NEW_FB_PANLE_TYPE 0x00
#define OTHERLINE_WORKQ_DEALY 900 /*ms*/
#define OTHERLINE_WORKQ_CNT 70

#define MDNIE_TUNE_MAX_SIZE 6

extern int poweroff_charging;

enum mipi_samsung_tx_cmd_list {
	TX_CMD_NULL,
	TX_DISPLAY_ON,
	TX_DISPLAY_OFF,
	TX_BRIGHT_CTRL,
	TX_MANUFACTURE_ID_READ_PRE,
	TX_LEVEL1_KEY_ENABLE,
	TX_LEVEL1_KEY_DISABLE,
	TX_LEVEL2_KEY_ENABLE,
	TX_LEVEL2_KEY_DISABLE,
	TX_MDNIE_ADB_TEST,
	TX_LPM_ON,
	TX_LPM_OFF,
	TX_LPM_HZ_NONE,
	TX_LPM_1HZ,
	TX_LPM_2HZ,
	TX_LPM_30HZ,
	TX_LPM_AOD_ON,
	TX_LPM_AOD_OFF,
	TX_LPM_2NIT_CMD,
	TX_LPM_40NIT_CMD,
	TX_LPM_60NIT_CMD,
	TX_ALPM_2NIT_CMD,
	TX_ALPM_40NIT_CMD,
	TX_ALPM_60NIT_CMD,
	TX_ALPM_2NIT_OFF,
	TX_ALPM_40NIT_OFF,
	TX_ALPM_60NIT_OFF,
	TX_HLPM_2NIT_CMD,
	TX_HLPM_40NIT_CMD,
	TX_HLPM_60NIT_CMD,
	TX_HLPM_2NIT_OFF,
	TX_HLPM_40NIT_OFF,
	TX_HLPM_60NIT_OFF,
	TX_PACKET_SIZE,
	TX_REG_READ_POS,
	TX_MDNIE_TUNE,
	TX_OSC_TE_FITTING,
	TX_AVC_ON,
	TX_LDI_FPS_CHANGE,
	TX_HMT_ENABLE,
	TX_HMT_DISABLE,
	TX_HMT_LOW_PERSISTENCE_OFF_BRIGHT,
	TX_HMT_REVERSE,
	TX_HMT_FORWARD,
	TX_CABC_ON,
	TX_CABC_OFF,
	TX_TFT_PWM,
	TX_BLIC_DIMMING,
	TX_LDI_SET_VDD_OFFSET,
	TX_LDI_SET_VDDM_OFFSET,
	TX_HSYNC_ON,
	TX_CABC_ON_DUTY,
	TX_CABC_OFF_DUTY,
	TX_SPI_ENABLE,
	TX_COLOR_WEAKNESS_ENABLE,
	TX_COLOR_WEAKNESS_DISABLE,
	TX_ESD_RECOVERY_1,
	TX_ESD_RECOVERY_2,
	TX_MCD_ON,
	TX_MCD_OFF,
	TX_GRADUAL_ACL,
	TX_HW_CURSOR,
	TX_MULTIRES_FHD_TO_WQHD,
	TX_MULTIRES_HD_TO_WQHD,
	TX_MULTIRES_FHD,
	TX_MULTIRES_HD,
	TX_COVER_CONTROL_ENABLE,
	TX_COVER_CONTROL_DISABLE,
	TX_HBM_GAMMA,
	TX_HBM_ETC,
	TX_HBM_IRC,
	TX_HBM_OFF,
	TX_AID,
	TX_AID_SUBDIVISION,
	TX_ACL_ON,
	TX_ACL_OFF,
	TX_ELVSS,
	TX_ELVSS_HIGH,
	TX_ELVSS_MID,
	TX_ELVSS_LOW,
	TX_ELVSS_PRE,
	TX_GAMMA,
	TX_HMT_ELVSS,
	TX_HMT_VINT,
	TX_HMT_IRC,
	TX_HMT_GAMMA,
	TX_HMT_AID,
	TX_ELVSS_LOWTEMP,
	TX_ELVSS_LOWTEMP2,
	TX_SMART_ACL_ELVSS,
	TX_SMART_ACL_ELVSS_LOWTEMP,
	TX_VINT,
	TX_IRC,
	TX_IRC_SUBDIVISION,
	TX_IRC_OFF,

	/* self display operation */
	TX_SELF_DISP_ON,
	TX_SELF_DISP_OFF,

	/* self mask */
	TX_SELF_MASK_SIDE_MEM_SET,
	TX_SELF_MASK_ON,
	TX_SELF_MASK_OFF,
	TX_SELF_MASK_IMAGE,

	/* self move */
	TX_SELF_MOVE_SMALL_JUMP_ON,
	TX_SELF_MOVE_MID_BIG_JUMP_ON,
	TX_SELF_MOVE_OFF,
	TX_SELF_MOVE_2C_SYNC_OFF,

	/* self icon, self grid */
	TX_SELF_ICON_SIDE_MEM_SET,
	TX_SELF_ICON_ON_GRID_ON,
	TX_SELF_ICON_ON_GRID_OFF,
	TX_SELF_ICON_OFF_GRID_ON,
	TX_SELF_ICON_OFF_GRID_OFF,
	TX_SELF_ICON_IMAGE,

	TX_SELF_ICON_GRID_2C_SYNC_OFF,

	TX_SELF_BRIGHTNESS_ICON_ON,
	TX_SELF_BRIGHTNESS_ICON_OFF,

	/* self clock */
	TX_SELF_ACLOCK_IMAGE,
	TX_SELF_ACLOCK_SIDE_MEM_SET,
	TX_SELF_ACLOCK_ON,
	TX_SELF_ACLOCK_TIME_UPDATE,
	TX_SELF_ACLOCK_ROTATION,
	TX_SELF_ACLOCK_OFF,

	TX_SELF_DCLOCK_IMAGE,
	TX_SELF_DCLOCK_SIDE_MEM_SET,
	TX_SELF_DCLOCK_ON,
	TX_SELF_DCLOCK_BLINKING_ON,
	TX_SELF_DCLOCK_BLINKING_OFF,
	TX_SELF_DCLOCK_TIME_UPDATE,
	TX_SELF_DCLOCK_OFF,

	TX_SELF_CLOCK_2C_SYNC_OFF,

	/* START POC CMDS */
	TX_POC_CMD_START,
	TX_POC_WRITE_1BYTE,
	TX_POC_ERASE,
	TX_POC_ERASE1,
	TX_POC_PRE_WRITE,
	TX_POC_WRITE_CONTINUE,
	TX_POC_WRITE_CONTINUE2,
	TX_POC_WRITE_CONTINUE3,
	TX_POC_WRITE_END,
	TX_POC_POST_WRITE,
	TX_POC_PRE_READ,
	TX_POC_READ,
	TX_POC_POST_READ,
	TX_POC_REG_READ_POS,
	TX_POC_CMD_END,
	/* END POC CMDS */

	TX_LDI_LOG_DISABLE,
	TX_LDI_LOG_ENABLE,
	TX_CMD_MAX,
};

enum mipi_samsung_rx_cmd_list {
	RX_CMD_NULL = TX_CMD_MAX,
	RX_SMART_DIM_MTP,
	RX_MANUFACTURE_ID,
	RX_MANUFACTURE_ID0,
	RX_MANUFACTURE_ID1,
	RX_MANUFACTURE_ID2,
	RX_MANUFACTURE_DATE,
	RX_DDI_ID,
	RX_CELL_ID,
	RX_OCTA_ID,
	RX_RDDPM,
	RX_MTP_READ_SYSFS,
	RX_IRC,
	RX_ELVSS,
	RX_HBM,
	RX_HBM2,
	RX_MDNIE,
	RX_LDI_DEBUG0, // 0x0A : RDDPM
	RX_LDI_DEBUG1,
	RX_LDI_DEBUG2, // 0xEE : ERR_FG
	RX_LDI_DEBUG3, // 0x0E : RDDSM
	RX_LDI_DEBUG4, // 0x05 : DSI_ERR
	RX_LDI_DEBUG5, // 0x0F : OTP loading error count
	RX_LDI_ERROR, // 0xEA
	RX_LDI_LOADING_DET,
	RX_LDI_FPS,
	RX_POC_READ,
	RX_POC_STATUS,
	RX_POC_CHECKSUM,
	RX_GCT_CHECKSUM,
	RX_SELF_DISP_DEBUG,
	RX_LDI_LOG,
	RX_CMD_MAX,
};

enum PANEL_LEVEL_KEY {
	LEVEL_KEY_NONE,
	LEVEL1_KEY,
	LEVEL2_KEY,
};

enum mipi_samsung_cmd_map_list {
	PANEL_CMD_MAP_NULL,
	PANEL_CMD_MAP_MAX,
};

enum {
	MIPI_RESUME_STATE,
	MIPI_SUSPEND_STATE,
};

enum {
	ACL_OFF,
	ACL_30,
	ACL_15,
	ACL_50
};

enum {
	TE_FITTING_DONE = BIT(0),
	TE_CHECK_ENABLE = BIT(1),
	TE_FITTING_REQUEST_IRQ = BIT(3),
	TE_FITTING_STEP1 = BIT(4),
	TE_FITTING_STEP2 = BIT(5),
};

enum ACL_GRADUAL_MODE {
	GRADUAL_ACL_OFF,
	GRADUAL_ACL_ON,
	GRADUAL_ACL_UNSTABLE,
};

#define SAMSUNG_DISPLAY_PINCTRL0_STATE_DEFAULT "samsung_display_gpio_control0_default"
#define SAMSUNG_DISPLAY_PINCTRL0_STATE_SLEEP  "samsung_display_gpio_control0_sleep"
#define SAMSUNG_DISPLAY_PINCTRL1_STATE_DEFAULT "samsung_display_gpio_control0_default"
#define SAMSUNG_DISPLAY_PINCTRL1_STATE_SLEEP  "samsung_display_gpio_control0_sleep"

enum {
	SAMSUNG_GPIO_CONTROL0,
	SAMSUNG_GPIO_CONTROL1,
};

/* foder open : 0, close : 1 */
enum {
	HALL_IC_OPEN,
	HALL_IC_CLOSE,
	HALL_IC_UNDEFINED,
};

enum IRC_MODE {
	IRC_LRU_MODE = 0x0D,
	IRC_CLIP_MODE = 0x1D,
	IRC_FLAT_GAMMA_MODE = 0x2D,
	IRC_CLEAR_COLOR_MODE = 0x3D,
	IRC_MODERATO_MODE = 0x6D,
	IRC_MAX_MODE = IRC_MODERATO_MODE + 1,
};

struct panel_irc_info {
	int irc_enable_status;
	enum IRC_MODE irc_mode;
};

struct te_fitting_lut {
	int te_duration;
	int value;
};

struct osc_te_fitting_info {
	unsigned int status;
	long long te_duration;
	long long *te_time;
	int sampling_rate;
	struct completion te_check_comp;
	struct work_struct work;
	struct te_fitting_lut *lut[OSC_TE_FITTING_LUT_MAX];
};

struct panel_lpm_info {
	u8 mode;
	u8 hz;
	int lpm_bl_level;
	int normal_bl_level;
	bool esd_recovery;
};

extern char mdss_mdp_panel[MDSS_MAX_PANEL_LEN];
struct samsung_display_driver_data *samsung_get_vdd(void);
struct mdss_dsi_ctrl_pdata *samsung_get_dsi_ctrl(struct samsung_display_driver_data *vdd);

struct cmd_map {
	int *bl_level;
	int *cmd_idx;
	int size;
};

struct candela_map_table {
	int tab_size;
	int *scaled_idx;
	int *idx;
	int *from;
	int *end;
	int *cd;
	int *interpolation_cd;
	int *bkl;
	int min_lv;
	int max_lv;
};

struct hbm_candela_map_table {
	int tab_size;
	int *cd;
	int *idx;
	int *from;
	int *end;
	int *auto_level;
	int hbm_min_lv;
	int hbm_max_lv;
};

struct samsung_display_dtsi_data {
	bool samsung_lp11_init;
	bool samsung_tcon_clk_on_support;
	bool samsung_esc_clk_128M;
	bool samsung_osc_te_fitting;
	bool samsung_support_factory_panel_swap;
	u32  samsung_power_on_reset_delay;
	u32  samsung_power_pre_off_reset_delay;
	u32  samsung_power_off_reset_delay;
	u32  samsung_dsi_off_reset_delay;
	u32 samsung_lpm_init_delay;

	/* To reduce DISPLAY ON time */
	u32 samsung_reduce_display_on_time;
	u32 samsung_dsi_force_clock_lane_hs;
	u32 samsung_wait_after_reset_delay;
	u32 samsung_wait_after_sleep_out_delay;
	u32 samsung_finger_print_irq_num;
	u32 samsung_home_key_irq_num;

	/*
	 * index[0] : array index for te fitting command from "ctrl->on_cmd"
	 * index[1] : array index for te fitting command from "osc_te_fitting_tx_cmds"
	 */
	int samsung_osc_te_fitting_cmd_index[2];
	int panel_extra_power_gpio[MAX_EXTRA_POWER_GPIO];
	int backlight_tft_gpio[MAX_BACKLIGHT_TFT_GPIO];

	/* PANEL TX / RX CMDS LIST */
	struct dsi_panel_cmds panel_tx_cmd_list[TX_CMD_MAX][SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds panel_rx_cmd_list[RX_CMD_MAX][SUPPORT_PANEL_REVISION];

	struct candela_map_table candela_map_table[SUPPORT_PANEL_REVISION];
	struct candela_map_table pac_candela_map_table[SUPPORT_PANEL_REVISION];
	struct hbm_candela_map_table hbm_candela_map_table[SUPPORT_PANEL_REVISION];
	struct hbm_candela_map_table pac_hbm_candela_map_table[SUPPORT_PANEL_REVISION];

	struct cmd_map aid_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map vint_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map acl_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map elvss_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map smart_acl_elvss_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map caps_map_table[SUPPORT_PANEL_REVISION];
	struct cmd_map copr_br_map_table[SUPPORT_PANEL_REVISION];

	struct candela_map_table scaled_level_map_table[SUPPORT_PANEL_REVISION];

	struct cmd_map hmt_reverse_aid_map_table[SUPPORT_PANEL_REVISION];
	struct candela_map_table hmt_candela_map_table[SUPPORT_PANEL_REVISION];

	bool panel_lpm_enable;
	bool hmt_enabled;

	/* TFT LCD Features*/
	int tft_common_support;
	int backlight_gpio_config;
	int pwm_ap_support;
	const char *tft_module_name;
	const char *panel_vendor;
	const char *disp_model;

	/* MDINE HBM_CE_TEXT_MDNIE mode used */
	int hbm_ce_text_mode_support;

	/* Backlight IC discharge delay */
	int blic_discharging_delay_tft;
	int cabc_delay;
};

struct samsung_mdnie_tune_data {
	/* Brightness packet set */
	struct dsi_panel_cmds mdnie_tune_packet_tx_cmds_dsi;
};

struct display_status {
	int wait_disp_on;
	int wait_actual_disp_on;
	int aod_delay;

	int hbm_mode;

	int elvss_value1;
	int elvss_value2;
	int irc_value1;
	int irc_value2;
	int disp_on_pre;
	int hall_ic_status;
	int hall_ic_mode_change_trigger;
};

struct hmt_status {
	unsigned int hmt_on;
	unsigned int hmt_reverse;
	unsigned int hmt_is_first;

	int hmt_bl_level;
	int candela_level_hmt;
	int cmd_idx_hmt;

	int (*hmt_enable)(struct mdss_dsi_ctrl_pdata *ctrl, struct samsung_display_driver_data *vdd);
	int (*hmt_reverse_update)(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
	int (*hmt_bright_update)(struct mdss_dsi_ctrl_pdata *ctrl);
};

struct multires_status {
	bool is_support;
	unsigned int curr_mode;
	unsigned int prev_mode;
};

enum {
	MULTIRES_WQHD,
	MULTIRES_FHD,
	MULTIRES_HD,
};

struct esd_recovery {
	spinlock_t irq_lock;
	bool esd_recovery_init;
	bool is_enabled_esd_recovery;
	bool is_wakeup_source;
	int esd_gpio[MAX_ESD_GPIO];
	u8 num_of_gpio;
	unsigned long irqflags[MAX_ESD_GPIO];
	void (*esd_irq_enable)(bool enable, bool nosync, void *data);
};

/* Panel LPM(ALPM/HLPM) status flag */
enum {
	MODE_OFF = 0,			/* Panel LPM Mode OFF */
	ALPM_MODE_ON,			/* ALPM Mode On */
	HLPM_MODE_ON,			/* HLPM Mode On */
	MAX_LPM_MODE,			/* Panel LPM Mode MAX */
};

enum {
	PANEL_LPM_2NIT = 0,
	PANEL_LPM_40NIT,
	PANEL_LPM_60NIT,
	PANEL_LPM_BRIGHTNESS_MAX,
};

struct panel_func {
	/* ON/OFF */
	int (*samsung_panel_on_pre)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_panel_on_post)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_panel_off_pre)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_panel_off_post)(struct mdss_dsi_ctrl_pdata *ctrl);
	void (*samsung_backlight_late_on)(struct mdss_dsi_ctrl_pdata *ctrl);
	void (*samsung_panel_init)(struct samsung_display_driver_data *vdd);

	/* DDI RX */
	char (*samsung_panel_revision)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_manufacture_date_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_ddi_id_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_cell_id_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_octa_id_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_hbm_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_elvss_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_mdnie_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_irc_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_smart_dimming_init)(struct mdss_dsi_ctrl_pdata *ctrl);
	struct smartdim_conf *(*samsung_smart_get_conf)(void);

	/* Brightness */
	struct dsi_panel_cmds * (*samsung_brightness_hbm_off)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_aid)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_acl_on)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_pre_acl_percent)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_acl_percent)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_acl_off)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_pre_elvss)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_elvss)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_pre_caps)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_caps)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_elvss_temperature1)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_elvss_temperature2)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_vint)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_irc)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_gamma)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);

	/* HBM */
	struct dsi_panel_cmds * (*samsung_hbm_gamma)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_hbm_etc)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_hbm_irc)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	int (*get_hbm_candela_value)(int level);

	/* Event */
	void (*mdss_samsung_event_frame_update)(struct mdss_panel_data *pdata, int event, void *arg);
	void (*mdss_samsung_event_fb_event_callback)(struct mdss_panel_data *pdata, int event, void *arg);
	void (*mdss_samsung_event_osc_te_fitting)(struct mdss_panel_data *pdata, int event, void *arg);
	void (*mdss_samsung_event_esd_recovery_init)(struct mdss_panel_data *pdata, int event, void *arg);

	/* OSC Tuning */
	int (*samsung_osc_te_fitting)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_change_ldi_fps)(struct mdss_dsi_ctrl_pdata *ctrl, unsigned int input_fps);

	/* HMT */
	struct dsi_panel_cmds * (*samsung_brightness_gamma_hmt)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_aid_hmt)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_elvss_hmt)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_vint_hmt)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds * (*samsung_brightness_irc_hmt)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);

	int (*samsung_smart_dimming_hmt_init)(struct mdss_dsi_ctrl_pdata *ctrl);
	struct smartdim_conf *(*samsung_smart_get_conf_hmt)(void);

	/* TFT */
	void (*samsung_tft_blic_init)(struct mdss_dsi_ctrl_pdata *ctrl);
	void (*samsung_brightness_tft_pwm)(struct mdss_dsi_ctrl_pdata *ctrl, int level);
	struct dsi_panel_cmds * (*samsung_brightness_tft_pwm_ldi)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);

	void (*samsung_bl_ic_pwm_en)(int enable);
	void (*samsung_bl_ic_i2c_ctrl)(int scaled_level);
	void (*samsung_bl_ic_outdoor)(int enable);

	/*LVDS*/
	void (*samsung_ql_lvds_register_set)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_lvds_write_reg)(u16 addr, u32 data);

	/* Panel LPM(ALPM/HLPM) */
	void (*samsung_get_panel_lpm_mode)(struct mdss_dsi_ctrl_pdata *ctrl, u8 *mode);

	/* A3 line panel data parsing fn */
	int (*parsing_otherline_pdata)(struct file *f, struct samsung_display_driver_data *vdd,
		char *src, int len);
	void (*set_panel_fab_type)(int type);
	int (*get_panel_fab_type)(void);

	/* color weakness */
	void (*color_weakness_ccb_on_off)(struct samsung_display_driver_data *vdd, int mode);

	/* DDI H/W Cursor */
	int (*ddi_hw_cursor)(struct mdss_dsi_ctrl_pdata *ctrl, int *input);

	/* MULTI_RESOLUTION */
	void (*samsung_multires)(struct samsung_display_driver_data *vdd);

	/* panel reset related */
	int (*samsung_panel_power_ic_control)(struct mdss_dsi_ctrl_pdata *ctrl, int enable);

	/* COVER CONTROL */
	void (*samsung_cover_control)(struct mdss_dsi_ctrl_pdata *ctrl, struct samsung_display_driver_data *vdd);

	/* COPR read */
	void (*samsung_copr_read)(struct samsung_display_driver_data *vdd);
	void (*samsung_set_copr_sum)(struct samsung_display_driver_data *vdd);
	void (*samsung_copr_enable)(struct samsung_display_driver_data *vdd, int enable);

	int (*samsung_poc_ctrl)(struct samsung_display_driver_data *vdd, u32 cmd);
};

struct samsung_register_info {
	size_t virtual_addr;
};

struct samsung_register_dump_info {
	/* DSI PLL */
	struct samsung_register_info dsi_pll;

	/* DSI CTRL */
	struct samsung_register_info dsi_ctrl;

	/* DSI PHY */
	struct samsung_register_info dsi_phy;
};

struct samsung_display_debug_data {
	struct dentry *root;
	struct dentry *dump;
	struct dentry *hw_info;
	struct dentry *display_status;

	bool print_cmds;
	bool *is_factory_mode;
	bool panic_on_pptimeout;
};

struct copr_spi_gpios {
	int clk;
	int miso;
	int mosi;
	int cs;
};

struct COPR {
	int copr_on;
	int copr_data;
	int copr_avr;
	int cd_avr;
	ktime_t cur_t;
	ktime_t last_t;
	s64 total_t;
	s64 copr_sum;
	s64 cd_sum;
	struct mutex copr_lock;
	struct workqueue_struct *read_copr_wq;
	struct work_struct read_copr_work;
	struct copr_spi_gpios copr_spi_gpio;
};

enum {
	POC_OP_NONE = 0,
	POC_OP_ERASE,
	POC_OP_WRITE,
	POC_OP_READ,
	POC_OP_ERASE_WRITE_IMG,
	POC_OP_ERASE_WRITE_TEST,
	POC_OP_BACKUP,
	POC_OP_CHECKSUM,
	POC_OP_CHECK_FLASH,
	POC_OP_SET_FLASH_WRITE,
	POC_OP_SET_FLASH_EMPTY,
	MAX_POC_OP,
};

enum poc_state {
	POC_STATE_NONE,
	POC_STATE_FLASH_EMPTY,
	POC_STATE_FLASH_FILLED,
	POC_STATE_ER_START,
	POC_STATE_ER_PROGRESS,
	POC_STATE_ER_COMPLETE,
	POC_STATE_ER_FAILED,
	POC_STATE_WR_START,
	POC_STATE_WR_PROGRESS,
	POC_STATE_WR_COMPLETE,
	POC_STATE_WR_FAILED,
	MAX_POC_STATE,
};

enum mdss_cpufreq_cluster {
	CPUFREQ_CLUSTER_BIG,
	CPUFREQ_CLUSTER_LITTLE,
	CPUFREQ_CLUSTER_ALL,
};

#define IOC_GET_POC_STATUS	_IOR('A', 100, __u32)		/* 0:NONE, 1:ERASED, 2:WROTE, 3:READ */
#define IOC_GET_POC_CHKSUM	_IOR('A', 101, __u32)		/* 0:CHKSUM ERROR, 1:CHKSUM SUCCESS */
#define IOC_GET_POC_CSDATA	_IOR('A', 102, __u32)		/* CHKSUM DATA 4 Bytes */
#define IOC_GET_POC_ERASED	_IOR('A', 103, __u32)		/* 0:NONE, 1:NOT ERASED, 2:ERASED */
#define IOC_GET_POC_FLASHED	_IOR('A', 104, __u32)		/* 0:NOT POC FLASHED(0x53), 1:POC FLAHSED(0x52) */

#define IOC_SET_POC_ERASE	_IOR('A', 110, __u32)		/* ERASE POC FLASH */
#define IOC_SET_POC_TEST	_IOR('A', 112, __u32)		/* POC FLASH TEST - ERASE/WRITE/READ/COMPARE */

/* POC */
struct POC {
	bool is_support;
	u32 file_opend;
	struct miscdevice dev;
	bool erased;
	atomic_t cancel;
#ifdef CONFIG_DISPLAY_USE_INFO
	struct notifier_block dpui_notif;
#endif

	u8 chksum_data[4];
	u8 chksum_res;

	u8 *wbuf;
	u32 wpos;
	u32 wsize;

	u8 *rbuf;
	u32 rpos;
	u32 rsize;

	u32 erase_delay_ms; /* msleep */
	u32 erase_delay_us; /* usleep */
	u32 write_delay_us; /* usleep */
};

struct samsung_display_driver_data {
	/*
	*	PANEL COMMON DATA
	*/
	struct mutex vdd_lock;
	struct mutex vdd_blank_unblank_lock;
	struct mutex vdd_hall_ic_blank_unblank_lock;
	struct mutex vdd_hall_ic_lock;
	struct mutex vdd_panel_lpm_lock;
	struct mutex vdd_poc_operation_lock;
	struct mutex vdd_mdss_direct_cmdlist_lock;

	struct samsung_display_debug_data *debug_data;

	int vdd_blank_mode[SUPPORT_PANEL_COUNT];

	int support_panel_max;

	int support_mdnie_lite;

	int support_mdnie_trans_dimming;

	bool is_factory_mode;

	int support_hall_ic;
	struct notifier_block hall_ic_notifier_display;

	int panel_attach_status; /* 0bit->DSI0 1bit->DSI1 */

	int panel_revision;

	char *panel_name;
	char *panel_vendor;

	int recovery_boot_mode;

	int elvss_interpolation_temperature;
	int temperature;
	char temperature_value;

	int lux;
	int enter_hbm_lux;

	int auto_brightness;
	int prev_auto_brightness;
	int bl_level;
	int pac_bl_level; // scaled idx
	int candela_level;
	int interpolation_cd;
	int cd_idx;		// original idx
	int pac_cd_idx; // scaled idx

	int acl_status;
	int siop_status;
	bool mdnie_tuning_enable_tft;
	int mdnie_tune_size[MDNIE_TUNE_MAX_SIZE];
	int mdnie_disable_trans_dimming;
	u32 samsung_hw_config;

	int samsung_support_irc;
	struct panel_irc_info irc_info;

	struct panel_func panel_func;

	struct msm_fb_data_type *mfd_dsi[SUPPORT_PANEL_COUNT];

	struct mdss_dsi_ctrl_pdata *ctrl_dsi[DSI_CTRL_MAX];

	struct samsung_display_dtsi_data dtsi_data[SUPPORT_PANEL_COUNT];

	struct display_status display_status_dsi[SUPPORT_PANEL_COUNT];

	/* register dump info */
	struct samsung_register_dump_info dump_info[MAX_INTF_NUM];

	/*
	*	PANEL OPERATION DATA
	*/
	int manufacture_id_dsi[SUPPORT_PANEL_COUNT];

	int manufacture_date_loaded_dsi[SUPPORT_PANEL_COUNT];
	int manufacture_date_dsi[SUPPORT_PANEL_COUNT];
	int manufacture_time_dsi[SUPPORT_PANEL_COUNT];

	int mdnie_loaded_dsi[SUPPORT_PANEL_COUNT];
	struct mdnie_lite_tun_type *mdnie_tune_state_dsi[SUPPORT_PANEL_COUNT];
	int mdnie_x[SUPPORT_PANEL_COUNT];
	int mdnie_y[SUPPORT_PANEL_COUNT];

	int ddi_id_loaded_dsi[SUPPORT_PANEL_COUNT];
	int ddi_id_dsi[SUPPORT_PANEL_COUNT][5];

	/* Panel Unique Cell ID */
	int cell_id_loaded_dsi[SUPPORT_PANEL_COUNT];
	int cell_id_dsi[SUPPORT_PANEL_COUNT][MAX_CELL_ID];

	/* Panel Unique OCTA ID */
	int octa_id_loaded_dsi[SUPPORT_PANEL_COUNT];
	int octa_id_dsi[SUPPORT_PANEL_COUNT][MAX_OCTA_ID];

	int hbm_loaded_dsi[SUPPORT_PANEL_COUNT];
	int elvss_loaded_dsi[SUPPORT_PANEL_COUNT];
	int irc_loaded_dsi[SUPPORT_PANEL_COUNT];
	int smart_dimming_loaded_dsi[SUPPORT_PANEL_COUNT];
	struct smartdim_conf *smart_dimming_dsi[SUPPORT_PANEL_COUNT];

	/*
	*	Brightness control packet
	*/
	struct samsung_brightenss_data *brightness[SUPPORT_PANEL_COUNT];

	/*
	 *	MDNIE tune data packet
	 */
	struct samsung_mdnie_tune_data mdnie_tune_data[SUPPORT_PANEL_COUNT];

	/*
	 * OSC TE fitting info
	 */
	struct osc_te_fitting_info te_fitting_info;

	/*
	 *  HMT
	 */
	struct hmt_status hmt_stat;
	int smart_dimming_hmt_loaded_dsi[SUPPORT_PANEL_COUNT];
	struct smartdim_conf *smart_dimming_dsi_hmt[SUPPORT_PANEL_COUNT];
	/*
	* MULTI_RESOLUTION
	*/

	struct multires_status multires_stat;

	/* CABC feature */
	int support_cabc;

	/* TFT BL DCS Scaled level*/
	int scaled_level;

	/* TFT LCD Features*/
	int (*backlight_tft_config)(struct mdss_panel_data *pdata, int enable);
	void (*backlight_tft_pwm_control)(struct mdss_dsi_ctrl_pdata *pdata, int bl_lvl);
	void (*mdss_panel_tft_outdoormode_update)(struct mdss_dsi_ctrl_pdata *pdata);
	/* ESD */
	struct esd_recovery esd_recovery;

	/*Image dump*/
	struct workqueue_struct *image_dump_workqueue;
	struct work_struct image_dump_work;

	/* Other line panel support */
	struct workqueue_struct *other_line_panel_support_workq;
	struct delayed_work other_line_panel_support_work;
	int other_line_panel_work_cnt;

	/* auto brightness level */
	int auto_brightness_level;

	/* weakness hbm comp */
	int weakness_hbm_comp;

	/* Panel LPM info */
	struct panel_lpm_info panel_lpm;

	/* send recovery pck before sending image date (for ESD recovery) */
	int send_esd_recovery;

	/* DOZE */
	struct wake_lock doze_wakelock;

	/* graudal acl */
	int gradual_acl_val;
	int gradual_pre_acl_on;
	int gradual_acl_update;
	enum ACL_GRADUAL_MODE gradual_acl_status;

	/* ldu correction */
	int ldu_correction_state;
	int ldu_bl_backup;

	/* color weakness */
	int color_weakness_mode;
	int color_weakness_level;
	int color_weakness_value; /*mode+level*/
	int ccb_bl_backup;
	int ccb_cd;
	int ccb_stepping;

	/* Cover Control Status */
	int cover_control;

	int select_panel_gpio;

	/* Power Control for LPM */
	bool lpm_power_control;
	char lpm_power_control_supply_name[32];
	int lpm_power_control_supply_min_voltage;
	int lpm_power_control_supply_max_voltage;

#ifdef CONFIG_DISPLAY_USE_INFO
	struct notifier_block dpui_notif;
#endif

	/* COPR */
	struct COPR copr;

	/* SPI device */
	struct spi_device *spi_dev;

	/* POC */
	struct POC poc_driver;

	u32 dsiphy_drive_str;

	/* PAC */
	int pac;

	int poc_operation;

	/* X-Talk */
	int xtalk_mode;

	/* SELF DISPLAY opration */
	struct self_display self_disp;

	int aid_subdivision_enable;

	/* Reset Time check */
	s64 reset_time_64;

	/* Sleep_out cmd time check */
	s64 sleep_out_time_64;

	/* boosting at device resume */
	int bootsing_at_resume;

	/* Initialize the panel before first commit on bootup */
	bool init_panel_before_commit;
};

/*SPI INTERFACE*/
//struct ss_spi_private;

struct ss_spi_data {
	u32		spi_speed;
	u32		max_speed_hz;
	u32		bits_per_word;
	u32		read_chunk_size;
	u32		write_chunk_size;
	u32		mode;
};

struct ss_spi_private {
	struct device			*dev;
	struct dbmdx_spi_data		*pdata;
	struct spi_device		*client;
};


/* COMMON FUNCTION */
void mdss_samsung_panel_init(struct device_node *np, struct mdss_dsi_ctrl_pdata *ctrl_pdata);
void mdss_samsung_dsi_panel_registered(struct mdss_panel_data *pdata);
int mdss_samsung_send_cmd(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_tx_cmd_list cmd);
int mdss_samsung_read_nv_mem(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_panel_cmds *cmds, unsigned char *buffer, int level_key);
void mdss_samsung_panel_parse_dt(struct device_node *np, struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int mdss_samsung_panel_on_pre(struct mdss_panel_data *pdata);
int mdss_samsung_panel_on_post(struct mdss_panel_data *pdata);
int mdss_samsung_panel_off_pre(struct mdss_panel_data *pdata);
int mdss_samsung_panel_off_post(struct mdss_panel_data *pdata);
int mdss_samsung_panel_extra_power(struct mdss_panel_data *pdata, int enable);
int mdss_backlight_tft_gpio_config(struct mdss_panel_data *pdata, int enable);
int mdss_backlight_tft_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl);
void mdss_samsung_panel_data_read(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_panel_cmds *cmds, char *buffer, int level_key);
void mdss_samsung_panel_low_power_config(struct mdss_panel_data *pdata, int enable);
void parse_dt_data(struct device_node *np, void *data,	char *cmd_string, char panel_rev, void *fnc);

/*
 * Check lcd attached status for DISPLAY_1 or DISPLAY_2
 * if the lcd was not attached, the function will return 0
 */
int mdss_panel_attached(int ndx);
int get_lcd_attached(char *mode);
int get_lcd_attached_secondary(char *mode);
int mdss_panel_attached(int ndx);

struct dsi_panel_cmds *get_panel_rx_cmd(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_rx_cmd_list cmd);

struct samsung_display_driver_data *check_valid_ctrl(struct mdss_dsi_ctrl_pdata *ctrl);

char mdss_panel_id0_get(struct mdss_dsi_ctrl_pdata *ctrl);
char mdss_panel_id1_get(struct mdss_dsi_ctrl_pdata *ctrl);
char mdss_panel_id2_get(struct mdss_dsi_ctrl_pdata *ctrl);
char mdss_panel_rev_get(struct mdss_dsi_ctrl_pdata *ctrl);

int mdss_panel_attach_get(struct mdss_dsi_ctrl_pdata *ctrl);
int mdss_panel_attach_set(struct mdss_dsi_ctrl_pdata *ctrl, int status);

int mdss_samsung_read_loading_detection(void);
/* void mdss_samsung_fence_dump(char *interface, struct sync_fence *fence); */
void mdss_samsung_check_hw_config(struct platform_device *pdev);
void mdss_samsung_dsi_force_hs(struct mdss_panel_data *pdata);

struct dsi_panel_cmds *mdss_samsung_cmds_select(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_tx_cmd_list cmd,
	u32 *flags);

/* EXTERN FUNCTION */
extern void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
		struct dsi_panel_cmds *pcmds, u32 flags);
#ifdef CONFIG_FOLDER_HALL
extern void hall_ic_register_notify(struct notifier_block *nb);
#endif

/* EXTERN VARIABLE */
extern struct dsi_status_data *pstatus_data;

/* HMT FUNCTION */
int hmt_enable(struct mdss_dsi_ctrl_pdata *ctrl, struct samsung_display_driver_data *vdd);
int hmt_reverse_update(struct mdss_dsi_ctrl_pdata *ctrl, int enable);

/* HALL IC FUNCTION */
int display_ndx_check(struct mdss_dsi_ctrl_pdata *ctrl);
int samsung_display_hall_ic_status(struct notifier_block *nb,
		unsigned long hall_ic, void *data);

/* CORP CALC */
void mdss_samsung_copr_calc_work(struct work_struct *work);
void mdss_samsung_copr_calc_delayed_work(struct delayed_work *work);

/* PANEL LPM FUNCTION */
int mdss_init_panel_lpm_reg_offset(struct mdss_dsi_ctrl_pdata *ctrl, int (*reg_list)[2],
					struct dsi_panel_cmds *cmd_list[], int list_size);
void mdss_samsung_panel_lpm_ctrl(struct mdss_panel_data *pdata, int enable);
void mdss_samsung_panel_lpm_hz_ctrl(struct mdss_panel_data *pdata, int aod_ctrl);

/* Other line panel support */
#define MAX_READ_LINE_SIZE 256
int read_line(char *src, char *buf, int *pos, int len);
void read_panel_data_work_fn(struct delayed_work *work);

/* check irq event */
void mdss_samsung_resume_event(int irq);

/* get panel cmd */
struct dsi_panel_cmds *get_panel_rx_cmds(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_rx_cmd_list cmd);
struct dsi_panel_cmds *get_panel_tx_cmds(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_tx_cmd_list cmd);

/***************************************************************************************************
*		BRIGHTNESS RELATED.
****************************************************************************************************/

#define DEFAULT_BRIGHTNESS 255
#define DEFAULT_BRIGHTNESS_PAC3 25500

#define BRIGHTNESS_MAX_PACKET 50
#define HBM_MODE 6
#define HBM_CE_MODE 9

struct samsung_brightenss_data {
	/* Brightness packet set */
	struct dsi_cmd_desc brightness_packet[BRIGHTNESS_MAX_PACKET];
};

/* BRIGHTNESS RELATED FUNCTION */
int mdss_samsung_brightness_dcs(struct mdss_dsi_ctrl_pdata *ctrl, int level);
void mdss_samsung_update_brightness_packet(struct dsi_cmd_desc *packet, int *count, struct dsi_panel_cmds *tx_cmd);
void mdss_samsung_brightness_tft_pwm(struct mdss_dsi_ctrl_pdata *ctrl, int level);
void update_packet_level_key_enable(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *packet, int *cmd_cnt, int level_key);
void update_packet_level_key_disable(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *packet, int *cmd_cnt, int level_key);
int mdss_samsung_single_transmission_packet(struct dsi_panel_cmds *cmds);
void mdss_samsung_panel_brightness_init(struct samsung_display_driver_data *vdd);

/* HMT BRIGHTNESS */
int mdss_samsung_brightness_dcs_hmt(struct mdss_dsi_ctrl_pdata *ctrl, int level);
int hmt_bright_update(struct mdss_dsi_ctrl_pdata *ctrl);
void set_hmt_br_values(struct samsung_display_driver_data *vdd, int ndx);

/* TFT BL DCS RELATED FUNCTION */
int get_scaled_level(struct samsung_display_driver_data *vdd, int ndx);
void mdss_tft_autobrightness_cabc_update(struct mdss_dsi_ctrl_pdata *ctrl);

/***************************************************************************************************
*		BRIGHTNESS RELATED END.
****************************************************************************************************/


/* SAMSUNG COMMON HEADER*/
#include "ss_dsi_smart_dimming_common.h"
/* MDNIE_LITE_COMMON_HEADER */
#include "ss_dsi_mdnie_lite_common.h"
/* SAMSUNG MODEL HEADER */

#endif /* PANEL_HEADER END	*/
