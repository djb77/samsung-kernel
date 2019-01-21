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
Copyright (C) 2015, Samsung Electronics. All rights reserved.

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
#include "ss_dsi_panel_common.h"
#include "../mdss_debug.h"

static void mdss_samsung_event_osc_te_fitting(struct mdss_panel_data *pdata, int event, void *arg);
static irqreturn_t samsung_te_check_handler(int irq, void *handle);
static void samsung_te_check_done_work(struct work_struct *work);
static void mdss_samsung_event_esd_recovery_init(struct mdss_panel_data *pdata, int event, void *arg);

struct samsung_display_driver_data vdd_data;

void __iomem *virt_mmss_gp_base;

struct samsung_display_driver_data *check_valid_ctrl(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = NULL;

	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("Invalid data ctrl : 0x%zx\n", (size_t)ctrl);
		return NULL;
	}

	vdd = (struct samsung_display_driver_data *)ctrl->panel_data.panel_private;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid vdd_data : 0x%zx\n", (size_t)vdd);
		return NULL;
	}

	return vdd;
}

char mdss_panel_id0_get(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return -ERANGE;
	}

	return (vdd->manufacture_id_dsi[display_ndx_check(ctrl)] & 0xFF0000) >> 16;
}

char mdss_panel_id1_get(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return -ERANGE;
	}

	return (vdd->manufacture_id_dsi[display_ndx_check(ctrl)] & 0xFF00) >> 8;
}

char mdss_panel_id2_get(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return -ERANGE;
	}

	return vdd->manufacture_id_dsi[display_ndx_check(ctrl)] & 0xFF;
}

char mdss_panel_rev_get(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return -ERANGE;
	}

	return vdd->manufacture_id_dsi[display_ndx_check(ctrl)] & 0x0F;
}

int mdss_panel_attach_get(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return -ERANGE;
	}

	return (vdd->panel_attach_status & (0x01 << ctrl->ndx)) > 0 ? true : false;
}

int mdss_panel_attach_set(struct mdss_dsi_ctrl_pdata *ctrl, int status)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return -ERANGE;
	}

	/* 0bit->DSI0 1bit->DSI1 */
	/* check the lcd id for DISPLAY_1 and DISPLAY_2 */
	if (likely(mdss_panel_attached(DISPLAY_1) || mdss_panel_attached(DISPLAY_2)) && status) {
		if (ctrl->cmd_sync_wait_broadcast)
			vdd->panel_attach_status |= (BIT(1) | BIT(0));
		else {
			/* One more time check dual dsi */
			if (!IS_ERR_OR_NULL(vdd->ctrl_dsi[DSI_CTRL_0]) &&
				!IS_ERR_OR_NULL(vdd->ctrl_dsi[DSI_CTRL_1]))
				vdd->panel_attach_status |= (BIT(1) | BIT(0));
			else
				vdd->panel_attach_status |= BIT(0) << ctrl->ndx;
		}
	} else {
		if (ctrl->cmd_sync_wait_broadcast)
			vdd->panel_attach_status &= ~(BIT(1) | BIT(0));
		else {
			/* One more time check dual dsi */
			if (!IS_ERR_OR_NULL(vdd->ctrl_dsi[DSI_CTRL_0]) &&
				!IS_ERR_OR_NULL(vdd->ctrl_dsi[DSI_CTRL_1]))
				vdd->panel_attach_status &= ~(BIT(1) | BIT(0));
			else
				vdd->panel_attach_status &= ~(BIT(0) << ctrl->ndx);
		}
	}

	LCD_INFO("panel_attach_status : %d\n", vdd->panel_attach_status);

	return vdd->panel_attach_status;
}

/*
 * Check the lcd id for DISPLAY_1 and DISPLAY_2 using the ndx
 */
int mdss_panel_attached(int ndx)
{
	int lcd_id = 0;

	/*
	 * ndx 0 means DISPLAY_1 and ndx 1 means DISPLAY_2
	 */
	if (ndx == 0)
		lcd_id = get_lcd_attached("GET");
	else if (ndx == 1)
		lcd_id = get_lcd_attached_secondary("GET");

	/*
	 * The 0xFFFFFF is the id for PBA booting
	 * if the id is same with 0xFFFFFF, this function
	 * will return 0
	 */
	return !(lcd_id == PBA_ID);
}

static int mdss_samsung_parse_panel_id(char *panel_id)
{
	char *pt;
	int lcd_id = 0;

	if (!IS_ERR_OR_NULL(panel_id))
		pt = panel_id;
	else
		return lcd_id;

	for (pt = panel_id; *pt != 0; pt++)  {
		lcd_id <<= 4;
		switch (*pt) {
		case '0' ... '9':
			lcd_id += *pt - '0';
			break;
		case 'a' ... 'f':
			lcd_id += 10 + *pt - 'a';
			break;
		case 'A' ... 'F':
			lcd_id += 10 + *pt - 'A';
			break;
		}
	}

	return lcd_id;
}

int get_lcd_attached(char *mode)
{
	static int lcd_id;

	LCD_DEBUG("%s", mode);

	if (mode == NULL)
		return true;

	if (!strncmp(mode, "GET", 3)) {
		goto end;
	} else {
		lcd_id = 0;
		lcd_id = mdss_samsung_parse_panel_id(mode);
	}

	LCD_ERR("LCD_ID = 0x%X\n", lcd_id);
end:
	return lcd_id;
}
EXPORT_SYMBOL(get_lcd_attached);
__setup("lcd_id=0x", get_lcd_attached);

int get_lcd_attached_secondary(char *mode)
{
	static int lcd_id;

	LCD_DEBUG("%s", mode);

	if (mode == NULL)
		return true;

	if (!strncmp(mode, "GET", 3))
		goto end;
	else
		lcd_id = mdss_samsung_parse_panel_id(mode);

	LCD_ERR("LCD_ID = 0x%X\n", lcd_id);
end:
	return lcd_id;
}
EXPORT_SYMBOL(get_lcd_attached_secondary);
__setup("lcd_id2=0x", get_lcd_attached_secondary);

static void mdss_samsung_event_frame_update(struct mdss_panel_data *pdata, int event, void *arg)
{
	int ndx;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)pdata->panel_private;
	struct panel_func *panel_func = NULL;

	s64 cur_time_64;

	int wait_time_32;
	s64 wait_time_64;

	int sleep_out_delay_32;
	s64 sleep_out_delay_64;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);
	panel_func = &vdd->panel_func;

	ndx = display_ndx_check(ctrl);

	sleep_out_delay_32 = vdd->dtsi_data[ndx].samsung_wait_after_sleep_out_delay;
	sleep_out_delay_64 = (s64)sleep_out_delay_32;

	cur_time_64 = ktime_to_ms(ktime_get());
	wait_time_64 = sleep_out_delay_64 - (cur_time_64 - vdd->sleep_out_time_64);

	/* To protect 64bit overflow & underflow */
	if (wait_time_64 <= 0)
		wait_time_32 = 0;
	else if (wait_time_64 > sleep_out_delay_64)
		wait_time_32 = sleep_out_delay_32;
	else
		wait_time_32 = (s32)wait_time_64;

	if (ctrl->cmd_sync_wait_broadcast) {
		if (ctrl->cmd_sync_wait_trigger) {
			if (vdd->display_status_dsi[ndx].wait_disp_on) {
				ATRACE_BEGIN(__func__);

				if (wait_time_32 > 0) {
					LCD_ERR("sleep_out_delay:%d sleep_out_t:%llu cur_t:%llu wait_t:%d start\n", sleep_out_delay_32,
						  vdd->sleep_out_time_64, cur_time_64, wait_time_32);
					usleep_range(wait_time_32*1000, wait_time_32*1000);
					LCD_ERR("wait_t: %d end\n", wait_time_32);
				} else
					LCD_ERR("sleep_out_delay:%d sleep_out_t:%llu cur_t:%llu wait_t:%d skip\n", sleep_out_delay_32,
						  vdd->sleep_out_time_64, cur_time_64, wait_time_32);

				mdss_samsung_send_cmd(ctrl, TX_DISPLAY_ON);
				vdd->display_status_dsi[ndx].wait_disp_on = 0;

				if (vdd->support_mdnie_lite){
					update_dsi_tcon_mdnie_register(vdd);

					if(vdd->support_mdnie_trans_dimming)
						vdd->mdnie_disable_trans_dimming = false;
				}

				if (vdd->panel_func.samsung_backlight_late_on)
					vdd->panel_func.samsung_backlight_late_on(ctrl);

				if (vdd->dtsi_data[0].hmt_enabled &&
					vdd->vdd_blank_mode[0] != FB_BLANK_NORMAL) {
					if (vdd->hmt_stat.hmt_on) {
						LCD_INFO("hmt reset ..\n");
						vdd->hmt_stat.hmt_enable(ctrl, vdd);
						vdd->hmt_stat.hmt_reverse_update(ctrl, 1);
						vdd->hmt_stat.hmt_bright_update(ctrl);
					}
				}
				LCD_INFO("DISPLAY_ON\n");
				ATRACE_END(__func__);
			}
		} else
			vdd->display_status_dsi[ndx].wait_disp_on = 0;
	} else {
		/* Check TE duration when the panel turned on */
		/*
		if (vdd->display_status_dsi[ctrl->ndx].wait_disp_on) {
			vdd->te_fitting_info.status &= ~TE_FITTING_DONE;
			vdd->te_fitting_info.te_duration = 0;
		}
		 */

		if (vdd->dtsi_data[ctrl->ndx].samsung_osc_te_fitting &&
				!(vdd->te_fitting_info.status & TE_FITTING_DONE)) {
			if (panel_func->mdss_samsung_event_osc_te_fitting)
				panel_func->mdss_samsung_event_osc_te_fitting(pdata, event, arg);
		}

		if (vdd->display_status_dsi[ndx].wait_disp_on) {
			ATRACE_BEGIN(__func__);
			if (wait_time_32 > 0) {
					LCD_ERR("sleep_out_delay:%d sleep_out_t:%llu cur_t:%llu wait_t:%d start\n", sleep_out_delay_32,
						vdd->sleep_out_time_64,cur_time_64,wait_time_32);
					usleep_range(wait_time_32*1000, wait_time_32*1000);
					LCD_ERR("wait_t: %d end\n", wait_time_32);
				} else
					LCD_ERR("sleep_out_delay:%d sleep_out_t:%llu cur_t:%llu wait_t:%d skip\n", sleep_out_delay_32,
						vdd->sleep_out_time_64,cur_time_64,wait_time_32);
			mdss_samsung_send_cmd(ctrl, TX_DISPLAY_ON);
			vdd->display_status_dsi[ndx].wait_disp_on = 0;

			if (vdd->support_mdnie_lite){
				update_dsi_tcon_mdnie_register(vdd);

				if(vdd->support_mdnie_trans_dimming)
					vdd->mdnie_disable_trans_dimming = false;
			}

			if (vdd->panel_func.samsung_backlight_late_on)
				vdd->panel_func.samsung_backlight_late_on(ctrl);

			if (vdd->dtsi_data[0].hmt_enabled &&
				vdd->vdd_blank_mode[0] != FB_BLANK_NORMAL) {
				if (vdd->hmt_stat.hmt_on) {
					LCD_INFO("hmt reset ..\n");
					vdd->hmt_stat.hmt_enable(ctrl, vdd);
					vdd->hmt_stat.hmt_reverse_update(ctrl, 1);
					vdd->hmt_stat.hmt_bright_update(ctrl);
				}
			}
			LCD_INFO("DISPLAY_ON\n");
			/* For esd test*/
			mdss_samsung_read_rddpm(vdd);
			ATRACE_END(__func__);
		}
	}

	if (vdd_data.copr.read_copr_wq && vdd->panel_func.samsung_copr_read && vdd->copr.copr_on)
		queue_work(vdd_data.copr.read_copr_wq, &vdd_data.copr.read_copr_work);
}

static void mdss_samsung_send_esd_recovery_cmd(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	static bool toggle;

	/* do not send esd recovery cmd when mipi data pck is sending */
	if (mutex_is_locked(&vdd->vdd_lock))
		return;

	if (toggle)
		mdss_samsung_send_cmd(ctrl, TX_ESD_RECOVERY_1);
	else
		mdss_samsung_send_cmd(ctrl, TX_ESD_RECOVERY_2);
	toggle = !toggle;
}

static void mdss_samsung_event_fb_event_callback(struct mdss_panel_data *pdata, int event, void *arg)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct panel_func *panel_func = NULL;

	if (IS_ERR_OR_NULL(pdata)) {
		LCD_ERR("Invalid data pdata : 0x%zx\n",
				(size_t)pdata);
		return;
	}

	vdd = (struct samsung_display_driver_data *)pdata->panel_private;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return;
	}

	panel_func = &vdd->panel_func;

	if (IS_ERR_OR_NULL(panel_func)) {
		LCD_ERR("Invalid data panel_func : 0x%zx\n",
				(size_t)panel_func);
		return;
	}

	if (panel_func->mdss_samsung_event_esd_recovery_init)
		panel_func->mdss_samsung_event_esd_recovery_init(pdata, event, arg);
}


static int mdss_samsung_dsi_panel_event_handler(
		struct mdss_panel_data *pdata, int event, void *arg)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)pdata->panel_private;
	struct panel_func *panel_func = NULL;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	if (IS_ERR_OR_NULL(ctrl) || IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n",
				(size_t)ctrl, (size_t)vdd);
		return -EINVAL;
	}

	if (event < MDSS_SAMSUNG_EVENT_START) {
		LCD_DEBUG("Unknown event(%d)\n", event);
		goto end;
	}

	/*
	 * The MDSS_SAMSUNG_EVENT_MAX value added
	 * for FB_BLANK_ related events
	 */
	if (event >= MDSS_SAMSUNG_EVENT_MAX)
		event -= MDSS_SAMSUNG_EVENT_MAX;

	LCD_DEBUG("%d\n", event);

	panel_func = &vdd->panel_func;

	if (IS_ERR_OR_NULL(panel_func)) {
		LCD_ERR("Invalid data panel_func : 0x%zx\n", (size_t)panel_func);
		return -EINVAL;
	}

	switch (event) {
	case MDSS_SAMSUNG_EVENT_FRAME_UPDATE:
		if (!IS_ERR_OR_NULL(panel_func->mdss_samsung_event_frame_update))
			panel_func->mdss_samsung_event_frame_update(pdata, event, arg);
		break;
	case MDSS_SAMSUNG_EVENT_FB_EVENT_CALLBACK:
		if (!IS_ERR_OR_NULL(panel_func->mdss_samsung_event_fb_event_callback))
			panel_func->mdss_samsung_event_fb_event_callback(pdata, event, arg);
		break;
	case MDSS_SAMSUNG_EVENT_PANEL_ESD_RECOVERY:
		if (vdd->send_esd_recovery)
			mdss_samsung_send_esd_recovery_cmd(ctrl);
		break;
	case MDSS_SAMSUNG_EVENT_MULTI_RESOLUTION:
		if (panel_func->samsung_multires)
			panel_func->samsung_multires(vdd);
		break;
	default:
		LCD_DEBUG("unhandled event=%d\n", event);
		break;
	}
end:

	return 0;
}

void mdss_samsung_panel_init(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	/* At this time ctrl_pdata->ndx is not set */
	struct mdss_panel_info *pinfo = NULL;
	int ndx = ctrl_pdata->panel_data.panel_info.pdest;
	int loop;


	LCD_ERR("++ (%d)\n", ndx);

	ctrl_pdata->panel_data.panel_private = &vdd_data;

	if (mdss_sasmung_panel_debug_init(&vdd_data))
		LCD_ERR("Fail to create debugfs\n");

	mutex_init(&vdd_data.vdd_lock);
	/* To guarantee BLANK & UNBLANK mode change operation*/
	mutex_init(&vdd_data.vdd_blank_unblank_lock);
	/* To guarantee ALPM ON or OFF mode change operation*/
	mutex_init(&vdd_data.vdd_panel_lpm_lock);
	/*
		To guarantee POC operation.
		Any dsi tx or rx operation is not permitted while poc operation.
	*/
	mutex_init(&vdd_data.vdd_poc_operation_lock);

	/*
		To guarantee POC operation.
		samsung generated cmds & mdss direct cmds are should be protected mutually at POC operation.
		Read operation use global parameter & partial update use 2A/2B.
		Global paratermeter has a chance to hit on 2A or 2B command.
	*/
	mutex_init(&vdd_data.vdd_mdss_direct_cmdlist_lock);

	vdd_data.ctrl_dsi[ndx] = ctrl_pdata;

	pinfo = &ctrl_pdata->panel_data.panel_info;

	if (pinfo && pinfo->mipi.mode == DSI_CMD_MODE) {
		vdd_data.panel_func.mdss_samsung_event_osc_te_fitting =
			mdss_samsung_event_osc_te_fitting;
	}

	vdd_data.panel_func.mdss_samsung_event_frame_update =
		mdss_samsung_event_frame_update;
	vdd_data.panel_func.mdss_samsung_event_fb_event_callback =
		mdss_samsung_event_fb_event_callback;
	vdd_data.panel_func.mdss_samsung_event_esd_recovery_init =
		mdss_samsung_event_esd_recovery_init;

	for (loop = 0; loop < SUPPORT_PANEL_COUNT; loop++)
		vdd_data.manufacture_id_dsi[loop] = PBA_ID;

	mdss_samsung_panel_brightness_init(&vdd_data);

	if (IS_ERR_OR_NULL(vdd_data.panel_func.samsung_panel_init))
		LCD_ERR("no samsung_panel_init fucn");
	else
		vdd_data.panel_func.samsung_panel_init(&vdd_data);

	vdd_data.ctrl_dsi[ndx]->event_handler = mdss_samsung_dsi_panel_event_handler;

	if (vdd_data.support_mdnie_lite || vdd_data.support_cabc) {
		/* check the lcd id for DISPLAY_1 and DISPLAY_2 */
		if (((ndx == 0) && mdss_panel_attached(ndx)) ||
				((ndx == 1) && mdss_panel_attached(ndx))) {
			vdd_data.mdnie_tune_state_dsi[ndx] = init_dsi_tcon_mdnie_class(ndx, &vdd_data);
		}
	}

	/*
		Below for loop are same as initializing sturct brightenss_pasket_dsi.
	vdd_data.brightness[ndx].brightness_packet_dsi[0] = {{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL};
	vdd_data.brightness[ndx].brightness_packet_dsi[1] = {{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL};
	...
	vdd_data.brightness[ndx].brightness_packet_dsi[BRIGHTNESS_MAX_PACKET - 2] = {{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL};
	vdd_data.brightness[ndx].brightness_packet_dsi[BRIGHTNESS_MAX_PACKET - 1 ] = {{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL};
	*/

	spin_lock_init(&vdd_data.esd_recovery.irq_lock);

	vdd_data.hmt_stat.hmt_enable = hmt_enable;
	vdd_data.hmt_stat.hmt_reverse_update = hmt_reverse_update;
	vdd_data.hmt_stat.hmt_bright_update = hmt_bright_update;

	mdss_panel_attach_set(ctrl_pdata, true);

	/* Init blank_mode */
	for (loop = 0; loop < SUPPORT_PANEL_COUNT; loop++)
		vdd_data.vdd_blank_mode[loop] = FB_BLANK_POWERDOWN;

	/* Init Hall ic related things */
	mutex_init(&vdd_data.vdd_hall_ic_lock); /* To guarantee HALL IC switching */
	mutex_init(&vdd_data.vdd_hall_ic_blank_unblank_lock); /* To guarantee HALL IC operation(PMS BLANK & UNBLAMK) */

	/* Init Other line panel support */
	if (!IS_ERR_OR_NULL(vdd_data.panel_func.parsing_otherline_pdata) && mdss_panel_attached(DISPLAY_1)) {
	   if (!IS_ERR_OR_NULL(vdd_data.panel_func.get_panel_fab_type)) {
		   if (vdd_data.panel_func.get_panel_fab_type() == NEW_FB_PANLE_TYPE) {
				LCD_ERR("parsing_otherline_pdata (%d)\n", vdd_data.panel_func.get_panel_fab_type());

				INIT_DELAYED_WORK(&vdd_data.other_line_panel_support_work, (work_func_t)read_panel_data_work_fn);
				vdd_data.other_line_panel_support_workq = create_singlethread_workqueue("other_line_panel_support_wq");

				if (vdd_data.other_line_panel_support_workq) {
				   vdd_data.other_line_panel_work_cnt = OTHERLINE_WORKQ_CNT;
				   queue_delayed_work(vdd_data.other_line_panel_support_workq,
						   &vdd_data.other_line_panel_support_work, msecs_to_jiffies(OTHERLINE_WORKQ_DEALY));
			    }
	       }
		}
	}

#if defined(CONFIG_SEC_FACTORY)
	vdd_data.is_factory_mode = true;
#endif

	LCD_ERR("--\n");
}

