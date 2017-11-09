/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/exynos_iovmm.h>
#include <linux/pm_runtime.h>

#include "vpu-config.h"
#include "vpu-device.h"
#include "vpu-graph.h"
#include "vpu-vertex.h"
#include "vpu-graphmgr.h"
#include "vpu-resource.h"

static int vpu_device_runtime_suspend(struct device *dev);
static int vpu_device_runtime_resume(struct device *dev);

void __vpu_fault_handler(struct vpu_device *device) {
	struct vpu_system *system;
	struct vpu_memory *memory;
	struct vpu_graphmgr *graphmgr;
	struct vpu_graph *graph;
	struct vpuo_pu *pu, *temp;
	struct vb_container *container;
	struct vb_buffer *buffer;
	u32 i, j, k;

	system = &device->system;
	memory = &system->memory;
	graphmgr = &device->graphmgr;

	/* 1. Internal Memory Infomation */
	vpu_info("internal memory : 0x%llX\n", memory->info.dvaddr);

	/* 2. Buffer Memroy Information */
	for (i = 0; i < VPU_MAX_GRAPH; ++i) {
		graph = graphmgr->graph[i];
		if (!graph)
			continue;

		vpu_info("=================================================\n");
		vpu_info("GRAPH : %d(%d)\n", graph->id, graph->uid);
		vpu_info("INPUT--------------------------------------------\n");
		list_for_each_entry_safe(pu, temp, &graph->inleaf_list, gleaf_entry) {
			vpu_info("PU : %d\n", pu->id);
			vpu_info("-------------------------------------------------\n");
			for (j = 0; j < VPU_MAX_BUFFER; ++j) {
				container = pu->container[j];
				if (!container)
					continue;

				if (j == 0) {
					vpu_info("TYPE : %d\n", container->type);
					vpu_info("RESOLUTION : %d x %d\n", container->format->width, container->format->height);
					vpu_info("SIZE : %d\n", container->format->size[container->format->plane]);
					vpu_info("-------------------------------------------------\n");
				}

				for (k = 0; k < container->count; ++k) {
					buffer = &container->buffers[k];
					vpu_info("[%d][%d]DVADDR : %llx\n", j, k, buffer->dvaddr);
					vpu_info("[%d][%d]KVADDR : %p\n", j, k, buffer->kvaddr);
				}

				vpu_info("-------------------------------------------------\n");
			}
		}

		vpu_info("OUTPUT-------------------------------------------\n");
		list_for_each_entry_safe(pu, temp, &graph->otleaf_list, gleaf_entry) {
			vpu_info("PU : %d\n", pu->id);
			vpu_info("-------------------------------------------------\n");
			for (j = 0; j < VPU_MAX_BUFFER; ++j) {
				container = pu->container[j];
				if (!container)
					continue;

				if (j == 0) {
					vpu_info("TYPE : %d\n", container->type);
					vpu_info("RESOLUTION : %d x %d\n", container->format->width, container->format->height);
					vpu_info("SIZE : %d\n", container->format->size[container->format->plane]);
					vpu_info("-------------------------------------------------\n");
				}

				for (k = 0; k < container->count; ++k) {
					buffer = &container->buffers[k];
					vpu_info("[%d][%d]DVADDR : %llx\n", j, k, buffer->dvaddr);
					vpu_info("[%d][%d]KVADDR : %p\n", j, k, buffer->kvaddr);
				}

				vpu_info("-------------------------------------------------\n");
			}
		}

		vpu_graph_print(graph);
	}
}

static int __attribute__((unused)) vpu_fault_handler(struct iommu_domain *domain,
	struct device *dev,
	unsigned long fault_addr,
	int fault_flag,
	void *token)
{
	struct vpu_device *device;

	pr_err("<VPU FAULT HANDLER>\n");
	pr_err("Device virtual(0x%X) is invalid access\n", (u32)fault_addr);

	device = dev_get_drvdata(dev);

	__vpu_fault_handler(device);

	return -EINVAL;
}

