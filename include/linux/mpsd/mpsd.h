#ifndef _LINUX_MPSD_H
#define _LINUX_MPSD_H
#include <linux/sched.h>

#define MPSD_KMD_VERSION (1)

#define MAX_PAGES (10)
#define BUFFER_LENGTH_MAX (MAX_PAGES * PAGE_SIZE)

#define DEFAULT_PID (0)
#define DEFAULT_EVENTS (0)
#define DEFAULT_CONFIG (0)
#define DEFAULT_VERSION (0)
#define DEFAULT_APP_FIELD_MASK (0)
#define DEFAULT_DEV_FIELD_MASK (0)
#define DEFAULT_TIMER_MIN_LIMIT (100)
#define DEFAULT_PARAM_VAL_INT (-99LL)
#define DEFAULT_PARAM_VAL_CHAR ('\0')
#define DEFAULT_PARAM_VAL_STR ("-99")

#define CPULOAD_HISTORY_INTERVAL (5000)
#define CPULOAD_CHECK_DELAY (20)

#define MAX_CHAR_BUF_SIZE (30)
#define MAX_FREQ_LIST_SIZE (40)
#define MAX_NUM_CPUS (NR_CPUS)
#define MAX_LOAD_HISTORY ((CPULOAD_HISTORY_INTERVAL / CPULOAD_CHECK_DELAY))
#define MAX_TAG_LENGTH (13)

#define APP_PARAM_MIN (0)

enum app_param {
	APP_PARAM_NAME = APP_PARAM_MIN,
	APP_PARAM_TGID,
	APP_PARAM_FLAGS,
	APP_PARAM_PRIORITY,
	APP_PARAM_NICE,
	APP_PARAM_CPUS_ALLOWED,
	APP_PARAM_VOLUNTARY_CTXT_SWITCHES,
	APP_PARAM_NONVOLUNTARY_CTXT_SWITCHES,
	APP_PARAM_NUM_THREADS,
	APP_PARAM_OOM_SCORE_ADJ,
	APP_PARAM_OOM_SCORE_ADJ_MIN,
	APP_PARAM_UTIME,
	APP_PARAM_STIME,
	APP_PARAM_MEM_MIN_PGFLT,
	APP_PARAM_MEM_MAJ_PGFLT,
	APP_PARAM_STARTTIME,
	APP_PARAM_MEM_VSIZE,
	APP_PARAM_MEM_DATA,
	APP_PARAM_MEM_STACK,
	APP_PARAM_MEM_TEXT,
	APP_PARAM_MEM_SWAP,
	APP_PARAM_MEM_SHARED,
	APP_PARAM_MEM_ANON,
	APP_PARAM_MEM_RSS,
	APP_PARAM_MEM_RSS_LIMIT,
	APP_PARAM_IO_RCHAR,
	APP_PARAM_IO_WCHAR,
	APP_PARAM_IO_SYSCR,
	APP_PARAM_IO_SYSCW,
	APP_PARAM_IO_SYSCFS,
	APP_PARAM_IO_BYTES_READ,
	APP_PARAM_IO_BYTES_WRITE,
	APP_PARAM_IO_BYTES_WRITE_CANCEL,
	APP_PARAM_MAX
};

#define TOTAL_APP_PARAMS (APP_PARAM_MAX - APP_PARAM_MIN)

