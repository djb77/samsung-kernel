/* drivers/input/touchscreen/sec_ts_fn.c
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_ts.h"

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_threshold(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void set_mis_cal_spec(void *device_data);
static void get_mis_cal_info(void *device_data);
static void get_wet_mode(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void get_checksum_data(void *device_data);
static void run_reference_read(void *device_data);
static void run_reference_read_all(void *device_data);
static void get_reference(void *device_data);
static void run_rawcap_read(void *device_data);
static void run_rawcap_read_all(void *device_data);
static void run_gap_data_x_read(void *device_data);
static void run_gap_data_y_read(void *device_data);
static void run_gap_data_x_read_all(void *device_data);
static void run_gap_data_y_read_all(void *device_data);
static void run_noise_read(void *device_data);
static void run_noise_read_all(void *device_data);
static void run_open_read(void *device_data);
static void run_short_read(void *device_data);
static void run_open_read_all(void *device_data);
static void run_short_read_all(void *device_data);
static void run_high_resistance_read(void *device_data);
static void run_high_resistance_read_all(void *device_data);
static void get_rawcap(void *device_data);
static void run_delta_read(void *device_data);
static void run_delta_read_all(void *device_data);
static void get_delta(void *device_data);
static void run_decoded_raw_read_all(void *device_data);
static void run_delta_cm_read_all(void *device_data);
static void run_rawdata_read_all(void *device_data);
static void run_force_calibration(void *device_data);
static void get_force_calibration(void *device_data);

#ifdef TCLM_CONCEPT
static void set_external_factory(void *device_data);
static void get_pat_information(void *device_data);
#endif

static void sec_ts_execute_selftest(void *device_data);
static void set_tsp_test_result(void *device_data);
static void get_tsp_test_result(void *device_data);
static void increase_disassemble_count(void *device_data);
static void get_disassemble_count(void *device_data);
static void dead_zone_enable(void *device_data);
static void set_grip_data(void *device_data);
static void dex_enable(void *device_data);
static void brush_enable(void *device_data);
static void set_touchable_area(void *device_data);
static void set_log_level(void *device_data);
static void check_connection(void *device_data);
static void debug(void *device_data);
static void factory_cmd_result_all(void *device_data);
static void set_wirelesscharger_mode(void *device_data);
static void set_factory_level(void *device_data);
static void not_support_cmd(void *device_data);
static int execute_selftest(struct sec_ts_data *ts, u8 first_reg, u8 boot_flag);
static int execute_selftest_result_read(struct sec_ts_data *ts);

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("get_config_ver", get_config_ver),},
	{SEC_CMD("get_threshold", get_threshold),},
	{SEC_CMD("get_chip_vendor", get_chip_vendor),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("set_mis_cal_spec", set_mis_cal_spec),},
	{SEC_CMD("get_mis_cal_info", get_mis_cal_info),},
	{SEC_CMD("get_wet_mode", get_wet_mode),},
	{SEC_CMD("get_x_num", get_x_num),},
	{SEC_CMD("get_y_num", get_y_num),},
	{SEC_CMD("get_checksum_data", get_checksum_data),},
	{SEC_CMD("run_reference_read", run_reference_read),},
	{SEC_CMD("run_reference_read_all", run_reference_read_all),},
	{SEC_CMD("get_reference", get_reference),},
	{SEC_CMD("run_rawcap_read", run_rawcap_read),},
	{SEC_CMD("run_rawcap_read_all", run_rawcap_read_all),},
	{SEC_CMD("get_rawcap", get_rawcap),},
	{SEC_CMD("run_gap_data_x_read", run_gap_data_x_read),},
	{SEC_CMD("run_gap_data_y_read", run_gap_data_y_read),},
	{SEC_CMD("run_gap_data_x_read_all", run_gap_data_x_read_all),},
	{SEC_CMD("run_gap_data_y_read_all", run_gap_data_y_read_all),},
	{SEC_CMD("run_noise_read", run_noise_read),},
	{SEC_CMD("run_noise_read_all", run_noise_read_all),},
	{SEC_CMD("run_open_read", run_open_read),},
	{SEC_CMD("run_short_read", run_short_read),},
	{SEC_CMD("run_open_read_all", run_open_read_all),},
	{SEC_CMD("run_oshort_read_all", run_short_read_all),},
	{SEC_CMD("run_high_resistance_read", run_high_resistance_read),},
	{SEC_CMD("run_high_resistance_read_all", run_high_resistance_read_all),},
	{SEC_CMD("get_delta", get_delta),},
	{SEC_CMD("run_cs_raw_read_all", run_decoded_raw_read_all),},
	{SEC_CMD("run_cs_delta_read_all", run_delta_cm_read_all),},
	{SEC_CMD("run_delta_read", run_delta_read),},
	{SEC_CMD("run_delta_read_all", run_delta_read_all),},
	{SEC_CMD("run_rawdata_read_all", run_rawdata_read_all),},
	{SEC_CMD("run_force_calibration", run_force_calibration),},
	{SEC_CMD("get_force_calibration", get_force_calibration),},
#ifdef TCLM_CONCEPT
	{SEC_CMD("get_pat_information", get_pat_information),},
	{SEC_CMD("set_external_factory", set_external_factory),},
#endif
	{SEC_CMD("run_trx_short_test", sec_ts_execute_selftest),},
	{SEC_CMD("set_tsp_test_result", set_tsp_test_result),},
	{SEC_CMD("get_tsp_test_result", get_tsp_test_result),},
	{SEC_CMD("increase_disassemble_count", increase_disassemble_count),},
	{SEC_CMD("get_disassemble_count", get_disassemble_count),},
	{SEC_CMD("dead_zone_enable", dead_zone_enable),},
	{SEC_CMD("set_wirelesscharger_mode", set_wirelesscharger_mode),},
	{SEC_CMD("set_grip_data", set_grip_data),},
	{SEC_CMD("dex_enable", dex_enable),},
	{SEC_CMD("brush_enable", brush_enable),},
	{SEC_CMD("set_touchable_area", set_touchable_area),},
	{SEC_CMD("set_log_level", set_log_level),},
	{SEC_CMD("check_connection", check_connection),},
	{SEC_CMD("debug", debug),},
	{SEC_CMD("factory_cmd_result_all", factory_cmd_result_all),},
	{SEC_CMD("set_factory_level", set_factory_level),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

static void sec_ts_execute_selftest(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	int ret;
	char buff[256] = { 0 };
	char rwbuf[2];

	sec_cmd_set_default_result(sec);

	ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
				__func__);
		goto out;
	}
	/* selftest result for minority */
	/* 1byte result : High Resistance | Short | Open | Noise(max) | Noise(min) | */
	/* Raw Data Variance(Vertical) | Raw Data Variance(Horizontal) | Sensor Uniformity*/
	/* saved ito_test[0] by result */
	disable_irq(ts->client->irq);
	execute_selftest(ts, 0, true);
	enable_irq(ts->client->irq);
	execute_selftest_result_read(ts);
	minority_report_calculate_ito(ts);
	minority_report_sync_latest_value(ts);

	sec_ts_release_tmode(ts);

	if (ts->ito_test[0] != 0)
		goto out;

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s ito_test : %2X\n", __func__, buff, ts->ito_test[0]);
	send_event_to_user(ts, sec->cmd_param[0], UEVENT_OPEN_SHORT_PASS);

	rwbuf[0] = 0x2;
	rwbuf[1] = 0x2;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, rwbuf, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: chage sysmode fail\n", __func__);
	return;

out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &ts->client->dev, "%s: %s ito_test : %2X\n", __func__, buff, ts->ito_test[0]);
	send_event_to_user(ts, sec->cmd_param[0], UEVENT_OPEN_SHORT_FAIL);

	rwbuf[0] = 0x2;
	rwbuf[1] = 0x2;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, rwbuf, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: chage sysmode fail\n", __func__);
}


void send_event_to_user(struct sec_ts_data *ts, int number, int val)
{
	char timestamp[32];
	char feature[32];
	char result[32];
	char test[32];
	char *event[5];
	ktime_t calltime;
	u64 realtime;
	int curr_time;
	char *eol = "\0";

	calltime = ktime_get();
	realtime = ktime_to_ns(calltime);
	do_div(realtime, NSEC_PER_USEC);
	do_div(realtime,USEC_PER_MSEC);
	curr_time = realtime;

	snprintf(timestamp, 32, "TIMESTAMP=%d", curr_time);
	strncat(timestamp, eol, 1);
	snprintf(feature, 32, "FEATURE=TSP");
	strncat(feature, eol, 1);
	snprintf(test, 32, "TEST=%d", number);
	strncat(test, eol, 1);
	if (val == UEVENT_OPEN_SHORT_PASS)
		snprintf(result, 32, "RESULT=PASS");
	else if (val == UEVENT_OPEN_SHORT_FAIL)
		snprintf(result, 32, "RESULT=FAIL");
	else if (val == UEVENT_TSP_I2C_RESET)
		snprintf(result, 32, "RESULT=RESET");
	else if (val == UEVENT_TSP_I2C_ERROR)
		snprintf(result, 32, "RESULT=I2C");
	else
		snprintf(result, 32, "RESULT=NULL");
	strncat(result, eol, 1);

	input_info(true, &ts->client->dev, "%s: time:%s, feature:%s, result:%s\n", __func__, timestamp, feature, result);
	event[0] = timestamp;
	event[1] = feature;
	event[2] = test;
	event[3] = result;
	event[4] = NULL;

	kobject_uevent_env(&ts->sec.fac_dev->kobj, KOBJ_CHANGE, event);
}

static ssize_t read_ito_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[256] = { 0 };

	input_info(true, &ts->client->dev, "%s: %02X\n", __func__,
			ts->ito_test[0]);

	snprintf(buff, sizeof(buff), "%02X", ts->ito_test[0]);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t read_raw_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	int ii, ret = 0;
	char *buffer = NULL;
	char temp[CMD_RESULT_WORD_LEN] = { 0 };

	buffer = vzalloc(ts->rx_count * ts->tx_count * 6);
	if (!buffer)
		return -ENOMEM;

	memset(buffer, 0x00, ts->rx_count * ts->tx_count * 6);

	for (ii = 0; ii < (ts->rx_count * ts->tx_count - 1); ii++) {
		snprintf(temp, CMD_RESULT_WORD_LEN, "%d ", ts->pFrame[ii]);
		strlcat(buffer, temp, ts->rx_count * ts->tx_count * 6);

		memset(temp, 0x00, CMD_RESULT_WORD_LEN);
	}

	snprintf(temp, CMD_RESULT_WORD_LEN, "%d", ts->pFrame[ii]);
	strlcat(buffer, temp, ts->rx_count * ts->tx_count * 6);

	ret = snprintf(buf, ts->rx_count * ts->tx_count * 6, buffer);
	vfree(buffer);

	return ret;
}

static ssize_t read_multi_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__,
			ts->multi_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", ts->multi_count);
}

static ssize_t clear_multi_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->multi_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_wet_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d, %d\n", __func__,
			ts->wet_count, ts->dive_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", ts->wet_count);
}

static ssize_t clear_wet_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->wet_count = 0;
	ts->dive_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_noise_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, ts->noise_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", ts->noise_count);
}

static ssize_t clear_noise_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->noise_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_comm_err_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__,
			ts->multi_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", ts->comm_err_count);
}

