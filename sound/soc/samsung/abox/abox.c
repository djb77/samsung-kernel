/* sound/soc/samsung/abox/abox.c
 *
 * ALSA SoC Audio Layer - Samsung Abox driver
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
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/exynos_iovmm.h>
#include <linux/workqueue.h>
#include <linux/smc.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/shm_ipc.h>
#include <linux/modem_notifier.h>

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>
#include <sound/samsung/abox.h>
#include <sound/samsung/vts.h>

#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-itmon.h>
#include <misc/exynos_ima.h>
#include "../../../../drivers/iommu/exynos-iommu.h"

#include "abox_util.h"
#include "abox_dbg.h"
#include "abox_log.h"
#include "abox_dump.h"
#include "abox_gic.h"
#include "abox.h"

#undef EMULATOR
#ifdef EMULATOR
static void __iomem *pmu_alive;
static void update_mask_value(void __iomem *sfr,
		unsigned int mask, unsigned int value)
{
	unsigned int sfr_value = readl(sfr);

	set_mask_value(sfr_value, mask, value);
	writel(sfr_value, sfr);
}
#endif

#define GPIO_MODE_ABOX_SYS_PWR_REG		(0x1308)
#define PAD_RETENTION_ABOX_OPTION		(0x3048)
#define ABOX_MAGIC				(0x0814)
#define ABOX_MAGIC_VALUE			(0xAB0CAB0C)
#define ABOX_CA7_CONFIGURATION			(0x2520)
#define ABOX_CA7_LOCAL_PWR_CFG			(0x00000001)
#define ABOX_CA7_STATUS				(0x2524)
#define ABOX_CA7_STATUS_STANDBYWFE_MASK		(0x20000000)
#define ABOX_CA7_STATUS_STANDBYWFI_MASK		(0x10000000)
#define ABOX_CA7_STATUS_STATUS_MASK		(0x00000001)
#define ABOX_CA7_OPTION				(0x2528)
#define ABOX_CA7_OPTION_USE_STANDBYWFE_MASK	(0x00020000)
#define ABOX_CA7_OPTION_USE_STANDBYWFI_MASK	(0x00010000)
#define ABOX_CA7_OPTION_ENABLE_CPU_MASK		(0x00008000)

#define CPU_GEAR_LOWER_LIMIT		ABOX_CPU_GEAR_LOWER_LIMIT
#define DEFAULT_CPU_GEAR_ID		(0xAB0CDEFA)
#define DEFAULT_LIT_FREQ_ID		DEFAULT_CPU_GEAR_ID
#define DEFAULT_BIG_FREQ_ID		DEFAULT_CPU_GEAR_ID
#define DEFAULT_HMP_BOOST_ID		DEFAULT_CPU_GEAR_ID
#define AUD_PLL_RATE_KHZ		(1179648)
#define AUD_PLL_RATE_HZ_FOR_48000	(1179648040)
#define AUD_PLL_RATE_HZ_FOR_44100	(1083801600)
#define AUD_PLL_RATE_HZ_BYPASS		(26000000)
#define AUDIF_RATE_HZ			(24576000)
#define CALLIOPE_ENABLE_TIMEOUT_MS	(1000)
#define BOOT_DONE_TIMEOUT_MS		(10000)

/* For only external static functions */
static struct abox_data *p_abox_data;

struct abox_data *abox_get_abox_data(void)
{
	return p_abox_data;
}

static int abox_iommu_fault_handler(
		struct iommu_domain *domain, struct device *dev,
		unsigned long fault_addr, int fault_flags, void *token)
{
	struct abox_data *data = token;

	abox_dbg_print_gpr(&data->pdev->dev, data);
	return 0;
}

static void abox_cpu_power(bool on);
static int abox_cpu_enable(bool enable);
static int abox_cpu_pm_ipc(struct device *dev, bool resume);
static void abox_boot_done(struct device *dev, unsigned int version);

static void exynos_abox_panic_handler(void)
{
	static bool has_run;
	struct abox_data *data = p_abox_data;
	struct device *dev = data ? (data->pdev ?
			&data->pdev->dev : NULL) : NULL;

	dev_dbg(dev, "%s\n", __func__);

	if (abox_is_on() && dev) {
		if (has_run) {
			dev_info(dev, "already dumped\n");
			return;
		}
		has_run = true;

		abox_dbg_dump_gpr(dev, data, ABOX_DBG_DUMP_KERNEL, "panic");
		abox_cpu_pm_ipc(dev, false);
		writel(0x504E4943, data->sram_base + 0x30FFC);
		abox_cpu_enable(false);
		abox_cpu_power(false);
		abox_cpu_power(true);
		abox_cpu_enable(true);
		mdelay(100);
		abox_dbg_dump_mem(dev, data, ABOX_DBG_DUMP_KERNEL, "panic");
	} else {
		dev_info(dev, "%s: dump is skipped due to no power\n",
				__func__);
	}
}

static int abox_panic_handler(struct notifier_block *nb,
			       unsigned long action, void *data)
{
	exynos_abox_panic_handler();
	return NOTIFY_OK;
}

static struct notifier_block abox_panic_notifier = {
	.notifier_call	= abox_panic_handler,
	.next		= NULL,
	.priority	= 0	/* priority: INT_MAX >= x >= 0 */
};

static void *abox_addr_to_kernel_addr(struct abox_data *data, unsigned int addr)
{
	void *result;

	if (addr < IOVA_DRAM_FIRMWARE)
		result = data->sram_base + addr;
	else if (addr >= IOVA_DRAM_FIRMWARE && addr < IOVA_IVA_FIRMWARE)
		result = data->dram_base + (addr - IOVA_DRAM_FIRMWARE);
	else if (addr >= IOVA_IVA_FIRMWARE && addr < IOVA_VSS_FIRMWARE)
		result = data->iva_base + (addr - IOVA_IVA_FIRMWARE);
	else if (addr >= IOVA_VSS_FIRMWARE && addr <  IOVA_DUMP_BUFFER)
		result = phys_to_virt(shm_get_phys_base() + shm_get_cp_size()) +
				(addr - IOVA_VSS_FIRMWARE);
	else
		result = data->dump_base + (addr - IOVA_DUMP_BUFFER);

	return result;
}

static phys_addr_t abox_addr_to_phys_addr(struct abox_data *data,
		unsigned int addr)
{
	phys_addr_t result;

	if (addr < IOVA_DRAM_FIRMWARE)
		result = data->sram_base_phys + addr;
	else if (addr >= IOVA_DRAM_FIRMWARE && addr < IOVA_IVA_FIRMWARE)
		result = data->dram_base_phys + (addr - IOVA_DRAM_FIRMWARE);
	else if (addr >= IOVA_IVA_FIRMWARE && addr < IOVA_VSS_FIRMWARE)
		result = data->iva_base_phys + (addr - IOVA_IVA_FIRMWARE);
	else if (addr >= IOVA_VSS_FIRMWARE && addr <  IOVA_DUMP_BUFFER)
		result = shm_get_phys_base() + shm_get_cp_size() +
				(addr - IOVA_VSS_FIRMWARE);
	else
		result = data->dump_base_phys + (addr - IOVA_DUMP_BUFFER);

	return result;
}

static unsigned int abox_get_out_rate(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	return data->out_rate[configmsg];
}

u32 abox_mailbox_read(struct device *dev, struct abox_data *data,
		unsigned int index)
{
	void __iomem *mailbox_base = data->sram_base + data->mailbox_offset;
	unsigned int result = 0;
	unsigned long flag;

	dev_dbg(dev, "%s(0x%x)\n", __func__, index);

	spin_lock_irqsave(&data->ipc_spinlock, flag);
	switch (data->calliope_state) {
	case CALLIOPE_ENABLING:
	case CALLIOPE_DISABLING:
	case CALLIOPE_ENABLED:
		break;
	default:
		dev_warn(dev, "Invalid calliope state: %d\n",
				data->calliope_state);
		goto out;
	}
	result = readl(mailbox_base + index);
out:
	spin_unlock_irqrestore(&data->ipc_spinlock, flag);

	return result;
}

void abox_mailbox_write(struct device *dev, struct abox_data *data,
		u32 index, u32 value)
{
	void __iomem *mailbox_base = data->sram_base + data->mailbox_offset;
	unsigned long flag;

	dev_dbg(dev, "%s(0x%x, 0x%x)\n", __func__, index, value);

	spin_lock_irqsave(&data->ipc_spinlock, flag);
	switch (data->calliope_state) {
	case CALLIOPE_ENABLING:
	case CALLIOPE_DISABLING:
	case CALLIOPE_ENABLED:
		break;
	default:
		dev_warn(dev, "Invalid calliope state: %d\n",
				data->calliope_state);
		goto out;
	}
	writel(value, mailbox_base + index);
out:
	spin_unlock_irqrestore(&data->ipc_spinlock, flag);
}

static int abox_start_ipc_transaction_atomic(struct device *dev,
		int hw_irq, const void *supplement,
		size_t size, int sync)
{
	struct abox_data *data = dev_get_drvdata(dev);
	void __iomem *tx_sram_base = data->sram_base + data->ipc_tx_offset;
	void __iomem *tx_ack_sram_base = data->sram_base +
			data->ipc_tx_ack_offset;
	int i;
	long result = 0;
	unsigned long flag;

	dev_dbg(dev, "%s(%d, %zu, %d)\n", __func__, hw_irq, size, sync);

	if (pm_runtime_suspended(dev)) {
		dev_info(dev, "%s: abox has been suspended\n", __func__);
		goto out;
	}

	spin_lock_irqsave(&data->ipc_spinlock, flag);

	switch (data->calliope_state) {
	case CALLIOPE_ENABLING:
		spin_unlock_irqrestore(&data->ipc_spinlock, flag);
		for (i = CALLIOPE_ENABLE_TIMEOUT_MS; i &&
				(data->calliope_state != CALLIOPE_ENABLED);
				i--) {
			mdelay(1);
		}
		spin_lock_irqsave(&data->ipc_spinlock, flag);
		if (data->calliope_state == CALLIOPE_ENABLED)
			break;
		/* Fallthrough */
	case CALLIOPE_DISABLED:
		dev_warn(dev, "IPC request when calliope is not ready: %d\n",
				data->calliope_state);
		result = -EAGAIN;
		goto unlock;
	case CALLIOPE_DISABLING:
	case CALLIOPE_ENABLED:
		break;
	default:
		dev_warn(dev, "Invalid calliope state: %d\n",
				data->calliope_state);
		break;
	}

	memcpy(tx_sram_base, supplement, size);
	writel(1, tx_ack_sram_base);
	abox_gic_generate_interrupt(data->pdev_gic, hw_irq);

	if (!sync)
		goto unlock;

	if (likely(data->calliope_version > CALLIOPE_VERSION('A', 1, 1, 0))) {

		for (i = 1000; i && readl(tx_ack_sram_base); i--)
			udelay(1);
		if (!i) {
			dev_warn_ratelimited(dev, "Transaction timeout\n");
		}
		result = readl(tx_ack_sram_base) ? -EIO : 0;
	} else {
		mdelay(1);
	}
unlock:
	spin_unlock_irqrestore(&data->ipc_spinlock, flag);
out:
	return (int)result;
}

int abox_request_ipc(struct device *dev,
		int hw_irq, const void *supplement,
		size_t size, int atomic, int sync)
{
	/* Use atomic sync IPC only */
	return abox_start_ipc_transaction_atomic(
			dev, hw_irq, supplement, size, 1);
}
EXPORT_SYMBOL(abox_request_ipc);

static void abox_process_ipc(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data, ipc_work);
	void __iomem *tx_sram_base = data->sram_base + data->ipc_tx_offset;
	long result = 0;

	enum ipc_state *state = &data->ipc_state;

	while (data->ipc_queue_start != data->ipc_queue_end) {
		struct abox_ipc *ipc = &data->ipc_queue[data->ipc_queue_start];
		struct device *dev = ipc->dev;
		int hw_irq = ipc->hw_irq;
		const unsigned char *supplement = ipc->supplement;
		size_t size = ipc->size;

		dev_dbg(dev, "%s(%d, %zu)\n", __func__, hw_irq, size);

		*state = SEND_MSG;
		memcpy(tx_sram_base, supplement, size);
		abox_gic_generate_interrupt(data->pdev_gic, hw_irq);
		result = wait_event_timeout(data->ipc_wait_queue,
				(*state == SEND_MSG_OK ||
				*state == SEND_MSG_FAIL),
				LIMIT_IN_JIFFIES);
		if (result > 0) {
			if (*state == SEND_MSG_OK)
				dev_dbg(dev, "Transaction success\n");
			else
				dev_err(dev, "Transaction failed\n");
		} else {
			dev_err(dev, "Transaction timeout or interrupted: %ld\n",
					result);
		}
		result = (*state == SEND_MSG_OK) ? 0 : -EIO;

		*state = IDLE;

		data->ipc_queue_start = (data->ipc_queue_start + 1 <
				ARRAY_SIZE(data->ipc_queue)) ?
				(data->ipc_queue_start + 1) : (0);
	}


}

int abox_schedule_ipc(struct device *dev, struct abox_data *data,
		int hw_irq, const void *supplement, size_t size)
{
	struct abox_ipc *ipc = &data->ipc_queue[data->ipc_queue_end];

	dev_dbg(dev, "%s\n", __func__);

	if (size > sizeof(ipc->supplement)) {
		dev_err(dev, "supplement is too large (%zu > %zu)\n",
				size, sizeof(ipc->supplement));
		return -EOVERFLOW;
	}

	if (data->ipc_queue_end == data->ipc_queue_start) {
		dev_warn(dev, "ipc queue overflow\n");
		return -EAGAIN;
	}

	ipc->dev = dev;
	ipc->hw_irq = hw_irq;
	memcpy(ipc->supplement, supplement, size);
	ipc->size = size;

	data->ipc_queue_end = (data->ipc_queue_end + 1 <
			ARRAY_SIZE(data->ipc_queue)) ?
			(data->ipc_queue_end + 1) : (0);

	schedule_work(&data->ipc_work);

	return 0;
}

bool abox_is_on(void)
{
	return p_abox_data && p_abox_data->enabled;
}
EXPORT_SYMBOL(abox_is_on);

static u32 get_sample_format(int bit_depth, int channels)
{
	u32 result = (channels - 1);

	switch (bit_depth) {
	case 16:
		result |= 1 << 3;
		break;
	case 24:
		result |= 2 << 3;
		break;
	case 32:
		result |= 3 << 3;
		break;
	default:
		break;
	}

	pr_debug("%s(%d, %d): %u\n", __func__, bit_depth, channels, result);

	return result;
}

static unsigned long abox_register_audif_rate(struct abox_data *data,
		enum abox_dai id, unsigned long rate)
{
	unsigned long ret = 0;
	unsigned int i;

	dev_dbg(&data->pdev->dev, "%s(%d, %lu)\n", __func__, id, rate);

	data->audif_rates[id] = rate;

	for (i = 0; i < ARRAY_SIZE(data->audif_rates); i++) {
		if (data->audif_rates[i] > 0 && data->audif_rates[i] > ret)
			ret = data->audif_rates[i];
	}

	return ret;
}

static void abox_unregister_audif_rates(struct abox_data *data, int id)
{
	data->audif_rates[id] = 0;
}

static int abox_uaif_control_bclk_polarity(struct snd_soc_dai *dai, bool set)
{
	struct device *dev = dai->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = dai->codec;
	enum abox_dai id = dai->id;
	unsigned int fmt = data->uaif_fmt[id];
	unsigned int ctrl1;

	if (dai->active)
		return 0;

	dev_info(dev, "%s(%s, %d)\n", __func__, dai->name, set);

	ctrl1 = snd_soc_read(codec, ABOX_UAIF_CTRL1(id));

	if (set) {
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
		case SND_SOC_DAIFMT_NB_IF:
			set_mask_value(ctrl1, ABOX_BCLK_POLARITY_MASK,
					1 << ABOX_BCLK_POLARITY_L);
			break;
		case SND_SOC_DAIFMT_IB_NF:
		case SND_SOC_DAIFMT_IB_IF:
			set_mask_value(ctrl1, ABOX_BCLK_POLARITY_MASK,
					0 << ABOX_BCLK_POLARITY_L);
			break;
		default:
			break;
		}
	} else {
		/* set ctrl1 BCLK_POLARITY 0 for idle power consumption */
		set_mask_value(ctrl1, ABOX_BCLK_POLARITY_MASK,
				0 << ABOX_BCLK_POLARITY_L);
	}

	return snd_soc_write(codec, ABOX_UAIF_CTRL1(id), ctrl1);
}

static int abox_uaif_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct device *dev = dai->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = dai->codec;
	enum abox_dai id = dai->id;
	unsigned int ctrl0, ctrl1;
	int result = 0;

	dev_info(dev, "%s[%d](0x%08x)\n", __func__, dai->id, fmt);

	data->uaif_fmt[id] = fmt;

	pm_runtime_get_sync(dev);

	ctrl0 = snd_soc_read(codec, ABOX_UAIF_CTRL0(id));
	ctrl1 = snd_soc_read(codec, ABOX_UAIF_CTRL1(id));

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		set_mask_value(ctrl1, ABOX_WS_MODE_MASK, 0 << ABOX_WS_MODE_L);
		break;
	case SND_SOC_DAIFMT_DSP_A:
		set_mask_value(ctrl1, ABOX_WS_MODE_MASK, 1 << ABOX_WS_MODE_L);
		break;
	default:
		result = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		set_mask_value(ctrl1, ABOX_BCLK_POLARITY_MASK,
				1 << ABOX_BCLK_POLARITY_L);
		set_mask_value(ctrl1, ABOX_WS_POLAR_MASK, 0 << ABOX_WS_POLAR_L);
		break;
	case SND_SOC_DAIFMT_NB_IF:
		set_mask_value(ctrl1, ABOX_BCLK_POLARITY_MASK,
				1 << ABOX_BCLK_POLARITY_L);
		set_mask_value(ctrl1, ABOX_WS_POLAR_MASK, 1 << ABOX_WS_POLAR_L);
		break;
	case SND_SOC_DAIFMT_IB_NF:
		set_mask_value(ctrl1, ABOX_BCLK_POLARITY_MASK,
				0 << ABOX_BCLK_POLARITY_L);
		set_mask_value(ctrl1, ABOX_WS_POLAR_MASK,
				0 << ABOX_WS_POLAR_L);
		break;
	case SND_SOC_DAIFMT_IB_IF:
		set_mask_value(ctrl1, ABOX_BCLK_POLARITY_MASK,
				0 << ABOX_BCLK_POLARITY_L);
		set_mask_value(ctrl1, ABOX_WS_POLAR_MASK, 1 << ABOX_WS_POLAR_L);
		break;
	default:
		result = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		set_mask_value(ctrl0, ABOX_MODE_MASK, 1 << ABOX_MODE_L);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		set_mask_value(ctrl0, ABOX_MODE_MASK, 0 << ABOX_MODE_L);
		break;
	default:
		result = -EINVAL;
	}

	/* set ctrl1 BCLK_POLARITY 0 for idle power consumption */
	set_mask_value(ctrl1, ABOX_BCLK_POLARITY_MASK,
			0 << ABOX_BCLK_POLARITY_L);

	snd_soc_write(codec, ABOX_UAIF_CTRL0(id), ctrl0);
	snd_soc_write(codec, ABOX_UAIF_CTRL1(id), ctrl1);

	pm_runtime_put(dev);

	return result;
}

static int abox_uaif_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_data *data = platform_get_drvdata(to_platform_device(dev));
	enum abox_dai id = dai->id;
	int result;

	dev_info(dev, "%s[%d:%c]\n", __func__, id,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P');

	pm_runtime_get_sync(dev);
	abox_uaif_control_bclk_polarity(dai, true);
	abox_request_cpu_gear_sync_dai(dev, data, dai, 3);
	result = clk_enable(data->clk_bclk[id]);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Failed to enable bclk: %d\n", result);
		goto err;
	}
	result = clk_enable(data->clk_bclk_gate[id]);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Failed to enable bclk_gate: %d\n", result);
		goto err;
	}
err:
	return result;
}

static void abox_uaif_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_data *data = platform_get_drvdata(to_platform_device(dev));
	enum abox_dai id = dai->id;
	struct snd_soc_codec *codec = dai->codec;

	dev_info(dev, "%s[%d:%c]\n", __func__, id,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P');

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_update_bits(codec, ABOX_UAIF_CTRL0(id),
				ABOX_SPK_ENABLE_MASK, 0 << ABOX_SPK_ENABLE_L);
	}

	clk_disable(data->clk_bclk_gate[id]);
	clk_disable(data->clk_bclk[id]);
	abox_unregister_audif_rates(data, id);
	abox_request_cpu_gear_dai(dev, data, dai, 12);
	abox_uaif_control_bclk_polarity(dai, false);
	pm_runtime_put(dev);
}

static int abox_uaif_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_data *data = platform_get_drvdata(to_platform_device(dev));
	struct snd_soc_codec *codec = dai->codec;
	enum abox_dai id = dai->id;
	unsigned int ctrl1;
	unsigned int channels, rate, width;
	unsigned long audif_rate;
	unsigned long target_pll;
	int result;

	dev_info(dev, "%s[%d:%c]\n", __func__, dai->id,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P');

	ctrl1 = snd_soc_read(codec, ABOX_UAIF_CTRL1(id));

	channels = params_channels(hw_params);
	rate = params_rate(hw_params);
	width = params_width(hw_params);

	target_pll = ((rate % 44100) == 0) ? AUD_PLL_RATE_HZ_FOR_44100 :
			AUD_PLL_RATE_HZ_FOR_48000;
	if (target_pll != clk_get_rate(data->clk_pll)) {
		dev_info(dev, "Set AUD_PLL rate: %lu -> %lu\n",
			clk_get_rate(data->clk_pll), target_pll);
		result = clk_set_rate(data->clk_pll, target_pll);
		if (IS_ERR_VALUE(result)) {
			dev_err(dev, "AUD_PLL set error=%d\n", result);
			return result;
		}
	}

	if ((rate % 44100) == 0)
		audif_rate = ((rate > 176400) ? 352800 : 176400) * width * 2;
	else
		audif_rate = ((rate > 192000) ? 384000 : 192000) * width * 2;

	while (audif_rate / rate / channels / width > 32)
		audif_rate /= 2;

	audif_rate = abox_register_audif_rate(data, id, audif_rate);
	result = clk_set_rate(data->clk_audif, audif_rate);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Failed to set audif clock: %d\n", result);
		return result;
	}
	dev_info(dev, "audif clock: %lu\n", clk_get_rate(data->clk_audif));

	result = clk_set_rate(data->clk_bclk[id], rate * channels * width);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "bclk set error=%d\n", result);
		return result;
	}

	dev_info(dev, "rate=%u, width=%d, channel=%u, bclk=%lu\n",
			rate,
			width,
			channels,
			clk_get_rate(data->clk_bclk[id]));

	switch (params_format(hw_params)) {
	case SNDRV_PCM_FORMAT_S16:
	case SNDRV_PCM_FORMAT_S24:
	case SNDRV_PCM_FORMAT_S32:
		set_mask_value(ctrl1, ABOX_SBIT_MAX_MASK,
				(width - 1) << ABOX_SBIT_MAX_L);
		break;
	default:
		return -EINVAL;
	}

	switch (channels) {
	case 2:
		set_mask_value(ctrl1, ABOX_VALID_STR_MASK, 0);
		set_mask_value(ctrl1, ABOX_VALID_END_MASK, 0);
		break;
	case 1:
	case 4:
	case 6:
	case 8:
		set_mask_value(ctrl1, ABOX_VALID_STR_MASK,
				(width - 1) << ABOX_VALID_STR_L);
		set_mask_value(ctrl1, ABOX_VALID_END_MASK,
				(width - 1) << ABOX_VALID_END_L);
		break;
	default:
		return -EINVAL;
	}
	set_mask_value(ctrl1, ABOX_SLOT_MAX_MASK,
			(channels - 1) << ABOX_SLOT_MAX_L);
	set_mask_value(ctrl1, ABOX_FORMAT_MASK,
			get_sample_format(width, channels) << ABOX_FORMAT_L);

	snd_soc_write(codec, ABOX_UAIF_CTRL1(id), ctrl1);

	return 0;
}

