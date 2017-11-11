/*
 * exynos_tmu.c - Samsung EXYNOS TMU (Thermal Management Unit)
 *
 *  Copyright (C) 2014 Samsung Electronics
 *  Bartlomiej Zolnierkiewicz <b.zolnierkie@samsung.com>
 *  Lukasz Majewski <l.majewski@samsung.com>
 *
 *  Copyright (C) 2011 Samsung Electronics
 *  Donggeun Kim <dg77.kim@samsung.com>
 *  Amit Daniel Kachhap <amit.kachhap@linaro.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/suspend.h>
#include <linux/pm_qos.h>
#include <linux/threads.h>
#include <linux/thermal.h>
#include <linux/gpu_cooling.h>
#include <linux/isp_cooling.h>
#include <linux/slab.h>
#include <linux/exynos-ss.h>
#include <linux/cpu.h>
#include <linux/exynos-cpufreq.h>
#include <soc/samsung/tmu.h>
#include <soc/samsung/ect_parser.h>
#include <soc/samsung/exynos-mcinfo.h>
#include <dt-bindings/thermal/thermal_exynos.h>

#include "exynos_tmu.h"
#include "../thermal_core.h"

#ifdef CONFIG_SEC_EXT
#include <linux/sec_ext.h>
#endif

/* Exynos generic registers */
#define EXYNOS_TMU_REG_TRIMINFO		0x0
#define EXYNOS_TMU_REG_TRIMINFO1	0x4
#define EXYNOS_TMU_REG_TRIMINFO2	0x8
#define EXYNOS_TMU_REG_CONTROL		0x20
#define EXYNOS_TMU_REG_CONTROL1		0x24
#define EXYNOS_TMU_REG_STATUS		0x28
#define EXYNOS_TMU_REG_CURRENT_TEMP1_0 	0x40
#define EXYNOS_TMU_REG_CURRENT_TEMP4_2  0x44
#define EXYNOS_TMU_REG_CURRENT_TEMP7_5  0x48
#define EXYNOS_TMU_REG_INTEN0		0x110
#define EXYNOS_TMU_REG_INTEN5		0x310
#define EXYNOS_TMU_REG_INTEN_OFFSET	0x10
#define EXYNOS_TMU_REG_INTSTAT		0x74
#define EXYNOS_TMU_REG_INTCLEAR		0x78

#define EXYNOS_TMU_REF_VOLTAGE_SHIFT	24
#define EXYNOS_TMU_REF_VOLTAGE_MASK	0x1f
#define EXYNOS_TMU_BUF_SLOPE_SEL_MASK	0xf
#define EXYNOS_TMU_BUF_SLOPE_SEL_SHIFT	8
#define EXYNOS_TMU_CORE_EN_SHIFT	0
#define EXYNOS_TMU_MUX_ADDR_SHIFT	20
#define EXYNOS_TMU_MUX_ADDR_MASK	0x7

#define EXYNOS_TMU_TRIP_MODE_SHIFT	13
#define EXYNOS_TMU_TRIP_MODE_MASK	0x7
#define EXYNOS_TMU_THERM_TRIP_EN_SHIFT	12

#define EXYNOS_TMU_INTEN_RISE0_SHIFT	0
#define EXYNOS_TMU_INTEN_FALL0_SHIFT	16

#define EXYNOS_EMUL_TIME	0x57F0
#define EXYNOS_EMUL_TIME_MASK	0xffff
#define EXYNOS_EMUL_TIME_SHIFT	16
#define EXYNOS_EMUL_DATA_SHIFT	7
#define EXYNOS_EMUL_DATA_MASK	0x1FF
#define EXYNOS_EMUL_ENABLE	0x1

#define EXYNOS_THD_TEMP_RISE7_6			0x50
#define EXYNOS_THD_TEMP_FALL7_6			0x60
#define EXYNOS_THD_TEMP_R_OFFSET		0x100
#define EXYNOS_THD_TEMP_RISE7_6_SHIFT		16
#define EXYNOS_TMU_INTEN_RISE0_SHIFT		0
#define EXYNOS_TMU_INTEN_RISE1_SHIFT		1
#define EXYNOS_TMU_INTEN_RISE2_SHIFT		2
#define EXYNOS_TMU_INTEN_RISE3_SHIFT		3
#define EXYNOS_TMU_INTEN_RISE4_SHIFT		4
#define EXYNOS_TMU_INTEN_RISE5_SHIFT		5
#define EXYNOS_TMU_INTEN_RISE6_SHIFT		6
#define EXYNOS_TMU_INTEN_RISE7_SHIFT		7

#define EXYNOS_TMU_CALIB_SEL_SHIFT		(23)
#define EXYNOS_TMU_CALIB_SEL_MASK		(0x1)
#define EXYNOS_TMU_TEMP_SHIFT			(9)
#define EXYNOS_TMU_TEMP_MASK			(0x1ff)
#define EXYNOS_TMU_TRIMINFO_85_P0_SHIFT		(9)
#define EXYNOS_TRIMINFO_ONE_POINT_TRIMMING	(0)
#define EXYNOS_TRIMINFO_TWO_POINT_TRIMMING	(1)
#define EXYNOS_TMU_T_BUF_VREF_SEL_SHIFT		(18)
#define EXYNOS_TMU_T_BUF_VREF_SEL_MASK		(0x1F)
#define EXYNOS_TMU_T_BUF_SLOPE_SEL_SHIFT	(18)
#define EXYNOS_TMU_T_BUF_SLOPE_SEL_MASK		(0xF)

#define EXYNOS_TMU_REG_INTPEND0			(0x118)
#define EXYNOS_TMU_REG_INTPEND5			(0x318)
#define EXYNOS_TMU_REG_INTPEN_OFFSET		(0x10)
#define EXYNOS_TMU_REG_EMUL_CON			(0x160)

#define EXYNOS_TMU_REG_AVG_CON			(0x38)
#define EXYNOS_TMU_AVG_CON_SHIFT		(18)
#define EXYNOS_TMU_AVG_CON_MASK			(0x3)
#define EXYNOS_TMU_AVG_MODE_MASK		(0x7)
#define EXYNOS_TMU_AVG_MODE_DEFAULT		(0x0)
#define EXYNOS_TMU_AVG_MODE_2			(0x5)
#define EXYNOS_TMU_AVG_MODE_4			(0x6)

#define EXYNOS_TMU_DEM_ENABLE			(1)
#define EXYNOS_TMU_DEM_SHIFT			(4)

#define EXYNOS_TMU_REG_COUNTER_VALUE0		(0x30)
#define EXYNOS_TMU_EN_TEMP_SEN_OFF_SHIFT	(0)
#define EXYNOS_TMU_EN_TEMP_SEN_OFF_MASK		(0xffff)
#define EXYNOS_TMU_REG_COUNTER_VALUE1		(0x34)
#define EXYNOS_TMU_CLK_SENSE_ON_SHIFT		(16)
#define EXYNOS_TMU_CLK_SENSE_ON_MASK		(0xffff)
#define EXYNOS_TMU_TEM1456X_SENSE_VALUE		(0x0A28)

#define EXYNOS_TMU_NUM_PROBE_SHIFT		(16)
#define EXYNOS_TMU_NUM_PROBE_MASK		(0x7)

#define TOTAL_SENSORS	8
#define DEFAULT_BALANCE_OFFSET	20

static bool suspended;
static bool is_cpu_hotplugged_out;
static DEFINE_MUTEX (thermal_suspend_lock);

/* list of multiple instance for each thermal sensor */
static LIST_HEAD(dtm_dev_list);
struct cpufreq_frequency_table gpu_freq_table[10];

static u32 global_avg_con;

static void exynos_report_trigger(struct exynos_tmu_data *p)
{
	struct thermal_zone_device *tz = p->tzd;

	if (!tz) {
		pr_err("No thermal zone device defined\n");
		return;
	}

	thermal_zone_device_update(tz);
}

/*
 * TMU treats temperature as a mapped temperature code.
 * The temperature is converted differently depending on the calibration type.
 */
static int temp_to_code(struct exynos_tmu_data *data, u8 temp)
{
	struct exynos_tmu_platform_data *pdata = data->pdata;
	int temp_code;

	if (temp > EXYNOS_MAX_TEMP)
		temp = EXYNOS_MAX_TEMP;
	else if (temp < EXYNOS_MIN_TEMP)
		temp = EXYNOS_MIN_TEMP;

	switch (pdata->cal_type) {
	case TYPE_TWO_POINT_TRIMMING:
		temp_code = (temp - pdata->first_point_trim) *
			(data->temp_error2 - data->temp_error1) /
			(pdata->second_point_trim - pdata->first_point_trim) +
			data->temp_error1;
		break;
	case TYPE_ONE_POINT_TRIMMING:
		temp_code = temp + data->temp_error1 - pdata->first_point_trim;
		break;
	default:
		temp_code = temp + pdata->default_temp_offset;
		break;
	}

	return temp_code;
}

