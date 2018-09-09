/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_SEC_DEFINE_H
#define FIMC_IS_SEC_DEFINE_H

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <video/videonode.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/v4l2-mediabus.h>
#include <linux/bug.h>
#include <linux/syscalls.h>
#include <linux/vmalloc.h>

#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/zlib.h>

#include "fimc-is-core.h"
#include "fimc-is-cmd.h"
#include "fimc-is-regs.h"
#include "fimc-is-err.h"
#include "fimc-is-video.h"

#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"
#include "crc32.h"
#include "fimc-is-companion.h"
#include "fimc-is-device-from.h"
#include "fimc-is-dt.h"

#define FW_CORE_VER		0
#define FW_PIXEL_SIZE		1
#define FW_ISP_COMPANY		3
#define FW_SENSOR_MAKER		4
#define FW_PUB_YEAR		5
#define FW_PUB_MON		6
#define FW_PUB_NUM		7
#define FW_MODULE_COMPANY	9
#define FW_VERSION_INFO		10

#define FW_ISP_COMPANY_BROADCOMM	'B'
#define FW_ISP_COMPANY_TN		'C'
#define FW_ISP_COMPANY_FUJITSU		'F'
#define FW_ISP_COMPANY_INTEL		'I'
#define FW_ISP_COMPANY_LSI		'L'
#define FW_ISP_COMPANY_MARVELL		'M'
#define FW_ISP_COMPANY_QUALCOMM		'Q'
#define FW_ISP_COMPANY_RENESAS		'R'
#define FW_ISP_COMPANY_STE		'S'
#define FW_ISP_COMPANY_TI		'T'
#define FW_ISP_COMPANY_DMC		'D'

#define FW_SENSOR_MAKER_SF		'F'
#define FW_SENSOR_MAKER_SLSI		'L'			/* rear_front*/
#define FW_SENSOR_MAKER_SONY		'S'			/* rear_front*/
#define FW_SENSOR_MAKER_SLSI_SONY		'X'	/* rear_front*/
#define FW_SENSOR_MAKER_SONY_LSI		'Y'		/* rear_front*/

#define FW_SENSOR_MAKER_SLSI		'L'

#define FW_MODULE_COMPANY_SEMCO		'S'
#define FW_MODULE_COMPANY_GUMI		'O'
#define FW_MODULE_COMPANY_CAMSYS	'C'
#define FW_MODULE_COMPANY_PATRON	'P'
#define FW_MODULE_COMPANY_MCNEX		'M'
#define FW_MODULE_COMPANY_LITEON	'L'
#define FW_MODULE_COMPANY_VIETNAM	'V'
#define FW_MODULE_COMPANY_SHARP		'J'
#define FW_MODULE_COMPANY_NAMUGA	'N'
#define FW_MODULE_COMPANY_POWER_LOGIX	'A'
#define FW_MODULE_COMPANY_DI		'D'

#define FW_2L3_H		"H12LL"		/* STAR1 rear*/
#define FW_2L3_I		"I12LL"		/* STAR2 wide */
#define FW_IMX345_H		"H12LS"		/* STAR1 rear*/
#define FW_IMX345_I		"I12LS"		/* STAR2 wide */

#define FW_3H1_W		"W08LL"		/* STAR1/2 front*/
#define FW_IMX320_W		"W08LS"

#define SDCARD_FW

#define FIMC_IS_DDK						"fimc_is_lib.bin"
#define FIMC_IS_RTA						"fimc_is_rta.bin"
#define FIMC_IS_RTA_2L3_3H1				"fimc_is_rta_2l3_3h1.bin"
#define FIMC_IS_RTA_2L3_IMX320			"fimc_is_rta_2l3_imx320.bin"
#define FIMC_IS_RTA_IMX345_3H1			"fimc_is_rta_imx345_3h1.bin"
#define FIMC_IS_RTA_IMX345_IMX320		"fimc_is_rta_imx345_imx320.bin"

