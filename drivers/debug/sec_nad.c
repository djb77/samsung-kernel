/*
 * drivers/debug/sec_nad.c
 *
 * COPYRIGHT(C) 2006-2017 Samsung Electronics Co., Ltd. All Right Reserved.
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

#include <linux/device.h>
#include <linux/sec_nad.h>
#include <linux/fs.h>
#include <linux/sec_class.h>
#include <linux/module.h>

#define NAD_PRINT(format, ...) printk(KERN_ERR "[NAD] " format, ##__VA_ARGS__)
#define NAD_DEBUG

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <asm/cacheflush.h>
#include <linux/sec_debug.h>
#include <soc/qcom/subsystem_restart.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sec_param.h>


#define BUFF_SZ 256
/* flag for nad test mode : SMD_NAD or ETC_NAD or MAIN_NAD*/
static int nad_test_mode = -1;

#define SMD_NAD_PROG		"/system/bin/qnad/qnad.sh"
#define MAIN_NAD_PROG		"/system/bin/qnad/qnad_main.sh"
#define ETC_NAD_PROG		"/system/bin/qnad/qnad.sh"
#define ERASE_NAD_PRG		"/system/bin/qnad/remove_files.sh"

#define SMD_NAD_RESULT		"/nad_refer/NAD_SMD/test_result.csv"
#define MAIN_NAD_RESULT		"/nad_refer/NAD_MAIN/test_result.csv"
#define ETC_NAD_RESULT		"/nad_refer/NAD/test_result.csv"

#define SMD_NAD_LOGPATH		"logPath:/nad_refer/NAD_SMD"
#define MAIN_NAD_LOGPATH	"logPath:/nad_refer/NAD_MAIN"
#define ETC_NAD_LOGPATH		"logPath:/nad_refer/NAD"

struct param_qnad param_qnad_data;
static int erase_pass;
extern unsigned int lpcharge;
unsigned int clk_limit_flag;
int curr_smd = ETC_NAD;
int main_reboot;

char *STR_TEST_ITEM[ITEM_CNT + 1] = {
	"UFS",
	"QMESACACHE",
	"QMESADDR",
	"VDDMIN",
	"SUSPEND",
	"CCOHERENCY",
	"ICACHE",
	"CRYPTO",
	"DDR",
	//
	"FULL"
};

struct kobj_uevent_env nad_uevent;

int get_nad_result(char *buf, int piece)
{
	int iCnt;
	unsigned int smd_result;

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}

	smd_result = param_qnad_data.total_test_result;

	NAD_PRINT("%s : param_qnad_data.total_test_result=%u,", __func__,
		param_qnad_data.total_test_result);

	if (piece == ITEM_CNT) {
		for (iCnt = 0; iCnt < ITEM_CNT; iCnt++) {
			switch (smd_result & 0x3) {
			case 0:
				strcat(buf, "[X]");
				break;
			case 1:
				strcat(buf, "[F]");
				break;
			case 2:
				strcat(buf, "[P]");
				break;
			case 3:
				strcat(buf, "[N]");
				break;
			}

			smd_result >>= 2;
		}
	} else {
		smd_result >>= 2 * piece;
		switch (smd_result & 0x3) {
		case 1:
			strlcpy(buf, "FAIL", sizeof(buf));
			break;
		case 2:
			strlcpy(buf, "PASS", sizeof(buf));
			break;
		case 3:
			strlcpy(buf, "NA", sizeof(buf));
			break;
		}
	}

	return ITEM_CNT;
err_out:
	return 0;
}
EXPORT_SYMBOL(get_nad_result);

static int NadResult(int smd_nad)
{
	int fd = 0;
	int ret = TOTALTEST_UNKNOWN;
	int ddr_result = 0;
	char buf[512] = { '\0', };
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);

	switch (smd_nad) {
	case SMD_NAD:
		fd = sys_open(SMD_NAD_RESULT, O_RDONLY, 0);
		break;
	case MAIN_NAD:
		fd = sys_open(MAIN_NAD_RESULT, O_RDONLY, 0);
		break;
	case ETC_NAD:
		fd = sys_open(ETC_NAD_RESULT, O_RDONLY, 0);
		break;
	default:
		break;
	}

	if (fd >= 0) {
		int found = 0;

		printk(KERN_DEBUG);

		while (sys_read(fd, buf, 512)) {
			char *ptr;
			char *div = "\n";
			char *tok = NULL;

			ptr = buf;
			while ((tok = strsep(&ptr, div)) != NULL) {

				if ((strstr(tok, "FAIL"))) {
					ret = TOTALTEST_FAIL;
					found = 1;
					break;
				}

				if ((strstr(tok, "AllTestDone"))) {
					ret = TOTALTEST_PASS;
					found = 1;
					break;
				}
			}

			if (found)
				break;
		}

		if (!found)
			ret = TOTALTEST_NO_RESULT_STRING;

		sys_close(fd);
	} else {
		NAD_PRINT("The result file is not existed. %s\n", __func__);
		ret = TOTALTEST_NO_RESULT_FILE;
	}

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}

	if ((smd_nad == SMD_NAD) || (smd_nad == MAIN_NAD)) {
		ddr_result = GET_DDR_TEST_RESULT(smd_nad, param_qnad_data.ddrtest_result);
		if (ddr_result == DDRTEST_PASS) {
			NAD_PRINT("ddr test was succeeded\n");
		} else if ((ret != TOTALTEST_UNKNOWN) && (ddr_result == DDRTEST_FAIL)) {
			NAD_PRINT("ddr test was failed\n");
			ret = TOTALTEST_FAIL;
		} else
			NAD_PRINT("ddr test was not executed\n");
	}

