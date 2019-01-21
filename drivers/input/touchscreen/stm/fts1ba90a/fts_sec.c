#ifdef SEC_TSP_FACTORY_TEST

#define BUFFER_MAX			((256 * 1024) - 16)

enum {
	TYPE_RAW_DATA = 0x31,
	TYPE_BASELINE_DATA = 0x32,
	TYPE_STRENGTH_DATA = 0x33,
};

enum {
	BUILT_IN = 0,
	UMS,
};

enum ito_error_type {
	ITO_FORCE_SHRT_GND		= 0x60,
	ITO_SENSE_SHRT_GND		= 0x61,
	ITO_FORCE_SHRT_VDD		= 0x62,
	ITO_SENSE_SHRT_VDD		= 0x63,
	ITO_FORCE_SHRT_FORCE	= 0x64,
	ITO_SENSE_SHRT_SENSE	= 0x65,
	ITO_FORCE_OPEN			= 0x66,
	ITO_SENSE_OPEN			= 0x67,
	ITO_KEY_OPEN			= 0x68
};

#ifdef FTS_SUPPORT_TOUCH_KEY
enum {
	TYPE_TOUCHKEY_RAW	= 0x34,
	TYPE_TOUCHKEY_STRENGTH	= 0x36,
};
#endif

#define FTS_COMP_DATA_HEADER_SIZE     16

enum fts_nvm_data_type {		/* Write Command */
	FTS_NVM_OFFSET_FAC_RESULT = 1,
	FTS_NVM_OFFSET_CAL_COUNT,
	FTS_NVM_OFFSET_DISASSEMBLE_COUNT,
	FTS_NVM_OFFSET_TUNE_VERSION,
	FTS_NVM_OFFSET_CAL_POSITION,
	FTS_NVM_OFFSET_HISTORY_QUEUE_COUNT,
	FTS_NVM_OFFSET_HISTORY_QUEUE_LASTP,
	FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO,
	FTS_NVM_OFFSET_PRESSURE_BASE_CAL_COUNT,
	FTS_NVM_OFFSET_PRESSURE_DELTA_CAL_COUNT,
	FTS_NVM_OFFSET_PRESSURE_INDEX,
};

struct fts_nvm_data_map {
	int type;
	int offset;
	int length;
};

#define NVM_CMD(mtype, moffset, mlength)		.type = mtype,	.offset = moffset,	.length = mlength

/* This Flash Meory Map is FIXED by STM firmware
 * Do not change MAP.
 */
struct fts_nvm_data_map nvm_data[] = {
	{NVM_CMD(0,						0x00, 0),},
	{NVM_CMD(FTS_NVM_OFFSET_FAC_RESULT,			0x00, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_CAL_COUNT,			0x01, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_DISASSEMBLE_COUNT,		0x02, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_TUNE_VERSION,			0x03, 2),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_CAL_POSITION,			0x05, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_HISTORY_QUEUE_COUNT,		0x06, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_HISTORY_QUEUE_LASTP,		0x07, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO,		0x08, 20),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_PRESSURE_BASE_CAL_COUNT,	0x23, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_PRESSURE_DELTA_CAL_COUNT,	0x24, 1),},	/* SEC */
	{NVM_CMD(FTS_NVM_OFFSET_PRESSURE_INDEX,			0x25, 1),},	/* SEC */
};
#define FTS_NVM_OFFSET_ALL	29

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_mis_cal_info(void *device_data);
static void get_wet_mode(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void get_checksum_data(void *device_data);
static void run_reference_read(void *device_data);
static void get_reference(void *device_data);
static void run_rawcap_read(void *device_data);
static void run_rawcap_read_all(void *device_data);
static void get_rawcap(void *device_data);
static void run_delta_read(void *device_data);
static void get_delta(void *device_data);
#ifdef TCLM_CONCEPT
static void get_pat_information(void *device_data);
static void set_external_factory(void *device_data);
#endif
static void run_ix_data_read(void *device_data);
static void run_ix_data_read_all(void *device_data);
static void run_self_raw_read(void *device_data);
static void run_self_raw_read_all(void *device_data);
static void run_trx_short_test(void *device_data);
static void check_connection(void *device_data);
static void get_cx_data(void *device_data);
static void run_cx_data_read(void *device_data);
static void get_cx_all_data(void *device_data);
static void run_cx_gap_data_x_all(void *device_data);
static void run_cx_gap_data_y_all(void *device_data);
static void get_strength_all_data(void *device_data);
#ifdef FTS_SUPPORT_TOUCH_KEY
static void run_key_cx_data_read(void *device_data);
#endif
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
static void run_force_pressure_calibration(void *device_data);
static void set_pressure_test_mode(void *device_data);
static void run_pressure_strength_read_all(void *device_data);
static void run_pressure_rawdata_read_all(void *device_data);
static void run_pressure_ix_data_read_all(void *device_data);
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
#endif

static void set_tsp_test_result(void *device_data);
static void get_tsp_test_result(void *device_data);
static void increase_disassemble_count(void *device_data);
static void get_disassemble_count(void *device_data);

#ifdef CONFIG_GLOVE_TOUCH
static void glove_mode(void *device_data);
static void get_glove_sensitivity(void *device_data);
#endif
static void clear_cover_mode(void *device_data);
static void report_rate(void *device_data);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
static void interrupt_control(void *device_data);
#endif

static void set_wirelesscharger_mode(void *device_data);
static void set_grip_data(void *device_data);
static void dead_zone_enable(void *device_data);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
static void drawing_test_enable(void *device_data);
#endif
static void spay_enable(void *device_data);
static void aot_enable(void *device_data);
static void aod_enable(void *device_data);
static void set_aod_rect(void *device_data);
static void get_aod_rect(void *device_data);
static void dex_enable(void *device_data);
static void brush_enable(void *device_data);
static void set_touchable_area(void *device_data);
static void delay(void *device_data);
static void debug(void *device_data);
static void factory_cmd_result_all(void *device_data);
static void run_force_calibration(void *device_data);
static void set_factory_level(void *device_data);

static void not_support_cmd(void *device_data);

static ssize_t fts_scrub_position(struct device *dev,
		struct device_attribute *attr, char *buf);

struct sec_cmd ft_commands[] = {
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("get_config_ver", get_config_ver),},
	{SEC_CMD("get_threshold", get_threshold),},
	{SEC_CMD("module_off_master", module_off_master),},
	{SEC_CMD("module_on_master", module_on_master),},
	{SEC_CMD("module_off_slave", not_support_cmd),},
	{SEC_CMD("module_on_slave", not_support_cmd),},
	{SEC_CMD("get_chip_vendor", get_chip_vendor),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("get_mis_cal_info", get_mis_cal_info),},
	{SEC_CMD("get_wet_mode", get_wet_mode),},
	{SEC_CMD("get_module_vendor", not_support_cmd),},
	{SEC_CMD("get_x_num", get_x_num),},
	{SEC_CMD("get_y_num", get_y_num),},
	{SEC_CMD("get_checksum_data", get_checksum_data),},
	{SEC_CMD("run_reference_read", run_reference_read),},
	{SEC_CMD("get_reference", get_reference),},
	{SEC_CMD("run_rawcap_read", run_rawcap_read),},
	{SEC_CMD("run_rawcap_read_all", run_rawcap_read_all),},
	{SEC_CMD("get_rawcap", get_rawcap),},
	{SEC_CMD("run_delta_read", run_delta_read),},
	{SEC_CMD("get_delta", get_delta),},
	{SEC_CMD("run_cs_raw_read_all", run_rawcap_read_all),},
	{SEC_CMD("run_cs_delta_read_all", get_strength_all_data),},
#ifdef TCLM_CONCEPT
	{SEC_CMD("get_pat_information", get_pat_information),},
	{SEC_CMD("set_external_factory", set_external_factory),},
#endif
	{SEC_CMD("run_ix_data_read", run_ix_data_read),},
	{SEC_CMD("run_ix_data_read_all", run_ix_data_read_all),},
	{SEC_CMD("run_self_raw_read", run_self_raw_read),},
	{SEC_CMD("run_self_raw_read_all", run_self_raw_read_all),},
	{SEC_CMD("run_trx_short_test", run_trx_short_test),},
	{SEC_CMD("check_connection", check_connection),},
	{SEC_CMD("get_cx_data", get_cx_data),},
	{SEC_CMD("run_cx_data_read", run_cx_data_read),},
	{SEC_CMD("run_cx_data_read_all", get_cx_all_data),},
	{SEC_CMD("get_cx_all_data", get_cx_all_data),},
	{SEC_CMD("run_cx_gap_data_x_all", run_cx_gap_data_x_all),},
	{SEC_CMD("run_cx_gap_data_y_all", run_cx_gap_data_y_all),},
	{SEC_CMD("get_strength_all_data", get_strength_all_data),},
#ifdef FTS_SUPPORT_TOUCH_KEY
	{SEC_CMD("run_key_cx_data_read", run_key_cx_data_read),},
#endif
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	{SEC_CMD("run_force_pressure_calibration", run_force_pressure_calibration),},
	{SEC_CMD("set_pressure_test_mode", set_pressure_test_mode),},
	{SEC_CMD("run_pressure_strength_read_all", run_pressure_strength_read_all),},
	{SEC_CMD("run_pressure_rawdata_read_all", run_pressure_rawdata_read_all),},
	{SEC_CMD("run_pressure_ix_data_read_all", run_pressure_ix_data_read_all),},
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
#endif
	{SEC_CMD("set_tsp_test_result", set_tsp_test_result),},
	{SEC_CMD("get_tsp_test_result", get_tsp_test_result),},
	{SEC_CMD("increase_disassemble_count", increase_disassemble_count),},
	{SEC_CMD("get_disassemble_count", get_disassemble_count),},
#ifdef CONFIG_GLOVE_TOUCH
	{SEC_CMD("glove_mode", glove_mode),},
	{SEC_CMD("get_glove_sensitivity", get_glove_sensitivity),},
#endif
	{SEC_CMD("clear_cover_mode", clear_cover_mode),},
	{SEC_CMD("report_rate", report_rate),},
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	{SEC_CMD("interrupt_control", interrupt_control),},
#endif
	{SEC_CMD("set_wirelesscharger_mode", set_wirelesscharger_mode),},
	{SEC_CMD("set_grip_data", set_grip_data),},
	{SEC_CMD("dead_zone_enable", dead_zone_enable),},
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	{SEC_CMD("drawing_test_enable", drawing_test_enable),},
#endif
	{SEC_CMD("spay_enable", spay_enable),},
	{SEC_CMD("aot_enable", aot_enable),},
	{SEC_CMD("aod_enable", aod_enable),},
	{SEC_CMD("set_aod_rect", set_aod_rect),},
	{SEC_CMD("get_aod_rect", get_aod_rect),},
	{SEC_CMD("dex_enable", dex_enable),},
	{SEC_CMD("brush_enable", brush_enable),},
	{SEC_CMD("set_touchable_area", set_touchable_area),},
	{SEC_CMD("delay", delay),},
	{SEC_CMD("debug", debug),},
	{SEC_CMD("factory_cmd_result_all", factory_cmd_result_all),},
	{SEC_CMD("run_force_calibration", run_force_calibration),},
	{SEC_CMD("set_factory_level", set_factory_level),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

void send_event_to_user(struct fts_ts_info *info, int number, int val)
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
	else
		snprintf(result, 32, "RESULT=NULL");
	strncat(result, eol, 1);

	input_info(true, &info->client->dev, "%s: time:%s, feature:%s, result:%s\n", __func__, timestamp, feature, result);
	event[0] = timestamp;
	event[1] = feature;
	event[2] = test;
	event[3] = result;
	event[4] = NULL;

	kobject_uevent_env(&info->sec.fac_dev->kobj, KOBJ_CHANGE, event);
}

static ssize_t read_ito_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %02X%02X%02X%02X\n", __func__,
			info->ito_test[0], info->ito_test[1],
			info->ito_test[2], info->ito_test[3]);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%02X%02X%02X%02X",
			info->ito_test[0], info->ito_test[1],
			info->ito_test[2], info->ito_test[3]);
}

static ssize_t read_raw_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	int ii, ret = 0;
	char *buffer = NULL;
	char temp[10] = { 0 };

	buffer = vzalloc(info->SenseChannelLength * info->ForceChannelLength * 6);
	if (!buffer)
		return -ENOMEM;

	for (ii = 0; ii < (info->SenseChannelLength * info->ForceChannelLength - 1); ii++) {
		snprintf(temp, 6, "%d ", info->pFrame[ii]);
		strncat(buffer, temp, 6);

		memset(temp, 0x00, 10);
	}

	snprintf(temp, 6, "%d", info->pFrame[ii]);
	strncat(buffer, temp, 6);

	ret = snprintf(buf, info->SenseChannelLength * info->ForceChannelLength * 6, buffer);
	vfree(buffer);

	return ret;
}

static ssize_t read_multi_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->multi_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->multi_count);
}

static ssize_t clear_multi_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	info->multi_count = 0;
	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_wet_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->wet_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->wet_count);
}


static ssize_t clear_wet_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	info->wet_count = 0;
	info->dive_count = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_comm_err_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->comm_err_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->comm_err_count);
}


static ssize_t clear_comm_err_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	info->comm_err_count = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_module_id_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "ST%02X%04X%02X%c%01X%02X%02X",
			info->panel_revision, info->fw_main_version_of_ic,
			info->test_result.data[0],
			info->tdata->tclm_string[info->tdata->nvdata.cal_position].s_name,
			info->tdata->nvdata.cal_count & 0xF,
			info->pressure_cal_base, info->pressure_cal_delta);
}

static ssize_t read_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	u8 buffer[10] = { 0 };

	snprintf(buffer, 9, info->board->firmware_name + 8);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "STM_%s", buffer);
}

static ssize_t read_checksum_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->checksum_result);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->checksum_result);
}


static ssize_t clear_checksum_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	info->checksum_result = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t clear_holding_time_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	info->time_longest = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_holding_time_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %ld\n", __func__,
			info->time_longest);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%ld", info->time_longest);
}

static ssize_t read_all_touch_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: touch:%d, force:%d, aod:%d, spay:%d\n", __func__,
			info->all_finger_count, info->all_force_count,
			info->all_aod_tap_count, info->all_spay_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE,
			"\"TTCN\":\"%d\",\"TFCN\":\"%d\",\"TACN\":\"%d\",\"TSCN\":\"%d\"",
			info->all_finger_count, info->all_force_count,
			info->all_aod_tap_count, info->all_spay_count);
}

static ssize_t clear_all_touch_count_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	info->all_force_count = 0;
	info->all_aod_tap_count = 0;
	info->all_spay_count = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t read_z_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: max:%d, min:%d, sum:%d\n", __func__,
			info->max_z_value, info->min_z_value,
			info->sum_z_value);

	if (info->all_finger_count > 0)
		return snprintf(buf, SEC_CMD_BUF_SIZE,
				"\"TMXZ\":\"%d\",\"TMNZ\":\"%d\",\"TAVZ\":\"%d\"",
				info->max_z_value, info->min_z_value,
				info->sum_z_value / info->all_finger_count);
	else
		return snprintf(buf, SEC_CMD_BUF_SIZE,
				"\"TMXZ\":\"%d\",\"TMNZ\":\"%d\"",
				info->max_z_value, info->min_z_value);

}

static ssize_t clear_z_value_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	info->max_z_value = 0;
	info->min_z_value = 0xFFFFFFFF;
	info->sum_z_value = 0;
	info->all_finger_count = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t sensitivity_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	unsigned char wbuf[2] = { 0 };
	unsigned long value = 0;
	int ret = 0;

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret != 0)
		return ret;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return -EPERM;
	}

	wbuf[0] = FTS_CMD_SENSITIVITY_MODE;
	if (value)
		wbuf[1] = 0x24; /* enable */
	else
		wbuf[1] = 0xFF; /* disable */

	ret = fts_write_reg(info, wbuf, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: write failed. ret: %d\n", __func__, ret);
		return ret;
	}

	fts_delay(30);

	input_info(true, &info->client->dev, "%s: %d\n", __func__, value);
	return count;
}

static ssize_t sensitivity_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	u8 rbuf[10] = { 0 };
	u8 reg_read = FTS_READ_SENSITIVITY_VALUE;
	int ret, i;
	s16 value[5];

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return -EPERM;
	}

	ret = info->fts_read_reg(info, &reg_read, 1, rbuf, 10);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: read failed ret = %d\n", __func__, ret);
		return ret;
	}

	for (i = 0; i < 5; i++)
		value[i] = rbuf[i * 2] + (rbuf[i * 2 + 1] << 8);

	input_info(true, &info->client->dev, "%s: %d,%d,%d,%d,%d\n", __func__,
			value[0], value[1], value[2], value[3], value[4]);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d,%d,%d,%d,%d",
			value[0], value[1], value[2], value[3], value[4]);
}

