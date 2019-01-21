/* Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define DEBUG
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/jack.h>
#include <sound/q6afe-v2.h>
#include <sound/q6core.h>
#include <sound/pcm_params.h>
#include <sound/info.h>
#include <soc/qcom/socinfo.h>
#include "qdsp6v2/msm-pcm-routing-v2.h"
#include "msm-audio-pinctrl.h"
#include <linux/regulator/consumer.h>
#include "../codecs/rt5665.h"
#if defined(CONFIG_SND_SOC_DBMDX)
#include <sound/dbmdx-export.h>
#endif

#define DRV_NAME "msm8952-asoc-rt5665"

#define BTSCO_RATE_8KHZ 8000
#define BTSCO_RATE_16KHZ 16000
#define SAMPLING_RATE_8KHZ      8000
#define SAMPLING_RATE_16KHZ     16000
#define SAMPLING_RATE_32KHZ     32000
#define SAMPLING_RATE_48KHZ     48000
#define SAMPLING_RATE_48KHZ     48000
#define SAMPLING_RATE_96KHZ     96000
#define SAMPLING_RATE_192KHZ    192000
#define SAMPLING_RATE_44P1KHZ   44100

#define PRI_MI2S_ID	(1 << 0)
#define SEC_MI2S_ID	(1 << 1)
#define TER_MI2S_ID	(1 << 2)
#define QUAT_MI2S_ID	(1 << 3)
#define QUIN_MI2S_ID	(1 << 4)

#define DEFAULT_MCLK_RATE 9600000

enum btsco_rates {
	RATE_8KHZ_ID,
	RATE_16KHZ_ID,
};

static bool ext_codec;

static int msm8952_auxpcm_rate = 8000;
static int msm_btsco_rate = BTSCO_RATE_8KHZ;
static int msm_btsco_ch = 1;
static int msm_ter_mi2s_tx_ch = 1;
static int msm_pri_mi2s_rx_ch = 1;
static int msm_proxy_rx_ch = 2;
static int msm_vi_feed_tx_ch = 2;
static int mi2s_rx_bit_format = SNDRV_PCM_FORMAT_S16_LE;
static int mi2s_rx_bits_per_sample = 16;
static int mi2s_rx_sample_rate = SAMPLING_RATE_48KHZ;

static atomic_t quat_mi2s_clk_ref;
static atomic_t quin_mi2s_clk_ref;
static atomic_t auxpcm_mi2s_clk_ref;
static struct snd_soc_codec *registered_codec;
static void *adsp_state_notifier;

static int msm8952_enable_dig_cdc_clk(struct snd_soc_codec *codec, int enable,
					bool dapm);

#if defined(CONFIG_SND_SOC_TDM_INTF)
#define TDM_SLOT_OFFSET_MAX    8

enum {
	PRIMARY_TDM_RX_0,
	PRIMARY_TDM_TX_0,
	SECONDARY_TDM_RX_0,
	SECONDARY_TDM_TX_0,
	TDM_MAX,
};

/* TDM default channels */
static int msm_pri_tdm_rx_0_ch = 8;
static int msm_pri_tdm_tx_0_ch = 8;

static int msm_sec_tdm_rx_0_ch = 8;
static int msm_sec_tdm_tx_0_ch = 8;

/* TDM default bit format */
static int msm_pri_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S16_LE;
static int msm_pri_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S16_LE;

static int msm_sec_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S16_LE;
static int msm_sec_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S16_LE;

/* TDM default sampling rate */
static int msm_pri_tdm_rx_0_sample_rate = SAMPLING_RATE_48KHZ;
static int msm_pri_tdm_tx_0_sample_rate = SAMPLING_RATE_48KHZ;

static int msm_sec_tdm_rx_0_sample_rate = SAMPLING_RATE_48KHZ;
static int msm_sec_tdm_tx_0_sample_rate = SAMPLING_RATE_48KHZ;

static char const *tdm_ch_text[] = {"One", "Two", "Three", "Four",
	"Five", "Six", "Seven", "Eight"};
static char const *tdm_bit_format_text[] = {"S16_LE", "S24_LE", "S24_3LE",
					    "S32_LE"};
static char const *tdm_sample_rate_text[] = {"KHZ_16", "KHZ_48"};

/* TDM default offset */
static unsigned int tdm_slot_offset[TDM_MAX][TDM_SLOT_OFFSET_MAX] = {
	/* PRI_TDM_RX */
	{0, 4, 8, 12, 16, 20, 24, 28},
	/* PRI_TDM_TX */
	{0, 4, 8, 12, 16, 20, 24, 28},
	/* SEC_TDM_RX */
	{0, 4, 8, 12, 16, 20, 24, 28},
	/* SEC_TDM_TX */
	{0, 4, 8, 12, 16, 20, 24, 28},
};

#endif

struct snd_sysclk_info {
	struct snd_soc_dai *codec_dai;
	int clk_id;
	int fll_in;
	int fll_out;
};

struct snd_sysclk_info rt5665_sysclk = {
	.clk_id = RT5665_PLL1_S_MCLK,
	.fll_in = 24000000,
	.fll_out = 24576000,
};

static struct snd_soc_jack hs_jack;

static struct afe_clk_cfg mi2s_rx_clk_v1 = {
	AFE_API_VERSION_I2S_CONFIG,
	Q6AFE_LPASS_IBIT_CLK_1_P536_MHZ,
	Q6AFE_LPASS_OSR_CLK_12_P288_MHZ,
	Q6AFE_LPASS_CLK_SRC_INTERNAL,
	Q6AFE_LPASS_CLK_ROOT_DEFAULT,
	Q6AFE_LPASS_MODE_CLK1_VALID,
	0,
};

static struct afe_clk_cfg mi2s_tx_clk_v1 = {
	AFE_API_VERSION_I2S_CONFIG,
	Q6AFE_LPASS_IBIT_CLK_1_P536_MHZ,
	Q6AFE_LPASS_OSR_CLK_12_P288_MHZ,
	Q6AFE_LPASS_CLK_SRC_INTERNAL,
	Q6AFE_LPASS_CLK_ROOT_DEFAULT,
	Q6AFE_LPASS_MODE_CLK1_VALID,
	0,
};

static struct afe_clk_set mi2s_tx_clk = {
	AFE_API_VERSION_I2S_CONFIG,
	Q6AFE_LPASS_CLK_ID_TER_MI2S_IBIT,
	Q6AFE_LPASS_IBIT_CLK_1_P536_MHZ,
	Q6AFE_LPASS_CLK_ATTRIBUTE_COUPLE_NO,
	Q6AFE_LPASS_CLK_ROOT_DEFAULT,
	0,
};

static struct afe_clk_set mi2s_rx_clk = {
	AFE_API_VERSION_I2S_CONFIG,
	Q6AFE_LPASS_CLK_ID_PRI_MI2S_IBIT,
	Q6AFE_LPASS_IBIT_CLK_1_P536_MHZ,
	Q6AFE_LPASS_CLK_ATTRIBUTE_COUPLE_NO,
	Q6AFE_LPASS_CLK_ROOT_DEFAULT,
	0,
};

static char const *rx_bit_format_text[] = {"S16_LE", "S24_LE", "S24_3LE"};
static const char *const mi2s_ch_text[] = {"One", "Two"};
static const char *const loopback_mclk_text[] = {"DISABLE", "ENABLE"};
static const char *const btsco_rate_text[] = {"BTSCO_RATE_8KHZ",
	"BTSCO_RATE_16KHZ"};
static const char *const proxy_rx_ch_text[] = {"One", "Two", "Three", "Four",
	"Five", "Six", "Seven", "Eight"};
static const char *const vi_feed_ch_text[] = {"One", "Two"};
static char const *mi2s_rx_sample_rate_text[] = {"KHZ_48",
					"KHZ_96", "KHZ_192"};

struct msm8952_asoc_mach_data {
	int codec_type;
	int ext_pa;
	int mclk_freq;
	int afe_clk_ver;
	atomic_t mclk_rsc_ref;
	atomic_t mclk_enabled;
	struct mutex cdc_mclk_mutex;
	struct delayed_work disable_mclk_work;
	struct afe_digital_clk_cfg digital_cdc_clk;
	struct afe_clk_set digital_cdc_core_clk;
	void __iomem *vaddr_gpio_mux_spkr_ctl;
	void __iomem *vaddr_gpio_mux_mic_ctl;
	void __iomem *vaddr_gpio_mux_pcm_ctl;
	void __iomem *vaddr_gpio_mux_quin_ctl;
#if defined(CONFIG_SND_SOC_TDM_INTF)
	void __iomem *vaddr_gpio_mux_qui_pcm_sec_mode_ctl;
	void __iomem *vaddr_gpio_mux_mic_ext_clk_ctl;
	void __iomem *vaddr_gpio_mux_sec_tlmm_ctl;
#endif
#ifdef CONFIG_SND_SOC_MSM8X16_RT5665
	struct snd_soc_codec *codec;
	struct clk *rt5665_ext_clk;
	int codec_irq_gpio;
	int codec_ear_bias;
#endif /* CONFIG_SND_SOC_MSM8X16_RT5659 */
};

static inline int param_is_mask(int p)
{
	return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
			(p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
	return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned bit)
{
	if (bit >= SNDRV_MASK_MAX)
		return;
	if (param_is_mask(n)) {
		struct snd_mask *m = param_to_mask(p, n);

		m->bits[0] = 0;
		m->bits[1] = 0;
		m->bits[bit >> 5] |= (1 << (bit & 31));
	}
}

static int msm_proxy_rx_ch_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_proxy_rx_ch = %d\n", __func__,
						msm_proxy_rx_ch);
	ucontrol->value.integer.value[0] = msm_proxy_rx_ch - 1;
	return 0;
}

static int msm_proxy_rx_ch_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	msm_proxy_rx_ch = ucontrol->value.integer.value[0] + 1;
	pr_debug("%s: msm_proxy_rx_ch = %d\n", __func__,
						msm_proxy_rx_ch);
	return 0;
}

static int msm_auxpcm_be_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate =
	    hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels =
	    hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = msm8952_auxpcm_rate;
	channels->min = channels->max = 1;

	return 0;
}

static int msm_mi2s_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
				   mi2s_rx_bit_format);
	rate->min = rate->max = mi2s_rx_sample_rate;
	channels->min = channels->max = msm_ter_mi2s_tx_ch;

	return 0;
}

static int msm_mi2s_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
				   mi2s_rx_bit_format);
	rate->min = rate->max = mi2s_rx_sample_rate;
	channels->min = channels->max = msm_pri_mi2s_rx_ch;

	return 0;
}

static int msm_pri_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s: Number of channels = %d\n", __func__,
			msm_pri_mi2s_rx_ch);
	rate->min = rate->max = 48000;
	channels->min = channels->max = msm_pri_mi2s_rx_ch;

	return 0;
}

static int msm_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);
	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s(), channel:%d\n", __func__, msm_ter_mi2s_tx_ch);
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
			SNDRV_PCM_FORMAT_S16_LE);
	rate->min = rate->max = 48000;
	channels->min = channels->max = msm_ter_mi2s_tx_ch;

	return 0;
}

static int msm_senary_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);
	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = 48000;
	channels->min = channels->max = 2;

	return 0;
}


static int msm_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	channels->min = channels->max = 2;

	return 0;
}

static int msm_btsco_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = msm_btsco_rate;
	channels->min = channels->max = msm_btsco_ch;

	return 0;
}

static int msm_proxy_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s: msm_proxy_rx_ch =%d\n", __func__, msm_proxy_rx_ch);

	if (channels->max < 2)
		channels->min = channels->max = 2;
	channels->min = channels->max = msm_proxy_rx_ch;
	rate->min = rate->max = 48000;
	return 0;
}

static int msm_proxy_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	rate->min = rate->max = 48000;
	return 0;
}

#if defined(CONFIG_SND_SOC_TDM_INTF)
int msm_tdm_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);
	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	switch (cpu_dai->id) {
	case AFE_PORT_ID_PRIMARY_TDM_RX:
		channels->min = channels->max = msm_pri_tdm_rx_0_ch;
		param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
			msm_pri_tdm_rx_0_bit_format);
		rate->min = rate->max = msm_pri_tdm_rx_0_sample_rate;
		break;
	case AFE_PORT_ID_PRIMARY_TDM_TX:
		channels->min = channels->max = msm_pri_tdm_tx_0_ch;
		param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
			msm_pri_tdm_tx_0_bit_format);
		rate->min = rate->max = msm_pri_tdm_tx_0_sample_rate;
		break;
	case AFE_PORT_ID_SECONDARY_TDM_RX:
		channels->min = channels->max = msm_sec_tdm_rx_0_ch;
		param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
			msm_sec_tdm_rx_0_bit_format);
		rate->min = rate->max = msm_sec_tdm_rx_0_sample_rate;
		break;
	case AFE_PORT_ID_SECONDARY_TDM_TX:
		channels->min = channels->max = msm_sec_tdm_tx_0_ch;
		param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
			msm_sec_tdm_tx_0_bit_format);
		rate->min = rate->max = msm_sec_tdm_tx_0_sample_rate;
		break;
	default:
		pr_err("%s: dai id 0x%x not supported\n",
			__func__, cpu_dai->id);
		return -EINVAL;
	}

	pr_debug("%s: dai id = 0x%x channels = %d rate = %d\n",
		__func__, cpu_dai->id, channels->max, rate->max);

	return 0;
}

static unsigned int tdm_param_set_slot_mask(u16 port_id,
				int slot_width, int slots)
{
	unsigned int slot_mask = 0;
	int upper, lower, i, j;
	unsigned int *slot_offset;

	switch (port_id) {
	case AFE_PORT_ID_PRIMARY_TDM_RX:
		lower = PRIMARY_TDM_RX_0;
		upper = PRIMARY_TDM_RX_0;
		break;
	case AFE_PORT_ID_PRIMARY_TDM_TX:
		lower = PRIMARY_TDM_TX_0;
		upper = PRIMARY_TDM_TX_0;
		break;
	case AFE_PORT_ID_SECONDARY_TDM_RX:
		lower = SECONDARY_TDM_RX_0;
		upper = SECONDARY_TDM_RX_0;
		break;
	case AFE_PORT_ID_SECONDARY_TDM_TX:
		lower = SECONDARY_TDM_TX_0;
		upper = SECONDARY_TDM_TX_0;
		break;
	default:
		return slot_mask;
	}

	for (i = lower; i <= upper; i++) {
		slot_offset = tdm_slot_offset[i];
		for (j = 0; j < TDM_SLOT_OFFSET_MAX; j++) {
			if (slot_offset[j] != AFE_SLOT_MAPPING_OFFSET_INVALID)
				/*
				 * set the mask of active slot according to
				 * the offset table for the group of devices
				 */
				slot_mask |=
				    (1 << ((slot_offset[j] * 8) / slot_width));
			else
				break;
		}
	}

	return slot_mask;
}

