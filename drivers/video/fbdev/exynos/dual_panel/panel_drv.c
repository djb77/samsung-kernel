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

#include <linux/err.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_reserved_mem.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <video/mipi_display.h>
#include "../dual_dpu/decon.h"
#include "panel.h"
#include "panel_drv.h"
#include "dpui.h"
#include "mdnie.h"
#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
#include "spi.h"
#endif
#ifdef CONFIG_ACTIVE_CLOCK
#include "active_clock.h"
#endif

static int boot_panel_id = 0;
int panel_log_level = 6;
#ifdef CONFIG_SUPPORT_PANEL_SWAP
static int connect_panel = PANEL_CONNECT;
#endif
static int panel_prepare(struct panel_device *panel,
		struct common_panel_info *info);
static struct common_panel_info *panel_detect(struct panel_device *panel);

#ifdef CONFIG_SUPPORT_DOZE
#define CONFIG_SET_1p5_ALPM
#define BUCK_ALPM_VOLT		1500000
#define BUCK_NORMAL_VOLT	1600000
#endif
/**
 * get_lcd info - get lcd global information.
 * @arg: key string of lcd information
 *
 * if get lcd info successfully, return 0 or positive value.
 * if not, return -EINVAL.
 */
int get_lcd_info(char *arg)
{
	if (!arg) {
		panel_err("%s invalid arg\n", __func__);
		return -EINVAL;
	}

	if (!strncmp(arg, "connected", 9))
		return (boot_panel_id < 0) ? 0 : 1;
	else if (!strncmp(arg, "id", 2))
		return (boot_panel_id < 0) ? 0 : boot_panel_id;
	else if (!strncmp(arg, "window_color", 12))
		return (boot_panel_id < 0) ? 0 : ((boot_panel_id & 0x0F0000) >> 16);
	else
		return -EINVAL;
}

EXPORT_SYMBOL(get_lcd_info);

void clear_disp_det_pend(struct panel_device *panel)
{
	int pend_disp_det;
	struct panel_pad *pad = &panel->pad;

	if (pad->pend_reg_disp_det != NULL) {
		pend_disp_det = readl(pad->pend_reg_disp_det);
		pend_disp_det = pend_disp_det & ~(pad->pend_bit_disp_det);
		writel(pend_disp_det, pad->pend_reg_disp_det);
	}

	return;
}

static int __set_panel_power(struct panel_device *panel, int power)
{
	int i;
	int ret = 0;
	struct panel_pad *pad = &panel->pad;

	if (panel->state.power == power) {
		panel_warn("PANEL:WARN:%s:same status.. skip..\n", __func__);
		goto set_err;
	}

	if (pad == NULL) {
		panel_err("PANEL:ERR:%s:pad is null\n", __func__);
		goto set_err;
	}

	if (power == PANEL_POWER_ON) {
		for (i = 0; i < REGULATOR_MAX; i++) {
			if (pad->regulator[i] != NULL) {
				ret = regulator_enable(pad->regulator[i]);
				if (ret) {
					panel_err("PANEL:%s:faield to enable regulator : %d :%d\n",
						__func__, i, ret);
					goto set_err;
				}
			}
		}
		usleep_range(10000, 10000);
	} else {
		gpio_set_value(pad->gpio_reset, 0);
		usleep_range(1000, 1500);
		for (i = REGULATOR_MAX - 1; i >= 0; i--) {
			ret = regulator_disable(pad->regulator[i]);
			if (ret) {
				panel_err("PANEL:%s:faield to disable regulator : %d :%d\n",
					__func__, i, ret);
				goto set_err;
			}
		}
	}
	panel->state.power = power;
set_err:
	return ret;
}

static int __set_panel_reset(struct panel_device *panel)
{
	struct panel_pad *pad = &panel->pad;
	int ret = 0;

	if (pad == NULL) {
		panel_err("PANEL:ERR:%s:pad is null\n", __func__);
		goto set_err;
	}
	pr_info("%s %d\n", __func__, panel->id);

	if (panel->state.power == PANEL_POWER_ON) {
		gpio_set_value(pad->gpio_reset, 1);
		usleep_range(5000, 5100);
		gpio_set_value(pad->gpio_reset, 0);
		usleep_range(5000, 5100);
		gpio_set_value(pad->gpio_reset, 1);
		usleep_range(5000, 5100);
	} else {
		panel_warn("PANEL:WARN:%s: power off state.. skip..\n", __func__);
		goto set_err;
	}

set_err:
	return ret;
}

int __panel_seq_display_on(struct panel_device *panel)
{
	int ret = 0;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DISPLAY_ON_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}

	return ret;
}


int __panel_seq_display_off(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DISPLAY_OFF_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}

	return ret;
}

#define PANEL_DISP_DET_HIGH 	1
#define PANEL_DISP_DET_LOW		0

static int panel_sleep_out(struct panel_device *panel);


static int __panel_seq_res_init(struct panel_device *panel)
{
	int ret;

	if (panel->state.cur_state!= PANEL_STATE_NORMAL) {
		ret = panel_sleep_out(panel);
		if (ret) {
			dsim_err("DSIM:ERR:%s:failed to panel on\n", __func__);
		}
	}

	ret = panel_do_seqtbl_by_index(panel, PANEL_RES_INIT_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write resource init seqtbl\n",
				__func__);
	}

	return ret;
}

static int __panel_seq_init(struct panel_device *panel)
{
	int ret = 0;
	int retry = 200;
	int disp_det = panel->pad.gpio_disp_det;
#if 0
#ifdef CONFIG_SUPPORT_PANEL_SWAP
	struct common_panel_info *info;
#endif
#endif
	if ((disp_det != 0) &&
		(gpio_get_value(disp_det) == PANEL_DISP_DET_HIGH)) {
		panel_warn("PANEL:WARN:%s:disp det already set to 1\n",
			__func__);
		return 0;
	}
#if 0
#ifdef CONFIG_SUPPORT_PANEL_SWAP
	info = panel_detect(panel);
	if (unlikely(!info)) {
		panel_err("PANEL:ERR:%s:panel detection failed\n", __func__);
		goto do_exit;
	}

	ret = panel_prepare(panel, info);
	if (unlikely(ret)) {
		panel_err("PANEL:ERR:%s, failed to panel_prepare\n", __func__);
		goto do_exit;
	}
#endif
#endif
	ret = panel_do_seqtbl_by_index(panel, PANEL_INIT_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}

check_disp_det:
	if (disp_det == 0) {
		panel_info("PNAEL:INFO:%s:does not support disp det\n", __func__);
		return 0;
	}
	if (gpio_get_value(disp_det) == PANEL_DISP_DET_LOW) {
		usleep_range(100, 100);
		if (retry--)
			goto check_disp_det;
		goto do_exit;
	}

	clear_disp_det_pend(panel);

	enable_irq(panel->pad.irq_disp_det);

	panel_info("PANEL:INFO:%s:check disp det .. success\n", __func__);
	return 0;

do_exit:
	panel_err("PANEL:ERR:%s:failed to panel power on\n", __func__);
#if 0
#ifdef CONFIG_SUPPORT_PANEL_SWAP
	panel->state.connect_panel = PANEL_DISCONNECT;
	return 0;
#else
	return -EAGAIN;
#endif
#endif
	return 0;

}

static int __panel_seq_exit(struct panel_device *panel)
{
	int ret;

	disable_irq(panel->pad.irq_disp_det);

	ret = panel_do_seqtbl_by_index(panel, PANEL_EXIT_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write exit seqtbl\n", __func__);
	}

	return ret;
}

#ifdef CONFIG_SUPPORT_HMD
static int __panel_seq_hmd_on(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state;

	if (!panel) {
		panel_err("PANEL:ERR:%s:pane is null\n", __func__);
		return 0;
	}
	state = &panel->state;

	panel_info("PANEK:INFO:%s:hmd was on, setting hmd on seq\n", __func__);
	ret = panel_do_seqtbl_by_index(panel, PANEL_HMD_ON_SEQ);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to set hmd on seq\n",
			__func__);
	}

	return ret;
}
#endif
#ifdef CONFIG_SUPPORT_DOZE
static int __panel_seq_exit_alpm(struct panel_device *panel)
{
	int ret = 0;
	struct backlight_device *bd = panel->panel_bl.bd;
#ifdef CONFIG_SET_1p5_ALPM
	int volt;
	struct panel_pad *pad = &panel->pad;
#endif
	panel_info("PANEL:INFO:%s:was called\n", __func__);
#ifdef CONFIG_SET_1p5_ALPM
	volt = regulator_get_voltage(pad->regulator[REGULATOR_1p6V]);
	panel_info("PANEL:INFO:%s:regulator voltage  : %d\n", __func__, volt);
	if (volt != BUCK_NORMAL_VOLT) {
		ret = regulator_set_voltage(pad->regulator[REGULATOR_1p6V],
			BUCK_NORMAL_VOLT, BUCK_NORMAL_VOLT);
		if (ret)
			panel_warn("PANEL:WARN:%s:failed to set volt to 1.6\n",
				__func__);
	}
#endif
	ret = panel_do_seqtbl_by_index(panel, PANEL_ALPM_EXIT_SEQ);
	if (ret) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n",
			__func__);
	}
	backlight_update_status(bd);
	return ret;
}