#ifdef FTS_SUPPORT_PRESSURE_SENSOR
static ssize_t pressure_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[256] = { 0 };

	if (info->lowpower_flag & FTS_MODE_PRESSURE)
		snprintf(buff, sizeof(buff), "1");
	else
		snprintf(buff, sizeof(buff), "0");

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);
}

static ssize_t pressure_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	int ret;
	unsigned long value = 0;

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret != 0)
		return ret;

	if (value == 1)
		info->lowpower_flag |= FTS_MODE_PRESSURE;
	else
		info->lowpower_flag &= ~FTS_MODE_PRESSURE;

#ifdef FTS_SUPPORT_SPONGELIB
	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
			&info->lowpower_flag, sizeof(info->lowpower_flag));
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
#endif

	input_info(true, &info->client->dev, "%s: %d\n", __func__, value);
	return count;
}
#endif

/*
 * read_support_feature function
 * returns the bit combination of specific feature that is supported.
 */
static ssize_t read_support_feature(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u32 feature = 0;

	if (info->board->support_aot)
		feature |= INPUT_FEATURE_SUPPORT_AOT;

	snprintf(buff, sizeof(buff), "%d", feature);
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);
}

#ifdef FTS_SUPPORT_SPONGELIB
static ssize_t get_lp_dump(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	u16 addr = FTS_CMD_SPONGE_LP_DUMP;
	u8 sponge_data[8] = { 0, };
	u16 current_index;
	int i, ret = 0;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		return snprintf(buf, SEC_CMD_BUF_SIZE, "TSP turned off");
	}

	fts_interrupt_set(info, INT_DISABLE);

	ret = info->fts_read_from_sponge(info, addr, sponge_data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: Failed to read from Sponge\n", __func__);
		snprintf(buf, SEC_CMD_BUF_SIZE, "NG, Failed to read from Sponge");
		goto out;
	}

	current_index = (sponge_data[1] & 0xFF) << 8 | (sponge_data[0] & 0xFF);
	if (current_index > 1000 || current_index < 500) {
		input_err(true, &info->client->dev,
				"Failed to read Sponge LP log %d\n", current_index);
		snprintf(buf, SEC_CMD_BUF_SIZE,
				"NG, Failed to read Sponge LP log, current_index=%d",
				current_index);
		goto out;
	}

	input_info(true, &info->client->dev,
			"%s: DEBUG current_index = %d\n", __func__, current_index);

	/* sponge has 62 stacks for LP dump */
	for (i = 61; i >= 0; i--) {
		u16 data0, data1, data2, data3;
		char buff[30] = { 0, };

		addr = current_index - (8 * i);
		if (addr < 500)
			addr += FTS_CMD_SPONGE_LP_DUMP;

		ret = info->fts_read_from_sponge(info, addr, sponge_data, 8);
		if (ret < 0) {
			input_err(true, &info->client->dev,
					"%s: Failed to read from Sponge\n", __func__);
			snprintf(buf, SEC_CMD_BUF_SIZE,
					"NG, Failed to read from Sponge, addr=%d",
					addr);
			goto out;
		}

		data0 = (sponge_data[1] & 0xFF) << 8 | (sponge_data[0] & 0xFF);
		data1 = (sponge_data[3] & 0xFF) << 8 | (sponge_data[2] & 0xFF);
		data2 = (sponge_data[5] & 0xFF) << 8 | (sponge_data[4] & 0xFF);
		data3 = (sponge_data[7] & 0xFF) << 8 | (sponge_data[6] & 0xFF);
		if (data0 || data1 || data2 || data3) {
			snprintf(buff, sizeof(buff),
					"%d: %04x%04x%04x%04x\n",
					addr, data0, data1, data2, data3);
			strncat(buf, buff, sizeof(buff));
		}
	}

out:
	fts_interrupt_set(info, INT_ENABLE);

	return strlen(buf);
}
#endif

static ssize_t prox_power_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: %d\n", __func__,
			info->prox_power_off);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->prox_power_off);
}

static ssize_t prox_power_off_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	int ret, data;

	ret = kstrtoint(buf, 10, &data);
	if (ret < 0)
		return ret;

	input_info(true, &info->client->dev, "%s: %d\n", __func__, data);

	info->prox_power_off = data;

	return count;
}

static DEVICE_ATTR(ito_check, 0444, read_ito_check_show, NULL);
static DEVICE_ATTR(raw_check, 0444, read_raw_check_show, NULL);
static DEVICE_ATTR(multi_count, 0644, read_multi_count_show, clear_multi_count_store);
static DEVICE_ATTR(wet_mode, 0644, read_wet_mode_show, clear_wet_mode_store);
static DEVICE_ATTR(comm_err_count, 0644, read_comm_err_count_show, clear_comm_err_count_store);
static DEVICE_ATTR(module_id, 0444, read_module_id_show, NULL);
static DEVICE_ATTR(vendor, 0444, read_vendor_show, NULL);
static DEVICE_ATTR(checksum, 0644, read_checksum_show, clear_checksum_store);
static DEVICE_ATTR(holding_time, 0644, read_holding_time_show, clear_holding_time_store);
static DEVICE_ATTR(all_touch_count, 0644,
				read_all_touch_count_show, clear_all_touch_count_store);
static DEVICE_ATTR(z_value, 0644, read_z_value_show, clear_z_value_store);
static DEVICE_ATTR(sensitivity_mode, 0664, sensitivity_mode_show, sensitivity_mode_store);
static DEVICE_ATTR(scrub_pos, 0444, fts_scrub_position, NULL);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
static DEVICE_ATTR(pressure_enable, 0644, pressure_enable_show, pressure_enable_store);
#endif
static DEVICE_ATTR(support_feature, 0444, read_support_feature, NULL);
#ifdef FTS_SUPPORT_SPONGELIB
static DEVICE_ATTR(get_lp_dump, 0444, get_lp_dump, NULL);
#endif
static DEVICE_ATTR(prox_power_off, 0664, prox_power_off_show, prox_power_off_store);

static struct attribute *sec_touch_facotry_attributes[] = {
	&dev_attr_scrub_pos.attr,
	&dev_attr_ito_check.attr,
	&dev_attr_raw_check.attr,
	&dev_attr_multi_count.attr,
	&dev_attr_wet_mode.attr,
	&dev_attr_comm_err_count.attr,
	&dev_attr_module_id.attr,
	&dev_attr_vendor.attr,
	&dev_attr_checksum.attr,
	&dev_attr_holding_time.attr,
	&dev_attr_all_touch_count.attr,
	&dev_attr_z_value.attr,
	&dev_attr_sensitivity_mode.attr,
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	&dev_attr_pressure_enable.attr,
#endif
	&dev_attr_support_feature.attr,
#ifdef FTS_SUPPORT_SPONGELIB
	&dev_attr_get_lp_dump.attr,
#endif
	&dev_attr_prox_power_off.attr,
	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};

static ssize_t fts_get_cmoffset_dump(struct fts_ts_info *info, char *buf, u8 position)
{
	u8 regaddr[4] = { 0 };
	u8 *rbuff;
	int ret, i, j, size = info->SenseChannelLength * info->ForceChannelLength;
	u32 signature;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return snprintf(buf, info->proc_size, "TSP turned off");
	}

	if (info->reset_is_on_going) {
		input_err(true, &info->client->dev, "%s: Reset is ongoing!\n", __func__);
		return snprintf(buf, info->proc_size, "Reset is ongoing");
	}

	rbuff = kzalloc(size, GFP_KERNEL);
	if (!rbuff) {
		input_err(true, &info->client->dev, "%s: alloc failed\n", __func__);
		return snprintf(buf, info->proc_size, "NG, mem alloc failed");
	}

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_interrupt_set(info, INT_DISABLE);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	/* Request SEC factory debug data from flash */
	regaddr[0] = 0xA4;
	regaddr[1] = 0x06;
	regaddr[2] = 0x92;
	regaddr[3] = position;
	ret = info->fts_write_reg(info, regaddr, 4);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: failed to request flash data ret: %d\n", __func__, ret);
		snprintf(buf, info->proc_size, "NG, failed to request flash data, %d", ret);
		goto out;
	}

	ret = fts_fw_wait_for_echo_event(info, regaddr, 4);
	if (ret < 0) {
		snprintf(buf, info->proc_size, "NG, timeout, %d", ret);
		goto out;
	}

	/* read header info */
	regaddr[0] = 0xA6;
	regaddr[1] = 0x00;
	regaddr[2] = 0x00;
	ret = info->fts_read_reg(info, &regaddr[0], 3, rbuff, 8);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: read header failed. ret: %d\n", __func__, ret);
		snprintf(buf, info->proc_size, "NG, failed to read header, %d", ret);
		goto out;
	}

	signature = rbuff[3] << 24 | rbuff[2] << 16 | rbuff[1] << 8 | rbuff[0];
	input_info(true, &info->client->dev,
			"%s: position:%d, signature:%08X (%X), validation:%X, try count:%X\n",
			__func__, position, signature, SEC_OFFSET_SIGNATURE, rbuff[4], rbuff[5]);

	if (signature != SEC_OFFSET_SIGNATURE) {
		input_err(true, &info->client->dev, "%s: cmoffset[%d], signature is mismatched\n",
				__func__, position);
		snprintf(buf, info->proc_size, "NG, signature mismatched %08X", signature);
		goto out;
	}

	/* read history data */
	regaddr[0] = 0xA6;
	regaddr[1] = 0x00;
	regaddr[2] = (u8)info->SenseChannelLength;
	ret = info->fts_read_reg(info, &regaddr[0], 3, rbuff, size);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: read data failed. ret: %d\n", __func__, ret);
		snprintf(buf, info->proc_size, "NG, failed to read data %d", ret);
		goto out;
	}

	memset(buf, 0x00, info->proc_size);
	for (i = 0; i < info->ForceChannelLength; i++) {
		char buff[4] = { 0 };
		for (j = 0; j < info->SenseChannelLength; j++) {
			snprintf(buff, sizeof(buff), " %d", rbuff[i * info->SenseChannelLength + j]);
			strncat(buf, buff, sizeof(buff));
		}
		snprintf(buff, sizeof(buff), "\n");
		strncat(buf, buff, sizeof(buff));
	}

out:
	input_err(true, &info->client->dev, "%s: pos:%d, buf size:%d\n", __func__, position, strlen(buf));

	fts_interrupt_set(info, INT_ENABLE);
	kfree(rbuff);
	return strlen(buf);
}

static void enter_factory_mode(struct fts_ts_info *info, bool fac_mode)
{
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN)
		return;

	info->fts_systemreset(info);
	fts_delay(50);
	info->fts_wait_for_ready(info);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	if (fac_mode) {
		// Auto-Tune without saving
		fts_execute_autotune(info, false);

		fts_delay(50);
	}

	fts_set_scanmode(info, info->scan_mode);
}

static int fts_check_index(struct fts_ts_info *info)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int node;

	if (sec->cmd_param[0] < 0
			|| sec->cmd_param[0] >= info->SenseChannelLength
			|| sec->cmd_param[1] < 0
			|| sec->cmd_param[1] >= info->ForceChannelLength) {

		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &info->client->dev, "%s: parameter error: %u,%u\n",
				__func__, sec->cmd_param[0], sec->cmd_param[1]);
		node = -1;
		return node;
	}
	node = sec->cmd_param[1] * info->SenseChannelLength + sec->cmd_param[0];
	input_info(true, &info->client->dev, "%s: node = %d\n", __func__, node);
	return node;
}

static ssize_t fts_scrub_position(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	input_info(true, &info->client->dev, "%s: %d %d %d\n",
			__func__, info->scrub_id, info->scrub_x, info->scrub_y);
	snprintf(buff, sizeof(buff), "%d %d %d", info->scrub_id, info->scrub_x, info->scrub_y);

	info->scrub_x = 0;
	info->scrub_y = 0;

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s\n", buff);
}

#if 0 //def CONFIG_TRUSTONIC_TRUSTED_UI
static void tui_mode_cmd(struct fts_ts_info *info)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[16] = "TUImode:FAIL";

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%s", "NA");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[64] = { 0 };
	int retval = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	retval = fts_fw_update_on_hidden_menu(info, sec->cmd_param[0]);

	if (retval < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &info->client->dev, "%s: failed [%d]\n", __func__, retval);
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		input_info(true, &info->client->dev, "%s: success [%d]\n", __func__, retval);
	}
}

static int fts_get_channel_info(struct fts_ts_info *info)
{
	int rc = -1;
	u8 regAdd[1] = { FTS_READ_PANEL_INFO };
	u8 data[11] = { 0 };

	memset(data, 0x0, FTS_EVENT_SIZE);

	rc = info->fts_read_reg(info, regAdd, 1, data, 11);
	if (rc <= 0) {
		info->ForceChannelLength = 0;
		info->SenseChannelLength = 0;
		input_err(true, &info->client->dev, "%s: Get channel info Read Fail!!\n", __func__);
		return rc;
	}

	info->ForceChannelLength = data[8]; // Number of TX CH
	info->SenseChannelLength = data[9]; // Number of RX CH

	info->ICXResolution = (data[0] << 8) | data[1]; // X resolution of IC
	info->ICYResolution = (data[2] << 8) | data[3]; // Y resolution of IC

	return rc;
}

void fts_print_frame(struct fts_ts_info *info, short *min, short *max)
{
	int i = 0;
	int j = 0;
	u8 *pStr = NULL;
	u8 pTmp[16] = { 0 };

	pStr = kzalloc(BUFFER_MAX, GFP_KERNEL);
	if (pStr == NULL)
		return;

	snprintf(pTmp, 4, "    ");
	strncat(pStr, pTmp, 4);

	for (i = 0; i < info->SenseChannelLength; i++) {
		snprintf(pTmp, 6, "Rx%02d  ", i);
		strncat(pStr, pTmp, 6);
	}

	input_raw_info(true, &info->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, 6 * (info->SenseChannelLength + 1));
	snprintf(pTmp, 2, " +");
	strncat(pStr, pTmp, 2);

	for (i = 0; i < info->SenseChannelLength; i++) {
		snprintf(pTmp, 6, "------");
		strncat(pStr, pTmp, 6);
	}

	input_raw_info(true, &info->client->dev, "%s\n", pStr);

	for (i = 0; i < info->ForceChannelLength; i++) {
		memset(pStr, 0x0, 6 * (info->SenseChannelLength + 1));
		snprintf(pTmp, 7, "Tx%02d | ", i);
		strncat(pStr, pTmp, 7);


		for (j = 0; j < info->SenseChannelLength; j++) {
			snprintf(pTmp, 6, "%5d ", info->pFrame[(i * info->SenseChannelLength) + j]);

			if (i > 0) {
				if (info->pFrame[(i * info->SenseChannelLength) + j] < *min)
					*min = info->pFrame[(i * info->SenseChannelLength) + j];

				if (info->pFrame[(i * info->SenseChannelLength) + j] > *max)
					*max = info->pFrame[(i * info->SenseChannelLength) + j];
			}
			strncat(pStr, pTmp, 6);
		}
		input_raw_info(true, &info->client->dev, "%s\n", pStr);
	}

	kfree(pStr);
}

int fts_read_frame(struct fts_ts_info *info, u8 type, short *min, short *max)
{
	struct FTS_SyncFrameHeader *pSyncFrameHeader;

	u8 regAdd[8] = { 0 };

	unsigned int totalbytes = 0;
	u8 pRead[info->ForceChannelLength * info->SenseChannelLength * 3 + 1];
	int rc = 0;
	int ret = 0;
	int i = 0;
	int retry = 10;

	/* Request Data Type */
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = (u8)type;
	info->fts_write_reg(info, &regAdd[0], 3);
	fts_delay(50);

	do {
		regAdd[0] = 0xA6;
		regAdd[1] = 0x00;
		regAdd[2] = 0x00;
		ret = info->fts_read_reg(info, &regAdd[0], 3, &pRead[0], FTS_COMP_DATA_HEADER_SIZE);
		if (ret <= 0) {
			input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
			rc = -3;
			goto ErrorExit;
		}

		pSyncFrameHeader = (struct FTS_SyncFrameHeader *) &pRead[0];

		if ((pSyncFrameHeader->header == 0xA5) && (pSyncFrameHeader->host_data_mem_id == type))
			break;

		fts_delay(100);
	} while (retry--);

	if (retry == 0) {
		input_err(true, &info->client->dev,
				"%s: didn't match header or id. header = %02X, id = %02X\n",
				__func__, pSyncFrameHeader->header, pSyncFrameHeader->host_data_mem_id);
		rc = -4;
		goto ErrorExit;
	}

	totalbytes = (info->ForceChannelLength * info->SenseChannelLength  * 2);

	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = FTS_COMP_DATA_HEADER_SIZE + pSyncFrameHeader->dbg_frm_len;
	ret = info->fts_read_reg(info, &regAdd[0], 3, &pRead[0], totalbytes);
	if (ret <= 0) {
		input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
		rc = -5;
		goto ErrorExit;
	}

	for (i = 0; i < totalbytes / 2; i++)
		info->pFrame[i] = (short)(pRead[i * 2] + (pRead[i * 2 + 1] << 8));

	switch (type) {
	case TYPE_RAW_DATA:
		input_raw_info(true, &info->client->dev, "%s: [Raw Data]\n", __func__);
		break;
	case TYPE_STRENGTH_DATA:
		input_raw_info(true, &info->client->dev, "%s: [Strength Data]\n", __func__);
		break;
	case TYPE_BASELINE_DATA:
		input_raw_info(true, &info->client->dev, "%s: [Baseline Data]\n", __func__);
		break;
	}

	fts_print_frame(info, min, max);

ErrorExit:
	return rc;
}

