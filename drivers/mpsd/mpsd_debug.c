/*
 * Source file for mpsd debug framework.
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *	   Ravi Solanki <ravi.siso@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/compiler.h>
#include <linux/kobject.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/cpufreq.h>

#include "mpsd_param.h"

enum param_level {
	APP_LEVEL = 0,
	DEV_LEVEL,
	ALL_LEVEL
};

#define MAX_PRINT_SIZE (5 * PAGE_SIZE)
#define MAX_INT_ARR_SIZE (MAX_NUM_CPUS)
#define MAX_PARAM_NAME_LENGTH (50)
#define MAX_EVENT_DATA_SIZE (100)
#define MAX_CPULOAD_PRINT_LIMIT (100)

#define DEFAULT_DEBUG_LEVEL_GLOBAL (0)
#define DEFAULT_DEBUG_LEVEL_DRIVER (2)
#define DEFAULT_DEBUG_LEVEL_READ (2)
#define DEFAULT_DEBUG_LEVEL_TIMER (2)
#define DEFAULT_DEBUG_LEVEL_NOTIFIER (2)

#define DEFAULT_DEBUG_PID (DEFAULT_PID)
#define DEFAULT_DEBUG_PARAM (-1)
#define DEFAULT_DEBUG_PARAM_LEVEL (DEV_LEVEL)

#define DEFAULT_DEBUG_APP_FIELD (0)
#define DEFAULT_DEBUG_DEV_FIELD (0)

#define DEFAULT_DEBUG_NOTIFIER_REG_FLAG (1)
#define DEFAULT_DEBUG_NOTIFIER_EVENTS (0)
#define DEFAULT_DEBUG_NOTIFIER_DELTA (-1)
#define DEFAULT_DEBUG_NOTIFIER_MIN (-1)
#define DEFAULT_DEBUG_NOTIFIER_MAX (-1)
#define DEFAULT_NOTIFIER_EVENTS_DATA (-1)
#define DEFAULT_NOTIFIER_EVENTS_DATA_IDX (0)

#define DEFAULT_DEBUG_TIMER_REG_FLAG (1)
#define DEFAULT_DEBUG_TIMER_VAL (5000)
#define DEFAULT_DEBUG_TIMER_COUNT (0)
#define DEFAULT_TIMER_EVENTS_DATA (-1)
#define DEFAULT_TIMER_EVENTS_DATA_IDX (0)

#define DEFAULT_USE_SNAPSHOT (1)
#define DEFAULT_DETAILED_INFO (0)

#define DEFAULT_NOTIFIER_REL (0)
#define DEFAULT_TIMER_REL (0)

struct kobject *mpsd_debug_kobj;

static unsigned int mpsd_debug_level = DEFAULT_DEBUG_LEVEL_GLOBAL;
static unsigned int mpsd_debug_level_driver = DEFAULT_DEBUG_LEVEL_DRIVER;
static unsigned int mpsd_debug_level_read = DEFAULT_DEBUG_LEVEL_READ;
static unsigned int mpsd_debug_level_timer = DEFAULT_DEBUG_LEVEL_TIMER;
static unsigned int mpsd_debug_level_notifier = DEFAULT_DEBUG_LEVEL_NOTIFIER;

static int debug_pid = DEFAULT_DEBUG_PID;
static int debug_param = DEFAULT_DEBUG_PARAM;
static int debug_param_level = DEFAULT_DEBUG_PARAM_LEVEL;

static unsigned long long debug_app_field = DEFAULT_DEBUG_APP_FIELD;
static unsigned long long debug_dev_field = DEFAULT_DEBUG_DEV_FIELD;

static int debug_notifier_param = DEFAULT_DEBUG_PARAM;
static int debug_notifier_reg_unreg_flag = DEFAULT_DEBUG_NOTIFIER_REG_FLAG;
static unsigned int debug_notifier_events = DEFAULT_DEBUG_NOTIFIER_EVENTS;
static long long debug_notifier_delta = DEFAULT_DEBUG_NOTIFIER_DELTA;
static long long debug_notifier_min = DEFAULT_DEBUG_NOTIFIER_MIN;
static long long debug_notifier_max = DEFAULT_DEBUG_NOTIFIER_MAX;
static unsigned int notifier_events_data_idx = DEFAULT_NOTIFIER_EVENTS_DATA_IDX;

static int debug_timer_param = DEFAULT_DEBUG_PARAM;
static int debug_timer_reg_unreg_flag = DEFAULT_DEBUG_TIMER_REG_FLAG;
static long long debug_timer_val = DEFAULT_DEBUG_TIMER_VAL;
static long long debug_timer_count = DEFAULT_DEBUG_TIMER_COUNT;
static unsigned int timer_events_data_idx = DEFAULT_TIMER_EVENTS_DATA_IDX;

static int use_snapshot = DEFAULT_USE_SNAPSHOT;
static int detailed_info = DEFAULT_DETAILED_INFO;

static int notifier_release = DEFAULT_NOTIFIER_REL;
static int timer_release = DEFAULT_TIMER_REL;

#define PARAM_TYPE_MIN (0)
/**
 * Enum for types of parameter values.
 */
enum param_type {
	PARAM_TYPE_INT = PARAM_TYPE_MIN,
	PARAM_TYPE_INT_ARR,
	PARAM_TYPE_INT_2D_ARR,
	PARAM_TYPE_STR,
	PARAM_TYPE_STR_ARR,
	PARAM_TYPE_CPU_FREQ_TIME_STATS,
	PARAM_TYPE_CPU_FREQ_GOV,
	PARAM_TYPE_MAX
};

static const char * const app_param_name[] = {
	"A-NAME",
	"A-TGID",
	"A-FLAGS",
	"A-PRIORITY",
	"A-NICE",
	"A-CPUS_ALLOWED",
	"A-VOLUNTARY_CTXT_SWITCHES",
	"A-NONVOLUNTARY_CTXT_SWITCHES",
	"A-NUM_THREADS",
	"A-OOM_SCORE_ADJ",
	"A-OOM_SCORE_ADJ_MIN",
	"A-UTIME",
	"A-STIME",
	"A-MEM_MIN_PGFLT",
	"A-MEM_MAJ_PGFLT",
	"A-STARTTIME",
	"A-MEM_VSIZE",
	"A-MEM_DATA",
	"A-MEM_STACK",
	"A-MEM_TEXT",
	"A-MEM_SWAP",
	"A-MEM_SHARED",
	"A-MEM_ANON",
	"A-MEM_RSS",
	"A-MEM_RSS_LIMIT",
	"A-IO_RCHAR",
	"A-IO_WCHAR",
	"A-IO_SYSCR",
	"A-IO_SYSCW",
	"A-IO_SYSCFS",
	"A-IO_BYTES_READ",
	"A-IO_BYTES_WRITE",
	"A-IO_BYTES_WRITE_CANCEL"
};

static const char * const dev_param_name[] = {
	"D-CPU_PRESENT",
	"D-CPU_ONLINE",
	"D-CPU_UTIL",
	"D-CPU_FREQ_AVAIL",
	"D-CPU_MAX_LIMIT",
	"D-CPU_MIN_LIMIT",
	"D-CPU_CUR_FREQ",
	"D-CPU_FREQ_TIME_STATS",
	"D-CPU_FREQ_GOV",
	"D-CPU_LOADAVG_1MIN",
	"D-CPU_LOADAVG_5MIN",
	"D-CPU_LOADAVG_15MIN",
	"D-SYSTEM_UPTIME",
	"D-CPU_TIME_USER",
	"D-CPU_TIME_NICE",
	"D-CPU_TIME_SYSTEM",
	"D-CPU_TIME_IDLE",
	"D-CPU_TIME_IO_WAIT",
	"D-CPU_TIME_IRQ",
	"D-CPU_TIME_SOFT_IRQ",
	"D-MEM_TOTAL",
	"D-MEM_FREE",
	"D-MEM_AVAIL",
	"D-MEM_BUFFER",
	"D-MEM_CACHED",
	"D-MEM_SWAP_CACHED",
	"D-MEM_SWAP_TOTAL",
	"D-MEM_SWAP_FREE",
	"D-MEM_DIRTY",
	"D-MEM_ANON_PAGES",
	"D-MEM_MAPPED",
	"D-MEM_SHMEM",
	"D-MEM_SYSHEAP",
	"D-MEM_SYSHEAP_POOL",
	"D-MEM_VMALLOC_API",
	"D-MEM_KGSL_SHARED",
	"D-MEM_ZSWAP"
};
/**
 * struct debug_param_val - Contains types and data for various types of params.
 * @type: The type of param value.
 * @int_val: If param value is integer value will be copied here.
 * @int_arr_val: If param value is integer array value will be copied here.
 * @int_2d_arr_val: If param value is 2d integer array value will be copied here.
 * @int_2d_arr_cpuload_val: If param value is for cpuload, value will be copied here.
 * @str_val: If param value is string value will be copied here.
 * @str_arr_val: If param value is array of strings value will be copied here.
 * @cpu_freq_time_stats: Param data of the cpufreq time in stats.
 * @cpu_freq_gov_val: Param data of the cpufreq governors.
 *
 * This structure contains all the application specific kernel parameters.
 */
