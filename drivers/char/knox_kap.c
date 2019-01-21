/*
 */
			
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/crash_dump.h>
#include <linux/backing-dev.h>
#include <linux/bootmem.h>
#include <linux/splice.h>
#include <linux/pfn.h>
#include <linux/export.h>
#include <linux/seq_file.h>
#include <linux/knox_kap.h>

#include <asm/uaccess.h>
#include <asm/io.h> 

#include "qseecom_kernel.h"

#define SMC_CMD_KAP_CALL                (0x83000009)
#define SMC_CMD_KAP_STATUS                (0x8300000A)

unsigned int kap_on_reboot = 0;  // 1: turn on kap after reboot; 0: no pending ON action
unsigned int kap_off_reboot = 0; // 1: turn off kap after reboot; 0: no pending OFF action

static int get_kap_lksecapp_call(void)
{
	int ret = -1 ;
	int err = -1 ;
	int dmv_req_len = 0 ;
	int dmv_rsp_len = 0 ;
	dmv_req_t *dmv_req = NULL;
	dmv_rsp_t *dmv_rsp = NULL;
	struct qseecom_handle *qhandle_kap = NULL;

  	printk(KERN_ERR" %s -> qhandle address : %p \n", __FUNCTION__, qhandle_kap);
  	ret = qseecom_start_app((&qhandle_kap), "lksecapp", QSEECOM_SBUFF_SIZE);
	if (ret) {
		printk(KERN_ERR" %s -> qseecom_start_app failed %d\n", __FUNCTION__, ret);
		goto err_ret;
	}
	printk(KERN_ERR" %s -> qseecom_start_app succeeded. %d\n", __FUNCTION__, ret);
	dmv_req = (dmv_req_t *)qhandle_kap->sbuf;
	printk(KERN_ERR" %s -> dmv_req address : %p . \n", __FUNCTION__, dmv_req);
	dmv_req_len = sizeof(dmv_req_t);

	dmv_req->cmd_id = SEC_BOOT_CMD101_GET_DMV_DATA;

	dmv_rsp = (dmv_rsp_t *)(qhandle_kap->sbuf + dmv_req_len);
	dmv_rsp_len = sizeof(dmv_rsp_t);

	err = qseecom_send_command(qhandle_kap, dmv_req, dmv_req_len, dmv_rsp, dmv_rsp_len);
	
	printk(KERN_ERR" %s -> read KAP status %d\n", __FUNCTION__, dmv_req->data.dmv_bl_status.security_mode);
	if (err) {
		printk(KERN_ERR" %s -> Failed to send command : %d\n", __FUNCTION__, err);
		goto err_ret;
	}

	return dmv_req->data.dmv_bl_status.security_mode;

err_ret:
	printk(KERN_ERR" %s -> sending command to tz app failed with error %d, shutting down %s\n", __FUNCTION__, err, "lksecapp");
	return err;	
}

static void turn_off_kap(void) {
	kap_on_reboot = 0;
	kap_off_reboot = 1;
	printk(KERN_ERR " %s -> Turn off kap mode\n", __FUNCTION__);
}

static void turn_on_kap(void) {
	kap_off_reboot = 0;
	kap_on_reboot = 1;
	printk(KERN_ERR " %s -> Turn on kap mode\n", __FUNCTION__);
}

ssize_t knox_kap_write(struct file *file, const char __user *buffer, size_t size, loff_t *offset) {

	unsigned long mode;
	char *string;

	printk(KERN_ERR " %s\n", __FUNCTION__);

	string = kmalloc(size + sizeof(char), GFP_KERNEL);
	if (string == NULL) {
		printk(KERN_ERR "%s failed kmalloc\n", __func__);
		return size;
	}

	memcpy(string, buffer, size);
	string[size] = '\0';

	if(kstrtoul(string, 0, &mode)) {
		kfree(string);
		return size;
	};

	kfree(string);

	printk(KERN_ERR "id: %d\n", (int)mode);

	switch(mode) {
		case 0:
			turn_off_kap();
			break;
		case 1:
		  turn_on_kap();
			break;
		default:
			printk(KERN_ERR " %s -> Invalid kap mode operations\n", __FUNCTION__);
			break;
	}

	*offset += size;

	return size;
}

#define KAP_RET_SIZE	5
#define KAP_MAGIC	0x5afe0000
#define KAP_MAGIC_MASK	0xffff0000
static int knox_kap_read(struct seq_file *m, void *v)
{
	unsigned long tz_ret = 0;
	unsigned char ret_buffer[KAP_RET_SIZE];
	unsigned volatile int ret_val;

// ????? //
	//clean_dcache_area(&tz_ret, 8);
	//tima_send_cmd(__pa(&tz_ret), 0x3f850221);
	//tz_ret = exynos_smc_kap(SMC_CMD_KAP_CALL, 0x50, 0, 0);
	tz_ret =   get_kap_lksecapp_call();
	//tz_ret = KAP_MAGIC | 1;

	printk(KERN_ERR "KAP Read STATUS val = %lx\n", tz_ret);

	if (tz_ret == (KAP_MAGIC | 0x01)) {
		ret_val = 0x03;		//RKP and/or DMVerity says device is tampered
	} else if (tz_ret == (KAP_MAGIC | 0x11)) {
		/* KAP is ON*/
		if (kap_off_reboot == 1){
			ret_val = 0x10;		//KAP is ON and will turn OFF upon next reboot 
		} else {
			ret_val = 0x11;		//KAP is ON and will stay ON
		}
	} else if (tz_ret == (KAP_MAGIC)) {
		/* KAP is OFF*/
		if (kap_on_reboot == 1){
			ret_val = 0x01;		//KAP is OFF but will turn on upon next reboot
		} else {
			ret_val = 0;		//KAP is OFF and will stay OFF upon next reboot
		}
	} else {
		ret_val = 0x04;		//The magic string is not there. KAP mode not implemented
	}
	memset(ret_buffer,0,KAP_RET_SIZE);
	snprintf(ret_buffer, sizeof(ret_buffer), "%02x\n", ret_val);
	seq_write(m, ret_buffer, sizeof(ret_buffer));

	return 0;
}
static int knox_kap_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, knox_kap_read, NULL);
}

long knox_kap_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/* 
	 * Switch according to the ioctl called 
	 */
	switch (cmd) {
		case 0:
			turn_off_kap();
			break;
		case 1:
			turn_on_kap();
			break;
		default:
			printk(KERN_ERR " %s -> Invalid kap mode operations\n", __FUNCTION__);
			return -1;
			break;
	}

	return 0;
}

const struct file_operations knox_kap_fops = {
	.open	= knox_kap_open,
	.release	= single_release,
	.read	= seq_read,
	.write	= knox_kap_write,
	.unlocked_ioctl  = knox_kap_ioctl,
};
