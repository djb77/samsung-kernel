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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
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

#include "../../mdss/mdss.h"
#include "../../mdss/mdss_panel.h"
#include "../../mdss/mdss_dsi.h"
#include "../../mdss/mdss_debug.h"
#include "ss_dpui_common.h"
#include "../../../../../fs/sysfs/sysfs.h"

#if defined(CONFIG_SEC_DEBUG)
#include <linux/sec_debug.h>
#endif

#if defined(CONFIG_SEC_INCELL)
#include <linux/sec_incell.h>
#endif

#define LCD_DEBUG(X, ...) pr_debug("[MDSS] %s : "X, __func__, ## __VA_ARGS__)
#define LCD_INFO(X, ...) pr_info("[MDSS] %s : "X, __func__, ## __VA_ARGS__)
#define LCD_ERR(X, ...) pr_err("[MDSS] %s : "X, __func__, ## __VA_ARGS__)

#define MAX_PANEL_NAME_SIZE 100
#define DEFAULT_BRIGHTNESS 255

#define SUPPORT_PANEL_COUNT 2
#define SUPPORT_PANEL_REVISION 20
#define PARSE_STRING 64
#define MAX_EXTRA_POWER_GPIO 4
#define MAX_BACKLIGHT_TFT_GPIO 4

/* Brightness stuff */
#define BRIGHTNESS_MAX_PACKET 50
#define HBM_MODE 6
#define HBM_CE_MODE 9

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
#define ENTER_HBM_LUX 10000

/* MAX ESD Recovery gpio */
#define MAX_ESD_GPIO 2

#define BASIC_FB_PANLE_TYPE 0x01
#define NEW_FB_PANLE_TYPE 0x00
#define OTHERLINE_WORKQ_DEALY 900 /*ms*/
#define OTHERLINE_WORKQ_CNT 70

