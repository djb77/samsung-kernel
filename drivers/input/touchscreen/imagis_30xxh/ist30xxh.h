/*
 *  Copyright (C) 2010, Imagis Technology Co. Ltd. All Rights Reserved.
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

#ifndef __IST30XXH_H__
#define __IST30XXH_H__

/*
 * Support F/W ver : ~ IST30xxH v1.0.0.0
 * Support CmCs ver : v1.0
 * Support IC : IST3038H
 * Release : 2016.11.15 by Drake
 */

#ifdef USE_TSP_TA_CALLBACKS
#include <linux/input/tsp_ta_callback.h>
#endif

#if defined(CONFIG_VBUS_NOTIFIER) || defined(CONFIG_MUIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/vbus_notifier.h>
#endif

#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/manager/usb_typec_manager_notifier.h>
#endif

#define FIRMWARE_PATH_LENGTH        (64)
#define FIRMWARE_PATH               ("tsp_imagis/")

#define IMAGIS_TSP_DD_VERSION       ("1.0.0.0")

#define IMAGIS_IST3038H             (1) /* 3038H */

#define IMAGIS_TSP_IC               IMAGIS_IST3038H // Select using IC
#define TSP_CHIP_VENDOR             ("IMAGIS")

/* IST30XX FUNCTION SET */
#define USE_OPEN_CLOSE
#define SEC_FACTORY_MODE
#define IST30XX_INTERNAL_BIN
#define IST30XX_NOISE_MODE
#define IST30XX_USE_CMCS
#define IST30XX_USE_KEY
#ifdef IST30XX_USE_KEY
#define IST30XX_KEY_CODES           { KEY_RECENT, KEY_BACK }
#endif
/* TCLM CONCEPT */
#define TCLM_CONCEPT
#ifdef TCLM_CONCEPT
/*------------------------------
    <<< apply to server >>>
    0x00 : no action
    0x01 : clear nv
    0x02 : lock down
    0x03 : rfu

    <<< use for temp bin >>>
    0x05 : everytime
    0x06 : rfu
-------------------------------*/
#define TCLM_LEVEL_NONE             (0x00)
#define TCLM_LEVEL_CLEAR_NV         (0x01)
#define TCLM_LEVEL_LOCKDOWN         (0x02)
#define TCLM_LEVEL_EVERYTIME        (0x05)

#define IST30XX_NVM_TSP_TEST_DATA				0
#define IST30XX_NVM_OFFSET_CAL_COUNT			1
#define IST30XX_NVM_OFFSET_TUNE_VERSION			2

/* ----------------------------------------
 * write 0xE4 [ 11 | 10 | 01 | 00 ]
 * MSB <-------------------> LSB
 * read 0xE4
 * mapping sequnce : LSB -> MSB
 * struct sec_ts_test_result {
 * * assy : front + OCTA assay
 * * module : only OCTA
 *   union {
 *       struct {
 *           u8 assy_count:2;   -> 00
 *           u8 assy_result:2;  -> 01
 *           u8 module_count:2; -> 10
 *           u8 module_result:2;-> 11
 *       } __attribute__ ((packed));
 *       unsigned char data[1];
 *   };
 *};
 * ----------------------------------------
 */

/*TCLM CONCEPT */
#define IST30XX_NVM_OFFSET_CAL_POSITON				3
#define IST30XX_NVM_OFFSET_HISTORY_QUEUE_COUNT		4
#define IST30XX_NVM_OFFSET_HISTORY_QUEUE_LASTP		5
#define IST30XX_NVM_OFFSET_HISTORY_QUEUE_ZERO		6
#define IST30XX_NVM_OFFSET_HISTORY_QUEUE_SIZE		6 /* QUEUE SIZE 6 */

enum tclm_root {
	CALPOSITION_NONE			= 0,
	CALPOSITION_INITIAL			= 1,
	CALPOSITION_FACTORY			= 2,
	CALPOSITION_OUTSIDE			= 3,
	CALPOSITION_LCIA			= 4,
	CALPOSITION_SVCCENTER		= 5,
	CALPOSITION_ABNORMAL		= 6,
	CALPOSITION_FIRMUP			= 7,
	CALPOSITION_SPECOUT			= 8,
	CALPOSITION_TUNEUP			= 9,
	CALPOSITION_EVERYTIME		= 10,
	CALPOSITION_TESTMODE		= 11,
	CALPOSITION_UNDEFINE		= 12,
	CALPOSITION_MAX				= 16,
};

