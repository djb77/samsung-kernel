/*
 * Source file for mpsd parameters read.
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *	   Ravi Solanki <ravi.siso@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/kernel_stat.h>
#include <linux/cpufreq.h>
#include <linux/swap.h>
#include <linux/version.h>
#include <linux/irq_work.h>
#include <linux/kthread.h>

#include "../../kernel/sched/sched.h"
#include "../cpufreq/cpufreq_governor.h"
#ifdef CONFIG_QCOM_KGSL
#include "../drivers/gpu/msm/kgsl.h"
#endif
#include "mpsd_param.h"

char tag[MAX_TAG_LENGTH] = "*** MPSD ***\0";

static int cpufreq_policy_map[MAX_NUM_CPUS];

static int cpuload_history_idx;
static long long cpuload_history[MAX_NUM_CPUS][MAX_LOAD_HISTORY];

static struct workqueue_struct *cpuload_wq;
static struct delayed_work cpuload_work;

static struct mutex mpsd_read_lock;

/**
 * set_cpufreq_policy_map - Get the freq_policy_nodes.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This gives the availble policy nodes of cpu frequnecy driver.
 */
void set_cpufreq_policy_map(void)
{
	unsigned int cpu = 0;
	struct cpufreq_policy policy;

	memset(cpufreq_policy_map, 0, MAX_NUM_CPUS);
	for_each_possible_cpu(cpu) {
		if (cpufreq_get_policy(&policy, cpu) != 0) {
#ifdef CONFIG_MPSD_DEBUG
			if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
				pr_err("%s: %s: Get Policy Failed!\n",
					tag, __func__);
			}
#endif
			return;
		}

		cpufreq_policy_map[cpumask_first(policy.related_cpus)] = 1;
	}
}

/**
 * get_cpufreq_policy_map - Gives the cpu freq policy map for related cpus.
 *
 */
void get_cpufreq_policy_map(int map[])
{
	int i = 0;

	for (i = 0; i < MAX_NUM_CPUS; i++)
		map[i] = cpufreq_policy_map[i];
}

/**
 * memset_app_params - Initialize the app_params_struct structure
 *				  with default values.
 * @data: app_params_struct app params which needs to be initialized.
 *
 * This function initializes the structure members with defualt values.
 */
void memset_app_params(struct app_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);

#endif
		return;
	}

	memset(data, 0, sizeof(struct app_params_struct));
	strlcpy(data->name, DEFAULT_PARAM_VAL_STR, MAX_CHAR_BUF_SIZE);
	data->tgid = DEFAULT_PARAM_VAL_INT;
	data->flags = DEFAULT_PARAM_VAL_INT;
	data->priority = DEFAULT_PARAM_VAL_INT;
	data->nice = DEFAULT_PARAM_VAL_INT;
	data->cpus_allowed = DEFAULT_PARAM_VAL_INT;
	data->voluntary_ctxt_switches = DEFAULT_PARAM_VAL_INT;
	data->nonvoluntary_ctxt_switches = DEFAULT_PARAM_VAL_INT;
	data->num_threads = DEFAULT_PARAM_VAL_INT;
	data->oom_score_adj = DEFAULT_PARAM_VAL_INT;
	data->oom_score_adj_min = DEFAULT_PARAM_VAL_INT;
	data->utime = DEFAULT_PARAM_VAL_INT;
	data->stime = DEFAULT_PARAM_VAL_INT;
	data->mem_min_pgflt = DEFAULT_PARAM_VAL_INT;
	data->mem_maj_pgflt = DEFAULT_PARAM_VAL_INT;
	data->start_time = DEFAULT_PARAM_VAL_INT;
	data->mem_vsize = DEFAULT_PARAM_VAL_INT;
	data->mem_data = DEFAULT_PARAM_VAL_INT;
	data->mem_stack = DEFAULT_PARAM_VAL_INT;
	data->mem_text = DEFAULT_PARAM_VAL_INT;
	data->mem_swap = DEFAULT_PARAM_VAL_INT;
	data->mem_shared = DEFAULT_PARAM_VAL_INT;
	data->mem_anon = DEFAULT_PARAM_VAL_INT;
	data->mem_rss = DEFAULT_PARAM_VAL_INT;
	data->mem_rss_limit = DEFAULT_PARAM_VAL_INT;
	data->io_rchar = DEFAULT_PARAM_VAL_INT;
	data->io_wchar = DEFAULT_PARAM_VAL_INT;
	data->io_syscr = DEFAULT_PARAM_VAL_INT;
	data->io_syscw = DEFAULT_PARAM_VAL_INT;
	data->io_syscfs = DEFAULT_PARAM_VAL_INT;
	data->io_bytes_read = DEFAULT_PARAM_VAL_INT;
	data->io_bytes_write = DEFAULT_PARAM_VAL_INT;
	data->io_bytes_write_cancel = DEFAULT_PARAM_VAL_INT;
}

/**
 * get_task_from_pid - Getting task structure from pid value.
 * @pid: pid of the application whose task_struct needs to be retrieved.
 *
 * This function finds and returns the task_struct of the
 * application whose pid is passed.
 * Returns task_struct pointer on success and NULL on failure.
 */
static inline struct task_struct *get_task_from_pid(unsigned int pid)
{
	struct task_struct *task;

	task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);

	return task;
}

/**
 * is_task_valid - Gives if the task exist or not.
 * @pid: pid whose validty needs to be checked.
 *
 * This function finds if a task is valid or not.
 * Returns true or false depending on the task validity.
 */
bool is_task_valid(unsigned int pid)
{
	struct task_struct *task;

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return false;
	}
	put_task_struct(task);

	return true;
}

/**
 * get_app_name - Get the name of the process.
 * @pid: pid of the application whose name need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gives the value of name of the process.
 */
static void get_app_name(unsigned int pid, struct app_params_struct *data)
{
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	get_task_comm(data->name, task);
	put_task_struct(task);
}

/**
 * get_app_tgid - Get the thread group id of the process
 * @pid: pid of the application whose tgid need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the value fo tgid of the process.
 */
static void get_app_tgid(unsigned int pid, struct app_params_struct *data)
{
	long long tgid = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	tgid = (long long)(task->tgid);
	put_task_struct(task);

	data->tgid = tgid;
}

/**
 * get_app_flags - Get the kernel flags word of the process.
 * @pid: pid of the application whose flag param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gives the value of flags word of the process.
 */
static void get_app_flags(unsigned int pid, struct app_params_struct *data)
{
	long long flags = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}
	flags = (long long)(task->flags);
	put_task_struct(task);

	data->flags = flags;
}

/**
 * get_app_priority - Get the priority of the process.
 * @pid: pid of the application whose priority param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gives the value of priority of the process.
 */
static void get_app_priority(unsigned int pid, struct app_params_struct *data)
{
	long long priority = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}
	priority = (long long)(task->prio - MAX_RT_PRIO);
	put_task_struct(task);

	data->priority = priority;
}

/**
 * get_app_nice - Get the nice value of the process.
 * @pid: pid of the application whose nice param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gives the nice value of the process.
 */
static void get_app_nice(unsigned int pid, struct app_params_struct *data)
{
	long long nice = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}
	nice = (long long)(PRIO_TO_NICE((task)->static_prio));
	put_task_struct(task);

	data->nice	= nice;
}

/**
 * get_app_cpus_allowed - Get the cpus_allowed value of the process.
 * @pid: pid of the application whose cpus_allowed param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpus_allowed value of the process.
 * This gives the info about the CPUs on which this app can run.
 */
static void get_app_cpus_allowed(unsigned int pid,
	struct app_params_struct *data)
{
	long long cpus_allowed = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	cpus_allowed = (long long)(task->nr_cpus_allowed);
	put_task_struct(task);

	data->cpus_allowed = cpus_allowed;
}

/**
 * get_app_voluntary_ctxt_switches - Get the voluntary context switch counts.
 * @pid: pid of the app whose voluntary_ctxt_switches param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the voluntary_ctxt_switches value of the process.
 */
static void get_app_voluntary_ctxt_switches(unsigned int pid,
	struct app_params_struct *data)
{
	long long voluntary_ctxt_switches = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}
	voluntary_ctxt_switches = (long long)(task->nvcsw);
	put_task_struct(task);

	data->voluntary_ctxt_switches = voluntary_ctxt_switches;
}

/**
 * get_app_nonvoluntary_ctxt_switches - Get the nonvoluntary context switchs.
 * @pid: pid of the app whose nonvoluntary_ctxt_switches param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the nonvoluntary_ctxt_switches value of the process.
 */
static void get_app_nonvoluntary_ctxt_switches(unsigned int pid,
	struct app_params_struct *data)
{
	long long nonvoluntary_ctxt_switches = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}
	nonvoluntary_ctxt_switches = (long long)(task->nivcsw);
	put_task_struct(task);

	data->nonvoluntary_ctxt_switches = nonvoluntary_ctxt_switches;
}

/**
 * get_app_num_threads- Get the total num of threads of the process.
 * @pid: pid of the application whose num_threads param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gives the total number of threads of the process.
 */
static void get_app_num_threads(unsigned int pid,
	struct app_params_struct *data)
{
	unsigned long flags;
	long long num_threads = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		num_threads = (long long)(task->signal->nr_threads);
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->num_threads = num_threads;
}

/**
 * get_app_utime - Get the utime value of the process in clock ticks.
 * @pid: pid of the application whose utime param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gives the utime value of the process.
 * This gives the time that this app has been scheduled in user mode.
 */
static void get_app_utime(unsigned int pid, struct app_params_struct *data)
{
	unsigned long flags;
	cputime_t utime = DEFAULT_PARAM_VAL_INT;
	cputime_t stime = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	data->utime = utime;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		thread_group_cputime_adjusted(task, &utime, &stime);
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->utime = (long long)cputime_to_clock_t(utime);
}

/**
 * get_app_stime - Get the stime value of the process in clock ticks.
 * @pid: pid of the application whose stime param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gives the stime value of the process.
 * This gives the time that this app has been scheduled in kernel mode.
 */
static void get_app_stime(unsigned int pid, struct app_params_struct *data)
{
	unsigned long flags;
	cputime_t utime = DEFAULT_PARAM_VAL_INT;
	cputime_t stime = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	data->stime = stime;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		thread_group_cputime_adjusted(task, &utime, &stime);
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->stime = (long long)cputime_to_clock_t(stime);
}

/**
 * get_app_oom_score_adj - Get the oom_score_adj value of the process.
 * @pid: pid of the application whose oom_score_adj param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the oom_score_adj value of the process.
 * Based on this value decision is made on which application to kill.
 */
static void get_app_oom_score_adj(unsigned int pid,
	struct app_params_struct *data)
{
	unsigned long flags;
	long long oom_score_adj = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);

#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);

#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		oom_score_adj = (long long)(task->signal->oom_score_adj);
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->oom_score_adj = oom_score_adj;
}

/**
 * get_app_oom_score_adj_min - Get the oom_score_adj_min value of the process.
 * @pid: pid of the application whose oom_score_adj_min param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the oom_score_adj_min value of the process.
 * Minimum value of oom_score_adj set for an app.
 * It can help to provde QOS for application in certain way.
 */
static void get_app_oom_score_adj_min(unsigned int pid,
	struct app_params_struct *data)
{
	unsigned long flags;
	long long oom_score_adj_min = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);

#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);

#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		oom_score_adj_min =
			(long long)(task->signal->oom_score_adj_min);
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->oom_score_adj_min = oom_score_adj_min;
}

/**
 * get_app_mem_min_pgflt - Get the mem_min_pgflt value of the process.
 * @pid: pid of the application whose mem_min_pgflt param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_min_pgflt value of the process.
 * This gives the number of minor faults the app has made.
 */
