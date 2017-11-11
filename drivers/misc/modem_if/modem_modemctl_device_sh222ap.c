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

#include <linux/platform_data/modem_v1.h>
#include <linux/platform_data/modemif.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "../mcu_ipc/mcu_ipc.h"

#define MIF_INIT_TIMEOUT	(300 * HZ)
#define MBREG_MAX_NUM 64
#define TMU_BASE		(0x100C0000)

static struct pm_qos_request pm_qos_req_mif;
static struct pm_qos_request pm_qos_req_cpu;

enum cp_mode {
	CP_POWER_ON,
	CP_RESET,
	CP_POWER_OFF,
	NUM_CP_MODE,
};

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

static ssize_t modem_ctrl_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	int ops_num;
	int ret;

	ret = sscanf(buf, "%1d", &ops_num);
	if (ret == 0)
		return count;

	switch (ops_num) {
	case 1:
		mif_info("Reset CP (Stop)!!!\n");
		mc->pmu->stop();
		break;

	case 2:
		mif_info("Reset CP - Release (Start)!!!\n");
		mc->pmu->start();
		break;

	case 3:
		mif_info("force_crash_exit!!!\n");
		mc->ops.modem_force_crash_exit(mc);
		break;

	case 4:
		mif_info("Modem Power OFF!!!\n");
		mc->pmu->power(CP_POWER_OFF);
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

#define TMU_COUNT	5

static DEVICE_ATTR(modem_ctrl, 0644, modem_ctrl_show, modem_ctrl_store);

static int code_to_temp(int tmp_code, int trim)
{
	int min, max, temp;

	min = 31;
	max = 146;

	if (tmp_code < min || tmp_code > max) {
		temp = -ENODATA;
		return -1;
	}

//	return tmp_code - 0b110010 + 25;
	return tmp_code - ((trim>>8) & 0xff) + 25;
}

static void bh_pm_tmu_req(struct work_struct *ws)
{
	struct modem_ctl *mc;
	struct link_device *ld;
	struct file *fp = NULL;
	char buf[100];
	const char *path = "/sys/class/thermal/thermal_zone0/temp";
	int thermal = 0;
	int len = 0;
	int ret = 0;
	int i = 0;
	int tmp;
	int reg_thermal;
	int trim_info;
	volatile u8 __iomem *addr;

	msleep(100);
	addr = ioremap(TMU_BASE, 0x90);
	tmp = readl(addr + 0x40);
	trim_info = readl(addr + 0x0);
	reg_thermal = code_to_temp(tmp, trim_info);

	mc = container_of(ws, struct modem_ctl, pm_tmu_work);
	ld = get_current_link(mc->iod);

	fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		mif_err("%s: ERR! %s open fail\n", __func__, path);
		return;
	}

	do {
		ret = fp->f_op->read(fp, &buf[i++], 1, &fp->f_pos);
		if( ret > 0) {
			mif_err("@@@ %s: read data: %d\n", __func__, (int)buf[i-1]);

		}
	} while(ret > 0);

	filp_close(fp, NULL);

	len = i - 5;
	for(i = 0; i < len; i++) {

		thermal = thermal * 10 + (buf[i] - 0x30);
	}

	mif_err("@@@ %s: read thermal value: %d\n", __func__, thermal);
	mif_err("@@@ %s: read register thermal value: %d,,,,%d\n", __func__, reg_thermal, tmp);

	ld->send_tmu(ld, mc->iod, thermal);
}

static void bh_pm_qos_req(struct work_struct *ws)
{
	struct modem_ctl *mc;
	int qos_val;
	int cpu_val;
	int mif_val;

	mc = container_of(ws, struct modem_ctl, pm_qos_work);

	qos_val = mbox_get_value(mc->mbx_perf_req);
	mif_err("pm_qos:0x%x requested\n", qos_val);

	cpu_val = (qos_val & 0xff);
	if (cpu_val == 0xff) {
		mif_err("Unlock CPU(0x%x)\n", cpu_val);
		pm_qos_update_request(&pm_qos_req_cpu, 0);
	} else if (cpu_val >= 0 && cpu_val <= 12) {
		mif_err("Lock CPU : %d\n", (14 - cpu_val) * 100000);
		pm_qos_update_request(&pm_qos_req_cpu, (14 - cpu_val) * 100000);
	} else {
		mif_err("Unexpected CPU LOCK reuest : %d\n(UNLOCK CPU)", cpu_val);
		pm_qos_update_request(&pm_qos_req_cpu, 0);
	}

	mif_val = (qos_val >> 8) & 0xff;

	switch (mif_val) {
	case 0:
		mif_err("Lock MIF : 400\n");
		pm_qos_update_request(&pm_qos_req_mif, 400 * 1000);
		break;
	case 1:
		mif_err("Lock MIF : 267\n");
		pm_qos_update_request(&pm_qos_req_mif, 267 * 1000);
		break;
	case 2:
		mif_err("Lock MIF : 200\n");
		pm_qos_update_request(&pm_qos_req_mif, 200 * 1000);
		break;
	case 0xff:
		mif_err("Unlock MIF(0x%x)\n", mif_val);
		pm_qos_update_request(&pm_qos_req_mif, 0);
		break;
	default:
		mif_err("Unexpected MIF LOCK reuest : 0x%x\n(UNLOCK MIF)", mif_val);
		pm_qos_update_request(&pm_qos_req_mif, 0);
	}
}

