/* sec_hw_param.c
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
 
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/sec_class.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <soc/qcom/socinfo.h>
#include <linux/sec_debug.h>
#include <linux/sec_param.h>

extern unsigned int system_rev;
extern char* get_ddr_vendor_name(void);
extern uint32_t get_ddr_DSF_version(void);
extern uint8_t get_ddr_rcw_tDQSCK(uint32_t ch, uint32_t cs, uint32_t dq);
extern uint8_t get_ddr_wr_coarseCDC(uint32_t ch, uint32_t cs, uint32_t dq);
extern uint8_t get_ddr_wr_fineCDC(uint32_t ch, uint32_t cs, uint32_t dq);
#ifdef CONFIG_REGULATOR_CPR3
extern int cpr3_get_fuse_open_loop_voltage(int id, int fuse_corner);
extern int cpr3_get_fuse_corner_count(int id);
extern int cpr3_get_fuse_cpr_rev(int id);
extern int cpr3_get_fuse_speed_bin(int id);
#endif
extern unsigned sec_debug_get_reset_reason(void);
extern char * sec_debug_get_reset_reason_str(unsigned int reason);

#define MAX_LEN_STR 1024
#define NUM_PARAM0 2

static ap_health_t *phealth;

static ssize_t show_last_dcvs(struct device *dev,
			         struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int i;
	unsigned int reset_reason;
	char prefix[MAX_CLUSTER_NUM][3] = {"SC", "GC"};
	char prefix_vreg[MAX_VREG_CNT][3] = {"MX", "CX"};

	if (!phealth)
		phealth = ap_health_data_read();

	if (!phealth) {
		pr_err("%s: fail to get ap health info\n", __func__);
		return info_size;
	}

	reset_reason = sec_debug_get_reset_reason();
	if (reset_reason < USER_UPLOAD_CAUSE_MIN ||
		reset_reason > USER_UPLOAD_CAUSE_MAX) {
		return info_size;
	}

	if (reset_reason == USER_UPLOAD_CAUSE_MANUAL_RESET ||
		reset_reason == USER_UPLOAD_CAUSE_REBOOT ||
		reset_reason == USER_UPLOAD_CAUSE_BOOTLOADER_REBOOT ||
		reset_reason == USER_UPLOAD_CAUSE_POWER_ON) {
		return info_size;
	}

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"RR\":\"%s\",",
				sec_debug_get_reset_reason_str(reset_reason));

	for (i = 0; i < MAX_CLUSTER_NUM; i++) {
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"%sKHz\":\"%u\",", prefix[i],
				phealth->last_dcvs.apps[i].cpu_KHz);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"%s\":\"%u\",", prefix[i],
				phealth->last_dcvs.apps[i].apc_mV);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"%sl2\":\"%u\",", prefix[i],
				phealth->last_dcvs.apps[i].l2_mV);
	}

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"DDRKHz\":\"%u\",", phealth->last_dcvs.rpm.ddr_KHz);

	for (i = 0; i < MAX_VREG_CNT; i++) {
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"%s\":\"%u\",", prefix_vreg[i],
				phealth->last_dcvs.rpm.mV[i]);
	}

	info_size--;
	info_size += sprintf((char*)(buf+info_size), "\n");

	return info_size;
}
static DEVICE_ATTR(last_dcvs, 0440, show_last_dcvs, NULL);

static ssize_t store_ap_health(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char clear = 0;

	sscanf(buf, "%1c", &clear);

	if (clear == 'c' || clear == 'C') {
		if (!phealth)
			phealth = ap_health_data_read();

		if (!phealth) {
			pr_err("%s: fail to get ap health info\n", __func__);
			return -1;
		}
		pr_info("%s: clear ap_health_data by HQM %zu\n", __func__, sizeof(ap_health_t));
		/*++ add here need init data by HQM ++*/
		memset((void *)&(phealth->daily_rr), 0, sizeof(reset_reason_t));
		memset((void *)&(phealth->daily_thermal), 0, sizeof(therm_health_t));
		memset((void *)&(phealth->daily_cache), 0, sizeof(cache_health_t));
		memset((void *)&(phealth->daily_pcie), 0, sizeof(pcie_health_t) * MAX_PCIE_NUM);

		ap_health_data_write(phealth);
	} else {
		pr_info("%s: command error\n", __func__);
	}

	return count;
}

