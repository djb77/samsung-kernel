/*
 *sec_debug_wrong_jump.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/sec_debug.h>

/* For debugging, print out all results of each branch checked */
#undef _BV_PRINT_ALL_BRANCH

static int __dump_backtrace_entry(unsigned long where)
{
	/*
	 * Note that 'where' can have a physical address, but it's not handled.
	 */
	return printk("[<%p>] %pS", (void *)where, (void *)where);
}

static int __dump_backtrace_entry_auto_summary(unsigned long where)
{
	/*
	 * Note that 'where' can have a physical address, but it's not handled.
	 */
	return pr_auto(ASL2, "[<%p>] %pS", (void *)where, (void *)where);
}

#define __virt_addr_valid(xaddr)	pfn_valid(__pa(xaddr) >> PAGE_SHIFT)

#define BL_OP_CODE	0x94000000
#define BL_OP_MASK	0xFC000000
#define BL_IMM_MASK	0x3FFFFFF
#define BLR_OP_CODE	0xD63F0000
#define BLR_OP_MASK	0xFFFFFC1F
#define IS_BL(c)	(BL_OP_CODE == ((c) & BL_OP_MASK))
#define IS_BLR(c)	(BLR_OP_CODE == ((c) & BLR_OP_MASK))

enum {
	WJ_OK = 0,
	WJ_SKIP,
	WJ_WARN,
	WJ_ERROR,
};

static inline int __in_entry_text(unsigned long ptr)
{
	return (ptr >= (unsigned long)&__entry_text_start &&
		ptr < (unsigned long)&__entry_text_end);
}

static inline int __in_init_text(unsigned long ptr)
{
	return (ptr >= (unsigned long)_sinittext &&
		ptr < (unsigned long)_einittext);
}

static inline int __is_kernel_text(unsigned long addr)
{
	return (addr >= (unsigned long)_text &&
		addr <= (unsigned long)_etext);
}

static inline unsigned long __get_bl_target(unsigned long addr)
{
	u32 code;
	s32 imm;

	code = *(u32 *)(addr);
	imm = code & BL_IMM_MASK;
	imm <<= 6;
	imm >>= 4;
	return (addr + imm);
}

#ifdef CONFIG_RKP_CFP_JOPP
extern char _start_hyperdrive[];
extern char _end_hyperdrive[];

static inline int __is_jopp_branch(unsigned long addr)
{
	unsigned long target;

	target = __get_bl_target(addr);
	return (target >= (unsigned long)_start_hyperdrive &&
		target <= (unsigned long)_end_hyperdrive);
}
#else
static inline int __is_jopp_branch(unsigned long addr) { return 0; }
#endif

/* kernel can't know symbols in fimc binary */
#define FIB_LIB_OFFSET		(VMALLOC_START + 0xF6000000 - 0x8000000)
#define FIB_LIB_START		(FIB_LIB_OFFSET + 0x04000000)
#define FIB_VRA_LIB_SIZE	(SZ_512K)
#define FIB_DDK_LIB_SIZE	(SZ_4M)
#define FIB_RTA_LIB_SIZE	(SZ_1M + SZ_2M)
#define FIB_LIB_SIZE		(FIB_VRA_LIB_SIZE + FIB_DDK_LIB_SIZE + FIB_RTA_LIB_SIZE)

static int __is_fimc_lib(unsigned long addr)
{
	return (addr >= (unsigned long)FIB_LIB_START &&
		addr < (unsigned long)(FIB_LIB_START + FIB_LIB_SIZE));
}

#ifdef _BV_PRINT_ALL_BRANCH
static inline int print_dot_cont_quiet(bool q, int prev, int printed)
{
#define MIN_DOT	1
#define DEF_DOT	5
	int min_d = MIN_DOT;
	int prev_size = prev - printed;
	int new_size;
	int i;

	if (q)
		return 0;

	if (prev && min_d < prev_size)
		new_size = prev_size;
	else
		new_size = DEF_DOT;

	for (i = 0; i < new_size; i++)
		pr_cont(".");

	return new_size;
}
#else
static inline int print_dot_cont_quiet(bool q, int prev, int printed)
{
	if (q)
		return 0;

	return pr_cont(" -----> ");
}
#endif

