/*
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mcu_ipc.h>
#include <linux/of_gpio.h>
#include <linux/smc.h>
#include <linux/exynos-pci-ctrl.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_link_device_shmem.h"
#include "pmu-cp.h"

#define MIF_INIT_TIMEOUT	(300 * HZ)
#define MBREG_MAX_NUM 64

static struct pm_qos_request pm_qos_req_mif;
static struct pm_qos_request pm_qos_req_cpu;
static struct pm_qos_request pm_qos_req_int;

static struct modem_ctl *g_mc;

static ssize_t modem_ctrl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct modem_ctl *mc = dev_get_drvdata(dev);

	ret = snprintf(buf, PAGE_SIZE, "Check Mailbox Registers\n");
	ret += snprintf(&buf[ret], PAGE_SIZE - ret, "AP2CP_STATUS : %d\n",
				mbox_get_value(mc->mbx_ap_status));
	ret += snprintf(&buf[ret], PAGE_SIZE - ret, "CP2AP_STATUS : %d\n",
				mbox_get_value(mc->mbx_cp_status));
	ret += snprintf(&buf[ret], PAGE_SIZE - ret, "PDA_ACTIVE : %d\n",
				mbox_get_value(mc->mbx_pda_active));
	ret += snprintf(&buf[ret], PAGE_SIZE - ret, "CP2AP_DVFS_REQ : %d\n",
				mbox_get_value(mc->mbx_perf_req));
	ret += snprintf(&buf[ret], PAGE_SIZE - ret, "PHONE_ACTIVE : %d\n",
				mbox_get_value(mc->mbx_phone_active));
	ret += snprintf(&buf[ret], PAGE_SIZE - ret, "HW_REVISION : %d\n",
				mbox_get_value(mc->mbx_sys_rev));

	return ret;
}

static ssize_t modem_ctrl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	long ops_num;
	int ret;

	ret = kstrtol(buf, 10, &ops_num);
	if (ret == 0)
		return count;

	switch (ops_num) {
	case 1:
		mif_info("Reset CP (Stop)!!!\n");
		exynos_cp_reset(mc);
		break;

	case 2:
		mif_info("Reset CP - Release (Start)!!!\n");
		exynos_cp_release(mc);
		break;

	case 3:
		mif_info("force_crash_exit!!!\n");
		mc->ops.modem_force_crash_exit(mc);
		break;

	case 4:
		mif_info("Modem Power OFF!!!\n");
		exynos_set_cp_power_onoff(mc, CP_POWER_OFF);
		break;

	default:
		mif_info("Wrong operation number\n");
		mif_info("1. Modem Reset - (Stop)\n");
		mif_info("2. Modem Reset - (Start)\n");
		mif_info("3. Modem force crash\n");
		mif_info("4. Modem Power OFF\n");
	}

	return count;
}

static DEVICE_ATTR(modem_ctrl, 0644, modem_ctrl_show, modem_ctrl_store);

static void bh_pm_qos_req(struct work_struct *ws)
{
	struct modem_ctl *mc;
	unsigned int qos_val;
	unsigned int level;

	mc = container_of(ws, struct modem_ctl, pm_qos_work);

	qos_val = mbox_get_value(mc->mbx_perf_req);
	mif_err("pm_qos:0x%x requested\n", qos_val);

	level = (qos_val & 0xff);
	if (level > 0 && level <= mc->ap_clk_cnt) {
		mif_err("Lock CPU(%u)\n", mc->ap_clk_table[level - 1]);
		pm_qos_update_request(&pm_qos_req_cpu,
				mc->ap_clk_table[level - 1]);
	} else {
		mif_err("Unlock CPU(%u)\n", level);
		pm_qos_update_request(&pm_qos_req_cpu, 0);
	}

	level = (qos_val >> 8) & 0xff;
	if (level > 0 && level <= mc->mif_clk_cnt) {
		mif_err("Lock MIF(%u)\n", mc->mif_clk_table[level - 1]);
		pm_qos_update_request(&pm_qos_req_mif,
				mc->mif_clk_table[level - 1]);
	} else {
		mif_err("Unlock MIF(%u)\n", level);
		pm_qos_update_request(&pm_qos_req_mif, 0);
	}

	level = (qos_val >> 16) & 0xff;
	if (level > 0 && level <= mc->int_clk_cnt) {
		mif_err("Lock INT(%u)\n", mc->int_clk_table[level - 1]);
		pm_qos_update_request(&pm_qos_req_int,
				mc->int_clk_table[level - 1]);
	} else {
		mif_err("Unlock INT(%u)\n", level);
		pm_qos_update_request(&pm_qos_req_int, 0);
	}

	mbox_set_value(mc->mbx_perf_req, 0);
}

static void bh_pm_qos_req_cpu(struct work_struct *ws)
{
	struct modem_ctl *mc;
	int qos_val;
	int cpu_val;

	mc = container_of(ws, struct modem_ctl, pm_qos_work_cpu);

	qos_val = mbox_get_value(mc->mbx_perf_req_cpu);
	mif_err("pm_qos:0x%x requested\n", qos_val);

	cpu_val = (qos_val & 0xff);
	if (cpu_val > 0 && cpu_val <= mc->ap_clk_cnt) {
		mif_err("Lock CPU[%d] : %u\n", cpu_val,
				mc->ap_clk_table[cpu_val - 1]);
		pm_qos_update_request(&pm_qos_req_cpu, mc->ap_clk_table[cpu_val - 1]);
	} else {
		mif_err("Unlock CPU(req_val : %d)\n", qos_val);
		pm_qos_update_request(&pm_qos_req_cpu, 0);
	}
}

static void bh_pm_qos_req_mif(struct work_struct *ws)
{
	struct modem_ctl *mc;
	int qos_val;
	int mif_val;

	mc = container_of(ws, struct modem_ctl, pm_qos_work_mif);

	qos_val = mbox_get_value(mc->mbx_perf_req_mif);
	mif_err("pm_qos:0x%x requested\n", qos_val);

	mif_val = (qos_val & 0xff);
	if (mif_val > 0 && mif_val <= mc->mif_clk_cnt) {
		mif_err("Lock MIF[%d] : %u\n", mif_val,
				mc->mif_clk_table[mif_val - 1]);
		pm_qos_update_request(&pm_qos_req_mif, mc->mif_clk_table[mif_val - 1]);
	} else {
		mif_err("Unlock MIF(req_val : %d)\n", qos_val);
		pm_qos_update_request(&pm_qos_req_mif, 0);
	}
}

static void bh_pm_qos_req_int(struct work_struct *ws)
{
	struct modem_ctl *mc;
	int qos_val;
	int int_val;

	mc = container_of(ws, struct modem_ctl, pm_qos_work_int);

	qos_val = mbox_get_value(mc->mbx_perf_req_int);
	mif_err("pm_qos:0x%x requested\n", qos_val);

	int_val = (qos_val & 0xff);
	if (int_val > 0 && int_val <= mc->int_clk_cnt) {
		mif_err("Lock INT[%d] : %u\n", int_val,
				mc->int_clk_table[int_val - 1]);
		pm_qos_update_request(&pm_qos_req_int, mc->int_clk_table[int_val - 1]);
	} else {
		mif_err("Unlock INT(req_val : %d)\n", qos_val);
		pm_qos_update_request(&pm_qos_req_int, 0);
	}
}

static irqreturn_t cp_wdt_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct link_device *ld = get_current_link(mc->iod);
	int new_state;

	mif_disable_irq(&mc->irq_cp_wdt);
	mif_err("%s: ERR! CP_WDOG occurred\n", mc->name);

	exynos_clear_cp_reset(mc);

	new_state = STATE_CRASH_EXIT;
	ld->mode = LINK_MODE_ULOAD;

	mif_err("new_state = %s\n", get_cp_state_str(new_state));
	mc->bootd->modem_state_changed(mc->bootd, new_state);
	mc->iod->modem_state_changed(mc->iod, new_state);

	return IRQ_HANDLED;
}

static irqreturn_t cp_fail_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct link_device *ld = get_current_link(mc->iod);
	int new_state;

	mif_disable_irq(&mc->irq_cp_fail);
	mif_err("%s: ERR! CP_FAIL occurred\n", mc->name);

	exynos_cp_active_clear(mc);

	new_state = STATE_CRASH_RESET;
	ld->mode = LINK_MODE_OFFLINE;

	mif_err("new_state = %s\n", get_cp_state_str(new_state));
	mc->bootd->modem_state_changed(mc->bootd, new_state);
	mc->iod->modem_state_changed(mc->iod, new_state);

	return IRQ_HANDLED;
}

static void cp_active_handler(void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct link_device *ld = get_current_link(mc->iod);
	int cp_on = exynos_get_cp_power_status(mc);
	int cp_active = mbox_get_value(mc->mbx_phone_active);
	int old_state = mc->phone_state;
	int new_state = mc->phone_state;

	mif_err("old_state:%s cp_on:%d cp_active:%d\n",
		get_cp_state_str(old_state), cp_on, cp_active);

	if (!cp_active) {
		if (cp_on) {
			new_state = STATE_OFFLINE;
			ld->mode = LINK_MODE_OFFLINE;
			complete_all(&mc->off_cmpl);
		} else {
			mif_err("don't care!!!\n");
		}
	}

	if (old_state != new_state) {
		mif_err("new_state = %s\n", get_cp_state_str(new_state));
		mc->bootd->modem_state_changed(mc->bootd, new_state);
		mc->iod->modem_state_changed(mc->iod, new_state);
	}
}

static void dvfs_req_handler(void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	mif_info("CP reqest to change DVFS level!!!!\n");

	schedule_work(&mc->pm_qos_work);
}

static void dvfs_req_handler_cpu(void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	mif_info("CP reqest to change DVFS level!!!!\n");

	schedule_work(&mc->pm_qos_work_cpu);
}

static void dvfs_req_handler_mif(void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	mif_info("CP reqest to change DVFS level!!!!\n");

	schedule_work(&mc->pm_qos_work_mif);
}

static void dvfs_req_handler_int(void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	mif_info("CP reqest to change DVFS level!!!!\n");

	schedule_work(&mc->pm_qos_work_int);
}

static void pcie_l1ss_disable_handler(void *arg)
{
#if defined(CONFIG_PCI_EXYNOS_L1SS)
	struct modem_mbox *mbx = (struct modem_mbox *)arg;
	unsigned int req;

	req = mbox_get_value(mbx->mbx_cp2ap_pcie_l1ss_disable);
	if (req == 1) {
		exynos_pcie_l1ss_ctrl(0, PCIE_L1SS_CTRL_MODEM_IF);
		mif_err("cp requests pcie l1ss disable\n");
	} else if (req == 0) {
		exynos_pcie_l1ss_ctrl(1, PCIE_L1SS_CTRL_MODEM_IF);
		mif_err("cp requests pcie l1ss enable\n");
	} else {
		mif_err("unsupported request: pcie_l1ss_disable\n");
	}
#endif
}

static int get_hw_rev(struct device_node *np)
{
	int value, cnt, gpio_cnt;
	unsigned gpio_hw_rev, hw_rev = 0;

	gpio_cnt = of_gpio_count(np);
	if (gpio_cnt < 0) {
		mif_err("failed to get gpio_count from DT(%d)\n", gpio_cnt);
		return gpio_cnt;
	}

	for (cnt = 0; cnt < gpio_cnt; cnt++) {
		gpio_hw_rev = of_get_gpio(np, cnt);
		if (!gpio_is_valid(gpio_hw_rev)) {
			mif_err("gpio_hw_rev%d: Invalied gpio\n", cnt);
			return -EINVAL;
		}

		value = gpio_get_value(gpio_hw_rev);
		hw_rev |= (value & 0x1) << cnt;
	}

	return hw_rev;
}

static int sh333ap_on(struct modem_ctl *mc)
{
	struct platform_device *pdev = to_platform_device(mc->dev);
	struct device_node *np = pdev->dev.of_node;
	struct link_device *ld = get_current_link(mc->iod);
	int cp_active = mbox_get_value(mc->mbx_phone_active);
	int cp_status = mbox_get_value(mc->mbx_cp_status);
	int ret;
	int i;
#if defined(CONFIG_LTE_MODEM_S5E7890) || defined(CONFIG_LTE_MODEM_S5E8890)
	struct modem_data *modem = mc->mdm_data;
#endif

#ifdef CONFIG_SOC_EXYNOS8890
	u32 __maybe_unused et_dac_cal;
	register u64 reg0 __asm__("x0");
	register u64 reg1 __asm__("x1");
	register u64 reg2 __asm__("x2");
	register u64 reg3 __asm__("x3");
#endif

	mif_info("+++\n");
	mif_info("cp_active:%d cp_status:%d\n", cp_active, cp_status);

	exynos_pmu_cp_init(mc);
	usleep_range(10000, 11000);

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	for (i = 0; i < MBREG_MAX_NUM; i++)
		mbox_set_value(i, 0);

	mbox_set_value(mc->mbx_sys_rev, get_hw_rev(np));
	mif_err("System Revision %d\n", mbox_get_value(mc->mbx_sys_rev));

#if defined(CONFIG_LTE_MODEM_S5E7890) || defined(CONFIG_LTE_MODEM_S5E8890)
	/* copy memory cal value from ap to cp */
	memmove(modem->ipc_base + 0x1000, mc->sysram_alive, 0x800);

	/* Print CP BIN INFO */
	mif_err("CP build info : %s\n", mc->info_cp_build);
	mif_err("CP carrior info : %s\n", mc->info_cp_carr);
