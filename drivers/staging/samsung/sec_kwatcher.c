#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/aio.h>
#include <linux/sec_debug.h>
#include <asm/map.h>
#include <asm/tlbflush.h>
#include <asm/hw_breakpoint.h>
#include <linux/hw_breakpoint.h>
#include <asm/debug-monitors.h>
#include <linux/kthread.h>
#include <linux/sec_sysfs.h>
#include <asm/system_misc.h>
#include <asm/smp.h>
#include <asm/smp_plat.h>
#include <linux/delay.h>
#ifdef CONFIG_NO_BOOTMEM
#include <linux/memblock.h>
#endif
#include <linux/sec_kwatcher.h>
#include <linux/pm_qos.h>
#include <linux/completion.h>

#if SEC_KWATCHER_UNIT_TEST
#include "sec_unit_test_core.h"
#endif

#define get_current_context(context) do { context.pid = current->pid; context.irq_count = in_interrupt();} while (0)
#define is_same_context(context1, context2) ((context1.pid == context2.pid && context1.irq_count == context2.irq_count) ? 1:0)

enum kwatcher_ops {
	KWATCHER_NONE,
	KWATCHER_DISABLE,
	KWATCHER_ENABLE,
	KWATCHER_UNINSTALL,
	KWATCHER_INSTALL
};

struct kw_event_info {
	int force_imask;
};

struct kwatcher_index{
	atomic_t wpe_idx;
	/*
	atomic_t bpe_idx;
	atomic_t sst_idx;
	 */
};

unsigned long long initBase;
unsigned long long initOffset;
unsigned long long initType;
unsigned long long initDtype;
int panic_on_watching = 0;

static DEFINE_PER_CPU(struct kw_event_info, wevt_info);

char __aligned(4096) wp_buf[4096];

static struct pm_qos_request kwatcher_min_pm_qos;
static struct kwatcher_index kwatcher_idx;
static struct completion kwatcher_control_done;

struct context_info {
	int pid;
	long unsigned int irq_count;
};

struct bypass_section_info {
	struct list_head list;
	struct context_info context;
};

struct kwatcher_group kwatcher;

struct kw_tester {
	char x1;
	char x2;
	int x3;
	int x4;
	int x5;
};

struct kw_tester *k1;
struct kw_tester *testrun = NULL;

int testIndex;

#define HOOK_BUF_SIZE 1000
#define HOOK_SHOW_SIZE 20
static struct _kwatcher_stack_info {
	int cpu;
	u64 time;
	u64 base;
	int offset;
	void *caller[4];
} kwatcher_stack_info[HOOK_BUF_SIZE];
static u64 kstack_cnt;
static int kstack_ovf;

struct device *sec_kwatcher_dev;
EXPORT_SYMBOL(sec_kwatcher_dev);

struct device *sec_kwatcher_tester_dev;
EXPORT_SYMBOL(sec_kwatcher_tester_dev);


static void clear_wpe(struct wpe_watcher *watcher)
{
	watcher->wp.hw.info.ctrl.len       = 0;
	watcher->wp.hw.info.ctrl.mask     = 0;
	watcher->wp.hw.info.ctrl.type      = 0;
	watcher->wp.hw.info.ctrl.privilege = 0;
	watcher->inUse	= 0;
	watcher->base   = 0;
	watcher->offset   = 0;
	watcher->wp.hw.info.ctrl.enabled   = 0;
	watcher->wp.hw.info.address        = 0;
	watcher->wp.used_in_kwatcher = 0;
	watcher->enabled = 0;

	watcher->dtype = 0;
	watcher->comp_condition.comp_op = COMP_NONE;
	watcher->comp_condition.comp_value = 0;
}

void kwatcher_exclusive_break_point(unsigned int *base)
{
	struct perf_event *bp;
	bp = this_cpu_ptr(kwatcher.bp);
	bp->hw.info.ctrl.type = ARM_BREAKPOINT_EXECUTE;
	bp->hw.info.ctrl.privilege = AARCH64_BREAKPOINT_EL1;
	bp->hw.info.ctrl.enabled = 1;
	bp->hw.info.ctrl.len = ARM_BREAKPOINT_LEN_4;
	bp->hw.info.address = (unsigned long) base;
	pr_info("kwatcher base : %lx\n", (unsigned long) base);
	arch_install_hw_breakpoint(bp);
	read_arg2();
}

void kwatcher_exclusive_break_point_uninstall(void)
{
	struct perf_event *bp;
	bp = this_cpu_ptr(kwatcher.bp);
	arch_uninstall_hw_breakpoint(bp);
	bp->hw.info.ctrl.type = 0;
	bp->hw.info.ctrl.privilege = 0;
	bp->hw.info.ctrl.enabled = 0;
	bp->hw.info.ctrl.len = 0;
	bp->hw.info.address = 0;
}