static irqreturn_t cp_wdt_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct link_device *ld = get_current_link(mc->iod);
	int new_state;

	mif_disable_irq(&mc->irq_cp_wdt);
	mif_err("%s: ERR! CP_WDOG occurred\n", mc->name);

	mc->pmu->clear_cp_wdt();

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

	mc->pmu->clear_cp_fail();

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
	int cp_on = mc->pmu->get_pwr_status();
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

static void tmu_req_handler(void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	mif_info("CP Request to get TMU Value!!!!!\n");

	schedule_work(&mc->pm_tmu_work);
}

static int sh222ap_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	int cp_active = mbox_get_value(mc->mbx_phone_active);
	int cp_status = mbox_get_value(mc->mbx_cp_status);
	int ret;
	int i;

	mif_info("+++\n");
	mif_info("cp_active:%d cp_status:%d\n", cp_active, cp_status);

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	for (i = 0; i < MBREG_MAX_NUM; i++)
		mbox_set_value(i, 0);

	mbox_set_value(mc->mbx_sys_rev, mc->hw_revision);
	mif_err("System Revision %d\n", mbox_get_value(mc->mbx_sys_rev));

	mbox_set_value(mc->mbx_pkg_id, mc->package_id);
	mif_err("Package ID %d\n", mbox_get_value(mc->mbx_pkg_id));

	mbox_set_value(mc->mbx_pmic_rev, get_pmic_rev());
	mif_err("PMIC Rev : %d\n", mbox_get_value(mc->mbx_pmic_rev));

	mbox_set_value(mc->mbx_pda_active, 1);

	ret = mc->pmu->power(CP_POWER_ON);
	if (ret < 0) {
		mc->pmu->stop();
		msleep(10);
		mc->pmu->start();
	} else {
		msleep(300);
	}

	mif_info("---\n");
	return 0;
}


static int sh222ap_off(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	int cp_on = mc->pmu->get_pwr_status();
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
	mc->pmu->stop();

	mif_info("---\n");
	return 0;
}

static int sh222ap_reset(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	mif_err("+++\n");

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	mc->pmu->stop();

	msleep(10);

	mc->pmu->start();

	mif_err("---\n");
	return 0;
}

static int sh222ap_boot_on(struct modem_ctl *mc)
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

static int sh222ap_boot_off(struct modem_ctl *mc)
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

static int sh222ap_boot_done(struct modem_ctl *mc)
{
	mif_info("+++\n");

	if (wake_lock_active(&mc->mc_wake_lock))
		wake_unlock(&mc->mc_wake_lock);

	mif_info("---\n");
	return 0;
}

static int sh222ap_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_err("+++\n");

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	mif_err("---\n");
	return 0;
}

static int sh222ap_dump_reset(struct modem_ctl *mc)
{
	mif_err("+++\n");

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	mc->pmu->stop();

	mif_err("---\n");
	return 0;
}

static int sh222ap_dump_start(struct modem_ctl *mc)
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

	err = mc->pmu->start();

	mbox_set_value(mc->mbx_ap_status, 1);

	mif_err("---\n");
	return err;
}

static int sh222ap_get_meminfo(struct modem_ctl *mc, unsigned long arg)
{
	struct meminfo mem_info;
	mem_info.base_addr = shm_get_phys_base();
	mem_info.size = shm_get_phys_size() / SZ_1M;

	if (copy_to_user((void __user *)arg, &mem_info, sizeof(struct meminfo)))
		return -EFAULT;

	return 0;
}

static int sh222ap_suspend_modemctl(struct modem_ctl *mc)
{
	mbox_set_value(mc->mbx_pda_active, 0);
	mbox_set_interrupt(mc->int_pda_active);

	return 0;
}