/* delay to prevent current leackage when alpm */
/* according to ha6 opmanual, the dealy value is 126msec */
static void __delay_normal_alpm(struct panel_device *panel)
{
	u32 gap;
	u32 delay = 0;
	struct seqinfo *seqtbl;
	struct delayinfo *delaycmd;

	seqtbl = find_index_seqtbl(&panel->panel_data, PANEL_ALPM_DELAY_SEQ);
	if (unlikely(!seqtbl))
		goto exit_delay;

	delaycmd = (struct delayinfo *)seqtbl->cmdtbl[0];
	if (delaycmd->type != CMD_TYPE_DELAY) {
		panel_err("PANEL:ERR:%s:can't find value\n", __func__);
		goto exit_delay;
	}

	if (ktime_after(ktime_get(), panel->ktime_panel_on)) {
		gap = ktime_to_us(ktime_sub(ktime_get(), panel->ktime_panel_on));
		if (gap > delaycmd->usec)
			goto exit_delay;

		delay = delaycmd->usec - gap;
		usleep_range(delay, delay);
	}
	panel_info("PANEL:INFO:%stotal elapsed time : %d\n", __func__,
		(int)ktime_to_us(ktime_sub(ktime_get(), panel->ktime_panel_on)));
exit_delay:
	return;
}

static int __panel_seq_set_alpm(struct panel_device *panel)
{
	int ret = 0;
	int volt = 0;
#ifdef CONFIG_SET_1p5_ALPM
	struct panel_pad *pad = &panel->pad;
#endif
	panel_info("PANEL:INFO:%s:was called\n", __func__);

	__delay_normal_alpm(panel);

	ret = panel_do_seqtbl_by_index(panel, PANEL_ALPM_ENTER_SEQ);
	if (ret) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n",
			__func__);
	}
#ifdef CONFIG_SET_1p5_ALPM
	volt = regulator_get_voltage(pad->regulator[REGULATOR_1p6V]);
	panel_info("PANEL:INFO:%s:regulator voltage : %d\n", __func__, volt);
	if (volt != BUCK_ALPM_VOLT) {
		ret = regulator_set_voltage(pad->regulator[REGULATOR_1p6V],
			BUCK_ALPM_VOLT, BUCK_ALPM_VOLT);
		if (ret)
			panel_warn("PANEL:WARN:%s:failed to set volt to 1.5\n",
				__func__);
	}
#endif
	return ret;
}
#endif

#ifdef CONFIG_ACTIVE_CLOCK
static int __panel_seq_active_clock(struct panel_device *panel, int send_img)
{
	int ret = 0;
	struct act_clock_dev *act_dev = &panel->act_clk_dev;
	struct act_clk_info *act_info = &act_dev->act_info;
	struct act_blink_info *blink_info = &act_dev->blink_info;

	if (act_info->en) {
		panel_dbg("PANEL:DBG:%s:active clock was enabed\n", __func__);
		if (send_img) {
			panel_dbg("PANEL:DBG:%s:send active clock's image\n", __func__);

			if (act_info->update_img == IMG_UPDATE_NEED) {
				ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_IMG_SEQ);
				if (unlikely(ret < 0)) {
					panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
				}
				act_info->update_img = IMG_UPDATE_DONE;
			}
		}
		usleep_range(5, 5);

		ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_CTRL_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
		}
	}

	if (blink_info->en) {
		panel_dbg("PANEL:DBG:%s:active blink was enabed\n", __func__);

		ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_CTRL_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
		}
	}
	return ret;
}
#endif

static int __panel_seq_dump(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DUMP_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write dump seqtbl\n", __func__);
	}

	return ret;
}

static int panel_debug_dump(struct panel_device *panel)
{
	int ret, disp_det;
	struct panel_pad *pad = &panel->pad;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		goto dump_exit;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_info("PANEL:INFO:Current state:%d\n", panel->state.cur_state);
		goto dump_exit;
	}

	ret = __panel_seq_dump(panel);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to dump\n", __func__);
		return ret;
	}

	disp_det = gpio_get_value(pad->gpio_disp_det);
	panel_info("PANEL:INFO:%s: disp gpio : %d\n", __func__, disp_det);
dump_exit:
	return 0;
}

static int panel_display_on(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WANR:%s:panel disconnected\n", __func__);
		goto do_exit;
	}

	if (state->cur_state == PANEL_STATE_OFF) {
		panel_warn("PANEL:WANR:%s:panel off\n", __func__);
		goto do_exit;
	}

	ret = __panel_seq_display_on(panel);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to display on\n", __func__);
		return ret;
	}

	state->disp_on = PANEL_DISPLAY_ON;
	copr_enable(&panel->copr);
	mdnie_enable(&panel->mdnie);

	return 0;

do_exit:
	return ret;
}

static int panel_display_off(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WANR:%s:panel disconnected\n", __func__);
		goto do_exit;
	}

	if (state->cur_state == PANEL_STATE_OFF) {
		panel_warn("PANEL:WANR:%s:panel off\n", __func__);
		goto do_exit;
	}

	ret = __panel_seq_display_off(panel);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n",
			__func__);
	}
	state->disp_on = PANEL_DISPLAY_OFF;

	return 0;
do_exit:
	return ret;
}

static struct common_panel_info *panel_detect(struct panel_device *panel)
{
	u8 id[3];
	u32 panel_id;
	int ret = 0;
	struct common_panel_info *info;
	struct panel_info *panel_data;

	if (panel == NULL) {
		panel_err("%s, panel is null\n", __func__);
		return NULL;
	}
	panel_data = &panel->panel_data;

	if (!panel_data->ldi_name) {
		panel_err("%s, panel ldi_name is empty\n", __func__);
		return NULL;
	}

	memset(id, 0, sizeof(id));
	if (panel->state.power == PANEL_POWER_ON) {
	 	ret = read_panel_id(panel, id);
		if (unlikely(ret < 0)) {
			panel_err("%s, can't not detect panel\n", __func__);
			memset(id, 0, sizeof(id));
			panel->state.connect_panel = PANEL_DISCONNECT;
			//return NULL;
		}
	}

	panel_id = (id[0] << 16) | (id[1] << 8) | id[2];
	memcpy(panel_data->id, id, sizeof(id));

	panel_info("%s, ldi_name %s, panel_id 0x%08X\n",
			__func__, panel_data->ldi_name, panel_id);

	info = find_panel(panel, panel_id);
	if (unlikely(!info)) {
		panel_err("%s, panel not found (name %s id 0x%08X)\n",
				__func__, panel_data->ldi_name, panel_id);
		return NULL;
	}

	return info;
}

static int panel_prepare(struct panel_device *panel, struct common_panel_info *info)
{
	int i;
	struct panel_info *panel_data = &panel->panel_data;

	mutex_lock(&panel->op_lock);
	panel_data->maptbl = info->maptbl;
	panel_data->nr_maptbl = info->nr_maptbl;
	panel_data->seqtbl = info->seqtbl;
	panel_data->nr_seqtbl = info->nr_seqtbl;
	panel_data->rditbl = info->rditbl;
	panel_data->nr_rditbl = info->nr_rditbl;
	panel_data->restbl = info->restbl;
	panel_data->nr_restbl = info->nr_restbl;
	panel_data->dumpinfo = info->dumpinfo;
	panel_data->nr_dumpinfo = info->nr_dumpinfo;
	panel_data->panel_dim_info[0] = info->panel_dim_info[0];
	panel_data->panel_dim_info[1] = info->panel_dim_info[1];
	for (i = 0; i < panel_data->nr_maptbl; i++)
		panel_data->maptbl[i].pdata = panel;
	for (i = 0; i < panel_data->nr_restbl; i++)
		panel_data->restbl[i].state = RES_UNINITIALIZED;
	mutex_unlock(&panel->op_lock);

	__panel_seq_res_init(panel);
	print_panel_resource(panel);

	mutex_lock(&panel->op_lock);
	for (i = 0; i < panel_data->nr_maptbl; i++)
		maptbl_init(&panel_data->maptbl[i]);
	mutex_unlock(&panel->op_lock);

	return 0;
}