void fts_get_sec_ito_test_result(struct fts_ts_info *info)
{
	struct sec_cmd_data *sec = &info->sec;
	struct fts_sec_panel_test_result result[10];
	u8 regAdd[3] = { 0 };
	u8 data[sizeof(struct fts_sec_panel_test_result) * 10 + 2] = { 0 };
	int ret, i, max_count = 0;
	u8 length = sizeof(data);
	u8 buff[100] = { 0 };
	u8 pos_buf[6] = { 0 };

	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = 0x94;
	ret = info->fts_write_reg(info, regAdd, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to write request cmd, %d\n", __func__, ret);
		goto done;
	}

	ret = fts_fw_wait_for_echo_event(info, regAdd, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to get echo, %d\n", __func__, ret);
		goto done;
	}

	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x00;
	ret = info->fts_read_reg(info, regAdd, 3, data, length);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read data, %d\n", __func__, ret);
		goto done;
	}

	memcpy(result, &data[2], length - 2);
	memset(info->ito_result, 0x00, FTS_ITO_RESULT_PRINT_SIZE);

	snprintf(buff, sizeof(buff), "%s: test count - sub:%d, main:%d\n", __func__, data[0], data[1]);
	input_info(true, &info->client->dev, "%s", buff);
	strncat(info->ito_result, buff, sizeof(buff));

	snprintf(buff, sizeof(buff), "ITO:              /   TX_GAP_MAX   /   RX_GAP_MAX\n");
	input_info(true, &info->client->dev, "%s", buff);
	strncat(info->ito_result, buff, sizeof(buff));

	for (i = 0; i < 10; i++) {
		switch (result[i].flag) {
		case OFFSET_FAC_SUB:
			snprintf(pos_buf, sizeof(pos_buf), "SUB ");
			break;
		case OFFSET_FAC_MAIN:
			snprintf(pos_buf, sizeof(pos_buf), "MAIN");
			break;
		case OFFSET_FAC_NOSAVE:
		default:
			snprintf(pos_buf, sizeof(pos_buf), "NONE");
			break;
		}

		snprintf(buff, sizeof(buff), "ITO: [%3d] %d-%s / Tx%02d,Rx%02d: %3d / Tx%02d,Rx%02d: %3d\n",
				result[i].num_of_test, result[i].flag, pos_buf,
				result[i].tx_of_txmax_gap, result[i].rx_of_txmax_gap,
				result[i].max_of_tx_gap,
				result[i].tx_of_rxmax_gap, result[i].rx_of_rxmax_gap,
				result[i].max_of_rx_gap);
		input_info(true, &info->client->dev, "%s", buff);
		strncat(info->ito_result, buff, sizeof(buff));

		/* when count is over 200, it restart from 1 */
		if (result[i].num_of_test > result[max_count].num_of_test + 100)
			continue;
		if (result[i].num_of_test > result[max_count].num_of_test)
			max_count = i;
		if (result[i].num_of_test == 1 && result[max_count].num_of_test == 200)
			max_count = i;
	}

	input_info(true, &info->client->dev, "%s: latest test is %d\n",
			__func__, result[max_count].num_of_test);

done:
	if (sec->cmd_all_factory_state != SEC_CMD_STATUS_RUNNING)
		return;

	if (ret < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_X");
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_Y");
	} else {
		snprintf(buff, sizeof(buff), "0,%d", result[max_count].max_of_rx_gap);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_X");
		snprintf(buff, sizeof(buff), "0,%d", result[max_count].max_of_tx_gap);
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_Y");
	}
}

int fts_set_sec_ito_test_result(struct fts_ts_info *info)
{
	struct sec_cmd_data *sec = &info->sec;
	u8 regAdd[3] = { 0 };
	int ret = -EINVAL;
	u8 buff[10] = { 0 };

	if (!info->factory_position) {
		input_err(true, &info->client->dev, "%s: not save, factory level = %d\n",
				__func__, info->factory_position);
		goto out;
	}

	regAdd[0] = 0xC7;
	regAdd[1] = 0x06;
	regAdd[2] = info->factory_position;
	ret = info->fts_write_reg(info, regAdd, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to write fac position, %d\n", __func__, ret);
		goto out;
	}

	regAdd[0] = 0xA4;
	regAdd[1] = 0x05;
	regAdd[2] = 0x04;
	ret = info->fts_write_reg(info, regAdd, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to write panel cfg area, %d\n", __func__, ret);
		goto out;
	}

	fts_delay(200);
	ret = fts_fw_wait_for_echo_event(info, regAdd, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to get echo, %d\n", __func__, ret);
		goto out;
	}

	input_info(true, &info->client->dev, "%s: position %d result is saved\n", __func__, info->factory_position);
	return 0;

out:
	if (sec->cmd_all_factory_state != SEC_CMD_STATUS_RUNNING)
		return ret;

	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_X");
	sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CH_OPEN/SHORT_TEST_Y");
	return ret;
}

void fts_checking_miscal(struct fts_ts_info *info, int testmode)
{
	u8 regAdd[3] = { 0 };
	u8 data[2] = { 0 };
	u16 jitter_avg = 0, miscal_thd = 0;
	int ret, diff_sum = 0, i;
	short min = 0x7FFF;
	short max = 0x8000;

	info->miscal_result = MISCAL_PASS;

	if (testmode == SAVE_MISCAL_REF_RAW) {
		/* store miscal ref raw data after CX2=0 : in autotune */
		fts_read_frame(info, TYPE_RAW_DATA, &min, &max);
		memcpy(&info->miscal_ref_raw[0], &info->pFrame[0],
				info->ForceChannelLength * info->SenseChannelLength * sizeof(short));
		input_info(true, &info->client->dev, "%s: miscal ref raw data is saved\n", __func__);
		return;
	} else if (testmode != OPEN_SHORT_CRACK_TEST) {
		return;
	}

	/* checking miscal ref raw is saved or not */
	for (i = 0; i < info->ForceChannelLength * info->SenseChannelLength; i++) {
		if (info->miscal_ref_raw[i] != 0)
			break;
	}

	if (i == info->ForceChannelLength * info->SenseChannelLength) {
		input_info(true, &info->client->dev,
				"%s: miscal ref raw data is not saved\n", __func__);
		return;
	}

	info->miscal_result = MISCAL_FAIL;

	/* get the raw data after CX2=0 : in selftest */
	fts_read_frame(info, TYPE_RAW_DATA, &min, &max);

	regAdd[0] = 0xC7;
	regAdd[1] = 0x0A;
	ret = info->fts_read_reg(info, regAdd, 2, data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read jitter avg\n", __func__);
		return;
	}

	jitter_avg = data[0] << 8 | data[1];

	regAdd[0] = 0xC7;
	regAdd[1] = 0x0B;
	ret = info->fts_read_reg(info, regAdd, 2, data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read miscal threshold\n", __func__);
		return;
	}

	miscal_thd = data[0] << 8 | data[1];

	/* compare raw data between autotune and selftest */
	for (i = 0; i < info->ForceChannelLength * info->SenseChannelLength; i++) {
		short node_diff = abs(info->miscal_ref_raw[i] - info->pFrame[i]);

		info->pFrame[i] = node_diff;

		if (node_diff > jitter_avg)
			diff_sum += node_diff - jitter_avg;
	}
	input_raw_info(true, &info->client->dev, "%s: [miscal diff data]\n", __func__);
	fts_print_frame(info, &min, &max);

	if (diff_sum < miscal_thd)
		info->miscal_result = MISCAL_PASS;

	input_raw_info(true, &info->client->dev, "%s: jitter avg:%d, threshold:%d, diff sum:%d, miscal:%s\n",
			__func__, jitter_avg, miscal_thd, diff_sum,
			info->miscal_result == MISCAL_PASS ? "PASS" : "FAIL");
}

int fts_panel_ito_test(struct fts_ts_info *info, int testmode)
{
	struct sec_cmd_data *sec = &info->sec;
	u8 cmd = FTS_READ_ONE_EVENT;
	u8 regAdd[4] = { 0 };
	u8 data[FTS_EVENT_SIZE];
	int i;
	bool matched = false;
	int retry = 0;
	int result = 0;
	u8 buff[50] = { 0 };

	info->fts_systemreset(info);

	info->fts_wait_for_ready(info);

	fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true);

	fts_release_all_finger(info);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	fts_interrupt_set(info, INT_DISABLE);

	regAdd[0] = 0xA4;
	regAdd[1] = 0x04;
	switch (testmode) {
	case OPEN_TEST:
		regAdd[2] = 0x00;
		regAdd[3] = 0x30;
		break;

	case OPEN_SHORT_CRACK_TEST:
	case SAVE_MISCAL_REF_RAW:
	default:
		regAdd[2] = 0xFF;
		regAdd[3] = 0x01;
		break;
	}

	info->fts_write_reg(info, &regAdd[0], 4); // ITO test command
	fts_delay(100);

	memset(info->ito_test, 0x0, 4);

	memset(data, 0x0, FTS_EVENT_SIZE);
	while (info->fts_read_reg(info, &cmd, 1, (u8 *)data, FTS_EVENT_SIZE)) {
		if ((data[0] == FTS_EVENT_STATUS_REPORT) && (data[1] == 0x01)) {  // Check command ECHO - finished
			for (i = 0; i < 4; i++) {
				if (data[i + 2] != regAdd[i]) {
					matched = false;
					break;
				}
				matched = true;
			}

			if (matched == true)
				break;
		} else if (data[0] == FTS_EVENT_ERROR_REPORT) {
			info->ito_test[0] = data[0];
			info->ito_test[1] = data[1];
			info->ito_test[2] = data[2];
			info->ito_test[3] = 0x00;
			result = -1;

			switch (data[1]) {
			case ITO_FORCE_SHRT_GND:
				input_info(true, &info->client->dev, "%s: Force channel [%d] short to GND\n",
						__func__, data[2]);
				break;

			case ITO_SENSE_SHRT_GND:
				input_info(true, &info->client->dev, "%s: Sense channel [%d] short to GND\n",
						__func__, data[2]);
				break;

			case ITO_FORCE_SHRT_VDD:
				input_info(true, &info->client->dev, "%s: Force channel [%d] short to VDD\n",
						__func__, data[2]);
				break;

			case ITO_SENSE_SHRT_VDD:
				input_info(true, &info->client->dev, "%s: Sense channel [%d] short to VDD\n",
						__func__, data[2]);
				break;

			case ITO_FORCE_SHRT_FORCE:
				input_info(true, &info->client->dev, "%s: Force channel [%d] short to force\n",
						__func__, data[2]);
				break;

			case ITO_SENSE_SHRT_SENSE:
				input_info(true, &info->client->dev, "%s: Sennse channel [%d] short to sense\n",
						__func__, data[2]);
				break;

			case ITO_FORCE_OPEN:
				input_info(true, &info->client->dev, "%s: Force channel [%d] open\n",
						__func__, data[2]);
				break;

			case ITO_SENSE_OPEN:
				input_info(true, &info->client->dev, "%s: Sense channel [%d] open\n",
						__func__, data[2]);
				break;

			case ITO_KEY_OPEN:
				input_info(true, &info->client->dev, "%s: Key channel [%d] open\n",
						__func__, data[2]);
				break;

			default:
				input_info(true, &info->client->dev, "%s: unknown event %02x %02x %02x %02x %02x %02x %02x %02x\n",
						__func__, data[0], data[1], data[2], data[3],
						data[4], data[5], data[6], data[7]);
				break;
			}
		}

		if (retry++ > 50) {
			result = -1;
			input_err(true, &info->client->dev, "%s: Time over - wait for result of ITO test\n", __func__);
			break;
		}
		fts_delay(20);
	}

	if (info->board->item_version == 2 && sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		if (result < 0)
			snprintf(buff, sizeof(buff), "%d", ITO_FAIL);
		else
			snprintf(buff, sizeof(buff), "%d", ITO_PASS);
		sec_cmd_set_cmd_result_all(sec, buff, sizeof(buff), "CH_OPEN/SHORT_TEST");
	} else if (info->board->item_version > 2) {
		if (fts_set_sec_ito_test_result(info) >= 0)
			fts_get_sec_ito_test_result(info);
	}

	fts_checking_miscal(info, testmode);

	info->fts_systemreset(info);

	/* wait for ready event */
	info->fts_wait_for_ready(info);

	if (info->flip_enable) {
		fts_set_cover_type(info, true);
	} else {
		info->touch_functions = (info->touch_functions & (~FTS_TOUCHTYPE_BIT_COVER)) |
					FTS_TOUCHTYPE_DEFAULT_ENABLE;
#ifdef CONFIG_GLOVE_TOUCH
		if (info->glove_enabled)
			info->touch_functions = info->touch_functions | FTS_TOUCHTYPE_BIT_GLOVE;
		else
			info->touch_functions = info->touch_functions & (~FTS_TOUCHTYPE_BIT_GLOVE);
#endif
	}

	regAdd[0] = FTS_CMD_SET_GET_TOUCHTYPE;
	regAdd[1] = (u8)(info->touch_functions & 0xFF);
	regAdd[2] = (u8)(info->touch_functions >> 8);
	fts_write_reg(info, &regAdd[0], 3);
	fts_delay(10);

#ifdef FTS_SUPPORT_TA_MODE
	if (info->TA_Pluged) {
		u8 wiredCharger;

		regAdd[0] = FTS_CMD_SET_GET_CHARGER_MODE;
		fts_read_reg(info, &regAdd[0], 1, &data[0], 1);
		wiredCharger = data[0] | FTS_BIT_CHARGER_MODE_WIRE_CHARGER;
		regAdd[1] = { wiredCharger };
		fts_write_reg(info, &regAdd[0], 2);
		fts_delay(10);
	}
#endif

	info->touch_count = 0;

	fts_set_scanmode(info, info->scan_mode);

	input_raw_info(true, &info->client->dev, "%s: mode:%d [%s] %02X %02X %02X %02X\n",
			__func__, testmode, result < 0 ? "FAIL" : "PASS",
			info->ito_test[0], info->ito_test[1],
			info->ito_test[2], info->ito_test[3]);

	return result;
}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "ST%02X%04X",
			info->panel_revision,
			info->fw_main_version_of_bin);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_BIN");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	fts_get_version_info(info);

	snprintf(buff, sizeof(buff), "ST%02X%04X",
			info->panel_revision,
			info->fw_main_version_of_ic);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_config_ver(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[20] = { 0 };

	snprintf(buff, sizeof(buff), "%s_ST_%04X",
			info->board->model_name ?: info->board->project_name ?: "STM",
			info->config_version_of_ic);

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	u8 regAdd[2];
	u8 buff[16] = { 0 };
	u8 data[5] = { 0 };
	u16 finger_threshold = 0;
	int rc;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	sec->cmd_state = SEC_CMD_STATUS_RUNNING;

	regAdd[0] = FTS_CMD_SET_GET_TOUCH_MODE_FOR_THRESHOLD;
	regAdd[1] = 0x00;
	info->fts_write_reg(info, &regAdd[0], 2);
	fts_delay(50);

	regAdd[0] = FTS_CMD_SET_GET_TOUCH_THRESHOLD;
	rc = info->fts_read_reg(info, &regAdd[0], 1, data, 2);
	if (rc <= 0) {
		input_err(true, &info->client->dev, "%s: Get threshold Read Fail!! [Data : %2X%2X]\n",
				__func__, data[0], data[1]);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	finger_threshold = (u16)(data[0] << 8 | data[1]);

	snprintf(buff, sizeof(buff), "%d", finger_threshold);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void module_off_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[3] = { 0 };
	int ret = 0;

	ret = fts_stop_device(info, false);

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
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void module_on_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[3] = { 0 };
	int ret = 0;

	ret = fts_start_device(info);

	if (info->input_dev->disabled)
		fts_stop_device(info, info->lowpower_flag);

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
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	strncpy(buff, "STM", sizeof(buff));
	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_VENDOR");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	memcpy(buff, info->firmware_name + 8, 9);

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_NAME");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_mis_cal_info(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%d", info->miscal_result);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "MIS_CAL");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;
}

