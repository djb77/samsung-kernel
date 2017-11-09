#ifdef SEC_TSP_FACTORY_TEST

#define BUFFER_MAX			((256 * 1024) - 16)
#define READ_CHUNK_SIZE			128 // (2 * 1024) - 16

enum {
	TYPE_RAW_DATA = 0,
	TYPE_FILTERED_DATA = 2,
	TYPE_STRENGTH_DATA = 4,
	TYPE_BASELINE_DATA = 6
};

enum {
	BUILT_IN = 0,
	UMS,
};

#ifdef FTS_SUPPORT_TOUCH_KEY
enum {
	TYPE_TOUCHKEY_RAW	= 0x34,
	TYPE_TOUCHKEY_STRENGTH	= 0x36,
};
#endif

#define FTS_FLASH_DATA_OFFSET_BASE			16

#define FTS_FLASH_WRITE_COMMAND_SEC_FACTORY_DEBUG	4
enum fts_nvm_data_type {		/* Write Command */
	FACTORY_TEST_RESULT = 1,	/* C7 01 */
	CALIBRATION_INFOMATION,		/* C7 02 */
	PRESSURE_STRENGTH,		/* C7 03 00 02*/
	PRESSURE_RAWDATA,		/* C7 03 00 03*/
	GROUP_INDEX,			/* C7 03 00 04*/
	SEC_FACTORY_DEBUG,		/* C7 04 */
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
	{NVM_CMD(0, 0, 0),},
	{NVM_CMD(FACTORY_TEST_RESULT, 0x0, 2),},	/* SEC */
	{NVM_CMD(CALIBRATION_INFOMATION, 0x2, 4),},	/* SEC */
	{NVM_CMD(PRESSURE_STRENGTH, 0x6, 0x20),},	/* STM */
	{NVM_CMD(PRESSURE_RAWDATA, 0x26, 0x20),},	/* STM */
	{NVM_CMD(GROUP_INDEX, 0x46, 1),},		/* STM */
	{NVM_CMD(SEC_FACTORY_DEBUG, 0x47, 4),},		/* SEC */
};

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
static void get_rawcap(void *device_data);
static void run_delta_read(void *device_data);
static void get_delta(void *device_data);
static void get_pat_information(void *device_data);
static void set_external_factory(void *device_data);
#ifdef FTS_SUPPORT_HOVER
static void run_abscap_read(void *device_data);
static void run_absdelta_read(void *device_data);
#endif
static void run_ix_data_read(void *device_data);
static void run_ix_data_read_all(void *device_data);
static void run_self_raw_read(void *device_data);
static void run_self_raw_read_all(void *device_data);
static void run_wtr_cx_data_read(void *device_data);
static void run_wtr_cx_data_read_all(void *device_data);
static void run_trx_short_test(void *device_data);
static void get_cx_data(void *device_data);
static void run_cx_data_read(void *device_data);
static void get_cx_all_data(void *device_data);
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
#ifdef FTS_SUPPORT_HOVER
static void hover_enable(void *device_data);
/* static void hover_no_sleep_enable(void *device_data); */
#endif
#ifdef CONFIG_GLOVE_TOUCH
static void glove_mode(void *device_data);
static void get_glove_sensitivity(void *device_data);
static void fast_glove_mode(void *device_data);
#endif
static void clear_cover_mode(void *device_data);
static void report_rate(void *device_data);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
static void interrupt_control(void *device_data);
#endif

static void set_wirelesscharger_mode(void *device_data);
static void set_grip_data(void *device_data);
static void set_dead_zone(void *device_data);
static void dead_zone_enable(void *device_data);
static void drawing_test_enable(void *device_data);
static void spay_enable(void *device_data);
static void aod_enable(void *device_data);
static void set_aod_rect(void *device_data);
static void get_aod_rect(void *device_data);
static void dex_enable(void *device_data);
static void brush_enable(void *device_data);
static void set_touchable_area(void *device_data);
static void delay(void *device_data);
static void debug(void *device_data);

static void run_force_calibration(void *device_data);

static void not_support_cmd(void *device_data);

static ssize_t fts_scrub_position(struct device *dev,
				struct device_attribute *attr, char *buf);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
extern int tui_force_close(uint32_t arg);
extern void tui_cover_mode_set(bool arg);
#endif

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
	{SEC_CMD("get_rawcap", get_rawcap),},
	{SEC_CMD("run_delta_read", run_delta_read),},
	{SEC_CMD("get_delta", get_delta),},
	{SEC_CMD("get_pat_information", get_pat_information),},
	{SEC_CMD("set_external_factory", set_external_factory),},
#ifdef FTS_SUPPORT_HOVER
	{SEC_CMD("run_abscap_read" , run_abscap_read),},
	{SEC_CMD("run_absdelta_read", run_absdelta_read),},
#endif
	{SEC_CMD("run_ix_data_read", run_ix_data_read),},
	{SEC_CMD("run_ix_data_read_all", run_ix_data_read_all),},
	{SEC_CMD("run_self_raw_read", run_self_raw_read),},
	{SEC_CMD("run_self_raw_read_all", run_self_raw_read_all),},
	{SEC_CMD("run_wtr_cx_data_read", run_wtr_cx_data_read),},
	{SEC_CMD("run_wtr_cx_data_read_all", run_wtr_cx_data_read_all),},
	{SEC_CMD("run_trx_short_test", run_trx_short_test),},
	{SEC_CMD("get_cx_data", get_cx_data),},
	{SEC_CMD("run_cx_data_read", run_cx_data_read),},
	{SEC_CMD("get_cx_all_data", get_cx_all_data),},
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
#ifdef FTS_SUPPORT_HOVER
	{SEC_CMD("hover_enable", hover_enable),},
#endif
#ifdef CONFIG_GLOVE_TOUCH
	{SEC_CMD("glove_mode", glove_mode),},
	{SEC_CMD("get_glove_sensitivity", get_glove_sensitivity),},
	{SEC_CMD("fast_glove_mode", fast_glove_mode),},
#endif
	{SEC_CMD("clear_cover_mode", clear_cover_mode),},
	{SEC_CMD("report_rate", report_rate),},
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	{SEC_CMD("interrupt_control", interrupt_control),},
#endif
	{SEC_CMD("set_wirelesscharger_mode", set_wirelesscharger_mode),},
	{SEC_CMD("set_grip_data", set_grip_data),},
	{SEC_CMD("set_dead_zone", set_dead_zone),},
	{SEC_CMD("dead_zone_enable", dead_zone_enable),},
	{SEC_CMD("drawing_test_enable", drawing_test_enable),},
	{SEC_CMD("spay_enable", spay_enable),},
	{SEC_CMD("aod_enable", aod_enable),},
	{SEC_CMD("set_aod_rect", set_aod_rect),},
	{SEC_CMD("get_aod_rect", get_aod_rect),},
	{SEC_CMD("dex_enable", dex_enable),},
	{SEC_CMD("brush_enable", brush_enable),},
	{SEC_CMD("set_touchable_area", set_touchable_area),},
	{SEC_CMD("delay", delay),},
	{SEC_CMD("debug", debug),},
	{SEC_CMD("run_force_calibration", run_force_calibration),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

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
	info->dive_count= 0;

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

	return snprintf(buf, SEC_CMD_BUF_SIZE, "ST%02X%04X%02X%02X%02X%02X",
		info->panel_revision, info->fw_main_version_of_ic,
		info->test_result.data[0], info->cal_count, info->pressure_cal_base, info->pressure_cal_delta);
}

static ssize_t read_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	unsigned char buffer[10] = { 0 };

	snprintf(buffer, 9, info->board->firmware_name + 8);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "STM_%s", buffer);
}

static ssize_t read_checksum_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	input_info(true, &info->client->dev, "%s: crc fail count in NV: %d\n", __func__, info->nv_crc_fail_count);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d", info->nv_crc_fail_count);
}


static ssize_t clear_checksum_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

//	info->checksum_result = 0;

	input_info(true, &info->client->dev, "%s:nothing\n", __func__);

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

	info->max_z_value= 0;
	info->min_z_value= 0xFFFFFFFF;
	info->sum_z_value= 0;
	info->all_finger_count = 0;

	input_info(true, &info->client->dev, "%s: clear\n", __func__);

	return count;
}

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
#ifdef FTS_SUPPORT_STRINGLIB
	unsigned short addr = FTS_CMD_STRING_ACCESS;
#endif

	if (count > 2)
		return -EINVAL;

	ret = kstrtoul(buf, 10, &value);
	if (ret != 0)
		return ret;

	if (value == 1) {
		info->lowpower_flag |= FTS_MODE_PRESSURE;
	} else {
		info->lowpower_flag &= ~FTS_MODE_PRESSURE;
	}

#ifdef FTS_SUPPORT_STRINGLIB
	ret = info->fts_write_to_string(info, &addr, &info->lowpower_flag, sizeof(info->lowpower_flag));
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
#endif

	input_info(true, &info->client->dev, "%s: %d\n", __func__, value);
	return count;
}

static DEVICE_ATTR(ito_check, S_IRUGO, read_ito_check_show, NULL);
static DEVICE_ATTR(raw_check, S_IRUGO, read_raw_check_show, NULL);
static DEVICE_ATTR(multi_count, S_IRUGO | S_IWUSR | S_IWGRP, read_multi_count_show, clear_multi_count_store);
static DEVICE_ATTR(wet_mode, S_IRUGO | S_IWUSR | S_IWGRP, read_wet_mode_show, clear_wet_mode_store);
static DEVICE_ATTR(comm_err_count, S_IRUGO | S_IWUSR | S_IWGRP, read_comm_err_count_show, clear_comm_err_count_store);
static DEVICE_ATTR(module_id, S_IRUGO, read_module_id_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, read_vendor_show, NULL);
static DEVICE_ATTR(checksum, S_IRUGO | S_IWUSR | S_IWGRP, read_checksum_show, clear_checksum_store);
static DEVICE_ATTR(holding_time, S_IRUGO | S_IWUSR | S_IWGRP, read_holding_time_show, clear_holding_time_store);
static DEVICE_ATTR(all_touch_count, S_IRUGO | S_IWUSR | S_IWGRP, read_all_touch_count_show, clear_all_touch_count_store);
static DEVICE_ATTR(z_value, S_IRUGO | S_IWUSR | S_IWGRP, read_z_value_show, clear_z_value_store);
static DEVICE_ATTR(scrub_pos, S_IRUGO, fts_scrub_position, NULL);
static DEVICE_ATTR(pressure_enable, S_IRUGO | S_IWUSR | S_IWGRP, pressure_enable_show, pressure_enable_store);

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
	&dev_attr_pressure_enable.attr,
	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};

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
	if (info->touch_stopped) {
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

	return;
}

