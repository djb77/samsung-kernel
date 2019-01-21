/*
 * Header file for mpsd parameters read.
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *	   Ravi Solanki <ravi.siso@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MPSD_PARAM_H
#define _MPSD_PARAM_H

#include <linux/threads.h>
#include <linux/mpsd/mpsd.h>

#define mpsd_attr_rw(_name) \
static struct kobj_attribute _name##_attr = {\
	.attr = {\
		.name = __stringify(_name),\
		.mode = 0644,\
	},\
	.show = _name##_show,\
	.store = _name##_store,\
}

#define mpsd_attr_ro(_name) \
static struct kobj_attribute _name##_attr = {\
	.attr = {\
		.name = __stringify(_name),\
		.mode = 0444,\
	},\
	.show = _name##_show,\
}

#define SHIFT_VALUE_32 (1)
#define get_bit_32(f, n)	(f >> n & (SHIFT_VALUE_32))
#define set_bit_32(f, n)	(f | (SHIFT_VALUE_32 << n))
#define clear_bit_32(f, n)	  (f & (~(SHIFT_VALUE_32 << n)))

#define SHIFT_VALUE_64 (1ULL)
#define get_bit_64(f, n)	(f >> n & (SHIFT_VALUE_64))
#define set_bit_64(f, n)	(f | (SHIFT_VALUE_64 << n))
#define clear_bit_64(f, n)	  (f & (~(SHIFT_VALUE_64 << n)))
#define mpsd_abs(a)	   (a > 0 ? a : (a * (-1)))

#ifdef CONFIG_MPSD_DEBUG
#define DEBUG_LEVEL_MIN (0)
#define DEBUG_LEVEL_LOW (1)
#define DEBUG_LEVEL_MID (2)
#define DEBUG_LEVEL_HIGH (3)
#define DEBUG_LEVEL_MAX (4)
#endif

extern char tag[MAX_TAG_LENGTH];

/**
 * struct param_val - Contains data types of the types of param values.
 * @int_val: get param functions need to fill this member
 *		 if value returned is some type of integer.
 * @int_arr_val: get param functions need to fill this member
 *		 if value returned is some type of integer array.
 *
 * This structure contains all the data types which can be
 * returned by param read functions.
 */
struct param_val {
	struct app_params_struct app_param_val;
	struct dev_params_struct dev_param_val;
};

/**
 * struct app_task_sig_params - Contains task signal related params.
 * @num_threads: Number of threads in this app.
 * @oom_score_adj: Based on this value decision is made which app to kill.
 * @oom_score_adj_min: Minimum value of oom_score_adj set for an app.
 * @utime: Time that this app has been scheduled in user mode.
 * @stime: Time that this app has been scheduled in kernel mode.
 * @mem_min_pgflt: The number of minor faults the app has made.
 * @mem_maj_pgflt: The number of major faults the app has made.
 *
 * This structure contains all the application specific task signal related
 * kernel parameters.
 */
struct app_task_sig_params {
	long long num_threads;
	long long oom_score_adj;
	long long oom_score_adj_min;
	long long utime;
	long long stime;
	long long mem_min_pgflt;
	long long mem_maj_pgflt;
};

/**
 * struct app_mem_params - Contains application memory related params.
 * @mem_vsize: Virtual memory size in bytes of this app.
 * @mem_data: Data + stack size in bytes of this app.
 * @mem_stack: stack size in bytes of this application.
 * @mem_text: Text (code) size in pages of this app.
 * @mem_swap: Swapped memory in pages of this app.
 * @mem_shared: Shared memory in pages of this app.
 * @mem_anon: Anonymous memory in pages of this app.
 * @mem_rss: Resident Set Size: Pages the app has in real memory.
 * @mem_rss_limit: Current soft limit in bytes on the rss of the app.
 *
 * This structure contains all the application specific memory related
 * kernel parameters.
 */
struct app_mem_params {
	long long mem_vsize;
	long long mem_data;
	long long mem_stack;
	long long mem_text;
	long long mem_swap;
	long long mem_shared;
	long long mem_anon;
	long long mem_rss;
	long long mem_rss_limit;
};

#ifdef CONFIG_PROC_PAGE_MONITOR
/**
 * struct app_pss_params - Contains application pss stats params.
 * @resident: Number of resident pages.
 * @shared_clean: Number of shared_clean pages.
 * @shared_dirty: Number of shared_dirty pages.
 * @private_clean: Number of private_clean pages.
 * @private_dirty: Number of private_dirty pages.
 * @referenced: Number of referenced pages.
 * @anonymous: Number of anonymous pages.
 * @anonymous_thp: Number of anonymous_thp pages.
 * @swap: Number of swap pages.
 * @shared_hugetlb: Number of shared_hugetlb pages.
 * @private_hugetlb: Number of private_hugetlb pages.
 * @pss: Number of pss pages.
 * @swap_pss: Number of swap_pss pages.
 *
 * This structure contains all the application specific pss stats related
 * kernel parameters.
 */
