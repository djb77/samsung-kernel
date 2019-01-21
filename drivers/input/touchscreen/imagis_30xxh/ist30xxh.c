/*
 *  Copyright (C) 2010,Imagis Technology Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/time.h>

#include <asm/unaligned.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/err.h>

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/input/mt.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#endif

#include "ist30xxh.h"
#include "ist30xxh_misc.h"
#include "ist30xxh_update.h"
#ifdef IST30XX_USE_CMCS
#include "ist30xxh_cmcs.h"
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
#include <linux/trustedui.h>
#endif

#ifdef IST30XX_USE_KEY
int ist30xx_key_code[] = IST30XX_KEY_CODES;
#endif

struct ist30xx_data *ts_data;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
struct ist30xx_data *tui_tsp_info;
extern int tui_force_close(uint32_t arg);
#endif

#ifdef TCLM_CONCEPT
	/* TCLM_CONCEPT */
static struct sec_cal_position sec_cal_positions[16] = {
	{CAL_POS_CMD("NONE",	'N'),}, /* 0, NONe */
	{CAL_POS_CMD("INIT",	'I'),},	/* 1, INIT case, calcount is 00 or FF */
	{CAL_POS_CMD("FACT",	'F'),},	/* 2, FACTory line, run_force_calibration without value */
	{CAL_POS_CMD("OUTS",	'O'),},	/* 3, OUTSide of factory */
	{CAL_POS_CMD("LCIA",	'L'),},	/* 4, LCIA in factory line */
	{CAL_POS_CMD("CENT",	'C'),},	/* 5, svc CENTer, cal from service center */
	{CAL_POS_CMD("ABNO",	'A'),},	/* 6, ABNOrmal case */
	{CAL_POS_CMD("BOOT",	'B'),},	/* 7, BOOT firmup, when firmup in booting time */
	{CAL_POS_CMD("SPEC",	'S'),},	/* 8, SPECout case */
	{CAL_POS_CMD("TUNE",	'V'),},	/* 9, TUNE Version up, when afe version is lage than tune version */
	{CAL_POS_CMD("EVER",	'E'),},	/* 10, EVERytime, always cal in Booting */
	{CAL_POS_CMD("TEST",	'T'),},	/* 11, TESTmode, firmup case in *#2663# */
	{CAL_POS_CMD("UNDE",	'U'),},	/* 12, UNDEfine, undefined value */
	{CAL_POS_CMD("UNDE",	'U'),},	/* 13 */
	{CAL_POS_CMD("UNDE",	'U'),},	/* 14 */
	{CAL_POS_CMD("UNDE",	'U'),}	/* 15 */
};
#endif

int ist30xx_log_level = IST30XX_LOG_LEVEL;
void tsp_printk(int level, const char *fmt, ...)
{
    struct va_format vaf;
    va_list args;

    if (ist30xx_log_level < level)
        return;

    va_start(args, fmt);

    vaf.fmt = fmt;
    vaf.va = &args;

    printk("%s %pV", IST30XX_LOG_TAG, &vaf);

    va_end(args);
}

long get_milli_second(struct ist30xx_data *data)
{
    ktime_get_ts(&data->t_current);

    return data->t_current.tv_sec * 1000 + data->t_current.tv_nsec / 1000000;
}

void ist30xx_delay(unsigned int ms)
{
    if (ms < 20)
        usleep_range(ms * 1000, ms * 1000);
    else
        msleep(ms);
}

int ist30xx_intr_wait(struct ist30xx_data *data, long ms)
{
    long start_ms = get_milli_second(data);
    long curr_ms = 0;

    while (1) {
        if (!data->irq_working)
            break;

        curr_ms = get_milli_second(data);
        if ((curr_ms < 0) || (start_ms < 0) || (curr_ms - start_ms > ms)) {
            tsp_info("%s() timeout(%dms)\n", __func__, ms);
            return -EPERM;
        }

        ist30xx_delay(2);
    }
    return 0;
}

void ist30xx_disable_irq(struct ist30xx_data *data)
{
    if (likely(data->irq_enabled)) {
        disable_irq(data->client->irq);
        data->irq_enabled = false;
        data->status.event_mode = false;
    }
}

void ist30xx_enable_irq(struct ist30xx_data *data)
{
    if (likely(!data->irq_enabled)) {
        enable_irq(data->client->irq);
        ist30xx_delay(10);
        data->irq_enabled = true;
        data->status.event_mode = true;
    }
}

void ist30xx_scheduled_reset(struct ist30xx_data *data)
{
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
    if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
        tsp_err("%s return, TUI is enabled!\n", __func__);
        return;
    }
#endif
    if (likely(data->initialized))
        schedule_delayed_work(&data->work_reset_check, 0);
}

static void ist30xx_request_reset(struct ist30xx_data *data)
{
    data->irq_err_cnt++;
    if (unlikely(data->irq_err_cnt >= data->max_irq_err_cnt)) {
        tsp_info("%s()\n", __func__);
        ist30xx_scheduled_reset(data);
        data->irq_err_cnt = 0;
    }
}

#define NOISE_MODE_TA       (0)
#define NOISE_MODE_CALL     (1)
#define NOISE_MODE_COVER    (2)
#define NOISE_MODE_EDGE     (4)
#define NOISE_MODE_POWER    (8)
void ist30xx_start(struct ist30xx_data *data)
{
    if (data->initialized) {
        data->scan_count = 0;
        data->scan_retry = 0;
        mod_timer(&data->event_timer,
                get_jiffies_64() + EVENT_TIMER_INTERVAL * 2);
    }
    
    data->ignore_delay = true;

    /* TA mode */
    ist30xx_write_cmd(data, IST30XX_HIB_CMD,
            ((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));

    if (data->report_rate >= 0) {
        ist30xx_write_cmd(data, IST30XX_HIB_CMD, 
                ((eHCOM_SET_TIME_ACTIVE << 16) | (data->report_rate & 0xFFFF)));
        tsp_info("%s: active rate : %dus\n", __func__, data->report_rate);
    }

    if (data->idle_rate >= 0) {
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                ((eHCOM_SET_TIME_IDLE << 16) | (data->idle_rate & 0xFFFF)));
        tsp_info("%s: idle rate : %dus\n", __func__, data->idle_rate);
    }

    if (data->jig_mode) {
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SET_JIG_MODE << 16) | (IST30XX_JIG_TOUCH & 0xFFFF));
        tsp_info("%s: jig mode start\n", __func__);
    }

    if (data->rec_mode) {
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SET_REC_MODE << 16) | (IST30XX_ENABLE & 0xFFFF));
        tsp_info("%s: rec mode start\n", __func__);
    }

    if (data->debugging_mode) {
        data->debugging_scancnt = 0;
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SET_DBG_MODE << 16) | (IST30XX_ENABLE & 0xFFFF));
        tsp_info("%s: debugging mode start\n", __func__);
    }

    if (data->debug_mode || data->jig_mode || data->rec_mode ||
            data->debugging_mode) {
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (IST30XX_DISABLE & 0xFFFF));
        tsp_info("%s: nosleep mode start\n", __func__);
    }
    
    ist30xx_cmd_start_scan(data);

    data->ignore_delay = false;

    if (data->rec_mode) {
        ist30xx_delay(100);
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SET_REC_MODE << 16) | (IST30XX_START_SCAN & 0xFFFF));
    }

    tsp_info("%s, mode : 0x%x\n", __func__, data->noise_mode & 0xFFFF);
}

int ist30xx_set_input_device(struct ist30xx_data *data)
{
    int ret;

    data->input_dev->name = "sec_touchscreen";
    data->input_dev->id.bustype = BUS_I2C;
    data->input_dev->dev.parent = &data->client->dev;

    input_mt_init_slots(data->input_dev, IST30XX_MAX_FINGERS, 0);

    set_bit(EV_SYN, data->input_dev->evbit);
    set_bit(EV_ABS, data->input_dev->evbit);
    set_bit(EV_KEY, data->input_dev->evbit);
    set_bit(INPUT_PROP_DIRECT, data->input_dev->propbit);

    input_set_abs_params(data->input_dev, ABS_MT_PALM, 0, 1, 0, 0);
    input_set_abs_params(data->input_dev, ABS_MT_POSITION_X, 0,
            data->tsp_info.width - 1, 0, 0);
    input_set_abs_params(data->input_dev, ABS_MT_POSITION_Y, 0,
            data->tsp_info.height - 1, 0, 0);
    input_set_abs_params(data->input_dev, ABS_MT_TOUCH_MAJOR, 0,
            IST30XX_MAX_MAJOR, 0, 0);
    input_set_abs_params(data->input_dev, ABS_MT_TOUCH_MINOR, 0,
            IST30XX_MAX_MINOR, 0, 0);
#ifdef CONFIG_SEC_FACTORY
    input_set_abs_params(data->input_dev, ABS_MT_PRESSURE, 0, IST30XX_MAX_Z, 0,
            0);
#endif

#ifdef IST30XX_USE_KEY
    {
        int i;
        for (i = 0; i < ARRAY_SIZE(ist30xx_key_code); i++)
            set_bit(ist30xx_key_code[i], data->input_dev->keybit);
    }
#endif
    input_set_capability(data->input_dev, EV_KEY, KEY_BLACK_UI_GESTURE);

    input_set_drvdata(data->input_dev, data);
    ret = input_register_device(data->input_dev);
    
    tsp_info("%s: input register device:%d\n", __func__, ret);

    return ret;
}

