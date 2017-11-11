/*
 *sec_hw_param.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sec_debug.h>
#include <linux/sec_ext.h>
#include <linux/sec_sysfs.h>
#include <linux/uaccess.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <linux/io.h>
#include <linux/thermal.h>

#define MAX_DDR_VENDOR 16
#define LPDDR_BASE 0x16509b00
#define DATA_SIZE 1024 
#define LOT_STRING_LEN 5

#define HW_PARAM_CHECK_SIZE(size)	\
	if (size >= 1024)		\
	return 1024;			\
	
enum ids_info {
	tg,
	lg,
	bg,
	g3dg,
	mifg,
	bids,
	gids,
};

extern int asv_ids_information(enum ids_info id);
extern struct thermal_data_devices thermal_data_info[THERMAL_ZONE_MAX];

/*
 * LPDDR4 (JESD209-4) MR5 Manufacturer ID
 * 0000 0000B : Reserved
 * 0000 0001B : Samsung
 * 0000 0101B : Nanya
 * 0000 0110B : SK hynix
 * 0000 1000B : Winbond
 * 0000 1001B : ESMT
 * 1111 1111B : Micron
 * All others : Reserved
 */
static char *lpddr4_manufacture_name[MAX_DDR_VENDOR] = {
	"NA",
	"SEC", /* Samsung */
	"NA",
	"NA",
	"NA",
	"NAN", /* Nanya */
	"HYN", /* SK hynix */
	"NA",
	"WIN", /* Winbond */
	"ESM", /* ESMT */
	"NA",
	"NA",
	"NA",
	"NA",
	"NA",
	"MIC", /* Micron */
};

static unsigned int sec_hw_rev;
static unsigned int chipid_fail_cnt;
static unsigned int lpi_timeout_cnt;
static unsigned int cache_err_cnt;
static unsigned int codediff_cnt;
static unsigned long pcb_offset;
static unsigned long smd_offset;
static unsigned int lpddr4_size;

static int __init sec_hw_param_get_hw_rev(char *arg)
{
	get_option(&arg, &sec_hw_rev);
	return 0;
}

early_param("androidboot.hw_rev", sec_hw_param_get_hw_rev);

static int __init sec_hw_param_check_chip_id(char *arg)
{
	get_option(&arg, &chipid_fail_cnt);
	return 0;
}

early_param("sec_debug.chipidfail_cnt", sec_hw_param_check_chip_id);

static int __init sec_hw_param_check_lpi_timeout(char *arg)
{
	get_option(&arg, &lpi_timeout_cnt);
	return 0;
}

early_param("sec_debug.lpitimeout_cnt", sec_hw_param_check_lpi_timeout);

static int __init sec_hw_param_cache_error(char *arg)
{
	get_option(&arg, &cache_err_cnt);
	return 0;
}

early_param("sec_debug.cache_err_cnt", sec_hw_param_cache_error);

static int __init sec_hw_param_code_diff(char *arg)
{
	get_option(&arg, &codediff_cnt);
	return 0;
}

early_param("sec_debug.codediff_cnt", sec_hw_param_code_diff);

static int __init sec_hw_param_pcb_offset(char *arg)
{
	pcb_offset = simple_strtoul(arg, NULL, 10);	
	return 0;
}

early_param("sec_debug.pcb_offset", sec_hw_param_pcb_offset);

static int __init sec_hw_param_smd_offset(char *arg)
{
	smd_offset = simple_strtoul(arg, NULL, 10);	
	return 0;
}

early_param("sec_debug.smd_offset", sec_hw_param_smd_offset);

static int __init sec_hw_param_lpddr4_size(char *arg)
{
	get_option(&arg, &lpddr4_size);
	return 0;
}

early_param("sec_debug.lpddr4_size", sec_hw_param_lpddr4_size);

static u32 chipid_reverse_value(u32 value, u32 bitcnt)
{
	int tmp, ret = 0;
	int i;

	for (i = 0; i < bitcnt; i++) {
		tmp = (value >> i) & 0x1;
		ret += tmp << ((bitcnt - 1) - i);
	}

	return ret;
}

static void chipid_dec_to_36(u32 in, char *p)
{
	int mod;
	int i;

	for (i = LOT_STRING_LEN - 1; i >= 1; i--) {
		mod = in % 36;
		in /= 36;
		p[i] = (mod < 10) ? (mod + '0') : (mod - 10 + 'A');
	}

	p[0] = 'N';
	p[LOT_STRING_LEN] = '\0';
}

static char *get_dram_manufacturer(void)
{
	void *lpddr_reg;
	u64 val;
	int mr5_vendor_id = 0;

	lpddr_reg = ioremap(LPDDR_BASE, SZ_64);

	if (!lpddr_reg) {
		pr_err("failed to get i/o address lpddr_reg\n");
		return lpddr4_manufacture_name[mr5_vendor_id];
	}

	val = readq((void __iomem *)lpddr_reg);

	mr5_vendor_id = 0xf & (val >> 8);

	return lpddr4_manufacture_name[mr5_vendor_id];
}