static ssize_t clear_comm_err_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->comm_err_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_module_id_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[256] = { 0 };

	input_info(true, &ts->client->dev, "%s: %d\n", __func__,
			ts->multi_count);

	snprintf(buff, sizeof(buff), "SE%02X%02X%02X%02X%1c%01X",
		ts->plat_data->panel_revision, ts->plat_data->img_version_of_bin[2],
		ts->plat_data->img_version_of_bin[3], ts->nv,
		ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].s_name, ts->tdata->nvdata.cal_count & 0xF);


	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t read_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	unsigned char buffer[10] = { 0 };

	if (ts->plat_data->firmware_name != NULL)
		snprintf(buffer, 5, ts->plat_data->firmware_name + 8);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "LSI_%s", buffer);
}

static ssize_t clear_checksum_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->checksum_result = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_checksum_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__,
			ts->checksum_result);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", ts->checksum_result);
}

static ssize_t clear_holding_time_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->time_longest = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_holding_time_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %ld\n", __func__,
			ts->time_longest);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%ld", ts->time_longest);
}

static ssize_t read_all_touch_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: touch:%d\n", __func__, ts->all_finger_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE,
			"\"TTCN\":\"%d\"",
			ts->all_finger_count);
}

static ssize_t read_z_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: max:%d, min:%d, avg:%d\n", __func__,
			ts->max_z_value, ts->min_z_value,
			ts->sum_z_value);

	if (ts->all_finger_count)
		return snprintf(buf, SEC_CMD_BUF_SIZE,
				"\"TMXZ\":\"%d\",\"TMNZ\":\"%d\",\"TAVZ\":\"%d\"",
				ts->max_z_value, ts->min_z_value,
				ts->sum_z_value / ts->all_finger_count);
	else
		return snprintf(buf, SEC_CMD_BUF_SIZE,
				"\"TMXZ\":\"%d\",\"TMNZ\":\"%d\"",
				ts->max_z_value, ts->min_z_value);

}

static ssize_t clear_z_value_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->max_z_value = 0;
	ts->min_z_value = 0xFFFFFFFF;
	ts->sum_z_value = 0;
	ts->all_finger_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t sensitivity_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char value_result[10] = { 0 };
	int ret;
	int value[5];

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SENSITIVITY_VALUE, value_result, 10);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c fail!, %d\n", __func__, ret);
		return ret;
	}

	value[0] = value_result[0]<<8 | value_result[1];
	value[1] = value_result[2]<<8 | value_result[3];
	value[2] = value_result[4]<<8 | value_result[5];
	value[3] = value_result[6]<<8 | value_result[7];
	value[4] = value_result[8]<<8 | value_result[9];

	input_info(true, &ts->client->dev, "%s: sensitivity mode,%d,%d,%d,%d,%d\n", __func__,
		value[0], value[1], value[2], value[3], value[4]);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d,%d,%d,%d,%d",
			value[0], value[1], value[2], value[3], value[4]);

}

static ssize_t sensitivity_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	int ret;
	u8 temp;
	unsigned long value = 0;

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret != 0)
		return ret;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: power off in IC\n", __func__);
		return 0;
	}

	input_err(true, &ts->client->dev, "%s: enable:%d\n", __func__, value);

	if (value == 1) {
		temp = 0x1;
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSITIVITY_MODE, &temp, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: send sensitivity mode on fail!\n", __func__);
			return ret;
		}
		sec_ts_delay(30);
		input_info(true, &ts->client->dev, "%s: enable end\n", __func__);
	} else {
		temp = 0x0;
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSITIVITY_MODE, &temp, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: send sensitivity mode off fail!\n", __func__);
			return ret;
		}
		input_info(true, &ts->client->dev, "%s: disable end\n", __func__);
	}

	input_info(true, &ts->client->dev, "%s: done\n", __func__);

	return count;
}


enum offset_fac_position {
	OFFSET_FAC_SUB		= 1,
	OFFSET_FAC_MAIN		= 2,
	OFFSET_FAC_SVC		= 3,
};

enum offset_fw_position {
	OFFSET_FW_SDC		= 0,
	OFFSET_FW_SUB		= 1,
	OFFSET_FW_MAIN		= 2,
	OFFSET_FW_SVC		= 3,
};

static void set_factory_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 position;
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		goto NG;
	}

	if (sec->cmd_param[0] < OFFSET_FAC_SUB || sec->cmd_param[0] > OFFSET_FAC_MAIN) {
		input_err(true, &ts->client->dev, "%s: cmd data is abnormal, %d\n", __func__, sec->cmd_param[0]);
		goto NG;
	}

	position = sec->cmd_param[0];

	ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
				__func__);
		goto NG;
	}

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CM_OFFSET_WRITE, &position, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: OFFSET CAL WRITE failed\n", __func__);
		goto NG;
	}

	sec_ts_delay(150);

	ret = sec_ts_release_tmode(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to release tmode\n",
				__func__);
		goto NG;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	return;

NG:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

}

static ssize_t get_cmoffset_dump(struct sec_ts_data *ts, char *buf, u8 position)
{
	u8 *rBuff;
	int i, j, ret;
	int max_node = ts->tx_count * ts->rx_count;
	char buff[6] = {0, };
	u8 temp;

	rBuff = kzalloc(max_node, GFP_KERNEL);
	if (!rBuff)
		goto err_mem;

	/* write index data */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CM_OFFSET_READ_SET, &position, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: write cmoffset failed!\n", __func__);
		goto err_i2c;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_CM_OFFSET_READ, rBuff, max_node);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read rawdata failed!\n", __func__);
		goto err_i2c;
	}

	for (i = 0; i < ts->rx_count; i++) {
		for (j = 0; j < ts->tx_count; j++) {
			temp = rBuff[(j * ts->rx_count) + i];

			snprintf(buff, sizeof(buff), " %4d", temp);
			strncat(buf, buff, sizeof(buff));
		}
		snprintf(buff, sizeof(buff), "\n");
		strncat(buf, buff, sizeof(buff));
	}
	input_err(true, &ts->client->dev, "%s: total buf size:%d\n", __func__, strlen(buf));

	kfree(rBuff);
	return strlen(buf);
err_i2c:
	kfree(rBuff);
err_mem:
	return snprintf(buf, SEC_CMD_BUF_SIZE, "NG, error");
}

static ssize_t get_cmoffset_sdc(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
	}

	return get_cmoffset_dump(ts, buf, OFFSET_FW_SDC);
}

static ssize_t get_cmoffset_sub(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
	}

	return get_cmoffset_dump(ts, buf, OFFSET_FW_SUB);
}

static ssize_t get_cmoffset_main (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
	}

	return get_cmoffset_dump(ts, buf, OFFSET_FW_MAIN);
}

static DEVICE_ATTR(ito_check, S_IRUGO, read_ito_check_show, NULL);
static DEVICE_ATTR(raw_check, S_IRUGO, read_raw_check_show, NULL);
static DEVICE_ATTR(multi_count, S_IRUGO | S_IWUSR | S_IWGRP, read_multi_count_show, clear_multi_count_store);
static DEVICE_ATTR(wet_mode, S_IRUGO | S_IWUSR | S_IWGRP, read_wet_mode_show, clear_wet_mode_store);
static DEVICE_ATTR(noise_mode, S_IRUGO | S_IWUSR | S_IWGRP, read_noise_mode_show, clear_noise_mode_store);
static DEVICE_ATTR(comm_err_count, S_IRUGO | S_IWUSR | S_IWGRP, read_comm_err_count_show, clear_comm_err_count_store);
static DEVICE_ATTR(checksum, S_IRUGO | S_IWUSR | S_IWGRP, read_checksum_show, clear_checksum_store);
static DEVICE_ATTR(holding_time, S_IRUGO | S_IWUSR | S_IWGRP, read_holding_time_show, clear_holding_time_store);
static DEVICE_ATTR(all_touch_count, S_IRUGO | S_IWUSR | S_IWGRP, read_all_touch_count_show, NULL);
static DEVICE_ATTR(z_value, S_IRUGO | S_IWUSR | S_IWGRP, read_z_value_show, clear_z_value_store);
static DEVICE_ATTR(module_id, S_IRUGO, read_module_id_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, read_vendor_show, NULL);
static DEVICE_ATTR(sensitivity_mode, S_IRUGO | S_IWUSR | S_IWGRP, sensitivity_mode_show, sensitivity_mode_store);
static DEVICE_ATTR(cmoffset_sdc, S_IRUGO, get_cmoffset_sdc, NULL);
static DEVICE_ATTR(cmoffset_sub, S_IRUGO, get_cmoffset_sub, NULL);
static DEVICE_ATTR(cmoffset_main, S_IRUGO, get_cmoffset_main, NULL);

static struct attribute *cmd_attributes[] = {
	&dev_attr_ito_check.attr,
	&dev_attr_raw_check.attr,
	&dev_attr_multi_count.attr,
	&dev_attr_wet_mode.attr,
	&dev_attr_noise_mode.attr,
	&dev_attr_comm_err_count.attr,
	&dev_attr_checksum.attr,
	&dev_attr_holding_time.attr,
	&dev_attr_all_touch_count.attr,
	&dev_attr_z_value.attr,
	&dev_attr_module_id.attr,
	&dev_attr_vendor.attr,
	&dev_attr_sensitivity_mode.attr,
	&dev_attr_cmoffset_sdc.attr,
	&dev_attr_cmoffset_sub.attr,
	&dev_attr_cmoffset_main.attr,
	NULL,
};

static struct attribute_group cmd_attr_group = {
	.attrs = cmd_attributes,
};

static int sec_ts_check_index(struct sec_ts_data *ts)
{
	struct sec_cmd_data *sec = &ts->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int node;

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > ts->tx_count
			|| sec->cmd_param[1] < 0 || sec->cmd_param[1] > ts->rx_count) {

		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_info(true, &ts->client->dev, "%s: parameter error: %u, %u\n",
				__func__, sec->cmd_param[0], sec->cmd_param[0]);
		node = -1;
		return node;
	}
	node = sec->cmd_param[1] * ts->tx_count + sec->cmd_param[0];
	input_info(true, &ts->client->dev, "%s: node = %d\n", __func__, node);
	return node;
}
static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[64] = { 0 };
	int retval = 0;

	sec_cmd_set_default_result(sec);
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	retval = sec_ts_firmware_update_on_hidden_menu(ts, sec->cmd_param[0]);
	if (retval < 0) {
		snprintf(buff, sizeof(buff), "%s", "NA");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &ts->client->dev, "%s: failed [%d]\n", __func__, retval);
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		input_info(true, &ts->client->dev, "%s: success [%d]\n", __func__, retval);
	}
}

int sec_ts_fix_tmode(struct sec_ts_data *ts, u8 mode, u8 state)
{
	int ret;
	u8 onoff[1] = {STATE_MANAGE_OFF};
	u8 tBuff[2] = { mode, state };

	input_info(true, &ts->client->dev, "%s\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, onoff, 1);
	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, tBuff, sizeof(tBuff));
	sec_ts_delay(20);

	return ret;
}