#define print_cont_quiet(q, fmt, ...) ({	\
	if (!q)					\
		pr_cont(fmt, ##__VA_ARGS__);	\
})

/* caller : address of bl instruction + 4 */
static int __check_calltrace(unsigned long caller, unsigned long callee, unsigned long fp, bool quiet)
{
	int result;
	u32 code;
	s32 imm;
	unsigned long target;
	unsigned long tsize, toffset;
	bool nodebug = 1;

#ifdef _BV_PRINT_ALL_BRANCH
	nodebug = quiet;
#endif

	if (__in_entry_text(callee)) {
		/* skip: callee is entry text */
		print_cont_quiet(nodebug, "(?) entry text(0x%p)\n", (void *)callee);
		result = WJ_SKIP;
		goto skip;
	}

	if (!__virt_addr_valid(caller - 4)) {
		/* skip: caller is invalid address */
		print_cont_quiet(nodebug, "(?) is invalid addr\n");
		result = WJ_SKIP;
		goto skip;
	}

	if (__in_init_text(caller)) {
		/* skip: caller is in init.text */
		print_cont_quiet(nodebug, "(?) in init.text\n");
		result = WJ_SKIP;
		goto skip;
	}

	code = *(u32 *)(caller - 4);

	if (!IS_BLR(code) && !IS_BL(code)) {
		/* error: caller is not bl/blr either */
		print_cont_quiet(quiet, "(error) not bl/blr either\n");
		result = WJ_ERROR;
		goto skip;
	}

	if (IS_BL(code) && !__is_jopp_branch(caller - 4)) {
		/* normal bl */
		imm = code & BL_IMM_MASK;
		imm <<= 6;
		imm >>= 4;
		target = caller - 4 + imm;
	} else {
#ifdef CONFIG_SEC_DEBUG_INDIRECT_BRANCH_VERIFIER
		/* check blr target */
		if (__in_entry_text(caller)) {
			/* skip: caller is blr and target is unknown */
			print_cont_quiet(nodebug, "(?) blr in entry\n");
			result = WJ_SKIP;
			goto skip;
		} else if (!__virt_addr_valid(fp)) {
			/* skip: caller is blr and target is unknown */
			print_cont_quiet(nodebug, "(?) blr\n");
			pr_err(" Error: fp(%p) is invalid\n", (void *)fp);
			result = WJ_SKIP;
			goto skip;
		}
		target = *(unsigned long *)(fp + 0x10);
#else
		/* skip: caller is blr */
		print_cont_quiet(nodebug, "(?) blr\n");
		result = WJ_SKIP;
		goto skip;
#endif
	}

	if (kallsyms_lookup_size_offset(target, &tsize, &toffset)) {
		/* check symbol matching */
		if (callee < target - toffset || callee >= target - toffset + tsize) {
#ifdef _BV_PRINT_ALL_BRANCH
			print_cont_quiet(quiet, "(Warning) branch target should be %ps()\n", (void *)target);
#else
			print_cont_quiet(quiet,
					 "\xA1\xE1\xA1\xE1\xA1\xE1\xA1\xE1\xA1\xE1 "
					 "%ps() "
					 "\xA1\xE1\xA1\xE1\xA1\xE1\xA1\xE1\xA1\xE1\n",
					 (void *)target);
#endif
			result = WJ_WARN;
		} else {
			/* ok */
			print_cont_quiet(nodebug, "(o)\n");
			result = WJ_OK;
		}

		if (toffset)
			pr_err(" Error: target(%pS) is not an entry of function\n", (void *)target);

		pr_debug("target: %p is %lu/%lu\n", (void *)target, toffset, tsize);
	} else {
		/* skip: failed to find symbol at target */
		if (__is_fimc_lib(target)) {
			print_cont_quiet(nodebug, "(?) target(0x%p) might be in fimc binary\n", (void *)target);
			result = WJ_SKIP;
		} else {
			print_cont_quiet(quiet, "(?) failed to find symbol for target(0x%p)\n", (void *)target);
			result = WJ_ERROR;
		}
	}
	pr_debug("%pS called to %pS\n", (void *)(caller - 4), (void *)target);

skip:
	/* return result */
	return result;
}