void mdss_samsung_dsi_panel_registered(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)pdata->panel_private;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	if (IS_ERR_OR_NULL(ctrl) || IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return;
	}

	if ((struct msm_fb_data_type *)registered_fb[ctrl->ndx]) {
		vdd->mfd_dsi[ctrl->ndx] = (struct msm_fb_data_type *)registered_fb[ctrl->ndx]->par;
	} else {
		LCD_ERR("no registered_fb[%d]\n", ctrl->ndx);
		return;
	}

	mdss_samsung_create_sysfs(vdd);
	LCD_INFO("DSI%d success", ctrl->ndx);
}

int mdss_samsung_parse_candella_mapping_table(struct device_node *np,
		struct candela_map_table *table, char *keystring)
{
	const __be32 *data;
	int len = 0, i = 0, data_offset = 0;
	int col_size = 0;

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_DEBUG("%d, Unable to read table %s ", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_ERR("Success to read table %s\n", keystring);

	col_size = 4;

	if ((len % col_size) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s , len : %d",
					__LINE__, keystring, len);
		return -EINVAL;
	}

	table->tab_size = len / (sizeof(int) * col_size);

	table->cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->cd)
		goto error;
	table->idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->idx)
		goto error;
	table->from = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->from)
		goto error;
	table->end = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->end)
		goto error;

	for (i = 0 ; i < table->tab_size; i++) {
		table->idx[i] = be32_to_cpup(&data[data_offset++]);		/* field => <idx> */
		table->from[i] = be32_to_cpup(&data[data_offset++]);	/* field => <from> */
		table->end[i] = be32_to_cpup(&data[data_offset++]);		/* field => <end> */
		table->cd[i] = be32_to_cpup(&data[data_offset++]);		/* field => <cd> */
	}

	table->min_lv = table->from[0];
	table->max_lv = table->end[table->tab_size-1];

	LCD_INFO("tab_size (%d), hbm min/max lv (%d/%d)\n", table->tab_size, table->min_lv, table->max_lv);

	return 0;

error:
	kfree(table->cd);
	kfree(table->idx);
	kfree(table->from);
	kfree(table->end);

	return -ENOMEM;
}

int mdss_samsung_parse_pac_candella_mapping_table(struct device_node *np,
		struct candela_map_table *table, char *keystring)
{
	const __be32 *data;
	int len = 0, i = 0, data_offset = 0;
	int col_size = 0;

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_DEBUG("%d, Unable to read table %s ", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_ERR("Success to read table %s\n", keystring);

	col_size = 6;

	if ((len % col_size) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s , len : %d",
					__LINE__, keystring, len);
		return -EINVAL;
	}

	table->tab_size = len / (sizeof(int) * col_size);

	table->interpolation_cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->interpolation_cd)
		return -ENOMEM;
	table->cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->cd)
		goto error;
	table->scaled_idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->scaled_idx)
		goto error;
	table->idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->idx)
		goto error;
	table->from = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->from)
		goto error;
	table->end = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->end)
		goto error;

	for (i = 0 ; i < table->tab_size; i++) {
		table->scaled_idx[i] = be32_to_cpup(&data[data_offset++]);	/* field => <scaeled idx> */
		table->idx[i] = be32_to_cpup(&data[data_offset++]);			/* field => <idx> */
		table->from[i] = be32_to_cpup(&data[data_offset++]);		/* field => <from> */
		table->end[i] = be32_to_cpup(&data[data_offset++]);			/* field => <end> */
		table->cd[i] = be32_to_cpup(&data[data_offset++]);			/* field => <cd> */
		table->interpolation_cd[i] = be32_to_cpup(&data[data_offset++]);	/* field => <interpolation cd> */
	}

	table->min_lv = table->from[0];
	table->max_lv = table->end[table->tab_size-1];

	LCD_INFO("tab_size (%d), hbm min/max lv (%d/%d)\n", table->tab_size, table->min_lv, table->max_lv);

	return 0;

error:
	kfree(table->interpolation_cd);
	kfree(table->cd);
	kfree(table->scaled_idx);
	kfree(table->idx);
	kfree(table->from);
	kfree(table->end);

	return -ENOMEM;
}

int mdss_samsung_parse_hbm_candella_mapping_table(struct device_node *np,
		struct hbm_candela_map_table *table, char *keystring)
{
	const __be32 *data;
	int data_offset = 0, len = 0, i = 0;

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_DEBUG("%d, Unable to read table %s ", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_ERR("Success to read table %s\n", keystring);

	if ((len % 4) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s",
					__LINE__, keystring);
		return -EINVAL;
	}

	table->tab_size = len / (sizeof(int)*5);

	table->cd = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->cd)
		return -ENOMEM;
	table->idx = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->idx)
		goto error;
	table->from = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->from)
		goto error;
	table->end = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->end)
		goto error;
	table->auto_level = kzalloc((sizeof(int) * table->tab_size), GFP_KERNEL);
	if (!table->auto_level)
		goto error;

	for (i = 0 ; i < table->tab_size; i++) {
		table->idx[i] = be32_to_cpup(&data[data_offset++]);		/* 1st field => <idx> */
		table->from[i] = be32_to_cpup(&data[data_offset++]);		/* 2nd field => <from> */
		table->end[i] = be32_to_cpup(&data[data_offset++]);			/* 3rd field => <end> */
		table->cd[i] = be32_to_cpup(&data[data_offset++]);		/* 4th field => <candella> */
		table->auto_level[i] = be32_to_cpup(&data[data_offset++]);  /* 5th field => <auto brightness level> */
	}

	table->hbm_min_lv = table->from[0];
	table->hbm_max_lv = table->end[table->tab_size-1];

	LCD_INFO("tab_size (%d), hbm min/max lv (%d/%d)\n", table->tab_size, table->hbm_min_lv, table->hbm_max_lv);

	return 0;
error:
	kfree(table->cd);
	kfree(table->idx);
	kfree(table->from);
	kfree(table->end);
	kfree(table->auto_level);

	return -ENOMEM;
}

int mdss_samsung_parse_panel_table(struct device_node *np,
		struct cmd_map *table, char *keystring)
{
	const __be32 *data;
	int  data_offset, len = 0, i = 0;

	data = of_get_property(np, keystring, &len);
	if (!data) {
		LCD_DEBUG("%d, Unable to read table %s\n", __LINE__, keystring);
		return -EINVAL;
	} else
		LCD_ERR("Success to read table %s\n", keystring);

	if ((len % 2) != 0) {
		LCD_ERR("%d, Incorrect table entries for %s",
					__LINE__, keystring);
		return -EINVAL;
	}

	table->size = len / (sizeof(int)*2);
	table->bl_level = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->bl_level)
		return -ENOMEM;

	table->cmd_idx = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
	if (!table->cmd_idx)
		goto error;

	data_offset = 0;
	for (i = 0 ; i < table->size; i++) {
		table->bl_level[i] = be32_to_cpup(&data[data_offset++]);
		table->cmd_idx[i] = be32_to_cpup(&data[data_offset++]);
	}

	return 0;
error:
	kfree(table->cmd_idx);

	return -ENOMEM;
}

static int samsung_nv_read(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_panel_cmds *cmds, unsigned char *destBuffer)
{
	int loop_limit = 0;
	int read_pos = 0;
	int read_count = 0;
	int show_cnt;
	int i, j;
	char show_buffer[256] = {0,};
	int show_buffer_pos = 0;
	int read_size = 0;
	int srcLength = 0;
	int startoffset = 0;
	int ndx = 0;
	int packet_size = 10; /* mipi limitation */
	struct samsung_display_driver_data *vdd = NULL;
	struct dsi_panel_cmds *read_pos_cmd = NULL;

	if (IS_ERR_OR_NULL(ctrl) || IS_ERR_OR_NULL(cmds)) {
		LCD_ERR("Invalid ctrl data\n");
		return -EINVAL;
	}

	vdd = (struct samsung_display_driver_data *)ctrl->panel_data.panel_private;
	ndx = display_ndx_check(ctrl);

	if (vdd->poc_operation) {
		srcLength = cmds->read_size[0];
		startoffset = read_pos = cmds->read_startoffset[0];
		show_buffer_pos += snprintf(show_buffer, sizeof(show_buffer), "read_reg : %X[%d] : ", cmds->cmds->payload[0], srcLength);
		loop_limit = (srcLength + packet_size - 1) / packet_size;

		show_cnt = 0;
		for (j = 0; j < loop_limit; j++) {
			read_size = ((srcLength - read_pos + startoffset) < packet_size) ?
			(srcLength - read_pos + startoffset) : packet_size;

#if 0
			mutex_lock(&vdd->vdd_mdss_direct_cmdlist_lock);
			mdss_samsung_send_cmd(ctrl, TX_POC_REG_READ_POS);
			mutex_lock(&vdd->vdd_lock);
			read_count = mdss_samsung_panel_cmd_read(ctrl, cmds, read_size);
			mutex_unlock(&vdd->vdd_lock);
			mutex_unlock(&vdd->vdd_mdss_direct_cmdlist_lock);
#else
			/*
				To reduce read time,
				TX_POC_REG_READ_POS packet is combined to last index of samsung,poc_read_tx_cmds_revA.
				It means TX_POC_REG_READ_POS packet is sent with single-transmission.

				vdd_mdss_direct_cmdlist_lock is also held at ss_poc_read function.
			*/
			mutex_lock(&vdd->vdd_lock);
			read_count = mdss_samsung_panel_cmd_read(ctrl, cmds, read_size);
			mutex_unlock(&vdd->vdd_lock);
#endif
			for (i = 0; i < read_count; i++, show_cnt++) {
				if (destBuffer != NULL && show_cnt < srcLength) {
						destBuffer[show_cnt] =
						ctrl->rx_buf.data[i];
				}
			}

			read_pos += read_count;

			if (read_pos-startoffset >= srcLength)
				break;
		}

		return read_pos-startoffset;
	} else {
		read_pos_cmd = get_panel_tx_cmds(ctrl, TX_REG_READ_POS);

		srcLength = cmds->read_size[0];
		startoffset = read_pos = cmds->read_startoffset[0];

		show_buffer_pos += snprintf(show_buffer, sizeof(show_buffer), "read_reg : %X[%d] : ", cmds->cmds->payload[0], srcLength);

		loop_limit = (srcLength + packet_size - 1) / packet_size;

		show_cnt = 0;
		for (j = 0; j < loop_limit; j++) {
			read_pos_cmd->cmds->payload[1] = read_pos;
			read_size = ((srcLength - read_pos + startoffset) < packet_size) ?
						(srcLength - read_pos + startoffset) : packet_size;
			mdss_samsung_send_cmd(ctrl, TX_REG_READ_POS);

			mutex_lock(&vdd->vdd_lock);
			read_count = mdss_samsung_panel_cmd_read(ctrl, cmds, read_size);
			mutex_unlock(&vdd->vdd_lock);

			for (i = 0; i < read_count; i++, show_cnt++) {
				show_buffer_pos += snprintf(show_buffer + show_buffer_pos, sizeof(show_buffer)-show_buffer_pos, "%02x ",
						ctrl->rx_buf.data[i]);
				if (destBuffer != NULL && show_cnt < srcLength) {
						destBuffer[show_cnt] =
						ctrl->rx_buf.data[i];
				}
			}

			show_buffer_pos += snprintf(show_buffer + show_buffer_pos, sizeof(show_buffer)-show_buffer_pos, ".");


			read_pos += read_count;

			if (read_pos-startoffset >= srcLength)
				break;
		}

		LCD_ERR("mdss DSI%d %s\n", ndx, show_buffer);

		return read_pos-startoffset;
	}
}

struct dsi_panel_cmds *get_panel_rx_cmds(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_rx_cmd_list cmd)
{
	int ndx;
	struct dsi_panel_cmds *cmds = NULL;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd))
		return NULL;

	ndx = display_ndx_check(ctrl);

	if (cmd <= RX_CMD_NULL || cmd >= RX_CMD_MAX) {
		LCD_ERR("unknown cmd(%d), MAX(%d)\n", cmd, RX_CMD_MAX);
	} else {
		LCD_DEBUG("cmd(%d) name(%s)\n", cmd, vdd->dtsi_data[ndx].panel_rx_cmd_list[cmd][vdd->panel_revision].name);
		cmds = &vdd->dtsi_data[ndx].panel_rx_cmd_list[cmd][vdd->panel_revision];
	}

	return cmds;
}

struct dsi_panel_cmds *get_panel_tx_cmds(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_tx_cmd_list cmd)
{
	int ndx;
	struct dsi_panel_cmds *cmds = NULL;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd))
		return NULL;

	ndx = display_ndx_check(ctrl);

	if (cmd <= TX_CMD_NULL || cmd >= TX_CMD_MAX) {
		LCD_ERR("unknown cmd(%d), MAX(%d)\n", cmd, TX_CMD_MAX);
	} else {
		LCD_DEBUG("cmd(%d) name(%s)\n", cmd, vdd->dtsi_data[ndx].panel_tx_cmd_list[cmd][vdd->panel_revision].name);
		cmds = &vdd->dtsi_data[ndx].panel_tx_cmd_list[cmd][vdd->panel_revision];
	}

	return cmds;
}

struct dsi_panel_cmds *mdss_samsung_cmds_select(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_tx_cmd_list cmd,
	u32 *flags)
{
	int ndx = DISPLAY_1;
	struct dsi_panel_cmds *cmds = NULL;
	struct mdss_panel_info *pinfo = NULL;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	u32 cmd_flags = CMD_REQ_COMMIT;

	if (IS_ERR_OR_NULL(vdd))
		return NULL;

	ndx = display_ndx_check(ctrl);
	pinfo = &(ctrl->panel_data.panel_info);

	/*
	 * Skip commands that are not related to ALPM
	 */
	if (pinfo->blank_state == MDSS_PANEL_BLANK_LOW_POWER) {
		switch (cmd) {
		case TX_LPM_ON:
		case TX_LPM_OFF:
		case TX_LPM_HZ_NONE:
		case TX_LPM_1HZ:
		case TX_LPM_2HZ:
		case TX_LPM_30HZ:
		case TX_LPM_AOD_ON:
		case TX_LPM_AOD_OFF:
		case TX_DISPLAY_ON:
		case TX_DISPLAY_OFF:
		case TX_LEVEL1_KEY_ENABLE:
		case TX_LEVEL1_KEY_DISABLE:
		case TX_LEVEL2_KEY_ENABLE:
		case TX_LEVEL2_KEY_DISABLE:
		case TX_ESD_RECOVERY_1:
		case TX_ESD_RECOVERY_2:
		case TX_MULTIRES_FHD_TO_WQHD:
		case TX_MULTIRES_HD_TO_WQHD:
		case TX_MULTIRES_FHD:
		case TX_MULTIRES_HD:
		case TX_MDNIE_TUNE:

		case TX_SELF_DISP_ON:
		case TX_SELF_DISP_OFF:
		case TX_SELF_MASK_SIDE_MEM_SET:
		case TX_SELF_MASK_ON:
		case TX_SELF_MASK_OFF:
		case TX_SELF_MASK_IMAGE:
		case TX_SELF_ICON_SIDE_MEM_SET:
		case TX_SELF_ICON_ON_GRID_ON:
		case TX_SELF_ICON_ON_GRID_OFF:
		case TX_SELF_ICON_OFF_GRID_ON:
		case TX_SELF_ICON_OFF_GRID_OFF:
		case TX_SELF_ICON_IMAGE:

		case TX_SELF_ICON_GRID_2C_SYNC_OFF:

		case TX_SELF_BRIGHTNESS_ICON_ON:
		case TX_SELF_BRIGHTNESS_ICON_OFF:

		case TX_SELF_ACLOCK_IMAGE:
		case TX_SELF_ACLOCK_SIDE_MEM_SET:
		case TX_SELF_ACLOCK_ON:
		case TX_SELF_ACLOCK_TIME_UPDATE:
		case TX_SELF_ACLOCK_ROTATION:
		case TX_SELF_ACLOCK_OFF:
		case TX_SELF_DCLOCK_IMAGE:
		case TX_SELF_DCLOCK_SIDE_MEM_SET:
		case TX_SELF_DCLOCK_ON:
		case TX_SELF_DCLOCK_TIME_UPDATE:
		case TX_SELF_DCLOCK_BLINKING_ON:
		case TX_SELF_DCLOCK_BLINKING_OFF:
		case TX_SELF_DCLOCK_OFF:

		case TX_SELF_CLOCK_2C_SYNC_OFF:
			break;
		default:
			LCD_INFO("Skip commands(%d) that are not related to ALPM\n", cmd);
			goto end;
		}
	}

	LCD_DEBUG("list(%d)\n", cmd);

	cmds = get_panel_tx_cmds(ctrl, cmd);

	if (vdd->samsung_hw_config == SINGLE_DSI) {
		if (cmd_flags & CMD_REQ_BROADCAST)
			cmd_flags ^= CMD_REQ_BROADCAST;
	}

	if (!IS_ERR_OR_NULL(flags)) {
		/* Because of performance issue, TPG is used for POC operation */
		if (cmd > TX_POC_CMD_START && cmd < TX_POC_CMD_END)
			*flags |= CMD_REQ_DMA_TPG;

		*flags |= cmd_flags;
	}
end:
	return cmds;
}

int mdss_samsung_send_cmd(struct mdss_dsi_ctrl_pdata *ctrl, enum mipi_samsung_tx_cmd_list cmd)
{
	struct mdss_panel_info *pinfo = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	struct dsi_panel_cmds *pcmds = NULL;
	u32 flags = CMD_REQ_COMMIT;

	if (!mdss_panel_attach_get(ctrl)) {
		LCD_ERR("mdss_panel_attach_get(%d) : %d\n",
				ctrl->ndx, mdss_panel_attach_get(ctrl));
		return -EAGAIN;
	}

	vdd = check_valid_ctrl(ctrl);
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null\n");
		return 0;
	}

	pinfo = &(ctrl->panel_data.panel_info);
	if ((pinfo->blank_state == MDSS_PANEL_BLANK_BLANK) || (vdd->panel_func.samsung_lvds_write_reg)) {
		LCD_ERR("blank stste is (%d), cmd (%d)\n", pinfo->blank_state, cmd);
		return 0;
	}

	/*
		Any dsi TX or RX operation is not permitted while poc operation.
		1. Whole POC related cmds use CMD_REQ_DMA_TPG.
		2. vdd->poc_operation is set at POC operation.
	*/

	if (!(flags & CMD_REQ_DMA_TPG))
		mutex_lock(&vdd->vdd_poc_operation_lock);

	mutex_lock(&vdd->vdd_blank_unblank_lock); /* To block blank & unblank operation while sending cmds */

#if 0
	/* To check registered FB */
	if (IS_ERR_OR_NULL(vdd->mfd_dsi[ctrl->ndx])) {
		/* Do not send any CMD data under FB_BLANK_POWERDOWN condition*/
		if (vdd->vdd_blank_mode[DISPLAY_1] == FB_BLANK_POWERDOWN) {
			mutex_unlock(&vdd->vdd_blank_unblank_lock); /* To block blank & unblank operation while sending cmds */
			LCD_ERR("fb blank is POWERDOWN mode (%d)\n", cmd);
			return 0;
		}
	} else {
		/* Do not send any CMD data under FB_BLANK_POWERDOWN condition*/
		if (vdd->vdd_blank_mode[ctrl->ndx] == FB_BLANK_POWERDOWN) {
			mutex_unlock(&vdd->vdd_blank_unblank_lock); /* To block blank & unblank operation while sending cmds*/
			LCD_ERR("fb blank is POWERDOWN mode. (%d)\n", cmd);
			return 0;
		}
	}
#endif

	mutex_lock(&vdd->vdd_lock);
	pcmds = mdss_samsung_cmds_select(ctrl, cmd, &flags);
	if (!IS_ERR_OR_NULL(pcmds) && !IS_ERR_OR_NULL(pcmds->cmds))
		mdss_dsi_panel_cmds_send(ctrl, pcmds, flags);
	else
		LCD_INFO("no panel_cmds..\n");
	mutex_unlock(&vdd->vdd_lock);

	mutex_unlock(&vdd->vdd_blank_unblank_lock); /* To block blank & unblank operation while sending cmds */

	/*
		Any dsi TX or RX operation is not permitted while poc operation.
		1. Whole POC related cmds use CMD_REQ_DMA_TPG.
		2. vdd->poc_operation is set at POC operation.
	*/
	if (!(flags & CMD_REQ_DMA_TPG))
		mutex_unlock(&vdd->vdd_poc_operation_lock);

	if (flags & CMD_REQ_BROADCAST)
		pr_debug("CMD_REQ_BROADCAST is set\n");

	return 0;
}

