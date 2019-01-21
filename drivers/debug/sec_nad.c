/* sec_nad.c
 *
 * Copyright (C) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/syscalls.h>

#include <linux/sec_class.h>
#include <linux/sec_debug.h>
#include <linux/sec_nad.h>
#include <linux/sec_param.h>

#include <asm/cacheflush.h>
#include <asm/system_misc.h>

#include <soc/qcom/restart.h>
#include <soc/qcom/subsystem_restart.h>

#define PARAM_QDAT_MAGIC		0xfaceb00c

static struct param_qnad param_qnad_data;
unsigned int clk_limit_flag;
static bool erase_pass;

bool sec_nad_get_clk_limit_flag(void)
{
	return clk_limit_flag;
}

static inline bool __sec_nad_write_param_qnad_data(void)
{
	bool err = sec_set_param(param_index_qnad,
			&param_qnad_data);

	if (unlikely(!err))
		pr_err("fail - set param!! param_qnad_data\n");

	return err;
}

static inline bool __sec_nad_read_param_qnad_data(void)
{
	bool err = sec_get_param(param_index_qnad,
			&param_qnad_data);

	if (unlikely(!err))
		pr_err("fail - get param!! param_qnad_data\n");

	return err;
}

static inline void __sec_nad_shutdown_modem_subsys(void)
{
	void *modem_subsys = subsystem_get("modem");
	const size_t retry_count = 3;
	size_t i;

	for (i = 0; modem_subsys && i < retry_count; i++)
		subsystem_put(modem_subsys);
}

#define QMVS_RESULT_PASS		0
#define QMVS_RESULT_FAIL		(-1)
#define QMVS_RESULT_NONE		(-2)
#define QMVS_RESULT_FILE_NOT_FOUND	(-2)

#define SMD_QMVS			0
#define NOT_SMD_QMVS			1

#define SMD_QMVS_RESULT			"/efs/QMVS/SMD/test_result.csv"
#define NOT_SMD_QMVS_RESULT		"/efs/QMVS/test_result.csv"

#define SMD_QMVS_LOGPATH		"logPath:/efs/QMVS/SMD"
#define NOT_SMD_QMVS_LOGPATH		"logPath:/efs/QMVS"

static int __sec_nad_get_qmvs_result(int smd)
{
	int fd, ret = QMVS_RESULT_PASS;
	char buf[SZ_256] = { '\0', };
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	switch (smd) {
	case SMD_QMVS:
		fd = sys_open(SMD_QMVS_RESULT, O_RDONLY, 0);
		break;
	case NOT_SMD_QMVS:
		fd = sys_open(NOT_SMD_QMVS_RESULT, O_RDONLY, 0);
		break;
	default:
		fd = -EINVAL;
		break;
	}

	if (fd < 0) {
		pr_err("The file is not existed\n");
		return QMVS_RESULT_FILE_NOT_FOUND;
	}

	while (sys_read(fd, buf, ARRAY_SIZE(buf))) {
		char *ptr;
		char *div = " ";
		char *tok = NULL;

		ptr = buf;
		while ((tok = strsep(&ptr, div)) != NULL) {
			if (!(strncasecmp(tok, "FAIL", strlen("FAIL")))) {
				ret = QMVS_RESULT_FAIL;
				pr_err("%s", buf);
				goto qmvs_not_passed;
			}

			if (!(strncasecmp(tok, "NONE", strlen("NONE")))) {
				ret = QMVS_RESULT_NONE;
				goto qmvs_not_passed;
			}
		}
	}

qmvs_not_passed:
	sys_close(fd);
	set_fs(old_fs);

	return ret;
}

#define USER_SPACE_PROGRAM_QMVS		"/system/vendor/bin/qmvs_sa.sh"
#define USER_SPACE_ENV_VALUE_0		"HOME=/"
#define USER_SPACE_ENV_VALUE_1		"PATH=/system/vendor/bin:/sbin:" \
		"/vendor/bin:/system/sbin:/system/bin:/system/xbin"

static inline int __sec_nad_launch_qmvs(int smd)
{
	char *argv[4] = { NULL, NULL, NULL, NULL };
	char *envp[3] = { NULL, NULL, NULL };
	int err;

	__sec_nad_shutdown_modem_subsys();

	argv[0] = USER_SPACE_PROGRAM_QMVS;
	switch (smd) {
	case SMD_QMVS:
		argv[1] = SMD_QMVS_LOGPATH;
		break;
	case NOT_SMD_QMVS:
		argv[1] = NOT_SMD_QMVS_LOGPATH;
		break;
	default:
		/* FIXME: is this fine? */
		break;
	}

	envp[0] = USER_SPACE_ENV_VALUE_0;
	envp[1] = USER_SPACE_ENV_VALUE_1;

	err = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
	if (!err) {
		pr_info("qmvs_sa.sh is executed.\n");
		if (erase_pass)
			erase_pass = false;
	} else {
		pr_warn("qmvs_sa.sh is NOT executed. ret = %d\n", err);
	}

	/* FIXME: despite of MSM8996 & MSM8998, MSM8953 does not use
	 * 'cpufreq limit at boot time' feature. */
	clk_limit_flag = 1;

	return err;
}

