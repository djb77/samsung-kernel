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
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/mcu_ipc.h>
#include <linux/smc.h>
#include <linux/regulator/consumer.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "pmu-cp.h"

#ifdef CONFIG_EXYNOS_BUSMONITOR
#include <linux/exynos-busmon.h>
#endif

#define MIF_INIT_TIMEOUT	(15 * HZ)

#ifdef CONFIG_REGULATOR_S2MPU01A
#include <linux/mfd/samsung/s2mpu01a.h>
static inline int change_cp_pmu_manual_reset(void)
{
	return change_mr_reset();
}
#else
static inline int change_cp_pmu_manual_reset(void) {return 0; }
#endif

static struct modem_ctl *g_mc;
extern void exynos_pcie_poweron(int);
extern void exynos_pcie_poweroff(int);
static int ap_wakeup, phone_active, cp_boot_noti;

#if 0
static irqreturn_t cp_wdt_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct io_device *iod;
	enum modem_state new_state;

	mif_disable_irq(&mc->irq_cp_wdt);
	mif_err("%s: ERR! CP_WDOG occurred\n", mc->name);

	/* Disable debug Snapshot */
	mif_set_snapshot(false);

	exynos_clear_cp_reset(mc);
	new_state = STATE_CRASH_WATCHDOG;

	mif_err("new_state = %s\n", cp_state_str(new_state));

	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, new_state);
	}

	return IRQ_HANDLED;
}

static irqreturn_t cp_fail_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct io_device *iod;
	enum modem_state new_state;

	mif_disable_irq(&mc->irq_cp_fail);
	mif_err("%s: ERR! CP_FAIL occurred\n", mc->name);

	exynos_cp_active_clear(mc);
	new_state = STATE_CRASH_RESET;

	mif_err("new_state = %s\n", cp_state_str(new_state));

	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, new_state);
	}

	return IRQ_HANDLED;
}

static void cp_active_handler(void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct io_device *iod;
	//int cp_on = exynos_get_cp_power_status(mc);
	enum modem_state old_state = mc->phone_state;
	enum modem_state new_state = mc->phone_state;

	if (old_state != new_state) {
		mif_err("new_state = %s\n", cp_state_str(new_state));

		/* Disable debug Snapshot */
		mif_set_snapshot(false);

		list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
			if (iod && atomic_read(&iod->opened) > 0)
				iod->modem_state_changed(iod, new_state);
		}
	}
}
#endif

#if 0
static int get_system_rev(struct device_node *np)
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

#ifdef CONFIG_GPIO_DS_DETECT
static int get_ds_detect(struct device_node *np)
{
	unsigned gpio_ds_det;

	gpio_ds_det = of_get_named_gpio(np, "mif,gpio_ds_det", 0);
	if (!gpio_is_valid(gpio_ds_det)) {
		mif_err("gpio_ds_det: Invalid gpio\n");
		return 0;
	}

	return gpio_get_value(gpio_ds_det);
}
#else
static int ds_detect = 1;
module_param(ds_detect, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(ds_detect, "Dual SIM detect");

static int get_ds_detect(struct device_node *np)
{
	mif_info("Dual SIM detect = %d\n", ds_detect);
	return ds_detect - 1;
}
#endif
#endif

static int ss5g_on(struct modem_ctl *mc)
{
	//int ret;

	mif_err("+++\n");

#if 0
	/* Enable debug Snapshot */
	mif_set_snapshot(true);

	mc->phone_state = STATE_OFFLINE;

	if (exynos_get_cp_power_status(mc) > 0) {
		mif_err("CP aleady Power on, Just start!\n");
		exynos_cp_release(mc);
	} else {
		exynos_set_cp_power_onoff(mc, CP_POWER_ON);
	}

	msleep(300);
	ret = change_cp_pmu_manual_reset();
	mif_err("change_mr_reset -> %d\n", ret);
#endif

	mif_err("---\n");
	return 0;
}

static int ss5g_off(struct modem_ctl *mc)
{
	mif_err("+++\n");

//	exynos_set_cp_power_onoff(mc, CP_POWER_OFF);

	mif_err("---\n");
	return 0;
}

static int ss5g_shutdown(struct modem_ctl *mc)
{
#if 0
	struct io_device *iod;
	unsigned long timeout = msecs_to_jiffies(3000);
	unsigned long remain;

	mif_err("+++\n");

	if (mc->phone_state == STATE_OFFLINE
		|| exynos_get_cp_power_status(mc) <= 0)
		goto exit;

	init_completion(&mc->off_cmpl);
	remain = wait_for_completion_timeout(&mc->off_cmpl, timeout);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		mc->phone_state = STATE_OFFLINE;
		list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
			if (iod && atomic_read(&iod->opened) > 0)
				iod->modem_state_changed(iod, STATE_OFFLINE);
		}
	}

