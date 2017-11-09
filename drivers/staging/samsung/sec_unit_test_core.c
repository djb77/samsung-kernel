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


/* Unit Test LIST
	UNIT Test designers should add their unit_test structure
	statically here
*/
extern struct unit_test ut_watcher_config;
extern struct unit_test ut_watcher_hit;
extern struct unit_test ut_watcher_compare;


static struct unit_test *all_unit_test[SEC_UNIT_TEST_MAX] = {
	&ut_watcher_config,
	&ut_watcher_hit,
	&ut_watcher_compare,
};

struct unit_test *sec_unit_test_get_ut(char *ut_name)
{
	struct unit_test **iter;

	iter = all_unit_test;

	while (*iter != NULL) {
		if (strcmp((*iter)->unit_test_name, ut_name) == 0)
			return *iter;
		iter++;
	}

	return NULL;
}

/*
	run registered test suite
*/
void sec_unit_test_do_all_test(void)
{
	int unit_count, tc;

	pr_info("kwatcher_ut *******************\n");
	pr_info("kwatcher_ut all unit test start\n");
	pr_info("kwatcher_ut *******************\n");

	for (unit_count = 0; unit_count < SEC_UNIT_TEST_MAX; unit_count++) {
		struct unit_test *each_unit;

		each_unit = all_unit_test[unit_count];

		/* all unit test done, then exit */
		if (each_unit == NULL)
			break;

		pr_info("kwatcher_ut =================\n");
		pr_info("kwatcher_ut UNIT [%s] start\n", each_unit->unit_test_name);
		pr_info("kwatcher_ut =================\n");

		if (each_unit->init)
			each_unit->init(each_unit);

		for (tc = 0; tc < each_unit->testcase_num; tc++) {
			int ret;

			if (each_unit->setup)
				each_unit->setup(tc, each_unit);

			if (each_unit->execute_test == NULL)
				pr_err("kwatcher_ut wrong unit test\n");

			ret = each_unit->execute_test(tc, each_unit);

			if (ret != 0 && each_unit->stop_at_fail) {
				pr_err("kwatcher_ut unit test [%s] failed at tc %d\n",
					each_unit->unit_test_name, tc);
				break;
			}

			if (each_unit->finalize)
				each_unit->finalize(tc, each_unit);
		}

		if (each_unit->end)
			each_unit->end(each_unit);
	}

	pr_info("kwatcher_ut *******************\n");
	pr_info("kwatcher_ut all unit test done\n");
	pr_info("kwatcher_ut *******************\n");
}