/*
 * TMU treats temperature with the index as a mapped temperature code.
 * The temperature is converted differently depending on the calibration type.
 */
static int temp_to_code_with_sensorinfo(struct exynos_tmu_data *data, u16 temp, struct sensor_info *info)
{
	struct exynos_tmu_platform_data *pdata = data->pdata;
	int temp_code;

	if (temp > EXYNOS_MAX_TEMP)
		temp = EXYNOS_MAX_TEMP;
	else if (temp < EXYNOS_MIN_TEMP)
		temp = EXYNOS_MIN_TEMP;

	switch (info->cal_type) {
		case TYPE_TWO_POINT_TRIMMING:
			temp_code = (temp - pdata->first_point_trim) *
				(info->temp_error2 - info->temp_error1) /
				(pdata->second_point_trim - pdata->first_point_trim) +
				info->temp_error1;
			break;
		case TYPE_ONE_POINT_TRIMMING:
			temp_code = temp + info->temp_error1 - pdata->first_point_trim;
			break;
		default:
			temp_code = temp + pdata->default_temp_offset;
			break;
	}

	return temp_code;
}

/*
 * Calculate a temperature value from a temperature code.
 * The unit of the temperature is degree Celsius.
 */
static int code_to_temp(struct exynos_tmu_data *data, u16 temp_code)
{
	struct exynos_tmu_platform_data *pdata = data->pdata;
	int temp;

	switch (pdata->cal_type) {
	case TYPE_TWO_POINT_TRIMMING:
		temp = (temp_code - data->temp_error1) *
			(pdata->second_point_trim - pdata->first_point_trim) /
			(data->temp_error2 - data->temp_error1) +
			pdata->first_point_trim;
		break;
	case TYPE_ONE_POINT_TRIMMING:
		temp = temp_code - data->temp_error1 + pdata->first_point_trim;
		break;
	default:
		temp = temp_code - pdata->default_temp_offset;
		break;
	}

	/* temperature should range between minimum and maximum */
	if (temp > EXYNOS_MAX_TEMP)
		temp = EXYNOS_MAX_TEMP;
	else if (temp < EXYNOS_MIN_TEMP)
		temp = EXYNOS_MIN_TEMP;

	return temp;
}

/*
 * Calculate a temperature value with the index from a temperature code.
 * The unit of the temperature is degree Celsius.
 */
static int code_to_temp_with_sensorinfo(struct exynos_tmu_data *data, u16 temp_code, struct sensor_info *info)
{
	struct exynos_tmu_platform_data *pdata = data->pdata;
	int temp;

	switch (info->cal_type) {
	case TYPE_TWO_POINT_TRIMMING:
		temp = (temp_code - info->temp_error1) *
			(pdata->second_point_trim - pdata->first_point_trim) /
			(info->temp_error2 - info->temp_error1) +
			pdata->first_point_trim;
		break;
	case TYPE_ONE_POINT_TRIMMING:
		temp = temp_code - info->temp_error1 + pdata->first_point_trim;
		break;
	default:
		temp = temp_code - pdata->default_temp_offset;
		break;
	}

	/* temperature should range between minimum and maximum */
	if (temp > EXYNOS_MAX_TEMP)
		temp = EXYNOS_MAX_TEMP;
	else if (temp < EXYNOS_MIN_TEMP)
		temp = EXYNOS_MIN_TEMP;

	return temp;
}

static int exynos_tmu_initialize(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int ret;

	mutex_lock(&data->lock);
	ret = data->tmu_initialize(pdev);
	mutex_unlock(&data->lock);

	return ret;
}

static u32 get_con_reg(struct exynos_tmu_data *data, u32 con)
{
	struct exynos_tmu_platform_data *pdata = data->pdata;

	con &= ~(EXYNOS_TMU_REF_VOLTAGE_MASK << EXYNOS_TMU_REF_VOLTAGE_SHIFT);
	con |= pdata->reference_voltage << EXYNOS_TMU_REF_VOLTAGE_SHIFT;

	con &= ~(EXYNOS_TMU_BUF_SLOPE_SEL_MASK << EXYNOS_TMU_BUF_SLOPE_SEL_SHIFT);
	con |= (pdata->gain << EXYNOS_TMU_BUF_SLOPE_SEL_SHIFT);

	if (pdata->noise_cancel_mode) {
		con &= ~(EXYNOS_TMU_TRIP_MODE_MASK << EXYNOS_TMU_TRIP_MODE_SHIFT);
		con |= (pdata->noise_cancel_mode << EXYNOS_TMU_TRIP_MODE_SHIFT);
	}

	return con;
}

static void exynos_tmu_control(struct platform_device *pdev, bool on)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);

	mutex_lock(&data->lock);
	data->tmu_control(pdev, on);
	mutex_unlock(&data->lock);
}

static int exynos8890_tmu_initialize(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct thermal_zone_device *tz = data->tzd;
	struct exynos_tmu_platform_data *pdata = data->pdata;
	unsigned int rising_threshold = 0, falling_threshold = 0;
	int temp, temp_hist;
	unsigned int trim_info;
	unsigned int reg_off, bit_off;
	int threshold_code, i;

	/* Check tmu core ready status */
	trim_info = readl(data->base + EXYNOS_TMU_REG_TRIMINFO);

	/* Check thermal calibration type */
	pdata->cal_type = (trim_info >> EXYNOS_TMU_CALIB_SEL_SHIFT)
			& EXYNOS_TMU_CALIB_SEL_MASK;

	/* Check temp_error1 and error2 value */
	data->temp_error1 = trim_info & EXYNOS_TMU_TEMP_MASK;
	data->temp_error2 = (trim_info >> EXYNOS_TMU_TRIMINFO_85_P0_SHIFT)
				& EXYNOS_TMU_TEMP_MASK;

	if (!data->temp_error1)
		data->temp_error1 = pdata->efuse_value & EXYNOS_TMU_TEMP_MASK;
	if (!data->temp_error2)
		data->temp_error2 = (pdata->efuse_value >>
					EXYNOS_TMU_TRIMINFO_85_P0_SHIFT)
					& EXYNOS_TMU_TEMP_MASK;
	/* Write temperature code for rising and falling threshold */
	for (i = (of_thermal_get_ntrips(tz) - 1); i >= 0; i--) {
		/*
		 * On exynos7 there are 4 rising and 4 falling threshold
		 * registers (0x50-0x5c and 0x60-0x6c respectively). Each
		 * register holds the value of two threshold levels (at bit
		 * offsets 0 and 16). Based on the fact that there are atmost
		 * eight possible trigger levels, calculate the register and
		 * bit offsets where the threshold levels are to be written.
		 *
		 * e.g. EXYNOS_THD_TEMP_RISE7_6 (0x50)
		 * [24:16] - Threshold level 7
		 * [8:0] - Threshold level 6
		 * e.g. EXYNOS_THD_TEMP_RISE5_4 (0x54)
		 * [24:16] - Threshold level 5
		 * [8:0] - Threshold level 4
		 *
		 * and similarly for falling thresholds.
		 *
		 * Based on the above, calculate the register and bit offsets
		 * for rising/falling threshold levels and populate them.
		 */
		reg_off = ((7 - i) / 2) * 4;
		bit_off = ((8 - i) % 2);

		tz->ops->get_trip_temp(tz, i, &temp);
		temp /= MCELSIUS;

		tz->ops->get_trip_hyst(tz, i, &temp_hist);
		temp_hist = temp - (temp_hist / MCELSIUS);

		/* Set 9-bit temperature code for rising threshold levels */
		threshold_code = temp_to_code(data, temp);
		rising_threshold = readl(data->base +
			EXYNOS_THD_TEMP_RISE7_6 + reg_off);
		rising_threshold &= ~(EXYNOS_TMU_TEMP_MASK << (16 * bit_off));
		rising_threshold |= threshold_code << (16 * bit_off);
		writel(rising_threshold,
		       data->base + EXYNOS_THD_TEMP_RISE7_6 + reg_off);

		/* Set 9-bit temperature code for falling threshold levels */
		threshold_code = temp_to_code(data, temp_hist);
		falling_threshold &= ~(EXYNOS_TMU_TEMP_MASK << (16 * bit_off));
		falling_threshold |= threshold_code << (16 * bit_off);
		writel(falling_threshold,
		       data->base + EXYNOS_THD_TEMP_FALL7_6 + reg_off);
	}

	data->tmu_clear_irqs(data);

	return 0;
}

