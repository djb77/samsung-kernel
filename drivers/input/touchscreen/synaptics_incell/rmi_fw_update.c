/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
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
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/firmware.h>
#include "synaptics_i2c_rmi.h"

#define DO_STARTUP_FW_UPDATE
#define STARTUP_FW_UPDATE_DELAY_MS 1000 /* ms */
#define FORCE_UPDATE false
#define DO_LOCKDOWN false

#define MAX_IMAGE_NAME_LEN 256
#define MAX_FIRMWARE_ID_LEN 10

#define F01_DEVICE_STATUS	0X0004

#define BOOTLOADER_ID_OFFSET 0
#define BLOCK_NUMBER_OFFSET 0

#define V5V6_BOOTLOADER_ID_OFFSET 0
#define V5V6_CONFIG_ID_SIZE 4

#define V5_PROPERTIES_OFFSET 2
#define V5_BLOCK_SIZE_OFFSET 3
#define V5_BLOCK_COUNT_OFFSET 5
#define V5_BLOCK_NUMBER_OFFSET 0
#define V5_BLOCK_DATA_OFFSET 2

#define V6_PROPERTIES_OFFSET 1
#define V6_BLOCK_SIZE_OFFSET 2
#define V6_BLOCK_COUNT_OFFSET 3
#define V6_PROPERTIES_2_OFFSET 4
#define V6_GUEST_CODE_BLOCK_COUNT_OFFSET 5
#define V6_BLOCK_NUMBER_OFFSET 0
#define V6_BLOCK_DATA_OFFSET 1
#define V6_FLASH_COMMAND_OFFSET 2
#define V6_FLASH_STATUS_OFFSET 3

#define V7_FLASH_STATUS_OFFSET 0
#define V7_PARTITION_ID_OFFSET 1
#define V7_BLOCK_NUMBER_OFFSET 2
#define V7_TRANSFER_LENGTH_OFFSET 3
#define V7_COMMAND_OFFSET 4
#define V7_PAYLOAD_OFFSET 5

#define V7_PARTITION_SUPPORT_BYTES 4

#define V7_CONFIG_ID_SIZE 32

#define IMG_VERSION_OFFSET 0x07
#define IMG_X10_TOP_CONTAINER_OFFSET 0x0C
#define IMG_X0_X6_FW_OFFSET 0x100

#define UI_CONFIG_AREA 0x00
#define PERM_CONFIG_AREA 0x01
#define BL_CONFIG_AREA 0x02
#define DISP_CONFIG_AREA 0x03

#define SLEEP_MODE_NORMAL (0x00)
#define SLEEP_MODE_SENSOR_SLEEP (0x01)
#define SLEEP_MODE_RESERVED0 (0x02)
#define SLEEP_MODE_RESERVED1 (0x03)

#define ENABLE_WAIT_MS (1 * 1000)
#define WRITE_WAIT_MS (3 * 1000)
#define ERASE_WAIT_MS (5 * 1000)

#define MIN_SLEEP_TIME_US 50
#define MAX_SLEEP_TIME_US 100
#define STATUS_POLLING_PERIOD_US 3000

#define POLLING_MODE_DEFAULT 0

#define ATTRIBUTE_FOLDER_NAME "fwu"

enum f34_version {
	F34_V0 = 0,
	F34_V1,
	F34_V2,
};
static int fwu_write_f34_command(struct synaptics_rmi4_data *rmi4_data, unsigned char cmd);

static ssize_t fwu_sysfs_show_image(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count);

static ssize_t fwu_sysfs_store_image(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count);

static ssize_t fwu_sysfs_do_reflash_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);

static ssize_t fwu_sysfs_write_config_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);

static ssize_t fwu_sysfs_read_config_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);

static ssize_t fwu_sysfs_config_area_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);

static ssize_t fwu_sysfs_image_size_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);

static ssize_t fwu_sysfs_block_size_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf);

static ssize_t fwu_sysfs_firmware_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf);

static ssize_t fwu_sysfs_configuration_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf);

static ssize_t fwu_sysfs_perm_config_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf);

static ssize_t fwu_sysfs_bl_config_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf);

static ssize_t fwu_sysfs_disp_config_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf);

static ssize_t fwu_sysfs_guest_code_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf);

static ssize_t fwu_sysfs_write_guest_code_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);

static void fwu_img_parse_format(struct synaptics_rmi4_data *rmi4_data);
static int fwu_do_write_guest_code(struct synaptics_rmi4_data *rmi4_data);
static int fwu_scan_pdt(struct synaptics_rmi4_data *rmi4_data);
static void synaptics_rmi4_fwu_reset(struct synaptics_rmi4_data *rmi4_data);
static int fwu_wait_for_idle(struct synaptics_rmi4_data *rmi4_data, int timeout_ms);

static struct bin_attribute dev_attr_data = {
	.attr = {
		.name = "data",
		.mode = (S_IRUGO | S_IWUSR | S_IWGRP),
	},
	.size = 0,
	.read = fwu_sysfs_show_image,
	.write = fwu_sysfs_store_image,
};

RMI_KOBJ_ATTR(doreflash, S_IWUSR | S_IWGRP,	synaptics_rmi4_show_error, fwu_sysfs_do_reflash_store);
RMI_KOBJ_ATTR(writeconfig, S_IWUSR | S_IWGRP, synaptics_rmi4_show_error, fwu_sysfs_write_config_store);
RMI_KOBJ_ATTR(readconfig, S_IWUSR | S_IWGRP, synaptics_rmi4_show_error,	fwu_sysfs_read_config_store);
RMI_KOBJ_ATTR(configarea, S_IWUSR | S_IWGRP, synaptics_rmi4_show_error,	fwu_sysfs_config_area_store);
RMI_KOBJ_ATTR(imagesize, S_IWUSR | S_IWGRP,	synaptics_rmi4_show_error, fwu_sysfs_image_size_store);
RMI_KOBJ_ATTR(blocksize, S_IRUGO, fwu_sysfs_block_size_show, synaptics_rmi4_store_error);
RMI_KOBJ_ATTR(fwblockcount, S_IRUGO, fwu_sysfs_firmware_block_count_show, synaptics_rmi4_store_error);
RMI_KOBJ_ATTR(configblockcount, S_IRUGO, fwu_sysfs_configuration_block_count_show, synaptics_rmi4_store_error);
RMI_KOBJ_ATTR(permconfigblockcount, S_IRUGO, fwu_sysfs_perm_config_block_count_show, synaptics_rmi4_store_error);
RMI_KOBJ_ATTR(blconfigblockcount, S_IRUGO, fwu_sysfs_bl_config_block_count_show, synaptics_rmi4_store_error);
RMI_KOBJ_ATTR(dispconfigblockcount, S_IRUGO, fwu_sysfs_disp_config_block_count_show, synaptics_rmi4_store_error);
RMI_KOBJ_ATTR(guestcodeblockcount, S_IRUGO, fwu_sysfs_guest_code_block_count_show, synaptics_rmi4_store_error);
RMI_KOBJ_ATTR(writeguestcode, S_IWUSR | S_IWGRP, synaptics_rmi4_show_error, fwu_sysfs_write_guest_code_store);

static struct attribute *attrs[] = {
	&kobj_attr_doreflash.attr,
	&kobj_attr_writeconfig.attr,
	&kobj_attr_readconfig.attr,
	&kobj_attr_configarea.attr,
	&kobj_attr_imagesize.attr,
	&kobj_attr_blocksize.attr,
	&kobj_attr_fwblockcount.attr,
	&kobj_attr_configblockcount.attr,
	&kobj_attr_permconfigblockcount.attr,
	&kobj_attr_blconfigblockcount.attr,
	&kobj_attr_dispconfigblockcount.attr,
	&kobj_attr_guestcodeblockcount.attr,
	&kobj_attr_writeguestcode.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static unsigned int extract_uint_le(const unsigned char *ptr)
{
	return (unsigned int)ptr[0] +
		(unsigned int)ptr[1] * 0x100 +
		(unsigned int)ptr[2] * 0x10000 +
		(unsigned int)ptr[3] * 0x1000000;
}

#ifdef FW_UPDATE_GO_NOGO
static unsigned int extract_uint_be(const unsigned char *ptr)
{
	return (unsigned int)ptr[3] +
			(unsigned int)ptr[2] * 0x100 +
			(unsigned int)ptr[1] * 0x10000 +
			(unsigned int)ptr[0] * 0x1000000;
}
#endif

static unsigned int le_to_uint(const unsigned char *ptr)
{
	return (unsigned int)ptr[0] +
			(unsigned int)ptr[1] * 0x100 +
			(unsigned int)ptr[2] * 0x10000 +
			(unsigned int)ptr[3] * 0x1000000;
}

static unsigned short extract_ushort_le(const unsigned char *ptr)
{
	return (unsigned int)ptr[0] + (unsigned int)ptr[1] * 0x100;
}
#define CHECKSUM_SIZE	4
static void synaptics_rmi_calculate_checksum(unsigned short *data,
				unsigned short len, unsigned long *result)
{
	unsigned long temp;
	unsigned long sum1 = 0xffff;
	unsigned long sum2 = 0xffff;

	*result = 0xffffffff;

	while (len--) {
		temp = *data;
		sum1 += temp;
		sum2 += sum1;
		sum1 = (sum1 & 0xffff) + (sum1 >> 16);
		sum2 = (sum2 & 0xffff) + (sum2 >> 16);
		data++;
	}

	*result = sum2 << 16 | sum1;

	return;
}

static void synaptics_rmi_convert_to_little_endian(unsigned char *dest,
				unsigned long src)
{
	dest[0] = (unsigned char)(src & 0xff);
	dest[1] = (unsigned char)((src >> 8) & 0xff);
	dest[2] = (unsigned char)((src >> 16) & 0xff);
	dest[3] = (unsigned char)((src >> 24) & 0xff);

	return;
}

static int fwu_write_f34_v7_command_single_transaction(struct synaptics_rmi4_data *rmi4_data, unsigned char cmd)
{
	int retval;
	unsigned char data_base;
	struct f34_v7_data_1_5 data_1_5;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	data_base = fwu->f34_fd.data_base_addr;

	memset(data_1_5.data, 0x00, sizeof(data_1_5.data));

	switch (cmd) {
	case CMD_ERASE_ALL:
		data_1_5.partition_id = CORE_CODE_PARTITION;
		data_1_5.command = CMD_V7_ERASE_AP;
		break;
	case CMD_ERASE_UI_FIRMWARE:
		data_1_5.partition_id = CORE_CODE_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_BL_CONFIG:
		data_1_5.partition_id = GLOBAL_PARAMETERS_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_UI_CONFIG:
		data_1_5.partition_id = CORE_CONFIG_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_DISP_CONFIG:
		data_1_5.partition_id = DISPLAY_CONFIG_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_FLASH_CONFIG:
		data_1_5.partition_id = FLASH_CONFIG_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_GUEST_CODE:
		data_1_5.partition_id = GUEST_CODE_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_BOOTLOADER:
		data_1_5.partition_id = BOOTLOADER_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_UTILITY_PARAMETER:
		data_1_5.partition_id = UTILITY_PARAMETER_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ENABLE_FLASH_PROG:
		data_1_5.partition_id = BOOTLOADER_PARTITION;
		data_1_5.command = CMD_V7_ENTER_BL;
		break;
	};

	data_1_5.payload_0 = fwu->bootloader_id[0];
	data_1_5.payload_1 = fwu->bootloader_id[1];

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.partition_id,
			data_1_5.data,
			sizeof(data_1_5.data));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write single transaction command\n",
				__func__);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_v7_command(struct synaptics_rmi4_data *rmi4_data, unsigned char cmd)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	int retval;
	unsigned char data_base;
	unsigned char command;

	data_base = fwu->f34_fd.data_base_addr;

	switch (cmd) {
	case CMD_WRITE_FW:
	case CMD_WRITE_CONFIG:
	case CMD_WRITE_LOCKDOWN:
	case CMD_WRITE_GUEST_CODE:
	case CMD_WRITE_BOOTLOADER:
	case CMD_WRITE_UTILITY_PARAM:
		command = CMD_V7_WRITE;
		break;
	case CMD_READ_CONFIG:
		command = CMD_V7_READ;
		break;
	case CMD_ERASE_ALL:
		command = CMD_V7_ERASE_AP;
		break;
	case CMD_ERASE_UI_FIRMWARE:
	case CMD_ERASE_BL_CONFIG:
	case CMD_ERASE_UI_CONFIG:
	case CMD_ERASE_DISP_CONFIG:
	case CMD_ERASE_FLASH_CONFIG:
	case CMD_ERASE_GUEST_CODE:
	case CMD_ERASE_BOOTLOADER:
	case CMD_ERASE_UTILITY_PARAMETER:
		command = CMD_V7_ERASE;
		break;
	case CMD_ENABLE_FLASH_PROG:
		command = CMD_V7_ENTER_BL;
		break;
	default:
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Invalid command 0x%02x\n",
				__func__, cmd);
		return -EINVAL;
	};

	fwu->command = command;

	switch (cmd) {
	case CMD_ERASE_ALL:
	case CMD_ERASE_UI_FIRMWARE:
	case CMD_ERASE_BL_CONFIG:
	case CMD_ERASE_UI_CONFIG:
	case CMD_ERASE_DISP_CONFIG:
	case CMD_ERASE_FLASH_CONFIG:
	case CMD_ERASE_GUEST_CODE:
	case CMD_ERASE_BOOTLOADER:
	case CMD_ERASE_UTILITY_PARAMETER:
	case CMD_ENABLE_FLASH_PROG:
		retval = fwu_write_f34_v7_command_single_transaction(rmi4_data, cmd);
		if (retval < 0)
			return retval;
		else
			return 0;
	default:
		break;
	};

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.flash_cmd,
			&command,
			sizeof(command));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write flash command\n",
				__func__);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_v7_partition_id(struct synaptics_rmi4_data *rmi4_data, unsigned char cmd)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	int retval;
	unsigned char data_base;
	unsigned char partition;

	data_base = fwu->f34_fd.data_base_addr;

	switch (cmd) {
	case CMD_WRITE_FW:
		partition = CORE_CODE_PARTITION;
		break;
	case CMD_WRITE_CONFIG:
	case CMD_READ_CONFIG:
		if (fwu->config_area == UI_CONFIG_AREA)
			partition = CORE_CONFIG_PARTITION;
		else if (fwu->config_area == DP_CONFIG_AREA)
			partition = DISPLAY_CONFIG_PARTITION;
		else if (fwu->config_area == PM_CONFIG_AREA)
			partition = GUEST_SERIALIZATION_PARTITION;
		else if (fwu->config_area == BL_CONFIG_AREA)
			partition = GLOBAL_PARAMETERS_PARTITION;
		else if (fwu->config_area == FLASH_CONFIG_AREA)
			partition = FLASH_CONFIG_PARTITION;
		else if (fwu->config_area == UPP_AREA)
			partition = UTILITY_PARAMETER_PARTITION;
		break;
	case CMD_WRITE_LOCKDOWN:
		partition = DEVICE_CONFIG_PARTITION;
		break;
	case CMD_WRITE_GUEST_CODE:
		partition = GUEST_CODE_PARTITION;
		break;
	case CMD_WRITE_BOOTLOADER:
		partition = BOOTLOADER_PARTITION;
		break;
	case CMD_WRITE_UTILITY_PARAM:
		partition = UTILITY_PARAMETER_PARTITION;
		break;
	case CMD_ERASE_ALL:
		partition = CORE_CODE_PARTITION;
		break;
	case CMD_ERASE_BL_CONFIG:
		partition = GLOBAL_PARAMETERS_PARTITION;
		break;
	case CMD_ERASE_UI_CONFIG:
		partition = CORE_CONFIG_PARTITION;
		break;
	case CMD_ERASE_DISP_CONFIG:
		partition = DISPLAY_CONFIG_PARTITION;
		break;
	case CMD_ERASE_FLASH_CONFIG:
		partition = FLASH_CONFIG_PARTITION;
		break;
	case CMD_ERASE_GUEST_CODE:
		partition = GUEST_CODE_PARTITION;
		break;
	case CMD_ERASE_BOOTLOADER:
		partition = BOOTLOADER_PARTITION;
		break;
	case CMD_ENABLE_FLASH_PROG:
		partition = BOOTLOADER_PARTITION;
		break;
	default:
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Invalid command 0x%02x\n",
				__func__, cmd);
		return -EINVAL;
	};

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.partition_id,
			&partition,
			sizeof(partition));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write partition ID\n",
				__func__);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_partition_id(struct synaptics_rmi4_data *rmi4_data, unsigned char cmd)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	int retval;

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		retval = fwu_write_f34_v7_partition_id(rmi4_data, cmd);
	else
		retval = 0;

	return retval;
}

