/*
 * drivers/debug/sec_debug_summary.c
 *
 * driver supporting debug functions for Samsung device
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

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <soc/qcom/smem.h>
#include <linux/ptrace.h>
#include <asm/processor.h>

#include <linux/sec_debug.h>
#include <linux/sec_debug_summary.h>

struct sec_debug_summary *secdbg_summary;
struct sec_debug_summary_data_apss *secdbg_apss;
extern phys_addr_t secdbg_paddr;
extern unsigned int secdbg_size;
extern struct sec_debug_log *secdbg_log;
static char build_root[] = __FILE__;

extern unsigned int system_rev;
static uint32_t tzapps_start_addr;
static uint32_t tzapps_size;

char *sec_debug_arch_desc;

static unsigned long cpu_buf_vaddr;
static unsigned long cpu_buf_paddr;
static unsigned long cpu_data_vaddr;
static unsigned long cpu_data_paddr;
static unsigned long max_cpu_ctx_size;

#ifdef CONFIG_SEC_DEBUG_VERBOSE_SUMMARY_HTML
unsigned int cpu_frequency[CONFIG_NR_CPUS];
unsigned int cpu_volt[CONFIG_NR_CPUS];
char cpu_state[CONFIG_NR_CPUS][VAR_NAME_MAX];
#endif

#ifdef CONFIG_ARM64
#define ARM_PT_REG_PC pc
#define ARM_PT_REG_LR regs[30]
#else
#define ARM_PT_REG_PC ARM_pc
#define ARM_PT_REG_LR ARM_lr
#endif

int sec_debug_save_die_info(const char *str, struct pt_regs *regs)
{
	if (!secdbg_apss)
		return -ENOMEM;
	snprintf(secdbg_apss->excp.pc_sym, sizeof(secdbg_apss->excp.pc_sym),
		"%pS", (void *)regs->ARM_PT_REG_PC);
	snprintf(secdbg_apss->excp.lr_sym, sizeof(secdbg_apss->excp.lr_sym),
		"%pS", (void *)regs->ARM_PT_REG_LR);

#ifdef CONFIG_USER_RESET_DEBUG
	if (sec_debug_reset_ex_info) {
		if (!sec_debug_reset_ex_info->tsk) {
			int slen;
			char *msg;

			sec_debug_reset_ex_info->ktime = local_clock();
			snprintf(sec_debug_reset_ex_info->pc,
				sizeof(sec_debug_reset_ex_info->pc), "%pS", (void *)regs->ARM_PT_REG_PC);
			snprintf(sec_debug_reset_ex_info->lr,
				sizeof(sec_debug_reset_ex_info->lr), "%pS", (void *)regs->ARM_PT_REG_LR);
			slen = snprintf(sec_debug_reset_ex_info->panic_buf,
				sizeof(sec_debug_reset_ex_info->panic_buf), "%s", str);

			msg = sec_debug_reset_ex_info->panic_buf;

			if ((slen >= 1) && (msg[slen-1] == '\n'))
				msg[slen-1] = 0;

			sec_debug_reset_ex_info->tsk = current;
		}
	}
#endif

	return 0;
}

int sec_debug_save_panic_info(const char *str, unsigned long caller)
{
	if (!secdbg_apss)
		return -ENOMEM;
	snprintf(secdbg_apss->excp.panic_caller,
		sizeof(secdbg_apss->excp.panic_caller), "%pS", (void *)caller);
	snprintf(secdbg_apss->excp.panic_msg,
		sizeof(secdbg_apss->excp.panic_msg), "%s", str);
	snprintf(secdbg_apss->excp.thread,
		sizeof(secdbg_apss->excp.thread), "%s:%d", current->comm,
		task_pid_nr(current));
#ifdef CONFIG_USER_RESET_DEBUG
	if (sec_debug_reset_ex_info) {
		if (!sec_debug_reset_ex_info->tsk) {
			int slen;
			char *msg;

			sec_debug_reset_ex_info->ktime = local_clock();
			snprintf(sec_debug_reset_ex_info->lr,
				sizeof(sec_debug_reset_ex_info->lr), "%pS", (void *)caller);
			slen = snprintf(sec_debug_reset_ex_info->panic_buf,
				sizeof(sec_debug_reset_ex_info->panic_buf), "%s", str);

			msg = sec_debug_reset_ex_info->panic_buf;

			if ((slen >= 1) && (msg[slen-1] == '\n'))
				msg[slen-1] = 0;

			sec_debug_reset_ex_info->tsk = current;
		}
	}
#endif

	return 0;
}

int sec_debug_summary_add_infomon(char *name, unsigned int size, phys_addr_t pa)
{
	if (!secdbg_apss)
		return -ENOMEM;

	if (secdbg_apss->info_mon.idx >= ARRAY_SIZE(secdbg_apss->info_mon.var))
		return -ENOMEM;

	strlcpy(secdbg_apss->info_mon.var[secdbg_apss->info_mon.idx].name,
		name, sizeof(secdbg_apss->info_mon.var[0].name));
	secdbg_apss->info_mon.var[secdbg_apss->info_mon.idx].sizeof_type
		= size;
	secdbg_apss->info_mon.var[secdbg_apss->info_mon.idx].var_paddr = pa;

	secdbg_apss->info_mon.idx++;

	return 0;
}

int sec_debug_summary_add_varmon(char *name, unsigned int size, phys_addr_t pa)
{
	if (!secdbg_apss)
		return -ENOMEM;

	if (secdbg_apss->var_mon.idx > ARRAY_SIZE(secdbg_apss->var_mon.var))
		return -ENOMEM;

	strlcpy(secdbg_apss->var_mon.var[secdbg_apss->var_mon.idx].name, name,
		sizeof(secdbg_apss->var_mon.var[0].name));
	secdbg_apss->var_mon.var[secdbg_apss->var_mon.idx].sizeof_type = size;
	secdbg_apss->var_mon.var[secdbg_apss->var_mon.idx].var_paddr = pa;

	secdbg_apss->var_mon.idx++;

	return 0;
}

#ifdef CONFIG_SEC_DEBUG_MDM_FILE_INFO
void sec_set_mdm_summary_info(char *str_buf)
{
	snprintf(secdbg_apss->mdmerr_info,
		sizeof(secdbg_apss->mdmerr_info), "%s", str_buf);
}
#endif

static int ___build_root_init(char *str)
{
	char *st, *ed;
	int len;
	ed = strstr(str, "/android/kernel");
	if (!ed || ed == str)
		return -1;
	*ed = '\0';
	st = strrchr(str, '/');
	if (!st)
		return -1;
	st++;
	len = (unsigned long)ed - (unsigned long)st + 1;
	memmove(str, st, len);
	return 0;
}

#ifdef CONFIG_SEC_DEBUG_VERBOSE_SUMMARY_HTML
void sec_debug_save_cpu_freq_voltage(int cpu, int flag, unsigned long value)
{
	if (SAVE_FREQ == flag)
		cpu_frequency[cpu] = value;
	else if (SAVE_VOLT == flag)
		cpu_volt[cpu] = (unsigned int)value;
}
#else
void sec_debug_save_cpu_freq_voltage(int cpu, int flag, unsigned long value)
{
}
#endif

void sec_debug_secure_app_addr_size(uint32_t addr, uint32_t size)
{
	tzapps_start_addr = addr;
	tzapps_size = size;
}

static int __init _set_kconst(struct sec_debug_summary_data_apss *p)
{
	p->kconst.nr_cpus = NR_CPUS;
	p->kconst.per_cpu_offset.pa = virt_to_phys(__per_cpu_offset);
	p->kconst.per_cpu_offset.size = sizeof(__per_cpu_offset[0]);
	p->kconst.per_cpu_offset.count = sizeof(__per_cpu_offset)/sizeof(__per_cpu_offset[0]);
	p->kconst.virt_to_phys = (uintptr_t)virt_to_phys(0);
	p->kconst.phys_to_virt = (uintptr_t)phys_to_virt(0);

	return 0;
}

static int __init summary_init_infomon(void)
{
	if (___build_root_init(build_root) == 0)
		ADD_STR_TO_INFOMON(build_root);
	ADD_STR_TO_INFOMON(linux_banner);
	sec_debug_summary_add_infomon("Kernel cmdline", -1, __pa(saved_command_line));
	sec_debug_summary_add_infomon("Hardware name", -1, __pa(sec_debug_arch_desc));
	ADD_VAR_TO_INFOMON(system_rev);

	return 0;
}

static int __init summary_init_varmon(void)
{
	uint64_t last_pet_paddr = 0;
	uint64_t last_ns_paddr = 0;

	/* save paddrs of last_pet und last_ns */
	if (secdbg_paddr && secdbg_log) {
		last_pet_paddr = secdbg_paddr +
			offsetof(struct sec_debug_log, last_pet);
		last_ns_paddr = secdbg_paddr +
			offsetof(struct sec_debug_log, last_ns);
		sec_debug_summary_add_varmon("last_pet",
			sizeof((secdbg_log->last_pet)), last_pet_paddr);
		sec_debug_summary_add_varmon("last_ns",
				sizeof((secdbg_log->last_ns.counter)),
				last_ns_paddr);
	} else
		pr_emerg("**** secdbg_log or secdbg_paddr is not initialized ****\n");

