/*
 * Copyright (c) 2016 Park Bumgyu, Samsung Electronics Co., Ltd <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Exynos ACME(A Cpufreq that Meets Every chipset) driver implementation
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/pm_qos.h>
#include <linux/exynos-ss.h>
#include <linux/pm_opp.h>

#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-dm.h>
#include <soc/samsung/ect_parser.h>
#include <soc/samsung/exynos-earlytmu.h>

#include "exynos-acme.h"

/*
 * list head of cpufreq domain
 */
LIST_HEAD(domains);

#ifdef CONFIG_SW_SELF_DISCHARGING
/* Support SW_SELF_DISCHARGING (SEC_PM) */
static int self_discharging;
#endif

/*********************************************************************
 *                          HELPER FUNCTION                          *
 *********************************************************************/
static struct exynos_cpufreq_domain *find_domain(unsigned int cpu)
{
	struct exynos_cpufreq_domain *domain;

	list_for_each_entry(domain, &domains, list)
		if (cpumask_test_cpu(cpu, &domain->cpus))
			return domain;

	pr_err("cannot find cpufreq domain by cpu\n");
	return NULL;
}

static
struct exynos_cpufreq_domain *find_domain_pm_qos_class(int pm_qos_class)
{
	struct exynos_cpufreq_domain *domain;

	list_for_each_entry(domain, &domains, list)
		if (domain->pm_qos_min_class == pm_qos_class ||
			domain->pm_qos_max_class == pm_qos_class)
			return domain;

	pr_err("cannot find cpufreq domain by PM QoS class\n");
	return NULL;
}

static struct
exynos_cpufreq_domain *find_domain_cpumask(const struct cpumask *mask)
{
	struct exynos_cpufreq_domain *domain;

	list_for_each_entry(domain, &domains, list)
		if (cpumask_subset(mask, &domain->cpus))
			return domain;

	pr_err("cannot find cpufreq domain by cpumask\n");
	return NULL;
}

static
struct exynos_cpufreq_domain *find_domain_dm_type(enum exynos_dm_type dm_type)
{
	struct exynos_cpufreq_domain *domain;

	list_for_each_entry(domain, &domains, list)
		if (domain->dm_type == dm_type)
			return domain;

	pr_err("cannot find cpufreq domain by DVFS Manager type\n");
	return NULL;
}

static struct exynos_cpufreq_domain* first_domain(void)
{
	return list_first_entry(&domains,
			struct exynos_cpufreq_domain, list);
}

static struct exynos_cpufreq_domain* last_domain(void)
{
	return list_last_entry(&domains,
			struct exynos_cpufreq_domain, list);
}

static void enable_domain(struct exynos_cpufreq_domain *domain)
{
	mutex_lock(&domain->lock);
	domain->enabled = true;
	mutex_unlock(&domain->lock);
}

static void disable_domain(struct exynos_cpufreq_domain *domain)
{
	mutex_lock(&domain->lock);
	domain->enabled = false;
	mutex_unlock(&domain->lock);
}

static bool static_governor(struct cpufreq_policy *policy)
{
	/*
	 * During cpu hotplug in sequence, governor can be empty for
	 * a while. In this case, we regard governor as default
	 * governor. Exynos never use static governor as default.
	 */
	if (!policy->governor)
		return false;

	if ((strcmp(policy->governor->name, "userspace") == 0)
		|| (strcmp(policy->governor->name, "performance") == 0)
		|| (strcmp(policy->governor->name, "powersave") == 0))
		return true;

	return false;
}

static int index_to_freq(struct cpufreq_frequency_table *table,
					unsigned int index)
{
	return table[index].frequency;
}

/*********************************************************************
 *                         FREQUENCY SCALING                         *
 *********************************************************************/
static unsigned int get_freq(struct exynos_cpufreq_domain *domain)
{
	unsigned int freq = (unsigned int)cal_dfs_get_rate(domain->cal_id);

	/* On changing state, CAL returns 0 */
	if (!freq)
		return domain->old;

	return freq;
}

static int set_freq(struct exynos_cpufreq_domain *domain,
					unsigned int target_freq)
{
	int err;

	err = cal_dfs_set_rate(domain->cal_id, target_freq);
	if (err < 0)
		pr_err("failed to scale frequency of domain%d (%d -> %d)\n",
			domain->id, domain->old, target_freq);

	return err;
}

static unsigned int apply_pm_qos(struct exynos_cpufreq_domain *domain,
					struct cpufreq_policy *policy,
					unsigned int target_freq)
{
	unsigned int freq;
	int qos_min, qos_max;

	/*
	 * In case of static governor, it should garantee to scale to
	 * target, it does not apply PM QoS.
	 */
	if (static_governor(policy))
		return target_freq;

	qos_min = pm_qos_request(domain->pm_qos_min_class);
	qos_max = pm_qos_request(domain->pm_qos_max_class);

	if (qos_min < 0 || qos_max < 0)
		return target_freq;

	freq = max((unsigned int)qos_min, target_freq);
	freq = min((unsigned int)qos_max, freq);

	return freq;
}

static int pre_scale(void)
{
	return 0;
}

static int post_scale(void)
{
	return 0;
}

static int scale(struct exynos_cpufreq_domain *domain,
				struct cpufreq_policy *policy,
				unsigned int target_freq)
{
	int ret;
	struct cpufreq_freqs freqs = {
		.cpu		= policy->cpu,
		.old		= domain->old,
		.new		= target_freq,
		.flags		= 0,
	};

	cpufreq_freq_transition_begin(policy, &freqs);
	exynos_ss_freq(domain->id, domain->old, target_freq, ESS_FLAG_IN);

	ret = pre_scale();
	if (ret)
		goto fail_scale;

	/* Scale frequency by hooked function, set_freq() */
	ret = set_freq(domain, target_freq);
	if (ret)
		goto fail_scale;

	ret = post_scale();
	if (ret)
		goto fail_scale;

fail_scale:
	/* In scaling failure case, logs -1 to exynos snapshot */
	exynos_ss_freq(domain->id, domain->old, target_freq,
					ret < 0 ? ret : ESS_FLAG_OUT);
	cpufreq_freq_transition_end(policy, &freqs, ret);

	return ret;
}

static int update_freq(struct exynos_cpufreq_domain *domain,
					 unsigned int freq)
{
	struct cpufreq_policy *policy;
	int ret;

	pr_debug("update frequency of domain%d to %d kHz\n",
						domain->id, freq);

	policy = cpufreq_cpu_get(cpumask_first(&domain->cpus));
	if (!policy)
		return -EINVAL;

	if (static_governor(policy)) {
		cpufreq_cpu_put(policy);
		return 0;
	}

	ret = __cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_H);
	cpufreq_cpu_put(policy);

	return ret;
}

/*********************************************************************
 *                   EXYNOS CPUFREQ DRIVER INTERFACE                 *
 *********************************************************************/
