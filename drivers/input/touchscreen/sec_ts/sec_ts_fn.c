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
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void set_mis_cal_spec(void *device_data);
static void get_mis_cal_info(void *device_data);
static void get_wet_mode(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void get_x_cross_routing(void *device_data);
static void get_y_cross_routing(void *device_data);
static void get_checksum_data(void *device_data);
static void run_reference_read(void *device_data);
static void run_reference_read_all(void *device_data);
static void get_reference(void *device_data);
static void run_rawcap_read(void *device_data);
static void run_rawcap_read_all(void *device_data);
static void get_rawcap(void *device_data);
static void run_delta_read(void *device_data);
static void run_delta_read_all(void *device_data);
static void run_decoded_raw_read_all(void *device_data);
static void run_delta_cm_read_all(void *device_data);
static void get_delta(void *device_data);
static void run_raw_p2p_read_all(void *device_data);
static void run_raw_p2p_min_read_all(void *device_data);
static void run_raw_p2p_max_read_all(void *device_data);
static void run_self_reference_read(void *device_data);
static void run_self_reference_read_all(void *device_data);
static void run_self_rawcap_read(void *device_data);
static void run_self_rawcap_read_all(void *device_data);
static void run_self_delta_read(void *device_data);
static void run_self_delta_read_all(void *device_data);
static void run_self_raw_p2p_min_read_all(void *device_data);
static void run_self_raw_p2p_max_read_all(void *device_data);
static void run_rawdata_read_all(void *device_data);
static void run_force_calibration(void *device_data);
static void get_force_calibration(void *device_data);
static void get_gap_data_x_all(void *device_data);
static void get_gap_data_y_all(void *device_data);
#ifdef USE_PRESSURE_SENSOR
static void run_force_pressure_calibration(void *device_data);
static void get_pressure_calibration_count(void *device_data);
static void set_pressure_test_mode(void *device_data);
static void run_pressure_filtered_strength_read_all(void *device_data);
static void run_pressure_strength_read_all(void *device_data);
static void run_pressure_rawdata_read_all(void *device_data);
static void run_pressure_offset_read_all(void *device_data);
static void set_pressure_strength(void *device_data);
static void set_pressure_rawdata(void *device_data);
static void set_pressure_data_index(void *device_data);
static void get_pressure_strength(void *device_data);
static void get_pressure_rawdata(void *device_data);
static void get_pressure_data_index(void *device_data);
static void set_pressure_strength_clear(void *device_data);
static void get_pressure_threshold(void *device_data);
static void set_pressure_user_level(void *device_data);
static void get_pressure_user_level(void *device_data);
static void set_pressure_setting_mode_enable(void *device_data);
static void run_pressure_jitter_p2p_read(void *device_data);
#endif
static void run_trx_short_test(void *device_data);
#ifdef TCLM_CONCEPT
static void set_external_factory(void *device_data);
static void get_pat_information(void *device_data);
#endif
static void set_tsp_test_result(void *device_data);
static void get_tsp_test_result(void *device_data);
static void clear_tsp_test_result(void *device_data);
static void run_lowpower_selftest(void *device_data);
static void increase_disassemble_count(void *device_data);
static void get_disassemble_count(void *device_data);
static void glove_mode(void *device_data);
static void clear_cover_mode(void *device_data);
static void dead_zone_enable(void *device_data);
static void drawing_test_enable(void *device_data);
static void set_lowpower_mode(void *device_data);
static void set_wirelesscharger_mode(void *device_data);
static void spay_enable(void *device_data);
static void set_aod_rect(void *device_data);
static void get_aod_rect(void *device_data);
static void aod_enable(void *device_data);
static void set_grip_data(void *device_data);
static void dex_enable(void *device_data);
static void brush_enable(void *device_data);
static void set_touchable_area(void *device_data);
static void set_log_level(void *device_data);
static void debug(void *device_data);
static void factory_cmd_result_all(void *device_data);
static void set_factory_level(void *device_data);
static void check_connection(void *device_data);
static void fix_active_mode(void *device_data);
static void touch_aging_mode(void *device_data);
static void not_support_cmd(void *device_data);

static int execute_selftest(struct sec_ts_data *ts, bool save_result);
static void sec_ts_print_frame(struct sec_ts_data *ts, short *min, short *max);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
extern int tui_force_close(uint32_t arg);
extern void tui_cover_mode_set(bool arg);
#endif

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("get_config_ver", get_config_ver),},
	{SEC_CMD("get_threshold", get_threshold),},
	{SEC_CMD("module_off_master", module_off_master),},
	{SEC_CMD("module_on_master", module_on_master),},
	{SEC_CMD("get_chip_vendor", get_chip_vendor),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("set_mis_cal_spec", set_mis_cal_spec),},
	{SEC_CMD("get_mis_cal_info", get_mis_cal_info),},
	{SEC_CMD("get_wet_mode", get_wet_mode),},
	{SEC_CMD("get_x_num", get_x_num),},
	{SEC_CMD("get_y_num", get_y_num),},
	{SEC_CMD("get_x_cross_routing", get_x_cross_routing),},
	{SEC_CMD("get_y_cross_routing", get_y_cross_routing),},
	{SEC_CMD("get_checksum_data", get_checksum_data),},
	{SEC_CMD("run_reference_read", run_reference_read),},
	{SEC_CMD("run_reference_read_all", run_reference_read_all),},
	{SEC_CMD("get_reference", get_reference),},
	{SEC_CMD("run_rawcap_read", run_rawcap_read),},
	{SEC_CMD("run_rawcap_read_all", run_rawcap_read_all),},
	{SEC_CMD("get_rawcap", get_rawcap),},
	{SEC_CMD("run_delta_read", run_delta_read),},
	{SEC_CMD("run_delta_read_all", run_delta_read_all),},
	{SEC_CMD("get_delta", get_delta),},
	{SEC_CMD("run_cs_raw_read_all", run_decoded_raw_read_all),},
	{SEC_CMD("run_cs_delta_read_all", run_delta_cm_read_all),},
	{SEC_CMD("run_raw_p2p_read_all", run_raw_p2p_read_all),},
	{SEC_CMD("run_raw_p2p_min_read_all", run_raw_p2p_min_read_all),},
	{SEC_CMD("run_raw_p2p_max_read_all", run_raw_p2p_max_read_all),},
	{SEC_CMD("run_self_reference_read", run_self_reference_read),},
	{SEC_CMD("run_self_reference_read_all", run_self_reference_read_all),},
	{SEC_CMD("run_self_rawcap_read", run_self_rawcap_read),},
	{SEC_CMD("run_self_rawcap_read_all", run_self_rawcap_read_all),},
	{SEC_CMD("run_self_delta_read", run_self_delta_read),},
	{SEC_CMD("run_self_delta_read_all", run_self_delta_read_all),},
	{SEC_CMD("run_self_raw_p2p_min_read_all", run_self_raw_p2p_min_read_all),},
	{SEC_CMD("run_self_raw_p2p_max_read_all", run_self_raw_p2p_max_read_all),},
	{SEC_CMD("run_rawdata_read_all_for_ghost", run_rawdata_read_all),},
	{SEC_CMD("run_force_calibration", run_force_calibration),},
	{SEC_CMD("get_force_calibration", get_force_calibration),},
	{SEC_CMD("get_gap_data_x_all", get_gap_data_x_all),},
	{SEC_CMD("get_gap_data_y_all", get_gap_data_y_all),},
#ifdef USE_PRESSURE_SENSOR
	{SEC_CMD("run_force_pressure_calibration", run_force_pressure_calibration),},
	{SEC_CMD("get_pressure_calibration_count", get_pressure_calibration_count),},
	{SEC_CMD("set_pressure_test_mode", set_pressure_test_mode),},
	{SEC_CMD("run_pressure_filtered_strength_read_all", run_pressure_filtered_strength_read_all),},
	{SEC_CMD("run_pressure_strength_read_all", run_pressure_strength_read_all),},
	{SEC_CMD("run_pressure_rawdata_read_all", run_pressure_rawdata_read_all),},
	{SEC_CMD("run_pressure_offset_read_all", run_pressure_offset_read_all),},
	{SEC_CMD("set_pressure_strength", set_pressure_strength),},
	{SEC_CMD("set_pressure_rawdata", set_pressure_rawdata),},
	{SEC_CMD("set_pressure_data_index", set_pressure_data_index),},
	{SEC_CMD("get_pressure_strength", get_pressure_strength),},
	{SEC_CMD("get_pressure_rawdata", get_pressure_rawdata),},
	{SEC_CMD("get_pressure_data_index", get_pressure_data_index),},
	{SEC_CMD("set_pressure_strength_clear", set_pressure_strength_clear),},
	{SEC_CMD("get_pressure_threshold", get_pressure_threshold),},
	{SEC_CMD("set_pressure_user_level", set_pressure_user_level),},
	{SEC_CMD("get_pressure_user_level", get_pressure_user_level),},
	{SEC_CMD("set_pressure_setting_mode_enable", set_pressure_setting_mode_enable),},
	{SEC_CMD("run_pressure_jitter_p2p_read", run_pressure_jitter_p2p_read),},
#endif
	{SEC_CMD("run_trx_short_test", run_trx_short_test),},
#ifdef TCLM_CONCEPT
	{SEC_CMD("get_pat_information", get_pat_information),},
	{SEC_CMD("set_external_factory", set_external_factory),},
#endif
	{SEC_CMD("set_tsp_test_result", set_tsp_test_result),},
	{SEC_CMD("get_tsp_test_result", get_tsp_test_result),},
	{SEC_CMD("clear_tsp_test_result", clear_tsp_test_result),},
	{SEC_CMD("run_lowpower_selftest", run_lowpower_selftest),},
	{SEC_CMD("increase_disassemble_count", increase_disassemble_count),},
	{SEC_CMD("get_disassemble_count", get_disassemble_count),},
	{SEC_CMD("glove_mode", glove_mode),},
	{SEC_CMD("clear_cover_mode", clear_cover_mode),},
	{SEC_CMD("dead_zone_enable", dead_zone_enable),},
	{SEC_CMD("drawing_test_enable", drawing_test_enable),},
	{SEC_CMD("set_lowpower_mode", set_lowpower_mode),},
	{SEC_CMD("set_wirelesscharger_mode", set_wirelesscharger_mode),},
	{SEC_CMD("spay_enable", spay_enable),},
	{SEC_CMD("set_aod_rect", set_aod_rect),},
	{SEC_CMD("get_aod_rect", get_aod_rect),},
	{SEC_CMD("aod_enable", aod_enable),},
	{SEC_CMD("set_grip_data", set_grip_data),},
	{SEC_CMD("dex_enable", dex_enable),},
	{SEC_CMD("brush_enable", brush_enable),},
	{SEC_CMD("set_touchable_area", set_touchable_area),},
	{SEC_CMD("set_log_level", set_log_level),},
	{SEC_CMD("debug", debug),},
	{SEC_CMD("factory_cmd_result_all", factory_cmd_result_all),},
	{SEC_CMD("set_factory_level", set_factory_level),},
	{SEC_CMD("check_connection", check_connection),},
	{SEC_CMD("fix_active_mode", fix_active_mode),},
	{SEC_CMD("touch_aging_mode", touch_aging_mode),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

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
	curr_time = realtime / USEC_PER_MSEC;

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

static ssize_t scrub_position_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[256] = { 0 };

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
	input_info(true, &ts->client->dev,
			"%s: scrub_id: %d\n", __func__, ts->scrub_id);
#else
	input_info(true, &ts->client->dev,
			"%s: scrub_id: %d, X:%d, Y:%d\n", __func__,
			ts->scrub_id, ts->scrub_x, ts->scrub_y);
#endif
	snprintf(buff, sizeof(buff), "%d %d %d", ts->scrub_id, ts->scrub_x, ts->scrub_y);

	ts->scrub_x = 0;
	ts->scrub_y = 0;

	return snprintf(buf, PAGE_SIZE, "%s", buff);
}

static DEVICE_ATTR(scrub_pos, 0444, scrub_position_show, NULL);

static ssize_t read_ito_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[256] = { 0 };

	input_info(true, &ts->client->dev, "%s: %02X%02X%02X%02X\n", __func__,
			ts->ito_test[0], ts->ito_test[1],
			ts->ito_test[2], ts->ito_test[3]);

	snprintf(buff, sizeof(buff), "%02X%02X%02X%02X",
			ts->ito_test[0], ts->ito_test[1],
			ts->ito_test[2], ts->ito_test[3]);

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
		strncat(buffer, temp, CMD_RESULT_WORD_LEN);

		memset(temp, 0x00, CMD_RESULT_WORD_LEN);
	}

	snprintf(temp, CMD_RESULT_WORD_LEN, "%d", ts->pFrame[ii]);
	strncat(buffer, temp, CMD_RESULT_WORD_LEN);

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
	ts->dive_count= 0;

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

	snprintf(buff, sizeof(buff), "SE%02X%02X%02X%02X%c%01X%02X%02X",
		ts->plat_data->panel_revision, ts->plat_data->img_version_of_bin[2],
		ts->plat_data->img_version_of_bin[3], ts->nv,
		ts->tdata->tclm_string[ts->tdata->cal_position].s_name, ts->tdata->cal_count & 0xF,
		ts->pressure_cal_base, ts->pressure_cal_delta);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t read_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	unsigned char buffer[10] = { 0 };

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

	input_info(true, &ts->client->dev, "%s: touch:%d, force:%d, aod:%d, spay:%d\n", __func__,
			ts->all_finger_count, ts->all_force_count,
			ts->all_aod_tap_count, ts->all_spay_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE,
			"\"TTCN\":\"%d\",\"TFCN\":\"%d\",\"TACN\":\"%d\",\"TSCN\":\"%d\"",
			ts->all_finger_count, ts->all_force_count,
			ts->all_aod_tap_count, ts->all_spay_count);
}

static ssize_t clear_all_touch_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->all_force_count = 0;
	ts->all_aod_tap_count = 0;
	ts->all_spay_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
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

	ts->max_z_value= 0;
	ts->min_z_value= 0xFFFFFFFF;
	ts->sum_z_value= 0;
	ts->all_finger_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_mode_change_failed_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__,
			ts->mode_change_failed_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d",
			ts->mode_change_failed_count);
}

static ssize_t clear_mode_change_failed_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->mode_change_failed_count= 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_irq_recovery_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__,
			ts->irq_recovery_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d",
			ts->irq_recovery_count);
}

static ssize_t clear_irq_recovery_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->irq_recovery_count = 0;

	input_info(true, &ts->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_ic_reset_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__,
			ts->ic_reset_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d",
			ts->ic_reset_count);
}

static ssize_t clear_ic_reset_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	ts->ic_reset_count = 0;

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

static ssize_t pressure_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[256] = { 0 };

	input_info(true, &ts->client->dev, "%s: force touch %s\n",
			__func__, ts->lowpower_mode & SEC_TS_MODE_SPONGE_FORCE_KEY ? "ON" : "OFF");

	if (ts->lowpower_mode & SEC_TS_MODE_SPONGE_FORCE_KEY)
		snprintf(buff, sizeof(buff), "1");
	else
		snprintf(buff, sizeof(buff), "0");

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);
}

/* Factory & game tools	: OFF value [0] / ON value [1]	*/
/* Settings				: OFF value [2] / ON value [3]	*/
static ssize_t pressure_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	int ret;
	unsigned long value = 0;

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret != 0)
		return ret;

	if (!ts->use_sponge)
		return -EINVAL;

	if (!ts->plat_data->support_pressure) {
		input_err(true, &ts->client->dev, "%s: Not support pressure[%d]\n",
				__func__, value);
		return -EINVAL;
	}

	input_info(true, &ts->client->dev, "%s: caller_id[%d]\n", __func__, value);

	if (value == 1 || value == 3) {
		ts->lowpower_mode |= SEC_TS_MODE_SPONGE_FORCE_KEY;
	} else if (value == 0 || value == 2) {
		ts->lowpower_mode &= ~SEC_TS_MODE_SPONGE_FORCE_KEY;
	} else {
		input_err(true, &ts->client->dev, "%s: Abnormal input value[%d]\n",
				__func__, value);
		return -EINVAL;
	}
	ts->pressure_caller_id = value;

	sec_ts_set_custom_library(ts);

	return count;
}

static ssize_t read_pressure_raw_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	/* need 22 char for each 'PRESSURE_CHANNEL_NUM * 4' data */
	char buff[PRESSURE_CHANNEL_NUM * 4 * 22] = {0};
	int data[4] = {TYPE_RAW_DATA, TYPE_SIGNAL_DATA,
		TYPE_REMV_AMB_DATA, TYPE_OFFSET_DATA_SEC};
	char loc[3] = {'R', 'C', 'L'};
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < PRESSURE_CHANNEL_NUM; j++) {
			char tmp[20] = {0};

			snprintf(tmp, sizeof(tmp), "\"TP%02d%c\":\"%d\"",
					data[i], loc[j], ts->pressure_data[data[i]][j]);
			strncat(buff, tmp, sizeof(tmp));
			if (i < 3 || j < PRESSURE_CHANNEL_NUM - 1)
				strncat(buff, ",", 2);
		}
	}
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t read_ambient_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: max: %d(%d,%d), min: %d(%d,%d)\n", __func__,
			ts->max_ambient, ts->max_ambient_channel_tx, ts->max_ambient_channel_rx,
			ts->min_ambient, ts->min_ambient_channel_tx, ts->min_ambient_channel_rx);

	return snprintf(buf, SEC_CMD_BUF_SIZE,
			"\"TAMB_MAX\":\"%d\",\"TAMB_MAX_TX\":\"%d\",\"TAMB_MAX_RX\":\"%d\","
			"\"TAMB_MIN\":\"%d\",\"TAMB_MIN_TX\":\"%d\",\"TAMB_MIN_RX\":\"%d\"",
			ts->max_ambient, ts->max_ambient_channel_tx, ts->max_ambient_channel_rx,
			ts->min_ambient, ts->min_ambient_channel_tx, ts->min_ambient_channel_rx);
}

