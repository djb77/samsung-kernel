/*
 * ccic_sysfs.c
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

#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h> 
#include <linux/uaccess.h> 
#include <linux/ccic/ccic_sysfs.h>
#include <linux/ccic/s2mm005.h>
#include <linux/ccic/s2mm005_ext.h>
#include <linux/ccic/s2mm005_fw.h>
#include <linux/ccic/ccic_alternate.h>

static ssize_t s2mm005_cur_ver_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct s2mm005_version chip_swver;
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);

	s2mm005_get_chip_swversion(usbpd_data, &chip_swver);
	pr_err("%s CHIP SWversion %2x %2x %2x %2x\n", __func__,
	       chip_swver.main[2] , chip_swver.main[1], chip_swver.main[0], chip_swver.boot);

	usbpd_data->firm_ver[0] = chip_swver.main[2];
	usbpd_data->firm_ver[1] = chip_swver.main[1];
	usbpd_data->firm_ver[2] = chip_swver.main[0];
	usbpd_data->firm_ver[3] = chip_swver.boot;

	return sprintf(buf, "%02X %02X %02X %02X\n",
		       usbpd_data->firm_ver[0], usbpd_data->firm_ver[1],
		       usbpd_data->firm_ver[2], usbpd_data->firm_ver[3]);

}
static DEVICE_ATTR(cur_version, 0444, s2mm005_cur_ver_show, NULL);

static ssize_t s2mm005_src_ver_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	struct s2mm005_version fw_swver;

	s2mm005_get_fw_version(&fw_swver, usbpd_data->firm_ver[3], usbpd_data->hw_rev);
	return sprintf(buf, "%02X %02X %02X %02X\n",
		fw_swver.main[2], fw_swver.main[1], fw_swver.main[0], fw_swver.boot);
}
static DEVICE_ATTR(src_version, 0444, s2mm005_src_ver_show, NULL);

static ssize_t s2mm005_show_manual_lpm_mode(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);


    if (!usbpd_data) {
        pr_err("usbpd_data is NULL\n");
        return -ENODEV;
    }

    return sprintf(buf, "%d\n", usbpd_data->manual_lpm_mode);	
}
static ssize_t s2mm005_store_manual_lpm_mode(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	int mode;

    if (!usbpd_data) {
        pr_err("usbpd_data is NULL\n");
        return -ENODEV;
    }

	sscanf(buf, "%d", &mode);
	pr_info("usb: %s mode=%d\n", __func__, mode);

	switch(mode){
	case 0:
		/* Disable Low Power Mode for App (JIGON Low + LP Off) */
		s2mm005_manual_LPM(usbpd_data, 0x3);
		s2mm005_manual_JIGON(usbpd_data, 0);
		usbpd_data->manual_lpm_mode = 0;
		break;
	case 1:
		/* Enable Low Power Mode (JIGON High + Force LP On) */
		s2mm005_manual_JIGON(usbpd_data, 1);
		s2mm005_manual_LPM(usbpd_data, 0x8);	// force Low power mode
		usbpd_data->manual_lpm_mode = 1;
		break;
	case 2:
		/* Enable Low Power Mode (Normal LP On) */
		s2mm005_manual_JIGON(usbpd_data, 1);
		s2mm005_manual_LPM(usbpd_data, 0x1);	// normal power mode
		usbpd_data->manual_lpm_mode = 1;
		break;
	case 3:
		/* Disable Low Power Mode (LP Off) */
		s2mm005_manual_LPM(usbpd_data, 0x3);
		usbpd_data->manual_lpm_mode = 0;
		break;
	default:
		/* Disable Low Power Mode (JIGON Low + LP Off) */
		s2mm005_manual_LPM(usbpd_data, 0x3);
		s2mm005_manual_JIGON(usbpd_data, 0);
		usbpd_data->manual_lpm_mode = 0;
		break;		
	}

	return size;
}
static DEVICE_ATTR(lpm_mode, 0664, 
	s2mm005_show_manual_lpm_mode, s2mm005_store_manual_lpm_mode);

static ssize_t ccic_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);

	if(!usbpd_data) {
		pr_err("%s usbpd_data is null!!\n", __func__);
		return -ENODEV;
	}
	
	return sprintf(buf, "%d\n", usbpd_data->pd_state);
}
static DEVICE_ATTR(state, 0444, ccic_state_show, NULL);

#if defined(CONFIG_SEC_FACTORY)
static ssize_t ccic_rid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);

	if(!usbpd_data) {
		pr_err("%s usbpd_data is null!!\n", __func__);
		return -ENODEV;
	}
	
	return sprintf(buf, "%d\n", usbpd_data->cur_rid);
}
static DEVICE_ATTR(rid, 0444, ccic_rid_show, NULL);

