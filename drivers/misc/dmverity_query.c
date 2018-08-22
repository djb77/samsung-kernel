#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/highmem.h>

#define	CMD_READ_SYSTEM_IMAGE_CHECK_STATUS 3

static inline u64 exynos_smc_verity(u64 cmd, u64 arg1, u64 arg2, u64 arg3)
{
	register u64 reg0 __asm__("x0") = cmd;
	register u64 reg1 __asm__("x1") = arg1;
	register u64 reg2 __asm__("x2") = arg2;
	register u64 reg3 __asm__("x3") = arg3;

	__asm__ volatile (
		"dsb    sy\n"
		"smc    0\n"
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
	);

	return reg0;
}


static int verity_scm_call(void)
{
	return exynos_smc_verity(0x83000006, CMD_READ_SYSTEM_IMAGE_CHECK_STATUS, 0, 0);
}

#define DRIVER_DESC   "Read whether odin flash succeeded"

ssize_t	dmverity_read(struct file *filep, char __user *buf, size_t size, loff_t *offset)
{
	uint32_t	odin_flag;
	//int ret;

	/* First check is to get rid of integer overflow exploits */
	if (size < sizeof(uint32_t)) {
		pr_err("Size must be atleast %d\n", (int)sizeof(uint32_t));
		return -EINVAL;
	}

	odin_flag = verity_scm_call();
	pr_info("dmverity: odin flag: %x\n", odin_flag);

	if (copy_to_user(buf, &odin_flag, sizeof(uint32_t))) {
		pr_err("Copy to user failed\n");
		return -1;
	} else
		return sizeof(uint32_t);
}

static const struct file_operations dmverity_proc_fops = {
	.read		= dmverity_read,
};

/**
 *      dmverity_odin_flag_read_init -  Initialization function for DMVERITY
 *
 *      It creates and initializes dmverity proc entry with initialized read handler
 */
static int __init dmverity_odin_flag_read_init(void)
{
	//extern int boot_mode_recovery;
	if (/* boot_mode_recovery == */ 1) {
		/* Only create this in recovery mode. Not sure why I am doing this */
		if (proc_create("dmverity_odin_flag", 0644, NULL, &dmverity_proc_fops) == NULL) {
			pr_err("%s: Error creating proc entry\n", __func__);
			goto error_return;
		}
		pr_info("%s:: Registering /proc/dmverity_odin_flag Interface \n", __func__);
	} else {
		pr_info("%s:: not enabling in non-recovery mode\n", __func__);
		goto error_return;
	}

	return 0;

error_return:
	return -1;
}

/**
 *      dmverity_odin_flag_read_exit -  Cleanup Code for DMVERITY
 *
 *      It removes /proc/dmverity proc entry and does the required cleanup operations
 */
static void __exit dmverity_odin_flag_read_exit(void)
{
	remove_proc_entry("dmverity_odin_flag", NULL);
	pr_info("Deregistering /proc/dmverity_odin_flag interface\n");
}

module_init(dmverity_odin_flag_read_init);
module_exit(dmverity_odin_flag_read_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