#define CAL_HISTORY_QUEUE_MAX			10
#define CAL_HISTORY_QUEUE_SHORT_DISPLAY		3

struct ts_test_result {
    union {
        struct {
            u8 assy_count:2;
            u8 assy_result:2;
            u8 module_count:2;
            u8 module_result:2;
        } __attribute__ ((packed));
        unsigned char data[1];
    };
};
#define TEST_OCTA_MODULE            (1)
#define TEST_OCTA_ASSAY             (2)

#define TEST_OCTA_NONE              (0)
#define TEST_OCTA_FAIL              (1)
#define TEST_OCTA_PASS              (2)

struct sec_cal_position {
	const char * f_name;
	const char s_name;
};

#define CAL_POS_CMD(full_name, short_name)		.f_name = full_name, .s_name = short_name
#endif

/* IST30XX FUNCTION SET */

#define IST30XXH_DEFAULT_CHIP_ID    (0x038D)
#if (IMAGIS_TSP_IC == IMAGIS_IST3038H)
#define TSP_CHIP_NAME               ("IST3038H")
#define IST30XX_CHIP_ID             (0x038D)
#define IST30XX_MAX_NODE_NUM        (24 * 24)
#define IST30XX_MAX_SELF_NODE_NUM   (38)
#else
#define TSP_CHIP_NAME               ("IST30XXH")
#define IST30XX_CHIP_ID             (0x038D)
#define IST30XX_MAX_NODE_NUM        (24 * 24)
#define IST30XX_MAX_SELF_NODE_NUM   (38)
#endif

#define IST30XX_TX_CHANNEL_NUM      (20) /* reserved for other models  */
#define IST30XX_RX_CHANNEL_NUM      (40) /* reserved for other models  */

#define IST30XX_MAX_FINGERS         (10)
#define IST30XX_MAX_KEYS            (5)
#define IST30XX_MAX_MAJOR           (15)
#define IST30XX_MAX_MINOR           (15)
#define IST30XX_MAX_Z               (0x1000)

#define IST30XX_INITIAL_VALUE       (0x8EAD8EAD)
#define IST30XX_REC_VALUE           (0x8CA0D00E)
#define IST30XX_REC_FILENAME_SIZE   (128)
#define IST30XX_DEBUGGING_VALUE     (0xDBDB0000)
#define IST30XX_MAX_DEBUGINFO       (512)
#define IST30XX_EXCEPT_MASK         (0xFFFFFF00)
#define IST30XX_EXCEPT_VALUE        (0xE11CE900)
#define IST30XX_MAX_EXCEPT_SIZE     (2)
#define IST30XX_EXCEPT_INTEGRITY    (0xE11CE901)
#define IST30XX_LPM_VALUE           (0x193030DE)

/* Calibration */
#define CALIB_MSG_MASK              (0xF0000FFF)
#define CALIB_MSG_VALID             (0x80000CAB)
#define CALIB_WAIT_TIME             (50)        /* unit : 100msec */
#define CALIB_MAX_I2C_FAIL_CNT      (10)
#define CALIB_TO_GAP(n)             ((n >> 16) & 0xFFF)
#define CALIB_TO_STATUS(n)          ((n >> 12) & 0xF)
#define CALIB_TO_OS_VALUE(n)        ((n >> 12) & 0xFFFF)
#define IST30XX_MAX_CALIB_SIZE      (3)

#define IST30XX_JIG_TOUCH           (0xC0)
#define IST30XX_START_SCAN          (2)
#define IST30XX_ENABLE              (1)
#define IST30XX_DISABLE             (0)
#define IST30XX_SPAY                (1 << 0)
#define IST30XX_AOD                 (1 << 1)
#define IST30XX_GETURE_CTRL_SPAY    (1 << 1)
#define IST30XX_GETURE_CTRL_AOD     (1 << 2)
#define IST30XX_GETURE_EVT_SPAY     (1 << 1)
#define IST30XX_GETURE_EVT_AOD      (1 << 2)
#define IST30XX_GETURE_SET_SPAY     (1 << 4)
#define IST30XX_GETURE_SET_AOD      (1 << 5)

