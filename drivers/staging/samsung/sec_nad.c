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

#include <linux/device.h>
#include <linux/sec_sysfs.h>
#include <linux/sec_nad.h>
#include <linux/fs.h>

#define NAD_PRINT(format, ...) printk("[NAD] " format, ##__VA_ARGS__)

#if defined(CONFIG_SEC_FACTORY)
static void sec_nad_param_update(struct work_struct *work)
{
	int ret = -1;
	struct file *fp;
	struct sec_nad_param *param_data =
		container_of(work, struct sec_nad_param, sec_nad_work);

	NAD_PRINT("%s: set param at %s\n", __func__, param_data->state ? "write" : "read");

	fp = filp_open(NAD_PARAM_NAME, O_WRONLY | O_SYNC, 0);
	if (IS_ERR(fp)) {
		pr_err("%s: filp_open error %ld\n", __func__, PTR_ERR(fp));
		return;
	}
	ret = fp->f_op->llseek(fp, -param_data->offset, SEEK_END);
	if (ret < 0) {
		pr_err("%s: llseek error %d!\n", __func__, ret);
		goto close_fp_out;
	}

	switch(param_data->state){
	case NAD_PARAM_WRITE:
		strcpy(param_data->val, "erase");

		ret = fp->f_op->write(fp, param_data->val, sizeof(param_data->val), &(fp->f_pos));
		if (ret < 0)
			pr_err("%s: write error! %d\n", __func__, ret);
		break;
	case NAD_PARAM_READ:
		ret = fp->f_op->read(fp, param_data->val, sizeof(param_data->val), &(fp->f_pos));
		if (ret < 0)
			pr_err("%s: read error! %d\n", __func__, ret);
        
		strcpy(sec_nad_param_data.val, param_data->val);
		break;
	}
close_fp_out:
	if (fp)
		filp_close(fp, NULL);

	NAD_PRINT("%s: exit %d\n", __func__, ret);
	return;
}

int sec_set_nad_param(int val)
{
	int ret = -1;

	mutex_lock(&sec_nad_param_mutex);

	switch(val) {
	case NAD_PARAM_WRITE:
	case NAD_PARAM_READ:
		goto set_param;
	default:
		goto unlock_out;
	}

set_param:
	sec_nad_param_data.state = val;
	schedule_work(&sec_nad_param_data.sec_nad_work);

	/* how to determine to return success or fail ? */
	ret = 0;
unlock_out:
	mutex_unlock(&sec_nad_param_mutex);
	return ret;
}


static int __init get_nad_data(char *str)
{
	get_option(&str, &nad_data);

	NAD_PRINT("data : 0x%x\n", nad_data);
	return 0;
}
early_param("NAD_DATA", get_nad_data);

static int __init get_nad_result(char *str)
{
	/* Read nad result */
	strncat(nad_result, str, sizeof(nad_result) - 1);

	NAD_PRINT("result : %s\n", nad_result);
	return 0;
}
early_param("NAD_RESULT", get_nad_result);

static ssize_t show_nad_stat(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	if (!strncasecmp(nad_result, "NG", 2))
	    return sprintf(buf, "NG_2.0_0x%x\n", nad_data);
	else
	    return sprintf(buf, "OK_2.0\n");

}
static DEVICE_ATTR(nad_stat, S_IRUGO, show_nad_stat, NULL);

static ssize_t store_nad_erase(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	int ret = -1;

	if (!strncmp(buf, "erase", 5)) {
		ret = sec_set_nad_param(NAD_PARAM_WRITE);
		if (ret < 0)
			pr_err("%s: write error! %d\n", __func__, ret);

		return count;
	} else
		return count;
}

static ssize_t show_nad_erase(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	int ret = -1;
	
	ret = sec_set_nad_param(NAD_PARAM_READ);
	if (ret < 0) {
		pr_err("%s: read error! %d\n", __func__, ret);
		goto err_out;
	}

	if (!strncmp(sec_nad_param_data.val, "erase", 5))
	    return sprintf(buf, "OK\n");
	else
	    return sprintf(buf, "NG\n");

err_out:
    return sprintf(buf, "READ ERROR\n");
}

static DEVICE_ATTR(nad_erase, S_IRUGO | S_IWUSR, show_nad_erase, store_nad_erase);
#endif

static int __init sec_nad_init(void)
{
	int ret = 0;
#if defined(CONFIG_SEC_FACTORY)
	struct device* sec_nad;

	NAD_PRINT("%s\n", __func__);

	sec_nad = sec_device_create(NULL, "sec_nad");

	if (IS_ERR(sec_nad)) {
		pr_err("%s Failed to create device(sec_nad)!\n", __func__);
		return PTR_ERR(sec_nad);
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_stat); 
	if(ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_erase); 
	if(ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

    /* Initialize nad param struct */
	sec_nad_param_data.offset = NAD_OFFSET;
	sec_nad_param_data.state = NAD_PARAM_EMPTY;
	strcpy(sec_nad_param_data.val, "empty");

	INIT_WORK(&sec_nad_param_data.sec_nad_work, sec_nad_param_update);

	return 0;
err_create_nad_sysfs:
	sec_device_destroy(sec_nad->devt);
	return ret;
#else
	return ret;
#endif
}

static void __exit sec_nad_exit(void)
{
#if defined(CONFIG_SEC_FACTORY)
	cancel_work_sync(&sec_nad_param_data.sec_nad_work);
	NAD_PRINT("%s: exit\n", __func__);
#endif
}

module_init(sec_nad_init);
module_exit(sec_nad_exit);
