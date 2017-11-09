/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm_qos.h>
#include <linux/device.h>

#include "fimc-is-sysfs.h"
#include "fimc-is-core.h"
#include "fimc-is-err.h"
#include "fimc-is-sec-define.h"
#if defined (CONFIG_OIS_USE)
#include "fimc-is-device-ois.h"
#endif
#ifdef CONFIG_AF_HOST_CONTROL
#include "fimc-is-device-af.h"
#endif
#include "fimc-is-vender-specific.h"
#include "fimc-is-interface-library.h"

/* #define FORCE_CAL_LOAD */

extern struct device *fimc_is_dev;
struct class *camera_class = NULL;
struct device *camera_front_dev;
struct device *camera_rear_dev;
#ifdef CONFIG_SECURE_CAMERA_USE
struct device *camera_secure_dev;
#endif
#if defined (CONFIG_OIS_USE)
struct device *camera_ois_dev;
#endif
static struct fimc_is_core *sysfs_core;
extern struct kset *devices_kset;
extern struct fimc_is_lib_support gPtr_lib_support;

extern bool crc32_fw_check;
extern bool crc32_setfile_check;
extern bool crc32_front_setfile_check;
extern bool crc32_check;
extern bool crc32_check_factory;
extern bool fw_version_crc_check;
extern bool is_latest_cam_module;
extern bool is_final_cam_module;
extern bool crc32_hifi_tunningfile_check;

#ifdef CONFIG_COMPANION_USE
extern bool crc32_c1_fw_check;
extern bool crc32_c1_check;
extern bool crc32_c1_check_factory;
#endif /* CONFIG_COMPANION_USE */
extern bool crc32_check_factory_rear2;
extern bool fw_version_crc_check;
static struct fimc_is_from_info *pinfo = NULL;
static struct fimc_is_from_info *finfo = NULL;
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
extern bool crc32_check_factory_front;
extern bool is_final_cam_module_front;
static struct fimc_is_from_info *front_finfo = NULL;
#endif

#ifdef CAMERA_SYSFS_V2
static struct fimc_is_cam_info cam_infos[CAM_INFO_MAX];
#endif

extern bool force_caldata_dump;
#if defined (CONFIG_OIS_USE)
static bool check_ois_power = false;
int ois_threshold = 150;
#ifdef CAMERA_REAR2_OIS
int ois_threshold_rear2 = 150;
#endif
#endif
static bool check_module_init = false;

#ifdef CONFIG_COMPANION_FACTORY_VALIDATION
extern int comp_fac_i2c_check;
extern u16 comp_fac_valid_check;
#endif

#ifdef CONFIG_SECURE_CAMERA_USE
extern bool is_final_cam_module_iris;
extern bool is_iris_ver_read;
extern bool is_iris_mtf_test_check;
extern u8 is_iris_mtf_read_data[3];	/* 0:year 1:month 2:company */
#endif

struct ssrm_camera_data {
	int operation;
	int cameraID;
	int previewSizeWidth;
	int previewSizeHeight;
	int previewMinFPS;
	int previewMaxFPS;
	int sensorOn;
};

enum ssrm_camerainfo_operation {
	SSRM_CAMERA_INFO_CLEAR,
	SSRM_CAMERA_INFO_SET,
	SSRM_CAMERA_INFO_UPDATE,
};

struct ssrm_camera_data SsrmCameraInfo[FIMC_IS_SENSOR_COUNT];

#ifdef CAMERA_SYSFS_V2
int fimc_is_get_cam_info(struct fimc_is_cam_info **caminfo)
{
	*caminfo = cam_infos;
	return 0;
}
#endif

static int read_from_firmware_version(int position)
{
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;
	int ret = 0;

	fimc_is_vender_check_hw_init_running();
	fimc_is_sec_get_sysfs_pinfo(&pinfo);
	fimc_is_sec_get_sysfs_finfo(&finfo);
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
	fimc_is_sec_get_sysfs_finfo_front(&front_finfo);
#endif

	if (((position == SENSOR_POSITION_REAR) && (!finfo->is_caldata_read))
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
		|| ((position == SENSOR_POSITION_FRONT) && (!front_finfo->is_caldata_read))
#endif
	) {
		ret = fimc_is_sec_run_fw_sel(is_dev, position);
		if (ret) {
			err("fimc_is_sec_run_fw_sel is fail(%d)", ret);
		}
	}

#ifdef CONFIG_COMPANION_USE
	if ((position == SENSOR_POSITION_REAR) && (!finfo->is_c1_caldata_read)) {
		fimc_is_sec_concord_fw_sel(sysfs_core, is_dev);
	}
#endif

	return 0;
}

static int read_from_firmware_version_reload(int position)
{
	int ret = 0;
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;

	ret = fimc_is_sec_run_fw_sel(is_dev, position);
	if (ret)
		err("fimc_is_sec_run_fw_sel is fail(%d)", ret);

	return 0;
}

#if defined (CONFIG_OIS_USE)
static bool read_ois_version(void)
{
	bool ret = true;
	struct fimc_is_vender_specific *specific = sysfs_core->vender.private_data;

	if (!specific->running_rear_camera) {
		if (!specific->ois_ver_read || force_caldata_dump) {
			fimc_is_ois_gpio_on(sysfs_core);
			msleep(150);

			ret = fimc_is_ois_check_fw(sysfs_core);
			if (!specific->running_rear_camera) {
				fimc_is_ois_gpio_off(sysfs_core);
			}
		}
	}
	return ret;
}
#endif

static ssize_t camera_front_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	int ret = 0;
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;

	if (force_caldata_dump == false) {
		ret = fimc_is_sec_run_fw_sel(is_dev, SENSOR_POSITION_FRONT);
		if (ret) {
			err("fimc_is_sec_run_fw_sel is fail(%d)", ret);
		}
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;

	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->front_sensor_id);
}

static ssize_t camera_rear_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;
	int ret = 0;
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;

	if (force_caldata_dump == false) {
		ret = fimc_is_sec_run_fw_sel(is_dev, SENSOR_POSITION_REAR);
		if (ret) {
			err("fimc_is_sec_run_fw_sel is fail(%d)", ret);
		}
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->rear_sensor_id);
}

static DEVICE_ATTR(front_sensorid, S_IRUGO, camera_front_sensorid_show, NULL);
static DEVICE_ATTR(rear_sensorid, S_IRUGO, camera_rear_sensorid_show, NULL);

static ssize_t camera_ssrm_camera_info_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct ssrm_camera_data temp;
	int ret_count;
	int index = -1;
	int i = 0;

	ret_count = sscanf(buf, "%d%d%d%d%d%d%d", &temp.operation, &temp.cameraID, &temp.previewMinFPS,
		&temp.previewMaxFPS, &temp.previewSizeWidth,  &temp.previewSizeHeight, &temp.sensorOn);

	if (ret_count > sizeof(SsrmCameraInfo)/sizeof(int))
		return -EINVAL;

	switch (temp.operation) {
	case SSRM_CAMERA_INFO_CLEAR:
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) { /* clear */
			if (SsrmCameraInfo[i].cameraID == temp.cameraID) {
				SsrmCameraInfo[i].previewMaxFPS = 0;
				SsrmCameraInfo[i].previewMinFPS = 0;
				SsrmCameraInfo[i].previewSizeHeight = 0;
				SsrmCameraInfo[i].previewSizeWidth = 0;
				SsrmCameraInfo[i].sensorOn = 0;
				SsrmCameraInfo[i].cameraID = -1;
			}
		}
		break;

	case SSRM_CAMERA_INFO_SET:
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) { /* find empty space*/
			if (SsrmCameraInfo[i].cameraID == -1) {
				index = i;
				break;
			}
		}

		if (index == -1)
			return -EPERM;

		memcpy(&SsrmCameraInfo[i], &temp, sizeof(temp));
		break;

	case SSRM_CAMERA_INFO_UPDATE:
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
			if (SsrmCameraInfo[i].cameraID == temp.cameraID) {
				SsrmCameraInfo[i].previewMaxFPS = temp.previewMaxFPS;
				SsrmCameraInfo[i].previewMinFPS = temp.previewMinFPS;
				SsrmCameraInfo[i].previewSizeHeight = temp.previewSizeHeight;
				SsrmCameraInfo[i].previewSizeWidth = temp.previewSizeWidth;
				SsrmCameraInfo[i].sensorOn = temp.sensorOn;
				break;
			}
		}
		break;
	default:
		break;
	}

	return count;
}

static ssize_t camera_ssrm_camera_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp_buffer[50] = {0,};
	int i = 0;

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		if (SsrmCameraInfo[i].cameraID != -1) {
			strncat(buf, "ID=", strlen("ID="));
			sprintf(temp_buffer, "%d;", SsrmCameraInfo[i].cameraID);
			strncat(buf, temp_buffer, strlen(temp_buffer));

			strncat(buf, "ON=", strlen("ON="));
			sprintf(temp_buffer, "%d;", SsrmCameraInfo[i].sensorOn);
			strncat(buf, temp_buffer, strlen(temp_buffer));

			if (SsrmCameraInfo[i].previewMinFPS && SsrmCameraInfo[i].previewMaxFPS) {
				strncat(buf, "FPS=", strlen("FPS="));
				sprintf(temp_buffer, "%d,%d;",
					SsrmCameraInfo[i].previewMinFPS, SsrmCameraInfo[i].previewMaxFPS);
				strncat(buf, temp_buffer, strlen(temp_buffer));
			}
			if (SsrmCameraInfo[i].previewSizeWidth && SsrmCameraInfo[i].previewSizeHeight) {
				strncat(buf, "SIZE=", strlen("SIZE="));
				sprintf(temp_buffer, "%d,%d;",
					SsrmCameraInfo[i].previewSizeWidth, SsrmCameraInfo[i].previewSizeHeight);
				strncat(buf, temp_buffer, strlen(temp_buffer));
			}
			strncat(buf, "\n", strlen("\n"));
		}
	}
	return strlen(buf);
}

static DEVICE_ATTR(ssrm_camera_info, 0644, camera_ssrm_camera_info_show, camera_ssrm_camera_info_store);

#ifdef CONFIG_SECURE_CAMERA_USE
static ssize_t camera_secure_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;

	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->secure_sensor_id);
}
static DEVICE_ATTR(secure_sensorid, S_IRUGO, camera_secure_sensorid_show, NULL);
#endif

static int fimc_is_get_sensor_data(struct device *dev, char *maker, char *name, int position)
{
	struct fimc_is_core *core;
	struct fimc_is_vender_specific *specific = NULL;
	struct fimc_is_device_sensor *device;
	struct fimc_is_module_enum *module;
	int sensor_id = 0;
	int i = 0;

	if (!fimc_is_dev) {
		dev_err(dev, "%s: fimc_is_dev is not yet probed", __func__);
		return -ENODEV;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		dev_err(dev, "%s: core is NULL", __func__);
		return -EINVAL;
	}

	specific = core->vender.private_data;

	if (position == SENSOR_POSITION_REAR) {
		sensor_id = specific->rear_sensor_id;
	} else if(position == SENSOR_POSITION_FRONT) {
		sensor_id = specific->front_sensor_id;
	}
#ifdef CONFIG_SECURE_CAMERA_USE
	else if(position == SENSOR_POSITION_SECURE) {
		sensor_id = specific->secure_sensor_id;
	}
#endif
	else {
		dev_err(dev, "%s: position value is wrong", __func__);
		return -EINVAL;
	}

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		fimc_is_search_sensor_module(&core->sensor[i], sensor_id, &module);
		if (module)
			break;
	}

	if(module) {
		if (maker != NULL)
			sprintf(maker, "%s", module->sensor_maker ?
					module->sensor_maker : "UNKNOWN");
		if (name != NULL)
			sprintf(name, "%s", module->sensor_name ?
					module->sensor_name : "UNKNOWN");
		return 0;
	}

	dev_err(dev, "%s: there's no matched sensor id", __func__);

	return -ENODEV;
}

static ssize_t camera_front_camtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char sensor_maker[50];
	char sensor_name[50];
	int ret;

	ret = fimc_is_get_sensor_data(dev, sensor_maker, sensor_name, SENSOR_POSITION_FRONT);

	if (ret < 0)
		return sprintf(buf, "UNKNOWN_UNKNOWN_FIMC_IS\n");
	else
		return sprintf(buf, "%s_%s_FIMC_IS\n", sensor_maker, sensor_name);
}

static ssize_t camera_front_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
	char command_ack[20] = {0, };

	fimc_is_vender_check_hw_init_running();

	if (force_caldata_dump || !front_finfo->is_check_cal_reload) {
		struct fimc_is_vender_specific *specific = sysfs_core->vender.private_data;

		if (specific->running_front_camera == false)
			read_from_firmware_version_reload(SENSOR_POSITION_FRONT);
	} else {
		read_from_firmware_version(SENSOR_POSITION_FRONT);
	}

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_FRONT)) {
		err(" NG, invalid FROM version");
		strcpy(command_ack, "NG_CD3");
		return sprintf(buf, "%s %s\n", front_finfo->header_ver, command_ack);
	}

	if (crc32_check_factory_front) {
		return sprintf(buf, "%s %s\n", front_finfo->header_ver, front_finfo->header_ver);
	} else {
		err(" NG, crc check fail");
		strcpy(command_ack, "NG_");
		if (!crc32_check_factory_front)
			strcat(command_ack, "CD3");
		if (front_finfo->header_ver[3] != 'L')
			strcat(command_ack, "_Q");
		return sprintf(buf, "%s %s\n", front_finfo->header_ver, command_ack);
	}
