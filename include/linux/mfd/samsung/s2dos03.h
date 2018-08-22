/*
 * s2dos03.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MFD_S2DOS03_H
#define __LINUX_MFD_S2DOS03_H
#include <linux/platform_device.h>
#include <linux/regmap.h>

#define MFD_DEV_NAME "s2dos03"

/**
 * sec_regulator_data - regulator data
 * @id: regulator id
 * @initdata: regulator init data (contraints, supplies, ...)
 */

struct s2dos03_dev {
	struct device *dev;
	struct i2c_client *i2c; /* 0xB2; PMIC, Flash LED */
	struct mutex i2c_lock;

	int type;
	u8 rev_num; /* pmic Rev */
	bool wakeup;

	struct s2dos03_platform_data *pdata;
};

struct s2dos03_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct s2dos03_platform_data {
	bool wakeup;
	int num_regulators;
	struct	s2dos03_regulator_data *regulators;
	int	device_type;
};

struct s2dos03 {
	struct regmap *regmap;
};

/* S2DOS03 registers */
/* Slave Addr : 0xC0 */
enum S2DOS03_reg {
	S2DOS03_REG_DEV_ID,
	S2DOS03_REG_TOPSYS_STAT,
	S2DOS03_REG_STAT,
	S2DOS03_REG_EN,
	S2DOS03_REG_GPIO_PD_CTRL,
	S2DOS03_REG_UVLO_CFG,
	S2DOS03_REG_LDO1_CFG,
	S2DOS03_REG_LDO2_CFG,
	S2DOS03_REG_LDO3_CFG,
	S2DOS03_REG_LDO4_CFG,
	S2DOS03_REG_BUCK_CFG,
	S2DOS03_REG_BUCK_VOUT,
	S2DOS03_REG_RTC_BUF,
};

/* S2DOS03 regulator ids */
enum S2DOS03_regulators {
	S2DOS03_LDO1,
	S2DOS03_LDO2,
	S2DOS03_LDO3,
	S2DOS03_LDO4,
	S2DOS03_BUCK1,
	S2DOS03_REG_MAX,
};

#define S2DOS03_BUCK_MIN1	500000
#define S2DOS03_LDO_MIN1	600000
#define S2DOS03_BUCK_STEP1	6250
#define S2DOS03_LDO_STEP1	25000
#define S2DOS03_LDO_VSEL_MASK	0x7F
#define S2DOS03_BUCK_VSEL_MASK	0xFF

#define S2DOS03_ENABLE_MASK_L1	1 << 0
#define S2DOS03_ENABLE_MASK_L2	1 << 1
#define S2DOS03_ENABLE_MASK_L3	1 << 2
#define S2DOS03_ENABLE_MASK_L4	1 << 3
#define S2DOS03_ENABLE_MASK_B1	1 << 4

#define S2DOS03_RAMP_DELAY	12000

#define S2DOS03_ENABLE_TIME_LDO		50
#define S2DOS03_ENABLE_TIME_BUCK	350

#define S2DOS03_ENABLE_SHIFT	0x06
#define S2DOS03_LDO_N_VOLTAGES	(S2DOS03_LDO_VSEL_MASK + 1)
#define S2DOS03_BUCK_N_VOLTAGES (S2DOS03_BUCK_VSEL_MASK + 1)

#define S2DOS03_PMIC_EN_SHIFT	6
#define S2DOS03_REGULATOR_MAX (S2DOS03_REG_MAX)

extern int s2dos03_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int s2dos03_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf);
extern int s2dos03_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int s2dos03_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf);

extern int s2dos03_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

#endif /*  __LINUX_MFD_S2DOS03_H */