#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
	ADD_VAR_TO_VARMON(boot_reason);
	ADD_VAR_TO_VARMON(cold_boot);
#endif

#ifdef CONFIG_SEC_DEBUG_VERBOSE_SUMMARY_HTML
	for (i = 0; i < CONFIG_NR_CPUS; i++) {
		ADD_STR_ARRAY_TO_VARMON(cpu_state[i], i, CPU_STAT_CORE);
		ADD_ARRAY_TO_VARMON(cpu_frequency[i], i, CPU_FREQ_CORE);
		ADD_ARRAY_TO_VARMON(cpu_volt[i], i, CPU_VOLT_CORE);
	}
#endif

	return 0;
}

static int __init summary_init_sched_log(struct sec_debug_summary_data_apss *p)
{
	if (!secdbg_paddr)
		return -ENOMEM;

	p->sched_log.sched_idx_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, idx_sched);
	p->sched_log.sched_buf_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, sched);
	p->sched_log.sched_struct_sz =
		sizeof(struct sched_log);
	p->sched_log.sched_array_cnt = SCHED_LOG_MAX;

	p->sched_log.irq_idx_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, idx_irq);
	p->sched_log.irq_buf_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, irq);
	p->sched_log.irq_struct_sz =
		sizeof(struct irq_log);
	p->sched_log.irq_array_cnt = SCHED_LOG_MAX;

	p->sched_log.secure_idx_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, idx_secure);
	p->sched_log.secure_buf_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, secure);
	p->sched_log.secure_struct_sz =
		sizeof(struct secure_log);
	p->sched_log.secure_array_cnt = TZ_LOG_MAX;

	p->sched_log.irq_exit_idx_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, idx_irq_exit);
	p->sched_log.irq_exit_buf_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, irq_exit);
	p->sched_log.irq_exit_struct_sz =
		sizeof(struct irq_exit_log);
	p->sched_log.irq_exit_array_cnt = SCHED_LOG_MAX;

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
	p->sched_log.msglog_idx_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, idx_secmsg);
	p->sched_log.msglog_buf_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, secmsg);
	p->sched_log.msglog_struct_sz =
		sizeof(struct secmsg_log);
	p->sched_log.msglog_array_cnt = MSG_LOG_MAX;