int msm_tdm_snd_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	int channels, slot_width, slots;
	unsigned int slot_mask;
	unsigned int *slot_offset;
	int offset_channels = 0;
	int i;

	pr_debug("%s: dai id = 0x%x\n", __func__, cpu_dai->id);

	channels = params_channels(params);
	switch (channels) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 6:
	case 8:
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S32_LE:
		case SNDRV_PCM_FORMAT_S24_LE:
		case SNDRV_PCM_FORMAT_S16_LE:
			/*
			 * up to 8 channel HW configuration should
			 * use 32 bit slot width for max support of
			 * stream bit width. (slot_width > bit_width)
			 */
			slot_width = 32;
			break;
		default:
			pr_err("%s: invalid param format 0x%x\n",
				__func__, params_format(params));
			return -EINVAL;
		}
		slots = 8;
		slot_mask = tdm_param_set_slot_mask(cpu_dai->id,
			slot_width, slots);
		if (!slot_mask) {
			pr_err("%s: invalid slot_mask 0x%x\n",
				__func__, slot_mask);
			return -EINVAL;
		}
		break;
	default:
		pr_err("%s: invalid param channels %d\n",
			__func__, channels);
		return -EINVAL;
	}

	switch (cpu_dai->id) {
	case AFE_PORT_ID_PRIMARY_TDM_RX:
		slot_offset = tdm_slot_offset[PRIMARY_TDM_RX_0];
		break;
	case AFE_PORT_ID_PRIMARY_TDM_TX:
		slot_offset = tdm_slot_offset[PRIMARY_TDM_TX_0];
		break;
	case AFE_PORT_ID_SECONDARY_TDM_RX:
		slot_offset = tdm_slot_offset[SECONDARY_TDM_RX_0];
		break;
	case AFE_PORT_ID_SECONDARY_TDM_TX:
		slot_offset = tdm_slot_offset[SECONDARY_TDM_TX_0];
		break;
	default:
		pr_err("%s: dai id 0x%x not supported\n",
			__func__, cpu_dai->id);
		return -EINVAL;
	}

	for (i = 0; i < TDM_SLOT_OFFSET_MAX; i++) {
		if (slot_offset[i] != AFE_SLOT_MAPPING_OFFSET_INVALID)
			offset_channels++;
		else
			break;
	}

	if (offset_channels == 0) {
		pr_err("%s: slot offset not supported, offset_channels %d\n",
			__func__, offset_channels);
		return -EINVAL;
	}

	if (channels > offset_channels) {
		pr_err("%s: channels %d exceed offset_channels %d\n",
			__func__, channels, offset_channels);
		return -EINVAL;
	}

	/* 
	* force to configurate TDM as below
	* slots : 4 channel
	* slot_mask : bits 0, 1, 2, 3 set for the first 8 slots
	* slot_width : 16 bit
	*/
	slots = 4;
	slot_mask = 15;
	slot_width = 16;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ret = snd_soc_dai_set_tdm_slot(cpu_dai, 0, slot_mask,
			slots, slot_width);
		if (ret < 0) {
			pr_err("%s: failed to set tdm slot, err:%d\n",
				__func__, ret);
			goto end;
		}

		ret = snd_soc_dai_set_channel_map(cpu_dai,
			0, NULL, channels, slot_offset);
		if (ret < 0) {
			pr_err("%s: failed to set channel map, err:%d\n",
				__func__, ret);
			goto end;
		}
	} else {
		ret = snd_soc_dai_set_tdm_slot(cpu_dai, slot_mask, 0,
			slots, slot_width);
		if (ret < 0) {
			pr_err("%s: failed to set tdm slot, err:%d\n",
				__func__, ret);
			goto end;
		}

		ret = snd_soc_dai_set_channel_map(cpu_dai,
			channels, slot_offset, 0, NULL);
		if (ret < 0) {
			pr_err("%s: failed to set channel map, err:%d\n",
				__func__, ret);
			goto end;
		}
	}

end:
	return ret;
}

#endif

static void msm8952_rt5665_set_mclk(struct snd_soc_card *card, int enable)
{
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);
	int ret;

	dev_info(card->dev, "%s enable %d\n", __func__, enable);
	if (enable) {
		ret = clk_set_rate(pdata->rt5665_ext_clk,
			(unsigned long)pdata->mclk_freq);
		if (ret < 0) {
			dev_err(card->dev, "%s: Can't set ext-mclk rate %dHz (%d)\n",
			__func__, pdata->mclk_freq, ret);
			return;
		}

		ret = clk_prepare_enable(pdata->rt5665_ext_clk);
		if (ret < 0) {
			dev_err(card->dev, "%s: Can't enable ext-mclk %d\n",
			__func__, ret);
			return;
		}
	} else {
		clk_disable_unprepare(pdata->rt5665_ext_clk);
	}

}

static const struct snd_kcontrol_new rt5665_controls[] = {
	SOC_DAPM_PIN_SWITCH("HP"),
	SOC_DAPM_PIN_SWITCH("SPK"),
	SOC_DAPM_PIN_SWITCH("RCV"),
	SOC_DAPM_PIN_SWITCH("Main Mic"),
	SOC_DAPM_PIN_SWITCH("Sub Mic"),
	SOC_DAPM_PIN_SWITCH("Headset Mic"),
};

const struct snd_soc_dapm_widget rt5665_dapm_widgets[] = {
	SND_SOC_DAPM_HP("HP", NULL),
	SND_SOC_DAPM_SPK("SPK", NULL),
	SND_SOC_DAPM_SPK("RCV", NULL),
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("Main Mic", NULL),
	SND_SOC_DAPM_MIC("Sub Mic", NULL),
};

static int msm_rt5665_aif1_mi2s_snd_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct msm8952_asoc_mach_data *pdata = NULL;
	int ret;

	pdata = snd_soc_card_get_drvdata(card);
	dev_info(card->dev, "%s-%d %dch, %dHz, %dbytes\n",
			rtd->dai_link->name, substream->stream,
			params_channels(params), params_rate(params),
			params_buffer_bytes(params));

	msm8952_rt5665_set_mclk(card, 1);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
			 mi2s_rx_bit_format);
	else
		param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
			 mi2s_rx_bit_format);

	/* Set Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		dev_err(card->dev, "Failed to set aif1 codec fmt: %d\n", ret);
		return ret;
	}

	/* Set CPU DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		dev_err(card->dev, "Failed to set aif1 cpu fmt: %d\n", ret);
		return ret;
	}

	if (params_rate(params) == 192000) {
		rt5665_sysclk.clk_id = RT5665_PLL1_S_BCLK1;
		rt5665_sysclk.fll_in = params_rate(params)*32*2;
		rt5665_sysclk.fll_out = params_rate(params)*512;
	} else {
		rt5665_sysclk.clk_id = RT5665_PLL1_S_BCLK1;
		rt5665_sysclk.fll_in = params_rate(params)*32;
		rt5665_sysclk.fll_out = params_rate(params)*512;
	}

	ret = snd_soc_dai_set_pll(codec_dai, 0, rt5665_sysclk.clk_id,
			rt5665_sysclk.fll_in, rt5665_sysclk.fll_out);
	if (ret < 0) {
		dev_err(card->dev, "codec_dai pll not set\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, RT5665_SCLK_S_PLL1,
			rt5665_sysclk.fll_out, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(card->dev, "codec_dai clock not set\n");
		return ret;
	}

	return ret;
}

static int msm_mi2s_snd_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
		 substream->name, substream->stream);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
			       mi2s_rx_bit_format);
	else
		param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
			       SNDRV_PCM_FORMAT_S16_LE);
	return 0;
}

static u32 msm8952_get_mi2s_bit_clock(int mi2s_bit_format, int sample_rate)
{
	u32 bit_clock = 0;

	if (mi2s_bit_format == SNDRV_PCM_FORMAT_S24_LE ||
		mi2s_bit_format == SNDRV_PCM_FORMAT_S32_LE) {
		switch (sample_rate) {
		case SAMPLING_RATE_192KHZ:
			bit_clock = Q6AFE_LPASS_IBIT_CLK_12_P288_MHZ;
			break;
		case SAMPLING_RATE_96KHZ:
			bit_clock = Q6AFE_LPASS_IBIT_CLK_6_P144_MHZ;
			break;
		case SAMPLING_RATE_48KHZ:
			bit_clock = Q6AFE_LPASS_IBIT_CLK_3_P072_MHZ;
			break;
		default:
			bit_clock = Q6AFE_LPASS_IBIT_CLK_3_P072_MHZ;
		}
	} else {
		/* bit clock is calculated based on 16 bit */
		switch (sample_rate) {
		case SAMPLING_RATE_192KHZ:
			bit_clock = Q6AFE_LPASS_IBIT_CLK_6_P144_MHZ;
			break;
		case SAMPLING_RATE_96KHZ:
			bit_clock = Q6AFE_LPASS_IBIT_CLK_3_P072_MHZ;
			break;
		case SAMPLING_RATE_48KHZ:
			bit_clock = Q6AFE_LPASS_IBIT_CLK_1_P536_MHZ;
			break;
		default:
			bit_clock = Q6AFE_LPASS_IBIT_CLK_1_P536_MHZ;
			break;
		}
	}
	pr_debug("%s: bit_width = %d, sample_rate = %d, bit_clock = %d\n",
			__func__, mi2s_bit_format, sample_rate, bit_clock);

	return bit_clock;
}

static int msm8952_get_clk_id(int port_id)
{
	switch (port_id) {
	case AFE_PORT_ID_PRIMARY_MI2S_RX:
		return Q6AFE_LPASS_CLK_ID_PRI_MI2S_IBIT;
	case AFE_PORT_ID_SECONDARY_MI2S_RX:
		return Q6AFE_LPASS_CLK_ID_SEC_MI2S_IBIT;
	case AFE_PORT_ID_TERTIARY_MI2S_TX:
		return Q6AFE_LPASS_CLK_ID_TER_MI2S_IBIT;
	case AFE_PORT_ID_QUATERNARY_MI2S_RX:
	case AFE_PORT_ID_QUATERNARY_MI2S_TX:
		return Q6AFE_LPASS_CLK_ID_QUAD_MI2S_IBIT;
	case AFE_PORT_ID_QUINARY_MI2S_RX:
	case AFE_PORT_ID_QUINARY_MI2S_TX:
		return Q6AFE_LPASS_CLK_ID_QUI_MI2S_IBIT;
	case AFE_PORT_ID_SENARY_MI2S_TX:
		return Q6AFE_LPASS_CLK_ID_SEN_MI2S_IBIT;
	default:
		pr_err("%s: invalid port_id: 0x%x\n", __func__, port_id);
		return -EINVAL;
	}
}

static int msm8952_get_port_id(int be_id)
{
	switch (be_id) {
	case MSM_BACKEND_DAI_PRI_MI2S_RX:
		return AFE_PORT_ID_PRIMARY_MI2S_RX;
	case MSM_BACKEND_DAI_SECONDARY_MI2S_RX:
		return AFE_PORT_ID_SECONDARY_MI2S_RX;
	case MSM_BACKEND_DAI_TERTIARY_MI2S_TX:
		return AFE_PORT_ID_TERTIARY_MI2S_TX;
	case MSM_BACKEND_DAI_QUATERNARY_MI2S_RX:
		return AFE_PORT_ID_QUATERNARY_MI2S_RX;
	case MSM_BACKEND_DAI_QUATERNARY_MI2S_TX:
		return AFE_PORT_ID_QUATERNARY_MI2S_TX;
	case MSM_BACKEND_DAI_QUINARY_MI2S_RX:
		return AFE_PORT_ID_QUINARY_MI2S_RX;
	case MSM_BACKEND_DAI_QUINARY_MI2S_TX:
		return AFE_PORT_ID_QUINARY_MI2S_TX;
	case MSM_BACKEND_DAI_SENARY_MI2S_TX:
		return AFE_PORT_ID_SENARY_MI2S_TX;
	default:
		pr_err("%s: Invalid be_id: %d\n", __func__, be_id);
		return -EINVAL;
	}
}

#ifdef CONFIG_AUDIO_SPEAKER_OUT_NXP_AMP_ENABLE
static int msm_q6_enable_mi2s_clocks(bool enable, int port_id)
{
	union afe_port_config port_config;
	int rc = 0;

	pr_info("set %s enable=%d\n", __func__, enable);

	if (enable) {
		port_config.i2s.channel_mode = AFE_PORT_I2S_SD1;
		port_config.i2s.mono_stereo = MSM_AFE_CH_STEREO;
		port_config.i2s.data_format = 0;
		port_config.i2s.bit_width = 16;
		port_config.i2s.reserved = 0;
		port_config.i2s.i2s_cfg_minor_version =
			AFE_API_VERSION_I2S_CONFIG;
		port_config.i2s.sample_rate = 48000;
		port_config.i2s.ws_src = 1;

		rc = afe_port_start(port_id, &port_config, 48000);

		if (IS_ERR_VALUE(rc)) {
			pr_err("fail to open AFE port\n");
			return -EINVAL;
		}
	} else {
		rc = afe_close(port_id);
		if (IS_ERR_VALUE(rc)) {
			pr_err("fail to close AFE port\n");
			return -EINVAL;
		}
	}
	return rc;
}
#endif /* CONFIG_AUDIO_SPEAKER_OUT_NXP_AMP_ENABLE */

static int msm_mi2s_sclk_ctl(struct snd_pcm_substream *substream, bool enable)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);
	int port_id = 0;

	port_id = msm8952_get_port_id(rtd->dai_link->be_id);
	if (port_id < 0) {
		pr_err("%s: Invalid port_id\n", __func__);
		return -EINVAL;
	}
	if (enable) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (pdata->afe_clk_ver == AFE_CLK_VERSION_V1) {
				mi2s_rx_clk_v1.clk_val1 =
					msm8952_get_mi2s_bit_clock(mi2s_rx_bit_format,
						mi2s_rx_sample_rate);
				ret = afe_set_lpass_clock(port_id,
							&mi2s_rx_clk_v1);
			} else {
				mi2s_rx_clk.enable = enable;
				mi2s_rx_clk.clk_id =
						msm8952_get_clk_id(port_id);

				mi2s_rx_clk.clk_freq_in_hz =
					msm8952_get_mi2s_bit_clock(mi2s_rx_bit_format,
						mi2s_rx_sample_rate);
				ret = afe_set_lpass_clock_v2(port_id,
							&mi2s_rx_clk);
			}

#ifdef CONFIG_AUDIO_SPEAKER_OUT_NXP_AMP_ENABLE
		if (port_id == AFE_PORT_ID_QUATERNARY_MI2S_RX)
			msm_q6_enable_mi2s_clocks(true, port_id);
#endif

		} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			if (pdata->afe_clk_ver == AFE_CLK_VERSION_V1) {
				mi2s_tx_clk_v1.clk_val1 =
						msm8952_get_mi2s_bit_clock(mi2s_rx_bit_format,
						mi2s_rx_sample_rate);
				ret = afe_set_lpass_clock(port_id,
							&mi2s_tx_clk_v1);
			} else {
				mi2s_tx_clk.enable = enable;
				mi2s_tx_clk.clk_id =
						msm8952_get_clk_id(port_id);
				mi2s_tx_clk.clk_freq_in_hz =
						msm8952_get_mi2s_bit_clock(mi2s_rx_bit_format,
						mi2s_rx_sample_rate);
				ret = afe_set_lpass_clock_v2(port_id,
							&mi2s_tx_clk);
			}
#ifdef CONFIG_AUDIO_SPEAKER_OUT_NXP_AMP_ENABLE
		if (port_id == AFE_PORT_ID_QUATERNARY_MI2S_TX)
			msm_q6_enable_mi2s_clocks(true, port_id);
#endif

		} else {
			pr_err("%s:Not valid substream.\n", __func__);
		}

		if (ret < 0)
			pr_err("%s:afe_set_lpass_clock_v2 failed\n", __func__);
	} else {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
#ifdef CONFIG_AUDIO_SPEAKER_OUT_NXP_AMP_ENABLE
			if (port_id == AFE_PORT_ID_QUATERNARY_MI2S_RX)
				msm_q6_enable_mi2s_clocks(false, port_id);
#endif /* CONFIG_AUDIO_SPEAKER_OUT_NXP_AMP_ENABLE */

			if (pdata->afe_clk_ver == AFE_CLK_VERSION_V1) {
				mi2s_rx_clk_v1.clk_val1 =
						Q6AFE_LPASS_IBIT_CLK_DISABLE;
				ret = afe_set_lpass_clock(port_id,
							&mi2s_rx_clk_v1);
			} else {
				mi2s_rx_clk.enable = enable;
				mi2s_rx_clk.clk_id =
						msm8952_get_clk_id(port_id);
				ret = afe_set_lpass_clock_v2(port_id,
							&mi2s_rx_clk);
			}
		} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
#ifdef CONFIG_AUDIO_SPEAKER_OUT_NXP_AMP_ENABLE
			if (port_id == AFE_PORT_ID_QUATERNARY_MI2S_TX)
				msm_q6_enable_mi2s_clocks(false, port_id);
#endif /* CONFIG_AUDIO_SPEAKER_OUT_NXP_AMP_ENABLE */

			if (pdata->afe_clk_ver == AFE_CLK_VERSION_V1) {
				mi2s_tx_clk_v1.clk_val1 =
						Q6AFE_LPASS_IBIT_CLK_DISABLE;
				ret = afe_set_lpass_clock(port_id,
							&mi2s_tx_clk_v1);
			} else {
				mi2s_tx_clk.enable = enable;
				mi2s_tx_clk.clk_id =
						msm8952_get_clk_id(port_id);
				ret = afe_set_lpass_clock_v2(port_id,
							&mi2s_tx_clk);
			}
		} else {
			pr_err("%s:Not valid substream.\n", __func__);
		}

		if (ret < 0)
			pr_err("%s:afe_set_lpass_clock_v2 failed\n", __func__);
	}
	return ret;
}