static ssize_t read_ambient_channel_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char *buffer;
	int ret;
	int i;
	char temp[30];

	buffer = vmalloc((ts->tx_count + ts->rx_count) * 25);
	if (!buffer)
		return -ENOMEM;

	memset(buffer, 0x00, (ts->tx_count + ts->rx_count) * 25);

	for (i = 0; i < ts->tx_count; i++) {
		snprintf(temp, sizeof(temp), "\"TAMB_TX%02d\":\"%d\",",
				i, ts->ambient_tx[i]);
		strncat(buffer, temp, sizeof(temp));
	}

	for (i = 0; i < ts->rx_count; i++) {
		snprintf(temp, sizeof(temp), "\"TAMB_RX%02d\":\"%d\"",
				i, ts->ambient_rx[i]);
		strncat(buffer, temp, sizeof(temp));
		if (i  != (ts->rx_count - 1))
			strncat(buffer, ",", 2);
	}

	ret = snprintf(buf, (ts->tx_count + ts->rx_count) * 25, buffer);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buffer);
	vfree(buffer);

	return ret;
}

static ssize_t read_ambient_channel_delta_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char *buffer;
	int ret;
	int i;
	char temp[30];

	buffer = vmalloc((ts->tx_count + ts->rx_count) * 25);
	if (!buffer)
		return -ENOMEM;

	memset(buffer, 0x00, (ts->tx_count + ts->rx_count) * 25);

	for (i = 0; i < ts->tx_count; i++) {
		snprintf(temp, sizeof(temp), "\"TCDT%02d\":\"%d\",",
				i, ts->ambient_tx_delta[i]);
		strncat(buffer, temp, sizeof(temp));
	}

	for (i = 0; i < ts->rx_count; i++) {
		snprintf(temp, sizeof(temp), "\"TCDR%02d\":\"%d\"",
				i, ts->ambient_rx_delta[i]);
		strncat(buffer, temp, sizeof(temp));
		if (i  != (ts->rx_count - 1))
			strncat(buffer, ",", 2);
	}

	ret = snprintf(buf, (ts->tx_count + ts->rx_count) * 25, buffer);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buffer);
	vfree(buffer);

	return ret;
}

static ssize_t get_lp_dump(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	u8 string_data[8] = {0, };
	u16 current_index;
	int i, ret;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
	}

	if (ts->reset_is_on_going) {
		input_err(true, &ts->client->dev, "%s: Reset is ongoing!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "Reset is ongoing");
	}

	string_data[0] = SEC_TS_CMD_SPONGE_LP_DUMP & 0xFF;
	string_data[1] = (SEC_TS_CMD_SPONGE_LP_DUMP & 0xFF00) >> 8;

	disable_irq(ts->client->irq);

	ret = ts->sec_ts_read_sponge(ts, string_data, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Failed to read rect\n", __func__);
		snprintf(buf, SEC_CMD_BUF_SIZE, "NG, Failed to read rect");
		goto out;
	}

	current_index = (string_data[1] & 0xFF) << 8 | (string_data[0] & 0xFF);
	if (current_index > 1000 || current_index < 500) {
		input_err(true, &ts->client->dev,
				"Failed to Sponge LP log %d\n", current_index);
		snprintf(buf, SEC_CMD_BUF_SIZE,
				"NG, Failed to Sponge LP log, current_index=%d",
				current_index);
		goto out;
	}

	input_info(true, &ts->client->dev,
			"%s: DEBUG current_index = %d\n", __func__, current_index);

	/* sponge has 62 stacks for LP dump */
	for (i = 61; i >= 0; i--) {
		u16 data0, data1, data2, data3;
		char buff[30] = {0, };
		u16 string_addr;

		string_addr = current_index - (8 * i);
		if (string_addr < 500)
			string_addr += SEC_TS_CMD_SPONGE_LP_DUMP;
		string_data[0] = string_addr & 0xFF;
		string_data[1] = (string_addr & 0xFF00) >> 8;

		ret = ts->sec_ts_read_sponge(ts, string_data, 8);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: Failed to read rect\n", __func__);
			snprintf(buf, SEC_CMD_BUF_SIZE,
					"NG, Failed to read rect, addr=%d",
					string_addr);
			goto out;
		}

		data0 = (string_data[1] & 0xFF) << 8 | (string_data[0] & 0xFF);
		data1 = (string_data[3] & 0xFF) << 8 | (string_data[2] & 0xFF);
		data2 = (string_data[5] & 0xFF) << 8 | (string_data[4] & 0xFF);
		data3 = (string_data[7] & 0xFF) << 8 | (string_data[6] & 0xFF);
		if (data0 || data1 || data2 || data3) {
			snprintf(buff, sizeof(buff),
					"%d: %04x%04x%04x%04x\n",
					string_addr, data0, data1, data2, data3);
			strncat(buf, buff, sizeof(buff));
		}
	}

out:
	enable_irq(ts->client->irq);
	return strlen(buf);
}

static ssize_t get_force_recal_count(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	u8 rbuf[4] = {0, };
	u32 recal_count;
	int ret;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", -ENODEV);
	}

	if (ts->reset_is_on_going) {
		input_err(true, &ts->client->dev, "%s: Reset is ongoing!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", -EBUSY);
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_FORCE_RECAL_COUNT, rbuf, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: Failed to read\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", -EIO);
	}

	recal_count = (rbuf[0] & 0xFF) << 24 | (rbuf[1] & 0xFF) << 16 |
			(rbuf[2] & 0xFF) << 8 | (rbuf[3] & 0xFF);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", recal_count);
}

static ssize_t ic_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[512] = { 0 };
	char temp[128] = { 0 };
	u8 data[2] = { 0 };
	int ret;

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, data, 2);
	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);

	snprintf(temp, sizeof(temp), "mutual,%d,", data[0] & 0x01 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "hover,%d,", data[0] & 0x02 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "cover,%d,", data[0] & 0x04 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "glove,%d,", data[0] & 0x08 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "stylus,%d,", data[0] & 0x10 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "palm,%d,", data[0] & 0x20 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "wet,%d,", data[0] & 0x40 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "prox,%d,", data[0] & 0x80 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));

	memset(data, 0x00, 2);
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SET_POWER_MODE, data, 1);
	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);

	snprintf(temp, sizeof(temp), "npm,%d,", data[0] == 0 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "lpm,%d,", data[0] == 1 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "test,%d,", data[0] == 2 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "flash,%d,", data[0] == 3 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));

	memset(data, 0x00, 2);
	ret = ts->sec_ts_i2c_read(ts, SET_TS_CMD_SET_CHARGER_MODE, data, 1);
	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);

	snprintf(temp, sizeof(temp), "no_charge,%d,", data[0] == 0 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "wire_charge,%d,", data[0] == 1 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));
	snprintf(temp, sizeof(temp), "wireless_charge,%d,", data[0] == 2 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));

	memset(data, 0x00, 2);
	ret = ts->sec_ts_i2c_read(ts, SET_TS_CMD_SET_NOISE_MODE, data, 1);
	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);

	snprintf(temp, sizeof(temp), "noise,%d,", data[0]);
	strncat(buff, temp, sizeof(temp));

	memset(data, 0x00, 2);
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SET_COVERTYPE, data, 1);
	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);

	snprintf(temp, sizeof(temp), "cover_type,%d,", data[0]);
	strncat(buff, temp, sizeof(temp));

	memset(data, 0x00, 2);
	ret = ts->sec_ts_read_sponge(ts, data, 1);
	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);

	snprintf(temp, sizeof(temp), "pressure,%d,", data[0] & 0x40 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));

	snprintf(temp, sizeof(temp), "aod,%d,", data[0] & 0x04 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));

	snprintf(temp, sizeof(temp), "spay,%d,", data[0] & 0x02 ? 1 : 0);
	strncat(buff, temp, sizeof(temp));

	data[0] = 0;

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SET_DEX_MODE, data, 1);
	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);

	snprintf(temp, sizeof(temp), "dex,%d,", data[0]);
	strncat(buff, temp, sizeof(temp));

	data[0] = 0;

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SET_BRUSH_MODE, data, 1);
	if (ret < 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);

	snprintf(temp, sizeof(temp), "artcanvas,%d,", data[0]);
	strncat(buff, temp, sizeof(temp));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);
}

enum offset_fac_position {
	OFFSET_FAC_NOSAVE		= 0,	// FW index 0
	OFFSET_FAC_SUB			= 1,	// FW Index 2
	OFFSET_FAC_MAIN			= 2,	// FW Index 3
	OFFSET_FAC_SVC			= 3,	// FW Index 4
};

enum offset_fw_position {
	OFFSET_FW_NOSAVE		= 0,
	OFFSET_FW_SDC			= 1,
	OFFSET_FW_SUB			= 2,
	OFFSET_FW_MAIN			= 3,
	OFFSET_FW_SVC			= 4,
};

static int sec_ts_write_factory_level(struct sec_ts_data *ts, u8 pos)
{
	int ret = 0;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_FACTORY_LEVEL, &pos, 1);
	if (ret < 0)
		input_err(true, &ts->client->dev,
				"%s: failed to set factory level,%d\n", __func__, pos);

	sec_ts_delay(30);
	return ret;
}

static void set_factory_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		goto NG;
	}

	if (sec->cmd_param[0] < OFFSET_FAC_SUB || sec->cmd_param[0] > OFFSET_FAC_MAIN) {
		input_err(true, &ts->client->dev, "%s: cmd data is abnormal, %d\n", __func__, sec->cmd_param[0]);
		goto NG;
	}

	ts->factory_position = sec->cmd_param[0];
	ts->factory_level = true;

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
	int value;
	u16 avg, try_cnt, status;
	int max_node = 8 + ts->tx_count * ts->rx_count;
	char buff[6] = {0, };
	u16 temp;

	/* set Factory level */
	ret = sec_ts_write_factory_level(ts, position);
	if (ret < 0)
		goto err_exit;

	rBuff = kzalloc(max_node, GFP_KERNEL);
	if (!rBuff)
		goto err_mem;

	/* read full data */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_GET_CM_OFFSET_DATA, rBuff, max_node);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read rawdata failed!\n", __func__);
		goto err_i2c;
	}

	/* check Header */
	value = rBuff[3] << 24 | rBuff[2] << 16 | rBuff[1] << 8 | rBuff[0];
	input_info(true, &ts->client->dev, "%s: signature:%8X (0x59525446)\n", __func__, value);

	if (value != SEC_OFFSET_SIGNATURE) {
		input_err(true, &ts->client->dev, "%s: cmoffset[%d], signature is mismatched\n",
			__func__, position);
		snprintf(buf, SEC_CMD_BUF_SIZE, "Empty, error");
		goto err_invalid;
	}

	status = rBuff[4];
	try_cnt = rBuff[5];
	avg = rBuff[7] << 8 | rBuff[6];
	input_info(true, &ts->client->dev, "%s: cmoffset[%d], avg:%d, try_cnt:%d, status:%d\n",
		__func__, position, avg, try_cnt, status);

	for (i = 0; i < ts->rx_count; i++) {
		for (j = 0; j < ts->tx_count; j++) {
			temp = rBuff[8 + (j * ts->rx_count) + i];

			if (temp > 127)
				temp = avg + temp - 256;
			else
				temp = avg + temp;

			snprintf(buff, sizeof(buff), " %4x", temp);
			strncat(buf, buff, sizeof(buff));
		}
		snprintf(buff, sizeof(buff), "\n");
		strncat(buf, buff, sizeof(buff));
	}

	input_err(true, &ts->client->dev, "%s: total buf size:%d\n", __func__, strlen(buf));

err_invalid:
	sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);
	kfree(rBuff);
	return strlen(buf);

err_i2c:
	kfree(rBuff);
err_mem:
	sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);
err_exit:
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

	if (ts->reset_is_on_going) {
		input_err(true, &ts->client->dev, "%s: Reset is ongoing!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "Reset is ongoing");
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

	if (ts->reset_is_on_going) {
		input_err(true, &ts->client->dev, "%s: Reset is ongoing!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "Reset is ongoing");
	}

	return get_cmoffset_dump(ts, buf, OFFSET_FW_SUB);
}

static ssize_t get_cmoffset_main(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
	}

	if (ts->reset_is_on_going) {
		input_err(true, &ts->client->dev, "%s: Reset is ongoing!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "Reset is ongoing");
	}

	return get_cmoffset_dump(ts, buf, OFFSET_FW_MAIN);
}

#ifdef USE_PRESSURE_SENSOR
static ssize_t get_pressure_cfoffset_strength_all(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	u8 *rBuff;
	int i, ret;
	int cfoffset_max = 3 * 3;
	int strength_max = 3 * 5;
	char buff[16] = {0, };
	short temp;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
	}

	if (ts->reset_is_on_going) {
		input_err(true, &ts->client->dev, "%s: Reset is ongoing!\n", __func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "Reset is ongoing");
	}

	rBuff = kzalloc(strength_max * 2, GFP_KERNEL);
	if (!rBuff)
		goto err_exit;

	/* cf offset 18byte */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_GET_FORCE_CFOFFSET_DATA, rBuff, cfoffset_max * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read cfoffset failed!\n", __func__);
		snprintf(buf, SEC_CMD_BUF_SIZE, "NG, error");
		goto err_i2c;
	}

	memset(buff, 0x00, 16);
	snprintf(buff, sizeof(buff), "[CFOFFSET]");
	strncat(buf, buff, sizeof(buff));

	for (i = 0; i < cfoffset_max; i++) {
		temp = rBuff[2 * i + 1] | rBuff[2 * i] << 8;

		memset(buff, 0x00, 16);
		if (i % 3 == 0) {
			snprintf(buff, sizeof(buff), "\n");
			strncat(buf, buff, sizeof(buff));
			if (i / 3 == 0) {
				snprintf(buff, sizeof(buff), "SDC: ");
				strncat(buf, buff, sizeof(buff));
			} else if (i / 3 == 1) {
				snprintf(buff, sizeof(buff), "SUB: ");
				strncat(buf, buff, sizeof(buff));
			} else if (i / 3 == 2) {
				snprintf(buff, sizeof(buff), "MAI: ");
				strncat(buf, buff, sizeof(buff));
			}
		}

		memset(buff, 0x00, 16);
		snprintf(buff, sizeof(buff), "\t%d", temp);
		strncat(buf, buff, sizeof(buff));
	}

	memset(buff, 0x00, 16);
	snprintf(buff, sizeof(buff), "\n[STRENGTH]");
	strncat(buf, buff, sizeof(buff));

	/* strength 30byte */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_GET_FORCE_PRESSURE_DATA, rBuff, strength_max * 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read strength failed!\n", __func__);
		snprintf(buff, sizeof(buff), "\n NG");
		strncat(buf, buff, sizeof(buff));
		goto err_i2c;
	}

	for (i = 0; i < strength_max; i++) {
		temp = rBuff[2 * i + 1] | rBuff[2 * i] << 8;

		memset(buff, 0x00, 16);
		if (i % 3 == 0) {
			snprintf(buff, sizeof(buff), "\n");
			strncat(buf, buff, sizeof(buff));
			if (i / 3 == 0) {
				snprintf(buff, sizeof(buff), "LAT: ");
				strncat(buf, buff, sizeof(buff));
			} else if (i / 3 == 1) {
				snprintf(buff, sizeof(buff), "SDC: ");
				strncat(buf, buff, sizeof(buff));
			} else if (i / 3 == 2) {
				snprintf(buff, sizeof(buff), "SUB: ");
				strncat(buf, buff, sizeof(buff));
			} else if (i / 3 == 3) {
				snprintf(buff, sizeof(buff), "MAI: ");
				strncat(buf, buff, sizeof(buff));
			} else if (i / 3 == 4) {
				snprintf(buff, sizeof(buff), "SVC: ");
				strncat(buf, buff, sizeof(buff));
			}
		}

		snprintf(buff, sizeof(buff), "\t%d", temp);
		strncat(buf, buff, sizeof(buff));
	}

	snprintf(buff, sizeof(buff), "\n");
	strncat(buf, buff, sizeof(buff));

	input_err(true, &ts->client->dev, "%s: total buf size:%d\n", __func__, strlen(buf));

err_i2c:
	kfree(rBuff);
err_exit:
	return strlen(buf);
}
#endif

static ssize_t prox_power_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

	input_info(true, &ts->client->dev, "%s: %d\n", __func__,
			ts->prox_power_off);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%ld", ts->prox_power_off);
}

static ssize_t prox_power_off_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	long data;
	int ret;

	ret = kstrtol(buf, 10, &data);
	if (ret < 0)
		return ret;

	input_info(true, &ts->client->dev, "%s: %ld\n", __func__, data);

	ts->prox_power_off = data;

	return count;
}

