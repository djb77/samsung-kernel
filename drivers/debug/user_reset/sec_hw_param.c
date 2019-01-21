/*
 * drivers/misc/samsung/sec_hw_param.c
 *
 * COPYRIGHT(C) 2016-2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/of.h>

#include <soc/qcom/socinfo.h>

#include <linux/sec_smem.h>
#include <linux/sec_class.h>
#include <linux/sec_debug.h>
#include <linux/user_reset/sec_debug_user_reset.h>
#include <linux/user_reset/sec_debug_partition.h>
#include <linux/user_reset/sec_hw_param.h>

#define PARAM0_IVALID			1
#define PARAM0_LESS_THAN_0		2

static unsigned int system_rev __read_mostly;

static int __init sec_hw_rev_setup(char *p)
{
	int ret;

	ret = kstrtouint(p, 0, &system_rev);
	if (unlikely(ret < 0)) {
		pr_warn("androidboot.revision is malformed (%s)\n", p);
		return -EINVAL;
	}

	pr_info("androidboot.revision %x\n", system_rev);

	return 0;
}
early_param("androidboot.revision", sec_hw_rev_setup);

unsigned int sec_hw_rev(void)
{
	return system_rev;
}
EXPORT_SYMBOL(sec_hw_rev);

static void check_format(char *buf, ssize_t *size, int max_len_str)
{
	int i = 0, cnt = 0, pos = 0;

	if (!buf || *size <= 0)
		return;

	if (*size >= max_len_str)
		*size = max_len_str - 1;

	while (i < *size && buf[i]) {
		if (buf[i] == '"') {
			cnt++;
			pos = i;
		}

		if (buf[i] == '\n')
			buf[i] = '/';
		i++;
	}

	if (cnt % 2) {
		if (pos == *size - 1) {
			buf[*size - 1] = '\0';
		} else {
			buf[*size - 1] = '"';
			buf[*size] = '\0';
		}
	}
}

static bool __is_ready_debug_reset_header(void)
{
	struct debug_reset_header *header = get_debug_reset_header();

	if (!header)
		return false;

	kfree(header);
	return true;
}

static bool __is_valid_reset_reason(unsigned int reset_reason)
{
	if (reset_reason < USER_UPLOAD_CAUSE_MIN ||
	    reset_reason > USER_UPLOAD_CAUSE_MAX)
		return false;

	if (reset_reason == USER_UPLOAD_CAUSE_MANUAL_RESET ||
	    reset_reason == USER_UPLOAD_CAUSE_REBOOT ||
	    reset_reason == USER_UPLOAD_CAUSE_BOOTLOADER_REBOOT ||
	    reset_reason == USER_UPLOAD_CAUSE_POWER_ON)
		return false;

	return true;
}

static ap_health_t *phealth;

static bool batt_info_cleaned;
static void clean_batt_info(void)
{
	memset(&phealth->battery, 0x0, sizeof(battery_health_t));
	batt_info_cleaned = true;
}

void battery_last_dcvs(int cap, int volt, int temp, int curr)
{
	uint32_t tail = 0;

	if (phealth == NULL || !batt_info_cleaned)
		return;

	if ((phealth->battery.tail & 0xf) >= MAX_BATT_DCVS) {
		phealth->battery.tail = 0x10;
	}

	tail = phealth->battery.tail & 0xf;

	phealth->battery.batt[tail].ktime = local_clock();
	phealth->battery.batt[tail].cap = cap;
	phealth->battery.batt[tail].volt = volt;
	phealth->battery.batt[tail].temp = temp;
	phealth->battery.batt[tail].curr = curr;

	phealth->battery.tail++;
}
EXPORT_SYMBOL(battery_last_dcvs);

static ssize_t show_last_dcvs(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	unsigned int reset_reason;
	char *prefix[MAX_CLUSTER_NUM] = {"L3", "SC", "GC"};
	char *prefix_vreg[MAX_VREG_CNT] = {"MX", "CX", "EBI"};
	size_t i;

	if (!phealth)
		phealth = ap_health_data_read();

	if (!phealth) {
		pr_err("fail to get ap health info\n");
		return info_size;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		return info_size;

	sysfs_scnprintf(buf, info_size, "\"RR\":\"%s\",",
			sec_debug_get_reset_reason_str(reset_reason));
	sysfs_scnprintf(buf, info_size, "\"RWC\":\"%d\",",
			sec_debug_get_reset_write_cnt());

	for (i = 0; i < MAX_CLUSTER_NUM; i++) {
		sysfs_scnprintf(buf, info_size, "\"%sKHz\":\"%u\",", prefix[i],
				phealth->last_dcvs.apps[i].cpu_KHz);
	}

	sysfs_scnprintf(buf, info_size, "\"DDRKHz\":\"%u\",",
			phealth->last_dcvs.rpm.ddr_KHz);

	for (i = 0; i < MAX_VREG_CNT; i++)
		sysfs_scnprintf(buf, info_size, "\"%s\":\"%u\",",
				prefix_vreg[i],
				phealth->last_dcvs.rpm.mV[i]);

	sysfs_scnprintf(buf, info_size,	"\"BCAP\":\"%d\",",
			phealth->last_dcvs.batt.cap);
	sysfs_scnprintf(buf, info_size, "\"BVOL\":\"%d\",",
			phealth->last_dcvs.batt.volt);
	sysfs_scnprintf(buf, info_size, "\"BTEM\":\"%d\",",
			phealth->last_dcvs.batt.temp);
	sysfs_scnprintf(buf, info_size, "\"BCUR\":\"%d\",",
			phealth->last_dcvs.batt.curr);

	// remove , character
	info_size--;

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(last_dcvs, 0440, show_last_dcvs, NULL);

static ssize_t store_ap_health(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char clear;
	int err;

	err = sscanf(buf, "%1c", &clear);
	if ((err <= 0) || (clear != 'c' && clear != 'C')) {
		pr_err("command error\n");
		return count;
	}

	if (!phealth)
		phealth = ap_health_data_read();

	if (!phealth) {
		pr_err("fail to get ap health info\n");
		return -EINVAL;
	}

	pr_info("clear ap_health_data by HQM %zu\n", sizeof(ap_health_t));
	/*++ add here need init data by HQM ++*/
	memset(&(phealth->daily_rr), 0, sizeof(reset_reason_t));
	memset(&(phealth->daily_cache), 0, sizeof(cache_health_t));
	memset(&(phealth->daily_pcie), 0, sizeof(pcie_health_t) * MAX_PCIE_NUM);

	ap_health_data_write(phealth);

	return count;
}