int mdss_samsung_read_nv_mem(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_panel_cmds *cmds, unsigned char *buffer, int level_key)
{
	int nv_read_cnt = 0;

	/* To block read operation at esd-recovery blank & unblank routine*/
	if (ctrl->panel_data.panel_info.panel_dead)
		return -EPERM;

	if (level_key & LEVEL1_KEY)
		mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_ENABLE);
	if (level_key & LEVEL2_KEY)
		mdss_samsung_send_cmd(ctrl, TX_LEVEL2_KEY_ENABLE);

	nv_read_cnt = samsung_nv_read(ctrl, cmds, buffer);

	if (level_key & LEVEL1_KEY)
		mdss_samsung_send_cmd(ctrl, TX_LEVEL1_KEY_DISABLE);
	if (level_key & LEVEL2_KEY)
		mdss_samsung_send_cmd(ctrl, TX_LEVEL2_KEY_DISABLE);

	return nv_read_cnt;
}

void mdss_samsung_panel_data_read(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_panel_cmds *cmds, char *buffer, int level_key)
{
	if (!ctrl) {
		LCD_ERR("Invalid ctrl data\n");
		return;
	}

	if (!mdss_panel_attach_get(ctrl)) {
		LCD_ERR("mdss_panel_attach_get(%d) : %d\n",
				ctrl->ndx, mdss_panel_attach_get(ctrl));
		return;
	}

	if (!cmds->cmd_cnt) {
		LCD_ERR("cmds_count is zero..\n");
		return;
	}

	mdss_samsung_read_nv_mem(ctrl, cmds, buffer, level_key);
}

static unsigned char mdss_samsung_manufacture_id(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_panel_cmds *cmds)
{
	char manufacture_id = 0;

	mdss_samsung_panel_data_read(ctrl, cmds, &manufacture_id, LEVEL1_KEY);

	return manufacture_id;
}

static int mdss_samsung_manufacture_id_full(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_panel_cmds *cmds)
{
	char manufacture_id[3];
	int ret = 0;

	mdss_samsung_panel_data_read(ctrl, cmds, manufacture_id, LEVEL1_KEY);

	ret |= (manufacture_id[0] << 16);
	ret |= (manufacture_id[1] << 8);
	ret |= (manufacture_id[2]);

	return ret;
}

int mdss_samsung_panel_on_pre(struct mdss_panel_data *pdata)
{
	int ndx;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)pdata->panel_private;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	if (IS_ERR_OR_NULL(ctrl) || IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	ndx = display_ndx_check(ctrl);

	vdd->display_status_dsi[ndx].disp_on_pre = 1;

	if (vdd->other_line_panel_work_cnt)
		vdd->other_line_panel_work_cnt = 0; /*stop open otherline dat file*/

#if !defined(CONFIG_SEC_FACTORY)
	/* LCD ID read every wake_up time incase of factory binary */
	if (vdd->dtsi_data[ndx].tft_common_support)
		return false;
#endif

	if (!mdss_panel_attach_get(ctrl)) {
		LCD_ERR("mdss_panel_attach_get(%d) : %d\n",
				ndx, mdss_panel_attach_get(ctrl));
		return false;
	}

	LCD_INFO("+: ndx=%d\n", ndx);

	if (unlikely(vdd->is_factory_mode) &&
			vdd->dtsi_data[ndx].samsung_support_factory_panel_swap) {
		/* LCD ID read every wake_up time incase of factory binary */
		vdd->manufacture_id_dsi[ndx] = PBA_ID;

		/* Factory Panel Swap*/
		vdd->manufacture_date_loaded_dsi[ndx] = 0;
		vdd->ddi_id_loaded_dsi[ndx] = 0;
		vdd->cell_id_loaded_dsi[ndx] = 0;
		vdd->hbm_loaded_dsi[ndx] = 0;
		vdd->mdnie_loaded_dsi[ndx] = 0;
		vdd->smart_dimming_loaded_dsi[ndx] = 0;
		vdd->smart_dimming_hmt_loaded_dsi[ndx] = 0;
	}

	if (vdd->manufacture_id_dsi[ndx] == PBA_ID) {
		/*
		*	At this time, panel revision it not selected.
		*	So last index(SUPPORT_PANEL_REVISION-1) used.
		*/
		vdd->panel_revision = SUPPORT_PANEL_REVISION-1;

		/*
		*	Some panel needs to update register at init time to read ID & MTP
		*	Such as, dual-dsi-control or sleep-out so on.
		*/

		if (!IS_ERR_OR_NULL(get_panel_rx_cmds(ctrl, RX_MANUFACTURE_ID)->cmds)) {
			vdd->manufacture_id_dsi[ndx] = mdss_samsung_manufacture_id_full(ctrl, get_panel_rx_cmds(ctrl, RX_MANUFACTURE_ID));
			LCD_INFO("DSI%d manufacture_id(04h)=0x%x\n", ndx, vdd->manufacture_id_dsi[ndx]);
		} else {
			if (!IS_ERR_OR_NULL(get_panel_tx_cmds(ctrl, TX_MANUFACTURE_ID_READ_PRE)->cmds)) {
				mutex_lock(&vdd->vdd_lock);
				mdss_dsi_panel_cmds_send(ctrl, get_panel_tx_cmds(ctrl, TX_MANUFACTURE_ID_READ_PRE), CMD_REQ_COMMIT);
				mutex_unlock(&vdd->vdd_lock);
				LCD_ERR("DSI%d manufacture_read_pre_tx_cmds ", ndx);
			}

			if (!IS_ERR_OR_NULL(get_panel_rx_cmds(ctrl, RX_MANUFACTURE_ID0)->cmds)) {
				vdd->manufacture_id_dsi[ndx] = mdss_samsung_manufacture_id(ctrl, get_panel_rx_cmds(ctrl, RX_MANUFACTURE_ID0));
				vdd->manufacture_id_dsi[ndx] <<= 8;
			} else
				LCD_ERR("DSI%d manufacture_id0_rx_cmds NULL", ndx);

			if (!IS_ERR_OR_NULL(get_panel_rx_cmds(ctrl, RX_MANUFACTURE_ID1)->cmds)) {
				vdd->manufacture_id_dsi[ndx] |= mdss_samsung_manufacture_id(ctrl, get_panel_rx_cmds(ctrl, RX_MANUFACTURE_ID1));
				vdd->manufacture_id_dsi[ndx] <<= 8;
			} else
				LCD_ERR("DSI%d manufacture_id1_rx_cmds NULL", ndx);

			if (!IS_ERR_OR_NULL(get_panel_rx_cmds(ctrl, RX_MANUFACTURE_ID2)->cmds))
				vdd->manufacture_id_dsi[ndx] |= mdss_samsung_manufacture_id(ctrl, get_panel_rx_cmds(ctrl, RX_MANUFACTURE_ID2));
			else
				LCD_ERR("DSI%d manufacture_id2_rx_cmds NULL", ndx);

			LCD_INFO("DSI%d manufacture_id=0x%x\n", ndx, vdd->manufacture_id_dsi[ndx]);
		}

		/* Panel revision selection */
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_revision))
			LCD_ERR("DSI%d no panel_revision_selection_error fucntion\n", ndx);
		else
			vdd->panel_func.samsung_panel_revision(ctrl);

		LCD_INFO("DSI%d Panel_Revision = %c %d\n", ndx, vdd->panel_revision + 'A', vdd->panel_revision);
	}

	/* Manufacture date */
	if (!vdd->manufacture_date_loaded_dsi[ndx]) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_manufacture_date_read))
			LCD_ERR("DSI%d no samsung_manufacture_date_read function\n", ndx);
		else
			vdd->manufacture_date_loaded_dsi[ndx] = vdd->panel_func.samsung_manufacture_date_read(ctrl);
	}

	/* DDI ID */
	if (!vdd->ddi_id_loaded_dsi[ndx]) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_ddi_id_read))
			LCD_ERR("DSI%d no samsung_ddi_id_read function\n", ndx);
		else
			vdd->ddi_id_loaded_dsi[ndx] = vdd->panel_func.samsung_ddi_id_read(ctrl);
	}

	/* Panel Unique OCTA ID */
	if (!vdd->octa_id_loaded_dsi[ndx]) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_octa_id_read))
			LCD_ERR("DSI%d no samsung_octa_id_read function\n", ndx);
		else
			vdd->octa_id_loaded_dsi[ndx] = vdd->panel_func.samsung_octa_id_read(ctrl);
	}

	/* ELVSS read */
	if (!vdd->elvss_loaded_dsi[ndx]) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_elvss_read))
			LCD_ERR("DSI%d no samsung_elvss_read function\n", ndx);
		else
			vdd->elvss_loaded_dsi[ndx] = vdd->panel_func.samsung_elvss_read(ctrl);
	}

	/* IRC read */
	if (!vdd->irc_loaded_dsi[ndx]) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_irc_read))
			LCD_ERR("DSI%d no samsung_irc_read function\n", ndx);
		else
			vdd->irc_loaded_dsi[ndx] = vdd->panel_func.samsung_irc_read(ctrl);
	}

	/* HBM */
	if (!vdd->hbm_loaded_dsi[ndx]) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_read))
			LCD_ERR("DSI%d no samsung_hbm_read function\n", ndx);
		else
			vdd->hbm_loaded_dsi[ndx] = vdd->panel_func.samsung_hbm_read(ctrl);
	}

	/* MDNIE X,Y */
	if (!vdd->mdnie_loaded_dsi[ndx]) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_mdnie_read))
			LCD_ERR("DSI%d no samsung_mdnie_read function\n", ndx);
		else
			vdd->mdnie_loaded_dsi[ndx] = vdd->panel_func.samsung_mdnie_read(ctrl);
	}

	/* Panel Unique Cell ID */
	if (!vdd->cell_id_loaded_dsi[ndx]) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_cell_id_read))
			LCD_ERR("DSI%d no samsung_cell_id_read function\n", ndx);
		else
			vdd->cell_id_loaded_dsi[ndx] = vdd->panel_func.samsung_cell_id_read(ctrl);
	}

	/* Smart dimming*/
	if (!vdd->smart_dimming_loaded_dsi[ndx]) {
		if (IS_ERR_OR_NULL(vdd->panel_func.samsung_smart_dimming_init))
			LCD_ERR("DSI%d no samsung_smart_dimming_init function\n", ndx);
		else
			vdd->smart_dimming_loaded_dsi[ndx] = vdd->panel_func.samsung_smart_dimming_init(ctrl);
	}

	/* Smart dimming for hmt */
	if (vdd->dtsi_data[0].hmt_enabled) {
		if (!vdd->smart_dimming_hmt_loaded_dsi[ndx]) {
			if (IS_ERR_OR_NULL(vdd->panel_func.samsung_smart_dimming_hmt_init))
				LCD_ERR("DSI%d no samsung_smart_dimming_hmt_init function\n", ndx);
			else
				vdd->smart_dimming_hmt_loaded_dsi[ndx] = vdd->panel_func.samsung_smart_dimming_hmt_init(ctrl);
		}
	}

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_on_pre))
		vdd->panel_func.samsung_panel_on_pre(ctrl);

	LCD_INFO("-: ndx=%d\n", ndx);

	return true;
}

int mdss_samsung_panel_on_post(struct mdss_panel_data *pdata)
{
	int ndx;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	struct mdss_panel_info *pinfo = NULL;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	if (!mdss_panel_attach_get(ctrl)) {
		LCD_ERR("mdss_panel_attach_get(%d) : %d\n",
				ctrl->ndx, mdss_panel_attach_get(ctrl));
		return false;
	}

	pinfo = &(ctrl->panel_data.panel_info);

	if (IS_ERR_OR_NULL(pinfo)) {
		LCD_ERR("Invalid data pinfo : 0x%zx\n",  (size_t)pinfo);
		return false;
	}

	ndx = display_ndx_check(ctrl);

	LCD_INFO("+: ndx=%d\n", ndx);

#ifdef CONFIG_DISPLAY_USE_INFO
	mdss_samsung_read_self_diag(vdd);
#endif

	if (vdd->support_cabc && !vdd->auto_brightness)
		mdss_samsung_cabc_update();
	else if (vdd->mdss_panel_tft_outdoormode_update && vdd->auto_brightness)
		vdd->mdss_panel_tft_outdoormode_update(ctrl);
	else if (vdd->support_cabc && vdd->auto_brightness)
		mdss_tft_autobrightness_cabc_update(ctrl);

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_on_post))
		vdd->panel_func.samsung_panel_on_post(ctrl);

	if ((vdd->panel_func.color_weakness_ccb_on_off) && vdd->color_weakness_mode)
		vdd->panel_func.color_weakness_ccb_on_off(vdd, vdd->color_weakness_mode);

	mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);

	if (vdd->support_hall_ic) {
		/*
		* Brightenss cmds sent by samsung_display_hall_ic_status() at panel switching operation
		*/
		if ((vdd->ctrl_dsi[DISPLAY_1]->bklt_ctrl == BL_DCS_CMD) &&
			(vdd->display_status_dsi[DISPLAY_1].hall_ic_mode_change_trigger == false))
			mdss_samsung_brightness_dcs(ctrl, vdd->bl_level);
	} else {
		if ((vdd->ctrl_dsi[DISPLAY_1]->bklt_ctrl == BL_DCS_CMD))
			mdss_samsung_brightness_dcs(ctrl, vdd->bl_level);
	}

	mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);

	/*
	 * Update Cover Control Status every Normal sleep & wakeup
	 * Do not update Cover_control at this point in case of AOD.
	 * Because, below update is done before entering AOD.
	 */
	if (vdd->panel_func.samsung_cover_control && vdd->cover_control
		&& vdd->vdd_blank_mode[DISPLAY_1] == FB_BLANK_UNBLANK)
		vdd->panel_func.samsung_cover_control(ctrl, vdd);

	vdd->display_status_dsi[ndx].wait_disp_on = true;
	vdd->display_status_dsi[ndx].wait_actual_disp_on = true;
	vdd->display_status_dsi[ndx].aod_delay = true;

	if (pinfo->esd_check_enabled) {
		if (vdd->esd_recovery.esd_irq_enable)
			vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd);
		vdd->esd_recovery.is_enabled_esd_recovery = true;
	}

	LCD_INFO("-: ndx=%d\n", ndx);

	return true;
}

int mdss_samsung_panel_off_pre(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)pdata->panel_private;
	struct mdss_panel_info *pinfo = NULL;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	if (IS_ERR_OR_NULL(ctrl) || IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n",
				(size_t)ctrl, (size_t)vdd);
		return false;
	}

	pinfo = &(ctrl->panel_data.panel_info);

	if (IS_ERR_OR_NULL(pinfo)) {
		LCD_ERR("Invalid data pinfo : 0x%zx\n",  (size_t)pinfo);
		return false;
	}

#ifdef CONFIG_DISPLAY_USE_INFO
	mdss_samsung_read_rddpm(vdd);
	mdss_samsung_read_errfg(vdd);
	mdss_samsung_read_dsierr(vdd);
#endif

	if (pinfo->esd_check_enabled) {
		vdd->esd_recovery.is_wakeup_source = false;
		if (vdd->esd_recovery.esd_irq_enable)
			vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd);
		vdd->esd_recovery.is_enabled_esd_recovery = false;
	}

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_off_pre))
		vdd->panel_func.samsung_panel_off_pre(ctrl);

	return ret;
}

int mdss_samsung_panel_off_post(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)pdata->panel_private;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	if (IS_ERR_OR_NULL(ctrl) || IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n",
				(size_t)ctrl, (size_t)vdd);
		return false;
	}

	if (vdd->support_mdnie_trans_dimming)
		vdd->mdnie_disable_trans_dimming = true;

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_panel_off_post))
		vdd->panel_func.samsung_panel_off_post(ctrl);

	/* gradual acl on/off */
	vdd->gradual_pre_acl_on = GRADUAL_ACL_UNSTABLE;

	return ret;
}

/*************************************************************
*
*		EXTRA POWER RELATED FUNCTION BELOW.
*
**************************************************************/
static int mdss_dsi_extra_power_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int rc = 0, i;
	/*
	 * gpio_name[] named as gpio_name + num(recomend as 0)
	 * because of the num will increase depend on number of gpio
	 */
	static const char gpio_name[] = "panel_extra_power";
	static u8 gpio_request_status = -EINVAL;
	struct samsung_display_driver_data *vdd = NULL;

	if (!gpio_request_status)
		goto end;

	if (!IS_ERR_OR_NULL(ctrl))
		vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data pinfo : 0x%zx\n",  (size_t)vdd);
		goto end;
	}

	for (i = 0; i < MAX_EXTRA_POWER_GPIO; i++) {
		if (gpio_is_valid(vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i]) && mdss_panel_attached(ctrl->ndx)) {
			rc = gpio_request(vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i],
							gpio_name);
			if (rc) {
				LCD_ERR("request %s failed, rc=%d\n", gpio_name, rc);
				goto extra_power_gpio_err;
			}
		}
	}

	gpio_request_status = rc;
end:
	return rc;
extra_power_gpio_err:
	if (i) {
		do {
			if (gpio_is_valid(vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i]))
				gpio_free(vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i--]);
			LCD_ERR("i = %d\n", i);
		} while (i > 0);
	}

	return rc;
}

int mdss_samsung_panel_extra_power(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	int ret = 0, i = 0, add_value = 1;
	struct samsung_display_driver_data *vdd = NULL;

	if (IS_ERR_OR_NULL(pdata)) {
		LCD_ERR("Invalid pdata : 0x%zx\n", (size_t)pdata);
		ret = -EINVAL;
		goto end;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);

	if (!IS_ERR_OR_NULL(ctrl))
		vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data pinfo : 0x%zx\n",  (size_t)vdd);
		goto end;
	}

	LCD_INFO("++ enable(%d) ndx(%d)\n",
			enable, ctrl->ndx);

	if (ctrl->ndx == DSI_CTRL_1)
		goto end;

	if (mdss_dsi_extra_power_request_gpios(ctrl)) {
		LCD_ERR("fail to request extra power gpios");
		goto end;
	}

	LCD_DEBUG("%s extra power gpios\n", enable ? "enable" : "disable");

	/*
	 * The order of extra_power_gpio enable/disable
	 * 1. Enable : panel_extra_power_gpio[0], [1], ... [MAX_EXTRA_POWER_GPIO - 1]
	 * 2. Disable : panel_extra_power_gpio[MAX_EXTRA_POWER_GPIO - 1], ... [1], [0]
	 */
	if (!enable) {
		add_value = -1;
		i = MAX_EXTRA_POWER_GPIO - 1;
	}

	do {
		if (gpio_is_valid(vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i]) && mdss_panel_attached(ctrl->ndx)) {
			gpio_set_value(vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i], enable);
			LCD_DEBUG("set extra power gpio[%d] to %s\n",
						 vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i],
						enable ? "high" : "low");
			usleep_range(1500, 1500);
		}
	} while (((i += add_value) < MAX_EXTRA_POWER_GPIO) && (i >= 0));

end:
	LCD_INFO("--\n");
	return ret;
}
/*************************************************************
*
*		TFT BACKLIGHT GPIO FUNCTION BELOW.
*
**************************************************************/
int mdss_backlight_tft_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int rc = 0, i;
	/*
	 * gpio_name[] named as gpio_name + num(recomend as 0)
	 * because of the num will increase depend on number of gpio
	 */
	char gpio_name[17] = "disp_bcklt_gpio0";
	static u8 gpio_request_status = -EINVAL;
	struct samsung_display_driver_data *vdd = NULL;

	if (!gpio_request_status)
		goto end;

	if (!IS_ERR_OR_NULL(ctrl))
		vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data pinfo : 0x%zx\n", (size_t)vdd);
		goto end;
	}

	for (i = 0; i < MAX_BACKLIGHT_TFT_GPIO; i++) {
		if (gpio_is_valid(vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i])) {
			rc = gpio_request(vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i],
							gpio_name);
			if (rc) {
				LCD_ERR("request %s failed, rc=%d\n", gpio_name, rc);
				goto tft_backlight_gpio_err;
			}
		}
	}

	gpio_request_status = rc;
end:
	return rc;
tft_backlight_gpio_err:
	if (i) {
		do {
			if (gpio_is_valid(vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i]))
				gpio_free(vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i--]);
			LCD_ERR("i = %d\n", i);
		} while (i > 0);
	}

	return rc;
}
int mdss_backlight_tft_gpio_config(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	int ret = 0, i = 0, add_value = 1;
	struct samsung_display_driver_data *vdd = NULL;

	if (IS_ERR_OR_NULL(pdata)) {
		LCD_ERR("Invalid pdata : 0x%zx\n", (size_t)pdata);
		ret = -EINVAL;
		goto end;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);

	if (!IS_ERR_OR_NULL(ctrl))
		vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data pinfo : 0x%zx\n",  (size_t)vdd);
		goto end;
	}

	LCD_INFO("++ enable(%d) ndx(%d)\n",
			enable, ctrl->ndx);

	if (ctrl->ndx == DSI_CTRL_1)
		goto end;

	if (mdss_backlight_tft_request_gpios(ctrl)) {
		LCD_ERR("fail to request tft backlight gpios");
		goto end;
	}

	LCD_DEBUG("%s tft backlight gpios\n", enable ? "enable" : "disable");

	/*
	 * The order of backlight_tft_gpio enable/disable
	 * 1. Enable : backlight_tft_gpio[0], [1], ... [MAX_BACKLIGHT_TFT_GPIO - 1]
	 * 2. Disable : backlight_tft_gpio[MAX_BACKLIGHT_TFT_GPIO - 1], ... [1], [0]
	 */
	if (!enable) {
		add_value = -1;
		i = MAX_BACKLIGHT_TFT_GPIO - 1;
	}

	do {
		if (gpio_is_valid(vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i])) {
			gpio_set_value(vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i], enable);
			LCD_DEBUG("set backlight tft gpio[%d] to %s\n",
						 vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i],
						enable ? "high" : "low");
			usleep_range(500, 500);
		}
	} while (((i += add_value) < MAX_BACKLIGHT_TFT_GPIO) && (i >= 0));

