/* drivers/input/touchscreen/sec_ts.c
 *
 * Copyright (C) 2011 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

struct sec_ts_data *tsp_info;

#include "sec_ts.h"

struct sec_ts_data *ts_dup;

static void sec_ts_read_info_work(struct work_struct *work);

#ifdef USE_OPEN_CLOSE
static int sec_ts_input_open(struct input_dev *dev);
static void sec_ts_input_close(struct input_dev *dev);
#endif

#ifdef CONFIG_FB
static int sec_ts_fb_notifier_cb(struct notifier_block *nb, unsigned long event,
					void *data);
#endif

int sec_ts_read_information(struct sec_ts_data *ts);

int sec_ts_i2c_write(struct sec_ts_data *ts, u8 reg, u8 *data, int len)
{
	u8 buf[I2C_WRITE_BUFFER_SIZE + 1];
	int ret;
	unsigned char retry;
	struct i2c_msg msg;
	int i;

	if (len > I2C_WRITE_BUFFER_SIZE) {
		input_err(true, &ts->client->dev, "%s: len is larger than buffer size\n", __func__);
		return -EINVAL;
	}

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	buf[0] = reg;
	memcpy(buf + 1, data, len);

	msg.addr = ts->client->addr;
	msg.flags = 0;
	msg.len = len + 1;
	msg.buf = buf;
	mutex_lock(&ts->i2c_mutex);
	for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
		ret = i2c_transfer(ts->client->adapter, &msg, 1);
		if (ret > 0)
			break;

		if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
			input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
			mutex_unlock(&ts->i2c_mutex);
			goto err;
		}

		usleep_range(1 * 1000, 1 * 1000);

		if (retry > 1) {
			input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retry + 1, ret);
			ts->comm_err_count++;
			if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT)
				send_event_to_user(ts, 0, UEVENT_TSP_I2C_ERROR);
		}
	}

	mutex_unlock(&ts->i2c_mutex);

	if (retry == SEC_TS_I2C_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_I2C_CMD) {
		pr_info("sec_input:i2c_cmd: W: %02X | ", reg);
		for (i = 0; i < len; i++)
			pr_cont("%02X ", data[i]);
		pr_cont("\n");
	}

	if (ret == 1)
		return 0;
err:
	return -EIO;
}

int sec_ts_i2c_read(struct sec_ts_data *ts, u8 reg, u8 *data, int len)
{
	u8 buf[4];
	int ret;
	unsigned char retry;
	struct i2c_msg msg[2];
	int remain = len;
	int i;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF\n", __func__);
		goto err;
	}

	buf[0] = reg;

	msg[0].addr = ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = data;

	mutex_lock(&ts->i2c_mutex);

	if (len <= ts->i2c_burstmax) {

		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(ts->client->adapter, msg, 2);
			if (ret > 0)
				break;
			usleep_range(1 * 1000, 1 * 1000);
			if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
				input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->i2c_mutex);
				goto err;
			}

			if (retry > 1) {
				input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n",
					__func__, retry + 1, ret);
				ts->comm_err_count++;
				if (ts->debug_flag & SEC_TS_DEBUG_SEND_UEVENT)
					send_event_to_user(ts, 0, UEVENT_TSP_I2C_ERROR);
			}
		}

	} else {
		/*
		 * I2C read buffer is 256 byte. do not support long buffer over than 256.
		 * So, try to seperate reading data about 256 bytes.
		 */

		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(ts->client->adapter, msg, 1);
			if (ret > 0)
				break;
			usleep_range(1 * 1000, 1 * 1000);
			if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
				input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
				mutex_unlock(&ts->i2c_mutex);
				goto err;
			}

			if (retry > 1) {
				input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retry + 1, ret);
				ts->comm_err_count++;
			}
		}

		do {
			if (remain > ts->i2c_burstmax)
				msg[1].len = ts->i2c_burstmax;
			else
				msg[1].len = remain;

			remain -= ts->i2c_burstmax;

			for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
				ret = i2c_transfer(ts->client->adapter, &msg[1], 1);
				if (ret > 0)
					break;
				usleep_range(1 * 1000, 1 * 1000);
				if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
					input_err(true, &ts->client->dev, "%s: POWER_STATUS : OFF, retry:%d\n", __func__, retry);
					mutex_unlock(&ts->i2c_mutex);
					goto err;
				}

				if (retry > 1) {
					input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retry + 1, ret);
					ts->comm_err_count++;
				}
			}

			msg[1].buf += msg[1].len;

		} while (remain > 0);

	}

	mutex_unlock(&ts->i2c_mutex);
	if (retry == SEC_TS_I2C_RETRY_CNT) {
			input_err(true, &ts->client->dev, "%s: I2C read over retry limit\n", __func__);
			ret = -EIO;

	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_I2C_CMD) {
		pr_info("sec_input:i2c_cmd: R: %02X | ", reg);
		for (i = 0; i < len; i++)
			pr_cont("%02X ", data[i]);
		pr_cont("\n");
	}

	return ret;

err:
	return -EIO;
}

static int sec_ts_i2c_write_burst(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;
	int retry;
	int i;

	mutex_lock(&ts->i2c_mutex);
	for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
		ret = i2c_master_send(ts->client, data, len);
		if (ret == len)
			break;

		usleep_range(1 * 1000, 1 * 1000);

		if (retry > 1) {
			input_err(true, &ts->client->dev, "%s: I2C retry %d\n", __func__, retry + 1);
			ts->comm_err_count++;
		}
	}

	mutex_unlock(&ts->i2c_mutex);
	if (retry == SEC_TS_I2C_RETRY_CNT) {
		input_err(true, &ts->client->dev, "%s: I2C write over retry limit\n", __func__);
		ret = -EIO;
	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_I2C_CMD) {
		pr_info("sec_input:i2c_cmd: W: %02X | ", data[0]);
		for (i = 1; i < len; i++)
			pr_cont("%02X ", data[i]);
		pr_cont("\n");
	}

	return ret;
}

static int sec_ts_i2c_read_bulk(struct sec_ts_data *ts, u8 *data, int len)
{
	int ret;
	unsigned char retry;
	int remain = len;
	int i = 0;
	struct i2c_msg msg;

	msg.addr = ts->client->addr;
	msg.flags = I2C_M_RD;
	msg.len = len;
	msg.buf = data;

	mutex_lock(&ts->i2c_mutex);

	do {
		if (remain > ts->i2c_burstmax)
			msg.len = ts->i2c_burstmax;
		else
			msg.len = remain;

		remain -= ts->i2c_burstmax;

		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			ret = i2c_transfer(ts->client->adapter, &msg, 1);
			if (ret > 0)
				break;
			usleep_range(1 * 1000, 1 * 1000);

			if (retry > 1) {
				input_err(true, &ts->client->dev, "%s: I2C retry %d, ret:%d\n", __func__, retry + 1, ret);
				ts->comm_err_count++;
			}
		}

		if (retry == SEC_TS_I2C_RETRY_CNT) {
			input_err(true, &ts->client->dev, "%s: I2C read over retry limit\n", __func__);
			ret = -EIO;

			break;
		}
		msg.buf += msg.len;

	} while (remain > 0);

	mutex_unlock(&ts->i2c_mutex);

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_I2C_CMD) {
		pr_info("sec_input:i2c_cmd: R: |");
		for (i = 0; i < len; i++)
			pr_cont("%02X ", data[i]);
		pr_cont("\n");
	}

	if (ret > 0)
		return 0;

	return -EIO;
}

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
#include <linux/sec_debug.h>
extern struct tsp_dump_callbacks dump_callbacks;
static struct delayed_work *p_ghost_check;

static void sec_ts_check_rawdata(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data, ghost_check.work);

	if (ts->tsp_dump_lock == 1) {
		input_err(true, &ts->client->dev, "%s: ignored ## already checking..\n", __func__);
		return;
	}
	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: ignored ## IC is power off\n", __func__);
		return;
	}

	sec_ts_run_rawdata_all(ts, true);
}

static void dump_tsp_log(void)
{
	pr_info("%s: %s %s: start\n", SEC_TS_I2C_NAME, SECLOG, __func__);

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		pr_err("%s: %s %s: ignored ## lpm charging Mode!!\n", SEC_TS_I2C_NAME, SECLOG, __func__);
		return;
	}
#endif

	if (p_ghost_check == NULL) {
		pr_err("%s: %s %s: ignored ## tsp probe fail!!\n", SEC_TS_I2C_NAME, SECLOG, __func__);
		return;
	}
	schedule_delayed_work(p_ghost_check, msecs_to_jiffies(100));
}
#endif


void sec_ts_delay(unsigned int ms)
{
	if (ms < 20)
		usleep_range(ms * 1000, ms * 1000);
	else
		msleep(ms);
}