int panel_probe(struct panel_device *panel)
{
	int i, ret = 0;
	struct decon_lcd *lcd_info;
	struct panel_info *panel_data;
	struct common_panel_info *info;

	panel_dbg("%s was callled\n", __func__);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	lcd_info = &panel->lcd_info;
	panel_data = &panel->panel_data;
	panel_data->ldi_name = lcd_info->ldi_name;

	if ((panel->id != 0 ) &&
		(lcd_info->op_mode == DSI_SDDD_MODE)) {
		panel_info("PANEL:INFO:%s:skip probe\n", __func__);
		return 0;
	}

	info = panel_detect(panel);
	if (unlikely(!info)) {
		panel_err("PANEL:ERR:%s:panel detection failed\n", __func__);
		return -ENODEV;
	}

#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
	/* TODO : move to parse dt function
	   1. get panel_spi device node.
	   2. get spi_device of node
	 */
	panel->spi = of_find_panel_spi_by_node(NULL);
	if (!panel->spi)
		panel_warn("%s, panel spi device unsupported\n", __func__);
#endif

	mutex_lock(&panel->op_lock);
	panel_data->props.temperature = NORMAL_TEMPERATURE;
	panel_data->props.alpm_mode = ALPM_OFF;
	panel_data->props.cur_alpm_mode = ALPM_OFF;
	for (i = 0; i < MAX_CMD_LEVEL; i++)
		panel_data->props.key[i] = 0;
	mutex_unlock(&panel->op_lock);

	mutex_lock(&panel->panel_bl.lock);
	panel_data->props.adaptive_control = ACL_OPR_MAX - 1;
#ifdef CONFIG_SUPPORT_XTALK_MODE
	panel_data->props.xtalk_mode = XTALK_OFF;
#endif
	mutex_unlock(&panel->panel_bl.lock);

	ret = panel_prepare(panel, info);
	if (unlikely(ret)) {
		pr_err("%s, failed to prepare common panel driver\n", __func__);
		return -ENODEV;
	}
	if (panel->id == 0)
		panel->lcd = lcd_device_register("panel", panel->dev, panel, NULL);
	else if (panel->id == 1)
		panel->lcd = lcd_device_register("panel_1", panel->dev, panel, NULL);
	else {
		panel_err("PANEL:ERR::%s: invalid panel id : %d\n", __func__, panel->id);
		return -EINVAL;
	}
	if (IS_ERR(panel->lcd)) {
		panel_err("%s, faield to register lcd device\n", __func__);
		return PTR_ERR(panel->lcd);
	}

	ret = panel_bl_probe(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe backlight driver\n", __func__);
		return -ENODEV;
	}

	ret = panel_sysfs_probe(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to init sysfs\n", __func__);
		return -ENODEV;
	}

	ret = mdnie_probe(panel, info->mdnie_tune);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe mdnie driver\n", __func__);
		return -ENODEV;
	}

#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	ret = copr_probe(panel, info->copr_data);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe copr driver\n", __func__);
		BUG();
		return -ENODEV;
	}
#endif

#ifdef CONFIG_ACTIVE_CLOCK
	ret = probe_live_clock_drv(&panel->act_clk_dev);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe copr driver\n", __func__);
		BUG();
		return -ENODEV;
	}
#endif
	return 0;
}

static int panel_sleep_in(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WANR:%s:panel disconnected\n", __func__);
		goto do_exit;
	}
	switch(state->cur_state) {
		case PANEL_STATE_ON:
			panel_warn("PANEL:WANR:%s:panel state is aready on : %d\n",
				__func__, state->cur_state);
			goto do_exit;
		case PANEL_STATE_NORMAL:
		case PANEL_STATE_ALPM:
			mdnie_disable(&panel->mdnie);
			ret = panel_display_off(panel);
			if (unlikely(ret < 0)) {
				panel_err("PANEL:ERR:%s, failed to write display_off seqtbl\n",
					__func__);
			}
			ret = __panel_seq_exit(panel);
			if (unlikely(ret < 0)) {
				panel_err("PANEL:ERR:%s, failed to write exit seqtbl\n",
					__func__);
			}
			copr_disable(&panel->copr);
			break;
		default:
			panel_err("PANEL:ERR:%s:invalid state : %d\n",
				__func__,state->cur_state);
			goto do_exit;
	}

	state->cur_state = PANEL_STATE_ON;
	return 0;

do_exit:
	return ret;
}
int power_skip;
static int panel_power_on(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WANR:%s:panel disconnected\n", __func__);
		goto do_exit;
	}

	if (state->cur_state == PANEL_STATE_OFF) {
		ret = __set_panel_power(panel, PANEL_POWER_ON);
		if (ret) {
			panel_err("PANEL:ERR:%s:failed to panel power on\n",
				__func__);
			goto do_exit;
		}
		state->cur_state = PANEL_STATE_ON;
		power_skip = 0;
	} else {
		power_skip = 1;
		panel_err("PANEL::%s:panel state on power skip\n",
					__func__);
	}
	return 0;

do_exit:
	return ret;
}
static int panel_set_reset(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WANR:%s:panel disconnected\n", __func__);
		goto do_exit;
	}
	if (power_skip) {
		panel_err("PANEL:%s:panel reset skip\n",
					__func__);
				goto do_exit;
	}
	ret = __set_panel_reset(panel);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to panel reset\n",
			__func__);
		goto do_exit;
	}
	return 0;

do_exit:
	return ret;
}


static int panel_power_off(struct panel_device *panel)
{
	int ret = -EINVAL;
	struct panel_state *state = &panel->state;
#ifdef CONFIG_ACTIVE_CLOCK
	struct act_clock_dev *act_clk;
#endif
#ifdef CONFIG_SUPPORT_PANEL_SWAP
	state->connect_panel = connect_panel;
#endif
	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WANR:%s:panel disconnected\n", __func__);
		goto do_exit;
	}

	switch (state->cur_state)  {
		case PANEL_STATE_OFF:
			panel_warn("PANEL:WANR:%s:panel state is already off: %d\n",
				__func__, state->cur_state);
			goto do_exit;
		case PANEL_STATE_ALPM:
		case PANEL_STATE_NORMAL:
			panel_sleep_in(panel);
		case PANEL_STATE_ON:
			ret = __set_panel_power(panel, PANEL_POWER_OFF);
			if (ret) {
				panel_err("PANEL:ERR:%s:failed to panel power off\n",
					__func__);
				goto do_exit;
			}
			break;
		default:
			panel_err("PANEL:ERR:%s:invalid state : %d\n",
				__func__, state->cur_state);
			goto do_exit;
	}

	state->cur_state = PANEL_STATE_OFF;
#ifdef CONFIG_ACTIVE_CLOCK
    /*clock image for active clock on ddi side ram was remove when ddi was turned off */
	act_clk = &panel->act_clk_dev;
	act_clk->act_info.update_img = IMG_UPDATE_NEED;
#endif
	return 0;

do_exit:
	return ret;

}

static int panel_sleep_out(struct panel_device *panel)
{
	int ret = 0, retry = 30;
	struct panel_state *state = &panel->state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WANR:%s:panel disconnected\n", __func__);
		goto do_exit;
	}

retry_sleep_out:
	switch (state->cur_state) {
		case PANEL_STATE_NORMAL:
			panel_warn("PANEL:WARN:%s:disp det already normal\n",
				__func__);
			goto do_exit;
		case PANEL_STATE_ALPM:
#ifdef CONFIG_SUPPORT_DOZE
			ret = __panel_seq_exit_alpm(panel);
			if (ret) {
				panel_err("PANEL:ERR:%s:failed to panel exit alpm\n",
					__func__);
				goto do_exit;
			}
#endif
			break;
		case PANEL_STATE_OFF:
			ret = panel_power_on(panel);
			if (ret) {
				panel_err("PANEL:ERR:%s:failed to power on\n", __func__);
				goto do_exit;
			}
		case PANEL_STATE_ON:
			ret = __panel_seq_init(panel);
			if (ret) {
				if (ret == -EAGAIN && retry--) {
					panel_power_off(panel);
					usleep_range(100000, 100000);
					goto retry_sleep_out;
				} else {
					panel_err("PANEL:ERR:%s:failed to panel init seq\n",
						__func__);
					goto do_exit;
				}
			}
			break;
		default:
			panel_warn("PANEL:WARN:%s:invalid cur state : %d\n",
				__func__, state->cur_state);
			goto do_exit;
	}
	state->cur_state = PANEL_STATE_NORMAL;
	panel->ktime_panel_on = ktime_get();