static DEVICE_ATTR(ito_check, 0444, read_ito_check_show, NULL);
static DEVICE_ATTR(raw_check, 0444, read_raw_check_show, NULL);
static DEVICE_ATTR(multi_count, 0664, read_multi_count_show, clear_multi_count_store);
static DEVICE_ATTR(wet_mode, 0664, read_wet_mode_show, clear_wet_mode_store);
static DEVICE_ATTR(noise_mode, 0664, read_noise_mode_show, clear_noise_mode_store);
static DEVICE_ATTR(comm_err_count, 0664, read_comm_err_count_show, clear_comm_err_count_store);
static DEVICE_ATTR(checksum, 0664, read_checksum_show, clear_checksum_store);
static DEVICE_ATTR(holding_time, 0664, read_holding_time_show, clear_holding_time_store);
static DEVICE_ATTR(all_touch_count, 0664, read_all_touch_count_show, clear_all_touch_count_store);
static DEVICE_ATTR(z_value, 0664, read_z_value_show, clear_z_value_store);
static DEVICE_ATTR(mode_change_failed_count, 0664, read_mode_change_failed_count_show, clear_mode_change_failed_count_store);
static DEVICE_ATTR(irq_recovery_count, 0664, read_irq_recovery_count_show, clear_irq_recovery_count_store);
static DEVICE_ATTR(ic_reset_count, 0664, read_ic_reset_count_show, clear_ic_reset_count_store);
static DEVICE_ATTR(module_id, 0444, read_module_id_show, NULL);
static DEVICE_ATTR(vendor, 0444, read_vendor_show, NULL);
static DEVICE_ATTR(pressure_enable, 0664, pressure_enable_show, pressure_enable_store);
static DEVICE_ATTR(pressure_raw_check, 0444, read_pressure_raw_check_show, NULL);
static DEVICE_ATTR(read_ambient_info, 0444, read_ambient_info_show, NULL);
static DEVICE_ATTR(read_ambient_channel_info, 0444, read_ambient_channel_info_show, NULL);
static DEVICE_ATTR(read_ambient_channel_delta, 0444, read_ambient_channel_delta_show, NULL);
static DEVICE_ATTR(get_lp_dump, 0444, get_lp_dump, NULL);
static DEVICE_ATTR(force_recal_count, 0444, get_force_recal_count, NULL);
static DEVICE_ATTR(status, 0444, ic_status_show, NULL);
static DEVICE_ATTR(sensitivity_mode, 0664, sensitivity_mode_show, sensitivity_mode_store);
static DEVICE_ATTR(cmoffset_sdc, 0444, get_cmoffset_sdc, NULL);
static DEVICE_ATTR(cmoffset_sub, 0444, get_cmoffset_sub, NULL);
static DEVICE_ATTR(cmoffset_main, 0444, get_cmoffset_main, NULL);
#ifdef USE_PRESSURE_SENSOR
static DEVICE_ATTR(cfoffset_strength, 0444, get_pressure_cfoffset_strength_all, NULL);
#endif
static DEVICE_ATTR(prox_power_off, 0664, prox_power_off_show, prox_power_off_store);

static struct attribute *cmd_attributes[] = {
	&dev_attr_scrub_pos.attr,
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
	&dev_attr_mode_change_failed_count.attr,
	&dev_attr_irq_recovery_count.attr,
	&dev_attr_ic_reset_count.attr,
	&dev_attr_module_id.attr,
	&dev_attr_vendor.attr,
	&dev_attr_pressure_enable.attr,
	&dev_attr_pressure_raw_check.attr,
	&dev_attr_read_ambient_info.attr,
	&dev_attr_read_ambient_channel_info.attr,
	&dev_attr_read_ambient_channel_delta.attr,
	&dev_attr_get_lp_dump.attr,
	&dev_attr_force_recal_count.attr,
	&dev_attr_status.attr,
	&dev_attr_sensitivity_mode.attr,
	&dev_attr_cmoffset_sdc.attr,
	&dev_attr_cmoffset_sub.attr,
	&dev_attr_cmoffset_main.attr,
#ifdef USE_PRESSURE_SENSOR
	&dev_attr_cfoffset_strength.attr,
#endif
	&dev_attr_prox_power_off.attr,
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

int sec_ts_p2p_tmode(struct sec_ts_data *ts)
{
	int ret;
	u8 mode[2] = {0x0F, 0x11};

	input_info(true, &ts->client->dev, "%s\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_P2P_MODE, mode, sizeof(mode));
	sec_ts_delay(30);

	return ret;
}

static int execute_p2ptest(struct sec_ts_data *ts, u8 *p2p_data)
{
	int ret;
	u8 test[2] = {0x00, 0x64};
	u8 tBuff[6] = {0};
	int retry = 0;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_P2P_TEST, test, sizeof(test));
	sec_ts_delay(1700);

	ret = -1;

	while (ts->sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, tBuff, 6)) {
		if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_VENDOR_INFO) {
			if (tBuff[1] == SEC_TS_VENDOR_ACK_CMR_TEST_DONE) {
				p2p_data[0] = (tBuff[2] & 0xFF) << 8 | (tBuff[3] & 0xFF);
			} else if (tBuff[1] == SEC_TS_VENDOR_ACK_CSR_TEST_DONE) {
				p2p_data[1] = (tBuff[2] & 0xFF) << 8 | (tBuff[3] & 0xFF);
			} else if (tBuff[1] == SEC_TS_VENDOR_ACK_CFR_TEST_DONE) {
				ret = 0;
				break;
			}
		}

		if (retry++ > SEC_TS_WAIT_RETRY_CNT) {
			input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
			break;
		}
		sec_ts_delay(20);
	}
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

static void sec_ts_print_frame(struct sec_ts_data *ts, short *min, short *max)
{
	int i = 0;
	int j = 0;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };

	input_raw_info(true, &ts->client->dev, "%s\n", __func__);

	pStr = kzalloc(6 * (ts->tx_count + 1), GFP_KERNEL);
	if (pStr == NULL)
		return;

	memset(pStr, 0x0, 6 * (ts->tx_count + 1));
	snprintf(pTmp, sizeof(pTmp), "      TX");
	strncat(pStr, pTmp, 6 * ts->tx_count);

	for (i = 0; i < ts->tx_count; i++) {
		snprintf(pTmp, sizeof(pTmp), " %02d ", i);
		strncat(pStr, pTmp, 6 * ts->tx_count);
	}

	input_raw_info(true, &ts->client->dev, "%s\n", pStr);
	memset(pStr, 0x0, 6 * (ts->tx_count + 1));
	snprintf(pTmp, sizeof(pTmp), " +");
	strncat(pStr, pTmp, 6 * ts->tx_count);

	for (i = 0; i < ts->tx_count; i++) {
		snprintf(pTmp, sizeof(pTmp), "----");
		strncat(pStr, pTmp, 6 * ts->rx_count);
	}

	input_raw_info(true, &ts->client->dev, "%s\n", pStr);

	for (i = 0; i < ts->rx_count; i++) {
		memset(pStr, 0x0, 6 * (ts->tx_count + 1));
		snprintf(pTmp, sizeof(pTmp), "Rx%02d | ", i);
		strncat(pStr, pTmp, 6 * ts->tx_count);

		for (j = 0; j < ts->tx_count; j++) {
			snprintf(pTmp, sizeof(pTmp), " %3d", ts->pFrame[(j * ts->rx_count) + i]);

			if (ts->pFrame[(j * ts->rx_count) + i] < *min)
				*min = ts->pFrame[(j * ts->rx_count) + i];

			if (ts->pFrame[(j * ts->rx_count) + i] > *max)
				*max = ts->pFrame[(j * ts->rx_count) + i];

			strncat(pStr, pTmp, 6 * ts->rx_count);
		}
		input_raw_info(true, &ts->client->dev, "%s\n", pStr);
	}
	kfree(pStr);
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

	if (type == TYPE_OFFSET_DATA_SDC) {
		/* excute selftest for real cap offset data, because real cap data is not memory data in normal touch. */
		char para = TO_TOUCH_MODE;

		disable_irq(ts->client->irq);

		ret = execute_selftest(ts, save_result);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: execute_selftest failed\n", __func__);
			enable_irq(ts->client->irq);
			goto ErrorRelease;
		}

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Set rawdata type failed\n", __func__);
			enable_irq(ts->client->irq);
			goto ErrorRelease;
		}

		enable_irq(ts->client->irq);
	}

	/* read data */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_TOUCH_RAWDATA, pRead, readbytes);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read rawdata failed!\n", __func__);
		goto ErrorRelease;
	}

	memset(ts->pFrame, 0x00, readbytes);

	for (i = 0; i < readbytes; i += 2)
		ts->pFrame[i / 2] = pRead[i + 1] + (pRead[i] << 8);

	*min = *max = ts->pFrame[0];

#ifdef DEBUG_MSG
	input_info(true, &ts->client->dev, "%s: 02X%02X%02X readbytes=%d\n", __func__,
			pRead[0], pRead[1], pRead[2], readbytes);
#endif
	sec_ts_print_frame(ts, min, max);

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

static void sec_ts_print_channel(struct sec_ts_data *ts)
{
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	int i = 0, j = 0, k = 0;

	if (!ts->tx_count)
		return;

	pStr = vzalloc(7 * (ts->tx_count + 1));
	if (!pStr)
		return;

	memset(pStr, 0x0, 7 * (ts->tx_count + 1));
	snprintf(pTmp, sizeof(pTmp), " TX");
	strncat(pStr, pTmp, 7 * ts->tx_count);

	for (k = 0; k < ts->tx_count; k++) {
		snprintf(pTmp, sizeof(pTmp), "    %02d", k);
		strncat(pStr, pTmp, 7 * ts->tx_count);
	}
	input_raw_info(true, &ts->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, 7 * (ts->tx_count + 1));
	snprintf(pTmp, sizeof(pTmp), " +");
	strncat(pStr, pTmp, 7 * ts->tx_count);

	for (k = 0; k < ts->tx_count; k++) {
		snprintf(pTmp, sizeof(pTmp), "------");
		strncat(pStr, pTmp, 7 * ts->tx_count);
	}
	input_raw_info(true, &ts->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, 7 * (ts->tx_count + 1));
	snprintf(pTmp, sizeof(pTmp), " | ");
	strncat(pStr, pTmp, 7 * ts->tx_count);

	for (i = 0; i < (ts->tx_count + ts->rx_count) * 2; i += 2) {
		if (j == ts->tx_count) {
			input_raw_info(true, &ts->client->dev, "%s\n", pStr);
			input_raw_info(true, &ts->client->dev, "\n");
			memset(pStr, 0x0, 7 * (ts->tx_count + 1));
			snprintf(pTmp, sizeof(pTmp), " RX");
			strncat(pStr, pTmp, 7 * ts->tx_count);

			for (k = 0; k < ts->tx_count; k++) {
				snprintf(pTmp, sizeof(pTmp), "    %02d", k);
				strncat(pStr, pTmp, 7 * ts->tx_count);
			}

			input_raw_info(true, &ts->client->dev, "%s\n", pStr);

			memset(pStr, 0x0, 7 * (ts->tx_count + 1));
			snprintf(pTmp, sizeof(pTmp), " +");
			strncat(pStr, pTmp, 7 * ts->tx_count);

			for (k = 0; k < ts->tx_count; k++) {
				snprintf(pTmp, sizeof(pTmp), "------");
				strncat(pStr, pTmp, 7 * ts->tx_count);
			}
			input_raw_info(true, &ts->client->dev, "%s\n", pStr);

			memset(pStr, 0x0, 7 * (ts->tx_count + 1));
			snprintf(pTmp, sizeof(pTmp), " | ");
			strncat(pStr, pTmp, 7 * ts->tx_count);
		} else if (j && !(j % ts->tx_count)) {
			input_raw_info(true, &ts->client->dev, "%s\n", pStr);
			memset(pStr, 0x0, 7 * (ts->tx_count + 1));
			snprintf(pTmp, sizeof(pTmp), " | ");
			strncat(pStr, pTmp, 7 * ts->tx_count);
		}

		snprintf(pTmp, sizeof(pTmp), " %5d", ts->pFrame[j]);
		strncat(pStr, pTmp, 7 * ts->tx_count);

		j++;
	}
	input_raw_info(true, &ts->client->dev, "%s\n", pStr);
	vfree(pStr);
}

static int sec_ts_read_channel(struct sec_ts_data *ts, u8 type,
				short *min, short *max, bool save_result)
{
	unsigned char *pRead = NULL;
	u8 mode = TYPE_INVALID_DATA;
	int ret = 0;
	int ii = 0;
	int jj = 0;
	unsigned int data_length = (ts->tx_count + ts->rx_count) * 2;
	u8 w_data;

	input_raw_info(true, &ts->client->dev, "%s: type %d\n", __func__, type);

	pRead = kzalloc(data_length, GFP_KERNEL);
	if (!pRead)
		return -ENOMEM;

	/* set OPCODE and data type */
	w_data = type;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELF_RAW_TYPE, &w_data, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Set rawdata type failed\n", __func__);
		goto out_read_channel;
	}

	sec_ts_delay(50);

	if (type == TYPE_OFFSET_DATA_SDC) {
		/* execute selftest for real cap offset data,
		 * because real cap data is not memory data in normal touch.
		 */
		char para = TO_TOUCH_MODE;
		disable_irq(ts->client->irq);
		ret = execute_selftest(ts, save_result);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: execute_selftest failed!\n", __func__);
			enable_irq(ts->client->irq);
			goto err_read_data;
		}
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: set rawdata type failed!\n", __func__);
			enable_irq(ts->client->irq);
			goto err_read_data;
		}
		enable_irq(ts->client->irq);
		/* end */
	}
	/* read data */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_TOUCH_SELF_RAWDATA, pRead, data_length);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read rawdata failed!\n", __func__);
		goto err_read_data;
	}

	/* clear all pFrame data */
	memset(ts->pFrame, 0x00, data_length);

	for (ii = 0; ii < data_length; ii += 2) {
		ts->pFrame[jj] = ((pRead[ii] << 8) | pRead[ii + 1]);

		if (ii == 0)
			*min = *max = ts->pFrame[jj];

		*min = min(*min, ts->pFrame[jj]);
		*max = max(*max, ts->pFrame[jj]);

		jj++;
	}

	sec_ts_print_channel(ts);

err_read_data:
	/* release data monitory (unprepare AFE data memory) */
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELF_RAW_TYPE, &mode, 1);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Set rawdata type failed\n", __func__);

out_read_channel:
	kfree(pRead);

	return ret;
}

static int get_gap_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	int ii;
	int node_gap_tx = 0;
	int node_gap_rx = 0;
	int tx_max = 0;
	int rx_max = 0;

	for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
		if ((ii + 1) % (ts->tx_count) != 0) {
			if (ts->pFrame[ii] > ts->pFrame[ii + 1])
				node_gap_tx = 100 - (ts->pFrame[ii + 1] * 100 / ts->pFrame[ii]);
			else
				node_gap_tx = 100 - (ts->pFrame[ii] * 100 / ts->pFrame[ii + 1]);
			tx_max = max(tx_max, node_gap_tx);
		}

		if (ii < (ts->rx_count - 1) * ts->tx_count) {
			if (ts->pFrame[ii] > ts->pFrame[ii + ts->tx_count])
				node_gap_rx = 100 - (ts->pFrame[ii + ts->tx_count] * 100 / ts->pFrame[ii]);
			else
				node_gap_rx = 100 - (ts->pFrame[ii] * 100 / ts->pFrame[ii + ts->tx_count]);
			rx_max = max(rx_max, node_gap_rx);
		}
	}

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		snprintf(buff, sizeof(buff), "%d,%d", 0, tx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "GAP_DATA_X");
		snprintf(buff, sizeof(buff), "%d,%d", 0, rx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "GAP_DATA_Y");
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	return 0;
}

static void get_gap_data_x_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char *buff = NULL;
	int ii;
	int node_gap = 0;
	char temp[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	buff = kzalloc(ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		return;

	for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
		if ((ii + 1) % (ts->tx_count) != 0) {
			if (ts->pFrame[ii] > ts->pFrame[ii + 1])
				node_gap = 100 - (ts->pFrame[ii + 1] * 100 / ts->pFrame[ii]);
			else
				node_gap = 100 - (ts->pFrame[ii] * 100 / ts->pFrame[ii + 1]);

			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", node_gap);
			strncat(buff, temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void get_gap_data_y_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char *buff = NULL;
	int ii;
	int node_gap = 0;
	char temp[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	buff = kzalloc(ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		return;

	for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
		if (ii < (ts->rx_count - 1) * ts->tx_count) {
			if (ts->pFrame[ii] > ts->pFrame[ii + ts->tx_count])
				node_gap = 100 - (ts->pFrame[ii + ts->tx_count] * 100 / ts->pFrame[ii]);
			else
				node_gap = 100 - (ts->pFrame[ii] * 100 / ts->pFrame[ii + ts->tx_count]);

			snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", node_gap);
			strncat(buff, temp, CMD_RESULT_WORD_LEN);
			memset(temp, 0x00, SEC_CMD_STR_LEN);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static int get_self_channel_data(void *device_data, u8 type)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	int ii;
	short tx_min = 0;
	short rx_min = 0;
	short tx_max = 0;
	short rx_max = 0;
	char *item_name = "NULL";
	char temp[SEC_CMD_STR_LEN] = { 0 };

	switch (type) {
	case TYPE_OFFSET_DATA_SDC:
		item_name = "SELF_OFFSET_MODULE";
		break;
	case TYPE_RAW_DATA:
		item_name = "SELF_DELTA";
		break;
	case TYPE_OFFSET_DATA_SEC:
		item_name = "SELF_OFFSET_SET";
		break;
	default:
		break;
	}

	for (ii = 0; ii < ts->tx_count; ii++) {
		if (ii == 0)
			tx_min = tx_max = ts->pFrame[ii];

		tx_min = min(tx_min, ts->pFrame[ii]);
		tx_max = max(tx_max, ts->pFrame[ii]);
	}
	snprintf(buff, sizeof(buff), "%d,%d", tx_min, tx_max);
	snprintf(temp, sizeof(temp), "%s%s", item_name, "_X");
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, sizeof(buff), temp);

	memset(temp, 0x00, SEC_CMD_STR_LEN);

	for (ii = ts->tx_count; ii < ts->tx_count + ts->rx_count; ii++) {
		if (ii == ts->tx_count)
			rx_min = rx_max = ts->pFrame[ii];

		rx_min = min(rx_min, ts->pFrame[ii]);
		rx_max = max(rx_max, ts->pFrame[ii]);
	}
	snprintf(buff, sizeof(buff), "%d,%d", rx_min, rx_max);
	snprintf(temp, sizeof(temp), "%s%s", item_name, "_Y");
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, sizeof(buff), temp);

	return 0;
}

static int get_self_p2p_diff_data(struct sec_cmd_data *sec, struct sec_ts_data *ts,
	short *p2p_min, short *p2p_max)
{
	char buff[16] = { 0 };
	int ii;
	short node_gap = 0;
	short tx_max = 0;
	short rx_max = 0;

	for (ii = 0; ii < ts->tx_count; ii++) {
		if (ii == 0)
			tx_max = p2p_max[ii] - p2p_min[ii];

		node_gap = p2p_max[ii] - p2p_min[ii];
		tx_max = max(tx_max, node_gap);
	}
	for (ii = ts->tx_count; ii < ts->tx_count + ts->rx_count; ii++) {
		if (ii == ts->tx_count)
			rx_max = p2p_max[ii] - p2p_min[ii];

		node_gap = p2p_max[ii] - p2p_min[ii];
		rx_max = max(rx_max, node_gap);
	}

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		snprintf(buff, sizeof(buff), "%d,%d", 0, tx_max);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_SET_P2P_X_DIFF");
		snprintf(buff, sizeof(buff), "%d,%d", 0, rx_max);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_SET_P2P_Y_DIFF");
	}

	return 0;
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
	case TYPE_OFFSET_DATA_SDC:
		item_name = "CM_OFFSET_MODULE";
		break;
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

	if (mode->frame_channel)
		ret = sec_ts_read_channel(ts, mode->type, &mode->min, &mode->max, true);
	else
		ret = sec_ts_read_frame(ts, mode->type, &mode->min, &mode->max, true);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}

	if (mode->allnode) {
		if (mode->frame_channel) {
			for (ii = 0; ii < (ts->rx_count + ts->tx_count); ii++) {
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
				strncat(buff, temp, CMD_RESULT_WORD_LEN);

				memset(temp, 0x00, SEC_CMD_STR_LEN);
			}
		} else {
			for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
				strncat(buff, temp, CMD_RESULT_WORD_LEN);

				memset(temp, 0x00, SEC_CMD_STR_LEN);
			}
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

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING && (!mode->frame_channel))
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
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING && (!mode->frame_channel))
		sec_cmd_set_cmd_result_all(sec, temp, SEC_CMD_STR_LEN, item_name);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	sec_ts_locked_release_all_finger(ts);

	return ret;
}

static int sec_ts_read_rawp2p_data(struct sec_ts_data *ts,
		struct sec_cmd_data *sec, struct sec_ts_test_mode *mode)
{
	int ii;
	int ret = 0;
	char temp[SEC_CMD_STR_LEN] = { 0 };
	char *buff;
	u8 p2p_data[3] = {0};
	char para = TO_TOUCH_MODE;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto error_power_state;
	}

	buff = kzalloc(ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN, GFP_KERNEL);
	if (!buff)
		goto error_alloc_mem;

	input_info(true, &ts->client->dev, "%s: %d, %s\n",
			__func__, mode->type, mode->allnode ? "ALL" : "");

	disable_irq(ts->client->irq);

	ret = sec_ts_p2p_tmode(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix p2p tmode\n",
				__func__);
		goto error_tmode_fail;
	}

	ret = execute_p2ptest(ts, p2p_data);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix p2p test\n",
				__func__);
		goto error_test_fail;
	}

	if (mode->frame_channel)
		ret = sec_ts_read_channel(ts, mode->type, &mode->min, &mode->max, true);
	else
		ret = sec_ts_read_frame(ts, mode->type, &mode->min, &mode->max, true);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}

	if (mode->allnode) {
		if (mode->frame_channel) {
			for (ii = 0; ii < (ts->rx_count + ts->tx_count); ii++) {
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
				strncat(buff, temp, CMD_RESULT_WORD_LEN);

				memset(temp, 0x00, SEC_CMD_STR_LEN);
			}
		} else {
			for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
				snprintf(temp, CMD_RESULT_WORD_LEN, "%d,", ts->pFrame[ii]);
				strncat(buff, temp, CMD_RESULT_WORD_LEN);

				memset(temp, 0x00, SEC_CMD_STR_LEN);
			}
		}
	} else {
		snprintf(buff, SEC_CMD_STR_LEN, "%d,%d", mode->min, mode->max);
	}

	if (!sec)
		goto out_rawdata;

	sec_ts_locked_release_all_finger(ts);
	sec_ts_delay(30);
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: set rawdata type failed!\n", __func__);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, ts->tx_count * ts->rx_count * CMD_RESULT_WORD_LEN));
	sec->cmd_state = SEC_CMD_STATUS_OK;