static void get_app_mem_min_pgflt(unsigned int pid,
	struct app_params_struct *data)
{
	unsigned long flags;
	long long mem_min_pgflt = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);

#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);

#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct signal_struct *sig = task->signal;
		struct task_struct *t = task;

		mem_min_pgflt = 0;
		do {
			mem_min_pgflt += t->min_flt;
		} while_each_thread(task, t);

		mem_min_pgflt += sig->min_flt;
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->mem_min_pgflt = mem_min_pgflt;
}

/**
 * get_app_mem_max_pgflt - Get the mem_max_pgflt value of the process.
 * @pid: pid of the application whose mem_max_pgflt param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_maj_pgflt value of the process.
 * This gives the number of major faults the app has made.
 */
static void get_app_mem_maj_pgflt(unsigned int pid,
	struct app_params_struct *data)
{
	unsigned long flags;
	long long mem_maj_pgflt = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);

#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);

#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct signal_struct *sig = task->signal;
		struct task_struct *t = task;

		mem_maj_pgflt = 0;
		do {
			mem_maj_pgflt += t->maj_flt;
		} while_each_thread(task, t);

		mem_maj_pgflt += sig->maj_flt;
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->mem_maj_pgflt = mem_maj_pgflt;
}

/**
 * get_app_task_sig_params - Get all task signal related params of the app.
 * @pid: pid of the application whose task signal params need to be read.
 *
 * This function gets the values of all task signal related params.
 * Returns filled app_task_sig_params structure.
 */
struct app_task_sig_params get_app_task_sig_params(unsigned int pid)
{
	unsigned long flags;
	cputime_t utime = DEFAULT_PARAM_VAL_INT;
	cputime_t stime = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct app_task_sig_params task_sig_params;

	task_sig_params.num_threads = DEFAULT_PARAM_VAL_INT;
	task_sig_params.oom_score_adj = DEFAULT_PARAM_VAL_INT;
	task_sig_params.oom_score_adj_min = DEFAULT_PARAM_VAL_INT;
	task_sig_params.utime = DEFAULT_PARAM_VAL_INT;
	task_sig_params.stime = DEFAULT_PARAM_VAL_INT;
	task_sig_params.mem_min_pgflt = DEFAULT_PARAM_VAL_INT;
	task_sig_params.mem_maj_pgflt = DEFAULT_PARAM_VAL_INT;

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);

#endif
		goto ret;
	}

	if (lock_task_sighand(task, &flags)) {
		struct signal_struct *sig = task->signal;
		struct task_struct *t = task;

		task_sig_params.mem_maj_pgflt = 0;
		task_sig_params.mem_min_pgflt = 0;
		do {
			task_sig_params.mem_maj_pgflt += t->maj_flt;
			task_sig_params.mem_min_pgflt += t->min_flt;
		} while_each_thread(task, t);

		task_sig_params.mem_maj_pgflt += sig->maj_flt;
		task_sig_params.mem_min_pgflt += sig->min_flt;

		task_sig_params.num_threads =
			(long long)(task->signal->nr_threads);
		task_sig_params.oom_score_adj =
			(long long)(task->signal->oom_score_adj);
		task_sig_params.oom_score_adj_min =
			(long long)(task->signal->oom_score_adj_min);

		thread_group_cputime_adjusted(task, &utime, &stime);
		task_sig_params.utime = (long long)cputime_to_clock_t(utime);
		task_sig_params.stime = (long long)cputime_to_clock_t(stime);

		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);
ret:
	return task_sig_params;
}

/**
 * get_app_start_time - Get the start_time value of the process in clock ticks.
 * @pid: pid of the application whose start_time param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the start_time value of the process.
 * This gives the time the app started after system boot, in nsecs.
 */
static void get_app_start_time(unsigned int pid, struct app_params_struct *data)
{
	long long start_time = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);

#endif
		return;
	}
	start_time = (long long)nsec_to_clock_t(task->real_start_time);
	put_task_struct(task);

	data->start_time = start_time;
}

/**
 * get_app_mem_vsize - Get the vsize of the process.
 * @pid: pid of the application whose mem_vsize param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_vsize value of the process.
 * This gives the total virtual memory size in bytes of this app.
 */
static void get_app_mem_vsize(unsigned int pid, struct app_params_struct *data)
{
	long long mem_vsize = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct mm_struct *mm;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	mm = get_task_mm(task);
	if (mm) {
		mem_vsize = (long long)(mm->total_vm);
		mmput(mm);
	}
	put_task_struct(task);

	data->mem_vsize = mem_vsize;
}

/**
 * get_app_mem_data - Get the data memory size of the process.
 * @pid: pid of the application whose mem_data param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_data value of the process.
 * This gives the data + stack size in bytes of this app.
 */
static void get_app_mem_data(unsigned int pid, struct app_params_struct *data)
{
	long long mem_data = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct mm_struct *mm;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);

#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);

#endif
		return;
	}

	mm = get_task_mm(task);
	if (mm) {
		mem_data = (long long)(mm->end_data - mm->start_data);
		mmput(mm);
	}
	put_task_struct(task);

	data->mem_data = mem_data;
}

/**
 * get_app_mem_stack - Get the stack memory size of the process.
 * @pid: pid of the application whose mem_stack param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_stack value of the process.
 * This gives the stack size in bytes of this application.
 */
static void get_app_mem_stack(unsigned int pid, struct app_params_struct *data)
{
	long long mem_stack = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct mm_struct *mm;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	mm = get_task_mm(task);
	if (mm) {
		mem_stack = (long long)(mm->stack_vm);
		mmput(mm);
	}
	put_task_struct(task);

	data->mem_stack = mem_stack;
}

/**
 * get_app_mem_text - Get the text memory size of the process.
 * @pid: pid of the application whose mem_text param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_text value of the process.
 * This gives the text (code) size in pages of this app.
 */
static void get_app_mem_text(unsigned int pid, struct app_params_struct *data)
{
	long long mem_text = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct mm_struct *mm;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	mm = get_task_mm(task);
	if (mm) {
		mem_text = (long long)((PAGE_ALIGN(mm->end_code)
			  - (mm->start_code & PAGE_MASK)) >> PAGE_SHIFT);
		mmput(mm);
	}
	put_task_struct(task);

	data->mem_text = mem_text;
}

/**
 * get_app_mem_swap - Get the swap memory size of the process.
 * @pid: pid of the application whose mem_swap param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_swap value of the process.
 * This gives the swapped memory in pages of this app.
 */
static void get_app_mem_swap(unsigned int pid, struct app_params_struct *data)
{
	long long mem_swap = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct mm_struct *mm;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	mm = get_task_mm(task);
	if (mm) {
		mem_swap = (long long)(get_mm_counter(mm, MM_SWAPENTS));
		mmput(mm);
	}
	put_task_struct(task);

	data->mem_swap = mem_swap;
}

/**
 * get_app_mem_shared - Get the shared memory size of the process.
 * @pid: pid of the application whose mem_shared param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_shared value of the process.
 * This gives the shared memory in pages of this app.
 */
static void get_app_mem_shared(unsigned int pid, struct app_params_struct *data)
{
	long long mem_shared = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct mm_struct *mm;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	mm = get_task_mm(task);
	if (mm) {
		mem_shared = (long long)(get_mm_counter(mm, MM_FILEPAGES));
		mmput(mm);
	}
	put_task_struct(task);

	data->mem_shared = mem_shared;
}

/**
 * get_app_mem_anon - Get the anonymous memory size of the process.
 * @pid: pid of the application whose mem_anon param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_anon value of the process.
 * This gives the anonymous memory in pages of this app.
 */
static void get_app_mem_anon(unsigned int pid, struct app_params_struct *data)
{
	long long mem_anon = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct mm_struct *mm;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	mm = get_task_mm(task);
	if (mm) {
		mem_anon = (long long)(get_mm_counter(mm, MM_ANONPAGES));
		mmput(mm);
	}
	put_task_struct(task);

	data->mem_anon = mem_anon;
}

/**
 * get_app_mem_rss - Get the resident memory size value of the process.
 * @pid: pid of the application whose mem_rss param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_rss value of the process.
 * This gives the rss value of this app. Pages the app has in real memory.
 */
static void get_app_mem_rss(unsigned int pid, struct app_params_struct *data)
{
	long long mem_rss = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct mm_struct *mm;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	mm = get_task_mm(task);
	if (mm) {
		mem_rss = (long long)(get_mm_counter(mm, MM_FILEPAGES)
			+ get_mm_counter(mm, MM_ANONPAGES));
		mmput(mm);
	}
	put_task_struct(task);

	data->mem_rss = mem_rss;
}

/**
 * get_app_mem_rss_limit - Get the rss_limit value of the process.
 * @pid: pid of the application whose mem_rss_limit param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_rss_limit value of the process.
 * This gives the current soft limit in bytes on the rss of the app.
 */
static void get_app_mem_rss_limit(unsigned int pid,
	struct app_params_struct *data)
{
	unsigned long flags;
	long long mem_rss_limit = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		mem_rss_limit = (long long)(ACCESS_ONCE(
			task->signal->rlim[RLIMIT_RSS].rlim_cur));
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->mem_rss_limit = mem_rss_limit;
}

/**
 * get_app_mem_params - Get all memory related params of the app.
 * @pid: pid of the application whose memory params need to be read.
 *
 * This function gets the values of all memory related params.
 * Returns filled app_mem_params structure.
 */
struct app_mem_params get_app_mem_params(unsigned int pid)
{
	unsigned long flags;
	struct task_struct *task;
	struct mm_struct *mm;
	struct app_mem_params mem_params;

	mem_params.mem_vsize = DEFAULT_PARAM_VAL_INT;
	mem_params.mem_data = DEFAULT_PARAM_VAL_INT;
	mem_params.mem_stack = DEFAULT_PARAM_VAL_INT;
	mem_params.mem_text = DEFAULT_PARAM_VAL_INT;
	mem_params.mem_swap = DEFAULT_PARAM_VAL_INT;
	mem_params.mem_shared = DEFAULT_PARAM_VAL_INT;
	mem_params.mem_anon = DEFAULT_PARAM_VAL_INT;
	mem_params.mem_rss = DEFAULT_PARAM_VAL_INT;
	mem_params.mem_rss_limit = DEFAULT_PARAM_VAL_INT;

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		goto ret;
	}

	mm = get_task_mm(task);
	if (mm) {
		mem_params.mem_vsize = (long long)(mm->total_vm);
		mem_params.mem_data =
			(long long)(mm->end_data - mm->start_data);
		mem_params.mem_stack = (long long)(mm->stack_vm);
		mem_params.mem_text = (long long)((PAGE_ALIGN(mm->end_code)
			- (mm->start_code & PAGE_MASK)) >> PAGE_SHIFT);
		mem_params.mem_swap =
			(long long)(get_mm_counter(mm, MM_SWAPENTS));
		mem_params.mem_shared = (long long)(get_mm_counter(mm,
			MM_FILEPAGES));
		mem_params.mem_anon =
			(long long)(get_mm_counter(mm, MM_ANONPAGES));
		mem_params.mem_rss =
			(long long)(get_mm_counter(mm, MM_FILEPAGES)
			+ get_mm_counter(mm, MM_ANONPAGES));
		mmput(mm);
	}

	if (lock_task_sighand(task, &flags)) {
		mem_params.mem_rss_limit = (long long)(ACCESS_ONCE(
			task->signal->rlim[RLIMIT_RSS].rlim_cur));
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

ret:
	return mem_params;
}

#ifdef CONFIG_TASK_XACCT
/**
 * get_app_io_rchar - Get the io rchar value of the process.
 * @pid: pid of the application whose io_rchar param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the io rchar value of the process.
 * This gives the number of bytes read by this application.
 */
