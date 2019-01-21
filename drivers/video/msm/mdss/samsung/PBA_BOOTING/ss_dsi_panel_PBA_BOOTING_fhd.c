/*
 * =================================================================
 *
 *
 *	Description:  samsung display panel file
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
 *
*/
#include "ss_dsi_panel_PBA_BOOTING_fhd.h"

static void mdss_panel_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("%s\n", vdd->panel_name);

	vdd->support_mdnie_lite = false;
	vdd->dtsi_data[DISPLAY_1].tft_common_support = true;
}

static int __init samsung_panel_init(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	char panel_string[] = "ss_dsi_panel_PBA_BOOTING_FHD";

	vdd->panel_name = mdss_mdp_panel + 8;

	LCD_INFO("PBA : %s / %s\n", vdd->panel_name, panel_string);

	if (!strncmp(vdd->panel_name, panel_string, strlen(panel_string)))
		vdd->panel_func.samsung_panel_init = mdss_panel_init;

	return 0;
}
early_initcall(samsung_panel_init);
