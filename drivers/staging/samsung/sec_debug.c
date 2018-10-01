/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <asm/cacheflush.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/kmsg_dump.h>
#include <linux/kallsyms.h>
#include <linux/kernel_stat.h>
#include <linux/tick.h>
#include <linux/module.h>
#include <linux/memblock.h>
#include <linux/sec_debug.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/mount.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/cpu.h>

#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-powermode.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <linux/exynos-ss.h>
#include <asm/stacktrace.h>
#include <linux/io.h>

#include <asm/sections.h>

#define EXYNOS_INFORM2 0x808
#define EXYNOS_INFORM3 0x80c
extern unsigned int get_smpl_warn_number(void);

//#include <mach/regs-pmu.h>

#if defined(CONFIG_SEC_DUMP_SUMMARY)
struct sec_debug_summary *summary_info=0;
static char *sec_summary_log_buf;
static unsigned long sec_summary_log_size;
static unsigned long reserved_out_buf=0;
static unsigned long reserved_out_size=0;
static char *last_summary_buffer;
static size_t last_summary_size;
#endif

#if defined(CONFIG_KFAULT_AUTO_SUMMARY)
static struct sec_debug_auto_summary* auto_summary_info = 0;
static char *auto_comment_buf = 0;

struct auto_summary_log_map {
	char prio_level;
	char max_count;
};

static const struct auto_summary_log_map init_data[SEC_DEBUG_AUTO_COMM_BUF_SIZE]
	= {{PRIO_LV0, 0}, {PRIO_LV5, 8}, {PRIO_LV9, 0}, {PRIO_LV5, 0}, {PRIO_LV5, 0}, 
	{PRIO_LV1, 1}, {PRIO_LV2, 2}, {PRIO_LV5, 0}, {PRIO_LV5, 8}, {PRIO_LV0, 0}};

extern void register_set_auto_comm_buf(void (*func)(int type, const char *buf, size_t size));
extern void register_set_auto_comm_lastfreq(void (*func)(int type, int old_freq, int new_freq, u64 time));
#endif

#ifdef CONFIG_SEC_DEBUG
/* enable/disable sec_debug feature
 * level = 0 when enable = 0 && enable_user = 0
 * level = 1 when enable = 1 && enable_user = 0
 * level = 0x10001 when enable = 1 && enable_user = 1
 * The other cases are not considered
 */
union sec_debug_level_t {
	struct {
		u16 kernel_fault;
		u16 user_fault;
	} en;
	u32 uint_val;
} sec_debug_level = { .en.kernel_fault = 1, };
module_param_named(enable, sec_debug_level.en.kernel_fault, ushort, 0644);
module_param_named(enable_user, sec_debug_level.en.user_fault, ushort, 0644);
module_param_named(level, sec_debug_level.uint_val, uint, 0644);

#define MS_TO_NS(ms) (ms * 1000 * 1000)
#define HARD_RESET_KEY 0x3

static struct hrtimer hardkey_triger_timer;
static enum hrtimer_restart Hard_Reset_Triger_callback(struct hrtimer *hrtimer);

int sec_debug_get_debug_level(void)
{
	return sec_debug_level.uint_val;
}

static void sec_debug_user_fault_dump(void)
{
	if (sec_debug_level.en.kernel_fault == 1
	    && sec_debug_level.en.user_fault == 1)
		panic("User Fault");
}

static ssize_t
sec_debug_user_fault_write(struct file *file, const char __user *buffer,
			   size_t count, loff_t *offs)
{
	char buf[100];

	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;
	buf[count] = '\0';

	if (strncmp(buf, "dump_user_fault", 15) == 0)
		sec_debug_user_fault_dump();

	return count;
}

static const struct file_operations sec_debug_user_fault_proc_fops = {
	.write = sec_debug_user_fault_write,
};

static int __init sec_debug_user_fault_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("user_fault", S_IWUSR | S_IWGRP, NULL,
			    &sec_debug_user_fault_proc_fops);
	if (!entry)
		return -ENOMEM;
	return 0;
}
device_initcall(sec_debug_user_fault_init);

static struct sec_debug_panic_extra_info panic_extra_info;
extern struct exynos_chipid_info exynos_soc_info;

enum sec_debug_upload_magic_t {
	UPLOAD_MAGIC_INIT		= 0x0,
	UPLOAD_MAGIC_PANIC		= 0x66262564,
};

#ifdef CONFIG_USER_RESET_DEBUG
enum sec_debug_reset_reason_t {
	RR_S = 1,
	RR_W = 2,
	RR_D = 3,
	RR_K = 4,
	RR_M = 5,
	RR_P = 6,
	RR_R = 7,
	RR_B = 8,
	RR_N = 9,
	RR_T = 10,
};

static unsigned reset_reason = RR_N;
module_param_named(reset_reason, reset_reason, uint, 0644);
#endif

/* timeout for dog bark/bite */
#define DELAY_TIME 30000
#define EXYNOS_PS_HOLD_CONTROL 0x330c

#ifdef CONFIG_HOTPLUG_CPU
static void pull_down_other_cpus(void)
{
	int cpu;

	for_each_online_cpu(cpu) {
		if (cpu == 0)
			continue;
		cpu_down(cpu);
	}
}
#else
static void pull_down_other_cpus(void)
{
}
#endif

static void simulate_wdog_reset(void)
{
	pull_down_other_cpus();
	pr_emerg("Simulating watch dog bite\n");
	local_irq_disable();
	mdelay(DELAY_TIME);
	local_irq_enable();
	/* if we reach here, simulation had failed */
	pr_emerg("Simualtion of watch dog bite failed\n");
}

static void simulate_wtsr(void)
{
	unsigned int ps_hold_control;

	pr_emerg("%s: set PS_HOLD low\n", __func__);

	/* power off code
	* PS_HOLD Out/High -->
	* Low PS_HOLD_CONTROL, R/W, 0x1002_330C
	*/
	exynos_pmu_read(EXYNOS_PS_HOLD_CONTROL, &ps_hold_control);
	exynos_pmu_write(EXYNOS_PS_HOLD_CONTROL, ps_hold_control & 0xFFFFFEFF);
}

#define EXYNOS_EMUL_DATA_MASK	0xFF
#define THER_EMUL_CON 0x160
#define THER_EMUL_TEMP 0x7
#define THER_EMUL_ENABLE 0x1
#define FIRST_TRIM 25
#define SECOND_TRIM 85

extern unsigned long base_addr[10];

