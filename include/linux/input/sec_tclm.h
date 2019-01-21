
#ifndef _SEC_TCLM_H_
#define _SEC_TCLM_H_

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/device.h>

/* TCLM_CONCEPT  - start */
#define TCLM_LEVEL_NONE			0x00
#define TCLM_LEVEL_CLEAR_NV		0x01
#define TCLM_LEVEL_LOCKDOWN		0x02
#define TCLM_LEVEL_EVERYTIME	0x05

#define CAL_HISTORY_QUEUE_MAX				10
#define CAL_HISTORY_QUEUE_SHORT_DISPLAY		3

#define TCLM_AMBIENT_CAL			0
#define TCLM_OFFSET_CAL_SDC			1
#define TCLM_OFFSET_CAL_SEC			2

#define SEC_TCLM_NVM_ALL_SIZE			37

enum tclm_offset {
	SEC_TCLM_NVM_OFFSET_FAC_RESULT			= 0,
	SEC_TCLM_NVM_OFFSET_CAL_COUNT			= 1,
	SEC_TCLM_NVM_OFFSET_TUNE_VERSION		= 2,
	SEC_TCLM_NVM_OFFSET_CAL_POSITION		= 3,
	SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_COUNT		= 4,
	SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_LASTP		= 5,
	SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_ZERO		= 6,
	SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_SIZE		= 7,
	SEC_TCLM_NVM_OFFSET_HISTORY_QUEUE_SAVE		= 7,
	SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER		= 8,
	SEC_TCLM_NVM_ALL_DATA				= 9,
	SEC_TCLM_NVM_ALL_DATA_DONE			= 10,
	SEC_TCLM_NVM_OFFSET_DISASSEMBLE_COUNT		= 11,
};

#define CAL_POS_CMD(full_name, short_name)		.f_name = full_name, .s_name = short_name

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
	CALPOSITION_MAX			= 16,
};

struct sec_cal_position {
	const char *f_name;
	const char s_name;
};

/* TCLM_CONCEPT  - end */
struct sec_tclm_data {
	/* TCLM_CONCEPT */
	int tclm_level;
	int afe_base;
	/* TCLM_CONCEPT v1.0~ */
	u8 cal_count;
	int tune_fix_ver;
	bool external_factory;
	/* TCLM_CONCEPT  v1.6~ */
	u8 cal_position;
	u8 root_of_calibration;
	struct sec_cal_position *tclm_string;
	u8 cal_pos_hist_queue[2 * CAL_HISTORY_QUEUE_MAX];	/* 20 */
	u8 cal_pos_hist_last3[2 * CAL_HISTORY_QUEUE_SHORT_DISPLAY + 1];	/* 7 */
	u8 cal_pos_hist_cnt;
	u8 cal_pos_hist_lastp;
	struct i2c_client *client;
	int (*tclm_read)(struct i2c_client *client, int address);
	void (*tclm_write)(struct i2c_client *client, int address, int data);
	int (*tclm_execute_force_calibration)(struct i2c_client *client, int cal_mode);
	int irq; /* device irq */
	u8 nvm_all_data[SEC_TCLM_NVM_ALL_SIZE];
};

void sec_tclm_case(struct sec_tclm_data *data, int tclm_case);
bool sec_tclm_get_nvm_all(struct sec_tclm_data *data);
void sec_tclm_position_history(struct sec_tclm_data *data);
void sec_tclm_root_of_cal(struct sec_tclm_data *data, int pos);
void sec_tclm_debug_info(struct sec_tclm_data *data);
bool sec_execute_tclm_package(struct sec_tclm_data *data, int factory_mode);
bool sec_tclm_check_cal_case(struct sec_tclm_data *data);
void sec_tclm_initialize(struct sec_tclm_data *data);

#endif