static void get_app_io_rchar(unsigned int pid, struct app_params_struct *data)
{
	unsigned long flags;
	long long io_rchar = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		io_rchar = task->signal->ioac.rchar;
		while_each_thread(task, t) {
			io_rchar += t->ioac.rchar;
		}
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->io_rchar = io_rchar;
}

/**
 * get_app_io_wchar - Get the io wchar value of the process.
 * @pid: pid of the application whose io_wchar param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the io wchar value of the process.
 * This gives the number of bytes wrriten by this application.
 */
static void get_app_io_wchar(unsigned int pid, struct app_params_struct *data)
{
	unsigned long flags;
	long long io_wchar = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		io_wchar = task->signal->ioac.wchar;
		while_each_thread(task, t) {
			io_wchar += t->ioac.wchar;
		}
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->io_wchar = io_wchar;
}

/**
 * get_app_io_syscr - Get the io syscr value of the process.
 * @pid: pid of the application whose io_syscr param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the io syscr value of the process.
 * This gives the number of read syscalls by this application.
 */
static void get_app_io_syscr(unsigned int pid, struct app_params_struct *data)
{
	unsigned long flags;
	long long io_syscr = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		io_syscr = task->signal->ioac.syscr;
		while_each_thread(task, t) {
			io_syscr += t->ioac.syscr;
		}
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->io_syscr = io_syscr;
}

/**
 * get_app_io_syscw - Get the io syscw value of the process.
 * @pid: pid of the application whose io_syscw param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the io syscw value of the process.
 * This gives the number of write syscalls by this application.
 */
static void get_app_io_syscw(unsigned int pid, struct app_params_struct *data)
{
	unsigned long flags;
	long long io_syscw = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		io_syscw = task->signal->ioac.syscw;
		while_each_thread(task, t) {
			io_syscw += t->ioac.syscw;
		}
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->io_syscw = io_syscw;
}

/**
 * get_app_io_syscfs - Get the io syscfs value of the process.
 * @pid: pid of the application whose io_syscfs param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the io syscfs value of the process.
 * This gives the number of fsync syscalls by this application.
 */
static void get_app_io_syscfs(unsigned int pid, struct app_params_struct *data)
{
	unsigned long flags;
	long long io_syscfs = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		io_syscfs = task->signal->ioac.syscfs;
		while_each_thread(task, t) {
			io_syscfs += t->ioac.syscfs;
		}
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->io_syscfs = io_syscfs;
}
#endif /* CONFIG_TASK_XACCT */

#ifdef CONFIG_TASK_IO_ACCOUNTING
/**
 * get_app_io_bytes_read - Get the io_bytes_read value of the process.
 * @pid: pid of the application whose io_bytes_read param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the io_bytes_read value of the process.
 * This gives the number of bytes caused to be read from storage.
 */
static void get_app_io_bytes_read(unsigned int pid,
	struct app_params_struct *data)
{
	unsigned long flags;
	long long io_bytes_read = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		io_bytes_read = task->signal->ioac.read_bytes;
		while_each_thread(task, t) {
			io_bytes_read += t->ioac.read_bytes;
		}
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->io_bytes_read = io_bytes_read;
}

/**
 * get_app_io_bytes_write - Get the io_bytes_write value of the process.
 * @pid: pid of the application whose io_bytes_write param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the io_bytes_write value of the process.
 * This gives the number of bytes caused to be written to storage.
 */
static void get_app_io_bytes_write(unsigned int pid,
	struct app_params_struct *data)
{
	unsigned long flags;
	long long io_bytes_write = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		io_bytes_write = task->signal->ioac.write_bytes;
		while_each_thread(task, t) {
			io_bytes_write += t->ioac.write_bytes;
		}
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->io_bytes_write = io_bytes_write;
}

/**
 * get_app_io_bytes_write_cancel - Get the io_bytes_write_cancel
 *	   value of the process.
 * @pid: pid of the app whose io_bytes_write_cancel param need to be read.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the io_bytes_write_cancel value of the process.
 * This gives the number of bytes cancelled from writting.
 */
static void get_app_io_bytes_write_cancel(unsigned int pid,
	struct app_params_struct *data)
{
	unsigned long flags;
	long long io_bytes_write_cancel = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	if (lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		io_bytes_write_cancel = task->signal
			->ioac.cancelled_write_bytes;
		while_each_thread(task, t) {
			io_bytes_write_cancel += t->ioac.cancelled_write_bytes;
		}
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

	data->io_bytes_write_cancel = io_bytes_write_cancel;
}
#endif /* CONFIG_TASK_IO_ACCOUNTING */

/**
 * get_app_io_params - Get all io related params data.
 * @pid: pid of the application whose io params need to be read.
 *
 * This function gets the values of all io related params
 * existing in the io accounting.
 * Returns filled app_io_params structure.
 */
struct app_io_params get_app_io_params(unsigned int pid)
{
	unsigned long flags;
	struct task_struct *task;
	struct app_io_params io_params;

	memset(&io_params, 0, sizeof(struct app_io_params));
#ifdef CONFIG_TASK_XACCT
	io_params.io_rchar = DEFAULT_PARAM_VAL_INT;
	io_params.io_wchar = DEFAULT_PARAM_VAL_INT;
	io_params.io_syscr = DEFAULT_PARAM_VAL_INT;
	io_params.io_syscw = DEFAULT_PARAM_VAL_INT;
	io_params.io_syscfs = DEFAULT_PARAM_VAL_INT;
#endif /* CONFIG_TASK_XACCT */
#ifdef CONFIG_TASK_IO_ACCOUNTING
	io_params.io_bytes_read = DEFAULT_PARAM_VAL_INT;
	io_params.io_bytes_write = DEFAULT_PARAM_VAL_INT;
	io_params.io_bytes_write_cancel = DEFAULT_PARAM_VAL_INT;
#endif /* CONFIG_TASK_IO_ACCOUNTING */

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		goto ret;
	}

	if (lock_task_sighand(task, &flags)) {
		struct task_struct *t = task;

		if (IS_ENABLED(CONFIG_TASK_XACCT)) {
			io_params.io_rchar = task->signal->ioac.rchar;
			io_params.io_wchar = task->signal->ioac.wchar;
			io_params.io_syscr = task->signal->ioac.syscr;
			io_params.io_syscw = task->signal->ioac.syscw;
			io_params.io_syscfs = task->signal->ioac.syscfs;
		}
		if (IS_ENABLED(CONFIG_TASK_IO_ACCOUNTING)) {
			io_params.io_bytes_read = task->signal
				->ioac.read_bytes;
			io_params.io_bytes_write = task->signal
				->ioac.write_bytes;
			io_params.io_bytes_write_cancel = task->signal
				->ioac.cancelled_write_bytes;
		}

		while_each_thread(task, t) {
			if (IS_ENABLED(CONFIG_TASK_XACCT)) {
				io_params.io_rchar += t->signal->ioac.rchar;
				io_params.io_wchar += t->signal->ioac.wchar;
				io_params.io_syscr += t->signal->ioac.syscr;
				io_params.io_syscw += t->signal->ioac.syscw;
				io_params.io_syscfs += t->signal->ioac.syscfs;
			}
			if (IS_ENABLED(CONFIG_TASK_IO_ACCOUNTING)) {
				io_params.io_bytes_read += t->signal
					->ioac.read_bytes;
				io_params.io_bytes_write += t->signal
					->ioac.write_bytes;
				io_params.io_bytes_write_cancel += t->signal
					->ioac.cancelled_write_bytes;
			}
		}
		unlock_task_sighand(task, &flags);
	}
	put_task_struct(task);

ret:
	return io_params;
}

/**
 * get_app_params_snapshot - Get all application level params data in one shot.
 * @pid: pid of the application whose param snapshot is needed.
 * @app_params: The read data will be filled here.
 *
 * This function takes the snapshot of all the application level params.
 */
void get_app_params_snapshot(unsigned int pid,
	struct app_params_struct *app_params)
{
	unsigned long flags;
	cputime_t utime = DEFAULT_PARAM_VAL_INT;
	cputime_t stime = DEFAULT_PARAM_VAL_INT;
	struct task_struct *task;
	struct mm_struct *mm;

	memset_app_params(app_params);

	task = get_task_from_pid(pid);
	if (unlikely(!task)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: task not found\n", tag, __func__);
#endif
		return;
	}

	get_task_comm(app_params->name, task);
	app_params->tgid = (long long)(task->tgid);
	app_params->flags = (long long)(task->flags);
	app_params->priority = (long long)(task->prio - MAX_RT_PRIO);
	app_params->nice = (long long)(PRIO_TO_NICE((task)->static_prio));
	app_params->cpus_allowed = (long long)(task->nr_cpus_allowed);
	app_params->voluntary_ctxt_switches = (long long)(task->nvcsw);
	app_params->nonvoluntary_ctxt_switches = (long long)(task->nivcsw);
	app_params->start_time =
		(long long)nsec_to_clock_t(task->real_start_time);

	mm = get_task_mm(task);
	if (mm) {
		app_params->mem_vsize = (long long)(mm->total_vm);
		app_params->mem_data =
			(long long)(mm->end_data - mm->start_data);
		app_params->mem_stack = (long long)(mm->stack_vm);
		app_params->mem_text = (long long)((PAGE_ALIGN(mm->end_code)
			- (mm->start_code & PAGE_MASK)) >> PAGE_SHIFT);
		app_params->mem_swap = (long long)(get_mm_counter(mm,
			MM_SWAPENTS));
		app_params->mem_shared = (long long)(get_mm_counter(mm,
			MM_FILEPAGES));
		app_params->mem_anon = (long long)(get_mm_counter(mm,
			MM_ANONPAGES));
		app_params->mem_rss =
			(long long)(get_mm_counter(mm, MM_FILEPAGES)
			+ get_mm_counter(mm, MM_ANONPAGES));
		mmput(mm);
	}

	if (lock_task_sighand(task, &flags)) {
		struct signal_struct *sig = task->signal;
		struct task_struct *t = task;

		app_params->mem_maj_pgflt = 0;
		app_params->mem_min_pgflt = 0;

		if (IS_ENABLED(CONFIG_TASK_XACCT)) {
			app_params->io_rchar = task->signal->ioac.rchar;
			app_params->io_wchar = task->signal->ioac.wchar;
			app_params->io_syscr = task->signal->ioac.syscr;
			app_params->io_syscw = task->signal->ioac.syscw;
			app_params->io_syscfs = task->signal->ioac.syscfs;
		}

		if (IS_ENABLED(CONFIG_TASK_IO_ACCOUNTING)) {
			app_params->io_bytes_read = task->signal
				->ioac.read_bytes;
			app_params->io_bytes_write = task->signal
				->ioac.write_bytes;
			app_params->io_bytes_write_cancel = task->signal
				->ioac.cancelled_write_bytes;
		}

		while_each_thread(task, t) {
			if (IS_ENABLED(CONFIG_TASK_XACCT)) {
				app_params->io_rchar += t->signal->ioac.rchar;
				app_params->io_wchar += t->signal->ioac.wchar;
				app_params->io_syscr += t->signal->ioac.syscr;
				app_params->io_syscw += t->signal->ioac.syscw;
				app_params->io_syscfs += t->signal->ioac.syscfs;
			}

			if (IS_ENABLED(CONFIG_TASK_IO_ACCOUNTING)) {
				app_params->io_bytes_read += t->signal
					->ioac.read_bytes;
				app_params->io_bytes_write += t->signal
					->ioac.write_bytes;
				app_params->io_bytes_write_cancel += t->signal
					->ioac.cancelled_write_bytes;
			}

			app_params->mem_maj_pgflt += t->maj_flt;
			app_params->mem_min_pgflt += t->min_flt;
		}

		app_params->mem_maj_pgflt += sig->maj_flt;
		app_params->mem_min_pgflt += sig->min_flt;

		app_params->num_threads =
			(long long)(task->signal->nr_threads);
		app_params->oom_score_adj =
			(long long)(task->signal->oom_score_adj);
		app_params->oom_score_adj_min =
			(long long)(task->signal->oom_score_adj_min);

		thread_group_cputime_adjusted(task, &utime, &stime);
		app_params->utime = (long long)utime;
		app_params->stime = (long long)stime;

		app_params->mem_rss_limit = (long long)(ACCESS_ONCE(
			task->signal->rlim[RLIMIT_RSS].rlim_cur));

		unlock_task_sighand(task, &flags);
	}