static void simulate_thermal_reset(void)
{
	unsigned int val;

	val = readl((void __iomem *)base_addr[0] + THER_EMUL_CON);

	val &= ~(EXYNOS_EMUL_DATA_MASK << THER_EMUL_TEMP);
	val |= (0x1ff << THER_EMUL_TEMP) | THER_EMUL_ENABLE;

	writel(val, (void __iomem *)base_addr[0] + THER_EMUL_CON);
}

static int force_error(const char *val, struct kernel_param *kp)
{
	pr_emerg("!!!WARN forced error : %s\n", val);

	if (!strncmp(val, "KP", 2)) {
		pr_emerg("Generating a data abort exception!\n");
		*(unsigned int *)0x0 = 0x0; /* SVACE: intended */
	} else if (!strncmp(val, "WP", 2)) {
		simulate_wtsr();
	} else if (!strncmp(val, "DP", 2)) {
		simulate_wdog_reset();
	} else if (!strncmp(val, "TP", 2)) {
		simulate_thermal_reset();
	} else if (!strncmp(val, "pabort", 6)) {
		pr_emerg("Generating a prefetch abort exception!\n");
		((void (*)(void))0x0)(); /* SVACE: intended */
	} else if (!strncmp(val, "undef", 5)) {
		pr_emerg("Generating a undefined instruction exception!\n");
		BUG();
	} else if (!strncmp(val, "dblfree", 7)) {
		void *p = kmalloc(sizeof(int), GFP_KERNEL);

		kfree(p);
		msleep(1000);
		kfree(p); /* SVACE: intended */
	} else if (!strncmp(val, "danglingref", 11)) {
		unsigned int *p = kmalloc(sizeof(int), GFP_KERNEL);

		kfree(p);
		*p = 0x1234; /* SVACE: intended */
	} else if (!strncmp(val, "lowmem", 6)) {
		int i = 0;

		pr_emerg("Allocating memory until failure!\n");
		while (kmalloc(128*1024, GFP_KERNEL))
			i++;
		pr_emerg("Allocated %d KB!\n", i * 128);

	} else if (!strncmp(val, "memcorrupt", 10)) {
		int *ptr = kmalloc(sizeof(int), GFP_KERNEL);

		*ptr++ = 4;
		*ptr = 2;
		panic("MEMORY CORRUPTION");
	} else if (!strncmp(val, "pageRDcheck", 11)) {
		struct page *page = alloc_pages(GFP_ATOMIC, 0);
		unsigned int *ptr = (unsigned int *)page_address(page);

		pr_emerg("Test with RD page configue");
		__free_pages(page, 0);
		*ptr = 0x12345678;
	} else {
		pr_emerg("No such error defined for now!\n");
	}

	return 0;
}

module_param_call(force_error, force_error, NULL, NULL, 0644);

static void sec_debug_init_panic_extra_info(void)
{
	int i;

	memset(&panic_extra_info, 0, sizeof(struct sec_debug_panic_extra_info));
	
	panic_extra_info.fault_addr = -1;
	panic_extra_info.pc = -1;
	panic_extra_info.lr = -1;

	for(i = 0; i < INFO_MAX; i++)
		strncpy(panic_extra_info.extra_buf[i], "N/A", 3);
	
	strncpy(panic_extra_info.backtrace, "N/A\",\n", 6);
}

static void sec_debug_store_panic_extra_info(char* str)
{
	/* store panic extra info
			"KTIME":""		: kernel time
			"FAULT":""		: pgd,va,*pgd,*pud,*pmd,*pte
			"BUG":""			: bug msg
			"PANIC":""		: panic buffer msg
			"PC":"" 			: pc val
			"LR":"" 			: link register val
			"STACK":""		: backtrace
			"CHIPID":"" 		: CPU Serial Number
			"DBG0":""		: EVT version
			"DBG1":""		: SYS MMU fault
			"DBG2":""		: BUS MON Error
			"DBG3":""		: dpm timeout
			"DBG4":""		: SMPL count
			"DBG5":""		: syndrome Register
	*/
	int i;
	int panic_string_offset = 0;
	int cp_len;
	unsigned long rem_nsec;
	u64 ts_nsec = local_clock();
	
	rem_nsec = do_div(ts_nsec, 1000000000);

	cp_len = strlen(str);
	if(str[cp_len-1] == '\n')
		str[cp_len-1] = '\0';

	memset((void*)SEC_DEBUG_EXTRA_INFO_VA, 0, SZ_1K);

	panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA, BUF_SIZE_MARGIN,
		"\"KTIME\":\"%lu.%06lu\",\n", (unsigned long)ts_nsec, rem_nsec / 1000);	

	if(panic_extra_info.fault_addr != -1)
		panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset, 
			BUF_SIZE_MARGIN - panic_string_offset,
			"\"FAULT\":\"0x%lx\",\n", panic_extra_info.fault_addr);
	else
		panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset, 
			BUF_SIZE_MARGIN - panic_string_offset,
			"\"FAULT\":\"\",\n");

	panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
		BUF_SIZE_MARGIN - panic_string_offset,
		"\"BUG\":\"%s\",\n", panic_extra_info.extra_buf[INFO_BUG]);

	panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
		BUF_SIZE_MARGIN - panic_string_offset,
		"\"PANIC\":\"%s\",\n", str);
	
	if(panic_extra_info.pc != -1)
		panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
			BUF_SIZE_MARGIN - panic_string_offset,
			"\"PC\":\"%pS\",\n", (void*)panic_extra_info.pc);
	else
		panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
			BUF_SIZE_MARGIN - panic_string_offset,
			"\"PC\":\"\",\n");
			
	if(panic_extra_info.lr != -1)
		panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
			BUF_SIZE_MARGIN - panic_string_offset,
			"\"LR\":\"%pS\",\n", (void*)panic_extra_info.lr);
	else
		panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
			BUF_SIZE_MARGIN - panic_string_offset,
			"\"LR\":\"\",\n");

	cp_len = strlen(panic_extra_info.backtrace);

	if(panic_string_offset + cp_len > BUF_SIZE_MARGIN)
		cp_len = BUF_SIZE_MARGIN - panic_string_offset;

	if(cp_len > 0) {
		panic_string_offset += sprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset, 
		"\"STACK\":\"");
		strncpy((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset, panic_extra_info.backtrace, cp_len);
		panic_string_offset += cp_len;
	}
	else {
		panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
			BUF_SIZE_MARGIN - panic_string_offset,
			"\"STACK\":\"N/A\",\n");
	}

	panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
		BUF_SIZE_MARGIN - panic_string_offset, "\"CHIPID\":\"\",\n");

	panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA  + panic_string_offset,
		BUF_SIZE_MARGIN - panic_string_offset,
		"\"DBG0\":\"%d.%d\"", (exynos_soc_info.product_id>>4) & 0xf, (exynos_soc_info.product_id & 0xf));		

	for(i = 1; i < INFO_MAX; i++) {
		if(panic_string_offset + strlen(panic_extra_info.extra_buf[i]) < BUF_SIZE_MARGIN)
			panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
				BUF_SIZE_MARGIN - panic_string_offset,
				",\n\"DBG%d\":\"%s\"", i, panic_extra_info.extra_buf[i]);
		else {
			panic_string_offset += snprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset,
				BUF_SIZE_MARGIN - panic_string_offset, ",\n\"DBG%d\":\"%s\"", i, panic_extra_info.extra_buf[i]);
		}
	}
	sprintf((char *)SEC_DEBUG_EXTRA_INFO_VA + panic_string_offset, "\n");
}

