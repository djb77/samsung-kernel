/*
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_HW_BREAKPOINT_H
#define __ASM_HW_BREAKPOINT_H

#include <asm/cputype.h>
#include <asm/cpufeature.h>

#ifdef __KERNEL__

struct arch_hw_breakpoint_ctrl {
#ifdef CONFIG_SEC_KWATCHER
	u32 __reserved  : 3,
	mask	 : 5,
	link_type		 : 4,
	lbn		 : 4,
	__reserved2  : 3,
#else
	u32 __reserved	: 19,
#endif
	len		: 8,
	type		: 2,
	privilege	: 2,
	enabled		: 1;
};

struct arch_hw_breakpoint {
	u64 address;
	u64 trigger;
	struct arch_hw_breakpoint_ctrl ctrl;
};

static inline u32 encode_ctrl_reg(struct arch_hw_breakpoint_ctrl ctrl)
{
#ifdef CONFIG_SEC_KWATCHER
	return (ctrl.mask << 24) | (ctrl.link_type << 20) | (ctrl.lbn << 16) | (ctrl.len << 5) | (ctrl.type << 3) | (ctrl.privilege << 1) |
		ctrl.enabled;
#else
	return (ctrl.len << 5) | (ctrl.type << 3) | (ctrl.privilege << 1) |
		ctrl.enabled;
#endif
}

static inline void decode_ctrl_reg(u32 reg,
				   struct arch_hw_breakpoint_ctrl *ctrl)
{
	ctrl->enabled	= reg & 0x1;
	reg >>= 1;
	ctrl->privilege	= reg & 0x3;
	reg >>= 2;
	ctrl->type	= reg & 0x3;
	reg >>= 2;
	ctrl->len	= reg & 0xff;
#ifdef CONFIG_SEC_KWATCHER
	reg >>= 8;
	ctrl->mask = reg & (0x1F << 24);
	reg >>= 5;
	ctrl->link_type = reg & (0xF << 20);
	reg >>= 4;
	ctrl->lbn = reg & (0xF << 16);
#endif
}

/* Breakpoint */
#define ARM_BREAKPOINT_EXECUTE	0

/* Watchpoints */
#define ARM_BREAKPOINT_LOAD	1
#define ARM_BREAKPOINT_STORE	2
#define AARCH64_ESR_ACCESS_MASK	(1 << 6)

/* Privilege Levels */
#define AARCH64_BREAKPOINT_EL1	1
#define AARCH64_BREAKPOINT_EL0	2

/* Lengths */
#define ARM_BREAKPOINT_LEN_1	0x1
#define ARM_BREAKPOINT_LEN_2	0x3
#define ARM_BREAKPOINT_LEN_3	0x7
#define ARM_BREAKPOINT_LEN_4	0xf
#define ARM_BREAKPOINT_LEN_5	0x1f
#define ARM_BREAKPOINT_LEN_6	0x3f
#define ARM_BREAKPOINT_LEN_7	0x7f
#define ARM_BREAKPOINT_LEN_8	0xff

/* Kernel stepping */
#define ARM_KERNEL_STEP_NONE	0
#define ARM_KERNEL_STEP_ACTIVE	1
#define ARM_KERNEL_STEP_SUSPEND	2

/*
 * Limits.
 * Changing these will require modifications to the register accessors.
 */
#define ARM_MAX_BRP		16
#define ARM_MAX_WRP		16

/* Virtual debug register bases. */
#define AARCH64_DBG_REG_BVR	0
#define AARCH64_DBG_REG_BCR	(AARCH64_DBG_REG_BVR + ARM_MAX_BRP)
#define AARCH64_DBG_REG_WVR	(AARCH64_DBG_REG_BCR + ARM_MAX_BRP)
#define AARCH64_DBG_REG_WCR	(AARCH64_DBG_REG_WVR + ARM_MAX_WRP)

/* Debug register names. */
#define AARCH64_DBG_REG_NAME_BVR	"bvr"
#define AARCH64_DBG_REG_NAME_BCR	"bcr"
#define AARCH64_DBG_REG_NAME_WVR	"wvr"
#define AARCH64_DBG_REG_NAME_WCR	"wcr"

/* Accessor macros for the debug registers. */
#define AARCH64_DBG_READ(N, REG, VAL) do {\
	asm volatile("mrs %0, dbg" REG #N "_el1" : "=r" (VAL));\
} while (0)

#define AARCH64_DBG_WRITE(N, REG, VAL) do {\
	asm volatile("msr dbg" REG #N "_el1, %0" :: "r" (VAL));\
} while (0)

struct task_struct;
struct notifier_block;
struct perf_event;
struct pmu;

extern int arch_bp_generic_fields(struct arch_hw_breakpoint_ctrl ctrl,
				  int *gen_len, int *gen_type, int *offset);
extern int arch_check_bp_in_kernelspace(struct perf_event *bp);
extern int arch_validate_hwbkpt_settings(struct perf_event *bp);
extern int hw_breakpoint_exceptions_notify(struct notifier_block *unused,
					   unsigned long val, void *data);

extern int arch_install_hw_breakpoint(struct perf_event *bp);
extern void arch_uninstall_hw_breakpoint(struct perf_event *bp);
extern void hw_breakpoint_pmu_read(struct perf_event *bp);
extern int hw_breakpoint_slots(int type);

#ifdef CONFIG_SEC_KWATCHER
int arch_update_hw_breakpoint(struct perf_event *bp);
extern void read_arg(void);
extern void read_arg2(void);
#endif

#ifdef CONFIG_HAVE_HW_BREAKPOINT
extern void hw_breakpoint_thread_switch(struct task_struct *next);
extern void ptrace_hw_copy_thread(struct task_struct *task);
#else
static inline void hw_breakpoint_thread_switch(struct task_struct *next)
{
}
static inline void ptrace_hw_copy_thread(struct task_struct *task)
{
}
#endif

extern struct pmu perf_ops_bp;

/* Determine number of BRP registers available. */
static inline int get_num_brps(void)
{
	u64 dfr0 = read_system_reg(SYS_ID_AA64DFR0_EL1);
	return 1 +
		cpuid_feature_extract_unsigned_field(dfr0,
						ID_AA64DFR0_BRPS_SHIFT);
}

/* Determine number of WRP registers available. */
static inline int get_num_wrps(void)
{
	u64 dfr0 = read_system_reg(SYS_ID_AA64DFR0_EL1);
	return 1 +
		cpuid_feature_extract_unsigned_field(dfr0,
						ID_AA64DFR0_WRPS_SHIFT);
}

#endif	/* __KERNEL__ */
#endif	/* __ASM_BREAKPOINT_H */
