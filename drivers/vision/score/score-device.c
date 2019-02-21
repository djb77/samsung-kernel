/*
 * Samsung Exynos SoC series SCORE driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/exynos_iovmm.h>
#include <linux/pm_runtime.h>

#include "score-config.h"
#include "score-device.h"
#include "score-regs.h"
#include "score-regs-user-def.h"
#include "score-debug.h"
#include "score-debug-print.h"
#include "score-dump.h"
#include "score-profiler.h"

static int __score_device_start(struct score_device *device)
{
	int ret = 0;

	if (!test_bit(SCORE_DEVICE_STATE_OPEN, &device->state)) {
		ret = -EINVAL;
		goto p_err;
	}

	ret = score_system_start(&device->system);
	if (ret)
		goto p_err;

p_err:
	return ret;
}

static int __score_device_stop(struct score_device *device)
{
	int ret = 0;

	ret = score_system_stop(&device->system);
	if (ret)
		goto p_err;

p_err:
	return ret;
}

int score_device_start(struct score_device *device)
{
	return __score_device_start(device);
}

int score_device_stop(struct score_device *device)
{
	return __score_device_stop(device);
}

static int score_device_suspend(struct device *dev)
{
	int ret = 0;
	struct score_device *device;
	struct score_system *system;

	device = dev_get_drvdata(dev);
	system = &device->system;

	if (score_system_active(system)) {
		score_fw_queue_block(&device->fw_dev);
		score_system_suspend(system);
	}

	return ret;
}

static int score_device_resume(struct device *dev)
{
	int ret = 0;
	struct score_device *device;
	struct score_system *system;

	device = dev_get_drvdata(dev);
	system = &device->system;

	if (score_system_active(system)) {
		score_fw_queue_unblock(&device->fw_dev);
		score_system_resume(system);
	}

	return ret;
}

static int score_device_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct score_device *device;

	device = dev_get_drvdata(dev);
	score_system_runtime_suspend(&device->system);

	return ret;
}

static int score_device_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct score_device *device;

	device = dev_get_drvdata(dev);
	ret = score_system_runtime_resume(&device->system);

	return ret;
}

static const struct dev_pm_ops score_pm_ops = {
	.suspend		= score_device_suspend,
	.resume			= score_device_resume,
	.runtime_suspend	= score_device_runtime_suspend,
	.runtime_resume		= score_device_runtime_resume,
};

static int __score_device_power_on(struct score_device *device)
{
	int ret = 0;

#if defined(CONFIG_PM)
	ret = pm_runtime_get_sync(device->dev);
#else
	ret = score_device_runtime_resume(device->dev);
#endif
	if (ret) {
		score_err("runtime_resume is fail(%d)", ret);
	}
	return ret;
}

static void __score_device_power_off(struct score_device *device)
{
#if defined(CONFIG_PM)
	pm_runtime_put_sync(device->dev);
#else
	score_device_runtime_suspend(device->dev);
#endif
}

int score_device_open(struct score_device *device)
{
	int ret = 0;
	struct score_system *system = &device->system;

	if (!test_bit(SCORE_DEVICE_STATE_CLOSE, &device->state)) {
		return -EBUSY;
	}

	ret = score_system_open(system);
	if (ret)
		goto p_err_system;

	ret = __score_device_power_on(device);
	if (ret)
		goto p_err_power;

	ret = score_vertexmgr_open(&device->vertexmgr);
	if (ret)
		goto p_err_vertexmgr;

	score_print_open(system);
	score_debug_open(system);
	score_fw_queue_init(&device->fw_dev);

	ret = score_system_fw_start(system);
	if (ret)
		goto p_err_fw;

#ifdef ENABLE_TIME_STAMP
	score_time_stamp_init(system);
#endif

	device->state = BIT(SCORE_DEVICE_STATE_OPEN);

	return ret;
p_err_fw:
	score_debug_close();
	score_print_close(system);
	score_vertexmgr_close(&device->vertexmgr);
p_err_vertexmgr:
	__score_device_power_off(device);
p_err_power:
	score_system_close(system);
p_err_system:
	return ret;
}

int score_device_close(struct score_device *device)
{
	if (test_bit(SCORE_DEVICE_STATE_SUSPEND, &device->state)) {
		return 0;
	} else if (!test_bit(SCORE_DEVICE_STATE_OPEN, &device->state)) {
		return -ENODEV;
	}

#ifdef ENABLE_TIME_STAMP
	del_timer_sync(&device->system.score_ts.timer);
#endif
	score_debug_close();
	score_print_close(&device->system);

	score_vertexmgr_close(&device->vertexmgr);
	__score_device_power_off(device);
	score_system_close(&device->system);

	device->state = BIT(SCORE_DEVICE_STATE_CLOSE);

	return 0;
}

static int __attribute__((unused)) score_fault_handler(struct iommu_domain *domain,
		struct device *dev, unsigned long fault_addr,
		int fault_flag, void *token)
{
	struct score_device *device;
	struct score_system *system;

	pr_err("<SCORE FAULT HANDLER>\n");
	pr_err("Device virtual(0x%X) is invalid access\n", (u32)fault_addr);

	device = dev_get_drvdata(dev);
	system = &device->system;

	if (score_system_active(system)) {
		/* for debug (change OFFSET, SIZE if you want) */
		/* pr_err("sfr %X \n", readl(system->regs + OFFSET)); */
		/* print_hex_dump(KERN_INFO, "PREFIX LOG", DUMP_PREFIX_OFFSET,
		   32, 4, system->regs + OFFSET, SIZE, false); */

		score_dump_sfr(system->regs);
		score_print_flush(system);
	}

	return -EINVAL;
}