/**
 * struct app_params_struct - Contains all the application level parameters.
 * @name: Name of the process associated with pid passed.
 * @tgid: Thread group id of the process whose params we are reading.
 * @flags: The kernel flags word of the process.
 * @priority: Priority of the application
 * @nice: The nice value, a value in the range 19 (low) to -20 (high).
 * @cpus_allowed: CPUs allowed for this app to run.
 * @voluntary_ctxt_switches: Voluntary context switch counts.
 * @nonvoluntary_ctxt_switches: Nonvoluntary context switch counts.
 * @num_threads: Number of threads in this app.
 * @oom_score_adj: Based on this value decision is made which app to kill.
 * @oom_score_adj_min: Minimum value of oom_score_adj set for an app.
 * @utime: Time that this app has been scheduled in user mode in clock ticks.
 * @stime: Time that this app has been scheduled in kernel mode in clock ticks.
 * @mem_min_pgflt: The number of minor faults the app has made.
 * @mem_maj_pgflt: The number of major faults the app has made.
 * @start_time: The time the app started after system boot, in clock ticks.
 * @mem_vsize: Virtual memory size in bytes of this app.
 * @mem_data: Data + stack size in bytes of this app.
 * @mem_stack: stack size in bytes of this application.
 * @mem_text: Text (code) size in pages of this app.
 * @mem_swap: Swapped memory in pages of this app.
 * @mem_shared: Shared memory in pages of this app.
 * @mem_anon: Anonymous memory in pages of this app.
 * @mem_rss: Resident Set Size: Pages the app has in real memory.
 * @mem_rss_limit: Current soft limit in bytes on the rss of the app.
 * @io_rchar: Number of bytes read by this application.
 * @io_wchar: Number of bytes wrote by this application.
 * @io_syscr: Number of read syscalls by this application.
 * @io_syscw: Number of write syscalls by this application.
 * @io_syscfs: Number of fsync syscalls by this application.
 * @io_bytes_read: The number of bytes caused to be read from storage.
 * @io_bytes_write: The number of bytes caused to be written to storage.
 * @io_bytes_write_cancel: Bytes cancelled from writting.
 *
 * This structure contains all the application specific kernel parameters.
 */
struct app_params_struct {
	char name[TASK_COMM_LEN];
	signed long long tgid;
	signed long long flags;
	signed long long priority;
	signed long long nice;
	signed long long cpus_allowed;
	signed long long voluntary_ctxt_switches;
	signed long long nonvoluntary_ctxt_switches;
	signed long long num_threads;
	signed long long oom_score_adj;
	signed long long oom_score_adj_min;
	signed long long utime;
	signed long long stime;
	signed long long mem_min_pgflt;
	signed long long mem_maj_pgflt;
	signed long long start_time;
	signed long long mem_vsize;
	signed long long mem_data;
	signed long long mem_stack;
	signed long long mem_text;
	signed long long mem_swap;
	signed long long mem_shared;
	signed long long mem_anon;
	signed long long mem_rss;
	signed long long mem_rss_limit;
	signed long long io_rchar;
	signed long long io_wchar;
	signed long long io_syscr;
	signed long long io_syscw;
	signed long long io_syscfs;
	signed long long io_bytes_read;
	signed long long io_bytes_write;
	signed long long io_bytes_write_cancel;
};

#define PARAM_DELTA (0)
#define DEV_PARAM_MIN (APP_PARAM_MAX + PARAM_DELTA)

enum dev_param {
	DEV_PARAM_CPU_PRESENT = DEV_PARAM_MIN,
	DEV_PARAM_CPU_ONLINE,
	DEV_PARAM_CPU_UTIL,
	DEV_PARAM_CPU_FREQ_AVAIL,
	DEV_PARAM_CPU_MAX_LIMIT,
	DEV_PARAM_CPU_MIN_LIMIT,
	DEV_PARAM_CPU_CUR_FREQ,
	DEV_PARAM_CPU_FREQ_TIME_STATS,
	DEV_PARAM_CPU_FREQ_GOV,
	DEV_PARAM_CPU_LOADAVG_1MIN,
	DEV_PARAM_CPU_LOADAVG_5MIN,
	DEV_PARAM_CPU_LOADAVG_15MIN,
	DEV_PARAM_SYSTEM_UPTIME,
	DEV_PARAM_CPU_TIME_USER,
	DEV_PARAM_CPU_TIME_NICE,
	DEV_PARAM_CPU_TIME_SYSTEM,
	DEV_PARAM_CPU_TIME_IDLE,
	DEV_PARAM_CPU_TIME_IO_WAIT,
	DEV_PARAM_CPU_TIME_IRQ,
	DEV_PARAM_CPU_TIME_SOFT_IRQ,
	DEV_PARAM_MEM_TOTAL,
	DEV_PARAM_MEM_FREE,
	DEV_PARAM_MEM_AVAIL,
	DEV_PARAM_MEM_BUFFER,
	DEV_PARAM_MEM_CACHED,
	DEV_PARAM_MEM_SWAP_CACHED,
	DEV_PARAM_MEM_SWAP_TOTAL,
	DEV_PARAM_MEM_SWAP_FREE,
	DEV_PARAM_MEM_DIRTY,
	DEV_PARAM_MEM_ANON_PAGES,
	DEV_PARAM_MEM_MAPPED,
	DEV_PARAM_MEM_SHMEM,
	DEV_PARAM_MEM_SYSHEAP,
	DEV_PARAM_MEM_SYSHEAP_POOL,
	DEV_PARAM_MEM_VMALLOC_API,
	DEV_PARAM_MEM_KGSL_SHARED,
	DEV_PARAM_MEM_ZSWAP,
	DEV_PARAM_MAX
};