err_out:
	set_fs(old_fs);

	return ret;
}

static void do_nad(int smd_nad)
{
	char *argv[4] = { NULL, NULL, NULL, NULL };
	char *envp[3] = { NULL, NULL, NULL };
	int ret_userapp;

	switch (smd_nad) {
	case SMD_NAD:
		argv[0] = SMD_NAD_PROG;
		argv[1] = SMD_NAD_LOGPATH;
		NAD_PRINT("SMD_NAD, nad_test_mode : %d\n", nad_test_mode);
		break;
	case MAIN_NAD:
		argv[0] = MAIN_NAD_PROG;
		argv[1] = MAIN_NAD_LOGPATH;
		if (main_reboot == 1)
			argv[2] = "Reboot";
		NAD_PRINT("MAIN_NAD, nad_test_mode : %d", nad_test_mode);
		NAD_PRINT("reboot option enabled \n");
		break;
	case ETC_NAD:
		argv[0] = ETC_NAD_PROG;
		argv[1] = ETC_NAD_LOGPATH;
		NAD_PRINT("ETC_NAD, nad_test_mode : %d\n", nad_test_mode);
		argv[2] = "Reboot";
		NAD_PRINT
		    ("Setting an argument to reboot after NAD completes.\n");
		break;
	default:
		NAD_PRINT("Invalid smd_nad value, nad_test_mode : %d\n",
			  nad_test_mode);
		break;
	}

	envp[0] = "HOME=/";
	envp[1] = "PATH=/system/bin/qnad:/system/bin:/system/xbin";
	ret_userapp = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);

	if (!ret_userapp) {
		NAD_PRINT("%s is executed. ret_userapp = %d\n", argv[0],
			  ret_userapp);

		if (erase_pass)
			erase_pass = 0;
	} else {
		NAD_PRINT("%s is NOT executed. ret_userapp = %d\n", argv[0],
			  ret_userapp);
		nad_test_mode = -1;
	}
}

static ssize_t show_nad_acat(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int nad_result;

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}
	NAD_PRINT("%s : magic %x, nad cnt %d, ddr cnt %d, ddr result 0x%2X\n",
		  __func__, param_qnad_data.magic,
		  param_qnad_data.nad_remain_count,
		  param_qnad_data.ddrtest_remain_count,
		  GET_DDR_TEST_RESULT(ETC_NAD, param_qnad_data.ddrtest_result));

	nad_result = NadResult(ETC_NAD);
	switch (nad_result) {
	case TOTALTEST_PASS: {
		NAD_PRINT("NAD Passed\n");
		return snprintf(buf, BUFF_SZ, "OK_ACAT_NONE\n");
	} break;

	case TOTALTEST_FAIL: {
		NAD_PRINT("NAD fail\n");
		return snprintf(buf, BUFF_SZ, "NG_ACAT_ASV\n");
	} break;

	default: {
		NAD_PRINT("NAD No Run\n");
		return snprintf(buf, BUFF_SZ, "OK\n");
	}
	}
err_out:
	return snprintf(buf, BUFF_SZ, "UNKNOWN\n");
}