int ist30xx_get_info(struct ist30xx_data *data)
{
    int ret;
    u32 calib_msg;

    ist30xx_disable_irq(data);

    ist30xx_write_cmd(data, IST30XX_HIB_CMD,
            (eHCOM_FW_HOLD << 16) | (IST30XX_ENABLE & 0xFFFF));
    ist30xx_delay(40);

    mutex_lock(&data->lock);
    ret = ist30xx_get_tsp_info(data);
    if (unlikely(ret)) {
        mutex_unlock(&data->lock);
        goto err_get_info;
    }
    mutex_unlock(&data->lock);

    ist30xx_print_info(data);

    ret = ist30xx_set_input_device(data);
    if (unlikely(ret))
        goto err_get_info;

    ist30xx_read_cmd(data, eHCOM_GET_SLF_CAL_RESULT, &calib_msg);
    tsp_info("SLF calib result: 0x%08X\n", calib_msg);
    ist30xx_read_cmd(data, eHCOM_GET_CAL_RESULT, &calib_msg);
    tsp_info("MTL calib result: 0x%08X\n", calib_msg);

    ret = ist30xx_write_cmd(data, IST30XX_HIB_CMD,
            (eHCOM_FW_HOLD << 16) | (IST30XX_DISABLE & 0xFFFF));
    if (ret) {
        tsp_warn("%s fail to disable hold\n", __func__);
        mutex_lock(&data->lock);
        ist30xx_reset(data, false);
        mutex_unlock(&data->lock);
    }

    ist30xx_enable_irq(data);

    return 0;

err_get_info:

    return ret;
}

#define SPECIAL_MAGIC_STRING        (0x4170CF00)
#define SPECIAL_MAGIC_MASK          (0xFFFFFF00)
#define SPECIAL_MESSAGE_MASK        (~SPECIAL_MAGIC_MASK)
#define PARSE_SPECIAL_MESSAGE(n)  \
        ((n & SPECIAL_MAGIC_MASK) == SPECIAL_MAGIC_STRING ? \
        (n & SPECIAL_MESSAGE_MASK) : -EINVAL)
#define SPECIAL_START_MASK          (0x80)
#define SPECIAL_SUCCESS_MASK        (0x0F)
void ist30xx_special_cmd(struct ist30xx_data *data, int cmd)
{
    if (cmd == 0) {
        ist30xx_burst_read(data->client, IST30XX_HIB_GESTURE_REG,
                data->g_reg.full, sizeof(data->g_reg.full) / IST30XX_DATA_LEN,
                true);

        tsp_debug("0x%02X, 0x%02X, %d, %d, %d, %d, %d, %d, 0x%02X\n",
                data->g_reg.b.ctrl, data->g_reg.b.evt, data->g_reg.b.w,
                data->g_reg.b.h, data->g_reg.b.x, data->g_reg.b.y,
                data->g_reg.b.evt_x, data->g_reg.b.evt_y,
                data->g_reg.b.setting);
 
        if (data->g_reg.b.evt & IST30XX_GETURE_EVT_SPAY) {
            tsp_noti("SPAY Trigger~\n");

            data->scrub_id = SPONGE_EVENT_TYPE_SPAY;
            data->scrub_x = 0;
            data->scrub_y = 0;
        
            input_report_key(data->input_dev, KEY_BLACK_UI_GESTURE, 1);
            input_sync(data->input_dev);
            input_report_key(data->input_dev, KEY_BLACK_UI_GESTURE, 0);
            input_sync(data->input_dev);
        } else if (data->g_reg.b.evt & IST30XX_GETURE_EVT_AOD) {
            tsp_noti("AOD Trigger~ (%d, %d)\n", data->g_reg.b.evt_x,
                    data->g_reg.b.evt_y);
            data->g_reg.b.evt = 0;
            ist30xx_burst_write(data->client, IST30XX_HIB_GESTURE_REG,
                    data->g_reg.full, 1);

            data->scrub_id = SPONGE_EVENT_TYPE_AOD_DOUBLETAB;
            data->scrub_x = data->g_reg.b.evt_x;
            data->scrub_y = data->g_reg.b.evt_y;

            input_report_key(data->input_dev, KEY_BLACK_UI_GESTURE, 1);
            input_sync(data->input_dev);
            input_report_key(data->input_dev, KEY_BLACK_UI_GESTURE, 0);
            input_sync(data->input_dev);
        }

        return;
    }

    tsp_warn("Not support gesture cmd: 0x%02X\n", cmd);
}

void location_detect (struct ist30xx_data *data, char *loc, int x, int y)
{
    if (x < data->dt_data->area_edge)
        strcat(loc, "E.");
    else if (x < (data->dt_data->x_max - data->dt_data->area_edge))
        strcat(loc, "C.");
    else
        strcat(loc, "e.");

    if (y < data->dt_data->area_indicator)
        strcat(loc, "S");
    else if (y < (data->dt_data->y_max - data->dt_data->area_navigation))
        strcat(loc, "C");
    else
        strcat(loc, "N");
}

#define PRESS_MSG_MASK          (0x01)
#define MULTI_MSG_MASK          (0x02)
#define TOUCH_DOWN_MESSAGE      ("p")
#define TOUCH_UP_MESSAGE        ("r")
#define TOUCH_MOVE_MESSAGE      (" ")
void print_tsp_event(struct ist30xx_data *data, int id, finger_info *finger)
{
    bool press;
    int ret = 0;
    u32 status = 0;
    u8 mode = TOUCH_STATUS_NORMAL_MODE;
    char location[6] = "xy:";

    press = PRESSED_FINGER(data->t_status, id);

    if (press) {
        data->move_count[id]++;
        if (data->tsp_touched[id] == false) {
            /* touch down */
            ret = ist30xx_read_reg(data->client, IST30XX_HIB_TOUCH_STATUS,
                    &status);
            if (ret == 0) {
                if ((status & TOUCH_STATUS_MASK) == TOUCH_STATUS_MAGIC) {
                    if (GET_NOISE_MODE(status))
                        mode |= TOUCH_STATUS_NOISE_MODE;
                    if (GET_WET_MODE(status))
                        mode |= TOUCH_STATUS_WET_MODE;
                } 
            }
            data->touch_pressed_num++;
            /*for getting coordinate of pressed point*/
            data->pressed_x[id] = data->released_x[id] = finger->bit_field.x;
            data->pressed_y[id] = data->released_y[id] = finger->bit_field.y;
            location_detect(data, location, data->pressed_x[id], data->pressed_y[id]);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
            input_info(true, &data->client->dev, "%s%d (%d, %d) %s (p:%d, ma:%d, mi:%d) (%d)\n",
                    TOUCH_DOWN_MESSAGE, id + 1,
                    finger->bit_field.x, finger->bit_field.y, location, PARSE_PALM_STATUS(data->t_status),
                    finger->bit_field.ma, finger->bit_field.mi, mode);
            data->tsp_touched[id] = true;
#else
            input_info(true, &data->client->dev, "%s%d %s (p:%d, ma:%d, mi:%d) (%d)\n",
                    TOUCH_DOWN_MESSAGE, id + 1, location, PARSE_PALM_STATUS(data->t_status),
                    finger->bit_field.ma,finger->bit_field.mi, mode);
            data->tsp_touched[id] = true;
#endif
            do_gettimeofday(&data->time_pressed[id]);
            data->all_finger_count++;

        } else {
            /* touch move */
            /*for getting coordinate of the last point of move event*/
            data->released_x[id] = finger->bit_field.x;
            data->released_y[id] = finger->bit_field.y;
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
            tsp_debug("%s%d (%d, %d) (p:%d, ma:%d, mi:%d)\n",
                    TOUCH_MOVE_MESSAGE, id + 1,
                    finger->bit_field.x, finger->bit_field.y,
                    PARSE_PALM_STATUS(data->t_status), finger->bit_field.ma,
                    finger->bit_field.mi);
#else
            tsp_debug("%s%d (p:%d, ma:%d, mi:%d)\n",
                    TOUCH_MOVE_MESSAGE, id + 1,
                    PARSE_PALM_STATUS(data->t_status), finger->bit_field.ma,
                    finger->bit_field.mi);
#endif
        }
    } else {
        if (data->tsp_touched[id] == true) {
            /* touch up */
            data->touch_pressed_num--;
            location_detect(data, location, data->released_x[id], data->released_y[id]);
#ifdef TCLM_CONCEPT
            input_info(true, &data->client->dev, "%s%d %s dX,dY:%d,%d mc:%d (0x%02x) (%x,C%02XT%04X.%4s%s)\n", TOUCH_UP_MESSAGE,
                    id + 1, location, (data->released_x[id] - data->pressed_x[id]),
                    (data->released_y[id] - data->pressed_y[id]),
                    data->move_count[id], data->fw.cur.fw_ver,
                    data->test_result.data[0], data->cal_count, data->tune_fix_ver,
                    data->tclm_string[data->cal_position].f_name,
                    (data->dt_data->tclm_level== TCLM_LEVEL_LOCKDOWN)?".L":" ");
#else
            input_info(true, &data->client->dev, "%s%d %s dX,dY:%d,%d mc:%d (0x%02x)\n", TOUCH_UP_MESSAGE,
                    id + 1, location, (data->released_x[id] - data->pressed_x[id]),
                    (data->released_y[id] - data->pressed_y[id]),
                    data->move_count[id], data->fw.cur.fw_ver);
#endif
            data->tsp_touched[id] = false;
            do_gettimeofday(&data->time_released[id]);
            if (data->time_longest < (data->time_released[id].tv_sec - data->time_pressed[id].tv_sec))
                data->time_longest = (data->time_released[id].tv_sec - data->time_pressed[id].tv_sec);
        }
        data->move_count[id] = 0;
    }

    if ((data->touch_pressed_num > 2) && (data->check_multi == 0)) {
        data->check_multi = 1;
        data->multi_count++;
    }

    if(data->touch_pressed_num == 0)
        data->check_multi = 0;    
}
#ifdef IST30XX_USE_KEY
#define PRESS_MSG_KEY       (0x06)
void print_tkey_event(struct ist30xx_data *data, int id)
{
    bool press = PRESSED_KEY(data->t_status, id);

    if (press) {
        if (data->tkey_pressed[id] == false) {
            /* tkey down */
            tsp_noti("k %s%d\n", TOUCH_DOWN_MESSAGE, id + 1);
            data->tkey_pressed[id] = true;
        }
    } else {
        if (data->tkey_pressed[id] == true) {
            /* tkey up */
            tsp_noti("k %s%d\n", TOUCH_UP_MESSAGE, id + 1);
            data->tkey_pressed[id] = false;
        }
    }
}
#endif
static void release_finger(struct ist30xx_data *data, int id)
{
    input_mt_slot(data->input_dev, id);
    input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);

    tsp_info("forced touch release: %d\n", id + 1);

    data->tsp_touched[id] = false;
    if (data->debugging_mode && (id < 2))
        data->t_frame[id] = 0;

    do_gettimeofday(&data->time_released[id]);

    if (data->time_longest < (data->time_released[id].tv_sec - data->time_pressed[id].tv_sec))
        data->time_longest = (data->time_released[id].tv_sec - data->time_pressed[id].tv_sec);

    input_sync(data->input_dev);
    data->touch_pressed_num = 0;
    data->check_multi = 0;
}

