/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Samsung's POC Driver
 * Author: ChangJae Jang <cj1225.jang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpufreq.h>
#include "../ss_dsi_panel_common.h"
#include "ss_dsi_poc_S6E3HA6_AMB577MQ01.h"

#define DEBUG_POC_CNT 100000

static int ss_poc_erase(struct samsung_display_driver_data *vdd)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	int i;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	ctrl_pdata = vdd->ctrl_dsi[DISPLAY_1];
	if (IS_ERR_OR_NULL(ctrl_pdata)) {
		LCD_ERR("ctrl_pdata is null\n");
		return -EINVAL;
	}

	LCD_INFO("ss_poc_erase !!\n");
	mdss_samsung_send_cmd(ctrl_pdata, TX_POC_ERASE);

	/* Delay 18 sec */
	for (i = 0; i < 180; i++) {
		msleep(100);
		if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
			LCD_ERR("cancel poc erase by user\n");
			ret = -EIO;
			goto cancel_poc;
		}
	}
	mdss_samsung_send_cmd(ctrl_pdata, TX_POC_ERASE1);
	LCD_INFO("ss_poc_erase done!!\n");

cancel_poc:
	atomic_set(&vdd->poc_driver.cancel, 0);
	return ret;
}

static int ss_poc_write(struct samsung_display_driver_data *vdd, char *data, u32 write_pos, u32 write_size)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct dsi_panel_cmds *poc_write_continue, *poc_write_1byte = NULL;
	int pos = 0;
	int ret = 0;
	int i;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	ctrl_pdata = vdd->ctrl_dsi[DISPLAY_1];
	if (IS_ERR_OR_NULL(ctrl_pdata)) {
		LCD_ERR("ctrl_pdata is null \n");
		return -EINVAL;
	}

	mutex_lock(&vdd->vdd_poc_operation_lock);

	vdd->poc_operation = true;

	mdss_dsi_samsung_poc_perf_mode_ctl(ctrl_pdata, true);

	if (write_pos == 0)	{
		LCD_INFO("1st, ss_poc_erase +++++\n");
		mdss_samsung_send_cmd(ctrl_pdata, TX_POC_ERASE);
		/* Delay 18 sec */
		for (i = 0; i < 180; i++) {
			msleep(100);
			if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
				LCD_ERR("cancel poc erase by user\n");
				ret = -EIO;
				goto cancel_poc;
			}
		}
		mdss_samsung_send_cmd(ctrl_pdata, TX_POC_ERASE1);
		LCD_INFO("ss_poc_erase -----\n");

		mdss_samsung_send_cmd(ctrl_pdata, TX_POC_PRE_WRITE);
	}

	LCD_INFO("ss_poc_write pos : %d size : %d +++++\n", write_pos, write_size);

	poc_write_continue = get_panel_tx_cmds(ctrl_pdata, TX_POC_WRITE_CONTINUE3);
	poc_write_1byte = get_panel_tx_cmds(ctrl_pdata, TX_POC_WRITE_1BYTE);

	for (pos = write_pos; pos < (write_pos + write_size); pos++) {
		if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
			LCD_ERR("cancel poc write by user\n");
			ret = -EIO;
			goto cancel_poc;
		}

		if (pos > 1 && (pos % 256 == 0)) {
			mdss_samsung_send_cmd(ctrl_pdata, TX_POC_WRITE_CONTINUE);
			usleep_range(4000, 4000);
			mdss_samsung_send_cmd(ctrl_pdata, TX_POC_WRITE_CONTINUE2);
			udelay(2);
			poc_write_continue->cmds[0].payload[6] = (pos & 0xFF0000) >> 16;
			poc_write_continue->cmds[0].payload[7] = (pos & 0x00FF00) >> 8;
			poc_write_continue->cmds[0].payload[8] = 0;
			mdss_samsung_send_cmd(ctrl_pdata, TX_POC_WRITE_CONTINUE3);
		}

		poc_write_1byte->cmds[0].payload[1] = data[pos];
		mdss_samsung_send_cmd(ctrl_pdata, TX_POC_WRITE_1BYTE);
		udelay(2);

		if (!(pos % DEBUG_POC_CNT))
			LCD_INFO("cur_write_pos : %d data : 0x%x\n", pos, data[pos]);
	}

