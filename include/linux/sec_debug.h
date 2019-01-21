/*
 * include/linux/sec_debug.h
 *
 * COPYRIGHT(C) 2006-2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef SEC_DEBUG_H
#define SEC_DEBUG_H

#include <linux/ftrace.h>
#include <linux/of_address.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/semaphore.h>

#include <linux/sec_debug_arm.h>
#include <linux/sec_debug_arm64.h>

#include <asm/cacheflush.h>
#include <asm/io.h>

enum sec_restart_reason_t {
	RESTART_REASON_NORMAL = 0x0,
	RESTART_REASON_BOOTLOADER = 0x77665500,
	RESTART_REASON_REBOOT = 0x77665501,
	RESTART_REASON_RECOVERY = 0x77665502,
	RESTART_REASON_RTC = 0x77665503,
	RESTART_REASON_DMVERITY_CORRUPTED = 0x77665508,
	RESTART_REASON_DMVERITY_ENFORCE = 0x77665509,
	RESTART_REASON_KEYS_CLEAR = 0x7766550a,
	RESTART_REASON_SEC_DEBUG_MODE = 0x776655ee,
	RESTART_REASON_END = 0xffffffff,
};

#define RESET_EXTRA_INFO_SIZE	1024

/* Enable CONFIG_RESTART_REASON_DDR to use DDR address for saving restart reason */
#ifdef CONFIG_RESTART_REASON_DDR
extern void *restart_reason_ddr_address;
#endif

#ifdef CONFIG_SEC_DEBUG
DECLARE_PER_CPU(struct sec_debug_core_t, sec_debug_core_reg);
DECLARE_PER_CPU(struct sec_debug_mmu_reg_t, sec_debug_mmu_reg);
DECLARE_PER_CPU(enum sec_debug_upload_cause_t, sec_debug_upload_cause);

static inline void sec_debug_save_context(void)
{
	unsigned long flags;

	local_irq_save(flags);
	sec_debug_save_mmu_reg(&per_cpu(sec_debug_mmu_reg,
				smp_processor_id()));
	sec_debug_save_core_reg(&per_cpu(sec_debug_core_reg,
				smp_processor_id()));
	pr_emerg("(%s) context saved(CPU:%d)\n", __func__,
			smp_processor_id());
	local_irq_restore(flags);
	flush_cache_all();
}

void simulate_msm_thermal_bite(void);
extern void sec_debug_prepare_for_wdog_bark_reset(void);
extern int sec_debug_init(void);
extern int sec_debug_dump_stack(void);
extern void sec_debug_hw_reset(void);
#ifdef CONFIG_SEC_PERIPHERAL_SECURE_CHK
extern void sec_peripheral_secure_check_fail(void);
#endif
extern void sec_getlog_supply_fbinfo(void *p_fb, u32 res_x, u32 res_y, u32 bpp,
		u32 frames);
extern void sec_getlog_supply_meminfo(u32 size0, u32 addr0, u32 size1,
		u32 addr1);
extern void sec_getlog_supply_loggerinfo(void *p_main, void *p_radio,
		void *p_events, void *p_system);
extern void sec_getlog_supply_kloginfo(void *klog_buf);

extern void sec_gaf_supply_rqinfo(unsigned short curr_offset,
				  unsigned short rq_offset);
extern int sec_debug_is_enabled(void);
extern int sec_debug_is_modem_seperate_debug_ssr(void);
extern int silent_log_panic_handler(void);
extern void sec_debug_secure_app_addr_size(uint32_t addr, uint32_t size);
extern void sec_debug_print_model(struct seq_file *m, const char *cpu_name);

extern void sec_debug_update_dload_mode(const int restart_mode,
		const int in_panic);
extern void sec_debug_update_restart_reason(const char *cmd,
		const int in_panic);
extern void sec_debug_set_thermal_upload(void);

/* from 'msm-poweroff.c' */
extern void set_dload_mode(int on);

#ifdef CONFIG_SEC_LOG_LAST_KMSG
extern void sec_set_reset_extra_info(char *last_kmsg_buffer,
			unsigned last_kmsg_size);
