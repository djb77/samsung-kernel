/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#ifndef _SX9306_I2C_WIFI_REG_H_
#define _SX9306_I2C_WIFI_REG_H_

/*
 *  I2C Registers
 */
#define SX9306_IRQSTAT_REG      0x00
#define SX9306_TCHCMPSTAT_REG   0x01
#define SX9306_IRQ_ENABLE_REG   0x03
#define SX9306_CPS_CTRL0_REG    0x06
#define SX9306_CPS_CTRL1_REG    0x07
#define SX9306_CPS_CTRL2_REG    0x08
#define SX9306_CPS_CTRL3_REG    0x09
#define SX9306_CPS_CTRL4_REG    0x0A
#define SX9306_CPS_CTRL5_REG    0x0B
#define SX9306_CPS_CTRL6_REG    0x0C
#define SX9306_CPS_CTRL7_REG    0x0D
#define SX9306_CPS_CTRL8_REG    0x0E
#define SX9306_CPS_CTRL9_REG    0x0F
#define SX9306_CPS_CTRL10_REG   0x10
#define SX9306_SOFTRESET_REG    0x7F

/* Sensor Readback */
#define SX9306_REGSENSORSELECT  0x20
#define SX9306_REGUSEMSB        0x21
#define SX9306_REGUSELSB        0x22
#define SX9306_REGAVGMSB        0x23
#define SX9306_REGAVGLSB        0x24
#define SX9306_REGDIFFMSB       0x25
#define SX9306_REGDIFFLSB       0x26
#define SX9306_REGOFFSETMSB     0x27
#define SX9306_REGOFFSETLSB     0x28

/* IrqStat 0:Inactive 1:Active */
#define SX9306_IRQSTAT_RESET_FLAG       0x80
#define SX9306_IRQSTAT_TOUCH_FLAG       0x40
#define SX9306_IRQSTAT_RELEASE_FLAG     0x20
#define SX9306_IRQSTAT_COMPDONE_FLAG    0x10
#define SX9306_IRQSTAT_CONV_FLAG        0x08
#define SX9306_IRQSTAT_TXENSTAT_FLAG    0x01

/* CpsStat */
#define SX9306_TCHCMPSTAT_TCHSTAT3_FLAG   0x80
#define SX9306_TCHCMPSTAT_TCHSTAT2_FLAG   0x40
#define SX9306_TCHCMPSTAT_TCHSTAT1_FLAG   0x20
#define SX9306_TCHCMPSTAT_TCHSTAT0_FLAG   0x10

/* SoftReset */
#define SX9306_SOFTRESET  0xDE

struct smtc_reg_data {
	unsigned char reg;
	unsigned char val;
};

enum {
	SX9306_CTRL0_IDX = 1,
	SX9306_CTRL1_IDX,
	SX9306_CTRL2_IDX,
	SX9306_CTRL3_IDX,
	SX9306_CTRL4_IDX,
	SX9306_CTRL5_IDX,
	SX9306_CTRL6_IDX,
	SX9306_CTRL7_IDX,
	SX9306_CTRL8_IDX,
	SX9306_CTRL9_IDX,
	SX9306_CTRL10_IDX
};

static struct smtc_reg_data setup_reg[] = {
	{
		.reg = SX9306_IRQ_ENABLE_REG,
		.val = 0x60,
	},
	{
		.reg = SX9306_CPS_CTRL0_REG,
		.val = 0x04,	/* SCANPERIOD: 30ms */
	},
	{
		.reg = SX9306_CPS_CTRL1_REG,
		.val = 0x43,	/* Range: Small (+/-2.5pF) */
	},
	{
		.reg = SX9306_CPS_CTRL2_REG,
		.val = 0x7f,	/* Gain : x8, Freq : Reserved, Resolution : Finest */
	},
	{
		.reg = SX9306_CPS_CTRL3_REG,
		.val = 0x01,	/* RawFilt : Low */
	},
	{
		.reg = SX9306_CPS_CTRL4_REG,
		.val = 0x80,	/* AVGTHRESH */
	},
	{
		.reg = SX9306_CPS_CTRL5_REG,
		.val = 0x0E,	/* AVGPOSFILT : 110, AVGNEGFILT : Lowest */
	},
	{
		.reg = SX9306_CPS_CTRL6_REG,
		.val = 0x0A,	/* PROXTHRESH */
	},
	{
		.reg = SX9306_CPS_CTRL7_REG,
		.val = 0x00,	/* HYST : 32 */
	},
	{
		.reg = SX9306_CPS_CTRL8_REG,
		.val = 0x00,
	},
	{
		.reg = SX9306_CPS_CTRL9_REG,
		.val = 0xC0,
	},
	{
		.reg = SX9306_CPS_CTRL10_REG,
		.val = 0xAA,
	},
};

enum {
	OFF = 0,
	ON = 1
};

#endif /* _SX9306_I2C_WIFI_REG_H_*/