#else
	char sensor_name[50];
	int ret;

	ret = fimc_is_get_sensor_data(dev, NULL, sensor_name, SENSOR_POSITION_FRONT);

	if (ret < 0)
		return sprintf(buf, "UNKNOWN UNKNOWN\n");
	else
		return sprintf(buf, "%s N\n", sensor_name);
#endif
}

#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
static ssize_t camera_front_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char command_ack[20] = {0, };

	read_from_firmware_version(SENSOR_POSITION_FRONT);

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_FRONT)) {
		err(" NG, invalid FROM version");
		strcpy(command_ack, "NG_CD3");
		return sprintf(buf, "%s %s %s\n", front_finfo->header_ver, "N", command_ack);
	}

	if (crc32_check_factory_front) {
		return sprintf(buf, "%s %s %s\n", front_finfo->header_ver, "N", front_finfo->header_ver);
	} else {
		err(" NG, crc check fail");
		strcpy(command_ack, "NG_");
		if (!crc32_check_factory_front)
			strcat(command_ack, "CD3");
		if (front_finfo->header_ver[3] != 'L')
			strcat(command_ack, "_Q");
		return sprintf(buf, "%s %s %s\n", front_finfo->header_ver, "N", command_ack);
	}
}

static ssize_t camera_front_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char command_ack[10] = {0, };

	read_from_firmware_version(SENSOR_POSITION_FRONT);

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_FRONT)) {
		err(" NG, invalid FROM version");
#ifdef CAMERA_SYSFS_V2
		return sprintf(buf, "%s\n", "NG_VER");
#else
		return sprintf(buf, "%s\n", "NG");
#endif
	}

	if (crc32_check_factory_front) {
		if (!is_final_cam_module_front) {
			err(" NG, not final cam module");
#ifdef CAMERA_SYSFS_V2
			strcpy(command_ack, "NG_VER\n");
#else
			strcpy(command_ack, "NG\n");
#endif
		} else {
			strcpy(command_ack, "OK\n");
		}
	} else {
		err(" NG, crc check fail");
#ifdef CAMERA_SYSFS_V2
		strcpy(command_ack, "NG_CRC\n");
#else
		strcpy(command_ack, "NG\n");
#endif
	}
	return sprintf(buf, "%s", command_ack);
}
#endif

#ifdef CAMERA_SYSFS_V2
static ssize_t camera_front_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char camera_info[130] = {0, };
#ifdef CONFIG_OF
	struct fimc_is_cam_info *cam_info = &(cam_infos[CAM_INFO_FRONT]);
	if(!cam_info->valid) {
		strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N");

		return sprintf(buf, "%s\n", camera_info);
	} else {
		strcpy(camera_info, "ISP=");
		switch(cam_info->isp) {
			case CAM_INFO_ISP_TYPE_INTERNAL :
				strncat(camera_info, "INT;", strlen("INT;"));
				break;
			case CAM_INFO_ISP_TYPE_EXTERNAL :
				strncat(camera_info, "EXT;", strlen("EXT;"));
				break;
			case CAM_INFO_ISP_TYPE_SOC :
				strncat(camera_info, "SOC;", strlen("SOC;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "CALMEM=", strlen("CALMEM="));
		switch(cam_info->cal_memory) {
			case CAM_INFO_CAL_MEM_TYPE_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_CAL_MEM_TYPE_FROM :
			case CAM_INFO_CAL_MEM_TYPE_EEPROM :
			case CAM_INFO_CAL_MEM_TYPE_OTP :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "READVER=", strlen("READVER="));
		switch(cam_info->read_version) {
			case CAM_INFO_READ_VER_SYSFS :
				strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
				break;
			case CAM_INFO_READ_VER_CAMON :
				strncat(camera_info, "CAMON;", strlen("CAMON;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "COREVOLT=", strlen("COREVOLT="));
		switch(cam_info->core_voltage) {
			case CAM_INFO_CORE_VOLT_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_CORE_VOLT_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "UPGRADE=", strlen("UPGRADE="));
		switch(cam_info->upgrade) {
			case CAM_INFO_FW_UPGRADE_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_UPGRADE_SYSFS :
				strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
				break;
			case CAM_INFO_FW_UPGRADE_CAMON :
				strncat(camera_info, "CAMON;", strlen("CAMON;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "FWWRITE=", strlen("FWWRITE="));
		switch(cam_info->fw_write) {
			case CAM_INFO_FW_WRITE_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_WRITE_OS :
				strncat(camera_info, "OS;", strlen("OS;"));
				break;
			case CAM_INFO_FW_WRITE_SD :
				strncat(camera_info, "SD;", strlen("SD;"));
				break;
			case CAM_INFO_FW_WRITE_ALL :
				strncat(camera_info, "ALL;", strlen("ALL;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "FWDUMP=", strlen("FWDUMP="));
		switch(cam_info->fw_dump) {
			case CAM_INFO_FW_DUMP_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_DUMP_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "CC=", strlen("CC="));
		switch(cam_info->companion) {
			case CAM_INFO_COMPANION_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_COMPANION_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "OIS=", strlen("OIS="));
		switch(cam_info->ois) {
			case CAM_INFO_OIS_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_OIS_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "VALID=", strlen("VALID="));
		switch(cam_info->valid) {
			case CAM_INFO_INVALID :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_VALID :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		return sprintf(buf, "%s\n", camera_info);
	}
#endif
	strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N");

	return sprintf(buf, "%s\n", camera_info);
}
#endif

static DEVICE_ATTR(front_camtype, S_IRUGO,
		camera_front_camtype_show, NULL);
#ifdef CAMERA_SYSFS_V2
static DEVICE_ATTR(front_caminfo, S_IRUGO,
		camera_front_info_show, NULL);
#endif
static DEVICE_ATTR(front_camfw, S_IRUGO, camera_front_camfw_show, NULL);
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
static DEVICE_ATTR(front_camfw_full, S_IRUGO, camera_front_camfw_full_show, NULL);
static DEVICE_ATTR(front_checkfw_factory, S_IRUGO, camera_front_checkfw_factory_show, NULL);
#endif

#ifdef CAMERA_MODULE_DUALIZE
static ssize_t camera_rear_writefw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
/* Disabled for product. Security Hole */
#if 0
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;
	int ret = 0;

	ret = fimc_is_sec_write_fw(sysfs_core, is_dev);

	if (ret)
		return sprintf(buf, "NG\n");
	else
		return sprintf(buf, "OK\n");
#else
	return sprintf(buf, "OK\n");
#endif
}
#endif

static ssize_t camera_rear_camtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char sensor_maker[50];
	char sensor_name[50];
	int ret;

	ret = fimc_is_get_sensor_data(dev, sensor_maker, sensor_name, SENSOR_POSITION_REAR);

	if (ret < 0)
		return sprintf(buf, "UNKNOWN_UNKNOWN_FIMC_IS\n");
	else
		return sprintf(buf, "%s_%s_FIMC_IS\n", sensor_maker, sensor_name);
}

static ssize_t camera_rear_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char command_ack[20] = {0, };
	char *loaded_fw;
	struct exynos_platform_fimc_is *core_pdata = NULL;

	core_pdata = dev_get_platdata(fimc_is_dev);

	fimc_is_vender_check_hw_init_running();

	if (force_caldata_dump || !finfo->is_check_cal_reload) {
		struct fimc_is_vender_specific *specific = sysfs_core->vender.private_data;

		if (specific->running_rear_camera == false)
			read_from_firmware_version_reload(SENSOR_POSITION_REAR);
	} else {
		read_from_firmware_version(SENSOR_POSITION_REAR);
	}

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR)) {
		err(" NG, invalid FROM version");
#ifdef CAMERA_REAR2
		strcpy(command_ack, "NG_FWCDFW1CD1CD4");
#else
		strcpy(command_ack, "NG_FWCDFW1CD1");
#endif
		return sprintf(buf, "%s %s\n", "NULL", command_ack);
	}

#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_REAR)
	loaded_fw = pinfo->header_ver;
#else
	fimc_is_sec_get_loaded_fw(&loaded_fw);
#endif

	if (fw_version_crc_check) {
		if (crc32_fw_check && crc32_check_factory && crc32_setfile_check
			&& crc32_front_setfile_check
#ifdef CONFIG_COMPANION_USE
			&& crc32_c1_fw_check && crc32_c1_check_factory
#endif
			&& crc32_check_factory_rear2
			&& crc32_hifi_tunningfile_check
		) {
			return sprintf(buf, "%s %s\n", finfo->header_ver, loaded_fw);
		} else {
			err(" NG, crc check fail");
			strcpy(command_ack, "NG_");
			if (!crc32_fw_check)
				strcat(command_ack, "FW");
			if (!crc32_check_factory)
				strcat(command_ack, "CD");
			if (!crc32_setfile_check || !crc32_front_setfile_check)
				strcat(command_ack, "SET");
#ifdef CONFIG_COMPANION_USE
			if (!crc32_c1_fw_check)
				strcat(command_ack, "FW1");
			if (!crc32_c1_check_factory)
				strcat(command_ack, "CD1");
#endif
			if (!crc32_check_factory_rear2)
				strcat(command_ack, "CD4");
			if (finfo->header_ver[3] != 'L')
				strcat(command_ack, "_Q");
			return sprintf(buf, "%s %s\n", finfo->header_ver, command_ack);
		}
	} else {
		err(" NG, fw ver crc check fail");
		strcpy(command_ack, "NG_");
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_REAR)
		strcat(command_ack, "CD");
#else
		strcat(command_ack, "FWCD");
#endif
#ifdef CONFIG_COMPANION_USE
		strcat(command_ack, "FW1CD1");
#endif
		return sprintf(buf, "%s %s\n", finfo->header_ver, command_ack);
	}
}

static ssize_t camera_rear_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char command_ack[20] = {0, };
	char *loaded_fw;

	read_from_firmware_version(SENSOR_POSITION_REAR);

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR)) {
		err(" NG, invalid FROM version");
		strcpy(command_ack, "NG_FWCDFW1CD1");
		return sprintf(buf, "%s %s %s\n", "NULL", "NULL", command_ack);
	}

#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_REAR)
	loaded_fw = pinfo->header_ver;
#else
	fimc_is_sec_get_loaded_fw(&loaded_fw);
#endif

	if (fw_version_crc_check) {
		if (crc32_fw_check && crc32_check_factory && crc32_setfile_check
			&& crc32_front_setfile_check
#ifdef CONFIG_COMPANION_USE
			&& crc32_c1_fw_check && crc32_c1_check_factory
#endif
			&& crc32_check_factory_rear2
			&& crc32_hifi_tunningfile_check
		) {
			return sprintf(buf, "%s %s %s\n", finfo->header_ver, pinfo->header_ver, loaded_fw);
		} else {
			err(" NG, crc check fail");
			strcpy(command_ack, "NG_");
			if (!crc32_fw_check)
				strcat(command_ack, "FW");
			if (!crc32_check_factory)
				strcat(command_ack, "CD");
			if (!crc32_setfile_check || !crc32_front_setfile_check)
				strcat(command_ack, "SET");
#ifdef CONFIG_COMPANION_USE
			if (!crc32_c1_fw_check)
				strcat(command_ack, "FW1");
			if (!crc32_c1_check_factory)
				strcat(command_ack, "CD1");
#endif
			if (!crc32_check_factory_rear2)
				strcat(command_ack, "CD4");
			if (finfo->header_ver[3] != 'L')
				strcat(command_ack, "_Q");
			return sprintf(buf, "%s %s %s\n", finfo->header_ver, pinfo->header_ver, command_ack);
		}
	} else {
		err(" NG, fw ver crc check fail");
		strcpy(command_ack, "NG_");
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_REAR)
		strcat(command_ack, "CD");
#else
		strcat(command_ack, "FWCD");
#endif
#ifdef CONFIG_COMPANION_USE
		strcat(command_ack, "FW1CD1");
#endif
		return sprintf(buf, "%s %s %s\n", finfo->header_ver, pinfo->header_ver, command_ack);
	}
}

static ssize_t camera_rear_checkfw_user_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;

	read_from_firmware_version(SENSOR_POSITION_REAR);

	/* 2T2 Sensor Only. Can be removed after development */
	fimc_is_sec_get_cal_buf(&cal_buf);
	if (((finfo->header_ver[0] == 'A') && (finfo->header_ver[1] == '2') && (finfo->header_ver[2] == '0'))
		   && (cal_buf[0xC0] < 0xC0)) {
		err(" NG, old sensor revision");
		return sprintf(buf, "%s\n", "NG");
	}

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR)) {
		err(" NG, invalid FROM version");
		return sprintf(buf, "%s\n", "NG");
	}

	if (fw_version_crc_check) {
		if (crc32_fw_check && crc32_check_factory
#ifdef CONFIG_COMPANION_USE
		    && crc32_c1_fw_check && crc32_c1_check_factory
#endif
			&& crc32_check_factory_rear2
		) {
			if (!is_latest_cam_module) {
				err(" NG, not latest cam module");
				return sprintf(buf, "%s\n", "NG");
			} else {
				return sprintf(buf, "%s\n", "OK");
			}
		} else {
			err(" NG, crc check fail");
			return sprintf(buf, "%s\n", "NG");
		}
	} else {
		err(" NG, fw ver crc check fail");
		return sprintf(buf, "%s\n", "NG");
	}
}