	put_task_struct(task);
}

/**
 * struct app_param_lookup - Contains param id and associated read function.
 * @param: Application parameter whose value need to be read.
 * @param_read: Function pointer for read function related to param (first arg)
 *
 * This structure contains param id and associated read function pointer.
 */
struct app_param_lookup {
	enum app_param param;
	void (*param_read)(unsigned int, struct app_params_struct*);
};

/**
 * Lookup table of function pointer for application parameters read.
 */
static const struct app_param_lookup app_param_read_funcs[] = {
	{ APP_PARAM_NAME, get_app_name },
	{ APP_PARAM_TGID, get_app_tgid },
	{ APP_PARAM_FLAGS, get_app_flags },
	{ APP_PARAM_PRIORITY, get_app_priority },
	{ APP_PARAM_NICE, get_app_nice },
	{ APP_PARAM_CPUS_ALLOWED, get_app_cpus_allowed },
	{ APP_PARAM_VOLUNTARY_CTXT_SWITCHES, get_app_voluntary_ctxt_switches },
	{ APP_PARAM_NONVOLUNTARY_CTXT_SWITCHES,
		get_app_nonvoluntary_ctxt_switches },
	{ APP_PARAM_NUM_THREADS, get_app_num_threads },
	{ APP_PARAM_UTIME, get_app_utime },
	{ APP_PARAM_STIME, get_app_stime },
	{ APP_PARAM_OOM_SCORE_ADJ, get_app_oom_score_adj },
	{ APP_PARAM_OOM_SCORE_ADJ_MIN, get_app_oom_score_adj_min },
	{ APP_PARAM_MEM_MIN_PGFLT, get_app_mem_min_pgflt },
	{ APP_PARAM_MEM_MAJ_PGFLT, get_app_mem_maj_pgflt },
	{ APP_PARAM_STARTTIME, get_app_start_time },
	{ APP_PARAM_MEM_VSIZE, get_app_mem_vsize },
	{ APP_PARAM_MEM_DATA, get_app_mem_data },
	{ APP_PARAM_MEM_STACK, get_app_mem_stack },
	{ APP_PARAM_MEM_TEXT, get_app_mem_text },
	{ APP_PARAM_MEM_SWAP, get_app_mem_swap },
	{ APP_PARAM_MEM_SHARED, get_app_mem_shared },
	{ APP_PARAM_MEM_ANON, get_app_mem_anon },
	{ APP_PARAM_MEM_RSS, get_app_mem_rss },
	{ APP_PARAM_MEM_RSS_LIMIT, get_app_mem_rss_limit },
	{ APP_PARAM_IO_RCHAR, get_app_io_rchar },
	{ APP_PARAM_IO_WCHAR, get_app_io_wchar },
	{ APP_PARAM_IO_SYSCR, get_app_io_syscr },
	{ APP_PARAM_IO_SYSCW, get_app_io_syscw },
	{ APP_PARAM_IO_SYSCFS, get_app_io_syscfs },
	{ APP_PARAM_IO_BYTES_READ, get_app_io_bytes_read },
	{ APP_PARAM_IO_BYTES_WRITE, get_app_io_bytes_write },
	{ APP_PARAM_IO_BYTES_WRITE_CANCEL, get_app_io_bytes_write_cancel }
};

/**
 * mpsd_get_app_params - Reads the value of the app parameter passed.
 * @pid: Process for which params are needed to be read.
 * @param: Device parameter whose data need to be read.
 * @data: The read data will be filled here.
 *
 * This function reads the value of the app parameter passed.
 */
void mpsd_get_app_params(unsigned int pid, enum app_param param,
	struct app_params_struct *data)
{
	unsigned int i = 0, size = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	if (pid <= 0) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
			pr_err("%s: %s: pid(%d) is negative or zero\n", tag,
				__func__, pid);
		}
#endif
		return;
	}

	if (!is_valid_app_param(param)) {
		pr_err("%s: %s: param[%d] invalid!\n", tag, __func__, param);
		return;
	}

	size = (unsigned int)ARRAY_SIZE(app_param_read_funcs);

	for (i = 0; i < size; i++) {
		if (app_param_read_funcs[i].param == param) {
			(app_param_read_funcs[i].param_read)(pid, data);
			break;
		}
	}
}

/**
 * memset_dev_params - Initialize the dev_params_struct structure
 *				  with default values.
 * @data: dev_params_struct dev params which needs to be initialized.
 *
 * This function initializes the structure members with defualt values.
 */
void memset_dev_params(struct dev_params_struct *data)
{
	int i = 0, j = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	memset(data, 0, sizeof(struct dev_params_struct));
	data->cpu_present = DEFAULT_PARAM_VAL_INT;
	data->cpu_online = DEFAULT_PARAM_VAL_INT;
	data->cpu_time_user = DEFAULT_PARAM_VAL_INT;
	data->cpu_time_nice = DEFAULT_PARAM_VAL_INT;
	data->cpu_time_system = DEFAULT_PARAM_VAL_INT;
	data->cpu_time_idle = DEFAULT_PARAM_VAL_INT;
	data->cpu_time_io_Wait = DEFAULT_PARAM_VAL_INT;
	data->cpu_time_irq = DEFAULT_PARAM_VAL_INT;
	data->cpu_time_soft_irq = DEFAULT_PARAM_VAL_INT;
	data->cpu_loadavg_1min = DEFAULT_PARAM_VAL_INT;
	data->cpu_loadavg_5min = DEFAULT_PARAM_VAL_INT;
	data->cpu_loadavg_15min = DEFAULT_PARAM_VAL_INT;
	data->system_uptime = DEFAULT_PARAM_VAL_INT;
	data->mem_total = DEFAULT_PARAM_VAL_INT;
	data->mem_free = DEFAULT_PARAM_VAL_INT;
	data->mem_avail = DEFAULT_PARAM_VAL_INT;
	data->mem_buffer = DEFAULT_PARAM_VAL_INT;
	data->mem_cached = DEFAULT_PARAM_VAL_INT;
	data->mem_swap_cached = DEFAULT_PARAM_VAL_INT;
	data->mem_swap_total = DEFAULT_PARAM_VAL_INT;
	data->mem_swap_free = DEFAULT_PARAM_VAL_INT;
	data->mem_dirty = DEFAULT_PARAM_VAL_INT;
	data->mem_anon_pages = DEFAULT_PARAM_VAL_INT;
	data->mem_mapped = DEFAULT_PARAM_VAL_INT;
	data->mem_shmem = DEFAULT_PARAM_VAL_INT;
	data->mem_sysheap = DEFAULT_PARAM_VAL_INT;
	data->mem_sysheap_pool = DEFAULT_PARAM_VAL_INT;
	data->mem_vmalloc_api = DEFAULT_PARAM_VAL_INT;
	data->mem_kgsl_shared = DEFAULT_PARAM_VAL_INT;
	data->mem_zswap = DEFAULT_PARAM_VAL_INT;

	for (i = 0; i < MAX_NUM_CPUS; i++) {
		for (j = 0; j < MAX_FREQ_LIST_SIZE; j++) {
			data->cpu_freq_avail[i][j] = DEFAULT_PARAM_VAL_INT;
			data->cpu_freq_time_stats[i][j].freq =
				DEFAULT_PARAM_VAL_INT;
			data->cpu_freq_time_stats[i][j].time =
				DEFAULT_PARAM_VAL_INT;
		}

		for (j = 0; j < MAX_LOAD_HISTORY; j++)
			data->cpu_util[i][j] = DEFAULT_PARAM_VAL_INT;

		data->cpu_max_limit[i] = DEFAULT_PARAM_VAL_INT;
		data->cpu_min_limit[i] = DEFAULT_PARAM_VAL_INT;
		data->cpu_cur_freq[i] = DEFAULT_PARAM_VAL_INT;
		strlcpy(data->cpu_freq_gov[i].gov, CPUFREQ_GOV_NONE,
			MAX_CHAR_BUF_SIZE);
	}
}

/**
 * get_dev_cpu_present - Get the cpu_present param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_present param value of the device.
 * This gives the number of CPUs that are actualy present in the hardware.
 */
static void get_dev_cpu_present(struct dev_params_struct *data)
{
	long long cpu_present = DEFAULT_PARAM_VAL_INT;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	cpu_present = (long long)num_present_cpus();
	data->cpu_present = cpu_present;
}

/**
 * get_dev_cpu_online - Get the cpu_online param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_online param value of the device.
 * This gives the number of CPUs which are currently online.
 */
static void get_dev_cpu_online(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	data->cpu_online = (long long)num_online_cpus();
}

/**
 * get_dev_cpu_util - Get the cpu_util param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_util param value of the device.
 * This gives the load on each CPUs.
 */
static void get_dev_cpu_util(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	memcpy(data->cpu_util, cpuload_history, sizeof(data->cpu_util));
}

/**
 * get_dev_cpu_freq_avail - Get the cpu_freq_avail param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_freq_avail param value of the device.
 * This gives the list of CPU frequencies available.
 */
static void get_dev_cpu_freq_avail(struct dev_params_struct *data)
{
	unsigned int cpu = 0;
	unsigned int count = 0;
	struct cpufreq_policy policy;
	struct cpufreq_frequency_table *pos;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for (cpu = 0; cpu < MAX_NUM_CPUS; cpu++) {
		if (cpufreq_policy_map[cpu] == 1) {
			if (cpufreq_get_policy(&policy, cpu) != 0) {
#ifdef CONFIG_MPSD_DEBUG
				if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
					pr_err("%s: %s: Get Policy Failed!\n",
						tag, __func__);
				}
#endif
				return;
			}

			if (policy.freq_table != NULL) {
				count = 0;
				cpufreq_for_each_valid_entry(pos,
					policy.freq_table) {
					data->cpu_freq_avail[cpu][count++] =
						pos->frequency;
				}
			}
		}
	}
}

/**
 * get_dev_cpu_max_limit - Get the cpu_max_limit param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_max_limit param value of the device.
 * This gives the maximum frequency limit put by CFMS, -1 if not set.
 */
static void get_dev_cpu_max_limit(struct dev_params_struct *data)
{
	unsigned int cpu = 0;
	struct cpufreq_policy policy;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for (cpu = 0; cpu < MAX_NUM_CPUS; cpu++) {
		if (cpufreq_policy_map[cpu] == 1) {
			if (cpufreq_get_policy(&policy, cpu) != 0) {
#ifdef CONFIG_MPSD_DEBUG
				if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
					pr_err("%s: %s: Get Policy Failed!\n",
						tag, __func__);
				}
#endif
				return;
			}

			data->cpu_max_limit[cpu] = policy.max;
		}
	}
}

/**
 * get_dev_cpu_min_limit - Get the cpu_min_limit param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_min_limit param value of the device.
 * This gives the minimum frequency limit put by CFMS, -1 if not set.
 */
