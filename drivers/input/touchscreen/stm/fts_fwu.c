
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#include "fts_ts.h"

#define WRITE_CHUNK_SIZE 64
#define FTS_DEFAULT_UMS_FW "/sdcard/Firmware/TSP/stm.fw"
#define FTS_DEFAULT_FFU_FW "ffu_tsp.bin"
#define FTSFILE_SIGNATURE 0xAAAA5555

enum {
	BUILT_IN = 0,
	UMS,
	NONE,
	FFU,
};

struct fts_header {
	unsigned int signature;
	unsigned short fw_ver;
	unsigned char fw_id;
	unsigned char reserved1;
	unsigned char internal_ver[8];
	unsigned char released_ver[8];
	unsigned int reserved2;
	unsigned int checksum;
};

#define	FW_IMAGE_SIZE_D3	(256 * 1024)
#define	SIGNEDKEY_SIZE		(256)

int FTS_Check_DMA_Done(struct fts_ts_info *info)
{
	int timeout = 60;
	unsigned char regAdd[2] = { 0xF9, 0x05};
	unsigned char val[1];

	do {
		info->fts_read_reg(info, &regAdd[0], 2, (unsigned char*)val, 1);

		if ((val[0] & 0x80) != 0x80)
			break;

		fts_delay(50);
		timeout--;
	} while (timeout != 0);

	if (timeout == 0)
		return -1;

	return 0;
}

static int FTS_Check_Erase_Done(struct fts_ts_info *info)
{
	int timeout = 60;  // 3 sec timeout
	unsigned char regAdd[2] = {0xF9, 0x02};
	unsigned char val[1];

	do {
		info->fts_read_reg(info, &regAdd[0], 2, (unsigned char*)val, 1);

		if ((val[0] & 0x80) != 0x80)
			break;

		fts_delay(50);
		timeout--;
	} while (timeout != 0);

	if (timeout == 0)
		return -1;

	return 0;
}