static void exynos8890_tmu_control(struct platform_device *pdev, bool on)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct thermal_zone_device *tz = data->tzd;
	unsigned int con, interrupt_en, trim_info, trim_info1;
	unsigned int t_buf_vref_sel, t_buf_slope_sel;

	trim_info = readl(data->base + EXYNOS_TMU_REG_TRIMINFO);
	trim_info1 = readl(data->base + EXYNOS_TMU_REG_TRIMINFO1);

	/* Save fuse buf_vref_sel, calib_sel value to TRIMINFO and 1 register */
	t_buf_vref_sel = (trim_info >> EXYNOS_TMU_T_BUF_VREF_SEL_SHIFT)
				& (EXYNOS_TMU_T_BUF_VREF_SEL_MASK);
	t_buf_slope_sel = (trim_info1 >> EXYNOS_TMU_T_BUF_SLOPE_SEL_SHIFT)
				& (EXYNOS_TMU_T_BUF_SLOPE_SEL_MASK);

	con = get_con_reg(data, readl(data->base + EXYNOS_TMU_REG_CONTROL));

	if (on) {
		con |= (t_buf_vref_sel << EXYNOS_TMU_REF_VOLTAGE_SHIFT);
		con |= (t_buf_slope_sel << EXYNOS_TMU_BUF_SLOPE_SEL_SHIFT);
		con |= (1 << EXYNOS_TMU_CORE_EN_SHIFT);
		con |= (1 << EXYNOS_TMU_THERM_TRIP_EN_SHIFT);
		interrupt_en =
			(of_thermal_is_trip_valid(tz, 7)
			<< EXYNOS_TMU_INTEN_RISE7_SHIFT) |
			(of_thermal_is_trip_valid(tz, 6)
			<< EXYNOS_TMU_INTEN_RISE6_SHIFT) |
			(of_thermal_is_trip_valid(tz, 5)
			<< EXYNOS_TMU_INTEN_RISE5_SHIFT) |
			(of_thermal_is_trip_valid(tz, 4)
			<< EXYNOS_TMU_INTEN_RISE4_SHIFT) |
			(of_thermal_is_trip_valid(tz, 3)
			<< EXYNOS_TMU_INTEN_RISE3_SHIFT) |
			(of_thermal_is_trip_valid(tz, 2)
			<< EXYNOS_TMU_INTEN_RISE2_SHIFT) |
			(of_thermal_is_trip_valid(tz, 1)
			<< EXYNOS_TMU_INTEN_RISE1_SHIFT) |
			(of_thermal_is_trip_valid(tz, 0)
			<< EXYNOS_TMU_INTEN_RISE0_SHIFT);

		interrupt_en |=
			interrupt_en << EXYNOS_TMU_INTEN_FALL0_SHIFT;
	} else {
		con &= ~(1 << EXYNOS_TMU_CORE_EN_SHIFT);
		con &= ~(1 << EXYNOS_TMU_THERM_TRIP_EN_SHIFT);
		interrupt_en = 0; /* Disable all interrupts */
	}

	writel(interrupt_en, data->base + EXYNOS_TMU_REG_INTEN0);
	writel(con, data->base + EXYNOS_TMU_REG_CONTROL);
}

static int exynos8895_tmu_initialize(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct thermal_zone_device *tz = data->tzd;
	struct exynos_tmu_platform_data *pdata = data->pdata;
	unsigned int rising_threshold = 0, falling_threshold = 0;
	int temp, temp_hist;
	unsigned int trim_info;
	unsigned int reg_off = 0, bit_off;
	int threshold_code, i;
	int sensor;
	int count = 0, interrupt_count = 0;
	struct sensor_info temp_info;
	enum thermal_trip_type type;

	u16 cal_type;
	u32 temp_error1;
	u32 temp_error2;

	trim_info = readl(data->base + EXYNOS_TMU_REG_TRIMINFO);
	cal_type = (trim_info >> EXYNOS_TMU_CALIB_SEL_SHIFT) & EXYNOS_TMU_CALIB_SEL_MASK;

	for (sensor = 0; sensor < TOTAL_SENSORS; sensor++) {
		/* Read the sensor error value from TRIMINFOX */
		trim_info = readl(data->base + EXYNOS_TMU_REG_TRIMINFO + 0x4 * sensor);
		temp_error1 = trim_info & EXYNOS_TMU_TEMP_MASK;
		temp_error2 = (trim_info >> EXYNOS_TMU_TRIMINFO_85_P0_SHIFT) & EXYNOS_TMU_TEMP_MASK;

		/* If the sensor is used, save its information. */
		if (data->sensors & (1 << sensor)) {
			/* Save sensor id */
			data->sensor_info[count].sensor_num = sensor;
			dev_info(&pdev->dev, "Sensor number = %d\n", sensor);

			/* Check thermal calibration type */
			data->sensor_info[count].cal_type = cal_type;

			/* Check temp_error1 value */
			data->sensor_info[count].temp_error1 = temp_error1;
			if (!data->sensor_info[count].temp_error1)
				data->sensor_info[count].temp_error1 = pdata->efuse_value & EXYNOS_TMU_TEMP_MASK;

			/* Check temp_error2 if calibration type is TYPE_TWO_POINT_TRIMMING */
			if (data->sensor_info[count].cal_type == TYPE_TWO_POINT_TRIMMING) {
				data->sensor_info[count].temp_error2 = temp_error2;

				if (!data->sensor_info[count].temp_error2)
					data->sensor_info[count].temp_error2 = (pdata->efuse_value >> EXYNOS_TMU_TRIMINFO_85_P0_SHIFT)
									& EXYNOS_TMU_TEMP_MASK;
			}
		}

		temp_info.cal_type = cal_type;
		temp_info.temp_error1 = temp_error1;
		temp_info.temp_error2 = temp_error2;

		if (data->sensors & (1 << sensor)) {
			interrupt_count = 0;
			/* Write temperature code for rising and falling threshold */
			for (i = (of_thermal_get_ntrips(tz) - 1); i >= 0; i--) {
				/*
				 * On exynos8 there are 4 rising and 4 falling threshold
				 * registers (0x50-0x5c and 0x60-0x6c respectively). Each
				 * register holds the value of two threshold levels (at bit
				 * offsets 0 and 16). Based on the fact that there are atmost
				 * eight possible trigger levels, calculate the register and
				 * bit offsets where the threshold levels are to be written.
				 *
				 * e.g. EXYNOS_THD_TEMP_RISE7_6 (0x50)
				 * [24:16] - Threshold level 7
				 * [8:0] - Threshold level 6
				 * e.g. EXYNOS_THD_TEMP_RISE5_4 (0x54)
				 * [24:16] - Threshold level 5
				 * [8:0] - Threshold level 4
				 *
				 * and similarly for falling thresholds.
				 *
				 * Based on the above, calculate the register and bit offsets
				 * for rising/falling threshold levels and populate them.
				 */

				tz->ops->get_trip_type(tz, i, &type);

				if (type == THERMAL_TRIP_PASSIVE)
					continue;

				reg_off = (interrupt_count / 2) * 4;
				bit_off = ((interrupt_count + 1) % 2);

				if (sensor > 0)
					reg_off = reg_off + EXYNOS_THD_TEMP_R_OFFSET + sensor * 0x20;

				tz->ops->get_trip_temp(tz, i, &temp);
				temp /= MCELSIUS;

				tz->ops->get_trip_hyst(tz, i, &temp_hist);
				temp_hist = temp - (temp_hist / MCELSIUS);

				/* Set 9-bit temperature code for rising threshold levels */
				threshold_code = temp_to_code_with_sensorinfo(data, temp, &data->sensor_info[count]);
				rising_threshold = readl(data->base +
					EXYNOS_THD_TEMP_RISE7_6 + reg_off);
				rising_threshold &= ~(EXYNOS_TMU_TEMP_MASK << (16 * bit_off));
				rising_threshold |= threshold_code << (16 * bit_off);
				writel(rising_threshold,
				       data->base + EXYNOS_THD_TEMP_RISE7_6 + reg_off);

				/* Set 9-bit temperature code for falling threshold levels */
				threshold_code = temp_to_code_with_sensorinfo(data, temp_hist, &data->sensor_info[count]);
				falling_threshold &= ~(EXYNOS_TMU_TEMP_MASK << (16 * bit_off));
				falling_threshold |= threshold_code << (16 * bit_off);
				writel(falling_threshold,
				       data->base + EXYNOS_THD_TEMP_FALL7_6 + reg_off);

				interrupt_count++;
			}
			count++;
		}
	}

	data->tmu_clear_irqs(data);

	return 0;
}