struct app_pss_params {
	long long resident;
	long long shared_clean;
	long long shared_dirty;
	long long private_clean;
	long long private_dirty;
	long long referenced;
	long long anonymous;
	long long anonymous_thp;
	long long swap;
	long long shared_hugetlb;
	long long private_hugetlb;
	long long pss;
	long long swap_pss;
};
#endif /* CONFIG_PROC_PAGE_MONITOR */

/**
 * struct app_io_params - Contains application io params.
 * @io_rchar: Number of bytes read by this application.
 * @io_wchar: Number of bytes wrote by this application.
 * @io_syscr: Number of read syscalls by this application.
 * @io_syscw: Number of write syscalls by this application.
 * @io_syscfs: Number of fsync syscalls by this application.
 * @io_bytes_read: The number of bytes caused to be read from storage.
 * @io_bytes_write: The number of bytes caused to be written to storage.
 * @io_bytes_write_cancel: Bytes cancelled from writting.
 *
 * This structure contains all the application specific io related
 * kernel parameters which will be used by mpsd.
 */
struct app_io_params {
#ifdef CONFIG_TASK_XACCT
	long long io_rchar;
	long long io_wchar;
	long long io_syscr;
	long long io_syscw;
	long long io_syscfs;
#endif /* CONFIG_TASK_XACCT */
#ifdef CONFIG_TASK_IO_ACCOUNTING
	long long io_bytes_read;
	long long io_bytes_write;
	long long io_bytes_write_cancel;
#endif /* CONFIG_TASK_IO_ACCOUNTING */
};

/**
 * is_valid_app_param - Checks if input param is valid application parameter.
 * @param: Application parameter to be checked.
 *
 * Checks input param passed if it belong to the application parameters enum.
 *
 * Returns 1 if param is valid, 0 if invalid.
 */
static inline unsigned int is_valid_app_param(enum app_param param)
{
	if (param < APP_PARAM_MIN || param >= APP_PARAM_MAX)
		return 0;
	return 1;
}

/**
 * is_valid_dev_param - Checks if input param is valid device parameter.
 * @param: Device parameter to be checked.
 *
 * Checks input param passed if it belong to the device parameters enum.
 *
 * Returns 1 if param is valid, 0 if invalid.
 */
static inline unsigned int is_valid_dev_param(enum dev_param param)
{
	if (param < DEV_PARAM_MIN || param >= DEV_PARAM_MAX)
		return 0;
	return 1;
}

/**
 * is_valid_param - Checks if input param is valid mpsd parameter.
 * @param: Mpsd parameter to be checked.
 *
 * Checks input param passed if it belong to the Mpsd paramters.
 *
 * Returns 1 if param is valid, 0 if invalid.
 */
static inline unsigned int is_valid_param(unsigned int param)
{
	if ((is_valid_app_param((enum app_param)param == 0))
		&& (is_valid_dev_param((enum dev_param)param == 0)))
		return 0;
	return 1;
}

/**
 * get_event_bit - Get bit corresponding to the event from bit field
 * @field: Pointer to the bit field to be set.
 * @event: Event whose bit field need to be checked.
 *
 * get bit corresponding to the event from bit field
 *
 * Returns the field as 0 or 1.
 */
static inline bool get_event_bit(unsigned int field, unsigned int event)
{
	return (bool)get_bit_32(field, event);
}

/**
 * set_event_bit - Set bit corresponding to the event from bit field
 * @field: Pointer to the bit field to be set.
 * @event: event whose bit field need to be set.
 *
 * Set bit corresponding to the event from bit field
 */
static inline void set_event_bit(unsigned int *field, unsigned int event)
{
	*field = set_bit_32(*field, event);
}

/**
 * clear_event_bit - Clear bit corresponding to the event from bit field
 * @field: Pointer to the bit field to be cleared.
 * @event: event whose bit field need to be clear.
 *
 * Clear bit corresponding to the event from bit field
 */
static inline void clear_event_bit(unsigned int *field, unsigned int event)
{
	*field = clear_bit_32(*field, event);
}

/**
 * adjust_param_bit - Adjust the bit offset for app and dev params.
 * @param: Mpsd parameter whose bit to be adjusted.
 *
 * Adjust the bit offset for app and dev params.
 *
 * Returns the adjusted bit.
 */