static void get_wet_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = { 0 };
	u8 data[2] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "WET_MODE");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	regAdd[0] = 0xC7;
	regAdd[1] = 0x03;
	ret = info->fts_read_reg(info, &regAdd[0], 2, &data[0], 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "WET_MODE");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	snprintf(buff, sizeof(buff), "%d", data[0]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "WET_MODE");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_x_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%d", info->SenseChannelLength);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_y_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "%d", info->ForceChannelLength);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_checksum_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };
	int rc;
	u32 checksum_data;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	rc = info->fts_get_sysinfo_data(info, FTS_SI_CONFIG_CHECKSUM, 4, (u8 *)&checksum_data);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: Get checksum data Read Fail!! [Data : %08X]\n",
				__func__, checksum_data);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	snprintf(buff, sizeof(buff), "%08X", checksum_data);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_reference_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	fts_read_frame(info, TYPE_BASELINE_DATA, &min, &max);
	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_reference(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = fts_check_index(info);
	if (node < 0)
		return;

	val = info->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "RAW_DATA");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	fts_read_frame(info, TYPE_RAW_DATA, &min, &max);
	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "RAW_DATA");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_rawcap_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;
	char *all_strbuff;
	int i, j;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	all_strbuff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 7 + 1, GFP_KERNEL);
	if (!all_strbuff) {
		input_err(true, &info->client->dev, "%s: alloc failed\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	enter_factory_mode(info, true);
	fts_read_frame(info, TYPE_RAW_DATA, &min, &max);
	for (j = 0; j < info->ForceChannelLength; j++) {
		for (i = 0; i < info->SenseChannelLength; i++) {
			snprintf(buff, sizeof(buff), "%d,", info->pFrame[j * info->SenseChannelLength + i]);
			strncat(all_strbuff, buff, sizeof(buff));
		}
	}
	enter_factory_mode(info, false);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, all_strbuff, strlen(all_strbuff));
	input_info(true, &info->client->dev, "%s: %ld\n", __func__, strlen(all_strbuff));
	kfree(all_strbuff);
}

static void get_rawcap(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = fts_check_index(info);
	if (node < 0)
		return;

	val = info->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_delta_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	fts_read_frame(info, TYPE_STRENGTH_DATA, &min, &max);
	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_strength_all_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;
	char *all_strbuff;
	int i, j;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	all_strbuff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 7 + 1, GFP_KERNEL);
	if (!all_strbuff) {
		input_err(true, &info->client->dev, "%s: alloc failed\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_read_frame(info, TYPE_STRENGTH_DATA, &min, &max);

	for (i = 0; i < info->ForceChannelLength; i++) {
		for (j = 0; j < info->SenseChannelLength; j++) {
			snprintf(buff, sizeof(buff), "%d,", info->pFrame[(i * info->SenseChannelLength) + j]);
			strncat(all_strbuff, buff, sizeof(buff));
		}
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, all_strbuff, strlen(all_strbuff));
	input_info(true, &info->client->dev, "%s: %ld\n", __func__, strlen(all_strbuff));
	kfree(all_strbuff);
}

static void get_delta(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = fts_check_index(info);
	if (node < 0)
		return;

	val = info->pFrame[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

#ifdef TCLM_CONCEPT
static void get_pat_information(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[50] = { 0 };

	sec_cmd_set_default_result(sec);

	/* fixed tune version will be saved at excute autotune */
	snprintf(buff, sizeof(buff), "C%02XT%04X.%4s%s%c%d%c%d%c%d",
		info->tdata->nvdata.cal_count, info->tdata->nvdata.tune_fix_ver,
		info->tdata->tclm_string[info->tdata->nvdata.cal_position].f_name,
		(info->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L " : " ",
		info->tdata->cal_pos_hist_last3[0], info->tdata->cal_pos_hist_last3[1],
		info->tdata->cal_pos_hist_last3[2], info->tdata->cal_pos_hist_last3[3],
		info->tdata->cal_pos_hist_last3[4], info->tdata->cal_pos_hist_last3[5]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_external_factory(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->tdata->external_factory = true;
	snprintf(buff, sizeof(buff), "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

static void fts_read_ix_data(struct fts_ts_info *info, bool allnode)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	int rc;

	u16 max_tx_ix_sum = 0;
	u16 min_tx_ix_sum = 0xFFFF;

	u16 max_rx_ix_sum = 0;
	u16 min_rx_ix_sum = 0xFFFF;

	u8 data[(info->ForceChannelLength + info->SenseChannelLength) * 2 + 1];

	u8 regAdd[FTS_EVENT_SIZE];

	u8 dataID;

	u16 force_ix_data[info->ForceChannelLength * 2 + 1];
	u16 sense_ix_data[info->SenseChannelLength * 2 + 1];

	int buff_size, j;
	char *mbuff = NULL;
	int num, n, a, fzero;
	char cnum;
	int i = 0;
	u16 comp_start_tx_addr, comp_start_rx_addr;
	unsigned int rx_num, tx_num;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true); // Clear FIFO

	fts_release_all_finger(info);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif


	fts_interrupt_set(info, INT_DISABLE);

	// Request compensation data type
	dataID = 0x52;
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = dataID; // SS - CX total
	info->fts_write_reg(info, &regAdd[0], 3);
	rc = fts_fw_wait_for_echo_event(info, &regAdd[0], 3);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		fts_interrupt_set(info, INT_ENABLE);
		goto out;
	}
	fts_interrupt_set(info, INT_ENABLE);

	// Read Header
	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x00;
	info->fts_read_reg(info, &regAdd[0], 3, &data[0], FTS_COMP_DATA_HEADER_SIZE);

	if ((data[0] != 0xA5) && (data[1] != dataID)) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	tx_num = data[4];
	rx_num = data[5];

	/* Read TX IX data */
	comp_start_tx_addr = (u16)FTS_COMP_DATA_HEADER_SIZE;
	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(comp_start_tx_addr >> 8);
	regAdd[2] = (u8)(comp_start_tx_addr & 0xFF);
	info->fts_read_reg(info, &regAdd[0], 3, &data[0], tx_num * 2);

	for (i = 0; i < tx_num; i++) {
		force_ix_data[i] = data[2 * i] | data[2 * i + 1] << 8;

		if (max_tx_ix_sum < force_ix_data[i])
			max_tx_ix_sum = force_ix_data[i];
		if (min_tx_ix_sum > force_ix_data[i])
			min_tx_ix_sum = force_ix_data[i];

	}

	/* Read RX IX data */
	comp_start_rx_addr = (u16)(FTS_COMP_DATA_HEADER_SIZE + (tx_num * 2));
	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(comp_start_rx_addr >> 8);
	regAdd[2] = (u8)(comp_start_rx_addr & 0xFF);
	info->fts_read_reg(info, &regAdd[0], 3, &data[0], rx_num * 2);

	for (i = 0; i < rx_num; i++) {
		sense_ix_data[i] = data[2 * i] | data[2 * i + 1] << 8;

		if (max_rx_ix_sum < sense_ix_data[i])
			max_rx_ix_sum = sense_ix_data[i];
		if (min_rx_ix_sum > sense_ix_data[i])
			min_rx_ix_sum = sense_ix_data[i];
	}

	input_info(true, &info->client->dev, "%s: MIN_TX_IX_SUM : %d MAX_TX_IX_SUM : %d\n",
			__func__, min_tx_ix_sum, max_tx_ix_sum);
	input_info(true, &info->client->dev, "%s: MIN_RX_IX_SUM : %d MAX_RX_IX_SUM : %d\n",
			__func__, min_rx_ix_sum, max_rx_ix_sum);

	if (allnode == true) {
		buff_size = (info->ForceChannelLength + info->SenseChannelLength + 2) * 5;
		mbuff = kzalloc(buff_size, GFP_KERNEL);
	}
	if (mbuff != NULL) {
		char *pBuf = mbuff;

		for (i = 0; i < info->ForceChannelLength; i++) {
			num =  force_ix_data[i];
			n = 100000;
			fzero = 0;
			for (j = 5; j > 0; j--) {
				n = n / 10;
				a = num / n;
				if (a)
					fzero = 1;
				cnum = a + '0';
				num  = num - a*n;
				if (fzero)
					*pBuf++ = cnum;
			}
			if (!fzero)
				*pBuf++ = '0';
			*pBuf++ = ',';
			input_info(true, &info->client->dev, "Force[%d] %d\n", i, force_ix_data[i]);
		}
		for (i = 0; i < info->SenseChannelLength; i++) {
			num =  sense_ix_data[i];
			n = 100000;
			fzero = 0;
			for (j = 5; j > 0; j--) {
				n = n / 10;
				a = num / n;
				if (a)
					fzero = 1;
				cnum = a + '0';
				num  = num - a * n;
				if (fzero)
					*pBuf++ = cnum;
			}
			if (!fzero)
				*pBuf++ = '0';
			if (i < (info->SenseChannelLength - 1))
				*pBuf++ = ',';
			input_info(true, &info->client->dev, "Sense[%d] %d\n", i, sense_ix_data[i]);
		}

		sec_cmd_set_cmd_result(sec, mbuff, buff_size);
		sec->cmd_state = SEC_CMD_STATUS_OK;
		kfree(mbuff);
		return;
	} else {
		if (allnode == true) {
			snprintf(buff, sizeof(buff), "%s", "kzalloc failed");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "%d,%d,%d,%d",
					min_tx_ix_sum, max_tx_ix_sum, min_rx_ix_sum, max_rx_ix_sum);
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}
out:
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		char ret_buff[SEC_CMD_STR_LEN] = { 0 };
		snprintf(ret_buff, sizeof(ret_buff), "%d,%d", min_rx_ix_sum, max_rx_ix_sum);
		sec_cmd_set_cmd_result_all(sec, ret_buff, strnlen(ret_buff, sizeof(ret_buff)), "IX_DATA_X");
		snprintf(ret_buff, sizeof(ret_buff), "%d,%d", min_tx_ix_sum, max_tx_ix_sum);
		sec_cmd_set_cmd_result_all(sec, ret_buff, strnlen(ret_buff, sizeof(ret_buff)), "IX_DATA_Y");
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_ix_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	fts_read_ix_data(info, false);
}

static void run_ix_data_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	enter_factory_mode(info, true);
	fts_read_ix_data(info, true);
	enter_factory_mode(info, false);
}

static void fts_read_self_raw_frame(struct fts_ts_info *info, bool allnode)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct FTS_SyncFrameHeader *pSyncFrameHeader;

	u8 regAdd[FTS_EVENT_SIZE] = {0};

	u8 data[(info->ForceChannelLength + info->SenseChannelLength) * 2 + 1];

	s16 self_force_raw_data[info->ForceChannelLength * 2 + 1];
	s16 self_sense_raw_data[info->SenseChannelLength * 2 + 1];

	int Offset = 0;
	u8 count = 0;
	int i;
	int ret;
	int totalbytes;
	int retry = 10;

	s16 min_tx_self_raw_data = S16_MAX;
	s16 max_tx_self_raw_data = S16_MIN;
	s16 min_rx_self_raw_data = S16_MAX;
	s16 max_rx_self_raw_data = S16_MIN;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	// Request Data Type
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = TYPE_RAW_DATA;
	info->fts_write_reg(info, &regAdd[0], 3);

	do {
		regAdd[0] = 0xA6;
		regAdd[1] = 0x00;
		regAdd[2] = 0x00;
		ret = info->fts_read_reg(info, &regAdd[0], 3, &data[0], FTS_COMP_DATA_HEADER_SIZE);
		if (ret <= 0) {
			input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			goto out;
		}

		pSyncFrameHeader = (struct FTS_SyncFrameHeader *) &data[0];

		if ((pSyncFrameHeader->header == 0xA5) && (pSyncFrameHeader->host_data_mem_id == TYPE_RAW_DATA))
			break;

		fts_delay(100);
	} while (retry--);

	if (retry == 0) {
		input_err(true, &info->client->dev, "%s: didn't match header or id. header = %02X, id = %02X\n",
				__func__, pSyncFrameHeader->header, pSyncFrameHeader->host_data_mem_id);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	Offset = FTS_COMP_DATA_HEADER_SIZE + pSyncFrameHeader->dbg_frm_len +
			(pSyncFrameHeader->ms_force_len * pSyncFrameHeader->ms_sense_len * 2);

	totalbytes = (pSyncFrameHeader->ss_force_len + pSyncFrameHeader->ss_sense_len) * 2;

	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(Offset >> 8);
	regAdd[2] = (u8)(Offset & 0xFF);
	ret = info->fts_read_reg(info, &regAdd[0], 3, &data[0], totalbytes);
	if (ret <= 0) {
		input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	Offset = 0;
	for (count = 0; count < (info->ForceChannelLength); count++) {
		self_force_raw_data[count] = (s16)(data[count * 2 + Offset] + (data[count * 2 + 1 + Offset] << 8));

		if (max_tx_self_raw_data < self_force_raw_data[count])
			max_tx_self_raw_data = self_force_raw_data[count];
		if (min_tx_self_raw_data > self_force_raw_data[count])
			min_tx_self_raw_data = self_force_raw_data[count];
	}

	Offset = (info->ForceChannelLength * 2);
	for (count = 0; count < info->SenseChannelLength; count++) {
		self_sense_raw_data[count] = (s16)(data[count * 2 + Offset] + (data[count * 2 + 1 + Offset] << 8));

		if (max_rx_self_raw_data < self_sense_raw_data[count])
			max_rx_self_raw_data = self_sense_raw_data[count];
		if (min_rx_self_raw_data > self_sense_raw_data[count])
			min_rx_self_raw_data = self_sense_raw_data[count];
	}

	input_info(true, &info->client->dev, "%s: MIN_TX_SELF_RAW: %d MAX_TX_SELF_RAW : %d\n",
			__func__, (s16)min_tx_self_raw_data, (s16)max_tx_self_raw_data);
	input_info(true, &info->client->dev, "%s: MIN_RX_SELF_RAW : %d MIN_RX_SELF_RAW : %d\n",
			__func__, (s16)min_rx_self_raw_data, (s16)max_rx_self_raw_data);

	if (allnode == true) {
		char mbuff[(info->ForceChannelLength + info->SenseChannelLength + 2) * 10];
		char temp[10] = { 0 };

		memset(mbuff, 0x0, sizeof(mbuff));
		for (i = 0; i < (info->ForceChannelLength); i++) {
			snprintf(temp, sizeof(temp), "%d,", (s16)self_force_raw_data[i]);
			strncat(mbuff, temp, sizeof(temp));
			input_info(true, &info->client->dev, "Force[%d] %d\n", i, (s16)self_force_raw_data[i]);
		}
		for (i = 0; i < (info->SenseChannelLength); i++) {
			snprintf(temp, sizeof(temp), "%d,", (s16)self_sense_raw_data[i]);
			strncat(mbuff, temp, sizeof(temp));
			input_info(true, &info->client->dev, "Sense[%d] %d\n", i, (s16)self_sense_raw_data[i]);
		}

		sec_cmd_set_cmd_result(sec, mbuff, sizeof(mbuff));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		return;
	} else {
		snprintf(buff, sizeof(buff), "%d,%d,%d,%d",
				(s16)min_tx_self_raw_data, (s16)max_tx_self_raw_data,
				(s16)min_rx_self_raw_data, (s16)max_rx_self_raw_data);
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
out:
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		char ret_buff[SEC_CMD_STR_LEN] = { 0 };
		snprintf(ret_buff, sizeof(ret_buff), "%d,%d", (s16)min_rx_self_raw_data, (s16)max_rx_self_raw_data);
		sec_cmd_set_cmd_result_all(sec, ret_buff, strnlen(ret_buff, sizeof(ret_buff)), "SELF_RAW_DATA_X");
		snprintf(ret_buff, sizeof(ret_buff), "%d,%d", (s16)min_tx_self_raw_data, (s16)max_tx_self_raw_data);
		sec_cmd_set_cmd_result_all(sec, ret_buff, strnlen(ret_buff, sizeof(ret_buff)), "SELF_RAW_DATA_Y");
	}
	sec_cmd_set_cmd_result(sec, &buff[0], strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_self_raw_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	fts_read_self_raw_frame(info, false);
}

static void run_self_raw_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	enter_factory_mode(info, true);
	fts_read_self_raw_frame(info, true);
	enter_factory_mode(info, false);
}

static void run_trx_short_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_panel_ito_test(info, OPEN_SHORT_CRACK_TEST);
	if (ret == 0) {
		snprintf(buff, sizeof(buff), "OK");
		send_event_to_user(info, sec->cmd_param[0], UEVENT_OPEN_SHORT_PASS);
	} else {
		snprintf(buff, sizeof(buff), "NG");
		send_event_to_user(info, sec->cmd_param[0], UEVENT_OPEN_SHORT_FAIL);
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void check_connection(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_panel_ito_test(info, OPEN_TEST);
	if (ret == 0)
		snprintf(buff, sizeof(buff), "OK");
	else
		snprintf(buff, sizeof(buff), "NG");

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_cx_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	node = fts_check_index(info);
	if (node < 0)
		return;

	if (info->cx_data)
		val = info->cx_data[node];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

}


static int read_ms_cx_data(struct fts_ts_info *info, u8 *cx_min, u8 *cx_max)
{
	u8 rdata[info->ForceChannelLength * info->SenseChannelLength];
	u8 regAdd[FTS_EVENT_SIZE] = { 0 };
	u8 dataID;
	u16 comp_start_addr;
	int txnum, rxnum, i, j, ret = 0;
	u8 *pStr = NULL;
	u8 pTmp[16] = { 0 };

	pStr = kzalloc(7 * (info->SenseChannelLength + 1), GFP_KERNEL);
	if (pStr == NULL)
		return -ENOMEM;

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true); // Clear FIFO
	fts_release_all_finger(info);

	fts_interrupt_set(info, INT_DISABLE);

	fts_delay(20);

	// Request compensation data type
	dataID = 0x11;  // MS - LP
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = dataID;
	info->fts_write_reg(info, &regAdd[0], 3);
	fts_fw_wait_for_echo_event(info, &regAdd[0], 3);

	// Read Header
	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x00;
	info->fts_read_reg(info, &regAdd[0], 3, &rdata[0], FTS_COMP_DATA_HEADER_SIZE);
	fts_interrupt_set(info, INT_ENABLE);

	if ((rdata[0] != 0xA5) && (rdata[1] != dataID)) {
		input_info(true, &info->client->dev, "%s: failed to read signature data of header.\n", __func__);
		ret = -EIO;
		goto out;
	}

	txnum = rdata[4];
	rxnum = rdata[5];

	comp_start_addr = (u16)FTS_COMP_DATA_HEADER_SIZE;
	regAdd[0] = 0xA6;
	regAdd[1] = (u8)(comp_start_addr >> 8);
	regAdd[2] = (u8)(comp_start_addr & 0xFF);
	info->fts_read_reg(info, &regAdd[0], 3, &rdata[0], txnum * rxnum);

	*cx_min = *cx_max = rdata[0];
	for (j = 0; j < info->ForceChannelLength; j++) {
		memset(pStr, 0x0, 7 * (info->SenseChannelLength + 1));
		snprintf(pTmp, sizeof(pTmp), "Tx%02d | ", j);
		strncat(pStr, pTmp, 7 * info->SenseChannelLength);

		for (i = 0; i < info->SenseChannelLength; i++) {
			snprintf(pTmp, sizeof(pTmp), "%3d", rdata[j * info->SenseChannelLength + i]);
			strncat(pStr, pTmp, 7 * info->SenseChannelLength);
			*cx_min = min(*cx_min, rdata[j * info->SenseChannelLength + i]);
			*cx_max = max(*cx_max, rdata[j * info->SenseChannelLength + i]);
		}
		input_raw_info(true, &info->client->dev, "%s\n", pStr);
	}
	input_raw_info(true, &info->client->dev, "cx min:%d, cx max:%d\n", *cx_min, *cx_max);

	if (info->cx_data)
		memcpy(&info->cx_data[0], &rdata[0], info->ForceChannelLength * info->SenseChannelLength);

out:
	kfree(pStr);
	return ret;
}

static void run_cx_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char buff_minmax[SEC_CMD_STR_LEN] = { 0 };
	int rc;
	u8 cx_min, cx_max;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
			sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "CX_DATA");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	input_raw_info(true, &info->client->dev, "%s: start\n", __func__);

	rc = read_ms_cx_data(info, &cx_min, &cx_max);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "NG");
		snprintf(buff_minmax, sizeof(buff_minmax), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff_minmax, sizeof(buff_minmax), "%d,%d", cx_min, cx_max);
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff_minmax, strnlen(buff_minmax, sizeof(buff_minmax)), "CX_DATA");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_cx_all_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rc, i, j;
	u8 cx_min, cx_max;
	char *all_strbuff;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}


	input_info(true, &info->client->dev, "%s: start\n", __func__);

	enter_factory_mode(info, true);
	rc = read_ms_cx_data(info, &cx_min, &cx_max);

	/* do not systemreset in COB type */
	if (info->board->chip_on_board)
		fts_set_scanmode(info, info->scan_mode);
	else
		enter_factory_mode(info, false);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	all_strbuff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 4 + 1, GFP_KERNEL);
	if (!all_strbuff) {
		input_err(true, &info->client->dev, "%s: alloc failed\n", __func__);
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	/* Read compensation data */
	if (info->cx_data) {
		for (j = 0; j < info->ForceChannelLength; j++) {
			for (i = 0; i < info->SenseChannelLength; i++) {
				snprintf(buff, sizeof(buff), "%d,", info->cx_data[j * info->SenseChannelLength + i]);
				strncat(all_strbuff, buff, sizeof(buff));
			}
		}
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, all_strbuff, strlen(all_strbuff));
	input_info(true, &info->client->dev, "%s: %ld\n", __func__, strlen(all_strbuff));
	kfree(all_strbuff);
}

