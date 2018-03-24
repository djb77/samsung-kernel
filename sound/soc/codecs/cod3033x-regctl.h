/*
 * Copyright (c) 2014 Samsung Electronics Co. Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _COD3033X_POWER_H
#define _COD3033X_POWER_H

#include <linux/completion.h>

#include <sound/soc.h>
#include <linux/iio/consumer.h>


struct cod3033x_power_priv {
	struct device *dev;
	struct pinctrl *pinctrl;
	unsigned short i2c_addr;
	struct i2c_client *i2c;
	struct mutex            io_lock; 
	
};


#define COD3026X_80_DET_PDB			0x80
#define COD3026X_81_DET_ON			0x81
#define COD3026X_82_MIC_BIAS			0x82
#define COD3026X_83_JACK_DET1			0x83
#define COD3026X_84_JACK_DET2			0x84
#define COD3026X_85_MIC_DET			0x85
#define COD3026X_86_DET_TIME			0x86
#define COD3026X_87_LDO_DIG			0x87
#define COD3026X_88_KEY_TIME			0x88
#define COD3026X_8B_MICBIAS2			0x8B

/** COD3026X_87_LDO_DIG */
#define CTMD_JD_DBNC_4_SHIFT	6
#define CTMD_JD_DBNC_4_MASK	BIT(CTMD_JD_DBNC_4_SHIFT)

#endif /* _COD3033X_POWER_H */