#define TOTAL_DEV_PARAMS (DEV_PARAM_MAX - DEV_PARAM_MIN)

#define CPUFREQ_GOV_NONE "none"
#define CPUFREQ_GOV_PERFORMANCE "performance"
#define CPUFREQ_GOV_POWERSAVE "powersave"
#define CPUFREQ_GOV_USERSPACE "userspace"
#define CPUFREQ_GOV_ONDEMAND "ondemand"
#define CPUFREQ_GOV_CONSERVATIVE "conservative"
#define CPUFREQ_GOV_INTERACTIVE "interactive"
#define CPUFREQ_GOV_SCHEDUTIL "schedutil"

/**
 * struct dev_cpufreq_gov_performance - Contains all the performance gov params.
 * @cur_freq: What is the current frequency, it should be same as max
 *		   freq of the policy.
 *
 * This structure contains all the performance gov params.
 */
struct dev_cpufreq_gov_performance {
	signed long long cur_freq;
};

/**
 * struct dev_cpufreq_gov_powersave - Contains all the powersave gov params.
 * @cur_freq: What is the current frequency, it should be same as min
 *		   freq of the policy.
 *
 * This structure contains all the powersave gov params.
 */
struct dev_cpufreq_gov_powersave {
	signed long long cur_freq;
};

/**
 * struct dev_cpufreq_gov_userspace - Contains all the userspace gov params.
 * @set_speed: The freq set from userspace.
 *
 * This structure contains all the userspace gov params.
 */
struct dev_cpufreq_gov_userspace {
	signed long long set_speed;
};

/**
 * struct dev_cpufreq_gov_ondemand - Contains all the ondemand gov params.
 * @min_sampling_rate: It is limited by the HW transition latency.
 * @ignore_nice_load: When set to '0' (its default), all processes are
 *		   counted towards the 'cpu utilisation' value.	 When set to '1'
 *		   the processes that are run with a 'nice' value will not count
 *		   (and thus be ignored) in the overall usage calculation.
 * @sampling_rate: This is how often you want the kernel to look at the CPU
 *		   usage and to make decisions about the frequency.
 * @sampling_down_factor: This parameter controls the rate at which the
 *		   kernel makes a decision to decrease the frequency
 *		   running at top speed.
 * @up_threshold: Defines what the average CPU usage between the sampling of
 *		   'sampling_rate' needs to be to make a decision on
 *		   whether it should increase the frequency.
 * @io_is_busy: Should io time be counted in busy time or idle while
 *		   calculating load.
 * @powersave_bias: It defines the percentage (times 10) value of the target
 *		   frequency that will be shaved off of the target.
 *
 * This structure contains all the ondemand gov params.
 */
struct dev_cpufreq_gov_ondemand {
	signed long long min_sampling_rate;
	signed long long ignore_nice_load;
	signed long long sampling_rate;
	signed long long sampling_down_factor;
	signed long long up_threshold;
	signed long long io_is_busy;
	signed long long powersave_bias;
};