static ssize_t show_ap_health(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int cpu;
	size_t i;

	if (!phealth)
		phealth = ap_health_data_read();

	if (!phealth) {
		pr_err("fail to get ap health info\n");
		return info_size;
	}

	sysfs_scnprintf(buf, info_size, "\"L1c\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		sysfs_scnprintf(buf, info_size, "%d",
				phealth->cache.edac[cpu][0].ce_cnt);

		if (cpu == (num_present_cpus() - 1))
			sysfs_scnprintf(buf, info_size, "\",");
		else
			sysfs_scnprintf(buf, info_size, ",");
	}

	sysfs_scnprintf(buf, info_size, "\"L1u\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		sysfs_scnprintf(buf, info_size, "%d",
				phealth->cache.edac[cpu][0].ue_cnt);

		if (cpu == (num_present_cpus() - 1))
			sysfs_scnprintf(buf, info_size, "\",");
		else
			sysfs_scnprintf(buf, info_size, ",");
	}

	sysfs_scnprintf(buf, info_size, "\"L2c\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		sysfs_scnprintf(buf, info_size, "%d",
				phealth->cache.edac[cpu][1].ce_cnt);

		if (cpu == (num_present_cpus() - 1))
			sysfs_scnprintf(buf, info_size, "\",");
		else
			sysfs_scnprintf(buf, info_size, ",");
	}

	sysfs_scnprintf(buf, info_size, "\"L2u\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		sysfs_scnprintf(buf, info_size, "%d",
				phealth->cache.edac[cpu][1].ue_cnt);

		if (cpu == (num_present_cpus() - 1))
			sysfs_scnprintf(buf, info_size, "\",");
		else
			sysfs_scnprintf(buf, info_size, ",");
	}

	sysfs_scnprintf(buf, info_size, "\"L3c\":\"%d\",",
			phealth->cache.edac_l3.ce_cnt);
	sysfs_scnprintf(buf, info_size,	"\"L3u\":\"%d\",",
			phealth->cache.edac_l3.ue_cnt);
	sysfs_scnprintf(buf, info_size,	"\"EDB\":\"%d\",",
			phealth->cache.edac_bus_cnt);

	sysfs_scnprintf(buf, info_size, "\"dL1c\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		sysfs_scnprintf(buf, info_size, "%d",
				phealth->daily_cache.edac[cpu][0].ce_cnt);

		if (cpu == (num_present_cpus() - 1))
			sysfs_scnprintf(buf, info_size, "\",");
		else
			sysfs_scnprintf(buf, info_size, ",");
	}

	sysfs_scnprintf(buf, info_size, "\"dL1u\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		sysfs_scnprintf(buf, info_size, "%d",
				phealth->daily_cache.edac[cpu][0].ue_cnt);

		if (cpu == (num_present_cpus() - 1))
			sysfs_scnprintf(buf, info_size, "\",");
		else
			sysfs_scnprintf(buf, info_size, ",");
	}

	sysfs_scnprintf(buf, info_size, "\"dL2c\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		sysfs_scnprintf(buf, info_size, "%d",
				phealth->daily_cache.edac[cpu][1].ce_cnt);

		if (cpu == (num_present_cpus() - 1))
			sysfs_scnprintf(buf, info_size, "\",");
		else
			sysfs_scnprintf(buf, info_size, ",");
	}

	sysfs_scnprintf(buf, info_size, "\"dL2u\":\"");
	for (cpu = 0; cpu < num_present_cpus(); cpu++) {
		sysfs_scnprintf(buf, info_size, "%d",
				phealth->daily_cache.edac[cpu][1].ue_cnt);

		if (cpu == (num_present_cpus() - 1))
			sysfs_scnprintf(buf, info_size, "\",");
		else
			sysfs_scnprintf(buf, info_size, ",");
	}

	sysfs_scnprintf(buf, info_size, "\"dL3c\":\"%d\",",
			phealth->daily_cache.edac_l3.ce_cnt);
	sysfs_scnprintf(buf, info_size,	"\"dL3u\":\"%d\",",
			phealth->daily_cache.edac_l3.ue_cnt);
	sysfs_scnprintf(buf, info_size,	"\"dEDB\":\"%d\",",
			phealth->daily_cache.edac_bus_cnt);

	for (i = 0; i < MAX_PCIE_NUM; i++) {
		sysfs_scnprintf(buf, info_size, "\"P%zuPF\":\"%d\",", i,
				phealth->pcie[i].phy_init_fail_cnt);
		sysfs_scnprintf(buf, info_size, "\"P%zuLD\":\"%d\",", i,
				phealth->pcie[i].link_down_cnt);
		sysfs_scnprintf(buf, info_size, "\"P%zuLF\":\"%d\",", i,
				phealth->pcie[i].link_up_fail_cnt);
		sysfs_scnprintf(buf, info_size, "\"P%zuLT\":\"%x\",", i,
				phealth->pcie[i].link_up_fail_ltssm);

		sysfs_scnprintf(buf, info_size, "\"dP%zuPF\":\"%d\",", i,
				phealth->daily_pcie[i].phy_init_fail_cnt);
		sysfs_scnprintf(buf, info_size, "\"dP%zuLD\":\"%d\",", i,
				phealth->daily_pcie[i].link_down_cnt);
		sysfs_scnprintf(buf, info_size, "\"dP%zuLF\":\"%d\",", i,
				phealth->daily_pcie[i].link_up_fail_cnt);
		sysfs_scnprintf(buf, info_size, "\"dP%zuLT\":\"%x\",", i,
				phealth->daily_pcie[i].link_up_fail_ltssm);
	}

	sysfs_scnprintf(buf, info_size, "\"dNP\":\"%d\",",
			phealth->daily_rr.np);
	sysfs_scnprintf(buf, info_size, "\"dRP\":\"%d\",",
			phealth->daily_rr.rp);
	sysfs_scnprintf(buf, info_size, "\"dMP\":\"%d\",",
			phealth->daily_rr.mp);
	sysfs_scnprintf(buf, info_size, "\"dKP\":\"%d\",",
			phealth->daily_rr.kp);
	sysfs_scnprintf(buf, info_size, "\"dDP\":\"%d\",",
			phealth->daily_rr.dp);
	sysfs_scnprintf(buf, info_size, "\"dWP\":\"%d\",",
			phealth->daily_rr.wp);
	sysfs_scnprintf(buf, info_size, "\"dTP\":\"%d\",",
			phealth->daily_rr.tp);
	sysfs_scnprintf(buf, info_size, "\"dSP\":\"%d\",",
			phealth->daily_rr.sp);
	sysfs_scnprintf(buf, info_size, "\"dPP\":\"%d\"",
			phealth->daily_rr.pp);

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(ap_health, 0660, show_ap_health, store_ap_health);