static int kwatcher_control_pcpu(unsigned int cpu)
{

	int ret = 0;
	int i;
	int bComplete = 0;

	for (i = 0; i < KWATCHER_MAX_NUM ; i++) {
		enum kwatcher_ops *updateNeeded;
		updateNeeded = per_cpu_ptr(kwatcher.wpe_watcher[i].updateNeeded, cpu);
		preempt_disable();
		if (*updateNeeded) {
			pr_info("kwatcher %s cpu : %d processing : %d\n", 
					__func__, cpu, atomic_read(&kwatcher.wpe_watcher[i].processing));
			atomic_sub(1, &kwatcher.wpe_watcher[i].processing);
			if (atomic_read(&kwatcher.wpe_watcher[i].processing) == 0) {
				bComplete = 1;
			}
		}
		switch (*updateNeeded) {
		case KWATCHER_DISABLE:
			kwatcher.wpe_watcher[i].wp.hw.info.ctrl.enabled = 0;
			ret = arch_update_hw_breakpoint(&(kwatcher.wpe_watcher[i].wp));
			break;
		case KWATCHER_ENABLE:
			kwatcher.wpe_watcher[i].wp.hw.info.ctrl.enabled = 1;
			ret = arch_update_hw_breakpoint(&(kwatcher.wpe_watcher[i].wp));
			break;
		case KWATCHER_UNINSTALL:
			arch_uninstall_hw_breakpoint(&(kwatcher.wpe_watcher[i].wp));
			break;
		case KWATCHER_INSTALL:
			ret = arch_install_hw_breakpoint(&(kwatcher.wpe_watcher[i].wp));
			break;
		default:
			break;
		}
		*updateNeeded = KWATCHER_NONE;
		preempt_enable();
	}

	if (bComplete) {
		pr_info("cpu : %d, complete\n", cpu);
		complete(&kwatcher_control_done);
	}

	return ret;
}
void smp_wpe_set(void *info)
{
	int ret;
	int cpu = smp_processor_id();
	ret = kwatcher_control_pcpu(cpu);
}


int sec_kwatcher_available_slot(void)
{
	int i;
	for (i = 0; i < KWATCHER_MAX_NUM ; i++) {
		if (kwatcher.wpe_watcher[i].inUse == 0)
			return i;
	}
	return -1;
}

static int kwatcher_control(enum kwatcher_ops ops, int index)
{
	unsigned int cpu;
	enum kwatcher_ops *updateNeeded;
	int i;

	if ((index < 0) || (index >= KWATCHER_MAX_NUM)) {
		pr_err("kwatcher wrong watchpoint index\n");
		return -EINVAL;
	}

	init_completion(&kwatcher_control_done);
	pm_qos_update_request(&kwatcher_min_pm_qos, PM_QOS_CPU_ONLINE_MAX_DEFAULT_VALUE);
	while (num_online_cpus() != PM_QOS_CPU_ONLINE_MAX_DEFAULT_VALUE)
		;
	atomic_set(&kwatcher.wpe_watcher[index].processing, 7);

	preempt_disable();
	cpu = smp_processor_id();
	updateNeeded = this_cpu_ptr(kwatcher.wpe_watcher[index].updateNeeded);
	*updateNeeded = KWATCHER_NONE;

	switch (ops) {
	case KWATCHER_INSTALL:
		/*
		 change wp.hw.info.ctrl.enabled status as "enabled" before calling
		 arch_install_hw_breakpoint() becaused actual register access depends
		 on the status value. But every core doesn't need to change this
		 variable because it's not per-cpu var.
		*/
		kwatcher.wpe_watcher[index].wp.hw.info.ctrl.enabled   = 1;
		i = arch_install_hw_breakpoint(&(kwatcher.wpe_watcher[index].wp));
		if (i < 0) {
			kwatcher.wpe_watcher[index].wp.hw.info.ctrl.enabled   = 0;
			goto error;
		}
		kwatcher.wpe_watcher[index].enabled = 1;
		break;

	case KWATCHER_UNINSTALL:
		if (kwatcher.wpe_watcher[index].base == 0x0) {
			pr_err("kwatcher index:%d is not installed\n", index);
			goto error;
		}
		pr_info("kwatcher CPU %d initiated uninstallation\n", smp_processor_id());
		arch_uninstall_hw_breakpoint(&(kwatcher.wpe_watcher[index].wp));
		kwatcher.wpe_watcher[index].enabled = 0;
		break;

	case KWATCHER_ENABLE:
		kwatcher.wpe_watcher[index].wp.hw.info.ctrl.enabled = 1;
		arch_update_hw_breakpoint(&(kwatcher.wpe_watcher[index].wp));
		kwatcher.wpe_watcher[index].enabled = 1;
		break;

	case KWATCHER_DISABLE:
		kwatcher.wpe_watcher[index].wp.hw.info.ctrl.enabled = 0;
		arch_update_hw_breakpoint(&(kwatcher.wpe_watcher[index].wp));
		kwatcher.wpe_watcher[index].enabled = 0;
		break;

	default:
		break;
	}

	for (i = 0; i < 8; i++) {
		if (i != cpu) {
			updateNeeded = per_cpu_ptr(kwatcher.wpe_watcher[index].updateNeeded, i);
			*updateNeeded = ops;
		}
	}

	smp_call_function(smp_wpe_set, NULL, 1);
	preempt_enable();
	wait_for_completion(&kwatcher_control_done);
	pm_qos_update_request(&kwatcher_min_pm_qos, PM_QOS_DEFAULT_VALUE);

	/*
		in case of uninstall, clear wpe_watcher data structure here
		this should be done after all IPI action complte, becuase each
		core's unistallation depends on the data wep structure
	*/

	if (ops == KWATCHER_UNINSTALL) {
		clear_wpe(&kwatcher.wpe_watcher[index]);
	}
	return 0;

error:
	preempt_enable();
	pm_qos_update_request(&kwatcher_min_pm_qos, PM_QOS_DEFAULT_VALUE);
	atomic_set(&kwatcher.wpe_watcher[index].processing, 0);

	return -EINVAL;
}


