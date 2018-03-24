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
extern unsigned long long last_hit_addr;

/********************************************
 unit_test: basic configuration test
*********************************************/

#define		UT_WATCHER_HIT_TC_NUM		14

struct ut_watcher_hit_tc {
	/* 0:simple hit, 1:range hit, 2:  */
	int tc_type;
	/* input */
	u64 base;
	int offset;
	int rw;
	int dtype;
	int test_offset;
};

struct ut_watcher_hit_tc ut_watcher_hit_testcases[] = {

	/* simple hit */
	{.tc_type = 0, .offset = 2, .rw = 3, .dtype = 3, .test_offset=0},
	{.tc_type = 0, .offset = 2, .rw = 3, .dtype = 3, .test_offset=1},

	{.tc_type = 0, .offset = 4, .rw = 3, .dtype = 3, .test_offset=0},
	{.tc_type = 0, .offset = 4, .rw = 3, .dtype = 3, .test_offset=1},
	{.tc_type = 0, .offset = 4, .rw = 3, .dtype = 3, .test_offset=2},
	{.tc_type = 0, .offset = 4, .rw = 3, .dtype = 3, .test_offset=3},

	{.tc_type = 0, .offset = 8, .rw = 3, .dtype = 3, .test_offset=0},
	{.tc_type = 0, .offset = 8, .rw = 3, .dtype = 3, .test_offset=1},
	{.tc_type = 0, .offset = 8, .rw = 3, .dtype = 3, .test_offset=2},
	{.tc_type = 0, .offset = 8, .rw = 3, .dtype = 3, .test_offset=3},
	{.tc_type = 0, .offset = 8, .rw = 3, .dtype = 3, .test_offset=4},
	{.tc_type = 0, .offset = 8, .rw = 3, .dtype = 3, .test_offset=5},
	{.tc_type = 0, .offset = 8, .rw = 3, .dtype = 3, .test_offset=6},
	{.tc_type = 0, .offset = 8, .rw = 3, .dtype = 3, .test_offset=7},

	/* range hit */
	{.tc_type = 0, .offset = 16, .rw = 3, .dtype = 3, .test_offset=7},
	{.tc_type = 0, .offset = 16, .rw = 3, .dtype = 3, .test_offset=15},
	{.tc_type = 0, .offset = 256, .rw = 3, .dtype = 3, .test_offset=7},
	{.tc_type = 0, .offset = 256, .rw = 3, .dtype = 3, .test_offset=255},
	{.tc_type = 0, .offset = 1024, .rw = 3, .dtype = 3, .test_offset=7},
	{.tc_type = 0, .offset = 1024, .rw = 3, .dtype = 3, .test_offset=1023},
	{.tc_type = 0, .offset = 65536, .rw = 3, .dtype = 3, .test_offset=7},
	{.tc_type = 0, .offset = 65536, .rw = 3, .dtype = 3, .test_offset=65535},

	/* not-hit */
	{.tc_type = 1, .offset = 4, .rw = 3, .dtype = 3, .test_offset=-1},
	{.tc_type = 1, .offset = 4, .rw = 3, .dtype = 3, .test_offset=4},
	{.tc_type = 1, .offset = 8, .rw = 3, .dtype = 3, .test_offset=-1},
	{.tc_type = 1, .offset = 8, .rw = 3, .dtype = 3, .test_offset=8},
	{.tc_type = 1, .offset = 16, .rw = 3, .dtype = 3, .test_offset=-1},
	{.tc_type = 1, .offset = 16, .rw = 3, .dtype = 3, .test_offset=16},
};

/* this function is called once at start of unit test
	I recommend you to organize test cases you want to test
*/
static int ut_watcher_hit_init(struct unit_test *ut)
{
	return 0;
}

/* this function is called at every test case
	so you can do what you want to do on each test case
*/
static int ut_watcher_hit_setup(int tc_num, struct unit_test *ut)
{
	struct ut_watcher_hit_tc *tc;

	tc = ut->test_cases;

	tc[tc_num].base = (unsigned long)kmalloc(tc[tc_num].offset, GFP_KERNEL);
	last_hit_addr = 0x0;
	

	return 0;
}

/* this function runs actual unit function
	unit_test->unit_func is kind of a general type, so you should
	cast it to a type of a function you want to test actually
	,refering to actual unit function
*/
static int ut_watcher_hit_execute(int tc_num, struct unit_test *ut)
{

	struct ut_watcher_hit_tc *tc;
	int (*target_func)(unsigned long base, unsigned long size, enum watch_type wType, enum detect_type dType, ...);
	volatile char *watching_area;
	volatile char dummy;

	tc = ut->test_cases;

	/* watcher allocation */
	target_func = (int (*)(unsigned long base, unsigned long size, enum watch_type wType, enum detect_type dType, ...))ut->unit_func;
	target_func(tc[tc_num].base, tc[tc_num].offset, tc[tc_num].rw, tc[tc_num].dtype);


	/* try to access memory under watch */
	watching_area = ((char *)tc[tc_num].base)+tc[tc_num].test_offset;

	switch (tc[tc_num].tc_type) {
	case 0:
		*watching_area = 'A';
		SEC_UT_ASSERT(tc_num,
			(last_hit_addr >= tc[tc_num].base && last_hit_addr <tc[tc_num].base + tc[tc_num].offset),
			ut->stop_at_fail);
		break;
	case 1:
		dummy = *watching_area;
		SEC_UT_ASSERT(tc_num, last_hit_addr == 0, ut->stop_at_fail);
		break;
	}

	return 0;

	SEC_UT_FAIL_EXIT
}

/* thif function is called at end of each test case
	if you hav any resource alloced at setup step,
	you can free it here
*/
static int ut_watcher_hit_finalize(int tc_num, struct unit_test *ut)
{
	struct ut_watcher_hit_tc *tc;

	tc = ut->test_cases;

	kfree((unsigned long long *)tc[tc_num].base);
	sec_kwatcher_free(0);

	return 0;
}


struct unit_test ut_watcher_hit = {
	.unit_test_name = "watcher basic hit test",
	.init = ut_watcher_hit_init,
	.setup = ut_watcher_hit_setup,
	.execute_test = ut_watcher_hit_execute,
	.finalize = ut_watcher_hit_finalize,
	.test_cases = ut_watcher_hit_testcases,
	.testcase_num = sizeof(ut_watcher_hit_testcases)/sizeof(struct ut_watcher_hit_tc),
	.stop_at_fail = 1,
};

