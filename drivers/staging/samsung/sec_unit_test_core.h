#ifndef SEC_UNIT_TEST_H
#define SEC_UNIT_TEST_H

#define	SEC_UNIT_TEST_MAX		100

struct unit_test {
	char *unit_test_name;
	int (*init)(struct unit_test *);
	int (*setup)(int, struct unit_test *);
	int (*execute_test)(int, struct unit_test *);
	void (*unit_func)(void);
	int (*finalize)(int, struct unit_test *);
	int (*end)(struct unit_test *);
	void *test_cases;
	int testcase_num;
	int stop_at_fail;
};

#define SEC_UT_ASSERT(tc, expr, stop_at_fail) \
	if (!(expr)) { \
		printk(KERN_ERR "kwatcher_ut [tc:%3d] FAILED! exp:%50s\n", tc, #expr); \
		if (stop_at_fail) \
			goto failed; \
	} else { \
		printk(KERN_ERR "kwatcher_ut [tc:%3d] PASSED! exp:%50s\n", tc, #expr); \
	}

#define SEC_UT_FAIL_EXIT		\
	failed: \
		return -1;

extern void sec_unit_test_do_all_test(void);
extern struct unit_test *sec_unit_test_get_ut(char *ut_name);

#endif
