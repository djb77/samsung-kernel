
#ifndef _INPUT_BOOSTER_H_
#define _INPUT_BOOSTER_H_

#include <linux/cpufreq.h>
#include <linux/device.h>

#define INPUT_BOOSTER_NAME	"input_booster"

enum input_booster_id {
	INPUT_BOOSTER_ID_TSP = 0,
	INPUT_BOOSTER_ID_TKEY,
	INPUT_BOOSTER_ID_WACOM,
};

#define DVFS_STAGE_NONE		1 << 0	// 0000 0000 0000 0001
#define DVFS_STAGE_SINGLE	1 << 1	// 0000 0000 0000 0010
#define DVFS_STAGE_DUAL		1 << 2	// 0000 0000 0000 0100
#define DVFS_STAGE_TRIPLE	1 << 3	// 0000 0000 0000 1000
#define DVFS_STAGE_PENTA	1 << 5	// 0000 0000 0010 0000
#define DVFS_STAGE_NINTH	1 << 9	// 0000 0010 0000 0000


/* Touchkey */
#define INPUT_BOOSTER_OFF_TIME_TKEY		500
#define INPUT_BOOSTER_CHG_TIME_TKEY		500


struct input_booster {
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	struct mutex		dvfs_lock;

	bool dvfs_lock_status;
	int dvfs_old_stauts;
	int dvfs_boost_mode;
	int dvfs_freq;
	int bimc_freq;
	int dvfs_id;
	int dvfs_stage;

	int (*dvfs_off)(struct input_booster *);
	void (*dvfs_set)(struct input_booster *, int);
};

struct dvfs {
	int time;
	s32 cpu_freq;
	s32 bimc_freq;
};

struct input_booster_dt_data {
	int tsp_stage;
	int tkey_stage;
	int wacom_stage;
	int level;
	struct dvfs head;
	struct dvfs tail;
};

struct input_booster_data {
	struct device *dev;
	struct input_booster_dt_data *dt_data;
	struct class *booster_class;
	int dbg_level;
	int level;
	struct dvfs head;
	struct dvfs tail;
};

struct input_booster *input_booster_allocate(int id);
void input_booster_free(struct input_booster *booster);
void input_booster_get_default_setting(const char *flag, struct dvfs *value);
int input_booster_set_level_change(int val);

#ifdef CONFIG_DEBUG_BUS_VOTER
extern int msm_bus_floor_vote(const char *name, u64 floor_hz);
#endif

#endif /* _INPUT_BOOSTER_H_ */