static int msm8952_enable_dig_cdc_clk(struct snd_soc_codec *codec,
					int enable, bool dapm)
{
	int ret = 0;
	struct msm8952_asoc_mach_data *pdata = NULL;

	pdata = snd_soc_card_get_drvdata(codec->component.card);
	pr_debug("%s: enable %d mclk ref counter %d\n",
		   __func__, enable,
		   atomic_read(&pdata->mclk_rsc_ref));
	if (enable) {
		if (!atomic_read(&pdata->mclk_rsc_ref)) {
			cancel_delayed_work_sync(
					&pdata->disable_mclk_work);
			mutex_lock(&pdata->cdc_mclk_mutex);
			if (atomic_read(&pdata->mclk_enabled) == false) {
				if (pdata->afe_clk_ver == AFE_CLK_VERSION_V1) {
					pdata->digital_cdc_clk.clk_val =
							pdata->mclk_freq;
					ret = afe_set_digital_codec_core_clock(
						AFE_PORT_ID_PRIMARY_MI2S_RX,
						&pdata->digital_cdc_clk);
				} else {
					pdata->digital_cdc_core_clk.enable = 1;
					ret = afe_set_lpass_clock_v2(
						AFE_PORT_ID_PRIMARY_MI2S_RX,
						&pdata->digital_cdc_core_clk);
				}
				if (ret < 0) {
					pr_err("%s: failed to enable CCLK\n",
							__func__);
					mutex_unlock(&pdata->cdc_mclk_mutex);
					return ret;
				}
				pr_debug("enabled digital codec core clk\n");
				atomic_set(&pdata->mclk_enabled, true);
			}
			mutex_unlock(&pdata->cdc_mclk_mutex);
		}
		atomic_inc(&pdata->mclk_rsc_ref);
	} else {
		cancel_delayed_work_sync(&pdata->disable_mclk_work);
		mutex_lock(&pdata->cdc_mclk_mutex);
		if (atomic_read(&pdata->mclk_enabled) == true) {
			if (pdata->afe_clk_ver == AFE_CLK_VERSION_V1) {
				pdata->digital_cdc_clk.clk_val = 0;
				ret = afe_set_digital_codec_core_clock(
					AFE_PORT_ID_PRIMARY_MI2S_RX,
					&pdata->digital_cdc_clk);
			} else {
				pdata->digital_cdc_core_clk.enable = 0;
				ret = afe_set_lpass_clock_v2(
					AFE_PORT_ID_PRIMARY_MI2S_RX,
					&pdata->digital_cdc_core_clk);
			}
			if (ret < 0)
				pr_err("%s: failed to disable CCLK\n",
						__func__);
			atomic_set(&pdata->mclk_enabled, false);
		}
		mutex_unlock(&pdata->cdc_mclk_mutex);
	}
	return ret;
}

static int msm_btsco_rate_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_btsco_rate  = %d", __func__, msm_btsco_rate);
	ucontrol->value.integer.value[0] = msm_btsco_rate;
	return 0;
}

static int msm_btsco_rate_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case RATE_8KHZ_ID:
		msm_btsco_rate = BTSCO_RATE_8KHZ;
		break;
	case RATE_16KHZ_ID:
		msm_btsco_rate = BTSCO_RATE_16KHZ;
		break;
	default:
		msm_btsco_rate = BTSCO_RATE_8KHZ;
		break;
	}

	pr_debug("%s: msm_btsco_rate = %d\n", __func__, msm_btsco_rate);
	return 0;
}

static int mi2s_rx_sample_rate_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int sample_rate_val = 0;

	switch (mi2s_rx_sample_rate) {
	case SAMPLING_RATE_96KHZ:
		sample_rate_val = 1;
		break;
	case SAMPLING_RATE_192KHZ:
		sample_rate_val = 2;
		break;
	case SAMPLING_RATE_48KHZ:
	default:
		sample_rate_val = 0;
		break;
	}
	ucontrol->value.integer.value[0] = sample_rate_val;
	pr_debug("%s: mi2s_rx_sample_rate = %d\n", __func__,
				mi2s_rx_sample_rate);
	return 0;
}

static int mi2s_rx_sample_rate_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int sample_rate_index = 0;

	sample_rate_index = ucontrol->value.integer.value[0];

	switch (sample_rate_index) {
	case 1:
		mi2s_rx_sample_rate = SAMPLING_RATE_96KHZ;
		break;
	case 2:
		mi2s_rx_sample_rate = SAMPLING_RATE_192KHZ;
		break;
	case 0:
	default:
		mi2s_rx_sample_rate = SAMPLING_RATE_48KHZ;
		break;
	}

	pr_debug("%s: sample_rate = %d\n", __func__, mi2s_rx_sample_rate);

	return 0;
}

static int mi2s_rx_bit_format_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{

	switch (mi2s_rx_bit_format) {
	case SNDRV_PCM_FORMAT_S24_3LE:
		ucontrol->value.integer.value[0] = 2;
		break;

	case SNDRV_PCM_FORMAT_S24_LE:
		ucontrol->value.integer.value[0] = 1;
		break;

	case SNDRV_PCM_FORMAT_S16_LE:
	default:
		ucontrol->value.integer.value[0] = 0;
		break;
	}

	pr_debug("%s: mi2s_rx_bit_format = %d, ucontrol value = %ld\n",
			__func__, mi2s_rx_bit_format,
			ucontrol->value.integer.value[0]);

	return 0;
}

static int mi2s_rx_bit_format_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 2:
		mi2s_rx_bit_format = SNDRV_PCM_FORMAT_S24_3LE;
		mi2s_rx_bits_per_sample = 32;
		break;
	case 1:
		mi2s_rx_bit_format = SNDRV_PCM_FORMAT_S24_LE;
		mi2s_rx_bits_per_sample = 32;
		break;
	case 0:
	default:
		mi2s_rx_bit_format = SNDRV_PCM_FORMAT_S16_LE;
		mi2s_rx_bits_per_sample = 16;
		break;
	}
	return 0;
}

static int msm_pri_mi2s_rx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_pri_mi2s_rx_ch  = %d\n", __func__,
		 msm_pri_mi2s_rx_ch);
	ucontrol->value.integer.value[0] = msm_pri_mi2s_rx_ch - 1;
	return 0;
}

static int msm_pri_mi2s_rx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm_pri_mi2s_rx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm_pri_mi2s_rx_ch = %d\n", __func__, msm_pri_mi2s_rx_ch);
	return 1;
}

static int msm_ter_mi2s_tx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_ter_mi2s_tx_ch  = %d\n", __func__,
		 msm_ter_mi2s_tx_ch);
	ucontrol->value.integer.value[0] = msm_ter_mi2s_tx_ch - 1;
	return 0;
}

static int msm_ter_mi2s_tx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm_ter_mi2s_tx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm_ter_mi2s_tx_ch = %d\n", __func__, msm_ter_mi2s_tx_ch);
	return 1;
}

static int msm_vi_feed_tx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = (msm_vi_feed_tx_ch/2 - 1);
	pr_debug("%s: msm_vi_feed_tx_ch = %ld\n", __func__,
				ucontrol->value.integer.value[0]);
	return 0;
}

static int msm_vi_feed_tx_ch_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	msm_vi_feed_tx_ch =
		roundup_pow_of_two(ucontrol->value.integer.value[0] + 2);

	pr_debug("%s: msm_vi_feed_tx_ch = %d\n", __func__, msm_vi_feed_tx_ch);
	return 1;
}

#if defined(CONFIG_SND_SOC_TDM_INTF)
static int msm_pri_tdm_rx_0_ch_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_pri_tdm_rx_0_ch = %d\n", __func__,
		msm_pri_tdm_rx_0_ch);
	ucontrol->value.integer.value[0] = msm_pri_tdm_rx_0_ch - 1;
	return 0;
}

static int msm_pri_tdm_rx_0_ch_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	msm_pri_tdm_rx_0_ch = ucontrol->value.integer.value[0] + 1;
	pr_debug("%s: msm_pri_tdm_rx_0_ch = %d\n", __func__,
		msm_pri_tdm_rx_0_ch);
	return 0;
}

static int msm_pri_tdm_tx_0_ch_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_pri_tdm_tx_0_ch = %d\n", __func__,
		msm_pri_tdm_tx_0_ch);
	ucontrol->value.integer.value[0] = msm_pri_tdm_tx_0_ch - 1;
	return 0;
}

static int msm_pri_tdm_tx_0_ch_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	msm_pri_tdm_tx_0_ch = ucontrol->value.integer.value[0] + 1;
	pr_debug("%s: msm_pri_tdm_tx_0_ch = %d\n", __func__,
		msm_pri_tdm_tx_0_ch);
	return 0;
}

static int msm_sec_tdm_rx_0_ch_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_sec_tdm_rx_0_ch = %d\n", __func__,
		msm_sec_tdm_rx_0_ch);
	ucontrol->value.integer.value[0] = msm_sec_tdm_rx_0_ch - 1;
	return 0;
}

static int msm_sec_tdm_rx_0_ch_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	msm_sec_tdm_rx_0_ch = ucontrol->value.integer.value[0] + 1;
	pr_debug("%s: msm_sec_tdm_rx_0_ch = %d\n", __func__,
		msm_sec_tdm_rx_0_ch);
	return 0;
}

static int msm_sec_tdm_tx_0_ch_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_sec_tdm_tx_0_ch = %d\n", __func__,
		msm_sec_tdm_tx_0_ch);
	ucontrol->value.integer.value[0] = msm_sec_tdm_tx_0_ch - 1;
	return 0;
}

static int msm_sec_tdm_tx_0_ch_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	msm_sec_tdm_tx_0_ch = ucontrol->value.integer.value[0] + 1;
	pr_debug("%s: msm_sec_tdm_tx_0_ch = %d\n", __func__,
		msm_sec_tdm_tx_0_ch);
	return 0;
}

static int msm_pri_tdm_rx_0_bit_format_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	switch (msm_pri_tdm_rx_0_bit_format) {
	case SNDRV_PCM_FORMAT_S32_LE:
		ucontrol->value.integer.value[0] = 3;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		ucontrol->value.integer.value[0] = 2;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		ucontrol->value.integer.value[0] = 1;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	default:
		ucontrol->value.integer.value[0] = 0;
		break;
	}
	pr_debug("%s: msm_pri_tdm_rx_0_bit_format = %ld\n",
		 __func__, ucontrol->value.integer.value[0]);
	return 0;
}

static int msm_pri_tdm_rx_0_bit_format_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 3:
		msm_pri_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S32_LE;
		break;
	case 2:
		msm_pri_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S24_3LE;
		break;
	case 1:
		msm_pri_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S24_LE;
		break;
	case 0:
	default:
		msm_pri_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S16_LE;
		break;
	}
	pr_debug("%s: msm_pri_tdm_rx_0_bit_format = %d\n",
		 __func__, msm_pri_tdm_rx_0_bit_format);
	return 0;
}

static int msm_pri_tdm_tx_0_bit_format_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	switch (msm_pri_tdm_tx_0_bit_format) {
	case SNDRV_PCM_FORMAT_S32_LE:
		ucontrol->value.integer.value[0] = 3;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		ucontrol->value.integer.value[0] = 2;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		ucontrol->value.integer.value[0] = 1;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	default:
		ucontrol->value.integer.value[0] = 0;
		break;
	}
	pr_debug("%s: msm_pri_tdm_tx_0_bit_format = %ld\n",
		 __func__, ucontrol->value.integer.value[0]);
	return 0;
}

static int msm_pri_tdm_tx_0_bit_format_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 3:
		msm_pri_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S32_LE;
		break;
	case 2:
		msm_pri_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S24_3LE;
		break;
	case 1:
		msm_pri_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S24_LE;
		break;
	case 0:
	default:
		msm_pri_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S16_LE;
		break;
	}
	pr_debug("%s: msm_pri_tdm_tx_0_bit_format = %d\n",
		 __func__, msm_pri_tdm_tx_0_bit_format);
	return 0;
}

static int msm_sec_tdm_rx_0_bit_format_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	switch (msm_sec_tdm_rx_0_bit_format) {
	case SNDRV_PCM_FORMAT_S32_LE:
		ucontrol->value.integer.value[0] = 3;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		ucontrol->value.integer.value[0] = 2;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		ucontrol->value.integer.value[0] = 1;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	default:
		ucontrol->value.integer.value[0] = 0;
		break;
	}
	pr_debug("%s: msm_sec_tdm_rx_0_bit_format = %ld\n",
		 __func__, ucontrol->value.integer.value[0]);
	return 0;
}

static int msm_sec_tdm_rx_0_bit_format_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 3:
		msm_sec_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S32_LE;
		break;
	case 2:
		msm_sec_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S24_3LE;
		break;
	case 1:
		msm_sec_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S24_LE;
		break;
	case 0:
	default:
		msm_sec_tdm_rx_0_bit_format = SNDRV_PCM_FORMAT_S16_LE;
		break;
	}
	pr_debug("%s: msm_sec_tdm_rx_0_bit_format = %d\n",
		 __func__, msm_sec_tdm_rx_0_bit_format);
	return 0;
}

static int msm_sec_tdm_tx_0_bit_format_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	switch (msm_sec_tdm_tx_0_bit_format) {
	case SNDRV_PCM_FORMAT_S32_LE:
		ucontrol->value.integer.value[0] = 3;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		ucontrol->value.integer.value[0] = 2;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		ucontrol->value.integer.value[0] = 1;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	default:
		ucontrol->value.integer.value[0] = 0;
		break;
	}
	pr_debug("%s: msm_sec_tdm_tx_0_bit_format = %ld\n",
		 __func__, ucontrol->value.integer.value[0]);
	return 0;
}

static int msm_sec_tdm_tx_0_bit_format_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 3:
		msm_sec_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S32_LE;
		break;
	case 2:
		msm_sec_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S24_3LE;
		break;
	case 1:
		msm_sec_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S24_LE;
		break;
	case 0:
	default:
		msm_sec_tdm_tx_0_bit_format = SNDRV_PCM_FORMAT_S16_LE;
		break;
	}
	pr_debug("%s: msm_sec_tdm_tx_0_bit_format = %d\n",
		 __func__, msm_sec_tdm_tx_0_bit_format);
	return 0;
}

static int msm_pri_tdm_rx_0_sample_rate_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (msm_pri_tdm_rx_0_sample_rate) {
	case SAMPLING_RATE_16KHZ:
		ucontrol->value.integer.value[0] = 0;
		break;
	case SAMPLING_RATE_48KHZ:
	default:
		ucontrol->value.integer.value[0] = 1;
		break;
	}
	pr_debug("%s: msm_pri_tdm_rx_0_sample_rate = %ld\n",
		 __func__, ucontrol->value.integer.value[0]);
	return 0;
}

static int  msm_pri_tdm_rx_0_sample_rate_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 0:
		msm_pri_tdm_rx_0_sample_rate = SAMPLING_RATE_16KHZ;
		break;
	case 1:
	default:
		msm_pri_tdm_rx_0_sample_rate = SAMPLING_RATE_48KHZ;
		break;
	}
	pr_debug("%s: msm_pri_tdm_rx_0_sample_rate = %d\n",
		 __func__, msm_pri_tdm_rx_0_sample_rate);
	return 0;
}

static int msm_sec_tdm_rx_0_sample_rate_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (msm_sec_tdm_rx_0_sample_rate) {
	case SAMPLING_RATE_16KHZ:
		ucontrol->value.integer.value[0] = 0;
		break;
	case SAMPLING_RATE_48KHZ:
	default:
		ucontrol->value.integer.value[0] = 1;
		break;
	}
	pr_debug("%s: msm_sec_tdm_rx_0_sample_rate = %ld\n",
		 __func__, ucontrol->value.integer.value[0]);
	return 0;
}

static int  msm_sec_tdm_rx_0_sample_rate_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 0:
		msm_sec_tdm_rx_0_sample_rate = SAMPLING_RATE_16KHZ;
		break;
	case 1:
	default:
		msm_sec_tdm_rx_0_sample_rate = SAMPLING_RATE_48KHZ;
		break;
	}
	pr_debug("%s: msm_sec_tdm_rx_0_sample_rate = %d\n",
		 __func__, msm_sec_tdm_rx_0_sample_rate);
	return 0;
}

static int msm_pri_tdm_tx_0_sample_rate_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (msm_pri_tdm_tx_0_sample_rate) {
	case SAMPLING_RATE_16KHZ:
		ucontrol->value.integer.value[0] = 0;
		break;
	case SAMPLING_RATE_48KHZ:
	default:
		ucontrol->value.integer.value[0] = 1;
		break;
	}
	pr_debug("%s: msm_pri_tdm_tx_0_sample_rate = %ld\n",
		 __func__, ucontrol->value.integer.value[0]);
	return 0;
}

