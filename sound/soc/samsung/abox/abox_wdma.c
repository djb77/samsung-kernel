/* sound/soc/samsung/abox/abox_rdma.c
 *
 * ALSA SoC Audio Layer - Samsung Abox WDMA driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/iommu.h>
#include <linux/delay.h>
#include <linux/memblock.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>

#include "../../../../drivers/iommu/exynos-iommu.h"
#include <sound/samsung/abox.h>
#include "abox_util.h"
#include "abox_gic.h"
#include "abox.h"

#define USE_FIXED_MEMORY
#define STR (SNDRV_PCM_STREAM_CAPTURE)

static const struct snd_pcm_hardware abox_wdma_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED
				| SNDRV_PCM_INFO_BLOCK_TRANSFER
				| SNDRV_PCM_INFO_MMAP
				| SNDRV_PCM_INFO_MMAP_VALID,
	.formats		= ABOX_WDMA_SAMPLE_FORMATS,
	.channels_min		= 1,
	.channels_max		= 8,
	.buffer_bytes_max	= BUFFER_BYTES_MAX,
	.period_bytes_min	= PERIOD_BYTES_MIN,
	.period_bytes_max	= PERIOD_BYTES_MAX,
	.periods_min		= BUFFER_BYTES_MAX / PERIOD_BYTES_MAX,
	.periods_max		= BUFFER_BYTES_MAX / PERIOD_BYTES_MIN,
};

static irqreturn_t abox_wdma_irq_handler(int irq, void *dev_id,
		ABOX_IPC_MSG *msg)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct abox_platform_data *data = platform_get_drvdata(pdev);
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg->msg.pcmtask;
	int id = data->id;

	if (id != pcmtask_msg->channel_id)
		return IRQ_NONE;

	dev_dbg(dev, "%s[%d]: ipcid=%d, msgtype=%d\n", __func__, id,
			msg->ipcid, pcmtask_msg->msgtype);

	switch (pcmtask_msg->msgtype) {
	case PCM_PLTDAI_POINTER:
		snd_pcm_period_elapsed(data->substream);
		break;
	default:
		dev_warn(dev, "Unknown pcmtask message: %d\n",
				pcmtask_msg->msgtype);
		break;
	}

	return IRQ_HANDLED;
}

static int abox_wdma_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct device *dev_abox = &data->pdev_abox->dev;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int id = data->id;
	int result;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	result = snd_pcm_lib_malloc_pages(substream,
			params_buffer_bytes(params));
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Memory allocation failed (size:%u)\n",
				params_buffer_bytes(params));
		return result;
	}

	pcmtask_msg->channel_id = id;
#ifndef USE_FIXED_MEMORY
	result = iommu_map(data->abox_data->iommu_domain, IOVA_WDMA_BUFFER(id),
			runtime->dma_addr, round_up(runtime->dma_bytes,
			PAGE_SIZE), 0);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "dma buffer iommu map failed\n");
		return result;
	}
#endif
	msg.ipcid = IPC_PCMCAPTURE;
	msg.task_id = pcmtask_msg->channel_id = id;

	pcmtask_msg->msgtype = PCM_SET_BUFFER;
	pcmtask_msg->param.setbuff.phyaddr = IOVA_WDMA_BUFFER(id);
	pcmtask_msg->param.setbuff.size = params_period_bytes(params);
	pcmtask_msg->param.setbuff.count = params_periods(params);
	abox_request_ipc(dev_abox, msg.ipcid, &msg, sizeof(msg), 0, 1);

	pcmtask_msg->msgtype = PCM_PLTDAI_HW_PARAMS;
	pcmtask_msg->param.hw_params.sample_rate = params_rate(params);
	pcmtask_msg->param.hw_params.bit_depth = params_width(params);
	pcmtask_msg->param.hw_params.channels = params_channels(params);
	abox_request_ipc(dev_abox, msg.ipcid, &msg, sizeof(msg), 0, 1);

	if (params_rate(params) > 48000)
		abox_request_cpu_gear_dai(dev, data->abox_data, rtd->cpu_dai, 2);

	dev_info(dev, "%s:Total=%zu PrdSz=%u(%u) #Prds=%u rate=%u, width=%d, channels=%u\n",
			snd_pcm_stream_str(substream), runtime->dma_bytes,
			params_period_size(params), params_period_bytes(params),
			params_periods(params), params_rate(params),
			params_width(params), params_channels(params));

	return 0;
}

static int abox_wdma_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct device *dev_abox = &data->pdev_abox->dev;
	int id = data->id;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_HW_FREE;
	msg.task_id = pcmtask_msg->channel_id = id;
	abox_request_ipc(dev_abox, msg.ipcid, &msg, sizeof(msg), 0, 1);
#ifndef USE_FIXED_MEMORY
	iommu_unmap(data->abox_data->iommu_domain, IOVA_WDMA_BUFFER(id),
			round_up(substream->runtime->dma_bytes, PAGE_SIZE));
	exynos_sysmmu_tlb_invalidate(data->abox_data->iommu_domain,
			(dma_addr_t)IOVA_WDMA_BUFFER(id),
			round_up(substream->runtime->dma_bytes, PAGE_SIZE));
#endif
	return snd_pcm_lib_free_pages(substream);
}

static int abox_wdma_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct device *dev_abox = &data->pdev_abox->dev;
	struct abox_data *abox_data = data->abox_data;
	int id = data->id;
	int result;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	if (abox_test_quirk(data->quirks, ABOX_QUIRK_BIT_TRY_TO_ASRC_OFF)) {
		result = abox_try_to_asrc_off(dev, abox_data, rtd->cpu_dai);
		if (IS_ERR_VALUE(result))
			dev_warn(dev, "abox_try_to_asrc_off: %d\n", result);
	}

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_PREPARE;
	msg.task_id = pcmtask_msg->channel_id = id;
	result = abox_request_ipc(dev_abox, msg.ipcid, &msg, sizeof(msg), 0, 1);

	data->pointer = IOVA_WDMA_BUFFER(id);

	return result;
}

static int abox_wdma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct device *dev_abox = &data->pdev_abox->dev;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int id = data->id;
	int result;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_info(dev, "%s[%d](%d)\n", __func__, id, cmd);

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_TRIGGER;
	msg.task_id = pcmtask_msg->channel_id = id;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (memblock_is_memory(runtime->dma_addr))
			abox_request_dram_on(data->pdev_abox, dev, true);

		pcmtask_msg->param.trigger = 1;
		result = abox_request_ipc(dev_abox,
				msg.ipcid, &msg, sizeof(msg), 1, 0);
		switch (data->type) {
		case PLATFORM_REALTIME:
			msg.ipcid = IPC_ERAP;
			msg.msg.erap.msgtype = REALTIME_START;
			result = abox_request_ipc(dev_abox,
					msg.ipcid, &msg, sizeof(msg), 1, 0);
			break;
		default:
			break;
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pcmtask_msg->param.trigger = 0;
		result = abox_request_ipc(dev_abox,
				msg.ipcid, &msg, sizeof(msg), 1, 0);
		switch (data->type) {
		case PLATFORM_REALTIME:
			msg.ipcid = IPC_ERAP;
			msg.msg.erap.msgtype = REALTIME_STOP;
			result = abox_request_ipc(dev_abox,
					msg.ipcid, &msg, sizeof(msg), 1, 0);
			break;
		default:
			break;
		}

		if (memblock_is_memory(runtime->dma_addr))
			abox_request_dram_on(data->pdev_abox, dev, false);
		break;
	default:
		result = -EINVAL;
		break;
	}

	return result;
}

static snd_pcm_uframes_t abox_wdma_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int id = data->id;
	ssize_t pointer;
	u32 status = readl(data->sfr_base + ABOX_WDMA_STATUS);
	bool progress = (status & ABOX_WDMA_PROGRESS_MASK) ? true : false;

	if (data->pointer >= IOVA_WDMA_BUFFER(id)) {
		pointer = data->pointer - IOVA_WDMA_BUFFER(id);
	} else if (((data->type == PLATFORM_NORMAL) ||
			(data->type == PLATFORM_SYNC)) && progress) {
		ssize_t offset, count;
		ssize_t buffer_bytes, period_bytes;

		buffer_bytes = snd_pcm_lib_buffer_bytes(substream);
		period_bytes = snd_pcm_lib_period_bytes(substream);

		offset = (((status & ABOX_WDMA_RBUF_OFFSET_MASK) >>
				ABOX_WDMA_RBUF_OFFSET_L) << 4);
		count = (status & ABOX_WDMA_RBUF_CNT_MASK);

		while ((offset % period_bytes) && (buffer_bytes >= 0)) {
			buffer_bytes -= period_bytes;
			if ((buffer_bytes & offset) == offset)
				offset = buffer_bytes;
		}

		pointer = offset + count;
	} else {
		pointer = 0;
	}

	dev_dbg(dev, "%s[%d]: pointer=%08zx\n", __func__, id, pointer);

	return bytes_to_frames(runtime, pointer);
}

static int abox_wdma_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct device *dev_abox = &data->pdev_abox->dev;
	int id = data->id;
	int result;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	if (data->type == PLATFORM_CALL) {
		abox_request_cpu_gear_sync(dev, data->abox_data,
				ABOX_CPU_GEAR_CALL_KERNEL, 1);
		result = abox_request_l2c_sync(dev, data->abox_data, dev, true);
		if (IS_ERR_VALUE(result))
			return result;
	}
	pm_runtime_get_sync(rtd->codec->dev);
	abox_request_cpu_gear_dai(dev, data->abox_data, rtd->cpu_dai, 3);

	snd_soc_set_runtime_hwparams(substream, &abox_wdma_hardware);

	data->substream = substream;

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_OPEN;
	msg.task_id = pcmtask_msg->channel_id = id;
	result = abox_request_ipc(dev_abox, msg.ipcid, &msg, sizeof(msg), 0, 1);

	return result;
}

static int abox_wdma_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct device *dev_abox = &data->pdev_abox->dev;
	int id = data->id;
	int result, i;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	dev_dbg(dev, "%s[%d]\n", __func__, id);

	data->substream = NULL;

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_CLOSE;
	msg.task_id = pcmtask_msg->channel_id = id;
	result = abox_request_ipc(dev_abox, msg.ipcid, &msg, sizeof(msg), 0, 1);

	switch (data->type) {
	case PLATFORM_CALL:
		break;
	default:
		for (i = ABOX_DMA_TIMEOUT_US;
				(readl(data->sfr_base + ABOX_WDMA_CTRL)
				& ABOX_WDMA_ENABLE_MASK) && i; i--) {
			udelay(1);
		}
		if (!i)
			dev_warn_ratelimited(dev, "disable timeout[%d]\n", id);
		break;
	}

	abox_request_cpu_gear_dai(dev, data->abox_data, rtd->cpu_dai, 12);
	pm_runtime_put(rtd->codec->dev);
	if (data->type == PLATFORM_CALL) {
		abox_request_cpu_gear_sync(dev, data->abox_data,
				ABOX_CPU_GEAR_CALL_KERNEL,
				ABOX_CPU_GEAR_LOWER_LIMIT);
		result = abox_request_l2c(dev, data->abox_data, dev, false);
		if (IS_ERR_VALUE(result))
			return result;
	}

	return result;
}

static int abox_wdma_mmap(struct snd_pcm_substream *substream,
		struct vm_area_struct *vma)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	int id = data->id;
	struct snd_pcm_runtime *runtime = substream->runtime;

	dev_info(dev, "%s[%d]\n", __func__, id);

	/* Increased cpu gear for sound camp.
	 * Only sound camp uses mmap now.
	 */
	abox_request_cpu_gear_dai(dev, data->abox_data, rtd->cpu_dai, 2);

	return dma_mmap_writecombine(dev, vma,
			runtime->dma_area,
			runtime->dma_addr,
			runtime->dma_bytes);
}