static int abox_uaif_trigger(struct snd_pcm_substream *substream,
		int trigger, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct snd_soc_codec *codec = dai->codec;
	enum abox_dai id = dai->id;

	dev_info(dev, "%s[%d:%c] trigger=%d\n", __func__, dai->id,
			(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ?
			'C' : 'P', trigger);

	switch (trigger) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			snd_soc_update_bits(codec, ABOX_UAIF_CTRL0(id),
					ABOX_MIC_ENABLE_MASK,
					1 << ABOX_MIC_ENABLE_L);
		} else {
			snd_soc_update_bits(codec, ABOX_UAIF_CTRL0(id),
					ABOX_SPK_ENABLE_MASK,
					1 << ABOX_SPK_ENABLE_L);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			snd_soc_update_bits(codec, ABOX_UAIF_CTRL0(id),
					ABOX_MIC_ENABLE_MASK,
					0 << ABOX_MIC_ENABLE_L);
		}
		/* disable DAI on shutdown to suppress pop noise */
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int abox_output_format_put_ipc(struct device *dev,
		enum ABOX_CONFIGMSG configmsg, int bit_depth, int channels)
{
	ABOX_IPC_MSG msg;
	struct IPC_ABOX_CONFIG_MSG *abox_config_msg = &msg.msg.config;
	int result;

	dev_dbg(dev, "%s(%d, %d, %d)\n", __func__, configmsg, bit_depth,
			channels);

	msg.ipcid = IPC_ABOX_CONFIG;
	abox_config_msg->param1 = bit_depth;
	abox_config_msg->param2 = channels;
	abox_config_msg->msgtype = configmsg;
	result = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "%d: setting format(%d bit, %d channels) is failed\n",
				configmsg, bit_depth, channels);
	}

	return result;
}

static const struct snd_soc_dai_ops abox_uaif_dai_ops = {
	.set_fmt	= abox_uaif_set_fmt,
	.startup	= abox_uaif_startup,
	.shutdown	= abox_uaif_shutdown,
	.hw_params	= abox_uaif_hw_params,
	.trigger	= abox_uaif_trigger,
};

static int abox_dsif_set_bclk_ratio(struct snd_soc_dai *dai, unsigned int ratio)
{
	struct device *dev = dai->dev;
	struct abox_data *data = platform_get_drvdata(to_platform_device(dev));
	enum abox_dai id = dai->id;
	unsigned long rate;
	unsigned long target_pll;
	int result;

	dev_info(dev, "%s[%d]\n", __func__, dai->id);

	rate = dai->rate;

	target_pll = ((rate % 44100) == 0) ? AUD_PLL_RATE_HZ_FOR_44100 :
			AUD_PLL_RATE_HZ_FOR_48000;
	if (target_pll != clk_get_rate(data->clk_pll)) {
		dev_info(dev, "Set AUD_PLL rate: %lu -> %lu\n",
			clk_get_rate(data->clk_pll), target_pll);
		result = clk_set_rate(data->clk_pll, target_pll);
		if (IS_ERR_VALUE(result)) {
			dev_err(dev, "AUD_PLL set error=%d\n", result);
			return result;
		}
	}

	result = clk_set_rate(data->clk_bclk[id], rate * ratio);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "bclk set error=%d\n", result);
	} else {
		dev_info(dev, "rate=%lu, bclk=%lu\n",
				rate, clk_get_rate(data->clk_bclk[id]));
	}

	return result;
}

static int abox_dsif_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct device *dev = dai->dev;
	struct snd_soc_codec *codec = dai->codec;
	unsigned int ctrl;
	int result = 0;

	dev_info(dev, "%s[%d](0x%08x)\n", __func__, dai->id, fmt);

	pm_runtime_get_sync(dev);

	ctrl = snd_soc_read(codec, ABOX_DSIF_CTRL);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_PDM:
		break;
	default:
		result = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
	case SND_SOC_DAIFMT_NB_IF:
		set_mask_value(ctrl, ABOX_DSIF_BCLK_POLARITY_MASK,
				1 << ABOX_DSIF_BCLK_POLARITY_L);
		break;
	case SND_SOC_DAIFMT_IB_NF:
	case SND_SOC_DAIFMT_IB_IF:
		set_mask_value(ctrl, ABOX_DSIF_BCLK_POLARITY_MASK,
				0 << ABOX_DSIF_BCLK_POLARITY_L);
		break;
	default:
		result = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		break;
	default:
		result = -EINVAL;
	}

	snd_soc_write(codec, ABOX_DSIF_CTRL, ctrl);

	pm_runtime_put(dev);

	return result;
}

static int abox_dsif_set_channel_map(struct snd_soc_dai *dai,
		unsigned int tx_num, unsigned int *tx_slot,
		unsigned int rx_num, unsigned int *rx_slot)
{
	struct device *dev = dai->dev;
	struct snd_soc_codec *codec = dai->codec;
	unsigned int ctrl;

	dev_info(dev, "%s[%d]\n", __func__, dai->id);

	ctrl = snd_soc_read(codec, ABOX_DSIF_CTRL);

	if (tx_slot[0])
		set_mask_value(ctrl, ABOX_ORDER_MASK, 1 << ABOX_ORDER_L);
	else
		set_mask_value(ctrl, ABOX_ORDER_MASK, 0 << ABOX_ORDER_L);

	snd_soc_write(codec, ABOX_DSIF_CTRL, ctrl);

	return 0;
}

static int abox_dsif_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_data *data = platform_get_drvdata(to_platform_device(dev));
	enum abox_dai id = dai->id;
	int result;

	dev_info(dev, "%s[%d]\n", __func__, id);

	pm_runtime_get_sync(dev);
	abox_request_cpu_gear_sync_dai(dev, data, dai, 3);
	result = clk_enable(data->clk_bclk[id]);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Failed to enable bclk: %d\n", result);
		goto err;
	}
	result = clk_enable(data->clk_bclk_gate[id]);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Failed to enable bclk_gate: %d\n", result);
		goto err;
	}
err:
	return result;
}

static void abox_dsif_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_data *data = platform_get_drvdata(to_platform_device(dev));
	enum abox_dai id = dai->id;
	struct snd_soc_codec *codec = dai->codec;

	dev_info(dev, "%s[%d]\n", __func__, id);

	snd_soc_update_bits(codec, ABOX_DSIF_CTRL, ABOX_ENABLE_MASK,
			0 << ABOX_ENABLE_L);

	clk_disable(data->clk_bclk_gate[id]);
	clk_disable(data->clk_bclk[id]);
	abox_request_cpu_gear_dai(dev, data, dai, 12);
	pm_runtime_put(dev);
}

static int abox_dsif_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct abox_data *data = platform_get_drvdata(to_platform_device(dev));
	enum abox_dai id = dai->id;
	unsigned int channels, rate, width;

	dev_info(dev, "%s[%d]\n", __func__, dai->id);

	channels = params_channels(hw_params);
	rate = params_rate(hw_params);
	width = params_width(hw_params);

	dev_info(dev, "rate=%u, width=%d, channel=%u, bclk=%lu\n",
			rate,
			width,
			channels,
			clk_get_rate(data->clk_bclk[id]));

	switch (params_format(hw_params)) {
	case SNDRV_PCM_FORMAT_S32:
		break;
	default:
		return -EINVAL;
	}

	switch (channels) {
	case 2:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int abox_dsif_trigger(struct snd_pcm_substream *substream,
		int trigger, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct snd_soc_codec *codec = dai->codec;
	enum abox_dai id = dai->id;

	dev_info(dev, "%s[%d] trigger=%d\n", __func__, id, trigger);

	switch (trigger) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		snd_soc_update_bits(codec, ABOX_DSIF_CTRL, ABOX_ENABLE_MASK,
				1 << ABOX_ENABLE_L);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* disable DAI on shutdown to suppress pop noise */
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops abox_dsif_dai_ops = {
	.set_bclk_ratio = abox_dsif_set_bclk_ratio,
	.set_fmt = abox_dsif_set_fmt,
	.set_channel_map = abox_dsif_set_channel_map,
	.startup = abox_dsif_startup,
	.shutdown = abox_dsif_shutdown,
	.hw_params = abox_dsif_hw_params,
	.trigger = abox_dsif_trigger,
};

static struct snd_soc_dai_driver abox_dais[] = {
	{
		.name = "UAIF0",
		.id = ABOX_UAIF0,
		.playback = {
			.stream_name = "UAIF0 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "UAIF0 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &abox_uaif_dai_ops,
		.symmetric_rates = 1,
		.symmetric_channels = 1,
		.symmetric_samplebits = 1,
	},
	{
		.name = "UAIF1",
		.id = ABOX_UAIF1,
		.playback = {
			.stream_name = "UAIF1 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		 .capture = {
			.stream_name = "UAIF1 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &abox_uaif_dai_ops,
		.symmetric_rates = 1,
		.symmetric_channels = 1,
		.symmetric_samplebits = 1,
	},
	{
		.name = "UAIF2",
		.id = ABOX_UAIF2,
		.playback = {
			.stream_name = "UAIF2 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		 .capture = {
			.stream_name = "UAIF2 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &abox_uaif_dai_ops,
		.symmetric_rates = 1,
		.symmetric_channels = 1,
		.symmetric_samplebits = 1,
	},
	{
		.name = "UAIF3",
		.id = ABOX_UAIF3,
		.playback = {
			.stream_name = "UAIF3 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		 .capture = {
			.stream_name = "UAIF3 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &abox_uaif_dai_ops,
		.symmetric_rates = 1,
		.symmetric_channels = 1,
		.symmetric_samplebits = 1,
	},
	{
		.name = "UAIF4",
		.id = ABOX_UAIF4,
		.playback = {
			.stream_name = "UAIF4 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		 .capture = {
			.stream_name = "UAIF4 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.ops = &abox_uaif_dai_ops,
		.symmetric_rates = 1,
		.symmetric_channels = 1,
		.symmetric_samplebits = 1,
	},
	{
		.name = "DSIF",
		.id = ABOX_DSIF,
		.playback = {
			.stream_name = "DSIF Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = SNDRV_PCM_FMTBIT_S32,
		},
		.ops = &abox_dsif_dai_ops,
	},
	{
		.name = "RDMA0",
		.id = ABOX_RDMA0,
		.playback = {
			.stream_name = "RDMA0 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
	{
		.name = "RDMA1",
		.id = ABOX_RDMA1,
		.playback = {
			.stream_name = "RDMA1 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
	{
		.name = "RDMA2",
		.id = ABOX_RDMA2,
		.playback = {
			.stream_name = "RDMA2 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
	{
		.name = "RDMA3",
		.id = ABOX_RDMA3,
		.playback = {
			.stream_name = "RDMA3 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
	{
		.name = "RDMA4",
		.id = ABOX_RDMA4,
		.playback = {
			.stream_name = "RDMA4 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
	{
		.name = "RDMA5",
		.id = ABOX_RDMA5,
		.playback = {
			.stream_name = "RDMA5 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.compress_new = snd_soc_new_compress,
	},
	{
		.name = "RDMA6",
		.id = ABOX_RDMA6,
		.playback = {
			.stream_name = "RDMA6 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
	{
		.name = "RDMA7",
		.id = ABOX_RDMA7,
		.playback = {
			.stream_name = "RDMA7 Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
	{
		.name = "WDMA0",
		.id = ABOX_WDMA0,
		.capture = {
			.stream_name = "WDMA0 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_WDMA_SAMPLE_FORMATS,
		},
	},
	{
		.name = "WDMA1",
		.id = ABOX_WDMA1,
		.capture = {
			.stream_name = "WDMA1 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_WDMA_SAMPLE_FORMATS,
		},
	},
	{
		.name = "WDMA2",
		.id = ABOX_WDMA2,
		.capture = {
			.stream_name = "WDMA2 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_WDMA_SAMPLE_FORMATS,
		},
	},
	{
		.name = "WDMA3",
		.id = ABOX_WDMA3,
		.capture = {
			.stream_name = "WDMA3 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_WDMA_SAMPLE_FORMATS,
		},
	},
	{
		.name = "WDMA4",
		.id = ABOX_WDMA4,
		.capture = {
			.stream_name = "WDMA4 Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_WDMA_SAMPLE_FORMATS,
		},
	},
	{
		.name = "Internal",
		.playback = {
			.stream_name = "Internal Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "Internal Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	}
};

static int abox_codec_suspend(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "%s\n", __func__);

	return 0;
}

static int abox_codec_resume(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "%s\n", __func__);

	return 0;
}

static int abox_codec_probe(struct snd_soc_codec *codec)
{
	struct abox_data *data = dev_get_drvdata(codec->dev);

	dev_info(codec->dev, "%s\n", __func__);

	data->codec = codec;

	snd_soc_update_bits(codec, ABOX_SPUM_CTRL0,
			ABOX_FUNC_CHAIN_RSRC_RECP_MASK,
			ABOX_FUNC_CHAIN_RSRC_RECP_MASK);

	return 0;
}

static int abox_codec_remove(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "%s\n", __func__);

	return 0;
}

static int abox_output_rate_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol,
		enum ABOX_CONFIGMSG configmsg)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s(%d)\n", __func__, configmsg);

	ucontrol->value.integer.value[0] = data->out_rate[configmsg];

	return 0;
}

static int abox_output_rate_put_ipc(struct device *dev, unsigned int val,
		enum ABOX_CONFIGMSG configmsg)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_ABOX_CONFIG_MSG *abox_config_msg = &msg.msg.config;
	int result;

	dev_dbg(dev, "%s(%u, %d)\n", __func__, val, configmsg);

	msg.ipcid = IPC_ABOX_CONFIG;
	abox_config_msg->param1 = val;
	abox_config_msg->msgtype = configmsg;
	result = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "setting %u to sample rate %d is failed\n",
				val, configmsg);
	}

	data->out_rate[abox_config_msg->msgtype] = abox_config_msg->param1;

	return result;
}

static int abox_output_rate_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol,
		enum ABOX_CONFIGMSG configmsg)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%u, %d)\n", __func__, val, configmsg);

	pm_runtime_barrier(dev);
	return abox_output_rate_put_ipc(dev, val, configmsg);
}

static int abox_output_rate_get_mixer(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_get(kcontrol, ucontrol, SET_MIXER_SAMPLE_RATE);
}

static int abox_output_rate_get_out1(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_get(kcontrol, ucontrol, SET_OUT1_SAMPLE_RATE);
}

static int abox_output_rate_get_out2(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_get(kcontrol, ucontrol, SET_OUT2_SAMPLE_RATE);
}

static int abox_output_rate_get_recp(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_get(kcontrol, ucontrol, SET_RECP_SAMPLE_RATE);
}

static int abox_output_rate_get_inmux0(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_get(kcontrol, ucontrol, SET_INMUX0_SAMPLE_RATE);
}

static int abox_output_rate_get_inmux1(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_get(kcontrol, ucontrol, SET_INMUX1_SAMPLE_RATE);
}

static int abox_output_rate_get_inmux2(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_get(kcontrol, ucontrol, SET_INMUX2_SAMPLE_RATE);
}

static int abox_output_rate_get_inmux3(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_get(kcontrol, ucontrol, SET_INMUX3_SAMPLE_RATE);
}

static int abox_output_rate_get_inmux4(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_get(kcontrol, ucontrol, SET_INMUX4_SAMPLE_RATE);
}

static int abox_output_rate_put_mixer(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_put(kcontrol, ucontrol, SET_MIXER_SAMPLE_RATE);
}

static int abox_output_rate_put_out1(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_put(kcontrol, ucontrol, SET_OUT1_SAMPLE_RATE);
}

static int abox_output_rate_put_out2(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_put(kcontrol, ucontrol, SET_OUT2_SAMPLE_RATE);
}

static int abox_output_rate_put_recp(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_put(kcontrol, ucontrol, SET_RECP_SAMPLE_RATE);
}

static int abox_output_rate_put_inmux0(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_put(kcontrol, ucontrol, SET_INMUX0_SAMPLE_RATE);
}

static int abox_output_rate_put_inmux1(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_put(kcontrol, ucontrol, SET_INMUX1_SAMPLE_RATE);
}

static int abox_output_rate_put_inmux2(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_put(kcontrol, ucontrol, SET_INMUX2_SAMPLE_RATE);
}

static int abox_output_rate_put_inmux3(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_put(kcontrol, ucontrol, SET_INMUX3_SAMPLE_RATE);
}

static int abox_output_rate_put_inmux4(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_output_rate_put(kcontrol, ucontrol, SET_INMUX4_SAMPLE_RATE);
}


static int abox_auto_output_rate_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	long value = data->out_rate_auto[reg];

	dev_dbg(dev, "%s(0x%08x): %ld\n", __func__, reg, value);

	ucontrol->value.integer.value[0] = value;

	return 0;
}

static int abox_auto_output_rate_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	long value = ucontrol->value.integer.value[0];

	dev_info(dev, "%s(0x%08x, %ld)\n", __func__, reg, value);

	data->out_rate_auto[reg] = value ? true : false;

	return 0;
}

static int abox_synchronize_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol, int id)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s(%d)\n", __func__, id);

	ucontrol->value.integer.value[0] = data->rdma_synchronizer[id];

	return 0;
}

static int abox_synchronize_put_ipc(struct device *dev,
		unsigned int val, int id)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;
	int result;

	dev_dbg(dev, "%s(%u, %d)\n", __func__, val, id);

	msg.ipcid = IPC_PCMPLAYBACK;
	pcmtask_msg->msgtype = PCM_SYNCHRONIZE;
	pcmtask_msg->param.synchronize = val;
	pcmtask_msg->channel_id = id;
	result = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "synchronize rdma %u to rdma %d is failed\n",
				id, val);
	}

	data->rdma_synchronizer[id] = val;

	return result;
}

static int abox_synchronize_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol, int id)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_info(dev, "%s(%u, %d)\n", __func__, val, id);

	pm_runtime_barrier(dev);
	return abox_synchronize_put_ipc(dev, val, id);
}

static int abox_synchronize_get_rdma0(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_get(kcontrol, ucontrol, 0);
}

static int abox_synchronize_get_rdma1(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_get(kcontrol, ucontrol, 1);
}

static int abox_synchronize_get_rdma2(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_get(kcontrol, ucontrol, 2);
}

static int abox_synchronize_get_rdma3(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_get(kcontrol, ucontrol, 3);
}

static int abox_synchronize_get_rdma4(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_get(kcontrol, ucontrol, 4);
}

static int abox_synchronize_get_rdma5(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_get(kcontrol, ucontrol, 5);
}

static int abox_synchronize_get_rdma6(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_get(kcontrol, ucontrol, 6);
}

static int abox_synchronize_get_rdma7(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_get(kcontrol, ucontrol, 7);
}

static int abox_synchronize_put_rdma0(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_put(kcontrol, ucontrol, 0);
}

static int abox_synchronize_put_rdma1(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_put(kcontrol, ucontrol, 1);
}

static int abox_synchronize_put_rdma2(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_put(kcontrol, ucontrol, 2);
}

static int abox_synchronize_put_rdma3(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_put(kcontrol, ucontrol, 3);
}

static int abox_synchronize_put_rdma4(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_put(kcontrol, ucontrol, 4);
}

static int abox_synchronize_put_rdma5(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_put(kcontrol, ucontrol, 5);
}

static int abox_synchronize_put_rdma6(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_put(kcontrol, ucontrol, 6);
}

static int abox_synchronize_put_rdma7(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return abox_synchronize_put(kcontrol, ucontrol, 7);
}

static int abox_erap_handler_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;
	enum ABOX_ERAP_TYPE type = (enum ABOX_ERAP_TYPE)mc->reg;

	dev_dbg(dev, "%s(%d)\n", __func__, type);

	ucontrol->value.integer.value[0] = data->erap_status[type];

	return 0;
}

static int abox_erap_handler_put_ipc(struct device *dev,
		enum ABOX_ERAP_TYPE type, unsigned int val)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;
	struct ERAP_ONOFF_PARAM *erap_param = &erap_msg->param.onoff;
	int result;

	dev_dbg(dev, "%s(%u, %d)\n", __func__, val, type);

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = val ? REALTIME_OPEN : REALTIME_CLOSE;
	erap_param->type = type;
	erap_param->channel_no = 0;
	erap_param->version = val;
	result = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "erap control failed(type:%d, status:%d)\n",
				type, val);
	}

	data->erap_status[type] = val;

	return result;
}

static int abox_erap_handler_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;
	enum ABOX_ERAP_TYPE type = (enum ABOX_ERAP_TYPE)mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	dev_dbg(dev, "%s(%u, %d)\n", __func__, val, type);

	pm_runtime_barrier(dev);
	return abox_erap_handler_put_ipc(dev, type, val);
}

static int abox_audio_mode_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int item;

	dev_dbg(dev, "%s: %u\n", __func__, data->audio_mode);

	item = snd_soc_enum_val_to_item(e, data->audio_mode);
	ucontrol->value.enumerated.item[0] = item;

	return 0;
}

static int abox_audio_mode_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;

	if (item[0] >= e->items)
		return -EINVAL;

	data->audio_mode_time = local_clock();
	data->audio_mode = snd_soc_enum_item_to_val(e, item[0]);

	dev_info(dev, "%s(%u)\n", __func__, data->audio_mode);

	return 0;
}

static const char * const abox_audio_mode_enum_texts[] = {
	"NORMAL",
	"RINGTONE",
	"IN_CALL",
	"IN_COMMUNICATION",
	"IN_VIDEOCALL",
};
static const unsigned int abox_audio_mode_enum_values[] = {
	MODE_NORMAL,
	MODE_RINGTONE,
	MODE_IN_CALL,
	MODE_IN_COMMUNICATION,
	MODE_IN_VIDEOCALL,
};
SOC_VALUE_ENUM_SINGLE_DECL(abox_audio_mode_enum, SND_SOC_NOPM, 0, 0,
	abox_audio_mode_enum_texts, abox_audio_mode_enum_values);

