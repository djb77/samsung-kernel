/* Himax Android Driver Sample Code for HMX83100 chipset
*
* Copyright (C) 2015 Himax Corporation.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#ifndef HIMAX83100_H
#define HIMAX83100_H

#include <asm/segment.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

// #include <mach/board.h>

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/async.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/input/mt.h>
#include <linux/firmware.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/wakelock.h>
#include <linux/pinctrl/consumer.h>
#include "himax_platform.h"

#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#define HIMAX_DRIVER_VER "0.0.9.0"

#define HIMAX_83100_NAME "Himax_83100"
#define HIMAX_83100_FINGER_SUPPORT_NUM 10
#define HIMAX_I2C_ADDR				0x48
#define INPUT_DEV_NAME	"himax-touchscreen"
#define FLASH_DUMP_FILE "/data/user/Flash_Dump.bin"
#define DIAG_COORDINATE_FILE "/sdcard/Coordinate_Dump.csv"

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#define D(x...) printk("[HXTSP] " x)
#define I(x...) printk("[HXTSP] " x)
#define W(x...) printk("[HXTSP][WARNING] " x)
#define E(x...) printk("[HXTSP][ERROR] " x)
#define DIF(x...) \
	if (debug_flag) \
	printk("[HXTSP][DEBUG] " x) \
} while(0)

#define HX_TP_SYS_DIAG
#define HX_TP_SYS_REGISTER
#define HX_TP_SYS_DEBUG
#define HX_TP_SYS_FLASH_DUMP
#define HX_TP_SYS_SELF_TEST
#define HX_TP_SYS_RESET 
#define HX_TP_SYS_HITOUCH
//#define HX_TP_SYS_2T2R

#else
#define D(x...) 
#define I(x...) 
#define W(x...) 
#define E(x...) 
#define DIF(x...)
#endif
//===========Himax Option function=============
//#define HX_RST_PIN_FUNC
#define HX_AUTO_UPDATE_FW
//#define HX_HIGH_SENSE
//#define HX_SMART_WAKEUP
//#define HX_USB_DETECT
//#define HX_ESD_WORKAROUND
//#define HX_LCD_FEATURE

//#define HX_EN_SEL_BUTTON		       // Support Self Virtual key		,default is close
//#define HX_EN_MUT_BUTTON		       // Support Mutual Virtual Key	,default is close


#define HX_85XX_A_SERIES_PWON		1
#define HX_85XX_B_SERIES_PWON		2
#define HX_85XX_C_SERIES_PWON		3
#define HX_85XX_D_SERIES_PWON		4
#define HX_85XX_E_SERIES_PWON		5
#define HX_85XX_ES_SERIES_PWON		6
#define HX_85XX_F_SERIES_PWON		7
#define HX_83100_SERIES_PWON		8

#define HX_TP_BIN_CHECKSUM_SW		1
#define HX_TP_BIN_CHECKSUM_HW		2
#define HX_TP_BIN_CHECKSUM_CRC		3
	
#define HX_KEY_MAX_COUNT             4			
#define DEFAULT_RETRY_CNT            3

#define HX_VKEY_0   KEY_BACK
#define HX_VKEY_1   KEY_HOME
#define HX_VKEY_2   KEY_RESERVED
#define HX_VKEY_3   KEY_RESERVED
#define HX_KEY_ARRAY    {HX_VKEY_0, HX_VKEY_1, HX_VKEY_2, HX_VKEY_3}

#define SHIFTBITS 5
//#define FLASH_SIZE 131072
#define  FW_SIZE_60k 	61440
#define  FW_SIZE_64k 	65536
#define  FW_SIZE_124k 	126976
#define  FW_SIZE_128k 	131072

struct himax_virtual_key {
	int index;
	int keycode;
	int x_range_min;
	int x_range_max;
	int y_range_min;
	int y_range_max;
};

struct himax_config {
	uint8_t  default_cfg;
	uint8_t  sensor_id;
	uint8_t  fw_ver;
	uint16_t length;
	uint32_t tw_x_min;
	uint32_t tw_x_max;
	uint32_t tw_y_min;
	uint32_t tw_y_max;
	uint32_t pl_x_min;
	uint32_t pl_x_max;
	uint32_t pl_y_min;
	uint32_t pl_y_max;
	uint8_t c1[11];
	uint8_t c2[11];
	uint8_t c3[11];
	uint8_t c4[11];
	uint8_t c5[11];
	uint8_t c6[11];
	uint8_t c7[11];
	uint8_t c8[11];
	uint8_t c9[11];
	uint8_t c10[11];
	uint8_t c11[11];
	uint8_t c12[11];
	uint8_t c13[11];
	uint8_t c14[11];
	uint8_t c15[11];
	uint8_t c16[11];
	uint8_t c17[11];
	uint8_t c18[17];
	uint8_t c19[15];
	uint8_t c20[5];
	uint8_t c21[11];
	uint8_t c22[4];
	uint8_t c23[3];
	uint8_t c24[3];
	uint8_t c25[4];
	uint8_t c26[2];
	uint8_t c27[2];
	uint8_t c28[2];
	uint8_t c29[2];
	uint8_t c30[2];
	uint8_t c31[2];
	uint8_t c32[2];
	uint8_t c33[2];
	uint8_t c34[2];
	uint8_t c35[3];
	uint8_t c36[5];
	uint8_t c37[5];
	uint8_t c38[9];
	uint8_t c39[14];
	uint8_t c40[159];
	uint8_t c41[99];
};

struct himax_ts_data {
	bool suspended;
	atomic_t suspend_mode;
	uint8_t x_channel;
	uint8_t y_channel;
	uint8_t useScreenRes;
	uint8_t diag_command;
	
	uint8_t protocol_type;
	uint8_t first_pressed;
	uint8_t coord_data_size;
	uint8_t area_data_size;
	uint8_t raw_data_frame_size;
	uint8_t raw_data_nframes;
	uint8_t nFinger_support;
	uint8_t irq_enabled;
	uint8_t diag_self[50];
	
	uint16_t finger_pressed;
	uint16_t last_slot;
	uint16_t pre_finger_mask;

	uint32_t debug_log_level;
	uint32_t widthFactor;
	uint32_t heightFactor;
	uint32_t tw_x_min;
	uint32_t tw_x_max;
	uint32_t tw_y_min;
	uint32_t tw_y_max;
	uint32_t pl_x_min;
	uint32_t pl_x_max;
	uint32_t pl_y_min;
	uint32_t pl_y_max;
	
	int use_irq;
	int vendor_fw_ver;
	int vendor_config_ver;
	int vendor_sensor_id;
	int (*power)(int on);
	int pre_finger_data[10][2];
	char phys[32];

	struct device *dev;
	struct workqueue_struct *himax_wq;
	struct work_struct work;
	struct input_dev *input_dev;
	struct hrtimer timer;
	struct i2c_client *client;
	struct himax_i2c_platform_data *pdata;	
	struct himax_virtual_key *button;
	
#ifdef HX_TP_SYS_FLASH_DUMP
	struct workqueue_struct 			*flash_wq;
	struct work_struct 					flash_work;
#endif
	int rst_gpio;

	struct pinctrl *pinctrl;

#ifdef HX_SMART_WAKEUP
	uint8_t SMWP_enable;
	struct wake_lock ts_SMWP_wake_lock;
	struct workqueue_struct *himax_smwp_wq;
	struct delayed_work smwp_work;
#endif

#ifdef HX_HIGH_SENSE
	uint8_t HSEN_enable;
#endif

#ifdef HX_USB_DETECT
	uint8_t usb_connected;
	uint8_t *cable_config;
#endif

#ifdef SEC_FACTORY_TEST
	struct tsp_factory_info 		*factory_info;
#endif

};

static struct himax_ts_data *private_ts;

#define HX_CMD_NOP					 0x00	
#define HX_CMD_SETMICROOFF			 0x35	
#define HX_CMD_SETROMRDY			 0x36	
#define HX_CMD_TSSLPIN				 0x80	
#define HX_CMD_TSSLPOUT 			 0x81	
#define HX_CMD_TSSOFF				 0x82	
#define HX_CMD_TSSON				 0x83	
#define HX_CMD_ROE					 0x85	
#define HX_CMD_RAE					 0x86	
#define HX_CMD_RLE					 0x87	
#define HX_CMD_CLRES				 0x88	
#define HX_CMD_TSSWRESET			 0x9E	
#define HX_CMD_SETDEEPSTB			 0xD7	
#define HX_CMD_SET_CACHE_FUN		 0xDD	
#define HX_CMD_SETIDLE				 0xF2	
#define HX_CMD_SETIDLEDELAY 		 0xF3	
#define HX_CMD_SELFTEST_BUFFER		 0x8D	
#define HX_CMD_MANUALMODE			 0x42
#define HX_CMD_FLASH_ENABLE 		 0x43
#define HX_CMD_FLASH_SET_ADDRESS	 0x44
#define HX_CMD_FLASH_WRITE_REGISTER  0x45
#define HX_CMD_FLASH_SET_COMMAND	 0x47
#define HX_CMD_FLASH_WRITE_BUFFER	 0x48
#define HX_CMD_FLASH_PAGE_ERASE 	 0x4D
#define HX_CMD_FLASH_SECTOR_ERASE	 0x4E
#define HX_CMD_CB					 0xCB
#define HX_CMD_EA					 0xEA
#define HX_CMD_4A					 0x4A
#define HX_CMD_4F					 0x4F
#define HX_CMD_B9					 0xB9
#define HX_CMD_76					 0x76

enum input_protocol_type {
	PROTOCOL_TYPE_A	= 0x00,
	PROTOCOL_TYPE_B	= 0x01,
};

enum fw_image_type {
	fw_image_60k	= 0x01,
	fw_image_64k,
	fw_image_124k,
	fw_image_128k,
};

#ifdef HX_TP_SYS_REGISTER
	static uint8_t register_command[4];
#endif

#ifdef HX_TP_SYS_DIAG
#ifdef HX_TP_SYS_2T2R
	static bool Is_2T2R = false;
	static uint8_t x_channel_2 = 0;
	static uint8_t y_channel_2 = 0;
	static uint8_t *diag_mutual_2 = NULL;
	
	static uint8_t *getMutualBuffer_2(void);
	static uint8_t 	getXChannel_2(void);
	static uint8_t 	getYChannel_2(void);
	
	static void 	setMutualBuffer_2(void);
	static void 	setXChannel_2(uint8_t x);
	static void 	setYChannel_2(uint8_t y);
#endif
	static uint8_t x_channel 		= 0;
	static uint8_t y_channel 		= 0;
	static uint8_t *diag_mutual = NULL;

	static int diag_command = 0;
	static uint8_t diag_coor[128];// = {0xFF};
	static uint8_t diag_self[100] = {0};

	static uint8_t *getMutualBuffer(void);
	static uint8_t *getSelfBuffer(void);
	static uint8_t 	getDiagCommand(void);
	static uint8_t 	getXChannel(void);
	static uint8_t 	getYChannel(void);
	
	static void 	setMutualBuffer(void);
	static void 	setXChannel(uint8_t x);
	static void 	setYChannel(uint8_t y);
	
	static uint8_t	coordinate_dump_enable = 0;
	struct file	*coordinate_fn;
#endif

#ifdef HX_TP_SYS_DEBUG
	static bool	fw_update_complete = false;
	static int handshaking_result = 0;
	static unsigned char debug_level_cmd = 0;
	static unsigned char upgrade_fw[128*1024];
#endif

#ifdef HX_TP_SYS_FLASH_DUMP
	static int Flash_Size = 131072;
	static uint8_t *flash_buffer 				= NULL;
	static uint8_t flash_command 				= 0;
	static uint8_t flash_read_step 			= 0;
	static uint8_t flash_progress 			= 0;
	static uint8_t flash_dump_complete	= 0;
	static uint8_t flash_dump_fail 			= 0;
	static uint8_t sys_operation				= 0;
	static uint8_t flash_dump_sector	 	= 0;
	static uint8_t flash_dump_page 			= 0;
	static bool    flash_dump_going			= false;

	static uint8_t getFlashCommand(void);
	static uint8_t getFlashDumpComplete(void);
	static uint8_t getFlashDumpFail(void);
	static uint8_t getFlashDumpProgress(void);
	static uint8_t getFlashReadStep(void);
	static uint8_t getSysOperation(void);
	static uint8_t getFlashDumpSector(void);
	static uint8_t getFlashDumpPage(void);

	static void setFlashBuffer(void);
	static void setFlashCommand(uint8_t command);
	static void setFlashReadStep(uint8_t step);
	static void setFlashDumpComplete(uint8_t complete);
	static void setFlashDumpFail(uint8_t fail);
	static void setFlashDumpProgress(uint8_t progress);
	static void setSysOperation(uint8_t operation);
	static void setFlashDumpSector(uint8_t sector);
	static void setFlashDumpPage(uint8_t page);
	static void setFlashDumpGoing(bool going);
#endif

#ifdef HX_RST_PIN_FUNC
	void himax_HW_reset(uint8_t loadconfig,uint8_t int_off);
#endif

#ifdef HX_TP_SYS_SELF_TEST
	static ssize_t himax_chip_self_test_function(struct device *dev, struct device_attribute *attr, char *buf);
	static int himax_chip_self_test(void);
	uint32_t **raw_data_array;
	uint8_t X_NUM = 0, Y_NUM = 0;
	uint8_t sel_type = 0x0D;
#endif

#ifdef HX_TP_SYS_HITOUCH
	static int	hitouch_command			= 0;
	static bool hitouch_is_connect	= false;
#endif

#ifdef HX_SMART_WAKEUP
	static bool FAKE_POWER_KEY_SEND = false;
	uint8_t HX_SMWP_EN;
#endif

#ifdef HX_ESD_WORKAROUND
	u8 		HX_ESD_RESET_ACTIVATE 	= 1;
#endif

#ifdef MTK
#define TPD_DEVICE            "mtk-tpd"

extern struct tpd_device *tpd;
struct i2c_client *i2c_client_point = NULL;
static int tpd_flag = 0;
static struct task_struct *touch_thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static unsigned short force[] = {0, 0x90, I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short *const forces[] = { force, NULL };

//Custom set some config
static int hx_panel_coords[4] = {0,720,0,1280};//[1]=X resolution, [3]=Y resolution
static int hx_display_coords[4] = {0,720,0,1280};
static int report_type = PROTOCOL_TYPE_B;

static int himax_83100_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int himax_83100_detect(struct i2c_client *client, struct i2c_board_info *info);
static int himax_83100_remove(struct i2c_client *client);
extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
#endif

extern int irq_enable_count;


#ifdef SEC_FACTORY_TEST

//extern struct class *sec_class;


/* Touch Screen */
#define TSP_CMD_STR_LEN			32
#define TSP_CMD_RESULT_STR_LEN	512
#define TSP_CMD_PARAM_NUM		8
#define TSP_CMD_Y_NUM			10
#define TSP_CMD_X_NUM			20
#define TSP_CMD_NODE_NUM		(TSP_CMD_Y_NUM * TSP_CMD_X_NUM)

struct tsp_factory_info {
	struct list_head cmd_list_head;
	char cmd[TSP_CMD_STR_LEN];
	char cmd_param[TSP_CMD_PARAM_NUM];
	char cmd_result[TSP_CMD_RESULT_STR_LEN];
	char cmd_buff[TSP_CMD_RESULT_STR_LEN];
	struct mutex cmd_lock;
	bool cmd_is_running;
	u8 cmd_state;
};

enum {
	WAITING = 0,
	RUNNING,
	OK,
	FAIL,
	NOT_APPLICABLE,
};

struct tsp_cmd {
	struct list_head list;
	const char *cmd_name;
	void (*cmd_func)(void *device_data);
};

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void module_off_slave(void *device_data);
static void module_on_slave(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_threshold(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void get_config_ver(void *device_data);
static void not_support_cmd(void *device_data);
#endif

#endif
