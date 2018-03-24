/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS IMA driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/list_sort.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <misc/exynos_ima.h>

#define IMA_DEBUG
#ifdef IMA_DEBUG
#define ima_dbg(fmt, args...)				\
	do {						\
		pr_info("[%s:%d] "			\
		fmt, __func__, __LINE__, ##args);	\
	} while (0)
#else
#define ima_dbg(fmt, args...)
#endif

static struct ima_dev *ima_device;

enum {
	IVA_PATH_CPU = 0,
	IVA_PATH_DMA,
};

int iva_path = IVA_PATH_DMA;

struct ima_client *ima_create_client(struct device *dev,
		ima_reclaim_callback_t reclaim_callback, void *priv)
{
	struct ima_dev *ima_dev = ima_device;
	struct ima_client *client;

	if (!ima_dev) {
		pr_err("No IMA device exists!\n");
		return ERR_PTR(-EINVAL);
	}

	if (!reclaim_callback) {
		dev_err(ima_dev->dev, "reclaim callback is mandatory!\n");
		return ERR_PTR(-EINVAL);
	}

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		return ERR_PTR(-ENOMEM);

	client->dev = dev;
	INIT_LIST_HEAD(&client->node);
	INIT_LIST_HEAD(&client->buffer_list);
	atomic_set(&client->refcount, 0);
	spin_lock_init(&client->lock);

	client->ima_dev = ima_dev;
	client->reclaim_callback = reclaim_callback;
	client->priv = priv;

	spin_lock(&ima_device->lock);
	list_add_tail(&client->node, &ima_device->client_list);
	spin_unlock(&ima_device->lock);

	ima_dbg("ima cleint is created for %s\n", dev_name(dev));

	return client;
}

void ima_destroy_client(struct ima_client *client)
{
	struct ima_dev *ima_dev = client->ima_dev;

	if (WARN_ON(!client))
		return;

	ima_dbg("++ ima client for %s will be destroyed.\n",
					dev_name(client->dev));

	if (atomic_read(&client->refcount)) {
		struct ima_buffer *buffer, *tmp;

		spin_lock(&client->lock);
		list_for_each_entry_safe(buffer, tmp,
					&client->buffer_list, node) {
			ima_dbg("freeing buffer %p/%#lx owned by %s\n",
					buffer->addr, buffer->size,
					dev_name(buffer->client->dev));
			list_del(&buffer->node);
			gen_pool_free(ima_dev->pool,
				(unsigned long)buffer->addr, buffer->size);
			kfree(buffer);

			atomic_dec(&client->refcount);
			atomic_dec(&ima_dev->refcount);
		}
		spin_unlock(&client->lock);
	}
	spin_lock(&ima_device->lock);
	list_del(&client->node);
	spin_unlock(&ima_device->lock);

	ima_dbg("-- ima client is destroyed\n");
	kfree(client);

	return;
}

void *ima_alloc(struct ima_client *client, unsigned long size,
						unsigned long flags)
{
	struct ima_dev *ima_dev = client->ima_dev;
	struct ima_buffer *buffer;
	void *addr;

	spin_lock(&ima_dev->lock);
	ima_dbg("Alloc size %#lx for %s, ima state = %d\n",
			size, dev_name(client->dev), ima_dev->state);

	if (ima_dev->state == IMA_STATE_HOST ||
			ima_dev->state == IMA_STATE_RECLAIMING) {
		dev_info(ima_dev->dev, "IMA is in busy, state = %d\n",
							ima_dev->state);
		spin_unlock(&ima_dev->lock);
		return ERR_PTR(-EBUSY);
	}
	spin_unlock(&ima_dev->lock);

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer) {
		dev_info(ima_dev->dev, "failed to alloc buffer structure\n");
		return NULL;
	}

	addr = (void *)gen_pool_alloc(ima_dev->pool, size);
	if (!addr) {
		dev_info(ima_dev->dev,
				"failed to alloc buffer, size = %#lx\n", size);
		kfree(buffer);
		return NULL;
	}
	atomic_inc(&client->refcount);

	buffer->addr = addr;
	buffer->size = size;
	buffer->client = client;
	spin_lock(&client->lock);
	list_add_tail(&buffer->node, &client->buffer_list);
	spin_unlock(&client->lock);

	if (atomic_inc_return(&ima_dev->refcount) == 1) {
		pm_runtime_get_sync(ima_dev->dev);
		spin_lock(&ima_dev->lock);
		ima_dev->state = IMA_STATE_CLIENT;
		spin_unlock(&ima_dev->lock);
	}

	ima_dbg("ref count = %d, return addr %p\n",
			atomic_read(&ima_dev->refcount), addr);

	return addr;
}

