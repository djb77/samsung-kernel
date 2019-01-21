/*
 * @file knox_test.c
 * @brief test driver for knox system related modules
 * Copyright (c) 2017, Samsung Electronics Corporation. All rights reserved.
 */
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/qseecom.h>
#include "knox_test.h"

struct dentry	*dent_dir;

cmdId disallowedCmdTable[] = {
	CMD_GET_WB,
	CMD_SET_WB,
	CMD_GET_RP_EN,
	CMD_GET_RP_IG,
	CMD_SET_ICCC,
	CMD_IS_SECBOOT,
	CMD_STORE_MEASURE,
	CMD_GET_RP_XBL,
	CMD_GET_RP_PIL,
	CMD_GET_RP_RPM,
	CMD_GET_RP_TZ,
	CMD_GET_RP_HYP,
	CMD_GET_RP_DP,
	CMD_GET_RP_DC,
	CMD_GET_RP_ABL,
	CMD_GET_RPMB_FUSE,
	CMD_GET_RP_PMIC,
	CMD_GET_EK_FUSE,
	CMD_SET_EK_FUSE_DEV,
	CMD_SET_EK_FUSE_USER,
	CMD_SET_EK_FUSE_FORCE_USER,
	CMD_SET_BOOT_COMPLETE,
	CMD_SET_DMV,
	CMD_SET_KASLR,
	CMD_DUMMY,
};

cmdId allowedCmdTable[] = {
	CMD_GET_DMV,
};

int run_lksecapp_test(char *buf) {
	char app_name[MAX_APP_NAME_SIZE]={0};
	int qsee_ret = 0, test_result = TEST_PASS, i;
	send_cmd_t *cmd;
	send_cmd_rsp_t *rsp;
	int req_len = 0, rsp_len = 0, len = 0;
	struct qseecom_handle *q_lksecapp_handle = NULL;

	printk(KERN_INFO "knox_test -- %s:\n", __func__);
	snprintf(app_name, MAX_APP_NAME_SIZE, "%s", "lksecapp");

	qsee_ret = qseecom_start_app(&q_lksecapp_handle, app_name, lksecapp_QSEE_BUFFER_LENGTH);
	if ( NULL == q_lksecapp_handle ) {
		printk(KERN_ERR"knox_test -- cannot get tzapp handle from kernel.\n");
		test_result = TEST_FAIL;
		goto exit;
	}
	if (qsee_ret) {
		printk(KERN_ERR"knox_test -- cannot load tzapp from kernel; qsee_ret =  %d.\n", qsee_ret);
		test_result = TEST_FAIL;
		goto exit;
	}
	
	/* Prepare request buffer */
	cmd = (send_cmd_t *) q_lksecapp_handle->sbuf;
	req_len = sizeof(send_cmd_t);
	if (req_len & QSEECOM_ALIGN_MASK)
		req_len = QSEECOM_ALIGN(req_len);

	/* prepare the response buffer */
	rsp =(send_cmd_rsp_t *)(q_lksecapp_handle->sbuf + req_len);
	rsp_len = sizeof(send_cmd_rsp_t);
	if (rsp_len & QSEECOM_ALIGN_MASK)
		rsp_len = QSEECOM_ALIGN(rsp_len);

	/* check disallowed commands */
	for(i=0; i<ARRAY_LENGTH(disallowedCmdTable); i++) {

		cmd->cmd_id = disallowedCmdTable[i];
		qseecom_set_bandwidth(q_lksecapp_handle, true);
		qsee_ret = qseecom_send_command(q_lksecapp_handle, cmd, req_len, rsp, rsp_len);
		qseecom_set_bandwidth(q_lksecapp_handle, false);
	
		if (qsee_ret) {
			printk(KERN_ERR"knox_test--failed to send cmd to qseecom; qsee_ret = %d.\n", qsee_ret);
			test_result = TEST_FAIL;
			goto shutdown_and_exit;
		}
		
		printk(KERN_INFO"knox_test -- for command %d response status 0x%x\n", disallowedCmdTable[i], rsp->status);
		
		if(rsp->status != BL_BOOT_COMPLETE_FAILURE) {
			printk(KERN_ERR"knox_test-- test case failure for cmd ID : %d.\n", disallowedCmdTable[i]);
			test_result = TEST_FAIL;
		}
	}
	
	/* check allowed commands */
	for(i=0; i<ARRAY_LENGTH(allowedCmdTable); i++) {

		cmd->cmd_id = allowedCmdTable[i];
		qseecom_set_bandwidth(q_lksecapp_handle, true);
		qsee_ret = qseecom_send_command(q_lksecapp_handle, cmd, req_len, rsp, rsp_len);
		qseecom_set_bandwidth(q_lksecapp_handle, false);
	
		if (qsee_ret) {
			printk(KERN_ERR"knox_test--failed to send cmd to qseecom; qsee_ret = %d.\n", qsee_ret);
			test_result = TEST_FAIL;
			goto shutdown_and_exit;
		}
		
		printk(KERN_INFO"knox_test -- for command %d response status 0x%x\n", allowedCmdTable[i], rsp->status);
		
		if(rsp->status == BL_BOOT_COMPLETE_FAILURE) {
			printk(KERN_ERR"knox_test-- test case failure for cmd ID : %d.\n", allowedCmdTable[i]);
			test_result = TEST_FAIL;
		}
	}

shutdown_and_exit:
	qsee_ret = qseecom_shutdown_app(&q_lksecapp_handle);
	if ( qsee_ret ) {
		printk(KERN_ERR"knox_test--failed to shut down the tzapp.\n");
		test_result = TEST_FAIL;
	}
	else
		q_lksecapp_handle = NULL;
	
exit:

	if(test_result) {
		printk(KERN_INFO "knox_test -- TEST PASSED\n");
		len += snprintf((unsigned char *)buf+len, TEST_RESULT_STRING_SIZE, "knox_test -- TEST PASSED\n");
	} else {
		printk(KERN_INFO "knox_test -- TEST FAILED !!\n");
		len += snprintf((unsigned char *)buf+len, TEST_RESULT_STRING_SIZE, "knox_test -- TEST FAILED !! Please check kernel logs\n");
	}

	return len;
}