static void get_cx_gap_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rx_max = 0, tx_max = 0, ii;

	for (ii = 0; ii < (info->SenseChannelLength * info->ForceChannelLength); ii++) {
		/* rx(x) gap max */
		if ((ii + 1) % (info->SenseChannelLength) != 0)
			rx_max = max(rx_max, (int)abs(info->cx_data[ii + 1] - info->cx_data[ii]));

		/* tx(y) gap max */
		if (ii < (info->ForceChannelLength - 1) * info->SenseChannelLength)
			tx_max = max(tx_max, (int)abs(info->cx_data[ii + info->SenseChannelLength] - info->cx_data[ii]));
	}

	input_raw_info(true, &info->client->dev, "%s: rx max:%d, tx max:%d\n", __func__, rx_max, tx_max);

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		snprintf(buff, sizeof(buff), "%d,%d", 0, rx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "CX_DATA_GAP_X");
		snprintf(buff, sizeof(buff), "%d,%d", 0, tx_max);
		sec_cmd_set_cmd_result_all(sec, buff, SEC_CMD_STR_LEN, "CX_DATA_GAP_Y");
	}
}

static void run_cx_gap_data_x_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char *buff = NULL;
	int ii;
	char temp[5] = { 0 };

	sec_cmd_set_default_result(sec);

	buff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 5, GFP_KERNEL);
	if (!buff) {
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (ii = 0; ii < (info->SenseChannelLength * info->ForceChannelLength); ii++) {
		if ((ii + 1) % (info->SenseChannelLength) != 0) {
			snprintf(temp, sizeof(temp), "%d,", (int)abs(info->cx_data[ii + 1] - info->cx_data[ii]));
			strncat(buff, temp, sizeof(temp));
			memset(temp, 0x00, 5);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, info->ForceChannelLength * info->SenseChannelLength * 5));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

static void run_cx_gap_data_y_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char *buff = NULL;
	int ii;
	char temp[5] = { 0 };

	sec_cmd_set_default_result(sec);

	buff = kzalloc(info->ForceChannelLength * info->SenseChannelLength * 5, GFP_KERNEL);
	if (!buff) {
		snprintf(temp, sizeof(temp), "NG");
		sec_cmd_set_cmd_result(sec, temp, strnlen(temp, sizeof(temp)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (ii = 0; ii < (info->SenseChannelLength * info->ForceChannelLength); ii++) {
		if (ii < (info->ForceChannelLength - 1) * info->SenseChannelLength) {
			snprintf(temp, sizeof(temp), "%d,",
					(int)abs(info->cx_data[ii + info->SenseChannelLength] - info->cx_data[ii]));
			strncat(buff, temp, sizeof(temp));
			memset(temp, 0x00, 5);
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, info->ForceChannelLength * info->SenseChannelLength * 5));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	kfree(buff);
}

#ifdef FTS_SUPPORT_PRESSURE_SENSOR
int fts_read_pressure_data(struct fts_ts_info *info)
{
	u8 pressure_reg[3] = {0, };
	u8 buf[32] = {0, };
	short value[10] = {0, };
	int rtn, i = 0;
	int pressure_sensor_value = 0;

	pressure_reg[0] = 0xD0;
	pressure_reg[1] = 0x00;
	pressure_reg[2] = FTS_SI_PRESSURE_STRENGTH_ADDR;

	rtn = fts_read_reg(info, pressure_reg, 3, &buf[0], 4);
	if (rtn <= 0) {
		input_err(true, &info->client->dev, "%s: Pressure read failed rc = %d\n", __func__, rtn);
		return rtn;
	}

	pressure_reg[1] = buf[2];
	pressure_reg[2] = buf[1];

	rtn = fts_read_reg(info, pressure_reg, 3, &buf[0], PRESSURE_SENSOR_COUNT * 2 + 1);
	if (rtn <= 0) {
		input_err(true, &info->client->dev, "%s: Pressure read failed rc = %d\n", __func__, rtn);
		return rtn;
	}

	for (i = 0; i < PRESSURE_SENSOR_COUNT; i++) {
		value[i] = buf[i * 2 + 1] + (buf[i * 2 + 1 + 1] << 8);
		pressure_sensor_value += value[i];
	}

	if (info->debug_string & 0x2)
		input_info(true, &info->client->dev, "%s: %d %d %d %d %d %d %d %d %d %d %d\n", __func__,
				value[0], value[1], value[2], value[3], value[4],
				value[5], value[6], value[7], value[8], value[9],
				pressure_sensor_value);

	return pressure_sensor_value;
}
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
#define USE_KEY_NUM 2
static void run_key_cx_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 key_cx2_data[2], key_cx1_data, total_cx_data[USE_KEY_NUM];
	u8 ReadData[USE_KEY_NUM * FTS_CX2_READ_LENGTH];
	u8 regAdd[8];
	unsigned int addr;
	int /*tx_num, */rx_num, DOFFSET = 1;
	int comp_start_addr, comp_header_addr;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	/* Request compensation data */
	regAdd[0] = 0xB8;
	regAdd[1] = 0x10;   // For button
	regAdd[2] = 0x00;
	fts_write_reg(info, &regAdd[0], 3);
	fts_fw_wait_for_specific_event(info, EVENTID_STATUS_REQUEST_COMP, regAdd[1], 0x00);

	/* Read an address of compensation data */
	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = FTS_SI_COMPENSATION_OFFSET_ADDR;
	fts_read_reg(info, regAdd, 3, &buff[0], 4);
	comp_header_addr = buff[0 + DOFFSET] + (buff[1 + DOFFSET] << 8);

	/* Read header of compensation area */
	regAdd[0] = 0xD0;
	regAdd[1] = (comp_header_addr >> 8) & 0xFF;
	regAdd[2] = comp_header_addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &buff[0], 16 + DOFFSET);
	/*tx_num = buff[4 + DOFFSET];*/
	rx_num = buff[5 + DOFFSET];
	key_cx1_data = buff[9 + DOFFSET];
	comp_start_addr = comp_header_addr + 0x10;

	memset(&ReadData[0], 0x0, rx_num);
	/* Read compensation data */
	addr = comp_start_addr;
	regAdd[0] = 0xD0;
	regAdd[1] = (addr >> 8) & 0xFF;
	regAdd[2] = addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &ReadData[0], rx_num + DOFFSET);
	key_cx2_data[0] = ReadData[0 + DOFFSET];
	key_cx2_data[1] = ReadData[1 + DOFFSET];
	total_cx_data[0] = key_cx1_data * 2 + key_cx2_data[0];
	total_cx_data[1] = key_cx1_data * 2 + key_cx2_data[1];

	//snprintf(buff, sizeof(buff), "%s", "OK");
	snprintf(buff, sizeof(buff), "%d,%d,%d,%d",
			key_cx2_data[0], key_cx2_data[1], total_cx_data[0], total_cx_data[1]);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

#ifdef FTS_SUPPORT_PRESSURE_SENSOR
static void run_force_pressure_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_info(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (info->touch_count > 0) {
		snprintf(buff, sizeof(buff), "%s", "NG_FINGER_ON");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out_force_pressure_cal;
	}

	info->fts_systemreset(info);
	fts_delay(20);
	info->fts_wait_for_ready(info);

	fts_command(info, PRESSURE_AUTO_TUNE);
	fts_delay(300);
	fts_fw_wait_for_event_D3(info, STATUS_EVENT_SELF_AUTOTUNE_DONE_D3, 0x00);

	fts_command(info, FTS_CMD_SAVE_CX_TUNING);
	fts_delay(230);
	fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);

	info->fts_systemreset(info);
	fts_delay(20);
	info->fts_wait_for_ready(info);

	fts_command(info, SENSEON);
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
	fts_fw_wait_for_event(info, STATUS_EVENT_FORCE_CAL_DONE_D3);

	fts_get_pressure_calibration_information(info);
	if (info->pressure_cal_base == 0xFF)
		info->pressure_cal_base = 0;

	info->pressure_cal_base++;

	if (info->pressure_cal_base > 0xFE)
		info->pressure_cal_base = 0xFE;

	fts_set_pressure_calibration_information(info, info->pressure_cal_base, info->pressure_cal_delta);

#ifdef FTS_SUPPORT_SPONGELIB
	info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
			&info->lowpower_flag, sizeof(info->lowpower_flag));
#endif

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

out_force_pressure_cal:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

/*
 * index is 0 : cleared, do not calibrated
 * index is 1 : Ass'y
 * index is 2 : Rear
 * index is 3 : BackGlass
 */