static int fwu_read_f34_v7_partition_table(struct synaptics_rmi4_data *rmi4_data, unsigned char *partition_table)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	int retval;
	unsigned char data_base;
	unsigned char length[2];
	unsigned short block_number = 0;

	data_base = fwu->f34_fd.data_base_addr;

	fwu->config_area = FLASH_CONFIG_AREA;

	retval = fwu_write_f34_partition_id(rmi4_data, CMD_READ_CONFIG);
	if (retval < 0)
		return retval;

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.block_number,
			(unsigned char *)&block_number,
			sizeof(block_number));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write block number\n",
				__func__);
		return retval;
	}

	length[0] = (unsigned char)(fwu->flash_config_length & MASK_8BIT);
	length[1] = (unsigned char)(fwu->flash_config_length >> 8);

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.transfer_length,
			length,
			sizeof(length));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write transfer length\n",
				__func__);
		return retval;
	}

	retval = fwu_write_f34_command(rmi4_data, CMD_READ_CONFIG);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write command\n",
				__func__);
		return retval;
	}

	retval = fwu_wait_for_idle(rmi4_data, WRITE_WAIT_MS);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to wait for idle status\n",
				__func__);
		return retval;
	}

	retval = rmi4_data->i2c_read(rmi4_data,
			data_base + fwu->off.payload,
			partition_table,
			fwu->partition_table_bytes);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read block data\n",
				__func__);
		return retval;
	}

	return 0;
}

static void fwu_parse_partition_table(struct synaptics_rmi4_data *rmi4_data,
		const unsigned char *partition_table,
		struct block_count *blkcount, struct physical_address *phyaddr)
{
	unsigned char ii;
	unsigned char index;
	unsigned char offset;
	unsigned short partition_length;
	unsigned short physical_address;
	struct partition_table *ptable;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	for (ii = 0; ii < fwu->partitions; ii++) {
		index = ii * 8 + 2;
		ptable = (struct partition_table *)&partition_table[index];
		partition_length = ptable->partition_length_15_8 << 8 |
				ptable->partition_length_7_0;
		physical_address = ptable->start_physical_address_15_8 << 8 |
				ptable->start_physical_address_7_0;
		input_dbg(true, &rmi4_data->i2c_client->dev,
				"%s: Partition entry %d:\n",
				__func__, ii);
		for (offset = 0; offset < 8; offset++) {
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: 0x%02x\n",
					__func__,
					partition_table[index + offset]);
		}
		switch (ptable->partition_id) {
		case CORE_CODE_PARTITION:
			blkcount->ui_firmware = partition_length;
			phyaddr->ui_firmware = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Core code block count: %d\n",
					__func__, blkcount->ui_firmware);
			blkcount->total_count += partition_length;
			break;
		case CORE_CONFIG_PARTITION:
			blkcount->ui_config = partition_length;
			phyaddr->ui_config = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Core config block count: %d\n",
					__func__, blkcount->ui_config);
			blkcount->total_count += partition_length;
			break;
		case BOOTLOADER_PARTITION:
			blkcount->bl_image = partition_length;
			phyaddr->bl_image = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Bootloader block count: %d\n",
					__func__, blkcount->bl_image);
			blkcount->total_count += partition_length;
			break;
		case UTILITY_PARAMETER_PARTITION:
			blkcount->utility_param = partition_length;
			phyaddr->utility_param = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Utility parameter block count: %d\n",
					__func__, blkcount->utility_param);
			blkcount->total_count += partition_length;
			break;
		case DISPLAY_CONFIG_PARTITION:
			blkcount->dp_config = partition_length;
			phyaddr->dp_config = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Display config block count: %d\n",
					__func__, blkcount->dp_config);
			blkcount->total_count += partition_length;
			break;
		case FLASH_CONFIG_PARTITION:
			blkcount->fl_config = partition_length;
			phyaddr->fl_config = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Flash config block count: %d\n",
					__func__, blkcount->fl_config);
			blkcount->total_count += partition_length;
			break;
		case GUEST_CODE_PARTITION:
			blkcount->guest_code = partition_length;
			phyaddr->guest_code = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Guest code block count: %d\n",
					__func__, blkcount->guest_code);
			blkcount->total_count += partition_length;
			break;
		case GUEST_SERIALIZATION_PARTITION:
			blkcount->pm_config = partition_length;
			phyaddr->pm_config = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Guest serialization block count: %d\n",
					__func__, blkcount->pm_config);
			blkcount->total_count += partition_length;
			break;
		case GLOBAL_PARAMETERS_PARTITION:
			blkcount->bl_config = partition_length;
			phyaddr->bl_config = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Global parameters block count: %d\n",
					__func__, blkcount->bl_config);
			blkcount->total_count += partition_length;
			break;
		case DEVICE_CONFIG_PARTITION:
			blkcount->lockdown = partition_length;
			phyaddr->lockdown = physical_address;
			input_dbg(true, &rmi4_data->i2c_client->dev,
					"%s: Device config block count: %d\n",
					__func__, blkcount->lockdown);
			blkcount->total_count += partition_length;
			break;
		};
	}

	return;
}

static int fwu_read_f34_v7_blocks(struct synaptics_rmi4_data *rmi4_data, unsigned short block_cnt,
		unsigned char command)
{
	int retval;
	unsigned char data_base;
	unsigned char length[2];
	unsigned short transfer;
	unsigned short remaining = block_cnt;
	unsigned short block_number = 0;
	unsigned short index = 0;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	data_base = fwu->f34_fd.data_base_addr;

	retval = fwu_write_f34_partition_id(rmi4_data, command);
	if (retval < 0)
		return retval;

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.block_number,
			(unsigned char *)&block_number,
			sizeof(block_number));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write block number\n",
				__func__);
		return retval;
	}

	do {
		if (remaining / fwu->payload_length)
			transfer = fwu->payload_length;
		else
			transfer = remaining;

		length[0] = (unsigned char)(transfer & MASK_8BIT);
		length[1] = (unsigned char)(transfer >> 8);

		retval = rmi4_data->i2c_write(rmi4_data,
				data_base + fwu->off.transfer_length,
				length,
				sizeof(length));
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to write transfer length (remaining = %d)\n",
					__func__, remaining);
			return retval;
		}

		retval = fwu_write_f34_command(rmi4_data, command);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to write command (remaining = %d)\n",
					__func__, remaining);
			return retval;
		}

		retval = fwu_wait_for_idle(rmi4_data, WRITE_WAIT_MS);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to wait for idle status (remaining = %d)\n",
					__func__, remaining);
			return retval;
		}

		retval = rmi4_data->i2c_read(rmi4_data,
				data_base + fwu->off.payload,
				&fwu->read_config_buf[index],
				transfer * fwu->block_size);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to read block data (remaining = %d)\n",
					__func__, remaining);
			return retval;
		}

		index += (transfer * fwu->block_size);
		remaining -= transfer;
	} while (remaining);

	return 0;
}

static int fwu_read_f34_v5v6_blocks(struct synaptics_rmi4_data *rmi4_data, unsigned short block_cnt,
		unsigned char command)
{
	int retval;
	unsigned char data_base;
	unsigned char block_number[] = {0, 0};
	unsigned short blk;
	unsigned short index = 0;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	data_base = fwu->f34_fd.data_base_addr;

	block_number[1] |= (fwu->config_area << 5);

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.block_number,
			block_number,
			sizeof(block_number));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write block number\n",
				__func__);
		return retval;
	}

	for (blk = 0; blk < block_cnt; blk++) {
		retval = fwu_write_f34_command(rmi4_data, command);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to write read config command\n",
					__func__);
			return retval;
		}

		retval = fwu_wait_for_idle(rmi4_data, WRITE_WAIT_MS);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to wait for idle status\n",
					__func__);
			return retval;
		}

		retval = rmi4_data->i2c_read(rmi4_data,
				data_base + fwu->off.payload,
				&fwu->read_config_buf[index],
				fwu->block_size);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to read block data (block %d)\n",
					__func__, blk);
			return retval;
		}

		index += fwu->block_size;
	}

	return 0;
}

static int fwu_read_f34_blocks(struct synaptics_rmi4_data *rmi4_data, unsigned short block_cnt, unsigned char cmd)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	int retval;

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		retval = fwu_read_f34_v7_blocks(rmi4_data, block_cnt, cmd);
	else
		retval = fwu_read_f34_v5v6_blocks(rmi4_data, block_cnt, cmd);

	return retval;
}

static int fwu_read_f34_v7_queries(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	int retval;
	unsigned char ii;
	unsigned char query_base;
	unsigned char index;
	unsigned char offset;
	unsigned char *ptable;
	struct f34_v7_query_0 query_0;
	struct f34_v7_query_1_7 query_1_7;

	query_base = fwu->f34_fd.query_base_addr;
	
	retval = rmi4_data->i2c_read(rmi4_data,
			query_base,
			query_0.data,
			sizeof(query_0.data));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read query 0\n",
				__func__);
		return retval;
	}

	offset = query_0.subpacket_1_size + 1;

	retval = rmi4_data->i2c_read(rmi4_data,
			query_base + offset,
			query_1_7.data,
			sizeof(query_1_7.data));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read queries 1 to 7\n",
				__func__);
		return retval;
	}

	fwu->bootloader_id[0] = query_1_7.bl_minor_revision;
	fwu->bootloader_id[1] = query_1_7.bl_major_revision;

	if (fwu->bootloader_id[1] == BL_V8)
		fwu->bl_version = BL_V8;

	fwu->block_size = query_1_7.block_size_15_8 << 8 |
			query_1_7.block_size_7_0;

	fwu->flash_config_length = query_1_7.flash_config_length_15_8 << 8 |
			query_1_7.flash_config_length_7_0;

	fwu->payload_length = query_1_7.payload_length_15_8 << 8 |
			query_1_7.payload_length_7_0;

	fwu->off.flash_status = V7_FLASH_STATUS_OFFSET;
	fwu->off.partition_id = V7_PARTITION_ID_OFFSET;
	fwu->off.block_number = V7_BLOCK_NUMBER_OFFSET;
	fwu->off.transfer_length = V7_TRANSFER_LENGTH_OFFSET;
	fwu->off.flash_cmd = V7_COMMAND_OFFSET;
	fwu->off.payload = V7_PAYLOAD_OFFSET;

	index = sizeof(query_1_7.data) - V7_PARTITION_SUPPORT_BYTES;

	fwu->partitions = 0;
	for (offset = 0; offset < V7_PARTITION_SUPPORT_BYTES; offset++) {
		for (ii = 0; ii < 8; ii++) {
			if (query_1_7.data[index + offset] & (1 << ii))
				fwu->partitions++;
		}

		input_dbg(true, &rmi4_data->i2c_client->dev,
				"%s: Supported partitions: 0x%02x\n",
				__func__, query_1_7.data[index + offset]);
	}

	fwu->partition_table_bytes = fwu->partitions * 8 + 2;

	ptable = kzalloc(fwu->partition_table_bytes, GFP_KERNEL);
	if (!ptable) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for partition table\n",
				__func__);
		return -ENOMEM;
	}

	retval = fwu_read_f34_v7_partition_table(rmi4_data, ptable);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read partition table\n",
				__func__);
		kfree(ptable);
		return retval;
	}

	fwu_parse_partition_table(rmi4_data, ptable, &fwu->blkcount, &fwu->phyaddr);

	if (fwu->blkcount.dp_config)
		fwu->flash_properties.has_disp_config = 1;
	else
		fwu->flash_properties.has_disp_config = 0;

	if (fwu->blkcount.pm_config)
		fwu->flash_properties.has_pm_config = 1;
	else
		fwu->flash_properties.has_pm_config = 0;

	if (fwu->blkcount.bl_config)
		fwu->flash_properties.has_bl_config = 1;
	else
		fwu->flash_properties.has_bl_config = 0;

	if (fwu->blkcount.guest_code)
		fwu->has_guest_code = 1;
	else
		fwu->has_guest_code = 0;

	if (fwu->blkcount.utility_param)
		fwu->has_utility_param = 1;
	else
		fwu->has_utility_param = 0;

	kfree(ptable);

	return 0;
}

