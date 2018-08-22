/*
 * Copyright (c) 2014-2017 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/kmsg_dump.h>
#include <linux/kallsyms.h>
#include <linux/kernel_stat.h>
#include <linux/irq.h>
#include <linux/tick.h>
#include <linux/file.h>
#include <linux/sec_sysfs.h>
#include <linux/sec_ext.h>
#include <linux/sec_debug.h>
#include <linux/sec_debug_hard_reset_hook.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/mount.h>
#include <linux/of_reserved_mem.h>

#include <asm/cacheflush.h>
#include <asm/stacktrace.h>

#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-powermode.h>
#include <linux/soc/samsung/exynos-soc.h>

#if defined(CONFIG_SEC_DUMP_SUMMARY)
#include <linux/cpufreq.h>

struct sec_debug_summary *summary_info;
static char *sec_summary_log_buf;
static unsigned long sec_summary_log_size;
static unsigned long reserved_out_buf;
static unsigned long reserved_out_size;
static char *last_summary_buffer;
static size_t last_summary_size;

#ifdef CONFIG_ARM64
#define ARM_PT_REG_PC pc
#define ARM_PT_REG_LR regs[30]
#else
#define ARM_PT_REG_PC ARM_pc
#define ARM_PT_REG_LR ARM_lr
#endif

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

static int sec_debug_reserve_ok;

int sec_debug_get_debug_level(void)
{
	return sec_debug_level.uint_val;
}

static void sec_debug_user_fault_dump(void)
{
	if (sec_debug_level.en.kernel_fault == 1 &&
	    sec_debug_level.en.user_fault == 1)
		panic("User Fault");
}

static ssize_t sec_debug_user_fault_write(struct file *file, const char __user *buffer, size_t count, loff_t *offs)
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

/* layout of SDRAM : First 4KB of DRAM
*         0x0: magic            (4B)
*   0x4~0x3FF: panic string     (1020B)
* 0x400~0x7FF: panic Extra Info (1KB)
* 0x800~0xFFB: panic dumper log (2KB - 4B)
*       0xFFC: copy of magic    (4B)
*/

enum sec_debug_upload_magic_t {
	UPLOAD_MAGIC_INIT		= 0x0,
	UPLOAD_MAGIC_PANIC		= 0x66262564,
};

enum sec_debug_upload_cause_t {
	UPLOAD_CAUSE_INIT		= 0xCAFEBABE,
	UPLOAD_CAUSE_KERNEL_PANIC	= 0x000000C8,
	UPLOAD_CAUSE_FORCED_UPLOAD	= 0x00000022,
#ifdef CONFIG_SEC_UPLOAD
	UPLOAD_CAUSE_USER_FORCED_UPLOAD	= 0x00000074,
#endif
	UPLOAD_CAUSE_CP_ERROR_FATAL	= 0x000000CC,
	UPLOAD_CAUSE_USER_FAULT		= 0x0000002F,
	UPLOAD_CAUSE_HSIC_DISCONNECTED	= 0x000000DD,
	UPLOAD_CAUSE_POWERKEY_LONG_PRESS = 0x00000085,
	UPLOAD_CAUSE_HARD_RESET	= 0x00000066,
};

static void sec_debug_set_upload_magic(unsigned magic, char *str)
{
	preempt_disable();

	*(unsigned int *)SEC_DEBUG_MAGIC_VA = magic;
	*(unsigned int *)(SEC_DEBUG_MAGIC_VA + SZ_4K - 4) = magic;

	if (str) {
		strncpy((char *)SEC_DEBUG_MAGIC_VA + 4, str, SZ_1K - 4);

#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
		sec_debug_set_extra_info_panic(str);
		sec_debug_finish_extra_info();
#endif
	}

	flush_cache_all();
	preempt_enable();

	pr_emerg("sec_debug: set magic code (0x%x)\n", magic);
}

static void sec_debug_set_upload_cause(enum sec_debug_upload_cause_t type)
{
	exynos_pmu_write(EXYNOS_PMU_INFORM3, type);

	pr_emerg("sec_debug: set upload cause (0x%x)\n", type);
}

static void sec_debug_kmsg_dump(struct kmsg_dumper *dumper, enum kmsg_dump_reason reason)
{
	kmsg_dump_get_buffer(dumper, true, (char *)SEC_DEBUG_DUMPER_LOG_VA, SZ_2K - 4, NULL);
}