static int fts_fw_burn_d3(struct fts_ts_info *info, unsigned char *fw_data)
{
	int rc;
	const unsigned long int FTS_CODE_SIZE = (244 * 1024);	// Total 244kB for Code
	const unsigned long int FTS_CONFIG_SIZE = (4 * 1024);	// Total 4kB for Config
	const unsigned long int DRAM_LEN = (64 * 1024);	// 64KB
	const unsigned int CODE_ADDR_START = (0x0000);
	const unsigned int CONFIG_ADDR_START = (0xFC00);
	const unsigned int WRITE_CHUNK_SIZE_D3 = 32;

	unsigned char *config_data = NULL;

	unsigned long int size = 0;
	unsigned long int i;
	unsigned long int j;
	unsigned long int k;
	unsigned long int dataLen;
	unsigned long int len = 0;
	unsigned long int writeAddr = 0;
	unsigned char buf[WRITE_CHUNK_SIZE_D3 + 3];
	unsigned char regAdd[8] = {0};
	int cnt;

	//==================== System reset ====================
	//System Reset ==> F7 52 34
	regAdd[0] = 0xF7;
	regAdd[1] = 0x52;
	regAdd[2] = 0x34;
	info->fts_write_reg(info, &regAdd[0], 3);
	fts_delay(200);

	//==================== Unlock Flash ====================
	//Unlock Flash Command ==> F7 74 45
	regAdd[0] = 0xF7;
	regAdd[1] = 0x74;
	regAdd[2] = 0x45;
	info->fts_write_reg(info, &regAdd[0], 3);
	fts_delay(100);

	//==================== Unlock Erase Operation ====================
	regAdd[0] = 0xFA;
	regAdd[1] = 0x72;
	regAdd[2] = 0x01;
	info->fts_write_reg(info, &regAdd[0], 3);
	fts_delay(100);

	//==================== Erase Partial Flash ====================
	for (i = 0; i < 64; i++) {
		if ( (i == 61) || (i == 62) )   // skip CX2 area (page 61 and page 62)
			continue;

		regAdd[0] = 0xFA;
		regAdd[1] = 0x02;
		regAdd[2] = (0x80 + i) & 0xFF;
		info->fts_write_reg(info, &regAdd[0], 3);
		rc = FTS_Check_Erase_Done(info);
		if (rc < 0)
			return rc;
	}

	//==================== Unlock Programming operation ====================
	regAdd[0] = 0xFA;
	regAdd[1] = 0x72;
	regAdd[2] = 0x02;
	info->fts_write_reg(info, &regAdd[0], 3);

	//========================== Write to FLASH ==========================
	// Main Code Programming
	i = 0;
	k = 0;
	size = FTS_CODE_SIZE;

	while(i < size) {
		j = 0;
		dataLen = size - i;

		while ((j < DRAM_LEN) && (j < dataLen)) {	//DRAM_LEN = 64*1024
			writeAddr = j & 0xFFFF;

			cnt = 0;
			buf[cnt++] = 0xF8;
			buf[cnt++] = (writeAddr >> 8) & 0xFF;
			buf[cnt++] = (writeAddr >> 0) & 0xFF;

			memcpy(&buf[cnt], &fw_data[sizeof(struct fts_header) + i], WRITE_CHUNK_SIZE_D3);
			cnt += WRITE_CHUNK_SIZE_D3;

			info->fts_write_reg(info, &buf[0], cnt);

			i += WRITE_CHUNK_SIZE_D3;
			j += WRITE_CHUNK_SIZE_D3;
		}
		input_info(true, info->dev, "%s: Write to Flash - Total %ld bytes\n", __func__, i);

		 //===================configure flash DMA=====================
		len = j / 4 - 1; 	// 64*1024 / 4 - 1

		buf[0] = 0xFA;
		buf[1] = 0x06;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = (CODE_ADDR_START +( (k * DRAM_LEN) >> 2)) & 0xFF;			// k * 64 * 1024 / 4
		buf[5] = (CODE_ADDR_START + ((k * DRAM_LEN) >> (2+8))) & 0xFF;		// k * 64 * 1024 / 4 / 256
		buf[6] = (len >> 0) & 0xFF;    //DMA length in word
		buf[7] = (len >> 8) & 0xFF;    //DMA length in word
		buf[8] = 0x00;
		info->fts_write_reg(info, &buf[0], 9);

		fts_delay(100);

		//===================START FLASH DMA=====================
		buf[0] = 0xFA;
		buf[1] = 0x05;
		buf[2] = 0xC0;
		info->fts_write_reg(info, &buf[0], 3);

		rc = FTS_Check_DMA_Done(info);
		if (rc < 0)
			return rc;
		k++;
	}
	input_info(true, info->dev, "%s: Total write %ld kbytes for Main Code\n", __func__, i / 1024);

	//=============================================================
	// Config Programming
	//=============================================================

	config_data = kzalloc(FTS_CONFIG_SIZE, GFP_KERNEL);
	if (!config_data) {
		input_err(true, info->dev, "%s: failed to alloc mem\n",
				__func__);
		return -ENOMEM;
	}

	memcpy(&config_data[0], &fw_data[sizeof(struct fts_header) + (252 * 1024)], FTS_CONFIG_SIZE);

	i = 0;
	size = FTS_CONFIG_SIZE;
	j = 0;
	while ((j < DRAM_LEN) && (j < size)) {		//DRAM_LEN = 64*1024
		writeAddr = j & 0xFFFF;

		cnt = 0;
		buf[cnt++] = 0xF8;
		buf[cnt++] = (writeAddr >> 8) & 0xFF;
		buf[cnt++] = (writeAddr >> 0) & 0xFF;

		memcpy(&buf[cnt], &config_data[i], WRITE_CHUNK_SIZE_D3);
		cnt += WRITE_CHUNK_SIZE_D3;

		info->fts_write_reg(info, &buf[0], cnt);

		i += WRITE_CHUNK_SIZE_D3;
		j += WRITE_CHUNK_SIZE_D3;
	}
	kfree(config_data);

	//===================configure flash DMA=====================
	len = j / 4 - 1;

	buf[0] = 0xFA;
	buf[1] = 0x06;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = (CONFIG_ADDR_START) & 0xFF;
	buf[5] = (CONFIG_ADDR_START >> 8) & 0xFF;
	buf[6] = (len >> 0) & 0xFF;    //DMA length in word
	buf[7] = (len >> 8) & 0xFF;    //DMA length in word
	buf[8] = 0x00;
	info->fts_write_reg(info, &buf[0], 9);

	fts_delay(100);

	//===================START FLASH DMA=====================
	buf[0] = 0xFA;
	buf[1] = 0x05;
	buf[2] = 0xC0;
	info->fts_write_reg(info, &buf[0], 3);

	rc = FTS_Check_DMA_Done(info);
	if (rc < 0)
		return rc;

	input_info(true, info->dev, "%s: Total write %ld kbytes for Config\n", __func__, i / 1024);

	//==================== System reset ====================
	//System Reset ==> F7 52 34
	regAdd[0] = 0xF7;		regAdd[1] = 0x52;		regAdd[2] = 0x34;
	info->fts_write_reg(info, &regAdd[0],3);

	return 0;
}

