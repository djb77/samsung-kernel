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
extern unsigned long long last_hit_io_data;

/********************************************
 unit_test: basic configuration test
*********************************************/

#define		UT_WATCHER_HIT_TC_NUM		14

struct ut_watcher_compare_tc {
	/* 0:simple hit, 1:range hit, 2:  */
	int tc_type;
	/* input */
	u64 base;
	int offset;
	int rw;
	int dtype;
	int comp_op;
	u64 comp_data;
	u64 test_data;

};

struct ut_watcher_compare_tc ut_watcher_compare_testcases[] = {
	/* hit */
	{.tc_type = 0, .offset = 1, .rw = 3, .dtype = 3, .comp_data = 0x12, .test_data = 0x12},
	{.tc_type = 0, .offset = 1, .rw = 3, .dtype = 3, .comp_data = 0x12, .test_data = 0x3412},
	{.tc_type = 0, .offset = 2, .rw = 3, .dtype = 3, .comp_data = 0x3456, .test_data = 0x3456},
	{.tc_type = 0, .offset = 2, .rw = 3, .dtype = 3, .comp_data = 0x3456, .test_data = 0x123456},
	{.tc_type = 0, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0x12345678, .test_data = 0x12345678},
	{.tc_type = 0, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0x12345678, .test_data = 0xCD12345678},
	{.tc_type = 0, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0x12345678ABCDEF12, .test_data = 0x12345678ABCDEF12},

	/* miss */
	{.tc_type = 1, .offset = 1, .rw = 3, .dtype = 3, .comp_data = 0x12, .test_data = 0x34},
	{.tc_type = 1, .offset = 2, .rw = 3, .dtype = 3, .comp_data = 0x3456, .test_data = 0x3458},
	{.tc_type = 1, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0x12345678, .test_data = 0x12AB5678},
	{.tc_type = 1, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0xAB123456, .test_data = 0xCD123456},
	{.tc_type = 1, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0x123456AB, .test_data = 0x123456CD},
	{.tc_type = 1, .offset = 8, .rw = 3, .dtype = 3, .comp_data = 0xAB345678ABCDEF12, .test_data = 0x12345678ABCDEF12},
	{.tc_type = 1, .offset = 8, .rw = 3, .dtype = 3, .comp_data = 0x12345678ABCDEFAB, .test_data = 0x12345678ABCDEFCD},

#if 0 /* implementation of a function to test is not done */
	/* not set */
	{.tc_type = 2, .offset = 1, .rw = 3, .dtype = 3, .comp_data = 0x1234, .test_data = 0x34},
	{.tc_type = 2, .offset = 1, .rw = 3, .dtype = 3, .comp_data = 0x8012, .test_data = 0x3412},
	{.tc_type = 2, .offset = 1, .rw = 3, .dtype = 3, .comp_data = 0x800012, .test_data = 0x3412},
	{.tc_type = 2, .offset = 2, .rw = 3, .dtype = 3, .comp_data = 0x123456, .test_data = 0x783456},
	{.tc_type = 2, .offset = 2, .rw = 3, .dtype = 3, .comp_data = 0x12003456, .test_data = 0x3456},
	{.tc_type = 2, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0xCDAB12345678, .test_data = 0x34},
	{.tc_type = 2, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0xAB0012345678, .test_data = 0x34},
	{.tc_type = 2, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0xAB000012345678, .test_data = 0x34},
	{.tc_type = 2, .offset = 4, .rw = 3, .dtype = 3, .comp_data = 0x8000000012345678, .test_data = 0x34},
#endif
};

/* this function is called once at start of unit test
	I recommend you to organize test cases you want to test
*/
static int ut_watcher_compare_init(struct unit_test *ut)
{
	return 0;
}

/* this function is called at every test case
	so you can do what you want to do on each test case
*/
static int ut_watcher_compare_setup(int tc_num, struct unit_test *ut)
{
	struct ut_watcher_compare_tc *tc;

	tc = ut->test_cases;

	tc[tc_num].base = (unsigned long)kmalloc(tc[tc_num].offset, GFP_KERNEL);
	tc[tc_num].comp_op = COMP_EQUAL;

	last_hit_addr = 0x0;
	last_hit_io_data = 0x0;
	

	return 0;
}

/* this function runs actual unit function
	unit_test->unit_func is kind of a general type, so you should
	cast it to a type of a function you want to test actually
	,refering to actual unit function
*/
static int ut_watcher_compare_execute(int tc_num, struct unit_test *ut)
{

	struct ut_watcher_compare_tc *tc;
	int (*target_func)(unsigned long base, unsigned long size, enum watch_type wType, enum detect_type dType, ...);
	volatile u64 *watching_area;

	tc = ut->test_cases;

	/* watcher allocation */
	target_func = (int (*)(unsigned long base, unsigned long size, enum watch_type wType, enum detect_type dType, ...))ut->unit_func;
	target_func(tc[tc_num].base, tc[tc_num].offset, tc[tc_num].rw, tc[tc_num].dtype, tc[tc_num].comp_op, tc[tc_num].comp_data);


	/* try to access memory under watch */
	watching_area = (u64 *)tc[tc_num].base;
	*watching_area = tc[tc_num].test_data;

	switch (tc[tc_num].tc_type) {
	case 0:
		SEC_UT_ASSERT(tc_num,
			(last_hit_addr == tc[tc_num].base)
			&& (last_hit_io_data == kwatcher.wpe_watcher[0].comp_condition.comp_value),
			ut->stop_at_fail);
		break;
	case 1:
		SEC_UT_ASSERT(tc_num,
			(last_hit_addr == 0x0) && (last_hit_io_data == 0x0),
			ut->stop_at_fail);
		break;
	case 2:
		SEC_UT_ASSERT(tc_num,
			(kwatcher.wpe_watcher[0].inUse == 0),
			ut->stop_at_fail);
		break;
	}

	return 0;

failed:

	switch (tc[tc_num].tc_type) {
	case 0:
	case 1:
		pr_err("kwatcher_ut base:0x%llx, addr:0x%llx, value:0x%llx, io_data:0x%llx, \n",
			tc[tc_num].base, last_hit_addr, last_hit_io_data,
			kwatcher.wpe_watcher[0].comp_condition.comp_value);
		break;
	case 2:
		pr_err("kwatcher_ut base:0x%llx, inUse:0x%d\n",tc[tc_num].base, kwatcher.wpe_watcher[0].inUse);
		break;
	}

	return -1;

}

/* thif function is called at end of each test case
	if you hav any resource alloced at setup step,
	you can free it here
*/
static int ut_watcher_compare_finalize(int tc_num, struct unit_test *ut)
{
	struct ut_watcher_compare_tc *tc;

	tc = ut->test_cases;

	kfree((unsigned long long *)tc[tc_num].base);
	sec_kwatcher_free(0);

	return 0;
}


struct unit_test ut_watcher_compare = {
	.unit_test_name = "watcher compare test",
	.init = ut_watcher_compare_init,
	.setup = ut_watcher_compare_setup,
	.execute_test = ut_watcher_compare_execute,
	.finalize = ut_watcher_compare_finalize,
	.test_cases = ut_watcher_compare_testcases,
	.testcase_num = sizeof(ut_watcher_compare_testcases)/sizeof(struct ut_watcher_compare_tc),
	.stop_at_fail = 1,
};

