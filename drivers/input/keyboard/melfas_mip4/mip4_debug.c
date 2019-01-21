/*
 * MELFAS MIP4 Touchkey
 *
 * Copyright (C) 2016 MELFAS Inc.
 *
 * mip4_debug.c : Debug functions (Optional)
 *
 * Version : 2016.06.30
 */

#include "mip4.h"

#if MIP_USE_DEV

/**
* Dev node output to user
*/
static ssize_t mip4_tk_dev_fs_read(struct file *fp, char *rbuf, size_t cnt, loff_t *fpos)
{
	struct mip4_tk_info *info = fp->private_data;
	int ret = 0;

	//input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	ret = copy_to_user(rbuf, info->dev_fs_buf, cnt);

	//input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	return ret;
}

/**
* Dev node input from user
*/
static ssize_t mip4_tk_dev_fs_write(struct file *fp, const char *wbuf, size_t cnt, loff_t *fpos)
{
	struct mip4_tk_info *info = fp->private_data;
	u8 *buf;
	int ret = 0;
	int cmd = 0;

	//input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	buf = kzalloc(cnt + 1, GFP_KERNEL);

	if ((buf == NULL) || copy_from_user(buf, wbuf, cnt)) {
		input_err(true, &info->client->dev, "%s [ERROR] copy_from_user\n", __func__);
		ret = -EIO;
		goto exit;
	}

	cmd = buf[cnt - 1];

	if (cmd == 1) {
		//input_dbg(true, &info->client->dev, "%s - cmd[%d] w_len[%d] r_len[%d]\n", __func__, cmd, (cnt - 2), buf[cnt - 2]);
		if (mip4_tk_i2c_read(info, buf, (cnt - 2), info->dev_fs_buf, buf[cnt - 2])) {
			input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_read\n", __func__);
		}
	} else if (cmd == 2) {
		//input_dbg(true, &info->client->dev, "%s - cmd[%d] w_len[%d]\n", __func__, cmd, (cnt - 1));
		if (mip4_tk_i2c_write(info, buf, (cnt - 1))) {
			input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_write\n", __func__);
		}
	} else {
		goto exit;
	}

exit:
	kfree(buf);
	//input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return ret;
}

/**
* Open dev node
*/
static int mip4_tk_dev_fs_open(struct inode *node, struct file *fp)
{
	struct mip4_tk_info *info = container_of(node->i_cdev, struct mip4_tk_info, cdev);

	//input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	fp->private_data = info;

	info->dev_fs_buf = kzalloc(1024 * 4, GFP_KERNEL);

	//input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	return 0;
}

/**
* Close dev node
*/
static int mip4_tk_dev_fs_release(struct inode *node, struct file *fp)
{
	struct mip4_tk_info *info = fp->private_data;

	//input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	kfree(info->dev_fs_buf);

	//input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	return 0;
}

/**
* Dev node info
*/
static struct file_operations mip4_tk_dev_fops = {
	.owner = THIS_MODULE,
	.open = mip4_tk_dev_fs_open,
	.release = mip4_tk_dev_fs_release,
	.read = mip4_tk_dev_fs_read,
	.write = mip4_tk_dev_fs_write,
};

/**
* Create dev node
*/
int mip4_tk_dev_create(struct mip4_tk_info *info)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	if (alloc_chrdev_region(&info->mip4_tk_dev, 0, 1, MIP_DEV_NAME)) {
		input_err(true, &client->dev, "%s [ERROR] alloc_chrdev_region\n", __func__);
		ret = -ENOMEM;
		goto error;
	}

	cdev_init(&info->cdev, &mip4_tk_dev_fops);
	info->cdev.owner = THIS_MODULE;

	if (cdev_add(&info->cdev, info->mip4_tk_dev, 1)) {
		input_err(true, &client->dev, "%s [ERROR] cdev_add\n", __func__);
		ret = -EIO;
		goto error;
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return 0;
}

#endif

#if MIP_USE_SYS || MIP_USE_CMD