static int exynos_cpufreq_driver_init(struct cpufreq_policy *policy)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);
	int ret;

	if (!domain)
		return -EINVAL;

	ret = cpufreq_table_validate_and_show(policy, domain->freq_table);
	if (ret) {
		pr_err("%s: invalid frequency table: %d\n", __func__, ret);
		return ret;
	}

	policy->cur = get_freq(domain);
	policy->cpuinfo.transition_latency = TRANSITION_LATENCY;
	cpumask_copy(policy->cpus, &domain->cpus);

	pr_info("CPUFREQ domain%d registered\n", domain->id);

	return 0;
}

static int exynos_cpufreq_verify(struct cpufreq_policy *policy)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);

	if (!domain)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, domain->freq_table);
}

static int __exynos_cpufreq_target(struct cpufreq_policy *policy,
				  unsigned int target_freq,
				  unsigned int relation)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);
	unsigned int index;
	int ret = 0;

	if (!domain)
		return -EINVAL;

	mutex_lock(&domain->lock);

	if (!domain->enabled)
		goto out;

	if (domain->old != get_freq(domain)) {
		pr_err("oops, inconsistency between domain->old:%d, real clk:%d\n",
			domain->old, get_freq(domain));
		BUG_ON(1);
	}

	/*
	 * Update target_freq.
	 * Updated target_freq is in between minimum and maximum PM QoS/policy,
	 * priority of policy is higher.
	 */
	target_freq = apply_pm_qos(domain, policy, target_freq);
	ret = cpufreq_frequency_table_target(policy, domain->freq_table,
					target_freq, relation, &index);
	if (ret) {
		pr_err("target frequency(%d) out of range\n", target_freq);
		goto out;
	}

	target_freq = index_to_freq(domain->freq_table, index);

	/* Target is same as current, skip scaling */
	if (domain->old == target_freq)
		goto out;

	ret = scale(domain, policy, target_freq);
	if (ret)
		goto out;

	pr_debug("CPUFREQ domain%d frequency change %u kHz -> %u kHz\n",
			domain->id, domain->old, target_freq);

	domain->old = target_freq;

out:
	mutex_unlock(&domain->lock);

	return ret;
}

static int exynos_cpufreq_target(struct cpufreq_policy *policy,
					unsigned int target_freq,
					unsigned int relation)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);
	unsigned long freq = (unsigned long)target_freq;

	if (!domain)
		return -EINVAL;

	if (list_empty(&domain->dm_list))
		return __exynos_cpufreq_target(policy, target_freq, relation);

	return DM_CALL(domain->dm_type, &freq);
}

static unsigned int exynos_cpufreq_get(unsigned int cpu)
{
	struct exynos_cpufreq_domain *domain = find_domain(cpu);

	if (!domain)
		return 0;

	return get_freq(domain);
}

static int exynos_cpufreq_suspend(struct cpufreq_policy *policy)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);
	unsigned int freq;

	if (!domain)
		return -EINVAL;

	/* To handle reboot faster, it does not thrrotle frequency of domain0 */
	if (system_state == SYSTEM_RESTART && domain->id != 0)
		freq = domain->min_freq;
	else
		freq = domain->resume_freq;

	pm_qos_update_request(&domain->min_qos_req, freq);
	pm_qos_update_request(&domain->max_qos_req, freq);

	/* To guarantee applying frequency, update_freq() is called explicitly */
	update_freq(domain, freq);

	/*
	 * Although cpufreq governor is stopped in cpufreq_suspend(),
	 * afterwards, frequency change can be requested by
	 * PM QoS. To prevent chainging frequency after
	 * cpufreq suspend, disable scaling for all domains.
	 */
	disable_domain(domain);

	return 0;
}

static int exynos_cpufreq_resume(struct cpufreq_policy *policy)
{
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);

	if (!domain)
		return -EINVAL;

	enable_domain(domain);

	pm_qos_update_request(&domain->min_qos_req, domain->min_freq);
	pm_qos_update_request(&domain->max_qos_req, domain->max_freq);

	return 0;
}

static struct cpufreq_driver exynos_driver = {
	.name		= "exynos_cpufreq",
	.flags		= CPUFREQ_STICKY | CPUFREQ_HAVE_GOVERNOR_PER_POLICY,
	.init		= exynos_cpufreq_driver_init,
	.verify		= exynos_cpufreq_verify,
	.target		= exynos_cpufreq_target,
	.get		= exynos_cpufreq_get,
	.suspend	= exynos_cpufreq_suspend,
	.resume		= exynos_cpufreq_resume,
};

/*********************************************************************
 *                      SUPPORT for DVFS MANAGER                     *
 *********************************************************************/
static void update_dm_constraint(struct exynos_cpufreq_domain *domain,
					struct cpufreq_policy *new_policy)
{
	struct cpufreq_policy *policy;
	unsigned int policy_min, policy_max;
	unsigned int pm_qos_min, pm_qos_max;

	if (new_policy) {
		policy_min = new_policy->min;
		policy_max = new_policy->max;
	} else {
		policy = cpufreq_cpu_get(cpumask_first(&domain->cpus));
		if (!policy)
			return;

		policy_min = policy->min;
		policy_max = policy->max;
		cpufreq_cpu_put(policy);
	}

	pm_qos_min = pm_qos_request(domain->pm_qos_min_class);
	pm_qos_max = pm_qos_request(domain->pm_qos_max_class);

	policy_update_call_to_DM(domain->dm_type, max(policy_min, pm_qos_min),
						  min(policy_max, pm_qos_max));
}

static int dm_scaler(enum exynos_dm_type dm_type, unsigned int target_freq,
						unsigned int relation)
{
	struct exynos_cpufreq_domain *domain = find_domain_dm_type(dm_type);
	struct cpufreq_policy *policy;
	struct cpumask mask;
	int ret;

	/* Skip scaling if all cpus of domain are hotplugged out */
	cpumask_and(&mask, &domain->cpus, cpu_online_mask);
	if (cpumask_weight(&mask) == 0)
		return 0;

	if (relation == EXYNOS_DM_RELATION_L)
		relation = CPUFREQ_RELATION_L;
	else
		relation = CPUFREQ_RELATION_H;

	policy = cpufreq_cpu_get(cpumask_first(&domain->cpus));
	if (!policy) {
		pr_err("%s: failed get cpufreq policy\n", __func__);
		return -ENODEV;
	}

	ret = __exynos_cpufreq_target(policy, target_freq, relation);

	cpufreq_cpu_put(policy);

	return ret;
}

/*********************************************************************
 *                       CPUFREQ PM QOS HANDLER                      *
 *********************************************************************/
static int need_update_freq(struct exynos_cpufreq_domain *domain,
				int pm_qos_class, unsigned int freq)
{
	unsigned int cur = get_freq(domain);

	if (cur == freq)
		return 0;

	if (pm_qos_class == domain->pm_qos_min_class) {
		if (cur > freq)
			return 0;
	} else if (domain->pm_qos_max_class == pm_qos_class) {
		if (cur < freq)
			return 0;
	} else {
		/* invalid PM QoS class */
		return -EINVAL;
	}

	return 1;
}