int fts_fw_wait_for_event(struct fts_ts_info *info, unsigned char eid)
{
	int rc;
	unsigned char regAdd;
	unsigned char data[FTS_EVENT_SIZE];
	int retry = 0;

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd = READ_ONE_EVENT;
	rc = -1;
	while (info->fts_read_reg(info, &regAdd, 1, (unsigned char *)data, FTS_EVENT_SIZE)) {
		if (data[0] == EVENTID_STATUS_EVENT || data[0] == EVENTID_ERROR) {
			if ((data[0] == EVENTID_STATUS_EVENT) && (data[1] == eid)) {
				rc = 0;
				break;
			} else {
				input_info(true, info->dev, "%s: %2X,%2X,%2X,%2X\n", __func__, data[0],data[1],data[2],data[3]);
			}
		} else if (data[0] == EVENTID_FROM_SPONGE && data[1] == FTS_STRING_EVENT_PRESSURE_RELEASED) {
			info->force_release = true;
			input_info(true, info->dev, "%s: %2X,%2X,%2X,%2X  force_release %d\n",
					__func__, data[0], data[1], data[2], data[3], info->force_release);
		}

		if (retry++ > FTS_RETRY_COUNT * 15) {
			rc = -1;
			input_err(true, info->dev, "%s: Time Over (%2X,%2X,%2X,%2X)\n", __func__, data[0],data[1],data[2],data[3]);
			break;
		}
		fts_delay(20);
	}

	return rc;
}

int fts_fw_wait_for_event_D3(struct fts_ts_info *info, unsigned char eid0, unsigned char eid1)
{
	int rc;
	unsigned char regAdd;
	unsigned char data[FTS_EVENT_SIZE];
	int retry = 0;

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd = READ_ONE_EVENT;
	rc = -1;
	while (info->fts_read_reg(info, &regAdd, 1, (unsigned char *)data, FTS_EVENT_SIZE)) {
		if (data[0] == EVENTID_STATUS_EVENT || data[0] == EVENTID_ERROR) {
			if ((data[0] == EVENTID_STATUS_EVENT) && (data[1] == eid0) && (data[2] == eid1)) {
				rc = 0;
				break;
			} else {
				input_info(true, info->dev, "%s: %2X,%2X,%2X,%2X\n", __func__, data[0],data[1],data[2],data[3]);
			}
		}
		if (retry++ > FTS_RETRY_COUNT * 15) {
			rc = -1;
			input_err(true, info->dev, "%s: Time Over (%2X,%2X,%2X,%2X)\n", __func__, data[0],data[1],data[2],data[3]);
			break;
		}
		fts_delay(20);
	}

	return rc;
}

int fts_fw_wait_for_specific_event(struct fts_ts_info *info, unsigned char eid0, unsigned char eid1, unsigned char eid2)
{
	int rc;
	unsigned char regAdd;
	unsigned char data[FTS_EVENT_SIZE];
	int retry = 0;

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd = READ_ONE_EVENT;
	rc = -1;
	while (info->fts_read_reg(info, &regAdd, 1, (unsigned char *)data, FTS_EVENT_SIZE)) {
		if (data[0]) {
			if ((data[0] == eid0) && (data[1] == eid1) && (data[2] == eid2)) {
				rc = 0;
				break;
			} else {
				input_info(true, info->dev, "%s: %2X,%2X,%2X,%2X\n", __func__, data[0],data[1],data[2],data[3]);
			}
		}
		if (retry++ > FTS_RETRY_COUNT * 15) {
			rc = -1;
			input_err(true, info->dev, "%s: Time Over ( %2X,%2X,%2X,%2X )\n", __func__, data[0],data[1],data[2],data[3]);
			break;
		}
		fts_delay(20);
	}

	return rc;
}