int sec_ts_release_tmode(struct sec_ts_data *ts)
{
	int ret;
	u8 onoff[1] = {STATE_MANAGE_ON};

	input_info(true, &ts->client->dev, "%s\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, onoff, 1);
	sec_ts_delay(20);

	return ret;
}

static void sec_ts_print_frame(struct sec_ts_data *ts, short *min, short *max, u8 selftest_type)
{
	int i = 0;
	int j = 0;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	int gap_para_x;
	int gap_para_y;

	if (selftest_type == TYPE_SELFTEST_RAW_VARIANCE_X) {
		gap_para_x = ts->rx_count - 1;
		gap_para_y = ts->tx_count;
	} else if (selftest_type == TYPE_SELFTEST_RAW_VARIANCE_Y) {
		gap_para_x = ts->rx_count;
		gap_para_y = ts->tx_count - 1;
	} else {
		gap_para_x = ts->rx_count;
		gap_para_y = ts->tx_count;
	}

	input_raw_info(true, &ts->client->dev, "%s\n", __func__);

	pStr = kzalloc(6 * (gap_para_y + 1), GFP_KERNEL);
	if (pStr == NULL)
		return;

	memset(pStr, 0x0, 6 * (gap_para_y + 1));
	snprintf(pTmp, sizeof(pTmp), "      TX");
	strlcat(pStr, pTmp, 6 * (gap_para_y + 1));

	for (i = 0; i < gap_para_y; i++) {
		snprintf(pTmp, sizeof(pTmp), " %02d ", i);
		strlcat(pStr, pTmp, 6 * (gap_para_y + 1));
	}

	input_raw_info(true, &ts->client->dev, "%s\n", pStr);
	memset(pStr, 0x0, 6 * (gap_para_y + 1));
	snprintf(pTmp, sizeof(pTmp), " +");
	strlcat(pStr, pTmp, 6 * (gap_para_y + 1));

	for (i = 0; i < gap_para_y; i++) {
		snprintf(pTmp, sizeof(pTmp), "----");
		strlcat(pStr, pTmp, 6 * (gap_para_y + 1));
	}

	input_raw_info(true, &ts->client->dev, "%s\n", pStr);

	for (i = 0; i < gap_para_x; i++) {
		memset(pStr, 0x0, 6 * (gap_para_y + 1));
		snprintf(pTmp, sizeof(pTmp), "Rx%02d | ", i);
		strlcat(pStr, pTmp, 6 * (gap_para_x + 1));

		for (j = 0; j < gap_para_y; j++) {
			snprintf(pTmp, sizeof(pTmp), " %3d", ts->pFrame[(j * gap_para_x) + i]);

			if (i > 0) {
				if (ts->pFrame[(j * gap_para_x) + i] < *min)
					*min = ts->pFrame[(j * gap_para_x) + i];

				if (ts->pFrame[(j * gap_para_x) + i] > *max)
					*max = ts->pFrame[(j * gap_para_x) + i];
			}
			strlcat(pStr, pTmp, 6 * (gap_para_y + 1));
		}
		input_raw_info(true, &ts->client->dev, "%s\n", pStr);
	}
	kfree(pStr);
}

static int sec_ts_selftest_read_frame(struct sec_ts_data *ts, u8 type, short *min,
		short *max, u8 selftest_type)
{
	unsigned int readbytes = 0xFF;
	unsigned char *pRead = NULL;
	int ret = 0;
	int i = 0;
	int j = 0;
	short *temp = NULL;
	u8 para = 0;

	input_raw_info(true, &ts->client->dev, "%s: type %d\n", __func__, type);

	/* change self parameter for SEC_TS_CMD_SELFTEST_READ register
	* raw data 		= 0
	* variance_x 		= 1
	* varaince_y 		= 2
	* noise_min 		= 3
	* noise_max 		= 4
	* open_data 		= 5
	* short_data		= 6
	* high_resistaance	= 7
	*/

	switch (selftest_type) {
	case TYPE_SELFTEST_RAW_DATA:
		para = 0;
		break;
	case TYPE_SELFTEST_RAW_VARIANCE_X:
		para = 1;
		break;
	case TYPE_SELFTEST_RAW_VARIANCE_Y:
		para = 2;
		break;
	case TYPE_SELFTEST_NOISE_MIN_DATA:
		para = 3;
		break;
	case TYPE_SELFTEST_NOISE_MAX_DATA:
		para = 4;
		break;
	case TYPE_SELFTEST_OPEN_DATA:
		para = 5;
		break;
	case TYPE_SELFTEST_SHORT_DATA:
		para = 6;
		break;
	case TYPE_SELFTEST_HIGH_RESISTANCE:
		para = 7;
		break;
	default:
		break;
	}

	/* set data length, allocation buffer memory */
	readbytes = ts->rx_count * ts->tx_count * 2;

	pRead = kzalloc(readbytes, GFP_KERNEL);
	if (!pRead)
		return -ENOMEM;

	disable_irq(ts->client->irq);

	ret = execute_selftest(ts, selftest_type, false);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: execute_selftest failed\n", __func__);
		enable_irq(ts->client->irq);
		goto ErrorRelease;
	}

	enable_irq(ts->client->irq);
	/* read data */

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST_READ, &para, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Set selftest_read failed\n", __func__);
		goto ErrorRelease;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SELFTEST_READ, pRead, readbytes);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read rawdata failed!\n", __func__);
		goto ErrorRelease;
	}

	memset(ts->pFrame, 0x00, readbytes);

	if ((selftest_type == TYPE_SELFTEST_RAW_VARIANCE_X) || (selftest_type == TYPE_SELFTEST_RAW_VARIANCE_Y)) {
		for (i = 0; i < (readbytes / 2); i++)
			ts->pFrame[i] = pRead[i];
	} else {
		for (i = 0; i < readbytes; i += 2)
			ts->pFrame[i / 2] = pRead[i] + (pRead[i + 1] << 8);
	}

	*min = *max = ts->pFrame[0];

#ifdef DEBUG_MSG
	input_info(true, &ts->client->dev, "%s: 02X%02X%02X readbytes=%d\n", __func__,
			pRead[0], pRead[1], pRead[2], readbytes);
#endif
	sec_ts_print_frame(ts, min, max, selftest_type);

	temp = kzalloc(readbytes, GFP_KERNEL);
	if (!temp)
		goto ErrorRelease;

	memcpy(temp, ts->pFrame, ts->tx_count * ts->rx_count * 2);
	memset(ts->pFrame, 0x00, ts->tx_count * ts->rx_count * 2);

	for (i = 0; i < ts->tx_count; i++) {
		for (j = 0; j < ts->rx_count; j++)
			ts->pFrame[(j * ts->tx_count) + i] = temp[(i * ts->rx_count) + j];
	}

	kfree(temp);
ErrorRelease:
	kfree(pRead);

	return ret;
}


static int sec_ts_read_frame(struct sec_ts_data *ts, u8 type, short *min,
		short *max, bool save_result)
{
	unsigned int readbytes = 0xFF;
	unsigned char *pRead = NULL;
	u8 mode = TYPE_INVALID_DATA;
	int ret = 0;
	int i = 0;
	int j = 0;
	short *temp = NULL;

	input_raw_info(true, &ts->client->dev, "%s: type %d\n", __func__, type);

	/* set data length, allocation buffer memory */
	readbytes = ts->rx_count * ts->tx_count * 2;

	pRead = kzalloc(readbytes, GFP_KERNEL);
	if (!pRead)
		return -ENOMEM;

	/* set OPCODE and data type */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MUTU_RAW_TYPE, &type, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Set rawdata type failed\n", __func__);
		goto ErrorExit;
	}

	sec_ts_delay(50);

	/* read data */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_TOUCH_RAWDATA, pRead, readbytes);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read rawdata failed!\n", __func__);
		goto ErrorRelease;
	}

	memset(ts->pFrame, 0x00, readbytes);

	for (i = 0; i < readbytes; i += 2)
		ts->pFrame[i / 2] = (pRead[i + 1] + (pRead[i] << 8));

	*min = *max = ts->pFrame[0];

#ifdef DEBUG_MSG
	input_info(true, &ts->client->dev, "%s: 02X%02X%02X readbytes=%d\n", __func__,
			pRead[0], pRead[1], pRead[2], readbytes);
#endif
	sec_ts_print_frame(ts, min, max, TYPE_SELFTEST_INVALID_DATA);

	temp = kzalloc(readbytes, GFP_KERNEL);
	if (!temp)
		goto ErrorRelease;

	memcpy(temp, ts->pFrame, ts->tx_count * ts->rx_count * 2);
	memset(ts->pFrame, 0x00, ts->tx_count * ts->rx_count * 2);

	for (i = 0; i < ts->tx_count; i++) {
		for (j = 0; j < ts->rx_count; j++)
			ts->pFrame[(j * ts->tx_count) + i] = temp[(i * ts->rx_count) + j];
	}

	kfree(temp);

ErrorRelease:
	/* release data monitory (unprepare AFE data memory) */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MUTU_RAW_TYPE, &mode, 1);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Set rawdata type failed\n", __func__);

ErrorExit:
	kfree(pRead);

	return ret;
}

static int sec_ts_read_raw_data(struct sec_ts_data *ts,
		struct sec_cmd_data *sec, struct sec_ts_test_mode *mode)
{
	int ii;
	int ret = 0;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	char *buff;
	char *item_name = "NULL";

	switch (mode->type) {
	case TYPE_RAW_DATA:
		item_name = "CM_DELTA";
		break;
	case TYPE_OFFSET_DATA_SEC:
		item_name = "CM_OFFSET_SET";
		break;
	default:
		break;
	}

	buff = kzalloc(ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto error_alloc_mem;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto error_power_state;
	}

	input_raw_info(true, &ts->client->dev, "%s: %d, %s\n",
			__func__, mode->type, mode->allnode ? "ALL" : "");

	ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
				__func__);
		goto error_test_fail;
	}

	ret = sec_ts_read_frame(ts, mode->type, &mode->min, &mode->max, true);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}

	if (mode->allnode) {
		for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
			strncat(buff, temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);
		}
	} else {
		snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
	}

	ret = sec_ts_release_tmode(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to release tmode\n",
				__func__);
		goto error_test_fail;
	}

	if (!sec)
		goto out_rawdata;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN));

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff,
				strnlen(buff, ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN), item_name);

	sec->cmd_state = SEC_CMD_STATUS_OK;

out_rawdata:
	kfree(buff);

	sec_ts_locked_release_all_finger(ts);

	return ret;

error_test_fail:
error_power_state:
	kfree(buff);
error_alloc_mem:
	if (!sec)
		return ret;

	snprintf(temp, SEC_CMD_STR_LEN, "FAIL");
	sec_cmd_set_cmd_result(sec, temp, SEC_CMD_STR_LEN);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, temp, SEC_CMD_STR_LEN, item_name);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	sec_ts_locked_release_all_finger(ts);

	return ret;
}

static int sec_ts_selftest_read_raw_data(struct sec_ts_data *ts,
		struct sec_cmd_data *sec, struct sec_ts_test_mode *mode)
{
	int ii;
	int ret = 0;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	char *buff;
	char *item_name = "NULL";
	char rwbuf[2];
	int gap_para = 0;

	switch (mode->selftest_type) {
	case TYPE_SELFTEST_RAW_DATA:
		item_name = "SELF_RAW_MODULE";
		break;
	case TYPE_SELFTEST_RAW_VARIANCE_X:
		item_name = "SELF_RAW_SET_GAP_X";
		break;
	case TYPE_SELFTEST_RAW_VARIANCE_Y:
		item_name = "SELF_RAW_SET_GAP_Y";
		break;
	case TYPE_SELFTEST_NOISE_MAX_DATA:
		item_name = "SELF_RAW_SET_P2P";
		break;
	case TYPE_SELFTEST_OPEN_DATA:
		item_name = "OPEN";
		break;
	case TYPE_SELFTEST_SHORT_DATA:
		item_name = "SHORT";
		break;
	case TYPE_SELFTEST_HIGH_RESISTANCE:
		item_name = "HIGH_R";
		break;
	default:
		break;
	}