#ifdef IST30XX_USE_KEY
static void release_key(struct ist30xx_data *data, int id)
{
    input_report_key(data->input_dev, ist30xx_key_code[id], false);

    tsp_info("forced key release: %d\n", id + 1);

    data->tkey_pressed[id] = false;

    input_sync(data->input_dev);
}
#endif
static void clear_input_data(struct ist30xx_data *data)
{
    int id = 0;
    u32 status;

    status = PARSE_FINGER_STATUS(data->t_status);
    while (status) {
        if (status & 1)
            release_finger(data, id);
        status >>= 1;
        id++;
    }
#ifdef IST30XX_USE_KEY
    id = 0;
    status = PARSE_KEY_STATUS(data->t_status);
    while (status) {
        if (status & 1)
            release_key(data, id);
        status >>= 1;
        id++;
    }
#endif

    data->t_status = 0;
}

static int check_valid_coord(u32 *msg, int cnt)
{
    u8 *buf = (u8 *)msg;
    u8 chksum1 = msg[0] >> 24;
    u8 chksum2 = 0;
    u32 tmp = msg[0];

    msg[0] &= 0x00FFFFFF;

    cnt *= IST30XX_DATA_LEN;

    while (cnt--)
        chksum2 += *buf++;

    msg[0] = tmp;

    if (chksum1 != chksum2) {
        tsp_err("intr chksum: %02x, %02x\n", chksum1, chksum2);
        return -EPERM;
    }

    return 0;
}

static void report_input_data(struct ist30xx_data *data, int finger_counts,
        int key_counts)
{
    int id;
    bool press = false;
    finger_info *fingers = (finger_info *)data->fingers;
    u32 *z_values = (u32 *)data->z_values;
    int idx = 0;
    u32 status;

    memset(data->t_frame, 0, sizeof(data->t_frame));

    status = PARSE_FINGER_STATUS(data->t_status);
    for (id = 0; id < IST30XX_MAX_FINGERS; id++) {
        press = (status & (1 << id)) ? true : false;

        input_mt_slot(data->input_dev, id);
        if (press == false) {
            if (data->jig_mode)
                input_report_abs(data->input_dev, ABS_MT_PRESSURE, 0);
        }
        input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, press);

        print_tsp_event(data, id, &fingers[idx]);

        if (press == false)
            continue;

        if (data->debugging_mode && (id < 2))
            data->t_frame[id] = fingers[idx].full_field;

        input_report_abs(data->input_dev, ABS_MT_PALM,
                PARSE_PALM_STATUS(data->t_status));
        input_report_abs(data->input_dev, ABS_MT_POSITION_X,
                fingers[idx].bit_field.x);
        input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
                fingers[idx].bit_field.y);
        input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
                fingers[idx].bit_field.ma);
        input_report_abs(data->input_dev, ABS_MT_TOUCH_MINOR,
                fingers[idx].bit_field.mi);
        if (data->jig_mode) {
            if(z_values[idx] < 1)
                z_values[idx] = 1;
            input_report_abs(data->input_dev, ABS_MT_PRESSURE, z_values[idx]);
        }
        idx++;
    }

#ifdef IST30XX_USE_KEY
    status = PARSE_KEY_STATUS(data->t_status);
    for (id = 0; id < ARRAY_SIZE(ist30xx_key_code); id++) {
        press = (status & (1 << id)) ? true : false;
        input_report_key(data->input_dev, ist30xx_key_code[id], press);
        print_tkey_event(data, id);
    }
#endif

    data->irq_err_cnt = 0;
    data->scan_retry = 0;

    input_sync(data->input_dev);
}

void recording_data(struct ist30xx_data *data, bool idle)
{
    int ret;
    u32 scancnt = 0;
    u32 addr = 0;
    u32 *buf32;
    TSP_INFO *tsp = &data->tsp_info;

    if (idle && (data->rec_mode > 1))
        goto state_idle;

    ist30xx_delay(data->rec_delay);

    ret = ist30xx_read_reg(data->client, IST30XX_HIB_TOUCH_STATUS,
            &scancnt);
    if (ret) {
        tsp_err("%s failed to read scancnt\n", __func__);
        goto state_idle;
    }

    if (data->recording_scancnt == scancnt) {
        tsp_warn("%s same recording scancnt\n", __func__);
        goto state_idle;
    }

    data->recording_scancnt = scancnt;

    buf32 = kzalloc(data->rec_size + (IST30XX_MAX_SELF_NODE_NUM +
                IST30XX_MAX_NODE_NUM) * sizeof(u32), GFP_KERNEL);
    if (unlikely(!buf32)) {
        tsp_err("%s failed to allocate\n", __func__);
        goto state_idle;
    }

    if (data->rec_size > 0) {
        addr = IST30XX_DA_ADDR(data->rec_addr);
        ret = ist30xx_burst_read(data->client, addr, buf32,
                data->rec_size / IST30XX_DATA_LEN, true);
        if (ret)
            goto err_rec_fail;
    }

    addr = IST30XX_DA_ADDR(data->self_cdc_addr);
    ret = ist30xx_burst_read(data->client, addr,
            buf32 + (data->rec_size / IST30XX_DATA_LEN), tsp->node.self_len,
            true);
    if (ret)
        goto err_rec_fail;

    addr = IST30XX_DA_ADDR(data->cdc_addr);
    ret = ist30xx_burst_read(data->client, addr,
            buf32 + (data->rec_size / IST30XX_DATA_LEN) +
            IST30XX_MAX_SELF_NODE_NUM, tsp->node.len, true);
    if (ret)
        goto err_rec_fail;

    ist30xx_recording_put_frame(data, buf32,
            (data->rec_size / IST30XX_DATA_LEN) + IST30XX_MAX_SELF_NODE_NUM +
            IST30XX_MAX_NODE_NUM);

err_rec_fail:
    kfree(buf32);

state_idle:
    data->ignore_delay = true;
    ist30xx_write_cmd(data, IST30XX_HIB_CMD,
            (eHCOM_SET_REC_MODE << 16) | (IST30XX_START_SCAN & 0xFFFF));
    data->ignore_delay = false;
}

/*
 * CMD : CMD_GET_COORD
 *
 *   1st  [31:24]   [23:21]   [20:16]   [15:12]   [11]  [10]   [9:0]
 *        Checksum  KeyCnt    KeyStatus FingerCnt Rsvd. Palm   FingerStatus
 *   2nd  [31:28]   [27:24]   [23:12]   [11:0]
 *        Major     Minor     X         Y
 */