#ifdef ENABLE_SYSFS_STATE
/* sysfs global variable for system */
struct score_sysfs_state sysfs_state;

static DEVICE_ATTR(state_val,
		0644,
		show_sysfs_state_val,
		store_sysfs_state_val);
static DEVICE_ATTR(state_en,
		0644,
		show_sysfs_state_en,
		store_sysfs_state_en);
#endif
#ifdef ENABLE_SYSFS_SYSTEM
/* sysfs global variable for system */
struct score_sysfs_system sysfs_system;

static DEVICE_ATTR(clk_gate_en,
		0644,
		show_sysfs_system_clk_gate_en,
		store_sysfs_system_clk_gate_en);
static DEVICE_ATTR(dvfs_en,
		0644,
		show_sysfs_system_dvfs_en,
		store_sysfs_system_dvfs_en);
static DEVICE_ATTR(clk,
		0644,
		show_sysfs_system_clk,
		store_sysfs_system_clk);
static DEVICE_ATTR(sfr,
		0644,
		show_sysfs_system_sfr,
		store_sysfs_system_sfr);
#endif

#ifdef ENABLE_SYSFS_DEBUG
struct score_sysfs_debug score_sysfs_debug;

static DEVICE_ATTR(debug_count,
		0644,
		show_sysfs_debug_count,
		store_sysfs_debug_count);
#endif

#if (defined(ENABLE_SYSFS_SYSTEM) || defined(ENABLE_SYSFS_STATE))
static struct attribute *score_system_entries[] = {
#ifdef ENABLE_SYSFS_SYSTEM
	&dev_attr_clk_gate_en.attr,
	&dev_attr_dvfs_en.attr,
	&dev_attr_clk.attr,
	&dev_attr_sfr.attr,
#endif
#ifdef ENABLE_SYSFS_STATE
	&dev_attr_state_val.attr,
	&dev_attr_state_en.attr,
#endif
#ifdef ENABLE_SYSFS_DEBUG
	&dev_attr_debug_count.attr,
#endif
	NULL,
};

static struct attribute_group score_system_attr_group = {
	.name   = "system",
	.attrs  = score_system_entries,
};

static struct bin_attribute score_dump_bin_attr = {
	.attr = {
		.name = "score_dump_bin",
		.mode = 0644,
	},
	.size = DUMP_BUF_SIZE,
	.mmap = mmap_sysfs_dump,
};

static struct bin_attribute score_profile_bin_attr = {
	.attr = {
		.name = "score_profile_bin",
		.mode = 0644,
	},
	.size = PROFILE_BUF_SIZE,
	.mmap = mmap_sysfs_profile,
};
#endif

static int score_device_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct score_device *device;

	device = devm_kzalloc(&pdev->dev, sizeof(struct score_device), GFP_KERNEL);
	if (!device) {
		probe_err("devm_kzalloc is failed\n");
		ret = -ENOMEM;
		goto p_exit;
	}

	device->pdev = pdev;
	device->dev = &pdev->dev;
	dev_set_drvdata(device->dev, device);

	ret = score_system_probe(&device->system, pdev);
	if (ret)
		goto p_err_system;

	ret = score_vertex_probe(&device->vertex, device->dev);
	if (ret)
		goto p_err_vertex;

	ret = score_vertexmgr_probe(&device->vertexmgr);
	if (ret)
		goto p_err_vertexmgr;

	score_debug_probe();

	ret = score_fw_queue_probe(&device->fw_dev);
	if (ret)
		goto p_err_fw_queue;

	iovmm_set_fault_handler(device->dev, score_fault_handler, NULL);