	buff = kzalloc(ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto error_alloc_mem;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto error_power_state;
	}

	input_raw_info(true, &ts->client->dev, "%s: %d, %s\n",
			__func__, mode->type, mode->allnode ? "ALL" : "");

	ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
				__func__);
		goto error_test_fail;
	}

	ret = sec_ts_selftest_read_frame(ts, mode->type, &mode->min, &mode->max, mode->selftest_type);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}

	if (mode->allnode) {
		if (mode->selftest_type == TYPE_SELFTEST_RAW_VARIANCE_X)
			gap_para = (ts->rx_count * ts->tx_count) - 32;
		else if (mode->selftest_type == TYPE_SELFTEST_RAW_VARIANCE_Y)
			gap_para = (ts->rx_count * ts->tx_count) - 18;
		else
			gap_para = (ts->rx_count * ts->tx_count);

		for (ii = 0; ii < gap_para; ii++) {
			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
			strlcat(buff, temp, ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);
		}
	} else {
		switch (mode->selftest_type) {
		case TYPE_SELFTEST_RAW_DATA:
			snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
			break;
		case TYPE_SELFTEST_RAW_VARIANCE_X:
			snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
			break;
		case TYPE_SELFTEST_RAW_VARIANCE_Y:
			snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
			break;
		/* check factory guide. The reason, as used below is to reduce the factory selftest time. */
		case TYPE_SELFTEST_NOISE_MIN_DATA:
			mode->noise_min = mode->min;
			break;
		case TYPE_SELFTEST_NOISE_MAX_DATA:
			snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->noise_min, mode->max);
			break;
		case TYPE_SELFTEST_OPEN_DATA:
			snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
			break;
		case TYPE_SELFTEST_SHORT_DATA:
			snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
			break;
		case TYPE_SELFTEST_HIGH_RESISTANCE:
			snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
			break;
		default:
			snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
			break;
		}
	}

	ret = sec_ts_release_tmode(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to release tmode\n",
				__func__);
		goto error_test_fail;
	}

	if (!sec)
		goto out_rawdata;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN));

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING && mode->selftest_type != TYPE_SELFTEST_NOISE_MIN_DATA )
		sec_cmd_set_cmd_result_all(sec, buff,
				strnlen(buff, ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN), item_name);

	sec->cmd_state = SEC_CMD_STATUS_OK;

out_rawdata:
	kfree(buff);

	sec_ts_locked_release_all_finger(ts);
	rwbuf[0] = 0x2;
	rwbuf[1] = 0x2;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, rwbuf, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: chage sysmode fail\n", __func__);
	sec_ts_delay(200);
	/*sec_ts_sw_reset(ts);*/

	return ret;

error_test_fail:
	ret = sec_ts_release_tmode(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to release tmode\n",
				__func__);
	}
error_power_state:
	kfree(buff);
error_alloc_mem:
	if (!sec)
		return ret;

	snprintf(temp, SEC_CMD_STR_LEN, "FAIL");
	sec_cmd_set_cmd_result(sec, temp, SEC_CMD_STR_LEN);
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, temp, SEC_CMD_STR_LEN, item_name);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	sec_ts_locked_release_all_finger(ts);
	rwbuf[0] = 0x2;
	rwbuf[1] = 0x2;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, rwbuf, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: chage sysmode fail\n", __func__);
	sec_ts_delay(200);
	/*sec_ts_sw_reset(ts);*/
	return ret;
}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "SE%02X%02X%02X",
			ts->plat_data->panel_revision, ts->plat_data->img_version_of_bin[2],
			ts->plat_data->img_version_of_bin[3]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_BIN");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	int ret;
	u8 fw_ver[4];

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_IMG_VERSION, fw_ver, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: firmware version read error\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	snprintf(buff, sizeof(buff), "SE%02X%02X%02X",
			ts->plat_data->panel_revision, fw_ver[2], fw_ver[3]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_config_ver(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[22] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s_SE_%02X%02X",
			ts->plat_data->model_name,
			ts->plat_data->config_version_of_ic[2], ts->plat_data->config_version_of_ic[3]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

#ifdef TCLM_CONCEPT
static void get_pat_information(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[50] = { 0 };

	sec_cmd_set_default_result(sec);

	/* fixed tune version will be saved at excute autotune */
	snprintf(buff, sizeof(buff), "C%02XT%04X.%4s%s%c%d%c%d%c%d",
		ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver, ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].f_name,
		(ts->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L " : " ",
		ts->tdata->cal_pos_hist_last3[0], ts->tdata->cal_pos_hist_last3[1],
		ts->tdata->cal_pos_hist_last3[2], ts->tdata->cal_pos_hist_last3[3],
		ts->tdata->cal_pos_hist_last3[4], ts->tdata->cal_pos_hist_last3[5]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void set_external_factory(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[22] = { 0 };

	sec_cmd_set_default_result(sec);

	ts->tdata->external_factory = true;
	snprintf(buff, sizeof(buff), "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}
#endif

static void get_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[20] = { 0 };
	char threshold[2] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		goto err;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_TOUCH_THRESHOLD, threshold, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read threshold fail!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		goto err;
	}

	input_info(true, &ts->client->dev, "0x%02X, 0x%02X\n",
			threshold[0], threshold[1]);

	snprintf(buff, sizeof(buff), "%d", (threshold[0] << 8) | threshold[1]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;
err:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	return;
}

static void get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };

	strlcpy(buff, "SEC", sizeof(buff));
	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_VENDOR");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };

	if (ts->plat_data->img_version_of_ic[0] == 0x02)
		strlcpy(buff, "MC44", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x05)
		strlcpy(buff, "A552", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x09)
		strlcpy(buff, "Y661", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x10)
		strlcpy(buff, "Y761", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x12)
		strlcpy(buff, "S6D7AT0", sizeof(buff));
	else
		strlcpy(buff, "N/A", sizeof(buff));

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_NAME");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void set_mis_cal_spec(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	char wreg[5] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->plat_data->mis_cal_check == 0) {
		input_err(true, &ts->client->dev, "%s: [ERROR] not support, %d\n", __func__);
		goto NG;
	} else if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		goto NG;
	} else {
		if ((sec->cmd_param[0] < 0 || sec->cmd_param[0] > 255) ||
				(sec->cmd_param[1] < 0 || sec->cmd_param[1] > 255) ||
				(sec->cmd_param[2] < 0 || sec->cmd_param[2] > 255)) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			goto NG;
		} else {
			wreg[0] = sec->cmd_param[0];
			wreg[1] = sec->cmd_param[1];
			wreg[2] = sec->cmd_param[2];

			ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MIS_CAL_SPEC, wreg, 3);
			if (ret < 0) {
				input_err(true, &ts->client->dev, "%s: nvm write failed. ret: %d\n", __func__, ret);
				goto NG;
			} else {
				input_info(true, &ts->client->dev, "%s: tx gap=%d, rx gap=%d, peak=%d\n", __func__, wreg[0], wreg[1], wreg[2]);
				sec_ts_delay(20);
			}
		}
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

NG:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;


}

/*
 *	## Mis Cal result ##
 *	FF : initial value in Firmware.
 *	FD : Cal fail case
 *	F4 : i2c fail case (5F)
 *	F3 : i2c fail case (5E)
 *	F2 : power off state
 *	F1 : not support mis cal concept
 *	F0 : initial value in fucntion
 *	08 : Ambient Ambient condition check(PEAK) result 0 (PASS), 1(FAIL)
 *	04 : Ambient Ambient condition check(DIFF MAX TX) result 0 (PASS), 1(FAIL)
 *	02 : Ambient Ambient condition check(DIFF MAX RX) result 0 (PASS), 1(FAIL)
 *	01 : Wet Wet mode result 0 (PASS), 1(FAIL)
 *	00 : Pass
 */
static void get_mis_cal_info(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	char mis_cal_data = 0xF0;
	char wreg[5] = { 0 };
	int ret;
	char buff_all[16] = { 0 };

	sec_cmd_set_default_result(sec);

	if (ts->plat_data->mis_cal_check == 0) {
		input_err(true, &ts->client->dev, "%s: [ERROR] not support, %d\n", __func__);
		mis_cal_data = 0xF1;
		goto NG;
	} else if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		mis_cal_data = 0xF2;
		goto NG;
	} else {
		ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_MIS_CAL_READ, &mis_cal_data, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: i2c fail!, %d\n", __func__, ret);
			mis_cal_data = 0xF3;
			goto NG;
		} else {
			input_info(true, &ts->client->dev, "%s: miss cal data : %d\n", __func__, mis_cal_data);
		}

		ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_MIS_CAL_SPEC, wreg, 3);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: i2c fail!, %d\n", __func__, ret);
			mis_cal_data = 0xF4;
			goto NG;
		} else {
			input_info(true, &ts->client->dev, "%s: miss cal spec : %d,%d,%d\n", __func__,
					wreg[0], wreg[1], wreg[2]);
		}
	}


	snprintf(buff, sizeof(buff), "%d", mis_cal_data);
	snprintf(buff_all, sizeof(buff_all), "%d,%d,%d,%d", mis_cal_data, wreg[0], wreg[1], wreg[2]);

	sec_cmd_set_cmd_result(sec, buff_all, strnlen(buff_all, sizeof(buff_all)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MIS_CAL");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff_all);
	return;

NG:
	snprintf(buff, sizeof(buff), "%d", mis_cal_data);
	snprintf(buff_all, sizeof(buff_all), "%d,%d,%d,%d", mis_cal_data, 0, 0, 0);
	sec_cmd_set_cmd_result(sec, buff_all, strnlen(buff_all, sizeof(buff_all)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MIS_CAL");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff_all);
}

static void get_wet_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	char wet_mode_info = 0;
	int ret;

	sec_cmd_set_default_result(sec);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_WET_MODE, &wet_mode_info, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c fail!, %d\n", __func__, ret);
		goto NG;
	}

	snprintf(buff, sizeof(buff), "%d", wet_mode_info);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "WET_MODE");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	return;

NG:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "WET_MODE");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

}

static void get_x_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%d", ts->tx_count);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_y_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%d", ts->rx_count);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_checksum_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	char csum_result[4] = { 0 };
	u8 temp = 0;
	u8 csum = 0;
	int ret, i;

	sec_cmd_set_default_result(sec);
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_info(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		goto err;
	}

	temp = DO_PARA_CHECKSUM;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_GET_CHECKSUM, &temp, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: send get_checksum_cmd fail!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "SendCMDfail");
		goto err;
	}

	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_read_bulk(ts, csum_result, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read get_checksum result fail!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "ReadCSUMfail");
		goto err;
	}

	for (i = 0; i < 4; i++)
		csum += csum_result[i];

	csum += temp;
	csum = ~csum;

	input_info(true, &ts->client->dev, "%s: checksum = %02X\n", __func__, csum);
	snprintf(buff, sizeof(buff), "%02X", csum);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return;

err:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	return;
}