static irqreturn_t ist30xx_irq_thread(int irq, void *ptr)
{
    int i;
    int ret = 0;
    int key_cnt, finger_cnt, read_cnt;
    struct ist30xx_data *data = (struct ist30xx_data *)ptr;
    int offset = 1;
    u32 t_status;
    u32 msg[IST30XX_MAX_FINGERS * 2 + offset];
    u32 *buf32;
    u32 debugBuf32[IST30XX_MAX_DEBUGINFO / IST30XX_DATA_LEN];
    u32 ms;
    u32 touch[2];
    bool idle = false;

    data->irq_working = true;

    memset(msg, 0, sizeof(msg));

    if (unlikely(!data->irq_enabled))
        goto irq_ignore;

    if (data->spay || data->aod){
        pm_wakeup_event(data->input_dev->dev.parent, 2000);
    }

    ms = get_milli_second(data);

    if (data->debugging_mode) {
        ist30xx_burst_read(data->client, 
                IST30XX_DA_ADDR(data->debugging_addr), debugBuf32,
                data->debugging_size / IST30XX_DATA_LEN, true);
    }

    if (data->intr_debug1_size > 0) {
        buf32 = kzalloc(data->intr_debug1_size * sizeof(u32), GFP_KERNEL);
        if (unlikely(!buf32)) {
            tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
            goto irq_err;
        }

        tsp_debug("Intr_debug1 (addr: 0x%08x)\n", data->intr_debug1_addr);
        ist30xx_burst_read(data->client, 
                IST30XX_DA_ADDR(data->intr_debug1_addr), buf32,
                data->intr_debug1_size, true);

        for (i = 0; i < data->intr_debug1_size; i++)
            tsp_debug("\t%08x\n", buf32[i]);
        kfree(buf32);
    }

    if (data->intr_debug2_size > 0) {
        buf32 = kzalloc(data->intr_debug2_size * sizeof(u32), GFP_KERNEL);
        if (unlikely(!buf32)) {
            tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
            goto irq_err;
        }

        tsp_debug("Intr_debug2 (addr: 0x%08x)\n", data->intr_debug2_addr);
        ist30xx_burst_read(data->client,
                IST30XX_DA_ADDR(data->intr_debug2_addr), buf32,
                data->intr_debug2_size, true);

        for (i = 0; i < data->intr_debug2_size; i++)
            tsp_debug("\t%08x\n", buf32[i]);
        kfree(buf32);        
    }

    ret = ist30xx_read_reg(data->client, IST30XX_HIB_INTR_MSG, msg);
    if (unlikely(ret))
        goto irq_err;

    tsp_verb("intr msg: 0x%08x\n", *msg);

    /* TSP End Initial */
    if (unlikely(*msg == IST30XX_INITIAL_VALUE)) {
        tsp_debug("IC Ready~\n");
        goto irq_event;
    }

    /* TSP Recording */
    if (unlikely(*msg == IST30XX_REC_VALUE)) {
        idle = true;
        goto irq_end;
    }

    /* TSP Debugging */
    if (unlikely(*msg == IST30XX_DEBUGGING_VALUE))
        goto irq_end;

    /* TSP IC Exception */
    if (unlikely((*msg & IST30XX_EXCEPT_MASK) == IST30XX_EXCEPT_VALUE)) {
        tsp_err("Occurred IC exception(0x%02X)\n", *msg & 0xFF);
        ret = ist30xx_burst_read(data->client, IST30XX_HIB_COORD,
                &msg[offset], IST30XX_MAX_EXCEPT_SIZE, true);
        if (unlikely(ret))
            tsp_err(" exception value read error(%d)\n", ret);
        else
            tsp_err(" exception value : 0x%08X, 0x%08X\n", msg[1], msg[2]);

        goto irq_ic_err;
    }

    if (unlikely(*msg == 0 || *msg == 0xFFFFFFFF)) /* Unknown CMD */
        goto irq_err;

    if (unlikely((*msg & CALIB_MSG_MASK) == CALIB_MSG_VALID)) {
        if (data->status.calib == 1) {
            ret = ist30xx_burst_read(data->client, IST30XX_HIB_INTR_MSG,
                data->status.calib_msg, IST30XX_MAX_CALIB_SIZE, true);
            tsp_info("Auto calibration\n");
            tsp_info("SLF calib status:0x%08X\n", data->status.calib_msg[0]);
            tsp_info("MTL calib status:0x%08X\n", data->status.calib_msg[1]);
            tsp_info("MAX CH status   :0x%08X\n", data->status.calib_msg[2]);
        } else if (data->status.miscalib == 1) {
            ret = ist30xx_burst_read(data->client, IST30XX_HIB_INTR_MSG,
                data->status.miscalib_msg, IST30XX_MAX_CALIB_SIZE, true);
            tsp_info("Mis calibration\n");
            tsp_info("SLF calib status:0x%08X\n", data->status.miscalib_msg[0]);
            tsp_info("MTL calib status:0x%08X\n", data->status.miscalib_msg[1]);
            tsp_info("MAX CH status   :0x%08X\n", data->status.miscalib_msg[2]);
        }
        goto irq_event;
    }

#ifdef IST30XX_USE_CMCS
    if (((*msg & CMCS_MSG_MASK) == CM_MSG_VALID) ||
        ((*msg & CMCS_MSG_MASK) == CS_MSG_VALID) ||
        ((*msg & CMCS_MSG_MASK) == CMJIT_MSG_VALID) ||
        ((*msg & CMCS_MSG_MASK) == INT_MSG_VALID) ||
        ((*msg & CMCS_MSG_MASK) == HVDD_MSG_VALID)) {
        data->status.cmcs = *msg;
        tsp_info("CMCS notify: 0x%08X\n", *msg);

        goto irq_event;
    }
#endif

    ret = PARSE_SPECIAL_MESSAGE(*msg);
    if (unlikely(ret >= 0)) {
        tsp_debug("special cmd: %d (0x%08X)\n", ret, *msg);
        mutex_lock(&data->aod_lock);
        ist30xx_special_cmd(data, ret);
        mutex_unlock(&data->aod_lock);

        goto irq_event;
    }

    memset(data->fingers, 0, sizeof(data->fingers));

    /* Unknown interrupt data for extend coordinate */
    if (unlikely(!CHECK_INTR_STATUS(*msg)))
        goto irq_err;

    t_status = *msg;
    key_cnt = PARSE_KEY_CNT(t_status);
    finger_cnt = PARSE_FINGER_CNT(t_status);

    if (unlikely((finger_cnt > data->max_fingers) ||
                (key_cnt > data->max_keys))) {
        tsp_warn("Invalid touch count - finger: %d(%d), key: %d(%d)\n",
                finger_cnt, data->max_fingers, key_cnt, data->max_keys);
        goto irq_err;
    }

    if (finger_cnt > 0) {
        ret = ist30xx_burst_read(data->client, IST30XX_HIB_COORD, &msg[offset],
                finger_cnt, true);
        if (unlikely(ret))
            goto irq_err;

        for (i = 0; i < finger_cnt; i++)
            data->fingers[i].full_field = msg[i + offset];

        if (data->jig_mode) {
            ret = ist30xx_burst_read(data->client,
                    IST30XX_DA_ADDR(data->zvalue_addr), data->z_values,
                    finger_cnt, true);
            if (unlikely(ret))
                goto irq_err;
        }
        
        for (i = 0; i < finger_cnt; i++) {
            tsp_verb("intr msg(%d): 0x%08x, %d\n",
                    i + offset, msg[i + offset], data->z_values[i]);
        }
    }

    read_cnt = finger_cnt + 1;

    ret = check_valid_coord(&msg[0], read_cnt);
    if (unlikely(ret))
        goto irq_err;

    data->t_status = t_status;
    report_input_data(data, finger_cnt, key_cnt);

    if (data->intr_debug3_size > 0) {
        buf32 = kzalloc(data->intr_debug3_size * sizeof(u32), GFP_KERNEL);
        if (unlikely(!buf32)) {
            tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
            goto irq_err;
        }
        tsp_debug("Intr_debug3 (addr: 0x%08x)\n", data->intr_debug3_addr);
        ist30xx_burst_read(data->client,
                IST30XX_DA_ADDR(data->intr_debug3_addr), buf32,
                data->intr_debug3_size, true);

        for (i = 0; i < data->intr_debug3_size; i++)
            tsp_debug("\t%08x\n", buf32[i]);
        kfree(buf32);
    }

    goto irq_end;

irq_err:
	ret = -EINVAL;
    tsp_err("intr msg: 0x%08x, ret: %d\n", msg[0], ret);
    ist30xx_request_reset(data);
    goto irq_event;
irq_end:
    if (data->rec_mode)
        recording_data(data, idle);
    if (data->debugging_mode) {
        if ((data->debugging_scancnt == debugBuf32[0]) && data->debugging_noise)
            goto irq_event;

        memset(touch, 0, sizeof(touch));
        touch[0] = data->t_frame[0];
        touch[1] = data->t_frame[1];

        ist30xx_put_frame(data, ms, touch, debugBuf32,
                data->debugging_size / IST30XX_DATA_LEN);
        data->debugging_scancnt = debugBuf32[0];
    }
irq_event:
irq_ignore:
    data->irq_working = false;
    data->event_ms = (u32)get_milli_second(data);
    if (data->initialized)
        mod_timer(&data->event_timer, get_jiffies_64() + EVENT_TIMER_INTERVAL);
    return IRQ_HANDLED;

irq_ic_err:
    ist30xx_scheduled_reset(data);
    data->irq_working = false;
    data->event_ms = (u32)get_milli_second(data);
    if (data->initialized)
        mod_timer(&data->event_timer, get_jiffies_64() + EVENT_TIMER_INTERVAL);
    return IRQ_HANDLED;
}

static int ist30xx_pinctrl_configure(struct ist30xx_data *data, bool active)
{
    struct pinctrl_state *set_state;

    int retval;
    tsp_err("%s: %s\n", __func__, active ? "ACTIVE" : "SUSPEND");

    set_state = pinctrl_lookup_state(data->pinctrl,
            active ? "on_state" : "off_state");
    if (IS_ERR(set_state)) {
        tsp_err("%s: cannot get active state\n", __func__);
        return -EINVAL;
    }

    retval = pinctrl_select_state(data->pinctrl, set_state);
    if (retval) {
        tsp_err("%s: cannot set pinctrl %s state\n", __func__,
                active ? "active" : "suspend");
        return -EINVAL;
    }

    return 0;
}