end:
	LCD_INFO("--\n");
	return ret;
}

/*************************************************************
*
*		ESD RECOVERY RELATED FUNCTION BELOW.
*
**************************************************************/

static int mdss_samsung_esd_check_status(struct mdss_dsi_ctrl_pdata *ctrl)
{
	LCD_INFO("lcd esd - check_ststus\n");
	return 0;
}
static int mdss_samsung_esd_read_status(struct mdss_dsi_ctrl_pdata *ctrl)
{
	LCD_INFO("lcd esd - check_read_status\n");
	/* esd status must return 0 to go status_dead(blank->unblnk) in mdss_check_dsi_ctrl_status.*/
	return 0;
}

/*
 * esd_irq_enable() - Enable or disable esd irq.
 *
 * @enable	: flag for enable or disabled
 * @nosync	: flag for disable irq with nosync
 * @data	: point ot struct mdss_panel_info
 */
static void esd_irq_enable(bool enable, bool nosync, void *data)
{
	/* The irq will enabled when do the request_threaded_irq() */
	static bool is_enabled = true;
	static bool is_wakeup_source;
	int gpio;
	unsigned long flags;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)data;
	u8 i = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return;
	}

	spin_lock_irqsave(&vdd->esd_recovery.irq_lock, flags);

	if (enable == is_enabled) {
		LCD_INFO("ESD irq already %s\n",
				enable ? "enabled" : "disabled");
		goto config_wakeup_source;
	}


	for (i = 0; i < vdd->esd_recovery.num_of_gpio; i++) {
		gpio = vdd->esd_recovery.esd_gpio[i];

		if (!gpio_is_valid(gpio)) {
			LCD_ERR("Invalid ESD_GPIO : %d\n", gpio);
			continue;
		}

		if (enable) {
			is_enabled = true;
			enable_irq(gpio_to_irq(gpio));
		} else {
			if (nosync)
				disable_irq_nosync(gpio_to_irq(gpio));
			else
				disable_irq(gpio_to_irq(gpio));
			is_enabled = false;
		}
	}

	/* TODO: Disable log if the esd function stable */
	LCD_DEBUG("ESD irq %s with %s\n",
				enable ? "enabled" : "disabled",
				nosync ? "nosync" : "sync");

config_wakeup_source:
	if (vdd->esd_recovery.is_wakeup_source == is_wakeup_source) {
		LCD_DEBUG("[ESD] IRQs are already irq_wake %s\n",
				is_wakeup_source ? "enabled" : "disabled");
		goto end;
	}

	for (i = 0; i < vdd->esd_recovery.num_of_gpio; i++) {
		gpio = vdd->esd_recovery.esd_gpio[i];

		if (!gpio_is_valid(gpio)) {
			LCD_ERR("Invalid ESD_GPIO : %d\n", gpio);
			continue;
		}

		is_wakeup_source =
			vdd->esd_recovery.is_wakeup_source;

		if (is_wakeup_source)
			enable_irq_wake(gpio_to_irq(gpio));
		else
			disable_irq_wake(gpio_to_irq(gpio));
	}

	LCD_DEBUG("[ESD] IRQs are set to irq_wake %s\n",
				is_wakeup_source ? "enabled" : "disabled");

end:
	spin_unlock_irqrestore(&vdd->esd_recovery.irq_lock, flags);

}

static irqreturn_t esd_irq_handler(int irq, void *handle)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	struct mdss_panel_info *pinfo;

	if (!handle) {
		LCD_INFO("handle is null\n");
		return IRQ_HANDLED;
	}

	ctrl = (struct mdss_dsi_ctrl_pdata *)handle;

	if (!IS_ERR_OR_NULL(ctrl))
		vdd = check_valid_ctrl(ctrl);


	pinfo = &ctrl->panel_data.panel_info;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		goto end;
	}

	if (!vdd->esd_recovery.is_enabled_esd_recovery) {
		LCD_DEBUG("esd recovery is not enabled yet");
		goto end;
	}
	LCD_INFO("++\n");

	esd_irq_enable(false, true, (void *)vdd);

	vdd->panel_lpm.esd_recovery = true;

	if (!(pinfo->blank_state == MDSS_PANEL_BLANK_LOW_POWER)) {
		schedule_work(&pstatus_data->check_status.work);
	} else {
		/* schedule_work(&pstatus_data->check_status.work); */
		mdss_fb_report_panel_dead(pstatus_data->mfd);
	}
	LCD_INFO("--\n");

end:
	return IRQ_HANDLED;
}

void mdss_samsung_panel_parse_dt_esd(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl)
{
	int rc = 0;
	const char *data;
	char esd_irq_gpio[] = "samsung,esd-irq-gpio1";
	char esd_irqflag[] = "qcom,mdss-dsi-panel-status-irq-trigger1";
	struct samsung_display_driver_data *vdd = NULL;
	struct esd_recovery *esd = NULL;
	struct mdss_panel_info *pinfo = NULL;
	u8 i = 0;

	if (!ctrl) {
		LCD_ERR("ctrl is null\n");
		return;
	}

	vdd = check_valid_ctrl(ctrl);
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return;
	}

	pinfo = &ctrl->panel_data.panel_info;
	esd = &vdd->esd_recovery;
	esd->num_of_gpio = 0;

	pinfo->esd_check_enabled = of_property_read_bool(np,
		"qcom,esd-check-enabled");

	if (!pinfo->esd_check_enabled)
		goto end;

	for (i = 0; i < MAX_ESD_GPIO; i++) {
		esd_irq_gpio[strlen(esd_irq_gpio) - 1] = '1' + i;
		esd->esd_gpio[esd->num_of_gpio] = of_get_named_gpio(np,
				esd_irq_gpio, 0);

		if (gpio_is_valid(esd->esd_gpio[esd->num_of_gpio])) {
			LCD_INFO("[ESD] gpio : %d, irq : %d\n",
					esd->esd_gpio[esd->num_of_gpio],
					gpio_to_irq(esd->esd_gpio[esd->num_of_gpio]));
			esd->num_of_gpio++;
		}
	}

	rc = of_property_read_string(np, "qcom,mdss-dsi-panel-status-check-mode", &data);
	if (!rc) {
		if (!strcmp(data, "reg_read_irq")) {
			ctrl->status_mode = ESD_REG_IRQ;
			ctrl->check_status = mdss_samsung_esd_check_status;
			ctrl->check_read_status = mdss_samsung_esd_read_status;
		} else {
			LCD_ERR("No valid panel-status-check-mode string\n");
		}
	}

	for (i = 0; i < esd->num_of_gpio; i++) {
		esd_irqflag[strlen(esd_irqflag) - 1] = '1' + i;
		rc = of_property_read_string(np, esd_irqflag, &data);
		if (!rc) {
			esd->irqflags[i] =
				IRQF_ONESHOT | IRQF_NO_SUSPEND;

			if (!strcmp(data, "rising"))
				esd->irqflags[i] |= IRQF_TRIGGER_RISING;
			else if (!strcmp(data, "falling"))
				esd->irqflags[i] |= IRQF_TRIGGER_FALLING;
			else if (!strcmp(data, "high"))
				esd->irqflags[i] |= IRQF_TRIGGER_HIGH;
			else if (!strcmp(data, "low"))
				esd->irqflags[i] |= IRQF_TRIGGER_LOW;
		}
	}

end:
	LCD_INFO("samsung esd enable (%d) mode (%d) \n",
		pinfo->esd_check_enabled, ctrl->status_mode);

	return;
}

static void mdss_samsung_event_esd_recovery_init(struct mdss_panel_data *pdata, int event, void *arg)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	int ret;
	uint32_t *interval;
	uint32_t interval_ms_for_irq = 500;
	struct mdss_panel_info *pinfo;
	u8 i;
	int gpio, irqflags;
	struct esd_recovery *esd = NULL;

	if (IS_ERR_OR_NULL(pdata)) {
		LCD_ERR("Invalid pdata : 0x%zx\n", (size_t)pdata);
		return;
	}

	vdd = (struct samsung_display_driver_data *)pdata->panel_private;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	esd = &vdd->esd_recovery;

	interval = arg;

	pinfo = &ctrl->panel_data.panel_info;

	if (IS_ERR_OR_NULL(ctrl) ||	IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n",
				(size_t)ctrl, (size_t)vdd);
		return;
	}
	if (unlikely(!esd->esd_recovery_init)) {
		esd->esd_recovery_init = true;
		esd->esd_irq_enable = esd_irq_enable;
		if (ctrl->status_mode == ESD_REG_IRQ) {
			for (i = 0; i < esd->num_of_gpio; i++) {
				/* Set gpio num and irqflags */
				gpio = esd->esd_gpio[i];
				irqflags = esd->irqflags[i];
				if (!gpio_is_valid(gpio)) {
					LCD_ERR("[ESD] Invalid GPIO : %d\n", gpio);
					continue;
				}

				gpio_request(gpio, "esd_recovery");
				ret = request_threaded_irq(
						gpio_to_irq(gpio),
						NULL,
						esd_irq_handler,
						irqflags,
						"esd_recovery",
						(void *)ctrl);
				if (ret)
					LCD_ERR("Failed to request_irq, gpio=%d ret=%d\n", gpio, ret);
			}
			esd_irq_enable(false, true, (void *)vdd);
			*interval = interval_ms_for_irq;
		}
	}
}


/*************************************************************
*
*		OSC TE FITTING RELATED FUNCTION BELOW.
*
**************************************************************/
static void mdss_samsung_event_osc_te_fitting(struct mdss_panel_data *pdata, int event, void *arg)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	struct osc_te_fitting_info *te_info = NULL;
	struct mdss_mdp_ctl *ctl = NULL;
	int ret, i, lut_count;

	if (IS_ERR_OR_NULL(pdata)) {
		LCD_ERR("Invalid pdata : 0x%zx\n", (size_t)pdata);
		return;
	}

	vdd = (struct samsung_display_driver_data *)pdata->panel_private;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	ctl = arg;

	if (IS_ERR_OR_NULL(ctrl) ||	IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(ctl)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx ctl : 0x%zx\n",
				(size_t)ctrl, (size_t)vdd, (size_t)ctl);
		return;
	}

	te_info = &vdd->te_fitting_info;

	if (IS_ERR_OR_NULL(te_info)) {
		LCD_ERR("Invalid te data : 0x%zx\n",
				(size_t)te_info);
		return;
	}

	if (pdata->panel_info.cont_splash_enabled) {
		LCD_ERR("cont splash enabled\n");
		return;
	}

	if (!ctl->vsync_handler.enabled) {
		LCD_DEBUG("vsync handler does not enabled yet\n");
		return;
	}

	te_info->status |= TE_FITTING_DONE;

	LCD_DEBUG("++\n");

	if (!(te_info->status & TE_FITTING_REQUEST_IRQ)) {
		te_info->status |= TE_FITTING_REQUEST_IRQ;

		ret = request_threaded_irq(
				gpio_to_irq(ctrl->disp_te_gpio),
				samsung_te_check_handler,
				NULL,
				IRQF_TRIGGER_FALLING,
				"VSYNC_GPIO",
				(void *)ctrl);
		if (ret)
			LCD_ERR("Failed to request_irq, ret=%d\n",
					ret);
		else
			disable_irq(gpio_to_irq(ctrl->disp_te_gpio));
		te_info->te_time =
			kzalloc(sizeof(long long) * te_info->sampling_rate, GFP_KERNEL);
		INIT_WORK(&te_info->work, samsung_te_check_done_work);
	}

	for (lut_count = 0; lut_count < OSC_TE_FITTING_LUT_MAX; lut_count++) {
		init_completion(&te_info->te_check_comp);
		te_info->status |= TE_CHECK_ENABLE;
		te_info->te_duration = 0;

		LCD_DEBUG("osc_te_fitting _irq : %d\n",
				gpio_to_irq(ctrl->disp_te_gpio));

		enable_irq(gpio_to_irq(ctrl->disp_te_gpio));
		ret = wait_for_completion_timeout(
				&te_info->te_check_comp, 1000);

		if (ret <= 0)
			LCD_ERR("timeout\n");

		for (i = 0; i < te_info->sampling_rate; i++) {
			te_info->te_duration +=
				(i != 0 ? (te_info->te_time[i] - te_info->te_time[i-1]) : 0);
			LCD_DEBUG("vsync time : %lld, sum : %lld\n",
					te_info->te_time[i], te_info->te_duration);
		}
		do_div(te_info->te_duration, te_info->sampling_rate - 1);
		LCD_INFO("ave vsync time : %lld\n",
				te_info->te_duration);
		te_info->status &= ~TE_CHECK_ENABLE;

		if (vdd->panel_func.samsung_osc_te_fitting)
			ret = vdd->panel_func.samsung_osc_te_fitting(ctrl);

		if (!ret)
			mdss_samsung_send_cmd(ctrl, TX_OSC_TE_FITTING);
		else
			break;
	}
	LCD_DEBUG("--\n");
}

static void samsung_te_check_done_work(struct work_struct *work)
{
	struct osc_te_fitting_info *te_info = NULL;

	te_info = container_of(work, struct osc_te_fitting_info, work);

	if (IS_ERR_OR_NULL(te_info)) {
		LCD_ERR("Invalid TE tuning data\n");
		return;
	}

	complete_all(&te_info->te_check_comp);
}

static irqreturn_t samsung_te_check_handler(int irq, void *handle)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct osc_te_fitting_info *te_info = NULL;
	static bool skip_first_te = true;
	static u8 count;

	if (skip_first_te) {
		skip_first_te = false;
		goto end;
	}

	if (IS_ERR_OR_NULL(handle)) {
		LCD_ERR("handle is null\n");
		goto end;
	}

	ctrl = (struct mdss_dsi_ctrl_pdata *)handle;

	vdd = (struct samsung_display_driver_data *)ctrl->panel_data.panel_private;

	te_info = &vdd->te_fitting_info;

	if (IS_ERR_OR_NULL(ctrl) || IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n",
				(size_t)ctrl, (size_t)vdd);
		return -EINVAL;
	}

	if (!(te_info->status & TE_CHECK_ENABLE))
		goto end;

	if (count < te_info->sampling_rate) {
		te_info->te_time[count++] =
			ktime_to_us(ktime_get());
	} else {
		disable_irq_nosync(gpio_to_irq(ctrl->disp_te_gpio));
		schedule_work(&te_info->work);
		skip_first_te = true;
		count = 0;
	}

end:
	return IRQ_HANDLED;
}

/*************************************************************
*
*		LDI FPS RELATED FUNCTION BELOW.
*
**************************************************************/
int ldi_fps(unsigned int input_fps)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	int rc = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return 0;
	}

	LCD_INFO("input_fps = %d\n", input_fps);

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..");
		return 0;
	}

	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_change_ldi_fps))
		rc = vdd->panel_func.samsung_change_ldi_fps(ctrl, input_fps);
	else {
		LCD_ERR("samsung_change_ldi_fps function is NULL\n");
		return 0;
	}

	if (rc)
		mdss_samsung_send_cmd(ctrl, TX_LDI_FPS_CHANGE);

	return rc;
}
EXPORT_SYMBOL(ldi_fps);

/*************************************************************
*
*		HMT RELATED FUNCTION BELOW.
*
**************************************************************/
int hmt_bright_update(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct mdss_panel_data *pdata;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	pdata = &vdd->ctrl_dsi[DSI_CTRL_0]->panel_data;

	if (vdd->hmt_stat.hmt_on) {
		mdss_samsung_brightness_dcs_hmt(ctrl, vdd->hmt_stat.hmt_bl_level);
	} else {
		mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
		pdata->set_backlight(pdata, vdd->bl_level);
		mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
		LCD_INFO("hmt off state!\n");
	}

	return 0;
}

int hmt_enable(struct mdss_dsi_ctrl_pdata *ctrl, struct samsung_display_driver_data *vdd)
{
	LCD_INFO("[HMT] HMT %s\n", vdd->hmt_stat.hmt_on ? "ON" : "OFF");

	if (vdd->hmt_stat.hmt_on) {
		mdss_samsung_send_cmd(ctrl, TX_HMT_ENABLE);
	} else {
		mdss_samsung_send_cmd(ctrl, TX_HMT_DISABLE);
	}

	return 0;
}

int hmt_reverse_update(struct mdss_dsi_ctrl_pdata *ctrl, int enable)
{
	LCD_INFO("[HMT] HMT %s\n", enable ? "REVERSE" : "FORWARD");

	if (enable)
		mdss_samsung_send_cmd(ctrl, TX_HMT_REVERSE);
	else
		mdss_samsung_send_cmd(ctrl, TX_HMT_FORWARD);

	return 0;
}