static int fwu_read_f34_v5v6_queries(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char count;
	unsigned char query_base;
	unsigned char buf[10];
	struct f34_v5v6_flash_properties_2 properties_2;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	query_base = fwu->f34_fd.query_base_addr;

	retval = rmi4_data->i2c_read(rmi4_data,
			query_base + V5V6_BOOTLOADER_ID_OFFSET,
			fwu->bootloader_id,
			sizeof(fwu->bootloader_id));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read bootloader ID\n",
				__func__);
		return retval;
	}

	if (fwu->bl_version == BL_V5) {
		fwu->off.properties = V5_PROPERTIES_OFFSET;
		fwu->off.block_size = V5_BLOCK_SIZE_OFFSET;
		fwu->off.block_count = V5_BLOCK_COUNT_OFFSET;
		fwu->off.block_number = V5_BLOCK_NUMBER_OFFSET;
		fwu->off.payload = V5_BLOCK_DATA_OFFSET;
	} else if (fwu->bl_version == BL_V6) {
		fwu->off.properties = V6_PROPERTIES_OFFSET;
		fwu->off.properties_2 = V6_PROPERTIES_2_OFFSET;
		fwu->off.block_size = V6_BLOCK_SIZE_OFFSET;
		fwu->off.block_count = V6_BLOCK_COUNT_OFFSET;
		fwu->off.gc_block_count = V6_GUEST_CODE_BLOCK_COUNT_OFFSET;
		fwu->off.block_number = V6_BLOCK_NUMBER_OFFSET;
		fwu->off.payload = V6_BLOCK_DATA_OFFSET;
	}

	retval = rmi4_data->i2c_read(rmi4_data,
			query_base + fwu->off.block_size,
			buf,
			2);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read block size info\n",
				__func__);
		return retval;
	}

	batohs(&fwu->block_size, &(buf[0]));

	if (fwu->bl_version == BL_V5) {
		fwu->off.flash_cmd = fwu->off.payload + fwu->block_size;
		fwu->off.flash_status = fwu->off.flash_cmd;
	} else if (fwu->bl_version == BL_V6) {
		fwu->off.flash_cmd = V6_FLASH_COMMAND_OFFSET;
		fwu->off.flash_status = V6_FLASH_STATUS_OFFSET;
	}

	retval = rmi4_data->i2c_read(rmi4_data,
			query_base + fwu->off.properties,
			fwu->flash_properties.data,
			sizeof(fwu->flash_properties.data));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read flash properties\n",
				__func__);
		return retval;
	}

	count = 4;

	if (fwu->flash_properties.has_pm_config)
		count += 2;

	if (fwu->flash_properties.has_bl_config)
		count += 2;

	if (fwu->flash_properties.has_disp_config)
		count += 2;

	retval = rmi4_data->i2c_read(rmi4_data,
			query_base + fwu->off.block_count,
			buf,
			count);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read block count info\n",
				__func__);
		return retval;
	}

	batohs(&fwu->blkcount.ui_firmware, &(buf[0]));
	batohs(&fwu->blkcount.ui_config, &(buf[2]));

	count = 4;

	if (fwu->flash_properties.has_pm_config) {
		batohs(&fwu->blkcount.pm_config, &(buf[count]));
		count += 2;
	}

	if (fwu->flash_properties.has_bl_config) {
		batohs(&fwu->blkcount.bl_config, &(buf[count]));
		count += 2;
	}

	if (fwu->flash_properties.has_disp_config)
		batohs(&fwu->blkcount.dp_config, &(buf[count]));

	fwu->has_guest_code = false;

	if (fwu->flash_properties.has_query4) {
		retval = rmi4_data->i2c_read(rmi4_data,
				query_base + fwu->off.properties_2,
				properties_2.data,
				sizeof(properties_2.data));
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to read flash properties 2\n",
					__func__);
			return retval;
		}

		if (properties_2.has_guest_code) {
			retval = rmi4_data->i2c_read(rmi4_data,
					query_base + fwu->off.gc_block_count,
					buf,
					2);
			if (retval < 0) {
				input_err(true, &rmi4_data->i2c_client->dev,
						"%s: Failed to read guest code block count\n",
						__func__);
				return retval;
			}

			batohs(&fwu->blkcount.guest_code, &(buf[0]));
			fwu->has_guest_code = true;
		}
	}

	fwu->has_utility_param = false;

	return 0;
}

static int fwu_read_f34_queries(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	memset(&fwu->blkcount, 0x00, sizeof(fwu->blkcount));
	memset(&fwu->phyaddr, 0x00, sizeof(fwu->phyaddr));

	if (fwu->bl_version == BL_V7)
		retval = fwu_read_f34_v7_queries(rmi4_data);
	else
		retval = fwu_read_f34_v5v6_queries(rmi4_data);

	return retval;
}

static int fwu_read_flash_status(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char status;
	unsigned char command;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	
	retval = rmi4_data->i2c_read(rmi4_data,
			fwu->f34_fd.data_base_addr + fwu->off.flash_status,
			&status,
			sizeof(status));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read flash status\n",
				__func__);
		return retval;
	}

	fwu->in_bl_mode = status >> 7;

	if (fwu->bl_version == BL_V5)
		fwu->flash_status = (status >> 4) & MASK_3BIT;
	else if (fwu->bl_version == BL_V6)
		fwu->flash_status = status & MASK_3BIT;
	else if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		fwu->flash_status = status & MASK_5BIT;

	if (fwu->write_bootloader)
		fwu->flash_status = 0x00;

	if (fwu->flash_status != 0x00) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Flash status = %d, command = 0x%02x\n",
				__func__, fwu->flash_status, fwu->command);
	}

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8) {
		if (fwu->flash_status == 0x08)
			fwu->flash_status = 0x00;
	}

	retval = rmi4_data->i2c_read(rmi4_data,
			fwu->f34_fd.data_base_addr + fwu->off.flash_cmd,
			&command,
			sizeof(command));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read flash command\n",
				__func__);
		return retval;
	}

	if (fwu->bl_version == BL_V5)
		fwu->command = command & MASK_4BIT;
	else if (fwu->bl_version == BL_V6)
		fwu->command = command & MASK_6BIT;
	else if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		fwu->command = command;

	if (fwu->write_bootloader)
		fwu->command = 0x00;

	return 0;
}

static int fwu_allocate_read_config_buf(struct synaptics_rmi4_data *rmi4_data, unsigned int count)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (count > fwu->read_config_buf_size) {
		kfree(fwu->read_config_buf);
		fwu->read_config_buf = kzalloc(count, GFP_KERNEL);
		if (!fwu->read_config_buf) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to alloc mem for fwu->read_config_buf\n",
					__func__);
				fwu->read_config_buf_size = 0;
			return -ENOMEM;
		}
		fwu->read_config_buf_size = count;
	}

	return 0;
}

static int fwu_write_f34_v5v6_command(struct synaptics_rmi4_data *rmi4_data, unsigned char cmd)
{
	int retval;
	unsigned char data_base;
	unsigned char command;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	data_base = fwu->f34_fd.data_base_addr;

	switch (cmd) {
	case CMD_IDLE:
		command = CMD_V5V6_IDLE;
		break;
	case CMD_WRITE_FW:
		command = CMD_V5V6_WRITE_FW;
		break;
	case CMD_WRITE_CONFIG:
		command = CMD_V5V6_WRITE_CONFIG;
		break;
	case CMD_WRITE_LOCKDOWN:
		command = CMD_V5V6_WRITE_LOCKDOWN;
		break;
	case CMD_WRITE_GUEST_CODE:
		command = CMD_V5V6_WRITE_GUEST_CODE;
		break;
	case CMD_READ_CONFIG:
		command = CMD_V5V6_READ_CONFIG;
		break;
	case CMD_ERASE_ALL:
		command = CMD_V5V6_ERASE_ALL;
		break;
	case CMD_ERASE_UI_CONFIG:
		command = CMD_V5V6_ERASE_UI_CONFIG;
		break;
	case CMD_ERASE_DISP_CONFIG:
		command = CMD_V5V6_ERASE_DISP_CONFIG;
		break;
	case CMD_ERASE_GUEST_CODE:
		command = CMD_V5V6_ERASE_GUEST_CODE;
		break;
	case CMD_ENABLE_FLASH_PROG:
		command = CMD_V5V6_ENABLE_FLASH_PROG;
		break;
	default:
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Invalid command 0x%02x\n",
				__func__, cmd);
		return -EINVAL;
	}

	switch (cmd) {
	case CMD_ERASE_ALL:
	case CMD_ERASE_UI_CONFIG:
	case CMD_ERASE_DISP_CONFIG:
	case CMD_ERASE_GUEST_CODE:
	case CMD_ENABLE_FLASH_PROG:
		retval = rmi4_data->i2c_write(rmi4_data,
				data_base + fwu->off.payload,
				fwu->bootloader_id,
				sizeof(fwu->bootloader_id));
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to write bootloader ID\n",
					__func__);
			return retval;
		}
		break;
	default:
		break;
	};

	fwu->command = command;

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.flash_cmd,
			&command,
			sizeof(command));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write command 0x%02x\n",
				__func__, command);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_command(struct synaptics_rmi4_data *rmi4_data, unsigned char cmd)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		retval = fwu_write_f34_v7_command(rmi4_data, cmd);
	else
		retval = fwu_write_f34_v5v6_command(rmi4_data, cmd);

	return retval;
}

static int fwu_wait_for_idle(struct synaptics_rmi4_data *rmi4_data, int timeout_ms)
{
	int count = 0;
	int timeout_count = ((timeout_ms * 1000) / MAX_SLEEP_TIME_US) + 1;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	input_dbg(true, &rmi4_data->i2c_client->dev,
			"%s: fwu->polling_mode = %d, timeout_count = %d\n",
			__func__, fwu->polling_mode, timeout_count );

	do {
		usleep_range(MIN_SLEEP_TIME_US, MAX_SLEEP_TIME_US);

		count++;
		if (fwu->polling_mode || (count == timeout_count))
			fwu_read_flash_status(rmi4_data);

		if ((fwu->command == CMD_IDLE) && (fwu->flash_status == 0x00))
			return 0;
	} while (count < timeout_count);

	input_err(true, &rmi4_data->i2c_client->dev,
			"%s: Timed out waiting for idle status\n",
			__func__);

	return -ETIMEDOUT;
}

#ifdef FW_UPDATE_GO_NOGO
static enum flash_area fwu_go_nogo(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	enum flash_area flash_area = NONE;
	unsigned char index = 0;
	unsigned char config_id[4];
	unsigned int device_config_id;
	unsigned int image_config_id;
	unsigned int device_fw_id;
	unsigned long image_fw_id;
	char *strptr;
	char *firmware_id;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (fwu->force_update) {
		flash_area = UI_FIRMWARE;
		goto exit;
	}

	/* Update both UI and config if device is in bootloader mode */
	if (fwu->in_flash_prog_mode) {
		flash_area = UI_FIRMWARE;
		goto exit;
	}

	/* Get device firmware ID */
	device_fw_id = rmi4_data->rmi4_mod_info.build_id[0] +
			rmi4_data->rmi4_mod_info.build_id[1] * 0x100 +
			rmi4_data->rmi4_mod_info.build_id[2] * 0x10000;
	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Device firmware ID = %d\n",
			__func__, device_fw_id);

	/* Get image firmware ID */
	if (fwu->img.firmwareId != NULL) {
		image_fw_id = extract_uint_le(fwu->img.firmwareId);
	} else {
		strptr = strstr(fwu->img_data.image_name, "PR");
		if (!strptr) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: No valid PR number (PRxxxxxxx) "
					"found in image file name (%s)\n",
					__func__, fwu->img_data.image_name);
			flash_area = NONE;
			goto exit;
		}

		strptr += 2;
		firmware_id = kzalloc(MAX_FIRMWARE_ID_LEN, GFP_KERNEL);
		while (strptr[index] >= '0' && strptr[index] <= '9') {
			firmware_id[index] = strptr[index];
			index++;
		}

		retval = kstrtoul(firmware_id, 10, &image_fw_id);
		kfree(firmware_id);
		if (retval) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to obtain image firmware ID\n",
					__func__);
			flash_area = NONE;
			goto exit;
		}
	}
	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Image firmware ID = %d\n",
			__func__, (unsigned int)image_fw_id);

	if (image_fw_id > device_fw_id) {
		flash_area = UI_FIRMWARE;
		goto exit;
	} else if (image_fw_id < device_fw_id) {
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: Image firmware ID older than device firmware ID\n",
				__func__);
		flash_area = NONE;
		goto exit;
	}

	/* Get device config ID */
	retval = rmi4_data->i2c_read(rmi4_data,
				fwu->f34_fd.ctrl_base_addr,
				config_id,
				sizeof(config_id));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read device config ID\n",
				__func__);
		flash_area = NONE;
		goto exit;
	}
	device_config_id = extract_uint_be(config_id);
	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Device config ID = 0x%02x 0x%02x 0x%02x 0x%02x\n",
			__func__,
			config_id[0],
			config_id[1],
			config_id[2],
			config_id[3]);

	/* Get image config ID */
	image_config_id = extract_uint_be(fwu->img.configId);
	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Image config ID = 0x%02x 0x%02x 0x%02x 0x%02x\n",
			__func__,
			fwu->img.configId[0],
			fwu->img.configId[1],
			fwu->img.configId[2],
			fwu->img.configId[3]);

	if (image_config_id > device_config_id) {
		flash_area = CONFIG_AREA;
		goto exit;
	}

exit:
	if (flash_area == NONE) {
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: No need to do reflash\n",
				__func__);
	} else {
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: Updating %s\n",
				__func__,
				flash_area == UI_FIRMWARE ?
				"UI firmware" :
				"config only");
	}
	return flash_area;
}
#endif

static int fwu_recovery_check_status(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char data_base;
	unsigned char status;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	data_base = fwu->f35_fd.data_base_addr;

	retval = rmi4_data->i2c_read(rmi4_data,
			data_base + F35_ERROR_CODE_OFFSET,
			&status, 1);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read status\n",
				__func__);
		return retval;
	}

	status = status & MASK_5BIT;

	if (status != 0x00) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Recovery mode status = %d\n",
				__func__, status);
		return -EINVAL;
	}

	return 0;
}

static int fwu_recovery_erase_completion(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char data_base;
	unsigned char command;
	unsigned char status;
	unsigned int timeout = F35_ERASE_ALL_WAIT_MS / 20;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	data_base = fwu->f35_fd.data_base_addr;

	do {
		command = 0x01;
		retval = rmi4_data->i2c_write(rmi4_data,
				fwu->f35_fd.cmd_base_addr,
				&command,
				sizeof(command));
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to issue command\n",
					__func__);
			return retval;
		}

		do {
			retval = rmi4_data->i2c_read(rmi4_data,
					fwu->f35_fd.cmd_base_addr,
					&command,
					sizeof(command));
			if (retval < 0) {
				input_err(true, &rmi4_data->i2c_client->dev,
						"%s: Failed to read command status\n",
						__func__);
				return retval;
			}

			if ((command & 0x01) == 0x00)
				break;

			msleep(20);
			timeout--;
		} while (timeout > 0);

		if (timeout == 0)
			goto exit;

		retval = rmi4_data->i2c_read(rmi4_data,
				data_base + F35_FLASH_STATUS_OFFSET,
				&status,
				sizeof(status));
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to read flash status\n",
					__func__);
			return retval;
		}

		if ((status & 0x01) == 0x00)
			break;

		msleep(20);
		timeout--;
	} while (timeout > 0);

