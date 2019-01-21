/*
 *  s2mu005_fuelgauge.c
 *  Samsung S2MU005 Fuel Gauge Driver
 *
 *  Copyright (C) 2015 Samsung Electronics
 *  Developed by Nguyen Tien Dat (tiendat.nt@samsung.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG

#define SINGLE_BYTE	1
#define TABLE_SIZE	22

#include <linux/battery/fuelgauge/s2mu005_fuelgauge.h>
#include <linux/of_gpio.h>

static enum power_supply_property s2mu005_fuelgauge_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_ENERGY_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
};

static int s2mu005_get_vbat(struct s2mu005_fuelgauge_data *fuelgauge);
static int s2mu005_get_ocv(struct s2mu005_fuelgauge_data *fuelgauge);
static int s2mu005_get_current(struct s2mu005_fuelgauge_data *fuelgauge);
static int s2mu005_get_avgcurrent(struct s2mu005_fuelgauge_data *fuelgauge);
static int s2mu005_get_avgvbat(struct s2mu005_fuelgauge_data *fuelgauge);

static int s2mu005_write_reg_byte(struct i2c_client *client, int reg, u8 data)
{
	int ret, i = 0;

	ret = i2c_smbus_write_byte_data(client, reg,  data);
	if (ret < 0) {
		for (i = 0; i < 3; i++) {
			ret = i2c_smbus_write_byte_data(client, reg,  data);
			if (ret >= 0)
				break;
		}

		if (i >= 3)
			dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	}

	return ret;
}

static int s2mu005_write_reg(struct i2c_client *client, int reg, u8 *buf)
{
#if SINGLE_BYTE
	int ret = 0 ;
	s2mu005_write_reg_byte(client, reg, buf[0]);
	s2mu005_write_reg_byte(client, reg+1, buf[1]);
#else
	int ret, i = 0;

	ret = i2c_smbus_write_i2c_block_data(client, reg, 2, buf);
	if (ret < 0) {
		for (i = 0; i < 3; i++) {
			ret = i2c_smbus_write_i2c_block_data(client, reg, 2, buf);
			if (ret >= 0)
				break;
		}

		if (i >= 3)
			dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	}
#endif
	return ret;
}

static int s2mu005_read_reg_byte(struct i2c_client *client, int reg, void *data)
{
	int ret = 0;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return ret;
	*(u8 *)data = (u8)ret;

	return ret;
}

static int s2mu005_read_reg(struct i2c_client *client, int reg, u8 *buf)
{

#if SINGLE_BYTE
	int ret =0;
	u8 data1 = 0 , data2 = 0;
	s2mu005_read_reg_byte(client, reg, &data1);
	s2mu005_read_reg_byte(client, reg+1, &data2);
	buf[0] = data1;
	buf[1] = data2;
#else
	int ret = 0, i = 0;

	ret = i2c_smbus_read_i2c_block_data(client, reg, 2, buf);
	if (ret < 0) {
		for (i = 0; i < 3; i++) {
			ret = i2c_smbus_read_i2c_block_data(client, reg, 2, buf);
			if (ret >= 0)
				break;
		}

		if (i >= 3)
			dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	}
#endif
	return ret;
}

static void s2mu005_fg_test_read(struct i2c_client *client)
{
	u8 data;
	char str[1016] = {0,};
	int i;

	/* address 0x00 ~ 0x1f */
	for (i = 0x0; i <= 0x1F; i++) {
		s2mu005_read_reg_byte(client, i, &data);
		sprintf(str+strlen(str), "0x%02x:0x%02x, ", i, data);
	}

	/* address 0x27 */
	s2mu005_read_reg_byte(client, 0x27, &data);
	sprintf(str+strlen(str),"0x27:0x%02x, ",data);

	/* address 0x44, 0x45 */
	for (i = 0x44; i <= 0x45; i++) {
		s2mu005_read_reg_byte(client, i, &data);
		sprintf(str+strlen(str), "0x%02x:0x%02x, ", i, data);
	}

	/* print buffer */
	pr_info("[FG]%s: %s\n", __func__, str);
}

static void WA_0_issue_at_init(struct s2mu005_fuelgauge_data *fuelgauge)
{
	int a = 0;
	u8 v_52 = 0, v_53 =0, temp1, temp2;
	int FG_volt, UI_volt, offset;

	/* Step 1: [Surge test]  get UI voltage (0.1mV)*/
	UI_volt = s2mu005_get_ocv(fuelgauge);

	s2mu005_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);
	msleep(50);

	/* Step 2: [Surge test] get FG voltage (0.1mV) */
	FG_volt = s2mu005_get_vbat(fuelgauge) * 10;

	/* Step 3: [Surge test] get offset */
	offset = UI_volt - FG_volt;
	pr_err("%s: UI_volt(%d), FG_volt(%d), offset(%d)\n",
			__func__, UI_volt, FG_volt, offset);

	/* Step 4: [Surge test] */
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x53, &v_53);
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x52, &v_52);
	pr_err("%s: v_53(0x%x), v_52(0x%x)\n", __func__, v_53, v_52);

	a = (v_53 & 0x0F) << 8;
	a += v_52;
	a = a << 3;
	pr_err("%s: a before add offset (0x%x)\n", __func__, a);

	a += (offset << 16) / 10000;
	pr_err("%s: a after add offset (0x%x)\n", __func__, a);

	a &= 0x7FFF;
	a = a >> 3;
	a &= 0xfff;
	pr_err("%s: (a >> 3)&0xFFF (0x%x)\n", __func__, a);

	/* modify 0x53[3:0] */
	temp1 = v_53 & 0xF0;
	temp2 = (u8)((a&0xF00) >> 8);
	temp1 |= temp2;
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x53, temp1);

	/* modify 0x52[7:0] */
	temp2 = (u8)(a & 0xFF);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x52, temp2);

	/* restart and dumpdone */
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);
	msleep(300);

	/* recovery 0x52 and 0x53 */
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x53, &temp1);
	temp1 &= 0xF0;
	temp1 |= (v_53 & 0x0F);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x53, temp1);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x52, v_52);
}

static int s2mu005_get_soc_from_ocv(struct s2mu005_fuelgauge_data *fuelgauge, int target_ocv)
{

	int *soc_arr;
	int *ocv_arr;
	int soc = 0;
	int ocv = target_ocv * 10;

	int high_index = TABLE_SIZE - 1;
	int low_index = 0;
	int mid_index = 0;

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	soc_arr = fuelgauge->age_data_info[fuelgauge->fg_age_step].soc_arr_val;
	ocv_arr = fuelgauge->age_data_info[fuelgauge->fg_age_step].ocv_arr_val;
#else
	soc_arr = fuelgauge->info.soc_arr_evt2;
	ocv_arr = fuelgauge->info.ocv_arr_evt2;
#endif
	pr_err("%s: soc_arr(%d) ocv_arr(%d)\n", __func__,*soc_arr, *ocv_arr);

	if(ocv <= ocv_arr[TABLE_SIZE - 1]) {
		soc = soc_arr[TABLE_SIZE - 1];
		goto soc_ocv_mapping;
	} else if (ocv >= ocv_arr[0]) {
		soc = soc_arr[0];
		goto soc_ocv_mapping;
	}
	while (low_index <= high_index) {
		mid_index = (low_index + high_index) >> 1;
		if(ocv_arr[mid_index] > ocv)
			low_index = mid_index + 1;
		else if(ocv_arr[mid_index] < ocv)
			high_index = mid_index - 1;
		else {
			soc = soc_arr[mid_index];
			goto soc_ocv_mapping;
		}
	}
	soc = soc_arr[high_index];
	soc += ((soc_arr[low_index] - soc_arr[high_index]) *
					(ocv - ocv_arr[high_index])) /
					(ocv_arr[low_index] - ocv_arr[high_index]);

soc_ocv_mapping:
	dev_info(&fuelgauge->i2c->dev, "%s: ocv (%d), soc (%d), EVT(%d)\n", __func__, ocv, soc, fuelgauge->revision);
	return soc;
}

static void WA_0_issue_at_init1(struct s2mu005_fuelgauge_data *fuelgauge, int target_ocv)
{
	int a = 0;
	u8 v_52 = 0, v_53 =0, temp1, temp2;
	int FG_volt, UI_volt, offset;

	/* Step 1: [Surge test]  get UI voltage (0.1mV)*/
	UI_volt = target_ocv * 10;

	s2mu005_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);
	msleep(50);

	/* Step 2: [Surge test] get FG voltage (0.1mV) */
	FG_volt = s2mu005_get_vbat(fuelgauge) * 10;

	/* Step 3: [Surge test] get offset */
	offset = UI_volt - FG_volt;
	pr_err("%s: UI_volt(%d), FG_volt(%d), offset(%d)\n",
			__func__, UI_volt, FG_volt, offset);

	/* Step 4: [Surge test] */
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x53, &v_53);
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x52, &v_52);
	pr_err("%s: v_53(0x%x), v_52(0x%x)\n", __func__, v_53, v_52);

	a = (v_53 & 0x0F) << 8;
	a += v_52;
	a = a << 3;
	pr_err("%s: a before add offset (0x%x)\n", __func__, a);

	a += (offset << 16) / 10000;
	pr_err("%s: a after add offset (0x%x)\n", __func__, a);

	a &= 0x7FFF;
	a = a >> 3;
	a &= 0xfff;
	pr_err("%s: (a >> 3)&0xFFF (0x%x)\n", __func__, a);

	/* modify 0x53[3:0] */
	temp1 = v_53 & 0xF0;
	temp2 = (u8)((a&0xF00) >> 8);
	temp1 |= temp2;
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x53, temp1);

	/* modify 0x52[7:0] */
	temp2 = (u8)(a & 0xFF);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x52, temp2);

	/* restart and dumpdone */
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);
	msleep(300);

	pr_info("%s: S2MU005 VBAT : %d\n", __func__, s2mu005_get_vbat(fuelgauge) * 10);

	/* recovery 0x52 and 0x53 */
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x53, &temp1);
	temp1 &= 0xF0;
	temp1 |= (v_53 & 0x0F);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x53, temp1);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x52, v_52);
}

