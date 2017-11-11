/*
* Samsung debugging features for Samsung's SoC's.
*
* Copyright (c) 2014 Samsung Electronics Co., Ltd.
*      http://www.samsung.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*/

#ifndef SEC_DEBUG_H
#define SEC_DEBUG_H

#include <linux/sizes.h>

#ifdef CONFIG_SEC_DEBUG
enum sec_debug_extra_buf_type {
	INFO_BUG	= 0,
	INFO_SYSMMU,			/* Debugging Option 1 */
	INFO_BUSMON,			/* Debugging Option 2 */
	INFO_DPM_TIMEOUT,		/* Debugging Option 3 */
	INFO_SMPL_CNT,			/* Debugging Option 4 */
	INFO_SLUB_DBG = INFO_SMPL_CNT,	/* Debugging Option 4 */
	INFO_SYN_INFO,			/* Debugging Option 5 */
	INFO_MAX,
};

extern void (*mach_restart)(char mode, const char *cmd);
extern bool exynos_ss_hardkey_triger;

extern int  sec_debug_init(void);
extern void sec_debug_reboot_handler(void);
extern void sec_debug_panic_handler(void *buf, bool dump);
extern void sec_debug_check_crash_key(unsigned int code, int value);

extern int  sec_debug_get_debug_level(void);
extern void sec_debug_disable_printk_process(void);

/* getlog support */
extern void sec_getlog_supply_kernel(void *klog_buf);
extern void sec_getlog_supply_platform(unsigned char *buffer, const char *name);

/* reset extra info */
extern void sec_debug_store_extra_buf(enum sec_debug_extra_buf_type type, const char *fmt, ...);
extern void sec_debug_store_fault_addr(unsigned long addr, struct pt_regs *regs);
extern void sec_debug_store_backtrace(struct pt_regs *regs);
extern void sec_gaf_supply_rqinfo(unsigned short curr_offset, unsigned short rq_offset);
#else
#define sec_debug_init()			(-1)
#define sec_debug_reboot_handler()		do { } while(0)
#define sec_debug_panic_handler(a,b)		do { } while(0)
#define sec_debug_check_crash_key(a,b)		do { } while(0)

#define sec_debug_get_debug_level()		0
#define sec_debug_disable_printk_process()	do { } while(0)

#define sec_getlog_supply_kernel(a)		do { } while(0)
#define sec_getlog_supply_platform(a,b)		do { } while(0)

static inline void sec_gaf_supply_rqinfo(unsigned short curr_offset, unsigned short rq_offset)
{
    return;
}
#endif /* CONFIG_SEC_DEBUG */


/* sec logging */
#ifdef CONFIG_SEC_AVC_LOG
extern void sec_debug_avc_log(char *fmt, ...);
#else
#define sec_debug_avc_log(a, ...)		do { } while(0)
#endif

#ifdef CONFIG_SEC_DEBUG_TSP_LOG
/**
 * sec_debug_tsp_log : Leave tsp log in tsp_msg file.
 * ( Timestamp + Tsp logs )
 * sec_debug_tsp_log_msg : Leave tsp log in tsp_msg file and
 * add additional message between timestamp and tsp log.
 * ( Timestamp + additional Message + Tsp logs )
 */
extern void sec_debug_tsp_log(char *fmt, ...);
extern void sec_debug_tsp_log_msg(char *msg, char *fmt, ...);
#if defined(CONFIG_TOUCHSCREEN_FTS)
extern void tsp_dump(void);
#elif defined(CONFIG_TOUCHSCREEN_SEC_TS)
extern void tsp_dump_sec(void);
#endif

#else
#define sec_debug_tsp_log(a, ...)		do { } while(0)
#endif

enum sec_debug_upload_cause_t {
	UPLOAD_CAUSE_INIT		= 0xCAFEBABE,
	UPLOAD_CAUSE_KERNEL_PANIC	= 0x000000C8,
	UPLOAD_CAUSE_HARDKEY_RESET	= 0x000000CA,
	UPLOAD_CAUSE_FORCED_UPLOAD	= 0x00000022,
#ifdef CONFIG_SEC_UPLOAD
	UPLOAD_CAUSE_USER_FORCED_UPLOAD	= 0x00000074,
#endif
	UPLOAD_CAUSE_CP_ERROR_FATAL	= 0x000000CC,
	UPLOAD_CAUSE_USER_FAULT		= 0x0000002F,
	UPLOAD_CAUSE_HSIC_DISCONNECTED	= 0x000000DD,
};

#if defined(CONFIG_SEC_INITCALL_DEBUG)
#define SEC_INITCALL_DEBUG_MIN_TIME	10000

