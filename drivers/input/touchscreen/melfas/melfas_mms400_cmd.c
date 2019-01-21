/*
 * MELFAS MMS400 Touchscreen
 *
 * Copyright (C) 2014 MELFAS Inc.
 *
 *
 * Command Functions (Optional)
 *
 */

#include "melfas_mms400.h"

#if MMS_USE_CMD_MODE

#define NAME_OF_UNKNOWN_CMD "not_support_cmd"

/**
 * Command : Update firmware
 */
static void cmd_fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	int fw_location = sec->cmd_param[0];

	sec_cmd_set_default_result(sec);

	switch (fw_location) {
	case 0:
		if (mms_fw_update_from_kernel(info, true)) {
			goto ERROR;
		}
		break;
	case 1:
		if (mms_fw_update_from_storage(info, true)) {
			goto ERROR;
		}
		break;
	default:
		goto ERROR;
		break;
	}

	sprintf(buf, "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	goto EXIT;

ERROR:
	sprintf(buf, "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	goto EXIT;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get firmware version from MFSB file
 */
static void cmd_get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	const struct firmware *fw;
	struct mms_bin_hdr *fw_hdr;
	struct mms_fw_img **img;
	u8 ver_file[MMS_FW_MAX_SECT_NUM * 2];
	int i = 0;
	int offset = sizeof(struct mms_bin_hdr);

	sec_cmd_set_default_result(sec);

	request_firmware(&fw, info->dtdata->fw_name, &info->client->dev);

	if (!fw) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	fw_hdr = (struct mms_bin_hdr *)fw->data;
	img = kzalloc(sizeof(*img) * fw_hdr->section_num, GFP_KERNEL);

	for (i = 0; i < fw_hdr->section_num; i++, offset += sizeof(struct mms_fw_img)) {
		img[i] = (struct mms_fw_img *)(fw->data + offset);
		ver_file[i * 2] = ((img[i]->version) >> 8) & 0xFF;
		ver_file[i * 2 + 1] = (img[i]->version) & 0xFF;
	}

	release_firmware(fw);

	input_info(true, &info->client->dev, "%s: OCTA ID 0x%06x\n", __func__, info->octa_id);

	sprintf(buf, "ME%02X%02X%02X\n", MMS_HW_VERSION, ver_file[3],ver_file[5]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(img);

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get firmware version from IC
 */
static void cmd_get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };
	u8 rbuf[8];

	sec_cmd_set_default_result(sec);

	if (mms_get_fw_version(info, rbuf)) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	info->boot_ver_ic = rbuf[1];
	info->core_ver_ic = rbuf[3];
	info->config_ver_ic = rbuf[5];

	input_info(true, &info->client->dev, "%s: OCTA ID 0x%06x\n", __func__, info->octa_id);

	sprintf(buf, "ME%02X%02X%02X\n",
		MMS_HW_VERSION, info->core_ver_ic, info->config_ver_ic);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get chip vendor
 */
static void cmd_get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	sprintf(buf, "MELFAS");
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get chip name
 */
static void cmd_get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	sprintf(buf, CHIP_NAME);
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

static void cmd_get_config_ver(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);
	sprintf(buf, "%s_ME_%02d%02d",
		info->product_name, info->fw_month, info->fw_date);
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get X ch num
 */
static void cmd_get_x_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };
	u8 rbuf[1];
	u8 wbuf[2];
	int val;

	sec_cmd_set_default_result(sec);

	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_NODE_NUM_X;
	if (mms_i2c_read(info, wbuf, 2, rbuf, 1)) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	val = rbuf[0];

	sprintf(buf, "%d", val);
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get Y ch num
 */
static void cmd_get_y_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };
	u8 rbuf[1];
	u8 wbuf[2];
	int val;

	sec_cmd_set_default_result(sec);

	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_NODE_NUM_Y;
	if (mms_i2c_read(info, wbuf, 2, rbuf, 1)) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	val = rbuf[0];

	sprintf(buf, "%d", val);
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get X resolution
 */