cancel_poc:
	if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
		LCD_ERR("cancel poc write by user\n");
		atomic_set(&vdd->poc_driver.cancel, 0);
		ret = -EIO;
	}

	if (pos == POC_IMG_SIZE || ret == -EIO) {
		LCD_INFO("POC_IMG_SIZE : %d cur_write_pos : %d ret : %d\n", POC_IMG_SIZE, pos, ret);
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], TX_POC_POST_WRITE);
	}

	mdss_dsi_samsung_poc_perf_mode_ctl(ctrl_pdata, false);

	vdd->poc_operation = false;

	mutex_unlock(&vdd->vdd_poc_operation_lock);

	LCD_INFO("ss_poc_write -----\n");
	return ret;
}

static int ss_poc_read(struct samsung_display_driver_data *vdd, u8 *buf, u32 read_pos, u32 read_size)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct dsi_panel_cmds *poc_tx_read, *poc_rx_read = NULL;
	struct cpufreq_policy *policy;
	int cpu;
	int pos = 0;
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	LCD_INFO("ss_poc_read pos : %d size : %d+++++\n", read_pos, read_size);

	ctrl_pdata = vdd->ctrl_dsi[DISPLAY_1];
	if (IS_ERR_OR_NULL(ctrl_pdata)) {
		LCD_ERR("ctrl_pdata is null \n");
		return -EINVAL;
	}

	mutex_lock(&vdd->vdd_poc_operation_lock);

	vdd->poc_operation = true;

	get_online_cpus();
	for_each_online_cpu(cpu) {
		if (cpu < 4) {
			policy = cpufreq_cpu_get(cpu);
			if (policy) {
				policy->user_policy.min = 1900800;
				cpufreq_update_policy(cpu);
				cpufreq_cpu_put(policy);
			}
		}
	}
	put_online_cpus();

	mdss_dsi_samsung_poc_perf_mode_ctl(ctrl_pdata, true);

	poc_tx_read = get_panel_tx_cmds(ctrl_pdata, TX_POC_READ);
	poc_rx_read = get_panel_rx_cmds(ctrl_pdata, RX_POC_READ);

	mdss_samsung_send_cmd(ctrl_pdata, TX_POC_PRE_READ);
	for (pos = read_pos; pos < (read_pos + read_size) ; pos++) {
		if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
			LCD_ERR("cancel poc read by user\n");
			ret = -EIO;
			goto cancel_poc;
		}

		poc_tx_read->cmds[0].payload[6] = (pos & 0xFF0000) >> 16;
		poc_tx_read->cmds[0].payload[7] = (pos & 0x00FF00) >> 8;
		poc_tx_read->cmds[0].payload[8] = pos & 0x0000FF;

		mutex_lock(&vdd->vdd_mdss_direct_cmdlist_lock);
		mdss_samsung_send_cmd(vdd->ctrl_dsi[DISPLAY_1], TX_POC_READ);
		usleep_range(200, 200);
		mdss_samsung_panel_data_read(vdd->ctrl_dsi[DISPLAY_1], poc_rx_read, buf + pos, LEVEL_KEY_NONE);
		mutex_unlock(&vdd->vdd_mdss_direct_cmdlist_lock);

		LCD_DEBUG("[MDSS] [IOCTRL] pos = %d, 0x%x\n", pos, buf[pos]);

		if (!(pos % DEBUG_POC_CNT))
			LCD_INFO("cur_read_pos : %d data : 0x%x\n", pos, buf[pos]);
	}

cancel_poc:
	if (unlikely(atomic_read(&vdd->poc_driver.cancel))) {
		LCD_ERR("cancel poc read by user\n");
		atomic_set(&vdd->poc_driver.cancel, 0);
		ret = -EIO;
	}

	if (pos == POC_IMG_SIZE || ret == -EIO) {
		LCD_INFO("POC_IMG_SIZE : %d cur_read_pos : %d ret : %d\n", POC_IMG_SIZE, pos, ret);
		mdss_samsung_send_cmd(ctrl_pdata, TX_POC_POST_READ);
	}

	mdss_dsi_samsung_poc_perf_mode_ctl(ctrl_pdata, false);

	get_online_cpus();
	for_each_online_cpu(cpu) {
		if (cpu < 4) {
			policy = cpufreq_cpu_get(cpu);
			if (policy) {
			policy->user_policy.min = 300000;
			cpufreq_update_policy(cpu);
			cpufreq_cpu_put(policy);
			}
		}
	}
	put_online_cpus();

	vdd->poc_operation = false;

	mutex_unlock(&vdd->vdd_poc_operation_lock);

	LCD_INFO("ss_poc_read -----\n");
	return ret;
}

