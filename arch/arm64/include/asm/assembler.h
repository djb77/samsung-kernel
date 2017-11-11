/*
 * Based on arch/arm/include/asm/assembler.h
 *
 * Copyright (C) 1996-2000 Russell King
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
#ifndef __ASSEMBLY__
#error "Only include this from assembly code"
#endif

#ifdef CONFIG_RKP_CFP
#ifndef __ASM_ASSEMBLER_H
#define __ASM_ASSEMBLER_H
#endif	/* __ASM_ASSEMBLER_H */
#endif
#include <asm/ptrace.h>
#include <asm/thread_info.h>

#ifdef CONFIG_RKP_CFP
#include <linux/rkp_cfp.h>
#include <asm/asm-offsets.h>
#endif

#ifdef CONFIG_RKP_CFP_JOPP
/* Macros for:
 * - storing the magic function entry point number in memory
 * - loading the magic function entry point number from memory into RRS
 * Store the magic number in the text segment with this label.
 * Put a hlt instruction right after it so if an attacker tries to jump to it, they 
 * will crash the kernel.
 */
	.macro alloc_function_entry_magic_number, label
	\label:
	.word CONFIG_RKP_CFP_JOPP_MAGIC
	hlt #0
	.endm
	
	//Load the magic number from memory into RRS.
	.macro load_function_entry_magic_number, label
	ldr RRS_32, =\label
	.endm
	
	
	/* The macros below are just variations on the above two basic macros.
	 * Should work before relocation entries are adjusted.
	 */
	.macro load_function_entry_magic_number_before_reloc, label
	adr RRS, \label
	ldr RRS_32, [RRS]
	.endm
	
	.macro load_function_entry_magic_number_no_mmu, label
	load_function_entry_magic_number_before_reloc \label
	.endm
	
	.macro load_function_entry_magic_number_far_away, label
	adr_l RRS, \label
	ldr RRS_32, [RRS]
	.endm

	/*
	 * Pseudo-ops for PC-relative adr/ldr/str <reg>, <symbol> where
	 * <symbol> is within the range +/- 4 GB of the PC.
	 *
	 * @dst: destination register (64 bit wide)
	 * @sym: name of the symbol
	 * @tmp: optional scratch register to be used if <dst> == sp, which
	 *       is not allowed in an adrp instruction
	 */
	.macro	adr_l, dst, sym, tmp=
	.ifb	\tmp
	adrp	\dst, \sym
	add	\dst, \dst, :lo12:\sym
	.else
	adrp	\tmp, \sym
	add	\dst, \tmp, :lo12:\sym
	.endif
	.endm

	/*
	 * @dst: destination register (32 or 64 bit wide)
	 * @sym: name of the symbol
	 * @tmp: optional 64-bit scratch register to be used if <dst> is a
	 *       32-bit wide register, in which case it cannot be used to hold
	 *       the address
	 */
	.macro	ldr_l, dst, sym, tmp=
	.ifb	\tmp
	adrp	\dst, \sym
	ldr	\dst, [\dst, :lo12:\sym]
	.else
	adrp	\tmp, \sym
	ldr	\dst, [\tmp, :lo12:\sym]
	.endif
	.endm

	/*
	 * @src: source register (32 or 64 bit wide)
	 * @sym: name of the symbol
	 * @tmp: mandatory 64-bit scratch register to calculate the address
	 *       while <src> needs to be preserved.
	 */
	.macro	str_l, src, sym, tmp
	adrp	\tmp, \sym
	str	\src, [\tmp, :lo12:\sym]
	.endm
#endif


#ifdef CONFIG_RKP_CFP_ROPP
	.macro	get_thread_info, rd
	mov	\rd, sp
	and	\rd, \rd, #~(THREAD_SIZE - 1)	// top of stack
	.endm

	/* Load the key register (RRK) with this task's return-address encryption key.
	 * For secure, store the encrypted per thread key in rrk
	 */
	.macro load_key, tsk
#ifdef CONFIG_RKP_CFP_ROPP_HYPKEY
	push	x29, x30
	push	x0, x1
	mov	x0, #0x3000
	movk	x0, #0x8389, lsl #16
	mov	x1, \tsk
	add	x1, x1, #TI_RRK
	bl	rkp_call
	pop	x0, x1
	pop	x29, x30
#else
	ldr RRK, [\tsk, #TI_RRK]
#endif
	.endm
#endif

/*
 * Stack pushing/popping (register pairs only). Equivalent to store decrement
 * before, load increment after.
 */
	.macro	push, xreg1, xreg2
	stp	\xreg1, \xreg2, [sp, #-16]!
	.endm

	.macro	pop, xreg1, xreg2
	ldp	\xreg1, \xreg2, [sp], #16
	.endm

/*
 * Enable and disable interrupts.
 */
	.macro	disable_irq
	msr	daifset, #2
	.endm

	.macro	enable_irq
	msr	daifclr, #2
	.endm

/*
 * Save/disable and restore interrupts.
 */
	.macro	save_and_disable_irqs, olddaif
	mrs	\olddaif, daif
	disable_irq
	.endm

	.macro	restore_irqs, olddaif
	msr	daif, \olddaif
	.endm

/*
 * Enable and disable debug exceptions.
 */
	.macro	disable_dbg
	msr	daifset, #8
	.endm

	.macro	enable_dbg
	msr	daifclr, #8
	.endm

	.macro	disable_step_tsk, flgs, tmp
	tbz	\flgs, #TIF_SINGLESTEP, 9990f
	mrs	\tmp, mdscr_el1
	bic	\tmp, \tmp, #1
	msr	mdscr_el1, \tmp
	isb	// Synchronise with enable_dbg
9990:
	.endm

	.macro	enable_step_tsk, flgs, tmp
	tbz	\flgs, #TIF_SINGLESTEP, 9990f
	disable_dbg
	mrs	\tmp, mdscr_el1
	orr	\tmp, \tmp, #1
	msr	mdscr_el1, \tmp
9990:
	.endm

/*
 * Enable both debug exceptions and interrupts. This is likely to be
 * faster than two daifclr operations, since writes to this register
 * are self-synchronising.
 */
	.macro	enable_dbg_and_irq
	msr	daifclr, #(8 | 2)
	.endm

/*
 * SMP data memory barrier
 */
	.macro	smp_dmb, opt
#ifdef CONFIG_SMP
	dmb	\opt
#endif
	.endm

#define USER(l, x...)				\
9999:	x;					\
	.section __ex_table,"a";		\
	.align	3;				\
	.quad	9999b,l;			\
	.previous

/*
 * Register aliases.
 */
lr	.req	x30		// link register

/*
 * Vector entry
 */
	 .macro	ventry	label
	.align	7
	b	\label
	.endm

/*
 * Select code when configured for BE.
 */
#ifdef CONFIG_CPU_BIG_ENDIAN
#define CPU_BE(code...) code
#else
#define CPU_BE(code...)
#endif

/*
 * Select code when configured for LE.
 */
#ifdef CONFIG_CPU_BIG_ENDIAN
#define CPU_LE(code...)
#else
#define CPU_LE(code...) code
#endif

/*
 * Define a macro that constructs a 64-bit value by concatenating two
 * 32-bit registers. Note that on big endian systems the order of the
 * registers is swapped.
 */
#ifndef CONFIG_CPU_BIG_ENDIAN
	.macro	regs_to_64, rd, lbits, hbits
#else
	.macro	regs_to_64, rd, hbits, lbits
#endif
	orr	\rd, \lbits, \hbits, lsl #32
	.endm