static const struct snd_kcontrol_new abox_codec_controls[] = {
	SOC_SINGLE_EXT("Sampling Rate Mixer", SND_SOC_NOPM, 8000, 192000, 0,
		abox_output_rate_get_mixer, abox_output_rate_put_mixer),
	SOC_SINGLE_EXT("Sampling Rate Out1", SND_SOC_NOPM, 8000, 384000, 0,
		abox_output_rate_get_out1, abox_output_rate_put_out1),
	SOC_SINGLE_EXT("Sampling Rate Out2", SND_SOC_NOPM, 8000, 384000, 0,
		abox_output_rate_get_out2, abox_output_rate_put_out2),
	SOC_SINGLE_EXT("Sampling Rate Recp", SND_SOC_NOPM, 8000, 192000, 0,
		abox_output_rate_get_recp, abox_output_rate_put_recp),
	SOC_SINGLE_EXT("Sampling Rate Inmux0", SND_SOC_NOPM, 8000, 192000, 0,
		abox_output_rate_get_inmux0, abox_output_rate_put_inmux0),
	SOC_SINGLE_EXT("Sampling Rate Inmux1", SND_SOC_NOPM, 8000, 192000, 0,
		abox_output_rate_get_inmux1, abox_output_rate_put_inmux1),
	SOC_SINGLE_EXT("Sampling Rate Inmux2", SND_SOC_NOPM, 8000, 192000, 0,
		abox_output_rate_get_inmux2, abox_output_rate_put_inmux2),
	SOC_SINGLE_EXT("Sampling Rate Inmux3", SND_SOC_NOPM, 8000, 192000, 0,
		abox_output_rate_get_inmux3, abox_output_rate_put_inmux3),
	SOC_SINGLE_EXT("Sampling Rate Inmux4", SND_SOC_NOPM, 8000, 192000, 0,
		abox_output_rate_get_inmux4, abox_output_rate_put_inmux4),
	SOC_SINGLE_EXT("Sampling Rate Mixer Auto", SET_MIXER_SAMPLE_RATE,
		0, 1, 0, abox_auto_output_rate_get, abox_auto_output_rate_put),
	SOC_SINGLE_EXT("Sampling Rate Out1 Auto", SET_OUT1_SAMPLE_RATE,
		0, 1, 0, abox_auto_output_rate_get, abox_auto_output_rate_put),
	SOC_SINGLE_EXT("Sampling Rate Out2 Auto", SET_OUT2_SAMPLE_RATE,
		0, 1, 0, abox_auto_output_rate_get, abox_auto_output_rate_put),
	SOC_SINGLE_EXT("Sampling Rate Recp Auto", SET_RECP_SAMPLE_RATE,
		0, 1, 0, abox_auto_output_rate_get, abox_auto_output_rate_put),
	SOC_SINGLE_EXT("Sampling Rate Inmux0 Auto", SET_INMUX0_SAMPLE_RATE,
		0, 1, 0, abox_auto_output_rate_get, abox_auto_output_rate_put),
	SOC_SINGLE_EXT("Sampling Rate Inmux1 Auto", SET_INMUX1_SAMPLE_RATE,
		0, 1, 0, abox_auto_output_rate_get, abox_auto_output_rate_put),
	SOC_SINGLE_EXT("Sampling Rate Inmux2 Auto", SET_INMUX2_SAMPLE_RATE,
		0, 1, 0, abox_auto_output_rate_get, abox_auto_output_rate_put),
	SOC_SINGLE_EXT("Sampling Rate Inmux3 Auto", SET_INMUX3_SAMPLE_RATE,
		0, 1, 0, abox_auto_output_rate_get, abox_auto_output_rate_put),
	SOC_SINGLE_EXT("Sampling Rate Inmux4 Auto", SET_INMUX4_SAMPLE_RATE,
		0, 1, 0, abox_auto_output_rate_get, abox_auto_output_rate_put),
	SOC_SINGLE_EXT("Synchronize RDMA0", SND_SOC_NOPM, 0, 7, 0,
		abox_synchronize_get_rdma0, abox_synchronize_put_rdma0),
	SOC_SINGLE_EXT("Synchronize RDMA1", SND_SOC_NOPM, 0, 7, 0,
		abox_synchronize_get_rdma1, abox_synchronize_put_rdma1),
	SOC_SINGLE_EXT("Synchronize RDMA2", SND_SOC_NOPM, 0, 7, 0,
		abox_synchronize_get_rdma2, abox_synchronize_put_rdma2),
	SOC_SINGLE_EXT("Synchronize RDMA3", SND_SOC_NOPM, 0, 7, 0,
		abox_synchronize_get_rdma3, abox_synchronize_put_rdma3),
	SOC_SINGLE_EXT("Synchronize RDMA4", SND_SOC_NOPM, 0, 7, 0,
		abox_synchronize_get_rdma4, abox_synchronize_put_rdma4),
	SOC_SINGLE_EXT("Synchronize RDMA5", SND_SOC_NOPM, 0, 7, 0,
		abox_synchronize_get_rdma5, abox_synchronize_put_rdma5),
	SOC_SINGLE_EXT("Synchronize RDMA6", SND_SOC_NOPM, 0, 7, 0,
		abox_synchronize_get_rdma6, abox_synchronize_put_rdma6),
	SOC_SINGLE_EXT("Synchronize RDMA7", SND_SOC_NOPM, 0, 7, 0,
		abox_synchronize_get_rdma7, abox_synchronize_put_rdma7),
	SOC_SINGLE_EXT("Echo Cancellation", ERAP_ECHO_CANCEL, 0, 2, 0,
		abox_erap_handler_get, abox_erap_handler_put),
	SOC_SINGLE_EXT("VI Sensing", ERAP_VI_SENSE, 0, 2, 0,
		abox_erap_handler_get, abox_erap_handler_put),
	SOC_VALUE_ENUM_EXT("Audio Mode", abox_audio_mode_enum,
		abox_audio_mode_get, abox_audio_mode_put),
};

static const char * const spus_inx_texts[] = {"RDMA", "SIFSM"};
static SOC_ENUM_SINGLE_DECL(spus_in0_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_IN_L(0), spus_inx_texts);
static const struct snd_kcontrol_new spus_in0_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in0_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in1_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_IN_L(1), spus_inx_texts);
static const struct snd_kcontrol_new spus_in1_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in1_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in2_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_IN_L(2), spus_inx_texts);
static const struct snd_kcontrol_new spus_in2_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in2_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in3_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_IN_L(3), spus_inx_texts);
static const struct snd_kcontrol_new spus_in3_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in3_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in4_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_IN_L(4), spus_inx_texts);
static const struct snd_kcontrol_new spus_in4_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in4_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in5_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_IN_L(5), spus_inx_texts);
static const struct snd_kcontrol_new spus_in5_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in5_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in6_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_IN_L(6), spus_inx_texts);
static const struct snd_kcontrol_new spus_in6_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in6_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in7_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_IN_L(7), spus_inx_texts);
static const struct snd_kcontrol_new spus_in7_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in7_enum),
};

static const char * const spus_asrcx_texts[] = {"Off", "On"};
static SOC_ENUM_SINGLE_DECL(spus_asrc0_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_ASRC_L(0), spus_asrcx_texts);
static const struct snd_kcontrol_new spus_asrc0_controls[] = {
	SOC_DAPM_ENUM("ASRC", spus_asrc0_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_asrc1_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_ASRC_L(1), spus_asrcx_texts);
static const struct snd_kcontrol_new spus_asrc1_controls[] = {
	SOC_DAPM_ENUM("ASRC", spus_asrc1_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_asrc2_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_ASRC_L(2), spus_asrcx_texts);
static const struct snd_kcontrol_new spus_asrc2_controls[] = {
	SOC_DAPM_ENUM("ASRC", spus_asrc2_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_asrc3_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_ASRC_L(3), spus_asrcx_texts);
static const struct snd_kcontrol_new spus_asrc3_controls[] = {
	SOC_DAPM_ENUM("ASRC", spus_asrc3_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_asrc4_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_ASRC_L(4), spus_asrcx_texts);
static const struct snd_kcontrol_new spus_asrc4_controls[] = {
	SOC_DAPM_ENUM("ASRC", spus_asrc4_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_asrc5_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_ASRC_L(5), spus_asrcx_texts);
static const struct snd_kcontrol_new spus_asrc5_controls[] = {
	SOC_DAPM_ENUM("ASRC", spus_asrc5_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_asrc6_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_ASRC_L(6), spus_asrcx_texts);
static const struct snd_kcontrol_new spus_asrc6_controls[] = {
	SOC_DAPM_ENUM("ASRC", spus_asrc6_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_asrc7_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_ASRC_L(7), spus_asrcx_texts);
static const struct snd_kcontrol_new spus_asrc7_controls[] = {
	SOC_DAPM_ENUM("ASRC", spus_asrc7_enum),
};

static const char * const spus_outx_texts[] = {"SIFS1", "SIFS0", "SIFS2"};
static SOC_ENUM_SINGLE_DECL(spus_out0_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_OUT_L(0), spus_outx_texts);
static const struct snd_kcontrol_new spus_out0_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out0_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out1_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_OUT_L(1), spus_outx_texts);
static const struct snd_kcontrol_new spus_out1_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out1_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out2_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_OUT_L(2), spus_outx_texts);
static const struct snd_kcontrol_new spus_out2_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out2_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out3_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_OUT_L(3), spus_outx_texts);
static const struct snd_kcontrol_new spus_out3_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out3_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out4_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_OUT_L(4), spus_outx_texts);
static const struct snd_kcontrol_new spus_out4_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out4_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out5_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_OUT_L(5), spus_outx_texts);
static const struct snd_kcontrol_new spus_out5_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out5_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out6_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_OUT_L(6), spus_outx_texts);
static const struct snd_kcontrol_new spus_out6_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out6_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_out7_enum, ABOX_SPUS_CTRL0,
	ABOX_FUNC_CHAIN_SRC_OUT_L(7), spus_outx_texts);
static const struct snd_kcontrol_new spus_out7_controls[] = {
	SOC_DAPM_ENUM("DEMUX", spus_out7_enum),
};

static const char * const spusm_texts[] = {
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"UAIF0", "UAIF1", "UAIF2", "UAIF3", "UAIF4",
};
static SOC_ENUM_SINGLE_DECL(spusm_enum, ABOX_ROUTE_CTRL1,
	ABOX_ROUTE_SPUSM_L, spusm_texts);
static const struct snd_kcontrol_new spusm_controls[] = {
	SOC_DAPM_ENUM("MUX", spusm_enum),
};

static int abox_flush_mixp(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s\n", __func__);

	dev_info(codec->dev, "%s: flush\n", __func__);
	snd_soc_update_bits(codec, ABOX_SPUS_CTRL2,
			ABOX_SPUS_MIXP_FLUSH_MASK,
			1 << ABOX_SPUS_MIXP_FLUSH_L);

	return 0;
}

static int abox_flush_sifm(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_input_ep(w, NULL)) {
		dev_info(codec->dev, "%s: flush\n", __func__);
		snd_soc_update_bits(codec, ABOX_SPUS_CTRL3,
				ABOX_SPUS_SIFM_FLUSH_MASK,
				1 << ABOX_SPUS_SIFM_FLUSH_L);
	}

	return 0;
}

static int abox_flush_sifs1(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_input_ep(w, NULL)) {
		dev_info(codec->dev, "%s: flush\n", __func__);
		snd_soc_update_bits(codec, ABOX_SPUS_CTRL3,
				ABOX_SPUS_SIFS1_FLUSH_MASK,
				1 << ABOX_SPUS_SIFS1_FLUSH_L);
	}

	return 0;
}

static int abox_flush_sifs2(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_input_ep(w, NULL)) {
		dev_info(codec->dev, "%s: flush\n", __func__);
		snd_soc_update_bits(codec, ABOX_SPUS_CTRL3,
				ABOX_SPUS_SIFS2_FLUSH_MASK,
				1 << ABOX_SPUS_SIFS2_FLUSH_L);
	}

	return 0;
}

static int abox_flush_recp(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(codec->dev, "%s: flush\n", __func__);
		snd_soc_update_bits(codec, ABOX_SPUM_CTRL2,
				ABOX_SPUM_RECP_FLUSH_MASK,
				1 << ABOX_SPUM_RECP_FLUSH_L);
	}

	return 0;
}

static int abox_flush_sifm0(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(codec->dev, "%s: flush\n", __func__);
		snd_soc_update_bits(codec, ABOX_SPUM_CTRL3,
				ABOX_SPUM_SIFM0_FLUSH_MASK,
				1 << ABOX_SPUM_SIFM0_FLUSH_L);
	}

	return 0;
}

static int abox_flush_sifm1(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(codec->dev, "%s: flush\n", __func__);
		snd_soc_update_bits(codec, ABOX_SPUM_CTRL3,
				ABOX_SPUM_SIFM1_FLUSH_MASK,
				1 << ABOX_SPUM_SIFM1_FLUSH_L);
	}

	return 0;
}

static int abox_flush_sifm2(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(codec->dev, "%s: flush\n", __func__);
		snd_soc_update_bits(codec, ABOX_SPUM_CTRL3,
				ABOX_SPUM_SIFM2_FLUSH_MASK,
				1 << ABOX_SPUM_SIFM2_FLUSH_L);
	}

	return 0;
}

static int abox_flush_sifm3(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s\n", __func__);

	if (!snd_soc_dapm_connected_output_ep(w, NULL)) {
		dev_info(codec->dev, "%s: flush\n", __func__);
		snd_soc_update_bits(codec, ABOX_SPUM_CTRL3,
				ABOX_SPUM_SIFM3_FLUSH_MASK,
				1 << ABOX_SPUM_SIFM3_FLUSH_L);
	}

	return 0;
}

static const char * const sifsx_texts[] = {
	"SPUS OUT0", "SPUS OUT1", "SPUS OUT2", "SPUS OUT3",
	"SPUS OUT4", "SPUS OUT5", "SPUS OUT6", "SPUS OUT7",
};
static SOC_ENUM_SINGLE_DECL(sifs1_enum, ABOX_SPUS_CTRL1,
	ABOX_SIFS_OUT1_SEL_L, sifsx_texts);
static const struct snd_kcontrol_new sifs1_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs1_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs2_enum, ABOX_SPUS_CTRL1,
	ABOX_SIFS_OUT2_SEL_L, sifsx_texts);
static const struct snd_kcontrol_new sifs2_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs2_enum),
};

static const char * const sifsm_texts[] = {
	"SPUS IN0", "SPUS IN1", "SPUS IN2", "SPUS IN3",
	"SPUS IN4", "SPUS IN5", "SPUS IN6", "SPUS IN7",
};
static SOC_ENUM_SINGLE_DECL(sifsm_enum, ABOX_SPUS_CTRL1,
	ABOX_SIFM_IN_SEL_L, sifsm_texts);
static const struct snd_kcontrol_new sifsm_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifsm_enum),
};

static const char * const uaif_spkx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"SIFMS",
};
static SOC_ENUM_SINGLE_DECL(uaif_spk0_enum, ABOX_ROUTE_CTRL0,
	ABOX_ROUTE_UAIF_SPK_L(0), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif_spk0_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif_spk0_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif_spk1_enum, ABOX_ROUTE_CTRL0,
	ABOX_ROUTE_UAIF_SPK_L(1), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif_spk1_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif_spk1_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif_spk2_enum, ABOX_ROUTE_CTRL0,
	ABOX_ROUTE_UAIF_SPK_L(2), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif_spk2_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif_spk2_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif_spk3_enum, ABOX_ROUTE_CTRL0,
	ABOX_ROUTE_UAIF_SPK_L(3), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif_spk3_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif_spk3_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif_spk4_enum, ABOX_ROUTE_CTRL0,
	ABOX_ROUTE_UAIF_SPK_L(4), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif_spk4_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif_spk4_enum),
};

static const char * const dsif_spk_texts[] = {
	"RESERVED", "RESERVED", "SIFS1", "SIFS2",
};
static SOC_ENUM_SINGLE_DECL(dsif_spk_enum, ABOX_ROUTE_CTRL0, 20,
	dsif_spk_texts);
static const struct snd_kcontrol_new dsif_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", dsif_spk_enum),
};

static const char * const rsrcx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"NSRC0", "NSRC1", "NSRC2", "NSRC3",
};
static SOC_ENUM_SINGLE_DECL(rsrc0_enum, ABOX_ROUTE_CTRL2, ABOX_ROUTE_RSRC_L(0),
	rsrcx_texts);
static const struct snd_kcontrol_new rsrc0_controls[] = {
	SOC_DAPM_ENUM("DEMUX", rsrc0_enum),
};
static SOC_ENUM_SINGLE_DECL(rsrc1_enum, ABOX_ROUTE_CTRL2, ABOX_ROUTE_RSRC_L(1),
	rsrcx_texts);
static const struct snd_kcontrol_new rsrc1_controls[] = {
	SOC_DAPM_ENUM("DEMUX", rsrc1_enum),
};

static const char * const nsrcx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"RESERVED", "RESERVED", "RESERVED", "RESERVED",
	"UAIF0", "UAIF1", "UAIF2", "UAIF3", "UAIF4",
};
static SOC_ENUM_SINGLE_DECL(nsrc0_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(0),
	nsrcx_texts);
static const struct snd_kcontrol_new nsrc0_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc0_enum),
};
static SOC_ENUM_SINGLE_DECL(nsrc1_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(1),
	nsrcx_texts);
static const struct snd_kcontrol_new nsrc1_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc1_enum),
};
static SOC_ENUM_SINGLE_DECL(nsrc2_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(2),
	nsrcx_texts);
static const struct snd_kcontrol_new nsrc2_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc2_enum),
};
static SOC_ENUM_SINGLE_DECL(nsrc3_enum, ABOX_ROUTE_CTRL1, ABOX_ROUTE_NSRC_L(3),
	nsrcx_texts);
static const struct snd_kcontrol_new nsrc3_controls[] = {
	SOC_DAPM_ENUM("DEMUX", nsrc3_enum),
};

static const struct snd_kcontrol_new recp_controls[] = {
	SOC_DAPM_SINGLE("PIFS0", ABOX_SPUM_CTRL1, ABOX_RECP_SRC_VALID_L, 1, 0),
	SOC_DAPM_SINGLE("PIFS1", ABOX_SPUM_CTRL1, ABOX_RECP_SRC_VALID_H, 1, 0),
};

static const char * const spum_asrcx_texts[] = {"Off", "On"};
static SOC_ENUM_SINGLE_DECL(spum_asrc0_enum, ABOX_SPUM_CTRL0,
	ABOX_FUNC_CHAIN_RSRC_ASRC_L, spum_asrcx_texts);
static const struct snd_kcontrol_new spum_asrc0_controls[] = {
	SOC_DAPM_ENUM("ASRC", spum_asrc0_enum),
};
static SOC_ENUM_SINGLE_DECL(spum_asrc1_enum, ABOX_SPUM_CTRL0,
	ABOX_FUNC_CHAIN_NSRC_ASRC_L(0), spum_asrcx_texts);
static const struct snd_kcontrol_new spum_asrc1_controls[] = {
	SOC_DAPM_ENUM("ASRC", spum_asrc1_enum),
};
static SOC_ENUM_SINGLE_DECL(spum_asrc2_enum, ABOX_SPUM_CTRL0,
	ABOX_FUNC_CHAIN_NSRC_ASRC_L(1), spum_asrcx_texts);
static const struct snd_kcontrol_new spum_asrc2_controls[] = {
	SOC_DAPM_ENUM("ASRC", spum_asrc2_enum),
};
static SOC_ENUM_SINGLE_DECL(spum_asrc3_enum, ABOX_SPUM_CTRL0,
	ABOX_FUNC_CHAIN_NSRC_ASRC_L(2), spum_asrcx_texts);
static const struct snd_kcontrol_new spum_asrc3_controls[] = {
	SOC_DAPM_ENUM("ASRC", spum_asrc3_enum),
};

static const char * const sifmx_texts[] = {
	"WDMA", "SIFMS",
};
static SOC_ENUM_SINGLE_DECL(sifm0_enum, ABOX_SPUM_CTRL0,
	ABOX_FUNC_CHAIN_NSRC_OUT_L(0), sifmx_texts);
static const struct snd_kcontrol_new sifm0_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm0_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm1_enum, ABOX_SPUM_CTRL0,
	ABOX_FUNC_CHAIN_NSRC_OUT_L(1), sifmx_texts);
static const struct snd_kcontrol_new sifm1_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm1_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm2_enum, ABOX_SPUM_CTRL0,
	ABOX_FUNC_CHAIN_NSRC_OUT_L(2), sifmx_texts);
static const struct snd_kcontrol_new sifm2_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm2_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm3_enum, ABOX_SPUM_CTRL0,
	ABOX_FUNC_CHAIN_NSRC_OUT_L(3), sifmx_texts);
static const struct snd_kcontrol_new sifm3_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm3_enum),
};

static const char * const sifms_texts[] = {
	"RESERVED", "SIFM0", "SIFM1", "SIFM2", "SIFM3",
};
static SOC_ENUM_SINGLE_DECL(sifms_enum, ABOX_SPUM_CTRL1, ABOX_SIFS_OUT_SEL_L,
	sifms_texts);
static const struct snd_kcontrol_new sifms_controls[] = {
	SOC_DAPM_ENUM("MUX", sifms_enum),
};