static void exynos8895_tmu_control(struct platform_device *pdev, bool on)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct thermal_zone_device *tz = data->tzd;
	unsigned int con, con1, interrupt_en, trim_info, trim_info1, trim_info2;
	unsigned int t_buf_vref_sel, t_buf_slope_sel;
	unsigned int bit_off;
	int i, interrupt_count = 0;
	u32 avg_con, avg_sel;
	u32 mux_val;
	u32 counter_value0, counter_value1;
	enum thermal_trip_type type;

	interrupt_en = 0;
	con = readl(data->base + EXYNOS_TMU_REG_CONTROL);
	con &= ~(1 << EXYNOS_TMU_CORE_EN_SHIFT);
	con &= ~(1 << EXYNOS_TMU_THERM_TRIP_EN_SHIFT);
	writel(con, data->base + EXYNOS_TMU_REG_CONTROL);
	con = 0;

	con1 = readl(data->base + EXYNOS_TMU_REG_CONTROL1);
	con1 &= ~(EXYNOS_TMU_NUM_PROBE_MASK << EXYNOS_TMU_NUM_PROBE_SHIFT);
	con1 |= (data->num_probe << EXYNOS_TMU_NUM_PROBE_SHIFT);
	writel(con1, data->base + EXYNOS_TMU_REG_CONTROL1);

	trim_info = readl(data->base + EXYNOS_TMU_REG_TRIMINFO);
	trim_info1 = readl(data->base + EXYNOS_TMU_REG_TRIMINFO1);
	trim_info2 = readl(data->base + EXYNOS_TMU_REG_TRIMINFO2);

	/* Save fuse buf_vref_sel, calib_sel value to TRIMINFO and 1 register */
	t_buf_vref_sel = (trim_info >> EXYNOS_TMU_T_BUF_VREF_SEL_SHIFT)
				& (EXYNOS_TMU_T_BUF_VREF_SEL_MASK);
	t_buf_slope_sel = (trim_info1 >> EXYNOS_TMU_T_BUF_SLOPE_SEL_SHIFT)
				& (EXYNOS_TMU_T_BUF_SLOPE_SEL_MASK);
	avg_sel = (trim_info2 >> EXYNOS_TMU_AVG_CON_SHIFT) & EXYNOS_TMU_AVG_CON_MASK;

	con = get_con_reg(data, readl(data->base + EXYNOS_TMU_REG_CONTROL));
	avg_con = readl(data->base + EXYNOS_TMU_REG_AVG_CON);

	if (avg_sel) {
		avg_con |= ((avg_con & EXYNOS_TMU_AVG_MODE_MASK) | EXYNOS_TMU_AVG_MODE_DEFAULT);
		avg_con &= ~(EXYNOS_TMU_DEM_ENABLE << EXYNOS_TMU_DEM_SHIFT);
	} else {
		avg_con |= ((avg_con & EXYNOS_TMU_AVG_MODE_MASK) | EXYNOS_TMU_AVG_MODE_4);
		avg_con |= EXYNOS_TMU_DEM_ENABLE << EXYNOS_TMU_DEM_SHIFT;
	}

	if (data->id == 0 || data->id == 1)
		global_avg_con = avg_con;

	/* Set MUX_ADDR SFR according to sensor_type */
	switch (data->pdata->sensor_type) {
		case TEM1456X :
			counter_value0 = readl(data->base + EXYNOS_TMU_REG_COUNTER_VALUE0);
			counter_value0 &= ~(EXYNOS_TMU_EN_TEMP_SEN_OFF_MASK << EXYNOS_TMU_EN_TEMP_SEN_OFF_SHIFT);
			counter_value0 |= EXYNOS_TMU_TEM1456X_SENSE_VALUE << EXYNOS_TMU_EN_TEMP_SEN_OFF_SHIFT;
			writel(counter_value0, data->base + EXYNOS_TMU_REG_COUNTER_VALUE0);

			counter_value1 = readl(data->base + EXYNOS_TMU_REG_COUNTER_VALUE1);
			counter_value1 &= ~(EXYNOS_TMU_CLK_SENSE_ON_MASK << EXYNOS_TMU_CLK_SENSE_ON_SHIFT);
			counter_value1 |= EXYNOS_TMU_TEM1456X_SENSE_VALUE << EXYNOS_TMU_CLK_SENSE_ON_SHIFT;
			writel(counter_value1, data->base + EXYNOS_TMU_REG_COUNTER_VALUE1);
		case TEM1455X :
			mux_val = (data->pdata->sensor_type << EXYNOS_TMU_MUX_ADDR_SHIFT);
			con |= (con & ~(EXYNOS_TMU_MUX_ADDR_MASK << EXYNOS_TMU_MUX_ADDR_SHIFT)) | mux_val;
			break;
	}

	if (on) {
		con |= (t_buf_vref_sel << EXYNOS_TMU_REF_VOLTAGE_SHIFT);
		con |= (t_buf_slope_sel << EXYNOS_TMU_BUF_SLOPE_SEL_SHIFT);
		con |= (1 << EXYNOS_TMU_CORE_EN_SHIFT);
		con |= (1 << EXYNOS_TMU_THERM_TRIP_EN_SHIFT);

		for (i = (of_thermal_get_ntrips(tz) - 1); i >= 0; i--) {

			tz->ops->get_trip_type(tz, i, &type);

			if (type == THERMAL_TRIP_PASSIVE)
				continue;

			bit_off = EXYNOS_TMU_INTEN_RISE7_SHIFT - interrupt_count;

			interrupt_en |= of_thermal_is_trip_valid(tz, i) << bit_off;
			interrupt_count++;
		}

		interrupt_en |=
			interrupt_en << EXYNOS_TMU_INTEN_FALL0_SHIFT;
	} else {
		con &= ~(1 << EXYNOS_TMU_CORE_EN_SHIFT);
		con &= ~(1 << EXYNOS_TMU_THERM_TRIP_EN_SHIFT);
		interrupt_en = 0; /* Disable all interrupts */
	}

	for (i = 0; i < TOTAL_SENSORS; i++) {
		if (data->sensors & (1 << i)) {
			int int_offset;

			if (i < 5)
				int_offset = EXYNOS_TMU_REG_INTEN0 + EXYNOS_TMU_REG_INTEN_OFFSET * i;
			else
				int_offset = EXYNOS_TMU_REG_INTEN5 + EXYNOS_TMU_REG_INTEN_OFFSET * (i - 5) ;

			writel(interrupt_en, data->base + int_offset);
		}
	}
	writel(con, data->base + EXYNOS_TMU_REG_CONTROL);

	/* Only TEM1002X sensor needs AVG_CON setting */
	if (data->pdata->sensor_type == TEM1002X)
		writel(global_avg_con, data->base + EXYNOS_TMU_REG_AVG_CON);

}

#define MCINFO_LOG_THRESHOLD	(4)

static int exynos_get_temp(void *p, int *temp)
{
	struct exynos_tmu_data *data = p;
	struct thermal_cooling_device *cdev;
	unsigned int mcinfo_count;
	unsigned int mcinfo_result[4] = {0, 0, 0, 0};
	unsigned int mcinfo_logging = 0;
	unsigned int mcinfo_temp = 0;
	unsigned int i;

	if (!data || !data->tmu_read)
		return -EINVAL;

	mutex_lock(&data->lock);

	if (data->num_of_sensors)
		*temp = data->tmu_read(data) * MCELSIUS;
	else
		*temp = code_to_temp(data, data->tmu_read(data)) * MCELSIUS;

	mutex_unlock(&data->lock);

	cdev = data->cool_dev;

	if (!cdev)
		return 0;

	mutex_lock(&thermal_suspend_lock);

	if (cdev->ops->set_cur_temp && data->id != 1)
		cdev->ops->set_cur_temp(cdev, suspended, *temp / 1000);

	mutex_unlock(&thermal_suspend_lock);

	exynos_ss_thermal(data->pdata, *temp / 1000, data->tmu_name, 0);

	if (data->id == 0) {
		mcinfo_count = get_mcinfo_base_count();
		get_refresh_rate(mcinfo_result);

		for (i = 0; i < mcinfo_count; i++) {
			mcinfo_temp |= (mcinfo_result[i] & 0xf) << (8 * i);

			if (mcinfo_result[i] >= MCINFO_LOG_THRESHOLD)
				mcinfo_logging = 1;
		}

		if (mcinfo_logging == 1)
			exynos_ss_thermal(NULL, mcinfo_temp, "MCINFO", 0);
	}
	return 0;
}

#ifdef CONFIG_SEC_BOOTSTAT
void sec_bootstat_get_thermal(int *temp)
{
	struct exynos_tmu_data *data;

	list_for_each_entry(data, &dtm_dev_list, node) {
		if (!strncasecmp(data->tmu_name, "MNGS", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[0]);
			temp[0] /= 1000;
		}
		else if (!strncasecmp(data->tmu_name, "APOLLO", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[1]);
			temp[1] /= 1000;
		}
		else if (!strncasecmp(data->tmu_name, "GPU", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[2]);
			temp[2] /= 1000;
		}
		else if (!strncasecmp(data->tmu_name, "ISP", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[3]);
			temp[3] /= 1000;
		}
		else
			continue;
	}
}
#endif

