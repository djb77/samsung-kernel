/* drivers/gpu/arm/tMIx/r9p0/platform/exynos/gpu_exynos8895.c
 *
 * Copyright 2011 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali-T604 DVFS driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file gpu_exynos8895.c
 * DVFS
 */

#include <mali_kbase.h>

#include <linux/regulator/driver.h>
#include <linux/pm_qos.h>
#include <linux/delay.h>
#include <linux/smc.h>

#include <soc/samsung/asv-exynos.h>

#ifdef CONFIG_CAL_IF
#include <soc/samsung/cal-if.h>
#endif

#include <soc/samsung/exynos-pd.h>
#include <soc/samsung/exynos-pmu.h>

#ifdef CONFIG_MALI_BTS_OPTIMIZATION
#include <soc/samsung/bts.h>
#endif
#include <linux/clk.h>


#include "mali_kbase_platform.h"
#include "gpu_dvfs_handler.h"
#include "gpu_dvfs_governor.h"
#include "gpu_control.h"
#include "../mali_midg_regmap.h"


extern struct kbase_device *pkbdev;
#ifdef CONFIG_REGULATOR
#if defined(CONFIG_REGULATOR_S2MPS16)
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 17, 0)
#define EXYNOS_PMU_G3D_STATUS		0x40E4
#define LOCAL_PWR_CFG				(0xF << 0)
#endif
extern int s2m_get_dvs_is_on(void);
#endif
#else
#endif

#define EXYNOS_PMU_G3D_STATUS		0x40E4
#define LOCAL_PWR_CFG				(0xF << 0)

#ifdef CONFIG_MALI_DVFS
#define CPU_MAX PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE
#else
#define CPU_MAX -1
#endif

#ifndef KHZ
#define KHZ (1000)
#endif

#ifdef CONFIG_EXYNOS_BUSMONITOR
void __iomem *g3d0_outstanding_regs;
void __iomem *g3d1_outstanding_regs;
#endif /* CONFIG_EXYNOS_BUSMONITOR */

/*  clk,vol,abb,min,max,down stay, pm_qos mem, pm_qos int, pm_qos cpu_kfc_min, pm_qos cpu_egl_max */
static gpu_dvfs_info gpu_dvfs_table_default[] = {
	{839, 850000, 0, 98, 100, 1, 0, 1794000, 400000, 1690000, CPU_MAX},
	{764, 850000, 0, 98, 100, 1, 0, 1794000, 400000, 1690000, CPU_MAX},
	{683, 800000, 0, 98, 100, 1, 0, 1794000, 400000, 1690000, CPU_MAX},
	{572, 800000, 0, 78,  99, 5, 0, 1794000, 400000, 1690000, CPU_MAX},
	{546, 800000, 0, 78,  99, 5, 0, 1794000, 400000, 1690000, 2002000},
	{455, 800000, 0, 78,  85, 9, 0, 1540000, 400000, 1456000, 2002000},
	{385, 800000, 0, 78,  85, 1, 0, 1352000, 400000, 1248000, CPU_MAX},
	{338, 800000, 0, 78,  85, 1, 0, 1014000, 267000,  949000, CPU_MAX},
	{260, 800000, 0, 78,  85, 1, 0,  421000, 178000,       0, CPU_MAX},
};

static int mif_min_table[] = {
	 100000,  133000,  167000,
	 276000,  348000,  416000,
	 543000,  632000,  828000,
	1026000, 1264000, 1456000,
	1552000,
};