#endif

#ifdef CONFIG_SOC_EXYNOS8890
	/* Transfer ET_DAC_CAL tuning bit to CP */
	reg0 = 0;
	reg1 = 0;
	reg2 = 0;
	reg3 = 0;

	/* SMC_ID = 0x82001014, CMD_ID = 0x2002, read_idx=0x1C */
	ret = exynos_smc(0x82001014, 0, 0x2002, 0x1C);
	__asm__ volatile(
			"\t"
			: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
			);
	et_dac_cal = (uint32_t)((0x00000000FFFF0000 & reg2) >> 16);

	mif_err("ret = 0x%x, et_cac_cal value = 0x%x\n", ret, et_dac_cal);
	mbox_set_value(mc->mbx_et_dac_cal,(u32)et_dac_cal);
#endif

	mbox_set_value(mc->mbx_pda_active, 1);

	ret = exynos_set_cp_power_onoff(mc, CP_POWER_ON);
	if (ret < 0) {
		exynos_cp_reset(mc);
		usleep_range(10000, 11000);
		exynos_set_cp_power_onoff(mc, CP_POWER_ON);
		usleep_range(10000, 11000);
		exynos_cp_release(mc);
	} else {
		msleep(300);
	}

	mif_info("---\n");
	return 0;
}

static int sh333ap_off(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	int cp_on = exynos_get_cp_power_status(mc);
	unsigned long timeout = msecs_to_jiffies(3000);
	unsigned long remain;
	mif_info("+++\n");

	if (mc->phone_state == STATE_OFFLINE || cp_on == 0)
		goto exit;

	init_completion(&mc->off_cmpl);
	remain = wait_for_completion_timeout(&mc->off_cmpl, timeout);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		mc->phone_state = STATE_OFFLINE;
		ld->mode = LINK_MODE_OFFLINE;
		mc->bootd->modem_state_changed(mc->iod, STATE_OFFLINE);
		mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);
	}