void fts_execute_autotune(struct fts_ts_info *info)
{
	input_info(true, info->dev, "%s: start\n", __func__);

	info->fts_command(info, CX_TUNNING);
	fts_delay(300);
	fts_fw_wait_for_event_D3(info, STATUS_EVENT_MUTUAL_AUTOTUNE_DONE, 0x00);
	info->fts_command(info, SELF_AUTO_TUNE);
	fts_delay(300);
	fts_fw_wait_for_event_D3(info, STATUS_EVENT_SELF_AUTOTUNE_DONE_D3, 0x00);

	info->fts_command(info, FTS_CMD_SAVE_CX_TUNING);
	fts_delay(230);
	fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CXTUNE_VALUE);

	info->fts_command(info, FTS_CMD_SAVE_FWCONFIG);
	fts_delay(230);
	fts_fw_wait_for_event(info, STATUS_EVENT_FLASH_WRITE_CONFIG);

	/* Reset FTS */
	info->fts_systemreset(info, 30);
}

void fts_fw_init(struct fts_ts_info *info, bool magic_cal)
{
	input_info(true, info->dev, "%s: magic_cal(%d)\n", __func__,magic_cal);

	info->fts_command(info, FTS_CMD_TRIM_LOW_POWER_OSCILLATOR);
	fts_delay(200);

#ifdef PAT_CONTROL
	/* count the number of calibration */
	fts_get_calibration_information(info);

	if ((info->cal_count == 0) || (info->cal_count == 0xFF) || (magic_cal == true)) {
		input_err(true, info->dev, "%s: RUN OFFSET CALIBRATION(0x%02X)\n", __func__, info->cal_count);
		fts_execute_autotune(info);
		if (magic_cal) {
			if (info->cal_count == 0x00 || info->cal_count == 0xFF)
				info->cal_count = PAT_MAGIC_NUMBER;
			else if (PAT_MAGIC_NUMBER <= info->cal_count && info->cal_count < PAT_MAX_MAGIC)
				info->cal_count++;
			input_info(true, info->dev, "%s: write to nvm %X\n",
						__func__, info->cal_count);
		}

		/* get ic information */
		info->fts_get_version_info(info);

		info->tune_fix_ver = info->fw_main_version_of_ic;
		fts_set_calibration_information(info, info->cal_count, info->fw_main_version_of_ic);

		fts_get_calibration_information(info);

		input_info(true, info->dev, "%s: tune_fix_ver [0x%4X]\n", __func__, info->tune_fix_ver);
	} else {
		input_err(true, info->dev, "%s: DO NOT CALIBRATION(0x%02X)\n", __func__, info->cal_count);
	}
#endif

	info->fts_command(info, SENSEON);
#ifdef FTS_SUPPORT_PRESSURE_SENSOR
	info->fts_command(info, FTS_CMD_PRESSURE_SENSE_ON);
#endif
	fts_fw_wait_for_event (info, STATUS_EVENT_FORCE_CAL_DONE_D3);

#ifdef FTS_SUPPORT_TOUCH_KEY
	if (info->board->support_mskey)
		info->fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif

	fts_interrupt_set(info, INT_ENABLE);
	fts_delay(20);
}

const int fts_fw_updater(struct fts_ts_info *info, unsigned char *fw_data, int restore_cal)
{
	const struct fts_header *header;
	int retval;
	int retry;
	unsigned short fw_main_version;
	bool magic_cal = false;

	if (!fw_data) {
		input_err(true, info->dev, "%s: Firmware data is NULL\n",
			__func__);
		return -ENODEV;
	}

	header = (struct fts_header *)fw_data;
	fw_main_version = (header->released_ver[0] << 8) + (header->released_ver[1]);

	input_info(true, info->dev,
		  "%s: Starting firmware update : 0x%04X\n", __func__,
		  fw_main_version);

	retry = 0;
	while (1) {

		retval = fts_fw_burn_d3(info, fw_data);
		if (retval >= 0) {
			info->fts_wait_for_ready(info);
			info->fts_get_version_info(info);

#ifdef FTS_SUPPORT_NOISE_PARAM
			info->fts_get_noise_param_address(info);
#endif

			if (fw_main_version == info->fw_main_version_of_ic) {
				input_info(true, info->dev,
					  "%s: Success Firmware update\n",
					  __func__);

#ifdef PAT_CONTROL
				if (restore_cal) {
					if(info->board->pat_function == PAT_CONTROL_PAT_MAGIC) {
						/* NOT to control cal count that was marked on external factory ( E0~E5 )*/
						if ((info->cal_count >= PAT_MAGIC_NUMBER) && (info->cal_count < PAT_MAX_MAGIC))
							magic_cal = true;
					}
				}
				input_info(true, info->dev, "%s: cal_count(0x%02X) pat_function dt(%d) restore_cal(%d) magic_cal(%d)\n",
					__func__,info->cal_count,info->board->pat_function,restore_cal,magic_cal);
#endif
				fts_fw_init(info, magic_cal);
				retval = 0;
				break;
			}
		}

		if (retry++ > 3) {
			input_err(true, info->dev, "%s: Fail Firmware update\n",
				 __func__);
			retval = -1;
			break;
		}
	}

	return retval;
}
EXPORT_SYMBOL(fts_fw_updater);