struct debug_param_val {
	enum param_type type;
	long long int_val;
	long long int_arr_val[MAX_NUM_CPUS];
	long long int_2d_arr_val[MAX_NUM_CPUS][MAX_FREQ_LIST_SIZE];
	long long int_2d_arr_cpuload_val[MAX_NUM_CPUS][MAX_LOAD_HISTORY];
	char str_val[MAX_CHAR_BUF_SIZE];
	char str_arr_val[MAX_NUM_CPUS][MAX_CHAR_BUF_SIZE];
	struct dev_cpu_freq_time_stats cpu_freq_time_stats[MAX_NUM_CPUS][MAX_FREQ_LIST_SIZE];
	struct dev_cpu_freq_gov cpu_freq_gov_val[MAX_NUM_CPUS];
};

/**
 * struct debug_notifier_event_data - Debug struct containg event data
 * @time_stamp: At what time timer event came.
 * @param: Param id of the paramater, either app_param or dev_prama type.
 * @events: Gives the information which event has just occured.
 * @prev: Previous value of the parameter.
 * @cur: Current value of the parameter
 * @app_field_mask: Bit field for which param event came.
 * @dev_field_mask: Bit field for which param event came.
 *
 * This structure contains event data.
 */
struct debug_notifier_event_data {
	long long time_stamp;
	int param;
	int events;
	long long prev;
	long long cur;
	long long notifier_app_field;
	long long notifier_dev_field;
};
static struct debug_notifier_event_data notifier_events_data[MAX_EVENT_DATA_SIZE];

/**
 * struct debug_timer_event_data - Debug struct containg timer event data
 * @time_stamp: At what time timer event came.
 * @value: For which timer the event came.
 * @count: How many params reg for this timer value.
 * @app_field_mask: Bit field for which param timer came.
 * @dev_field_mask: Bit field for which param timer came.
 *
 * This structure contains event data.
 */
struct debug_timer_event_data {
	long long time_stamp;
	long long value;
	int count;
	long long timer_app_field;
	long long timer_dev_field;
};
static struct debug_timer_event_data timer_events_data[MAX_EVENT_DATA_SIZE];

unsigned int get_mpsd_debug_level(void)
{
	return mpsd_debug_level;
}

static void set_mpsd_debug_level(unsigned int val)
{
	mpsd_debug_level = val;
}

unsigned int get_debug_level_driver(void)
{
	return mpsd_debug_level_driver;
}

static void set_debug_level_driver(unsigned int val)
{
	mpsd_debug_level_driver = val;
}

unsigned int get_debug_level_read(void)
{
	return mpsd_debug_level_read;
}

static void set_debug_level_read(unsigned int val)
{
	mpsd_debug_level_read = val;
}

unsigned int get_debug_level_timer(void)
{
	return mpsd_debug_level_timer;
}

static void set_debug_level_timer(unsigned int val)
{
	mpsd_debug_level_timer = val;
}

unsigned int get_debug_level_notifier(void)
{
	return mpsd_debug_level_notifier;
}

static void set_debug_level_notifier(unsigned int val)
{
	mpsd_debug_level_notifier = val;
}

static void init_debug_framework(void)
{
	int i = 0;

	mpsd_debug_level = DEFAULT_DEBUG_LEVEL_GLOBAL;
	mpsd_debug_level_driver = DEFAULT_DEBUG_LEVEL_DRIVER;
	mpsd_debug_level_read = DEFAULT_DEBUG_LEVEL_READ;
	mpsd_debug_level_timer = DEFAULT_DEBUG_LEVEL_TIMER;
	mpsd_debug_level_notifier = DEFAULT_DEBUG_LEVEL_NOTIFIER;

	debug_pid = DEFAULT_DEBUG_PID;
	debug_param = DEFAULT_DEBUG_PARAM;
	debug_param_level = DEFAULT_DEBUG_PARAM_LEVEL;

	debug_app_field = DEFAULT_DEBUG_APP_FIELD;
	debug_dev_field = DEFAULT_DEBUG_DEV_FIELD;

	debug_notifier_param = DEFAULT_DEBUG_PARAM;
	debug_notifier_reg_unreg_flag = DEFAULT_DEBUG_NOTIFIER_REG_FLAG;
	debug_notifier_events = DEFAULT_DEBUG_NOTIFIER_EVENTS;
	debug_notifier_delta = DEFAULT_DEBUG_NOTIFIER_DELTA;
	debug_notifier_min = DEFAULT_DEBUG_NOTIFIER_MIN;
	debug_notifier_max = DEFAULT_DEBUG_NOTIFIER_MAX;
	notifier_events_data_idx = DEFAULT_NOTIFIER_EVENTS_DATA_IDX;

	debug_timer_param = DEFAULT_DEBUG_PARAM;
	debug_timer_reg_unreg_flag = DEFAULT_DEBUG_TIMER_REG_FLAG;
	debug_timer_val = DEFAULT_DEBUG_TIMER_VAL;
	debug_timer_count = DEFAULT_DEBUG_TIMER_COUNT;
	timer_events_data_idx = DEFAULT_TIMER_EVENTS_DATA_IDX;

	use_snapshot = DEFAULT_USE_SNAPSHOT;
	detailed_info = DEFAULT_DETAILED_INFO;

	notifier_release = DEFAULT_NOTIFIER_REL;
	timer_release = DEFAULT_TIMER_REL;

	for (i = 0; i < MAX_EVENT_DATA_SIZE; i++) {
		notifier_events_data[i].time_stamp =
			DEFAULT_NOTIFIER_EVENTS_DATA;
		notifier_events_data[i].param =
			DEFAULT_NOTIFIER_EVENTS_DATA;
		notifier_events_data[i].events =
			DEFAULT_NOTIFIER_EVENTS_DATA;
		notifier_events_data[i].prev =
			DEFAULT_NOTIFIER_EVENTS_DATA;
		notifier_events_data[i].cur =
			DEFAULT_NOTIFIER_EVENTS_DATA;
		notifier_events_data[i].notifier_app_field =
			DEFAULT_NOTIFIER_EVENTS_DATA;
		notifier_events_data[i].notifier_dev_field =
			DEFAULT_NOTIFIER_EVENTS_DATA;

		timer_events_data[i].time_stamp =
			DEFAULT_TIMER_EVENTS_DATA;
		timer_events_data[i].value =
			DEFAULT_TIMER_EVENTS_DATA;
		timer_events_data[i].count =
			DEFAULT_TIMER_EVENTS_DATA;
		timer_events_data[i].timer_app_field =
			DEFAULT_TIMER_EVENTS_DATA;
		timer_events_data[i].timer_dev_field =
			DEFAULT_TIMER_EVENTS_DATA;
	}
}

/**
 * get_param_type - Get the parameter type.
 * @param: Param ID whose type need to be found.
 *
 * This function finds the type of the param associated
 * with a Param ID and returns the type.
 */
enum param_type get_param_type(int param)
{
	enum param_type type = -1;

	if (is_valid_param(param) == 0) {
		pr_err("%s: %s: Parameter is not Valid\n", tag, __func__);
		return type;
	}

	if (is_valid_app_param((enum app_param)param) == 1) {
		switch ((enum app_param)param) {
		case APP_PARAM_NAME:
			type = PARAM_TYPE_STR;
			break;
		case APP_PARAM_TGID:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_FLAGS:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_PRIORITY:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_NICE:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_CPUS_ALLOWED:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_VOLUNTARY_CTXT_SWITCHES:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_NONVOLUNTARY_CTXT_SWITCHES:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_NUM_THREADS:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_OOM_SCORE_ADJ:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_OOM_SCORE_ADJ_MIN:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_UTIME:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_STIME:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_MIN_PGFLT:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_MAJ_PGFLT:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_STARTTIME:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_VSIZE:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_DATA:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_STACK:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_TEXT:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_SWAP:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_SHARED:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_ANON:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_RSS:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_MEM_RSS_LIMIT:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_IO_RCHAR:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_IO_WCHAR:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_IO_SYSCR:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_IO_SYSCW:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_IO_SYSCFS:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_IO_BYTES_READ:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_IO_BYTES_WRITE:
			type = PARAM_TYPE_INT;
			break;
		case APP_PARAM_IO_BYTES_WRITE_CANCEL:
			type = PARAM_TYPE_INT;
			break;
		default:
			pr_err("%s: %s: Invalid param\n", tag, __func__);
			return type;
		}
	} else if (is_valid_dev_param((enum dev_param)param) == 1) {
		switch ((enum dev_param)param) {
		case DEV_PARAM_CPU_PRESENT:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_ONLINE:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_UTIL:
			type = PARAM_TYPE_INT_2D_ARR;
			break;
		case DEV_PARAM_CPU_FREQ_AVAIL:
			type = PARAM_TYPE_INT_2D_ARR;
			break;
		case DEV_PARAM_CPU_MAX_LIMIT:
			type = PARAM_TYPE_INT_ARR;
			break;
		case DEV_PARAM_CPU_MIN_LIMIT:
			type = PARAM_TYPE_INT_ARR;
			break;
		case DEV_PARAM_CPU_CUR_FREQ:
			type = PARAM_TYPE_INT_ARR;
			break;
		case DEV_PARAM_CPU_FREQ_TIME_STATS:
			type = PARAM_TYPE_CPU_FREQ_TIME_STATS;
			break;
		case DEV_PARAM_CPU_FREQ_GOV:
			type = PARAM_TYPE_CPU_FREQ_GOV;
			break;
		case DEV_PARAM_CPU_LOADAVG_1MIN:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_LOADAVG_5MIN:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_LOADAVG_15MIN:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_SYSTEM_UPTIME:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_TIME_USER:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_TIME_NICE:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_TIME_SYSTEM:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_TIME_IDLE:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_TIME_IO_WAIT:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_TIME_IRQ:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_CPU_TIME_SOFT_IRQ:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_TOTAL:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_FREE:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_AVAIL:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_BUFFER:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_CACHED:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_SWAP_CACHED:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_SWAP_TOTAL:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_SWAP_FREE:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_DIRTY:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_ANON_PAGES:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_MAPPED:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_SHMEM:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_SYSHEAP:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_SYSHEAP_POOL:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_VMALLOC_API:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_KGSL_SHARED:
			type = PARAM_TYPE_INT;
			break;
		case DEV_PARAM_MEM_ZSWAP:
			type = PARAM_TYPE_INT;
			break;
		default:
			pr_err("%s: %s: Invalid param\n", tag, __func__);
			return type;
		}
	} else {
		pr_err("%s: %s: Parameter is not Valid\n", tag, __func__);
		return type;
	}