static int ss_poc_checksum(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("POC: checksum\n");
	return 0;
}

static int ss_poc_check_flash(struct samsung_display_driver_data *vdd)
{
	LCD_INFO("POC: check flash\n");
	return 0;
}

static int ss_dsi_poc_ctrl(struct samsung_display_driver_data *vdd, u32 cmd)
{
	int ret = 0;

	if (cmd >= MAX_POC_OP) {
		LCD_ERR("%s invalid poc_op %d\n", __func__, cmd);
		return -EINVAL;
	}

	switch (cmd) {
	case POC_OP_ERASE:
		ret = ss_poc_erase(vdd);
		break;
	case POC_OP_WRITE:
		ret = ss_poc_write(vdd,
			vdd->poc_driver.wbuf,
			POC_IMG_ADDR + vdd->poc_driver.wpos,
			vdd->poc_driver.wsize);
		if (unlikely(ret < 0)) {
			LCD_ERR("%s, failed to write poc-write-seq\n", __func__);
			return ret;
		}
		break;
	case POC_OP_READ:
		ret = ss_poc_read(vdd,
			vdd->poc_driver.rbuf,
			POC_IMG_ADDR + vdd->poc_driver.rpos,
			vdd->poc_driver.rsize);
		if (unlikely(ret < 0)) {
			LCD_ERR("%s, failed to write poc-read-seq\n", __func__);
			return ret;
		}
		break;
	case POC_OP_ERASE_WRITE_IMG:
		break;
	case POC_OP_ERASE_WRITE_TEST:
		break;
	case POC_OP_BACKUP:
		break;
	case POC_OP_CHECKSUM:
		ss_poc_checksum(vdd);
		break;
	case POC_OP_CHECK_FLASH:
		ss_poc_check_flash(vdd);
		break;
	case POC_OP_SET_FLASH_WRITE:
		break;
	case POC_OP_SET_FLASH_EMPTY:
		break;
	case POC_OP_NONE:
		break;
	default:
		LCD_ERR("%s invalid poc_op %d\n", __func__, cmd);
		break;
	}
	return ret;
}

static long ss_dsi_poc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	int ret = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -EINVAL;
	}

	LCD_INFO("POC IOCTL CMD=%d\n", cmd);

	switch (cmd) {
	case IOC_GET_POC_CHKSUM:
		ret = ss_dsi_poc_ctrl(vdd, POC_OP_CHECKSUM);
		if (ret) {
			LCD_ERR("%s error set_panel_poc\n", __func__);
			ret = -EFAULT;
		}
		if (copy_to_user((u8 __user *)arg,
				&vdd->poc_driver.chksum_res,
					sizeof(vdd->poc_driver.chksum_res))) {
			ret = -EFAULT;
			break;
		}
		break;
	case IOC_GET_POC_CSDATA:
		ret = ss_dsi_poc_ctrl(vdd, POC_OP_CHECKSUM);
		if (ret) {
			LCD_ERR("%s error set_panel_poc\n", __func__);
			ret = -EFAULT;
		}
		if (copy_to_user((u8 __user *)arg,
				vdd->poc_driver.chksum_data,
					sizeof(vdd->poc_driver.chksum_data))) {
			ret = -EFAULT;
			break;
		}
		break;
	default:
		break;
	};
	return ret;
}

static int ss_dsi_poc_open(struct inode *inode, struct file *file)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	int ret = 0;

	LCD_INFO("POC Open !!\n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -ENOMEM;
	}

	if (likely(!vdd->poc_driver.wbuf)) {
		vdd->poc_driver.wbuf = vmalloc(POC_IMG_SIZE);
		if (unlikely(!vdd->poc_driver.wbuf)) {
			LCD_ERR("%s: fail to allocate poc wbuf\n", __func__);
			return -ENOMEM;
		}
	}

	vdd->poc_driver.wpos = 0;
	vdd->poc_driver.wsize = 0;

	if (likely(!vdd->poc_driver.rbuf)) {
		vdd->poc_driver.rbuf = vmalloc(POC_IMG_SIZE);
		if (unlikely(!vdd->poc_driver.rbuf)) {
			vfree(vdd->poc_driver.wbuf);
			vdd->poc_driver.wbuf = NULL;
			LCD_ERR("%s: fail to allocate poc rbuf\n", __func__);
			return -ENOMEM;
		}
	}

	vdd->poc_driver.rpos = 0;
	vdd->poc_driver.rsize = 0;
	atomic_set(&vdd->poc_driver.cancel, 0);

	return ret;
}