#define FW_IMAGE_NAME_FTS8			"tsp_stm/fts8cd56_dream2.fw"

int fts_fw_update_on_probe(struct fts_ts_info *info)
{
	int retval = 0;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;
	char fw_path[FTS_MAX_FW_PATH];
	const struct fts_header *header;
	int restore_cal = 0;
	bool cal_clear = false;

	if (info->board->bringup == 1)
		return 0;

	if (info->board->firmware_name) {
		if (gpio_is_valid(info->board->device_id) && gpio_get_value(info->board->device_id)) {
			retval = fts_read_analog_chip_id(info, ANALOG_ID_FTS8);
			if (retval == 1)
				info->firmware_name = FW_IMAGE_NAME_FTS8;
			else
				info->firmware_name = info->board->firmware_name;
		}
		else
			info->firmware_name = info->board->firmware_name;
	}
	else {
		input_err(true, info->dev,"%s: firmware name does not declair in dts\n", __func__);
		goto exit_fwload;
	}

	snprintf(fw_path, FTS_MAX_FW_PATH, "%s", info->firmware_name);
	input_info(true, info->dev, "%s: Load firmware : %s, TSP_ID : %d\n", __func__, fw_path, info->board->tsp_id);

	retval = request_firmware(&fw_entry, fw_path, info->dev);
	if (retval) {
		input_err(true, info->dev,
			"%s: Firmware image %s not available\n", __func__,
			fw_path);
		goto done;
	}

	if (fw_entry->size != (FW_IMAGE_SIZE_D3 + sizeof(struct fts_header))) {
		input_err(true, info->dev,
			"%s: Firmware image %s not available for FTS D3\n", __func__,
			fw_path);
		goto done;
	}

	fw_data = (unsigned char *)fw_entry->data;
	header = (struct fts_header *)fw_data;

	info->fw_version_of_bin = (fw_data[5] << 8)+fw_data[4];
	info->fw_main_version_of_bin = (header->released_ver[0] << 8) + (header->released_ver[1]);
	info->config_version_of_bin = (fw_data[CONFIG_OFFSET_BIN_D3] << 8) + fw_data[CONFIG_OFFSET_BIN_D3 - 1];

	input_info(true, info->dev,
		"%s: [BIN] Firmware Ver: 0x%04X, Config Ver: 0x%04X, Main Ver: 0x%04X\n", __func__,
		info->fw_version_of_bin,
		info->config_version_of_bin,
		info->fw_main_version_of_bin);

	if (info->board->bringup == 2) {
		input_err(true, info->dev, "%s: skip fw_update for bringup\n", __func__);
		retval = FTS_NOT_ERROR;
		goto done;
	}

#ifdef PAT_CONTROL
	if ((info->fw_main_version_of_ic < info->fw_main_version_of_bin)
		|| ((info->config_version_of_ic < info->config_version_of_bin))
		|| ((info->fw_version_of_ic < info->fw_version_of_bin))
		|| (info->boot_crc_check_fail == FTS_BOOT_CRC_FAIL))
		retval = FTS_NEED_FW_UPDATE;
	else
		retval = FTS_NOT_ERROR;

	/* ic fw ver > bin fw ver && force is false*/
	if (retval != FTS_NEED_FW_UPDATE) {
		/* clear nv,  forced f/w update eventhough same f/w,  then apply pat magic */
		if (info->board->pat_function == PAT_CONTROL_FORCE_UPDATE) {
			input_info(true, info->dev, "%s: run forced f/w update and excute autotune\n", __func__);
		} else {
			input_info(true, info->dev, "%s: skip fw update & nv read\n", __func__);
			goto done;
		}
	}

	/* load  pat information variable from ic */
	fts_get_calibration_information(info);
	input_info(true, info->dev, "%s: tune_fix_ver [%04X] afe_base [%04X]\n", __func__, info->tune_fix_ver, info->board->afe_base);

	/* initialize default flash value from 0xff to 0x00  for stm */
	if (info->cal_count == 0xFF) {
		cal_clear = true;
		info->tune_fix_ver = 0x0000;
	}

	/* check dt to clear pat */
	if (info->board->pat_function == PAT_CONTROL_CLEAR_NV
		|| info->board->pat_function == PAT_CONTROL_FORCE_UPDATE) {
		cal_clear = true;
	}

	/* mismatch calibration - ic has too old calibration data after pat enabled*/
	if (info->board->afe_base > info->tune_fix_ver) {
		restore_cal = 1;
		cal_clear = true;
	}

	if (cal_clear) {
		fts_set_calibration_information(info, PAT_COUNT_ZERO, info->tune_fix_ver);
		fts_get_calibration_information(info);
	}

	retval = fts_fw_updater(info, fw_data, restore_cal);

	/* change cal_count from 0 to magic number to make virtual pure auto tune */
	if ((info->cal_count == 0 && info->board->pat_function == PAT_CONTROL_PAT_MAGIC )||
			info->board->pat_function == PAT_CONTROL_FORCE_UPDATE) {
		fts_set_calibration_information(info, PAT_MAGIC_NUMBER, info->tune_fix_ver);
		fts_get_calibration_information(info);
	}
#else
	if ((info->fw_main_version_of_ic < info->fw_main_version_of_bin)
		|| ((info->config_version_of_ic < info->config_version_of_bin))
		|| ((info->fw_version_of_ic < info->fw_version_of_bin))
		|| (info->boot_crc_check_fail == FTS_BOOT_CRC_FAIL))
		retval = fts_fw_updater(info, fw_data, restore_cal);
	else
		retval = FTS_NOT_ERROR;
#endif

done:
	if (fw_entry)
		release_firmware(fw_entry);
exit_fwload:
	return retval;
}
EXPORT_SYMBOL(fts_fw_update_on_probe);

