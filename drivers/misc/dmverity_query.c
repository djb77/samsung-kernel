#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/highmem.h>
#include <linux/knox_kap.h>
#include "qseecom_kernel.h"

static int verity_lksecapp_call(void)
{
	int ret = -1 ;
	int err = -1 ;
	int dmv_req_len = 0 ;
	int dmv_rsp_len = 0 ;
	dmv_req_t *dmv_req = NULL;
	dmv_rsp_t *dmv_rsp = NULL;
	struct qseecom_handle *qhandle_dmv = NULL;  

  	printk(KERN_ERR" %s -> qhandle address : %p \n", __FUNCTION__, qhandle_dmv);
  	ret = qseecom_start_app((&qhandle_dmv),"lksecapp", QSEECOM_SBUFF_SIZE);
	if (ret) {
		printk(KERN_ERR" %s -> qseecom_start_app failed %d\n", __FUNCTION__, ret);
		goto err_ret;
	}
	printk(KERN_ERR" %s -> qseecom_start_app succeeded. %d\n", __FUNCTION__, ret);
	dmv_req = (dmv_req_t *)qhandle_dmv->sbuf;
	printk(KERN_ERR" %s -> dmv_req address : %p . \n", __FUNCTION__, dmv_req);
	dmv_req_len = sizeof(dmv_req_t);
	dmv_req->cmd_id = SEC_BOOT_CMD101_GET_DMV_DATA ; 	
	
	dmv_rsp = (dmv_rsp_t *)(qhandle_dmv->sbuf + dmv_req_len);
	dmv_rsp_len = sizeof(dmv_rsp_t);

	err = qseecom_send_command(qhandle_dmv, dmv_req, dmv_req_len, dmv_rsp, dmv_rsp_len);

	printk(KERN_ERR" %s -> read odin_flag %d\n", __FUNCTION__, dmv_req->data.dmv_bl_status.odin_flag);
	if (err) {
		printk(KERN_ERR" %s -> Failed to send command : %d\n", __FUNCTION__, err);
		goto err_ret;
	}

	return dmv_req->data.dmv_bl_status.odin_flag ;	
	
err_ret:
	printk(KERN_ERR" %s -> sending command to tz app failed with error %d, shutting down %s\n", __FUNCTION__, err, "lksecapp");
	return err;	
}

#define DRIVER_DESC   "Read whether odin flash succeeded"


#if 0
ssize_t	dmverity_read(struct file *filep, char __user *buf, size_t size, loff_t *offset)
{
	uint32_t	odin_flag;
	//int ret;

	/* First check is to get rid of integer overflow exploits */
	if (size < sizeof(uint32_t)) {
		printk(KERN_ERR"Size must be atleast %d\n",(int)sizeof(uint32_t));
		return -EINVAL;
	}

	odin_flag = verity_lksecapp_call();
	printk(KERN_INFO"dmverity: odin flag: %x\n", odin_flag);

	if (copy_to_user(buf, &odin_flag, (int)sizeof(uint32_t))) {
		printk(KERN_ERR"Copy to user failed\n");
		return -1;
	} else
		return (int)sizeof(uint32_t);
}
#endif

static int dmverity_read(struct seq_file *m, void *v){
	int odin_flag = 0;
	unsigned char ret_buffer[10];
	odin_flag = verity_lksecapp_call();

	memset(ret_buffer, 0, sizeof(ret_buffer));
	snprintf(ret_buffer, sizeof(ret_buffer), "%08x\n", odin_flag);
	seq_write(m, ret_buffer, sizeof(ret_buffer));
	printk(KERN_INFO"dmverity: odin_flag: %x\n", odin_flag);
	return 0;
}
static int dmverity_open(struct inode *inode, struct file *filep){
	return single_open(filep, dmverity_read, NULL);
}

static const struct file_operations dmverity_proc_fops = {
	.open       = dmverity_open,
	.read	    = seq_read,
	
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
      		if (proc_create("dmverity_odin_flag", 0644,NULL, &dmverity_proc_fops) == NULL) {
				printk(KERN_ERR"dmverity_odin_flag_read_init: Error creating proc entry\n");
				goto error_return;
      		}
      		printk(KERN_INFO"dmverity_odin_flag_read_init:: Registering /proc/dmverity_odin_flag Interface \n");
	} else {
		printk(KERN_INFO"dmverity_odin_flag_read_init:: not enabling in non-recovery mode\n");
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
        printk(KERN_INFO"Deregistering /proc/dmverity_odin_flag interface\n");
}


module_init(dmverity_odin_flag_read_init);
module_exit(dmverity_odin_flag_read_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