static struct snd_pcm_ops abox_wdma_ops = {
	.open		= abox_wdma_open,
	.close		= abox_wdma_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= abox_wdma_hw_params,
	.hw_free	= abox_wdma_hw_free,
	.prepare	= abox_wdma_prepare,
	.trigger	= abox_wdma_trigger,
	.pointer	= abox_wdma_pointer,
	.mmap		= abox_wdma_mmap,
};

static int abox_wdma_new(struct snd_soc_pcm_runtime *runtime)
{
	struct snd_pcm *pcm = runtime->pcm;
	struct snd_pcm_substream *substream = pcm->streams[STR].substream;
	struct snd_soc_platform *platform = runtime->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct abox_data *abox_data = data->abox_data;
	struct iommu_domain *domain = abox_data->iommu_domain;
	struct snd_dma_buffer *dma_buffer = &substream->dma_buffer;
	int id = data->id;
	size_t buffer_bytes;
	int result;

	switch (data->type) {
	case PLATFORM_NORMAL:
		buffer_bytes = BUFFER_BYTES_MAX;
		break;
	default:
		buffer_bytes = BUFFER_BYTES_MAX >> 2;
		break;
	}

	result = snd_pcm_lib_preallocate_pages(substream, SNDRV_DMA_TYPE_DEV,
			runtime->cpu_dai->dev, buffer_bytes, buffer_bytes);
	if (IS_ERR_VALUE(result)) {
		result = -ENOMEM;
	} else {
#ifdef USE_FIXED_MEMORY
		iommu_map(domain, IOVA_WDMA_BUFFER(id), dma_buffer->addr,
				BUFFER_BYTES_MAX, 0);
#endif
	}

	return result;
}