static void set_pressure_test_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[4] = {0xC7, 0x03, 0x00, 0x00};
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_info(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		regAdd[3] = sec->cmd_param[0];
		ret = fts_write_reg(info, regAdd, 4);

		if (ret < 0) {
			snprintf(buff, sizeof(buff), "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "%s", "OK");
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_pressure_strength_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAddr[3] = {0xD0, 0x00, FTS_SI_PRESSURE_STRENGTH_ADDR};
	u8 data[PRESSURE_SENSOR_COUNT * 2 + 1];
	int ret, i;
	short value[3] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	memset(data, 3, PRESSURE_SENSOR_COUNT * 2 + 1);

	ret = fts_read_reg(info, regAddr, 3, data, 4);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read pressure strength addr\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	regAddr[1] = data[2];
	regAddr[2] = data[1];

	memset(data, 0x00, PRESSURE_SENSOR_COUNT * 2 + 1);

	ret = fts_read_reg(info, regAddr, 3, data, PRESSURE_SENSOR_COUNT * 2 + 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read pressure strength\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_err(true, &info->client->dev, "%s: [3] %02X, %02X, %02X, %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3], data[4], data[5], data[6]);

	for (i = 0; i < PRESSURE_SENSOR_COUNT; i++)
		value[i] = (short)(data[i * 2 + 1] | (data[i * 2 + 1 + 1] << 8));

	snprintf(buff, sizeof(buff), "%d,%d,%d", value[0], value[1], value[2]);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_pressure_rawdata_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAddr[3] = {0xD0, 0x00, 0x00};
	u8 data[PRESSURE_SENSOR_COUNT * 2 + 1];
	int ret, i;
	short value[3] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	memset(data, 3, PRESSURE_SENSOR_COUNT * 2 + 1);

	ret = info->fts_get_sysinfo_data(info, FTS_SI_PRESSURE_FILTERED_RAW_ADDR, 3, data);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read pressure rawdata addr\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	regAddr[1] = data[1];
	regAddr[2] = data[0];

	memset(data, 0x00, PRESSURE_SENSOR_COUNT * 2 + 1);

	ret = fts_read_reg(info, regAddr, 3, data, PRESSURE_SENSOR_COUNT * 2 + 1);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read pressure rawdata\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_err(true, &info->client->dev, "%s: [3] %02X, %02X, %02X, %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3], data[4], data[5], data[6]);

	for (i = 0; i < PRESSURE_SENSOR_COUNT; i++)
		value[i] = (short)(data[i * 2 + 1] | (data[i * 2 + 1 + 1] << 8));

	snprintf(buff, sizeof(buff), "%d,%d,%d", value[0], value[1], value[2]);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}


#define FTS_PRESSURE_IX1	10
#define FTS_PRESSIRE_IX2	17
static void run_pressure_ix_data_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {0xB8, 0x00, 0x02};
	u8 data[32];
	int ret, i;
	int value[3] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	fts_command(info, SENSEOFF);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_command(info, FTS_CMD_KEY_SENSE_OFF);
#endif
	fts_command(info, FLUSHBUFFER);
	fts_delay(50);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	ret = fts_write_reg(info, regAdd, 3);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to write pressure ix\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_fw_wait_for_specific_event(info, EVENTID_STATUS_REQUEST_COMP, regAdd[1], regAdd[2]);

	memset(data, 0x00, 32);

	ret = info->fts_get_sysinfo_data(info, FTS_SI_COMPENSATION_OFFSET_ADDR, 7, data);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read ix offset addr\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, &info->client->dev, "%s: [0] 0x%02X, [0x%02X, 0x%02X], 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
			__func__, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

	regAdd[0] = 0xD0;
	regAdd[1] = data[1];
	regAdd[2] = data[0];

	memset(data, 0x00, 32);

	ret = fts_read_reg(info, &regAdd[0], 3, data, 32);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read ix\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	for (i = 0; i < 4; i++)
		input_info(true, &info->client->dev, "%s: [%d] 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
				__func__, i, data[i * 8 + 0], data[i * 8 + 1], data[i * 8 + 2], data[i * 8 + 3],
				data[i * 8 + 4], data[i * 8 + 5], data[i * 8 + 6], data[i * 8 + 7]);

	for (i = 0; i < PRESSURE_SENSOR_COUNT; i++)
		value[i] = ((data[FTS_PRESSURE_IX1] * 2) + data[FTS_PRESSIRE_IX2 + i]);

	snprintf(buff, sizeof(buff), "%d,%d,%d", value[0], value[1], value[2]);

	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_pressure_strength(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 strength[12] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	input_info(true, &info->client->dev, "%s: [index:%d], [1: %d], [2:%d], [3:%d]\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1], sec->cmd_param[2], sec->cmd_param[3]);

	if ((sec->cmd_param[0] < 1) || (sec->cmd_param[0] > 4)) {
		input_err(true, &info->client->dev, "%s: wrong index.\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "WRONG INDEX");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	index = sec->cmd_param[0];

	strength[0] = 0xC7;
	strength[1] = 0x03;
	strength[2] = 0x02;
	strength[3] = (char)index;
	strength[4] = (char)(sec->cmd_param[1] & 0xFF);
	strength[5] = (char)(sec->cmd_param[1] >> 8);
	strength[6] = (char)(sec->cmd_param[2] & 0xFF);
	strength[7] = (char)(sec->cmd_param[2] >> 8);
	strength[8] = (char)(sec->cmd_param[3] & 0xFF);
	strength[9] = (char)(sec->cmd_param[3] >> 8);
	strength[10] = 0x00;
	strength[11] = 0x00;

	fts_command(info, SENSEOFF);
	fts_delay(50);
	fts_command(info, FLUSHBUFFER);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	ret = fts_write_reg(info, strength, 12);
	if (ret <= 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		fts_command(info, FTS_CMD_SAVE_CX_TUNING);
		fts_delay(230);
		fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);

		fts_command(info, FLUSHBUFFER);
		fts_delay(10);
		fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
		fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
		return;
	}

	fts_command(info, FTS_CMD_SAVE_CX_TUNING);
	fts_delay(230);
	fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);

	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif

	fts_get_pressure_calibration_information(info);
	if (info->pressure_cal_delta == 0xFF)
		info->pressure_cal_delta = 0;

	info->pressure_cal_delta++;

	if (info->pressure_cal_delta > 0xFE)
		info->pressure_cal_delta = 0xFE;

	fts_set_pressure_calibration_information(info, info->pressure_cal_base, info->pressure_cal_delta);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_pressure_rawdata(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 rawdata[12] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	input_info(true, &info->client->dev, "%s: [index:%d], [1: %d], [2:%d], [3:%d]\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1], sec->cmd_param[2], sec->cmd_param[3]);

	if ((sec->cmd_param[0] < 1) || (sec->cmd_param[0] > 4)) {
		input_err(true, &info->client->dev, "%s: wrong index.\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "WRONG INDEX");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	index = sec->cmd_param[0];

	rawdata[0] = 0xC7;
	rawdata[1] = 0x03;
	rawdata[2] = 0x03;
	rawdata[3] = (char)index;
	rawdata[4] = (char)(sec->cmd_param[1] & 0xFF);
	rawdata[5] = (char)(sec->cmd_param[1] >> 8);
	rawdata[6] = (char)(sec->cmd_param[2] & 0xFF);
	rawdata[7] = (char)(sec->cmd_param[2] >> 8);
	rawdata[8] = (char)(sec->cmd_param[3] & 0xFF);
	rawdata[9] = (char)(sec->cmd_param[3] >> 8);
	rawdata[10] = 0x00;
	rawdata[11] = 0x00;

	fts_command(info, SENSEOFF);
	fts_delay(50);
	fts_command(info, FLUSHBUFFER);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	ret = fts_write_reg(info, rawdata, 12);
	if (ret <= 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		fts_command(info, FTS_CMD_SAVE_CX_TUNING);
		fts_delay(230);
		fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);

		fts_command(info, FLUSHBUFFER);
		fts_delay(10);
		fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
		fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
		return;
	}

	fts_command(info, FTS_CMD_SAVE_CX_TUNING);
	fts_delay(230);
	fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);

	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_pressure_data_index(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[12] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	input_info(true, &info->client->dev, "%s: %d\n",
			__func__, sec->cmd_param[0]);

	if ((sec->cmd_param[0] < 0) || (sec->cmd_param[0] > 4)) {
		input_err(true, &info->client->dev, "%s: wrong index.\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "WRONG INDEX");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (sec->cmd_param[0] == 0)
		input_info(true, &info->client->dev, "%s: disable calibrated strength\n", __func__);

	index = sec->cmd_param[0];

	data[0] = 0xC7;
	data[1] = 0x03;
	data[2] = 0x01;
	data[3] = (char)index;

	fts_command(info, SENSEOFF);
	fts_delay(50);
	fts_command(info, FLUSHBUFFER);


	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	ret = fts_write_reg(info, data, 4);
	if (ret <= 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

		if (index != 0x00) {
			fts_command(info, FTS_CMD_SAVE_CX_TUNING);
			fts_delay(230);
			fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);
		}

		fts_command(info, FLUSHBUFFER);
		fts_delay(10);
		fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
		fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
		return;
	}

	if (index != 0x00) {
		fts_command(info, FTS_CMD_SAVE_CX_TUNING);
		fts_delay(230);
		fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);
	}

	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_pressure_strength(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 *data = NULL;
	short strength[3] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	input_info(true, &info->client->dev, "%s: index: %d\n",
			__func__, sec->cmd_param[0]);

	if ((sec->cmd_param[0] < 1) || (sec->cmd_param[0] > 4)) {
		input_err(true, &info->client->dev, "%s: wrong index.\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "WRONG INDEX");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	index = sec->cmd_param[0] - 1;

	data = kzalloc(nvm_data[PRESSURE_STRENGTH].length, GFP_KERNEL);
	if (!data) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);
		return;
	}

	ret = get_nvm_data(info, PRESSURE_STRENGTH, data);
	if (ret <= 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		kfree(data);
		return;
	}

	if ((data[index * 8 + 6] == 0xFF) && (data[index * 8 + 7] == 0xFF)) {
		input_info(true, &info->client->dev, "%s: flash is initailized, clear\n", __func__);
		memset(&data[index * 8], 0x00, 8);
	}

	strength[0] = (short)(data[index * 8 + 0] | ((data[index * 8 + 1] << 8) & 0xFF00));
	strength[1] = (short)(data[index * 8 + 2] | ((data[index * 8 + 3] << 8) & 0xFF00));
	strength[2] = (short)(data[index * 8 + 4] | ((data[index * 8 + 5] << 8) & 0xFF00));

	snprintf(buff, sizeof(buff), "%d,%d,%d", strength[0], strength[1], strength[2]);

	kfree(data);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_pressure_rawdata(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 *data = NULL;
	short rawdata[3] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	input_info(true, &info->client->dev, "%s: index: %d\n",
			__func__, sec->cmd_param[0]);

	if ((sec->cmd_param[0] < 1) || (sec->cmd_param[0] > 4)) {
		input_err(true, &info->client->dev, "%s: wrong index.\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "WRONG INDEX");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	index = sec->cmd_param[0] - 1;

	data = kzalloc(nvm_data[PRESSURE_RAWDATA].length, GFP_KERNEL);
	if (!data) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);
		return;
	}

	ret = get_nvm_data(info, PRESSURE_RAWDATA, data);
	if (ret <= 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		kfree(data);
		return;
	}

	if ((data[index * 8 + 6] == 0xFF) && (data[index * 8 + 7] == 0xFF)) {
		input_info(true, &info->client->dev, "%s: flash is initailized, clear\n", __func__);
		memset(&data[index * 8], 0x00, 8);
	}

	rawdata[0] = (short)(data[index * 8 + 0] | ((data[index * 8 + 1] << 8) & 0xFF00));
	rawdata[1] = (short)(data[index * 8 + 2] | ((data[index * 8 + 3] << 8) & 0xFF00));
	rawdata[2] = (short)(data[index * 8 + 4] | ((data[index * 8 + 5] << 8) & 0xFF00));

	snprintf(buff, sizeof(buff), "%d,%d,%d", rawdata[0], rawdata[1], rawdata[2]);

	kfree(data);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_pressure_data_index(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 *data = NULL;
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	input_info(true, &info->client->dev, "%s: index: %d\n",
			__func__, sec->cmd_param[0]);

	data = kzalloc(nvm_data[GROUP_INDEX].length, GFP_KERNEL);
	if (!data) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);
		return;
	}

	ret = get_nvm_data(info, GROUP_INDEX, data);
	if (ret <= 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		kfree(data);
		return;
	}

	if (data[0] == 0xFF) {
		input_info(true, &info->client->dev, "%s: flash is initailized, clear\n", __func__);
		data[0] = 0;
	}

	snprintf(buff, sizeof(buff), "%d", data[0]);

	kfree(data);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_pressure_strength_clear(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[12] = { 0 };
	int ret;
	int ii = 0;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	fts_command(info, SENSEOFF);
	fts_delay(50);
	fts_command(info, FLUSHBUFFER);


	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif
	for (ii = 1; ii < 4; ii++) {
		data[0] = 0xC7;
		data[1] = 0x03;
		data[2] = 0x02;
		data[3] = ii;
		ret = fts_write_reg(info, data, 12);
		if (ret <= 0)
			goto err_write_reg;
	}

	input_info(true, &info->client->dev, "%s: clear strength\n", __func__);

	for (ii = 1; ii < 4; ii++) {
		data[0] = 0xC7;
		data[1] = 0x03;
		data[2] = 0x03;
		data[3] = ii;

		ret = fts_write_reg(info, data, 12);
		if (ret <= 0)
			goto err_write_reg;
	}

	input_info(true, &info->client->dev, "%s: clear rawdata\n", __func__);

	data[0] = 0xC7;
	data[1] = 0x03;
	data[2] = 0x01;
	data[3] = 0;

	ret = fts_write_reg(info, data, 4);
	if (ret <= 0)
		goto err_write_reg;

	input_info(true, &info->client->dev, "%s: clear group index\n", __func__);

	fts_command(info, FTS_CMD_SAVE_CX_TUNING);
	fts_delay(230);
	fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);

	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

	return;

err_write_reg:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	fts_command(info, FTS_CMD_SAVE_CX_TUNING);
	fts_delay(230);
	fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);

	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
}

static void get_pressure_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[3] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = info->fts_get_sysinfo_data(info, FTS_SI_PRESSURE_THRESHOLD, 3, data);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed to read pressure threshold\n", __func__);

	snprintf(buff, sizeof(buff), "%d", ((data[2] << 8) + data[1]));

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

/* low level is more sensitivity, except level-0(value 0) */
static void set_pressure_user_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[1] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if ((sec->cmd_param[0] < 1) || (sec->cmd_param[0] > 5)) {
		input_err(true, &info->client->dev, "%s: wrong index.\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "WRONG INDEX");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	data[0] = sec->cmd_param[0];

	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_PRESSURE_LEVEL, data, 1);
	if (ret <= 0)
		goto out_set_user_level;

	ret = info->fts_read_from_sponge(info, FTS_CMD_SPONGE_OFFSET_PRESSURE_LEVEL, data, 1);
	if (ret <= 0)
		goto out_set_user_level;

	input_info(true, &info->client->dev, "%s: set user level: %d\n", __func__, data[0]);

	info->pressure_user_level = data[0];

	fts_delay(20);

	data[0] = 0;
	ret = info->fts_read_from_sponge(info, FTS_CMD_SPONGE_OFFSET_PRESSURE_THD_HIGH, data, 1);
	if (ret <= 0)
		goto out_set_user_level;

	input_info(true, &info->client->dev, "%s: HIGH THD: %d\n", __func__, data[0]);

	data[0] = 0;
	ret = info->fts_read_from_sponge(info, FTS_CMD_SPONGE_OFFSET_PRESSURE_THD_LOW, data, 1);
	if (ret <= 0)
		goto out_set_user_level;

	input_info(true, &info->client->dev, "%s: HIGH LOW: %d\n", __func__, data[0]);

	snprintf(buff, sizeof(buff), "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;

out_set_user_level:
	snprintf(buff, sizeof(buff), "FAIL");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_pressure_user_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[1] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	data[0] = sec->cmd_param[0];

	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_PRESSURE_LEVEL, data, 1);
	if (ret <= 0)
		goto out_get_user_level;

	ret = info->fts_read_from_sponge(info, FTS_CMD_SPONGE_OFFSET_PRESSURE_LEVEL, data, 1);
	if (ret <= 0)
		goto out_get_user_level;

	input_info(true, &info->client->dev, "%s: set user level: %d\n", __func__, data[0]);

	info->pressure_user_level = data[0];

	snprintf(buff, sizeof(buff), "%d", info->pressure_user_level);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;

out_get_user_level:
	snprintf(buff, sizeof(buff), "NG");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

static void factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		sec->cmd_all_factory_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	snprintf(buff, sizeof(buff), "%d", info->board->item_version);
	sec_cmd_set_cmd_result_all(sec, buff, sizeof(buff), "ITEM_VERSION");

	get_chip_vendor(sec);
	get_chip_name(sec);
	get_fw_ver_bin(sec);
	get_fw_ver_ic(sec);

	if (info->board->item_version > 1)
		fts_panel_ito_test(info, OPEN_SHORT_CRACK_TEST);

	enter_factory_mode(info, true);

	run_rawcap_read(sec);
	run_self_raw_read(sec);

	fts_set_scanmode(info, FTS_SCAN_MODE_SCAN_OFF);
	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	run_cx_data_read(sec);
	get_cx_gap_data(sec);
	run_ix_data_read(sec);

	if (info->board->item_version == 1)
		get_wet_mode(sec);
	get_mis_cal_info(sec);

	/* do not systemreset in COB type */
	if (info->board->chip_on_board)
		fts_set_scanmode(info, info->scan_mode);
	else
		enter_factory_mode(info, false);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

out:
	input_info(true, &info->client->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static void set_factory_level(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: Touch is stopped!\n", __func__);
		goto NG;
	}

	if (sec->cmd_param[0] < OFFSET_FAC_SUB || sec->cmd_param[0] > OFFSET_FAC_MAIN) {
		input_err(true, &info->client->dev,
				"%s: cmd data is abnormal, %d\n", __func__, sec->cmd_param[0]);
		goto NG;
	}

	info->factory_position = sec->cmd_param[0];

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->factory_position);
	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	return;

NG:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
}

int fts_get_tsp_test_result(struct fts_ts_info *info)
{
	u8 data;
	int ret;

	ret = get_nvm_data(info, FTS_NVM_OFFSET_FAC_RESULT, &data);
	if (ret < 0)
		goto err_read;

	if (data == 0xFF)
		data = 0;

	info->test_result.data[0] = data;

err_read:
	return ret;
}
EXPORT_SYMBOL(fts_get_tsp_test_result);

static void get_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_get_tsp_test_result(info);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: get failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "M:%s, M:%d, A:%s, A:%d",
				info->test_result.module_result == 0 ? "NONE" :
				info->test_result.module_result == 1 ? "FAIL" :
				info->test_result.module_result == 2 ? "PASS" : "A",
				info->test_result.module_count,
				info->test_result.assy_result == 0 ? "NONE" :
				info->test_result.assy_result == 1 ? "FAIL" :
				info->test_result.assy_result == 2 ? "PASS" : "A",
				info->test_result.assy_count);

		sec_cmd_set_cmd_result(sec, buff, strlen(buff));
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
}

/* FACTORY TEST RESULT SAVING FUNCTION
 * bit 3 ~ 0 : OCTA Assy
 * bit 7 ~ 4 : OCTA module
 * param[0] : OCTA module(1) / OCTA Assy(2)
 * param[1] : TEST NONE(0) / TEST FAIL(1) / TEST PASS(2) : 2 bit
 */
static void set_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_get_tsp_test_result(info);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: get failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}
	sec->cmd_state = SEC_CMD_STATUS_RUNNING;

	if (sec->cmd_param[0] == TEST_OCTA_ASSAY) {
		info->test_result.assy_result = sec->cmd_param[1];
		if (info->test_result.assy_count < 3)
			info->test_result.assy_count++;

	} else if (sec->cmd_param[0] == TEST_OCTA_MODULE) {
		info->test_result.module_result = sec->cmd_param[1];
		if (info->test_result.module_count < 3)
			info->test_result.module_count++;
	}

	input_info(true, &info->client->dev, "%s: [0x%X] M:%s, M:%d, A:%s, A:%d\n",
			__func__, info->test_result.data[0],
			info->test_result.module_result == 0 ? "NONE" :
			info->test_result.module_result == 1 ? "FAIL" :
			info->test_result.module_result == 2 ? "PASS" : "A",
			info->test_result.module_count,
			info->test_result.assy_result == 0 ? "NONE" :
			info->test_result.assy_result == 1 ? "FAIL" :
			info->test_result.assy_result == 2 ? "PASS" : "A",
			info->test_result.assy_count);

	ret = set_nvm_data(info, FTS_NVM_OFFSET_FAC_RESULT, info->test_result.data);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: set failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

int fts_get_disassemble_count(struct fts_ts_info *info)
{
	u8 data;
	int ret;

	ret = get_nvm_data(info, FTS_NVM_OFFSET_DISASSEMBLE_COUNT, &data);
	if (ret < 0)
		goto err_read;

	if (data == 0xFF)
		data = 0;

	info->disassemble_count = data;

err_read:
	return ret;
}

static void increase_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_get_disassemble_count(info);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: get failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}
	sec->cmd_state = SEC_CMD_STATUS_RUNNING;

	if (info->disassemble_count < 0xFE)
		info->disassemble_count++;

	ret = set_nvm_data(info, FTS_NVM_OFFSET_DISASSEMBLE_COUNT, &info->disassemble_count);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: set failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_get_disassemble_count(info);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: get failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%d", info->disassemble_count);

		sec_cmd_set_cmd_result(sec, buff, strlen(buff));
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
}

#ifdef FTS_SUPPORT_PRESSURE_SENSOR
int fts_set_pressure_calibration_information(struct fts_ts_info *info, u8 base, u8 delta)
{
	u8 *data = NULL;
	int ret;

	/* save calibration count */
	data = kzalloc(nvm_data[SEC_FACTORY_DEBUG].length, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data[0] = base;
	data[1] = delta;
	data[2] = 0x00;
	data[3] = 0x00;

	/* SEC_FACTORY_DEBUG index is 4. refer "fts_nvm_data_type" write command */
	ret = set_nvm_data(info, SEC_FACTORY_DEBUG, data);
	if (ret < 0)
		input_err(true, &info->client->dev,
				"%s: set failed. ret: %d\n", __func__, ret);

	kfree(data);

	return ret;
}

int fts_get_pressure_calibration_information(struct fts_ts_info *info)
{
	u8 *data = NULL;
	int ret;

	data = kzalloc(nvm_data[SEC_FACTORY_DEBUG].length, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret = get_nvm_data(info, SEC_FACTORY_DEBUG, data);
	if (ret <= 0) {
		input_err(true, &info->client->dev,
				"%s: set failed. ret: %d\n", __func__, ret);
	} else {
		info->pressure_cal_base = data[0];
		info->pressure_cal_delta = data[1];
		input_info(true, &info->client->dev,
				"%s: pressure base:%X, delta:%X\n", __func__,
				info->pressure_cal_base, info->pressure_cal_delta);
	}

	kfree(data);
	return ret;
}
#endif

int set_nvm_data_by_size(struct fts_ts_info *info, u8 offset, int length, u8 *buf)
{
	u8 regAdd[256] = { 0 };
	u8 remaining, index, sendinglength;
	int ret;

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true); // Clear FIFO

	fts_release_all_finger(info);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	remaining = length;
	index = 0;
	sendinglength = 0;

	while (remaining) {
		regAdd[0] = 0xC7;
		regAdd[1] = 0x01;
		regAdd[2] = offset + index;

		// write data up to 12 bytes available
		if (remaining < 13) {
			memcpy(&regAdd[3], &buf[index], remaining);
			sendinglength = remaining;
		} else {
			memcpy(&regAdd[3], &buf[index], 12);
			index += 12;
			sendinglength = 12;
		}

		ret = fts_write_reg(info, &regAdd[0], sendinglength + 3);
		if (ret < 0) {
			input_err(true, &info->client->dev,
					"%s: write failed. ret: %d\n", __func__, ret);
			return ret;
		}
		remaining -= sendinglength;
	}

	fts_interrupt_set(info, INT_DISABLE);

	// Save to flash
	regAdd[0] = 0xA4;
	regAdd[1] = 0x05;
	regAdd[2] = 0x04; // panel configuration area
	ret = info->fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: save to flash failed. ret: %d\n", __func__, ret);
		goto out;
	}

	fts_delay(200);
	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 3);
	if (ret < 0)
		input_err(true, &info->client->dev,
				"%s: failed to get echo. ret: %d\n", __func__, ret);

out:
	fts_interrupt_set(info, INT_ENABLE);

	return ret;
}

int set_nvm_data(struct fts_ts_info *info, u8 type, u8 *buf)
{
	return set_nvm_data_by_size(info, nvm_data[type].offset, nvm_data[type].length, buf);
}

int get_nvm_data_by_size(struct fts_ts_info *info, u8 offset, int length, u8 *nvdata)
{
	u8 regAdd[3] = {0};
	u8 data[128] = { 0 };
	int ret;

	info->fts_command(info, FTS_CMD_CLEAR_ALL_EVENT, true); // Clear FIFO

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	fts_interrupt_set(info, INT_DISABLE);

	// Request SEC factory debug data from flash
	regAdd[0] = 0xA4;
	regAdd[1] = 0x06;
	regAdd[2] = 0x90;
	ret = fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: write failed. ret: %d\n", __func__, ret);
		fts_interrupt_set(info, INT_ENABLE);
		return ret;
	}

	ret = fts_fw_wait_for_echo_event(info, &regAdd[0], 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: timeout. ret: %d\n", __func__, ret);
		fts_interrupt_set(info, INT_ENABLE);
		return ret;
	}

	fts_interrupt_set(info, INT_ENABLE);

	regAdd[0] = 0xA6;
	regAdd[1] = 0x00;
	regAdd[2] = offset;

	ret = fts_read_reg(info, &regAdd[0], 3, data, length + 1);
	if (ret < 0) {
		input_err(true, &info->client->dev,
				"%s: read failed. ret: %d\n", __func__, ret);
		return ret;
	}

	memcpy(nvdata, &data[0], length);

	input_raw_info(true, &info->client->dev, "%s: offset [%d], length [%d]\n",
 			__func__, offset, length);

	return ret;
}

int get_nvm_data(struct fts_ts_info *info, int type, u8 *nvdata)
{
	int size = sizeof(nvm_data) / sizeof(struct fts_nvm_data_map);

	if (type >= size)
		return -EINVAL;

	return get_nvm_data_by_size(info, nvm_data[type].offset, nvm_data[type].length, nvdata);
}

#ifdef TCLM_CONCEPT
int sec_tclm_data_read(struct i2c_client *client, int address)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);
	int ret = 0;
	int i = 0;
	u8 nbuff[FTS_NVM_OFFSET_ALL];

	switch (address) {
	case SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER:
		ret = info->fts_get_version_info(info);
		return info->fw_main_version_of_ic;
	case SEC_TCLM_NVM_ALL_DATA:
		ret = get_nvm_data_by_size(info, nvm_data[FTS_NVM_OFFSET_FAC_RESULT].offset,
				FTS_NVM_OFFSET_ALL, nbuff);
		if (ret < 0)
			return ret;
		info->tdata->nvdata.cal_count = nbuff[nvm_data[FTS_NVM_OFFSET_CAL_COUNT].offset];
		info->tdata->nvdata.tune_fix_ver = (nbuff[nvm_data[FTS_NVM_OFFSET_TUNE_VERSION].offset] << 8) |
							nbuff[nvm_data[FTS_NVM_OFFSET_TUNE_VERSION].offset + 1];
		info->tdata->nvdata.cal_position = nbuff[nvm_data[FTS_NVM_OFFSET_CAL_POSITION].offset];
		info->tdata->nvdata.cal_pos_hist_cnt = nbuff[nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_COUNT].offset];
		info->tdata->nvdata.cal_pos_hist_lastp = nbuff[nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_LASTP].offset];
		for (i = nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset;
				i < nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset +
				nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].length; i++)
			info->tdata->nvdata.cal_pos_hist_queue[i - nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset] = nbuff[i];

		info->fac_nv = nbuff[nvm_data[FTS_NVM_OFFSET_FAC_RESULT].offset];
		info->disassemble_count = nbuff[nvm_data[FTS_NVM_OFFSET_DISASSEMBLE_COUNT].offset];
		return ret;
	default:
		return ret;
	}
}