static ssize_t camera_rear_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char command_ack[10] = {0, };
#if defined (CONFIG_OIS_USE)
	struct fimc_is_ois_info *ois_minfo = NULL;
	bool ois_ret = false;

	ois_ret = read_ois_version();
	fimc_is_ois_get_module_version(&ois_minfo);
#endif

	read_from_firmware_version(SENSOR_POSITION_REAR);

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR)) {
		err(" NG, invalid FROM version");
#ifdef CAMERA_SYSFS_V2
		return sprintf(buf, "%s\n", "NG_VER");
#else
		return sprintf(buf, "%s\n", "NG");
#endif
	}

	if (fw_version_crc_check) {
		if (crc32_fw_check && crc32_check_factory && crc32_setfile_check
			&& crc32_front_setfile_check
#ifdef CONFIG_COMPANION_USE
		    && crc32_c1_fw_check && crc32_c1_check_factory
#endif
			&& crc32_check_factory_rear2
			&& crc32_hifi_tunningfile_check
		) {
			if (!is_final_cam_module) {
				err(" NG, not final cam module");
#ifdef CAMERA_SYSFS_V2
				strcpy(command_ack, "NG_VER\n");
#else
				strcpy(command_ack, "NG\n");
#endif
			} else {
#if defined (CONFIG_OIS_USE)
				if (ois_minfo->checksum != 0x00 || ois_minfo->caldata != 0x00 || !ois_ret) {
					err(" NG, OIS crc check fail, checksum = 0x%02x, caldata = 0x%02x, ois_ret = %d",
						ois_minfo->checksum, ois_minfo->caldata, ois_ret);
#ifdef CAMERA_SYSFS_V2
					strcpy(command_ack, "NG_CRC\n");
#else
					strcpy(command_ack, "NG\n");
#endif
				} else {
					strcpy(command_ack, "OK\n");
				}
#else
				strcpy(command_ack, "OK\n");
#endif
			}
		} else {
			err(" NG, crc check fail");
#ifdef CAMERA_SYSFS_V2
			strcpy(command_ack, "NG_CRC\n");
#else
			strcpy(command_ack, "NG\n");
#endif
		}
	} else {
		err(" NG, fw ver crc check fail");
#ifdef CAMERA_SYSFS_V2
		strcpy(command_ack, "NG_VER\n");
#else
		strcpy(command_ack, "NG\n");
#endif
	}

	return sprintf(buf, "%s", command_ack);
}
#ifdef CAMERA_SYSFS_V2
static ssize_t camera_rear_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char camera_info[130] = {0, };
#ifdef CONFIG_OF
	struct fimc_is_cam_info *cam_info = &(cam_infos[CAM_INFO_REAR]);

	if(!cam_info->valid) {
		strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N");

		return sprintf(buf, "%s\n", camera_info);
	} else {
		strcpy(camera_info, "ISP=");
		switch(cam_info->isp) {
			case CAM_INFO_ISP_TYPE_INTERNAL :
				strncat(camera_info, "INT;", strlen("INT;"));
				break;
			case CAM_INFO_ISP_TYPE_EXTERNAL :
				strncat(camera_info, "EXT;", strlen("EXT;"));
				break;
			case CAM_INFO_ISP_TYPE_SOC :
				strncat(camera_info, "SOC;", strlen("SOC;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "CALMEM=", strlen("CALMEM="));
		switch(cam_info->cal_memory) {
			case CAM_INFO_CAL_MEM_TYPE_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_CAL_MEM_TYPE_FROM :
			case CAM_INFO_CAL_MEM_TYPE_EEPROM :
			case CAM_INFO_CAL_MEM_TYPE_OTP :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "READVER=", strlen("READVER="));
		switch(cam_info->read_version) {
			case CAM_INFO_READ_VER_SYSFS :
				strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
				break;
			case CAM_INFO_READ_VER_CAMON :
				strncat(camera_info, "CAMON;", strlen("CAMON;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "COREVOLT=", strlen("COREVOLT="));
		switch(cam_info->core_voltage) {
			case CAM_INFO_CORE_VOLT_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_CORE_VOLT_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "UPGRADE=", strlen("UPGRADE="));
		switch(cam_info->upgrade) {
			case CAM_INFO_FW_UPGRADE_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_UPGRADE_SYSFS :
				strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
				break;
			case CAM_INFO_FW_UPGRADE_CAMON :
				strncat(camera_info, "CAMON;", strlen("CAMON;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "FWWRITE=", strlen("FWWRITE="));
		switch(cam_info->fw_write) {
			case CAM_INFO_FW_WRITE_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_WRITE_OS :
				strncat(camera_info, "OS;", strlen("OS;"));
				break;
			case CAM_INFO_FW_WRITE_SD :
				strncat(camera_info, "SD;", strlen("SD;"));
				break;
			case CAM_INFO_FW_WRITE_ALL :
				strncat(camera_info, "ALL;", strlen("ALL;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "FWDUMP=", strlen("FWDUMP="));
		switch(cam_info->fw_dump) {
			case CAM_INFO_FW_DUMP_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_DUMP_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "CC=", strlen("CC="));
		switch(cam_info->companion) {
			case CAM_INFO_COMPANION_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_COMPANION_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "OIS=", strlen("OIS="));
		switch(cam_info->ois) {
			case CAM_INFO_OIS_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_OIS_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "VALID=", strlen("VALID="));
		switch(cam_info->valid) {
			case CAM_INFO_INVALID :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_VALID :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		return sprintf(buf, "%s\n", camera_info);
	}
#endif
	strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N");

	return sprintf(buf, "%s\n", camera_info);
}
#endif

#ifdef CAMERA_REAR2
static ssize_t camera_rear2_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->rear_second_sensor_id);
}

static ssize_t camera_rear2_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char command_ack[20] = {0, };
	struct exynos_platform_fimc_is *core_pdata = NULL;

	core_pdata = dev_get_platdata(fimc_is_dev);

	fimc_is_vender_check_hw_init_running();
	read_from_firmware_version(SENSOR_POSITION_REAR);

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR)) {
		err(" NG, invalid FROM version");
		strcpy(command_ack, "NG_FWCDFW1CD1CD4");
		return sprintf(buf, "%s %s\n", "NULL", command_ack);
	}

	if (fw_version_crc_check) {
		if (crc32_check_factory_rear2) {
			return sprintf(buf, "%s %s\n", finfo->header2_ver, finfo->header2_ver);
		} else {
			err(" NG, crc check fail");
			strcpy(command_ack, "NG_CD4");
			return sprintf(buf, "%s %s\n", finfo->header2_ver, command_ack);
		}
	} else {
		err(" NG, fw ver crc check fail");
		strcpy(command_ack, "NG_CD4");
		return sprintf(buf, "%s %s\n", finfo->header2_ver, command_ack);
	}
}

static ssize_t camera_rear2_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char command_ack[20] = {0, };

	read_from_firmware_version(SENSOR_POSITION_REAR);

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR)) {
		err(" NG, invalid FROM version");
		strcpy(command_ack, "NG_FWCDFW1CD1CD4");
		return sprintf(buf, "%s %s %s\n", "NULL", "NULL", command_ack);
	}

	if (fw_version_crc_check) {
		if (crc32_check_factory_rear2) {
			return sprintf(buf, "%s %s %s\n", finfo->header2_ver, "N", finfo->header2_ver);
		} else {
			err(" NG, crc check fail");
			strcpy(command_ack, "NG_CD4");
			return sprintf(buf, "%s %s %s\n", finfo->header2_ver, "N", command_ack);
		}
	} else {
		strcpy(command_ack, "NG_CD4");
		return sprintf(buf, "%s %s %s\n", finfo->header2_ver, "N", command_ack);
	}
}

static ssize_t camera_rear2_checkfw_user_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	read_from_firmware_version(SENSOR_POSITION_REAR);

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR)) {
		err(" NG, invalid FROM version");
		return sprintf(buf, "%s\n", "NG");
	}

	if (fw_version_crc_check) {
		if (crc32_check_factory_rear2) {
			if (!is_latest_cam_module) {
				err(" NG, not latest cam module");
				return sprintf(buf, "%s\n", "NG");
			} else {
				return sprintf(buf, "%s\n", "OK");
			}
		} else {
			err(" NG, crc check fail");
			return sprintf(buf, "%s\n", "NG");
		}
	} else {
		err(" NG, fw ver crc check fail");
		return sprintf(buf, "%s\n", "NG");
	}
}

static ssize_t camera_rear2_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char command_ack[10] = {0, };

	read_from_firmware_version(SENSOR_POSITION_REAR);

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR)) {
		err(" NG, invalid FROM version");
#ifdef CAMERA_SYSFS_V2
		return sprintf(buf, "%s\n", "NG_VER");
#else
		return sprintf(buf, "%s\n", "NG");
#endif
	}

	if (fw_version_crc_check) {
		if (crc32_check_factory_rear2) {
			if (!is_final_cam_module) {
				err(" NG, not final cam module");
#ifdef CAMERA_SYSFS_V2
				strcpy(command_ack, "NG_VER\n");
#else
				strcpy(command_ack, "NG\n");
#endif
			} else {
				strcpy(command_ack, "OK\n");
			}
		} else {
			err(" NG, crc check fail");
#ifdef CAMERA_SYSFS_V2
			strcpy(command_ack, "NG_CRC\n");
#else
			strcpy(command_ack, "NG\n");
#endif
		}
	} else {
		err(" NG, fw ver crc check fail");
#ifdef CAMERA_SYSFS_V2
		strcpy(command_ack, "NG_VER\n");
#else
		strcpy(command_ack, "NG\n");
#endif
	}

	return sprintf(buf, "%s", command_ack);
}
#ifdef CAMERA_SYSFS_V2
static ssize_t camera_rear2_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char camera_info[130] = {0, };
#ifdef CONFIG_OF
	struct fimc_is_cam_info *cam_info = &(cam_infos[CAM_INFO_REAR2]);

	if(!cam_info->valid) {
		strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N;DUALOPEN=N");

		return sprintf(buf, "%s\n", camera_info);
	} else {
		strcpy(camera_info, "ISP=");
		switch (cam_info->isp) {
		case CAM_INFO_ISP_TYPE_INTERNAL:
			strncat(camera_info, "INT;", strlen("INT;"));
			break;
		case CAM_INFO_ISP_TYPE_EXTERNAL:
			strncat(camera_info, "EXT;", strlen("EXT;"));
			break;
		case CAM_INFO_ISP_TYPE_SOC:
			strncat(camera_info, "SOC;", strlen("SOC;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "CALMEM=", strlen("CALMEM="));
		switch (cam_info->cal_memory) {
		case CAM_INFO_CAL_MEM_TYPE_NONE:
			strncat(camera_info, "N;", strlen("N;"));
			break;
		case CAM_INFO_CAL_MEM_TYPE_FROM:
		case CAM_INFO_CAL_MEM_TYPE_EEPROM:
		case CAM_INFO_CAL_MEM_TYPE_OTP:
			strncat(camera_info, "Y;", strlen("Y;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "READVER=", strlen("READVER="));
		switch (cam_info->read_version) {
		case CAM_INFO_READ_VER_SYSFS:
			strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
			break;
		case CAM_INFO_READ_VER_CAMON:
			strncat(camera_info, "CAMON;", strlen("CAMON;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "COREVOLT=", strlen("COREVOLT="));
		switch (cam_info->core_voltage) {
		case CAM_INFO_CORE_VOLT_NONE:
			strncat(camera_info, "N;", strlen("N;"));
			break;
		case CAM_INFO_CORE_VOLT_USE:
			strncat(camera_info, "Y;", strlen("Y;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "UPGRADE=", strlen("UPGRADE="));
		switch (cam_info->upgrade) {
		case CAM_INFO_FW_UPGRADE_NONE:
			strncat(camera_info, "N;", strlen("N;"));
			break;
		case CAM_INFO_FW_UPGRADE_SYSFS:
			strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
			break;
		case CAM_INFO_FW_UPGRADE_CAMON:
			strncat(camera_info, "CAMON;", strlen("CAMON;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "FWWRITE=", strlen("FWWRITE="));
		switch (cam_info->fw_write) {
		case CAM_INFO_FW_WRITE_NONE:
			strncat(camera_info, "N;", strlen("N;"));
			break;
		case CAM_INFO_FW_WRITE_OS:
			strncat(camera_info, "OS;", strlen("OS;"));
			break;
		case CAM_INFO_FW_WRITE_SD:
			strncat(camera_info, "SD;", strlen("SD;"));
			break;
		case CAM_INFO_FW_WRITE_ALL:
			strncat(camera_info, "ALL;", strlen("ALL;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "FWDUMP=", strlen("FWDUMP="));
		switch (cam_info->fw_dump) {
		case CAM_INFO_FW_DUMP_NONE:
			strncat(camera_info, "N;", strlen("N;"));
			break;
		case CAM_INFO_FW_DUMP_USE:
			strncat(camera_info, "Y;", strlen("Y;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "CC=", strlen("CC="));
		switch (cam_info->companion) {
		case CAM_INFO_COMPANION_NONE:
			strncat(camera_info, "N;", strlen("N;"));
			break;
		case CAM_INFO_COMPANION_USE:
			strncat(camera_info, "Y;", strlen("Y;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "OIS=", strlen("OIS="));
		switch (cam_info->ois) {
		case CAM_INFO_OIS_NONE:
			strncat(camera_info, "N;", strlen("N;"));
			break;
		case CAM_INFO_OIS_USE:
			strncat(camera_info, "Y;", strlen("Y;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "VALID=", strlen("VALID="));
		switch (cam_info->valid) {
		case CAM_INFO_INVALID:
			strncat(camera_info, "N;", strlen("N;"));
			break;
		case CAM_INFO_VALID:
			strncat(camera_info, "Y;", strlen("Y;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		strncat(camera_info, "DUALOPEN=", strlen("DUALOPEN="));
		switch (cam_info->dual_open) {
		case CAM_INFO_SINGLE_OPEN:
			strncat(camera_info, "N;", strlen("N;"));
			break;
		case CAM_INFO_DUAL_OPEN:
			strncat(camera_info, "Y;", strlen("Y;"));
			break;
		default:
			strncat(camera_info, "NULL;", strlen("NULL;"));
			break;
		}

		return sprintf(buf, "%s\n", camera_info);
	}
#endif
	strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N;DUALOPEN=N");

	return sprintf(buf, "%s\n", camera_info);
}
#endif

static ssize_t camera_rear2_afcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char tempbuf[10];
	char *cal_buf;
	char N[] = "N ";

	read_from_firmware_version(SENSOR_POSITION_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf);

	strncat(buf, "10 ", strlen("10 "));

#ifdef FROM_AF2_CAL_D10_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF2_CAL_D10_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF2_CAL_D20_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF2_CAL_D20_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF2_CAL_D30_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF2_CAL_D30_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF2_CAL_D40_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF2_CAL_D40_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF2_CAL_D50_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF2_CAL_D50_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF2_CAL_D60_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF2_CAL_D60_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF2_CAL_D70_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF2_CAL_D70_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF2_CAL_D80_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF2_CAL_D80_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF2_CAL_PAN_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF2_CAL_PAN_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

	return strlen(buf);
}

static ssize_t camera_rear2_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	read_from_firmware_version(SENSOR_POSITION_REAR);

	return sprintf(buf, "%s", finfo->from_sensor2_id);
}

static ssize_t camera_rear2_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;

	read_from_firmware_version(SENSOR_POSITION_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf);

	memcpy(buf, &cal_buf[finfo->mtf_data2_addr], FIMC_IS_RESOLUTION_DATA_SIZE);
	return FIMC_IS_RESOLUTION_DATA_SIZE;
}

static ssize_t camera_rear_dualcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;

	read_from_firmware_version(SENSOR_POSITION_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf);

	memcpy(buf, &cal_buf[FROM_REAR2_DUAL_CAL2], FROM_REAR2_DUAL_CAL2_SIZE);
	return FROM_REAR2_DUAL_CAL2_SIZE;
}

static ssize_t camera_rear_dualcal_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", FROM_REAR2_DUAL_CAL2_SIZE);
}

static ssize_t camera_rear2_tilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;

	read_from_firmware_version(SENSOR_POSITION_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf);

	if (!fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR)) {
		err(" NG, invalid FROM version");
		return sprintf(buf, "%s\n", "NG");
	}
#ifdef FROM_REAR2_DUAL_TILT_X
	if (fw_version_crc_check && crc32_check_factory_rear2) {
		s32 *x = NULL, *y = NULL, *z = NULL, *sx = NULL, *sy = NULL;
		s32 *range = NULL, *max_err = NULL, *avg_err = NULL, *dll_version = NULL;

		x = (s32 *)&cal_buf[FROM_REAR2_DUAL_TILT_X];
		y = (s32 *)&cal_buf[FROM_REAR2_DUAL_TILT_Y];
		z = (s32 *)&cal_buf[FROM_REAR2_DUAL_TILT_Z];
		sx = (s32 *)&cal_buf[FROM_REAR2_DUAL_TILT_SX];
		sy = (s32 *)&cal_buf[FROM_REAR2_DUAL_TILT_SY];
		range = (s32 *)&cal_buf[FROM_REAR2_DUAL_TILT_RANGE];
		max_err = (s32 *)&cal_buf[FROM_REAR2_DUAL_TILT_MAX_ERR];
		avg_err = (s32 *)&cal_buf[FROM_REAR2_DUAL_TILT_AVG_ERR];
		dll_version = (s32 *)&cal_buf[FROM_REAR2_DUAL_TILT_DLL_VERSION];

		return sprintf(buf, "1 %d %d %d %d %d %d %d %d %d\n",
							*x, *y, *z, *sx, *sy, *range, *max_err, *avg_err, *dll_version);
	} else {
		return sprintf(buf, "%s\n", "NG");
	}
#else
	return sprintf(buf, "%s\n", "NG");
#endif
}
#endif

static ssize_t camera_rear_sensor_standby(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	switch (buf[0]) {
	case '0':
		break;
	case '1':
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}

	return count;
}

static ssize_t camera_rear_sensor_standby_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Rear sensor standby \n");
}

#ifdef CONFIG_COMPANION_USE
static ssize_t camera_rear_companionfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *loaded_c1_fw;

	fimc_is_vender_check_hw_init_running();
	read_from_firmware_version(SENSOR_POSITION_REAR);
	fimc_is_sec_get_loaded_c1_fw(&loaded_c1_fw);

	return sprintf(buf, "%s %s\n",
		finfo->concord_header_ver, loaded_c1_fw);
}

static ssize_t camera_rear_companionfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *loaded_c1_fw;

	read_from_firmware_version(SENSOR_POSITION_REAR);
	fimc_is_sec_get_loaded_c1_fw(&loaded_c1_fw);

	return sprintf(buf, "%s %s %s\n",
		finfo->concord_header_ver, pinfo->concord_header_ver, loaded_c1_fw);
}
#endif

static ssize_t camera_rear_camfw_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = -EINVAL;

	if ((size == 1 || size == 2) && (buf[0] == 'F' || buf[0] == 'f')) {
		fimc_is_sec_set_force_caldata_dump(true);
		ret = size;
	} else {
		fimc_is_sec_set_force_caldata_dump(false);
	}
	return ret;
}

static ssize_t camera_rear_calcheck_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char rear_sensor[10] = {0, };
#ifdef CONFIG_COMPANION_USE
	char rear_companion[10] = {0, };
#endif
#ifdef CONFIG_CAMERA_EEPROM_SUPPORT_FRONT
	char front_sensor[10] = {0, };
#endif

	read_from_firmware_version(SENSOR_POSITION_REAR);

	if (fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR) && crc32_check_factory)
		strcpy(rear_sensor, "Normal");
	else
		strcpy(rear_sensor, "Abnormal");

#ifdef CONFIG_CAMERA_EEPROM_SUPPORT_FRONT
	read_from_firmware_version(SENSOR_POSITION_FRONT);
#ifdef CONFIG_COMPANION_USE
	if (fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR) && crc32_c1_check_factory)
		strcpy(rear_companion, "Normal");
	else
		strcpy(rear_companion, "Abnormal");

	if (fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_FRONT) && crc32_check_factory_front)
		strcpy(front_sensor, "Normal");
	else
		strcpy(front_sensor, "Abnormal");

	return sprintf(buf, "%s %s %s\n", rear_sensor, rear_companion, front_sensor);
#else
	return sprintf(buf, "%s %s\n", rear_sensor, front_sensor);
#endif
#else
#ifdef CONFIG_COMPANION_USE
	if (fimc_is_sec_check_from_ver(sysfs_core, SENSOR_POSITION_REAR) && crc32_c1_check_factory)
		strcpy(rear_companion, "Normal");
	else
		strcpy(rear_companion, "Abnormal");

	return sprintf(buf, "%s %s %s\n", rear_sensor, rear_companion, "Null");
#else
	return sprintf(buf, "%s %s\n", rear_sensor, "Null");
#endif
#endif
}

#ifdef CONFIG_COMPANION_USE
static ssize_t camera_isp_core_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
#ifdef CONFIG_COMPANION_DCDC_USE
	int sel;
	struct fimc_is_vender_specific *specific = sysfs_core->vender.private_data;

	if (DCDC_VENDOR_NONE == specific->companion_dcdc.type)
		return sprintf(buf, "none\n");

	sel = fimc_is_power_binning(sysfs_core);
	return sprintf(buf, "%s\n", specific->companion_dcdc.get_vout_str(sel));
#else
	return sprintf(buf, "none\n");
#endif
}
#endif

static ssize_t camera_hw_init_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_vender *vender;
	int i;

	vender = &sysfs_core->vender;

	if (!check_module_init
		|| gPtr_lib_support.binary_code_load_flg != BINARY_LOAD_ALL_DONE) {
		fimc_is_vender_hw_init(vender);
		check_module_init = true;
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
			SsrmCameraInfo[i].cameraID = -1;
		}
	}

	return sprintf(buf, "%s\n", "HW init done.");
}

#if defined (CONFIG_OIS_USE)
static ssize_t camera_ois_power_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)

{
	struct fimc_is_vender_specific *specific = sysfs_core->vender.private_data;

	fimc_is_vender_check_hw_init_running();

	switch (buf[0]) {
	case '0':
		if (!specific->running_rear_camera) {
			fimc_is_ois_gpio_off(sysfs_core);
		} else {
			err("cannot control OIS PowerOff  because camera is working.\n");
		}
		check_ois_power = false;
		break;
	case '1':
		fimc_is_ois_gpio_on(sysfs_core);
		check_ois_power = true;
		msleep(150);
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}
	return count;
}

static ssize_t camera_ois_autotest_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)

{
	int value = 0;

	if (kstrtoint(buf, 10, &value)) {
		err("convert fail");
	}

	ois_threshold = value;

	return count;
}

static ssize_t camera_ois_autotest_show(struct device *dev,
				    struct device_attribute *attr, char *buf)

{
	bool x_result = false, y_result = false;
	int sin_x = 0, sin_y = 0;

	if (check_ois_power) {
		fimc_is_ois_auto_test(sysfs_core, ois_threshold,
			&x_result, &y_result, &sin_x, &sin_y);

		if (x_result && y_result) {
			return sprintf(buf, "%s,%d,%s,%d\n", "pass", 0, "pass", 0);
		} else if (x_result) {
			return sprintf(buf, "%s,%d,%s,%d\n", "pass", 0, "fail", sin_y);
		} else if (y_result) {
			return sprintf(buf, "%s,%d,%s,%d\n", "fail", sin_x, "pass", 0);
		} else {
			return sprintf(buf, "%s,%d,%s,%d\n", "fail", sin_x, "fail", sin_y);
		}
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%s,%d,%s,%d\n", "fail", sin_x, "fail", sin_y);
	}
}

#ifdef CAMERA_REAR2_OIS
static ssize_t camera_ois_autotest_2nd_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	int value = 0;

	if (kstrtoint(buf, 10, &value)) {
		err("convert fail");
	}

	ois_threshold_rear2 = value;

	return count;
}

static ssize_t camera_ois_autotest_2nd_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	bool x_result = false, y_result = false;
	bool x_result_2nd = false, y_result_2nd = false;
	int sin_x = 0, sin_y = 0;
	int sin_x_2nd = 0, sin_y_2nd = 0;
	char wide_buffer[60] = {0,};
	char tele_buffer[30] = {0,};


	if (check_ois_power) {
		fimc_is_ois_auto_test_rear2(sysfs_core, ois_threshold_rear2,
			&x_result, &y_result, &sin_x, &sin_y,
			&x_result_2nd, &y_result_2nd, &sin_x_2nd, &sin_y_2nd);

		if (x_result && y_result) {
			sprintf(wide_buffer, "%s,%d,%s,%d,", "pass", 0, "pass", 0);
		} else if (x_result) {
			sprintf(wide_buffer, "%s,%d,%s,%d,", "pass", 0, "fail", sin_y);
		} else if (y_result) {
			sprintf(wide_buffer, "%s,%d,%s,%d,", "fail", sin_x, "pass", 0);
		} else {
			sprintf(wide_buffer, "%s,%d,%s,%d,", "fail", sin_x, "fail", sin_y);
		}

		if (x_result_2nd && y_result_2nd) {
			sprintf(tele_buffer, "%s,%d,%s,%d", "pass", 0, "pass", 0);
		} else if (x_result_2nd) {
			sprintf(tele_buffer, "%s,%d,%s,%d", "pass", 0, "fail", sin_y_2nd);
		} else if (y_result_2nd) {
			sprintf(tele_buffer, "%s,%d,%s,%d", "fail", sin_x_2nd, "pass", 0);
		} else {
			sprintf(tele_buffer, "%s,%d,%s,%d", "fail", sin_x_2nd, "fail", sin_y_2nd);
		}

		strncat(wide_buffer, tele_buffer, strlen(tele_buffer));
		return sprintf(buf, "%s\n", wide_buffer);

	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%s,%d,%s,%d\n", "fail", sin_x, "fail", sin_y);
	}
}
#endif

static ssize_t camera_ois_selftest_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int result_total = 0;
	bool result_offset = 0, result_selftest = 0;
	int selftest_ret = 0;
	long raw_data_x = 0, raw_data_y = 0;

	if (check_ois_power) {
		fimc_is_ois_offset_test(sysfs_core, &raw_data_x, &raw_data_y);
		msleep(50);
		selftest_ret = fimc_is_ois_self_test(sysfs_core);

		if (selftest_ret == 0x0) {
			result_selftest = true;
		} else {
			result_selftest = false;
		}

		if (abs(raw_data_x) > CAMERA_OIS_GYRO_OFFSET_SPEC || abs(raw_data_y) > CAMERA_OIS_GYRO_OFFSET_SPEC)  {
			result_offset = false;
		} else {
			result_offset = true;
		}

		if (result_offset && result_selftest) {
			result_total = 0;
		} else if (!result_offset && !result_selftest) {
			result_total = 3;
		} else if (!result_offset) {
			result_total = 1;
		} else if (!result_selftest) {
			result_total = 2;
		}

		if (raw_data_x < 0 && raw_data_y < 0) {
			return sprintf(buf, "%d,-%lld.%03lld,-%lld.%03lld\n", result_total, abs(raw_data_x /1000), abs(raw_data_x % 1000),
				abs(raw_data_y /1000), abs(raw_data_y % 1000));
		} else if (raw_data_x < 0) {
			return sprintf(buf, "%d,-%lld.%03lld,%ld.%03ld\n", result_total, abs(raw_data_x /1000), abs(raw_data_x % 1000),
				raw_data_y /1000, raw_data_y % 1000);
		} else if (raw_data_y < 0) {
			return sprintf(buf, "%d,%ld.%03ld,-%lld.%03lld\n", result_total, raw_data_x /1000, raw_data_x % 1000,
				abs(raw_data_y /1000), abs(raw_data_y % 1000));
		} else {
			return sprintf(buf, "%d,%ld.%03ld,%ld.%03ld\n", result_total, raw_data_x /1000, raw_data_x % 1000,
				raw_data_y /1000, raw_data_y % 1000);
		}
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%d,%ld.%03ld,%ld.%03ld\n", result_total, raw_data_x /1000, raw_data_x % 1000,
			raw_data_y /1000, raw_data_y % 1000);
	}
}

static ssize_t camera_ois_rawdata_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	long raw_data_x = 0, raw_data_y = 0;

	if (check_ois_power) {
		fimc_is_ois_get_offset_data(sysfs_core, &raw_data_x, &raw_data_y);

		if (raw_data_x < 0 && raw_data_y < 0) {
			return sprintf(buf, "-%lld.%03lld,-%lld.%03lld\n", abs(raw_data_x /1000), abs(raw_data_x % 1000),
				abs(raw_data_y /1000), abs(raw_data_y % 1000));
		} else if (raw_data_x < 0) {
			return sprintf(buf, "-%lld.%03lld,%ld.%03ld\n", abs(raw_data_x /1000), abs(raw_data_x % 1000),
				raw_data_y /1000, raw_data_y % 1000);
		} else if (raw_data_y < 0) {
			return sprintf(buf, "%ld.%03ld,-%lld.%03lld\n", raw_data_x /1000, raw_data_x % 1000,
				abs(raw_data_y /1000), abs(raw_data_y % 1000));
		} else {
			return sprintf(buf, "%ld.%03ld,%ld.%03ld\n", raw_data_x /1000, raw_data_x % 1000,
				raw_data_y /1000, raw_data_y % 1000);
		}
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%ld.%03ld,%ld.%03ld\n", raw_data_x /1000, raw_data_x % 1000,
			raw_data_y /1000, raw_data_y % 1000);
	}
}

static ssize_t camera_ois_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_ois_info *ois_minfo = NULL;
	struct fimc_is_ois_info *ois_pinfo = NULL;
	bool ret = false;

	fimc_is_vender_check_hw_init_running();
	ret = read_ois_version();
	fimc_is_ois_get_module_version(&ois_minfo);
	fimc_is_ois_get_phone_version(&ois_pinfo);

	if (ois_minfo->checksum != 0x00 || !ret) {
		return sprintf(buf, "%s %s\n", "NG_FW2", "NULL");
	} else if (ois_minfo->caldata != 0x00) {
		return sprintf(buf, "%s %s\n", "NG_CD2", ois_pinfo->header_ver);
	} else {
		return sprintf(buf, "%s %s\n", ois_minfo->header_ver, ois_pinfo->header_ver);
	}
}

static ssize_t camera_ois_diff_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int result = 0;
	int x_diff = 0, y_diff = 0;

	if (check_ois_power) {
		result = fimc_is_ois_diff_test(sysfs_core, &x_diff, &y_diff);

		return sprintf(buf, "%d,%d,%d\n", result == true ? 0 : 1, x_diff, y_diff);
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%d,%d,%d\n", 0, x_diff, y_diff);
	}
}

static ssize_t camera_ois_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_ois_info *ois_minfo = NULL;
	struct fimc_is_ois_info *ois_pinfo = NULL;
	struct fimc_is_ois_info *ois_uinfo = NULL;
	struct fimc_is_ois_exif *ois_exif = NULL;

	fimc_is_ois_get_module_version(&ois_minfo);
	fimc_is_ois_get_phone_version(&ois_pinfo);
	fimc_is_ois_get_user_version(&ois_uinfo);
	fimc_is_ois_get_exif_data(&ois_exif);

	return sprintf(buf, "%s %s %s %d %d", ois_minfo->header_ver, ois_pinfo->header_ver,
		ois_uinfo->header_ver, ois_exif->error_data, ois_exif->status_data);
}
#endif