static int ss_dsi_poc_release(struct inode *inode, struct file *file)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	int ret = 0;

	LCD_INFO("POC Release\n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd\n");
		return -ENOMEM;
	}

	if (unlikely(vdd->poc_driver.wbuf)) {
		vfree(vdd->poc_driver.wbuf);
	}

	if (unlikely(vdd->poc_driver.rbuf)) {
		vfree(vdd->poc_driver.rbuf);
	}

	vdd->poc_driver.wbuf = NULL;
	vdd->poc_driver.wpos = 0;
	vdd->poc_driver.wsize = 0;

	vdd->poc_driver.rbuf = NULL;
	vdd->poc_driver.rpos = 0;
	vdd->poc_driver.rsize = 0;
	atomic_set(&vdd->poc_driver.cancel, 0);

	return ret;
}

static ssize_t ss_dsi_poc_read(struct file *file, char __user *buf, size_t count,
			loff_t *ppos)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	int ret = 0;

	LCD_INFO("ss_dsi_poc_read \n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	if (unlikely(!buf)) {
		LCD_ERR("invalid read buffer\n");
		return -EINVAL;
	}

	if (unlikely(*ppos + count > POC_IMG_SIZE)) {
		LCD_ERR("POC:ERR:%s: invalid read size pos %d, size %d\n",
				__func__, (int)*ppos, (int)count);
		return -EINVAL;
	}

	vdd->poc_driver.rpos = *ppos;
	vdd->poc_driver.rsize = count;
	ret = ss_dsi_poc_ctrl(vdd, POC_OP_READ);
	if (ret) {
		LCD_ERR("fail to read poc (%d)\n", ret);
		return ret;
	}

	return simple_read_from_buffer(buf, count, ppos, vdd->poc_driver.rbuf, POC_IMG_SIZE);
}

static ssize_t ss_dsi_poc_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	int ret = 0;

	LCD_INFO("ss_dsi_poc_write : count (%d), ppos(%d)\n", (int)count, (int)*ppos);

	if (IS_ERR_OR_NULL(vdd) || IS_ERR_OR_NULL(vdd->mfd_dsi[DISPLAY_1])) {
		LCD_ERR("no vdd");
		return -ENODEV;;
	}

	if (unlikely(!buf)) {
		LCD_ERR("invalid read buffer\n");
		return -EINVAL;
	}

	if (unlikely(*ppos + count > POC_IMG_SIZE)) {
		LCD_ERR("POC:ERR:%s: invalid write size pos %d, size %d\n",
				__func__, (int)*ppos, (int)count);
		return -EINVAL;
	}

	vdd->poc_driver.wpos = *ppos;
	vdd->poc_driver.wsize = count;

	ret = simple_write_to_buffer(vdd->poc_driver.wbuf, POC_IMG_SIZE, ppos, buf, count);
	if (unlikely(ret < 0)) {
		LCD_ERR("%s, failed to simple_write_to_buffer \n", __func__);
		return ret;
	}

	ret = ss_dsi_poc_ctrl(vdd, POC_OP_WRITE);
	if (ret) {
		LCD_ERR("fail to write poc (%d)\n", ret);
		return ret;
	}

	return count;
}

static const struct file_operations poc_fops = {
	.owner = THIS_MODULE,
	.read = ss_dsi_poc_read,
	.write = ss_dsi_poc_write,
	.unlocked_ioctl = ss_dsi_poc_ioctl,
	.open = ss_dsi_poc_open,
	.release = ss_dsi_poc_release,
};

#ifdef CONFIG_DISPLAY_USE_INFO
#define EPOCEFS_IMGIDX (100)
enum {
	EPOCEFS_NOENT = 1,		/* No such file or directory */
	EPOCEFS_EMPTY = 2,		/* Empty file */
	EPOCEFS_READ = 3,		/* Read failed */
	MAX_EPOCEFS,
};

static int poc_get_efs_s32(char *filename, int *value)
{
	mm_segment_t old_fs;
	struct file *filp = NULL;
	int fsize = 0, nread, ret = 0;

	if (!filename || !value) {
		pr_err("%s invalid parameter\n", __func__);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret == -ENOENT)
			pr_err("%s file(%s) not exist\n", __func__, filename);
		else
			pr_info("%s file(%s) open error(ret %d)\n",
					__func__, filename, ret);
		set_fs(old_fs);
		return -EPOCEFS_NOENT;
	}

	if (filp->f_path.dentry && filp->f_path.dentry->d_inode)
		fsize = filp->f_path.dentry->d_inode->i_size;

	if (fsize == 0) {
		pr_err("%s invalid file(%s) size %d\n",
				__func__, filename, fsize);
		ret = -EPOCEFS_EMPTY;
		goto exit;
	}

	nread = vfs_read(filp, (char __user *)value, 4, &filp->f_pos);
	if (nread != 4) {
		pr_err("%s failed to read (ret %d)\n", __func__, nread);
		ret = -EPOCEFS_READ;
		goto exit;
	}

	pr_info("%s %s(size %d) : %d\n", __func__, filename, fsize, *value);