/*
 * kwatcher_alloc_internal()
 * param def.
 *	base :
 *	size :
 *	access_type 1:ARM_BREAKPOINT_LOAD,2:ARM_BREAKPOINT_STORE,3:BOTH
*/
static int kwatcher_alloc_internal(unsigned long long base,
								unsigned long long offset,
								unsigned long access_type, unsigned long dtype, unsigned long enable, ...)
{
	int index;
	unsigned long maskValue, mask = 0;
	va_list args;
	enum comp_op_type op;
	unsigned long long value;

	va_start(args, enable);
	op = va_arg(args, int);
	value = va_arg(args, unsigned long long);

	index = sec_kwatcher_available_slot();
	if (index == -1) {
		pr_err("kwatcher WP list is full, adding is not available\n");
		return -1;
	}
	if (offset == 1) {
		kwatcher.wpe_watcher[index].wp.hw.info.ctrl.len		= 1 << (base & 0x3);
	} else if (offset == 2) {
		kwatcher.wpe_watcher[index].wp.hw.info.ctrl.len		= ARM_BREAKPOINT_LEN_2 << (base & 0x3);
	} else if (offset < 5) {
		kwatcher.wpe_watcher[index].wp.hw.info.ctrl.len  = ARM_BREAKPOINT_LEN_4;
	} else {
		kwatcher.wpe_watcher[index].wp.hw.info.ctrl.len  = ARM_BREAKPOINT_LEN_8;
		maskValue = 0x8;
		mask = 3;

		while (mask < 31 && ((base | (maskValue - 1)) < (base + offset - 1))) {
			maskValue *= 2;
			mask++;
		}
		pr_info("%s : mask : %ld\n", __func__, mask);
	}
	kwatcher.wpe_watcher[index].wp.used_in_kwatcher = 1;
	kwatcher.wpe_watcher[index].wp.hw.info.ctrl.mask = mask;
	kwatcher.wpe_watcher[index].wp.hw.info.ctrl.type      = access_type;
	kwatcher.wpe_watcher[index].wp.hw.info.ctrl.privilege = AARCH64_BREAKPOINT_EL1;
	kwatcher.wpe_watcher[index].inUse = 1;
	kwatcher.wpe_watcher[index].base   = base;
	kwatcher.wpe_watcher[index].offset   = offset;
	kwatcher.wpe_watcher[index].wp.hw.info.ctrl.enabled   = enable;

	if (offset > 0x8) {
		kwatcher.wpe_watcher[index].wp.hw.info.address = (u64)WATCHER_BIT_LSB_CLR(base, mask);
	} else {
		kwatcher.wpe_watcher[index].wp.hw.info.address = (u64)(base & ~0x3);
	}

	/*
	 Do not assign kwatcher.wpe_watcher[index].enabled in this routine.
	 The variable is of per-cpu so its modification should follow each core's
	 actual enabling and disabling action
	*/

	kwatcher.wpe_watcher[index].dtype = dtype;

	if (op != COMP_NONE) {
		kwatcher.wpe_watcher[index].comp_condition.comp_op = op;
		kwatcher.wpe_watcher[index].comp_condition.comp_value = WATCHER_BIT_LSB_REMAIN(value, offset*8);
		pr_info("kwatcher value comparison op:%d, value:%llx, saved_value:%llx", op, value, WATCHER_BIT_LSB_REMAIN(value, offset*8));
	}
	va_end(args);

	return index;
}


static int kwatcher_free_internal(int index)
{
	int ret = 0;

	if (index < 0 && index >= KWATCHER_MAX_NUM) {
		pr_err("kwatcher invalid index\n");
		return -EINVAL;
	}

	if (kwatcher.wpe_watcher[index].inUse == 0) {
		pr_err("kwatcher index %d is no in use\n", index);
		return -EINVAL;
	}

	ret = kwatcher_control(KWATCHER_UNINSTALL, index);

	if (ret) {
		pr_err("kwatcher f:%s result: FAIL!, index: %d, err: %d\n", __func__, (int)index, ret);
	}

	return ret;
}

struct task_struct *testtask;
struct _tp {
	unsigned long long base;
	unsigned long long offset;
	unsigned long long value;
} tp;

int kwatcher_testthread_function(void *data)
{
	udelay(5000);

	if (tp.value != 0)
		*(volatile unsigned long long*)(tp.base+tp.offset) = tp.value;
	else
		*(volatile unsigned long long*)(tp.base+tp.offset) = 0x12345678;
	pr_debug("kwatcher f:%s,l:%d base:%llx offset:%llx\n",
									__func__, __LINE__, tp.base, tp.offset);

	return 0;
}

ssize_t testaccess_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	/* TODO
	 * so dirty type casting
	 */
	char *str = (char *)buf;
	char *str1 = NULL , *str2 = NULL;
	tp.base = memparse(str, &str1);
	tp.offset = memparse(str1+1, &str2);
	tp.value = memparse(str2+1, &str1);

	if (tp.base == 0x0)
		return size;

	pr_info("kwatcher current cpu:%d\n", get_current_cpunum());
	testtask = kthread_run(kwatcher_testthread_function, NULL, "kwatcher_test_thread");
	pr_info("kwatcher testthread triggered with:%p,value:%llx \n", (unsigned int *)(tp.base+tp.offset), tp.value);

	return size;
}

static ssize_t testaccess_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_info("kwatcher your target addr:%p \n", &wp_buf[0]);
	read_arg();
	return 0;
}

static ssize_t testbuffer_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	k1 = kzalloc(sizeof(struct kw_tester), GFP_KERNEL);
	pr_info("kwatcher base:%lx size:%ld\n", (unsigned long)k1, sizeof(*k1));
	testIndex = sec_kwatcher_alloc((unsigned long)k1, sizeof(*k1), 0x3, 0x4);
	return size;
}

static ssize_t testbuffer_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	sec_kwatcher_bypass_enter();
	k1->x1 = 'x';
	k1->x2 = 'y';
	k1->x3 = 5;
	sec_kwatcher_bypass_exit();
	return 0;
}