static void s2mu005_reset_fg(struct s2mu005_fuelgauge_data *fuelgauge)
{
	int i;
	u8 temp = 0;

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	mutex_lock(&fuelgauge->fg_lock);
#endif
	/* step 0: [Surge test] initialize register of FG */
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x0F, fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[0]);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x0E, fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[1]);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x11, fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[2]);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x10, fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[3]);
#else
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x0F, fuelgauge->info.batcap[0]);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x0E, fuelgauge->info.batcap[1]);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x11, fuelgauge->info.batcap[2]);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x10, fuelgauge->info.batcap[3]);
#endif

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	for (i = 0x92; i <= 0xe9; i++) {
		s2mu005_write_reg_byte(fuelgauge->i2c, i, fuelgauge->age_data_info[fuelgauge->fg_age_step].battery_table3[i - 0x92]);
	}
	for (i = 0xea; i <= 0xff; i++) {
		s2mu005_write_reg_byte(fuelgauge->i2c, i, fuelgauge->age_data_info[fuelgauge->fg_age_step].battery_table4[i - 0xea]);
	}
#else
	for(i = 0x92; i <= 0xe9; i++) {
		s2mu005_write_reg_byte(fuelgauge->i2c, i, fuelgauge->info.battery_table3[i - 0x92]);
	}
	for(i = 0xea; i <= 0xff; i++) {
		s2mu005_write_reg_byte(fuelgauge->i2c, i, fuelgauge->info.battery_table4[i - 0xea]);
	}
#endif

	s2mu005_write_reg_byte(fuelgauge->i2c, 0x21, 0x13);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x14, 0x40);

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
	temp &= 0xF0;
	temp |= fuelgauge->age_data_info[fuelgauge->fg_age_step].accum[1];
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x44,  fuelgauge->age_data_info[fuelgauge->fg_age_step].accum[0]);
#else
	if(fuelgauge->revision >= 2) {
		s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
		temp &= 0xF0;
		temp |= 0x08;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0x00);
	} else {
		s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
		temp &= 0xF0;
		temp |= 0x07;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0xCC);
	}
#endif

	s2mu005_read_reg_byte(fuelgauge->i2c, 0x27, &temp);
	temp |= 0x10;
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x27, temp);

	if(fuelgauge->revision >= 2) {
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x4B, 0x0B);
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x4A, 0x10);

		s2mu005_read_reg_byte(fuelgauge->i2c, 0x03, &temp);
		temp |= 0x40;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x03, temp);
	}
	else {
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x4B, 0x09);

		s2mu005_read_reg_byte(fuelgauge->i2c, 0x27, &temp);
		temp |= 0x0F;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x27, temp);

		s2mu005_read_reg_byte(fuelgauge->i2c, 0x26, &temp);
		temp |= 0xFE;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x26, temp);

		s2mu005_write_reg_byte(fuelgauge->i2c, 0x4A, 0xFF);

		s2mu005_read_reg_byte(fuelgauge->i2c, 0x26, &temp);
		temp &= 0xFE;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x26, temp);
	}

	s2mu005_write_reg_byte(fuelgauge->i2c, 0x40, 0x04);

	WA_0_issue_at_init(fuelgauge);
#if defined(CONFIG_BATTERY_AGE_FORECAST)
	mutex_unlock(&fuelgauge->fg_lock);
#endif
	pr_err("%s: Reset FG completed\n", __func__);
}

static void s2mu005_restart_gauging(struct s2mu005_fuelgauge_data *fuelgauge)
{
	u8 temp=0, temp_REG26=0, temp_REG27=0;
	u8 data[2], r_data[2];
	pr_err("%s: Re-calculate SOC and voltage\n", __func__);

	s2mu005_read_reg_byte(fuelgauge->i2c, 0x27, &temp_REG27);
	temp=temp_REG27;
	temp |= 0x0F;
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x27, temp);

	s2mu005_read_reg_byte(fuelgauge->i2c, 0x26, &temp_REG26);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x26, 0xFF);

	s2mu005_read_reg(fuelgauge->i2c, S2MU005_REG_IRQ, data);
	pr_info("%s: irq_reg data (%02x%02x)  \n",__func__, data[1], data[0]);

	/* store data for interrupt mask */
	r_data[0] = data[0];
	r_data[1] = data[1];

	/* disable irq for unwanted interrupt */
	data[1] |= 0x0f;
	s2mu005_write_reg(fuelgauge->i2c, S2MU005_REG_IRQ, data);

	/* restart gauge */
	//s2mu005_write_reg_byte(fuelgauge->i2c, 0x1f, 0x01);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x21, 0x13);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);

	msleep(200);

	/* enable irq after reset */
	s2mu005_write_reg(fuelgauge->i2c, S2MU005_REG_IRQ, r_data);
	pr_info("%s: re-store irq_reg data (%02x%02x) \n",__func__, r_data[1], r_data[0]);

	s2mu005_write_reg_byte(fuelgauge->i2c, 0x27, temp_REG27);
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x26, temp_REG26);

	s2mu005_read_reg_byte(fuelgauge->i2c, 0x27, &temp);
	pr_info("%s: 0x27 : %02x \n", __func__,temp);
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x26, &temp);
	pr_info("%s: 0x26 : %02x \n", __func__,temp);
}

static void s2mu005_init_regs(struct s2mu005_fuelgauge_data *fuelgauge)
{
	u8 temp = 0;
	pr_err("%s: s2mu005 fuelgauge initialize\n", __func__);

	/* Reduce top-off current difference between
	 * Power on charging and Power off charging
	 */
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x27, &temp);
	temp |= 0x10;
	s2mu005_write_reg_byte(fuelgauge->i2c, 0x27, temp);

	if(fuelgauge->revision < 2) {
		/* Sampling time set 500ms */
		s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
		temp &= 0x3F;
		temp |= 0x0;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);
	}
}

static void s2mu005_alert_init(struct s2mu005_fuelgauge_data *fuelgauge)
{
	u8 data[2];

	/* VBAT Threshold setting: 3.55V */
	data[0] = 0x00 & 0x0f;

	/* SOC Threshold setting */
	data[0] = data[0] | (fuelgauge->pdata->fuel_alert_soc << 4);

	data[1] = 0x00;
	s2mu005_write_reg(fuelgauge->i2c, S2MU005_REG_IRQ_LVL, data);
}

static bool s2mu005_check_status(struct i2c_client *client)
{
	u8 data[2];
	bool ret = false;

	/* check if Smn was generated */
	if (s2mu005_read_reg(client, S2MU005_REG_STATUS, data) < 0)
		return ret;

	dev_dbg(&client->dev, "%s: status to (%02x%02x)\n",
		__func__, data[1], data[0]);

	if (data[1] & (0x1 << 1))
		return true;
	else
		return false;
}

static int s2mu005_set_temperature(struct s2mu005_fuelgauge_data *fuelgauge,
			int temperature)
{
	/*
	 * s5mu005 include temperature sensor so,
	 * do not need to set temperature value.
	 */
	return temperature;
}

static int s2mu005_temperature_compensation(struct s2mu005_fuelgauge_data *fuelgauge, int temp)
{
	int temp_comp;
	int low = 0;
	int high = 0;
	int mid = 0;

	if (fuelgauge->temp_table[0].adc >= temp) {
		temp_comp = fuelgauge->temp_table[0].data;
		goto temp_by_goto;
	} else if (fuelgauge->temp_table[fuelgauge->temp_table_size - 1].adc <= temp) {
		temp_comp = fuelgauge->temp_table[fuelgauge->temp_table_size - 1].data;
		goto temp_by_goto;
	}

	high = fuelgauge->temp_table_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (fuelgauge->temp_table[mid].adc > temp)
			high = mid - 1;
		else if (fuelgauge->temp_table[mid].adc < temp)
			low = mid + 1;
		else {
			temp_comp = fuelgauge->temp_table[mid].data;
			goto temp_by_goto;
		}
	}

	temp_comp = fuelgauge->temp_table[high].data;
	temp_comp += ((fuelgauge->temp_table[low].data - fuelgauge->temp_table[high].data) *
		 (temp - fuelgauge->temp_table[high].adc)) /
		(fuelgauge->temp_table[low].adc - fuelgauge->temp_table[high].adc);

temp_by_goto:
	pr_info("%s: Comp_Temp(%d), Temp(%d)\n",
		__func__, temp_comp, temp);

	return temp_comp;
}