static int ist30xx_suspend(struct device *dev)
{
    int ret;
    struct i2c_client *client = to_i2c_client(dev);
    struct ist30xx_data *data = i2c_get_clientdata(client);

    if (data->debugging_mode)
        return 0;

    del_timer(&data->event_timer);
    cancel_delayed_work_sync(&data->work_reset_check);
#ifdef IST30XX_NOISE_MODE
    cancel_delayed_work_sync(&data->work_noise_protect);
#else
    cancel_delayed_work_sync(&data->work_force_release);
#endif
    cancel_delayed_work_sync(&data->work_debug_algorithm);
    mutex_lock(&data->lock);
    data->suspend = true;
    if (data->spay || data->aod) {
        ist30xx_cmd_gesture(data, IST30XX_ENABLE);
        data->status.noise_mode = false;

        if (device_may_wakeup(&data->client->dev))
            enable_irq_wake(data->client->irq);

        data->status.sys_mode = LPM_MODE;
    } else {
        ist30xx_power_off(data);
        ist30xx_disable_irq(data);
        if (data->pinctrl) {
            ret = ist30xx_pinctrl_configure(data, false);
            if (ret)
                tsp_err("%s: cannot set pinctrl state\n", __func__);
        }
    }
    clear_input_data(data);
    mutex_unlock(&data->lock);

    return 0;
}

static int ist30xx_resume(struct device *dev)
{
    int ret;
    struct i2c_client *client = to_i2c_client(dev);
    struct ist30xx_data *data = i2c_get_clientdata(client);

    data->noise_mode |= (1 << NOISE_MODE_POWER);

    if (data->debugging_mode && data->status.power)
        return 0;

    mutex_lock(&data->lock);
    data->suspend = false;
    if (data->status.sys_mode != TOUCH_MODE) {
        ist30xx_cmd_gesture(data, IST30XX_DISABLE);
        mod_timer(&data->event_timer,
                get_jiffies_64() + EVENT_TIMER_INTERVAL * 2);
        data->status.noise_mode = true;

        if (device_may_wakeup(&data->client->dev))
            disable_irq_wake(data->client->irq);

        data->status.sys_mode = TOUCH_MODE;
    } else {
        if (data->pinctrl) {
            ret = ist30xx_pinctrl_configure(data, true);
            if (ret)
                tsp_err("%s: cannot set pinctrl state\n", __func__);
        }

        if (data->status.power)
            ist30xx_reset(data, false);
        else
            ist30xx_power_on(data, false);
        ist30xx_enable_irq(data);
        ist30xx_start(data);
    }
    mutex_unlock(&data->lock);

    return 0;
}

#ifdef USE_OPEN_CLOSE
static void ist30xx_ts_close(struct input_dev *dev)
{
    struct ist30xx_data *data = input_get_drvdata(dev);

    tsp_info("%s\n", __func__);
#ifdef TCLM_CONCEPT
	ist30xx_tclm_debug_info(data);
#endif
    ist30xx_suspend(&data->client->dev);
}