static int fts_get_channel_info(struct fts_ts_info *info)   // Need to change function for sysinfo
{
	int rc = -1;
	unsigned char data[FTS_EVENT_SIZE] = { 0 };

	memset(data, 0x0, FTS_EVENT_SIZE);

	fts_interrupt_set(info, INT_DISABLE);
	fts_command(info, FLUSHBUFFER);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	// Read Sense Channel
	rc = info->fts_get_sysinfo_data(info, FTS_SI_SENSE_CH_LENGTH, 3, data);
	if (rc <= 0) {
		info->SenseChannelLength = 0;
		input_err(true, info->dev, "%s: Get channel info Read Fail!! [Data : %2X]\n", __func__, data[0]);
		return rc;
	}
	info->SenseChannelLength = data[0];

	// Read Force Channel
	rc = info->fts_get_sysinfo_data(info, FTS_SI_FORCE_CH_LENGTH, 3, data);
	if (rc <= 0) {
		info->ForceChannelLength = 0;
		input_err(true, info->dev, "%s: Get channel info Read Fail!! [Data : %2X]\n", __func__, data[0]);
		return rc;
	}
	info->ForceChannelLength = data[0];

	fts_interrupt_set(info, INT_ENABLE);

	return rc;
}

void fts_print_frame(struct fts_ts_info *info, short *min, short *max)
{
	int i = 0;
	int j = 0;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };

	pStr = kzalloc(6 * (info->SenseChannelLength + 1), GFP_KERNEL);
	if (pStr == NULL) {
		input_err(true, &info->client->dev, "%s: pStr kzalloc failed\n", __func__);
		return;
	}

	snprintf(pTmp, 4, "    ");
	strncat(pStr, pTmp, 4);


	for (i = 0; i < info->SenseChannelLength; i++) {
		snprintf(pTmp, 6, "Rx%02d  ", i);
		strncat(pStr, pTmp, 6);
	}

	input_info(true, &info->client->dev, "%s\n", pStr);

	memset(pStr, 0x0, 6 * (info->SenseChannelLength + 1));
	snprintf(pTmp, 2, " +");
	strncat(pStr, pTmp, 2);

	for (i = 0; i < info->SenseChannelLength; i++) {
		snprintf(pTmp, 6, "------");
		strncat(pStr, pTmp, 6);

	}

	input_info(true, &info->client->dev, "%s\n", pStr);

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
		input_info(true, &info->client->dev, "%s\n", pStr);
	}

	kfree(pStr);
}

int fts_read_frame(struct fts_ts_info *info, unsigned char type, short *min,
		 short *max)
{
	unsigned char pFrameAddress[8] =
	{ 0xD0, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00 };
	unsigned int FrameAddress = 0;
	unsigned int writeAddr = 0;
	unsigned int start_addr = 0;
	unsigned int end_addr = 0;
	unsigned int totalbytes = 0;
	unsigned int remained = 0;
	unsigned int readbytes = 0xFF;
	unsigned int dataposition = 0;
	unsigned char *pRead = NULL;
	int rc = 0;
	int ret = 0;
	int i = 0;

	pRead = kzalloc(BUFFER_MAX, GFP_KERNEL);
	if (pRead == NULL) {
		input_err(true, &info->client->dev, "%s: pRead kzalloc failed\n", __func__);
		rc = 1;
		goto ErrorExit;
	}

	pFrameAddress[2] = type;
	totalbytes = info->SenseChannelLength * info->ForceChannelLength * 2;
	ret = fts_read_reg(info, &pFrameAddress[0], 3, pRead, pFrameAddress[3]);

	if (ret >= 0) {
		FrameAddress = pRead[1] + (pRead[2] << 8);

		start_addr = FrameAddress+info->SenseChannelLength * 2;
		end_addr = start_addr + totalbytes;
	} else {
		input_err(true, &info->client->dev, "%s: read failed rc = %d\n", __func__, ret);
		rc = 2;
		goto ErrorExit;
	}

#ifdef DEBUG_MSG
	input_info(true, &info->client->dev, "%s: FrameAddress = %X\n", __func__, FrameAddress);
	input_info(true, &info->client->dev, "%s: start_addr = %X, end_addr = %X\n", __func__, start_addr, end_addr);
#endif

	remained = totalbytes;
	for (writeAddr = start_addr; writeAddr < end_addr; writeAddr += READ_CHUNK_SIZE) {
		pFrameAddress[1] = (writeAddr >> 8) & 0xFF;
		pFrameAddress[2] = writeAddr & 0xFF;

		if (remained >= READ_CHUNK_SIZE)
			readbytes = READ_CHUNK_SIZE;
		else
			readbytes = remained;

		memset(pRead, 0x0, readbytes);

#ifdef DEBUG_MSG
		input_info(true, &info->client->dev, "%s: %02X%02X%02X readbytes=%d\n", __func__,
			   pFrameAddress[0], pFrameAddress[1],
			   pFrameAddress[2], readbytes);

#endif

		fts_read_reg(info, &pFrameAddress[0], 3, pRead, readbytes + 1);
		remained -= readbytes;

		for (i = 1; i < (readbytes+1); i += 2) {
			info->pFrame[dataposition++] =
			pRead[i] + (pRead[i + 1] << 8);
		}

	}
	kfree(pRead);

#ifdef DEBUG_MSG
	input_info(true, &info->client->dev,
		   "%s: writeAddr = %X, start_addr = %X, end_addr = %X\n", __func__,
		   writeAddr, start_addr, end_addr);
#endif

	switch (type) {
	case TYPE_RAW_DATA:
		input_info(true, &info->client->dev, "%s: [Raw Data : 0x%X%X]\n", __func__, pFrameAddress[0],
			FrameAddress);
		break;
	case TYPE_FILTERED_DATA:
		input_info(true, &info->client->dev, "%s: [Filtered Data : 0x%X%X]\n", __func__,
			pFrameAddress[0], FrameAddress);
		break;
	case TYPE_STRENGTH_DATA:
		input_info(true, &info->client->dev, "%s: [Strength Data : 0x%X%X]\n", __func__,
			pFrameAddress[0], FrameAddress);
		break;
	case TYPE_BASELINE_DATA:
		input_info(true, &info->client->dev, "%s: [Baseline Data : 0x%X%X]\n", __func__,
			pFrameAddress[0], FrameAddress);
		break;
	}
	fts_print_frame(info, min, max);

ErrorExit:
	return rc;
}

