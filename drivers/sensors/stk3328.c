/*
 *  stk3328.c - Linux kernel modules for sensortek stk301x, stk321x, stk331x
 *  , and stk3410 proximity/ambient light sensor
 *
 *  Copyright (C) 2012~2016 Lex Hsieh / sensortek <lex_hsieh@sensortek.com.tw>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include   <linux/fs.h>
#include  <asm/uaccess.h>
#ifdef CONFIG_OF
    #include <linux/of_gpio.h>
#endif
#include "stk3328.h"
#include <linux/sensor/sensors_core.h>

#define DRIVER_VERSION  "3.11.0"

/* Driver Settings */
#define CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
#define STK_ALS_CHANGE_THD  10      /* The threshold to trigger ALS interrupt, unit: lux */
#define STK_INT_PS_MODE     1       /* 1, 2, or 3   */
//#define STK_POLL_PS
#define STK_POLL_ALS                /* ALS interrupt is valid only when STK_INT_PS_MODE = 1 or 4*/
//#define STK_TUNE0
#define CALI_PS_EVERY_TIME
#define STK_DEBUG_PRINTF
#define STK_ALS_FIR
// #define STK_IRS
//#define STK_CHK_REG

/* Define Register Map */
#define STK_STATE_REG           0x00
#define STK_PSCTRL_REG          0x01
#define STK_ALSCTRL_REG         0x02
#define STK_LEDCTRL_REG         0x03
#define STK_INT_REG             0x04
#define STK_WAIT_REG            0x05
#define STK_THDH1_PS_REG        0x06
#define STK_THDH2_PS_REG        0x07
#define STK_THDL1_PS_REG        0x08
#define STK_THDL2_PS_REG        0x09
#define STK_THDH1_ALS_REG       0x0A
#define STK_THDH2_ALS_REG       0x0B
#define STK_THDL1_ALS_REG       0x0C
#define STK_THDL2_ALS_REG       0x0D
#define STK_FLAG_REG            0x10
#define STK_DATA1_PS_REG        0x11
#define STK_DATA2_PS_REG        0x12
#define STK_DATA1_RESERVED1_REG         0x13
#define STK_DATA2_RESERVED1_REG         0x14
#define STK_DATA1_ALS_REG       0x15
#define STK_DATA2_ALS_REG       0x16
#define STK_DATA1_RESERVED2_REG         0x17
#define STK_DATA2_RESERVED2_REG         0x18
#define STK_DATA1_C_REG         0x19
#define STK_DATA2_C_REG         0x1A
#define STK_DATA1_OFFSET_REG    0x1B
#define STK_DATA2_OFFSET_REG    0x1C
#define STK_ALSCTRL2_REG        0x1D
#define STK_PDT_ID_REG          0x3E
#define STK_RSRVD_REG           0x3F
#define STK_SW_RESET_REG        0x80
#define STK_INT2_REG            0xA4
#define STK_ALSC_GAIN_REG       0x1D
#define STK_REG_INTELLI_WAIT_PS_REG 0x1E
#define STK_BGIR_REG            0xA0
#define STK_CI_REG              0xDB

/* Define state reg */
#define STK_STATE_EN_INTELLPRST_SHIFT   3
#define STK_STATE_EN_WAIT_SHIFT     2
#define STK_STATE_EN_ALS_SHIFT      1
#define STK_STATE_EN_PS_SHIFT       0

#define STK_STATE_EN_INTELLPRST_MASK    0x08
#define STK_STATE_EN_WAIT_MASK  0x04
#define STK_STATE_EN_ALS_MASK   0x02
#define STK_STATE_EN_PS_MASK    0x01

/* Define PS ctrl reg */
#define STK_PS_PRS_SHIFT        6
#define STK_PS_GAIN_SHIFT       4
#define STK_PS_IT_SHIFT         0

#define STK_PS_PRS_MASK         0xC0
#define STK_PS_GAIN_MASK        0x30
#define STK_PS_IT_MASK          0x0F

/* Define ALS ctrl reg */
#define STK_ALS_PRS_SHIFT       6
#define STK_ALS_GAIN_SHIFT      4
#define STK_ALS_IT_SHIFT        0
#define STK_ALS_PRS_MASK        0xC0
#define STK_ALS_GAIN_MASK       0x30
#define STK_ALS_IT_MASK         0x0F

/* Define LED ctrl reg */
#define STK_LED_IRDR_SHIFT      5
#define STK_LED_DT_SHIFT        0

#define STK_LED_IRDR_MASK       0xE0

/* Define interrupt reg */
#define STK_INT_CTRL_SHIFT      7
#define STK_INT_ENIVALIDPS_SHIFT        5
#define STK_INT_ALS_SHIFT       3
#define STK_INT_PS_SHIFT        0

#define STK_INT_CTRL_MASK       0x80
#define STK_INT_ENIVALIDPS_MASK     0x20
#define STK_INT_ALS_MASK        0x08
#define STK_INT_PS_MASK         0x07

/* Define flag reg */
#define STK_FLG_ALSDR_SHIFT         7
#define STK_FLG_PSDR_SHIFT          6
#define STK_FLG_ALSINT_SHIFT        5
#define STK_FLG_PSINT_SHIFT         4
#define STK_FLG_ALSSAT_SHIFT        2
#define STK_FLG_INVALIDPS_SHIFT         1
#define STK_FLG_NF_SHIFT        0

#define STK_FLG_ALSDR_MASK      0x80
#define STK_FLG_PSDR_MASK       0x40
#define STK_FLG_ALSINT_MASK     0x20
#define STK_FLG_PSINT_MASK      0x10
#define STK_FLG_ALSSAT_MASK     0x04
#define STK_FLG_INVALIDPS_MASK      0x02
#define STK_FLG_NF_MASK         0x01

/* misc define */
#define MIN_ALS_POLL_DELAY_NS   60000000
#define PROX_READ_NUM   30

#ifdef STK_TUNE0
    #define STK_MAX_MIN_DIFF    200
    #define STK_LT_N_CT 100
    #define STK_HT_N_CT 150
#endif  /* #ifdef STK_TUNE0 */

#define STK_IRC_MAX_ALS_CODE        20000
#define STK_IRC_MIN_ALS_CODE        25
#define STK_IRC_MIN_IR_CODE     50
#define STK_IRC_ALS_DENOMI      2
#define STK_IRC_ALS_NUMERA      5
#define STK_IRC_ALS_CORREC      850

#define STK_IRS_IT_REDUCE           2
#define STK_ALS_READ_IRS_IT_REDUCE  5
#define STK_ALS_THRESHOLD           30

#define VENDOR_NAME "SENSORTEK"
#define DEVICE_NAME "STK3328"
#define ALS_NAME    "light_sensor"
#define PS_NAME     "proximity_sensor"

#define STK3325_PID     0x1A
#define STK3327_PID     0x1A
#define STK3220_PID     0x1B

#ifdef STK_ALS_FIR
#define STK_FIR_LEN 8
#define MAX_FIR_LEN 32

struct data_filter
{
    u16 raw[MAX_FIR_LEN];
    int sum;
    int number;
    int idx;
};
#endif

enum {
	OFF = 0,
	ON,
};

struct stk3328_data
{
    struct i2c_client *client;
    struct stk3328_platform_data *pdata;
#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))
    int32_t irq;
    struct work_struct stk_work;
    struct workqueue_struct *stk_wq;
#endif
    uint16_t ir_code;
    uint16_t als_correct_factor;
    uint8_t alsctrl_reg;
    uint8_t psctrl_reg;
    uint8_t ledctrl_reg;
    uint8_t state_reg;
    int     int_pin;
    uint8_t wait_reg;
    uint8_t als_cgain;
    uint8_t int_reg;
    uint16_t ps_thd_h;
    uint16_t ps_thd_l;
    bool bgir_update;
    bool ps_report;
#ifdef CALI_PS_EVERY_TIME
    uint16_t ps_high_thd_boot;
    uint16_t ps_low_thd_boot;
#endif
    struct mutex io_lock;
    struct input_dev *ps_input_dev;
    int32_t ps_distance_last;
    bool ps_enabled;
    bool re_enable_ps;
    struct wake_lock ps_wakelock;
#ifdef STK_POLL_PS
    struct hrtimer ps_timer;
    struct work_struct stk_ps_work;
    struct workqueue_struct *stk_ps_wq;
    struct wake_lock ps_nosuspend_wl;
#endif
    struct input_dev *als_input_dev;
    int32_t als_lux_last;
    uint32_t als_transmittance;
    bool als_enabled;
    bool re_enable_als;
    ktime_t ps_poll_delay;
    ktime_t als_poll_delay;
	struct device *proximity_dev;
	struct device *light_dev;
#ifdef STK_POLL_ALS
    struct work_struct stk_als_work;
    struct hrtimer als_timer;
    struct workqueue_struct *stk_als_wq;
#endif
    bool first_boot;
#ifdef STK_TUNE0
    uint16_t psa;
    uint16_t psi;
    uint16_t psi_set;
    struct hrtimer ps_tune0_timer;
    struct workqueue_struct *stk_ps_tune0_wq;
    struct work_struct stk_ps_tune0_work;
    ktime_t ps_tune0_delay;
    bool tune_zero_init_proc;
    uint32_t ps_stat_data[3];
    int data_count;
    int stk_max_min_diff;
    int stk_lt_n_ct;
    int stk_ht_n_ct;
#endif
#ifdef STK_ALS_FIR
    struct data_filter      fir;
    atomic_t                firlength;
#endif
    atomic_t    recv_reg;
    
#ifdef STK_IRS
    int als_data_index;
#endif
    struct hrtimer prox_timer;
	ktime_t prox_poll_delay;
	struct workqueue_struct *prox_wq;
	struct work_struct work_prox;
    uint8_t pid;
    uint32_t als_code_last;
    bool als_en_hal;
    uint32_t ps_code_last;
    uint8_t boot_cali;
	uint16_t ps_default_trim;
	int c_data;
	int als_data;
	int gain;
	bool change_gain;
	int avg[3];
};

#if( !defined(CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD))
static uint32_t lux_threshold_table[] =
{
    3,
    10,
    40,
    65,
    145,
    300,
    550,
    930,
    1250,
    1700,
};

#define LUX_THD_TABLE_SIZE (sizeof(lux_threshold_table)/sizeof(uint32_t)+1)
static uint16_t code_threshold_table[LUX_THD_TABLE_SIZE + 1];
#endif

static int32_t stk3328_enable_ps(struct stk3328_data *ps_data, uint8_t enable, uint8_t validate_reg);
static int32_t stk3328_enable_als(struct stk3328_data *ps_data, uint8_t enable);
static int32_t stk3328_set_ps_thd_l(struct stk3328_data *ps_data, uint16_t thd_l);
static int32_t stk3328_set_ps_thd_h(struct stk3328_data *ps_data, uint16_t thd_h);
static int32_t stk3328_get_ir_reading(struct stk3328_data *ps_data, int32_t als_it_reduce);
static uint32_t stk_alscode2lux(struct stk3328_data *ps_data, uint32_t alscode);

#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))
    static uint32_t stk_lux2alscode(struct stk3328_data *ps_data, uint32_t lux);
    static int32_t stk3328_set_als_thd_l(struct stk3328_data *ps_data, uint16_t thd_l);
    static int32_t stk3328_set_als_thd_h(struct stk3328_data *ps_data, uint16_t thd_h);
    #ifdef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
        static void stk_als_set_new_thd(struct stk3328_data *ps_data, uint16_t alscode);
    #endif
#endif

#ifdef STK_TUNE0
    static int stk_ps_tune_zero_func_fae(struct stk3328_data *ps_data);
	static int stk_ps_val(struct stk3328_data *ps_data);
#endif
#ifdef STK_CHK_REG
    static int stk3328_validate_n_handle(struct i2c_client *client);
#endif

static int stk3328_i2c_read_data(struct i2c_client *client, unsigned char command, int length, unsigned char *values)
{
    uint8_t retry;
    int err;
    struct i2c_msg msgs[] =
    {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf = &command,
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = length,
            .buf = values,
        },
    };
    for (retry = 0; retry < 5; retry++)
    {
        err = i2c_transfer(client->adapter, msgs, 2);
        if (err == 2)
            break;
        else
            mdelay(5);
    }
    if (retry >= 5)
    {
        pr_err("[SENSOR] %s: i2c read fail, err=%d\n", __func__, err);
        return -EIO;
    }
    return 0;
}

static int stk3328_i2c_write_data(struct i2c_client *client, unsigned char command, int length, unsigned char *values)
{
    int retry;
    int err;
    unsigned char data[11];
    struct i2c_msg msg;
    int index;
    if (!client)
        return -EINVAL;
    else if (length >= 10)
    {
        pr_err("[SENSOR] %s:length %d exceeds 10\n", __func__, length);
        return -EINVAL;
    }
    data[0] = command;
    for (index = 1; index <= length; index++)
        data[index] = values[index - 1];
    msg.addr = client->addr;
    msg.flags = 0;
    msg.len = length + 1;
    msg.buf = data;
    for (retry = 0; retry < 5; retry++)
    {
        err = i2c_transfer(client->adapter, &msg, 1);
        if (err == 1)
            break;
        else
            mdelay(5);
    }
    if (retry >= 5)
    {
        pr_err("[SENSOR] %s: i2c write fail, err=%d\n", __func__, err);
        return -EIO;
    }
    return 0;
}

static int stk3328_i2c_smbus_read_byte_data(struct i2c_client *client, unsigned char command)
{
    unsigned char value;
    int err;
    err = stk3328_i2c_read_data(client, command, 1, &value);
    if (err < 0)
        return err;
    return value;
}

static int stk3328_i2c_smbus_write_byte_data(struct i2c_client *client, unsigned char command, unsigned char value)
{
    int err;
    err = stk3328_i2c_write_data(client, command, 1, &value);
    return err;
}

static uint32_t stk_alscode2lux(struct stk3328_data *ps_data, uint32_t alscode)
{
    alscode += ((alscode << 7) + (alscode << 3) + (alscode >> 1));
    alscode <<= 3;
    alscode /= ps_data->als_transmittance;
	pr_info("[SENSOR]%s: alscode2lux = %d\n", __func__, alscode);
    return alscode;
}

#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))
static uint32_t stk_lux2alscode(struct stk3328_data *ps_data, uint32_t lux)
{
    lux *= ps_data->als_transmittance;
    lux /= 1100;
    if (unlikely(lux >= (1 << 16)))
        lux = (1 << 16) - 1;
    return lux;
}

static int32_t stk3328_set_als_thd_l(struct stk3328_data *ps_data, uint16_t thd_l)
{
    unsigned char val[2];
    int ret;
    val[0] = (thd_l & 0xFF00) >> 8;
    val[1] = thd_l & 0x00FF;
    ret = stk3328_i2c_write_data(ps_data->client, STK_THDL1_ALS_REG, 2, val);

    if (ret < 0)
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, ret);

    return ret;
}
static int32_t stk3328_set_als_thd_h(struct stk3328_data *ps_data, uint16_t thd_h)
{
    unsigned char val[2];
    int ret;
    val[0] = (thd_h & 0xFF00) >> 8;
    val[1] = thd_h & 0x00FF;
    ret = stk3328_i2c_write_data(ps_data->client, STK_THDH1_ALS_REG, 2, val);

    if (ret < 0)
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, ret);

    return ret;
}
#endif