static int __vpu_device_start(struct vpu_device *device)
{
	int ret = 0;

	if (test_bit(VPU_DEVICE_STATE_START, &device->state)) {
		vpu_err("already started\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_system_start(&device->system);
	if (ret) {
		vpu_err("vpu_system_start is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_debug_start(&device->debug);
	if (ret) {
		vpu_err("vpu_debug_start is fail(%d)\n", ret);
		goto p_err;
	}

	set_bit(VPU_DEVICE_STATE_START, &device->state);

p_err:
	return ret;
}

static int __vpu_device_stop(struct vpu_device *device)
{
	int ret = 0;

	if (!test_bit(VPU_DEVICE_STATE_START, &device->state))
		goto p_err;

	ret = vpu_debug_stop(&device->debug);
	if (ret)
		vpu_err("vpu_debug_stop is fail(%d)\n", ret);

	ret = vpu_system_stop(&device->system);
	if (ret)
		vpu_err("vpu_system_stop is fail(%d)\n", ret);

	clear_bit(VPU_DEVICE_STATE_START, &device->state);

p_err:
	return ret;
}

static int __vpu_device_power_on(struct vpu_device *device)
{
	int ret = 0;

	ret = pm_runtime_get_sync(device->dev);
	if (ret)
		vpu_err("runtime resume is fail(%d)", ret);

	return ret;
}

static int __vpu_device_power_off(struct vpu_device *device)
{
	int ret = 0;

	ret = pm_runtime_put_sync(device->dev);
	if (ret)
		vpu_err("runtime resume is fail(%d)", ret);

	return ret;
}

static int vpu_device_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;
	struct vpu_device *device;

	BUG_ON(!pdev);

	dev = &pdev->dev;

	device = devm_kzalloc(dev, sizeof(struct vpu_device), GFP_KERNEL);
	if (!device) {
		probe_err("device is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	ret = vpu_system_probe(&device->system, pdev);
	if (ret) {
		probe_err("vpu_system_probe is fail(%d)\n", ret);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_vertex_probe(&device->vertex, dev);
	if (ret) {
		probe_err("vpu_vertex_probe is fail(%d)\n", ret);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_resource_probe(&device->resource, &device->vertex.vd.dev);
	if (ret) {
		probe_err("vpu_resource_probe is fail(%d)\n", ret);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_graphmgr_probe(&device->graphmgr, &device->resource);
	if (ret) {
		probe_err("vpu_graphmgr_probe is fail(%d)\n", ret);
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_debug_probe(&device->debug, &device->graphmgr, &device->system);
	if (ret) {
		probe_err("vpu_debug_probe is fail(%d)\n", ret);
		ret = -EINVAL;
		goto p_err;
	}

	iovmm_set_fault_handler(dev, vpu_fault_handler, NULL);
	pm_runtime_enable(dev);

	device->dev = dev;
	device->mode = VPU_DEVICE_MODE_NORMAL;
	clear_bit(VPU_DEVICE_STATE_OPEN, &device->state);
	clear_bit(VPU_DEVICE_STATE_START, &device->state);
	dev_set_drvdata(dev, device);

p_err:
	probe_info("%s():%d\n", __func__, ret);
	return ret;
}

int vpu_device_open(struct vpu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	if (test_bit(VPU_DEVICE_STATE_OPEN, &device->state)) {
		vpu_err("device is already opened\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = vpu_system_open(&device->system);
	if (ret) {
		vpu_err("vpu_system_open is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_resource_open(&device->resource);
	if (ret) {
		vpu_err("vpu_resource_open is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_debug_open(&device->debug);
	if (ret) {
		vpu_err("vpu_debug_open is fail(%d)\n", ret);
		goto p_err;
	}

	ret = vpu_graphmgr_open(&device->graphmgr);
	if (ret) {
		vpu_err("vpu_graphmgr_open is fail(%d)\n", ret);
		goto p_err;
	}

	ret = __vpu_device_power_on(device);
	if (ret) {
		vpu_err("__vpu_device_power_on is fail(%d)\n", ret);
		goto p_err;
	}

	set_bit(VPU_DEVICE_STATE_OPEN, &device->state);

p_err:
	vpu_info("%s():%d\n", __func__, ret);
	return ret;
}

int vpu_device_close(struct vpu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	if (!test_bit(VPU_DEVICE_STATE_OPEN, &device->state)) {
		vpu_err("device is already closed\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = __vpu_device_stop(device);
	if (ret)
		vpu_err("__vpu_device_stop is fail(%d)\n", ret);

	ret = vpu_graphmgr_close(&device->graphmgr);
	if (ret)
		vpu_err("vpu_graphmgr_close is fail(%d)\n", ret);

	ret = vpu_resource_close(&device->resource);
	if (ret)
		vpu_err("vpu_resource_close is fail(%d)\n", ret);

	ret = vpu_system_close(&device->system);
	if (ret)
		vpu_err("vpu_system_close is fail(%d)\n", ret);

	ret = vpu_debug_close(&device->debug);
	if (ret)
		vpu_err("vpu_debug_close is fail(%d)\n", ret);

	ret = __vpu_device_power_off(device);
	if (ret)
		vpu_err("__vpu_device_power_off is fail(%d)\n", ret);

	clear_bit(VPU_DEVICE_STATE_OPEN, &device->state);

p_err:
	vpu_info("%s():%d\n", __func__, ret);
	return ret;
}

int vpu_device_start(struct vpu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	ret = __vpu_device_start(device);
	if (ret)
		vpu_err("__vpu_device_start is fail(%d)\n", ret);

	vpu_info("%s():%d\n", __func__, ret);

	return ret;
}

int vpu_device_stop(struct vpu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	ret = __vpu_device_stop(device);
	if (ret)
		vpu_err("__vpu_device_stop is fail(%d)\n", ret);

	vpu_info("%s():%d\n", __func__, ret);

	return ret;
}

static int vpu_device_remove(struct platform_device *pdev)
{
	return 0;
}

static int vpu_device_suspend(struct device *dev)
{
	return 0;
}

static int vpu_device_resume(struct device *dev)
{
	return 0;
}

static int vpu_device_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct vpu_device *device;

	device = dev_get_drvdata(dev);

	ret = vpu_system_suspend(&device->system);
	if (ret)
		vpu_err("vpu_system_suspend is fail(%d)\n", ret);

	vpu_info("%s():%d\n", __func__, ret);
	return ret;
}

static int vpu_device_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct vpu_device *device;

	device = dev_get_drvdata(dev);

	ret = vpu_system_resume(&device->system, device->mode);
	if (ret) {
		vpu_err("vpu_system_resume is fail(%d)\n", ret);
		goto p_err;
	}

p_err:
	vpu_info("%s():%d\n", __func__, ret);
	return ret;
}

static const struct dev_pm_ops vpu_pm_ops = {
	.suspend		= vpu_device_suspend,
	.resume			= vpu_device_resume,
	.runtime_suspend	= vpu_device_runtime_suspend,
	.runtime_resume		= vpu_device_runtime_resume,
};

static const struct of_device_id exynos_vpu_match[] = {
	{
		.compatible = "samsung,exynos-vpu",
	},
	{}
};
MODULE_DEVICE_TABLE(of, exynos_vpu_match);

static struct platform_driver vpu_driver = {
	.probe		= vpu_device_probe,
	.remove		= vpu_device_remove,
	.driver = {
		.name	= "exynos-vpu",
		.owner	= THIS_MODULE,
		.pm	= &vpu_pm_ops,
		.of_match_table = of_match_ptr(exynos_vpu_match)
	}
};

static int __init vpu_device_init(void)
{
	int ret = platform_driver_register(&vpu_driver);
	if (ret)
		probe_err("platform_driver_register is fail():%d\n", ret);

	probe_info("vpu device init is loaded");

	return ret;
}
late_initcall(vpu_device_init);

static void __exit vpu_device_exit(void)
{
	platform_driver_unregister(&vpu_driver);
}
module_exit(vpu_device_exit);

MODULE_AUTHOR("Gilyeon im<kilyeon.im@samsung.com>");
MODULE_DESCRIPTION("Exynos VPU driver");
MODULE_LICENSE("GPL");