static inline int __sec_nad_launch_ddr_test(void)
{
	/* to ensure cold reset */
	set_dload_mode(0);

	sec_debug_hw_reset();

	return 0;
}

static ssize_t show_nad_acat(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int qmvs_result;

	if (!__sec_nad_read_param_qnad_data())
		return scnprintf(buf, PAGE_SIZE, "ERROR\n");

	pr_info("magic %x, qmvs cnt %u, ddr cnt %u, ddr result %u\n",
		param_qnad_data.magic,
		param_qnad_data.qmvs_remain_count,
		param_qnad_data.ddrtest_remain_count,
		param_qnad_data.ddrtest_result);

	qmvs_result = __sec_nad_get_qmvs_result(NOT_SMD_QMVS);

	if (QMVS_RESULT_PASS == qmvs_result) {
		pr_info("QMVS Passed\n");
		return scnprintf(buf, PAGE_SIZE, "OK_ACAT_NONE\n");
	} else if (QMVS_RESULT_FAIL == qmvs_result) {
		pr_warn("QMVS fail\n");
		return scnprintf(buf, PAGE_SIZE, "NG_ACAT_ASV\n");
	}

	pr_warn("QMVS No Run\n");
	return scnprintf(buf, PAGE_SIZE, "OK\n");
}

static ssize_t store_nad_acat(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	uint32_t qmvs_loop_count, dram_loop_count;
	char temp[NAD_BUFF_SIZE * NAD_CMD_LIST];
	char nad_cmd[NAD_CMD_LIST][NAD_BUFF_SIZE];
	char *nad_ptr, *string;
	size_t idx;
	int err;

	pr_info("buf : %s count : %zu\n", buf, count);

	if (count < NAD_BUFF_SIZE)
		return -EINVAL;

	/* Copy buf to nad temp */
	strlcpy(temp, buf, ARRAY_SIZE(temp));
	string = temp;

	idx = 0;
	while (idx < NAD_CMD_LIST) {
		nad_ptr = strsep(&string, ",");
		strlcpy(nad_cmd[idx], nad_ptr, NAD_BUFF_SIZE - idx);
		idx++;
	}

	if (!__sec_nad_read_param_qnad_data())
		goto err_out;

	if (NAD_CMD_LIST != idx || strncmp(buf, "nad_acat", strlen("nad_acat")))
		goto err_out;

#if defined(CONFIG_ARCH_MSM8917)
retry:
#endif
	/* checking 1st boot and setting test count */
	if (PARAM_QDAT_MAGIC != param_qnad_data.magic) {
		pr_info("1st boot at SMD\n");
		param_qnad_data.magic = PARAM_QDAT_MAGIC;
		param_qnad_data.qmvs_remain_count = 0;
		param_qnad_data.ddrtest_remain_count = 0;

		/* flushing to param partition */
		if (!__sec_nad_write_param_qnad_data())
			goto err_out;

		err = __sec_nad_launch_qmvs(SMD_QMVS);
		if (err < 0) {
			pr_err("launching qmvs failed (%d)\n", err);
#if defined(CONFIG_ARCH_MSM8917)
			goto retry;
#else
			goto err_out;
#endif
		}
	} else {
		if (kstrtou32(nad_cmd[1], 0, &qmvs_loop_count))
			return -EINVAL;

		if (kstrtou32(nad_cmd[2], 0, &dram_loop_count))
			return -EINVAL;

		/* not (0, 0) means
		 *  1. new test count came
		 *  2. so overwrite the remain_count
		 *  3. and NOT reboot by itself
		 *  4. reboot cmd will come from outside like factory PGM
		 */
		if (qmvs_loop_count || dram_loop_count) {
			param_qnad_data.qmvs_remain_count = qmvs_loop_count;
			param_qnad_data.ddrtest_remain_count = dram_loop_count;

			/* flushing to param partition */
			if (!__sec_nad_write_param_qnad_data())
				goto err_out;

			pr_info("new cmd : qmvs_remain_count = %u, ddrtest_remain_count = %u\n",
					qmvs_loop_count, dram_loop_count);

			goto out;
		}

		/* (0, 0) means
		 *  1. veryfying QMVS testing result
		 *    1-1. if failed reset remain count and exit
		 *  2. launching testing
		 *    2-1. launch qmvs until qmvs_remain_count be 0.
		 *    2-2. lauunc drm-test until remain count be 0.
		 */
		if (__sec_nad_get_qmvs_result(NOT_SMD_QMVS) ==
				QMVS_RESULT_FAIL) {
			pr_err("qmvs test fail - set the remain counts to 0\n");
			param_qnad_data.qmvs_remain_count = 0;
			param_qnad_data.ddrtest_remain_count = 0;

			/* flushing to param partition */
			if (!__sec_nad_write_param_qnad_data())
				goto err_out;
		}

		pr_info("qmvs_remain_count = %u, ddrtest_remain_count = %u\n",
					param_qnad_data.qmvs_remain_count,
					param_qnad_data.ddrtest_remain_count);

		if (param_qnad_data.qmvs_remain_count > 0) {
			param_qnad_data.qmvs_remain_count--;

			/* flushing to param partition */
			if (!__sec_nad_write_param_qnad_data()) {
				param_qnad_data.qmvs_remain_count++;
				goto err_out;
			}

			err = __sec_nad_launch_qmvs(NOT_SMD_QMVS);
			if (err < 0) {
				pr_err("launching qmvs failed (%d)\n", err);
				goto err_out;
			}
		} else if (param_qnad_data.ddrtest_remain_count > 0) {
			err = __sec_nad_launch_ddr_test();
			if (err < 0) {
				pr_err("launching ddr test failed (%d)\n", err);
				goto err_out;
			}
		}
	}

err_out:
out:
	return count;
}