/**
* Process table data
*/
static int mip4_tk_proc_table_data(struct mip4_tk_info *info, u8 size, u8 data_type_size, u8 data_type_sign, u8 buf_addr_h, u8 buf_addr_l, u8 row_num, u8 col_num, u8 buf_col_num, u8 rotate, u8 key_num)
{
	char data[10];
	int i_col, i_row;
	int i_x, i_y;
	int lim_x, lim_y;
	int lim_col, lim_row;
	int max_x = 0;
	int max_y = 0;
	bool flip_x = false;
	int sValue = 0;
	unsigned int uValue = 0;
	int value = 0;
	u8 wbuf[8];
	u8 rbuf[512];
	unsigned int buf_addr;
	int offset;
	int data_size = data_type_size;
	int data_sign = data_type_sign;
	int has_key = 0;
	int size_screen = col_num * row_num;

	memset(data, 0, 10);

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	//set axis
	if (rotate == 0) {
		max_x = col_num;
		max_y = row_num;
		if (key_num > 0) {
			max_y += 1;
			has_key = 1;
		}
		flip_x = false;
	} else if (rotate == 1) {
		max_x = row_num;
		max_y = col_num;
		if (key_num > 0) {
			max_y += 1;
			has_key = 1;
		}
		flip_x = true;
	} else {
		input_err(true, &info->client->dev, "%s [ERROR] rotate [%d]\n", __func__, rotate);
		goto error;
	}

	//get table data
	lim_row = row_num + has_key;
	for (i_row = 0; i_row < lim_row; i_row++) {
		//get line data
		offset = buf_col_num * data_type_size;
		size = col_num * data_type_size;

		buf_addr = (buf_addr_h << 8) | buf_addr_l | (offset * i_row);
		wbuf[0] = (buf_addr >> 8) & 0xFF;
		wbuf[1] = buf_addr & 0xFF;
		if (mip4_tk_i2c_read(info, wbuf, 2, rbuf, size)) {
			input_err(true, &info->client->dev, "%s [ERROR] Read data buffer\n", __func__);
			goto error;
		}

		//save data
		if ((key_num > 0) && (i_row == (lim_row - 1))) {
			lim_col = key_num;
		} else {
			lim_col = col_num;
		}
		for (i_col = 0; i_col < lim_col; i_col++) {
			if (data_sign == 0) {
				//unsigned
				if (data_size == 1) {
					uValue = (u8)rbuf[i_col];
				} else if (data_size == 2) {
					uValue = (u16)(rbuf[data_size * i_col] | (rbuf[data_size * i_col + 1] << 8));
				} else if (data_size == 4) {
					uValue = (u32)(rbuf[data_size * i_col] | (rbuf[data_size * i_col + 1] << 8) | (rbuf[data_size * i_col + 2] << 16) | (rbuf[data_size * i_col + 3] << 24));
				} else {
					input_err(true, &info->client->dev, "%s [ERROR] data_size [%d]\n", __func__, data_size);
					goto error;
				}
				value = (int)uValue;
			} else {
				//signed
				if (data_size == 1) {
					sValue = (s8)rbuf[i_col];
				} else if (data_size == 2) {
					sValue = (s16)(rbuf[data_size * i_col] | (rbuf[data_size * i_col + 1] << 8));
				} else if (data_size == 4) {
					sValue = (s32)(rbuf[data_size * i_col] | (rbuf[data_size * i_col + 1] << 8) | (rbuf[data_size * i_col + 2] << 16) | (rbuf[data_size * i_col + 3] << 24));
				} else {
					input_err(true, &info->client->dev, "%s [ERROR] data_size [%d]\n", __func__, data_size);
					goto error;
				}
				value = (int)sValue;
			}

 			switch (rotate) {
			case 0:
				info->image_buf[i_row * col_num + i_col] = value;
				break;
			case 1:
				if ((key_num > 0) && (i_row == (lim_row - 1))) {
					info->image_buf[size_screen + i_col] = value;
				} else {
					info->image_buf[i_col * row_num + (row_num - 1 - i_row)] = value;
				}
				break;
			default:
				input_err(true, &info->client->dev, "%s [ERROR] rotate [%d]\n", __func__, rotate);
				goto error;
				break;
			}
		}
	}

	//print table header
	printk("    ");
	sprintf(data, "    ");
	strcat(info->print_buf, data);
	memset(data, 0, 10);

	switch (data_size) {
	case 1:
		for (i_x = 0; i_x < max_x; i_x++) {
			printk("[%2d]", i_x);
			sprintf(data, "[%2d]", i_x);
			strcat(info->print_buf, data);
			memset(data, 0, 10);
		}
		break;
	case 2:
		for (i_x = 0; i_x < max_x; i_x++) {
			printk("[%4d]", i_x);
			sprintf(data, "[%4d]", i_x);
			strcat(info->print_buf, data);
			memset(data, 0, 10);
		}
		break;
	case 4:
		for (i_x = 0; i_x < max_x; i_x++) {
			printk("[%5d]", i_x);
			sprintf(data, "[%5d]", i_x);
			strcat(info->print_buf, data);
			memset(data, 0, 10);
		}
		break;
	default:
		input_err(true, &info->client->dev, "%s [ERROR] data_size [%d]\n", __func__, data_size);
		goto error;
		break;
	}

	printk("\n");
	sprintf(data, "\n");
	strcat(info->print_buf, data);
	memset(data, 0, 10);

	//print table
	lim_y = max_y;
	for (i_y = 0; i_y < lim_y; i_y++) {
		//print line header
		if ((key_num > 0) && (i_y == (lim_y -1))) {
			printk("[TK]");
			sprintf(data, "[TK]");
		} else {
			printk("[%2d]", i_y);
			sprintf(data, "[%2d]", i_y);
		}
		strcat(info->print_buf, data);
		memset(data, 0, 10);

		//print line
		if ((key_num > 0) && (i_y == (lim_y - 1))) {
			lim_x = key_num;
		} else {
			lim_x = max_x;
		}
		for (i_x = 0; i_x < lim_x; i_x++) {
			switch (data_size) {
			case 1:
				printk(" %3d", info->image_buf[i_y * max_x + i_x]);
				sprintf(data, " %3d", info->image_buf[i_y * max_x + i_x]);
				break;
			case 2:
				printk(" %5d", info->image_buf[i_y * max_x + i_x]);
				sprintf(data, " %5d", info->image_buf[i_y * max_x + i_x]);
				break;
			case 4:
				printk(" %6d", info->image_buf[i_y * max_x + i_x]);
				sprintf(data, " %6d", info->image_buf[i_y * max_x + i_x]);
				break;
			default:
				input_err(true, &info->client->dev, "%s [ERROR] data_size [%d]\n", __func__, data_size);
				goto error;
				break;
			}

			strcat(info->print_buf, data);
			memset(data, 0, 10);
		}

		printk("\n");
		sprintf(data, "\n");
		strcat(info->print_buf, data);
		memset(data, 0, 10);
	}

	printk("\n");
	sprintf(data, "\n");
	strcat(info->print_buf, data);
	memset(data, 0, 10);

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/**
* Run test
*/
int mip4_tk_run_test(struct mip4_tk_info *info, u8 test_type)
{
	int busy_cnt = 50;
	int wait_cnt = 100;
	u8 wbuf[8];
	u8 rbuf[512];
	u8 size = 0;
	u8 row_num;
	u8 col_num;
	u8 buffer_col_num;
	u8 rotate;
	u8 key_num;
	u8 data_type;
	u8 data_type_size;
	u8 data_type_sign;
	u8 buf_addr_h;
	u8 buf_addr_l;
	int ret = 0;

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);
	input_dbg(true, &info->client->dev, "%s - test_type[%d]\n", __func__, test_type);

	while (busy_cnt--) {
		if (info->test_busy == false) {
			break;
		}
		msleep(10);
	}
	mutex_lock(&info->lock);
	info->test_busy = true;
	mutex_unlock(&info->lock);

	memset(info->print_buf, 0, PAGE_SIZE);

	//disable touch event
	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_EVENT_TRIGGER_TYPE;
	wbuf[2] = MIP_CTRL_TRIGGER_NONE;
	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] Disable event\n", __func__);
		goto error;
	}

	//check test type
	switch (test_type) {
	case MIP_TEST_TYPE_CP:
		//printk("=== Cp Test ===\n");
		sprintf(info->print_buf, "\n=== Cp Test ===\n\n");
		break;
	default:
		input_err(true, &info->client->dev, "%s [ERROR] Unknown test type\n", __func__);
		sprintf(info->print_buf, "\nERROR : Unknown test type\n\n");
		goto error;
		break;
	}

	//set test mode
	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_MODE;
	wbuf[2] = MIP_CTRL_MODE_TEST;
	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] Write test mode\n", __func__);
		goto error;
	}

	//wait ready status
	wait_cnt = 100;
	while (wait_cnt--) {
		if (mip4_tk_get_ready_status(info) == MIP_CTRL_STATUS_READY) {
			break;
		}
		msleep(10);

		input_dbg(true, &info->client->dev, "%s - wait [%d]\n", __func__, wait_cnt);
	}

	if (wait_cnt <= 0) {
		input_err(true, &info->client->dev, "%s [ERROR] Wait timeout\n", __func__);
		goto error;
	}

	input_dbg(true, &info->client->dev, "%s - set control mode\n", __func__);

	//set test type
	wbuf[0] = MIP_R0_TEST;
	wbuf[1] = MIP_R1_TEST_TYPE;
	wbuf[2] = test_type;
	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] Write test type\n", __func__);
		goto error;
	}

	input_dbg(true, &info->client->dev, "%s - set test type\n", __func__);

	//wait ready status
	wait_cnt = 100;
	while (wait_cnt--) {
		if (mip4_tk_get_ready_status(info) == MIP_CTRL_STATUS_READY) {
			break;
		}
		msleep(10);

		input_dbg(true, &info->client->dev, "%s - wait [%d]\n", __func__, wait_cnt);
	}

	if (wait_cnt <= 0) {
		input_err(true, &info->client->dev, "%s [ERROR] Wait timeout\n", __func__);
		goto error;
	}

	input_dbg(true, &info->client->dev, "%s - ready\n", __func__);

	//data format
	wbuf[0] = MIP_R0_TEST;
	wbuf[1] = MIP_R1_TEST_DATA_FORMAT;
	if (mip4_tk_i2c_read(info, wbuf, 2, rbuf, 6)) {
		input_err(true, &info->client->dev, "%s [ERROR] Read data format\n", __func__);
		goto error;
	}
	row_num = rbuf[0];
	col_num = rbuf[1];
	buffer_col_num = rbuf[2];
	rotate = rbuf[3];
	key_num = rbuf[4];
	data_type = rbuf[5];

	data_type_sign = (data_type & 0x80) >> 7;
	data_type_size = data_type & 0x7F;

	input_dbg(true, &info->client->dev, "%s - row_num[%d] col_num[%d] buffer_col_num[%d] rotate[%d] key_num[%d]\n", __func__, row_num, col_num, buffer_col_num, rotate, key_num);
	input_dbg(true, &info->client->dev, "%s - data_type[0x%02X] data_sign[%d] data_size[%d]\n", __func__, data_type, data_type_sign, data_type_size);

	//get buf addr
	wbuf[0] = MIP_R0_TEST;
	wbuf[1] = MIP_R1_TEST_BUF_ADDR;
	if (mip4_tk_i2c_read(info, wbuf, 2, rbuf, 2)) {
		input_err(true, &info->client->dev, "%s [ERROR] Read buf addr\n", __func__);
		goto error;
	}

	buf_addr_l = rbuf[0];
	buf_addr_h = rbuf[1];
	input_dbg(true, &info->client->dev, "%s - buf_addr[0x%02X 0x%02X]\n", __func__, buf_addr_h, buf_addr_l);

	//print data
	if (mip4_tk_proc_table_data(info, size, data_type_size, data_type_sign, buf_addr_h, buf_addr_l, row_num, col_num, buffer_col_num, rotate, key_num)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_proc_table_data\n", __func__);
		goto error;
	}

	//set normal mode
	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_MODE;
	wbuf[2] = MIP_CTRL_MODE_NORMAL;
	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_write\n", __func__);
		goto error;
	}

	//wait ready status
	wait_cnt = 100;
	while (wait_cnt--) {
		if (mip4_tk_get_ready_status(info) == MIP_CTRL_STATUS_READY) {
			break;
		}
		msleep(10);

		input_dbg(true, &info->client->dev, "%s - wait [%d]\n", __func__, wait_cnt);
	}

	if (wait_cnt <= 0) {
		input_err(true, &info->client->dev, "%s [ERROR] Wait timeout\n", __func__);
		goto error;
	}

	input_dbg(true, &info->client->dev, "%s - set normal mode\n", __func__);

	goto exit;