static ssize_t show_ap_health(struct device *dev,
			         struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int i, cpu;
	char prefix[2][3] = {"SC", "GC"};

	if (!phealth)
		phealth = ap_health_data_read();

	if (!phealth) {
		pr_err("%s: fail to get ap health info\n", __func__);
		return info_size;
	}

	for (i = 0; i < MAX_CLUSTER_NUM; i++) {
#if defined(CONFIG_ARCH_MSM8917)
		cpu = 0;
#else
		cpu = i * CPU_NUM_PER_CLUSTER;
#endif
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"T%sT\":\"%d\",", prefix[i],
				phealth->thermal.cpu_throttle_cnt[cpu]);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"dT%sT\":\"%d\",", prefix[i],
				phealth->daily_thermal.cpu_throttle_cnt[cpu]);
	}

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"TC%dH\":\"%d\",",
				cpu, phealth->thermal.cpu_hotplug_cnt[cpu]);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"dTC%dH\":\"%d\",",
				cpu, phealth->daily_thermal.cpu_hotplug_cnt[cpu]);
	}

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"TR\":\"%d\",", phealth->thermal.ktm_reset_cnt +
				phealth->thermal.gcc_t0_reset_cnt +
				phealth->thermal.gcc_t1_reset_cnt);

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"dTR\":\"%d\",", phealth->thermal.ktm_reset_cnt +
				phealth->daily_thermal.gcc_t0_reset_cnt +
				phealth->daily_thermal.gcc_t1_reset_cnt);

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"CEG\":\"%u\",",
				phealth->cache.gld_err_cnt);

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"dCEG\":\"%u\",",
				phealth->daily_cache.gld_err_cnt);

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"CEO\":\"%u\",",
				phealth->cache.obsrv_err_cnt);

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"dCEO\":\"%u\",",
				phealth->daily_cache.obsrv_err_cnt);
	
	for (i = 0; i < MAX_PCIE_NUM; i++) {
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"P%dPF\":\"%d\",", i,
				phealth->pcie[i].phy_init_fail_cnt);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"P%dLD\":\"%d\",", i,
				phealth->pcie[i].link_down_cnt);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"P%dLF\":\"%d\",", i,
				phealth->pcie[i].link_up_fail_cnt);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"P%dLT\":\"%x\",", i,
				phealth->pcie[i].link_up_fail_ltssm);

		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"dP%dPF\":\"%d\",", i,
				phealth->daily_pcie[i].phy_init_fail_cnt);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"dP%dLD\":\"%d\",", i,
				phealth->daily_pcie[i].link_down_cnt);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"dP%dLF\":\"%d\",", i,
				phealth->daily_pcie[i].link_up_fail_cnt);
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"dP%dLT\":\"%x\",", i,
				phealth->daily_pcie[i].link_up_fail_ltssm);
	}
	
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
			"\"dNP\":\"%d\",", phealth->daily_rr.np);
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
			"\"dRP\":\"%d\",", phealth->daily_rr.rp);
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
			"\"dMP\":\"%d\",", phealth->daily_rr.mp);
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
			"\"dKP\":\"%d\",", phealth->daily_rr.kp);
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
			"\"dDP\":\"%d\",", phealth->daily_rr.dp);
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
			"\"dWP\":\"%d\",", phealth->daily_rr.wp);
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
			"\"dTP\":\"%d\",", phealth->daily_rr.tp);
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
			"\"dSP\":\"%d\",", phealth->daily_rr.sp);
	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
			"\"dPP\":\"%d\",", phealth->daily_rr.pp);

	info_size--;
	info_size += sprintf((char*)(buf+info_size), "\n");

	return info_size;
}
static DEVICE_ATTR(ap_health, 0660, show_ap_health, store_ap_health);


static ssize_t show_ddr_info(struct device *dev,
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

	for (ch = 0; ch < 2; ch++) {
		for (cs = 0; cs < 2; cs++) {
			for (dq = 0; dq < 4; dq++) {
				info_size += snprintf((char*)(buf+info_size),
					MAX_LEN_STR - info_size,
					"\"RW_%d_%d_%d\":\"%d\",", ch, cs, dq,
					get_ddr_rcw_tDQSCK(ch, cs, dq));
				info_size += snprintf((char*)(buf+info_size),
					MAX_LEN_STR - info_size,
					"\"WC_%d_%d_%d\":\"%d\",", ch, cs, dq,
					get_ddr_wr_coarseCDC(ch, cs, dq));
				info_size += snprintf((char*)(buf+info_size),
					MAX_LEN_STR - info_size,
					"\"WF_%d_%d_%d\":\"%d\",", ch, cs, dq,
					get_ddr_wr_fineCDC(ch, cs, dq));
			}
		}
	}

	info_size--;
	info_size += sprintf((char*)(buf+info_size), "\n");

	return info_size;
}

static DEVICE_ATTR(ddr_info, 0440, show_ddr_info, NULL);