#ifdef CONFIG_COMPANION_FACTORY_VALIDATION
static ssize_t camera_comp_ic_check_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	struct fimc_is_vender_specific *specific = sysfs_core->vender.private_data;

	info("%s: buf[0]=%c running_rear_camera=%d running_front_camera=%d\n",
		__func__,buf[0],
		specific->running_rear_camera,specific->running_front_camera);

	switch (buf[0]) {
	case '0':
		break;
	case '1':
		comp_fac_i2c_check = 0;
		comp_fac_valid_check = 0;
		if (!specific->running_rear_camera) {
			if (!specific->running_front_camera) {
				ret = fimc_is_comp_fac_valid(sysfs_core);
			}
		}
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}

	return count;
}

static ssize_t camera_comp_ic_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	info("%s: comp_fac_i2c_check[%d] comp_fac_valid_check[0x%04x]\n",
		__func__, comp_fac_i2c_check, comp_fac_valid_check);

	if (comp_fac_i2c_check) {
		len = sprintf(buf, "%s\n", "NG_I2C");
	} else if (comp_fac_valid_check) {
		len = sprintf(buf, "%04X\n", comp_fac_valid_check);
	} else {
		len = sprintf(buf, "%s\n", "NG");
	}

	info("buf: %s\n",buf);

	return len;
}
#endif
#ifdef FORCE_CAL_LOAD
static ssize_t camera_rear_force_cal_load_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	fimc_is_sec_set_force_caldata_dump(true);
	return sprintf(buf, "FORCE CALDATA LOAD\n");
}
#endif