static void abox_wdma_free(struct snd_pcm *pcm)
{
#ifdef USE_FIXED_MEMORY
	struct snd_pcm_substream *substream = pcm->streams[STR].substream;
	struct snd_soc_pcm_runtime *runtime = substream->private_data;
	struct snd_soc_platform *platform = runtime->platform;
	struct device *dev = platform->dev;
	struct abox_platform_data *data = dev_get_drvdata(dev);
	struct abox_data *abox_data = data->abox_data;
	struct iommu_domain *domain = abox_data->iommu_domain;
	int id = data->id;

	iommu_unmap(domain, IOVA_WDMA_BUFFER(id), BUFFER_BYTES_MAX);
#endif
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static struct snd_soc_platform_driver abox_wdma = {
	.ops		= &abox_wdma_ops,
	.pcm_new	= abox_wdma_new,
	.pcm_free	= abox_wdma_free,
};

static int samsung_abox_wdma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *np_abox;
	struct abox_platform_data *data;
	int result;
	const char *type;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);

	data->sfr_base = devm_not_request_and_map(pdev, "sfr", 0, NULL, NULL);
	if (IS_ERR(data->sfr_base))
		return PTR_ERR(data->sfr_base);

	np_abox = of_parse_phandle(np, "abox", 0);
	if (!np_abox) {
		dev_err(dev, "Failed to get abox device node\n");
		return -EPROBE_DEFER;
	}
	data->pdev_abox = of_find_device_by_node(np_abox);
	if (!data->pdev_abox) {
		dev_err(dev, "Failed to get abox platform device\n");
		return -EPROBE_DEFER;
	}
	data->abox_data = platform_get_drvdata(data->pdev_abox);

	abox_register_irq_handler(&data->pdev_abox->dev, IPC_PCMCAPTURE,
			abox_wdma_irq_handler, pdev);

	result = of_property_read_u32_index(np, "id", 0, &data->id);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "id property reading fail\n");
		return result;
	}

	result = of_property_read_string(np, "type", &type);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "type property reading fail\n");
		return result;
	}
	if (!strncmp(type, "call", sizeof("call")))
		data->type = PLATFORM_CALL;
	else if (!strncmp(type, "compress", sizeof("compress")))
		data->type = PLATFORM_COMPRESS;
	else if (!strncmp(type, "realtime", sizeof("realtime")))
		data->type = PLATFORM_REALTIME;
	else if (!strncmp(type, "vi-sensing", sizeof("vi-sensing")))
		data->type = PLATFORM_VI_SENSING;
	else if (!strncmp(type, "sync", sizeof("sync")))
		data->type = PLATFORM_SYNC;
	else
		data->type = PLATFORM_NORMAL;

	data->quirks = abox_probe_quirks(np);

	abox_register_wdma(data->abox_data->pdev, pdev, data->id);

	return snd_soc_register_platform(&pdev->dev, &abox_wdma);
}

static int samsung_abox_wdma_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static const struct of_device_id samsung_abox_wdma_match[] = {
	{
		.compatible = "samsung,abox-wdma",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_wdma_match);

static struct platform_driver samsung_abox_wdma_driver = {
	.probe  = samsung_abox_wdma_probe,
	.remove = samsung_abox_wdma_remove,
	.driver = {
		.name = "samsung-abox-wdma",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_wdma_match),
	},
};

module_platform_driver(samsung_abox_wdma_driver);

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box WDMA Driver");
MODULE_ALIAS("platform:samsung-abox-wdma");
MODULE_LICENSE("GPL");