void sec_debug_store_fault_addr(unsigned long addr, struct pt_regs *regs)
{
	printk("sec_debug_store_fault_addr 0x%lx\n", addr);
	
	panic_extra_info.fault_addr = addr;
	if(regs) {
		panic_extra_info.pc = regs->pc;
		panic_extra_info.lr = compat_user_mode(regs) ? regs->compat_lr : regs->regs[30];
	}
}

void sec_debug_store_extra_buf(enum sec_debug_extra_buf_type type, const char *fmt, ...)
{	
	va_list args;
	int len;

	if(!strncmp(panic_extra_info.extra_buf[type], "N/A", 3))
		len = 0;
	else
		len = strlen(panic_extra_info.extra_buf[type]);

	printk("sec_debug_store_extra_buf type %d, len %d\n", type, len);
	
	va_start(args, fmt);
	vsnprintf(&panic_extra_info.extra_buf[type][len], SZ_256 - len, fmt, args);
	va_end(args);
}

void sec_debug_store_backtrace(struct pt_regs *regs)
{
	char buf[64];
	struct stackframe frame;
	int offset = 0;
	int sym_name_len;

	printk("sec_debug_store_backtrace\n");

	if (regs) {
		frame.fp = regs->regs[29];
		frame.sp = regs->sp;
		frame.pc = regs->pc;
	} else {
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.sp = current_stack_pointer;
		frame.pc = (unsigned long)sec_debug_store_backtrace;
	}
	
	while (1) {
		unsigned long where = frame.pc;
		int ret;

		ret = unwind_frame(&frame);
		if (ret < 0)
			break;

		snprintf(buf, sizeof(buf), "%pf", (void *)where);
		sym_name_len = strlen(buf);	

		if(offset + sym_name_len > SZ_1K)
			break;

		if(offset)
			offset += sprintf((char*)panic_extra_info.backtrace+offset, " : ");

		sprintf((char*)panic_extra_info.backtrace+offset, "%s", buf);		
		offset += sym_name_len;
	}
	sprintf((char*)panic_extra_info.backtrace+offset, "\",\n");
	
}

static void sec_debug_set_upload_magic(unsigned magic, char *str)
{
	pr_emerg("sec_debug: set magic code (0x%x)\n", magic);

	*(unsigned int *)SEC_DEBUG_MAGIC_VA = magic;
	*(unsigned int *)(SEC_DEBUG_MAGIC_VA + SZ_4K - 4) = magic;

	if (str) {
		strncpy((char *)SEC_DEBUG_MAGIC_VA + 4, str, SZ_1K - 4);
		sec_debug_store_panic_extra_info(str);
	}
}

static void sec_debug_set_upload_cause(enum sec_debug_upload_cause_t type)
{
	exynos_pmu_write(EXYNOS_INFORM3, type);

	pr_emerg("sec_debug: set upload cause (0x%x)\n", type);
}
#if 1
static void sec_debug_kmsg_dump(struct kmsg_dumper *dumper,
				enum kmsg_dump_reason reason)
{
	char *ptr = (char *)SEC_DEBUG_MAGIC_VA + SZ_2K;
#if 0
	int total_chars = SZ_4K - SZ_1K;
	int total_lines = 50;
	/* no of chars which fits in total_chars *and* in total_lines */
	int last_chars;

	for (last_chars = 0;
	     l2 && l2 > last_chars && total_lines > 0
	     && total_chars > 0; ++last_chars, --total_chars) {
		if (s2[l2 - last_chars] == '\n')
			--total_lines;
	}
	s2 += (l2 - last_chars);
	l2 = last_chars;

	for (last_chars = 0;
	     l1 && l1 > last_chars && total_lines > 0
	     && total_chars > 0; ++last_chars, --total_chars) {
		if (s1[l1 - last_chars] == '\n')
			--total_lines;
	}
	s1 += (l1 - last_chars);
	l1 = last_chars;

	while (l1-- > 0)
		*ptr++ = *s1++;
	while (l2-- > 0)
		*ptr++ = *s2++;
#endif
	kmsg_dump_get_buffer(dumper, true, ptr, SZ_4K - SZ_2K, NULL);

}
static struct kmsg_dumper sec_dumper = {
	.dump = sec_debug_kmsg_dump,
};
#endif