#ifdef CONFIG_SUPPORT_HMD
	if (state->hmd_on == PANEL_HMD_ON) {
		panel_info("PANEK:INFO:%s:hmd was on, setting hmd on seq\n", __func__);
		ret = __panel_seq_hmd_on(panel);
		if (ret) {
			panel_err("PANEL:ERR:%s:failed to set hmd on seq\n",
				__func__);
		}
	}
#endif
	return 0;

do_exit:
	return ret;
}

#ifdef CONFIG_SUPPORT_DOZE

static int panel_update_alpm(struct panel_device *panel)
{
	int ret = 0;
	struct panel_properties *props = &panel->panel_data.props;

	if (props->cur_alpm_mode != props->alpm_mode) {
		ret = __panel_seq_set_alpm(panel);
		if (ret) {
			panel_err("PANEL:ERR:%s, failed to write alpm\n",
				__func__);
		}
	}
	return ret;
}

static int panel_doze(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WANR:%s:panel disconnected\n", __func__);
		goto do_exit;
	}

	switch(state->cur_state) {
		case PANEL_STATE_ALPM:
			ret = panel_update_alpm(panel);
			if (ret) {
				panel_err("PANEL:ERR:%s:failed to update alpm mode\n",
					__func__);
				goto do_exit;
			}
			break;
		case PANEL_POWER_ON:
		case PANEL_POWER_OFF:
			ret = panel_sleep_out(panel);
			if (ret) {
				panel_err("PANEL:ERR:%s:failed to set normal\n", __func__);
				goto do_exit;
			}
		case PANEL_STATE_NORMAL:
#ifdef CONFIG_ACTIVE_CLOCK
			ret = __panel_seq_active_clock(panel, 1);
			if (ret) {
				panel_err("PANEL:ERR:%s, failed to write active seqtbl\n",
				__func__);
			}
#endif
			ret = __panel_seq_set_alpm(panel);
			if (ret) {
				panel_err("PANEL:ERR:%s, failed to write alpm\n",
					__func__);
			}
			state->cur_state = PANEL_STATE_ALPM;
			break;
		default:
			break;
	}
	panel_info("PANEL:INFO:%s:was called\n", __func__);

do_exit:
	return ret;
}

#endif //CONFIG_SUPPORT_DOZE




#ifdef CONFIG_SUPPORT_DSU
int panel_set_dsu(struct panel_device *panel, struct dsu_info *dsu)
{
	int ret = 0;
	int xres, yres;
	int actual_mode;
	struct panel_state *state;
	struct decon_lcd *lcd_info;
	struct dsu_info_dt *dt_dsu_info;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		ret = -EINVAL;
		goto do_exit;
	}
	if (unlikely(!dsu)) {
		panel_err("PANEL:ERR:%s:dsu is null\n", __func__);
		ret = -EINVAL;
		goto do_exit;
	}

	state = &panel->state;
	lcd_info = &panel->lcd_info;
	dt_dsu_info = &lcd_info->dt_dsu_info;

	if (dt_dsu_info->dsu_en == 0) {
		panel_err("PANEL:ERR:%s:this panel does not support dsu\n", __func__);
		ret = -EINVAL;
		goto do_exit;
	}

	if (dsu->right > dsu->left)
		xres = dsu->right - dsu->left;
	else
		xres = dsu->left - dsu->right;

	if (dsu->bottom > dsu->top)
		yres = dsu->bottom - dsu->top;
	else
		yres = dsu->top - dsu->bottom;

	panel_info("dsu mode : %d, %d:%d-%d:%d", dsu->mode,
		dsu->left, dsu->top, dsu->right, dsu->bottom);
	panel_info("dsu_mode : xres : %d, yres : %d\n", xres, yres);

	actual_mode = dsu->mode - DSU_MODE_1;
	if (actual_mode >= dt_dsu_info->dsu_number) {
		panel_err("PANEL:ERR:%s:Wrong actual mode : %d , number : %d\n",
			__func__, actual_mode, dt_dsu_info->dsu_number);
		actual_mode = 0;
	}
	lcd_info->xres = xres;
	lcd_info->yres = yres;
	lcd_info->dsu_mode = dsu->mode;

	lcd_info->dsc_enabled = dt_dsu_info->res_info[actual_mode].dsc_en;
	lcd_info->dsc_slice_h = dt_dsu_info->res_info[actual_mode].dsc_height;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DSU_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}

	return 0;

do_exit:
	return ret;
}
#endif


static int panel_ioctl_dsim_probe(struct v4l2_subdev *sd, void *arg)
{
	int *param = (int *)arg;
	struct panel_device *panel = container_of(sd, struct panel_device, sd);

	panel_info("PANEL:INFO:%s:PANEL_IOC_DSIM_PROBE\n", __func__);
	if (param == NULL) {
		panel_err("PANEL:ERR:%s:invalid arg\n", __func__);
		return -EINVAL;
	}
	panel->dsi_id = *param;
	panel_info("PANEL:INFO:%s:panel id : %d, dsim id : %d\n",
		__func__, panel->id, panel->dsi_id);
	v4l2_set_subdev_hostdata(sd, &panel->lcd_info);

	return 0;
}

static int panel_ioctl_dsim_ops(struct v4l2_subdev *sd)
{
	struct mipi_drv_ops *mipi_ops;
	struct panel_device *panel = container_of(sd, struct panel_device, sd);

	panel_info("PANEL:INFO:%s:PANEL_IOC_MIPI_OPS\n", __func__);
	mipi_ops = (struct mipi_drv_ops *)v4l2_get_subdev_hostdata(sd);
	if (mipi_ops == NULL) {
		panel_err("PANEL:ERR:%s:mipi_ops is null\n", __func__);
		return -EINVAL;
	}
	panel->mipi_drv.read = mipi_ops->read;
	panel->mipi_drv.write = mipi_ops->write;
	panel->mipi_drv.get_state = mipi_ops->get_state;

	return 0;
}


#ifdef CONFIG_SUPPORT_DSU
static int panel_ioctl_set_dsu(struct v4l2_subdev *sd)
{
	int ret = 0;
	struct dsu_info *dsu;
	struct panel_device *panel = container_of(sd, struct panel_device, sd);

	panel_info("PANEL:INFO:%s:PANEL_IOC_SET_DSU\n", __func__);
	dsu = (struct dsu_info *)v4l2_get_subdev_hostdata(sd);
	if (dsu == NULL) {
		panel_err("PANEL:ERR:%s:dsu info is invalid\n", __func__);
		return -EINVAL;
	}
	ret = panel_set_dsu(panel, dsu);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to set dsu on\n", __func__);
	}
	return ret;
}
#endif

static int panel_ioctl_display_on(struct panel_device *panel, void *arg)
{
	int ret = 0;
	int *disp_on = (int *)arg;

	if (disp_on == NULL) {
		panel_err("PANEL:ERR:%s:invalid arg\n", __func__);
		return -EINVAL;
	}
	if (*disp_on == 0)
		ret = panel_display_off(panel);
	else
		ret = panel_display_on(panel);

	return ret;
}

static int panel_ioctl_set_reset(struct panel_device *panel)
{
	int ret = 0;

	ret = panel_set_reset(panel);
	return ret;
}

static int panel_ioctl_set_power(struct panel_device *panel, void *arg)
{
	int ret = 0;
	int *disp_on = (int *)arg;

	if (disp_on == NULL) {
		panel_err("PANEL:ERR:%s:invalid arg\n", __func__);
		return -EINVAL;
	}
	if (*disp_on == 0)
		ret = panel_power_off(panel);
	else
		ret = panel_power_on(panel);

	return ret;
}