static const struct snd_soc_dapm_widget abox_codec_dapm_widgets[] = {
	SND_SOC_DAPM_MUX("SPUSM", SND_SOC_NOPM, 0, 0, spusm_controls),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN3", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN4", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN5", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN6", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFSM-SPUS IN7", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_DEMUX_E("SIFSM", SND_SOC_NOPM, 0, 0, sifsm_controls,
		abox_flush_sifm, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("SPUS IN0", SND_SOC_NOPM, 0, 0, spus_in0_controls),
	SND_SOC_DAPM_MUX("SPUS IN1", SND_SOC_NOPM, 0, 0, spus_in1_controls),
	SND_SOC_DAPM_MUX("SPUS IN2", SND_SOC_NOPM, 0, 0, spus_in2_controls),
	SND_SOC_DAPM_MUX("SPUS IN3", SND_SOC_NOPM, 0, 0, spus_in3_controls),
	SND_SOC_DAPM_MUX("SPUS IN4", SND_SOC_NOPM, 0, 0, spus_in4_controls),
	SND_SOC_DAPM_MUX("SPUS IN5", SND_SOC_NOPM, 0, 0, spus_in5_controls),
	SND_SOC_DAPM_MUX("SPUS IN6", SND_SOC_NOPM, 0, 0, spus_in6_controls),
	SND_SOC_DAPM_MUX("SPUS IN7", SND_SOC_NOPM, 0, 0, spus_in7_controls),
	SND_SOC_DAPM_MUX("SPUS ASRC0", SND_SOC_NOPM, 0, 0, spus_asrc0_controls),
	SND_SOC_DAPM_MUX("SPUS ASRC1", SND_SOC_NOPM, 0, 0, spus_asrc1_controls),
	SND_SOC_DAPM_MUX("SPUS ASRC2", SND_SOC_NOPM, 0, 0, spus_asrc2_controls),
	SND_SOC_DAPM_MUX("SPUS ASRC3", SND_SOC_NOPM, 0, 0, spus_asrc3_controls),
	SND_SOC_DAPM_MUX("SPUS ASRC4", SND_SOC_NOPM, 0, 0, spus_asrc4_controls),
	SND_SOC_DAPM_MUX("SPUS ASRC5", SND_SOC_NOPM, 0, 0, spus_asrc5_controls),
	SND_SOC_DAPM_MUX("SPUS ASRC6", SND_SOC_NOPM, 0, 0, spus_asrc6_controls),
	SND_SOC_DAPM_MUX("SPUS ASRC7", SND_SOC_NOPM, 0, 0, spus_asrc7_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT0", SND_SOC_NOPM, 0, 0, spus_out0_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT1", SND_SOC_NOPM, 0, 0, spus_out1_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT2", SND_SOC_NOPM, 0, 0, spus_out2_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT3", SND_SOC_NOPM, 0, 0, spus_out3_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT4", SND_SOC_NOPM, 0, 0, spus_out4_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT5", SND_SOC_NOPM, 0, 0, spus_out5_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT6", SND_SOC_NOPM, 0, 0, spus_out6_controls),
	SND_SOC_DAPM_DEMUX("SPUS OUT7", SND_SOC_NOPM, 0, 0, spus_out7_controls),

	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT0-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT1-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT2-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT3-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT4-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT5-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT6-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SPUS OUT7-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER_E("SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0,
		abox_flush_mixp, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS1", SND_SOC_NOPM, 0, 0, sifs1_controls,
		abox_flush_sifs1, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS2", SND_SOC_NOPM, 0, 0, sifs2_controls,
		abox_flush_sifs2, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("UAIF SPK0", SND_SOC_NOPM, 0, 0, uaif_spk0_controls),
	SND_SOC_DAPM_MUX("UAIF SPK1", SND_SOC_NOPM, 0, 0, uaif_spk1_controls),
	SND_SOC_DAPM_MUX("UAIF SPK2", SND_SOC_NOPM, 0, 0, uaif_spk2_controls),
	SND_SOC_DAPM_MUX("UAIF SPK3", SND_SOC_NOPM, 0, 0, uaif_spk3_controls),
	SND_SOC_DAPM_MUX("UAIF SPK4", SND_SOC_NOPM, 0, 0, uaif_spk4_controls),
	SND_SOC_DAPM_MUX("DSIF SPK", SND_SOC_NOPM, 0, 0, dsif_spk_controls),

	SND_SOC_DAPM_MUX("RSRC0", SND_SOC_NOPM, 0, 0, rsrc0_controls),
	SND_SOC_DAPM_MUX("RSRC1", SND_SOC_NOPM, 0, 0, rsrc1_controls),
	SND_SOC_DAPM_MUX("NSRC0", SND_SOC_NOPM, 0, 0, nsrc0_controls),
	SND_SOC_DAPM_MUX("NSRC1", SND_SOC_NOPM, 0, 0, nsrc1_controls),
	SND_SOC_DAPM_MUX("NSRC2", SND_SOC_NOPM, 0, 0, nsrc2_controls),
	SND_SOC_DAPM_MUX("NSRC3", SND_SOC_NOPM, 0, 0, nsrc3_controls),

	SND_SOC_DAPM_PGA("PIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("PIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
	SOC_MIXER_E_ARRAY("RECP", SND_SOC_NOPM, 0, 0, recp_controls,
		abox_flush_recp, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("SPUM ASRC0", SND_SOC_NOPM, 0, 0, spum_asrc0_controls),
	SND_SOC_DAPM_MUX("SPUM ASRC1", SND_SOC_NOPM, 0, 0, spum_asrc1_controls),
	SND_SOC_DAPM_MUX("SPUM ASRC2", SND_SOC_NOPM, 0, 0, spum_asrc2_controls),
	SND_SOC_DAPM_MUX("SPUM ASRC3", SND_SOC_NOPM, 0, 0, spum_asrc3_controls),
	SND_SOC_DAPM_DEMUX_E("SIFM0", SND_SOC_NOPM, 0, 0, sifm0_controls,
		abox_flush_sifm0, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DEMUX_E("SIFM1", SND_SOC_NOPM, 0, 0, sifm1_controls,
		abox_flush_sifm1, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DEMUX_E("SIFM2", SND_SOC_NOPM, 0, 0, sifm2_controls,
		abox_flush_sifm2, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DEMUX_E("SIFM3", SND_SOC_NOPM, 0, 0, sifm3_controls,
		abox_flush_sifm3, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA("SIFM0-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM1-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM2-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFM3-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MUX("SIFMS", SND_SOC_NOPM, 0, 0, sifms_controls),

	SND_SOC_DAPM_MIC("Internal MIC", NULL),
	SND_SOC_DAPM_SPK("Internal SPK", NULL),

	SND_SOC_DAPM_AIF_IN("UAIF0IN", "UAIF0 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("UAIF1IN", "UAIF1 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("UAIF2IN", "UAIF2 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("UAIF3IN", "UAIF3 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("UAIF4IN", "UAIF4 Capture", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_AIF_OUT("UAIF0OUT", "UAIF0 Playback", 0, SND_SOC_NOPM,
		0, 0),
	SND_SOC_DAPM_AIF_OUT("UAIF1OUT", "UAIF1 Playback", 0, SND_SOC_NOPM,
		0, 0),
	SND_SOC_DAPM_AIF_OUT("UAIF2OUT", "UAIF2 Playback", 0, SND_SOC_NOPM,
		0, 0),
	SND_SOC_DAPM_AIF_OUT("UAIF3OUT", "UAIF3 Playback", 0, SND_SOC_NOPM,
		0, 0),
	SND_SOC_DAPM_AIF_OUT("UAIF4OUT", "UAIF4 Playback", 0, SND_SOC_NOPM,
		0, 0),
	SND_SOC_DAPM_AIF_OUT("DSIFOUT", "DSIF Playback", 0, SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_dapm_route abox_codec_dapm_routes[] = {
	/* sink, control, source */
	{"SPUSM", "UAIF0", "UAIF0 Capture"},
	{"SPUSM", "UAIF1", "UAIF1 Capture"},
	{"SPUSM", "UAIF2", "UAIF2 Capture"},
	{"SPUSM", "UAIF3", "UAIF3 Capture"},
	{"SPUSM", "UAIF4", "UAIF4 Capture"},

	{"SIFSM", NULL, "SPUSM"},
	{"SIFSM-SPUS IN0", "SPUS IN0", "SIFSM"},
	{"SIFSM-SPUS IN1", "SPUS IN1", "SIFSM"},
	{"SIFSM-SPUS IN2", "SPUS IN2", "SIFSM"},
	{"SIFSM-SPUS IN3", "SPUS IN3", "SIFSM"},
	{"SIFSM-SPUS IN4", "SPUS IN4", "SIFSM"},
	{"SIFSM-SPUS IN5", "SPUS IN5", "SIFSM"},
	{"SIFSM-SPUS IN6", "SPUS IN6", "SIFSM"},
	{"SIFSM-SPUS IN7", "SPUS IN7", "SIFSM"},

	{"SPUS IN0", "RDMA", "RDMA0 Playback"},
	{"SPUS IN0", "SIFSM", "SIFSM-SPUS IN0"},
	{"SPUS IN1", "RDMA", "RDMA1 Playback"},
	{"SPUS IN1", "SIFSM", "SIFSM-SPUS IN1"},
	{"SPUS IN2", "RDMA", "RDMA2 Playback"},
	{"SPUS IN2", "SIFSM", "SIFSM-SPUS IN2"},
	{"SPUS IN3", "RDMA", "RDMA3 Playback"},
	{"SPUS IN3", "SIFSM", "SIFSM-SPUS IN3"},
	{"SPUS IN4", "RDMA", "RDMA4 Playback"},
	{"SPUS IN4", "SIFSM", "SIFSM-SPUS IN4"},
	{"SPUS IN5", "RDMA", "RDMA5 Playback"},
	{"SPUS IN5", "SIFSM", "SIFSM-SPUS IN5"},
	{"SPUS IN6", "RDMA", "RDMA6 Playback"},
	{"SPUS IN6", "SIFSM", "SIFSM-SPUS IN6"},
	{"SPUS IN7", "RDMA", "RDMA7 Playback"},
	{"SPUS IN7", "SIFSM", "SIFSM-SPUS IN7"},

	{"SPUS ASRC0", "On", "SPUS IN0"},
	{"SPUS ASRC0", "Off", "SPUS IN0"},
	{"SPUS ASRC1", "On", "SPUS IN1"},
	{"SPUS ASRC1", "Off", "SPUS IN1"},
	{"SPUS ASRC2", "On", "SPUS IN2"},
	{"SPUS ASRC2", "Off", "SPUS IN2"},
	{"SPUS ASRC3", "On", "SPUS IN3"},
	{"SPUS ASRC3", "Off", "SPUS IN3"},
	{"SPUS ASRC4", "On", "SPUS IN4"},
	{"SPUS ASRC4", "Off", "SPUS IN4"},
	{"SPUS ASRC5", "On", "SPUS IN5"},
	{"SPUS ASRC5", "Off", "SPUS IN5"},
	{"SPUS ASRC6", "On", "SPUS IN6"},
	{"SPUS ASRC6", "Off", "SPUS IN6"},
	{"SPUS ASRC7", "On", "SPUS IN7"},
	{"SPUS ASRC7", "Off", "SPUS IN7"},

	{"SPUS OUT0", NULL, "SPUS ASRC0"},
	{"SPUS OUT1", NULL, "SPUS ASRC1"},
	{"SPUS OUT2", NULL, "SPUS ASRC2"},
	{"SPUS OUT3", NULL, "SPUS ASRC3"},
	{"SPUS OUT4", NULL, "SPUS ASRC4"},
	{"SPUS OUT5", NULL, "SPUS ASRC5"},
	{"SPUS OUT6", NULL, "SPUS ASRC6"},
	{"SPUS OUT7", NULL, "SPUS ASRC7"},

	{"SPUS OUT0-SIFS0", "SIFS0", "SPUS OUT0"},
	{"SPUS OUT1-SIFS0", "SIFS0", "SPUS OUT1"},
	{"SPUS OUT2-SIFS0", "SIFS0", "SPUS OUT2"},
	{"SPUS OUT3-SIFS0", "SIFS0", "SPUS OUT3"},
	{"SPUS OUT4-SIFS0", "SIFS0", "SPUS OUT4"},
	{"SPUS OUT5-SIFS0", "SIFS0", "SPUS OUT5"},
	{"SPUS OUT6-SIFS0", "SIFS0", "SPUS OUT6"},
	{"SPUS OUT7-SIFS0", "SIFS0", "SPUS OUT7"},
	{"SPUS OUT0-SIFS1", "SIFS1", "SPUS OUT0"},
	{"SPUS OUT1-SIFS1", "SIFS1", "SPUS OUT1"},
	{"SPUS OUT2-SIFS1", "SIFS1", "SPUS OUT2"},
	{"SPUS OUT3-SIFS1", "SIFS1", "SPUS OUT3"},
	{"SPUS OUT4-SIFS1", "SIFS1", "SPUS OUT4"},
	{"SPUS OUT5-SIFS1", "SIFS1", "SPUS OUT5"},
	{"SPUS OUT6-SIFS1", "SIFS1", "SPUS OUT6"},
	{"SPUS OUT7-SIFS1", "SIFS1", "SPUS OUT7"},
	{"SPUS OUT0-SIFS2", "SIFS2", "SPUS OUT0"},
	{"SPUS OUT1-SIFS2", "SIFS2", "SPUS OUT1"},
	{"SPUS OUT2-SIFS2", "SIFS2", "SPUS OUT2"},
	{"SPUS OUT3-SIFS2", "SIFS2", "SPUS OUT3"},
	{"SPUS OUT4-SIFS2", "SIFS2", "SPUS OUT4"},
	{"SPUS OUT5-SIFS2", "SIFS2", "SPUS OUT5"},
	{"SPUS OUT6-SIFS2", "SIFS2", "SPUS OUT6"},
	{"SPUS OUT7-SIFS2", "SIFS2", "SPUS OUT7"},

	{"SIFS0", NULL, "SPUS OUT0-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT1-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT2-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT3-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT4-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT5-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT6-SIFS0"},
	{"SIFS0", NULL, "SPUS OUT7-SIFS0"},
	{"SIFS1", "SPUS OUT0", "SPUS OUT0-SIFS1"},
	{"SIFS1", "SPUS OUT1", "SPUS OUT1-SIFS1"},
	{"SIFS1", "SPUS OUT2", "SPUS OUT2-SIFS1"},
	{"SIFS1", "SPUS OUT3", "SPUS OUT3-SIFS1"},
	{"SIFS1", "SPUS OUT4", "SPUS OUT4-SIFS1"},
	{"SIFS1", "SPUS OUT5", "SPUS OUT5-SIFS1"},
	{"SIFS1", "SPUS OUT6", "SPUS OUT6-SIFS1"},
	{"SIFS1", "SPUS OUT7", "SPUS OUT7-SIFS1"},
	{"SIFS2", "SPUS OUT0", "SPUS OUT0-SIFS2"},
	{"SIFS2", "SPUS OUT1", "SPUS OUT1-SIFS2"},
	{"SIFS2", "SPUS OUT2", "SPUS OUT2-SIFS2"},
	{"SIFS2", "SPUS OUT3", "SPUS OUT3-SIFS2"},
	{"SIFS2", "SPUS OUT4", "SPUS OUT4-SIFS2"},
	{"SIFS2", "SPUS OUT5", "SPUS OUT5-SIFS2"},
	{"SIFS2", "SPUS OUT6", "SPUS OUT6-SIFS2"},
	{"SIFS2", "SPUS OUT7", "SPUS OUT7-SIFS2"},

	{"UAIF SPK0", "SIFS0", "SIFS0"},
	{"UAIF SPK0", "SIFS1", "SIFS1"},
	{"UAIF SPK0", "SIFS2", "SIFS2"},
	{"UAIF SPK0", "SIFMS", "SIFMS"},
	{"UAIF SPK1", "SIFS0", "SIFS0"},
	{"UAIF SPK1", "SIFS1", "SIFS1"},
	{"UAIF SPK1", "SIFS2", "SIFS2"},
	{"UAIF SPK1", "SIFMS", "SIFMS"},
	{"UAIF SPK2", "SIFS0", "SIFS0"},
	{"UAIF SPK2", "SIFS1", "SIFS1"},
	{"UAIF SPK2", "SIFS2", "SIFS2"},
	{"UAIF SPK2", "SIFMS", "SIFMS"},
	{"UAIF SPK3", "SIFS0", "SIFS0"},
	{"UAIF SPK3", "SIFS1", "SIFS1"},
	{"UAIF SPK3", "SIFS2", "SIFS2"},
	{"UAIF SPK3", "SIFMS", "SIFMS"},
	{"UAIF SPK4", "SIFS0", "SIFS0"},
	{"UAIF SPK4", "SIFS1", "SIFS1"},
	{"UAIF SPK4", "SIFS2", "SIFS2"},
	{"UAIF SPK4", "SIFMS", "SIFMS"},
	{"DSIF SPK", "SIFS1", "SIFS1"},
	{"DSIF SPK", "SIFS2", "SIFS2"},

	{"UAIF0 Playback", NULL, "UAIF SPK0"},
	{"UAIF1 Playback", NULL, "UAIF SPK1"},
	{"UAIF2 Playback", NULL, "UAIF SPK2"},
	{"UAIF3 Playback", NULL, "UAIF SPK3"},
	{"UAIF4 Playback", NULL, "UAIF SPK4"},
	{"DSIF Playback", NULL, "DSIF SPK"},

	{"RSRC0", "SIFS0", "Internal Capture"},
	{"RSRC0", "SIFS1", "Internal Capture"},
	{"RSRC0", "SIFS2", "Internal Capture"},
	{"RSRC0", "NSRC0", "NSRC0"},
	{"RSRC0", "NSRC1", "NSRC1"},
	{"RSRC0", "NSRC2", "NSRC2"},
	{"RSRC0", "NSRC3", "NSRC3"},
	{"RSRC1", "SIFS0", "Internal Capture"},
	{"RSRC1", "SIFS1", "Internal Capture"},
	{"RSRC1", "SIFS2", "Internal Capture"},
	{"RSRC1", "NSRC0", "NSRC0"},
	{"RSRC1", "NSRC1", "NSRC1"},
	{"RSRC1", "NSRC2", "NSRC2"},
	{"RSRC1", "NSRC3", "NSRC3"},

	{"NSRC0", "SIFS0", "Internal Capture"},
	{"NSRC0", "SIFS1", "Internal Capture"},
	{"NSRC0", "SIFS2", "Internal Capture"},
	{"NSRC0", "UAIF0", "UAIF0 Capture"},
	{"NSRC0", "UAIF1", "UAIF1 Capture"},
	{"NSRC0", "UAIF2", "UAIF2 Capture"},
	{"NSRC0", "UAIF3", "UAIF3 Capture"},
	{"NSRC0", "UAIF4", "UAIF4 Capture"},
	{"NSRC1", "SIFS0", "Internal Capture"},
	{"NSRC1", "SIFS1", "Internal Capture"},
	{"NSRC1", "SIFS2", "Internal Capture"},
	{"NSRC1", "UAIF0", "UAIF0 Capture"},
	{"NSRC1", "UAIF1", "UAIF1 Capture"},
	{"NSRC1", "UAIF2", "UAIF2 Capture"},
	{"NSRC1", "UAIF3", "UAIF3 Capture"},
	{"NSRC1", "UAIF4", "UAIF4 Capture"},
	{"NSRC2", "SIFS0", "Internal Capture"},
	{"NSRC2", "SIFS1", "Internal Capture"},
	{"NSRC2", "SIFS2", "Internal Capture"},
	{"NSRC2", "UAIF0", "UAIF0 Capture"},
	{"NSRC2", "UAIF1", "UAIF1 Capture"},
	{"NSRC2", "UAIF2", "UAIF2 Capture"},
	{"NSRC2", "UAIF3", "UAIF3 Capture"},
	{"NSRC2", "UAIF4", "UAIF4 Capture"},
	{"NSRC3", "SIFS0", "Internal Capture"},
	{"NSRC3", "SIFS1", "Internal Capture"},
	{"NSRC3", "SIFS2", "Internal Capture"},
	{"NSRC3", "UAIF0", "UAIF0 Capture"},
	{"NSRC3", "UAIF1", "UAIF1 Capture"},
	{"NSRC3", "UAIF2", "UAIF2 Capture"},
	{"NSRC3", "UAIF3", "UAIF3 Capture"},
	{"NSRC3", "UAIF4", "UAIF4 Capture"},

	{"PIFS0", NULL, "RSRC0"},
	{"PIFS1", NULL, "RSRC1"},
	{"RECP", "PIFS0", "PIFS0"},
	{"RECP", "PIFS1", "PIFS1"},

	{"SPUM ASRC0", "On", "RECP"},
	{"SPUM ASRC0", "Off", "RECP"},
	{"SPUM ASRC1", "On", "NSRC0"},
	{"SPUM ASRC1", "Off", "NSRC0"},
	{"SPUM ASRC2", "On", "NSRC1"},
	{"SPUM ASRC2", "Off", "NSRC1"},
	{"SPUM ASRC3", "On", "NSRC2"},
	{"SPUM ASRC3", "Off", "NSRC2"},

	{"SIFM0", NULL, "SPUM ASRC1"},
	{"SIFM1", NULL, "SPUM ASRC2"},
	{"SIFM2", NULL, "SPUM ASRC3"},
	{"SIFM3", NULL, "NSRC3"},

	{"SIFM0-SIFMS", "SIFMS", "SIFM0"},
	{"SIFM1-SIFMS", "SIFMS", "SIFM1"},
	{"SIFM2-SIFMS", "SIFMS", "SIFM2"},
	{"SIFM3-SIFMS", "SIFMS", "SIFM3"},

	{"SIFMS", "SIFM0", "SIFM0-SIFMS"},
	{"SIFMS", "SIFM1", "SIFM1-SIFMS"},
	{"SIFMS", "SIFM2", "SIFM2-SIFMS"},
	{"SIFMS", "SIFM3", "SIFM3-SIFMS"},

	{"WDMA0 Capture", NULL, "SPUM ASRC0"},
	{"WDMA1 Capture", "WDMA", "SIFM0"},
	{"WDMA2 Capture", "WDMA", "SIFM1"},
	{"WDMA3 Capture", "WDMA", "SIFM2"},
	{"WDMA4 Capture", "WDMA", "SIFM3"},

	{"Internal Capture", NULL, "Internal MIC"},
	{"Internal SPK", NULL, "Internal Playback"},
};

static bool abox_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ABOX_SYSPOWER_CTRL:
	case ABOX_SYSPOWER_STATUS:
	case ABOX_SPUS_CTRL1:
	case ABOX_SPUS_CTRL2:
	case ABOX_SPUS_CTRL3:
	case ABOX_SPUM_CTRL1:
	case ABOX_SPUM_CTRL2:
	case ABOX_SPUM_CTRL3:
	case ABOX_UAIF_STATUS(0):
	case ABOX_UAIF_STATUS(1):
	case ABOX_UAIF_STATUS(2):
	case ABOX_UAIF_STATUS(3):
	case ABOX_UAIF_STATUS(4):
	case ABOX_DSIF_STATUS:
		return true;
	default:
		return false;
	}
}

static bool abox_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ABOX_IP_INDEX:
	case ABOX_VERSION:
	case ABOX_SYSPOWER_CTRL:
	case ABOX_SYSPOWER_STATUS:
	case ABOX_SYSTEM_CONFIG0:
	case ABOX_REMAP_MASK:
	case ABOX_REMAP_ADDR:
	case ABOX_DYN_CLOCK_OFF:
	case ABOX_ROUTE_CTRL0:
	case ABOX_ROUTE_CTRL1:
	case ABOX_ROUTE_CTRL2:
	case ABOX_SPUS_CTRL0:
	case ABOX_SPUS_CTRL1:
	case ABOX_SPUS_CTRL2:
	case ABOX_SPUS_CTRL3:
	case ABOX_SPUM_CTRL0:
	case ABOX_SPUM_CTRL1:
	case ABOX_SPUM_CTRL2:
	case ABOX_SPUM_CTRL3:
	case ABOX_UAIF_CTRL0(0):
	case ABOX_UAIF_CTRL1(0):
	case ABOX_UAIF_STATUS(0):
	case ABOX_UAIF_CTRL0(1):
	case ABOX_UAIF_CTRL1(1):
	case ABOX_UAIF_STATUS(1):
	case ABOX_UAIF_CTRL0(2):
	case ABOX_UAIF_CTRL1(2):
	case ABOX_UAIF_STATUS(2):
	case ABOX_UAIF_CTRL0(3):
	case ABOX_UAIF_CTRL1(3):
	case ABOX_UAIF_STATUS(3):
	case ABOX_UAIF_CTRL0(4):
	case ABOX_UAIF_CTRL1(4):
	case ABOX_UAIF_STATUS(4):
	case ABOX_DSIF_CTRL:
	case ABOX_DSIF_STATUS:
		return true;
	default:
		return false;
	}
}

static bool abox_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ABOX_SYSPOWER_CTRL:
	case ABOX_SYSTEM_CONFIG0:
	case ABOX_REMAP_MASK:
	case ABOX_REMAP_ADDR:
	case ABOX_DYN_CLOCK_OFF:
	case ABOX_ROUTE_CTRL0:
	case ABOX_ROUTE_CTRL1:
	case ABOX_ROUTE_CTRL2:
	case ABOX_SPUS_CTRL0:
	case ABOX_SPUS_CTRL1:
	case ABOX_SPUS_CTRL2:
	case ABOX_SPUS_CTRL3:
	case ABOX_SPUM_CTRL0:
	case ABOX_SPUM_CTRL1:
	case ABOX_SPUM_CTRL2:
	case ABOX_SPUM_CTRL3:
	case ABOX_UAIF_CTRL0(0):
	case ABOX_UAIF_CTRL1(0):
	case ABOX_UAIF_CTRL0(1):
	case ABOX_UAIF_CTRL1(1):
	case ABOX_UAIF_CTRL0(2):
	case ABOX_UAIF_CTRL1(2):
	case ABOX_UAIF_CTRL0(3):
	case ABOX_UAIF_CTRL1(3):
	case ABOX_UAIF_CTRL0(4):
	case ABOX_UAIF_CTRL1(4):
	case ABOX_DSIF_CTRL:
		return true;
	default:
		return false;
	}
}

static const struct reg_default abox_reg_defaults[] = {
	{0x0000, 0x41424F58},
	{0x0004, 0x01000000},
	{0x0010, 0x00000000},
	{0x0014, 0x00000000},
	{0x0020, 0x00000000},
	{0x0024, 0xFFF00000},
	{0x0028, 0x13F00000},
	{0x0030, 0x7FFFFFFF},
	{0x0040, 0x00000000},
	{0x0044, 0x00000000},
	{0x0048, 0x00000000},
	{0x0200, 0x00000000},
	{0x0204, 0x00000000},
	{0x0208, 0x00000000},
	{0x020C, 0x00000000},
	{0x0220, 0x00000000},
	{0x0224, 0x00000000},
	{0x0228, 0x00000000},
	{0x022C, 0x00000000},
	{0x0230, 0x00000000},
	{0x0234, 0x00000000},
	{0x0238, 0x00000000},
	{0x023C, 0x00000000},
	{0x0240, 0x00000000},
	{0x0260, 0x00000000},
	{0x0300, 0x00000000},
	{0x0304, 0x00000000},
	{0x0308, 0x00000000},
	{0x030C, 0x00000000},
	{0x0320, 0x00000000},
	{0x0324, 0x00000000},
	{0x0328, 0x00000000},
	{0x032C, 0x00000000},
	{0x0330, 0x00000000},
	{0x0334, 0x00000000},
	{0x0338, 0x00000000},
	{0x033C, 0x00000000},
	{0x0340, 0x00000000},
	{0x0344, 0x00000000},
	{0x0348, 0x00000000},
	{0x0500, 0x01000000},
	{0x0504, 0x00000000},
	{0x050C, 0x00000000},
	{0x0510, 0x01000000},
	{0x0514, 0x00000000},
	{0x051C, 0x00000000},
	{0x0520, 0x01000000},
	{0x0524, 0x00000000},
	{0x052C, 0x00000000},
	{0x0530, 0x01000000},
	{0x0534, 0x00000000},
	{0x053C, 0x00000000},
	{0x0540, 0x01000000},
	{0x0544, 0x00000000},
	{0x054C, 0x00000000},
	{0x0550, 0x00000000},
	{0x0554, 0x00000000},
};

static const struct regmap_config abox_codec_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.max_register = ABOX_MAX_REGISTERS,
	.volatile_reg = abox_volatile_reg,
	.readable_reg = abox_readable_reg,
	.writeable_reg = abox_writeable_reg,
	.reg_defaults = abox_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(abox_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
	.fast_io = true,
};

static struct regmap *abox_codec_get_regmap(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);

	return data->regmap;
}

static const struct snd_soc_codec_driver abox_codec = {
	.probe			= abox_codec_probe,
	.remove			= abox_codec_remove,
	.suspend		= abox_codec_suspend,
	.resume			= abox_codec_resume,
	.controls		= abox_codec_controls,
	.num_controls		= ARRAY_SIZE(abox_codec_controls),
	.dapm_widgets		= abox_codec_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(abox_codec_dapm_widgets),
	.dapm_routes		= abox_codec_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(abox_codec_dapm_routes),
	.get_regmap		= abox_codec_get_regmap,
};

static bool abox_find_nrf(const char *name,
		int stream,
		enum ABOX_CONFIGMSG *rate,
		enum ABOX_CONFIGMSG *format)
{
	struct abox_name_rate_format {
		const char *name;
		int stream;
		const enum ABOX_CONFIGMSG rate;
		const enum ABOX_CONFIGMSG format;
	};

	static const struct abox_name_rate_format abox_nrf[] = {
		{ "ABOX SIFS0",	SNDRV_PCM_STREAM_PLAYBACK,
			SET_MIXER_SAMPLE_RATE,	SET_MIXER_FORMAT },
		{ "ABOX SIFS1",	SNDRV_PCM_STREAM_PLAYBACK,
			SET_OUT1_SAMPLE_RATE,	SET_OUT1_FORMAT },
		{ "ABOX SIFS2",	SNDRV_PCM_STREAM_PLAYBACK,
			SET_OUT2_SAMPLE_RATE,	SET_OUT2_FORMAT },
		{ "ABOX RECP",	SNDRV_PCM_STREAM_CAPTURE,
			SET_RECP_SAMPLE_RATE,	SET_RECP_FORMAT },
		{ "ABOX SIFM0",	SNDRV_PCM_STREAM_CAPTURE,
			SET_INMUX0_SAMPLE_RATE,	SET_INMUX0_FORMAT },
		{ "ABOX SIFM1",	SNDRV_PCM_STREAM_CAPTURE,
			SET_INMUX1_SAMPLE_RATE,	SET_INMUX1_FORMAT },
		{ "ABOX SIFM2",	SNDRV_PCM_STREAM_CAPTURE,
			SET_INMUX2_SAMPLE_RATE,	SET_INMUX2_FORMAT },
		{ "ABOX SIFM3",	SNDRV_PCM_STREAM_CAPTURE,
			SET_INMUX3_SAMPLE_RATE,	SET_INMUX3_FORMAT },
		{ "SIFS0",	SNDRV_PCM_STREAM_PLAYBACK,
			SET_MIXER_SAMPLE_RATE,	SET_MIXER_FORMAT },
		{ "SIFS1",	SNDRV_PCM_STREAM_PLAYBACK,
			SET_OUT1_SAMPLE_RATE,	SET_OUT1_FORMAT },
		{ "SIFS2",	SNDRV_PCM_STREAM_PLAYBACK,
			SET_OUT2_SAMPLE_RATE,	SET_OUT2_FORMAT },
		{ "RECP",	SNDRV_PCM_STREAM_CAPTURE,
			SET_RECP_SAMPLE_RATE,	SET_RECP_FORMAT },
		{ "SIFM0",	SNDRV_PCM_STREAM_CAPTURE,
			SET_INMUX0_SAMPLE_RATE,	SET_INMUX0_FORMAT },
		{ "SIFM1",	SNDRV_PCM_STREAM_CAPTURE,
			SET_INMUX1_SAMPLE_RATE,	SET_INMUX1_FORMAT },
		{ "SIFM2",	SNDRV_PCM_STREAM_CAPTURE,
			SET_INMUX2_SAMPLE_RATE,	SET_INMUX2_FORMAT },
		{ "SIFM3",	SNDRV_PCM_STREAM_CAPTURE,
			SET_INMUX3_SAMPLE_RATE,	SET_INMUX3_FORMAT },
	};

	const struct abox_name_rate_format *nrf;

	for (nrf = abox_nrf; nrf - abox_nrf < ARRAY_SIZE(abox_nrf); nrf++) {
		if ((nrf->stream == stream) && (strcmp(nrf->name, name) == 0)) {
			*rate = nrf->rate;
			*format = nrf->format;
			return true;
		}
	}

	return false;
}

int abox_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_dai *dai = rtd->cpu_dai;
	struct snd_soc_component *cmpnt = dai->component;
	struct device *dev = dai->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct snd_soc_dpcm *dpcm;
	struct snd_pcm_hw_params *fastest_hw_params = NULL;
	struct snd_soc_dapm_widget *w;
	LIST_HEAD(widget_list);
	int stream;
	enum ABOX_CONFIGMSG rate, format;

	dev_dbg(dev, "%s[%s]\n", __func__, dai->name);

	switch (rtd->dpcm[SNDRV_PCM_STREAM_PLAYBACK].state) {
	case SND_SOC_DPCM_STATE_OPEN:
	case SND_SOC_DPCM_STATE_HW_PARAMS:
	case SND_SOC_DPCM_STATE_HW_FREE:
		stream = SNDRV_PCM_STREAM_PLAYBACK;
		break;
	default:
		stream = SNDRV_PCM_STREAM_CAPTURE;
		break;
	}

	list_for_each_entry(dpcm, &rtd->dpcm[stream].fe_clients, list_fe) {
		if (!fastest_hw_params || (params_rate(fastest_hw_params) <
				params_rate(&dpcm->hw_params))) {
			fastest_hw_params = &dpcm->hw_params;
		}
	}

	if (params_channels(params) < 1) {
		dev_info(dev, "channel is fixed from %d to 2\n",
				params_channels(params));
		hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS)->min = 2;
	}

	if (params_width(params) < 16) {
		dev_info(dev, "width is fixed from %d to 16\n",
				params_width(params));
		params_set_format(params, SNDRV_PCM_FORMAT_S16_LE);
	}

	snd_soc_dapm_mutex_lock(snd_soc_component_get_dapm(cmpnt));
	/*
	 * For snd_soc_dapm_connected_{output,input}_ep fully discover the graph
	 * we need to reset the cached number of inputs and outputs.
	 */
	list_for_each_entry(w, &cmpnt->card->widgets, list) {
		w->endpoints[SND_SOC_DAPM_DIR_IN] = -1;
		w->endpoints[SND_SOC_DAPM_DIR_OUT] = -1;
	}
	snd_soc_dapm_connected_input_ep(dai->playback_widget, &widget_list);
	snd_soc_dapm_connected_output_ep(dai->capture_widget, &widget_list);

	rate = (stream == SNDRV_PCM_STREAM_PLAYBACK) ?
			SET_MIXER_SAMPLE_RATE : SET_INMUX0_SAMPLE_RATE;
	format = (stream == SNDRV_PCM_STREAM_PLAYBACK) ?
			SET_MIXER_FORMAT : SET_INMUX0_FORMAT;

	list_for_each_entry(w, &widget_list, work_list) {
		dev_dbg(dev, "%s", w->name);

		if (!abox_find_nrf(w->name, stream, &rate, &format))
			continue;

		if (data->out_rate_auto[rate]) {
			dev_dbg(dev, "%s: automatic\n", __func__);
			abox_output_rate_put_ipc(dev,
					params_rate(fastest_hw_params),
					rate);
		}

		abox_output_format_put_ipc(dev, format, params_width(params),
				params_channels(params));
		hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE)->min =
				abox_get_out_rate(data, rate);
		dev_info(dev, "%s: %d bit, %u channel, %uHz\n", __func__,
				params_width(params), params_channels(params),
				abox_get_out_rate(data, rate));
	}
	snd_soc_dapm_mutex_unlock(snd_soc_component_get_dapm(cmpnt));

	return 0;
}
EXPORT_SYMBOL(abox_hw_params_fixup_helper);

static struct pm_qos_request abox_pm_qos_int;
static struct pm_qos_request abox_pm_qos_mif;
static struct pm_qos_request abox_pm_qos_lit;
static struct pm_qos_request abox_pm_qos_big;

unsigned int abox_get_requiring_int_freq_in_khz(void)
{
	struct abox_data *data = p_abox_data;
	unsigned int gear;
	unsigned int int_freq;

	if (data == NULL)
		return 0;

	gear = data->clk_ca7_gear;

	if (gear <= ARRAY_SIZE(data->pm_qos_int))
		int_freq = data->pm_qos_int[gear - 1];
	else
		int_freq = 0;

	return int_freq;
}
EXPORT_SYMBOL(abox_get_requiring_int_freq_in_khz);

bool abox_cpu_gear_idle(struct device *dev, struct abox_data *data,
		unsigned int id)
{
	struct abox_qos_request *request;
	size_t len = ARRAY_SIZE(data->ca7_gear_requests);

	dev_dbg(dev, "%s(%x)\n", __func__, id);

	for (request = data->ca7_gear_requests;
			request - data->ca7_gear_requests < len && request->id;
			request++) {
		if (id == request->id)
			return (request->value >= CPU_GEAR_LOWER_LIMIT);
	}

	return true;
}

static void abox_check_call_cpu_gear(struct device *dev,
		struct abox_data *data,
		unsigned int old_id, unsigned int old_gear,
		unsigned int id, unsigned int gear)
{
	if (id == ABOX_CPU_GEAR_BOOT &&
			data->calliope_state == CALLIOPE_ENABLING) {
		abox_boot_done(dev, data->calliope_version);
		return;
	}

	if (id != ABOX_CPU_GEAR_CALL)
		return;

	if (old_id != id) {
		if (gear < CPU_GEAR_LOWER_LIMIT) {
			/* new */
			dev_info(dev, "%s: new\n", __func__);
			pm_runtime_get(&data->pdev->dev);
		}
	} else {
		if ((old_gear >= CPU_GEAR_LOWER_LIMIT) &&
				(gear < CPU_GEAR_LOWER_LIMIT)) {
			/* on */
			dev_info(dev, "%s: on\n", __func__);
			pm_runtime_get(&data->pdev->dev);
		} else if ((old_gear < CPU_GEAR_LOWER_LIMIT) &&
				(gear >= CPU_GEAR_LOWER_LIMIT)) {
			/* off */
			dev_info(dev, "%s: off\n", __func__);
			pm_runtime_put(&data->pdev->dev);
		}
	}
}

static void abox_notify_cpu_gear(struct abox_data *data, unsigned int freq)
{
	struct device *dev = &data->pdev->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	unsigned long long time = sched_clock();
	unsigned long rem = do_div(time, NSEC_PER_SEC);

	switch (data->calliope_state) {
	case CALLIOPE_ENABLING:
	case CALLIOPE_ENABLED:
		dev_dbg(dev, "%s\n", __func__);

		msg.ipcid = IPC_SYSTEM;
		system_msg->msgtype = ABOX_CHANGED_GEAR;
		system_msg->param1 = (int)freq;
		system_msg->param2 = (int)time; /* SEC */
		system_msg->param3 = (int)rem; /* NSEC */
		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
		break;
	case CALLIOPE_DISABLING:
	case CALLIOPE_DISABLED:
	default:
		/* notification to passing by context is not needed */
		break;
	}
}

static void abox_change_cpu_gear(struct device *dev, struct abox_data *data)
{
	struct abox_qos_request *request;
	size_t len = ARRAY_SIZE(data->ca7_gear_requests);
	unsigned int gear = UINT_MAX;
	int result;
	bool increasing;

	dev_dbg(dev, "%s\n", __func__);

	for (request = data->ca7_gear_requests;
			request - data->ca7_gear_requests < len && request->id;
			request++) {
		if (gear > request->value)
			gear = request->value;
		dev_dbg(dev, "id=%x, value=%u, gear=%u\n",
				request->id, request->value, gear);
	}

	increasing = (gear < data->clk_ca7_gear);
	data->clk_ca7_gear = gear;

	if (increasing) {
		if (gear <= ARRAY_SIZE(data->pm_qos_int)) {
			pm_qos_update_request(&abox_pm_qos_int,
					data->pm_qos_int[gear - 1]);
		} else {
			pm_qos_update_request(&abox_pm_qos_int, 0);
		}
	}

	if (gear >= CPU_GEAR_LOWER_LIMIT) {
		result = clk_set_rate(data->clk_pll, 0);
		if (IS_ERR_VALUE(result)) {
			dev_warn(dev, "setting pll clock to %d is failed: %d\n",
					0, result);
		}
		dev_info(dev, "pll clock: %lu\n", clk_get_rate(data->clk_pll));
		result = clk_set_rate(data->clk_ca7, AUD_PLL_RATE_KHZ);
		if (IS_ERR_VALUE(result)) {
			dev_warn(dev, "setting cpu clock gear to %d is failed: %d\n",
					gear, result);
		}
	} else {
		result = clk_set_rate(data->clk_ca7, AUD_PLL_RATE_KHZ / gear);
		if (IS_ERR_VALUE(result)) {
			dev_warn(dev, "setting cpu clock gear to %d is failed: %d\n",
					gear, result);
		}

		if (clk_get_rate(data->clk_pll) <= AUD_PLL_RATE_HZ_BYPASS) {
			result = clk_set_rate(data->clk_pll,
				AUD_PLL_RATE_HZ_FOR_48000);
			if (IS_ERR_VALUE(result)) {
				dev_warn(dev, "setting pll clock to %d is failed: %d\n",
						AUD_PLL_RATE_HZ_FOR_48000,
						result);
			}
			dev_info(dev, "pll clock: %lu\n",
					clk_get_rate(data->clk_pll));
		}
	}
	dev_info(dev, "cpu clock: %lukHz\n", clk_get_rate(data->clk_ca7));

	if (!increasing) {
		if (gear <= ARRAY_SIZE(data->pm_qos_int)) {
			pm_qos_update_request(&abox_pm_qos_int,
					data->pm_qos_int[gear - 1]);
		} else {
			pm_qos_update_request(&abox_pm_qos_int, 0);
		}
	}

	abox_notify_cpu_gear(data, clk_get_rate(data->clk_ca7) * 1000);
}

static void abox_change_cpu_gear_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			change_cpu_gear_work);

	abox_change_cpu_gear(&data->pdev->dev, data);
}

int abox_request_cpu_gear(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int gear)
{
	struct abox_qos_request *request;
	size_t len = ARRAY_SIZE(data->ca7_gear_requests);

	dev_info(dev, "%s(%x, %u)\n", __func__, id, gear);

	for (request = data->ca7_gear_requests;
			request - data->ca7_gear_requests < len
			&& request->id && request->id != id;
			request++) {
	}

	abox_check_call_cpu_gear(dev, data, request->id, request->value,
			id, gear);

	request->value = gear;
	wmb(); /* value is read only when id is valid */
	request->id = id;

	if (request - data->ca7_gear_requests >= len) {
		dev_err(dev, "%s: out of index. id=%x, gear=%u\n", __func__,
				id, gear);
		return -ENOMEM;
	}

	queue_work(system_freezable_wq, &data->change_cpu_gear_work);

	return 0;
}

int abox_request_cpu_gear_sync(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int gear)
{
	int result = abox_request_cpu_gear(dev, data, id, gear);

	flush_work(&data->change_cpu_gear_work);
	return result;
}

static void abox_clear_cpu_gear_requests(struct device *dev,
		struct abox_data *data)
{
	struct abox_qos_request *req;
	size_t len = ARRAY_SIZE(data->ca7_gear_requests);

	dev_info(dev, "%s\n", __func__);

	for (req = data->ca7_gear_requests; req - data->ca7_gear_requests < len
			&& req->id; req++) {
		if (req->value < CPU_GEAR_LOWER_LIMIT) {
			req->value = CPU_GEAR_LOWER_LIMIT;
			abox_request_cpu_gear(dev, data, req->id, req->value);
		}
	}
}

static void abox_change_mif_freq_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			change_mif_freq_work);

	dev_info(&data->pdev->dev, "%s(%u)\n", __func__, data->mif_freq);

	pm_qos_update_request(&abox_pm_qos_mif, data->mif_freq);
}