int __init sec_debug_init(void)
{
	size_t size = SZ_4K;
	size_t base = SEC_DEBUG_MAGIC_PA;

	/* clear traps info */
	memset((void*)SEC_DEBUG_MAGIC_VA + 4, 0, SZ_1K - 4);

	hrtimer_init(&hardkey_triger_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	hardkey_triger_timer.function = Hard_Reset_Triger_callback;

/*
	if (!sec_debug_level.en.kernel_fault) {
		pr_info("sec_debug: disabled due to debug level (0x%x)\n",
		sec_debug_level.uint_val);
		return -1;
	}
*/

#ifdef CONFIG_NO_BOOTMEM
	if (!memblock_is_region_reserved(base, size) &&
		!memblock_reserve(base, size)) {
#else
	if (!reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE)) {
#endif
		pr_info("%s: Reserved Mem(0x%zx, 0x%zx) - Success\n",
			__FILE__, base, size);

		sec_debug_set_upload_magic(UPLOAD_MAGIC_PANIC, NULL);
		sec_debug_init_panic_extra_info();
	} else
		goto out;

	/* sec_debug_set_upload_cause(UPLOAD_CAUSE_INIT); */

	kmsg_dump_register(&sec_dumper);

	return 0;
out:
	pr_err("%s: Reserved Mem(0x%zx, 0x%zx) - Failed\n",
	       __FILE__, base, size);
	return -ENOMEM;
}

#if 0
/* task state
 * R (running)
 * S (sleeping)
 * D (disk sleep)
 * T (stopped)
 * t (tracing stop)
 * Z (zombie)
 * X (dead)
 * K (killable)
 * W (waking)
 * P (parked)
 */
static void sec_debug_dump_task_info(struct task_struct *tsk, bool is_main)
{
	char state_array[] = { 'R', 'S', 'D', 'T', 't',
			       'Z', 'X', 'x', 'K', 'W', 'P' };
	unsigned char idx = 0;
	unsigned int state = (tsk->state & TASK_REPORT) | tsk->exit_state;
	unsigned long wchan;
	unsigned long pc = 0;
	char symname[KSYM_NAME_LEN];
	int permitted;
	struct mm_struct *mm;

	permitted = ptrace_may_access(tsk, PTRACE_MODE_READ);
	mm = get_task_mm(tsk);
	if (mm) {
		if (permitted)
			pc = KSTK_EIP(tsk);
	}

	wchan = get_wchan(tsk);
	if (lookup_symbol_name(wchan, symname) < 0) {
		if (!ptrace_may_access(tsk, PTRACE_MODE_READ))
			sprintf(symname, "_____");
		else
			sprintf(symname, "%lu", wchan);
	}

	while (state) {
		idx++;
		state >>= 1;
	}

	pr_info("%8d %8d %8d %16lld %c(%d) %3d  %16zx %16zx  %16zx %c %16s [%s]\n",
		 tsk->pid, (int)(tsk->utime), (int)(tsk->stime), tsk->se.exec_start,
		 state_array[idx], (int)(tsk->state), task_cpu(tsk), wchan, pc,
		 (unsigned long)tsk, is_main ? '*' : ' ', tsk->comm, symname);

	if (tsk->state == TASK_RUNNING ||
			tsk->state == TASK_UNINTERRUPTIBLE || tsk->mm == NULL) {
		show_stack(tsk, NULL);
		pr_info("\n");
	}
}

static void sec_debug_dump_all_task(void)
{
	struct task_struct *frst_tsk;
	struct task_struct *curr_tsk;
	struct task_struct *frst_thr;
	struct task_struct *curr_thr;

	pr_info("\n");
	pr_info("current proc : %d %s\n", current->pid, current->comm);
	pr_info("----------------------------------------------------------------------------------------------------------------------------\n");
	pr_info("    pid      uTime    sTime      exec(ns)  stat  cpu       wchan           user_pc        task_struct       comm   sym_wchan\n");
	pr_info("----------------------------------------------------------------------------------------------------------------------------\n");

	/* processes */
	frst_tsk = &init_task;
	curr_tsk = frst_tsk;
	while (curr_tsk != NULL) {
		sec_debug_dump_task_info(curr_tsk, true);
		/* threads */
		if (curr_tsk->thread_group.next != NULL) {
			frst_thr = container_of(curr_tsk->thread_group.next,
						struct task_struct, thread_group);
			curr_thr = frst_thr;
			if (frst_thr != curr_tsk) {
				while (curr_thr != NULL) {
					sec_debug_dump_task_info(curr_thr, false);
					curr_thr = container_of(curr_thr->thread_group.next, struct task_struct, thread_group);
					if (curr_thr == curr_tsk)
						break;
				}
			}
		}
		curr_tsk = container_of(curr_tsk->tasks.next, struct task_struct, tasks);
		if (curr_tsk == frst_tsk)
			break;
	}
	pr_info("----------------------------------------------------------------------------------------------------------------------------\n");
}
#endif

#ifndef arch_irq_stat_cpu
#define arch_irq_stat_cpu(cpu) 0
#endif
#ifndef arch_irq_stat
#define arch_irq_stat() 0
#endif
#ifdef arch_idle_time
static cputime64_t get_idle_time(int cpu)
{
	cputime64_t idle;

	idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	if (cpu_online(cpu) && !nr_iowait_cpu(cpu))
		idle += arch_idle_time(cpu);
	return idle;
}

static cputime64_t get_iowait_time(int cpu)
{
	cputime64_t iowait;

	iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	if (cpu_online(cpu) && nr_iowait_cpu(cpu))
		iowait += arch_idle_time(cpu);
	return iowait;
}
#else
static u64 get_idle_time(int cpu)
{
	u64 idle, idle_time = -1ULL;

	if (cpu_online(cpu))
		idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.idle */
		idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	else
		idle = usecs_to_cputime64(idle_time);

	return idle;
}

static u64 get_iowait_time(int cpu)
{
	u64 iowait, iowait_time = -1ULL;

	if (cpu_online(cpu))
		iowait_time = get_cpu_iowait_time_us(cpu, NULL);

	if (iowait_time == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.iowait */
		iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	else
		iowait = usecs_to_cputime64(iowait_time);

	return iowait;
}
#endif

static void sec_debug_dump_cpu_stat(void)
{
	int i, j;
	u64 user, nice, system, idle, iowait, irq, softirq, steal;
	u64 guest, guest_nice;
	u64 sum = 0;
	u64 sum_softirq = 0;
	unsigned int per_softirq_sums[NR_SOFTIRQS] = {0};
	struct timespec boottime;

	char *softirq_to_name[NR_SOFTIRQS] = { "HI", "TIMER",
					       "NET_TX", "NET_RX",
					       "BLOCK", "BLOCK_IOPOLL",
					       "TASKLET", "SCHED",
					       "HRTIMER", "RCU" };

	user = nice = system = idle = iowait = irq = softirq = steal = 0;
	guest = guest_nice = 0;
	getboottime(&boottime);

	for_each_possible_cpu(i) {
		user	+= kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice	+= kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system	+= kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle	+= get_idle_time(i);
		iowait	+= get_iowait_time(i);
		irq	+= kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq	+= kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal	+= kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest	+= kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice += kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];
		sum	+= kstat_cpu_irqs_sum(i);
		sum	+= arch_irq_stat_cpu(i);

		for (j = 0; j < NR_SOFTIRQS; j++) {
			unsigned int softirq_stat = kstat_softirqs_cpu(j, i);

			per_softirq_sums[j] += softirq_stat;
			sum_softirq += softirq_stat;
		}
	}
	sum += arch_irq_stat();

	pr_info("\n");
	pr_info("cpu   user:%llu \tnice:%llu \tsystem:%llu \tidle:%llu \tiowait:%llu \tirq:%llu \tsoftirq:%llu \t %llu %llu %llu\n",
			(unsigned long long)cputime64_to_clock_t(user),
			(unsigned long long)cputime64_to_clock_t(nice),
			(unsigned long long)cputime64_to_clock_t(system),
			(unsigned long long)cputime64_to_clock_t(idle),
			(unsigned long long)cputime64_to_clock_t(iowait),
			(unsigned long long)cputime64_to_clock_t(irq),
			(unsigned long long)cputime64_to_clock_t(softirq),
			(unsigned long long)cputime64_to_clock_t(steal),
			(unsigned long long)cputime64_to_clock_t(guest),
			(unsigned long long)cputime64_to_clock_t(guest_nice));
	pr_info("-------------------------------------------------------------------------------------------------------------\n");

	for_each_possible_cpu(i) {
		/* Copy values here to work around gcc-2.95.3, gcc-2.96 */
		user	= kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice	= kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system	= kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle	= get_idle_time(i);
		iowait	= get_iowait_time(i);
		irq	= kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq	= kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal	= kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest	= kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice = kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];

		pr_info("cpu%d  user:%llu \tnice:%llu \tsystem:%llu \tidle:%llu \tiowait:%llu \tirq:%llu \tsoftirq:%llu \t %llu %llu %llu\n",
			i,
			(unsigned long long)cputime64_to_clock_t(user),
			(unsigned long long)cputime64_to_clock_t(nice),
			(unsigned long long)cputime64_to_clock_t(system),
			(unsigned long long)cputime64_to_clock_t(idle),
			(unsigned long long)cputime64_to_clock_t(iowait),
			(unsigned long long)cputime64_to_clock_t(irq),
			(unsigned long long)cputime64_to_clock_t(softirq),
			(unsigned long long)cputime64_to_clock_t(steal),
			(unsigned long long)cputime64_to_clock_t(guest),
			(unsigned long long)cputime64_to_clock_t(guest_nice));
	}
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	pr_info("\n");
	pr_info("irq : %llu", (unsigned long long)sum);
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	/* sum again ? it could be updated? */
	for_each_irq_nr(j) {
		unsigned int irq_stat = kstat_irqs(j);

		if (irq_stat) {
			struct irq_desc *desc = irq_to_desc(j);
#if defined(CONFIG_SEC_DEBUG_PRINT_IRQ_PERCPU)
			pr_info("irq-%-4d : %8u :", j, irq_stat);
			for_each_possible_cpu(i)
				pr_info(" %8u", kstat_irqs_cpu(j, i));
			pr_info(" %s", desc->action ? desc->action->name ? : "???" : "???");
			pr_info("\n");
#else
			pr_info("irq-%-4d : %8u %s\n", j, irq_stat,
				desc->action ? desc->action->name ? : "???" : "???");
#endif
		}
	}
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	pr_info("\n");
	pr_info("softirq : %llu", (unsigned long long)sum_softirq);
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < NR_SOFTIRQS; i++)
		if (per_softirq_sums[i])
			pr_info("softirq-%d : %8u %s\n",
				i, per_softirq_sums[i], softirq_to_name[i]);
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
}