#define DEVICE_ID_RETRY_COUNT 10
int sec_ts_wait_for_ready(struct sec_ts_data *ts, unsigned int ack)
{
	int rc = -1;
	int retry = 0;
	u8 tBuff[SEC_TS_EVENT_BUFF_SIZE] = {0,};

	while (sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, tBuff, SEC_TS_EVENT_BUFF_SIZE)) {
		if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_INFO) {
			if (tBuff[1] == ack) {
				rc = 0;
				break;
			}
		} else if (((tBuff[0] >> 2) & 0xF) == TYPE_STATUS_EVENT_VENDOR_INFO) {
			if (tBuff[1] == ack) {
				rc = 0;
				break;
			}
		}

		if (retry++ > SEC_TS_WAIT_RETRY_CNT) {
			input_err(true, &ts->client->dev, "%s: Time Over\n", __func__);
			break;
		}
		sec_ts_delay(20);
	}

	input_info(true, &ts->client->dev,
		"%s: %02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X [%d]\n",
		__func__, tBuff[0], tBuff[1], tBuff[2], tBuff[3],
		tBuff[4], tBuff[5], tBuff[6], tBuff[7], retry);

	return rc;

}

int sec_ts_read_calibration_report(struct sec_ts_data *ts)
{
	int ret;
	u8 buf[5] = { 0 };

	buf[0] = SEC_TS_READ_CALIBRATION_REPORT;

	ret = sec_ts_i2c_read(ts, buf[0], &buf[1], 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to read, %d\n", __func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev, "%s: count:%d, pass count:%d, fail count:%d, status:0x%X\n",
			__func__, buf[1], buf[2], buf[3], buf[4]);

	return buf[4];
}

void sec_ts_reinit(struct sec_ts_data *ts)
{
	u8 w_data[2] = {0x00, 0x00};
	int ret = 0;

	input_info(true, &ts->client->dev,
			"%s : charger=0x%x, Cover=0x%x\n",
			__func__, ts->charger_mode, ts->touch_functions);

	/* charger mode */
	if (ts->charger_mode != SEC_TS_BIT_CHARGER_MODE_NO) {
		w_data[0] = ts->charger_mode;
		ret = ts->sec_ts_i2c_write(ts, SET_TS_CMD_SET_CHARGER_MODE, (u8 *)&w_data[0], 1);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
					__func__, SET_TS_CMD_SET_CHARGER_MODE);
	}

	/* Cover mode */
	if (ts->touch_functions & SEC_TS_BIT_SETFUNC_COVER) {
		w_data[0] = ts->cover_cmd;
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, (u8 *)&w_data[0], 1);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
					__func__, SEC_TS_CMD_SET_COVERTYPE);

		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&(ts->touch_functions), 2);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: Failed to send command(0x%x)",
					__func__, SEC_TS_CMD_SET_TOUCHFUNCTION);
	}

	sec_ts_set_grip_type(ts, ONLY_EDGE_HANDLER);

	if (ts->dex_mode) {
		input_info(true, &ts->client->dev, "%s: set dex mode\n", __func__);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_DEX_MODE, &ts->dex_mode, 1);
		if (ret < 0)
			input_err(true, &ts->client->dev,
					"%s: failed to set dex mode %x\n", __func__, ts->dex_mode);
	}

	if (ts->brush_mode) {
		input_info(true, &ts->client->dev, "%s: set brush mode\n", __func__);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_BRUSH_MODE, &ts->brush_mode, 1);
		if (ret < 0)
			input_err(true, &ts->client->dev,
					"%s: failed to set brush mode\n", __func__);
	}
	if (ts->touchable_area) {
		input_info(true, &ts->client->dev, "%s: set 16:9 mode\n", __func__);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHABLE_AREA, &ts->touchable_area, 1);
		if (ret < 0)
			input_err(true, &ts->client->dev,
					"%s: failed to set 16:9 mode\n", __func__);
	}
	return;
}

#define MAX_EVENT_COUNT 32
static void sec_ts_read_event(struct sec_ts_data *ts)
{
	int ret;
	u8 t_id;
	u8 event_id;
	u8 left_event_count;
	u8 read_event_buff[MAX_EVENT_COUNT][SEC_TS_EVENT_BUFF_SIZE] = { { 0 } };
	u8 *event_buff;
	struct sec_ts_event_coordinate *p_event_coord;
	struct sec_ts_event_status *p_event_status;
	int curr_pos;
	int remain_event_count = 0;
	int pre_ttype = 0;
	static bool error_report;

	if (ts->power_status == SEC_TS_STATE_LPM) {

		wake_lock_timeout(&ts->wakelock, msecs_to_jiffies(3 * MSEC_PER_SEC));
		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(&ts->resume_done, msecs_to_jiffies(3 * MSEC_PER_SEC));
		if (ret == 0) {
			input_err(true, &ts->client->dev, "%s: LPM: pm resume is not handled\n", __func__);
			return;
		}

		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: LPM: -ERESTARTSYS if interrupted, %d\n", __func__, ret);
			return;
		}

		input_info(true, &ts->client->dev, "%s: run LPM interrupt handler, %d\n", __func__, ret);
		/* run lpm interrupt handler */
	}

	ret = t_id = event_id = curr_pos = remain_event_count = 0;
	/* repeat READ_ONE_EVENT until buffer is empty(No event) */
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_ONE_EVENT, (u8 *)read_event_buff[0], SEC_TS_EVENT_BUFF_SIZE);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: i2c read one event failed\n", __func__);
		return;
	}

	if (ts->debug_flag & SEC_TS_DEBUG_PRINT_ONEEVENT)
		input_info(true, &ts->client->dev, "ONE: %02X %02X %02X %02X %02X %02X %02X %02X\n",
				read_event_buff[0][0], read_event_buff[0][1],
				read_event_buff[0][2], read_event_buff[0][3],
				read_event_buff[0][4], read_event_buff[0][5],
				read_event_buff[0][6], read_event_buff[0][7]);

	if (read_event_buff[0][0] == 0) {
		input_info(true, &ts->client->dev, "%s: event buffer is empty\n", __func__);
		return;
	}

	left_event_count = read_event_buff[0][7] & 0x3F;
	remain_event_count = left_event_count;

	if (left_event_count > MAX_EVENT_COUNT - 1) {
		input_err(true, &ts->client->dev, "%s: event buffer overflow\n", __func__);

		/* write clear event stack command when read_event_count > MAX_EVENT_COUNT */
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
		if (ret < 0)
			input_err(true, &ts->client->dev, "%s: i2c write clear event failed\n", __func__);
		return;
	}

	if (left_event_count > 0) {
		ret = sec_ts_i2c_read(ts, SEC_TS_READ_ALL_EVENT, (u8 *)read_event_buff[1],
				sizeof(u8) * (SEC_TS_EVENT_BUFF_SIZE) * (left_event_count));
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: i2c read one event failed\n", __func__);
			return;
		}
	}

	do {
		event_buff = read_event_buff[curr_pos];
		event_id = event_buff[0] & 0x3;

		if (ts->debug_flag & SEC_TS_DEBUG_PRINT_ALLEVENT)
			input_info(true, &ts->client->dev, "ALL: %02X %02X %02X %02X %02X %02X %02X %02X\n",
					event_buff[0], event_buff[1], event_buff[2], event_buff[3],
					event_buff[4], event_buff[5], event_buff[6], event_buff[7]);

		switch (event_id) {
		case SEC_TS_STATUS_EVENT:
			p_event_status = (struct sec_ts_event_status *)event_buff;

			/* tchsta == 0 && ttype == 0 && eid == 0 : buffer empty */
			if (p_event_status->stype > 0)
				input_info(true, &ts->client->dev, "%s: STATUS %x %x %x %x %x %x %x %x\n", __func__,
						event_buff[0], event_buff[1], event_buff[2],
						event_buff[3], event_buff[4], event_buff[5],
						event_buff[6], event_buff[7]);

			/* watchdog reset -> send SENSEON command */
			if ((p_event_status->stype == TYPE_STATUS_EVENT_INFO) &&
					(p_event_status->status_id == SEC_TS_ACK_BOOT_COMPLETE) &&
					(p_event_status->status_data_1 == 0x20)) {

				sec_ts_unlocked_release_all_finger(ts);

				ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
				if (ret < 0)
					input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
				sec_ts_reinit(ts);
			}

			/* event queue full-> all finger release */
			if ((p_event_status->stype == TYPE_STATUS_EVENT_ERR) &&
					(p_event_status->status_id == SEC_TS_ERR_EVENT_QUEUE_FULL)) {
				input_err(true, &ts->client->dev, "%s: IC Event Queue is full\n", __func__);
				sec_ts_unlocked_release_all_finger(ts);
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_ERR) &&
					(p_event_status->status_id == SEC_TS_ERR_EVENT_ESD)) {
				input_err(true, &ts->client->dev, "%s: ESD detected. run reset\n", __func__);
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_INFO) &&
					(p_event_status->status_id == SEC_TS_ACK_WET_MODE)) {
				ts->wet_mode = p_event_status->status_data_1;
				input_info(true, &ts->client->dev, "%s: water wet mode %d\n",
						__func__, ts->wet_mode);
				if (ts->wet_mode)
					ts->wet_count++;
			}

			if ((p_event_status->stype == TYPE_STATUS_EVENT_VENDOR_INFO) &&
					(p_event_status->status_id == SEC_TS_VENDOR_ACK_NOISE_STATUS_NOTI)) {

				ts->touch_noise_status = !!p_event_status->status_data_1;
				input_info(true, &ts->client->dev, "%s: TSP NOISE MODE %s[%d]\n",
						__func__, ts->touch_noise_status == 0 ? "OFF" : "ON",
						p_event_status->status_data_1);

				if (ts->touch_noise_status)
					ts->noise_count++;
			}

			break;

		case SEC_TS_COORDINATE_EVENT:
			if (!error_report && ts->abc_err_flag) {
				error_report = true;
#ifdef CONFIG_SEC_ABC
				sec_abc_send_event("MODULE=tsp@ERROR=power_status_mismatch");
#endif
			}
			error_report = false;

			if (ts->input_closed) {
				input_err(true, &ts->client->dev, "%s: device is closed\n", __func__);
				break;
			}

			p_event_coord = (struct sec_ts_event_coordinate *)event_buff;

			t_id = (p_event_coord->tid - 1);

			if (t_id < MAX_SUPPORT_TOUCH_COUNT + MAX_SUPPORT_HOVER_COUNT) {
				pre_ttype = ts->coord[t_id].ttype;
				ts->coord[t_id].id = t_id;
				ts->coord[t_id].action = p_event_coord->tchsta;
				ts->coord[t_id].x = (p_event_coord->x_11_4 << 4) | (p_event_coord->x_3_0);
				ts->coord[t_id].y = (p_event_coord->y_11_4 << 4) | (p_event_coord->y_3_0);
				ts->coord[t_id].z = p_event_coord->z & 0x3F;
				ts->coord[t_id].ttype = p_event_coord->ttype_3_2 << 2 | p_event_coord->ttype_1_0 << 0;
				ts->coord[t_id].major = p_event_coord->major;
				ts->coord[t_id].minor = p_event_coord->minor;

				if (!ts->coord[t_id].palm && (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM))
					ts->coord[t_id].palm_count++;

				ts->coord[t_id].palm = (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM);
				ts->coord[t_id].left_event = p_event_coord->left_event;

				if (ts->coord[t_id].z <= 0)
					ts->coord[t_id].z = 1;

				if ((ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_NORMAL)
						|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_PALM)
						|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_WET)
						|| (ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_GLOVE)) {

					if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_RELEASE) {
						do_gettimeofday(&ts->time_released[t_id]);

						if (ts->time_longest < (ts->time_released[t_id].tv_sec - ts->time_pressed[t_id].tv_sec))
							ts->time_longest = (ts->time_released[t_id].tv_sec - ts->time_pressed[t_id].tv_sec);

						input_mt_slot(ts->input_dev, t_id);
						if (ts->plat_data->support_mt_pressure)
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);

						if (ts->touch_count > 0)
							ts->touch_count--;
						if (ts->touch_count == 0) {
							input_report_key(ts->input_dev, BTN_TOUCH, 0);
							input_report_key(ts->input_dev, BTN_TOOL_FINGER, 0);
							ts->check_multi = 0;
						}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
						input_info(true, &ts->client->dev,
								"%s[R] tID:%d mc:%d tc:%d lx:%d ly:%d v:%02X%02X cal:%02X(%02X) id(%d,%d) p:%d noise:%x D%05X C%02XT%04X.%4s%s\n",
								ts->dex_name,
								t_id, ts->coord[t_id].mcount, ts->touch_count,
								ts->coord[t_id].x, ts->coord[t_id].y,
								ts->plat_data->img_version_of_ic[2],
								ts->plat_data->img_version_of_ic[3],
								ts->cal_status, ts->nv, ts->tspid_val,
								ts->tspicid_val, ts->coord[t_id].palm_count,
								ts->touch_noise_status, ts->defect_probability,
#ifdef TCLM_CONCEPT
								ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver,
								ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].f_name,
								(ts->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ");
#else
                                0,0," "," ");
