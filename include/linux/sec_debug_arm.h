/*
 * include/linux/sec_debug_arm.h
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

#ifndef SEC_DEBUG_ARM_H
#define SEC_DEBUG_ARM_H

#if defined(CONFIG_ARM) && defined(CONFIG_SEC_DEBUG)

struct sec_debug_mmu_reg_t {
	uint32_t SCTLR;
	uint32_t TTBR0;
	uint32_t TTBR1;
	uint32_t TTBCR;
	uint32_t DACR;
	uint32_t DFSR;
	uint32_t DFAR;
	uint32_t IFSR;
	uint32_t IFAR;
	uint32_t DAFSR;
	uint32_t IAFSR;
	uint32_t PMRRR;
	uint32_t NMRRR;
	uint32_t FCSEPID;
	uint32_t CONTEXT;
	uint32_t URWTPID;
	uint32_t UROTPID;
	uint32_t POTPIDR;
};

/* ARM CORE regs mapping structure */
struct sec_debug_core_t {
	/* COMMON */
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;

	/* SVC */
	uint32_t r13_svc;
	uint32_t r14_svc;
	uint32_t spsr_svc;

	/* PC & CPSR */
	uint32_t pc;
	uint32_t cpsr;

	/* USR/SYS */
	uint32_t r13_usr;
	uint32_t r14_usr;

	/* FIQ */
	uint32_t r8_fiq;
	uint32_t r9_fiq;
	uint32_t r10_fiq;
	uint32_t r11_fiq;
	uint32_t r12_fiq;
	uint32_t r13_fiq;
	uint32_t r14_fiq;
	uint32_t spsr_fiq;

	/* IRQ */
	uint32_t r13_irq;
	uint32_t r14_irq;
	uint32_t spsr_irq;

	/* MON */
	uint32_t r13_mon;
	uint32_t r14_mon;
	uint32_t spsr_mon;

	/* ABT */
	uint32_t r13_abt;
	uint32_t r14_abt;
	uint32_t spsr_abt;

	/* UNDEF */
	uint32_t r13_und;
	uint32_t r14_und;
	uint32_t spsr_und;
};

static inline void sec_debug_save_core_reg(struct sec_debug_core_t *core_reg)
{
	/* we will be in SVC mode when we enter this function. Collect
	   SVC registers along with cmn registers. */
	asm volatile (
		"str r0, [%0,#0]\n\t"		/* R0 is pushed first to core_reg */
		"mov r0, %0\n\t"		/* R0 will be alias for core_reg */
		"str r1, [r0,#4]\n\t"		/* R1 */
		"str r2, [r0,#8]\n\t"		/* R2 */
		"str r3, [r0,#12]\n\t"		/* R3 */
		"str r4, [r0,#16]\n\t"		/* R4 */
		"str r5, [r0,#20]\n\t"		/* R5 */
		"str r6, [r0,#24]\n\t"		/* R6 */
		"str r7, [r0,#28]\n\t"		/* R7 */
		"str r8, [r0,#32]\n\t"		/* R8 */
		"str r9, [r0,#36]\n\t"		/* R9 */
		"str r10, [r0,#40]\n\t"		/* R10 */
		"str r11, [r0,#44]\n\t"		/* R11 */
		"str r12, [r0,#48]\n\t"		/* R12 */

		/* SVC */
		"str r13, [r0,#52]\n\t"		/* R13_SVC */
		"str r14, [r0,#56]\n\t"		/* R14_SVC */
		"mrs r1, spsr\n\t"		/* SPSR_SVC */
		"str r1, [r0,#60]\n\t"

		/* PC and CPSR */
		"sub r1, r15, #0x4\n\t"		/* PC */
		"str r1, [r0,#64]\n\t"
		"mrs r1, cpsr\n\t"		/* CPSR */
		"str r1, [r0,#68]\n\t"

		/* SYS/USR */
		"mrs r1, cpsr\n\t"		/* switch to SYS mode */
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x1f\n\t"
		"msr cpsr,r1\n\t"
		"str r13, [r0,#72]\n\t"		/* R13_USR */
		"str r14, [r0,#76]\n\t"		/* R14_USR */

		/* FIQ */
		"mrs r1, cpsr\n\t"		/* switch to FIQ mode */
		"and r1,r1,#0xFFFFFFE0\n\t"
		"orr r1,r1,#0x11\n\t"
		"msr cpsr,r1\n\t"
		"str r8, [r0,#80]\n\t"		/* R8_FIQ */
		"str r9, [r0,#84]\n\t"		/* R9_FIQ */
		"str r10, [r0,#88]\n\t"		/* R10_FIQ */
		"str r11, [r0,#92]\n\t"		/* R11_FIQ */
		"str r12, [r0,#96]\n\t"		/* R12_FIQ */
		"str r13, [r0,#100]\n\t"	/* R13_FIQ */
		"str r14, [r0,#104]\n\t"	/* R14_FIQ */
		"mrs r1, spsr\n\t"		/* SPSR_FIQ */
		"str r1, [r0,#108]\n\t"

		/* IRQ */
		"mrs r1, cpsr\n\t"		/* switch to IRQ mode */
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x12\n\t"
		"msr cpsr,r1\n\t"
		"str r13, [r0,#112]\n\t"	/* R13_IRQ */
		"str r14, [r0,#116]\n\t"	/* R14_IRQ */
		"mrs r1, spsr\n\t"		/* SPSR_IRQ */
		"str r1, [r0,#120]\n\t"

		/* MON */
#if 0		/* To-Do */
		"mrs r1, cpsr\n\t"		/* switch to monitor mode */
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x16\n\t"
		"msr cpsr,r1\n\t"
		"str r13, [r0,#124]\n\t"	/* R13_MON */
		"str r14, [r0,#128]\n\t"	/* R14_MON */
		"mrs r1, spsr\n\t"		/* SPSR_MON */
		"str r1, [r0,#132]\n\t"
#endif

		/* ABT */
		"mrs r1, cpsr\n\t"		/* switch to Abort mode */
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x17\n\t"
		"msr cpsr,r1\n\t"
		"str r13, [r0,#136]\n\t"	/* R13_ABT */
		"str r14, [r0,#140]\n\t"	/* R14_ABT */
		"mrs r1, spsr\n\t"		/* SPSR_ABT */
		"str r1, [r0,#144]\n\t"

		/* UND */
		"mrs r1, cpsr\n\t"		/* switch to undef mode */
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x1B\n\t"
		"msr cpsr,r1\n\t"
		"str r13, [r0,#148]\n\t"	/* R13_UND */
		"str r14, [r0,#152]\n\t"	/* R14_UND */
		"mrs r1, spsr\n\t"		/* SPSR_UND */
		"str r1, [r0,#156]\n\t"

		/* restore to SVC mode */
		"mrs r1, cpsr\n\t"		/* switch to SVC mode */
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x13\n\t"
		"msr cpsr,r1\n\t" :		/* output */
		: "r"(core_reg)			/* input */
		: "%r0", "%r1"			/* clobbered registers */
	);

	return;
}