static void abox_request_mif_freq(struct device *dev, unsigned int mif_freq)
{
	struct abox_data *data = dev_get_drvdata(dev);

	dev_info(dev, "%s(%u)\n", __func__, mif_freq);

	data->mif_freq = mif_freq;
	schedule_work(&data->change_mif_freq_work);
}

static void abox_change_lit_freq_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			change_lit_freq_work);
	struct device *dev = &data->pdev->dev;
	size_t array_size = ARRAY_SIZE(data->lit_requests);
	struct abox_qos_request *request;
	unsigned int freq = 0;

	dev_dbg(dev, "%s\n", __func__);

	for (request = data->lit_requests;
			request - data->lit_requests < array_size &&
			request->id; request++) {
		if (freq < request->value)
			freq = request->value;

		dev_dbg(dev, "id=%x, value=%u, freq=%u\n", request->id,
				request->value, freq);
	}

	data->lit_freq = freq;
	pm_qos_update_request(&abox_pm_qos_lit, data->lit_freq);

	dev_info(dev, "pm qos request little: %dkHz\n",
			pm_qos_request(abox_pm_qos_lit.pm_qos_class));
}

int abox_request_lit_freq(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int freq)
{
	size_t array_size = ARRAY_SIZE(data->lit_requests);
	struct abox_qos_request *request;

	if (!id)
		id = DEFAULT_LIT_FREQ_ID;

	for (request = data->lit_requests;
			request - data->lit_requests < array_size &&
			request->id && request->id != id; request++) {
	}

	if ((request->id == id) && (request->value == freq))
		return 0;

	request->value = freq;
	wmb(); /* value is read only when id is valid */
	request->id = id;

	dev_info(dev, "%s(%x, %u)\n", __func__, id, freq);

	if (request - data->lit_requests >= ARRAY_SIZE(data->lit_requests)) {
		dev_err(dev, "%s: out of index. id=%x, freq=%u\n",
				__func__, id, freq);
		return -ENOMEM;
	}

	schedule_work(&data->change_lit_freq_work);

	return 0;
}

static void abox_change_big_freq_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			change_big_freq_work);
	struct device *dev = &data->pdev->dev;
	size_t array_size = ARRAY_SIZE(data->big_requests);
	struct abox_qos_request *request;
	unsigned int freq = 0;

	dev_dbg(dev, "%s\n", __func__);

	for (request = data->big_requests;
			request - data->big_requests < array_size &&
			request->id; request++) {
		if (freq < request->value)
			freq = request->value;

		dev_dbg(dev, "id=%x, value=%u, freq=%u\n", request->id,
				request->value, freq);
	}

	data->big_freq = freq;
	pm_qos_update_request(&abox_pm_qos_big, data->big_freq);

	dev_info(dev, "pm qos request big: %dkHz\n",
			pm_qos_request(abox_pm_qos_big.pm_qos_class));
}