out_rawdata:
	enable_irq(ts->client->irq);
	kfree(buff);

	return ret;
error_test_fail:
error_tmode_fail:
	kfree(buff);
	enable_irq(ts->client->irq);
error_alloc_mem:
error_power_state:
	if (!sec)
		return ret;

	snprintf(temp, SEC_CMD_STR_LEN, "FAIL");
	sec_cmd_set_cmd_result(sec, temp, SEC_CMD_STR_LEN);
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	return ret;
}

static int sec_ts_read_rawp2p_data_all(struct sec_ts_data *ts,
		struct sec_cmd_data *sec, struct sec_ts_test_mode *mode)
{
	int ret = 0;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int m_min = 0, m_max = 0;	/* mutual */
	int s_min = 0, s_max = 0;	/* self */
	int ii;
	short tx_min = 0, rx_min = 0, tx_max = 0, rx_max = 0;
	char para = TO_TOUCH_MODE;
	short *p2p_min = NULL;
	short *p2p_max = NULL;
	unsigned int readbytes = 0xFF;
	u8 p2p_data[3] = {0};

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto error_power_state;
	}

	input_info(true, &ts->client->dev, "%s: start\n", __func__);

	readbytes = ts->tx_count + ts->rx_count;

	p2p_min = kzalloc(readbytes, GFP_KERNEL);
	if (!p2p_min)
		goto error_power_state;

	p2p_max = kzalloc(readbytes, GFP_KERNEL);
	if (!p2p_max)
		goto error_alloc_mem;

	disable_irq(ts->client->irq);
	ret = sec_ts_p2p_tmode(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix p2p tmode\n",
				__func__);
		goto error_tmode_fail;
	}

	ret = execute_p2ptest(ts, p2p_data);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix p2p test\n",
				__func__);
		goto error_test_fail;
	}

	/** MIN **/
	mode->type = TYPE_RAW_DATA_P2P_MIN;

	ret = sec_ts_read_frame(ts, mode->type, &mode->min, &mode->max, true);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}
	m_min = mode->min;

	ret = sec_ts_read_channel(ts, mode->type, &mode->min, &mode->max, true);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}
	s_min = mode->min;

	memset(p2p_min, 0x00, readbytes);
	memcpy(p2p_min, ts->pFrame, readbytes);

	for (ii = 0; ii < ts->tx_count; ii++) {
		if (ii == 0)
			tx_min = ts->pFrame[ii];
		tx_min = min(tx_min, ts->pFrame[ii]);
	}
	for (ii = ts->tx_count; ii < ts->tx_count + ts->rx_count; ii++) {
		if (ii == ts->tx_count)
			rx_min = ts->pFrame[ii];
		rx_min = min(rx_min, ts->pFrame[ii]);
	}

	/** MAX **/
	mode->type = TYPE_RAW_DATA_P2P_MAX;

	ret = sec_ts_read_frame(ts, mode->type, &mode->min, &mode->max, true);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}
	m_max = mode->max;

	ret = sec_ts_read_channel(ts, mode->type, &mode->min, &mode->max, true);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read frame\n",
				__func__);
		goto error_test_fail;
	}
	s_max = mode->max;

	memset(p2p_max, 0x00, readbytes);
	memcpy(p2p_max, ts->pFrame, readbytes);

	for (ii = 0; ii < ts->tx_count; ii++) {
		if (ii == 0)
			tx_max = ts->pFrame[ii];
		tx_max = max(tx_max, ts->pFrame[ii]);
	}
	for (ii = ts->tx_count; ii < ts->tx_count + ts->rx_count; ii++) {
		if (ii == ts->tx_count)
			rx_max = ts->pFrame[ii];
		rx_max = max(rx_max, ts->pFrame[ii]);
	}

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		snprintf(buff, sizeof(buff), "%d,%d", m_min, m_max);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CM_RAW_SET_P2P");
		snprintf(buff, sizeof(buff), "%d,%d", 0, p2p_data[0]);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CM_RAW_SET_P2P_DIFF");

		snprintf(buff, sizeof(buff), "%d,%d", tx_min, tx_max);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_SET_P2P_X");
		snprintf(buff, sizeof(buff), "%d,%d", rx_min, rx_max);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_SET_P2P_Y");
		get_self_p2p_diff_data(sec, ts, p2p_min, p2p_max);
	}

	snprintf(buff, sizeof(buff), "%d,%d,%d,%d", m_min, m_max, s_min, s_max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_ts_locked_release_all_finger(ts);

	sec_ts_delay(30);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: set rawdata type failed!\n", __func__);

	enable_irq(ts->client->irq);

	kfree(p2p_min);
	kfree(p2p_max);

	return ret;

error_test_fail:
	sec_ts_locked_release_all_finger(ts);
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: set rawdata type failed!\n", __func__);

error_tmode_fail:
	enable_irq(ts->client->irq);
	kfree(p2p_max);
error_alloc_mem:
	kfree(p2p_min);
error_power_state:
	snprintf(buff, sizeof(buff), "%s", "FAIL");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CM_RAW_SET_P2P");
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CM_RAW_SET_P2P_DIFF");
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_SET_P2P_X");
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_SET_P2P_Y");
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_SET_P2P_X_DIFF");
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "SELF_RAW_SET_P2P_Y_DIFF");
	}
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
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

	/* fixed tune version will be saved at execute autotune */
	snprintf(buff, sizeof(buff), "C%02XT01%02X.%4s%s%c%d%c%d%c%d",
		ts->tdata->cal_count, ts->tdata->tune_fix_ver, ts->tdata->tclm_string[ts->tdata->cal_position].f_name,
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

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_TOUCH_MODE_FOR_THRESHOLD, threshold, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: threshold write type failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
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

static void module_off_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[3] = { 0 };
	int ret = 0;

	ret = sec_ts_stop_device(ts);

	if (ret == 0)
		snprintf(buff, sizeof(buff), "%s", "OK");
	else
		snprintf(buff, sizeof(buff), "%s", "NG");

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (strncmp(buff, "OK", 2) == 0)
		sec->cmd_state = SEC_CMD_STATUS_OK;
	else
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void module_on_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[3] = { 0 };
	int ret = 0;

	ret = sec_ts_start_device(ts);

	if (ts->input_dev->disabled) {
		sec_ts_set_lowpowermode(ts, TO_LOWPOWER_MODE);
		ts->power_status = SEC_TS_STATE_LPM;
	}

	if (ret == 0)
		snprintf(buff, sizeof(buff), "%s", "OK");
	else
		snprintf(buff, sizeof(buff), "%s", "NG");

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (strncmp(buff, "OK", 2) == 0)
		sec->cmd_state = SEC_CMD_STATUS_OK;
	else
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };

	strncpy(buff, "SEC", sizeof(buff));
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
		strncpy(buff, "MC44", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x05)
		strncpy(buff, "A552", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x09)
		strncpy(buff, "Y661", sizeof(buff));
	else if (ts->plat_data->img_version_of_ic[0] == 0x10)
		strncpy(buff, "Y761", sizeof(buff));
	else
		strncpy(buff, "N/A", sizeof(buff));

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

static void get_x_cross_routing(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void get_y_cross_routing(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	ret = strncmp(ts->plat_data->model_name, "G935", 4)
		&& strncmp(ts->plat_data->model_name, "N930", 4);
	if (ret == 0)
		snprintf(buff, sizeof(buff), "13,14");
	else
		snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static u32 checksum_sum(struct sec_ts_data *ts, u8 *chunk_data, u32 size)
{
	u32 data_32 = 0;
	u32 checksum = 0;
	u8 *fd = chunk_data;
	int i;

	for (i = 0; i < size/4; i++) {
		data_32 = (((fd[(i*4)+0]&0xFF)<<0) | ((fd[(i*4)+1]&0xFF)<<8) |
					((fd[(i*4)+2]&0xFF)<<16) | ((fd[(i*4)+3]&0xFF)<<24));
		checksum += data_32;
	}

	checksum &= 0xFFFFFFFF;

	input_info(true, &ts->client->dev, "%s: checksum = [%x]\n", __func__, checksum);

	return checksum;
}

static u32 get_bin_checksum(struct sec_ts_data *ts, const u8 *data, size_t size)
{

	int i;
	fw_header *fw_hd;
	fw_chunk *fw_ch;
	u8 *fd = (u8 *)data;
	u32 data_32;
	u32 checksum = 0;
	u32 chunk_checksum[3];
	u32 img_checksum;

	fw_hd = (fw_header *)fd;
	fd += sizeof(fw_header);

	if (fw_hd->signature != SEC_TS_FW_HEADER_SIGN) {
		input_err(true, &ts->client->dev, "%s: firmware header error = %08X\n", __func__, fw_hd->signature);
		return 0;
	}

	for (i = 0; i < (fw_hd->totalsize/4); i++) {
		data_32 = (((data[(i*4)+0]&0xFF)<<0) | ((data[(i*4)+1]&0xFF)<<8) |
					((data[(i*4)+2]&0xFF)<<16) | ((data[(i*4)+3]&0xFF)<<24));
		checksum += data_32;
	}

	input_info(true, &ts->client->dev, "%s: fw_hd->totalsize = [%d] checksum = [%x]\n",
				__func__, fw_hd->totalsize, checksum);

	checksum &= 0xFFFFFFFF;
	if (checksum != 0) {
		input_err(true, &ts->client->dev, "%s: Checksum fail! = %08X\n", __func__, checksum);
		return 0;
	}

	for (i = 0; i < fw_hd->num_chunk; i++) {
		fw_ch = (fw_chunk *)fd;

		input_info(true, &ts->client->dev, "%s: [%d] 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", __func__, i,
				fw_ch->signature, fw_ch->addr, fw_ch->size, fw_ch->reserved);

		if (fw_ch->signature != SEC_TS_FW_CHUNK_SIGN) {
			input_err(true, &ts->client->dev, "%s: firmware chunk error = %08X\n",
						__func__, fw_ch->signature);
			return 0;
		}
		fd += sizeof(fw_chunk);

		checksum = checksum_sum(ts, fd, fw_ch->size);
		chunk_checksum[i] = checksum;
		fd += fw_ch->size;
	}

	img_checksum = chunk_checksum[0] + chunk_checksum[1];

	return img_checksum;
}

static void get_checksum_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	char csum_result[4] = { 0 };
	char data[5] = { 0 };
	u8 cal_result;
	u8 nv_result;
	u8 temp;
	u8 csum = 0;
	int ret, i;

	u32 ic_checksum;
	u32 img_checksum;
	const struct firmware *fw_entry;
	char fw_path[SEC_TS_MAX_FW_PATH];
	u8 fw_ver[4];

	sec_cmd_set_default_result(sec);
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		goto err;
	}

	disable_irq(ts->client->irq);

	ts->plat_data->power(ts, false);
	ts->power_status = SEC_TS_STATE_POWER_OFF;
	sec_ts_delay(50);

	ts->plat_data->power(ts, true);
	ts->power_status = SEC_TS_STATE_POWER_ON;
	sec_ts_delay(70);

	ret = sec_ts_wait_for_ready(ts, SEC_TS_ACK_BOOT_COMPLETE);
	if (ret < 0) {
		enable_irq(ts->client->irq);
		input_err(true, &ts->client->dev, "%s: boot complete failed\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		goto err;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_FIRMWARE_INTEGRITY, &data[0], 1);
	if (ret < 0 || (data[0] != 0x80)) {
		enable_irq(ts->client->irq);
		input_err(true, &ts->client->dev, "%s: firmware integrity failed, ret:%d, data:%X\n",
				__func__, ret, data[0]);
		snprintf(buff, sizeof(buff), "%s", "NG");
		goto err;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, &data[1], 1);
	if (ret < 0 || (data[1] != SEC_TS_STATUS_APP_MODE)) {
		enable_irq(ts->client->irq);
		input_err(true, &ts->client->dev, "%s: boot status failed, ret:%d, data:%X\n", __func__, ret, data[0]);
		snprintf(buff, sizeof(buff), "%s", "NG");
		goto err;
	}

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, &data[2], 4);
	if (ret < 0 || (data[3] == TOUCH_SYSTEM_MODE_FLASH)) {
		enable_irq(ts->client->irq);
		input_err(true, &ts->client->dev, "%s: touch status failed, ret:%d, data:%X\n", __func__, ret, data[3]);
		snprintf(buff, sizeof(buff), "%s", "NG");
		goto err;
	}

	sec_ts_reinit(ts);

	enable_irq(ts->client->irq);

	input_err(true, &ts->client->dev, "%s: data[0]:%X, data[1]:%X, data[3]:%X\n", __func__, data[0], data[1], data[3]);

	temp = DO_FW_CHECKSUM | DO_PARA_CHECKSUM;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_GET_CHECKSUM, &temp, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: send get_checksum_cmd fail!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "SendCMDfail");
		goto err;
	}

	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_GET_CHECKSUM, csum_result, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read get_checksum result fail!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "ReadCSUMfail");
		goto err;
	}

	nv_result = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_FAC_RESULT);
	nv_result += get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_CAL_COUNT);

	cal_result = sec_ts_read_calibration_report(ts);

	for (i = 0; i < 4; i++)
		csum += csum_result[i];

	csum += nv_result;
	csum += cal_result;

	csum = ~csum;

	if (ts->plat_data->firmware_name != NULL) {

		ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_IMG_VERSION, fw_ver, 4);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: firmware version read error\n", __func__);

		for (i = 0; i < 4; i++) {
			if (ts->plat_data->img_version_of_bin[i] !=  fw_ver[i]) {
				input_err(true, &ts->client->dev, "%s: do not matched version info [%d]:[%x]!=[%x] and skip!\n",
							__func__, i, ts->plat_data->img_version_of_bin[i], fw_ver[i]);
				goto out;
			}
		}

		ic_checksum = (((csum_result[0]&0xFF)<<24) | ((csum_result[1]&0xFF)<<16) |
						((csum_result[2]&0xFF)<<8) | ((csum_result[3]&0xFF)<<0));

		snprintf(fw_path, SEC_TS_MAX_FW_PATH, "%s", ts->plat_data->firmware_name);

		if (request_firmware(&fw_entry, fw_path, &ts->client->dev) !=  0) {
			input_err(true, &ts->client->dev, "%s: firmware is not available\n", __func__);
			snprintf(buff, sizeof(buff), "%s", "NG");
			goto err;
		}

		img_checksum = get_bin_checksum(ts, fw_entry->data, fw_entry->size);

		release_firmware(fw_entry);

		input_info(true, &ts->client->dev, "%s: img_checksum=[0x%X], ic_checksum=[0x%X]\n",
						__func__, img_checksum, ic_checksum);

		if (img_checksum != ic_checksum) {
			input_err(true, &ts->client->dev, "%s: img_checksum=[0x%X] != ic_checksum=[0x%X]!!!\n",
							__func__, img_checksum, ic_checksum);
			snprintf(buff, sizeof(buff), "%s", "NG");
			goto err;
		}
	}
