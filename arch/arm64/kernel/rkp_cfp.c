/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 * Authors: 	James Gleeson <jagleeso@gmail.com>
 *		Wenbo Shen <wenbo.s@samsung.com>
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

#include <linux/module.h>
#include <asm/stacktrace.h>
#include <asm/insn.h>
#include <linux/rkp_cfp.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/kallsyms.h>

unsigned long dump_stack_dec=0x1;
EXPORT_SYMBOL(dump_stack_dec); //remove this line after LKM test

/* We want to start walking the stackframe from within the caller; don't make a new stackframe.
 * 
 * continue_fn returns 0 if the current frame (it's fp/pc/sp) indicates we should stop.
 * 
 * NOTE:
 * For now, we walk all except the very last stackframe (i.e. where EL1_LR is stored).
 * This is because we aren't instrumenting eret yet.
 * See entry.S for a more in depth discussion.
 */

#define __unwind_frame(frame) \
({ \
	unsigned long fp = (frame)->fp; \
	(frame)->sp = fp + 0x10; \
	(frame)->fp = *(unsigned long *)(fp); \
	/* \
	 * -4 here because we care about the PC at time of bl, \
	 * not where the return will go. \
	 */ \
	(frame)->pc = *(unsigned long *)(fp + 8) - 4; \
})

#define walk_current_stackframe(tsk, fn, data, continue_fn) \
({ \
 	struct stackframe frame; \
 	const register unsigned long current_sp asm ("sp"); \
 \
	if (tsk == current) { \
		frame.fp = (unsigned long)__builtin_frame_address(0); \
		frame.sp = current_sp; \
		frame.pc = (unsigned long)reencrypt_return_addresses; \
	} else { \
		frame.fp = thread_saved_fp(tsk); \
		frame.sp = thread_saved_sp(tsk); \
		frame.pc = thread_saved_pc(tsk); \
	} \
 \
	while (1) { \
	if (!continue_fn(tsk, &frame)) \
		break; \
		if (fn(&frame, data)) \
			break; \
		__unwind_frame(&frame); \
	} \
})

int continue_reenc_fp(struct task_struct * tsk, struct stackframe * frame)
{
	unsigned long high, low;
	unsigned long fp = frame->fp;
	low = ((unsigned long)task_thread_info(tsk) + sizeof(struct thread_info));
	high = ALIGN(low, THREAD_SIZE);
	//Does not work on exynos (with constant non-zero key) without the fp stack restriction.
	return (fp >= low) && (fp <= high - 0x18) && 
		((void *)frame->fp != NULL) && (frame->fp >= PAGE_OFFSET);
}

int return_addr_eor(struct stackframe * frame, unsigned long key2_eor)
{
	unsigned long * LR = (unsigned long *)(frame->fp + 8);
	*LR = *LR ^ key2_eor;
	return 0;
}

void reencrypt_task_return_addresses(struct task_struct * tsk, unsigned long key2_eor)
{
	unsigned long flags;
	local_irq_save(flags);
	walk_current_stackframe(tsk, return_addr_eor, key2_eor, continue_reenc_fp);
	local_irq_restore(flags);
}

void reencrypt_return_addresses(unsigned long key2_eor)
{
	reencrypt_task_return_addresses(current, key2_eor);
}

/*#if defined(CONFIG_BPF) && defined(CONFIG_RKP_CFP_JOPP) */
/* Ignore BPF if present on QEMU... it's hard to disable.
 */
/*#error "Cannot enable CONFIG_BPF when CONFIG_RKP_CFP_JOPP is on x(see CONFIG_RKP_CFP_JOPP for details))"*/
/*#endif*/

#ifdef CONFIG_RKP_CFP_ROPP_HYPKEY //function define
static void hyp_change_keys(struct task_struct *p){
	/*
	 * Hypervisor generate new key, load key to RRK, 
	 * store enc_key to thread_info
	 */
	unsigned long va, key2_eor = 0x0, is_current=0x0;

	if (!p) {
		p = current;
		is_current = 0x1;
	}
	va = (unsigned long) &(task_thread_info(p)->rrk);

	rkp_call(CFP_ROPP_NEW_KEY_REENC, va, is_current, 0, 0, 0);

	asm("mov %0, x16" : "=r" (key2_eor));
	reencrypt_task_return_addresses(p, key2_eor);
}

#else //function define
static void non_hyp_change_keys(struct task_struct *p){
	unsigned long old_key = 0x0, new_key = 0x0;

	if (!p) {
		p = current;
	}

	old_key = task_thread_info(p)->rrk;
#ifdef CONFIG_RKP_CFP_ROPP_RANDOM_KEY
	asm("mrs %0, cntpct_el0" : "=r" (new_key));
#else
	new_key = 0x0;
#endif
	
	//we can update key now
	task_thread_info(p)->rrk = new_key;
	if (p == current)
		asm("mov x17,  %0" : "=r" (new_key));
	reencrypt_task_return_addresses(p, old_key ^ new_key);
}
#endif //function define

void rkp_cfp_ropp_change_keys(struct task_struct *p){
#ifdef CONFIG_RKP_CFP_ROPP_HYPKEY
	hyp_change_keys(p);
#else
	non_hyp_change_keys(p);
#endif
}