exit:
	exynos_cp_reset(mc);

	mif_info("---\n");
	return 0;
}

static int sh333ap_reset(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);

	if (*(unsigned int *)(mc->mdm_data->ipc_base + SHM_CPINFO_DEBUG)
			== 0xDEB)
		return 0;

	mif_err("+++\n");

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	exynos_cp_reset(mc);

	usleep_range(10000, 11000);

	mif_err("---\n");
	return 0;
}

static int sh333ap_boot_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	int cnt = 100;
	mif_info("+++\n");

	ld->mode = LINK_MODE_BOOT;

	mc->bootd->modem_state_changed(mc->bootd, STATE_BOOTING);
	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);

	while (mbox_get_value(mc->mbx_cp_status) == 0) {
		if (--cnt > 0)
			usleep_range(10000, 20000);
		else
			return -EACCES;
	}

	mif_disable_irq(&mc->irq_cp_wdt);
	mif_enable_irq(&mc->irq_cp_fail);

	mbox_set_value(mc->mbx_ap_status, 1);

	mif_info("---\n");
	return 0;
}

static int sh333ap_boot_off(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	unsigned long remain;
	int err = 0;
	mif_info("+++\n");

	ld->mode = LINK_MODE_IPC;

	remain = wait_for_completion_timeout(&ld->init_cmpl, MIF_INIT_TIMEOUT);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		err = -EAGAIN;
		goto exit;
	}

	mif_enable_irq(&mc->irq_cp_wdt);

	mif_info("---\n");