#endif

#else
						input_info(true, &ts->client->dev,
								"%s[R] tID:%d mc:%d tc:%d v:%02X%02X cal:%02X(%02X) id(%d,%d) p:%d noise:%x D%05X C%02XT%04X.%4s%s\n",
								ts->dex_name,
								t_id, ts->coord[t_id].mcount, ts->touch_count,
								ts->plat_data->img_version_of_ic[2],
								ts->plat_data->img_version_of_ic[3],
								ts->cal_status, ts->nv, ts->tspid_val,
								ts->tspicid_val, ts->coord[t_id].palm_count,
								ts->touch_noise_status, ts->defect_probability,
#ifdef TCLM_CONCEPT
								ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver,
								ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].f_name,
								(ts->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ");
#else
                                0,0," "," ");
#endif
#endif
						ts->coord[t_id].action = SEC_TS_COORDINATE_ACTION_NONE;
						ts->coord[t_id].mcount = 0;
						ts->coord[t_id].palm_count = 0;


					} else if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS) {
						do_gettimeofday(&ts->time_pressed[t_id]);

						ts->touch_count++;
						ts->all_finger_count++;

						ts->max_z_value = max((unsigned int)ts->coord[t_id].z, ts->max_z_value);
						ts->min_z_value = min((unsigned int)ts->coord[t_id].z, ts->min_z_value);
						ts->sum_z_value += (unsigned int)ts->coord[t_id].z;

						input_mt_slot(ts->input_dev, t_id);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
						input_report_key(ts->input_dev, BTN_TOUCH, 1);
						input_report_key(ts->input_dev, BTN_TOOL_FINGER, 1);

						input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->coord[t_id].x);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->coord[t_id].y);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, ts->coord[t_id].major);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, ts->coord[t_id].minor);
						if (ts->brush_mode)
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM, (ts->coord[t_id].z << 1) | ts->coord[t_id].palm);
						else
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM, (BRUSH_Z_DATA << 1) | ts->coord[t_id].palm);
						if (ts->plat_data->support_mt_pressure)
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, ts->coord[t_id].z);

						if ((ts->touch_count > 4) && (ts->check_multi == 0)) {
							ts->check_multi = 1;
							ts->multi_count++;
						}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
						input_info(true, &ts->client->dev,
								"%s[P] tID:%d x:%d y:%d z:%d major:%d minor:%d tc:%d type:%X noise:%x\n",
								ts->dex_name, t_id, ts->coord[t_id].x,
								ts->coord[t_id].y, ts->coord[t_id].z,
								ts->coord[t_id].major, ts->coord[t_id].minor,
								ts->touch_count,
								ts->coord[t_id].ttype, ts->touch_noise_status);
#else
						input_info(true, &ts->client->dev,
								"%s[P] tID:%d z:%d major:%d minor:%d tc:%d type:%X noise:%x\n",
								ts->dex_name,
								t_id, ts->coord[t_id].z, ts->coord[t_id].major,
								ts->coord[t_id].minor, ts->touch_count,
								ts->coord[t_id].ttype, ts->touch_noise_status);
#endif
					} else if (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_MOVE) {
						if ((ts->coord[t_id].ttype == SEC_TS_TOUCHTYPE_GLOVE) && !ts->touchkey_glove_mode_status) {
							ts->touchkey_glove_mode_status = true;
							input_report_switch(ts->input_dev, SW_GLOVE, 1);
						} else if ((ts->coord[t_id].ttype != SEC_TS_TOUCHTYPE_GLOVE) && ts->touchkey_glove_mode_status) {
							ts->touchkey_glove_mode_status = false;
							input_report_switch(ts->input_dev, SW_GLOVE, 0);
						}

						input_mt_slot(ts->input_dev, t_id);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
						input_report_key(ts->input_dev, BTN_TOUCH, 1);
						input_report_key(ts->input_dev, BTN_TOOL_FINGER, 1);

						input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->coord[t_id].x);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->coord[t_id].y);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, ts->coord[t_id].major);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, ts->coord[t_id].minor);
						if (ts->brush_mode)
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM, (ts->coord[t_id].z << 1) | ts->coord[t_id].palm);
						else
							input_report_abs(ts->input_dev, ABS_MT_CUSTOM, (BRUSH_Z_DATA << 1) | ts->coord[t_id].palm);

						if (ts->plat_data->support_mt_pressure)
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, ts->coord[t_id].z);
						ts->coord[t_id].mcount++;
					} else {
						input_dbg(true, &ts->client->dev,
								"%s: do not support coordinate action(%d)\n", __func__, ts->coord[t_id].action);
					}

					if ((ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_PRESS)
							|| (ts->coord[t_id].action == SEC_TS_COORDINATE_ACTION_MOVE)) {

						if (ts->coord[t_id].ttype != pre_ttype) {
							input_info(true, &ts->client->dev, "%s : tID:%d ttype(%x->%x)\n",
									__func__, ts->coord[t_id].id,
									pre_ttype, ts->coord[t_id].ttype);
						}
					}

				} else {
					input_dbg(true, &ts->client->dev,
							"%s: do not support coordinate type(%d)\n", __func__, ts->coord[t_id].ttype);
				}
			} else {
				input_err(true, &ts->client->dev, "%s: tid(%d) is out of range\n", __func__, t_id);
			}
			break;
		default:
			input_err(true, &ts->client->dev, "%s: unknown event %x %x %x %x %x %x\n", __func__,
					event_buff[0], event_buff[1], event_buff[2],
					event_buff[3], event_buff[4], event_buff[5]);
			break;
		}

		curr_pos++;
		remain_event_count--;
	} while (remain_event_count >= 0);

	input_sync(ts->input_dev);
}

static irqreturn_t sec_ts_irq_thread(int irq, void *ptr)
{
	struct sec_ts_data *ts = (struct sec_ts_data *)ptr;

	mutex_lock(&ts->eventlock);

	sec_ts_read_event(ts);

	mutex_unlock(&ts->eventlock);

	return IRQ_HANDLED;
}