static int panel_set_reset_cb(struct v4l2_subdev *sd)
{
	struct host_cb *rst_cb;
	struct panel_device *panel = container_of(sd, struct panel_device, sd);

	rst_cb = (struct host_cb *)v4l2_get_subdev_hostdata(sd);
	if (!rst_cb) {
		panel_err("PANEL:ERR:%s:reset cb is null\n", __func__);
		return -EINVAL;
	}
	panel->reset_cb.cb = rst_cb->cb;
	panel->reset_cb.data = rst_cb->data;

	return 0;
}
static long panel_core_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct panel_device *panel = container_of(sd, struct panel_device, sd);

	mutex_lock(&panel->io_lock);

	switch(cmd) {
		case PANEL_IOC_DSIM_PROBE:
			ret = panel_ioctl_dsim_probe(sd, arg);
			break;

		case PANEL_IOC_DECON_PROBE:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_DECON_PROBE\n", __func__, panel->id);
			v4l2_set_subdev_hostdata(sd, &panel->lcd_info);
			break;

		case PANEL_IOC_DSIM_PUT_MIPI_OPS:
			ret = panel_ioctl_dsim_ops(sd);
			break;

		case PANEL_IOC_REG_RESET_CB:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_REG_PANEL_RESET\n", __func__, panel->id);
			ret = panel_set_reset_cb(sd);
			break;

		case PANEL_IOC_GET_PANEL_STATE:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_GET_PANEL_STATE\n", __func__, panel->id);
			v4l2_set_subdev_hostdata(sd, &panel->state);
			break;

		case PANEL_IOC_PANEL_PROBE:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_PANEL_PROBE\n", __func__, panel->id);
			ret = panel_probe(panel);
			break;

		case PANEL_IOC_SLEEP_IN:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_SLEEP_IN\n", __func__, panel->id);
			ret = panel_sleep_in(panel);
			break;

		case PANEL_IOC_SLEEP_OUT:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_SLEEP_OUT\n", __func__, panel->id);
			ret = panel_sleep_out(panel);
			break;

		case PANEL_IOC_SET_POWER:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_SET_POWER\n", __func__, panel->id);
			ret = panel_ioctl_set_power(panel, arg);
			break;
		case PANEL_IOC_SET_PANEL_RESET:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_SET_PANEL_RESET\n", __func__, panel->id);
			ret = panel_ioctl_set_reset(panel);
			break;
		case PANEL_IOC_PANEL_DUMP :
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_PANEL_DUMP\n", __func__, panel->id);
			ret = panel_debug_dump(panel);
			break;
#ifdef CONFIG_SUPPORT_DOZE
		case PANEL_IOC_DOZE:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_DOZE\n", __func__ ,panel->id);
			ret = panel_doze(panel);
			mdnie_update(&panel->mdnie);
			break;
#endif
#ifdef CONFIG_SUPPORT_DSU
		case PANEL_IOC_SET_DSU:
			ret = panel_ioctl_set_dsu(sd);
			break;
#endif
		case PANEL_IOC_DISP_ON:
			panel_info("PANEL:INFO:%s:%d:PANEL_IOC_DISP_ON\n", __func__, panel->id);
			ret = panel_ioctl_display_on(panel, arg);
			break;

		case PANEL_IOC_EVT_FRAME_DONE:
			//panel_info("PANEL:INFO:%s:PANEL_IOC_EVT_FRAME_DONE\n", __func__);
			copr_update_start(&panel->copr, 3);
			break;
		case PANEL_IOC_EVT_VSYNC:
			//panel_dbg("PANEL:INFO:%s:PANEL_IOC_EVT_VSYNC\n", __func__);
			break;
		default:
			panel_err("PANEL:ERR:%s:undefined command\n", __func__);
			ret = -EINVAL;
			break;

	}

	if (ret) {
		panel_err("PANEL:ERR:%s:failed to ioctl panel cmd : %d\n",
			__func__,  _IOC_NR(cmd));
	}
	mutex_unlock(&panel->io_lock);

	return (long)ret;
}

static const struct v4l2_subdev_core_ops panel_v4l2_sd_core_ops = {
	.ioctl = panel_core_ioctl,
};

static const struct v4l2_subdev_ops panel_subdev_ops = {
	.core = &panel_v4l2_sd_core_ops,
};

static void panel_init_v4l2_subdev(struct panel_device *panel)
{
	struct v4l2_subdev *sd = &panel->sd;

	v4l2_subdev_init(sd, &panel_subdev_ops);
	sd->owner = THIS_MODULE;
	sd->grp_id = 0;
	snprintf(sd->name, sizeof(sd->name), "%s.%d", "panel-sd", panel->id);

	v4l2_set_subdevdata(sd, panel);
}


static int panel_drv_set_gpios(struct panel_device *panel)
{
	int ret = 0;
	int rst_val = -1, det_val = -1;
	struct panel_pad *pad = &panel->pad;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:pad is null\n", __func__);
		return -EINVAL;
	}

	if (pad->gpio_reset != 0) {
		ret = gpio_request(pad->gpio_reset, "lcd_reset");
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:failed to request gpio reset\n",
				__func__);
			return -EINVAL;
		}
		rst_val = gpio_get_value(pad->gpio_reset);
	}

	if (pad->gpio_disp_det != 0) {
		ret = gpio_request_one(pad->gpio_disp_det, GPIOF_IN, "disp_det");
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:failed to request gpio disp det : %d\n",
				__func__, ret);
			return -EINVAL;
		}
		det_val = gpio_get_value(pad->gpio_disp_det);
	} else {
		panel_info("PANEL:INFO:%s:gpio for disp_det was not supported\n", __func__);
	}
	/*
	 * panel state is decided by rst and disp_det pin
	 *
	 * @rst_val
	 *  0 : need to init panel in kernel
	 *  1 : already initialized in bootloader
	 *
	 * @det_val
	 *  0 : panel is not connected
	 *  1 : panel is connected
	 */
	if (rst_val == 1) {
		panel->state.init_at = PANEL_INIT_BOOT;
		if (det_val == 0) {
			gpio_direction_output(pad->gpio_reset, 0);
			panel->state.connect_panel = PANEL_DISCONNECT;
		} else {
			gpio_direction_output(pad->gpio_reset, 1);
			panel->state.connect_panel = PANEL_CONNECT;
			panel->state.cur_state = PANEL_STATE_NORMAL;
			panel->state.power = PANEL_POWER_ON;
			panel->state.disp_on = PANEL_DISPLAY_ON;
		}
#ifdef CONFIG_SUPPORT_PANEL_SWAP
		connect_panel = panel->state.connect_panel;
#endif
	} else {
		panel->state.init_at = PANEL_INIT_KERNEL;
	}

	if (pad->gpio_disp_det != 0) {
		panel_dbg("PANEL:INFO:%s: rst:%d, disp_det:%d (init_at:%s, state:%s)\n",
			__func__, rst_val, det_val,
			(panel->state.init_at ? "BL" : "KERNEL"),
			(panel->state.connect_panel ? "CONNECTED" : "DISCONNECTED"));
	} else {
		panel_dbg("PANEL:INFO:%s: rst:%d (init_at:%s, state:%s)\n",
			__func__, rst_val,
			(panel->state.init_at ? "BL" : "KERNEL"),
			(panel->state.connect_panel ? "CONNECTED" : "DISCONNECTED"));
	}

	return 0;
}

static inline int panel_get_gpio(struct device *dev ,char *name)
{
	int ret = 0;
	ret = of_gpio_named_count(dev->of_node, name);
	if (ret != 1) {
		panel_err("PANEL:ERR:%s:can't find gpio named : %s\n",
			__func__, name);
		return -EINVAL;
	}
	ret = of_get_named_gpio(dev->of_node, name, 0);
	if (!gpio_is_valid(ret)) {
		panel_err("PANEL:ERR:%s:failed to get gpio named : %s\n",
			__func__, name);
		return -EINVAL;
	}
	return ret;
}