static void sec_debug_save_mmu_reg(struct sec_debug_mmu_reg_t *mmu_reg)
{
	asm volatile (
		"mrc p15, 0, r1, c1, c0, 0\n\t"	/* SCTLR */
		"str r1, [%0]\n\t"
		"mrc p15, 0, r1, c2, c0, 0\n\t"	/* TTBR0 */
		"str r1, [%0,#4]\n\t"
		"mrc p15, 0, r1, c2, c0,1\n\t"	/* TTBR1 */
		"str r1, [%0,#8]\n\t"
		"mrc p15, 0, r1, c2, c0,2\n\t"	/* TTBCR */
		"str r1, [%0,#12]\n\t"
		"mrc p15, 0, r1, c3, c0,0\n\t"	/* DACR */
		"str r1, [%0,#16]\n\t"
		"mrc p15, 0, r1, c5, c0,0\n\t"	/* DFSR */
		"str r1, [%0,#20]\n\t"
		"mrc p15, 0, r1, c6, c0,0\n\t"	/* DFAR */
		"str r1, [%0,#24]\n\t"
		"mrc p15, 0, r1, c5, c0,1\n\t"	/* IFSR */
		"str r1, [%0,#28]\n\t"
		"mrc p15, 0, r1, c6, c0,2\n\t"	/* IFAR */
		"str r1, [%0,#32]\n\t"
		/* Don't populate DAFSR and RAFSR */
		"mrc p15, 0, r1, c10, c2,0\n\t"	/* PMRRR */
		"str r1, [%0,#44]\n\t"
		"mrc p15, 0, r1, c10, c2,1\n\t"	/* NMRRR */
		"str r1, [%0,#48]\n\t"
		"mrc p15, 0, r1, c13, c0,0\n\t"	/* FCSEPID */
		"str r1, [%0,#52]\n\t"
		"mrc p15, 0, r1, c13, c0,1\n\t"	/* CONTEXT */
		"str r1, [%0,#56]\n\t"
		"mrc p15, 0, r1, c13, c0,2\n\t"	/* URWTPID */
		"str r1, [%0,#60]\n\t"
		"mrc p15, 0, r1, c13, c0,3\n\t"	/* UROTPID */
		"str r1, [%0,#64]\n\t"
		"mrc p15, 0, r1, c13, c0,4\n\t"	/* POTPIDR */
		"str r1, [%0,#68]\n\t" :		/* output */
		: "r"(mmu_reg)			/* input */
		: "%r1", "memory"			/* clobbered register */
	);
}

#endif /* defined(CONFIG_ARM) && defined(CONFIG_SEC_DEBUG) */

#endif /* SEC_DEBUG_ARM64_H */
