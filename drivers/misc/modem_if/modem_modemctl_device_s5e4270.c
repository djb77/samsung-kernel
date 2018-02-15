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

#include <linux/platform_data/modem_v1.h>

#include <asm/cacheflush.h>

#include "modem_prj.h"
#include "modem_link_device_shmem.h"
#include "modem_utils.h"
#include "../mcu_ipc/mcu_ipc.h"

#define MIF_INIT_TIMEOUT	(30 * HZ)
#define MCU_CPWAKE_INT	1
#define MCU_APWAKE_INT	1
#define MCU_AP2CP_STAT 2
#define MCU_CP2AP_STAT 2
#define MCU_CP2AP_DVFS_REQ 4

#define MBREG_AP2CP_WAKEUP 2
#define MBREG_CP2AP_WAKEUP 3
#define MBREG_AP2CP_STATUS 4
#define MBREG_CP2AP_STATUS 5
#define MBREG_PDA_ACTIVE 6
#define MBREG_PHONE_ACTIVE 7

static ssize_t modem_ctrl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = sprintf(buf, "Modem Control SysFS\n");
	ret += sprintf(&buf[ret], "1. Modem Reset\n");

	return ret;
}

static ssize_t modem_ctrl_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ops_num;
	struct modem_ctl *mc = dev_get_drvdata(dev);
	sscanf(buf, "%d", &ops_num);

	switch (ops_num) {
	case 1:
		mif_info("Reset CP!!!\n");
		mc->cp_reset();
		break;
	default:
		mif_info("Wrong operation number\n");
		mif_info("1. Modem Reset\n");
	}

	return count;
}

static DEVICE_ATTR(modem_ctrl, 0644, modem_ctrl_show, modem_ctrl_store);

static void s5e4270_mc_state_fsm(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int old_state = mc->phone_state;
	int new_state = mc->phone_state;

	mif_err("old_state:%s cp_on:%d cp_reset:%d cp_active:%d\n",
		get_cp_state_str(old_state), cp_on, cp_reset, cp_active);

	if (!cp_active) {
		if (!cp_on) {
			new_state = STATE_OFFLINE;
			ld->mode = LINK_MODE_OFFLINE;
		} else if (old_state == STATE_ONLINE) {
			new_state = STATE_CRASH_EXIT;
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

static irqreturn_t phone_active_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int cp_reset = gpio_get_value(mc->gpio_cp_reset);

	if (cp_reset)
		s5e4270_mc_state_fsm(mc);

	return IRQ_HANDLED;
}

static irqreturn_t cp2ap_status_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct link_device *ld = get_current_link(mc->bootd);
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_off = gpio_get_value(mc->gpio_cp_off);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int cp_status = gpio_get_value(mc->gpio_cp_status);

	if (ld->mode != LINK_MODE_BOOT)
		goto exit;

	mif_err("cp_on:%d cp_reset:%d ps_hold:%d cp_active:%d cp_status:%d\n",
		cp_on, cp_reset, cp_off, cp_active, cp_status);

exit:
	return IRQ_HANDLED;
}

static irqreturn_t cp2ap_dvfsreq_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	mif_err("[Modem IF]CP reqest to change DVFS level!!!!\n");

	return IRQ_HANDLED;
}

static inline void make_gpio_floating(int gpio, bool floating)
{
	if (floating)
		gpio_direction_input(gpio);
	else
		gpio_direction_output(gpio, 0);
}

static int s5e4270_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	#if 0	/* Masked by Kisang */
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_off = gpio_get_value(mc->gpio_cp_off);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int cp_status = gpio_get_value(mc->gpio_cp_status);
	mif_err("+++\n");
	mif_err("cp_on:%d cp_reset:%d ps_hold:%d cp_active:%d cp_status:%d\n",
		cp_on, cp_reset, cp_off, cp_active, cp_status);

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	/* Make PS_HOLD_OFF floating (Hi-Z) for CP ON */
	make_gpio_floating(mc->gpio_cp_off, true);

	gpio_set_value(mc->gpio_cp_on, 0);
	msleep(100);

	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(500);

	gpio_set_value(mc->gpio_cp_on, 1);
	msleep(100);

	gpio_set_value(mc->gpio_cp_reset, 1);

	mif_err("---\n");
	#endif
	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);
	#ifdef MODEM_CACHE_FLUSH
	mif_err("cache is flushed!!\n");
	dmac_flush_range(phys_to_virt(0x60000000), phys_to_virt(0x64000000));
	#endif

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	mc->cp_onoff(0);

	return 0;
}