static int exynos_cpufreq_pm_qos_callback(struct notifier_block *nb,
					unsigned long val, void *v)
{
	int pm_qos_class = *((int *)v);
	struct exynos_cpufreq_domain *domain;
	int ret;

	pr_debug("update PM QoS class %d to %ld kHz\n", pm_qos_class, val);

	domain = find_domain_pm_qos_class(pm_qos_class);
	if (!domain)
		return NOTIFY_BAD;

	update_dm_constraint(domain, NULL);

	ret = need_update_freq(domain, pm_qos_class, val);
	if (ret < 0)
		return NOTIFY_BAD;
	if (!ret)
		return NOTIFY_OK;

	if (update_freq(domain, val))
		return NOTIFY_BAD;

	return NOTIFY_OK;
}

/*********************************************************************
 *                              FREQ BOOST                           *
 *********************************************************************/
static unsigned int get_boost_max_freq(struct exynos_cpufreq_domain *domain)
{
	int num_online = cpumask_weight(&domain->online_cpus);

	/* all cpus are down, it returns lowest max frequency */
	if (!num_online) {
		int total_cpus = cpumask_weight(&domain->cpus);
		return domain->boost_max_freqs[total_cpus - 1];
	}

	return domain->boost_max_freqs[num_online - 1];
}

static void update_boost_max_freq(struct exynos_cpufreq_domain *domain,
					struct cpufreq_policy *policy)
{
	unsigned int max_freq;

	if (!domain->boost_supported)
		return;

	max_freq = get_boost_max_freq(domain);
	if (policy->max != max_freq)
		cpufreq_verify_within_limits(policy, 0, max_freq);
}