error:
	ret = 1;

exit:
	//enable touch event
	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_EVENT_TRIGGER_TYPE;
	wbuf[2] = MIP_CTRL_TRIGGER_INTR;
	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] Enable event\n", __func__);
	}

	mip4_tk_reboot(info);

	mutex_lock(&info->lock);
	info->test_busy = false;
	mutex_unlock(&info->lock);

	if (!ret) {
		input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	} else {
		input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	}

	return ret;
}

/**
* Read image data
*/
int mip4_tk_get_image(struct mip4_tk_info *info, u8 image_type)
{
	int busy_cnt = 100;
	int wait_cnt = 100;
	u8 wbuf[8];
	u8 rbuf[512];
	u8 size = 0;
	u8 row_num;
	u8 col_num;
	u8 buffer_col_num;
	u8 rotate;
	u8 key_num;
	u8 data_type;
	u8 data_type_size;
	u8 data_type_sign;
	u8 buf_addr_h;
	u8 buf_addr_l;
	int ret = 0;

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);
	input_dbg(true, &info->client->dev, "%s - image_type[%d]\n", __func__, image_type);

	while (busy_cnt--) {
		if (info->test_busy == false) {
			break;
		}
		input_dbg(true, &info->client->dev, "%s - busy_cnt[%d]\n", __func__, busy_cnt);
		msleep(1);
	}

	mutex_lock(&info->lock);
	info->test_busy = true;
	mutex_unlock(&info->lock);

	memset(info->print_buf, 0, PAGE_SIZE);

	//disable touch event
