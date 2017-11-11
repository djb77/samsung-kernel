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
	INFO_SYSMMU,		// Debugging Option 1
	INFO_BUSMON,		// Debugging Option 2
	INFO_DPM_TIMEOUT,	// Debugging Option 3
	INFO_EXTRA_4,		// Debugging Option 4
	INFO_EXTRA_5,		// Debugging Option 5
	INFO_MAX,
};


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
	UPLOAD_CAUSE_FORCED_UPLOAD	= 0x00000022,
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
extern void sec_debug_save_last_kmsg(unsigned char* head_ptr, unsigned char* curr_ptr);
#else
#define sec_debug_save_last_kmsg(a, b)		do { } while(0)
#endif

#ifdef CONFIG_SEC_PARAM
#define CM_OFFSET CONFIG_CM_OFFSET
#define CM_OFFSET_LIMIT 1
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

#endif /* SEC_DEBUG_H */