static int  msm_pri_tdm_tx_0_sample_rate_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 0:
		msm_pri_tdm_tx_0_sample_rate = SAMPLING_RATE_16KHZ;
		break;
	case 1:
	default:
		msm_pri_tdm_tx_0_sample_rate = SAMPLING_RATE_48KHZ;
		break;
	}
	pr_debug("%s: msm_pri_tdm_tx_0_sample_rate = %d\n",
		 __func__, msm_pri_tdm_tx_0_sample_rate);
	return 0;
}

static int msm_sec_tdm_tx_0_sample_rate_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (msm_sec_tdm_tx_0_sample_rate) {
	case SAMPLING_RATE_16KHZ:
		ucontrol->value.integer.value[0] = 0;
		break;
	case SAMPLING_RATE_48KHZ:
	default:
		ucontrol->value.integer.value[0] = 1;
		break;
	}
	pr_debug("%s: msm_sec_tdm_tx_0_sample_rate = %ld\n",
		 __func__, ucontrol->value.integer.value[0]);
	return 0;
}

static int  msm_sec_tdm_tx_0_sample_rate_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 0:
		msm_sec_tdm_tx_0_sample_rate = SAMPLING_RATE_16KHZ;
		break;
	case 1:
	default:
		msm_sec_tdm_tx_0_sample_rate = SAMPLING_RATE_48KHZ;
		break;
	}
	pr_debug("%s: msm_sec_tdm_tx_0_sample_rate = %d\n",
		 __func__, msm_sec_tdm_tx_0_sample_rate);
	return 0;
}
#endif

static const struct soc_enum msm_snd_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(rx_bit_format_text),
				rx_bit_format_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mi2s_ch_text),
				mi2s_ch_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(loopback_mclk_text),
				loopback_mclk_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(btsco_rate_text),
				btsco_rate_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(proxy_rx_ch_text),
				proxy_rx_ch_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(vi_feed_ch_text),
				vi_feed_ch_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mi2s_rx_sample_rate_text),
				mi2s_rx_sample_rate_text),
#if defined(CONFIG_SND_SOC_TDM_INTF)
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tdm_ch_text),
				tdm_ch_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tdm_bit_format_text),
				tdm_bit_format_text),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tdm_sample_rate_text),
				tdm_sample_rate_text),
#endif
};

static const struct snd_kcontrol_new msm_snd_controls[] = {
	SOC_ENUM_EXT("MI2S_RX Format", msm_snd_enum[0],
			mi2s_rx_bit_format_get, mi2s_rx_bit_format_put),
	SOC_ENUM_EXT("MI2S_TX Channels", msm_snd_enum[1],
			msm_ter_mi2s_tx_ch_get, msm_ter_mi2s_tx_ch_put),
	SOC_ENUM_EXT("MI2S_RX Channels", msm_snd_enum[1],
			msm_pri_mi2s_rx_ch_get, msm_pri_mi2s_rx_ch_put),
	SOC_ENUM_EXT("Internal BTSCO SampleRate", msm_snd_enum[3],
		     msm_btsco_rate_get, msm_btsco_rate_put),
	SOC_ENUM_EXT("PROXY_RX Channels", msm_snd_enum[4],
			msm_proxy_rx_ch_get, msm_proxy_rx_ch_put),
	SOC_ENUM_EXT("VI_FEED_TX Channels", msm_snd_enum[5],
			msm_vi_feed_tx_ch_get, msm_vi_feed_tx_ch_put),
	SOC_ENUM_EXT("MI2S_RX SampleRate", msm_snd_enum[6],
			mi2s_rx_sample_rate_get, mi2s_rx_sample_rate_put),
#if defined(CONFIG_SND_SOC_TDM_INTF)
	SOC_ENUM_EXT("PRI_TDM_RX_0 Channels", msm_snd_enum[7],
			msm_pri_tdm_rx_0_ch_get, msm_pri_tdm_rx_0_ch_put),
	SOC_ENUM_EXT("PRI_TDM_TX_0 Channels", msm_snd_enum[7],
			msm_pri_tdm_tx_0_ch_get, msm_pri_tdm_tx_0_ch_put),
	SOC_ENUM_EXT("SEC_TDM_RX_0 Channels", msm_snd_enum[7],
			msm_sec_tdm_rx_0_ch_get, msm_sec_tdm_rx_0_ch_put),
	SOC_ENUM_EXT("SEC_TDM_TX_0 Channels", msm_snd_enum[7],
			msm_sec_tdm_tx_0_ch_get, msm_sec_tdm_tx_0_ch_put),
	SOC_ENUM_EXT("PRI_TDM_RX_0 Bit Format", msm_snd_enum[8],
			msm_pri_tdm_rx_0_bit_format_get,
			msm_pri_tdm_rx_0_bit_format_put),
	SOC_ENUM_EXT("PRI_TDM_TX_0 Bit Format", msm_snd_enum[8],
			msm_pri_tdm_tx_0_bit_format_get,
			msm_pri_tdm_tx_0_bit_format_put),
	SOC_ENUM_EXT("SEC_TDM_RX_0 Bit Format", msm_snd_enum[8],
			msm_sec_tdm_rx_0_bit_format_get,
			msm_sec_tdm_rx_0_bit_format_put),
	SOC_ENUM_EXT("SEC_TDM_TX_0 Bit Format", msm_snd_enum[8],
			msm_sec_tdm_tx_0_bit_format_get,
			msm_sec_tdm_tx_0_bit_format_put),
	SOC_ENUM_EXT("PRI_TDM_RX_0 SampleRate", msm_snd_enum[9],
			msm_pri_tdm_rx_0_sample_rate_get,
			msm_pri_tdm_rx_0_sample_rate_put),
	SOC_ENUM_EXT("PRI_TDM_TX_0 SampleRate", msm_snd_enum[9],
			msm_pri_tdm_tx_0_sample_rate_get,
			msm_pri_tdm_tx_0_sample_rate_put),
	SOC_ENUM_EXT("SEC_TDM_RX_0 SampleRate", msm_snd_enum[9],
			msm_sec_tdm_rx_0_sample_rate_get,
			msm_sec_tdm_rx_0_sample_rate_put),
	SOC_ENUM_EXT("SEC_TDM_TX_0 SampleRate", msm_snd_enum[9],
			msm_sec_tdm_tx_0_sample_rate_get,
			msm_sec_tdm_tx_0_sample_rate_put),
#endif
};

static int msm_mi2s_snd_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_codec *codec = rtd->codec;
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);
	int ret = 0, val = 0;

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
		 substream->name, substream->stream);

	if (!q6core_is_adsp_ready()) {
		pr_err("%s(): adsp not ready\n", __func__);
		return -EINVAL;
	}

	/*
	 * configure the slave select to
	 * invalid state for internal codec
	 */
	if (pdata->vaddr_gpio_mux_spkr_ctl) {
		val = ioread32(pdata->vaddr_gpio_mux_spkr_ctl);
		val = val | 0x00010000;
		iowrite32(val, pdata->vaddr_gpio_mux_spkr_ctl);
	}

	if (pdata->vaddr_gpio_mux_mic_ctl) {
		val = ioread32(pdata->vaddr_gpio_mux_mic_ctl);
		val = val | 0x00200000;
		iowrite32(val, pdata->vaddr_gpio_mux_mic_ctl);
	}

	ret = msm_mi2s_sclk_ctl(substream, true);
	if (ret < 0) {
		pr_err("%s: failed to enable sclk %d\n",
				__func__, ret);
		return ret;
	}
	ret =  msm8952_enable_dig_cdc_clk(codec, 1, true);
	if (ret < 0) {
		pr_err("failed to enable mclk\n");
		return ret;
	}
	/* Enable the codec mclk config */
	ret = msm_gpioset_activate(CLIENT_WCD_INT, "pri_i2s");
	if (ret < 0) {
		pr_err("%s: gpio set cannot be activated %sd",
				__func__, "pri_i2s");
		return ret;
	}
	/* msm8x16_wcd_mclk_enable(codec, 1, true); */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		pr_err("%s: set fmt cpu dai failed; ret=%d\n", __func__, ret);

	return ret;
}
static void msm_mi2s_snd_shutdown(struct snd_pcm_substream *substream)
{
	int ret;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
			substream->name, substream->stream);

	ret = msm_mi2s_sclk_ctl(substream, false);
	if (ret < 0)
		pr_err("%s:clock disable failed; ret=%d\n", __func__,
				ret);
	if (atomic_read(&pdata->mclk_rsc_ref) > 0) {
		atomic_dec(&pdata->mclk_rsc_ref);
		pr_err("%s: decrementing mclk_res_ref %d\n",
				__func__, atomic_read(&pdata->mclk_rsc_ref));
	}
}

static int msm_prim_auxpcm_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);
	int ret = 0, val = 0;

	pr_debug("%s(): substream = %s\n",
			__func__, substream->name);

	if (!q6core_is_adsp_ready()) {
		pr_err("%s(): adsp not ready\n", __func__);
		return -EINVAL;
	}

	/* mux config to route the AUX MI2S */
	if (pdata->vaddr_gpio_mux_mic_ctl) {
		val = ioread32(pdata->vaddr_gpio_mux_mic_ctl);
		val = val | 0x2;
		iowrite32(val, pdata->vaddr_gpio_mux_mic_ctl);
	}
	if (pdata->vaddr_gpio_mux_pcm_ctl) {
		val = ioread32(pdata->vaddr_gpio_mux_pcm_ctl);
		val = val | 0x1;
		iowrite32(val, pdata->vaddr_gpio_mux_pcm_ctl);
	}
	msm8952_enable_dig_cdc_clk(codec, 1, true);
	atomic_inc(&auxpcm_mi2s_clk_ref);

	/* enable the gpio's used for the external AUXPCM interface */
	ret = msm_gpioset_activate(CLIENT_WCD_INT, "quat_i2s");
	if (ret < 0)
		pr_err("%s(): configure gpios failed = %s\n",
				__func__, "quat_i2s");
	return ret;
}

static void msm_prim_auxpcm_shutdown(struct snd_pcm_substream *substream)
{
	int ret;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_codec *codec = rtd->codec;
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);

	pr_debug("%s(): substream = %s\n",
			__func__, substream->name);
	if (atomic_read(&pdata->mclk_rsc_ref) > 0) {
		atomic_dec(&pdata->mclk_rsc_ref);
		pr_debug("%s: decrementing mclk_res_ref %d\n",
			__func__, atomic_read(&pdata->mclk_rsc_ref));
	}
	if (atomic_read(&auxpcm_mi2s_clk_ref) > 0)
		atomic_dec(&auxpcm_mi2s_clk_ref);
	if ((atomic_read(&auxpcm_mi2s_clk_ref) == 0) &&
		(atomic_read(&pdata->mclk_rsc_ref) == 0)) {
		msm8952_enable_dig_cdc_clk(codec, 0, true);
	}
	ret = msm_gpioset_suspend(CLIENT_WCD_INT, "quat_i2s");
	if (ret < 0)
		pr_err("%s(): configure gpios failed = %s\n",
				__func__, "quat_i2s");
}

static int msm_sec_mi2s_snd_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct msm8952_asoc_mach_data *pdata =
			snd_soc_card_get_drvdata(card);
	int ret = 0, val = 0;

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
				substream->name, substream->stream);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		pr_info("%s: Secondary Mi2s does not support capture\n",
					__func__);
		return 0;
	}

	if (!q6core_is_adsp_ready()) {
		pr_err("%s(): adsp not ready\n", __func__);
		return -EINVAL;
	}

	if ((pdata->ext_pa & SEC_MI2S_ID) == SEC_MI2S_ID) {
		if (pdata->vaddr_gpio_mux_spkr_ctl) {
			val = ioread32(pdata->vaddr_gpio_mux_spkr_ctl);
			val = val | 0x0004835c;
			iowrite32(val, pdata->vaddr_gpio_mux_spkr_ctl);
		}
		ret = msm_mi2s_sclk_ctl(substream, true);
		if (ret < 0) {
			pr_err("failed to enable sclk\n");
			return ret;
		}
		pr_debug("%s(): SEC I2S gpios turned on  = %s\n", __func__,
				"sec_i2s");
		ret = msm_gpioset_activate(CLIENT_WCD_INT, "sec_i2s");
		if (ret < 0) {
			pr_err("%s: gpio set cannot be activated %sd",
						__func__, "sec_i2s");
			goto err;
		}
	} else {
		pr_err("%s: error codec type\n", __func__);
	}
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		pr_err("%s: set fmt cpu dai failed\n", __func__);
		ret = msm_gpioset_suspend(CLIENT_WCD_INT, "sec_i2s");
		if (ret < 0) {
			pr_err("%s: gpio set cannot be de-activated %sd",
						__func__, "sec_i2s");
			goto err;
		}
	}
	return ret;
err:
	ret = msm_mi2s_sclk_ctl(substream, false);
	if (ret < 0)
		pr_err("failed to disable sclk\n");
	return ret;
}

static void msm_sec_mi2s_snd_shutdown(struct snd_pcm_substream *substream)
{
	int ret;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
				substream->name, substream->stream);
	if ((pdata->ext_pa & SEC_MI2S_ID) == SEC_MI2S_ID) {
		ret = msm_gpioset_suspend(CLIENT_WCD_INT, "sec_i2s");
		if (ret < 0) {
			pr_err("%s: gpio set cannot be de-activated: %sd",
					__func__, "sec_i2s");
			return;
		}
		ret = msm_mi2s_sclk_ctl(substream, false);
		if (ret < 0)
			pr_err("%s:clock disable failed\n", __func__);
	}
}