out:

	input_info(true, &ts->client->dev, "%s: checksum = %02X\n", __func__, csum);
	snprintf(buff, sizeof(buff), "%02X", csum);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	return;

err:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
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

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, sec, &mode);
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

static void run_raw_p2p_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));

	/* both mutual, self for factory_cmd_result_all*/
	sec_ts_read_rawp2p_data_all(ts, sec, &mode);
}

static void run_raw_p2p_min_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA_P2P_MIN;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_rawp2p_data(ts, sec, &mode);
}

static void run_raw_p2p_max_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA_P2P_MAX;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_rawp2p_data(ts, sec, &mode);
}

/* self reference : send TX power in TX channel, receive in TX channel */
static void run_self_reference_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SEC;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_self_reference_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SEC;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_self_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_self_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_OFFSET_DATA_SDC;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_self_delta_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_self_delta_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_raw_data(ts, sec, &mode);
}

static void run_self_raw_p2p_min_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA_P2P_MIN;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_rawp2p_data(ts, sec, &mode);
}

static void run_self_raw_p2p_max_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	struct sec_ts_test_mode mode;

	sec_cmd_set_default_result(sec);

	memset(&mode, 0x00, sizeof(struct sec_ts_test_mode));
	mode.type = TYPE_RAW_DATA_P2P_MAX;
	mode.frame_channel = TEST_MODE_READ_CHANNEL;
	mode.allnode = TEST_MODE_ALL_NODE;

	sec_ts_read_rawp2p_data(ts, sec, &mode);
}

void sec_ts_get_saved_cmoffset(struct sec_ts_data *ts)
{
	int rc;
	u8 *rBuff = NULL;
	int i;
	int result_size = SEC_TS_SELFTEST_REPORT_SIZE + ts->tx_count * ts->rx_count * 2;
	short min, max;

	input_raw_info(true, &ts->client->dev, "%s:\n", __func__);

	rBuff = kzalloc(result_size, GFP_KERNEL);
	if (!rBuff)
		return;

	rc = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SELFTEST_RESULT, rBuff, result_size);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}

	memset(ts->pFrame, 0x00, result_size);

	for (i = 0; i < result_size - SEC_TS_SELFTEST_REPORT_SIZE; i += 2)
		ts->pFrame[i / 2] = (rBuff[SEC_TS_SELFTEST_REPORT_SIZE + i + 1] << 8)
				| rBuff[SEC_TS_SELFTEST_REPORT_SIZE + i];

	min = max = ts->pFrame[0];

	sec_ts_print_frame(ts, &min, &max);

err_exit:
	kfree(rBuff);
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
#ifdef USE_PRESSURE_SENSOR
	short pressure[3] = { 0 };
	u8 cal_data[18] = { 0 };
#endif

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

#ifdef MINORITY_REPORT
		if (test_type[i] == TYPE_AMBIENT_DATA) {
			minority_report_calculate_rawdata(ts);
		} else if (test_type[i] == TYPE_OFFSET_DATA_SDC) {
			minority_report_calculate_ito(ts);
			minority_report_sync_latest_value(ts);
		}
#endif
		if (full_read) {
			ret = sec_ts_read_channel(ts, test_type[i], &min, &max, false);
			if (ret < 0) {
				input_raw_info(true, &ts->client->dev,
						"%s: self %d : error ## ret:%d\n",
						__func__, test_type[i], ret);
				goto out;
			} else {
				input_raw_info(true, &ts->client->dev,
						"%s: self %d : Max/Min %d,%d ##\n",
						__func__, test_type[i], max, min);
			}
			sec_ts_delay(20);
		}
	}

#ifdef USE_PRESSURE_SENSOR
	ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to fix tmode\n",
				__func__);
		goto out;
	}

	/* run pressure offset data read */
	read_pressure_data(ts, TYPE_OFFSET_DATA_SEC, pressure);
	sec_ts_delay(20);

	/* run pressure rawdata read */
	read_pressure_data(ts, TYPE_RAW_DATA, pressure);
	sec_ts_delay(20);

	/* run pressure raw delta read  */
	read_pressure_data(ts, TYPE_REMV_AMB_DATA, pressure);
	sec_ts_delay(20);

	/* run pressure sigdata read */
	read_pressure_data(ts, TYPE_SIGNAL_DATA, pressure);
	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SET_GET_PRESSURE, cal_data, 6);
	ts->pressure_left = ((cal_data[0] << 8) | cal_data[1]);
	ts->pressure_center = ((cal_data[2] << 8) | cal_data[3]);
	ts->pressure_right = ((cal_data[4] << 8) | cal_data[5]);
	input_raw_info(true, &ts->client->dev, "%s: pressure cal data - Left: %d, Center: %d, Right: %d\n",
			__func__, ts->pressure_left, ts->pressure_center,
			ts->pressure_right);
#endif
	sec_ts_release_tmode(ts);

	if (full_read)
		sec_ts_get_saved_cmoffset(ts);

out:
	input_raw_info(true, &ts->client->dev, "%s: ito : %02X %02X %02X %02X\n",
			__func__, ts->ito_test[0], ts->ito_test[1]
			, ts->ito_test[2], ts->ito_test[3]);

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

	snprintf(buff, sizeof(buff), "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

/* Use TSP NV area
 * buff[0] : offset from user NVM storage
 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
 * buff[2] : write data
 * buff[..] : cont.
 */
void set_tsp_nvm_data_clear(struct sec_ts_data *ts, u8 offset)
{
	char buff[4] = { 0 };
	int ret;

	input_dbg(true, &ts->client->dev, "%s\n", __func__);

	buff[0] = offset;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: nvm write failed. ret: %d\n", __func__, ret);

	sec_ts_delay(20);
}

int get_tsp_nvm_data(struct sec_ts_data *ts, u8 offset)
{
	char buff[2] = { 0 };
	int ret;

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

	/* send NV data using command
	 * Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 */
	memset(buff, 0x00, 2);
	buff[0] = offset;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	sec_ts_delay(20);

	/* read NV data
	 * Use TSP NV area : in this model, use only one byte
	 */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_NVM, buff, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	input_info(true, &ts->client->dev, "%s: offset:%u  data:%02X\n", __func__, offset, buff[0]);

out_nvm:
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);

	sec_ts_delay(300);

	input_dbg(false, &ts->client->dev, "%s: SENSE ON\n", __func__);

	return buff[0];
}

int get_tsp_nvm_data_by_size(struct sec_ts_data *ts, u8 offset, int length, u8 *data)
{
	char *buff = NULL;
	int ret;

	buff = kzalloc(length, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	input_info(true, &ts->client->dev, "%s: offset:%u, length:%d, size:%d\n", __func__, offset, length, sizeof(data));

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

	sec_ts_locked_release_all_finger(ts);

	/* send NV data using command
	 * Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 */
	memset(buff, 0x00, 2);
	buff[0] = offset;
	buff[1] = length - 1;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	sec_ts_delay(20);

	/* read NV data
	 * Use TSP NV area : in this model, use only one byte
	 */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_NVM, buff, length);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	memcpy(data, buff, length);

out_nvm:
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);

	sec_ts_delay(300);

	input_dbg(true, &ts->client->dev, "%s: SENSE ON\n", __func__);

	kfree(buff);

	return ret;
}

#ifdef TCLM_CONCEPT
int sec_tclm_data_read(struct i2c_client *client, int address)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);
	int ret = 0;
	u8 reg = 0;
	u8 buff[4];
	int i = 0;
	static bool all_data_flag;
	bool tune_ver_flag = false;

	switch (address) {
	case SEC_TCLM_NVM_OFFSET_FAC_RESULT:
		if (all_data_flag)
			return ts->tdata->nvm_all_data[SEC_TS_NVM_OFFSET_FAC_RESULT];
		else
			reg = SEC_TS_NVM_OFFSET_FAC_RESULT;
		break;
	case SEC_TCLM_NVM_OFFSET_CAL_COUNT:
		if (all_data_flag)
			return ts->tdata->nvm_all_data[SEC_TS_NVM_OFFSET_CAL_COUNT];
		else
			reg = SEC_TS_NVM_OFFSET_CAL_COUNT;
		break;
	case SEC_TCLM_NVM_OFFSET_TUNE_VERSION:
		if (all_data_flag)
			return ts->tdata->nvm_all_data[SEC_TS_NVM_OFFSET_TUNE_VERSION + 1];
		else
			tune_ver_flag = true;
		break;
	case SEC_TCLM_NVM_OFFSET_CAL_POSITION:
		if (all_data_flag)
			return ts->tdata->nvm_all_data[SEC_TS_NVM_OFFSET_CAL_POSITION];
		else
			reg = SEC_TS_NVM_OFFSET_CAL_POSITION;
		break;
	case SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_COUNT:
		if (all_data_flag)
			return ts->tdata->nvm_all_data[SEC_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT];
		else
			reg = SEC_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT;
		break;
	case SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_LASTP:
		if (all_data_flag)
			return ts->tdata->nvm_all_data[SEC_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP];
		else
			reg = SEC_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP;
		break;
	case SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_ZERO:
		if (all_data_flag)
			return ts->tdata->nvm_all_data[SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO];
		else
			reg = SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO;
		break;
	case SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_SIZE:
		if (all_data_flag)
			for (i = 0; i < ts->tdata->cal_pos_hist_cnt * 2; i++)
				ts->tdata->cal_pos_hist_queue[i] = ts->tdata->nvm_all_data[SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO + i];
		else
			ret = get_tsp_nvm_data_by_size(ts, SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO,
				ts->tdata->cal_pos_hist_cnt * 2, ts->tdata->cal_pos_hist_queue);
		return ret;
	case SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER:
		ts->sec_ts_i2c_read(ts, SEC_TS_READ_IMG_VERSION, buff, 4);
		ret =  buff[2] << 8 | buff[3];
		return ret;
	case SEC_TCLM_NVM_ALL_DATA:
		ret = get_tsp_nvm_data_by_size(ts, SEC_TS_NVM_OFFSET_FAC_RESULT,
				SEC_TCLM_NVM_ALL_SIZE, ts->tdata->nvm_all_data);
		all_data_flag = true;
		return ret;
	case SEC_TCLM_NVM_ALL_DATA_DONE:
		all_data_flag = false;
		return 0;
	default:
		return 0;
	}

	if (tune_ver_flag) {
		ret = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_TUNE_VERSION + 1);
		return ret;
	}
	ret = get_tsp_nvm_data(ts, reg);

	return ret;
}

void sec_tclm_data_write(struct i2c_client *client, int address, int data)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);
	u8 reg = 0;
	bool tune_ver_flag = false;

	switch (address) {
	case SEC_TCLM_NVM_OFFSET_FAC_RESULT:
		reg = SEC_TS_NVM_OFFSET_FAC_RESULT;
		break;
	case SEC_TCLM_NVM_OFFSET_CAL_COUNT:
		reg = SEC_TS_NVM_OFFSET_CAL_COUNT;
		break;
	case SEC_TCLM_NVM_OFFSET_TUNE_VERSION:
		reg = SEC_TS_NVM_OFFSET_TUNE_VERSION;
		tune_ver_flag = true;
		break;
	case SEC_TCLM_NVM_OFFSET_CAL_POSITION:
		reg = SEC_TS_NVM_OFFSET_CAL_POSITION;
		break;
	case SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_COUNT:
		reg = SEC_TS_NVM_OFFSET_HISTORY_QUEUE_COUNT;
		break;
	case SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_LASTP:
		reg = SEC_TS_NVM_OFFSET_HISTORY_QUEUE_LASTP;
		break;
	case SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_ZERO:
		reg = SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO;
		break;
	case SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_SAVE:
		sec_ts_tclm_set_nvm_data(ts, SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO + ts->tdata->cal_pos_hist_lastp * 2, ts->tdata->cal_position);
		sec_ts_tclm_set_nvm_data(ts, SEC_TS_NVM_OFFSET_HISTORY_QUEUE_ZERO + ts->tdata->cal_pos_hist_lastp * 2 + 1, ts->tdata->cal_count);
		return;
	default:
		return;
	}

	if (tune_ver_flag) {
		sec_ts_tclm_set_nvm_data(ts, reg, (u8)(data >> 8));
		sec_ts_tclm_set_nvm_data(ts, reg + 1, (u8)(0xff & data));
	} else {
		sec_ts_tclm_set_nvm_data(ts, reg, (u8)data);
	}
}
void sec_ts_tclm_set_nvm_data(struct sec_ts_data *ts, u8 reg, u8 data)
{
	char buff[SEC_CMD_STR_LEN] = {0};
	int rc;

	buff[0] = reg;
	buff[1] = 0;	/* 1bytes */
	buff[2] = (u8) data;
	input_info(true, &ts->client->dev, "%s: write to nvm (%d)\n",
				__func__, buff[2]);

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 3);
	if (rc < 0) {
		input_err(true, &ts->client->dev,
			"%s: nvm write failed. ret: %d\n", __func__, rc);
	}
	sec_ts_delay(20);
}
#if 0
static void sec_ts_tclm_set_nvm_data_2byte(struct sec_ts_data *ts, u8 reg, u8 data1, u8 data2)
{
	char buff[SEC_CMD_STR_LEN] = {0};
	int rc;

	buff[0] = reg;
	buff[1] = 1;	/* 2bytes */
	buff[2] = data1;
	buff[3] = data2;
	input_info(true, &ts->client->dev, "%s: write nvm (%2X %2X)\n", __func__, buff[2], buff[3]);

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 4);
	if (rc < 0) {
		input_err(true, &ts->client->dev,
			"%s: nvm write failed. ret: %d\n", __func__, rc);
	}
	sec_ts_delay(20);
}
#endif
#endif

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
	int ii, jj;
	int temp = 0;
	int max = -30000;
	int min = 30000;
	int node_gap = 1;
	short tx_max[20] = { 0 };
	short tx_min[20] = { 0 };
	short rx_max[40] = { 0 };
	short rx_min[40] = { 0 };

	for (ii = 0; ii < 20; ii++) {
		tx_max[ii] = -30000;
		tx_min[ii] = 30000;
	}

	for (ii = 0; ii < 40; ii++) {
		rx_max[ii] = -30000;
		rx_min[ii] = 30000;
	}

	if (!ts->tx_count) {
		ts->item_rawdata = 0xD;
		return;
	}

	for (ii = 0; ii < (ts->rx_count * ts->tx_count); ii++) {
		if (max < ts->pFrame[ii]) {
			ts->max_ambient = max = ts->pFrame[ii];
			ts->max_ambient_channel_tx = ii % ts->tx_count;
			ts->max_ambient_channel_rx = ii / ts->tx_count;
		}
		if (min > ts->pFrame[ii]) {
			ts->min_ambient = min = ts->pFrame[ii];
			ts->min_ambient_channel_tx = ii % ts->tx_count;
			ts->min_ambient_channel_rx = ii / ts->tx_count;
		}

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

		ts->ambient_rx[ii / ts->tx_count] += ts->pFrame[ii];
		ts->ambient_tx[ii % ts->tx_count] += ts->pFrame[ii];

		rx_max[ii / ts->tx_count] = max(rx_max[ii / ts->tx_count], ts->pFrame[ii]);
		rx_min[ii / ts->tx_count] = min(rx_min[ii / ts->tx_count], ts->pFrame[ii]);
		tx_max[ii % ts->tx_count] = max(tx_max[ii % ts->tx_count], ts->pFrame[ii]);
		tx_min[ii % ts->tx_count] = min(tx_min[ii % ts->tx_count], ts->pFrame[ii]);

	}

	for (ii = 0; ii < ts->tx_count; ii++)
		ts->ambient_tx[ii] /= ts->rx_count;

	for (ii = 0; ii < ts->rx_count; ii++)
		ts->ambient_rx[ii] /= ts->tx_count;

	for (ii = 0; ii < ts->rx_count; ii++)
		ts->ambient_rx_delta[ii] = rx_max[ii] - rx_min[ii];

	for (jj = 0; jj < ts->tx_count; jj++)
		ts->ambient_tx_delta[jj] = tx_max[jj] - tx_min[jj];

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

	if (ts->ito_test[0] ||  ts->ito_test[1] || ts->ito_test[2] || ts->ito_test[3])
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
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char r_data[1] = { 0 };
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

	r_data[0] = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_FAC_RESULT);
	if (r_data[0] == 0xFF)
		r_data[0] = 0;

	result = (struct sec_ts_test_result *)r_data;

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

	/* Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 * buff[2] : write data
	 */
	memset(buff, 0x00, SEC_CMD_STR_LEN);
	buff[2] = *result->data;

	input_info(true, &ts->client->dev, "%s: command (1)%X, (2)%X: %X\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1], buff[2]);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: nvm write failed. ret: %d\n", __func__, ret);

	sec_ts_delay(20);

	ts->nv = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_FAC_RESULT);

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void get_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct sec_ts_test_result *result;

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
	buff[0] = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_FAC_RESULT);
	if (buff[0] == 0xFF) {
		set_tsp_nvm_data_clear(ts, SEC_TS_NVM_OFFSET_FAC_RESULT);
		buff[0] = 0;
	}

	ts->nv = buff[0];

	result = (struct sec_ts_test_result *)buff;

	input_info(true, &ts->client->dev, "%s: [0x%X][0x%X] M:%d, M:%d, A:%d, A:%d\n",
			__func__, *result->data, buff[0],
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

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void clear_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
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

	/* Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 * buff[2] : write data
	 */
	memset(buff, 0x00, SEC_CMD_STR_LEN);
	buff[0] = SEC_TS_NVM_OFFSET_FAC_RESULT;
	buff[1] = 0x00;
	buff[2] = 0x00;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: nvm write failed. ret: %d\n", __func__, ret);

	sec_ts_delay(20);

	ts->nv = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_FAC_RESULT);

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void increase_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[3] = { 0 };
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

	buff[2] = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT);

	input_info(true, &ts->client->dev, "%s: disassemble count is #1 %d\n", __func__, buff[2]);

	if (buff[2] == 0xFF)
		buff[2] = 0;

	if (buff[2] < 0xFE)
		buff[2]++;

	/* Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 * buff[2] : write data
	 */
	buff[0] = SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT;
	buff[1] = 0;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: nvm write failed. ret: %d\n", __func__, ret);

	sec_ts_delay(20);

	memset(buff, 0x00, 3);
	buff[0] = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT);
	input_info(true, &ts->client->dev, "%s: check disassemble count: %d\n", __func__, buff[0]);

	snprintf(buff, sizeof(buff), "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void get_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

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
	buff[0] = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT);
	if (buff[0] == 0xFF) {
		set_tsp_nvm_data_clear(ts, SEC_TS_NVM_OFFSET_DISASSEMBLE_COUNT);
		buff[0] = 0;
	}

	input_info(true, &ts->client->dev, "%s: read disassemble count: %d\n", __func__, buff[0]);

	snprintf(buff, sizeof(buff), "%d", buff[0]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

#define GLOVE_MODE_EN		(1 << 0)
#define CLEAR_COVER_EN		(1 << 1)
#define FAST_GLOVE_MODE_EN	(1 << 2)

static void glove_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int glove_mode_enables = 0;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		int retval;

		if (sec->cmd_param[0])
			glove_mode_enables |= GLOVE_MODE_EN;
		else
			glove_mode_enables &= ~(GLOVE_MODE_EN);

		retval = sec_ts_glove_mode_enables(ts, glove_mode_enables);

		if (retval < 0) {
			input_err(true, &ts->client->dev, "%s: failed, retval = %d\n", __func__, retval);
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);
}