int abox_request_big_freq(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int freq)
{
	size_t array_size = ARRAY_SIZE(data->big_requests);
	struct abox_qos_request *request;

	if (!id)
		id = DEFAULT_BIG_FREQ_ID;

	for (request = data->big_requests;
			request - data->big_requests < array_size &&
			request->id && request->id != id; request++) {
	}

	if ((request->id == id) && (request->value == freq))
		return 0;

	dev_info(dev, "%s(%x, %u)\n", __func__, id, freq);

	request->value = freq;
	wmb(); /* value is read only when id is valid */
	request->id = id;

	if (request - data->big_requests >= ARRAY_SIZE(data->big_requests)) {
		dev_err(dev, "%s: out of index. id=%x, freq=%u\n",
				__func__, id, freq);
		return -ENOMEM;
	}

	schedule_work(&data->change_big_freq_work);

	return 0;
}

static void abox_change_hmp_boost_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			change_hmp_boost_work);
	struct device *dev = &data->pdev->dev;
	size_t array_size = ARRAY_SIZE(data->hmp_requests);
	struct abox_qos_request *request;
	unsigned int on = 0;

	dev_dbg(dev, "%s\n", __func__);

	for (request = data->hmp_requests;
			request - data->hmp_requests < array_size &&
			request->id; request++) {
		if (request->value)
			on = request->value;

		dev_dbg(dev, "id=%x, value=%u, on=%u\n", request->id,
				request->value, on);
	}

	if (data->hmp_boost != on) {
		dev_info(dev, "request hmp boost: %d\n", on);

		data->hmp_boost = on;
		set_hmp_boost(on);
	}
}

int abox_request_hmp_boost(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int on)
{
	size_t array_size = ARRAY_SIZE(data->hmp_requests);
	struct abox_qos_request *request;

	if (!id)
		id = DEFAULT_HMP_BOOST_ID;

	for (request = data->hmp_requests;
			request - data->hmp_requests < array_size &&
			request->id && request->id != id; request++) {
	}

	if ((request->id == id) && (request->value == on))
		return 0;

	dev_info(dev, "%s(%x, %u)\n", __func__, id, on);

	request->value = on;
	wmb(); /* value is read only when id is valid */
	request->id = id;

	if (request - data->hmp_requests >= ARRAY_SIZE(data->hmp_requests)) {
		dev_err(dev, "%s: out of index. id=%x, on=%u\n",
				__func__, id, on);
		return -ENOMEM;
	}

	schedule_work(&data->change_hmp_boost_work);

	return 0;
}

void abox_request_dram_on(struct platform_device *pdev_abox, void *id, bool on)
{
	struct device *dev = &pdev_abox->dev;
	struct abox_data *data = platform_get_drvdata(pdev_abox);
	struct abox_dram_request *request;
	unsigned int val = 0x0;

	dev_dbg(dev, "%s(%d)\n", __func__, on);

	for (request = data->dram_requests;
			request - data->dram_requests <
			ARRAY_SIZE(data->dram_requests)
			&& request->id && request->id != id;
			request++) {
	}

	request->on = on;
	wmb(); /* value is read only when id is valid */
	request->id = id;

	for (request = data->dram_requests;
			request - data->dram_requests <
			ARRAY_SIZE(data->dram_requests)
			&& request->id;
			request++) {
		if (request->on) {
			val = ABOX_SYSPOWER_CTRL_MASK;
			break;
		}
	}

	regmap_write(data->regmap, ABOX_SYSPOWER_CTRL, val);
	dev_dbg(dev, "%s: SYSPOWER_CTRL=%08x\n", __func__,
			({regmap_read(data->regmap, ABOX_SYSPOWER_CTRL, &val);
			val; }));
}

int abox_register_irq_handler(struct device *dev, int ipc_id,
		abox_irq_handler_t irq_handler, void *dev_id)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_irq_action *irq_action = NULL;
	bool new_handler = true;

	if (ipc_id >= IPC_ID_COUNT)
		return -EINVAL;

	list_for_each_entry(irq_action, &data->irq_actions, list) {
		if (irq_action->irq == ipc_id && irq_action->dev_id == dev_id) {
			new_handler = false;
			break;
		}
	}

	if (new_handler) {
		irq_action = devm_kzalloc(dev, sizeof(struct abox_irq_action),
				GFP_KERNEL);
		if (IS_ERR_OR_NULL(irq_action)) {
			dev_err(dev, "%s: kmalloc fail\n", __func__);
			return -ENOMEM;
		}
		irq_action->irq = ipc_id;
		irq_action->dev_id = dev_id;
		list_add_tail(&irq_action->list, &data->irq_actions);
	}

	irq_action->irq_handler = irq_handler;

	return 0;
}
EXPORT_SYMBOL(abox_register_irq_handler);

static int abox_control_asrc(struct snd_soc_dapm_widget *w, bool on)
{
	struct snd_kcontrol *kcontrol;
	struct snd_soc_component *cmpnt;
	struct soc_enum *e;
	unsigned int reg, mask, val;

	if (!w || !w->name || !w->num_kcontrols)
		return -EINVAL;

	kcontrol = w->kcontrols[0];
	cmpnt = w->dapm->component;
	e = (struct soc_enum *)kcontrol->private_value;
	reg = e->reg;
	mask = e->mask << e->shift_l;
	val = (on ? 1 : 0) << e->shift_l;

	return snd_soc_component_update_bits(cmpnt, reg, mask, val);
}

static bool abox_is_asrc_widget(struct snd_soc_dapm_widget *w)
{
	return w->name && !!strstr(w->name, "ASRC");
}

int abox_try_to_asrc_off(struct device *dev, struct abox_data *data,
		struct snd_soc_dai *dai)
{
	struct snd_soc_component *cmpnt = dai->component;
	struct snd_soc_dapm_widget *w, *w_asrc = NULL;
	LIST_HEAD(widget_list);
	enum ABOX_CONFIGMSG rate, format;
	int stream, srate = 0;

	dev_info(dev, "%s(%s)\n", __func__, dai->name);

	if (dai->playback_widget == dai->capture_widget) {
		dev_warn(dev, "%s: %s has playback, capture both\n",
				__func__, dai->name);
		return -EINVAL;
	}

	snd_soc_dapm_mutex_lock(snd_soc_component_get_dapm(cmpnt));
	/*
	 * For snd_soc_dapm_connected_{output,input}_ep fully discover the graph
	 * we need to reset the cached number of inputs and outputs.
	 */
	list_for_each_entry(w, &cmpnt->card->widgets, list) {
		w->endpoints[SND_SOC_DAPM_DIR_IN] = -1;
		w->endpoints[SND_SOC_DAPM_DIR_OUT] = -1;
	}
	if (dai->playback_widget) {
		stream = SNDRV_PCM_STREAM_PLAYBACK;
		snd_soc_dapm_connected_output_ep(dai->playback_widget,
				&widget_list);
	}
	if (dai->capture_widget) {
		stream = SNDRV_PCM_STREAM_CAPTURE;
		snd_soc_dapm_connected_input_ep(dai->capture_widget,
				&widget_list);
	}

	list_for_each_entry(w, &widget_list, work_list) {
		dev_dbg(dev, "%s", w->name);

		if (abox_find_nrf(w->name, stream, &rate, &format)) {
			srate = abox_get_out_rate(data, rate);
			dev_dbg(dev, "%s: rate=%d\n", w->name, srate);
		}

		if (abox_is_asrc_widget(w)) {
			w_asrc = w;
			dev_dbg(dev, "%s is asrc\n", w->name);
		}
	}
	snd_soc_dapm_mutex_unlock(snd_soc_component_get_dapm(cmpnt));

	if (!w_asrc || !srate) {
		dev_warn(dev, "%s: incomplete path: w_asrc=%s, srate=%d",
				__func__, w_asrc ? w_asrc->name : "(null)",
				srate);
		return -EINVAL;
	}

	return abox_control_asrc(w_asrc, (dai->rate != srate));
}

void abox_register_rdma(struct platform_device *pdev_abox,
		struct platform_device *pdev_rdma, unsigned int id)
{
	struct abox_data *data = platform_get_drvdata(pdev_abox);

	if (id < ARRAY_SIZE(data->pdev_rdma)) {
		data->pdev_rdma[id] = pdev_rdma;
		if (id > data->rdma_count)
			data->rdma_count = id + 1;
	} else {
		dev_err(&data->pdev->dev, "%s: invalid id(%u)\n", __func__, id);
	}
}

void abox_register_wdma(struct platform_device *pdev_abox,
		struct platform_device *pdev_wdma, unsigned int id)
{
	struct abox_data *data = platform_get_drvdata(pdev_abox);

	if (id < ARRAY_SIZE(data->pdev_wdma)) {
		data->pdev_wdma[id] = pdev_wdma;
		if (id > data->wdma_count)
			data->wdma_count = id + 1;
	} else {
		dev_err(&data->pdev->dev, "%s: invalid id(%u)\n", __func__, id);
	}
}

static int abox_component_control_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;

	dev_dbg(dev, "%s(%s)\n", __func__, kcontrol->id.name);

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = value->control->count;
	uinfo->value.integer.min = value->control->min;
	uinfo->value.integer.max = value->control->max;
	return 0;
}

static ABOX_IPC_MSG abox_component_control_get_msg;

static int abox_component_control_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	ABOX_IPC_MSG *msg = &abox_component_control_get_msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg->msg.system;
	int i, result;

	dev_dbg(dev, "%s\n", __func__);

	pm_runtime_barrier(dev);

	msg->ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_REQUEST_COMPONENT_CONTROL;
	system_msg->param1 = value->desc->id;
	system_msg->param2 = value->control->id;
	result = abox_request_ipc(dev, msg->ipcid, msg, sizeof(*msg), 0, 1);
	if (IS_ERR_VALUE(result))
		return result;

	result = wait_event_timeout(data->ipc_wait_queue,
			system_msg->msgtype == ABOX_REPORT_COMPONENT_CONTROL,
			msecs_to_jiffies(1000));
	if (system_msg->msgtype != ABOX_REPORT_COMPONENT_CONTROL)
		return -ETIME;

	for (i = 0; i < value->control->count; i++) {
		long val = (long)system_msg->bundle.param_s32[i];

		ucontrol->value.integer.value[i] = val;
	}

	return 0;
}

static int abox_component_control_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct device *dev = codec->dev;
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	int i;

	dev_dbg(dev, "%s\n", __func__);

	pm_runtime_barrier(dev);

	for (i = 0; i < value->control->count; i++) {
		int val = (int)ucontrol->value.integer.value[i];

		system_msg->bundle.param_s32[i] = val;
		dev_dbg(dev, "%s[%d] <= %d", kcontrol->id.name, i, val);
	}

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_UPDATE_COMPONENT_CONTROL;
	system_msg->param1 = value->desc->id;
	system_msg->param2 = value->control->id;

	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
}

#define ABOX_COMPONENT_KCONTROL(xname, xdesc, xcontrol)	\
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = abox_component_control_info, \
	.get = abox_component_control_get, \
	.put = abox_component_control_put, \
	.private_value = \
		(unsigned long)&(struct abox_component_kcontrol_value) \
		{.desc = xdesc, .control = xcontrol} }

struct snd_kcontrol_new abox_component_kcontrols[] = {
	ABOX_COMPONENT_KCONTROL(NULL, NULL, NULL),
};

static int __abox_register_component_work_func(struct device *dev,
		struct abox_data *data, struct abox_component *component)
{
	int i;
	struct ABOX_COMPONENT_DESCRIPTIOR *desc = component->desc;

	if (!component->desc || component->registered)
		return -EEXIST;

	component->registered = true;
	for (i = 0; i < desc->control_count; i++) {
		struct ABOX_COMPONENT_CONTROL *control = &desc->controls[i];
		struct abox_component_kcontrol_value *value;
		char kcontrol_name[64];

		value = devm_kmalloc(dev, sizeof(*value), GFP_KERNEL);
		if (IS_ERR_OR_NULL(value)) {
			dev_err(dev, "%s: kmalloc fail\n", __func__);
			continue;
		}
		value->desc = desc;
		value->control = control;

		snprintf(kcontrol_name, sizeof(kcontrol_name), "%s %s",
				desc->name, control->name);

		abox_component_kcontrols[0].name =
				devm_kstrdup(dev, kcontrol_name, GFP_KERNEL);
		abox_component_kcontrols[0].private_value =
				(unsigned long)value;
		if (data->codec) {
			snd_soc_add_codec_controls(data->codec,
					abox_component_kcontrols, 1);
		}
	}
	return 0;
}

static void abox_register_component_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			register_component_work);
	struct device *dev = &data->pdev->dev;
	struct abox_component *component;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	for (component = data->components;
			((component - data->components) <
			ARRAY_SIZE(data->components));
			component++) {
		ret = __abox_register_component_work_func(dev, data, component);
		if (ret == -EEXIST)
			continue;
	}
}


static int abox_register_component(struct device *dev,
		struct ABOX_COMPONENT_DESCRIPTIOR *desc)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_component *component;

	dev_dbg(dev, "%s(%d, %s)\n", __func__, desc->id, desc->name);

	for (component = data->components;
			((component - data->components) <
			ARRAY_SIZE(data->components)) &&
			component->desc && component->desc != desc;
			component++) {
	}

	if (component->desc == NULL) {
		component->desc = desc;
		schedule_work(&data->register_component_work);
	}

	return 0;
}

static bool abox_is_calliope_incompatible(struct platform_device *pdev)
{
	struct abox_data *data = platform_get_drvdata(pdev);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	memcpy(&msg, data->sram_base + 0x30040, 0x3C);

	return ((system_msg->param3 >> 24) == 'A');
}

static void abox_restore_output_rate(struct device *dev,
		struct abox_data *data, enum ABOX_CONFIGMSG msg)
{
	abox_output_rate_put_ipc(dev, data->out_rate[msg], msg);
}

static void abox_restore_erap_status(struct device *dev,
		struct abox_data *data, enum ABOX_ERAP_TYPE type)
{
	abox_erap_handler_put_ipc(dev, type, data->erap_status[type]);
}

static void abox_restore_data(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	abox_restore_output_rate(dev, data, SET_MIXER_SAMPLE_RATE);
	abox_restore_output_rate(dev, data, SET_OUT1_SAMPLE_RATE);
	abox_restore_output_rate(dev, data, SET_OUT2_SAMPLE_RATE);
	abox_restore_output_rate(dev, data, SET_RECP_SAMPLE_RATE);
	abox_restore_output_rate(dev, data, SET_INMUX0_SAMPLE_RATE);
	abox_restore_output_rate(dev, data, SET_INMUX1_SAMPLE_RATE);
	abox_restore_output_rate(dev, data, SET_INMUX2_SAMPLE_RATE);
	abox_restore_output_rate(dev, data, SET_INMUX3_SAMPLE_RATE);
	abox_restore_output_rate(dev, data, SET_INMUX4_SAMPLE_RATE);

	abox_synchronize_put_ipc(dev, data->rdma_synchronizer[0], 0);
	abox_synchronize_put_ipc(dev, data->rdma_synchronizer[1], 1);
	abox_synchronize_put_ipc(dev, data->rdma_synchronizer[2], 2);
	abox_synchronize_put_ipc(dev, data->rdma_synchronizer[3], 3);
	abox_synchronize_put_ipc(dev, data->rdma_synchronizer[4], 4);
	abox_synchronize_put_ipc(dev, data->rdma_synchronizer[5], 5);
	abox_synchronize_put_ipc(dev, data->rdma_synchronizer[6], 6);
	abox_synchronize_put_ipc(dev, data->rdma_synchronizer[7], 7);

	abox_restore_erap_status(dev, data, ERAP_ECHO_CANCEL);
	abox_restore_erap_status(dev, data, ERAP_VI_SENSE);
}

static void abox_boot_done_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			boot_done_work);
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "%s\n", __func__);

	abox_cpu_pm_ipc(dev, true);
	abox_restore_data(dev);
	abox_request_cpu_gear(dev, data, DEFAULT_CPU_GEAR_ID, 12);
	abox_request_dram_on(pdev, dev, false);
	wake_unlock(&data->wake_lock);
}

static void abox_boot_done(struct device *dev, unsigned int version)
{
	struct abox_data *data = dev_get_drvdata(dev);
	char ver_char[4];

	dev_dbg(dev, "%s\n", __func__);

	data->calliope_version = version;
	memcpy(ver_char, &version, sizeof(ver_char));
	dev_info(dev, "Calliope is ready to sing (version:%c%c%c%c)\n",
			ver_char[3], ver_char[2], ver_char[1], ver_char[0]);

	schedule_work(&data->boot_done_work);
	data->calliope_state = CALLIOPE_ENABLED;
}

static irqreturn_t abox_dma_irq_handler(int irq, struct abox_data *data)
{
	struct device *dev = &data->pdev->dev;
	int id;
	struct platform_device **pdev_dma;
	struct abox_platform_data *platform_data;

	dev_dbg(dev, "%s(%d)\n", __func__, irq);

	switch (irq) {
	case RDMA0_BUF_EMPTY:
		id = 0;
		pdev_dma = data->pdev_rdma;
		break;
	case RDMA1_BUF_EMPTY:
		id = 1;
		pdev_dma = data->pdev_rdma;
		break;
	case RDMA2_BUF_EMPTY:
		id = 2;
		pdev_dma = data->pdev_rdma;
		break;
	case RDMA3_BUF_EMPTY:
		id = 3;
		pdev_dma = data->pdev_rdma;
		break;
	case WDMA0_BUF_FULL:
		id = 0;
		pdev_dma = data->pdev_wdma;
		break;
	case WDMA1_BUF_FULL:
		id = 1;
		pdev_dma = data->pdev_wdma;
		break;
	default:
		return IRQ_NONE;
	}

	if (unlikely(!pdev_dma[id])) {
		dev_err(dev, "spurious dma irq: irq=%d id=%d\n", irq, id);
		return IRQ_HANDLED;
	}

	platform_data = platform_get_drvdata(pdev_dma[id]);
	if (unlikely(!platform_data)) {
		dev_err(dev, "dma irq with null data: irq=%d id=%d\n", irq, id);
		return IRQ_HANDLED;
	}

	platform_data->pointer = 0;
	snd_pcm_period_elapsed(platform_data->substream);

	return IRQ_HANDLED;
}

static void abox_system_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_SYSTEM_MSG *system_msg = &msg->msg.system;
	struct platform_device *pdev = to_platform_device(dev);
	struct abox_irq_action *irq_action;
	int result;

	dev_dbg(dev, "msgtype=%d\n", system_msg->msgtype);

	switch (system_msg->msgtype) {
	case ABOX_BOOT_DONE:
		if (abox_is_calliope_incompatible(pdev))
			dev_err(dev, "Calliope is not compatible with the driver\n");

		abox_boot_done(dev, system_msg->param3);

		list_for_each_entry(irq_action, &data->irq_actions, list) {
			if (irq_action->irq == IPC_SYSTEM) {
				irq_action->irq_handler(IPC_SYSTEM,
						irq_action->dev_id, msg);
			}
		}
		break;
	case ABOX_CHANGE_GEAR:
		abox_request_cpu_gear(dev, data, system_msg->param2,
				system_msg->param1);
		break;
	case ABOX_END_L2C_CONTROL:
		data->l2c_controlled = true;
		wake_up(&data->ipc_wait_queue);
		break;
	case ABOX_REQUEST_L2C:
	{
		void *id = (void *)(long)system_msg->param2;
		bool on = !!system_msg->param1;

		abox_request_l2c(dev, data, id, on);
		break;
	}
	case ABOX_REQUEST_SYSCLK:
		abox_request_mif_freq(dev, system_msg->param1);
		break;
	case ABOX_REPORT_LOG:
		result = abox_log_register_buffer(dev, system_msg->param1,
				abox_addr_to_kernel_addr(data,
				system_msg->param2));
		if (IS_ERR_VALUE(result)) {
			dev_err(dev, "log buffer registration failed: %u, %u\n",
					system_msg->param1, system_msg->param2);
		}
		break;
	case ABOX_FLUSH_LOG:
		break;
	case ABOX_REPORT_DUMP:
		result = abox_dump_register_buffer(dev, system_msg->param1,
				system_msg->bundle.param_bundle,
				abox_addr_to_kernel_addr(data,
				system_msg->param2),
				abox_addr_to_phys_addr(data,
				system_msg->param2),
				system_msg->param3);
		if (IS_ERR_VALUE(result)) {
			dev_err(dev, "dump buffer registration failed: %u, %u\n",
					system_msg->param1, system_msg->param2);
		}
		break;
	case ABOX_FLUSH_DUMP:
		abox_dump_period_elapsed(system_msg->param1,
				system_msg->param2);
		break;
	case ABOX_END_CLAIM_SRAM:
		data->ima_claimed = true;
		wake_up(&data->ipc_wait_queue);
		break;
	case ABOX_END_RECLAIM_SRAM:
		data->ima_claimed = false;
		wake_up(&data->ipc_wait_queue);
		break;
	case ABOX_REPORT_COMPONENT:
		abox_register_component(dev, abox_addr_to_kernel_addr(data,
				system_msg->param1));
		break;
	case ABOX_REPORT_COMPONENT_CONTROL:
		abox_component_control_get_msg = *msg;
		wake_up(&data->ipc_wait_queue);
		break;
	case ABOX_REPORT_FAULT:
	{
		const char *type;
		unsigned int *addr;

		switch (system_msg->param1) {
		case 1:
			type = "data abort";
			break;
		case 2:
			type = "prefetch abort";
			break;
		case 3:
			type = "os error";
			break;
		case 4:
			type = "vss error";
			break;
		default:
			type = "unknown error";
			break;
		}
		dev_err(dev, "%s(%08X, %08X, %08X) is reported from calliope\n",
				type, system_msg->param1, system_msg->param2,
				system_msg->param3);

		switch (system_msg->param1) {
		case 1:
		case 2:
			addr = abox_addr_to_kernel_addr(data,
					system_msg->bundle.param_s32[0]);
			abox_dbg_print_gpr_from_addr(dev, data, addr);
			abox_dbg_dump_gpr_from_addr(dev, addr,
					ABOX_DBG_DUMP_FIRMWARE, type);
			abox_dbg_dump_mem(dev, data,
					ABOX_DBG_DUMP_FIRMWARE, type);
			break;
		case 4:
			abox_dbg_print_gpr(dev, data);
			abox_dbg_dump_gpr(dev, data, ABOX_DBG_DUMP_VSS, type);
			abox_dbg_dump_mem(dev, data, ABOX_DBG_DUMP_VSS, type);
			break;
		default:
			abox_dbg_print_gpr(dev, data);
			abox_dbg_dump_gpr(dev, data,
					ABOX_DBG_DUMP_FIRMWARE, type);
			abox_dbg_dump_mem(dev, data,
					ABOX_DBG_DUMP_FIRMWARE, type);
			break;
		}
		break;
	}
	default:
		dev_warn(dev, "Redundant system message: %d(%d, %d, %d)\n",
				system_msg->msgtype, system_msg->param1,
				system_msg->param2, system_msg->param3);
		break;
	}
}