exit:
	filp_close(filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int poc_get_efs_image_index_org(char *filename, int *value)
{
	mm_segment_t old_fs;
	struct file *filp = NULL;
	int fsize = 0, nread, rc, ret = 0;
	char binary;
	int image_index, chksum;
	u8 buf[128];

	if (!filename || !value) {
		pr_err("%s invalid parameter\n", __func__);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret == -ENOENT)
			pr_err("%s file(%s) not exist\n", __func__, filename);
		else
			pr_info("%s file(%s) open error(ret %d)\n",
					__func__, filename, ret);
		set_fs(old_fs);
		return -EPOCEFS_NOENT;
	}

	if (filp->f_path.dentry && filp->f_path.dentry->d_inode)
		fsize = filp->f_path.dentry->d_inode->i_size;

	if (fsize == 0 || fsize > ARRAY_SIZE(buf)) {
		pr_err("%s invalid file(%s) size %d\n",
				__func__, filename, fsize);
		ret = -EPOCEFS_EMPTY;
		goto exit;
	}

	memset(buf, 0, sizeof(buf));
	nread = vfs_read(filp, (char __user *)buf, fsize, &filp->f_pos);
	if (nread != fsize) {
		pr_err("%s failed to read (ret %d)\n", __func__, nread);
		ret = -EPOCEFS_READ;
		goto exit;
	}

	rc = sscanf(buf, "%c %d %d", &binary, &image_index, &chksum);
	if (rc != 3) {
		pr_err("%s failed to sscanf %d\n", __func__, rc);
		ret = -EINVAL;
		goto exit;
	}

	pr_info("%s %s(size %d) : %c %d %d\n",
			__func__, filename, fsize, binary, image_index, chksum);

	*value = image_index;

exit:
	filp_close(filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int poc_get_efs_image_index(char *filename, int *value)
{
	mm_segment_t old_fs;
	struct file *filp = NULL;
	int fsize = 0, nread, rc, ret = 0;
	int image_index, seek;
	u8 buf[128];

	if (!filename || !value) {
		pr_err("%s invalid parameter\n", __func__);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret == -ENOENT)
			pr_err("%s file(%s) not exist\n", __func__, filename);
		else
			pr_info("%s file(%s) open error(ret %d)\n",
					__func__, filename, ret);
		set_fs(old_fs);
		return -EPOCEFS_NOENT;
	}

	if (filp->f_path.dentry && filp->f_path.dentry->d_inode)
		fsize = filp->f_path.dentry->d_inode->i_size;

	if (fsize == 0 || fsize > ARRAY_SIZE(buf)) {
		pr_err("%s invalid file(%s) size %d\n",
				__func__, filename, fsize);
		ret = -EPOCEFS_EMPTY;
		goto exit;
	}

	memset(buf, 0, sizeof(buf));
	nread = vfs_read(filp, (char __user *)buf, fsize, &filp->f_pos);
	if (nread != fsize) {
		pr_err("%s failed to read (ret %d)\n", __func__, nread);
		ret = -EPOCEFS_READ;
		goto exit;
	}

	rc = sscanf(buf, "%d,%d", &image_index, &seek);
	if (rc != 2) {
		pr_err("%s failed to sscanf %d\n", __func__, rc);
		ret = -EINVAL;
		goto exit;
	}

	pr_info("%s %s(size %d) : %d %d\n",
			__func__, filename, fsize, image_index, seek);

	*value = image_index;

exit:
	filp_close(filp, current->files);
	set_fs(old_fs);

	return ret;
}

#ifdef CONFIG_SEC_FACTORY
#define POC_TOTAL_TRY_COUNT_FILE_PATH	("/efs/FactoryApp/poc_totaltrycount")
#define POC_TOTAL_FAIL_COUNT_FILE_PATH	("/efs/FactoryApp/poc_totalfailcount")
#else
#define POC_TOTAL_TRY_COUNT_FILE_PATH	("/efs/etc/poc/totaltrycount")
#define POC_TOTAL_FAIL_COUNT_FILE_PATH	("/efs/etc/poc/totalfailcount")
#endif

#define POC_INFO_FILE_PATH	("/efs/FactoryApp/poc_info")
#define POC_USER_FILE_PATH	("/efs/FactoryApp/poc_user")

static int poc_dpui_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct POC *poc = container_of(self, struct POC, dpui_notif);
	struct dpui_info *dpui = data;
	char tbuf[MAX_DPUI_VAL_LEN];
	int total_fail_cnt;
	int total_try_cnt;
	int size, ret, poci, poci_org;

	if (dpui == NULL) {
		LCD_ERR("err: dpui is null\n");
		return 0;
	}

	if (poc == NULL) {
		LCD_ERR("err: poc is null\n");
		return 0;
	}

	ret = poc_get_efs_s32(POC_TOTAL_TRY_COUNT_FILE_PATH, &total_try_cnt);
	if (ret < 0)
		total_try_cnt = (ret > -MAX_EPOCEFS) ? ret : -1;
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", total_try_cnt);
	set_dpui_field(DPUI_KEY_PNPOCT, tbuf, size);

	ret = poc_get_efs_s32(POC_TOTAL_FAIL_COUNT_FILE_PATH, &total_fail_cnt);
	if (ret < 0)
		total_fail_cnt = (ret > -MAX_EPOCEFS) ? ret : -1;
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", total_fail_cnt);
	set_dpui_field(DPUI_KEY_PNPOCF, tbuf, size);

	ret = poc_get_efs_image_index_org(POC_INFO_FILE_PATH, &poci_org);
	if (ret < 0)
		poci_org = -EPOCEFS_IMGIDX + ret;
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", poci_org);
	set_dpui_field(DPUI_KEY_PNPOCI_ORG, tbuf, size);

	ret = poc_get_efs_image_index(POC_USER_FILE_PATH, &poci);
	if (ret < 0)
		poci = -EPOCEFS_IMGIDX + ret;
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", poci);
	set_dpui_field(DPUI_KEY_PNPOCI, tbuf, size);

	LCD_INFO("poc dpui: try=%d, fail=%d, id=%d, %d\n",
			total_try_cnt, total_fail_cnt, poci, poci_org);

	return 0;
}