static int32_t stk3328_set_ps_thd_l(struct stk3328_data *ps_data, uint16_t thd_l)
{
    unsigned char val[2];
    int ret;
    val[0] = (thd_l & 0xFF00) >> 8;
    val[1] = thd_l & 0x00FF;
    ret = stk3328_i2c_write_data(ps_data->client, STK_THDL1_PS_REG, 2, val);

    if (ret < 0)
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, ret);

    return ret;
}
static int32_t stk3328_set_ps_thd_h(struct stk3328_data *ps_data, uint16_t thd_h)
{
    unsigned char val[2];
    int ret;
    val[0] = (thd_h & 0xFF00) >> 8;
    val[1] = thd_h & 0x00FF;
    ret = stk3328_i2c_write_data(ps_data->client, STK_THDH1_PS_REG, 2, val);

    if (ret < 0)
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, ret);

    return ret;
}

#if( !defined(CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD))
static void stk_init_code_threshold_table(struct stk3328_data *ps_data)
{
    uint32_t i, j;
    uint32_t alscode;
    code_threshold_table[0] = 0;
#ifdef STK_DEBUG_PRINTF
    pr_info("[SENSOR] alscode[0]=%d\n", 0);
#endif
    for (i = 1, j = 0; i < LUX_THD_TABLE_SIZE; i++, j++)
    {
        alscode = stk_lux2alscode(ps_data, lux_threshold_table[j]);
        pr_info("[SENSOR] alscode[%d]=%d\n", i, alscode);
        code_threshold_table[i] = (uint16_t)(alscode);
    }
    code_threshold_table[i] = 0xffff;
    pr_info("[SENSOR] alscode[%d]=%d\n", i, alscode);
}

static uint32_t stk_get_lux_interval_index(uint16_t alscode)
{
    uint32_t i;
    for (i = 1; i <= LUX_THD_TABLE_SIZE; i++)
    {
        if ((alscode >= code_threshold_table[i - 1]) && (alscode < code_threshold_table[i]))
        {
            return i;
        }
    }
    return LUX_THD_TABLE_SIZE;
}
#else

#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))
static void stk_als_set_new_thd(struct stk3328_data *ps_data, uint16_t alscode)
{
    int32_t high_thd, low_thd;
    high_thd = alscode + stk_lux2alscode(ps_data, STK_ALS_CHANGE_THD);
    low_thd = alscode - stk_lux2alscode(ps_data, STK_ALS_CHANGE_THD);
    if (high_thd >= (1 << 16))
        high_thd = (1 << 16) - 1;
    if (low_thd < 0)
        low_thd = 0;
    stk3328_set_als_thd_h(ps_data, (uint16_t)high_thd);
    stk3328_set_als_thd_l(ps_data, (uint16_t)low_thd);
}
#endif
#endif // CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD

static void stk3328_proc_plat_data(struct stk3328_data *ps_data, struct stk3328_platform_data *plat_data)
{
    uint8_t w_reg;
    ps_data->state_reg = plat_data->state_reg;
	ps_data->als_cgain = plat_data->als_cgain;
	ps_data->ps_default_trim = plat_data->ps_default_trim;
    ps_data->psctrl_reg = plat_data->psctrl_reg;//0x31
#ifdef STK_POLL_PS
    ps_data->psctrl_reg &= 0x3F;
#endif
    ps_data->alsctrl_reg = plat_data->alsctrl_reg;//0x32
    ps_data->ledctrl_reg = plat_data->ledctrl_reg;
    if (ps_data->pid == STK3325_PID || ps_data->pid == STK3327_PID)
        ps_data->ledctrl_reg &= 0x20;
    else if(ps_data->pid == STK3220_PID)
        ps_data->ledctrl_reg &= 0x80;
    ps_data->wait_reg = plat_data->wait_reg;//0x02
    if (ps_data->wait_reg < 2)
    {
        printk(KERN_WARNING "%s: wait_reg should be larger than 2, force to write 2\n", __func__);
        ps_data->wait_reg = 2;
    }
    else if (ps_data->wait_reg > 0xFF)
    {
        printk(KERN_WARNING "%s: wait_reg should be less than 0xFF, force to write 0xFF\n", __func__);
        ps_data->wait_reg = 0xFF;
    }
    ps_data->int_reg = 0x01;
    //#ifndef STK_TUNE0
    if (ps_data->ps_thd_h == 0 && ps_data->ps_thd_l == 0)
    {
        ps_data->ps_thd_h = plat_data->ps_thd_h;
        ps_data->ps_thd_l = plat_data->ps_thd_l;
    }
    //#endif
#ifdef CALI_PS_EVERY_TIME
    ps_data->ps_high_thd_boot = plat_data->ps_thd_h;
    ps_data->ps_low_thd_boot = plat_data->ps_thd_l;
#endif
    w_reg = 0;
#ifndef STK_POLL_PS

    w_reg |= STK_INT_PS_MODE;

#else
    w_reg |= 0x01;
#endif
#if (!defined(STK_POLL_ALS) && (STK_INT_PS_MODE != 0x02) && (STK_INT_PS_MODE != 0x03))
    w_reg |= STK_INT_ALS;
#endif
    ps_data->int_reg = w_reg;
    return;
}

static int32_t stk3328_init_all_reg(struct stk3328_data *ps_data)
{
    int32_t ret;
    uint8_t reg;
	unsigned char val[2];

    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_STATE_REG, ps_data->state_reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error STK_STATE_REG\n", __func__);
        return ret;
    }
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_PSCTRL_REG, ps_data->psctrl_reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error STK_PSCTRL_REG\n", __func__);
        return ret;
    }
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_ALSCTRL_REG, ps_data->alsctrl_reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error STK_ALSCTRL_REG\n", __func__);
        return ret;
    }
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_LEDCTRL_REG, ps_data->ledctrl_reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_WAIT_REG, ps_data->wait_reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_INT_REG, ps_data->int_reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_ALSC_GAIN_REG, ps_data->als_cgain);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    reg = 0x32;
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_REG_INTELLI_WAIT_PS_REG, reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    //BGIR
    reg = 0x10;
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_BGIR_REG, reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error STK_BGIR_REG\n", __func__);
        return ret;
    }
    reg = 0x54;
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_CI_REG, reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error STK_CI_REG\n", __func__);
        return ret;
    }
	val[0] = (ps_data->ps_default_trim & 0xFF00) >> 8;
    val[1] = ps_data->ps_default_trim  & 0x00FF;
    ret = stk3328_i2c_write_data(ps_data->client, STK_DATA1_OFFSET_REG, 2, val);
	if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error STK_DATA1_OFFSET_REG\n", __func__);
        return ret;
    }
	
#ifdef STK_TUNE0
    ps_data->psa = 0x0;
    ps_data->psi = 0xFFFF;
#endif
    stk3328_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
    stk3328_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_INT_REG, ps_data->int_reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    /*
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, 0x87, 0x60);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    */
    return 0;
}

static int32_t stk3328_check_pid(struct stk3328_data *ps_data)
{
    unsigned char value[3];
    int err;
    err = stk3328_i2c_read_data(ps_data->client, STK_PDT_ID_REG, 2, &value[0]);
    if (err < 0)
    {
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, err);
        return err;
    }
    pr_info("[SENSOR] %s: PID=0x%x, RID=0x%x\n", __func__, value[0], value[1]);
    ps_data->pid = value[0];
    if (value[0] == 0)
    {
        pr_err("[SENSOR] PID=0x0, please make sure the chip is stk3328!\n");
        return -2;
    }
    return 0;
}

static int32_t stk3328_software_reset(struct stk3328_data *ps_data)
{
    int32_t r;
    uint8_t w_reg;
    w_reg = 0x7F;
    r = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_WAIT_REG, w_reg);
    if (r < 0)
    {
        pr_err("[SENSOR] %s: software reset: write i2c error, ret=%d\n", __func__, r);
        return r;
    }
    r = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_WAIT_REG);
    if (w_reg != r)
    {
        pr_err("[SENSOR] %s: software reset: read-back value is not the same\n", __func__);
        return -1;
    }
    r = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_SW_RESET_REG, 0);
    if (r < 0)
    {
        pr_err("[SENSOR] %s: software reset: read error after reset\n", __func__);
        return r;
    }
    usleep_range(13000, 15000);
    return 0;
}

static uint32_t stk3328_get_ps_reading(struct stk3328_data *ps_data)
{
    unsigned char value[2];
    int err;
    err = stk3328_i2c_read_data(ps_data->client, STK_DATA1_PS_REG, 2, &value[0]);
    if (err < 0)
    {
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, err);
        return err;
    }
    ps_data->ps_code_last = ((value[0] << 8) | value[1]);
    return ((value[0] << 8) | value[1]);
}

#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
#if ((STK_INT_PS_MODE != 0x03) && (STK_INT_PS_MODE != 0x02))
static int32_t stk3328_set_flag(struct stk3328_data *ps_data, uint8_t org_flag_reg, uint8_t clr)
{
    uint8_t w_flag;
    int ret;
    w_flag = org_flag_reg | (STK_FLG_ALSINT_MASK | STK_FLG_PSINT_MASK | STK_FLG_ALSSAT_MASK | STK_FLG_INVALIDPS_MASK);
    w_flag &= (~clr);
    //pr_info("[SENSOR] %s: org_flag_reg=0x%x, w_flag = 0x%x\n", __func__, org_flag_reg, w_flag);
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_FLAG_REG, w_flag);
    if (ret < 0)
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, ret);
    return ret;
}
#endif
#endif

static int32_t stk3328_get_flag(struct stk3328_data *ps_data)
{
    int ret;
    ret = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_FLAG_REG);
    if (ret < 0)
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, ret);
    return ret;
}

static int32_t stk3328_set_state(struct stk3328_data *ps_data, uint8_t state)
{
    int ret;
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_STATE_REG, state);
    if (ret < 0)
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, ret);
    return ret;
}

static int32_t stk3328_get_state(struct stk3328_data *ps_data)
{
    int ret;
    ret = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_STATE_REG);
    if (ret < 0)
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, ret);
    return ret;
}




//----
static void stk_ps_bgir_update(struct stk3328_data *ps_data);
static void stk_ps_bgir_judgement(struct stk3328_data *ps_data);
static void stk_ps_bgir_task(struct stk3328_data *ps_data, int nf);

static void stk_ps_bgir_update(struct stk3328_data *ps_data)
{
    ps_data->bgir_update = 0;
    stk3328_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
    stk3328_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
}

static void stk_ps_bgir_judgement(struct stk3328_data *ps_data)
{
    int ret;
    ret = stk3328_i2c_smbus_read_byte_data(ps_data->client, 0xA5);
    if (ret == 0x28)
    {
        uint8_t bgir_data[2];
        bgir_data[0] = stk3328_i2c_smbus_read_byte_data(ps_data->client, 0xB0);
        bgir_data[1] = stk3328_i2c_smbus_read_byte_data(ps_data->client, 0xB1);
        if ((bgir_data[0] < 100) && (bgir_data[1] < 100) && (ps_data->ps_code_last == 0))
        {
            if (ps_data->ps_report == true)
            {
                ps_data->ps_report = false;
            }
            ps_data->ps_code_last = 0xFFFF;
            stk3328_set_ps_thd_h(ps_data, 0x01);
            stk3328_set_ps_thd_l(ps_data, 0x01);
            ps_data->bgir_update = true;
        }
    }
}

static void stk_ps_bgir_task(struct stk3328_data *ps_data, int nf)
{
    if (ps_data->bgir_update == 1)
    {
        stk_ps_bgir_update(ps_data);
    }
    if (nf == 1)
    {
        stk_ps_bgir_judgement(ps_data);
    }
}
//----
static void stk_ps_report(struct stk3328_data *ps_data, int nf)
{
    ps_data->ps_distance_last = true;
    stk_ps_bgir_task(ps_data, nf);
    //if(ps_data->ps_report == 1)
    {
        ps_data->ps_distance_last = nf;
		pr_info("[SENSOR] %s, nf = %d\n",__func__,nf);
        input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, nf);
        input_sync(ps_data->ps_input_dev);
        wake_lock_timeout(&ps_data->ps_wakelock, 3 * HZ);
    }
}

static void stk_als_report(struct stk3328_data *ps_data, int als)
{
	struct timespec ts = ktime_to_timespec(ktime_get_boottime());
	u64 timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi = (int)((timestamp & TIME_HI_MASK) >> TIME_HI_SHIFT);
	int time_lo = (int)(timestamp & TIME_LO_MASK);

	ps_data->als_lux_last = als;
    input_report_rel(ps_data->als_input_dev, REL_DIAL, ps_data->als_data + 1);
	input_report_rel(ps_data->als_input_dev, REL_WHEEL, ps_data->c_data + 1);
	input_report_rel(ps_data->als_input_dev, REL_X, time_hi);
	input_report_rel(ps_data->als_input_dev, REL_Y, time_lo);
	if (ps_data->change_gain){
		input_report_rel(ps_data->als_input_dev, REL_Z, ps_data->gain);
		ps_data->change_gain = false;
	}
    input_sync(ps_data->als_input_dev);
#ifdef STK_DEBUG_PRINTF
    pr_info("[SENSOR] %s: als input event %d lux\n", __func__, als);
#endif
}

