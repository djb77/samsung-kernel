/*
 * Samsung TZIC Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define KMSG_COMPONENT "TZIC"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/android_pmem.h>
#include <linux/io.h>
#include <soc/qcom/scm.h> // multiple oemflag
//#include <mach/scm.h>   // one oemflag => old version
#include <linux/types.h>

#define TZIC_DEV "tzic"

#ifdef CONFIG_TZDEV // For blowfish
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/miscdevice.h>
#include "tzdev/tzirs.h"
#include "tzdev/tz_cdev.h"
#endif /* CONFIG_TZDEV */

static DEFINE_MUTEX(tzic_mutex);

static struct class *driver_class;
static dev_t tzic_device_no;
static struct cdev tzic_cdev;

#define HLOS_IMG_TAMPER_FUSE    0
typedef enum {
    OEMFLAG_MIN_FLAG = 2,
    OEMFLAG_TZ_DRM,
    OEMFLAG_FIDD,
    OEMFLAG_CC,
    OEMFLAG_SYSSCOPE,
    OEMFLAG_NUM_OF_FLAG,
} Sec_OemFlagID_t;

typedef struct
{
    uint32_t  name;
    uint32_t  func_cmd;
    uint32_t  value;
}t_flag;

#ifndef CONFIG_TZDEV
typedef enum {
	IRS_SET_FLAG_CMD        =           1,
	IRS_SET_FLAG_VALUE_CMD,
	IRS_INC_FLAG_CMD,
	IRS_GET_FLAG_VAL_CMD,
	IRS_ADD_FLAG_CMD,
	IRS_DEL_FLAG_CMD
} TZ_IRS_CMD;
#endif /* CONFIG_TZDEV */

#ifndef SCM_SVC_FUSE
#define SCM_SVC_FUSE            0x08
#endif
#define SCM_BLOW_SW_FUSE_ID     0x01
#define SCM_IS_SW_FUSE_BLOWN_ID 0x02
#define TZIC_IOC_MAGIC          0x9E
#define TZIC_IOCTL_GET_FUSE_REQ _IO(TZIC_IOC_MAGIC, 0)
#define TZIC_IOCTL_SET_FUSE_REQ _IO(TZIC_IOC_MAGIC, 1)

#define TZIC_IOCTL_SET_FUSE_REQ_DEFAULT _IO(TZIC_IOC_MAGIC, 2)

#define TZIC_IOCTL_GET_FUSE_REQ_NEW _IO(TZIC_IOC_MAGIC, 10)
#define TZIC_IOCTL_SET_FUSE_REQ_NEW _IO(TZIC_IOC_MAGIC, 11)

#define STATE_IC_BAD    1
#define STATE_IC_GOOD   0

#define LOG printk

static int ic = STATE_IC_GOOD;
static int set_tamper_fuse_cmd(void);
static uint8_t get_tamper_fuse_cmd(void);

static int set_tamper_fuse_cmd_new(uint32_t flag);
static uint8_t get_tamper_fuse_cmd_new(uint32_t flag);

static int set_tamper_fuse_cmd()
{
	struct scm_desc desc = {0};
	uint32_t fuse_id;

	desc.args[0] = fuse_id = HLOS_IMG_TAMPER_FUSE;
	desc.arginfo = SCM_ARGS(1);

	if (!is_scm_armv8()) {
		return scm_call(SCM_SVC_FUSE, SCM_BLOW_SW_FUSE_ID, &fuse_id,
			sizeof(fuse_id), NULL, 0);
	} else {
		return scm_call2(SCM_SIP_FNID(SCM_SVC_FUSE, SCM_BLOW_SW_FUSE_ID),
				&desc);
	}
}


static int set_tamper_fuse_cmd_new(uint32_t flag)
{
	struct scm_desc desc = {0};
	uint32_t fuse_id;

	desc.args[0] = fuse_id = flag;
	desc.arginfo = SCM_ARGS(1);

	if (!is_scm_armv8()) {
		return scm_call(SCM_SVC_FUSE, SCM_BLOW_SW_FUSE_ID, &fuse_id,
			sizeof(fuse_id), NULL, 0);
	} else {
		return scm_call2(SCM_SIP_FNID(SCM_SVC_FUSE, SCM_BLOW_SW_FUSE_ID),
				&desc);
	}
}