static ssize_t knox_test_read(struct file *fp, char __user *buf, size_t len, loff_t *off)
{
	int count;
	char ret_buf[TEST_RESULT_STRING_SIZE];

	printk(KERN_INFO "knox_test -- %s:\n", __func__);

	count = run_lksecapp_test(ret_buf);

	return simple_read_from_buffer(buf, count, off,
				ret_buf, count);
}

static const struct file_operations knox_test_proc_fops = {
	.read = knox_test_read,
};

static int __init knox_test_init(void)
{
	int ret = 0;
	struct dentry   *dent;

	printk(KERN_INFO"knox_test -- %s:\n", __func__);

	dent_dir = debugfs_create_dir("knox_test", NULL);
	if (dent_dir == NULL) {
		printk(KERN_ERR"knox_test -- debugfs_create_dir failed\n");
		ret = -1;
	}

	dent = debugfs_create_file("lksecapp", S_IRUGO, dent_dir, NULL, &knox_test_proc_fops);
	if (dent == NULL) {
			printk(KERN_ERR"knox_test -- debugfs_create_file failed\n");
			ret = -1;
			goto exit;
	}

	printk(KERN_INFO"knox_test -- %s: registered /d/knox_test/lksecapp interface\n", __func__);
	return 0;
exit:
	debugfs_remove_recursive(dent_dir);
	return ret;
}


static void __exit knox_test_exit(void)
{
	printk(KERN_INFO"knox_test -- deregistering /d/knox_test interface\n");
	if(dent_dir != NULL) {
		debugfs_remove_recursive(dent_dir);
	} else {
		printk(KERN_INFO"knox_test -- knox_test debugfs entry does not exist\n");
	}	
}

module_init(knox_test_init);
module_exit(knox_test_exit);

MODULE_DESCRIPTION(DRIVER_DESC);