static ssize_t store_nad_acat(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret = -1;
	int idx = 0;
	int nad_loop_count, dram_loop_count;
	char temp[NAD_BUFF_SIZE * 3];
	char nad_cmd[NAD_CMD_LIST][NAD_BUFF_SIZE];
	char *nad_ptr, *string;
	NAD_PRINT("buf : %s count : %d\n", buf, (int)count);
	if ((int)count < NAD_BUFF_SIZE)
		return -EINVAL;

	/* Copy buf to nad temp */
	strlcpy(temp, buf, NAD_BUFF_SIZE * 3);
	string = temp;

	while (idx < NAD_CMD_LIST) {
		nad_ptr = strsep(&string, ",");
		strlcpy(nad_cmd[idx++], nad_ptr, NAD_BUFF_SIZE);
	}

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}

	if (!strncmp(buf, "nad_acat", 8)) {
		// checking 1st boot and setting test count
		if (param_qnad_data.magic != NAD_SMD_MAGIC) {	// <======================== 1st boot at right after SMD D/L done
			nad_test_mode = SMD_NAD;
			NAD_PRINT("1st boot at SMD\n");
			param_qnad_data.magic = NAD_SMD_MAGIC;
			param_qnad_data.nad_remain_count = 0x0;
			param_qnad_data.ddrtest_remain_count = 0x0;

			// flushing to param partition
			if (!sec_set_param(param_index_qnad, &param_qnad_data)) {
				pr_err
				    ("%s : fail - set param!! param_qnad_data\n",
				     __func__);
				goto err_out;
			}
			curr_smd = nad_test_mode;
			do_nad(nad_test_mode);
		} else {		//  <========================not SMD, it can be LCIA, CAL, FINAL and 15 ACAT.
			nad_test_mode = ETC_NAD;
			ret = sscanf(nad_cmd[1], "%d\n", &nad_loop_count);
			if (ret != 1)
				return -EINVAL;

			ret = sscanf(nad_cmd[2], "%d\n", &dram_loop_count);
			if (ret != 1)
				return -EINVAL;

			NAD_PRINT("ETC NAD, nad_acat%d,%d\n", nad_loop_count,
				  dram_loop_count);
			if (!nad_loop_count && !dram_loop_count) {	// <+++++++++++++++++ both counts are 0, it means 1. testing refers to current remain_count

				// stop retrying when failure occur during retry test at ACAT/15test
				if (param_qnad_data.magic == NAD_SMD_MAGIC
				    && NadResult(ETC_NAD) == TOTALTEST_FAIL) {
					pr_err
					    ("%s : nad test fail - set the remain counts to 0\n",
					     __func__);
					param_qnad_data.nad_remain_count = 0;
					param_qnad_data.ddrtest_remain_count =
					    0;

					// flushing to param partition
					if (!sec_set_param
					    (param_index_qnad,
					     &param_qnad_data)) {
						pr_err
						    ("%s : fail - set param!! param_qnad_data\n",
						     __func__);
						goto err_out;
					}
				}

				if (param_qnad_data.nad_remain_count > 0) {	// NAD count still remain
					NAD_PRINT
					    ("nad : nad_remain_count = %d, ddrtest_remain_count = %d\n",
					     param_qnad_data.nad_remain_count,
					     param_qnad_data.
					     ddrtest_remain_count);
					param_qnad_data.nad_remain_count--;
					param_qnad_data.total_test_result &= 0xffffffff;

/*					if(param_qnad_data.ddrtest_remain_count && param_qnad_data.nad_remain_count == 0) { // last NAD count, and next is ddr test. rebooting will be done by NAD
						NAD_PRINT("switching : nad_remain_count = %d, ddrtest_remain_count = %d\n", param_qnad_data.nad_remain_count, param_qnad_data.ddrtest_remain_count);
						param_qnad_data.ddrtest_remain_count--;

						do_ddrtest();
					}*/

					// flushing to param partition
					if (!sec_set_param
					    (param_index_qnad,
					     &param_qnad_data)) {
						pr_err
						    ("%s : fail - set param!! param_qnad_data\n",
						     __func__);
						goto err_out;
					}

					curr_smd = nad_test_mode;
					do_nad(nad_test_mode);
				} else if (param_qnad_data.ddrtest_remain_count) {	// NAD already done before, only ddr test remains. then it needs selfrebooting.
					NAD_PRINT
					    ("ddrtest : nad_remain_count = %d, ddrtest_remain_count = %d\n",
					     param_qnad_data.nad_remain_count,
					     param_qnad_data.
					     ddrtest_remain_count);

					//do_msm_restart(REBOOT_HARD, "sec_debug_hw_reset");
					while (1)
						;
				}
			} else {	// <+++++++++++++++++ not (0,0) means 1. new test count came, 2. so overwrite the remain_count, 3. and not reboot by itsself, 4. reboot cmd will come from outside like factory PGM

				param_qnad_data.nad_remain_count =
				    nad_loop_count;
				param_qnad_data.ddrtest_remain_count =
				    dram_loop_count;
				param_qnad_data.ddrtest_mode = UPLOAD_CAUSE_DDR_TEST;
				// flushing to param partition
				if (!sec_set_param
				    (param_index_qnad, &param_qnad_data)) {
					pr_err
					    ("%s : fail - set param!! param_qnad_data\n",
					     __func__);
					goto err_out;
				}

				NAD_PRINT
				    ("new cmd : nad_remain_count = %d, ddrtest_remain_count = %d\n",
				     param_qnad_data.nad_remain_count,
				     param_qnad_data.ddrtest_remain_count);
			}
		}
		return count;
	} else
		return count;