exit:
	mif_disable_irq(&mc->irq_cp_fail);
	return err;
}

static int sh333ap_boot_done(struct modem_ctl *mc)
{
	mif_info("+++\n");

	if (wake_lock_active(&mc->mc_wake_lock))
		wake_unlock(&mc->mc_wake_lock);

	mif_info("---\n");
	return 0;
}

static int sh333ap_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_err("+++\n");

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	mif_err("---\n");
	return 0;
}

int force_crash_exit_ext(void)
{
	mif_err("+++\n");

	if (g_mc)
		sh333ap_force_crash_exit(g_mc);

	mif_err("---\n");
	return 0;
}

static int sh333ap_dump_reset(struct modem_ctl *mc)
{
	mif_err("+++\n");

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	exynos_cp_reset(mc);

	mif_err("---\n");
	return 0;
}

static int sh333ap_dump_start(struct modem_ctl *mc)
{
	int err;
	struct link_device *ld = get_current_link(mc->bootd);
	mif_err("+++\n");

	if (!ld->dump_start) {
		mif_err("ERR! %s->dump_start not exist\n", ld->name);
		return -EFAULT;
	}

	err = ld->dump_start(ld, mc->bootd);
	if (err)
		return err;

	err = exynos_cp_release(mc);

	mbox_set_value(mc->mbx_ap_status, 1);

	mif_err("---\n");
	return err;
}