	return type;
}

/**
 * get_param_name - Get the parameter name from Param ID.
 * @param: Param ID whose name need to be found.
 * @name: String in which name is to be copied and returned.
 *
 * This function finds the name of the param associated with a Param ID.
 */
static void get_param_name(int param, char name[])
{
	const char *p_name = NULL;

	if (unlikely(!name)) {
		pr_err("%s: %s : name is null\n",  tag, __func__);
		return;
	}

	if (is_valid_param(param) == 0) {
		pr_err("%s: %s: Parameter is not Valid\n", tag, __func__);
		return;
	}

	memset(name, '\0', MAX_PARAM_NAME_LENGTH);
	if (is_valid_app_param((enum app_param)param) == 1)
		p_name = app_param_name[param];
	else if (is_valid_dev_param((enum dev_param)param) == 1)
		p_name = dev_param_name[param-DEV_PARAM_MIN];
	else{
		pr_err("%s: %s: Invalid param\n", tag, __func__);
		return;
	}

	strlcpy(name, p_name, MAX_PARAM_NAME_LENGTH);
}

/**
 * get_param_from_app_snapshot - Get the parameter value from app snapshot.
 * @param: Param ID of the parameter whose value need to be filled.
 * @app_param_snapshot: Snapshot structure for app specific param.
 * @value = Structure where read data need to be filled.
 *
 * This function gets the value of the param from app snapshot structure.
 */
static void get_param_from_app_snapshot(int param,
	struct app_params_struct *app_params,
	struct debug_param_val *value)
{
	if (unlikely(!value || !app_params ||
		(is_valid_app_param((enum app_param)param)) == 0)) {
		pr_err("%s: %s : Invalid Arguments\n",	tag, __func__);
		return;
	}

	switch (param) {
	case APP_PARAM_NAME:
		strlcpy(value->str_val, app_params->name, MAX_CHAR_BUF_SIZE);
		break;
	case APP_PARAM_TGID:
		value->int_val = app_params->tgid;
		break;
	case APP_PARAM_FLAGS:
		value->int_val = app_params->flags;
		break;
	case APP_PARAM_PRIORITY:
		value->int_val = app_params->priority;
		break;
	case APP_PARAM_NICE:
		value->int_val = app_params->nice;
		break;
	case APP_PARAM_CPUS_ALLOWED:
		value->int_val = app_params->cpus_allowed;
		break;
	case APP_PARAM_VOLUNTARY_CTXT_SWITCHES:
		value->int_val = app_params->voluntary_ctxt_switches;
		break;
	case APP_PARAM_NONVOLUNTARY_CTXT_SWITCHES:
		value->int_val = app_params->nonvoluntary_ctxt_switches;
		break;
	case APP_PARAM_NUM_THREADS:
		value->int_val = app_params->num_threads;
		break;
	case APP_PARAM_OOM_SCORE_ADJ:
		value->int_val = app_params->oom_score_adj;
		break;
	case APP_PARAM_OOM_SCORE_ADJ_MIN:
		value->int_val = app_params->oom_score_adj_min;
		break;
	case APP_PARAM_UTIME:
		value->int_val = app_params->utime;
		break;
	case APP_PARAM_STIME:
		value->int_val = app_params->stime;
		break;
	case APP_PARAM_MEM_MIN_PGFLT:
		value->int_val = app_params->mem_min_pgflt;
		break;
	case APP_PARAM_MEM_MAJ_PGFLT:
		value->int_val = app_params->mem_maj_pgflt;
		break;
	case APP_PARAM_STARTTIME:
		value->int_val = app_params->start_time;
		break;
	case APP_PARAM_MEM_VSIZE:
		value->int_val = app_params->mem_vsize;
		break;
	case APP_PARAM_MEM_DATA:
		value->int_val = app_params->mem_data;
		break;
	case APP_PARAM_MEM_STACK:
		value->int_val = app_params->mem_stack;
		break;
	case APP_PARAM_MEM_TEXT:
		value->int_val = app_params->mem_text;
		break;
	case APP_PARAM_MEM_SWAP:
		value->int_val = app_params->mem_swap;
		break;
	case APP_PARAM_MEM_SHARED:
		value->int_val = app_params->mem_shared;
		break;
	case APP_PARAM_MEM_ANON:
		value->int_val = app_params->mem_anon;
		break;
	case APP_PARAM_MEM_RSS:
		value->int_val = app_params->mem_rss;
		break;
	case APP_PARAM_MEM_RSS_LIMIT:
		value->int_val = app_params->mem_rss_limit;
		break;
	case APP_PARAM_IO_RCHAR:
		value->int_val = app_params->io_rchar;
		break;
	case APP_PARAM_IO_WCHAR:
		value->int_val = app_params->io_wchar;
		break;
	case APP_PARAM_IO_SYSCR:
		value->int_val = app_params->io_syscr;
		break;
	case APP_PARAM_IO_SYSCW:
		value->int_val = app_params->io_syscw;
		break;
	case APP_PARAM_IO_SYSCFS:
		value->int_val = app_params->io_syscfs;
		break;
	case APP_PARAM_IO_BYTES_READ:
		value->int_val = app_params->io_bytes_read;
		break;
	case APP_PARAM_IO_BYTES_WRITE:
		value->int_val = app_params->io_bytes_write;
		break;
	case APP_PARAM_IO_BYTES_WRITE_CANCEL:
		value->int_val = app_params->io_bytes_write_cancel;
		break;
	default:
		pr_err("%s: %s: Parameter is not valid\n", tag, __func__);
		return;
	}
}

/**
 * get_param_from_dev_snapshot - Get the parameter value from dev snapshot.
 * @param: Param ID of the parameter whose value need to be filled.
 * @dev_param_snapshot: Snapshot structure for dev specific param.
 * @value = Structure where read data need to be filled.
 *
 * This function gets the value of the param from dev snapshot structure.
 */
static void get_param_from_dev_snapshot(int param,
	struct dev_params_struct *dev_params,
	struct debug_param_val *value)
{
	if (unlikely(!value || !dev_params ||
		(is_valid_dev_param((enum dev_param)param) == 0))) {
		pr_err("%s: %s : Invalid Arguments\n",	tag, __func__);
		return;
	}