int fts_panel_ito_test(struct fts_ts_info *info)
{
	unsigned char cmd = READ_ONE_EVENT;
	unsigned char data[FTS_EVENT_SIZE];
	int retry = 0;
	int result = -1;

	fts_systemreset(info, 10);

	disable_irq(info->irq);
	fts_interrupt_set(info, INT_DISABLE);
	fts_command(info, FLUSHBUFFER);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	fts_command(info, 0xA7);    // ITO test command
	fts_delay(200);
	memset(data, 0x0, FTS_EVENT_SIZE);
	while (fts_read_reg(info, &cmd, 1, (unsigned char *)data, FTS_EVENT_SIZE)) {
		if ((data[0] == 0x0F) && (data[1] == 0x05)) {

			info->ito_test[0] = 0x0;
			info->ito_test[1] = 0x0;
			info->ito_test[2] = data[2];
			info->ito_test[3] = data[3];

			switch (data[2]) {
			case 0x00 :
				result = 0;
				break;
			case 0x01 :
				input_info(true, &info->client->dev, "%s: Force channel [%d] open\n", __func__,
					data[3]);
				break;
			case 0x02 :
				input_info(true, &info->client->dev, "%s: Sense channel [%d] open\n", __func__,
					data[3]);
				break;
			case 0x03 :
				input_info(true, &info->client->dev, "%s: Force channel [%d] short to GND\n", __func__,
					data[3]);
				break;
			case 0x04 :
				input_info(true, &info->client->dev, "%s: Sense channel [%d] short to GND\n", __func__,
					data[3]);
				break;
			case 0x05 :
				input_info(true, &info->client->dev, "%s: Force channel [%d] short to VDD\n", __func__,
					data[3]);
				break;
			case 0x06 :
				input_info(true, &info->client->dev, "%s: Sense channel [%d] short to VDD\n", __func__,
					data[3]);
				break;
			case 0x07 :
				input_info(true, &info->client->dev, "%s: Force channel [%d] short to force\n", __func__,
					data[3]);
				break;
			case 0x08 :
				input_info(true, &info->client->dev, "%s: Sennse channel [%d] short to sense\n", __func__,
					data[3]);
				break;
			default:
				break;
			}

			break;
		}

		if (retry++ > 30) {
			input_err(true, &info->client->dev, "%s: Time over - wait for result of ITO test\n", __func__);
			break;
		}
		fts_delay(10);
	}

	fts_systemreset(info, 10);

#ifdef FTS_SUPPORT_NOISE_PARAM
	fts_set_noise_param(info);
#endif

	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif
#ifdef FTS_SUPPORT_HOVER
	if (info->hover_enabled)
		fts_command(info, FTS_CMD_HOVER_ON);
#endif
	if (info->flip_enable) {
		fts_set_cover_type(info, true);
	}
#ifdef CONFIG_GLOVE_TOUCH
	else {
		if (info->glove_enabled)
			fts_command(info, FTS_CMD_GLOVE_ON);
	}
#endif
#ifdef FTS_SUPPORT_TA_MODE
	if (info->TA_Pluged)
		fts_command(info, FTS_CMD_CHARGER_PLUGGED);
#endif

	info->touch_count = 0;

	fts_interrupt_set(info, INT_ENABLE);
	enable_irq(info->irq);

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
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	disable_irq(info->irq);
	fts_get_version_info(info);
	enable_irq(info->irq);

	snprintf(buff, sizeof(buff), "ST%02X%04X",
			info->panel_revision,
			info->fw_main_version_of_ic);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
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
	unsigned char buff[16] = { 0 };
	unsigned char data[5] = { 0 };
	unsigned short finger_threshold = 0;
	int rc;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	sec->cmd_state = SEC_CMD_STATUS_RUNNING;
	rc = info->fts_get_sysinfo_data(info, FTS_SI_FINGER_THRESHOLD, 4, data);
	if (rc <= 0) {
		input_err(true, info->dev, "%s: Get threshold Read Fail!! [Data : %2X%2X]\n", __func__, data[0], data[1]);
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	finger_threshold = (unsigned short)(data[0] + (data[1] << 8));

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
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[16] = { 0 };

	memcpy(buff, info->firmware_name + 7, 9);

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_mis_cal_info(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char regAdd[3] = { 0 };
	unsigned char data[2] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	regAdd[0] = 0xC7;
	regAdd[1] = 0x05;

	ret = fts_write_reg(info, regAdd, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to write\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_delay(50);

	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = 0x67;

	ret = fts_read_reg(info, regAdd, 3, data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_err(true, &info->client->dev, "%s: %02X, %02X\n", __func__, data[0], data[1]);

	snprintf(buff, sizeof(buff), "%d", data[1]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_wet_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char regAdd[3] = { 0 };
	unsigned char data[2] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = 0x66;

	ret = fts_read_reg(info, regAdd, 3, data, 2);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to read\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_err(true, &info->client->dev, "%s: %02X, %02X\n", __func__, data[0], data[1]);

	snprintf(buff, sizeof(buff), "%d", data[1]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
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
	unsigned char data[6] = { 0 };

	sec_cmd_set_default_result(sec);
	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	rc = fts_systemreset(info, 10);
	if (rc != FTS_NOT_ERROR) {
		snprintf(buff, sizeof(buff), "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	} else {
		fts_reinit(info);

		fts_interrupt_set(info, INT_DISABLE);

		rc = info->fts_get_sysinfo_data(info, FTS_SI_CONFIG_CHECKSUM, 5, data);
		if (rc <= 0) {
			input_err(true, info->dev, "%s: Get checksum data Read Fail!! [Data : %2X%2X%2X%2X]\n", __func__, data[1], data[0], data[3], data[2]);
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			return;
		}

		fts_interrupt_set(info, INT_ENABLE);

		snprintf(buff, sizeof(buff), "%02X%02X%02X%02X", data[1], data[0], data[3], data[2]);
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	}
}

static void run_reference_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short min = 0x7FFF;
	short max = 0x8000;

	sec_cmd_set_default_result(sec);
	if (info->touch_stopped) {
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
	if (info->touch_stopped) {
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
	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	fts_read_frame(info, TYPE_FILTERED_DATA, &min, &max);
	snprintf(buff, sizeof(buff), "%d,%d", min, max);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_rawcap(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (info->touch_stopped) {
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
	if (info->touch_stopped) {
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
	char all_strbuff[(info->ForceChannelLength)*(info->SenseChannelLength)*5];
	int i, j;

	memset(all_strbuff,0,sizeof(char)*((info->ForceChannelLength)*(info->SenseChannelLength)*5));	//size 5  ex(1125,)

	sec_cmd_set_default_result(sec);
	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
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

	sec_cmd_set_cmd_result(sec, all_strbuff, strnlen(all_strbuff, sizeof(all_strbuff)));
	input_info(true, &info->client->dev, "%s: %ld (%ld)\n", __func__, strnlen(all_strbuff, sizeof(all_strbuff)),sizeof(all_strbuff));
}

static void get_delta(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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

static void get_pat_information(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[22] = { 0 };
	sec_cmd_set_default_result(sec);

#ifdef PAT_CONTROL
	fts_get_calibration_information(info);
#endif
	/* fixed tune version will be saved at excute autotune */
	snprintf(buff, sizeof(buff), "P%02XT%04X",
		info->cal_count,info->tune_fix_ver);

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

	info->external_factory = true;
	snprintf(buff, sizeof(buff), "OK");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

#ifdef FTS_SUPPORT_HOVER
void fts_read_self_frame(struct fts_ts_info *info, unsigned short oAddr)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[66] = {0, };
	short *data = 0;
	char temp[9] = {0, };
	char temp2[512] = {0, };
	int i;
	int rc;
	int retry=1;
	unsigned char regAdd[6] = {0xD0, 0x00, 0x00, 0xD0, 0x00, 0x00};

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (!info->hover_enabled) {
		input_err(true, &info->client->dev, "%s: [ERROR] Hover is disabled\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP Hover disabled");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	while (!info->hover_ready) {
		if (retry++ > 500) {
			input_err(true, &info->client->dev, "%s: Timeout - Abs Raw Data Ready Event\n",
					  __func__);
			break;
		}
		fts_delay(10);
	}

	regAdd[1] = (oAddr >> 8) & 0xff;
	regAdd[2] = oAddr & 0xff;
	rc = info->fts_read_reg(info, &regAdd[0], 3, (unsigned char *)&buff[0], 5);
	if (rc <= 0) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	input_info(true, &info->client->dev, "%s: Force Address : %02x%02x\n",
			__func__, buff[2], buff[1]);
	input_info(true, &info->client->dev, "%s: Sense Address : %02x%02x\n",
			__func__, buff[4], buff[3]);
	regAdd[1] = buff[4];
	regAdd[2] = buff[3];
	regAdd[4] = buff[2];
	regAdd[5] = buff[1];

	rc = info->fts_read_reg(info, &regAdd[0], 3,
					(unsigned char *)&buff[0],
					info->SenseChannelLength * 2 + 1);
	if (rc <= 0) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = (short *)&buff[1];

	memset(temp, 0x00, ARRAY_SIZE(temp));
	memset(temp2, 0x00, ARRAY_SIZE(temp2));

	for (i = 0; i < info->SenseChannelLength; i++) {
		input_info(true, &info->client->dev,
				"%s: Rx [%d] = %d\n", __func__, i, *data);
		snprintf(temp, sizeof(temp), "%d,", *data);
		strncat(temp2, temp, 9);
		data++;
	}

	rc = info->fts_read_reg(info, &regAdd[3], 3,
							(unsigned char *)&buff[0],
							info->ForceChannelLength * 2 + 1);
	if (rc <= 0) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	data = (short *)&buff[1];

	for (i = 0; i < info->ForceChannelLength; i++) {
		input_info(true, &info->client->dev,
				"%s: Tx [%d] = %d\n", __func__, i, *data);
		snprintf(temp, sizeof(temp), "%d,", *data);
		strncat(temp2, temp, 9);
		data++;
	}

	sec_cmd_set_cmd_result(sec, temp2, strnlen(temp2, sizeof(temp2)));

	sec->cmd_state = SEC_CMD_STATUS_OK;
}

static void run_abscap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	fts_read_self_frame(info, 0x000E);
}

static void run_absdelta_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	fts_read_self_frame(info, 0x0012);
}
#endif

#define FTS_WATER_SELF_RAW_ADDR	0x1A

static void fts_read_ix_data(struct fts_ts_info *info, bool allnode)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	unsigned short max_tx_ix_sum = 0;
	unsigned short min_tx_ix_sum = 0xFFFF;

	unsigned short max_rx_ix_sum = 0;
	unsigned short min_rx_ix_sum = 0xFFFF;

	unsigned char tx_ix2[info->ForceChannelLength + 4];
	unsigned char rx_ix2[info->SenseChannelLength + 4];

	unsigned char regAdd[FTS_EVENT_SIZE];
	unsigned short tx_ix1 = 0, rx_ix1 = 0;

	unsigned short force_ix_data[info->ForceChannelLength * 2 + 1];
	unsigned short sense_ix_data[info->SenseChannelLength * 2 + 1];
	int buff_size,j;
	char *mbuff = NULL;
	int num,n,a,fzero;
	char cnum;
	int i = 0;
	int comp_header_addr, comp_start_tx_addr, comp_start_rx_addr;
	unsigned int rx_num, tx_num;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
		       __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	disable_irq(info->irq);
	fts_interrupt_set(info, INT_DISABLE);

	fts_command(info, SENSEOFF);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		fts_command(info, FTS_CMD_KEY_SENSE_OFF);
	}
#endif

	fts_command(info, FLUSHBUFFER);                 // Clear FIFO
	fts_delay(50);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	/* Request compensation data */
	regAdd[0] = 0xB8;
	regAdd[1] = 0x20;		// SELF IX
	regAdd[2] = 0x00;
	fts_write_reg(info, &regAdd[0], 3);
	fts_fw_wait_for_specific_event(info, EVENTID_STATUS_REQUEST_COMP, 0x20, 0x00);

	/* Read an address of compensation data */
	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = FTS_SI_COMPENSATION_OFFSET_ADDR;
	fts_read_reg(info, regAdd, 3, &buff[0], 4);
	comp_header_addr = buff[1] + (buff[2] << 8);

	/* Read header of compensation area */
	regAdd[0] = 0xD0;
	regAdd[1] = (comp_header_addr >> 8) & 0xFF;
	regAdd[2] = comp_header_addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &buff[0], 16 + 1);
	tx_num = buff[5];
	rx_num = buff[6];

	tx_ix1 = (short) buff[10] * FTS_SEC_IX1_TX_MULTIPLIER;		// Self TX Ix1
	rx_ix1 = (short) buff[11] * FTS_SEC_IX1_RX_MULTIPLIER;		// Self RX Ix1

	comp_start_tx_addr = comp_header_addr + 0x10;
	comp_start_rx_addr = comp_start_tx_addr + tx_num;

	memset(tx_ix2, 0x0, tx_num);
	memset(rx_ix2, 0x0, rx_num);

	/* Read Self TX Ix2 */
	regAdd[0] = 0xD0;
	regAdd[1] = (comp_start_tx_addr >> 8) & 0xFF;
	regAdd[2] = comp_start_tx_addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &tx_ix2[0], tx_num + 1);

	/* Read Self RX Ix2 */
	regAdd[0] = 0xD0;
	regAdd[1] = (comp_start_rx_addr >> 8) & 0xFF;
	regAdd[2] = comp_start_rx_addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &rx_ix2[0], rx_num + 1);

	for(i = 0; i < info->ForceChannelLength; i++) {
		force_ix_data[i] = tx_ix1 + tx_ix2[i + 1];
		if(max_tx_ix_sum < force_ix_data[i])
			max_tx_ix_sum = force_ix_data[i];
		if(min_tx_ix_sum > force_ix_data[i])
			min_tx_ix_sum = force_ix_data[i];
	}

	for(i = 0; i < info->SenseChannelLength; i++) {
		sense_ix_data[i] = rx_ix1 + rx_ix2[i + 1];
		if(max_rx_ix_sum < sense_ix_data[i])
			max_rx_ix_sum = sense_ix_data[i];
		if(min_rx_ix_sum > sense_ix_data[i])
			min_rx_ix_sum = sense_ix_data[i];
	}

	input_info(true, &info->client->dev, "%s: MIN_TX_IX_SUM : %d MAX_TX_IX_SUM : %d\n",
				__func__, min_tx_ix_sum, max_tx_ix_sum );
	input_info(true, &info->client->dev, "%s: MIN_RX_IX_SUM : %d MAX_RX_IX_SUM : %d\n",
				__func__, min_rx_ix_sum, max_rx_ix_sum );

	fts_systemreset(info, 10);

	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
	fts_fw_wait_for_event (info, STATUS_EVENT_FORCE_CAL_DONE_D3);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif

	enable_irq(info->irq);
	fts_interrupt_set(info, INT_ENABLE);

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
	} else {
		if (allnode == true) {
		   snprintf(buff, sizeof(buff), "%s", "kzalloc failed");
		   sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "%d,%d,%d,%d", min_tx_ix_sum, max_tx_ix_sum, min_rx_ix_sum, max_rx_ix_sum);
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	}
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
	fts_read_ix_data(info, true);
}

static void fts_read_self_raw_frame(struct fts_ts_info *info, unsigned short oAddr, bool allnode)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char D0_offset = 1;
	unsigned char regAdd[3] = {0xD0, 0x00, 0x00};
	unsigned char ReadData[(info->ForceChannelLength + info->SenseChannelLength) * 2 + 1];
	unsigned short self_force_raw_data[info->ForceChannelLength * 2 + 1];
	unsigned short self_sense_raw_data[info->SenseChannelLength * 2 + 1];
	unsigned int FrameAddress = 0;
	unsigned char count=0;
	int buff_size,i,j;
	char *mbuff = NULL;
	int num,n,a,fzero;
	char cnum;
	unsigned short min_tx_self_raw_data = 0xFFFF;
	unsigned short max_tx_self_raw_data = 0;
	unsigned short min_rx_self_raw_data = 0xFFFF;
	unsigned short max_rx_self_raw_data = 0;
	unsigned char cmd[4] = {0xC7, 0x00, FTS_CFG_APWR, 0x00}; // Don't enter to IDLE

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	fts_systemreset(info, 60);

	// Don't enter to IDLE mode
	fts_write_reg(info, &cmd[0], 4);
	fts_delay(20);

	fts_command(info, FLUSHBUFFER);                 // Clear FIFO
	fts_delay(20);

	// Auto-Tune
	fts_command(info, CX_TUNNING);
	msleep(300);
	fts_fw_wait_for_event_D3(info, STATUS_EVENT_MUTUAL_AUTOTUNE_DONE, 0x00);

	fts_command(info, SELF_AUTO_TUNE);
	msleep(300);
	fts_fw_wait_for_event_D3(info, STATUS_EVENT_SELF_AUTOTUNE_DONE_D3, 0x00);

	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
	fts_delay(100);

	fts_command(info, SENSEOFF);
	fts_delay(100);

	disable_irq(info->irq);
	fts_interrupt_set(info, INT_DISABLE);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		fts_command(info, FTS_CMD_KEY_SENSE_OFF);
	}
#endif

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	regAdd[1] = 0x00;
	regAdd[2] = oAddr;
	fts_read_reg(info, regAdd, 3, &ReadData[0], 4);

	FrameAddress = ReadData[D0_offset] + (ReadData[D0_offset + 1] << 8);           // D1 : DOFFSET = 0, D2 : DOFFSET : 1

	regAdd[1] = (FrameAddress >> 8) & 0xFF;
	regAdd[2] = FrameAddress & 0xFF;

	fts_read_reg(info, regAdd, 3, &ReadData[0], info->ForceChannelLength * 2 + 1);

	for(count = 0; count < info->ForceChannelLength; count++) {
		self_force_raw_data[count] = ReadData[count*2+D0_offset] + (ReadData[count*2+D0_offset+1]<<8);

		if(max_tx_self_raw_data < self_force_raw_data[count])
			max_tx_self_raw_data = self_force_raw_data[count];
		if(min_tx_self_raw_data > self_force_raw_data[count])
			min_tx_self_raw_data = self_force_raw_data[count];
	}

	regAdd[1] = 0x00;
	regAdd[2] = oAddr + 2;
	fts_read_reg(info, regAdd, 3, &ReadData[0], 4);

	FrameAddress = ReadData[D0_offset] + (ReadData[D0_offset + 1] << 8);           // D1 : DOFFSET = 0, D2 : DOFFSET : 1

	regAdd[1] = (FrameAddress >> 8) & 0xFF;
	regAdd[2] = FrameAddress & 0xFF;

	fts_read_reg(info, regAdd, 3, &ReadData[0], info->SenseChannelLength * 2 + 1);

	for(count = 0; count < info->SenseChannelLength; count++) {
		self_sense_raw_data[count] = ReadData[count*2+D0_offset] + (ReadData[count*2+D0_offset+1]<<8);

		if(max_rx_self_raw_data < self_sense_raw_data[count])
			max_rx_self_raw_data = self_sense_raw_data[count];
		if(min_rx_self_raw_data > self_sense_raw_data[count])
			min_rx_self_raw_data = self_sense_raw_data[count];
	}

	input_info(true, &info->client->dev, "%s: MIN_TX_SELF_RAW: %d MAX_TX_SELF_RAW : %d\n",
				__func__, min_tx_self_raw_data, max_tx_self_raw_data );
	input_info(true, &info->client->dev, "%s: MIN_RX_SELF_RAW : %d MIN_RX_SELF_RAW : %d\n",
				__func__, min_rx_self_raw_data, max_rx_self_raw_data );


	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
	fts_delay(50);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif

	enable_irq(info->irq);
	fts_interrupt_set(info, INT_ENABLE);

	if (allnode == true) {
		buff_size = (info->ForceChannelLength + info->SenseChannelLength + 2) * 7;
		mbuff = kzalloc(buff_size, GFP_KERNEL);
	}
	if (mbuff != NULL) {
		char *pBuf = mbuff;
		for (i = 0; i < info->ForceChannelLength; i++) {
			num =  self_force_raw_data[i];
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
			*pBuf++ = ',';
			input_info(true, &info->client->dev, "Force[%d] %d\n", i, self_force_raw_data[i]);
		}
		for (i = 0; i < info->SenseChannelLength; i++) {
			num =  self_sense_raw_data[i];
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
			input_info(true, &info->client->dev, "Sense[%d] %d\n", i, self_sense_raw_data[i]);
		}

		sec_cmd_set_cmd_result(sec, mbuff, buff_size);
		sec->cmd_state = SEC_CMD_STATUS_OK;
		kfree(mbuff);
	} else {
		if (allnode == true) {
		   snprintf(buff, sizeof(buff), "%s", "kzalloc failed");
		   sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "%d,%d,%d,%d", min_tx_self_raw_data, max_tx_self_raw_data, min_rx_self_raw_data, max_rx_self_raw_data);
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	}
}

static void run_self_raw_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	fts_read_self_raw_frame(info, FTS_WATER_SELF_RAW_ADDR,false);
}

static void run_self_raw_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	fts_read_self_raw_frame(info, FTS_WATER_SELF_RAW_ADDR,true);
}

static void run_trx_short_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	sec_cmd_set_default_result(sec);
	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_panel_ito_test(info);
	if (ret == 0)
		snprintf(buff, sizeof(buff), "%s", "OK");
	else
		snprintf(buff, sizeof(buff), "%s", "FAIL");

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}