static int fts_load_fw_from_kernel(struct fts_ts_info *info,
				 const char *fw_path)
{
	int retval;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;

	if (!fw_path) {
		input_err(true, info->dev, "%s: Firmware name is not defined\n",
			__func__);
		return -EINVAL;
	}

	input_info(true, info->dev, "%s: Load firmware : %s\n", __func__,
		 fw_path);

	retval = request_firmware(&fw_entry, fw_path, info->dev);

	if (retval) {
		input_err(true, info->dev,
			"%s: Firmware image %s not available\n", __func__,
			fw_path);
		goto done;
	}

	fw_data = (unsigned char *)fw_entry->data;

	disable_irq(info->irq);

	info->fts_systemreset(info, 10);

	/* use virtual pat_control - magic cal 1 */
	retval = fts_fw_updater(info, fw_data, 1);
	if (retval)
		input_err(true, info->dev, "%s: failed update firmware\n",
			__func__);

	enable_irq(info->irq);
 done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}

static int fts_load_fw_from_ums(struct fts_ts_info *info)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fw_size, nread;
	int error = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(FTS_DEFAULT_UMS_FW, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		input_err(true, info->dev, "%s: failed to open %s.\n", __func__,
			FTS_DEFAULT_UMS_FW);
		error = -ENOENT;
		goto open_err;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (0 < fw_size) {
		unsigned char *fw_data;
		const struct fts_header *header;

		fw_data = kzalloc(fw_size, GFP_KERNEL);
		if (!fw_data) {
			input_err(true, info->dev, "%s: failed to alloc mem\n",
					__func__);
			error = -ENOMEM;
			goto alloc_err;
		}

		nread = vfs_read(fp, (char __user *)fw_data,
				 fw_size, &fp->f_pos);

		input_info(true, info->dev,
			 "%s: start, file path %s, size %ld Bytes\n",
			 __func__, FTS_DEFAULT_UMS_FW, fw_size);

		if (nread != fw_size) {
			input_err(true, info->dev,
				"%s: failed to read firmware file, nread %ld Bytes\n",
				__func__, nread);
			error = -EIO;
		} else {
			header = (struct fts_header *)fw_data;
			if (header->signature == FTSFILE_SIGNATURE) {

				disable_irq(info->irq);

				info->fts_systemreset(info, 10);

				input_info(true, info->dev,
					"%s: [UMS] Firmware Ver: 0x%04X, Main Version : 0x%04X\n",
					__func__, (fw_data[5] << 8)+fw_data[4],
					(header->released_ver[0] << 8) +
					(header->released_ver[1]));

				/* use virtual pat_control - magic cal 1 */
				error = fts_fw_updater(info, fw_data, 1);

				enable_irq(info->irq);

			} else {
				error = -1;
				input_err(true, info->dev,
					 "%s: File type is not match with FTS64 file. [%8x]\n",
					 __func__, header->signature);
			}
		}

		if (error < 0)
			input_err(true, info->dev, "%s: failed update firmware\n",
				__func__);

		kfree(fw_data);
	}

alloc_err:
	filp_close(fp, NULL);

 open_err:
	set_fs(old_fs);
	return error;
}