/* retry count */
#define IST30XX_MAX_RETRY_CNT       (3)
#define IST30XX_MAX_CAL_REF_CNT     (3)

/* Log message */
#define DEV_ERR                     (1)
#define DEV_WARN                    (2)
#define DEV_INFO                    (3)
#define DEV_NOTI                    (4)
#define DEV_DEBUG                   (5)
#define DEV_VERB                    (6)

#define IST30XX_LOG_TAG             ("[ TSP ]")
#define IST30XX_LOG_LEVEL           DEV_NOTI

#define tsp_err(fmt, ...)           tsp_printk(DEV_ERR, fmt, ## __VA_ARGS__)
#define tsp_warn(fmt, ...)          tsp_printk(DEV_WARN, fmt, ## __VA_ARGS__)
#define tsp_info(fmt, ...)          tsp_printk(DEV_INFO, fmt, ## __VA_ARGS__)
#define tsp_noti(fmt, ...)          tsp_printk(DEV_NOTI, fmt, ## __VA_ARGS__)
#define tsp_debug(fmt, ...)         tsp_printk(DEV_DEBUG, fmt, ## __VA_ARGS__)
#define tsp_verb(fmt, ...)          tsp_printk(DEV_VERB, fmt, ## __VA_ARGS__)

/* timer & err cnt */
#define IST30XX_MAX_ERR_CNT         (100)
#define EVENT_TIMER_INTERVAL        (HZ * data->timer_period_ms / 1000)

/* i2c setting */
/* I2C Device info */
#define IST30XX_DEV_NAME            ("IST30XX")
#define IST30XX_DEV_ID              (0xA0 >> 1)

/* I2C transfer msg number */
#define WRITE_CMD_MSG_LEN           (1)
#define READ_CMD_MSG_LEN            (2)

/* I2C address/Data length */
#define IST30XX_ADDR_LEN            (4) /* bytes */
#define IST30XX_DATA_LEN            (4) /* bytes */

/* I2C transaction size */
#define I2C_MAX_WRITE_SIZE          (256) /* bytes */
#define I2C_MAX_READ_SIZE           (256) /* bytes */

/* I2C access mode */
#define IST30XX_DIRECT_ACCESS       (1 << 31)
#define IST30XX_BURST_ACCESS        (1 << 27)
#define IST30XX_HIB_ACCESS          (0x800B << 16)
#define IST30XX_CMD_ACCESS          (0x800A << 16)
#define IST30XX_DA_ADDR(n)          (n | IST30XX_DIRECT_ACCESS)
#define IST30XX_BA_ADDR(n)          (n | IST30XX_BURST_ACCESS)
#define IST30XX_HA_ADDR(n)          (n | IST30XX_HIB_ACCESS)
#define IST30XX_CA_ADDR(n)          (n | IST30XX_CMD_ACCESS)

/* register */
/* Info register */
#define rSYS_CHIPID                 IST30XX_DA_ADDR(0x40001000)
#define rSYS_LCLK_CON               IST30XX_DA_ADDR(0x40001054)

#define rCMD_SW_RESET               IST30XX_CA_ADDR(0x40000000)

/* HIB register */
#define IST30XX_HIB_BASE            (0x30000100)
#define IST30XX_HIB_TOUCH_STATUS    IST30XX_HA_ADDR(IST30XX_HIB_BASE | 0x00)
#define IST30XX_HIB_INTR_MSG        IST30XX_HA_ADDR(IST30XX_HIB_BASE | 0x04)
#define IST30XX_HIB_COORD           IST30XX_HA_ADDR(IST30XX_HIB_BASE | 0x08)
#define IST30XX_HIB_GESTURE_REG     IST30XX_HA_ADDR(IST30XX_HIB_BASE | 0x0C)
#define IST30XX_HIB_CMD             IST30XX_HA_ADDR(IST30XX_HIB_BASE | 0x1C)
#define IST30XX_HIB_RW_STATUS       IST30XX_HA_ADDR(IST30XX_HIB_BASE | 0x40)