static void run_reference_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SEC;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_reference_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SEC;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void get_reference(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = sec_ts_check_index(ts);
	if (node < 0)
		return;

	val = ts->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_RAW_DATA;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_RAW_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_gap_data_x_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_RAW_VARIANCE_X;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_gap_data_y_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_RAW_VARIANCE_Y;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_gap_data_x_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_RAW_VARIANCE_X;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_gap_data_y_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_RAW_VARIANCE_Y;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_noise_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_NOISE_MIN_DATA;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);

	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_NOISE_MAX_DATA;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_noise_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_NOISE_MIN_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);

	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_NOISE_MAX_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_open_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_OPEN_DATA;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_short_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_SHORT_DATA;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_open_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_OPEN_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_short_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_SHORT_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_high_resistance_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_HIGH_RESISTANCE;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void run_high_resistance_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.selftest_type = TYPE_SELFTEST_HIGH_RESISTANCE;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_selftest_read_raw_data(ts, sec, &mode);
}

static void get_rawcap(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = sec_ts_check_index(ts);
	if (node < 0)
		return;

	val = ts->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_delta_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_delta_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void get_delta(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = sec_ts_check_index(ts);
	if (node < 0)
		return;

	val = ts->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void run_decoded_raw_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_DECODED_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_delta_cm_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_REMV_AMB_DATA;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, sec, &mode);
}


/*
 * sec_ts_run_rawdata_all : read all raw data
 *
 * when you want to read full raw data (full_read : true)
 * "mutual/self 3, 5, 29, 1, 19" data will be saved in log
 *
 * otherwise, (full_read : false, especially on boot time)
 * only "mutual 3, 5, 29" data will be saved in log
 */
void sec_ts_run_rawdata_all(struct sec_ts_data *ts, bool full_read)
{
	short min, max;
	int ret, i, read_num;
	u8 test_type[5] = {TYPE_AMBIENT_DATA, TYPE_DECODED_DATA,
		TYPE_SIGNAL_DATA, TYPE_OFFSET_DATA_SEC, TYPE_OFFSET_DATA_SDC};

	ts->tsp_dump_lock = 1;
	input_raw_data_clear();
	input_raw_info(true, &ts->client->dev,
			"%s: start (noise:%d, wet:%d)##\n",
			__func__, ts->touch_noise_status, ts->wet_mode);

	ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
				__func__);
		goto out;
	}

	if (full_read) {
		read_num = 5;
	} else {
		read_num = 3;
		test_type[read_num - 1] = TYPE_OFFSET_DATA_SDC;
	}

	for (i = 0; i < read_num; i++) {
		if (test_type[i] != TYPE_OFFSET_DATA_SDC) {
			ret = sec_ts_read_frame(ts, test_type[i], &min, &max, false);
			if (ret < 0) {
				input_raw_info(true, &ts->client->dev,
						"%s: mutual %d : error ## ret:%d\n",
						__func__, test_type[i], ret);
				goto out;
			} else {
				input_raw_info(true, &ts->client->dev,
						"%s: mutual %d : Max/Min %d,%d ##\n",
						__func__, test_type[i], max, min);
			}
			sec_ts_delay(20);
		}
#ifdef MINORITY_REPORT
		if (test_type[i] == TYPE_AMBIENT_DATA) {
			minority_report_calculate_rawdata(ts);
		} else if (test_type[i] == TYPE_OFFSET_DATA_SDC) {
			/* selftest result for minority */
			/* 1byte result : High Resistance | Short | Open | Noise(max) | Noise(min) | */
			/* Raw Data Variance(Vertical) | Raw Data Variance(Horizontal) | Sensor Uniformity*/
			/* saved ito_test[0] by result */
			disable_irq(ts->client->irq);
			execute_selftest(ts, 0, true);
			enable_irq(ts->client->irq);
			execute_selftest_result_read(ts);
			minority_report_calculate_ito(ts);
			minority_report_sync_latest_value(ts);
		}
#endif
	}

	sec_ts_release_tmode(ts);

out:
	input_raw_info(true, &ts->client->dev, "%s: ito : %02X\n",
			__func__, ts->ito_test[0]);

	input_raw_info(true, &ts->client->dev, "%s: done (noise:%d, wet:%d)##\n",
			__func__, ts->touch_noise_status, ts->wet_mode);
	ts->tsp_dump_lock = 0;

	sec_ts_locked_release_all_finger(ts);
}

static void run_rawdata_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (ts->tsp_dump_lock == 1) {
		input_err(true, &ts->client->dev, "%s: already checking now\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: IC is power off\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	sec_ts_run_rawdata_all(ts, true);
	
	ret = sec_ts_sw_reset(ts, SEC_TS_CMD_SW_RESET);
	if (!ret) {
		input_err(true, &ts->client->dev, "%s: failed\n", __func__);
	}

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

int set_tsp_nvm_data_by_size(struct sec_ts_data *ts, u8 length, u8 *data)
{
	int ret;
	int rc;
	u8 buf = length;
	input_info(true, &ts->client->dev, "%s\n", __func__);

		/* SENSE OFF -> CELAR EVENT STACK -> READ NV -> SENSE ON */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_OFF, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write Sense_off\n", __func__);
		goto out_nvm;
	}

	input_dbg(false, &ts->client->dev, "%s: SENSE OFF\n", __func__);

	sec_ts_delay(100);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);
		goto out_nvm;
	}

	input_dbg(false, &ts->client->dev, "%s: CLEAR EVENT STACK\n", __func__);

	sec_ts_delay(100);

	sec_ts_locked_release_all_finger(ts);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM_SAVE, &buf, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c write nvm save failed\n", __func__);
		goto out_nvm;
	}

	sec_ts_delay(10);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM_WRITE, data, length);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm write failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	sec_ts_delay(200);

out_nvm:
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
		return rc;
	}

	sec_ts_delay(300);

	input_dbg(false, &ts->client->dev, "%s: SENSE ON\n", __func__);

	return ret;
}

int get_tsp_nvm_data_by_size(struct sec_ts_data *ts, int length, u8 *data)
{
	char *buff = NULL;
	int ret;
	int rc;
	u8 buf = length;

	buff = kzalloc(length, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	input_info(true, &ts->client->dev, "%s: length:%d, size:%d\n", __func__, length, sizeof(data));

	/* SENSE OFF -> CELAR EVENT STACK -> READ NV -> SENSE ON */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_OFF, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write Sense_off\n", __func__);
		goto out_nvm;
	}

	input_dbg(true, &ts->client->dev, "%s: SENSE OFF\n", __func__);

	sec_ts_delay(100);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);
		goto out_nvm;
	}

	input_dbg(true, &ts->client->dev, "%s: CLEAR EVENT STACK\n", __func__);

	sec_ts_delay(100);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM_SAVE, &buf, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c write NVM SAVE failed\n", __func__);
		goto out_nvm;
	}

	sec_ts_delay(100);

	sec_ts_locked_release_all_finger(ts);

	/* read NV data
	 * Use TSP NV area : in this model, use only one byte
	 */

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_NVM_READ, buff, length);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	memcpy(data, buff, length);

out_nvm:
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
		kfree(buff);
		return rc;
	}

	sec_ts_delay(300);

	kfree(buff);

	return ret;
}

int sec_tclm_data_read(struct i2c_client *client, int address)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);
	int ret = 0;
	int i = 0;
	u8 buff[4];
	u8 nbuff[SEC_TS_NVM_OFFSET_LENGTH];
	/* Lsi incell don't support index range. only supports from '0' to length */
	switch (address) {
	case SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER:
		sec_ts_delay(100);
		ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_IMG_VERSION, buff, 4);
		if (ret < 0)
			return ret;
		input_err(true, &ts->client->dev, "%s SEC_TS_READ_IMG_VERSION: %02X%02X\n",
			__func__, buff[2], buff[3]);
		ret = (buff[2] << 8) | buff[3];
		return ret;
	case SEC_TCLM_NVM_ALL_DATA:
		ret = get_tsp_nvm_data_by_size(ts, SEC_TS_NVM_OFFSET_LENGTH, nbuff);
		if (ret < 0)
			return ret;
		ts->tdata->nvdata.cal_count = nbuff[SEC_TS_NVM_OFFSET_CAL_COUNT];
		ts->tdata->nvdata.tune_fix_ver = (nbuff[SEC_TS_NVM_OFFSET_TUNE_VERSION] << 8) | nbuff[SEC_TS_NVM_OFFSET_TUNE_VERSION + 1];
		ts->tdata->nvdata.cal_position = nbuff[SEC_TS_NVM_OFFSET_CAL_POSITION];
		ts->tdata->nvdata.cal_pos_hist_cnt = nbuff[SEC_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT];
		ts->tdata->nvdata.cal_pos_hist_lastp = nbuff[SEC_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP];
		for (i = SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO; i < 2 * CAL_HISTORY_QUEUE_MAX + SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO; i++)
			ts->tdata->nvdata.cal_pos_hist_queue[i - SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO] = nbuff[i];

		input_err(true, &ts->client->dev, "%s:  %d %d %d %d\n", __func__,
			ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver,
			ts->tdata->nvdata.cal_pos_hist_cnt, ts->tdata->nvdata.cal_pos_hist_lastp);
		/* use only incell */
		ts->nv = nbuff[SEC_TS_NVM_OFFSET_FAC_RESULT];
		ts->disassemble_count = nbuff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT];
		return ret;
	default:
		return ret;
	}
}

int sec_tclm_data_write(struct i2c_client *client)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);
	int ret = 1;
	int i = 0;
	u8 nbuff[SEC_TS_NVM_OFFSET_LENGTH];

	memset(nbuff, 0x00, SEC_TS_NVM_OFFSET_LENGTH);
	/* use only incell */
	nbuff[SEC_TS_NVM_OFFSET_FAC_RESULT] = ts->nv;
	nbuff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT] = ts->disassemble_count;
	nbuff[SEC_TS_NVM_OFFSET_CAL_COUNT] = ts->tdata->nvdata.cal_count;
	nbuff[SEC_TS_NVM_OFFSET_TUNE_VERSION] = (u8)(ts->tdata->nvdata.tune_fix_ver >> 8);
	nbuff[SEC_TS_NVM_OFFSET_TUNE_VERSION + 1] = (u8)(0xff & ts->tdata->nvdata.tune_fix_ver);
	nbuff[SEC_TS_NVM_OFFSET_CAL_POSITION] = ts->tdata->nvdata.cal_position;
	nbuff[SEC_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT] = ts->tdata->nvdata.cal_pos_hist_cnt;
	nbuff[SEC_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP] = ts->tdata->nvdata.cal_pos_hist_lastp;
	for (i = SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO; i < 2 * CAL_HISTORY_QUEUE_MAX + SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO; i++)
		nbuff[i] = ts->tdata->nvdata.cal_pos_hist_queue[i - SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO];

	ret = set_tsp_nvm_data_by_size(ts, SEC_TS_NVM_OFFSET_LENGTH, nbuff);

	return ret;
}

#ifdef MINORITY_REPORT

/*	ts->defect_probability is FFFFF
 *
 *	0	is 100% normal.
 *	1~9	is normal, but need check
 *	A~E	is abnormal, must check
 *	F	is Device defect
 *
 *	F----	: ito
 *	-F---	: rawdata
 *	--A--	: crc
 *	---A-	: i2c_err
 *	----A	: wet
 */