static ssize_t testrun_alloc_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	char *str = (char *)buf;
	char *str1 = NULL, *str2 = NULL;
	unsigned long long wType, dType;

	wType = memparse(str, &str1);
	dType = memparse(str1+1, &str2);

	if ((wType == 0) && (dType == 0)) {
		if (testrun == NULL) {
			pr_err("kwatcher testrun buffer free FAIL, buffer has not been allocated!\n");
			return size;
		}

		kfree(testrun);
		testrun = NULL;
		pr_info("kwatcher testrun buffer free complete!\n");
	} else {
		if ((wType < 1) || (wType >= ARM_KWATCHER_MAX) ||\
			(dType < 1) || (dType >= TYPE_KWATCHER_MAX)) {
			pr_err("kwatcher f:%s result: FAIL!, argument error\n", __func__);
			return size;
		}
		if (testrun == NULL)
			testrun = kzalloc(sizeof(struct kw_tester), GFP_KERNEL);

		pr_info("kwatcher base:%lx size:%ld, wType: %d, dType: %d\n", (unsigned long)testrun, sizeof(*testrun), (int)wType, (int)dType);

		ret = sec_kwatcher_alloc((unsigned long)testrun, sizeof(*testrun), (enum watch_type)wType, (enum detect_type)dType);
		if (!ret)
			pr_info("kwatcher testrun buffer alloc&set success\n");
	}
	return size;
}

static ssize_t testrun_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{

	char *str = (char *)buf;
	char *str1 = NULL;
	unsigned long long index = memparse(str, &str1);

	if (testrun == NULL) {
		pr_err("kwatcher testrun FAIL, buffer not allocated!\n");
		return size;
	}

	switch ((int)index) {
	case 0: /* read */
		pr_info("kwatcher testbuffer read test(%d)\n",  testrun->x3);
		break;

	case 1: /* write */
		testrun->x3 = (int)index;
		pr_info("kwatcher testbuffer write  test\n");
		break;

	case 2: /* read/write */
		pr_info("kwatcher testbuffer read test(%d)\n",  testrun->x3);
		testrun->x3 = (int)index;
		pr_info("kwatcher testbuffer write  test\n");
		break;

	default:
		break;
	}

	return size;
}

static ssize_t testrun_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	len += scnprintf(buf+len, PAGE_SIZE, "----------------------------------------------------------------------------------------\n");
	len += scnprintf(buf+len, PAGE_SIZE, "Test buffer access test\n");
	len += scnprintf(buf+len, PAGE_SIZE, "1. Test buffer alloc & type set first\n");
	len += scnprintf(buf+len, PAGE_SIZE, "  ex) echo 0x3@0x4 > testrunalloc (testrun buffer alloc, sType-r/w, dType-logging)\n\n");
	len += scnprintf(buf+len, PAGE_SIZE, "2. Access test\n");
	len += scnprintf(buf+len, PAGE_SIZE, "  echo 0 > testrun (read test)\n");
	len += scnprintf(buf+len, PAGE_SIZE, "  echo 1 > testrun (write test)\n");
	len += scnprintf(buf+len, PAGE_SIZE, "  echo 2 > testrun (r/w test)\n");
	len += scnprintf(buf+len, PAGE_SIZE, "----------------------------------------------------------------------------------------\n");

	return len;
}

#if SEC_KWATCHER_UNIT_TEST
//extern struct unit_test ut_watcher_config;

static ssize_t unit_testrun_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	struct unit_test * ut;

	pr_info("kwatcher_ut unit test start\n");


	ut = sec_unit_test_get_ut("watcher basic config test");
	if (ut != NULL) {
		/* register unti function to test here */
		ut->unit_func = (void (*)(void))sec_kwatcher_alloc;
	} else {
		pr_info("kwatcher_ut failed to find\n");
	}

	ut = sec_unit_test_get_ut("watcher basic hit test");
	if (ut != NULL) {
		/* register unti function to test here */
		ut->unit_func = (void (*)(void))sec_kwatcher_alloc;
	} else {
		pr_info("kwatcher_ut failed to find\n");
	}

	ut = sec_unit_test_get_ut("watcher compare test");
	if (ut != NULL) {
		/* register unti function to test here */
		ut->unit_func = (void (*)(void))sec_kwatcher_alloc;
	} else {
		printk("kwatcher_ut failed to find\n");
	}

	sec_unit_test_do_all_test();

	return size;
}

static ssize_t unit_testrun_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int len = 0;

	return len;
}
#endif

static ssize_t wplist_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}

static ssize_t wplist_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	int i;
	int len = 0;

	len += scnprintf(buf+len, PAGE_SIZE, "------------------------------------------------------------------------------\n");
	len += scnprintf(buf+len, PAGE_SIZE, "[#]       base             offset      type      detect     enabled \n");
	len += scnprintf(buf+len, PAGE_SIZE, "------------------------------------------------------------------------------\n");

	for (i = 0; i < KWATCHER_MAX_NUM ; i++) {

		len += scnprintf(buf+len, PAGE_SIZE, "[%3d] %16llx %8d %8d %8d %8d \n", i,
				kwatcher.wpe_watcher[i].base,
				kwatcher.wpe_watcher[i].offset,
				kwatcher.wpe_watcher[i].wp.hw.info.ctrl.type,
				kwatcher.wpe_watcher[i].dtype,
				kwatcher.wpe_watcher[i].enabled);
	}

	len += scnprintf(buf+len, PAGE_SIZE,  "------------------------------------------------------------------------------\n");

	return len;
}