extern void sec_initcall_debug_add(initcall_t fn,
	unsigned long long duration);
#endif

#ifdef CONFIG_SEC_DEBUG_LAST_KMSG
extern void sec_debug_save_last_kmsg(unsigned char* head_ptr, unsigned char* curr_ptr, size_t log_size);
#else
#define sec_debug_save_last_kmsg(a, b)		do { } while(0)
#endif

#ifdef CONFIG_SEC_PARAM
#define CM_OFFSET CONFIG_CM_OFFSET
#define CM_OFFSET_LIMIT 8
enum
{
	PARAM_OFF = '0',
	PARAM_ON = '1',
};
enum
{
	PARAM_TEST0 = 0,
	PARAM_TEST1,
	PARAM_TEST2,
	PARAM_TEST3,
	PARAM_MAX,
};
int sec_set_param(unsigned long offset, char val);
#endif /* CONFIG_SEC_PARAM */

/* layout of SDRAM
	   0: magic (4B)
      4~1023: panic string (1020B)
 0x400~0x7ff: panic Extra Info (1KB)
0x800~0x1000: panic dumper log
      0x4000: copy of magic
 */
#define SEC_DEBUG_MAGIC_PA 0x80000000
#define SEC_DEBUG_MAGIC_VA phys_to_virt(SEC_DEBUG_MAGIC_PA)
#define SEC_DEBUG_EXTRA_INFO_VA SEC_DEBUG_MAGIC_VA+0x400
#define BUF_SIZE_MARGIN SZ_1K - 0x80

/* store panic extra info
        "KTIME":""      : kernel time
        "FAULT":""      : pgd,va,*pgd,*pud,*pmd,*pte
        "BUG":""        : bug msg
        "PANIC":""      : panic buffer msg
        "PC":""         : pc val
        "LR":""         : link register val
        "STACK":""      : backtrace
        "CHIPID":""     : CPU Serial Number
        "DBG0":""       : Debugging Option 0
        "DBG1":""       : Debugging Option 1
        "DBG2":""       : Debugging Option 2
        "DBG3":""       : Debugging Option 3
        "DBG4":""       : Debugging Option 4
        "DBG5":""       : Debugging Option 5
*/
struct sec_debug_panic_extra_info {
	unsigned long fault_addr;
	char extra_buf[INFO_MAX][SZ_256];
	unsigned long pc;
	unsigned long lr;
	char backtrace[SZ_512];
};

#ifdef CONFIG_KFAULT_AUTO_SUMMARY
#define AUTO_COMMENT_SIZE 0xf3c
#define AUTO_SUMMARY_MAGIC 0xcafecafe
#define AUTO_SUMMARY_TAIL_MAGIC 0x00c0ffee

#define PRIO_LV0 0
#define PRIO_LV1 1
#define PRIO_LV2 2
#define PRIO_LV3 3
#define PRIO_LV4 4
#define PRIO_LV5 5
#define PRIO_LV6 6
#define PRIO_LV7 7
#define PRIO_LV8 8
#define PRIO_LV9 9

#define SEC_DEBUG_AUTO_COMM_BUF_SIZE 10

enum sec_debug_FREQ_INFO
{
	FREQ_INFO_CLD0 = 0,
	FREQ_INFO_CLD1,
	FREQ_INFO_INT,
	FREQ_INFO_MIF,
	FREQ_INFO_MAX,
};

struct sec_debug_auto_comm_buf {
	atomic_t logging_entry;
	atomic_t logging_diable;
	atomic_t logging_count;
	unsigned int offset;
	char buf[SZ_4K];
};

struct sec_debug_auto_comm_freq_info {
	int old_freq;
	int new_freq;
	u64 time_stamp;
	u64 last_freq_info;
};

struct sec_debug_auto_summary {
	int haeder_magic;
	int fault_flag;
	int lv5_log_cnt;
	u64 lv5_log_order;
	int order_map_cnt;
	int order_map[SEC_DEBUG_AUTO_COMM_BUF_SIZE];
	struct sec_debug_auto_comm_buf auto_Comm_buf[SEC_DEBUG_AUTO_COMM_BUF_SIZE];
	struct sec_debug_auto_comm_freq_info freq_info[FREQ_INFO_MAX];
	// for code diff
	u64 pa_text;
	u64 pa_start_rodata;
	int tail_magic;
};

extern void sec_debug_auto_summary_log_disable(int type);
extern void sec_debug_auto_summary_log_once(int type);
extern void sec_debug_set_auto_comm_last_devfreq_buf(struct sec_debug_auto_comm_freq_info *freq_info);
extern void sec_debug_set_auto_comm_last_cpufreq_buf(struct sec_debug_auto_comm_freq_info *freq_info);
#endif