/************************************************************

		PANEL DTSI PARSE FUNCTION.

		--- NEVER DELETE OR CHANGE ORDER ---
		---JUST ADD ITEMS AT LAST---

	"samsung,display_on_tx_cmds_rev%c"
	"samsung,display_off_tx_cmds_rev%c"
	"samsung,level1_key_enable_tx_cmds_rev%c"
	"samsung,level1_key_disable_tx_cmds_rev%c"
	"samsung,hsync_on_tx_cmds_rev%c"
	"samsung,level2_key_enable_tx_cmds_rev%c"
	"samsung,level2_key_disable_tx_cmds_rev%c"
	"samsung,smart_dimming_mtp_rx_cmds_rev%c"
	"samsung,manufacture_read_pre_tx_cmds_rev%c"
	"samsung,manufacture_id0_rx_cmds_rev%c"
	"samsung,manufacture_id1_rx_cmds_rev%c"
	"samsung,manufacture_id2_rx_cmds_rev%c"
	"samsung,manufacture_date_rx_cmds_rev%c"
	"samsung,ddi_id_rx_cmds_rev%c"
	"samsung,rddpm_rx_cmds_rev%c"
	"samsung,mtp_read_sysfs_rx_cmds_rev%c"
	"samsung,vint_tx_cmds_rev%c"
	"samsung,vint_map_table_rev%c"
	"samsung,acl_off_tx_cmds_rev%c"
	"samsung,acl_map_table_rev%c"
	"samsung,candela_map_table_rev%c"
	"samsung,acl_percent_tx_cmds_rev%c"
	"samsung,acl_on_tx_cmds_rev%c"
	"samsung,gamma_tx_cmds_rev%c"
	"samsung,elvss_rx_cmds_rev%c"
	"samsung,elvss_tx_cmds_rev%c"
	"samsung,aid_map_table_rev%c"
	"samsung,aid_tx_cmds_rev%c"
	"samsung,hbm_rx_cmds_rev%c"
	"samsung,hbm2_rx_cmds_rev%c"
	"samsung,hbm_gamma_tx_cmds_rev%c"
	"samsung,hbm_etc_tx_cmds_rev%c"
	"samsung,hbm_off_tx_cmds_rev%c"
	"samsung,mdnie_read_rx_cmds_rev%c"
	"samsung,ldi_debug0_rx_cmds_rev%c"
	"samsung,ldi_debug1_rx_cmds_rev%c"
	"samsung,ldi_debug2_rx_cmds_rev%c"
	"samsung,elvss_lowtemp_tx_cmds_rev%c"
	"samsung,elvss_lowtemp2_tx_cmds_rev%c"
	"samsung,smart_acl_elvss_tx_cmds_rev%c"
	"samsung,smart_acl_elvss_map_table_rev%c"
	"samsung,partial_display_on_tx_cmds_rev%c"
	"samsung,partial_display_off_tx_cmds_rev%c"
	"samsung,partial_display_column_row_tx_cmds_rev%c"
	"samsung,alpm_on_tx_cmds_rev%c"
	"samsung,alpm_off_tx_cmds_rev%c"
	"samsung,lpm_2nit_tx_cmds_rev%c"
	"samsung,lpm_40nit_tx_cmds_rev%c"
	"samsung,lpm_ctrl_alpm_tx_cmds_rev%c"
	"samsung,lpm_ctrl_hlpm_tx_cmds_rev%c"
	"samsung,lpm_ctrl_alpm_2nit_tx_cmds_rev%c"
	"samsung,lpm_ctrl_alpm_40nit_tx_cmds_rev%c"
	"samsung,lpm_ctrl_hlpm_2nit_tx_cmds_rev%c"
	"samsung,lpm_ctrl_hlpm_40nit_tx_cmds_rev%c"
	"samsung,lpm_1hz_tx_cmds_rev%c"
	"samsung,lpm_2hz_tx_cmds_rev%c"
	"samsung,lpm_hz_none_tx_cmds_rev%c"
	"samsung,hmt_gamma_tx_cmds_rev%c"
	"samsung,hmt_elvss_tx_cmds_rev%c"
	"samsung,hmt_vint_tx_cmds_rev%c"
	"samsung,hmt_enable_tx_cmds_rev%c"
	"samsung,hmt_disable_tx_cmds_rev%c"
	"samsung,hmt_reverse_enable_tx_cmds_rev%c"
	"samsung,hmt_reverse_disable_tx_cmds_rev%c"
	"samsung,hmt_aid_tx_cmds_rev%c"
	"samsung,hmt_reverse_aid_map_table_rev%c"
	"samsung,hmt_150cd_rx_cmds_rev%c"
	"samsung,hmt_candela_map_table_rev%c"
	"samsung,ldi_fps_change_tx_cmds_rev%c"
	"samsung,ldi_fps_rx_cmds_rev%c"
	"samsung,tft_pwm_tx_cmds_rev%c"
	"samsung,scaled_level_map_table_rev%c"
	"samsung,packet_size_tx_cmds_rev%c"
	"samsung,reg_read_pos_tx_cmds_rev%c"
	"samsung,osc_te_fitting_tx_cmds_rev%c"
	"samsung,panel_ldi_vdd_read_cmds_rev%c"
	"samsung,panel_ldi_vddm_read_cmds_rev%c"
	"samsung,panel_ldi_vdd_offset_write_cmds_rev%c"
	"samsung,panel_ldi_vddm_offset_write_cmds_rev%c"
	"samsung,cabc_on_tx_cmds_rev%c"
	"samsung,cabc_off_tx_cmds_rev%c"
	"samsung,cabc_on_duty_tx_cmds_rev%c"
	"samsung,cabc_off_duty_tx_cmds_rev%c"
*************************************************************/
void parse_dt_data(struct device_node *np,
		void *data,	char *cmd_string, char panel_rev, void *fnc)
{
	char string[PARSE_STRING];
	int ret = 0;

	if (IS_ERR_OR_NULL(np) || IS_ERR_OR_NULL(data) || IS_ERR_OR_NULL(cmd_string)) {
		LCD_ERR("Invalid data, np : 0x%zx data : 0x%zx cmd_string : 0x%zx\n",
				(size_t)np, (size_t)data, (size_t)cmd_string);
		return;
	}

	/* Generate string to parsing from DT */
	snprintf(string, PARSE_STRING, "%s%c", cmd_string, 'A' + panel_rev);

	if (fnc == mdss_samsung_parse_panel_table) {
		ret = mdss_samsung_parse_panel_table(np,
				(struct cmd_map *)data, string);

		if (ret && (panel_rev > 0))
			memcpy(data, (struct cmd_map *)data - 1, sizeof(struct cmd_map));
	} else if (fnc == mdss_samsung_parse_candella_mapping_table) {
		ret = mdss_samsung_parse_candella_mapping_table(np,
			(struct candela_map_table *)data, string);

		if (ret && (panel_rev > 0))
			memcpy(data, (struct candela_map_table *)data - 1, sizeof(struct candela_map_table));
	} else if (fnc == mdss_samsung_parse_pac_candella_mapping_table) {
		ret = mdss_samsung_parse_pac_candella_mapping_table(np,
			(struct candela_map_table *)data, string);

		if (ret && (panel_rev > 0))
			memcpy(data, (struct candela_map_table *)data - 1, sizeof(struct candela_map_table));
	} else if (fnc == mdss_samsung_parse_hbm_candella_mapping_table) {
		ret = mdss_samsung_parse_hbm_candella_mapping_table(np,
			(struct hbm_candela_map_table *)data, string);

		if (ret && (panel_rev > 0))
			memcpy(data, (struct hbm_candela_map_table *)data - 1, sizeof(struct hbm_candela_map_table));
	} else if ((fnc == mdss_samsung_parse_dcs_cmds) || (fnc == NULL)) {
		ret = mdss_samsung_parse_dcs_cmds(np,
			(struct dsi_panel_cmds *)data, string, NULL);

		if (ret && (panel_rev > 0))
			memcpy(data, (struct dsi_panel_cmds *)data - 1, sizeof(struct dsi_panel_cmds));
	}
}

void mdss_samsung_panel_parse_dt_cmds(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl)
{
	int panel_rev;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	struct samsung_display_dtsi_data *dtsi_data = NULL;
	/* At this time ctrl->ndx is not set */
	int ndx = ctrl->panel_data.panel_info.pdest;
	static int parse_dt_cmds_cnt;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n",
				(size_t)ctrl, (size_t)vdd);
		return;
	}

	LCD_INFO("DSI%d", ndx);

	if (vdd->support_hall_ic) {
		if (!parse_dt_cmds_cnt) {
			ndx = DSI_CTRL_0;
			parse_dt_cmds_cnt = 1;
		} else
			ndx = DSI_CTRL_1;
	}

	dtsi_data = &vdd->dtsi_data[ndx];

	for (panel_rev = 0; panel_rev < SUPPORT_PANEL_REVISION; panel_rev++) {

		/* PANEL TX CMDs */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_DISPLAY_ON][panel_rev],
				"samsung,display_on_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_DISPLAY_OFF][panel_rev],
				"samsung,display_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_BRIGHT_CTRL][panel_rev],
				"samsung,brightness_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_MDNIE_TUNE][panel_rev],
				"samsung,mdnie_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ESD_RECOVERY_1][panel_rev],
				"samsung,esd_recovery_1_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ESD_RECOVERY_2][panel_rev],
				"samsung,esd_recovery_2_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LEVEL1_KEY_ENABLE][panel_rev],
				"samsung,level1_key_enable_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LEVEL1_KEY_DISABLE][panel_rev],
				"samsung,level1_key_disable_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HSYNC_ON][panel_rev],
				"samsung,hsync_on_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LEVEL2_KEY_ENABLE][panel_rev],
				"samsung,level2_key_enable_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LEVEL2_KEY_DISABLE][panel_rev],
				"samsung,level2_key_disable_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_SMART_DIM_MTP][panel_rev],
				"samsung,smart_dimming_mtp_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_MANUFACTURE_ID_READ_PRE][panel_rev],
				"samsung,manufacture_read_pre_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_MANUFACTURE_ID][panel_rev],
				"samsung,manufacture_id_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_MANUFACTURE_ID0][panel_rev],
				"samsung,manufacture_id0_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_MANUFACTURE_ID1][panel_rev],
				"samsung,manufacture_id1_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_MANUFACTURE_ID2][panel_rev],
				"samsung,manufacture_id2_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_MANUFACTURE_DATE][panel_rev],
				"samsung,manufacture_date_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_DDI_ID][panel_rev],
				"samsung,ddi_id_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_CELL_ID][panel_rev],
				"samsung,cell_id_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_OCTA_ID][panel_rev],
				"samsung,octa_id_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_RDDPM][panel_rev],
				"samsung,rddpm_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_MTP_READ_SYSFS][panel_rev],
				"samsung,mtp_read_sysfs_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_VINT][panel_rev],
				"samsung,vint_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->vint_map_table[panel_rev],
				"samsung,vint_map_table_rev", panel_rev,
				mdss_samsung_parse_panel_table); /* VINT TABLE */

		parse_dt_data(np, &dtsi_data->candela_map_table[panel_rev],
				"samsung,candela_map_table_rev", panel_rev,
				mdss_samsung_parse_candella_mapping_table);

		parse_dt_data(np, &dtsi_data->hbm_candela_map_table[panel_rev],
				"samsung,hbm_candela_map_table_rev", panel_rev,
				mdss_samsung_parse_hbm_candella_mapping_table);

		if (vdd->pac) {
			parse_dt_data(np, &dtsi_data->pac_candela_map_table[panel_rev],
					"samsung,pac_candela_map_table_rev", panel_rev,
					mdss_samsung_parse_pac_candella_mapping_table);

			parse_dt_data(np, &dtsi_data->pac_hbm_candela_map_table[panel_rev],
					"samsung,pac_hbm_candela_map_table_rev", panel_rev,
					mdss_samsung_parse_hbm_candella_mapping_table);
		}

		parse_dt_data(np, &dtsi_data->copr_br_map_table[panel_rev],
				"samsung,copr_br_table_rev", panel_rev,
				mdss_samsung_parse_panel_table); /* COPR BR TABLE */

		/* ACL */
		parse_dt_data(np, &dtsi_data->acl_map_table[panel_rev],
				"samsung,acl_map_table_rev", panel_rev,
				mdss_samsung_parse_panel_table); /* ACL TABLE */

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ACL_ON][panel_rev],
				"samsung,acl_on_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ACL_OFF][panel_rev],
				"samsung,acl_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_GAMMA][panel_rev],
				"samsung,gamma_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_IRC][panel_rev],
				"samsung,irc_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_ELVSS][panel_rev],
				"samsung,elvss_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ELVSS][panel_rev],
				"samsung,elvss_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ELVSS_HIGH][panel_rev],
				"samsung,elvss_high_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ELVSS_MID][panel_rev],
				"samsung,elvss_mid_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ELVSS_LOW][panel_rev],
				"samsung,elvss_low_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->elvss_map_table[panel_rev],
				"samsung,elvss_map_table_rev", panel_rev,
				mdss_samsung_parse_panel_table); /* ELVSS TABLE */

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ELVSS_PRE][panel_rev],
				"samsung,elvss_pre_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->aid_map_table[panel_rev],
				"samsung,aid_map_table_rev", panel_rev,
				mdss_samsung_parse_panel_table); /* AID TABLE */

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_AID][panel_rev],
				"samsung,aid_tx_cmds_rev", panel_rev, NULL);

		if (vdd->pac)
			parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_AID_SUBDIVISION][panel_rev],
					"samsung,pac_aid_subdivision_tx_cmds_rev", panel_rev, NULL);
		else
			parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_AID_SUBDIVISION][panel_rev],
					"samsung,aid_subdivision_tx_cmds_rev", panel_rev, NULL);

		/* CONFIG_HBM_RE */
		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_HBM][panel_rev],
				"samsung,hbm_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_HBM2][panel_rev],
				"samsung,hbm2_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HBM_GAMMA][panel_rev],
				"samsung,hbm_gamma_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HBM_ETC][panel_rev],
				"samsung,hbm_etc_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HBM_OFF][panel_rev],
				"samsung,hbm_off_tx_cmds_rev", panel_rev, NULL);

		/* CCB */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_COLOR_WEAKNESS_ENABLE][panel_rev],
				"samsung,ccb_on_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_COLOR_WEAKNESS_DISABLE][panel_rev],
				"samsung,ccb_off_tx_cmds_rev", panel_rev, NULL);

		/* IRC */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_IRC][panel_rev],
				"samsung,irc_tx_cmds_rev", panel_rev, NULL);

		if (vdd->pac)
			parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_IRC_SUBDIVISION][panel_rev],
					"samsung,pac_irc_subdivision_tx_cmds_rev", panel_rev, NULL);
		else
			parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_IRC_SUBDIVISION][panel_rev],
					"samsung,irc_subdivision_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HBM_IRC][panel_rev],
				"samsung,hbm_irc_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_IRC_OFF][panel_rev],
				"samsung,irc_off_tx_cmds_rev", panel_rev, NULL);

		/* MCD */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_MCD_ON][panel_rev],
				"samsung,mcd_on_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_MCD_OFF][panel_rev],
				"samsung,mcd_off_tx_cmds_rev", panel_rev, NULL);

		/* PoC */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_ERASE][panel_rev], "samsung,poc_erase_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_ERASE1][panel_rev], "samsung,poc_erase1_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_WRITE_1BYTE][panel_rev], "samsung,poc_write_1byte_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_PRE_WRITE][panel_rev], "samsung,poc_pre_write_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_WRITE_CONTINUE][panel_rev], "samsung,poc_write_continue_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_WRITE_CONTINUE2][panel_rev], "samsung,poc_write_continue2_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_WRITE_CONTINUE3][panel_rev], "samsung,poc_write_continue3_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_WRITE_END][panel_rev], "samsung,poc_write_end_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_POST_WRITE][panel_rev], "samsung,poc_post_write_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_PRE_READ][panel_rev], "samsung,poc_pre_read_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_READ][panel_rev], "samsung,poc_read_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_POST_READ][panel_rev], "samsung,poc_post_read_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_POC_REG_READ_POS][panel_rev], "samsung,reg_poc_read_pos_tx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_POC_READ][panel_rev], "samsung,poc_read_rx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_POC_STATUS][panel_rev], "samsung,poc_status_rx_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_POC_CHECKSUM][panel_rev], "samsung,poc_checksum_rx_cmds_rev", panel_rev, NULL);

		/* CONFIG_TCON_MDNIE_LITE */
		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_MDNIE][panel_rev],
				"samsung,mdnie_read_rx_cmds_rev", panel_rev, NULL);

		/* CONFIG_DEBUG_LDI_STATUS */
		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_DEBUG0][panel_rev],
				"samsung,ldi_debug0_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_DEBUG1][panel_rev],
				"samsung,ldi_debug1_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_DEBUG2][panel_rev],
				"samsung,ldi_debug2_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_DEBUG3][panel_rev],
				"samsung,ldi_debug3_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_DEBUG4][panel_rev],
				"samsung,ldi_debug4_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_DEBUG5][panel_rev],
				"samsung,ldi_debug5_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_ERROR][panel_rev],
				"samsung,ldi_error_rx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_LOADING_DET][panel_rev],
				"samsung,ldi_loading_det_rx_cmds_rev", panel_rev, NULL);

		/* CONFIG_TEMPERATURE_ELVSS */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ELVSS_LOWTEMP][panel_rev],
				"samsung,elvss_lowtemp_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ELVSS_LOWTEMP2][panel_rev],
				"samsung,elvss_lowtemp2_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SMART_ACL_ELVSS_LOWTEMP][panel_rev],
				"samsung,smart_acl_elvss_lowtemp_tx_cmds_rev", panel_rev, NULL);

		/* CONFIG_SMART_ACL */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SMART_ACL_ELVSS][panel_rev],
				"samsung,smart_acl_elvss_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->smart_acl_elvss_map_table[panel_rev],
				"samsung,smart_acl_elvss_map_table_rev", panel_rev,
				mdss_samsung_parse_panel_table); /* TABLE */

		/* CONFIG_ALPM_MODE */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_ON][panel_rev],
				"samsung,alpm_on_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_OFF][panel_rev],
				"samsung,alpm_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_2NIT_CMD][panel_rev],
				"samsung,lpm_2nit_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_40NIT_CMD][panel_rev],
				"samsung,lpm_40nit_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_60NIT_CMD][panel_rev],
				"samsung,lpm_60nit_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ALPM_2NIT_CMD][panel_rev],
				"samsung,lpm_ctrl_alpm_2nit_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ALPM_40NIT_CMD][panel_rev],
				"samsung,lpm_ctrl_alpm_40nit_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ALPM_60NIT_CMD][panel_rev],
				"samsung,lpm_ctrl_alpm_60nit_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HLPM_2NIT_CMD][panel_rev],
				"samsung,lpm_ctrl_hlpm_2nit_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HLPM_40NIT_CMD][panel_rev],
				"samsung,lpm_ctrl_hlpm_40nit_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HLPM_60NIT_CMD][panel_rev],
				"samsung,lpm_ctrl_hlpm_60nit_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ALPM_2NIT_OFF][panel_rev],
				"samsung,lpm_ctrl_alpm_2nit_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ALPM_40NIT_OFF][panel_rev],
				"samsung,lpm_ctrl_alpm_40nit_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_ALPM_60NIT_OFF][panel_rev],
				"samsung,lpm_ctrl_alpm_60nit_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HLPM_2NIT_OFF][panel_rev],
				"samsung,lpm_ctrl_hlpm_2nit_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HLPM_40NIT_OFF][panel_rev],
				"samsung,lpm_ctrl_hlpm_40nit_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HLPM_60NIT_OFF][panel_rev],
				"samsung,lpm_ctrl_hlpm_60nit_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_1HZ][panel_rev],
				"samsung,lpm_1hz_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_2HZ][panel_rev],
				"samsung,lpm_2hz_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_HZ_NONE][panel_rev],
				"samsung,lpm_hz_none_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_AOD_ON][panel_rev],
				"samsung,lpm_ctrl_alpm_aod_on_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LPM_AOD_OFF][panel_rev],
				"samsung,lpm_ctrl_alpm_aod_off_tx_cmds_rev", panel_rev, NULL);

		/* HMT */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HMT_GAMMA][panel_rev],
				"samsung,hmt_gamma_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HMT_ELVSS][panel_rev],
				"samsung,hmt_elvss_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HMT_VINT][panel_rev],
				"samsung,hmt_vint_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HMT_IRC][panel_rev],
				"samsung,hmt_irc_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HMT_ENABLE][panel_rev],
				"samsung,hmt_enable_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HMT_DISABLE][panel_rev],
				"samsung,hmt_disable_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HMT_REVERSE][panel_rev],
				"samsung,hmt_reverse_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HMT_FORWARD][panel_rev],
				"samsung,hmt_forward_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HMT_AID][panel_rev],
				"samsung,hmt_aid_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->hmt_reverse_aid_map_table[panel_rev],
				"samsung,hmt_reverse_aid_map_table_rev", panel_rev,
				mdss_samsung_parse_panel_table); /* TABLE */

		parse_dt_data(np, &dtsi_data->hmt_candela_map_table[panel_rev],
				"samsung,hmt_candela_map_table_rev", panel_rev,
				mdss_samsung_parse_candella_mapping_table);

		/* CONFIG_FPS_CHANGE */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LDI_FPS_CHANGE][panel_rev],
				"samsung,ldi_fps_change_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_rx_cmd_list[RX_LDI_FPS][panel_rev],
				"samsung,ldi_fps_rx_cmds_rev", panel_rev, NULL);

		/* TFT PWM CONTROL */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_TFT_PWM][panel_rev],
				"samsung,tft_pwm_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_BLIC_DIMMING][panel_rev],
				"samsung,blic_dimming_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->scaled_level_map_table[panel_rev],
				"samsung,scaled_level_map_table_rev", panel_rev,
				mdss_samsung_parse_candella_mapping_table);

		/* Additional Command */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_PACKET_SIZE][panel_rev],
				"samsung,packet_size_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_REG_READ_POS][panel_rev],
				"samsung,reg_read_pos_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_OSC_TE_FITTING][panel_rev],
				"samsung,osc_te_fitting_tx_cmds_rev", panel_rev, NULL);

		/* samsung,avc_on */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_AVC_ON][panel_rev],
				"samsung,avc_on_rev", panel_rev, NULL);

#if 0
		/* VDDM OFFSET */
		parse_dt_data(np, &dtsi_data->read_vdd_ref_cmds[panel_rev],
				"samsung,panel_ldi_vdd_read_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->read_vddm_ref_cmds[panel_rev],
				"samsung,panel_ldi_vddm_read_cmds_rev", panel_rev, NULL);
#endif

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LDI_SET_VDD_OFFSET][panel_rev],
				"samsung,panel_ldi_vdd_offset_write_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_LDI_SET_VDDM_OFFSET][panel_rev],
				"samsung,panel_ldi_vddm_offset_write_cmds_rev", panel_rev, NULL);

		/* TFT CABC CONTROL */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_CABC_ON][panel_rev],
				"samsung,cabc_on_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_CABC_OFF][panel_rev],
				"samsung,cabc_off_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_CABC_ON_DUTY][panel_rev],
				"samsung,cabc_on_duty_tx_cmds_rev", panel_rev, NULL);

		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_CABC_OFF_DUTY][panel_rev],
				"samsung,cabc_off_duty_tx_cmds_rev", panel_rev, NULL);

		/* SPI ENABLE */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_SPI_ENABLE][panel_rev],
				"samsung,spi_enable_tx_cmds_rev", panel_rev, NULL);

		/* GRADUAL ACL ON/OFF */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_GRADUAL_ACL][panel_rev],
				"samsung,gradual_acl_tx_cmds_rev", panel_rev, NULL);

		/* H/W CURSOR */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_HW_CURSOR][panel_rev],
				"samsung,hw_cursor_tx_cmds_rev", panel_rev, NULL);

		/* MULTI_RESOLUTION */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_MULTIRES_FHD_TO_WQHD][panel_rev],
				"samsung,panel_multires_fhd_to_wqhd_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_MULTIRES_HD_TO_WQHD][panel_rev],
				"samsung,panel_multires_hd_to_wqhd_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_MULTIRES_FHD][panel_rev],
				"samsung,panel_multires_fhd_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_MULTIRES_HD][panel_rev],
				"samsung,panel_multires_hd_rev", panel_rev, NULL);

		/* PANEL COVER CONTROL */
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_COVER_CONTROL_ENABLE][panel_rev],
				"samsung,panel_cover_control_enable_cmds_rev", panel_rev, NULL);
		parse_dt_data(np, &dtsi_data->panel_tx_cmd_list[TX_COVER_CONTROL_DISABLE][panel_rev],
				"samsung,panel_cover_control_disable_cmds_rev", panel_rev, NULL);

		/* SELF DISPLAY */
		ss_self_display_parse_dt_cmds(np, ctrl, panel_rev, dtsi_data);
	}
}