#else /* CONFIG_SEC_LOG_LAST_KMSG */
static inline void sec_set_reset_extra_info(char *last_kmsg_buffer,
			unsigned last_kmsg_size) {}
#endif /* CONFIG_SEC_LOG_LAST_KMSG */

#else /* CONFIG_SEC_DEBUG */
static inline void sec_debug_save_context(void) {}
static inline void sec_debug_prepare_for_wdog_bark_reset(void) {}
static inline int sec_debug_init(void) { return 0; }
static inline int sec_debug_dump_stack(void) { return 0; }
static inline void sec_peripheral_secure_check_fail(void) {}
static inline void sec_getlog_supply_fbinfo(void *p_fb, u32 res_x, u32 res_y,
					    u32 bpp, u32 frames) {}

static inline void sec_getlog_supply_meminfo(u32 size0, u32 addr0, u32 size1,
					     u32 addr1) {}

#define sec_getlog_supply_loggerinfo(p_main, p_radio, p_events, p_system)

static inline void sec_getlog_supply_kloginfo(void *klog_buf) {}

static inline void sec_gaf_supply_rqinfo(unsigned short curr_offset,
					 unsigned short rq_offset) {}

static inline int sec_debug_is_enabled(void) { return 0; }
static inline void sec_debug_hw_reset(void) {}
static inline void emerg_pet_watchdog(void) {}
static inline void sec_debug_set_rr(u32 reason) {}
static inline u32 sec_debug_get_rr(void) { return 0; }
static inline void sec_debug_print_model(
		struct seq_file *m, const char *cpu_name) {}

static void sec_debug_update_dload_mode(const int restart_mode,
		const int in_panic) {}
static inline void sec_debug_update_restart_reason(const char *cmd,
		const int in_panic) {}
static inline void sec_debug_set_thermal_upload(void) {}
#endif /* CONFIG_SEC_DEBUG */

#ifdef CONFIG_SEC_FILE_LEAK_DEBUG
extern void sec_debug_EMFILE_error_proc(void);
#else
static inline void sec_debug_EMFILE_error_proc(void) {}
#endif

#ifdef CONFIG_SEC_SSR_DEBUG_LEVEL_CHK
extern int sec_debug_is_enabled_for_ssr(void);
#else
static inline int sec_debug_is_enabled_for_ssr(void) { return 0; }
#endif

#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
extern void sec_debug_task_sched_log_short_msg(char *msg);
extern void sec_debug_secure_log(u32 svc_id, u32 cmd_id);
extern void sec_debug_task_sched_log(int cpu, struct task_struct *task,
		struct task_struct *prev);
extern void sec_debug_irq_sched_log(unsigned int irq, void *fn, char *name,
		unsigned int en);
