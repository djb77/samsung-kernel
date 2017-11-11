/*
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <linux/linkage.h>
#include <asm/rkp_cfp.h>
#include <asm/assembler.h>
#include <linux/rkp_cfp.h>

.section .hyperdrive, "ax"
.global _start_hyperdrive
_start_hyperdrive:
# Need a nop otherwise GCC keeps _start_hyperdrive label but NOT 
# jopp_springboard_bl label.
nop


/*
 * Snippets of code in the springboard must be labled with the prefix: jopp_springboard_
 * This naming convention ensures that springboards themselves aren't instrumented.
 */

#ifdef CONFIG_RKP_CFP_JOPP
	.macro zero_reg, reg, blr_reg
	.if \reg == \blr_reg
	//DON'T zero the BLR target register, we need to jump to it
	nop
	.else
	mov \reg, #0
	.endif
	.endm

/* Load the word just before the function instructions.
 * Halt execution if the word doesn't match the magic number placed at function 
 * entries.
 */
	.macro springboard_blr, reg 
	alloc_function_entry_magic_number func_entry_magic_\reg
	jopp_springboard_blr_\reg:
	ldr RRX_32, [\reg, #-4]
	cmp RRX_32, RRS_32
	b.eq 1f
	hlt #0
1:
	br \reg
	.endm

	// Only add springboards for a register rX if there exists a "BLR rX" instruction in vmlinux to instrument
	springboard_blr x0
	springboard_blr	x1
	springboard_blr	x2
	springboard_blr	x3
	springboard_blr	x4
	springboard_blr	x5
	springboard_blr	x6
	springboard_blr	x7
	springboard_blr	x8
	springboard_blr	x9
	springboard_blr	x10
	springboard_blr	x11
	springboard_blr	x12
	springboard_blr	x13
	springboard_blr	x14
	springboard_blr	x15
	/* x16, x17, x18 are reserved.
	 * Undefine ROPP will free x16, x17
	 */
#ifndef CONFIG_RKP_CFP_ROPP
	springboard_blr	x16
	springboard_blr	x17
#endif
	springboard_blr	x19
	springboard_blr	x20
	springboard_blr	x21
	springboard_blr	x22
	springboard_blr	x23
	springboard_blr	x24
	springboard_blr	x25
	springboard_blr	x26
	springboard_blr	x27
	springboard_blr	x28
	springboard_blr	x29
	springboard_blr	x30
	// Don't need springboards for x31 (not allowed in RET by assembler)

#endif
.global _end_hyperdrive
_end_hyperdrive:

.section .text