	switch (param) {
	case DEV_PARAM_CPU_PRESENT:
		value->int_val = dev_params->cpu_present;
		break;
	case DEV_PARAM_CPU_ONLINE:
		value->int_val = dev_params->cpu_online;
		break;
	case DEV_PARAM_CPU_UTIL:
		memcpy(&value->int_2d_arr_cpuload_val, &dev_params->cpu_util,
			sizeof(dev_params->cpu_util));
		break;
	case DEV_PARAM_CPU_FREQ_AVAIL:
		memcpy(&value->int_2d_arr_val,
			&dev_params->cpu_freq_avail,
			sizeof(dev_params->cpu_freq_avail));
		break;
	case DEV_PARAM_CPU_MAX_LIMIT:
		memcpy(&value->int_arr_val, &dev_params->cpu_max_limit,
			sizeof(dev_params->cpu_max_limit));
		break;
	case DEV_PARAM_CPU_MIN_LIMIT:
		memcpy(&value->int_arr_val, &dev_params->cpu_min_limit,
			sizeof(dev_params->cpu_min_limit));
		break;
	case DEV_PARAM_CPU_CUR_FREQ:
		memcpy(&value->int_arr_val, &dev_params->cpu_cur_freq,
			sizeof(dev_params->cpu_cur_freq));
		break;
	case DEV_PARAM_CPU_FREQ_TIME_STATS:
		memcpy(&value->cpu_freq_time_stats,
			&dev_params->cpu_freq_time_stats,
			sizeof(dev_params->cpu_freq_time_stats));
		break;
	case DEV_PARAM_CPU_FREQ_GOV:
		memcpy(&value->cpu_freq_gov_val,
			&dev_params->cpu_freq_gov,
			sizeof(dev_params->cpu_freq_gov));
		break;
	case DEV_PARAM_CPU_LOADAVG_1MIN:
		value->int_val = dev_params->cpu_loadavg_1min;
		break;
	case DEV_PARAM_CPU_LOADAVG_5MIN:
		value->int_val = dev_params->cpu_loadavg_5min;
		break;
	case DEV_PARAM_CPU_LOADAVG_15MIN:
		value->int_val = dev_params->cpu_loadavg_15min;
		break;
	case DEV_PARAM_SYSTEM_UPTIME:
		value->int_val = dev_params->system_uptime;
		break;
	case DEV_PARAM_CPU_TIME_USER:
		value->int_val = dev_params->cpu_time_user;
		break;
	case DEV_PARAM_CPU_TIME_NICE:
		value->int_val = dev_params->cpu_time_nice;
		break;
	case DEV_PARAM_CPU_TIME_SYSTEM:
		value->int_val = dev_params->cpu_time_system;
		break;
	case DEV_PARAM_CPU_TIME_IDLE:
		value->int_val = dev_params->cpu_time_idle;
		break;
	case DEV_PARAM_CPU_TIME_IO_WAIT:
		value->int_val = dev_params->cpu_time_io_Wait;
		break;
	case DEV_PARAM_CPU_TIME_IRQ:
		value->int_val = dev_params->cpu_time_irq;
		break;
	case DEV_PARAM_CPU_TIME_SOFT_IRQ:
		value->int_val = dev_params->cpu_time_soft_irq;
		break;
	case DEV_PARAM_MEM_TOTAL:
		value->int_val = dev_params->mem_total;
		break;
	case DEV_PARAM_MEM_FREE:
		value->int_val = dev_params->mem_free;
		break;
	case DEV_PARAM_MEM_AVAIL:
		value->int_val = dev_params->mem_avail;
		break;
	case DEV_PARAM_MEM_BUFFER:
		value->int_val = dev_params->mem_buffer;
		break;
	case DEV_PARAM_MEM_CACHED:
		value->int_val = dev_params->mem_cached;
		break;
	case DEV_PARAM_MEM_SWAP_CACHED:
		value->int_val = dev_params->mem_swap_cached;
		break;
	case DEV_PARAM_MEM_SWAP_TOTAL:
		value->int_val = dev_params->mem_swap_total;
		break;
	case DEV_PARAM_MEM_SWAP_FREE:
		value->int_val = dev_params->mem_swap_free;
		break;
	case DEV_PARAM_MEM_DIRTY:
		value->int_val = dev_params->mem_dirty;
		break;
	case DEV_PARAM_MEM_ANON_PAGES:
		value->int_val = dev_params->mem_anon_pages;
		break;
	case DEV_PARAM_MEM_MAPPED:
		value->int_val = dev_params->mem_mapped;
		break;
	case DEV_PARAM_MEM_SHMEM:
		value->int_val = dev_params->mem_shmem;
		break;
	case DEV_PARAM_MEM_SYSHEAP:
		value->int_val = dev_params->mem_sysheap;
		break;
	case DEV_PARAM_MEM_SYSHEAP_POOL:
		value->int_val = dev_params->mem_sysheap_pool;
		break;
	case DEV_PARAM_MEM_VMALLOC_API:
		value->int_val = dev_params->mem_vmalloc_api;
		break;
	case DEV_PARAM_MEM_KGSL_SHARED:
		value->int_val = dev_params->mem_kgsl_shared;
		break;
	case DEV_PARAM_MEM_ZSWAP:
		value->int_val = dev_params->mem_zswap;
		break;
	default:
		pr_err("%s: %s: Parameter is not valid\n", tag, __func__);
		return;
	}
}

/**
 * get_param_data -Get the parameter data from mpsd sync read framework.
 * @param: Param ID of the parameter whose value need to be read.
 * @data: Structure where read data need to be filled.
 *
 * This function gets the data of the param from mpsd sync read framework.
 */
static void get_param_data(int param, struct param_val *data)
{
	if (unlikely(!data)) {
		pr_err("%s: %s : data is null\n",  tag, __func__);
		return;
	}

	if (is_valid_app_param((enum app_param)param) == 1) {
		if (use_snapshot == 0) {
			mpsd_get_app_params(debug_pid, param,
				&data->app_param_val);
		} else if (use_snapshot == 1 || use_snapshot == 2) {
			get_app_params_snapshot(debug_pid,
				&data->app_param_val);
		}
	} else if (is_valid_dev_param(param) == 1) {
		if (use_snapshot == 0) {
			mpsd_get_dev_params(param,
				&data->dev_param_val);
		} else if (use_snapshot == 1 || use_snapshot == 2) {
			get_dev_params_snapshot(&data->dev_param_val);
		}
	}
}

/**
 * get_param_val_from_data -Get the param value from read param data.
 * @param: Param ID of the parameter whose value need to be read.
 * @data: Structure where value of the param need to be read from.
 * @value: Structure where read value need to be filled.
 *
 * This function gets the value of the param from read data based on
 * its param type.
 */
static void get_param_val_from_data(int param, struct param_val *data,
	struct debug_param_val *value)
{
	if (unlikely(!data)) {
		pr_err("%s: %s : data is null\n",  tag, __func__);
		return;
	}

	if (unlikely(!value)) {
		pr_err("%s: %s : value is null\n",	tag, __func__);
		return;
	}

	if (is_valid_param(param) == 0) {
		pr_err("%s: %s: Parameter is not valid\n", tag, __func__);
		return;
	}

	memset(value, 0, sizeof(struct debug_param_val));
	if (is_valid_app_param((enum app_param)param) == 1) {
		get_param_from_app_snapshot(param,
			&data->app_param_val, value);
	} else if (is_valid_dev_param(param) == 1) {
		get_param_from_dev_snapshot(param,
			&data->dev_param_val, value);
	}
}

/**
 * print_param_data -Print the value of param in a character buffer
 * @param: Param ID of the parameter whose value need to be read.
 * @value: Structure from where value need to be printed.
 * @buf: Character buffer where value need to be printed.
 *
 * This function prints the param value based n its type in a character buffer.
 * This functions returns the number of characters wrriten.
 */