err_out:
	return count;
}

static DEVICE_ATTR(nad_acat, S_IRUGO | S_IWUSR, show_nad_acat, store_nad_acat);

static ssize_t show_nad_stat(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int nad_result;

	// refer to qpnp_pon_reason (index=boot_reason-1)
	NAD_PRINT("%s : boot_reason was %d\n", __func__, boot_reason);

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}
	NAD_PRINT("%s : magic %x, nad cnt %d, ddr cnt %d, ddr result 0x%2X\n",
		  __func__, param_qnad_data.magic,
		  param_qnad_data.nad_remain_count,
		  param_qnad_data.ddrtest_remain_count,
		  GET_DDR_TEST_RESULT(SMD_NAD, param_qnad_data.ddrtest_result));

	if (param_qnad_data.magic != NAD_SMD_MAGIC) {
		NAD_PRINT("SMD NAD NOT_TESTED\n");
		return snprintf(buf, BUFF_SZ, "NOT_TESTED\n");
	} else {
		nad_result = NadResult(SMD_NAD);
		switch (nad_result) {
		case TOTALTEST_PASS: {
			// there is "AllTestDone" at SMD/test_result.csv
			NAD_PRINT("SMD NAD PASS\n");
			return snprintf(buf, BUFF_SZ, "OK_2.0\n");
		} break;

		case TOTALTEST_FAIL: {
			// there is "FAIL" at SMD/test_result.csv
			char strResult[BUFF_SZ-14] = { '\0', };

			get_nad_result(strResult, ITEM_CNT);
			NAD_PRINT("SMD NAD FAIL, %s", buf);
			return snprintf(buf, BUFF_SZ, "NG_2.0_FAIL_%s\n", strResult);
		} break;

		case TOTALTEST_NO_RESULT_FILE: {
			// there is no SMD/test_result.csv
			if (nad_test_mode == SMD_NAD) {
				// will be executed soon
				NAD_PRINT("SMD NAD TESTING\n");
				return snprintf(buf, BUFF_SZ, "TESTING\n");
			} else {
				// not exeuted by unknown reasons
				// ex1) magic exists but nad_refer was removed
				// ex2) fail to execute qnad.sh
				// ex3) fail to make test_result.csv
				// ex4) etc...
				NAD_PRINT("SMD NAD NO_RESULT_FILE && not started\n");
				return snprintf(buf, BUFF_SZ, "RE_WORK\n");
			}
		} break;

		case TOTALTEST_NO_RESULT_STRING: {
			if (nad_test_mode == SMD_NAD) {
				// will be completed
				NAD_PRINT("SMD NAD TESTING\n");
				return snprintf(buf, BUFF_SZ, "TESTING\n");
			} else {
				// need to rework
				NAD_PRINT("SMD NAD NO_RESULT_STRING && not started\n");
				return snprintf(buf, BUFF_SZ, "RE_WORK\n");
			}
		} break;
		}
	}

err_out:
	NAD_PRINT("SMD NAD UNKNOWN\n");
	return snprintf(buf, BUFF_SZ, "RE_WORK\n");
}

static DEVICE_ATTR(nad_stat, S_IRUGO, show_nad_stat, NULL);

static ssize_t show_ddrtest_remain_count(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}

	return snprintf(buf, BUFF_SZ, "%d\n",
		 param_qnad_data.ddrtest_remain_count);
err_out:
	return snprintf(buf, BUFF_SZ, "PARAM ERROR\n");
}

static DEVICE_ATTR(nad_ddrtest_remain_count, S_IRUGO, show_ddrtest_remain_count,
		   NULL);

static ssize_t show_nad_remain_count(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}

	return snprintf(buf, BUFF_SZ, "%d\n", param_qnad_data.nad_remain_count);
err_out:
	return snprintf(buf, BUFF_SZ, "PARAM ERROR\n");
}

static DEVICE_ATTR(nad_qmvs_remain_count, 0444, show_nad_remain_count, NULL);