static ssize_t kwatcher_hooktrace_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	int len = 0;
	int index, cnt = 0;

	len += scnprintf(buf+len, PAGE_SIZE, "------------------------------------------------------------------------------\n");
	len += scnprintf(buf+len, PAGE_SIZE, "[#]	cpu	time         base                 offset   caller\n");
	len += scnprintf(buf+len, PAGE_SIZE, "------------------------------------------------------------------------------\n");


	if ((kstack_cnt < HOOK_SHOW_SIZE) && (kstack_ovf == 0))
		index = 0;
	else if (kstack_cnt >= HOOK_SHOW_SIZE)
		index = kstack_cnt - HOOK_SHOW_SIZE;
	else
		index = HOOK_BUF_SIZE - (HOOK_SHOW_SIZE - kstack_cnt);

	while (cnt < HOOK_SHOW_SIZE) {
		if (index >= HOOK_BUF_SIZE)
			index -= HOOK_BUF_SIZE;
		if (kwatcher_stack_info[index].time == 0)
			break;
		len += scnprintf(buf+len, PAGE_SIZE, "[%3d]	%d	%10lld  0x%16llx %8d   %pf < %pf < %pf < %pf\n", index,
							kwatcher_stack_info[index].cpu,
							kwatcher_stack_info[index].time,
							kwatcher_stack_info[index].base,
							kwatcher_stack_info[index].offset,
							kwatcher_stack_info[index].caller[0], kwatcher_stack_info[index].caller[1], kwatcher_stack_info[index].caller[2], kwatcher_stack_info[index].caller[3]);
		index++;
		cnt++;
	}
	len += scnprintf(buf+len, PAGE_SIZE,  "------------------------------------------------------------------------------\n");

	return len;
}

static ssize_t enable_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	char *str = (char *)buf;
	char *str1 = NULL;
	unsigned long long index = memparse(str, &str1);

	ret = kwatcher_control(KWATCHER_ENABLE, index);
	if (ret)
		pr_err("kwatcher f:%s result: FAIL!, index: %d, err: %d\n", __func__, (int)index, ret);

	return size;

}

static ssize_t enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t disable_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	char *str = (char *)buf;
	char *str1 = NULL;
	unsigned long long index = memparse(str, &str1);

	ret = kwatcher_control(KWATCHER_DISABLE, index);
	if (ret)
		pr_err("kwatcher f:%s result: FAIL!, index: %d, err: %d\n", __func__, (int)index, ret);

	return size;
}

static ssize_t disable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static char ksym_name[KSYM_NAME_LEN];
static ssize_t add_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	char *str = (char *)buf;
	char *str1 = NULL , *str2 = NULL, *str3 = NULL, *str4 = NULL;
	int index, i, ret = 0;
	unsigned long long base, offset, wType, dType;
	char *remained;
	enum comp_op_type comp_op;
	unsigned long long comp_value;

	if (str[0] != '0') {
		memset(&ksym_name, 0, KSYM_NAME_LEN);
		for (i = 0; i < KSYM_NAME_LEN; i++) {
			if (str[i] == '@')
				break;
			ksym_name[i] = str[i];
		}
		base = kallsyms_lookup_name(ksym_name);
		str2 = (char *)&str[i];
		kallsyms_lookup_size_offset(base, (unsigned long *)&offset, NULL);
	} else {
		 base = memparse(str, &str1);
		 offset = memparse(str1+1, &str2);
	}

	wType = memparse(str2+1, &str3);
	dType = memparse(str3+1, &str4);

	pr_info("kwatcher \n\n");
	pr_info("kwatcher ========================================================================\n");
	pr_info("kwatcher f:%s,l:%d base:%llx off:%llx type:%llx, dtype:%llx \n",
									__func__, __LINE__, base, offset, wType, dType);

	if ((base == 0x0) || (offset < 1) ||\
		(wType < 1) || (wType >= ARM_KWATCHER_MAX) ||\
		(dType < 1) || (dType >= TYPE_KWATCHER_MAX)) {
		pr_err("kwatcher f:%s result: FAIL!, argument error\n", __func__);
		return size;
	}

	pr_info("kwatcher current cpu:%d \n", get_current_cpunum());
	comp_op = memparse(str4+1, &remained);
	str4 = remained;

	comp_value = memparse(str4+1, &remained);


	if (comp_op != COMP_NONE) {
		if (!(offset == 1 || offset == 2 || offset == 4 || offset == 8)) {
			pr_err("kwatcher size of data to be compared is not supported\n");
			pr_err("kwatcher f:%s result: FAIL!, data size error\n", __func__);
			return size;
		}

		if (offset != 8 && WATCHER_BIT_LSB_CLR(comp_value, offset*8) != 0) {
			pr_err("kwatcher given compare data (0x%llx) is bigger than offset\n", WATCHER_BIT_LSB_CLR(comp_value, offset*8));
			pr_err("kwatcher f:%s result: FAIL!, compare data size error\n", __func__);
			return size;
		}

		index = kwatcher_alloc_internal(base, offset, wType, dType, 1, comp_op, comp_value);
	} else
		index = kwatcher_alloc_internal(base, offset, wType, dType, 1);

	pr_info("kwatcher index : %d\n", index);
	if (index == -1) {
		pr_err("kwatcher f:%s result: FAIL!, indexing error\n", __func__);
		return size;
	}

	ret = kwatcher_control(KWATCHER_INSTALL, index);
	if (ret)
		pr_err("kwatcher add result: FAIL!, index: %d, err: %d\n", index, ret);

	return size;
}

static ssize_t add_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t remove_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	char *str = (char *)buf;
	char *str1 = NULL;
	unsigned long long index = memparse(str, &str1);

	kwatcher_free_internal(index);

	return size;
}

static ssize_t remove_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t clear_hooktrace_buffer(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t size)
{
	int i;
	char *str = (char *)buf;
	char *str1 = NULL;
	unsigned long long index = memparse(str, &str1);

	if (index) {
		for (i = 0; i < KWATCHER_MAX_NUM ; i++)
			if (kwatcher.wpe_watcher[i].wp.hw.info.ctrl.enabled) {
				pr_info("kwatcher index:%d is enabled, so it doesn't clear the log buffer\n", i);
				return size;
			}

		memset(&kwatcher_stack_info, 0, sizeof(kwatcher_stack_info));
		pr_info("kwatcher hooktrace buffer has been cleard!\n");
	}

	return size;
}