exit:
	if (timeout == 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Timed out waiting for flash erase completion\n",
				__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int fwu_recovery_erase_all(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ctrl_base;
	unsigned char command = CMD_F35_ERASE_ALL;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	ctrl_base = fwu->f35_fd.ctrl_base_addr;

	retval = rmi4_data->i2c_write(rmi4_data,
			ctrl_base + F35_CHUNK_COMMAND_OFFSET,
			&command,
			sizeof(command));
	if (retval < 0) {
	input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to issue erase all command\n",
				__func__);
		return retval;
	}

	if (fwu->f35_fd.cmd_base_addr) {
		retval = fwu_recovery_erase_completion(rmi4_data);
		if (retval < 0)
			return retval;
	} else {
		msleep(F35_ERASE_ALL_WAIT_MS);
	}

	retval = fwu_recovery_check_status(rmi4_data);
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_recovery_write_chunk(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ctrl_base;
	unsigned char chunk_number[] = {0, 0};
	unsigned char chunk_spare;
	unsigned char chunk_size;
	unsigned char buf[F35_CHUNK_SIZE + 1];
	unsigned short chunk;
	unsigned short chunk_total;
	unsigned short bytes_written = 0;
	unsigned char *chunk_ptr = (unsigned char *)rmi4_data->fwu->image;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	ctrl_base = fwu->f35_fd.ctrl_base_addr;

	retval = rmi4_data->i2c_write(rmi4_data,
			ctrl_base + F35_CHUNK_NUM_LSB_OFFSET,
			chunk_number,
			sizeof(chunk_number));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write chunk number\n",
				__func__);
		return retval;
	}

	buf[sizeof(buf) - 1] = CMD_F35_WRITE_CHUNK;

	chunk_total = fwu->img.image_size / F35_CHUNK_SIZE;
	chunk_spare = fwu->img.image_size % F35_CHUNK_SIZE;
	if (chunk_spare)
		chunk_total++;

	input_err(true, &rmi4_data->i2c_client->dev,
		"%s: image_size %d\n",
		__func__, fwu->img.image_size);

	for (chunk = 0; chunk < chunk_total; chunk++) {
		if (chunk_spare && chunk == chunk_total - 1)
			chunk_size = chunk_spare;
		else
			chunk_size = F35_CHUNK_SIZE;

		memset(buf, 0x00, F35_CHUNK_SIZE);
		secure_memcpy(buf, sizeof(buf), chunk_ptr,
					fwu->img.image_size - bytes_written,
					chunk_size);

		retval = rmi4_data->i2c_write(rmi4_data,
				ctrl_base + F35_CHUNK_DATA_OFFSET,
				buf,
				sizeof(buf));
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to write chunk data (chunk %d)\n",
					__func__, chunk);
			return retval;
		}
		chunk_ptr += chunk_size;
		bytes_written += chunk_size;
	}

	retval = fwu_recovery_check_status(rmi4_data);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write chunk data\n",
				__func__);
		return retval;
	}

	return 0;
}

static int fwu_recovery_reset(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ctrl_base;
	unsigned char command = CMD_F35_RESET;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	input_err(true, &rmi4_data->i2c_client->dev,
			"%s:\n", __func__);

	ctrl_base = fwu->f35_fd.ctrl_base_addr;

	retval = rmi4_data->i2c_write(rmi4_data,
			ctrl_base + F35_CHUNK_COMMAND_OFFSET,
			&command,
			sizeof(command));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command\n",
				__func__);
		return retval;
	}

	msleep(F35_RESET_WAIT_MS);

	return 0;
}

static int fwu_start_recovery(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	const struct firmware *fw_entry = NULL;

	if (rmi4_data->sensor_sleep) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Sensor sleeping\n",
				__func__);
		return -ENODEV;
	}

	rmi4_data->stay_awake = true;

	mutex_lock(&rmi4_data->rmi4_reflash_mutex);

	input_err(true, &rmi4_data->i2c_client->dev,
			"%s: Start of recovery process\n", __func__);

	disable_irq(rmi4_data->i2c_client->irq);

	retval = fwu_recovery_erase_all(rmi4_data);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to do erase all in recovery mode\n",
				__func__);
		goto exit;
	}

	input_err(true, &rmi4_data->i2c_client->dev,
			"%s: External flash erased\n", __func__);

	retval = fwu_recovery_write_chunk(rmi4_data);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write chunk data in recovery mode\n",
				__func__);
		goto exit;
	}

	input_err(true, &rmi4_data->i2c_client->dev,
			"%s: Chunk data programmed\n", __func__);

	retval = fwu_recovery_reset(rmi4_data);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to reset device in recovery mode\n",
				__func__);
		goto exit;
	}

	input_err(true, &rmi4_data->i2c_client->dev,
			"%s: Recovery mode reset issued\n", __func__);

	rmi4_data->reset_device(rmi4_data);
	synaptics_rmi4_fwu_reset(rmi4_data);

	retval = 0;

exit:	
	if (fw_entry)
		release_firmware(fw_entry);

	input_err(true, &rmi4_data->i2c_client->dev,
			"%s: End of recovery process\n", __func__);

	mutex_unlock(&rmi4_data->rmi4_reflash_mutex);

	rmi4_data->stay_awake = false;

	return retval;
}

static int fwu_scan_pdt(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	unsigned char intr_count = 0;
	unsigned char intr_off;
	unsigned char intr_src;
	unsigned short addr;
	bool f01found = false;
	bool f34found = false;
	bool f35found = false;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	fwu->in_ub_mode = false;

	for (addr = PDT_START; addr > PDT_END; addr -= PDT_ENTRY_SIZE) {
		retval = rmi4_data->i2c_read(rmi4_data,
				addr,
				(unsigned char *)&rmi_fd,
				sizeof(rmi_fd));
		if (retval < 0)
			return retval;

		if (rmi_fd.fn_number) {
			input_dbg(false, &rmi4_data->i2c_client->dev,
					"%s: Found F%02x\n",
					__func__, rmi_fd.fn_number);
			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F01:
				f01found = true;
				fwu->f01_fd.query_base_addr =
					rmi_fd.query_base_addr;
				fwu->f01_fd.ctrl_base_addr =
					rmi_fd.ctrl_base_addr;
				fwu->f01_fd.data_base_addr =
					rmi_fd.data_base_addr;
				fwu->f01_fd.cmd_base_addr =
					rmi_fd.cmd_base_addr;
				rmi4_data->f01_query_base_addr =
					rmi_fd.query_base_addr;
				rmi4_data->f01_ctrl_base_addr =
					rmi_fd.ctrl_base_addr;
				rmi4_data->f01_data_base_addr =
					rmi_fd.data_base_addr;
				rmi4_data->f01_cmd_base_addr =
					rmi_fd.cmd_base_addr;
				break;
			case SYNAPTICS_RMI4_F34:
				f34found = true;
				fwu->f34_fd.query_base_addr =
					rmi_fd.query_base_addr;
				fwu->f34_fd.ctrl_base_addr =
					rmi_fd.ctrl_base_addr;
				fwu->f34_fd.data_base_addr =
					rmi_fd.data_base_addr;

				switch (rmi_fd.fn_version) {
				case F34_V0:
					fwu->bl_version = BL_V5;
					break;
				case F34_V1:
					fwu->bl_version = BL_V6;
					break;
				case F34_V2:
					fwu->bl_version = BL_V7;
					break;
				default:
					input_err(true, &rmi4_data->i2c_client->dev,
							"%s: Unrecognized F34 version\n",
							__func__);
					return -EINVAL;
				}

				fwu->intr_mask = 0;
				intr_src = rmi_fd.intr_src_count;
				intr_off = intr_count % 8;
				for (ii = intr_off;	ii < ((intr_src & MASK_3BIT) + intr_off); ii++)
					fwu->intr_mask |= 1 << ii;
				break;
			case SYNAPTICS_RMI4_F35:
				f35found = true;
				fwu->f35_fd.query_base_addr =
						rmi_fd.query_base_addr;
				fwu->f35_fd.ctrl_base_addr =
						rmi_fd.ctrl_base_addr;
				fwu->f35_fd.data_base_addr =
						rmi_fd.data_base_addr;
				fwu->f35_fd.cmd_base_addr =
						rmi_fd.cmd_base_addr;
				break;
			}
		} else {
			break;
		}

		intr_count += (rmi_fd.intr_src_count & MASK_3BIT);
	}

	if (!f01found || !f34found) {
		if (!f35found) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to find F35\n",
					__func__);
			return -EINVAL;
		} else {
			fwu->in_ub_mode = true;
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: In microbootloader mode\n",
					__func__);
			fwu_recovery_check_status(rmi4_data);
			return 0;
		}
	}

	return 0;
}

static int fwu_write_bootloader_id(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	retval = rmi4_data->i2c_write(rmi4_data,
			fwu->f34_fd.data_base_addr + fwu->blk_data_off,
			fwu->bootloader_id,
			sizeof(fwu->bootloader_id));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write bootloader ID\n",
				__func__);
		return retval;
	}

	return 0;
}

static int fwu_enter_flash_prog(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct f01_device_control f01_device_control;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	retval = fwu_read_flash_status(rmi4_data);
	if (retval < 0)
		return retval;

	if (fwu->in_bl_mode) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: BL mode  entered\n",
				__func__);
		return 0;
	}

	retval = rmi4_data->irq_enable(rmi4_data, false, true);
	if (retval < 0)
		return retval;

	msleep(20);

	retval = fwu_write_f34_command(rmi4_data, CMD_ENABLE_FLASH_PROG);
	if (retval < 0)
		return retval;

	retval = fwu_wait_for_idle(rmi4_data, ENABLE_WAIT_MS);
	if (retval < 0)
		return retval;

	if (!fwu->in_bl_mode) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: BL mode not entered\n",
				__func__);
		return -EINVAL;
	}

	retval = fwu_scan_pdt(rmi4_data);
	if (retval < 0)
		return retval;

	retval = fwu_read_f34_queries(rmi4_data);
	if (retval < 0)
		return retval;

	retval = rmi4_data->i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			f01_device_control.data,
			sizeof(f01_device_control.data));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read F01 device control\n",
				__func__);
		return retval;
	}

	f01_device_control.nosleep = true;
	f01_device_control.sleep_mode = SLEEP_MODE_NORMAL;

	retval = rmi4_data->i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			f01_device_control.data,
			sizeof(f01_device_control.data));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write F01 device control\n",
				__func__);
		return retval;
	}

	msleep(20);

	return retval;
}

static int fwu_check_ui_firmware_size(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned short block_count;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	block_count = fwu->img_data.ui_firmware.size / fwu->block_size;
		
	if (block_count != fwu->blkcount.ui_firmware) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: UI firmware size mismatch\n",
				__func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_ui_configuration_size(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned short block_count;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	block_count = fwu->img_data.ui_config.size / fwu->block_size;

	if (block_count != fwu->blkcount.ui_config) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: UI configuration size mismatch\n",
				__func__);
		return -EINVAL;
	}
	return 0;
}
static int fwu_check_dp_configuration_size(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned short block_count;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	block_count = fwu->img_data.dp_config.size / fwu->block_size;

	if (block_count != fwu->blkcount.dp_config) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Display configuration size mismatch\n",
				__func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_pm_configuration_size(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned short block_count;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	block_count = fwu->img_data.pm_config.size / fwu->block_size;

	if (block_count != fwu->blkcount.pm_config) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Permanent configuration size mismatch\n",
				__func__);
		return -EINVAL;
	}

	return 0;
}
static int fwu_check_bl_configuration_size(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned short block_count;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	block_count = fwu->img_data.bl_config.size / fwu->block_size;
	if (block_count != fwu->blkcount.bl_config) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Guest code size mismatch\n",
				__func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_guest_code_size(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned short block_count;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	block_count = fwu->img_data.guest_code.size / fwu->block_size;
	if (block_count != fwu->blkcount.guest_code) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Guest code size mismatch\n",
				__func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_erase_bootloader(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	retval = fwu_write_f34_command(rmi4_data, CMD_ERASE_BOOTLOADER);
	if (retval < 0)
		return retval;

	input_dbg(true, &rmi4_data->i2c_client->dev,
			"%s: Erase command written\n",
			__func__);

	retval = fwu_wait_for_idle(rmi4_data, ERASE_WAIT_MS);
	if (retval < 0)
		return retval;

	input_dbg(true, &rmi4_data->i2c_client->dev,
			"%s: Idle status detected\n",
			__func__);

	return 0;
}

static int fwu_erase_configuration(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	switch (fwu->config_area) {
	case UI_CONFIG_AREA:
		retval = fwu_write_f34_command(rmi4_data, CMD_ERASE_UI_CONFIG);
		if (retval < 0)
			return retval;
		break;
	case DP_CONFIG_AREA:
		retval = fwu_write_f34_command(rmi4_data,CMD_ERASE_DISP_CONFIG);
		if (retval < 0)
			return retval;
		break;
	case BL_CONFIG_AREA:
		retval = fwu_write_f34_command(rmi4_data,CMD_ERASE_BL_CONFIG);
		if (retval < 0)
			return retval;
		break;
	case FLASH_CONFIG_AREA:
		retval = fwu_write_f34_command(rmi4_data,CMD_ERASE_FLASH_CONFIG);
		if (retval < 0)
			return retval;
		break;
	case UPP_AREA:
		retval = fwu_write_f34_command(rmi4_data,CMD_ERASE_UTILITY_PARAMETER);
		if (retval < 0)
			return retval;
	default:
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: Invalid config area\n",
				__func__);
		return -EINVAL;
	}

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Erase command written\n",
			__func__);

	retval = fwu_wait_for_idle(rmi4_data, ENABLE_WAIT_MS);
	if (retval < 0)
		return retval;

	input_dbg(false, &rmi4_data->i2c_client->dev,
			"%s: Idle status detected\n",
			__func__);

	return retval;
}
static int fwu_erase_guest_code(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	retval = fwu_write_f34_command(rmi4_data, CMD_ERASE_GUEST_CODE);
	if (retval < 0)
		return retval;

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Erase command written\n",
			__func__);

	retval = fwu_wait_for_idle(rmi4_data, ENABLE_WAIT_MS);
	if (retval < 0)
		return retval;

	input_dbg(false, &rmi4_data->i2c_client->dev,
			"%s: Idle status detected\n",
			__func__);

	return 0;
}

static int fwu_erase_all(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (fwu->bl_version == BL_V7) {
		retval = fwu_write_f34_command(rmi4_data, CMD_ERASE_UI_FIRMWARE);
		if (retval < 0)
			return retval;

		input_dbg(false, &rmi4_data->i2c_client->dev,
				"%s: Erase command written\n",
				__func__);

		retval = fwu_wait_for_idle(rmi4_data, ERASE_WAIT_MS);
		if (retval < 0)
			return retval;

		input_dbg(false, &rmi4_data->i2c_client->dev,
				"%s: Idle status detected\n",
				__func__);

		fwu->config_area = UI_CONFIG_AREA;
		retval = fwu_erase_configuration(rmi4_data);
		if (retval < 0)
			return retval;
	} else {
		retval = fwu_write_f34_command(rmi4_data, CMD_ERASE_ALL);
		if (retval < 0)
			return retval;

		input_dbg(false, &rmi4_data->i2c_client->dev,
				"%s: Erase all command written\n",
				__func__);

		retval = fwu_wait_for_idle(rmi4_data, ERASE_WAIT_MS);
		if (!(fwu->bl_version == BL_V8 &&
				fwu->flash_status == BAD_PARTITION_TABLE)) {
			if (retval < 0)
				return retval;
		}

		input_dbg(false, &rmi4_data->i2c_client->dev,
				"%s: Idle status detected\n",
				__func__);

		if (fwu->bl_version == BL_V8)
			return 0;
	}

	if (fwu->flash_properties.has_disp_config) {
		input_info(false, &rmi4_data->i2c_client->dev,
				"%s: has_disp_config\n",
				__func__);
		fwu->config_area = DP_CONFIG_AREA;
		retval = fwu_erase_configuration(rmi4_data);
		if (retval < 0)
			return retval;
	}

	if (fwu->has_guest_code) {
		retval = fwu_erase_guest_code(rmi4_data);
		if (retval < 0)
			return retval;
	}

	return 0;
}