#define FTS_MAX_TX_LENGTH		44
#define FTS_MAX_RX_LENGTH		64

#define FTS_CX2_READ_LENGTH		4
#define FTS_CX2_ADDR_OFFSET		3
#define FTS_CX2_TX_START		0

static void get_cx_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);
	if (info->touch_stopped) {
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

static void run_cx_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char ReadData[info->ForceChannelLength][info->SenseChannelLength + FTS_CX2_READ_LENGTH];
	unsigned char regAdd[8];
	unsigned int addr, rx_num, tx_num;
	int i, j;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };

	int comp_header_addr, comp_start_addr;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	pStr = kzalloc(4 * (info->SenseChannelLength + 1), GFP_KERNEL);
	if (pStr == NULL) {
		input_err(true, &info->client->dev, "%s: pStr kzalloc failed\n", __func__);
		return;
	}

	input_info(true, &info->client->dev, "%s: start\n", __func__);

	disable_irq(info->irq);
	fts_interrupt_set(info, INT_DISABLE);

	fts_command(info, SENSEOFF);
	fts_delay(50);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		fts_command(info, FTS_CMD_KEY_SENSE_OFF); // Key Sensor OFF
	}
#endif
	fts_command(info, FLUSHBUFFER);
	fts_delay(50);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	/* Request compensation data */
	regAdd[0] = 0xB8;
	regAdd[1] = 0x04;		// MUTUAL CX (LPA)
	regAdd[2] = 0x00;
	fts_write_reg(info, &regAdd[0], 3);
	fts_fw_wait_for_specific_event(info, EVENTID_STATUS_REQUEST_COMP, 0x04, 0x00);

	/* Read an address of compensation data */
	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = FTS_SI_COMPENSATION_OFFSET_ADDR;
	fts_read_reg(info, regAdd, 3, &buff[0], 4);
	comp_header_addr = buff[1] + (buff[2] << 8);

	/* Read header of compensation area */
	regAdd[0] = 0xD0;
	regAdd[1] = (comp_header_addr >> 8) & 0xFF;
	regAdd[2] = comp_header_addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &buff[0], 16 + 1);
	tx_num = buff[5];
	rx_num = buff[6];
	comp_start_addr = comp_header_addr + 0x10;

	/* Read compensation data */
	for (j = 0; j < tx_num; j++) {
		memset(&ReadData[j], 0x0, (rx_num + 1));
		memset(pStr, 0x0, 4 * (rx_num + 1));
		snprintf(pTmp, sizeof(pTmp), "Tx%02d | ", j);
		strncat(pStr, pTmp, 4 * rx_num);

		addr = comp_start_addr + (rx_num * j);
		regAdd[0] = 0xD0;
		regAdd[1] = (addr >> 8) & 0xFF;
		regAdd[2] = addr & 0xFF;
		fts_read_reg(info, regAdd, 3, &ReadData[j][0], rx_num + 1);
		for (i = 0; i < rx_num; i++) {
			snprintf(pTmp, sizeof(pTmp), "%3d", ReadData[j][i + 1]);
			strncat(pStr, pTmp, 4 * rx_num);
		}
		input_info(true, &info->client->dev, "%s\n", pStr);
	}

	if (info->cx_data) {
		for (j = 0; j < tx_num; j++) {
			for(i = 0; i < rx_num; i++)
				info->cx_data[(j * rx_num) + i] = ReadData[j][i + 1];
		}
	}

	kfree(pStr);

	snprintf(buff, sizeof(buff), "%s", "OK");
	enable_irq(info->irq);
	fts_interrupt_set(info, INT_ENABLE);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
	}
#endif
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_cx_all_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char ReadData[info->ForceChannelLength][info->SenseChannelLength + FTS_CX2_READ_LENGTH];
	unsigned char regAdd[8];
	unsigned int addr, rx_num, tx_num;
	int i, j;
	unsigned char *pStr = NULL;
	unsigned char pTmp[16] = { 0 };
	char all_strbuff[(info->ForceChannelLength)*(info->SenseChannelLength)*3];
	int comp_header_addr, comp_start_addr;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	input_info(true, &info->client->dev, "%s: start\n", __func__);

	pStr = kzalloc(4 * (info->SenseChannelLength + 1), GFP_KERNEL);
	if (pStr == NULL) {
		input_err(true, &info->client->dev, "%s: pStr kzalloc failed\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}

	fts_command(info, SENSEOFF);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		fts_command(info, FTS_CMD_KEY_SENSE_OFF); // Key Sensor OFF
	}
#endif
	disable_irq(info->irq);
	fts_command(info, FLUSHBUFFER);
	fts_delay(50);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	tx_num = info->ForceChannelLength;
	rx_num = info->SenseChannelLength;

	memset(all_strbuff, 0, sizeof(char) * (tx_num*rx_num*3));	//size 3  ex(45,)

	/* Request compensation data */
	regAdd[0] = 0xB8;
	regAdd[1] = 0x04;		// MUTUAL CX (LPA)
	regAdd[2] = 0x00;
	fts_write_reg(info, &regAdd[0], 3);
	fts_fw_wait_for_specific_event(info, EVENTID_STATUS_REQUEST_COMP, 0x04, 0x00);

	/* Read an address of compensation data */
	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = FTS_SI_COMPENSATION_OFFSET_ADDR;
	fts_read_reg(info, regAdd, 3, &buff[0], 4);
	comp_header_addr = buff[1] + (buff[2] << 8);

	/* Read header of compensation area */
	regAdd[0] = 0xD0;
	regAdd[1] = (comp_header_addr >> 8) & 0xFF;
	regAdd[2] = comp_header_addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &buff[0], 16 + 1);
	tx_num = buff[5];
	rx_num = buff[6];
	comp_start_addr = comp_header_addr + 0x10;

	/* Read compensation data */
	for (j = 0; j < tx_num; j++) {
		memset(&ReadData[j], 0x0, (rx_num + 1));
		memset(pStr, 0x0, 4 * (rx_num + 1));
		snprintf(pTmp, sizeof(pTmp), "Tx%02d | ", j);
		strncat(pStr, pTmp, 4 * rx_num);

		addr = comp_start_addr + (rx_num * j);
		regAdd[0] = 0xD0;
		regAdd[1] = (addr >> 8) & 0xFF;
		regAdd[2] = addr & 0xFF;
		fts_read_reg(info, regAdd, 3, &ReadData[j][0], rx_num + 1);
		for (i = 0; i < rx_num; i++) {
			snprintf(pTmp, sizeof(pTmp), "%3d", ReadData[j][i + 1]);
			strncat(pStr, pTmp, 4 * rx_num);
		}
		input_info(true, &info->client->dev, "%s\n", pStr);
	}

	if (info->cx_data) {
		for (j = 0; j < tx_num; j++) {
			for(i = 0; i < rx_num; i++){
				info->cx_data[(j * rx_num) + i] = ReadData[j][i + 1];
				snprintf(buff, sizeof(buff), "%d,", ReadData[j][i + 1]);
				strncat(all_strbuff, buff, sizeof(buff));
			}
		}
	}

	kfree(pStr);

	enable_irq(info->irq);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
	}