void mdss_samsung_check_hw_config(struct platform_device *pdev)
{
	struct mdss_dsi_data *dsi_res = platform_get_drvdata(pdev);
	struct dsi_shared_data *sdata;
	struct samsung_display_driver_data *vdd;

	if (!dsi_res)
		return;

	vdd = check_valid_ctrl(dsi_res->ctrl_pdata[DSI_CTRL_0]);
	sdata = dsi_res->shared_data;

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(sdata))
		return;

	sdata->hw_config = vdd->samsung_hw_config;
	LCD_INFO("DSI h/w configuration is %d\n",
			sdata->hw_config);
}

void mdss_samsung_panel_pbaboot_config(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct mdss_panel_info *pinfo = NULL;
	struct mdss_debug_data *mdd =
		(struct mdss_debug_data *)((mdss_mdp_get_mdata())->debug_inf.debug_data);
	struct samsung_display_driver_data *vdd = NULL;
	bool need_to_force_vidoe_mode = false;

	if (IS_ERR_OR_NULL(ctrl) || IS_ERR_OR_NULL(mdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx, mdd : 9x%zx\n",
				(size_t)ctrl, (size_t)mdd);
		return;
	}

	pinfo = &ctrl->panel_data.panel_info;
	vdd = check_valid_ctrl(ctrl);

	if (vdd->support_hall_ic) {
		/* check the lcd id for DISPLAY_1 and DISPLAY_2 */
		if (!mdss_panel_attached(DISPLAY_1) && !mdss_panel_attached(DISPLAY_2))
			need_to_force_vidoe_mode = true;
	} else {
		/* check the lcd id for DISPLAY_1 */
		if (!mdss_panel_attached(DISPLAY_1))
			need_to_force_vidoe_mode = true;
	}

	/* Support PBA boot without lcd */
	if (need_to_force_vidoe_mode &&
			!IS_ERR_OR_NULL(pinfo) &&
			!IS_ERR_OR_NULL(vdd) &&
			(pinfo->mipi.mode == DSI_CMD_MODE)) {
		LCD_ERR("force VIDEO_MODE : %d\n", ctrl->ndx);
		pinfo->type = MIPI_VIDEO_PANEL;
		pinfo->mipi.mode = DSI_VIDEO_MODE;
		pinfo->mipi.traffic_mode = DSI_BURST_MODE;
		pinfo->mipi.bllp_power_stop = true;
		pinfo->mipi.te_sel = 0;
		pinfo->mipi.vsync_enable = 0;
		pinfo->mipi.hw_vsync_mode = 0;
		pinfo->mipi.force_clk_lane_hs = true;
		pinfo->mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;

		pinfo->cont_splash_enabled = false;
		pinfo->mipi.lp11_init = false;

		vdd->support_mdnie_lite = false;
		vdd->support_mdnie_trans_dimming = false;
		vdd->mdnie_disable_trans_dimming = false;

		if (!IS_ERR_OR_NULL(vdd->panel_func.parsing_otherline_pdata) && mdss_panel_attached(DISPLAY_1)) {
			vdd->panel_func.parsing_otherline_pdata = NULL;
			destroy_workqueue(vdd->other_line_panel_support_workq);
		}

		pinfo->esd_check_enabled = false;
		ctrl->on_cmds.link_state = DSI_LP_MODE;
		ctrl->off_cmds.link_state = DSI_LP_MODE;

		/* To avoid underrun panic*/
		mdd->logd.xlog_enable = 0;
		vdd->dtsi_data[ctrl->ndx].samsung_osc_te_fitting = false;
	}
}

void mdss_samsung_panel_parse_dt(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl)
{
	int rc, i;
	u32 tmp[2];
	char panel_extra_power_gpio[] = "samsung,panel-extra-power-gpio1";
	char backlight_tft_gpio[] = "samsung,panel-backlight-tft-gpio1";
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);
	const char *data;
	const __be32 *data_32;

	rc = of_property_read_u32(np, "samsung,support_panel_max", tmp);
	vdd->support_panel_max = !rc ? tmp[0] : 1;

	/* Set LP11 init flag */
	vdd->dtsi_data[ctrl->ndx].samsung_lp11_init = of_property_read_bool(np, "samsung,dsi-lp11-init");
	LCD_ERR("LP11 init %s\n",
		vdd->dtsi_data[ctrl->ndx].samsung_lp11_init ? "enabled" : "disabled");

	rc = of_property_read_u32(np, "samsung,mdss-power-on-reset-delay-us", tmp);
	vdd->dtsi_data[ctrl->ndx].samsung_power_on_reset_delay = (!rc ? tmp[0] : 0);

	rc = of_property_read_u32(np, "samsung,mdss-power-pre-off-reset-delay-us", tmp);
	vdd->dtsi_data[ctrl->ndx].samsung_power_pre_off_reset_delay = (!rc ? tmp[0] : 0);

	rc = of_property_read_u32(np, "samsung,mdss-power-off-reset-delay-us", tmp);
	vdd->dtsi_data[ctrl->ndx].samsung_power_off_reset_delay = (!rc ? tmp[0] : 0);

	rc = of_property_read_u32(np, "samsung,mdss-dsi-off-reset-delay-us", tmp);
	vdd->dtsi_data[ctrl->ndx].samsung_dsi_off_reset_delay = (!rc ? tmp[0] : 0);

	/* Set esc clk 128M */
	vdd->dtsi_data[ctrl->ndx].samsung_esc_clk_128M = of_property_read_bool(np, "samsung,esc-clk-128M");
	LCD_ERR("ESC CLK 128M %s\n",
		vdd->dtsi_data[ctrl->ndx].samsung_esc_clk_128M ? "enabled" : "disabled");

	vdd->dtsi_data[ctrl->ndx].panel_lpm_enable = of_property_read_bool(np, "samsung,panel-lpm-enable");
	LCD_ERR("alpm enable %s\n",
		vdd->dtsi_data[ctrl->ndx].panel_lpm_enable ? "enabled" : "disabled");

	/*Set OSC TE fitting flag */
	vdd->dtsi_data[ctrl->ndx].samsung_osc_te_fitting =
		of_property_read_bool(np, "samsung,osc-te-fitting-enable");

	if (vdd->dtsi_data[ctrl->ndx].samsung_osc_te_fitting) {
		rc = of_property_read_u32_array(np, "samsung,osc-te-fitting-cmd-index", tmp, 2);
		if (!rc) {
			vdd->dtsi_data[ctrl->ndx].samsung_osc_te_fitting_cmd_index[0] =
				tmp[0];
			vdd->dtsi_data[ctrl->ndx].samsung_osc_te_fitting_cmd_index[1] =
				tmp[1];
		}

		rc = of_property_read_u32(np, "samsung,osc-te-fitting-sampling-rate", tmp);

		vdd->te_fitting_info.sampling_rate = !rc ? tmp[0] : 2;

	}

	LCD_INFO("OSC TE fitting %s\n",
		vdd->dtsi_data[0].samsung_osc_te_fitting ? "enabled" : "disabled");

	/* Set HMT flag */
	vdd->dtsi_data[0].hmt_enabled = of_property_read_bool(np, "samsung,hmt_enabled");
	if (vdd->dtsi_data[0].hmt_enabled)
		for (i = 1; i < vdd->support_panel_max; i++)
			vdd->dtsi_data[i].hmt_enabled = true;

	LCD_INFO("hmt %s\n",
		vdd->dtsi_data[0].hmt_enabled ? "enabled" : "disabled");

	/* TCON Clk On Support */
	vdd->dtsi_data[ctrl->ndx].samsung_tcon_clk_on_support =
		of_property_read_bool(np, "samsung,tcon-clk-on-support");
	LCD_INFO("tcon clk on support: %s\n",
			vdd->dtsi_data[ctrl->ndx].samsung_tcon_clk_on_support ?
			"enabled" : "disabled");

	/* Set TFT flag */
	vdd->mdnie_tuning_enable_tft = of_property_read_bool(np,
				"samsung,mdnie-tuning-enable-tft");
	vdd->dtsi_data[ctrl->ndx].tft_common_support  = of_property_read_bool(np,
		"samsung,tft-common-support");

	LCD_INFO("tft_common_support %s\n",
	vdd->dtsi_data[ctrl->ndx].tft_common_support ? "enabled" : "disabled");

	vdd->dtsi_data[ctrl->ndx].tft_module_name = of_get_property(np,
		"samsung,tft-module-name", NULL);  /* for tft tablet */

	vdd->dtsi_data[ctrl->ndx].panel_vendor = of_get_property(np,
		"samsung,panel-vendor", NULL);

	vdd->dtsi_data[ctrl->ndx].disp_model = of_get_property(np,
		"samsung,disp-model", NULL);

	vdd->dtsi_data[ctrl->ndx].backlight_gpio_config = of_property_read_bool(np,
		"samsung,backlight-gpio-config");

	LCD_INFO("backlight_gpio_config %s\n",
	vdd->dtsi_data[ctrl->ndx].backlight_gpio_config ? "enabled" : "disabled");

	/* Factory Panel Swap*/
	vdd->dtsi_data[ctrl->ndx].samsung_support_factory_panel_swap = of_property_read_bool(np,
		"samsung,support_factory_panel_swap");

	/* Set extra power gpio */
	for (i = 0; i < MAX_EXTRA_POWER_GPIO; i++) {
		panel_extra_power_gpio[strlen(panel_extra_power_gpio) - 1] = '1' + i;
		vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i] =
				 of_get_named_gpio(np,
						panel_extra_power_gpio, 0);
		if (!gpio_is_valid(vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i]))
			LCD_ERR("%d, panel_extra_power gpio%d not specified\n",
							__LINE__, i+1);
		else
			LCD_ERR("extra gpio num : %d\n", vdd->dtsi_data[ctrl->ndx].panel_extra_power_gpio[i]);
	}

	/* Set tft backlight gpio */
	for (i = 0; i < MAX_BACKLIGHT_TFT_GPIO; i++) {
		backlight_tft_gpio[strlen(backlight_tft_gpio) - 1] = '1' + i;
		vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i] =
				 of_get_named_gpio(np,
						backlight_tft_gpio, 0);
		if (!gpio_is_valid(vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i]))
			LCD_ERR("%d, backlight_tft_gpio gpio%d not specified\n",
							__LINE__, i+1);
		else
			LCD_ERR("tft gpio num : %d\n", vdd->dtsi_data[ctrl->ndx].backlight_tft_gpio[i]);
	}

	/* Set Mdnie lite HBM_CE_TEXT_MDNIE mode used */
	vdd->dtsi_data[ctrl->ndx].hbm_ce_text_mode_support = of_property_read_bool(np, "samsung,hbm_ce_text_mode_support");

	/* Set Backlight IC discharge time */
	rc = of_property_read_u32(np, "samsung,blic-discharging-delay-us", tmp);
	vdd->dtsi_data[ctrl->ndx].blic_discharging_delay_tft = (!rc ? tmp[0] : 6);

	/* Set cabc delay time */
	rc = of_property_read_u32(np, "samsung,cabc-delay-us", tmp);
	vdd->dtsi_data[ctrl->ndx].cabc_delay = (!rc ? tmp[0] : 6);

	/* Change hw_config to support single, dual and split dsi on runtime */
	data = of_get_property(np, "samsung,hw-config", NULL);
	if (data) {
		if (!strcmp(data, "dual_dsi"))
			vdd->samsung_hw_config = DUAL_DSI;
		else if (!strcmp(data, "split_dsi"))
			vdd->samsung_hw_config = SPLIT_DSI;
		else if (!strcmp(data, "single_dsi"))
			vdd->samsung_hw_config = SINGLE_DSI;
	}

	/* IRC */
	vdd->samsung_support_irc = of_property_read_bool(np, "samsung,support_irc");

	/* MULTI_RESOLUTION	*/
	vdd->multires_stat.is_support = of_property_read_bool(np, "samsung,support_multires");
	LCD_DEBUG("vdd->multires_stat.is_support = %d\n", vdd->multires_stat.is_support);

	/* POC Driver */
	vdd->poc_driver.is_support = of_property_read_bool(np, "samsung,support_poc_driver");
	LCD_INFO("vdd->poc_drvier.is_support = %d\n", vdd->poc_driver.is_support);

	rc = of_property_read_u32(np, "samsung,poc_erase_delay_ms", tmp);
	vdd->poc_driver.erase_delay_ms = (!rc ? tmp[0] : 0);

	rc = of_property_read_u32(np, "samsung,poc_write_delay_us", tmp);
	vdd->poc_driver.write_delay_us = (!rc ? tmp[0] : 0);

	LCD_INFO("erase_delay_ms(%d) write_delay_us (%d)\n", vdd->poc_driver.erase_delay_ms, vdd->poc_driver.write_delay_us);

	/* dsiphy drive strength*/
	rc = of_property_read_u32(np, "samsung,dsiphy_drive_str", tmp);
	vdd->dsiphy_drive_str = (!rc ? tmp[0] : 0);

	LCD_INFO("dsiphy_drive_str(0x%x)\n", vdd->dsiphy_drive_str);

	/* PAC */
	vdd->pac = of_property_read_bool(np, "samsung,support_pac");
	LCD_INFO("vdd->pac = %d\n", vdd->pac);

	/* Set elvss_interpolation_temperature */
	data_32 = of_get_property(np, "samsung,elvss_interpolation_temperature", NULL);

	if (data_32)
		vdd->elvss_interpolation_temperature = (int)(be32_to_cpup(data_32));
	else
		vdd->elvss_interpolation_temperature = ELVSS_INTERPOLATION_TEMPERATURE;

	/* Set lux value for mdnie HBM */
	data_32 = of_get_property(np, "samsung,enter_hbm_lux", NULL);
	if (data_32)
		vdd->enter_hbm_lux = (int)(be32_to_cpup(data_32));
	else
		vdd->enter_hbm_lux = ENTER_HBM_LUX;

	/* Power Control for LPM */
	vdd->lpm_power_control = of_property_read_bool(np, "samsung,lpm-power-control");
	LCD_INFO("lpm_power_control %s\n", vdd->lpm_power_control ? "enabled" : "disabled");

	if (vdd->lpm_power_control) {
		rc = of_property_read_string(np, "samsung,lpm-power-control-supply-name", &data);
		if (rc)
			LCD_ERR("error reading lpm-power name. rc=%d\n", rc);
		else
			snprintf(vdd->lpm_power_control_supply_name,
				ARRAY_SIZE((vdd->lpm_power_control_supply_name)), "%s", data);

		data_32 = of_get_property(np, "samsung,lpm-power-control-supply-min-voltage", NULL);
		if (data_32)
			vdd->lpm_power_control_supply_min_voltage = (int)(be32_to_cpup(data_32));
		else
			LCD_ERR("error reading lpm-power min_voltage\n");

		data_32 = of_get_property(np, "samsung,lpm-power-control-supply-max-voltage", NULL);
		if (data_32)
			vdd->lpm_power_control_supply_max_voltage = (int)(be32_to_cpup(data_32));
		else
			LCD_ERR("error reading lpm-power max_voltage\n");

		LCD_INFO("lpm_power_control Name=%s, Min=%d, Max=%d\n",
			vdd->lpm_power_control_supply_name, vdd->lpm_power_control_supply_min_voltage,
			vdd->lpm_power_control_supply_max_voltage);
	}

	/* To reduce DISPLAY ON time */
	vdd->dtsi_data[ctrl->ndx].samsung_reduce_display_on_time = of_property_read_bool(np,"samsung,reduce_display_on_time");
	vdd->dtsi_data[ctrl->ndx].samsung_dsi_force_clock_lane_hs = of_property_read_bool(np,"samsung,dsi_force_clock_lane_hs");
	rc = of_property_read_u32(np, "samsung,wait_after_reset_delay", tmp);
	vdd->dtsi_data[ctrl->ndx].samsung_wait_after_reset_delay = (!rc ? tmp[0] : 0);
	rc = of_property_read_u32(np, "samsung,wait_after_sleep_out_delay", tmp);
	vdd->dtsi_data[ctrl->ndx].samsung_wait_after_sleep_out_delay = (!rc ? tmp[0] : 0);

	if (vdd->dtsi_data[ctrl->ndx].samsung_reduce_display_on_time) {
		/*
			Please Check interrupt gpio is general purpose gpio
			1. pmic gpio or gpio-expender is not permitted.
			2. gpio should have unique interrupt number.
		*/

		if(gpio_is_valid(of_get_named_gpio(np, "samsung,home_key_irq_gpio", 0))) {
				vdd->dtsi_data[ctrl->ndx].samsung_home_key_irq_num =
					gpio_to_irq(of_get_named_gpio(np, "samsung,home_key_irq_gpio", 0));
				LCD_ERR("%s gpio : %d (irq : %d)\n",
					np->name, of_get_named_gpio(np, "samsung,home_key_irq_gpio", 0),
					vdd->dtsi_data[ctrl->ndx].samsung_home_key_irq_num);
		}

		if(gpio_is_valid(of_get_named_gpio(np, "samsung,fingerprint_irq_gpio", 0))) {
			vdd->dtsi_data[ctrl->ndx].samsung_finger_print_irq_num =
				gpio_to_irq(of_get_named_gpio(np, "samsung,fingerprint_irq_gpio", 0));
			LCD_ERR("%s gpio : %d (irq : %d)\n",
				np->name, of_get_named_gpio(np, "samsung,fingerprint_irq_gpio", 0),
				vdd->dtsi_data[ctrl->ndx].samsung_finger_print_irq_num);
		}
	}

	mdss_samsung_panel_parse_dt_cmds(np, ctrl);

	/* Set HALL IC */
	vdd->support_hall_ic  = of_property_read_bool(np, "samsung,mdss_dsi_hall_ic_enable");
	LCD_ERR("hall_ic %s\n", vdd->support_hall_ic ? "enabled" : "disabled");

	if (vdd->support_hall_ic) {
		vdd->hall_ic_notifier_display.priority = 0; /* Tsp is 1, Touch key is 2 */
		vdd->hall_ic_notifier_display.notifier_call = samsung_display_hall_ic_status;
#ifdef CONFIG_FOLDER_HALL
		hall_ic_register_notify(&vdd->hall_ic_notifier_display);
#endif

		mdss_samsung_panel_parse_dt_cmds(np, ctrl);
		/*temporary, MUST FIX*/
		/*vdd->mdnie_tune_state_dsi[DISPLAY_2] = init_dsi_tcon_mdnie_class(DISPLAY_2, vdd);*/
	}

	/* LCD SELECT*/
	vdd->select_panel_gpio = of_get_named_gpio(np,
			"samsung,mdss_dsi_lcd_sel_gpio", 0);
	if (gpio_is_valid(vdd->select_panel_gpio))
		gpio_request(vdd->select_panel_gpio, "lcd_sel_gpio");

	/* Panel LPM */
	rc = of_property_read_u32(np, "samsung,lpm-init-delay-ms", tmp);
	vdd->dtsi_data[ctrl->ndx].samsung_lpm_init_delay = (!rc ? tmp[0] : 0);

	mdss_samsung_panel_parse_dt_esd(np, ctrl);
	mdss_samsung_panel_pbaboot_config(np, ctrl);
}

void mdss_samsung_dsi_force_hs(struct mdss_panel_data *pdata)
{
	u32 tmp;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!IS_ERR_OR_NULL(ctrl_pdata)) {
		tmp = MIPI_INP((ctrl_pdata->ctrl_base) + 0xac);
		tmp |= (1<<28);
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0xac, tmp);
		wmb();

		LCD_INFO("ndx : %d \n", ctrl_pdata->ndx);
		/* To guarantee HS clock boosting */
		usleep_range(2000, 2000);
	}
}

void mdss_samsung_resume_event(int irq)
{
	/*
	*	SKIP mutex_lock & mutex_unlock
	*
	*	mdss_dsi_resume is executed at only sleep status.
	*	mdss_samsung_resume_event is called from pinctrl irq handler directrly.
	*	pinctrl irq happens very very frequently.
	*/

	//mutex_lock(&vdd_data.vdd_blank_unblank_lock);

	if (vdd_data.dtsi_data[DISPLAY_1].samsung_reduce_display_on_time &&
		vdd_data.vdd_blank_mode[DISPLAY_1] == FB_BLANK_POWERDOWN) {

		if ((irq == vdd_data.dtsi_data[DISPLAY_1].samsung_finger_print_irq_num) ||
			(irq == vdd_data.dtsi_data[DISPLAY_1].samsung_home_key_irq_num)) {
			vdd_data.bootsing_at_resume = true;
			LCD_INFO("irq : %d\n", irq);
		}
	}

	//mutex_unlock(&vdd_data.vdd_blank_unblank_lock);
}