void init_branch_info(struct branch_info *info)
{
	info->lr = 0;
	info->regs_lr = 0;
	info->regs_fp = 0;
	info->state = BRSTATE_INIT;
	info->log_pos = 0;
}

void pre_check_backtrace_auto_summary(struct branch_info *info, unsigned long where, unsigned long fp)
{
	int printed;
	unsigned long pc, lr, caller;
	int warn_caller, warn_lr;

	if (info->state != BRSTATE_PCLR)
		return;

	info->state = BRSTATE_NORMAL;
	pc = info->lr;
	lr = info->regs_lr;
	caller = where;

	if (__check_calltrace(caller, pc, fp, true) != WJ_WARN)
		return;

	warn_caller = __check_calltrace(caller, lr, fp, true);

	if (warn_caller != WJ_OK)
		return;

	warn_lr = __check_calltrace(lr, pc, info->regs_fp, true);
#ifdef _BV_PRINT_ALL_BRANCH
	warn_lr = WJ_WARN;
#endif

	if (warn_lr == WJ_ERROR || warn_lr == WJ_WARN) {
		printed = __dump_backtrace_entry_auto_summary(info->regs_lr);
		printed += print_dot_cont_quiet(false, info->log_pos, printed);
		__check_calltrace(lr, pc, info->regs_fp, false);

		info->log_pos = printed;
	}

	info->lr = info->regs_lr;
}

static void __check_bl_target(struct branch_info *info, unsigned long where, unsigned long fp, unsigned int fromirq, struct pt_regs *regs, int printed)
{
	unsigned long prev;
	int warn;

	if (info->state == BRSTATE_INIT) {
		/* first line */
		pr_cont("\n");
		if (regs) {
			info->regs_lr = regs->regs[30];
			info->regs_fp = regs->regs[29];
			info->state = BRSTATE_PCLR;
		} else {
			info->state = BRSTATE_NORMAL;
		}
		info->lr = where;
		return;
	}

	if (info->state == BRSTATE_NORMAL_PC) {
		/* The value below el1_irq is not LR but PC(ELR) without LR.
		 * So we need to ignore the next branch.
		 *  ex>
		 *  |el1_irq
		 *  |rcu_idle_exit
		 *  |secondary_start_kernel : bl cpu_startup_entry
		 */
		pr_cont("\n");
		info->state = BRSTATE_NORMAL;
		info->lr = where;
		return;
	}

	prev = info->lr;

	warn = __check_calltrace(where, prev, fp, true);
#ifdef _BV_PRINT_ALL_BRANCH
	warn = WJ_WARN;
#endif

	if (warn == WJ_ERROR || warn == WJ_WARN) {
		printed += print_dot_cont_quiet(false, info->log_pos, printed);
		__check_calltrace(where, prev, fp, false);
		info->log_pos = printed;
	} else {
		pr_cont("\n");
	}

	if (fromirq)
		info->state = BRSTATE_NORMAL_PC;
	info->lr = where;
}

void check_backtrace(struct branch_info *info, unsigned long where, unsigned long fp, unsigned int fromirq, struct pt_regs *regs)
{
	int printed;

	printed = __dump_backtrace_entry(where);

	__check_bl_target(info, where, fp, fromirq, regs, printed);
}

void check_backtrace_auto_summary(struct branch_info *info, unsigned long where, unsigned long fp, unsigned int fromirq, struct pt_regs *regs)
{
	int printed;

	printed = __dump_backtrace_entry_auto_summary(where);

	__check_bl_target(info, where, fp, fromirq, regs, printed);
}