int get_tsp_status(void)
{
	return 0;
}
EXPORT_SYMBOL(get_tsp_status);

void sec_ts_set_charger(bool enable)
{
	return;
#if 0
	int ret;
	u8 noise_mode_on[] = {0x01};
	u8 noise_mode_off[] = {0x00};

	if (enable) {
		input_info(true, &ts->client->dev, "sec_ts_set_charger : charger CONNECTED!!\n");
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_NOISE_MODE, noise_mode_on, sizeof(noise_mode_on));
		if (ret < 0)
			input_err(true, &ts->client->dev, "sec_ts_set_charger: fail to write NOISE_ON\n");
	} else {
		input_info(true, &ts->client->dev, "sec_ts_set_charger : charger DISCONNECTED!!\n");
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_NOISE_MODE, noise_mode_off, sizeof(noise_mode_off));
		if (ret < 0)
			input_err(true, &ts->client->dev, "sec_ts_set_charger: fail to write NOISE_OFF\n");
	}
#endif
}
EXPORT_SYMBOL(sec_ts_set_charger);

int sec_ts_glove_mode_enables(struct sec_ts_data *ts, int mode)
{
	int ret;

	if (mode)
		ts->touch_functions = (ts->touch_functions | SEC_TS_BIT_SETFUNC_GLOVE | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);
	else
		ts->touch_functions = ((ts->touch_functions & (~SEC_TS_BIT_SETFUNC_GLOVE)) | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: pwr off, glove:%d, status:%x\n", __func__,
				mode, ts->touch_functions);
		goto glove_enable_err;
	}

	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&ts->touch_functions, 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Failed to send command", __func__);
		goto glove_enable_err;
	}

	input_info(true, &ts->client->dev, "%s: glove:%d, status:%x\n", __func__,
			mode, ts->touch_functions);

	return 0;

glove_enable_err:
	return -EIO;
}
EXPORT_SYMBOL(sec_ts_glove_mode_enables);

int sec_ts_set_cover_type(struct sec_ts_data *ts, bool enable)
{
	int ret;

	input_info(true, &ts->client->dev, "%s: %d\n", __func__, ts->cover_type);


	switch (ts->cover_type) {
	case SEC_TS_VIEW_WIRELESS:
	case SEC_TS_VIEW_COVER:
	case SEC_TS_VIEW_WALLET:
	case SEC_TS_FLIP_WALLET:
	case SEC_TS_LED_COVER:
	case SEC_TS_MONTBLANC_COVER:
	case SEC_TS_CLEAR_FLIP_COVER:
	case SEC_TS_QWERTY_KEYBOARD_EUR:
	case SEC_TS_QWERTY_KEYBOARD_KOR:
		ts->cover_cmd = (u8)ts->cover_type;
		break;
	case SEC_TS_CHARGER_COVER:
	case SEC_TS_COVER_NOTHING1:
	case SEC_TS_COVER_NOTHING2:
	default:
		ts->cover_cmd = 0;
		input_err(true, &ts->client->dev, "%s: not chage touch state, %d\n",
				__func__, ts->cover_type);
		break;
	}

	if (enable)
		ts->touch_functions = (ts->touch_functions | SEC_TS_BIT_SETFUNC_COVER | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);
	else
		ts->touch_functions = ((ts->touch_functions & (~SEC_TS_BIT_SETFUNC_COVER)) | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: pwr off, close:%d, status:%x\n", __func__,
				enable, ts->touch_functions);
		goto cover_enable_err;
	}

	if (enable) {
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, &ts->cover_cmd, 1);
		if (ret < 0) {
			input_err(true, &ts->client->dev, "%s: Failed to send covertype command: %d", __func__, ts->cover_cmd);
			goto cover_enable_err;
		}
	}

	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&(ts->touch_functions), 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Failed to send command", __func__);
		goto cover_enable_err;
	}

	input_info(true, &ts->client->dev, "%s: close:%d, status:%x\n", __func__,
			enable, ts->touch_functions);

	return 0;

cover_enable_err:
	return -EIO;


}
EXPORT_SYMBOL(sec_ts_set_cover_type);

void sec_ts_set_grip_type(struct sec_ts_data *ts, u8 set_type)
{
	u8 mode = G_NONE;

	input_info(true, &ts->client->dev, "%s: re-init grip(%d), edh:%d, edg:%d, lan:%d\n", __func__,
			set_type, ts->grip_edgehandler_direction, ts->grip_edge_range, ts->grip_landscape_mode);

	/* edge handler */
	if (ts->grip_edgehandler_direction != 0)
		mode |= G_SET_EDGE_HANDLER;

	if (set_type == GRIP_ALL_DATA) {
		/* edge */
		if (ts->grip_edge_range != 60)
			mode |= G_SET_EDGE_ZONE;

		/* dead zone */
		if (ts->grip_landscape_mode == 1)	/* default 0 mode, 32 */
			mode |= G_SET_LANDSCAPE_MODE;
		else
			mode |= G_SET_NORMAL_MODE;
	}

	if (mode)
		set_grip_data_to_ic(ts, mode);

}

/* for debugging--------------------------------------------------------------------------------------*/
static int sec_ts_pinctrl_configure(struct sec_ts_data *ts, bool enable)
{
	struct pinctrl_state *state;

	input_info(true, &ts->client->dev, "%s: %s\n", __func__, enable ? "ACTIVE" : "SUSPEND");

	if (enable) {
		state = pinctrl_lookup_state(ts->plat_data->pinctrl, "on_state");
		if (IS_ERR(ts->plat_data->pinctrl))
			input_err(true, &ts->client->dev, "%s: could not get active pinstate\n", __func__);
	} else {
		state = pinctrl_lookup_state(ts->plat_data->pinctrl, "off_state");
		if (IS_ERR(ts->plat_data->pinctrl))
			input_err(true, &ts->client->dev, "%s: could not get suspend pinstate\n", __func__);
	}

	if (!IS_ERR_OR_NULL(state))
		return pinctrl_select_state(ts->plat_data->pinctrl, state);

	return 0;

}

static int sec_ts_parse_dt(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct sec_ts_plat_data *pdata = dev->platform_data;
	struct device_node *np = dev->of_node;
	u32 coords[2];
	int ret = 0;
	int count = 0;
	u32 ic_match_value;
	int lcdtype = 0;
#if 0 /*defined(CONFIG_EXYNOS_DECON_FB)*/
	int connected;
#endif

	pdata->tsp_icid = of_get_named_gpio(np, "sec,tsp-icid_gpio", 0);
	if (gpio_is_valid(pdata->tsp_icid)) {
		input_info(true, dev, "%s: TSP_ICID : %d\n", __func__, gpio_get_value(pdata->tsp_icid));
		if (of_property_read_u32(np, "sec,icid_match_value", &ic_match_value)) {
			input_err(true, dev, "%s: Failed to get icid match value\n", __func__);
			return -EINVAL;
		}

		if (gpio_get_value(pdata->tsp_icid) != ic_match_value) {
			input_err(true, dev, "%s: Do not match TSP_ICID\n", __func__);
			return -EINVAL;
		}
	} else {
		input_err(true, dev, "%s: Failed to get tsp-icid gpio\n", __func__);
	}

	pdata->tsp_vsync = of_get_named_gpio(np, "sec,tsp_vsync_gpio", 0);
	if (gpio_is_valid(pdata->tsp_vsync))
		input_info(true, &client->dev, "%s: vsync %s\n", __func__,
				gpio_get_value(pdata->tsp_vsync) ? "disable" : "enable");

	pdata->irq_gpio = of_get_named_gpio(np, "sec,irq_gpio", 0);
	if (gpio_is_valid(pdata->irq_gpio)) {
		ret = gpio_request_one(pdata->irq_gpio, GPIOF_DIR_IN, "sec,tsp_int");
		if (ret) {
			input_err(true, &client->dev, "%s: Unable to request tsp_int [%d]\n", __func__, pdata->irq_gpio);
			return -EINVAL;
		}
	} else {
		input_err(true, &client->dev, "%s: Failed to get irq gpio\n", __func__);
		return -EINVAL;
	}

	client->irq = gpio_to_irq(pdata->irq_gpio);

	if (of_property_read_u32(np, "sec,irq_type", &pdata->irq_type)) {
		input_err(true, dev, "%s: Failed to get irq_type property\n", __func__);
		pdata->irq_type = IRQF_TRIGGER_LOW | IRQF_ONESHOT;
	}

	if (of_property_read_u32(np, "sec,i2c-burstmax", &pdata->i2c_burstmax)) {
		input_dbg(false, &client->dev, "%s: Failed to get i2c_burstmax property\n", __func__);
		pdata->i2c_burstmax = 256;
	}

	if (of_property_read_u32_array(np, "sec,max_coords", coords, 2)) {
		input_err(true, &client->dev, "%s: Failed to get max_coords property\n", __func__);
		return -EINVAL;
	}
	pdata->max_x = coords[0] - 1;
	pdata->max_y = coords[1] - 1;

	pdata->tsp_id = of_get_named_gpio(np, "sec,tsp-id_gpio", 0);
	if (gpio_is_valid(pdata->tsp_id))
		input_info(true, dev, "%s: TSP_ID : %d\n", __func__, gpio_get_value(pdata->tsp_id));
	else
		input_err(true, dev, "%s: Failed to get tsp-id gpio\n", __func__);

	count = of_property_count_strings(np, "sec,firmware_name");
	if (count <= 0) {
		pdata->firmware_name = NULL;
	} else {
		if (gpio_is_valid(pdata->tsp_id))
			of_property_read_string_index(np, "sec,firmware_name", gpio_get_value(pdata->tsp_id), &pdata->firmware_name);
		else
			of_property_read_string_index(np, "sec,firmware_name", 0, &pdata->firmware_name);
	}

	if (of_property_read_string_index(np, "sec,project_name", 0, &pdata->project_name))
		input_err(true, &client->dev, "%s: skipped to get project_name property\n", __func__);
	if (of_property_read_string_index(np, "sec,project_name", 1, &pdata->model_name))
		input_err(true, &client->dev, "%s: skipped to get model_name property\n", __func__);

#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG)
	lcdtype = get_lcd_attached("GET");
	if (lcdtype == 0xFFFFFF) {
		input_err(true, &client->dev, "%s: lcd is not attached\n", __func__);
		return -ENODEV;
	}
#endif

	if (strncmp(pdata->model_name, "G950", 4) == 0)
		pdata->panel_revision = 0;
	else
		pdata->panel_revision = ((lcdtype >> 8) & 0xFF) >> 4;

	if (of_property_read_u32(np, "sec,bringup", &pdata->bringup) < 0)
		pdata->bringup = 0;

	if (of_property_read_u32(np, "sec,mis_cal_check", &pdata->mis_cal_check) < 0)
		pdata->mis_cal_check = 0;

	pdata->support_sidegesture = of_property_read_bool(np, "sec,support_sidegesture");
	pdata->support_dex = of_property_read_bool(np, "support_dex_mode");
	pdata->support_brush = of_property_read_bool(np, "support_brush_mode");
	pdata->support_touchable_area = of_property_read_bool(np, "support_touchable_area");
	pdata->support_girp = of_property_read_bool(np, "support_grip");
	pdata->support_log_level = of_property_read_bool(np, "support_log_level");
	pdata->support_wirelesscharger_mode = of_property_read_bool(np, "support_wirelesscharger_mode");
	pdata->use_ic_resolution = of_property_read_bool(np, "sec,use_ic_resolution");
	if (of_property_read_u32(np, "sec,factory_item_version", &pdata->item_version) < 0)
		pdata->item_version = 0;

#ifdef CONFIG_SEC_FACTORY
	pdata->support_mt_pressure = true;
#endif

	input_err(true, &client->dev, "%s: i2c buffer limit: %d, lcd_id:%06X, bringup:%d, FW:%s(%d), id:%d,%d, mis_cal:%d dex:%d, gesture:%d\n",
		__func__, pdata->i2c_burstmax, lcdtype, pdata->bringup, pdata->firmware_name,
			count, pdata->tsp_id, pdata->tsp_icid, pdata->mis_cal_check,
			pdata->support_dex, pdata->support_sidegesture);
	return ret;
}

