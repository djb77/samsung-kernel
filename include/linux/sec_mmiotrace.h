
#ifndef __LINUX_SEC_MMIOTRACE_H
#define __LINUX_SEC_MMIOTRACE_H

/* FEATURE DEFINE */
#define PERFORMANCE_PROFILE				0
#define SAVE_CALLSTACK					0
#define MMIOTRACE_SFR_NAME

#define	MMIOTRACE_LOG_BUFFER_SIZE			51200
#define	MMIOTRACE_START_ADDR				0x10000000
#define	MMIOTRACE_END_ADDR				0x16800000

#define MMIOTRACE_STATE_NONE	(0x0)
#define MMIOTRACE_STATE_PAGE	(0x1 << 0)
#define MMIOTRACE_STATE_RANGE	(0x1 << 1)

#define MMIOTRACE_INVALID_PAGE	(0x1)

#define MMIOTRACE_TYPE_NONE	(0x0)
#define MMIOTRACE_TYPE_LOAD	(0x1 << 0)
#define MMIOTRACE_TYPE_STORE	(0x1 << 1)

#define LOGINFO_CORE_SHIFT	(2)
#define LOGINFO_TYPE_SHIFT	(0)

#define LOGINFO_CORE_MASK	(0x7 << LOGINFO_CORE_SHIFT)
#define LOGINFO_TYPE_MASK	(0x3 << LOGINFO_TYPE_SHIFT)

#if PERFORMANCE_PROFILE
#define	MMIOTRACE_PROF_BUFFER_SIZE		5000
#endif

#define MMIOTRACE_PSTATE_I		(0x1 << 7)
#define MMIOTRACE_PSTATE_D		(0x1 << 9)
#define MMIOTRACE_SAVED_PSTATE_MASK	(MMIOTRACE_PSTATE_I | MMIOTRACE_PSTATE_D)
#define MMIOTRACE_PSTATE_TARGET		(0x1<<7 | 0x0<<9) /* I:1, D:0 */

struct mmiotrace_loginfo {
	u64 ts_nsec;		/* time_stamp */
	u32 loginfo;		/* type + core */
	u32 io_data;		/* data sent to or recive from a register */
	u64 pc;	 /* pc of an instruction which trys to access mmio region */
	u64 virt_addr;	/* virtual address of the region which a cpu trys to access */
	phys_addr_t phys_addr;	/* physical address */
#if SAVE_CALLSTACK
	u64 callstack[4];
#endif
};

struct mmiotrace_faultinfo {
	u64 addr;
	u64 size;
	u32 type;
	int state;	/* fault_state */
	u64 saved_pstate;
	int pstate_changed;
	u32 index;	/* index of log_buff where this log is written into */
	u64 fcnt;	/* count of the number of fault handler call */
	u64 scnt;	/* count of the number of single-step handler call */
	unsigned int esr;
};

struct mmiotrace_addrinfo {
	struct list_head list;
	const char *name;
	phys_addr_t phys_addr;
	u64 virt_addr;
	u64 size;
	u32 type;
	int state;	/* 0:inactive , 1:activated, 2:activation pended */
};

struct mmiotrace_ioremap_addr {
	struct list_head list;
	const char *name;
	phys_addr_t phys_addr;
	unsigned long virt_addr;
	size_t size;
};

#if PERFORMANCE_PROFILE
struct mmiotrace_profinfo {
	int cur_loginfo;
	u64 t[11];
};
#endif

extern int mmiotrace_do_fault_handler(unsigned long virt_addr, unsigned int esr,
				      struct pt_regs *regs);
extern void mmiotrace_add_ioremap(phys_addr_t phys_addr,
				  unsigned long virt_addr, size_t size);
extern void mmiotrace_remove_ioremap(unsigned long virt_addr);
extern void sec_mmiotrace_stop(void);
extern int set_memory_valid_n(unsigned long addr, int numpages);
extern int set_memory_invalid_n(unsigned long addr, int numpages);
#endif /* __LINUX_SEC_MMIOTRACE_H */