#endif
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, all_strbuff, strnlen(all_strbuff, sizeof(all_strbuff)));
	input_info(true, &info->client->dev, "%s: %ld (%ld)\n", __func__, strnlen(all_strbuff, sizeof(all_strbuff)),sizeof(all_strbuff));
}

static void fts_read_wtr_cx_data(struct fts_ts_info *info, bool allnode)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[512] = { 0 };
	unsigned char regAdd[8];
	unsigned char *result_tx = NULL;
	unsigned char *result_rx = NULL;
	char temp[8];
	unsigned int ii;
	unsigned int tx_num, rx_num;
        unsigned int comp_header_addr, comp_start_tx_addr, comp_start_rx_addr;
        int i;

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	input_info(true, &info->client->dev, "%s: ++\n", __func__);

	disable_irq(info->irq);
	fts_interrupt_set(info, INT_DISABLE);

	fts_command(info, SENSEOFF);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		fts_command(info, FTS_CMD_KEY_SENSE_OFF);
	}
#endif
	fts_command(info, FLUSHBUFFER);
	fts_delay(50);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	result_tx = kzalloc(info->ForceChannelLength + 10, GFP_KERNEL);
	result_rx = kzalloc(info->SenseChannelLength + 10, GFP_KERNEL);
	if (!result_tx || !result_rx) {
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);

		snprintf(buff, sizeof(buff), "%s", "FAIL");
		goto err_alloc_mem;
	}

	/* Request compensation data */
	regAdd[0] = 0xB8;
	regAdd[1] = 0x20;		// SELF IX
	regAdd[2] = 0x00;
	fts_write_reg(info, &regAdd[0], 3);
	fts_fw_wait_for_specific_event(info, EVENTID_STATUS_REQUEST_COMP, 0x20, 0x00);

	/* Read an address of compensation data */
	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = FTS_SI_COMPENSATION_OFFSET_ADDR;
	fts_read_reg(info, regAdd, 3, &buff[0], 4);
	comp_header_addr = buff[1] + (buff[2] << 8);

	/* Read header of compensation area */
	regAdd[0] = 0xD0;
	regAdd[1] = (comp_header_addr >> 8) & 0xFF;
	regAdd[2] = comp_header_addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &buff[0], 16 + 1);
	tx_num = buff[5];
	rx_num = buff[6];

	comp_start_tx_addr = comp_header_addr + 0x10 + tx_num + rx_num;
	comp_start_rx_addr = comp_start_tx_addr + tx_num;

	memset(result_tx, 0x0, tx_num);
	memset(result_rx, 0x0, rx_num);

	memset(buff, 0x0, 512);
	/* Read Self TX Cx2 */
	regAdd[0] = 0xD0;
	regAdd[1] = (comp_start_tx_addr >> 8) & 0xFF;
	regAdd[2] = comp_start_tx_addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &buff[0], tx_num + 1);

	for(i = 0; i < tx_num; i++) {
		result_tx[i] = buff[i + 1];
	}

	memset(buff, 0x0, 512);
	/* Read Self RX Ix2 */
	regAdd[0] = 0xD0;
	regAdd[1] = (comp_start_rx_addr >> 8) & 0xFF;
	regAdd[2] = comp_start_rx_addr & 0xFF;
	fts_read_reg(info, regAdd, 3, &buff[0], rx_num + 1);

	for(i = 0; i < rx_num; i++) {
		result_rx[i] = buff[i + 1];
	}

	if (allnode) {
		char *data = 0;

		data = result_tx;
		for (ii = 0; ii < info->ForceChannelLength; ii++) {
			input_info(true, &info->client->dev,
					"%s: Tx[%d] = %d\n", __func__, ii, *data);
			snprintf(temp, sizeof(temp), "%d,", *data);
			strncat(buff, temp, 9);
			data++;
		}

		data = result_rx;
		for (ii = 0; ii < info->SenseChannelLength; ii++) {
			input_info(true, &info->client->dev,
					"%s: Rx[%d] = %d\n", __func__, ii, *data);
			snprintf(temp, sizeof(temp), "%d,", *data);
			strncat(buff, temp, 9);
			data++;
		}
	} else {
		unsigned char min_tx = 255, max_tx = 0, min_rx = 255, max_rx = 0;

		for (ii = 0; ii < info->ForceChannelLength; ii++) {
			input_info(true, &info->client->dev,
					"%s: Tx[%d] = %d\n", __func__, ii, result_tx[ii]);

			min_tx = min(min_tx, result_tx[ii]);
			max_tx = max(max_tx, result_tx[ii]);
		}

		for (ii = 0; ii < info->SenseChannelLength; ii++) {
			input_info(true, &info->client->dev,
					"%s: Rx[%d] = %d\n", __func__, ii, result_rx[ii]);
			min_rx = min(min_rx, result_rx[ii]);
			max_rx = max(max_rx, result_rx[ii]);
		}

		snprintf(buff, sizeof(buff), "%d,%d,%d,%d", min_tx, max_tx, min_rx, max_rx);

	}

err_alloc_mem:
	if (result_tx)
		kfree(result_tx);
	if (result_rx)
		kfree(result_rx);

	enable_irq(info->irq);
	fts_interrupt_set(info, INT_ENABLE);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
	}
#endif
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);

}

static void run_wtr_cx_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	fts_read_wtr_cx_data(info, false);
}

static void run_wtr_cx_data_read_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);

	sec_cmd_set_default_result(sec);
	fts_read_wtr_cx_data(info, true);
}

#ifdef FTS_SUPPORT_PRESSURE_SENSOR
int fts_read_pressure_data(struct fts_ts_info *info)
{
	unsigned char pressure_reg[3] = {0, };
	unsigned char buf[32] = {0, };
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
	} else {
		pressure_reg[1] = buf[2];
		pressure_reg[2] = buf[1];
	}

	rtn = fts_read_reg(info, pressure_reg, 3, &buf[0], PRESSURE_SENSOR_COUNT * 2 + 1);
	if (rtn <= 0) {
		input_err(true, &info->client->dev, "%s: Pressure read failed rc = %d\n", __func__, rtn);
		return rtn;
	} else {
		for (i = 0; i < PRESSURE_SENSOR_COUNT; i++) {
			value[i] = buf[i * 2 + 1] + (buf[i * 2 + 1 + 1] << 8);
			pressure_sensor_value += value[i];
		}
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
	unsigned char key_cx2_data[2], key_cx1_data, total_cx_data[USE_KEY_NUM];
	unsigned char ReadData[USE_KEY_NUM * FTS_CX2_READ_LENGTH];
	unsigned char regAdd[8];
	unsigned int addr;
	int /*tx_num, */rx_num, DOFFSET = 1;
	int comp_start_addr, comp_header_addr;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
		            __func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	disable_irq(info->irq);

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
	snprintf(buff, sizeof(buff), "%d,%d,%d,%d", key_cx2_data[0], key_cx2_data[1], total_cx_data[0], total_cx_data[1]);

	enable_irq(info->irq);
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
	char buff[SEC_CMD_STR_LEN] = {0};
#ifdef FTS_SUPPORT_STRINGLIB
	unsigned short addr = FTS_CMD_STRING_ACCESS;
	int ret;
#endif

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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

	disable_irq(info->irq);
	fts_interrupt_set(info, INT_DISABLE);
	fts_command(info, FTS_CMD_PRESSURE_SENSE_OFF);

	info->fts_systemreset(info, 30);

	fts_command(info, PRESSURE_AUTO_TUNE);
	fts_delay(300);
	fts_fw_wait_for_event_D3(info, STATUS_EVENT_SELF_AUTOTUNE_DONE_D3, 0x00);

	fts_command(info, FTS_CMD_SAVE_CX_TUNING);
	fts_delay(230);
	fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);

	info->fts_systemreset(info, 30);

	fts_command(info, SENSEON);
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
	fts_fw_wait_for_event (info, STATUS_EVENT_FORCE_CAL_DONE_D3);
	fts_interrupt_set(info, INT_ENABLE);
	enable_irq(info->irq);

	fts_get_factory_debug_information(info);

	info->pressure_cal_base++;

	if (info->pressure_cal_base > 0xFE)
		info->pressure_cal_base = 0xFE;

	fts_set_factory_debug_information(info, info->pressure_cal_base, info->pressure_cal_delta, info->nv_crc_fail_count);