static int fwu_write_f34_v7_blocks(struct synaptics_rmi4_data *rmi4_data,
		unsigned char *block_ptr,
		unsigned short block_cnt, unsigned char command)
{
	int retval;
	unsigned char data_base;
	unsigned char length[2];
	unsigned short transfer;
	unsigned short remaining = block_cnt;
	unsigned short block_number = 0;
	unsigned short left_bytes;
	unsigned short write_size;
	unsigned short max_write_size;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;


	data_base = fwu->f34_fd.data_base_addr;

	retval = fwu_write_f34_partition_id(rmi4_data, command);
	if (retval < 0)
		return retval;

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.block_number,
			(unsigned char *)&block_number,
			sizeof(block_number));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write block number\n",
				__func__);
		return retval;
	}

	do {
		if (remaining / fwu->payload_length)
			transfer = fwu->payload_length;
		else
			transfer = remaining;

		length[0] = (unsigned char)(transfer & MASK_8BIT);
		length[1] = (unsigned char)(transfer >> 8);

		retval = rmi4_data->i2c_write(rmi4_data,
				data_base + fwu->off.transfer_length,
				length,
				sizeof(length));
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to write transfer length (remaining = %d)\n",
					__func__, remaining);
			return retval;
		}

		retval = fwu_write_f34_command(rmi4_data, command);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to write command (remaining = %d)\n",
					__func__, remaining);
			return retval;
		}

#ifdef MAX_WRITE_SIZE
		max_write_size = MAX_WRITE_SIZE;
		if (max_write_size >= transfer * fwu->block_size)
			max_write_size = transfer * fwu->block_size;
		else if (max_write_size > fwu->block_size)
			max_write_size -= max_write_size % fwu->block_size;
		else
			max_write_size = fwu->block_size;
#else
		max_write_size = transfer * fwu->block_size;
#endif
		left_bytes = transfer * fwu->block_size;

		do {
			if (left_bytes / max_write_size)
				write_size = max_write_size;
			else
				write_size = left_bytes;

			retval = rmi4_data->i2c_write(rmi4_data,
					data_base + fwu->off.payload,
					block_ptr,
					write_size);
			if (retval < 0) {
				input_err(true, &rmi4_data->i2c_client->dev,
						"%s: Failed to write block data (remaining = %d)\n",
						__func__, remaining);
				return retval;
			}

			block_ptr += write_size;
			left_bytes -= write_size;
		} while (left_bytes);

		retval = fwu_wait_for_idle(rmi4_data, WRITE_WAIT_MS);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to wait for idle status (remaining = %d)\n",
					__func__, remaining);
			return retval;
		}

		remaining -= transfer;
	} while (remaining);

	return 0;
}

static int fwu_write_f34_v5v6_blocks(struct synaptics_rmi4_data *rmi4_data, unsigned char *block_ptr,
		unsigned short block_cnt, unsigned char command)
{
	int retval;
	unsigned char data_base;
	unsigned char block_number[] = {0, 0};
	unsigned short blk;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	data_base = fwu->f34_fd.data_base_addr;

	block_number[1] |= (fwu->config_area << 5);

	retval = rmi4_data->i2c_write(rmi4_data,
			data_base + fwu->off.block_number,
			block_number,
			sizeof(block_number));
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write block number\n",
				__func__);
		return retval;
	}

	for (blk = 0; blk < block_cnt; blk++) {
		retval = rmi4_data->i2c_write(rmi4_data,
				data_base + fwu->off.payload,
				block_ptr,
				fwu->block_size);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to write block data (block %d)\n",
					__func__, blk);
			return retval;
		}

		retval = fwu_write_f34_command(rmi4_data, command);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to write command for block %d\n",
					__func__, blk);
			return retval;
		}

		retval = fwu_wait_for_idle(rmi4_data, WRITE_WAIT_MS);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to wait for idle status (block %d)\n",
					__func__, blk);
			return retval;
		}

		block_ptr += fwu->block_size;
	}

	return 0;
}

static int fwu_write_f34_blocks(struct synaptics_rmi4_data *rmi4_data,
		unsigned char *block_ptr,
		unsigned short block_cnt, unsigned char cmd)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		retval = fwu_write_f34_v7_blocks(rmi4_data, block_ptr, block_cnt, cmd);
	else
		retval = fwu_write_f34_v5v6_blocks(rmi4_data, block_ptr, block_cnt, cmd);

	return retval;
}

static int fwu_write_utility_parameter(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	unsigned char checksum_array[4];
	unsigned char *pbuf;
	unsigned short remaining_size;
	unsigned short utility_param_size;
	unsigned long checksum;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	utility_param_size = fwu->blkcount.utility_param * fwu->block_size;
	retval = fwu_allocate_read_config_buf(rmi4_data, utility_param_size);
	if (retval < 0)
		return retval;
	memset(fwu->read_config_buf, 0x00, utility_param_size);

	pbuf = fwu->read_config_buf;
	remaining_size = utility_param_size - 4;

	for (ii = 0; ii < MAX_UTILITY_PARAMS; ii++) {
		if (fwu->img_data.utility_param_id[ii] == UNUSED)
			continue;

#ifdef F51_DISCRETE_FORCE
		if (fwu->img_data.utility_param_id[ii] == FORCE_PARAMETER) {
			if (fwu->bl_mode_device) {
				input_info(true, &rmi4_data->i2c_client->dev,
						"%s: Device in bootloader mode, skipping calibration data restoration\n",
						__func__);
				goto image_param;
			}
			retval = secure_memcpy(&(pbuf[4]),
					remaining_size - 4,
					fwu->cal_data,
					fwu->cal_data_buf_size,
					fwu->cal_data_size);
			if (retval < 0) {
				input_err(true, &rmi4_data->i2c_client->dev,
						"%s: Failed to copy force calibration data\n",
						__func__);
				return retval;
			}
			pbuf[0] = FORCE_PARAMETER;
			pbuf[1] = 0x00;
			pbuf[2] = (4 + fwu->cal_data_size) / 2;
			pbuf += (fwu->cal_data_size + 4);
			remaining_size -= (fwu->cal_data_size + 4);
			continue;
		}
image_param:
#endif

		retval = secure_memcpy(pbuf,
				remaining_size,
				fwu->img_data.utility_param[ii].data,
				fwu->img_data.utility_param[ii].size,
				fwu->img_data.utility_param[ii].size);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to copy utility parameter data\n",
					__func__);
			return retval;
		}
		pbuf += fwu->img_data.utility_param[ii].size;
		remaining_size -= fwu->img_data.utility_param[ii].size;
	}

	synaptics_rmi_calculate_checksum((unsigned short *)fwu->read_config_buf,
			((utility_param_size - 4) / 2),
			&checksum);

	synaptics_rmi_convert_to_little_endian(checksum_array, checksum);

	fwu->read_config_buf[utility_param_size - 4] = checksum_array[0];
	fwu->read_config_buf[utility_param_size - 3] = checksum_array[1];
	fwu->read_config_buf[utility_param_size - 2] = checksum_array[2];
	fwu->read_config_buf[utility_param_size - 1] = checksum_array[3];

	retval = fwu_write_f34_blocks(rmi4_data, (unsigned char *)fwu->read_config_buf,
			fwu->blkcount.utility_param, CMD_WRITE_UTILITY_PARAM);
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_write_configuration(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	return fwu_write_f34_blocks(rmi4_data, (unsigned char *)fwu->config_data,
			fwu->config_block_count, CMD_WRITE_CONFIG);
}

static int fwu_write_flash_configuration(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	fwu->config_area = FLASH_CONFIG_AREA;
	fwu->config_data = fwu->img_data.fl_config.data;
	fwu->config_size = fwu->img_data.fl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	if (fwu->config_block_count != fwu->blkcount.fl_config) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Flash configuration size mismatch\n",
				__func__);
		return -EINVAL;
	}

	retval = fwu_erase_configuration(rmi4_data);
	if (retval < 0)
		return retval;

	retval = fwu_write_configuration(rmi4_data);
	if (retval < 0)
		return retval;

	rmi4_data->reset_device(rmi4_data);

	return 0;
}

static int fwu_write_ui_configuration(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	fwu->config_area = UI_CONFIG_AREA;
	fwu->config_data = fwu->img_data.ui_config.data;
	fwu->config_size = fwu->img_data.ui_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	return fwu_write_configuration(rmi4_data);
}

static int fwu_write_dp_configuration(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	fwu->config_area = DP_CONFIG_AREA;
	fwu->config_data = fwu->img_data.dp_config.data;
	fwu->config_size = fwu->img_data.dp_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	return fwu_write_configuration(rmi4_data);
}
static int fwu_write_pm_configuration(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	fwu->config_area = PM_CONFIG_AREA;
	fwu->config_data = fwu->img_data.pm_config.data;
	fwu->config_size = fwu->img_data.pm_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	return fwu_write_configuration(rmi4_data);
}

static int fwu_write_guest_code(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	int retval;
	unsigned short guest_code_block_count;

	guest_code_block_count = fwu->img_data.guest_code.size / fwu->block_size;

	retval = fwu_write_f34_blocks(rmi4_data, (unsigned char *)fwu->img_data.guest_code.data,
			guest_code_block_count, CMD_WRITE_GUEST_CODE);
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_write_bootloader(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	int retval;
	unsigned short bootloader_block_count;

	bootloader_block_count = fwu->img_data.bl_image.size / fwu->block_size;

	fwu->write_bootloader = true;
	retval = fwu_write_f34_blocks(rmi4_data, (unsigned char *)fwu->img_data.bl_image.data,
			bootloader_block_count, CMD_WRITE_BOOTLOADER);
	fwu->write_bootloader = false;

	return retval;
}

static int fwu_write_firmware(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	unsigned short firmware_block_count;

	firmware_block_count = fwu->img_data.ui_firmware.size / fwu->block_size;

	input_info(true, &rmi4_data->i2c_client->dev,
					"%s: Write Run!!!\n",
					__func__);

	return fwu_write_f34_blocks(rmi4_data, (unsigned char *)fwu->img_data.ui_firmware.data,
			firmware_block_count, CMD_WRITE_FW);
}

static int fwu_write_partition_table_v8(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	fwu->config_area = FLASH_CONFIG_AREA;
	fwu->config_data = fwu->img_data.fl_config.data;
	fwu->config_size = fwu->img_data.fl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	if (fwu->config_block_count != fwu->blkcount.fl_config) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Flash configuration size mismatch\n",
				__func__);
		return -EINVAL;
	}

	retval = fwu_write_configuration(rmi4_data);
	if (retval < 0)
		return retval;

	rmi4_data->reset_device(rmi4_data);

	return 0;
}

static int fwu_write_partition_table_v7(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned short block_count;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	block_count = fwu->blkcount.bl_config;
	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_size = fwu->block_size * block_count;

	retval = fwu_allocate_read_config_buf(rmi4_data, fwu->config_size);
	if (retval < 0)
		return retval;

	retval = fwu_read_f34_blocks(rmi4_data, block_count, CMD_READ_CONFIG);
	if (retval < 0)
		return retval;

	retval = fwu_erase_configuration(rmi4_data);
	if (retval < 0)
		return retval;

	retval = fwu_write_flash_configuration(rmi4_data);
	if (retval < 0)
		return retval;

	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_data = fwu->read_config_buf;
	fwu->config_size = fwu->img_data.bl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	retval = fwu_write_configuration(rmi4_data);
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_write_bl_area_v7(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	bool has_utility_param;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	has_utility_param = fwu->has_utility_param;

	if (fwu->has_utility_param) {
		fwu->config_area = UPP_AREA;
		retval = fwu_erase_configuration(rmi4_data);
		if (retval < 0)
			return retval;
	}

	fwu->config_area = BL_CONFIG_AREA;
	retval = fwu_erase_configuration(rmi4_data);
	if (retval < 0)
		return retval;

	fwu->config_area = FLASH_CONFIG_AREA;
	retval = fwu_erase_configuration(rmi4_data);
	if (retval < 0)
		return retval;

	retval = fwu_erase_bootloader(rmi4_data);
	if (retval < 0)
		return retval;

	retval = fwu_write_bootloader(rmi4_data);
	if (retval < 0)
		return retval;

	msleep(200);
	rmi4_data->reset_device(rmi4_data);

	fwu->config_area = FLASH_CONFIG_AREA;
	fwu->config_data = fwu->img_data.fl_config.data;
	fwu->config_size = fwu->img_data.fl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;
	retval = fwu_write_configuration(rmi4_data);
	if (retval < 0)
		return retval;
	rmi4_data->reset_device(rmi4_data);

	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_data = fwu->img_data.bl_config.data;
	fwu->config_size = fwu->img_data.bl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;
	retval = fwu_write_configuration(rmi4_data);
	if (retval < 0)
		return retval;

	if (fwu->img_data.contains_utility_param) {
		retval = fwu_write_utility_parameter(rmi4_data);
		if (retval < 0)
			return retval;
	}

	return 0;
}

static int fwu_do_reflash(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	bool do_bl_update = false;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (!fwu->new_partition_table) {
		retval = fwu_check_ui_firmware_size(rmi4_data);
		if (retval < 0)
			return retval;

		retval = fwu_check_ui_configuration_size(rmi4_data);
		if (retval < 0)
			return retval;

		if (fwu->flash_properties.has_disp_config &&
				fwu->img_data.contains_disp_config) {
			retval = fwu_check_dp_configuration_size(rmi4_data);
			if (retval < 0)
				return retval;
		}

		if (fwu->has_guest_code && fwu->img_data.contains_guest_code) {
			retval = fwu_check_guest_code_size(rmi4_data);
			if (retval < 0)
				return retval;
		}
	} else if (fwu->bl_version == BL_V7) {
		retval = fwu_check_bl_configuration_size(rmi4_data);
		if (retval < 0)
			return retval;
	}

	if (!fwu->has_utility_param && fwu->img_data.contains_utility_param) {
		if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
			do_bl_update = true;
	}

	if (fwu->has_utility_param && !fwu->img_data.contains_utility_param) {
		if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
			do_bl_update = true;
	}

	if (!do_bl_update && fwu->incompatible_partition_tables) {
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: Incompatible partition tables\n",
				__func__);
		return -EINVAL;
	} else if (!do_bl_update && fwu->new_partition_table) {
		if (!fwu->force_update) {
			input_info(true, &rmi4_data->i2c_client->dev,
					"%s: Partition table mismatch\n",
					__func__);
			return -EINVAL;
		}
	}

	retval = fwu_erase_all(rmi4_data);
	if (retval < 0)
		return retval;

	if (do_bl_update) {
		retval = fwu_write_bl_area_v7(rmi4_data);
		if (retval < 0)
			return retval;
		pr_notice("%s: Bootloader area programmed\n", __func__);
	} else if (fwu->bl_version == BL_V7 && fwu->new_partition_table) {
		retval = fwu_write_partition_table_v7(rmi4_data);
		if (retval < 0)
			return retval;
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: Partition table programmed\n", __func__);
	} else if (fwu->bl_version == BL_V8) {
		retval = fwu_write_partition_table_v8(rmi4_data);
		if (retval < 0)
			return retval;
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: Partition table programmed\n", __func__);
	}

	retval = fwu_write_firmware(rmi4_data);
	if (retval < 0)
		return retval;
	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Firmware programmed\n", __func__);

	fwu->config_area = UI_CONFIG_AREA;
	retval = fwu_write_ui_configuration(rmi4_data);
	if (retval < 0)
		return retval;
	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Configuration programmed\n", __func__);

	if (fwu->flash_properties.has_disp_config &&
			fwu->img_data.contains_disp_config) {
		retval = fwu_write_dp_configuration(rmi4_data);
		if (retval < 0)
			return retval;
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: Display configuration programmed\n", __func__);
	}

	if (fwu->has_guest_code && fwu->img_data.contains_guest_code) {
		retval = fwu_write_guest_code(rmi4_data);
		if (retval < 0)
			return retval;
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: Guest code programmed\n", __func__);
	}

	return retval;
}