/* Touch status macro */
#define TOUCH_STATUS_MAGIC          (0x00000075)
#define TOUCH_STATUS_MASK           (0x000000FF)
#define FINGER_ENABLE_MASK          (1 << 20)
#define NOISE_MODE_MASK             (1 << 19)
#define WET_MODE_MASK               (1 << 18)
#define SCAN_CNT_MASK               (0xFFE00000)
#define GET_FINGER_ENABLE(n)        (n & FINGER_ENABLE_MASK)
#define GET_NOISE_MODE(n)           (n & NOISE_MODE_MASK)
#define GET_WET_MODE(n)             (n & WET_MODE_MASK)
#define GET_SCAN_CNT(n)             ((n & SCAN_CNT_MASK) >> 21)
#define TOUCH_STATUS_NORMAL_MODE    (0)
#define TOUCH_STATUS_NOISE_MODE     (1 << 0)
#define TOUCH_STATUS_WET_MODE       (1 << 1)

/* interrupt macro */
#define IST30XX_INTR_STATUS         (0x00000800)
#define CHECK_INTR_STATUS(n)        (((n & IST30XX_INTR_STATUS) \
                                            == IST30XX_INTR_STATUS) ? 1 : 0)
#define PARSE_FINGER_CNT(n)         ((n >> 12) & 0xF)
#define PARSE_KEY_CNT(n)            ((n >> 21) & 0x7)
/* Finger status: [9:0] */
#define PARSE_FINGER_STATUS(n)      (n & 0x3FF)
/* Key status: [20:16] */
#define PARSE_KEY_STATUS(n)         ((n >> 16) & 0x1F)
#define PARSE_PALM_STATUS(n)        ((n >> 10) & 0x1)
#define PRESSED_FINGER(s, id)       ((s & (1 << id)) ? true : false)
#define PRESSED_KEY(s, id)          ((s & (1 << (16 + id))) ? true : false)

/* mode */
typedef enum {
    TOUCH_MODE = 0,
    LPM_MODE,
} SYSTEM_STATUS;

#define IST30XX_SELF_CDC_OFFSET     (0xD90)
#define IST30XX_MAX_CMD_SIZE        (0x20)
#define IST30XX_CMD_ADDR(n)         (n * 4)
#define IST30XX_CMD_VALUE(n)        (n / 4)
enum ist30xx_read_commands {
    eHCOM_GET_CHIP_ID       = IST30XX_CMD_ADDR(0x00),
    eHCOM_GET_VER_MAIN      = IST30XX_CMD_ADDR(0x01),
    eHCOM_GET_VER_FW        = IST30XX_CMD_ADDR(0x02),
    eHCOM_GET_VER_CORE      = IST30XX_CMD_ADDR(0x03),
    eHCOM_GET_VER_TEST      = IST30XX_CMD_ADDR(0x04),
    eHCOM_GET_CRC32         = IST30XX_CMD_ADDR(0x05),
    eHCOM_GET_FW_MODE       = IST30XX_CMD_ADDR(0x06),
    eHCOM_GET_CAL_RESULT    = IST30XX_CMD_ADDR(0x07),
    eHCOM_GET_SLF_CAL_RESULT= IST30XX_CMD_ADDR(0x08),
    eHCOM_GET_FW_INTEGRITY  = IST30XX_CMD_ADDR(0x09),

    eHCOM_GET_SELF_CDC_BASE = IST30XX_CMD_ADDR(0x0B),

    eHCOM_GET_REC_INFO_BASE = IST30XX_CMD_ADDR(0x0D),
    eHCOM_GET_DBG_INFO_BASE = IST30XX_CMD_ADDR(0x0E),

