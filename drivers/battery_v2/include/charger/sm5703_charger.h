/*
 * drivers/battery/sm5703_charger.h
 *
 * Header of Siliconmitus SM5703 Fuelgauge Driver
 *
 * Copyright (C) 2013 Siliconmitus Technology Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SM5703_CHARGER_H
#define SM5703_CHARGER_H
#include <linux/mfd/sm5703.h>
#include <linux/mfd/sm5703_irq.h>

enum {
	CHG_REG = 0,
	CHG_DATA,
	CHG_REGS,
};

enum {
    SM5703_TOPOFF_TIMER_10m         = 0x0,
    SM5703_TOPOFF_TIMER_20m         = 0x1,
    SM5703_TOPOFF_TIMER_30m         = 0x2,
    SM5703_TOPOFF_TIMER_45m         = 0x3,
};

#define SM5703_CNTL				    0x0C
#define SM5703_VBUSCNTL				0x0D
#define SM5703_CHGCNTL1				0x0E
#define SM5703_CHGCNTL2				0x0F
#define SM5703_CHGCNTL3				0x10
#define SM5703_CHGCNTL4				0x11
#define SM5703_CHGCNTL5				0x12
#define SM5703_CHGCNTL6				0x13
#define SM5703_OTGCURRENTCNTL		0x60
#define SM5703_Q3LIMITCNTL			0x66

#define SM5703_USBLDO1CNTL			0x0C
#define SM5703_USBLDO2CNTL			0x0C
#define SM5703_LDOOUT1CNTL			0x1A
#define SM5703_LDOOUT2CNTL			0x1B
#define SM5703_LDOOUT3CNTL			0x1C
#define SM5703_BUCKCNTL				0x1D
#define SM5703_DEVICE_ID			0x1E

#define SM5703_AUTOSET             0x1
#define SM5703_AUTOSET_MASK        (1 << 4)

#define SM5703_OPERATION_MODE				        0x07
#define SM5703_OPERATION_MODE_MASK                  0x07
#define SM5703_OPERATION_MODE_SHIFT			        0

#define SM5703_OPERATION_MODE_SUSPEND               0x00
#define SM5703_OPERATION_MODE_CHARGING_OFF          0x04//100
#define SM5703_OPERATION_MODE_CHARGING_ON           0x05//101
#define SM5703_OPERATION_MODE_FLASH_BOOST_MODE      0x06//110
#define SM5703_OPERATION_MODE_USB_OTG_MODE          0x07//111

#define SM5703_BSTOUT			0x0F
#define SM5703_BSTOUT_MASK			0x0F
#define SM5703_BSTOUT_SHIFT			0

#define SM5703_BSTOUT_4P5           0x05
#define SM5703_BSTOUT_5P0           0x0A
#define SM5703_BSTOUT_5P1           0x0B

#define SM5703_AUTOSTOP             0x1
#define SM5703_AUTOSTOP_MASK        (1 << 7)

#define SM5703_TOPOFF_TIMER		0x3
#define SM5703_TOPOFF_TIMER_MASK	0x60
#define SM5703_TOPOFF_TIMER_SHIFT	0x5

#define SM5703_AICLEN               0x1
#define SM5703_AICLEN_MASK          (1 << 7)

#define SM5703_VBUSLIMIT            0x3F
#define SM5703_VBUSLIMIT_MASK       0x3F

#define SM5703_BATREG               0x1F
#define SM5703_BATREG_MASK          0x1F

#define SM5703_FASTCHG               0x3F
#define SM5703_FASTCHG_MASK          0x3F

#define SM5703_TOPOFF               0x0F
#define SM5703_TOPOFF_MASK          0x78
#define SM5703_TOPOFF_SHIFT         0x03

#define SM5703_CHGON                0x01
#define SM5703_CHGON_MASK           (1 << 0)
#define SM5703_CHGON_SHIFT          0x00

#define SM5703_AICLTH                 0x07
#define SM5703_AICLTH_MASK            0x07

#define SM5703_FREQSEL                 0x03
#define SM5703_FREQSEL_MASK            0x0C
#define SM5703_FREQSEL_SHIFT           0x02

#define SM5703_OTGCURRENT               0x03
#define SM5703_OTGCURRENT_MASK          0x03

#define SM5703_OTGCURRENT_0P5A          0x00
#define SM5703_OTGCURRENT_0P7A          0x01
#define SM5703_OTGCURRENT_0P9A          0x02
#define SM5703_OTGCURRENT_1P2A          0x03

#define SM5703_FREQSEL_3MHZ             0x0
#define SM5703_FREQSEL_2P4MHZ           0x1
#define SM5703_FREQSEL_1P5MHZ           0x2
#define SM5703_FREQSEL_1P8MHZ           0x3

#define SM5703_BST_IQ3LIMIT_0P7X        0x0
#define SM5703_BST_IQ3LIMIT_1X          0x1
#define SM5703_BST_IQ3LIMIT_MASK            0x80
#define SM5703_BST_IQ3LIMIT_SHIFT           0x7

#define REDUCE_CURRENT_STEP				100
#define MINIMUM_INPUT_CURRENT			300
#if defined(CONFIG_TABLET_MODEL_CONCEPT) && !defined(CONFIG_SEC_FACTORY)
#define SLOW_CHARGING_CURRENT_STANDARD	999
#else
#define SLOW_CHARGING_CURRENT_STANDARD	400
#endif

#define ENABLE 1
#define DISABLE 0

extern bool sec_bat_get_slate_mode(void);

#define ENABLE_AICL 1

#define EN_NOBAT_IRQ	0
#define EN_DONE_IRQ	1
#define EN_TOPOFF_IRQ	1
#define EN_CHGON_IRQ	0
#define EN_AICL_IRQ	1
#define EN_OTGFAIL_IRQ	1

#define MINVAL(a, b) ((a <= b) ? a : b)

#ifndef EN_TEST_READ
#define EN_TEST_READ 1
#endif

#define ENABLE 1
#define DISABLE 0

static int sm5703_reg_map[] = {
	SM5703_INTMSK1,
	SM5703_INTMSK2,
	SM5703_INTMSK3,
	SM5703_INTMSK4,
	SM5703_STATUS1,
	SM5703_STATUS2,
	SM5703_STATUS3,
	SM5703_STATUS4,
	SM5703_CNTL,
	SM5703_VBUSCNTL,
	SM5703_CHGCNTL1,
	SM5703_CHGCNTL2,
	SM5703_CHGCNTL3,
	SM5703_CHGCNTL4,
	SM5703_CHGCNTL5,
	SM5703_CHGCNTL6,
	SM5703_OTGCURRENTCNTL,
	SM5703_Q3LIMITCNTL,
	SM5703_STATUS5,
};

typedef struct sm5703_charger_data {
	struct device *dev;
	struct i2c_client	*i2c;
	sm5703_mfd_chip_t	*sm5703;
	struct power_supply	psy_chg;
	struct power_supply	psy_otg;

	sm5703_charger_platform_data_t *pdata;

	struct wake_lock aicl_wake_lock;
	struct delayed_work aicl_work;
	int input_current;
	int charging_current;
	int topoff_current;
	int cable_type;
	bool aicl_state;
	bool is_charging;
	int charge_mode;
	struct mutex io_lock;
	/* register programming */
	int reg_addr;
	int reg_data;
	int nchgen;

	bool ovp;
	bool is_mdock;
	struct workqueue_struct *wq;
	int status;
#ifdef CONFIG_FLED_SM5703
	struct sm_fled_info *fled_info;
#endif
} sm5703_charger_data_t;

enum {
	SM5703_AICL_4300MV = 0,
	SM5703_AICL_4400MV,
	SM5703_AICL_4500MV,
	SM5703_AICL_4600MV,
	SM5703_AICL_4700MV,
	SM5703_AICL_4800MV,
	SM5703_AICL_4900MV,
};

extern int poweroff_charging;

#endif /*SM5703_CHARGER_H*/