/*	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_EVENT_TRIGGER_TYPE;
	wbuf[2] = MIP_CTRL_TRIGGER_REG;
	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] Disable event\n", __func__);
		goto error;
	}
*/
	//check image type
	switch (image_type) {
	case MIP_IMG_TYPE_INTENSITY:
		input_dbg(true, &info->client->dev, "=== Intensity Image ===\n");
		sprintf(info->print_buf, "\n=== Intensity Image ===\n\n");
		break;
	case MIP_IMG_TYPE_RAWDATA:
		input_dbg(true, &info->client->dev, "=== Rawdata Image ===\n");
		sprintf(info->print_buf, "\n=== Rawdata Image ===\n\n");
		break;
	case MIP_IMG_TYPE_SELF_RAWDATA:
		input_dbg(true, &info->client->dev, "=== Self Rawdata Image ===\n");
		sprintf(info->print_buf, "\n=== Self Rawdata Image ===\n\n");
		break;
	case MIP_IMG_TYPE_SELF_INTENSITY:
		input_dbg(true, &info->client->dev, "=== Self Intensity Image ===\n");
		sprintf(info->print_buf, "\n=== Self Intensity Image ===\n\n");
		break;
	default:
		input_err(true, &info->client->dev, "%s [ERROR] Unknown image type\n", __func__);
		sprintf(info->print_buf, "\nERROR : Unknown image type\n\n");
		goto error;
		break;
	}

	//set image type
	wbuf[0] = MIP_R0_IMAGE;
	wbuf[1] = MIP_R1_IMAGE_TYPE;
	wbuf[2] = image_type;
	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] Write image type\n", __func__);
		goto error;
	}

	input_dbg(true, &info->client->dev, "%s - set image type\n", __func__);

	//wait ready status
	wait_cnt = 100;
	while (wait_cnt--) {
		if (mip4_tk_get_ready_status(info) == MIP_CTRL_STATUS_READY) {
			break;
		}
		msleep(10);
		input_dbg(true, &info->client->dev, "%s - wait [%d]\n", __func__, wait_cnt);
	}

	if (wait_cnt <= 0) {
		input_err(true, &info->client->dev, "%s [ERROR] Wait timeout\n", __func__);
		goto error;
	}
	input_dbg(true, &info->client->dev, "%s - ready\n", __func__);

	//data format
	wbuf[0] = MIP_R0_IMAGE;
	wbuf[1] = MIP_R1_IMAGE_DATA_FORMAT;
	if (mip4_tk_i2c_read(info, wbuf, 2, rbuf, 6)) {
		input_err(true, &info->client->dev, "%s [ERROR] Read data format\n", __func__);
		goto error;
	}
	row_num = rbuf[0];
	col_num = rbuf[1];
	buffer_col_num = rbuf[2];
	rotate = rbuf[3];
	key_num = rbuf[4];
	data_type = rbuf[5];

	data_type_sign = (data_type & 0x80) >> 7;
	data_type_size = data_type & 0x7F;

	input_dbg(true, &info->client->dev, "%s - row_num[%d] col_num[%d] buffer_col_num[%d] rotate[%d] key_num[%d]\n", __func__, row_num, col_num, buffer_col_num, rotate, key_num);
	input_dbg(true, &info->client->dev, "%s - data_type[0x%02X] data_sign[%d] data_size[%d]\n", __func__, data_type, data_type_sign, data_type_size);

	//get buf addr
	wbuf[0] = MIP_R0_IMAGE;
	wbuf[1] = MIP_R1_IMAGE_BUF_ADDR;
	if (mip4_tk_i2c_read(info, wbuf, 2, rbuf, 2)) {
		input_err(true, &info->client->dev, "%s [ERROR] Read buf addr\n", __func__);
		goto error;
	}

	buf_addr_l = rbuf[0];
	buf_addr_h = rbuf[1];
	input_dbg(true, &info->client->dev, "%s - buf_addr[0x%02X 0x%02X]\n", __func__, buf_addr_h, buf_addr_l);

	//print data
	if (mip4_tk_proc_table_data(info, size, data_type_size, data_type_sign, buf_addr_h, buf_addr_l, row_num, col_num, buffer_col_num, rotate, key_num)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_proc_table_data\n", __func__);
		goto error;
	}

	//clear image type
	wbuf[0] = MIP_R0_IMAGE;
	wbuf[1] = MIP_R1_IMAGE_TYPE;
	wbuf[2] = MIP_IMG_TYPE_NONE;
	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] Clear image type\n", __func__);
		goto error;
	}

	goto exit;