static ssize_t store_nad_erase(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	if (!strncmp(buf, "erase", 5)) {
		char *argv[4] = { NULL, NULL, NULL, NULL };
		char *envp[3] = { NULL, NULL, NULL };
		int ret_userapp;
		int api_gpio_test = 0;
		char api_gpio_test_result[256] = { 0, };

		argv[0] = ERASE_NAD_PRG;

		envp[0] = "HOME=/";
		envp[1] =
		    "PATH=/system/bin/qnad/:/system/bin:/system/xbin";
		ret_userapp =
		    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);

		if (!ret_userapp) {
			NAD_PRINT
			    ("remove_files.sh is executed. ret_userapp = %d\n",
			     ret_userapp);
			erase_pass = 1;
		} else {
			NAD_PRINT
			    ("remove_files.sh is NOT executed. ret_userapp = %d\n",
			     ret_userapp);
			erase_pass = 0;
		}

		if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
			pr_err("%s : fail - get param!! param_qnad_data\n",
			       __func__);
			goto err_out;
		}

		param_qnad_data.magic = 0x0;
		param_qnad_data.nad_remain_count = 0x0;
		param_qnad_data.ddrtest_remain_count = 0x0;
		param_qnad_data.ddrtest_result = 0x0;
		param_qnad_data.ddrtest_mode = 0x0;
		param_qnad_data.total_test_result = 0x0;

		// flushing to param partition
		if (!sec_set_param(param_index_qnad, &param_qnad_data)) {
			pr_err("%s : fail - set param!! param_qnad_data\n",
			       __func__);
			goto err_out;
		}

		NAD_PRINT("clearing MAGIC code done = %d\n",
			  param_qnad_data.magic);
		NAD_PRINT("nad_remain_count = %d\n",
			  param_qnad_data.nad_remain_count);
		NAD_PRINT("ddrtest_remain_count = %d\n",
			  param_qnad_data.ddrtest_remain_count);
		NAD_PRINT("ddrtest_result = 0x%8X\n",
			  param_qnad_data.ddrtest_result);
		NAD_PRINT("clearing smd ddr test MAGIC code done = %d\n",
			  param_qnad_data.magic);

		// clearing API test result
		if (!sec_set_param(param_index_apigpiotest, &api_gpio_test)) {
			pr_err("%s : fail - set param!! param_qnad_data\n",
			       __func__);
			goto err_out;
		}

		if (!sec_set_param
		    (param_index_apigpiotestresult, api_gpio_test_result)) {
			pr_err("%s : fail - set param!! param_qnad_data\n",
			       __func__);
			goto err_out;
		}
		return count;
	} else
		return count;
err_out:
	return count;
}

static ssize_t show_nad_erase(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	if (erase_pass)
		return snprintf(buf, BUFF_SZ, "OK\n");
	else
		return snprintf(buf, BUFF_SZ, "NG\n");
}

static DEVICE_ATTR(nad_erase, S_IRUGO | S_IWUSR, show_nad_erase,
		   store_nad_erase);

static ssize_t show_nad_dram(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int ret = 0;

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}

	// The factory app needs only the ddrtest result of ACAT now.
	// If the ddrtest result of SMD and MAIN are also needed,
	// implement an additional sysfs node or a modification of app.
	ret = GET_DDR_TEST_RESULT(ETC_NAD, param_qnad_data.ddrtest_result);

	if (ret == DDRTEST_PASS)
		return snprintf(buf, BUFF_SZ, "OK_DRAM\n");
	else if (ret == DDRTEST_FAIL)
		return snprintf(buf, BUFF_SZ, "NG_DRAM_DATA\n");
	else
		return snprintf(buf, BUFF_SZ, "NO_DRAMTEST\n");
err_out:
	return snprintf(buf, BUFF_SZ, "READ ERROR\n");
}

static DEVICE_ATTR(nad_dram, S_IRUGO, show_nad_dram, NULL);

static ssize_t show_nad_dram_debug(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
//      int ret=0;

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}

	return snprintf(buf, BUFF_SZ, "0x%x\n", param_qnad_data.ddrtest_result);
err_out:
	return snprintf(buf, BUFF_SZ, "READ ERROR\n");
}

static DEVICE_ATTR(nad_dram_debug, S_IRUGO, show_nad_dram_debug, NULL);

static ssize_t show_nad_dram_err_addr(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int i = 0;
	struct param_qnad_ddr_result param_qnad_ddr_result_data;

	if (!sec_get_param
	    (param_index_qnad_ddr_result, &param_qnad_ddr_result_data)) {
		pr_err("%s : fail - get param!! param_qnad_ddr_result_data\n",
		       __func__);
		goto err_out;
	}

	ret =
	    snprintf(buf, BUFF_SZ, "Total : %d\n\n",
		    param_qnad_ddr_result_data.ddr_err_addr_total);
	for (i = 0; i < param_qnad_ddr_result_data.ddr_err_addr_total; i++) {
		ret +=
		    snprintf(buf + ret - 1, BUFF_SZ, "[%d] 0x%llx\n", i,
			    param_qnad_ddr_result_data.ddr_err_addr[i]);
	}

	return ret;
err_out:
	return snprintf(buf, BUFF_SZ, "READ ERROR\n");
}

static DEVICE_ATTR(nad_dram_err_addr, S_IRUGO, show_nad_dram_err_addr, NULL);

static ssize_t show_nad_support(struct device *dev,
				struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_ARCH_MSM8998) || defined(CONFIG_ARCH_MSM8996) || defined(CONFIG_ARCH_SDM845) || defined(CONFIG_ARCH_SDM450)
	return snprintf(buf, BUFF_SZ, "SUPPORT\n");