static uint8_t get_tamper_fuse_cmd()
{
	int ret;
	uint32_t fuse_id;
	uint8_t resp_buf;
	size_t resp_len;
	struct scm_desc desc = {0};

	resp_len = sizeof(resp_buf);

	desc.args[0] = fuse_id = HLOS_IMG_TAMPER_FUSE;
	desc.arginfo = SCM_ARGS(1);

	if (!is_scm_armv8()) {
		ret = scm_call(SCM_SVC_FUSE, SCM_IS_SW_FUSE_BLOWN_ID, &fuse_id,
			sizeof(fuse_id), &resp_buf, resp_len);
	} else {
		ret = scm_call2(SCM_SIP_FNID(SCM_SVC_FUSE, SCM_IS_SW_FUSE_BLOWN_ID),
				&desc);
		resp_buf = desc.ret[0];
	}

	if (ret) {
		printk("scm_call/2 returned %d", ret);
		resp_buf = 0xff;
	}

	ic = resp_buf;
	return resp_buf;
}

static uint8_t get_tamper_fuse_cmd_new(uint32_t flag)
{
	int ret;
	uint32_t fuse_id;
	uint8_t resp_buf;
	size_t resp_len;
	struct scm_desc desc = {0};

	resp_len = sizeof(resp_buf);

	desc.args[0] = fuse_id = flag;
	desc.arginfo = SCM_ARGS(1);

	if (!is_scm_armv8()) {
		ret = scm_call(SCM_SVC_FUSE, SCM_IS_SW_FUSE_BLOWN_ID, &fuse_id,
			sizeof(fuse_id), &resp_buf, resp_len);
	} else {
		ret = scm_call2(SCM_SIP_FNID(SCM_SVC_FUSE, SCM_IS_SW_FUSE_BLOWN_ID),
				&desc);
		resp_buf = desc.ret[0];
	}

	if (ret) {
		printk("scm_call/2 returned %d", ret);
		resp_buf = 0xff;
	}

	ic = resp_buf;
	return resp_buf;
}