static ssize_t s2mm005_store_control_option_command(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	int cmd;

	if(!usbpd_data) {
		pr_err("%s usbpd_data is null!!\n", __func__);
		return -ENODEV;
	}

	sscanf(buf, "%d", &cmd);
	pr_info("usb: %s mode=%d\n", __func__, cmd);

	s2mm005_control_option_command(usbpd_data, cmd);

	return size;
}
static DEVICE_ATTR(ccic_control_option, 0220, NULL, s2mm005_store_control_option_command);
#endif
static int ccic_firmware_update_built_in(struct device *dev)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	struct s2mm005_version chip_swver, fw_swver;

	s2mm005_get_chip_swversion(usbpd_data, &chip_swver);
	pr_err("%s CHIP SWversion %2x %2x %2x %2x - before\n", __func__,
	       chip_swver.main[2] , chip_swver.main[1], chip_swver.main[0], chip_swver.boot);
	s2mm005_get_fw_version(&fw_swver, chip_swver.boot, usbpd_data->hw_rev);
	pr_err("%s SRC SWversion:%2x,%2x,%2x,%2x\n",__func__,
		fw_swver.main[2], fw_swver.main[1], fw_swver.main[0], fw_swver.boot);

	pr_err("%s: FW UPDATE boot:%01d hw_rev:%02d\n", __func__, chip_swver.boot, usbpd_data->hw_rev);

	if(chip_swver.main[0] == fw_swver.main[0]) {
		pr_err("%s: FW version is same. Stop FW update. src:%2x chip:%2x\n", 
			__func__, chip_swver.main[0], fw_swver.main[0]);
		goto done;
	}

	if (chip_swver.boot == 4) {
		if (usbpd_data->hw_rev >= 9)
			s2mm005_flash_fw(usbpd_data, FLASH_WRITE);
		else
			s2mm005_flash_fw(usbpd_data, FLASH_WRITE4);
	} else if (chip_swver.boot == 5) {
		if (usbpd_data->hw_rev >= 9)
			s2mm005_flash_fw(usbpd_data, FLASH_WRITE5);
		else
			s2mm005_flash_fw(usbpd_data, FLASH_WRITE5_NODPDM);
	} else if (chip_swver.boot == 6) {
		if (usbpd_data->hw_rev >= 9)
			s2mm005_flash_fw(usbpd_data, FLASH_WRITE6);
	} else {
		pr_err("%s: Didn't have same FW boot version. Stop FW update. src_boot:%2x chip_boot:%2x\n", 
			__func__, chip_swver.boot, fw_swver.boot);
		return -1;
	}
done:
	return 0;
}

static int ccic_firmware_update_ums(struct device *dev)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	unsigned char *fw_data;
	struct s2mm005_fw *fw_hd;
	struct file *fp; 
	mm_segment_t old_fs; 
	long fw_size, nread; 
	int error = 0; 

	if(!usbpd_data) {
		pr_err("%s usbpd_data is null!!\n", __func__);
		return -ENODEV;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(CCIC_DEFAULT_UMS_FW, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		pr_err("%s: failed to open %s.\n", __func__,
						CCIC_DEFAULT_UMS_FW);
		error = -ENOENT;
		goto open_err;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (0 < fw_size) {
		fw_data = kzalloc(fw_size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)fw_data, fw_size, &fp->f_pos);

		pr_info("%s: start, file path %s, size %ld Bytes\n",
					__func__, CCIC_DEFAULT_UMS_FW, fw_size);
		filp_close(fp, NULL);

		if (nread != fw_size) {
			pr_err("%s: failed to read firmware file, nread %ld Bytes\n",
					__func__, nread);
			error = -EIO;
		} else {
			fw_hd = (struct s2mm005_fw*)fw_data;
			pr_info("CCIC FW ver - cur:%02X %02X %02X %02X / bin:%02X %02X %02X %02X\n",
					usbpd_data->firm_ver[0], usbpd_data->firm_ver[1], usbpd_data->firm_ver[2], usbpd_data->firm_ver[3],
					fw_hd->boot, fw_hd->main[0], fw_hd->main[1], fw_hd->main[2]);

			if(fw_hd->boot == usbpd_data->firm_ver[3]) {
				if (s2mm005_flash_fw(usbpd_data, FLASH_WRITE_UMS) >= 0)
					goto done;
			} else {
				pr_err("error : Didn't match to CCIC FW firmware version\n");
				error = -EINVAL;
			}
		}
		if (error < 0)
			pr_err("%s: failed update firmware\n", __func__);
done:
		kfree(fw_data);
	}

open_err:
	set_fs(old_fs);
	return error; 
}