static int s5e4270_off(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	#if 0 /* Masked by Kisang */
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	mif_err("+++\n");

	if (mc->phone_state == STATE_OFFLINE || cp_on == 0)
		return 0;

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	gpio_set_value(mc->gpio_cp_reset, 0);
	make_gpio_floating(mc->gpio_cp_off, false);
	gpio_set_value(mc->gpio_cp_on, 0);

	mif_err("---\n");
	#endif

	mc->phone_state = STATE_OFFLINE;
	ld->mode = LINK_MODE_OFFLINE;

	mc->cp_onoff(2);

	return 0;
}

static int s5e4270_reset(struct modem_ctl *mc)
{
	#if 0 /* Masked by Kisang */
	mif_err("+++\n");

	if (s5e4270_off(mc))
		return -EIO;

	msleep(100);

	if (s5e4270_on(mc))
		return -EIO;

	mif_err("---\n");
	#endif

	mc->cp_reset();

	return 0;
}

static int s5e4270_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_err("+++\n");

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	mif_err("---\n");
	return 0;
}

static int s5e4270_dump_reset(struct modem_ctl *mc)
{
	mif_err("+++\n");

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	gpio_set_value(mc->gpio_host_active, 0);
	gpio_set_value(mc->gpio_cp_reset, 0);

	udelay(200);

	gpio_set_value(mc->gpio_cp_reset, 1);

	msleep(300);

	mif_err("---\n");
	return 0;
}

static int s5e4270_boot_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	int remain = 0;
	#if 0 /* masked by Kisang */
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_off = gpio_get_value(mc->gpio_cp_off);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int cp_status = gpio_get_value(mc->gpio_cp_status);
	mif_err("+++\n");
	mif_err("cp_on:%d cp_reset:%d ps_hold:%d cp_active:%d cp_status:%d\n",
		cp_on, cp_reset, cp_off, cp_active, cp_status);
	#endif

	disable_irq_nosync(mc->irq_phone_active);
	mc->bootd->modem_state_changed(mc->bootd, STATE_BOOTING);
	mc->iod->modem_state_changed(mc->iod, STATE_BOOTING);
	ld->mode = LINK_MODE_BOOT;
	mc->cp_onoff(0);

	/* Added by Kisang */
	remain = wait_for_completion_timeout(&ld->init_cmpl, MIF_INIT_TIMEOUT);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		mif_err("xxx\n");
		return -EAGAIN;
	}
	/* End */
	return 0;
}

static int s5e4270_boot_off(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_off = gpio_get_value(mc->gpio_cp_off);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int cp_status = gpio_get_value(mc->gpio_cp_status);
	unsigned long remain;
	mif_err("+++\n");
	mif_err("cp_on:%d cp_reset:%d ps_hold:%d cp_active:%d cp_status:%d\n",
		cp_on, cp_reset, cp_off, cp_active, cp_status);

	remain = wait_for_completion_timeout(&ld->init_cmpl, MIF_INIT_TIMEOUT);
	cp_on = gpio_get_value(mc->gpio_cp_on);
	cp_off = gpio_get_value(mc->gpio_cp_off);
	cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	cp_active = gpio_get_value(mc->gpio_phone_active);
	cp_status = gpio_get_value(mc->gpio_cp_status);
	mif_err("cp_on:%d cp_reset:%d ps_hold:%d cp_active:%d cp_status:%d\n",
		cp_on, cp_reset, cp_off, cp_active, cp_status);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		mif_err("xxx\n");
		return -EAGAIN;
	}

	mif_err("---\n");

	return 0;
}

