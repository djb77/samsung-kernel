/*
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
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

#ifndef __LINUX_TAOS_H
#define __LINUX_TAOS_H

#include <linux/types.h>

#ifdef __KERNEL__
#define TAOS_OPT "taos-opt"
#define MIN 1
#define FEATURE_TMD3700 1


#define CNTRL				0x80
#define ALS_TIME			0X81
#define PRX_RATE		       0x82
#define WAIT_TIME			0x83
#define ALS_MINTHRESHLO			0X84
#define ALS_MINTHRESHHI			0X85
#define ALS_MAXTHRESHLO			0X86
#define ALS_MAXTHRESHHI			0X87
#define PRX_MINTHRESH			      0X88
#define PRX_MAXTHRESH			      0X8A
#define PPERS		      0X8C
#define PGCFG0	      0X8E
#define PGCFG1		      0X8F
#define CFG1		      0X90



#define REVID				0x91
#define CHIPID				0x92
#define STATUS				0x93
#define CLR_CHAN0LO			0x94
#define CLR_CHAN0HI			0x95
#define RED_CHAN1LO			0x96
#define RED_CHAN1HI			0x97
#define GRN_CHAN1LO			0x98
#define GRN_CHAN1HI			0x99
#define BLU_CHAN1LO			0x9A
#define BLU_CHAN1HI			0x9B
#define PRX_DATA_HIGH		0x9C
#define PRX_DATA_LOW			0x9D
//#define TEST_STATUS			0x1F

#define CFG2		      0X9F
#define CFG3	      		0XAB
#define CFG4	      		0XAC
#define CFG5		      0XAD
#define POFFSET_L		      0XC0
#define POFFSET_H		      0XC1
#define AZ_CONFIG		      0XD6
#define CALIB		      0XD7
#define CALIBCFG		      0XD9
#define CALIBSTAT		      0XDC
#define INTENAB		      0XDD
#define AZ_CONFIG          0XD6

enum cmd_enable_reg {
	CMD_wen           = (0x1 << 3),
	CMD_pen         = (0x1 << 2),
	CMD_aen        = (0x1 << 1),
	CMD_pon  = (0x1),
};


/*TMD3782 cmd reg masks*/
//#define CMD_REG					0x80
#define CMD_BYTE_RW				0x00
#define CMD_WORD_BLK_RW			0x20
//#define CMD_SPL_FN				0x60
#define CMD_PROX_INTCLR			0X05
#define CMD_ALS_INTCLR			0X06
#define CMD_PROXALS_INTCLR		0X80

#define P_TIME_US(p)   ((((p)/88)    - 1.0) + 0.5)
#define PRX_PERSIST(p) (((p) & 0xf) << 4)
#define ALS_PERSIST(p) (((p) & 0xf) << 0)


#define CMD_TST_REG				0X08
#define CMD_USER_REG			0X09

/* TMD3782 cntrl reg masks */
#define CNTL_REG_CLEAR			0x00
#define CNTL_PROX_INT_ENBL		0X20
#define CNTL_ALS_INT_ENBL		0X10
#define CNTL_WAIT_TMR_ENBL		0X08
#define CNTL_PROX_DET_ENBL		0X04
#define CNTL_ADC_ENBL			0x02
#define CNTL_PWRON				0x01
#define CNTL_ALSPON_ENBL		0x03
#define CNTL_INTALSPON_ENBL		0x13
#define CNTL_PROXPON_ENBL		0x0F
#define CNTL_INTPROXPON_ENBL		0x2F

/* TMD3782 status reg masks */
#define STA_ADCVALID			0x01
#define STA_PRXVALID			0x02
#define STA_ADC_PRX_VALID		0x03
#define STA_ADCINTR				0x10
#define STA_PRXINTR				0x20

#ifdef CONFIG_PROX_WINDOW_TYPE
#define WINTYPE_OTHERS	'0'
#define WINTYPE_WHITE		'2'
#define WHITEWINDOW_HI_THRESHOLD		720
#define WHITEWINDOW_LOW_THRESHOLD		590
#if defined(CONFIG_MACH_MELIUS_USC) || defined(CONFIG_MACH_MELIUS_SPR)
#define BLACKWINDOW_HI_THRESHOLD		750
#define BLACKWINDOW_LOW_THRESHOLD		520
#else
#define BLACKWINDOW_HI_THRESHOLD		650
#define BLACKWINDOW_LOW_THRESHOLD		520
#endif
#endif

enum cmd_reg {
	CMD_REG           = (1 << 7),
	CMD_INCR          = (0x1 << 5),
	CMD_SPL_FN        = (0x3 << 5),
	CMD_PROX_INT_CLR  = (0x5 << 0),
	CMD_ALS_INT_CLR   = (0x6 << 0),
};

enum taos_3xxx_pwr_state {
	POWER_ON,
	POWER_OFF,
	POWER_STANDBY,
};



struct taos_platform_data {
	int als_int;
	u32 als_int_flags;
	int enable;
	void (*power)(bool);
	int (*light_adc_value)(void);
	void (*led_on)(bool);
	int prox_thresh_hi;
	int prox_thresh_low;
	int prox_th_hi_cal;
	int prox_th_low_cal;
	int als_time;
	int intr_filter;
	int prox_pulsecnt;
	int als_gain;
	int dgf;
	int cct_coef;
	int cct_offset;
	int coef_r;
	int coef_g;
	int coef_b;
	int min_max;
	int prox_rawdata_trim;
	int crosstalk_max_offset;
	int thresholed_max_offset;
	int lux_multiple;
	u32 als_en_flags;
	int vled_ldo;

};
#endif /*__KERNEL__*/
#endif