static ssize_t camera_rear_afcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
#ifdef CAMERA_REAR2
	char tempbuf[10];
	char *cal_buf;
	char N[] = "N ";
#endif

	read_from_firmware_version(SENSOR_POSITION_REAR);

#ifdef CAMERA_REAR2
	fimc_is_sec_get_cal_buf(&cal_buf);
	strncat(buf, "10 ", strlen("10 "));

#ifdef FROM_AF_CAL_D10_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF_CAL_D10_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF_CAL_D20_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF_CAL_D20_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF_CAL_D30_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF_CAL_D30_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF_CAL_D40_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF_CAL_D40_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF_CAL_D50_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF_CAL_D50_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF_CAL_D60_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF_CAL_D60_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF_CAL_D70_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF_CAL_D70_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF_CAL_D80_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF_CAL_D80_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

#ifdef FROM_AF_CAL_PAN_ADDR
	sprintf(tempbuf, "%d ", *((s32*)&cal_buf[FROM_AF_CAL_PAN_ADDR]));
#else
	sprintf(tempbuf, "%s", N);
#endif
	strncat(buf, tempbuf, strlen(tempbuf));

	return strlen(buf);
#else
	return sprintf(buf, "1 %d %d\n", finfo->af_cal_macro, finfo->af_cal_pan);
#endif
}

static ssize_t camera_rear_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	read_from_firmware_version(SENSOR_POSITION_REAR);

	return sprintf(buf, "%s", finfo->from_sensor_id);
}

static ssize_t camera_rear_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;

	read_from_firmware_version(SENSOR_POSITION_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf);

	memcpy(buf, &cal_buf[finfo->mtf_data_addr], FIMC_IS_RESOLUTION_DATA_SIZE);
	return FIMC_IS_RESOLUTION_DATA_SIZE;
}

static ssize_t camera_rear_awb_master_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;

	read_from_firmware_version(SENSOR_POSITION_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf);

	memcpy(buf, &cal_buf[finfo->awb_master_addr], FIMC_IS_AWB_MASTER_DATA_SIZE);
	return FIMC_IS_AWB_MASTER_DATA_SIZE;
}

static ssize_t camera_rear_awb_module_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;

	read_from_firmware_version(SENSOR_POSITION_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf);

	memcpy(buf, &cal_buf[finfo->awb_module_addr], FIMC_IS_AWB_MODULE_DATA_SIZE);
	return FIMC_IS_AWB_MODULE_DATA_SIZE;
}

static ssize_t camera_rear_moduleid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	read_from_firmware_version(SENSOR_POSITION_REAR);

	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
		finfo->from_module_id[0], finfo->from_module_id[1], finfo->from_module_id[2],
		finfo->from_module_id[3], finfo->from_module_id[4], finfo->from_module_id[5],
		finfo->from_module_id[6], finfo->from_module_id[7], finfo->from_module_id[8],
		finfo->from_module_id[9]);
}

#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
static ssize_t camera_front_afcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	read_from_firmware_version(SENSOR_POSITION_FRONT);

	return sprintf(buf, "1 %d %d\n", front_finfo->af_cal_macro, front_finfo->af_cal_pan);
}

static ssize_t camera_front_moduleid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	read_from_firmware_version(SENSOR_POSITION_FRONT);

	return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
		front_finfo->eeprom_front_module_id[0], front_finfo->eeprom_front_module_id[1], front_finfo->eeprom_front_module_id[2],
		front_finfo->eeprom_front_module_id[3], front_finfo->eeprom_front_module_id[4], front_finfo->eeprom_front_module_id[5],
		front_finfo->eeprom_front_module_id[6], front_finfo->eeprom_front_module_id[7], front_finfo->eeprom_front_module_id[8],
		front_finfo->eeprom_front_module_id[9]);
}

static ssize_t camera_front_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;

	read_from_firmware_version(SENSOR_POSITION_FRONT);
	fimc_is_sec_get_front_cal_buf(&cal_buf);

	memcpy(buf, &cal_buf[front_finfo->mtf_data_addr], FIMC_IS_RESOLUTION_DATA_SIZE);
	return FIMC_IS_RESOLUTION_DATA_SIZE;
}

static ssize_t camera_front_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	read_from_firmware_version(SENSOR_POSITION_FRONT);

	return sprintf(buf, "%s", front_finfo->from_sensor_id);
}
#endif

int camera_fw_show_sub(char *path, char *buf, int startPos, int readsize)
{
	struct file *fp = NULL;
	long fsize, nread;
	int ret = 0;

	fp = filp_open(path, O_RDONLY, 0660);

	fsize = fp->f_path.dentry->d_inode->i_size;

	if (IS_ERR(fp)) {
		err("Camera: Failed open phone firmware");
		ret = -EIO;
		fp = NULL;
		goto exit;
	}

	if (startPos < 0)
		fp->f_pos = fsize + startPos;
	else
		fp->f_pos = startPos;

	fsize = readsize;
	nread = vfs_read(fp, (char __user *)buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		err("failed to read firmware file, %ld Bytes", nread);
		ret = -EIO;
	}

exit:
	if (fp) {
		filp_close(fp, current->files);
		fp = NULL;
	}

	return ret;
}