#ifdef FTS_SUPPORT_STRINGLIB
	ret = info->fts_write_to_string(info, &addr, &info->lowpower_flag, sizeof(info->lowpower_flag));
	if (ret < 0)
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
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
	unsigned char regAdd[4] = {0xC7, 0x03, 0x00, 0x00};
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
	unsigned char regAddr[3] = {0xD0, 0x00, FTS_SI_PRESSURE_STRENGTH_ADDR};
	unsigned char data[PRESSURE_SENSOR_COUNT * 2 + 1];
	int ret, i;
	short value[3] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
	unsigned char regAddr[3] = {0xD0, 0x00, 0x00};
	unsigned char data[PRESSURE_SENSOR_COUNT * 2 + 1];
	int ret, i;
	short value[3] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
	unsigned char regAdd[3] = {0xB8, 0x00, 0x02};
	unsigned char data[32];
	int ret, i;
	int value[3] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
					__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	disable_irq(info->irq);
	fts_interrupt_set(info, INT_DISABLE);
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

	enable_irq(info->irq);
	fts_interrupt_set(info, INT_ENABLE);
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
	unsigned char strength[12] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
	fts_interrupt_set(info, INT_DISABLE);

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
		fts_interrupt_set(info, INT_ENABLE);
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
	fts_interrupt_set(info, INT_ENABLE);

	fts_get_factory_debug_information(info);

	info->pressure_cal_delta++;

	if (info->pressure_cal_delta > 0xFE)
		info->pressure_cal_delta = 0xFE;

	fts_set_factory_debug_information(info, info->pressure_cal_base, info->pressure_cal_delta, info->nv_crc_fail_count);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_pressure_rawdata(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char rawdata[12] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
	fts_interrupt_set(info, INT_DISABLE);

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
		fts_interrupt_set(info, INT_ENABLE);
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
	fts_interrupt_set(info, INT_ENABLE);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_pressure_data_index(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char data[12] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
	fts_interrupt_set(info, INT_DISABLE);

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
		fts_interrupt_set(info, INT_ENABLE);
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
	fts_interrupt_set(info, INT_ENABLE);

	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_pressure_strength(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char *data = NULL;
	short strength[3] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
	unsigned char *data = NULL;
	short rawdata[3] = { 0 };
	int ret, index;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
	unsigned char *data = NULL;
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
		memset(&data[0], 0x00, 1);
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
	unsigned char data[12] = { 0 };
	int ret;
	int ii = 0;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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
	fts_interrupt_set(info, INT_DISABLE);

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
		if (ret <= 0) {
			goto err_write_reg;
		}
	}

	input_info(true, &info->client->dev, "%s: clear strength\n", __func__);

	for (ii = 1; ii < 4; ii++) {
		data[0] = 0xC7;
		data[1] = 0x03;
		data[2] = 0x03;
		data[3] = ii;

		ret = fts_write_reg(info, data, 12);
		if (ret <= 0) {
			goto err_write_reg;
		}
	}

	input_info(true, &info->client->dev, "%s: clear rawdata\n", __func__);

	data[0] = 0xC7;
	data[1] = 0x03;
	data[2] = 0x01;
	data[3] = 0;

	ret = fts_write_reg(info, data, 4);
	if (ret <= 0) {
		goto err_write_reg;
	}

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
	fts_interrupt_set(info, INT_ENABLE);

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
	fts_interrupt_set(info, INT_ENABLE);
	return;
}

static void get_pressure_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char data[3] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
					__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = info->fts_get_sysinfo_data(info, FTS_SI_PRESSURE_THRESHOLD, 3, data);
	if (ret <= 0)
		input_err(true, info->dev, "%s: failed to read pressure threshold\n", __func__);

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
	unsigned char data[1] = { 0 };
	unsigned short addr;
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
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

	addr = FTS_CMD_STRING_ACCESS + FTS_CMD_OFFSET_PRESSURE_LEVEL;
	data[0] = sec->cmd_param[0];

	ret = info->fts_write_to_string(info, &addr, data, 1);
	if (ret <= 0)
		goto out_set_user_level;

	ret = info->fts_read_from_string(info, &addr, data, 1);
	if (ret <= 0)
		goto out_set_user_level;

	input_info(true, &info->client->dev, "%s: set user level: %d\n", __func__, data[0]);

	info->pressure_user_level = data[0];

	fts_delay(20);

	addr = FTS_CMD_STRING_ACCESS + FTS_CMD_OFFSET_PRESSURE_THD_HIGH;
	data[0] = 0;
	ret = info->fts_read_from_string(info, &addr, data, 1);
	if (ret <= 0)
		goto out_set_user_level;

	input_info(true, &info->client->dev, "%s: HIGH THD: %d\n", __func__, data[0]);

	addr = FTS_CMD_STRING_ACCESS + FTS_CMD_OFFSET_PRESSURE_THD_LOW;
	data[0] = 0;
	ret = info->fts_read_from_string(info, &addr, data, 1);
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
	unsigned char data[1] = { 0 };
	unsigned short addr;
	int ret;

	sec_cmd_set_default_result(sec);

	data[0] = sec->cmd_param[0];
	addr = FTS_CMD_STRING_ACCESS + FTS_CMD_OFFSET_PRESSURE_LEVEL;

	ret = info->fts_write_to_string(info, &addr, data, 1);
	if (ret <= 0)
		goto out_get_user_level;

	ret = info->fts_read_from_string(info, &addr, data, 1);
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

int fts_get_tsp_test_result(struct fts_ts_info *info)
{
	unsigned char *data = NULL;
	int ret;

	data = kzalloc(nvm_data[FACTORY_TEST_RESULT].length, GFP_KERNEL);
	if (!data) {
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);
		return -ENOMEM;
	}

	ret = get_nvm_data(info, FACTORY_TEST_RESULT, data);
	if (ret <= 0) {
		input_err(true, &info->client->dev,
			"%s: get failed. ret: %d\n", __func__, ret);
		goto err_read;
	}

	if (data[0] == 0xFF)
		data[0] = 0;
	if (data[1] == 0xFF)
		data[1] = 0;

	info->test_result.data[0] = data[0];
	info->disassemble_count = data[1];

err_read:
	kfree(data);
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

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_get_tsp_test_result(info);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to get result\n",
			__func__);
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
static int fts_set_tsp_test_result(struct fts_ts_info *info)
{
	unsigned char *data = NULL;
	int ret;

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

	data = kzalloc(nvm_data[FACTORY_TEST_RESULT].length, GFP_KERNEL);
	if (!data) {
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);
		return -ENOMEM;
	}

	data[0] = info->test_result.data[0];
	data[1] = info->disassemble_count;

	ret = set_nvm_data(info, FACTORY_TEST_RESULT, data);
	if (ret <= 0)
		input_err(true, &info->client->dev,
			"%s: set failed. ret: %d\n", __func__, ret);

	kfree(data);
	return ret;
}

static void set_tsp_test_result(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_get_tsp_test_result(info);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to get_tsp_test_result\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}
	sec->cmd_state = SEC_CMD_STATUS_RUNNING;

	if (info->test_result.data[0] == 0xFF) {
		input_info(true, &info->client->dev,
			"%s: clear factory_result as zero\n",
			__func__);
		info->test_result.data[0] = 0;
	}

	if (sec->cmd_param[0] == TEST_OCTA_ASSAY) {
		info->test_result.assy_result = sec->cmd_param[1];
		if (info->test_result.assy_count < 3)
			info->test_result.assy_count++;

	} else if (sec->cmd_param[0] == TEST_OCTA_MODULE) {
		info->test_result.module_result = sec->cmd_param[1];
		if (info->test_result.module_count < 3)
			info->test_result.module_count++;
	}

	ret = fts_set_tsp_test_result(info);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void increase_disassemble_count(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_get_tsp_test_result(info);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: [ERROR] failed to get_tsp_test_result\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		return;
	}
	sec->cmd_state = SEC_CMD_STATUS_RUNNING;

	if (info->disassemble_count < 0xFE)
		info->disassemble_count++;

	ret = fts_set_tsp_test_result(info);
	if (ret < 0) {
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

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	ret = fts_get_tsp_test_result(info);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to get result\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {

		snprintf(buff, sizeof(buff), "%d", info->disassemble_count);

		sec_cmd_set_cmd_result(sec, buff, strlen(buff));
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
}

int fts_get_tsp_flash_data(struct fts_ts_info *info, int offset, unsigned char cnt, unsigned char* buf)
{
	unsigned char regAdd[3] = {0xB8, 0x00, 0x08};
  	unsigned char data[255] = { 0 };
	unsigned short offset_addr;
	int ret;

	ret = fts_write_reg(info, &regAdd[0], 3);
	if (ret <= 0) {
		input_err(true, &info->client->dev,
			"%s: failed. ret: %d\n", __func__, ret);
		return -EINVAL;
	}

	ret = info->fts_get_sysinfo_data(info, FTS_SI_COMPENSATION_OFFSET_ADDR, 4, data);
	if (ret < 0) {
		input_err(true, &info->client->dev,
			"%s: failed. ret: %d\n", __func__, ret);
		return -EINVAL;
	}

	offset_addr = data[0] + (data[1] << 8);

	regAdd[0] = 0xD0;
	regAdd[1] = (offset_addr >> 8) & 0xFF;
	regAdd[2] = (offset_addr & 0xFF) + offset;

	ret = fts_read_reg(info, &regAdd[0], 3, (unsigned char *)buf, cnt + 1);
	if (ret <= 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n",
			__func__, ret);
		return ret;
	}
	return 0;
}

#ifdef PAT_CONTROL
int fts_set_calibration_information(struct fts_ts_info *info, unsigned char count, unsigned short version)
{
	unsigned char *data = NULL;
	int ret;

	data = kzalloc(nvm_data[CALIBRATION_INFOMATION].length, GFP_KERNEL);
	if (!data) {
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);
		return -ENOMEM;
	}

	data[0] = count;
	data[1] = ((version >> 8) & 0xFF);
	data[2] = (version & 0xFF);
	data[3] = 0x00;

	ret = set_nvm_data(info, CALIBRATION_INFOMATION, data);
	if (ret <= 0)
	{
		input_err(true, &info->client->dev,
			"%s: set failed. ret: %d\n", __func__, ret);
	}
	else{
		info->cal_count = data[0];
		info->tune_fix_ver = (unsigned short)((data[1] << 8) | data[2]);
		input_info(true, &info->client->dev,
			"%s: pat count:%X, version:%X\n", __func__, info->cal_count, info->tune_fix_ver);
	}

	kfree(data);
	return ret;
}

int fts_get_calibration_information(struct fts_ts_info *info)
{
	unsigned char *data = NULL;
	int ret;

	data = kzalloc(nvm_data[CALIBRATION_INFOMATION].length, GFP_KERNEL);
	if (!data) {
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);
		return -ENOMEM;
	}

	ret = get_nvm_data(info, CALIBRATION_INFOMATION, data);
	if (ret <= 0) {
		input_err(true, &info->client->dev,
			"%s: set failed. ret: %d\n", __func__, ret);
	} else {
		info->cal_count = data[0];
		info->tune_fix_ver = (unsigned short)((data[1] << 8) | data[2]);
		input_info(true, &info->client->dev,
			"%s: pat count:%X, version:%X\n", __func__, info->cal_count, info->tune_fix_ver);
	}

	kfree(data);
	return ret;
}
#endif

int fts_set_factory_debug_information(struct fts_ts_info *info, unsigned char base, unsigned char delta, unsigned char checksum)
{
	unsigned char *data = NULL;
	int ret;

	/* save calibration count */
	data = kzalloc(nvm_data[SEC_FACTORY_DEBUG].length, GFP_KERNEL);
	if (!data) {
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);
		return -ENOMEM;
	}

	data[0] = base;
	data[1] = delta;
	data[2] = checksum;
	data[3] = 0x00;

	/* SEC_FACTORY_DEBUG index is 4. refer "fts_nvm_data_type" write command */
	ret = set_nvm_data(info, FTS_FLASH_WRITE_COMMAND_SEC_FACTORY_DEBUG, data);
	if (ret <= 0)
		input_err(true, &info->client->dev,
			"%s: set failed. ret: %d\n", __func__, ret);

	kfree(data);

	return ret;
}

int fts_get_factory_debug_information(struct fts_ts_info *info)
{
	unsigned char *data = NULL;
	int ret;

	data = kzalloc(nvm_data[SEC_FACTORY_DEBUG].length, GFP_KERNEL);
	if (!data) {
		input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
				__func__);
		return -ENOMEM;
	}

	ret = get_nvm_data(info, SEC_FACTORY_DEBUG, data);
	if (ret <= 0) {
		input_err(true, &info->client->dev,
			"%s: set failed. ret: %d\n", __func__, ret);
	} else {
		if (data[0] == 0xFF)
			data[0] = 0x00;
		info->pressure_cal_base = data[0];

		if (data[1] == 0xFF)
			data[1] = 0x00;
		info->pressure_cal_delta = data[1];

		if (data[2] == 0xFF)
			data[2] = 0x00;
		info->nv_crc_fail_count = data[2];

		input_info(true, &info->client->dev,
			"%s: pressure base:%X, delta:%X, crc fail count:%X\n", __func__,
			info->pressure_cal_base, info->pressure_cal_delta,
			info->nv_crc_fail_count);
	}

	kfree(data);
	return ret;
}

