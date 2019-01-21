/*
 * STMicroelectronics lsm6ds3 core driver
 *
 * Copyright 2014 STMicroelectronics Inc.
 *
 * Denis Ciocca <denis.ciocca@st.com>
 *
 * Licensed under the GPL-2.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/unaligned.h>
#include <linux/input.h>
#include "st_lsm6ds3.h"
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>

#ifdef TAG
#undef TAG
#define TAG "[ACCEL]"
#endif

#define MS_TO_NS(msec)				((msec) * 1000 * 1000)

#define ST_LSM6DS3_DATA_FW			"st_lsm6ds3_data_fw"

/* ZGL: 180mg @ 4G */
#define ST_LSM6DS3_ACC_MIN_ZGL_XY		((int)(-180/0.122f) - 1)
#define ST_LSM6DS3_ACC_MAX_ZGL_XY		((int)(180/0.122f) + 1)
#define ST_LSM6DS3_ACC_MIN_ZGL_Z		((int)((1000-210)/0.122f) - 1)
#define ST_LSM6DS3_ACC_MAX_ZGL_Z		((int)((1000+210)/0.122f) + 1)
/* Selftest: 90~1700mg @ 2G */
#define ST_LSM6DS3_ACC_MIN_ST			((int)(90/0.061f))
#define ST_LSM6DS3_ACC_MAX_ST			((int)(1700/0.061f) + 1)
#define ST_LSM6DS3_ACC_DA_RETRY_COUNT		5

/* ZRL: 40dps @ 2000dps */
#define ST_LSM6DS3_GYR_MIN_ZRL			((int)(-40/0.07f) - 1)
#define ST_LSM6DS3_GYR_MAX_ZRL			((int)(40/0.07f) + 1)
#define ST_LSM6DS3_GYR_ZRL_DELTA		((int)(6/0.07f) + 1)
/* Selftest: 250~700dps @ 2000dps */
#define ST_LSM6DS3_GYR_MIN_ST			((int)(200/0.07f))
#define ST_LSM6DS3_GYR_MAX_ST			((int)(700/0.07f) + 1)
#define ST_LSM6DS3_GYR_DA_RETRY_COUNT		5

#ifndef ABS
#define ABS(a)		((a) > 0 ? (a) : -(a))
#endif

#define ACCEL_LOG_TIME                15 /* 15 sec */
#define DEGREE_TO_RAD(deg) (((deg) * 314159ULL + 9000000ULL) / 18000000ULL)
#define G_TO_M_S_2(g) ((g) * 980665ULL / 100000ULL)

/* COMMON VALUES FOR ACCEL-GYRO SENSORS */
#define ST_LSM6DS3_WAI_ADDRESS			0x0f
#define ST_LSM6DS3_WAI_EXP			0x69
#define ST_LSM6DS3_AXIS_EN_MASK			0x38
#define ST_LSM6DS3_INT1_ADDR			0x0d
#define ST_LSM6DS3_INT2_ADDR			0x0e
#define ST_LSM6DS3_MD1_ADDR			0x5e
#define ST_LSM6DS3_MD2_ADDR			0x5f
#define ST_LSM6DS3_FIFO_CTRL1_ADDR		0x06
#define ST_LSM6DS3_FIFO_CTRL2_ADDR		0x07
#define ST_LSM6DS3_FIFO_CTRL3_ADDR		0x08
#define ST_LSM6DS3_FIFO_CTRL4_ADDR		0x09
#define ST_LSM6DS3_FIFO_CTRL5_ADDR		0x0a
#define ST_LSM6DS3_FIFO_STAT1_ADDR		0x3a
#define ST_LSM6DS3_FIFO_STAT2_ADDR		0x3b
#define ST_LSM6DS3_FIFO_STAT3_ADDR		0x3c
#define ST_LSM6DS3_FIFO_STAT4_ADDR		0x3d
#define ST_LSM6DS3_FIFO_OUT_L_ADDR		0x3e
#define ST_LSM6DS3_CTRL1_ADDR			0x10
#define ST_LSM6DS3_CTRL2_ADDR			0x11
#define ST_LSM6DS3_CTRL3_ADDR			0x12
#define ST_LSM6DS3_CTRL4_ADDR			0x13
#define ST_LSM6DS3_CTRL5_ADDR			0x14
#define ST_LSM6DS3_CTRL6_ADDR			0x15
#define ST_LSM6DS3_CTRL7_ADDR			0x16
#define ST_LSM6DS3_CTRL8_ADDR			0x17
#define ST_LSM6DS3_CTRL9_ADDR			0x18
#define ST_LSM6DS3_CTRL10_ADDR			0x19
#define ST_LSM6DS3_OUT_TEMP_L_ADDR		0x20
#define ST_LSM6DS3_WU_SRC_ADDR			0x1b
#define ST_LSM6DS3_WU_THS_ADDR			0x5b
#define ST_LSM6DS3_WU_DUR_ADDR			0x5c
#define ST_LSM6DS3_ODR_LIST_NUM			8
#define ST_LSM6DS3_ODR_POWER_OFF_VAL		0x00
#define ST_LSM6DS3_ODR_13HZ_VAL			0x01
#define ST_LSM6DS3_ODR_26HZ_VAL			0x02
#define ST_LSM6DS3_ODR_52HZ_VAL			0x03
#define ST_LSM6DS3_ODR_104HZ_VAL		0x04
#define ST_LSM6DS3_ODR_208HZ_VAL		0x05
#define ST_LSM6DS3_ODR_416HZ_VAL		0x06
#define ST_LSM6DS3_ODR_833HZ_VAL		0x07
#define ST_LSM6DS3_ODR_1660HZ_VAL		0x08
#define ST_LSM6DS3_ODR_3330HZ_VAL		0x09
#define ST_LSM6DS3_ODR_6660HZ_VAL		0x0a
#define ST_LSM6DS3_ACCEL_BW_LIST_NUM		4
#define ST_LSM6DS3_ACCEL_BW_50HZ_VAL		0x03
#define ST_LSM6DS3_ACCEL_BW_100HZ_VAL		0x02
#define ST_LSM6DS3_ACCEL_BW_200HZ_VAL		0x01
#define ST_LSM6DS3_ACCEL_BW_400HZ_VAL		0x00
#define ST_LSM6DS3_FS_LIST_NUM			4
#define ST_LSM6DS3_BDU_ADDR			0x12
#define ST_LSM6DS3_BDU_MASK			0x40
#define ST_LSM6DS3_EN_BIT			0x01
#define ST_LSM6DS3_DIS_BIT			0x00
#define ST_LSM6DS3_FUNC_EN_ADDR			0x19
#define ST_LSM6DS3_FUNC_EN_MASK			0x04
#define ST_LSM6DS3_FUNC_CFG_ACCESS_ADDR		0x01
#define ST_LSM6DS3_FUNC_CFG_ACCESS_MASK		0x01
#define ST_LSM6DS3_FUNC_CFG_ACCESS_MASK2	0x04
#define ST_LSM6DS3_FUNC_CFG_REG2_MASK		0x80
#define ST_LSM6DS3_FUNC_CFG_START1_ADDR		0x62
#define ST_LSM6DS3_FUNC_CFG_START2_ADDR		0x63
#define ST_LSM6DS3_FUNC_CFG_DATA_WRITE_ADDR	0x64
#define ST_LSM6DS3_SELFTEST_ADDR		0x14
#define ST_LSM6DS3_SELFTEST_ACCEL_MASK		0x03
#define ST_LSM6DS3_SELFTEST_GYRO_MASK		0x0c
#define ST_LSM6DS3_GYRO_READY_TIME_FROM_PD_MS	150
#define ST_LSM6DS3_ACCEL_READY_TIME_FROM_PD_MS	10
#define ST_LSM6DS3_DEFAULT_STEP_C_MAXDR_MS	(1000UL)
#define ST_LSM6DS3_DEFAULT_HRTIMER_ODR_MS	(10UL)
#define ST_LSM6DS3_SELF_TEST_DISABLED_VAL	0x00
#define ST_LSM6DS3_SELF_TEST_POS_SIGN_VAL	0x01
#define ST_LSM6DS3_SELF_TEST_NEG_ACCEL_SIGN_VAL	0x02
#define ST_LSM6DS3_SELF_TEST_NEG_GYRO_SIGN_VAL	0x03
#define ST_LSM6DS3_FLASH_ADDR			0x00
#define ST_LSM6DS3_FLASH_MASK			0x40
#define ST_LSM6DS3_ORIENT_ADDRESS		0x4d
#define ST_LSM6DS3_ORIENT_MASK			0x07
#define ST_LSM6DS3_ORIENT_MODULE1		0x00
#define ST_LSM6DS3_ORIENT_MODULE2		0x05
#define ST_LSM6DS3_GAIN_X_ADDRESS		0x44
#define ST_LSM6DS3_GAIN_MASK			0x7f
#define ST_LSM6DS3_AD_FEATURES_ADDRESS		0x43
#define ST_LSM6DS3_AD_FEATURES_MASK		0x04
#define ST_LSM6DS3_LIR_ADDR			0x58
#define ST_LSM6DS3_LIR_MASK			0x01
#define ST_LSM6DS3_RESET_ADDR			0x12
#define ST_LSM6DS3_RESET_MASK			0x01
#define ST_LSM6DS3_FIFO_TEST_DEPTH		32
#define ST_LSM6DS3_SA_DYNAMIC_THRESHOLD		50 /* mg */

/* CUSTOM VALUES FOR ACCEL SENSOR */
#define ST_LSM6DS3_ACCEL_ODR_ADDR		0x10
#define ST_LSM6DS3_ACCEL_ODR_MASK		0xf0
#define ST_LSM6DS3_ACCEL_BW_ADDR		0x10
#define ST_LSM6DS3_ACCEL_BW_MASK		0x03
#define ST_LSM6DS3_ACCEL_FS_ADDR		0x10
#define ST_LSM6DS3_ACCEL_FS_MASK		0x0c
#define ST_LSM6DS3_ACCEL_FS_2G_VAL		0x00
#define ST_LSM6DS3_ACCEL_FS_4G_VAL		0x02
#define ST_LSM6DS3_ACCEL_FS_8G_VAL		0x03
#define ST_LSM6DS3_ACCEL_FS_16G_VAL		0x01
#define ST_LSM6DS3_ACCEL_FS_2G_GAIN		G_TO_M_S_2(61)
#define ST_LSM6DS3_ACCEL_FS_4G_GAIN		G_TO_M_S_2(122)
#define ST_LSM6DS3_ACCEL_FS_8G_GAIN		G_TO_M_S_2(244)
#define ST_LSM6DS3_ACCEL_FS_16G_GAIN		G_TO_M_S_2(488)
#define ST_LSM6DS3_ACCEL_OUT_X_L_ADDR		0x28
#define ST_LSM6DS3_ACCEL_OUT_Y_L_ADDR		0x2a
#define ST_LSM6DS3_ACCEL_OUT_Z_L_ADDR		0x2c
#define ST_LSM6DS3_ACCEL_AXIS_EN_ADDR		0x18
#define ST_LSM6DS3_ACCEL_DRDY_IRQ_MASK		0x01

/* CUSTOM VALUES FOR GYRO SENSOR */
#define ST_LSM6DS3_GYRO_ODR_ADDR		0x11
#define ST_LSM6DS3_GYRO_ODR_MASK		0xf0
#define ST_LSM6DS3_GYRO_FS_ADDR			0x11
#define ST_LSM6DS3_GYRO_FS_MASK			0x0c
#define ST_LSM6DS3_GYRO_FS_245_VAL		0x00
#define ST_LSM6DS3_GYRO_FS_500_VAL		0x01
#define ST_LSM6DS3_GYRO_FS_1000_VAL		0x02
#define ST_LSM6DS3_GYRO_FS_2000_VAL		0x03
#define ST_LSM6DS3_GYRO_FS_245_GAIN		DEGREE_TO_RAD(4375)
#define ST_LSM6DS3_GYRO_FS_500_GAIN		DEGREE_TO_RAD(8750)
#define ST_LSM6DS3_GYRO_FS_1000_GAIN		DEGREE_TO_RAD(17500)
#define ST_LSM6DS3_GYRO_FS_2000_GAIN		DEGREE_TO_RAD(70000)
#define ST_LSM6DS3_GYRO_OUT_X_L_ADDR		0x22
#define ST_LSM6DS3_GYRO_OUT_Y_L_ADDR		0x24
#define ST_LSM6DS3_GYRO_OUT_Z_L_ADDR		0x26
#define ST_LSM6DS3_GYRO_AXIS_EN_ADDR		0x19
#define ST_LSM6DS3_GYRO_DRDY_IRQ_MASK		0x02

/* CUSTOM VALUES FOR SIGNIFICANT MOTION SENSOR */
#define ST_LSM6DS3_SIGN_MOTION_EN_ADDR		0x19
#define ST_LSM6DS3_SIGN_MOTION_EN_MASK		0x01
#define ST_LSM6DS3_SIGN_MOTION_DRDY_IRQ_MASK	0x40

/* CUSTOM VALUES FOR STEP COUNTER SENSOR */
#define ST_LSM6DS3_STEP_COUNTER_EN_ADDR		0x58
#define ST_LSM6DS3_STEP_COUNTER_EN_MASK		0x40
#define ST_LSM6DS3_STEP_COUNTER_DRDY_IRQ_MASK	0x80
#define ST_LSM6DS3_STEP_COUNTER_OUT_L_ADDR	0x4b
#define ST_LSM6DS3_STEP_COUNTER_RES_ADDR	0x19
#define ST_LSM6DS3_STEP_COUNTER_RES_MASK	0x02
#define ST_LSM6DS3_STEP_COUNTER_THS_ADDR	0x0f
#define ST_LSM6DS3_STEP_COUNTER_THS_MASK	0x1f
#define ST_LSM6DS3_STEP_COUNTER_THS_VALUE	0x0f

/* CUSTOM VALUES FOR TILT SENSOR */
#define ST_LSM6DS3_TILT_EN_ADDR			0x58
#define ST_LSM6DS3_TILT_EN_MASK			0x20
#define ST_LSM6DS3_TILT_DRDY_IRQ_MASK		0x02

#define ST_LSM6DS3_LPF_ENABLE			0xf0
#define ST_LSM6DS3_LPF_DISABLE			0x00

#define ST_LSM6DS3_ENABLE_AXIS			0x07
#define ST_LMS6DS3_PRINT_SEC			10
#define ST_GYRO_SKIP_DELAY			80000000L /* 80ms */

#define ST_LSM6DS3_SRC_WAKE_UP_REG		0x1b
#define ST_LSM6DS3_SRC_ACCEL_GYRO_REG		0x1e
#define ST_LSM6DS3_SRC_FUNC_REG			0x53