#else
	return snprintf(buf, BUFF_SZ, "NOT_SUPPORT\n");
#endif
}

static DEVICE_ATTR(nad_support, S_IRUGO, show_nad_support, NULL);

static ssize_t store_nad_logs(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int fd = 0, i = 0;
	char logbuf[500] = { '\0' };
	char path[100] = { '\0' };
	char ptr;
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);

	NAD_PRINT("%s\n", buf);

	sscanf(buf, "%s", path);
	fd = sys_open(path, O_RDONLY, 0);

	if (fd >= 0) {
		while (sys_read(fd, &ptr, 1) && ptr != -1) {
			//NAD_PRINT("%c\n", ptr);
			logbuf[i] = ptr;
			i++;
			if (ptr == '\n') {
				NAD_PRINT("%s", logbuf);
				i = 0;
			}
		}

		sys_close(fd);
	} else {
		NAD_PRINT("The File is not existed. %s\n", __func__);
	}

	set_fs(old_fs);
	return count;
}

static DEVICE_ATTR(nad_logs, 0200, NULL, store_nad_logs);

static ssize_t store_nad_end(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	char result[20] = { '\0' };

	NAD_PRINT("result : %s\n", buf);

	sscanf(buf, "%s", result);

	if (!strcmp(result, "nad_pass")) {
		kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, nad_uevent.envp);
		NAD_PRINT
		    ("NAD result : %s, nad_pass, Send to Process App for Nad test end : %s\n",
		     result, __func__);
	} else {
		if (nad_test_mode == ETC_NAD) {
			NAD_PRINT
			    ("NAD result : %s, Device enter the upload mode because it is ETC_NAD : %s\n",
			     result, __func__);
			panic(result);
		} else {
			kobject_uevent_env(&dev->kobj, KOBJ_CHANGE,
					   nad_uevent.envp);
			NAD_PRINT
			    ("NAD result : %s, Send to Process App for Nad test end : %s\n",
			     result, __func__);
		}
	}

	nad_test_mode = -1;
	return count;
}

static DEVICE_ATTR(nad_end, 0200, NULL, store_nad_end);

static ssize_t show_nad_api(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	unsigned int api_gpio_test;
	char api_gpio_test_result[256];

	if (!sec_get_param(param_index_apigpiotest, &api_gpio_test)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}

	if (api_gpio_test) {
		if (!sec_get_param
		    (param_index_apigpiotestresult, api_gpio_test_result)) {
			pr_err("%s : fail - get param!! param_qnad_data\n",
			       __func__);
			goto err_out;
		}
		return snprintf(buf, BUFF_SZ, "%s", api_gpio_test_result);
	} else
		return snprintf(buf, BUFF_SZ, "NONE\n");

err_out:
	return snprintf(buf, BUFF_SZ, "READ ERROR\n");
}

static DEVICE_ATTR(nad_api, 0444, show_nad_api, NULL);

static ssize_t store_nad_result(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int _result = -1;
	char test_name[NAD_BUFF_SIZE * 2] = { '\0', };
	char temp[NAD_BUFF_SIZE * 3] = { '\0', };
	char nad_test[2][NAD_BUFF_SIZE * 2];	// 2: "test_name", "result"
	char result_string[NAD_BUFF_SIZE] = { '\0', };
	char *nad_ptr, *string;
	int smd = curr_smd;
	int item = -1;

	if (curr_smd != SMD_NAD) {
		NAD_PRINT("store nad_result only at smd_nad\n");
		return -EIO;
	}

	NAD_PRINT("buf : %s count : %d\n", buf, (int)count);

	if (NAD_BUFF_SIZE * 3 < (int)count || (int)count < 4) {
		NAD_PRINT("result cmd size too long : NAD_BUFF_SIZE<%d\n",
			  (int)count);
		return -EINVAL;
	}

	/* Copy buf to nad temp */
	strlcpy(temp, buf, NAD_BUFF_SIZE * 3);
	string = temp;

	nad_ptr = strsep(&string, ",");
	strlcpy(nad_test[0], nad_ptr, NAD_BUFF_SIZE * 2);
	nad_ptr = strsep(&string, ",");
	strlcpy(nad_test[1], nad_ptr, NAD_BUFF_SIZE * 2);

	sscanf(nad_test[0], "%s", test_name);
	sscanf(nad_test[1], "%s", result_string);

	NAD_PRINT("test_name : %s, test result=%s\n", test_name, result_string);

	if (TEST_PASS(result_string))
		_result = 2;
	else if (TEST_FAIL(result_string))
		_result = 1;
	else
		_result = 0;

	// _results = 0(N/A), 1(FAIL), 2(PASS) from QNAD

	if (TEST_CRYPTO(test_name))
		item = CRYPTO;
	else if (TEST_ICACHE(test_name))
		item = ICACHE;
	else if (TEST_CCOHERENCY(test_name))
		item = CCOHERENCY;
	else if (TEST_SUSPEND(test_name))
		item = SUSPEND;
	else if (TEST_VDDMIN(test_name))
		item = VDDMIN;
	else if (TEST_QMESADDR(test_name))
		item = QMESADDR;
	else if (TEST_QMESACACHE(test_name))
		item = QMESACACHE;
	else if (TEST_UFS(test_name))
		item = UFS;
	else if (TEST_DDR(test_name))
		item = DDR;
	else
		item = NOT_ASSIGN;

	if (item == NOT_ASSIGN) {
		pr_err("%s : fail - get test item in QNAD!! \n", __func__);
		return count;
	}

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		return -EINVAL;
	}

	param_qnad_data.total_test_result |=
	    TEST_ITEM_RESULT(smd, item, _result);

	NAD_PRINT("total_test_result=%u, smd=%d, item=%d, _result=%d\n",
		  param_qnad_data.total_test_result, smd, item, _result);

	if (!sec_set_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - set param!! param_qnad_data\n", __func__);
		return -EINVAL;
	}

	return count;
}