/**
 * struct dev_cpufreq_gov_conservative - Contains all the conservative gov
 *	params.
 * @min_sampling_rate: It is limited by the HW transition latency.
 * @ignore_nice_load: When set to '0' (its default), all processes are
 *		   counted towards the 'cpu utilisation' value.	 When set to '1'
 *		   the processes that are run with a 'nice' value will not count
 *		   (and thus be ignored) in the overall usage calculation.
 * @sampling_rate: This is how often you want the kernel to look at the CPU
 *		   usage and to make decisions on what to do about the frequency
 * @sampling_down_factor: This parameter controls the rate at which the
 *		   kernel makes a decision on when to decrease the frequency
 *		 while running at top speed.
 * @up_threshold: Defines what the average CPU usage between the sampling of
 *		   'sampling_rate' needs to be for the kernel to make a decision
 *		   on whether it should increase the frequency.
 * @down_threshold: Defines what the average CPU usage between the sampling of
 *		   'sampling_rate' needs to be for the kernel to make a
 *		   decision on whether it should decrease the frequency.
 * @freq_step: This describes what percentage steps the cpu freq should be
 *		   increased and decreased smoothly by.
 *
 * This structure contains all the conservative gov params.
 */
struct dev_cpufreq_gov_conservative {
	signed long long min_sampling_rate;
	signed long long ignore_nice_load;
	signed long long sampling_rate;
	signed long long sampling_down_factor;
	signed long long up_threshold;
	signed long long down_threshold;
	signed long long freq_step;
};

/**
 * struct cpufreq_gov_interactive - Contains all the interactive gov params.
 *
 * @hispeed_freq: An intermediate "hi speed" at which to initially
 *		   ramp when loadhits value specified in go_hispeed_load.
 * @go_hispeed_load: The CPU load at which to ramp to hispeed_freq.
 * @target_loads: CPU load values used to adjust speed to influence
 *		   the current CPU load toward that value. In hz.
 * @min_sample_time: The minimum amount of time to spend at the
 *		   current frequency before ramping down. In uS.
 * @timer_rate: Sample rate for reevaluating CPU load when the
 *		   CPU is not idle. In uS.
 * @above_hispeed_delay: When speed is at or above hispeed_freq,
 *		   wait for this long before raising speed in response to
 *		   high load.
 * @boost: If non-zero, immediately boost speed of all CPUs to at
 *		   least hispeed_freq until zero is written to this attribute.
 * @boostpulse_duration: Length of time to hold CPU speed at
 *		   hispeed_freq on a write to boostpulse, before allowing speed
 *		   drop according to load as usual. In uS.
 * @timer_slack: Maximum additional time to defer handling the
 *		   governor sampling timer beyond timer_rate when running at
 *		   speeds above the minimum. In uS.
 * @io_is_busy: To include i/o time as busy time of idle, depending on
 *		   which load will come different.
 * @use_sched_load: If non-zero, query scheduler for CPU busy time,
 *		   instead of collecting it directly in governor.
 * @use_migration_notif: If non-zero, schedule hrtimer to fire in
 *		   1ms to reevaluate frequency of notified CPU, unless the
 *		   is already pending. If zero, ignore scheduler notification.
 * @align_windows: If non-zero, align governor timer window to fire at
 *		   multiples ofnumber of jiffies timer_rate converts to.
 * @max_freq_hysteresis: Each time freq evaluation chooses
 *		   policy->max, next max_freq_hysteresis is considered as
 *		   hysteresis period. In uS.
 * @ignore_hispeed_on_notif: If non-zero, do not apply hispeed related
 *		   logic if frequency evaluation is triggered by scheduler.
 * @fast_ramp_down: If non-zero, do not apply min_sample_time if
 *		   frequency evaluationis triggered by scheduler notification.
 * @enable_prediction: If non-zero, two frequencies will be calculated during
 *		   each sampling period: one based on busy time in previous
 *		   sampling period (f_prev), and the other based on prediction
 *		   provided by scheduler (f_pred). Max of both will be selected
 *		   as final frequency.
 *
 * This structure contains all the interactive gov params.
 */
struct dev_cpufreq_gov_interactive {
	signed long long hispeed_freq;
	signed long long go_hispeed_load;
	signed long long target_loads;
	signed long long min_sample_time;
	signed long long timer_rate;
	signed long long above_hispeed_delay;
	signed long long boost;
	signed long long boostpulse_duration;
	signed long long timer_slack;
	signed long long io_is_busy;
	signed long long use_sched_load;
	signed long long use_migration_notif;
	signed long long align_windows;
	signed long long max_freq_hysteresis;
	signed long long ignore_hispeed_on_notif;
	signed long long fast_ramp_down;
	signed long long enable_prediction;
};