static ssize_t ccic_store_firmware_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	u8 val = 0;

	if(!usbpd_data) {
		pr_err("%s usbpd_data is null!!\n", __func__);
		return -ENODEV;
	}
	s2mm005_read_byte_flash(usbpd_data->i2c, FLASH_STATUS_0x24, &val, 1);
	pr_err("%s flash mode: %s\n", __func__, flashmode_to_string(val));

	return sprintf(buf, "%s\n", flashmode_to_string(val));
}
static DEVICE_ATTR(fw_update_status, 0444, ccic_store_firmware_status_show, NULL);

static ssize_t ccic_store_firmware_update(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	struct s2mm005_version version;
	int mode = 0,  ret = 1;

    if (!usbpd_data) {
        pr_err("usbpd_data is NULL\n");
        return -ENODEV;
    }

	sscanf(buf, "%d", &mode);
	pr_info("%s mode=%d\n", __func__, mode);

	s2mm005_get_chip_swversion(usbpd_data, &version);
	pr_err("%s CHIP SWversion %2x %2x %2x %2x - before\n", __func__,
	       version.main[2] , version.main[1], version.main[0], version.boot);

	/* Factory cmd for firmware update
 	* argument represent what is source of firmware like below.
 	*
 	* 0 : [BUILT_IN] Getting firmware from source.
 	* 1 : [UMS] Getting firmware from sd card.
 	*/

	switch (mode) {
	case BUILT_IN:
		ret = ccic_firmware_update_built_in(dev);
		break;
	case UMS:
		ret = ccic_firmware_update_ums(dev);
		break;
	default:
		pr_err("%s: Not support command[%d]\n",
			__func__, mode);
		break;
	}

	s2mm005_get_chip_swversion(usbpd_data, &version);
	pr_err("%s CHIP SWversion %2x %2x %2x %2x - after\n", __func__,
	       version.main[2] , version.main[1], version.main[0], version.boot);

	return size;
}
static DEVICE_ATTR(fw_update, 0220, NULL, ccic_store_firmware_update);

#if defined(CONFIG_CCIC_ALTERNATE_MODE)
static ssize_t ccic_send_uVDM_message(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	int cmd = 0;

    if (!usbpd_data) {
        pr_err("usbpd_data is NULL\n");
        return -ENODEV;
    }

	sscanf(buf, "%d", &cmd);
	pr_info("%s cmd=%d\n", __func__, cmd);

	send_unstructured_vdm_message(usbpd_data, cmd);

	return size;
}
static DEVICE_ATTR(uvdm, 0220, NULL, ccic_send_uVDM_message);
static ssize_t ccic_send_attention_message(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	int cmd = 0;

    if (!usbpd_data) {
        pr_err("usbpd_data is NULL\n");
        return -ENODEV;
    }

	sscanf(buf, "%d", &cmd);
	pr_info("%s cmd=%d\n", __func__, cmd);

	send_attention_message(usbpd_data, cmd);

	return size;
}
static DEVICE_ATTR(attention, 0220, NULL, ccic_send_attention_message);
static ssize_t ccic_send_role_swap_message(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct s2mm005_data *usbpd_data = dev_get_drvdata(dev);
	int cmd = 0;

    if (!usbpd_data) {
        pr_err("usbpd_data is NULL\n");
        return -ENODEV;
    }

	sscanf(buf, "%d", &cmd);
	pr_info("%s cmd=%d\n", __func__, cmd);

	send_role_swap_message(usbpd_data, cmd);

	return size;
}
static DEVICE_ATTR(role_swap, 0220, NULL, ccic_send_role_swap_message);
#endif
static struct attribute *ccic_attributes[] = {
	&dev_attr_cur_version.attr,
	&dev_attr_src_version.attr,
	&dev_attr_lpm_mode.attr,
	&dev_attr_state.attr,
#if defined(CONFIG_SEC_FACTORY)
	&dev_attr_rid.attr,
	&dev_attr_ccic_control_option.attr,
#endif
	&dev_attr_fw_update.attr,
	&dev_attr_fw_update_status.attr,
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
	&dev_attr_uvdm.attr,
	&dev_attr_attention.attr,
	&dev_attr_role_swap.attr,
#endif
	NULL
};

const struct attribute_group ccic_sysfs_group = {
	.attrs = ccic_attributes,
};