#ifdef CONFIG_SEC_PM
static ssize_t
exynos_tmu_curr_temp(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct exynos_tmu_data *data;
	int temp[4];
	int len = 0;

	list_for_each_entry(data, &dtm_dev_list, node) {
		if (!strncasecmp(data->tmu_name, "MNGS", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[0]);
			temp[0] /= 1000;
		}
		else if (!strncasecmp(data->tmu_name, "APOLLO", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[1]);
			temp[1] /= 1000;
		}
		else if (!strncasecmp(data->tmu_name, "GPU", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[2]);
			temp[2] /= 1000;
		}
		else if (!strncasecmp(data->tmu_name, "ISP", THERMAL_NAME_LENGTH)) {
			exynos_get_temp(data, &temp[3]);
			temp[3] /= 1000;
		}
		else {
			pr_info("%s: %s\n", __func__, data->tmu_name);
			continue;
		}
	}

	len += snprintf(&buf[len], PAGE_SIZE,"%d,", temp[0]);
	len += snprintf(&buf[len], PAGE_SIZE,"%d,", temp[1]);
	len += snprintf(&buf[len], PAGE_SIZE,"%d,", temp[2]);
	len += snprintf(&buf[len], PAGE_SIZE,"%d", temp[3]);
	len += snprintf(&buf[len], PAGE_SIZE,"\n");

	return len;
}

static DEVICE_ATTR(curr_temp, S_IRUGO, exynos_tmu_curr_temp, NULL);


static struct attribute *exynos_thermal_sensor_attributes[] = {
	&dev_attr_curr_temp.attr,
	NULL
};

static const struct attribute_group exynos_thermal_sensor_attr_group = {
	.attrs = exynos_thermal_sensor_attributes,
};
#endif

#ifdef CONFIG_THERMAL_EMULATION
static u32 get_emul_con_reg(struct exynos_tmu_data *data, unsigned int val,
			    int temp)
{
	if (temp) {
		temp /= MCELSIUS;
		val &= ~(EXYNOS_EMUL_DATA_MASK <<
			EXYNOS_EMUL_DATA_SHIFT);
		val |= (temp_to_code(data, temp) <<
			EXYNOS_EMUL_DATA_SHIFT) |
			EXYNOS_EMUL_ENABLE;
	} else {
		val &= ~EXYNOS_EMUL_ENABLE;
	}

	return val;
}

static void exynos8890_tmu_set_emulation(struct exynos_tmu_data *data,
					 int temp)
{
	unsigned int val;
	u32 emul_con;

	emul_con = EXYNOS_TMU_REG_EMUL_CON;

	val = readl(data->base + emul_con);
	val = get_emul_con_reg(data, val, temp);
	writel(val, data->base + emul_con);
}

static void exynos8895_tmu_set_emulation(struct exynos_tmu_data *data,
					 int temp)
{
	unsigned int val;
	u32 emul_con;

	emul_con = EXYNOS_TMU_REG_EMUL_CON;

	val = readl(data->base + emul_con);

	if (temp) {
		temp /= MCELSIUS;
		val &= ~(EXYNOS_EMUL_DATA_MASK << EXYNOS_EMUL_DATA_SHIFT);
		val |= (temp_to_code_with_sensorinfo(data, temp, &data->sensor_info[0]) << EXYNOS_EMUL_DATA_SHIFT)
			| EXYNOS_EMUL_ENABLE;
	} else {
		val &= ~EXYNOS_EMUL_ENABLE;
	}

	writel(val, data->base + emul_con);
}

static int exynos_tmu_set_emulation(void *drv_data, int temp)
{
	struct exynos_tmu_data *data = drv_data;
	int ret = -EINVAL;

	if (temp && temp < MCELSIUS)
		goto out;

	mutex_lock(&data->lock);
	data->tmu_set_emulation(data, temp);
	mutex_unlock(&data->lock);
	return 0;
out:
	return ret;
}
#else
#define exynos8890_tmu_set_emulation NULL
#define exynos8895_tmu_set_emulation NULL
static int exynos_tmu_set_emulation(void *drv_data, int temp)
	{ return -EINVAL; }
#endif /* CONFIG_THERMAL_EMULATION */

static int exynos8890_tmu_read(struct exynos_tmu_data *data)
{
	return readw(data->base + EXYNOS_TMU_REG_CURRENT_TEMP1_0) &
		EXYNOS_TMU_TEMP_MASK;
}

static int exynos8895_tmu_read(struct exynos_tmu_data *data)
{
	u8 i;
	u32 reg_offset, bit_offset;
	u32 temp_code, temp_cel;
	u32 count = 0, result = 0;

	for (i = 0; i < data->num_of_sensors; i++) {
		if (data->sensor_info[i].sensor_num < 2) {
			reg_offset = 0;
			bit_offset = EXYNOS_TMU_TEMP_SHIFT * data->sensor_info[i].sensor_num;
		} else {
			reg_offset = ((data->sensor_info[i].sensor_num - 2) / 3 + 1) * 4;
			bit_offset = EXYNOS_TMU_TEMP_SHIFT * ((data->sensor_info[i].sensor_num - 2) % 3);
		}

		temp_code = (readl(data->base + EXYNOS_TMU_REG_CURRENT_TEMP1_0 + reg_offset)
				>> bit_offset) & EXYNOS_TMU_TEMP_MASK;
		temp_cel = code_to_temp_with_sensorinfo(data, temp_code, &data->sensor_info[i]);

		switch (data->sensing_mode) {
			case AVG : result = result + temp_cel;
				break;
			case MAX : result = result > temp_cel ? result : temp_cel;
				break;
			case MIN : result = result < temp_cel ? result : temp_cel;
				break;
			case BALANCE:
				if (data->sensor_info[i].sensor_num != 0) {
					if (temp_cel >= data->balance_offset)
						temp_cel = temp_cel - data->balance_offset;
					else
						temp_cel = 0;
				}
				result = result > temp_cel ? result : temp_cel;
				break;
			default : result = temp_cel;
				break;
		}
		count++;
	}

	switch (data->sensing_mode) {
		case AVG :
			if (count != 0)
				result = result / count;
			break;
		case MAX :
		case MIN :
		case BALANCE:
		default :
			break;
	}

	return result;
}

static void exynos_tmu_work(struct work_struct *work)
{
	struct exynos_tmu_data *data = container_of(work,
			struct exynos_tmu_data, irq_work);

	exynos_report_trigger(data);
	mutex_lock(&data->lock);

	/* TODO: take action based on particular interrupt */
	data->tmu_clear_irqs(data);

	mutex_unlock(&data->lock);
	enable_irq(data->irq);
}

static void exynos8890_tmu_clear_irqs(struct exynos_tmu_data *data)
{
	unsigned int val_irq;

	val_irq = readl(data->base + EXYNOS_TMU_REG_INTPEND0);
	writel(val_irq, data->base + EXYNOS_TMU_REG_INTPEND0);
}

static void exynos8895_tmu_clear_irqs(struct exynos_tmu_data *data)
{
	unsigned int i, val_irq;
	u32 pend_reg;

	for (i = 0; i < TOTAL_SENSORS; i++) {
		if (data->sensors & (1 << i)) {
			if (i < 5)
				pend_reg = EXYNOS_TMU_REG_INTPEND0 + EXYNOS_TMU_REG_INTPEN_OFFSET * i;
			else
				pend_reg = EXYNOS_TMU_REG_INTPEND5 + EXYNOS_TMU_REG_INTPEN_OFFSET * (i - 5);

			val_irq = readl(data->base + pend_reg);
			writel(val_irq, data->base + pend_reg);
		}
	}
}

static irqreturn_t exynos_tmu_irq(int irq, void *id)
{
	struct exynos_tmu_data *data = id;

	disable_irq_nosync(irq);
	schedule_work(&data->irq_work);

	return IRQ_HANDLED;
}