    eHCOM_GET_LCD_INFO      = IST30XX_CMD_ADDR(0x10),
    eHCOM_GET_TSP_INFO      = IST30XX_CMD_ADDR(0x11),
    eHCOM_GET_KEY_INFO_0    = IST30XX_CMD_ADDR(0x12),
    eHCOM_GET_KEY_INFO_1    = IST30XX_CMD_ADDR(0x13),
    eHCOM_GET_KEY_INFO_2    = IST30XX_CMD_ADDR(0x14),
    eHCOM_GET_SCR_INFO      = IST30XX_CMD_ADDR(0x15),
    eHCOM_GET_GTX_INFO      = IST30XX_CMD_ADDR(0x16),
    eHCOM_GET_SWAP_INFO     = IST30XX_CMD_ADDR(0x17),
    eHCOM_GET_FINGER_INFO   = IST30XX_CMD_ADDR(0x18),
    eHCOM_GET_BASELINE      = IST30XX_CMD_ADDR(0x19),
    eHCOM_GET_TOUCH_TH      = IST30XX_CMD_ADDR(0x1A),

    eHCOM_GET_SEC_INFO_BASE = IST30XX_CMD_ADDR(0x1B),
    eHCOM_GET_ZVALUE_BASE   = IST30XX_CMD_ADDR(0x1C),
    eHCOM_GET_CDC_BASE      = IST30XX_CMD_ADDR(0x1D),
    eHCOM_GET_ALGO_BASE     = IST30XX_CMD_ADDR(0x1E),
    eHCOM_GET_COM_CHECKSUM  = IST30XX_CMD_ADDR(0x1F),
};

enum ist30xx_write_commands {
    eHCOM_FW_START          = 0x01,
    eHCOM_FW_HOLD           = 0x02,

    eHCOM_CP_CORRECT_EN     = 0x10,
    eHCOM_WDT_EN            = 0x11,
    eHCOM_GESTURE_EN        = 0x12,
    eHCOM_SCALE_EN          = 0x13,
    eHCOM_NEW_POSITION_DIS  = 0x14,
    eHCOM_SLEEP_MODE_EN     = 0x15,

    eHCOM_SET_TIME_ACTIVE   = 0x20,
    eHCOM_SET_TIME_IDLE     = 0x21,
    eHCOM_SET_MODE_SPECIAL  = 0x22,
    eHCOM_SET_LOCAL_MODEL   = 0x23,

    eHCOM_RUN_RAMCODE       = 0x30,
    eHCOM_RUN_CAL_AUTO      = 0x31,
    eHCOM_RUN_MISCAL_AUTO   = 0x32,
    eHCOM_RUN_CAL_REF       = 0x33,

    eHCOM_SET_SEC_INFO_W_IDX= 0x40,
    eHCOM_SET_SEC_INFO_R_IDX= 0x41,

    eHCOM_NOTIRY_G_REGMAP   = 0x50,

    eHCOM_SET_JIG_MODE      = 0x80,
    eHCOM_SET_JIG_SENSITI   = 0x81,

    eHCOM_SET_REC_MODE      = 0xE0,
    eHCOM_SET_DBG_MODE      = 0xE1,

    eHCOM_DEFAULT           = 0xFF,
};

#define IST30XX_RINGBUF_NO_ERR      (0)
#define IST30XX_RINGBUF_NOT_ENOUGH  (1)
#define IST30XX_RINGBUF_EMPTY       (2)
#define IST30XX_RINGBUF_FULL        (3)
#define IST30XX_RINGBUF_TIMEOUT     (4)
#define IST30XX_MAX_RINGBUF_SIZE    (4 * 100 * 1024)
typedef struct _IST30XX_RINGBUF {
    u32 RingBufCtr;
    u32 RingBufInIdx;
    u32 RingBufOutIdx;
    u8 LogBuf[IST30XX_MAX_RINGBUF_SIZE];
} IST30XX_RING_BUF;

typedef union {
    struct {
        u32 y:12;
        u32 x:12;
        u32 mi:4;
        u32 ma:4;
    } bit_field;
    u32 full_field;
} finger_info;

struct ist30xx_status {
    int power;
    int update;
    int update_result;
    int calib;
    u32 calib_msg[IST30XX_MAX_CALIB_SIZE];
    int miscalib;
    u32 miscalib_msg[IST30XX_MAX_CALIB_SIZE];
    u32 cmcs;
    bool event_mode;
    bool noise_mode;
    int sys_mode;
};

struct ist30xx_version {
    u32 main_ver;
    u32 fw_ver;
    u32 core_ver;
    u32 test_ver;
};

struct ist30xx_fw {
    struct ist30xx_version cur;
    struct ist30xx_version bin;
    u32 index;
    u32 size;
    u32 chksum;
    u32 buf_size;
    u8 *buf;
};