static ssize_t show_nad_result(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int iCnt;

	for (iCnt = 0; iCnt < ITEM_CNT; iCnt++) {
		char strResult[NAD_BUFF_SIZE] = { '\0', };

		if (!get_nad_result(strResult, iCnt))
			goto err_out;

		info_size +=
		    snprintf((char *)(buf + info_size), MAX_LEN_STR - info_size,
			     "\"%s\":\"%s\",", STR_TEST_ITEM[iCnt], strResult);
	}

	pr_info("%s, result=%s\n", __func__, buf);

	return info_size;
err_out:
	return snprintf(buf, BUFF_SZ, "PARAM ERROR\n");
}

static DEVICE_ATTR(nad_result, S_IRUGO | S_IWUSR, show_nad_result,
		   store_nad_result);

static ssize_t store_nad_info(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	char info_name[NAD_BUFF_SIZE * 2] = { '\0', };
	char temp[NAD_BUFF_SIZE * 3] = { '\0', };
	char nad_test[2][NAD_BUFF_SIZE * 2];	// 2: "info_name", "result"
	int resultValue;
	char *nad_ptr, *string;

	NAD_PRINT("buf : %s count : %d\n", buf, (int)count);

	if (NAD_BUFF_SIZE * 3 < (int)count || (int)count < 4) {
		NAD_PRINT("result cmd size too long : NAD_BUFF_SIZE<%d\n",
			  (int)count);
		return -EINVAL;
	}

	/* Copy buf to nad temp */
	strlcpy(temp, buf, NAD_BUFF_SIZE * 3);
	string = temp;

	nad_ptr = strsep(&string, ",");
	strlcpy(nad_test[0], nad_ptr, NAD_BUFF_SIZE * 2);
	nad_ptr = strsep(&string, ",");
	strlcpy(nad_test[1], nad_ptr, NAD_BUFF_SIZE * 2);

	sscanf(nad_test[0], "%s", info_name);
	sscanf(nad_test[1], "%d", &resultValue);

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		return -EINVAL;
	}

	if (!strcmp("thermal", info_name))
		param_qnad_data.thermal = resultValue;
	else if (!strcmp("clock", info_name))
		param_qnad_data.tested_clock = resultValue;

	NAD_PRINT("info_name : %s, result=%d\n", info_name, resultValue);

	if (!sec_set_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - set param!! param_qnad_data\n", __func__);
		return -EINVAL;
	}

	return count;
}

static ssize_t show_nad_info(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
		pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
		goto err_out;
	}

	info_size +=
	    snprintf((char *)(buf + info_size), MAX_LEN_STR - info_size,
		     "\"REMAIN_CNT\":\"%d\",",
		     param_qnad_data.nad_remain_count);
	info_size +=
	    snprintf((char *)(buf + info_size), MAX_LEN_STR - info_size,
		     "\"THERMAL\":\"%d\",", param_qnad_data.thermal);
	info_size +=
	    snprintf((char *)(buf + info_size), MAX_LEN_STR - info_size,
		     "\"CLOCK\":\"%d\",", param_qnad_data.tested_clock);

	return info_size;
err_out:
	return snprintf(buf, BUFF_SZ, "PARAM ERROR\n");
}

static DEVICE_ATTR(nad_info, S_IRUGO | S_IWUSR, show_nad_info, store_nad_info);