/***********/
/* A2 line */
/***********/

#define OTHER_PANEL_FILE "/efs/FactoryApp/a2_line.dat"

int read_line(char *src, char *buf, int *pos, int len)
{
	int idx = 0;

	LCD_DEBUG("(%d) ++\n", *pos);

	while (*(src + *pos) != 10 && *(src + *pos) != 13) {
		buf[idx] = *(src + *pos);

#if 0
		LCD_ERR("%c (%3d) idx(%d)\n", buf[idx], buf[idx], idx);
#endif
		idx++;
		(*pos)++;

		if (idx > MAX_READ_LINE_SIZE) {
			LCD_ERR("overflow!\n");
			return idx;
		}

		if (*pos >= len) {
			LCD_ERR("End of File (%d) / (%d)\n", *pos, len);
			return idx;
		}
	}

	while (*(src + *pos) == 10 || *(src + *pos) == 13)
		(*pos)++;

	LCD_DEBUG("--\n");

	return idx;
}

int mdss_samsung_read_otherline_panel_data(struct samsung_display_driver_data *vdd)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret = 0;
	mm_segment_t fs;

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(OTHER_PANEL_FILE, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR "%s File open failed\n", __func__);

		if (!IS_ERR_OR_NULL(vdd->panel_func.set_panel_fab_type))
			vdd->panel_func.set_panel_fab_type(BASIC_FB_PANLE_TYPE);/*to work as original line panel*/
		ret = -ENOENT;
		goto err;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	LCD_INFO("Loading File Size : %ld(bytes)", l);

	dp = kmalloc(l + 10, GFP_KERNEL);
	if (dp == NULL) {
		LCD_INFO("Can't not alloc memory for tuning file load\n");
		filp_close(filp, current->files);
		ret = -1;
		goto err;
	}
	pos = 0;
	memset(dp, 0, l);

	LCD_INFO("before vfs_read()\n");
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	LCD_INFO("after vfs_read()\n");

	if (ret != l) {
		LCD_INFO("vfs_read() filed ret : %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		ret = -1;
		goto err;
	}

	if (!IS_ERR_OR_NULL(vdd->panel_func.parsing_otherline_pdata))
		ret = vdd->panel_func.parsing_otherline_pdata(filp, vdd, dp, l);

	filp_close(filp, current->files);

	set_fs(fs);

	kfree(dp);

	return ret;
err:
	set_fs(fs);
	return ret;
}

void read_panel_data_work_fn(struct delayed_work *work)
{
	int ret = 1;

	ret = mdss_samsung_read_otherline_panel_data(&vdd_data);

	if (ret && vdd_data.other_line_panel_work_cnt) {
		queue_delayed_work(vdd_data.other_line_panel_support_workq,
						&vdd_data.other_line_panel_support_work, msecs_to_jiffies(OTHERLINE_WORKQ_DEALY));
		vdd_data.other_line_panel_work_cnt--;
	} else
		destroy_workqueue(vdd_data.other_line_panel_support_workq);

	if (vdd_data.other_line_panel_work_cnt == 0)
		LCD_ERR(" cnt (%d)\n", vdd_data.other_line_panel_work_cnt);
}

/*********************/
/* LPM control       */
/*********************/
void mdss_samsung_panel_low_power_config(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	if (IS_ERR_OR_NULL(pdata)) {
		LCD_ERR("Invalid input data\n");
		return;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	vdd = check_valid_ctrl(ctrl);

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..\n");
		return;
	}

	if (!vdd->dtsi_data[ctrl->ndx].panel_lpm_enable) {
		LCD_INFO("[Panel LPM] LPM(ALPM/HLPM) is not supported\n");
		return;
	}

	mdss_samsung_panel_lpm_ctrl(pdata, enable);

	if (enable) {
		vdd->esd_recovery.is_wakeup_source = true;
	} else {
		vdd->esd_recovery.is_wakeup_source = false;
	}

	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd);
}

/*
 * mdss_init_panel_lpm_reg_offset()
 * This function find offset for reg value
 * reg_list[X][0] is reg value
 * reg_list[X][1] is offset for reg value
 * cmd_list is the target cmds for searching reg value
 */
int mdss_init_panel_lpm_reg_offset(struct mdss_dsi_ctrl_pdata *ctrl,
		int (*reg_list)[2],
		struct dsi_panel_cmds *cmd_list[], int list_size)
{
	struct dsi_panel_cmds *lpm_cmds = NULL;
	int i = 0, j = 0, max_cmd_cnt;

	if (IS_ERR_OR_NULL(ctrl))
		goto end;

	if (IS_ERR_OR_NULL(reg_list) || IS_ERR_OR_NULL(cmd_list))
		goto end;

	for (i = 0; i < list_size; i++) {
		lpm_cmds = cmd_list[i];
		max_cmd_cnt = lpm_cmds->cmd_cnt;

		for (j = 0; j < max_cmd_cnt; j++) {
			if (lpm_cmds->cmds[j].payload &&
					lpm_cmds->cmds[j].payload[0] == reg_list[i][0]) {
				reg_list[i][1] = j;
				break;
			}
		}
	}

end:
	for (i = 0; i < list_size; i++)
		LCD_DEBUG("[Panel LPM] offset[%d] : %d\n", i, reg_list[i][1]);
	return 0;
}

void mdss_samsung_panel_lpm_hz_ctrl(struct mdss_panel_data *pdata, int aod_ctrl)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	struct mdss_panel_info *pinfo = NULL;

	if (IS_ERR_OR_NULL(pdata)) {
		LCD_ERR("Invalid input data\n");
		return;
	}

	pinfo = &pdata->panel_info;
	if (IS_ERR_OR_NULL(pinfo)) {
		LCD_ERR("pinfo is null..\n");
		return;
	}

	vdd = (struct samsung_display_driver_data *)pdata->panel_private;
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null..\n");
		return;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..\n");
		return;
	}

	if (!vdd->dtsi_data[ctrl->ndx].panel_lpm_enable) {
		LCD_INFO("[Panel LPM] LPM(ALPM/HLPM) is not supported\n");
		return;
	}

	if (pinfo->blank_state == MDSS_PANEL_BLANK_BLANK) {
		LCD_INFO("[Panel LPM] Do not change Hz\n");
		/*return;*/
	}

	switch (aod_ctrl) {
	case TX_LPM_HZ_NONE:
		mdss_samsung_send_cmd(ctrl, vdd->panel_lpm.hz);
		break;
	case TX_LPM_AOD_ON:
	case TX_LPM_AOD_OFF:
		mdss_samsung_send_cmd(ctrl, aod_ctrl);
		break;
	default:
		break;
	}

	LCD_DEBUG("[Panel LPM] Update Hz: %s, aod_ctrl : %s\n",
			vdd->panel_lpm.hz == TX_LPM_HZ_NONE ? "NONE" :
			vdd->panel_lpm.hz == TX_LPM_1HZ ? "1HZ" :
			vdd->panel_lpm.hz == TX_LPM_2HZ ? "2HZ" :
			vdd->panel_lpm.hz == TX_LPM_30HZ ? "30HZ" : "Default",
			aod_ctrl == TX_LPM_HZ_NONE ? "NONE" :
			aod_ctrl == TX_LPM_AOD_ON ? "AOD_ON" :
			aod_ctrl == TX_LPM_AOD_OFF ? "AOD_OFF" : "UNKNOWN");
}

void mdss_samsung_panel_lpm_ctrl(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	struct mdss_panel_info *pinfo = NULL;
	static int stored_bl_level; /* Used for factory mode only */
	int current_bl_level = 0;
	int ndx = 0;
	u32 lpm_init_delay = 0;

	if (IS_ERR_OR_NULL(pdata)) {
		LCD_ERR("Invalid input data\n");
		return;
	}

	LCD_ERR("[Panel LPM] ++ (%d) \n", enable);

	pinfo = &pdata->panel_info;
	if (IS_ERR_OR_NULL(pinfo)) {
		LCD_ERR("pinfo is null..\n");
		return;
	}

	vdd = (struct samsung_display_driver_data *)pdata->panel_private;
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("vdd is null..\n");
		return;
	}

	ctrl = samsung_get_dsi_ctrl(vdd);
	if (IS_ERR_OR_NULL(ctrl)) {
		LCD_ERR("ctrl is null..\n");
		return;
	}

	ndx = display_ndx_check(ctrl);

	if (!vdd->dtsi_data[ctrl->ndx].panel_lpm_enable) {
		LCD_INFO("[Panel LPM] LPM(ALPM/HLPM) is not supported\n");
		return;
	}

	if (pinfo->blank_state == MDSS_PANEL_BLANK_BLANK) {
		LCD_INFO("[Panel LPM] Do not change mode\n");
		goto end;
	}

	lpm_init_delay = vdd->dtsi_data[ctrl->ndx].samsung_lpm_init_delay;

	mutex_lock(&vdd->vdd_panel_lpm_lock);

	if (enable) { /* AOD ON(Enter) */
		if (unlikely(vdd->is_factory_mode)) {
			LCD_INFO("[Panel LPM] Set low brightness for factory mode\n");
			mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
			stored_bl_level = vdd->mfd_dsi[ndx]->bl_level;
			pdata->set_backlight(pdata, 0);
			mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
		}

		if (pinfo->blank_state == MDSS_PANEL_BLANK_BLANK ||
			pinfo->blank_state == MDSS_PANEL_BLANK_UNBLANK) {
			mdss_samsung_send_cmd(ctrl, TX_DISPLAY_OFF);
			LCD_DEBUG("[Panel LPM] Send panel DISPLAY_OFF cmds\n");
		} else
			LCD_DEBUG("[Panel LPM] skip DISPLAY_OFF cmds\n");

		if (vdd->panel_func.samsung_multires)
			vdd->panel_func.samsung_multires(vdd);

		/* self display operation when AOD ENTER */
		ss_self_display_aod_enter(ctrl);

		/* lpm init delay   */
		if (vdd->display_status_dsi[ndx].aod_delay == true) {
			vdd->display_status_dsi[ndx].aod_delay = false;
			if (lpm_init_delay)
				msleep(lpm_init_delay);
			LCD_DEBUG("%ums delay before turn on lpm mode",
					lpm_init_delay);
		}

		mdss_samsung_send_cmd(ctrl, TX_LPM_ON);
		LCD_DEBUG("[Panel LPM] Send panel LPM cmds\n");

		mdss_samsung_panel_lpm_hz_ctrl(pdata, TX_LPM_HZ_NONE);

		if (unlikely(vdd->is_factory_mode))
			mdss_samsung_send_cmd(ctrl, TX_DISPLAY_ON);
		else {
			/* The display_on cmd will be sent on next commit */
			vdd->display_status_dsi[ndx].wait_disp_on = true;
			vdd->display_status_dsi[ndx].wait_actual_disp_on = true;
			LCD_DEBUG("[Panel LPM] Set wait_disp_on to true\n");
		}
	} else { /* AOD OFF(Exit) */
		/* Turn Off ALPM Mode */
		mdss_samsung_send_cmd(ctrl, TX_LPM_OFF);
		LCD_ERR("[Panel LPM] Send panel LPM off cmds\n");

		LCD_DEBUG("[Panel LPM] Restore brightness level\n");
		mutex_lock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);
		if (unlikely(vdd->is_factory_mode &&
					vdd->mfd_dsi[ndx]->unset_bl_level == 0)) {
			LCD_INFO("[Panel LPM] restore bl_level for factory\n");

			current_bl_level = stored_bl_level;
		} else {
			current_bl_level = vdd->mfd_dsi[DISPLAY_1]->bl_level;
		}
		pinfo->blank_state = MDSS_PANEL_BLANK_UNBLANK;

		/* self display operation when AOD EXIT */
		ss_self_display_aod_exit(ctrl);

		pdata->set_backlight(pdata, current_bl_level);
		mutex_unlock(&vdd->mfd_dsi[DISPLAY_1]->bl_lock);

		if (vdd->support_mdnie_lite) {
			update_dsi_tcon_mdnie_register(vdd);
			if (vdd->support_mdnie_trans_dimming)
				vdd->mdnie_disable_trans_dimming = false;
		}

		if (vdd->panel_func.samsung_cover_control && vdd->cover_control)
			vdd->panel_func.samsung_cover_control(ctrl, vdd);

		if (unlikely(vdd->is_factory_mode))
			mdss_samsung_send_cmd(ctrl, TX_DISPLAY_ON);
		else {
			/* The display_on cmd will be sent on next commit */
			vdd->display_status_dsi[ndx].wait_disp_on = true;
			vdd->display_status_dsi[ndx].wait_actual_disp_on = true;
			LCD_DEBUG("[Panel LPM] Set wait_disp_on to true\n");
		}
	}

	LCD_INFO("[Panel LPM] En/Dis : %s, LPM_MODE : %s, Hz : %s, bl_level : %s\n",
				/* Enable / Disable */
				enable ? "Enable" : "Disable",
				/* Check LPM mode */
				vdd->panel_lpm.mode == ALPM_MODE_ON ? "ALPM" :
				vdd->panel_lpm.mode == HLPM_MODE_ON ? "HLPM" :
				vdd->panel_lpm.mode == MODE_OFF ? "MODE_OFF" : "UNKNOWN",
				/* Check the currnet Hz */
				vdd->panel_lpm.hz == TX_LPM_1HZ ? "1Hz" :
				vdd->panel_lpm.hz == TX_LPM_2HZ ? "2Hz" :
				vdd->panel_lpm.hz == TX_LPM_30HZ ? "30Hz" :
				vdd->panel_lpm.hz == TX_LPM_HZ_NONE ? "NONE" : "UNKNOWN",
				/* Check current brightness level */
				vdd->panel_lpm.lpm_bl_level == PANEL_LPM_2NIT ? "2NIT" :
				vdd->panel_lpm.lpm_bl_level == PANEL_LPM_40NIT ? "40NIT" :
				vdd->panel_lpm.lpm_bl_level == PANEL_LPM_60NIT ? "60NIT" : "UNKNOWN");

	mutex_unlock(&vdd->vdd_panel_lpm_lock);
end:
	LCD_DEBUG("[Panel LPM] --\n");
}


struct samsung_display_driver_data *samsung_get_vdd(void)
{
	return &vdd_data;
}

struct mdss_dsi_ctrl_pdata *samsung_get_dsi_ctrl(struct samsung_display_driver_data *vdd)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid vdd vdd : 0x%zx", (size_t)vdd);
		return NULL;
	}

	if (unlikely(vdd->ctrl_dsi[DSI_CTRL_0]->cmd_sync_wait_broadcast)) {
		if (vdd->ctrl_dsi[DSI_CTRL_0]->cmd_sync_wait_trigger)
			ctrl = vdd->ctrl_dsi[DSI_CTRL_0];
		else
			ctrl = vdd->ctrl_dsi[DSI_CTRL_1];
	} else
		ctrl = vdd->ctrl_dsi[DSI_CTRL_0];

	return ctrl;
}

int display_ndx_check(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return DSI_CTRL_0;
	}

	if (vdd->support_hall_ic &&
		mdss_panel_attached(DISPLAY_1) && mdss_panel_attached(DISPLAY_2)) {
		if (vdd->display_status_dsi[DISPLAY_1].hall_ic_status == HALL_IC_OPEN)
			return DISPLAY_1; /*OPEN : Internal PANEL */
		else
			return DISPLAY_2; /*CLOSE : External PANEL */
	} else
		return ctrl->ndx;
}

int samsung_display_hall_ic_status(struct notifier_block *nb,
			unsigned long hall_ic, void *data)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = vdd_data.ctrl_dsi[DISPLAY_1];
	struct fb_info *fbi = registered_fb[ctrl_pdata->ndx];
	struct msm_fb_data_type *mfd = fbi->par;
	struct mdss_panel_info *pinfo = mfd->panel_info;

	/*
		previou panel off -> current panel on

		foder open : 0, close : 1
	*/

	if (!vdd_data.support_hall_ic)
		return 0;

	mutex_lock(&vdd_data.vdd_hall_ic_blank_unblank_lock); /*HALL IC blank mode change */
	mutex_lock(&vdd_data.vdd_hall_ic_lock); /* HALL IC switching */

	LCD_ERR("mdss hall_ic : %s, blank_status: %d start\n", hall_ic ? "CLOSE" : "OPEN", vdd_data.vdd_blank_mode[DISPLAY_1]);

	/* check the lcd id for DISPLAY_1 and DISPLAY_2 */
	if (mdss_panel_attached(DISPLAY_1) && mdss_panel_attached(DISPLAY_2)) {
		/* To check current blank mode */
		if ((vdd_data.vdd_blank_mode[DISPLAY_1] == FB_BLANK_UNBLANK ||
			pinfo->blank_state == MDSS_PANEL_BLANK_LOW_POWER) &&
			ctrl_pdata->panel_data.panel_info.panel_state &&
			vdd_data.display_status_dsi[DISPLAY_1].hall_ic_status != hall_ic) {

			/* set flag */
			vdd_data.display_status_dsi[DISPLAY_1].hall_ic_mode_change_trigger = true;

			/* panel off */
			ctrl_pdata->off(&ctrl_pdata->panel_data);

			/* set status */
			vdd_data.display_status_dsi[DISPLAY_1].hall_ic_status = hall_ic;

			/* panel on */
			ctrl_pdata->on(&ctrl_pdata->panel_data);

			/* clear flag */
			vdd_data.display_status_dsi[DISPLAY_1].hall_ic_mode_change_trigger = false;

			/* Brightness setting */
			if (vdd_data.ctrl_dsi[DISPLAY_1]->bklt_ctrl == BL_DCS_CMD) {
				mutex_lock(&vdd_data.mfd_dsi[DISPLAY_1]->bl_lock);
				mdss_samsung_brightness_dcs(ctrl_pdata, vdd_data.bl_level);
				mutex_unlock(&vdd_data.mfd_dsi[DISPLAY_1]->bl_lock);
			}

			/* display on */
			if (vdd_data.ctrl_dsi[DISPLAY_1]->panel_mode == DSI_VIDEO_MODE)
				mdss_samsung_send_cmd(ctrl_pdata, TX_DISPLAY_ON);

			/* refresh a frame to panel */
			if (vdd_data.ctrl_dsi[DISPLAY_1]->panel_mode == DSI_CMD_MODE) {
				fbi->fbops->fb_pan_display(&fbi->var, fbi);
			}
		} else {
			vdd_data.display_status_dsi[DISPLAY_1].hall_ic_status = hall_ic;
			LCD_ERR("mdss skip display changing\n");
		}
	} else {
		/* check the lcd id for DISPLAY_1 */
		if (mdss_panel_attached(DISPLAY_1))
			vdd_data.display_status_dsi[DISPLAY_1].hall_ic_status = HALL_IC_OPEN;

		/* check the lcd id for DISPLAY_2 */
		if (mdss_panel_attached(DISPLAY_2))
			vdd_data.display_status_dsi[DISPLAY_1].hall_ic_status = HALL_IC_CLOSE;
	}

	mutex_unlock(&vdd_data.vdd_hall_ic_lock); /* HALL IC switching */
	mutex_unlock(&vdd_data.vdd_hall_ic_blank_unblank_lock); /*HALL IC blank mode change */

	LCD_ERR("mdss hall_ic : %s, blank_status: %d end\n", hall_ic ? "CLOSE" : "OPEN", vdd_data.vdd_blank_mode[DISPLAY_1]);

	return 0;
}

int get_hall_ic_status(char *mode)
{
	if (mode == NULL)
		return true;

	if (*mode - '0')
		vdd_data.display_status_dsi[DISPLAY_1].hall_ic_status = HALL_IC_CLOSE;
	else
		vdd_data.display_status_dsi[DISPLAY_1].hall_ic_status = HALL_IC_OPEN;

	LCD_ERR("hall_ic : %s\n", vdd_data.display_status_dsi[DISPLAY_1].hall_ic_status ? "CLOSE" : "OPEN");

	return true;
}
EXPORT_SYMBOL(get_hall_ic_status);
__setup("hall_ic=0x", get_hall_ic_status);