static void abox_playback_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg->msg.pcmtask;
	struct abox_platform_data *platform_data;
	struct platform_device *pdev;
	int id;

	dev_dbg(dev, "msgtype=%d\n", pcmtask_msg->msgtype);

	switch (pcmtask_msg->msgtype) {
	case PCM_PLTDAI_POINTER:
		id = pcmtask_msg->channel_id;
		pdev = data->pdev_rdma[id];
		if (likely(id < ARRAY_SIZE(data->pdev_rdma)) && pdev) {
			platform_data = platform_get_drvdata(pdev);
			platform_data->pointer = pcmtask_msg->param.pointer;
			snd_pcm_period_elapsed(platform_data->substream);
		} else {
			dev_err(dev, "pcm playback irq: id=%d\n", id);
		}
		break;
	default:
		dev_warn(dev, "Redundant pcmtask message: %d\n",
				pcmtask_msg->msgtype);
		break;
	}
}

static void abox_capture_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg->msg.pcmtask;
	struct abox_platform_data *platform_data;
	struct platform_device *pdev;
	int id;

	dev_dbg(dev, "msgtype=%d\n", pcmtask_msg->msgtype);

	switch (pcmtask_msg->msgtype) {
	case PCM_PLTDAI_POINTER:
		id = pcmtask_msg->channel_id;
		pdev = data->pdev_wdma[id];
		if (likely(id < ARRAY_SIZE(data->pdev_wdma)) && pdev) {
			platform_data = platform_get_drvdata(pdev);
			platform_data->pointer = pcmtask_msg->param.pointer;
			snd_pcm_period_elapsed(platform_data->substream);
		} else {
			dev_err(dev, "pcm capture irq: id=%d\n", id);
		}
		break;
	default:
		dev_warn(dev, "Redundant pcmtask message: %d\n",
				pcmtask_msg->msgtype);
		break;
	}
}

static void abox_offload_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_OFFLOADTASK_MSG *offloadtask_msg = &msg->msg.offload;
	int id = offloadtask_msg->channel_id;
	struct abox_platform_data *platform_data;

	if (id != 5) {
		dev_warn(dev, "%s: unknown channel id(%d)\n", __func__, id);
		id = 5;
	}

	platform_data = platform_get_drvdata(data->pdev_rdma[id]);
	if (platform_data->compr_data.isr_handler)
		platform_data->compr_data.isr_handler(data->pdev_rdma[id]);
	else
		dev_warn(dev, "Redundant offload message on rdma[%d]", id);
}

static irqreturn_t abox_irq_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct abox_data *data = platform_get_drvdata(pdev);
	struct abox_irq_action *irq_action;
	ABOX_IPC_MSG msg;
	irqreturn_t ret = IRQ_HANDLED;

	if (abox_dma_irq_handler(irq, data) == IRQ_HANDLED)
		return IRQ_HANDLED;

	memcpy(&msg, data->sram_base + data->ipc_rx_offset, sizeof(msg));
	writel(0, data->sram_base + data->ipc_rx_ack_offset);

	dev_dbg(dev, "%s: irq=%d, ipcid=%d\n", __func__, irq, msg.ipcid);

	switch (msg.ipcid) {
	case IPC_RECEIVED:
		break;
	case IPC_SYSTEM:
		abox_system_ipc_handler(dev, data, &msg);
		break;
	case IPC_PCMPLAYBACK:
		abox_playback_ipc_handler(dev, data, &msg);
		break;
	case IPC_PCMCAPTURE:
		abox_capture_ipc_handler(dev, data, &msg);
		break;
	case IPC_OFFLOAD:
		abox_offload_ipc_handler(dev, data, &msg);
		break;
	default:
		list_for_each_entry(irq_action, &data->irq_actions, list) {
			if (irq_action->irq == msg.ipcid) {
				if (irq_action->irq_handler(msg.ipcid,
						irq_action->dev_id, &msg) ==
						IRQ_HANDLED)
					break;
			}
		}
		break;
	}

	abox_log_schedule_flush_all(dev);

	dev_dbg(dev, "%s: exit\n", __func__);
	return ret;
}

static int abox_cpu_pm_ipc(struct device *dev, bool resume)
{
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system = &msg.msg.system;
	int result;

	dev_dbg(dev, "%s\n", __func__);

	msg.ipcid = IPC_SYSTEM;
	system->msgtype = resume ? ABOX_RESUME : ABOX_SUSPEND;
	result = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 1, 1);
	if (!IS_ERR_VALUE(result) && !resume) {
		int i = 1000;
		unsigned val;

		do {
			exynos_pmu_read(ABOX_CA7_STATUS, &val);
		} while (i-- && !(val & ABOX_CA7_STATUS_STANDBYWFI_MASK));

		if (!i)
			dev_warn(dev, "calliope suspend time out\n");
	}

	return result;
}

static void abox_pad_retention(bool retention)
{
	if (retention) {
#ifndef EMULATOR
		exynos_pmu_update(GPIO_MODE_ABOX_SYS_PWR_REG, 0x1, 0x1);
#else
		update_mask_value(pmu_alive + GPIO_MODE_ABOX_SYS_PWR_REG,
				0x1, 0x1);
#endif
	} else {
#ifndef EMULATOR
		exynos_pmu_update(PAD_RETENTION_ABOX_OPTION,
				0x10000000, 0x10000000);
		exynos_pmu_update(GPIO_MODE_ABOX_SYS_PWR_REG, 0x1, 0x1);
#else
		update_mask_value(pmu_alive + PAD_RETENTION_ABOX_OPTION,
				0x10000000, 0x10000000);
		update_mask_value(pmu_alive + GPIO_MODE_ABOX_SYS_PWR_REG,
				0x1, 0x1);
#endif
	}
}

static void abox_cpu_power(bool on)
{
	pr_info("%s(%d)\n", __func__, on);

#ifndef EMULATOR
	exynos_pmu_update(ABOX_CA7_CONFIGURATION, ABOX_CA7_LOCAL_PWR_CFG,
			on ? ABOX_CA7_LOCAL_PWR_CFG : 0);
#else
	update_mask_value(pmu_alive + ABOX_CA7_CONFIGURATION,
			ABOX_CA7_LOCAL_PWR_CFG,
			on ? ABOX_CA7_LOCAL_PWR_CFG : 0);
#endif
}

static int abox_cpu_enable(bool enable)
{
	unsigned int mask = ABOX_CA7_OPTION_ENABLE_CPU_MASK;
	unsigned int val = (enable ? mask : 0);
	unsigned int status = 0;
	unsigned long after;


	pr_info("%s(%d)\n", __func__, enable);

#ifndef EMULATOR
	exynos_pmu_update(ABOX_CA7_OPTION, mask, val);
#else
	update_mask_value(pmu_alive + ABOX_CA7_OPTION, mask, val);
#endif
	if (enable) {
		after = jiffies + LIMIT_IN_JIFFIES;
		do {
#ifndef EMULATOR
			exynos_pmu_read(ABOX_CA7_STATUS, &status);
#else
			status = readl(pmu_alive + ABOX_CA7_STATUS);
#endif
		} while (((status & ABOX_CA7_STATUS_STATUS_MASK)
				!= ABOX_CA7_STATUS_STATUS_MASK)
				&& time_is_after_eq_jiffies(after));
		if (time_is_before_jiffies(after)) {
			pr_err("abox cpu enable timeout\n");
			return -ETIME;
		}
	}

	return 0;

}

static void abox_save_register(struct abox_data *data)
{
	regcache_cache_only(data->regmap, true);
	regcache_mark_dirty(data->regmap);
}

static void abox_restore_register(struct abox_data *data)
{
	regcache_cache_only(data->regmap, false);
	regcache_sync(data->regmap);
}

static void abox_reload_extra_firmware(struct abox_data *data, const char *name)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	struct abox_extra_firmware *ext_fw;
	int result;

	dev_dbg(dev, "%s(%s)\n", __func__, name);

	for (ext_fw = data->firmware_extra;
			ext_fw->name && ext_fw->firmware &&
			ext_fw - data->firmware_extra <
			ARRAY_SIZE(data->firmware_extra);
			ext_fw++) {
		void __iomem *base;
		size_t size;

		if (strcmp(ext_fw->name, name) != 0)
			continue;

		/* request */
		release_firmware(ext_fw->firmware);
		result = request_firmware(&ext_fw->firmware, ext_fw->name, dev);
		if (IS_ERR_VALUE(result)) {
			dev_err(dev, "%s: %s request failed\n", __func__,
					ext_fw->name);
			break;
		}

		/* download */
		switch (ext_fw->area) {
		case 0:
			base = data->sram_base;
			size = data->sram_size;
			break;
		case 1:
			base = data->dram_base;
			size = DRAM_FIRMWARE_SIZE;
			break;
		case 2:
			base = phys_to_virt(shm_get_phys_base() +
					shm_get_cp_size());
			size = shm_get_vss_size();
			break;
		default:
			dev_err(dev, "%s: area is invalid name=%s, area=%u, offset=%u\n",
					__func__, ext_fw->name, ext_fw->area,
					ext_fw->offset);
			continue;
		}

		if (ext_fw->offset + ext_fw->firmware->size > size) {
			dev_err(dev, "%s: firmware is too large name=%s, area=%u, offset=%u\n",
					__func__, ext_fw->name, ext_fw->area,
					ext_fw->offset);
			break;
		}

		memcpy(base + ext_fw->offset, ext_fw->firmware->data,
				ext_fw->firmware->size);
		dev_info(dev, "%s: %s is downloaded at area %u offset %u\n",
				__func__, ext_fw->name, ext_fw->area,
				ext_fw->offset);
		break;
	}
}

static void abox_request_extra_firmware(struct abox_data *data)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	int i = 0, result;

	dev_dbg(dev, "%s\n", __func__);

	for_each_child_of_node(np, child_np) {
		struct abox_extra_firmware *ext_fw = &data->firmware_extra[i];
		const char *status;

		status = of_get_property(child_np, "status", NULL);
		if (status && strcmp("okay", status) && strcmp("ok", status))
			continue;

		result = of_property_read_string(child_np, "samsung,name",
				&ext_fw->name);
		if (IS_ERR_VALUE(result))
			continue;

		result = of_property_read_u32(child_np, "samsung,area",
				&ext_fw->area);
		if (IS_ERR_VALUE(result))
			continue;

		result = of_property_read_u32(child_np, "samsung,offset",
				&ext_fw->offset);
		if (IS_ERR_VALUE(result))
			continue;

		dev_dbg(dev, "%s: name=%s, area=%u, offset=%u\n", __func__,
				ext_fw->name, ext_fw->area, ext_fw->offset);

		release_firmware(ext_fw->firmware);
		result = request_firmware(&ext_fw->firmware, ext_fw->name, dev);
		if (IS_ERR_VALUE(result)) {
			dev_err(dev, "%s: %s request failed\n", __func__,
					ext_fw->name);
			continue;
		}

		i++;
	}

}

static void abox_download_extra_firmware(struct abox_data *data)
{
	struct device *dev = &data->pdev->dev;
	struct abox_extra_firmware *ext_fw;
	void __iomem *base;
	size_t size;

	dev_dbg(dev, "%s\n", __func__);

	for (ext_fw = data->firmware_extra;
			ext_fw->name && ext_fw->firmware &&
			ext_fw - data->firmware_extra <
			ARRAY_SIZE(data->firmware_extra);
			ext_fw++) {
		switch (ext_fw->area) {
		case 0:
			base = data->sram_base;
			size = data->sram_size;
			break;
		case 1:
			base = data->dram_base;
			size = DRAM_FIRMWARE_SIZE;
			break;
		case 2:
			base = phys_to_virt(shm_get_phys_base() +
					shm_get_cp_size());
			size = shm_get_vss_size();
			break;
		default:
			dev_err(dev, "%s: area is invalid name=%s, area=%u, offset=%u\n",
					__func__, ext_fw->name, ext_fw->area,
					ext_fw->offset);
			continue;
		}

		if (ext_fw->offset + ext_fw->firmware->size > size) {
			dev_err(dev, "%s: firmware is too large name=%s, area=%u, offset=%u\n",
					__func__, ext_fw->name, ext_fw->area,
					ext_fw->offset);
			continue;
		}

		memcpy(base + ext_fw->offset, ext_fw->firmware->data,
				ext_fw->firmware->size);
		dev_info(dev, "%s: %s is downloaded at area %u offset %u\n",
				__func__, ext_fw->name, ext_fw->area,
				ext_fw->offset);
	}
}

static int abox_download_firmware(struct platform_device *pdev)
{
	struct abox_data *data = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	dev_info(dev, "%s\n", __func__);

	if (!data->firmware_sram) {
		dev_warn(dev, "SRAM firmware downloading is deferred\n");
		return -EAGAIN;
	}
	memset_io(data->sram_base, 0, data->sram_size);
	memcpy(data->sram_base, data->firmware_sram->data,
			data->firmware_sram->size);

	if (!data->firmware_dram) {
		dev_warn(dev, "DRAM firmware downloading is defferred\n");
		return -EAGAIN;
	}
	memcpy(data->dram_base, data->firmware_dram->data,
			data->firmware_dram->size);

	if (!data->firmware_iva) {
		dev_warn(dev, "IVA firmware is not loaded\n");
	} else {
		memcpy(data->iva_base, data->firmware_iva->data,
				data->firmware_iva->size);
	}

	abox_request_extra_firmware(data);
	abox_download_extra_firmware(data);

	return 0;
}

static void abox_cfg_gpio(struct device *dev, const char *name)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct pinctrl_state *pin_state;
	int ret;

	dev_info(dev, "%s(%s)\n", __func__, name);

	pin_state = pinctrl_lookup_state(data->pinctrl, name);
	if (IS_ERR(pin_state)) {
		dev_err(dev, "Couldn't find pinctrl %s\n", name);
	} else {
		ret = pinctrl_select_state(data->pinctrl, pin_state);
		if (ret < 0)
			dev_err(dev, "Unable to configure pinctrl %s\n", name);
	}
}

#undef MANUAL_SECURITY_CHANGE
#ifdef MANUAL_SECURITY_CHANGE
static void work_temp_function(struct work_struct *work)
{
	exynos_smc(0x82000701, 0, 0, 0);
	pr_err("%s: ABOX_CA7 security changed!!!\n", __func__);
}
static DECLARE_DELAYED_WORK(work_temp, work_temp_function);
#endif

int abox_ima_claim(struct device *dev, struct abox_data *data,
		phys_addr_t *addr)
{
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	phys_addr_t paddr;
	int result;

	dev_info(dev, "%s\n", __func__);

	mutex_lock(&data->ima_lock);

	if (data->ima_claimed) {
		mutex_unlock(&data->ima_lock);
		return 0;
	}

	data->ima_vaddr = ima_alloc(data->ima_client, IVA_FIRMWARE_SIZE, 0);
	if (IS_ERR_OR_NULL(data->ima_vaddr)) {
		dev_err(dev, "%s: ima_alloc failed: %ld\n", __func__,
				PTR_ERR(data->ima_vaddr));
		result = data->ima_vaddr ? PTR_ERR(data->ima_vaddr) : -ENOMEM;
		goto error;
	}
	paddr = ima_get_dma_addr(data->ima_client, data->ima_vaddr);
	if (addr)
		*addr = paddr;

	result = iommu_map(data->iommu_domain,
			IOVA_IVA(ABOX_IVA_MEMORY_PREPARE), paddr,
			IVA_FIRMWARE_SIZE, 0);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "%s: iommu mapping failed(%d)\n", __func__,
				result);
		goto error;
	}

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_START_CLAIM_SRAM;
	system_msg->param1 = ABOX_IVA_MEMORY_PREPARE;
	system_msg->param2 = IOVA_IVA(ABOX_IVA_MEMORY_PREPARE);
	system_msg->param3 = IVA_FIRMWARE_SIZE;
	result = abox_request_ipc(&data->pdev->dev, msg.ipcid, &msg,
			sizeof(msg), 0, 0);
	if (IS_ERR_VALUE(result))
		goto error;

	result = wait_event_timeout(data->ipc_wait_queue,
			data->ima_claimed, msecs_to_jiffies(1000));
	if (data->ima_claimed) {
		result = 0;
	} else {
		dev_err(dev, "IVA memory claim failed\n");
		result = -ETIME;
		goto error;
	}

	iommu_unmap(data->iommu_domain, IOVA_IVA(ABOX_IVA_MEMORY),
			IVA_FIRMWARE_SIZE);
	iommu_unmap(data->iommu_domain, IOVA_IVA(ABOX_IVA_MEMORY_PREPARE),
			IVA_FIRMWARE_SIZE);
	exynos_sysmmu_tlb_invalidate(data->iommu_domain,
			IOVA_IVA(ABOX_IVA_MEMORY), IVA_FIRMWARE_SIZE);
	exynos_sysmmu_tlb_invalidate(data->iommu_domain,
			IOVA_IVA(ABOX_IVA_MEMORY_PREPARE), IVA_FIRMWARE_SIZE);

	result = iommu_map(data->iommu_domain, IOVA_IVA(ABOX_IVA_MEMORY),
			paddr, IVA_FIRMWARE_SIZE, 0);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "%s: iommu map failed(%d)\n", __func__, result);
		goto error;
	}
	result = iommu_map(data->iommu_domain,
			IOVA_IVA(ABOX_IVA_MEMORY_PREPARE),
			data->iva_base_phys, IVA_FIRMWARE_SIZE, 0);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "%s: iommu map failed(%d)\n", __func__, result);
		goto error;
	}

	system_msg->msgtype = ABOX_REPORT_SRAM;
	system_msg->param1 = ABOX_IVA_MEMORY;
	system_msg->param2 = IOVA_IVA(ABOX_IVA_MEMORY);
	system_msg->param3 = IVA_FIRMWARE_SIZE;
	result = abox_request_ipc(&data->pdev->dev, msg.ipcid, &msg,
			sizeof(msg), 0, 1);
	if (IS_ERR_VALUE(result))
		goto error;

	mutex_unlock(&data->ima_lock);
	return result;

error:
	iommu_unmap(data->iommu_domain, IOVA_IVA(ABOX_IVA_MEMORY),
			IVA_FIRMWARE_SIZE);
	iommu_unmap(data->iommu_domain, IOVA_IVA(ABOX_IVA_MEMORY_PREPARE),
			IVA_FIRMWARE_SIZE);
	exynos_sysmmu_tlb_invalidate(data->iommu_domain,
			IOVA_IVA(ABOX_IVA_MEMORY), IVA_FIRMWARE_SIZE);
	exynos_sysmmu_tlb_invalidate(data->iommu_domain,
			IOVA_IVA(ABOX_IVA_MEMORY_PREPARE), IVA_FIRMWARE_SIZE);
	iommu_map(data->iommu_domain, IOVA_IVA(ABOX_IVA_MEMORY),
			data->iva_base_phys, IVA_FIRMWARE_SIZE, 0);
	ima_free(data->ima_client, data->ima_vaddr);
	mutex_unlock(&data->ima_lock);
	return result;
}

static int abox_ima_reclaim(struct ima_client *client, struct device *dev,
		void *priv)
{
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	struct abox_data *data = priv;
	long result;

	dev_info(dev, "%s\n", __func__);

	mutex_lock(&data->ima_lock);

	if (!data->ima_claimed) {
		result = 0;
		goto error;
	}

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_START_RECLAIM_SRAM;
	system_msg->param1 = ABOX_IVA_MEMORY;
	system_msg->param2 = IOVA_IVA(ABOX_IVA_MEMORY_PREPARE);
	system_msg->param3 = IVA_FIRMWARE_SIZE;

	abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);

	result = wait_event_timeout(data->ipc_wait_queue,
			!data->ima_claimed, msecs_to_jiffies(1000));
	if (!data->ima_claimed) {
		result = 0;
	} else {
		dev_err(dev, "IVA memory reclamation failed\n");
		result = -ETIME;
	}

	iommu_unmap(data->iommu_domain, IOVA_IVA(ABOX_IVA_MEMORY),
			IVA_FIRMWARE_SIZE);
	iommu_unmap(data->iommu_domain, IOVA_IVA(ABOX_IVA_MEMORY_PREPARE),
			IVA_FIRMWARE_SIZE);
	exynos_sysmmu_tlb_invalidate(data->iommu_domain,
			IOVA_IVA(ABOX_IVA_MEMORY), IVA_FIRMWARE_SIZE);
	exynos_sysmmu_tlb_invalidate(data->iommu_domain,
			IOVA_IVA(ABOX_IVA_MEMORY_PREPARE), IVA_FIRMWARE_SIZE);

	result = iommu_map(data->iommu_domain, IOVA_IVA(ABOX_IVA_MEMORY),
			data->iva_base_phys, IVA_FIRMWARE_SIZE, 0);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "%s: iommu mapping failed(%ld)\n", __func__,
				result);
		goto error;
	}

	ima_free(data->ima_client, data->ima_vaddr);

	system_msg->msgtype = ABOX_REPORT_DRAM;
	system_msg->param1 = ABOX_IVA_MEMORY;
	system_msg->param2 = IOVA_IVA(ABOX_IVA_MEMORY);
	system_msg->param3 = IVA_FIRMWARE_SIZE;
	result = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (IS_ERR_VALUE(result))
		goto error;

error:
	mutex_unlock(&data->ima_lock);
	return (int)result;
}

static int abox_ima_init(struct device *dev, struct abox_data *data)
{
	dev_dbg(dev, "%s\n", __func__);

	mutex_init(&data->ima_lock);
	data->ima_client = ima_create_client(dev, abox_ima_reclaim, data);
	if (IS_ERR(data->ima_client)) {
		dev_err(dev, "ima_create_client failed: %ld\n",
				PTR_ERR(data->ima_client));
		return PTR_ERR(data->ima_client);
	}

	return 0;
}

static void __abox_control_l2c(struct abox_data *data, bool enable)
{
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	struct device *dev = &data->pdev->dev;

	if (data->l2c_enabled == enable)
		return;

	dev_info(dev, "%s(%d)\n", __func__, enable);

	data->l2c_controlled = false;

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_START_L2C_CONTROL;
	system_msg->param1 = enable ? 1 : 0;

	if (enable) {
		vts_acquire_sram(data->pdev_vts, 0);

		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 1, 0);
		wait_event_timeout(data->ipc_wait_queue,
				data->l2c_controlled, LIMIT_IN_JIFFIES);
		if (!data->l2c_controlled)
			dev_err(dev, "l2c enable failed\n");
	} else {
		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 1, 0);
		wait_event_timeout(data->ipc_wait_queue,
				data->l2c_controlled, LIMIT_IN_JIFFIES);
		if (!data->l2c_controlled)
			dev_err(dev, "l2c disable failed\n");

		vts_release_sram(data->pdev_vts, 0);
	}

	data->l2c_enabled = enable;
}

static void abox_l2c_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data, l2c_work);
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	size_t length = ARRAY_SIZE(data->l2c_requests);
	struct abox_l2c_request *request;
	bool enable = false;

	dev_dbg(dev, "%s\n", __func__);

	for (request = data->l2c_requests;
			request - data->l2c_requests < length
			&& request->id;
			request++) {
		if (request->on) {
			enable = true;
			break;
		}
	}

	__abox_control_l2c(data, enable);
}