static void get_dev_cpu_min_limit(struct dev_params_struct *data)
{
	unsigned int cpu = 0;
	struct cpufreq_policy policy;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for (cpu = 0; cpu < MAX_NUM_CPUS; cpu++) {
		if (cpufreq_policy_map[cpu] == 1) {
			if (cpufreq_get_policy(&policy, cpu) != 0) {
#ifdef CONFIG_MPSD_DEBUG
				if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
					pr_err("%s: %s: Get Policy Failed!\n",
						tag, __func__);
				}
#endif
				return;
			}

			data->cpu_min_limit[cpu] = policy.min;
		}
	}
}

/**
 * get_dev_cpu_cur_freq - Get the cpu_cur_freq param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_cur_freq param value of the device.
 * This gives the current cpu frequency of all the cpufreq driver
 * policy nodes in the system.
 */
static void get_dev_cpu_cur_freq(struct dev_params_struct *data)
{
	unsigned int cpu = 0;
	struct cpufreq_policy policy;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for (cpu = 0; cpu < MAX_NUM_CPUS; cpu++) {
		if (cpufreq_policy_map[cpu] == 1) {
			if (cpufreq_get_policy(&policy, cpu) != 0) {
#ifdef CONFIG_MPSD_DEBUG
				if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
					pr_err("%s: %s: Get Policy Failed!\n",
						tag, __func__);
				}
#endif
				return;
			}

			data->cpu_cur_freq[cpu] = policy.cur;
		}
	}
}

struct cpufreq_stats {
	unsigned int total_trans;
	unsigned long long last_time;
	unsigned int max_state;
	unsigned int state_num;
	unsigned int last_index;
	u64 *time_in_state;
	unsigned int *freq_table;
#ifdef CONFIG_CPU_FREQ_STAT_DETAILS
	unsigned int *trans_table;
#endif
};

/**
 * get_dev_cpu_freq_time_stats - Get the cpu_freq_time_stats param.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_freq_time_stats param value of the device.
 * This gives the list of CPU frequencies available and corresponding
 * time in those frequencies.
 */
static void get_dev_cpu_freq_time_stats(struct dev_params_struct *data)
{
	unsigned int cpu = 0;
	unsigned int i = 0;
	struct cpufreq_policy policy;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for (cpu = 0; cpu < MAX_NUM_CPUS; cpu++) {
		if (cpufreq_policy_map[cpu] == 1) {
			if (cpufreq_get_policy(&policy, cpu) != 0) {
#ifdef CONFIG_MPSD_DEBUG
				if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
					pr_err("%s: %s: Get Policy Failed!\n",
						tag, __func__);
				}
#endif
				return;
			}

			if (policy.stats != NULL) {
				for (i = 0; i < policy.stats->state_num; i++) {
					data->cpu_freq_time_stats[cpu][i]
						.freq = (long long)
						(policy.stats->freq_table[i]);
					data->cpu_freq_time_stats[cpu][i]
						.time = (long long)
						jiffies_64_to_clock_t(
						policy.stats->time_in_state[i]);
				}
			}
		}
	}
}

#ifdef CONFIG_CPU_FREQ_GOV_PERFORMANCE
/**
 * get_performance_gov_params - Get the performance gov params.
 * @policy: Cpu frequency policy of the current policy node.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the params related to performance gov.
 */
static inline void get_performance_gov_params(struct cpufreq_policy *policy,
	union cpufreq_gov_data *data)
{
	data->gov_performance.cur_freq = (long long)policy->cur;
}
#endif

#ifdef CONFIG_CPU_FREQ_GOV_POWERSAVE
/**
 * get_powersave_gov_params - Get the powersave gov params.
 * @policy: Cpu frequency policy of the current policy node.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the params related to powersave gov.
 */
static inline void get_powersave_gov_params(struct cpufreq_policy *policy,
	union cpufreq_gov_data *data)
{
	data->gov_powersave.cur_freq = (long long)policy->cur;
}
#endif

#ifdef CONFIG_CPU_FREQ_GOV_USERSPACE
/**
 * get_userspace_gov_params - Get the userspace gov params.
 * @policy: Cpu frequency policy of the current policy node.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the params related to userspace gov.
 */
static inline void get_userspace_gov_params(struct cpufreq_policy *policy,
	union cpufreq_gov_data *data)
{
	unsigned int *gov_data = policy->governor_data;

	data->gov_userspace.set_speed = (long long)*gov_data;
}
#endif

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND
struct od_gov_tuners {
	unsigned int ignore_nice_load;
	unsigned int sampling_rate;
	unsigned int sampling_down_factor;
	unsigned int up_threshold;
	unsigned int powersave_bias;
	unsigned int io_is_busy;
};

/**
 * get_ondemand_gov_params - Get the ondemand gov params.
 * @policy: Cpu frequency policy of the current policy node.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the params related to ondemand gov.
 */
static inline void get_ondemand_gov_params(struct cpufreq_policy *policy,
	union cpufreq_gov_data *data)
{
	struct dbs_data *gov_data = policy->governor_data;
	struct od_gov_tuners *tuners = gov_data->tuners;

	if (unlikely(!tuners)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: tuners is NULL\n", tag, __func__);
#endif
		return;
	}

	data->gov_ondemand.min_sampling_rate =
		gov_data->min_sampling_rate;
	data->gov_ondemand.ignore_nice_load =
		tuners->ignore_nice_load;
	data->gov_ondemand.sampling_rate =
		tuners->sampling_rate;
	data->gov_ondemand.sampling_down_factor =
		tuners->sampling_down_factor;
	data->gov_ondemand.up_threshold =
		tuners->up_threshold;
	data->gov_ondemand.io_is_busy =
		tuners->io_is_busy;
	data->gov_ondemand.powersave_bias =
		tuners->powersave_bias;
}
#endif

#ifdef CONFIG_CPU_FREQ_GOV_CONSERVATIVE
struct cs_gov_tuners {
	unsigned int ignore_nice_load;
	unsigned int sampling_rate;
	unsigned int sampling_down_factor;
	unsigned int up_threshold;
	unsigned int down_threshold;
	unsigned int freq_step;
};

/**
 * get_conservative_gov_params - Get the conservative gov params.
 * @policy: Cpu frequency policy of the current policy node.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the params related to conservative gov.
 */
static inline void get_conservative_gov_params(struct cpufreq_policy *policy,
	union cpufreq_gov_data *data)
{
	struct dbs_data *gov_data = policy->governor_data;
	struct cs_dbs_tuners *tuners = gov_data->tuners;

	if (unlikely(!tuners)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: tuners is NULL\n", tag, __func__);
#endif
		return;
	}

	data->gov_conservative.min_sampling_rate =
		gov_data->min_sampling_rate;
	data->gov_conservative.ignore_nice_load =
		tuners->ignore_nice_load;
	data->gov_conservative.sampling_rate =
		tuners->sampling_rate;
	data->gov_conservative.sampling_down_factor =
		tuners->sampling_down_factor;
	data->gov_conservative.up_threshold =
		tuners->up_threshold;
	data->gov_conservative.down_threshold =
		tuners->down_threshold;
	data->gov_conservative.freq_step =
		tuners->freq_step;
}
#endif

#ifdef CONFIG_CPU_FREQ_GOV_INTERACTIVE
struct interactive_gov_tunables {
	int usage_count;
	unsigned int hispeed_freq;
	unsigned long go_hispeed_load;
	spinlock_t target_loads_lock;
	unsigned int *target_loads;
	int ntarget_loads;
	unsigned long min_sample_time;
	unsigned long timer_rate;
	spinlock_t above_hispeed_delay_lock;
	unsigned int *above_hispeed_delay;
	int nabove_hispeed_delay;
	int boost_val;
	int boostpulse_duration_val;
	u64 boostpulse_endtime;
	bool boosted;
	int timer_slack_val;
	bool io_is_busy;
	bool use_sched_load;
	bool use_migration_notif;
	bool align_windows;
	unsigned int max_freq_hysteresis;
	bool ignore_hispeed_on_notif;
	bool fast_ramp_down;
	bool enable_prediction;
#ifdef CONFIG_DYNAMIC_MODE_SUPPORT
	spinlock_t mode_lock;
	spinlock_t param_index_lock;
	unsigned int mode;
	unsigned int old_mode;
	unsigned int param_index;
	unsigned int cur_param_index;
	unsigned int *target_loads_set[MAX_PARAM_SET];
	int ntarget_loads_set[MAX_PARAM_SET];
	unsigned int *above_hispeed_delay_set[MAX_PARAM_SET];
	int nabove_hispeed_delay_set[MAX_PARAM_SET];
	unsigned int hispeed_freq_set[MAX_PARAM_SET];
	unsigned long go_hispeed_load_set[MAX_PARAM_SET];
#endif /* CONFIG_DYNAMIC_MODE_SUPPORT */
};

/**
 * get_interactive_gov_params - Get the interactive gov params.
 * @policy: Cpu frequency policy of the current policy node.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the params related to interactive gov.
 */
static inline void get_interactive_gov_params(struct cpufreq_policy *policy,
	union cpufreq_gov_data *data)
{
	unsigned long flags;
	struct interactive_gov_tunables *gov_data = policy->governor_data;

	data->gov_interactive.hispeed_freq =
		gov_data->hispeed_freq;
	data->gov_interactive.go_hispeed_load =
		gov_data->go_hispeed_load;
	spin_lock_irqsave(&gov_data->target_loads_lock, flags);
	data->gov_interactive.target_loads =
		*gov_data->target_loads;
	spin_unlock_irqrestore(&gov_data->target_loads_lock, flags);
	data->gov_interactive.min_sample_time =
		gov_data->min_sample_time;
	data->gov_interactive.timer_rate =
		gov_data->timer_rate;
	spin_lock_irqsave(&gov_data->above_hispeed_delay_lock, flags);
	data->gov_interactive.above_hispeed_delay =
		*gov_data->above_hispeed_delay;
	spin_unlock_irqrestore(&gov_data->above_hispeed_delay_lock, flags);
	data->gov_interactive.boost =
		gov_data->boost_val;
	data->gov_interactive.boostpulse_duration =
		gov_data->boostpulse_duration_val;
	data->gov_interactive.timer_slack =
		gov_data->timer_slack_val;
	data->gov_interactive.io_is_busy =
		gov_data->io_is_busy;
	data->gov_interactive.use_sched_load =
		gov_data->use_sched_load;
	data->gov_interactive.use_migration_notif =
		gov_data->use_migration_notif;
	data->gov_interactive.align_windows =
		gov_data->align_windows;
	data->gov_interactive.max_freq_hysteresis =
		gov_data->max_freq_hysteresis;
	data->gov_interactive.ignore_hispeed_on_notif =
		gov_data->ignore_hispeed_on_notif;
	data->gov_interactive.fast_ramp_down =
		gov_data->fast_ramp_down;
	data->gov_interactive.enable_prediction =
		gov_data->enable_prediction;
}
#endif

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL
struct sugov_tunables {
	struct gov_attr_set attr_set;
	unsigned int up_rate_limit_us;
	unsigned int down_rate_limit_us;
};

struct su_gov_policy {
	struct cpufreq_policy *policy;
	struct sugov_tunables *tunables;
	struct list_head tunables_hook;
	raw_spinlock_t update_lock;
	u64 last_freq_update_time;
	s64 min_rate_limit_ns;
	s64 up_rate_delay_ns;
	s64 down_rate_delay_ns;
	unsigned int next_freq;
	unsigned int cached_raw_freq;
	struct irq_work irq_work;
	struct kthread_work work;
	struct mutex work_lock;
	struct kthread_worker worker;
	struct task_struct *thread;
	bool work_in_progress;
	bool need_freq_update;
};

/**
 * get_schedutil_gov_params - Get the schedutil gov params.
 * @policy: Cpu frequency policy of the current policy node.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the params related to schedutil gov.
 */
static inline void get_schedutil_gov_params(struct cpufreq_policy *policy,
	union cpufreq_gov_data *data)
{
	struct su_gov_policy *gov_data = policy->governor_data;