#if defined(CONFIG_SND_SOC_TDM_INTF)
int msm_tdm_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct msm8952_asoc_mach_data *pdata =
			snd_soc_card_get_drvdata(card);
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0, val = 0;

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
			 substream->name, substream->stream);
	pr_debug("%s: dai id = 0x%x\n", __func__, cpu_dai->id);

	switch (cpu_dai->id) {
	case AFE_PORT_ID_PRIMARY_TDM_RX:
	case AFE_PORT_ID_PRIMARY_TDM_RX_1:
	case AFE_PORT_ID_PRIMARY_TDM_RX_2:
	case AFE_PORT_ID_PRIMARY_TDM_RX_3:
	case AFE_PORT_ID_PRIMARY_TDM_RX_4:
	case AFE_PORT_ID_PRIMARY_TDM_RX_5:
	case AFE_PORT_ID_PRIMARY_TDM_RX_6:
	case AFE_PORT_ID_PRIMARY_TDM_RX_7:
	case AFE_PORT_ID_PRIMARY_TDM_TX:
	case AFE_PORT_ID_PRIMARY_TDM_TX_1:
	case AFE_PORT_ID_PRIMARY_TDM_TX_2:
	case AFE_PORT_ID_PRIMARY_TDM_TX_3:
	case AFE_PORT_ID_PRIMARY_TDM_TX_4:
	case AFE_PORT_ID_PRIMARY_TDM_TX_5:
	case AFE_PORT_ID_PRIMARY_TDM_TX_6:
	case AFE_PORT_ID_PRIMARY_TDM_TX_7:
		/* Configure mux for Primary TDM */
		if (pdata->vaddr_gpio_mux_pcm_ctl) {
			val = ioread32(pdata->vaddr_gpio_mux_pcm_ctl);
			val = val | 0x00000001;
			iowrite32(val, pdata->vaddr_gpio_mux_pcm_ctl);
		} else {
			return -EINVAL;
		}

		if (pdata->vaddr_gpio_mux_mic_ctl) {
			val = ioread32(pdata->vaddr_gpio_mux_mic_ctl);
			val = val | 0x00000002;
			iowrite32(val, pdata->vaddr_gpio_mux_mic_ctl);
		} else {
			return -EINVAL;
		}

		ret = msm_gpioset_activate(CLIENT_WCD_INT, "quat_i2s");
		if (ret < 0)
			pr_err("%s: failed to activate primary TDM gpio set\n",
				   __func__);
		break;
	case AFE_PORT_ID_SECONDARY_TDM_RX:
	case AFE_PORT_ID_SECONDARY_TDM_RX_1:
	case AFE_PORT_ID_SECONDARY_TDM_RX_2:
	case AFE_PORT_ID_SECONDARY_TDM_RX_3:
	case AFE_PORT_ID_SECONDARY_TDM_RX_4:
	case AFE_PORT_ID_SECONDARY_TDM_RX_5:
	case AFE_PORT_ID_SECONDARY_TDM_RX_6:
	case AFE_PORT_ID_SECONDARY_TDM_RX_7:
	case AFE_PORT_ID_SECONDARY_TDM_TX:
	case AFE_PORT_ID_SECONDARY_TDM_TX_1:
	case AFE_PORT_ID_SECONDARY_TDM_TX_2:
	case AFE_PORT_ID_SECONDARY_TDM_TX_3:
	case AFE_PORT_ID_SECONDARY_TDM_TX_4:
	case AFE_PORT_ID_SECONDARY_TDM_TX_5:
	case AFE_PORT_ID_SECONDARY_TDM_TX_6:
	case AFE_PORT_ID_SECONDARY_TDM_TX_7:
		/* Configure mux for Secondary TDM */
		if (pdata->vaddr_gpio_mux_qui_pcm_sec_mode_ctl) {
			val = ioread32(
				pdata->vaddr_gpio_mux_qui_pcm_sec_mode_ctl);
			val = val | 0x00000001;
			iowrite32(val,
				pdata->vaddr_gpio_mux_qui_pcm_sec_mode_ctl);
		} else {
			return -EINVAL;
		}

		if (pdata->vaddr_gpio_mux_quin_ctl) {
			val = ioread32(pdata->vaddr_gpio_mux_quin_ctl);
			val = val | 0x00000001;
			iowrite32(val, pdata->vaddr_gpio_mux_quin_ctl);
		} else {
			return -EINVAL;
		}

		if (pdata->vaddr_gpio_mux_mic_ext_clk_ctl) {
			val = ioread32(pdata->vaddr_gpio_mux_mic_ext_clk_ctl);
			val = val | 0x00000001;
			iowrite32(val, pdata->vaddr_gpio_mux_mic_ext_clk_ctl);
		} else {
			return -EINVAL;
		}

		if (pdata->vaddr_gpio_mux_sec_tlmm_ctl) {
			val = ioread32(pdata->vaddr_gpio_mux_sec_tlmm_ctl);
			val = val | 0x00000002;
			iowrite32(val, pdata->vaddr_gpio_mux_sec_tlmm_ctl);
		} else {
			return -EINVAL;
		}

		if (pdata->vaddr_gpio_mux_spkr_ctl) {
			val = ioread32(pdata->vaddr_gpio_mux_spkr_ctl);
			val = val | 0x00000002;
			iowrite32(val, pdata->vaddr_gpio_mux_spkr_ctl);
		} else {
			return -EINVAL;
		}
		ret = msm_gpioset_activate(CLIENT_WCD_INT, "quin_i2s");
		if (ret < 0)
			pr_err("%s: failed to activate secondary TDM gpio set\n",
				   __func__);
		break;
	default:
		pr_err("%s: dai id 0x%x not supported\n",
		       __func__, cpu_dai->id);
		break;
		return -EINVAL;
	}
	return ret;
}

void msm_tdm_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	switch (cpu_dai->id) {
	case AFE_PORT_ID_PRIMARY_TDM_RX:
	case AFE_PORT_ID_PRIMARY_TDM_RX_1:
	case AFE_PORT_ID_PRIMARY_TDM_RX_2:
	case AFE_PORT_ID_PRIMARY_TDM_RX_3:
	case AFE_PORT_ID_PRIMARY_TDM_RX_4:
	case AFE_PORT_ID_PRIMARY_TDM_RX_5:
	case AFE_PORT_ID_PRIMARY_TDM_RX_6:
	case AFE_PORT_ID_PRIMARY_TDM_RX_7:
	case AFE_PORT_ID_PRIMARY_TDM_TX:
	case AFE_PORT_ID_PRIMARY_TDM_TX_1:
	case AFE_PORT_ID_PRIMARY_TDM_TX_2:
	case AFE_PORT_ID_PRIMARY_TDM_TX_3:
	case AFE_PORT_ID_PRIMARY_TDM_TX_4:
	case AFE_PORT_ID_PRIMARY_TDM_TX_5:
	case AFE_PORT_ID_PRIMARY_TDM_TX_6:
	case AFE_PORT_ID_PRIMARY_TDM_TX_7:
		ret = msm_gpioset_suspend(CLIENT_WCD_INT, "quat_i2s");
		if (ret < 0) {
			pr_err("%s: gpio set cannot be de-activated %s\n",
					__func__, "pri_tdm");
			return;
		}
		break;
	case AFE_PORT_ID_SECONDARY_TDM_RX:
	case AFE_PORT_ID_SECONDARY_TDM_RX_1:
	case AFE_PORT_ID_SECONDARY_TDM_RX_2:
	case AFE_PORT_ID_SECONDARY_TDM_RX_3:
	case AFE_PORT_ID_SECONDARY_TDM_RX_4:
	case AFE_PORT_ID_SECONDARY_TDM_RX_5:
	case AFE_PORT_ID_SECONDARY_TDM_RX_6:
	case AFE_PORT_ID_SECONDARY_TDM_RX_7:
	case AFE_PORT_ID_SECONDARY_TDM_TX:
	case AFE_PORT_ID_SECONDARY_TDM_TX_1:
	case AFE_PORT_ID_SECONDARY_TDM_TX_2:
	case AFE_PORT_ID_SECONDARY_TDM_TX_3:
	case AFE_PORT_ID_SECONDARY_TDM_TX_4:
	case AFE_PORT_ID_SECONDARY_TDM_TX_5:
	case AFE_PORT_ID_SECONDARY_TDM_TX_6:
	case AFE_PORT_ID_SECONDARY_TDM_TX_7:
		ret = msm_gpioset_suspend(CLIENT_WCD_INT, "quin_i2s");
		if (ret < 0) {
			pr_err("%s: gpio set cannot be de-activated %s\n",
				   __func__, "sec_tdm");
			return;
		}
		break;
	}
}

#endif
static int msm_quat_mi2s_snd_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct msm8952_asoc_mach_data *pdata =
			snd_soc_card_get_drvdata(card);
	int ret = 0, val = 0;

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
				substream->name, substream->stream);

	if (!q6core_is_adsp_ready()) {
		pr_err("%s(): adsp not ready\n", __func__);
		return -EINVAL;
	}

	if ((pdata->ext_pa & QUAT_MI2S_ID) == QUAT_MI2S_ID) {
		if (pdata->vaddr_gpio_mux_mic_ctl) {
			val = ioread32(pdata->vaddr_gpio_mux_mic_ctl);
			val = val | 0x02020002;
			iowrite32(val, pdata->vaddr_gpio_mux_mic_ctl);
		}
		ret = msm_mi2s_sclk_ctl(substream, true);
		if (ret < 0) {
			pr_err("failed to enable sclk\n");
			return ret;
		}
		ret = msm_gpioset_activate(CLIENT_WCD_INT, "quat_i2s");
		if (ret < 0) {
			pr_err("failed to enable codec gpios\n");
			goto err;
		}
	} else {
		pr_err("%s: error codec type\n", __func__);
	}
	if (atomic_inc_return(&quat_mi2s_clk_ref) == 1) {
		ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0)
			pr_err("%s: set fmt cpu dai failed\n", __func__);
	}
	return ret;
err:
	ret = msm_mi2s_sclk_ctl(substream, false);
	if (ret < 0)
		pr_err("failed to disable sclk\n");
	return ret;
}

static void msm_quat_mi2s_snd_shutdown(struct snd_pcm_substream *substream)
{
	int ret;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
				substream->name, substream->stream);
	if ((pdata->ext_pa & QUAT_MI2S_ID) == QUAT_MI2S_ID) {
		ret = msm_mi2s_sclk_ctl(substream, false);
		if (ret < 0)
			pr_err("%s:clock disable failed\n", __func__);
		if (atomic_read(&quat_mi2s_clk_ref) > 0)
			atomic_dec(&quat_mi2s_clk_ref);
		ret = msm_gpioset_suspend(CLIENT_WCD_INT, "quat_i2s");
		if (ret < 0) {
			pr_err("%s: gpio set cannot be de-activated %sd",
						__func__, "quat_i2s");
			return;
		}
	}
}

static int msm_quin_mi2s_snd_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct msm8952_asoc_mach_data *pdata =
			snd_soc_card_get_drvdata(card);
	int ret = 0, val = 0;

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
				substream->name, substream->stream);

	if (!q6core_is_adsp_ready()) {
		pr_err("%s(): adsp not ready\n", __func__);
		return -EINVAL;
	}

	if (pdata->vaddr_gpio_mux_quin_ctl) {
		val = ioread32(pdata->vaddr_gpio_mux_quin_ctl);
		val = val | 0x00000001;
		iowrite32(val, pdata->vaddr_gpio_mux_quin_ctl);
	} else {
		return -EINVAL;
	}
	ret = msm_mi2s_sclk_ctl(substream, true);
	if (ret < 0) {
		pr_err("failed to enable sclk\n");
		return ret;
	}
	ret = msm_gpioset_activate(CLIENT_WCD_INT, "quin_i2s");
	if (ret < 0) {
		pr_err("failed to enable codec gpios\n");
		goto err;
	}
	if (atomic_inc_return(&quin_mi2s_clk_ref) == 1) {
		ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0)
			pr_err("%s: set fmt cpu dai failed\n", __func__);
	}
	return ret;
err:
	ret = msm_mi2s_sclk_ctl(substream, false);
	if (ret < 0)
		pr_err("failed to disable sclk\n");
	return ret;
}

static void msm_quin_mi2s_snd_shutdown(struct snd_pcm_substream *substream)
{
	int ret;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
				substream->name, substream->stream);
		ret = msm_mi2s_sclk_ctl(substream, false);
		if (ret < 0)
			pr_err("%s:clock disable failed\n", __func__);
		if (atomic_read(&quin_mi2s_clk_ref) > 0)
			atomic_dec(&quin_mi2s_clk_ref);
		ret = msm_gpioset_suspend(CLIENT_WCD_INT, "quin_i2s");
		if (ret < 0) {
			pr_err("%s: gpio set cannot be de-activated %sd",
						__func__, "quin_i2s");
			return;
		}

	msm8952_rt5665_set_mclk(card, 0);
}

static int msm8952_rt5665_device_down(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "%s: device down!\n", __func__);
	snd_soc_card_change_online_state(codec->component.card, 0);
	return 0;
}

static int msm8952_rt5665_device_up(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "%s: device up!\n", __func__);
	snd_soc_card_change_online_state(codec->component.card, 1);
	return 0;
}

static int adsp_state_callback(struct notifier_block *nb, unsigned long value,
			       void *priv)
{
	bool timedout;
	unsigned long timeout;

	if (value == SUBSYS_BEFORE_SHUTDOWN)
		msm8952_rt5665_device_down(registered_codec);
	else if (value == SUBSYS_AFTER_POWERUP) {
		dev_info(registered_codec->dev,
			"ADSP is about to power up. bring up codec\n");
		if (!q6core_is_adsp_ready()) {
			dev_info(registered_codec->dev,
				"ADSP isn't ready\n");
			timeout = jiffies +
				  msecs_to_jiffies(50);
			while (!(timedout = time_after(jiffies, timeout))) {
				if (!q6core_is_adsp_ready()) {
					dev_info(registered_codec->dev,
						"ADSP isn't ready\n");
				} else {
					dev_info(registered_codec->dev,
						"ADSP is ready\n");
					break;
				}
			}
		} else {
			dev_info(registered_codec->dev,
				"%s: DSP is ready\n", __func__);
		}
		msm8952_rt5665_device_up(registered_codec);
	}
	return NOTIFY_OK;
}

static struct notifier_block adsp_state_notifier_block = {
	.notifier_call = adsp_state_callback,
	.priority = -INT_MAX,
};

static int rt5665_audrx_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_card *card = codec->component.card;
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);

	dev_info(cpu_dai->dev, "%s\n", __func__);

	/* close codec device immediately when pcm is closed */
	codec->component.ignore_pmdown_time = true;

	snd_soc_dapm_ignore_suspend(&card->dapm, "Main Mic");
	snd_soc_dapm_ignore_suspend(&card->dapm, "Sub Mic");
	snd_soc_dapm_ignore_suspend(&card->dapm, "Headset Mic");
	snd_soc_dapm_ignore_suspend(&card->dapm, "RCV");
	snd_soc_dapm_ignore_suspend(&card->dapm, "SPK");
	snd_soc_dapm_ignore_suspend(&card->dapm, "HP");
	snd_soc_dapm_sync(&card->dapm);

	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF1 Playback");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF1_1 Capture");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF2_1 Playback");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF2_1 Capture");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF3 Playback");
	snd_soc_dapm_ignore_suspend(&codec->dapm, "AIF3 Capture");
	snd_soc_dapm_sync(&codec->dapm);

	pdata->codec = codec;	/* added rt5665 codec */
	rt5665_sysclk.codec_dai = rtd->codec_dai;

	registered_codec = codec;
	adsp_state_notifier =
	    subsys_notif_register_notifier("adsp",
					   &adsp_state_notifier_block);
	if (!adsp_state_notifier) {
		dev_err(codec->dev, "Failed to register adsp state notifier\n");
		registered_codec = NULL;
		return -ENOMEM;
	}

	snd_soc_add_codec_controls(codec, msm_snd_controls,
			ARRAY_SIZE(msm_snd_controls));

#if defined(CONFIG_SND_SOC_DBMDX)
		dbmdx_remote_add_codec_controls(codec);
#endif

	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &hs_jack);
	rt5665_set_jack_detect(codec, &hs_jack);
	snd_jack_set_key(hs_jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	snd_jack_set_key(hs_jack.jack, SND_JACK_BTN_1, KEY_VOICECOMMAND);
	snd_jack_set_key(hs_jack.jack, SND_JACK_BTN_2, KEY_VOLUMEUP);
	snd_jack_set_key(hs_jack.jack, SND_JACK_BTN_3, KEY_VOLUMEDOWN);

	return 0;
}

static int msm_rt5665_aif3_mi2s_startup(struct snd_pcm_substream *substream)
{
	int ret = 0;

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
				substream->name, substream->stream);

	return ret;
}

static void msm_rt5665_aif3_mi2s_shutdown(struct snd_pcm_substream *substream)
{
	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
				substream->name, substream->stream);
}

