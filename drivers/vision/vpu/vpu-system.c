/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm_qos.h>

#include "vpu-config.h"
#include "vpu-device.h"
#include "vpu-system.h"
#include "vpu-interface.h"

#define CAM_L0 690000
#define CAM_L1 680000
#define CAM_L2 670000
#define CAM_L3 660000
#define CAM_L4 650000
#define CAM_L5 640000

#define MIF_L0 2093000
#define MIF_L1 2002000
#define MIF_L2 1794000
#define MIF_L3 1539000
#define MIF_L4 1352000
#define MIF_L5 1014000
#define MIF_L6 845000
#define MIF_L7 676000

struct pm_qos_request exynos_vpu_qos_cam;
struct pm_qos_request exynos_vpu_qos_mem;

int vpu_system_probe(struct vpu_system *system, struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;
	struct resource *res;
	void __iomem *iomem;
	int irq;

	BUG_ON(!system);
	BUG_ON(!pdev);

	dev = &pdev->dev;
	system->cam_qos = 0;
	system->mif_qos = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		probe_err("platform_get_resource(0) is fail(%p)", res);
		ret = PTR_ERR(res);
		goto p_err;
	}

	iomem =  devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(iomem)) {
		probe_err("devm_ioremap_resource(0) is fail(%p)", iomem);
		ret = PTR_ERR(iomem);
		goto p_err;
	}

	system->ram0 = iomem;
	system->ram0_size = resource_size(res);
	probe_info("resource0 : %p\n", iomem);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		probe_err("platform_get_resource(1) is fail(%p)", res);
		ret = PTR_ERR(res);
		goto p_err;
	}

	iomem =  devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(iomem)) {
		probe_err("devm_ioremap_resource(1) is fail(%p)", iomem);
		ret = PTR_ERR(iomem);
		goto p_err;
	}

	system->ram1 = iomem;
	system->ram1_size = resource_size(res);
	probe_info("resource1 : %p\n", iomem);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!res) {
		probe_err("platform_get_resource(2) is fail(%p)", res);
		ret = PTR_ERR(res);
		goto p_err;
	}

	iomem =  devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(iomem)) {
		probe_err("devm_ioremap_resource(2) is fail(%p)", iomem);
		ret = PTR_ERR(iomem);
		goto p_err;
	}

	system->code = iomem;
	system->code_size = resource_size(res);
	probe_info("resource2 : %p\n", iomem);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (!res) {
		probe_err("platform_get_resource(3) is fail(%p)", res);
		ret = PTR_ERR(res);
		goto p_err;
	}

	iomem =  devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(iomem)) {
		probe_err("devm_ioremap_resource(3) is fail(%p)", iomem);
		ret = PTR_ERR(iomem);
		goto p_err;
	}

	system->regs = iomem;
	system->regs_size = resource_size(res);
	probe_info("resource3 : %p\n", iomem);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		probe_err("platform_get_irq(0) is fail(%d)", irq);
		ret = -EINVAL;
		goto p_err;
	}

	system->irq0 = irq;

	irq = platform_get_irq(pdev, 1);
	if (irq < 0) {
		probe_err("platform_get_irq(1) is fail(%d)", irq);
		ret = -EINVAL;
		goto p_err;
	}

	system->irq1 = irq;

	ret = vpu_exynos_probe(&system->exynos, dev, system->regs, system->ram0, system->ram1);
	if (ret) {
		probe_err("vpu_exynos_probe is fail(%d)\n", ret);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_memory_probe(&system->memory, dev);
	if (ret) {
		vpu_err("vpu_memory_probe is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_interface_probe(&system->interface, dev,
		system->code, system->code_size,
		system->regs, system->regs_size,
		system->irq0, system->irq1);
	if (ret) {
		vpu_err("vpu_interface_probe is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_binary_init(&system->binary,
		dev,
		VPU_FW_PATH1,
		VPU_FW_PATH2,
		VPU_FW_NAME);
	if (ret) {
		vpu_err("vpu_binary_init is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_tv_probe(&system->tvset, dev, &system->exynos, &system->memory);
	if (ret) {
		vpu_err("vpu_tv_probe is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_system_open(struct vpu_system *system)
{
	int ret = 0;

	ret = vpu_memory_open(&system->memory);
	if (ret) {
		vpu_err("vpu_memory_open is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_interface_open(&system->interface);
	if (ret) {
		vpu_err("vpu_interface_open is fail(%d)\n", ret);
		vpu_memory_close(&system->memory);
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_system_close(struct vpu_system *system)
{
	int ret = 0;

	ret = vpu_interface_close(&system->interface);
	if (ret)
		vpu_err("vpu_interface_close is fail(%d)\n", ret);

	ret = vpu_memory_close(&system->memory);
	if (ret)
		vpu_err("vpu_memory_close is fail(%d)\n", ret);

	return ret;
}

int vpu_system_resume(struct vpu_system *system, u32 mode)
{
	int ret = 0;
	struct vpu_exynos *exynos;
	struct vpu_memory *memory;
	struct vpu_binary *binary;

	BUG_ON(!system);

	exynos = &system->exynos;
	memory = &system->memory;
	binary = &system->binary;

	ret = CLK_OP(exynos, clk_cfg);
	if (ret) {
		vpu_err("CLK_OP(clk_cfg) is fail(%d)\n", ret);
		goto p_err;
	}

#if defined(CONFIG_PM_DEVFREQ)
	pm_qos_add_request(&exynos_vpu_qos_cam, PM_QOS_CAM_THROUGHPUT, CAM_L5);
	pm_qos_add_request(&exynos_vpu_qos_mem, PM_QOS_BUS_THROUGHPUT, MIF_L6);
#endif

	ret = CLK_OP(exynos, clk_on);
	if (ret) {
		vpu_err("CLK_OP(clk_on) is fail(%d)\n", ret);
		goto p_err;
	}

#if defined(CONFIG_VIDEOBUF2_ION)
	vb2_ion_attach_iommu(system->memory.alloc_ctx);
#endif

	if (mode == VPU_DEVICE_MODE_TEST)
		goto p_err;

	ret = vpu_binary_read(binary, system->code, system->code_size);
	if (ret) {
		vpu_err("vpu_binary_load is fail(%d)\n", ret);
		goto p_err;
	}

	ret = CTL_OP(exynos, ctl_reset, false);
	if (ret) {
		vpu_err("CTL_OP(ctl_reset) is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_system_suspend(struct vpu_system *system)
{
	int ret = 0;
	struct vpu_exynos *exynos;
	struct vpu_memory *memory;

	BUG_ON(!system);

	exynos = &system->exynos;
	memory = &system->memory;

#if defined(CONFIG_VIDEOBUF2_ION)
	vb2_ion_detach_iommu(memory->alloc_ctx);
#endif

	ret = CTL_OP(exynos, ctl_reset, true);
	if (ret)
		vpu_err("CTL_OP(ctl_reset) is fail(%d)\n", ret);

	ret = CLK_OP(exynos, clk_off);
	if (ret)
		vpu_err("CLK_OP(clk_off) is fail(%d)\n", ret);

#if defined(CONFIG_PM_DEVFREQ)
	pm_qos_remove_request(&exynos_vpu_qos_cam);
	pm_qos_remove_request(&exynos_vpu_qos_mem);
#endif

	return ret;
}

int vpu_system_start(struct vpu_system *system)
{
	int ret = 0;

	ret = vpu_interface_start(&system->interface);
	if (ret) {
		vpu_err("vpu_interface_start is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_system_stop(struct vpu_system *system)
{
	int ret = 0;

	ret = vpu_interface_stop(&system->interface);
	if (ret)
		vpu_err("vpu_interface_stop is fail(%d)\n", ret);

	return ret;
}