void ima_free(struct ima_client *client, void *vaddr)
{
	struct ima_dev *ima_dev = client->ima_dev;
	struct ima_buffer *buffer, *tmp;
	bool valid = false;

	ima_dbg("Free addr %p for %s, ima state = %d\n",
			vaddr, dev_name(client->dev), ima_dev->state);

	spin_lock(&client->lock);
	list_for_each_entry_safe(buffer, tmp, &client->buffer_list, node) {
		if (buffer->addr == vaddr) {
			list_del(&buffer->node);
			gen_pool_free(ima_dev->pool,
				(unsigned long)vaddr, buffer->size);
			kfree(buffer);

			atomic_dec(&client->refcount);
			valid = true;
			break;
		}
	}
	spin_unlock(&client->lock);

	/* Power/clock management */
	if (valid && atomic_dec_return(&ima_dev->refcount) == 0) {
		/*
		 * Here we use pm_runtime_put() instead of put_sync(),
		 * because it can be called in atomic context.
		 */
		pm_runtime_put(ima_dev->dev);
		ima_dev->state = IMA_STATE_IDLE;
	}

	ima_dbg("ref count = %d\n", atomic_read(&ima_dev->refcount));
}

phys_addr_t ima_get_dma_addr(struct ima_client *client, void *vaddr)
{
	struct ima_dev *ima_dev = client->ima_dev;
	struct ima_buffer *buffer;
	phys_addr_t dma_addr = 0;

	spin_lock(&client->lock);
	list_for_each_entry(buffer, &client->buffer_list, node) {
		if (buffer->addr == vaddr) {
			dma_addr = gen_pool_virt_to_phys(ima_dev->pool,
							(unsigned long)vaddr);
			break;
		}
	}
	spin_unlock(&client->lock);

	ima_dbg("vaddr %p returned phys %pa for %s\n",
		vaddr, &dma_addr, dev_name(client->dev));

	return dma_addr;
}

void ima_host_begin(void)
{
	struct ima_dev *ima_dev = ima_device;
	struct ima_client *client;
	int ret;

	if (!ima_dev) {
		pr_err("No IMA device exists!\n");
		return;
	}

	spin_lock(&ima_dev->lock);
	ima_dbg("++ host beginning, state = %d\n", ima_dev->state);

	switch (ima_dev->state) {
	case IMA_STATE_IDLE:
		ima_dev->state = IMA_STATE_HOST;
		/* fall through */
	case IMA_STATE_HOST:
		ima_dbg("-- host returned immediately\n");
		spin_unlock(&ima_dev->lock);
		return;
	default:
		break;
	}

	ima_dev->state = IMA_STATE_RECLAIMING;

	list_for_each_entry(client, &ima_dev->client_list, node) {
		ret = client->reclaim_callback(client,
				client->dev, client->priv);
		if (ret)
			dev_err(client->dev, "reclaim failed, ret = %d\n", ret);
		if (atomic_read(&client->refcount))
			dev_err(client->dev,
				"%d buffer remained after reclaiming\n",
				atomic_read(&client->refcount));
	}

	if (atomic_read(&ima_dev->refcount)) {
		dev_err(ima_dev->dev, "Warning: %d buffer is still remained\n",
				atomic_read(&ima_dev->refcount));
		list_for_each_entry(client, &ima_dev->client_list, node)
			dev_err(ima_dev->dev, "client %s, buf count = %d\n",
				dev_name(client->dev),
				atomic_read(&client->refcount));
	}

	ima_dev->state = IMA_STATE_HOST;

	ima_dbg("-- host beginning, state = %d\n", ima_dev->state);
	spin_unlock(&ima_dev->lock);

	return;
}