#define ST_LSM6DS3_SRC_ACCEL_DATA_AVL		0x01
#define ST_LSM6DS3_SRC_GYRO_DATA_AVL		0x02
#define ST_LSM6DS3_SRC_SIGN_MOTION_DATA_AVL	0x40
#define ST_LSM6DS3_SRC_STEP_COUNTER_DATA_AVL	0x10
#define ST_LSM6DS3_SRC_TILT_DATA_AVL		0x20
#define ST_LSM6DS3_SRC_WU_AVL			0x0f

#define ST_LSM6DS3_ODR_13HZ			0
#define ST_LSM6DS3_ODR_26HZ			1
#define ST_LSM6DS3_ODR_52HZ			2
#define ST_LSM6DS3_ODR_104HZ			3
#define ST_LSM6DS3_ODR_208HZ			4
#define ST_LSM6DS3_ODR_416HZ			5
#define ST_LSM6DS3_ODR_1660HZ			6
#define ST_LSM6DS3_ODR_6660HZ			7

#define ST_LSM6DS3_ACCEL_FS_2G			0
#define ST_LSM6DS3_ACCEL_FS_4G			1
#define ST_LSM6DS3_ACCEL_FS_8G			2
#define ST_LSM6DS3_ACCEL_FS_16G			3

#define ST_LSM6DS3_GYRO_FS_245			0
#define ST_LSM6DS3_GYRO_FS_500			1
#define ST_LSM6DS3_GYRO_FS_1000			2
#define ST_LSM6DS3_GYRO_FS_2000			3

#undef ST_LSM6DS3_REG_DUMP_TEST

#define SENSOR_DATA_X(datax, datay, dataz, x1, y1, z1, x2, y2, z2, x3, y3, z3) \
				((x1 == 1 ? datax : (x1 == -1 ? -datax : 0)) + \
				(x2 == 1 ? datay : (x2 == -1 ? -datay : 0)) + \
				(x3 == 1 ? dataz : (x3 == -1 ? -dataz : 0)))

#define SENSOR_DATA_Y(datax, datay, dataz, x1, y1, z1, x2, y2, z2, x3, y3, z3) \
				((y1 == 1 ? datax : (y1 == -1 ? -datax : 0)) + \
				(y2 == 1 ? datay : (y2 == -1 ? -datay : 0)) + \
				(y3 == 1 ? dataz : (y3 == -1 ? -dataz : 0)))

#define SENSOR_DATA_Z(datax, datay, dataz, x1, y1, z1, x2, y2, z2, x3, y3, z3) \
				((z1 == 1 ? datax : (z1 == -1 ? -datax : 0)) + \
				(z2 == 1 ? datay : (z2 == -1 ? -datay : 0)) + \
				(z3 == 1 ? dataz : (z3 == -1 ? -dataz : 0)))

#define SENSOR_X_DATA(...)			SENSOR_DATA_X(__VA_ARGS__)
#define SENSOR_Y_DATA(...)			SENSOR_DATA_Y(__VA_ARGS__)
#define SENSOR_Z_DATA(...)			SENSOR_DATA_Z(__VA_ARGS__)

static const u8 st_lsm6ds3_data[] = {
	#include "st_lsm6ds3_custom_data.dat"
};
DECLARE_BUILTIN_FIRMWARE(ST_LSM6DS3_DATA_FW, st_lsm6ds3_data);

static struct st_lsm6ds3_gain_table {
	u8 code;
	u16 value;
} st_lsm6ds3_gain_table[] = {
	{ .code = 0x00, .value = 5005 },
	{ .code = 0x01, .value = 4813 },
	{ .code = 0x02, .value = 4635 },
	{ .code = 0x03, .value = 5959 },
	{ .code = 0x04, .value = 5753 },
	{ .code = 0x05, .value = 5562 },
	{ .code = 0x06, .value = 5382 },
	{ .code = 0x07, .value = 5214 },
	{ .code = 0x08, .value = 5056 },
	{ .code = 0x09, .value = 4907 },
	{ .code = 0x0a, .value = 4767 },
	{ .code = 0x0b, .value = 4635 },
	{ .code = 0x0c, .value = 5637 },
	{ .code = 0x0d, .value = 5488 },
	{ .code = 0x0e, .value = 5348 },
	{ .code = 0x0f, .value = 5214 },
	{ .code = 0x10, .value = 5087 },
	{ .code = 0x11, .value = 4966 },
	{ .code = 0x12, .value = 4850 },
	{ .code = 0x13, .value = 4740 },
	{ .code = 0x14, .value = 4635 },
	{ .code = 0x15, .value = 6348 },
	{ .code = 0x16, .value = 6212 },
	{ .code = 0x17, .value = 6083 },
	{ .code = 0x18, .value = 5959 },
	{ .code = 0x19, .value = 5840 },
	{ .code = 0x1a, .value = 5725 },
	{ .code = 0x1b, .value = 5615 },
	{ .code = 0x1c, .value = 5509 },
	{ .code = 0x1d, .value = 5407 },
	{ .code = 0x1e, .value = 6067 },
	{ .code = 0x1f, .value = 5959 },
	{ .code = 0x20, .value = 5854 },
	{ .code = 0x21, .value = 5753 },
	{ .code = 0x22, .value = 5656 },
	{ .code = 0x23, .value = 5561 },
	{ .code = 0x24, .value = 5470 },
	{ .code = 0x25, .value = 5382 },
	{ .code = 0x26, .value = 5297 },
	{ .code = 0x40, .value = 5005 },
	{ .code = 0x41, .value = 5212 },
	{ .code = 0x42, .value = 5441 },
	{ .code = 0x43, .value = 5688 },
	{ .code = 0x44, .value = 5959 },
	{ .code = 0x45, .value = 6257 },
	{ .code = 0x46, .value = 6586 },
	{ .code = 0x47, .value = 4635 },
	{ .code = 0x48, .value = 4907 },
	{ .code = 0x49, .value = 5214 },
	{ .code = 0x4a, .value = 5562 },
	{ .code = 0x4b, .value = 5959 },
	{ .code = 0x4c, .value = 6417 },
	{ .code = 0x4d, .value = 6952 },
	{ .code = 0x4e, .value = 7584 },
	{ .code = 0x4f, .value = 8343 },
};

static struct st_lsm6ds3_selftest_table {
	char *string_mode;
	u8 accel_value;
	u8 gyro_value;
	u8 accel_mask;
	u8 gyro_mask;
} st_lsm6ds3_selftest_table[] = {
	[0] = {
		.string_mode = "disabled",
		.accel_value = ST_LSM6DS3_SELF_TEST_DISABLED_VAL,
		.gyro_value = ST_LSM6DS3_SELF_TEST_DISABLED_VAL,
	},
	[1] = {
		.string_mode = "positive-sign",
		.accel_value = ST_LSM6DS3_SELF_TEST_POS_SIGN_VAL,
		.gyro_value = ST_LSM6DS3_SELF_TEST_POS_SIGN_VAL
	},
	[2] = {
		.string_mode = "negative-sign",
		.accel_value = ST_LSM6DS3_SELF_TEST_NEG_ACCEL_SIGN_VAL,
		.gyro_value = ST_LSM6DS3_SELF_TEST_NEG_GYRO_SIGN_VAL
	},
};

struct st_lsm6ds3_odr_reg {
	unsigned int hz;
	unsigned long ns;
	u8 value;
};

static struct st_lsm6ds3_odr_table {
	u8 addr[2];
	u8 mask[2];
	struct st_lsm6ds3_odr_reg odr_avl[ST_LSM6DS3_ODR_LIST_NUM];
} st_lsm6ds3_odr_table = {
	.addr[ST_INDIO_DEV_ACCEL] = ST_LSM6DS3_ACCEL_ODR_ADDR,
	.mask[ST_INDIO_DEV_ACCEL] = ST_LSM6DS3_ACCEL_ODR_MASK,
	.addr[ST_INDIO_DEV_GYRO] = ST_LSM6DS3_GYRO_ODR_ADDR,
	.mask[ST_INDIO_DEV_GYRO] = ST_LSM6DS3_GYRO_ODR_MASK,
	.odr_avl[0] = { .hz = 13,
			.value = ST_LSM6DS3_ODR_13HZ_VAL,
			.ns = (unsigned long)(NSEC_PER_SEC/13)},
	.odr_avl[1] = {	.hz = 26,
			.value = ST_LSM6DS3_ODR_26HZ_VAL,
			.ns = (unsigned long)(NSEC_PER_SEC/26)},
	.odr_avl[2] = {	.hz = 52,
			.value = ST_LSM6DS3_ODR_52HZ_VAL,
			.ns = (unsigned long)(NSEC_PER_SEC/52)},
	.odr_avl[3] = {	.hz = 104,
			.value = ST_LSM6DS3_ODR_104HZ_VAL,
			.ns = (unsigned long)(NSEC_PER_SEC/104)},
	.odr_avl[4] = { .hz = 208,
			.value = ST_LSM6DS3_ODR_208HZ_VAL,
			.ns = (unsigned long)(NSEC_PER_SEC/208)},
	.odr_avl[5] = { .hz = 416,
			.value = ST_LSM6DS3_ODR_416HZ_VAL,
			.ns = (unsigned long)(NSEC_PER_SEC/416)},
	.odr_avl[6] = { .hz = 1660,
			.value = ST_LSM6DS3_ODR_1660HZ_VAL,
			.ns = (unsigned long)(NSEC_PER_SEC/1660)},
	.odr_avl[7] = { .hz = 6660,
			.value = ST_LSM6DS3_ODR_6660HZ_VAL,
			.ns = (unsigned long)(NSEC_PER_SEC/6660)},
};

struct st_lsm6ds3_fs_reg {
	unsigned int gain;
	u8 value;
};

static struct st_lsm6ds3_fs_table {
	u8 addr;
	u8 mask;
	struct st_lsm6ds3_fs_reg fs_avl[ST_LSM6DS3_FS_LIST_NUM];
} st_lsm6ds3_fs_table[ST_INDIO_DEV_NUM] = {
	[ST_INDIO_DEV_ACCEL] = {
		.addr = ST_LSM6DS3_ACCEL_FS_ADDR,
		.mask = ST_LSM6DS3_ACCEL_FS_MASK,
		.fs_avl[0] = {	.gain = ST_LSM6DS3_ACCEL_FS_2G_GAIN,
				.value = ST_LSM6DS3_ACCEL_FS_2G_VAL },
		.fs_avl[1] = {	.gain = ST_LSM6DS3_ACCEL_FS_4G_GAIN,
				.value = ST_LSM6DS3_ACCEL_FS_4G_VAL },
		.fs_avl[2] = {	.gain = ST_LSM6DS3_ACCEL_FS_8G_GAIN,
				.value = ST_LSM6DS3_ACCEL_FS_8G_VAL },
		.fs_avl[3] = {	.gain = ST_LSM6DS3_ACCEL_FS_16G_GAIN,
				.value = ST_LSM6DS3_ACCEL_FS_16G_VAL },
	},
	[ST_INDIO_DEV_GYRO] = {
		.addr = ST_LSM6DS3_GYRO_FS_ADDR,
		.mask = ST_LSM6DS3_GYRO_FS_MASK,
		.fs_avl[0] = {	.gain = ST_LSM6DS3_GYRO_FS_245_GAIN,
				.value = ST_LSM6DS3_GYRO_FS_245_VAL },
		.fs_avl[1] = {	.gain = ST_LSM6DS3_GYRO_FS_500_GAIN,
				.value = ST_LSM6DS3_GYRO_FS_500_VAL },
		.fs_avl[2] = {	.gain = ST_LSM6DS3_GYRO_FS_1000_GAIN,
				.value = ST_LSM6DS3_GYRO_FS_1000_VAL },
		.fs_avl[3] = {	.gain = ST_LSM6DS3_GYRO_FS_2000_GAIN,
				.value = ST_LSM6DS3_GYRO_FS_2000_VAL },
	}
};

int st_lsm6ds3_acc_open_calibration(struct lsm6ds3_data *cdata)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	SENSOR_INFO("\n");

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);

		cdata->accel_cal_data[0] = 0;
		cdata->accel_cal_data[1] = 0;
		cdata->accel_cal_data[2] = 0;

		SENSOR_INFO("Can't open calibration file\n");
		return ret;
	}

	ret = cal_filp->f_op->read(cal_filp, (char *)&cdata->accel_cal_data,
		3 * sizeof(s16), &cal_filp->f_pos);
	if (ret != 3 * sizeof(s16)) {

		cdata->accel_cal_data[0] = 0;
		cdata->accel_cal_data[1] = 0;
		cdata->accel_cal_data[2] = 0;

		SENSOR_ERR("Can't read the cal data\n");
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	SENSOR_INFO("%d, %d, %d\n",
		cdata->accel_cal_data[0], cdata->accel_cal_data[1],
		cdata->accel_cal_data[2]);

	if ((cdata->accel_cal_data[0] == 0) && (cdata->accel_cal_data[1] == 0)
		&& (cdata->accel_cal_data[2] == 0))
		return -EIO;

	return ret;
}

int st_lsm6ds3_write_data_with_mask(struct lsm6ds3_data *common_data,
			u8 reg_addr, u8 mask, u8 data)
{
	int err;
	u8 new_data = 0x00, bak_data;

	err = common_data->tf->read(&common_data->tb,
			common_data->dev, reg_addr, 1, &new_data);
	if (err < 0)
		return err;

	bak_data = new_data;
	new_data = ((new_data & (~mask)) | ((data << __ffs(mask)) & mask));

	if (new_data != bak_data)
		err = common_data->tf->write(&common_data->tb,
				common_data->dev, reg_addr, 1, &new_data);
	return err;
}

int st_lsm6ds3_reg_write(struct lsm6ds3_data *cdata,
			u8 reg_addr, int len, u8 *data)
{
	int i;

	for (i = 0; i < len; i++) {
		int err = cdata->tf->write(&cdata->tb, cdata->dev,
						reg_addr + i, 1, &data[i]);
		if (err < 0) {
			SENSOR_ERR("failed to write reg = %d\n", *data);
			return err;
		}
	}

	return i;
}

int st_lsm6ds3_reg_read(struct lsm6ds3_data *cdata,
			u8 reg_addr, int len, u8 *data)
{
	int i;

	for (i = 0; i < len; i++) {
		int err = cdata->tf->read(&cdata->tb, cdata->dev,
						reg_addr + i, 1, &data[i]);
		if (err < 0) {
			SENSOR_ERR("failed to read reg = %d\n", reg_addr + i);
			return err;
		}
	}

	return i;
}

static int st_lsm6ds3_set_fs(struct lsm6ds3_data *cdata,
		unsigned int gain, int sindex)
{
	int err, i;

	for (i = 0; i < ST_LSM6DS3_FS_LIST_NUM; i++) {
		if (st_lsm6ds3_fs_table[sindex].fs_avl[i].gain == gain)
			break;
	}
	if (i == ST_LSM6DS3_FS_LIST_NUM)
		return -EINVAL;

	err = st_lsm6ds3_write_data_with_mask(cdata,
			st_lsm6ds3_fs_table[sindex].addr,
			st_lsm6ds3_fs_table[sindex].mask,
			st_lsm6ds3_fs_table[sindex].fs_avl[i].value);
	if (err < 0)
		return err;

	SENSOR_INFO("sindex = %d, fs = %d\n",
		sindex, st_lsm6ds3_fs_table[sindex].fs_avl[i].value);

	return 0;
}