#define IST30XX_TAG_MAGIC               ("ISTV2TAG")
struct ist30xx_tags {
    char magic1[8];
    u32 rom_base;
    u32 ram_base;
    u32 reserved0;
    u32 reserved1;

    u32 fw_addr;
    u32 fw_size;
    u32 cfg_addr;
    u32 cfg_size;
    u32 sensor_addr;
    u32 sensor_size;
    u32 cp_addr;
    u32 cp_size;
    u32 flag_addr;
    u32 flag_size;
    u32 reserved2;
    u32 reserved3;

    u32 zvalue_base;
    u32 algr_base;
    u32 cdc_base;
    u32 filter_base;
    u32 reserved4;
    u32 reserved5;

    u32 chksum;
    u32 chksum_all;
    u32 reserved6;
    u32 reserved7;

    u8 day;
    u8 month;
    u16 year;
    u8 hour;
    u8 min;
    u8 sec;
    u8 reserved8;
    char magic2[8];
};

struct CH_NUM {
    u8 tx;
    u8 rx;
};

struct GTX_INFO {
    u8 num;
    u8 ch_num[4];
};

struct TSP_NODE_BUF {
    u16 cdc[IST30XX_MAX_NODE_NUM];
    u16 self_cdc[IST30XX_MAX_SELF_NODE_NUM];
    u16 base[IST30XX_MAX_NODE_NUM];
    u16 self_base[IST30XX_MAX_SELF_NODE_NUM];
    u16 memx_cp[IST30XX_MAX_NODE_NUM];
    u16 memx_self_cp[IST30XX_MAX_SELF_NODE_NUM];
    u16 rom_cp[IST30XX_MAX_NODE_NUM];
    u16 rom_self_cp[IST30XX_MAX_SELF_NODE_NUM];
    u16 len;
    u16 self_len;
};

struct TSP_DIRECTION {
    bool swap_xy;
    bool flip_x;
    bool flip_y;
};

typedef struct _TSP_INFO {
    struct CH_NUM ch_num;
    struct CH_NUM screen;
    struct GTX_INFO gtx;
    struct TSP_DIRECTION dir;
    struct TSP_NODE_BUF node;
    int height;
    int width;
    int finger_num;
    u16 baseline;
} TSP_INFO;

typedef struct _TKEY_INFO {
    int key_num;
    bool enable;
    struct CH_NUM ch_num[IST30XX_MAX_KEYS];
    u16 baseline;
} TKEY_INFO;

typedef union {
    struct {
        u32 ctrl:8;
        u32 evt:8;
        u32 w:16;
        u32 h:16;
        u32 x:16;
        u32 y:16;
        u32 evt_x:16;
        u32 evt_y:16;
        u32 setting:8;
        u32 reserved:8;
    } b;
    u32 full[4];
} gesture_reg;

#ifdef SEC_FACTORY_MODE
#include "ist30xxh_sec.h"
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
struct ist30xx_dt_data {
    int irq_gpio;
    int power_gpio;
    bool is_power_by_gpio;
    const char *regulator_avdd;
    int fw_bin;
    const char *ic_version;
    const char *project_name;
    char fw_path[FIRMWARE_PATH_LENGTH];
#ifdef IST30XX_USE_CMCS
    char cmcs_path[FIRMWARE_PATH_LENGTH];
#endif
    int octa_hw;
    u32 x_max;
    u32 y_max;
    u32 area_indicator;
    u32 area_navigation;
    u32 area_edge;
#ifdef TCLM_CONCEPT
    int tclm_level;
    int afe_base;
#endif
#ifdef IST30XX_USE_KEY
	bool tkey_use_sec_sysfs;
#endif
};