#if 0
#define for_each_ch_cs_dq(ch, max_ch, cs, max_cs, dq, max_dq)		\
for (ch = 0; ch < max_ch; ch++)						\
	for (cs = 0; cs < max_cs; cs++)					\
		for (dq = 0; dq < max_dq; dq++)

static ssize_t show_ddr_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

	sysfs_scnprintf(buf, info_size, "\"DDRV\":\"%s\",",
			get_ddr_vendor_name());
	sysfs_scnprintf(buf, info_size, "\"DSF\":\"%d.%d\",",
			(get_ddr_DSF_version() >> 16) & 0xFFFF,
			get_ddr_DSF_version() & 0xFFFF);

	for_each_ch_cs_dq (ch, 2, cs, 2, dq, 4) {
		sysfs_scnprintf(buf, info_size, "\"RW_%d_%d_%d\":\"%d\",",
				ch, cs, dq, get_ddr_rcw_tDQSCK(ch, cs, dq));
		sysfs_scnprintf(buf, info_size, "\"WC_%d_%d_%d\":\"%d\",",
				ch, cs, dq, get_ddr_wr_coarseCDC(ch, cs, dq));
		sysfs_scnprintf(buf, info_size, "\"WF_%d_%d_%d\":\"%d\",",
				ch, cs, dq, get_ddr_wr_fineCDC(ch, cs, dq));
	}

	/* remove the last ',' character */
	info_size--;

	check_format(buf, &info_size, MAX_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(ddr_info, 0440, show_ddr_info, NULL);

static ssize_t show_eye_rd_info(struct device *dev,
			         struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"DDRV\":\"%s\",", get_ddr_vendor_name());
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"DSF\":\"%d.%d\",",
				(get_ddr_DSF_version() >> 16) & 0xFFFF,
				get_ddr_DSF_version() & 0xFFFF);

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"CNT\":\"%d\",",ddr_get_small_eye_detected());

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"rd_width\":\"R1\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (cs = 0; cs < NUM_CS; cs++) {
			for (dq = 0; dq < NUM_DQ_PCH; dq++) {
				info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size,
				"\"RW%d_%d_%d\":\"%d\",", ch, cs, dq,
				ddr_get_rd_pr_width(ch, cs, dq));
			}
		}
	}

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"rd_height\":\"R2\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (cs = 0; cs < NUM_CS; cs++) {
			info_size += snprintf((char*)(buf+info_size),
			MAX_LEN_STR - info_size,
			"\"RH%d_%d_x\":\"%d\",", ch, cs,
			ddr_get_rd_min_eye_height(ch, cs));
		}
	}

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"rd_vref\":\"Rx\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (cs = 0; cs < NUM_CS; cs++) {
			for (dq = 0; dq < NUM_DQ_PCH; dq++) {
				info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size,
				"\"RV%d_%d_%d\":\"%d\",", ch, cs, dq,
				ddr_get_rd_best_vref(ch, cs, dq));
			}
		}
	}

	// remove , character
	info_size--;

	check_format(buf, &info_size, MAX_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(eye_rd_info, 0440, show_eye_rd_info, NULL);

static ssize_t show_eye_dcc_info(struct device *dev,
			         struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"dqs_dcc\":\"D3\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (dq = 0; dq < NUM_DQ_PCH; dq++) {
			info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size,
				"\"DS%d_x_%d\":\"%d\",", ch, dq,
				ddr_get_dqs_dcc_adj(ch, dq));
		}
	}

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"dq_dcc\":\"D5\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (cs = 0; cs < NUM_CS; cs++) {
			for (dq = 0; dq < NUM_DQ_PCH; dq++) {
				info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size,
				"\"DQ%d_%d_%d\":\"%d\",", ch, cs, dq,
				ddr_get_dq_dcc_abs(ch, cs, dq));
			}
		}
	}

	// remove , character
	info_size--;

	check_format(buf, &info_size, MAX_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(eye_dcc_info, 0440, show_eye_dcc_info, NULL);

static ssize_t show_eye_wr1_info(struct device *dev,
			         struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"wr_width\":\"W1\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (cs = 0; cs < NUM_CS; cs++) {
			for (dq = 0; dq < NUM_DQ_PCH; dq++) {
				info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size,
				"\"WW%d_%d_%d\":\"%d\",", ch, cs, dq,
				ddr_get_wr_pr_width(ch, cs, dq));
			}
		}
	}

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"wr_height\":\"W2\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (cs = 0; cs < NUM_CS; cs++) {
			info_size += snprintf((char*)(buf+info_size),
			MAX_LEN_STR - info_size,
			"\"WH%d_%d_x\":\"%d\",", ch, cs,
			ddr_get_wr_min_eye_height(ch, cs));
		}
	}

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"wr_vref\":\"Wx\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (cs = 0; cs < NUM_CS; cs++) {
			info_size += snprintf((char*)(buf+info_size),
			MAX_LEN_STR - info_size,
			"\"WV%d_%d_x\":\"%d\",", ch, cs,
			ddr_get_wr_best_vref(ch, cs));
		}
	}

	// remove , character
	info_size--;

	check_format(buf, &info_size, MAX_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(eye_wr1_info, 0440, show_eye_wr1_info, NULL);

static ssize_t show_eye_wr2_info(struct device *dev,
			         struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	uint32_t ch, cs, dq;

 	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"wr_topw\":\"W4\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (cs = 0; cs < NUM_CS; cs++) {
			for (dq = 0; dq < NUM_DQ_PCH; dq++) {
				info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size,
				"\"WT%d_%d_%d\":\"%d\",", ch, cs, dq,
				ddr_get_wr_vmax_to_vmid(ch, cs, dq));
			}
		}
	}

	info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size, "\"wr_botw\":\"W4\",");
	for (ch = 0; ch < NUM_CH; ch++) {
		for (cs = 0; cs < NUM_CS; cs++) {
			for (dq = 0; dq < NUM_DQ_PCH; dq++) {
				info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size,
				"\"WB%d_%d_%d\":\"%d\",", ch, cs, dq,
				ddr_get_wr_vmid_to_vmin(ch, cs, dq));
			}
		}
	}

	// remove , character
	info_size--;

	check_format(buf, &info_size, MAX_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(eye_wr2_info, 0440, show_eye_wr2_info, NULL);
#endif

static int get_param0(int id)
{
	struct device_node *np = of_find_node_by_path("/soc/sec_hw_param");
	u32 val;
	static int num_param;
	static u32 hw_param0[NUM_PARAM0];
	struct property *prop;
	const __be32 *p;

	if (likely(num_param != 0))
		goto out;

	if (!np) {
		pr_err("No sec_hw_param found\n");
		return -PARAM0_IVALID;
	}

	of_property_for_each_u32(np, "param0", prop, p, val) {
		pr_debug("%d : %d\n", num_param, val);
		hw_param0[num_param++] = val;

		if (num_param >= NUM_PARAM0)
			break;
	}

out:
	if (id < 0)
		return -PARAM0_LESS_THAN_0;

	return id < num_param ? hw_param0[id] : -PARAM0_IVALID;
}

static ssize_t show_ap_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int corner;
	char *prefix[MAX_CLUSTER_NUM] = {"L3", "SC", "GC"};
	size_t i;

	sysfs_scnprintf(buf, info_size, "\"HW_REV\":\"%d\"", system_rev);
	sysfs_scnprintf(buf, info_size, ",\"SoC_ID\":\"%u\"", socinfo_get_id());
	sysfs_scnprintf(buf, info_size, ",\"SoC_REV\":\"%u.%u.%u\"",
			SOCINFO_VERSION_MAJOR(socinfo_get_version()),
			0,
			SOCINFO_VERSION_MINOR(socinfo_get_version()));

	for (i = PWR_CLUSTER; i < MAX_CLUSTER_NUM; i++) {
		sysfs_scnprintf(buf, info_size, ",\"%s_PRM\":\"%d\"",
				prefix[i], get_param0(i - PWR_CLUSTER));

		sysfs_scnprintf(buf, info_size, ",\"%s_SB\":\"%d\"",
				prefix[i], get_param0(4));

		sysfs_scnprintf(buf, info_size, ",\"%s_CR\":\"%d\"",
				prefix[i], get_param0(5));

		for (corner = 0; corner < 4; corner++) {
			sysfs_scnprintf(buf, info_size, ",\"%s_OPV_%d\":\"%d\"",
					prefix[i], corner,
					get_param0(6 + ((i - PWR_CLUSTER)*4) + corner) * 1000);
		}

		if (i == MAX_CLUSTER_NUM-1)
			sysfs_scnprintf(buf, info_size, ",\"%s_OPV_%d\":\"%d\"",
					prefix[i], corner,
					get_param0(14) * 1000);
	}

	sysfs_scnprintf(buf, info_size, ",\"DOUR\":\"%d\",\"DOUB\":\"%d\"",
			get_param0(15), get_param0(16));

	check_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}