static int s5e4270_boot_done(struct modem_ctl *mc)
{
	mif_err("+++\n");

	if (wake_lock_active(&mc->mc_wake_lock))
		wake_unlock(&mc->mc_wake_lock);

#if 0
	enable_irq(mc->irq_phone_active);
#endif

	mif_err("---\n");
	return 0;
}

#if 0
static int s5e4270_get_cp_status(struct modem_ctl *mc)
{
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_off = gpio_get_value(mc->gpio_cp_off);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int cp_status = gpio_get_value(mc->gpio_cp_status);

	mif_err("cp_on:%d cp_reset:%d ps_hold:%d cp_active:%d cp_status:%d\n",
		cp_on, cp_reset, cp_off, cp_active, cp_status);

	return cp_status;
}

static int s5e4270_set_ap_status(struct modem_ctl *mc)
{
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_off = gpio_get_value(mc->gpio_cp_off);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int cp_status = gpio_get_value(mc->gpio_cp_status);

	mif_err("cp_on:%d cp_reset:%d ps_hold:%d cp_active:%d cp_status:%d\n",
		cp_on, cp_reset, cp_off, cp_active, cp_status);

	gpio_set_value(mc->gpio_ap_status, 1);

	return 0;
}

static int s5e4270_clear_ap_status(struct modem_ctl *mc)
{
	int cp_on = gpio_get_value(mc->gpio_cp_on);
	int cp_off = gpio_get_value(mc->gpio_cp_off);
	int cp_reset  = gpio_get_value(mc->gpio_cp_reset);
	int cp_active = gpio_get_value(mc->gpio_phone_active);
	int cp_status = gpio_get_value(mc->gpio_cp_status);

	mif_err("cp_on:%d cp_reset:%d ps_hold:%d cp_active:%d cp_status:%d\n",
		cp_on, cp_reset, cp_off, cp_active, cp_status);

	gpio_set_value(mc->gpio_ap_status, 0);

	return 0;
}
#endif

static irqreturn_t cp_wdt_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;

	//==> Check This mc->cp_reset();

	return IRQ_HANDLED;
}

static int s5e4270_get_cp_status(struct modem_ctl *mc)
{
	mif_err("<%s>\n", __func__);

	return mbox_get_value(MBREG_CP2AP_STATUS);
}

static int s5e4270_set_ap_status(struct modem_ctl *mc)
{
	mif_err("<%s>\n", __func__);

	mbox_set_value(MBREG_AP2CP_STATUS, 1);

	return 0;
}

static int s5e4270_clear_ap_status(struct modem_ctl *mc)
{
	mif_err("<%s>\n", __func__);

	mbox_set_value(MBREG_AP2CP_STATUS, 0);

	return 0;
}

static int s5e4270_set_pda_active(struct modem_ctl *mc)
{
	mif_err("<%s>\n", __func__);

	mbox_set_value(MBREG_PDA_ACTIVE, 1);

	return 0;
}

static int s5e4270_clear_pda_active(struct modem_ctl *mc)
{
	mif_err("<%s>\n", __func__);

	mbox_set_value(MBREG_PDA_ACTIVE, 0);

	return 0;
}

static int s5e4270_get_phone_active(struct modem_ctl *mc)
{
	mif_err("<%s>\n", __func__);

	return mbox_get_value(MBREG_PHONE_ACTIVE);
}

static void s5e4270_send_cp_wakeup()
{
	mbox_set_interrupt(MCU_CPWAKE_INT);
}

static void ap_wake_lock(u16 cmd, void *data)
{
	pr_info("[Modem IF] CP2AP wake interrupt is occured.\n");
	struct modem_ctl *mc = (struct modem_ctl *)data;

	if (!wake_lock_active(&mc->mc_wake_lock)) {
		wake_lock(&mc->mc_wake_lock);
	} else {
		wake_unlock(&mc->mc_wake_lock);
	}
}