static int sh222ap_resume_modemctl(struct modem_ctl *mc)
{
	mbox_set_value(mc->mbx_pda_active, 1);
	mbox_set_interrupt(mc->int_pda_active);

	/* Below codes will be used next time.
	if (get_cp2ap_status(mc))
		mc->request_pm_qos(mc);
	*/

	return 0;
}

static void sh222ap_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = sh222ap_on;
	mc->ops.modem_off = sh222ap_off;
	mc->ops.modem_reset = sh222ap_reset;
	mc->ops.modem_boot_on = sh222ap_boot_on;
	mc->ops.modem_boot_off = sh222ap_boot_off;
	mc->ops.modem_boot_done = sh222ap_boot_done;
	mc->ops.modem_force_crash_exit = sh222ap_force_crash_exit;
	mc->ops.modem_dump_reset = sh222ap_dump_reset;
	mc->ops.modem_dump_start = sh222ap_dump_start;
	mc->ops.modem_get_meminfo = sh222ap_get_meminfo;
	mc->ops.suspend_modem_ctrl = sh222ap_suspend_modemctl;
	mc->ops.resume_modem_ctrl = sh222ap_resume_modemctl;
}

static void sh222ap_get_pdata(struct modem_ctl *mc, struct modem_data *modem)
{
	struct modem_mbox *mbx = modem->mbx;

	mc->mbx_pda_active = mbx->mbx_ap2cp_active;
	mc->int_pda_active = mbx->int_ap2cp_active;

	mc->mbx_phone_active = mbx->mbx_cp2ap_active;
	mc->irq_phone_active = mbx->irq_cp2ap_active;

	mc->mbx_ap_status = mbx->mbx_ap2cp_status;
	mc->mbx_cp_status = mbx->mbx_cp2ap_status;

	mc->mbx_perf_req = mbx->mbx_cp2ap_perf_req;
	mc->irq_perf_req = mbx->irq_cp2ap_perf_req;

	mc->mbx_sys_rev = mbx->mbx_ap2cp_sys_rev;
	mc->mbx_pmic_rev = mbx->mbx_ap2cp_pmic_rev;
	mc->mbx_pkg_id = mbx->mbx_ap2cp_pkg_id;

	mc->hw_revision = modem->hw_revision;
	mc->package_id = modem->package_id;
	mc->pmu = modem->pmu;
}

int init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	struct platform_device *pdev = to_platform_device(mc->dev);
	int ret = 0;
	int irq_num;
	unsigned long flags = IRQF_NO_SUSPEND | IRQF_NO_THREAD;
	mif_err("+++\n");

	sh222ap_get_ops(mc);
	sh222ap_get_pdata(mc, pdata);
	dev_set_drvdata(mc->dev, mc);

	wake_lock_init(&mc->mc_wake_lock, WAKE_LOCK_SUSPEND, "umts_wake_lock");

	/*
	** Register CP_FAIL interrupt handler
	*/
	irq_num = platform_get_irq(pdev, 0);
	mif_init_irq(&mc->irq_cp_fail, irq_num, "cp_fail", flags);

	ret = mif_request_irq(&mc->irq_cp_fail, cp_fail_handler, mc);
	if (ret)
		return ret;

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

	/* CP_WDT interrupt must be enabled only after CP booting */
	mc->irq_cp_wdt.active = true;
	mif_disable_irq(&mc->irq_cp_wdt);

	/*
	** Register DVFS_REQ MBOX interrupt handler
	*/
	mbox_request_irq(mc->irq_perf_req, dvfs_req_handler, mc);
	mif_err("dvfs_req_handler registered\n");

	/*
	** Register TMU MBOX interrupt handler
	*/
	mbox_request_irq(mc->irq_tmu_req, tmu_req_handler, mc);
	mif_err("tmu_req_handler registered\n");

	INIT_WORK(&mc->pm_tmu_work, bh_pm_tmu_req);

	/*
	** Register LTE_ACTIVE MBOX interrupt handler
	*/
	mbox_request_irq(mc->irq_phone_active, cp_active_handler, mc);
	mif_err("cp_active_handler registered\n");

	pm_qos_add_request(&pm_qos_req_mif, PM_QOS_BUS_THROUGHPUT, 0);
	pm_qos_add_request(&pm_qos_req_cpu, PM_QOS_CPU_FREQ_MIN, 0);
	INIT_WORK(&mc->pm_qos_work, bh_pm_qos_req);


	init_completion(&mc->off_cmpl);

	ret = device_create_file(mc->dev, &dev_attr_modem_ctrl);
	if (ret)
		mif_err("can't create modem_ctrl!!!\n");

	mif_info("---\n");
	return 0;
}