static DEVICE_ATTR(ap_info, 0440, show_ap_info, NULL);

static ssize_t show_extra_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t offset = 0;
	unsigned long long rem_nsec;
	unsigned long long ts_nsec;
	unsigned int reset_reason;
	rst_exinfo_t *p_rst_exinfo = NULL;
	_kern_ex_info_t *p_kinfo = NULL;
	int cpu = -1;

	if (!__is_ready_debug_reset_header()) {
		pr_info("updated nothing.\n");
		goto out;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		goto out;

	p_rst_exinfo = kmalloc(sizeof(rst_exinfo_t), GFP_KERNEL);
	if (!p_rst_exinfo)
		goto out;

	if (!read_debug_partition(debug_index_reset_ex_info, p_rst_exinfo)) {
		pr_err("fail - get param!!\n");
		goto out;
	}
	p_kinfo = &p_rst_exinfo->kern_ex_info.info;
	cpu = p_kinfo->cpu;

	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
		"\"RR\":\"%s\",", sec_debug_get_reset_reason_str(reset_reason));
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
		"\"RWC\":\"%d\",", sec_debug_get_reset_write_cnt());

	ts_nsec = p_kinfo->ktime;
	rem_nsec = do_div(ts_nsec, 1000000000ULL);

	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"KTIME\":\"%llu.%06llu\",", ts_nsec, rem_nsec / 1000ULL);
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"CPU\":\"%d\",", p_kinfo->cpu);
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"TASK\":\"%s\",", p_kinfo->task_name);

	if (p_kinfo->smmu.fsr) {
		offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
				"\"SDN\":\"%s\",", p_kinfo->smmu.dev_name);
		offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
				"\"FSR\":\"%x\",", p_kinfo->smmu.fsr);

		if (p_kinfo->smmu.fsynr) {
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"FSRY\":\"%x\",", p_kinfo->smmu.fsynr);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"IOVA\":\"%08lx\",", p_kinfo->smmu.iova);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"FAR\":\"%016lx\",", p_kinfo->smmu.far);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"SMN\":\"%s\",", p_kinfo->smmu.mas_name);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"CB\":\"%d\",", p_kinfo->smmu.cbndx);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"SOFT\":\"%016llx\",", p_kinfo->smmu.phys_soft);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"ATOS\":\"%016llx\",", p_kinfo->smmu.phys_atos);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"SID\":\"%x\",", p_kinfo->smmu.sid);
		}
	}

	if (p_kinfo->badmode.esr) {
		offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
				"\"BDR\":\"%08x\",", p_kinfo->badmode.reason);
		offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
				"\"BDRS\":\"%s\",", p_kinfo->badmode.handler_str);
		offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
				"\"BDE\":\"%08x\",", p_kinfo->badmode.esr);
		offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
				"\"BDES\":\"%s\",", p_kinfo->badmode.esr_str);
	}

	if ((cpu > -1) && (cpu < num_present_cpus())) {
		if (p_kinfo->fault[cpu].esr) {
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"ESR\":\"%08x\",", p_kinfo->fault[cpu].esr);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"FNM\":\"%s\",", p_kinfo->fault[cpu].str);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"FV1\":\"%016llx\",", p_kinfo->fault[cpu].var1);
			offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
					"\"FV2\":\"%016llx\",",	p_kinfo->fault[cpu].var2);
		}

		offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
				"\"FAULT\":\"pgd=%016llx VA=%016llx ",
				p_kinfo->fault[cpu].pte[0],
				p_kinfo->fault[cpu].pte[1]);
		offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
				"*pgd=%016llx *pud=%016llx ",
				p_kinfo->fault[cpu].pte[2],
				p_kinfo->fault[cpu].pte[3]);
		offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
				"*pmd=%016llx *pte=%016llx\",",
				p_kinfo->fault[cpu].pte[4],
				p_kinfo->fault[cpu].pte[5]);
	}

	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"BUG\":\"%s\",", p_kinfo->bug_buf);
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"PANIC\":\"%s\",", p_kinfo->panic_buf);
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"PC\":\"%s\",", p_kinfo->pc);
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"LR\":\"%s\",", p_kinfo->lr);
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"GLE\":\"%s\",", p_kinfo->dbg0);
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"UFS\":\"%s\",", p_kinfo->ufs_err);
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"DISP\":\"%s\",", p_kinfo->display_err);
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"ROT\":\"W%dC%d\",", get_param0(0), get_param0(1));
	offset += snprintf((char*)(buf + offset), EXTRA_LEN_STR - offset,
			"\"STACK\":\"%s\"", p_kinfo->backtrace);