static void fwu_parse_image_header_10_bl_container(struct synaptics_rmi4_data *rmi4_data, unsigned char *image)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	unsigned char ii;
	unsigned char num_of_containers;
	unsigned int addr;
	unsigned int container_id;
	unsigned int length;
	unsigned char *content;
	struct container_descriptor *descriptor;

	num_of_containers = (fwu->img_data.bootloader.size - 4) / 4;

	for (ii = 1; ii <= num_of_containers; ii++) {
		addr = le_to_uint(fwu->img_data.bootloader.data + (ii * 4));
		descriptor = (struct container_descriptor *)(image + addr);
		container_id = descriptor->container_id[0] |
				descriptor->container_id[1] << 8;
		content = image + le_to_uint(descriptor->content_address);
		length = le_to_uint(descriptor->content_length);
		switch (container_id) {
		case BL_CONFIG_CONTAINER:
		case GLOBAL_PARAMETERS_CONTAINER:
			fwu->img_data.bl_config.data = content;
			fwu->img_data.bl_config.size = length;
			break;
		case BL_LOCKDOWN_INFO_CONTAINER:
		case DEVICE_CONFIG_CONTAINER:
			fwu->img_data.lockdown.data = content;
			fwu->img_data.lockdown.size = length;
			break;
		default:
			break;
		};
	}

	return;
}

void fwu_parse_image_header_10_simple(struct synaptics_rmi4_data *rmi4_data, unsigned char *image)
{
	unsigned char ii;
	unsigned char num_of_containers;
	unsigned int addr;
	unsigned int offset;
	unsigned int container_id;
	unsigned int length;
	unsigned char *content;
	struct container_descriptor *descriptor;
	struct image_header_10 *header;

	header = (struct image_header_10 *)image;

	/* address of top level container */
	offset = le_to_uint(header->top_level_container_start_addr);
	descriptor = (struct container_descriptor *)(image + offset);

	/* address of top level container content */
	offset = le_to_uint(descriptor->content_address);
	num_of_containers = le_to_uint(descriptor->content_length) / 4;

	for (ii = 0; ii < num_of_containers; ii++) {
		addr = le_to_uint(image + offset);
		offset += 4;
		descriptor = (struct container_descriptor *)(image + addr);
		container_id = descriptor->container_id[0] |
				descriptor->container_id[1] << 8;
		content = image + le_to_uint(descriptor->content_address);
		length = le_to_uint(descriptor->content_length);

		if(container_id == UI_CONFIG_CONTAINER || container_id == CORE_CONFIG_CONTAINER){
			input_info(true, &rmi4_data->i2c_client->dev,
								"%s: container_id=%d, length=%d data=[0x%02x/0x%02x/0x%02x/0x%02x]\n",
								__func__, container_id, length,
								content[0], content[1], content[2], content[3]);

			rmi4_data->ic_revision_of_bin = (int)content[2];
			rmi4_data->fw_version_of_bin = (int)content[3];
		}
		if(container_id == GENERAL_INFORMATION_CONTAINER){
			snprintf(rmi4_data->product_id_string_of_bin, SYNAPTICS_RMI4_PRODUCT_ID_SIZE, "%c%c%c%c%c",
				content[24], content[25], content[26], content[27], content[28]);

			input_info(true, &rmi4_data->i2c_client->dev,
							"%s: container_id=%d, length=%d data=[%s]\n",
							__func__, container_id, length,
							rmi4_data->product_id_string_of_bin);
		}
	}

	return;
}
EXPORT_SYMBOL(fwu_parse_image_header_10_simple);
void fwu_parse_image_header_0506_simple(struct synaptics_rmi4_data *rmi4_data)
{
	struct image_header_05_06 *header;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	const struct firmware *fw_entry = NULL;
	unsigned char *image = NULL;
	int retval;

	rmi4_data->firmware_name = rmi4_data->board->firmware_name;
	if (rmi4_data->firmware_name == NULL) {
		input_err(true, &rmi4_data->i2c_client->dev, "%s: firmware name is NULL!, Skip update firmware.\n",
				__func__);
		return;
	} else {
		input_info(true, &rmi4_data->i2c_client->dev, "%s: Load firmware : %s\n",
					__func__, rmi4_data->firmware_name);
	}

	retval = request_firmware(&fw_entry, rmi4_data->firmware_name, &rmi4_data->i2c_client->dev);
	if (retval) {
		input_err(true, &rmi4_data->i2c_client->dev, "%s: Firmware image %s not available\n",
				__func__, rmi4_data->firmware_name);
		return;
	}
	image = (unsigned char *) fw_entry->data;

	header = (struct image_header_05_06 *)image;

	fwu->img_data.checksum = le_to_uint(header->checksum);

	fwu->img_data.bl_version = header->header_version;

	fwu->img_data.contains_bootloader = header->options_bootloader;
	if (fwu->img_data.contains_bootloader)
		fwu->img_data.bootloader_size = le_to_uint(header->bootloader_size);

	fwu->img_data.ui_firmware.size = le_to_uint(header->firmware_size);
	if (fwu->img_data.ui_firmware.size) {
		fwu->img_data.ui_firmware.data = image + IMAGE_AREA_OFFSET;
		if (fwu->img_data.contains_bootloader)
			fwu->img_data.ui_firmware.data += fwu->img_data.bootloader_size;
	}

	if ((fwu->img_data.bl_version == BL_V6) && header->options_tddi)
		fwu->img_data.ui_firmware.data = image + IMAGE_AREA_OFFSET;

	fwu->img_data.ui_config.size = le_to_uint(header->config_size);
	if (fwu->img_data.ui_config.size) {
		fwu->img_data.ui_config.data = fwu->img_data.ui_firmware.data +
				fwu->img_data.ui_firmware.size;
	}

	rmi4_data->ic_revision_of_bin = fwu->img_data.ui_config.data[2];
	rmi4_data->fw_version_of_bin = fwu->img_data.ui_config.data[3];

	release_firmware(fw_entry);
	return;
}
EXPORT_SYMBOL(fwu_parse_image_header_0506_simple);

static void fwu_parse_image_header_05_06(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	const unsigned char *image;
	struct image_header_05_06 *header;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	image = fwu->image;
	header = (struct image_header_05_06 *)image;

	fwu->img_data.checksum = le_to_uint(header->checksum);

	fwu->img_data.bl_version = header->header_version;

	fwu->img_data.contains_bootloader = header->options_bootloader;
	if (fwu->img_data.contains_bootloader)
		fwu->img_data.bootloader_size = le_to_uint(header->bootloader_size);

	fwu->img_data.ui_firmware.size = le_to_uint(header->firmware_size);
	if (fwu->img_data.ui_firmware.size) {
		fwu->img_data.ui_firmware.data = image + IMAGE_AREA_OFFSET;
		if (fwu->img_data.contains_bootloader)
			fwu->img_data.ui_firmware.data += fwu->img_data.bootloader_size;
	}

	if ((fwu->img_data.bl_version == BL_V6) && header->options_tddi)
		fwu->img_data.ui_firmware.data = image + IMAGE_AREA_OFFSET;

	fwu->img_data.ui_config.size = le_to_uint(header->config_size);
	if (fwu->img_data.ui_config.size) {
		fwu->img_data.ui_config.data = fwu->img_data.ui_firmware.data +
				fwu->img_data.ui_firmware.size;
	}

	if (fwu->img_data.contains_bootloader|| header->options_tddi)
		fwu->img_data.contains_disp_config = true;
	else
		fwu->img_data.contains_disp_config = false;

	if (fwu->img_data.contains_disp_config) {
		fwu->img_data.disp_config_offset = le_to_uint(header->dsp_cfg_addr);
		fwu->img_data.dp_config.size = le_to_uint(header->dsp_cfg_size);
		fwu->img_data.dp_config.data = image + fwu->img_data.disp_config_offset;
	} else {
		retval = secure_memcpy(fwu->img_data.cstmr_product_id,
				sizeof(fwu->img_data.cstmr_product_id),
				header->cstmr_product_id,
				sizeof(header->cstmr_product_id),
				PRODUCT_ID_SIZE);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to copy custom product ID string\n",
					__func__);
		}
		fwu->img_data.cstmr_product_id[PRODUCT_ID_SIZE] = 0;
	}

	fwu->img_data.contains_firmware_id = header->options_firmware_id;
	if (fwu->img_data.contains_firmware_id)
		fwu->img_data.firmware_id = le_to_uint(header->firmware_id);

	retval = secure_memcpy(fwu->img_data.product_id,
			sizeof(fwu->img_data.product_id),
			header->product_id,
			sizeof(header->product_id),
			PRODUCT_ID_SIZE);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to copy product ID string\n",
				__func__);
	}
	fwu->img_data.product_id[PRODUCT_ID_SIZE] = 0;

	fwu->img_data.lockdown.size = LOCKDOWN_SIZE;
	fwu->img_data.lockdown.data = image + IMAGE_AREA_OFFSET - LOCKDOWN_SIZE;

	return;
}

static void fwu_parse_image_header_10(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	unsigned char ii;
	unsigned char num_of_containers;
	unsigned int addr;
	unsigned int offset;
	unsigned int container_id;
	unsigned int length;
	unsigned char *image;
	unsigned char *content;
	struct container_descriptor *descriptor;
	struct image_header_10 *header;

	image = fwu->image;
	header = (struct image_header_10 *)image;

	fwu->img_data.checksum = le_to_uint(header->checksum);

	input_info(true, &rmi4_data->i2c_client->dev,
						"%s: fwu->img_data.checksum=%d\n",
						__func__, fwu->img_data.checksum);

	/* address of top level container */
	offset = le_to_uint(header->top_level_container_start_addr);
	descriptor = (struct container_descriptor *)(image + offset);

	/* address of top level container content */
	offset = le_to_uint(descriptor->content_address);
	num_of_containers = le_to_uint(descriptor->content_length) / 4;

	for (ii = 0; ii < num_of_containers; ii++) {
		addr = le_to_uint(image + offset);
		offset += 4;
		descriptor = (struct container_descriptor *)(image + addr);
		container_id = descriptor->container_id[0] |
				descriptor->container_id[1] << 8;
		content = image + le_to_uint(descriptor->content_address);
		length = le_to_uint(descriptor->content_length);

		input_info(true, &rmi4_data->i2c_client->dev,
							"%s: container_id=%d, length=%d\n",
							__func__, container_id, length);

		
		switch (container_id) {
		case UI_CONTAINER:
		case CORE_CODE_CONTAINER:
			fwu->img_data.ui_firmware.data = content;
			fwu->img_data.ui_firmware.size = length;
			break;
		case UI_CONFIG_CONTAINER:
		case CORE_CONFIG_CONTAINER:
			fwu->img_data.ui_config.data = content;
			fwu->img_data.ui_config.size = length;
			break;
		case BL_CONTAINER:
			fwu->img_data.bl_version = *content;
			fwu->img_data.bootloader.data = content;
			fwu->img_data.bootloader.size = length;
			fwu_parse_image_header_10_bl_container(rmi4_data, image);
			break;
		case GUEST_CODE_CONTAINER:
			fwu->img_data.contains_guest_code = true;
			fwu->img_data.guest_code.data = content;
			fwu->img_data.guest_code.size = length;
			break;
		case DISPLAY_CONFIG_CONTAINER:
			fwu->img_data.contains_disp_config = true;
			fwu->img_data.dp_config.data = content;
			fwu->img_data.dp_config.size = length;
			break;
		case FLASH_CONFIG_CONTAINER:
			fwu->img_data.contains_flash_config = true;
			fwu->img_data.fl_config.data = content;
			fwu->img_data.fl_config.size = length;
			break;
		case GENERAL_INFORMATION_CONTAINER:
			fwu->img_data.contains_firmware_id = true;
			fwu->img_data.firmware_id = le_to_uint(content + 4);
			break;
		default:
			break;
		}
	}

	return;
}

static void fwu_compare_partition_tables(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	fwu->incompatible_partition_tables = false;
	
	if (fwu->phyaddr.bl_image != fwu->img_data.phyaddr.bl_image)
		fwu->incompatible_partition_tables = true;
	else if (fwu->phyaddr.lockdown != fwu->img_data.phyaddr.lockdown)
		fwu->incompatible_partition_tables = true;
	else if (fwu->phyaddr.bl_config != fwu->img_data.phyaddr.bl_config)
		fwu->incompatible_partition_tables = true;
	else if (fwu->phyaddr.utility_param != fwu->img_data.phyaddr.utility_param)
		fwu->incompatible_partition_tables = true;

	if (fwu->bl_version == BL_V7) {
		if (fwu->phyaddr.fl_config != fwu->img_data.phyaddr.fl_config)
			fwu->incompatible_partition_tables = true;
	}

	fwu->new_partition_table = false;

	if (fwu->phyaddr.ui_firmware != fwu->img_data.phyaddr.ui_firmware)
		fwu->new_partition_table = true;
	else if (fwu->phyaddr.ui_config != fwu->img_data.phyaddr.ui_config)
		fwu->new_partition_table = true;

	if (fwu->flash_properties.has_disp_config) {
		if (fwu->phyaddr.dp_config != fwu->img_data.phyaddr.dp_config)
			fwu->new_partition_table = true;
	}

	if (fwu->has_guest_code) {
		if (fwu->phyaddr.guest_code != fwu->img_data.phyaddr.guest_code)
			fwu->new_partition_table = true;
	}

	return;
}