static int s2mu005_get_temperature(struct s2mu005_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u16 compliment;
	int temperature = 0;
	int temp_value;

	/*
	 *  use monitor regiser.
	 *  monitor register default setting is temperature
	 */
	mutex_lock(&fuelgauge->fg_lock);

	s2mu005_write_reg_byte(fuelgauge->i2c, S2MU005_REG_MONOUT_SEL, 0x10);
	msleep(10);
	if (s2mu005_read_reg(fuelgauge->i2c, S2MU005_REG_MONOUT, data) < 0)
		goto err;
	pr_info("%s temp data = 0x%x 0x%x\n", __func__, data[0], data[1]);

	mutex_unlock(&fuelgauge->fg_lock);
	compliment = (data[1] << 8) | (data[0]);

	/* data[] store 2's compliment format number */
	if (compliment & (0x1 << 15)) {
		/* Negative */
		temperature = -1 * ((~compliment & 0xFFFF) + 1);
	} else {
		temperature = compliment & 0x7FFF;
	}
	temperature = ((temperature * 100) >> 8)/10;

	dev_info(&fuelgauge->i2c->dev, "%s: temperature (%d)\n",
		__func__, temperature);

	if(fuelgauge->temperature_compensation) {
		temp_value = s2mu005_temperature_compensation(fuelgauge, temperature);
		return temp_value;
	} else {
		return temperature;
	}
err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -ERANGE;
}

static int s2mu005_get_rawsoc(struct s2mu005_fuelgauge_data *fuelgauge)
{
	u8 data[2], check_data[2];
	u16 compliment;
	int rsoc, i;
	u8 por_state = 0;
	u8 temp = 0;
	u8 reg = S2MU005_REG_RSOC;
	int fg_reset = 0;
	union power_supply_propval value;

	int avg_current = 0, avg_vbat = 0, vbat = 0, curr = 0;
	int ocv_pwroff = 0, ocv_100 = 0;
	int target_soc = 0, soc_100 = 0;
	//bkj - rempcap logging
	int rsoc1;

	s2mu005_read_reg_byte(fuelgauge->i2c, 0x1F, &por_state);
#if defined(CONFIG_BATTERY_AGE_FORECAST)
    if((por_state & 0x10) && (fuelgauge->age_reset_status == 0))
#else
	if(por_state & 0x10) 
#endif
	{
		value.intval = 0;
		psy_do_property("s2mu005-charger", set, POWER_SUPPLY_PROP_CHARGING_ENABLED, value);

		dev_info(&fuelgauge->i2c->dev, "%s: FG reset\n", __func__);
		s2mu005_reset_fg(fuelgauge);
		por_state &= ~0x10;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x1F, por_state);

		fg_reset = 1;
	}

	mutex_lock(&fuelgauge->fg_lock);

	if(fuelgauge->revision >= 2)
		reg = S2MU005_REG_RSOC;
	else {
		if(fuelgauge->mode == CURRENT_MODE)
			reg = S2MU005_REG_RSOC;
		else {
			s2mu005_write_reg_byte(fuelgauge->i2c, 0x0C, 0x03);
			reg = S2MU005_REG_MONOUT;
		}
	}

	for (i = 0; i < 50; i++) {
		if (s2mu005_read_reg(fuelgauge->i2c, reg, data) < 0)
			goto err;
		if (s2mu005_read_reg(fuelgauge->i2c, reg, check_data) < 0)
			goto err;

		dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: data0 (%d) data1 (%d) \n", __func__, data[0], data[1]);
		if ((data[0] == check_data[0]) && (data[1] == check_data[1]))
			break;
	}

	mutex_unlock(&fuelgauge->fg_lock);

	if (fg_reset) {
		value.intval = 1;
		psy_do_property("s2mu005-charger", set, POWER_SUPPLY_PROP_CHARGE_ENABLED, value);
		psy_do_property("s2mu005-charger", set, POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
	}

	compliment = (data[1] << 8) | (data[0]);

	/* data[] store 2's compliment format number */
	if (compliment & (0x1 << 15)) {
		/* Negative */
		rsoc = ((~compliment) & 0xFFFF) + 1;
		rsoc = (rsoc * (-10000)) / (0x1 << 14);
	} else {
		rsoc = compliment & 0x7FFF;
		rsoc = ((rsoc * 10000) / (0x1 << 14));
	}

	if (fg_reset)
		fuelgauge->diff_soc = fuelgauge->info.soc - rsoc;

	dev_info(&fuelgauge->i2c->dev, "%s: current_soc (%d), previous soc (%d), diff (%d), FG_mode(%d)\n",
		 __func__, rsoc, fuelgauge->info.soc, fuelgauge->diff_soc, fuelgauge->mode);

	fuelgauge->info.soc = rsoc + fuelgauge->diff_soc;

	if(fuelgauge->revision >= 2) {
		if(fuelgauge->info.soc <= 300) {
			if(fuelgauge->mode == CURRENT_MODE) { /* switch to VOLTAGE_MODE */
				fuelgauge->mode = LOW_SOC_VOLTAGE_MODE;

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x4A, 0xFF);

				dev_info(&fuelgauge->i2c->dev, "%s: FG is in low soc voltage mode\n", __func__);
			}
		}
		else if (fuelgauge->info.soc > 325) {
			if(fuelgauge->mode == LOW_SOC_VOLTAGE_MODE) {
				fuelgauge->mode = CURRENT_MODE;

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x4A, 0x10);

				dev_info(&fuelgauge->i2c->dev, "%s: FG is in current mode\n", __func__);
			}
		}

		psy_do_property("battery", get, POWER_SUPPLY_PROP_CAPACITY, value);
		dev_info(&fuelgauge->i2c->dev, "%s: UI SOC = %d\n", __func__, value.intval);

		if (value.intval >= 98) {
			if(fuelgauge->mode == CURRENT_MODE) { /* switch to VOLTAGE_MODE */
				fuelgauge->mode = HIGH_SOC_VOLTAGE_MODE;

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x4A, 0xFF);

				dev_info(&fuelgauge->i2c->dev, "%s: FG is in high soc voltage mode\n", __func__);
			}
		}
		else if (value.intval < 97) {
			if(fuelgauge->mode == HIGH_SOC_VOLTAGE_MODE) {
				fuelgauge->mode = CURRENT_MODE;

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x4A, 0x10);

				dev_info(&fuelgauge->i2c->dev, "%s: FG is in current mode\n", __func__);
			}
		}
	}
	else {
		if(!fuelgauge->is_charging && fuelgauge->info.soc <= 300) {
			if(fuelgauge->mode == CURRENT_MODE) { /* switch to VOLTAGE_MODE */

				fuelgauge->mode = LOW_SOC_VOLTAGE_MODE;
				value.intval = fuelgauge->mode;
				psy_do_property("s2mu005-charger", set, POWER_SUPPLY_PROP_SCOPE, value);

				s2mu005_read_reg_byte(fuelgauge->i2c, 0x26, &temp);
				temp |= 0x01;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x26, temp);

				s2mu005_read_reg_byte(fuelgauge->i2c, 0x4B, &temp);
				temp |= 0x02;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x4B, temp);

				s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
				temp &= 0xF0;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0x00);
				fuelgauge->vm_soc = fuelgauge->info.soc;

				dev_info(&fuelgauge->i2c->dev, "%s: FG is in low soc voltage mode: %d\n",
					__func__, fuelgauge->vm_soc);
			}
		} else if (fuelgauge->is_charging && fuelgauge->info.soc >= fuelgauge->vm_soc) {
			if(fuelgauge->mode == LOW_SOC_VOLTAGE_MODE) {

				fuelgauge->mode = CURRENT_MODE;
				value.intval = fuelgauge->mode;
				psy_do_property("s2mu005-charger", set, POWER_SUPPLY_PROP_SCOPE, value);

				s2mu005_read_reg_byte(fuelgauge->i2c, 0x4B, &temp);
				temp &= ~0x02;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x4B, temp);

				s2mu005_read_reg_byte(fuelgauge->i2c, 0x26, &temp);
				temp &= ~0x01;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x26, temp);

				s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
				temp &= 0xF0;
				temp |= 0x07;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0xCC);
				dev_info(&fuelgauge->i2c->dev, "%s: FG is in current mode\n", __func__);
			}
		}
	}

	avg_current = s2mu005_get_avgcurrent(fuelgauge);
	avg_vbat =  s2mu005_get_avgvbat(fuelgauge);
	vbat = s2mu005_get_vbat(fuelgauge);
	curr = s2mu005_get_current(fuelgauge);

	if (!fuelgauge->is_charging && avg_vbat <= 3300) {
		if (fuelgauge->mode == CURRENT_MODE) {
			if (abs(avg_vbat - vbat) <= 20 && abs(avg_current - curr) <= 30) {
				ocv_pwroff = avg_vbat - avg_current * 15 / 100;
				target_soc = s2mu005_get_soc_from_ocv(fuelgauge, ocv_pwroff);
				if (abs(target_soc - fuelgauge->info.soc) > 300) {
					pr_info("%s : F/G reset Start\n", __func__);
					WA_0_issue_at_init1(fuelgauge, ocv_pwroff);

				}
			}
		} else {
			if (abs(avg_vbat - vbat) <= 20) {
				ocv_pwroff = avg_vbat;
				target_soc = s2mu005_get_soc_from_ocv(fuelgauge, ocv_pwroff);
				if (abs(target_soc - fuelgauge->info.soc) > 300) {
					pr_info("%s : F/G reset Start\n", __func__);
					WA_0_issue_at_init1(fuelgauge, ocv_pwroff);
				}
			}
		}
	}

	if(fuelgauge->revision < 2) {
			/* -------------- for enable/disable Current Sensing -------------- */
			if(fuelgauge->mode == CURRENT_MODE) {
				ocv_100 = avg_vbat - avg_current * 15 / 100;
				soc_100 = s2mu005_get_soc_from_ocv(fuelgauge, ocv_100);

			if (fuelgauge->is_charging && avg_current > 0 && fuelgauge->info.soc >= 10000 && fuelgauge->cc_on == true) {
				fuelgauge->cc_on = false;
				s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
				temp &= 0xF0;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0x00);
				dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: stop CC, ocv_100: (%d), soc_100: (%d)\n", __func__, ocv_100, soc_100);
			} else if((!fuelgauge->is_charging || (fuelgauge->is_charging && avg_current < 0))
				&& (soc_100 < 10000) && fuelgauge->cc_on == false) {
				fuelgauge->cc_on = true;
				s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
				temp &= 0xF0;
				temp |= 0x07;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0xCC);
				dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: start CC, ocv_100: (%d), soc_100: (%d)\n", __func__, ocv_100, soc_100);
			}
		}
		/* -------------- for enable/disable Current Sensing -------------- */

		/* For debugging */
		s2mu005_read_reg_byte(fuelgauge->i2c, 0x44, &temp);
		pr_info("%s: Reg 0x44 : 0x%x\n", __func__, temp);
		s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
		pr_info("%s: Reg 0x45 : 0x%x\n", __func__, temp);

		//bkj - rempcap logging
		/* ------ read remaining capacity -------- */
		if (fuelgauge->mode == CURRENT_MODE)
		{
			mutex_lock(&fuelgauge->fg_lock);

			s2mu005_read_reg_byte(fuelgauge->i2c, 0x0C, &temp);
			s2mu005_write_reg_byte(fuelgauge->i2c, 0x0C, 0x2A);

			for (i = 0; i < 50; i++) {
				if (s2mu005_read_reg(fuelgauge->i2c, S2MU005_REG_MONOUT, data) < 0)
					goto err;
				if (s2mu005_read_reg(fuelgauge->i2c, S2MU005_REG_MONOUT, check_data) < 0)
					goto err;

				dev_dbg(&fuelgauge->i2c->dev, "[DEBUG]%s: remaining capacity data0 (%d) data1 (%d)\n", __func__, data[0], data[1]);
				if ((data[0] == check_data[0]) && (data[1] == check_data[1]))
					break;
			}
			s2mu005_write_reg_byte(fuelgauge->i2c, 0x0C, temp);

			mutex_unlock(&fuelgauge->fg_lock);

			compliment = (data[1] << 8) | (data[0]);

			/* data[] store 2's compliment format number */
			if (compliment & (0x1 << 15)) {
				/* Negative */
				rsoc1 = ((~compliment) & 0xFFFF) + 1;
				rsoc1 = (rsoc1 * (-1)) / (0x1 << 1);
			} else {
				rsoc1 = compliment & 0x7FFF;
				rsoc1 = ((rsoc1 * 1) / (0x1 << 1));
			}

			pr_info("%s: remcap (%d) \n", __func__, rsoc1);
		}
		/* ------ read remaining capacity -------- */
	}

	/* S2MU005 FG debug */
	if(fuelgauge->pdata->fg_log_enable)
		s2mu005_fg_test_read(fuelgauge->i2c);

	return min(fuelgauge->info.soc, 10000);