static inline int adjust_param_bit(int param)
{
	int bit = 0;

	if (is_valid_app_param((enum app_param)param))
		bit = param - APP_PARAM_MIN;
	else if (is_valid_dev_param((enum dev_param)param))
		bit = param - DEV_PARAM_MIN;
	else
		pr_err("%s: %s: Parameter is not Valid\n", tag, __func__);
	return bit;
}

/**
 * get_param_bit - Get bit corresponding to the param from bit field
 * @field: Pointer to the bit field to be set.
 * @id: Param whose bit field need to be checked.
 *
 * get bit corresponding to the param from bit field
 *
 * Returns the field as 0 or 1.
 */
static inline bool get_param_bit(unsigned long long field, int id)
{
	return (bool)get_bit_64(field, adjust_param_bit(id));
}

/**
 * set_param_bit - Set bit corresponding to the param from bit field
 * @field: Pointer to the bit field to be set.
 * @id: Param whose bit field need to be set.
 *
 * Set bit corresponding to the param from bit field
 */
static inline void set_param_bit(unsigned long long *field, int id)
{
	*field = set_bit_64(*field, adjust_param_bit(id));
}

/**
 * clear_param_bit - Clear bit corresponding to the param from bit field
 * @field: Pointer to the bit field to be cleared.
 * @id: Param whose bit field need to be clear.
 *
 * Clear bit corresponding to the param from bit field
 */
static inline void clear_param_bit(unsigned long long *field, int id)
{
	*field = clear_bit_64(*field, adjust_param_bit(id));
}

struct app_io_params get_app_io_params(unsigned int pid);
struct app_pss_params get_app_pss_params(unsigned int pid);
struct app_mem_params get_app_mem_params(unsigned int pid);
struct app_task_sig_params get_app_task_sig_params(unsigned int pid);

bool is_task_valid(unsigned int pid);

void memset_app_params(struct app_params_struct *data);
void memset_dev_params(struct dev_params_struct *data);

void get_app_params_snapshot(unsigned int pid,
	struct app_params_struct *app_params);
void  get_dev_params_snapshot(struct dev_params_struct *dev_params);

void mpsd_get_app_params(unsigned int pid, enum app_param param,
	struct app_params_struct *data);
void mpsd_get_dev_params(enum dev_param param,
	struct dev_params_struct *data);

void mpsd_read_init(void);
void mpsd_read_exit(void);
void mpsd_read_start_cpuload_work(void);
void mpsd_read_stop_cpuload_work(void);

struct memory_area *get_device_mmap(void);
u32 get_behavior(void);
u64 get_app_field_mask(void);
u64 get_dev_field_mask(void);

int mpsd_populate_mmap(unsigned long long app_field_mask,
	unsigned long long device_field_mask, int pid, enum req_id req,
	unsigned int event);
void mpsd_notify_umd(void);

void set_cpufreq_policy_map(void);
void get_cpufreq_policy_map(int map[]);

void mpsd_set_timer_pid(int pid);

void mpsd_timer_deregister_all(void);
void mpsd_timer_suspend(void);
void mpsd_timer_resume(void);

int mpsd_timer_deregister(int param_id);
int mpsd_timer_register(int param_id, int val);

void mpsd_timer_init(void);
void mpsd_timer_exit(void);

void mpsd_set_notifier_pid(int pid);

void mpsd_register_for_event_notify(struct param_info_notifier *param);
void mpsd_unregister_for_event_notify(struct param_info_notifier *param);

bool get_notifier_init_status(void);

void mpsd_notifier_init(void);
void mpsd_notifier_release(void);

#ifdef CONFIG_MPSD_DEBUG
unsigned int get_mpsd_debug_level(void);
unsigned int get_debug_level_driver(void);
unsigned int get_debug_level_read(void);
unsigned int get_debug_level_timer(void);
unsigned int get_debug_level_notifier(void);

struct param_info_notifier *get_notifier_app_info_arr(void);
struct param_info_notifier *get_notifier_dev_info_arr(void);

unsigned long long get_notifier_app_param_mask(void);
unsigned long long get_notifier_dev_param_mask(void);

int print_mmap_buffer(void);

void print_notifier_registration_data(void);
void print_notifier_events_data(struct param_data_notifier *data);

int print_timer_reg_data(int in_buf, char *buf);
void print_timer_events_data(long long value, int count,
	unsigned long long app_field, unsigned long long dev_field);

void debug_set_mpsd_flag(int val);
void debug_set_mpsd_behavior(unsigned int val);

int debug_create_mmap_area(void);

int mpsd_debug_init(void);
void mpsd_debug_exit(void);
#endif

#endif /* _MPSD_PARAM_H */