/*#define FIMC_IS_FW_SDCARD			"/data/media/0/fimc_is_fw.bin" */
/* Rear setfile */
#define FIMC_IS_2L3_SETF			"setfile_2l3.bin"
#define FIMC_IS_3M3_SETF			"setfile_3m3.bin"
#define FIMC_IS_IMX345_SETF			"setfile_imx345.bin"

/* Front setfile */
#define FIMC_IS_IMX320_SETF			"setfile_imx320.bin"
#define FIMC_IS_3H1_SETF			"setfile_3h1.bin"

#define FIMC_IS_CAL_SDCARD_FRONT		"/data/cal_data_front.bin"
#define FIMC_IS_FW_FROM_SDCARD		"/data/media/0/CamFW_Main.bin"
#define FIMC_IS_SETFILE_FROM_SDCARD		"/data/media/0/CamSetfile_Main.bin"
#define FIMC_IS_KEY_FROM_SDCARD		"/data/media/0/1q2w3e4r.key"

#define FIMC_IS_HEADER_VER_SIZE      11
#define FIMC_IS_OEM_VER_SIZE         11
#define FIMC_IS_AWB_VER_SIZE         11
#define FIMC_IS_SHADING_VER_SIZE     11
#define FIMC_IS_CAL_MAP_VER_SIZE     4
#define FIMC_IS_PROJECT_NAME_SIZE    8
#define FIMC_IS_ISP_SETFILE_VER_SIZE 6
#define FIMC_IS_SENSOR_ID_SIZE       16
#define FIMC_IS_CAL_DLL_VERSION_SIZE 4
#define FIMC_IS_MODULE_ID_SIZE       10
#define FIMC_IS_RESOLUTION_DATA_SIZE 54
#define FIMC_IS_AWB_MASTER_DATA_SIZE 8
#define FIMC_IS_AWB_MODULE_DATA_SIZE 8

#ifdef USE_BINARY_PADDING_DATA_ADDED
#define FIMC_IS_HEADER_VER_OFFSET   (FIMC_IS_HEADER_VER_SIZE+FIMC_IS_SIGNATURE_SIZE)
#else
#define FIMC_IS_HEADER_VER_OFFSET   (FIMC_IS_HEADER_VER_SIZE)
#endif
#define FIMC_IS_MAX_TUNNING_BUFFER_SIZE (15 * 1024)

#define FROM_VERSION_V002 '2'
#define FROM_VERSION_V003 '3'
#define FROM_VERSION_V004 '4'
#define FROM_VERSION_V005 '5'

/* #define DEBUG_FORCE_DUMP_ENABLE */

/* #define CONFIG_RELOAD_CAL_DATA 1 */

enum {
        CC_BIN1 = 0,
        CC_BIN2,
        CC_BIN3,
        CC_BIN4,
        CC_BIN5,
        CC_BIN6,
        CC_BIN7,
        CC_BIN_MAX,
};

#ifdef CONFIG_COMPANION_USE
enum fimc_is_companion_sensor {
	COMPANION_SENSOR_2P2 = 1,
	COMPANION_SENSOR_IMX240 = 2,
	COMPANION_SENSOR_2T2 = 3
};
#endif

struct fimc_is_from_info {
	u32		bin_start_addr;		/* DDK */
	u32		bin_end_addr;
#ifdef USE_RTA_BINARY
	u32		rta_bin_start_addr;	/* RTA */
	u32		rta_bin_end_addr;
#endif
	u32		hifi_tunning_start_addr;	/* HIFI TUNNING */
	u32		hifi_tunning_end_addr;
	u32		header_start_addr;
	u32		header_end_addr;