static int print_param_data(int param, struct debug_param_val *value, char *buf)
{
	int i = 0, j = 0;
	int len = 0;
	int type = get_param_type(param);
	int poliy_map[MAX_INT_ARR_SIZE];
	char name[MAX_PARAM_NAME_LENGTH];
	char str_tag[MAX_PARAM_NAME_LENGTH];

	if (unlikely(!value)) {
		pr_err("%s: %s : value is null\n",	tag, __func__);
		return len;
	}

	if (unlikely(!buf)) {
		pr_err("%s: %s : buf is null\n",  tag, __func__);
		return len;
	}

	if (is_valid_param(param) == 0) {
		pr_err("%s: %s: Parameter is not valid\n", tag, __func__);
		return len;
	}

	memset(name, '\0', MAX_PARAM_NAME_LENGTH);
	memset(str_tag, '\0', MAX_PARAM_NAME_LENGTH);

	if (detailed_info == 1) {
		get_param_name(param, name);
		snprintf(str_tag, MAX_PARAM_NAME_LENGTH, "ID:%d-[%s] => ", param, name);
	} else {
		snprintf(str_tag, MAX_PARAM_NAME_LENGTH, "ID:%d => ", param);
	}

	get_cpufreq_policy_map(poliy_map);
	memset(buf, '\0', MAX_PRINT_SIZE);

	if (type == PARAM_TYPE_INT) {
		len = snprintf(buf, MAX_PRINT_SIZE, "%s%lld", str_tag, value->int_val);
	} else if (type == PARAM_TYPE_STR) {
		len = snprintf(buf, MAX_PRINT_SIZE, "%s%s", str_tag, value->str_val);
	} else if (type == PARAM_TYPE_INT_ARR) {
		len = snprintf(buf + len, MAX_PRINT_SIZE, "%s", str_tag);
		for (i = 0; i < MAX_INT_ARR_SIZE; i++) {
			if (poliy_map[i] == 0 && param != DEV_PARAM_CPU_UTIL) {
				if (detailed_info == 0 || detailed_info == 1)
					continue;
			}

			len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ",
				value->int_arr_val[i]);
		}
	} else if (type == PARAM_TYPE_INT_2D_ARR) {
		int int_2d_arr_start = 0;
		int int_2d_arr_len = 0;
		long long read_val = 0;

		len = snprintf(buf + len, MAX_PRINT_SIZE, "%s", str_tag);
		for (i = 0; i < MAX_INT_ARR_SIZE; i++) {
			if (poliy_map[i] == 0 && param != DEV_PARAM_CPU_UTIL) {
				if (detailed_info == 0 || detailed_info == 1)
					continue;
			}

			if (param == DEV_PARAM_CPU_UTIL) {
				if (MAX_LOAD_HISTORY > MAX_CPULOAD_PRINT_LIMIT) {
					int_2d_arr_start = MAX_LOAD_HISTORY -
						MAX_CPULOAD_PRINT_LIMIT;
				}

				int_2d_arr_len = MAX_LOAD_HISTORY;
			} else if (param == DEV_PARAM_CPU_FREQ_AVAIL) {
				int_2d_arr_len = MAX_FREQ_LIST_SIZE;
			}

			for (j = int_2d_arr_start; j < int_2d_arr_len; j++) {
				if (param == DEV_PARAM_CPU_UTIL)
					read_val = value->int_2d_arr_cpuload_val[i][j];
				else if (param == DEV_PARAM_CPU_FREQ_AVAIL)
					read_val = value->int_2d_arr_val[i][j];
				if (read_val == DEFAULT_PARAM_VAL_INT) {
					if (detailed_info == 0
						|| detailed_info == 1) {
						continue;
					}
				}

				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", read_val);
			}
			len += snprintf(buf + len, MAX_PRINT_SIZE, "| ");
		}
	} else if (type == PARAM_TYPE_CPU_FREQ_TIME_STATS) {
		len = snprintf(buf + len, MAX_PRINT_SIZE, "%s", str_tag);
		for (i = 0; i < MAX_INT_ARR_SIZE; i++) {
			if (poliy_map[i] == 0) {
				if (detailed_info == 0 || detailed_info == 1)
					continue;
			}

			for (j = 0; j < MAX_FREQ_LIST_SIZE; j++) {
				if (value->cpu_freq_time_stats[i][j].freq ==
					DEFAULT_PARAM_VAL_INT) {
					if (detailed_info == 0
						|| detailed_info == 1) {
						continue;
					}
				}

				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld %lld, ",
					value->cpu_freq_time_stats[i][j].freq,
					value->cpu_freq_time_stats[i][j].time);
			}

			len += snprintf(buf + len, MAX_PRINT_SIZE, "| ");
		}
	} else if (type == PARAM_TYPE_STR_ARR) {
		len = snprintf(buf + len, MAX_PRINT_SIZE, "%s", str_tag);
		for (i = 0; i < MAX_INT_ARR_SIZE; i++) {
			if (poliy_map[i] == 0) {
				if (detailed_info == 0 || detailed_info == 1)
					continue;
			}

			len += snprintf(buf + len, MAX_PRINT_SIZE, "%s | ",
				value->str_arr_val[i]);
		}
	} else if (type == PARAM_TYPE_CPU_FREQ_GOV) {
		len = snprintf(buf + len, MAX_PRINT_SIZE, "%s", str_tag);
		for (i = 0; i < MAX_INT_ARR_SIZE; i++) {
			char gov[MAX_CHAR_BUF_SIZE];
			struct cpufreq_policy policy;
			union cpufreq_gov_data *gov_data;

			if (cpufreq_get_policy(&policy, i) != 0) {
				if (detailed_info == 0 || detailed_info == 1)
					continue;

				strlcpy(gov, CPUFREQ_GOV_NONE, MAX_CHAR_BUF_SIZE);
			}

			strlcpy(gov, value->cpu_freq_gov_val[i].gov, MAX_CHAR_BUF_SIZE);
			len += snprintf(buf + len, MAX_PRINT_SIZE, "[%s]=> ", gov);

			gov_data = &value->cpu_freq_gov_val[i]
				.cpu_freq_gov_data;
			if (strcmp(gov, CPUFREQ_GOV_PERFORMANCE) == 0) {
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_performance.cur_freq);
			} else if (strcmp(gov, CPUFREQ_GOV_POWERSAVE) == 0) {
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_powersave.cur_freq);
			} else if (strcmp(gov, CPUFREQ_GOV_USERSPACE) == 0) {
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_userspace.set_speed);
			} else if (strcmp(gov, CPUFREQ_GOV_ONDEMAND) == 0) {
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_ondemand.min_sampling_rate);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_ondemand.ignore_nice_load);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_ondemand.sampling_rate);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_ondemand.sampling_down_factor);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_ondemand.up_threshold);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_ondemand.io_is_busy);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_ondemand.powersave_bias);
			} else if (strcmp(gov, CPUFREQ_GOV_CONSERVATIVE) == 0) {
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_conservative.min_sampling_rate);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_conservative.ignore_nice_load);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_conservative.sampling_rate);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_conservative.sampling_down_factor);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_conservative.up_threshold);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_conservative.down_threshold);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_conservative.freq_step);
			} else if (strcmp(gov, CPUFREQ_GOV_INTERACTIVE) == 0
				|| strcmp(gov, CPUFREQ_GOV_NONE) == 0) {
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.hispeed_freq);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.go_hispeed_load);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.target_loads);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.min_sample_time);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.timer_rate);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.above_hispeed_delay);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.boost);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.boostpulse_duration);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.timer_slack);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.io_is_busy);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.use_sched_load);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.use_migration_notif);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.align_windows);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.max_freq_hysteresis);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.ignore_hispeed_on_notif);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.fast_ramp_down);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_interactive.enable_prediction);
			} else if (strcmp(gov, CPUFREQ_GOV_SCHEDUTIL) == 0) {
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_schedutil.up_rate_limit_us);
				len += snprintf(buf + len, MAX_PRINT_SIZE, "%lld ", gov_data
					->gov_schedutil.down_rate_limit_us);
			}

			len += snprintf(buf + len, MAX_PRINT_SIZE, "| ");
		}
	} else {
		len = snprintf(buf, MAX_PRINT_SIZE, "%sERROR: Wrong type=%d\n", tag, type);
	}

	return len;
}

/**
 * print_mmap_buffer -Prints the containts of whole shared memory area.
 *
 * This function prints the the containts of whole shared memory area.
 */
int print_mmap_buffer(void)
{
	int i = 0;
	int ret = 0;
	char *print_data;
	struct memory_area *mmap_buf = get_device_mmap();
	struct debug_param_val *value;
	struct param_val *data;

	print_data = kmalloc(BUFFER_LENGTH_MAX, GFP_ATOMIC);
	if (!print_data) {
		ret = -ENOMEM;
		goto ret_back;
	}

	data = kmalloc(sizeof(struct param_val), GFP_ATOMIC);
	if (!data) {
		ret = -ENOMEM;
		goto free_print_data;
	}

	value = kmalloc(sizeof(struct debug_param_val), GFP_ATOMIC);
	if (!value) {
		ret = -ENOMEM;
		goto free_data;
	}

	if (!mmap_buf) {
		ret = -EPERM;
		goto free_value;
	}

	pr_info("%s: %s: Masks[%llu, %llu] ReqID=%d E[%d]\n", tag, __func__,
		mmap_buf->app_field_mask, mmap_buf->dev_field_mask,
		(int)mmap_buf->req, mmap_buf->event);

	memset_app_params(&data->app_param_val);
	memset_dev_params(&data->dev_param_val);

	memcpy(&data->app_param_val, &mmap_buf->app_params,
		sizeof(mmap_buf->app_params));
	memcpy(&data->dev_param_val, &mmap_buf->dev_params,
		sizeof(mmap_buf->dev_params));

	for (i = APP_PARAM_MIN; i < APP_PARAM_MAX; i++) {
		get_param_val_from_data(i, data, value);
		print_param_data(i, value, print_data);
		pr_info("%s: %s: %s\n", tag, __func__, print_data);
	}

	for (i = DEV_PARAM_MIN; i < DEV_PARAM_MAX; i++) {
		get_param_val_from_data(i, data, value);
		print_param_data(i, value, print_data);
		pr_info("%s: %s: %s\n", tag, __func__, print_data);
	}

free_value:
	kfree(value);
free_data:
	kfree(data);
free_print_data:
	kfree(print_data);
ret_back:
	return ret;
}

/**
 * print_notifier_registration_data -Prints the data of all params registration.
 *
 * This function prints the notifier registration data.
 */
void print_notifier_registration_data(void)
{
	int i = 0;
	struct param_info_notifier *app = get_notifier_app_info_arr();
	struct param_info_notifier *dev = get_notifier_dev_info_arr();

	if (!app || !dev) {
		pr_err("%s: %s: Debug level lower than required\n",
			tag, __func__);
		return;
	}

	for (i = 0; i < TOTAL_APP_PARAMS; i++) {
		pr_info("%s: %s: ID:%d E[%d] (%lld %lld %lld)\n",
			tag, __func__, app[i].param, app[i].events,
			app[i].delta, app[i].max_threshold,
			app[i].min_threshold);
	}

	for (i = 0; i < TOTAL_DEV_PARAMS; i++) {
		pr_info("%s: %s: ID:%d E[%d] (%lld %lld %lld)\n",
			tag, __func__, dev[i].param, dev[i].events,
			dev[i].delta, dev[i].max_threshold,
			dev[i].min_threshold);
	}
}

/**
 * print_notifier_event_data -Prints the data of the recent events.
 * @data: notifier event data got from notifier framework.
 *
 * This function prints the data of last few recent events.
 */
