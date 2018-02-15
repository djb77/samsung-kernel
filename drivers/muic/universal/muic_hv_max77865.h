/*
 * muic_hv.h
 *
 * Copyright (C) 2011 Samsung Electrnoics
 * Thomas Ryu <smilesr.ryu@samsung.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *
 */

#ifndef __MAX77865_MUIC_HV_H__
#define __MAX77865_MUIC_HV_H__

#include "muic-internal.h"

/* MAX77865 HV irq */
#define MAX77865_VDNMON_MASK    (0x1 << 5)
#define MAX77865_MRXRDY_MASK    (0x1 << 7)
#define MAX77865_MPNACK_MASK    (0x1 << 3)
#define MAX77865_VBADC_MASK     (0x1 << 2)

/* MAX77865 BC STATUS1 register */
#define BC_STATUS1_VBVOLT_SHIFT		7
#define BC_STATUS1_CHGDETRUN_SHIFT	6
#define BC_STATUS1_PRCHGTYP_SHIFT	3
#define BC_STATUS1_DCDTMR_SHIFT		2
#define BC_STATUS1_CHGTYP_SHIFT		0
#define BC_STATUS1_VBVOLT_MASK		(0x1 << BC_STATUS1_VBVOLT_SHIFT)
#define BC_STATUS1_CHGDETRUN_MASK	(0x1 << BC_STATUS1_CHGDETRUN_SHIFT)
#define BC_STATUS1_PRCHGTYP_MASK	(0x7 << BC_STATUS1_PRCHGTYP_SHIFT)
#define BC_STATUS1_DCDTMR_MASK		(0x1 << BC_STATUS1_DCDTMR_SHIFT)
#define BC_STATUS1_CHGTYP_MASK		(0x3 << BC_STATUS1_CHGTYP_SHIFT)

/* MAX77865 BC STATUS2 register
 * [7:2] RSVD
 */
#define BC_STATUS2_DXOVP_SHIFT		1
#define BC_STATUS2_DNVDATREF_SHIFT	0
#define BC_STATUS2_DXOVP_MASK		(0x1 << BC_STATUS2_DXOVP_SHIFT)
#define BC_STATUS2_DNVDATREF_MASK	(0x1 << BC_STATUS2_DNVDATREF_SHIFT)

/* MAX77865 GP STATUS register */
#define GP_STATUS_UID_SHIFT		6
#define GP_STATUS_UIDGND_SHIFT		5
#define GP_STATUS_TSHUT_SHIFT		4
#define GP_STATUS_VBADC_SHIFT		0
#define GP_STATUS_UID_MASK		(0x3 << GP_STATUS_UID_SHIFT)
#define GP_STATUS_UIDGND_MASK		(0x1 << GP_STATUS_UIDGND_SHIFT)
#define GP_STATUS_TSHUT_MASK		(0x1 << GP_STATUS_TSHUT_SHIFT)
#define GP_STATUS_VBADC_MASK		(0xF << GP_STATUS_VBADC_SHIFT)
#define UID_JIG_USB_ON			(0x00)
#define UID_JIG_UART_OFF		(0x40)
#define UID_OPEN			(0xC0)

/* MAX77865 BC CONTROL1 register */
#define BC_CONTROL1_DCDCPL_SHIFT	7
#define BC_CONTROL1_UIDEN_SHIFT		6
#define BC_CONTROL1_NOAUTOIBUS_SHIFT	5
#define BC_CONTROL1_3ADCPDET_SHIFT	4
#define BC_CONTROL1_SFOUTCTRL_SHIFT	2
#define BC_CONTROL1_CHGDETMAN_SHIFT	1
#define BC_CONTROL1_CHGDETEN_SHIFT	0
#define BC_CONTROL1_DCDCPL_MASK		(0x1 << BC_CONTROL1_DCDCPL_SHIFT)
#define BC_CONTROL1_UIDEN_MASK		(0x1 << BC_CONTROL1_UIDEN_SHIFT)
#define BC_CONTROL1_NOAUTOIBUS_MASK	(0x1 << BC_CONTROL1_NOAUTOIBUS_SHIFT)
#define BC_CONTROL1_3ADCPDET_MASK	(0x1 << BC_CONTROL1_3ADCPDET_SHIFT)
#define BC_CONTROL1_SFOUTCTRL_MASK	(0x3 << BC_CONTROL1_SFOUTCTRL_SHIFT)
#define BC_CONTROL1_CHGDETMAN_MASK	(0x1 << BC_CONTROL1_CHGDETMAN_SHIFT)
#define BC_CONTROL1_CHGDETEN_MASK	(0x1 << BC_CONTROL1_CHGDETEN_SHIFT)
#define BC_CONTROL1_RESET		(0xC5)