extern int poweroff_charging;
enum mipi_samsung_cmd_list {
	PANEL_CMD_NULL,
	PANEL_READY_TO_ON,
	PANEL_DISPLAY_ON,
	PANEL_DISPLAY_OFF,
	PANEL_BRIGHT_CTRL,
	PANEL_LEVE_KEY_NONE,
	PANEL_LEVE1_KEY,
	PANEL_LEVE1_KEY_ENABLE,
	PANEL_LEVE1_KEY_DISABLE,
	PANEL_LEVE2_KEY, /*LEVEL2 CONTAINS LEVE1 KEY DISABLE */
	PANEL_LEVE2_KEY_ENABLE,
	PANEL_LEVE2_KEY_DISABLE,
	PANEL_MDNIE_ADB_TEST,
	PANEL_LPM_ON,
	PANEL_LPM_OFF,
	PANEL_LPM_HZ_NONE,
	PANEL_LPM_1HZ,
	PANEL_LPM_2HZ,
	PANEL_LPM_30HZ,
	PANEL_LPM_AOD_ON,
	PANEL_LPM_AOD_OFF,
	PANEL_PACKET_SIZE,
	PANEL_REG_READ_POS,
	PANEL_MDNIE_TUNE,
	PANEL_OSC_TE_FITTING,
	PANEL_AVC_ON,
	PANEL_LDI_FPS_CHANGE,
	PANEL_HMT_ENABLE,
	PANEL_HMT_DISABLE,
	PANEL_HMT_LOW_PERSISTENCE_OFF_BRIGHT,
	PANEL_HMT_REVERSE,
	PANEL_HMT_FORWARD,
	PANEL_CABC_ON,
	PANEL_CABC_OFF,
	PANEL_BLIC_DIMMING,
	PANEL_LDI_SET_VDD_OFFSET,
	PANEL_LDI_SET_VDDM_OFFSET,
	PANEL_HSYNC_ON,
	PANEL_CABC_ON_DUTY,
	PANEL_CABC_OFF_DUTY,
	PANEL_SPI_ENABLE,
	PANEL_COLOR_WEAKNESS_ENABLE,
	PANEL_COLOR_WEAKNESS_DISABLE,
	PANEL_ESD_RECOVERY_1,
	PANEL_ESD_RECOVERY_2,
	PANEL_MCD_ON,
	PANEL_MCD_OFF,
	PANEL_GRADUAL_ACL,
	PANEL_HW_CURSOR,
	PANEL_MULTIRES_FHD_TO_WQHD,
	PANEL_MULTIRES_HD_TO_WQHD,
	PANEL_MULTIRES_FHD,
	PANEL_MULTIRES_HD,
	PANEL_COVER_CONTROL_ENABLE,
	PANEL_COVER_CONTROL_DISABLE,
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

#define LCD_FLIP_NOT_REFRESH	BIT(8)

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

struct candella_lux_map {
	int *lux_tab;
	int *cmd_idx;
	int lux_tab_size;
	int from[257];
	int end[257];
	int bkl[257];
};

struct hbm_candella_lux_map {
	int *lux_tab;
	int *cmd_idx;
	int lux_tab_size;
	int *from;
	int *end;
	int *auto_level;
	int hbm_min_lv;
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
	u32  samsung_dsi_off_lp11_delay;

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
	struct dsi_panel_cmds display_on_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds display_off_tx_cmds[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds level1_key_enable_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds level1_key_disable_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds level2_key_enable_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds level2_key_disable_tx_cmds[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds esd_recovery_1_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds esd_recovery_2_tx_cmds[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds smart_dimming_mtp_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds manufacture_read_pre_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds manufacture_id_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds manufacture_id0_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds manufacture_id1_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds manufacture_id2_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds manufacture_date_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds ddi_id_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds cell_id_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds octa_id_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds rddpm_rx_cmds[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds mtp_read_sysfs_rx_cmds[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds read_vdd_ref_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds write_vdd_offset_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds read_vddm_ref_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds write_vddm_offset_cmds[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds vint_tx_cmds[SUPPORT_PANEL_REVISION];
	struct cmd_map vint_map_table[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds acl_off_tx_cmds[SUPPORT_PANEL_REVISION];

	struct cmd_map acl_map_table[SUPPORT_PANEL_REVISION];
	struct candella_lux_map candela_map_table[SUPPORT_PANEL_REVISION];

	struct hbm_candella_lux_map hbm_candela_map_table[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds acl_pre_percent_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds acl_percent_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds acl_on_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds gamma_tx_cmds[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds elvss_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds elvss_pre_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds elvss_tx_cmds[SUPPORT_PANEL_REVISION];
	struct cmd_map elvss_map_table[SUPPORT_PANEL_REVISION];

	struct cmd_map aid_map_table[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds aid_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds aid_subdivision_tx_cmds[SUPPORT_PANEL_REVISION];

	/* CONFIG_HBM_RE */
	struct dsi_panel_cmds hbm_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hbm2_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hbm_gamma_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hbm_etc_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hbm_etc2_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hbm_off_tx_cmds[SUPPORT_PANEL_REVISION];

	/* CONFIG_TCON_MDNIE_LITE */
	struct dsi_panel_cmds mdnie_read_rx_cmds[SUPPORT_PANEL_REVISION];

	/* CONFIG_DEBUG_LDI_STATUS */
	struct dsi_panel_cmds ldi_debug0_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds ldi_debug1_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds ldi_debug2_rx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds ldi_loading_det_rx_cmds[SUPPORT_PANEL_REVISION];

	/* CONFIG_TEMPERATURE_ELVSS */
	struct dsi_panel_cmds elvss_lowtemp_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds elvss_lowtemp2_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds smart_acl_elvss_lowtemp_tx_cmds[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds smart_acl_elvss_tx_cmds[SUPPORT_PANEL_REVISION];
	struct cmd_map smart_acl_elvss_map_table[SUPPORT_PANEL_REVISION];

	/* CONFIG_CAPS */
	struct dsi_panel_cmds pre_caps_setting_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds caps_setting_tx_cmds[SUPPORT_PANEL_REVISION];
	struct cmd_map caps_map_table[SUPPORT_PANEL_REVISION];

	/* PARTIAL_UPDATE */
	struct dsi_panel_cmds partial_display_on_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds partial_display_off_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds partial_display_column_row_tx_cmds[SUPPORT_PANEL_REVISION];

	/* Panel LPM(ALPM/HLPM) MODE */
	struct dsi_panel_cmds alpm_on_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds alpm_off_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_ctrl_alpm_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_ctrl_hlpm_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_ctrl_alpm_2nit_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_ctrl_alpm_40nit_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_ctrl_alpm_60nit_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_ctrl_hlpm_2nit_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_ctrl_hlpm_40nit_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_ctrl_hlpm_60nit_tx_cmds[SUPPORT_PANEL_REVISION];

	/* Panel LPM(ALPM/HLPM) Brightness Command */
	struct dsi_panel_cmds lpm_2nit_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_40nit_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_60nit_tx_cmds[SUPPORT_PANEL_REVISION];

	/* Panel LPM(ALPM/HLPM) Hz Control command */
	struct dsi_panel_cmds lpm_1hz_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_2hz_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_hz_none_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_aod_on_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds lpm_aod_off_tx_cmds[SUPPORT_PANEL_REVISION];

	/* CONFIG FPS CHANGE */
	struct dsi_panel_cmds ldi_fps_change_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds ldi_fps_rx_cmds[SUPPORT_PANEL_REVISION];

	/* TFT PWM CONTROL */
	struct dsi_panel_cmds tft_pwm_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds blic_dimming_cmds[SUPPORT_PANEL_REVISION];
	struct candella_lux_map scaled_level_map_table[SUPPORT_PANEL_REVISION];

	/* Command for nv read */
	struct dsi_panel_cmds packet_size_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds reg_read_pos_tx_cmds[SUPPORT_PANEL_REVISION];

	/* CONFIG_LCD_HMT */
	struct dsi_panel_cmds hmt_gamma_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hmt_elvss_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hmt_vint_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hmt_enable_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hmt_disable_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hmt_reverse_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hmt_forward_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hmt_aid_tx_cmds[SUPPORT_PANEL_REVISION];
	struct cmd_map hmt_reverse_aid_map_table[SUPPORT_PANEL_REVISION];
	struct candella_lux_map hmt_candela_map_table[SUPPORT_PANEL_REVISION];

	struct dsi_panel_cmds hsync_on_tx_cmds[SUPPORT_PANEL_REVISION];
	/* OSC TE Fitting */
	struct dsi_panel_cmds osc_te_fitting_tx_cmds[SUPPORT_PANEL_REVISION];

	/* AVC seq. */
	struct dsi_panel_cmds avc_on_tx_cmds[SUPPORT_PANEL_REVISION];

	/* TFT CABC CONTROL */
	struct dsi_panel_cmds cabc_on_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds cabc_off_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds cabc_on_duty_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds cabc_off_duty_tx_cmds[SUPPORT_PANEL_REVISION];

	/* CCB */
	struct dsi_panel_cmds ccb_on_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds ccb_off_tx_cmds[SUPPORT_PANEL_REVISION];

	/* IRC */
	struct dsi_panel_cmds irc_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds irc_subdivision_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds irc_off_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds hbm_irc_tx_cmds[SUPPORT_PANEL_REVISION];

	/* MCD */
	struct dsi_panel_cmds mcd_on_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds mcd_off_tx_cmds[SUPPORT_PANEL_REVISION];

	/* GRADUAL_ACL*/
	struct dsi_panel_cmds gradual_acl_tx_cmds[SUPPORT_PANEL_REVISION];

	/* H/W Cursor */
	struct dsi_panel_cmds hw_cursor_tx_cmds[SUPPORT_PANEL_REVISION];

	/* MULTI_RESOLUTION */
	struct dsi_panel_cmds panel_multires_fhd_to_wqhd[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds panel_multires_hd_to_wqhd[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds panel_multires_fhd[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds panel_multires_hd[SUPPORT_PANEL_REVISION];

	/* COVER CONTROL */
	struct dsi_panel_cmds panel_cover_control_enable_tx_cmds[SUPPORT_PANEL_REVISION];
	struct dsi_panel_cmds panel_cover_control_disable_tx_cmds[SUPPORT_PANEL_REVISION];

	/* TFT LCD Features*/
	int tft_common_support;
	int backlight_gpio_config;
	int pwm_ap_support;
	const char *tft_module_name;
	const char *panel_vendor;

	/* MDINE HBM_CE_TEXT_MDNIE mode used */
	int hbm_ce_text_mode_support;

	/* Backlight IC discharge delay */
	int blic_discharging_delay_tft;
	int cabc_delay;

	/* SPI I/F enable */
	struct dsi_panel_cmds spi_enable_tx_cmds[SUPPORT_PANEL_REVISION];
};

struct samsung_brightenss_data {
	/* Brightness packet set */
	struct dsi_cmd_desc brightness_packet_dsi[BRIGHTNESS_MAX_PACKET];
	struct dsi_panel_cmds brightness_packet_tx_cmds_dsi;
};

struct samsung_mdnie_tune_data {
	/* Brightness packet set */
	struct dsi_panel_cmds mdnie_tune_packet_tx_cmds_dsi;
};


struct display_status {
	int wait_disp_on;

	int hbm_mode;

	int elvss_value1;
	int elvss_value2;
	int disp_on_pre;
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
	int (*samsung_panel_revision)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_manufacture_date_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_ddi_id_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_cell_id_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_octa_id_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_hbm_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_elvss_read)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*samsung_mdnie_read)(struct mdss_dsi_ctrl_pdata *ctrl);
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
	struct dsi_panel_cmds *(*samsung_brightness_pre_caps)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
	struct dsi_panel_cmds *(*samsung_brightness_caps)(struct mdss_dsi_ctrl_pdata *ctrl, int *level_key);
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

	/*SPI INTERFACE*/
	char* (*samsung_spi_read_reg)(void);

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

	/* COVER CONTROL */
	void (*samsung_cover_control)(struct mdss_dsi_ctrl_pdata *ctrl, struct samsung_display_driver_data *vdd);

	/* mDNIe*/
	void (*samsung_mdnie_init)(struct samsung_display_driver_data *vdd);
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
	struct dentry *display_tuning;

	bool print_cmds;
	bool *is_factory_mode;
	bool panic_on_pptimeout;
	bool pwm_tuning;
};

struct samsung_display_driver_data {
	/*
	*	PANEL COMMON DATA
	*/
	struct mutex vdd_lock;
	struct mutex vdd_blank_unblank_lock;
	struct mutex vdd_panel_lpm_lock;
	struct samsung_display_debug_data *debug_data;

	int vdd_blank_mode[SUPPORT_PANEL_COUNT];

	int support_panel_max;
	int support_mdnie_lite;

	int support_mdnie_trans_dimming;

	bool is_factory_mode;

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
	int candela_level;
	int cmd_idx;

	int acl_status;
	int siop_status;
	bool mdnie_tuning_enable_tft;
	int mdnie_tune_size1;
	int mdnie_tune_size2;
	int mdnie_tune_size3;
	int mdnie_tune_size4;
	int mdnie_tune_size5;
	int mdnie_tune_size6;
	int mdnie_reverse_scr;
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
	int smart_dimming_loaded_dsi[SUPPORT_PANEL_COUNT];
	struct smartdim_conf *smart_dimming_dsi[SUPPORT_PANEL_COUNT];

	/*
	*	Brightness control packet
	*/
	struct samsung_brightenss_data brightness[SUPPORT_PANEL_COUNT];

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
	bool hmt_enabled;
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

	/* AID subdivision */
	int aid_subdivision_enable;

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
	bool panel_lpm_enable;
	bool lpm_power_control;
	char lpm_power_control_supply_name[32];
	int lpm_power_control_supply_min_voltage;
	int lpm_power_control_supply_max_voltage;

	/* hall ic */
	bool support_hall_ic;
	bool hall_ic_status;
	bool hall_ic_status_pending;
	bool hall_ic_status_unhandled;
	bool hall_ic_mode_change_trigger;
	struct mutex vdd_hall_ic_blank_unblank_lock;
	struct mutex vdd_hall_ic_lock;
	struct notifier_block hall_ic_notifier_display;
	bool lcd_flip_not_refresh;
	u32 lcd_flip_delay_ms;
	struct delayed_work delay_disp_on_work;

	/* Reset Time check */
	s64 reset_time_64;

	/* Sleep_out cmd time check */
	s64 sleep_out_time_64;

	/* boosting at device resume */
	int bootsing_at_resume;
	
#ifdef CONFIG_DISPLAY_USE_INFO
	struct notifier_block dpui_notif;
#endif

};

/*SPI INTERFACE*/
struct ss_spi_private;

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
int mdss_samsung_send_cmd(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_cmd_list cmd);
void mdss_samsung_panel_parse_dt(struct device_node *np, struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int mdss_samsung_panel_on_pre(struct mdss_panel_data *pdata);
int mdss_samsung_panel_on_post(struct mdss_panel_data *pdata);
int mdss_samsung_panel_off_pre(struct mdss_panel_data *pdata);
int mdss_samsung_panel_off_post(struct mdss_panel_data *pdata);
int mdss_samsung_panel_extra_power(struct mdss_panel_data *pdata, int enable);
int mdss_backlight_tft_gpio_config(struct mdss_panel_data *pdata, int enable);
int mdss_backlight_tft_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl);
void mdss_tft_autobrightness_cabc_update(struct mdss_dsi_ctrl_pdata *ctrl);
void mdss_samsung_panel_data_read(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_panel_cmds *cmds, char *buffer, int level_key);
void mdss_samsung_cabc_update(void);
void mdss_samsung_panel_low_power_config(struct mdss_panel_data *pdata, int enable);

/*
 * Check lcd attached status for DISPLAY_1 or DISPLAY_2
 * if the lcd was not attached, the function will return 0
 */
int mdss_panel_attached(int ndx);

int get_lcd_attached(char *mode);
int get_lcd_attached_secondary(char *mode);
int mdss_panel_attached(int ndx);

struct samsung_display_driver_data *check_valid_ctrl(struct mdss_dsi_ctrl_pdata *ctrl);

char mdss_panel_id0_get(struct mdss_dsi_ctrl_pdata *ctrl);
char mdss_panel_id1_get(struct mdss_dsi_ctrl_pdata *ctrl);
char mdss_panel_id2_get(struct mdss_dsi_ctrl_pdata *ctrl);
char mdss_panel_rev_get(struct mdss_dsi_ctrl_pdata *ctrl);

int mdss_panel_attach_get(struct mdss_dsi_ctrl_pdata *ctrl);
int mdss_panel_attach_set(struct mdss_dsi_ctrl_pdata *ctrl, int status);

void mdss_samsung_dump_regs(void);
void mdss_samsung_dsi_dump_regs(int dsi_num);
void mdss_mdp_underrun_dump_info(void);
int mdss_samsung_read_rddpm(void);
int mdss_samsung_read_loading_detection(void);
int mdss_samsung_dsi_te_check(void);
/* void mdss_samsung_fence_dump(char *interface, struct sync_fence *fence); */
void mdss_samsung_check_hw_config(struct platform_device *pdev);
void mdss_samsung_dsi_force_hs(struct mdss_panel_data *pdata);
void mdss_samsung_dsi_video_engine_enable(struct mdss_panel_data *pdata);

/* BRIGHTNESS RELATED FUNCTION */
int get_cmd_index(struct samsung_display_driver_data *vdd, int ndx);
int get_candela_value(struct samsung_display_driver_data *vdd, int ndx);
int mdss_samsung_brightness_dcs(struct mdss_dsi_ctrl_pdata *ctrl, int level);
void mdss_samsung_brightness_tft_pwm(struct mdss_dsi_ctrl_pdata *ctrl, int level);
/* TFT BL DCS RELATED FUNCTION */
int get_scaled_level(struct samsung_display_driver_data *vdd, int ndx);

/* SYSFS RELATED FUNCTION */
int mdss_samsung_create_sysfs(void *data);

/* EXTERN FUNCTION */
extern void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
		struct dsi_panel_cmds *pcmds, u32 flags);
#ifdef CONFIG_FOLDER_HALL
extern void hall_ic_register_notify(struct notifier_block *nb);
#endif

/* EXTERN VARIABLE */
extern struct dsi_status_data *pstatus_data;

/* HMT FUNCTION */
int hmt_bright_update(struct mdss_dsi_ctrl_pdata *ctrl);
int hmt_enable(struct mdss_dsi_ctrl_pdata *ctrl, struct samsung_display_driver_data *vdd);
int hmt_reverse_update(struct mdss_dsi_ctrl_pdata *ctrl, int enable);

/* HALL IC FUNCTION */
int display_ndx_check(struct mdss_dsi_ctrl_pdata *ctrl);
int samsung_display_hall_ic_status(struct notifier_block *nb,
		unsigned long hall_ic, void *data);

/* For Image_Dump */
void samsung_image_dump_worker(struct work_struct *work);
void samsung_mdss_image_dump(void);

/* PANEL LPM FUNCTION */
int mdss_init_panel_lpm_reg_offset(struct mdss_dsi_ctrl_pdata *ctrl,
		int (*reg_list)[2],
		struct dsi_panel_cmds *cmd_list[], int list_size);

/* Other line panel support */
#define MAX_READ_LINE_SIZE 256
int read_line(char *src, char *buf, int *pos, int len);
void read_panel_data_work_fn(struct delayed_work *work);

/* check irq event */
void mdss_samsung_resume_event(int irq);

#define BF_TYPE 0x4D42             /* "MB" */

struct BITMAPFILEHEADER                 /**** BMP file header structure ****/
{
	unsigned short bfType;           /* Magic number for file */
	unsigned int   bfSize;           /* Size of file */
	unsigned short bfReserved1;      /* Reserved */
	unsigned short bfReserved2;      /* ... */
	unsigned int   bfOffBits;        /* Offset to bitmap data */
	unsigned int   biSize;           /* Size of info header */
	int            biWidth;          /* Width of image */
	int            biHeight;         /* Height of image */
	unsigned short biPlanes;         /* Number of color planes */
	unsigned short biBitCount;       /* Number of bits per pixel */
	unsigned int   biCompression;    /* Type of compression to use */
	unsigned int   biSizeImage;      /* Size of image data */
	int            biXPelsPerMeter;  /* X pixels per meter */
	int            biYPelsPerMeter;  /* Y pixels per meter */
	unsigned int   biClrUsed;        /* Number of colors used */
	unsigned int   biClrImportant;   /* Number of important colors */
} __packed;
extern struct kset *devices_kset;

/* SAMSUNG COMMON HEADER*/
#include "ss_dsi_smart_dimming_common.h"
/* MDNIE_LITE_COMMON_HEADER */
#include "ss_dsi_mdnie_lite_common.h"

/* PANEL_HEADER END	*/

#endif