out:
	if (p_rst_exinfo)
		kfree(p_rst_exinfo);

	check_format(buf, &offset, EXTRA_LEN_STR);

	return offset;
}
static DEVICE_ATTR(extra_info, 0440, show_extra_info, NULL);

static ssize_t show_extrb_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t offset = 0;
	int idx, cnt, max_cnt;
	unsigned long rem_nsec;
	u64 ts_nsec;
	unsigned int reset_reason;

	rst_exinfo_t *p_rst_exinfo = NULL;
	__rpm_log_t *pRPMlog = NULL;

	if (!__is_ready_debug_reset_header()) {
		pr_info("updated nothing.\n");
		goto out;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		goto out;

	p_rst_exinfo = kmalloc(sizeof(rst_exinfo_t), GFP_KERNEL);
	if (!p_rst_exinfo) {
		pr_err("%s : fail - kmalloc\n", __func__);
		goto out;
	}

	if (!read_debug_partition(debug_index_reset_ex_info, p_rst_exinfo)) {
		pr_err("%s : fail - get param!!\n", __func__);
		goto out;
	}

	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
			"\"RR\":\"%s\",", sec_debug_get_reset_reason_str(reset_reason));

	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
			"\"RWC\":\"%d\",", sec_debug_get_reset_write_cnt());

	if (p_rst_exinfo->rpm_ex_info.info.magic == RPM_EX_INFO_MAGIC
			&& p_rst_exinfo->rpm_ex_info.info.nlog > 0) {
		offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
				"\"RPM\":\"");

		if (p_rst_exinfo->rpm_ex_info.info.nlog > 5) {
			idx = p_rst_exinfo->rpm_ex_info.info.nlog % 5;
			max_cnt = 5;
		} else {
			idx = 0;
			max_cnt = p_rst_exinfo->rpm_ex_info.info.nlog;
		}

		for (cnt  = 0; cnt < max_cnt; cnt++, idx++) {
			pRPMlog = &p_rst_exinfo->rpm_ex_info.info.log[idx % 5];

			ts_nsec = pRPMlog->nsec;
			rem_nsec = do_div(ts_nsec, 1000000000);

			offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
					"%lu.%06lu ", (unsigned long)ts_nsec, rem_nsec / 1000);

			offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
					"%s ", pRPMlog->msg);

			offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
					"%x %x %x %x", pRPMlog->arg[0], pRPMlog->arg[1], pRPMlog->arg[2], pRPMlog->arg[3]);

			if (cnt == max_cnt - 1) {
				offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, "\",");
			} else {
				offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, "/");
			}
		}
	}