static int32_t stk3328_enable_ps(struct stk3328_data *ps_data, uint8_t enable, uint8_t validate_reg)
{
    int32_t ret;
    uint8_t w_state_reg;
    uint8_t curr_ps_enable;
    uint32_t reading;
    int32_t near_far_state;
#ifdef STK_CHK_REG
    if (validate_reg)
    {
        ret = stk3328_validate_n_handle(ps_data->client);
        if (ret < 0)
            pr_err("[SENSOR] stk3328_validate_n_handle fail: %d\n", ret);
    }
#endif /* #ifdef STK_CHK_REG */
    curr_ps_enable = ps_data->ps_enabled ? 1 : 0;
    if (curr_ps_enable == enable)
        return 0;
#ifdef STK_TUNE0
    if (!(ps_data->psi_set) && !enable)
    {
        hrtimer_cancel(&ps_data->ps_tune0_timer);
        cancel_work_sync(&ps_data->stk_ps_tune0_work);
    }
#endif
    if (ps_data->first_boot == true)
    {
        ps_data->first_boot = false;
    }
    if (enable)
    {
        stk3328_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
        stk3328_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
    }
    ret = stk3328_get_state(ps_data);
    if (ret < 0)
        return ret;
    w_state_reg = ret;
    w_state_reg &= ~(STK_STATE_EN_PS_MASK | STK_STATE_EN_WAIT_MASK);
    if (enable)
    {
        w_state_reg |= STK_STATE_EN_PS_MASK;
        if (!(ps_data->als_enabled))
            w_state_reg |= STK_STATE_EN_WAIT_MASK;
    }
    ret = stk3328_set_state(ps_data, w_state_reg);
    if (ret < 0)
        return ret;
    ps_data->state_reg = w_state_reg;
    if (enable)
    {
#ifdef STK_TUNE0
#ifdef CALI_PS_EVERY_TIME
        ps_data->psi_set = 0;
        ps_data->psa = 0;
        ps_data->psi = 0xFFFF;

        ps_data->ps_thd_h = ps_data->ps_high_thd_boot;
        ps_data->ps_thd_l = ps_data->ps_low_thd_boot;

        hrtimer_start(&ps_data->ps_tune0_timer, ps_data->ps_tune0_delay, HRTIMER_MODE_REL);
#else
        if (!(ps_data->psi_set))
            hrtimer_start(&ps_data->ps_tune0_timer, ps_data->ps_tune0_delay, HRTIMER_MODE_REL);
#endif  /* #ifdef CALI_PS_EVERY_TIME */
#endif

#ifdef STK_CHK_REG
        if (!validate_reg)
        {
            reading = stk3328_get_ps_reading(ps_data);
            stk_ps_report(ps_data, 1);
            pr_info("[SENSOR] %s: force report ps input event=1, ps code = %d\n", __func__, reading);
        }
        else
#endif /* #ifdef STK_CHK_REG */
        {
            usleep_range(4000, 5000);
            reading = stk3328_get_ps_reading(ps_data);
            if (reading < 0)
                return reading;

            ret = stk3328_get_flag(ps_data);
            if (ret < 0)
                return ret;
            near_far_state = ret & STK_FLG_NF_MASK;

            stk_ps_report(ps_data, near_far_state);
            pr_info("[SENSOR] %s: ps input event=%d, ps=%d\n", __func__, near_far_state, reading);
        }
#ifdef STK_POLL_PS
        hrtimer_start(&ps_data->ps_timer, ps_data->ps_poll_delay, HRTIMER_MODE_REL);
        ps_data->ps_distance_last = -1;
#endif
#ifndef STK_POLL_PS
#ifndef STK_POLL_ALS
        if (!(ps_data->als_enabled))
#endif  /* #ifndef STK_POLL_ALS */
            enable_irq(ps_data->irq);
#endif  /* #ifndef STK_POLL_PS */
        ps_data->ps_enabled = true;

#ifdef CALI_PS_EVERY_TIME
        if (ps_data->boot_cali == 1 && ps_data->ps_low_thd_boot < 1000 )
        {
            ps_data->ps_thd_h = ps_data->ps_high_thd_boot;
            ps_data->ps_thd_l = ps_data->ps_low_thd_boot;
        }
        else
#endif

        pr_info("[SENSOR] %s: HT=%d, LT=%d\n", __func__, ps_data->ps_thd_h, ps_data->ps_thd_l);
    }
    else
    {
#ifdef STK_POLL_PS
        hrtimer_cancel(&ps_data->ps_timer);
        cancel_work_sync(&ps_data->stk_ps_work);
#else
#ifndef STK_POLL_ALS
        if (!(ps_data->als_enabled))
#endif
            disable_irq(ps_data->irq);
#endif
        ps_data->ps_enabled = false;
    }
    return ret;
}

static int32_t stk3328_enable_als(struct stk3328_data *ps_data, uint8_t enable)
{
    int32_t ret;
    uint8_t w_state_reg;
	uint8_t reg;
    uint8_t curr_als_enable = (ps_data->als_enabled) ? 1 : 0;
    if (curr_als_enable == enable)
        return 0;
#ifndef STK_POLL_ALS
#ifdef STK_IRS
    if (enable && !(ps_data->ps_enabled))
    {
        ret = stk3328_get_ir_reading(ps_data, STK_IRS_IT_REDUCE );
        if (ret > 0)
            ps_data->ir_code = ret;
    }
#endif
    if (enable)
    {
        stk3328_set_als_thd_h(ps_data, 0x0000);
        stk3328_set_als_thd_l(ps_data, 0xFFFF);
    }
#endif
    ret = stk3328_get_state(ps_data);
    if (ret < 0)
        return ret;
    w_state_reg = (uint8_t)(ret & (~(STK_STATE_EN_ALS_MASK | STK_STATE_EN_WAIT_MASK)));
    if (enable)
        w_state_reg |= STK_STATE_EN_ALS_MASK;
    else if (ps_data->ps_enabled)
        w_state_reg |= STK_STATE_EN_WAIT_MASK;
    ret = stk3328_set_state(ps_data, w_state_reg);
    if (ret < 0)
        return ret;
    ps_data->state_reg = w_state_reg;
	ps_data->als_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
    if (enable)
    {
        ps_data->als_enabled = true;
		/* Set the default gain to x64 */
		ps_data->gain = 64;
		ps_data->change_gain = false;
		reg = 0x32;
		ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_ALSCTRL_REG, reg);
		if (ret < 0)
		{
			pr_err("[SENSOR] %s: write i2c error STK_ALSCTRL_REG : %x\n", __func__, reg);
			return ret;
		}
		reg = 0x30;
		ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_ALSCTRL2_REG, reg);
		if (ret < 0)
		{
			pr_err("[SENSOR] %s: write i2c error STK_ALSCTRL2_REG : %x\n", __func__, reg);
			return ret;
		}

#ifdef STK_POLL_ALS
        hrtimer_start(&ps_data->als_timer, ps_data->als_poll_delay, HRTIMER_MODE_REL);
#else
#ifndef STK_POLL_PS
        if (!(ps_data->ps_enabled))
#endif
            enable_irq(ps_data->irq);
#endif
#ifdef STK_IRS
        ps_data->als_data_index = 0;
#endif
    }
    else
    {
        ps_data->als_enabled = false;
#ifdef STK_POLL_ALS
        hrtimer_cancel(&ps_data->als_timer);
        cancel_work_sync(&ps_data->stk_als_work);
#else
#ifndef STK_POLL_PS
        if (!(ps_data->ps_enabled))
#endif
            disable_irq(ps_data->irq);
#endif
    }
    return ret;
}

static int stk3328_als_gain_check(struct stk3328_data *ps_data)
{
	int ret;
	uint8_t reg1, reg2;
	pr_err("[SENSOR] %s:\n", __func__);
	/* Change the gain as per the lux formula */
	if (ps_data->als_data > 60000 || ps_data->c_data > 60000) {
		reg1 = 0x12;
		reg2 = 0x10;
		ps_data->change_gain = true;
		ps_data->gain = 4;
	} else if (ps_data->als_data < 100 || ps_data->c_data < 100) {
		reg1 = 0x32;
		reg2 = 0x30;
		ps_data->change_gain = true;
		ps_data->gain = 64;
	}
	
	if (ps_data->change_gain) {
		ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_ALSCTRL_REG, reg1);
		if (ret < 0)
		{
			pr_err("[SENSOR] %s: write i2c error STK_ALSCTRL_REG : %x\n", __func__, reg1);
			return ret;
		}
		ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_ALSCTRL2_REG, reg2);
		if (ret < 0)
		{
			pr_err("[SENSOR] %s: write i2c error STK_ALSCTRL2_REG : %x\n", __func__, reg2);
			return ret;
		}
	}
	return 0;
}

static int32_t stk3328_get_als_reading(struct stk3328_data *ps_data)
{
    int32_t als_data[4];
    int32_t ir_data = 0;
#ifdef STK_ALS_FIR
    int index;
    int firlen = atomic_read(&ps_data->firlength);
#endif
    unsigned char value[8];
    int ret;
#ifdef STK_IRS
    const int ir_enlarge = 1 << (STK_ALS_READ_IRS_IT_REDUCE - STK_IRS_IT_REDUCE);
#endif
    ret = stk3328_i2c_read_data(ps_data->client, STK_DATA1_RESERVED1_REG, 2, &value[0]);
    ret = stk3328_i2c_read_data(ps_data->client, STK_DATA1_ALS_REG, 2, &value[2]);
    ret = stk3328_i2c_read_data(ps_data->client, STK_DATA1_RESERVED2_REG, 2, &value[4]);
    ret = stk3328_i2c_read_data(ps_data->client, STK_DATA1_C_REG, 2, &value[6]);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s fail, ret=0x%x", __func__, ret);
        return ret;
    }
    als_data[0] = (value[0] << 8 | value[1]);
    als_data[1] = (value[2] << 8 | value[3]);
    als_data[2] = (value[4] << 8 | value[5]);
    als_data[3] = (value[6] << 8 | value[7]);
	
	ps_data->als_data = als_data[1];
	ps_data->c_data = als_data[3];
	
    pr_info("[SENSOR] %s: als_data=%d, als_code_last=%d,ir_data=%d\n", __func__, als_data[1], ps_data->als_code_last, ir_data);
    pr_info("[SENSOR] %s: als_data[0]=%d, als_data[1]=%d,als_data[2]=%d,data[3]=%d\n", __func__, als_data[0], als_data[1], als_data[2], als_data[3]);
    ps_data->als_code_last = als_data[1];
#ifdef STK_ALS_FIR
    if (ps_data->fir.number < firlen)
    {
        ps_data->fir.raw[ps_data->fir.number] = als_data[1];
        ps_data->fir.sum += als_data[1];
        ps_data->fir.number++;
        ps_data->fir.idx++;
    }
    else
    {
        index = ps_data->fir.idx % firlen;
        ps_data->fir.sum -= ps_data->fir.raw[index];
        ps_data->fir.raw[index] = als_data[1];
        ps_data->fir.sum += als_data[1];
        ps_data->fir.idx++;
        als_data[1] = ps_data->fir.sum / firlen;
    }
#endif
    return als_data[1];
}

#if (defined(STK_IRS) && defined(STK_POLL_ALS))
static int stk_als_ir_skip_als(struct stk3328_data *ps_data)
{
    int ret;
    unsigned char value[2];
    if (ps_data->als_data_index < 60000)
        ps_data->als_data_index++;
    else
        ps_data->als_data_index = 0;
    if (    ps_data->als_data_index % 10 == 1)
    {
        ret = stk3328_i2c_read_data(ps_data->client, STK_DATA1_ALS_REG, 2, &value[0]);
        if (ret < 0)
        {
            pr_err("[SENSOR] %s fail, ret=0x%x", __func__, ret);
            return ret;
        }
        return 1;
    }
    return 0;
}

static void stk_als_ir_get_corr(struct stk3328_data *ps_data, int32_t als)
{
    int32_t als_comperator;
    if (ps_data->ir_code)
    {
        ps_data->als_correct_factor = 1000;
        if (als < STK_IRC_MAX_ALS_CODE && als > STK_IRC_MIN_ALS_CODE &&
            ps_data->ir_code > STK_IRC_MIN_IR_CODE)
        {
            als_comperator = als * STK_IRC_ALS_NUMERA / STK_IRC_ALS_DENOMI;
            if (ps_data->ir_code > als_comperator)
                ps_data->als_correct_factor = STK_IRC_ALS_CORREC;
        }
#ifdef STK_DEBUG_PRINTF
        pr_info("[SENSOR] %s: als=%d, ir=%d, als_correct_factor=%d", __func__,
               als, ps_data->ir_code, ps_data->als_correct_factor);
#endif
        ps_data->ir_code = 0;
    }
    return;
}

static int stk_als_ir_run(struct stk3328_data *ps_data)
{
    int ret;
    if (    ps_data->als_data_index % 10 == 0)
    {
        if (ps_data->ps_distance_last != 0 && ps_data->ir_code == 0)
        {
            ret = stk3328_get_ir_reading(ps_data, STK_IRS_IT_REDUCE);
            if (ret > 0)
                ps_data->ir_code = ret;
        }
        return ret;
    }
    return 0;
}

#endif  /* #if (defined(STK_IRS) && defined(STK_POLL_ALS)) */

static int32_t stk3328_get_ir_reading(struct stk3328_data *ps_data, int32_t als_it_reduce)
{
//    int32_t word_data, ret;
//    uint8_t w_reg, retry = 0;
//    uint16_t irs_slp_time = 100;
//    unsigned char value[2];
//    ret = stk3328_set_irs_it_slp(ps_data, &irs_slp_time, als_it_reduce);
//
//    if (ret < 0)
//        goto irs_err_i2c_rw;
//
//    ret = stk3328_get_state(ps_data);
//
//    if (ret < 0)
//        goto irs_err_i2c_rw;
//
//    //w_reg = ret | STK_STATE_EN_IRS_MASK;
//    //ret = stk3328_set_state(ps_data, w_reg);
//
//    if (ret < 0)
//        goto irs_err_i2c_rw;
//
//    msleep(irs_slp_time);
//
//    do
//    {
//        usleep_range(3000, 4000);
//        ret = stk3328_get_flag(ps_data);
//
//        if (ret < 0)
//            goto irs_err_i2c_rw;
//
//        retry++;
//    }
//    while (retry < 10 && ((ret & STK_FLG_INVALIDPS_MASK) == 0));
//
//    if (retry == 10)
//    {
//        pr_err("[SENSOR] %s: ir data is not ready for a long time\n", __func__);
//        ret = -EINVAL;
//        goto irs_err_i2c_rw;
//    }
//
//    //ret = stk3328_set_flag(ps_data, ret, STK_FLG_IR_RDY_MASK);
//
//    if (ret < 0)
//        goto irs_err_i2c_rw;
//
//    ret = stk3328_i2c_read_data(ps_data->client, STK_DATA1_IR_REG, 2, &value[0]);
//
//    if (ret < 0)
//    {
//        pr_err("[SENSOR] %s fail, ret=0x%x", __func__, ret);
//        goto irs_err_i2c_rw;
//    }
//
//    word_data = ((value[0] << 8) | value[1]);
//    //pr_info("[SENSOR] %s: ir=%d\n", __func__, word_data);
//    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_ALSCTRL_REG, ps_data->alsctrl_reg );
//
//    if (ret < 0)
//    {
//        pr_err("[SENSOR] %s: write i2c error\n", __func__);
//        goto irs_err_i2c_rw;
//    }
//
//    return word_data;
//irs_err_i2c_rw:
//    return ret;
    return 0;
}

#ifdef STK_CHK_REG
static int stk3328_chk_reg_valid(struct stk3328_data *ps_data)
{
    unsigned char value[9];
    int err;
    /*
    uint8_t cnt;
    for(cnt=0;cnt<9;cnt++)
    {
        value[cnt] = stk3328_i2c_smbus_read_byte_data(ps_data->client, (cnt+1));
        if(value[cnt] < 0)
        {
            pr_err("[SENSOR] %s fail, ret=%d", __func__, value[cnt]);
            return value[cnt];
        }
    }
    */
    err = stk3328_i2c_read_data(ps_data->client, STK_PSCTRL_REG, 9, &value[0]);
    if (err < 0)
    {
        pr_err("[SENSOR] %s: fail, ret=%d\n", __func__, err);
        return err;
    }
    if (value[0] != ps_data->psctrl_reg)
    {
        pr_err("[SENSOR] %s: invalid reg 0x01=0x%2x\n", __func__, value[0]);
        return 0xFF;
    }
#ifdef STK_IRS
    if ((value[1] != ps_data->alsctrl_reg) && (value[1] != (ps_data->alsctrl_reg - STK_IRS_IT_REDUCE))
        && (value[1] != (ps_data->alsctrl_reg - STK_ALS_READ_IRS_IT_REDUCE)))
#else
    if ((value[1] != ps_data->alsctrl_reg) && (value[1] != (ps_data->alsctrl_reg - STK_ALS_READ_IRS_IT_REDUCE)))
#endif
    {
        pr_err("[SENSOR] %s: invalid reg 0x02=0x%2x\n", __func__, value[1]);
        return 0xFF;
    }
    if (value[2] != ps_data->ledctrl_reg)
    {
        pr_err("[SENSOR] %s: invalid reg 0x03=0x%2x\n", __func__, value[2]);
        return 0xFF;
    }
    if (value[3] != ps_data->int_reg)
    {
        pr_err("[SENSOR] %s: invalid reg 0x04=0x%2x\n", __func__, value[3]);
        return 0xFF;
    }
    if (value[4] != ps_data->wait_reg)
    {
        pr_err("[SENSOR] %s: invalid reg 0x05=0x%2x\n", __func__, value[4]);
        return 0xFF;
    }
    if (value[5] != ((ps_data->ps_thd_h & 0xFF00) >> 8))
    {
        pr_err("[SENSOR] %s: invalid reg 0x06=0x%2x\n", __func__, value[5]);
        return 0xFF;
    }
    if (value[6] != (ps_data->ps_thd_h & 0x00FF))
    {
        pr_err("[SENSOR] %s: invalid reg 0x07=0x%2x\n", __func__, value[6]);
        return 0xFF;
    }
    if (value[7] != ((ps_data->ps_thd_l & 0xFF00) >> 8))
    {
        pr_err("[SENSOR] %s: invalid reg 0x08=0x%2x\n", __func__, value[7]);
        return 0xFF;
    }
    if (value[8] != (ps_data->ps_thd_l & 0x00FF))
    {
        pr_err("[SENSOR] %s: invalid reg 0x09=0x%2x\n", __func__, value[8]);
        return 0xFF;
    }
    return 0;
}