#else
	p->sched_log.msglog_idx_paddr = 0;
	p->sched_log.msglog_buf_paddr = 0;
	p->sched_log.msglog_struct_sz = 0;
	p->sched_log.msglog_array_cnt = 0;
#endif

	return 0;
}

static unsigned long get_wdog_regsave_paddr(void)
{
	return __pa(&cpu_buf_paddr);
}

void sec_debug_summary_bark_dump(unsigned long cpu_data,
		unsigned long pcpu_data, unsigned long cpu_buf,
		unsigned long pcpu_buf, unsigned long cpu_ctx_size)
{
	cpu_data_vaddr = cpu_data;
	cpu_data_paddr = pcpu_data;
	cpu_buf_vaddr = cpu_buf;
	cpu_buf_paddr = pcpu_buf;
	max_cpu_ctx_size = cpu_ctx_size;
}

static int sec_debug_summary_set_msm_dump_info(
		struct sec_debug_summary_data_apss *apss)
{
	apss->cpu_reg.msm_dump_info.cpu_data_paddr = cpu_data_paddr;
	apss->cpu_reg.msm_dump_info.cpu_buf_paddr = cpu_buf_paddr;
	apss->cpu_reg.msm_dump_info.cpu_ctx_size = max_cpu_ctx_size;
	apss->cpu_reg.msm_dump_info.offset = 0x10;

	pr_info("cpu_data_paddr = 0x%llx\n",
			apss->cpu_reg.msm_dump_info.cpu_data_paddr);
	pr_info("cpu_buf_paddr = 0x%llx\n",
			apss->cpu_reg.msm_dump_info.cpu_buf_paddr);

	return 0;
}

static int __init summary_init_core_reg(struct sec_debug_summary_data_apss *p)
{
	/* setup sec debug core reg info */
	p->cpu_reg.sec_debug_core_reg_paddr = virt_to_phys(&sec_debug_core_reg);

	pr_info("sec_debug_core_reg_paddr = 0x%llx\n", p->cpu_reg.sec_debug_core_reg_paddr);

#ifdef CONFIG_MSM_MEMORY_DUMP_V2
	/* setup qc core reg info */
	sec_debug_summary_set_msm_dump_info(p);
#endif

	return 0;
}

