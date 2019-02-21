
#ifdef CONFIG_SCHED_TUNE

#ifdef CONFIG_FREQVAR_SCHEDTUNE
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/of.h>

#define SCHEDTUNE_LOAD_BOOST_UTIT 10

struct freqvar_boost_state {
	struct freqvar_boost_table *table;
	bool enabled;	/* boost enabled */
	int ratio;	/* current boost ratio */
};

struct freqvar_boost_table {
	int frequency;
	int boost;
};

struct freqvar_boost_data {
	struct freqvar_boost_table *table;
};

int schedtune_freqvar_boost(int cpu);
int schedtune_freqvar_boost_init(struct cpufreq_policy *policy, struct freqvar_boost_data *data);
int schedtune_freqvar_boost_exit(struct cpufreq_policy *policy, struct freqvar_boost_data *data);
int schedtune_freqvar_update_table(unsigned int *src, int src_size,
					struct freqvar_boost_table *dst);
#else
static inline int schedtune_freqvar_boost(int cpu) { return 0; }
static inline int schedtune_freqvar_boost_init(struct cpufreq_policy *policy,
						struct freqvar_boost_data *data) { return 0; };
static inline int schedtune_freqvar_boost_exit(struct cpufreq_policy *policy,
						struct freqvar_boost_data *data) { return 0; };
#endif

#include <linux/reciprocal_div.h>

/*
 * System energy normalization constants
 */
struct target_nrg {
	unsigned long min_power;
	unsigned long max_power;
	struct reciprocal_value rdiv;
};

#ifdef CONFIG_CGROUP_SCHEDTUNE

int schedtune_cpu_boost(int cpu);
int schedtune_task_boost(struct task_struct *tsk);

int schedtune_prefer_idle(struct task_struct *tsk);

void schedtune_exit_task(struct task_struct *tsk);

void schedtune_enqueue_task(struct task_struct *p, int cpu);
void schedtune_dequeue_task(struct task_struct *p, int cpu);

#else /* CONFIG_CGROUP_SCHEDTUNE */

#define schedtune_cpu_boost(cpu)  get_sysctl_sched_cfs_boost()
#define schedtune_task_boost(tsk) get_sysctl_sched_cfs_boost()

#define schedtune_exit_task(task) do { } while (0)

#define schedtune_enqueue_task(task, cpu) do { } while (0)
#define schedtune_dequeue_task(task, cpu) do { } while (0)

#endif /* CONFIG_CGROUP_SCHEDTUNE */

int schedtune_normalize_energy(int energy);
int schedtune_accept_deltas(int nrg_delta, int cap_delta,
                           struct task_struct *task);

#else /* CONFIG_SCHED_TUNE */

#define schedtune_cpu_boost(cpu)  0
#define schedtune_task_boost(tsk) 0

#define schedtune_exit_task(task) do { } while (0)

#define schedtune_enqueue_task(task, cpu) do { } while (0)
#define schedtune_dequeue_task(task, cpu) do { } while (0)

#define schedtune_accept_deltas(nrg_delta, cap_delta, task) nrg_delta

#endif /* CONFIG_SCHED_TUNE */