static int fwu_parse_image_info(struct synaptics_rmi4_data *rmi4_data)
{
	struct image_header_10 *header;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	header = (struct image_header_10 *)fwu->image;

	memset(&fwu->img_data, 0x00, sizeof(fwu->img_data));

	input_dbg(false, &rmi4_data->i2c_client->dev,
						"%s: header->major_header_version = %d\n",
						__func__, header->major_header_version);

	switch (header->major_header_version) {
	case IMAGE_HEADER_VERSION_10:
		fwu_parse_image_header_10(rmi4_data);
		break;
	case IMAGE_HEADER_VERSION_05:
	case IMAGE_HEADER_VERSION_06:
		fwu_parse_image_header_05_06(rmi4_data);
		break;
	default:
		input_err(false, &rmi4_data->i2c_client->dev,
				"%s: Unsupported image file format (0x%02x)\n",
				__func__, header->major_header_version);
		return -EINVAL;
	}

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8) {
		if (!fwu->img_data.contains_flash_config) {
			input_err(false, &rmi4_data->i2c_client->dev,
					"%s: No flash config found in firmware image\n",
					__func__);
			return -EINVAL;
		}

		fwu_parse_partition_table(rmi4_data, fwu->img_data.fl_config.data,
				&fwu->img_data.blkcount, &fwu->img_data.phyaddr);

		if (fwu->img_data.blkcount.utility_param)
			fwu->img_data.contains_utility_param = true;

		fwu_compare_partition_tables(rmi4_data);
	} else {
		fwu->new_partition_table = false;
		fwu->incompatible_partition_tables = false;
	}

	return 0;
}

static int fwu_start_reflash(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = 0;
	enum flash_area flash_area;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (rmi4_data->sensor_sleep) {
		input_err(false, &rmi4_data->i2c_client->dev,
				"%s: Sensor sleeping\n",
				__func__);
		return -ENODEV;
	}

	rmi4_data->stay_awake = true;

	mutex_lock(&rmi4_data->rmi4_reflash_mutex);

	input_info(false, &rmi4_data->i2c_client->dev, 
					"%s: Start of reflash process\n", __func__);

	retval = fwu_parse_image_info(rmi4_data);
	if (retval < 0)
		goto exit;

	if (fwu->blkcount.total_count != fwu->img_data.blkcount.total_count) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Flash size mismatch\n",
				__func__);
		retval = -EINVAL;
		goto exit;
	}

	if (fwu->bl_version != fwu->img_data.bl_version) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Bootloader version mismatch\n",
				__func__);
		retval = -EINVAL;
		goto exit;
	}

	fwu->force_update = true;

	if (!fwu->force_update && fwu->new_partition_table) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Partition table mismatch\n",
				__func__);
		retval = -EINVAL;
		goto exit;
	}

	retval = fwu_read_f34_queries(rmi4_data);
	if (retval < 0)
		goto exit;

	retval = fwu_read_flash_status(rmi4_data);
	if (retval < 0)
		goto exit;

	if (fwu->in_bl_mode) {
		fwu->bl_mode_device = true;
		input_info(false, &rmi4_data->i2c_client->dev,
				"%s: Device in bootloader mode\n",
				__func__);
	} else {
		fwu->bl_mode_device = false;
	}

	flash_area = UI_FIRMWARE;
	rmi4_data->doing_reflash = true;
	fwu_enter_flash_prog(rmi4_data);

	retval = fwu_do_reflash(rmi4_data);

	if (retval < 0) {
		input_err(false, &rmi4_data->i2c_client->dev,
				"%s: Failed to do reflash\n",
				__func__);
	}

	rmi4_data->reset_device(rmi4_data);
	synaptics_rmi4_fwu_reset(rmi4_data);
	rmi4_data->doing_reflash = false;

	fwu->bootloader_id[0] = fwu->bootloader_id_ic[0];
	fwu->bootloader_id[1] = fwu->bootloader_id_ic[1];

exit:
	input_info(false, &rmi4_data->i2c_client->dev,
			"%s: End of reflash process\n", __func__);

	mutex_unlock(&rmi4_data->rmi4_reflash_mutex);

	return retval;
}

static int fwu_do_write_guest_code(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (!fwu->has_guest_code) {
		input_err(true, &rmi4_data->i2c_client->dev,
			"%s: Firmware does not support Guest Code.\n",
			__func__);
		retval = -EINVAL;
	}
	if (fwu->guest_code_block_count !=
		(fwu->img.guestCode.size/fwu->block_size)) {
		input_err(true, &rmi4_data->i2c_client->dev,
			"%s: Size of Guest Code not match (dev: %x, img: %x).\n",
			__func__, fwu->guest_code_block_count,
			fwu->img.guestCode.size/fwu->block_size);
		retval = -EINVAL;
	}

	retval = fwu_enter_flash_prog(rmi4_data);
	if (retval < 0)
		return retval;

	input_dbg(false, &rmi4_data->i2c_client->dev,
			"%s: Entered flash prog mode\n",
			__func__);

	retval = fwu_write_bootloader_id(rmi4_data);
	if (retval < 0)
		return retval;

	input_dbg(false, &rmi4_data->i2c_client->dev,
			"%s: Bootloader ID written\n",
			__func__);

	retval = fwu_write_f34_command(rmi4_data, CMD_ERASE_GUEST_CODE);
	if (retval < 0)
		return retval;

	input_dbg(false, &rmi4_data->i2c_client->dev,
			"%s: Erase command written\n",
			__func__);

	retval = fwu_wait_for_idle(rmi4_data, ERASE_WAIT_MS);
	if (retval < 0)
		return retval;

	input_dbg(false, &rmi4_data->i2c_client->dev,
			"%s: Idle status detected\n",
			__func__);

//	retval = fwu_write_guest_code_block(rmi4_data);
	if (retval < 0)
		return retval;

	input_info(true, &rmi4_data->i2c_client->dev, "%s: guest code written\n",
			__func__);

	return retval;
}

static int fwu_start_write_config(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned short config_area;
	//unsigned int device_fw_id;
//	unsigned int image_fw_id;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	retval = fwu_parse_image_info(rmi4_data);
	if (retval < 0)
		return -EINVAL;

	switch (fwu->config_area) {
	case UI_CONFIG_AREA:
		/*device_fw_id = rmi4_data->firmware_id;
		retval = fwu_get_image_firmware_id(&image_fw_id);
		if (retval < 0)
			return retval;
		if (device_fw_id != image_fw_id) {
			dev_err(rmi4_data->pdev->dev.parent,
					"%s: Device and image firmware IDs don't match\n",
					__func__);
			return -EINVAL;
		}*/
		retval = fwu_check_ui_configuration_size(rmi4_data);
		if (retval < 0)
			return retval;
		break;
	case DP_CONFIG_AREA:
		if (!fwu->flash_properties.has_disp_config) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Display configuration not supported\n",
					__func__);
			return -EINVAL;
		}
		if (!fwu->img_data.contains_disp_config) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: No display configuration in firmware image\n",
					__func__);
			return -EINVAL;
		}
		retval = fwu_check_dp_configuration_size(rmi4_data);
		if (retval < 0)
			return retval;
		break;
	case PM_CONFIG_AREA:
		if (!fwu->flash_properties.has_pm_config) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Permanent configuration not supported\n",
					__func__);
			return -EINVAL;
		}
		if (!fwu->img_data.contains_perm_config) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: No permanent configuration in firmware image\n",
					__func__);
			return -EINVAL;
		}
		retval = fwu_check_pm_configuration_size(rmi4_data);
		if (retval < 0)
			return retval;
		break;
	default:
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Configuration not supported\n",
				__func__);
		return -EINVAL;
	}

	if (rmi4_data->sensor_sleep) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Sensor sleeping\n",
				__func__);
		return -ENODEV;
	}

	rmi4_data->stay_awake = true;

	mutex_lock(&rmi4_data->rmi4_reflash_mutex);

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Start of write config process\n", __func__);

	config_area = fwu->config_area;

	retval = fwu_enter_flash_prog(rmi4_data);
	if (retval < 0)
		goto exit;

	fwu->config_area = config_area;

	if (fwu->config_area != PM_CONFIG_AREA) {
		retval = fwu_erase_configuration(rmi4_data);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to erase config\n",
					__func__);
			goto exit;
		}
	}

	switch (fwu->config_area) {
	case UI_CONFIG_AREA:
		retval = fwu_write_ui_configuration(rmi4_data);
		if (retval < 0)
			goto exit;
		break;
	case DP_CONFIG_AREA:
		retval = fwu_write_dp_configuration(rmi4_data);
		if (retval < 0)
			goto exit;
		break;
	case PM_CONFIG_AREA:
		retval = fwu_write_pm_configuration(rmi4_data);
		if (retval < 0)
			goto exit;
		break;
	}

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Config written\n", __func__);

exit:
	switch (fwu->config_area) {
	case UI_CONFIG_AREA:
		rmi4_data->reset_device(rmi4_data);
		break;
	case DP_CONFIG_AREA:
	case PM_CONFIG_AREA:
		rmi4_data->reset_device(rmi4_data);
		break;
	}

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: End of write config process\n", __func__);

	mutex_unlock(&rmi4_data->rmi4_reflash_mutex);

	rmi4_data->stay_awake = false;

	return retval;
}

static int fwu_start_write_guest_code(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (!fwu->ext_data_source)
		return -EINVAL;

	fwu_img_parse_format(rmi4_data);

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Start of update guest code process\n", __func__);

	retval = fwu_do_write_guest_code(rmi4_data);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write config\n",
				__func__);
	}

	rmi4_data->reset_device(rmi4_data);

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: End of write guest code process\n", __func__);

	return retval;
}

static int fwu_do_read_config(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned short block_count;
	unsigned short config_area;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	switch (fwu->config_area) {
	case UI_CONFIG_AREA:
		block_count = fwu->blkcount.ui_config;
		break;
	case DP_CONFIG_AREA:
		if (!fwu->flash_properties.has_disp_config) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Display configuration not supported\n",
					__func__);
			return -EINVAL;
		}
		block_count = fwu->blkcount.dp_config;
		break;
	case PM_CONFIG_AREA:
		if (!fwu->flash_properties.has_pm_config) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Permanent configuration not supported\n",
					__func__);
			return -EINVAL;
		}
		block_count = fwu->blkcount.pm_config;
		break;
	case BL_CONFIG_AREA:
		if (!fwu->flash_properties.has_bl_config) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Bootloader configuration not supported\n",
					__func__);
			return -EINVAL;
		}
		block_count = fwu->blkcount.bl_config;
		break;
	case UPP_AREA:
		if (!fwu->has_utility_param) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Utility parameter not supported\n",
					__func__);
			return -EINVAL;
		}
		block_count = fwu->blkcount.utility_param;
		break;
	default:
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Invalid config area\n",
				__func__);
		return -EINVAL;
	}

	if (block_count == 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Invalid block count\n",
				__func__);
		return -EINVAL;
	}

	mutex_lock(&rmi4_data->rmi4_reflash_mutex);

	if (fwu->bl_version == BL_V5 || fwu->bl_version == BL_V6) {
		config_area = fwu->config_area;
		retval = fwu_enter_flash_prog(rmi4_data);
		fwu->config_area = config_area;
		if (retval < 0)
			goto exit;
	}

	fwu->config_size = fwu->block_size * block_count;

	retval = fwu_allocate_read_config_buf(rmi4_data, fwu->config_size);
	if (retval < 0)
		goto exit;

	retval = fwu_read_f34_blocks(rmi4_data, block_count, CMD_READ_CONFIG);

exit:
	if (fwu->bl_version == BL_V5 || fwu->bl_version == BL_V6)
		rmi4_data->reset_device(rmi4_data);

	mutex_unlock(&rmi4_data->rmi4_reflash_mutex);

	return retval;

}

int synaptics_fw_updater(struct synaptics_rmi4_data *rmi4_data, unsigned char *fw_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (!fwu)
		return -ENODEV;

	if (!fwu->initialized)
		return -ENODEV;

	if (!fw_data) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Firmware data is NULL\n", __func__);
		return -ENODEV;
	}

	fwu->ext_data_source = fw_data;
	fwu->config_area = UI_CONFIG_AREA;

	fwu->image = fw_data;

	if (fwu->in_ub_mode) {
		retval = fwu_start_recovery(rmi4_data);
		if (retval < 0)
			return retval;
		goto out_fw_update;
	}

	retval = fwu_start_reflash(rmi4_data);
	if (retval < 0)
		goto out_fw_update;

out_fw_update:
	return retval;
}
EXPORT_SYMBOL(synaptics_fw_updater);
int synaptics_rmi4_set_tsp_test_result_in_config(struct synaptics_rmi4_data *rmi4_data, int value)

{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	int retval;
	unsigned char buf[10] = {0, };
	unsigned long checksum;

	/* read config from IC */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, 2, "%u\n", 1);
	fwu_sysfs_read_config_store(fwu->attr_dir, NULL, buf, 1);

	/* set test result value
	 * MSB 4bit of Customr derined config ID 0 used for factory test in TSP.
	 * PASS : 2, FAIL : 1, NONE: 0.
	 */
	fwu->read_config_buf[0] &= 0x0F;
	fwu->read_config_buf[0] |= value << 4;

	/* check CRC checksum value and re-write checksum in config */
	synaptics_rmi_calculate_checksum((unsigned short *)fwu->read_config_buf,
			(fwu->config_size - CHECKSUM_SIZE) / 2, &checksum);

	synaptics_rmi_convert_to_little_endian(&fwu->read_config_buf[fwu->config_size - CHECKSUM_SIZE],
			checksum);

	rmi4_data->doing_reflash = true;

	retval = fwu_enter_flash_prog(rmi4_data);
	if (retval < 0)
		goto err_config_write;


	retval = fwu_write_bootloader_id(rmi4_data);
	if (retval < 0)
		goto err_config_write;

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Bootloader ID written\n",
			__func__);

//	retval = fwu_write_f34_command(rmi4_data, CMD_ERASE_CONFIG);
	if (retval < 0)
		goto err_config_write;

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Erase command written\n",
			__func__);

	retval = fwu_wait_for_idle(rmi4_data, ERASE_WAIT_MS);
	if (retval < 0)
		goto err_config_write;

	input_info(true, &rmi4_data->i2c_client->dev,
			"%s: Idle status detected\n",
			__func__);

/*	fwu_write_blocks(rmi4_data, fwu->read_config_buf,
			fwu->config_size, CMD_WRITE_CONFIG_BLOCK);
*/
	input_info(true, &rmi4_data->i2c_client->dev, "%s: Config written\n",
			__func__);

err_config_write:
	rmi4_data->reset_device(rmi4_data);
	rmi4_data->doing_reflash = false;

	return retval;
}
static ssize_t fwu_sysfs_show_image(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count)
{
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (count < fwu->config_size) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Not enough space (%ld bytes) in buffer\n",
				__func__, count);
		return -EINVAL;
	}

	memcpy(buf, fwu->read_config_buf, fwu->config_size);

	return fwu->config_size;
}

static ssize_t fwu_sysfs_store_image(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count)
{
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	memcpy((void *)(&fwu->ext_data_source[fwu->data_pos]),
			(const void *)buf,
			count);

	fwu->data_pos += count;
	fwu->img.fw_image = fwu->ext_data_source;
	return count;
}