void ima_host_end(void)
{
	struct ima_dev *ima_dev = ima_device;

	if (!ima_dev) {
		pr_err("No IMA device exists!\n");
		return;
	}

	ima_dbg("++ host end, state = %d\n", ima_dev->state);

	ima_dev->state = IMA_STATE_IDLE;

	return;
}

#ifdef CONFIG_PM_SLEEP
static int ima_suspend(struct device *dev)
{
	return 0;
}

static int ima_resume(struct device *dev)
{
	return 0;
}
#endif

#ifdef CONFIG_PM

#define IVA_PRE_REG_CLK_CONTROL		0x20
#define IVA_PRE_REG_SOFT_RESET		0x24
#define IVA_PRE_REG_SET_VAL		0x7FFF
#define IVA_PRE_REG_MASK		0x7FFF

#define IVA_SYS_REG_SC_CON		0x1014

static int ima_runtime_resume(struct device *dev)
{
	struct ima_dev *ima_dev = dev_get_drvdata(dev);
	int val;

	ima_dbg("ref count = %d\n", atomic_read(&ima_dev->refcount));

	if (iva_path == IVA_PATH_CPU) {
		val = readl(ima_dev->pre_base + IVA_PRE_REG_CLK_CONTROL);
		val &= ~IVA_PRE_REG_MASK;
		writel(val, ima_dev->pre_base + IVA_PRE_REG_CLK_CONTROL);

		val = readl(ima_dev->pre_base + IVA_PRE_REG_SOFT_RESET);
		val |= IVA_PRE_REG_SET_VAL;
		writel(val, ima_dev->pre_base + IVA_PRE_REG_SOFT_RESET);
	} else {
		writel(0x1, ima_dev->sysreg_base + IVA_SYS_REG_SC_CON);
	}

	return 0;
}

static int ima_runtime_suspend(struct device *dev)
{
	struct ima_dev *ima_dev = dev_get_drvdata(dev);

	ima_dbg("ref count = %d\n", atomic_read(&ima_dev->refcount));

	return 0;
}
#endif

static const struct dev_pm_ops ima_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ima_suspend, ima_resume)
	SET_RUNTIME_PM_OPS(NULL, ima_runtime_resume, ima_runtime_suspend)
};