struct ist30xx_data {
    struct mutex lock;
    struct mutex i2c_lock;
    struct mutex aod_lock;
    struct i2c_client *client;
    struct input_dev *input_dev;
    struct ist30xx_dt_data *dt_data;
    TSP_INFO tsp_info;
    TKEY_INFO tkey_info;
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif
    struct ist30xx_status status;
    struct ist30xx_fw fw;
    struct ist30xx_tags tags;
    struct pinctrl *pinctrl;
#ifdef SEC_FACTORY_MODE
    struct sec_factory sec;
#endif
    u32 chip_id;
    u32 max_fingers;
    u32 max_keys;
    u32 t_status;
    finger_info fingers[IST30XX_MAX_FINGERS];
    bool tsp_touched[IST30XX_MAX_FINGERS];
    u32 move_count[IST30XX_MAX_FINGERS];
#ifdef IST30XX_USE_KEY
    bool tkey_pressed[IST30XX_MAX_KEYS];
#endif
#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
    struct delayed_work ghost_check;
    u8 tsp_dump_lock;
#endif
    u32 cdc_addr;
    u32 self_cdc_addr;
    u32 rec_addr;
    u32 rec_size;
    u32 sec_info_addr;
    u32 zvalue_addr;
    u32 algorithm_addr;
    volatile bool irq_working;
    bool irq_enabled;
    bool initialized;
    bool ignore_delay;
    u32 noise_mode;
    u32 debug_mode;
    u32 jig_mode;
    u32 z_values[IST30XX_MAX_FINGERS];
    int report_rate;
    int idle_rate;
    bool suspend;
#ifdef USE_OPEN_CLOSE
    bool probe_done;
#endif
    bool spay;
    bool aod;
    gesture_reg g_reg;
    int scan_count;
    int scan_retry;
    int max_scan_retry;
    int timer_period_ms;
    struct timer_list event_timer;
    struct timespec t_current;
    u32 event_ms; 
    u32 timer_ms;
    int irq_err_cnt;
    int max_irq_err_cnt;
    u32 intr_debug1_addr;
    u32 intr_debug1_size;
    u32 intr_debug2_addr;
    u32 intr_debug2_size;
    u32 intr_debug3_addr;
    u32 intr_debug3_size;
    u32 rec_mode;
    int rec_delay;
    char *rec_file_name;
    u32 recording_scancnt;
    u32 debugging_mode;
    u32 debugging_addr;
    u8 debugging_size;
    u32 debugging_scancnt;
    u32 debugging_noise;
    u32 t_frame[2];
    struct delayed_work work_reset_check;
#ifdef IST30XX_NOISE_MODE
    struct delayed_work work_noise_protect;
#else
    struct delayed_work work_force_release;
#endif
    struct delayed_work work_debug_algorithm;
    u32 algr_addr;
    u32 algr_size;
#ifdef USE_TSP_TA_CALLBACKS
    struct tsp_callbacks callbacks;
#endif
#ifdef CONFIG_MUIC_NOTIFIER
    struct notifier_block muic_nb;
#endif
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
    struct notifier_block ccic_nb;
#endif
#ifdef CONFIG_VBUS_NOTIFIER
    struct notifier_block vbus_nb;
#endif
#ifdef TCLM_CONCEPT
	u8 cal_position;
	u8 root_of_calibration;
	struct sec_cal_position *tclm_string;
	u8 cal_pos_hist_queue[2 * CAL_HISTORY_QUEUE_MAX];
	u8 cal_pos_hist_last3[2 * CAL_HISTORY_QUEUE_SHORT_DISPLAY + 1];
	u8 cal_pos_hist_cnt;
	u8 cal_pos_hist_lastp;
	bool external_factory;
	struct delayed_work work_tclm_initialize;
    u8 cal_count;
    u16 tune_fix_ver;
    struct ts_test_result test_result;
#endif
    int touch_pressed_num;
    u8 ito_test[4];
    u8 check_multi;
    unsigned int multi_count;
    unsigned int wet_count;
    unsigned int dive_count;
    unsigned int comm_err_count;
    unsigned int checksum_result;
    unsigned int all_finger_count;

    long time_longest;
    struct timeval time_pressed[IST30XX_MAX_FINGERS];
    struct timeval time_released[IST30XX_MAX_FINGERS];

    u16 pressed_x[IST30XX_MAX_FINGERS];
    u16 pressed_y[IST30XX_MAX_FINGERS];
    u16 released_x[IST30XX_MAX_FINGERS];
    u16 released_y[IST30XX_MAX_FINGERS];

    unsigned int scrub_id;
    unsigned int scrub_x;
    unsigned int scrub_y;
};