	u32		oem_start_addr;
	u32		oem_end_addr;
	u32		awb_start_addr;
	u32		awb_end_addr;
	u32		shading_start_addr;
	u32		shading_end_addr;
	u32		setfile_start_addr;
	u32		setfile_end_addr;
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
	u32		setfile2_start_addr;
	u32		setfile2_end_addr;
#endif
	u32		front_setfile_start_addr;
	u32		front_setfile_end_addr;
	u32		header_section_crc_addr;
	u32		cal_data_section_crc_addr;
	u32		cal_data_start_addr;
	u32		cal_data_end_addr;
	u32		cal2_data_section_crc_addr;
	u32		cal2_data_start_addr;
	u32		cal2_data_end_addr;
	u32		oem_section_crc_addr;
	u32		awb_section_crc_addr;
	u32		shading_section_crc_addr;
	u32		af_cal_pan;
	u32		af_cal_macro;
	u32		af_cal_d1;
	u32		af_cal2_pan;		/* Rear Second Sensor (TELE) */
	u32		af_cal2_macro;		/* Rear Second Sensor (TELE) */
	u32		af_cal2_d1;		/* Rear Second Sensor (TELE) */
	u32		mtf_data_addr;
	u32		mtf_data2_addr;		/*TELE*/
	char		header_ver[FIMC_IS_HEADER_VER_SIZE + 1];
	char		header2_ver[FIMC_IS_HEADER_VER_SIZE + 1]; /* Rear Second Sensor (TELE) */
	char		cal_map_ver[FIMC_IS_CAL_MAP_VER_SIZE + 1];
	char		setfile_ver[FIMC_IS_SETFILE_VER_SIZE + 1];
	char		oem_ver[FIMC_IS_OEM_VER_SIZE + 1];
	char		awb_ver[FIMC_IS_AWB_VER_SIZE + 1];
	char		shading_ver[FIMC_IS_SHADING_VER_SIZE + 1];

	char		load_fw_name[50];		/* DDK */
	char		load_rta_fw_name[50];	/* RTA */

	char		load_setfile_name[50];
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
	char		load_setfile2_name[50];  /*rear 2*/
#endif
	char		load_front_setfile_name[50];
	char		load_tunning_hifills_name[50];
	char		project_name[FIMC_IS_PROJECT_NAME_SIZE + 1];
	char		from_sensor_id[FIMC_IS_SENSOR_ID_SIZE + 1];
	char		from_sensor2_id[FIMC_IS_SENSOR_ID_SIZE + 1]; /*REAR 2 Sensor ID*/
	u8		from_module_id[FIMC_IS_MODULE_ID_SIZE + 1];
	u8		eeprom_front_module_id[FIMC_IS_MODULE_ID_SIZE + 1];
	bool		is_caldata_read;
	bool		is_fw_find_done;
	bool		is_check_bin_done;
	bool		is_check_cal_reload;
	u8		sensor_version;
	u32		awb_master_addr;
	u32		awb_module_addr;
	u32		lsc_gain_start_addr;
	u32		lsc_gain_end_addr;
	u32		pdaf_cal_start_addr;
	u32		pdaf_cal_end_addr;
	u32		lsc_i0_gain_addr;
	u32		lsc_j0_gain_addr;
	u32		lsc_a_gain_addr;
	u32		lsc_k4_gain_addr;
	u32		lsc_scale_gain_addr;
	u32		lsc_gain_crc_addr;
#ifdef FROM_SUPPORT_APERTURE_F2
	u32		shading_f2_start_addr;
	u32		shading_f2_end_addr;
	u32		mtf_f2_data_addr;
	u32		awb_f2_master_addr;
	u32		awb_f2_module_addr;
	u32		pdaf_cal_f2_start_addr;
	u32		pdaf_cal_f2_end_addr;
	u32		lsc_gain_f2_start_addr;
	u32		lsc_gain_f2_end_addr;
	u32		lsc_i0_f2_gain_addr;
	u32		lsc_j0_f2_gain_addr;
	u32		lsc_a_f2_gain_addr;
	u32		lsc_k4_f2_gain_addr;
	u32		lsc_scale_f2_gain_addr;
	u32		lsc_gain_f2_crc_addr;
#endif
	unsigned long		fw_size;
#ifdef USE_RTA_BINARY
	unsigned long		rta_fw_size;
#endif
	unsigned long		setfile_size;
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
	unsigned long		setfile2_size;
#endif
	unsigned long		front_setfile_size;
	unsigned long		hifi_tunning_size;
};