static void clear_cover_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	input_info(true, &ts->client->dev, "%s: start clear_cover_mode %s\n", __func__, buff);
	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0] > 1) {
			ts->flip_enable = true;
			ts->cover_type = sec->cmd_param[1];
			ts->cover_cmd = (u8)ts->cover_type;
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
			if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
				sec_ts_delay(500);
				tui_force_close(1);
				sec_ts_delay(200);
				if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
					trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
					trustedui_set_mode(TRUSTEDUI_MODE_OFF);
				}
			}

			tui_cover_mode_set(true);
#endif

		} else {
			ts->flip_enable = false;
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
			tui_cover_mode_set(false);
#endif
		}

		if (!ts->power_status == SEC_TS_STATE_POWER_OFF && ts->reinit_done) {
			if (ts->flip_enable)
				sec_ts_set_cover_type(ts, true);
			else
				sec_ts_set_cover_type(ts, false);
		}

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
};

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


static void drawing_test_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (ts->use_sponge) {
			if (sec->cmd_param[0])
				ts->lowpower_mode &= ~SEC_TS_MODE_SPONGE_FORCE_KEY;
			else
				ts->lowpower_mode |= SEC_TS_MODE_SPONGE_FORCE_KEY;

			ret = sec_ts_set_custom_library(ts);
			if (ret < 0) {
				snprintf(buff, sizeof(buff), "%s", "NG");
				sec->cmd_state = SEC_CMD_STATUS_FAIL;
			} else {
				snprintf(buff, sizeof(buff), "%s", "OK");
				sec->cmd_state = SEC_CMD_STATUS_OK;
			}

		} else {
			snprintf(buff, sizeof(buff), "%s", "NA");
			sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
};

static void sec_ts_swap(u8 *a, u8 *b)
{
	u8 temp = *a;

	*a = *b;
	*b = temp;
}

static void rearrange_sft_result(u8 *data, int length)
{
	int i;

	for(i = 0; i < length; i += 4) {
		sec_ts_swap(&data[i], &data[i + 3]);
		sec_ts_swap(&data[i + 1], &data[i + 2]);
	}
}

static int execute_selftest(struct sec_ts_data *ts, bool save_result)
{
	u8 pStr[50] = {0};
	u8 pTmp[20];
	int rc = 0;
	u8 tpara[2] = {0x23, 0x40};
	u8 *rBuff;
	int i;
	int result_size = SEC_TS_SELFTEST_REPORT_SIZE + ts->tx_count * ts->rx_count * 2;

	/* set Factory level */
	if (ts->factory_level) {
		ts->factory_position += 1;
		rc = sec_ts_write_factory_level(ts, ts->factory_position);
		if (rc < 0)
			goto err_set_level;
	}

	/* save selftest result in flash */
	if (save_result)
		tpara[0] = 0x23;
	else
		tpara[0] = 0xA3;

	if (ts->plat_data->support_pressure)
		tpara[0] |= 0x10;

	rBuff = kzalloc(result_size, GFP_KERNEL);
	if (!rBuff)
		goto err_mem;

	input_info(true, &ts->client->dev, "%s: Self test start!\n", __func__);
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST, tpara, 2);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Send selftest cmd failed!\n", __func__);
		goto err_exit;
	}

	sec_ts_delay(350);

	rc = sec_ts_wait_for_ready(ts, SEC_TS_VENDOR_ACK_SELF_TEST_DONE);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}

	input_raw_info(true, &ts->client->dev, "%s: Self test done!\n", __func__);

	rc = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SELFTEST_RESULT, rBuff, result_size);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_exit;
	}
	rearrange_sft_result(rBuff, result_size);

	for (i = 0; i < 80; i += 4) {
		if (i / 4 == 0)
			strncat(pStr, "SIG ", 5);
		else if (i / 4 == 1)
			strncat(pStr, "VER ", 5);
		else if (i / 4 == 2)
			strncat(pStr, "SIZ ", 5);
		else if (i / 4 == 3)
			strncat(pStr, "CRC ", 5);
		else if (i / 4 == 4)
			strncat(pStr, "RES ", 5);
		else if (i / 4 == 5)
			strncat(pStr, "COU ", 5);
		else if (i / 4 == 6)
			strncat(pStr, "PAS ", 5);
		else if (i / 4 == 7)
			strncat(pStr, "FAI ", 5);
		else if (i / 4 == 8)
			strncat(pStr, "CHA ", 5);
		else if (i / 4 == 9)
			strncat(pStr, "AMB ", 5);
		else if (i / 4 == 10)
			strncat(pStr, "RXS ", 5);
		else if (i / 4 == 11)
			strncat(pStr, "TXS ", 5);
		else if (i / 4 == 12)
			strncat(pStr, "RXO ", 5);
		else if (i / 4 == 13)
			strncat(pStr, "TXO ", 5);
		else if (i / 4 == 14)
			strncat(pStr, "RXG ", 5);
		else if (i / 4 == 15)
			strncat(pStr, "TXG ", 5);
		else if (i / 4 == 16)
			strncat(pStr, "RXR ", 5);
		else if (i / 4 == 17)
			strncat(pStr, "TXT ", 5);
		else if (i / 4 == 18)
			strncat(pStr, "RXT ", 5);
		else if (i / 4 == 19)
			strncat(pStr, "TXR ", 5);

		snprintf(pTmp, sizeof(pTmp), "%2X, %2X, %2X, %2X",
			rBuff[i], rBuff[i + 1], rBuff[i + 2], rBuff[i + 3]);
		strncat(pStr, pTmp, strnlen(pTmp, sizeof(pTmp)));

		if (i / 4 == 4) {
			if ((rBuff[i + 3] & 0x30) != 0)// RX, RX open check.
				rc = 0;
			else
				rc = 1;

			ts->ito_test[0] = rBuff[i];
			ts->ito_test[1] = rBuff[i + 1];
			ts->ito_test[2] = rBuff[i + 2];
			ts->ito_test[3] = rBuff[i + 3];
		}
		if (i % 8 == 4) {
			input_raw_info(true, &ts->client->dev, "%s\n", pStr);
			memset(pStr, 0x00, sizeof(pStr));
		} else {
			strncat(pStr, "  ", 3);
		}
	}

err_exit:
	kfree(rBuff);
err_mem:
	if (ts->factory_level)
		sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);
err_set_level:
	ts->factory_level = false;
	ts->factory_position = 0;
	return rc;
}

static void run_15khz_cm3_gap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	/* fix length: if all channel failed, can occur overflow */
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rc;
	int size = ts->tx_count * ts->rx_count * 2;
	u8 *rBuff = NULL;
	short *pFrame = NULL;
	int ii, jj;
	u8 data[32] = { 0 };
	short cm3[ts->rx_count][ts->tx_count];
	short gap_x[ts->rx_count][ts->tx_count];
	short gap_y[ts->rx_count][ts->tx_count];
	short gap_max[ts->rx_count][ts->tx_count];
	short spec[ts->rx_count][ts->tx_count];

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	rBuff = kzalloc(size, GFP_KERNEL);
	if (!rBuff) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	memset(rBuff, 0x00, size);
	memset(data, 0x00, 32);

	pFrame = vmalloc(size);
	if (!pFrame) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		kfree(rBuff);
		return;
	}

	memset(pFrame, 0x00, size);

	disable_irq(ts->client->irq);

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_OFF, NULL, 0);
	if (rc < 0)
		goto cm_gap_i2c_err;

	data[0] = 0xC0;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST, data, 1);
	if (rc < 0)
		goto cm_gap_i2c_err;

	sec_ts_delay(30);

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (rc < 0)
		goto cm_gap_i2c_err;

	data[0] = 0x1D;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MUTU_RAW_TYPE, data, 1);
	if (rc < 0)
		goto cm_gap_i2c_err;

	data[0] = 0x40;
	data[1] = 0x00;
	data[2] = 0x50;
	data[3] = 0x1C;
	rc = ts->sec_ts_i2c_write(ts, 0xD0, data, 4);
	if (rc < 0)
		goto cm_gap_i2c_err;

	data[0] = 0x00;
	data[1] = 0x04;
	rc = ts->sec_ts_i2c_write(ts, 0xD1, data, 2);
	if (rc < 0)
		goto cm_gap_i2c_err;

	data[0] = 0x1F;
	data[1] = 0x00;
	data[2] = 0x00;
	data[3] = 0x00;
	rc = ts->sec_ts_i2c_write(ts, 0xD3, data, 4);
	if (rc < 0)
		goto cm_gap_i2c_err;

	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = 0x0F;
	rc = ts->sec_ts_i2c_write(ts, 0xF7, data, 3);
	if (rc < 0)
		goto cm_gap_i2c_err;

	data[0] = 0x00;
	data[1] = 0x19;
	rc = ts->sec_ts_i2c_write(ts, 0xF8, data, 2);
	if (rc < 0)
		goto cm_gap_i2c_err;

	data[0] = 0x82;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST, data, 1);
	if (rc < 0)
		goto cm_gap_i2c_err;

	sec_ts_delay(1500);

	rc = sec_ts_wait_for_ready(ts, SEC_TS_VENDOR_ACK_SELF_TEST_DONE);
	if (rc < 0)
		goto cm_gap_i2c_err;

	rc = ts->sec_ts_i2c_read(ts, SEC_TS_READ_TOUCH_RAWDATA, rBuff, size);
	if (rc < 0)
		goto cm_gap_i2c_err;

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SW_RESET, NULL, 0);
	if (rc < 0)
		goto cm_gap_i2c_err;

	rc = 0;
	sec_ts_reinit(ts);

	enable_irq(ts->client->irq);

	for (ii = 0; ii < (ts->tx_count * ts->rx_count * 2); ii += 2)
		pFrame[ii / 2] = rBuff[ii + 1] + (rBuff[ii] << 8);

	memset(&cm3[0][0], 0x00, ts->tx_count * ts->rx_count * 2);
	memset(&gap_x[0][0], 0x00, ts->tx_count * ts->rx_count * 2);
	memset(&gap_y[0][0], 0x00, ts->tx_count * ts->rx_count * 2);
	memset(&gap_max[0][0], 0x00, ts->tx_count * ts->rx_count * 2);

	memset(&spec[0][0], 0x00, ts->tx_count * ts->rx_count * 2);
	for (ii = 0; ii < ts->rx_count; ii++) {
		for (jj = 0; jj < ts->tx_count; jj++) {
			spec[ii][jj] = 7;
		}
	}
	spec[0][0] = 40;
	spec[0][ts->tx_count - 1] = 40;
	spec[0][ts->tx_count - 2] = 40;
	spec[ts->rx_count - 1][0] = 40;
	spec[ts->rx_count - 2][0] = 40;
	spec[ts->rx_count - 1][ts->tx_count - 2] = 40;
	spec[ts->rx_count - 2][ts->tx_count - 1] = 40;

	for (ii = 0; ii < ts->rx_count; ii++) {
		for (jj = 0; jj < ts->tx_count; jj++) {
			cm3[ii][jj] = pFrame[jj * ts->rx_count + ii];
		}
	}

	pr_cont("\n sec_input: cm3\n");
	for (ii = 0; ii < ts->rx_count; ii++) {
		pr_cont("sec_input: ");
		for (jj = 0; jj < ts->tx_count; jj++) {
			pr_cont("%d ", cm3[ii][jj]);
		}
		pr_cont("\n");
	}
	pr_cont("\n sec_input: ==============\n");

	for (ii = 0; ii < ts->rx_count; ii++) {
		for (jj = 0; jj < ts->tx_count - 1; jj++) {
			if ((ii > 0 && ii < (ts->rx_count - 1)) && (jj == 0 || jj == (ts->tx_count - 2))) {
				gap_x[ii][jj] = 0;
				continue;
			}

			if (cm3[ii][jj] > cm3[ii][jj + 1]) {
				gap_x[ii][jj] = (cm3[ii][jj] - cm3[ii][jj + 1]) * 100 / cm3[ii][jj];
			} else {
				gap_x[ii][jj] = (cm3[ii][jj + 1] - cm3[ii][jj]) * 100 / cm3[ii][jj + 1];
			}
		}
	}

	for (ii = 0; ii < ts->rx_count - 1; ii++) {
		for (jj = 0; jj < ts->tx_count; jj++) {
			if ((jj > 0 && jj < (ts->tx_count - 1)) && (ii == 0 || ii == (ts->rx_count - 2))) {
				gap_y[ii][jj] = 0;
				continue;
			}

			if (cm3[ii][jj] > cm3[ii + 1][jj]) {
				gap_y[ii][jj] = (cm3[ii][jj] - cm3[ii + 1][jj]) * 100 / cm3[ii][jj];
			} else {
				gap_y[ii][jj] = (cm3[ii + 1][jj] - cm3[ii][jj]) * 100 / cm3[ii + 1][jj];
			}
		}
	}

	for (ii = 0; ii < ts->rx_count; ii++) {
		for (jj = 0; jj < ts->tx_count; jj++) {
			gap_max[ii][jj] = max(gap_x[ii][jj], gap_y[ii][jj]);
		}
	}

	pr_cont("\n sec_input: GAP MAX\n");
	for (ii = 0; ii < ts->rx_count; ii++) {
		pr_cont("sec_input: ");
		for (jj = 0; jj < ts->tx_count; jj++) {
			if (gap_max[ii][jj] < spec[ii][jj]) {
				pr_cont("%d ", gap_max[ii][jj]);
			} else {
				pr_cont("%d(X) ", gap_max[ii][jj]);
				rc = 1;
			}
		}
		pr_cont("\n");
	}
	pr_cont("\n sec_input: ==============\n");

	if (rc == 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		send_event_to_user(ts, sec->cmd_param[0], UEVENT_OPEN_SHORT_FAIL);
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		send_event_to_user(ts, sec->cmd_param[0], UEVENT_OPEN_SHORT_PASS);
	}
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	kfree(rBuff);
	vfree(pFrame);
	return;

cm_gap_i2c_err:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SW_RESET, NULL, 0);
	enable_irq(ts->client->irq);
	sec_ts_reinit(ts);
	kfree(rBuff);
	vfree(pFrame);
	send_event_to_user(ts, sec->cmd_param[0], UEVENT_OPEN_SHORT_FAIL);
}

/*
 * bit		| [0]	[1]	..	[8]	[9]	..	[16]	[17]	..	[24]	..	[31]
 * byte[0]	| TX0	TX1	..	TX8	TX9	..	RX0	RX1	..	RX8	..	RX15
 * byte[1]	| RX16	RX17	..	RX24	RX25	..	RX32	F0	..		..
 */
#define OPEN_SHORT_TEST		1
#define CRACK_TEST		2
/*
#define BRIDGE_SHORT_TEST	3
*/
#define AG_SHORT_TEST		3/*4*/