int st_lsm6ds3_set_axis_enable(struct lsm6ds3_data *cdata, u8 value, int sindex)
{
	u8 reg_addr;

	switch (sindex) {
	case ST_INDIO_DEV_ACCEL:
		reg_addr = ST_LSM6DS3_ACCEL_AXIS_EN_ADDR;
		break;
	case ST_INDIO_DEV_GYRO:
		reg_addr = ST_LSM6DS3_GYRO_AXIS_EN_ADDR;
		break;
	default:
		return 0;
	}

	return st_lsm6ds3_write_data_with_mask(cdata,
				reg_addr, ST_LSM6DS3_AXIS_EN_MASK, value);
}

#ifdef ST_LSM6DS3_REG_DUMP_TEST
void st_lsm6ds3_reg_dump(struct lsm6ds3_data *cdata)
{
	int err, i;
	u8 regs[10] = {0,};

	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_INT1_ADDR, 2, regs);
	if (err < 0)
		SENSOR_ERR("failed to read int registers\n");

	for (i = 0; i < 2; i++)
		SENSOR_INFO("INT [%d]: %x\n", i, regs[i]);

	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_MD1_ADDR, 2, regs);
	if (err < 0)
		SENSOR_ERR("failed to read md registers\n");

	for (i = 0; i < 2; i++)
		SENSOR_INFO("MD [%d]: %x\n", i, regs[i]);

	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_FIFO_CTRL1_ADDR, 5, regs);
	if (err < 0)
		SENSOR_ERR("failed to read fifo registers\n");

	for (i = 0; i < 5; i++)
		SENSOR_INFO("FIFO [%d]: %x\n", i, regs[i]);

	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_CTRL1_ADDR, 10, regs);
	if (err < 0)
		SENSOR_ERR("failed to read ctrl registers\n");

	for (i = 0; i < 10; i++)
		SENSOR_INFO("CTRL [%d]: %x\n", i + 1, regs[i]);

	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_THS_ADDR, 2, regs);
	if (err < 0)
		SENSOR_ERR("failed to read wu registers\n");

	for (i = 0; i < 2; i++)
		SENSOR_INFO("WU [%d]: %x\n", i, regs[i]);

	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_SRC_ADDR, 1, regs);
	if (err < 0)
		SENSOR_ERR("failed to read wu registers\n");

	for (i = 0; i < 1; i++)
		SENSOR_INFO("WU SRC [%d]: %x\n", i, regs[i]);

	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_STEP_COUNTER_EN_ADDR,
								1, regs);
	if (err < 0)
		SENSOR_ERR("failed to read TAP_CFG registers\n");

	for (i = 0; i < 1; i++)
		SENSOR_INFO("TAP CFG [%d]: %x\n", i, regs[i]);
}
#endif

int st_lsm6ds3_acc_set_enable(struct lsm6ds3_data *cdata, bool enable)
{
	int err;

	if (enable) {
		err = st_lsm6ds3_write_data_with_mask(cdata,
		st_lsm6ds3_odr_table.addr[ST_INDIO_DEV_ACCEL],
		st_lsm6ds3_odr_table.mask[ST_INDIO_DEV_ACCEL],
		st_lsm6ds3_odr_table.odr_avl[ST_LSM6DS3_ODR_104HZ].value);
		if (err < 0)
			return err;

		cdata->sensors_enabled |= (1 << ST_INDIO_DEV_ACCEL);

		hrtimer_start(&cdata->acc_timer, cdata->delay,
							HRTIMER_MODE_REL);
	} else {
		if ((cdata->sensors_enabled & ST_LSM6DS3_EXTRA_DEPENDENCY)
			|| (cdata->sa_flag)) {
			err = st_lsm6ds3_write_data_with_mask(cdata,
			st_lsm6ds3_odr_table.addr[ST_INDIO_DEV_ACCEL],
			st_lsm6ds3_odr_table.mask[ST_INDIO_DEV_ACCEL],
			st_lsm6ds3_odr_table.odr_avl[ST_LSM6DS3_ODR_26HZ].value);
		} else {
			err = st_lsm6ds3_write_data_with_mask(cdata,
			st_lsm6ds3_odr_table.addr[ST_INDIO_DEV_ACCEL],
			st_lsm6ds3_odr_table.mask[ST_INDIO_DEV_ACCEL],
			ST_LSM6DS3_ODR_POWER_OFF_VAL);
		}

		if (err < 0)
			return err;

		cdata->sensors_enabled &= ~(1 << ST_INDIO_DEV_ACCEL);

		hrtimer_cancel(&cdata->acc_timer);
		cancel_work_sync(&cdata->acc_work);
	}

	return 0;
}

int st_lsm6ds3_gyro_set_enable(struct lsm6ds3_data *cdata, bool enable)
{
	int err;

	if (enable) {
		err = st_lsm6ds3_write_data_with_mask(cdata,
		st_lsm6ds3_odr_table.addr[ST_INDIO_DEV_GYRO],
		st_lsm6ds3_odr_table.mask[ST_INDIO_DEV_GYRO],
		st_lsm6ds3_odr_table.odr_avl[ST_LSM6DS3_ODR_208HZ].value);
		if (err < 0)
			return err;

		cdata->sensors_enabled |= (1 << ST_INDIO_DEV_GYRO);

		hrtimer_start(&cdata->gyro_timer, cdata->gyro_delay,
							HRTIMER_MODE_REL);
	} else {
		err = st_lsm6ds3_write_data_with_mask(cdata,
		st_lsm6ds3_odr_table.addr[ST_INDIO_DEV_GYRO],
		st_lsm6ds3_odr_table.mask[ST_INDIO_DEV_GYRO],
		ST_LSM6DS3_ODR_POWER_OFF_VAL);
		if (err < 0)
			return err;

		cdata->sensors_enabled &= ~(1 << ST_INDIO_DEV_GYRO);

		hrtimer_cancel(&cdata->gyro_timer);
		cancel_work_sync(&cdata->gyro_work);
	}

	return 0;
}

static int st_lsm6ds3_custom_data(struct lsm6ds3_data *cdata)
{
	int err;
	u8 data = 0x00;
	const struct firmware *fw;

	err = request_firmware(&fw, ST_LSM6DS3_DATA_FW, cdata->dev);
	if (err < 0)
		return err;

	err = cdata->tf->write(&cdata->tb, cdata->dev,
				ST_LSM6DS3_FUNC_CFG_ACCESS_ADDR, 1, &data);
	if (err < 0)
		goto realease_firmware;

	err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FUNC_CFG_ACCESS_ADDR,
				ST_LSM6DS3_FUNC_CFG_ACCESS_MASK,
				ST_LSM6DS3_EN_BIT);
	if (err < 0)
		goto realease_firmware;

	err = cdata->tf->write(&cdata->tb, cdata->dev,
				ST_LSM6DS3_FUNC_CFG_START1_ADDR, 1, &data);
	if (err < 0)
		goto realease_firmware;

	err = cdata->tf->write(&cdata->tb, cdata->dev,
				ST_LSM6DS3_FUNC_CFG_START2_ADDR, 1, &data);
	if (err < 0)
		goto realease_firmware;

	err = cdata->tf->write(&cdata->tb, cdata->dev,
				ST_LSM6DS3_FUNC_CFG_DATA_WRITE_ADDR,
				fw->size, (u8 *)fw->data);
	if (err < 0)
		goto realease_firmware;

	err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FUNC_CFG_ACCESS_ADDR,
				ST_LSM6DS3_FUNC_CFG_ACCESS_MASK,
				ST_LSM6DS3_DIS_BIT);
	if (err < 0)
		goto realease_firmware;

	err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FUNC_CFG_ACCESS_ADDR,
				ST_LSM6DS3_FUNC_CFG_ACCESS_MASK2,
				ST_LSM6DS3_EN_BIT);

realease_firmware:
	release_firmware(fw);
	return err < 0 ? err : 0;
}

static int st_lsm6ds3_gyro_selftest_parameters(struct lsm6ds3_data *cdata)
{
	int err, i;
	u8 orientation = 0x00, gain[3], temp_gain;
	bool found_x = false, found_y = false, found_z = false;

	err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FUNC_CFG_ACCESS_ADDR,
				ST_LSM6DS3_FUNC_CFG_REG2_MASK,
				ST_LSM6DS3_DIS_BIT);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FLASH_ADDR,
				ST_LSM6DS3_FLASH_MASK,
				ST_LSM6DS3_EN_BIT);
	if (err < 0)
		return err;

	err = cdata->tf->read(&cdata->tb, cdata->dev,
				ST_LSM6DS3_ORIENT_ADDRESS, 1, &orientation);
	if (err < 0)
		return err;

	orientation &= ST_LSM6DS3_ORIENT_MASK;

	err = cdata->tf->read(&cdata->tb, cdata->dev,
				ST_LSM6DS3_GAIN_X_ADDRESS, 3, gain);
	if (err < 0)
		return err;

	gain[0] &= ST_LSM6DS3_GAIN_MASK;
	gain[1] &= ST_LSM6DS3_GAIN_MASK;
	gain[2] &= ST_LSM6DS3_GAIN_MASK;

	if (orientation == ST_LSM6DS3_ORIENT_MODULE1) {
		cdata->gyro_module = 1;
	} else if (orientation == ST_LSM6DS3_ORIENT_MODULE2) {
		cdata->gyro_module = 2;
		temp_gain = gain[1];
		gain[1] = gain[2];
		gain[2] = temp_gain;
	} else {
		return -EINVAL;
	}

	err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FLASH_ADDR,
				ST_LSM6DS3_FLASH_MASK,
				ST_LSM6DS3_DIS_BIT);
	if (err < 0)
		return err;

	for (i = 0; i < ARRAY_SIZE(st_lsm6ds3_gain_table); i++) {
		if (st_lsm6ds3_gain_table[i].code == gain[0]) {
			cdata->gyro_selftest_delta[0] =
						st_lsm6ds3_gain_table[i].value;
			found_x = true;
		}
		if (st_lsm6ds3_gain_table[i].code == gain[1]) {
			cdata->gyro_selftest_delta[1] =
						st_lsm6ds3_gain_table[i].value;
			found_y = true;
		}
		if (st_lsm6ds3_gain_table[i].code == gain[2]) {
			cdata->gyro_selftest_delta[2] =
						st_lsm6ds3_gain_table[i].value;
			found_z = true;
		}
		if (found_x && found_y && found_z)
			break;
	}
	if (i == ARRAY_SIZE(st_lsm6ds3_gain_table))
		return -EINVAL;

	return 0;
}

static int st_lsm6ds3_read_ad_feature_version(struct lsm6ds3_data *cdata)
{
	int err;
	u8 patch_mode = 0x00;

	err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FUNC_CFG_ACCESS_ADDR,
				ST_LSM6DS3_FUNC_CFG_REG2_MASK,
				ST_LSM6DS3_DIS_BIT);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FLASH_ADDR,
				ST_LSM6DS3_FLASH_MASK,
				ST_LSM6DS3_EN_BIT);
	if (err < 0)
		return err;

	err = cdata->tf->read(&cdata->tb, cdata->dev,
				ST_LSM6DS3_AD_FEATURES_ADDRESS, 1, &patch_mode);
	if (err < 0)
		return err;

	patch_mode &= ST_LSM6DS3_AD_FEATURES_MASK;

	if (patch_mode)
		cdata->patch_feature = false;
	else
		cdata->patch_feature = true;

	err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FLASH_ADDR,
				ST_LSM6DS3_FLASH_MASK,
				ST_LSM6DS3_DIS_BIT);
	if (err < 0)
		return err;

	return 0;
}

static int st_lsm6ds3_init_sensor(struct lsm6ds3_data *cdata)
{
	int err;
	u8 val;

	val = 0x01;
	st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_CTRL3_ADDR, 1, &val);

	st_lsm6ds3_set_fs(cdata, cdata->sdata->acc_c_gain, ST_INDIO_DEV_ACCEL);
	st_lsm6ds3_set_fs(cdata, cdata->sdata->gyr_c_gain, ST_INDIO_DEV_GYRO);

	cdata->sign_motion_event_ready = false;
	cdata->gyro_selftest_status = 0;
	cdata->accel_selftest_status = 0;

	err = st_lsm6ds3_read_ad_feature_version(cdata);
	if (err < 0)
		return err;
	if (cdata->patch_feature) {
		err = st_lsm6ds3_gyro_selftest_parameters(cdata);
		if (err < 0)
			return err;
	}

	err = st_lsm6ds3_write_data_with_mask(cdata, ST_LSM6DS3_WU_THS_ADDR,
					0x3f, 0x3f);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata, ST_LSM6DS3_WU_DUR_ADDR,
					0x60, 0x00);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata, ST_LSM6DS3_LIR_ADDR,
					ST_LSM6DS3_LIR_MASK, ST_LSM6DS3_EN_BIT);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata, ST_LSM6DS3_BDU_ADDR,
					ST_LSM6DS3_BDU_MASK, ST_LSM6DS3_EN_BIT);
	if (err < 0)
		return err;

	if (cdata->patch_feature) {
		err = st_lsm6ds3_custom_data(cdata);
		if (err < 0)
			return err;
	}

	err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_STEP_COUNTER_RES_ADDR,
					ST_LSM6DS3_STEP_COUNTER_RES_MASK,
					ST_LSM6DS3_EN_BIT);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_STEP_COUNTER_RES_ADDR,
					ST_LSM6DS3_STEP_COUNTER_RES_MASK,
					ST_LSM6DS3_DIS_BIT);
	if (err < 0)
		return err;

	if (cdata->patch_feature) {
		err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_FUNC_CFG_ACCESS_ADDR,
					ST_LSM6DS3_FUNC_CFG_REG2_MASK,
					ST_LSM6DS3_EN_BIT);
		if (err < 0)
			return err;

		err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_STEP_COUNTER_THS_ADDR,
					ST_LSM6DS3_STEP_COUNTER_THS_MASK,
					ST_LSM6DS3_STEP_COUNTER_THS_VALUE);
		if (err < 0)
			return err;

		err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_FUNC_CFG_ACCESS_ADDR,
					ST_LSM6DS3_FUNC_CFG_REG2_MASK,
					ST_LSM6DS3_DIS_BIT);
		if (err < 0)
			return err;
	}

	return 0;
}