error:
	ret = 1;

exit:
	//enable touch event
/*	wbuf[0] = MIP_R0_CTRL;
	wbuf[1] = MIP_R1_CTRL_EVENT_TRIGGER_TYPE;
	wbuf[2] = MIP_CTRL_TRIGGER_INTR;
	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] Enable event\n", __func__);
	}
*/
	mutex_lock(&info->lock);
	info->test_busy = false;
	mutex_unlock(&info->lock);

	if (!ret) {
		input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	} else {
		input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	}

	return ret;
}

#endif

#if MIP_USE_SYS
/**
* Print chip firmware version
*/
static ssize_t mip4_tk_sys_fw_version(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 rbuf[16];

	memset(info->print_buf, 0, PAGE_SIZE);

	if (mip4_tk_get_fw_version(info, rbuf)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_get_fw_version\n", __func__);

		sprintf(data, "F/W Version : ERROR\n");
		goto error;
	}

	input_info(true, &info->client->dev, "%s - F/W Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n", __func__, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7]);
	sprintf(data, "F/W Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7]);

error:
	strcat(info->print_buf, data);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;
}

/**
* Set firmware path
*/
static ssize_t mip4_tk_sys_fw_path_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	char *path;

	input_dbg(true, &info->client->dev, "%s [START] \n", __func__);

	if (count <= 1) {
		input_err(true, &info->client->dev, "%s [ERROR] Wrong value [%s]\n", __func__, buf);
		goto error;
	}

	if (info->fw_path_ext)
		kfree(info->fw_path_ext);

	path = kzalloc(count - 1, GFP_KERNEL);
	memcpy(path, buf, count - 1);

	info->fw_path_ext = kstrdup(path, GFP_KERNEL);

	kfree(path);

	input_dbg(true, &info->client->dev, "%s - Path : %s\n", __func__, info->fw_path_ext);

	input_dbg(true, &info->client->dev, "%s [DONE] \n", __func__);
	return count;

error:
	input_err(true, &info->client->dev, "%s [ERROR] \n", __func__);
	return count;
}

/**
* Print firmware path
*/
static ssize_t mip4_tk_sys_fw_path_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;

	memset(info->print_buf, 0, PAGE_SIZE);

	input_dbg(true, &info->client->dev, "%s [START] \n", __func__);

	sprintf(data, "Path : %s\n", info->fw_path_ext);

	input_dbg(true, &info->client->dev, "%s [DONE] \n", __func__);

	strcat(info->print_buf, data);
	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;
}

/**
* Print bin(file) firmware version
*/
static ssize_t mip4_tk_sys_bin_version(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 rbuf[16];

	if (mip4_tk_get_fw_version_from_bin(info, rbuf)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip_get_fw_version_from_bin\n", __func__);
		sprintf(data, "BIN Version : ERROR\n");
		goto error;
	}

	input_info(true, &info->client->dev, "%s - BIN Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n", __func__, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7]);
	sprintf(data, "BIN Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7]);

error:
	ret = snprintf(buf, 255, "%s\n", data);
	return ret;
}

/**
* Print channel info
*/
static ssize_t mip4_tk_sys_info(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 rbuf[16];
	u8 wbuf[2];

	memset(info->print_buf, 0, PAGE_SIZE);

	sprintf(data, "\n");
	strcat(info->print_buf, data);

	sprintf(data, "Device Name : %s\n", MIP_DEV_NAME);
	strcat(info->print_buf, data);

	sprintf(data, "Chip Model : %s\n", CHIP_NAME);
	strcat(info->print_buf, data);

	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_PRODUCT_NAME;
	mip4_tk_i2c_read(info, wbuf, 2, rbuf, 16);
	sprintf(data, "Product Name : %s\n", rbuf);
	strcat(info->print_buf, data);

	mip4_tk_get_fw_version(info, rbuf);
	sprintf(data, "F/W Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7]);
	strcat(info->print_buf, data);

	wbuf[0] = MIP_R0_INFO;
	wbuf[1] = MIP_R1_INFO_KEY_NUM;
	mip4_tk_i2c_read(info, wbuf, 2, rbuf, 1);
	sprintf(data, "Key Num : %d\n", rbuf[0]);
	strcat(info->print_buf, data);

	wbuf[0] = MIP_R0_LED;
	wbuf[1] = MIP_R1_LED_NUM;
	mip4_tk_i2c_read(info, wbuf, 2, rbuf, 2);
	sprintf(data, "LED Num : %d\n", rbuf[0]);
	strcat(info->print_buf, data);
	sprintf(data, "LED Max Brightness : %d\n", rbuf[1]);
	strcat(info->print_buf, data);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;
}

#ifdef DEBUG
/**
* Power on
*/
static ssize_t mip4_tk_sys_power_on(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	u8 data[255];
	int ret;

	memset(info->print_buf,0,PAGE_SIZE);

	mip4_tk_power_on(info);

	input_info(true, &client->dev, "%s", __func__);

	sprintf(data, "Power : On\n");
	strcat(info->print_buf,data);

	ret = snprintf(buf,PAGE_SIZE,"%s\n",info->print_buf);
	return ret;

}

/**
* Power off
*/
static ssize_t mip4_tk_sys_power_off(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	u8 data[255];
	int ret;

	memset(info->print_buf,0,PAGE_SIZE);

	mip4_tk_power_off(info);

	input_info(true, &client->dev, "%s", __func__);

	sprintf(data, "Power : Off\n");
	strcat(info->print_buf,data);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;

}

/**
* Reboot chip
*/
static ssize_t mip4_tk_sys_reboot(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	u8 data[255];
	int ret;

	memset(info->print_buf, 0, PAGE_SIZE);

	input_info(true, &client->dev, "%s", __func__);

	disable_irq(info->irq);
	mip4_tk_clear_input(info);
	mip4_tk_reboot(info);
	enable_irq(info->irq);

	sprintf(data, "Reboot\n");
	strcat(info->print_buf,data);

	ret = snprintf(buf,PAGE_SIZE,"%s\n",info->print_buf);
	return ret;
}
#endif

/**
* Set mode
*/
static ssize_t mip4_tk_sys_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 wbuf[8];
	u8 value = 0;

	input_dbg(true, &info->client->dev, "%s [START] \n", __func__);

	wbuf[0] = MIP_R0_CTRL;

	if (!strcmp(attr->attr.name, "mode_glove")) {
		wbuf[1] = MIP_R1_CTRL_HIGH_SENS_MODE;
	} else if (!strcmp(attr->attr.name, "mode_charger")) {
		wbuf[1] = MIP_R1_CTRL_CHARGER_MODE;
	} else if (!strcmp(attr->attr.name, "mode_power")) {
		wbuf[1] = MIP_R1_CTRL_POWER_STATE;
	} else {
		input_err(true, &info->client->dev, "%s [ERROR] Unknown mode [%s]\n", __func__, attr->attr.name);
		goto error;
	}

	if (buf[0] == 48) {
		value = 0;
	} else if (buf[0] == 49) {
		value = 1;
	} else {
		input_err(true, &info->client->dev, "%s [ERROR] Unknown value [%c]\n", __func__, buf[0]);
		goto exit;
	}
	wbuf[2] = value;

	if (mip4_tk_i2c_write(info, wbuf, 3)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_write\n", __func__);
	} else {
		input_info(true, &info->client->dev, "%s - addr[0x%02X%02X] value[%d]\n", __func__, wbuf[0], wbuf[1], value);
	}

exit:
	input_dbg(true, &info->client->dev, "%s [DONE] \n", __func__);
	return count;

error:
	input_err(true, &info->client->dev, "%s [ERROR] \n", __func__);
	return count;
}