/* MAX77865 BC CONTROL2 register
 * [7:6] RSVD
 */
#define BC_CONTROL2_DNMONEN_SHIFT	5
#define BC_CONTROL2_DPDNMAN_SHIFT	4
#define BC_CONTROL2_DPDRV_SHIFT		2
#define BC_CONTROL2_DNDRV_SHIFT		0
#define BC_CONTROL2_DNMONEN_MASK	(0x1 << BC_CONTROL2_DNMONEN_SHIFT)
#define BC_CONTROL2_DPDNMAN_MASK	(0x1 << BC_CONTROL2_DPDNMAN_SHIFT)
#define BC_CONTROL2_DPDRV_MASK		(0x3 << BC_CONTROL2_DPDRV_SHIFT)
#define BC_CONTROL2_DNDRV_MASK		(0x3 << BC_CONTROL2_DNDRV_SHIFT)

/* MAX77865 FC_INTMASK register 
 * [1:0] RSVD
 */
#define INTMASK_FC_MPGDONE_SHIFT	2
#define INTMASK_FC_MPNACK_SHIFT		3
#define INTMASK_FC_MRXBUFOW_SHIFT	4
#define INTMASK_FC_MRXTRF_SHIFT		5
#define INTMASK_FC_MRXPERR_SHIFT	6
#define INTMASK_FC_MRXRDY_SHIFT		7
#define INTMASK_FC_MPGDONE_UNMASK	(0 << INTMASK_FC_MPGDONE_SHIFT)
#define INTMASK_FC_MPNACK_UNMASK	(0 << INTMASK_FC_MPNACK_SHIFT)
#define INTMASK_FC_MRXBUFOW_UNMASK	(0 << INTMASK_FC_MRXBUFOW_SHIFT)
#define INTMASK_FC_MRXTRF_UNMASK	(0 << INTMASK_FC_MRXTRF_SHIFT)
#define INTMASK_FC_MRXPERR_UNMASK	(0 << INTMASK_FC_MRXPERR_SHIFT)
#define INTMASK_FC_MRXRDY_UNMASK	(0 << INTMASK_FC_MRXRDY_SHIFT)
#define INTMASK_FC_RESET		(0xFF) /* All MASK */
#define INTMASK_FC_INIT			(0x03) /* All UNMASK except for RSVD */

/* MAX77865 HV CONTROL1 register
 * [7:1] RSVD
 */
#define HVCONTROL1_VBUSADCEN_SHIFT	0
#define HVCONTROL1_VBUSADCEN_MASK	(0x1 << HVCONTROL1_VBUSADCEN_SHIFT)

/* MAX77865 HV CONTROL2 register
 * [2] RSVD
 */
#define HVCONTROL2_FCRXEN_SHIFT		7
#define HVCONTROL2_MPINGEN_SHIFT	6
#define HVCONTROL2_MTXBUSRES_SHIFT	5
#define HVCONTROL2_MTXEN_SHIFT		4
#define HVCONTROL2_MPING_SHIFT		3
#define HVCONTROL2_DP06EN_SHIFT		1
#define HVCONTROL2_HVDIGEN_SHIFT	0
#define HVCONTROL2_FCRXEN_MASK		(0x1 << HVCONTROL2_FCRXEN_SHIFT)
#define HVCONTROL2_MPINGEN_MASK		(0x1 << HVCONTROL2_MPINGEN_SHIFT)
#define HVCONTROL2_MTXBUSRES_MASK	(0x1 << HVCONTROL2_MTXBUSRES_SHIFT)
#define HVCONTROL2_MTXEN_MASK		(0x1 << HVCONTROL2_MTXEN_SHIFT)
#define HVCONTROL2_MPING_MASK		(0x1 << HVCONTROL2_MPING_SHIFT)
#define HVCONTROL2_DP06EN_MASK		(0x1 << HVCONTROL2_DP06EN_SHIFT)
#define HVCONTROL2_HVDIGEN_MASK		(0x1 << HVCONTROL2_HVDIGEN_SHIFT)

/* MAX77865 HVRXBYTE register */
#define HVRXBYTE_MAX			16

/* MAX77865 AFC charger W/A Check NUM */
#define AFC_CHARGER_WA_PING		5

/* MAX77865 MPing miss SW Workaround - delay time */
#define MPING_MISS_WA_TIME		2000


/* MAX77865 REGISTER ENABLE or DISABLE bit */
enum max77865_reg_bit_control {
        MAX77865_DISABLE_BIT            = 0,
        MAX77865_ENABLE_BIT,
};