int sec_tclm_data_write(struct i2c_client *client)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);
	int ret = 1;
	int i = 0;
	u8 nbuff[FTS_NVM_OFFSET_ALL];

	memset(nbuff, 0x00, FTS_NVM_OFFSET_ALL);
	nbuff[nvm_data[FTS_NVM_OFFSET_FAC_RESULT].offset] = info->fac_nv;
	nbuff[nvm_data[FTS_NVM_OFFSET_DISASSEMBLE_COUNT].offset] = info->disassemble_count;
	nbuff[nvm_data[FTS_NVM_OFFSET_CAL_COUNT].offset] = info->tdata->nvdata.cal_count;
	nbuff[nvm_data[FTS_NVM_OFFSET_TUNE_VERSION].offset] = (u8)(info->tdata->nvdata.tune_fix_ver >> 8);
	nbuff[nvm_data[FTS_NVM_OFFSET_TUNE_VERSION].offset + 1] = (u8)(0xff & info->tdata->nvdata.tune_fix_ver);
	nbuff[nvm_data[FTS_NVM_OFFSET_CAL_POSITION].offset] = info->tdata->nvdata.cal_position;
	nbuff[nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_COUNT].offset] = info->tdata->nvdata.cal_pos_hist_cnt;
	nbuff[nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_LASTP].offset] = info->tdata->nvdata.cal_pos_hist_lastp;
	for (i = nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset;
			i < nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset +
			nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].length; i++)
		nbuff[i] = info->tdata->nvdata.cal_pos_hist_queue[i - nvm_data[FTS_NVM_OFFSET_HISTORY_QUEUE_ZERO].offset];
	ret = set_nvm_data_by_size(info, nvm_data[FTS_NVM_OFFSET_FAC_RESULT].offset, FTS_NVM_OFFSET_ALL, nbuff);

	return ret;
}
#endif

#ifdef CONFIG_GLOVE_TOUCH
static void glove_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {0};

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->glove_enabled = sec->cmd_param[0];

		if (info->fts_power_state != FTS_POWER_STATE_POWERDOWN && info->reinit_done) {
			if (info->glove_enabled)
				info->touch_functions = info->touch_functions | FTS_TOUCHTYPE_BIT_GLOVE |
							FTS_TOUCHTYPE_DEFAULT_ENABLE;
			else
				info->touch_functions = (info->touch_functions & (~FTS_TOUCHTYPE_BIT_GLOVE)) |
							FTS_TOUCHTYPE_DEFAULT_ENABLE;
		}

		regAdd[0] = FTS_CMD_SET_GET_TOUCHTYPE;
		regAdd[1] = (u8)(info->touch_functions & 0xFF);
		regAdd[2] = (u8)(info->touch_functions >> 8);
		fts_write_reg(info, &regAdd[0], 3);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_glove_sensitivity(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	u8 regAdd[4] = {0};
	int timeout = 0;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		sec_cmd_set_cmd_exit(sec);
		return;
	}

	// need to add
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

#endif

static void clear_cover_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0] > 1) {
			info->flip_enable = true;
			info->cover_type = sec->cmd_param[1];
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
			if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
				fts_delay(500);
				tui_force_close(1);
				fts_delay(200);
				if (TRUSTEDUI_MODE_TUI_SESSION & trustedui_get_current_mode()) {
					trustedui_clear_mask(TRUSTEDUI_MODE_VIDEO_SECURED|TRUSTEDUI_MODE_INPUT_SECURED);
					trustedui_set_mode(TRUSTEDUI_MODE_OFF);
				}
			}

			tui_cover_mode_set(true);
#endif
		} else {
			info->flip_enable = false;
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
			tui_cover_mode_set(false);
#endif
		}

		if (info->fts_power_state != FTS_POWER_STATE_POWERDOWN && info->reinit_done) {
			if (info->flip_enable)
				fts_set_cover_type(info, true);
			else
				fts_set_cover_type(info, false);
		}

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
};

static void report_rate(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 scan_rate;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 255) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {

		scan_rate = sec->cmd_param[0];
		fts_change_scan_rate(info, scan_rate);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;

out:
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
static void interrupt_control(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		int enables = sec->cmd_param[0];

		if (enables)
			fts_irq_enable(info, true);
		else
			fts_irq_enable(info, false);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;

out:
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

static void set_wirelesscharger_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		info->charger_mode = sec->cmd_param[0];

		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->charger_mode = sec->cmd_param[0];

		fts_wirelesscharger_mode(info);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;

out:
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
};

/*********************************************************************
 * flag	1  :  set edge handler
 *	2  :  set (portrait, normal) edge zone data
 *	4  :  set (portrait, normal) dead zone data
 *	8  :  set landscape mode data
 *	16 :  mode clear
 * data
 *	0x00, FFF (y start), FFF (y end),  FF(direction)
 *	0x01, FFFF (edge zone)
 *	0x02, FF (up x), FF (down x), FFFF (y)
 *	0x03, FF (mode), FFF (edge), FFF (dead zone)
 * case
 *	edge handler set :  0x00....
 *	booting time :  0x00...  + 0x01...
 *	normal mode : 0x02...  (+0x01...)
 *	landscape mode : 0x03...
 *	landscape -> normal (if same with old data) : 0x03, 0
 *	landscape -> normal (etc) : 0x02....  + 0x03, 0
 *********************************************************************/

void fts_set_grip_data_to_ic(struct fts_ts_info *info, u8 flag)
{
	u8 data[4] = { 0 };
	u8 regAdd[6] = {0xC3, 0x00, 0x00, 0x00, 0x00, 0x00};

	input_info(true, &info->client->dev, "%s: flag: %02X (clr,lan,nor,edg,han)\n", __func__, flag);

	if (flag & G_SET_EDGE_HANDLER) {
		if (info->grip_edgehandler_direction == 0) {
			data[0] = 0x0;
			data[1] = 0x0;
			data[2] = 0x0;
			data[3] = 0x0;
		} else {
			data[0] = (info->grip_edgehandler_start_y >> 4) & 0xFF;
			data[1] = (info->grip_edgehandler_start_y << 4 & 0xF0) |
					((info->grip_edgehandler_end_y >> 8) & 0xF);
			data[2] = info->grip_edgehandler_end_y & 0xFF;
			data[3] = info->grip_edgehandler_direction & 0x3;
		}

		regAdd[1] = FTS_CMD_EDGE_HANDLER;
		regAdd[2] = data[0];
		regAdd[3] = data[1];
		regAdd[4] = data[2];
		regAdd[5] = data[3];

		fts_write_reg(info, regAdd, 6);
		input_info(true, &info->client->dev, "%s: 0x%02X %02X,%02X,%02X,%02X\n",
				__func__, FTS_CMD_EDGE_HANDLER, data[0], data[1], data[2], data[3]);
	}

	if (flag & G_SET_EDGE_ZONE) {
		data[0] = (info->grip_edge_range >> 8) & 0xFF;
		data[1] = info->grip_edge_range  & 0xFF;

		regAdd[1] = FTS_CMD_EDGE_AREA;
		regAdd[2] = data[0];
		regAdd[3] = data[1];

		fts_write_reg(info, regAdd, 4);
		input_info(true, &info->client->dev, "%s: 0x%02X %02X,%02X\n",
				__func__, FTS_CMD_EDGE_AREA, data[0], data[1]);
	}

	if (flag & G_SET_NORMAL_MODE) {
		data[0] = info->grip_deadzone_up_x & 0xFF;
		data[1] = info->grip_deadzone_dn_x & 0xFF;
		data[2] = (info->grip_deadzone_y >> 8) & 0xFF;
		data[3] = info->grip_deadzone_y & 0xFF;

		regAdd[1] = FTS_CMD_DEAD_ZONE;
		regAdd[2] = data[0];
		regAdd[3] = data[1];
		regAdd[4] = data[2];
		regAdd[5] = data[3];

		fts_write_reg(info, regAdd, 6);
		input_info(true, &info->client->dev, "%s: 0x%02X %02X,%02X,%02X,%02X\n",
				__func__, FTS_CMD_DEAD_ZONE, data[0], data[1], data[2], data[3]);
	}

	if (flag & G_SET_LANDSCAPE_MODE) {
		data[0] = info->grip_landscape_mode & 0x1;
		data[1] = (info->grip_landscape_edge >> 4) & 0xFF;
		data[2] = (info->grip_landscape_edge << 4 & 0xF0) | ((info->grip_landscape_deadzone >> 8) & 0xF);
		data[3] = info->grip_landscape_deadzone & 0xFF;

		regAdd[1] = FTS_CMD_LANDSCAPE_MODE;
		regAdd[2] = data[0];
		regAdd[3] = data[1];
		regAdd[4] = data[2];
		regAdd[5] = data[3];

		fts_write_reg(info, regAdd, 6);
		input_info(true, &info->client->dev, "%s: 0x%02X %02X,%02X,%02X,%02X\n",
				__func__, FTS_CMD_LANDSCAPE_MODE, data[0], data[1], data[2], data[3]);
	}

	if (flag & G_CLR_LANDSCAPE_MODE) {
		data[0] = info->grip_landscape_mode;

		regAdd[1] = FTS_CMD_LANDSCAPE_MODE;
		regAdd[2] = data[0];

		fts_write_reg(info, regAdd, 3);
		input_info(true, &info->client->dev, "%s: 0x%02X %02X\n",
				__func__, FTS_CMD_LANDSCAPE_MODE, data[0]);
	}
}

/*********************************************************************
 * index
 *	0 :  set edge handler
 *	1 :  portrait (normal) mode
 *	2 :  landscape mode
 * data
 *	0, X (direction), X (y start), X (y end)
 *	direction : 0 (off), 1 (left), 2 (right)
 *	ex) echo set_grip_data,0,2,600,900 > cmd
 *
 *
 *	1, X (edge zone), X (dead zone up x), X (dead zone down x), X (dead zone y)
 *	ex) echo set_grip_data,1,200,10,50,1500 > cmd
 *
 *	2, 1 (landscape mode), X (edge zone), X (dead zone)
 *	ex) echo set_grip_data,2,1,200,100 > cmd
 *
 *	2, 0 (portrait mode)
 *	ex) echo set_grip_data,2,0  > cmd
 *********************************************************************/
static void set_grip_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 mode = G_NONE;

	sec_cmd_set_default_result(sec);

	memset(buff, 0, sizeof(buff));

	mutex_lock(&info->device_mutex);

	if (sec->cmd_param[0] == 0) {	// edge handler
		if (sec->cmd_param[1] == 0) {	// clear
			info->grip_edgehandler_direction = 0;
		} else if (sec->cmd_param[1] < 3) {
			info->grip_edgehandler_direction = sec->cmd_param[1];
			info->grip_edgehandler_start_y = sec->cmd_param[2];
			info->grip_edgehandler_end_y = sec->cmd_param[3];
		} else {
			input_err(true, &info->client->dev, "%s: cmd1 is abnormal, %d (%d)\n",
					__func__, sec->cmd_param[1], __LINE__);
			goto err_grip_data;
		}

		mode = mode | G_SET_EDGE_HANDLER;
		fts_set_grip_data_to_ic(info, mode);
	} else if (sec->cmd_param[0] == 1) {	// normal mode
		if (info->grip_edge_range != sec->cmd_param[1])
			mode = mode | G_SET_EDGE_ZONE;

		info->grip_edge_range = sec->cmd_param[1];
		info->grip_deadzone_up_x = sec->cmd_param[2];
		info->grip_deadzone_dn_x = sec->cmd_param[3];
		info->grip_deadzone_y = sec->cmd_param[4];
		mode = mode | G_SET_NORMAL_MODE;

		if (info->grip_landscape_mode == 1) {
			info->grip_landscape_mode = 0;
			mode = mode | G_CLR_LANDSCAPE_MODE;
		}

		fts_set_grip_data_to_ic(info, mode);
	} else if (sec->cmd_param[0] == 2) {	// landscape mode
		if (sec->cmd_param[1] == 0) {	// normal mode
			info->grip_landscape_mode = 0;
			mode = mode | G_CLR_LANDSCAPE_MODE;
		} else if (sec->cmd_param[1] == 1) {
			info->grip_landscape_mode = 1;
			info->grip_landscape_edge = sec->cmd_param[2];
			info->grip_landscape_deadzone = sec->cmd_param[3];
			mode = mode | G_SET_LANDSCAPE_MODE;
		} else {
			input_err(true, &info->client->dev, "%s: cmd1 is abnormal, %d (%d)\n",
					__func__, sec->cmd_param[1], __LINE__);
			goto err_grip_data;
		}

		fts_set_grip_data_to_ic(info, mode);
	} else {
		input_err(true, &info->client->dev, "%s: cmd0 is abnormal, %d", __func__, sec->cmd_param[0]);
		goto err_grip_data;
	}

	mutex_unlock(&info->device_mutex);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;

err_grip_data:
	mutex_unlock(&info->device_mutex);

	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
}