static gpu_attribute gpu_config_attributes[] = {
	{GPU_MAX_CLOCK, 546},
	{GPU_MAX_CLOCK_LIMIT, 546},
	{GPU_MIN_CLOCK, 260},
	{GPU_DVFS_START_CLOCK, 260},
	{GPU_DVFS_BL_CONFIG_CLOCK, 260},
	{GPU_GOVERNOR_TYPE, G3D_DVFS_GOVERNOR_INTERACTIVE},
	{GPU_GOVERNOR_START_CLOCK_DEFAULT, 260},
	{GPU_GOVERNOR_START_CLOCK_INTERACTIVE, 260},
	{GPU_GOVERNOR_START_CLOCK_STATIC, 260},
	{GPU_GOVERNOR_START_CLOCK_BOOSTER, 260},
	{GPU_GOVERNOR_TABLE_DEFAULT, (uintptr_t)&gpu_dvfs_table_default},
	{GPU_GOVERNOR_TABLE_INTERACTIVE, (uintptr_t)&gpu_dvfs_table_default},
	{GPU_GOVERNOR_TABLE_STATIC, (uintptr_t)&gpu_dvfs_table_default},
	{GPU_GOVERNOR_TABLE_BOOSTER, (uintptr_t)&gpu_dvfs_table_default},
	{GPU_GOVERNOR_TABLE_SIZE_DEFAULT, GPU_DVFS_TABLE_LIST_SIZE(gpu_dvfs_table_default)},
	{GPU_GOVERNOR_TABLE_SIZE_INTERACTIVE, GPU_DVFS_TABLE_LIST_SIZE(gpu_dvfs_table_default)},
	{GPU_GOVERNOR_TABLE_SIZE_STATIC, GPU_DVFS_TABLE_LIST_SIZE(gpu_dvfs_table_default)},
	{GPU_GOVERNOR_TABLE_SIZE_BOOSTER, GPU_DVFS_TABLE_LIST_SIZE(gpu_dvfs_table_default)},
	{GPU_GOVERNOR_INTERACTIVE_HIGHSPEED_CLOCK, 455},
	{GPU_GOVERNOR_INTERACTIVE_HIGHSPEED_LOAD, 95},
	{GPU_GOVERNOR_INTERACTIVE_HIGHSPEED_DELAY, 0},
	{GPU_DEFAULT_VOLTAGE, 800000},
	{GPU_COLD_MINIMUM_VOL, 0},
	{GPU_VOLTAGE_OFFSET_MARGIN, 37500},
	{GPU_TMU_CONTROL, 1},
	{GPU_TEMP_THROTTLING1, 546},
	{GPU_TEMP_THROTTLING2, 455},
	{GPU_TEMP_THROTTLING3, 385},
	{GPU_TEMP_THROTTLING4, 338},
	{GPU_TEMP_THROTTLING5, 260},
	{GPU_TEMP_TRIPPING, 260},
	{GPU_POWER_COEFF, 625}, /* all core on param */
	{GPU_DVFS_TIME_INTERVAL, 5},
	{GPU_DEFAULT_WAKEUP_LOCK, 1},
	{GPU_BUS_DEVFREQ, 0},
	{GPU_DYNAMIC_ABB, 0},
	{GPU_EARLY_CLK_GATING, 0},
	{GPU_DVS, 0},
	{GPU_INTER_FRAME_PM, 1},
	{GPU_PERF_GATHERING, 0},
	{GPU_RUNTIME_PM_DELAY_TIME, 50},
	{GPU_DVFS_POLLING_TIME, 30},
	{GPU_PMQOS_INT_DISABLE, 1},
	{GPU_PMQOS_MIF_MAX_CLOCK, 1794000},
	{GPU_PMQOS_MIF_MAX_CLOCK_BASE, 546},
	{GPU_CL_DVFS_START_BASE, 455},
	{GPU_DEBUG_LEVEL, DVFS_WARNING},
	{GPU_TRACE_LEVEL, TRACE_ALL},
	{GPU_MO_MIN_CLOCK, 455},
	{GPU_BOOST_EGL_MIN_LOCK, 1872000},
#ifdef CONFIG_MALI_VK_BOOST
	{GPU_VK_BOOST_MAX_LOCK, 338},
	{GPU_VK_BOOST_MIF_MIN_LOCK, 1794000},
#endif
	{GPU_CONFIG_LIST_END, 0}
};

int gpu_dvfs_decide_max_clock(struct exynos_context *platform)
{
	if (!platform)
		return -1;

	return 0;
}

void *gpu_get_config_attributes(void)
{
	return &gpu_config_attributes;
}

uintptr_t gpu_get_max_freq(void)
{
	return gpu_get_attrib_data(gpu_config_attributes, GPU_MAX_CLOCK) * 1000;
}

uintptr_t gpu_get_min_freq(void)
{
	return gpu_get_attrib_data(gpu_config_attributes, GPU_MIN_CLOCK) * 1000;
}

struct clk *vclk_g3d;
#ifdef CONFIG_REGULATOR
struct regulator *g3d_regulator;
struct regulator *g3d_m_regulator;
#endif /* CONFIG_REGULATOR */

int gpu_is_power_on(void)
{
	unsigned int val;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
	val = __raw_readl(EXYNOS_PMU_G3D_STATUS);
#else
	exynos_pmu_read(EXYNOS_PMU_G3D_STATUS, &val);
#endif

	return ((val & LOCAL_PWR_CFG) == LOCAL_PWR_CFG) ? 1 : 0;
}