static int st_lsm6ds3_set_selftest(struct lsm6ds3_data *cdata,
				int index, int sindex)
{
	int err;
	u8 mode, mask;

	switch (sindex) {
	case ST_INDIO_DEV_ACCEL:
		mask = ST_LSM6DS3_SELFTEST_ACCEL_MASK;
		mode = st_lsm6ds3_selftest_table[index].accel_value;
		break;
	case ST_INDIO_DEV_GYRO:
		mask = ST_LSM6DS3_SELFTEST_GYRO_MASK;

		if (cdata->patch_feature)
			mode = st_lsm6ds3_selftest_table[0].gyro_value;
		else
			mode = st_lsm6ds3_selftest_table[index].gyro_value;

		break;
	default:
		return -EINVAL;
	}

	err = st_lsm6ds3_write_data_with_mask(cdata,
			ST_LSM6DS3_SELFTEST_ADDR, mask, mode);
	if (err < 0)
		return err;

	switch (sindex) {
	case ST_INDIO_DEV_ACCEL:
		cdata->accel_selftest_status = index;
		break;
	case ST_INDIO_DEV_GYRO:
		cdata->gyro_selftest_status = index;
		break;
	}

	return 0;
}

static int st_lsm6ds3_acc_hw_selftest(struct lsm6ds3_data *cdata,
		s32 *NOST, s32 *ST, s32 *DIFF_ST)
{
	int err;
	u8 buf = 0x00;
	s16 nOutData[3] = {0,};
	s32 i, retry;
	u8 testset_regs[10] = {0x30, 0x00, 0x44, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x38, 0x38};

	err = cdata->tf->write(&cdata->tb, cdata->dev,
			ST_LSM6DS3_CTRL1_ADDR, 10, testset_regs);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	mdelay(200);

	retry = ST_LSM6DS3_ACC_DA_RETRY_COUNT;
	do {
		usleep_range(1000, 1100);
		err = cdata->tf->read(&cdata->tb, cdata->dev, 0x1e, 1, &buf);
		if (err < 0)
			goto XL_HW_SELF_EXIT;

		retry--;
		if (!retry)
			break;
	} while (!(buf & 0x01));

	err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_ACCEL_OUT_X_L_ADDR, 6, (u8 *)nOutData);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	for (i = 0; i < 6; i++) {
		retry = ST_LSM6DS3_ACC_DA_RETRY_COUNT;
		do {
			usleep_range(1000, 1100);
			err = cdata->tf->read(&cdata->tb, cdata->dev,
				0x1e, 1, &buf);
			if (err < 0)
				goto XL_HW_SELF_EXIT;
			retry--;
			if (!retry)
				break;
		} while (!(buf & 0x01));

		err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_ACCEL_OUT_X_L_ADDR, 6, (u8 *)nOutData);
		if (err < 0)
			goto XL_HW_SELF_EXIT;

		if (i > 0) {
			NOST[0] += nOutData[0];
			NOST[1] += nOutData[1];
			NOST[2] += nOutData[2];
		}
	}

	NOST[0] /= 5;
	NOST[1] /= 5;
	NOST[2] /= 5;

	err = st_lsm6ds3_set_selftest(cdata, 1, ST_INDIO_DEV_ACCEL);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	mdelay(200);

	retry = ST_LSM6DS3_ACC_DA_RETRY_COUNT;
	do {
		usleep_range(1000, 1100);
		err = cdata->tf->read(&cdata->tb, cdata->dev,
					0x1e, 1, &buf);
		if (err < 0)
			goto XL_HW_SELF_EXIT;
		retry--;
		if (!retry)
			break;
	} while (!(buf & 0x01));

	err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_ACCEL_OUT_X_L_ADDR, 6, (u8 *)nOutData);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	for (i = 0; i < 6; i++) {
		retry = ST_LSM6DS3_ACC_DA_RETRY_COUNT;
		do {
			usleep_range(1000, 1100);
			err = cdata->tf->read(&cdata->tb, cdata->dev,
					0x1e, 1, &buf);
			if (err < 0)
				goto XL_HW_SELF_EXIT;

			retry--;
			if (!retry)
				break;
		} while (!(buf & 0x01));

		err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_ACCEL_OUT_X_L_ADDR, 6, (u8 *)nOutData);
		if (err < 0)
			goto XL_HW_SELF_EXIT;

		if (i > 0) {
			ST[0] += nOutData[0];
			ST[1] += nOutData[1];
			ST[2] += nOutData[2];
		}
	}

	ST[0] /= 5;
	ST[1] /= 5;
	ST[2] /= 5;

	buf = 0x00;
	err = cdata->tf->write(&cdata->tb, cdata->dev,
			ST_LSM6DS3_CTRL1_ADDR, 1, &buf);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	err = st_lsm6ds3_set_selftest(cdata, 0, ST_INDIO_DEV_ACCEL);
	if (err < 0)
		goto XL_HW_SELF_EXIT;

	retry = 0;
	for (i = 0; i < 3; i++) {
		DIFF_ST[i] = ABS(ST[i] - NOST[i]);
		if ((ST_LSM6DS3_ACC_MIN_ST > DIFF_ST[i])
			|| (ST_LSM6DS3_ACC_MAX_ST < DIFF_ST[i])) {
			retry++;
		}
	}

	if (retry > 0)
		return 0;
	else
		return 1;

XL_HW_SELF_EXIT:
	return err;
}

static int st_lsm6ds3_gyro_fifo_test(struct lsm6ds3_data *cdata,
		s16 *zero_rate_lsb, s32 *fifo_cnt, u8 *fifo_pass,
		s16 *slot_raw, char *out_str)
{
	int err;
	u8 buf[5] = {0x00,};
	s16 nFifoDepth = (ST_LSM6DS3_FIFO_TEST_DEPTH + 1) * 3;
	bool zero_rate_read_2nd = 0;
	s16 raw[3] = {0,}, zero_rate_delta[3] = {0,}, length = 0;
	s16 data[ST_LSM6DS3_FIFO_TEST_DEPTH * 3] = {0,};
	s32 i = 0, j = 0, sum_raw[3] = {0,};

	SENSOR_INFO("start\n");
	err = st_lsm6ds3_write_data_with_mask(cdata,
			ST_LSM6DS3_CTRL4_ADDR, 0x40, ST_LSM6DS3_DIS_BIT);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
			ST_LSM6DS3_CTRL4_ADDR, 0x01, ST_LSM6DS3_EN_BIT);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
			ST_LSM6DS3_CTRL2_ADDR, 0xf0, ST_LSM6DS3_ODR_104HZ_VAL);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
			ST_LSM6DS3_CTRL2_ADDR, 0x0c,
			ST_LSM6DS3_GYRO_FS_2000_VAL);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
			ST_LSM6DS3_CTRL3_ADDR, 0x40, ST_LSM6DS3_DIS_BIT);
	if (err < 0)
		return err;

	buf[0] = (u8)(nFifoDepth & 0xff);
	err = cdata->tf->write(&cdata->tb, cdata->dev,
			ST_LSM6DS3_FIFO_CTRL1_ADDR, 1, buf);
	if (err < 0)
		return err;

	buf[0] = (u8)((nFifoDepth >> 8) & 0x0f);
	err = st_lsm6ds3_write_data_with_mask(cdata,
			ST_LSM6DS3_FIFO_CTRL2_ADDR, 0x0f, buf[0]);
	if (err < 0)
		return err;

	buf[0] = (0x01 << 3);
	err = cdata->tf->write(&cdata->tb, cdata->dev,
			ST_LSM6DS3_FIFO_CTRL3_ADDR, 1, buf);
	if (err < 0)
		return err;

	buf[0] = 0x00;
	err = cdata->tf->write(&cdata->tb, cdata->dev,
			ST_LSM6DS3_FIFO_CTRL4_ADDR, 1, buf);
	if (err < 0)
		return err;

	buf[0] = (0x04 << 3) | 0x01;
	err = cdata->tf->write(&cdata->tb, cdata->dev,
			ST_LSM6DS3_FIFO_CTRL5_ADDR, 1, buf);
	if (err < 0)
		return err;

	mdelay(800);

read_zero_rate_again:
	err = st_lsm6ds3_write_data_with_mask(cdata,
			ST_LSM6DS3_FIFO_CTRL5_ADDR, 0x07, 0x00);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
			ST_LSM6DS3_FIFO_CTRL5_ADDR, 0x07, 0x01);
	if (err < 0)
		return err;

	length = ST_LSM6DS3_FIFO_TEST_DEPTH * 3;
	while (1) {
		mdelay(20);
		err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_FIFO_STAT1_ADDR, 4, buf);

		if (err < 0)
			return err;

		if (buf[1] & 0xA0)
			break;

		if ((--length) == 0) {
			SENSOR_ERR("fifo not filled\n");
			return -EBUSY;
		}
	}

	length = ST_LSM6DS3_FIFO_TEST_DEPTH * 3;
	for (i = 0; i < length; i++) {
		err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_FIFO_OUT_L_ADDR, 2, (u8 *)(data + i));
		if (err < 0) {
			SENSOR_ERR("Reading fifo output is fail\n");
			return err;
		}
	}

	*fifo_cnt = 0;
	/* Check fifo pass or fail */
	for (i = 0; i < length; i += 3) {
		*fifo_cnt += 1;

		raw[0] = data[i];
		raw[1] = data[i + 1];
		raw[2] = data[i + 2];

		sum_raw[0] += raw[0];
		sum_raw[1] += raw[1];
		sum_raw[2] += raw[2];

		for (j = 0; j < 3; j++) {
			if (raw[j] < ST_LSM6DS3_GYR_MIN_ZRL
				|| raw[j] > ST_LSM6DS3_GYR_MAX_ZRL) {
				slot_raw[0] = raw[0] * 7 / 100;
				slot_raw[1] = raw[1] * 7 / 100;
				slot_raw[2] = raw[2] * 7 / 100;
				*fifo_pass = 0;
				SENSOR_INFO("fifo fail = %d\n", *fifo_pass);
				return -EAGAIN;
			}
		}
	}

	for (i = 0; i < 3; i++) {
		zero_rate_lsb[i] = sum_raw[i];
		zero_rate_lsb[i] += (ST_LSM6DS3_FIFO_TEST_DEPTH / 2);
		zero_rate_lsb[i] /= ST_LSM6DS3_FIFO_TEST_DEPTH;
	}

	if (zero_rate_read_2nd == 1) {
		*fifo_pass = 1;
		SENSOR_INFO("fifo pass = %d\n", *fifo_pass);
		/* check zero_rate second time */
		zero_rate_delta[0] -= zero_rate_lsb[0];
		zero_rate_delta[1] -= zero_rate_lsb[1];
		zero_rate_delta[2] -= zero_rate_lsb[2];
		for (i = 0; i < 3; i++) {
			if (ABS(zero_rate_delta[i]) > ST_LSM6DS3_GYR_ZRL_DELTA)
				return -EAGAIN;
		}
	} else {
		/* check zero_rate first time, go to check again */
		zero_rate_read_2nd = 1;
		sum_raw[0] = 0;
		sum_raw[1] = 0;
		sum_raw[2] = 0;
		zero_rate_delta[0] = zero_rate_lsb[0];
		zero_rate_delta[1] = zero_rate_lsb[1];
		zero_rate_delta[2] = zero_rate_lsb[2];

		goto read_zero_rate_again;
	}

	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00;
	err = st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_FIFO_CTRL5_ADDR, 1, buf);
	if (err < 0)
		return err;

	return 0;
}

static int st_lsm6ds3_gyro_hw_selftest(struct lsm6ds3_data *cdata,
		s32 *NOST, s32 *ST, s32 *DIFF_ST)
{
	int err;
	u8 buf = 0x00;
	s16 nOutData[3] = {0,};
	s32 i, retry;
	u8 testset_regs[10] = {0x00, 0x5C, 0x44, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x38, 0x38};

	SENSOR_INFO("start\n");

	err = cdata->tf->write(&cdata->tb, cdata->dev,
				ST_LSM6DS3_CTRL1_ADDR, 10, testset_regs);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	mdelay(800);

	retry = ST_LSM6DS3_GYR_DA_RETRY_COUNT;
	do {
		usleep_range(1000, 1100);
		err = cdata->tf->read(&cdata->tb, cdata->dev, 0x1e, 1, &buf);
		if (err < 0)
			goto G_HW_SELF_EXIT;

		retry--;
		if (!retry)
			break;
	} while (!(buf & 0x02));

	err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_GYRO_OUT_X_L_ADDR, 6, (u8 *)nOutData);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	for (i = 0; i < 6; i++) {
		retry = ST_LSM6DS3_GYR_DA_RETRY_COUNT;
		do {
			usleep_range(1000, 1100);
			err = cdata->tf->read(&cdata->tb, cdata->dev,
					0x1e, 1, &buf);
			if (err < 0)
				goto G_HW_SELF_EXIT;

			retry--;
			if (!retry)
				break;
		} while (!(buf & 0x02));

		err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_GYRO_OUT_X_L_ADDR, 6, (u8 *)nOutData);
		if (err < 0)
			goto G_HW_SELF_EXIT;

		if (i > 0) {
			NOST[0] += nOutData[0];
			NOST[1] += nOutData[1];
			NOST[2] += nOutData[2];
		}
	}

	NOST[0] /= 5;
	NOST[1] /= 5;
	NOST[2] /= 5;

	err = st_lsm6ds3_set_selftest(cdata, 1, ST_INDIO_DEV_GYRO);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	mdelay(60);

	retry = ST_LSM6DS3_GYR_DA_RETRY_COUNT;
	do {
		usleep_range(1000, 1100);
		err = cdata->tf->read(&cdata->tb, cdata->dev,
					0x1e, 1, &buf);
		if (err < 0)
			goto G_HW_SELF_EXIT;

		retry--;
		if (!retry)
			break;
	} while (!(buf & 0x02));

	err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_GYRO_OUT_X_L_ADDR, 6, (u8 *)nOutData);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	for (i = 0; i < 6; i++) {
		retry = ST_LSM6DS3_GYR_DA_RETRY_COUNT;
		do {
			usleep_range(1000, 1100);
			err = cdata->tf->read(&cdata->tb, cdata->dev,
					0x1e, 1, &buf);
			if (err < 0)
				goto G_HW_SELF_EXIT;

			retry--;
			if (!retry)
				break;
		} while (!(buf & 0x02));

		err = cdata->tf->read(&cdata->tb, cdata->dev,
			ST_LSM6DS3_GYRO_OUT_X_L_ADDR, 6, (u8 *)nOutData);
		if (err < 0)
			goto G_HW_SELF_EXIT;

		if (i > 0) {
			ST[0] += nOutData[0];
			ST[1] += nOutData[1];
			ST[2] += nOutData[2];
			if ((cdata->patch_feature) &&
					(cdata->gyro_selftest_status > 0)) {
				if (cdata->gyro_selftest_status == 1) {
					ST[0] += cdata->gyro_selftest_delta[0];
					ST[1] += cdata->gyro_selftest_delta[1];
					ST[2] += cdata->gyro_selftest_delta[2];
				} else {
					ST[0] -= cdata->gyro_selftest_delta[0];
					ST[1] -= cdata->gyro_selftest_delta[1];
					ST[2] -= cdata->gyro_selftest_delta[2];
				}
			}
		}
	}

	ST[0] /= 5;
	ST[1] /= 5;
	ST[2] /= 5;

	buf = 0x00;
	err = cdata->tf->write(&cdata->tb, cdata->dev,
					ST_LSM6DS3_CTRL2_ADDR, 1, &buf);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	err = st_lsm6ds3_set_selftest(cdata, 0, ST_INDIO_DEV_GYRO);
	if (err < 0)
		goto G_HW_SELF_EXIT;

	retry = 0;
	for (i = 0; i < 3; i++) {
		DIFF_ST[i] = ABS(ST[i] - NOST[i]);
		if ((ST_LSM6DS3_GYR_MIN_ST > DIFF_ST[i])
			|| (ST_LSM6DS3_GYR_MAX_ST < DIFF_ST[i])) {
			retry++;
		}
	}

	if (retry > 0)
		return 0;
	else
		return 1;