typedef enum {
    SPONGE_EVENT_TYPE_SPAY = 0x04,
    SPONGE_EVENT_TYPE_AOD = 0x08,
    SPONGE_EVENT_TYPE_AOD_PRESS = 0x09,
    SPONGE_EVENT_TYPE_AOD_LONGPRESS = 0x0A,
    SPONGE_EVENT_TYPE_AOD_DOUBLETAB = 0x0B,
} SPONGE_EVENT_TYPE;

extern int ist30xx_log_level;
extern struct class *ist30xx_class;

void tsp_printk(int level, const char *fmt, ...);
void ist30xx_delay(unsigned int ms);
int ist30xx_intr_wait(struct ist30xx_data *data, long ms);

void ist30xx_enable_irq(struct ist30xx_data *data);
void ist30xx_disable_irq(struct ist30xx_data *data);
void ist30xx_set_ta_mode(bool charging);
void ist30xx_set_edge_mode(int mode);
void ist30xx_set_call_mode(int mode);
void ist30xx_set_cover_mode(int mode);
void ist30xx_start(struct ist30xx_data *data);

int ist30xx_read_reg(struct i2c_client *client, u32 reg, u32 *buf);
int ist30xx_read_cmd(struct ist30xx_data *data, u32 cmd, u32 *buf);
int ist30xx_write_cmd(struct ist30xx_data *data, u32 cmd, u32 val);
int ist30xx_read_buf(struct i2c_client *client, u32 cmd, u32 *buf, u16 len);
int ist30xx_write_buf(struct i2c_client *client, u32 cmd, u32 *buf, u16 len);
int ist30xx_burst_read(struct i2c_client *client, u32 addr, u32 *buf32, u16 len,
        bool bit_en);
int ist30xx_burst_write(struct i2c_client *client, u32 addr, u32 *buf32,
        u16 len);

int ist30xx_cmd_gesture(struct ist30xx_data *data, u16 value);
int ist30xx_cmd_start_scan(struct ist30xx_data *data);
int ist30xx_cmd_calibrate(struct ist30xx_data *data);
int ist30xx_cmd_miscalibrate(struct ist30xx_data *data);
int ist30xx_cmd_check_calib(struct ist30xx_data *data);
int ist30xx_cmd_update(struct ist30xx_data *data, int cmd);
int ist30xx_cmd_hold(struct ist30xx_data *data, int enable);

int ist30xx_power_on(struct ist30xx_data *data, bool download);
int ist30xx_power_off(struct ist30xx_data *data);
int ist30xx_reset(struct ist30xx_data *data, bool download);

int ist30xx_init_system(struct ist30xx_data *data);

int ist30xx_execute_force_calibration(struct ist30xx_data *data);
void ist30xx_display_dump_log(struct ist30xx_data *data);

#ifdef SEC_FACTORY_MODE
extern struct class *sec_class;
extern int sec_touch_sysfs(struct ist30xx_data *data);
extern int sec_fac_cmd_init(struct ist30xx_data *data);
extern void sec_fac_cmd_remove(struct ist30xx_data *data);
extern void sec_touch_sysfs_remove(struct ist30xx_data *data);
#endif

#ifdef TCLM_CONCEPT
int ist30xx_write_sec_info(struct ist30xx_data *data, u8 idx, u32 *buf32,
        int len);
int ist30xx_read_sec_info(struct ist30xx_data *data, u8 idx, u32 *buf32,
        int len);
void ist30xx_tclm_position_history(struct ist30xx_data *data);
void ist30xx_tclm_debug_info(struct ist30xx_data *data);
void ist30xx_tclm_root_of_cal(struct ist30xx_data *data, int pos);
bool ist30xx_tclm_check_condition_valid(struct ist30xx_data *data);
bool ist30xx_execute_tclm_package(struct ist30xx_data *data, int factory_mode);
bool ist30xx_tclm_get_nvm_all(struct ist30xx_data *data, bool add_flag);
#endif

#ifdef CONFIG_TRUSTONIC_TRUSTED_UI
extern void trustedui_mode_on(void);
extern void trustedui_mode_off(void);
#endif

#endif  /* __IST30XXH_H__ */