#if 0
	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
			"\"TZ_RR\":\"%s\"",
			p_rst_exinfo->tz_ex_info.msg);
	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
			",\"PIMEM\":\"0x%08x,0x%08x,0x%08x,0x%08x\"",
			p_rst_exinfo->pimem_info.esr, p_rst_exinfo->pimem_info.ear0,
			p_rst_exinfo->pimem_info.esr_sdi, p_rst_exinfo->pimem_info.ear0_sdi);
	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
			",\"HYP\":\"%s\"",
			p_rst_exinfo->hyp_ex_info.msg);

	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
			",\"LPM\":\"");
	max_cnt = sizeof(p_rst_exinfo->kern_ex_info.info.lpm_state);
	max_cnt = max_cnt/sizeof(p_rst_exinfo->kern_ex_info.info.lpm_state[0]);
	for (idx = 0; idx < max_cnt; idx++) {
		offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
				"%x", p_rst_exinfo->kern_ex_info.info.lpm_state[idx]);
		if (idx != max_cnt - 1)
			offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, ",");
	}
	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, "\"");
#endif

	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, 
			"\"PKO\":\"%x\"", p_rst_exinfo->kern_ex_info.info.pko);

	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, ",\"LR\":\"");
	max_cnt = sizeof(p_rst_exinfo->kern_ex_info.info.lr_val);
	max_cnt = max_cnt/sizeof(p_rst_exinfo->kern_ex_info.info.lr_val[0]);
	for (idx = 0; idx < max_cnt; idx++) {
		offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
				"%llx", p_rst_exinfo->kern_ex_info.info.lr_val[idx]);
		if (idx != max_cnt - 1)
			offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, ",");
	}
	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, "\"");

	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, ",\"PC\":\"");
	max_cnt = sizeof(p_rst_exinfo->kern_ex_info.info.pc_val);
	max_cnt = max_cnt/sizeof(p_rst_exinfo->kern_ex_info.info.pc_val[0]);
	for (idx = 0; idx < max_cnt; idx++) {
		offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
				"%llx", p_rst_exinfo->kern_ex_info.info.pc_val[idx]);
		if (idx != max_cnt - 1)
			offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, ",");
	}
	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset, "\"");

