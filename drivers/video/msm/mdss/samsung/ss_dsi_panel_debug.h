/*
 * =================================================================
 *
 *	Description:  samsung display debug common file
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
#ifndef _SAMSUNG_DSI_PANEL_DEBUG_H_
#define _SAMSUNG_DSI_PANEL_DEBUG_H_

#include "ss_dsi_panel_common.h"

struct samsung_display_driver_data;

/* PANEL DEBUG FUCNTION */
void mdss_samsung_dump_regs(void);
void mdss_samsung_dsi_dump_regs(struct samsung_display_driver_data *vdd, int dsi_num);
void mdss_mdp_underrun_dump_info(struct samsung_display_driver_data *vdd);
int mdss_samsung_read_rddpm(struct samsung_display_driver_data *vdd);
int mdss_samsung_dsi_te_check(struct samsung_display_driver_data *vdd);
void samsung_image_dump_worker(struct samsung_display_driver_data *vdd, struct work_struct *work);
void samsung_mdss_image_dump(void);
int mdss_sasmung_panel_debug_init(struct samsung_display_driver_data *vdd);

int mdss_samsung_read_rddpm(struct samsung_display_driver_data *vdd);
int mdss_samsung_read_rddsm(struct samsung_display_driver_data *vdd);
int mdss_samsung_read_errfg(struct samsung_display_driver_data *vdd);
int mdss_samsung_read_dsierr(struct samsung_display_driver_data *vdd);
int mdss_samsung_read_self_diag(struct samsung_display_driver_data *vdd);

#endif