exit:
	exynos_set_cp_power_onoff(mc, CP_POWER_OFF);
	mif_err("---\n");
#endif
	return 0;
}

static int ss5g_reset(struct modem_ctl *mc)
{
	struct regulator *regulator_ldo1;
	struct regulator *regulator_ldo2;
	struct regulator *regulator_ldo3;
	struct regulator *regulator_ldo4;
	struct regulator *regulator_ldo5;
	struct regulator *regulator_ldo11;
	struct regulator *regulator_ldo12;
	struct regulator *regulator_ldo13;
	struct regulator *regulator_ldo14;
	struct regulator *regulator_ldo15;
	struct regulator *regulator_ldo16;
	struct regulator *regulator_ldo17;
	struct regulator *regulator_sw2;
	int ret;

	regulator_ldo1 = regulator_get(NULL, mc->regulator_ldo1);
	if (IS_ERR(regulator_ldo1)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo1);
		return PTR_ERR(regulator_ldo1);
	}
	regulator_ldo2 = regulator_get(NULL, mc->regulator_ldo2);
	if (IS_ERR(regulator_ldo2)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo2);
		return PTR_ERR(regulator_ldo2);
	}
	regulator_ldo3 = regulator_get(NULL, mc->regulator_ldo3);
	if (IS_ERR(regulator_ldo3)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo3);
		return PTR_ERR(regulator_ldo3);
	}
	regulator_ldo4 = regulator_get(NULL, mc->regulator_ldo4);
	if (IS_ERR(regulator_ldo4)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo4);
		return PTR_ERR(regulator_ldo4);
	}
	regulator_ldo5 = regulator_get(NULL, mc->regulator_ldo5);
	if (IS_ERR(regulator_ldo5)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo5);
		return PTR_ERR(regulator_ldo5);
	}
	regulator_ldo11 = regulator_get(NULL, mc->regulator_ldo11);
	if (IS_ERR(regulator_ldo11)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo11);
		return PTR_ERR(regulator_ldo11);
	}
	regulator_ldo12 = regulator_get(NULL, mc->regulator_ldo12);
	if (IS_ERR(regulator_ldo12)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo12);
		return PTR_ERR(regulator_ldo12);
	}
	regulator_ldo13 = regulator_get(NULL, mc->regulator_ldo13);
	if (IS_ERR(regulator_ldo13)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo13);
		return PTR_ERR(regulator_ldo13);
	}
	regulator_ldo14 = regulator_get(NULL, mc->regulator_ldo14);
	if (IS_ERR(regulator_ldo14)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo14);
		return PTR_ERR(regulator_ldo14);
	}
	regulator_ldo15 = regulator_get(NULL, mc->regulator_ldo15);
	if (IS_ERR(regulator_ldo15)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo15);
		return PTR_ERR(regulator_ldo15);
	}
	regulator_ldo16 = regulator_get(NULL, mc->regulator_ldo16);
	if (IS_ERR(regulator_ldo16)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo16);
		return PTR_ERR(regulator_ldo16);
	}
	regulator_ldo17 = regulator_get(NULL, mc->regulator_ldo17);
	if (IS_ERR(regulator_ldo17)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_ldo17);
		return PTR_ERR(regulator_ldo17);
	}
	regulator_sw2 = regulator_get(NULL, mc->regulator_sw2);
	if (IS_ERR(regulator_sw2)) {
		mif_err("Failed to get %s regulator.\n", mc->regulator_sw2);
		return PTR_ERR(regulator_sw2);
	}
	mif_info("got regulator\n");

	ret = regulator_enable(regulator_ldo11);
	if (ret) {
		mif_err("Failed to enable ldo11: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(regulator_ldo14);
	if (ret) {
		mif_err("Failed to enable ldo14: %d\n", ret);
		return ret;
	}
	usleep_range(500, 1000);

	gpio_set_value(mc->gpio_buck_en1, 1);
	mif_info("buck_en1: %d\n", gpio_get_value(mc->gpio_buck_en1));
	ret = regulator_enable(regulator_ldo4);
	if (ret) {
		mif_err("Failed to enable ldo4: %d\n", ret);
		return ret;
	}
	usleep_range(200, 500);

	ret = regulator_enable(regulator_ldo1);
	if (ret) {
		mif_err("Failed to enable ldo1: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(regulator_ldo2);
	if (ret) {
		mif_err("Failed to enable ldo2: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(regulator_ldo3);
	if (ret) {
		mif_err("Failed to enable ldo3: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(regulator_sw2);
	if (ret) {
		mif_err("Failed to enable sw2: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(regulator_ldo5);
	if (ret) {
		mif_err("Failed to enable ldo5: %d\n", ret);
		return ret;
	}
	usleep_range(200, 500);

	ret = regulator_enable(regulator_ldo15);
	if (ret) {
		mif_err("Failed to enable ldo15: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(regulator_ldo13);
	if (ret) {
		mif_err("Failed to enable ldo13: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(regulator_ldo16);
	if (ret) {
		mif_err("Failed to enable ldo16: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(regulator_ldo17);
	if (ret) {
		mif_err("Failed to enable ldo17: %d\n", ret);
		return ret;
	}
	ret = regulator_enable(regulator_ldo12);
	if (ret) {
		mif_err("Failed to enable ldo12: %d\n", ret);
		return ret;
	}
	usleep_range(200, 500);

	gpio_set_value(mc->gpio_cp_reset, 1);
	msleep(50);
	gpio_set_value(mc->gpio_cp_reset, 0);
	msleep(50);
	gpio_set_value(mc->gpio_cp_reset, 1);

	return 0;
}

static int ss5g_boot_on(struct modem_ctl *mc)
{
#if 0
	struct link_device *ld = get_current_link(mc->iod);
	struct io_device *iod;

	mif_err("+++\n");

	if (ld->boot_on)
		ld->boot_on(ld, mc->bootd);

	init_completion(&mc->init_cmpl);
	init_completion(&mc->off_cmpl);

	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, STATE_BOOTING);
	}

	mif_err("---\n");
#endif

#define MODEM_PCIE_CH_NUM	(1)
	exynos_pcie_poweron(MODEM_PCIE_CH_NUM);

	return 0;
}

static int ss5g_boot_off(struct modem_ctl *mc)
{
#if 0
	struct io_device *iod;
	unsigned long remain;
	int err = 0;
	mif_info("+++\n");

	remain = wait_for_completion_timeout(&mc->init_cmpl, MIF_INIT_TIMEOUT);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		err = -EAGAIN;
		goto exit;
	}

	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, STATE_ONLINE);
	}

	mif_info("---\n");

exit:
	return err;
#else
	return 0;
#endif
}

static int ss5g_boot_done(struct modem_ctl *mc)
{
	mif_info("+++\n");
	mif_info("---\n");
	return 0;
}

static int ss5g_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_err("+++\n");

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	mif_err("---\n");
	return 0;
}

int ss5g_force_crash_exit_ext(void)
{
	if (g_mc)
		ss5g_force_crash_exit(g_mc);

	return 0;
}

static int ss5g_dump_start(struct modem_ctl *mc)
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

	exynos_cp_release(mc);

	mif_err("---\n");
	return err;
}

static void ss5g_modem_boot_confirm(struct modem_ctl *mc)
{
}

static void ss5g_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = ss5g_on;
	mc->ops.modem_off = ss5g_off;
	mc->ops.modem_shutdown = ss5g_shutdown;
	mc->ops.modem_reset = ss5g_reset;
	mc->ops.modem_boot_on = ss5g_boot_on;
	mc->ops.modem_boot_off = ss5g_boot_off;
	mc->ops.modem_boot_done = ss5g_boot_done;
	mc->ops.modem_force_crash_exit = ss5g_force_crash_exit;
	mc->ops.modem_dump_start = ss5g_dump_start;
	mc->ops.modem_boot_confirm = ss5g_modem_boot_confirm;
}

#ifdef CONFIG_EXYNOS_BUSMONITOR
static int ss5g_busmon_notifier(struct notifier_block *nb,
						unsigned long event, void *data)
{
	struct busmon_notifier *info = (struct busmon_notifier *)data;
	char *init_desc = info->init_desc;

	if (init_desc != NULL &&
		(strncmp(init_desc, "CP", strlen(init_desc)) == 0 ||
		strncmp(init_desc, "APB_CORE_CP", strlen(init_desc)) == 0 ||
		strncmp(init_desc, "MIF_CP", strlen(init_desc)) == 0)) {
		struct modem_ctl *mc =
			container_of(nb, struct modem_ctl, busmon_nfb);

		ss5g_force_crash_exit(mc);
	}
	return 0;
}
#endif

static void handle_wake_work(struct work_struct *work)
{
	struct modem_ctl *mc = container_of(work, struct modem_ctl, dwork.work);
	struct link_device *ld;

	ld = get_current_link(mc->iod);
	if (ld)
		ld->init_descriptor(ld);
	else
		mif_err("there is no ld\n");
}

static void handle_boot_noti_work(struct work_struct *work)
{
	struct modem_ctl *mc = container_of(work, struct modem_ctl, work);

	complete_all(&mc->boot_noti);
	mif_info("[5GDBG] complete boot noti\n");
}

static irqreturn_t handle_wake_irq(int irq, void *data)
{
	struct modem_ctl *mc = data;

	mif_info("[5GDBG] %s ---->, gpio_get_value(%d)\n", __func__, gpio_get_value(ap_wakeup));
	if (gpio_get_value(ap_wakeup))
		schedule_delayed_work(&mc->dwork, 0);

	return IRQ_HANDLED;
}

static irqreturn_t handle_active_irq(int irq, void *data)
{
	mif_info("[5GDBG] %s ---->, gpio_get_value(%d)\n", __func__, gpio_get_value(phone_active));
	mif_info("[5GDBG] %s <----, gpio_get_value(%d)\n", __func__, gpio_get_value(phone_active));
	return IRQ_HANDLED;
}

static irqreturn_t handle_cp_boot_noti_irq(int irq, void *data)
{
	struct modem_ctl *mc = data;

	mif_info("[5GDBG] %s <---- , gpio_get_value(%d)\n", __func__, gpio_get_value(cp_boot_noti));

	if (gpio_get_value(cp_boot_noti))
		schedule_work(&mc->work);

	return IRQ_HANDLED;
}

/* TEMP:
 * Get pdata to modemctl device.
 * request_irq and others except just move pdata to mc
 * are going to be moved to init_modemctl_device later
 */
static int ss5g_get_pdata(struct modem_ctl *mc, struct modem_data *pdata)
{
	struct device *dev = mc->dev;
	struct device_node *np;
	int ret;

	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_reset = pdata->gpio_cp_reset;

	mc->regulator_ldo1 = pdata->regulator_ldo1;
	mc->regulator_ldo2 = pdata->regulator_ldo2;
	mc->regulator_ldo3 = pdata->regulator_ldo3;
	mc->regulator_ldo4 = pdata->regulator_ldo4;
	mc->regulator_ldo5 = pdata->regulator_ldo5;
	mc->regulator_ldo11 = pdata->regulator_ldo11;
	mc->regulator_ldo12 = pdata->regulator_ldo12;
	mc->regulator_ldo13 = pdata->regulator_ldo13;
	mc->regulator_ldo14 = pdata->regulator_ldo14;
	mc->regulator_ldo15 = pdata->regulator_ldo15;
	mc->regulator_ldo16 = pdata->regulator_ldo16;
	mc->regulator_ldo17 = pdata->regulator_ldo17;
	mc->regulator_sw2 = pdata->regulator_sw2;

	mc->gpio_buck_en1 = pdata->gpio_buck_en1;

	/* TEMP:
	 * request_irq and others except just move pdata to mc
	 * are going to be moved to init_modemctl_device later
	 */
	np = dev->of_node;
	if (!np) {
		mif_err("DT, failed to get node\n");
		return -EINVAL;
	}

	/* 5G_AP_WAKEUP irq */
	ap_wakeup = of_get_named_gpio(np, "mif,5g_ap_wakeup", 0);
	if (gpio_is_valid(ap_wakeup)) {
		mif_err("[5GDBG] ap_wakeup %d\n", gpio_get_value(ap_wakeup));
		mc->irq_ap_wakeup = gpio_to_irq(ap_wakeup);
		ret = request_irq(mc->irq_ap_wakeup, handle_wake_irq,
				IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"5g_ap_wakeup", mc);
		if (ret) {
			mif_err("failed to request_irq:%d\n", ret);
			return ret;
		}
	}
	ret = enable_irq_wake(mc->irq_ap_wakeup);
	if (ret) {
		mif_err("failed to enable_irq_wake:%d\n", ret);
		/* Temporary skip
		 * return ret;
		 */
	}

	/* 5G_ACTIVE irq */
	mc->irq_phone_active = gpio_to_irq(mc->gpio_phone_active);
	ret = request_irq(mc->irq_phone_active, handle_active_irq,
			IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING,
			"irq_phone_active", mc);
	if (ret) {
		mif_err("failed to request_irq:%d\n", ret);
		return ret;
	}

	ret = enable_irq_wake(mc->irq_phone_active);
	if (ret) {
		mif_err("failed to enable_irq_wake:%d\n", ret);
		return ret;
	}

#ifdef CONFIG_LINK_DEVICE_PCI
	cp_boot_noti = of_get_named_gpio(np, "mif,gpio_cp_boot_noti", 0);
	if (!gpio_is_valid(cp_boot_noti)) {
		mif_err("gpio_cp_boot_noti: Invalied gpio pins\n");
		return -EINVAL;
	}

	ret = gpio_request(cp_boot_noti, "CP_BOOT_NOTI");
	if (ret)
		mif_err("fail to request gpio %s:%d\n", "CP_BOOT_NOTI", ret);

	mc->irq_cp_boot_noti = gpio_to_irq(cp_boot_noti);
	ret = request_irq(mc->irq_cp_boot_noti, handle_cp_boot_noti_irq,
			IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"5g_cp_boot_noti", mc);
	if (ret) {
		mif_err("failed to request_irq:%d\n", ret);
		return ret;
	}

	board_gpio_export(mc->dev, cp_boot_noti,
			false, "gpio_cp_boot_noti");
#endif

	board_gpio_export(mc->dev, pdata->gpio_phone_active,
			false, "gpio_phone_active");

	return ret;

}

int ss5g_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret;

	g_mc = mc;

	ss5g_get_ops(mc);
	ret = ss5g_get_pdata(mc, pdata);
	if (ret) {
		mif_err("[5GDBG] %s[%d] ret = %d\n", __func__, __LINE__, ret);
	}
	dev_set_drvdata(mc->dev, mc);

	init_completion(&mc->init_cmpl);
	init_completion(&mc->boot_noti);
	init_completion(&mc->off_cmpl);
	INIT_WORK(&mc->work, handle_boot_noti_work);
	INIT_DELAYED_WORK(&mc->dwork, handle_wake_work);

#ifdef CONFIG_EXYNOS_BUSMONITOR
	/*
	 ** Register BUS Mon notifier
	 */
	mc->busmon_nfb.notifier_call = ss5g_busmon_notifier;
	busmon_notifier_chain_register(&mc->busmon_nfb);
#endif

	mc->phone_state = STATE_OFFLINE;
	return 0;
}