int set_nvm_data(struct fts_ts_info *info, unsigned char type, unsigned char *buf)
{
	unsigned char regAdd[256] = { 0 };
	unsigned int length = nvm_data[type].length;
	int ret;

	fts_command(info, SENSEOFF);
	fts_delay(50);
	fts_command(info, FLUSHBUFFER);
	fts_interrupt_set(info, INT_DISABLE);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	regAdd[0] = 0xC7;
	regAdd[1] = type;

	memcpy(&regAdd[2], buf, length);

	ret = fts_write_reg(info, regAdd, length + 2);
	if (ret < 0) {
		input_err(true, &info->client->dev,
			"%s: failed. ret: %d\n", __func__, ret);
		return ret;
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
	fts_interrupt_set(info, INT_ENABLE);
	return ret;

}

int get_nvm_data(struct fts_ts_info *info, int type, unsigned char *nvdata)
{
	unsigned char regAdd[3] = {0xB8, 0x00, 0x08};
	unsigned char r_data[8] = { 0 };
  	unsigned char data[33] = { 0 };
	int size = 0, length = 0;
	int ret;

	size = sizeof(nvm_data) / sizeof(struct fts_nvm_data_map);
	if (type >= size)
		return -EINVAL;

	length = nvm_data[type].length + 1;

	fts_command(info, SENSEOFF);
	fts_delay(30);
	fts_interrupt_set(info, INT_DISABLE);
	fts_command(info, FLUSHBUFFER);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	ret = fts_write_reg(info, &regAdd[0], 3);
	if (ret <= 0) {
		input_err(true, &info->client->dev,
			"%s: write failed. ret: %d\n", __func__, ret);
		goto err_mode;
	}
	fts_fw_wait_for_specific_event(info, EVENTID_STATUS_REQUEST_COMP, regAdd[1], regAdd[2]);

	ret = info->fts_get_sysinfo_data(info, FTS_SI_COMPENSATION_OFFSET_ADDR, 7, r_data);
	if (ret < 0) {
		input_err(true, &info->client->dev,
			"%s: get failed. ret: %d\n", __func__, ret);
		goto err_mode;
	}

	input_info(true, &info->client->dev, "%s: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
		__func__, r_data[0], r_data[1], r_data[2], r_data[3], r_data[4], r_data[5], r_data[6], r_data[7]);

	regAdd[0] = 0xD0;
	regAdd[1] = r_data[1];
	regAdd[2] = r_data[0];

	regAdd[2] += (FTS_FLASH_DATA_OFFSET_BASE + nvm_data[type].offset);

	ret = fts_read_reg(info, &regAdd[0], 3, data, length);
	if (ret < 0) {
		input_err(true, &info->client->dev,
			"%s: read failed. ret: %d\n", __func__, ret);
		goto err_mode;
	}

	input_info(true, &info->client->dev, "%s: [%d][%d] (0x%02X), 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
		__func__, type, nvm_data[type].length, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

	memcpy(nvdata, &data[1], nvm_data[type].length);

err_mode:
	fts_command(info, FLUSHBUFFER);
	fts_delay(10);
	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif
	fts_interrupt_set(info, INT_ENABLE);

	return ret;
}

#ifdef FTS_SUPPORT_HOVER
static void hover_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped || !(info->reinit_done) || (info->fts_power_state != FTS_POWER_STATE_ACTIVE)) {
		input_err(true, &info->client->dev,
			"%s: [ERROR] Touch is stopped:%d, reinit_done:%d, power_state:%d\n",
			__func__, info->touch_stopped, info->reinit_done, info->fts_power_state);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;

		if(sec->cmd_param[0]==1){
			info->retry_hover_enable_after_wakeup = 1;
			input_info(true, &info->client->dev, "%s: retry_hover_on_after_wakeup\n", __func__);
		}

		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		int enables;
		enables = sec->cmd_param[0];
		if (enables == info->hover_enabled) {
			input_dbg(true, &info->client->dev,
				"%s: Skip duplicate command. Hover is already %s.\n",
				__func__, info->hover_enabled ? "enabled" : "disabled");
		} else {
			if (enables) {
				unsigned char regAdd[4] = {0xB0, 0x01, 0x29, 0x41};
				unsigned char Dly_regAdd[4] = {0xB0, 0x01, 0x72, 0x04};
				fts_write_reg(info, &Dly_regAdd[0], 4);
				fts_write_reg(info, &regAdd[0], 4);
				fts_command(info, FTS_CMD_HOVER_ON);
				info->hover_enabled = true;
				info->hover_ready = false;
			} else {
				unsigned char Dly_regAdd[4] = {0xB0, 0x01, 0x72, 0x08};
				fts_write_reg(info, &Dly_regAdd[0], 4);
				fts_command(info, FTS_CMD_HOVER_OFF);
				info->hover_enabled = false;
				info->hover_ready = false;
			}
		}
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;

out:
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

/* static void hover_no_sleep_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	unsigned char regAdd[4] = {0xB0, 0x01, 0x18, 0x00};
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
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
		if (sec->cmd_param[0]) {
			regAdd[3]=0x0F;
		} else {
			regAdd[3]=0x08;
		}
		fts_write_reg(info, &regAdd[0], 4);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
} */
#endif

#ifdef CONFIG_GLOVE_TOUCH
static void glove_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->glove_enabled = sec->cmd_param[0];

		if (!info->touch_stopped && info->reinit_done) {
			if (info->glove_enabled)
				fts_command(info, FTS_CMD_GLOVE_ON);
			else
				fts_command(info, FTS_CMD_GLOVE_OFF);
		}

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
	unsigned char cmd[4] =
		{ 0xB2, 0x01, 0xC6, 0x02 };
	int timeout=0;

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		char buff[SEC_CMD_STR_LEN] = { 0 };
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	fts_write_reg(info, &cmd[0], 4);
	sec->cmd_state = SEC_CMD_STATUS_RUNNING;

	while (sec->cmd_state == SEC_CMD_STATUS_RUNNING) {
		if (timeout++>30) {
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
			break;
		}
		msleep(10);
	}
}

static void fast_glove_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		info->fast_glove_enabled = sec->cmd_param[0];

		if (!info->touch_stopped && info->reinit_done) {
			if (info->fast_glove_enabled)
				fts_command(info, FTS_CMD_SET_FAST_GLOVE_MODE);
			else
				fts_command(info, FTS_CMD_SET_NOR_GLOVE_MODE);
		}

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
};
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

		if (!info->touch_stopped && info->reinit_done) {
			if (info->flip_enable) {
#ifdef CONFIG_GLOVE_TOUCH
				if (info->glove_enabled
					&& (strncmp(info->board->project_name, "TB", 2) != 0))
					fts_command(info, FTS_CMD_GLOVE_OFF);
#endif
				fts_set_cover_type(info, true);
			} else {
				fts_set_cover_type(info, false);
#ifdef CONFIG_GLOVE_TOUCH
				if (info->fast_glove_enabled)
					fts_command(info, FTS_CMD_SET_FAST_GLOVE_MODE);
				else if (info->glove_enabled)
					fts_command(info, FTS_CMD_GLOVE_ON);
#endif
			}
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

	sec_cmd_set_default_result(sec);

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 2) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0] == REPORT_RATE_90HZ)
			fts_change_scan_rate(info, FTS_CMD_FAST_SCAN);
		else if (sec->cmd_param[0] == REPORT_RATE_60HZ)
			fts_change_scan_rate(info, FTS_CMD_SLOW_SCAN);
		else if (sec->cmd_param[0] == REPORT_RATE_30HZ)
			fts_change_scan_rate(info, FTS_CMD_USLOW_SCAN);

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

	if (info->touch_stopped) {
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
		int enables;
		enables = sec->cmd_param[0];
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

	if (info->touch_stopped) {
		info->wirelesscharger_mode = sec->cmd_param[0];

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
		info->wirelesscharger_mode = sec->cmd_param[0];

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

/* ######################################################
    flag     1  :  set edge handler
              2  :  set (portrait, normal) edge zone data
              4  :  set (portrait, normal) dead zone data
              8  :  set landscape mode data
              16 :  mode clear
    data
              0x30, FFF (y start), FFF (y end),  FF(direction)
              0x31, FFFF (edge zone)
              0x32, FF (up x), FF (down x), FFFF (y)
              0x33, FF (mode), FFF (edge), FFF (dead zone)
    case
           edge handler set :  0x30....
           booting time :  0x30...  + 0x31...
           normal mode : 0x32...  (+0x31...)
           landscape mode : 0x33...
           landscape -> normal (if same with old data) : 0x33, 0
           landscape -> normal (etc) : 0x32....  + 0x33, 0
    ###################################################### */

void fts_set_grip_data_to_ic(struct fts_ts_info *info, u8 flag){
	u8 data[4] = { 0 };
	u8 regAdd[6] = {0xC6, 0x00, 0x00, 0x00, 0x00, 0x00};

	input_info(true, &info->client->dev, "%s: flag: %02X (clr,lan,nor,edg,han)\n", __func__, flag);

	if (flag & G_SET_EDGE_HANDLER) {
		if (info->grip_edgehandler_direction == 0) {
			data[0] = 0x0;
			data[1] = 0x0;
			data[2] = 0x0;
			data[3] = 0x0;
		} else {
			data[0] = (info->grip_edgehandler_start_y >> 4) & 0xFF;
			data[1] = (info->grip_edgehandler_start_y << 4 & 0xF0) | ((info->grip_edgehandler_end_y >> 8) & 0xF);
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

/* ######################################################
    index  0 :  set edge handler
              1 :  portrait (normal) mode
              2 :  landscape mode
    data
              0, X (direction), X (y start), X (y end)
                     direction : 0 (off), 1 (left), 2 (right)
                     ex) echo set_grip_data,0,2,600,900 > cmd

              1, X (edge zone), X (dead zone up x), X (dead zone down x), X (dead zone y)
                     ex) echo set_grip_data,1,200,10,50,1500 > cmd

              2, 1 (landscape mode), X (edge zone), X (dead zone)
                     ex) echo set_grip_data,2,1,200,100 > cmd

              2, 0 (portrait mode)
                     ex) echo set_grip_data,2,0  > cmd
###################################################### */

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
				__func__,sec->cmd_param[1], __LINE__);
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
		if (sec->cmd_param[1] == 0) { 	// normal mode
			info->grip_landscape_mode = 0;
			mode = mode | G_CLR_LANDSCAPE_MODE;
		} else if (sec->cmd_param[1] == 1) {
			info->grip_landscape_mode = 1;
			info->grip_landscape_edge = sec->cmd_param[2];
			info->grip_landscape_deadzone = sec->cmd_param[3];
			mode = mode | G_SET_LANDSCAPE_MODE;
		} else {
			input_err(true, &info->client->dev, "%s: cmd1 is abnormal, %d (%d)\n",
				__func__,sec->cmd_param[1], __LINE__);
			goto err_grip_data;
		}

		fts_set_grip_data_to_ic(info, mode);
	} else {
		input_err(true, &info->client->dev, "%s: cmd0 is abnormal, %d", __func__,sec->cmd_param[0]);
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
	return;
}

static void set_dead_zone(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char regAdd[2] = {0xC4, 0x00};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 6) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0]==1)
			regAdd[1] = 0x01;	/* side edge top */
		else if (sec->cmd_param[0]==2)
			regAdd[1] = 0x02;	/* side edge bottom */
		else if (sec->cmd_param[0]==3)
			regAdd[1] = 0x03;	/* side edge All On */
		else if (sec->cmd_param[0]==4)
			regAdd[1] = 0x04;	/* side edge Left Off */
		else if (sec->cmd_param[0]==5)
			regAdd[1] = 0x05;	/* side edge Right Off */
		else if (sec->cmd_param[0]==6)
			regAdd[1] = 0x06;	/* side edge All Off */
		else
			regAdd[1] = 0x0;	/* none	*/

		ret = fts_write_reg(info, regAdd, 2);

		if (ret < 0)
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		else
			input_info(true, &info->client->dev, "%s: reg:%d, ret: %d\n", __func__, sec->cmd_param[0], ret);

		fts_delay(1);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void dead_zone_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	unsigned char regAdd[2] = {0xC2, 0x0C};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0]==0) {
			regAdd[0] = 0xC1;	/* dead zone disable */
		} else {
			regAdd[0] = 0xC2;	/* dead zone enable */
		}

		ret = fts_write_reg(info, regAdd, 2);

		if (ret < 0)
			input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		else
			input_info(true, &info->client->dev, "%s: reg:%d, ret: %d\n", __func__, sec->cmd_param[0], ret);

		fts_delay(1);

		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void drawing_test_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