void minority_report_calculate_rawdata(struct sec_ts_data *ts)
{
	int ii;
	int temp = 0;
	int max = -30000;
	int min = 30000;
	int node_gap = 1;

	if (!ts->tx_count) {
		ts->item_rawdata = 0xD;
		return;
	}

	for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
		if (max < ts->pFrame[ii])
			max = ts->pFrame[ii];
		if (min > ts->pFrame[ii])
			min = ts->pFrame[ii];

		if ((ii + 1) % (ts->tx_count) != 0) {
			temp = ts->pFrame[ii] - ts->pFrame[ii+1];
			if (temp < 0)
				temp = -temp;
			if (temp > node_gap)
				node_gap = temp;
		}

		if (ii < (ts->rx_count - 1) * ts->tx_count) {
			temp = ts->pFrame[ii] - ts->pFrame[ii + ts->tx_count];
			if (temp < 0)
				temp = -temp;
			if (temp > node_gap)
				node_gap = temp;
		}
	}

	if (max >= 80 || min <= -80)
		ts->item_rawdata = 0xF;
	else if ((max >= 50 || min <= -50) && (node_gap > 40))
		ts->item_rawdata = 0xC;
	else if (max >= 50 || min <= -50)
		ts->item_rawdata = 0xB;
	else if (node_gap > 40)
		ts->item_rawdata = 0xA;
	else if ((max >= 40 || min <= -40) && (node_gap > 30))
		ts->item_rawdata = 0x3;
	else if (max >= 40 || min <= -40)
		ts->item_rawdata = 0x2;
	else if (node_gap > 30)
		ts->item_rawdata = 0x1;
	else
		ts->item_rawdata = 0;

	input_info(true, &ts->client->dev, "%s min:%d,max:%d,node gap:%d =>%X\n",
			__func__, min, max, node_gap, ts->item_rawdata);

}

void minority_report_calculate_ito(struct sec_ts_data *ts)
{

	if (ts->ito_test[0])
		ts->item_ito = 0xF;
	else
		ts->item_ito = 0;
}

u8 minority_report_check_count(int value)
{
	u8 ret;

	if (value > 160)
		ret = 0xA;
	else if (value > 90)
		ret = 3;
	else if (value > 40)
		ret = 2;
	else if (value > 10)
		ret = 1;
	else
		ret = 0;

	return ret;
}

void minority_report_sync_latest_value(struct sec_ts_data *ts)
{
	u32 temp = 0;

	/* crc */
	if (ts->checksum_result == 1)
		ts->item_crc = 0xA;

	/* i2c_err */
	ts->item_i2c_err = minority_report_check_count(ts->comm_err_count);

	/* wet */
	ts->item_wet = minority_report_check_count(ts->wet_count);

	temp |= (ts->item_ito & 0xF) << 16;
	temp |= (ts->item_rawdata & 0xF) << 12;
	temp |= (ts->item_crc & 0xF) << 8;
	temp |= (ts->item_i2c_err & 0xF) << 4;
	temp |= (ts->item_wet & 0xF);

	ts->defect_probability = temp;
}
#endif

/* FACTORY TEST RESULT SAVING FUNCTION
 * bit 3 ~ 0 : OCTA Assy
 * bit 7 ~ 4 : OCTA module
 * param[0] : OCTA modue(1) / OCTA Assy(2)
 * param[1] : TEST NONE(0) / TEST FAIL(1) / TEST PASS(2) : 2 bit
 */

#define TEST_OCTA_MODULE	1
#define TEST_OCTA_ASSAY		2

#define TEST_OCTA_NONE		0
#define TEST_OCTA_FAIL		1
#define TEST_OCTA_PASS		2

static void set_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_result *result;
	const int nv_size = SEC_TS_NVM_OFFSET_FAC_RESULT + 1;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP_truned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = get_tsp_nvm_data_by_size(ts, nv_size, &buff[SEC_TS_NVM_OFFSET_FAC_RESULT]);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: [ERROR] get_tsp_nvm_data_by_size ret:%d\n",
				__func__, ret);
		goto error;
	}

	if (buff[SEC_TS_NVM_OFFSET_FAC_RESULT] == 0xFF)
		buff[SEC_TS_NVM_OFFSET_FAC_RESULT] = 0;

	result = (struct sec_ts_test_result *)&buff[SEC_TS_NVM_OFFSET_FAC_RESULT];

	if (sec->cmd_param[0] == TEST_OCTA_ASSAY) {
		result->assy_result = sec->cmd_param[1];
		if (result->assy_count < 3)
			result->assy_count++;
	}

	if (sec->cmd_param[0] == TEST_OCTA_MODULE) {
		result->module_result = sec->cmd_param[1];
		if (result->module_count < 3)
			result->module_count++;
	}

	input_info(true, &ts->client->dev, "%s: %d, %d, %d, %d, 0x%X\n", __func__,
			result->module_result, result->module_count,
			result->assy_result, result->assy_count, result->data[0]);

	input_info(true, &ts->client->dev, "%s: command (1)%X, (2)%X: %X\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1], *result->data);

	ret = set_tsp_nvm_data_by_size(ts, nv_size, result->data);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: [ERROR] set_tsp_nvm_data_by_size ret:%d\n",
				__func__, ret);
		goto error;
	}

	ts->nv = *result->data;
	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strlen(buff));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return;
error:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strlen(buff));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void get_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	const int nv_size = SEC_TS_NVM_OFFSET_FAC_RESULT + 1;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct sec_ts_test_result *result;
	int ret = 0;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP_truned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	memset(buff, 0x00, SEC_CMD_STR_LEN);

	ret = get_tsp_nvm_data_by_size(ts, nv_size, &buff[SEC_TS_NVM_OFFSET_FAC_RESULT]);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: [ERROR] get_tsp_nvm_data_by_size ret:%d\n",
				__func__, ret);
		goto error;
	}

	if (buff[SEC_TS_NVM_OFFSET_FAC_RESULT] == 0xFF) {
		buff[SEC_TS_NVM_OFFSET_FAC_RESULT] = 0;
	}

	ts->nv = buff[SEC_TS_NVM_OFFSET_FAC_RESULT];

	result = (struct sec_ts_test_result *)buff;

	input_info(true, &ts->client->dev, "%s: [0x%X][0x%X] M:%d, M:%d, A:%d, A:%d\n",
			__func__, *result->data, buff[SEC_TS_NVM_OFFSET_FAC_RESULT],
			result->module_result, result->module_count,
			result->assy_result, result->assy_count);

	snprintf(buff, sizeof(buff), "M:%s, M:%d, A:%s, A:%d",
			result->module_result == 0 ? "NONE" :
			result->module_result == 1 ? "FAIL" :
			result->module_result == 2 ? "PASS" : "A",
			result->module_count,
			result->assy_result == 0 ? "NONE" :
			result->assy_result == 1 ? "FAIL" :
			result->assy_result == 2 ? "PASS" : "A",
			result->assy_count);

	sec_cmd_set_cmd_result(sec, buff, strlen(buff));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return;
error:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strlen(buff));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void increase_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	const int nv_size = SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT + 1;
	u8 buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP_truned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = get_tsp_nvm_data_by_size(ts, nv_size, buff);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: [ERROR] get_tsp_nvm_data_by_size ret:%d\n",
				__func__, ret);
		goto error;
	}

	input_info(true, &ts->client->dev, "%s: disassemble count is #1 %d\n", __func__, buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT]);

	if (buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT] == 0xFF)
		buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT] = 0;

	if (buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT] < 0xFE)
		buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT]++;

	ts->disassemble_count= buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT];

	ret = set_tsp_nvm_data_by_size(ts, nv_size, buff);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: [ERROR] set_tsp_nvm_data_by_size ret:%d\n",
				__func__, ret);
		goto error;
	}

	input_info(true, &ts->client->dev, "%s: check disassemble count: %d\n",
			__func__, buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT]);

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strlen(buff));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return;
error:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strlen(buff));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void get_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	const int nv_size = SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT + 1;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP_truned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	memset(buff, 0x00, nv_size);

	ret = get_tsp_nvm_data_by_size(ts, nv_size, buff);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: [ERROR] get_tsp_nvm_data_by_size ret:%d\n",
				__func__, ret);
		goto error;
	}

	if (buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT] == 0xFF) {
		buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT] = 0;
		ts->disassemble_count = buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT];
		ret = set_tsp_nvm_data_by_size(ts, nv_size, buff);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: [ERROR] set_tsp_nvm_data_by_size ret:%d\n",
					__func__, ret);
			goto error;
		}
	}

	input_info(true, &ts->client->dev, "%s: read disassemble count: %d\n",
			__func__, buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT]);

	snprintf(buff, sizeof(buff), "%d", buff[SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT]);

	sec_cmd_set_cmd_result(sec, buff, strlen(buff));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return;
error:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strlen(buff));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void dead_zone_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	char data = 0;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		data = sec->cmd_param[0];

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_EDGE_DEADZONE, &data, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: failed to set deadzone\n", __func__);
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto err_set_dead_zone;
		}

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

err_set_dead_zone:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
};

static int execute_selftest_result_read(struct sec_ts_data *ts)
{
	int ret;
	u8 para = 0;
	u8 result;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST_READ, &para, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Set selftest_read failed\n", __func__);
		return ret;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SELFTEST_READ, &result, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read rawdata failed!\n", __func__);
		return ret;
	}

	ts->ito_test[0] = result;

	return ret;
}
static int execute_selftest(struct sec_ts_data *ts, u8 selfmode, u8 boot_flag)
{
	int rc;
	u8 tpara[3] = {1, 0x00 | selfmode, 0x03};

	input_info(true, &ts->client->dev, "%s: Self test start!\n", __func__);

	if (boot_flag) {
		tpara[0] = 3;
		tpara[1] = 0x00;
		tpara[2] = 0x03;
	}

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST_CHOICE, tpara, 2);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Send selftest cmd failed!\n", __func__);
		goto err_exit;
	}

	sec_ts_delay(50);

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST, &tpara[2], 1);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Send selftest cmd failed!\n", __func__);
		goto err_exit;
	}

	/*sec_ts_delay(350);*/

	rc = sec_ts_wait_for_ready(ts, SEC_TS_VENDOR_ACK_SELF_TEST_DONE);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}

	input_info(true, &ts->client->dev, "%s: Self test done!\n", __func__);

err_exit:
	return rc;
}

int sec_tclm_execute_force_calibration(struct i2c_client *client, int cal_mode)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);
	int rc;
	/* cal_mode is same tclm and sec. if they are different, modify source */
	rc = sec_ts_execute_force_calibration(ts, cal_mode);

	return rc;
}

int sec_ts_execute_force_calibration(struct sec_ts_data *ts, int cal_mode)
{
	int rc = -1;
	u8 cmd;

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, cal_mode);

	if (cal_mode == OFFSET_CAL_SEC)
		cmd = SEC_TS_CMD_FACTORY_PANELCALIBRATION;
	else
		return rc;

	if (ts->sec_ts_i2c_write(ts, cmd, NULL, 0) < 0) {
		input_err(true, &ts->client->dev, "%s: Write Cal commend failed!\n", __func__);
		return rc;
	}

	sec_ts_delay(1000);

	rc = sec_ts_wait_for_ready(ts, SEC_TS_VENDOR_ACK_OFFSET_CAL_DONE);

	return rc;
}