/**
* Print mode
*/
static ssize_t mip4_tk_sys_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 wbuf[8];
	u8 rbuf[4];

	memset(info->print_buf, 0, PAGE_SIZE);

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	wbuf[0] = MIP_R0_CTRL;

	if (!strcmp(attr->attr.name, "mode_glove")) {
		wbuf[1] = MIP_R1_CTRL_HIGH_SENS_MODE;
	} else if (!strcmp(attr->attr.name, "mode_charger")) {
		wbuf[1] = MIP_R1_CTRL_CHARGER_MODE;
	} else if (!strcmp(attr->attr.name, "mode_power")) {
		wbuf[1] = MIP_R1_CTRL_POWER_STATE;
	} else {
		input_err(true, &info->client->dev, "%s [ERROR] Unknown mode [%s]\n", __func__, attr->attr.name);
		sprintf(data, "%s : Unknown Mode\n", attr->attr.name);
		goto exit;
	}

	if (mip4_tk_i2c_read(info, wbuf, 2, rbuf, 1)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_read\n", __func__);
		sprintf(data, "%s : ERROR\n", attr->attr.name);
	} else {
		input_info(true, &info->client->dev, "%s - addr[0x%02X%02X] value[%d]\n", __func__, wbuf[0], wbuf[1], rbuf[0]);
		sprintf(data, "%s : %d\n", attr->attr.name, rbuf[0]);
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

exit:
	strcat(info->print_buf, data);
	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;
}

/**
* Store LED on/off
*/
static ssize_t mip4_tk_sys_led_onoff_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 wbuf[6];
	const char delimiters[] = ",\n";
	char string[count];
	char *stringp = string;
	char *token;
	int i, value, idx, bit;
	u8 values[4] = {0, };

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	if (info->led_num <= 0) {
		input_dbg(true, &info->client->dev, "%s - N/A\n", __func__);
		goto exit;
	}

	memset(string, 0x00, count);
	memcpy(string, buf, count);

	//Input format: "LED #0,LED #1,LED #2,..."
	//	Number of LEDs should be matched.

	for (i = 0; i < info->led_num; i++) {
		token = strsep(&stringp, delimiters);
		if (token == NULL) {
			input_err(true, &info->client->dev, "%s [ERROR] LED number mismatch\n", __func__);
			goto error;
		} else {
			if (kstrtoint(token, 10, &value)) {
				input_err(true, &info->client->dev, "%s [ERROR] wrong input value [%s]\n", __func__, token);
				goto error;
			}

			idx = i / 8;
			bit = i % 8;
			if (value == 1) {
				values[idx] |= (1 << bit);
			} else {
				values[idx] &= ~(1 << bit);
			}
		}
	}

	wbuf[0] = MIP_R0_LED;
	wbuf[1] = MIP_R1_LED_ON;
	wbuf[2] = values[0];
	wbuf[3] = values[1];
	wbuf[4] = values[2];
	wbuf[5] = values[3];
	if (mip4_tk_i2c_write(info, wbuf, 6)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_write\n", __func__);
		goto error;
	}
	input_dbg(true, &info->client->dev, "%s - wbuf 0x%02X 0x%02X 0x%02X 0x%02X\n", __func__, wbuf[2], wbuf[3], wbuf[4], wbuf[5]);

exit:
	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return count;

error:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return count;
}