static DEVICE_ATTR(nad_acat, S_IRUGO | S_IWUSR, show_nad_acat, store_nad_acat);

static ssize_t show_nad_stat(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	int qmvs_result;

	if (!__sec_nad_read_param_qnad_data())
		return scnprintf(buf, PAGE_SIZE, "ERROR\n");

	pr_info("magic %x, qmvs cnt %u, ddr cnt %u, ddr result %u\n",
		param_qnad_data.magic,
		param_qnad_data.qmvs_remain_count,
		param_qnad_data.ddrtest_remain_count,
		param_qnad_data.ddrtest_result);

	if (PARAM_QDAT_MAGIC != param_qnad_data.magic) {
		pr_warn("QMVS No Run\n");
		return scnprintf(buf, PAGE_SIZE, "OK\n");
	}

	qmvs_result = __sec_nad_get_qmvs_result(SMD_QMVS);

	if (QMVS_RESULT_PASS == qmvs_result) {
		pr_info("QMVS Passed\n");
		return scnprintf(buf, PAGE_SIZE, "OK_2.0\n");
	} else if (QMVS_RESULT_FAIL == qmvs_result) {
		pr_warn("QMVS fail\n");
		return scnprintf(buf, PAGE_SIZE, "NG_2.0_FAIL\n");
	}

	pr_warn("QMVS Magic exist and Empty log\n");
	return scnprintf(buf, PAGE_SIZE, "RE_WORK\n");
}

static DEVICE_ATTR(nad_stat, S_IRUGO, show_nad_stat, NULL);

static ssize_t show_ddrtest_remain_count(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	if (!__sec_nad_read_param_qnad_data())
		goto err_out;

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			param_qnad_data.ddrtest_remain_count);

err_out:
	return scnprintf(buf, PAGE_SIZE, "PARAM ERROR\n");
}

static DEVICE_ATTR(nad_ddrtest_remain_count, S_IRUGO, show_ddrtest_remain_count,
		   NULL);

static ssize_t show_qmvs_remain_count(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	if (!__sec_nad_read_param_qnad_data())
		goto err_out;

	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 param_qnad_data.qmvs_remain_count);