static int fts_load_fw_from_ffu(struct fts_ts_info *info)
{
	int retval;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;
	const char *fw_path = FTS_DEFAULT_FFU_FW;
	const struct fts_header *header;

	if (!fw_path) {
		input_err(true, info->dev, "%s: Firmware name is not defined\n",
			__func__);
		return -EINVAL;
	}

	input_info(true, info->dev, "%s: Load firmware : %s\n", __func__,
		 fw_path);

	retval = request_firmware(&fw_entry, fw_path, info->dev);

	if (retval) {
		input_err(true, info->dev,
			"%s: Firmware image %s not available\n", __func__,
			fw_path);
		goto done;
	}

	if (fw_entry->size != (FW_IMAGE_SIZE_D3 + sizeof(struct fts_header) + SIGNEDKEY_SIZE)) {
		input_err(true, info->dev,
			"%s: Unsigned firmware %s is not available, %ld\n", __func__,
			fw_path, fw_entry->size);
		retval = -EPERM;
		goto done;
	}

	fw_data = (unsigned char *)fw_entry->data;
	header = (struct fts_header *)fw_data;

	info->fw_version_of_bin = (fw_data[5] << 8)+fw_data[4];
	info->fw_main_version_of_bin = (header->released_ver[0] << 8) + (header->released_ver[1]);
	info->config_version_of_bin = (fw_data[CONFIG_OFFSET_BIN_D3] << 8) + fw_data[CONFIG_OFFSET_BIN_D3 - 1];

	input_info(true, info->dev,
		"%s: [FFU] Firmware Ver: 0x%04X, Config Ver: 0x%04X, Main Ver: 0x%04X\n",
		__func__, info->fw_version_of_bin, info->config_version_of_bin,
		info->fw_main_version_of_bin);

	disable_irq(info->irq);

	info->fts_systemreset(info, 10);

	/* use virtual pat_control - magic cal 0 */
	retval = fts_fw_updater(info, fw_data, 0);
	if (retval)
		input_err(true, info->dev, "%s: failed update firmware\n", __func__);

	enable_irq(info->irq);

done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}

int fts_fw_update_on_hidden_menu(struct fts_ts_info *info, int update_type)
{
	int retval = 0;

	/* Factory cmd for firmware update
	 * argument represent what is source of firmware like below.
	 *
	 * 0 : [BUILT_IN] Getting firmware which is for user.
	 * 1 : [UMS] Getting firmware from sd card.
	 * 2 : none
	 * 3 : [FFU] Getting firmware from air.
	 */
	switch (update_type) {
	case BUILT_IN:
		retval = fts_load_fw_from_kernel(info, info->firmware_name);
		break;

	case UMS:
		retval = fts_load_fw_from_ums(info);
		break;

	case FFU:
		retval = fts_load_fw_from_ffu(info);
		break;

	default:
		input_err(true, info->dev, "%s: Not support command[%d]\n",
			__func__, update_type);
		break;
	}

	return retval;
}
EXPORT_SYMBOL(fts_fw_update_on_hidden_menu);