static int sh333ap_get_meminfo(struct modem_ctl *mc, unsigned long arg)
{
	struct meminfo mem_info;
	struct modem_data *modem = mc->mdm_data;

	mem_info.base_addr = modem->shmem_base;
	mem_info.size = modem->ipcmem_offset;

	if (copy_to_user((void __user *)arg, &mem_info, sizeof(struct meminfo)))
		return -EFAULT;

	return 0;
}

static int sh333ap_suspend_modemctl(struct modem_ctl *mc)
{
	mbox_set_value(mc->mbx_pda_active, 0);
	mbox_set_interrupt(mc->int_pda_active);

	return 0;
}

static int sh333ap_resume_modemctl(struct modem_ctl *mc)
{
	mbox_set_value(mc->mbx_pda_active, 1);
	mbox_set_interrupt(mc->int_pda_active);

	/* Below codes will be used next time.
	if (get_cp2ap_status(mc))
		mc->request_pm_qos(mc);
	*/

	return 0;
}

static void sh333ap_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = sh333ap_on;
	mc->ops.modem_off = sh333ap_off;
	mc->ops.modem_reset = sh333ap_reset;
	mc->ops.modem_boot_on = sh333ap_boot_on;
	mc->ops.modem_boot_off = sh333ap_boot_off;
	mc->ops.modem_boot_done = sh333ap_boot_done;
	mc->ops.modem_force_crash_exit = sh333ap_force_crash_exit;
	mc->ops.modem_dump_reset = sh333ap_dump_reset;
	mc->ops.modem_dump_start = sh333ap_dump_start;
	mc->ops.modem_get_meminfo = sh333ap_get_meminfo;
	mc->ops.suspend_modem_ctrl = sh333ap_suspend_modemctl;
	mc->ops.resume_modem_ctrl = sh333ap_resume_modemctl;
}