static long tzic_ioctl(struct file *file, unsigned cmd,
		unsigned long arg)
{
	int ret = 0;
	int i = 0;
	t_flag param = { 0, 0, 0 };
#ifdef CONFIG_TZDEV
	unsigned long p1, p2, p3;

	//struct irs_ctx __user *ioargp = (struct irs_ctx __user *) arg;
	//struct irs_ctx ctx = {0};

	if ( _IOC_TYPE(cmd) != IOC_MAGIC  && _IOC_TYPE(cmd) != TZIC_IOC_MAGIC ) {
		LOG(KERN_INFO "[oemflag]INVALID CMD = %d\n", cmd);
		return -ENOTTY;
	}
#endif /* CONFIG_TZDEV */

	switch(cmd){
#ifdef CONFIG_TZDEV
		case IOCTL_IRS_CMD:
			LOG(KERN_INFO "[oemflag]tzirs cmd\n");
			/* get flag id */
			ret=copy_from_user( &param, (void *)arg, sizeof(param) );
			if (ret != 0) {
				LOG(KERN_INFO "[oemflag]copy_from_user failed, ret = 0x%08x\n", ret);
				goto return_new_from;
			}

			p1 = param.name;
			p2 = param.value;
			p3 = param.func_cmd;

			LOG(KERN_INFO "[oemflag]before: id = 0x%lx, value = 0x%lx, cmd = 0x%lx\n", (unsigned long)p1, (unsigned long)p2, (unsigned long)p3);

			ret = tzirs_smc(&p1, &p2, &p3);

			LOG(KERN_INFO "[oemflag]after: id = 0x%lx, value = 0x%lx, cmd = 0x%lx\n", (unsigned long)p1, (unsigned long)p2, (unsigned long)p3);

			if (ret) {
				LOG(KERN_INFO "[oemflag]Unable to send IRS_CMD : id = 0x%lx, ret = %d\n", (unsigned long)p1, ret);
				return -EFAULT;
			}

			param.name = p1;
			param.value = p2;
			param.func_cmd = p3;

			goto return_new_to;
		break;
#endif /* CONFIG_TZDEV */

		case TZIC_IOCTL_GET_FUSE_REQ:
			LOG(KERN_INFO "[oemflag]get_fuse\n");
			ret = get_tamper_fuse_cmd();
			LOG(KERN_INFO "[oemflag]tamper_fuse value = %x\n", ret);
		break;

		case TZIC_IOCTL_SET_FUSE_REQ:
			LOG(KERN_INFO "[oemflag]set_fuse\n");
			ret = get_tamper_fuse_cmd();
			LOG(KERN_INFO "[oemflag]tamper_fuse before = %x\n", ret);
			LOG(KERN_INFO "[oemflag]ioctl set_fuse\n");
			mutex_lock(&tzic_mutex);
			ret = set_tamper_fuse_cmd();
			mutex_unlock(&tzic_mutex);
			if (ret)
				LOG(KERN_INFO "[oemflag]failed tzic_set_fuse_cmd: %d\n", ret);
			ret = get_tamper_fuse_cmd();
			LOG(KERN_INFO "[oemflag]tamper_fuse after = %x\n", ret);
		break;

		case TZIC_IOCTL_SET_FUSE_REQ_DEFAULT://SET ALL OEM FLAG EXCEPT 0
			LOG(KERN_INFO "[oemflag]set_fuse_default\n");
			ret=copy_from_user( &param, (void *)arg, sizeof(param) );
			if(ret) {
				LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
				 return ret;
			}
			for (i=OEMFLAG_MIN_FLAG+1;i<OEMFLAG_NUM_OF_FLAG;i++){
				param.name=i;
				LOG(KERN_INFO "[oemflag]set_fuse_name : %u\n", param.name);
				ret = get_tamper_fuse_cmd_new(param.name);
				LOG(KERN_INFO "[oemflag]tamper_fuse before = %x\n", ret);
				LOG(KERN_INFO "[oemflag]ioctl set_fuse\n");
				mutex_lock(&tzic_mutex);
				ret = set_tamper_fuse_cmd_new(param.name);
				mutex_unlock(&tzic_mutex);
				if (ret)
					LOG(KERN_INFO "[oemflag]failed tzic_set_fuse_cmd: %d\n", ret);
				ret = get_tamper_fuse_cmd_new(param.name);
				LOG(KERN_INFO "[oemflag]tamper_fuse after = %x\n", ret);
			}
		break;

		case TZIC_IOCTL_GET_FUSE_REQ_NEW:
			LOG(KERN_INFO "[oemflag]get_fuse\n");
			ret=copy_from_user( &param, (void *)arg, sizeof(param) );
			if(ret) {
				LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
				 return ret;
			}
			if ((OEMFLAG_MIN_FLAG < param.name) && (param.name < OEMFLAG_NUM_OF_FLAG)){
				LOG(KERN_INFO "[oemflag]get_fuse_name : %u\n", param.name);
				ret = get_tamper_fuse_cmd_new(param.name);
				LOG(KERN_INFO "[oemflag]tamper_fuse value = %x\n", ret);
			} else {
				LOG(KERN_INFO "[oemflag]command error\n");
				return -EINVAL;
			}
		break;

		case TZIC_IOCTL_SET_FUSE_REQ_NEW:
			LOG(KERN_INFO "[oemflag]set_fuse\n");
			ret=copy_from_user( &param, (void *)arg, sizeof(param) );
			if(ret) {
				LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
				 return ret;
			}
			if ((OEMFLAG_MIN_FLAG < param.name) && (param.name < OEMFLAG_NUM_OF_FLAG)){
				LOG(KERN_INFO "[oemflag]set_fuse_name : %u\n", param.name);
				ret = get_tamper_fuse_cmd_new(param.name);
				LOG(KERN_INFO "[oemflag]tamper_fuse before = %x\n", ret);
				LOG(KERN_INFO "[oemflag]ioctl set_fuse\n");
				//Qualcomm DRM oemflag only support HLOS_IMG_TAMPER_FUSE
				if (param.name == OEMFLAG_TZ_DRM) {
					mutex_lock(&tzic_mutex);
					ret = set_tamper_fuse_cmd();
					mutex_unlock(&tzic_mutex);
					if (ret)
						LOG(KERN_INFO "[oemflag]failed tzic_set_fuse_cmd: %d\n", ret);
				}
				mutex_lock(&tzic_mutex);
				ret = set_tamper_fuse_cmd_new(param.name);
				mutex_unlock(&tzic_mutex);
				if (ret)
					LOG(KERN_INFO "[oemflag]failed tzic_set_fuse_cmd: %d\n", ret);
				ret = get_tamper_fuse_cmd_new(param.name);
				LOG(KERN_INFO "[oemflag]tamper_fuse after = %x\n", ret);
			} else {
				LOG(KERN_INFO "[oemflag]command error\n");
				return -EINVAL;
			}
		break;

		default:
			LOG(KERN_INFO "[oemflag]default\n");
			ret=copy_from_user( &param, (void *)arg, sizeof(param) );

			if (param.func_cmd == IRS_SET_FLAG_VALUE_CMD) {
				LOG(KERN_INFO "[oemflag]set_fuse\n");
				if(ret) {
					LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
					 return ret;
				}
				if ((OEMFLAG_MIN_FLAG < param.name) && (param.name < OEMFLAG_NUM_OF_FLAG)){
					LOG(KERN_INFO "[oemflag]set_fuse_name : %u\n", param.name);
					ret = get_tamper_fuse_cmd_new(param.name);
					LOG(KERN_INFO "[oemflag]tamper_fuse before = %x\n", ret);
					LOG(KERN_INFO "[oemflag]ioctl set_fuse\n");
					//Qualcomm DRM oemflag only support HLOS_IMG_TAMPER_FUSE
					if (param.name == OEMFLAG_TZ_DRM) {
						mutex_lock(&tzic_mutex);
						ret = set_tamper_fuse_cmd();
						mutex_unlock(&tzic_mutex);
						if (ret)
							LOG(KERN_INFO "[oemflag]failed tzic_set_fuse_cmd: %d\n", ret);
					}
					mutex_lock(&tzic_mutex);
					ret = set_tamper_fuse_cmd_new(param.name);
					mutex_unlock(&tzic_mutex);
					if (ret)
						LOG(KERN_INFO "[oemflag]failed tzic_set_fuse_cmd: %d\n", ret);
					ret = get_tamper_fuse_cmd_new(param.name);
					LOG(KERN_INFO "[oemflag]tamper_fuse after = %x\n", ret);
				} else {
					LOG(KERN_INFO "[oemflag]command error\n");
					return -EINVAL;
				}
			} else if (param.func_cmd == IRS_GET_FLAG_VAL_CMD) {
				LOG(KERN_INFO "[oemflag]get_fuse\n");
				if(ret) {
					LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
					 return ret;
				}
				if ((OEMFLAG_MIN_FLAG < param.name) && (param.name < OEMFLAG_NUM_OF_FLAG)){
					LOG(KERN_INFO "[oemflag]get_fuse_name : %u\n", param.name);
					ret = get_tamper_fuse_cmd_new(param.name);
					LOG(KERN_INFO "[oemflag]tamper_fuse value = %x\n", ret);
				} else {
					LOG(KERN_INFO "[oemflag]command error\n");
					return -EINVAL;
				}
			} else {
				LOG(KERN_INFO "[oemflag]command error\n");
				return -EINVAL;
			}
	}
	return ret;
}