err_out:
	return scnprintf(buf, PAGE_SIZE, "PARAM ERROR\n");
}

static DEVICE_ATTR(nad_qmvs_remain_count, S_IRUGO, show_qmvs_remain_count,
		   NULL);

#define USER_SPACE_PROGRAM_REMOVE	"/system/vendor/bin/remove_files.sh"

static ssize_t store_nad_erase(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	char *argv[4] = { NULL, NULL, NULL, NULL };
	char *envp[3] = { NULL, NULL, NULL };
	int ret_userapp;

	if (strncmp(buf, "erase", strlen("erase")))
		goto err_out;

	argv[0] = USER_SPACE_PROGRAM_REMOVE;

	envp[0] = USER_SPACE_ENV_VALUE_0;
	envp[1] = USER_SPACE_ENV_VALUE_1;

	ret_userapp = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);

	if (ret_userapp) {
		pr_warn("remove_files.sh is NOT executed. ret_userapp = %d\n",
				ret_userapp);
		erase_pass = false;
	} else {
		pr_info("remove_files.sh is executed. ret_userapp = %d\n",
				ret_userapp);
		erase_pass = true;
	}

	if (!__sec_nad_read_param_qnad_data())
		goto err_out;

	param_qnad_data.magic = 0x0;
	param_qnad_data.qmvs_remain_count = 0;
	param_qnad_data.ddrtest_remain_count = 0;
	param_qnad_data.ddrtest_result = 0;

	/* flushing to param partition */
	if (!__sec_nad_write_param_qnad_data())
		goto err_out;

	pr_info("clearing MAGIC code done = %x\n", param_qnad_data.magic);
	pr_info("qmvs_remain_count = %d\n", param_qnad_data.qmvs_remain_count);
	pr_info("ddrtest_remain_count = %d\n",
			param_qnad_data.ddrtest_remain_count);
	pr_info("ddrtest_result = %d\n", param_qnad_data.ddrtest_result);

err_out:
	return count;
}

static ssize_t show_nad_erase(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	if (erase_pass)
		return scnprintf(buf, PAGE_SIZE, "OK\n");
	else
		return scnprintf(buf, PAGE_SIZE, "NG\n");
}

static DEVICE_ATTR(nad_erase, S_IRUGO | S_IWUSR, show_nad_erase,
		   store_nad_erase);

#define DRAM_TEST_RESULT_OK		0x11
#define DRAM_TEST_RESULT_NG		0x22

static ssize_t show_nad_dram(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	if (!__sec_nad_read_param_qnad_data())
		goto err_out;

	if (DRAM_TEST_RESULT_OK == param_qnad_data.ddrtest_result)
		return scnprintf(buf, PAGE_SIZE, "OK_DRAM\n");
	else if (DRAM_TEST_RESULT_NG == param_qnad_data.ddrtest_result)
		return scnprintf(buf, PAGE_SIZE, "NG_DRAM_DATA\n");
	else
		return scnprintf(buf, PAGE_SIZE, "NO_DRAMTEST\n");

err_out:
	return scnprintf(buf, PAGE_SIZE, "READ ERROR\n");
}

static DEVICE_ATTR(nad_dram, S_IRUGO, show_nad_dram, NULL);

static ssize_t show_nad_support(struct device *dev,
				struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_ARCH_MSM8998) || defined(CONFIG_ARCH_MSM8996) || \
	defined(CONFIG_SEC_MSM8953_PROJECT)
	return scnprintf(buf, PAGE_SIZE, "SUPPORT\n");
#else
	return scnprintf(buf, PAGE_SIZE, "NOT_SUPPORT\n");
#endif
}

static DEVICE_ATTR(nad_support, S_IRUGO, show_nad_support, NULL);


static ssize_t show_nad_dram_debug(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!__sec_nad_read_param_qnad_data()) {
		pr_err("fail - get param!! param_qnad_data\n");
		goto err_out;
	}

	return scnprintf(buf, PAGE_SIZE, "0x%x\n",
			param_qnad_data.ddrtest_result);

err_out:
	return scnprintf(buf, PAGE_SIZE, "READ ERROR\n");
}
static DEVICE_ATTR(nad_dram_debug, S_IRUGO, show_nad_dram_debug, NULL);