G_HW_SELF_EXIT:
	return err;
}

static int st_lsm6ds3_selftest_run(struct lsm6ds3_data *cdata,
			int test, char *out_str, u8 sindex)
{
	int err;
	u8 zero[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	u8 backup_regs1[5] = {0,}, backup_regs2[10] = {0,};
	u8 backup_regs3[2] = {0,}, backup_regs4[2] = {0,};
	u8 init_status = 0, cal_pass = 0, fifo_pass = 0;
	s16 zero_rate_data[3] = {0,}, slot_raw[3] = {0,};
	s32 NOST[3] = {0,}, ST[3] = {0,}, DIFF_ST[3] = {0,};
	s32 fifo_count = 0, hw_st_ret = 0;

	SENSOR_INFO("start\n");
	/* check ID */
	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WAI_ADDRESS,
							1, &init_status);
	if (err < 0) {
		SENSOR_ERR("failed to read Who-Am-I register\n");
		return err;
	}

	if (init_status != ST_LSM6DS3_WAI_EXP) {
		SENSOR_ERR("Who-Am-I register is wrong %x\n", init_status);
		init_status = 0;
	} else {
		init_status = 1;
	}

	/* backup registers */
	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_INT1_ADDR, 2, backup_regs4);
	if (err < 0) {
		SENSOR_ERR("failed to read int registers\n");
		goto restore_exit;
	}
	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_MD1_ADDR, 2, backup_regs3);
	if (err < 0) {
		SENSOR_ERR("failed to read md registers\n");
		goto restore_int;
	}
	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_CTRL1_ADDR,
							10, backup_regs2);
	if (err < 0) {
		SENSOR_ERR("failed to read ctrl registers\n");
		goto restore_md;
	}
	err = st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_FIFO_CTRL1_ADDR,
							5, backup_regs1);
	if (err < 0) {
		SENSOR_ERR("failed to read fifo registers\n");
		goto restore_ctrl;
	}

	/* disable int */
	err = st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_MD1_ADDR, 2, zero);
	if (err < 0) {
		SENSOR_ERR("failed to write 0 into MD registers\n");
		goto restore_regs;
	}
	err = st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_INT1_ADDR, 2, zero);
	if (err < 0) {
		SENSOR_ERR("failed to write 0 into INT registers\n");
		goto restore_regs;
	}
	err = st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_FIFO_CTRL1_ADDR, 5, zero);
	if (err < 0) {
		SENSOR_ERR("failed to write 0 into fifo registers\n");
		goto restore_regs;
	}
	zero[2] = 0x04;
	zero[8] = 0x38;
	zero[9] = 0x38;
	err = st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_CTRL1_ADDR, 10, zero);
	if (err < 0) {
		SENSOR_ERR("failed to write 0 into CTRL registers\n");
		goto restore_regs;
	}

	/* fifo test */
	switch (sindex) {
	case ST_INDIO_DEV_ACCEL:
		if (test & 2) {
			hw_st_ret = st_lsm6ds3_acc_hw_selftest(cdata, NOST,
							ST, DIFF_ST);
			if (hw_st_ret > 0) {
				SENSOR_INFO("ST = 1 %d %d %d\n",
					DIFF_ST[0], DIFF_ST[1], DIFF_ST[2]);
			} else {
				SENSOR_INFO("ST = 0 %d %d %d\n",
					DIFF_ST[0], DIFF_ST[1], DIFF_ST[2]);

				goto restore_regs;
			}
		}
		break;
	case ST_INDIO_DEV_GYRO:
		if (test & 1) {
			err = st_lsm6ds3_gyro_fifo_test(cdata, zero_rate_data,
				&fifo_count, &fifo_pass, slot_raw, out_str);
			if (err < 0) {
				SENSOR_ERR("gyro fifo test fail = %d, fail raw = %d, %d, %d\n",
					fifo_count, slot_raw[0],
					slot_raw[1], slot_raw[2]);
				goto restore_regs;
			}
		}
		if (test & 2) {
			hw_st_ret = st_lsm6ds3_gyro_hw_selftest(cdata, NOST,
							ST, DIFF_ST);
			cal_pass = 1;
			zero_rate_data[0] = zero_rate_data[0] * 7 / 100;
			zero_rate_data[1] = zero_rate_data[1] * 7 / 100;
			zero_rate_data[2] = zero_rate_data[2] * 7 / 100;
			NOST[0] = NOST[0] * 7 / 100;
			NOST[1] = NOST[1] * 7 / 100;
			NOST[2] = NOST[2] * 7 / 100;
			ST[0] = ST[0] * 7 / 100;
			ST[1] = ST[1] * 7 / 100;
			ST[2] = ST[2] * 7 / 100;
			DIFF_ST[0] = DIFF_ST[0] * 7 / 100;
			DIFF_ST[1] = DIFF_ST[1] * 7 / 100;
			DIFF_ST[2] = DIFF_ST[2] * 7 / 100;

			SENSOR_INFO("zero_rate_data = %d, %d, %d, st = %d, %d, %d, nost = %d, %d, %d, diff_st = %d, %d, %d, fifo = %d, cal = %d\n",
				zero_rate_data[0], zero_rate_data[1],
				zero_rate_data[2], ST[0], ST[1], ST[2],
				NOST[0], NOST[1], NOST[2],
				DIFF_ST[0], DIFF_ST[1], DIFF_ST[2],
				fifo_pass, cal_pass);

			if (hw_st_ret > 0) {
				SENSOR_INFO("gyro selftest pass\n");
			} else {
				SENSOR_INFO("gyro selftest fail\n");
				goto restore_regs;
			}
		}
		break;
	}
restore_regs:
	/* backup registers */
	err = cdata->tf->write(&cdata->tb, cdata->dev,
				ST_LSM6DS3_FIFO_CTRL1_ADDR, 5, backup_regs1);
	if (err < 0)
		SENSOR_ERR("failed to write fifo registers\n");
restore_ctrl:
	err = cdata->tf->write(&cdata->tb, cdata->dev,
				ST_LSM6DS3_CTRL1_ADDR, 10, backup_regs2);
	if (err < 0)
		SENSOR_ERR("failed to write ctrl registers\n");
restore_md:
	err = cdata->tf->write(&cdata->tb, cdata->dev,
				ST_LSM6DS3_MD1_ADDR, 2, backup_regs3);
	if (err < 0)
		SENSOR_ERR("failed to write md registers\n");
restore_int:
	err = cdata->tf->write(&cdata->tb, cdata->dev,
				ST_LSM6DS3_INT1_ADDR, 2, backup_regs4);
	if (err < 0)
		SENSOR_ERR("failed to write int registers\n");
restore_exit:
	switch (sindex) {
	case ST_INDIO_DEV_ACCEL:
		if (hw_st_ret > 0) {
			return snprintf(out_str, PAGE_SIZE, "1,%d,%d,%d\n",
					DIFF_ST[0], DIFF_ST[1], DIFF_ST[2]);
		} else {
			return snprintf(out_str, PAGE_SIZE, "0,%d,%d,%d\n",
					DIFF_ST[0], DIFF_ST[1], DIFF_ST[2]);
		}
		break;

	case ST_INDIO_DEV_GYRO:
		if (!fifo_pass) {
			return snprintf(out_str, PAGE_SIZE, "%d,%d,%d\n",
				slot_raw[0], slot_raw[1], slot_raw[2]);
		} else {
			return snprintf(out_str, PAGE_SIZE,
				"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
				zero_rate_data[0], zero_rate_data[1],
				zero_rate_data[2], NOST[0], NOST[1], NOST[2],
				ST[0], ST[1], ST[2], DIFF_ST[0], DIFF_ST[1],
				DIFF_ST[2], fifo_pass, cal_pass);
		}
		break;
	}

	return 0;
}

static ssize_t st_lsm6ds3_gyro_sysfs_get_selftest_run(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lsm6ds3_data *cdata = dev_get_drvdata(dev);
	ssize_t ret;

	ret = st_lsm6ds3_selftest_run(cdata, cdata->sdata->st_mode, buf,
					ST_INDIO_DEV_GYRO);

	return ret;
}

static ssize_t st_lsm6ds3_acc_sysfs_get_selftest_run(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lsm6ds3_data *cdata = dev_get_drvdata(dev);
	ssize_t ret;

	ret = st_lsm6ds3_selftest_run(cdata, cdata->sdata->st_mode, buf,
					ST_INDIO_DEV_ACCEL);

	return ret;
}

static int st_lsm6ds3_set_extra_dependency(struct lsm6ds3_data *cdata,
								bool enable)
{
	int err;

	if (!(cdata->sensors_enabled & ST_LSM6DS3_ACCEL_DEPENDENCY)) {
		if (enable) {
			err = st_lsm6ds3_write_data_with_mask(cdata,
				st_lsm6ds3_odr_table.addr[ST_INDIO_DEV_ACCEL],
				st_lsm6ds3_odr_table.mask[ST_INDIO_DEV_ACCEL],
				st_lsm6ds3_odr_table.odr_avl[ST_LSM6DS3_ODR_26HZ].value);

			if (err < 0)
				return err;
		} else {
			err = st_lsm6ds3_write_data_with_mask(cdata,
				st_lsm6ds3_odr_table.addr[ST_INDIO_DEV_ACCEL],
				st_lsm6ds3_odr_table.mask[ST_INDIO_DEV_ACCEL],
				ST_LSM6DS3_ODR_POWER_OFF_VAL);

			if (err < 0)
				return err;
		}
	}

	if (!(cdata->sensors_enabled & ST_LSM6DS3_EXTRA_DEPENDENCY)) {
		if (enable) {
			err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FUNC_EN_ADDR,
				ST_LSM6DS3_FUNC_EN_MASK, ST_LSM6DS3_EN_BIT);

			if (err < 0)
				return err;
		} else {
			err = st_lsm6ds3_write_data_with_mask(cdata,
				ST_LSM6DS3_FUNC_EN_ADDR,
				ST_LSM6DS3_FUNC_EN_MASK, ST_LSM6DS3_DIS_BIT);

			if (err < 0)
				return err;
		}
	}

	return 0;
}

static int st_lsm6ds3_enable_step_c(struct lsm6ds3_data *cdata, bool enable)
{
	int err;
	u8 value = ST_LSM6DS3_DIS_BIT;

	if (enable)
		value = ST_LSM6DS3_EN_BIT;

	/* FUNC_EN */
	err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_CTRL10_ADDR,
					0x04, value);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_STEP_COUNTER_EN_ADDR,
					0x01, value);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_STEP_COUNTER_EN_ADDR,
					ST_LSM6DS3_STEP_COUNTER_EN_MASK, value);
	if (err < 0)
		return err;

	if (!(cdata->sensors_enabled & (1 << ST_INDIO_DEV_TILT))) {
		err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_TILT_EN_ADDR,
					ST_LSM6DS3_TILT_EN_MASK, value);
		if (err < 0)
			return err;
	}

	err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_INT1_ADDR,
					0x80, value);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_INT1_ADDR,
					ST_LSM6DS3_SIGN_MOTION_DRDY_IRQ_MASK,
					value);
	if (err < 0)
		return err;

	err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_SIGN_MOTION_EN_ADDR,
					ST_LSM6DS3_SIGN_MOTION_EN_MASK,
					value);
	if (err < 0)
		return err;

	err = st_lsm6ds3_set_extra_dependency(cdata, enable);
	if (err < 0)
		return err;

	return 0;
}

void st_lsm6ds3_set_irq(struct lsm6ds3_data *cdata, bool enable)
{

	if (enable) {
		cdata->states++;
		SENSOR_INFO("enable state count:(%d)\n", cdata->states);
		if (cdata->states == 1) {
			SENSOR_INFO("enable irq come (%d)\n", cdata->irq);
			enable_irq_wake(cdata->irq);
			enable_irq(cdata->irq);
		}
	} else if (cdata->states != 0 && enable == 0) {
		cdata->states--;
		if (cdata->states == 0) {
			disable_irq_wake(cdata->irq);
			disable_irq_nosync(cdata->irq);
			SENSOR_INFO("disable irq come (%d)\n", cdata->irq);
		}
		SENSOR_INFO("disable state count:(%d)\n", cdata->states);
	}
}

static void st_lsm6ds3_sa_irq_work(struct work_struct *work)
{
	struct lsm6ds3_data *cdata;
	u8 val;

	cdata = container_of((struct delayed_work *)work,
				struct lsm6ds3_data, sa_irq_work);

	st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_SRC_ADDR, 1, &val);

	if (cdata->drdy_int_pin == 1) {
		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_MD1_ADDR, 1, &val);
		val &= ~0x20;
		st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_MD1_ADDR, 1, &val);
	} else {
		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_MD2_ADDR, 1, &val);
		val &= ~0x20;
		st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_MD2_ADDR, 1, &val);
	}
}

irqreturn_t st_lsm6ds3_threaded(int irq, void *private)
{
	struct lsm6ds3_data *cdata = private;

	queue_work(cdata->irq_wq, &cdata->data_work);

	return IRQ_HANDLED;
}