static int ss_dsi_poc_register_dpui(struct POC *poc)
{
	memset(&poc->dpui_notif, 0,
			sizeof(poc->dpui_notif));
	poc->dpui_notif.notifier_call = poc_dpui_notifier_callback;

	return dpui_logging_register(&poc->dpui_notif, DPUI_TYPE_PANEL);
}
#endif	/* CONFIG_DISPLAY_USE_INFO */

static int __init ss_dsi_poc_init(void)
{
	int ret = 0;
	struct samsung_display_driver_data *vdd = samsung_get_vdd();

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR("no vdd");
		return -ENODEV;
	}

	if (!vdd->poc_driver.is_support) {
		LCD_ERR("Not Support POC Function \n");
		return -ENODEV;
	}

	LCD_INFO("POC Driver Init ++\n");

	vdd->poc_driver.dev.minor = MISC_DYNAMIC_MINOR;
	vdd->poc_driver.dev.name = "poc";
	vdd->poc_driver.dev.fops = &poc_fops;
	vdd->poc_driver.dev.parent = NULL;

	vdd->poc_driver.wbuf = NULL;
	vdd->poc_driver.rbuf = NULL;
	atomic_set(&vdd->poc_driver.cancel, 0);

	vdd->panel_func.samsung_poc_ctrl = ss_dsi_poc_ctrl;

	ret = misc_register(&vdd->poc_driver.dev);

	if (ret) {
		LCD_ERR("failed to register POC driver : %d\n", ret);
		return ret;
	}

#ifdef CONFIG_DISPLAY_USE_INFO
	ss_dsi_poc_register_dpui(&vdd->poc_driver);
#endif

	LCD_INFO("POC Driver Init --\n");
	return ret;
}
module_init(ss_dsi_poc_init);

static void __exit ss_dsi_poc_remove(void)
{
	struct samsung_display_driver_data *vdd = samsung_get_vdd();
	LCD_ERR("POC Remove \n");
	misc_deregister(&vdd->act_clock.dev);
}
module_exit(ss_dsi_poc_remove);

MODULE_DESCRIPTION("POC driver");
MODULE_LICENSE("GPL");