static void sec_tclm_parse_dt(struct i2c_client *client, struct sec_tclm_data *tdata)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "sec,tclm_level", &tdata->tclm_level) < 0) {
		tdata->tclm_level = 0;
		input_err(true, dev, "%s: Failed to get tclm_level property\n", __func__);
	}

	if (of_property_read_u32(np, "sec,afe_base", &tdata->afe_base) < 0) {
		tdata->afe_base = 0;
		input_err(true, dev, "%s: Failed to get afe_base property\n", __func__);
	}

	input_err(true, &client->dev, "%s: tclm_level %d, sec_afe_base %d\n", __func__, tdata->tclm_level, tdata->afe_base);

}


int sec_ts_read_information(struct sec_ts_data *ts)
{
	unsigned char data[13] = { 0 };
	int ret;

	memset(data, 0x0, 3);
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_ID, data, 3);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read device id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: %X, %X, %X\n",
			__func__, data[0], data[1], data[2]);
	memset(data, 0x0, 11);
	ret = sec_ts_i2c_read(ts,  SEC_TS_READ_PANEL_INFO, data, 11);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: nTX:%X, nRX:%X, rY:%d, rX:%d\n",
			__func__, data[8], data[9],
			(data[2] << 8) | data[3], (data[0] << 8) | data[1]);

	/* Set X,Y Resolution from IC information. */
	if (ts->plat_data->use_ic_resolution) {
		if (((data[0] << 8) | data[1]) > 0)
			ts->plat_data->max_x = ((data[0] << 8) | data[1]) - 1;

		if (((data[2] << 8) | data[3]) > 0)
			ts->plat_data->max_y = ((data[2] << 8) | data[3]) - 1;
	}

	if (data[8] != 0 && data[9] != 0) {
		ts->tx_count = data[8];
		ts->rx_count = data[9];
	} else {
		ts->tx_count = 30; /* j7top data */
		ts->rx_count = 15;
	}

	data[0] = 0;
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, data, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: STATUS : %X\n",
			__func__, data[0]);

	memset(data, 0x0, 4);
	ret = sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, data, 4);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: TOUCH STATUS : %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3]);
	ret = sec_ts_i2c_read(ts, SEC_TS_CMD_SET_TOUCHFUNCTION,  (u8 *)&(ts->touch_functions), 2);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read touch functions(%d)\n",
				__func__, ret);
		return ret;
	}

	input_info(true, &ts->client->dev,
			"%s: Functions : %02X\n",
			__func__, ts->touch_functions);

	return ret;
}

static void sec_ts_set_input_prop(struct sec_ts_data *ts, struct input_dev *dev, u8 propbit)
{
	static char sec_ts_phys[64] = { 0 };

	snprintf(sec_ts_phys, sizeof(sec_ts_phys), "%s/input1",
			dev->name);
	dev->phys = sec_ts_phys;
	dev->id.bustype = BUS_I2C;
	dev->dev.parent = &ts->client->dev;

	set_bit(EV_SYN, dev->evbit);
	set_bit(EV_KEY, dev->evbit);
	set_bit(EV_ABS, dev->evbit);
	set_bit(EV_SW, dev->evbit);
	set_bit(BTN_TOUCH, dev->keybit);
	set_bit(BTN_TOOL_FINGER, dev->keybit);
	set_bit(KEY_BLACK_UI_GESTURE, dev->keybit);
#ifdef SEC_TS_SUPPORT_TOUCH_KEY
	if (ts->plat_data->support_mskey) {
		int i;

		for (i = 0 ; i < ts->plat_data->num_touchkey ; i++)
			set_bit(ts->plat_data->touchkey[i].keycode, dev->keybit);

		set_bit(EV_LED, dev->evbit);
		set_bit(LED_MISC, dev->ledbit);
	}
#endif
	if (ts->plat_data->support_sidegesture) {
		set_bit(KEY_SIDE_GESTURE, dev->keybit);
		set_bit(KEY_SIDE_GESTURE_RIGHT, dev->keybit);
		set_bit(KEY_SIDE_GESTURE_LEFT, dev->keybit);
	}
	set_bit(propbit, dev->propbit);
	set_bit(KEY_HOMEPAGE, dev->keybit);

	input_set_capability(dev, EV_SW, SW_GLOVE);

	input_set_abs_params(dev, ABS_MT_POSITION_X, 0, ts->plat_data->max_x, 0, 0);
	input_set_abs_params(dev, ABS_MT_POSITION_Y, 0, ts->plat_data->max_y, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_TOUCH_MINOR, 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_MT_CUSTOM, 0, 0xFFFF, 0, 0);
	if (ts->plat_data->support_mt_pressure)
		input_set_abs_params(dev, ABS_MT_PRESSURE, 0, 255, 0, 0);

	if (propbit == INPUT_PROP_POINTER)
		input_mt_init_slots(dev, MAX_SUPPORT_TOUCH_COUNT, INPUT_MT_POINTER);
	else
		input_mt_init_slots(dev, MAX_SUPPORT_TOUCH_COUNT, INPUT_MT_DIRECT);

	input_set_drvdata(dev, ts);
}

static int sec_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sec_ts_data *ts;
	struct sec_ts_plat_data *pdata;
	struct sec_tclm_data *tdata = NULL;
	int ret = 0;
	bool force_update = false;
	bool valid_firmware_integrity = false;
	unsigned char data[5] = { 0 };
	unsigned char deviceID[5] = { 0 };
	unsigned char result = 0;

	input_info(true, &client->dev, "%s\n", __func__);