static struct kmsg_dumper sec_dumper = {
	.dump = sec_debug_kmsg_dump,
};

static int sec_debug_reserved(phys_addr_t base, phys_addr_t size)
{
#ifdef CONFIG_NO_BOOTMEM
	return (memblock_is_region_reserved(base, size) ||
		memblock_reserve(base, size));
#else
	return reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE);
#endif
}

static int __init sec_debug_magic_setup(struct reserved_mem *rmem)
{
	pr_info("%s: Reserved Mem(0x%llx, 0x%llx) - Success\n",
		__func__, rmem->base, rmem->size);
	sec_debug_reserve_ok = 1;

	return 0;
}
RESERVEDMEM_OF_DECLARE(sec_debug_magic, "exynos,sec_debug_magic", sec_debug_magic_setup);

#define MAX_RECOVERY_CAUSE_SIZE 256
char recovery_cause[MAX_RECOVERY_CAUSE_SIZE];
unsigned long recovery_cause_offset;

static ssize_t show_recovery_cause(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!recovery_cause_offset)
		return 0;

	sec_get_param_str(recovery_cause_offset, buf);
	pr_info("%s: %s\n", __func__, buf);

	return strlen(buf);
}

static ssize_t store_recovery_cause(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if (!recovery_cause_offset)
		return 0;

	if (strlen(buf) > sizeof(recovery_cause))
		pr_err("%s: input buffer length is out of range.\n", __func__);

	snprintf(recovery_cause, sizeof(recovery_cause), "%s:%d ", current->comm, task_pid_nr(current));
	if (strlen(recovery_cause) + strlen(buf) >= sizeof(recovery_cause)) {
		pr_err("%s: input buffer length is out of range.\n", __func__);
		return count;
	}
	strncat(recovery_cause, buf, strlen(buf));

	sec_set_param_str(recovery_cause_offset, recovery_cause, sizeof(recovery_cause));
	pr_info("%s: %s, count:%d\n", __func__, recovery_cause, (int)count);

	return count;
}

static DEVICE_ATTR(recovery_cause, 0660, show_recovery_cause, store_recovery_cause);

void sec_debug_recovery_reboot(void)
{
	char *buf;

	if (recovery_cause_offset) {
		if (!recovery_cause[0] || !strlen(recovery_cause)) {
			buf = "empty caller";
			store_recovery_cause(NULL, NULL, buf, strlen(buf));
		}
	}
}

static int __init sec_debug_recovery_cause_setup(char *str)
{
	recovery_cause_offset = memparse(str, &str);

	/* If we encounter any problem parsing str ... */
	if (!recovery_cause_offset) {
		pr_err("%s: failed to parse address.\n", __func__);
		goto out;
	}

	pr_info("%s, recovery_cause_offset :%lx\n", __func__, recovery_cause_offset);
out:
	return 0;
}
__setup("androidboot.recovery_offset=", sec_debug_recovery_cause_setup);

static int __init sec_debug_recovery_cause_init(void)
{
	struct device *dev;

	memset(recovery_cause, 0, MAX_RECOVERY_CAUSE_SIZE);

	dev = sec_device_create(NULL, "sec_debug");
	WARN_ON(!dev);
	if (IS_ERR(dev))
		pr_err("%s:Failed to create devce\n", __func__);

	if (device_create_file(dev, &dev_attr_recovery_cause) < 0)
		pr_err("%s: Failed to create device file\n", __func__);

	return 0;
}
late_initcall(sec_debug_recovery_cause_init);