err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -EINVAL;
}

static int s2mu005_get_current(struct s2mu005_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u16 compliment;
	int curr = 0;

	if (s2mu005_read_reg(fuelgauge->i2c, S2MU005_REG_RCUR_CC, data) < 0)
		return -EINVAL;
	compliment = (data[1] << 8) | (data[0]);
	dev_dbg(&fuelgauge->i2c->dev, "%s: rCUR_CC(0x%4x)\n", __func__, compliment);

	if (compliment & (0x1 << 15)) { /* Charging */
		curr = ((~compliment) & 0xFFFF) + 1;
		curr = (curr * 1000) >> 12;
	} else { /* dischaging */
		curr = compliment & 0x7FFF;
		curr = (curr * (-1000)) >> 12;
	}

	dev_info(&fuelgauge->i2c->dev, "%s: current (%d)mA\n", __func__, curr);

	return curr;
}

#define TABLE_SIZE	22
static int s2mu005_get_ocv(struct s2mu005_fuelgauge_data *fuelgauge)
{
	int *soc_arr;
	int *ocv_arr;

	int soc = fuelgauge->info.soc;
	int ocv = 0;

	int high_index = TABLE_SIZE - 1;
	int low_index = 0;
	int mid_index = 0;

#if defined(CONFIG_BATTERY_AGE_FORECAST)
	soc_arr = fuelgauge->age_data_info[fuelgauge->fg_age_step].soc_arr_val;
	ocv_arr = fuelgauge->age_data_info[fuelgauge->fg_age_step].ocv_arr_val;
#else
	soc_arr = fuelgauge->info.soc_arr_evt2;
	ocv_arr = fuelgauge->info.ocv_arr_evt2;
#endif
	dev_err(&fuelgauge->i2c->dev,
		"%s: soc (%d) soc_arr[TABLE_SIZE-1] (%d) ocv_arr[TABLE_SIZE-1) (%d)\n",
		__func__, soc, soc_arr[TABLE_SIZE-1] , ocv_arr[TABLE_SIZE-1]);

	if(soc <= soc_arr[TABLE_SIZE - 1]) {
		ocv = ocv_arr[TABLE_SIZE - 1];
		goto ocv_soc_mapping;
	} else if (soc >= soc_arr[0]) {
		ocv = ocv_arr[0];
		goto ocv_soc_mapping;
	}
	while (low_index <= high_index) {
		mid_index = (low_index + high_index) >> 1;
		if(soc_arr[mid_index] > soc)
			low_index = mid_index + 1;
		else if(soc_arr[mid_index] < soc)
			high_index = mid_index - 1;
		else {
			ocv = ocv_arr[mid_index];
			goto ocv_soc_mapping;
		}
	}
	ocv = ocv_arr[high_index];
	ocv += ((ocv_arr[low_index] - ocv_arr[high_index]) *
					(soc - soc_arr[high_index])) /
					(soc_arr[low_index] - soc_arr[high_index]);

ocv_soc_mapping:
	dev_info(&fuelgauge->i2c->dev, "%s: soc (%d), ocv (%d), EVT(%d)\n", __func__, soc, ocv, fuelgauge->revision);
	return ocv;
}

static int s2mu005_get_avgcurrent(struct s2mu005_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u16 compliment;
	int curr = 0;

	mutex_lock(&fuelgauge->fg_lock);

	s2mu005_write_reg_byte(fuelgauge->i2c, S2MU005_REG_MONOUT_SEL, 0x26);

	if (s2mu005_read_reg(fuelgauge->i2c, S2MU005_REG_MONOUT, data) < 0)
		goto err;
	compliment = (data[1] << 8) | (data[0]);
	dev_dbg(&fuelgauge->i2c->dev, "%s: MONOUT(0x%4x)\n", __func__, compliment);

	if (compliment & (0x1 << 15)) { /* Charging */
		curr = ((~compliment) & 0xFFFF) + 1;
		curr = (curr * 1000) >> 12;
	} else { /* dischaging */
		curr = compliment & 0x7FFF;
		curr = (curr * (-1000)) >> 12;
	}
	s2mu005_write_reg_byte(fuelgauge->i2c, S2MU005_REG_MONOUT_SEL, 0x10);

	mutex_unlock(&fuelgauge->fg_lock);

	dev_info(&fuelgauge->i2c->dev, "%s: avg current (%d)mA\n", __func__, curr);

	dev_info(&fuelgauge->i2c->dev, "%s: SOC(%d)\n", __func__, fuelgauge->info.soc);
	if ((fuelgauge->info.soc < 100) && (curr < 0) &&
	    fuelgauge->is_charging) {
		curr = 1;
		dev_info(&fuelgauge->i2c->dev, "%s: modified avg current (%d)mA\n", __func__, curr);
	}

	return curr;

err:
	mutex_unlock(&fuelgauge->fg_lock);
	return -EINVAL;
}

static int s2mu005_get_vbat(struct s2mu005_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u32 vbat = 0;

	if (s2mu005_read_reg(fuelgauge->i2c, S2MU005_REG_RVBAT, data) < 0)
		return -EINVAL;

	dev_dbg(&fuelgauge->i2c->dev, "%s: data0 (%d) data1 (%d) \n", __func__, data[0], data[1]);
	vbat = ((data[0] + (data[1] << 8)) * 1000) >> 13;

	dev_info(&fuelgauge->i2c->dev, "%s: vbat (%d)\n", __func__, vbat);

	return vbat;
}

static int s2mu005_get_avgvbat(struct s2mu005_fuelgauge_data *fuelgauge)
{
	u8 data[2];
	u32 new_vbat, old_vbat = 0;
	int cnt;

	for (cnt = 0; cnt < 5; cnt++) {
		if (s2mu005_read_reg(fuelgauge->i2c, S2MU005_REG_RVBAT, data) < 0)
			return -EINVAL;

		new_vbat = ((data[0] + (data[1] << 8)) * 1000) >> 13;

		if (cnt == 0)
			old_vbat = new_vbat;
		else
			old_vbat = new_vbat / 2 + old_vbat / 2;
	}

	dev_info(&fuelgauge->i2c->dev, "%s: avgvbat (%d)\n", __func__, old_vbat);

	return old_vbat;
}

