#ifndef _LINUX_CYPRESS_TOUCHKEY_I2C_H
#define _LINUX_CYPRESS_TOUCHKEY_I2C_H

#include <linux/types.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/* LDO Regulator */
#define	TK_REGULATOR_NAME	"vtouch_1.8v"

/* LDO Regulator */
#define	TK_LED_REGULATOR_NAME	"vtouch_3.3v"

#define	TK_CORE_REGULATOR_NAME	"vtouch_3.3v"
#define TK_IO_REGULATOR_NAME	"vtouch_1.8v"

/* LED LDO Type*/
#define LED_LDO_WITH_REGULATOR

#define CYPRESS_IC_20055 20055
#define CYPRESS_IC_20075 20075
#define CYPRESS_IC_MBR3155 3155

struct touchkey_platform_data {
	int gpio_sda;
	int gpio_scl;
	int gpio_int;
	int sub_det;
	u32 irq_gpio_flags;
	u32 sda_gpio_flags;
	u32 scl_gpio_flags;
	u32 sub_det_flags;
	bool i2c_gpio;
	u32 stabilizing_time;
	u32 ic_type;
	bool ap_io_power;
	bool boot_on_ldo;
	bool led_by_ldo;
	bool glove_mode_keep_status;
	char *fw_path;
	void (*init_platform_hw)(void);
	int (*suspend) (void);
	int (*resume) (void);
	int (*power_on)(void *, bool);
	int (*led_power_on) (void *, bool);
	int (*reset_platform_hw)(void);
	void (*register_cb)(void *);
};

#endif /* _LINUX_CYPRESS_TOUCHKEY_I2C_H */