static int get_param0(int id)
{
	struct device_node *np = of_find_node_by_path("/soc/sec_hw_param");
	u32 val;
	static int num_param = 0;
	static u32 hw_param0[NUM_PARAM0];
	struct property *prop;
	const __be32 *p;

	if (num_param != 0)
		goto out;

	if (!np) {
		pr_err("No sec_hw_param found\n");
		return -1;
	}

	of_property_for_each_u32(np, "param0", prop, p, val) {
		hw_param0[num_param++] = val;

		if (num_param >= NUM_PARAM0)
			break;
	}

out:
	if (id < 0)
		return -2;

	return id < num_param ? hw_param0[id] : -1;
}

static ssize_t show_ap_info(struct device *dev,
			         struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int i, idx;
	char prefix[MAX_CLUSTER_NUM][3] = {"SC", "GC"};
#ifdef CONFIG_REGULATOR_CPR3
	int corner, max_corner;
#endif

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				"\"HW_REV\":\"%d\"", system_rev);

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				",\"SoC_ID\":\"%u\"", socinfo_get_id());

	info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				",\"SoC_REV\":\"%u.%u.%u\"",
				SOCINFO_VERSION_MAJOR(socinfo_get_version()),
				SOCINFO_VERSION_MINOR(socinfo_get_version()), 0);

	for (i = 0; i < MAX_CLUSTER_NUM; i++) {
#if defined(CONFIG_ARCH_MSM8953) || defined(CONFIG_ARCH_MSM8917)
		idx = 0;
#else
		idx = i;
#endif
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				",\"%s_PRM\":\"%d\"", prefix[i], get_param0(idx));
#ifdef CONFIG_REGULATOR_CPR3
		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				",\"%s_SB\":\"%d\"", prefix[i], cpr3_get_fuse_speed_bin(idx));

		info_size += snprintf((char*)(buf+info_size), MAX_LEN_STR - info_size,
				",\"%s_CR\":\"%d\"", prefix[i], cpr3_get_fuse_cpr_rev(idx));

		max_corner = cpr3_get_fuse_corner_count(idx);
		for (corner = 0; corner < max_corner; corner++) {
			info_size += snprintf((char*)(buf+info_size),
				MAX_LEN_STR - info_size,
				",\"%s_OPV_%d\":\"%d\"", prefix[i], corner,
				cpr3_get_fuse_open_loop_voltage(idx, corner));
		}
#endif
	}
	info_size += sprintf((char*)(buf+info_size), "\n");

	return info_size;
}

static DEVICE_ATTR(ap_info, 0440, show_ap_info, NULL);

static ssize_t show_extra_info(struct device *dev,
			         struct device_attribute *attr, char *buf)
{
	int offset = 0;
	char sub_buf[(SEC_DEBUG_EX_INFO_SIZE>>1)] = {0,};
	int sub_offset = 0;
	unsigned long rem_nsec;
	u64 ts_nsec;
	unsigned int reset_reason;
	struct sec_debug_reset_ex_info *info = NULL;

