/*
 *  ss_thermal.h
 *  Samsung thermal mitigation through core wear levelling
 *
 *  Copyright (C) 2015 Samsung Electronics
 *
 */

#define CORE_MITIGATION_TEMP_VALUE 940
#define TSENS_NAME_MAX_LENGTH 20
#define SENSOR_ID_0 7
#define SENSOR_ID_1 6
#define SENSOR_ID_2 9
#define SENSOR_ID_3 10
#define SENSOR_ID_4 5
#define SENSOR_ID_5 4
#define SENSOR_ID_6 12
#define SENSOR_ID_7 11

#define CORE_ID_0 0
#define CORE_ID_1 1
#define CORE_ID_2 2
#define CORE_ID_3 3
#define CORE_ID_4 4
#define CORE_ID_5 5
#define CORE_ID_6 6
#define CORE_ID_7 7
#define BOOT_CPU_SENSOR  7

#define TEMP_STEP_UP_VALUE 50
#define TEMP_STEP_DOWN_VALUE 100
#define MAX_CORE_WEAR_LEVEL 6

#define HOTPLUG_UPDATE_CORE_CTRL  "/sys/module/msm_thermal/core_control/cpus_offlined"
#define BUF_MAX 12

struct core_mitigation_temp_unplug {
    unsigned int core_mitigation_temp_step[8];
    unsigned int core_to_unplug[8];
};

struct tsens_id_to_core {
    unsigned int sensor_id;
    unsigned int core_id;
    bool unplugged;
};

#define SENSOR_CORE_INIT(num)		\
{					\
	.sensor_id = SENSOR_ID_##num,   \
	.core_id = CORE_ID_##num,       \
}
