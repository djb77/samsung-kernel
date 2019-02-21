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

#include "sec_unit_test_core.h"

extern struct kwatcher_group kwatcher;


/********************************************
 unit_test: basic configuration test
*********************************************/

#define		UT_WATCHER_CONFIG_TC_NUM		1

struct ut_watcher_config_tc {
	/* input */
	u64 base;
	int offset;
	int rw;
	int dtype;
	/* output */
	struct wpe_watcher watcher;
};

struct ut_watcher_config_tc ut_watcher_config_testcases[UT_WATCHER_CONFIG_TC_NUM] = {
};

static int ut_watcher_config_init(struct unit_test *ut)
{
	struct ut_watcher_config_tc *tc;

	/* this function is called once at start of unit test
		I recommend you to organize test cases you want to test
	*/

	tc = ut->test_cases;

	tc[0].offset = 0x8;
	tc[0].rw = 0x3;
	tc[0].dtype = 0x3;

	tc[0].watcher.wp.hw.info.ctrl.len       = 8;
	tc[0].watcher.wp.hw.info.ctrl.mask     = 0;
	tc[0].watcher.wp.hw.info.ctrl.type      = 0;
	tc[0].watcher.wp.hw.info.ctrl.privilege = 0;
	tc[0].watcher.inUse	= 1;
	tc[0].watcher.base   = 0;
	tc[0].watcher.offset   = 0x8;
	tc[0].watcher.wp.hw.info.ctrl.enabled   = 0;
	tc[0].watcher.wp.hw.info.address        = 0;
	tc[0].watcher.dtype = 0x3;
	tc[0].watcher.comp_condition.comp_op = COMP_NONE;
	tc[0].watcher.comp_condition.comp_value = 0;

	return 0;
}

static int ut_watcher_config_setup(int tc_num, struct unit_test *ut)
{
	struct ut_watcher_config_tc *tc;

	/* this function is called at every test case
		so you can do what you want to do on each test case
	*/
	tc = ut->test_cases;

	tc[tc_num].base = (unsigned long)kmalloc(tc[tc_num].offset, GFP_KERNEL);
	tc[tc_num].watcher.base = tc[tc_num].base;


	return 0;
}

static int ut_watcher_config_execute(int tc_num, struct unit_test *ut)
{

	struct ut_watcher_config_tc *tc;
	int (*target_func)(unsigned long base, unsigned long size, enum watch_type wType, enum detect_type dType, ...);

	/* this function runs actual unit function
		unit_test->unit_func is kind of a general type, so you should
		cast it to a type of a function you want to test actually
		,refering to actual unit function
	*/

	tc = ut->test_cases;

	target_func = (int (*)(unsigned long base, unsigned long size, enum watch_type wType, enum detect_type dType, ...))ut->unit_func;
	target_func(tc[tc_num].base, tc[tc_num].offset, tc[tc_num].rw, tc[tc_num].dtype);

	SEC_UT_ASSERT(tc_num, (tc[tc_num].base == kwatcher.wpe_watcher[0].base), ut->stop_at_fail);
	SEC_UT_ASSERT(tc_num, (tc[tc_num].offset == kwatcher.wpe_watcher[0].offset), ut->stop_at_fail);

	return 0;

	SEC_UT_FAIL_EXIT
}

static int ut_watcher_config_finalize(int tc_num, struct unit_test *ut)
{
	/* thif function is called at end of each test case
		if you hav any resource alloced at setup step,
		you can free it here
	*/
	struct ut_watcher_config_tc *tc;

	tc = ut->test_cases;

	sec_kwatcher_free(0);
	kfree((unsigned long long *)tc[tc_num].base);


	return 0;
}


struct unit_test ut_watcher_config = {
	.unit_test_name = "watcher basic config test",
	.init = ut_watcher_config_init,
	.setup = ut_watcher_config_setup,
	.execute_test = ut_watcher_config_execute,
	.finalize = ut_watcher_config_finalize,
	.test_cases = ut_watcher_config_testcases,
	.testcase_num = UT_WATCHER_CONFIG_TC_NUM,
	.stop_at_fail = 1,
};