static int __init sec_debug_init(void)
{
	if (!sec_debug_reserve_ok)
		pr_crit("fatal: %s: memory has not been reserved\n", __func__);

	/* clear traps info */
	memset((void *)SEC_DEBUG_MAGIC_VA + 4, 0, SZ_1K - 4);

	sec_debug_set_upload_magic(UPLOAD_MAGIC_PANIC, NULL);

	kmsg_dump_register(&sec_dumper);

	return 0;
}
early_initcall(sec_debug_init);

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
	u64 user = 0;
	u64 nice = 0;
	u64 system = 0;
	u64 idle = 0;
	u64 iowait = 0;
	u64 irq = 0;
	u64 softirq = 0;
	u64 steal = 0;
	u64 guest = 0;
	u64 guest_nice = 0;
	u64 sum = 0;
	u64 sum_softirq = 0;
	unsigned int per_softirq_sums[NR_SOFTIRQS] = {0};

	char *softirq_to_name[NR_SOFTIRQS] = { "HI", "TIMER", "NET_TX", "NET_RX", "BLOCK", "BLOCK_IOPOLL", "TASKLET", "SCHED", "HRTIMER", "RCU" };

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
			pr_info("irq-%-4d : %8u %s\n", j, irq_stat,
				irq_to_desc(j)->action ? irq_to_desc(j)->action->name ? : "???" : "???");
		}
	}
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	pr_info("\n");
	pr_info("softirq : %llu", (unsigned long long)sum_softirq);
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < NR_SOFTIRQS; i++)
		if (per_softirq_sums[i])
			pr_info("softirq-%d : %8u %s\n", i, per_softirq_sums[i], softirq_to_name[i]);
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
}

void sec_debug_reboot_handler(void *p)
{
	/* Clear magic code in normal reboot */
	sec_debug_set_upload_magic(UPLOAD_MAGIC_INIT, NULL);

	if (p != NULL)
		if (!strcmp(p, "recovery"))
			sec_debug_recovery_reboot();
}

void sec_debug_panic_handler(void *buf, bool dump)
{
	/* Set upload cause */
	sec_debug_set_upload_magic(UPLOAD_MAGIC_PANIC, buf);
	if (!strncmp(buf, "User Fault", 10))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_USER_FAULT);
	else if (is_hard_reset_occurred())
		sec_debug_set_upload_cause(UPLOAD_CAUSE_HARD_RESET);
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
		sec_debug_dump_cpu_stat();
		debug_show_all_locks();
	}
}

void sec_debug_post_panic_handler(void)
{
	hard_reset_delay();

	/* reset */
	pr_emerg("sec_debug: %s\n", linux_banner);
	pr_emerg("sec_debug: rebooting...\n");

	flush_cache_all();
}