static int stk3328_validate_n_handle(struct i2c_client *client)
{
    struct stk3328_data *ps_data = i2c_get_clientdata(client);
    int err;
    err = stk3328_chk_reg_valid(ps_data);
    if (err < 0)
    {
        pr_err("[SENSOR] stk3328_chk_reg_valid fail: %d\n", err);
        return err;
    }
    if (err == 0xFF)
    {
        pr_err("[SENSOR] %s: Re-init chip\n", __func__);
        err = stk3328_software_reset(ps_data);
        if (err < 0)
            return err;
        err = stk3328_init_all_reg(ps_data);
        if (err < 0)
            return err;
        //ps_data->psa = 0;
        //ps_data->psi = 0xFFFF;
        stk3328_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
        stk3328_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
#ifdef STK_ALS_FIR
        memset(&ps_data->fir, 0x00, sizeof(ps_data->fir));
#endif
        return 0xFF;
    }
    return 0;
}
#endif /* #ifdef STK_CHK_REG */

static ssize_t stk_als_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    int32_t reading;
#ifdef STK_POLL_ALS
    reading = ps_data->als_code_last;
#else
    unsigned char value[2];
    int ret;
    ret = stk3328_i2c_read_data(ps_data->client, STK_DATA1_ALS_REG, 2, &value[0]);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s fail, ret=0x%x", __func__, ret);
        return ret;
    }
    reading = (value[0] << 8) | value[1];
#endif
    return scnprintf(buf, PAGE_SIZE, "%d\n", reading);
}

static ssize_t stk_als_vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{    
    return scnprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t stk_als_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{    
    return scnprintf(buf, PAGE_SIZE, "%s\n", DEVICE_NAME);
}

static ssize_t stk_als_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    int32_t ret;
    ret = stk3328_get_state(ps_data);
    if (ret < 0)
        return ret;
    ret = (ret & STK_STATE_EN_ALS_MASK) ? 1 : 0;
    return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t stk_als_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data = dev_get_drvdata(dev);
    uint8_t en;
    if (sysfs_streq(buf, "1"))
        en = 1;
    else if (sysfs_streq(buf, "0"))
        en = 0;
    else
    {
        pr_err("[SENSOR] %s, invalid value %d\n", __func__, *buf);
        return -EINVAL;
    }
    pr_info("[SENSOR] %s: Enable ALS : %d\n", __func__, en);
    mutex_lock(&ps_data->io_lock);
    stk3328_enable_als(ps_data, en);
    mutex_unlock(&ps_data->io_lock);
    ps_data->als_en_hal = en ? true : false;
    return size;
}

static ssize_t stk_als_poll_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return 0;
}

static ssize_t stk_als_poll_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	return 0;
}

static ssize_t stk_als_lux_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data = dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "%u,%u\n", ps_data->als_data, ps_data->c_data);
}

static ssize_t stk_als_raw_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data = dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "%u,%u\n", ps_data->als_data, ps_data->c_data);
}

static ssize_t stk_als_lux_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    ret = kstrtoul(buf, 16, &value);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    stk_als_report(ps_data, value);
    return size;
}

static ssize_t stk_als_transmittance_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    int32_t transmittance;
    transmittance = ps_data->als_transmittance;
    return scnprintf(buf, PAGE_SIZE, "%d\n", transmittance);
}

static ssize_t stk_als_transmittance_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    ret = kstrtoul(buf, 10, &value);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    ps_data->als_transmittance = value;
    return size;
}

static ssize_t stk_als_ir_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    int32_t reading;
    reading = stk3328_get_ir_reading(ps_data, STK_IRS_IT_REDUCE);
    return scnprintf(buf, PAGE_SIZE, "%d\n", reading);
}

#ifdef STK_ALS_FIR
static ssize_t stk_als_firlen_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    int len = atomic_read(&ps_data->firlength);
    pr_info("[SENSOR] %s: len = %2d, idx = %2d\n", __func__, len, ps_data->fir.idx);
    pr_info("[SENSOR] %s: sum = %5d, ave = %5d\n", __func__, ps_data->fir.sum, ps_data->fir.sum / len);
    return scnprintf(buf, PAGE_SIZE, "%d\n", len);
}

static ssize_t stk_als_firlen_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    uint64_t value = 0;
    int ret;
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    ret = kstrtoull(buf, 10, &value);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s:kstrtoull failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    if (value > MAX_FIR_LEN)
    {
        pr_err("[SENSOR] %s: firlen exceed maximum filter length\n", __func__);
    }
    else if (value < 1)
    {
        atomic_set(&ps_data->firlength, 1);
        memset(&ps_data->fir, 0x00, sizeof(ps_data->fir));
    }
    else
    {
        atomic_set(&ps_data->firlength, value);
        memset(&ps_data->fir, 0x00, sizeof(ps_data->fir));
    }
    return size;
}
#endif  /* #ifdef STK_ALS_FIR */

static ssize_t stk_ps_raw_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    uint32_t reading;
    reading = stk3328_get_ps_reading(ps_data);
    return scnprintf(buf, PAGE_SIZE, "%d\n", reading);
}

static ssize_t stk_prox_cal_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%u,%u,%u\n", ps_data->ps_default_trim, ps_data->ps_thd_h, ps_data->ps_thd_l);
}

static ssize_t stk_ps_vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{    
    return scnprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t stk_ps_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{    
    return scnprintf(buf, PAGE_SIZE, "%s\n", DEVICE_NAME);
}

static ssize_t stk_ps_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int32_t ret;
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    ret = stk3328_get_state(ps_data);
    if (ret < 0)
        return ret;
    ret = (ret & STK_STATE_EN_PS_MASK) ? 1 : 0;
    return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t stk_ps_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    uint8_t en;
    if (sysfs_streq(buf, "1"))
        en = 1;
    else if (sysfs_streq(buf, "0"))
        en = 0;
    else
    {
        pr_err("[SENSOR] %s, invalid value %d\n", __func__, *buf);
        return -EINVAL;
    }
    pr_info("[SENSOR] %s: Enable PS : %d\n", __func__, en);
    mutex_lock(&ps_data->io_lock);
    stk3328_enable_ps(ps_data, en, 0);
    mutex_unlock(&ps_data->io_lock);
    return size;
}

static ssize_t stk_ps_enable_aso_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int32_t ret;
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    ret = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_STATE_REG);
    return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t stk_ps_enable_aso_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    //struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    //uint8_t en;
    //int32_t ret;
    //uint8_t w_state_reg;
    //if (sysfs_streq(buf, "1"))
    //    en = 1;
    //else if (sysfs_streq(buf, "0"))
    //    en = 0;
    //else
    //{
    //    pr_err("[SENSOR] %s, invalid value %d\n", __func__, *buf);
    //    return -EINVAL;
    //}
    //pr_info("[SENSOR] %s: Enable PS ASO : %d\n", __func__, en);
    //ret = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_STATE_REG);
    //if (ret < 0)
    //{
    //    pr_err("[SENSOR] %s: write i2c error\n", __func__);
    //    return ret;
    //}
    //w_state_reg = (uint8_t)(ret & (~STK_STATE_EN_ASO_MASK));
    //if (en)
    //    w_state_reg |= STK_STATE_EN_ASO_MASK;
    //ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_STATE_REG, w_state_reg);
    //if (ret < 0)
    //{
    //    pr_err("[SENSOR] %s: write i2c error\n", __func__);
    //    return ret;
    //}
    //return size;
    return 0;
}

static ssize_t stk_ps_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    int32_t word_data;
    unsigned char value[2];
    int ret;
    ret = stk3328_i2c_read_data(ps_data->client, STK_DATA1_OFFSET_REG, 2, &value[0]);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s fail, ret=0x%x", __func__, ret);
        return ret;
    }
    word_data = (value[0] << 8) | value[1];
    return scnprintf(buf, PAGE_SIZE, "%d\n", word_data);
}

static ssize_t stk_ps_offset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    unsigned long offset = 0;
    int ret;
    unsigned char val[2];
    ret = kstrtoul(buf, 10, &offset);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    if (offset > 65535)
    {
        pr_err("[SENSOR] %s: invalid value, offset=%ld\n", __func__, offset);
        return -EINVAL;
    }
    val[0] = (offset & 0xFF00) >> 8;
    val[1] = offset & 0x00FF;
    ret = stk3328_i2c_write_data(ps_data->client, STK_DATA1_OFFSET_REG, 2, val);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    return size;
}

static ssize_t stk_ps_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct stk3328_data *ps_data =  dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", ps_data->avg[0],
		ps_data->avg[1], ps_data->avg[2]);
}

static void proximity_get_avg_val(struct stk3328_data *ps_data)
{
	int min = 0, max = 0, avg = 0;
	int i;
	uint32_t read_value;

	for (i = 0; i < PROX_READ_NUM; i++) {
		msleep(40);
		read_value = stk3328_get_ps_reading(ps_data);
		avg += read_value;

		if (!i)
			min = read_value;
		else if (read_value < min)
			min = read_value;

		if (read_value > max)
			max = read_value;
	}
	avg /= PROX_READ_NUM;

	ps_data->avg[0] = min;
	ps_data->avg[1] = avg;
	ps_data->avg[2] = max;
	
	pr_info("[SENSOR] %s, min = %d, avg = %d, max = %d", __func__, ps_data->avg[0], ps_data->avg[1], ps_data->avg[2]);
}

static ssize_t stk_ps_avg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3328_data *ps_data = dev_get_drvdata(dev);
	bool new_value = false;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		SENSOR_ERR("invalid value %d\n", *buf);
		return -EINVAL;
	}

	SENSOR_INFO("average enable = %d\n",  new_value);
	if (new_value) {
		if ((ps_data->ps_enabled ? 1 : 0) == OFF) {
			mutex_lock(&ps_data->io_lock);
			stk3328_enable_ps(ps_data, new_value, 1);
			mutex_unlock(&ps_data->io_lock);
		}
		hrtimer_start(&ps_data->prox_timer, ps_data->prox_poll_delay,
			HRTIMER_MODE_REL);
	} else if (!new_value) {
		hrtimer_cancel(&ps_data->prox_timer);
		cancel_work_sync(&ps_data->work_prox);
		if ((ps_data->ps_enabled ? 1 : 0) == OFF) {
			mutex_lock(&ps_data->io_lock);
			stk3328_enable_ps(ps_data, new_value, 0);
			mutex_unlock(&ps_data->io_lock);
		}
	}

	return size;
}

static void stk3328_work_func_prox(struct work_struct *work)
{
	struct stk3328_data *ps_data = container_of(work,
		struct stk3328_data, work_prox);

	proximity_get_avg_val(ps_data);
}

static enum hrtimer_restart stk3328_prox_timer_func(struct hrtimer *timer)
{
	struct stk3328_data *ps_data = container_of(timer,
		struct stk3328_data, prox_timer);

	queue_work(ps_data->prox_wq, &ps_data->work_prox);
	hrtimer_forward_now(&ps_data->prox_timer, ps_data->prox_poll_delay);
	return HRTIMER_RESTART;
}

static ssize_t stk_ps_distance_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    int32_t dist = 1;
    int32_t ret;

    ret = stk3328_get_flag(ps_data);
    if (ret < 0)
        return ret;
    dist = (ret & STK_FLG_NF_MASK) ? 1 : 0;

    stk_ps_report(ps_data, dist);
    pr_info("[SENSOR] %s: ps input event=%d\n", __func__, dist);
    return scnprintf(buf, PAGE_SIZE, "%d\n", dist);
}

static ssize_t stk_ps_distance_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    ret = kstrtoul(buf, 10, &value);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    stk_ps_report(ps_data, value);
    pr_info("[SENSOR] %s: ps input event=%d\n", __func__, (int)value);
    return size;
}

static ssize_t stk_ps_code_thd_l_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int32_t ps_thd_l1_reg, ps_thd_l2_reg;
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    ps_thd_l1_reg = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_THDL1_PS_REG);
    if (ps_thd_l1_reg < 0)
    {
        pr_err("[SENSOR] %s fail, err=0x%x", __func__, ps_thd_l1_reg);
        return -EINVAL;
    }
    ps_thd_l2_reg = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_THDL2_PS_REG);
    if (ps_thd_l2_reg < 0)
    {
        pr_err("[SENSOR] %s fail, err=0x%x", __func__, ps_thd_l2_reg);
        return -EINVAL;
    }
    ps_thd_l1_reg = ps_thd_l1_reg << 8 | ps_thd_l2_reg;
    return scnprintf(buf, PAGE_SIZE, "%d\n", ps_thd_l1_reg);
}

static ssize_t stk_ps_code_thd_l_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    ret = kstrtoul(buf, 10, &value);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    stk3328_set_ps_thd_l(ps_data, value);
    return size;
}

static ssize_t stk_ps_code_thd_h_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int32_t ps_thd_h1_reg, ps_thd_h2_reg;
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    ps_thd_h1_reg = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_THDH1_PS_REG);
    if (ps_thd_h1_reg < 0)
    {
        pr_err("[SENSOR] %s fail, err=0x%x", __func__, ps_thd_h1_reg);
        return -EINVAL;
    }
    ps_thd_h2_reg = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_THDH2_PS_REG);
    if (ps_thd_h2_reg < 0)
    {
        pr_err("[SENSOR] %s fail, err=0x%x", __func__, ps_thd_h2_reg);
        return -EINVAL;
    }
    ps_thd_h1_reg = ps_thd_h1_reg << 8 | ps_thd_h2_reg;
    return scnprintf(buf, PAGE_SIZE, "%d\n", ps_thd_h1_reg);
}

static ssize_t stk_ps_code_thd_h_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    ret = kstrtoul(buf, 10, &value);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    stk3328_set_ps_thd_h(ps_data, value);
    return size;
}