char *rta_fw_list[] = {
#ifdef RTA_FW_LL
	RTA_FW_LL,
#endif
#ifdef RTA_FW_LX
	RTA_FW_LX,
#endif
#ifdef RTA_FW_LY
	RTA_FW_LY,
#endif
#ifdef RTA_FW_LS
	RTA_FW_LS
#endif
};

char *companion_fw_list[] = {
#ifdef COMPANION_FW_LL
	COMPANION_FW_LL,
#endif
#ifdef COMPANION_FW_LX
	COMPANION_FW_LX,
#endif
#ifdef COMPANION_FW_LY
	COMPANION_FW_LY,
#endif
#ifdef COMPANION_FW_LS
	COMPANION_FW_LS
#endif
};

static ssize_t camera_rear_camfw_all_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	#define VER_MAX_SIZE 100
	int ret;
	mm_segment_t old_fs;
	char fw_ver[VER_MAX_SIZE] = {0, };
	char output[1024] = {0, };
	char path[128] = {0, };
	int cnt = 0, loop = 0, i;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	sprintf(output, "[RCAM]");
	cnt = 0;

	loop = sizeof(rta_fw_list) / sizeof(char *);
	for (i = 0; i < loop; i++) {
		sprintf(path, FIMC_IS_FW_PATH"%s", rta_fw_list[i]);
		ret = camera_fw_show_sub(path, fw_ver,
						(-FIMC_IS_HEADER_VER_OFFSET), FIMC_IS_HEADER_VER_SIZE);
		if (ret == 0) {
			if (cnt++ > 0)
				strcat(output, ",");
			strcat(output, fw_ver);
			memset(fw_ver, 0, sizeof(fw_ver));
		}
	}

	strcat(output, ";[COMPANION]");
	cnt = 0;

	loop = sizeof(companion_fw_list) / sizeof(char *);
	for (i = 0; i < loop; i++) {
		sprintf(path, FIMC_IS_FW_PATH"%s", companion_fw_list[i]);
		ret = camera_fw_show_sub(path, fw_ver,
						(-16), FIMC_IS_HEADER_VER_SIZE);
		if (ret == 0) {
			if (cnt++ > 0)
				strcat(output, ",");
			strcat(output, fw_ver);
			memset(fw_ver, 0, sizeof(fw_ver));
		}
	}

	set_fs(old_fs);

	strcat(output, ";[OIS]");
	ret = fimc_is_ois_read_fw_ver(sysfs_core, FIMC_IS_FW_PATH FIMC_OIS_FW_NAME_SEC, fw_ver);
	if (ret == 0) {
		strcat(output, fw_ver);
		memset(fw_ver, 0, sizeof(fw_ver));
	}

	ret = fimc_is_ois_read_fw_ver(sysfs_core, FIMC_IS_FW_PATH FIMC_OIS_FW_NAME_DOM, fw_ver);
	if (ret == 0) {
		strcat(output, ",");
		strcat(output, fw_ver);
		memset(fw_ver, 0, sizeof(fw_ver));
	}
	return sprintf(buf, "%s\n", output);
}

#ifdef CONFIG_SECURE_CAMERA_USE
static ssize_t camera_iris_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char sensor_name[50];
	int ret;

	if (is_iris_ver_read) {
		ret = fimc_is_get_sensor_data(dev, NULL, sensor_name, SENSOR_POSITION_SECURE);

		if (ret < 0) {
			return sprintf(buf, "UNKNOWN N\n");
		} else {
			return sprintf(buf, "%s N\n", sensor_name);
		}
	} else {
		return sprintf(buf, "UNKNOWN N\n");
	}
}

static ssize_t camera_iris_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char sensor_name[50];
	int ret;

	if (is_iris_ver_read) {
		ret = fimc_is_get_sensor_data(dev, NULL, sensor_name, SENSOR_POSITION_SECURE);

		if (ret < 0) {
			return sprintf(buf, "UNKNOWN N N\n");
		} else {
			return sprintf(buf, "%s N N\n", sensor_name);
		}
	} else {
		return sprintf(buf, "UNKNOWN N N\n");
	}
}

static ssize_t camera_iris_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 year_month_company[3] = {'0', '0', '0'};

	info("camera_iris_checkfw_factory_show\n");

	if (is_iris_mtf_read_data[0] != 0x00 && is_iris_mtf_read_data[0] >= 'A' && is_iris_mtf_read_data[0] <= 'Z')
		year_month_company[0] = is_iris_mtf_read_data[0];

	if (is_iris_mtf_read_data[1] != 0x00 && is_iris_mtf_read_data[1] >= 'A' && is_iris_mtf_read_data[1] <= 'Z')
		year_month_company[1] = is_iris_mtf_read_data[1];

	if (is_iris_mtf_read_data[2] != 0x00 && is_iris_mtf_read_data[2] >= 'A' && is_iris_mtf_read_data[2] <= 'Z')
		year_month_company[2] = is_iris_mtf_read_data[2];

	if (is_iris_mtf_test_check == false) {
		return sprintf(buf, "NG_RES %c %c %c\n", year_month_company[0], year_month_company[1], year_month_company[2]);
	}
	if (is_iris_ver_read) {
		if (is_final_cam_module_iris) {
			return sprintf(buf, "OK %c %c %c\n", year_month_company[0], year_month_company[1], year_month_company[2]);
		} else {
#ifdef CAMERA_SYSFS_V2
			return sprintf(buf, "NG_VER %c %c %c\n", year_month_company[0], year_month_company[1], year_month_company[2]);
#else
			return sprintf(buf, "NG %c %c %c\n", year_month_company[0], year_month_company[1], year_month_company[2]);
#endif
		}
	} else {
		return sprintf(buf, "NG_VER %c %c %c\n", year_month_company[0], year_month_company[1], year_month_company[2]);
	}
}

#ifdef USE_CAMERA_HW_BIG_DATA
static ssize_t rear_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_REAR);

	if (fimc_is_sec_is_valid_moduleid(finfo->from_module_id)) {
		return sprintf(buf, "\"CAMIR_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CR_AF\":\"%d\","
			"\"I2CR_COM\":\"%d\",\"I2CR_OIS\":\"%d\",\"I2CR_SEN\":\"%d\",\"MIPIR_COM\":\"%d\",\"MIPIR_SEN\":\"%d\"\n",
			finfo->from_module_id[0], finfo->from_module_id[1], finfo->from_module_id[2], finfo->from_module_id[3],
			finfo->from_module_id[4], finfo->from_module_id[7], finfo->from_module_id[8], finfo->from_module_id[9],
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIR_ID\":\"MIR_ERR\",\"I2CR_AF\":\"%d\","
			"\"I2CR_COM\":\"%d\",\"I2CR_OIS\":\"%d\",\"I2CR_SEN\":\"%d\",\"MIPIR_COM\":\"%d\",\"MIPIR_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t rear_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_REAR);

		if (ec_param)
			fimc_is_sec_init_err_cnt_file(ec_param);
	}

	return count;
}

static ssize_t front_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_FRONT);

	if (fimc_is_sec_is_valid_moduleid(front_finfo->eeprom_front_module_id)) {
		return sprintf(buf, "\"CAMIF_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CF_AF\":\"%d\","
			"\"I2CF_COM\":\"%d\",\"I2CF_OIS\":\"%d\",\"I2CF_SEN\":\"%d\",\"MIPIF_COM\":\"%d\",\"MIPIF_SEN\":\"%d\"\n",
			front_finfo->eeprom_front_module_id[0], front_finfo->eeprom_front_module_id[1], front_finfo->eeprom_front_module_id[2],
			front_finfo->eeprom_front_module_id[3], front_finfo->eeprom_front_module_id[4], front_finfo->eeprom_front_module_id[7],
			front_finfo->eeprom_front_module_id[8], front_finfo->eeprom_front_module_id[9], ec_param->i2c_af_err_cnt,
			ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt, ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
			ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIF_ID\":\"MIR_ERR\",\"I2CF_AF\":\"%d\","
			"\"I2CF_COM\":\"%d\",\"I2CF_OIS\":\"%d\",\"I2CF_SEN\":\"%d\",\"MIPIF_COM\":\"%d\",\"MIPIF_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt, ec_param->i2c_sensor_err_cnt,
			ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t front_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_FRONT);

		if (ec_param)
			fimc_is_sec_init_err_cnt_file(ec_param);
	}

	return count;
}

#ifdef CAMERA_REAR2
static ssize_t rear2_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_REAR2);

	if (fimc_is_sec_is_valid_moduleid(finfo->from_module_id)) {
		return sprintf(buf, "\"CAMIR2_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CR2_AF\":\"%d\","
			"\"I2CR2_COM\":\"%d\",\"I2CR2_OIS\":\"%d\",\"I2CR2_SEN\":\"%d\",\"MIPIR2_COM\":\"%d\",\"MIPIR2_SEN\":\"%d\"\n",
			finfo->from_module_id[0], finfo->from_module_id[1], finfo->from_module_id[2], finfo->from_module_id[3],
			finfo->from_module_id[4], finfo->from_module_id[7], finfo->from_module_id[8], finfo->from_module_id[9],
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIR2_ID\":\"MIR_ERR\",\"I2CR2_AF\":\"%d\","
			"\"I2CR2_COM\":\"%d\",\"I2CR2_OIS\":\"%d\",\"I2CR2_SEN\":\"%d\",\"MIPIR2_COM\":\"%d\",\"MIPIR2_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t rear2_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_REAR2);

		if (ec_param)
			fimc_is_sec_init_err_cnt_file(ec_param);
	}

	return count;
}
#endif

#ifdef CONFIG_SECURE_CAMERA_USE
static ssize_t iris_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_SECURE);

	return sprintf(buf, "\"CAMII_ID\":\"MI_NO\",\"I2CI_AF\":\"%d\",\"I2CI_COM\":\"%d\",\"I2CI_OIS\":\"%d\","
		"\"I2CI_SEN\":\"%d\",\"MIPII_COM\":\"%d\",\"MIPII_SEN\":\"%d\"\n",
		ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
		ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
}

static ssize_t iris_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;

	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_SECURE);

		if (ec_param)
			fimc_is_sec_init_err_cnt_file(ec_param);
	}

	return count;
}
#endif
#endif

static ssize_t camera_iris_checkfw_user_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (is_iris_ver_read) {
		if (is_final_cam_module_iris)
			return sprintf(buf, "%s\n", "OK");
		else
			return sprintf(buf, "%s\n", "NG");
	} else {
		return sprintf(buf, "%s\n", "NG");
	}
}