static int ist30xx_ts_open(struct input_dev *dev)
{
    struct ist30xx_data *data = input_get_drvdata(dev);

#ifdef USE_OPEN_CLOSE
    /* return if it is called by sec_key_notifier righ after run input_register_device */
    if (!data->probe_done)
        return 0;
#endif

    tsp_info("%s\n", __func__);

    return ist30xx_resume(&data->client->dev);
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void ist30xx_early_suspend(struct early_suspend *h)
{
    struct ist30xx_data *data = container_of(h, struct ist30xx_data,
            early_suspend);

    ist30xx_suspend(&data->client->dev);
}

static void ist30xx_late_resume(struct early_suspend *h)
{
    struct ist30xx_data *data = container_of(h, struct ist30xx_data,
            early_suspend);

    ist30xx_resume(&data->client->dev);
}
#endif

void ist30xx_set_ta_mode(bool mode)
{
    struct ist30xx_data *data = ts_data;

    if (unlikely(mode == ((data->noise_mode >> NOISE_MODE_TA) & 1)))
        return;

    tsp_info("%s(), mode = %d\n", __func__, mode);

    if (mode)
        data->noise_mode |= (1 << NOISE_MODE_TA);
    else
        data->noise_mode &= ~(1 << NOISE_MODE_TA);

    if (data->initialized && data->status.power)
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                ((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist30xx_set_ta_mode);

void ist30xx_set_edge_mode(int mode)
{
    struct ist30xx_data *data = ts_data;

    tsp_info("%s(), mode = %d\n", __func__, mode);

    if (mode)
        data->noise_mode |= (1 << NOISE_MODE_EDGE);
    else
        data->noise_mode &= ~(1 << NOISE_MODE_EDGE);

    if (data->status.power)
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                ((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist30xx_set_edge_mode);

void ist30xx_set_call_mode(int mode)
{
    struct ist30xx_data *data = ts_data;

    if (unlikely(mode == ((data->noise_mode >> NOISE_MODE_CALL) & 1)))
        return;

    tsp_info("%s(), mode = %d\n", __func__, mode);

    if (mode)
        data->noise_mode |= (1 << NOISE_MODE_CALL);
    else
        data->noise_mode &= ~(1 << NOISE_MODE_CALL);

    if (data->initialized && data->status.power)
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                ((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist30xx_set_call_mode);


void ist30xx_get_call_mode(int *mode)
{
    struct ist30xx_data *data = ts_data;
	
    *mode = ((data->noise_mode & (1 << NOISE_MODE_CALL)) ? 1 : 0);    
}
EXPORT_SYMBOL(ist30xx_get_call_mode);

void ist30xx_set_cover_mode(int mode)
{
    struct ist30xx_data *data = ts_data;

    if (unlikely(mode == ((data->noise_mode >> NOISE_MODE_COVER) & 1)))
        return;

    tsp_info("%s(), mode = %d\n", __func__, mode);

    if (mode)
        data->noise_mode |= (1 << NOISE_MODE_COVER);
    else
        data->noise_mode &= ~(1 << NOISE_MODE_COVER);

    if (data->initialized && data->status.power)
        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                ((eHCOM_SET_MODE_SPECIAL << 16) | (data->noise_mode & 0xFFFF)));
}
EXPORT_SYMBOL(ist30xx_set_cover_mode);

#ifdef USE_TSP_TA_CALLBACKS
void charger_enable(struct tsp_callbacks *cb, int enable)
{
    bool charging = enable ? true : false;

    ist30xx_set_ta_mode(charging);
}

static void ist30xx_register_callback(struct tsp_callbacks *cb)
{
    charger_callbacks = cb;
    pr_info("%s\n", __func__);
}
#endif

#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER) || defined(CONFIG_MUIC_NOTIFIER)
static int otg_flag = 0;
#endif

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
static int tsp_ccic_notification(struct notifier_block *nb,
        unsigned long action, void *data)
{
    CC_NOTI_USB_STATUS_TYPEDEF usb_status = *(CC_NOTI_USB_STATUS_TYPEDEF *)data;

    switch (usb_status.drp){
    case USB_STATUS_NOTIFY_ATTACH_DFP:
        otg_flag = 1;
        tsp_info("%s : otg_flag 1\n", __func__);
        break;
    case USB_STATUS_NOTIFY_DETACH:
        otg_flag = 0;
        break;
    default:
        break;
    }

    return 0;
}
#else
#ifdef CONFIG_MUIC_NOTIFIER
static int tsp_muic_notification(struct notifier_block *nb,
        unsigned long action, void *data)
{
    muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;

    switch (action) {
    case MUIC_NOTIFY_CMD_DETACH:
    case MUIC_NOTIFY_CMD_LOGICALLY_DETACH:
        otg_flag = 0;
        break;
    case MUIC_NOTIFY_CMD_ATTACH:
    case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
        if(attached_dev==ATTACHED_DEV_OTG_MUIC){
            otg_flag = 1;
            tsp_info("%s : otg_flag 1\n", __func__);
        }
        break;
    default:
        break;
    }

    return 0;
}
#endif
#endif

#ifdef CONFIG_VBUS_NOTIFIER
static int tsp_vbus_notification(struct notifier_block *nb,
        unsigned long cmd, void *data)
{
    vbus_status_t vbus_type = *(vbus_status_t *)data;

    tsp_info("%s cmd=%lu, vbus_type=%d\n", __func__, cmd, vbus_type);

    switch (vbus_type) {
    case STATUS_VBUS_HIGH:
        tsp_info("%s : attach\n",__func__);
#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER) || defined(CONFIG_MUIC_NOTIFIER)
        if(!otg_flag)
#endif
        ist30xx_set_ta_mode(true);
        break;
    case STATUS_VBUS_LOW:
        tsp_info("%s : detach\n",__func__);
        ist30xx_set_ta_mode(false);
        break;
    default:
        break;
    }
    return 0;
}
#endif

static void reset_work_func(struct work_struct *work)
{
    struct delayed_work *delayed_work = to_delayed_work(work);
    struct ist30xx_data *data = container_of(delayed_work, struct ist30xx_data,
            work_reset_check);

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
    if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
        tsp_err("%s return, TUI is enabled!\n", __func__);
        return;
    }
#endif

    if (unlikely((data == NULL) || (data->client == NULL)))
        return;

    tsp_info("Request reset function\n");

    if (likely((data->initialized == 1) && (data->status.power == 1) &&
            (data->status.update != 1) && (data->status.calib != 1 &&
            (data->status.calib != 2)) && (data->status.miscalib != 1))) {
        mutex_lock(&data->lock);
        ist30xx_disable_irq(data);
        ist30xx_reset(data, false);
        clear_input_data(data);
        ist30xx_enable_irq(data);
        if ((data->spay || data->aod) && data->suspend) {
            ist30xx_cmd_gesture(data, IST30XX_ENABLE);
            data->status.noise_mode = false;
        }
        ist30xx_start(data);
        mutex_unlock(&data->lock);
    }
}

#ifdef IST30XX_NOISE_MODE
static void noise_work_func(struct work_struct *work)
{
    int ret;
    u32 touch_status = 0;
    u32 scan_count = 0;
    struct delayed_work *delayed_work = to_delayed_work(work);
    struct ist30xx_data *data = container_of(delayed_work, struct ist30xx_data,
            work_noise_protect);
#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
    if (TRUSTEDUI_MODE_INPUT_SECURED & trustedui_get_current_mode()) {
        tsp_err("%s return, TUI is enabled!\n", __func__);
        return;
    }
#endif
    ret = ist30xx_read_reg(data->client, IST30XX_HIB_TOUCH_STATUS,
            &touch_status);
    if (unlikely(ret)) {
        tsp_warn("Touch status read fail!\n");
        goto retry_timer;
    }

    tsp_verb("Touch Info: 0x%08x\n", touch_status);

    /* Check valid scan count */
    if (unlikely((touch_status & TOUCH_STATUS_MASK) != TOUCH_STATUS_MAGIC)) {
        tsp_warn("Touch status is not corrected! (0x%08x)\n", touch_status);
        goto retry_timer;
    }

    /* Status of IC is idle */
    if (GET_FINGER_ENABLE(touch_status) == 0) {
        if ((PARSE_FINGER_CNT(data->t_status) > 0) ||
                (PARSE_KEY_CNT(data->t_status) > 0))
            clear_input_data(data);
    }

    scan_count = GET_SCAN_CNT(touch_status);

    /* Status of IC is lock-up */
    if (unlikely(scan_count == data->scan_count)) {
        tsp_warn("TSP IC is not responded! (0x%08x)\n", scan_count);
        goto retry_timer;
    }

    data->scan_retry = 0;
    data->scan_count = scan_count;
    return;

retry_timer:
    data->scan_retry++;
    tsp_warn("Retry touch status!(%d)\n", data->scan_retry);

    if (unlikely(data->scan_retry >= data->max_scan_retry)) {
        ist30xx_scheduled_reset(data);
        data->scan_retry = 0;
    }
}
#else
static void release_work_func(struct work_struct *work)
{
    int ret;
    u32 touch_status = 0;
    struct delayed_work *delayed_work = to_delayed_work(work);
    struct ist30xx_data *data = container_of(delayed_work, struct ist30xx_data,
            work_force_release);

    if ((PARSE_FINGER_CNT(data->t_status) == 0) &&
            (PARSE_KEY_CNT(data->t_status) == 0))
        return;

    ret = ist30xx_read_reg(data->client, IST30XX_HIB_TOUCH_STATUS,
            &touch_status);
    if (unlikely(ret)) {
        tsp_warn("Touch status read fail!\n");
        return;
    }

    tsp_verb("Touch Info: 0x%08x\n", touch_status);

    /* Check valid scan count */
    if (unlikely((touch_status & TOUCH_STATUS_MASK) != TOUCH_STATUS_MAGIC)) {
        tsp_warn("Touch status is not corrected! (0x%08x)\n", touch_status);
        return;
    }

    /* Status of IC is idle */
    if (GET_FINGER_ENABLE(touch_status) == 0)
            clear_input_data(data);
}
#endif

static void debug_work_func(struct work_struct *work)
{
    int ret = -EPERM;
    int i;
    u32 *buf32;

    struct delayed_work *delayed_work = to_delayed_work(work);
    struct ist30xx_data *data = container_of(delayed_work,
            struct ist30xx_data, work_debug_algorithm);

    buf32 = kzalloc(data->algr_size * sizeof(u32), GFP_KERNEL);
    if (unlikely(!buf32)) {
        tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
        return;
    }
    ret = ist30xx_burst_read(data->client, IST30XX_DA_ADDR(data->algr_addr),
            buf32, data->algr_size, true);
    if (ret) {
        tsp_warn("Algorithm mem addr read fail!\n");
        kfree(buf32);
        return;
    }

    tsp_info("algorithm struct\n");
    for (i = 0; i < data->algr_size; i++)
        tsp_info(" 0x%08x\n", buf32[i]);

    kfree(buf32);
}

void timer_handler(unsigned long timer_data)
{
    struct ist30xx_data *data = (struct ist30xx_data *)timer_data;
    struct ist30xx_status *status = &data->status;

    if (data->irq_working || !data->initialized || data->rec_mode)
        goto restart_timer;

    if (status->event_mode) {
        if (likely((status->power == 1) && (status->update != 1) &&
                    (status->calib != 1) && (status->miscalib != 1))) {
            data->timer_ms = (u32)get_milli_second(data);            

            if (likely(status->noise_mode)) {
                /* 100ms after last interrupt */
                if (data->timer_ms - data->event_ms > 100) {
#ifdef IST30XX_NOISE_MODE
                    schedule_delayed_work(&data->work_noise_protect, 0);
#else
                    schedule_delayed_work(&data->work_force_release, 0);
#endif
                }
            }

            if (data->algr_size > 0) {
                /* 100ms after last interrupt */
                if (data->timer_ms - data->event_ms > 100)
                    schedule_delayed_work(&data->work_debug_algorithm, 0);
            }
        }
    }

restart_timer:
    mod_timer(&data->event_timer, get_jiffies_64() + EVENT_TIMER_INTERVAL);
}

#ifdef CONFIG_OF
static int ist30xx_parse_dt(struct device *dev, struct ist30xx_data *data)
{
    struct device_node *np = dev->of_node;
    u32 coords[2];
    u32 px_zone[3];

    data->dt_data->irq_gpio = of_get_named_gpio(np, "imagis,irq-gpio", 0);
	
    data->dt_data->is_power_by_gpio = of_property_read_bool(np, "imagis,power-gpioen");
    if(data->dt_data->is_power_by_gpio)
    {		
        data->dt_data->power_gpio = of_get_named_gpio(np, "imagis,power-gpio", 0);
    }
    else
    {
        if (of_property_read_string(np, "imagis,regulator_avdd",
                &data->dt_data->regulator_avdd)) {
            input_err(true, dev, "%s: Failed to get regulator_avdd name property\n", __func__);
        }
    }    

    if (of_property_read_u32(np, "imagis,fw-bin", &data->dt_data->fw_bin) >= 0)
        input_info(true, dev, "%s() fw-bin: %d\n", __func__, data->dt_data->fw_bin);

    if (of_property_read_u32_array(np, "imagis,resolution", coords, 2)) {
        input_err(true, dev, "%s: Failed to get resolution\n", __func__);
        data->dt_data->x_max = 540;
        data->dt_data->y_max = 960;
    } else {
        data->dt_data->x_max = coords[0];
        data->dt_data->y_max = coords[1];
    }

    if (of_property_read_u32_array(np, "imagis,area-size", px_zone, 3)) {
        input_err(true, dev, "%s: Failed to get zone's size\n", __func__);
        data->dt_data->area_indicator = 36;
        data->dt_data->area_navigation = 72;
        data->dt_data->area_edge = 60;
    } else {
        data->dt_data->area_indicator = px_zone[0];
        data->dt_data->area_navigation = px_zone[1];
        data->dt_data->area_edge = px_zone[2];
    }

    if (of_property_read_string(np, "imagis,ic-version",
                &data->dt_data->ic_version) >= 0)
        input_info(true, dev, "%s() ic_version: %s\n", __func__, data->dt_data->ic_version);

    if (of_property_read_string(np, "imagis,project-name",
                &data->dt_data->project_name) >= 0)
        input_info(true, dev, "%s() project_name: %s\n", __func__,
                data->dt_data->project_name);

    if (data->dt_data->ic_version && data->dt_data->project_name) {
        snprintf(data->dt_data->fw_path, FIRMWARE_PATH_LENGTH,
                "%s%s_%s.fw", FIRMWARE_PATH,
                data->dt_data->ic_version, data->dt_data->project_name);
        input_info(true, dev, "%s() firm path: %s\n", __func__, data->dt_data->fw_path);

#ifdef IST30XX_USE_CMCS
        snprintf(data->dt_data->cmcs_path, FIRMWARE_PATH_LENGTH,
                "%s%s_%s_cmcs.bin", FIRMWARE_PATH,
                data->dt_data->ic_version, data->dt_data->project_name);
        input_info(true, dev, "%s() cmcs bin path: %s\n", __func__,
                data->dt_data->cmcs_path);
#endif
    }
	
#ifdef IST30XX_USE_KEY
    data->dt_data->tkey_use_sec_sysfs = of_property_read_bool(np, "imagis,use-sec-sysfs");
#endif

    if (of_property_read_u32(np, "imagis,octa-hw", &data->dt_data->octa_hw) >=
            0)
        input_info(true, dev, "%s() octa-hw: %d\n", __func__, data->dt_data->octa_hw);

#ifdef TCLM_CONCEPT
    if (of_property_read_u32(np, "imagis,tclm_level",
                &data->dt_data->tclm_level) >= 0)
        input_info(true, dev, "%s() tclm_level: %d\n", __func__,
            data->dt_data->tclm_level);

    if (of_property_read_u32(np, "imagis,afe_base", &data->dt_data->afe_base) >=
            0)
        input_info(true, dev, "%s() afe_base: %x\n", __func__, data->dt_data->afe_base);
#endif

    input_info(true, dev, "%s() irq:%d, tsp_ldo: %s\n", __func__, data->dt_data->irq_gpio,
        data->dt_data->regulator_avdd);

	input_info(true, dev, "%s() power source by gpio:%d, power_gpio: %s\n", __func__, data->dt_data->is_power_by_gpio, data->dt_data->power_gpio);
	
    return 0;
}

static void ist30xx_request_gpio(struct i2c_client *client,
        struct ist30xx_data *data)
{
    int ret;

    input_info(true, &client->dev, "%s\n", __func__);

    if (gpio_is_valid(data->dt_data->irq_gpio)) {
        ret = gpio_request(data->dt_data->irq_gpio, "imagis,irq_gpio");
        if (ret) {
            input_err(true, &client->dev, "%s: unable to request irq_gpio [%d]\n",
                    __func__, data->dt_data->irq_gpio);
            return;
        }

        ret = gpio_direction_input(data->dt_data->irq_gpio);
        if (ret) {
            input_err(true, &client->dev, "%s: unable to set direction for gpio [%d]\n",
                    __func__, data->dt_data->irq_gpio);
        }
        client->irq = gpio_to_irq(data->dt_data->irq_gpio);
    }

	if (gpio_is_valid(data->dt_data->power_gpio)) {		
        ret = gpio_request(data->dt_data->power_gpio, "imagis,power_gpio");
        if (ret) {
            input_err(true, &client->dev, "%s: unable to request power_gpio [%d]\n",
                    __func__, data->dt_data->power_gpio);
            return;
        }

        ret = gpio_direction_output(data->dt_data->power_gpio, 1);
        if (ret) {
            input_err(true, &client->dev, "%s: unable to set direction for gpio [%d]\n",
                    __func__, data->dt_data->power_gpio);
        }		
    }
}

static void ist30xx_free_gpio(struct ist30xx_data *data)
{
    tsp_info("%s\n", __func__);

    if (gpio_is_valid(data->dt_data->irq_gpio))
        gpio_free(data->dt_data->irq_gpio);

	if (gpio_is_valid(data->dt_data->power_gpio))
        gpio_free(data->dt_data->power_gpio);
}
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
void trustedui_mode_on(void)
{
    tsp_info("%s, release all finger..", __func__);
    clear_input_data(tui_tsp_info);

    del_timer(&tui_tsp_info->event_timer);

    cancel_delayed_work_sync(&tui_tsp_info->work_reset_check);
#ifdef IST30XX_NOISE_MODE
    cancel_delayed_work_sync(&tui_tsp_info->work_noise_protect);
#else
    cancel_delayed_work_sync(&tui_tsp_info->work_force_release);
#endif
    cancel_delayed_work_sync(&tui_tsp_info->work_debug_algorithm);
    tui_tsp_info->status.noise_mode = false;
}

void trustedui_mode_off(void)
{
    tsp_info("%s ", __func__);

    tui_tsp_info->status.noise_mode = true;

    mod_timer(&tui_tsp_info->event_timer, get_jiffies_64() +
            (HZ * tui_tsp_info->timer_period_ms / 1000)); //EVENT_TIMER_INTERVAL
}
#endif
#ifdef TCLM_CONCEPT
static void ist30xx_tclm_initialize(struct work_struct *work)
{
	struct ist30xx_data *data = container_of(work, struct ist30xx_data,
							work_tclm_initialize.work);
	int ret;
	tsp_info("%s start \n", __func__);
	ret = ist30xx_tclm_get_nvm_all(data, false);
	if (unlikely(!ret)) {
		tsp_info("%s TCLM_CONCEPT get_nvm_all failed \n", __func__);
	}
	input_log_fix();
}
#endif

unsigned int bootmode;
static int __init get_bootmode(char *arg)
{
	get_option(&arg, &bootmode);

	return 0;
}
early_param("bootmode", get_bootmode);

extern int get_lcd_attached(char *mode);

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
#include <linux/sec_debug.h>
extern struct tsp_dump_callbacks dump_callbacks;
static struct delayed_work *p_ghost_check;

static void ist30xx_check_rawdata(struct work_struct *work)
{
    struct ist30xx_data *data = container_of(work, struct ist30xx_data, ghost_check.work);

    if (data->tsp_dump_lock == 1) {
        input_info(true, &data->client->dev, "%s: ignored ## already checking..\n", __func__);
        return;
    }
    if (data->status.power == 0) {
        input_info(true, &data->client->dev, "%s: ignored ## IC is power off\n", __func__);
        return;
    }

    data->tsp_dump_lock = 1;
    input_raw_data_clear();
    input_raw_info(true, &data->client->dev, "%s: start ##\n", __func__);
    ist30xx_display_dump_log(data);
    msleep(100);

    input_raw_info(true, &data->client->dev, "%s: done ##\n", __func__);
    data->tsp_dump_lock = 0;
}

static void dump_tsp_log(void)
{
#ifdef CONFIG_BATTERY_SAMSUNG
    extern int poweroff_charging;
#endif

    tsp_info("%s: start\n", __func__);

#ifdef CONFIG_BATTERY_SAMSUNG	
    if (poweroff_charging == 1) {
        tsp_info("%s: ignored ## lpm charging Mode!!\n",__func__);
        return;
    }
#endif

    if (p_ghost_check == NULL) {
        tsp_info("%s: ignored ## tsp probe fail!!\n", __func__);
        return;
    }
    schedule_delayed_work(p_ghost_check, msecs_to_jiffies(100));
}
#endif

static int ist30xx_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int ret;
    int i;
    int retry = 3;
    struct ist30xx_data *data;
    struct input_dev *input_dev;

    input_info(true, &client->dev, "### IMAGIS probe(ver:%s, addr:0x%02X) ###\n",
            IMAGIS_TSP_DD_VERSION, client->addr);
			
	if (bootmode == 2) {
		input_err(true, &client->dev, "%s : Do not load driver due to : device entered recovery mode %d\n",
			__func__, bootmode);
		return -ENODEV;
	}

	/* when lcd moudle was taken off, lcd id is received 0xFFFFFF */
    if (get_lcd_attached("GET") == 0xFFFFFF) {
        input_err(true, &client->dev, "%s: LCD module was taken off\n", __func__);
        return -ENODEV;
    }

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_INFO "i2c_check_functionality error\n");
        return -EIO;
    }

    data = kzalloc(sizeof(struct ist30xx_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

#ifdef CONFIG_OF
    // TODO: Load device tree.
    data->dt_data = NULL;
    if (client->dev.of_node) {
        data->dt_data = kzalloc(sizeof(struct ist30xx_dt_data), GFP_KERNEL);
        if (unlikely(!data->dt_data)) {
            input_err(true, &client->dev, "failed to allocate dt data\n");
            goto err_alloc_dev;
        }

        ret = ist30xx_parse_dt(&client->dev, data);
        if (unlikely(ret))
            goto err_alloc_dev;
    } else {
        data->dt_data = NULL;
        input_err(true, &client->dev, "don't exist of_node\n");
        goto err_alloc_dev;
    }

    ist30xx_request_gpio(client, data);

    /* Get pinctrl if target uses pinctrl */
    data->pinctrl = devm_pinctrl_get(&client->dev);
    if (IS_ERR(data->pinctrl)) {
        if (PTR_ERR(data->pinctrl) == -EPROBE_DEFER)
            goto err_pinctrl;

        input_err(true, &client->dev, "%s: Target does not use pinctrl\n", __func__);
        data->pinctrl = NULL;
    }

    if (data->pinctrl) {
        ret = ist30xx_pinctrl_configure(data, true);
        if (ret)
        input_err(true, &client->dev, "%s: cannot set pinctrl state\n", __func__);
    }
#endif

    input_dev = input_allocate_device();
    if (unlikely(!input_dev)) {
        input_err(true, &client->dev, "input_allocate_device failed\n");
        goto err_alloc_dt;
    }

    input_info(true, &client->dev, "client->irq : %d\n", client->irq);
    data->client = client;
    data->input_dev = input_dev;
#ifdef USE_OPEN_CLOSE
    data->probe_done = false;
#endif
    i2c_set_clientdata(client, data);

#ifdef USE_OPEN_CLOSE
    input_dev->open = ist30xx_ts_open;
    input_dev->close = ist30xx_ts_close;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
    data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    data->early_suspend.suspend = ist30xx_early_suspend;
    data->early_suspend.resume = ist30xx_late_resume;
    register_early_suspend(&data->early_suspend);
#endif

    data->max_fingers = IST30XX_MAX_FINGERS;
    data->max_keys = IST30XX_MAX_KEYS;
    data->irq_enabled = false;
    data->status.event_mode = false;
    data->status.sys_mode = TOUCH_MODE;
    mutex_init(&data->lock);
    mutex_init(&data->aod_lock);
    mutex_init(&data->i2c_lock);
    ts_data = data;

    /* initialize data variable */
    data->ignore_delay = false;
    data->irq_working = false;
    data->max_scan_retry = 2;
    data->max_irq_err_cnt = IST30XX_MAX_ERR_CNT;
    data->report_rate = -1;
    data->idle_rate = -1;
    data->timer_period_ms = 500;
    data->suspend = false;
    data->spay = false;
    data->aod = false;
    data->rec_mode = 0;
    data->rec_file_name = kzalloc(IST30XX_REC_FILENAME_SIZE, GFP_KERNEL);
    data->debug_mode = 0;
    data->debugging_mode = 0;
    data->debugging_noise = 1;
    data->checksum_result = 0;
    data->time_longest = 0;
    data->all_finger_count = 0;
#ifdef CONFIG_SEC_FACTORY
    data->jig_mode = 1;
#endif
#ifdef TCLM_CONCEPT
	data->root_of_calibration = CALPOSITION_NONE;
	data->cal_position = 0;
	data->cal_pos_hist_cnt = 0;
	data->cal_pos_hist_last3[0] = 0;
	data->tclm_string = sec_cal_positions;
#endif
    for (i = 0; i < IST30XX_MAX_FINGERS; i++)
        data->tsp_touched[i] = false;
#ifdef IST30XX_USE_KEY
    for (i = 0; i < IST30XX_MAX_KEYS; i++)
        data->tkey_pressed[i] = false;
#endif
    data->g_reg.b.w = 0;
    data->g_reg.b.h = 0;
    data->g_reg.b.x = 0;
    data->g_reg.b.y = 0;

    /* system power init */
    ret = ist30xx_init_system(data);
    if (unlikely(ret)) {
        input_err(true, &client->dev, "chip initialization failed\n");
        goto err_init_drv;
    }

    retry = IST30XX_MAX_RETRY_CNT;
    while (retry-- > 0) {
        ret = ist30xx_read_reg(data->client, rSYS_CHIPID, &data->chip_id);
        data->chip_id &= 0xFFFF;
        if (unlikely(ret == 0)) {
            if (data->chip_id == IST30XX_CHIP_ID)
                break;
        } else if (unlikely(retry == 0)) {
            goto err_init_drv;
        }

        ist30xx_reset(data, false);
    }

    input_info(true, &client->dev, "TSP IC: %x\n", data->chip_id);

    /* init irq thread */
    ret = request_threaded_irq(client->irq, NULL, ist30xx_irq_thread,
            IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_DISABLED, "ist30xx_ts",
            data);
    if (unlikely(ret))
        goto err_init_drv;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
    trustedui_set_tsp_irq(client->irq);
    input_info(true, &client->dev, "%s[%d] called!\n", __func__, client->irq);
#endif

#ifdef TCLM_CONCEPT
	INIT_DELAYED_WORK(&data->work_tclm_initialize, ist30xx_tclm_initialize);
#endif

#ifdef IST30XX_INTERNAL_BIN
	// Do not update the FW, if its in Factory Calibration mode
	{
#if defined (CONFIG_SEC_FACTORY)
		extern char *saved_command_line;
		
		printk("%s\n", saved_command_line);
		if(strstr(saved_command_line, "calibration") == NULL)
#endif //CONFIG_SEC_FACTORY
		{
			ret = ist30xx_auto_bin_update(data);
			input_info(true, &client->dev, "auto bin update: %s\n", (ret != 0 ? "success" : "fail"));
			if (unlikely(ret == 0))
				goto err_irq;
		}
	}
#endif

    ret = ist30xx_get_info(data);
    input_info(true, &client->dev, "Get info: %s\n", (ret == 0 ? "success" : "fail"));
    if (unlikely(ret))
        goto err_read_info;

    ret = ist30xx_init_update_sysfs(data);
    if (unlikely(ret))
        goto err_sysfs;

    ret = ist30xx_init_misc_sysfs(data);
    if (unlikely(ret))
        goto err_sysfs;

#ifdef IST30XX_USE_CMCS
    ret = ist30xx_init_cmcs_sysfs(data);
    if (unlikely(ret))
        goto err_sysfs;
#endif

#ifdef SEC_FACTORY_MODE
    ret = sec_fac_cmd_init(data);
    if (unlikely(ret))
        goto err_sysfs;

    ret = sec_touch_sysfs(data);
    if (unlikely(ret))
        goto err_sec_sysfs;
#endif
    input_info(true, &client->dev, "Create sysfs!!\n");

    INIT_DELAYED_WORK(&data->work_reset_check, reset_work_func);
#ifdef IST30XX_NOISE_MODE
    INIT_DELAYED_WORK(&data->work_noise_protect, noise_work_func);
#else
    INIT_DELAYED_WORK(&data->work_force_release, release_work_func);
#endif
    INIT_DELAYED_WORK(&data->work_debug_algorithm, debug_work_func);

    init_timer(&data->event_timer);
    data->event_timer.data = (unsigned long)data;
    data->event_timer.function = timer_handler;
    data->event_timer.expires = jiffies_64 + EVENT_TIMER_INTERVAL;
    mod_timer(&data->event_timer, get_jiffies_64() + EVENT_TIMER_INTERVAL * 2);

#ifdef USE_TSP_TA_CALLBACKS
    data->callbacks.inform_charger = charger_enable;
    ist30xx_register_callback(&data->callbacks);
#endif
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
    manager_notifier_register(&data->ccic_nb, tsp_ccic_notification,
            MANAGER_NOTIFY_CCIC_USB);
#else
#ifdef CONFIG_MUIC_NOTIFIER
    muic_notifier_register(&data->muic_nb, tsp_muic_notification,
            MUIC_NOTIFY_DEV_CHARGER);
#endif
#endif
#ifdef CONFIG_VBUS_NOTIFIER
    vbus_notifier_register(&data->vbus_nb, tsp_vbus_notification,
            VBUS_NOTIFY_DEV_CHARGER);
#endif

    device_init_wakeup(&client->dev, true);

    ist30xx_start(data);
    data->initialized = true;

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
    tui_tsp_info = data;
#endif
#ifdef TCLM_CONCEPT
	schedule_delayed_work(&data->work_tclm_initialize, msecs_to_jiffies(5000));
#endif
#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
    dump_callbacks.inform_dump = dump_tsp_log;
    INIT_DELAYED_WORK(&data->ghost_check, ist30xx_check_rawdata);
    p_ghost_check = &data->ghost_check;
    schedule_delayed_work(p_ghost_check, msecs_to_jiffies(6000));
#endif

    input_info(true, &client->dev, "### IMAGIS probe success ###\n");
#ifdef USE_OPEN_CLOSE
    data->probe_done = true;
#endif
    input_log_fix();

    return 0;

#ifdef SEC_FACTORY_MODE
err_sec_sysfs:
    sec_fac_cmd_remove(data);
    sec_touch_sysfs_remove(data);
#endif
err_sysfs:
    class_destroy(ist30xx_class);
err_read_info:
#ifdef IST30XX_INTERNAL_BIN
err_irq:
#endif
    ist30xx_disable_irq(data);
    free_irq(client->irq, data);
err_init_drv:
    input_free_device(input_dev);
    data->status.event_mode = false;
    ist30xx_power_off(data);
#if (defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE))
    unregister_early_suspend(&data->early_suspend);
#endif
err_alloc_dt:
err_pinctrl:
    if (data->dt_data) {
        input_err(true, &client->dev, "Error, ist30xx mem free\n");
        ist30xx_free_gpio(data);
        kfree(data->dt_data);
    }
#ifdef CONFIG_OF
err_alloc_dev:
#endif
#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
    p_ghost_check = NULL;
#endif
    kfree(data);
    input_err(true, &client->dev, "Error, ist30xx init driver\n");
    input_log_fix();

    return -ENODEV;
}

static int ist30xx_remove(struct i2c_client *client)
{
    struct ist30xx_data *data = i2c_get_clientdata(client);

#if (defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE))
    unregister_early_suspend(&data->early_suspend);
#endif

    ist30xx_disable_irq(data);
    free_irq(client->irq, data);
    ist30xx_power_off(data);
#ifdef SEC_FACTORY_MODE
    sec_fac_cmd_remove(data);
#endif

#ifdef CONFIG_OF
    if (data->dt_data) {
        ist30xx_free_gpio(data);
        kfree(data->dt_data);
    }
#endif

    input_unregister_device(data->input_dev);
    input_free_device(data->input_dev);
    kfree(data);

    return 0;
}

static void ist30xx_shutdown(struct i2c_client *client)
{
    struct ist30xx_data *data = i2c_get_clientdata(client);

    del_timer(&data->event_timer);
    cancel_delayed_work_sync(&data->work_reset_check);
#ifdef IST30XX_NOISE_MODE
    cancel_delayed_work_sync(&data->work_noise_protect);
#else
    cancel_delayed_work_sync(&data->work_force_release);
#endif
    cancel_delayed_work_sync(&data->work_debug_algorithm);
    mutex_lock(&data->lock);
    ist30xx_disable_irq(data);
    ist30xx_power_off(data);
    clear_input_data(data);
    mutex_unlock(&data->lock);
}

static struct i2c_device_id ist30xx_idtable[] = {
    { IST30XX_DEV_NAME, 0 },
    {},
};
MODULE_DEVICE_TABLE(i2c, ist30xx_idtable);

#ifdef CONFIG_OF
static struct of_device_id ist30xx_match_table[] = {
    { .compatible = "imagis,ist30xx-ts", },
    { },
};
#else
#define ist30xx_match_table NULL
#endif

#if (!defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE))
static const struct dev_pm_ops ist30xx_pm_ops = {
    .suspend    = ist30xx_suspend,
    .resume     = ist30xx_resume,
};
#endif

static struct i2c_driver ist30xx_i2c_driver = {
    .id_table   = ist30xx_idtable,
    .probe      = ist30xx_probe,
    .remove     = ist30xx_remove,
    .shutdown   = ist30xx_shutdown,
    .driver     = {
        .owner          = THIS_MODULE,
        .name           = IST30XX_DEV_NAME,
        .of_match_table = ist30xx_match_table,
#if (!defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE))
        .pm             = &ist30xx_pm_ops,
#endif
    },
};

static int __init ist30xx_init(void)
{
    tsp_info("%s()\n", __func__);

#ifdef CONFIG_BATTERY_SAMSUNG
	{
		extern int poweroff_charging;
		
		if (poweroff_charging) {
			tsp_info("%s : Do not load driver due to : lpm %d\n",
                __func__, poweroff_charging);
			return 0;
		}
	}
#endif

    return i2c_add_driver(&ist30xx_i2c_driver);
}


static void __exit ist30xx_exit(void)
{
    tsp_info("%s()\n", __func__);
    i2c_del_driver(&ist30xx_i2c_driver);
}

module_init(ist30xx_init);
module_exit(ist30xx_exit);

MODULE_DESCRIPTION("Imagis IST30XX touch driver");
MODULE_LICENSE("GPL");