static void run_force_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	int rc;
	struct sec_ts_test_mode mode;
	char mis_cal_data = 0xF0;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out_force_cal_before_irq_ctrl;
	}

	if (ts->touch_count > 0) {
		snprintf(buff, sizeof(buff), "%s", "NG_FINGER_ON");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out_force_cal_before_irq_ctrl;
	}

	disable_irq(ts->client->irq);

	rc = sec_ts_execute_force_calibration(ts, OFFSET_CAL_SEC);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "FAIL");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out_force_cal;
	}

	sec_ts_sw_reset(ts, SEC_TS_CMD_SW_RESET);

	if (ts->plat_data->mis_cal_check) {
		buff[0] = 0;
		rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, buff, 1);
		if (rc < 0) {
			input_err(true, &ts->client->dev,
					"%s: mis_cal_check error[1] ret: %d\n", __func__, rc);
		}

		sec_ts_delay(100);

		buff[0] = 0x2;
		buff[1] = 0x2;
		rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, buff, 2);
		if (rc < 0) {
			input_err(true, &ts->client->dev,
					"%s: mis_cal_check error[2] ret: %d\n", __func__, rc);
		}

		sec_ts_delay(100);

		input_err(true, &ts->client->dev, "%s: try mis Cal. check\n", __func__);
		rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MIS_CAL_CHECK, NULL, 0);
		if (rc < 0) {
			input_err(true, &ts->client->dev,
					"%s: mis_cal_check error[3] ret: %d\n", __func__, rc);
		}

		rc = sec_ts_wait_for_ready(ts, SEC_TS_VENDOR_ACK_SELF_TEST_DONE); /* miscal check selt test done same */
		if (rc < 0) {
			input_err(true, &ts->client->dev, "%s: miscal check time out!\n", __func__);
		}

		rc = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_MIS_CAL_READ, &mis_cal_data, 1);
		if (rc < 0) {
			input_err(true, &ts->client->dev, "%s: i2c fail!, %d\n", __func__, rc);
			mis_cal_data = 0xF3;
		} else {
			input_info(true, &ts->client->dev, "%s: miss cal data : %d\n", __func__, mis_cal_data);
		}

		buff[0] = 1;
		rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, buff, 1);
		if (rc < 0) {
			input_err(true, &ts->client->dev,
					"%s: mis_cal_check error[4] ret: %d\n", __func__, rc);
		}

		if (mis_cal_data) {
			memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
			mode.type = TYPE_AMBIENT_DATA;
			mode.allnode = TEST_MODE_ALL_NODE;

			sec_ts_read_raw_data(ts, NULL, &mode);
			snprintf(buff, sizeof(buff), "%s", "MIS CAL");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out_force_cal;
		}
	}

#ifdef TCLM_CONCEPT
	/* devide tclm case */
	sec_tclm_case(ts->tdata, sec->cmd_param[0]);

	input_info(true, &ts->client->dev, "%s: param, %d, %c, %d\n", __func__,
		sec->cmd_param[0], sec->cmd_param[0], ts->tdata->root_of_calibration);

	rc = sec_execute_tclm_package(ts->tdata, 1);
	if (rc < 0) {
		input_err(true, &ts->client->dev,
					"%s: sec_execute_tclm_package\n", __func__);
	}

	sec_tclm_root_of_cal(ts->tdata, CALPOSITION_NONE);
#endif

	ts->cal_status = sec_ts_read_calibration_report(ts);
	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

out_force_cal:
	enable_irq(ts->client->irq);

out_force_cal_before_irq_ctrl:
	/* not to enter external factory mode without setting everytime */
	ts->tdata->external_factory = false;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

}

static void get_force_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	int rc;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	rc = sec_ts_read_calibration_report(ts);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "FAIL");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else if (rc == SEC_TS_STATUS_CALIBRATION_SEC) {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	} else if (rc == SEC_TS_STATUS_CALIBRATION_SDC) {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	} else {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

}

static void factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	int ret = 0;

	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	if (ts->tsp_dump_lock == 1) {
		input_err(true, &ts->client->dev, "%s: already checking now\n", __func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: IC is power off\n", __func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	snprintf(buff, sizeof(buff), "%d", ts->plat_data->item_version);
	sec_cmd_set_cmd_result_all(sec, buff, sizeof(buff), "ITEM_VERSION");

	get_chip_vendor(sec);
	get_chip_name(sec);
	get_fw_ver_bin(sec);
	get_fw_ver_ic(sec);
	
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed\n", __func__);
	}

	ret = sec_ts_sw_reset(ts, SEC_TS_CMD_SW_RESET);
	if (!ret) {
		input_err(true, &ts->client->dev, "%s: failed\n", __func__);
	}
	
	run_rawcap_read(sec);
	run_gap_data_x_read(sec);
	run_gap_data_y_read(sec);
	run_noise_read(sec);
	run_open_read(sec);
	run_short_read(sec);
	run_high_resistance_read(sec);

	get_mis_cal_info(sec);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

	ret = sec_ts_sw_reset(ts, SEC_TS_CMD_SW_RESET);
	if (!ret) {
		input_err(true, &ts->client->dev, "%s: failed\n", __func__);
	}
out:
	input_info(true, &ts->client->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static void set_wirelesscharger_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	bool mode;
	u8 w_data[1] = {0x00};

	sec_cmd_set_default_result(sec);

	if (!ts->plat_data->support_wirelesscharger_mode) {
		input_err(true, &ts->client->dev, "%s: not support\n", __func__);
		goto OUT;
	}


	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3)
		goto OUT;

	if (sec->cmd_param[0] == 0) {
		ts->charger_mode |= SEC_TS_BIT_CHARGER_MODE_NO;
		mode = false;
	} else {
		ts->charger_mode &= (~SEC_TS_BIT_CHARGER_MODE_NO);
		mode = true;
	}

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: fail to enable w-charger status, POWER_STATUS=OFF\n", __func__);
		goto NG;
	}

	if (sec->cmd_param[0] == 1)
		ts->charger_mode = ts->charger_mode | SEC_TS_BIT_CHARGER_MODE_WIRELESS_CHARGER;
	else if (sec->cmd_param[0] == 3)
		ts->charger_mode = ts->charger_mode | SEC_TS_BIT_CHARGER_MODE_WIRELESS_BATTERY_PACK;
	else if (mode == false)
		ts->charger_mode = ts->charger_mode & (~SEC_TS_BIT_CHARGER_MODE_WIRELESS_CHARGER) & (~SEC_TS_BIT_CHARGER_MODE_WIRELESS_BATTERY_PACK);

	w_data[0] = ts->charger_mode;
	ret = ts->sec_ts_i2c_write(ts, SET_TS_CMD_SET_CHARGER_MODE, w_data, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Failed to send command 74\n", __func__);
		goto NG;
	}

	input_err(true, &ts->client->dev, "%s: %s, status =%x\n",
			__func__, (mode) ? "wireless enable" : "wireless disable", ts->charger_mode);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

NG:
	input_err(true, &ts->client->dev, "%s: %s, status =%x\n",
			__func__, (mode) ? "wireless enable" : "wireless disable", ts->charger_mode);

OUT:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

/*
 *	flag     1  :  set edge handler
 *		2  :  set (portrait, normal) edge zone data
 *		4  :  set (portrait, normal) dead zone data
 *		8  :  set landscape mode data
 *		16 :  mode clear
 *	data
 *		0x30, FFF (y start), FFF (y end),  FF(direction)
 *		0x31, FFFF (edge zone)
 *		0x32, FF (up x), FF (down x), FFFF (y)
 *		0x33, FF (mode), FFF (edge), FFF (dead zone)
 *	case
 *		edge handler set :  0x30....
 *		booting time :  0x30...  + 0x31...
 *		normal mode : 0x32...  (+0x31...)
 *		landscape mode : 0x33...
 *		landscape -> normal (if same with old data) : 0x33, 0
 *		landscape -> normal (etc) : 0x32....  + 0x33, 0
 */

void set_grip_data_to_ic(struct sec_ts_data *ts, u8 flag)
{
	u8 data[8] = { 0 };

	input_info(true, &ts->client->dev, "%s: flag: %02X (clr,lan,nor,edg,han)\n", __func__, flag);

	if (flag & G_SET_EDGE_HANDLER) {
		if (ts->grip_edgehandler_direction == 0) {
			data[0] = 0x0;
			data[1] = 0x0;
			data[2] = 0x0;
			data[3] = 0x0;
		} else {
			data[0] = (ts->grip_edgehandler_start_y >> 4) & 0xFF;
			data[1] = (ts->grip_edgehandler_start_y << 4 & 0xF0) | ((ts->grip_edgehandler_end_y >> 8) & 0xF);
			data[2] = ts->grip_edgehandler_end_y & 0xFF;
			data[3] = ts->grip_edgehandler_direction & 0x3;
		}
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_EDGE_HANDLER, data, 4);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X,%02X,%02X,%02X\n",
				__func__, SEC_TS_CMD_EDGE_HANDLER, data[0], data[1], data[2], data[3]);
	}

	if (flag & G_SET_EDGE_ZONE) {
		data[0] = (ts->grip_edge_range >> 8) & 0xFF;
		data[1] = ts->grip_edge_range  & 0xFF;
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_EDGE_AREA, data, 2);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X,%02X\n",
				__func__, SEC_TS_CMD_EDGE_AREA, data[0], data[1]);
	}

	if (flag & G_SET_NORMAL_MODE) {
		data[0] = ts->grip_deadzone_up_x & 0xFF;
		data[1] = ts->grip_deadzone_dn_x & 0xFF;
		data[2] = (ts->grip_deadzone_y >> 8) & 0xFF;
		data[3] = ts->grip_deadzone_y & 0xFF;
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_DEAD_ZONE, data, 4);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X,%02X,%02X,%02X\n",
				__func__, SEC_TS_CMD_DEAD_ZONE, data[0], data[1], data[2], data[3]);
	}

	if (flag & G_SET_LANDSCAPE_MODE) {
		data[0] = ts->grip_landscape_mode & 0x1;
		data[1] = (ts->grip_landscape_edge >> 4) & 0xFF;
		data[2] = (ts->grip_landscape_edge << 4 & 0xF0) | ((ts->grip_landscape_deadzone >> 8) & 0xF);
		data[3] = ts->grip_landscape_deadzone & 0xFF;
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_LANDSCAPE_MODE, data, 4);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X,%02X,%02X,%02X\n",
				__func__, SEC_TS_CMD_LANDSCAPE_MODE, data[0], data[1], data[2], data[3]);
	}

	if (flag & G_CLR_LANDSCAPE_MODE) {
		data[0] = ts->grip_landscape_mode;
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_LANDSCAPE_MODE, data, 1);
		input_info(true, &ts->client->dev, "%s: 0x%02X %02X\n",
				__func__, SEC_TS_CMD_LANDSCAPE_MODE, data[0]);
	}
}

/*
 *	index  0 :  set edge handler
 *		1 :  portrait (normal) mode
 *		2 :  landscape mode
 *
 *	data
 *		0, X (direction), X (y start), X (y end)
 *		direction : 0 (off), 1 (left), 2 (right)
 *			ex) echo set_grip_data,0,2,600,900 > cmd
 *
 *		1, X (edge zone), X (dead zone up x), X (dead zone down x), X (dead zone y)
 *			ex) echo set_grip_data,1,200,10,50,1500 > cmd
 *
 *		2, 1 (landscape mode), X (edge zone), X (dead zone)
 *			ex) echo set_grip_data,2,1,200,100 > cmd
 *
 *		2, 0 (portrait mode)
 *			ex) echo set_grip_data,2,0  > cmd
 */