void sec_debug_reboot_handler(void)
{
	/* Clear magic code in normal reboot */
	sec_debug_set_upload_magic(UPLOAD_MAGIC_INIT, NULL);
}

void sec_debug_panic_handler(void *buf, bool dump)
{
	/* Set upload cause */
	sec_debug_set_upload_magic(UPLOAD_MAGIC_PANIC, buf);
	if (!strncmp(buf, "User Fault", 10))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_USER_FAULT);
	else if (exynos_ss_hardkey_triger)
		sec_debug_set_upload_cause(UPLOAD_CAUSE_HARDKEY_RESET);	
	else if (!strncmp(buf, "Crash Key", 9))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_FORCED_UPLOAD);
#ifdef CONFIG_SEC_UPLOAD
	else if (!strncmp(buf, "User Crash Key", 14))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_USER_FORCED_UPLOAD);
#endif
	else if (!strncmp(buf, "CP Crash", 8))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_CP_ERROR_FATAL);
	else if (!strncmp(buf, "HSIC Disconnected", 17))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_HSIC_DISCONNECTED);
	else
		sec_debug_set_upload_cause(UPLOAD_CAUSE_KERNEL_PANIC);

	/* dump debugging info */
	if (dump) {
#if 0
		sec_debug_dump_all_task();
#endif
		sec_debug_dump_cpu_stat();
		show_state_filter(TASK_STATE_MAX);
		debug_show_all_locks();
	}

	/* hw reset */
	pr_emerg("sec_debug: %s\n", linux_banner);
	pr_emerg("sec_debug: rebooting...\n");

	flush_cache_all();
#if 0
	mach_restart(0, 0);

	while (1)
		pr_err("sec_debug: should not be happend\n");
#endif
}

static enum hrtimer_restart Hard_Reset_Triger_callback(struct hrtimer *hrtimer)
{
	pr_info("sec_debug: 7Sec_HardKey Triger\n");

	exynos_ss_hardkey_triger = true;
	BUG();

	return HRTIMER_RESTART;
}

void sec_debug_Hard_Reset_Triger(unsigned int code, int value)
{
	static int hardkey = 0x0;
	ktime_t ktime;

	if (value) {
		if (code == KEY_POWER)
			hardkey |= 0x2;
		if (code == KEY_VOLUMEDOWN)
			hardkey |= 0x1;
		if (hardkey == HARD_RESET_KEY) {
			pr_info("sec_debug: Start HARD KEY RESET Triger\n");
			ktime = ktime_set(0, MS_TO_NS(6000L));
			hrtimer_start(&hardkey_triger_timer,
				      ktime, HRTIMER_MODE_REL);
		}
	} else {
		if (code == KEY_POWER)
			hardkey &= 0x1;
		if (code == KEY_VOLUMEDOWN)
			hardkey &= 0x2;
		if (hardkey != HARD_RESET_KEY) {
			pr_info("sec_debug: Cancle HARD KEY RESET Triger\n");
			hrtimer_cancel(&hardkey_triger_timer);
		}
	}
}

#ifdef CONFIG_SEC_UPLOAD
extern void check_crash_keys_in_user(unsigned int code, int onoff);
#endif