/**
* Show LED on/off
*/
static ssize_t mip4_tk_sys_led_onoff_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 wbuf[2];
	u8 rbuf[4];
	int i, idx, bit;

	memset(info->print_buf, 0, PAGE_SIZE);

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	if (info->led_num <= 0) {
		sprintf(data, "LED On/Off : N/A\n");
		goto exit;
	}

	wbuf[0] = MIP_R0_LED;
	wbuf[1] = MIP_R1_LED_ON;

	if (mip4_tk_i2c_read(info, wbuf, 2, rbuf, 4)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_read\n", __func__);
		sprintf(data, "LED On/Off : Error\n");
	} else {
		for (i = 0; i < info->led_num; i++) {
			idx = i / 8;
			bit = i % 8;
			if (i == 0) {
				sprintf(data, "LED On/Off : %d", ((rbuf[idx] >> bit) & 0x01));
			} else {
				sprintf(data, ",%d", ((rbuf[idx] >> bit) & 0x01));
			}
			strcat(info->print_buf, data);
		}
	}
	sprintf(data, "\n");

exit:
	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	strcat(info->print_buf, data);
	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;
}

/**
* Store LED brightness
*/
static ssize_t mip4_tk_sys_led_brightness_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 wbuf[2 + MAX_LED_NUM];
	const char delimiters[] = ",\n";
	char string[count];
	char *stringp = string;
	char *token;
	int value, i;
	u8 values[MAX_LED_NUM];

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	if (info->led_num <= 0) {
		input_dbg(true, &info->client->dev, "%s - N/A\n", __func__);
		goto exit;
	}

	memset(string, 0x00, count);
	memcpy(string, buf, count);

	//Input format: "LED #0,LED #1,LED #2,..."
	//	Number of LEDs should be matched.

	for (i = 0; i < info->led_num; i++) {
		token = strsep(&stringp, delimiters);
		if (token == NULL) {
			input_err(true, &info->client->dev, "%s [ERROR] LED number mismatch\n", __func__);
			goto error;
		} else {
			if (kstrtoint(token, 10, &value)) {
				input_err(true, &info->client->dev, "%s [ERROR] wrong input value [%s]\n", __func__, token);
				goto error;
			}
			values[i] = (u8)value;
		}
	}

	wbuf[0] = MIP_R0_LED;
	wbuf[1] = MIP_R1_LED_BRIGHTNESS;
	memcpy(&wbuf[2], values, info->led_num);
	if (mip4_tk_i2c_write(info, wbuf, (2 + info->led_num))) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_write\n", __func__);
		goto error;
	}

exit:
	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);
	return count;

error:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return count;
}

/**
* Show LED brightness
*/
static ssize_t mip4_tk_sys_led_brightness_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret, i;
	u8 wbuf[2];
	u8 rbuf[MAX_LED_NUM];

	memset(info->print_buf, 0, PAGE_SIZE);

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	if (info->led_num <= 0) {
		sprintf(data, "LED brightness : N/A\n");
		goto exit;
	}

	wbuf[0] = MIP_R0_LED;
	wbuf[1] = MIP_R1_LED_BRIGHTNESS;
	if (mip4_tk_i2c_read(info, wbuf, 2, rbuf, info->led_num)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_read\n", __func__);
		sprintf(data, "LED brightness : ERROR\n");
	} else {
		for (i = 0; i < info->led_num; i++) {
			if (i == 0) {
				sprintf(data, "LED brightness : %d", rbuf[i]);
			} else {
				sprintf(data, ",%d", rbuf[i]);
			}
			strcat(info->print_buf, data);
		}
		sprintf(data, "\n");
	}

exit:
	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	strcat(info->print_buf, data);
	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;
}

/**
* Show LED max brightness
*/
static ssize_t mip4_tk_sys_led_max_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	u8 data[255];
	int ret;
	u8 wbuf[2];
	u8 rbuf[2];

	memset(info->print_buf, 0, PAGE_SIZE);

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	wbuf[0] = MIP_R0_LED;
	wbuf[1] = MIP_R1_LED_MAX_BRIGHTNESS;
	if (mip4_tk_i2c_read(info, wbuf, 2, rbuf, 1)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_i2c_read\n", __func__);
		sprintf(data, "LED max brightness : ERROR\n");
	} else {
		sprintf(data, "LED max brightness : %d\n", rbuf[0]);
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	strcat(info->print_buf, data);
	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;
}