static void sh333ap_get_pdata(struct modem_ctl *mc, struct modem_data *modem)
{
	struct platform_device *pdev = to_platform_device(mc->dev);
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct modem_mbox *mbx = modem->mbx;
	const struct property *prop;
	const __be32 *val;
	int nr;

	mc->mbx_pda_active = mbx->mbx_ap2cp_active;
	mc->int_pda_active = mbx->int_ap2cp_active;

	mc->mbx_phone_active = mbx->mbx_cp2ap_active;
	mc->irq_phone_active = mbx->irq_cp2ap_active;

	mc->mbx_ap_status = mbx->mbx_ap2cp_status;
	mc->mbx_cp_status = mbx->mbx_cp2ap_status;

	mc->mbx_perf_req = mbx->mbx_cp2ap_perf_req;
	mc->irq_perf_req = mbx->irq_cp2ap_perf_req;
	mc->mbx_perf_req_cpu = mbx->mbx_cp2ap_perf_req_cpu;
	mc->irq_perf_req_cpu = mbx->irq_cp2ap_perf_req_cpu;
	mc->mbx_perf_req_mif = mbx->mbx_cp2ap_perf_req_mif;
	mc->irq_perf_req_mif = mbx->irq_cp2ap_perf_req_mif;
	mc->mbx_perf_req_int = mbx->mbx_cp2ap_perf_req_int;
	mc->irq_perf_req_int = mbx->irq_cp2ap_perf_req_int;

	mc->mbx_et_dac_cal = mbx->mbx_ap2cp_et_dac_cal;

	mc->mbx_sys_rev = mbx->mbx_ap2cp_sys_rev;
	mc->mbx_pmic_rev = mbx->mbx_ap2cp_pmic_rev;
	mc->mbx_pkg_id = mbx->mbx_ap2cp_pkg_id;
	mc->mbx_lock_value = mbx->mbx_ap2cp_lock_value;

	mc->hw_revision = modem->hw_revision;
	mc->package_id = modem->package_id;
	mc->lock_value = modem->lock_value;

	/* get AP core clock table from DT */
	prop = of_find_property(np, "mif,ap_clk_table", NULL);
	if (!prop || !prop->value) {
		mif_err("failed to get ap_clk_table prop\n");
		mc->ap_clk_cnt = 0;
	} else {
		nr = prop->length / sizeof(u32);
		mc->ap_clk_cnt = nr;

		mc->ap_clk_table = devm_kzalloc(dev, sizeof(u32) * nr, GFP_KERNEL);
		if (!mc->ap_clk_table)
			mif_err("ap_clk_table: failed to alloc memory\n");
		val = prop->value;
		while (nr--)
			mc->ap_clk_table[nr] = be32_to_cpup(val++);
	}

	/* get MIF clock table from DT */
	prop = of_find_property(np, "mif,mif_clk_table", NULL);
	if (!prop || !prop->value) {
		mif_err("failed to get mif_clk_table prop\n");
		mc->mif_clk_cnt = 0;
	} else {
		nr = prop->length / sizeof(u32);
		mc->mif_clk_cnt = nr;

		mc->mif_clk_table = devm_kzalloc(dev, sizeof(u32) * nr, GFP_KERNEL);
		if (!mc->mif_clk_table)
			mif_err("mif_clk_table: failed to alloc memory\n");
		val = prop->value;
		while (nr--)
			mc->mif_clk_table[nr] = be32_to_cpup(val++);
	}

	/* get INT clock table from DT */
	prop = of_find_property(np, "mif,int_clk_table", NULL);
	if (!prop || !prop->value) {
		mif_err("failed to get int_clk_table prop\n");
		mc->int_clk_cnt = 0;
	} else {
		nr = prop->length / sizeof(u32);
		mc->int_clk_cnt = nr;

		mc->int_clk_table = devm_kzalloc(dev, sizeof(u32) * nr, GFP_KERNEL);
		if (!mc->int_clk_table)
			mif_err("int_clk_table: failed to alloc memory\n");
		val = prop->value;
		while (nr--)
			mc->int_clk_table[nr] = be32_to_cpup(val++);
	}
}