#ifdef CAMERA_SYSFS_V2
static ssize_t camera_iris_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char camera_info[130] = {0, };
#ifdef CONFIG_OF
	struct fimc_is_cam_info *cam_info = &(cam_infos[CAM_INFO_IRIS]);

	if(!cam_info->valid) {
		strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N");

		return sprintf(buf, "%s\n", camera_info);
	} else {
		strcpy(camera_info, "ISP=");
		switch(cam_info->isp) {
			case CAM_INFO_ISP_TYPE_INTERNAL :
				strncat(camera_info, "INT;", strlen("INT;"));
				break;
			case CAM_INFO_ISP_TYPE_EXTERNAL :
				strncat(camera_info, "EXT;", strlen("EXT;"));
				break;
			case CAM_INFO_ISP_TYPE_SOC :
				strncat(camera_info, "SOC;", strlen("SOC;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "CALMEM=", strlen("CALMEM="));
		switch(cam_info->cal_memory) {
			case CAM_INFO_CAL_MEM_TYPE_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_CAL_MEM_TYPE_FROM :
			case CAM_INFO_CAL_MEM_TYPE_EEPROM :
			case CAM_INFO_CAL_MEM_TYPE_OTP :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "READVER=", strlen("READVER="));
		switch(cam_info->read_version) {
			case CAM_INFO_READ_VER_SYSFS :
				strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
				break;
			case CAM_INFO_READ_VER_CAMON :
				strncat(camera_info, "CAMON;", strlen("CAMON;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "COREVOLT=", strlen("COREVOLT="));
		switch(cam_info->core_voltage) {
			case CAM_INFO_CORE_VOLT_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_CORE_VOLT_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "UPGRADE=", strlen("UPGRADE="));
		switch(cam_info->upgrade) {
			case CAM_INFO_FW_UPGRADE_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_UPGRADE_SYSFS :
				strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
				break;
			case CAM_INFO_FW_UPGRADE_CAMON :
				strncat(camera_info, "CAMON;", strlen("CAMON;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "FWWRITE=", strlen("FWWRITE="));
		switch(cam_info->fw_write) {
			case CAM_INFO_FW_WRITE_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_WRITE_OS :
				strncat(camera_info, "OS;", strlen("OS;"));
				break;
			case CAM_INFO_FW_WRITE_SD :
				strncat(camera_info, "SD;", strlen("SD;"));
				break;
			case CAM_INFO_FW_WRITE_ALL :
				strncat(camera_info, "ALL;", strlen("ALL;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "FWDUMP=", strlen("FWDUMP="));
		switch(cam_info->fw_dump) {
			case CAM_INFO_FW_DUMP_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_DUMP_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "CC=", strlen("CC="));
		switch(cam_info->companion) {
			case CAM_INFO_COMPANION_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_COMPANION_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "OIS=", strlen("OIS="));
		switch(cam_info->ois) {
			case CAM_INFO_OIS_NONE :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_OIS_USE :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "VALID=", strlen("VALID="));
		switch(cam_info->valid) {
			case CAM_INFO_INVALID :
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_VALID :
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default :
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		return sprintf(buf, "%s\n", camera_info);
	}
#endif
	strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N");

	return sprintf(buf, "%s\n", camera_info);
}
#endif
#endif

#ifdef CAMERA_MODULE_DUALIZE
static DEVICE_ATTR(from_write, S_IRUGO,
		camera_rear_writefw_show, NULL);
#endif
static DEVICE_ATTR(rear_camtype, S_IRUGO,
		camera_rear_camtype_show, NULL);
static DEVICE_ATTR(rear_camfw, S_IRUGO,
		camera_rear_camfw_show, camera_rear_camfw_write);
static DEVICE_ATTR(rear_camfw_full, S_IRUGO,
		camera_rear_camfw_full_show, NULL);
#ifdef CONFIG_COMPANION_USE
static DEVICE_ATTR(rear_companionfw, S_IRUGO,
		camera_rear_companionfw_show, NULL);
static DEVICE_ATTR(rear_companionfw_full, S_IRUGO,
		camera_rear_companionfw_full_show, NULL);
#endif
static DEVICE_ATTR(rear_calcheck, S_IRUGO,
		camera_rear_calcheck_show, NULL);
static DEVICE_ATTR(rear_checkfw_user, S_IRUGO,
		camera_rear_checkfw_user_show, NULL);
static DEVICE_ATTR(rear_checkfw_factory, S_IRUGO,
		camera_rear_checkfw_factory_show, NULL);
#ifdef CAMERA_REAR2
static DEVICE_ATTR(rear2_camfw, S_IRUGO,
		camera_rear2_camfw_show, camera_rear_camfw_write);
static DEVICE_ATTR(rear2_camfw_full, S_IRUGO,
		camera_rear2_camfw_full_show, NULL);
static DEVICE_ATTR(rear2_checkfw_user, S_IRUGO,
		camera_rear2_checkfw_user_show, NULL);
static DEVICE_ATTR(rear2_checkfw_factory, S_IRUGO,
		camera_rear2_checkfw_factory_show, NULL);
static DEVICE_ATTR(rear2_sensorid_exif, S_IRUGO, camera_rear2_sensorid_exif_show, NULL);
static DEVICE_ATTR(rear2_mtf_exif, S_IRUGO, camera_rear2_mtf_exif_show, NULL);
static DEVICE_ATTR(rear2_sensorid, S_IRUGO, camera_rear2_sensorid_show, NULL);
static DEVICE_ATTR(rear2_afcal, S_IRUGO, camera_rear2_afcal_show, NULL);
static DEVICE_ATTR(rear_dualcal, S_IRUGO, camera_rear_dualcal_show, NULL);
static DEVICE_ATTR(rear_dualcal_size, S_IRUGO, camera_rear_dualcal_size_show, NULL);
static DEVICE_ATTR(rear2_tilt, S_IRUGO, camera_rear2_tilt_show, NULL);
#endif
static DEVICE_ATTR(rear_sensor_standby, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
		camera_rear_sensor_standby_show, camera_rear_sensor_standby);
#ifdef CAMERA_SYSFS_V2
static DEVICE_ATTR(rear_caminfo, S_IRUGO,
		camera_rear_info_show, NULL);
#ifdef CAMERA_REAR2
static DEVICE_ATTR(rear2_caminfo, S_IRUGO,
		camera_rear2_info_show, NULL);
#endif
#endif
#ifdef CONFIG_COMPANION_USE
static DEVICE_ATTR(isp_core, S_IRUGO,
		camera_isp_core_show, NULL);
#endif
static DEVICE_ATTR(fw_update, S_IRUGO,
		camera_hw_init_show, NULL);
#ifdef CONFIG_COMPANION_FACTORY_VALIDATION
static DEVICE_ATTR(companion_ic_check, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
		camera_comp_ic_check_show, camera_comp_ic_check_store);
#endif
#if defined (CONFIG_OIS_USE)
static DEVICE_ATTR(selftest, S_IRUGO,
		camera_ois_selftest_show, NULL);
static DEVICE_ATTR(ois_power, S_IWUSR,
		NULL, camera_ois_power_store);
static DEVICE_ATTR(autotest, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
		camera_ois_autotest_show, camera_ois_autotest_store);
#ifdef CAMERA_REAR2_OIS
static DEVICE_ATTR(autotest_2nd, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
		camera_ois_autotest_2nd_show, camera_ois_autotest_2nd_store);
#endif
static DEVICE_ATTR(ois_rawdata, S_IRUGO,
		camera_ois_rawdata_show, NULL);
static DEVICE_ATTR(oisfw, S_IRUGO,
		camera_ois_version_show, NULL);
static DEVICE_ATTR(ois_diff, S_IRUGO,
		camera_ois_diff_show, NULL);
static DEVICE_ATTR(ois_exif, S_IRUGO,
		camera_ois_exif_show, NULL);
#endif
#ifdef FORCE_CAL_LOAD
static DEVICE_ATTR(rear_force_cal_load, S_IRUGO, camera_rear_force_cal_load_show, NULL);
#endif
static DEVICE_ATTR(rear_afcal, S_IRUGO, camera_rear_afcal_show, NULL);
static DEVICE_ATTR(rear_sensorid_exif, S_IRUGO, camera_rear_sensorid_exif_show, NULL);
static DEVICE_ATTR(rear_moduleid, S_IRUGO, camera_rear_moduleid_show, NULL);
static DEVICE_ATTR(rear_mtf_exif, S_IRUGO, camera_rear_mtf_exif_show, NULL);
static DEVICE_ATTR(rear_awb_master, S_IRUGO, camera_rear_awb_master_show, NULL);
static DEVICE_ATTR(rear_awb_module, S_IRUGO, camera_rear_awb_module_show, NULL);
static DEVICE_ATTR(SVC_rear_module, S_IRUGO, camera_rear_moduleid_show, NULL);
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
static DEVICE_ATTR(front_afcal, S_IRUGO, camera_front_afcal_show, NULL);
static DEVICE_ATTR(front_moduleid, S_IRUGO, camera_front_moduleid_show, NULL);
static DEVICE_ATTR(front_mtf_exif, S_IRUGO, camera_front_mtf_exif_show, NULL);
static DEVICE_ATTR(front_sensorid_exif, S_IRUGO, camera_front_sensorid_exif_show, NULL);
static DEVICE_ATTR(SVC_front_module, S_IRUGO, camera_front_moduleid_show, NULL);
#endif

static DEVICE_ATTR(rear_camfw_all, S_IRUGO, camera_rear_camfw_all_show, NULL);

#ifdef CONFIG_SECURE_CAMERA_USE
static DEVICE_ATTR(iris_camfw, S_IRUGO, camera_iris_camfw_show, NULL);
static DEVICE_ATTR(iris_camfw_full, S_IRUGO, camera_iris_camfw_full_show, NULL);
static DEVICE_ATTR(iris_checkfw_factory, S_IRUGO, camera_iris_checkfw_factory_show, NULL);
static DEVICE_ATTR(iris_checkfw_user, S_IRUGO, camera_iris_checkfw_user_show, NULL);
#ifdef CAMERA_SYSFS_V2
static DEVICE_ATTR(iris_caminfo, S_IRUGO, camera_iris_info_show, NULL);
#endif
#endif

#ifdef USE_CAMERA_HW_BIG_DATA
static DEVICE_ATTR(rear_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				rear_camera_hw_param_show, rear_camera_hw_param_store);
static DEVICE_ATTR(front_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				front_camera_hw_param_show, front_camera_hw_param_store);
#ifdef CAMERA_REAR2
static DEVICE_ATTR(rear2_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				rear2_camera_hw_param_show, rear2_camera_hw_param_store);
#endif
#ifdef CONFIG_SECURE_CAMERA_USE
static DEVICE_ATTR(iris_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				iris_camera_hw_param_show, iris_camera_hw_param_store);
#endif
#endif

int svc_cheating_prevent_device_file_create(struct kobject **obj)
{
	struct kernfs_node *svc_sd;
	struct kobject *data;
	struct kobject *Camera;

	/* To find svc kobject */
	svc_sd = sysfs_get_dirent(devices_kset->kobj.sd, "svc");
	if (IS_ERR_OR_NULL(svc_sd)) {
		/* try to create svc kobject */
		data = kobject_create_and_add("svc", &devices_kset->kobj);
		if (IS_ERR_OR_NULL(data))
			pr_info("Failed to create sys/devices/svc already exist svc : 0x%p\n", data);
		else
			pr_info("Success to create sys/devices/svc svc : 0x%p\n", data);
	} else {
		data = (struct kobject *)svc_sd->priv;
		pr_info("Success to find svc_sd : 0x%p svc : 0x%p\n", svc_sd, data);
	}

	Camera = kobject_create_and_add("Camera", data);
	if (IS_ERR_OR_NULL(Camera))
		pr_info("Failed to create sys/devices/svc/Camera : 0x%p\n", Camera);
	else
		pr_info("Success to create sys/devices/svc/Camera : 0x%p\n", Camera);

	*obj = Camera;

	return 0;
}

int fimc_is_create_sysfs(struct fimc_is_core *core)
{
	struct kobject *svc = 0;

	if (!core) {
		err("fimc_is_core is null");
		return -EINVAL;
	}

	svc_cheating_prevent_device_file_create(&svc);

	if (camera_class == NULL) {
		camera_class = class_create(THIS_MODULE, "camera");
		if (IS_ERR(camera_class)) {
			pr_err("Failed to create class(camera)!\n");
			return PTR_ERR(camera_class);
		}
	}

	camera_front_dev = device_create(camera_class, NULL, 0, NULL, "front");
	if (IS_ERR(camera_front_dev)) {
		printk(KERN_ERR "failed to create front device!\n");
	} else {
		if (device_create_file(camera_front_dev,
				&dev_attr_front_sensorid) < 0) {
			printk(KERN_ERR "failed to create front device file, %s\n",
					dev_attr_front_sensorid.attr.name);
		}

		if (device_create_file(camera_front_dev,
					&dev_attr_front_camtype) < 0) {
			printk(KERN_ERR
				"failed to create front device file, %s\n",
				dev_attr_front_camtype.attr.name);
		}
		if (device_create_file(camera_front_dev,
					&dev_attr_front_camfw) < 0) {
			printk(KERN_ERR
				"failed to create front device file, %s\n",
				dev_attr_front_camfw.attr.name);
		}
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
		if (device_create_file(camera_front_dev,
					&dev_attr_front_camfw_full) < 0) {
			printk(KERN_ERR
				"failed to create front device file, %s\n",
				dev_attr_front_camfw_full.attr.name);
		}
		if (device_create_file(camera_front_dev,
				&dev_attr_front_checkfw_factory) < 0) {
			printk(KERN_ERR
				"failed to create front device file, %s\n",
				dev_attr_front_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_front_dev,
				&dev_attr_front_sensorid_exif) < 0) {
			printk(KERN_ERR
				"failed to create front device file, %s\n",
				dev_attr_front_sensorid_exif.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_moduleid) < 0) {
			printk(KERN_ERR "failed to create front device file, %s\n",
					dev_attr_front_moduleid.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_mtf_exif) < 0) {
			printk(KERN_ERR "failed to create front device file, %s\n",
					dev_attr_front_mtf_exif.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_afcal) < 0) {
			printk(KERN_ERR "failed to create front device file, %s\n",
					dev_attr_front_afcal.attr.name);
		}
		if (sysfs_create_file(svc, &dev_attr_SVC_front_module.attr) < 0) {
			printk(KERN_ERR "failed to create front device file, %s\n",
					dev_attr_SVC_front_module.attr.name);
		}
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_front_dev,
					&dev_attr_front_hwparam) < 0) {
			printk(KERN_ERR
				"failed to create front device file, %s\n",
				dev_attr_front_hwparam.attr.name);
		}
#endif
	}
#ifdef CAMERA_SYSFS_V2
		if (device_create_file(camera_front_dev,
					&dev_attr_front_caminfo) < 0) {
			printk(KERN_ERR
				"failed to create front device file, %s\n",
				dev_attr_front_caminfo.attr.name);
		}
#endif
	camera_rear_dev = device_create(camera_class, NULL, 1, NULL, "rear");
	if (IS_ERR(camera_rear_dev)) {
		printk(KERN_ERR "failed to create rear device!\n");
	} else {
		if (device_create_file(camera_rear_dev, &dev_attr_rear_sensorid) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_sensorid.attr.name);
		}
#ifdef CAMERA_REAR2
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_sensorid) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear2_sensorid.attr.name);
		}
#endif
#ifdef CAMERA_MODULE_DUALIZE
		if (device_create_file(camera_rear_dev, &dev_attr_from_write) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_from_write.attr.name);
		}
#endif
		if (device_create_file(camera_rear_dev, &dev_attr_rear_camtype) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_camtype.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_camfw) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_camfw.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_camfw_full) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_camfw_full.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_checkfw_user) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_checkfw_user.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_checkfw_factory) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_checkfw_factory.attr.name);
		}
#ifdef CAMERA_REAR2
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_camfw) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear2_camfw.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_camfw_full) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear2_camfw_full.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_checkfw_user) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear2_checkfw_user.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_checkfw_factory) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear2_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_sensorid_exif) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear2_sensorid_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_mtf_exif) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear2_mtf_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_dualcal) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_dualcal.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_dualcal_size) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_dualcal_size.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_afcal) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear2_afcal.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_tilt) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear2_tilt.attr.name);
		}