/* if ret < 0, discharge */
static int sec_bat_check_discharge(int vcell)
{
	static int cnt;
	static int pre_vcell = 0;

	if (pre_vcell == 0)
		pre_vcell = vcell;
	else if (pre_vcell > vcell)
		cnt++;
	else if (vcell >= 3400)
		cnt = 0;
	else
		cnt--;

	pre_vcell = vcell;

	if (cnt >= 3)
		return -1;
	else
		return 1;
}

/* judge power off or not by current_avg */
static int s2mu005_get_current_average(struct s2mu005_fuelgauge_data *fuelgauge)
{
	union power_supply_propval value_bat;
	int vcell, soc, curr_avg;
	int check_discharge;

	psy_do_property("battery", get,
			POWER_SUPPLY_PROP_HEALTH, value_bat);
	vcell = s2mu005_get_vbat(fuelgauge);
	soc = s2mu005_get_rawsoc(fuelgauge) / 100;
	check_discharge = sec_bat_check_discharge(vcell);

	/* if 0% && under 3.4v && low power charging(1000mA), power off */
	//if (!lpcharge && (soc <= 0) && (vcell < 3400) &&
	if ((soc <= 0) && (vcell < 3400) &&
			((check_discharge < 0) ||
			 ((value_bat.intval == POWER_SUPPLY_HEALTH_OVERHEAT) ||
			  (value_bat.intval == POWER_SUPPLY_HEALTH_COLD)))) {
		pr_info("%s: SOC(%d), Vnow(%d) \n",
				__func__, soc, vcell);
		curr_avg = -1;
	} else {
		curr_avg = 0;
	}

	return curr_avg;
}

/* capacity is  0.1% unit */
static void s2mu005_fg_get_scaled_capacity(
		struct s2mu005_fuelgauge_data *fuelgauge,
		union power_supply_propval *val)
{
	val->intval = (val->intval < fuelgauge->pdata->capacity_min) ?
		0 : ((val->intval - fuelgauge->pdata->capacity_min) * 1000 /
		(fuelgauge->capacity_max - fuelgauge->pdata->capacity_min));

	dev_info(&fuelgauge->i2c->dev,
			"%s: scaled capacity (%d.%d)\n",
			__func__, val->intval/10, val->intval%10);
}

/* capacity is integer */
static void s2mu005_fg_get_atomic_capacity(
		struct s2mu005_fuelgauge_data *fuelgauge,
		union power_supply_propval *val)
{
	if (fuelgauge->pdata->capacity_calculation_type &
			SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC) {
		if (fuelgauge->capacity_old < val->intval)
			val->intval = fuelgauge->capacity_old + 1;
		else if (fuelgauge->capacity_old > val->intval)
			val->intval = fuelgauge->capacity_old - 1;
	}

	/* keep SOC stable in abnormal status */
	if (fuelgauge->pdata->capacity_calculation_type &
			SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL) {
		if (!fuelgauge->is_charging &&
				fuelgauge->capacity_old < val->intval) {
			dev_err(&fuelgauge->i2c->dev,
					"%s: capacity (old %d : new %d)\n",
					__func__, fuelgauge->capacity_old, val->intval);
			val->intval = fuelgauge->capacity_old;
		}
	}

	/* updated old capacity */
	fuelgauge->capacity_old = val->intval;
}

static int s2mu005_fg_check_capacity_max(
		struct s2mu005_fuelgauge_data *fuelgauge, int capacity_max)
{
	int new_capacity_max = capacity_max;

	if (new_capacity_max < (fuelgauge->pdata->capacity_max -
				fuelgauge->pdata->capacity_max_margin - 10)) {
		new_capacity_max =
			(fuelgauge->pdata->capacity_max -
			 fuelgauge->pdata->capacity_max_margin);

		dev_info(&fuelgauge->i2c->dev, "%s: set capacity max(%d --> %d)\n",
				__func__, capacity_max, new_capacity_max);
	} else if (new_capacity_max > (fuelgauge->pdata->capacity_max +
				fuelgauge->pdata->capacity_max_margin)) {
		new_capacity_max =
			(fuelgauge->pdata->capacity_max +
			 fuelgauge->pdata->capacity_max_margin);

		dev_info(&fuelgauge->i2c->dev, "%s: set capacity max(%d --> %d)\n",
				__func__, capacity_max, new_capacity_max);
	}

	return new_capacity_max;
}

static int s2mu005_fg_calculate_dynamic_scale(
		struct s2mu005_fuelgauge_data *fuelgauge, int capacity)
{
	union power_supply_propval raw_soc_val;
	raw_soc_val.intval = s2mu005_get_rawsoc(fuelgauge) / 10;

	if (raw_soc_val.intval <
			fuelgauge->pdata->capacity_max -
			fuelgauge->pdata->capacity_max_margin) {
		pr_info("%s: raw soc(%d) is very low, skip routine\n",
			__func__, raw_soc_val.intval);
		return fuelgauge->capacity_max;
	} else {
		fuelgauge->capacity_max =
			(raw_soc_val.intval >
			 fuelgauge->pdata->capacity_max +
			 fuelgauge->pdata->capacity_max_margin) ?
			(fuelgauge->pdata->capacity_max +
			 fuelgauge->pdata->capacity_max_margin) :
			raw_soc_val.intval;
		dev_dbg(&fuelgauge->i2c->dev, "%s: raw soc (%d)",
				__func__, fuelgauge->capacity_max);
	}

	fuelgauge->capacity_max =
		(fuelgauge->capacity_max * 100 / (capacity + 1));

	/* update capacity_old for sec_fg_get_atomic_capacity algorithm */
	fuelgauge->capacity_old = capacity;

	dev_info(&fuelgauge->i2c->dev, "%s: %d is used for capacity_max\n",
			__func__, fuelgauge->capacity_max);

	return fuelgauge->capacity_max;
}

bool s2mu005_fuelgauge_fuelalert_init(struct i2c_client *client, int soc)
{
	struct s2mu005_fuelgauge_data *fuelgauge = i2c_get_clientdata(client);
	u8 data[2];

	fuelgauge->is_fuel_alerted = false;

	/* 1. Set s2mu005 alert configuration. */
	s2mu005_alert_init(fuelgauge);

	if (s2mu005_read_reg(client, S2MU005_REG_IRQ, data) < 0)
		return -1;

	/*Enable VBAT, SOC */
	data[1] &= 0xfc;

	/*Disable IDLE_ST, INIT)ST */
	data[1] |= 0x0c;

	s2mu005_write_reg(client, S2MU005_REG_IRQ, data);

	dev_dbg(&client->dev, "%s: irq_reg(%02x%02x) irq(%d)\n",
			__func__, data[1], data[0], fuelgauge->pdata->fg_irq);

	return true;
}

bool s2mu005_fuelgauge_is_fuelalerted(struct s2mu005_fuelgauge_data *fuelgauge)
{
	return s2mu005_check_status(fuelgauge->i2c);
}

bool s2mu005_hal_fg_fuelalert_process(void *irq_data, bool is_fuel_alerted)
{
	struct s2mu005_fuelgauge_data *fuelgauge = irq_data;
	int ret;

	ret = i2c_smbus_write_byte_data(fuelgauge->i2c, S2MU005_REG_IRQ, 0x00);
	if (ret < 0)
		dev_err(&fuelgauge->i2c->dev, "%s: Error(%d)\n", __func__, ret);

	return ret;
}

bool s2mu005_hal_fg_full_charged(struct i2c_client *client)
{
	return true;
}

#if defined(CONFIG_BATTERY_AGE_FORECAST)
static int s2mu005_fg_aging_check(
		struct s2mu005_fuelgauge_data *fuelgauge, int step)
{
	u8 batcap0, batcap1, batcap2, batcap3;
	u8 por_state = 0;
	union power_supply_propval value;
	int charging_enabled = false;

	fuelgauge->fg_age_step = step;

	s2mu005_read_reg_byte(fuelgauge->i2c, 0x0F, &batcap0);
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x0E, &batcap1);
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x11, &batcap2);
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x10, &batcap3);

	pr_info("%s: [Long life] orig. batcap : %02x, %02x, %02x, %02x , fg_age_step data : %02x, %02x, %02x, %02x \n",
		__func__, batcap0, batcap1, batcap2, batcap3,
		fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[0],
		fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[1],
		fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[2],
		fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[3]);

	if ((batcap0 != fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[0]) ||
		(batcap1 != fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[1]) ||
		(batcap2 != fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[2]) ||
		(batcap3 != fuelgauge->age_data_info[fuelgauge->fg_age_step].batcap[3])) {

		pr_info("%s: [Long life] reset gauge for age forcast , step[%d] \n", __func__, fuelgauge->fg_age_step);

		fuelgauge->age_reset_status = 1;
		por_state |= 0x10;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x1F, por_state);

		/* check charging enable */
		psy_do_property("s2mu005-charger", get, POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		charging_enabled = value.intval;

		if (charging_enabled == true) {
			pr_info("%s: [Long life] disable charger for reset gauge age forcast \n", __func__);
			value.intval = SEC_BAT_CHG_MODE_CHARGING_OFF;
			psy_do_property("s2mu005-charger", set, POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
		}

		s2mu005_reset_fg(fuelgauge);

		if (charging_enabled == true) {
			psy_do_property("battery", get, POWER_SUPPLY_PROP_STATUS, value);
			charging_enabled = value.intval;

			if (charging_enabled == 1) { /* POWER_SUPPLY_STATUS_CHARGING 1 */
				pr_info("%s: [Long life] enable charger for reset gauge age forcast \n", __func__);
				value.intval = SEC_BAT_CHG_MODE_CHARGING;
				psy_do_property("s2mu005-charger", set, POWER_SUPPLY_PROP_CHARGING_ENABLED, value);
			}
		}

		por_state &= ~0x10;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x1F, por_state);
		fuelgauge->age_reset_status = 0;

		return 1;
	}
	return 0;
}
#endif