/**
 * struct dev_cpufreq_gov_schedutil - Contains all the schedutil gov params.
 * @up_rate_limit_us: The governor waits for up_rate_limit_us time before
 *		   reevaluating the load again, after it has evaluated the
 *		   load once.
 * @down_rate_limit_us: The governor waits for up_rate_limit_us time before
 *		   reevaluating the load again, after it has evaluated the
 *		   load once.
 *
 * This structure contains all the schedutil gov params.
 */
struct dev_cpufreq_gov_schedutil {
	signed long long up_rate_limit_us;
	signed long long down_rate_limit_us;
};

/**
 * union dev_cpufreq_gov_data - Contains the params related to respective
 *		   cpufreq governors.
 * @gov_performance: If gov is performance we need to collect related params.
 * @gov_powersave: If gov is powersave we need to collect related params.
 * @gov_userspace: If gov is userspace we need to collect related params.
 * @gov_ondemand: If gov is ondemand we need to collect related params.
 * @gov_conservative: If gov is conservative we need to collect related params.
 * @gov_interactive: If gov is interactive we need to collect related params.
 * @gov_schedutil: If gov is schedutil we need to collect related params.
 *
 * This union provides the data to be used for registering
 */
union cpufreq_gov_data {
	struct dev_cpufreq_gov_performance gov_performance;
	struct dev_cpufreq_gov_powersave gov_powersave;
	struct dev_cpufreq_gov_userspace gov_userspace;
	struct dev_cpufreq_gov_ondemand gov_ondemand;
	struct dev_cpufreq_gov_conservative gov_conservative;
	struct dev_cpufreq_gov_interactive gov_interactive;
	struct dev_cpufreq_gov_schedutil gov_schedutil;
};

/**
 * struct cpufreq_gov_params - Contains all the params related to cpufreq gov.
 * @gov_id: Enum mapping to a gov id.
 * @cpu_freq_gov_data: Enum mapping to a gov id.
 *
 * This structure contains all the userspace gov params.
 */
struct dev_cpu_freq_gov {
	char gov[MAX_CHAR_BUF_SIZE];
	union cpufreq_gov_data cpu_freq_gov_data;
};

/**
 * struct dev_cpu_freq_time_stats - Contains the data for time_in_stats.
 * @freq: List of freq for this policy node.
 * @time: Time for frequnecy for the same policy node as freq.
 *
 * This structure contains cpufreq time in stats.
 */
struct dev_cpu_freq_time_stats {
	signed long long freq;
	signed long long time;
};

/**
 * struct dev_params_struct - Contains all the device level parameters.
 * @cpu_present: CPUs that are actualy present in the hardware.
 * @cpu_online: CPUs which are currently online.
 * @cpu_util: Load on each CPU stored in a array.
 * @cpu_freq_avail: List of CPU frequencies available.
 * @cpu_max_limit: Maximum frequency limit put by CFMS, -1 if not set.
 * @cpu_min_limit: Minimum frequency limit put by CFMS, -1 if not set.
 * @cpu_cur_freq: Current frequency of the cpu.
 * @cpu_freq_time_stats: time_in_stats data for cpus.
 * @cpu_freq_gov: Current frequency governor related params.
 * @cpu_min_limit: Minimum frequency limit put by CFMS, -1 if not set.
 * @cpu_loadavg_1min: Number of jobs in the run queue (state R) or waiting
 *		   for disk I/O(state D) averaged over 1 min.
 * @cpu_loadavg_5min: Number of jobs in the run queue (state R) or waiting
 *		   for disk I/O(state D) averaged over 5 min.
 * @cpu_loadavg_15min: Number of jobs in the run queue (state R) or waiting
 *		   for disk I/O(state D) averaged over 15 min.
 * @cpu_time_user: Time spent in user mode in units of USER_HZ.
 * @cpu_time_nice: Time spent in user mode with low priority (nice) in USER_HZ.
 * @cpu_time_system: Time spent in system mode in units of USER_HZ.
 * @cpu_time_idle: Time spent in the idle task.
 * @cpu_time_io_Wait: Time waiting for I/O to complete in units of USER_HZ.
 * @cpu_time_irq: Time servicing interrupts in units of USER_HZ.
 * @cpu_time_soft_irq: Time servicing softirqs in units of USER_HZ.
 * @mem_total: Total usable RAM.
 * @mem_free: The sum of LowFree + HighFree.
 * @mem_avail: An estimate of how much memory is available for starting
 *		   new applications,without swapping.
 * @mem_buffer: Relatively temporary storage for raw disk blocks that
 *		   shouldn't get tremendously large.
 * @mem_cached: In-memory cache for files read from the disk (the page cache).
 * @mem_swap_cached: Memory that once was swapped out, is swapped back in
 *		   but still also is in the swap file.
 * @mem_swap_total: Total amount of swap space available.
 * @mem_swap_free: Amount of swap space that is currently unused.
 * @mem_dirty: Memory which is waiting to get written back to the disk.
 * @mem_anon_pages: Non-file backed pages mapped into user-space page tables.
 * @mem_mapped: Files which have been mapped into memory, such as libraries.
 * @mem_shmem: Amount of memory consumed in tmpfs(5) filesystems.
 * @mem_sysheap: System heap memory.
 * @mem_sysheap_pool: System heap memory pool.
 * @mem_vmalloc_api: Memory of vmallock.
 * @mem_kgsl_shared: Shared memory used by Kgsl.
 * @mem_zswap: Zswap memory of the device.
 * @cpu_freq_gov_params: Cpu frequnecy governor params.
 *
 * This structure contains all the device specific kernel parameters.
 */