static ssize_t stk_all_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int32_t ps_reg[0x23];
    uint8_t cnt;
    int len = 0;
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    for (cnt = 0; cnt < 0x20; cnt++)
    {
        ps_reg[cnt] = stk3328_i2c_smbus_read_byte_data(ps_data->client, (cnt));
        if (ps_reg[cnt] < 0)
        {
            pr_err("[SENSOR] %s fail, ret=%d", __func__, ps_reg[cnt]);
            return -EINVAL;
        }
        else
        {
            pr_info("[SENSOR] reg[0x%2X]=0x%2X\n", cnt, ps_reg[cnt]);
            len += scnprintf(buf + len, PAGE_SIZE - len, "[%2X]%2X,\n", cnt, ps_reg[cnt]);
        }
    }
    ps_reg[cnt] = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_PDT_ID_REG);
    if (ps_reg[cnt] < 0)
    {
        printk( KERN_ERR "%s fail, ret=%d", __func__, ps_reg[cnt]);
        return -EINVAL;
    }
    printk( KERN_INFO "reg[0x%x]=0x%2X\n", STK_PDT_ID_REG, ps_reg[cnt]);
    cnt++;
    ps_reg[cnt] = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_RSRVD_REG);
    if (ps_reg[cnt] < 0)
    {
        printk( KERN_ERR "%s fail, ret=%d", __func__, ps_reg[cnt]);
        return -EINVAL;
    }
    pr_info("[SENSOR] reg[0x%x]=0x%2X\n", STK_RSRVD_REG, ps_reg[cnt]);
    cnt++;
    ps_reg[cnt] = stk3328_i2c_smbus_read_byte_data(ps_data->client, 0xE0);
    if (ps_reg[cnt] < 0)
    {
        printk( KERN_ERR "%s fail, ret=%d", __func__, ps_reg[cnt]);
        return -EINVAL;
    }
    pr_info("[SENSOR] reg[0xE0]=0x%2X\n", ps_reg[cnt]);
    len += scnprintf(buf + len, PAGE_SIZE - len, "[3E]%2X,[3F]%2X,[E0]%2X\n", ps_reg[cnt - 2], ps_reg[cnt - 1], ps_reg[cnt]);
    return len;
    /*
        return scnprintf(buf, PAGE_SIZE, "[0]%2X [1]%2X [2]%2X [3]%2X [4]%2X [5]%2X [6/7 HTHD]%2X,%2X [8/9 LTHD]%2X, %2X [A]%2X [B]%2X [C]%2X [D]%2X [E/F Aoff]%2X,%2X,[10]%2X [11/12 PS]%2X,%2X [13]%2X [14]%2X [15/16 Foff]%2X,%2X [17]%2X [18]%2X [3E]%2X [3F]%2X\n",
            ps_reg[0], ps_reg[1], ps_reg[2], ps_reg[3], ps_reg[4], ps_reg[5], ps_reg[6], ps_reg[7], ps_reg[8],
            ps_reg[9], ps_reg[10], ps_reg[11], ps_reg[12], ps_reg[13], ps_reg[14], ps_reg[15], ps_reg[16], ps_reg[17],
            ps_reg[18], ps_reg[19], ps_reg[20], ps_reg[21], ps_reg[22], ps_reg[23], ps_reg[24], ps_reg[25], ps_reg[26]);
            */
}

static ssize_t stk_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int32_t ps_reg[27];
    uint8_t cnt;
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    for (cnt = 0; cnt < 25; cnt++)
    {
        ps_reg[cnt] = stk3328_i2c_smbus_read_byte_data(ps_data->client, (cnt));
        if (ps_reg[cnt] < 0)
        {
            pr_err("[SENSOR] %s fail, ret=%d", __func__, ps_reg[cnt]);
            return -EINVAL;
        }
        else
        {
            pr_info("[SENSOR] reg[0x%2X]=0x%2X\n", cnt, ps_reg[cnt]);
        }
    }
    ps_reg[cnt] = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_PDT_ID_REG);
    if (ps_reg[cnt] < 0)
    {
        printk( KERN_ERR "%s fail, ret=%d", __func__, ps_reg[cnt]);
        return -EINVAL;
    }
    printk( KERN_INFO "reg[0x%x]=0x%2X\n", STK_PDT_ID_REG, ps_reg[cnt]);
    cnt++;
    ps_reg[cnt] = stk3328_i2c_smbus_read_byte_data(ps_data->client, STK_RSRVD_REG);
    if (ps_reg[cnt] < 0)
    {
        printk( KERN_ERR "%s fail, ret=%d", __func__, ps_reg[cnt]);
        return -EINVAL;
    }
    printk( KERN_INFO "reg[0x%x]=0x%2X\n", STK_RSRVD_REG, ps_reg[cnt]);
    return scnprintf(buf, PAGE_SIZE, "[PS=%2X] [ALS=%2X] [WAIT=0x%4Xms] [EN_ASO=%2X] [EN_AK=%2X] [NEAR/FAR=%2X] [FLAG_OUI=%2X] [FLAG_PSINT=%2X] [FLAG_ALSINT=%2X]\n",
                     ps_reg[0] & 0x01, (ps_reg[0] & 0x02) >> 1, ((ps_reg[0] & 0x04) >> 2) * ps_reg[5] * 6, (ps_reg[0] & 0x20) >> 5,
                     (ps_reg[0] & 0x40) >> 6, ps_reg[16] & 0x01, (ps_reg[16] & 0x04) >> 2, (ps_reg[16] & 0x10) >> 4, (ps_reg[16] & 0x20) >> 5);
}

static ssize_t stk_recv_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&ps_data->recv_reg));
}

static ssize_t stk_recv_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    unsigned long value = 0;
    int ret;
    int32_t recv_data;
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    if ((ret = kstrtoul(buf, 16, &value)) < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    recv_data = stk3328_i2c_smbus_read_byte_data(ps_data->client, value);
    //  printk("%s: reg 0x%x=0x%x\n", __func__, (int)value, recv_data);
    atomic_set(&ps_data->recv_reg, recv_data);
    return size;
}

static ssize_t stk_send_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return 0;
}

static ssize_t stk_send_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int addr, cmd;
    int32_t ret, i;
    char *token[10];
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    for (i = 0; i < 2; i++)
        token[i] = strsep((char **)&buf, " ");
    if ((ret = kstrtoul(token[0], 16, (unsigned long *) & (addr))) < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    if ((ret = kstrtoul(token[1], 16, (unsigned long *) & (cmd))) < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    pr_info("[SENSOR] %s: write reg 0x%x=0x%x\n", __func__, addr, cmd);
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, (unsigned char)addr, (unsigned char)cmd);
    if (0 != ret)
    {
        pr_err("[SENSOR] %s: stk3328_i2c_smbus_write_byte_data fail\n", __func__);
        return ret;
    }
    return size;
}

#ifdef STK_TUNE0
static ssize_t stk_ps_cali_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    int32_t word_data;
    unsigned char value[2];
    int ret;
    ret = stk3328_i2c_read_data(ps_data->client, 0x20, 2, &value[0]);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s fail, ret=0x%x", __func__, ret);
        return ret;
    }
    word_data = (value[0] << 8) | value[1];
    ret = stk3328_i2c_read_data(ps_data->client, 0x22, 2, &value[0]);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s fail, ret=0x%x", __func__, ret);
        return ret;
    }
    word_data += ((value[0] << 8) | value[1]);
    printk("%s: psi_set=%d, psa=%d,psi=%d, word_data=%d\n", __func__,
           ps_data->psi_set, ps_data->psa, ps_data->psi, word_data);
#ifdef CALI_PS_EVERY_TIME
    printk("%s: boot HT=%d, LT=%d\n", __func__, ps_data->ps_high_thd_boot, ps_data->ps_low_thd_boot);
#endif
    return 0;
}

static ssize_t stk_ps_maxdiff_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    if ((ret = kstrtoul(buf, 10, &value)) < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    ps_data->stk_max_min_diff = (int) value;
    return size;
}

static ssize_t stk_ps_maxdiff_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d\n", ps_data->stk_max_min_diff);
}

static ssize_t stk_ps_ltnct_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    if ((ret = kstrtoul(buf, 10, &value)) < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    ps_data->stk_lt_n_ct = (int) value;
    return size;
}

static ssize_t stk_ps_ltnct_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d\n", ps_data->stk_lt_n_ct);
}

static ssize_t stk_ps_htnct_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    unsigned long value = 0;
    int ret;
    if ((ret = kstrtoul(buf, 10, &value)) < 0)
    {
        pr_err("[SENSOR] %s:kstrtoul failed, ret=0x%x\n", __func__, ret);
        return ret;
    }
    ps_data->stk_ht_n_ct = (int) value;
    return size;
}

static ssize_t stk_ps_htnct_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct stk3328_data *ps_data =  dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d\n", ps_data->stk_ht_n_ct);
}
#endif  /* #ifdef STK_TUNE0 */

static struct device_attribute als_enable_attribute = __ATTR(enable,0664,stk_als_enable_show,stk_als_enable_store);
static struct device_attribute als_poll_delay_attribute = __ATTR(poll_delay,0664,stk_als_poll_delay_show,stk_als_poll_delay_store);
static struct device_attribute als_lux_attribute = __ATTR(lux,0664,stk_als_lux_show,stk_als_lux_store);
static struct device_attribute als_raw_data_attribute = __ATTR(raw_data,0444,stk_als_raw_data_show,NULL);
static struct device_attribute als_vendor_attribute = __ATTR(vendor, 0444, stk_als_vendor_show, NULL);
static struct device_attribute als_name_attribute = __ATTR(name, 0444, stk_als_name_show, NULL);
static struct device_attribute als_code_attribute = __ATTR(code, 0444, stk_als_code_show, NULL);
static struct device_attribute als_transmittance_attribute = __ATTR(transmittance, 0664, stk_als_transmittance_show, stk_als_transmittance_store);
static struct device_attribute als_ir_code_attribute = __ATTR(ircode, 0444, stk_als_ir_code_show, NULL);
#ifdef STK_ALS_FIR
    static struct device_attribute als_firlen_attribute = __ATTR(firlen, 0664, stk_als_firlen_show, stk_als_firlen_store);
#endif

static struct attribute *stk_als_attrs [] =
{
    &als_enable_attribute.attr,
    &als_poll_delay_attribute.attr,
    NULL
};

static struct device_attribute *light_sensor_attrs [] =
{
    &als_vendor_attribute,
    &als_name_attribute,
    &als_lux_attribute,
	&als_raw_data_attribute,
    &als_code_attribute,
    &als_transmittance_attribute,
    &als_ir_code_attribute,
#ifdef STK_ALS_FIR
    &als_firlen_attribute,
#endif
    NULL
};

static struct attribute_group stk_als_attribute_group =
{
    .attrs = stk_als_attrs,
};

static struct device_attribute ps_enable_attribute = __ATTR(enable,0664,stk_ps_enable_show,stk_ps_enable_store);
static struct device_attribute ps_enable_aso_attribute = __ATTR(enableaso,0664,stk_ps_enable_aso_show,stk_ps_enable_aso_store);
static struct device_attribute ps_state_attribute = __ATTR(state,0664,stk_ps_distance_show, stk_ps_distance_store);
static struct device_attribute ps_default_trim_attribute = __ATTR(prox_trim,0664,stk_ps_offset_show, stk_ps_offset_store);
static struct device_attribute ps_prox_avg_attribute = __ATTR(prox_avg,0664,stk_ps_avg_show, stk_ps_avg_store);
static struct device_attribute ps_raw_data_attribute = __ATTR(raw_data,0444, stk_ps_raw_data_show, NULL);
static struct device_attribute ps_prox_cal_attribute = __ATTR(prox_cal,0444, stk_prox_cal_data_show, NULL);
static struct device_attribute ps_name_attribute = __ATTR(name, 0444, stk_ps_name_show, NULL);
static struct device_attribute ps_vendor_attribute = __ATTR(vendor, 0444, stk_ps_vendor_show, NULL);
static struct device_attribute ps_thresh_low_attribute = __ATTR(thresh_low,0664,stk_ps_code_thd_l_show,stk_ps_code_thd_l_store);
static struct device_attribute ps_thresh_high_attribute = __ATTR(thresh_high,0664,stk_ps_code_thd_h_show,stk_ps_code_thd_h_store);
static struct device_attribute ps_recv_attribute = __ATTR(recv,0664,stk_recv_show,stk_recv_store);
static struct device_attribute ps_send_attribute = __ATTR(send,0664,stk_send_show, stk_send_store);
static struct device_attribute all_reg_attribute = __ATTR(allreg, 0444, stk_all_reg_show, NULL);
static struct device_attribute status_attribute = __ATTR(status, 0444, stk_status_show, NULL);
#ifdef STK_TUNE0
    static struct device_attribute ps_cali_attribute = __ATTR(cali, 0444, stk_ps_cali_show, NULL);
    static struct device_attribute ps_maxdiff_attribute = __ATTR(maxdiff, 0664, stk_ps_maxdiff_show, stk_ps_maxdiff_store);
    static struct device_attribute ps_ltnct_attribute = __ATTR(ltnct, 0664, stk_ps_ltnct_show, stk_ps_ltnct_store);
    static struct device_attribute ps_htnct_attribute = __ATTR(htnct, 0664, stk_ps_htnct_show, stk_ps_htnct_store);
#endif

static struct attribute *stk_ps_attrs [] =
{
    &ps_enable_attribute.attr,
    NULL
};

static struct device_attribute *prox_sensor_attrs [] =
{
    &ps_enable_aso_attribute,
    &ps_name_attribute,
    &ps_vendor_attribute,
    &ps_state_attribute,
    &ps_default_trim_attribute,
    &ps_raw_data_attribute,
    &ps_prox_cal_attribute,
    &ps_thresh_low_attribute,
    &ps_thresh_high_attribute,
    &ps_prox_avg_attribute,
    &ps_recv_attribute,
    &ps_send_attribute,
    &all_reg_attribute,
    &status_attribute,
#ifdef STK_TUNE0
    &ps_cali_attribute,
    &ps_maxdiff_attribute,
    &ps_ltnct_attribute,
    &ps_htnct_attribute,
#endif
    NULL
};

static struct attribute_group stk_ps_attribute_group =
{
    .attrs = stk_ps_attrs,
};

#ifdef STK_TUNE0
static int stk_ps_val(struct stk3328_data *ps_data)
{
    int mode;
    int32_t word_data, lii;
    unsigned char value[4];
    int ret;
    ret = stk3328_i2c_read_data(ps_data->client, 0x20, 4, value);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s fail, ret=0x%x", __func__, ret);
        return ret;
    }
    word_data = (value[0] << 8) | value[1];
    word_data += ((value[2] << 8) | value[3]);
    mode = (ps_data->psctrl_reg) & 0x3F;
    if (mode == 0x30)
        lii = 100;
    else if (mode == 0x31)
        lii = 200;
    else if (mode == 0x32)
        lii = 400;
    else if (mode == 0x33)
        lii = 800;
    else
    {
        pr_err("[SENSOR] %s: unsupported PS_IT(0x%x)\n", __func__, mode);
        return -1;
    }
    if (word_data > lii)
    {
        pr_info("[SENSOR] %s: word_data=%d, lii=%d\n", __func__, word_data, lii);
        return 0xFFFF;
    }
    return 0;
}