void print_notifier_events_data(struct param_data_notifier *data)
{
	int i = 0;
	long long time = ktime_to_ms(ktime_get());

	if (unlikely(!data)) {
		pr_err("%s: %s : data is null\n",  tag, __func__);
		return;
	}

	if (notifier_events_data_idx >= MAX_EVENT_DATA_SIZE)
		notifier_events_data_idx = 0;

	pr_info("%s: %s: CUR[%lld]=> ID:%d E[%d] PREV=%lld CUR=%lld MASK[%llu, %llu]\n",
		tag, __func__, time, data->param,
		data->events, data->prev, data->cur,
		get_notifier_app_param_mask(),
		get_notifier_dev_param_mask());

	notifier_events_data[notifier_events_data_idx].time_stamp = time;
	notifier_events_data[notifier_events_data_idx].param = data->param;
	notifier_events_data[notifier_events_data_idx].events = data->events;
	notifier_events_data[notifier_events_data_idx].prev = data->prev;
	notifier_events_data[notifier_events_data_idx].cur = data->cur;
	notifier_events_data[notifier_events_data_idx].notifier_app_field =
		get_notifier_app_param_mask();
	notifier_events_data[notifier_events_data_idx].notifier_dev_field =
		get_notifier_dev_param_mask();
	notifier_events_data_idx++;

	pr_info("%s: %s: MPSD_NOTIFIER_EVENT_DATA==>\n",  tag, __func__);
	for (i = 0; i < MAX_EVENT_DATA_SIZE; i++) {
		pr_info("%s: %s: [%lld]=> ID:%d E[%d] PREV=%lld CUR=%lld MASKS[%lld %lld]\n",
			tag, __func__,
			notifier_events_data[i].time_stamp,
			notifier_events_data[i].param,
			notifier_events_data[i].events,
			notifier_events_data[i].prev,
			notifier_events_data[i].cur,
			notifier_events_data[i].notifier_app_field,
			notifier_events_data[i].notifier_dev_field);
	}
}

/**
 * print_timer_events_data - Prints the data of the recent timer events.
 * @value: For which timer the event came.
 * @count: How many params reg for this timer value.
 * @app_field_mask: Bit field for which param timer came.
 * @dev_field_mask: Bit field for which param timer came.
 *
 * This function prints the data of the last few recent events.
 */
void print_timer_events_data(long long value, int count, unsigned long long app_field,
	unsigned long long dev_field)
{
	int i = 0;
	long long time = ktime_to_ms(ktime_get());

	pr_info("%s: %s: CUR[%lld]=> VAL[%lld] COUNT[%d] MASKS[%llu %llu]\n",
		tag, __func__, time, value, count, app_field, dev_field);

	if (timer_events_data_idx >= MAX_EVENT_DATA_SIZE)
		timer_events_data_idx = 0;

	timer_events_data[timer_events_data_idx].time_stamp = time;
	timer_events_data[timer_events_data_idx].value = value;
	timer_events_data[timer_events_data_idx].count = count;
	timer_events_data[timer_events_data_idx].timer_app_field = app_field;
	timer_events_data[timer_events_data_idx].timer_dev_field =	dev_field;
	timer_events_data_idx++;

	pr_info("%s: %s: MPSD_TIMER_EVENT_DATA :-\n", tag, __func__);
	for (i = 0; i < MAX_EVENT_DATA_SIZE; i++) {
		pr_info("%s: %s: [%lld]=> VAL[%lld] COUNT[%d] MASKS[%lld %lld]\n",
			tag, __func__,
			timer_events_data[i].time_stamp,
			timer_events_data[i].value,
			timer_events_data[i].count,
			timer_events_data[i].timer_app_field,
			timer_events_data[i].timer_dev_field);
	}
}

static ssize_t mpsd_enable_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "MPSD_ENABLE_FLAG=%u\n", get_mpsd_flag());
}

static ssize_t mpsd_enable_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	int val = 0;
	int ret;

	if (kstrtoint(buf, 0, &val) < 0)
		return -EINVAL;

	if (val != 1)
		val = 0;

	debug_set_mpsd_flag(val);
	ret = debug_create_mmap_area();
	if (ret) {
		pr_err("%s: mmap create failed\n", __func__);
		return -ENOMEM;
	}
	return n;
}

static ssize_t mpsd_behavior_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "MPSD_BEHAVIOR=%u\n", get_behavior());
}

static ssize_t mpsd_behavior_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	int val = 0;

	if (kstrtoint(buf, 0, &val) < 0)
		return -EINVAL;

	if (val != 1)
		val = 0;

	debug_set_mpsd_behavior(val);
	return n;
}

static ssize_t mpsd_debug_level_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "MPSD_DEBUG_LEVEL=%u\n", get_mpsd_debug_level());
}

static ssize_t mpsd_debug_level_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	int val = 0;

	if (kstrtoint(buf, 0, &val) < 0)
		return -EINVAL;

	if (val < DEBUG_LEVEL_MIN || val > DEBUG_LEVEL_MAX)
		val = 0;

	set_mpsd_debug_level(val);

	return n;
}

static ssize_t mpsd_debug_level_all_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "MPSD_DEBUG_LEVELS_ALL=> Global[%u] Driver[%u] Read[%u] Timer[%u] Notifer[%u]\n",
		get_mpsd_debug_level(),
		get_debug_level_driver(), get_debug_level_read(),
		get_debug_level_timer(), get_debug_level_notifier());
}

static ssize_t mpsd_debug_level_all_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	int global = 0, driver = 0, read = 0, timer = 0, notifier = 0;

	if (sscanf(buf, "%d %d %d %d %d", &global, &driver, &read, &timer,
		&notifier) != 5) {
		return -EINVAL;
	}

	if (global < DEBUG_LEVEL_MIN || global > DEBUG_LEVEL_MAX)
		global = 0;

	if (driver < DEBUG_LEVEL_MIN || driver > DEBUG_LEVEL_MAX)
		driver = 0;

	if (read < DEBUG_LEVEL_MIN || read > DEBUG_LEVEL_MAX)
		read = 0;

	if (timer < DEBUG_LEVEL_MIN || timer > DEBUG_LEVEL_MAX)
		timer = 0;

	if (notifier < DEBUG_LEVEL_MIN || notifier > DEBUG_LEVEL_MAX)
		notifier = 0;

	set_mpsd_debug_level(global);
	set_debug_level_driver(driver);
	set_debug_level_read(read);
	set_debug_level_timer(timer);
	set_debug_level_notifier(notifier);

	return n;
}

static ssize_t use_snapshot_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "USE_SNAPSHOT=%d\n", use_snapshot);
}

static ssize_t use_snapshot_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	if (kstrtoint(buf, 0, &use_snapshot) < 0)
		return -EINVAL;

	return n;
}

static ssize_t detailed_info_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "detailed_info=%d\n", detailed_info);
}

static ssize_t detailed_info_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	if (kstrtoint(buf, 0, &detailed_info) < 0)
		return -EINVAL;
	return n;
}

static ssize_t pid_show(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "PID = %d\n", debug_pid);
}

static ssize_t pid_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t n)
{
	if (kstrtoint(buf, 0, &debug_pid) < 0)
		return -EINVAL;

	if (!is_task_valid(debug_pid)) {
		pr_err("%s: %s: Process(%d) not found, setting default!\n",
			tag, __func__, debug_pid);
		debug_pid = DEFAULT_PID;
	}

	return n;
}

static ssize_t param_show(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	char name[MAX_PARAM_NAME_LENGTH];

	get_param_name(debug_param, name);
	return snprintf(buf, MAX_PRINT_SIZE, "ID=%d - NAME=%s\n", debug_param, name);
}

static ssize_t param_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t n)
{
	if (kstrtoint(buf, 0, &debug_param) < 0)
		return -EINVAL;

	if (is_valid_param(debug_param) == 0)
		return -EINVAL;

	return n;
}

static ssize_t param_list_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int i = 0;
	int ret = 0;
	char name[MAX_PARAM_NAME_LENGTH];

	for (i = APP_PARAM_MIN; i < APP_PARAM_MAX; i++) {
		get_param_name(i, name);
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "ID=%d - NAME=%s\n", i, name);
	}
	for (i = DEV_PARAM_MIN; i < DEV_PARAM_MAX; i++) {
		get_param_name(i, name);
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "ID=%d - NAME=%s\n", i, name);
	}

	return ret;
}

static ssize_t param_read_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int ret = 0;
	char *print_data;
	struct param_val *data;
	struct debug_param_val *value;

	print_data = kmalloc(MAX_PRINT_SIZE, GFP_KERNEL);
	if (unlikely(!print_data)) {
		pr_err("%s: %s : No Memory for print_data!\n",	tag, __func__);
		ret = -ENOMEM;
		goto ret_back;
	}

	data = kmalloc(sizeof(struct param_val), GFP_KERNEL);
	if (unlikely(!data)) {
		pr_err("%s: %s : No Memory for data!\n",  tag, __func__);
		ret = -ENOMEM;
		goto free_print_data;
	}
	memset_app_params(&data->app_param_val);
	memset_dev_params(&data->dev_param_val);

	value = kmalloc(sizeof(struct debug_param_val), GFP_KERNEL);
	if (unlikely(!value)) {
		pr_err("%s: %s : No Memory for value!\n",  tag, __func__);
		ret = -ENOMEM;
		goto free_data;
	}
	memset(value, 0, sizeof(struct debug_param_val));

	if ((is_valid_app_param(debug_param) == 1) &&
		(debug_pid == DEFAULT_PID)) {
		pr_err("%s: %s: Process(%d) not found\n", tag, __func__,
			debug_pid);
		ret = -ESRCH;
		goto free_value;
	}

	get_param_data(debug_param, data);
	get_param_val_from_data(debug_param, data, value);
	print_param_data(debug_param, value, print_data);
	ret = snprintf(buf, MAX_PRINT_SIZE, "%s\n", print_data);