static void run_trx_short_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	/* fix length: if all channel failed, can occur overflow */
	char buff[1024 + 256] = {0};
	char tempn[40] = {0};
	char tempv[25] = {0};
	int rc;
	int size = SEC_TS_SELFTEST_REPORT_SIZE + ts->tx_count * ts->rx_count * 2;
	char para = TO_TOUCH_MODE;
	u8 *rBuff = NULL;
	short *pFrame = NULL;
	int ii, jj;
	u8 data[32] = { 0 };
	int len = 0;
	int delay = 0;
	int checklen = 0;
	int sum = 0;

	if (sec->cmd_param[0] == AG_SHORT_TEST) {
		run_15khz_cm3_gap_read(sec);
		return;
	}

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	rBuff = kzalloc(size, GFP_KERNEL);
	if (!rBuff) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	memset(rBuff, 0x00, size);
	memset(data, 0x00, 32);

	disable_irq(ts->client->irq);

	/*
	 * old version remaining
	 * old version is not send parameter
	 */
	if (sec->cmd_param[0] == 0) {
		rc = execute_selftest(ts, false);
		ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
		enable_irq(ts->client->irq);

		if (rc > 0) {
			snprintf(buff, sizeof(buff), "%s", "OK");
			sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
			sec->cmd_state = SEC_CMD_STATUS_OK;
		} else {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		}

		input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
		kfree(rBuff);
		return;
	}

	input_info(true, &ts->client->dev, "%s: set power mode to test mode\n", __func__);
	data[0] = 0x02;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, data, 1);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: set test mode failed\n", __func__);
		goto err_trx_short;
	}

	input_info(true, &ts->client->dev, "%s: clear event stack\n", __func__);
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: clear event stack failed\n", __func__);
		goto err_trx_short;
	}

	if (sec->cmd_param[0] == OPEN_SHORT_TEST) {
		data[0] = 0xB7;
		len = 1;
		delay = 700;
		checklen = 8 * 4;
	} else if (sec->cmd_param[0] == CRACK_TEST) {
		data[0] = 0x81;
		data[1] = 0x01;
		len = 2;
		delay = 200;
		checklen = 8;
/*
	} else if (sec->cmd_param[0] == BRIDGE_SHORT_TEST) {
		data[0] = 0x81;
		data[1] = 0x02;
		len = 2;
		delay = 1000;
		checklen = 8;
*/
	}

	input_info(true, &ts->client->dev, "%s: self test start\n", __func__);
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST, data, len);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Send selftest cmd failed!\n", __func__);
		goto err_trx_short;
	}

	sec_ts_delay(delay);

	rc = sec_ts_wait_for_ready(ts, SEC_TS_VENDOR_ACK_SELF_TEST_DONE);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_trx_short;
	}

	input_info(true, &ts->client->dev, "%s: self test done\n", __func__);

	rc = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SELFTEST_RESULT, rBuff, size);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_trx_short;
	}


	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: mode changed failed!\n", __func__);
		goto err_trx_short;
	}

	sec_ts_reinit(ts);

	input_info(true, &ts->client->dev, "%s: %02X, %02X, %02X, %02X\n",
			__func__, rBuff[16], rBuff[17], rBuff[18], rBuff[19]);

	pFrame = vmalloc(ts->tx_count * ts->rx_count * 2);
	if (!pFrame)
		goto err_trx_short;

	for (ii = 0; ii < (ts->tx_count * ts->rx_count * 2); ii += 2)
		pFrame[ii / 2] = (rBuff[80 + ii + 1] << 8) + rBuff[80 + ii];

	input_info(true, &ts->client->dev, "%s: #29\n", __func__);
	for (ii = 0; ii < ts->rx_count; ii++) {
		pr_info("[sec_input]: ");
		for (jj = 0; jj < ts->tx_count; jj++)
			pr_cont(" %3d", pFrame[(jj * ts->rx_count) + ii]);
		pr_cont("\n");
	}
	vfree(pFrame);
	enable_irq(ts->client->irq);

	memcpy(data, &rBuff[48], 32);

	for (ii = 0; ii < 32; ii++)
		sum += data[ii];

	if (!sum)
		goto test_ok;

	for (ii = 0; ii < checklen; ii += 8) {
		int jj;
		long long lldata = 0;

		memcpy(&lldata, &data[ii], 8);
		if (!lldata)
			continue;

		memset(tempn, 0x00, 40);

		if (sec->cmd_param[0] == OPEN_SHORT_TEST) {
			if (ii / 8 == 0)
				snprintf(tempn, 40, " OPEN_SHORT(TX/RX_OPEN):");
			else if (ii / 8 == 1)
				snprintf(tempn, 40, " OPEN_SHORT(TX/RX_SHORT_TO_GND):");
			else if (ii / 8 == 2)
				snprintf(tempn, 40, " OPEN_SHORT(TX/RX_SHORT_TO_TX/RX):");
			else if (ii / 8 == 3)
				snprintf(tempn, 40, " OPEN_SHORT(TX/RX_SHORT_TO_RX/TX):");
		} else if (sec->cmd_param[0] == CRACK_TEST) {
			snprintf(tempn, 40, " CRACK:");
/*
		} else if (sec->cmd_param[0] == BRIDGE_SHORT_TEST) {
			snprintf(tempn, 40, "BRIDGE_SHORT:");
*/
		}
		strncat(buff, tempn, 40);

		for (jj = 0; jj < 8; jj++) {
			int lshift = 0;

			if (!data[ii + jj])
				continue;

			input_info(true, &ts->client->dev, "%s: [%d] %02X\n",
					__func__, ii + jj, data[ii + jj]);

			while (lshift <= 7) {
				if ((data[ii + jj] & (0x01 << lshift)) == 0) {
					lshift++;
					continue;
				}

				memset(tempv, 0x00, 25);

				if (jj == 0) {
					snprintf(tempv, 20, "TX%d,", lshift);
				} else if (jj == 1) {
					snprintf(tempv, 20, "TX%d,", lshift + 8);
				} else if (jj == 2) {
					snprintf(tempv, 20, "RX%d,", lshift);
				} else if (jj == 3) {
					snprintf(tempv, 20, "RX%d,", lshift + 8);
				} else if (jj == 4) {
					snprintf(tempv, 20, "RX%d,", lshift + 16);
				} else if (jj == 5) {
					snprintf(tempv, 20, "RX%d,", lshift + 24);
				} else if (jj == 6) {
					if (lshift == 0)
						snprintf(tempv, 20, "RX32,");
					else if (lshift == 1)
						snprintf(tempv, 20, "F0,");
					else if (lshift == 2)
						snprintf(tempv, 20, "F1,");
					else if (lshift == 3)
						snprintf(tempv, 20, "F2,");
					else
						snprintf(tempv, 20, "N,");
				} else {
					snprintf(tempv, 20, "N,");
				}
				input_info(true, &ts->client->dev, "%s: %s\n", __func__, tempv);

				strncat(buff, tempv, 25);
				lshift++;
			}
		}

	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	kfree(rBuff);
	send_event_to_user(ts, sec->cmd_param[0], UEVENT_OPEN_SHORT_FAIL);
	return;

test_ok:
	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	kfree(rBuff);
	send_event_to_user(ts, sec->cmd_param[0], UEVENT_OPEN_SHORT_PASS);
	return;

err_trx_short:

	ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &para, 1);
	enable_irq(ts->client->irq);

	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	kfree(rBuff);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	return;
}

static void run_lowpower_selftest(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[16] = { 0 };
	char rwbuf[2] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	disable_irq(ts->client->irq);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, rwbuf, 1);
	if (ret < 0)
		goto err_exit;

	rwbuf[0] = 0x5;
	rwbuf[1] = 0x0;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, rwbuf, 2);
	if (ret < 0)
		goto err_exit;

	sec_ts_delay(20);

	rwbuf[0] = 0x1;
	rwbuf[1] = 0x0;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_START_LOWPOWER_TEST, rwbuf, 1);
	if (ret < 0)
		goto err_exit;

	ret = sec_ts_wait_for_ready(ts,  SEC_TS_VENDOR_ACK_LOWPOWER_SELF_TEST_DONE);
	if (ret < 0)
		goto err_exit;

	rwbuf[0] = 0;
	rwbuf[1] = 0;
	/* success : 1, fail : 0 */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_START_LOWPOWER_TEST, rwbuf, 1);
	if (rwbuf[0] != 1) {
		input_err(true, &ts->client->dev,
				"%s: result failed, %d\n", __func__, rwbuf[0]);
		ret = -EIO;
		goto err_exit;
	}

	rwbuf[0] = 0x2;
	rwbuf[1] = 0x2;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, rwbuf, 2);
	if (ret < 0)
		goto err_exit;

	rwbuf[0] = 1;
	rwbuf[1] = 0;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, rwbuf, 1);
	if (ret < 0)
		goto err_exit;

err_exit:
	enable_irq(ts->client->irq);

	if (ret < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
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
	else if (cal_mode == AMBIENT_CAL)
		cmd = SEC_TS_CMD_CALIBRATION_AMBIENT;
#ifdef USE_PRESSURE_SENSOR
	else if (cal_mode == PRESSURE_CAL)
		cmd = SEC_TS_CMD_CALIBRATION_PRESSURE;
#endif
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

#ifdef USE_PRESSURE_SENSOR
	rc = sec_ts_execute_force_calibration(ts, PRESSURE_CAL);
	if (rc < 0)
		input_err(true, &ts->client->dev, "%s: fail to write PRESSURE CAL!\n", __func__);
#endif

	if (ts->plat_data->mis_cal_check) {
		sec_ts_delay(50);

		buff[0] = 0;
		rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, buff, 1);
		if (rc < 0) {
			input_err(true, &ts->client->dev,
					"%s: mis_cal_check error[1] ret: %d\n", __func__, rc);
		}

		buff[0] = 0x2;
		buff[1] = 0x2;
		rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, buff, 2);
		if (rc < 0) {
			input_err(true, &ts->client->dev,
					"%s: mis_cal_check error[2] ret: %d\n", __func__, rc);
		}

		input_err(true, &ts->client->dev, "%s: try mis Cal. check\n", __func__);
		rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_MIS_CAL_CHECK, NULL, 0);
		if (rc < 0) {
			input_err(true, &ts->client->dev,
					"%s: mis_cal_check error[3] ret: %d\n", __func__, rc);
		}
		sec_ts_delay(200);

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

	sec_execute_tclm_package(ts->tdata, 1);
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
	} else {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

}

#ifdef USE_PRESSURE_SENSOR
static void run_force_pressure_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	int rc;
	char data[3] = { 0 };

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (ts->touch_count > 0) {
		snprintf(buff, sizeof(buff), "%s", "NG_FINGER_ON");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out_force_pressure_cal;
	}

	disable_irq(ts->client->irq);

	rc = sec_ts_execute_force_calibration(ts, PRESSURE_CAL);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "FAIL");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	ts->pressure_cal_base = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_PRESSURE_BASE_CAL_COUNT);
	if (ts->pressure_cal_base == 0xFF)
		ts->pressure_cal_base = 0;
	if (ts->pressure_cal_base > 0xFD)
		ts->pressure_cal_base = 0xFD;

	/* Use TSP NV area : in this model, use only one byte
	 * data[0] : offset from user NVM storage
	 * data[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 * data[2] : write data
	 */
	data[0] = SEC_TS_NVM_OFFSET_PRESSURE_BASE_CAL_COUNT;
	data[1] = 0;
	data[2] = ts->pressure_cal_base + 1;

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, data, 3);
	if (rc < 0)
		input_err(true, &ts->client->dev,
				"%s: nvm write failed. ret: %d\n", __func__, rc);

	ts->pressure_cal_base = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_PRESSURE_BASE_CAL_COUNT);

	input_info(true, &ts->client->dev, "%s: count:%d\n", __func__, ts->pressure_cal_base);

	enable_irq(ts->client->irq);

out_force_pressure_cal:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

}

static void get_pressure_calibration_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	unsigned char count;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	count = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_PRESSURE_DELTA_CAL_COUNT);
	if (count == 0xFF)
		count = 0;

	snprintf(buff, sizeof(buff), "%d", count);
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void set_pressure_test_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	int ret;
	unsigned char data = TYPE_INVALID_DATA;
	unsigned char enable = 0;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (sec->cmd_param[0] == 1) {
		enable = 0x1;
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TEMPERATURE_COMP_MODE, &enable, 1);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out_test_mode;
		}

		ret = sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out_test_mode;
		}

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_GET_FACTORY_MODE, &enable, 1);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out_test_mode;
		}
	} else {
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELECT_PRESSURE_TYPE, &data, 1);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out_test_mode;
		}

		ret = sec_ts_release_tmode(ts);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out_test_mode;
		}

		enable = 0x0;
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TEMPERATURE_COMP_MODE, &enable, 1);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out_test_mode;
		}

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_GET_FACTORY_MODE, &enable, 1);
		if (ret < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out_test_mode;
		}
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

out_test_mode:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

int read_pressure_data(struct sec_ts_data *ts, u8 type, short *value)
{
	unsigned char data[6] = { 0 };
	short pressure[3] = { 0 };
	int ret, i;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF)
		return -ENODEV;

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SELECT_PRESSURE_TYPE, data, 1);
	if (ret < 0)
		return -EIO;

	if (data[0] != type) {
		data[1] = type;

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELECT_PRESSURE_TYPE, &data[1], 1);
		if (ret < 0)
			return -EIO;

		sec_ts_delay(50);
	}

	memset(data, 0x00, 6);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_READ_PRESSURE_DATA, data, 6);
	if (ret < 0)
		return -EIO;

	pressure[0] = (data[0] << 8 | data[1]);
	pressure[1] = (data[2] << 8 | data[3]);
	pressure[2] = (data[4] << 8 | data[5]);

	input_raw_info(true, &ts->client->dev, "%s: Type:%d, Left: %d, Center: %d, Rignt: %d\n",
			__func__, type, pressure[0], pressure[1], pressure[2]);

	memcpy(value, pressure, 3 * 2);

	for (i = 0; i < PRESSURE_CHANNEL_NUM; i++)
		ts->pressure_data[type][i] = pressure[i];

	return ret;
}

static void run_pressure_filtered_strength_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	short pressure[3] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	ret = read_pressure_data(ts, TYPE_SIGNAL_DATA, pressure);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "WRITE FAILED");
		goto error_read_str;
	}

	snprintf(buff, sizeof(buff), "%d,%d,%d", pressure[0], pressure[1], pressure[2]);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

error_read_str:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
}

static void run_pressure_strength_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	short pressure[3] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	ret = read_pressure_data(ts, TYPE_REMV_AMB_DATA, pressure);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "WRITE FAILED");
		goto error_read_str;
	}

	snprintf(buff, sizeof(buff), "%d,%d,%d", pressure[0], pressure[1], pressure[2]);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

error_read_str:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
}

static void run_pressure_rawdata_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	short pressure[3] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	ret = read_pressure_data(ts, TYPE_RAW_DATA, pressure);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "WRITE FAILED");
		goto error_read_rawdata;
	}

	snprintf(buff, sizeof(buff), "%d,%d,%d", pressure[0], pressure[1], pressure[2]);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

error_read_rawdata:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
}

static void run_pressure_offset_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	short pressure[3] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	ret = read_pressure_data(ts, TYPE_OFFSET_DATA_SEC, pressure);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "WRITE FAILED");
		goto error_read_str;
	}

	snprintf(buff, sizeof(buff), "%d,%d,%d", pressure[0], pressure[1], pressure[2]);

	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);

	return;

error_read_str:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
}

/*
 * index	Factory App		Firmware
 * 0		X			latest
 * 1		SUB(but not use)	SDC
 * 2		MAIN			SUB
 * 3		SVC			MAIN
 * 4		X			SVC
 *
 * change index value(+ 1) for mismatching about factory app and firmware
 */

static void set_pressure_strength(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 cal_data[18] = { 0 };
	u8 index;
	u8 data[3] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if ((sec->cmd_param[0] < 1) || (sec->cmd_param[0] > 3)) {
		input_info(true, &ts->client->dev, "%s: parameter error: %u\n",
			__func__, sec->cmd_param[0]);
		goto err_cmd_param_str;
	}

	index = (u8)sec->cmd_param[0] + 1;

	/* LEFT */
	cal_data[0] = (sec->cmd_param[1] >> 8);
	cal_data[1] = (sec->cmd_param[1] & 0xFF);
	/* CENTER */
	cal_data[2] = (sec->cmd_param[2] >> 8);
	cal_data[3] = (sec->cmd_param[2] & 0xFF);
	/* RIGHT */
	cal_data[4] = (sec->cmd_param[3] >> 8);
	cal_data[5] = (sec->cmd_param[3] & 0xFF);

	ret = sec_ts_write_factory_level(ts, index);
	if (ret < 0)
		goto err_cmd_param_str;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_GET_PRESSURE, cal_data, 6);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: pressure write failed. ret: %d\n", __func__, ret);
		goto err_comm_str;
	}

	sec_ts_delay(30);

	ts->pressure_cal_delta = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_PRESSURE_DELTA_CAL_COUNT);
	if (ts->pressure_cal_delta == 0xFF)
		ts->pressure_cal_delta = 0;

	if (ts->pressure_cal_delta > 0xFD)
		ts->pressure_cal_delta = 0xFD;

	/* Use TSP NV area : in this model, use only one byte
	 * data[0] : offset from user NVM storage
	 * data[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 * data[2] : write data
	 */
	data[0] = SEC_TS_NVM_OFFSET_PRESSURE_DELTA_CAL_COUNT;
	data[1] = 0;
	data[2] = ts->pressure_cal_delta + 1;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, data, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev,
				"%s: count nvm write failed. ret: %d\n", __func__, ret);

	ts->pressure_cal_delta = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_PRESSURE_DELTA_CAL_COUNT);

	input_info(true, &ts->client->dev, "%s: count:%d\n", __func__, ts->pressure_cal_delta);

	data[0] = SEC_TS_NVM_OFFSET_PRESSURE_INDEX;
	data[1] = 0;
	data[2] = index;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, data, 3);
	if (ret < 0)
		input_err(true, &ts->client->dev,
				"%s: index nvm write failed. ret: %d\n", __func__, ret);

	sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return;

err_comm_str:
	sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);
err_cmd_param_str:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void set_pressure_rawdata(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[SEC_CMD_STR_LEN] = {0};

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s", "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return;
}

static void set_pressure_data_index(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 data[30] = { 0 };
	u8 cal_data[6] = { 0 };
	u8 index;
	int ret;

	sec_cmd_set_default_result(sec);

	if ((sec->cmd_param[0] < 0) || (sec->cmd_param[0] > 4)) {
		input_info(true, &ts->client->dev, "%s: parameter error: %u\n",
			__func__, sec->cmd_param[0]);
		goto err_set_cmd_param_index;
	}

	memset(cal_data, 0x00, 6);
	memset(data, 0x00, 30);

	if (sec->cmd_param[0] == 0) {
		input_info(true, &ts->client->dev, "%s: clear calibration result\n", __func__);
/*
 * clear pressure calibrated data before force calibration.(need to change Zero Offset)
 */
		ret = sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);
		if (ret < 0)
			goto err_set_cmd_param_index;

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_GET_PRESSURE, cal_data, 6);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: clear pressure strength failed. ret: %d\n", __func__, ret);
			goto err_set_cmd_param_index;
		}

		sec_ts_delay(30);
		goto clear_index;
	}

	index = (u8)sec->cmd_param[0] + 1;

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_GET_FORCE_PRESSURE_DATA, data, 30);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read pressure data failed. ret: %d\n", __func__, ret);
		goto err_set_cmd_param_index;
	}

	memcpy(cal_data, &data[index * 6], 6);
	input_info(true, &ts->client->dev, "%s: index:%d: L:%d, C:%d, R:%d\n",
			__func__, index, (cal_data[0] << 8) | cal_data[1],
			(cal_data[2] << 8) | cal_data[3], (cal_data[4] << 8) | cal_data[5]);

	ret = sec_ts_write_factory_level(ts, index);
	if (ret < 0)
		goto err_set_cmd_param_index;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_GET_PRESSURE, cal_data, 6);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: re-write failed. ret: %d\n", __func__, ret);
		goto err_set_comm_index;
	}

	sec_ts_delay(30);

	/* Use TSP NV area : in this model, use only one byte
	 * buff[0] : offset from user NVM storage
	 * buff[1] : length of stroed data - 1 (ex. using 1byte, value is  1 - 1 = 0)
	 * buff[2] : write data
	 */
	memset(data, 0x00, 3);
	data[0] = SEC_TS_NVM_OFFSET_PRESSURE_INDEX;
	data[1] = 0;
	data[2] = (u8)(index & 0xFF);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, data, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm write failed. ret: %d\n", __func__, ret);
		goto err_set_comm_index;
	}

	sec_ts_delay(20);

	sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);