#ifdef CONFIG_SEC_DUMP_SUMMARY

#define SEC_DEBUG_SUMMARY_MAGIC0 0xFFFFFFFF
#define SEC_DEBUG_SUMMARY_MAGIC1 0x5ECDEB6
#define SEC_DEBUG_SUMMARY_MAGIC2 0x14F014F0
 /* high word : major version
  * low word : minor version
  * minor version changes should not affect bootloader behavior
  */
#define SEC_DEBUG_SUMMARY_MAGIC3 0x00010001

struct __log_struct_info {
	unsigned int buffer_offset;
	unsigned int w_off_offset;
	unsigned int head_offset;
	unsigned int size_offset;
	unsigned int size_t_typesize;
};

struct __log_data {
	unsigned long log_paddr;
	unsigned long buffer_paddr;
};

struct sec_debug_summary_log {
	unsigned long idx_paddr;
	unsigned long log_paddr;
	unsigned long size;
};

struct sec_debug_summary_logger_log_info {
	struct __log_struct_info stinfo;
	struct __log_data main;
	struct __log_data system;
	struct __log_data events;
	struct __log_data radio;
};

struct sec_debug_summary_excp_kernel {
	char pc_sym[64];
	char lr_sym[64];
	char panic_caller[64];
	char panic_msg[128];
	char thread[32];
};

struct sec_debug_summary_sched_log {
	unsigned long task_idx_paddr;
	unsigned long task_buf_paddr;
	unsigned int task_struct_sz;
	unsigned int task_array_cnt;
	unsigned long irq_idx_paddr;
	unsigned long irq_buf_paddr;
	unsigned int irq_struct_sz;
	unsigned int irq_array_cnt;
	unsigned long work_idx_paddr;
	unsigned long work_buf_paddr;
	unsigned int work_struct_sz;
	unsigned int work_array_cnt;
	unsigned long cpuidle_idx_paddr;
	unsigned long cpuidle_buf_paddr;
	unsigned int cpuidle_struct_sz;
	unsigned int cpuidle_array_cnt;	
#ifdef CONFIG_EXYNOS_SNAPSHOT_HRTIMER
	unsigned long timer_idx_paddr;
	unsigned long timer_buf_paddr;
	unsigned int timer_struct_sz;
	unsigned int timer_array_cnt;
#endif
};

struct sec_debug_summary_cpufreq_policy {
	unsigned long paddr;
	int name_length;
	int min_offset;
	int max_offset;
	int cur_offset;
};

struct sec_debug_summary_cpu_info {
	struct sec_debug_summary_cpufreq_policy cpufreq_policy;
	unsigned long cpu_offset_paddr;
	unsigned long cpu_active_mask_paddr;
	unsigned long cpu_online_mask_paddr;
};

struct sec_debug_summary_data_kernel {
	char name[16];
	char state[16];
	int nr_cpus;

	struct sec_debug_summary_log log;
	struct sec_debug_summary_excp_kernel excp;
	struct sec_debug_summary_sched_log sched_log;
	struct sec_debug_summary_logger_log_info logger_log;
	struct sec_debug_summary_cpu_info cpu_info;
#if 0
	unsigned long cmdline_paddr;
	unsigned long cmdline_len;
	unsigned long linuxbanner_paddr;
	unsigned long linuxbanner_len;
#endif
};

struct sec_debug_summary_ksyms {
	uint32_t magic;
	uint32_t kallsyms_all;
	uint64_t addresses_pa;
	uint64_t names_pa;
	uint64_t num_syms;
	uint64_t token_table_pa;
	uint64_t token_index_pa;
	uint64_t markers_pa;
	struct ksect {
		uint64_t sinittext;
		uint64_t einittext;
		uint64_t stext;
		uint64_t etext;
		uint64_t end;
	} sect;
};

struct sec_debug_summary {
	unsigned int magic[4];

	struct sec_debug_summary_data_kernel kernel;	
	struct sec_debug_summary_ksyms ksyms;

	unsigned long log_kernel_base;
	unsigned long log_kernel_start;

	unsigned long reserved_out_buf;
	unsigned long reserved_out_size;
};

extern int sec_debug_summary_set_logger_info(	struct sec_debug_summary_logger_log_info *log_info);
extern void exynos_ss_summary_set_sched_log_buf(struct sec_debug_summary *);
extern void sec_debug_summary_set_kallsyms_info(struct sec_debug_summary *);
int sec_debug_save_panic_info(const char *str, unsigned long caller);
int sec_debug_set_cpu_info(struct sec_debug_summary *summary_info, char *summary_log_buf);
#endif

#endif /* SEC_DEBUG_H */