free_value:
	kfree(value);
free_data:
	kfree(data);
free_print_data:
	kfree(print_data);
ret_back:
	return ret;
}

static ssize_t param_latency_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int ret = 0;
	char *print_data;
	struct timespec ts_b, ts_a, latency;
	struct param_val *data;
	struct debug_param_val *value;

	print_data = kmalloc(MAX_PRINT_SIZE, GFP_KERNEL);
	if (unlikely(!print_data)) {
		pr_err("%s: %s : No Memory for print_data!\n",	tag, __func__);
		ret = -ENOMEM;
		goto ret_back;
	}

	data = kmalloc(sizeof(struct param_val), GFP_KERNEL);
	if (unlikely(!data)) {
		pr_err("%s: %s : No Memory for data!\n",  tag, __func__);
		ret = -ENOMEM;
		goto free_print_data;
	}
	memset_app_params(&data->app_param_val);
	memset_dev_params(&data->dev_param_val);

	value = kmalloc(sizeof(struct debug_param_val), GFP_KERNEL);
	if (unlikely(!value)) {
		pr_err("%s: %s : No Memory for value!\n",  tag, __func__);
		ret = -ENOMEM;
		goto free_data;
	}
	memset(value, 0, sizeof(struct debug_param_val));

	if ((is_valid_app_param(debug_param) == 1) &&
		(debug_pid == DEFAULT_PID)) {
		pr_err("%s: %s: Process(%d) not found\n", tag, __func__,
			debug_pid);
		ret = -ESRCH;
		goto free_value;
	}

	getnstimeofday(&ts_b);
	get_param_data(debug_param, data);
	getnstimeofday(&ts_a);

	latency.tv_sec = ts_a.tv_sec - ts_b.tv_sec;
	latency.tv_nsec = ts_a.tv_nsec - ts_b.tv_nsec;

	get_param_val_from_data(debug_param, data, value);
	print_param_data(debug_param,  value, print_data);
	ret = snprintf(buf, MAX_PRINT_SIZE, "%s, Latency = %lld.%.9ld\n",
		print_data, (long long)latency.tv_sec, latency.tv_nsec);

free_value:
	kfree(value);
free_data:
	kfree(data);
free_print_data:
	kfree(print_data);
ret_back:
	return ret;
}

static ssize_t param_level_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "DEBUG_PARAM_LEVEL=%d\n", debug_param_level);
}

static ssize_t param_level_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	if (kstrtoint(buf, 0, &debug_param_level) < 0)
		return -EINVAL;

	if (debug_param_level != APP_LEVEL
		&& debug_param_level != DEV_LEVEL
		&& debug_param_level != ALL_LEVEL) {
		debug_param_level = DEV_LEVEL;
	}

	return n;
}

static ssize_t param_read_all_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int i = 0;
	int ret = 0;
	char *print_data;
	struct param_val *data;
	struct debug_param_val *value;

	print_data = kmalloc(MAX_PRINT_SIZE, GFP_KERNEL);
	if (unlikely(!print_data)) {
		pr_err("%s: %s : No Memory for print_data!\n",	tag, __func__);
		ret = -ENOMEM;
		goto ret_back;
	}

	data = kmalloc(sizeof(struct param_val), GFP_KERNEL);
	if (unlikely(!data)) {
		pr_err("%s: %s : No Memory for data!\n", tag, __func__);
		ret = -ENOMEM;
		goto free_print_data;
	}
	memset_app_params(&data->app_param_val);
	memset_dev_params(&data->dev_param_val);

	value = kmalloc(sizeof(struct debug_param_val), GFP_KERNEL);
	if (unlikely(!value)) {
		pr_err("%s: %s : No Memory for value!\n", tag, __func__);
		ret = -ENOMEM;
		goto free_data;
	}
	memset(value, 0, sizeof(struct debug_param_val));

	for (i = APP_PARAM_MIN; i < APP_PARAM_MAX; i++) {

		if (debug_pid == DEFAULT_PID) {
			pr_err("%s: %s: Process(%d) not found\n",
				tag, __func__, debug_pid);
			continue;
		}

		if (debug_param_level == DEV_LEVEL)
			continue;

		get_param_data(i, data);
		get_param_val_from_data(i, data, value);
		print_param_data(i, value, print_data);

		pr_info("%s: %s: %s\n", tag, __func__, print_data);
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "%s\n", print_data);
	}

	for (i = DEV_PARAM_MIN; i < DEV_PARAM_MAX; i++) {
		if (debug_param_level == APP_LEVEL)
			continue;

		get_param_data(i, data);
		get_param_val_from_data(i, data, value);
		print_param_data(i, value, print_data);

		pr_info("%s: %s: %s\n", tag, __func__, print_data);
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "%s\n", print_data);
	}

	kfree(value);
free_data:
	kfree(data);
free_print_data:
	kfree(print_data);
ret_back:
	return ret;
}

static ssize_t param_latency_all_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int i = 0;
	int ret = 0;
	char name[MAX_PARAM_NAME_LENGTH];
	struct timespec ts_b, ts_a, latency;
	struct param_val *data;
	struct debug_param_val *value;

	data = kmalloc(sizeof(struct param_val), GFP_KERNEL);
	if (unlikely(!data)) {
		pr_err("%s: %s : No Memory for data!\n",  tag, __func__);
		ret = -ENOMEM;
		goto ret_back;
	}
	memset_app_params(&data->app_param_val);
	memset_dev_params(&data->dev_param_val);

	value = kmalloc(sizeof(struct debug_param_val), GFP_KERNEL);
	if (unlikely(!value)) {
		pr_err("%s: %s : No Memory for value!\n",  tag, __func__);
		ret = -ENOMEM;
		goto free_data;
	}
	memset(value, 0, sizeof(struct debug_param_val));

	for (i = APP_PARAM_MIN; i < APP_PARAM_MAX; i++) {
		if (debug_pid == DEFAULT_PID) {
			pr_err("%s: %s: Process(%d) not found\n",
				tag, __func__, debug_pid);
			continue;
		}

		getnstimeofday(&ts_b);
		get_param_data(i, data);
		getnstimeofday(&ts_a);

		latency.tv_sec = ts_a.tv_sec - ts_b.tv_sec;
		latency.tv_nsec = ts_a.tv_nsec - ts_b.tv_nsec;

		get_param_name(i, name);
		if (detailed_info == 1)
			ret += snprintf(buf + ret, MAX_PRINT_SIZE, "ID:%d-[%s] => ", i, name);
		else
			ret += snprintf(buf + ret, MAX_PRINT_SIZE, "ID:%d => ", i);
		pr_info("%s: %s: ID:%d-[%s] => L[%lld.%.9ld]\n", tag,
			__func__, i, name, (long long)latency.tv_sec,
			latency.tv_nsec);
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "L[%lld.%.9ld]\n",
			(long long)latency.tv_sec, latency.tv_nsec);
	}

	for (i = DEV_PARAM_MIN; i < DEV_PARAM_MAX; i++) {
		if (debug_pid == DEFAULT_PID) {
			pr_err("%s: %s: Process(%d) not found\n",
				tag, __func__, debug_pid);
			continue;
		}

		getnstimeofday(&ts_b);
		get_param_data(i, data);
		getnstimeofday(&ts_a);

		latency.tv_sec = ts_a.tv_sec - ts_b.tv_sec;
		latency.tv_nsec = ts_a.tv_nsec - ts_b.tv_nsec;

		get_param_name(i, name);
		if (detailed_info == 1)
			ret += snprintf(buf + ret, MAX_PRINT_SIZE, "ID:%d-[%s] => ", i, name);
		else
			ret += snprintf(buf + ret, MAX_PRINT_SIZE, "ID:%d => ", i);
		pr_info("%s: %s: ID:%d-[%s] => L[%lld.%.9ld]\n", tag,
			__func__, i, name, (long long)latency.tv_sec,
			latency.tv_nsec);
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "L[%lld.%.9ld]\n",
			(long long)latency.tv_sec, latency.tv_nsec);
	}
	kfree(value);
free_data:
	kfree(data);
ret_back:
	return ret;
}