static void st_lsm6ds3_irq_management(struct work_struct *data_work)
{
	int err;
	struct lsm6ds3_data *cdata;
	u8 src_value_reg1 = 0x00, src_value_reg2 = 0x00, src_wake_up = 0x00;

	cdata = container_of((struct work_struct *)data_work,
						struct lsm6ds3_data, data_work);

	err = cdata->tf->read(&cdata->tb, cdata->dev,
		ST_LSM6DS3_SRC_ACCEL_GYRO_REG, 1, &src_value_reg1);
	if (err < 0)
		goto st_lsm6ds3_irq_enable_irq;

	err = cdata->tf->read(&cdata->tb, cdata->dev,
		ST_LSM6DS3_SRC_FUNC_REG, 1, &src_value_reg2);
	if (err < 0)
		goto st_lsm6ds3_irq_enable_irq;

	err = cdata->tf->read(&cdata->tb, cdata->dev,
		ST_LSM6DS3_SRC_WAKE_UP_REG, 1, &src_wake_up);
	if (err < 0)
		goto st_lsm6ds3_irq_enable_irq;

	if ((src_wake_up & ST_LSM6DS3_SRC_WU_AVL) || (cdata->sa_factory_flag)) {
		SENSOR_INFO("########### smart alert irq ############\n");
		cdata->sa_irq_state = 1;
		wake_lock_timeout(&cdata->sa_wake_lock, msecs_to_jiffies(3000));
		schedule_delayed_work(&cdata->sa_irq_work,
					msecs_to_jiffies(100));
	}

	if ((src_value_reg2 & ST_LSM6DS3_SRC_SIGN_MOTION_DATA_AVL)
		|| (src_value_reg2 & ST_LSM6DS3_SRC_STEP_COUNTER_DATA_AVL)) {
		SENSOR_INFO("########## SMD (0x%x)#########\n", src_value_reg2);
		input_report_rel(cdata->smd_input, REL_MISC, 1);
		input_sync(cdata->smd_input);
		wake_lock_timeout(&cdata->sa_wake_lock, msecs_to_jiffies(3000));

		cdata->sensors_enabled &= ~(1 << ST_INDIO_DEV_SIGN_MOTION);
		st_lsm6ds3_enable_step_c(cdata, false);
	}

	if (src_value_reg2 & ST_LSM6DS3_SRC_TILT_DATA_AVL) {
		SENSOR_INFO("########### TILT ############\n");
		input_report_rel(cdata->tilt_input, REL_MISC, 1);
		input_sync(cdata->tilt_input);
		wake_lock_timeout(&cdata->sa_wake_lock,
					msecs_to_jiffies(3000));
	}

st_lsm6ds3_irq_enable_irq:
	return;
}
static ssize_t st_lsm6ds3_sysfs_get_smart_alert(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lsm6ds3_data *cdata = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", cdata->sa_irq_state);
}

static ssize_t st_lsm6ds3_sysfs_set_smart_alert(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct lsm6ds3_data *cdata = dev_get_drvdata(dev);

	u8 val, addr, odr, dur;
	unsigned int threshold = 0, fs;
	int enable = 0, factory_mode = 0, i;

	if (sysfs_streq(buf, "0")) {
		enable = 0;
		factory_mode = 0;
		if (cdata->sa_factory_flag)
			cdata->sa_factory_flag = 0;
		SENSOR_INFO("disable\n");
	} else if (sysfs_streq(buf, "1")) {
		enable = 1;
		factory_mode = 0;
		SENSOR_INFO("enable\n");
	} else if (sysfs_streq(buf, "2")) {
		enable = 1;
		factory_mode = 1;
		cdata->sa_factory_flag = 1;
		SENSOR_INFO("factory mode\n");
	} else {
		SENSOR_ERR("invalid value %s\n", buf);
		return size;
	}

	if ((enable == 1) && (cdata->sa_flag == 0)) {
		cdata->sa_irq_state = 0;
		cdata->sa_flag = 1;

		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_CTRL1_ADDR, 1, &odr);
		odr &= ~ST_LSM6DS3_ACCEL_ODR_MASK;
		st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_CTRL1_ADDR, 1, &odr);
		mdelay(100);

		if (factory_mode == 1) {
			threshold = 0;
			odr |= 0x50;
			dur = 0x00;
		} else {
			for (i = 0; i < ST_LSM6DS3_FS_LIST_NUM; i++) {
				if (st_lsm6ds3_fs_table[ST_INDIO_DEV_ACCEL].fs_avl[i].gain
					== cdata->sdata->acc_c_gain)
					break;
			}

			switch (i) {
			case 0:
				fs = 2000;
				break;
			case 1:
				fs = 4000;
				break;
			case 2:
				fs = 8000;
				break;
			case 3:
				fs = 16000;
				break;
			default:
				return size;
			}

			threshold = ST_LSM6DS3_SA_DYNAMIC_THRESHOLD * 64;
			threshold += fs / 2;
			threshold = (threshold / fs) & 0x3f;
			SENSOR_INFO("fs= %d, thr= %d[mg] = %d\n",
				fs, ST_LSM6DS3_SA_DYNAMIC_THRESHOLD, threshold);

			odr |= 0x20;
			dur = 0x40;
		}

		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_SRC_ADDR, 1, &val);

		if (cdata->drdy_int_pin == 1)
			addr = ST_LSM6DS3_MD1_ADDR;
		else
			addr = ST_LSM6DS3_MD2_ADDR;

		st_lsm6ds3_reg_read(cdata, addr, 1, &val);
		val |= 0x20;
		st_lsm6ds3_reg_write(cdata, addr, 1, &val);

		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_THS_ADDR, 1, &val);
		val &= ~0x3f;
		val |= threshold;
		st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_WU_THS_ADDR, 1, &val);

		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_DUR_ADDR, 1, &val);
		val &= ~0x60;
		val |= dur;
		st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_WU_DUR_ADDR, 1, &val);

		st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_CTRL1_ADDR, 1, &odr);
		mdelay(100);
		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_SRC_ADDR, 1, &val);
		SENSOR_INFO("WU_SRC= %x\n", val);
		st_lsm6ds3_set_irq(cdata, 1);
		SENSOR_INFO("smart alert is on!\n");
	} else if ((enable == 0) && (cdata->sa_flag == 1)) {
		st_lsm6ds3_set_irq(cdata, 0);
		cdata->sa_flag = 0;
		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_SRC_ADDR, 1, &val);

		if (cdata->drdy_int_pin == 1)
			addr = ST_LSM6DS3_MD1_ADDR;
		else
			addr = ST_LSM6DS3_MD2_ADDR;

		st_lsm6ds3_reg_read(cdata, addr, 1, &val);
		val &= ~0x20;
		st_lsm6ds3_reg_write(cdata, addr, 1, &val);

		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_THS_ADDR, 1, &val);
		val |= 0x3f;
		st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_WU_THS_ADDR, 1, &val);

		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_WU_SRC_ADDR, 1, &val);
		SENSOR_INFO("WU_SRC = %x\n", val);

		st_lsm6ds3_reg_read(cdata, ST_LSM6DS3_CTRL1_ADDR, 1, &val);
		val &= ~ST_LSM6DS3_ACCEL_ODR_MASK;
		if (cdata->sensors_enabled & (1 << ST_INDIO_DEV_ACCEL))
			val |= 0x50;
		else if (cdata->sensors_enabled & ST_LSM6DS3_EXTRA_DEPENDENCY)
			val |= 0x20;

		st_lsm6ds3_reg_write(cdata, ST_LSM6DS3_CTRL1_ADDR, 1, &val);

		SENSOR_INFO("smart alert is off! irq = %d, odr 0x%x\n",
						cdata->sa_irq_state, val >> 4);
	}

	return size;
}

static enum hrtimer_restart gyro_timer_func(struct hrtimer *timer)
{
	struct lsm6ds3_data *cdata = container_of(timer,
					struct lsm6ds3_data, gyro_timer);

	if (!work_pending(&cdata->gyro_work))
		queue_work(cdata->gyro_wq, &cdata->gyro_work);

	hrtimer_forward_now(&cdata->gyro_timer, cdata->gyro_delay);

	return HRTIMER_RESTART;
}

static enum hrtimer_restart acc_timer_func(struct hrtimer *timer)
{
	struct lsm6ds3_data *cdata = container_of(timer,
					struct lsm6ds3_data, acc_timer);

	if (!work_pending(&cdata->acc_work))
		queue_work(cdata->accel_wq, &cdata->acc_work);

	hrtimer_forward_now(&cdata->acc_timer, cdata->delay);

	return HRTIMER_RESTART;
}

static void st_lsm6ds3_acc_work_func(struct work_struct *work)
{
	struct lsm6ds3_data *cdata =
		container_of(work, struct lsm6ds3_data, acc_work);

	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi, time_lo;
	int n, len;
	u8 buf[6];
	s16 tmp_data[3], raw_data[3];

	len = cdata->tf->read(&cdata->tb, cdata->dev,
		ST_LSM6DS3_ACCEL_OUT_X_L_ADDR,
		ST_LSM6DS3_BYTE_FOR_CHANNEL * ST_LSM6DS3_NUMBER_DATA_CHANNELS,
		buf);
	if (len < 0)
		goto exit;

	/* Applying rotation matrix */
	for (n = 0; n < ST_LSM6DS3_NUMBER_DATA_CHANNELS; n++) {
		raw_data[n] = *((s16 *)&buf[2 * n]);
		if (raw_data[n] == -32768)
			raw_data[n] = -32767;
	}

	tmp_data[0] = SENSOR_X_DATA(raw_data[0], raw_data[1], raw_data[2],
		cdata->orientation[0], cdata->orientation[1],
		cdata->orientation[2], cdata->orientation[3],
		cdata->orientation[4], cdata->orientation[5],
		cdata->orientation[6], cdata->orientation[7],
		cdata->orientation[8]);
	tmp_data[1] = SENSOR_Y_DATA(raw_data[0], raw_data[1], raw_data[2],
		cdata->orientation[0], cdata->orientation[1],
		cdata->orientation[2], cdata->orientation[3],
		cdata->orientation[4], cdata->orientation[5],
		cdata->orientation[6], cdata->orientation[7],
		cdata->orientation[8]);
	tmp_data[2] = SENSOR_Z_DATA(raw_data[0], raw_data[1], raw_data[2],
		cdata->orientation[0], cdata->orientation[1],
		cdata->orientation[2], cdata->orientation[3],
		cdata->orientation[4], cdata->orientation[5],
		cdata->orientation[6], cdata->orientation[7],
		cdata->orientation[8]);

	cdata->accel_data[0] = tmp_data[0] - cdata->accel_cal_data[0];
	cdata->accel_data[1] = tmp_data[1] - cdata->accel_cal_data[1];
	cdata->accel_data[2] = tmp_data[2] - cdata->accel_cal_data[2];

	time_hi = (int)((timestamp_new & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(timestamp_new & TIME_LO_MASK);

	input_report_rel(cdata->acc_input, REL_X, cdata->accel_data[0]);
	input_report_rel(cdata->acc_input, REL_Y, cdata->accel_data[1]);
	input_report_rel(cdata->acc_input, REL_Z, cdata->accel_data[2]);
	input_report_rel(cdata->acc_input, REL_DIAL, time_hi);
	input_report_rel(cdata->acc_input, REL_MISC, time_lo);
	input_sync(cdata->acc_input);

exit:
	if ((ktime_to_ns(cdata->delay) * cdata->time_count)
			>= ((int64_t)ACCEL_LOG_TIME * NSEC_PER_SEC)) {
		SENSOR_INFO("x = %d, y = %d, z = %d\n",
			cdata->accel_data[0],
			cdata->accel_data[1], cdata->accel_data[2]);
		cdata->time_count = 0;
	} else {
		cdata->time_count++;
	}
}

static void st_lsm6ds3_gyro_work_func(struct work_struct *work)
{
	struct lsm6ds3_data *cdata =
		container_of(work, struct lsm6ds3_data, gyro_work);

	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi, time_lo;
	int n, len;
	u8 buf[6];
	s16 tmp_data[3], raw_data[3];

	len = cdata->tf->read(&cdata->tb, cdata->dev,
		ST_LSM6DS3_GYRO_OUT_X_L_ADDR,
		ST_LSM6DS3_BYTE_FOR_CHANNEL * ST_LSM6DS3_NUMBER_DATA_CHANNELS,
		buf);
	if (len < 0)
		goto exit;

	/* Applying rotation matrix */
	for (n = 0; n < ST_LSM6DS3_NUMBER_DATA_CHANNELS; n++) {
		raw_data[n] = *((s16 *)&buf[2 * n]);
		if (raw_data[n] == -32768)
			raw_data[n] = -32767;
	}

	tmp_data[0] = SENSOR_X_DATA(raw_data[0], raw_data[1], raw_data[2],
		cdata->orientation[0], cdata->orientation[1],
		cdata->orientation[2], cdata->orientation[3],
		cdata->orientation[4], cdata->orientation[5],
		cdata->orientation[6], cdata->orientation[7],
		cdata->orientation[8]);
	tmp_data[1] = SENSOR_Y_DATA(raw_data[0], raw_data[1], raw_data[2],
		cdata->orientation[0], cdata->orientation[1],
		cdata->orientation[2], cdata->orientation[3],
		cdata->orientation[4], cdata->orientation[5],
		cdata->orientation[6], cdata->orientation[7],
		cdata->orientation[8]);
	tmp_data[2] = SENSOR_Z_DATA(raw_data[0], raw_data[1], raw_data[2],
		cdata->orientation[0], cdata->orientation[1],
		cdata->orientation[2], cdata->orientation[3],
		cdata->orientation[4], cdata->orientation[5],
		cdata->orientation[6], cdata->orientation[7],
		cdata->orientation[8]);

	cdata->gyro_data[0] = tmp_data[0];
	cdata->gyro_data[1] = tmp_data[1];
	cdata->gyro_data[2] = tmp_data[2];

	time_hi = (int)((timestamp_new & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(timestamp_new & TIME_LO_MASK);

	input_report_rel(cdata->gyro_input, REL_RX, cdata->gyro_data[0]);
	input_report_rel(cdata->gyro_input, REL_RY, cdata->gyro_data[1]);
	input_report_rel(cdata->gyro_input, REL_RZ, cdata->gyro_data[2]);
	input_report_rel(cdata->gyro_input, REL_X, time_hi);
	input_report_rel(cdata->gyro_input, REL_Y, time_lo);
	input_sync(cdata->gyro_input);

exit:
	if ((ktime_to_ns(cdata->gyro_delay) * cdata->gyro_time_count)
			>= ((int64_t)ACCEL_LOG_TIME * NSEC_PER_SEC)) {
		SENSOR_INFO("x = %d, y = %d, z = %d\n",
			cdata->gyro_data[0],
			cdata->gyro_data[1], cdata->gyro_data[2]);
		cdata->gyro_time_count = 0;
	} else {
		cdata->gyro_time_count++;
	}
}

static ssize_t st_lsm6ds3_acc_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 16, "%lld\n", ktime_to_ns(cdata->delay));
}

static ssize_t st_lsm6ds3_acc_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);
	int err;
	unsigned long data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return count;

	if (data == 0)
		return count;

	if (data > ST_LSM6DS3_DELAY_DEFAULT)
		data = ST_LSM6DS3_DELAY_DEFAULT;

	cdata->delay = ns_to_ktime(data);

	SENSOR_INFO("new_delay = %lld\n", ktime_to_ns(cdata->delay));

	return count;
}

static ssize_t st_lsm6ds3_acc_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 16, "%d\n", atomic_read(&cdata->wkqueue_en));

}

static ssize_t st_lsm6ds3_acc_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;
	int pre_enable = atomic_read(&cdata->wkqueue_en);

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d, pre_enable = %d\n", enable, pre_enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		if (pre_enable == 0) {
			st_lsm6ds3_acc_open_calibration(cdata);
			st_lsm6ds3_set_fs(cdata,
				cdata->sdata->acc_c_gain, ST_INDIO_DEV_ACCEL);
			st_lsm6ds3_acc_set_enable(cdata, true);
			atomic_set(&cdata->wkqueue_en, 1);
		}
	} else {
		if (pre_enable == 1) {
			atomic_set(&cdata->wkqueue_en, 0);
			st_lsm6ds3_acc_set_enable(cdata, false);
		}
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static ssize_t st_lsm6ds3_gyro_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 16, "%lld\n", ktime_to_ns(cdata->gyro_delay));
}

