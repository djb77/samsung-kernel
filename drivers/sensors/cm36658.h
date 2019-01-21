/* include/linux/CM36658.h
 *
 * Copyright (C) 2017 Vishay Capella Microsystems Limited
 * Author: Frank Hsieh <Frank.Hsieh@vishay.com>
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

#ifndef __LINUX_CM36658_H
#define __LINUX_CM36658_H

/* Define Command Code */
#define CS_CONF		0x00
#define CS_THDH		0x01
#define CS_THDL		0x02
#define CS_R_DATA	0xF0
#define CS_G_DATA	0xF1
#define CS_B_DATA	0xF2
#define CS_IR_DATA	0xF3
#define PS_DATA		0xF4
#define PS_CONF1	0x03
#define PS_CONF3	0x04
#define PS_THDL		0x05
#define PS_THDH		0x06
#define PS_CANC		0x07
#define PS_AC_L		0x08
#define PS_AC_DATA	0xF7

#define INT_FLAG		0xF5
#define ID_REG			0xF6

/* for ALS CONF command */

#define CS_RESERVED_1		(1 << 9)

#define CM36658_CS_START	(1 << 7)
#define CM36658_CS_IT_50MS	(0 << 2)
#define CM36658_CS_IT_100MS	(1 << 2)
#define CM36658_CS_IT_200MS	(2 << 2)
#define CM36658_CS_IT_400MS	(3 << 2)

#define CM36658_CS_STANDBY	(1 << 1)

#define CM36658_CS_GAIN			(1 << 13)
#define CM36658_CS_GAIN_MASK	0xDFFF

/* enable/disable ALS func, 1:disable , 0:enable */
#define CM36658_CS_SD		(1 << 0)
#define CM36658_CS_SD_MASK	0xFFFE

/* for PS CONF1 command */
#define CM36658_PS_IT_1T	(0 << 14)
#define CM36658_PS_IT_2T	(1 << 14)
#define CM36658_PS_IT_4T	(2 << 14)
#define CM36658_PS_IT_8T	(3 << 14)

#define CM36658_PS_START	(1 << 11)

#define CM36658_PS_INT_SEL	(1 << 8)

/* enable/disable Interrupt */
#define CM36658_PS_INT_ENABLE	(2 << 2)
#define CM36658_PS_INT_MASK		0xFFF3 

#define CM36658_PS_PERS_1		(0 << 4)
#define CM36658_PS_PERS_2		(1 << 4)
#define CM36658_PS_PERS_3		(2 << 4)
#define CM36658_PS_PERS_4		(3 << 4)

#define CM36658_PS_SMART_PERS	(1 << 1)
/* enable/disable PS func, 1:disable, 0:enable */
#define CM36658_PS_SD			(1 << 0)
#define CM36658_PS_SD_MASK		0xFFFE

/* for PS CONF3 command */
#define CM36658_PS_SUNLIGHT_ENABLE		(1 << 15)
#define CM36658_PS_SUNLIGHT_LV1			(0 << 13)
#define CM36658_PS_SUNLIGHT_LV2			(1 << 13)
#define CM36658_PS_SUNLIGHT_LV3			(2 << 13)
#define CM36658_PS_SUNLIGHT_LV4			(3 << 13)
#define CM36658_LED_I_70				(0 << 8)
#define CM36658_LED_I_95				(1 << 8)
#define CM36658_LED_I_110				(2 << 8)
#define CM36658_LED_I_130				(3 << 8)
#define CM36658_LED_I_170				(4 << 8)
#define CM36658_LED_I_200				(5 << 8)
#define CM36658_LED_I_220				(6 << 8)
#define CM36658_LED_I_240				(7 << 8)
#define CM36658_PS_AF_FORCE_MODE		(1 << 6)
#define CM36658_PS_AF_AUTO_MODE_MASK	0xFFBF
#define CM36658_PS_ACTIVE_FORCE_TRIG	(1 << 5)
#define CM36658_PS_START2				(1 << 3)

/* for INT FLAG */
#define INT_FLAG_PS_SPFLAG				(1 << 14)
#define PS_AC_FINISH_FLAG				(1 << 13)
#define INT_FLAG_PS_IF_CLOSE			(1 << 9)  //PS rises above PS_THDH INT trigger event
#define INT_FLAG_PS_IF_AWAY				(1 << 8)  //PS drops below PS_THDL INT trigger event

#endif