static int msm_rt5665_aif3_mi2s_snd_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct msm8952_asoc_mach_data *pdata = NULL;
	int ret;

	pdata = snd_soc_card_get_drvdata(card);

	dev_info(card->dev, "%s-%d %dch, %dHz\n",
			rtd->dai_link->name, substream->stream,
			params_channels(params), params_rate(params));

	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
				| SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0) {
		dev_err(codec_dai->dev, "Failed to set master mode: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_pll(codec_dai, 0, RT5665_PLL1_S_MCLK,
			pdata->mclk_freq, 24576000);
	if (ret < 0) {
		dev_err(card->dev, "codec_dai pll not set\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, RT5665_SCLK_S_PLL1,
			24576000, SND_SOC_CLOCK_IN);

	if (ret < 0) {
		dev_err(card->dev, "codec_dai clock not set\n");
		return ret;
	}

	return 0;
}

static int msm8952_rt5665_start_sysclk(struct snd_soc_card *card)
{
	int ret;

	dev_info(card->dev, "%s\n", __func__);

	//msm8952_rt5665_set_mclk(card, 1);

	ret = snd_soc_dai_set_pll(rt5665_sysclk.codec_dai,
			0, rt5665_sysclk.clk_id,
			rt5665_sysclk.fll_in, rt5665_sysclk.fll_out);
	if (ret < 0) {
		dev_err(card->dev, "codec_dai pll not set\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(rt5665_sysclk.codec_dai,
			RT5665_SCLK_S_PLL1,
			rt5665_sysclk.fll_out, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(card->dev, "codec_dai clock not set\n");
		return ret;
	}

	return 0;
}

static int msm8952_rt5665_stop_sysclk(struct snd_soc_card *card)
{
	dev_info(card->dev, "%s\n", __func__);

	snd_soc_dai_set_sysclk(rt5665_sysclk.codec_dai, RT5665_SCLK_S_PLL1, 0, 0);

	return 0;
}

static int msm8952_rt5665_set_bias_level(struct snd_soc_card *card,
				struct snd_soc_dapm_context *dapm,
				enum snd_soc_bias_level level)
{
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);

	if (!pdata->codec || dapm != &pdata->codec->dapm)
		return 0;

	switch (level) {
	case SND_SOC_BIAS_OFF:
		if (card->dapm.bias_level == SND_SOC_BIAS_STANDBY)
			msm8952_rt5665_stop_sysclk(card);
		break;
	case SND_SOC_BIAS_STANDBY:
		if (card->dapm.bias_level == SND_SOC_BIAS_OFF)
			msm8952_rt5665_start_sysclk(card);
		break;
	default:
		break;
	}

	card->dapm.bias_level = level;
	dev_dbg(card->dev, "%s: %d\n", __func__, level);

	return 0;
}

static struct snd_soc_ops msm8952_quat_mi2s_be_ops = {
	.startup = msm_quat_mi2s_snd_startup,
	.hw_params = msm_mi2s_snd_hw_params,
	.shutdown = msm_quat_mi2s_snd_shutdown,
};

static struct snd_soc_ops msm8952_quin_mi2s_be_ops = {
	.startup = msm_quin_mi2s_snd_startup,
	.hw_params = msm_rt5665_aif1_mi2s_snd_hw_params,
	.shutdown = msm_quin_mi2s_snd_shutdown,
};

static struct snd_soc_ops msm8952_sec_mi2s_be_ops = {
	.startup = msm_sec_mi2s_snd_startup,
	.hw_params = msm_mi2s_snd_hw_params,
	.shutdown = msm_sec_mi2s_snd_shutdown,
};

static struct snd_soc_ops msm8952_mi2s_be_ops = {
	.startup = msm_mi2s_snd_startup,
	.hw_params = msm_mi2s_snd_hw_params,
	.shutdown = msm_mi2s_snd_shutdown,
};

static struct snd_soc_ops msm_pri_auxpcm_be_ops = {
	.startup = msm_prim_auxpcm_startup,
	.shutdown = msm_prim_auxpcm_shutdown,
};

static struct snd_soc_ops msm8952_dummy_mi2s_be_ops = {
	.startup = msm_rt5665_aif3_mi2s_startup,
	.hw_params = msm_rt5665_aif3_mi2s_snd_hw_params,
	.shutdown = msm_rt5665_aif3_mi2s_shutdown,
};

#if defined(CONFIG_SND_SOC_TDM_INTF)
static struct snd_soc_ops msm_tdm_be_ops = {
	.startup = msm_tdm_startup,
	.hw_params = msm_tdm_snd_hw_params,
	.shutdown = msm_tdm_shutdown,
};
#endif


#if defined(CONFIG_SND_SOC_TFA9896)
static struct snd_soc_dai_link_component amp_tdm[] = {
	{
		.name = "tfa98xx.6-0034",
		.dai_name = "tfa98xx-aif-6-34",
	}, {
		.name = "tfa98xx.6-0035",
		.dai_name = "tfa98xx-aif-6-35",
	}, {
		.name = "tfa98xx.6-0036",
		.dai_name = "tfa98xx-aif-6-36",
	}, {
		.name = "tfa98xx.6-0037",
		.dai_name = "tfa98xx-aif-6-37",
	},
};
#endif

/* Digital audio interface glue - connects codec <---> CPU */
static struct snd_soc_dai_link msm8952_dai[] = {
	/* FrontEnd DAI Links */
	{/* hw:x,0 */
		.name = "MSM8952 Media1",
		.stream_name = "MultiMedia1",
		.cpu_dai_name	= "MultiMedia1",
		.platform_name  = "msm-pcm-dsp.0",
		.dynamic = 1,
		.async_ops = ASYNC_DPCM_SND_SOC_PREPARE,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA1
	},
	{/* hw:x,1 */
		.name = "MSM8952 Media2",
		.stream_name = "MultiMedia2",
		.cpu_dai_name   = "MultiMedia2",
		.platform_name  = "msm-pcm-dsp.0",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA2,
	},
	{/* hw:x,2 */
		.name = "Circuit-Switch Voice",
		.stream_name = "CS-Voice",
		.cpu_dai_name   = "CS-VOICE",
		.platform_name  = "msm-pcm-voice",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_CS_VOICE,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{/* hw:x,3 */
		.name = "MSM VoIP",
		.stream_name = "VoIP",
		.cpu_dai_name	= "VoIP",
		.platform_name  = "msm-voip-dsp",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_VOIP,
	},
	{/* hw:x,4 */
		.name = "MSM8X16 ULL",
		.stream_name = "ULL",
		.cpu_dai_name	= "MultiMedia3",
		.platform_name  = "msm-pcm-dsp.2",
		.dynamic = 1,
		.async_ops = ASYNC_DPCM_SND_SOC_PREPARE,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA3,
	},
	/* Hostless PCM purpose */
	{/* hw:x,5 */
		.name = "Primary MI2S_RX Hostless",
		.stream_name = "Primary MI2S_RX Hostless",
		.cpu_dai_name = "PRI_MI2S_RX_HOSTLESS",
		.platform_name	= "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		 /* this dailink has playback support */
		.ignore_pmdown_time = 1,
		/* This dainlink has MI2S support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{/* hw:x,6 */
		.name = "INT_FM Hostless",
		.stream_name = "INT_FM Hostless",
		.cpu_dai_name	= "INT_FM_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{/* hw:x,7 */
		.name = "MSM AFE-PCM RX",
		.stream_name = "AFE-PROXY RX",
		.cpu_dai_name = "msm-dai-q6-dev.241",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.platform_name  = "msm-pcm-afe",
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
	},
	{/* hw:x,8 */
		.name = "MSM AFE-PCM TX",
		.stream_name = "AFE-PROXY TX",
		.cpu_dai_name = "msm-dai-q6-dev.240",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.platform_name  = "msm-pcm-afe",
		.ignore_suspend = 1,
	},
	{/* hw:x,9 */
		.name = "MSM8952 Compress1",
		.stream_name = "Compress1",
		.cpu_dai_name	= "MultiMedia4",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.async_ops = ASYNC_DPCM_SND_SOC_HW_PARAMS,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		/* this dainlink has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA4,
	},
	{/* hw:x,10 */
		.name = "AUXPCM Hostless",
		.stream_name = "AUXPCM Hostless",
		.cpu_dai_name   = "AUXPCM_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{/* hw:x,11 */
		.name = "Tertiary MI2S_TX Hostless",
		.stream_name = "Tertiary MI2S_TX Hostless",
		.cpu_dai_name = "TERT_MI2S_TX_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* dai link has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{/* hw:x,12 */
		.name = "MSM8x16 LowLatency",
		.stream_name = "MultiMedia5",
		.cpu_dai_name   = "MultiMedia5",
		.platform_name  = "msm-pcm-dsp.1",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.async_ops = ASYNC_DPCM_SND_SOC_PREPARE |
			ASYNC_DPCM_SND_SOC_HW_PARAMS,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA5,
	},
	{/* hw:x,13 */
		.name = "Voice2",
		.stream_name = "Voice2",
		.cpu_dai_name   = "Voice2",
		.platform_name  = "msm-pcm-voice",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_VOICE2,
	},
	{/* hw:x,14 */
		.name = "MSM8x16 Media9",
		.stream_name = "MultiMedia9",
		.cpu_dai_name   = "MultiMedia9",
		.platform_name  = "msm-pcm-dsp.0",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		/* This dailink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA9,
	},
	{ /* hw:x,15 */
		.name = "VoLTE",
		.stream_name = "VoLTE",
		.cpu_dai_name   = "VoLTE",
		.platform_name  = "msm-pcm-voice",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_VOLTE,
	},
	{ /* hw:x,16 */
		.name = "VoWLAN",
		.stream_name = "VoWLAN",
		.cpu_dai_name   = "VoWLAN",
		.platform_name  = "msm-pcm-voice",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_VOWLAN,
	},
	{/* hw:x,17 */
		.name = "INT_HFP_BT Hostless",
		.stream_name = "INT_HFP_BT Hostless",
		.cpu_dai_name = "INT_HFP_BT_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		/* this dai link has playback support */
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{/* hw:x,18 */
		.name = "MSM8916 HFP",
		.stream_name = "MultiMedia6",
		.cpu_dai_name = "MultiMedia6",
		.platform_name  = "msm-pcm-loopback",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		/* this dai link has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA6,
	},
	/* LSM FE */
	{/* hw:x,19 */
		.name = "Listen 1 Audio Service",
		.stream_name = "Listen 1 Audio Service",
		.cpu_dai_name = "LSM1",
		.platform_name = "msm-lsm-client",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST },
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_LSM1,
	},
	{/* hw:x,20 */
		.name = "Listen 2 Audio Service",
		.stream_name = "Listen 2 Audio Service",
		.cpu_dai_name = "LSM2",
		.platform_name = "msm-lsm-client",
		.dynamic = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST },
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_LSM2,
	},
	{/* hw:x,21 */
		.name = "Listen 3 Audio Service",
		.stream_name = "Listen 3 Audio Service",
		.cpu_dai_name = "LSM3",
		.platform_name = "msm-lsm-client",
		.dynamic = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST },
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_LSM3,
	},
	{/* hw:x,22 */
		.name = "Listen 4 Audio Service",
		.stream_name = "Listen 4 Audio Service",
		.cpu_dai_name = "LSM4",
		.platform_name = "msm-lsm-client",
		.dynamic = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST },
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_LSM4,
	},
	{/* hw:x,23 */
		.name = "Listen 5 Audio Service",
		.stream_name = "Listen 5 Audio Service",
		.cpu_dai_name = "LSM5",
		.platform_name = "msm-lsm-client",
		.dynamic = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST },
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_LSM5,
	},
	{ /* hw:x,24 */
		.name = "MSM8X16 Compress2",
		.stream_name = "Compress2",
		.cpu_dai_name   = "MultiMedia7",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.dpcm_playback = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA7,
	},
	{ /* hw:x,25 */
		.name = "QUIN_MI2S Hostless",
		.stream_name = "QUIN_MI2S Hostless",
		.cpu_dai_name = "QUIN_MI2S_RX_HOSTLESS",
		.platform_name = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{/* hw:x,26 */
		.name = LPASS_BE_SENARY_MI2S_TX,
		.stream_name = "Senary_mi2s Capture",
		.cpu_dai_name = "msm-dai-q6-mi2s.6",
		.platform_name = "msm-pcm-hostless",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.be_id = MSM_BACKEND_DAI_SENARY_MI2S_TX,
		.be_hw_params_fixup = msm_senary_tx_be_hw_params_fixup,
		.ops = &msm8952_mi2s_be_ops,
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.dpcm_capture = 1,
		.ignore_pmdown_time = 1,
	},
	{/* hw:x,27 */
		.name = "MSM8X16 Compress3",
		.stream_name = "Compress3",
		.cpu_dai_name	= "MultiMedia10",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			 SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		 /* this dai link has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA10,
	},
	{/* hw:x,28 */
		.name = "MSM8X16 Compress4",
		.stream_name = "Compress4",
		.cpu_dai_name	= "MultiMedia11",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			 SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		 /* this dai link has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA11,
	},
	{/* hw:x,29 */
		.name = "MSM8X16 Compress5",
		.stream_name = "Compress5",
		.cpu_dai_name	= "MultiMedia12",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			 SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		 /* this dai link has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA12,
	},
	{/* hw:x,30 */
		.name = "MSM8X16 Compress6",
		.stream_name = "Compress6",
		.cpu_dai_name	= "MultiMedia13",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			 SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		 /* this dai link has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA13,
	},
	{/* hw:x,31 */
		.name = "MSM8X16 Compress7",
		.stream_name = "Compress7",
		.cpu_dai_name	= "MultiMedia14",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			 SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		 /* this dai link has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA14,
	},
	{/* hw:x,32 */
		.name = "MSM8X16 Compress8",
		.stream_name = "Compress8",
		.cpu_dai_name	= "MultiMedia15",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			 SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		 /* this dai link has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA15,
	},
	{/* hw:x,33 */
		.name = "MSM8X16 Compress9",
		.stream_name = "Compress9",
		.cpu_dai_name	= "MultiMedia16",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			 SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		 /* this dai link has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA16,
	},
	{/* hw:x,34 */
		.name = "VoiceMMode1",
		.stream_name = "VoiceMMode1",
		.cpu_dai_name   = "VoiceMMode1",
		.platform_name  = "msm-pcm-voice",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_VOICEMMODE1,
	},
	{/* hw:x,35 */
		.name = "VoiceMMode2",
		.stream_name = "VoiceMMode2",
		.cpu_dai_name   = "VoiceMMode2",
		.platform_name  = "msm-pcm-voice",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_VOICEMMODE2,
	},
	{ /* hw:x,36 */
		.name = "QUAT_MI2S Hostless",
		.stream_name = "QUAT_MI2S Hostless",
		.cpu_dai_name = "QUAT_MI2S_RX_HOSTLESS",
		.platform_name = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	/* Backend I2S DAI Links */
	{
		.name = LPASS_BE_PRI_MI2S_RX,
		.stream_name = "Primary MI2S Playback",
		.cpu_dai_name = "msm-dai-q6-mi2s.0",
		.platform_name = "msm-pcm-routing",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.async_ops = ASYNC_DPCM_SND_SOC_PREPARE |
			ASYNC_DPCM_SND_SOC_HW_PARAMS,
		.be_id = MSM_BACKEND_DAI_PRI_MI2S_RX,
		.be_hw_params_fixup = msm_pri_rx_be_hw_params_fixup,
		.ops = &msm8952_mi2s_be_ops,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_SEC_MI2S_RX,
		.stream_name = "Secondary MI2S Playback",
		.cpu_dai_name = "msm-dai-q6-mi2s.1",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_SECONDARY_MI2S_RX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ops = &msm8952_sec_mi2s_be_ops,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_TERT_MI2S_TX,
		.stream_name = "Tertiary MI2S Capture",
		.cpu_dai_name = "msm-dai-q6-mi2s.2",
		.platform_name = "msm-pcm-routing",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.async_ops = ASYNC_DPCM_SND_SOC_PREPARE |
			ASYNC_DPCM_SND_SOC_HW_PARAMS,
		.be_id = MSM_BACKEND_DAI_TERTIARY_MI2S_TX,
		.be_hw_params_fixup = msm_tx_be_hw_params_fixup,
		.ops = &msm8952_mi2s_be_ops,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_QUAT_MI2S_RX,
		.stream_name = "Quaternary MI2S Playback",
		.cpu_dai_name = "msm-dai-q6-mi2s.3",
		.platform_name = "msm-pcm-routing",
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_QUATERNARY_MI2S_RX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ops = &msm8952_quat_mi2s_be_ops,
		.ignore_pmdown_time = 1, /* dai link has playback support */
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_QUAT_MI2S_TX,
		.stream_name = "Quaternary MI2S Capture",
		.cpu_dai_name = "msm-dai-q6-mi2s.3",
		.platform_name = "msm-pcm-routing",
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_QUATERNARY_MI2S_TX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ops = &msm8952_quat_mi2s_be_ops,
		.ignore_suspend = 1,
	},
	/* Primary AUX PCM Backend DAI Links */
	{
		.name = LPASS_BE_AUXPCM_RX,
		.stream_name = "AUX PCM Playback",
		.cpu_dai_name = "msm-dai-q6-auxpcm.1",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_AUXPCM_RX,
		.be_hw_params_fixup = msm_auxpcm_be_params_fixup,
		.ops = &msm_pri_auxpcm_be_ops,
		.ignore_pmdown_time = 1,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_AUXPCM_TX,
		.stream_name = "AUX PCM Capture",
		.cpu_dai_name = "msm-dai-q6-auxpcm.1",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_AUXPCM_TX,
		.be_hw_params_fixup = msm_auxpcm_be_params_fixup,
		.ops = &msm_pri_auxpcm_be_ops,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_INT_BT_SCO_RX,
		.stream_name = "Internal BT-SCO Playback",
		.cpu_dai_name = "msm-dai-q6-dev.12288",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name	= "msm-stub-rx",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_INT_BT_SCO_RX,
		.be_hw_params_fixup = msm_btsco_be_hw_params_fixup,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_INT_BT_SCO_TX,
		.stream_name = "Internal BT-SCO Capture",
		.cpu_dai_name = "msm-dai-q6-dev.12289",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name	= "msm-stub-tx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_INT_BT_SCO_TX,
		.be_hw_params_fixup = msm_btsco_be_hw_params_fixup,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_INT_FM_RX,
		.stream_name = "Internal FM Playback",
		.cpu_dai_name = "msm-dai-q6-dev.12292",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_INT_FM_RX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_INT_FM_TX,
		.stream_name = "Internal FM Capture",
		.cpu_dai_name = "msm-dai-q6-dev.12293",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_INT_FM_TX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_AFE_PCM_RX,
		.stream_name = "AFE Playback",
		.cpu_dai_name = "msm-dai-q6-dev.224",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_AFE_PCM_RX,
		.be_hw_params_fixup = msm_proxy_rx_be_hw_params_fixup,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_AFE_PCM_TX,
		.stream_name = "AFE Capture",
		.cpu_dai_name = "msm-dai-q6-dev.225",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_AFE_PCM_TX,
		.be_hw_params_fixup = msm_proxy_tx_be_hw_params_fixup,
		.ignore_suspend = 1,
	},
	/* Incall Record Uplink BACK END DAI Link */
	{
		.name = LPASS_BE_INCALL_RECORD_TX,
		.stream_name = "Voice Uplink Capture",
		.cpu_dai_name = "msm-dai-q6-dev.32772",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_INCALL_RECORD_TX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ignore_suspend = 1,
	},
	/* Incall Record Downlink BACK END DAI Link */
	{
		.name = LPASS_BE_INCALL_RECORD_RX,
		.stream_name = "Voice Downlink Capture",
		.cpu_dai_name = "msm-dai-q6-dev.32771",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_INCALL_RECORD_RX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ignore_suspend = 1,
	},
	/* Incall Music BACK END DAI Link */
	{
		.name = LPASS_BE_VOICE_PLAYBACK_TX,
		.stream_name = "Voice Farend Playback",
		.cpu_dai_name = "msm-dai-q6-dev.32773",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_VOICE_PLAYBACK_TX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ignore_suspend = 1,
	},
	/* Incall Music 2 BACK END DAI Link */
	{
		.name = LPASS_BE_VOICE2_PLAYBACK_TX,
		.stream_name = "Voice2 Farend Playback",
		.cpu_dai_name = "msm-dai-q6-dev.32770",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_VOICE2_PLAYBACK_TX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_QUIN_MI2S_TX,
		.stream_name = "Quinary MI2S Capture",
		.cpu_dai_name = "msm-dai-q6-mi2s.5",
		.platform_name = "msm-pcm-routing",
		.codec_dai_name = "rt5665-aif1_1",
		.codec_name = "rt5665",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_QUINARY_MI2S_TX,
		.be_hw_params_fixup = msm_mi2s_tx_be_hw_params_fixup,
		.ops = &msm8952_quin_mi2s_be_ops,
		.ignore_suspend = 1,
	},
#if defined(CONFIG_SND_SOC_JACK_AUDIO)
	{
		.name = "MSM8952 JACK LowLatency",
		.stream_name = "MultiMedia17",
		.cpu_dai_name	= "MultiMedia17",
		.platform_name	= "msm-pcm-dsp.1",
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.async_ops = ASYNC_DPCM_SND_SOC_PREPARE |
			ASYNC_DPCM_SND_SOC_HW_PARAMS,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA17,
	},
#else
	{/* hw:x,38 */
		.name = "MSM8X16 Compress10",
		.stream_name = "Compress10",
		.cpu_dai_name	= "MultiMedia17",
		.platform_name  = "msm-compress-dsp",
		.dynamic = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			 SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA17,
	},
#endif
	{
		.name = "Dummy MI2S Capture",
		.stream_name = "Dummy MI2S Capture",
		.cpu_dai_name = "PRI_MI2S_TX_HOSTLESS",
		.platform_name	= "msm-pcm-hostless",
		.codec_dai_name = "rt5665-aif3",
		.codec_name = "rt5665",
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ops = &msm8952_dummy_mi2s_be_ops,
		.ignore_suspend = 1,
	},
	{
		.name = "Dummy MI2S Playback",
		.stream_name = "Dummy MI2S Playback",
		.cpu_dai_name = "PRI_MI2S_RX_HOSTLESS",
		.platform_name	= "msm-pcm-hostless",
		.codec_dai_name = "rt5665-aif3",
		.codec_name = "rt5665",
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ops = &msm8952_dummy_mi2s_be_ops,
		.ignore_suspend = 1,
	},
#ifdef CONFIG_SEC_SND_ADAPTATION
	{
		.name = "ADAPTATION Hostless",
		.stream_name = "ADAPTATION Hostless",
		.cpu_dai_name = "snd-soc-dummy-dai",
		.platform_name = "q6audio-adaptation",
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
#endif /* CONFIG_SEC_SND_ADAPTATION */
};

static struct snd_soc_dai_link msm8952_hdmi_dba_dai_link[] = {
	{
		.name = LPASS_BE_QUIN_MI2S_RX,
		.stream_name = "Quinary MI2S Playback",
		.cpu_dai_name = "msm-dai-q6-mi2s.5",
		.platform_name = "msm-pcm-routing",
		.codec_dai_name = "msm_hdmi_dba_codec_rx_dai",
		.codec_name = "msm-hdmi-dba-codec-rx",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_QUINARY_MI2S_RX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ops = &msm8952_quin_mi2s_be_ops,
		.ignore_pmdown_time = 1, /* dai link has playback support */
		.ignore_suspend = 1,
	},
};

static struct snd_soc_dai_link msm8952_quin_dai_link[] = {
	{
		.name = LPASS_BE_QUIN_MI2S_RX,
		.stream_name = "Quinary MI2S Playback",
		.cpu_dai_name = "msm-dai-q6-mi2s.5",
		.platform_name = "msm-pcm-routing",
		.codec_dai_name = "rt5665-aif1_1",
		.codec_name = "rt5665",
		.init = &rt5665_audrx_init,
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_QUINARY_MI2S_RX,
		.be_hw_params_fixup = msm_mi2s_rx_be_hw_params_fixup,
		.ops = &msm8952_quin_mi2s_be_ops,
		.ignore_pmdown_time = 1, /* dai link has playback support */
		.ignore_suspend = 1,
	},
};

#if defined(CONFIG_SND_SOC_TDM_INTF)
static struct snd_soc_dai_link msm8952_tdm_fe_dai[] = {
	/* FE TDM DAI links */
	{
		.name = "Primary TDM RX 0 Hostless",
		.stream_name = "Primary TDM RX 0 Hostless",
		.cpu_dai_name = "PRI_TDM_RX_0_HOSTLESS",
		.platform_name = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{
		.name = "Primary TDM TX 0 Hostless",
		.stream_name = "Primary TDM TX 0 Hostless",
		.cpu_dai_name = "PRI_TDM_TX_0_HOSTLESS",
		.platform_name = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{
		.name = "Secondary TDM RX 0 Hostless",
		.stream_name = "Secondary TDM RX 0 Hostless",
		.cpu_dai_name = "SEC_TDM_RX_0_HOSTLESS",
		.platform_name = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_playback = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{
		.name = "Secondary TDM TX 0 Hostless",
		.stream_name = "Secondary TDM TX 0 Hostless",
		.cpu_dai_name = "SEC_TDM_TX_0_HOSTLESS",
		.platform_name = "msm-pcm-hostless",
		.dynamic = 1,
		.dpcm_capture = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
};

static struct snd_soc_dai_link msm8952_tdm_be_dai_link[] = {
	/* TDM be dai links */
	{
		.name = LPASS_BE_PRI_TDM_RX_0,
		.stream_name = "Primary TDM0 Playback",
		.cpu_dai_name = "msm-dai-q6-tdm.36864",
		.platform_name = "msm-pcm-routing",
#if defined(CONFIG_SND_SOC_TFA9896)
		.codecs = amp_tdm,
		.num_codecs = ARRAY_SIZE(amp_tdm),
#else
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
#endif
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_PRI_TDM_RX_0,
		.be_hw_params_fixup = msm_tdm_be_hw_params_fixup,
		.ops = &msm_tdm_be_ops,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_PRI_TDM_TX_0,
		.stream_name = "Primary TDM0 Capture",
		.cpu_dai_name = "msm-dai-q6-tdm.36865",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_PRI_TDM_TX_0,
		.be_hw_params_fixup = msm_tdm_be_hw_params_fixup,
		.ops = &msm_tdm_be_ops,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_SEC_TDM_RX_0,
		.stream_name = "Secondary TDM0 Playback",
		.cpu_dai_name = "msm-dai-q6-tdm.36880",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.be_id = MSM_BACKEND_DAI_SEC_TDM_RX_0,
		.be_hw_params_fixup = msm_tdm_be_hw_params_fixup,
		.ops = &msm_tdm_be_ops,
		.ignore_suspend = 1,
	},
	{
		.name = LPASS_BE_SEC_TDM_TX_0,
		.stream_name = "Secondary TDM0 Capture",
		.cpu_dai_name = "msm-dai-q6-tdm.36881",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.be_id = MSM_BACKEND_DAI_SEC_TDM_TX_0,
		.be_hw_params_fixup = msm_tdm_be_hw_params_fixup,
		.ops = &msm_tdm_be_ops,
		.ignore_suspend = 1,
	},
};
#endif

static struct snd_soc_dai_link msm8952_dai_links[
ARRAY_SIZE(msm8952_dai) +
ARRAY_SIZE(msm8952_hdmi_dba_dai_link)+
ARRAY_SIZE(msm8952_tdm_fe_dai)+
ARRAY_SIZE(msm8952_tdm_be_dai_link)];

#if defined(CONFIG_SND_SOC_TFA9896)
static struct snd_soc_codec_conf ext_amp_conf[] = {
	{  .name_prefix = "SPKUR", },
	{  .name_prefix = "SPKUL", },
	{  .name_prefix = "SPKLR", },
	{  .name_prefix = "SPKLL", },
};
#endif

static struct snd_soc_card bear_card = {
	/* snd_soc_card_msm8952 */
	.name		= "msm8952-rt5665-snd-card",
	.dai_link	= msm8952_dai,
	.num_links	= ARRAY_SIZE(msm8952_dai),
	.controls = rt5665_controls,
	.num_controls = ARRAY_SIZE(rt5665_controls),
	.dapm_widgets = rt5665_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(rt5665_dapm_widgets),
	.set_bias_level = msm8952_rt5665_set_bias_level,
};

void msm8952_rt5665_disable_mclk(struct work_struct *work)
{
	struct msm8952_asoc_mach_data *pdata = NULL;
	struct delayed_work *dwork;
	int ret = 0;

	dwork = to_delayed_work(work);
	pdata = container_of(dwork, struct msm8952_asoc_mach_data,
			disable_mclk_work);
	mutex_lock(&pdata->cdc_mclk_mutex);
	pr_debug("%s: mclk_enabled %d mclk_rsc_ref %d\n", __func__,
			atomic_read(&pdata->mclk_enabled),
			atomic_read(&pdata->mclk_rsc_ref));

	if (atomic_read(&pdata->mclk_enabled) == true
			&& atomic_read(&pdata->mclk_rsc_ref) == 0) {
		pr_debug("Disable the mclk\n");
		if (pdata->afe_clk_ver == AFE_CLK_VERSION_V1) {
			pdata->digital_cdc_clk.clk_val = 0;
			ret = afe_set_digital_codec_core_clock(
				AFE_PORT_ID_PRIMARY_MI2S_RX,
				&pdata->digital_cdc_clk);

		} else {
			pdata->digital_cdc_core_clk.enable = 0;
			ret = afe_set_lpass_clock_v2(
				AFE_PORT_ID_PRIMARY_MI2S_RX,
				&pdata->digital_cdc_core_clk);
		}
		if (ret < 0)
			pr_err("%s failed to disable the CCLK\n", __func__);
		atomic_set(&pdata->mclk_enabled, false);
	}
	mutex_unlock(&pdata->cdc_mclk_mutex);
}

static int msm8952_populate_dai_link_component_of_node(
		struct snd_soc_card *card)
{
	int i, index, ret = 0;
	struct device *cdev = card->dev;
	struct snd_soc_dai_link *dai_link = card->dai_link;
	struct device_node *phandle;

	if (!cdev) {
		pr_err("%s: Sound card device memory NULL\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < card->num_links; i++) {
		if (dai_link[i].platform_of_node && dai_link[i].cpu_of_node)
			continue;

		/* populate platform_of_node for snd card dai links */
		if (dai_link[i].platform_name &&
				!dai_link[i].platform_of_node) {
			index = of_property_match_string(cdev->of_node,
					"asoc-platform-names",
					dai_link[i].platform_name);
			if (index < 0) {
				pr_err("%s: No match found for platform name: %s\n",
					__func__, dai_link[i].platform_name);
				ret = index;
				goto cpu_dai;
			}
			phandle = of_parse_phandle(cdev->of_node,
					"asoc-platform",
					index);
			if (!phandle) {
				pr_err("%s: retrieving phandle for platform %s, index %d failed\n",
					__func__, dai_link[i].platform_name,
						index);
				ret = -ENODEV;
				goto err;
			}
			dai_link[i].platform_of_node = phandle;
			dai_link[i].platform_name = NULL;
		}
cpu_dai:
		/* populate cpu_of_node for snd card dai links */
		if (dai_link[i].cpu_dai_name && !dai_link[i].cpu_of_node) {
			index = of_property_match_string(cdev->of_node,
					"asoc-cpu-names",
					dai_link[i].cpu_dai_name);
			if (index < 0)
				goto codec_dai;
			phandle = of_parse_phandle(cdev->of_node, "asoc-cpu",
					index);
			if (!phandle) {
				pr_err("%s: retrieving phandle for cpu dai %s failed\n",
					__func__, dai_link[i].cpu_dai_name);
				ret = -ENODEV;
				goto err;
			}
			dai_link[i].cpu_of_node = phandle;
			dai_link[i].cpu_dai_name = NULL;
		}
codec_dai:
		/* populate codec_of_node for snd card dai links */
		if (dai_link[i].codec_name && !dai_link[i].codec_of_node) {
			index = of_property_match_string(cdev->of_node,
					"asoc-codec-names",
					dai_link[i].codec_name);
			if (index < 0)
				continue;
			phandle = of_parse_phandle(cdev->of_node, "asoc-codec",
					index);
			if (!phandle) {
				pr_err("%s: retrieving phandle for codec dai %s failed\n",
					__func__, dai_link[i].codec_name);
				ret = -ENODEV;
				goto err;
			}
			pr_err("%s: codec dai %s failed\n",
					__func__, dai_link[i].codec_name);
			dai_link[i].codec_of_node = phandle;
			dai_link[i].codec_name = NULL;
		}
	}
err:
	return ret;
}

static struct snd_soc_card *msm8952_populate_sndcard_dailinks(
						struct device *dev)
{
	struct snd_soc_card *card = &bear_card;
	struct snd_soc_dai_link *dailink;
	int len1;

	card->name = dev_name(dev);
	len1 = ARRAY_SIZE(msm8952_dai);
	memcpy(msm8952_dai_links, msm8952_dai, sizeof(msm8952_dai));
	dailink = msm8952_dai_links;
	if (of_property_read_bool(dev->of_node,
				"qcom,hdmi-dba-codec-rx")) {
		dev_dbg(dev, "%s(): hdmi audio support present\n",
				__func__);
		memcpy(dailink + len1, msm8952_hdmi_dba_dai_link,
				sizeof(msm8952_hdmi_dba_dai_link));
		len1 += ARRAY_SIZE(msm8952_hdmi_dba_dai_link);
	} else {
		dev_dbg(dev, "%s(): No hdmi dba present, add quin dai\n",
				__func__);
		memcpy(dailink + len1, msm8952_quin_dai_link,
				sizeof(msm8952_quin_dai_link));
		len1 += ARRAY_SIZE(msm8952_quin_dai_link);
	}
#if defined(CONFIG_SND_SOC_TDM_INTF)
	if (of_property_read_bool(dev->of_node, "qcom,tdm-audio-intf")) {
		dev_dbg(dev, "%s(): TDM support present\n",
				__func__);
		memcpy(dailink + len1,
			msm8952_tdm_fe_dai, sizeof(msm8952_tdm_fe_dai));

		len1 = len1 + ARRAY_SIZE(msm8952_tdm_fe_dai);

		memcpy(dailink + len1, msm8952_tdm_be_dai_link,
			sizeof(msm8952_tdm_be_dai_link));

		len1 = len1 + ARRAY_SIZE(msm8952_tdm_be_dai_link);
	}
#endif
	card->dai_link = dailink;
	card->num_links = len1;
	return card;
}

static int msm8952_rt5665_asoc_machine_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card;
	struct msm8952_asoc_mach_data *pdata = NULL;
	const char *ext_pa = "qcom,msm-ext-pa";
	const char *mclk = "qcom,msm-mclk-freq";
	const char *ext_pa_str = NULL;
	int num_strings;
	int ret, id, i;
#if defined(CONFIG_SND_SOC_TFA9896)
	int nxp_amp_cnt = 0;
#endif
	struct resource	*muxsel;

	if (ext_codec == true) {
		pr_err("%s: ext codec setprop is true\n", __func__);
		return -EINVAL;
	}

	pdata = devm_kzalloc(&pdev->dev,
			sizeof(struct msm8952_asoc_mach_data), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	muxsel = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"csr_gp_io_mux_mic_ctl");
	if (!muxsel) {
		dev_err(&pdev->dev, "MUX addr invalid for MI2S\n");
		ret = -ENODEV;
		goto err1;
	}
	pdata->vaddr_gpio_mux_mic_ctl =
		ioremap(muxsel->start, resource_size(muxsel));
	if (pdata->vaddr_gpio_mux_mic_ctl == NULL) {
		pr_err("%s ioremap failure for muxsel virt addr\n",
				__func__);
		ret = -ENOMEM;
		goto err1;
	}

	muxsel = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"csr_gp_io_mux_spkr_ctl");
	if (!muxsel) {
		dev_err(&pdev->dev, "MUX addr invalid for MI2S\n");
		ret = -ENODEV;
		goto err;
	}
	pdata->vaddr_gpio_mux_spkr_ctl =
		ioremap(muxsel->start, resource_size(muxsel));
	if (pdata->vaddr_gpio_mux_spkr_ctl == NULL) {
		pr_err("%s ioremap failure for muxsel virt addr\n",
				__func__);
		ret = -ENOMEM;
		goto err;
	}

	muxsel = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"csr_gp_io_mux_quin_ctl");
	if (!muxsel) {
		dev_err(&pdev->dev, "MUX addr invalid for MI2S\n");
		ret = -ENODEV;
		goto err;
	}
	pdata->vaddr_gpio_mux_quin_ctl =
		ioremap(muxsel->start, resource_size(muxsel));
	if (pdata->vaddr_gpio_mux_quin_ctl == NULL) {
		pr_err("%s ioremap failure for muxsel virt addr\n",
				__func__);
		ret = -ENOMEM;
		goto err;
	}

	muxsel = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"csr_gp_io_lpaif_pri_pcm_pri_mode_muxsel");
	if (!muxsel) {
		dev_err(&pdev->dev, "MUX addr invalid for MI2S\n");
		ret = -ENODEV;
		goto err;
	}
	pdata->vaddr_gpio_mux_pcm_ctl =
		ioremap(muxsel->start, resource_size(muxsel));
	if (pdata->vaddr_gpio_mux_pcm_ctl == NULL) {
		pr_err("%s ioremap failure for muxsel virt addr\n",
				__func__);
		ret = -ENOMEM;
		goto err;
	}

#if defined(CONFIG_SND_SOC_TDM_INTF)
	muxsel = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"csr_gp_io_lpaif_qui_pcm_sec_mode_muxsel");
	if (!muxsel) {
		dev_err(&pdev->dev, "MUX addr invalid for QUIN PCM\n");
		ret = -ENODEV;
	} else {
		pdata->vaddr_gpio_mux_qui_pcm_sec_mode_ctl =
			ioremap(muxsel->start, resource_size(muxsel));
		if (pdata->vaddr_gpio_mux_qui_pcm_sec_mode_ctl == NULL) {
			pr_err("%s ioremap failure for muxsel virt addr\n",
				__func__);
			ret = -ENOMEM;
			goto err;
		}
	}

	muxsel = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"csr_gp_io_mux_mic_ext_clk_ctl");
	if (!muxsel) {
		dev_err(&pdev->dev, "MUX addr invalid for EXT CLK CTL\n");
		ret = -ENODEV;
	} else {
		pdata->vaddr_gpio_mux_mic_ext_clk_ctl =
			ioremap(muxsel->start, resource_size(muxsel));
		if (pdata->vaddr_gpio_mux_mic_ext_clk_ctl == NULL) {
			pr_err("%s ioremap failure for muxsel virt addr\n",
				__func__);
			ret = -ENOMEM;
			goto err;
		}
	}

	muxsel = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"csr_gp_io_mux_sec_tlmm_ctl");
	if (!muxsel) {
		dev_err(&pdev->dev, "MUX addr invalid for SEC TLMM CTL\n");
		ret = -ENODEV;
	} else {
		pdata->vaddr_gpio_mux_sec_tlmm_ctl =
			ioremap(muxsel->start, resource_size(muxsel));
		if (pdata->vaddr_gpio_mux_sec_tlmm_ctl == NULL) {
			pr_err("%s ioremap failure for muxsel virt addr\n",
				__func__);
			ret = -ENOMEM;
			goto err;
		}
	}
#endif

	ret = of_property_read_u32(pdev->dev.of_node, mclk, &id);
	if (ret) {
		dev_err(&pdev->dev,
				"%s: missing %s in dt node\n", __func__, mclk);
		id = DEFAULT_MCLK_RATE;
	}
	pdata->mclk_freq = id;

	/*reading the gpio configurations from dtsi file*/
	ret = msm_gpioset_initialize(CLIENT_WCD_INT, &pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"%s: error reading dtsi files%d\n", __func__, ret);
		goto err;
	}

	ret = msm_gpioset_activate(CLIENT_WCD_INT, "quin_i2s");
	if (ret < 0) {
		pr_err("%s: gpio set cannot be de-activated %sd",
			__func__, "quin_i2s");
		goto err;
	}

	ret = msm_gpioset_suspend(CLIENT_WCD_INT, "quin_i2s");
	if (ret < 0) {
		pr_err("%s: gpio set cannot be de-suspended %sd",
			__func__, "quin_i2s");
		goto err;
	}

	card = msm8952_populate_sndcard_dailinks(&pdev->dev);
	dev_info(&pdev->dev, "default codec configured\n");
	num_strings = of_property_count_strings(pdev->dev.of_node,
			ext_pa);
	if (num_strings < 0) {
		dev_err(&pdev->dev,
				"%s: missing %s in dt node or length is incorrect\n",
				__func__, ext_pa);
		goto err;
	}
	for (i = 0; i < num_strings; i++) {
		ret = of_property_read_string_index(pdev->dev.of_node,
				ext_pa, i, &ext_pa_str);
		if (ret) {
			dev_err(&pdev->dev, "%s:of read string %s i %d error %d\n",
					__func__, ext_pa, i, ret);
			goto err;
		}
		if (!strcmp(ext_pa_str, "primary"))
			pdata->ext_pa = (pdata->ext_pa | PRI_MI2S_ID);
		else if (!strcmp(ext_pa_str, "secondary"))
			pdata->ext_pa = (pdata->ext_pa | SEC_MI2S_ID);
		else if (!strcmp(ext_pa_str, "tertiary"))
			pdata->ext_pa = (pdata->ext_pa | TER_MI2S_ID);
		else if (!strcmp(ext_pa_str, "quaternary"))
			pdata->ext_pa = (pdata->ext_pa | QUAT_MI2S_ID);
		else if (!strcmp(ext_pa_str, "quinary"))
			pdata->ext_pa = (pdata->ext_pa | QUIN_MI2S_ID);
	}
	pr_debug("%s: ext_pa = %d\n", __func__, pdata->ext_pa);

	/* initialize the digital codec core clk */
	pdata->digital_cdc_core_clk.clk_set_minor_version =
			AFE_API_VERSION_I2S_CONFIG;
	pdata->digital_cdc_core_clk.clk_id =
			Q6AFE_LPASS_CLK_ID_INTERNAL_DIGITAL_CODEC_CORE;
	pdata->digital_cdc_core_clk.clk_freq_in_hz =
			pdata->mclk_freq;
	pdata->digital_cdc_core_clk.clk_attri =
			Q6AFE_LPASS_CLK_ATTRIBUTE_COUPLE_NO;
	pdata->digital_cdc_core_clk.clk_root =
			Q6AFE_LPASS_CLK_ROOT_DEFAULT;
	pdata->digital_cdc_core_clk.enable = 1;

	card->dev = &pdev->dev;

