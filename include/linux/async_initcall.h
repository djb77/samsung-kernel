/* 
 * These macros are used to make any init function run in parallel.
 * Any initcall function marked "initcall_async" is going to run on a
 * work-queue. For more details, please refer "linux/initcall.h".
 * Note : It doesn't support MODULE.
 */

#ifndef MODULE

#include <linux/init.h>
#include <linux/async.h>

#define __define_initcall_async(fn, id) \
	static void __init fn##_async(void *dummy, async_cookie_t c)	\
	{								\
		fn();							\
		return;							\
	}								\
	static int __init _##fn(void)					\
	{								\
		async_schedule(fn##_async, NULL);			\
		return 0;						\
	}								\
	__define_initcall(_##fn, id)

#define pure_initcall_async(fn)		__define_initcall_async(fn, 0)
#define core_initcall_async(fn)		__define_initcall_async(fn, 1)
#define postcore_initcall_async(fn)	__define_initcall_async(fn, 2)
#define arch_initcall_async(fn)		__define_initcall_async(fn, 3)
#define subsys_initcall_async(fn)	__define_initcall_async(fn, 4)
#define fs_initcall_async(fn)		__define_initcall_async(fn, 5)
#define device_initcall_async(fn)	__define_initcall_async(fn, 6)
#define late_initcall_async(fn)		__define_initcall_async(fn, 7)

#define module_init_async(fn)		device_initcall_async(fn)

#endif //MODULE