static int panel_parse_gpio(struct panel_device *panel)
{
	int i;
	int ret;
	struct device_node *pend_disp_det;
	struct device *dev = panel->dev;
	struct panel_pad *pad = &panel->pad;
	int gpio_result[PANEL_GPIO_MAX];
	char *gpio_lists[PANEL_GPIO_MAX] = {
		GPIO_NAME_RESET, GPIO_NAME_DISP_DET, GPIO_NAME_PCD, GPIO_NAME_ERR_FG
	};

	for (i = 0; i < PANEL_GPIO_MAX; i++) {
		ret = panel_get_gpio(dev, gpio_lists[i]);
		if (ret <= 0)
			 ret = 0;
		gpio_result[i] = ret;
	}
	pad->gpio_reset = gpio_result[PANEL_GPIO_RESET];
	pad->gpio_disp_det = gpio_result[PANEL_GPIO_DISP_DET];
	pad->gpio_pcd = gpio_result[PANEL_GPIO_PCD];
	pad->gpio_err_fg = gpio_result[PANEL_GPIO_ERR_FG];

	ret = panel_drv_set_gpios(panel);

	if (pad->gpio_disp_det) {
		pad->irq_disp_det = gpio_to_irq(pad->gpio_disp_det);
		pend_disp_det = of_get_child_by_name(dev->of_node, "pend,disp-det");
		if (!pend_disp_det) {
			panel_warn("PANEL:WARN:%s:No DT node for te_eint\n", __func__);
		}
		else {
			pad->pend_reg_disp_det = of_iomap(pend_disp_det, 0);
			if (!pad->pend_reg_disp_det) {
				panel_err("PANEL:ERR:%s:failed to get disp pend reg\n", __func__);
				return -EINVAL;
			}
			of_property_read_u32(pend_disp_det, "pend-bit",
				&pad->pend_bit_disp_det);
			panel_info("PANEL:INFO:%s:pend bit disp_det %x\n",\
				__func__, pad->pend_bit_disp_det);
			panel_info("PANEL:INFO:%s:disp_det pend : %x\n",
				__func__, readl(pad->pend_reg_disp_det));
		}
	}
	if (pad->gpio_pcd)
		pad->irq_pcd = gpio_to_irq(pad->gpio_pcd);

	if (pad->gpio_err_fg)
		pad->irq_err_fg = gpio_to_irq(pad->gpio_err_fg);

	if (pad->gpio_reset == 0) {
		panel_err("PANEL:ERR:%s:can't find gpio for panel reset\n",
			__func__);
		return -EINVAL;
	}

	return ret;
}


static int panel_parse_regulator(struct panel_device *panel)
{
	int i;
	int ret = 0;
	char *tmp_str;
	struct device *dev = panel->dev;
	struct panel_pad *pad = &panel->pad;
	struct regulator *reg[REGULATOR_MAX];
	char *reg_lists[REGULATOR_MAX] = {
		REGULATOR_3p0_NAME, REGULATOR_1p8_NAME, REGULATOR_1p6_NAME,
	};

	for (i = 0; i < REGULATOR_MAX; i++) {
		of_property_read_string(dev->of_node, reg_lists[i],
			(const char **)&tmp_str);
		if (tmp_str == NULL) {
			panel_err("PANEL:ERR:%s:failed to get regulator string : %s\n",
				__func__, reg_lists[i]);
			ret = -EINVAL;
			goto parse_regulator_err;
		}
		reg[i] = regulator_get(NULL, tmp_str);
		if (IS_ERR(reg[i])) {
			panel_err("PANEL:ERR:%s:failed to get regulator :%s:%s\n",
				__func__, reg_lists[i], tmp_str);
			ret = -EINVAL;
			goto parse_regulator_err;
		}
		if (panel->state.init_at == PANEL_INIT_BOOT) {
			ret = regulator_enable(reg[i]);
			if (ret) {
				panel_err("PANEL:ERR:%s:failed to enable regualtor : %s\n",
					__func__, tmp_str);
				goto parse_regulator_err;
			}
#ifdef CONFIG_SUPPORT_DOZE
#ifdef CONFIG_SET_1p5_ALPM
			if (i == REGULATOR_1p6V) {
				ret = regulator_set_voltage(reg[i], 1600000, 1600000);
				if (ret) {
					panel_err("PANEL:%s:failed to set volatege\n",
						__func__);
					goto parse_regulator_err;
				}
			}
#endif
#endif
			if (panel->state.connect_panel == PANEL_DISCONNECT) {
				panel_info("PANEL:INFO:%s:panel was disconnected: disable : %s\n",
					__func__,reg_lists[i]);
				ret = regulator_disable(reg[i]);
				if (ret) {
					panel_err("PANEL:ERR:%s:failed to disable regualtor : %s\n",
						__func__, tmp_str);
					goto parse_regulator_err;
				}
			}
		}
	}
	pad->regulator[REGULATOR_3p0V] = reg[REGULATOR_3p0V];
	pad->regulator[REGULATOR_1p8V] = reg[REGULATOR_1p8V];
	pad->regulator[REGULATOR_1p6V] = reg[REGULATOR_1p6V];

	ret = 0;

	panel_dbg("PANEL:INFO:%s:panel parse regulator : done\n", __func__);

parse_regulator_err:
	return ret;
}


static irqreturn_t panel_disp_det_irq(int irq, void *dev_id)
{
	struct panel_device *panel = (struct panel_device *)dev_id;

	queue_work(panel->disp_det_workqueue, &panel->disp_det_work);

	return IRQ_HANDLED;
}


#define ISR_NAME_MAX_CNT 32


int panel_register_isr(struct panel_device *panel)
{
	int ret = 0;
	struct panel_pad *pad = &panel->pad;
	char isr_name[ISR_NAME_MAX_CNT];

	if (panel->state.connect_panel == PANEL_DISCONNECT) {
		return 0;
	}

	memset(isr_name, 0, ISR_NAME_MAX_CNT);
	sprintf(isr_name, "%s_%d","disp_det", panel->id);

	panel_info("PANEL:INFO:%s:isr name : %s\n", __func__, isr_name);
	clear_disp_det_pend(panel);
	if (pad->gpio_disp_det) {
		ret = devm_request_irq(panel->dev, pad->irq_disp_det, panel_disp_det_irq,
			IRQF_TRIGGER_FALLING, isr_name, panel);
		if (ret) {
			panel_dbg("PANEL:ERR:%s:failed to register disp det irq\n", __func__);
			return ret;
		}
	}
	if (panel->state.cur_state!= PANEL_STATE_NORMAL) {
		disable_irq(panel->pad.irq_disp_det);
	}

	return 0;
}