static DEVICE_ATTR(testaccess, S_IRUGO | S_IWUSR | S_IWGRP,\
			testaccess_show, testaccess_store);

static DEVICE_ATTR(testbuffer, S_IRUGO | S_IWUSR | S_IWGRP,\
			testbuffer_show, testbuffer_store);

static DEVICE_ATTR(wplist, S_IRUGO,\
			wplist_show, wplist_store);

static DEVICE_ATTR(enable, S_IWUSR | S_IWGRP,\
			enable_show, enable_store);

static DEVICE_ATTR(disable, S_IWUSR | S_IWGRP,\
			disable_show, disable_store);

static DEVICE_ATTR(add, S_IWUSR | S_IWGRP,\
			add_show, add_store);

static DEVICE_ATTR(remove, S_IWUSR | S_IWGRP,\
			remove_show, remove_store);

static DEVICE_ATTR(hooktrace, S_IRUGO,\
			kwatcher_hooktrace_show, NULL);

static DEVICE_ATTR(clearhooktrace, S_IWUSR,\
			NULL, clear_hooktrace_buffer);

static DEVICE_ATTR(testrunalloc, S_IWUSR | S_IWGRP,\
			NULL, testrun_alloc_store);

static DEVICE_ATTR(testrun, S_IRUGO | S_IWUSR | S_IWGRP,\
			testrun_show, testrun_store);

#if SEC_KWATCHER_UNIT_TEST
static DEVICE_ATTR(unit_testrun, S_IRUGO | S_IWUSR | S_IWGRP,\
			unit_testrun_show, unit_testrun_store);
#endif

static struct device_attribute *sec_kwatcher_attrs[] = {
		&dev_attr_testaccess,
		&dev_attr_testbuffer,
		&dev_attr_wplist,
		&dev_attr_enable,
		&dev_attr_disable,
		&dev_attr_add,
		&dev_attr_remove,
		&dev_attr_hooktrace,
		&dev_attr_clearhooktrace,
		&dev_attr_testrunalloc,
		&dev_attr_testrun,
#if SEC_KWATCHER_UNIT_TEST
		&dev_attr_unit_testrun,
#endif
};

static void sec_kwatcher_sysfs_init(void)
{
	int ret;
	int i;
	sec_kwatcher_dev = sec_device_create(NULL, "sec_kwatcher");

	ret = IS_ERR(sec_kwatcher_dev);
	if (ret) {
		pr_err("kwatcher err f%s,l%d\n", __func__, __LINE__);

		return;
	}
	for (i = 0; i < ARRAY_SIZE(sec_kwatcher_attrs) ; i++) {
		ret = device_create_file(sec_kwatcher_dev, sec_kwatcher_attrs[i]);
		if (ret < 0) {
			pr_err("kwatcher err f%s,l%d\n", __func__, __LINE__);
		}
	}
}

static int __init sec_kwatcher_setup(char *str)
{
	char *str1 = NULL , *str2 = NULL, *str3 = NULL, *str4 = NULL;
	int i;

	if (str[0] != '0') {
		memset(&ksym_name, 0, KSYM_NAME_LEN);
		for (i = 0; i < KSYM_NAME_LEN; i++) {
			if (str[i] == '@')
				break;
			ksym_name[i] = str[i];
		}
		initBase = kallsyms_lookup_name(ksym_name);
		str2 = (char *)&str[i];
		kallsyms_lookup_size_offset(initBase, (unsigned long *)&initOffset, NULL);
	} else {
		initBase = memparse(str, &str1);
		initOffset = memparse(str1+1, &str2);
	}

	initType = memparse(str2+1, &str3);
	initDtype = memparse(str3+1, &str4);

	pr_err("kwatcher f:%s,l:%d base:%llx off:%llx type:%llx, dtype:%llx \n",
									__func__, __LINE__, initBase, initOffset, initType, initDtype);

	return 0;
}


__setup("sec_kwatcher=", sec_kwatcher_setup);


static int get_irqmask_status(struct pt_regs *regs)
{
	if (regs->pstate & IRQ_MASK_BIT)
		return 1;
	else
		return 0;
}

static void set_irqmask_status(struct pt_regs *regs, int status)
{
	if (status) {
		regs->pstate = regs->pstate | IRQ_MASK_BIT;
	} else {
		regs->pstate = regs->pstate & (~IRQ_MASK_BIT);
	}
}

void kwatcher_mask_and_save_irq_status(struct pt_regs *regs)
{
	struct kw_event_info *evt_info;

	preempt_disable();
	evt_info = this_cpu_ptr(&wevt_info);
	preempt_enable();


	if (get_irqmask_status(regs) == 0) {
		evt_info->force_imask = 1;
		set_irqmask_status(regs, 1);
	} else
		evt_info->force_imask = 0;
}

void kwtacher_restore_irq_status(struct pt_regs *regs)
{
	struct kw_event_info *evt_info;

	preempt_disable();
	evt_info = this_cpu_ptr(&wevt_info);
	preempt_enable();

	if (evt_info->force_imask) {
		evt_info->force_imask = 0;
		set_irqmask_status(regs, 0);
	}

}

extern void register_kwatcher_hook_handler(void(*) (unsigned long,
					  unsigned int, struct pt_regs *));

int is_in_bypass_section(struct context_info curr_context) {
	struct list_head *tmp;
	struct bypass_section_info *element;
	unsigned long flags;

	spin_lock_irqsave(&kwatcher.lock, flags);
	list_for_each(tmp, &kwatcher.bypass_list) {
		element = list_entry(tmp, struct bypass_section_info, list);
		if(is_same_context(element->context, curr_context)) {
			spin_unlock_irqrestore(&kwatcher.lock, flags);
			pr_info("kwatcher %s bypass\n", __func__);
			return 1;
		}
	}
	spin_unlock_irqrestore(&kwatcher.lock, flags);
	return 0;
}