static int __init summary_init_avc_log(struct sec_debug_summary_data_apss *p)
{
	if (!secdbg_paddr)
		return -EINVAL;

#ifdef CONFIG_SEC_DEBUG_AVC_LOG
	p->avc_log.secavc_idx_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, idx_secavc);
	p->avc_log.secavc_buf_paddr = secdbg_paddr +
		offsetof(struct sec_debug_log, secavc);
	p->avc_log.secavc_struct_sz =
		sizeof(struct secavc_log);
	p->avc_log.secavc_array_cnt = AVC_LOG_MAX;
#else
	p->avc_log.secavc_idx_paddr = 0;
	p->avc_log.secavc_buf_paddr = 0;
	p->avc_log.secavc_struct_sz = 0;
	p->avc_log.secavc_array_cnt = 0;
#endif

	return 0;
}

int sec_debug_is_modem_seperate_debug_ssr(void)
{
	return secdbg_summary->priv.modem.seperate_debug;
}

int __init sec_debug_summary_init(void)
{
	pr_info("%s: SMEM_ID_VENDOR0=0x%x size=0x%zx\n",
		__func__,  (unsigned int)SMEM_ID_VENDOR2,
		sizeof(struct sec_debug_summary));

	/* set summary address in smem for other subsystems to see */
	secdbg_summary = (struct sec_debug_summary *)smem_alloc(
		SMEM_ID_VENDOR2,
		sizeof(struct sec_debug_summary),
		0,
		SMEM_ANY_HOST_FLAG);

	if (secdbg_summary == NULL) {
		pr_info("%s: smem alloc failed!\n", __func__);
		return -ENOMEM;
	}

	memset(secdbg_summary, 0, (unsigned long)sizeof(secdbg_summary));

	secdbg_summary->secure_app_start_addr = tzapps_start_addr;
	secdbg_summary->secure_app_size = tzapps_size;

	secdbg_apss = &secdbg_summary->priv.apss;

	secdbg_summary->apss = (struct sec_debug_summary_data_apss *)
	    (smem_virt_to_phys(&secdbg_summary->priv.apss)&0xFFFFFFFF);
	secdbg_summary->rpm = (struct sec_debug_summary_data *)
		(smem_virt_to_phys(&secdbg_summary->priv.rpm)&0xFFFFFFFF);
	secdbg_summary->modem = (struct sec_debug_summary_data_modem *)
		(smem_virt_to_phys(&secdbg_summary->priv.modem)&0xFFFFFFFF);
	secdbg_summary->dsps = (struct sec_debug_summary_data *)
		(smem_virt_to_phys(&secdbg_summary->priv.dsps)&0xFFFFFFFF);

	pr_info("%s: apss(%lx) rpm(%lx) modem(%lx) dsps(%lx)\n", __func__,
		(unsigned long)secdbg_summary->apss,
		(unsigned long)secdbg_summary->rpm,
		(unsigned long)secdbg_summary->modem,
		(unsigned long)secdbg_summary->dsps);


	strlcpy(secdbg_apss->name, "APSS", sizeof(secdbg_apss->name) + 1);
	strlcpy(secdbg_apss->state, "Init", sizeof(secdbg_apss->state) + 1);
	secdbg_apss->nr_cpus = CONFIG_NR_CPUS;

	sec_debug_summary_set_kloginfo(&secdbg_apss->log.first_idx_paddr,
		&secdbg_apss->log.next_idx_paddr,
		&secdbg_apss->log.log_paddr, &secdbg_apss->log.size);
	sec_debug_summary_set_logger_info(&secdbg_apss->logger_log);
	secdbg_apss->tz_core_dump =
		(struct msm_dump_data **)get_wdog_regsave_paddr();

	summary_init_infomon();

	summary_init_varmon();

	summary_init_sched_log(secdbg_apss);

	summary_init_core_reg(secdbg_apss);

	summary_init_avc_log(secdbg_apss);

	sec_debug_summary_set_kallsyms_info(secdbg_apss);

	_set_kconst(secdbg_apss);

#ifdef CONFIG_MSM_RTB
	sec_debug_summary_set_rtb_info(secdbg_apss);
#endif

	/* fill magic nubmer last to ensure data integrity when the magic
	 * numbers are written
	 */
	secdbg_summary->magic[0] = SEC_DEBUG_SUMMARY_MAGIC0;
	secdbg_summary->magic[1] = SEC_DEBUG_SUMMARY_MAGIC1;
	secdbg_summary->magic[2] = SEC_DEBUG_SUMMARY_MAGIC2;
	secdbg_summary->magic[3] = SEC_DEBUG_SUMMARY_MAGIC3;

	return 0;
}

subsys_initcall_sync(sec_debug_summary_init);