static void dead_zone_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {0xC1, 0x04, 0x00};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0] == 0)
			regAdd[2] = 0x01;	/* dead zone disable */
		else
			regAdd[2] = 0x00;	/* dead zone enable */

		ret = info->fts_write_reg(info, regAdd, 3);
		if (ret < 0)
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		else
			input_info(true, &info->client->dev, "%s: reg:%d, ret: %d\n", __func__, sec->cmd_param[0], ret);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

#ifdef FTS_SUPPORT_PRESSURE_SENSOR
static void drawing_test_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	int ret = 0;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (!sec->cmd_param[0])
		info->lowpower_flag |= FTS_MODE_PRESSURE;
	else
		info->lowpower_flag &= ~FTS_MODE_PRESSURE;

#ifdef FTS_SUPPORT_SPONGELIB
	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
			&info->lowpower_flag, sizeof(info->lowpower_flag));
#endif
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

		goto out;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

out:
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
#endif

static void spay_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	int ret = 0;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0])
		info->lowpower_flag |= FTS_MODE_SPAY;
	else
		info->lowpower_flag &= ~FTS_MODE_SPAY;

#ifdef FTS_SUPPORT_SPONGELIB
	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
			&info->lowpower_flag, sizeof(info->lowpower_flag));
#endif
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

		goto out;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

out:
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void aot_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
#ifdef FTS_SUPPORT_SPONGELIB
	int ret = 0;
#endif
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0])
		info->lowpower_flag |= FTS_MODE_DOUBLETAP_WAKEUP;
	else
		info->lowpower_flag &= ~FTS_MODE_DOUBLETAP_WAKEUP;

#ifdef FTS_SUPPORT_SPONGELIB
	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
			&info->lowpower_flag, sizeof(info->lowpower_flag));
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec_cmd_set_cmd_exit(sec);
		return;
	}
#endif
	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	input_info(true, &info->client->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
}

static void aod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
#ifdef FTS_SUPPORT_SPONGELIB
	int ret = 0;
#endif
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0])
		info->lowpower_flag |= FTS_MODE_AOD;
	else
		info->lowpower_flag &= ~FTS_MODE_AOD;

#ifdef FTS_SUPPORT_SPONGELIB
	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_MODE,
			&info->lowpower_flag, sizeof(info->lowpower_flag));
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec_cmd_set_cmd_exit(sec);
		return;
	}
#endif
	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	input_info(true, &info->client->dev, "%s: %d\n", __func__, sec->cmd_param[0]);
}

static void set_aod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[8] = {0, };
	int i, ret = -1;

	sec_cmd_set_default_result(sec);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	input_info(true, &info->client->dev, "%s: w:%d, h:%d, x:%d, y:%d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1],
			sec->cmd_param[2], sec->cmd_param[3]);
#endif

	for (i = 0; i < 4; i++) {
		data[i * 2] = sec->cmd_param[i] & 0xFF;
		data[i * 2 + 1] = (sec->cmd_param[i] >> 8) & 0xFF;
		info->rect_data[i] = sec->cmd_param[i];
	}

#ifdef FTS_SUPPORT_SPONGELIB
	ret = info->fts_write_to_sponge(info, FTS_CMD_SPONGE_OFFSET_AOD_RECT, data, sizeof(data));
#endif
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
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

static void get_aod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[8] = {0, };
	u16 rect_data[4] = {0, };
	int i, ret = -1;

	sec_cmd_set_default_result(sec);

#ifdef FTS_SUPPORT_SPONGELIB
	ret = info->fts_read_from_sponge(info, FTS_CMD_SPONGE_OFFSET_AOD_RECT, data, sizeof(data));
#endif
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto NG;
	}

	for (i = 0; i < 4; i++)
		rect_data[i] = (data[i * 2 + 1] & 0xFF) << 8 | (data[i * 2] & 0xFF);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	input_info(true, &info->client->dev, "%s: w:%d, h:%d, x:%d, y:%d\n",
			__func__, rect_data[0], rect_data[1], rect_data[2], rect_data[3]);
#endif

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
 *  C2 13: Disable dex mode
 *  C1 13 01: Full screen mode
 *  C1 13 02: Iris mode
 */
static void dex_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {0xC1, 0x01, 0x00};
	int ret;

	sec_cmd_set_default_result(sec);

	if (!info->board->support_dex) {
		input_err(true, &info->client->dev, "%s: not support DeX mode\n", __func__);
		goto out;
	}

	if ((sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) &&
			(sec->cmd_param[1] < 0 || sec->cmd_param[1] > 1)) {
		input_err(true, &info->client->dev, "%s: not support param\n", __func__);
		goto out;
	}

	info->dex_mode = sec->cmd_param[0];
	if (info->dex_mode) {
		input_err(true, &info->client->dev, "%s: set DeX touch_pad mode%s\n",
				__func__, sec->cmd_param[1] ? " & Iris mode" : "");
		info->input_dev = info->input_dev_pad;
		if (sec->cmd_param[1]) {
			/* Iris mode */
			info->dex_mode = 0x02;
			info->dex_name = "[DeXI]";
		} else {
			info->dex_name = "[DeX]";
		}
		regAdd[2] = info->dex_mode;
	} else {
		input_err(true, &info->client->dev, "%s: set touch mode\n", __func__);
		info->input_dev = info->input_dev_touch;
		info->dex_name = "";
		regAdd[2] = 0x00;
	}

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		goto out;
	}

	ret = info->fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
	else
		input_info(true, &info->client->dev, "%s: reg:%d, ret: %d\n", __func__, regAdd[2], ret);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_exit(sec);
	return;

out:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_exit(sec);
}

static void brush_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {0xC1, 0x02, 0x00};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	info->brush_mode = sec->cmd_param[0];

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	input_info(true, &info->client->dev,
			"%s: set brush mode %s\n", __func__, info->brush_mode ? "enable" : "disable");

	if (info->brush_mode == 0)
		regAdd[2] = 0x00;	/* 0: Disable Artcanvas min phi mode */
	else
		regAdd[2] = 0x01;	/* 1: Enable Artcanvas min phi mode  */

	ret = info->fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
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

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_touchable_area(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[3] = {0xC1, 0x03, 0x00};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	info->touchable_area = sec->cmd_param[0];

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	input_info(true, &info->client->dev,
			"%s: set 16:9 mode %s\n", __func__, info->touchable_area ? "enable" : "disable");

	if (info->touchable_area == 0)
		regAdd[2] = 0x00;	/* 0: Disable 16:9 mode */
	else
		regAdd[2] = 0x01;	/* 1: Enable 16:9 mode  */

	ret = info->fts_write_reg(info, &regAdd[0], 3);
	if (ret < 0) {
		input_err(true, &info->client->dev,
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

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void delay(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->delay_time = sec->cmd_param[0];

	input_info(true, &info->client->dev, "%s time is %d\n", __func__, info->delay_time);
	snprintf(buff, sizeof(buff), "%d", info->delay_time);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void debug(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->debug_string = sec->cmd_param[0];

	input_info(true, &info->client->dev, "%s: command is %d\n", __func__, info->debug_string);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_force_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	bool touch_on = false;

	sec_cmd_set_default_result(sec);

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
#ifdef TCLM_CONCEPT
		info->tdata->external_factory = false;
#endif
		return;
	}

	if (info->rawdata_read_lock == 1) {
		input_err(true, &info->client->dev, "%s: ramdump mode is running, %d\n",
				__func__, info->rawdata_read_lock);
		goto autotune_fail;
	}

	if (info->touch_count > 0) {
		touch_on = true;
		input_err(true, &info->client->dev, "%s: finger on touch(%d)\n", __func__, info->touch_count);
	}

	info->fts_systemreset(info);
	info->fts_wait_for_ready(info);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	if (touch_on) {
		input_err(true, &info->client->dev, "%s: finger! do not run autotune\n", __func__);
	} else {
		input_info(true, &info->client->dev, "%s: run autotune\n", __func__);

		input_err(true, &info->client->dev, "%s: RUN OFFSET CALIBRATION \n", __func__);
		if (fts_execute_autotune(info, true) < 0) {
			fts_set_scanmode(info, info->scan_mode);
			goto autotune_fail;
		}
#ifdef TCLM_CONCEPT
		/* devide tclm case */
		sec_tclm_case(info->tdata, sec->cmd_param[0]);

		input_info(true, &info->client->dev, "%s: param, %d, %c, %d\n", __func__,
			sec->cmd_param[0], sec->cmd_param[0], info->tdata->root_of_calibration);

		if (sec_execute_tclm_package(info->tdata, 1) < 0)
			input_err(true, &info->client->dev,
						"%s: sec_execute_tclm_package\n", __func__);

		sec_tclm_root_of_cal(info->tdata, CALPOSITION_NONE);
#endif
	}

	fts_set_scanmode(info, info->scan_mode);

	if (touch_on) {
		snprintf(buff, sizeof(buff), "NG_FINGER_ON");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
#ifdef TCLM_CONCEPT
	info->tdata->external_factory = false;
#endif

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;

autotune_fail:
#ifdef TCLM_CONCEPT
	info->tdata->external_factory = false;
#endif
	fts_interrupt_set(info, INT_ENABLE);
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

#ifdef FTS_SUPPORT_TOUCH_KEY
int read_touchkey_data(struct fts_ts_info *info, u8 type, unsigned int keycode)
{
	u8 pCMD[3] = { 0xD0, 0x00, 0x00};
	u8 buf[9] = { 0 };
	int i;
	int ret = 0;

	pCMD[2] = type;

	ret = fts_read_reg(info, &pCMD[0], 3, buf, 3);
	if (ret >= 0) {
		pCMD[1] = buf[2];
		pCMD[2] = buf[1];
	} else {
		return -1;
	}

	ret = fts_read_reg(info, &pCMD[0], 3, buf, 9);
	if (ret < 0)
		return -2;

	for (i = 0 ; i < info->board->num_touchkey ; i++)
		if (info->board->touchkey[i].keycode == keycode)
			return *(short *)&buf[(info->board->touchkey[i].value - 1) * 2 + 1];

	return -3;
}

static ssize_t touchkey_recent_strength(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return sprintf(buf, "%d\n", value);
	}

	value = read_touchkey_data(info, TYPE_TOUCHKEY_STRENGTH, KEY_RECENT);

	return sprintf(buf, "%d\n", value);
}

static ssize_t touchkey_back_strength(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return sprintf(buf, "%d\n", value);
	}

	value = read_touchkey_data(info, TYPE_TOUCHKEY_STRENGTH, KEY_BACK);

	return sprintf(buf, "%d\n", value);
}

static ssize_t touchkey_recent_raw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return sprintf(buf, "%d\n", value);
	}

	value = read_touchkey_data(info, TYPE_TOUCHKEY_RAW, KEY_RECENT);

	return sprintf(buf, "%d\n", value);
}

static ssize_t touchkey_back_raw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->fts_power_state == FTS_POWER_STATE_POWERDOWN) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return sprintf(buf, "%d\n", value);
	}

	value = read_touchkey_data(info, TYPE_TOUCHKEY_RAW, KEY_BACK);

	return sprintf(buf, "%d\n", value);
}

static ssize_t touchkey_threshold(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	u8 pCMD[3] = { 0xD0, 0x00, 0x00};
	int value;
	int ret = 0;

	value = -1;
	pCMD[2] = FTS_SI_SS_KEY_THRESHOLD;
	ret = fts_read_reg(info, &pCMD[0], 3, buf, 3);
	if (ret >= 0)
		value = *(u16 *)&buf[1];

	info->touchkey_threshold = value;
	return sprintf(buf, "%d\n", info->touchkey_threshold);
}

static ssize_t fts_touchkey_led_control(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int ret;
	unsigned long data;

	if (size > 2) {
		input_err(true, &info->client->dev,
				"%s: cmd length is over (%s,%d)!!\n",
				__func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	ret = kstrtoul(buf, 10, &data);
	if (ret != 0) {
		input_err(true, &info->client->dev, "%s: failed to read:%d\n",
				__func__, ret);
		return -EINVAL;
	}
	input_dbg(true, &info->client->dev, "%s: %d\n", __func__, data);

	if (data != 0 && data != 1) {
		input_err(true, &info->client->dev, "%s: wrong cmd %x\n",
				__func__, data);
		return size;
	}

	ret = info->board->led_power(info, (bool)data);
	if (ret) {
		input_err(true, &info->client->dev, "%s: Error turn on led %d\n",
				__func__, ret);

		goto out;
	}
	msleep(30);

out:
	return size;
}

static DEVICE_ATTR(brightness, 0644, NULL, fts_touchkey_led_control);
static DEVICE_ATTR(touchkey_recent, 0444, touchkey_recent_strength, NULL);
static DEVICE_ATTR(touchkey_back, 0444, touchkey_back_strength, NULL);
static DEVICE_ATTR(touchkey_recent_raw, 0444, touchkey_recent_raw, NULL);
static DEVICE_ATTR(touchkey_back_raw, 0444, touchkey_back_raw, NULL);
static DEVICE_ATTR(touchkey_threshold, 0444, touchkey_threshold, NULL);

static struct attribute *sec_touchkey_factory_attributes[] = {
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_back_raw.attr,
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_brightness.attr,
	NULL,
};

static struct attribute_group sec_touchkey_factory_attr_group = {
	.attrs = sec_touchkey_factory_attributes,
};
#endif

#endif