	data->gov_schedutil.up_rate_limit_us =
		(long long)gov_data->tunables->up_rate_limit_us;
	data->gov_schedutil.up_rate_limit_us =
		(long long)gov_data->tunables->down_rate_limit_us;
}
#endif

/**
 * get_cpufreq_gov_params - Get the params of cpufreq governors.
 * @policy: Cpu frequency policy of the current policy node.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the params related to current gov used.
 */
static inline void get_cpufreq_gov_params(struct cpufreq_policy *policy,
	struct dev_cpu_freq_gov *data)
{
	strlcpy(data->gov, policy->governor->name, MAX_CHAR_BUF_SIZE);
	if (!strcmp(policy->governor->name,
		CPUFREQ_GOV_PERFORMANCE)) {
#ifdef CONFIG_CPU_FREQ_GOV_PERFORMANCE
		get_performance_gov_params(policy, &data->cpu_freq_gov_data);
#endif
	} else if (!strcmp(policy->governor->name,
		CPUFREQ_GOV_POWERSAVE)) {
#ifdef CONFIG_CPU_FREQ_GOV_POWERSAVE
		get_powersave_gov_params(policy, &data->cpu_freq_gov_data);
#endif
	} else if (!strcmp(policy->governor->name,
		CPUFREQ_GOV_USERSPACE)) {
#ifdef CONFIG_CPU_FREQ_GOV_USERSPACE
		get_userspace_gov_params(policy, &data->cpu_freq_gov_data);
#endif
	} else if (!strcmp(policy->governor->name,
		CPUFREQ_GOV_ONDEMAND)) {
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND
		get_ondemand_gov_params(policy, &data->cpu_freq_gov_data);
#endif
	} else if (!strcmp(policy->governor->name,
		CPUFREQ_GOV_CONSERVATIVE)) {
#ifdef CONFIG_CPU_FREQ_GOV_CONSERVATIVE
		get_conservative_gov_params(policy, &data->cpu_freq_gov_data);
#endif
	} else if (!strcmp(policy->governor->name,
		CPUFREQ_GOV_INTERACTIVE)) {
#ifdef CONFIG_CPU_FREQ_GOV_INTERACTIVE
		get_interactive_gov_params(policy, &data->cpu_freq_gov_data);
#endif
	} else if (!strcmp(policy->governor->name,
		CPUFREQ_GOV_SCHEDUTIL)) {
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL
		get_schedutil_gov_params(policy, &data->cpu_freq_gov_data);
#endif
	}
}

/**
 * get_dev_cpu_freq_gov - Get the cpufreq gov params.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the params related to cpu frequency gov.
 */
static void get_dev_cpu_freq_gov(struct dev_params_struct *data)
{
	unsigned int cpu = 0;
	struct cpufreq_policy policy;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for (cpu = 0; cpu < MAX_NUM_CPUS; cpu++) {
		if (cpufreq_policy_map[cpu] == 1) {
			if (cpufreq_get_policy(&policy, cpu) != 0) {
#ifdef CONFIG_MPSD_DEBUG
				if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
					pr_err("%s: %s: Get Policy Failed!\n",
						tag, __func__);
				}
#endif
				return;
			}

			get_cpufreq_gov_params(&policy,
				&data->cpu_freq_gov[cpu]);
		}
	}
}

/**
 * get_dev_cpu_time_user - Get the cpu_time_user param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_time_user param value of the device.
 * This gives the time spent in user mode in units of USER_HZ.
 */
static void get_dev_cpu_time_user(struct dev_params_struct *data)
{
	int cpu = 0;
	unsigned long long user_time = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for_each_possible_cpu(cpu) {
		user_time += kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	}

	data->cpu_time_user =
		(long long)cputime64_to_clock_t(user_time);
}

/**
 * get_dev_cpu_time_nice - Get the cpu_time_nice param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_time_nice param value of the device.
 * This gives the time spent in user mode with low priority (nice) in USER_HZ.
 */
static void get_dev_cpu_time_nice(struct dev_params_struct *data)
{
	int cpu = 0;
	unsigned long long nice_time = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for_each_possible_cpu(cpu) {
		nice_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];
	}

	data->cpu_time_nice =
		(long long)cputime64_to_clock_t(nice_time);
}

/**
 * get_dev_cpu_time_system - Get the cpu_time_system param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_time_system param value of the device.
 * This gives the time spent in system mode in units of USER_HZ.
 */
static void get_dev_cpu_time_system(struct dev_params_struct *data)
{
	int cpu = 0;
	unsigned long long system_time = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for_each_possible_cpu(cpu) {
		system_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	}

	data->cpu_time_system =
		(long long)cputime64_to_clock_t(system_time);
}

/**
 * get_dev_cpu_time_idle - Get the cpu_time_idle param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_time_idle param value of the device.
 * This gives the time spent in the idle task.
 */
static void get_dev_cpu_time_idle(struct dev_params_struct *data)
{
	int cpu = 0;
	unsigned long long idle_time = -1ULL;
	unsigned long long idle_total = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for_each_possible_cpu(cpu) {
		if (cpu_online(cpu))
			idle_time = get_cpu_idle_time_us(cpu, NULL);

		if (idle_time == -1ULL)
			idle_total += kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
		else
			idle_total += usecs_to_cputime64(idle_time);
	}

	data->cpu_time_idle =
		(long long)cputime64_to_clock_t(idle_total);
}

/**
 * get_dev_cpu_time_io_wait - Get the cpu_time_io_wait param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_time_io_wait param value of the device.
 * This gives the time waiting for I/O to complete in units of USER_HZ.
 */
static void get_dev_cpu_time_io_wait(struct dev_params_struct *data)
{
	int cpu = 0;
	unsigned long long iowait_time = -1ULL;
	unsigned long long iowait_total = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for_each_possible_cpu(cpu) {
		if (cpu_online(cpu))
			iowait_time = get_cpu_iowait_time_us(cpu, NULL);

		if (iowait_time == -1ULL) {
			iowait_total += kcpustat_cpu(cpu)
				.cpustat[CPUTIME_IOWAIT];
		} else
			iowait_total += usecs_to_cputime64(iowait_time);
	}

	data->cpu_time_io_Wait =
		(long long)cputime64_to_clock_t(iowait_total);
}

/**
 * get_dev_cpu_time_irq - Get the cpu_time_irq param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_time_irq param value of the device.
 * This gives the time servicing interrupts in units of USER_HZ.
 */
static void get_dev_cpu_time_irq(struct dev_params_struct *data)
{
	int i = 0;
	unsigned long long irq_time = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for_each_possible_cpu(i)
		irq_time += kcpustat_cpu(i).cpustat[CPUTIME_IRQ];

	data->cpu_time_irq =
		(long long)cputime64_to_clock_t(irq_time);
}

/**
 * get_dev_cpu_time_soft_irq - Get the cpu_time_soft_irq param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_time_soft_irq param value of the device.
 * This gives the time servicing softirqs in units of USER_HZ.
 */
static void get_dev_cpu_time_soft_irq(struct dev_params_struct *data)
{
	int i = 0;
	unsigned long long softirq_time = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	for_each_possible_cpu(i)
		softirq_time += kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];

	data->cpu_time_soft_irq =
		(long long)cputime64_to_clock_t(softirq_time);
}

/**
 * get_dev_cpu_loadavg_1min - Get the cpu_loadavg_1min param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_loadavg_1min param value of the device.
 * This gives the Number of jobs in the run queue (state R)
 * or waiting for disk I/O(state D) averaged over 1 min.
 */
static void get_dev_cpu_loadavg_1min(struct dev_params_struct *data)
{
	unsigned long loads[3];
	unsigned long *ptr = loads;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	get_avenrun(ptr, FIXED_1/200, 0);
	data->cpu_loadavg_1min = (long long)loads[0];
}

/**
 * get_dev_cpu_loadavg_5min - Get the cpu_loadavg_5min param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_loadavg_5min param value of the device.
 * This gives the Number of jobs in the run queue (state R)
 * or waiting for disk I/O(state D) averaged over 5 min.
 */
static void get_dev_cpu_loadavg_5min(struct dev_params_struct *data)
{
	unsigned long loads[3];
	unsigned long *ptr = loads;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	get_avenrun(ptr, FIXED_1/200, 0);
	data->cpu_loadavg_5min = (long long)loads[1];
}

/**
 * get_dev_cpu_loadavg_15min - Get the cpu_loadavg_15min param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the cpu_loadavg_15min param value of the device.
 * This gives the Number of jobs in the run queue (state R)
 * or waiting for disk I/O(state D) averaged over 15 min.
 */
static void get_dev_cpu_loadavg_15min(struct dev_params_struct *data)
{
	unsigned long loads[3];
	unsigned long *ptr = loads;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	get_avenrun(ptr, FIXED_1/200, 0);
	data->cpu_loadavg_15min = (long long)loads[2];
}

/**
 * get_dev_system_uptime - Get the system_uptime param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the system_uptime param value of the device.
 * This gives the uptime of the system in seconds.
 */
static void get_dev_system_uptime(struct dev_params_struct *data)
{
	struct timespec uptime;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	get_monotonic_boottime(&uptime);
	data->system_uptime = (long long)uptime.tv_sec;
}

/**
 * get_dev_mem_total - Get the mem_total param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_total param value of the device.
 * This gives the total usable RAM.
 */
static void get_dev_mem_total(struct dev_params_struct *data)
{
	struct sysinfo meminfo;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	si_meminfo(&meminfo);
	data->mem_total = (long long)meminfo.totalram;
}

/**
 * get_dev_mem_free - Get the mem_free param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_free param value of the device.
 * This gives the sum of LowFree + HighFree.
 */
static void get_dev_mem_free(struct dev_params_struct *data)
{
	struct sysinfo meminfo;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	si_meminfo(&meminfo);
	data->mem_free = (long long)meminfo.freeram;
}

/**
 * get_dev_mem_avail - Get the mem_avail param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_avail param value of the device.
 * This gives An estimate of how much memory is available for
 * starting new applications, without swapping.
 */
static void get_dev_mem_avail(struct dev_params_struct *data)
{
	int lru;
	long available;
	unsigned long pagecache;
	unsigned long wmark_low = 0;
	unsigned long pages[NR_LRU_LISTS];
	struct zone *zone;
	struct sysinfo meminfo;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	si_meminfo(&meminfo);

	for (lru = LRU_BASE; lru < NR_LRU_LISTS; lru++)
		pages[lru] = global_page_state(NR_LRU_BASE + lru);

	for_each_zone(zone)
		wmark_low += zone->watermark[WMARK_LOW];

	available = meminfo.freeram - wmark_low;
	pagecache = pages[LRU_ACTIVE_FILE] + pages[LRU_INACTIVE_FILE];
	pagecache -= min(pagecache / 2, wmark_low);
	available += pagecache;
	available += global_page_state(NR_SLAB_RECLAIMABLE)
		- min(global_page_state(NR_SLAB_RECLAIMABLE) / 2, wmark_low);

	if (available < 0)
		available = 0;

	data->mem_avail = (long long)available;
}

/**
 * get_dev_mem_buffer - Get the mem_buffer param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_buffer param value of the device.
 * This gives the relatively temporary storage for raw disk blocks
 * that shouldn't get tremendously large.
 */
static void get_dev_mem_buffer(struct dev_params_struct *data)
{
	struct sysinfo meminfo;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	si_meminfo(&meminfo);
	data->mem_buffer = meminfo.bufferram;
}

/**
 * get_dev_mem_cached - Get the mem_cached param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_cached param value of the device.
 * This gives in-memory cache for files read from the disk (the page cache).
 */