static int ima_get_resources(struct platform_device *pdev, struct ima_dev *ima)
{
	struct resource *res_cpu, *res_dma, *res_reg;
	size_t size_cpu, size_dma;
	int ret;

	/* Get resource for CPU path */
	res_cpu = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_cpu) {
		dev_err(ima->dev, "failed to get mem resource(cpu)\n");
		return -EINVAL;
	}
	size_cpu = resource_size(res_cpu);

	/* Get resource for DMA path */
	res_dma = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res_dma) {
		dev_err(ima->dev, "failed to get mem resource(dma)\n");
		return -EINVAL;
	}

	size_dma = resource_size(res_dma);
	if (!devm_request_mem_region(ima->dev, res_dma->start,
						size_dma, pdev->name)) {
		dev_err(ima->dev, "failed to request mem region(dma)\n");
		return -EBUSY;
	}

	if (size_cpu != size_dma) {
		dev_err(ima->dev, "size is different between cpu and dma\n");
		dev_err(ima->dev, "cpu size = %zx, dma size = %zx\n",
				size_cpu, size_dma);
		return -EINVAL;
	}

	/* Create gen pool and add pool with CPU and DMA address */
	ima->pool = devm_gen_pool_create(ima->dev, PAGE_SHIFT, -1, NULL);
	if (IS_ERR(ima->pool)) {
		dev_err(ima->dev, "failed to create pool, err = %ld\n",
				PTR_ERR(ima->pool));
		return PTR_ERR(ima->pool);
	}

	ret = gen_pool_add_virt(ima->pool, (unsigned long)res_cpu->start,
			res_dma->start, size_cpu, -1);
	if (ret) {
		dev_err(ima->dev, "failed to add pool, err = %d\n", ret);
		return ret;
	}

	/* Get resource for sysreg control */
	res_reg = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!res_reg) {
		dev_err(ima->dev, "failed to get mem resource(sysreg)\n");
		return -EINVAL;
	}

	ima->sysreg_base = devm_ioremap_resource(&pdev->dev, res_reg);
	if (IS_ERR(ima->sysreg_base)) {
		dev_err(ima->dev, "failed to map sysreg, err = %ld\n",
						PTR_ERR(ima->sysreg_base));
		return PTR_ERR(ima->sysreg_base);
	}

	/* Get resource for pre-register control */
	res_reg = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (!res_reg) {
		dev_err(ima->dev, "failed to get mem resource(pre-reg)\n");
		return -EINVAL;
	}

	ima->pre_base = ioremap(res_reg->start, resource_size(res_reg));
	if (!ima->pre_base) {
		dev_err(ima->dev, "failed to map pre-register\n");
		return -ENOMEM;
	}

	dev_info(ima->dev, "pool size %zu KiB@0x%pa for CPU, 0x%pa for DMA\n",
			gen_pool_size(ima->pool) / 1024,
			&res_cpu->start, &res_dma->start);

	return 0;
}

static int ima_probe(struct platform_device *pdev)
{
	struct ima_dev *ima;
	int ret;

	ima = devm_kzalloc(&pdev->dev, sizeof(*ima), GFP_KERNEL);
	if (!ima) {
		dev_err(&pdev->dev, "failed to allocate ima dev\n");
		return -ENOMEM;
	}
	ima->dev = &pdev->dev;

	ret = ima_get_resources(pdev, ima);
	if (ret) {
		dev_err(ima->dev, "failed to get resources, ret = %d\n", ret);
		return ret;
	}

	ima->clock = devm_clk_get(ima->dev, "gate");
	if (IS_ERR(ima->clock)) {
		dev_err(ima->dev, "failed to get clock, err = %ld\n",
				PTR_ERR(ima->clock));
		ret = PTR_ERR(ima->clock);
		goto err_get_resource;
	}

	ima->state = IMA_STATE_IDLE;
	atomic_set(&ima->refcount, 0);

	spin_lock_init(&ima->lock);
	INIT_LIST_HEAD(&ima->client_list);

	platform_set_drvdata(pdev, ima);
	ima_device = ima;

	pm_runtime_enable(ima->dev);

	dev_info(ima->dev, "probed successfully\n");

	return 0;

err_get_resource:
	iounmap(ima->pre_base);

	dev_err(ima->dev, "probe failed, ret = %d\n", ret);

	return ret;
}

static int ima_remove(struct platform_device *pdev)
{
	struct ima_dev *ima = platform_get_drvdata(pdev);

	ima_device = NULL;
	iounmap(ima->pre_base);
	dev_info(ima->dev, "removed\n");

	return 0;
}

static const struct of_device_id exynos_ima_match[] = {
	{
		.compatible = "samsung,exynos-ima",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_ima_match);

static struct platform_driver ima_driver = {
	.probe		= ima_probe,
	.remove		= ima_remove,
	.driver = {
		.name	= "exynos-ima",
		.owner	= THIS_MODULE,
		.pm	= &ima_pm_ops,
		.of_match_table = of_match_ptr(exynos_ima_match),
	}
};

module_platform_driver(ima_driver);

MODULE_AUTHOR("Janghyuck, Kim <janghyuck.kim@samsung.com>");
MODULE_DESCRIPTION("EXYNOS IMA Driver");
MODULE_LICENSE("GPL");