static ssize_t sec_hw_param_ap_info_show(struct kobject *kobj,
					 struct kobj_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int reverse_id_0 = 0;
	u32 tmp = 0;
	char lot_id[LOT_STRING_LEN + 1];

	reverse_id_0 = chipid_reverse_value(exynos_soc_info.lot_id, 32);
	tmp = (reverse_id_0 >> 11) & 0x1FFFFF;
	chipid_dec_to_36(tmp, lot_id);

	info_size += snprintf(buf, DATA_SIZE, "\"HW_REV\":\"%d\",", sec_hw_rev);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"LOT_ID\":\"%s\",", lot_id);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"CHIPID_FAIL\":\"%d\",", chipid_fail_cnt);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"LPI_TIMEOUT\":\"%d\",", lpi_timeout_cnt);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"CACHE_ERR\":\"%d\",", cache_err_cnt);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"CODE_DIFF\":\"%d\",", codediff_cnt);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"ASV_BIG\":\"%d\",", asv_ids_information(bg));
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"ASV_LIT\":\"%d\",", asv_ids_information(lg));
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"ASV_MIF\":\"%d\",", asv_ids_information(mifg));
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"IDS_BIG\":\"%d\"", asv_ids_information(bids));

	return info_size;
}

static ssize_t sec_hw_param_ddr_info_show(struct kobject *kobj,
					  struct kobj_attribute *attr,
					  char *buf)
{
	ssize_t info_size = 0;

	info_size +=
	    snprintf((char *)(buf), DATA_SIZE, "\"DDRV\":\"%s\",",
		     get_dram_manufacturer());
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"LPDDR4\":\"%dGB\",", lpddr4_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"C2D\":\"\",");
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"D2D\":\"\"");

	return info_size;
}

static ssize_t sec_hw_param_extra_info_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	if (reset_reason == RR_K || reset_reason == RR_D) {
		strncpy(buf, (char *)SEC_DEBUG_EXTRA_INFO_VA, SZ_1K);
		info_size = strlen(buf);
	}

	return info_size;
}

static ssize_t sec_hw_param_pcb_info_store(struct kobject *kobj,
				struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned char barcode[6] = {0,};
	int ret = -1;

	strncpy(barcode, buf, 5);

	ret = sec_set_param_str(pcb_offset , barcode, 5);
	if (ret < 0)
		pr_err("%s : Set Param fail. offset (%lu), data (%s)", __func__, pcb_offset, barcode);
	
	return count;
}

static ssize_t sec_hw_param_smd_info_store(struct kobject *kobj,
				struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned char smd_date[9] = {0,};
	int ret = -1;

	strncpy(smd_date, buf, 8);

	ret = sec_set_param_str(smd_offset , smd_date, 8);
	if (ret < 0)
		pr_err("%s : Set Param fail. offset (%lu), data (%s)", __func__, smd_offset, smd_date);
	
	return count;
}

static ssize_t sec_hw_param_thermal_info_show(struct kobject *kobj,
					  struct kobj_attribute *attr,
					  char *buf)
{
	ssize_t info_size = 0;
	int idx, zone;
	unsigned long sum;
	int per[THERMAL_ZONE_MAX][3], freq[THERMAL_ZONE_MAX][15];
	for (zone = 0; zone < THERMAL_ZONE_MAX; zone++) {
		sum = thermal_data_info[zone].times[ACTIVE_TIMES] + 
			thermal_data_info[zone].times[PASSIVE_TIMES] + 
			thermal_data_info[zone].times[HOT_TIMES];
		if (sum) {
			per[zone][0] = (thermal_data_info[zone].times[ACTIVE_TIMES] * 1000) / sum;
			per[zone][1] = (thermal_data_info[zone].times[PASSIVE_TIMES] * 1000) / sum;
			per[zone][2] = (thermal_data_info[zone].times[HOT_TIMES] * 1000) / sum;
		}
		else {
			per[zone][0] = per[zone][1] = per[zone][2] = 0;
		}
	}

	sum = 0;	// MNGS
	for (idx = 0; idx <= thermal_data_info[0].max_level; idx++)
		sum += thermal_data_info[0].freq_level[idx];
	if (sum) {
		for (idx = 0; idx <= thermal_data_info[0].max_level; idx++)
			freq[0][idx] = (thermal_data_info[0].freq_level[idx] * 1000) / sum;
	}
	else {
		for (idx = 0; idx <= thermal_data_info[0].max_level; idx++)
			freq[0][idx] = 0;
	}

	sum = 0;	// APO
	for (idx = 0; idx <= thermal_data_info[1].max_level; idx++)
		sum += thermal_data_info[1].freq_level[idx];
	if (sum) {
		for (idx = 0; idx <= thermal_data_info[1].max_level; idx++)
			freq[1][idx] = (thermal_data_info[1].freq_level[idx] * 1000) / sum;
	}
	else {
		for (idx = 0; idx <= thermal_data_info[1].max_level; idx++)
			freq[1][idx] = 0;
	}
		

	sum = 0;	// GPU
	for (idx = 0; idx <= thermal_data_info[2].max_level; idx++)
		sum += thermal_data_info[2].freq_level[idx];
	if (sum) {
		for (idx = 0; idx <= thermal_data_info[2].max_level; idx++)
			freq[2][idx] = (thermal_data_info[2].freq_level[idx] * 1000) / sum;
	}
	else {
		for (idx = 0; idx <= thermal_data_info[2].max_level; idx++)
			freq[2][idx] = 0;
	}

// MNGS
	info_size +=
	    snprintf((char *)(buf), DATA_SIZE, "\"MNGS_MAX\":\"%d\",",
		     thermal_data_info[0].max_temp);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"MNG_A\":\"%d.%1d\",", per[0][0]/10, per[0][0]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"MNG_P\":\"%d.%1d\",", per[0][1]/10, per[0][1]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"MNG_H\":\"%d.%1d\",", per[0][2]/10, per[0][2]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"HOTPLUG\":\"%ld\",", thermal_data_info[0].hotplug_out);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"MNGS_F0\":\"%d.%1d\",", freq[0][idx]/10, freq[0][idx]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"MNGS_F1\":\"%d.%1d\",", (1000 - freq[0][idx])/10, 
						(1000 - freq[0][idx])%10);
	HW_PARAM_CHECK_SIZE(info_size);