static ssize_t show_nad_version(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	#if 0
		//QNAD_1.0.0_SLT_12212017_SDM450 - Porting
		return snprintf(buf, BUFF_SZ, "S450.0101.01.Port\n");
	
		//QNAD_1.0.0_SLT_12212017_SDM450 - change distribution of time
		return snprintf(buf, BUFF_SZ, "S450.0101.02.DisT\n");
	#else
		//QNAD_1.0.1_SLT_22022018_SDM450 - Thermal Warning enable
		return snprintf(buf, BUFF_SZ, "S450.0102.01.Therm\n");
	#endif
}

static DEVICE_ATTR(nad_version, 0444, show_nad_version, NULL);

static ssize_t store_nad_main(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int idx = 0;
	int ret = -1;
	char temp[NAD_BUFF_SIZE * 3];
	char nad_cmd[NAD_MAIN_CMD_LIST][NAD_BUFF_SIZE];
	char *nad_ptr, *string;
	int running_time;

	/* Copy buf to nad temp */
	strlcpy(temp, buf, NAD_BUFF_SIZE * 3);
	string = temp;

	while (idx < NAD_MAIN_CMD_LIST) {
		nad_ptr = strsep(&string, ",");
		strlcpy(nad_cmd[idx++], nad_ptr, NAD_BUFF_SIZE);
	}

	if (nad_test_mode == MAIN_NAD) {
		NAD_PRINT("duplicated!\n");
		return count;
	}

	if (!strncmp(buf, "start", 5)) {
		ret = sscanf(nad_cmd[1], "%d\n", &running_time);
		if (ret != 1)
			return -EINVAL;

		main_reboot = 1;

		nad_test_mode = MAIN_NAD;

		if (!sec_get_param(param_index_qnad, &param_qnad_data)) {
			pr_err("%s : fail - get param!! param_qnad_data\n", __func__);
			return -1;
		}

		param_qnad_data.ddrtest_mode = UPLOAD_CAUSE_DDR_TEST_FOR_MAIN;
		param_qnad_data.ddrtest_remain_count = 1;
		param_qnad_data.nad_remain_count = 0;
		param_qnad_data.total_test_result &= 0xffffffff;

		if (!sec_set_param(param_index_qnad, &param_qnad_data)) {
			pr_err("%s : fail - set param!! param_qnad_data\n", __func__);
			return -1;
		}

		do_nad(MAIN_NAD);
	}

	return count;
}

static ssize_t show_nad_main(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int nad_result;

	nad_result = NadResult(MAIN_NAD);
	switch (nad_result) {
	case TOTALTEST_PASS: {
		NAD_PRINT("MAIN NAD Passed\n");
		return snprintf(buf, BUFF_SZ, "OK_2.0\n");
	} break;

	case TOTALTEST_FAIL: {
		NAD_PRINT("MAIN NAD fail\n");
		return snprintf(buf, BUFF_SZ, "NG_2.0_FAIL\n");
	} break;

	default: {
		NAD_PRINT("MAIN NAD No Run\n");
		return snprintf(buf, BUFF_SZ, "OK\n");
	}
	}

	return snprintf(buf, BUFF_SZ, "MAIN NAD UNKNOWN\n");
}

static DEVICE_ATTR(balancer, S_IRUGO | S_IWUSR, show_nad_main, store_nad_main);

static ssize_t show_nad_main_timeout(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return snprintf(buf, BUFF_SZ, "%d\n", NAD_MAIN_TIMEOUT);
}

static DEVICE_ATTR(timeout, 0444, show_nad_main_timeout, NULL);

static int __init sec_nad_init(void)
{
	int ret = 0;
	struct device *sec_nad;
	struct device *sec_nad_balancer;

	NAD_PRINT("%s\n", __func__);

	/* Skip nad init when device goes to lp charging */
	if (lpcharge)
		return ret;

	sec_nad = sec_device_create(0, NULL, "sec_nad");

	if (IS_ERR(sec_nad)) {
		pr_err("%s Failed to create device(sec_nad)!\n", __func__);
		return PTR_ERR(sec_nad);
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_stat);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_ddrtest_remain_count);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_qmvs_remain_count);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_erase);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_acat);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_dram);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_support);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_logs);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_end);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_dram_debug);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_dram_err_addr);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_result);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_api);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_info);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_version);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	if (add_uevent_var(&nad_uevent, "NAD_TEST=%s", "DONE")) {
		pr_err("%s : uevent NAD_TEST_AND_PASS is failed to add\n",
		       __func__);
		goto err_create_nad_sysfs;
	}

	sec_nad_balancer = sec_device_create(0, NULL, "sec_nad_balancer");

	if (IS_ERR(sec_nad)) {
		pr_err("%s Failed to create device(sec_nad)!\n", __func__);
		return PTR_ERR(sec_nad);
	}

	ret = device_create_file(sec_nad_balancer, &dev_attr_balancer);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad_balancer, &dev_attr_timeout);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	return 0;
err_create_nad_sysfs:
	return ret;
}

module_init(sec_nad_init);