static int exynos_pm_notifier(struct notifier_block *notifier,
			unsigned long event, void *v)
{
	struct exynos_tmu_data *devnode;
	struct thermal_cooling_device *cdev;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		mutex_lock(&thermal_suspend_lock);
		suspended = true;
		list_for_each_entry(devnode, &dtm_dev_list, node) {
			cdev = devnode->cool_dev;

			if (cdev && cdev->ops->set_cur_temp && devnode->id != 1)
				cdev->ops->set_cur_temp(cdev, suspended, 0);
		}
		mutex_unlock(&thermal_suspend_lock);
		break;
	case PM_POST_SUSPEND:
		mutex_lock(&thermal_suspend_lock);
		suspended = false;
		list_for_each_entry(devnode, &dtm_dev_list, node) {
			cdev = devnode->cool_dev;

			if (cdev && cdev->ops->set_cur_temp && devnode->id != 1)
				cdev->ops->set_cur_temp(cdev, suspended, devnode->tzd->temperature / 1000);
		}
		mutex_unlock(&thermal_suspend_lock);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_tmu_pm_notifier = {
	.notifier_call = exynos_pm_notifier,
};

static const struct of_device_id exynos_tmu_match[] = {
	{ .compatible = "samsung,exynos8890-tmu", },
	{ .compatible = "samsung,exynos8895-tmu", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, exynos_tmu_match);

static int exynos_of_get_soc_type(struct device_node *np)
{
	if (of_device_is_compatible(np, "samsung,exynos8890-tmu"))
		return SOC_ARCH_EXYNOS8890;
	if (of_device_is_compatible(np, "samsung,exynos8895-tmu"))
		return SOC_ARCH_EXYNOS8895;

	return -EINVAL;
}

static int exynos_of_sensor_conf(struct device_node *np,
				 struct exynos_tmu_platform_data *pdata)
{
	u32 value;
	int ret;

	of_node_get(np);

	ret = of_property_read_u32(np, "samsung,tmu_gain", &value);
	pdata->gain = (u8)value;
	of_property_read_u32(np, "samsung,tmu_reference_voltage", &value);
	pdata->reference_voltage = (u8)value;
	of_property_read_u32(np, "samsung,tmu_noise_cancel_mode", &value);
	pdata->noise_cancel_mode = (u8)value;

	of_property_read_u32(np, "samsung,tmu_efuse_value",
			     &pdata->efuse_value);

	of_property_read_u32(np, "samsung,tmu_first_point_trim", &value);
	pdata->first_point_trim = (u8)value;
	of_property_read_u32(np, "samsung,tmu_second_point_trim", &value);
	pdata->second_point_trim = (u8)value;
	of_property_read_u32(np, "samsung,tmu_default_temp_offset", &value);
	pdata->default_temp_offset = (u8)value;

	of_property_read_u32(np, "samsung,tmu_default_trip_temp", &pdata->trip_temp);
	of_property_read_u32(np, "samsung,tmu_cal_type", &pdata->cal_type);

	if (of_property_read_u32(np, "samsung,tmu_sensor_type", &pdata->sensor_type))
	        pr_err("%s: failed to get thermel sensor type\n", __func__);

	of_node_put(np);
	return 0;
}

static int exynos_map_dt_data(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct exynos_tmu_platform_data *pdata;
	struct resource res;
	int i;
	const char *temp, *tmu_name;

	if (!data || !pdev->dev.of_node)
		return -ENODEV;

	data->np = pdev->dev.of_node;

	if (of_property_read_u32(pdev->dev.of_node, "id", &data->id)) {
		dev_err(&pdev->dev, "failed to get TMU ID\n");
		return -ENODEV;
	}

	data->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (data->irq <= 0) {
		dev_err(&pdev->dev, "failed to get IRQ\n");
		return -ENODEV;
	}

	if (of_address_to_resource(pdev->dev.of_node, 0, &res)) {
		dev_err(&pdev->dev, "failed to get Resource 0\n");
		return -ENODEV;
	}

	data->base = devm_ioremap(&pdev->dev, res.start, resource_size(&res));
	if (!data->base) {
		dev_err(&pdev->dev, "Failed to ioremap memory\n");
		return -EADDRNOTAVAIL;
	}

	/* If remote sensor is exist, parse it. Remote sensor is used when reading the temperature. */
	if (!of_property_read_u32(pdev->dev.of_node, "sensors", &data->sensors)) {
		for (i = 0; i < TOTAL_SENSORS; i++) {
			if (data->sensors & (1 << i))
				data->num_of_sensors++;
		}

		data->sensor_info = kzalloc(sizeof(struct sensor_info) * data->num_of_sensors, GFP_KERNEL);
	} else {
		dev_err(&pdev->dev, "failed to get sensors information \n");
		return -ENODEV;
	}

	if (of_property_read_string(pdev->dev.of_node, "tmu_name", &tmu_name)) {
		dev_err(&pdev->dev, "failed to get tmu_name\n");
	} else
		strncpy(data->tmu_name, tmu_name, THERMAL_NAME_LENGTH);

	if (of_property_read_string(pdev->dev.of_node, "sensing_mode", &temp))
	        dev_err(&pdev->dev, "failed to get sensing_mode of thermel sensor\n");
	else {
		for (i = 0; i<ARRAY_SIZE(sensing_method); i++)
			if (!strcasecmp(temp, sensing_method[i]))
				data->sensing_mode = i;
	}

	data->balance_offset = DEFAULT_BALANCE_OFFSET;

	data->hotplug_enable = of_property_read_bool(pdev->dev.of_node, "hotplug_enable");
	if (data->hotplug_enable) {
		dev_info(&pdev->dev, "thermal zone use hotplug function \n");
		of_property_read_u32(pdev->dev.of_node, "hotplug_in_threshold",
					&data->hotplug_in_threshold);
		if (!data->hotplug_in_threshold)
			dev_err(&pdev->dev, "No input hotplug_in_threshold \n");

		of_property_read_u32(pdev->dev.of_node, "hotplug_out_threshold",
					&data->hotplug_out_threshold);
		if (!data->hotplug_out_threshold)
			dev_err(&pdev->dev, "No input hotplug_out_threshold \n");
	}

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct exynos_tmu_platform_data), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	exynos_of_sensor_conf(pdev->dev.of_node, pdata);
	data->pdata = pdata;
	data->soc = exynos_of_get_soc_type(pdev->dev.of_node);

	switch (data->soc) {
	case SOC_ARCH_EXYNOS8890:
		data->tmu_initialize = exynos8890_tmu_initialize;
		data->tmu_control = exynos8890_tmu_control;
		data->tmu_read = exynos8890_tmu_read;
		data->tmu_set_emulation = exynos8890_tmu_set_emulation;
		data->tmu_clear_irqs = exynos8890_tmu_clear_irqs;
		break;
	case SOC_ARCH_EXYNOS8895:
		data->tmu_initialize = exynos8895_tmu_initialize;
		data->tmu_control = exynos8895_tmu_control;
		data->tmu_read = exynos8895_tmu_read;
		data->tmu_set_emulation = exynos8895_tmu_set_emulation;
		data->tmu_clear_irqs = exynos8895_tmu_clear_irqs;
		break;

	default:
		dev_err(&pdev->dev, "Platform not supported\n");
		return -EINVAL;
	}

	return 0;
}
#ifdef CONFIG_GPU_THERMAL
static int gpu_cooling_table_init(struct platform_device *pdev)
{
	struct cpufreq_frequency_table *table_ptr;
	unsigned int table_size;
	u32 gpu_idx_num = 0;
	int ret = 0, i = 0;

	/* gpu cooling frequency table parse */
	ret = of_property_read_u32(pdev->dev.of_node, "gpu_idx_num",
					&gpu_idx_num);
	if (ret < 0)
		dev_err(&pdev->dev, "gpu_idx_num happend error value\n");

	if (gpu_idx_num) {
		table_ptr = kzalloc(sizeof(struct cpufreq_frequency_table)
						* gpu_idx_num, GFP_KERNEL);
		if (!table_ptr) {
			dev_err(&pdev->dev, "failed to allocate for gpu_table\n");
			return -ENODEV;
		}
		table_size = sizeof(struct cpufreq_frequency_table) /
							sizeof(unsigned int);
		ret = of_property_read_u32_array(pdev->dev.of_node, "gpu_cooling_table",
			(unsigned int *)table_ptr, table_size * gpu_idx_num);

		for (i = 0; i < gpu_idx_num; i++) {
			gpu_freq_table[i].flags = table_ptr[i].flags;
			gpu_freq_table[i].driver_data = table_ptr[i].driver_data;
			gpu_freq_table[i].frequency = table_ptr[i].frequency;
			dev_info(&pdev->dev, "[GPU TMU] index : %d, frequency : %d \n",
				gpu_freq_table[i].driver_data, gpu_freq_table[i].frequency);
		}
		kfree(table_ptr);
	}
	return ret;
}
#else
static int gpu_cooling_table_init(struct platform_device *pdev) {return 0;}
#endif

#ifdef CONFIG_SEC_DEBUG_HW_PARAM
static u64 last_time, curr_time;
extern struct thermal_data_devices thermal_data_info[THERMAL_ZONE_MAX];
#endif

struct pm_qos_request thermal_cpu_hotplug_request;
static int exynos_throttle_cpu_hotplug(void *p, int temp)
{
	struct exynos_tmu_data *data = p;
	int ret = 0;

	temp = temp / MCELSIUS;

	if (is_cpu_hotplugged_out) {
		if (temp < data->hotplug_in_threshold) {
			/*
			 * If current temperature is lower than low threshold,
			 * call cluster1_cores_hotplug(false) for hotplugged out cpus.
			 */
			pm_qos_update_request(&thermal_cpu_hotplug_request,
						NR_CPUS);
			is_cpu_hotplugged_out = false;
#ifdef CONFIG_SEC_DEBUG_HW_PARAM
			curr_time = ktime_to_ns(ktime_get()) / 1000000;
			if(last_time) {
				thermal_data_info[THERMAL_ZONE_MNGS].hotplug_out += 
					curr_time - last_time;
			}
#endif
		}
	} else {
		if (temp >= data->hotplug_out_threshold) {
			/*
			 * If current temperature is higher than high threshold,
			 * call cluster1_cores_hotplug(true) to hold temperature down.
			 */
			is_cpu_hotplugged_out = true;

			pm_qos_update_request(&thermal_cpu_hotplug_request,
						NR_HOTPLUG_CPUS);
#ifdef CONFIG_SEC_DEBUG_HW_PARAM
			last_time = ktime_to_ns(ktime_get()) / 1000000;
#endif
		}
	}

	return ret;
}

static struct thermal_zone_of_device_ops exynos_hotplug_sensor_ops = {
	.get_temp = exynos_get_temp,
	.set_emul_temp = exynos_tmu_set_emulation,
	.throttle_cpu_hotplug = exynos_throttle_cpu_hotplug,
};

static struct thermal_zone_of_device_ops exynos_sensor_ops = {
	.get_temp = exynos_get_temp,
	.set_emul_temp = exynos_tmu_set_emulation,
};

static int exynos_cpufreq_cooling_register(struct exynos_tmu_data *data)
{
	struct device_node *np, *child = NULL, *gchild, *ggchild;
	struct device_node *cool_np;
	struct of_phandle_args cooling_spec;
	struct cpumask mask_val;
	int cpu, ret;
	const char *governor_name;
	u32 power_coefficient = 0;
	void *gen_block;
	struct ect_gen_param_table *pwr_coeff;

	np = of_find_node_by_name(NULL, "thermal-zones");
	if (!np)
		return -ENODEV;

	/* Register cpufreq cooling device */
	for_each_child_of_node(np, child) {
		struct device_node *zone_np;
		zone_np = of_parse_phandle(child, "thermal-sensors", 0);

		if (zone_np == data->np) break;
	}

	gchild = of_get_child_by_name(child, "cooling-maps");
	ggchild = of_get_next_child(gchild, NULL);
	ret = of_parse_phandle_with_args(ggchild, "cooling-device", "#cooling-cells",
					 0, &cooling_spec);
	if (ret < 0)
		pr_err("%s do not get cooling spec(err = %d) \n", data->tmu_name, ret);

	cool_np = cooling_spec.np;

	for_each_possible_cpu(cpu)
		if (cpu < NR_CPUS && cpu_topology[cpu].cluster_id == data->id)
			cpumask_copy(&mask_val, topology_core_cpumask(cpu));

	if (!of_property_read_string(child, "governor", &governor_name)) {
		if (!strncasecmp(governor_name, "power_allocator", THERMAL_NAME_LENGTH)) {
			gen_block = ect_get_block("GEN");
			if (gen_block == NULL) {
				pr_err("%s: Failed to get gen block from ECT\n", __func__);
				return -EINVAL;
			}
			pwr_coeff = ect_gen_param_get_table(gen_block, "DTM_PWR_Coeff");
			if (pwr_coeff == NULL) {
				pr_err("%s: Failed to get power coeff from ECT\n", __func__);
				return -EINVAL;
			}
			power_coefficient = pwr_coeff->parameter[data->id];
		}
	}

	data->cool_dev = of_cpufreq_power_cooling_register(cool_np, &mask_val, power_coefficient, NULL);

	if (IS_ERR(data->cool_dev)) {
		data->cool_dev = NULL;
	        pr_err("cooling device register fail (mask = %x) \n", *(unsigned int*)cpumask_bits(&mask_val));
		return -ENODEV;
	}

	return ret;
}

#ifdef CONFIG_GPU_THERMAL

#ifdef CONFIG_MALI_DEBUG_KERNEL_SYSFS
struct exynos_tmu_data *gpu_thermal_data = NULL;
#endif

static int exynos_gpufreq_cooling_register(struct exynos_tmu_data *data)
{
	struct device_node *np, *child = NULL, *gchild, *ggchild;
	struct device_node *cool_np;
	struct of_phandle_args cooling_spec;
	int ret;
	const char *governor_name;
	u32 power_coefficient = 0;
	void *gen_block;
	struct ect_gen_param_table *pwr_coeff;

	np = of_find_node_by_name(NULL, "thermal-zones");
	if (!np)
		return -ENODEV;

	/* Regist gpufreq cooling device */
	for_each_child_of_node(np, child) {
		struct device_node *zone_np;
		zone_np = of_parse_phandle(child, "thermal-sensors", 0);

		if (zone_np == data->np) break;
	}

	gchild = of_get_child_by_name(child, "cooling-maps");
	ggchild = of_get_next_child(gchild, NULL);
	ret = of_parse_phandle_with_args(ggchild, "cooling-device", "#cooling-cells",
					 0, &cooling_spec);
	if (ret < 0)
		pr_err("%s do not get cooling spec(err = %d) \n", data->tmu_name, ret);

	cool_np = cooling_spec.np;

	if (!of_property_read_string(child, "governor", &governor_name)) {
		if (!strncasecmp(governor_name, "power_allocator", THERMAL_NAME_LENGTH)) {
			gen_block = ect_get_block("GEN");
			if (gen_block == NULL) {
				pr_err("%s: Failed to get gen block from ECT\n", __func__);
				return -EINVAL;
			}
			pwr_coeff = ect_gen_param_get_table(gen_block, "DTM_PWR_Coeff");
			if (pwr_coeff == NULL) {
				pr_err("%s: Failed to get power coeff from ECT\n", __func__);
				return -EINVAL;
			}
			power_coefficient = pwr_coeff->parameter[data->id];
		}
	}

	data->cool_dev = of_gpufreq_power_cooling_register(cool_np, NULL, power_coefficient, NULL);

	if (IS_ERR(data->cool_dev)) {
		data->cool_dev = NULL;
	        pr_err("gpu cooling device register fail \n");
		return -ENODEV;
	}

#ifdef CONFIG_MALI_DEBUG_KERNEL_SYSFS
	gpu_thermal_data = data;
#endif

	return ret;
}
#else
static int exynos_gpufreq_cooling_register(struct exynos_tmu_data *data) {return 0;}
#endif

#ifdef CONFIG_ISP_THERMAL
static int exynos_isp_cooling_register(struct exynos_tmu_data *data)
{
	struct device_node *np, *child = NULL, *gchild, *ggchild;
	struct device_node *cool_np;
	struct of_phandle_args cooling_spec;
	int ret;

	np = of_find_node_by_name(NULL, "thermal-zones");
	if (!np)
		return -ENODEV;

	/* Regist isp cooling device */
	for_each_child_of_node(np, child) {
		struct device_node *zone_np;
		zone_np = of_parse_phandle(child, "thermal-sensors", 0);

		if (zone_np == data->np) break;
	}

	gchild = of_get_child_by_name(child, "cooling-maps");
	ggchild = of_get_next_child(gchild, NULL);
	ret = of_parse_phandle_with_args(ggchild, "cooling-device", "#cooling-cells",
					 0, &cooling_spec);
	if (ret < 0)
		pr_err("%s do not get cooling spec(err = %d) \n", data->tmu_name, ret);

	cool_np = cooling_spec.np;

	data->cool_dev = of_isp_cooling_register(cool_np, NULL);

	if (IS_ERR(data->cool_dev)) {
		data->cool_dev = NULL;
	        pr_err("isp cooling device register fail \n");
		return -ENODEV;
	}

	return ret;
}
#else
static int exynos_isp_cooling_register(struct exynos_tmu_data *data) {return 0;}
#endif

extern struct cpumask hmp_fast_cpu_mask;
static int exynos_tmu_cpus_notifier(struct notifier_block *nb,
				    unsigned long event, void *data)
{
	struct exynos_tmu_data *tmudata = container_of(nb, struct exynos_tmu_data, nb);
	struct thermal_zone_device *tz = tmudata->tzd;
	struct cpumask mask;
	int big_cpu_cnt;
	int count;
	struct cpufreq_cooling_device *cpufreq_device = (struct cpufreq_cooling_device *)tmudata->cool_dev->devdata;


	cpumask_copy(&mask, data);
	cpumask_and(&mask, &mask, &hmp_fast_cpu_mask);
	big_cpu_cnt = cpumask_weight(&mask);

	cpumask_copy(&cpufreq_device->target_cpus, &mask);

	switch (event) {
	case CPUS_DOWN_COMPLETE:
		if (big_cpu_cnt == DUAL_CPU) {
			for (count = 0; count < tz->trips; count++)
				thermal_notify_framework(tz, count);
		}
		break;
	case CPUS_UP_PREPARE:
		if (big_cpu_cnt == DUAL_CPU || big_cpu_cnt == QUAD_CPU) {
			for (count = 0; count < tz->trips; count++)
				thermal_notify_framework(tz, count);
		}
		break;
	}
	return NOTIFY_OK;
}

static ssize_t
balance_offset_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->balance_offset);
}