int gpu_power_init(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	if (!platform)
		return -ENODEV;

	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "power initialized\n");

	return 0;
}

int gpu_get_cur_clock(struct exynos_context *platform)
{
	if (!platform)
		return -ENODEV;
	return cal_dfs_get_rate(platform->g3d_cmu_cal_id)/KHZ;
}


int gpu_register_dump(void)
{
	return 0;
}

#ifdef CONFIG_MALI_SEC_JOB_STATUS_CHECK
#if defined(CONFIG_MALI_FENCE_DEBUG)
#error YOU MUST turn off MALI_FENCE_DEBUG.
#endif

#include "../../backend/gpu/mali_kbase_jm_rb.h"
int gpu_job_fence_status_dump(struct sync_fence *timeout_fence);
static char *gpu_fence_status_to_string(int status)
{
	if (status == 0)
		return "signaled";
	else if (status > 0)
		return "active";
	else
		return "error";
}

void gpu_fence_debug_check_dependency_atom(struct kbase_jd_atom *katom)
{
	struct kbase_context *kctx = katom->kctx;
	struct device *dev = kctx->kbdev->dev;
	int i;

	for (i = 0; i < 2; i++) {
		struct kbase_jd_atom *dep;

		list_for_each_entry(dep, &katom->dep_head[i], dep_item[i]) {
			if (dep->status == KBASE_JD_ATOM_STATE_UNUSED ||
					dep->status == KBASE_JD_ATOM_STATE_COMPLETED)
				continue;

			if ((dep->core_req & BASE_JD_REQ_SOFT_JOB_TYPE) == BASE_JD_REQ_SOFT_FENCE_TRIGGER ||
				(dep->core_req & BASE_JD_REQ_SOFT_JOB_TYPE) == BASE_JD_REQ_SOFT_FENCE_WAIT) {
				struct sync_fence *fence = dep->fence;
				int status = atomic_read(&fence->status);

				/* Found blocked fence. */
				dev_warn(dev,
						"\t\t\t\t--- Atom %d fence [%p] %s: %s, fence type = 0x%x\n",
						kbase_jd_atom_id(kctx, dep),
						fence, fence->name,
						gpu_fence_status_to_string(status),
						dep->core_req);
			} else {
				dev_warn(dev,
						"\t\t\t\t--- Atom %d\n", kbase_jd_atom_id(kctx, dep));
			}

			/* gpu_fence_debug_check_dependency_atom(dep); */
		}
	}
}