#if defined(CONFIG_PM)
	pm_runtime_enable(device->dev);
#endif
#ifdef ENABLE_SYSFS_SYSTEM
	/* set sysfs for system */
	sysfs_system.en_clk_gate = 0;
	sysfs_system.en_dvfs = 1;
#endif
#ifdef ENABLE_SYSFS_STATE
	sysfs_state.is_en = false;
	sysfs_state.frame_duration = 0;
	sysfs_state.long_time = 3;
	sysfs_state.short_time = 0;
	sysfs_state.long_v_rank = 9;
	sysfs_state.short_v_rank = 0;
	sysfs_state.long_r_rank = 4;
	sysfs_state.short_r_rank = 1;
#endif
#ifdef ENABLE_SYSFS_DEBUG
	memset(&score_sysfs_debug, 0x0, sizeof(struct score_sysfs_debug));
#endif
#if (defined(ENABLE_SYSFS_SYSTEM) || defined(ENABLE_SYSFS_STATE))
	ret = sysfs_create_group(&device->pdev->dev.kobj, &score_system_attr_group);
	if (ret) {
		probe_err("score_system_addr_group is no created (%d)\n", ret);
		goto p_err_sysfs;
	}

	ret = device_create_bin_file(&device->pdev->dev, &score_dump_bin_attr);
	if (ret < 0)
		probe_err("score_dump_bin_addr is not created (%d)\n", ret);

	ret = device_create_bin_file(&device->pdev->dev, &score_profile_bin_attr);
	if (ret < 0)
		probe_err("score_profile_bin_addr is not created (%d)\n", ret);
#endif

	device->state = BIT(SCORE_DEVICE_STATE_CLOSE);

	return 0;
p_err_sysfs:
#if defined(CONFIG_PM)
	pm_runtime_disable(device->dev);
#endif
	score_fw_queue_release(&device->fw_dev);
p_err_fw_queue:
	score_vertexmgr_remove(&device->vertexmgr);
p_err_vertexmgr:
	score_vertex_release(&device->vertex);
p_err_vertex:
	score_system_release(&device->system);
p_err_system:
	devm_kfree(&pdev->dev, device);
p_exit:
	return ret;
}

static int score_device_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;
	struct score_device *device;

	dev = &pdev->dev;
	device = dev_get_drvdata(dev);

#if defined(CONFIG_PM)
	pm_runtime_disable(dev);
#endif
	score_fw_queue_release(&device->fw_dev);

	score_vertexmgr_remove(&device->vertexmgr);
	score_vertex_release(&device->vertex);
	score_system_release(&device->system);

	devm_kfree(dev, device);

	return ret;
}

static void score_device_shutdown(struct platform_device *pdev)
{
	struct device *dev;
	struct score_device *device;

	struct score_system *system;
	struct score_framemgr *iframemgr;
	unsigned long flags;

	dev = &pdev->dev;
	device = dev_get_drvdata(dev);
	system = &device->system;

	device->state = BIT(SCORE_DEVICE_STATE_SUSPEND);
	if (score_system_active(system)) {
		iframemgr = &system->interface.framemgr;

		spin_lock_irqsave(&iframemgr->slock, flags);
		score_system_reset(system, RESET_OPT_FORCE | RESET_SFR_CLEAN);
		spin_unlock_irqrestore(&iframemgr->slock, flags);
	}
}

static const struct of_device_id exynos_score_match[] = {
	{
		.compatible = "samsung,score",
	},
	{}
};
MODULE_DEVICE_TABLE(of, exynos_score_match);

static struct platform_driver score_driver = {
	.probe		= score_device_probe,
	.remove		= score_device_remove,
	.shutdown	= score_device_shutdown,
	.driver = {
		.name	= "exynos-score",
		.owner	= THIS_MODULE,
		.pm	= &score_pm_ops,
		.of_match_table = of_match_ptr(exynos_score_match)
	}
};

static int __init score_device_init(void)
{
	int ret = platform_driver_register(&score_driver);
	if (ret)
		probe_err("platform_driver_register is fail():%d\n", ret);

	return ret;
}
late_initcall(score_device_init);

static void __exit score_device_exit(void)
{
	platform_driver_unregister(&score_driver);
}
module_exit(score_device_exit);

MODULE_AUTHOR("Pilsun Jang<pilsun.jang@samsung.com>");
MODULE_DESCRIPTION("Exynos SCORE driver");
MODULE_LICENSE("GPL");