struct dev_params_struct {
	signed long long cpu_present;
	signed long long cpu_online;
	signed long long cpu_util[MAX_NUM_CPUS][MAX_LOAD_HISTORY];
	signed long long cpu_freq_avail[MAX_NUM_CPUS][MAX_FREQ_LIST_SIZE];
	signed long long cpu_max_limit[MAX_NUM_CPUS];
	signed long long cpu_min_limit[MAX_NUM_CPUS];
	signed long long cpu_cur_freq[MAX_NUM_CPUS];
	struct dev_cpu_freq_time_stats cpu_freq_time_stats[MAX_NUM_CPUS][MAX_FREQ_LIST_SIZE];
	struct dev_cpu_freq_gov cpu_freq_gov[MAX_NUM_CPUS];
	signed long long cpu_loadavg_1min;
	signed long long cpu_loadavg_5min;
	signed long long cpu_loadavg_15min;
	signed long long system_uptime;
	signed long long cpu_time_user;
	signed long long cpu_time_nice;
	signed long long cpu_time_system;
	signed long long cpu_time_idle;
	signed long long cpu_time_io_Wait;
	signed long long cpu_time_irq;
	signed long long cpu_time_soft_irq;
	signed long long mem_total;
	signed long long mem_free;
	signed long long mem_avail;
	signed long long mem_buffer;
	signed long long mem_cached;
	signed long long mem_swap_cached;
	signed long long mem_swap_total;
	signed long long mem_swap_free;
	signed long long mem_dirty;
	signed long long mem_anon_pages;
	signed long long mem_mapped;
	signed long long mem_shmem;
	signed long long mem_sysheap;
	signed long long mem_sysheap_pool;
	signed long long mem_vmalloc_api;
	signed long long mem_kgsl_shared;
	signed long long mem_zswap;
};

#define EVENT_TYPE_MIN (0)

/* Event Types used by mpsd for registration of notifications */
enum notifier_event_type {
	EVENT_VALUE_CHANGED = EVENT_TYPE_MIN,
	EVENT_REACHED_MAX_THRESHOLD,
	EVENT_REACHED_MIN_THRESHOLD,
	EVENT_TYPE_MAX,
};

/**
 * struct param_info_notifier - Contains input params information used
 *		   when param is registered for event notification.
 * @param: Param id of the paramater, either app_param or dev_prama type.
 * @events: Contains information about what
 *		   all events this parameter needs to be registered.
 * @max_threshold: If param is regsitered for max event, this value is needed.
 * @min_threshold: If param is regsitered for min event, this value is needed.
 *
 * This structure contains all the information needed by mpsd driver
 * to provide data and events for that param.
 */