/*
	description
		find a watchpoint whose address range includes a given address
	return
		NULL : not found
		Not NULL :
 *
 */

struct wpe_watcher *find_matched_watcher(unsigned long addr)
{
	int i;
	struct wpe_watcher *ret;

	ret = NULL;

	for (i = 0; i < KWATCHER_MAX_NUM ; i++) {
		if (kwatcher.wpe_watcher[i].base <= addr
			&& (kwatcher.wpe_watcher[i].base + kwatcher.wpe_watcher[i].offset > addr)) {
			ret = &kwatcher.wpe_watcher[i];
			break;
		}
	}

	return ret;
 }

/*
	description
		calculate destination register of memory access instrunction
		using given context(regs) and return value of the register.

		in case of store, you should call this function after executing
		store instruction in order to get meaninful data
*/
unsigned long long get_io_data(struct pt_regs *regs)
{
	int rt;

	rt = (int)((*(u64 *)((regs->pc))) & 0x1F);

	return regs->regs[rt];
}

/*
	description
		compare given io_data with saved value of watcher and check if condition is true or not.
		In case comparison operation is COMP_NONE, return true unconditionally.
*/
int hit_by_comparison(struct wpe_watcher *watcher, volatile unsigned long long io_data)
{
	int ret;

	ret = 1;

	if (watcher->comp_condition.comp_op == COMP_NONE)
		return 1;

	switch (watcher->comp_condition.comp_op) {
	case COMP_EQUAL:
		{
			ret = (io_data == watcher->comp_condition.comp_value) ? 1 : 0;
			pr_info("kwatcher compare saved:%llx with io_data:%llx, result:%d\n", watcher->comp_condition.comp_value, io_data , ret);
		}
		break;
	/*
		not implemented yet
	*/
	case COMP_GREATER:
		{
		}
		break;
	case COMP_LESS:
		{
		}
		break;
	default:
		pr_debug("kwatcher not defined comparison operation\n");
		break;
	}

	return ret;
}

int check_operation_is_write(void)
{
	unsigned int esr_el1;
	int is_write; /* read:0 write:1 */

	asm volatile("mrs %0, esr_el1":"=r"(esr_el1) : : );
	is_write = (esr_el1 & 0x40) ? 1 : 0;
	pr_info("kwatcher operation %d\n", is_write);

	return is_write;

}

static void sec_kwatcher_hook_handler_internal(struct wpe_watcher *watcher)
{
	int j;

	switch (watcher->dtype) {
	case 1: /*panic*/
		panic("kwatcher\n");
		break;
	case 2: /*halt*/
		preempt_disable();
		local_irq_disable();
		__asm__ __volatile__(
				"nop \n\t"
				"nop \n\t"
				"b . \n\t"
				"nop \n\t"
				"nop \n\t");
	case 3: /*logging*/
		dump_stack();
		break;
	case 4:	/* stack logging */
		preempt_disable();
		if (kstack_cnt >= HOOK_BUF_SIZE) {
			kstack_cnt = 0;
			kstack_ovf = 1;
		}
		kwatcher_stack_info[kstack_cnt].cpu = smp_processor_id();
		kwatcher_stack_info[kstack_cnt].time = cpu_clock(0);
		kwatcher_stack_info[kstack_cnt].base = watcher->base;
		kwatcher_stack_info[kstack_cnt].offset = watcher->offset;
		for (j = 0; j < KWATCHER_MAX_NUM; j++)
			kwatcher_stack_info[kstack_cnt].caller[j] = (void *)((size_t)return_address(j + 3));
		kstack_cnt++;
		preempt_enable();
		break;
	default:
		panic("kwatcher : should not be reached @here\n");
	}

}
#if SEC_KWATCHER_UNIT_TEST
unsigned long long last_hit_pc;
unsigned long long last_hit_addr;
unsigned long long last_hit_io_data;
#endif


static void remove_kwatcher(void)
{
	int index = 0 ;
	for (index = 0; index < KWATCHER_MAX_NUM; index++){
		if (kwatcher.wpe_watcher[index].base == 0x0) {
			pr_info("kwatcher index:%d is not installed\n", index);
		} else{
			pr_info("kwatcher CPU %d initiated uninstallation\n", smp_processor_id());
			arch_uninstall_hw_breakpoint(&(kwatcher.wpe_watcher[index].wp));
			kwatcher.wpe_watcher[index].enabled = 0;
		}
	}
}

void sec_kwatcher_hook_handler(unsigned long addr, unsigned int esr,
		struct pt_regs *regs)
{
	struct wpe_watcher *watcher;
	unsigned long long io_data;
	struct context_info curr_context;

	/*
		should mask irq so that irq exception cannot be taken
		before expected single step exception is taken
	*/
	if(panic_on_watching){
		remove_kwatcher();
		return ;
	}

	kwatcher_mask_and_save_irq_status(regs);

	watcher = find_matched_watcher(addr);

	if (watcher != NULL) {

		get_current_context(curr_context);

		if(is_in_bypass_section(curr_context))
			return;

		/* check 3 condition
			1. given watcher has comaparison condition
			2. a operation which caused watchpoint is write
			3. a io data which the operation has tried to write is
				same as given watcher's saved data
		*/
		io_data = get_io_data(regs);
		pr_info("kwatcher given io data is 0x%llx\n", io_data);

		if (watcher->comp_condition.comp_op == COMP_NONE ||
			(watcher->comp_condition.comp_op != COMP_NONE /* should follow this sequence */
			&& check_operation_is_write()
			&& hit_by_comparison(watcher, WATCHER_BIT_LSB_REMAIN(io_data, watcher->offset*8)))
		) {
#if SEC_KWATCHER_UNIT_TEST
			last_hit_pc = regs->pc;
			last_hit_addr = addr;
			last_hit_io_data = WATCHER_BIT_LSB_REMAIN(io_data, watcher->offset*8);
#endif
			sec_kwatcher_hook_handler_internal(watcher);
		}
	}
}