void sec_debug_check_crash_key(unsigned int code, int value)
{
	static bool volup_p;
	static bool voldown_p;
	static int loopcount;

	static const unsigned int VOLUME_UP = KEY_VOLUMEUP;
	static const unsigned int VOLUME_DOWN = KEY_VOLUMEDOWN;

	sec_debug_Hard_Reset_Triger(code, value);

	if (!sec_debug_level.en.kernel_fault) {
#ifdef CONFIG_SEC_UPLOAD
		check_crash_keys_in_user(code, value);
#endif
		return;
	}

	if (code == KEY_POWER)
		pr_info("%s: POWER-KEY(%d)\n", __FILE__, value);

	/* Enter Forced Upload
	 *  Hold volume down key first
	 *  and then press power key twice
	 *  and volume up key should not be pressed
	 */
	if (value) {
		if (code == VOLUME_UP)
			volup_p = true;
		if (code == VOLUME_DOWN)
			voldown_p = true;
		if (!volup_p && voldown_p) {
			if (code == KEY_POWER) {
				pr_info("%s: Forced Upload Count: %d\n",
					__FILE__, ++loopcount);
				if (loopcount == 2)
					panic("Crash Key");
			}
		}

#if defined(CONFIG_SEC_DEBUG_TSP_LOG) && (defined(CONFIG_TOUCHSCREEN_FTS) || defined(CONFIG_TOUCHSCREEN_SEC_TS))
		/* dump TSP rawdata
		 *	Hold volume up key first
		 *	and then press home key twice
		 *	and volume down key should not be pressed
		 */
		if (volup_p && !voldown_p) {
			if (code == KEY_HOMEPAGE) {
				pr_info("%s: count to dump tsp rawdata : %d\n",
					 __func__, ++loopcount);
				if (loopcount == 2) {
#if defined(CONFIG_TOUCHSCREEN_FTS)
					tsp_dump();
#elif defined(CONFIG_TOUCHSCREEN_SEC_TS)
					tsp_dump_sec();
#endif
					loopcount = 0;
				}
			}
		}
#endif
	} else {
		if (code == VOLUME_UP)
			volup_p = false;
		if (code == VOLUME_DOWN) {
			loopcount = 0;
			voldown_p = false;
		}
	}
}

#ifdef CONFIG_USER_RESET_DEBUG
static int set_reset_reason_proc_show(struct seq_file *m, void *v)
{
	if (reset_reason == RR_S)
		seq_puts(m, "SPON\n");
	else if (reset_reason == RR_W)
		seq_puts(m, "WPON\n");
	else if (reset_reason == RR_D)
		seq_puts(m, "DPON\n");
	else if (reset_reason == RR_K)
		seq_puts(m, "KPON\n");
	else if (reset_reason == RR_M)
		seq_puts(m, "MPON\n");
	else if (reset_reason == RR_P)
		seq_puts(m, "PPON\n");
	else if (reset_reason == RR_R)
		seq_puts(m, "RPON\n");
	else if (reset_reason == RR_B)
		seq_puts(m, "BPON\n");
	else if (reset_reason == RR_T)
		seq_puts(m, "TPON\n");
	else
		seq_puts(m, "NPON\n");

	return 0;
}

static int set_reset_extra_info_proc_show(struct seq_file *m, void *v)
{
	char buf[SZ_1K];

	memcpy(buf, (char *)SEC_DEBUG_EXTRA_INFO_VA, SZ_1K);

	if (reset_reason == RR_K)
		seq_printf(m,buf);
	else
		return -ENOENT;

	return 0;
}

#if defined(CONFIG_SEC_DUMP_SUMMARY)
static ssize_t sec_reset_summary_info_proc_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if(reset_reason < RR_D || reset_reason > RR_P)
		return -ENOENT;

	if (pos >= last_summary_size)
		return -ENOENT;

	count = min(len, (size_t)(last_summary_size - pos));
	if (copy_to_user(buf, last_summary_buffer + pos, count))
		return -EFAULT;
	
	*offset += count;
	return count;
}

static const struct file_operations sec_reset_summary_info_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_summary_info_proc_read,
};
#endif

#if defined(CONFIG_KFAULT_AUTO_SUMMARY)
static ssize_t sec_reset_auto_comment_proc_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if(!auto_comment_buf)
	{
		printk("%s : auto_comment_buf null\n", __func__);
		return -ENODEV;
	}

	if(reset_reason >= RR_R && reset_reason <= RR_N) {
		printk("%s : auto_comment_buf reset_reason %d\n", __func__, reset_reason);
		return -ENOENT;
	}

	if (pos >= AUTO_COMMENT_SIZE) {
		printk("%s : auto_comment_buf pos 0x%llx\n", __func__, pos);
		return -ENOENT;
	}

	count = min(len, (size_t)(AUTO_COMMENT_SIZE - pos));
	if (copy_to_user(buf, auto_comment_buf + pos, count))
		return -EFAULT;
	
	*offset += count;
	return count;
}

static const struct file_operations sec_reset_auto_comment_proc_fops = {
	.owner = THIS_MODULE,
	.read = sec_reset_auto_comment_proc_read,
};
#endif

static int
sec_reset_reason_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_reset_reason_proc_show, NULL);
}

static int
sec_reset_extra_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_reset_extra_info_proc_show, NULL);
}