#if defined(CONFIG_SND_SOC_TFA9896)
	nxp_amp_cnt = of_count_phandle_with_args(pdev->dev.of_node, "qcom,nxp-amps", NULL);
	for (i = 0; i < nxp_amp_cnt; i++)
		ext_amp_conf[i].of_node = of_parse_phandle(pdev->dev.of_node, "qcom,nxp-amps", i);
	if (nxp_amp_cnt) {
		card->codec_conf = ext_amp_conf;
		card->num_configs = i;
	}
#endif

	platform_set_drvdata(pdev, card);
	snd_soc_card_set_drvdata(card, pdata);
	ret = snd_soc_of_parse_card_name(card, "qcom,model");
	if (ret)
		goto err;
	/* initialize timer */
	INIT_DELAYED_WORK(&pdata->disable_mclk_work,
			msm8952_rt5665_disable_mclk);
	mutex_init(&pdata->cdc_mclk_mutex);
	atomic_set(&pdata->mclk_rsc_ref, 0);
	atomic_set(&pdata->mclk_enabled, false);
	atomic_set(&quat_mi2s_clk_ref, 0);
	atomic_set(&quin_mi2s_clk_ref, 0);
	atomic_set(&auxpcm_mi2s_clk_ref, 0);
	ret = snd_soc_of_parse_audio_routing(card,
			"qcom,audio-routing");
	if (ret)
		goto err;

	ret = msm8952_populate_dai_link_component_of_node(card);
	if (ret) {
		ret = -EPROBE_DEFER;
		goto err;
	}

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
			ret);
		goto err;
	}
	dev_info(&pdev->dev, "Sound card %s registered\n", card->name);

	rt5665_sysclk.fll_in = pdata->mclk_freq;

	if (!of_find_property(pdev->dev.of_node, "clock-names", NULL)) {
		dev_dbg(&pdev->dev, "%s: codec not using audio-ext-clk driver\n",
			__func__);
	} else {
		dev_dbg(&pdev->dev, "%s: codec using audio-ext-clk driver\n",
			__func__);
		pdata->rt5665_ext_clk = clk_get(card->dev, "ext-mclk");
		if (IS_ERR(pdata->rt5665_ext_clk)) {
			dev_err(&pdev->dev, "%s: clk get %s failed\n",
			__func__, "pdata->rt5665_ext_clk");
		}
	}

	pdata->codec_irq_gpio = of_get_named_gpio(pdev->dev.of_node,
					"qcom,codec_irq_n", 0);

	if (gpio_is_valid(pdata->codec_irq_gpio)) {
		if (gpio_request(pdata->codec_irq_gpio, "codec_irq_n"))
			dev_err(&pdev->dev, "Failed to request codec_irq_n\n");
		else if (gpio_direction_input(pdata->codec_irq_gpio))
			dev_err(&pdev->dev, "Failed to set codec_irq_n\n");
	}

	return 0;