static const struct file_operations tzic_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = tzic_ioctl,
	.compat_ioctl = tzic_ioctl,
};

static int __init tzic_init(void)
{
	int rc;
	struct device *class_dev;

	LOG(KERN_INFO "init tzic");

	rc = alloc_chrdev_region(&tzic_device_no, 0, 1, TZIC_DEV);
	if (rc < 0) {
		LOG(KERN_INFO "alloc_chrdev_region failed %d", rc);
		return rc;
	}

	driver_class = class_create(THIS_MODULE, TZIC_DEV);
	if (IS_ERR(driver_class)) {
		rc = -ENOMEM;
		LOG(KERN_INFO "class_create failed %d", rc);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(driver_class, NULL, tzic_device_no, NULL,
			TZIC_DEV);
	if (!class_dev) {
		LOG(KERN_INFO "class_device_create failed %d", rc);
		rc = -ENOMEM;
		goto class_destroy;
	}

	cdev_init(&tzic_cdev, &tzic_fops);
	tzic_cdev.owner = THIS_MODULE;

	rc = cdev_add(&tzic_cdev, MKDEV(MAJOR(tzic_device_no), 0), 1);
	if (rc < 0) {
		LOG(KERN_INFO "cdev_add failed %d", rc);
		goto class_device_destroy;
	}

	return 0;

class_device_destroy:
	device_destroy(driver_class, tzic_device_no);
class_destroy:
	class_destroy(driver_class);
unregister_chrdev_region:
	unregister_chrdev_region(tzic_device_no, 1);
	return rc;
}

static void __exit tzic_exit(void)
{
	LOG(KERN_INFO "exit tzic");
	device_destroy(driver_class, tzic_device_no);
	class_destroy(driver_class);
	unregister_chrdev_region(tzic_device_no, 1);
}


MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Samsung TZIC Driver");
MODULE_VERSION("1.00");

module_init(tzic_init);
module_exit(tzic_exit);

