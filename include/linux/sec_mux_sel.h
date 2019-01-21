/*
 * Copyright (C) 2017 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/device.h>

#ifndef __ASM_ARCH_SEC_MUX_SEL_H
#define __ASM_ARCH_SEC_MUX_SEL_H

#ifdef __KERNEL__

struct sec_mux_sel_platform_data {
	/* callback functions */
	void (*initial_check)(void);

	bool mux_sel_1_en;
	bool mux_sel_2_en;
	bool mux_sel_3_en;

	int mux_sel_1;
	int mux_sel_2;
	int mux_sel_3;

	int mux_sel_1_mpp;
	int mux_sel_2_mpp;
	int mux_sel_3_mpp;

	int mux_sel_1_low;
	int mux_sel_2_low;
	int mux_sel_3_low;

	int mux_sel_1_high;
	int mux_sel_2_high;
	int mux_sel_3_high;

	int mux_sel_1_type;
	int mux_sel_2_type;
	int mux_sel_3_type;
};

#define sec_mux_sel_platform_data_t \
	struct sec_mux_sel_platform_data

struct sec_mux_sel_info {
	struct device *dev;
	sec_mux_sel_platform_data_t *pdata;
	struct mutex mpp_share_mutex;
};

enum sec_mux_sel {
	SEC_MUX_SEL_1 = 1,
	SEC_MUX_SEL_2,
	SEC_MUX_SEL_3,
};

enum sec_mux_sel_type {
	SEC_MUX_SEL_EAR_ADC = 0,
	SEC_MUX_SEL_BATT_ID,
	SEC_MUX_SEL_BATT_THM,
	SEC_MUX_SEL_CHG_THM,
	SEC_MUX_SEL_AP_THM,
	SEC_MUX_SEL_WPC_THM,
	SEC_MUX_SEL_SLAVE_CHG_THM,
	SEC_MUX_SEL_BLKT_THM, //7
};

#define EAR_ADC_MUX_SEL 	0x01
#define BATT_ID_MUX_SEL		0x02
#define BATT_THM_MUX_SEL	0x04
#define CHG_THM_MUX_SEL		0x08
#define AP_THM_MUX_SEL		0x10
#define WPC_THM_MUX_SEL		0x20
#define SLAVE_CHG_THM_MUX_SEL	0x40
#define BLKT_THM_MUX_SEL	0x80

void sec_mpp_mux_control(int mux_sel, int adc_type, int mutex_on);

extern int EAR_ADC_MUX_SEL_NUM;
extern int BATT_ID_MUX_SEL_NUM;
extern int BATT_THM_MUX_SEL_NUM;
extern int CHG_THM_MUX_SEL_NUM;
extern int AP_THM_MUX_SEL_NUM;
extern int WPC_THM_MUX_SEL_NUM;
extern int SLAVE_CHG_THM_MUX_SEL_NUM;
extern int BLKT_THM_MUX_SEL_NUM;
#endif
#endif