int abox_request_l2c(struct device *dev, struct abox_data *data,
		void *id, bool on)
{
	struct abox_l2c_request *request;
	size_t length = ARRAY_SIZE(data->l2c_requests);

	dev_info(dev, "%s(%#lx, %d)\n", __func__, (unsigned long)id, on);

	for (request = data->l2c_requests;
			request - data->l2c_requests < length
			&& request->id && request->id != id;
			request++) {
	}

	request->on = on;
	wmb(); /* value is read only when id is valid */
	request->id = id;

	if (request - data->l2c_requests >= ARRAY_SIZE(data->l2c_requests)) {
		dev_err(dev, "%s: out of index. id=%#lx, on=%d\n",
				__func__, (unsigned long)id, on);
		return -ENOMEM;
	}

	schedule_work(&data->l2c_work);

	return 0;
}

int abox_request_l2c_sync(struct device *dev, struct abox_data *data,
		void *id, bool on)
{
	abox_request_l2c(dev, data, id, on);
	flush_work(&data->l2c_work);
	return 0;
}

static void abox_clear_l2c_requests(struct device *dev, struct abox_data *data)
{
	struct abox_l2c_request *req;
	size_t len = ARRAY_SIZE(data->l2c_requests);

	dev_info(dev, "%s\n", __func__);

	for (req = data->l2c_requests; req - data->l2c_requests < len &&
			req->id; req++) {
		req->on = false;
	}

	__abox_control_l2c(data, false);
}

static int abox_enable(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int i, value;
	int result = 0;

	dev_info(dev, "%s\n", __func__);

	abox_gic_enable_irq(&data->pdev_gic->dev);

	abox_cpu_enable(false);
	abox_cpu_power(false);

	abox_request_cpu_gear_sync(dev, data, DEFAULT_CPU_GEAR_ID, 3);

	if (is_secure_gic()) {
		exynos_pmu_write(ABOX_MAGIC, 0);
		result = exynos_smc(0x82000501, 0, 0, 0);
		dev_dbg(dev, "%s: smc result=%d\n", __func__, result);

		for (i = 1000; i; i--) {
			exynos_pmu_read(ABOX_MAGIC, &value);
			if (value == ABOX_MAGIC_VALUE)
				break;
		}
		if (value != ABOX_MAGIC_VALUE)
			dev_warn(dev, "%s: abox magic timeout\n", __func__);

		abox_cpu_enable(false);
		abox_cpu_power(false);
	}

	writel(0x1, data->sysreg_base + ABOX_SYSREG_MISC_CON);
	writel(0x1, data->sysreg_base + ABOX_SYSREG_L2_CACHE_CON);

	result = clk_enable(data->clk_ca7);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Failed to enable cpu clock: %d\n", result);
		goto error;
	}

	result = clk_set_rate(data->clk_audif, AUDIF_RATE_HZ);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Failed to set audif clock: %d\n", result);
		goto error;
	}
	dev_info(dev, "audif clock: %lu\n", clk_get_rate(data->clk_audif));

	result = clk_enable(data->clk_audif);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "Failed to enable audif clock: %d\n", result);
		goto error;
	}

	abox_restore_register(data);

	abox_gic_init_gic(data->pdev_gic);

	abox_cfg_gpio(dev, "default");

	result = abox_download_firmware(pdev);
	if (IS_ERR_VALUE(result)) {
		if (result != -EAGAIN)
			dev_err(dev, "Failed to download firmware\n");
		goto error;
	}

	abox_request_dram_on(pdev, dev, true);
	abox_cpu_power(true);
	abox_cpu_enable(true);
	data->calliope_state = CALLIOPE_ENABLING;

	abox_pad_retention(false);
#ifdef MANUAL_SECURITY_CHANGE
	schedule_delayed_work(&work_temp, msecs_to_jiffies(3000));
#endif

	data->enabled = true;
	wake_lock_timeout(&data->wake_lock, BOOT_DONE_TIMEOUT_MS);
error:
	return result;
}

static int abox_disable(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	int i;
	unsigned long flag;

	dev_info(dev, "%s\n", __func__);

	switch (data->calliope_state) {
	case CALLIOPE_ENABLED:
		break;
	case CALLIOPE_ENABLING:
		for (i = CALLIOPE_ENABLE_TIMEOUT_MS;
				i && (data->calliope_state != CALLIOPE_ENABLED);
				i--)
			mdelay(1);

		if (data->calliope_state == CALLIOPE_ENABLED)
			break;
		/* Fallthrough */
	default:
		dev_warn(dev, "Invalid calliope state: %d\n",
				data->calliope_state);
		data->calliope_state = CALLIOPE_ENABLED;
		break;
	}
	abox_clear_l2c_requests(dev, data);
	abox_clear_cpu_gear_requests(dev, data);
	flush_work(&data->boot_done_work);
	flush_work(&data->l2c_work);

	abox_ima_reclaim(data->ima_client, dev, data);

	flush_work(&data->change_cpu_gear_work);
	abox_cpu_pm_ipc(dev, false);

	spin_lock_irqsave(&data->ipc_spinlock, flag);
	data->calliope_state = CALLIOPE_DISABLED;
	spin_unlock_irqrestore(&data->ipc_spinlock, flag);

	abox_save_register(data);

	abox_cfg_gpio(dev, "idle");

	abox_pad_retention(true);

	abox_cpu_enable(false);

	abox_cpu_power(false);
	data->enabled = false;

	clk_disable(data->clk_ca7);

	abox_gic_disable_irq(&data->pdev_gic->dev);

	cancel_work_sync(&data->change_cpu_gear_work);

	return 0;
}

static int abox_runtime_suspend(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	return abox_disable(dev);
}

static int abox_runtime_resume(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	return abox_enable(dev);
}

static int abox_suspend(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	if (data->enabled)
		return 0;

	return abox_disable(dev);
}

static int abox_resume(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	if (data->enabled)
		return 0;

	return abox_enable(dev);
}

static void abox_request_iva_firmware(struct abox_data *data)
{
	int result;
	struct device *dev = &data->pdev->dev;
	const char firmware_name[] = "calliope_iva.bin";

	dev_dbg(dev, "%s\n", __func__);

	result = request_firmware(&data->firmware_iva, firmware_name, dev);
	if (IS_ERR_VALUE(result)) {
		dev_err(dev, "%s: %s request failed\n", __func__,
				firmware_name);
		return;
	}

	dev_info(dev, "IVA firmware loaded\n");
}

static void abox_complete_dram_firmware_request(const struct firmware *fw,
		void *context)
{
	struct platform_device *pdev = context;
	struct device *dev = &pdev->dev;
	struct abox_data *data = platform_get_drvdata(pdev);

	if (!fw) {
		dev_err(dev, "Failed to request firmware\n");
		return;
	}

	data->firmware_dram = fw;

	dev_info(dev, "DRAM firmware loaded\n");

	abox_request_extra_firmware(data);
	abox_request_iva_firmware(data);

	if (pm_runtime_active(dev))
		abox_enable(dev);
}

static void abox_complete_sram_firmware_request(const struct firmware *fw,
		void *context)
{
	struct platform_device *pdev = context;
	struct device *dev = &pdev->dev;
	struct abox_data *data = platform_get_drvdata(pdev);

	if (!fw) {
		dev_err(dev, "Failed to request firmware\n");
		return;
	}

	data->firmware_sram = fw;

	dev_info(dev, "SRAM firmware loaded\n");

	request_firmware_nowait(THIS_MODULE,
		FW_ACTION_HOTPLUG,
		"calliope_dram.bin",
		dev,
		GFP_KERNEL,
		pdev,
		abox_complete_dram_firmware_request);
}

static int abox_pm_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct abox_data *data = container_of(nb, struct abox_data, pm_nb);

	dev_dbg(&data->pdev->dev, "%s(%lu)\n", __func__, action);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		pm_runtime_put_sync(&data->pdev->dev);
		break;
	case PM_POST_SUSPEND:
		pm_runtime_get_sync(&data->pdev->dev);
		break;
	}
	return NOTIFY_OK;
}

static int abox_modem_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct abox_data *data = container_of(nb, struct abox_data, modem_nb);
	struct device *dev = &data->pdev->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	dev_info(&data->pdev->dev, "%s(%lu)\n", __func__, action);

	switch (action) {
	case MODEM_EVENT_ONLINE:
		msg.ipcid = IPC_SYSTEM;
		system_msg->msgtype = ABOX_START_VSS;
		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 1, 1);
		break;
	}

	return NOTIFY_OK;
}

static int abox_itmon_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct abox_data *data = container_of(nb, struct abox_data, itmon_nb);
	struct device *dev = &data->pdev->dev;
	struct itmon_notifier *itmon_data = nb_data;

	if (itmon_data && itmon_data->dest && (strncmp("ABOX", itmon_data->dest,
			sizeof("ABOX") - 1) == 0)) {
		dev_info(dev, "%s(%lu)\n", __func__, action);
		data->enabled = false;
		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}

static ssize_t calliope_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int version = be32_to_cpu(data->calliope_version);

	memcpy(buf, &version, sizeof(version));
	buf[4] = '\n';
	buf[5] = '\0';

	return 6;
}

static ssize_t calliope_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ABOX_IPC_MSG msg = {0,};
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	int result;

	dev_dbg(dev, "%s\n", __func__);

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_REQUEST_DEBUG;
	result = sscanf(buf, "%10d,%10d,%10d,%739s", &system_msg->param1,
			&system_msg->param2, &system_msg->param3,
			system_msg->bundle.param_bundle);
	result = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 1);
	if (IS_ERR_VALUE(result))
		count = result;

	return count;
}

static ssize_t calliope_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct abox_data *data = dev_get_drvdata(dev);
	const char cmd_reload_ext_bin[] = "RELOAD EXT BIN";
	char name[80];

	dev_dbg(dev, "%s(%s)\n", __func__, buf);

	if (!strncmp(cmd_reload_ext_bin, buf, sizeof(cmd_reload_ext_bin) - 1)) {
		dev_dbg(dev, "reload ext bin\n");

		if (sscanf(buf, "RELOAD EXT BIN:%63s", name) == 1)
			abox_reload_extra_firmware(data, name);
	}

	return count;
}

static DEVICE_ATTR_RO(calliope_version);
static DEVICE_ATTR_WO(calliope_debug);
static DEVICE_ATTR_WO(calliope_cmd);

static int samsung_abox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *np_tmp;
	struct abox_data *data;
	int ret, i;

	dev_info(dev, "%s\n", __func__);

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	platform_set_drvdata(pdev, data);
	data->pdev = pdev;
	p_abox_data = data;

	init_waitqueue_head(&data->ipc_wait_queue);
	spin_lock_init(&data->ipc_spinlock);
	mutex_init(&data->ipc_mutex);
	wake_lock_init(&data->wake_lock, WAKE_LOCK_SUSPEND, "abox");
	for (i = SET_MIXER_SAMPLE_RATE; i <= SET_INMUX4_SAMPLE_RATE; i++)
		data->out_rate[i] = 48000;
	for (i = 0; i < 8; i++)
		data->rdma_synchronizer[i] = i;
	INIT_WORK(&data->ipc_work, abox_process_ipc);
	INIT_WORK(&data->change_cpu_gear_work, abox_change_cpu_gear_work_func);
	INIT_WORK(&data->change_mif_freq_work, abox_change_mif_freq_work_func);
	INIT_WORK(&data->change_lit_freq_work, abox_change_lit_freq_work_func);
	INIT_WORK(&data->change_big_freq_work, abox_change_big_freq_work_func);
	INIT_WORK(&data->change_hmp_boost_work,
			abox_change_hmp_boost_work_func);
	INIT_WORK(&data->register_component_work,
			abox_register_component_work_func);
	INIT_WORK(&data->boot_done_work, abox_boot_done_work_func);
	INIT_WORK(&data->l2c_work, abox_l2c_work_func);
	INIT_LIST_HEAD(&data->irq_actions);

	data->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(data->pinctrl)) {
		dev_err(dev, "Couldn't get pins (%li)\n",
				PTR_ERR(data->pinctrl));
		return PTR_ERR(data->pinctrl);
	}

	data->sfr_base = devm_request_and_map_byname(pdev, "sfr", NULL, NULL);
	if (IS_ERR(data->sfr_base))
		return PTR_ERR(data->sfr_base);

	data->sysreg_base = devm_request_and_map_byname(pdev, "sysreg",
			NULL, NULL);
	if (IS_ERR(data->sysreg_base))
		return PTR_ERR(data->sysreg_base);

	data->sram_base = devm_request_and_map_byname(pdev, "sram",
			&data->sram_base_phys, &data->sram_size);
	if (IS_ERR(data->sram_base))
		return PTR_ERR(data->sram_base);

	data->iommu_domain = get_domain_from_dev(dev);
	if (IS_ERR(data->iommu_domain)) {
		dev_err(dev, "Unable to get iommu domain\n");
		return PTR_ERR(data->iommu_domain);
	}

	ret = iommu_attach_device(data->iommu_domain, dev);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "Unable to attach iommu domain (%d)\n", ret);
		return ret;
	}

	data->dram_base = dmam_alloc_coherent(dev, DRAM_FIRMWARE_SIZE,
			&data->dram_base_phys, GFP_KERNEL);
	if (IS_ERR_OR_NULL(data->dram_base)) {
		dev_err(dev, "Failed to allocate coherent memory: %ld\n",
				PTR_ERR(data->dram_base));
		return PTR_ERR(data->dram_base);
	}
	dev_info(dev, "%s(%#x) alloc\n", "dram firmware", DRAM_FIRMWARE_SIZE);
	iommu_map(data->iommu_domain, IOVA_DRAM_FIRMWARE, data->dram_base_phys,
			DRAM_FIRMWARE_SIZE, 0);

	data->iva_base = dmam_alloc_coherent(dev, IVA_FIRMWARE_SIZE,
			&data->iva_base_phys, GFP_KERNEL);
	if (IS_ERR_OR_NULL(data->iva_base)) {
		dev_err(dev, "Failed to allocate coherent memory: %ld\n",
				PTR_ERR(data->iva_base));
		return PTR_ERR(data->iva_base);
	}
	dev_info(dev, "%s(%#x) alloc\n", "iva firmware", IVA_FIRMWARE_SIZE);
	iommu_map(data->iommu_domain, IOVA_IVA_FIRMWARE, data->iva_base_phys,
			IVA_FIRMWARE_SIZE, 0);

	dev_dbg(dev, "%s(%#x) alloc\n", "vss firmware", shm_get_vss_size());
	iommu_map(data->iommu_domain, IOVA_VSS_FIRMWARE,
			shm_get_phys_base() + shm_get_cp_size(),
			shm_get_vss_size(), 0);

	iommu_map(data->iommu_domain, 0x16480000, 0x16480000, 0x10000, 0);
	iommu_map(data->iommu_domain, 0x10000000, 0x10000000, PAGE_SIZE, 0);
	iovmm_set_fault_handler(&pdev->dev, abox_iommu_fault_handler, data);

	data->clk_pll = devm_clk_get_and_prepare(pdev, "pll");
	if (IS_ERR(data->clk_pll))
		return PTR_ERR(data->clk_pll);

	data->clk_audif = devm_clk_get_and_prepare(pdev, "audif");
	if (IS_ERR(data->clk_audif))
		return PTR_ERR(data->clk_audif);

	data->clk_ca7 = devm_clk_get_and_prepare(pdev, "ca7");
	if (IS_ERR(data->clk_ca7))
		return PTR_ERR(data->clk_ca7);

	data->clk_dmic = devm_clk_get_and_prepare(pdev, "dmic");
	if (IS_ERR(data->clk_dmic))
		return PTR_ERR(data->clk_dmic);

	for (i = 0; i < ARRAY_SIZE(data->clk_bclk); i++) {
		char name[16];

		sprintf(name, "bclk%d", i);
		data->clk_bclk[i] = devm_clk_get_and_prepare(pdev, name);
		if (IS_ERR(data->clk_bclk[i]))
			return PTR_ERR(data->clk_bclk[i]);

		sprintf(name, "bclk%d_gate", i);
		data->clk_bclk_gate[i] = devm_clk_get_and_prepare(pdev, name);
		if (IS_ERR(data->clk_bclk_gate[i])) {
			dev_warn(dev, "%s don't exist\n", name);
			return PTR_ERR(data->clk_bclk[i]);
		}
	}

	ret = of_property_read_u32(np, "ipc_tx_offset",
			&data->ipc_tx_offset);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "Failed to read %s: %d\n", "ipc_tx_offset", ret);
		return ret;
	}

	ret = of_property_read_u32(np, "ipc_rx_offset",
			&data->ipc_rx_offset);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "Failed to read %s: %d\n", "ipc_rx_offset", ret);
		return ret;
	}

	ret = of_property_read_u32(np, "ipc_tx_ack_offset",
			&data->ipc_tx_ack_offset);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "Failed to read %s: %d\n",
				"ipc_tx_ack_offset", ret);
		return ret;
	}

	ret = of_property_read_u32(np, "ipc_rx_ack_offset",
			&data->ipc_rx_ack_offset);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "Failed to read %s: %d\n",
				"ipc_rx_ack_offset", ret);
		return ret;
	}

	ret = of_property_read_u32(np, "mailbox_offset",
			&data->mailbox_offset);
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "Failed to read %s: %d\n", "mailbox_offset", ret);
		return ret;
	}

	ret = of_property_read_u32_array(np, "pm_qos_int",
			data->pm_qos_int, ARRAY_SIZE(data->pm_qos_int));
	if (IS_ERR_VALUE(ret)) {
		dev_err(dev, "Failed to read %s: %d\n", "pm_qos_int", ret);
		return ret;
	}

	np_tmp = of_parse_phandle(np, "abox_gic", 0);
	if (!np_tmp) {
		dev_err(dev, "Failed to get abox_gic device node\n");
		return -EPROBE_DEFER;
	}
	data->pdev_gic = of_find_device_by_node(np_tmp);
	if (!data->pdev_gic) {
		dev_err(dev, "Failed to get abox_gic platform device\n");
		return -EPROBE_DEFER;
	}

	np_tmp = of_parse_phandle(np, "vts", 0);
	if (!np_tmp) {
		dev_err(dev, "Failed to get vts device node\n");
		return -EPROBE_DEFER;
	}
	data->pdev_vts = of_find_device_by_node(np_tmp);
	if (!data->pdev_vts) {
		dev_err(dev, "Failed to get vts platform device\n");
		return -EPROBE_DEFER;
	}

	request_firmware_nowait(THIS_MODULE,
			FW_ACTION_HOTPLUG,
			"calliope_sram.bin",
			dev,
			GFP_KERNEL,
			pdev,
			abox_complete_sram_firmware_request);

#ifdef EMULATOR
	pmu_alive = ioremap(0x16480000, 0x10000);
#endif
	abox_cpu_enable(false);
	abox_cpu_power(false);

	pm_qos_add_request(&abox_pm_qos_int, PM_QOS_DEVICE_THROUGHPUT, 0);
	pm_qos_add_request(&abox_pm_qos_mif, PM_QOS_BUS_THROUGHPUT, 0);
	pm_qos_add_request(&abox_pm_qos_lit, PM_QOS_CLUSTER0_FREQ_MIN, 0);
	pm_qos_add_request(&abox_pm_qos_big, PM_QOS_CLUSTER1_FREQ_MIN, 0);

	abox_gic_register_irq_handler(data->pdev_gic, abox_irq_handler, pdev);

	data->regmap = devm_regmap_init_mmio_clk(dev,
			NULL,
			data->sfr_base,
			&abox_codec_regmap_config);

	pm_runtime_enable(dev);
	pm_runtime_get(dev);

	data->pm_nb.notifier_call = abox_pm_notifier;
	register_pm_notifier(&data->pm_nb);

	data->modem_nb.notifier_call = abox_modem_notifier;
	register_modem_event_notifier(&data->modem_nb);

	data->itmon_nb.notifier_call = abox_itmon_notifier;
	itmon_notifier_chain_register(&data->itmon_nb);

	abox_ima_init(dev, data);

	dev_info(dev, "%s: probe complete\n", __func__);

	of_platform_populate(np, NULL, NULL, dev);

	ret = device_create_file(dev, &dev_attr_calliope_version);
	if (IS_ERR_VALUE(ret))
		dev_warn(dev, "Failed to create file: %s\n", "version");

	ret = device_create_file(dev, &dev_attr_calliope_debug);
	if (IS_ERR_VALUE(ret))
		dev_warn(dev, "Failed to create file: %s\n", "debug");

	ret = device_create_file(dev, &dev_attr_calliope_cmd);
	if (IS_ERR_VALUE(ret))
		dev_warn(dev, "Failed to create file: %s\n", "cmd");

	atomic_notifier_chain_register(&panic_notifier_list,
			&abox_panic_notifier);

	return snd_soc_register_codec(dev, &abox_codec, abox_dais,
			ARRAY_SIZE(abox_dais));
}

static int samsung_abox_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct abox_data *data = platform_get_drvdata(pdev);

	dev_info(dev, "%s\n", __func__);

	pm_runtime_disable(dev);
#ifndef CONFIG_PM
	abox_runtime_suspend(dev);
#endif
	pm_qos_remove_request(&abox_pm_qos_int);
	pm_qos_remove_request(&abox_pm_qos_mif);
	pm_qos_remove_request(&abox_pm_qos_lit);
	pm_qos_remove_request(&abox_pm_qos_big);
	snd_soc_unregister_codec(dev);
	iommu_unmap(data->iommu_domain, IOVA_DRAM_FIRMWARE, DRAM_FIRMWARE_SIZE);
#ifdef EMULATOR
	iounmap(pmu_alive);
#endif
	return 0;
}

static void samsung_abox_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_info(dev, "%s\n", __func__);
	pm_runtime_disable(dev);
}

static const struct of_device_id samsung_abox_match[] = {
	{
		.compatible = "samsung,abox",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_match);

static const struct dev_pm_ops samsung_abox_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(abox_suspend, abox_resume)
	SET_RUNTIME_PM_OPS(abox_runtime_suspend, abox_runtime_resume, NULL)
};

static struct platform_driver samsung_abox_driver = {
	.probe  = samsung_abox_probe,
	.remove = samsung_abox_remove,
	.shutdown = samsung_abox_shutdown,
	.driver = {
		.name = "samsung-abox",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_match),
		.pm = &samsung_abox_pm,
	},
};

module_platform_driver(samsung_abox_driver);

static int __init samsung_abox_late_initcall(void)
{
	pr_info("%s\n", __func__);

	if (p_abox_data && p_abox_data->pdev) {
		pm_runtime_put_sync(&p_abox_data->pdev->dev);
	} else {
		pr_err("%s: p_abox_data or p_abox_data->pdev is null",
				__func__);
	}
	return 0;
}
late_initcall(samsung_abox_late_initcall);

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box Driver");
MODULE_ALIAS("platform:samsung-abox");
MODULE_LICENSE("GPL");