static void cmd_get_max_x(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };
	u8 rbuf[2];
	u8 wbuf[2];
	int val;

	sec_cmd_set_default_result(sec);

	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_RESOLUTION_X;
	if (mms_i2c_read(info, wbuf, 2, rbuf, 2)) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	val = (rbuf[0]) | (rbuf[1] << 8);

	sprintf(buf, "%d", val);
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get Y resolution
 */
static void cmd_get_max_y(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };
	u8 rbuf[2];
	u8 wbuf[2];
	int val;

	sec_cmd_set_default_result(sec);

	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_RESOLUTION_Y;
	if (mms_i2c_read(info, wbuf, 2, rbuf, 2)) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	val = (rbuf[0]) | (rbuf[1] << 8);

	sprintf(buf, "%d", val);
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));

	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Power off
 */
static void cmd_module_off_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	mms_power_control(info, 0);

	sprintf(buf, "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Power on
 */
static void cmd_module_on_master(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	mms_power_control(info, 1);

	if (info->flip_enable)
		mms_set_cover_type(info);

	sprintf(buf, "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Read intensity image
 */
static void cmd_read_intensity(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	int min = 999999;
	int max = -999999;
	int i = 0;

	sec_cmd_set_default_result(sec);

	if (mms_get_image(info, MIP_IMG_TYPE_INTENSITY)) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	for (i = 0; i < (info->node_x * info->node_y); i++) {
		if (info->image_buf[i] > max) {
			max = info->image_buf[i];
		}
		if (info->image_buf[i] < min) {
			min = info->image_buf[i];
		}
	}

	sprintf(buf, "%d,%d", min, max);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get intensity data
 */
static void cmd_get_intensity(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	int x = sec->cmd_param[0];
	int y = sec->cmd_param[1];
	int idx = 0;

	sec_cmd_set_default_result(sec);

	if ((x < 0) || (x >= info->node_x) || (y < 0) || (y >= info->node_y)) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	idx = x + y * info->node_x;

	sprintf(buf, "%d", info->image_buf[idx]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Read rawdata image
 */
static void cmd_read_rawdata(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	int min = 999999;
	int max = -999999;
	int i = 0;

	sec_cmd_set_default_result(sec);

	if (mms_get_image(info, MIP_IMG_TYPE_RAWDATA)) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	for (i = 0; i < (info->node_x * info->node_y); i++) {
		if (info->image_buf[i] > max) {
			max = info->image_buf[i];
		}
		if (info->image_buf[i] < min) {
			min = info->image_buf[i];
		}
	}

	sprintf(buf, "%d,%d", min, max);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get rawdata
 */
static void cmd_get_rawdata(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	int x = sec->cmd_param[0];
	int y = sec->cmd_param[1];
	int idx = 0;

	sec_cmd_set_default_result(sec);

	if ((x < 0) || (x >= info->node_x) || (y < 0) || (y >= info->node_y)) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	idx = x + y * info->node_x;

	sprintf(buf, "%d", info->image_buf[idx]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Run cm delta test
 */
static void cmd_run_test_cm_delta(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	int min = 999999;
	int max = -999999;
	int i = 0;

	sec_cmd_set_default_result(sec);

	if (mms_run_test(info, MIP_TEST_TYPE_CM_DELTA)) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	for (i = 0; i < (info->node_x * info->node_y); i++) {
		if (info->image_buf[i] > max) {
			max = info->image_buf[i];
		}
		if (info->image_buf[i] < min) {
			min = info->image_buf[i];
		}
	}

	sprintf(buf, "%d,%d", min, max);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get result of cm delta test
 */
static void cmd_get_cm_delta(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	int x = sec->cmd_param[0];
	int y = sec->cmd_param[1];
	int idx = 0;

	sec_cmd_set_default_result(sec);

	if ((x < 0) || (x >= info->node_x) || (y < 0) || (y >= info->node_y)) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	idx = x + y * info->node_x;

	sprintf(buf, "%d", info->image_buf[idx]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Run cm abs test
 */
static void cmd_run_test_cm_abs(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	int min = 999999;
	int max = -999999;
	int i = 0;

	sec_cmd_set_default_result(sec);

	if (mms_run_test(info, MIP_TEST_TYPE_CM_ABS)) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	for (i = 0; i < (info->node_x * info->node_y); i++) {
		if (info->image_buf[i] > max) {
			max = info->image_buf[i];
		}
		if (info->image_buf[i] < min) {
			min = info->image_buf[i];
		}
	}

	sprintf(buf, "%d,%d", min, max);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * Command : Get result of cm abs test
 */
static void cmd_get_cm_abs(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };

	int x = sec->cmd_param[0];
	int y = sec->cmd_param[1];
	int idx = 0;

	sec_cmd_set_default_result(sec);

	if ((x < 0) || (x >= info->node_x) || (y < 0) || (y >= info->node_y)) {
		sprintf(buf, "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	idx = x + y * info->node_x;

	sprintf(buf, "%d", info->image_buf[idx]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

static void cmd_get_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };
	u8 wbuf[2];
	u8 rbuf[1];

	sec_cmd_set_default_result(sec);

	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_IC_CONTACT_ON_THD;
	if (mms_i2c_read(info, wbuf, 2, rbuf, 1)) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		sprintf(buf, "%s", "NG");
		goto EXIT;
	}
	input_info(true, &info->client->dev,
			"%s: read from IC, %d\n",
			__func__, rbuf[0]);
	if (rbuf[0] > 0)
		sprintf(buf, "%d", rbuf[0]);
	else
		sprintf(buf, "55");

	sec->cmd_state = SEC_CMD_STATUS_OK;
EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

static void dead_zone_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };
	u8 wbuf[3];

	sec_cmd_set_default_result(sec);

	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_DISABLE_EDGE_EXPAND;
	wbuf[2] = 0;

	if (sec->cmd_param[0] == 0) {
		wbuf[2] = 2;
	} else if (sec->cmd_param[0] == 1) {
		wbuf[2] = 0;
	} else {
		sprintf(buf, "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto EXIT;
	}

	if (mms_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] mms_i2c_write\n", __func__);
		sprintf(buf, "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		sprintf(buf, "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));


	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

static void get_checksum_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[SEC_CMD_STR_LEN] = { 0 };
	u8 wbuf[2];
	u8 rbuf[12];

	sec_cmd_set_default_result(sec);

	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_BUILD_DATE;

	if (mms_i2c_read(info, wbuf, 2, rbuf, 12)) {
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		sprintf(buf, "NG");
		goto EXIT;
	}
	snprintf(buf, sizeof(buf), "%02X%02X", rbuf[11], rbuf[10]);
	sec->cmd_state = SEC_CMD_STATUS_OK;

EXIT:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}


static void clear_cover_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	struct i2c_client *client = info->client;
	char buf[SEC_CMD_STR_LEN] = { 0 };

	input_info(true, &client->dev, "%s: %d, %d\n",
			__func__, sec->cmd_param[0], sec->cmd_param[1]);
	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 3) {
		sprintf(buf, "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0] > 1) {
			info->flip_enable = true;
			info->cover_type = sec->cmd_param[1];
		} else {
			info->flip_enable = false;
		}

		if (info->enabled && !info->init)
			mms_set_cover_type(info);

		sprintf(buf, "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));

	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	input_info(true, &client->dev, "%s: %s\n", __func__, buf);
};

static void get_intensity_all_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	int ret;
	int length;

	sec_cmd_set_default_result(sec);

	info->read_all_data = true;

	ret = mms_get_image(info, MIP_IMG_TYPE_INTENSITY);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read intensity, %d\n", __func__, ret);
		sprintf(info->print_buf, "%s", "NG");
		goto out;
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;

	length = (int)strlen(info->print_buf);
	input_err(true, &info->client->dev, "%s: length is %d\n", __func__, length);

out:
	sec_cmd_set_cmd_result(sec, info->print_buf, length);

	info->read_all_data = false;
}

static void get_rawdata_all_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	int ret;
	int length;

	sec_cmd_set_default_result(sec);

	info->read_all_data = true;

	ret = mms_get_image(info, MIP_IMG_TYPE_RAWDATA);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read raw data, %d\n", __func__, ret);
		sprintf(info->print_buf, "%s", "NG");
		goto out;
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	length = (int)strlen(info->print_buf);
	input_err(true, &info->client->dev, "%s: length is %d\n", __func__, length);

out:
	sec_cmd_set_cmd_result(sec, info->print_buf, length);

	info->read_all_data = false;
}

static void get_cm_delta_all_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	int ret;
	int length;

	sec_cmd_set_default_result(sec);

	info->read_all_data = true;

	ret = mms_run_test(info, MIP_TEST_TYPE_CM_DELTA);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read cm delta, %d\n", __func__, ret);
		sprintf(info->print_buf, "%s", "NG");
		goto out;
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	length = (int)strlen(info->print_buf);
	input_err(true, &info->client->dev, "%s: length is %d\n", __func__, length);

out:
	sec_cmd_set_cmd_result(sec, info->print_buf, length);

	info->read_all_data = false;
}

static void get_cm_abs_all_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	int ret;
	int length;

	sec_cmd_set_default_result(sec);

	info->read_all_data = true;

	ret = mms_run_test(info, MIP_TEST_TYPE_CM_ABS);
	if (ret < 0) {
		input_err(true, &info->client->dev, "%s: failed to read cm abs, %d\n", __func__, ret);
		sprintf(info->print_buf, "%s", "NG");
		goto out;
	}

	sec->cmd_state = SEC_CMD_STATUS_OK;
	length = (int)strlen(info->print_buf);
	input_err(true, &info->client->dev, "%s: length is %d\n", __func__, length);

out:
	sec_cmd_set_cmd_result(sec, info->print_buf, length);

	info->read_all_data = false;
}

static void spay_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	struct i2c_client *client = info->client;
	char buf[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	/* if (!info->enabled) {
		input_err(true, &info->client->dev, "%s: [ERROR] Touch is stopped\n", __func__);
		snprintf(buf, sizeof(buf), "%s", "TSP turned off");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}*/

	if (!info->dtdata->support_lpm) {
		input_err(true, &info->client->dev, "%s not supported\n", __func__);
		snprintf(buf, sizeof(buf), "%s", NAME_OF_UNKNOWN_CMD);
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	if (sec->cmd_param[0]) {
		info->lowpower_mode = true;
		info->lowpower_flag = info->lowpower_flag | MMS_LPM_FLAG_SPAY;
	} else {
		info->lowpower_flag = info->lowpower_flag & ~(MMS_LPM_FLAG_SPAY);
		if (!info->lowpower_flag)
			info->lowpower_mode = false;
	}

	input_info(true, &client->dev, "%s: %s mode, %x\n",
			__func__, info->lowpower_mode ? "LPM" : "normal",
			info->lowpower_flag);
	snprintf(buf, sizeof(buf), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	sec_cmd_set_cmd_exit(sec);
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	input_info(true, &client->dev, "%s: %s\n", __func__, buf);
}

/**
 * Command : Unknown cmd
 */
static void cmd_unknown_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	char buf[16] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buf, sizeof(buf), "%s", NAME_OF_UNKNOWN_CMD);
	sec_cmd_set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));

	sec_cmd_set_cmd_exit(sec);

	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;

	input_dbg(true, &info->client->dev, "%s - cmd[%s] len[%d] state[%d]\n",
		__func__, buf, (int)strnlen(buf, sizeof(buf)), sec->cmd_state);
}

/**
 * List of command functions
 */
static struct sec_cmd mms_commands[] = {
	{SEC_CMD("fw_update", cmd_fw_update),},
	{SEC_CMD("get_fw_ver_bin", cmd_get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", cmd_get_fw_ver_ic),},
	{SEC_CMD("get_chip_vendor", cmd_get_chip_vendor),},
	{SEC_CMD("get_chip_name", cmd_get_chip_name),},
	{SEC_CMD("get_x_num", cmd_get_x_num),},
	{SEC_CMD("get_y_num", cmd_get_y_num),},
	{SEC_CMD("get_max_x", cmd_get_max_x),},
	{SEC_CMD("get_max_y", cmd_get_max_y),},
	{SEC_CMD("module_off_master", cmd_module_off_master),},
	{SEC_CMD("module_on_master", cmd_module_on_master),},
	{SEC_CMD("run_intensity_read", cmd_read_intensity),},
	{SEC_CMD("get_intensity", cmd_get_intensity),},
	{SEC_CMD("run_rawdata_read", cmd_read_rawdata),},
	{SEC_CMD("get_rawdata", cmd_get_rawdata),},
	{SEC_CMD("run_inspection_read", cmd_run_test_cm_delta),},
	{SEC_CMD("get_inspection", cmd_get_cm_delta),},
	{SEC_CMD("run_cm_delta_read", cmd_run_test_cm_delta),},
	{SEC_CMD("get_cm_delta", cmd_get_cm_delta),},
	{SEC_CMD("run_cm_abs_read", cmd_run_test_cm_abs),},
	{SEC_CMD("get_cm_abs", cmd_get_cm_abs),},
	{SEC_CMD("get_config_ver", cmd_get_config_ver),},
	{SEC_CMD("get_threshold", cmd_get_threshold),},
	{SEC_CMD("get_intensity_all_data", get_intensity_all_data),},
	{SEC_CMD("get_rawdata_all_data", get_rawdata_all_data),},
	{SEC_CMD("get_cm_delta_all_data", get_cm_delta_all_data),},
	{SEC_CMD("get_cm_abs_all_data", get_cm_abs_all_data),},
	{SEC_CMD("module_off_slave", cmd_unknown_cmd),},
	{SEC_CMD("module_on_slave", cmd_unknown_cmd),},
	{SEC_CMD("dead_zone_enable", dead_zone_enable),},
	{SEC_CMD("get_checksum_data", get_checksum_data),},
	{SEC_CMD("clear_cover_mode", clear_cover_mode),},
	{SEC_CMD("spay_enable", spay_enable),},
	{SEC_CMD(NAME_OF_UNKNOWN_CMD, cmd_unknown_cmd),},
};

static ssize_t scrub_position_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct mms_ts_info *info = container_of(sec, struct mms_ts_info, sec);
	struct i2c_client *client = info->client;
	char buff[256] = { 0 };

	input_info(true, &client->dev, "%s: scrub_id: %d, X:%d, Y:%d\n", __func__,
				info->scrub_id, 0, 0);

	snprintf(buff, sizeof(buff), "%d %d %d", info->scrub_id, 0, 0);

	info->scrub_id = 0;
	return snprintf(buf, PAGE_SIZE, "%s", buff);
}

static DEVICE_ATTR(scrub_pos, S_IRUGO, scrub_position_show, NULL);

static struct attribute *touchscreen_attributes[] = {
	&dev_attr_scrub_pos.attr,
	NULL,
};

static struct attribute_group touchscreen_attr_group = {
	.attrs = touchscreen_attributes,
};

/**
 * Create sysfs command functions
 */
int mms_sysfs_cmd_create(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int retval;

	retval = sec_cmd_init(&info->sec, mms_commands,
			ARRAY_SIZE(mms_commands), SEC_CLASS_DEVT_TSP);
	if (retval < 0) {
		input_err(true, &info->client->dev,
			"%s: Failed to sec_cmd_init\n", __func__);
		return retval;
	}

	retval = sysfs_create_group(&info->sec.fac_dev->kobj,
			&touchscreen_attr_group);
	if (retval < 0) {
		input_err(true, &info->client->dev,
			"%s: Failed to sysfs group\n", __func__);
		return retval;
	}

	info->print_buf = kzalloc(sizeof(u8) * 4096, GFP_KERNEL);
	if (!info->print_buf) {
		input_err(true, &info->client->dev,
			"%s: Failed to alloc\n", __func__);
		sec_cmd_exit(&info->sec, SEC_CLASS_DEVT_TSP);
		return -ENOMEM;
	}

	retval = sysfs_create_link(&info->sec.fac_dev->kobj,
		&info->input_dev->dev.kobj, "input");
	if (retval < 0) {
		input_err(true, &client->dev, "%s [ERROR] sysfs_create_link\n", __func__);
		kfree(info->print_buf);
		sec_cmd_exit(&info->sec, SEC_CLASS_DEVT_TSP);
		return retval;
	}

	return 0;
}
#endif
