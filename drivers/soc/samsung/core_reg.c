#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/kthread.h>

static DEFINE_MUTEX(core_reg_mutex);

enum armv8_core_type {
	A53_CORE = 1,
	MNGS_CORE,
	NOT_SUPPORTED,
};

#define MRS_ASM(func_name, reg_name) static inline u64 mrs_##func_name##_read(void) \
	{       \
		u64 val;        \
		asm volatile("mrs %0, "#reg_name : "=r"(val));  \
		return val;     \
	}

MRS_ASM(CPUACTLR, s3_1_c15_c2_0)
MRS_ASM(CPUECTLR, s3_1_c15_c2_1)
MRS_ASM(L2CTLR, s3_1_c11_c0_2)
MRS_ASM(L2ACTLR, s3_1_c15_c0_0)
MRS_ASM(L2ECTLR, s3_1_c11_c0_3)
MRS_ASM(MIDR, midr_el1)

MRS_ASM(LSMERR0SR, s3_1_c15_c2_1)
MRS_ASM(TBWMERR0SR, s3_1_c15_c2_2)
MRS_ASM(L2MERR0SR, s3_1_c15_c2_3)
MRS_ASM(LSACTLR, s3_1_c15_c4_3)
MRS_ASM(LSACTLR2, s3_1_c15_c5_3)
MRS_ASM(L2CTLR_EL1, s3_1_c11_c0_2)

static enum armv8_core_type get_core_type(void)
{
	u32 midr = 0;
	asm volatile("mrs %0, midr_el1" : "=r"(midr));

	if ((midr >> 24) == 0x53) {
		if ((midr & 0xfff0) == 0x10)
			return MNGS_CORE;
	} else if ((midr >> 24) == 0x41) {
		if ((midr & 0xfff0) == 0xD030)
			return A53_CORE;
	}

	return NOT_SUPPORTED;
}

static int ecc_dump_thread(void *data)
{
	enum armv8_core_type type = get_core_type();
	u64 val = 0L;

	if (type == NOT_SUPPORTED) {
		pr_info("core type is not defined\n");
		mutex_unlock(&core_reg_mutex);
		return 0;
	}

	if (type == A53_CORE) {
		val = mrs_CPUACTLR_read();
		pr_info("%20s: 0x%016llX\n", "CPUACTLR", val);

		val = mrs_CPUECTLR_read();
		pr_info("%20s: 0x%016llX\n", "CPUECTLR", val);

		val = mrs_L2CTLR_read();
		pr_info("%20s: 0x%016llX\n", "L2CTLR", val);

		val = mrs_L2ACTLR_read();
		pr_info("%20s: 0x%016llX\n", "L2ACTLR", val);

		val = mrs_L2ECTLR_read();
		pr_info("%20s: 0x%016llX\n", "L2ECTLR", val);
	} else {
		val = mrs_LSACTLR_read();
		pr_info("%20s: 0x%016llX\n", "LSACTLR", val);

		val = mrs_LSACTLR2_read();
		pr_info("%20s: 0x%016llX\n", "LSACTLR2", val);

		val = mrs_L2ACTLR_read();
		pr_info("%20s: 0x%016llX\n", "L2ACTLR", val);

		val = mrs_L2CTLR_EL1_read();
		pr_info("%20s: 0x%016llX\n", "L2CTLR_EL1", val);
	}

	val = mrs_LSMERR0SR_read();
	pr_info("%20s: 0x%016llX\n", "LSMERR0SR", val);

	val = mrs_TBWMERR0SR_read();
	pr_info("%20s: 0x%016llX\n", "TBWMERR0SR", val);

	val = mrs_L2MERR0SR_read();
	pr_info("%20s: 0x%016llX\n", "L2MERR0SR", val);

	mutex_unlock(&core_reg_mutex);

	return 0;
}

static int dump_ecc_status(struct seq_file *s, void *data)
{
	int core = 0;

	for_each_cpu(core, cpu_online_mask) {
		static struct task_struct *task = NULL;

		task = kthread_create(ecc_dump_thread, NULL, "\tCore%u Status", core);
		if (IS_ERR(task)) {
			pr_err("kthread_create failed\n");
			return -ENOMEM;
		}
		mutex_lock(&core_reg_mutex);
		kthread_bind(task, core);
		pr_info("Core%d\n", core);
		wake_up_process(task);
	}

	return 0;
}

static int open_ecc_dump(struct inode *inode, struct file *file)
{
	return single_open(file, dump_ecc_status, inode->i_private);
}

static struct file_operations ops_dump_ecc_status = {
	.open = open_ecc_dump,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init core_reg_init(void)
{
	struct dentry *root, *d;

	root = debugfs_create_dir("core_reg", NULL);
	if (!root) {
		pr_err("%s: couln't create debugfs\n", __FILE__);
		return -ENOMEM;
	}

	d = debugfs_create_file("core_status", S_IRUGO, root, NULL,
				&ops_dump_ecc_status);
	if (!d)
		return -ENOMEM;

	return 0;
}
late_initcall_sync(core_reg_init)