/*	if (!lcdtype) {
		input_err(true, &client->dev, "lcd is disconnected\n");
		return -ENODEV;
	}
*/
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		input_err(true, &client->dev, "%s: EIO err!\n", __func__);
		return -EIO;
	}

	/* parse dt */
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_ts_plat_data), GFP_KERNEL);

		if (!pdata) {
			input_err(true, &client->dev, "%s: Failed to allocate platform data\n", __func__);
			goto error_allocate_pdata;
		}

		client->dev.platform_data = pdata;

		ret = sec_ts_parse_dt(client);
		if (ret) {
			input_err(true, &client->dev, "%s: Failed to parse dt\n", __func__);
			goto error_allocate_mem;
		}
		tdata = devm_kzalloc(&client->dev,
				sizeof(struct sec_tclm_data), GFP_KERNEL);
		if (!tdata)
			goto error_allocate_tdata;

		sec_tclm_parse_dt(client, tdata);
	} else {
		pdata = client->dev.platform_data;
		if (!pdata) {
			input_err(true, &client->dev, "%s: No platform data found\n", __func__);
			goto error_allocate_pdata;
		}
	}

	pdata->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(pdata->pinctrl))
		input_err(true, &client->dev, "%s: could not get pinctrl\n", __func__);

	ts = kzalloc(sizeof(struct sec_ts_data), GFP_KERNEL);
	if (!ts)
		goto error_allocate_mem;

	ts->client = client;
	ts->plat_data = pdata;
	ts->crc_addr = 0x0001FE00;
	ts->fw_addr = 0x00002000;
	ts->para_addr = 0x18000;
	ts->flash_page_size = SEC_TS_FW_BLK_SIZE_DEFAULT;
	ts->sec_ts_i2c_read = sec_ts_i2c_read;
	ts->sec_ts_i2c_write = sec_ts_i2c_write;
	ts->sec_ts_i2c_write_burst = sec_ts_i2c_write_burst;
	ts->sec_ts_i2c_read_bulk = sec_ts_i2c_read_bulk;
	ts->i2c_burstmax = pdata->i2c_burstmax;
	ts->tdata = tdata;
	if (!ts->tdata)
		goto err_null_tdata;

#ifdef TCLM_CONCEPT
	sec_tclm_initialize(ts->tdata);
	ts->tdata->client = ts->client;
	ts->tdata->tclm_read = sec_tclm_data_read;
	ts->tdata->tclm_write = sec_tclm_data_write;
	ts->tdata->tclm_execute_force_calibration = sec_tclm_execute_force_calibration;
#endif
	INIT_DELAYED_WORK(&ts->work_read_info, sec_ts_read_info_work);

	i2c_set_clientdata(client, ts);

	if (gpio_is_valid(ts->plat_data->tsp_id))
		ts->tspid_val = gpio_get_value(ts->plat_data->tsp_id);

	if (gpio_is_valid(ts->plat_data->tsp_icid))
		ts->tspicid_val = gpio_get_value(ts->plat_data->tsp_icid);

	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		input_err(true, &ts->client->dev, "%s: allocate device err!\n", __func__);
		ret = -ENOMEM;
		goto err_allocate_input_dev;
	}

	if (ts->plat_data->support_dex) {
		ts->input_dev_pad = input_allocate_device();
		if (!ts->input_dev_pad) {
			input_err(true, &ts->client->dev, "%s: allocate device err!\n", __func__);
			ret = -ENOMEM;
			goto err_allocate_input_dev_pad;
		}
	}

	ts->touch_count = 0;
	ts->sec_ts_i2c_write = sec_ts_i2c_write;
	ts->sec_ts_i2c_read = sec_ts_i2c_read;

	ts->max_z_value = 0;
	ts->min_z_value = 0xFFFFFFFF;
	ts->sum_z_value = 0;

	mutex_init(&ts->lock);
	mutex_init(&ts->device_mutex);
	mutex_init(&ts->i2c_mutex);
	mutex_init(&ts->eventlock);
	mutex_init(&ts->modechange);

	wake_lock_init(&ts->wakelock, WAKE_LOCK_SUSPEND, "tsp_wakelock");
	init_completion(&ts->resume_done);
	complete_all(&ts->resume_done);

	input_info(true, &client->dev, "%s: init resource\n", __func__);

	sec_ts_pinctrl_configure(ts, true);

	/* power enable */

	ts->power_status = SEC_TS_STATE_POWER_ON;
	ts->tdata->external_factory = false;

	sec_ts_wait_for_ready(ts, SEC_TS_ACK_BOOT_COMPLETE);

	input_info(true, &client->dev, "%s: power enable\n", __func__);

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_DEVICE_ID, deviceID, 5);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: failed to read device ID(%d)\n", __func__, ret);
	else
		input_info(true, &ts->client->dev,
				"%s: TOUCH DEVICE ID : %02X, %02X, %02X, %02X, %02X\n", __func__,
				deviceID[0], deviceID[1], deviceID[2], deviceID[3], deviceID[4]);

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_FIRMWARE_INTEGRITY, &result, 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: failed to integrity check (%d)\n", __func__, ret);
	} else {
		if (result & 0x80) {
			valid_firmware_integrity = true;
		} else if (result & 0x40) {
			valid_firmware_integrity = false;
			input_err(true, &ts->client->dev, "%s: invalid firmware (0x%x)\n", __func__, result);
		} else {
			valid_firmware_integrity = false;
			input_err(true, &ts->client->dev, "%s: invalid integrity result (0x%x)\n", __func__, result);
		}
	}

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, &data[0], 1);
	if (ret < 0) {
		input_err(true, &ts->client->dev,
				"%s: failed to read sub id(%d)\n",
				__func__, ret);
	} else {
		ret = sec_ts_i2c_read(ts, SEC_TS_READ_TS_STATUS, &data[1], 4);
		if (ret < 0) {
			input_err(true, &ts->client->dev,
					"%s: failed to touch status(%d)\n",
					__func__, ret);
		}
	}
	input_info(true, &ts->client->dev,
			"%s: TOUCH STATUS : %02X || %02X, %02X, %02X, %02X\n",
			__func__, data[0], data[1], data[2], data[3], data[4]);

	if (data[0] == SEC_TS_STATUS_BOOT_MODE)
		ts->checksum_result = 1;

	if ((data[0] == SEC_TS_STATUS_APP_MODE && data[2] == TOUCH_SYSTEM_MODE_FLASH) ||
			!valid_firmware_integrity)
		force_update = true;
	else
#if defined(CONFIG_SEC_J4CORELTE_PROJECT) && defined(CONFIG_SEC_FACTORY)
			force_update = true;
#else
			force_update = false;
#endif

	ret = sec_ts_firmware_update_on_probe(ts, force_update);
	if (ret < 0)
		goto err_init;

	ret = sec_ts_read_information(ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to read information 0x%x\n", __func__, ret);
		goto err_init;
	}

	ret = sec_ts_i2c_read(ts, SEC_TS_READ_DEVICE_ID, deviceID, 5);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: failed to read device ID2:(%d)\n", __func__, ret);
	else
		input_info(true, &ts->client->dev,
				"%s: TOUCH DEVICE ID : %02X, %02X, %02X, %02X, %02X\n", __func__,
				deviceID[0], deviceID[1], deviceID[2], deviceID[3], deviceID[4]);

	ts->touch_functions |= SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC;
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&ts->touch_functions, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to send touch func_mode command", __func__);

	sec_ts_sw_reset(ts, SEC_TS_CMD_SW_RESET);

	/* Sense_on */
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
		goto err_init;
	}

	ts->pFrame = kzalloc(ts->tx_count * ts->rx_count * 2, GFP_KERNEL);
	if (!ts->pFrame) {
		ret = -ENOMEM;
		goto err_allocate_frame;
	}

	if (ts->plat_data->support_dex) {
		ts->input_dev_pad->name = "sec_touchpad";
		sec_ts_set_input_prop(ts, ts->input_dev_pad, INPUT_PROP_POINTER);
	}
	ts->dex_name = "";

	ts->input_dev->name = "sec_touchscreen";
	sec_ts_set_input_prop(ts, ts->input_dev, INPUT_PROP_DIRECT);

#ifdef CONFIG_FB
	ts->fb_notifier.notifier_call = sec_ts_fb_notifier_cb;
	ret = fb_register_client(&ts->fb_notifier);
	if (ret) {
		input_err(true, &ts->client->dev, "failed to register fb notifier client\n");
		goto err_allocate_frame;
	}
#endif

	ts->input_dev_touch = ts->input_dev;

	ret = input_register_device(ts->input_dev);
	if (ret) {
		input_err(true, &ts->client->dev, "%s: Unable to register %s input device\n", __func__, ts->input_dev->name);
		goto err_input_register_device;
	}
	if (ts->plat_data->support_dex) {
		ret = input_register_device(ts->input_dev_pad);
		if (ret) {
			input_err(true, &ts->client->dev, "%s: Unable to register %s input device\n", __func__, ts->input_dev_pad->name);
			goto err_input_pad_register_device;
		}
	}

	input_info(true, &ts->client->dev, "%s: request_irq = %d\n", __func__, client->irq);

	ret = request_threaded_irq(client->irq, NULL, sec_ts_irq_thread,
			ts->plat_data->irq_type, SEC_TS_I2C_NAME, ts);
	if (ret < 0) {
		input_err(true, &ts->client->dev, "%s: Unable to request threaded irq\n", __func__);
		goto err_irq;
	}

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	tsp_info = ts;

	trustedui_set_tsp_irq(client->irq);
	input_info(true, &client->dev, "%s[%d] called!\n",
			__func__, client->irq);
#endif

	/* need remove below resource @ remove driver */

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	sec_ts_raw_device_init(ts);
#endif

	sec_ts_fn_init(ts);

	device_init_wakeup(&client->dev, true);

	schedule_delayed_work(&ts->work_read_info, msecs_to_jiffies(5000));

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	dump_callbacks.inform_dump = dump_tsp_log;
	INIT_DELAYED_WORK(&ts->ghost_check, sec_ts_check_rawdata);
	p_ghost_check = &ts->ghost_check;