static ssize_t st_lsm6ds3_gyro_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);
	int err;
	unsigned long data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;

	if (data == 0)
		return count;

	if (data > ST_LSM6DS3_DELAY_DEFAULT)
		data = ST_LSM6DS3_DELAY_DEFAULT;

	cdata->gyro_delay = ns_to_ktime(data);

	SENSOR_INFO("new_delay = %lld\n", ktime_to_ns(cdata->gyro_delay));

	return count;
}

static ssize_t st_lsm6ds3_gyro_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);

	return sprintf(buf, "%d\n", atomic_read(&cdata->gyro_wkqueue_en));
}

static ssize_t st_lsm6ds3_gyro_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;
	int pre_enable = atomic_read(&cdata->gyro_wkqueue_en);

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d, pre_enable = %d\n", enable, pre_enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		if (pre_enable == 0) {
			st_lsm6ds3_set_fs(cdata,
				cdata->sdata->gyr_c_gain, ST_INDIO_DEV_GYRO);
			st_lsm6ds3_gyro_set_enable(cdata, true);
			atomic_set(&cdata->gyro_wkqueue_en, 1);
		}
	} else {
		if (pre_enable == 1) {
			atomic_set(&cdata->gyro_wkqueue_en, 0);
			st_lsm6ds3_gyro_set_enable(cdata, false);
		}
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static ssize_t st_lsm6ds3_smd_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 16, "%d\n", atomic_read(&cdata->wkqueue_en));
}

static ssize_t st_lsm6ds3_smd_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n",	enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		err = st_lsm6ds3_enable_step_c(cdata, true);
		if (err < 0) {
			mutex_unlock(&cdata->mutex_enable);
			return count;
		}

		st_lsm6ds3_set_irq(cdata, 1);
		cdata->sensors_enabled |= (1 << ST_INDIO_DEV_SIGN_MOTION);
	} else {
		cdata->sensors_enabled &= ~(1 << ST_INDIO_DEV_SIGN_MOTION);
		st_lsm6ds3_set_irq(cdata, 0);

		err = st_lsm6ds3_enable_step_c(cdata, false);
		if (err < 0) {
			mutex_unlock(&cdata->mutex_enable);
			return count;
		}
	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static ssize_t st_lsm6ds3_tilt_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);

	return snprintf(buf, 16, "%d\n", atomic_read(&cdata->wkqueue_en));

}

static ssize_t st_lsm6ds3_tilt_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct lsm6ds3_data *cdata = input_get_drvdata(input);
	int err;
	u8 enable;

	err = kstrtou8(buf, 10, &enable);
	if (err)
		return count;

	enable = enable ? 1 : 0;

	SENSOR_INFO("new_value = %d\n",	enable);

	mutex_lock(&cdata->mutex_enable);
	if (enable) {
		err = st_lsm6ds3_set_extra_dependency(cdata, true);
		if (err < 0) {
			mutex_unlock(&cdata->mutex_enable);
			return count;
		}

		err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_MD1_ADDR,
					ST_LSM6DS3_TILT_DRDY_IRQ_MASK,
					ST_LSM6DS3_EN_BIT);
		if (err < 0) {
			mutex_unlock(&cdata->mutex_enable);
			return count;
		}

		err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_TILT_EN_ADDR,
					ST_LSM6DS3_TILT_EN_MASK,
					ST_LSM6DS3_EN_BIT);
		if (err < 0) {
			mutex_unlock(&cdata->mutex_enable);
			return count;
		}

		st_lsm6ds3_set_irq(cdata, 1);
		cdata->sensors_enabled |= (1 << ST_INDIO_DEV_TILT);
	} else {
		cdata->sensors_enabled &= ~(1 << ST_INDIO_DEV_TILT);
		st_lsm6ds3_set_irq(cdata, 0);
		err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_TILT_EN_ADDR,
					ST_LSM6DS3_TILT_EN_MASK,
					ST_LSM6DS3_DIS_BIT);
		if (err < 0) {
			mutex_unlock(&cdata->mutex_enable);
			return count;
		}

		err = st_lsm6ds3_write_data_with_mask(cdata,
					ST_LSM6DS3_MD1_ADDR,
					ST_LSM6DS3_TILT_DRDY_IRQ_MASK,
					ST_LSM6DS3_DIS_BIT);
		if (err < 0) {
			mutex_unlock(&cdata->mutex_enable);
			return count;
		}

		err = st_lsm6ds3_set_extra_dependency(cdata, false);
		if (err < 0) {
			mutex_unlock(&cdata->mutex_enable);
			return count;
		}

	}
	mutex_unlock(&cdata->mutex_enable);

	return count;
}

static struct device_attribute dev_attr_acc_poll_delay =
	__ATTR(poll_delay, S_IRUGO|S_IWUSR,
	st_lsm6ds3_acc_delay_show,
	st_lsm6ds3_acc_delay_store);
static struct device_attribute dev_attr_acc_enable =
	__ATTR(enable, S_IRUGO|S_IWUSR,
	st_lsm6ds3_acc_enable_show,
	st_lsm6ds3_acc_enable_store);

static struct attribute *st_lsm6ds3_acc_attributes[] = {
	&dev_attr_acc_poll_delay.attr,
	&dev_attr_acc_enable.attr,
	NULL
};

static struct attribute_group st_lsm6ds3_accel_attribute_group = {
	.attrs = st_lsm6ds3_acc_attributes
};

static struct device_attribute dev_attr_gyro_poll_delay =
	__ATTR(poll_delay, S_IRUGO|S_IWUSR,
	st_lsm6ds3_gyro_delay_show,
	st_lsm6ds3_gyro_delay_store);
static struct device_attribute dev_attr_gyro_enable =
	__ATTR(enable, S_IRUGO|S_IWUSR,
	st_lsm6ds3_gyro_enable_show,
	st_lsm6ds3_gyro_enable_store);

static struct attribute *st_lsm6ds3_gyro_attributes[] = {
	&dev_attr_gyro_poll_delay.attr,
	&dev_attr_gyro_enable.attr,
	NULL
};

static struct attribute_group st_lsm6ds3_gyro_attribute_group = {
	.attrs = st_lsm6ds3_gyro_attributes
};


static struct device_attribute dev_attr_smd_enable =
	__ATTR(enable, S_IRUGO|S_IWUSR,
	st_lsm6ds3_smd_enable_show,
	st_lsm6ds3_smd_enable_store);

static struct attribute *st_lsm6ds3_smd_attributes[] = {
	&dev_attr_smd_enable.attr,
	NULL
};

static struct attribute_group st_lsm6ds3_smd_attribute_group = {
	.attrs = st_lsm6ds3_smd_attributes
};

static struct device_attribute dev_attr_tilt_enable =
	__ATTR(enable, S_IRUGO|S_IWUSR,
	st_lsm6ds3_tilt_enable_show,
	st_lsm6ds3_tilt_enable_store);

static struct attribute *st_lsm6ds3_tilt_attributes[] = {
	&dev_attr_tilt_enable.attr,
	NULL
};

static struct attribute_group st_lsm6ds3_tilt_attribute_group = {
	.attrs = st_lsm6ds3_tilt_attributes
};

static ssize_t st_acc_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	signed short cx, cy, cz;
	struct lsm6ds3_data *cdata;

	cdata = dev_get_drvdata(dev);

	cx = cdata->accel_data[0];
	cy = cdata->accel_data[1];
	cz = cdata->accel_data[2];

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d\n", cx, cy, cz);
}

static ssize_t st_gyro_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	signed short cx, cy, cz;
	struct lsm6ds3_data *cdata;

	cdata = dev_get_drvdata(dev);

	cx = cdata->gyro_data[0];
	cy = cdata->gyro_data[1];
	cz = cdata->gyro_data[2];

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d\n", cx, cy, cz);
}

static int st_acc_do_calibrate(struct lsm6ds3_data *cdata, int enable)
{
	int sum[3] = { 0, };
	int ret = 0, cnt;
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;

	SENSOR_INFO("\n");

	if (enable) {
		cdata->accel_cal_data[0] = 0;
		cdata->accel_cal_data[1] = 0;
		cdata->accel_cal_data[2] = 0;

		for (cnt = 0; cnt < CALIBRATION_DATA_AMOUNT; cnt++) {
			sum[0] += cdata->accel_data[0];
			sum[1] += cdata->accel_data[1];
			sum[2] += cdata->accel_data[2];
			msleep(20);
		}

		cdata->accel_cal_data[0] = (sum[0] / CALIBRATION_DATA_AMOUNT);
		cdata->accel_cal_data[1] = (sum[1] / CALIBRATION_DATA_AMOUNT);
		cdata->accel_cal_data[2] = (sum[2] / CALIBRATION_DATA_AMOUNT);

		if (cdata->accel_cal_data[2] > 0)
			cdata->accel_cal_data[2] -= MAX_ACCEL_1G;
		else if (cdata->accel_cal_data[2] < 0)
			cdata->accel_cal_data[2] += MAX_ACCEL_1G;

	} else {
		cdata->accel_cal_data[0] = 0;
		cdata->accel_cal_data[1] = 0;
		cdata->accel_cal_data[2] = 0;
	}

	SENSOR_INFO("do accel calibrate %d, %d, %d\n", cdata->accel_cal_data[0],
		cdata->accel_cal_data[1], cdata->accel_cal_data[2]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
		O_CREAT | O_TRUNC | O_WRONLY, 0660);
	if (IS_ERR(cal_filp)) {
		SENSOR_ERR("Can't open calibration file\n");
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		return ret;
	}

	ret = cal_filp->f_op->write(cal_filp, (char *)&cdata->accel_cal_data,
		3 * sizeof(s16), &cal_filp->f_pos);
	if (ret != 3 * sizeof(s16)) {
		SENSOR_ERR("Can't write the caldata to file\n");
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static ssize_t st_lsm6ds3_acc_calibration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	struct lsm6ds3_data *cdata = dev_get_drvdata(dev);

	SENSOR_INFO("\n");

	ret = st_lsm6ds3_acc_open_calibration(cdata);
	if (ret < 0)
		SENSOR_ERR("calibration open failed = %d\n", ret);

	SENSOR_INFO("cal data %d %d %d, ret = %d\n", cdata->accel_cal_data[0],
		cdata->accel_cal_data[1], cdata->accel_cal_data[2], ret);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n", ret,
		cdata->accel_cal_data[0], cdata->accel_cal_data[1],
		cdata->accel_cal_data[2]);
}

static ssize_t st_lsm6ds3_acc_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct lsm6ds3_data *cdata = dev_get_drvdata(dev);

	SENSOR_INFO("\n");

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0) {
		SENSOR_ERR("kstrtoll failed\n");
		return size;
	}

	ret = st_acc_do_calibrate(cdata, (int)dEnable);
	if (ret < 0)
		SENSOR_ERR("accel calibrate failed\n");

	return size;
}

#ifdef ST_LSM6DS3_REG_DUMP_TEST
static ssize_t st_lsm6ds3_reg_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct lsm6ds3_data *cdata = dev_get_drvdata(dev);

	st_lsm6ds3_reg_dump(cdata);

	return snprintf(buf, PAGE_SIZE, "reg_dump.\n");
}
#endif

static ssize_t st_lsm6ds3_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t st_lsm6ds3_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", LSM6DS3_DEV_NAME);
}

static int st_lsm6ds3_set_lpf(struct lsm6ds3_data *cdata, int onoff)
{
	int ret;
	int i;
	u8 buf, bw, odr;
	u8 lpf_odr = 0;

	SENSOR_INFO("cdata->odr = %x, onoff = %d\n",
					cdata->sdata->acc_c_odr, onoff);

	for (i = 0; i < ST_LSM6DS3_ODR_LIST_NUM; i++) {
		if (cdata->sdata->acc_c_odr ==
			st_lsm6ds3_odr_table.odr_avl[i].hz) {
			SENSOR_INFO("odr_avl channel is %d\n", i);
			lpf_odr = st_lsm6ds3_odr_table.odr_avl[i].value;
			break;
		}
	}

	if (onoff) {
		odr = lpf_odr;
		bw = 0x00;
	} else {
		odr = ST_LSM6DS3_ODR_6660HZ_VAL;
		bw = 0x00;
	}

	/*
	 * K6DS3 CTRL1_XL(0X10) register configuration
	 * [7]ODR_XL3	[6]ODR_XL2	[5]ODR_XL1	[4]ODR_XL0
	 * [3]FS_XL1	[2]FS_XL0	[1]BW_XL1	[0]BW_XL0
	 */

	ret = cdata->tf->read(&cdata->tb, cdata->dev,
					ST_LSM6DS3_ACCEL_ODR_ADDR, 1, &buf);
	buf = (ST_LSM6DS3_ACCEL_ODR_MASK & (odr << 4)) |
		((~ST_LSM6DS3_ACCEL_ODR_MASK) & buf);
	ret += cdata->tf->write(&cdata->tb, cdata->dev,
					ST_LSM6DS3_ACCEL_ODR_ADDR, 1, &buf);

	ret = cdata->tf->read(&cdata->tb, cdata->dev,
					ST_LSM6DS3_ACCEL_BW_ADDR, 1, &buf);
	buf = (ST_LSM6DS3_ACCEL_BW_MASK & bw) |
		((~ST_LSM6DS3_ACCEL_BW_MASK) & buf);
	ret += cdata->tf->write(&cdata->tb, cdata->dev,
					ST_LSM6DS3_ACCEL_BW_ADDR, 1, &buf);

	cdata->lpf_on = onoff;

	return ret;
}

static ssize_t st_lsm6ds3_lowpassfilter_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct lsm6ds3_data *cdata = dev_get_drvdata(dev);

	if (cdata->lpf_on)
		ret = 1;
	else
		ret = 0;

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t st_lsm6ds3_lowpassfilter_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct lsm6ds3_data *cdata;

	cdata = dev_get_drvdata(dev);
	SENSOR_INFO("\n");

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0)
		SENSOR_ERR("kstrtoll failed, ret = %d\n", ret);

	ret = st_lsm6ds3_set_lpf(cdata , dEnable);
	if (ret < 0)
		SENSOR_ERR("st_lsm6ds3_set_lpf failed, ret = %d\n", ret);

	return size;
}

