/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Authors: 	James Gleeson <jagleeso@gmail.com>
 *		Wenbo Shen <wenbo.s@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#ifndef __RKP_CFP_H__
#define __RKP_CFP_H__

/* 
 * We've reserved x16, x17 using GCC options:
 * KCFLAGS="-ffixed-x16 -ffixed-17"
 * (RR = reserved register).
 *
 * Choose RRK to be x17, since x17 is used the least frequently in low-level assembly code already.
 */
#define RRX x16
#define RRX_32 w16
#define RRX_NUM 16

#define RRK x17
#define RRK_NUM 17

#define RRS x18
#define RRS_32 w18
#define RRS_NUM 18

#define RRMK pmccntr_el0

/* 
 * Can't include TI_RRK from asm/asm-offsets.h due to conflicts (it's meant for assembly), 
 * nor can we use offsetof.
 * Include this in asm/rkp_cfp.h and check consistency there.
 */
#define TI_RRK_AGAIN 32

#ifndef __ASSEMBLY__

struct task_struct;

//two wrappers for replace and stringify
#define _STR(x) #x
#define STR(x) _STR(x)

#ifdef CONFIG_RKP_CFP_ROPP
void ropp_change_key(struct task_struct *);
#endif // CONFIG_RKP_CFP_ROPP

#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
#define PRE_SMC_INLINE \
	"stp	" STR(RRX) ", " STR(RRS) ", [sp, #-16]!\n" \
	"dsb	sy\n" \
	"isb\n" \

#ifdef CONFIG_RKP_CFP_ROPP_HYPKEY
#define POST_SMC_INLINE \
	"dsb	sy\n" \
	"isb\n" \
	/* "get_thread_info " STR(RRK) "\n" */ \
	"mov	" STR(RRX) ", sp\n" \
	"and	" STR(RRX) ", " STR(RRX) ", #" STR(~(THREAD_SIZE - 1)) "	// top of stack\n" \
	/* load key from hypervisor */ \
	"stp	x29, x30, [sp, #-16]!\n" \
	"stp	x0, x1, [sp, #-16]!\n" \
	"mov	x0, #0x3000\n" \
	"movk	x0, #0x8389, lsl #16\n" \
	"mov	x1, " STR(RRX) "\n" \
	"add	x1, x1, #" STR(TI_RRK_AGAIN) "\n" \
	"bl	rkp_call\n" \
	"ldp	x0, x1, [sp], #16\n" \
	"ldp	x29, x30, [sp], #16\n" \
	/*load RRX and RRS*/ \
	"ldp	" STR(RRX) ", " STR(RRS) ", [sp], #16\n"
#else //CONFIG_RKP_CFP_ROPP_HYPKEY
#define POST_SMC_INLINE \
	"dsb	sy\n" \
	"isb\n" \
	/* "get_thread_info " STR(RRK) "\n" */ \
	"mov	" STR(RRX) ", sp\n" \
	"and	" STR(RRX) ", " STR(RRX) ", #" STR(~(THREAD_SIZE - 1)) "	// top of stack\n" \
	/* "load_key " STR(RRK) "\n" */ \
	"mrs	" STR(RRS) ", DAIF\n" \
	"msr	DAIFset, #0x3\n" \
	"stp	x0, x1, [sp, #-16]!\n" \
	"ldr	x0, [" STR(RRX) " , #" STR(TI_RRK_AGAIN) " ]\n" \
	"mrs	" STR(RRK) ", " STR(RRMK) "\n" \
	"eor	" STR(RRK) ", x0, " STR(RRK) " \n" \
	"ldp	x0, x1, [sp], #16\n" \
	"msr	DAIF, " STR(RRS) "\n" \
	"ldp	" STR(RRX) ", " STR(RRS) ", [sp], #16\n" 
#endif //CONFIG_RKP_CFP_ROPP_HYPKEY

#endif // CONFIG_RKP_CFP_FIX_SMC_BUG

#endif //__ASSEMBLY__

#endif //__RKP_CFP_H__