#endif

	ts_dup = ts;
	ts->probe_done = true;

	input_err(true, &ts->client->dev, "%s: done\n", __func__);
	input_log_fix();

	return 0;

err_irq:
	if (ts->plat_data->support_dex) {
		input_unregister_device(ts->input_dev_pad);
		ts->input_dev_pad = NULL;
	}
err_input_pad_register_device:
	input_unregister_device(ts->input_dev);
	ts->input_dev = NULL;
	ts->input_dev_touch = NULL;
err_input_register_device:
	kfree(ts->pFrame);
err_allocate_frame:
err_init:
	wake_lock_destroy(&ts->wakelock);
	if (ts->plat_data->support_dex) {
		if (ts->input_dev_pad)
			input_free_device(ts->input_dev_pad);
	}
err_allocate_input_dev_pad:
	if (ts->input_dev)
		input_free_device(ts->input_dev);
err_allocate_input_dev:
err_null_tdata:
	kfree(ts);

error_allocate_mem:
	if (gpio_is_valid(pdata->irq_gpio))
		gpio_free(pdata->irq_gpio);
	if (gpio_is_valid(pdata->tsp_id))
		gpio_free(pdata->tsp_id);
	if (gpio_is_valid(pdata->tsp_icid))
		gpio_free(pdata->tsp_icid);

error_allocate_tdata:
error_allocate_pdata:
	if (ret == -ECONNREFUSED)
		sec_ts_delay(100);
	ret = -ENODEV;
#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	p_ghost_check = NULL;
#endif
	ts_dup = NULL;
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	tsp_info = NULL;
#endif
	input_err(true, &client->dev, "%s: failed(%d)\n", __func__, ret);
	input_log_fix();
	return ret;
}

void sec_ts_unlocked_release_all_finger(struct sec_ts_data *ts)
{
	int i;

	for (i = 0; i < MAX_SUPPORT_TOUCH_COUNT; i++) {
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

		if ((ts->coord[i].action == SEC_TS_COORDINATE_ACTION_PRESS) ||
				(ts->coord[i].action == SEC_TS_COORDINATE_ACTION_MOVE)) {

			ts->coord[i].action = SEC_TS_COORDINATE_ACTION_RELEASE;
			input_info(true, &ts->client->dev,
					"%s: [RA] tID:%d mc:%d tc:%d v:%02X%02X, id(%d,%d) p:%d, D%05X, C%02XT%04X.%4s%s\n",
					__func__, i, ts->coord[i].mcount, ts->touch_count,
					ts->plat_data->img_version_of_ic[2],
					ts->plat_data->img_version_of_ic[3],
					ts->tspid_val, ts->tspicid_val,
					ts->coord[i].palm_count, ts->defect_probability,
					ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver,
					ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].f_name,
					(ts->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN)?".L":" ");

			do_gettimeofday(&ts->time_released[i]);

			if (ts->time_longest < (ts->time_released[i].tv_sec - ts->time_pressed[i].tv_sec))
				ts->time_longest = (ts->time_released[i].tv_sec - ts->time_pressed[i].tv_sec);
		}

		ts->coord[i].mcount = 0;
		ts->coord[i].palm_count = 0;

	}

	input_mt_slot(ts->input_dev, 0);

	input_report_key(ts->input_dev, BTN_TOUCH, false);
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, false);
	input_report_switch(ts->input_dev, SW_GLOVE, false);
	ts->touchkey_glove_mode_status = false;
	ts->touch_count = 0;
	ts->check_multi = 0;

	if (ts->plat_data->support_sidegesture) {
		input_report_key(ts->input_dev, KEY_SIDE_GESTURE, 0);
		input_report_key(ts->input_dev, KEY_SIDE_GESTURE_LEFT, 0);
		input_report_key(ts->input_dev, KEY_SIDE_GESTURE_RIGHT, 0);
	}

	input_report_key(ts->input_dev, KEY_HOMEPAGE, 0);
	input_sync(ts->input_dev);

}

void sec_ts_locked_release_all_finger(struct sec_ts_data *ts)
{
	int i;

	mutex_lock(&ts->eventlock);

	for (i = 0; i < MAX_SUPPORT_TOUCH_COUNT; i++) {
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

		if ((ts->coord[i].action == SEC_TS_COORDINATE_ACTION_PRESS) ||
				(ts->coord[i].action == SEC_TS_COORDINATE_ACTION_MOVE)) {

			ts->coord[i].action = SEC_TS_COORDINATE_ACTION_RELEASE;
			input_info(true, &ts->client->dev,
					"%s: [RA] tID:%d mc: %d tc:%d, v:%02X%02X, cal:%X(%X) C%02XT%04X.%4s%s id(%d,%d), p:%d, D%05X\n",
					__func__, i, ts->coord[i].mcount, ts->touch_count,
					ts->plat_data->img_version_of_ic[2],
					ts->plat_data->img_version_of_ic[3],
					ts->cal_status, ts->nv,
					ts->tdata->nvdata.cal_count, ts->tdata->nvdata.tune_fix_ver,
					ts->tdata->tclm_string[ts->tdata->nvdata.cal_position].f_name,
					(ts->tdata->tclm_level == TCLM_LEVEL_LOCKDOWN) ? ".L" : " ",
					ts->tspid_val, ts->tspicid_val, ts->coord[i].palm_count,
					ts->defect_probability);

			do_gettimeofday(&ts->time_released[i]);

			if (ts->time_longest < (ts->time_released[i].tv_sec - ts->time_pressed[i].tv_sec))
				ts->time_longest = (ts->time_released[i].tv_sec - ts->time_pressed[i].tv_sec);
		}

		ts->coord[i].mcount = 0;
		ts->coord[i].palm_count = 0;

	}

	input_mt_slot(ts->input_dev, 0);

	input_report_key(ts->input_dev, BTN_TOUCH, false);
	input_report_key(ts->input_dev, BTN_TOOL_FINGER, false);
	input_report_switch(ts->input_dev, SW_GLOVE, false);
	ts->touchkey_glove_mode_status = false;
	ts->touch_count = 0;
	ts->check_multi = 0;

	if (ts->plat_data->support_sidegesture) {
		input_report_key(ts->input_dev, KEY_SIDE_GESTURE, 0);
		input_report_key(ts->input_dev, KEY_SIDE_GESTURE_LEFT, 0);
		input_report_key(ts->input_dev, KEY_SIDE_GESTURE_RIGHT, 0);
	}

	input_report_key(ts->input_dev, KEY_HOMEPAGE, 0);
	input_sync(ts->input_dev);

	mutex_unlock(&ts->eventlock);

}

static void sec_ts_read_info_work(struct work_struct *work)
{
	struct sec_ts_data *ts = container_of(work, struct sec_ts_data,
			work_read_info.work);
	int ret;

	mutex_lock(&ts->modechange);

#ifdef TCLM_CONCEPT
	disable_irq(ts->client->irq);

	ret = sec_tclm_check_cal_case(ts->tdata);
	input_info(true, &ts->client->dev, "%s: sec_tclm_check_cal_case ret: %d \n", __func__, ret);

	enable_irq(ts->client->irq);
#endif
	input_log_fix();
	sec_ts_run_rawdata_all(ts, false);
	ts->info_work_done = true;
	sec_ts_sw_reset(ts, SEC_TS_CMD_SW_RESET);

	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: fail to write Sense_on\n", __func__);
	mutex_unlock(&ts->modechange);
}

#ifdef USE_OPEN_CLOSE
static int sec_ts_input_open(struct input_dev *dev)
{
	struct sec_ts_data *ts = input_get_drvdata(dev);
	int ret;

	if (ts == NULL) {
		pr_err("%s %s ts is null\n", SECLOG, __func__);
		return 0;
	}

	if (!ts->info_work_done) {
		input_err(true, &ts->client->dev, "%s not finished info work\n", __func__);
		return 0;
	}

	ts->abc_err_flag = false;
	ts->input_closed = false;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	ret = sec_ts_start_device(ts);
	if (ret < 0)
		input_err(true, &ts->client->dev, "%s: Failed to start device\n", __func__);

	/* because edge and dead zone will recover soon */
	sec_ts_set_grip_type(ts, ONLY_EDGE_HANDLER);

	return 0;
}

static void sec_ts_input_close(struct input_dev *dev)
{
	struct sec_ts_data *ts = input_get_drvdata(dev);

	if (ts == NULL) {
		pr_err("%s %s ts is null\n", SECLOG, __func__);
		return;
	}

	if (!ts->info_work_done) {
		input_err(true, &ts->client->dev, "%s not finished info work\n", __func__);
		return;
	}

	ts->input_closed = true;

	input_info(true, &ts->client->dev, "%s\n", __func__);

#ifdef TCLM_CONCEPT
	sec_tclm_debug_info(ts->tdata);
#endif
#ifdef MINORITY_REPORT
	minority_report_sync_latest_value(ts);
#endif
	sec_ts_stop_device(ts);

	ts->abc_err_flag = true;
}
#endif