int gpu_job_fence_status_dump(struct sync_fence *timeout_fence)
{
	struct device *dev;
	struct list_head *entry;
	const struct list_head *kbdev_list;
	struct kbase_device *kbdev = NULL;
	struct kbasep_kctx_list_element *element;
	struct kbase_context *kctx;
	struct sync_fence *fence;
	unsigned long lflags;
	int i;
	int cnt[5] = {0,};

	/* dev_warn(dev,"GPU JOB STATUS DUMP\n"); */

	kbdev_list = kbase_dev_list_get();

	if (kbdev_list == NULL) {
		kbase_dev_list_put(kbdev_list);
		return -ENODEV;
	}

	list_for_each(entry, kbdev_list) {
		kbdev = list_entry(entry, struct kbase_device, entry);

		if (kbdev == NULL) {
			kbase_dev_list_put(kbdev_list);
			return -ENODEV;
		}

		dev = kbdev->dev;
		dev_warn(dev, "[%p] kbdev dev name : %s\n", kbdev, kbdev->devname);
		mutex_lock(&kbdev->kctx_list_lock);
		list_for_each_entry(element, &kbdev->kctx_list, link) {
			kctx = element->kctx;
			mutex_lock(&kctx->jctx.lock);
			dev_warn(dev, "\t[%p] kctx(%d_%d_%d)_jobs_nr(%d)\n", kctx, kctx->pid, kctx->tgid, kctx->id, kctx->jctx.job_nr);
			if (kctx->jctx.job_nr > 0) {
				for (i = BASE_JD_ATOM_COUNT-1; i > 0; i--) {
					if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_UNUSED) {
						cnt[0]++;
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_QUEUED) {
						cnt[1]++;
						dev_warn(dev, "\t\t- [%p] Atom %d STATE_QUEUED\n", &kctx->jctx.atoms[i], i);
						/* dev_warn(dev, "		-- Atom %d slot_nr 0x%x coreref_state 0x%x core_req 0x%x event_code 0x%x gpu_rb_state 0x%x\n",
								i, kctx->jctx.atoms[i].slot_nr, kctx->jctx.atoms[i].coreref_state, kctx->jctx.atoms[i].core_req, kctx->jctx.atoms[i].event_code, kctx->jctx.atoms[i].gpu_rb_state); */
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_IN_JS) {
						cnt[2]++;
						dev_warn(dev, "\t\t- [%p] Atom %d STATE_IN_JS\n", &kctx->jctx.atoms[i], i);
						dev_warn(dev, "\t\t\t-- Atom %d	slot_nr 0x%x coreref_state 0x%x core_req 0x%x event_code 0x%x gpu_rb_state 0x%x\n",
								i, kctx->jctx.atoms[i].slot_nr, kctx->jctx.atoms[i].coreref_state, kctx->jctx.atoms[i].core_req, kctx->jctx.atoms[i].event_code, kctx->jctx.atoms[i].gpu_rb_state);
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_HW_COMPLETED) {
						cnt[3]++;
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_COMPLETED) {
						cnt[4]++;
					}

					spin_lock_irqsave(&kctx->waiting_soft_jobs_lock, lflags);
					/* Print fence infomation */
					fence = kctx->jctx.atoms[i].fence;
					if (fence != NULL) {
						dev_warn(dev, "\t\t\t-- Atom %d Fence Info [%p] %s: %s, fence type = 0x%x, %s\n",
								i, fence, fence->name, gpu_fence_status_to_string(atomic_read(&fence->status)), kctx->jctx.atoms[i].core_req, (fence == timeout_fence) ? "***" : "  ");
					}
					/* Print dependency atom infomation */
					if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_QUEUED || kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_IN_JS) {
						dev_warn(dev, "\t\t\t-- Dependency Atom List\n");
						gpu_fence_debug_check_dependency_atom(&kctx->jctx.atoms[i]);
					}
					spin_unlock_irqrestore(&kctx->waiting_soft_jobs_lock, lflags);
				}
				dev_warn(dev, "\t\t: ATOM STATE INFO : UNUSED(%d)_QUEUED(%d)_IN_JS(%d)_HW_COMPLETED(%d)_COMPLETED(%d)\n", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4]);
				cnt[0] = cnt[1] = cnt[2] = cnt[3] = cnt[4] = 0;
			}
			mutex_unlock(&kctx->jctx.lock);
		}
		mutex_unlock(&kbdev->kctx_list_lock);
		/* katom list in backed slot rb */
		kbase_gpu_dump_slots(kbdev);
	}

	if (timeout_fence != NULL)
		dev_warn(dev, "Guilty Fence *** [%p] %s: %s\n", timeout_fence, timeout_fence->name, gpu_fence_status_to_string(atomic_read(&timeout_fence->status)));

	kbase_dev_list_put(kbdev_list);

	return 0;
}
#endif

#ifdef CONFIG_MALI_DVFS
static int gpu_set_dvfs(struct exynos_context *platform, int clk)
{
	unsigned long g3d_rate = clk * KHZ;
	int ret = 0;

#ifdef CONFIG_MALI_RT_PM
	if (platform->exynos_pm_domain)
		mutex_lock(&platform->exynos_pm_domain->access_lock);

	if (!gpu_is_power_on()) {
		ret = -1;
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "%s: can't set clock in the power-off state!\n", __func__);
		goto err;
	}
#endif /* CONFIG_MALI_RT_PM */

	if (clk == platform->cur_clock) {
		ret = 0;
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "%s: skipped to set clock for %dMhz!\n", __func__, platform->cur_clock);

#ifdef CONFIG_MALI_RT_PM
		if (platform->exynos_pm_domain)
			mutex_unlock(&platform->exynos_pm_domain->access_lock);
#endif
		return ret;
	}

	cal_dfs_set_rate(platform->g3d_cmu_cal_id, g3d_rate);

	platform->cur_clock = cal_dfs_get_rate(platform->g3d_cmu_cal_id)/KHZ;

	GPU_LOG(DVFS_INFO, LSI_CLOCK_VALUE, g3d_rate/KHZ, platform->cur_clock,
		"[id: %x] clock set: %ld, clock get: %d\n", platform->g3d_cmu_cal_id, g3d_rate/KHZ, platform->cur_clock);