static ssize_t show_nad_dram_err_addr(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	size_t i;

	struct param_qnad_ddr_result param_qnad_ddr_result_data;

	if (!__sec_nad_read_param_qnad_data()) {
		pr_err("fail - get param!! param_qnad_ddr_result_data\n");
		goto err_out;
	}

	ret = scnprintf(buf, PAGE_SIZE, "Total : %d\n\n",
			param_qnad_ddr_result_data.ddr_err_addr_total);
	for (i = 0; i < param_qnad_ddr_result_data.ddr_err_addr_total; i++)
		ret += scnprintf(buf + ret - 1, PAGE_SIZE - ret,
				 "[%zu] 0x%llx\n", i,
				 param_qnad_ddr_result_data.ddr_err_addr[i]);

	return ret;

err_out:
	return scnprintf(buf, PAGE_SIZE, "READ ERROR\n");
}
static DEVICE_ATTR(nad_dram_err_addr, S_IRUSR | S_IRGRP, show_nad_dram_err_addr, NULL);


static ssize_t store_qmvs_logs(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int fd;
	char logbuf[SZ_256];
	char path[SZ_128];
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pr_info("%s\n", buf);

	if (!strlcpy(path, buf, ARRAY_SIZE(path)))
		goto err_out;

	fd = sys_open(path, O_RDONLY, 0);
	if (unlikely(fd < 0)) {
		pr_err("The File is not existed\n");
		goto err_out;
	}

	while (sys_read(fd, logbuf, ARRAY_SIZE(logbuf)))
		pr_info("%s", logbuf);

	sys_close(fd);

err_out:
	set_fs(old_fs);
	return count;
}
static DEVICE_ATTR(nad_logs, S_IWUSR, NULL, store_qmvs_logs);

static struct kobj_uevent_env nad_uevent;

static ssize_t store_qmvs_end(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char result[SZ_32] = {'\0'};
	void *modem_subsys;
	int err;

	/* FIXME: what is the purpose of this code? */
	modem_subsys = subsystem_get("modem");
	if (modem_subsys)
		pr_info("failed to get modem\n");

	pr_info("result : %s\n", buf);

	err = strlcpy(result, buf, ARRAY_SIZE(result));
	if (unlikely(err > 0) &&
	    !strncmp(result, "nad_pass", strlen("nad_pass"))) {
		kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, nad_uevent.envp);
		pr_info("Send to Process App for Nad test end\n");
	} else
		panic(result);

	return count;
}
static DEVICE_ATTR(nad_end, S_IWUSR, NULL, store_qmvs_end);

static struct attribute *sec_nad_attributes[] = {
	&dev_attr_nad_acat.attr,
	&dev_attr_nad_ddrtest_remain_count.attr,
	&dev_attr_nad_dram.attr,
	&dev_attr_nad_erase.attr,
	&dev_attr_nad_qmvs_remain_count.attr,
	&dev_attr_nad_stat.attr,
	&dev_attr_nad_support.attr,
	&dev_attr_nad_dram_debug.attr,
	&dev_attr_nad_dram_err_addr.attr,
	&dev_attr_nad_logs.attr,
	&dev_attr_nad_end.attr,
	NULL
};

static const struct attribute_group sec_nad_attribute_group = {
	.attrs = sec_nad_attributes,
};

static int __init sec_nad_init(void)
{
	struct device *sec_nad;
	int err;

	pr_info("initialize\n");

#if 0	/* FIXME: I think this feature only available in LSI project. */
	/* Skip nad init when device goes to lp charging */
	if (lpcharge)
		return 0;
#endif

	sec_nad = sec_device_create(0, NULL, "sec_nad");

	if (IS_ERR(sec_nad)) {
		pr_err("Failed to create device(sec_nad)!\n");
		return PTR_ERR(sec_nad);
	}

	err = sysfs_create_group(&sec_nad->kobj, &sec_nad_attribute_group);
	if (unlikely(err)) {
		pr_err("Failed to create device file!\n");
		goto err_create_nad_sysfs;
	}

	err = add_uevent_var(&nad_uevent, "NAD_TEST_END_RESULT=%s", "PASS");
	if (unlikely(err)) {
		pr_err("uevent NAD_TEST_AND_PASS is failed to add\n");
		goto err_create_nad_uevent;
	}

	return 0;

err_create_nad_uevent:
	sysfs_remove_group(&sec_nad->kobj, &sec_nad_attribute_group);

err_create_nad_sysfs:
	sec_device_destroy(sec_nad->devt);

	return err;
}
module_init(sec_nad_init);