out:
	if (p_rst_exinfo)
		kfree(p_rst_exinfo);

	check_format(buf, &offset, SPECIAL_LEN_STR);

	return offset;
}

static DEVICE_ATTR(extrb_info, 0440, show_extrb_info, NULL);

static ssize_t show_extrc_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t offset = 0;
	unsigned int reset_reason;
	char *extrc_buf = NULL;

	if (!__is_ready_debug_reset_header()) {
		pr_info("updated nothing.\n");
		goto out;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (!__is_valid_reset_reason(reset_reason))
		goto out;

	extrc_buf = kmalloc(SEC_DEBUG_RESET_EXTRC_SIZE, GFP_KERNEL);
	if (!extrc_buf)
		goto out;

	if (!read_debug_partition(debug_index_reset_extrc_info, extrc_buf)) {
		pr_err("%s : fail - get param!!\n", __func__);
		goto out;
	}

	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
			"\"RR\":\"%s\",", sec_debug_get_reset_reason_str(reset_reason));

	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
			"\"RWC\":\"%d\",", sec_debug_get_reset_write_cnt());

	offset += snprintf((char*)(buf + offset), SPECIAL_LEN_STR- offset,
			"\"LKL\":\"%s\"", &extrc_buf[offset]);
out:
	check_format(buf, &offset, SPECIAL_LEN_STR);
	if (extrc_buf)
		kfree(extrc_buf);

	return offset;
}