bool fimc_is_sec_get_force_caldata_dump(void);
int fimc_is_sec_set_force_caldata_dump(bool fcd);

ssize_t write_data_to_file(char *name, char *buf, size_t count, loff_t *pos);
ssize_t read_data_from_file(char *name, char *buf, size_t count, loff_t *pos);
bool fimc_is_sec_file_exist(char *name);

int fimc_is_sec_get_sysfs_finfo(struct fimc_is_from_info **finfo);
int fimc_is_sec_get_sysfs_pinfo(struct fimc_is_from_info **pinfo);
int fimc_is_sec_get_sysfs_finfo_front(struct fimc_is_from_info **finfo);
int fimc_is_sec_get_sysfs_pinfo_front(struct fimc_is_from_info **pinfo);
int fimc_is_sec_get_front_cal_buf(char **buf);

int fimc_is_sec_get_cal_buf(char **buf);
int fimc_is_sec_get_loaded_fw(char **buf);
int fimc_is_sec_set_loaded_fw(char *buf);
int fimc_is_sec_get_loaded_c1_fw(char **buf);
int fimc_is_sec_set_loaded_c1_fw(char *buf);

int fimc_is_sec_get_camid_from_hal(char *fw_name, char *setf_name);
int fimc_is_sec_get_camid(void);
int fimc_is_sec_set_camid(int id);
int fimc_is_sec_get_pixel_size(char *header_ver);
int fimc_is_sec_sensorid_find(struct fimc_is_core *core);
int fimc_is_sec_sensorid_find_front(struct fimc_is_core *core);
int fimc_is_sec_fw_find(struct fimc_is_core *core);
int fimc_is_sec_run_fw_sel(struct device *dev, int position);

int fimc_is_sec_readfw(struct fimc_is_core *core);
int fimc_is_sec_read_setfile(struct fimc_is_core *core);
#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
int fimc_is_sec_inflate_fw(u8 **buf, unsigned long *size);
#endif
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_REAR) || defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
int fimc_is_sec_fw_sel_eeprom(struct device *dev, int id, bool headerOnly);
#endif
int fimc_is_sec_write_fw(struct fimc_is_core *core, struct device *dev);
#if !defined(CONFIG_CAMERA_EEPROM_SUPPORT_REAR)
int fimc_is_sec_readcal(struct fimc_is_core *core);
int fimc_is_sec_fw_sel(struct fimc_is_core *core, struct device *dev, bool headerOnly);
#endif
#ifdef CONFIG_COMPANION_USE
int fimc_is_sec_read_companion_fw(struct fimc_is_core *core);
int fimc_is_sec_concord_fw_sel(struct fimc_is_core *core, struct device *dev);
#endif
int fimc_is_sec_fw_revision(char *fw_ver);
int fimc_is_sec_fw_revision(char *fw_ver);
bool fimc_is_sec_fw_module_compare(char *fw_ver1, char *fw_ver2);
u8 fimc_is_sec_compare_ver(int position);
bool fimc_is_sec_check_from_ver(struct fimc_is_core *core, int position);

bool fimc_is_sec_check_fw_crc32(char *buf, u32 checksum_seed, unsigned long size);
bool fimc_is_sec_check_cal_crc32(char *buf, int id);
void fimc_is_sec_make_crc32_table(u32 *table, u32 id);

int fimc_is_sec_gpio_enable(struct exynos_platform_fimc_is *pdata, char *name, bool on);
int fimc_is_sec_core_voltage_select(struct device *dev, char *header_ver);
int fimc_is_sec_ldo_enable(struct device *dev, char *name, bool on);
int fimc_is_sec_rom_power_on(struct fimc_is_core *core, int position);
int fimc_is_sec_rom_power_off(struct fimc_is_core *core, int position);
int fimc_is_sec_check_bin_files(struct fimc_is_core *core);
void remove_dump_fw_file(void);
void fimc_is_get_rear_dual_cal_buf(char **buf, int *size);
#endif /* FIMC_IS_SEC_DEFINE_H */
