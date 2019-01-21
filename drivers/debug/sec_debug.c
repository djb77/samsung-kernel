/*
 * drivers/debug/sec_debug.c
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

#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/nmi.h>
#include <linux/of_address.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>
#include <linux/regulator/consumer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>
#include <linux/vmalloc.h>
#include <linux/msm_thermal.h>
#include <soc/qcom/watchdog.h>

#include <linux/sec_debug.h>
#include <linux/sec_debug_summary.h>
#include <linux/sec_param.h>

#ifdef CONFIG_USER_RESET_DEBUG
#include <linux/user_reset/sec_debug_user_reset.h>
#endif

#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/system_misc.h>
#include <asm/stacktrace.h>

#include <linux/qpnp/power-on.h>
#include <soc/qcom/scm.h>
#include <soc/qcom/smem.h>
#define CREATE_TRACE_POINTS
#include <trace/events/sec_debug.h>

//#define CONFIG_SEC_DEBUG_CANCERIZER

#ifdef CONFIG_SEC_DEBUG_CANCERIZER
#include <linux/random.h>
#include <linux/kthread.h>
#endif

// [ SEC_SELINUX_PORTING_QUALCOMM
#include <linux/proc_avc.h>
// ] SEC_SELINUX_PORTING_QUALCOMM

#include "sec_key_notifier.h"

#define MAX_SIZE_KEY	0x0f

#ifdef CONFIG_SEC_DEBUG_CANCERIZER
#define SZ_TEST		(0x4000 * PAGE_SIZE)
#define DEGREE		26

#define POWEROF16	0xFFFF
#define POWEROF20	0xFFFFF
#define POWEROF24	0xFFFFFF
#define POWEROF26	0x3FFFFFF
#define POWEROF28	0xFFFFFFF
#define POWEROF29	0x1FFFFFFF
#define POWEROF31	0x7FFFFFFF
#define POWEROF32	0xFFFFFFFF

#define POLY_DEGREE16	0xB400
#define POLY_DEGREE20	0x90000
#define POLY_DEGREE24	0xE10000
#define POLY_DEGREE26	0x2000023
#define POLY_DEGREE28	0x9000000
#define POLY_DEGREE29	0x14000000
#define POLY_DEGREE31	0x48000000
#define POLY_DEGREE32	0x80200003

#define NR_CANCERIZERD	4
#define NR_INFINITE	0x7FFFFFFF
/**
 * cancerizer_mon_ctx
 */
struct cancerizer_mon_ctx {
	atomic_t		nr_test;
	/* thread monitoring cancerizer test sessions
	 */
	struct task_struct	*self;
	wait_queue_head_t	mwq;
	/* threads conducting the actual test
	 */
	struct task_struct	*td[NR_CANCERIZERD];
	wait_queue_head_t	dwq;
	atomic_t		nr_running;
	struct completion	done;
} cmctx;
#endif

struct sec_debug_log *secdbg_log;

/* enable sec_debug feature */
static unsigned enable = 1;
static unsigned enable_user = 1;
phys_addr_t secdbg_paddr;
size_t secdbg_size;
#ifdef CONFIG_SEC_SSR_DEBUG_LEVEL_CHK
static unsigned enable_cp_debug = 1;
#endif
#ifdef CONFIG_ARCH_MSM8974PRO
static unsigned pmc8974_rev;
#else
static unsigned pm8941_rev;
static unsigned pm8841_rev;
#endif
unsigned int sec_dbg_level;

