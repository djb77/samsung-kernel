/*
 * Copyrights (C) 2017 Samsung Electronics, Inc.
 * Copyrights (C) 2017 Maxim Integrated Products, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_MFD_MAX77705_PD_H
#define __LINUX_MFD_MAX77705_PD_H
#include <linux/ccic/max77705.h>
#include <linux/wakelock.h>

#define MAX77705_PD_NAME	"MAX77705_PD"

enum {
	CC_SNK = 0,
	CC_SRC,
	CC_NO_CONN,
};

struct max77705_pd_data {
	/* interrupt pin */
	int irq_pdmsg;
	int irq_psrdy;
	int irq_datarole;
	int irq_ssacc;
	int irq_fct_id;

	u8 usbc_status1;
	u8 usbc_status2;
	u8 bc_status;
	u8 cc_status0;
	u8 cc_status1;
	u8 pd_status0;
	u8 pd_status1;

	u8 opcode_res;

	/* PD Message */
	u8 pdsmg;

	/* Data Role */
	enum max77705_data_role current_dr;
	enum max77705_data_role previous_dr;
	/* SSacc */
	u8 ssacc;
	/* FCT cable */
	u8 fct_id;
	enum max77705_ccpd_device device;

	bool pdo_list;
	bool psrdy_received;

	int cc_status;

	/* wakelock */
	struct wake_lock pdmsg_wake_lock;
	struct wake_lock datarole_wake_lock;
	struct wake_lock ssacc_wake_lock;
	struct wake_lock fct_id_wake_lock;
};

#endif