static ssize_t notifier_reg_data_all_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int i = 0, ret = 0;
	struct param_info_notifier *app;
	struct param_info_notifier *dev;

	if (get_notifier_init_status())
		mpsd_notifier_init();

	app = get_notifier_app_info_arr();
	dev = get_notifier_dev_info_arr();

	if (!app || !dev) {
		pr_err("%s: %s: Debug level lower than required\n",
			tag, __func__);
		return -EINVAL;
	}

	ret += snprintf(buf + ret, MAX_PRINT_SIZE, "\nNOTIFIER DATA:-\n");
	for (i = 0; i < TOTAL_APP_PARAMS; i++) {
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "ID:%d E[%d] (%lld %lld %lld)\n",
			app[i].param, app[i].events, app[i].delta,
			app[i].max_threshold, app[i].min_threshold);
	}
	ret += snprintf(buf + ret, MAX_PRINT_SIZE, "\n");

	for (i = 0; i < TOTAL_DEV_PARAMS; i++) {
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "ID:%d E[%d] (%lld %lld %lld)\n",
			dev[i].param, dev[i].events, dev[i].delta,
			dev[i].max_threshold, dev[i].min_threshold);
	}
	ret += snprintf(buf + ret, MAX_PRINT_SIZE, "\n");

	return ret;
}

static ssize_t notifier_param_reg_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "ID=%d F[%d] E[%d] (%lld %lld %lld)\n",
		debug_notifier_param, debug_notifier_reg_unreg_flag,
		debug_notifier_events, debug_notifier_delta,
		debug_notifier_max, debug_notifier_min);
}

static ssize_t notifier_param_reg_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	struct param_info_notifier data;

	if (sscanf(buf, "%d %d %d %lld %lld %lld",
		&debug_notifier_param, &debug_notifier_reg_unreg_flag,
		&debug_notifier_events, &debug_notifier_delta,
		&debug_notifier_max, &debug_notifier_min) != 6) {
		return -EINVAL;
	}

	data.param = debug_notifier_param;
	data.events = debug_notifier_events;
	data.delta = debug_notifier_delta;
	data.max_threshold = debug_notifier_max;
	data.min_threshold = debug_notifier_min;

	pr_info("%s: %s: DEBUG: ID=%d E[%d] (%lld %lld %lld)\n",
		tag, __func__, data.param, data.events, data.delta,
		data.max_threshold, data.min_threshold);

	if (get_notifier_init_status())
		mpsd_notifier_init();

	if (debug_notifier_reg_unreg_flag == 1) {
		pr_info("%s: %s: DEBUG: Register\n", tag, __func__);
		mpsd_register_for_event_notify(&data);
	} else if (debug_notifier_reg_unreg_flag == 0) {
		pr_info("%s: %s: DEBUG: Unregister\n", tag, __func__);
		mpsd_unregister_for_event_notify(&data);
	}

	return n;
}

static ssize_t notifier_events_data_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int i = 0, ret = 0;

	ret += snprintf(buf, MAX_PRINT_SIZE, "MPSD_NOTIFIER_EVENT_DATA:-\n");
	for (i = 0; i < MAX_EVENT_DATA_SIZE; i++) {
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "[%lld]=> ID:%d E[%d] PREV=%lld CUR=%lld MASKS[%lld %lld]\n",
			notifier_events_data[i].time_stamp,
			notifier_events_data[i].param,
			notifier_events_data[i].events,
			notifier_events_data[i].prev,
			notifier_events_data[i].cur,
			notifier_events_data[i].notifier_app_field,
			notifier_events_data[i].notifier_dev_field);
	}

	return ret;
}

static ssize_t notifier_release_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "NOTIFIER_RELEASE=%d\n", notifier_release);
}

static ssize_t notifier_release_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	if (kstrtoint(buf, 0, &notifier_release) < 0)
		return -EINVAL;

	if (notifier_release == 1) {
		pr_info("%s: %s: DEBUG: Called the notifier release\n",
			tag, __func__);
		mpsd_notifier_release();
	}

	return n;
}

static ssize_t timer_param_reg_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "ID=%d FLAG[%d] VAL[%lld]\n", debug_timer_param,
		debug_timer_reg_unreg_flag, debug_timer_val);
}

static ssize_t timer_param_reg_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	int ret;

	if (sscanf(buf, "%d %d %lld", &debug_timer_param,
		&debug_timer_reg_unreg_flag, &debug_timer_val) != 3) {
		return -EINVAL;
	}

	pr_info("%s: %s: DEBUG: ID=%d FLAG[%d] VAL[%lld]\n", tag, __func__,
		debug_timer_param, debug_timer_reg_unreg_flag,
		debug_timer_val);

	if (debug_timer_reg_unreg_flag == 1) {
		ret = mpsd_timer_register(debug_timer_param, debug_timer_val);
		if (ret)
			return ret;
	} else if (debug_timer_reg_unreg_flag == 0) {
		mpsd_timer_deregister(debug_timer_param);
	}

	return n;
}

static ssize_t timer_reg_data_all_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int ret = 0;

	ret = print_timer_reg_data(1, buf);
	return ret;
}

static ssize_t timer_events_data_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	int i = 0, ret = 0;

	ret += snprintf(buf, MAX_PRINT_SIZE, "MPSD_TIMER_EVENT_DATA:-\n");
	for (i = 0; i < MAX_EVENT_DATA_SIZE; i++) {
		ret += snprintf(buf + ret, MAX_PRINT_SIZE, "[%lld]=> VAL[%lld] COUNT[%d] MASKS[%lld %lld]\n",
			timer_events_data[i].time_stamp,
			timer_events_data[i].value,
			timer_events_data[i].count,
			timer_events_data[i].timer_app_field,
			timer_events_data[i].timer_dev_field);
	}

	return ret;
}

static ssize_t timer_release_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "TIMER_RELEASE=%d\n", timer_release);
}

static ssize_t timer_release_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	if (kstrtoint(buf, 0, &timer_release) < 0)
		return -EINVAL;

	if (timer_release == 1) {
		pr_info("%s: %s: DEBUG: Called the Timer release\n",
			tag, __func__);
		mpsd_timer_deregister_all();
	}

	return n;
}

static ssize_t debug_init_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_PRINT_SIZE, "!WRITE ONLY!\n");
}

static ssize_t debug_init_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	int val = 0;

	if (kstrtoint(buf, 0, &val) < 0)
		return -EINVAL;

	if (val == 1) {
		pr_info("%s: %s: DEBUG: Init debug FW\n", tag, __func__);
		init_debug_framework();
	}

	return n;
}

mpsd_attr_rw(mpsd_enable);
mpsd_attr_rw(mpsd_behavior);
mpsd_attr_rw(mpsd_debug_level);
mpsd_attr_rw(mpsd_debug_level_all);
mpsd_attr_rw(use_snapshot);
mpsd_attr_rw(detailed_info);
mpsd_attr_rw(pid);
mpsd_attr_rw(param);
mpsd_attr_ro(param_list);
mpsd_attr_ro(param_read);
mpsd_attr_ro(param_latency);
mpsd_attr_rw(param_level);
mpsd_attr_ro(param_read_all);
mpsd_attr_ro(param_latency_all);
mpsd_attr_rw(notifier_param_reg);
mpsd_attr_ro(notifier_reg_data_all);
mpsd_attr_ro(notifier_events_data);
mpsd_attr_rw(notifier_release);
mpsd_attr_rw(timer_param_reg);
mpsd_attr_ro(timer_reg_data_all);
mpsd_attr_ro(timer_events_data);
mpsd_attr_rw(timer_release);
mpsd_attr_rw(debug_init);

static struct attribute *mpsd_debug_g[] = {
	&mpsd_enable_attr.attr,
	&mpsd_behavior_attr.attr,
	&mpsd_debug_level_attr.attr,
	&mpsd_debug_level_all_attr.attr,
	&use_snapshot_attr.attr,
	&detailed_info_attr.attr,
	&pid_attr.attr,
	&param_list_attr.attr,
	&param_attr.attr,
	&param_read_attr.attr,
	&param_latency_attr.attr,
	&param_level_attr.attr,
	&param_read_all_attr.attr,
	&param_latency_all_attr.attr,
	&notifier_param_reg_attr.attr,
	&notifier_reg_data_all_attr.attr,
	&notifier_events_data_attr.attr,
	&notifier_release_attr.attr,
	&timer_param_reg_attr.attr,
	&timer_reg_data_all_attr.attr,
	&timer_events_data_attr.attr,
	&timer_release_attr.attr,
	&debug_init_attr.attr,
	NULL,
};

static struct attribute_group mpsd_debug_attr_group = {
	.attrs = mpsd_debug_g,
};

/**
 * mpsd_debug_init - Initialize mpsd debug framework, create sysfs files etc.
 * @mpsd_driver: Mpsd driver kobj, parent of mpsd_debug kobj.
 *
 * This function initialize mpsd debig framework, create sysfs files etc.
 */
int mpsd_debug_init(void)
{
	int ret = 0;

	mpsd_debug_kobj = kobject_create_and_add("mpsd_debug", NULL);
	if (!mpsd_debug_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(mpsd_debug_kobj, &mpsd_debug_attr_group);

	init_debug_framework();
	return ret;
}

/**
 * mpsd_debug_exit - Reomves sysfs files etc.
 * @mpsd_driver: Mpsd driver kobj, parent of mpsd_debug kobj.
 *
 * This function removes mpsd debig framework, removes sysfs files etc.
 */
void mpsd_debug_exit(void)
{
	if (mpsd_debug_kobj)
		sysfs_remove_group(mpsd_debug_kobj, &mpsd_debug_attr_group);
}