static int s2mu005_fg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct s2mu005_fuelgauge_data *fuelgauge =
		container_of(psy, struct s2mu005_fuelgauge_data, psy_fg);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		return -ENODATA;
		/* Cell voltage (VCELL, mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = s2mu005_get_vbat(fuelgauge);
		break;
		/* Additional Voltage Information (mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		switch (val->intval) {
			case SEC_BATTERY_VOLTAGE_AVERAGE:
				val->intval = s2mu005_get_avgvbat(fuelgauge);
				break;
			case SEC_BATTERY_VOLTAGE_OCV:
				val->intval = s2mu005_get_ocv(fuelgauge);
				break;
		}
		break;
		/* Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = s2mu005_get_current(fuelgauge);
		break;
		/* Average Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		if (fuelgauge->mode && fuelgauge->info.soc < 100)
			val->intval = s2mu005_get_current_average(fuelgauge);
		else
			val->intval = s2mu005_get_avgcurrent(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RAW) {
			val->intval = s2mu005_get_rawsoc(fuelgauge);
		} else {
			val->intval = s2mu005_get_rawsoc(fuelgauge) / 10;

			if (fuelgauge->pdata->capacity_calculation_type &
				(SEC_FUELGAUGE_CAPACITY_TYPE_SCALE |
					SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE))
				s2mu005_fg_get_scaled_capacity(fuelgauge, val);

			/* capacity should be between 0% and 100%
			 * (0.1% degree)
			 */
			if (val->intval > 1000)
				val->intval = 1000;
			if (val->intval < 0)
				val->intval = 0;

			/* get only integer part */
			val->intval /= 10;

			/* check whether doing the wake_unlock */
			if ((val->intval > fuelgauge->pdata->fuel_alert_soc) &&
					fuelgauge->is_fuel_alerted) {
				wake_unlock(&fuelgauge->fuel_alert_wake_lock);
				s2mu005_fuelgauge_fuelalert_init(fuelgauge->i2c,
						fuelgauge->pdata->fuel_alert_soc);
			}

			/* (Only for atomic capacity)
			 * In initial time, capacity_old is 0.
			 * and in resume from sleep,
			 * capacity_old is too different from actual soc.
			 * should update capacity_old
			 * by val->intval in booting or resume.
			 */
			if (fuelgauge->initial_update_of_soc) {
				/* updated old capacity */
				fuelgauge->capacity_old = val->intval;
				fuelgauge->initial_update_of_soc = false;
				break;
			}
			
			if (fuelgauge->sleep_initial_update_of_soc) {
				/* updated old capacity in case of resume */
				if(fuelgauge->is_charging ||
					((!fuelgauge->is_charging) && (fuelgauge->capacity_old >= val->intval))) {
					fuelgauge->capacity_old = val->intval;
					fuelgauge->sleep_initial_update_of_soc = false;
					break;
				}
			}

			if (fuelgauge->pdata->capacity_calculation_type &
				(SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC |
					 SEC_FUELGAUGE_CAPACITY_TYPE_SKIP_ABNORMAL))
				s2mu005_fg_get_atomic_capacity(fuelgauge, val);
		}

		break;
	/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
	/* Target Temperature */
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = s2mu005_get_temperature(fuelgauge);
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = fuelgauge->capacity_max;
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = fuelgauge->mode;
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
	{
		u8 temp;
		int pre_volt;

		pre_volt = s2mu005_get_vbat(fuelgauge) / 10;

		s2mu005_read_reg_byte(fuelgauge->i2c, 0x25, &temp);
		temp &= 0xCF;
		temp |= 0x10;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x25, temp);

		s2mu005_write_reg_byte(fuelgauge->i2c, 0x1E, 0x0F);
		msleep(300);

		val->intval = s2mu005_get_vbat(fuelgauge) / 10;

		pr_info("%s : !!!!!! PRE VOLT(%d) || VOLT(%d) !!!!!!!!\n", __func__, pre_volt, val->intval);

		break;
	}
	default:
		return -EINVAL;
	}

	return 0;
}

static int s2mu005_fg_set_property(struct power_supply *psy,
                            enum power_supply_property psp,
                            const union power_supply_propval *val)
{
	struct s2mu005_fuelgauge_data *fuelgauge =
		container_of(psy, struct s2mu005_fuelgauge_data, psy_fg);

	switch (psp) {
		case POWER_SUPPLY_PROP_STATUS:
			break;
		case POWER_SUPPLY_PROP_CHARGE_FULL:
			if (fuelgauge->pdata->capacity_calculation_type &
					SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE) {
#if defined(CONFIG_PREVENT_SOC_JUMP)
				s2mu005_fg_calculate_dynamic_scale(fuelgauge, val->intval);
#else
				s2mu005_fg_calculate_dynamic_scale(fuelgauge, 100);
#endif
			}
			break;
		case POWER_SUPPLY_PROP_ONLINE:
			fuelgauge->cable_type = val->intval;
			if (val->intval == POWER_SUPPLY_TYPE_BATTERY)
				fuelgauge->is_charging = false;
			else
				fuelgauge->is_charging = true;
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
			if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RESET) {
				fuelgauge->initial_update_of_soc = true;
				s2mu005_restart_gauging(fuelgauge);
			}
			break;
		case POWER_SUPPLY_PROP_TEMP:
		case POWER_SUPPLY_PROP_TEMP_AMBIENT:
			s2mu005_set_temperature(fuelgauge, val->intval);
			break;
		case POWER_SUPPLY_PROP_ENERGY_NOW:
#if 0
		{
			u8 temp = 0;
			s2mu005_read_reg_byte(fuelgauge->i2c, 0x27, &temp);
			if(val->intval) {
				temp |= 0x80;
			} else {
				temp &= ~0x80;
			}
			s2mu005_write_reg_byte(fuelgauge->i2c, 0x27, temp);
		}
#endif
			break;
		case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
			dev_info(&fuelgauge->i2c->dev,
				"%s: capacity_max changed, %d -> %d\n",
				__func__, fuelgauge->capacity_max, val->intval);
			fuelgauge->capacity_max = s2mu005_fg_check_capacity_max(fuelgauge, val->intval);
			fuelgauge->initial_update_of_soc = true;
			break;
		case POWER_SUPPLY_PROP_CHARGE_TYPE:
			/* rt5033_fg_reset_capacity_by_jig_connection(fuelgauge->i2c); */
			break;
		case POWER_SUPPLY_PROP_CHARGE_EMPTY:
			pr_info("%s: WA for battery 0 percent\n", __func__);
			s2mu005_write_reg_byte(fuelgauge->i2c, 0x1F, 0x01);
			break;
		case POWER_SUPPLY_PROP_ENERGY_AVG:
			pr_info("%s: WA for power off issue: val(%d)\n", __func__, val->intval);
			if(val->intval)
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x41, 0x10); /* charger start */
			else
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x41, 0x04); /* charger end */
			break;
		case POWER_SUPPLY_PROP_MAX ... POWER_SUPPLY_EXT_PROP_MAX:
			{
			enum power_supply_ext_property ext_psp = psp;
			switch (ext_psp) {			
#if defined(CONFIG_BATTERY_AGE_FORECAST)
			case POWER_SUPPLY_EXT_PROP_UPDATE_BATTERY_DATA:
				s2mu005_fg_aging_check(fuelgauge, val->intval);
				break;
#endif
			default:
				return -EINVAL;
			}
			break;
			}
		default:
			return -EINVAL;
	}

	return 0;
}

static void s2mu005_fg_isr_work(struct work_struct *work)
{
	struct s2mu005_fuelgauge_data *fuelgauge =
		container_of(work, struct s2mu005_fuelgauge_data, isr_work.work);
	u8 fg_alert_status = 0;

	s2mu005_read_reg_byte(fuelgauge->i2c, S2MU005_REG_STATUS, &fg_alert_status);
	dev_info(&fuelgauge->i2c->dev, "%s : fg_alert_status(0x%x)\n",
		__func__, fg_alert_status);

	fg_alert_status &= 0x03;
	if (fg_alert_status & 0x01) {
		pr_info("%s : Battery Level is very Low!\n", __func__);
	}

	if (fg_alert_status & 0x02) {
		pr_info("%s : Battery Voltage is Very Low!\n", __func__);
	}

	if (!fg_alert_status) {
		fuelgauge->is_fuel_alerted = false;
		pr_info("%s : SOC or Volage is Good!\n", __func__);
		wake_unlock(&fuelgauge->fuel_alert_wake_lock);
	}
}

