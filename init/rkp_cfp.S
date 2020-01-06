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
#include <asm/assembler.h>

.section .hyperdrive, "ax"
.global _start_hyperdrive
_start_hyperdrive:
# Need a nop otherwise GCC keeps _start_hyperdrive label but NOT
# jopp_springboard_bl label.
nop

#define RRX x16
#define RRX_32 w16

#define RRS x18
#define RRS_32 w18

#define ALLOW_FIMC

#ifdef CONFIG_RKP_CFP_JOPP

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
 * Load the word just before the function instructions.
 * Halt execution if the word doesn't match the magic number placed at function
 * entries.
 */
	.macro	springboard_blr, reg
	jopp_springboard_blr_\reg:
	push	RRX, RRS
	ldr 	RRX_32, [\reg, #-4]
	subs	RRX_32, RRX_32, #0xbe7, lsl #12
	cmp	RRX_32, #0xbad
	b.eq 	1f

#ifdef ALLOW_FIMC
	// DDK/VRA/RTA	0xffffff80fa000000 ~ 0xffffff80fa5fffff
	mov	RRX, #0xffffff8000000000
	movk	RRX, 0xfa00, lsl #16
	cmp 	\reg, RRX
	b.ls	2f
	movk	RRX, #0xfa60, lsl #16
	cmp 	\reg, RRX
	b.hs	2f
#else
	.inst	0xdeadc0de //crash for sure
#endif
1:
	pop	RRX, RRS
	br 	\reg
2:
	.inst	0xdeadc0de //crash for sure
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
	springboard_blr	x16
	springboard_blr	x17
	springboard_blr	x18
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