typedef enum {
        CHGDETRUN_FALSE         = 0x00,
        CHGDETRUN_TRUE          = (0x1 << BC_CONTROL1_CHGDETEN_SHIFT),

        CHGDETRUN_DONTCARE      = 0xff,
} chgdetrun_t;

typedef enum {
	DPDNVDEN_DISABLE	= 0x00,
	DPDNVDEN_ENABLE		= (0x1 << BC_CONTROL2_DPDNMAN_SHIFT),

	DPDNVDEN_DONTCARE	= 0xff,
} dpdnvden_t;

typedef enum {
	VDNMON_LOW		= 0x00,
	VDNMON_HIGH		= (0x1 << BC_STATUS2_DNVDATREF_SHIFT),

	VDNMON_DONTCARE		= 0xff,
} vdnmon_t;

typedef enum {
	VBADC_VBDET		= 0x00,
	VBADC_4V_5V		= (0x1 << GP_STATUS_VBADC_SHIFT),
	VBADC_5V_6V		= (0x2 << GP_STATUS_VBADC_SHIFT),
	VBADC_6V_7V		= (0x3 << GP_STATUS_VBADC_SHIFT),
	VBADC_7V_8V		= (0x4 << GP_STATUS_VBADC_SHIFT),
	VBADC_8V_9V		= (0x5 << GP_STATUS_VBADC_SHIFT),
	VBADC_9V_10V		= (0x6 << GP_STATUS_VBADC_SHIFT),
	VBADC_10V_11V		= (0x7 << GP_STATUS_VBADC_SHIFT),
	VBADC_11V_12V		= (0x8 << GP_STATUS_VBADC_SHIFT),
	VBADC_12V_13V		= (0x9 << GP_STATUS_VBADC_SHIFT),
	VBADC_13V		= (0xA << GP_STATUS_VBADC_SHIFT),
	VBADC_RSVD1		= (0xB << GP_STATUS_VBADC_SHIFT),
	VBADC_RSVD2		= (0xC << GP_STATUS_VBADC_SHIFT),
	VBADC_RSVD3		= (0xD << GP_STATUS_VBADC_SHIFT),
	VBADC_RSVD4		= (0xE << GP_STATUS_VBADC_SHIFT),
	VBADC_RSVD5		= (0xF << GP_STATUS_VBADC_SHIFT),

	VBADC_QC_5V		= 0xeb,
	VBADC_QC_9V		= 0xec,
	VBADC_QC_12V		= 0xed,
	VBADC_QC_20V		= 0xee,

#if defined(CONFIG_MUIC_HV_12V)
	VBADC_AFC_12V		= 0xf9,
#endif	
	VBADC_AFC_5V		= 0xfa,
	VBADC_AFC_9V		= 0xfb,
	VBADC_AFC_ERR_V		= 0xfc,
	VBADC_AFC_ERR_V_NOT_0	= 0xfd,

	VBADC_ANY		= 0xfe,
	VBADC_DONTCARE		= 0xff,
} vbadc_t;

enum {
	HV_SUPPORT_QC_5V	= 5,
	HV_SUPPORT_QC_9V	= 9,
	HV_SUPPORT_QC_12V	= 12,
	HV_SUPPORT_QC_20V	= 20,
};

extern muic_attached_dev_t hv_muic_check_id_err
	(struct hv_data *phv, muic_attached_dev_t new_dev);

extern void max77865_hv_muic_reset_hvcontrol_reg(struct hv_data *phv);
#if defined(CONFIG_OF)
extern int of_max77865_hv_muic_dt(struct hv_data *phv);
#endif

extern int max77865_muic_hv_update_reg(struct i2c_client *i2c,
		const u8 reg, const u8 val, const u8 mask, const bool debug_en);

extern void max77865_hv_muic_init_detect(struct hv_data *phv);
extern void max77865_hv_muic_remove(struct hv_data *phv);
extern void max77865_hv_muic_remove_wo_free_irq(struct hv_data *phv);

extern void max77865_muic_set_adcmode_always(struct hv_data *phv);
#if !defined(CONFIG_SEC_FACTORY)
extern void max77865_muic_set_adcmode_oneshot(struct hv_data *phv);
#endif /* !CONFIG_SEC_FACTORY */
extern void max77865_hv_muic_adcmode_oneshot(struct hv_data *phv);

extern void max77865_muic_prepare_afc_charger(struct hv_data *phv);
extern bool max77865_muic_check_change_dev_afc_charger
	(struct hv_data *phv, muic_attached_dev_t new_dev);
extern void max77865_hv_muic_connect_start(struct hv_data *phv);
extern void hv_muic_chgdet_ready(struct hv_data *phv);

#endif /* __MAX77865_MUIC_HV_H__ */