static ssize_t
balance_offset_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int balance_offset = 0;

	mutex_lock(&data->lock);

	if (kstrtos32(buf, 10, &balance_offset)) {
		mutex_unlock(&data->lock);
		return -EINVAL;
	}

	data->balance_offset = balance_offset;

	mutex_unlock(&data->lock);

	return count;
}

static ssize_t
all_temp_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int i, len = 0;
	u32 reg_offset, bit_offset;
	u32 temp_code, temp_cel;

	for (i = 0; i < data->num_of_sensors; i++) {
		if (data->sensor_info[i].sensor_num < 2) {
			reg_offset = 0;
			bit_offset = EXYNOS_TMU_TEMP_SHIFT * data->sensor_info[i].sensor_num;
		} else {
			reg_offset = ((data->sensor_info[i].sensor_num - 2) / 3 + 1) * 4;
			bit_offset = EXYNOS_TMU_TEMP_SHIFT * ((data->sensor_info[i].sensor_num - 2) % 3);
		}

		temp_code = (readl(data->base + EXYNOS_TMU_REG_CURRENT_TEMP1_0 + reg_offset)
				>> bit_offset) & EXYNOS_TMU_TEMP_MASK;
		temp_cel = code_to_temp_with_sensorinfo(data, temp_code, &data->sensor_info[i]);

		len += snprintf(&buf[len], PAGE_SIZE, "%u, ", temp_cel);
	}

	len += snprintf(&buf[len], PAGE_SIZE, "\n");

	return len;
}