static int panel_parse_lcd_info(struct panel_device *panel)
{
	int ret = 0;
	int err = 0;
	unsigned int res[3];

	struct device_node *node;
	struct device *dev = panel->dev;
	struct decon_lcd *panel_info = &panel->lcd_info;
#ifdef CONFIG_SUPPORT_DSU
	int dsu_number, i;
	unsigned int dsu_res[MAX_DSU_RES_NUMBER];
	struct dsu_info_dt *dt_dsu_info = &panel_info->dt_dsu_info;
#endif

	node = of_parse_phandle(dev->of_node, "ddi_info", 0);

	err = of_property_read_string(node, "ldi_name", &panel_info->ldi_name);
	if (err) {
		panel_err("PANEL:ERR:%s:failed to get ldi name\n", __func__);
		panel_info->ldi_name = NULL;
	}

	if (panel_info->ldi_name != NULL)
		panel_dbg("PANEL:DBG:Registered ldi : %s\n", panel_info->ldi_name);

	of_property_read_u32(node, "mode", &panel_info->mode);
	panel_dbg("PANEL:DBG:mode : %s\n", panel_info->mode ? "command" : "video");

	of_property_read_u32_array(node, "resolution", res, 2);
	panel_info->xres = res[0];
	panel_info->yres = res[1];
	panel_dbg("PAENL:DBG: LCD(%s) resolution: xres(%d), yres(%d)\n",
			of_node_full_name(node), panel_info->xres, panel_info->yres);

	of_property_read_u32_array(node, "size", res, 2);
	panel_info->width = res[0];
	panel_info->height = res[1];
	panel_dbg("LCD size: width(%d), height(%d)\n", res[0], res[1]);

	of_property_read_u32(node, "timing,refresh", &panel_info->fps);
	panel_dbg("PANEL:DBG:LCD refresh rate(%d)\n", panel_info->fps);

	of_property_read_u32_array(node, "timing,h-porch", res, 3);
	panel_info->hbp = res[0];
	panel_info->hfp = res[1];
	panel_info->hsa = res[2];
	panel_dbg("PANEL:DBG:hbp(%d), hfp(%d), hsa(%d)\n",
		panel_info->hbp, panel_info->hfp, panel_info->hsa);

	of_property_read_u32_array(node, "timing,v-porch", res, 3);
	panel_info->vbp = res[0];
	panel_info->vfp = res[1];
	panel_info->vsa = res[2];
	panel_dbg("PANEL:DBG:vbp(%d), vfp(%d), vsa(%d)\n",
		panel_info->vbp, panel_info->vfp, panel_info->vsa);


	of_property_read_u32(node, "timing,dsi-hs-clk", &panel_info->hs_clk);
	//dsim->clks.hs_clk = dsim->lcd_info.hs_clk;
	panel_dbg("PANEL:DBG:requested hs clock(%d)\n", panel_info->hs_clk);

	of_property_read_u32_array(node, "timing,pms", res, 3);
	panel_info->dphy_pms.p = res[0];
	panel_info->dphy_pms.m = res[1];
	panel_info->dphy_pms.s = res[2];
	panel_dbg("PANEL:DBG:p(%d), m(%d), s(%d)\n",
		panel_info->dphy_pms.p, panel_info->dphy_pms.m, panel_info->dphy_pms.s);


	of_property_read_u32(node, "timing,dsi-escape-clk", &panel_info->esc_clk);
	//dsim->clks.esc_clk = panel_dbg->esc_clk;
	panel_dbg("PANEL:DBG:requested escape clock(%d)\n", panel_info->esc_clk);


	of_property_read_u32(node, "mic_en", &panel_info->mic_enabled);
	panel_dbg("PANEL:DBG:mic enabled (%d)\n", panel_info->mic_enabled);

	of_property_read_u32(node, "type_of_ddi", &panel_info->ddi_type);
	panel_dbg("PANEL:DBG:ddi type(%d)\n",  panel_info->ddi_type);

	of_property_read_u32(node, "dsc_en", &panel_info->dsc_enabled);
	panel_dbg("PANEL:DBG:dsc is %s\n", panel_info->dsc_enabled ? "enabled" : "disabled");

	if (panel_info->dsc_enabled) {
		of_property_read_u32(node, "dsc_cnt", &panel_info->dsc_cnt);
		panel_dbg("PANEL:DBG:dsc count(%d)\n", panel_info->dsc_cnt);

		of_property_read_u32(node, "dsc_slice_num", &panel_info->dsc_slice_num);
		panel_dbg("PANEL:DBG:dsc slice count(%d)\n", panel_info->dsc_slice_num);

		of_property_read_u32(node, "dsc_slice_h", &panel_info->dsc_slice_h);
		panel_dbg("PANEL:DBG:dsc slice height(%d)\n", panel_info->dsc_slice_h);
	}
	of_property_read_u32(node, "data_lane",
		&panel_info->data_lane_cnt);
	panel_dbg("PANEL:DBG:using data lane count(%d)\n",
		panel_info->data_lane_cnt);

	if (panel_info->mode == DECON_MIPI_COMMAND_MODE) {
		of_property_read_u32(node, "cmd_underrun_lp_ref",
			&panel_info->cmd_underrun_lp_ref);
		panel_dbg("PANEL:DBG:cmd_underrun_lp_ref(%d)\n",
			panel_info->cmd_underrun_lp_ref);
	} else {
		of_property_read_u32(node, "vt_compensation",
			&panel_info->vt_compensation);
		panel_dbg("PANEL:DBG:vt_compensation(%d)\n",
			panel_info->vt_compensation);
	}
#ifdef CONFIG_SUPPORT_DSU
	of_property_read_u32(node, "dsu_en", &dt_dsu_info->dsu_en);
	panel_info("PANEL:INFO:%s:dsu_en : %d\n", __func__, dt_dsu_info->dsu_en);

	if (dt_dsu_info->dsu_en) {
		of_property_read_u32(node, "dsu_number", &dsu_number);
		panel_info("PANEL:INFO:%s:dsu_number : %d\n", __func__, dsu_number);
		if (dsu_number > MAX_DSU_RES_NUMBER) {
			panel_err("PANEL:ERR:%s:Exceed dsu res number : %d\n",
				__func__, dsu_number);
			dsu_number = MAX_DSU_RES_NUMBER;
		}
		dt_dsu_info->dsu_number = dsu_number;

		of_property_read_u32_array(node, "dsu_width", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].width = dsu_res[i];
			panel_info("PANEL:INFO:%s:width[%d]:%d\n", __func__, i, dsu_res[i]);
		}
		of_property_read_u32_array(node, "dsu_height", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].height = dsu_res[i];
			panel_info("PANEL:INFO:%s:height[%d]:%d\n", __func__, i, dsu_res[i]);
		}
		of_property_read_u32_array(node, "dsu_dsc_en", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].dsc_en = dsu_res[i];
			panel_info("PANEL:INFO:%s:dsc_en[%d]:%d\n", __func__, i, dsu_res[i]);
		}

		of_property_read_u32_array(node, "dsu_dsc_width", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].dsc_width = dsu_res[i];
			panel_info("PANEL:INFO:%s:dsc_width[%d]:%d\n", __func__, i, dsu_res[i]);
		}
		of_property_read_u32_array(node, "dsu_dsc_height", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].dsc_height= dsu_res[i];
			panel_info("PANEL:INFO:%s:dsc_height[%d]:%d\n", __func__, i, dsu_res[i]);
		}
	}

#endif
	of_property_read_u32(node, "op_mode", &panel_info->op_mode);
	panel_dbg("PANEL:DBG:op_mode : %d\n", panel_info->op_mode);

	of_property_read_u32(node, "ss_path_swap", &panel_info->disp_ss_path_swap);
	panel_dbg("PANEL:DBG:ss path swap : %d\n", panel_info->disp_ss_path_swap);
	return ret;
}