static int stk_ps_tune_zero_final(struct stk3328_data *ps_data)
{
    int ret;
    ps_data->tune_zero_init_proc = false;
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_INT_REG, ps_data->int_reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_STATE_REG, 0);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    if (ps_data->data_count == -1)
    {
        pr_info("[SENSOR] %s: exceed limit\n", __func__);
        hrtimer_cancel(&ps_data->ps_tune0_timer);
        return 0;
    }
    ps_data->psa = ps_data->ps_stat_data[0];
    ps_data->psi = ps_data->ps_stat_data[2];
#ifdef CALI_PS_EVERY_TIME
    ps_data->ps_high_thd_boot = ps_data->ps_stat_data[1] + ps_data->stk_ht_n_ct * 3;
    ps_data->ps_low_thd_boot = ps_data->ps_stat_data[1] + ps_data->stk_lt_n_ct * 3;
    ps_data->ps_thd_h = ps_data->ps_high_thd_boot ;
    ps_data->ps_thd_l = ps_data->ps_low_thd_boot ;
#else
    ps_data->ps_thd_h = ps_data->ps_stat_data[1] + ps_data->stk_ht_n_ct;
    ps_data->ps_thd_l = ps_data->ps_stat_data[1] + ps_data->stk_lt_n_ct;
#endif
    stk3328_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
    stk3328_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
    ps_data->boot_cali = 1;
    pr_info("[SENSOR] %s: set HT=%d,LT=%d\n", __func__, ps_data->ps_thd_h,  ps_data->ps_thd_l);
    hrtimer_cancel(&ps_data->ps_tune0_timer);
    return 0;
}

static int32_t stk_tune_zero_get_ps_data(struct stk3328_data *ps_data)
{
    uint32_t ps_adc;
    int ret;
    ret = stk_ps_val(ps_data);
    if (ret == 0xFFFF)
    {
        ps_data->data_count = -1;
        stk_ps_tune_zero_final(ps_data);
        return 0;
    }
    ps_adc = stk3328_get_ps_reading(ps_data);
    pr_info("[SENSOR] %s: ps_adc #%d=%d\n", __func__, ps_data->data_count, ps_adc);
    if (ps_adc < 0)
        return ps_adc;
    ps_data->ps_stat_data[1]  +=  ps_adc;
    if (ps_adc > ps_data->ps_stat_data[0])
        ps_data->ps_stat_data[0] = ps_adc;
    if (ps_adc < ps_data->ps_stat_data[2])
        ps_data->ps_stat_data[2] = ps_adc;
    ps_data->data_count++;
    if (ps_data->data_count == 5)
    {
        ps_data->ps_stat_data[1]  /= ps_data->data_count;
        stk_ps_tune_zero_final(ps_data);
    }
    return 0;
}

static int stk_ps_tune_zero_init(struct stk3328_data *ps_data)
{
    int32_t ret = 0;
    uint8_t w_state_reg;
#ifdef CALI_EVERY_TIME
    ps_data->ps_high_thd_boot = ps_data->ps_thd_h;
    ps_data->ps_low_thd_boot = ps_data->ps_thd_l;
    if (ps_data->ps_high_thd_boot <= 0)
    {
        ps_data->ps_high_thd_boot = ps_data->stk_ht_n_ct * 3;
        ps_data->ps_low_thd_boot = ps_data->stk_lt_n_ct * 3;
    }
#endif
    ps_data->psi_set = 0;
    ps_data->ps_stat_data[0] = 0;
    ps_data->ps_stat_data[2] = 9999;
    ps_data->ps_stat_data[1] = 0;
    ps_data->data_count = 0;
    ps_data->boot_cali = 0;
    ps_data->tune_zero_init_proc = true;
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_INT_REG, 0);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    w_state_reg = (STK_STATE_EN_PS_MASK | STK_STATE_EN_WAIT_MASK);
    ret = stk3328_i2c_smbus_write_byte_data(ps_data->client, STK_STATE_REG, w_state_reg);
    if (ret < 0)
    {
        pr_err("[SENSOR] %s: write i2c error\n", __func__);
        return ret;
    }
    hrtimer_start(&ps_data->ps_tune0_timer, ps_data->ps_tune0_delay, HRTIMER_MODE_REL);
    return 0;
}

static int stk_ps_tune_zero_func_fae(struct stk3328_data *ps_data)
{
    int32_t word_data;
    int ret, diff;
    unsigned char value[2];
#ifdef CALI_PS_EVERY_TIME
    if (!(ps_data->ps_enabled))
#else
    if (ps_data->psi_set || !(ps_data->ps_enabled))
#endif
    {
        return 0;
    }
    ret = stk3328_get_flag(ps_data);
    if (ret < 0)
        return ret;
    if (!(ret & STK_FLG_PSDR_MASK))
    {
        //pr_info("[SENSOR] %s: ps data is not ready yet\n", __func__);
        return 0;
    }
    ret = stk_ps_val(ps_data);
    if (ret == 0)
    {
        ret = stk3328_i2c_read_data(ps_data->client, 0x11, 2, &value[0]);
        if (ret < 0)
        {
            pr_err("[SENSOR] %s fail, ret=0x%x", __func__, ret);
            return ret;
        }
        word_data = (value[0] << 8) | value[1];
        //pr_info("[SENSOR] %s: word_data=%d\n", __func__, word_data);
        if (word_data == 0)
        {
            //pr_err("[SENSOR] %s: incorrect word data (0)\n", __func__);
            return 0xFFFF;
        }
        if (word_data > ps_data->psa)
        {
            ps_data->psa = word_data;
            pr_info("[SENSOR] %s: update psa: psa=%d,psi=%d\n", __func__, ps_data->psa, ps_data->psi);
        }
        if (word_data < ps_data->psi)
        {
            ps_data->psi = word_data;
            pr_info("[SENSOR] %s: update psi: psa=%d,psi=%d\n", __func__, ps_data->psa, ps_data->psi);
        }
    }
    diff = ps_data->psa - ps_data->psi;
    if (diff > ps_data->stk_max_min_diff)
    {
        ps_data->psi_set = ps_data->psi;
#ifdef CALI_PS_EVERY_TIME
        if (((ps_data->psi + ps_data->stk_ht_n_ct) > (ps_data->ps_thd_h + 500)) && (ps_data->ps_thd_h != 0))
        {
            //  ps_data->ps_thd_h = ps_data->ps_thd_h;
            //  ps_data->ps_thd_l = ps_data->ps_thd_l;
            pr_info("[SENSOR] %s: no update thd, HT=%d, LT=%d\n", __func__, ps_data->ps_thd_h, ps_data->ps_thd_l);
        }
        else
        {
            ps_data->ps_thd_h = ps_data->psi + ps_data->stk_ht_n_ct;
            ps_data->ps_thd_l = ps_data->psi + ps_data->stk_lt_n_ct;
            pr_info("[SENSOR] %s: update thd, HT=%d, LT=%d\n", __func__, ps_data->ps_thd_h, ps_data->ps_thd_l);
        }
#else
        ps_data->ps_thd_h = ps_data->psi + ps_data->stk_ht_n_ct;
        ps_data->ps_thd_l = ps_data->psi + ps_data->stk_lt_n_ct;
        pr_info("[SENSOR] %s: update thd, HT=%d, LT=%d\n", __func__, ps_data->ps_thd_h, ps_data->ps_thd_l);
#endif
        stk3328_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
        stk3328_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
        printk("%s: FAE tune0 psa-psi(%d) > STK_DIFF found\n", __func__, diff);
        hrtimer_cancel(&ps_data->ps_tune0_timer);
    }
    return 0;
}

static void stk_ps_tune0_work_func(struct work_struct *work)
{
    struct stk3328_data *ps_data = container_of(work, struct stk3328_data, stk_ps_tune0_work);
    if (ps_data->tune_zero_init_proc)
        stk_tune_zero_get_ps_data(ps_data);
    else
        stk_ps_tune_zero_func_fae(ps_data);
    return;
}

static enum hrtimer_restart stk_ps_tune0_timer_func(struct hrtimer *timer)
{
    struct stk3328_data *ps_data = container_of(timer, struct stk3328_data, ps_tune0_timer);
    queue_work(ps_data->stk_ps_tune0_wq, &ps_data->stk_ps_tune0_work);
    hrtimer_forward_now(&ps_data->ps_tune0_timer, ps_data->ps_tune0_delay);
    return HRTIMER_RESTART;
}
#endif

#ifdef STK_POLL_ALS
static enum hrtimer_restart stk_als_timer_func(struct hrtimer *timer)
{
    struct stk3328_data *ps_data = container_of(timer, struct stk3328_data, als_timer);
    queue_work(ps_data->stk_als_wq, &ps_data->stk_als_work);
    hrtimer_forward_now(&ps_data->als_timer, ps_data->als_poll_delay);
    return HRTIMER_RESTART;
}

static void stk_als_poll_work_func(struct work_struct *work)
{
    struct stk3328_data *ps_data = container_of(work, struct stk3328_data, stk_als_work);
    int32_t reading = 0, reading_lux, flag_reg;
#ifdef STK_IRS
    int ret;
#endif
    flag_reg = stk3328_get_flag(ps_data);
    if (flag_reg < 0)
        return;
    if (!(flag_reg & STK_FLG_ALSDR_MASK))
    {
        //pr_info("[SENSOR] %s: als is not ready\n", __func__);
        return;
    }
#ifdef STK_IRS
    ret = stk_als_ir_skip_als(ps_data);
    if (ret == 1)
        return;
#endif
    reading = stk3328_get_als_reading(ps_data);
    if (reading < 0)
        return;
    // printk("%s: als_data_index=%d, als_data=%d\n", __func__, ps_data->als_data_index, reading);
#ifdef STK_IRS
    stk_als_ir_get_corr(ps_data, reading);
    reading = reading * ps_data->als_correct_factor / 1000;
#endif
    reading_lux = stk_alscode2lux(ps_data, reading);
    //if (abs(ps_data->als_lux_last - reading_lux) >= STK_ALS_CHANGE_THD)
    {
        stk_als_report(ps_data, reading_lux);
		stk3328_als_gain_check(ps_data);
    }

#ifdef STK_IRS
    stk_als_ir_run(ps_data);
#endif
}
#endif /* #ifdef STK_POLL_ALS */

#ifdef STK_POLL_PS
static enum hrtimer_restart stk_ps_timer_func(struct hrtimer *timer)
{
    struct stk3328_data *ps_data = container_of(timer, struct stk3328_data, ps_timer);
    queue_work(ps_data->stk_ps_wq, &ps_data->stk_ps_work);
    hrtimer_forward_now(&ps_data->ps_timer, ps_data->ps_poll_delay);
    return HRTIMER_RESTART;
}

static void stk_ps_poll_work_func(struct work_struct *work)
{
    struct stk3328_data *ps_data = container_of(work, struct stk3328_data, stk_ps_work);
    uint32_t reading;
    int32_t near_far_state;
    uint8_t org_flag_reg;
    if (ps_data->ps_enabled)
    {
#ifdef STK_TUNE0
        // if(!(ps_data->psi_set))
        // return;
#endif
        org_flag_reg = stk3328_get_flag(ps_data);
        if (org_flag_reg < 0)
            return;
        if (!(org_flag_reg & STK_FLG_PSDR_MASK))
        {
            //pr_info("[SENSOR] %s: ps is not ready\n", __func__);
            return;
        }
        near_far_state = (org_flag_reg & STK_FLG_NF_MASK) ? 1 : 0;
        reading = stk3328_get_ps_reading(ps_data);
        ps_data->ps_code_last = reading;
        if (ps_data->ps_distance_last != near_far_state)
        {
            stk_ps_report(ps_data, near_far_state);
            pr_info("[SENSOR] %s: ps input event=%d, ps=%d\n", __func__, near_far_state, reading);
        }
    }
}
#endif

#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))
#if ((STK_INT_PS_MODE == 0x03) || (STK_INT_PS_MODE  == 0x02))
static void stk_ps_int_handle_int_mode_2_3(struct stk3328_data *ps_data)
{
    uint32_t reading;
    int32_t near_far_state;
#if (STK_INT_PS_MODE    == 0x03)
    near_far_state = gpio_get_value(ps_data->int_pin);
#elif   (STK_INT_PS_MODE == 0x02)
    near_far_state = !(gpio_get_value(ps_data->int_pin));
#endif
    reading = stk3328_get_ps_reading(ps_data);
    stk_ps_report(ps_data, near_far_state);
    pr_info("[SENSOR] %s: ps input event=%d, ps code=%d\n", __func__, near_far_state, reading);
}
#endif

static void stk_ps_int_handle(struct stk3328_data *ps_data, uint32_t ps_reading, int32_t nf_state)
{
    stk_ps_report(ps_data, nf_state);
    pr_info("[SENSOR] %s: ps input event=%d, ps code=%d\n", __func__, nf_state, ps_reading);
}

static int stk_als_int_handle(struct stk3328_data *ps_data, uint32_t als_reading)
{
    int32_t als_comperator;
#if( !defined(CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD))
    uint32_t nLuxIndex;
#endif
    int lux;
#if( !defined(CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD))
    nLuxIndex = stk_get_lux_interval_index(als_reading);
    stk3328_set_als_thd_h(ps_data, code_threshold_table[nLuxIndex]);
    stk3328_set_als_thd_l(ps_data, code_threshold_table[nLuxIndex - 1]);
#else
    stk_als_set_new_thd(ps_data, als_reading);
#endif //CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
    if (ps_data->ir_code)
    {
        if (als_reading < STK_IRC_MAX_ALS_CODE && als_reading > STK_IRC_MIN_ALS_CODE &&
            ps_data->ir_code > STK_IRC_MIN_IR_CODE)
        {
            als_comperator = als_reading * STK_IRC_ALS_NUMERA / STK_IRC_ALS_DENOMI;
            if (ps_data->ir_code > als_comperator)
                ps_data->als_correct_factor = STK_IRC_ALS_CORREC;
            else
                ps_data->als_correct_factor = 1000;
        }
        // pr_info("[SENSOR] %s: als=%d, ir=%d, als_correct_factor=%d", __func__, als_reading, ps_data->ir_code, ps_data->als_correct_factor);
        ps_data->ir_code = 0;
    }
    als_reading = als_reading * ps_data->als_correct_factor / 1000;
    ps_data->als_code_last = als_reading;
    lux = stk_alscode2lux(ps_data, als_reading);
    stk_als_report(ps_data, lux);
    return 0;
}