err:
	if (pdata->vaddr_gpio_mux_spkr_ctl)
		iounmap(pdata->vaddr_gpio_mux_spkr_ctl);
	if (pdata->vaddr_gpio_mux_mic_ctl)
		iounmap(pdata->vaddr_gpio_mux_mic_ctl);
	if (pdata->vaddr_gpio_mux_pcm_ctl)
		iounmap(pdata->vaddr_gpio_mux_pcm_ctl);
	if (pdata->vaddr_gpio_mux_quin_ctl)
		iounmap(pdata->vaddr_gpio_mux_quin_ctl);
#if defined(CONFIG_SND_SOC_TDM_INTF)
	if (pdata->vaddr_gpio_mux_qui_pcm_sec_mode_ctl)
		iounmap(pdata->vaddr_gpio_mux_qui_pcm_sec_mode_ctl);
	if (pdata->vaddr_gpio_mux_mic_ext_clk_ctl)
		iounmap(pdata->vaddr_gpio_mux_mic_ext_clk_ctl);
	if (pdata->vaddr_gpio_mux_sec_tlmm_ctl)
		iounmap(pdata->vaddr_gpio_mux_sec_tlmm_ctl);
#endif
err1:
	devm_kfree(&pdev->dev, pdata);
#ifdef CONFIG_SEC_SND_DEBUG
	if(ret != -EPROBE_DEFER) {		
		pr_err("%s: err case ret = %d\n", __func__, ret);
		panic("msm8952_rt5665_asoc_machine_probe fail");
	}
#endif
	return ret;
}

static int msm8952_rt5665_asoc_machine_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct msm8952_asoc_mach_data *pdata = snd_soc_card_get_drvdata(card);

	if (gpio_is_valid(pdata->codec_irq_gpio))
		gpio_free(pdata->codec_irq_gpio);

	if (pdata->vaddr_gpio_mux_spkr_ctl)
		iounmap(pdata->vaddr_gpio_mux_spkr_ctl);
	if (pdata->vaddr_gpio_mux_mic_ctl)
		iounmap(pdata->vaddr_gpio_mux_mic_ctl);
	if (pdata->vaddr_gpio_mux_pcm_ctl)
		iounmap(pdata->vaddr_gpio_mux_pcm_ctl);
	if (pdata->vaddr_gpio_mux_quin_ctl)
		iounmap(pdata->vaddr_gpio_mux_quin_ctl);
#if defined(CONFIG_SND_SOC_TDM_INTF)
	if (pdata->vaddr_gpio_mux_qui_pcm_sec_mode_ctl)
		iounmap(pdata->vaddr_gpio_mux_qui_pcm_sec_mode_ctl);
	if (pdata->vaddr_gpio_mux_mic_ext_clk_ctl)
		iounmap(pdata->vaddr_gpio_mux_mic_ext_clk_ctl);
	if (pdata->vaddr_gpio_mux_sec_tlmm_ctl)
		iounmap(pdata->vaddr_gpio_mux_sec_tlmm_ctl);
#endif
	snd_soc_unregister_card(card);
	mutex_destroy(&pdata->cdc_mclk_mutex);
	return 0;
}

static const struct of_device_id msm8952_rt5665_asoc_machine_of_match[]  = {
	{ .compatible = "qcom,msm8952-rt5665-audio-codec", },
	{},
};

static struct platform_driver msm8952_rt5665_asoc_machine_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = msm8952_rt5665_asoc_machine_of_match,
	},
	.probe = msm8952_rt5665_asoc_machine_probe,
	.remove = msm8952_rt5665_asoc_machine_remove,
};

static int __init msm8952_rt5665_machine_init(void)
{
	return platform_driver_register(&msm8952_rt5665_asoc_machine_driver);
}
late_initcall(msm8952_rt5665_machine_init);

static void __exit msm8952_rt5665_machine_exit(void)
{
	return platform_driver_unregister(&msm8952_rt5665_asoc_machine_driver);
}
module_exit(msm8952_rt5665_machine_exit);

static ssize_t codec_detect_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	int boot = 0;

	if (sscanf(buf, "%du", &boot) == 1) {
		if (boot == 1)
			ext_codec = true;
	}
	return count;
}

static struct kobj_attribute codec_detect_attribute =
	__ATTR(boot, 0220, NULL, codec_detect_store);

static struct attribute *attrs[] = {
	&codec_detect_attribute.attr,
	NULL,
};

static int codec_detector_init_sysfs(void)
{
	int ret = -EINVAL;
	struct kobject *boot_cdc_det = NULL;
	struct attribute_group *attr_group;

	attr_group = kzalloc(sizeof(*(attr_group)),
				GFP_KERNEL);
	if (!attr_group) {
		ret = -ENOMEM;
		goto error_return;
	}

	attr_group->attrs = attrs;

	boot_cdc_det = kobject_create_and_add("codec_type", kernel_kobj);
	if (!boot_cdc_det) {
		pr_err("%s: sysfs create and add failed\n",
						__func__);
		ret = -ENOMEM;
		goto error_return;
	}

	ret = sysfs_create_group(boot_cdc_det, attr_group);
	if (ret) {
		pr_err("%s: sysfs create group failed %d\n",
							__func__, ret);
		goto error_return;
	}

	return 0;

error_return:
	kfree(attr_group);
	if (boot_cdc_det) {
		kobject_del(boot_cdc_det);
		boot_cdc_det = NULL;
	}

	return ret;
}

static int __init codec_detector_init(void)
{
	int ret = codec_detector_init_sysfs();

	if (ret != 0) {
		pr_err("%s: Error in initing sysfs\n", __func__);
		return ret;
	}
	return 0;
}
module_init(codec_detector_init);

MODULE_DESCRIPTION("ALSA SoC msm");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DEVICE_TABLE(of, msm8952_rt5665_asoc_machine_of_match);