static ssize_t fwu_sysfs_do_reflash_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (sscanf(buf, "%u", &input) != 1) {
		retval = -EINVAL;
		goto exit;
	}

	if (input & UPDATE_MODE_LOCKDOWN) {
		fwu->do_lockdown = true;
		input &= ~UPDATE_MODE_LOCKDOWN;
	}

	if ((input != UPDATE_MODE_NORMAL) && (input != UPDATE_MODE_FORCE)) {
		retval = -EINVAL;
		goto exit;
	}

	if (input == UPDATE_MODE_FORCE)
		fwu->force_update = true;

	retval = synaptics_fw_updater(rmi4_data, fwu->ext_data_source);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to do reflash\n",
				__func__);
		goto exit;
	}

	retval = count;

exit:
	kfree(fwu->ext_data_source);
	fwu->ext_data_source = NULL;
	fwu->force_update = FORCE_UPDATE;
	fwu->do_lockdown = DO_LOCKDOWN;
	return retval;
}

static ssize_t fwu_sysfs_write_config_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (sscanf(buf, "%u", &input) != 1) {
		retval = -EINVAL;
		goto exit;
	}

	if (input != 1) {
		retval = -EINVAL;
		goto exit;
	}

	retval = fwu_start_write_config(rmi4_data);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write config\n",
				__func__);
		goto exit;
	}

	retval = count;

exit:
	kfree(fwu->ext_data_source);
	fwu->ext_data_source = NULL;
	return retval;
}

static ssize_t fwu_sysfs_read_config_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	if (input != 1)
		return -EINVAL;

	retval = fwu_do_read_config(rmi4_data);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read config\n",
				__func__);
		return retval;
	}

	return count;
}

static ssize_t fwu_sysfs_config_area_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long config_area;
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	retval = kstrtoul(buf, 10, &config_area);
	if (retval)
		return retval;

	fwu->config_area = config_area;

	return count;
}

static ssize_t fwu_sysfs_image_size_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long size;
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	retval = kstrtoul(buf, 10, &size);
	if (retval)
		return retval;

	fwu->img.image_size = size;
	fwu->data_pos = 0;

	kfree(fwu->ext_data_source);
	fwu->ext_data_source = kzalloc(fwu->img.image_size, GFP_KERNEL);
	if (!fwu->ext_data_source) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for image data\n",
				__func__);
		return -ENOMEM;
	}

	return count;
}

static ssize_t fwu_sysfs_block_size_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	return snprintf(buf, PAGE_SIZE, "%u\n", fwu->block_size);
}

static ssize_t fwu_sysfs_firmware_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	return snprintf(buf, PAGE_SIZE, "%u\n", fwu->fw_block_count);
}

static ssize_t fwu_sysfs_configuration_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	return snprintf(buf, PAGE_SIZE, "%u\n", fwu->config_block_count);
}

static ssize_t fwu_sysfs_perm_config_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	return snprintf(buf, PAGE_SIZE, "%u\n", fwu->perm_config_block_count);
}

static ssize_t fwu_sysfs_bl_config_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	return snprintf(buf, PAGE_SIZE, "%u\n", fwu->bl_config_block_count);
}

static ssize_t fwu_sysfs_disp_config_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	return snprintf(buf, PAGE_SIZE, "%u\n", fwu->disp_config_block_count);
}

static ssize_t fwu_sysfs_guest_code_block_count_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	return snprintf(buf, PAGE_SIZE, "%u\n", fwu->guest_code_block_count);
}

static ssize_t fwu_sysfs_write_guest_code_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = rmi_attr_kobj_to_drvdata(kobj);
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (sscanf(buf, "%u", &input) != 1) {
		retval = -EINVAL;
		goto exit;
	}

	if (input != 1) {
		retval = -EINVAL;
		goto exit;
	}

	retval = fwu_start_write_guest_code(rmi4_data);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to write guest code\n",
				__func__);
		goto exit;
	}

	retval = count;

exit:
	kfree(fwu->ext_data_source);
	fwu->ext_data_source = NULL;
	return retval;
}

static void synaptics_rmi4_fwu_attn(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (!fwu)
		return;

	if (fwu->intr_mask & intr_mask)
		fwu_read_flash_status(rmi4_data);

	return;
}

static void fwu_img_scan_x10_container(struct synaptics_rmi4_data *rmi4_data,
		unsigned int list_length, unsigned char *start_addr)
{
	unsigned int i;
	unsigned int length;
	unsigned int contentAddr, addr;
	unsigned short containerId;
	struct block_data block;
	struct img_x10_descriptor *containerDescriptor;
	struct img_x10_bl_container *blContainer;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	for (i = 0; i < list_length; i += 4) {
		contentAddr = extract_uint_le(start_addr + i);
		containerDescriptor =
			(struct img_x10_descriptor *)(fwu->img.fw_image + contentAddr);
		containerId = extract_ushort_le(containerDescriptor->containerID);
		addr = extract_uint_le(containerDescriptor->contentAddress);
		length = extract_uint_le(containerDescriptor->contentLength);
		block.data = fwu->img.fw_image + addr;
		block.size = length;
		switch (containerId) {
		case ID_UI_CONTAINER:
			fwu->img.uiFirmware = block;
			break;
		case ID_UI_CONFIGURATION:
			fwu->img.uiConfig = block;
			fwu->img.configId = (unsigned char *)fwu->img.uiConfig.data;
			break;
		case ID_BOOTLOADER_LOCKDOWN_INFORMATION_CONTAINER:
			fwu->img.lockdown = block;
			break;
		case ID_GUEST_CODE_CONTAINER:
			fwu->img.guestCode = block;
			break;
		case ID_BOOTLOADER_CONTAINER:
			blContainer =
			(struct img_x10_bl_container *)(fwu->img.fw_image + addr);
			fwu->img.blMajorVersion = blContainer->majorVersion;
			fwu->img.blMinorVersion = blContainer->minorVersion;
			fwu->img.bootloaderInfo = block;
			break;
		case ID_PERMANENT_CONFIGURATION_CONTAINER:
			fwu->img.permanent = block;
			break;
		case ID_GENERAL_INFORMATION_CONTAINER:
			fwu->img.packageId = fwu->img.fw_image + addr + 0;
			fwu->img.firmwareId = fwu->img.fw_image + addr + 4;
			fwu->img.dsFirmwareInfo = fwu->img.fw_image + addr + 8;
			break;
		default:
			break;
		}
	}
}

static void fwu_img_parse_x10_topcontainer(struct synaptics_rmi4_data *rmi4_data)
{
	struct img_x10_descriptor *descriptor;
	unsigned int topAddr;
	unsigned int list_length, bl_length;
	unsigned char *start_addr;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	topAddr = extract_uint_le(fwu->img.fw_image +
				IMG_X10_TOP_CONTAINER_OFFSET);
	descriptor = (struct img_x10_descriptor *)
			(fwu->img.fw_image + topAddr);
	list_length = extract_uint_le(descriptor->contentLength);
	start_addr = fwu->img.fw_image +
			extract_uint_le(descriptor->contentAddress);
	fwu_img_scan_x10_container(rmi4_data, list_length, start_addr);
	/* scan sub bootloader container (lockdown container) */
	if (fwu->img.bootloaderInfo.data != NULL) {
		bl_length = fwu->img.bootloaderInfo.size - 4;
		if (bl_length)
			fwu_img_scan_x10_container(rmi4_data, bl_length,
					(unsigned char *)fwu->img.bootloaderInfo.data);
	}
}

static void fwu_img_parse_x10(struct synaptics_rmi4_data *rmi4_data)
{
	fwu_img_parse_x10_topcontainer(rmi4_data);
}

static void fwu_img_parse_x0_x6(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;
	struct img_x0x6_header *header = (struct img_x0x6_header *)fwu->img.fw_image;

	if (header->bootloader_version > 6)
		return;
	fwu->img.blMajorVersion = header->bootloader_version;
	fwu->img.uiFirmware.size = extract_uint_le(header->firmware_size);
	fwu->img.uiFirmware.data = fwu->img.fw_image + IMG_X0_X6_FW_OFFSET;
	fwu->img.uiConfig.size = extract_uint_le(header->config_size);
	fwu->img.uiConfig.data = fwu->img.uiFirmware.data + fwu->img.uiFirmware.size;
	fwu->img.configId = (unsigned char *)fwu->img.uiConfig.data;
	switch (fwu->img.imageFileVersion) {
	case 0x2:
		fwu->img.lockdown.size = 0x30;
		break;
	case 0x3:
	case 0x4:
		fwu->img.lockdown.size = 0x40;
		break;
	case 0x5:
	case 0x6:
		fwu->img.lockdown.size = 0x50;
		if (header->options_firmware_id) {
			fwu->img.firmwareId = header->firmware_id;
			fwu->img.packageId = header->package_id;
			fwu->img.dsFirmwareInfo = header->ds_firmware_info;
		}
		break;
	default:
		break;
	}
	fwu->img.lockdown.data = fwu->img.fw_image +
		IMG_X0_X6_FW_OFFSET - fwu->img.lockdown.size;
}

static void fwu_img_parse_format(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	fwu->polling_mode = POLLING_MODE_DEFAULT;
	fwu->img.firmwareId = NULL;
	fwu->img.packageId = NULL;
	fwu->img.dsFirmwareInfo = NULL;
	fwu->img.uiFirmware.data = NULL;
	fwu->img.uiConfig.data = NULL;
	fwu->img.lockdown.data = NULL;
	fwu->img.guestCode.data = NULL;
	fwu->img.uiConfig.size = 0;
	fwu->img.uiFirmware.size = 0;
	fwu->img.lockdown.size = 0;
	fwu->img.guestCode.size =	 0;

	fwu->img.imageFileVersion = fwu->img.fw_image[IMG_VERSION_OFFSET];

	switch (fwu->img.imageFileVersion) {
	case 0x10:
		fwu_img_parse_x10(rmi4_data);
		break;
	case 0x5:
	case 0x6:
		fwu_img_parse_x0_x6(rmi4_data);
		break;
	default:
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Unsupported image file format $%X\n",
				__func__, fwu->img.imageFileVersion);
		break;
	}

	if (fwu->bl_version == BL_V7) {

		memset(&fwu->img, 0x00, sizeof(fwu->img));

		if (!fwu->img_data.contains_flash_config) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: No flash config found in firmware image\n",
					__func__);
			return;
		}

		fwu_parse_partition_table(rmi4_data, fwu->img_data.fl_config.data,
				&fwu->img_data.blkcount, &fwu->img_data.phyaddr);

		fwu_compare_partition_tables(rmi4_data);
	} else {
		fwu->new_partition_table = false;
	}
}
static int fwu_get_device_config_id(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char config_id_size;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (fwu->bl_version == BL_V7 || fwu->bl_version == BL_V8)
		config_id_size = V7_CONFIG_ID_SIZE;
	else
		config_id_size = V5V6_CONFIG_ID_SIZE;

	retval = rmi4_data->i2c_read(rmi4_data,
				fwu->f34_fd.ctrl_base_addr,
				fwu->config_id,
				config_id_size);
	if (retval < 0)
		return retval;

	return 0;
}

static int synaptics_rmi4_fwu_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct pdt_properties pdt_props;
	struct synaptics_rmi4_fwu_handle *fwu = NULL;

	fwu = kzalloc(sizeof(struct synaptics_rmi4_fwu_handle), GFP_KERNEL);
	if (!fwu) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for fwu\n",
				__func__);
		retval = -ENOMEM;
		goto exit;
	}

	rmi4_data->fwu = fwu;
	fwu->rmi4_data = rmi4_data;

	memset(&fwu->img_data, 0, sizeof(struct img_file_content));

	fwu->img.image_name = kzalloc(MAX_IMAGE_NAME_LEN, GFP_KERNEL);
	if (!fwu->img.image_name) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for image name\n",
				__func__);
		retval = -ENOMEM;
		goto err_free_fwu;
	}

	retval = rmi4_data->i2c_read(rmi4_data,
			PDT_PROPS,
			pdt_props.data,
			sizeof(pdt_props.data));
	if (retval < 0) {
		input_info(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to read PDT properties, assuming 0x00\n",
				__func__);
	} else if (pdt_props.has_bsr) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Reflash for LTS not currently supported\n",
				__func__);
		retval = -ENODEV;
		goto err_free_mem;
	}

	retval = fwu_scan_pdt(rmi4_data);
	if (retval < 0)
		goto err_free_mem;

	if (!fwu->in_ub_mode) {
		retval = fwu_read_f34_queries(rmi4_data);
		if (retval < 0)
			goto err_free_mem;

		retval = fwu_get_device_config_id(rmi4_data);
		if (retval < 0) {
			input_err(true, &rmi4_data->i2c_client->dev,
					"%s: Failed to read device config ID\n",
					__func__);
			goto err_free_mem;
		}
	}

	fwu->force_update = FORCE_UPDATE;
	fwu->do_lockdown = DO_LOCKDOWN;
	fwu->initialized = true;

	fwu->attr_dir = kobject_create_and_add(ATTRIBUTE_FOLDER_NAME,
			&rmi4_data->input_dev->dev.kobj);
	if (!fwu->attr_dir) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs directory\n",
				__func__);
		retval = -ENODEV;
		goto err_attr_dir;
	}

	retval = sysfs_create_bin_file(fwu->attr_dir, &dev_attr_data);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs bin file\n",
				__func__);
		goto err_sysfs_bin;
	}

	retval = sysfs_create_group(fwu->attr_dir, &attr_group);
	if (retval < 0) {
		input_err(true, &rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs attributes\n", __func__);
		retval = -ENODEV;
		goto err_sysfs_attrs;
	}

	return 0;

err_sysfs_attrs:
	sysfs_remove_group(fwu->attr_dir, &attr_group);
	sysfs_remove_bin_file(fwu->attr_dir, &dev_attr_data);

err_sysfs_bin:
	kobject_put(fwu->attr_dir);

err_attr_dir:
err_free_mem:
	kfree(fwu->img.image_name);

err_free_fwu:
	kfree(fwu);
	rmi4_data->fwu = NULL;

exit:
	return retval;
}

static void synaptics_rmi4_fwu_remove(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (!fwu)
		goto exit;

	sysfs_remove_group(fwu->attr_dir, &attr_group);
	sysfs_remove_bin_file(fwu->attr_dir, &dev_attr_data);

	kobject_put(fwu->attr_dir);

	kfree(fwu->read_config_buf);
	kfree(fwu->img.image_name);
	kfree(fwu);
	rmi4_data->fwu = NULL;

exit:
	return;
}
static void synaptics_rmi4_fwu_reset(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_fwu_handle *fwu = rmi4_data->fwu;

	if (!fwu) {
		synaptics_rmi4_fwu_init(rmi4_data);
		return;
	}

	retval = fwu_scan_pdt(rmi4_data);
	if (retval < 0)
		return;

	return;
}

int rmi4_fw_update_module_register(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	retval = synaptics_rmi4_new_function(RMI_FW_UPDATER,
			rmi4_data,
			synaptics_rmi4_fwu_init,
			NULL,
			synaptics_rmi4_fwu_remove,
			synaptics_rmi4_fwu_attn);

	return retval;
}