static void set_grip_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 mode = G_NONE;

	sec_cmd_set_default_result(sec);
	if (!ts->plat_data->support_girp) {
		input_err(true, &ts->client->dev, "%s: not support\n", __func__);
		goto out;
	}

	memset(buff, 0, sizeof(buff));

	mutex_lock(&ts->device_mutex);

	if (sec->cmd_param[0] == 0) {	/* edge handler */
		if (sec->cmd_param[1] == 0) {	/* clear */
			ts->grip_edgehandler_direction = 0;
		} else if (sec->cmd_param[1] < 3) {
			ts->grip_edgehandler_direction = sec->cmd_param[1];
			ts->grip_edgehandler_start_y = sec->cmd_param[2];
			ts->grip_edgehandler_end_y = sec->cmd_param[3];
		} else {
			input_err(true, &ts->client->dev, "%s: cmd1 is abnormal, %d (%d)\n",
					__func__, sec->cmd_param[1], __LINE__);
			goto err_grip_data;
		}

		mode = mode | G_SET_EDGE_HANDLER;
		set_grip_data_to_ic(ts, mode);

	} else if (sec->cmd_param[0] == 1) {	/* normal mode */
		if (ts->grip_edge_range != sec->cmd_param[1])
			mode = mode | G_SET_EDGE_ZONE;

		ts->grip_edge_range = sec->cmd_param[1];
		ts->grip_deadzone_up_x = sec->cmd_param[2];
		ts->grip_deadzone_dn_x = sec->cmd_param[3];
		ts->grip_deadzone_y = sec->cmd_param[4];
		mode = mode | G_SET_NORMAL_MODE;

		if (ts->grip_landscape_mode == 1) {
			ts->grip_landscape_mode = 0;
			mode = mode | G_CLR_LANDSCAPE_MODE;
		}
		set_grip_data_to_ic(ts, mode);
	} else if (sec->cmd_param[0] == 2) {	/* landscape mode */
		if (sec->cmd_param[1] == 0) {	/* normal mode */
			ts->grip_landscape_mode = 0;
			mode = mode | G_CLR_LANDSCAPE_MODE;
		} else if (sec->cmd_param[1] == 1) {
			ts->grip_landscape_mode = 1;
			ts->grip_landscape_edge = sec->cmd_param[2];
			ts->grip_landscape_deadzone	= sec->cmd_param[3];
			mode = mode | G_SET_LANDSCAPE_MODE;
		} else {
			input_err(true, &ts->client->dev, "%s: cmd1 is abnormal, %d (%d)\n",
					__func__, sec->cmd_param[1], __LINE__);
			goto err_grip_data;
		}
		set_grip_data_to_ic(ts, mode);
	} else {
		input_err(true, &ts->client->dev, "%s: cmd0 is abnormal, %d", __func__, sec->cmd_param[0]);
		goto err_grip_data;
	}

	mutex_unlock(&ts->device_mutex);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

err_grip_data:
	mutex_unlock(&ts->device_mutex);
out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

/*
 * Set/Get Dex Mode 0xE7
 *  0: Disable dex mode
 *  1: Full screen mode
 *  2: Iris mode
 */
static void dex_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (!ts->plat_data->support_dex) {
		input_err(true, &ts->client->dev, "%s: not support\n", __func__);
		goto NG;
	}

	if ((sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) &&
			(sec->cmd_param[1] < 0 || sec->cmd_param[1] > 1)) {
		input_err(true, &ts->client->dev, "%s: not support param\n", __func__);
		goto NG;
	}

	sec_ts_unlocked_release_all_finger(ts);

	ts->dex_mode = sec->cmd_param[0];
	if (ts->dex_mode) {
		input_err(true, &ts->client->dev, "%s: set DeX touch_pad mode%s\n",
				__func__, sec->cmd_param[1] ? " & Iris mode" : "");
		ts->input_dev = ts->input_dev_pad;
		if (sec->cmd_param[1]) {
			/* Iris mode */
			ts->dex_mode = 0x02;
			ts->dex_name = "[DeXI]";
		} else {
			ts->dex_name = "[DeX]";
		}
	} else {
		input_err(true, &ts->client->dev, "%s: set touch mode\n", __func__);
		ts->input_dev = ts->input_dev_touch;
		ts->dex_name = "";
	}

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		goto NG;
	}

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_DEX_MODE, &ts->dex_mode, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to set dex %smode\n", __func__,
				sec->cmd_param[1] ? "iris " : "");
		goto NG;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

NG:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void brush_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (!ts->plat_data->support_brush) {
		input_err(true, &ts->client->dev, "%s: not support\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	ts->brush_mode = sec->cmd_param[0];

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	input_info(true, &ts->client->dev,
			"%s: set brush mode %s\n", __func__, ts->brush_mode ? "enable" : "disable");

	/*	- 0: Disable Artcanvas min phi mode
	 *	- 1: Enable Artcanvas min phi mode
	 */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_BRUSH_MODE, &ts->brush_mode, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to set brush mode\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void set_touchable_area(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (!ts->plat_data->support_touchable_area) {
		input_err(true, &ts->client->dev, "%s: not support\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	ts->touchable_area = sec->cmd_param[0];

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	input_info(true, &ts->client->dev,
			"%s: set 16:9 mode %s\n", __func__, ts->touchable_area ? "enable" : "disable");

	/*  - 0: Disable 16:9 mode
	 *  - 1: Enable 16:9 mode
	 */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHABLE_AREA, &ts->touchable_area, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to set 16:9 mode\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void set_log_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char tBuff[2] = { 0 };
	u8 w_data[1] = {0x00};
	int ret;

	sec_cmd_set_default_result(sec);

	if (!ts->plat_data->support_log_level) {
		input_err(true, &ts->client->dev, "%s: not support\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	if ((sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) ||
			(sec->cmd_param[1] < 0 || sec->cmd_param[1] > 1) ||
			(sec->cmd_param[2] < 0 || sec->cmd_param[2] > 1) ||
			(sec->cmd_param[3] < 0 || sec->cmd_param[3] > 1) ||
			(sec->cmd_param[4] < 0 || sec->cmd_param[4] > 1) ||
			(sec->cmd_param[5] < 0 || sec->cmd_param[5] > 1) ||
			(sec->cmd_param[6] < 0 || sec->cmd_param[6] > 1) ||
			(sec->cmd_param[7] < 0 || sec->cmd_param[7] > 1)) {
		input_err(true, &ts->client->dev, "%s: para out of range\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "Para out of range");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_STATUS_EVENT_TYPE, tBuff, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Read Event type enable status fail\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "Read Stat Fail");
		goto err;
	}

	input_info(true, &ts->client->dev, "%s: STATUS_EVENT enable = 0x%02X, 0x%02X\n",
			__func__, tBuff[0], tBuff[1]);

	tBuff[0] = BIT_STATUS_EVENT_VENDOR_INFO(sec->cmd_param[6]);
	tBuff[1] = BIT_STATUS_EVENT_ERR(sec->cmd_param[0]) |
		BIT_STATUS_EVENT_INFO(sec->cmd_param[1]) |
		BIT_STATUS_EVENT_USER_INPUT(sec->cmd_param[2]);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATUS_EVENT_TYPE, tBuff, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Write Event type enable status fail\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "Write Stat Fail");
		goto err;
	}

	if (sec->cmd_param[0] == 1 && sec->cmd_param[1] == 1 &&
			sec->cmd_param[2] == 1 && sec->cmd_param[3] == 1 &&
			sec->cmd_param[4] == 1 && sec->cmd_param[5] == 1 &&
			sec->cmd_param[6] == 1 && sec->cmd_param[7] == 1) {
		w_data[0] = 0x1;
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_VENDOR_EVENT_LEVEL, w_data, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Write Vendor Event Level fail\n", __func__);
			snprintf(buff, sizeof(buff), "%s", "Write Stat Fail");
			goto err;
		}
	} else {
		w_data[0] = 0x0;
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_VENDOR_EVENT_LEVEL, w_data, 0);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Write Vendor Event Level fail\n", __func__);
			snprintf(buff, sizeof(buff), "%s", "Write Stat Fail");
			goto err;
		}
	}

	input_info(true, &ts->client->dev, "%s: ERROR : %d, INFO : %d, USER_INPUT : %d, INFO_SPONGE : %d, VENDOR_INFO : %d, VENDOR_EVENT_LEVEL : %d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1], sec->cmd_param[2], sec->cmd_param[5], sec->cmd_param[6], w_data[0]);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return;
err:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
}

static void debug(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	ts->debug_flag = sec->cmd_param[0];

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}


static void check_connection(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char *rbuff;
	u8 type = TYPE_OFFSET_DATA_SDC;
	u8 selftest_type = TYPE_SELFTEST_OPEN_DATA;
	short min, max;
	int i;
	int ret;
	char rwbuf[2];

	sec_cmd_set_default_result(sec);

	rbuff = kzalloc(ts->tx_count * ts->rx_count * 2, GFP_KERNEL);
	if (!rbuff)
		goto error_alloc_mem;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto error_power_state;
	}

	ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
				__func__);
		goto error_test_fail;
	}

	ret = sec_ts_selftest_read_frame(ts, type, &min, &max, selftest_type);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_selftest_fail;
	}

	for (i = 0; i < ts->tx_count * ts->rx_count; i++)
		if (ts->pFrame[i] != 0)
			goto error_selftest_fail;

	ret = sec_ts_release_tmode(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to release tmode\n",
				__func__);
		goto error_selftest_fail;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	rwbuf[0] = 0x2;
	rwbuf[1] = 0x2;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, rwbuf, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: chage sysmode fail\n", __func__);

	kfree(rbuff);
	return;

error_selftest_fail:
	ret = sec_ts_release_tmode(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to release tmode\n",
				__func__);
	}

error_test_fail:
error_power_state:
	kfree(rbuff);
error_alloc_mem:
	snprintf(buff, SEC_CMD_STR_LEN, "FAIL");
	sec_cmd_set_cmd_result(sec, buff, SEC_CMD_STR_LEN);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	rwbuf[0] = 0x2;
	rwbuf[1] = 0x2;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, rwbuf, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: chage sysmode fail\n", __func__);
}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%s", "NA");

	sec_cmd_set_cmd_result(sec, buff, strlen(buff));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);
}

int sec_ts_fn_init(struct sec_ts_data *ts)
{
	int retval;

	retval = sec_cmd_init(&ts->sec, sec_cmds,
			ARRAY_SIZE(sec_cmds), SEC_CLASS_DEVT_TSP);
	if (retval < 0) {
		input_err(true, &ts->client->dev,
				"%s: Failed to sec_cmd_init\n", __func__);
		goto exit;
	}

	retval = sysfs_create_group(&ts->sec.fac_dev->kobj,
			&cmd_attr_group);
	if (retval < 0) {
		input_err(true, &ts->client->dev,
				"%s: FTS Failed to create sysfs attributes\n", __func__);
		goto exit;
	}

	retval = sysfs_create_link(&ts->sec.fac_dev->kobj,
			&ts->input_dev->dev.kobj, "input");
	if (retval < 0) {
		input_err(true, &ts->client->dev,
				"%s: Failed to create input symbolic link\n",
				__func__);
		goto exit;
	}

	ts->reinit_done = true;

	return 0;

exit:
	return retval;
}

void sec_ts_fn_remove(struct sec_ts_data *ts)
{
	input_err(true, &ts->client->dev, "%s\n", __func__);

	sysfs_delete_link(&ts->sec.fac_dev->kobj, &ts->input_dev->dev.kobj, "input");

	sysfs_remove_group(&ts->sec.fac_dev->kobj,
			&cmd_attr_group);

	sec_cmd_exit(&ts->sec, SEC_CLASS_DEVT_TSP);

}