static int panel_parse_panel_lookup(struct panel_device *panel)
{
	struct device *dev = panel->dev;
	struct panel_info *panel_data = &panel->panel_data;
	struct panel_lut_info *lut_info = &panel_data->lut_info;
	struct device_node *np;
	int ret, i, sz, sz_lut;

	np = of_get_child_by_name(dev->of_node, "panel-lookup");
	if (unlikely(!np)) {
		panel_warn("PANEL:WARN:%s:No DT node for panel-lookup\n", __func__);
		return -EINVAL;
	}

	sz = of_property_count_strings(np, "panel-name");
	if (sz <= 0) {
		panel_warn("PANEL:WARN:%s:No panel-name property\n", __func__);
		return -EINVAL;
	}

	if (sz >= ARRAY_SIZE(lut_info->names)) {
		panel_warn("PANEL:WARN:%s:exceeded MAX_PANEL size\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < sz; i++) {
		ret = of_property_read_string_index(np,
				"panel-name", i, &lut_info->names[i]);
		if (ret) {
			panel_warn("PANEL:WARN:%s:failed to read panel-name[%d]\n",
					__func__, i);
			return -EINVAL;
		}
	}
	lut_info->nr_panel = sz;

	sz_lut = of_property_count_u32_elems(np, "panel-lut");
	if ((sz_lut % 3) || (sz_lut >= MAX_PANEL_LUT)) {
		panel_warn("PANEL:WARN:%s:sz_lut(%d) should be multiple of 3"
				" and less than MAX_PANEL_LUT\n", __func__, sz_lut);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "panel-lut",
			(u32 *)lut_info->lut, sz_lut);
	if (ret) {
		panel_warn("PANEL:WARN:%s:failed to read panel-ltu\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < sz_lut / 3; i++) {
		if (lut_info->lut[i].index >= lut_info->nr_panel) {
			panel_warn("PANEL:WARN:%s:invalid panel index(%d)\n",
					__func__, lut_info->lut[i].index);
			return -EINVAL;
		}
	}
	lut_info->nr_lut = sz_lut / 3;

	print_panel_lut(lut_info);

	return 0;
}

static int panel_parse_dt(struct panel_device *panel)
{
	int ret = 0;
	struct device *dev = panel->dev;

	if (IS_ERR_OR_NULL(dev->of_node)) {
		panel_err("ERR:PANEL:%s:failed to get dt info\n", __func__);
		return -EINVAL;
	}

	panel->id = of_alias_get_id(dev->of_node, "panel");
	if (panel->id < 0) {
		panel_err("ERR:PANEL:%s:invalid panel's id : %d\n",
			__func__, panel->id);
		return panel->id;
	}
	panel_dbg("INFO:PANEL:%s:panel-id:%d\n", __func__, panel->id);

	panel_parse_lcd_info(panel);

	if ((panel->id != 0) &&
		(panel->lcd_info.op_mode == DSI_SDDD_MODE)) {
			goto skip_gpio_regulator;
	}
	panel_parse_gpio(panel);
	panel_parse_regulator(panel);
skip_gpio_regulator:
	panel_parse_panel_lookup(panel);

	return ret;
}

void disp_det_handler(struct work_struct *data)
{
	int ret, disp_det;
	struct panel_device *panel =
			container_of(data, struct panel_device, disp_det_work);
	struct panel_pad *pad = &panel->pad;
	struct backlight_device *bd = panel->panel_bl.bd;
	struct panel_state *state = &panel->state;
	struct host_cb *reset_cb = &panel->reset_cb;

	disp_det = gpio_get_value(pad->gpio_disp_det);
	panel_info("PANEL:INFO:%s: disp_det:%d\n", __func__, disp_det);

	switch (state->cur_state) {
		case PANEL_STATE_ALPM :
		case PANEL_STATE_NORMAL:
			if (disp_det == 0) {
				disable_irq(pad->irq_disp_det);
				/* delay for disp_det deboundce */
				usleep_range(200000, 200000);

				panel_err("PANEL:ERR:%s:disp_det is abnormal state\n",
					__func__);
				if (reset_cb->cb) {
					ret = reset_cb->cb(reset_cb->data);
					if (ret)
						panel_err("PANEL:ERR:%s:failed to reset panel\n",
							__func__);
					backlight_update_status(bd);
				}
				clear_disp_det_pend(panel);
				enable_irq(pad->irq_disp_det);
			}
			break;
		default:
			break;
	}

	return;
}

static int panel_fb_notifier(struct notifier_block *self, unsigned long event, void *data)
{
	int *blank = NULL;
	struct panel_device *panel;
	struct fb_event *fb_event = data;

	switch (event) {
		case FB_EARLY_EVENT_BLANK:
		case FB_EVENT_BLANK:
			break;
		case FB_EVENT_FB_REGISTERED:
			panel_dbg("PANEL:INFO:%s:FB Registeted\n", __func__);
			return 0;
		default:
			return 0;

	}

	panel = container_of(self, struct panel_device, fb_notif);
	blank = fb_event->data;
	if (!blank || !panel) {
		panel_err("PANEL:ERR:%s:blank is null\n", __func__);
		return 0;
	}

	switch (*blank) {
		case FB_BLANK_POWERDOWN:
		case FB_BLANK_NORMAL:
			if (event == FB_EARLY_EVENT_BLANK)
				panel_dbg("PANEL:INFO:%s:EARLY BLANK POWER DOWN\n", __func__);
			else
				panel_dbg("PANEL:INFO:%s:BLANK POWER DOWN\n", __func__);
			break;
		case FB_BLANK_UNBLANK:
			if (event == FB_EARLY_EVENT_BLANK)
				panel_dbg("PANEL:INFO:%s:EARLY UNBLANK\n", __func__);
			else
				panel_dbg("PANEL:INFO:%s:UNBLANK\n", __func__);
			break;
	}
	return 0;
}

#ifdef CONFIG_DISPLAY_USE_INFO
static int panel_dpui_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct panel_info *panel_data;
	struct panel_device *panel;
	struct common_panel_info *info;
	int panel_id;
	struct dpui_info *dpui = data;
	char tbuf[MAX_DPUI_VAL_LEN];
	u8 panel_datetime[7] = { 0, };
	u8 panel_chip_id[5] = { 0, };
	int size;

	if (dpui == NULL) {
		panel_err("%s: dpui is null\n", __func__);
		return 0;
	}

	panel = container_of(self, struct panel_device, panel_dpui_notif);
	panel_data = &panel->panel_data;
	panel_id = panel_data->id[0] << 16 | panel_data->id[1] << 8 | panel_data->id[2];

	info = find_panel(panel, panel_id);
	if (unlikely(!info)) {
		panel_err("%s, panel not found\n", __func__);
		return -ENODEV;
	}

	resource_copy_by_name(panel_data, panel_datetime, "date");
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%04d%02d%02d %02d%02d%02d",
			((panel_datetime[0] & 0xF0) >> 4) + 2011, panel_datetime[0] & 0xF, panel_datetime[1] & 0x1F,
			panel_datetime[2] & 0x1F, panel_datetime[3] & 0x3F, panel_datetime[4] & 0x3F);
	set_dpui_field(DPUI_KEY_MAID_DATE, tbuf, size);

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", panel_data->id[0]);
	set_dpui_field(DPUI_KEY_LCDID1, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", panel_data->id[1]);
	set_dpui_field(DPUI_KEY_LCDID2, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", panel_data->id[2]);
	set_dpui_field(DPUI_KEY_LCDID3, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%s_%s", info->vendor, info->model);
	set_dpui_field(DPUI_KEY_DISP_MODEL, tbuf, size);

	resource_copy_by_name(panel_data, panel_chip_id, "chip_id");
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "0x%02X%02X%02X%02X%02X",
			panel_chip_id[0], panel_chip_id[1], panel_chip_id[2],
			panel_chip_id[3], panel_chip_id[4]);
	set_dpui_field(DPUI_KEY_CHIPID, tbuf, size);

	return 0;
}
#endif /* CONFIG_DISPLAY_USE_INFO */

static int panel_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct panel_device *panel = NULL;

	panel = devm_kzalloc(dev, sizeof(struct panel_device), GFP_KERNEL);
	if (!panel) {
		panel_err("failed to allocate dsim device.\n");
		ret = -ENOMEM;
		goto probe_err;
	}
	panel->dev = dev;
	power_skip = 0;
	panel->state.init_at = PANEL_INIT_BOOT;
	panel->state.connect_panel = PANEL_CONNECT;
	panel->state.cur_state = PANEL_STATE_OFF;
	panel->state.power = PANEL_POWER_OFF;
	panel->state.disp_on = PANEL_DISPLAY_OFF;
	panel->ktime_panel_on = ktime_get();
#ifdef CONFIG_SUPPORT_HMD
	panel->state.hmd_on = PANEL_HMD_OFF;
#endif

#ifdef CONFIG_SUPPORT_DSU
	panel->lcd_info.dsu_mode = DSU_MODE_1;
#endif
#ifdef CONFIG_ACTIVE_CLOCK
	panel->act_clk_dev.act_info.update_img = IMG_UPDATE_NEED;
#endif
	mutex_init(&panel->op_lock);
	mutex_init(&panel->data_lock);
	mutex_init(&panel->io_lock);
	mutex_init(&panel->panel_bl.lock);
	mutex_init(&panel->mdnie.lock);
	mutex_init(&panel->copr.lock);

	panel_parse_dt(panel);

	panel_init_v4l2_subdev(panel);

	platform_set_drvdata(pdev, panel);

	INIT_WORK(&panel->disp_det_work, disp_det_handler);

	panel->disp_det_workqueue = create_singlethread_workqueue("disp_det");
	if (panel->disp_det_workqueue == NULL) {
		panel_err("ERR:PANEL:%s:failed to create workqueue for disp det\n", __func__);
		ret = -ENOMEM;
		goto probe_err;
	}

	panel->fb_notif.notifier_call = panel_fb_notifier;
	ret = fb_register_client(&panel->fb_notif);
	if (ret) {
		panel_err("ERR:PANEL:%s:failed to register fb notifier callback\n", __func__);
		goto probe_err;
	}
#ifdef CONFIG_DISPLAY_USE_INFO
	panel->panel_dpui_notif.notifier_call = panel_dpui_notifier_callback;
	ret = dpui_logging_register(&panel->panel_dpui_notif, DPUI_TYPE_PANEL);
	if (ret) {
		panel_err("ERR:PANEL:%s:failed to register dpui notifier callback\n", __func__);
		goto probe_err;
	}
#endif
	panel_register_isr(panel);
probe_err:
	return ret;
}

static const struct of_device_id panel_drv_of_match_table[] = {
	{ .compatible = "samsung,panel-drv", },
	{ },
};
MODULE_DEVICE_TABLE(of, panel_drv_of_match_table);

static struct platform_driver panel_driver = {
	.probe = panel_drv_probe,
	.driver = {
		.name = "panel-drv",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(panel_drv_of_match_table),
	}
};

static int __init get_boot_panel_id(char *arg)
{
	get_option(&arg, &boot_panel_id);
	panel_info("PANEL:INFO:%s:boot_panel id : 0x%x\n",
			__func__, boot_panel_id);

	return 0;
}

early_param("lcdtype", get_boot_panel_id);

static int __init panel_drv_init (void)
{
	return platform_driver_register(&panel_driver);
}

static void __exit panel_drv_exit(void)
{
	platform_driver_unregister(&panel_driver);
}

module_init(panel_drv_init);
module_exit(panel_drv_exit);
MODULE_DESCRIPTION("Samsung's Panel Driver");
MODULE_AUTHOR("<minwoo7945.kim@samsung.com>");
MODULE_LICENSE("GPL");