uint runtime_debug_val;
module_param_named(enable, enable, uint,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param_named(enable_user, enable_user, uint,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param_named(runtime_debug_val, runtime_debug_val, uint,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#ifdef CONFIG_SEC_SSR_DEBUG_LEVEL_CHK
module_param_named(enable_cp_debug, enable_cp_debug, uint,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif

module_param_named(pm8941_rev, pm8941_rev, uint,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param_named(pm8841_rev, pm8841_rev, uint,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#ifdef CONFIG_SEC_DEBUG_FORCE_ERROR
static int force_error(const char *val, struct kernel_param *kp);
module_param_call(force_error, force_error, NULL, NULL,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif /* CONFIG_SEC_DEBUG_FORCE_ERROR */

static int sec_alloc_virtual_mem(const char *val, struct kernel_param *kp);
module_param_call(alloc_virtual_mem, sec_alloc_virtual_mem, NULL, NULL,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

static int sec_free_virtual_mem(const char *val, struct kernel_param *kp);
module_param_call(free_virtual_mem, sec_free_virtual_mem, NULL, NULL,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

static int sec_alloc_physical_mem(const char *val, struct kernel_param *kp);
module_param_call(alloc_physical_mem, sec_alloc_physical_mem, NULL, NULL,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

static int sec_free_physical_mem(const char *val, struct kernel_param *kp);
module_param_call(free_physical_mem, sec_free_physical_mem, NULL, NULL,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

static int dbg_set_cpu_affinity(const char *val, struct kernel_param *kp);
module_param_call(setcpuaff, dbg_set_cpu_affinity, NULL, NULL,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

/* klaatu - schedule log */
static void __iomem *upload_cause;

DEFINE_PER_CPU(struct sec_debug_core_t, sec_debug_core_reg);
DEFINE_PER_CPU(struct sec_debug_mmu_reg_t, sec_debug_mmu_reg);
DEFINE_PER_CPU(enum sec_debug_upload_cause_t, sec_debug_upload_cause);

#ifdef CONFIG_SEC_DEBUG_CANCERIZER
static int run_cancerizer_test(const char *val, struct kernel_param *kp);
module_param_call(cancerizer, run_cancerizer_test, NULL, NULL, 0664);
MODULE_PARM_DESC(cancerizer, "Crashing Algorithm!!!");
#endif

#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
/* save last_pet and last_ns with these nice functions */
void sec_debug_save_last_pet(unsigned long long last_pet)
{
	if (secdbg_log)
		secdbg_log->last_pet = last_pet;
}

void sec_debug_save_last_ns(unsigned long long last_ns)
{
	if (secdbg_log)
		atomic64_set(&(secdbg_log->last_ns), last_ns);
}
#endif

#ifdef CONFIG_SEC_DEBUG_FORCE_ERROR
#ifdef CONFIG_HOTPLUG_CPU
static void pull_down_other_cpus(void)
{
	int cpu;

	for_each_online_cpu(cpu) {
		if (0 == cpu)
			continue;
		cpu_down(cpu);
	}
}
#else
static void pull_down_other_cpus(void) {}
#endif

/* timeout for dog bark/bite */
#define DELAY_TIME 20000

static void simulate_apps_wdog_bark(void)
{
	pr_emerg("Simulating apps watch dog bark\n");
	preempt_disable();
	mdelay(DELAY_TIME);
	preempt_enable();
	/* if we reach here, simulation failed */
	pr_emerg("Simulation of apps watch dog bark failed\n");
}

static void simulate_apps_wdog_bite(void)
{
	pull_down_other_cpus();
	pr_emerg("Simulating apps watch dog bite\n");
	local_irq_disable();
	mdelay(DELAY_TIME);
	local_irq_enable();
	/* if we reach here, simulation had failed */
	pr_emerg("Simualtion of apps watch dog bite failed\n");
}

#ifdef CONFIG_SEC_DEBUG_SEC_WDOG_BITE

#define SCM_SVC_SEC_WDOG_TRIG	0x8

static int simulate_secure_wdog_bite(void)
{	int ret;
	struct scm_desc desc = {
		.args[0] = 0,
		.arginfo = SCM_ARGS(1),
	};

	pr_emerg("simulating secure watch dog bite\n");
	if (!is_scm_armv8())
		ret = scm_call_atomic2(SCM_SVC_BOOT,
					       SCM_SVC_SEC_WDOG_TRIG, 0, 0);
	else
		ret = scm_call2_atomic(SCM_SIP_FNID(SCM_SVC_BOOT,
						  SCM_SVC_SEC_WDOG_TRIG), &desc);
	/* if we hit, scm_call has failed */
	pr_emerg("simulation of secure watch dog bite failed\n");

	return ret;
}
#else
int simulate_secure_wdog_bite(void)
{
	return 0;
}
#endif

#if defined(CONFIG_ARCH_MSM8226) || defined(CONFIG_ARCH_MSM8974)
/*
 * Misc data structures needed for simulating bus timeout in
 * camera
 */

#define HANG_ADDRESS 0xfda10000

struct clk_pair {
	const char *dev;
	const char *clk;
};

static struct clk_pair bus_timeout_camera_clocks_on[] = {
	/*
	 * gcc_mmss_noc_cfg_ahb_clk should be on but right
	 * now this clock is on by default and not accessable.
	 * Update this table if gcc_mmss_noc_cfg_ahb_clk is
	 * ever not enabled by default!
	 */
	{
		.dev = "fda0c000.qcom,cci",
		.clk = "camss_top_ahb_clk",
	},
	{
		.dev = "fda10000.qcom,vfe",
		.clk = "iface_clk",
	},
};

static struct clk_pair bus_timeout_camera_clocks_off[] = {
	{
		.dev = "fda10000.qcom,vfe",
		.clk = "camss_vfe_vfe_clk",
	}
};

static void bus_timeout_clk_access(struct clk_pair bus_timeout_clocks_off[],
				struct clk_pair bus_timeout_clocks_on[],
				int off_size, int on_size)
{
	size_t i;

	/*
	 * Yes, none of this cleans up properly but the goal here
	 * is to trigger a hang which is going to kill the rest of
	 * the system anyway
	 */

	for (i = 0; i < on_size; i++) {
		struct clk *this_clock;

		this_clock = clk_get_sys(bus_timeout_clocks_on[i].dev,
					bus_timeout_clocks_on[i].clk);
		if (!IS_ERR(this_clock))
			if (clk_prepare_enable(this_clock))
				pr_warn("Device %s: Clock %s not enabled",
					bus_timeout_clocks_on[i].clk,
					bus_timeout_clocks_on[i].dev);
	}

	for (i = 0; i < off_size; i++) {
		struct clk *this_clock;

		this_clock = clk_get_sys(bus_timeout_clocks_off[i].dev,
					 bus_timeout_clocks_off[i].clk);
		if (!IS_ERR(this_clock))
			clk_disable_unprepare(this_clock);
	}
}

static void simulate_bus_hang(void)
{
	/* This simulates bus timeout on camera */
	int ret;
	uint32_t dummy_value;
	uint32_t address = HANG_ADDRESS;
	void *hang_address;
	struct regulator *r;

	/* simulate */
	hang_address = ioremap(address, SZ_4K);
	r = regulator_get(NULL, "gdsc_vfe");
	ret = IS_ERR(r);
	if (!ret)
		regulator_enable(r);
	else
		pr_emerg("Unable to get regulator reference\n");

	bus_timeout_clk_access(bus_timeout_camera_clocks_off,
				bus_timeout_camera_clocks_on,
				ARRAY_SIZE(bus_timeout_camera_clocks_off),
				ARRAY_SIZE(bus_timeout_camera_clocks_on));

	dummy_value = readl_relaxed(hang_address);
	mdelay(DELAY_TIME);
	/* if we hit here, test had failed */
	pr_emerg("Bus timeout test failed...0x%x\n", dummy_value);
	iounmap(hang_address);
}
#else /* defined(CONFIG_ARCH_MSM8226) || defined(CONFIG_ARCH_MSM8974) */
static void simulate_bus_hang(void)
{
	void __iomem *p;

	pr_emerg("Generating Bus Hang!\n");
	p = ioremap_nocache(0xFC4B8000, 32);
	*(unsigned int *)p = *(unsigned int *)p;
	mb();	/* memory barriar to generate bus hang */
	pr_info("*p = %x\n", *(unsigned int *)p);
	pr_emerg("Clk may be enabled.Try again if it reaches here!\n");
}
#endif /* defined(CONFIG_ARCH_MSM8226) || defined(CONFIG_ARCH_MSM8974) */

static int force_error(const char *val, struct kernel_param *kp)
{
	struct page *page = NULL;
	unsigned int *ptr;
	size_t i;

	/* FIXME: this is a tricky way to prevent error */
	page = page;

	pr_emerg("!!!WARN forced error : %s\n", val);

	if (!strncmp(val, "appdogbark", 10)) {
		pr_emerg("Generating an apps wdog bark!\n");
		simulate_apps_wdog_bark();
	} else if (!strncmp(val, "appdogbite", 10)) {
		pr_emerg("Generating an apps wdog bite!\n");
		simulate_apps_wdog_bite();
	} else if (!strncmp(val, "dabort", 6)) {
		pr_emerg("Generating a data abort exception!\n");
		*(unsigned int *)NULL = 0x0;
	} else if (!strncmp(val, "pabort", 6)) {
		pr_emerg("Generating a prefetch abort exception!\n");
		((void (*)(void))NULL)();
	} else if (!strncmp(val, "undef", 5)) {
		pr_emerg("Generating a undefined instruction exception!\n");
		BUG();
	} else if (!strncmp(val, "bushang", 7)) {
		pr_emerg("Generating a Bus Hang!\n");
		simulate_bus_hang();
	} else if (!strncmp(val, "dblfree", 7)) {
		ptr = kmalloc(sizeof(unsigned int), GFP_KERNEL);
		kfree(ptr);
		msleep(1000);
		kfree(ptr);
	} else if (!strncmp(val, "danglingref", 11)) {
		ptr = kmalloc(sizeof(unsigned int), GFP_KERNEL);
		kfree(ptr);
		*ptr = 0x1234;
	} else if (!strncmp(val, "lowmem", 6)) {
		pr_emerg("Allocating memory until failure!\n");
		for (i = 0; kmalloc(128 * 1024, GFP_KERNEL); i++)
			;
		pr_emerg("Allocated %zu KB!\n", i * 128);
	} else if (!strncmp(val, "memcorrupt", 10)) {
		ptr = kmalloc(sizeof(unsigned int), GFP_KERNEL);
		*ptr++ = 4;
		*ptr = 2;
		panic("MEMORY CORRUPTION");
#ifdef CONFIG_SEC_DEBUG_SEC_WDOG_BITE
	} else if (!strncmp(val, "secdogbite", 10)) {
		simulate_secure_wdog_bite();
#endif
#ifdef CONFIG_FREE_PAGES_RDONLY
	} else if (!strncmp(val, "pageRDcheck", 11)) {
		page = alloc_pages(GFP_ATOMIC, 0);
		ptr = (unsigned int *)page_address(page);
		pr_emerg("Test with RD page configue");
		__free_pages(page, 0);
		*ptr = 0x12345678;
#endif
#ifdef CONFIG_USER_RESET_DEBUG_TEST
	} else if (!strncmp(val, "TP", 2)) {
		force_thermal_reset();
	} else if (!strncmp(val, "KP", 2)) {
		pr_emerg("Generating a data abort exception!\n");
		*(unsigned int *)0x0 = 0x0;
	} else if (!strncmp(val, "DP", 2)) {
		force_watchdog_bark();
	} else if (!strncmp(val, "WP", 2)) {
		simulate_secure_wdog_bite();
#endif
	} else {
		pr_emerg("No such error defined for now!\n");
	}

	return 0;
}
#endif /* CONFIG_SEC_DEBUG_FORCE_ERROR */

#ifdef CONFIG_SEC_DEBUG_CANCERIZER
/* core algorithm written in asm codes
 */
static noinline void cancerizer_core(char *buf, unsigned int *curr_lfsr)
{
#ifdef CONFIG_ARM64
	asm volatile(
	/* LFSR
	 */
	"	ldr	w2, [%1]\n"
	"	mov	w3, %2\n"
	"	and	w4, w2, #1\n"
	"	neg	w4, w4\n"
	"	and	w4, w3, w4\n"
	"	eor	w2, w4, w2, lsr #1\n"
	"	str	w2, [%1]\n"
	/* pick a bit
	 */
	"	lsr	w4, w2, #3\n"
	"	prfm	pstl1strm, [%0, x4]\n"
	"	ldrb	w3, [%0, x4]\n"
	"	and	w4, w2, #7\n"
	"	mov	w5, #1\n"
	"	lsl	w5, w5, w4\n"
	"	and	w5, w3, w5\n"
	"	lsr	w5, w5, w4\n"
	/* bit-validation
	 */
	"	mov	x6, x5\n"
	"	mov	x7, x6\n"
	"	mov	x8, x7\n"
	"	mov	x9, x8\n"
	"	mov	x10, x9\n"
	"	mov	x11, x10\n"
	"	mov	x12, x11\n"
	"	mov	x13, x12\n"
	"	mov	x14, x13\n"
	"	mov	x15, x14\n"
	"	mov	x16, x15\n"
	"	mov	x17, x16\n"
	"	mov	x18, x17\n"
	"	udiv	%0, %0, x18\n"
	/* clear the bit
	 */
	"	lsl	w5, w5, w4\n"
	"	bic	w3, w3, w5\n"
	"	lsr	w4, w2, #3\n"
	"	strb	w3, [%0, x4]\n"
	:
	: "r" (buf), "r" (curr_lfsr), "M" (POLY_DEGREE29)
	: "cc", "memory");
#else
	unsigned int poly = POLY_DEGREE29;
	asm volatile(
	/* LFSR
	 */
	"	ldr	r2, [%1]\n"
	"	ldr	r3, [%2]\n"
	"	and	r4, r2, #1\n"
	"	neg	r4, r4\n"
	"	and	r4, r3, r4\n"
	"	eor	r2, r4, r2, lsr #1\n"
	"	str	r2, [%1]\n"
	/* pick a bit
	 */
	"	lsr	r4, r2, #3\n"
	"	ldrb	r3, [%0, r4]\n"
	"	and	r4, r2, #7\n"
	"	mov	r5, #1\n"
	"	lsl	r5, r5, r4\n"
	"	and	r5, r3, r5\n"
	"	lsr	r5, r5, r4\n"
	/* bit-validation
	 */
	"	teq	r5, #1\n"
	"	bne	2f\n"
	/* clear the bit
	 */
	"1:	lsl	r5, r5, r4\n"
	"	bic	r3, r3, r5\n"
	"	lsr	r4, r2, #3\n"
	"	strb	r3, [%0, r4]\n"
	"	b	3f\n"
	"2:	mov	%0, #0\n"
	"	b	1b\n"
	"3:	nop\n"
	:
	: "r" (buf), "r" (curr_lfsr), "r" (&poly)
	: "cc", "memory");
#endif
}

static int cancerizer_thread(void *arg)
{
	int ret = 0;
	unsigned int lfsr, loop_cnt = 0;
	char *ptr_memtest;

	/* D thread should not exit until monitor kicks off.
	 */
	wait_event_interruptible(cmctx.dwq, !completion_done(&cmctx.done));

	/* preparation of insanity core loop;
	 * memory allocation up to 2^DEGREE - 1 bits
	 */
	ptr_memtest = vmalloc(SZ_TEST);
	if (ptr_memtest) {
		printk(KERN_ERR "%s: Be aware of starting probabilistic crash algorithm on %s\n",
			 __func__, current->comm);

		memset(ptr_memtest, 0xFF, SZ_TEST);

		/* assign a nonzero seed to LFSR */
		while ((lfsr = (get_random_int() & POWEROF29)) == 0)
			;

		while (loop_cnt++ < POWEROF29)
			cancerizer_core(ptr_memtest, &lfsr);

		vfree(ptr_memtest);

		printk(KERN_ERR "%s: Finished the algorithm without any fatal accident on %s..\n",
			 __func__, current->comm);
	} else {
		printk(KERN_ERR "%s: Failed to allocate enough memory\n", __func__);
		ret = -ENOMEM;
	}

	if (atomic_dec_and_test(&cmctx.nr_running))
		complete(&cmctx.done);

	/* The duty of this thread is only limited to a single turn test.
	 * Wait for being killed by monitor.
	 */
	while (!kthread_should_stop())
		msleep_interruptible(10);

	return ret;
}

static int cancerizer_monitor(void *arg)
{
	int cnt, ret;

	while (!kthread_should_stop()) {
		/* Monitor should sleep unless nr_test is nonzero.
		 */
		wait_event_interruptible(cmctx.mwq, atomic_read(&cmctx.nr_test));

		/* nr_test can be overrided by an user request.
		 */
		do {
			/* Get ready to check if whole test sessions are done.
			 */
			reinit_completion(&cmctx.done);

			/* Multiple threads to run core algorithm.
			 * The number of threads does not need to be specified here,
			 * creating threads matching to NR_CPUS is an alternative choice though.
			 */
			for (cnt = 0; cnt < NR_CANCERIZERD; cnt++) {
				cmctx.td[cnt] = kthread_create(cancerizer_thread, NULL,
							"cancerizerd/%d", cnt);
				if (IS_ERR(cmctx.td[cnt])) {
					ret = PTR_ERR(cmctx.td[cnt]);
					cmctx.td[cnt] = NULL;
					return ret;
				}
				atomic_inc(&cmctx.nr_running);
			}

			/* Start to run core algorithm.
			 */
			for (cnt = 0; cnt < NR_CANCERIZERD; cnt++)
				wake_up_process(cmctx.td[cnt]);

			wait_for_completion_interruptible(&cmctx.done);

			/* Kill the D threads.
			 */
			for (cnt = 0; cnt < NR_CANCERIZERD; cnt++)
				kthread_stop(cmctx.td[cnt]);
		} while (atomic_dec_return(&cmctx.nr_test));
	}

	return 0;
}

static int run_cancerizer_test(const char *val, struct kernel_param *kp)
{
	int test_case = (int)simple_strtoul((const char *)val, NULL, 10);

	if (cmctx.self) {
		atomic_set(&cmctx.nr_test, (test_case == 0) ? NR_INFINITE : test_case);
		wake_up_process(cmctx.self);
	}

	return 0;
}
#endif

static long *g_allocated_phys_mem;
static long *g_allocated_virt_mem;

static int sec_alloc_virtual_mem(const char *val, struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t size = (size_t)memparse(str, &str);

	if (size) {
		mem = vmalloc(size);
		if (!mem) {
			pr_err("%s: Failed to allocate virtual memory of size: 0x%zx bytes\n",
					__func__, size);
		} else {
			pr_info("%s: Allocated virtual memory of size: 0x%zx bytes\n",
					__func__, size);
			*mem = (long)g_allocated_virt_mem;
			g_allocated_virt_mem = mem;

			return 0;
		}
	}

	pr_info("%s: Invalid size: %s bytes\n", __func__, val);

	return -EAGAIN;
}

static int sec_free_virtual_mem(const char *val, struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t free_count = (size_t)memparse(str, &str);

	if (!free_count) {
		if (strncmp(val, "all", 4)) {
			free_count = 10;
		} else {
			pr_err("%s: Invalid free count: %s\n", __func__, val);
			return -EAGAIN;
		}
	}

	if (free_count > 10)
		free_count = 10;

	if (!g_allocated_virt_mem) {
		pr_err("%s: No virtual memory chunk to free.\n", __func__);
		return 0;
	}

	while (g_allocated_virt_mem && free_count--) {
		mem = (long *) *g_allocated_virt_mem;
		vfree(g_allocated_virt_mem);
		g_allocated_virt_mem = mem;
	}

	pr_info("%s: Freed previously allocated virtual memory chunks.\n",
			__func__);

	if (g_allocated_virt_mem)
		pr_info("%s: Still, some virtual memory chunks are not freed. Try again.\n",
				__func__);

	return 0;
}

static int sec_alloc_physical_mem(const char *val, struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t size = (size_t)memparse(str, &str);

	if (size) {
		mem = kmalloc(size, GFP_KERNEL);
		if (!mem) {
			pr_err("%s: Failed to allocate physical memory of size: 0x%zx bytes\n",
					__func__, size);
		} else {
			pr_info("%s: Allocated physical memory of size: 0x%zx bytes\n",
					__func__, size);
			*mem = (long) g_allocated_phys_mem;
			g_allocated_phys_mem = mem;

			return 0;
		}
	}

	pr_info("%s: Invalid size: %s bytes\n", __func__, val);

	return -EAGAIN;
}

static int sec_free_physical_mem(const char *val, struct kernel_param *kp)
{
	long *mem;
	char *str = (char *) val;
	size_t free_count = (size_t)memparse(str, &str);

	if (!free_count) {
		if (strncmp(val, "all", 4)) {
			free_count = 10;
		} else {
			pr_info("%s: Invalid free count: %s\n", __func__, val);
			return -EAGAIN;
		}
	}

	if (free_count > 10)
		free_count = 10;

	if (!g_allocated_phys_mem) {
		pr_info("%s: No physical memory chunk to free.\n", __func__);
		return 0;
	}

	while (g_allocated_phys_mem && free_count--) {
		mem = (long *) *g_allocated_phys_mem;
		kfree(g_allocated_phys_mem);
		g_allocated_phys_mem = mem;
	}

	pr_info("%s: Freed previously allocated physical memory chunks.\n",
			__func__);

	if (g_allocated_phys_mem)
		pr_info("%s: Still, some physical memory chunks are not freed. Try again.\n",
				__func__);

	return 0;
}

static int dbg_set_cpu_affinity(const char *val, struct kernel_param *kp)
{
	char *endptr;
	pid_t pid;
	int cpu;
	struct cpumask mask;
	long ret;

	pid = (pid_t)memparse(val, &endptr);
	if (*endptr != '@') {
		pr_info("%s: invalid input strin: %s\n", __func__, val);
		return -EINVAL;
	}

	cpu = (int)memparse(++endptr, &endptr);
	cpumask_clear(&mask);
	cpumask_set_cpu(cpu, &mask);
	pr_info("%s: Setting %d cpu affinity to cpu%d\n",
				__func__, pid, cpu);

	ret = sched_setaffinity(pid, &mask);
	pr_info("%s: sched_setaffinity returned %ld\n", __func__, ret);

	return 0;
}

/* for sec debug level */
static int __init sec_debug_level(char *str)
{
	get_option(&str, &sec_dbg_level);
	return 0;
}
early_param("level", sec_debug_level);

static void sec_debug_set_qc_dload_magic(int on)
{
	pr_info("%s: on=%d\n", __func__, on);
	set_dload_mode(on);
}

#define PON_RESTART_REASON_NOT_HANDLE	PON_RESTART_REASON_MAX
#define RESTART_REASON_NOT_HANDLE	RESTART_REASON_END

/* This is shared with 'msm-poweroff.c'  module. */
static enum sec_restart_reason_t __iomem *qcom_restart_reason;

static void sec_debug_set_upload_magic(unsigned magic)
{
	pr_emerg("(%s): %x\n", __func__, magic);

	if (magic)
		sec_debug_set_qc_dload_magic(1);
	__raw_writel(magic, qcom_restart_reason);

	flush_cache_all();
#ifndef CONFIG_ARM64
	outer_flush_all();
#endif
}

static int sec_debug_normal_reboot_handler(struct notifier_block *nb,
		unsigned long action, void *data)
{
	char recovery_cause[256];

	set_dload_mode(0);	/* set defalut (not upload mode) */

	sec_debug_set_upload_magic(RESTART_REASON_NORMAL);

	if (unlikely(!data))
		return 0;

	if ((action == SYS_RESTART) &&
		!strncmp((char *)data, "recovery", 8)) {
		sec_get_param(param_index_reboot_recovery_cause, recovery_cause);
		if (!recovery_cause[0] || !strlen(recovery_cause)) {
			snprintf(recovery_cause, sizeof(recovery_cause),
				 "%s:%d ", current->comm, task_pid_nr(current));
			sec_set_param(param_index_reboot_recovery_cause, recovery_cause);
		}
	}

	return 0;
}

void sec_debug_update_dload_mode(const int restart_mode, const int in_panic)
{
#ifdef CONFIG_SEC_DEBUG_LOW_LOG
	if (sec_debug_is_enabled() &&
	    ((RESTART_DLOAD == restart_mode) || in_panic))
		set_dload_mode(1);
	else
		set_dload_mode(0);
#else
	/* FIXME: dead code? */
	/* set_dload_mod((RESTART_DLOAD == restart_mode) || in_panic); */
#endif
}

static inline void __sec_debug_set_restart_reason(enum sec_restart_reason_t __r)
{
	__raw_writel((u32)__r, qcom_restart_reason);
}

void sec_debug_update_restart_reason(const char *cmd, const int in_panic)
{
	struct __magic {
		const char *cmd;
		enum pon_restart_reason pon_rr;
		enum sec_restart_reason_t sec_rr;
	} magic[] = {
		{ "sec_debug_hw_reset",
			PON_RESTART_REASON_NOT_HANDLE,
			RESTART_REASON_SEC_DEBUG_MODE, },
		{ "userrequested",
			PON_RESTART_REASON_NORMALBOOT,
			RESTART_REASON_NOT_HANDLE, },
		{ "GlobalActions restart",
			PON_RESTART_REASON_NORMALBOOT,
			RESTART_REASON_NOT_HANDLE, },
		{ "download",
			PON_RESTART_REASON_DOWNLOAD,
			RESTART_REASON_NOT_HANDLE, },
		{ "nvbackup",
			PON_RESTART_REASON_NVBACKUP,
			RESTART_REASON_NOT_HANDLE, },
		{ "nvrestore",
			PON_RESTART_REASON_NVRESTORE,
			RESTART_REASON_NOT_HANDLE, },
		{ "nverase",
			PON_RESTART_REASON_NVERASE,
			RESTART_REASON_NOT_HANDLE, },
		{ "nvrecovery",
			PON_RESTART_REASON_NVRECOVERY,
			RESTART_REASON_NOT_HANDLE, },
		{ "cpmem_on",
			PON_RESTART_REASON_CP_MEM_RESERVE_ON,
			RESTART_REASON_NOT_HANDLE, },
		{ "cpmem_off",
			PON_RESTART_REASON_CP_MEM_RESERVE_OFF,
			RESTART_REASON_NOT_HANDLE, },
		{ "mbsmem_on",
			PON_RESTART_REASON_MBS_MEM_RESERVE_ON,
			RESTART_REASON_NOT_HANDLE, },
		{ "mbsmem_off",
			PON_RESTART_REASON_MBS_MEM_RESERVE_OFF,
			RESTART_REASON_NOT_HANDLE, },
	};
	unsigned long opt_code;
	unsigned long value;
	enum pon_restart_reason pon_rr = (!in_panic) ?
				PON_RESTART_REASON_NORMALBOOT :
				PON_RESTART_REASON_KERNEL_PANIC;
	enum sec_restart_reason_t sec_rr = RESTART_REASON_NORMAL;
	char cmd_buf[16];
	size_t i;

	if (!cmd || !strlen(cmd))
		goto __done;

	if (!strncmp(cmd, "oem-", strlen("oem-"))) {
		pon_rr = PON_RESTART_REASON_UNKNOWN;
		goto __done;
	} else if (!strncmp(cmd, "sud", 3) && !kstrtoul(cmd + 3, 0, &value)) {
		pon_rr = (PON_RESTART_REASON_RORY_START | value) ;
		goto __done;
	} else if (!strncmp(cmd, "debug", strlen("debug")) &&
		   !kstrtoul(cmd + strlen("debug"), 0, &opt_code)) {
		switch (opt_code) {
		case 0x4f4c:
			pon_rr = PON_RESTART_REASON_DBG_LOW;
			break;
		case 0x494d:
			pon_rr = PON_RESTART_REASON_DBG_MID;
			break;
		case 0x4948:
			pon_rr = PON_RESTART_REASON_DBG_HIGH;
			break;
		default:
			pon_rr = PON_RESTART_REASON_UNKNOWN;
		}
		goto __done;
	} else if (!strncmp(cmd, "cpdebug", strlen("cpdebug")) &&
		   !kstrtoul(cmd + strlen("cpdebug"), 0, &opt_code)) {
		switch (opt_code) {
		case 0x5500:
			pon_rr = PON_RESTART_REASON_CP_DBG_ON;
			break;
		case 0x55ff:
			pon_rr = PON_RESTART_REASON_CP_DBG_OFF;
			break;
		default:
			pon_rr = PON_RESTART_REASON_UNKNOWN;
		}
		goto __done;
	}
#ifdef CONFIG_MUIC_SUPPORT_RUSTPROOF
	else if (!strncmp(cmd, "swsel", 5) /* set switch value */
			&& !kstrtoul(cmd + 5, 0, &opt_code)) {
		opt_code = (((opt_code & 0x8) >> 1) | opt_code) & 0x7;
		pon_rr = (PON_RESTART_REASON_SWITCHSEL | opt_code);
		goto __done;
	}
#endif

	for (i = 0; i < ARRAY_SIZE(magic); i++) {
		size_t len = strlen(magic[i].cmd);

		if (strncmp(cmd, magic[i].cmd, len))
			continue;

		pon_rr = magic[i].pon_rr;
		sec_rr = magic[i].sec_rr;

		goto __done;
	}


	strlcpy(cmd_buf, cmd, ARRAY_SIZE(cmd_buf));
	pr_warn("(%s): unknown reboot command : %s\n", __func__, cmd_buf);

	__sec_debug_set_restart_reason(RESTART_REASON_REBOOT);

	return;

__done:
	if (PON_RESTART_REASON_NOT_HANDLE != pon_rr)
		qpnp_pon_set_restart_reason(pon_rr);
	if (RESTART_REASON_NOT_HANDLE != sec_rr)
		__sec_debug_set_restart_reason(sec_rr);
	pr_err("(%s): restart_reason = 0x%x(0x%x)\n", __func__, sec_rr, pon_rr);
}

static void sec_debug_set_upload_cause(enum sec_debug_upload_cause_t type)
{
	if (unlikely(!upload_cause)) {
		pr_err("upload cause address unmapped.\n");
	} else {
		per_cpu(sec_debug_upload_cause, smp_processor_id()) = type;
		__raw_writel(type, upload_cause);

		pr_emerg("(%s) %x\n", __func__, type);
	}
}

extern struct uts_namespace init_uts_ns;
void sec_debug_hw_reset(void)
{
	pr_emerg("(%s) %s %s\n", __func__, init_uts_ns.name.release,
					init_uts_ns.name.version);
	pr_emerg("(%s) rebooting...\n", __func__);
	flush_cache_all();
#ifndef CONFIG_ARM64
	outer_flush_all();
#endif
	arm_pm_restart(0, "sec_debug_hw_reset");

	/* while (1) ; */
	asm volatile ("b .");
}

#ifdef CONFIG_SEC_PERIPHERAL_SECURE_CHK
void sec_peripheral_secure_check_fail(void)
{
	/* sec_debug_set_upload_magic(0x77665507); */
	sec_debug_set_qc_dload_magic(0);
	pr_emerg("(%s) %s %s\n", __func__, init_uts_ns.name.release,
					init_uts_ns.name.version);
	pr_emerg("(%s) rebooting...\n", __func__);
	flush_cache_all();
#ifndef CONFIG_ARM64
	outer_flush_all();
#endif
	arm_pm_restart(0, "peripheral_hw_reset");

	/* while (1) ; */
	asm volatile ("b .");
}
#endif

void sec_debug_set_thermal_upload(void)
{
	pr_emerg("(%s) set thermal upload cause\n", __func__);
	sec_debug_set_upload_magic(0x776655ee);
	sec_debug_set_upload_cause(UPLOAD_CAUSE_POWER_THERMAL_RESET);
}
EXPORT_SYMBOL(sec_debug_set_thermal_upload);

void sec_debug_print_model(struct seq_file *m, const char *cpu_name)
{
	u32 cpuid = read_cpuid_id();

	seq_printf(m, "model name\t: %s rev %d (%s)\n",
		   cpu_name, cpuid & 15, ELF_PLATFORM);
}

#ifdef CONFIG_USER_RESET_DEBUG
static inline void sec_debug_store_backtrace(void)
{
	unsigned long flags;

	local_irq_save(flags);
	sec_debug_backtrace();
	local_irq_restore(flags);
}
#endif /* CONFIG_USER_RESET_DEBUG */

static int sec_debug_panic_handler(struct notifier_block *nb,
		unsigned long l, void *buf)
{
	size_t len;
	int timeout = 100; /* means timeout * 100ms */

	emerg_pet_watchdog();	/* CTC-should be modify */
#ifdef CONFIG_USER_RESET_DEBUG
	sec_debug_store_backtrace();
#endif
	sec_debug_set_upload_magic(RESTART_REASON_SEC_DEBUG_MODE);

	len = strnlen(buf, 80);
	if (!strncmp(buf, "User Fault", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_USER_FAULT);
	else if (!strncmp(buf, "Crash Key", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_FORCED_UPLOAD);
	else if (!strncmp(buf, "User Crash Key", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_USER_FORCED_UPLOAD);
	else if (!strncmp(buf, "CP Crash", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_CP_ERROR_FATAL);
	else if (!strncmp(buf, "MDM Crash", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_MDM_ERROR_FATAL);
	else if (strnstr(buf, "external_modem", len) != NULL)
		sec_debug_set_upload_cause(UPLOAD_CAUSE_MDM_ERROR_FATAL);
	else if (strnstr(buf, "esoc0 crashed", len) != NULL)
		sec_debug_set_upload_cause(UPLOAD_CAUSE_MDM_ERROR_FATAL);
	else if (strnstr(buf, "modem", len) != NULL)
		sec_debug_set_upload_cause(UPLOAD_CAUSE_MODEM_RST_ERR);
	else if (strnstr(buf, "riva", len) != NULL)
		sec_debug_set_upload_cause(UPLOAD_CAUSE_RIVA_RST_ERR);
	else if (strnstr(buf, "lpass", len) != NULL)
		sec_debug_set_upload_cause(UPLOAD_CAUSE_LPASS_RST_ERR);
	else if (strnstr(buf, "dsps", len) != NULL)
		sec_debug_set_upload_cause(UPLOAD_CAUSE_DSPS_RST_ERR);
	else if (!strnicmp(buf, "subsys", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_PERIPHERAL_ERR);
#if defined(CONFIG_SEC_NAD)
	else if (!strnicmp(buf, "nad_srtest", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_NAD_SUSPEND);
	else if (!strnicmp(buf, "nad_qmesaddr", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_NAD_QMESADDR);
	else if (!strnicmp(buf, "nad_qmesacache", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_NAD_QMESACACHE);
	else if (!strnicmp(buf, "nad_pmic", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_NAD_PMIC);
	else if (!strnicmp(buf, "nad_fail", len))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_NAD_FAIL);
#endif	
	else
		sec_debug_set_upload_cause(UPLOAD_CAUSE_KERNEL_PANIC);

	if (!enable) {
#ifdef CONFIG_SEC_DEBUG_LOW_LOG
		sec_debug_hw_reset();
#endif
		return -EPERM;
	}

	/* wait for all cpus to be deactivated */
	while (num_active_cpus() > 1 && timeout--) {
		touch_nmi_watchdog();
		mdelay(100);
	}

	/* save context here so that function call after this point doesn't
	 * corrupt stacks below the saved sp
	 */
	sec_debug_save_context();
	sec_debug_hw_reset();

	return 0;
}

void sec_debug_prepare_for_wdog_bark_reset(void)
{
	sec_debug_set_upload_magic(RESTART_REASON_SEC_DEBUG_MODE);
	sec_debug_set_upload_cause(UPLOAD_CAUSE_NON_SECURE_WDOG_BARK);
}

struct sec_crash_key {
	char name[128];			/* name */
	unsigned int *keycode;		/* keycode array */
	unsigned int size;		/* number of used keycode */
	unsigned int timeout;		/* msec timeout */
	unsigned int unlock;		/* unlocking mask value */
	unsigned int trigger;		/* trigger key code */
	unsigned int knock;		/* number of triggered */
	void (*callback)(void);		/* callback function when the key triggered */
	struct list_head node;
};

struct sec_crash_key_callback {
	char name[128];
	void (*function)(void);
};

static unsigned int __crash_key[] = {
	KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_POWER, KEY_HOMEPAGE, KEY_ENDCALL, KEY_WINK
};

static unsigned int default_crash_key_code[] = {
	KEY_VOLUMEDOWN, KEY_POWER, KEY_POWER
};

static struct sec_crash_key dflt_crash_key = {
	.keycode	= default_crash_key_code,
	.size		= ARRAY_SIZE(default_crash_key_code),
	.timeout	= 1000,	/* 1 sec */
};

static unsigned int max_key_size;
static unsigned int *__key_store;
static struct sec_crash_key *__sec_crash_key = &dflt_crash_key;

static inline void __cb_dummy(void)
{
	pr_info("(%s) callback is not registered for the key combination.\n",
			__func__);
}

static inline void __cb_keycrash(void)
{
	emerg_pet_watchdog();
	dump_stack();
	dump_all_task_info();
	dump_cpu_stat();
	panic("Crash Key");
}

#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
struct tsp_dump_callbacks dump_callbacks;
static inline void __cb_tsp_dump(void)
{
	pr_info("%s %s\n", SECLOG, __func__);
	if (dump_callbacks.inform_dump)
		dump_callbacks.inform_dump();
}
#endif

static struct sec_crash_key_callback sec_debug_crash_key_cb[] = {
	{ "keycrash", &__cb_keycrash },
#if defined(CONFIG_TOUCHSCREEN_DUMP_MODE)
	{ "tsp_dump", &__cb_tsp_dump },
#endif
};

static inline void __init __register_crash_key_cb(
		struct device_node *nc, struct sec_crash_key *crash_key)
{
	int i, ret;
	const char *function;
	struct sec_crash_key_callback *__crash_key_cb;

	crash_key->callback = __cb_dummy;
	ret = of_property_read_string(nc, "function", &function);
	if (unlikely(!ret)) {
		for (i = 0; i < ARRAY_SIZE(sec_debug_crash_key_cb); i++) {
			__crash_key_cb = &sec_debug_crash_key_cb[i];
			if (!strcmp(function, __crash_key_cb->name)) {
				crash_key->callback =
					 __crash_key_cb->function;
			}
		}
	}
}

static unsigned int sec_debug_unlock_crash_key(unsigned int value,
		unsigned int *unlock)
{
	unsigned int ret = 0;
	unsigned int i = __sec_crash_key->size -
		__sec_crash_key->knock - 1;	/* except triggers */

	do {
		if (value == __sec_crash_key->keycode[i]) {
			ret = 1;
			*unlock |= 1 << i;
		}
	} while (i-- != 0);

	return ret;
}

static LIST_HEAD(key_crash_list);

static struct sec_crash_key *sec_debug_get_first_key_entry(void)
{
	if (list_empty(&key_crash_list)) {
		pr_err("(%s) key crash list is empty.\n", __func__);
		return NULL;
	}

	return list_first_entry(&key_crash_list, struct sec_crash_key, node);
}

static void sec_debug_check_crash_key(unsigned int value, int pressed)
{
	static unsigned long unlock_jiffies;
	static unsigned long trigger_jiffies;
	static bool other_key_pressed;
	static unsigned int unlock;
	static unsigned int knock;
	static size_t k_idx;
	unsigned int timeout;
	static bool vd_key_pressed, pwr_key_pressed;

	if (!pressed) { // key released
		if (value == KEY_POWER) {
			printk(KERN_ERR "%s %s:[%s]\n", __func__, "POWER_KEY", pressed ? "P":"R");
			pwr_key_pressed = false;
		} else if (value == KEY_VOLUMEDOWN) {
			printk(KERN_ERR "%s %s:[%s]\n", __func__, "VOL_DOWN", pressed ? "P":"R");
			vd_key_pressed = false;
		}

		if (!(vd_key_pressed && pwr_key_pressed)) {
			sec_debug_set_upload_cause(UPLOAD_CAUSE_INIT);
		}

		if (unlock != __sec_crash_key->unlock ||
		    value != __sec_crash_key->trigger)
			goto __clear_all;
		else
			return;
	}

	if (value == KEY_POWER) {
		printk(KERN_ERR "%s %s:[%s]\n", __func__, "POWER_KEY", pressed ? "P":"R");
		pwr_key_pressed = true;
	} else if (value == KEY_VOLUMEDOWN) {
		printk(KERN_ERR "%s %s:[%s]\n", __func__, "VOL_DOWN", pressed ? "P":"R");
		vd_key_pressed = true;
	}

	if ((value == KEY_VOLUMEDOWN) || (value == KEY_POWER))
		if (vd_key_pressed && pwr_key_pressed)
			sec_debug_set_upload_cause(UPLOAD_CAUSE_POWER_LONG_PRESS);

	if (unlikely(!sec_debug_is_enabled()))
		return;

	__key_store[k_idx++] = value;

	if (sec_debug_unlock_crash_key(value, &unlock)) {
		if (unlock == __sec_crash_key->unlock && !unlock_jiffies)
			unlock_jiffies = jiffies;
	} else if (value == __sec_crash_key->trigger) {
		trigger_jiffies = jiffies;
		knock++;
	} else {
		size_t i;
		struct list_head *head = &__sec_crash_key->node;

		if (list_empty(&__sec_crash_key->node))
			goto __clear_all;

		list_for_each_entry(__sec_crash_key, head, node) {
			if (&__sec_crash_key->node == &key_crash_list || __sec_crash_key->size < k_idx)
				continue;

			for (i = 0; i < k_idx; i++) {
				if (__sec_crash_key->keycode[i] !=
						__key_store[i])
					break;
			}

			if (i == k_idx) {
				if (sec_debug_unlock_crash_key(value,
								&unlock)) {
					if (unlock == __sec_crash_key->unlock
						&& !unlock_jiffies)
						unlock_jiffies = jiffies;
				} else {
					trigger_jiffies = jiffies;
					knock++;
				}
				goto __next;
			}
		}
		other_key_pressed = true;
		goto __clear_timeout;
	}

__next:
	if (unlock_jiffies && trigger_jiffies && !other_key_pressed &&
	    time_after(trigger_jiffies, unlock_jiffies)) {
		timeout = jiffies_to_msecs(trigger_jiffies - unlock_jiffies);
		if (timeout >= __sec_crash_key->timeout) {
			goto __clear_all;
		} else {
			if (knock == __sec_crash_key->knock)
				__sec_crash_key->callback();
			return;
		}
	}

	if (k_idx < max_key_size)
		return;

__clear_all:
	other_key_pressed = false;
__clear_timeout:
	k_idx = 0;
	__sec_crash_key = sec_debug_get_first_key_entry();

	unlock_jiffies = 0;
	trigger_jiffies = 0;
	unlock = 0;
	knock = 0;
}

static int sec_debug_keyboard_call(struct notifier_block *this,
				unsigned long type, void *data)
{
	struct sec_key_notifier_param *param = data;

	sec_debug_check_crash_key(param->keycode, param->down);

	return NOTIFY_DONE;

}

static struct notifier_block sec_debug_keyboard_notifier = {
	.notifier_call = sec_debug_keyboard_call,
};

static int __check_crash_key(struct sec_crash_key *sck)
{
	if (!list_empty(&key_crash_list)) {
		size_t i;
		struct sec_crash_key *crash_key;

		list_for_each_entry(crash_key, &key_crash_list, node) {
			if (sck->size != crash_key->size)
				continue;

			for (i = 0; i < sck->size; i++) {
				if (crash_key->keycode[i] !=
						sck->keycode[i])
					break;
			}

			if (i == sck->size)
				return -ECANCELED;
		}
	}

	return 0;
}

static inline void __init __sec_debug_init_crash_key(void)
{
	int i;
	struct sec_crash_key *crash_key = NULL;

#ifdef CONFIG_OF
	struct device_node *np, *nc = NULL;
	unsigned int timeout, size_keys, j;
	unsigned int key;

	np = of_find_node_by_path("/sec-debug");
	if (unlikely(!np)) {
		pr_err("(%s) could not find sec-debug node...\n", __func__);
		return;
	}

	while ((nc = of_get_next_child(np, nc))) {
		if (of_property_read_u32(nc, "size-keys", &size_keys))
			continue;

		if (unlikely(size_keys > MAX_SIZE_KEY)) {
			pr_err("Size of '%s' should be less than %d\n",
					nc->name, MAX_SIZE_KEY + 1);
			continue;
		}

		if (max_key_size < size_keys)
			max_key_size = size_keys;

		crash_key = kmalloc(sizeof(struct sec_crash_key), GFP_KERNEL);
		if (unlikely(!crash_key)) {
			pr_err("unable to allocate memory for crash key.\n");
			return;
		}

		crash_key->keycode = kmalloc(sizeof(unsigned int) *
						size_keys, GFP_KERNEL);
		crash_key->size = size_keys;
		for (i = 0; i < size_keys; i++) {
			of_property_read_u32_index(nc, "keys", i, &key);
			for (j = 0; j < ARRAY_SIZE(__crash_key); j++)
				if (key == __crash_key[j]) {
					crash_key->keycode[i] = key;
					break;
				}

			if (j == ARRAY_SIZE(__crash_key))
				goto __unknown_key;
		}

		if (!of_property_read_u32(nc, "timeout", &timeout))
			crash_key->timeout = timeout;
		else
			crash_key->timeout = dflt_crash_key.timeout;

		crash_key->trigger = crash_key->keycode[crash_key->size - 1];
		crash_key->knock = 1;

		for (i = crash_key->size - 2; i >= 0; i--) {
			if (crash_key->keycode[i] != crash_key->trigger)
				break;
			crash_key->knock++;
		}

		crash_key->unlock = 0;
		for (i = 0; i < crash_key->size - crash_key->knock; i++)
			crash_key->unlock |= 1 << i;

		strlcpy(crash_key->name, nc->name, sizeof(crash_key->name));
		__register_crash_key_cb(nc, crash_key);
		if (__check_crash_key(crash_key)) {
			pr_err("(%s) duplicated key combination found in %s\n",
					__func__, nc->name);
			goto __unregister_key;
		}

		list_add_tail(&crash_key->node, &key_crash_list);
		pr_info("(%s) %s registered.\n",
					__func__, crash_key->name);
		continue;
__unknown_key:
		pr_err("(%s) unknown key found in %s\n",
					__func__, nc->name);
__unregister_key:
		kfree(crash_key->keycode);
		kfree(crash_key);
	}
#endif

	__sec_crash_key = sec_debug_get_first_key_entry();
	if (!__sec_crash_key)
		return;

	__key_store = kmalloc_array(max_key_size, sizeof(unsigned int),
			GFP_KERNEL);
	if (unlikely(!__key_store)) {
		pr_err("(%s) unable to allocate memory for crash key store.\n",
				__func__);
		goto __error;
	}

	sec_kn_register_notifier(&sec_debug_keyboard_notifier);
	return;

__error:
	list_for_each_entry(crash_key, &__sec_crash_key->node, node) {
		list_del(&crash_key->node);

		if (!crash_key->keycode)
			kfree(crash_key->keycode);
		kfree(crash_key);
	}
}

static int __init sec_debug_init_crash_key(void)
{
	__sec_debug_init_crash_key();

	return 0;
}
fs_initcall_sync(sec_debug_init_crash_key);

static struct notifier_block nb_reboot_block = {
	.notifier_call = sec_debug_normal_reboot_handler,
};

static struct notifier_block nb_panic_block = {
	.notifier_call = sec_debug_panic_handler,
	.priority = -1,
};

#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
static int __init __init_sec_debug_log(void)
{
	size_t i;
	struct sec_debug_log *vaddr;
	size_t size;

	if (secdbg_paddr == 0 || secdbg_size == 0) {
		pr_info("%s: sec debug buffer not provided. Using kmalloc..\n",
				__func__);
		size = sizeof(struct sec_debug_log);
		vaddr = kzalloc(size, GFP_KERNEL);
	} else {
		size = secdbg_size;
		vaddr = ioremap_nocache(secdbg_paddr, secdbg_size);
	}

	pr_info("%s: vaddr=0x%p paddr=0x%llx size=0x%zx sizeof(struct sec_debug_log)=0x%zx\n",
			__func__, vaddr, (uint64_t)secdbg_paddr,
			secdbg_size, sizeof(struct sec_debug_log));

	if ((!vaddr) || (sizeof(struct sec_debug_log) > size)) {
		pr_err("%s: ERROR! init failed!\n", __func__);
		return -EFAULT;
	}
#if 0 /* enable this if required */
	memset(vaddr->sched, 0x0, sizeof(vaddr->sched));
	memset(vaddr->irq, 0x0, sizeof(vaddr->irq));
	memset(vaddr->irq_exit, 0x0, sizeof(vaddr->irq_exit));
	memset(vaddr->timer_log, 0x0, sizeof(vaddr->timer_log));
	memset(vaddr->secure, 0x0, sizeof(vaddr->secure));
#ifdef CONFIG_SEC_DEBUG_MSGLOG
	memset(vaddr->secmsg, 0x0, sizeof(vaddr->secmsg));
#endif
#ifdef CONFIG_SEC_DEBUG_AVC_LOG
	memset(vaddr->secavc, 0x0, sizeof(vaddr->secavc));
#endif
#endif
	for (i = 0; i < CONFIG_NR_CPUS; i++) {
		atomic_set(&(vaddr->idx_sched[i]), -1);
		atomic_set(&(vaddr->idx_irq[i]), -1);
		atomic_set(&(vaddr->idx_secure[i]), -1);
		atomic_set(&(vaddr->idx_irq_exit[i]), -1);
		atomic_set(&(vaddr->idx_timer[i]), -1);

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
		atomic_set(&(vaddr->idx_secmsg[i]), -1);
#endif
#ifdef CONFIG_SEC_DEBUG_AVC_LOG
		atomic_set(&(vaddr->idx_secavc[i]), -1);
#endif
	}
#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
	for (i = 0; i < CONFIG_NR_CPUS; i++)
		atomic_set(&(vaddr->dcvs_log_idx[i]), -1);
#endif

#ifdef CONFIG_SEC_DEBUG_POWER_LOG
	for (i = 0; i < CONFIG_NR_CPUS; i++)
		atomic_set(&(vaddr->idx_power[i]), -1);
#endif

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
	atomic_set(&(vaddr->fg_log_idx), -1);
#endif

	secdbg_log = vaddr;

	pr_info("%s: init done\n", __func__);

	return 0;
}
#else /* CONFIG_SEC_DEBUG_SCHED_LOG */
#define __init_sec_debug_log()
#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

#ifdef CONFIG_OF
static int __init __sec_debug_dt_addr_init(void)
{
	struct device_node *np;

	/* Using bottom of sec_dbg DDR address range
	 * for writing restart reason */
	np = of_find_compatible_node(NULL, NULL,
			"qcom,msm-imem-restart_reason");
	if (unlikely(!np)) {
		pr_err("unable to find DT imem restart reason node\n");
		return -ENODEV;
	}

	qcom_restart_reason = of_iomap(np, 0);
	if (unlikely(!qcom_restart_reason)) {
		pr_err("unable to map imem restart reason offset\n");
		return -ENODEV;
	}

	/* check restart_reason address here */
	pr_emerg("%s: restart_reason addr : 0x%p(0x%llx)\n", __func__,
			qcom_restart_reason,
			(uint64_t)virt_to_phys(qcom_restart_reason));

	/* Using bottom of sec_dbg DDR address range for writing upload_cause */
	np = of_find_compatible_node(NULL, NULL, "qcom,msm-imem-upload_cause");
	if (unlikely(!np)) {
		pr_err("unable to find DT imem upload cause node\n");
		return -ENODEV;
	}

	upload_cause = of_iomap(np, 0);
	if (unlikely(!upload_cause)) {
		pr_err("unable to map imem restart reason offset\n");
		return -ENODEV;
	}

	/* check upload_cause here */
	pr_emerg("%s: upload_cause addr : 0x%p(0x%llx)\n", __func__,
			upload_cause,
			(uint64_t)virt_to_phys(upload_cause));

	return 0;
}
#else /* CONFIG_OF */
static int __init __sec_debug_dt_addr_init(void) { return 0; }
#endif /* CONFIG_OF */

#define SCM_WDOG_DEBUG_BOOT_PART 0x9
void sec_do_bypass_sdi_execution_in_low(void)
{
	int ret;
	struct scm_desc desc = {
		.args[0] = 1,
		.args[1] = 0,
		.arginfo = SCM_ARGS(2),
	};

	/* Needed to bypass debug image on some chips */
	if (!is_scm_armv8())
		ret = scm_call_atomic2(SCM_SVC_BOOT,
			       SCM_WDOG_DEBUG_BOOT_PART, 1, 0);
	else
		ret = scm_call2_atomic(SCM_SIP_FNID(SCM_SVC_BOOT,
			  SCM_WDOG_DEBUG_BOOT_PART), &desc);
	if (ret)
		pr_err("Failed to disable wdog debug: %d\n", ret);

}

static char ap_serial_from_cmdline[20];

static int __init ap_serial_setup(char *str)
{
        snprintf(ap_serial_from_cmdline, sizeof(ap_serial_from_cmdline), "%s", &str[2]);
        return 1;
}
__setup("androidboot.ap_serial=", ap_serial_setup);

extern struct kset *devices_kset;

struct bus_type chip_id_subsys = {
        .name = "chip-id",
        .dev_name = "chip-id",
};

static ssize_t ap_serial_show(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf)
{
        pr_info("%s: ap_serial:[%s]\n", __func__, ap_serial_from_cmdline);
        return snprintf(buf, sizeof(ap_serial_from_cmdline), "%s\n", ap_serial_from_cmdline);
}

static struct kobj_attribute sysfs_SVC_AP_attr =
__ATTR(SVC_AP, 0444, ap_serial_show, NULL);

static struct kobj_attribute chipid_unique_id_attr =
__ATTR(unique_id, 0444, ap_serial_show, NULL);

static struct attribute *chip_id_attrs[] = {
        &chipid_unique_id_attr.attr,
        NULL,
};

static struct attribute_group chip_id_attr_group = {
        .attrs = chip_id_attrs,
};

static const struct attribute_group *chip_id_attr_groups[] = {
        &chip_id_attr_group,
        NULL,
};

static void __init create_ap_serial_node(void)
{
        int ret;
        struct kernfs_node *svc_sd;
        struct kobject *svc;
        struct kobject *AP;

        /* create /sys/devices/system/chip-id/unique_id */
        if (subsys_system_register(&chip_id_subsys, chip_id_attr_groups))
                pr_err("%s:Failed to register subsystem-%s\n", __func__, chip_id_subsys.name);

        /* To find /sys/devices/svc/ */
        svc_sd = sysfs_get_dirent(devices_kset->kobj.sd, "svc");
        if (IS_ERR_OR_NULL(svc_sd)) {
                svc = kobject_create_and_add("svc", &devices_kset->kobj);
                if (IS_ERR_OR_NULL(svc)) {
                        pr_err("%s:Failed to create sys/devices/svc\n", __func__);
                        goto error_create_svc;
                }
        } else {
                svc = (struct kobject *)svc_sd->priv;
        }
        /* create /sys/devices/SVC/AP */
        AP = kobject_create_and_add("AP", svc);
        if (IS_ERR_OR_NULL(AP)) {
                pr_err("%s:Failed to create sys/devices/svc/AP\n", __func__);
                goto error_create_AP;
        } else {
                /* create /sys/devices/svc/AP/SVC_AP */
                ret = sysfs_create_file(AP, &sysfs_SVC_AP_attr.attr);
                if (ret < 0) {
                        pr_err("%s:sysfs create fail-%s\n", __func__, sysfs_SVC_AP_attr.attr.name);
			goto error_create_sysfs;
                }
        }

	pr_info("%s: Completed\n",__func__);
	return;

error_create_sysfs:
	if(AP){
		kobject_put(AP);
		AP = NULL;
	}
error_create_AP:
	if(svc){
		kobject_put(svc);
		svc = NULL;
	}
error_create_svc:
        return;
}

int __init sec_debug_init(void)
{
	int ret;

// [ SEC_SELINUX_PORTING_QUALCOMM
#ifdef CONFIG_PROC_AVC
	sec_avc_log_init();
#endif
// ] SEC_SELINUX_PORTING_QUALCOMM

	ret = __sec_debug_dt_addr_init();
	if (unlikely(ret < 0))
		return ret;

	register_reboot_notifier(&nb_reboot_block);
	atomic_notifier_chain_register(&panic_notifier_list, &nb_panic_block);

	__init_sec_debug_log();

	sec_debug_set_upload_magic(RESTART_REASON_SEC_DEBUG_MODE);
	sec_debug_set_upload_cause(UPLOAD_CAUSE_INIT);

        create_ap_serial_node();
	if (unlikely(!sec_debug_is_enabled())) {
		sec_do_bypass_sdi_execution_in_low();
		return -EPERM;
	}
#ifdef CONFIG_SEC_DEBUG_CANCERIZER
	atomic_set(&cmctx.nr_test, 0);
	atomic_set(&cmctx.nr_running, 0);
	init_waitqueue_head(&cmctx.mwq);
	init_waitqueue_head(&cmctx.dwq);
	init_completion(&cmctx.done);
	cmctx.self = kthread_create(cancerizer_monitor, NULL, "cancerizerm");
	if (IS_ERR(cmctx.self)) {
		ret = PTR_ERR(cmctx.self);
		cmctx.self = NULL;
		return ret;
	}
#endif

	return 0;
}

int sec_debug_is_enabled(void)
{
	return enable;
}

#ifdef CONFIG_SEC_SSR_DEBUG_LEVEL_CHK
int sec_debug_is_enabled_for_ssr(void)
{
	return enable_cp_debug;
}
#endif

#ifdef CONFIG_SEC_FILE_LEAK_DEBUG
int sec_debug_print_file_list(void)
{
	size_t i = 0;
	unsigned int nCnt = 0;
	struct file *file = NULL;
	struct files_struct *files = current->files;
	const char *pRootName = NULL;
	const char *pFileName = NULL;
	int ret = 0;

	nCnt = files->fdt->max_fds;

	pr_err(" [Opened file list of process %s(PID:%d, TGID:%d) :: %d]\n",
		current->group_leader->comm, current->pid, current->tgid, nCnt);

	for (i = 0; i < nCnt; i++) {
		rcu_read_lock();
		file = fcheck_files(files, i);

		pRootName = NULL;
		pFileName = NULL;

		if (file) {
			if (file->f_path.mnt &&
			    file->f_path.mnt->mnt_root &&
			    file->f_path.mnt->mnt_root->d_name.name)
				pRootName =
					file->f_path.mnt->mnt_root->d_name.name;

			if (file->f_path.dentry &&
			    file->f_path.dentry->d_name.name)
				pFileName = file->f_path.dentry->d_name.name;

			pr_err("[%04zd]%s%s\n", i,
					pRootName ? pRootName : "null",
					pFileName ? pFileName : "null");
			ret++;
		}
		rcu_read_unlock();
	}
	if (ret > nCnt - 50)
		return 1;
	else
		return 0;
}

void sec_debug_EMFILE_error_proc(void)
{
	pr_err("Too many open files(%d:%s)\n",
		current->tgid, current->group_leader->comm);

	if (!enable)
		return;

	/* We check EMFILE error in only "system_server",
	 * "mediaserver" and "surfaceflinger" process.
	 */
	if (!strcmp(current->group_leader->comm, "system_server") ||
	    !strcmp(current->group_leader->comm, "mediaserver") ||
	    !strcmp(current->group_leader->comm, "surfaceflinger")) {
		if (sec_debug_print_file_list() == 1)
			panic("Too many open files");
	}
}
#endif /* CONFIG_SEC_FILE_LEAK_DEBUG */

/* klaatu - schedule log */
#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
static inline long get_switch_state(struct task_struct *p)
{
	long state = p->state;

#ifdef CONFIG_PREEMPT
	/*
	 * For all intents and purposes a preempted task is a running task.
	 */
	if (preempt_count() & PREEMPT_ACTIVE)
		state = TASK_RUNNING | TASK_STATE_MAX;
#endif

	return state;
}

static __always_inline void __sec_debug_task_sched_log(int cpu,
		struct task_struct *task, struct task_struct *prev,
		char *msg)
{
	int i;

	if (unlikely(!secdbg_log))
		return;

	if (unlikely(!task && !msg))
		return;

	i = atomic_inc_return(&(secdbg_log->idx_sched[cpu]))
		& (SCHED_LOG_MAX - 1);
	secdbg_log->sched[cpu][i].time = cpu_clock(cpu);
	if (task) {
		strlcpy(secdbg_log->sched[cpu][i].comm, task->comm,
			sizeof(secdbg_log->sched[cpu][i].comm));
		secdbg_log->sched[cpu][i].pid = task->pid;
		secdbg_log->sched[cpu][i].pTask = task;
		secdbg_log->sched[cpu][i].prio = task->prio;
		strlcpy(secdbg_log->sched[cpu][i].prev_comm, prev->comm,
			sizeof(secdbg_log->sched[cpu][i].prev_comm));

		secdbg_log->sched[cpu][i].prev_pid = prev->pid;
		secdbg_log->sched[cpu][i].prev_state = get_switch_state(prev);
		secdbg_log->sched[cpu][i].prev_prio = prev->prio;

	} else {
		strlcpy(secdbg_log->sched[cpu][i].comm, msg,
			sizeof(secdbg_log->sched[cpu][i].comm));
		secdbg_log->sched[cpu][i].pid = current->pid;
		secdbg_log->sched[cpu][i].pTask = NULL;
	}
}

void sec_debug_irq_enterexit_log(unsigned int irq, u64 start_time)
{
	int cpu = smp_processor_id();
	int i;

	if (unlikely(!secdbg_log))
		return;

	i = atomic_inc_return(&(secdbg_log->idx_irq_exit[cpu]))
		& (SCHED_LOG_MAX - 1);
	secdbg_log->irq_exit[cpu][i].time = start_time;
	secdbg_log->irq_exit[cpu][i].end_time = cpu_clock(cpu);
	secdbg_log->irq_exit[cpu][i].irq = irq;
	secdbg_log->irq_exit[cpu][i].elapsed_time =
		secdbg_log->irq_exit[cpu][i].end_time - start_time;
	secdbg_log->irq_exit[cpu][i].pid = current->pid;
}

void sec_debug_task_sched_log_short_msg(char *msg)
{
	__sec_debug_task_sched_log(raw_smp_processor_id(), NULL, NULL, msg);
}

void sec_debug_task_sched_log(int cpu,
		struct task_struct *task, struct task_struct *prev)
{
	__sec_debug_task_sched_log(cpu, task, prev, NULL);
}

void sec_debug_timer_log(unsigned int type, int int_lock, void *fn)
{
	int cpu = smp_processor_id();
	int i;

	if (unlikely(!secdbg_log))
		return;

	i = atomic_inc_return(&(secdbg_log->idx_timer[cpu]))
		& (SCHED_LOG_MAX - 1);
	secdbg_log->timer_log[cpu][i].time = cpu_clock(cpu);
	secdbg_log->timer_log[cpu][i].type = type;
	secdbg_log->timer_log[cpu][i].int_lock = int_lock;
	secdbg_log->timer_log[cpu][i].fn = (void *)fn;
	secdbg_log->timer_log[cpu][i].pid = current->pid;
}

void sec_debug_secure_log(u32 svc_id, u32 cmd_id)
{
	static DEFINE_SPINLOCK(secdbg_securelock);
	unsigned long flags;
	int cpu, i;

	if (unlikely(!secdbg_log))
		return;

	spin_lock_irqsave(&secdbg_securelock, flags);

	cpu = smp_processor_id();
	i = atomic_inc_return(&(secdbg_log->idx_secure[cpu]))
		& (TZ_LOG_MAX - 1);
	secdbg_log->secure[cpu][i].time = cpu_clock(cpu);
	secdbg_log->secure[cpu][i].svc_id = svc_id;
	secdbg_log->secure[cpu][i].cmd_id = cmd_id;
	secdbg_log->secure[cpu][i].pid = current->pid;

	spin_unlock_irqrestore(&secdbg_securelock, flags);
}

void sec_debug_irq_sched_log(unsigned int irq, void *fn, char *name, unsigned int en)
{
	int cpu = smp_processor_id();
	int i;

	if (unlikely(!secdbg_log))
		return;

	i = atomic_inc_return(&(secdbg_log->idx_irq[cpu]))
		& (SCHED_LOG_MAX - 1);
	secdbg_log->irq[cpu][i].time = cpu_clock(cpu);
	secdbg_log->irq[cpu][i].irq = irq;
	secdbg_log->irq[cpu][i].fn = (void *)fn;
	secdbg_log->irq[cpu][i].name = name;
	secdbg_log->irq[cpu][i].en = irqs_disabled();
	secdbg_log->irq[cpu][i].preempt_count = preempt_count();
	secdbg_log->irq[cpu][i].context = &cpu;
	secdbg_log->irq[cpu][i].pid = current->pid;
	secdbg_log->irq[cpu][i].entry_exit = en;
}

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
asmlinkage int sec_debug_msg_log(void *caller, const char *fmt, ...)
{
	int cpu = smp_processor_id();
	int r;
	int i;
	va_list args;

	if (unlikely(!secdbg_log))
		return 0;

	i = atomic_inc_return(&(secdbg_log->idx_secmsg[cpu]))
		& (MSG_LOG_MAX - 1);
	secdbg_log->secmsg[cpu][i].time = cpu_clock(cpu);
	va_start(args, fmt);
	r = vsnprintf(secdbg_log->secmsg[cpu][i].msg,
		sizeof(secdbg_log->secmsg[cpu][i].msg), fmt, args);
	va_end(args);

	secdbg_log->secmsg[cpu][i].caller0 = __builtin_return_address(0);
	secdbg_log->secmsg[cpu][i].caller1 = caller;
	secdbg_log->secmsg[cpu][i].task = current->comm;

	return r;
}
#endif /* CONFIG_SEC_DEBUG_MSG_LOG */

#ifdef CONFIG_SEC_DEBUG_AVC_LOG
asmlinkage int sec_debug_avc_log(const char *fmt, ...)
{
	int cpu = smp_processor_id();
	int r;
	int i;
	va_list args;

	if (unlikely(!secdbg_log))
		return 0;

	i = atomic_inc_return(&(secdbg_log->idx_secavc[cpu]))
		& (AVC_LOG_MAX - 1);
	va_start(args, fmt);
	r = vsnprintf(secdbg_log->secavc[cpu][i].msg,
		sizeof(secdbg_log->secavc[cpu][i].msg), fmt, args);
	va_end(args);

	return r;
}
#endif /* CONFIG_SEC_DEBUG_AVC_LOG */

#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

static int __init sec_dbg_setup(char *str)
{
	size_t size = (size_t)memparse(str, &str);

	pr_emerg("%s: str=%s\n", __func__, str);

	if (size /*&& (size == roundup_pow_of_two(size))*/ && (*str == '@')) {
		secdbg_paddr = (phys_addr_t)memparse(++str, NULL);
		secdbg_size = size;
	}

	pr_emerg("%s: secdbg_paddr = 0x%llx\n", __func__,
			(uint64_t)secdbg_paddr);
	pr_emerg("%s: secdbg_size = 0x%zx\n", __func__, secdbg_size);

	return 1;
}
__setup("sec_dbg=", sec_dbg_setup);

static void sec_user_fault_dump(void)
{
	if (enable == 1 && enable_user == 1)
		panic("User Fault");
}

static ssize_t sec_user_fault_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *offs)
{
	char buf[100];

	if (count > sizeof(buf) - 1)
		return -EINVAL;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	buf[count] = '\0';
	if (!strcmp(buf, "dump_user_fault"))
		sec_user_fault_dump();

	return count;
}

static const struct file_operations sec_user_fault_proc_fops = {
	.write = sec_user_fault_write,
};

static int __init sec_debug_user_fault_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("user_fault", S_IWUSR | S_IWGRP, NULL,
			&sec_user_fault_proc_fops);
	if (!entry)
		return -ENOMEM;

	return 0;
}
device_initcall(sec_debug_user_fault_init);

#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
void sec_debug_dcvs_log(int cpu_no, unsigned int prev_freq,
						unsigned int new_freq)
{
	int i;

	if (!secdbg_log)
		return;

	i = atomic_inc_return(&(secdbg_log->dcvs_log_idx[cpu_no]))
		& (DCVS_LOG_MAX - 1);
	secdbg_log->dcvs_log[cpu_no][i].cpu_no = cpu_no;
	secdbg_log->dcvs_log[cpu_no][i].prev_freq = prev_freq;
	secdbg_log->dcvs_log[cpu_no][i].new_freq = new_freq;
	secdbg_log->dcvs_log[cpu_no][i].time = cpu_clock(cpu_no);
}
#endif


#ifdef CONFIG_SEC_DEBUG_POWER_LOG
void sec_debug_cpu_lpm_log(int cpu, unsigned int index, bool success, int entry_exit)
{
	int i;

	if (!secdbg_log)
		return;

	i = atomic_inc_return(&(secdbg_log->idx_power[cpu]))
		& (POWER_LOG_MAX - 1);
	secdbg_log->pwr_log[cpu][i].time = cpu_clock(cpu);
	secdbg_log->pwr_log[cpu][i].pid = current->pid;
	secdbg_log->pwr_log[cpu][i].type = CPU_POWER_TYPE;

	secdbg_log->pwr_log[cpu][i].cpu.index = index;
	secdbg_log->pwr_log[cpu][i].cpu.success = success;
	secdbg_log->pwr_log[cpu][i].cpu.entry_exit = entry_exit;
}

void sec_debug_cluster_lpm_log(const char *name, int index, unsigned long sync_cpus,
			unsigned long child_cpus, bool from_idle, int entry_exit)
{
	int i;
	int cpu = smp_processor_id();

	if (!secdbg_log)
		return;

	i = atomic_inc_return(&(secdbg_log->idx_power[cpu]))
		& (POWER_LOG_MAX - 1);

	secdbg_log->pwr_log[cpu][i].time = cpu_clock(cpu);
	secdbg_log->pwr_log[cpu][i].pid = current->pid;
	secdbg_log->pwr_log[cpu][i].type = CLUSTER_POWER_TYPE;

	secdbg_log->pwr_log[cpu][i].cluster.name = (char *) name;
	secdbg_log->pwr_log[cpu][i].cluster.index = index;
	secdbg_log->pwr_log[cpu][i].cluster.sync_cpus = sync_cpus;
	secdbg_log->pwr_log[cpu][i].cluster.child_cpus = child_cpus;
	secdbg_log->pwr_log[cpu][i].cluster.from_idle = from_idle;
	secdbg_log->pwr_log[cpu][i].cluster.entry_exit = entry_exit;
}
void sec_debug_clock_log(const char *name, unsigned int state, unsigned int cpu_id, int complete)
{
	int i;
	int cpu = cpu_id;
	if (!secdbg_log)
		return;

	i = atomic_inc_return(&(secdbg_log->idx_power[cpu_id]))
		& (POWER_LOG_MAX - 1);
	secdbg_log->pwr_log[cpu][i].time = cpu_clock(cpu_id);
	secdbg_log->pwr_log[cpu][i].pid = current->pid;
	secdbg_log->pwr_log[cpu][i].type = CLOCK_RATE_TYPE;

	secdbg_log->pwr_log[cpu][i].clk_rate.name = (char *)name;
	secdbg_log->pwr_log[cpu][i].clk_rate.state = state;
	secdbg_log->pwr_log[cpu][i].clk_rate.cpu_id = cpu_id;
	secdbg_log->pwr_log[cpu][i].clk_rate.complete = complete;
}
#endif

#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
void sec_debug_fuelgauge_log(unsigned int voltage, unsigned short soc,
				unsigned short charging_status)
{
	int cpu = smp_processor_id();
	int i;

	if (!secdbg_log)
		return;

	i = atomic_inc_return(&(secdbg_log->fg_log_idx))
		& (FG_LOG_MAX - 1);
	secdbg_log->fg_log[i].time = cpu_clock(cpu);
	secdbg_log->fg_log[i].voltage = voltage;
	secdbg_log->fg_log[i].soc = soc;
	secdbg_log->fg_log[i].charging_status = charging_status;
}
#endif