#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_hwparam) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear2_hwparam.attr.name);
		}
#endif
#endif
		if (device_create_file(camera_rear_dev,
				&dev_attr_ssrm_camera_info) < 0) {
			printk(KERN_ERR
				"failed to create front device file, %s\n",
				dev_attr_ssrm_camera_info.attr.name);
		}

		if (device_create_file(camera_rear_dev, &dev_attr_rear_sensor_standby) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_sensor_standby.attr.name);
		}
#ifdef CAMERA_SYSFS_V2
		if (device_create_file(camera_rear_dev,
					&dev_attr_rear_caminfo) < 0) {
			printk(KERN_ERR
				"failed to create rear device file, %s\n",
				dev_attr_rear_caminfo.attr.name);
		}
#ifdef CAMERA_REAR2
		if (device_create_file(camera_rear_dev,
					&dev_attr_rear2_caminfo) < 0) {
			printk(KERN_ERR
				"failed to create rear device file, %s\n",
				dev_attr_rear_caminfo.attr.name);
		}
#endif
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_rear_dev,
					&dev_attr_rear_hwparam) < 0) {
			printk(KERN_ERR
				"failed to create rear device file, %s\n",
				dev_attr_rear_hwparam.attr.name);
		}
#endif
#ifdef CONFIG_COMPANION_USE
		if (device_create_file(camera_rear_dev, &dev_attr_rear_companionfw) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_companionfw.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_companionfw_full) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_companionfw_full.attr.name);
		}
#endif
		if (device_create_file(camera_rear_dev, &dev_attr_rear_calcheck) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_calcheck.attr.name);
		}
#ifdef CONFIG_COMPANION_USE
		if (device_create_file(camera_rear_dev, &dev_attr_isp_core) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_isp_core.attr.name);
		}
#endif
		if (device_create_file(camera_rear_dev, &dev_attr_fw_update) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_fw_update.attr.name);
		}
#ifdef FORCE_CAL_LOAD
		if (device_create_file(camera_rear_dev, &dev_attr_rear_force_cal_load) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_force_cal_load.attr.name);
		}
#endif
		if (device_create_file(camera_rear_dev, &dev_attr_rear_afcal) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_afcal.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_sensorid_exif) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_sensorid_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_moduleid) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_moduleid.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_mtf_exif) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_mtf_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_awb_master) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_awb_master.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_awb_module) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_awb_module.attr.name);
		}
		if (sysfs_create_file(svc, &dev_attr_SVC_rear_module.attr) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
			dev_attr_SVC_rear_module.attr.name);
		}
#ifdef CONFIG_COMPANION_FACTORY_VALIDATION
		if (device_create_file(camera_rear_dev, &dev_attr_companion_ic_check) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_companion_ic_check.attr.name);
		}
#endif
		if (device_create_file(camera_rear_dev, &dev_attr_rear_camfw_all) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_camfw_all.attr.name);
		}
	}

#if defined (CONFIG_OIS_USE)
	camera_ois_dev = device_create(camera_class, NULL, 2, NULL, "ois");
	if (IS_ERR(camera_ois_dev)) {
		printk(KERN_ERR "failed to create ois device!\n");
	} else {
		if (device_create_file(camera_ois_dev, &dev_attr_selftest) < 0) {
			printk(KERN_ERR "failed to create ois device file, %s\n",
				dev_attr_selftest.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_power) < 0) {
			printk(KERN_ERR "failed to create ois device file, %s\n",
				dev_attr_ois_power.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_autotest) < 0) {
			printk(KERN_ERR "failed to create ois device file, %s\n",
				dev_attr_autotest.attr.name);
		}
#ifdef CAMERA_REAR2_OIS
		if (device_create_file(camera_ois_dev, &dev_attr_autotest_2nd) < 0) {
			printk(KERN_ERR "failed to create ois device file, %s\n",
				dev_attr_autotest_2nd.attr.name);
		}
#endif
		if (device_create_file(camera_ois_dev, &dev_attr_ois_rawdata) < 0) {
			printk(KERN_ERR "failed to create ois device file, %s\n",
				dev_attr_ois_rawdata.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_oisfw) < 0) {
			printk(KERN_ERR "failed to create ois device file, %s\n",
				dev_attr_oisfw.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_diff) < 0) {
			printk(KERN_ERR "failed to create ois device file, %s\n",
				dev_attr_ois_diff.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_exif) < 0) {
			printk(KERN_ERR "failed to create ois device file, %s\n",
				dev_attr_ois_exif.attr.name);
		}
	}
#endif
#ifdef CONFIG_SECURE_CAMERA_USE
	camera_secure_dev = device_create(camera_class, NULL, 3, NULL, "secure");
	if (IS_ERR(camera_secure_dev)) {
		printk(KERN_ERR "failed to create secure device!\n");
	} else {
		if (device_create_file(camera_secure_dev, &dev_attr_secure_sensorid) < 0) {
			printk(KERN_ERR "failed to create secure device file, %s\n",
				dev_attr_secure_sensorid.attr.name);
		}
		if (device_create_file(camera_secure_dev, &dev_attr_iris_camfw) < 0) {
			printk(KERN_ERR "failed to create iris device file, %s\n",
				dev_attr_iris_camfw.attr.name);
		}
		if (device_create_file(camera_secure_dev, &dev_attr_iris_camfw_full) < 0) {
			printk(KERN_ERR "failed to create iris device file, %s\n",
				dev_attr_iris_camfw_full.attr.name);
		}
		if (device_create_file(camera_secure_dev, &dev_attr_iris_checkfw_user) < 0) {
			printk(KERN_ERR "failed to create iris device file, %s\n",
				dev_attr_iris_checkfw_user.attr.name);
		}
		if (device_create_file(camera_secure_dev, &dev_attr_iris_checkfw_factory) < 0) {
			printk(KERN_ERR "failed to create iris device file, %s\n",
				dev_attr_iris_checkfw_factory.attr.name);
		}
#ifdef CAMERA_SYSFS_V2
		if (device_create_file(camera_secure_dev,
					&dev_attr_iris_caminfo) < 0) {
			printk(KERN_ERR
				"failed to create iris device file, %s\n",
				dev_attr_iris_caminfo.attr.name);
		}
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_secure_dev,
					&dev_attr_iris_hwparam) < 0) {
			printk(KERN_ERR
				"failed to create iris device file, %s\n",
				dev_attr_iris_hwparam.attr.name);
		}
#endif
	}
#endif

	sysfs_core = core;

	return 0;
}

int fimc_is_destroy_sysfs(struct fimc_is_core *core)
{
	if (camera_front_dev) {
		device_remove_file(camera_front_dev, &dev_attr_front_sensorid);
		device_remove_file(camera_front_dev, &dev_attr_front_camtype);
		device_remove_file(camera_front_dev, &dev_attr_front_camfw);
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
		device_remove_file(camera_front_dev, &dev_attr_front_afcal);
		device_remove_file(camera_front_dev, &dev_attr_front_camfw_full);
		device_remove_file(camera_front_dev, &dev_attr_front_checkfw_factory);
		device_remove_file(camera_front_dev, &dev_attr_front_moduleid);
		device_remove_file(camera_front_dev, &dev_attr_front_mtf_exif);
		device_remove_file(camera_front_dev, &dev_attr_front_sensorid_exif);
#endif
#ifdef CAMERA_SYSFS_V2
		device_remove_file(camera_front_dev, &dev_attr_front_caminfo);
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_front_dev, &dev_attr_front_hwparam);
#endif
	}

	if (camera_rear_dev) {
		device_remove_file(camera_rear_dev, &dev_attr_rear_sensorid);

#ifdef CAMERA_MODULE_DUALIZE
		device_remove_file(camera_rear_dev, &dev_attr_from_write);
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_camtype);
		device_remove_file(camera_rear_dev, &dev_attr_rear_camfw);
		device_remove_file(camera_rear_dev, &dev_attr_rear_camfw_full);
		device_remove_file(camera_rear_dev, &dev_attr_rear_checkfw_user);
		device_remove_file(camera_rear_dev, &dev_attr_rear_checkfw_factory);
#ifdef CAMERA_REAR2
		device_remove_file(camera_rear_dev, &dev_attr_rear2_sensorid);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_camfw);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_camfw_full);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_checkfw_user);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_checkfw_factory);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_sensorid_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_mtf_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_afcal);
		device_remove_file(camera_rear_dev, &dev_attr_rear_dualcal);
		device_remove_file(camera_rear_dev, &dev_attr_rear_dualcal_size);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_tilt);
#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_rear_dev, &dev_attr_rear2_hwparam);
#endif
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_sensor_standby);
#ifdef CONFIG_COMPANION_USE
		device_remove_file(camera_rear_dev, &dev_attr_rear_companionfw);
		device_remove_file(camera_rear_dev, &dev_attr_rear_companionfw_full);
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_calcheck);
#ifdef CAMERA_SYSFS_V2
		device_remove_file(camera_rear_dev, &dev_attr_rear_caminfo);
#ifdef CAMERA_REAR2
		device_remove_file(camera_rear_dev, &dev_attr_rear2_caminfo);
#endif
#endif
#ifdef CONFIG_COMPANION_USE
		device_remove_file(camera_rear_dev, &dev_attr_isp_core);
#endif
#ifdef FORCE_CAL_LOAD
		device_remove_file(camera_rear_dev, &dev_attr_rear_force_cal_load);
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_afcal);
		device_remove_file(camera_rear_dev, &dev_attr_rear_sensorid_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear_moduleid);
		device_remove_file(camera_rear_dev, &dev_attr_rear_mtf_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear_awb_master);
		device_remove_file(camera_rear_dev, &dev_attr_rear_awb_module);
		device_remove_file(camera_rear_dev, &dev_attr_fw_update);
#ifdef CONFIG_COMPANION_FACTORY_VALIDATION
		device_remove_file(camera_rear_dev, &dev_attr_companion_ic_check);
#endif
		device_remove_file(camera_rear_dev, &dev_attr_ssrm_camera_info);
#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_rear_dev, &dev_attr_rear_hwparam);
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_camfw_all);
	}

#if defined (CONFIG_OIS_USE)
	if (camera_ois_dev) {
		device_remove_file(camera_ois_dev, &dev_attr_selftest);
		device_remove_file(camera_ois_dev, &dev_attr_ois_power);
		device_remove_file(camera_ois_dev, &dev_attr_autotest);
#ifdef CAMERA_REAR2_OIS
		device_remove_file(camera_ois_dev, &dev_attr_autotest_2nd);
#endif
		device_remove_file(camera_ois_dev, &dev_attr_ois_rawdata);
		device_remove_file(camera_ois_dev, &dev_attr_oisfw);
		device_remove_file(camera_ois_dev, &dev_attr_ois_diff);
		device_remove_file(camera_ois_dev, &dev_attr_ois_exif);
	}
#endif
#ifdef CONFIG_SECURE_CAMERA_USE
	if (camera_secure_dev) {
		device_remove_file(camera_secure_dev, &dev_attr_secure_sensorid);
		device_remove_file(camera_secure_dev, &dev_attr_iris_camfw);
		device_remove_file(camera_secure_dev, &dev_attr_iris_camfw_full);
		device_remove_file(camera_secure_dev, &dev_attr_iris_checkfw_user);
		device_remove_file(camera_secure_dev, &dev_attr_iris_checkfw_factory);
#ifdef CAMERA_SYSFS_V2
		device_remove_file(camera_secure_dev, &dev_attr_iris_caminfo);
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_secure_dev, &dev_attr_iris_hwparam);
#endif
	}
#endif

	if (camera_class) {
		if (camera_front_dev)
			device_destroy(camera_class, camera_front_dev->devt);

		if (camera_rear_dev)
			device_destroy(camera_class, camera_rear_dev->devt);

#if defined (CONFIG_OIS_USE)
		if (camera_ois_dev)
			device_destroy(camera_class, camera_ois_dev->devt);
#endif
#ifdef CONFIG_SECURE_CAMERA_USE
		if (camera_secure_dev)
			device_destroy(camera_class, camera_secure_dev->devt);
#endif
	}

	class_destroy(camera_class);

	return 0;
}