#ifdef CONFIG_SEC_DEBUG_FILE_LEAK
void sec_debug_print_file_list(void)
{
	int i = 0;
	unsigned int count = 0;
	struct file *file = NULL;
	struct files_struct *files = current->files;
	const char *p_rootname = NULL;
	const char *p_filename = NULL;

	count = files->fdt->max_fds;

	pr_err("[Opened file list of process %s(PID:%d, TGID:%d) :: %d]\n",
	       current->group_leader->comm, current->pid, current->tgid, count);

	for (i = 0; i < count; i++) {
		rcu_read_lock();
		file = fcheck_files(files, i);

		p_rootname = NULL;
		p_filename = NULL;

		if (file) {
			if (file->f_path.mnt && file->f_path.mnt->mnt_root &&
			    file->f_path.mnt->mnt_root->d_name.name)
				p_rootname = file->f_path.mnt->mnt_root->d_name.name;

			if (file->f_path.dentry && file->f_path.dentry->d_name.name)
				p_filename = file->f_path.dentry->d_name.name;

			pr_err("[%04d]%s%s\n", i, p_rootname ? p_rootname : "null",
			       p_filename ? p_filename : "null");
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
	       current->tgid, current->group_leader->comm, __builtin_return_address(0));

	if (!sec_debug_level.en.kernel_fault)
		return;

	/* We check EMFILE error in only "system_server","mediaserver" and "surfaceflinger" process.*/
	if (!strcmp(current->group_leader->comm, "system_server") ||
	    !strcmp(current->group_leader->comm, "mediaserver") ||
	    !strcmp(current->group_leader->comm, "surfaceflinger")) {
		sec_debug_print_file_list();
		panic("Too many open files");
	}
}
#endif /* CONFIG_SEC_DEBUG_FILE_LEAK */

/* leave the following definithion of module param call here for the compatibility with other models */
module_param_call(force_error, sec_debug_force_error, NULL, NULL, 0644);

static struct sec_debug_shared_info *sec_debug_info;

static void sec_debug_init_base_buffer(unsigned long base, unsigned long size)
{
	sec_debug_info = (struct sec_debug_shared_info *)phys_to_virt(base);

	if (sec_debug_info) {

		sec_debug_info->magic[0] = SEC_DEBUG_SHARED_MAGIC0;
		sec_debug_info->magic[1] = SEC_DEBUG_SHARED_MAGIC1;
		sec_debug_info->magic[2] = SEC_DEBUG_SHARED_MAGIC2;
		sec_debug_info->magic[3] = SEC_DEBUG_SHARED_MAGIC3;

		sec_debug_set_kallsyms_info(&(sec_debug_info->ksyms), SEC_DEBUG_SHARED_MAGIC1);

#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
		sec_debug_init_extra_info(sec_debug_info);
#endif

	}
	pr_info("%s, base(virt):0x%lx size:0x%lx\n", __func__, (unsigned long)sec_debug_info, size);

}

char *verbose_reg(int cpu_type, int reg_name, unsigned long reg_val)
{
	int len;
	unsigned long v;
	static char explain[512];
	char *p = explain;

	if (reg_val == 0) {
		explain[0] = 0;
		return explain;
	}

	memset(explain, 0, ARRAY_SIZE(explain));

	if (cpu_type == ARM_CPU_PART_ANANKE) {
		switch (reg_name) {
		case DISR_EL1:
			v = FIELD_VAL(reg_val, A);
			if (v == 0b1) {
				len = sprintf(p, "[Async SError INT]");
				p += len;
			}
			v = FIELD_VAL(reg_val, AET);
			if (v == 0b001) {
				len = sprintf(p, "[Unrecoverable Error]");
				p += len;
			}
			v = FIELD_VAL(reg_val, DFSC);
			if (v == 0b010001)
				sprintf(p, "[in AArch64 state or at EL2, or in AArch32 state at EL1, EAE=1 in TTBCR]");
			else if (v == 0b000110)
				sprintf(p, "[in AArch32 state at EL1, EAE=0 in TTBCR]");
			break;
		case ERR0STATUS:
		case ERR1STATUS:
			v = FIELD_VAL(reg_val, V);
			if (v == 0b1) {
				len = sprintf(p, "[VALID]");
				p += len;
			}
			v = FIELD_VAL(reg_val, UE);
			if (v == 0b1) {
				len = sprintf(p, "[Uncorrected Error]");
				p += len;
			}
			v = FIELD_VAL(reg_val, ER);
			if (v == 0b1) {
				len = sprintf(p, "[Error Reported]");
				p += len;
			}
			v = FIELD_VAL(reg_val, OF);
			if (v == 0b1) {
				len = sprintf(p, "[More than 1 ERR]");
				p += len;
			}
			v = FIELD_VAL(reg_val, MV);
			if (v == 0b1) {
				len = sprintf(p, "[More Info on ERRxMISC0]");
				p += len;
			}
			v = FIELD_VAL(reg_val, CE);
			if (v == 0b10) {
				len = sprintf(p, "[Corrected Error]");
				p += len;
			}
			v = FIELD_VAL(reg_val, DE);
			if (v == 0b1) {
				len = sprintf(p, "[Deferred Error]");
				p += len;
			}
			if (reg_name == ERR0STATUS) {
				v = FIELD_VAL(reg_val, IERR);
				if (v == 0x1) {
					len = sprintf(p, "[Error on L1 dirty RAM]");
					p += len;
				}
				v = FIELD_VAL(reg_val, SERR);
				if (v == 0x2)
					sprintf(p, "[ECC error from internal data buffer]");
				else if (v == 0x6)
					sprintf(p, "[ECC error on cache data RAM]");
				else if (v == 0x7)
					sprintf(p, "[ECC error on cache tag or dirty RAM]");
				else if (v == 0x8)
					sprintf(p, "[Parity error on TLB data RAM]");
				else if (v == 0x9)
					sprintf(p, "[Parity error on TLB tag RAM]");
				else if (v == 0x15)
					sprintf(p, "[Deferred error from slave not supported at the consumer]");
			} else if (reg_name == ERR1STATUS) {
				v = FIELD_VAL(reg_val, IERR);
				if (v == 0x2) {
					len = sprintf(p, "[Error on L3 snoop filter RAM]");
					p += len;
				}
				v = FIELD_VAL(reg_val, SERR);
				if (v == 0x2)
					sprintf(p, "[ECC error from internal data buffer]");
				else if (v == 0x6)
					sprintf(p, "[ECC error on cache data RAM]");
				else if (v == 0x7)
					sprintf(p, "[ECC error on cache tag or dirty RAM]");
				else if (v == 0x12)
					sprintf(p, "[Bus error]");
			}
			break;
		case ERR0MISC0:
		case ERR1MISC0:
			v = FIELD_VAL(reg_val, OFO);
			if (v == 0b1) {
				len = sprintf(p, "[OverFlow Other]");
				p += len;
			}
			v = FIELD_VAL(reg_val, CECO);
			if (v != 0) {
				len = sprintf(p, "[Corrected Error Count Other: %lu]", v);
				p += len;
			}
			v = FIELD_VAL(reg_val, OFR);
			if (v == 0b1) {
				len = sprintf(p, "[OverFlow Repeat]");
				p += len;
			}
			v = FIELD_VAL(reg_val, CECR);
			if (v != 0) {
				len = sprintf(p, "[Corrected Error Count Repeat: %lu]", v);
				p += len;
			}
			v = FIELD_VAL(reg_val, WAY);
			len = sprintf(p, "[WAY containing Error: %lu]", v);
			p += len;
			v = FIELD_VAL(reg_val, INDX);
			len = sprintf(p, "[INDEX containing Error: %lu]", v);
			p += len;
			if (reg_name == ERR0MISC0) {
				v = FIELD_VAL(reg_val, LVL);
				if (v == 0b000) {
					len = sprintf(p, "[Level 1]");
					p += len;
				} else if (v == 0b001) {
					len = sprintf(p, "[Level 2]");
					p += len;
				}
				v = FIELD_VAL(reg_val, IND);
				if (v == 0b0)
					sprintf(p, "[L1 data cache, unified L2 cache, or TLB]");
				else if (v == 0b1)
					sprintf(p, "[L1 instruction cache]");
			} else if (reg_name == ERR1MISC0) {
				v = FIELD_VAL(reg_val, LVL);
				if (v == 0b010) {
					len = sprintf(p, "[Level 3]");
					p += len;
				}
				v = FIELD_VAL(reg_val, IND);
				if (v == 0b0)
					sprintf(p, "[L3 cache]");
			}
			break;
		case ERR0ADDR:	// This register is unused in the Cortex-A55 core and marked as RES0
		case ERR1ADDR:	// There is no description in the TRM
		default:
			break;
		}
	} else if (cpu_type == ARM_CPU_PART_MEERKAT) {
		switch (reg_name) {
		case FEMERR1SR:
			v = FIELD_VAL(reg_val, ADDRINDEX_FE);
			sprintf(p, "[INDEX of the array: %lu]", v);
			break;
		case LSMERR1SR:
			v = FIELD_VAL(reg_val, ADDRINDEX_LS);
			sprintf(p, "[INDEX of the array: %lu]", v);
			break;
		case TBWMERR1SR:
			v = FIELD_VAL(reg_val, ADDRINDEX_TBW);
			sprintf(p, "[INDEX of the array: %lu]", v);
			break;
		case L2MERR1SR:
			v = FIELD_VAL(reg_val, ADDRINDEX_L2);
			sprintf(p, "[INDEX of the array: %lu]", v);
			break;
		case L3MERR1SR:
			v = FIELD_VAL(reg_val, ADDRINDEX_L3);
			sprintf(p, "[INDEX of the array: %lu]", v);
			break;
		case FEMERR0SR:
		case LSMERR0SR:
		case TBWMERR0SR:
		case L2MERR0SR:
		case L3MERR0SR:
			v = FIELD_VAL(reg_val, ADDRVALID);
			if (v == 0b1) {
				len = sprintf(p, "[ADDR is logged]");
				p += len;
			} else if (v == 0b0) {
				len = sprintf(p, "[INDEX is logged]");
				p += len;
			}
			v = FIELD_VAL(reg_val, UNCONTAINED);
			if (v == 0b1) {
				len = sprintf(p, "[Uncontained]");
				p += len;
			} else if (v == 0b0) {
				len = sprintf(p, "[Contained]");
				p += len;
			}
			v = FIELD_VAL(reg_val, UC);
			if (v == 0b1) {
				len = sprintf(p, "[Uncorrectable]");
				p += len;
			} else if (v == 0b0) {
				len = sprintf(p, "[Correctable]");
				p += len;
			}
			v = FIELD_VAL(reg_val, VALID);
			if (v == 0b1)
				sprintf(p, "[VALID]");
			break;
		default:
			break;
		}
	}

	return explain;
}

static int __init sec_debug_base_setup(char *str)
{
	unsigned long size = memparse(str, &str);
	unsigned long base = 0;

	/* If we encounter any problem parsing str ... */
	if (!size || *str != '@' || kstrtoul(str + 1, 0, &base)) {
		pr_err("%s: failed to parse address.\n", __func__);
		goto out;
	}

	if (sec_debug_reserved(base, size)) {
		/* size is not match with -size and size + sizeof(...) */
		pr_err("%s: failed to reserve size:0x%lx at base 0x%lx\n",
		       __func__, size, base);
		goto out;
	}

	sec_debug_init_base_buffer(base, size);

	pr_info("%s, base(phys):0x%lx size:0x%lx\n", __func__, base, size);
out:
	return 0;
}
__setup("sec_debug.base=", sec_debug_base_setup);

#endif /* CONFIG_SEC_DEBUG */

#if defined(CONFIG_SEC_DUMP_SUMMARY)

void __sec_debug_task_sched_log(int cpu, struct task_struct *task, char *msg)
{
	unsigned long i;

	if (!summary_info)
		return;

	if (!task && !msg)
		return;

	i = atomic_inc_return(&summary_info->sched_log.idx_sched[cpu]) & (SCHED_LOG_MAX - 1);
	summary_info->sched_log.sched[cpu][i].time = cpu_clock(cpu);
	if (task) {
		strlcpy(summary_info->sched_log.sched[cpu][i].comm, task->comm,
			sizeof(summary_info->sched_log.sched[cpu][i].comm));
		summary_info->sched_log.sched[cpu][i].pid = task->pid;
		summary_info->sched_log.sched[cpu][i].pTask = task;
	} else {
		strlcpy(summary_info->sched_log.sched[cpu][i].comm, msg,
			sizeof(summary_info->sched_log.sched[cpu][i].comm));
		summary_info->sched_log.sched[cpu][i].pid = -1;
		summary_info->sched_log.sched[cpu][i].pTask = NULL;
	}
}

void sec_debug_task_sched_log_short_msg(char *msg)
{
	__sec_debug_task_sched_log(raw_smp_processor_id(), NULL, msg);
}

void sec_debug_task_sched_log(int cpu, struct task_struct *task)
{
	__sec_debug_task_sched_log(cpu, task, NULL);
}

void sec_debug_irq_sched_log(unsigned int irq, void *fn, int en)
{
	int cpu = smp_processor_id();
	unsigned long i;

	if (!summary_info)
		return;

	i = atomic_inc_return(&summary_info->sched_log.idx_irq[cpu]) & (SCHED_LOG_MAX - 1);
	summary_info->sched_log.irq[cpu][i].time = cpu_clock(cpu);
	summary_info->sched_log.irq[cpu][i].irq = irq;
	summary_info->sched_log.irq[cpu][i].fn = (void *)fn;
	summary_info->sched_log.irq[cpu][i].en = en;
	summary_info->sched_log.irq[cpu][i].preempt_count = preempt_count();
	summary_info->sched_log.irq[cpu][i].context = &cpu;
}

void sec_debug_irq_enterexit_log(unsigned int irq, unsigned long long start_time)
{
	int cpu = smp_processor_id();
	unsigned long i;

	if (!summary_info)
		return;

	i = atomic_inc_return(&summary_info->sched_log.idx_irq_exit[cpu]) & (SCHED_LOG_MAX - 1);
	summary_info->sched_log.irq_exit[cpu][i].time = start_time;
	summary_info->sched_log.irq_exit[cpu][i].end_time = cpu_clock(cpu);
	summary_info->sched_log.irq_exit[cpu][i].irq = irq;
	summary_info->sched_log.irq_exit[cpu][i].elapsed_time =
		summary_info->sched_log.irq_exit[cpu][i].end_time - start_time;
}

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

#ifdef CONFIG_NO_BOOTMEM
	if (memblock_is_region_reserved(base, size) || memblock_reserve(base, size)) {
#else
	if (reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE)) {
#endif
		pr_err("%s: failed to reserve size:0x%lx at base 0x%lx\n", __func__, size, base);
		goto out;
	}

	pr_info("%s, base:0x%lx size:0x%lx\n", __func__, base, size);

	sec_summary_log_buf = phys_to_virt(base);
	sec_summary_log_size = round_up(sizeof(struct sec_debug_summary), PAGE_SIZE);
	last_summary_buffer = phys_to_virt(base + sec_summary_log_size);
	sec_debug_summary_set_reserved_out_buf(base + sec_summary_log_size, (size - sec_summary_log_size));
out:
	return 0;
}
__setup("sec_summary_log=", sec_summary_log_setup);