static void get_dev_mem_cached(struct dev_params_struct *data)
{
	long cached;
	struct sysinfo meminfo;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	si_meminfo(&meminfo);

	cached = global_page_state(NR_FILE_PAGES) - total_swapcache_pages()
		- meminfo.bufferram;
	if (cached < 0)
		cached = 0;

	data->mem_cached = (long long)cached;
}

/**
 * get_dev_mem_swap_cached - Get the mem_swap_cached param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_swap_cached param value of the device.
 * This gives the memory that once was swapped out, is swapped back in
 * but still also is in the swap file.
 */
static void get_dev_mem_swap_cached(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	data->mem_swap_cached = (long long)total_swapcache_pages();
}

/**
 * get_dev_mem_swap_total - Get the mem_swap_total param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_swap_total param value of the device.
 * This gives the total amount of swap space available.
 */
static void get_dev_mem_swap_total(struct dev_params_struct *data)
{
	struct sysinfo meminfo;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	si_swapinfo(&meminfo);
	data->mem_swap_total = (long long)meminfo.totalswap;
}

/**
 * get_dev_mem_swap_free - Get the mem_swap_free param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_swap_free param value of the device.
 * This gives the amount of swap space that is currently unused.
 */
static void get_dev_mem_swap_free(struct dev_params_struct *data)
{
	struct sysinfo meminfo;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	si_swapinfo(&meminfo);
	data->mem_swap_free = (long long)meminfo.freeswap;
}

/**
 * get_dev_mem_dirty - Get the mem_dirty param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_dirty param value of the device.
 * This gives the memory which is waiting to get written back to the disk.
 */
static void get_dev_mem_dirty(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	data->mem_dirty =
		(long long)global_page_state(NR_FILE_DIRTY);
}

/**
 * get_dev_mem_anon_pages - Get the mem_anon_pages param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_anon_pages param value of the device.
 * This gives the non-file backed pages mapped into user-space page tables.
 */
static void get_dev_mem_anon_pages(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	data->mem_anon_pages =
		(long long)global_page_state(NR_ANON_MAPPED);
}

/**
 * get_dev_mem_mapped - Get the mem_mapped param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_mapped param value of the device.
 * This gives the files which have been mapped into memory, such as libraries.
 */
static void get_dev_mem_mapped(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	data->mem_mapped =
		(long long)global_page_state(NR_FILE_MAPPED);
}

/**
 * get_dev_mem_shmem - Get the mem_shmem param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_shmem param value of the device.
 * This gives the amount of memory consumed in tmpfs(5) filesystems.
 */
static void get_dev_mem_shmem(struct dev_params_struct *data)
{
	struct sysinfo meminfo;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	si_meminfo(&meminfo);
	data->mem_shmem = (long long)meminfo.sharedram;
}

/**
 * get_dev_mem_sysheap - Get the mem_sysheap param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_sysheap param value of the device.
 * This gives the system heap memory.
 */
static void get_dev_mem_sysheap(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}
}

/**
 * get_dev_mem_sysheap_pool - Get the mem_sysheap_pool param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_sysheap_pool param value of the device.
 * This gives the system heap memory pool.
 */
static void get_dev_mem_sysheap_pool(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}
}

//extern atomic_long_t nr_vmalloc_pages;
/**
 * get_dev_mem_vmalloc_api - Get the mem_vmalloc_api param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_vmalloc_api param value of the device.
 * This gives the memory of vmallock.
 */
static void get_dev_mem_vmalloc_api(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	data->mem_vmalloc_api = DEFAULT_PARAM_VAL_INT;
}

#ifdef CONFIG_QCOM_KGSL
//extern struct kgsl_driver kgsl_driver;
/**
 * get_dev_mem_kgsl_shared - Get the mem_kgsl_shared param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_kgsl_shared param value of the device.
 * This gives shared memory used by Kgsl.
 */
static void get_dev_mem_kgsl_shared(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);

#endif
		return;
	}

	data->mem_kgsl_shared = DEFAULT_PARAM_VAL_INT;
}
#endif

//extern u64 zswap_pool_pages;
/**
 * get_dev_mem_zswap - Get the mem_zswap param of the device.
 * @data: Pointer to structure in which read data need to be filled.
 *
 * This function gets the mem_zswap param value of the device.
 * This gives the zswap memory of the device.
 */
static void get_dev_mem_zswap(struct dev_params_struct *data)
{
	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	data->mem_zswap = DEFAULT_PARAM_VAL_INT;
}

/**
 * get_dev_cpufreq_params - Fills the cpufreq stats params.
 * @dev_params: The read data will be filled here.
 *
 * This function fill the members of the structure dev_params_struct
 * which are related to memory.
 */
static void get_dev_cpufreq_params(struct dev_params_struct *dev_params)
{
	unsigned int cpu = 0, i = 0;
	unsigned int count = 0;
	struct cpufreq_policy policy;
	struct cpufreq_frequency_table *pos;

	if (unlikely(!dev_params)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: dev_params is null\n", tag, __func__);
#endif
		return;
	}

	for (cpu = 0; cpu < MAX_NUM_CPUS; cpu++) {
		if (cpufreq_policy_map[cpu] == 1) {
			if (cpufreq_get_policy(&policy, cpu) != 0) {
#ifdef CONFIG_MPSD_DEBUG
				if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
					pr_err("%s: %s: Get Policy Failed!\n",
						tag, __func__);
				}
#endif
				return;
			}

			if (policy.freq_table != NULL) {
				count = 0;
				cpufreq_for_each_valid_entry(pos, policy
					.freq_table) {
					dev_params->cpu_freq_avail[cpu][count++]
					= pos->frequency;
				}
			}

			if (policy.stats != NULL) {
				for (i = 0; i < policy.stats->state_num; i++) {
					dev_params->cpu_freq_time_stats[cpu][i]
						.freq = (long long)
						(policy.stats->freq_table[i]);
					dev_params->cpu_freq_time_stats[cpu][i]
						.time = (long long)
						jiffies_64_to_clock_t(
						policy.stats->time_in_state[i]);
				}
			}

			dev_params->cpu_max_limit[cpu] = policy.max;
			dev_params->cpu_min_limit[cpu] = policy.min;
			dev_params->cpu_cur_freq[cpu] = policy.cur;
			get_cpufreq_gov_params(&policy,
				&dev_params->cpu_freq_gov[cpu]);
		}
	}
}

/**
 * get_dev_mem_params - Fills the memory related params.
 * @dev_params: The read data will be filled here.
 *
 * This function fill the members of the structure dev_params_struct
 * which are related to memory.
 */
static void get_dev_mem_params(struct dev_params_struct *dev_params)
{
	int lru = 0;
	long available = 0;
	long cached = 0;
	unsigned long pagecache = 0;
	unsigned long wmark_low = 0;
	unsigned long pages[NR_LRU_LISTS] = {0};
	struct zone *zone;
	struct sysinfo meminfo;

	if (unlikely(!dev_params)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: dev_params is null\n", tag, __func__);
#endif
		return;
	}

	si_meminfo(&meminfo);
	si_swapinfo(&meminfo);

	dev_params->mem_total = (long long)meminfo.totalram;
	dev_params->mem_free = (long long)meminfo.freeram;
	dev_params->mem_buffer = meminfo.bufferram;
	dev_params->mem_shmem = (long long)meminfo.sharedram;
	dev_params->mem_swap_total = (long long)meminfo.totalswap;
	dev_params->mem_swap_free = (long long)meminfo.freeswap;

	for (lru = LRU_BASE; lru < NR_LRU_LISTS; lru++)
		pages[lru] = global_page_state(NR_LRU_BASE + lru);

	for_each_zone(zone)
		wmark_low += zone->watermark[WMARK_LOW];

	available = meminfo.freeram - wmark_low;
	pagecache = pages[LRU_ACTIVE_FILE] + pages[LRU_INACTIVE_FILE];
	pagecache -= min(pagecache / 2, wmark_low);
	available += pagecache;
	available += global_page_state(NR_SLAB_RECLAIMABLE)
		- min(global_page_state(NR_SLAB_RECLAIMABLE) / 2, wmark_low);
	if (available < 0)
		available = 0;

	dev_params->mem_avail = (long long)available;

	cached = global_page_state(NR_FILE_PAGES) - total_swapcache_pages()
		- meminfo.bufferram;
	if (cached < 0)
		cached = 0;

	dev_params->mem_cached = (long long)cached;

	dev_params->mem_swap_cached = (long long)total_swapcache_pages();
	dev_params->mem_dirty = (long long)global_page_state(NR_FILE_DIRTY);
	dev_params->mem_anon_pages =
		(long long)global_page_state(NR_ANON_MAPPED);
	dev_params->mem_mapped = (long long)global_page_state(NR_FILE_MAPPED);
	dev_params->mem_sysheap = -1;
	dev_params->mem_sysheap_pool = -1;
	dev_params->mem_vmalloc_api = DEFAULT_PARAM_VAL_INT;
#ifdef CONFIG_QCOM_KGSL
	dev_params->mem_kgsl_shared = DEFAULT_PARAM_VAL_INT;
#endif
	dev_params->mem_zswap = DEFAULT_PARAM_VAL_INT;
}

/**
 * get_dev_cpu_params - Fills the cpu stats data.
 * @dev_params: The read data will be filled here.
 *
 * This function fill the members of the structure dev_params_struct
 * which are related to CPU stats.
 */
static void get_dev_cpu_params(struct dev_params_struct *dev_params)
{
	unsigned int cpu = 0;
	unsigned long loads[3];
	unsigned long *ptr = loads;
	unsigned long long user_time = 0;
	unsigned long long nice_time = 0;
	unsigned long long system_time = 0;
	unsigned long long idle_time = -1ULL;
	unsigned long long idle_total = 0;
	unsigned long long iowait_time = -1ULL;
	unsigned long long iowait_total = 0;
	unsigned long long irq_time = 0;
	unsigned long long softirq_time = 0;
	struct timespec uptime;

	if (unlikely(!dev_params)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: dev_params is null\n", tag, __func__);
#endif
		return;
	}

	dev_params->cpu_present = (int)num_present_cpus();
	dev_params->cpu_online = (int)num_online_cpus();

	for_each_possible_cpu(cpu) {
		user_time += kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
		nice_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];
		system_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
		irq_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
		softirq_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];

		if (cpu_online(cpu)) {
			idle_time = get_cpu_idle_time_us(cpu, NULL);
			iowait_time = get_cpu_iowait_time_us(cpu, NULL);
		}

		if (idle_time == -1ULL)
			idle_total += kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
		else
			idle_total += usecs_to_cputime64(idle_time);

		if (iowait_time == -1ULL) {
			iowait_total +=
				kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
		} else
			iowait_total += usecs_to_cputime64(iowait_time);
	}

	memcpy(dev_params->cpu_util, cpuload_history,
		sizeof(dev_params->cpu_util));

	dev_params->cpu_time_user =
		(long long)cputime64_to_clock_t(user_time);
	dev_params->cpu_time_nice =
		(long long)cputime64_to_clock_t(nice_time);
	dev_params->cpu_time_system =
		(long long)cputime64_to_clock_t(system_time);
	dev_params->cpu_time_idle =
		(long long)cputime64_to_clock_t(idle_total);
	dev_params->cpu_time_io_Wait =
		(long long)cputime64_to_clock_t(iowait_total);
	dev_params->cpu_time_irq =
		(long long)cputime64_to_clock_t(irq_time);
	dev_params->cpu_time_soft_irq =
		(long long)cputime64_to_clock_t(softirq_time);

	get_avenrun(ptr, FIXED_1/200, 0);
	dev_params->cpu_loadavg_1min = loads[0];
	dev_params->cpu_loadavg_5min = loads[1];
	dev_params->cpu_loadavg_15min = loads[2];

	get_monotonic_boottime(&uptime);
	dev_params->system_uptime = (long long)uptime.tv_sec;
}