static DEVICE_ATTR(extrc_info, 0440, show_extrc_info, NULL);

static int sec_hw_param_dbg_part_notifier_callback(
	struct notifier_block *nfb, unsigned long action, void *data)
{
	switch (action) {
	case DBG_PART_DRV_INIT_DONE:
		phealth = ap_health_data_read();
		clean_batt_info();
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct attribute *sec_hw_param_attributes[] = {
	&dev_attr_ap_info.attr,
#if 0
	&dev_attr_ddr_info.attr,
	&dev_attr_eye_rd_info.attr,
	&dev_attr_eye_wr1_info.attr,
	&dev_attr_eye_wr2_info.attr,
	&dev_attr_eye_dcc_info.attr,
#endif
	&dev_attr_ap_health.attr,
	&dev_attr_last_dcvs.attr,
	&dev_attr_extra_info.attr,
	&dev_attr_extrb_info.attr,
	&dev_attr_extrc_info.attr,
	NULL,
};

static const struct attribute_group sec_hw_param_attribute_group = {
	.attrs = sec_hw_param_attributes,
};

static struct notifier_block sec_hw_param_dbg_part_notifier = {
	.notifier_call = sec_hw_param_dbg_part_notifier_callback,
};

static int __init sec_hw_param_init(void)
{
	struct device *sec_hw_param_dev;
	int err;

	sec_hw_param_dev = sec_device_create(0, NULL, "sec_hw_param");
	if (IS_ERR(sec_hw_param_dev)) {
		pr_err("Failed to create devce\n");
		return -ENODEV;
	}

	err = sysfs_create_group(&sec_hw_param_dev->kobj,
			&sec_hw_param_attribute_group);
	if (unlikely(err)) {
		pr_err("Failed to create device files!\n");
		goto err_dev_create_file;
	}

	dbg_partition_notifier_register(&sec_hw_param_dbg_part_notifier);

	return 0;

err_dev_create_file:
	sec_device_destroy(sec_hw_param_dev->devt);
	return err;
}
device_initcall(sec_hw_param_init);