int sec_debug_summary_init(void)
{
	int offset = 0;
	int i;

	if (!sec_summary_log_buf) {
		pr_info("no summary buffer\n");
		return 0;
	}

	summary_info = (struct sec_debug_summary *)sec_summary_log_buf;
	memset(summary_info, 0, DUMP_SUMMARY_MAX_SIZE);

	sec_debug_save_cpu_info();

	summary_info->kernel.nr_cpus = CONFIG_NR_CPUS;

	strcpy(summary_info->summary_cmdline, saved_command_line);
	strcpy(summary_info->summary_linuxbanner, linux_banner);

	summary_info->reserved_out_buf = reserved_out_buf;
	summary_info->reserved_out_size = reserved_out_size;

	memset_io(summary_info->sched_log.sched, 0x0, sizeof(summary_info->sched_log.sched));
	memset_io(summary_info->sched_log.irq, 0x0, sizeof(summary_info->sched_log.irq));
	memset_io(summary_info->sched_log.irq_exit, 0x0, sizeof(summary_info->sched_log.irq_exit));

	for (i = 0; i < CONFIG_NR_CPUS; i++) {
		atomic_set(&summary_info->sched_log.idx_sched[i], -1);
		atomic_set(&summary_info->sched_log.idx_irq[i], -1);
		atomic_set(&summary_info->sched_log.idx_irq_exit[i], -1);
	}

	summary_info->magic[0] = SEC_DEBUG_SUMMARY_MAGIC0;
	summary_info->magic[1] = SEC_DEBUG_SUMMARY_MAGIC1;
	summary_info->magic[2] = SEC_DEBUG_SUMMARY_MAGIC2;
	summary_info->magic[3] = SEC_DEBUG_SUMMARY_MAGIC3;

	sec_debug_set_kallsyms_info(&(summary_info->ksyms), SEC_DEBUG_SUMMARY_MAGIC1);

	pr_debug("%s done [%d]\n", __func__, offset);

	return 0;
}
late_initcall(sec_debug_summary_init);