/**
 * get_dev_params_snapshot - Get all device level params data in one shot.
 * @dev_params: The read data will be filled here.
 *
 * This function takes the snapshot of all the device level params.
 */
void  get_dev_params_snapshot(struct dev_params_struct *dev_params)
{
	if (unlikely(!dev_params)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: dev_params is null\n", tag, __func__);
#endif
		return;
	}

	get_dev_cpufreq_params(dev_params);
	get_dev_mem_params(dev_params);
	get_dev_cpu_params(dev_params);
}

/**
 * struct dev_param_lookup - Contains param id and associated read function.
 * @param: Device parameter whose value need to be read.
 * @param_read: Function pointer for read function related to param (first arg)
 *
 * This structure contains param id and associated read function pointer.
 */
struct dev_param_lookup {
	enum dev_param param;
	void (*param_read)(struct dev_params_struct *);
};

/**
 * Lookup table of function pointer for device parameters read.
 */
static const struct dev_param_lookup dev_param_read_funcs[] = {
	{ DEV_PARAM_CPU_PRESENT, get_dev_cpu_present },
	{ DEV_PARAM_CPU_ONLINE, get_dev_cpu_online },
	{ DEV_PARAM_CPU_UTIL, get_dev_cpu_util },
	{ DEV_PARAM_CPU_FREQ_AVAIL, get_dev_cpu_freq_avail },
	{ DEV_PARAM_CPU_MAX_LIMIT, get_dev_cpu_max_limit },
	{ DEV_PARAM_CPU_MIN_LIMIT, get_dev_cpu_min_limit },
	{ DEV_PARAM_CPU_CUR_FREQ, get_dev_cpu_cur_freq },
	{ DEV_PARAM_CPU_FREQ_TIME_STATS, get_dev_cpu_freq_time_stats },
	{ DEV_PARAM_CPU_FREQ_GOV, get_dev_cpu_freq_gov },
	{ DEV_PARAM_CPU_LOADAVG_1MIN, get_dev_cpu_loadavg_1min },
	{ DEV_PARAM_CPU_LOADAVG_5MIN, get_dev_cpu_loadavg_5min },
	{ DEV_PARAM_CPU_LOADAVG_15MIN, get_dev_cpu_loadavg_15min },
	{ DEV_PARAM_SYSTEM_UPTIME, get_dev_system_uptime },
	{ DEV_PARAM_CPU_TIME_USER, get_dev_cpu_time_user },
	{ DEV_PARAM_CPU_TIME_NICE, get_dev_cpu_time_nice },
	{ DEV_PARAM_CPU_TIME_SYSTEM, get_dev_cpu_time_system },
	{ DEV_PARAM_CPU_TIME_IDLE, get_dev_cpu_time_idle },
	{ DEV_PARAM_CPU_TIME_IO_WAIT, get_dev_cpu_time_io_wait },
	{ DEV_PARAM_CPU_TIME_IRQ, get_dev_cpu_time_irq },
	{ DEV_PARAM_CPU_TIME_SOFT_IRQ, get_dev_cpu_time_soft_irq },
	{ DEV_PARAM_MEM_TOTAL, get_dev_mem_total },
	{ DEV_PARAM_MEM_FREE, get_dev_mem_free },
	{ DEV_PARAM_MEM_AVAIL, get_dev_mem_avail },
	{ DEV_PARAM_MEM_BUFFER, get_dev_mem_buffer },
	{ DEV_PARAM_MEM_CACHED, get_dev_mem_cached },
	{ DEV_PARAM_MEM_SWAP_CACHED, get_dev_mem_swap_cached },
	{ DEV_PARAM_MEM_SWAP_TOTAL, get_dev_mem_swap_total },
	{ DEV_PARAM_MEM_SWAP_FREE, get_dev_mem_swap_free },
	{ DEV_PARAM_MEM_DIRTY, get_dev_mem_dirty },
	{ DEV_PARAM_MEM_ANON_PAGES, get_dev_mem_anon_pages },
	{ DEV_PARAM_MEM_MAPPED, get_dev_mem_mapped },
	{ DEV_PARAM_MEM_SHMEM, get_dev_mem_shmem },
	{ DEV_PARAM_MEM_SYSHEAP, get_dev_mem_sysheap },
	{ DEV_PARAM_MEM_SYSHEAP_POOL, get_dev_mem_sysheap_pool },
	{ DEV_PARAM_MEM_VMALLOC_API, get_dev_mem_vmalloc_api },
#ifdef CONFIG_QCOM_KGSL
	{ DEV_PARAM_MEM_KGSL_SHARED, get_dev_mem_kgsl_shared },
#endif
	{ DEV_PARAM_MEM_ZSWAP, get_dev_mem_zswap }
};

/**
 * mpsd_get_dev_params - Reads the value of the device parameter passed.
 * @param: Device parameter whose data need to be read.
 * @data: The read data will be filled here.
 *
 * This function reads the value of the device parameter passed.
 */
void mpsd_get_dev_params(enum dev_param param, struct dev_params_struct *data)
{
	unsigned int i = 0, size = 0;

	if (unlikely(!data)) {
#ifdef CONFIG_MPSD_DEBUG
		if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN)
			pr_err("%s: %s: data is NULL\n", tag, __func__);
#endif
		return;
	}

	if (unlikely(!is_valid_dev_param(param))) {
		pr_err("%s: %s: param[%d] invalid!\n", tag, __func__, param);
		return;
	}

	size = (unsigned int)ARRAY_SIZE(dev_param_read_funcs);

	for (i = 0; i < size; i++) {
		if (dev_param_read_funcs[i].param == param) {
			(dev_param_read_funcs[i].param_read)(data);
			break;
		}
	}
}

/**
 * mpsd_populate_mmap- updates the mmap region with params values.
 * @app_field_mask: Bitmap for read of specific app parameters.
 * @dev_field_mask: Bitmap for read of specific device parameters.
 * @pid: Device parameter whose data need to be read.
 * @req: Sync, Timed or Notification update.
 * @event: Which notification update has occured.
 *
 * Returns 0 if populating is success , negative if failed.
 */
int mpsd_populate_mmap(unsigned long long app_field_mask,
	unsigned long long dev_field_mask, int pid, enum req_id req,
	unsigned int event)
{
	int i = 0, ret = 0;
	struct memory_area *mmap_buf = NULL;

#ifdef CONFIG_MPSD_DEBUG
	if (get_debug_level_read() <= get_mpsd_debug_level()) {
		pr_info("%s: %s: Config[%d] Pid[%d] Masks[%llu, %llu] ReqID[%d] E[%d] ->\n",
			tag, __func__, get_behavior(),
			pid, app_field_mask, dev_field_mask, (int)req, event);
	}
#endif

	mutex_lock(&mpsd_read_lock);

	mmap_buf = get_device_mmap();
	if (unlikely(!mmap_buf)) {
		pr_err("%s: %s: mmap_buf is null\n", tag, __func__);
		mutex_unlock(&mpsd_read_lock);
		return -EPERM;
	}

	memset(mmap_buf, 0, sizeof(struct memory_area));
	mmap_buf->updating = 1;
	mmap_buf->req = req;
	mmap_buf->event = event;

	mmap_buf->dev_field_mask = dev_field_mask;
	memset_dev_params(&mmap_buf->dev_params);
	if (!get_behavior()) {
		for (i = DEV_PARAM_MIN; i < DEV_PARAM_MAX; i++) {
			if (get_param_bit(dev_field_mask, i))
				mpsd_get_dev_params(i, &mmap_buf->dev_params);
		}
	} else
		get_dev_params_snapshot(&mmap_buf->dev_params);

	mmap_buf->app_field_mask = app_field_mask;
	memset_app_params(&mmap_buf->app_params);
	if (!is_task_valid(pid))
		pid = DEFAULT_PID;

	if (pid != DEFAULT_PID) {
		if (!get_behavior()) {
			set_param_bit(&app_field_mask, APP_PARAM_NAME);
			set_param_bit(&app_field_mask, APP_PARAM_TGID);

			for (i = APP_PARAM_MIN; i < APP_PARAM_MAX; i++) {
				if (get_param_bit(app_field_mask, i)) {
					mpsd_get_app_params(pid, i,
						&mmap_buf->app_params);
				}
			}
		} else {
			get_app_params_snapshot(pid, &mmap_buf->app_params);
		}
	}

	if (req != SYNC_READ)
		mpsd_notify_umd();

#ifdef CONFIG_MPSD_DEBUG
	if (get_debug_level_read() <= get_mpsd_debug_level()) {
		pr_info("%s: %s: Config[%d] Pid[%d] Masks[%llu, %llu] ReqID[%d] E[%d] <-\n",
			tag, __func__, get_behavior(),
			pid, mmap_buf->app_field_mask,
			mmap_buf->dev_field_mask, (int)req, event);
	}

	if (get_debug_level_read() < get_mpsd_debug_level())
		print_mmap_buffer();
#endif

	mmap_buf->updating = 0;
	mutex_unlock(&mpsd_read_lock);

	return ret;
}

static void mpsd_cpuload_work_fn(struct work_struct *work)
{
	int i = 0, cpu = 0, idx = 0;

	for_each_possible_cpu(cpu) {
		if (cpuload_history_idx >=	MAX_LOAD_HISTORY) {
			idx = MAX_LOAD_HISTORY - 1;
			for (i = 0; i < MAX_LOAD_HISTORY - 1; i++)
				cpuload_history[cpu][i] =
					cpuload_history[cpu][i + 1];
		} else {
			idx = cpuload_history_idx;
			cpuload_history_idx++;
		}
		cpuload_history[cpu][idx] = (long long)(((cpu_util(cpu) * 100) /
			capacity_orig_of(cpu)));
	}

#ifdef CONFIG_MPSD_DEBUG
	if (get_mpsd_debug_level() > DEBUG_LEVEL_MIN) {
		pr_info("%s: %s: Added in queue => ", tag, __func__);
		for_each_possible_cpu(cpu)
			pr_info("%lld ", cpuload_history[cpu][idx]);

	}
#endif

	queue_delayed_work(cpuload_wq, &cpuload_work,
		CPULOAD_CHECK_DELAY);
}

void mpsd_read_start_cpuload_work(void)
{
	pr_info("%s: %s:\n", tag, __func__);
	queue_delayed_work(cpuload_wq, &cpuload_work,
		CPULOAD_CHECK_DELAY);
	pr_info("%s: %s: Done!\n", tag, __func__);
}

void mpsd_read_stop_cpuload_work(void)
{
	pr_info("%s: %s:\n", tag, __func__);
	cancel_delayed_work_sync(&cpuload_work);
	pr_info("%s: %s: Done!\n", tag, __func__);
}

void mpsd_read_init(void)
{
	int i = 0, cpu = 0;

	pr_info("%s: %s:\n", tag, __func__);
	set_cpufreq_policy_map();


	cpuload_wq = alloc_ordered_workqueue("mpsd_cpu_load_history",
		WQ_FREEZABLE | WQ_HIGHPRI);

	BUG_ON(!cpuload_wq);
	INIT_DEFERRABLE_WORK(&cpuload_work, mpsd_cpuload_work_fn);

	for_each_possible_cpu(cpu) {
		for (i = 0; i < MAX_LOAD_HISTORY; i++)
			cpuload_history[cpu][i] = DEFAULT_PARAM_VAL_INT;
	}

	mutex_init(&mpsd_read_lock);
	pr_info("%s: %s: Done!\n", tag, __func__);
}

void mpsd_read_exit(void)
{
	pr_info("%s: %s:\n", tag, __func__);
	destroy_workqueue(cpuload_wq);
	mutex_destroy(&mpsd_read_lock);
	pr_info("%s: %s: Done!\n", tag, __func__);
}