static irqreturn_t s2mu005_fg_irq_thread(int irq, void *irq_data)
{
	struct s2mu005_fuelgauge_data *fuelgauge = irq_data;
	u8 fg_irq = 0;

	s2mu005_read_reg_byte(fuelgauge->i2c, S2MU005_REG_IRQ, &fg_irq);
	dev_info(&fuelgauge->i2c->dev, "%s: fg_irq(0x%x)\n",
		__func__, fg_irq);

	if (fuelgauge->is_fuel_alerted) {
		return IRQ_HANDLED;
	} else {
		wake_lock(&fuelgauge->fuel_alert_wake_lock);
		fuelgauge->is_fuel_alerted = true;
		schedule_delayed_work(&fuelgauge->isr_work, 0);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
static int s2mu005_fuelgauge_parse_dt(struct s2mu005_fuelgauge_data *fuelgauge)
{
	struct device_node *np = of_find_node_by_name(NULL, "s2mu005-fuelgauge");
	int ret;
	int i, len;
	const u32 *p;

	/* reset, irq gpio info */
	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
	} else {
		fuelgauge->pdata->fg_irq = of_get_named_gpio(np, "fuelgauge,fuel_int", 0);
		if (fuelgauge->pdata->fg_irq < 0)
			pr_err("%s error reading fg_irq = %d\n",
				__func__, fuelgauge->pdata->fg_irq);

		ret = of_property_read_u32(np, "fuelgauge,capacity_max",
				&fuelgauge->pdata->capacity_max);
		if (ret < 0)
			pr_err("%s error reading capacity_max %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_max_margin",
				&fuelgauge->pdata->capacity_max_margin);
		if (ret < 0)
			pr_err("%s error reading capacity_max_margin %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_min",
				&fuelgauge->pdata->capacity_min);
		if (ret < 0)
			pr_err("%s error reading capacity_min %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,capacity_calculation_type",
				&fuelgauge->pdata->capacity_calculation_type);
		if (ret < 0)
			pr_err("%s error reading capacity_calculation_type %d\n",
					__func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,fg_log_enable",
				&fuelgauge->pdata->fg_log_enable);
		if (ret < 0)
			pr_err("%s fg_log_disabled %d\n", __func__, ret);

		ret = of_property_read_u32(np, "fuelgauge,fuel_alert_soc",
				&fuelgauge->pdata->fuel_alert_soc);
		if (ret < 0)
			pr_err("%s error reading pdata->fuel_alert_soc %d\n",
					__func__, ret);
		fuelgauge->pdata->repeated_fuelalert = of_property_read_bool(np,
				"fuelgauge,repeated_fuelalert");

		fuelgauge->temperature_compensation = of_property_read_bool(np,
				"fuelgauge,temperature_compensation");

		if (fuelgauge->temperature_compensation) {
			p = of_get_property(np, "fuelgauge,temp_table", &len);
			if (!p)
				return 1;

			len = len / sizeof(u32);
			fuelgauge->temp_table_size = len;

			fuelgauge->temp_table =
				kzalloc(sizeof(sec_bat_adc_table_data_t) * fuelgauge->temp_table_size, GFP_KERNEL);

			for(i = 0; i < fuelgauge->temp_table_size; i++) {
				u32 temp;
				ret = of_property_read_u32_index(np,
								 "fuelgauge,temp_table", i, &temp);
				fuelgauge->temp_table[i].adc = (int)temp;
				
				ret = of_property_read_u32_index(np,
								 "fuelgauge,temp_comp_table", i, &temp);
				fuelgauge->temp_table[i].data = (int)temp;
			}
		}

		np = of_find_node_by_name(NULL, "battery");
		if (!np) {
			pr_err("%s np NULL\n", __func__);
		} else {
			ret = of_property_read_string(np,
				"battery,fuelgauge_name",
				(char const **)&fuelgauge->pdata->fuelgauge_name);
			p = of_get_property(np,
					"battery,input_current_limit", &len);
			if (!p)
				return 1;

			len = len / sizeof(u32);
			fuelgauge->pdata->charging_current =
					kzalloc(sizeof(struct sec_charging_current) * len,
					GFP_KERNEL);

			for(i = 0; i < len; i++) {
				ret = of_property_read_u32_index(np,
					"battery,input_current_limit", i,
					&fuelgauge->pdata->charging_current[i].input_current_limit);
				ret = of_property_read_u32_index(np,
					"battery,fast_charging_current", i,
					&fuelgauge->pdata->charging_current[i].fast_charging_current);
				ret = of_property_read_u32_index(np,
					"battery,full_check_current_1st", i,
					&fuelgauge->pdata->charging_current[i].full_check_current_1st);
				ret = of_property_read_u32_index(np,
					"battery,full_check_current_2nd", i,
					&fuelgauge->pdata->charging_current[i].full_check_current_2nd);
			}
		}

		/* get battery_params node */
		np = of_find_node_by_name(NULL, "battery_params");
		if (!np) {
			pr_err("%s battery_params node NULL\n", __func__);
		} else {
#if !defined(CONFIG_BATTERY_AGE_FORECAST)
			/* get battery_table */
			ret = of_property_read_u32_array(np, "battery,battery_table1", fuelgauge->info.battery_table1, 88);
			if (ret < 0) {
				pr_err("%s error reading battery,battery_table1\n", __func__);
			}

			ret = of_property_read_u32_array(np, "battery,battery_table2", fuelgauge->info.battery_table2, 22);
			if (ret < 0) {
				pr_err("%s error reading battery,battery_table2\n", __func__);
			}

			ret = of_property_read_u32_array(np, "battery,battery_table3", fuelgauge->info.battery_table3, 88);
			if (ret < 0) {
				pr_err("%s error reading battery,battery_table3\n", __func__);
			}

			ret = of_property_read_u32_array(np, "battery,battery_table4", fuelgauge->info.battery_table4, 22);
			if (ret < 0) {
				pr_err("%s error reading battery,battery_table4\n", __func__);
			}

			ret = of_property_read_u32_array(np, "battery,batcap", fuelgauge->info.batcap, 4);
			if (ret < 0) {
				pr_err("%s error reading battery,batcap\n", __func__);
			}

			ret = of_property_read_u32_array(np, "battery,soc_arr_evt2", fuelgauge->info.soc_arr_evt2, 22);
			if (ret < 0) {
				pr_err("%s error reading battery,soc_arr_evt2\n", __func__);
			}

			ret = of_property_read_u32_array(np, "battery,ocv_arr_evt2", fuelgauge->info.ocv_arr_evt2, 22);
			if (ret < 0) {
				pr_err("%s error reading battery,ocv_arr_evt2\n", __func__);
			}
#else
			of_get_property(np, "battery,battery_data", &len);
			fuelgauge->fg_num_age_step = len / sizeof(fg_age_data_info_t);
			fuelgauge->age_data_info = kzalloc(len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,battery_data",
					(int *)fuelgauge->age_data_info, len/sizeof(int));

			pr_err("%s: [Long life] fuelgauge->fg_num_age_step %d \n", __func__,fuelgauge->fg_num_age_step);

			for (i=0 ; i < fuelgauge->fg_num_age_step ; i++){
				pr_err("%s: [Long life] age_step = %d, table3[0] %d, table4[0] %d, batcap[0] %02x, accum[0] %02x, soc_arr[0] %d, ocv_arr[0] %d \n",
					__func__, i,
					fuelgauge->age_data_info[i].battery_table3[0],
					fuelgauge->age_data_info[i].battery_table4[0],
					fuelgauge->age_data_info[i].batcap[0],
					fuelgauge->age_data_info[i].accum[0],
					fuelgauge->age_data_info[i].soc_arr_val[0],
					fuelgauge->age_data_info[i].ocv_arr_val[0]);
			}
#endif
		}
	}

	return 0;
}

static struct of_device_id s2mu005_fuelgauge_match_table[] = {
        { .compatible = "samsung,s2mu005-fuelgauge",},
        {},
};
#else
static int s2mu005_fuelgauge_parse_dt(struct s2mu005_fuelgauge_data *fuelgauge)
{
    return -ENOSYS;
}

#define s2mu005_fuelgauge_match_table NULL
#endif /* CONFIG_OF */

static int s2mu005_fuelgauge_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct s2mu005_fuelgauge_data *fuelgauge;
	union power_supply_propval raw_soc_val;
	int ret = 0;
	u8 temp = 0;

	pr_info("%s: S2MU005 Fuelgauge Driver Loading\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	fuelgauge = kzalloc(sizeof(*fuelgauge), GFP_KERNEL);
	if (!fuelgauge)
		return -ENOMEM;

	mutex_init(&fuelgauge->fg_lock);

	fuelgauge->i2c = client;

	if (client->dev.of_node) {
		fuelgauge->pdata = devm_kzalloc(&client->dev, sizeof(*(fuelgauge->pdata)),
				GFP_KERNEL);
		if (!fuelgauge->pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_parse_dt_nomem;
		}
		ret = s2mu005_fuelgauge_parse_dt(fuelgauge);
		if (ret < 0)
			goto err_parse_dt;
	} else {
		fuelgauge->pdata = client->dev.platform_data;
	}

	i2c_set_clientdata(client, fuelgauge);

	if (fuelgauge->pdata->fuelgauge_name == NULL)
		fuelgauge->pdata->fuelgauge_name = "sec-fuelgauge";

	fuelgauge->psy_fg.name          = fuelgauge->pdata->fuelgauge_name;
	fuelgauge->psy_fg.type          = POWER_SUPPLY_TYPE_UNKNOWN;
	fuelgauge->psy_fg.get_property  = s2mu005_fg_get_property;
	fuelgauge->psy_fg.set_property  = s2mu005_fg_set_property;
	fuelgauge->psy_fg.properties    = s2mu005_fuelgauge_props;
	fuelgauge->psy_fg.num_properties =
			ARRAY_SIZE(s2mu005_fuelgauge_props);

	/* 0x48[7:4]=0010 : EVT2 */
	fuelgauge->revision = 0;
	s2mu005_read_reg_byte(fuelgauge->i2c, 0x48, &temp);
	fuelgauge->revision = (temp & 0xF0) >> 4;
	pr_info("%s: S2MU005 Fuelgauge revision: %d, reg 0x48 = 0x%x\n", __func__, fuelgauge->revision, temp);

	fuelgauge->capacity_max = fuelgauge->pdata->capacity_max;
	fuelgauge->info.soc = 0;
	fuelgauge->mode = CURRENT_MODE;

	raw_soc_val.intval = s2mu005_get_rawsoc(fuelgauge);
	raw_soc_val.intval = raw_soc_val.intval / 10;

	if (raw_soc_val.intval > fuelgauge->capacity_max)
		s2mu005_fg_calculate_dynamic_scale(fuelgauge, 100);

	s2mu005_init_regs(fuelgauge);

	ret = power_supply_register(&client->dev, &fuelgauge->psy_fg);
	if (ret) {
		pr_err("%s: Failed to Register psy_fg\n", __func__);
		goto err_data_free;
	}

	fuelgauge->is_fuel_alerted = false;
	if (fuelgauge->pdata->fuel_alert_soc >= 0) {
		s2mu005_fuelgauge_fuelalert_init(fuelgauge->i2c,
					fuelgauge->pdata->fuel_alert_soc);
		wake_lock_init(&fuelgauge->fuel_alert_wake_lock,
					WAKE_LOCK_SUSPEND, "fuel_alerted");

		if (fuelgauge->pdata->fg_irq > 0) {
			INIT_DELAYED_WORK(
					&fuelgauge->isr_work, s2mu005_fg_isr_work);

			fuelgauge->fg_irq = gpio_to_irq(fuelgauge->pdata->fg_irq);
			dev_info(&client->dev,
					"%s : fg_irq = %d\n", __func__, fuelgauge->fg_irq);
			if (fuelgauge->fg_irq > 0) {
				ret = request_threaded_irq(fuelgauge->fg_irq,
						NULL, s2mu005_fg_irq_thread,
						IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING
						| IRQF_ONESHOT,
						"fuelgauge-irq", fuelgauge);
				if (ret) {
					dev_err(&client->dev,
							"%s: Failed to Request IRQ\n", __func__);
					goto err_supply_unreg;
				}

				ret = enable_irq_wake(fuelgauge->fg_irq);
				if (ret < 0)
					dev_err(&client->dev,
							"%s: Failed to Enable Wakeup Source(%d)\n",
							__func__, ret);
			} else {
				dev_err(&client->dev, "%s: Failed gpio_to_irq(%d)\n",
						__func__, fuelgauge->fg_irq);
				goto err_supply_unreg;
			}
		}
	}

	fuelgauge->sleep_initial_update_of_soc = false;
	fuelgauge->initial_update_of_soc = true;

	fuelgauge->cc_on = true;

	pr_info("%s: S2MU005 Fuelgauge Driver Loaded\n", __func__);
	return 0;

err_supply_unreg:
	power_supply_unregister(&fuelgauge->psy_fg);
err_data_free:
	if (client->dev.of_node)
		kfree(fuelgauge->pdata);

err_parse_dt:
err_parse_dt_nomem:
	mutex_destroy(&fuelgauge->fg_lock);
	kfree(fuelgauge);

	return ret;
}

static const struct i2c_device_id s2mu005_fuelgauge_id[] = {
	{"s2mu005-fuelgauge", 0},
	{}
};

static void s2mu005_fuelgauge_shutdown(struct i2c_client *client)
{
	struct s2mu005_fuelgauge_data *fuelgauge = i2c_get_clientdata(client);
	u8 temp = 0;

	if(fuelgauge->revision < 2) {
		s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
		temp &= 0xF0;
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);
		s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0x00);
	}
}