static int exynos_cpufreq_cpus_callback(struct notifier_block *nb,
				    unsigned long event, void *data)
{
	struct cpumask *mask = (struct cpumask *)data;
	struct exynos_cpufreq_domain *domain;
	char buf[10];
	unsigned long timeout;

	list_for_each_entry(domain, &domains, list) {
		if (!domain->boost_supported)
			continue;

		switch (event) {
		case CPUS_DOWN_COMPLETE:
		case CPUS_UP_PREPARE:
			/*
			 * The first incoming cpu of domain never call cpufreq_update_policy()
			 * because policy is not available.
			 */
			cpumask_and(&domain->online_cpus, cpu_online_mask, mask);
			cpumask_and(&domain->online_cpus, &domain->online_cpus, &domain->cpus);
			if (!cpumask_weight(&domain->online_cpus))
				return NOTIFY_OK;

			/* If it fail to update policy over 50ms, it cancels cpu up */
			timeout = jiffies + msecs_to_jiffies(50);
			while (cpufreq_update_policy(cpumask_first(&domain->online_cpus))) {
				if (event == CPUS_DOWN_COMPLETE)
					break;

				if (time_after(jiffies, timeout))
					return NOTIFY_BAD;
			}
		}

		if (event == CPUS_UP_PREPARE) {
			mutex_lock(&domain->lock);
			if (unlikely(domain->old > get_boost_max_freq(domain))) {
				scnprintf(buf, sizeof(buf), "%*pbl", cpumask_pr_args(mask));
				pr_err("Overspeed frequency, %dkHz. Cancel hotplug in CPU%s.\n",
						domain->old, buf);
				mutex_unlock(&domain->lock);
				return NOTIFY_BAD;
			}
			mutex_unlock(&domain->lock);
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_cpufreq_cpus_notifier = {
	.notifier_call = exynos_cpufreq_cpus_callback,
	.priority = INT_MIN,
};

/*********************************************************************
 *                       EXTERNAL EVENT HANDLER                      *
 *********************************************************************/
static int exynos_cpufreq_policy_callback(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	struct exynos_cpufreq_domain *domain = find_domain(policy->cpu);

	if (!domain)
		return NOTIFY_OK;

	switch (event) {
	case CPUFREQ_ADJUST:
		update_boost_max_freq(domain, policy);
		break;
	case CPUFREQ_NOTIFY:
		update_dm_constraint(domain, policy);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_cpufreq_policy_notifier = {
	.notifier_call = exynos_cpufreq_policy_callback,
};

static int exynos_cpufreq_cpu_up_callback(struct notifier_block *notifier,
					unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	struct exynos_cpufreq_domain *domain;
	struct cpumask mask;

	/*
	 * enable_nonboot_cpus() sends CPU_ONLINE_FROZEN notify event,
	 * but cpu_up() sends just CPU_ONLINE. We don't want that
	 * this callback does nothing before cpufreq_resume().
	 */
	if (action != CPU_ONLINE && action != CPU_DOWN_FAILED)
		return NOTIFY_OK;

	domain = find_domain(cpu);
	if (!domain)
		return NOTIFY_BAD;

	/*
	 * The first incomming cpu in domain enables frequency scaling
	 * and clears limit of frequency.
	 */
	cpumask_and(&mask, &domain->cpus, cpu_online_mask);
	if (cpumask_weight(&mask) == 1) {
		enable_domain(domain);
		pm_qos_update_request(&domain->max_qos_req, domain->max_freq);
	}

	return NOTIFY_OK;
}

static struct notifier_block __refdata exynos_cpufreq_cpu_up_notifier = {
	.notifier_call = exynos_cpufreq_cpu_up_callback,
	.priority = INT_MIN,
};

static int exynos_cpufreq_cpu_down_callback(struct notifier_block *notifier,
					unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	struct exynos_cpufreq_domain *domain;
	struct cpumask mask;

	/*
	 * disable_nonboot_cpus() sends CPU_DOWN_PREPARE_FROZEN notify
	 * event, but cpu_down() sends just CPU_DOWN_PREPARE. We don't
	 * want that this callback does nothing after cpufreq_suspend().
	 */
	if (action != CPU_DOWN_PREPARE)
		return NOTIFY_OK;

	domain = find_domain(cpu);
	if (!domain)
		return NOTIFY_BAD;

	/*
	 * The last outgoing cpu in domain limits frequency to minimum
	 * and disables frequency scaling.
	 */
	cpumask_and(&mask, &domain->cpus, cpu_online_mask);
	if (cpumask_weight(&mask) == 1) {
		pm_qos_update_request(&domain->max_qos_req, domain->min_freq);
		disable_domain(domain);
	}

	return NOTIFY_OK;
}

static struct notifier_block __refdata exynos_cpufreq_cpu_down_notifier = {
	.notifier_call = exynos_cpufreq_cpu_down_callback,
	/*
	 * This notifier should be perform before cpufreq_cpu_notifier.
	 */
	.priority = INT_MIN + 2,
};

/*********************************************************************
 *                       EXTERNAL REFERENCE APIs                     *
 *********************************************************************/
unsigned int exynos_cpufreq_get_max_freq(struct cpumask *mask)
{
	struct exynos_cpufreq_domain *domain = find_domain_cpumask(mask);

	return domain->max_freq;
}
EXPORT_SYMBOL(exynos_cpufreq_get_max_freq);

#ifdef CONFIG_SEC_BOOTSTAT
void sec_bootstat_get_cpuinfo(int *freq, int *online)
{
	int cpu;
	int cluster;
	struct exynos_cpufreq_domain *domain;
							  
	get_online_cpus();
	*online = cpumask_bits(cpu_online_mask)[0];
	for_each_online_cpu(cpu) {
		domain = find_domain(cpu);
		if (!domain)
			continue;

		cluster = 0;
		if (domain->dm_type == DM_CPU_CL1)
			cluster = 1;

		freq[cluster] = get_freq(domain);
	}
	put_online_cpus();
}
#endif

void exynos_cpufreq_reset_boot_qos(void)
{
	struct exynos_cpufreq_domain *domain;

	list_for_each_entry(domain, &domains, list) {
		if (!domain->boot_qos)
			continue;

		pm_qos_update_request_timeout(&domain->min_qos_req,
				domain->boot_qos, 40 * USEC_PER_SEC);
		pm_qos_update_request_timeout(&domain->max_qos_req,
				domain->boot_qos, 40 * USEC_PER_SEC);
	}
}
EXPORT_SYMBOL(exynos_cpufreq_reset_boot_qos);

/*********************************************************************
 *                          SYSFS INTERFACES                         *
 *********************************************************************/
/*
 * Log2 of the number of scale size. The frequencies are scaled up or
 * down as the multiple of this number.
 */
#define SCALE_SIZE	2

static ssize_t show_cpufreq_table(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	struct exynos_cpufreq_domain *domain;
	ssize_t count = 0;
	int i, scale = 0;

	list_for_each_entry_reverse(domain, &domains, list) {
		for (i = 0; i < domain->table_size; i++) {
			unsigned int freq = domain->freq_table[i].frequency;
			if (freq == CPUFREQ_ENTRY_INVALID)
				continue;

			count += snprintf(&buf[count], 10, "%d ",
					freq >> (scale * SCALE_SIZE));
		}

		scale++;
	}

        count += snprintf(&buf[count - 1], 2, "\n");

	return count - 1;
}

static ssize_t show_cpufreq_min_limit(struct kobject *kobj,
                                struct attribute *attr, char *buf)
{
	struct exynos_cpufreq_domain *domain;
	unsigned int pm_qos_min;
	int scale = -1;

	list_for_each_entry_reverse(domain, &domains, list) {
		scale++;

#ifdef CONFIG_SCHED_HMP
		/*
		 * In HMP architecture, last domain is big.
		 * If HMP boost is not activated, PM QoS value of
		 * big is not shown.
		 */
		if (domain == last_domain() && !get_hmp_boost())
			continue;
#endif

		/* get value of minimum PM QoS */
		pm_qos_min = pm_qos_request(domain->pm_qos_min_class);
		if (pm_qos_min > 0) {
			pm_qos_min = min(pm_qos_min, domain->max_freq);
			pm_qos_min = max(pm_qos_min, domain->min_freq);

			/*
			 * To manage frequencies of all domains at once,
			 * scale down frequency as multiple of 4.
			 * ex) domain2 = freq
			 *     domain1 = freq /4
			 *     domain0 = freq /16
			 */
			pm_qos_min = pm_qos_min >> (scale * SCALE_SIZE);
			return snprintf(buf, 10, "%u\n", pm_qos_min);
		}
	}

	/*
	 * If there is no QoS at all domains, it returns minimum
	 * frequency of last domain
	 */
	return snprintf(buf, 10, "%u\n",
		first_domain()->min_freq >> (scale * SCALE_SIZE));
}

#ifdef CONFIG_SCHED_HMP
static bool hmp_boost;
static void control_hmp_boost(bool enable)
{
	if (hmp_boost && !enable) {
		set_hmp_boost(0);
		hmp_boost = false;
	} else if (!hmp_boost && enable) {
		set_hmp_boost(1);
		hmp_boost = true;
	}
}
#else
static inline void control_hmp_boost(bool enable) {}
#endif

static ssize_t store_cpufreq_min_limit(struct kobject *kobj,
				struct attribute *attr, const char *buf,
				size_t count)
{
	struct exynos_cpufreq_domain *domain;
	int input, scale = -1;
	unsigned int freq;
	bool set_max = false;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	list_for_each_entry_reverse(domain, &domains, list) {
		scale++;

		if (set_max) {
			unsigned int qos = domain->max_freq;

			if (domain->user_default_qos)
				qos = domain->user_default_qos;

			pm_qos_update_request(&domain->user_min_qos_req, qos);
			continue;
		}

		/* Clear all constraint by cpufreq_min_limit */
		if (input < 0) {
			pm_qos_update_request(&domain->user_min_qos_req, 0);
			control_hmp_boost(false);
			continue;
		}

		/*
		 * User inputs scaled down frequency. To recover real
		 * frequency, scale up frequency as multiple of 4.
		 * ex) domain2 = freq
		 *     domain1 = freq * 4
		 *     domain0 = freq * 16
		 */
		freq = input << (scale * SCALE_SIZE);

		if (freq < domain->min_freq) {
			pm_qos_update_request(&domain->user_min_qos_req, 0);
			continue;
		}

		/*
		 * In HMP, last domain is big. Input frequency is in range
		 * of big, it enables HMP boost.
		 */
		if (domain == last_domain())
			control_hmp_boost(true);
		else
			control_hmp_boost(false);

		freq = min(freq, domain->max_freq);
		pm_qos_update_request(&domain->user_min_qos_req, freq);

		set_max = true;
	}

	return count;
}

static ssize_t store_cpufreq_min_limit_wo_boost(struct kobject *kobj,
				struct attribute *attr, const char *buf,
				size_t count)
{
	struct exynos_cpufreq_domain *domain;
	int input, scale = -1;
	unsigned int freq;
	bool set_max = false;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	list_for_each_entry_reverse(domain, &domains, list) {
		scale++;

		if (set_max) {
			unsigned int qos = domain->max_freq;

			if (domain->user_default_qos)
				qos = domain->user_default_qos;

			pm_qos_update_request(&domain->user_min_qos_wo_boost_req, qos);
			continue;
		}

		/* Clear all constraint by cpufreq_min_limit */
		if (input < 0) {
			pm_qos_update_request(&domain->user_min_qos_wo_boost_req, 0);
			continue;
		}

		/*
		 * User inputs scaled down frequency. To recover real
		 * frequency, scale up frequency as multiple of 4.
		 * ex) domain2 = freq
		 *     domain1 = freq * 4
		 *     domain0 = freq * 16
		 */
		freq = input << (scale * SCALE_SIZE);

		if (freq < domain->min_freq) {
			pm_qos_update_request(&domain->user_min_qos_wo_boost_req, 0);
			continue;
		}

#ifdef CONFIG_SCHED_HMP
		/*
		 * If hmp_boost was already activated by cpufreq_min_limit,
		 * print a message to avoid confusing who activated hmp_boost.
		 */
		if (domain == last_domain() && hmp_boost)
			pr_info("HMP boost was already activated by cpufreq_min_limit node");
#endif

		freq = min(freq, domain->max_freq);
		pm_qos_update_request(&domain->user_min_qos_wo_boost_req, freq);

		set_max = true;
	}

	return count;

}

static ssize_t show_cpufreq_max_limit(struct kobject *kobj,
                                struct attribute *attr, char *buf)
{
	struct exynos_cpufreq_domain *domain;
	unsigned int pm_qos_max;
	int scale = -1;

	list_for_each_entry_reverse(domain, &domains, list) {
		scale++;

		/* get value of minimum PM QoS */
		pm_qos_max = pm_qos_request(domain->pm_qos_max_class);
		if (pm_qos_max > 0) {
			pm_qos_max = min(pm_qos_max, domain->max_freq);
			pm_qos_max = max(pm_qos_max, domain->min_freq);

			/*
			 * To manage frequencies of all domains at once,
			 * scale down frequency as multiple of 4.
			 * ex) domain2 = freq
			 *     domain1 = freq /4
			 *     domain0 = freq /16
			 */
			pm_qos_max = pm_qos_max >> (scale * SCALE_SIZE);
			return snprintf(buf, 10, "%u\n", pm_qos_max);
		}
	}

	/*
	 * If there is no QoS at all domains, it returns minimum
	 * frequency of last domain
	 */
	return snprintf(buf, 10, "%u\n",
		first_domain()->min_freq >> (scale * SCALE_SIZE));
}

struct pm_qos_request cpu_online_max_qos_req;
static void enable_domain_cpus(struct exynos_cpufreq_domain *domain)
{
	struct cpumask mask;

	if (domain == first_domain())
		return;

	cpumask_or(&mask, cpu_online_mask, &domain->cpus);
	pm_qos_update_request(&cpu_online_max_qos_req, cpumask_weight(&mask));
}

static void disable_domain_cpus(struct exynos_cpufreq_domain *domain)
{
	struct cpumask mask;

	if (domain == first_domain())
		return;

	cpumask_andnot(&mask, cpu_online_mask, &domain->cpus);
	pm_qos_update_request(&cpu_online_max_qos_req, cpumask_weight(&mask));
}

static ssize_t store_cpufreq_max_limit(struct kobject *kobj, struct attribute *attr,
                                        const char *buf, size_t count)
{
	struct exynos_cpufreq_domain *domain;
	int input, scale = -1;
	unsigned int freq;
	bool set_max = false;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	list_for_each_entry_reverse(domain, &domains, list) {
		scale++;

		if (set_max) {
			pm_qos_update_request(&domain->user_max_qos_req,
					domain->max_freq);
			continue;
		}

		/* Clear all constraint by cpufreq_max_limit */
		if (input < 0) {
			enable_domain_cpus(domain);
			pm_qos_update_request(&domain->user_max_qos_req,
						domain->max_freq);
			continue;
		}

		/*
		 * User inputs scaled down frequency. To recover real
		 * frequency, scale up frequency as multiple of 4.
		 * ex) domain2 = freq
		 *     domain1 = freq * 4
		 *     domain0 = freq * 16
		 */
		freq = input << (scale * SCALE_SIZE);
		if (freq < domain->min_freq) {
			pm_qos_update_request(&domain->user_max_qos_req, 0);
			disable_domain_cpus(domain);
			continue;
		}

		enable_domain_cpus(domain);

		freq = max(freq, domain->min_freq);
		pm_qos_update_request(&domain->user_max_qos_req, freq);

		set_max = true;
	}

	return count;
}

#ifdef CONFIG_SW_SELF_DISCHARGING
static ssize_t show_cpufreq_self_discharging(struct kobject *kobj,
			     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", self_discharging);
}

static ssize_t store_cpufreq_self_discharging(struct kobject *kobj, struct attribute *attr,
			      const char *buf, size_t count)
{
	int input;

	if (!sscanf(buf, "%d", &input))
		return -EINVAL;

	if (input > 0) {
		self_discharging = input;
		cpu_idle_poll_ctrl(true);
	}
	else {
		self_discharging = 0;
		cpu_idle_poll_ctrl(false);
	}

	return count;
}
#endif

static struct global_attr cpufreq_table =
__ATTR(cpufreq_table, S_IRUGO, show_cpufreq_table, NULL);
static struct global_attr cpufreq_min_limit =
__ATTR(cpufreq_min_limit, S_IRUGO | S_IWUSR,
		show_cpufreq_min_limit, store_cpufreq_min_limit);
static struct global_attr cpufreq_min_limit_wo_boost =
__ATTR(cpufreq_min_limit_wo_boost, S_IRUGO | S_IWUSR,
		show_cpufreq_min_limit, store_cpufreq_min_limit_wo_boost);
static struct global_attr cpufreq_max_limit =
__ATTR(cpufreq_max_limit, S_IRUGO | S_IWUSR,
		show_cpufreq_max_limit, store_cpufreq_max_limit);

#ifdef CONFIG_SW_SELF_DISCHARGING
static struct global_attr cpufreq_self_discharging =
		__ATTR(cpufreq_self_discharging, S_IRUGO | S_IWUSR,
			show_cpufreq_self_discharging, store_cpufreq_self_discharging);
#endif

/*********************************************************************
 *                  INITIALIZE EXYNOS CPUFREQ DRIVER                 *
 *********************************************************************/
static void print_domain_info(struct exynos_cpufreq_domain *domain)
{
	int i;
	char buf[10];

	pr_info("CPUFREQ of domain%d cal-id : %#x\n",
			domain->id, domain->cal_id);

	scnprintf(buf, sizeof(buf), "%*pbl", cpumask_pr_args(&domain->cpus));
	pr_info("CPUFREQ of domain%d sibling cpus : %s\n",
			domain->id, buf);

	pr_info("CPUFREQ of domain%d boot freq = %d kHz, resume freq = %d kHz\n",
			domain->id, domain->boot_freq, domain->resume_freq);

	pr_info("CPUFREQ of domain%d max freq : %d kHz, min freq : %d kHz\n",
			domain->id,
			domain->max_freq, domain->min_freq);

	pr_info("CPUFREQ of domain%d PM QoS max-class-id : %d, min-class-id : %d\n",
			domain->id,
			domain->pm_qos_max_class, domain->pm_qos_min_class);

	pr_info("CPUFREQ of domain%d table size = %d\n",
			domain->id, domain->table_size);

	for (i = 0; i < domain->table_size; i ++) {
		if (domain->freq_table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;

		pr_info("CPUFREQ of domain%d : L%2d  %7d kHz\n",
			domain->id,
			domain->freq_table[i].driver_data,
			domain->freq_table[i].frequency);
	}
}

static __init void init_sysfs(void)
{
	if (sysfs_create_file(power_kobj, &cpufreq_table.attr))
		pr_err("failed to create cpufreq_table node\n");

	if (sysfs_create_file(power_kobj, &cpufreq_min_limit.attr))
		pr_err("failed to create cpufreq_min_limit node\n");

	if (sysfs_create_file(power_kobj, &cpufreq_min_limit_wo_boost.attr))
		pr_err("failed to create cpufreq_min_limit_wo_boost node\n");

	if (sysfs_create_file(power_kobj, &cpufreq_max_limit.attr))
		pr_err("failed to create cpufreq_max_limit node\n");

#ifdef CONFIG_SW_SELF_DISCHARGING
	if (sysfs_create_file(power_kobj, &cpufreq_self_discharging.attr))
		pr_err("failed to create cpufreq_self_discharging node\n");
#endif
}

static __init int init_table(struct exynos_cpufreq_domain *domain)
{
	unsigned int index;
	unsigned long *table;
	unsigned int *volt_table;
	struct exynos_cpufreq_dm *dm;
	int ret = 0;

	/*
	 * Initialize frequency and voltage table of domain.
	 * Allocate temporary table to get DVFS table from CAL.
	 * Deliver this table to CAL API, then CAL fills the information.
	 */
	table = kzalloc(sizeof(unsigned long) * domain->table_size, GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	volt_table = kzalloc(sizeof(unsigned int) * domain->table_size, GFP_KERNEL);
	if (!volt_table) {
		ret = -ENOMEM;
		goto free_table;
	}

	cal_dfs_get_rate_table(domain->cal_id, table);
	cal_dfs_get_asv_table(domain->cal_id, volt_table);

	for (index = 0; index < domain->table_size; index++) {
		domain->freq_table[index].driver_data = index;

		if (table[index] > domain->max_freq)
			domain->freq_table[index].frequency = CPUFREQ_ENTRY_INVALID;
		else if (table[index] < domain->min_freq)
			domain->freq_table[index].frequency = CPUFREQ_ENTRY_INVALID;
		else {
			domain->freq_table[index].frequency = table[index];
			/* Add OPP table to first cpu of domain */
			dev_pm_opp_add(get_cpu_device(cpumask_first(&domain->cpus)),
					table[index] * 1000, volt_table[index]);
		}

		/* Initialize table of DVFS manager constraint */
		list_for_each_entry(dm, &domain->dm_list, list)
			dm->c.freq_table[index].master_freq = table[index];
	}
	domain->freq_table[index].driver_data = index;
	domain->freq_table[index].frequency = CPUFREQ_TABLE_END;

	kfree(volt_table);

free_table:
	kfree(table);

	return ret;
}

/* check jig detection by boot param */
#if defined(CONFIG_SEC_PM) && defined(CONFIG_SEC_FACTORY)
bool jig_attached = false;

static __init int get_jig_status(char *arg)
{
	jig_attached = true;	
	return 0;
}

early_param("jig", get_jig_status);
#endif

static __init void set_boot_qos(struct exynos_cpufreq_domain *domain,
					struct device_node *dn)
{
	unsigned int boot_qos, val;
	int freq;

	/*
	 * Basically booting pm_qos is set to max frequency of domain.
	 * But if pm_qos-booting exists in device tree,
	 * booting pm_qos is selected to smaller one
	 * between max frequency of domain and the value defined in device tree.
	 */
	boot_qos = domain->max_freq;
	if (!of_property_read_u32(dn, "pm_qos-booting", &val))
		boot_qos = min(boot_qos, val);

	/*
	 * Before setting booting pm_qos, ACME driver check thermal condition.
	 * If necessary, booting pm_qos should be set to frequency
	 * that considering thermal condition.
	 * exynos_earlytmu_get_boot_freq function return this frequency.
	 * 	1. return > 0	: booting pm_qos should be set lower than return value
	 *	2. return == 0	: booting pm_qos should be set to min_freq of domain
	 *	3. return < 0	: don't need to apply thermal condition
	 */
	freq = exynos_earlytmu_get_boot_freq(domain->id);
	if (freq >= 0) {
		/*
		 * Back up boot_qos value for reset booting pm_qos
		 * after thermal-throttling is enabled.
		 */
		domain->boot_qos = boot_qos;

		val = (unsigned int)freq;
		val = max(val, domain->min_freq);
		boot_qos = min(val, boot_qos);
	}
/*
 * Featuring VDD auto calibration code,
 * because VDD auto calibration will be used selectively depending on the project.
 */
#ifdef CONFIG_VDD_AUTO_CAL
	else {
		unsigned int auto_cal_qos;

		/*
		 * If thernmal condition is ok
		 * and auto calibration is defined in Device Tree,
		 * booting pm_qos should set to defined auto-cal-freq
		 * for defined auto-cal-duration.
		 */
		if (!of_property_read_u32(dn, "auto-cal-freq", &auto_cal_qos) &&
				!of_property_read_u32(dn, "auto-cal-duration", &val)) {
			/*
			 * auto-cal qos use user_qos_req,
			 * because user_qos_req isn't used in booting time.
			 */
			pm_qos_update_request_timeout(&domain->user_min_qos_req,
					auto_cal_qos, val * USEC_PER_MSEC);
			pm_qos_update_request_timeout(&domain->user_max_qos_req,
					auto_cal_qos, val * USEC_PER_MSEC);
		}

	}
#endif

	pm_qos_update_request_timeout(&domain->min_qos_req,
			boot_qos, 40 * USEC_PER_SEC);
	pm_qos_update_request_timeout(&domain->max_qos_req,
			boot_qos, 40 * USEC_PER_SEC);

	/* In case of factory mode, if jig cable is attached,
	 * set cpufreq max limit as frequency of "pm_qos-jigbooting" in device tree.
	 */
#if defined(CONFIG_SEC_PM) && defined(CONFIG_SEC_FACTORY)
	pr_info("%s:jig_attached = %d\n", __func__, jig_attached);

	if(jig_attached && !of_property_read_u32(dn, "pm_qos-jigbooting", &val)) {	
		pm_qos_update_request_timeout(&domain->max_qos_req,
				val, 100 * USEC_PER_SEC);
		
		pr_info("%s:Jig detected!. Set cpufreq max limit as %d for 100 secs(cpufreq-domain%d)\n", 
			__func__, val, domain->id);
	}
#endif
}

static __init int init_pm_qos(struct exynos_cpufreq_domain *domain,
					struct device_node *dn)
{
	int ret;

	ret = of_property_read_u32(dn, "pm_qos-min-class",
					&domain->pm_qos_min_class);
	if (ret)
		return ret;

	ret = of_property_read_u32(dn, "pm_qos-max-class",
					&domain->pm_qos_max_class);
	if (ret)
		return ret;

	domain->pm_qos_min_notifier.notifier_call = exynos_cpufreq_pm_qos_callback;
	domain->pm_qos_min_notifier.priority = INT_MAX;
	domain->pm_qos_max_notifier.notifier_call = exynos_cpufreq_pm_qos_callback;
	domain->pm_qos_max_notifier.priority = INT_MAX;

	pm_qos_add_notifier(domain->pm_qos_min_class,
				&domain->pm_qos_min_notifier);
	pm_qos_add_notifier(domain->pm_qos_max_class,
				&domain->pm_qos_max_notifier);

	pm_qos_add_request(&domain->min_qos_req,
			domain->pm_qos_min_class, domain->min_freq);
	pm_qos_add_request(&domain->max_qos_req,
			domain->pm_qos_max_class, domain->max_freq);
	pm_qos_add_request(&domain->user_min_qos_req,
			domain->pm_qos_min_class, domain->min_freq);
	pm_qos_add_request(&domain->user_max_qos_req,
			domain->pm_qos_max_class, domain->max_freq);
	pm_qos_add_request(&domain->user_min_qos_wo_boost_req,
			domain->pm_qos_min_class, domain->min_freq);

	set_boot_qos(domain, dn);

	return 0;
}

static int init_constraint_table_ect(struct exynos_cpufreq_domain *domain,
					struct exynos_cpufreq_dm *dm,
					struct device_node *dn)
{
	void *block;
	struct ect_minlock_domain *ect_domain;
	const char *ect_name;
	unsigned int index, c_index;
	bool valid_row = false;
	int ret;

	if (dm->c.constraint_dm_type != DM_MIF)
		return -EINVAL;

	ret = of_property_read_string(dn, "ect-name", &ect_name);
	if (ret)
		return ret;

	block = ect_get_block(BLOCK_MINLOCK);
	if (!block)
		return -ENODEV;

	ect_domain = ect_minlock_get_domain(block, (char *)ect_name);
	if (!ect_domain)
		return -ENODEV;

	for (index = 0; index < domain->table_size; index++) {
		unsigned int freq = domain->freq_table[index].frequency;

		for (c_index = 0; c_index < ect_domain->num_of_level; c_index++) {
			/* find row same as frequency */
			if (freq == ect_domain->level[c_index].main_frequencies) {
				dm->c.freq_table[index].constraint_freq
					= ect_domain->level[c_index].sub_frequencies;
				valid_row = true;
				break;
			}
		}

		/*
		 * Due to higher levels of constraint_freq should not be NULL,
		 * they should be filled with highest value of sub_frequencies of ect
		 * until finding first(highest) domain frequency fit with main_frequeucy of ect.
		 */
		if (!valid_row)
			dm->c.freq_table[index].constraint_freq
				= ect_domain->level[0].sub_frequencies;
	}

	return 0;
}

static int init_constraint_table_dt(struct exynos_cpufreq_domain *domain,
					struct exynos_cpufreq_dm *dm,
					struct device_node *dn)
{
	struct exynos_dm_freq *table;
	int size, index, c_index;

	/*
	 * A DVFS Manager table row consists of CPU and MIF frequency
	 * value, the size of a row is 64bytes. Divide size in half when
	 * table is allocated.
	 */
	size = of_property_count_u32_elems(dn, "table");
	if (size < 0)
		return size;

	table = kzalloc(sizeof(struct exynos_dm_freq) * size / 2, GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	of_property_read_u32_array(dn, "table", (unsigned int *)table, size);
	for (index = 0; index < domain->table_size; index++) {
		unsigned int freq = domain->freq_table[index].frequency;

		if (freq == CPUFREQ_ENTRY_INVALID)
			continue;

		for (c_index = 0; c_index < size / 2; c_index++) {
			/* find row same or nearby frequency */
			if (freq <= table[c_index].master_freq)
				dm->c.freq_table[index].constraint_freq
					= table[c_index].constraint_freq;

			if (freq >= table[c_index].master_freq)
				break;

		}
	}

	kfree(table);
	return 0;
}

static int init_dm(struct exynos_cpufreq_domain *domain,
				struct device_node *dn)
{
	struct device_node *root, *child;
	struct exynos_cpufreq_dm *dm;
	int ret;

	if (list_empty(&domain->dm_list))
		return 0;

	ret = of_property_read_u32(dn, "dm-type", &domain->dm_type);
	if (ret)
		return ret;

	ret = exynos_dm_data_init(domain->dm_type, domain->min_freq,
				domain->max_freq, domain->old);
	if (ret)
		return ret;

	dm = list_entry(&domain->dm_list, struct exynos_cpufreq_dm, list);
	root = of_find_node_by_name(dn, "dm-constraints");
	for_each_child_of_node(root, child) {
		/*
		 * Initialize DVFS Manaver constraints
		 * - constraint_type : minimum or maximum constraint
		 * - constraint_dm_type : cpu/mif/int/.. etc
		 * - guidance : constraint from chipset characteristic
		 * - freq_table : constraint table
		 */
		dm = list_next_entry(dm, list);

		of_property_read_u32(child, "const-type", &dm->c.constraint_type);
		of_property_read_u32(child, "dm-type", &dm->c.constraint_dm_type);

		if (of_property_read_bool(child, "guidance")) {
			dm->c.guidance = true;
			if (init_constraint_table_ect(domain, dm, child))
				continue;
		} else {
			if (init_constraint_table_dt(domain, dm, child))
				continue;
		}

		dm->c.table_length = domain->table_size;

		ret = register_exynos_dm_constraint_table(domain->dm_type, &dm->c);
		if (ret)
			return ret;
	}

	return register_exynos_dm_freq_scaler(domain->dm_type, dm_scaler);
}

static __init int init_domain(struct exynos_cpufreq_domain *domain,
					struct device_node *dn)
{
	unsigned int val;
	int ret;

	mutex_init(&domain->lock);

	/* Initialize frequency scaling */
	domain->max_freq = cal_dfs_get_max_freq(domain->cal_id);
	domain->min_freq = cal_dfs_get_min_freq(domain->cal_id);

	/*
	 * If max-freq property exists in device tree, max frequency is
	 * selected to smaller one between the value defined in device
	 * tree and CAL. In case of min-freq, min frequency is selected
	 * to bigger one.
	 */
	if (!of_property_read_u32(dn, "max-freq", &val))
		domain->max_freq = min(domain->max_freq, val);
	if (!of_property_read_u32(dn, "min-freq", &val))
		domain->min_freq = max(domain->min_freq, val);

	/* Default QoS for user */
	if (!of_property_read_u32(dn, "user-default-qos", &val))
		domain->user_default_qos = val;

	domain->boot_freq = cal_dfs_get_boot_freq(domain->cal_id);
	domain->resume_freq = cal_dfs_get_resume_freq(domain->cal_id);

	/* Initialize freq boost */
	if (domain->boost_supported) {
		unsigned int i, cpu_count = cpumask_weight(&domain->cpus);

		cal_dfs_get_bigturbo_max_freq(domain->boost_max_freqs);
		if (of_find_property(dn, "boost_max_freqs", NULL)) {
			unsigned int *max_freqs;

			max_freqs = kzalloc(sizeof(unsigned int) * cpu_count, GFP_KERNEL);
			if (!max_freqs)
				return -ENOMEM;

			of_property_read_u32_array(dn, "boost_max_freqs",
						max_freqs, cpu_count);

			/*
			 * If boost_max_freqs property exists in device tree,
			 * frequency is selected to smaller one.
			 */
			for (i = 0; i < cpu_count; i++)
				domain->boost_max_freqs[i] =
					min(domain->boost_max_freqs[i], max_freqs[i]);

			kfree(max_freqs);
		}

		for (i = 0; i < cpu_count; i++) {
			if (domain->boost_max_freqs[i] < domain->min_freq) {
				pr_err("Wrong boost maximum frequency\n");
				domain->boost_supported = false;
				goto init_table;
			}
		}

		/* change domain->max_freq to maximum level in boost table */
		domain->max_freq = max(domain->max_freq, domain->boost_max_freqs[0]);
	}

init_table:
	ret = init_table(domain);
	if (ret)
		return ret;

	domain->old = get_freq(domain);

	/* Initialize PM QoS */
	ret = init_pm_qos(domain, dn);
	if (ret)
		return ret;

	/*
	 * Initialize CPUFreq DVFS Manager
	 * DVFS Manager is the optional function, it does not check return value
	 */
	init_dm(domain, dn);

	pr_info("Complete to initialize cpufreq-domain%d\n", domain->id);

	return ret;
}

static __init int early_init_domain(struct exynos_cpufreq_domain *domain,
					struct device_node *dn)
{
	const char *buf;
	int ret;

	/* Initialize list head of DVFS Manager constraints */
	INIT_LIST_HEAD(&domain->dm_list);

	ret = of_property_read_u32(dn, "cal-id", &domain->cal_id);
	if (ret)
		return ret;

	/* Get size of frequency table from CAL */
	domain->table_size = cal_dfs_get_lv_num(domain->cal_id);

	/* Get cpumask which belongs to domain */
	ret = of_property_read_string(dn, "sibling-cpus", &buf);
	if (ret)
		return ret;
	cpulist_parse(buf, &domain->cpus);
	cpumask_and(&domain->cpus, &domain->cpus, cpu_possible_mask);
	if (cpumask_weight(&domain->cpus) == 0)
		return -ENODEV;

	/* Get whether supporting freq boost */
	if (of_property_read_bool(dn, "boost_supported"))
		domain->boost_supported = true;

	return 0;
}

static __init void __free_domain(struct exynos_cpufreq_domain *domain)
{
	struct exynos_cpufreq_dm *dm;

	while (!list_empty(&domain->dm_list)) {
		dm = list_last_entry(&domain->dm_list,
				struct exynos_cpufreq_dm, list);
		list_del(&dm->list);
		kfree(dm->c.freq_table);
		kfree(dm);
	}

	kfree(domain->freq_table);
	kfree(domain);
}

static __init void free_domain(struct exynos_cpufreq_domain *domain)
{
	list_del(&domain->list);
	unregister_exynos_dm_freq_scaler(domain->dm_type);

	__free_domain(domain);
}

static __init struct exynos_cpufreq_domain *alloc_domain(struct device_node *dn)
{
	struct exynos_cpufreq_domain *domain;
	struct device_node *root, *child;

	domain = kzalloc(sizeof(struct exynos_cpufreq_domain), GFP_KERNEL);
	if (!domain)
		return NULL;

	/*
	 * early_init_domain() initailize the domain information requisite
	 * to allocate domain and table.
	 */
	if (early_init_domain(domain, dn))
		goto free;

	/*
	 * Allocate frequency table.
	 * Last row of frequency table must be set to CPUFREQ_TABLE_END.
	 * Table size should be one larger than real table size.
	 */
	domain->freq_table =
		kzalloc(sizeof(struct cpufreq_frequency_table)
				* (domain->table_size + 1), GFP_KERNEL);
	if (!domain->freq_table)
		goto free;

	/* Allocate boost table */
	if (domain->boost_supported) {
		domain->boost_max_freqs = kzalloc(sizeof(unsigned int)
				* cpumask_weight(&domain->cpus), GFP_KERNEL);
		if (!domain->boost_max_freqs)
			goto free;
	}

	/*
	 * Allocate DVFS Manager constraints.
	 * Constraints are needed only by DVFS Manager, these are not
	 * created when DVFS Manager is disabled. If constraints does
	 * not exist, driver does scaling without DVFS Manager.
	 */
#ifndef CONFIG_EXYNOS_DVFS_MANAGER
	return domain;
#endif

	root = of_find_node_by_name(dn, "dm-constraints");
	for_each_child_of_node(root, child) {
		struct exynos_cpufreq_dm *dm;

		dm = kzalloc(sizeof(struct exynos_cpufreq_dm), GFP_KERNEL);
		if (!dm)
			goto free;

		dm->c.freq_table = kzalloc(sizeof(struct exynos_dm_freq)
					* domain->table_size, GFP_KERNEL);
		if (!dm->c.freq_table)
			goto free;

		list_add_tail(&dm->list, &domain->dm_list);
	}

	return domain;

free:
	__free_domain(domain);

	return NULL;
}

static int __init exynos_cpufreq_init(void)
{
	struct device_node *dn = NULL;
	struct exynos_cpufreq_domain *domain;
	int ret = 0;
	unsigned int domain_id = 0;

	while ((dn = of_find_node_by_type(dn, "cpufreq-domain"))) {
		domain = alloc_domain(dn);
		if (!domain) {
			pr_err("failed to allocate domain%d\n", domain_id);
			continue;
		}

		list_add_tail(&domain->list, &domains);

		domain->id = domain_id;
		ret = init_domain(domain, dn);
		if (ret) {
			pr_err("failed to initialize cpufreq domain%d\n",
							domain_id);
			free_domain(domain);
			continue;
		}

		print_domain_info(domain);
		domain_id++;
	}

	if (!domain_id) {
		pr_err("Can't find device type cpufreq-domain\n");
		return -ENODATA;
	}

	ret = cpufreq_register_driver(&exynos_driver);
	if (ret) {
		pr_err("failed to register cpufreq driver\n");
		return ret;
	}

	init_sysfs();

	pm_qos_add_request(&cpu_online_max_qos_req, PM_QOS_CPU_ONLINE_MAX,
					PM_QOS_CPU_ONLINE_MAX_DEFAULT_VALUE);
	register_hotcpu_notifier(&exynos_cpufreq_cpu_up_notifier);
	register_hotcpu_notifier(&exynos_cpufreq_cpu_down_notifier);
	register_cpus_notifier(&exynos_cpufreq_cpus_notifier);

	cpufreq_register_notifier(&exynos_cpufreq_policy_notifier,
					CPUFREQ_POLICY_NOTIFIER);

	/*
	 * Enable scale of domain.
	 * Update frequency as soon as domain is enabled.
	 */
	list_for_each_entry(domain, &domains, list) {
		enable_domain(domain);
		update_freq(domain, domain->old);
		cpufreq_update_policy(cpumask_first(&domain->cpus));
	}

	pr_info("Initialized Exynos cpufreq driver\n");

	return ret;
}
device_initcall(exynos_cpufreq_init);