/***************************************************************************************************
*		BRIGHTNESS RELATED FUNCTION.
****************************************************************************************************/
#define NORMAL_MAX_BL 255
void set_normal_br_values(struct samsung_display_driver_data *vdd, int ndx)
{
	int from, end;
	int left, right, p;
	int loop = 0;
	struct candela_map_table *table;

	if (vdd->pac)
		table = &vdd->dtsi_data[ndx].pac_candela_map_table[vdd->panel_revision];
	else
		table = &vdd->dtsi_data[ndx].candela_map_table[vdd->panel_revision];

	if (IS_ERR_OR_NULL(table->cd)) {
		LCD_ERR("No candela_map_table..\n");
		return;
	}

	LCD_DEBUG("table size (%d)\n", table->tab_size);

	if (vdd->bl_level > table->max_lv)
		vdd->bl_level = table->max_lv;

	left = 0;
	right = table->tab_size - 1;

	while (left <= right) {
		loop++;
		p = (left + right) / 2;
		from = table->from[p];
		end = table->end[p];
		LCD_DEBUG("[%d] from(%d) end(%d) / %d\n", p, from, end, vdd->bl_level);

		if (vdd->bl_level >= from && vdd->bl_level <= end)
			break;
		if (vdd->bl_level < from)
			right = p - 1;
		else
			left = p + 1;

		if (loop > table->tab_size) {
			pr_err("can not find (%d) level in table!\n", vdd->bl_level);
			p = table->tab_size - 1;
			break;
		}
	};

	/* for elvess, vint etc.. which are using 74 steps.*/
	vdd->candela_level = table->cd[p];
	vdd->cd_idx = table->idx[p];

	if (vdd->pac) {
		vdd->pac_cd_idx = table->scaled_idx[p];
		vdd->interpolation_cd = table->interpolation_cd[p];
		LCD_INFO("pac_cd_idx (%d) cd_idx (%d) cd (%d) interpolation_cd (%d)\n",
			vdd->pac_cd_idx, vdd->cd_idx, vdd->candela_level, vdd->interpolation_cd);
	} else
		LCD_INFO("cd_idx (%d) candela_level (%d) \n", vdd->cd_idx, vdd->candela_level);

	return;
}

void set_hbm_br_values(struct samsung_display_driver_data *vdd, int ndx)
{
	int from, end;
	int left, right, p;
	int loop = 0;
	struct hbm_candela_map_table *table;

	if (vdd->pac)
		table = &vdd->dtsi_data[ndx].pac_hbm_candela_map_table[vdd->panel_revision];
	else
		table = &vdd->dtsi_data[ndx].hbm_candela_map_table[vdd->panel_revision];

	if (IS_ERR_OR_NULL(table->cd)) {
		LCD_ERR("No hbm_candela_map_table..\n");
		return;
	}

	if (vdd->bl_level > table->hbm_max_lv)
		vdd->bl_level = table->hbm_max_lv;

	left = 0;
	right = table->tab_size - 1;

	while (1) {
		loop++;
		p = (left + right) / 2;
		from = table->from[p];
		end = table->end[p];
		LCD_DEBUG("[%d] from(%d) end(%d) / %d\n", p, from, end, vdd->bl_level);

		if (vdd->bl_level >= from && vdd->bl_level <= end)
			break;
		if (vdd->bl_level < from)
			right = p - 1;
		else
			left = p + 1;
		LCD_DEBUG("left(%d) right(%d)\n", left, right);

		if (loop > table->tab_size) {
			pr_err("can not find (%d) level in table!\n", vdd->bl_level);
			p = table->tab_size - 1;
			break;
		}
	};

	/* set values..*/
	vdd->interpolation_cd = vdd->candela_level = table->cd[p];
	vdd->auto_brightness = table->auto_level[p];

	LCD_INFO("[%d] candela_level (%d) auto_brightness (%d) \n", p, vdd->candela_level, vdd->auto_brightness);

	return;
}

void mdss_samsung_update_brightness_packet(struct dsi_cmd_desc *packet, int *count, struct dsi_panel_cmds *tx_cmd)
{
	int loop = 0;

	if (IS_ERR_OR_NULL(packet)) {
		LCD_ERR("%ps no packet\n", __builtin_return_address(0));
		return;
	}

	if (IS_ERR_OR_NULL(tx_cmd)) {
		LCD_ERR("%ps no tx_cmd\n", __builtin_return_address(0));
		return;
	}

	if (*count > (BRIGHTNESS_MAX_PACKET - 1))
		panic("over max brightness_packet size(%d).. !!", BRIGHTNESS_MAX_PACKET);

	for (loop = 0; loop < tx_cmd->cmd_cnt; loop++) {
		packet[*count].dchdr.dtype = tx_cmd->cmds[loop].dchdr.dtype;
		packet[*count].dchdr.wait = tx_cmd->cmds[loop].dchdr.wait;
		packet[*count].dchdr.dlen = tx_cmd->cmds[loop].dchdr.dlen;

		packet[*count].payload = tx_cmd->cmds[loop].payload;

		(*count)++;
	}
}

void update_packet_level_key_enable(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *packet, int *cmd_cnt, int level_key)
{
	if (!level_key)
		return;
	else {
		if (level_key & LEVEL1_KEY)
			mdss_samsung_update_brightness_packet(packet, cmd_cnt, mdss_samsung_cmds_select(ctrl, TX_LEVEL1_KEY_ENABLE, NULL));

		if (level_key & LEVEL2_KEY)
			mdss_samsung_update_brightness_packet(packet, cmd_cnt, mdss_samsung_cmds_select(ctrl, TX_LEVEL2_KEY_ENABLE, NULL));
	}
}

void update_packet_level_key_disable(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *packet, int *cmd_cnt, int level_key)
{
	if (!level_key)
		return;
	else {
		if (level_key & LEVEL1_KEY)
			mdss_samsung_update_brightness_packet(packet, cmd_cnt, mdss_samsung_cmds_select(ctrl, TX_LEVEL1_KEY_DISABLE, NULL));

		if (level_key & LEVEL2_KEY)
			mdss_samsung_update_brightness_packet(packet, cmd_cnt, mdss_samsung_cmds_select(ctrl, TX_LEVEL2_KEY_DISABLE, NULL));
	}
}

int mdss_samsung_hbm_brightness_packet_set(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ndx;
	int cmd_cnt = 0;
	int level_key = 0;
	struct dsi_cmd_desc *packet = NULL;
	struct dsi_panel_cmds *tx_cmd = NULL;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return 0;
	}

	ndx = display_ndx_check(ctrl);

	/* init packet */
	packet = &vdd->brightness[ndx]->brightness_packet[0];

	set_hbm_br_values(vdd, ctrl->ndx);

	/* IRC */
	if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_irc)) {
		level_key = false;
		tx_cmd = vdd->panel_func.samsung_hbm_irc(ctrl, &level_key);

		update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
		mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
		update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
	}

	/* Gamma */
	if (get_panel_tx_cmds(ctrl, TX_HBM_GAMMA)->cmd_cnt) {
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_gamma)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_hbm_gamma(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}
	}

	/* hbm etc */
	if (get_panel_tx_cmds(ctrl, TX_HBM_ETC)->cmd_cnt) {
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_hbm_etc)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_hbm_etc(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}
	}

	return cmd_cnt;
}

int mdss_samsung_normal_brightness_packet_set(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ndx;
	int cmd_cnt = 0;
	int level_key = 0;
	struct dsi_cmd_desc *packet = NULL;
	struct dsi_panel_cmds *tx_cmd = NULL;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return 0;
	}

	ndx = display_ndx_check(ctrl);

	/* init packet */
	packet = &vdd->brightness[ndx]->brightness_packet[0];

	vdd->auto_brightness = 0;

	set_normal_br_values(vdd, ndx);

	if (vdd->smart_dimming_loaded_dsi[ndx]) { /* OCTA PANEL */

		if (vdd->bl_level > NORMAL_MAX_BL)
			vdd->bl_level = NORMAL_MAX_BL;

		/* hbm off */
		if (vdd->display_status_dsi[ndx].hbm_mode) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_hbm_off)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_hbm_off(ctrl, &level_key);

				update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
				mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
			}
		}

		/* aid/aor */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_aid)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_aid(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		/* acl */
		if (vdd->acl_status || vdd->siop_status) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_acl_on)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_acl_on(ctrl, &level_key);

				update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
				mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
			}

			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_pre_acl_percent)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_pre_acl_percent(ctrl, &level_key);

				update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
				mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
			}

			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_acl_percent)) {

				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_acl_percent(ctrl, &level_key);

				update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
				mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
			}
		} else {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_acl_off)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_acl_off(ctrl, &level_key);

				update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
				mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
			}
		}

		/* elvss */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_elvss)) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_pre_elvss)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_pre_elvss(ctrl, &level_key);

				update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
				mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
			}

			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_elvss(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		/* temperature elvss */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_elvss_temperature1)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_elvss_temperature1(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_elvss_temperature2)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_elvss_temperature2(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		/* caps*/
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_caps)) {
			if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_pre_caps)) {
				level_key = false;
				tx_cmd = vdd->panel_func.samsung_brightness_pre_caps(ctrl, &level_key);

				update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
				mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
				update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
			}
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_caps(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		/* vint */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vint)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_vint(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		/* IRC */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_irc)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_irc(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		/* gamma */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_gamma)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_gamma(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}
	} else { /* TFT PANEL */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_tft_pwm_ldi)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_tft_pwm_ldi(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}
	}

	return cmd_cnt;
}

int mdss_samsung_single_transmission_packet(struct dsi_panel_cmds *cmds)
{
	int loop;
	struct dsi_cmd_desc *packet = cmds->cmds;
	int packet_cnt = cmds->cmd_cnt;

	for (loop = 0; (loop < packet_cnt) && (loop < BRIGHTNESS_MAX_PACKET); loop++) {
		if (packet[loop].dchdr.dtype == DTYPE_DCS_LWRITE ||
			packet[loop].dchdr.dtype == DTYPE_GEN_LWRITE)
			packet[loop].dchdr.last = 0;
		else {
			if (loop > 0)
				packet[loop - 1].dchdr.last = 1; /*To ensure previous single tx packet */

			packet[loop].dchdr.last = 1;
		}
	}

	if (loop == BRIGHTNESS_MAX_PACKET)
		return false;
	else {
		packet[loop - 1].dchdr.last = 1; /* To make last packet flag */
		return true;
	}
}

bool is_hbm_level(struct samsung_display_driver_data *vdd, int ndx)
{
	struct hbm_candela_map_table *table;

	if (!vdd->dtsi_data[ndx].hbm_candela_map_table[vdd->panel_revision].tab_size){
		LCD_ERR("hbm candela map able is empty\n");
		return false;
	}

	if (vdd->pac)
		table = &vdd->dtsi_data[ndx].pac_hbm_candela_map_table[vdd->panel_revision];
	else
		table = &vdd->dtsi_data[ndx].hbm_candela_map_table[vdd->panel_revision];

	if (vdd->bl_level < table->hbm_min_lv)
		return false;

	if (vdd->bl_level > table->hbm_max_lv) {
		LCD_ERR("bl_level(%d) is over max_level (%d), force set to max\n", vdd->bl_level, table->hbm_max_lv);
		vdd->bl_level = table->hbm_max_lv;
	}

	return true;
}

int mdss_samsung_brightness_dcs(struct mdss_dsi_ctrl_pdata *ctrl, int level)
{
	int cmd_cnt = 0;
	int ret = 0;
	int ndx;
	struct samsung_display_driver_data *vdd = NULL;
	struct dsi_panel_cmds *brightness_cmds = NULL;

	vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	brightness_cmds = get_panel_tx_cmds(ctrl, TX_BRIGHT_CTRL);

	vdd->bl_level = level;

	/* check the lcd id for DISPLAY_1 or DISPLAY_2 */
	if (!mdss_panel_attached(ctrl->ndx))
		return false;

	if (vdd->dtsi_data[0].hmt_enabled && vdd->hmt_stat.hmt_on) {
		LCD_ERR("HMT is on. do not set normal brightness..(%d)\n", level);
		return false;
	}

	if (vdd->mfd_dsi[DISPLAY_1]->panel_info->cont_splash_enabled) {
		LCD_ERR("splash is not done..\n");
		return 0;
	}

	ndx = display_ndx_check(ctrl);

	if (!vdd->dtsi_data[ndx].tft_common_support && is_hbm_level(vdd, ndx) ) {
		cmd_cnt = mdss_samsung_hbm_brightness_packet_set(ctrl);
		cmd_cnt > 0 ? vdd->display_status_dsi[ndx].hbm_mode = true : false;
	} else {
		cmd_cnt = mdss_samsung_normal_brightness_packet_set(ctrl);
		cmd_cnt > 0 ? vdd->display_status_dsi[ndx].hbm_mode = false : false;
	}

	if (cmd_cnt) {
		/* setting tx cmds cmt */
		brightness_cmds->cmd_cnt = cmd_cnt;

		/* generate single tx packet */
		ret = mdss_samsung_single_transmission_packet(brightness_cmds);

		/* sending tx cmds */
		if (ret) {
			mdss_samsung_send_cmd(ctrl, TX_BRIGHT_CTRL);

			if (!IS_ERR_OR_NULL(get_panel_tx_cmds(ctrl, TX_BLIC_DIMMING)->cmds)) {
				if (vdd->bl_level == 0)
					get_panel_tx_cmds(ctrl, TX_BLIC_DIMMING)->cmds->payload[1] = 0x24;
				else
					get_panel_tx_cmds(ctrl, TX_BLIC_DIMMING)->cmds->payload[1] = 0x2C;

				mdss_samsung_send_cmd(ctrl, TX_BLIC_DIMMING);
			}

			/* copr sum after changing brightness to calculate brightness avg.*/
			if (vdd->copr.copr_on)
				vdd->panel_func.samsung_set_copr_sum(vdd);

			LCD_INFO("DSI%d level : %d  candela : %dCD hbm : %d (%d)\n",
				ndx, vdd->bl_level, vdd->candela_level, vdd->display_status_dsi[ndx].hbm_mode, vdd->auto_brightness);
		} else
			LCD_INFO("DSDI%d single_transmission_fail error\n", ndx);
	} else
		LCD_INFO("DSI%d level : %d skip\n", ndx, vdd->bl_level);

	if (vdd->auto_brightness >= HBM_CE_MODE &&
			!vdd->dtsi_data[ndx].tft_common_support &&
			vdd->support_mdnie_lite)
		update_dsi_tcon_mdnie_register(vdd);

	return cmd_cnt;
}

/* HMT brightness*/
void set_hmt_br_values(struct samsung_display_driver_data *vdd, int ndx)
{
	int from, end;
	int left, right, p;
	struct candela_map_table *table;

	table = &vdd->dtsi_data[ndx].hmt_candela_map_table[vdd->panel_revision];

	if (IS_ERR_OR_NULL(table->cd)) {
		LCD_ERR("No candela_map_table..\n");
		return;
	}

	LCD_DEBUG("table size (%d)\n", table->tab_size);

	if (vdd->hmt_stat.hmt_bl_level > table->max_lv)
		vdd->hmt_stat.hmt_bl_level = table->max_lv;

	left = 0;
	right = table->tab_size - 1;

	while (left <= right) {
		p = (left + right) / 2;
		from = table->from[p];
		end = table->end[p];
		LCD_DEBUG("[%d] from(%d) end(%d) / %d\n", p, from, end, vdd->hmt_stat.hmt_bl_level);

		if (vdd->hmt_stat.hmt_bl_level >= from && vdd->hmt_stat.hmt_bl_level <= end)
			break;
		if (vdd->hmt_stat.hmt_bl_level < from)
			right = p - 1;
		else
			left = p + 1;
	};

	/* for elvess, vint etc.. which are using 74 steps.*/
	vdd->interpolation_cd = vdd->hmt_stat.candela_level_hmt = table->cd[p];
	vdd->hmt_stat.cmd_idx_hmt = table->idx[p];

	LCD_INFO("cd_idx (%d) candela_level (%d) \n", vdd->hmt_stat.cmd_idx_hmt, vdd->hmt_stat.candela_level_hmt);

	return;
}

int mdss_samsung_hmt_brightenss_packet_set(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ndx = 0;
	int cmd_cnt = 0;
	int level_key = 0;
	struct dsi_cmd_desc *packet = NULL;
	struct dsi_panel_cmds *tx_cmd = NULL;
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	LCD_DEBUG("++\n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return 0;
	}

	ndx = display_ndx_check(ctrl);

	set_hmt_br_values(vdd, ndx);

	/* init packet */
	packet = &vdd->brightness[ctrl->ndx]->brightness_packet[0];

	if (vdd->smart_dimming_hmt_loaded_dsi[ctrl->ndx]) {
		/* aid/aor B2 */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_aid_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_aid_hmt(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		/* elvss B5 */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_elvss_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_elvss_hmt(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		/* vint F4 */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_vint_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_vint_hmt(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}

		/* gamma CA */
		if (!IS_ERR_OR_NULL(vdd->panel_func.samsung_brightness_gamma_hmt)) {
			level_key = false;
			tx_cmd = vdd->panel_func.samsung_brightness_gamma_hmt(ctrl, &level_key);

			update_packet_level_key_enable(ctrl, packet, &cmd_cnt, level_key);
			mdss_samsung_update_brightness_packet(packet, &cmd_cnt, tx_cmd);
			update_packet_level_key_disable(ctrl, packet, &cmd_cnt, level_key);
		}
	}

	LCD_DEBUG("--\n");

	return cmd_cnt;
}

int mdss_samsung_brightness_dcs_hmt(struct mdss_dsi_ctrl_pdata *ctrl, int level)
{
	struct dsi_panel_cmds *brightness_cmds = NULL;
	struct samsung_display_driver_data *vdd = NULL;
	int cmd_cnt;
	int ndx;
	int ret = 0;

	vdd = check_valid_ctrl(ctrl);
	ndx = display_ndx_check(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx\n", (size_t)ctrl, (size_t)vdd);
		return false;
	}

	brightness_cmds = get_panel_tx_cmds(ctrl, TX_BRIGHT_CTRL);

	vdd->hmt_stat.hmt_bl_level = level;
	LCD_ERR("[HMT] hmt_bl_level(%d)\n", vdd->hmt_stat.hmt_bl_level);

	cmd_cnt = mdss_samsung_hmt_brightenss_packet_set(ctrl);

	/* sending tx cmds */
	if (cmd_cnt) {
		/* setting tx cmds cmt */
		brightness_cmds->cmd_cnt = cmd_cnt;

		/* generate single tx packet */
		ret = mdss_samsung_single_transmission_packet(brightness_cmds);

		if (ret) {
			mdss_samsung_send_cmd(ctrl, TX_BRIGHT_CTRL);

			LCD_INFO("DSI(%d) idx(%d), candela_level(%d), hmt_bl_level(%d)",
				ctrl->ndx, vdd->hmt_stat.cmd_idx_hmt, vdd->hmt_stat.candela_level_hmt, vdd->hmt_stat.hmt_bl_level);
		} else
			LCD_DEBUG("DSDI%d single_transmission_fail error\n", ctrl->ndx);
	} else
		LCD_INFO("DSI%d level : %d skip\n", ctrl->ndx, vdd->bl_level);

	return cmd_cnt;
}

/* TFT brightness*/
void mdss_samsung_brightness_tft_pwm(struct mdss_dsi_ctrl_pdata *ctrl, int level)
{
	struct samsung_display_driver_data *vdd = NULL;
	struct mdss_panel_info *pinfo;

	vdd = check_valid_ctrl(ctrl);
	if (vdd == NULL) {
		LCD_ERR("no PWM\n");
		return;
	}

	pinfo = &(ctrl->panel_data.panel_info);
	if (pinfo->blank_state == MDSS_PANEL_BLANK_BLANK)
		return;

	vdd->bl_level = level;

	if (vdd->panel_func.samsung_brightness_tft_pwm)
		vdd->panel_func.samsung_brightness_tft_pwm(ctrl, level);
}

void mdss_tft_autobrightness_cabc_update(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct samsung_display_driver_data *vdd = check_valid_ctrl(ctrl);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("Invalid data ctrl : 0x%zx vdd : 0x%zx", (size_t)ctrl, (size_t)vdd);
		return;
	}
	LCD_INFO("\n");

	switch (vdd->auto_brightness) {
	case 0:
		mdss_samsung_cabc_update();
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		mdss_samsung_send_cmd(ctrl, TX_CABC_ON);
		break;
	case 5:
	case 6:
		mdss_samsung_send_cmd(ctrl, TX_CABC_OFF);
		break;
	}
}

void mdss_samsung_panel_brightness_init(struct samsung_display_driver_data *vdd)
{
	int loop, loop2, loop3;
	struct dsi_cmd_desc default_cmd = {{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL};

	/* Set default link_state of brightness command */
	for (loop = 0; loop < SUPPORT_PANEL_COUNT; loop++) {
		vdd->brightness[loop] = kzalloc(sizeof(struct samsung_brightenss_data), GFP_KERNEL);

		for (loop2 = 0; loop2 < BRIGHTNESS_MAX_PACKET; loop2++) {
			vdd->brightness[loop]->brightness_packet[loop2] = default_cmd;

			for (loop3 = 0; loop3 < SUPPORT_PANEL_REVISION; loop3++) {
				vdd->dtsi_data[loop].panel_tx_cmd_list[TX_BRIGHT_CTRL][loop3].cmds = vdd->brightness[loop]->brightness_packet;
				vdd->dtsi_data[loop].panel_tx_cmd_list[TX_BRIGHT_CTRL][loop3].link_state = DSI_HS_MODE;
			}
		}
	}

	LCD_INFO("init done..\n");
}

/***************************************************************************************************
*		BRIGHTNESS RELATED END.
****************************************************************************************************/