clear_index:
	snprintf(buff, sizeof(buff), "%s", "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return;

err_set_comm_index:
	sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);
err_set_cmd_param_index:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	return;

}
static void get_pressure_strength(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 index;
	u8 data[30] = { 0 };
	int ret;
	short pressure[3] = { 0 };

	sec_cmd_set_default_result(sec);

	if ((sec->cmd_param[0] < 1) || (sec->cmd_param[0] > 4))
		goto err_get_cmd_param_str;

	index = sec->cmd_param[0] + 1;

	memset(data, 0x00, 30);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_GET_FORCE_PRESSURE_DATA, data, 30);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: read pressure data failed. ret: %d\n", __func__, ret);
		goto err_set_comm_index;
	}

	pressure[0] = ((data[6 * index + 0] << 8) + data[6 * index + 1]);
	pressure[1] = ((data[6 * index + 2] << 8) + data[6 * index + 3]);
	pressure[2] = ((data[6 * index + 4] << 8) + data[6 * index + 5]);

	snprintf(buff, sizeof(buff), "%d,%d,%d", pressure[0], pressure[1], pressure[2]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &ts->client->dev, "%s: [%d] : %d, %d, %d\n",
			__func__, index, pressure[0], pressure[1], pressure[2]);

	return;

err_set_comm_index:
err_get_cmd_param_str:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void get_pressure_rawdata(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[SEC_CMD_STR_LEN] = {0};

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return;
}

static void get_pressure_data_index(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	int index = 0;

	sec_cmd_set_default_result(sec);

	index = get_tsp_nvm_data(ts, SEC_TS_NVM_OFFSET_PRESSURE_INDEX);
	if (index < 0) {
		goto err_get_index;
	} else {
		if (index == 0xFF)
			snprintf(buff, sizeof(buff), "%d", 0);
		else
			snprintf(buff, sizeof(buff), "%d", index - 1);
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_info(true, &ts->client->dev, "%s: %d\n",
			__func__, index);

	return;

err_get_index:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	return;

}

static void set_pressure_strength_clear(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	u8 data[3];
	u8 cal_data[6] = { 0 };
	int i;
	int ret;

	sec_cmd_set_default_result(sec);

	/* clear pressure calibrated data */
	/* i : 1 -> value of SDC, do not remove */
	for (i = 2; i < 5; i++) {
		u8 index = (u8)i;

		ret = sec_ts_write_factory_level(ts, index);
		if (ret < 0)
			goto err_set_level;

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_GET_PRESSURE, cal_data, 6);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: cmd write failed. ret: %d\n", __func__, ret);
			goto err_comm_str;
		}

		sec_ts_delay(30);
	}

	memset(data, 0x00, 3);
	data[0] = SEC_TS_NVM_OFFSET_PRESSURE_INDEX;
	data[1] = 0;
	data[2] = 0;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, data, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: nvm write failed. ret: %d\n", __func__, ret);
		goto err_comm_str;
	}

	sec_ts_delay(20);

	sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);

	snprintf(buff, sizeof(buff), "%s", "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	return;

err_comm_str:
	sec_ts_write_factory_level(ts, OFFSET_FW_NOSAVE);
err_set_level:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_FAIL;
}

static void get_pressure_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[SEC_CMD_STR_LEN] = {0};

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "300");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

/* low level is more sensitivity, except level-0(value 0) */
static void set_pressure_user_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	int ret;
	char addr[3] = { 0 };
	char data[2] = { 0 };
	int pressure_thd = 0;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if ((sec->cmd_param[0] < 1) || (sec->cmd_param[0] > 5))
		goto out_set_user_level;

	/*
	 * byte[0]: m_sponge_ifpacket_addr[7:0]
	 * byte[1]: m_sponge_ifpacket_addr[15:8]
	 * byte[n] : user data (max 32 bytes)
	 */
	addr[0] = SEC_TS_CMD_SPONGE_OFFSET_PRESSURE_LEVEL;
	addr[1] = 0x00;
	addr[2] = sec->cmd_param[0];

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_WRITE_PARAM, addr, 3);
	if (ret < 0)
		goto out_set_user_level;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_NOTIFY_PACKET, NULL, 0);
	if (ret < 0)
		goto out_set_user_level;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_READ_PARAM, addr, 2);
	if (ret < 0)
		goto out_set_user_level;

	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SPONGE_READ_PARAM, data, 1);
	if (ret < 0)
		goto out_set_user_level;

	input_info(true, &ts->client->dev, "%s: set user level: %d\n", __func__, data[0]);

	ts->pressure_user_level = data[0];

	addr[0] = SEC_TS_CMD_SPONGE_OFFSET_PRESSURE_THD_HIGH;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_READ_PARAM, addr, 2);
	if (ret < 0)
		goto out_set_user_level;

	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SPONGE_READ_PARAM, data, 2);
	if (ret < 0)
		goto out_set_user_level;

	pressure_thd = data[0] | data[1] << 8;
	input_info(true, &ts->client->dev, "%s: HIGH THD: %d\n", __func__, pressure_thd);

	addr[0] = SEC_TS_CMD_SPONGE_OFFSET_PRESSURE_THD_LOW;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_READ_PARAM, addr, 2);
	if (ret < 0)
		goto out_set_user_level;

	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SPONGE_READ_PARAM, data, 2);
	if (ret < 0)
		goto out_set_user_level;

	pressure_thd = data[0] | data[1] << 8;
	input_info(true, &ts->client->dev, "%s: LOW THD: %d\n", __func__, pressure_thd);

	snprintf(buff, sizeof(buff), "%s", "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);
	return;

out_set_user_level:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_exit(sec);
}

static void get_pressure_user_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = {0};
	char addr[3] = { 0 };
	char data[2] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%d", ts->pressure_user_level);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	addr[0] = SEC_TS_CMD_SPONGE_OFFSET_PRESSURE_LEVEL;
	addr[1] = 0x00;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_READ_PARAM, addr, 2);
	if (ret < 0)
		goto out_get_user_level;

	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_SPONGE_READ_PARAM, data, 1);
	if (ret < 0)
		goto out_get_user_level;

	input_err(true, &ts->client->dev, "%s: set user level: %d\n", __func__, data[0]);
	ts->pressure_user_level = data[0];

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);
	return;

out_get_user_level:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_exit(sec);
}

static void set_pressure_setting_mode_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		input_err(true, &ts->client->dev, "%s: not support param\n", __func__);
		goto NG;
	}

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		goto NG;
	}

	ts->pressure_setting_mode = sec->cmd_param[0];

	input_info(true, &ts->client->dev,
				"%s: %s\n", __func__, ts->pressure_setting_mode ? "enabled" : "disabled");

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

static void run_pressure_jitter_p2p_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[6];
	int i;
	int rc;
	short pressure[3] = {0, 0, 0};
	short min_pressure[3] = {30000, 30000, 30000};
	short max_pressure[3] = {-30000, -30000, -30000};

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		goto err_jitter_p2p;
	}

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_OFF, NULL, 0);
	if (rc < 0)
		goto err_jitter_p2p;

	sec_ts_delay(20);

	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (rc < 0)
		goto err_jitter_p2p;

	sec_ts_delay(20);

	data[0] = 0x00;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELECT_PRESSURE_TYPE, data, 1);
	if (rc < 0)
		goto err_jitter_p2p;

	sec_ts_delay(700);

	data[0] = 0x00;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, data, 1);
	if (rc < 0)
		goto err_jitter_p2p;

	sec_ts_delay(20);

	data[0] = 0x02;
	data[1] = 0x02;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CHG_SYSMODE, data, 2);
	if (rc < 0)
		goto err_jitter_p2p;

	memset(data, 0x00, 6);
	for (i = 0; i < 120; i++) {
		rc = ts->sec_ts_i2c_read(ts, SEC_TS_CMD_READ_PRESSURE_DATA, data, 6);
		if (rc < 0)
			goto err_jitter_p2p;

		pressure[0] = (data[0] << 8 | data[1]);
		pressure[1] = (data[2] << 8 | data[3]);
		pressure[2] = (data[4] << 8 | data[5]);

		max_pressure[0] = max(max_pressure[0], pressure[0]);
		max_pressure[1] = max(max_pressure[1], pressure[1]);
		max_pressure[2] = max(max_pressure[2], pressure[2]);

		min_pressure[0] = min(min_pressure[0], pressure[0]);
		min_pressure[1] = min(min_pressure[1], pressure[1]);
		min_pressure[2] = min(min_pressure[2], pressure[2]);

		sec_ts_delay(10);
	}

	memset(data, 0x00, 6);
	data[0] = 0x01;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_STATEMANAGE_ON, data, 1);
	if (rc < 0)
		goto err_jitter_p2p;

	input_info(true, &ts->client->dev, "[L]%d,%d:%d [R]%d,%d:%d [C]%d,%d:%d\n",
			max_pressure[0], min_pressure[0], max_pressure[0] - min_pressure[0],
			max_pressure[1], min_pressure[1], max_pressure[1] - min_pressure[1],
			max_pressure[2], min_pressure[2], max_pressure[2] - min_pressure[2]);

	sec_ts_locked_release_all_finger(ts);

	sec_ts_reinit(ts);

	snprintf(buff, sizeof(buff), "%d,%d,%d", max_pressure[0] - min_pressure[0],
			max_pressure[1] - min_pressure[1], max_pressure[2] - min_pressure[2]);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	return;

err_jitter_p2p:
	sec_ts_locked_release_all_finger(ts);

	sec_ts_reinit(ts);

	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}
#endif

static void factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);

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

	get_chip_vendor(sec);
	get_chip_name(sec);
	get_fw_ver_bin(sec);
	get_fw_ver_ic(sec);

	run_rawcap_read(sec);
	get_gap_data(sec);
	run_reference_read(sec);

	run_raw_p2p_read_all(sec);

	run_self_rawcap_read(sec);
	get_self_channel_data(sec, TYPE_OFFSET_DATA_SDC);
	run_self_reference_read(sec);
	get_self_channel_data(sec, TYPE_OFFSET_DATA_SEC);

	get_wet_mode(sec);
	get_mis_cal_info(sec);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

out:
	input_info(true, &ts->client->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static void set_lowpower_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

#if 0
	/* set lowpower mode by spay, edge_swipe function. */
	ts->lowpower_mode = sec->cmd_param[0];
#endif
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec_cmd_set_cmd_exit(sec);

	return;

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

static void spay_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1)
		goto NG;

	if (sec->cmd_param[0]) {
		if (ts->use_sponge)
			ts->lowpower_mode |= SEC_TS_MODE_SPONGE_SPAY;
	} else {
		if (ts->use_sponge)
			ts->lowpower_mode &= ~SEC_TS_MODE_SPONGE_SPAY;
	}

	input_info(true, &ts->client->dev, "%s: %02X\n", __func__, ts->lowpower_mode);

	if (ts->use_sponge)
		sec_ts_set_custom_library(ts);

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

static void set_aod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[10] = {0x02, 0};
	int ret, i;

	sec_cmd_set_default_result(sec);

	input_info(true, &ts->client->dev, "%s: w:%d, h:%d, x:%d, y:%d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2], sec->cmd_param[3]);

	for (i = 0; i < 4; i++) {
		data[i * 2 + 2] = sec->cmd_param[i] & 0xFF;
		data[i * 2 + 3] = (sec->cmd_param[i] >> 8) & 0xFF;
		ts->rect_data[i] = sec->cmd_param[i];
	}

	if (ts->use_sponge) {
		disable_irq(ts->client->irq);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_WRITE_PARAM, &data[0], 10);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to write offset\n", __func__);
			goto NG;
		}

		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SPONGE_NOTIFY_PACKET, NULL, 0);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send notify\n", __func__);
			goto NG;
		}
		enable_irq(ts->client->irq);
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;
NG:
	enable_irq(ts->client->irq);
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}


static void get_aod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[8] = {0x02, 0};
	u16 rect_data[4] = {0, };
	int ret, i;

	sec_cmd_set_default_result(sec);

	if (ts->use_sponge) {
		disable_irq(ts->client->irq);
		ret = ts->sec_ts_read_sponge(ts, data, 8);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to read rect\n", __func__);
			goto NG;
		}
		enable_irq(ts->client->irq);
	}

	for (i = 0; i < 4; i++)
		rect_data[i] = (data[i * 2 + 1] & 0xFF) << 8 | (data[i * 2] & 0xFF);

	input_info(true, &ts->client->dev, "%s: w:%d, h:%d, x:%d, y:%d\n",
			__func__, rect_data[0], rect_data[1], rect_data[2], rect_data[3]);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;
NG:
	enable_irq(ts->client->irq);
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void aod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1)
		goto NG;

	if (sec->cmd_param[0]) {
		if (ts->use_sponge)
			ts->lowpower_mode |= SEC_TS_MODE_SPONGE_AOD;
	} else {
		if (ts->use_sponge)
			ts->lowpower_mode &= ~SEC_TS_MODE_SPONGE_AOD;
	}

	input_info(true, &ts->client->dev, "%s: %02X\n", __func__, ts->lowpower_mode);

	if (ts->use_sponge)
		sec_ts_set_custom_library(ts);

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

	memset(buff, 0, sizeof(buff));

	mutex_lock(&ts->device_mutex);

	if (sec->cmd_param[0] == 0) {	// edge handler
		if (sec->cmd_param[1] == 0) {	// clear
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

	} else if (sec->cmd_param[0] == 1) {	// normal mode
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
	} else if (sec->cmd_param[0] == 2) {	// landscape mode
		if (sec->cmd_param[1] == 0) {	// normal mode
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
		input_err(true, &ts->client->dev, "%s: not support DeX mode\n", __func__);
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
	char buff[SEC_CMD_STR_LEN] = {0};
	int rc;
	int size = SEC_TS_SELFTEST_REPORT_SIZE + ts->tx_count * ts->rx_count * 2;
	u8 *rBuff = NULL;
	int ii;
	u8 data[8] = { 0 };
	int result = 0;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	rBuff = kzalloc(size, GFP_KERNEL);
	if (!rBuff) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	memset(rBuff, 0x00, size);
	memset(data, 0x00, 8);

	disable_irq(ts->client->irq);

	sec_ts_locked_release_all_finger(ts);

	input_info(true, &ts->client->dev, "%s: set power mode to test mode\n", __func__);
	data[0] = 0x02;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, data, 1);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: set test mode failed\n", __func__);
		goto err_conn_check;
	}

	input_info(true, &ts->client->dev, "%s: clear event stack\n", __func__);
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: clear event stack failed\n", __func__);
		goto err_conn_check;
	}

	data[0] = 0xB7;
	input_info(true, &ts->client->dev, "%s: self test start\n", __func__);
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SELFTEST, data, 1);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Send selftest cmd failed!\n", __func__);
		goto err_conn_check;
	}

	sec_ts_delay(700);

	rc = sec_ts_wait_for_ready(ts, SEC_TS_VENDOR_ACK_SELF_TEST_DONE);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_conn_check;
	}

	input_info(true, &ts->client->dev, "%s: self test done\n", __func__);
	rc = ts->sec_ts_i2c_read(ts, SEC_TS_READ_SELFTEST_RESULT, rBuff, size);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: Selftest execution time out!\n", __func__);
		goto err_conn_check;
	}

	data[0] = TO_TOUCH_MODE;
	rc = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, data, 1);
	if (rc < 0) {
		input_err(true, &ts->client->dev, "%s: mode changed failed!\n", __func__);
		goto err_conn_check;
	}

	input_info(true, &ts->client->dev, "%s: %02X, %02X, %02X, %02X\n",
			__func__, rBuff[16], rBuff[17], rBuff[18], rBuff[19]);

	memcpy(data, &rBuff[48], 8);

	for (ii = 0; ii < 8; ii++) {
		input_info(true, &ts->client->dev, "%s: [%d] %02X\n", __func__, ii, data[ii]);

		result += data[ii];
	}

	if (result != 0)
		goto err_conn_check;

	enable_irq(ts->client->irq);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(rBuff);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
	return;

err_conn_check:
	data[0] = TO_TOUCH_MODE;
	ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, data, 1);
	enable_irq(ts->client->irq);

	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	kfree(rBuff);
	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void fix_active_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	ts->fix_active_mode = sec->cmd_param[0];

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: Touch is stopped!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	if (ts->power_status != SEC_TS_STATE_LPM) {
		if (ts->fix_active_mode)
			sec_ts_fix_tmode(ts, TOUCH_SYSTEM_MODE_TOUCH, TOUCH_MODE_STATE_TOUCH);
		else
			sec_ts_release_tmode(ts);
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void touch_aging_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct sec_ts_data *ts = container_of(sec, struct sec_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;
	u8 data;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	if (sec->cmd_param[0] == 1)
		data = 5;
	else
		data = 0;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_POWER_MODE, &data, 1);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, buff);
}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%s", "NA");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
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
				"%s: Failed to create sysfs attributes\n", __func__);
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
