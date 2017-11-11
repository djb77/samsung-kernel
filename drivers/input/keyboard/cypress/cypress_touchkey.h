#ifndef _CYPRESS_TOUCHKEY_H
#define _CYPRESS_TOUCHKEY_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_SEC_DEBUG_TSP_LOG
#define SEC_DEBUG_TK_LOG
#endif

#ifdef SEC_DEBUG_TK_LOG
#include <linux/sec_debug.h>
#endif

#include <linux/i2c/touchkey_i2c.h>

#ifdef SEC_DEBUG_TK_LOG
#define tk_debug_dbg(mode, dev, fmt, ...)	\
({								\
	if (mode) {					\
		dev_dbg(dev, fmt, ## __VA_ARGS__);	\
		sec_debug_tsp_log(fmt, ## __VA_ARGS__);		\
	}				\
	else					\
		dev_dbg(dev, fmt, ## __VA_ARGS__);	\
})

#define tk_debug_info(mode, dev, fmt, ...)	\
({								\
	if (mode) {							\
		dev_info(dev, fmt, ## __VA_ARGS__);		\
		sec_debug_tsp_log(fmt, ## __VA_ARGS__);		\
	}				\
	else					\
		dev_info(dev, fmt, ## __VA_ARGS__);	\
})

#define tk_debug_err(mode, dev, fmt, ...)	\
({								\
	if (mode) {					\
		dev_err(dev, fmt, ## __VA_ARGS__);	\
		sec_debug_tsp_log(fmt, ## __VA_ARGS__);	\
	}				\
	else					\
		dev_err(dev, fmt, ## __VA_ARGS__); \
})
#else
#define tk_debug_dbg(mode, dev, fmt, ...)	dev_dbg(dev, fmt, ## __VA_ARGS__)
#define tk_debug_info(mode, dev, fmt, ...)	dev_info(dev, fmt, ## __VA_ARGS__)
#define tk_debug_err(mode, dev, fmt, ...)	dev_err(dev, fmt, ## __VA_ARGS__)
#endif

/* Touchkey Register */
#define KEYCODE_REG			0x03
#define BASE_REG			0x00
#define TK_STATUS_FLAG		0x07
#define TK_DIFF_DATA		0x16
#define TK_RAW_DATA			0x1E
#define TK_IDAC				0x0D
#define TK_COMP_IDAC		0x11
#define TK_BASELINE_DATA	0x26
#define TK_THRESHOLD		0x09
#define TK_FW_VER			0x04

#define TK_BIT_PRESS_EV		0x08
#define TK_BIT_KEYCODE		0x07

/* New command*/
#define TK_BIT_CMD_LEDCONTROL   0x40    // Owner for LED on/off control (0:host / 1:TouchIC)
#define TK_BIT_CMD_INSPECTION   0x20    // Inspection mode
#define TK_BIT_CMD_1mmSTYLUS    0x10    // 1mm stylus mode
#define TK_BIT_CMD_FLIP         0x08    // flip mode
#define TK_BIT_CMD_GLOVE        0x04    // glove mode
#define TK_BIT_CMD_TA_ON		0x02    // Ta mode
#define TK_BIT_CMD_REGULAR		0x01    // regular mode = normal mode

#define TK_BIT_WRITE_CONFIRM	0xAA
#define TK_BIT_EXIT_CONFIRM	0xBB

/* Status flag after sending command*/
#define TK_BIT_LEDCONTROL   0x40    // Owner for LED on/off control (0:host / 1:TouchIC)
#define TK_BIT_1mmSTYLUS    0x20    // 1mm stylus mode
#define TK_BIT_FLIP         0x10    // flip mode
#define TK_BIT_GLOVE        0x08    // glove mode
#define TK_BIT_TA_ON		0x04    // Ta mode
#define TK_BIT_REGULAR		0x02    // regular mode = normal mode
#define TK_BIT_LED_STATUS	0x01    // LED status

#define TK_CMD_LED_ON		0x10
#define TK_CMD_LED_OFF		0x20

#define TK_UPDATE_DOWN		1
#define TK_UPDATE_FAIL		-1
#define TK_UPDATE_PASS		0

/* update condition */
#define TK_RUN_UPDATE 1
#define TK_EXIT_UPDATE 2
#define TK_RUN_CHK 3

/* KEY_TYPE*/
#define TK_USE_2KEY_TYPE

#ifdef CONFIG_GLOVE_TOUCH
/* Flip cover*/
#define TKEY_FLIP_MODE
#endif

/* 1MM stylus */
#define TKEY_1MM_MODE

#define TK_SUPPORT_MT

/* Boot-up Firmware Update */
#define TK_HAS_FIRMWARE_UPDATE

#define FW_PATH "cypress/cypress_zerof.fw"

#define TKEY_FW_PATH "/sdcard/cypress/fw.bin"
#define TKEY_FW_FFU_PATH "ffu_tk.bin"

/* #define TK_INFORM_CHARGER	1 */
/* #define TK_USE_OPEN_DWORK */
#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCH_MBR31X5)
/* #define TK_USE_FWUPDATE_DWORK */
#define CRC_CHECK_INTERNAL
#else
#define TK_USE_FWUPDATE_DWORK
#endif

#ifdef TK_USE_OPEN_DWORK
#define	TK_OPEN_DWORK_TIME	10
#endif

#ifdef CONFIG_GLOVE_TOUCH
#define	TK_GLOVE_DWORK_TIME	300
#endif

#if defined(TK_INFORM_CHARGER)
struct touchkey_callbacks {
	void (*inform_charger)(struct touchkey_callbacks *, bool);
};
#endif

enum {
	FW_NONE = 0,
	FW_BUILT_IN,
	FW_HEADER,
	FW_IN_SDCARD,
	FW_EX_SDCARD,
	FW_FFU,
};

struct fw_image {
	u8 hdr_ver;
	u8 hdr_len;
	u16 first_fw_ver;
	u16 second_fw_ver;
	u16 third_ver;
	u32 fw_len;
	u16 checksum;
	u16 alignment_dummy;
	u8 data[0];
} __attribute__ ((packed));

/* mode change struct */
#define MODE_NONE 0
#define MODE_NORMAL 1
#define MODE_GLOVE 2
#define MODE_FLIP 3

#define CMD_GLOVE_ON 1
#define CMD_GLOVE_OFF 2
#define CMD_FLIP_ON 3
#define CMD_FLIP_OFF 4
#define CMD_GET_LAST_MODE 0xfd
#define CMD_MODE_RESERVED 0xfe
#define CMD_MODE_RESET 0xff

struct cmd_mode_change {
	int mode;
	int cmd;
};
struct mode_change_data {
	bool busy;
	int cur_mode;
	struct cmd_mode_change mtc;	/* mode to change */
	struct cmd_mode_change mtr;	/* mode to reserved */
	/* ctr vars */
	struct mutex mc_lock;
	struct mutex mcf_lock;
};

/*Parameters for i2c driver*/
struct touchkey_i2c {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct completion init_done;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct mutex lock;
	struct mutex i2c_lock;
	struct wake_lock fw_wakelock;
	struct device	*dev;
	int irq;
	int md_ver_ic; /*module ver*/
	int fw_ver_ic;
	int device_ver;
	int firmware_id;
#ifdef CRC_CHECK_INTERNAL
	int crc;
#endif
	struct touchkey_platform_data *pdata;
	char *name;
	int (*power)(int on);
	int update_status;
	bool enabled;
	int	src_fw_ver;
	int	src_md_ver;
#ifdef TK_USE_OPEN_DWORK
	struct delayed_work open_work;
#endif
#ifdef TK_INFORM_CHARGER
	struct touchkey_callbacks callbacks;
	bool charging_mode;
#endif
#ifdef TKEY_1MM_MODE
	bool enabled_1mm;
#endif
	bool status_update;
	struct work_struct update_work;
	struct workqueue_struct *fw_wq;
	u8 fw_path;
	const struct firmware *firm_data;
	struct fw_image *fw_img;
	bool do_checksum;
	struct pinctrl *pinctrl_irq;
	struct pinctrl *pinctrl_i2c;
	struct pinctrl *pinctrl_det;
	struct pinctrl_state *pin_state[4];
	struct pinctrl_state *pins_default;
	struct work_struct mode_change_work;
	struct mode_change_data mc_data;
	int ic_mode;
	int tsk_enable_glove_mode;
};

extern struct class *sec_class;
void touchkey_charger_infom(bool en);
extern unsigned int lcdtype;
extern unsigned int system_rev;
#endif /* _CYPRESS_TOUCHKEY_H */