#ifdef FTS_SUPPORT_STRINGLIB
	unsigned short addr = FTS_CMD_STRING_ACCESS;
#endif
	int ret = 0;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (!sec->cmd_param[0]) {
		info->lowpower_flag |= FTS_MODE_PRESSURE;
	} else {
		info->lowpower_flag &= ~FTS_MODE_PRESSURE;
	}

#ifdef FTS_SUPPORT_STRINGLIB
	ret = info->fts_write_to_string(info, &addr, &info->lowpower_flag, sizeof(info->lowpower_flag));
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

static void spay_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
#ifdef FTS_SUPPORT_STRINGLIB
	unsigned short addr = FTS_CMD_STRING_ACCESS;
#endif
	int ret = 0;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0]) {
		info->lowpower_flag |= FTS_MODE_SPAY;
	} else {
		info->lowpower_flag &= ~FTS_MODE_SPAY;
	}

#ifdef FTS_SUPPORT_STRINGLIB
	ret = info->fts_write_to_string(info, &addr, &info->lowpower_flag, sizeof(info->lowpower_flag));
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

static void aod_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
#ifdef FTS_SUPPORT_STRINGLIB
	unsigned short addr = FTS_CMD_STRING_ACCESS;
#endif
	int ret = 0;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0]) {
		info->lowpower_flag |= FTS_MODE_AOD;
	} else {
		info->lowpower_flag &= ~FTS_MODE_AOD;
	}

#ifdef FTS_SUPPORT_STRINGLIB
	ret = info->fts_write_to_string(info, &addr, &info->lowpower_flag, sizeof(info->lowpower_flag));
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

static void set_aod_rect(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fts_ts_info *info = container_of(sec, struct fts_ts_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 data[8] = {0, };
	int i, ret = -1;
#ifdef FTS_SUPPORT_STRINGLIB
	unsigned short addr = FTS_CMD_STRING_ACCESS + 2;
#endif

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

	disable_irq(info->client->irq);
#ifdef FTS_SUPPORT_STRINGLIB
	ret = info->fts_write_to_string(info, &addr, data, sizeof(data));
#endif
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto NG;
	}

	enable_irq(info->client->irq);

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec_cmd_set_cmd_exit(sec);
	return;
NG:
	enable_irq(info->client->irq);
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
#ifdef FTS_SUPPORT_STRINGLIB
	unsigned short addr = FTS_CMD_STRING_ACCESS + 2;
#endif

	sec_cmd_set_default_result(sec);

	disable_irq(info->client->irq);
#ifdef FTS_SUPPORT_STRINGLIB
	ret = info->fts_read_from_string(info, &addr, data, sizeof(data));
#endif
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed. ret: %d\n", __func__, ret);
		goto NG;
	}

	enable_irq(info->client->irq);

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
	enable_irq(info->client->irq);
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
	unsigned char regAdd[3] = {0xC1, 0x13};
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
		regAdd[0] = 0xC1;
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
		regAdd[0] = 0xC2;
	}

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		goto out;
	}

	ret = fts_write_reg(info, regAdd, info->dex_mode ? 3 : 2);
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
	unsigned char regAdd[2] = {0xC1, 0x14};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	info->brush_mode = sec->cmd_param[0];

	if (info->touch_stopped) {
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
		regAdd[0] = 0xC2;	/* 0: Disable Artcanvas min phi mode */
	else
		regAdd[0] = 0xC1;	/* 1: Enable Artcanvas min phi mode  */

	ret = fts_write_reg(info, regAdd, 2);
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
	unsigned char regAdd[2] = {0xC1, 0x15};
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto out;
	}

	info->touchable_area = sec->cmd_param[0];

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
			__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	input_info(true, &info->client->dev,
		"%s: set 16:9 mode %s\n", __func__, info->touchable_area ? "enable" : "disable");

	if (info->touchable_area == 0)
		regAdd[0] = 0xC2;	/* 0: Disable 16:9 mode */
	else
		regAdd[0] = 0xC1;	/* 1: Enable 16:9 mode  */

	ret = fts_write_reg(info, regAdd, 2);
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

	input_info(true, &info->client->dev, "%s: delay time is %d\n", __func__, info->delay_time);
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

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n",
		__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto autotune_fail;
	}

	if (info->rawdata_read_lock == 1) {
		input_err(true, &info->client->dev, "%s: ramdump mode is runing, %d\n", __func__, info->rawdata_read_lock);
		goto autotune_fail;
	}

	if (info->touch_count > 0) {
		touch_on = true;
		input_err(true, info->dev, "%s: finger on touch(%d)\n", __func__, info->touch_count);
	}

	disable_irq(info->irq);

	fts_interrupt_set(info, INT_DISABLE);

	fts_command(info, SENSEOFF);
	fts_delay(50);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey) {
		fts_command(info, FTS_CMD_KEY_SENSE_OFF);
	}
#endif
	fts_command(info, FLUSHBUFFER);
	fts_delay(10);

	fts_release_all_finger(info);
#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_release_all_key(info);
#endif

	fts_command(info,FTS_CMD_FORCE_AUTOTUNE);

	if (touch_on) {
		input_err(true, info->dev, "%s: finger! do not run autotune\n", __func__);
	} else {
		input_info(true, info->dev, "%s: run autotune\n", __func__);
#ifdef PAT_CONTROL
		fts_execute_autotune(info);

		if(info->external_factory == true) {
			/* for external factory mode */
			if (info->cal_count == PAT_MAX_EXT)
				info->cal_count = PAT_MAX_EXT;
			else if (PAT_EXT_FACT <= info->cal_count && info->cal_count < PAT_MAX_EXT)
				info->cal_count++;
			else
				info->cal_count = PAT_EXT_FACT;

			/* not to enter external factory mode without setting everytime */
			info->external_factory = false;
		} else {
			/*change  from ( virtual pat  or vpat by external fatory )  to real pat by forced calibarion by LCIA   */
			if (info->cal_count >= PAT_MAGIC_NUMBER)
				info->cal_count = 1;
			else if (info->cal_count == PAT_MAX_LCIA)
				info->cal_count = PAT_MAX_LCIA;
			else
				info->cal_count++;
		}

		input_info(true, &info->client->dev, "%s: write to nvm %X\n",
					__func__, info->cal_count);

		/* get ic information */
		info->fts_get_version_info(info);

		fts_set_calibration_information(info, info->cal_count, info->fw_main_version_of_ic);

		fts_get_calibration_information(info);

		input_info(true, &info->client->dev, "%s: tune_fix_ver [0x%04X]\n", __func__, info->tune_fix_ver);
#else
		fts_execute_autotune(info);
#endif
	}

	fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
	fts_fw_wait_for_event (info, STATUS_EVENT_FORCE_CAL_DONE_D3);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif

	fts_interrupt_set(info, INT_ENABLE);
	enable_irq(info->irq);

	if (touch_on) {
		snprintf(buff, sizeof(buff), "NG_FINGER_ON");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;

autotune_fail:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
	return;
}

#ifdef FTS_SUPPORT_TOUCH_KEY
int read_touchkey_data(struct fts_ts_info *info, unsigned char type, unsigned int keycode)
{
	unsigned char pCMD[3] = { 0xD0, 0x00, 0x00};
	unsigned char buf[9] = { 0 };
	int i;
	int ret = 0;

	pCMD[2] = type;

	ret = fts_read_reg(info, &pCMD[0], 3, buf, 3);
	if (ret >= 0) {
		pCMD[1] = buf[2];
		pCMD[2] = buf[1];
	} else
		return -1;

	ret = fts_read_reg(info, &pCMD[0], 3, buf, 9);
	if (ret < 0)
		return -2;

	for (i = 0 ; i < info->board->num_touchkey ; i++)
		if (info->board->touchkey[i].keycode == keycode) {
			return *(short *)&buf[(info->board->touchkey[i].value - 1) * 2 + 1];
		}

	return -3;
}

static ssize_t touchkey_recent_strength(struct device *dev,
				       struct device_attribute *attr, char *buf) {
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return sprintf(buf, "%d\n", value);
	}

	value = read_touchkey_data(info, TYPE_TOUCHKEY_STRENGTH, KEY_RECENT);

	return sprintf(buf, "%d\n", value);
}

static ssize_t touchkey_back_strength(struct device *dev,
				       struct device_attribute *attr, char *buf) {
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return sprintf(buf, "%d\n", value);
	}

	value = read_touchkey_data(info, TYPE_TOUCHKEY_STRENGTH, KEY_BACK);

	return sprintf(buf, "%d\n", value);
}

static ssize_t touchkey_recent_raw(struct device *dev,
				       struct device_attribute *attr, char *buf) {
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return sprintf(buf, "%d\n", value);
	}

	value = read_touchkey_data(info, TYPE_TOUCHKEY_RAW, KEY_RECENT);

	return sprintf(buf, "%d\n", value);
}

static ssize_t touchkey_back_raw(struct device *dev,
				       struct device_attribute *attr, char *buf) {
	struct fts_ts_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->touch_stopped) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		return sprintf(buf, "%d\n", value);
	}

	value = read_touchkey_data(info, TYPE_TOUCHKEY_RAW, KEY_BACK);

	return sprintf(buf, "%d\n", value);
}

static ssize_t touchkey_threshold(struct device *dev,
				       struct device_attribute *attr, char *buf) {
	struct fts_ts_info *info = dev_get_drvdata(dev);
	unsigned char pCMD[3] = { 0xD0, 0x00, 0x00};
	int value;
	int ret = 0;

	value = -1;
	pCMD[2] = FTS_SI_SS_KEY_THRESHOLD;
	ret = fts_read_reg(info, &pCMD[0], 3, buf, 3);
	if (ret >= 0) {
		value = *(unsigned short *)&buf[1];
	}

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

static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL, fts_touchkey_led_control);
static DEVICE_ATTR(touchkey_recent, S_IRUGO, touchkey_recent_strength, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, touchkey_back_strength, NULL);
static DEVICE_ATTR(touchkey_recent_raw, S_IRUGO, touchkey_recent_raw, NULL);
static DEVICE_ATTR(touchkey_back_raw, S_IRUGO, touchkey_back_raw, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold, NULL);

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