extern void sec_debug_irq_sched_log_end(void);
extern void sec_debug_timer_log(unsigned int type, int int_lock, void *fn);
extern void sec_debug_sched_log_init(void);
extern void sec_debug_save_last_pet(unsigned long long last_pet);
extern void sec_debug_save_last_ns(unsigned long long last_ns);
extern void sec_debug_irq_enterexit_log(unsigned int irq, u64 start_time);
#define secdbg_sched_msg(fmt, ...) \
	do { \
		char ___buf[16]; \
		snprintf(___buf, sizeof(___buf), fmt, ##__VA_ARGS__); \
		sec_debug_task_sched_log_short_msg(___buf); \
	} while (0)
#else
static inline void sec_debug_task_sched_log(int cpu, struct task_struct *task,
					struct task_struct *prev) {}
static inline void sec_debug_secure_log(u32 svc_id, u32 cmd_id) {}
#define sec_debug_irq_sched_log(...)  do {} while(0)
static inline void sec_debug_irq_sched_log_end(void) {}
static inline void sec_debug_timer_log(unsigned int type,
					int int_lock, void *fn) {}
static inline void sec_debug_sched_log_init(void) {}
static inline void sec_debug_save_last_pet(unsigned long long last_pet) {}
static inline void sec_debug_save_last_ns(unsigned long long last_ns) {}
static inline void sec_debug_irq_enterexit_log(unsigned int irq,
					u64 start_time) {}
#define secdbg_sched_msg(fmt, ...)
#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

#define IRQ_ENTRY 0x4945
#define IRQ_EXIT  0x4958

#define SOFTIRQ_ENTRY  0x5345
#define SOFTIRQ_EXIT   0x5358

#define SCM_ENTRY 0x5555
#define SCM_EXIT  0x6666

/* klaatu - schedule log */
#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
#define SCHED_LOG_MAX 512
#define TZ_LOG_MAX 64

struct irq_log {
	u64 time;
	unsigned int irq;
	void *fn;
	char *name;
	int en;
	int preempt_count;
	void *context;
	pid_t pid;
	unsigned int entry_exit;
};

struct secure_log {
	u64 time;
	u32 svc_id, cmd_id;
	pid_t pid;
};

struct irq_exit_log {
	unsigned int irq;
	u64 time;
	u64 end_time;
	u64 elapsed_time;
	pid_t pid;
};

struct sched_log {
	u64 time;
	char comm[TASK_COMM_LEN];
	pid_t pid;
	struct task_struct *pTask;
	char prev_comm[TASK_COMM_LEN];
	int prio;
	pid_t prev_pid;
	int   prev_prio;
	int   prev_state;
};

struct timer_log {
	u64 time;
	unsigned int type;
	int int_lock;
	void *fn;
	pid_t pid;
};
#endif	/* CONFIG_SEC_DEBUG_SCHED_LOG */

#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG
#define SEMAPHORE_LOG_MAX 100
struct sem_debug {
	struct list_head list;
	struct semaphore *sem;
	struct task_struct *task;
	pid_t pid;
	int cpu;
	/* char comm[TASK_COMM_LEN]; */
};

enum {
	READ_SEM,
	WRITE_SEM
};

#define RWSEMAPHORE_LOG_MAX 100
struct rwsem_debug {
	struct list_head list;
	struct rw_semaphore *sem;
	struct task_struct *task;
	pid_t pid;
	int cpu;
	int direction;
	/* char comm[TASK_COMM_LEN]; */
};
#endif	/* CONFIG_SEC_DEBUG_SEMAPHORE_LOG */

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
extern asmlinkage int sec_debug_msg_log(void *caller, const char *fmt, ...);
#define MSG_LOG_MAX 1024
struct secmsg_log {
	u64 time;
	char msg[64];
	void *caller0;
	void *caller1;
	char *task;
};
#define secdbg_msg(fmt, ...) \
	sec_debug_msg_log(__builtin_return_address(0), fmt, ##__VA_ARGS__)
#else
#define secdbg_msg(fmt, ...)
#endif

/* KNOX_SEANDROID_START */
#ifdef CONFIG_SEC_DEBUG_AVC_LOG
extern asmlinkage int sec_debug_avc_log(const char *fmt, ...);
#define AVC_LOG_MAX 256
struct secavc_log {
	char msg[256];
};
#define secdbg_avc(fmt, ...) \
	sec_debug_avc_log(fmt, ##__VA_ARGS__)
#else /* CONFIG_SEC_DEBUG_AVC_LOG */
#define secdbg_avc(fmt, ...)
#endif /* CONFIG_SEC_DEBUG_AVC_LOG */
/* KNOX_SEANDROID_END */

#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
#define DCVS_LOG_MAX 256

struct dcvs_debug {
	u64 time;
	int cpu_no;
	unsigned int prev_freq;
	unsigned int new_freq;
};
extern void sec_debug_dcvs_log(int cpu_no, unsigned int prev_freq,
			unsigned int new_freq);
#else /* CONFIG_SEC_DEBUG_DCVS_LOG */
static inline void sec_debug_dcvs_log(int cpu_no, unsigned int prev_freq,
					unsigned int new_freq) {}

#endif /* CONFIG_SEC_DEBUG_DCVS_LOG */

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
#define FG_LOG_MAX 128

struct fuelgauge_debug {
	u64 time;
	unsigned int voltage;
	unsigned short soc;
	unsigned short charging_status;
};
extern void sec_debug_fuelgauge_log(unsigned int voltage, unsigned short soc,
			unsigned short charging_status);
#else /* CONFIG_SEC_DEBUG_FUELGAUGE_LOG */
static inline void sec_debug_fuelgauge_log(unsigned int voltage,
			unsigned short soc, unsigned short charging_status) {}

#endif /* CONFIG_SEC_DEBUG_FUELGAUGE_LOG */

#ifdef CONFIG_SEC_DEBUG_POWER_LOG

#define POWER_LOG_MAX 256
enum {
	CLUSTER_POWER_TYPE,
	CPU_POWER_TYPE,
	CLOCK_RATE_TYPE,
};

struct cluster_power {
	char *name;
	int index;
	unsigned long sync_cpus;
	unsigned long child_cpus;
	bool from_idle;
	int entry_exit;
};

struct cpu_power {
	int index;
	int entry_exit;
	bool success;
};

struct clock_rate {
	char *name;
	u64 state;
	u64 cpu_id;
	int complete;
};

struct power_log {
	u64 time;
	pid_t pid;
	unsigned int type;
	union {
		struct cluster_power cluster;
		struct cpu_power cpu;
		struct clock_rate clk_rate;
	};
};
void sec_debug_cpu_lpm_log(int cpu, unsigned int index, bool success, int entry_exit);
void sec_debug_cluster_lpm_log(const char *name, int index, unsigned long sync_cpus,
			unsigned long child_cpus, bool from_idle, int entry_exit);

extern void sec_debug_clock_log(const char *name, unsigned int state, unsigned int cpu_id, int complete);
#define sec_debug_clock_rate_log(name, state, cpu_id) \
	sec_debug_clock_log((name), (state), (cpu_id), 0)
#define sec_debug_clock_rate_complete_log(name, state, cpu_id) \
	sec_debug_clock_log((name), (state), (cpu_id), 1)
#else
#define sec_debug_cpu_lpm_log(...) do {} while(0)
#define sec_debug_cluster_lpm_log(...) do {} while(0)
#define sec_debug_clock_rate_log(...) do {} while(0)
#define sec_debug_clock_rate_complete_log(...) do {} while(0)
#endif

#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
struct sec_debug_log {
	atomic_t idx_sched[CONFIG_NR_CPUS];
	struct sched_log sched[CONFIG_NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_irq[CONFIG_NR_CPUS];
	struct irq_log irq[CONFIG_NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_secure[CONFIG_NR_CPUS];
	struct secure_log secure[CONFIG_NR_CPUS][TZ_LOG_MAX];

	atomic_t idx_irq_exit[CONFIG_NR_CPUS];
	struct irq_exit_log irq_exit[CONFIG_NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_timer[CONFIG_NR_CPUS];
	struct timer_log timer_log[CONFIG_NR_CPUS][SCHED_LOG_MAX];

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
	atomic_t idx_secmsg[CONFIG_NR_CPUS];
	struct secmsg_log secmsg[CONFIG_NR_CPUS][MSG_LOG_MAX];
#endif
#ifdef CONFIG_SEC_DEBUG_AVC_LOG
	atomic_t idx_secavc[CONFIG_NR_CPUS];
	struct secavc_log secavc[CONFIG_NR_CPUS][AVC_LOG_MAX];
#endif
#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
	atomic_t dcvs_log_idx[CONFIG_NR_CPUS] ;
	struct dcvs_debug dcvs_log[CONFIG_NR_CPUS][DCVS_LOG_MAX] ;
#endif
#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
	atomic_t fg_log_idx;
	struct fuelgauge_debug fg_log[FG_LOG_MAX] ;
#endif

#ifdef CONFIG_SEC_DEBUG_POWER_LOG
	atomic_t idx_power[CONFIG_NR_CPUS];
	struct power_log  pwr_log[CONFIG_NR_CPUS][POWER_LOG_MAX];
#endif
	/* zwei variables -- last_pet und last_ns */
	unsigned long long last_pet;
	atomic64_t last_ns;
};

#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

/* for sec debug level */
#define KERNEL_SEC_DEBUG_LEVEL_LOW	(0x574F4C44)
#define KERNEL_SEC_DEBUG_LEVEL_MID	(0x44494D44)
#define KERNEL_SEC_DEBUG_LEVEL_HIGH	(0x47494844)
#define ANDROID_DEBUG_LEVEL_LOW		0x4f4c
#define ANDROID_DEBUG_LEVEL_MID		0x494d
#define ANDROID_DEBUG_LEVEL_HIGH	0x4948

__weak void dump_all_task_info(void);
__weak void dump_cpu_stat(void);
extern void emerg_pet_watchdog(void);
#define LOCAL_CONFIG_PRINT_EXTRA_INFO

#ifdef CONFIG_SEC_DEBUG_DOUBLE_FREE
extern void *kfree_hook(void *p, void *caller);
#endif

#ifdef CONFIG_USER_RESET_DEBUG

#ifdef CONFIG_USER_RESET_DEBUG_TEST
extern void force_thermal_reset(void);
extern void force_watchdog_bark(void);
#endif

#define SEC_DEBUG_EX_INFO_SIZE	(0x400)

typedef enum {
	USER_UPLOAD_CAUSE_MIN = 1,
	USER_UPLOAD_CAUSE_SMPL = USER_UPLOAD_CAUSE_MIN,	/* RESET_REASON_SMPL */
	USER_UPLOAD_CAUSE_WTSR,			/* RESET_REASON_WTSR */
	USER_UPLOAD_CAUSE_WATCHDOG,		/* RESET_REASON_WATCHDOG */
	USER_UPLOAD_CAUSE_PANIC,		/* RESET_REASON_PANIC */
	USER_UPLOAD_CAUSE_MANUAL_RESET,	/* RESET_REASON_MANUAL_RESET */
	USER_UPLOAD_CAUSE_POWER_RESET,	/* RESET_REASON_POWER_RESET */
	USER_UPLOAD_CAUSE_REBOOT,		/* RESET_REASON_REBOOT */
	USER_UPLOAD_CAUSE_BOOTLOADER_REBOOT,/* RESET_REASON_BOOTLOADER_REBOOT */
	USER_UPLOAD_CAUSE_POWER_ON,		/* RESET_REASON_POWER_ON */
	USER_UPLOAD_CAUSE_THERMAL,		/* RESET_REASON_THERMAL_RESET */
	USER_UPLOAD_CAUSE_UNKNOWN,		/* RESET_REASON_UNKNOWN */
	USER_UPLOAD_CAUSE_MAX = USER_UPLOAD_CAUSE_UNKNOWN,
} user_upload_cause_t;

enum extra_info_dbg_type {
	DBG_0_L2_ERR = 0,
	DBG_1_RESERVED,
	DBG_2_RESERVED,
	DBG_3_RESERVED,
	DBG_4_RESERVED,
	DBG_5_RESERVED,
	DBG_MAX,
};

struct sec_debug_reset_ex_info {
	u64 ktime;
	struct task_struct *tsk;
	char bug_buf[64];
	char panic_buf[64];
	u64 fault_addr[6];
	char pc[64];
	char lr[64];
	char dbg0[96];
	char backtrace[600];
}; /* size SEC_DEBUG_EX_INFO_SIZE */

extern struct sec_debug_reset_ex_info *sec_debug_reset_ex_info;
extern int sec_debug_get_cp_crash_log(char *str);
extern void _sec_debug_store_backtrace(unsigned long where);
extern void sec_debug_store_bug_string(const char *fmt, ...);
extern void sec_debug_store_additional_dbg(enum extra_info_dbg_type type, unsigned int value, const char *fmt, ...);

static inline void sec_debug_store_pte(unsigned long addr, int idx)
{
	if (sec_debug_reset_ex_info) {
		if(idx == 0)
			memset(sec_debug_reset_ex_info->fault_addr, 0,
				sizeof(sec_debug_reset_ex_info->fault_addr));

		sec_debug_reset_ex_info->fault_addr[idx] = addr;
	}
}
#else /* CONFIG_USER_RESET_DEBUG */
static inline void sec_debug_store_pte(unsigned long addr, int idx)
{
}
#endif /* CONFIG_USER_RESET_DEBUG */

#ifdef CONFIG_TOUCHSCREEN_DUMP_MODE
struct tsp_dump_callbacks {
	void (*inform_dump)(void);
};
#endif

#endif	/* SEC_DEBUG_H */