#ifdef CONFIG_MALI_RT_PM
err:
	if (platform->exynos_pm_domain)
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
#endif /* CONFIG_MALI_RT_PM */
	return ret;
}

static int gpu_get_clock(struct kbase_device *kbdev)
{
#ifdef CONFIG_OF
	struct device_node *np = NULL;
#endif
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform)
		return -ENODEV;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

#ifdef CONFIG_OF
#ifdef CONFIG_CAL_IF
	np = kbdev->dev->of_node;

	if (np != NULL) {
		if (of_property_read_u32(np, "g3d_cmu_cal_id", &platform->g3d_cmu_cal_id)) {
			GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: failed to get CMU CAL ID [ACPM_DVFS_G3D]\n", __func__);
			return -1;
		}
	}
#endif
#endif

	return 0;
}
#endif

int gpu_clock_init(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_DVFS
	int ret;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	ret = gpu_get_clock(kbdev);
	if (ret < 0)
		return -1;

#ifdef CONFIG_EXYNOS_BUSMONITOR
	g3d0_outstanding_regs = ioremap(0x14A00000, SZ_1K);
	g3d1_outstanding_regs = ioremap(0x14A20000, SZ_1K);
#endif /* CONFIG_EXYNOS_BUSMONITOR */

	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "clock initialized\n");
#endif
	return 0;
}

int gpu_get_cur_voltage(struct exynos_context *platform)
{
	int ret = 0;
#ifdef CONFIG_REGULATOR
	if (!g3d_regulator) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: regulator is not initialized\n", __func__);
		return -1;
	}

	ret = regulator_get_voltage(g3d_regulator);
#endif /* CONFIG_REGULATOR */
	return ret;
}

static struct gpu_control_ops ctr_ops = {
	.is_power_on = gpu_is_power_on,
#ifdef CONFIG_MALI_DVFS
	.set_dvfs = gpu_set_dvfs,
	.set_voltage = NULL,
	.set_voltage_pre = NULL,
	.set_voltage_post = NULL,
	.set_clock_to_osc = NULL,
	.set_clock = NULL,
	.set_clock_pre = NULL,
	.set_clock_post = NULL,
	.enable_clock = NULL,
	.disable_clock = NULL,
#endif
};

struct gpu_control_ops *gpu_get_control_ops(void)
{
	return &ctr_ops;
}

#ifdef CONFIG_REGULATOR
extern int s2m_set_dvs_pin(bool gpio_val);
int gpu_enable_dvs(struct exynos_context *platform)
{
#ifdef CONFIG_MALI_RT_PM
	if (!platform->dvs_status)
		return 0;

	if (!gpu_is_power_on()) {
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "%s: can't set dvs in the power-off state!\n", __func__);
		return -1;
	}

#if defined(CONFIG_REGULATOR_S2MPS16)
	/* Do not need to enable dvs during suspending */
	if (!pkbdev->pm.suspending) {
		if (cal_dfs_ext_ctrl(dvfs_g3d, cal_dfs_dvs, 1) != 0) {
			GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: failed to enable dvs\n", __func__);
			return -1;
		}
	}
#endif /* CONFIG_REGULATOR_S2MPS16 */

	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "dvs is enabled (vol: %d)\n", gpu_get_cur_voltage(platform));
#endif
	return 0;
}

int gpu_disable_dvs(struct exynos_context *platform)
{
	if (!platform->dvs_status)
		return 0;

#ifdef CONFIG_MALI_RT_PM
#if defined(CONFIG_REGULATOR_S2MPS16)
	if (cal_dfs_ext_ctrl(dvfs_g3d, cal_dfs_dvs, 0) != 0) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: failed to disable dvs\n", __func__);
		return -1;
	}
#endif /* CONFIG_REGULATOR_S2MPS16 */

	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "dvs is disabled (vol: %d)\n", gpu_get_cur_voltage(platform));
#endif
	return 0;
}

int gpu_inter_frame_power_on(struct exynos_context *platform)
{
#ifdef CONFIG_MALI_RT_PM
	int status;

	if (!platform->inter_frame_pm_status)
		return 0;

	mutex_lock(&platform->exynos_pm_domain->access_lock);

	status = cal_pd_status(platform->exynos_pm_domain->cal_pdid);
	if (status) {
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "%s: status checking : Already gpu inter frame power on\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return 0;
	}

	if (cal_pd_control(platform->exynos_pm_domain->cal_pdid, 1) != 0) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: failed to gpu inter frame power on\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return -1;
	}

	status = cal_pd_status(platform->exynos_pm_domain->cal_pdid);
	if (!status) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: status error : gpu inter frame power on\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return -1;
	}

	mutex_unlock(&platform->exynos_pm_domain->access_lock);
	GPU_LOG(DVFS_INFO, LSI_IFPM_POWER_ON, 0u, 0u, "gpu inter frame power on\n");