static ssize_t st_lsm6ds3_vendor_temperature(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	s16 temp;
	u8 data[2];

	struct lsm6ds3_data *cdata = dev_get_drvdata(dev);

	ret = cdata->tf->read(&cdata->tb, cdata->dev,
				ST_LSM6DS3_OUT_TEMP_L_ADDR, 2, data);
	if (ret < 0)
		SENSOR_ERR("st_lsm6ds3_i2c_read_dev failed ret = %d\n", ret);

	temp = (s16)(data[0] | (data[1] << 8)) / 16 + 25;

	return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

#ifdef ST_LSM6DS3_REG_DUMP_TEST
static DEVICE_ATTR(reg_dump, S_IRUGO, st_lsm6ds3_reg_dump_show, NULL);
#endif
static DEVICE_ATTR(temperature, S_IRUGO, st_lsm6ds3_vendor_temperature, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, st_lsm6ds3_vendor_show, NULL);
static DEVICE_ATTR(name, S_IRUGO, st_lsm6ds3_name_show, NULL);
static DEVICE_ATTR(lowpassfilter, S_IRUGO | S_IWUSR | S_IWGRP,
	st_lsm6ds3_lowpassfilter_show, st_lsm6ds3_lowpassfilter_store);

static struct device_attribute dev_attr_acc_raw_data =
	__ATTR(raw_data, S_IRUGO, st_acc_raw_data_show, NULL);

static struct device_attribute dev_attr_acc_self_test =
	__ATTR(selftest, S_IRUGO,
	st_lsm6ds3_acc_sysfs_get_selftest_run,
	NULL);

static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
	st_lsm6ds3_acc_calibration_show, st_lsm6ds3_acc_calibration_store);

static DEVICE_ATTR(reactive_alert, S_IWUSR | S_IRUGO,
				st_lsm6ds3_sysfs_get_smart_alert,
				st_lsm6ds3_sysfs_set_smart_alert);

static struct device_attribute *acc_sensor_attrs[] = {
	&dev_attr_acc_raw_data,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_acc_self_test,
	&dev_attr_calibration,
	&dev_attr_lowpassfilter,
	&dev_attr_reactive_alert,
	NULL
};

static struct device_attribute dev_attr_gyro_raw_data =
	__ATTR(raw_data, S_IRUGO, st_gyro_raw_data_show, NULL);
static struct device_attribute dev_attr_gyro_self_test =
	__ATTR(selftest, S_IRUGO,
	st_lsm6ds3_gyro_sysfs_get_selftest_run,
	NULL);

static struct device_attribute *gyro_sensor_attrs[] = {
	&dev_attr_gyro_raw_data,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_gyro_self_test,
	&dev_attr_temperature,
	NULL
};

static struct device_attribute *smd_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	NULL
};

static struct device_attribute *tilt_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
#ifdef ST_LSM6DS3_REG_DUMP_TEST
	&dev_attr_reg_dump,
#endif
	NULL
};

static int st_lsm6ds3_acc_input_init(struct lsm6ds3_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_ACC;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_capability(dev, EV_REL, REL_Z);
	input_set_capability(dev, EV_REL, REL_DIAL);
	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_drvdata(dev, cdata);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		goto err_register_input_dev;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj,
				&st_lsm6ds3_accel_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->acc_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static int st_lsm6ds3_gyro_input_init(struct lsm6ds3_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_GYRO;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_RX);
	input_set_capability(dev, EV_REL, REL_RY);
	input_set_capability(dev, EV_REL, REL_RZ);
	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_drvdata(dev, cdata);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		goto err_register_input_dev;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj,
				&st_lsm6ds3_gyro_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->gyro_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static int st_lsm6ds3_smd_input_init(struct lsm6ds3_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_SMD;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_drvdata(dev, cdata);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		goto err_register_input_dev;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj,
				&st_lsm6ds3_smd_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->smd_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static int st_lsm6ds3_tilt_input_init(struct lsm6ds3_data *cdata)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME_TILT;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_drvdata(dev, cdata);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		goto err_register_input_dev;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0)
		goto err_create_sensor_symlink;

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj,
				&st_lsm6ds3_tilt_attribute_group);
	if (ret < 0)
		goto err_create_sysfs_group;

	cdata->tilt_input = dev;

	return 0;

err_create_sysfs_group:
	sensors_remove_symlink(&dev->dev.kobj, dev->name);
err_create_sensor_symlink:
	input_unregister_device(dev);
err_register_input_dev:
	return ret;
}

static int st_lsm6ds3_parse_dt(struct lsm6ds3_data *cdata)
{
	u32 val;
	struct device_node *np;
	enum of_gpio_flags flags;
	u32 orientation[9], i = 0;

	np = cdata->dev->of_node;
	if (!np)
		return -EINVAL;

	if (!of_property_read_u32(np, "st,drdy-int-pin", &val) &&
						(val <= 2) && (val > 0)) {
		cdata->drdy_int_pin = (u8)val;
	} else {
		cdata->drdy_int_pin = 1;
	}

	cdata->irq_gpio = of_get_named_gpio_flags(np, "st,irq_gpio", 0, &flags);
	if (cdata->irq_gpio < 0) {
		SENSOR_ERR("get irq_gpio = %d error\n", cdata->irq_gpio);
		return -ENODEV;
	}
	cdata->irq = gpio_to_irq(cdata->irq_gpio);

	if (of_property_read_u32_array(np, "st,orientation",
						orientation, 9) < 0) {
		SENSOR_ERR("get orientation %d error\n", orientation[0]);
		return -ENODEV;
	}

	for (i = 0 ; i < 9 ; i++)
		cdata->orientation[i] = ((s8)orientation[i]) - 1;

	return 0;
}

int st_lsm6ds3_common_probe(struct lsm6ds3_data *cdata, int irq)
{
	int err = 0;
	u8 wai = 0x00;
	struct lsm6ds3_sensor_data *sdata;

	dev_set_drvdata(cdata->dev, cdata);

	cdata->err_count = 0;
	cdata->states = 0;
	cdata->sensors_enabled = 0;

	mutex_init(&cdata->tb.buf_lock);
	mutex_init(&cdata->mutex_enable);

	err = cdata->tf->read(&cdata->tb, cdata->dev,
					ST_LSM6DS3_WAI_ADDRESS, 1, &wai);
	if (err < 0) {
		SENSOR_ERR("failed to read Who-Am-I register\n");
		return err;
	}
	if (wai != ST_LSM6DS3_WAI_EXP) {
		SENSOR_ERR("Who-Am-I value not valid\n");
		return -ENODEV;
	}

	sdata = kzalloc(sizeof(struct lsm6ds3_sensor_data), GFP_KERNEL);
	if (!sdata) {
		SENSOR_ERR("kzalloc error\n");
		err = -ENOMEM;
		goto exit;
	}

	/* input device init */
	err = st_lsm6ds3_acc_input_init(cdata);
	if (err < 0)
		goto exit_acc_input_init;

	err = st_lsm6ds3_gyro_input_init(cdata);
	if (err < 0)
		goto exit_gyro_input_init;

	err = st_lsm6ds3_smd_input_init(cdata);
	if (err < 0)
		goto exit_smd_input_init;

	err = st_lsm6ds3_tilt_input_init(cdata);
	if (err < 0)
		goto exit_tilt_input_init;

	cdata->sdata = sdata;
	cdata->sdata->st_mode = 3;

	cdata->sdata->acc_c_odr =
		st_lsm6ds3_odr_table.odr_avl[ST_LSM6DS3_ODR_104HZ].hz;
	cdata->sdata->acc_c_gain =
		st_lsm6ds3_fs_table[ST_INDIO_DEV_ACCEL].fs_avl[ST_LSM6DS3_ACCEL_FS_4G].gain;

	cdata->sdata->gyr_c_odr =
		st_lsm6ds3_odr_table.odr_avl[ST_LSM6DS3_ODR_104HZ].hz;
	cdata->sdata->gyr_c_gain =
		st_lsm6ds3_fs_table[ST_INDIO_DEV_GYRO].fs_avl[ST_LSM6DS3_GYRO_FS_1000].gain;

	if (irq > 0) {
		err = st_lsm6ds3_parse_dt(cdata);
		if (err < 0)
			goto exit_of_node;
	}

	err = st_lsm6ds3_init_sensor(cdata);
	if (err < 0)
		goto exit_init_sensor;

	/* factory test sysfs node */
	err = sensors_register(&cdata->acc_factory_dev, cdata,
		acc_sensor_attrs, MODULE_NAME_ACC);
	if (err < 0) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_acc_sensor_register_failed;
	}

	err = sensors_register(&cdata->gyro_factory_dev, cdata,
		gyro_sensor_attrs, MODULE_NAME_GYRO);
	if (err < 0) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_gyro_sensor_register_failed;
	}

	err = sensors_register(&cdata->smd_factory_dev, cdata,
		smd_sensor_attrs, MODULE_NAME_SMD);
	if (err) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_smd_sensor_register_failed;
	}

	err = sensors_register(&cdata->tilt_factory_dev, cdata,
		tilt_sensor_attrs, MODULE_NAME_TILT);
	if (err) {
		SENSOR_ERR("failed to sensors_register = %d\n", err);
		goto exit_tilt_sensor_register_failed;
	}

	/* workqueue init */
	hrtimer_init(&cdata->acc_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cdata->delay = ns_to_ktime(ST_LSM6DS3_DELAY_DEFAULT);
	cdata->acc_timer.function = acc_timer_func;

	hrtimer_init(&cdata->gyro_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cdata->gyro_delay = ns_to_ktime(ST_LSM6DS3_DELAY_DEFAULT);
	cdata->gyro_timer.function = gyro_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	cdata->accel_wq = create_singlethread_workqueue("accel_wq");
	if (!cdata->accel_wq) {
		err = -ENOMEM;
		SENSOR_ERR("could not create accel workqueue\n");
		goto exit_create_workqueue_acc;
	}

	cdata->gyro_wq = create_singlethread_workqueue("gyro_wq");
	if (!cdata->gyro_wq) {
		err = -ENOMEM;
		SENSOR_ERR("could not create gyro workqueue\n");
		goto exit_create_workqueue_gyro;
	}

	cdata->time_count = 0;
	cdata->gyro_time_count = 0;

	/* this is the thread function we run on the work queue */
	INIT_WORK(&cdata->acc_work, st_lsm6ds3_acc_work_func);
	INIT_WORK(&cdata->gyro_work, st_lsm6ds3_gyro_work_func);

	atomic_set(&cdata->wkqueue_en, 0);
	atomic_set(&cdata->gyro_wkqueue_en, 0);

	cdata->sa_irq_state = 0;
	cdata->sa_flag = 0;

	cdata->irq_wq = create_workqueue(cdata->name);
	if (!cdata->irq_wq) {
		err = -ENOMEM;
		SENSOR_ERR("could not create irq workqueue\n");
		goto exit_create_workqueue_irq;
	}

	if (cdata->irq > 0) {
		wake_lock_init(&cdata->sa_wake_lock, WAKE_LOCK_SUSPEND,
		       LSM6DS3_DEV_NAME "_sa_wake_lock");

		INIT_WORK(&cdata->data_work, st_lsm6ds3_irq_management);
		INIT_DELAYED_WORK(&cdata->sa_irq_work, st_lsm6ds3_sa_irq_work);

		err = request_threaded_irq(irq, st_lsm6ds3_threaded, NULL,
				IRQF_TRIGGER_RISING, cdata->name, cdata);
		disable_irq(cdata->irq);

		SENSOR_INFO("Smart alert init, irq = %d\n", cdata->irq);
	}

	return 0;

exit_create_workqueue_irq:
	destroy_workqueue(cdata->gyro_wq);
exit_create_workqueue_gyro:
	destroy_workqueue(cdata->accel_wq);
exit_create_workqueue_acc:
	sensors_unregister(cdata->tilt_factory_dev, tilt_sensor_attrs);
exit_tilt_sensor_register_failed:
	sensors_unregister(cdata->smd_factory_dev, smd_sensor_attrs);
exit_smd_sensor_register_failed:
	sensors_unregister(cdata->gyro_factory_dev, gyro_sensor_attrs);
exit_gyro_sensor_register_failed:
	sensors_unregister(cdata->acc_factory_dev, acc_sensor_attrs);
exit_acc_sensor_register_failed:
exit_init_sensor:
exit_of_node:
	sensors_remove_symlink(&cdata->tilt_input->dev.kobj,
					cdata->tilt_input->name);
	sysfs_remove_group(&cdata->tilt_input->dev.kobj,
					&st_lsm6ds3_tilt_attribute_group);
	input_unregister_device(cdata->tilt_input);
exit_tilt_input_init:
	sensors_remove_symlink(&cdata->smd_input->dev.kobj,
					cdata->smd_input->name);
	sysfs_remove_group(&cdata->smd_input->dev.kobj,
					&st_lsm6ds3_smd_attribute_group);
	input_unregister_device(cdata->smd_input);
exit_smd_input_init:
	sensors_remove_symlink(&cdata->gyro_input->dev.kobj,
					cdata->gyro_input->name);
	sysfs_remove_group(&cdata->gyro_input->dev.kobj,
					&st_lsm6ds3_gyro_attribute_group);
	input_unregister_device(cdata->gyro_input);
exit_gyro_input_init:
	sensors_remove_symlink(&cdata->acc_input->dev.kobj,
					cdata->acc_input->name);
	sysfs_remove_group(&cdata->acc_input->dev.kobj,
					&st_lsm6ds3_accel_attribute_group);
	input_unregister_device(cdata->acc_input);
exit_acc_input_init:
	kfree(sdata);
exit:
	return err;
}
EXPORT_SYMBOL(st_lsm6ds3_common_probe);

void st_lsm6ds3_common_remove(struct lsm6ds3_data *cdata, int irq)
{
}
EXPORT_SYMBOL(st_lsm6ds3_common_remove);

void st_lsm6ds3_common_shutdown(struct lsm6ds3_data *cdata)
{
	SENSOR_INFO("\n");
	if (atomic_read(&cdata->wkqueue_en) == 1)
		st_lsm6ds3_acc_set_enable(cdata, false);
	if (atomic_read(&cdata->gyro_wkqueue_en) == 1)
		st_lsm6ds3_gyro_set_enable(cdata, false);
}
EXPORT_SYMBOL(st_lsm6ds3_common_shutdown);

int st_lsm6ds3_common_suspend(struct lsm6ds3_data *cdata)
{
	if (atomic_read(&cdata->wkqueue_en) == 1)
		st_lsm6ds3_acc_set_enable(cdata, false);
	if (atomic_read(&cdata->gyro_wkqueue_en) == 1)
		st_lsm6ds3_gyro_set_enable(cdata, false);

	return 0;
}
EXPORT_SYMBOL(st_lsm6ds3_common_suspend);

int st_lsm6ds3_common_resume(struct lsm6ds3_data *cdata)
{
	if (atomic_read(&cdata->wkqueue_en) == 1) {
		st_lsm6ds3_set_fs(cdata, cdata->sdata->acc_c_gain,
					ST_INDIO_DEV_ACCEL);
		st_lsm6ds3_acc_set_enable(cdata, true);
	}
	if (atomic_read(&cdata->gyro_wkqueue_en) == 1) {
		st_lsm6ds3_set_fs(cdata, cdata->sdata->gyr_c_gain,
					ST_INDIO_DEV_GYRO);
		st_lsm6ds3_gyro_set_enable(cdata, true);
	}

	return 0;
}
EXPORT_SYMBOL(st_lsm6ds3_common_resume);

MODULE_DESCRIPTION("lsm6ds3 accelerometer sensor driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL v2");