	reset_reason = sec_debug_get_reset_reason();
	if (reset_reason == USER_UPLOAD_CAUSE_PANIC) {

		/* store panic extra info
		   "KTIME":""	: kernel time
		   "FAULT":""	: pgd,va,*pgd,*pud,*pmd,*pte
		   "BUG":""	: bug msg
		   "PANIC":""	: panic buffer msg
		   "PC":""		: pc val
		   "LR":""		: link register val
		   "STACK":""	: backtrace
		   "CHIPID":""	: CPU Serial Number
		   "DBG0":""	: Debugging Option 0
		   "DBG1":""	: Debugging Option 1
		   "DBG2":""	: Debugging Option 2
		   "DBG3":""	: Debugging Option 3
		   "DBG4":""	: Debugging Option 4
		   "DBG5":""	: Debugging Option 5
		   */

		info = kmalloc(sizeof(struct sec_debug_reset_ex_info), GFP_KERNEL);
		if (!info) {
			pr_err("%s : fail - kmalloc\n", __func__);
			goto out;
		}

		if (!sec_get_param(param_index_reset_ex_info, info)) {
			pr_err("%s : fail - get param!!\n", __func__);
			goto out;
		}

		ts_nsec = info->ktime;
		rem_nsec = do_div(ts_nsec, 1000000000);

		offset += snprintf((char *)buf + offset, SEC_DEBUG_EX_INFO_SIZE - offset,
				"\"KTIME\":\"%lu.%06lu\",", (unsigned long)ts_nsec, rem_nsec / 1000);

		offset += snprintf((char *)buf + offset, SEC_DEBUG_EX_INFO_SIZE - offset,
				"\"FAULT\":\"pgd=%016llx VA=%016llx *pgd=%016llx *pud=%016llx *pmd=%016llx *pte=%016llx\",",
				info->fault_addr[0],info->fault_addr[1],info->fault_addr[2],
				info->fault_addr[3],info->fault_addr[4],info->fault_addr[5]);

		offset += snprintf((char *)buf + offset, SEC_DEBUG_EX_INFO_SIZE - offset,
				"\"BUG\":\"%s\",", info->bug_buf);

		offset += snprintf((char *)buf + offset, SEC_DEBUG_EX_INFO_SIZE - offset,
				"\"PANIC\":\"%s\",", info->panic_buf);

		offset += snprintf((char *)buf + offset, SEC_DEBUG_EX_INFO_SIZE - offset,
				"\"PC\":\"%s\",", info->pc);

		offset += snprintf((char *)buf + offset, SEC_DEBUG_EX_INFO_SIZE - offset,
				"\"LR\":\"%s\",", info->lr);

		offset += snprintf((char *)buf + offset, SEC_DEBUG_EX_INFO_SIZE - offset,
				"\"STACK\":\"%s\",", info->backtrace);

		sub_offset += snprintf((char *)sub_buf + sub_offset, (SEC_DEBUG_EX_INFO_SIZE>>1) - sub_offset,
				"\"CHIPID\":\"\",");

		sub_offset += snprintf((char *)sub_buf + sub_offset, (SEC_DEBUG_EX_INFO_SIZE>>1) - sub_offset,
				"\"DBG0\":\"Gladiator ERROR - %s\",", info->dbg0);

		sub_offset += snprintf((char *)sub_buf + sub_offset, (SEC_DEBUG_EX_INFO_SIZE>>1) - sub_offset,
				"\"DBG1\":\"%s\",", "");

		sub_offset += snprintf((char *)sub_buf + sub_offset, (SEC_DEBUG_EX_INFO_SIZE>>1) - sub_offset,
				"\"DBG2\":\"%s\",", "");

		sub_offset += snprintf((char *)sub_buf + sub_offset, (SEC_DEBUG_EX_INFO_SIZE>>1) - sub_offset,
				"\"DBG3\":\"%s\",", "");

		sub_offset += snprintf((char *)sub_buf + sub_offset, (SEC_DEBUG_EX_INFO_SIZE>>1) - sub_offset,
				"\"DBG4\":\"%s\",", "");

		sub_offset += snprintf((char *)sub_buf + sub_offset, (SEC_DEBUG_EX_INFO_SIZE>>1) - sub_offset,
				"\"DBG5\":\"%s\"", "");

		if (SEC_DEBUG_EX_INFO_SIZE - offset >= sub_offset) {
			offset += snprintf((char *)buf + offset, SEC_DEBUG_EX_INFO_SIZE - offset,
					"%s\n", sub_buf);
		} else {
			/* const 3 is ",\n size */
			offset += snprintf((char *)buf + (SEC_DEBUG_EX_INFO_SIZE - sub_offset - 3), sub_offset + 3,
					"\",%s\n", sub_buf);
		}
	}
out:
	if (info)
		kfree(info);

	return offset;
}

static DEVICE_ATTR(extra_info, 0440, show_extra_info, NULL);

static int sec_hw_param_notifier_callback(
	struct notifier_block *nfb, unsigned long action, void *data)
{
	switch (action) {
		case SEC_PARAM_DRV_INIT_DONE:
			phealth = ap_health_data_read();
			break;
		default:
			return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block sec_hw_param_notifier = {
	.notifier_call = sec_hw_param_notifier_callback,
};

static int __init sec_hw_param_init(void)
{
	struct device* sec_hw_param_dev;

	sec_hw_param_dev = sec_device_create(0, NULL, "sec_hw_param");

	if (IS_ERR(sec_hw_param_dev)) {
		pr_err("%s: Failed to create devce\n",__func__);
		goto out;
	}

	if (device_create_file(sec_hw_param_dev, &dev_attr_ap_info) < 0) {
		pr_err("%s: could not create ap_info sysfs node\n", __func__);
	}

	if (device_create_file(sec_hw_param_dev, &dev_attr_ddr_info) < 0) {
		pr_err("%s: could not create ddr_info sysfs node\n", __func__);
	}

	if (device_create_file(sec_hw_param_dev, &dev_attr_ap_health) < 0) {
		pr_err("%s: could not create ap_health sysfs node\n", __func__);
	}

	if (device_create_file(sec_hw_param_dev, &dev_attr_last_dcvs) < 0) {
		pr_err("%s: could not create last_dcvs sysfs node\n", __func__);
	}

	if (device_create_file(sec_hw_param_dev, &dev_attr_extra_info) < 0) {
		pr_err("%s: could not create extra_info sysfs node\n", __func__);
	}

	sec_param_notifier_register(&sec_hw_param_notifier);
out:
	return 0;
}
device_initcall(sec_hw_param_init);