int sec_debug_save_cpu_info(void)
{
	struct cpufreq_policy *policy;
	int i;

	for_each_possible_cpu(i) {
		policy = cpufreq_cpu_get_raw(i);
		if (policy) {
			strcpy(summary_info->kernel.cpu_info[i].policy_name, policy->governor->name);
			summary_info->kernel.cpu_info[i].freq_min = policy->min;
			summary_info->kernel.cpu_info[i].freq_max = policy->max;
			summary_info->kernel.cpu_info[i].freq_cur = policy->cur;
		} else {
			summary_info->kernel.cpu_info[i].freq_min = -1;
			summary_info->kernel.cpu_info[i].freq_max = -1;
			summary_info->kernel.cpu_info[i].freq_cur = -1;
		}
	}

	return 0;
}

int sec_debug_save_die_info(const char *str, struct pt_regs *regs)
{
	if (!sec_summary_log_buf || !summary_info || !regs)
		return 0;
	snprintf(summary_info->kernel.excp.pc_sym, sizeof(summary_info->kernel.excp.pc_sym),
			"%pS", (void *)regs->ARM_PT_REG_PC);
	snprintf(summary_info->kernel.excp.lr_sym, sizeof(summary_info->kernel.excp.lr_sym),
			"%pS", (void *)regs->ARM_PT_REG_LR);

	return 0;
}

int sec_debug_save_panic_info(const char *str, unsigned long caller)
{
	if (!sec_summary_log_buf || !summary_info)
		return 0;
	snprintf(summary_info->kernel.excp.panic_caller, sizeof(summary_info->kernel.excp.panic_caller),
			"%pS", (void *)caller);
	snprintf(summary_info->kernel.excp.panic_msg, sizeof(summary_info->kernel.excp.panic_msg),
			"%s", str);
	snprintf(summary_info->kernel.excp.thread, sizeof(summary_info->kernel.excp.thread),
			"%s:%d", current->comm, task_pid_nr(current));

	return 0;
}
#endif /* CONFIG_SEC_DUMP_SUMMARY */