#endif
	return 0;
}

int gpu_inter_frame_power_off(struct exynos_context *platform)
{
#ifdef CONFIG_MALI_RT_PM
	int status;

	if (!platform->inter_frame_pm_status)
		return 0;

	mutex_lock(&platform->exynos_pm_domain->access_lock);

	status = cal_pd_status(platform->exynos_pm_domain->cal_pdid);
	if (!status) {
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "%s: status checking: Already gpu inter frame power off\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return 0;
	}

	if (cal_pd_control(platform->exynos_pm_domain->cal_pdid, 0) != 0) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: failed to gpu inter frame power off\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return -1;
	}

	status = cal_pd_status(platform->exynos_pm_domain->cal_pdid);
	if (status) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: status error :  gpu inter frame power off\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return -1;
	}

	mutex_unlock(&platform->exynos_pm_domain->access_lock);
	GPU_LOG(DVFS_INFO, LSI_IFPM_POWER_OFF, 0u, 0u, "gpu inter frame power off\n");
#endif
	return 0;
}

#ifdef CONFIG_MALI_ASV_CALIBRATION_SUPPORT
struct workqueue_struct *gpu_asv_cali_wq;
struct delayed_work gpu_asv_cali_stop_work;

int gpu_asv_calibration_start(void)
{
	struct exynos_context *platform = (struct exynos_context *) pkbdev->platform_context;

	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return -ENODEV;
	}

	gpu_control_power_policy_set(pkbdev, "always_on");
	gpu_dvfs_clock_lock(GPU_DVFS_MAX_LOCK, ASV_CALI_LOCK, 546);
	gpu_dvfs_clock_lock(GPU_DVFS_MIN_LOCK, ASV_CALI_LOCK, 546);
	gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_RESET);
	return 0;
}

int gpu_asv_calibration_stop(void)
{
	gpu_dvfs_clock_lock(GPU_DVFS_MAX_UNLOCK, ASV_CALI_LOCK, 0);
	gpu_dvfs_clock_lock(GPU_DVFS_MIN_UNLOCK, ASV_CALI_LOCK, 0);
	gpu_control_power_policy_set(pkbdev, "demand");
	return 0;
}

static void gpu_asv_calibration_stop_callback(struct work_struct *data)
{
	struct exynos_context *platform = (struct exynos_context *) pkbdev->platform_context;

	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return;
	}

	gpu_dvfs_clock_lock(GPU_DVFS_MAX_UNLOCK, ASV_CALI_LOCK, 0);
	gpu_dvfs_clock_lock(GPU_DVFS_MIN_UNLOCK, ASV_CALI_LOCK, 0);
	gpu_control_power_policy_set(pkbdev, "demand");
	platform->gpu_auto_cali_status = false;
}

int gpu_asv_calibration_start_wq(void)
{
	struct exynos_context *platform = (struct exynos_context *) pkbdev->platform_context;

	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return -ENODEV;
	}

	platform->gpu_auto_cali_status = true;
	gpu_control_power_policy_set(pkbdev, "always_on");
	gpu_dvfs_clock_lock(GPU_DVFS_MAX_LOCK, ASV_CALI_LOCK, 546);
	gpu_dvfs_clock_lock(GPU_DVFS_MIN_LOCK, ASV_CALI_LOCK, 546);

	if (NULL == gpu_asv_cali_wq) {
		INIT_DELAYED_WORK(&gpu_asv_cali_stop_work, gpu_asv_calibration_stop_callback);
		gpu_asv_cali_wq = create_workqueue("g3d_asv_cali");

		queue_delayed_work_on(0, gpu_asv_cali_wq,
				&gpu_asv_cali_stop_work, msecs_to_jiffies(15000));	/* 15 second */
	}

	return 0;
}
#endif

int gpu_regulator_init(struct exynos_context *platform)
{
	return 0;
}
#endif /* CONFIG_REGULATOR */

int *get_mif_table(int *size)
{
	*size = ARRAY_SIZE(mif_min_table);
	return mif_min_table;
}