static void stk_work_func(struct work_struct *work)
{
    int32_t ret;
    int32_t near_far_state;
#if ((STK_INT_PS_MODE != 0x03) && (STK_INT_PS_MODE != 0x02))
    uint8_t disable_flag = 0;
    int32_t org_flag_reg;
#endif  /* #if ((STK_INT_PS_MODE != 0x03) && (STK_INT_PS_MODE != 0x02)) */
    struct stk3328_data *ps_data = container_of(work, struct stk3328_data, stk_work);
    uint32_t reading;
	
	pr_info("[SENSOR] %s \n",__func__);
#if ((STK_INT_PS_MODE == 0x03) || (STK_INT_PS_MODE  == 0x02))
    stk_ps_int_handle_int_mode_2_3(ps_data);
#else
    /* mode 0x01 or 0x04 */
    org_flag_reg = stk3328_get_flag(ps_data);
    if (org_flag_reg < 0)
        goto err_i2c_rw;
#ifdef STK_DEBUG_PRINTF
    pr_info("[SENSOR] %s: flag=0x%x\n", __func__, org_flag_reg);
#endif
    if (org_flag_reg & STK_FLG_ALSINT_MASK)
    {
        disable_flag |= STK_FLG_ALSINT_MASK;
        reading = stk3328_get_als_reading(ps_data);
        if (reading < 0)
            goto err_i2c_rw;
        ret = stk_als_int_handle(ps_data, reading);
        if (ret < 0)
            goto err_i2c_rw;
    }
    if (org_flag_reg & STK_FLG_PSINT_MASK)
    {
        disable_flag |= STK_FLG_PSINT_MASK;
        reading = stk3328_get_ps_reading(ps_data);
        if (reading < 0)
            goto err_i2c_rw;
        ps_data->ps_code_last = reading;

        near_far_state = (org_flag_reg & STK_FLG_NF_MASK) ? 1 : 0;
        stk_ps_int_handle(ps_data, reading, near_far_state);
    }
    if (disable_flag)
    {
        ret = stk3328_set_flag(ps_data, org_flag_reg, disable_flag);
        if (ret < 0)
            goto err_i2c_rw;
    }
#endif
    usleep_range(1000, 2000);
    enable_irq(ps_data->irq);
    return;
err_i2c_rw:
    msleep(30);
    enable_irq(ps_data->irq);
    return;
}

static irqreturn_t stk_oss_irq_handler(int irq, void *data)
{
    struct stk3328_data *pData = data;
	pr_info("[SENSOR] %s irq = %d\n",__func__,irq);
    disable_irq_nosync(irq);
    queue_work(pData->stk_wq, &pData->stk_work);
    return IRQ_HANDLED;
}
#endif  /*  #if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))   */

#ifdef STK_POLL_ALS
static void stk3328_als_set_poll_delay(struct stk3328_data *ps_data)
{
    uint8_t als_it = ps_data->alsctrl_reg & 0x0F;
    if (als_it == 0x8)
    {
        ps_data->als_poll_delay = ns_to_ktime(60 * NSEC_PER_MSEC);
    }
    else if (als_it == 0x9)
    {
        ps_data->als_poll_delay = ns_to_ktime(110 * NSEC_PER_MSEC);
    }
    else if (als_it == 0xA)
    {
        ps_data->als_poll_delay = ns_to_ktime(220 * NSEC_PER_MSEC);
    }
    else if (als_it == 0xB)
    {
        ps_data->als_poll_delay = ns_to_ktime(440 * NSEC_PER_MSEC);
    }
    else if (als_it == 0xC)
    {
        ps_data->als_poll_delay = ns_to_ktime(880 * NSEC_PER_MSEC);
    }
    else
    {
        ps_data->als_poll_delay = ns_to_ktime(110 * NSEC_PER_MSEC);
        pr_info("[SENSOR] %s: unknown ALS_IT=%d, set als_poll_delay=110ms\n", __func__, als_it);
    }
}
#endif

static int32_t stk3328_init_all_setting(struct i2c_client *client, struct stk3328_platform_data *plat_data)
{
    int32_t ret;
    struct stk3328_data *ps_data = i2c_get_clientdata(client);
    ret = stk3328_software_reset(ps_data);
    if (ret < 0)
        return ret;
    ret = stk3328_check_pid(ps_data);
    if (ret < 0)
        return ret;
    stk3328_proc_plat_data(ps_data, plat_data);
    ret = stk3328_init_all_reg(ps_data);
    if (ret < 0)
        return ret;
#ifdef STK_POLL_ALS
    stk3328_als_set_poll_delay(ps_data);
#endif
    ps_data->als_enabled = false;
    ps_data->ps_enabled = false;
    ps_data->re_enable_als = false;
    ps_data->re_enable_ps = false;
    ps_data->ir_code = 0;
    ps_data->als_correct_factor = 1000;
    ps_data->first_boot = true;
#if( !defined(CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD))
    stk_init_code_threshold_table(ps_data);
#endif
#ifdef STK_TUNE0
    stk_ps_tune_zero_init(ps_data);
#endif
#ifdef STK_ALS_FIR
    memset(&ps_data->fir, 0x00, sizeof(ps_data->fir));
    atomic_set(&ps_data->firlength, STK_FIR_LEN);
#endif
    atomic_set(&ps_data->recv_reg, 0);
#ifdef STK_IRS
    ps_data->als_data_index = 0;
#endif
    ps_data->ps_distance_last = -1;
    ps_data->als_code_last = 100;
    return 0;
}

#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))
static int stk3328_setup_irq(struct i2c_client *client)
{
    int irq, err = -EIO;
    struct stk3328_data *ps_data = i2c_get_clientdata(client);
    irq = gpio_to_irq(ps_data->int_pin);
#ifdef STK_DEBUG_PRINTF
    pr_info("[SENSOR] %s: int pin #=%d, irq=%d\n", __func__, ps_data->int_pin, irq);
#endif
    if (irq <= 0)
    {
        pr_err("[SENSOR] irq number is not specified, irq # = %d, int pin=%d\n", irq, ps_data->int_pin);
        return irq;
    }
    ps_data->irq = irq;
    err = gpio_request(ps_data->int_pin, "stk-int");
    if (err < 0)
    {
        pr_err("[SENSOR] %s: gpio_request, err=%d", __func__, err);
        return err;
    }
    // gpio_tlmm_config(GPIO_CFG(ps_data->int_pin, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
    err = gpio_direction_input(ps_data->int_pin);
    if (err < 0)
    {
        pr_err("[SENSOR] %s: gpio_direction_input, err=%d", __func__, err);
        return err;
    }
#if ((STK_INT_PS_MODE == 0x03) || (STK_INT_PS_MODE  == 0x02))
    err = request_any_context_irq(irq, stk_oss_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, DEVICE_NAME, ps_data);
#else
    err = request_any_context_irq(irq, stk_oss_irq_handler, IRQF_TRIGGER_LOW, DEVICE_NAME, ps_data);
#endif
    if (err < 0)
    {
        pr_err("[SENSOR] %s: request_any_context_irq(%d) failed for (%d)\n", __func__, irq, err);
        goto err_request_any_context_irq;
    }
    disable_irq(irq);
    return 0;
err_request_any_context_irq:
    gpio_free(ps_data->int_pin);
    return err;
}
#endif

static int stk3328_suspend(struct device *dev)
{
    struct stk3328_data *ps_data = dev_get_drvdata(dev);
#if (defined(STK_CHK_REG) || !defined(STK_POLL_PS))
    int err;
#endif
#ifndef STK_POLL_PS
    struct i2c_client *client = to_i2c_client(dev);
#endif
    pr_info("[SENSOR] %s\n", __func__);
    mutex_lock(&ps_data->io_lock);
#ifdef STK_CHK_REG
    err = stk3328_validate_n_handle(ps_data->client);
    if (err < 0)
    {
        pr_err("[SENSOR] stk3328_validate_n_handle fail: %d\n", err);
    }
    else if (err == 0xFF)
    {
        if (ps_data->ps_enabled)
            stk3328_enable_ps(ps_data, 1, 0);
    }
#endif /* #ifdef STK_CHK_REG */
    if (ps_data->als_enabled)
    {
        pr_info("[SENSOR] %s: Enable ALS : 0\n", __func__);
        stk3328_enable_als(ps_data, 0);
        ps_data->re_enable_als = true;
    }
    if (ps_data->ps_enabled)
    {
#ifdef STK_POLL_PS
        wake_lock(&ps_data->ps_nosuspend_wl);
#else
        if (device_may_wakeup(&client->dev))
        {
            err = enable_irq_wake(ps_data->irq);
            if (err)
                printk(KERN_WARNING "%s: set_irq_wake(%d) failed, err=(%d)\n", __func__, ps_data->irq, err);
        }
        else
        {
            pr_err("[SENSOR] %s: not support wakeup source\n", __func__);
        }
#endif
    }
    mutex_unlock(&ps_data->io_lock);
    return 0;
}

static int stk3328_resume(struct device *dev)
{
    struct stk3328_data *ps_data = dev_get_drvdata(dev);
#if (defined(STK_CHK_REG) || !defined(STK_POLL_PS))
    int err;
#endif
#ifndef STK_POLL_PS
    struct i2c_client *client = to_i2c_client(dev);
#endif
    pr_info("[SENSOR] %s\n", __func__);
    mutex_lock(&ps_data->io_lock);
#ifdef STK_CHK_REG
    err = stk3328_validate_n_handle(ps_data->client);
    if (err < 0)
    {
        pr_err("[SENSOR] stk3328_validate_n_handle fail: %d\n", err);
    }
    else if (err == 0xFF)
    {
        if (ps_data->ps_enabled)
            stk3328_enable_ps(ps_data, 1, 0);
    }
#endif /* #ifdef STK_CHK_REG */
    if (ps_data->re_enable_als)
    {
        pr_info("[SENSOR] %s: Enable ALS : 1\n", __func__);
        stk3328_enable_als(ps_data, 1);
        ps_data->re_enable_als = false;
    }
    if (ps_data->ps_enabled)
    {
#ifdef STK_POLL_PS
        wake_unlock(&ps_data->ps_nosuspend_wl);
#else
        if (device_may_wakeup(&client->dev))
        {
            err = disable_irq_wake(ps_data->irq);
            if (err)
                printk(KERN_WARNING "%s: disable_irq_wake(%d) failed, err=(%d)\n", __func__, ps_data->irq, err);
        }
#endif
    }
    mutex_unlock(&ps_data->io_lock);
    return 0;
}

static const struct dev_pm_ops stk3328_pm_ops =
{
    SET_SYSTEM_SLEEP_PM_OPS(stk3328_suspend, stk3328_resume)
};

#ifdef CONFIG_OF
static int stk3328_parse_dt(struct device *dev,
                            struct stk3328_platform_data *pdata)
{
#if 0
    pdata->int_pin = 65;
    pdata->transmittance = 500;
    pdata->state_reg = 0;
    pdata->psctrl_reg = 0x31;
    pdata->alsctrl_reg = 0x39;
    pdata->ledctrl_reg = 0xff;
    pdata->wait_reg = 0x7;
    pdata->ps_thd_h = 1700;
    pdata->ps_thd_l = 1500;
#else
    int rc;
    struct device_node *np = dev->of_node;
    u32 temp_val;
    enum of_gpio_flags flags;
    
    pdata->int_pin = of_get_named_gpio_flags(np, "stk,irq_gpio",
                     0, &flags);
    if (pdata->int_pin < 0)
    {
        dev_err(dev, "Unable to read irq_gpio\n");
        return pdata->int_pin;
    }
    rc = of_property_read_u32(np, "stk,transmittance", &temp_val);
    if (!rc)
        pdata->transmittance = temp_val;
    else
    {
        dev_err(dev, "Unable to read transmittance\n");
        return rc;
    }
    rc = of_property_read_u32(np, "stk,state_reg", &temp_val);
    if (!rc)
        pdata->state_reg = temp_val;
    else
    {
        dev_err(dev, "Unable to read state_reg\n");
        return rc;
    }
    rc = of_property_read_u32(np, "stk,psctrl_reg", &temp_val);
    if (!rc)
        pdata->psctrl_reg = (u8)temp_val;
    else
    {
        dev_err(dev, "Unable to read psctrl_reg\n");
        return rc;
    }
    rc = of_property_read_u32(np, "stk,alsctrl_reg", &temp_val);
    if (!rc)
        pdata->alsctrl_reg = (u8)temp_val;
    else
    {
        dev_err(dev, "Unable to read alsctrl_reg\n");
        return rc;
    }
    rc = of_property_read_u32(np, "stk,ledctrl_reg", &temp_val);
    if (!rc)
        pdata->ledctrl_reg = (u8)temp_val;
    else
    {
        dev_err(dev, "Unable to read ledctrl_reg\n");
        return rc;
    }
    rc = of_property_read_u32(np, "stk,wait_reg", &temp_val);
    if (!rc)
        pdata->wait_reg = (u8)temp_val;
    else
    {
        dev_err(dev, "Unable to read wait_reg\n");
        return rc;
    }
	rc = of_property_read_u32(np, "stk,als_cgain", &temp_val);
    if (!rc)
        pdata->als_cgain = (u8)temp_val;
    else
    {
        dev_err(dev, "Unable to read als_cgain\n");
        return rc;
    }
    rc = of_property_read_u32(np, "stk,ps_thdh", &temp_val);
    if (!rc)
        pdata->ps_thd_h = (u16)temp_val;
    else
    {
        dev_err(dev, "Unable to read ps_thdh\n");
        return rc;
    }
    rc = of_property_read_u32(np, "stk,ps_thdl", &temp_val);
    if (!rc)
        pdata->ps_thd_l = (u16)temp_val;
    else
    {
        dev_err(dev, "Unable to read ps_thdl\n");
        return rc;
    }
	rc = of_property_read_u32(np, "stk,default_trim", &temp_val);
    if (!rc)
        pdata->ps_default_trim = (u16)temp_val;
    else
    {
        dev_err(dev, "Unable to read ps_default_trim\n");
        return rc;
    }
    pdata->use_fir = of_property_read_bool(np, "stk,use_fir");
#endif
    return 0;
}
#else
static int stk3328_parse_dt(struct device *dev,
                            struct stk3328_platform_data *pdata)
{
    return -ENODEV;
}
#endif /* !CONFIG_OF */

static int prox_regulator_onoff(struct device *dev, bool onoff)
{
    struct regulator *vdd;
    int ret = 0;
    int voltage = 0;

    pr_info("[SENSOR] %s %s\n", __func__, (onoff) ? "on" : "off");

    vdd = devm_regulator_get(dev, "stk,reg_vdd");
    if (IS_ERR(vdd)) {
        pr_err("[SENSOR] %s: cannot get vdd\n", __func__);
        ret = -ENOMEM;
        goto err_vdd;
    } else {
        voltage = regulator_get_voltage(vdd);
        pr_info("[SENSOR] %s, voltage %d", __func__, voltage);
        /* Set voltage in dtsi */
    }

    if (onoff) {
        if (regulator_is_enabled(vdd)) {
            pr_info("[SENSOR] Regulator already enabled\n");
        }
        ret = regulator_enable(vdd);
        if (ret)
            pr_err("[SENSOR] %s: Failed to enable vdd.\n", __func__);
    } else {
        ret = regulator_disable(vdd);
        if (ret)
            pr_err("[SENSOR] %s: Failed to enable vdd.\n", __func__);
    }

    usleep_range(20000, 21000);

err_vdd:
    return ret;
}