/**
* Sysfs print image
*/
static ssize_t mip4_tk_sys_image(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	int ret;
	u8 type;

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	if (!strcmp(attr->attr.name, "image_intensity")) {
		type = MIP_IMG_TYPE_INTENSITY;
	} else if (!strcmp(attr->attr.name, "image_rawdata")) {
		type = MIP_IMG_TYPE_RAWDATA;
	} else {
		input_err(true, &info->client->dev, "%s [ERROR] Unknown image [%s]\n", __func__, attr->attr.name);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR : Unknown image type");
		goto error;
	}

	if (mip4_tk_get_image(info, type)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_get_image\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR");
		goto error;
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;

error:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/**
* Sysfs run test
*/
static ssize_t __maybe_unused mip4_tk_sys_test(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mip4_tk_info *info = dev_get_drvdata(dev);
	int ret;
	u8 test_type;

	input_dbg(true, &info->client->dev, "%s [START] \n", __func__);

	if (!strcmp(attr->attr.name, "test_cp")) {
		test_type = MIP_TEST_TYPE_CP;
	} else if (!strcmp(attr->attr.name, "test_cp_jitter")) {
		test_type = MIP_TEST_TYPE_CP_JITTER;
	} else {
		input_err(true, &info->client->dev, "%s [ERROR] Unknown test [%s]\n", __func__, attr->attr.name);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR : Unknown test type");
		goto error;
	}

	if (mip4_tk_run_test(info, test_type)) {
		input_err(true, &info->client->dev, "%s [ERROR] mip4_tk_run_test\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", "ERROR");
		goto error;
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	ret = snprintf(buf, PAGE_SIZE, "%s\n", info->print_buf);
	return ret;

error:
	input_err(true, &info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/**
* Sysfs functions
*/
static DEVICE_ATTR(fw_version, S_IRUGO, mip4_tk_sys_fw_version, NULL);
static DEVICE_ATTR(fw_version_bin, S_IRUGO, mip4_tk_sys_bin_version, NULL);
static DEVICE_ATTR(fw_path, S_IRUGO | S_IWUSR | S_IWGRP, mip4_tk_sys_fw_path_show, mip4_tk_sys_fw_path_store);
static DEVICE_ATTR(info, S_IRUGO, mip4_tk_sys_info, NULL);
static DEVICE_ATTR(mode_glove, S_IRUGO | S_IWUSR | S_IWGRP, mip4_tk_sys_mode_show, mip4_tk_sys_mode_store);
static DEVICE_ATTR(mode_charger, S_IRUGO | S_IWUSR | S_IWGRP, mip4_tk_sys_mode_show, mip4_tk_sys_mode_store);
static DEVICE_ATTR(mode_power, S_IRUGO | S_IWUSR | S_IWGRP, mip4_tk_sys_mode_show, mip4_tk_sys_mode_store);
static DEVICE_ATTR(led_onoff, S_IRUGO | S_IWUSR | S_IWGRP, mip4_tk_sys_led_onoff_show, mip4_tk_sys_led_onoff_store);
static DEVICE_ATTR(led_brightness, S_IRUGO | S_IWUSR | S_IWGRP, mip4_tk_sys_led_brightness_show, mip4_tk_sys_led_brightness_store);
static DEVICE_ATTR(led_max_brightness, S_IRUGO, mip4_tk_sys_led_max_brightness, NULL);
static DEVICE_ATTR(image_intensity, S_IRUGO, mip4_tk_sys_image, NULL);
static DEVICE_ATTR(image_rawdata, S_IRUGO, mip4_tk_sys_image, NULL);
static DEVICE_ATTR(test_cp, S_IRUGO, mip4_tk_sys_test, NULL);
static DEVICE_ATTR(test_cp_jitter, S_IRUGO, mip4_tk_sys_test, NULL);
#ifdef DEBUG
static DEVICE_ATTR(power_on, S_IRUGO, mip4_tk_sys_power_on, NULL);
static DEVICE_ATTR(power_off, S_IRUGO, mip4_tk_sys_power_off, NULL);
static DEVICE_ATTR(reboot, S_IRUGO, mip4_tk_sys_reboot, NULL);
#endif

/**
* Sysfs attr list info
*/
static struct attribute *mip4_tk_sys_attr[] = {
	&dev_attr_fw_version.attr,
	&dev_attr_fw_version_bin.attr,
	&dev_attr_fw_path.attr,
	&dev_attr_info.attr,
	&dev_attr_mode_glove.attr,
	&dev_attr_mode_charger.attr,
	&dev_attr_mode_power.attr,
	&dev_attr_led_onoff.attr,
	&dev_attr_led_brightness.attr,
	&dev_attr_led_max_brightness.attr,
	&dev_attr_image_intensity.attr,
	&dev_attr_image_rawdata.attr,
	&dev_attr_test_cp.attr,
	&dev_attr_test_cp_jitter.attr,
#ifdef DEBUG
	&dev_attr_power_on.attr,
	&dev_attr_power_off.attr,
	&dev_attr_reboot.attr,
#endif
	NULL,
};

/**
* Sysfs attr group info
*/
static const struct attribute_group mip4_tk_sys_attr_group = {
	.attrs = mip4_tk_sys_attr,
};

/**
* Create sysfs test functions
*/
int mip4_tk_sysfs_create(struct mip4_tk_info *info)
{
	struct i2c_client *client = info->client;
	int ret;

	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	ret = sysfs_create_group(&client->dev.kobj, &mip4_tk_sys_attr_group);
	if (ret) {
		input_err(true, &client->dev,
			"%s [ERROR] sysfs_create_group\n", __func__);
		return ret;
	}

	if (info->print_buf == NULL) {
		info->print_buf = kzalloc(sizeof(u8) * PAGE_SIZE, GFP_KERNEL);
		if (!info->print_buf) {
			input_err(true, &client->dev,
				"%s [ERROR] kzalloc for print buf\n",
				__func__);
			ret = -ENOMEM;
			goto exti_print_buf;
		}
	}

	if (info->image_buf == NULL) {
		info->image_buf = kzalloc(sizeof(int) * PAGE_SIZE, GFP_KERNEL);
		if (!info->image_buf) {
			input_err(true, &client->dev,
				"%s [ERROR] kzalloc for image buf\n",
				__func__);
			ret = -ENOMEM;
			goto exit_image_buf;
		}
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	return 0;

exti_print_buf:
	sysfs_remove_group(&info->client->dev.kobj, &mip4_tk_sys_attr_group);
exit_image_buf:
	if (info->print_buf) {
		kfree(info->print_buf);
		info->print_buf = NULL;
	}

	return ret;
}

/**
* Remove sysfs test functions
*/
void mip4_tk_sysfs_remove(struct mip4_tk_info *info)
{
	input_dbg(true, &info->client->dev, "%s [START]\n", __func__);

	sysfs_remove_group(&info->client->dev.kobj, &mip4_tk_sys_attr_group);

	if (info->image_buf) {
		kfree(info->image_buf);
		info->image_buf = NULL;
	}

	if (info->print_buf) {
		kfree(info->print_buf);
		info->print_buf = NULL;
	}

	input_dbg(true, &info->client->dev, "%s [DONE]\n", __func__);

	return;
}

#endif