static ssize_t
hotplug_out_temp_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->hotplug_out_threshold);
}

static ssize_t
hotplug_out_temp_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int hotplug_out = 0;

	mutex_lock(&data->lock);

	if (kstrtos32(buf, 10, &hotplug_out)) {
		mutex_unlock(&data->lock);
		return -EINVAL;
	}

	data->hotplug_out_threshold = hotplug_out;

	mutex_unlock(&data->lock);

	return count;
}

static ssize_t
hotplug_in_temp_show(struct device *dev, struct device_attribute *devattr,
		       char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->hotplug_in_threshold);
}

static ssize_t
hotplug_in_temp_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	int hotplug_in = 0;

	mutex_lock(&data->lock);

	if (kstrtos32(buf, 10, &hotplug_in)) {
		mutex_unlock(&data->lock);
		return -EINVAL;
	}

	data->hotplug_in_threshold = hotplug_in;

	mutex_unlock(&data->lock);

	return count;
}

static DEVICE_ATTR(balance_offset, S_IWUSR | S_IRUGO, balance_offset_show,
		balance_offset_store);

static DEVICE_ATTR(all_temp, S_IRUGO, all_temp_show, NULL);

static DEVICE_ATTR(hotplug_out_temp, S_IWUSR | S_IRUGO, hotplug_out_temp_show,
		hotplug_out_temp_store);

static DEVICE_ATTR(hotplug_in_temp, S_IWUSR | S_IRUGO, hotplug_in_temp_show,
		hotplug_in_temp_store);

static struct attribute *exynos_tmu_attrs[] = {
	&dev_attr_balance_offset.attr,
	&dev_attr_all_temp.attr,
	&dev_attr_hotplug_out_temp.attr,
	&dev_attr_hotplug_in_temp.attr,
	NULL,
};

static const struct attribute_group exynos_tmu_attr_group = {
	.attrs = exynos_tmu_attrs,
};

static int exynos_tmu_probe(struct platform_device *pdev)
{
	struct exynos_tmu_data *data;
	int ret;
#ifdef CONFIG_CPU_FREQ
	if (!cpufreq_frequency_get_table(0))
		return -EPROBE_DEFER;
#endif

	data = devm_kzalloc(&pdev->dev, sizeof(struct exynos_tmu_data),
					GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);
	mutex_init(&data->lock);

	ret = exynos_map_dt_data(pdev);
	if (ret)
		goto err_sensor;

	if (data->id == 0 || data->id == 1) {
		ret = exynos_cpufreq_cooling_register(data);
		if (ret) {
			dev_err(&pdev->dev, "Failed cooling register \n");
			goto err_sensor;
		}
	} else if (data->id == 2) {
		ret = gpu_cooling_table_init(pdev);
		if (ret)
			goto err_sensor;

		ret = exynos_gpufreq_cooling_register(data);
		if (ret) {
			dev_err(&pdev->dev, "Failed cooling register \n");
			goto err_sensor;
		}
	} else if (data->id == 3) {
		ret = isp_cooling_table_init(pdev);
		if (ret)
			goto err_sensor;

		ret = exynos_isp_cooling_register(data);
		if (ret) {
			dev_err(&pdev->dev, "Failed cooling register \n");
			goto err_sensor;
		}
	}

	INIT_WORK(&data->irq_work, exynos_tmu_work);

	/*
	 * data->tzd must be registered before calling exynos_tmu_initialize(),
	 * requesting irq and calling exynos_tmu_control().
	 */
	if(data->hotplug_enable)
		pm_qos_add_request(&thermal_cpu_hotplug_request,
					PM_QOS_CPU_ONLINE_MAX,
					PM_QOS_CPU_ONLINE_MAX_DEFAULT_VALUE);

	data->tzd = thermal_zone_of_sensor_register(&pdev->dev, 0, data,
						    data->hotplug_enable ?
						    &exynos_hotplug_sensor_ops :
						    &exynos_sensor_ops);
	if (IS_ERR(data->tzd)) {
		ret = PTR_ERR(data->tzd);
		dev_err(&pdev->dev, "Failed to register sensor: %d\n", ret);
		goto err_sensor;
	}

	data->num_probe = (readl(data->base + EXYNOS_TMU_REG_CONTROL1) >> EXYNOS_TMU_NUM_PROBE_SHIFT)
				& EXYNOS_TMU_NUM_PROBE_MASK;

	ret = exynos_tmu_initialize(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to initialize TMU\n");
		goto err_thermal;
	}

	ret = devm_request_irq(&pdev->dev, data->irq, exynos_tmu_irq,
				IRQF_SHARED, dev_name(&pdev->dev), data);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq: %d\n", data->irq);
		goto err_thermal;
	}

	exynos_tmu_control(pdev, true);

	ret = sysfs_create_group(&pdev->dev.kobj, &exynos_tmu_attr_group);
	if (ret)
		dev_err(&pdev->dev, "cannot create exynos tmu attr group");

	mutex_lock(&data->lock);
	list_add_tail(&data->node, &dtm_dev_list);
	mutex_unlock(&data->lock);

	if (list_is_singular(&dtm_dev_list)) {
		register_pm_notifier(&exynos_tmu_pm_notifier);
		data->nb.notifier_call = exynos_tmu_cpus_notifier;
		register_cpus_notifier(&data->nb);
		exynos_cpufreq_reset_boot_qos();
	}

	if (!IS_ERR(data->tzd))
		data->tzd->ops->set_mode(data->tzd, THERMAL_DEVICE_ENABLED);

#ifdef CONFIG_SEC_PM
	ret = sysfs_create_group(&pdev->dev.kobj, &exynos_thermal_sensor_attr_group);
	if (ret)
		dev_err(&pdev->dev, "cannot create thermal sensor attributes\n");
#endif

	return 0;

err_thermal:
	thermal_zone_of_sensor_unregister(&pdev->dev, data->tzd);
err_sensor:
	return ret;
}

static int exynos_tmu_remove(struct platform_device *pdev)
{
	struct exynos_tmu_data *data = platform_get_drvdata(pdev);
	struct thermal_zone_device *tzd = data->tzd;
	struct exynos_tmu_data *devnode;

	if (list_is_singular(&dtm_dev_list)) {
		unregister_pm_notifier(&exynos_tmu_pm_notifier);
		unregister_cpus_notifier(&data->nb);
	}

	thermal_zone_of_sensor_unregister(&pdev->dev, tzd);
	exynos_tmu_control(pdev, false);

	mutex_lock(&data->lock);
	list_for_each_entry(devnode, &dtm_dev_list, node) {
		if (devnode->id == data->id) {
			list_del(&devnode->node);
		}
	}
	mutex_unlock(&data->lock);

#ifdef CONFIG_SEC_PM
	sysfs_remove_group(&pdev->dev.kobj, &exynos_thermal_sensor_attr_group);
#endif

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int exynos_tmu_suspend(struct device *dev)
{
	exynos_tmu_control(to_platform_device(dev), false);

	return 0;
}

static int exynos_tmu_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	exynos_tmu_initialize(pdev);
	exynos_tmu_control(pdev, true);

	return 0;
}

static SIMPLE_DEV_PM_OPS(exynos_tmu_pm,
			 exynos_tmu_suspend, exynos_tmu_resume);
#define EXYNOS_TMU_PM	(&exynos_tmu_pm)
#else
#define EXYNOS_TMU_PM	NULL
#endif

static struct platform_driver exynos_tmu_driver = {
	.driver = {
		.name   = "exynos-tmu",
		.pm     = EXYNOS_TMU_PM,
		.of_match_table = exynos_tmu_match,
	},
	.probe = exynos_tmu_probe,
	.remove	= exynos_tmu_remove,
};

module_platform_driver(exynos_tmu_driver);

MODULE_DESCRIPTION("EXYNOS TMU Driver");
MODULE_AUTHOR("Donggeun Kim <dg77.kim@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:exynos-tmu");