int init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	struct platform_device *pdev = to_platform_device(mc->dev);
	int ret = 0;
	int irq_num;
	struct resource *sysram_alive;
	unsigned long flags = IRQF_NO_SUSPEND | IRQF_NO_THREAD;
	struct modem_mbox *mbx;

	mif_err("+++\n");

	g_mc = mc;

	sh333ap_get_ops(mc);
	sh333ap_get_pdata(mc, pdata);
	dev_set_drvdata(mc->dev, mc);
	mbx = pdata->mbx;

	wake_lock_init(&mc->mc_wake_lock, WAKE_LOCK_SUSPEND, "umts_wake_lock");

	/*
	** Register CP_FAIL interrupt handler
	*/
	irq_num = platform_get_irq(pdev, 0);
	mif_init_irq(&mc->irq_cp_fail, irq_num, "cp_fail", flags);

	ret = mif_request_irq(&mc->irq_cp_fail, cp_fail_handler, mc);
	if (ret)
		return ret;

	irq_set_irq_wake(irq_num, 1);

	/* CP_FAIL interrupt must be enabled only during CP booting */
	mc->irq_cp_fail.active = true;
	mif_disable_irq(&mc->irq_cp_fail);

	/*
	** Register CP_WDT interrupt handler
	*/
	irq_num = platform_get_irq(pdev, 1);
	mif_init_irq(&mc->irq_cp_wdt, irq_num, "cp_wdt", flags);

	ret = mif_request_irq(&mc->irq_cp_wdt, cp_wdt_handler, mc);
	if (ret)
		return ret;

	irq_set_irq_wake(irq_num, 1);

	/* CP_WDT interrupt must be enabled only after CP booting */
	mc->irq_cp_wdt.active = true;
	mif_disable_irq(&mc->irq_cp_wdt);

	/*
	** Register DVFS_REQ MBOX interrupt handler
	*/
	mbox_request_irq(mc->irq_perf_req, dvfs_req_handler, mc);
	mif_err("dvfs_req_handler registered\n");

	mbox_request_irq(mc->irq_perf_req_cpu,
			dvfs_req_handler_cpu, mc);
	mif_err("dvfs_req_handler_cpu registered\n");

	mbox_request_irq(mc->irq_perf_req_mif,
			dvfs_req_handler_mif, mc);
	mif_err("dvfs_req_handler_mif registered\n");

	mbox_request_irq(mc->irq_perf_req_int,
			dvfs_req_handler_int, mc);
	mif_err("dvfs_req_handler_int registered\n");

	/*
	** Register PCIE_CTRL MBOX interrupt handler
	*/
	mbox_request_irq(mbx->irq_cp2ap_pcie_l1ss_disable,
			pcie_l1ss_disable_handler, mbx);
	mif_err("pcie_l1ss_disable_handler registered\n");

	/*
	** Register LTE_ACTIVE MBOX interrupt handler
	*/
	mbox_request_irq(mc->irq_phone_active, cp_active_handler, mc);
	mif_err("cp_active_handler registered\n");

	pm_qos_add_request(&pm_qos_req_cpu, PM_QOS_CLUSTER0_FREQ_MIN, 0);
	pm_qos_add_request(&pm_qos_req_mif, PM_QOS_BUS_THROUGHPUT, 0);
	pm_qos_add_request(&pm_qos_req_int, PM_QOS_DEVICE_THROUGHPUT, 0);
	INIT_WORK(&mc->pm_qos_work, bh_pm_qos_req);
	INIT_WORK(&mc->pm_qos_work_cpu, bh_pm_qos_req_cpu);
	INIT_WORK(&mc->pm_qos_work_mif, bh_pm_qos_req_mif);
	INIT_WORK(&mc->pm_qos_work_int, bh_pm_qos_req_int);

	sysram_alive = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mc->sysram_alive = devm_ioremap_resource(&pdev->dev, sysram_alive);
	if (IS_ERR(mc->sysram_alive)) {
		ret = PTR_ERR(mc->sysram_alive);
		return ret;
	}

	init_completion(&mc->off_cmpl);

	ret = device_create_file(mc->dev, &dev_attr_modem_ctrl);
	if (ret)
		mif_err("can't create modem_ctrl!!!\n");

	mif_err("---\n");
	return 0;
}