#ifdef CONFIG_FB
static int sec_ts_fb_notifier_cb(struct notifier_block *nb, unsigned long event,
					void *data)
{
	struct fb_event *evdata = data;
	struct sec_ts_data *ts_data = container_of(nb, struct sec_ts_data, fb_notifier);
	int *blank = evdata->data;

	if (evdata == NULL) {
		pr_err("%s %s evdata is null\n", SECLOG, __func__);
		return 0;
	}

	if (evdata && evdata->data && ts_data) {
		if (event == FB_EARLY_EVENT_BLANK &&
		    *blank == FB_BLANK_POWERDOWN) {
			sec_ts_input_close(ts_data->input_dev);
			//ts_data->fb_ready = false;
		} else if (event == FB_EVENT_BLANK &&
			   *blank == FB_BLANK_UNBLANK) {
				sec_ts_input_open(ts_data->input_dev);
				//ts_data->fb_ready = true;
		}
	}

	return 0;
}
#endif

static int sec_ts_remove(struct i2c_client *client)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);

	input_info(true, &ts->client->dev, "%s\n", __func__);

	cancel_delayed_work_sync(&ts->work_read_info);
	flush_delayed_work(&ts->work_read_info);

	disable_irq_nosync(ts->client->irq);
	free_irq(ts->client->irq, ts);
	input_info(true, &ts->client->dev, "%s: irq disabled\n", __func__);

	sec_ts_fn_remove(ts);

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
	p_ghost_check = NULL;
#endif
	device_init_wakeup(&client->dev, false);
	wake_lock_destroy(&ts->wakelock);

	dev_set_drvdata(&ts->client->dev, NULL);

	ts->probe_done = false;

	if (ts->plat_data->support_dex) {
		input_mt_destroy_slots(ts->input_dev_pad);
		input_unregister_device(ts->input_dev_pad);
	}

	ts->input_dev = ts->input_dev_touch;
	input_mt_destroy_slots(ts->input_dev);
	input_unregister_device(ts->input_dev);

	ts->input_dev_pad = NULL;
	ts->input_dev = NULL;
	ts->input_dev_touch = NULL;
	ts_dup = NULL;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
	tsp_info = NULL;
#endif

	kfree(ts);
	return 0;
}

static void sec_ts_shutdown(struct i2c_client *client)
{
	struct sec_ts_data *ts = i2c_get_clientdata(client);

	if (ts == NULL) {
		pr_err("%s %s ts is null\n", SECLOG, __func__);
		return;
	}

	input_info(true, &ts->client->dev, "%s\n", __func__);

	sec_ts_remove(client);
}

int sec_ts_stop_device(struct sec_ts_data *ts)
{
	input_info(true, &ts->client->dev, "%s\n", __func__);

	mutex_lock(&ts->device_mutex);

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		input_err(true, &ts->client->dev, "%s: already power off\n", __func__);
		goto out;
	}

	ts->power_status = SEC_TS_STATE_POWER_OFF;

	disable_irq(ts->client->irq);
	sec_ts_locked_release_all_finger(ts);

	if (ts->plat_data->enable_sync)
		ts->plat_data->enable_sync(false);

	/*sec_ts_pinctrl_configure(ts, false);*/

out:
	mutex_unlock(&ts->device_mutex);
	return 0;
}

int sec_ts_start_device(struct sec_ts_data *ts)
{
	int ret;

	input_info(true, &ts->client->dev, "%s\n", __func__);

	/*sec_ts_pinctrl_configure(ts, true);*/

	mutex_lock(&ts->device_mutex);

	if (ts->power_status == SEC_TS_STATE_POWER_ON) {
		input_err(true, &ts->client->dev, "%s: already power on\n", __func__);
		goto out;
	}

	sec_ts_locked_release_all_finger(ts);

	ts->power_status = SEC_TS_STATE_POWER_ON;

	if (ts->plat_data->enable_sync)
		ts->plat_data->enable_sync(true);

	if (ts->flip_enable) {
		ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_COVERTYPE, &ts->cover_cmd, 1);

		ts->touch_functions = ts->touch_functions | SEC_TS_BIT_SETFUNC_COVER;
		input_info(true, &ts->client->dev,
				"%s: cover cmd write type:%d, mode:%x, ret:%d", __func__, ts->touch_functions, ts->cover_cmd, ret);
	} else {
		ts->touch_functions = (ts->touch_functions & (~SEC_TS_BIT_SETFUNC_COVER));
		input_info(true, &ts->client->dev,
				"%s: cover open, not send cmd", __func__);
	}

	ts->touch_functions = ts->touch_functions | SEC_TS_DEFAULT_ENABLE_BIT_SETFUNC;
	ret = sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHFUNCTION, (u8 *)&ts->touch_functions, 2);
	if (ret < 0)
		input_err(true, &ts->client->dev,
				"%s: Failed to send touch function command", __func__);

	sec_ts_set_grip_type(ts, ONLY_EDGE_HANDLER);

	if (ts->dex_mode) {
		input_info(true, &ts->client->dev, "%s: set dex mode\n", __func__);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_DEX_MODE, &ts->dex_mode, 1);
		if (ret < 0)
			input_err(true, &ts->client->dev,
					"%s: failed to set dex mode %x\n", __func__, ts->dex_mode);
	}

	if (ts->brush_mode) {
		input_info(true, &ts->client->dev, "%s: set brush mode\n", __func__);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_BRUSH_MODE, &ts->brush_mode, 1);
		if (ret < 0)
			input_err(true, &ts->client->dev,
					"%s: failed to set brush mode\n", __func__);
	}

	if (ts->touchable_area) {
		input_info(true, &ts->client->dev, "%s: set 16:9 mode\n", __func__);
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SET_TOUCHABLE_AREA, &ts->touchable_area, 1);
		if (ret < 0)
			input_err(true, &ts->client->dev,
					"%s: failed to set 16:9 mode\n", __func__);
	}
	enable_irq(ts->client->irq);

out:
	mutex_unlock(&ts->device_mutex);
	return 0;
}

#ifdef CONFIG_PM
static int sec_ts_pm_suspend(struct device *dev)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);
#ifdef USE_OPEN_CLOSE
	int retval;
#endif

	if (ts == NULL) {
		pr_err("%s %s ts is null\n", SECLOG, __func__);
		return 0;
	}

#ifdef USE_OPEN_CLOSE
	if (ts->input_dev) {
		retval = mutex_lock_interruptible(&ts->input_dev->mutex);
		if (retval) {
			input_err(true, &ts->client->dev,
					"%s : mutex error\n", __func__);
			goto out;
		}

		if (!ts->input_dev->disabled) {
			ts->input_dev->disabled = true;
			if (ts->input_dev->users && ts->input_dev->close) {
				input_err(true, &ts->client->dev,
						"%s called without input_close\n",
						__func__);
				ts->input_dev->close(ts->input_dev);
#ifdef CONFIG_SEC_ABC
				sec_abc_send_event("MODULE=tsp@ERROR=suspend_without_input_close");
#endif
			}
			ts->input_dev->users = 0;
		}

		mutex_unlock(&ts->input_dev->mutex);
	}

out:
#endif
	return 0;
}

static int sec_ts_pm_resume(struct device *dev)
{
	struct sec_ts_data *ts = dev_get_drvdata(dev);

	if (ts == NULL) {
		pr_err("%s %s ts is null\n", SECLOG, __func__);
		return 0;
	}
	return 0;
}
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
void trustedui_mode_on(void)
{
	if (!tsp_info)
		return;

	sec_ts_unlocked_release_all_finger(tsp_info);
}
EXPORT_SYMBOL(trustedui_mode_on);

void trustedui_mode_off(void)
{
	if (!tsp_info)
		return;
}
EXPORT_SYMBOL(trustedui_mode_off);
#endif

static const struct i2c_device_id sec_ts_id[] = {
	{ SEC_TS_I2C_NAME, 0 },
	{ },
};

#ifdef CONFIG_PM
static const struct dev_pm_ops sec_ts_dev_pm_ops = {
	.suspend = sec_ts_pm_suspend,
	.resume = sec_ts_pm_resume,
};
#endif

#ifdef CONFIG_OF
static const struct of_device_id sec_ts_match_table[] = {
	{ .compatible = "sec,sec_ts",},
	{ },
};
#else
#define sec_ts_match_table NULL
#endif

static struct i2c_driver sec_ts_driver = {
	.probe		= sec_ts_probe,
	.remove		= sec_ts_remove,
	.shutdown	= sec_ts_shutdown,
	.id_table	= sec_ts_id,
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SEC_TS_I2C_NAME,
#ifdef CONFIG_OF
		.of_match_table = sec_ts_match_table,
#endif
#ifdef CONFIG_PM
		.pm = &sec_ts_dev_pm_ops,
#endif
	},
};

static int __init sec_ts_init(void)
{

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge == 1) {
		pr_err("%s %s: Do not load driver due to : lpm %d\n",
				SECLOG, __func__, lpcharge);
		return -ENODEV;
	}
#endif

	pr_err("%s %s\n", SECLOG, __func__);

	return i2c_add_driver(&sec_ts_driver);
}

static void __exit sec_ts_exit(void)
{
	i2c_del_driver(&sec_ts_driver);
}

MODULE_AUTHOR("Hyobae, Ahn<hyobae.ahn@samsung.com>");
MODULE_DESCRIPTION("Samsung Electronics TouchScreen driver");
MODULE_LICENSE("GPL");

late_initcall_sync(sec_ts_init);
module_exit(sec_ts_exit);