struct param_info_notifier {
	int param;
	int events;
	signed long long delta;
	signed long long max_threshold;
	signed long long min_threshold;
};

/**
 * struct param_data_notifier - Contains event information and read data of
 * param.
 * @param: Param id of the paramater, either app_param or dev_prama type.
 * @events: Gives the information which event has just occured.
 * @prev: Previous value of the parameter.
 * @cur: Current value of the parameter
 *
 * This structure provides the data back to mpsd driver which will send this
 * data to UKD with information about what kind of event has occured.
 */
struct param_data_notifier {
	unsigned int param;
	unsigned int events;
	signed long long prev;
	signed long long cur;
};

/* Request types to be used for params to register or unregister */
enum req_id {
	SYNC_READ = 0,
	TIMED_UPDATE,
	NOTIFICATION_UPDATE,
};

/**
 * union req_data - Contains the data related to request id.
 * @val: Timer interval in msec to be used for timed notification.
 * @info: Data containg information for notifier framework.
 *
 * This union provides the data to be used for registering
 * the params to different request types.
 */
union req_data {
	unsigned int val;
	struct param_info_notifier info;
};

/**
 * struct mpsd_req - Contains the information to be passed from UMD.
 * @param_id: Parameter ID or PID
 * @id: Sync, Timed or Notification update.
 * @data: Contains the data related to request id
 *
 * This structure provides the data to be passed from UMD to KMD.
 */
struct mpsd_req {
	unsigned int param_id;
	enum req_id id;
	union req_data data;
};

/**
 * struct memory_area - Contains the layout for shared memory.
 * @updating: Flag to be set by the writer when start writing to buffer.
 * @app_field_mask: Give bit field for the the app params to be read.
 * @app_params: Contains values for all the app specific params.
 * @device_field_mask: Give bit field for the the device params to be read.
 * @dev_params: Contains values for all the device specific params.
 * @req: Sync, Timed or Notification update.
 * @event: Which notification update has occured.
 *
 * This structure provides the layout for the shared memory to be read from UMD
 */
struct memory_area {
	unsigned int updating;
	unsigned long long app_field_mask;
	struct app_params_struct app_params;
	unsigned long long dev_field_mask;
	struct dev_params_struct dev_params;
	enum req_id req;
	unsigned int event;
};

bool get_mpsd_flag(void);
void mpsd_event_notify(int param, unsigned long prev, unsigned long cur);

#define MPSD_IOC_MAGIC 'M'

/**
 * DOC: MPSD_IOC_REGISTER - register access to kernel parameter
 *
 * Takes an mpsd_msg_data struct and returns it with the success or failure.
 */
#define MPSD_IOC_REGISTER _IOWR(MPSD_IOC_MAGIC, 0, struct mpsd_req)

/**
 * DOC: MPSD_IOC_UNREGISTER - unregister access to kernel parameter
 *
 * Takes an mpsd_msg_data struct and returns it with the success or failure.
 */
#define MPSD_IOC_UNREGISTER _IOWR(MPSD_IOC_MAGIC, 1, struct mpsd_req)

/**
 * DOC: MPSD_IOC_REGISTER - register access to kernel parameter
 *
 * Takes an uint32 as pid
 * Will return success or failure.
 */
#define MPSD_IOC_READ _IOW(MPSD_IOC_MAGIC, 2, unsigned int)

/**
 * DOC: MPSD_IOC_VERSION - register access to kernel parameter
 *
 * Takes an int value and returns the current version of MPSD KMD interface
 */
#define MPSD_IOC_VERSION _IOR(MPSD_IOC_MAGIC, 3, unsigned int)

/**
 * DOC: MPSD_IOC_CONFIG - register access to kernel parameter
 *
 * Takes an int value and returns the success or failure
 */
#define MPSD_IOC_CONFIG _IOW(MPSD_IOC_MAGIC, 4, unsigned int)

/**
 * DOC: MPSD_IOC_SET_PID - register access to kernel parameter
 *
 * Takes an int value and returns the success or failure
 */
#define MPSD_IOC_SET_PID _IOW(MPSD_IOC_MAGIC, 5, unsigned int)

#endif /* _LINUX_MPSD_H */