// APOLLO
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"APO_MAX\":\"%d\",", thermal_data_info[1].max_temp);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"APO_A\":\"%d.%1d\",", per[1][0]/10, per[1][0]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"APO_H\":\"%d.%1d\",", per[1][2]/10, per[1][2]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"APO_F0\":\"%d.%1d\",", freq[1][0]/10, freq[1][0]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"APO_F1\":\"%d.%1d\",", (1000 - freq[1][0])/10, 
						(1000 - freq[1][0])%10);
	HW_PARAM_CHECK_SIZE(info_size);
// GPU
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"GPU_MAX\":\"%d\",", thermal_data_info[2].max_temp);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"GPU_A\":\"%d.%1d\",", per[2][0]/10, per[2][0]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"GPU_P\":\"%d.%1d\",", per[2][1]/10, per[2][1]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"GPU_H\":\"%d.%1d\",", per[2][2]/10, per[2][2]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"GPU_F0\":\"%d.%1d\",", freq[2][0]/10, freq[2][0]%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"GPU_F1\":\"%d.%1d\",", (1000 - freq[2][0])/10, 
						(1000 - freq[2][0])%10);
	HW_PARAM_CHECK_SIZE(info_size);
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"THERMAL\":\"\"");
	HW_PARAM_CHECK_SIZE(info_size);

	for (zone = 0 ; zone < THERMAL_ZONE_MAX; zone++) {
		thermal_data_info[zone].times[ACTIVE_TIMES] = 0;
		thermal_data_info[zone].times[PASSIVE_TIMES] = 0;
		thermal_data_info[zone].times[HOT_TIMES] = 0;
		thermal_data_info[zone].hotplug_out = 0;
		for (idx = 0; idx < 15; idx++)
			thermal_data_info[zone].freq_level[idx] = 0;
	}

	return info_size;
}

static struct kobj_attribute sec_hw_param_ap_info_attr =
        __ATTR(ap_info, 0440, sec_hw_param_ap_info_show, NULL);

static struct kobj_attribute sec_hw_param_ddr_info_attr =
        __ATTR(ddr_info, 0440, sec_hw_param_ddr_info_show, NULL);

static struct kobj_attribute sec_hw_param_extra_info_attr =
	__ATTR(extra_info, 0440, sec_hw_param_extra_info_show, NULL);

static struct kobj_attribute sec_hw_param_pcb_info_attr =
        __ATTR(pcb_info, 0660, NULL, sec_hw_param_pcb_info_store);

static struct kobj_attribute sec_hw_param_smd_info_attr =
	__ATTR(smd_info, 0660, NULL, sec_hw_param_smd_info_store);

static struct kobj_attribute sec_hw_param_thermal_info_attr =
	__ATTR(thermal_info, 0440, sec_hw_param_thermal_info_show, NULL);

static struct attribute *sec_hw_param_attributes[] = {
	&sec_hw_param_ap_info_attr.attr,
	&sec_hw_param_ddr_info_attr.attr,
	&sec_hw_param_extra_info_attr.attr,
	&sec_hw_param_pcb_info_attr.attr,
	&sec_hw_param_smd_info_attr.attr,
	&sec_hw_param_thermal_info_attr.attr,
	NULL,
};

static struct attribute_group sec_hw_param_attr_group = {
	.attrs = sec_hw_param_attributes,
};

static int __init sec_hw_param_init(void)
{
	int ret = 0;
	struct device *dev;

	dev = sec_device_create(NULL, "sec_hw_param");

	ret = sysfs_create_group(&dev->kobj, &sec_hw_param_attr_group);
	if (ret)
		pr_err("%s : could not create sysfs noden", __func__);

	return 0;
}

device_initcall(sec_hw_param_init);
