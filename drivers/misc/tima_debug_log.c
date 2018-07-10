#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/bootmem.h>

#ifdef CONFIG_NO_BOOTMEM
#include <linux/memblock.h>
#endif

#ifdef CONFIG_RKP
#include <linux/rkp_entry.h>
#endif

#define	SECURE_LOG	0
#define	DEBUG_LOG	1

#ifdef CONFIG_RKP
#define DEBUG_RKP_LOG_START	RKP_LOG_START
#endif
#define	DEBUG_LOG_SIZE	(1<<20)
#define	DEBUG_LOG_MAGIC	(0xaabbccdd)
#define	DEBUG_LOG_ENTRY_SIZE	128

typedef struct debug_log_entry_s
{
	uint32_t	timestamp;          /* timestamp at which log entry was made*/
	uint32_t	logger_id;          /* id is 1 for tima, 2 for lkmauth app  */
#define	DEBUG_LOG_MSG_SIZE	(DEBUG_LOG_ENTRY_SIZE - sizeof(uint32_t) - sizeof(uint32_t))
	char	log_msg[DEBUG_LOG_MSG_SIZE];      /* buffer for the entry                 */
} __attribute__ ((packed)) debug_log_entry_t;

typedef struct debug_log_header_s
{
	uint32_t	magic;              /* magic number                         */
	uint32_t	log_start_addr;     /* address at which log starts          */
	uint32_t	log_write_addr;     /* address at which next entry is written*/
	uint32_t	num_log_entries;    /* number of log entries                */
	char	padding[DEBUG_LOG_ENTRY_SIZE - 4 * sizeof(uint32_t)];
} __attribute__ ((packed)) debug_log_header_t;

#define DRIVER_DESC   "A kernel module to read tima debug log"

unsigned long *tima_log_addr = 0;
unsigned long *tima_debug_log_addr = 0;
unsigned long *tima_secure_log_addr = 0;
#ifdef CONFIG_RKP
unsigned long *tima_debug_rkp_log_addr = 0;
#endif

unsigned long tima_debug_logging_start = 0;

static int __init tima_log_setup(char *str)
{
	unsigned size = memparse(str, &str);
	unsigned long base = 0;
	/* If we encounter any problem parsing str ... */
	if (!size || size != roundup_pow_of_two(size) || *str != '@'
		|| kstrtoul(str + 1, 0, &base))
			goto out;

#ifdef CONFIG_NO_BOOTMEM
	if (memblock_is_region_reserved(base, size) ||
		memblock_reserve(base, size)) {
#else
	if (reserve_bootmem(base , size, BOOTMEM_EXCLUSIVE)) {
#endif
			pr_err("%s: failed reserving size %d " \
						"at base 0x%lx\n", __func__, size, base);
			goto out;
	}
	pr_info("tima :%s, base:%lx, size:%x \n", __func__,base, size);
	
	tima_debug_logging_start = base;

	return 1;
out:
	return 0;
}
__setup("tima_log=", tima_log_setup);

ssize_t	tima_read(struct file *filep, char __user *buf, size_t size, loff_t *offset)
{
	/* First check is to get rid of integer overflow exploits */
	if (size > DEBUG_LOG_SIZE || (*offset) + size > DEBUG_LOG_SIZE) {
		printk(KERN_ERR"Extra read\n");
		return -EINVAL;
	}

	if (!strcmp(filep->f_path.dentry->d_iname, "tima_secure_log"))
		tima_log_addr = tima_secure_log_addr;
	else if( !strcmp(filep->f_path.dentry->d_iname, "tima_debug_log"))
		tima_log_addr = tima_debug_log_addr;

#ifdef CONFIG_RKP
	else if (!strcmp(filep->f_path.dentry->d_iname, "rkp_log")) {
		if (*offset >= RKP_LOG_SIZE)
			return -EINVAL;
		else if (*offset + size > RKP_LOG_SIZE)
			size = (RKP_LOG_SIZE) - *offset;

		tima_log_addr = tima_debug_rkp_log_addr;
	}
#endif
	if (copy_to_user(buf, (const char *)tima_log_addr + (*offset), size)) {
		printk(KERN_ERR"Copy to user failed\n");
		return -1;
	} else {
		*offset += size;
		return size;
	}
}

static const struct file_operations tima_proc_fops = {
	.read		= tima_read,
};

/**
 *      tima_debug_log_read_init -  Initialization function for TIMA
 *
 *      It creates and initializes tima proc entry with initialized read handler 
 */
static int __init tima_debug_log_read_init(void)
{
	unsigned long tima_secure_logging_start = 0;
	tima_secure_logging_start = tima_debug_logging_start + DEBUG_LOG_SIZE;
	
	if (proc_create("tima_debug_log", 0644,NULL, &tima_proc_fops) == NULL) {
		printk(KERN_ERR"tima_debug_log_read_init: Error creating proc entry\n");
		goto error_return;
	}
	if (proc_create("tima_secure_log", 0644,NULL, &tima_proc_fops) == NULL) {
		printk(KERN_ERR"tima_secure_log_read_init: Error creating proc entry\n");
		goto remove_debug_entry;
	}
	printk(KERN_INFO"tima_debug_log_read_init: Registering /proc/tima_debug_log Interface \n");

	tima_debug_log_addr = (unsigned long *)phys_to_virt(tima_debug_logging_start);
	tima_secure_log_addr = (unsigned long *)phys_to_virt(tima_secure_logging_start);
#ifdef CONFIG_RKP
	tima_debug_rkp_log_addr  = (unsigned long *)phys_to_virt(DEBUG_RKP_LOG_START);
#endif
	return 0;

remove_debug_entry:
	remove_proc_entry("tima_debug_log", NULL);
error_return:
	return -1;
}

/**
 *      tima_debug_log_read_exit -  Cleanup Code for TIMA
 *
 *      It removes /proc/tima proc entry and does the required cleanup operations 
 */
static void __exit tima_debug_log_read_exit(void)
{
	remove_proc_entry("tima_debug_log", NULL);
	remove_proc_entry("tima_secure_log", NULL);

	printk(KERN_INFO"Deregistering /proc/tima_debug_log Interface\n");
}

module_init(tima_debug_log_read_init);
module_exit(tima_debug_log_read_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