void kwatcher_trace_stop(void)
{
	panic_on_watching = 1;
}

int sec_kwatcher_alloc(unsigned long base, unsigned long size, enum watch_type wType, enum detect_type dType, ...)
{
	va_list args;
	enum comp_op_type op;
	unsigned long long value;
	int ret = 0;
	int index;

	va_start(args, dType);

	op = va_arg(args, int);
	value = va_arg(args, unsigned long long);

	pr_info("kwatcher value comparison op:%d, value:%llx", op, value);

	if (op != COMP_NONE)
		index = kwatcher_alloc_internal(base, size, wType, dType, 0, op, value);
	else
		index = kwatcher_alloc_internal(base, size, wType, dType, 0);

	if (index == -1)
		return -EINVAL;

	ret = kwatcher_control(KWATCHER_INSTALL, index);
	if (ret)
		pr_err("kwatcher f:%s result: FAIL!, index: %d, err: %d\n", __func__, index, ret);

	va_end(args);

	return ret;
}

int sec_kwatcher_free(int index)
{
	int ret = 0;
	ret = kwatcher_free_internal(index);
	return ret;
}

int sec_kwatcher_disable(int index)
{
	int ret = 0;

	ret = kwatcher_control(KWATCHER_DISABLE, index);
	if (ret)
		pr_err("kwatcher f:%s result: FAIL!, index: %d, err: %d\n", __func__, index, ret);

	return ret;
}

int sec_kwatcher_enable(int index)
{
	int ret = 0;

	ret = kwatcher_control(KWATCHER_ENABLE, index);
	if (ret)
		pr_err("kwatcher f:%s result: FAIL!, index: %d, err: %d\n", __func__, index, ret);

	return ret;
}

void sec_kwatcher_bypass_enter(void)
{
	struct bypass_section_info *element;
	unsigned long flags;

	element = kzalloc(sizeof(*element), GFP_KERNEL);

	get_current_context(element->context);

	pr_info("kwatcher %s current pid : %d, irq_count : %lu\n",
		__func__, element->context.pid, element->context.irq_count);

	local_dbg_disable();
	spin_lock_irqsave(&kwatcher.lock, flags);
	list_add(&element->list, &kwatcher.bypass_list);
	spin_unlock_irqrestore(&kwatcher.lock, flags);
	local_dbg_enable();
}

void sec_kwatcher_bypass_exit(void)
{
	struct list_head *node, *tmp;
	struct bypass_section_info *element;
	struct context_info current_context;
	unsigned long flags;

	get_current_context(current_context);

	local_dbg_disable();
	spin_lock_irqsave(&kwatcher.lock, flags);
	list_for_each_safe(node, tmp, &kwatcher.bypass_list) {
		element = list_entry(node, struct bypass_section_info, list);
		pr_info("kwatcher %s enter pid : %d irq_count : %lu, current pid : %d irq_count : %lu\n", 
			__func__, element->context.pid, element->context.irq_count, current->pid, in_interrupt());
		if(is_same_context(element->context, current_context)) {
			list_del(node);
			kfree(element);
		}
	}
	spin_unlock_irqrestore(&kwatcher.lock, flags);
	local_dbg_enable();
}

static int  sec_kwatcher_init(void)
{
	atomic_set(&(kwatcher_idx.wpe_idx), -1);

	register_kwatcher_hook_handler(sec_kwatcher_hook_handler);

	sec_kwatcher_sysfs_init();
	return 0;
}

static ssize_t sec_kwatcher_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	ssize_t count = 0;

	return count;
}

static ssize_t sec_kwatcher_write(struct file *file, const char __user *buf,
		size_t len, loff_t *offset)
{
	ssize_t count = 0;

	return count;
}

static const struct file_operations kwatcher_msg_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_kwatcher_read,
	.write = sec_kwatcher_write,
	.llseek = generic_file_llseek,
};

static int __init sec_kwatcher_late_init(void)
{
	struct proc_dir_entry *entry;
	int i, ret = 0;
	int index;

	if (sec_debug_get_debug_level() == 0)
		return 0;

	sec_kwatcher_init();

	entry = proc_create("kwatcher", S_IFREG | S_IRUGO, NULL,
			&kwatcher_msg_file_ops);

	if (!entry) {
		pr_err("kwatcher %s: failed to create proc entry\n", __func__);
		return 0;
	}

	proc_set_size(entry, 0x1000);

	for (i = 0; i < KWATCHER_MAX_NUM ; i++) {
		kwatcher.wpe_watcher[i].updateNeeded = alloc_percpu(unsigned int);
	}

	kwatcher.bp = alloc_percpu(struct perf_event);

	spin_lock_init(&kwatcher.lock);

	INIT_LIST_HEAD(&kwatcher.bypass_list);

	pm_qos_add_request(&kwatcher_min_pm_qos, PM_QOS_CPU_ONLINE_MIN, PM_QOS_CPU_ONLINE_MIN_DEFAULT_VALUE);

	if (initBase != 0 && initOffset != 0) {
		index = kwatcher_alloc_internal(initBase, initOffset, initType, initDtype, 1);
		if (index != -1) {
			ret = kwatcher_control(KWATCHER_INSTALL, index);
			if (ret)
				pr_err("kwatcher f:%s result: FAIL!, index: %d, err: %d\n", __func__, index, ret);
		}
	}

	return 0;
}

late_initcall(sec_kwatcher_late_init);