static const struct file_operations sec_reset_extra_info_proc_fops = {
	.open = sec_reset_extra_info_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations sec_reset_reason_proc_fops = {
	.open = sec_reset_reason_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init sec_debug_reset_reason_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("reset_reason", S_IWUGO, NULL,
		&sec_reset_reason_proc_fops);

	if (!entry)
		return -ENOMEM;

	entry = proc_create("reset_reason_extra_info", S_IWUGO, NULL,
		&sec_reset_extra_info_proc_fops);

	if (!entry)
		return -ENOMEM;

	proc_set_size(entry, SZ_1K);
	
#if defined(CONFIG_SEC_DUMP_SUMMARY)
	entry = proc_create("reset_summary", S_IWUGO, NULL,
		&sec_reset_summary_info_proc_fops);

	if (!entry)
		return -ENOMEM;	

	proc_set_size(entry, last_summary_size);
#endif

#if defined(CONFIG_KFAULT_AUTO_SUMMARY)
	entry = proc_create("auto_comment", S_IWUGO, NULL,
		&sec_reset_auto_comment_proc_fops);

	if (!entry)
		return -ENOMEM;

	proc_set_size(entry, AUTO_COMMENT_SIZE);
#endif

	return 0;
}

device_initcall(sec_debug_reset_reason_init);
#endif

#ifdef CONFIG_SEC_FILE_LEAK_DEBUG
void sec_debug_print_file_list(void)
{
	int i = 0;
	unsigned int nCnt = 0;
	struct file *file = NULL;
	struct files_struct *files = current->files;
	const char *pRootName = NULL;
	const char *pFileName = NULL;

	nCnt = files->fdt->max_fds;

	pr_err("[Opened file list of process %s(PID:%d, TGID:%d) :: %d]\n",
	       current->group_leader->comm, current->pid, current->tgid, nCnt);

	for (i = 0; i < nCnt; i++) {
		rcu_read_lock();
		file = fcheck_files(files, i);

		pRootName = NULL;
		pFileName = NULL;

		if (file) {
			if (file->f_path.mnt
				&& file->f_path.mnt->mnt_root
				&& file->f_path.mnt->mnt_root->d_name.name)
				pRootName = file->f_path.mnt->mnt_root->d_name.name;

			if (file->f_path.dentry && file->f_path.dentry->d_name.name)
				pFileName = file->f_path.dentry->d_name.name;

			pr_err("[%04d]%s%s\n", i,
			       pRootName == NULL ? "null" : pRootName,
			       pFileName == NULL ? "null" : pFileName);
		}
		rcu_read_unlock();
	}
}

void sec_debug_EMFILE_error_proc(unsigned long files_addr)
{
	if (files_addr != (unsigned long)(current->files)) {
		pr_err("Too many open files Error at %pS\n"
		       "%s(%d) thread of %s process tried fd allocation by proxy.\n"
		       "files_addr = 0x%lx, current->files=0x%p\n",
		       __builtin_return_address(0),
		       current->comm, current->tgid, current->group_leader->comm,
		       files_addr, current->files);
		return;
	}

	pr_err("Too many open files(%d:%s) at %pS\n",
	       current->tgid, current->group_leader->comm,
	       __builtin_return_address(0));

	if (!sec_debug_level.en.kernel_fault)
		return;

	/* We check EMFILE error in only "system_server",
	   "mediaserver" and "surfaceflinger" process. */
	if (!strcmp(current->group_leader->comm, "system_server")
		|| !strcmp(current->group_leader->comm, "mediaserver")
		|| !strcmp(current->group_leader->comm, "surfaceflinger")) {
		sec_debug_print_file_list();
		panic("Too many open files");
	}
}
#endif

#if defined(CONFIG_SEC_PARAM)
#define SEC_PARAM_NAME "/dev/block/param"
struct sec_param_data_s {
	struct work_struct sec_param_work;
	unsigned long offset;
	char val;
};

static struct sec_param_data_s sec_param_data;
static DEFINE_MUTEX(sec_param_mutex);

static void sec_param_update(struct work_struct *work)
{
	int ret = -1;
	struct file *fp;
	struct sec_param_data_s *param_data =
		container_of(work, struct sec_param_data_s, sec_param_work);

	fp = filp_open(SEC_PARAM_NAME, O_WRONLY | O_SYNC, 0);
	if (IS_ERR(fp)) {
		pr_err("%s: filp_open error %ld\n", __func__, PTR_ERR(fp));
		return;
	}
	pr_info("%s: set param %c at %lu\n", __func__,
				param_data->val, param_data->offset);
	ret = fp->f_op->llseek(fp, param_data->offset, SEEK_SET);
	if (ret < 0) {
		pr_err("%s: llseek error %d!\n", __func__, ret);
		goto close_fp_out;
	}

	ret = fp->f_op->write(fp, &param_data->val, 1, &(fp->f_pos));
	if (ret < 0)
		pr_err("%s: write error! %d\n", __func__, ret);

close_fp_out:
	if (fp)
		filp_close(fp, NULL);

	pr_info("%s: exit %d\n", __func__, ret);
}

/*
  success : ret >= 0
  fail : ret < 0
 */
int sec_set_param(unsigned long offset, char val)
{
	int ret = -1;

	mutex_lock(&sec_param_mutex);

	if ((offset < CM_OFFSET) || (offset > CM_OFFSET + CM_OFFSET_LIMIT))
		goto unlock_out;

	switch (val) {
	case PARAM_OFF:
	case PARAM_ON:
		goto set_param;
	default:
		if (val >= PARAM_TEST0 && val < PARAM_MAX)
			goto set_param;
		goto unlock_out;
	}

set_param:
	sec_param_data.offset = offset;
	sec_param_data.val = val;

	schedule_work(&sec_param_data.sec_param_work);

	/* how to determine to return success or fail ? */

	ret = 0;
unlock_out:
	mutex_unlock(&sec_param_mutex);
	return ret;
}

static int __init sec_param_work_init(void)
{
	pr_info("%s: start\n", __func__);

	sec_param_data.offset = 0;
	sec_param_data.val = '0';

	INIT_WORK(&sec_param_data.sec_param_work, sec_param_update);

	return 0;
}

static void __exit sec_param_work_exit(void)
{
	cancel_work_sync(&sec_param_data.sec_param_work);
	pr_info("%s: exit\n", __func__);
}
module_init(sec_param_work_init);
module_exit(sec_param_work_exit);
#endif /* CONFIG_SEC_PARAM */

#endif /* CONFIG_SEC_DEBUG */

#if defined(CONFIG_KFAULT_AUTO_SUMMARY)
void sec_debug_auto_summary_log_disable(int type)
{
	atomic_inc(&auto_summary_info->auto_Comm_buf[type].logging_diable);
}

void sec_debug_auto_summary_log_once(int type)
{
	if (atomic64_read(&auto_summary_info->auto_Comm_buf[type].logging_entry))
		sec_debug_auto_summary_log_disable(type);
	else
		atomic_inc(&auto_summary_info->auto_Comm_buf[type].logging_entry);
}

static inline void sec_debug_hook_auto_comm_lastfreq(int type, int old_freq, int new_freq, u64 time)
{
	if(type < FREQ_INFO_MAX) {
		auto_summary_info->freq_info[type].old_freq = old_freq;
		auto_summary_info->freq_info[type].new_freq = new_freq;
		auto_summary_info->freq_info[type].time_stamp = time;
	}
}

static inline void sec_debug_hook_auto_comm(int type, const char *buf, size_t size)
{
	struct sec_debug_auto_comm_buf* auto_comm_buf = &auto_summary_info->auto_Comm_buf[type];
	int offset = auto_comm_buf->offset;

	if (atomic64_read(&auto_comm_buf->logging_diable))
		return;

	if (offset + size > SZ_4K)
		return;

	if (init_data[type].max_count &&
		atomic64_read(&auto_comm_buf->logging_count) > init_data[type].max_count)
		return;

	if (!(auto_summary_info->fault_flag & 1 << type)) {
		auto_summary_info->fault_flag |= 1 << type;
		if(init_data[type].prio_level == PRIO_LV5) {
			auto_summary_info->lv5_log_order |= type << auto_summary_info->lv5_log_cnt * 4;
			auto_summary_info->lv5_log_cnt++;
		}		
		auto_summary_info->order_map[auto_summary_info->order_map_cnt++] = type;
	}
	
	atomic_inc(&auto_comm_buf->logging_count);

	memcpy(auto_comm_buf->buf + offset, buf, size);
	auto_comm_buf->offset += size;
}

static void sec_auto_summary_init_print_buf(unsigned long base)
{
	auto_comment_buf = (char*)phys_to_virt(base);
	auto_summary_info = (struct sec_debug_auto_summary*)phys_to_virt(base+SZ_4K);

	memset(auto_summary_info, 0, sizeof(struct sec_debug_auto_summary));

	auto_summary_info->haeder_magic = AUTO_SUMMARY_MAGIC;
	auto_summary_info->tail_magic = AUTO_SUMMARY_TAIL_MAGIC;
	
	auto_summary_info->pa_text = virt_to_phys(_text);
	auto_summary_info->pa_start_rodata = virt_to_phys(__start_rodata);
	
	register_set_auto_comm_buf(sec_debug_hook_auto_comm);
	register_set_auto_comm_lastfreq(sec_debug_hook_auto_comm_lastfreq);
	sec_debug_set_auto_comm_last_devfreq_buf(auto_summary_info->freq_info);
	sec_debug_set_auto_comm_last_cpufreq_buf(auto_summary_info->freq_info);
}

static int __init sec_auto_summary_log_setup(char *str)
{
	unsigned long size = memparse(str, &str);
	unsigned long base = 0;

	/* If we encounter any problem parsing str ... */
	if (!size || *str != '@' || kstrtoul(str + 1, 0, &base)) {
		pr_err("%s: failed to parse address.\n", __func__);
		goto out;
	}

	size += sizeof(struct sec_debug_auto_summary);

#ifdef CONFIG_NO_BOOTMEM
		if (memblock_is_region_reserved(base, size) ||
			memblock_reserve(base, size)) {
#else
		if (reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE)) {
#endif
		pr_err("%s: failed to reserve size:0x%lx " \
				"at base 0x%lx\n", __func__, size + sizeof(auto_summary_info), base);
		goto out;
	}
		
	sec_auto_summary_init_print_buf(base);	

	pr_info("%s, base:0x%lx size:0x%lx\n", __func__, base, size);
	
out:
	return 0;
}
__setup("auto_summary_log=", sec_auto_summary_log_setup);
#endif

#if defined(CONFIG_SEC_DUMP_SUMMARY)
void sec_debug_summary_set_reserved_out_buf(unsigned long buf, unsigned long size)
{
	reserved_out_buf = buf;
	reserved_out_size = size;
}

static int __init sec_summary_log_setup(char *str)
{
	unsigned long size = memparse(str, &str);
	unsigned long base = 0;

	/* If we encounter any problem parsing str ... */
	if (!size || *str != '@' || kstrtoul(str + 1, 0, &base)) {
		pr_err("%s: failed to parse address.\n", __func__);
		goto out;
	}

	last_summary_size = size;

	// dump_summary size set 1MB in low level.
	if (!sec_debug_level.en.kernel_fault)
		size=0x100000;
		

#ifdef CONFIG_NO_BOOTMEM
		if (memblock_is_region_reserved(base, size) ||
			memblock_reserve(base, size)) {
#else
		if (reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE)) {
#endif
		pr_err("%s: failed to reserve size:0x%lx " \
				"at base 0x%lx\n", __func__, size, base);
		goto out;
	}

	pr_info("%s, base:0x%lx size:0x%lx\n", __func__, base, size);

	sec_summary_log_buf = phys_to_virt(base);
	sec_summary_log_size = round_up(sizeof(struct sec_debug_summary), PAGE_SIZE);
	last_summary_buffer = phys_to_virt(base+sec_summary_log_size);
	sec_debug_summary_set_reserved_out_buf(base + sec_summary_log_size, (size - sec_summary_log_size));
out:
	return 0;
}
__setup("sec_summary_log=", sec_summary_log_setup);

int sec_debug_summary_init(void)
{
	int offset = 0;
	
	if(!sec_summary_log_buf) {
		pr_info("no summary buffer\n");
		return 0;
	}
	
	summary_info = (struct sec_debug_summary *)sec_summary_log_buf;
	memset(summary_info, 0, sizeof(struct sec_debug_summary));

	exynos_ss_summary_set_sched_log_buf(summary_info);
#ifdef CONFIG_ANDROID_LOGGER
	sec_debug_summary_set_logger_info(&summary_info->kernel.logger_log);
#endif
	offset += sizeof(struct sec_debug_summary);

	summary_info->kernel.cpu_info.cpu_active_mask_paddr = virt_to_phys(cpu_active_mask);
	summary_info->kernel.cpu_info.cpu_online_mask_paddr = virt_to_phys(cpu_online_mask);
	offset += sec_debug_set_cpu_info(summary_info,sec_summary_log_buf+offset);

	summary_info->kernel.nr_cpus = CONFIG_NR_CPUS;
	summary_info->reserved_out_buf = reserved_out_buf;
	summary_info->reserved_out_size = reserved_out_size;

	summary_info->magic[0] = SEC_DEBUG_SUMMARY_MAGIC0;
	summary_info->magic[1] = SEC_DEBUG_SUMMARY_MAGIC1;
	summary_info->magic[2] = SEC_DEBUG_SUMMARY_MAGIC2;
	summary_info->magic[3] = SEC_DEBUG_SUMMARY_MAGIC3;
	
	sec_debug_summary_set_kallsyms_info(summary_info);

	pr_debug("%s, sec_debug_summary_init done [%d]\n", __func__, offset);

	return 0;
}
late_initcall(sec_debug_summary_init);

int sec_debug_save_panic_info(const char *str, unsigned long caller)
{
	if(!sec_summary_log_buf || !summary_info)
		return 0;
	snprintf(summary_info->kernel.excp.panic_caller,
		sizeof(summary_info->kernel.excp.panic_caller), "%pS", (void *)caller);
	snprintf(summary_info->kernel.excp.panic_msg,
		sizeof(summary_info->kernel.excp.panic_msg), "%s", str);
	snprintf(summary_info->kernel.excp.thread,
		sizeof(summary_info->kernel.excp.thread), "%s:%d", current->comm,
		task_pid_nr(current));

	return 0;
}
#endif /* defined(CONFIG_SEC_DUMP_SUMMARY) */