static int stk3328_set_wq(struct stk3328_data *ps_data)
{
	int ret = 0;
#ifdef STK_POLL_ALS
    ps_data->stk_als_wq = create_singlethread_workqueue("stk_als_wq");
	if (!ps_data->stk_als_wq) {
		ret = -ENOMEM;
		pr_err("[SENSOR] %s: could not create stk_als_wq workqueue\n",
			__func__);
		return ret;
	}
    INIT_WORK(&ps_data->stk_als_work, stk_als_poll_work_func);
    hrtimer_init(&ps_data->als_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    ps_data->als_poll_delay = ns_to_ktime(110 * NSEC_PER_MSEC);
    ps_data->als_timer.function = stk_als_timer_func;
#endif
#ifdef STK_POLL_PS
    ps_data->stk_ps_wq = create_singlethread_workqueue("stk_ps_wq");
	if (!ps_data->stk_ps_wq) {
		ret = -ENOMEM;
		pr_err("[SENSOR] %s: could not create stk_ps_wq workqueue\n",
			__func__);
		return ret;
	}
    INIT_WORK(&ps_data->stk_ps_work, stk_ps_poll_work_func);
    hrtimer_init(&ps_data->ps_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    ps_data->ps_poll_delay = ns_to_ktime(60 * NSEC_PER_MSEC);
    ps_data->ps_timer.function = stk_ps_timer_func;
#endif
#ifdef STK_TUNE0
    ps_data->stk_ps_tune0_wq = create_singlethread_workqueue("stk_ps_tune0_wq");
	if (!ps_data->stk_ps_tune0_wq) {
		ret = -ENOMEM;
		pr_err("[SENSOR] %s: could not create stk_ps_wq workqueue\n",
			__func__);
		return ret;
	}
    INIT_WORK(&ps_data->stk_ps_tune0_work, stk_ps_tune0_work_func);
    hrtimer_init(&ps_data->ps_tune0_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    ps_data->ps_tune0_delay = ns_to_ktime(60 * NSEC_PER_MSEC);
    ps_data->ps_tune0_timer.function = stk_ps_tune0_timer_func;
#endif
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
    ps_data->stk_wq = create_singlethread_workqueue("stk_wq");
	if (!ps_data->stk_wq) {
		ret = -ENOMEM;
		pr_err("[SENSOR] %s: could not create stk_wq workqueue\n",
			__func__);
		return ret;
	}
    INIT_WORK(&ps_data->stk_work, stk_work_func);
#endif
    return 0;
}

static int stk3328_set_input_devices(struct stk3328_data *ps_data)
{
    int err;
    ps_data->als_input_dev = input_allocate_device();
    if (ps_data->als_input_dev == NULL)
    {
        pr_err("[SENSOR] %s: could not allocate als device\n", __func__);
        err = -ENOMEM;
        goto err_als_input_allocate;
    }
    
    input_set_drvdata(ps_data->als_input_dev, ps_data);
    ps_data->als_input_dev->name = ALS_NAME;
    input_set_capability(ps_data->als_input_dev, EV_REL, REL_DIAL);
    input_set_capability(ps_data->als_input_dev, EV_REL, REL_WHEEL);
    input_set_capability(ps_data->als_input_dev, EV_REL, REL_X);
    input_set_capability(ps_data->als_input_dev, EV_REL, REL_Y);
    input_set_capability(ps_data->als_input_dev, EV_REL, REL_Z);

    err = input_register_device(ps_data->als_input_dev);
    if (err < 0)
    {
        pr_err("[SENSOR] %s: can not register als input device\n", __func__);
        goto err_als_input_register;
    }
    
    err = sensors_create_symlink(&ps_data->als_input_dev->dev.kobj,
                    ps_data->als_input_dev->name);
    if (err < 0) {
        pr_err("[SENSOR] %s: create_symlink error\n", __func__);
        goto err_sensors_create_symlink_light;
    }
    
    err = sysfs_create_group(&ps_data->als_input_dev->dev.kobj, &stk_als_attribute_group);
    if (err < 0)
    {
        pr_err("[SENSOR] %s:could not create sysfs group for als\n", __func__);
        goto err_als_create_group;
    }
    
    
    ps_data->ps_input_dev = input_allocate_device();
    if (ps_data->ps_input_dev == NULL)
    {
        pr_err("[SENSOR] %s: could not allocate ps device\n", __func__);
        err = -ENOMEM;
        goto err_ps_input_allocate;
    }
    
    input_set_drvdata(ps_data->ps_input_dev, ps_data);
    ps_data->ps_input_dev->name = PS_NAME;
    input_set_capability(ps_data->ps_input_dev, EV_ABS, ABS_DISTANCE);
    input_set_abs_params(ps_data->ps_input_dev, ABS_DISTANCE, 0, 1, 0, 0);
    
    err = input_register_device(ps_data->ps_input_dev);
    if (err < 0)
    {
        pr_err("[SENSOR] %s: can not register ps input device\n", __func__);
        goto err_ps_input_register;
    }
    
    err = sensors_create_symlink(&ps_data->ps_input_dev->dev.kobj,
                    ps_data->ps_input_dev->name);
    if (err < 0) {
        pr_err("[SENSOR] %s: create_symlink error\n", __func__);
        goto err_sensors_create_symlink_prox;
    }
    
    err = sysfs_create_group(&ps_data->ps_input_dev->dev.kobj, &stk_ps_attribute_group);
    if (err < 0)
    {
        pr_err("[SENSOR] %s:could not create sysfs group for ps\n", __func__);
        goto err_ps_create_group;
    }
    
    return 0;

err_ps_create_group:
err_sensors_create_symlink_prox:
	input_free_device(ps_data->ps_input_dev);
	input_unregister_device(ps_data->ps_input_dev);
err_ps_input_register:
err_ps_input_allocate:
	sysfs_remove_group(&ps_data->als_input_dev->dev.kobj, &stk_als_attribute_group);
err_als_create_group:
err_sensors_create_symlink_light:
	input_free_device(ps_data->als_input_dev);
    input_unregister_device(ps_data->als_input_dev);   
err_als_input_register:
err_als_input_allocate:
    return err;
}

static int stk3328_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    int err = -ENODEV;
    struct stk3328_data *ps_data;
    struct stk3328_platform_data *plat_data;
    pr_info("[SENSOR] %s: driver version = %s\n", __func__, DRIVER_VERSION);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        pr_err("[SENSOR] %s: No Support for I2C_FUNC_I2C\n", __func__);
        return -ENODEV;
    }
    ps_data = kzalloc(sizeof(struct stk3328_data), GFP_KERNEL);
    if (!ps_data)
    {
        pr_err("[SENSOR] %s: failed to allocate stk3328_data\n", __func__);
        return -ENOMEM;
    }
    ps_data->client = client;
    i2c_set_clientdata(client, ps_data);
    mutex_init(&ps_data->io_lock);
    wake_lock_init(&ps_data->ps_wakelock, WAKE_LOCK_SUSPEND, "stk_input_wakelock");
#ifdef STK_POLL_PS
    wake_lock_init(&ps_data->ps_nosuspend_wl, WAKE_LOCK_SUSPEND, "stk_nosuspend_wakelock");
#endif
    if (client->dev.of_node)
    {
        pr_info("[SENSOR] %s: probe with device tree\n", __func__);
        plat_data = devm_kzalloc(&client->dev,
                                 sizeof(struct stk3328_platform_data), GFP_KERNEL);
        if (!plat_data)
        {
            dev_err(&client->dev, "Failed to allocate memory\n");
            return -ENOMEM;
        }
        err = stk3328_parse_dt(&client->dev, plat_data);
        if (err)
        {
            dev_err(&client->dev,
                    "%s: stk3328_parse_dt ret=%d\n", __func__, err);
            return err;
        }
    }
    else
    {
        pr_info("[SENSOR] %s: probe with platform data\n", __func__);
        plat_data = client->dev.platform_data;
    }
    if (!plat_data)
    {
        dev_err(&client->dev,
                "%s: no stk3328 platform data!\n", __func__);
        goto err_als_input_allocate;
    }

    prox_regulator_onoff(&client->dev, 1);
    usleep_range(2000, 2100);
    
    ps_data->als_transmittance = plat_data->transmittance;
    ps_data->int_pin = plat_data->int_pin;
    ps_data->pdata = plat_data;
    if (ps_data->als_transmittance == 0)
    {
        dev_err(&client->dev,
                "%s: Please set als_transmittance\n", __func__);
        goto err_als_input_allocate;
    }
    stk3328_set_wq(ps_data);
    
#ifdef STK_TUNE0
    ps_data->stk_max_min_diff = STK_MAX_MIN_DIFF;
    ps_data->stk_lt_n_ct = STK_LT_N_CT;
    ps_data->stk_ht_n_ct = STK_HT_N_CT;
#endif

    err = stk3328_init_all_setting(client, plat_data);
    if (err < 0)
        goto err_init_all_setting;
    
    err = stk3328_set_input_devices(ps_data);
    if (err < 0)
        goto err_setup_input_device;
    
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
    err = stk3328_setup_irq(client);
    if (err < 0)
        goto err_stk3328_setup_irq;
#endif

    /* set sysfs for light sensor */
    err = sensors_register(&ps_data->light_dev,
        ps_data, light_sensor_attrs,
            "light_sensor");
    if (err) {
        pr_err("[SENSOR] %s: cound not register light sensor device(%d).\n",
            __func__, err);
        goto light_sensor_register_failed;
    }
    
	hrtimer_init(&ps_data->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ps_data->prox_poll_delay = ns_to_ktime(2000 * NSEC_PER_MSEC);/*2 sec*/
	ps_data->prox_timer.function = stk3328_prox_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	ps_data->prox_wq = create_singlethread_workqueue("stk3328_prox_wq");
	if (!ps_data->prox_wq) {
		err = -ENOMEM;
		SENSOR_ERR("could not create prox workqueue\n");
		goto err_create_prox_workqueue;
	}
	/* this is the thread function we run on the work queue */
	INIT_WORK(&ps_data->work_prox, stk3328_work_func_prox);
	
    /* set sysfs for proximity sensor */
    err = sensors_register(&ps_data->proximity_dev,
        ps_data, prox_sensor_attrs,
            "proximity_sensor");
    if (err) {
        pr_err("[SENSOR] %s: cound not register proximity sensor device(%d).\n",
            __func__, err);
        goto prox_sensor_register_failed;
    }

    device_init_wakeup(&client->dev, true);
    
    pr_info("[SENSOR] %s is success.\n", __func__);
    return 0;

prox_sensor_register_failed:
	destroy_workqueue(ps_data->prox_wq);
err_create_prox_workqueue:
	sensors_unregister(ps_data->light_dev, light_sensor_attrs);
light_sensor_register_failed:
    //device_init_wakeup(&client->dev, false);
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
    free_irq(ps_data->irq, ps_data);
    gpio_free(ps_data->int_pin);
#endif
err_stk3328_setup_irq:
    sysfs_remove_group(&ps_data->ps_input_dev->dev.kobj, &stk_ps_attribute_group);
    sysfs_remove_group(&ps_data->als_input_dev->dev.kobj, &stk_als_attribute_group);
    input_unregister_device(ps_data->ps_input_dev);
    input_unregister_device(ps_data->als_input_dev);
err_setup_input_device:
err_init_all_setting:
#ifdef STK_POLL_ALS
    hrtimer_try_to_cancel(&ps_data->als_timer);
    destroy_workqueue(ps_data->stk_als_wq);
#endif
#ifdef STK_TUNE0
    destroy_workqueue(ps_data->stk_ps_tune0_wq);
#endif
#ifdef STK_POLL_PS
    hrtimer_try_to_cancel(&ps_data->ps_timer);
    destroy_workqueue(ps_data->stk_ps_wq);
#endif
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
    destroy_workqueue(ps_data->stk_wq);
#endif
err_als_input_allocate:
#ifdef STK_POLL_PS
    wake_lock_destroy(&ps_data->ps_nosuspend_wl);
#endif
    wake_lock_destroy(&ps_data->ps_wakelock);
    mutex_destroy(&ps_data->io_lock);
    kfree(ps_data);
    return err;
}

static int stk3328_remove(struct i2c_client *client)
{
    struct stk3328_data *ps_data = i2c_get_clientdata(client);
    device_init_wakeup(&client->dev, false);
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
    free_irq(ps_data->irq, ps_data);
    gpio_free(ps_data->int_pin);
#endif  /* #if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS)) */

    prox_regulator_onoff(&client->dev, 0);

	/* sysfs destroy */
	sensors_unregister(ps_data->light_dev, light_sensor_attrs);
	sensors_unregister(ps_data->proximity_dev, prox_sensor_attrs);
	sensors_remove_symlink(&ps_data->als_input_dev->dev.kobj,
			ps_data->als_input_dev->name);
	sensors_remove_symlink(&ps_data->ps_input_dev->dev.kobj,
			ps_data->ps_input_dev->name);

    sysfs_remove_group(&ps_data->ps_input_dev->dev.kobj, &stk_ps_attribute_group);
    sysfs_remove_group(&ps_data->als_input_dev->dev.kobj, &stk_als_attribute_group);
    input_unregister_device(ps_data->ps_input_dev);
    input_unregister_device(ps_data->als_input_dev);
#ifdef STK_POLL_ALS
    hrtimer_try_to_cancel(&ps_data->als_timer);
    destroy_workqueue(ps_data->stk_als_wq);
#endif
#ifdef STK_TUNE0
    destroy_workqueue(ps_data->stk_ps_tune0_wq);
#endif
#ifdef STK_POLL_PS
    hrtimer_try_to_cancel(&ps_data->ps_timer);
    destroy_workqueue(ps_data->stk_ps_wq);
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
    destroy_workqueue(ps_data->stk_wq);
#endif
    wake_lock_destroy(&ps_data->ps_nosuspend_wl);
#endif
    wake_lock_destroy(&ps_data->ps_wakelock);
    mutex_destroy(&ps_data->io_lock);
    kfree(ps_data);
    return 0;
}

static void stk3328_shutdown(struct i2c_client *client)
{
	struct stk3328_data *ps_data = i2c_get_clientdata(client);
    pr_info("[SENSOR] %s\n", __func__);

	if (ps_data->ps_enabled){
		disable_irq_wake(ps_data->irq);
        stk3328_enable_ps(ps_data, 0, 0);
	}
	if (ps_data->als_enabled)
    {
        stk3328_enable_als(ps_data, 0);
    }
    
	prox_regulator_onoff(&client->dev, 0);

	pr_info("[SENSOR] %s: done\n", __func__);
}

static const struct i2c_device_id stk_ps_id[] =
{
    { "stk3328", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, stk_ps_id);
static struct of_device_id stk_match_table[] =
{
    { .compatible = "stk,stk3328", },
    { },
};

static struct i2c_driver stk_ps_driver =
{
    .driver = {
        .name = DEVICE_NAME,
        .owner = THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = stk_match_table,
#endif
        .pm = &stk3328_pm_ops,
    },
    .probe = stk3328_probe,
    .remove = stk3328_remove,
	.shutdown = stk3328_shutdown,
    .id_table = stk_ps_id,
};

static int __init stk3328_init(void)
{
    int ret;
    ret = i2c_add_driver(&stk_ps_driver);
    if (ret)
    {
        i2c_del_driver(&stk_ps_driver);
        return ret;
    }
    return 0;
}

static void __exit stk3328_exit(void)
{
    i2c_del_driver(&stk_ps_driver);
}

module_init(stk3328_init);
module_exit(stk3328_exit);
MODULE_AUTHOR("Lex Hsieh <lex_hsieh@sensortek.com.tw>");
MODULE_DESCRIPTION("Sensortek stk3328 Proximity Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);