static int s2mu005_fuelgauge_remove(struct i2c_client *client)
{
	struct s2mu005_fuelgauge_data *fuelgauge = i2c_get_clientdata(client);

	if (fuelgauge->pdata->fuel_alert_soc >= 0)
		wake_lock_destroy(&fuelgauge->fuel_alert_wake_lock);

	return 0;
}

#if defined CONFIG_PM
static int s2mu005_fuelgauge_suspend(struct device *dev)
{
	struct s2mu005_fuelgauge_data *fuelgauge = dev_get_drvdata(dev);
	u8 temp = 0;

	if(fuelgauge->revision < 2) {
		if (!fuelgauge->is_charging) {
			if (fuelgauge->mode == CURRENT_MODE) {
					s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
					temp &= 0xF0;
					temp |= 0x06;
					s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0xBD);
			} else {
				s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
				temp &= 0xF0;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0x00);
			}

			s2mu005_read_reg_byte(fuelgauge->i2c, 0x44, &temp);
			pr_info("%s: Reg set suspend 0x44 : 0x%x\n",
				__func__, temp);
			s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
			pr_info("%s: Reg set suspend 0x45 : 0x%x\n",
				__func__, temp);
		}
	}
	return 0;
}

static int s2mu005_fuelgauge_resume(struct device *dev)
{
	struct s2mu005_fuelgauge_data *fuelgauge = dev_get_drvdata(dev);
	static int avg_vbat[5] = {0, };
	static int vbat[5] = {0, };
	static int avg_current[5] = {100, 100, 100, 100, 100};
	static int loop_count = 0;
	int target_ocv = 0, target_soc = 0, temp_vol = 0, j = 0, k = 0;
	u8 temp = 0;

	if(fuelgauge->revision < 2) {
			if (fuelgauge->mode == CURRENT_MODE) {
				avg_current[loop_count] = s2mu005_get_avgcurrent(fuelgauge);
				avg_vbat[loop_count] =  s2mu005_get_avgvbat(fuelgauge);
				vbat[loop_count] = s2mu005_get_vbat(fuelgauge);

			if (loop_count++ >= 5) loop_count = 0;

			for (j = 0; j < 5; j++) {
				pr_info("%s: abs avergae current : %ld\n", __func__, abs(avg_current[j]));
				if (abs(avg_current[j]) > 30)
					break;
			}

			pr_info("%s: avg current count : %d\n", __func__, j);
			if (j >= 5) {
				for (k = 0; k < 5; k++) {
					if (avg_vbat[k] > vbat[k])
						temp_vol = avg_vbat[k];
					else
						temp_vol = vbat[k];

					if (temp_vol > target_ocv)
						target_ocv = temp_vol;
				}

				pr_info("%s: target ocv : %d\n", __func__, target_ocv);

				/* work-around for restart */
				fuelgauge->target_ocv = target_ocv;      /* max( vbat[5], avgvbat[5] ) */
				target_soc = s2mu005_get_soc_from_ocv(fuelgauge, fuelgauge->target_ocv);

				if( abs(target_soc - fuelgauge->info.soc) > 300 )
					WA_0_issue_at_init1(fuelgauge, fuelgauge->target_ocv);
			}
		}

		if (!fuelgauge->is_charging) {
			if (fuelgauge->mode == CURRENT_MODE) {
				s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
				temp &= 0xF0;
				temp |= 0x07;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0xCC);
			} else {
				s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
				temp &= 0xF0;
				s2mu005_write_reg_byte(fuelgauge->i2c, 0x45, temp);

				s2mu005_write_reg_byte(fuelgauge->i2c, 0x44, 0x00);
			}
		}

		s2mu005_read_reg_byte(fuelgauge->i2c, 0x44, &temp);
		pr_info("%s: Reg set resume 0x44 : 0x%x\n",
				__func__, temp);
		s2mu005_read_reg_byte(fuelgauge->i2c, 0x45, &temp);
		pr_info("%s: Reg set resume 0x45 : 0x%x\n",
				__func__, temp);
	}

	fuelgauge->sleep_initial_update_of_soc = true;

	return 0;
}
#else
#define s2mu005_fuelgauge_suspend NULL
#define s2mu005_fuelgauge_resume NULL
#endif

static SIMPLE_DEV_PM_OPS(s2mu005_fuelgauge_pm_ops, s2mu005_fuelgauge_suspend,
		s2mu005_fuelgauge_resume);

static struct i2c_driver s2mu005_fuelgauge_driver = {
	.driver = {
		.name = "s2mu005-fuelgauge",
		.owner = THIS_MODULE,
		.pm = &s2mu005_fuelgauge_pm_ops,
		.of_match_table = s2mu005_fuelgauge_match_table,
	},
	.probe  = s2mu005_fuelgauge_probe,
	.remove = s2mu005_fuelgauge_remove,
	.shutdown   = s2mu005_fuelgauge_shutdown,
	.id_table   = s2mu005_fuelgauge_id,
};

static int __init s2mu005_fuelgauge_init(void)
{
	pr_info("%s: S2MU005 Fuelgauge Init\n", __func__);
	return i2c_add_driver(&s2mu005_fuelgauge_driver);
}

static void __exit s2mu005_fuelgauge_exit(void)
{
	i2c_del_driver(&s2mu005_fuelgauge_driver);
}
module_init(s2mu005_fuelgauge_init);
module_exit(s2mu005_fuelgauge_exit);

MODULE_DESCRIPTION("Samsung S2MU005 Fuel Gauge Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