static void s5e4270_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = s5e4270_on;
	mc->ops.modem_off = s5e4270_off;
	mc->ops.modem_reset = s5e4270_reset;
	mc->ops.modem_boot_on = s5e4270_boot_on;
	mc->ops.modem_boot_off = s5e4270_boot_off;
	mc->ops.modem_boot_done = s5e4270_boot_done;
	mc->ops.modem_force_crash_exit = s5e4270_force_crash_exit;
	mc->ops.modem_dump_reset = s5e4270_dump_reset;

	mc->ops.send_cp_wakeup = s5e4270_send_cp_wakeup;
	mc->ops.set_ap2cp_status = s5e4270_set_ap_status;
	mc->ops.clear_ap2cp_status = s5e4270_clear_ap_status;
	mc->ops.get_cp2ap_status = s5e4270_get_cp_status;
	mc->ops.set_pda_active = s5e4270_set_pda_active;
	mc->ops.clear_pda_active = s5e4270_clear_pda_active;
	mc->ops.get_phone_active = s5e4270_get_phone_active;
}

int init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret = 0;
	int irq = 0;
	unsigned long flag = 0;
	struct platform_device *pdev = NULL;
	printk("[Modem IF] Initialization Modem Control for s5e4270!!!!\n");
	mif_err("+++\n");

	mc->mb_phone_active = pdata->mb_phone_active;
	mc->mb_pda_active = pdata->mb_pda_active;
	mc->mb_ap2cp_wakeup = pdata->mb_ap2cp_wakeup;
	mc->mb_cp2ap_wakeup = pdata->mb_cp2ap_wakeup;
	mc->mb_ap2cp_status = pdata->mb_ap2cp_status;
	mc->mb_cp2ap_status = pdata->mb_cp2ap_status;
	mc->cp_onoff = pdata->cp_on;
	mc->cp_reset = pdata->cp_reset;

	s5e4270_get_ops(mc);
	dev_set_drvdata(mc->dev, mc);

	wake_lock_init(&mc->mc_wake_lock, WAKE_LOCK_SUSPEND, "umts_wake_lock");

	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = platform_get_irq(pdev, 0);
	mc->irq_cp_wdt = platform_get_irq(pdev, 1);
	pr_info("[Modem IF] Request irq : PHONE_ACTIVE(%d), CP_WDT(%d)\n",
			mc->irq_phone_active, mc->irq_cp_wdt);

	ret = request_irq(mc->irq_phone_active, phone_active_handler,
			0, "phone_active", mc);
	if (ret) {
		mif_err("Request irq fail - phone_active(%d)\n", ret);
		return ret;
	}
	/*
	ret = enable_irq_wake(mc->irq_phone_active);
	if (ret)
		mif_err("enable_irq_wake fail (%d)\n", ret);
	*/

	ret = request_irq(mc->irq_cp_wdt, cp_wdt_handler, 0, "cp_wdt", mc);
	if (ret) {
		mif_err("request_irq fail - cp_wdt (%d)\n", ret);
		return ret;
	}
	/*
	ret = enable_irq_wake(mc->irq_cp_wdt);
	if (ret)
		mif_err("irq_cp_wdt fail (%d)\n", ret);
	*/

	pr_info("[Modem IF] Register Mailbox interrupt for CP2AP wakeupl!!!\n");
	mbox_request_irq(MCU_APWAKE_INT, ap_wake_lock, (void*)mc);

	pr_info("[Modem IF] Register Mailbox interrupt for CP2AP status!!!\n");
	mbox_request_irq(MCU_CP2AP_STAT, cp2ap_status_handler, (void*)mc);

	pr_info("[Modem IF] Register Mailbox interrupt for CP2AP status!!!\n");
	mbox_request_irq(MCU_CP2AP_DVFS_REQ, cp2ap_dvfsreq_handler, (void*)mc);

	s5e4270_set_ap_status(mc);
	s5e4270_set_pda_active(mc);

	ret = device_create_file(mc->dev, &dev_attr_modem_ctrl);
	if (ret)
		mif_err("can't create modem_ctrl!!!\n");

	mif_err("---\n");

	return 0;
}

