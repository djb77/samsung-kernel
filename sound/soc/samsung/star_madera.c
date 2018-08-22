/*
 *  Sound Machine Driver for Madera CODECs on Star
 *
 *  Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/debugfs.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/mfd/madera/core.h>
#include <linux/extcon/extcon-madera.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/wakelock.h>

#include <soc/samsung/exynos-pmu.h>
#include <sound/samsung/abox.h>
#include <sound/samsung/vts.h>

#include <linux/mfd/madera/core.h>
#include <linux/extcon/extcon-madera.h>
#include "../codecs/madera.h"

#include "jack_madera_sysfs_cb.h"

#define EXYNOS_PMU_PMU_DEBUG_OFFSET		(0x0A00)
#define MADERA_DAI_OFFSET			(16)

#define SYSCLK_RATE_CHECK_PARAM 8000
#define SYSCLK_RATE_48KHZ   98304000
#define SYSCLK_RATE_44_1KHZ 90316800

#define GPIO_AUX_FN_1 0x2280
#define GPIO_AUX_FN_2 0x1000
#define GPIO_AUX_FN_3 0x2281
#define GPIO_AUX_FN_4 0x1000

#define GPIO_HIZ_FN_1 0x2001
#define GPIO_HIZ_FN_2 0x9000

#define GPIO_AUXPDM_MASK_1 0x0fff
#define GPIO_AUXPDM_MASK_2 0xf000

/* Used for debugging and test automation */
static u32 voice_trigger_count;

struct clk_conf {
	int id;
	const char *name;
	int source;
	int rate;

	bool valid;
};

struct madera_drvdata {
	struct device *dev;
	struct snd_soc_codec *codec;

	struct clk_conf fll1_refclk;
	struct clk_conf fll2_refclk;
	struct clk_conf sysclk;
	struct clk_conf asyncclk;
	struct clk_conf dspclk;
	struct clk_conf outclk;

	struct notifier_block nb;
	unsigned int hp_impedance_step;
	bool hiz_val;
	int fll2_switch;

	struct wake_lock wake_lock;
	int wake_lock_switch;

	int fm_mute_switch;
};

struct gain_table {
	int min; /* min impedance */
	int max; /* max impedance */
	unsigned int gain; /* offset value added to current register val */
};

static struct impedance_table {
	struct gain_table hp_gain_table[8];
	char imp_region[4];	/* impedance region */
} imp_table = {
	.hp_gain_table = {
		{    0,      13, 0 },
		{   14,      25, 5 },
		{   26,      42, 7 },
		{   43,     100, 14 },
		{  101,     200, 18 },
		{  201,     450, 20 },
		{  451,    1000, 20 },
		{ 1001, INT_MAX, 16 },
	},
};

static struct madera_drvdata star_drvdata;
static struct snd_soc_card star_madera;

static struct clk *xclkout;

static const struct snd_soc_ops rdma_ops = {
};

static const struct snd_soc_ops wdma_ops = {
};

static int star_uaif0_madera_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct madera_drvdata *drvdata = card->drvdata;
	unsigned int rate = params_rate(params);
	int ret = 0;

	dev_info(card->dev, "%s\n", __func__);
	dev_info(card->dev, "%s-%d %dch, %dHz, %dbytes, %dbit\n",
			rtd->dai_link->name, substream->stream,
			params_channels(params), params_rate(params),
			params_buffer_bytes(params), params_width(params));

	drvdata->fll2_refclk.rate = rate * params_width(params) * 2;

	if (rate % SYSCLK_RATE_CHECK_PARAM)
		drvdata->asyncclk.rate = SYSCLK_RATE_44_1KHZ;
	else
		drvdata->asyncclk.rate = SYSCLK_RATE_48KHZ;

	return ret;
}

static int star_uaif0_madera_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct madera_drvdata *drvdata = card->drvdata;
	struct snd_soc_codec *codec;
	int ret = 0;

	dev_info(card->dev, "%s-%d prepare\n",
			rtd->dai_link->name, substream->stream);

	/* enable bclk from UAIF0 */
	snd_soc_dai_set_tristate(rtd->cpu_dai, 0);

	codec = rtd->codec_dai->codec;

	ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
				       drvdata->asyncclk.source,
				       drvdata->asyncclk.rate,
				       SND_SOC_CLOCK_IN);
	if (ret < 0)
		dev_err(card->dev, "Failed to set ASYNCCLK to FLL2: %d\n", ret);

	ret = snd_soc_codec_set_pll(codec, drvdata->fll2_refclk.id,
					drvdata->fll2_refclk.source,
					drvdata->fll2_refclk.rate,
					drvdata->asyncclk.rate);
	if (ret < 0)
		dev_err(card->dev,
				"Failed to start ASYNCCLK FLL REF: %d\n", ret);

	return ret;
}

static void star_uaif0_madera_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct madera_drvdata *drvdata = card->drvdata;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec;
	int ret;

	codec = rtd->codec_dai->codec;

	dev_info(card->dev, "%s\n", __func__);
	dev_info(card->dev, "codec_dai: %s\n", codec_dai->name);
	dev_info(card->dev, "%s-%d playback: %d, capture: %d, active: %d\n",
			rtd->dai_link->name, substream->stream,
			codec_dai->playback_active, codec_dai->capture_active,
			codec->component.active);

	if (!codec_dai->playback_active && !codec_dai->capture_active &&
	    !codec->component.active) {
		snd_soc_update_bits(codec, MADERA_OUTPUT_ENABLES_1,
				MADERA_OUT2L_ENA | MADERA_OUT2R_ENA, 0);
		msleep(1);

		ret = snd_soc_codec_set_sysclk(codec,
						drvdata->asyncclk.id,
						0, 0, 0);
		if (ret != 0)
			dev_err(drvdata->dev,
					"Failed to set ASYNCCLK: %d\n", ret);

		ret = snd_soc_codec_set_pll(codec,
						drvdata->fll2_refclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev,
				"Failed to stop ASYNCCLK FLL REF: %d\n", ret);
	}

	snd_soc_dai_set_tristate(rtd->cpu_dai, 1);
}

static const struct snd_soc_ops uaif0_madera_ops = {
	.hw_params = star_uaif0_madera_hw_params,
	.prepare = star_uaif0_madera_prepare,
	.shutdown = star_uaif0_madera_shutdown,
};

static int star_uaif2_madera_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct madera_drvdata *drvdata = card->drvdata;
	int ret = 0;

	dev_info(drvdata->dev, "%s\n", __func__);
	dev_info(drvdata->dev, "%s-%d %dch, %dHz, %dbytes, %dbit\n",
			rtd->dai_link->name, substream->stream,
			params_channels(params), params_rate(params),
			params_buffer_bytes(params), params_width(params));

	return ret;
}

static int star_uaif2_madera_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct madera_drvdata *drvdata = card->drvdata;
	int ret = 0;

	dev_info(drvdata->dev, "%s-%d prepare\n",
			rtd->dai_link->name, substream->stream);

	return ret;
}

static void star_uaif2_madera_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct madera_drvdata *drvdata = card->drvdata;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_codec *codec;

	codec = rtd->codec_dai->codec;

	dev_info(drvdata->dev, "%s\n", __func__);
	dev_info(drvdata->dev, "codec_dai: %s\n", codec_dai->name);
	dev_info(drvdata->dev, "%s-%d playback: %d, capture: %d, active: %d\n",
			rtd->dai_link->name, substream->stream,
			codec_dai->playback_active, codec_dai->capture_active,
			codec->component.active);
}

static const struct snd_soc_ops uaif2_madera_ops = {
	.hw_params = star_uaif2_madera_hw_params,
	.prepare = star_uaif2_madera_prepare,
	.shutdown = star_uaif2_madera_shutdown,
};

static const struct snd_soc_ops uaif_ops = {
};

static int dsif_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *hw_params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int tx_slot[] = {0, 1};

	/* bclk ratio 64 for DSD64, 128 for DSD128 */
	snd_soc_dai_set_bclk_ratio(cpu_dai, 64);

	/* channel map 0 1 if left is first, 1 0 if right is first */
	snd_soc_dai_set_channel_map(cpu_dai, 2, tx_slot, 0, NULL);
	return 0;
}

static const struct snd_soc_ops dsif_ops = {
	.hw_params = dsif_hw_params,
};

static int star_set_bias_level(struct snd_soc_card *card,
				  struct snd_soc_dapm_context *dapm,
				  enum snd_soc_bias_level level)
{
	struct snd_soc_pcm_runtime *rtd;
	struct snd_soc_dai *codec_dai;
	struct madera_drvdata *drvdata = card->drvdata;
	struct snd_soc_codec *codec;
	int ret;

	rtd = snd_soc_get_pcm_runtime(card,
			card->dai_link[MADERA_DAI_OFFSET].name);
	codec_dai = rtd->codec_dai;
	codec = rtd->codec_dai->codec;

	if (dapm->dev != codec_dai->dev)
		return 0;

	switch (level) {
	case SND_SOC_BIAS_STANDBY:
		if (dapm->bias_level == SND_SOC_BIAS_OFF) {
			dev_info(card->dev, "%s: set SYSCLK\n", __func__);
			clk_enable(xclkout);

			ret = snd_soc_codec_set_sysclk(codec,
						drvdata->sysclk.id,
						drvdata->sysclk.source,
						drvdata->sysclk.rate,
						SND_SOC_CLOCK_IN);
			if (ret < 0)
				dev_err(card->dev,
					"Failed to set SYSCLK to FLL1: %d\n",
					ret);

			ret = snd_soc_codec_set_pll(codec,
						drvdata->fll1_refclk.id,
						drvdata->fll1_refclk.source,
						drvdata->fll1_refclk.rate,
						drvdata->sysclk.rate);
			if (ret < 0)
				dev_err(card->dev,
					"Failed to start SYSCLK FLL REF: %d\n",
					ret);
		}
		break;
	case SND_SOC_BIAS_OFF:
		clk_disable(xclkout);

		ret = snd_soc_codec_set_sysclk(codec,
						drvdata->sysclk.id,
						0, 0, 0);
		if (ret != 0)
			dev_err(drvdata->dev,
					"Failed to set SYSCLK: %d\n", ret);

		ret = snd_soc_codec_set_pll(codec,
						drvdata->fll1_refclk.id,
						0, 0, 0);
		if (ret < 0)
			dev_err(card->dev,
				"Failed to stop SYSCLK FLL REF: %d\n", ret);
		break;
	default:
		break;
	}

	dev_dbg(card->dev, "%s: %d\n", __func__, level);

	return 0;
}

static int star_set_bias_level_post(struct snd_soc_card *card,
				       struct snd_soc_dapm_context *dapm,
				       enum snd_soc_bias_level level)
{
	dev_dbg(card->dev, "%s: %d\n", __func__, level);

	return 0;
}

void star_madera_hpdet_cb(unsigned int meas)
{
	struct snd_soc_card *card = &star_madera;
	int jack_det;
	struct madera_drvdata *drvdata = card->drvdata;
	int i, num_hp_gain_table;

	if (meas == (INT_MAX / 100))
		jack_det = 0;
	else
		jack_det = 1;

	madera_jack_det = jack_det;

	dev_info(card->dev, "%s(%d) meas(%d)\n", __func__, jack_det, meas);

	num_hp_gain_table = (int) ARRAY_SIZE(imp_table.hp_gain_table);
	for (i = 0; i < num_hp_gain_table; i++) {
		if (meas < imp_table.hp_gain_table[i].min
				|| meas > imp_table.hp_gain_table[i].max)
			continue;

		dev_info(card->dev, "SET GAIN %d step for %d ohms\n",
			 imp_table.hp_gain_table[i].gain, meas);
		drvdata->hp_impedance_step = imp_table.hp_gain_table[i].gain;
	}
}

void star_madera_micd_cb(bool mic)
{
	struct snd_soc_card *card = &star_madera;

	madera_ear_mic = mic;
	dev_info(card->dev, "%s: madera_ear_mic = %d\n",
			__func__, madera_ear_mic);
}

static int madera_notify(struct notifier_block *nb,
				   unsigned long event, void *data)
{
	const struct madera_hpdet_notify_data *hp_inf;
	const struct madera_micdet_notify_data *md_inf;
	const struct madera_voice_trigger_info *vt_inf;
	const struct madera_drvdata *drvdata =
		container_of(nb, struct madera_drvdata, nb);

	switch (event) {
	case MADERA_NOTIFY_VOICE_TRIGGER:
		vt_inf = data;
		dev_info(drvdata->dev, "Voice Triggered (core_num=%d)\n",
			 vt_inf->core_num);
		++voice_trigger_count;
		break;
	case MADERA_NOTIFY_HPDET:
		hp_inf = data;
		star_madera_hpdet_cb(hp_inf->impedance_x100 / 100);
		dev_info(drvdata->dev, "HPDET val=%d.%02d ohms\n",
			 hp_inf->impedance_x100 / 100,
			 hp_inf->impedance_x100 % 100);
		break;
	case MADERA_NOTIFY_MICDET:
		md_inf = data;
		star_madera_micd_cb(md_inf->present);
		dev_info(drvdata->dev, "MICDET present=%c val=%d.%02d ohms\n",
			 md_inf->present ? 'Y' : 'N',
			 md_inf->impedance_x100 / 100,
			 md_inf->impedance_x100 % 100);
		break;
	default:
		dev_info(drvdata->dev, "notifier event=0x%lx data=0x%p\n",
			 event, data);
		break;
	}

	return NOTIFY_DONE;
}

static int madera_put_impedance_volsw(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct madera_drvdata *drvdata = card->drvdata;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	int max = mc->max;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;
	int err;
	unsigned int val, val_mask;

	val = (unsigned int) (ucontrol->value.integer.value[0] & mask);
	val += drvdata->hp_impedance_step;
	dev_info(drvdata->dev,
			 "SET GAIN %d according to impedance, moved %d step\n",
			 val, drvdata->hp_impedance_step);

	if (invert)
		val = max - val;
	val_mask = mask << shift;
	val = val << shift;

	err = snd_soc_update_bits(codec, reg, val_mask, val);
	if (err < 0) {
		dev_err(drvdata->dev,
				"impedance volume update err %d\n", err);
		return err;
	}

	return err;
}

static int get_auxpdm_hiz_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct madera_drvdata *drvdata = codec->component.card->drvdata;

	ucontrol->value.integer.value[0] = drvdata->hiz_val;

	return 0;
}

static int set_auxpdm_hiz_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct madera_drvdata *drvdata = codec->component.card->drvdata;

	drvdata->hiz_val = ucontrol->value.integer.value[0];

	dev_info(card->dev, "%s ev: %d\n", __func__, drvdata->hiz_val);

	if (drvdata->hiz_val) {
		snd_soc_update_bits(codec, MADERA_GPIO10_CTRL_1,
				GPIO_AUXPDM_MASK_1,
				GPIO_AUX_FN_1);
		snd_soc_update_bits(codec, MADERA_GPIO10_CTRL_2,
				GPIO_AUXPDM_MASK_2,
				GPIO_AUX_FN_2);
		snd_soc_update_bits(codec, MADERA_GPIO9_CTRL_1,
				GPIO_AUXPDM_MASK_1,
				GPIO_AUX_FN_3);
		snd_soc_update_bits(codec, MADERA_GPIO9_CTRL_2,
				GPIO_AUXPDM_MASK_2,
				GPIO_AUX_FN_4);
	} else {
		snd_soc_update_bits(codec, MADERA_GPIO10_CTRL_1,
				GPIO_AUXPDM_MASK_1,
				GPIO_HIZ_FN_1);
		snd_soc_update_bits(codec, MADERA_GPIO10_CTRL_2,
				GPIO_AUXPDM_MASK_2,
				GPIO_HIZ_FN_2);
		snd_soc_update_bits(codec, MADERA_GPIO9_CTRL_1,
				GPIO_AUXPDM_MASK_1,
				GPIO_HIZ_FN_1);
		snd_soc_update_bits(codec, MADERA_GPIO9_CTRL_2,
				GPIO_AUXPDM_MASK_2,
				GPIO_HIZ_FN_2);
	}

	return 0;
}

static int get_fll2_switch(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct madera_drvdata *drvdata = card->drvdata;

	ucontrol->value.integer.value[0] = drvdata->fll2_switch;

	return 0;
}

static int set_fll2_switch(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct madera_drvdata *drvdata = card->drvdata;
	int ret = 0;

	drvdata->fll2_switch = ucontrol->value.integer.value[0];

	if (drvdata->fll2_switch) {
		clk_enable(xclkout);

		/* asyncclk id : 2, asyncclk source 5, asyncclk rate 98304000 */
		ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
				drvdata->asyncclk.source,
				drvdata->asyncclk.rate,
				SND_SOC_CLOCK_IN);
		if (ret < 0)
			dev_err(card->dev, "%s: Failed to set ASYNCCLK to FLL2: %d\n",
					__func__, ret);

		/* fll2_refclk id : 2 */
		ret = snd_soc_codec_set_pll(codec, drvdata->fll2_refclk.id,
				MADERA_FLL_SRC_MCLK2,
				32768,
				drvdata->asyncclk.rate);
		if (ret < 0)
			dev_err(card->dev, "%s: Failed to start ASYNCCLK FLL REF: %d\n",
					__func__, ret);
	} else {
		clk_disable(xclkout);

		/* asyncclk id : 2, asyncclk source 5, asyncclk rate 98304000 */
		ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
				0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "%s: Failed to set ASYNCCLK to FLL2: %d\n",
					__func__, ret);

		/* fll2_refclk id : 2 */
		ret = snd_soc_codec_set_pll(codec, drvdata->fll2_refclk.id,
				0, 0, 0);
		if (ret < 0)
			dev_err(card->dev, "%s: Failed to start ASYNCCLK FLL REF: %d\n",
					__func__, ret);
	}

	return ret;
}

static int get_sound_wakelock(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct madera_drvdata *drvdata = codec->component.card->drvdata;

	ucontrol->value.integer.value[0] = drvdata->wake_lock_switch;

	return 0;
}

static int set_sound_wakelock(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct madera_drvdata *drvdata = codec->component.card->drvdata;
	struct snd_soc_card *card = codec->component.card;

	drvdata->wake_lock_switch = ucontrol->value.integer.value[0];

	dev_info(card->dev, "%s: lock %d\n", __func__, drvdata->wake_lock_switch);

	if (drvdata->wake_lock_switch) {
		wake_lock(&drvdata->wake_lock);
	} else {
		wake_unlock(&drvdata->wake_lock);
	}

	return 0;
}

static int get_fm_mute_switch(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct madera_drvdata *drvdata = card->drvdata;

	ucontrol->value.integer.value[0] = drvdata->fm_mute_switch;

	return 0;
}

static int set_fm_mute_switch(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct snd_soc_card *card = codec->component.card;
	struct madera_drvdata *drvdata = card->drvdata;
	int ret = 0;
	unsigned int fm_in_reg = MADERA_ADC_DIGITAL_VOLUME_2L;
	int i;

	drvdata->fm_mute_switch = ucontrol->value.integer.value[0];

	dev_info(card->dev, "%s: mute %d\n", __func__, drvdata->fm_mute_switch);

#ifdef CONFIG_SEC_FACTORY
	return ret;
#endif

	for (i = 0; i < 2; i++) {
		ret = snd_soc_update_bits(codec, fm_in_reg + (i * 4),
				MADERA_IN2L_MUTE_MASK,
				drvdata->fm_mute_switch << MADERA_IN2L_MUTE_SHIFT);

		if (ret < 0) {
			dev_err(card->dev, "Failed to update registers 0x%x\n", fm_in_reg + (i * 4));
		}
	}

	return ret;
}

static const struct snd_kcontrol_new madera_codec_controls[] = {
	SOC_SINGLE_EXT_TLV("HPOUT2L Impedance Volume",
		MADERA_DAC_DIGITAL_VOLUME_2L,
		MADERA_OUT2L_VOL_SHIFT, 0xbf, 0,
		snd_soc_get_volsw, madera_put_impedance_volsw,
		madera_digital_tlv),

	SOC_SINGLE_EXT_TLV("HPOUT2R Impedance Volume",
		MADERA_DAC_DIGITAL_VOLUME_2R,
		MADERA_OUT2L_VOL_SHIFT, 0xbf, 0,
		snd_soc_get_volsw, madera_put_impedance_volsw,
		madera_digital_tlv),

	SOC_SINGLE_BOOL_EXT("AUXPDM Switch",
		0, get_auxpdm_hiz_mode, set_auxpdm_hiz_mode),

	SOC_SINGLE_BOOL_EXT("FLL2 Switch",
		0, get_fll2_switch, set_fll2_switch),

	SOC_SINGLE_BOOL_EXT("Sound Wakelock",
		0, get_sound_wakelock, set_sound_wakelock),

	SOC_SINGLE_BOOL_EXT("FM Mute Switch",
		0, get_fm_mute_switch, set_fm_mute_switch),
};

static int star_uaif0_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct madera_drvdata *drvdata = snd_soc_card_get_drvdata(card);
	int ret = 0;

	dev_info(drvdata->dev, "%s\n", __func__);

	drvdata->codec = codec;

	ret = snd_soc_add_codec_controls(codec, madera_codec_controls,
					ARRAY_SIZE(madera_codec_controls));
	if (ret < 0) {
		dev_err(drvdata->dev,
			"Failed to add controls to codec: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->sysclk.id,
				       drvdata->sysclk.source,
				       drvdata->sysclk.rate,
				       SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(drvdata->dev, "Failed to set SYSCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->asyncclk.id,
				       drvdata->asyncclk.source,
				       drvdata->asyncclk.rate,
				       SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(drvdata->dev, "Failed to set ASYNCCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->dspclk.id,
				       drvdata->dspclk.source,
				       drvdata->dspclk.rate,
				       SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(drvdata->dev, "Failed to set DSPCLK: %d\n", ret);
		return ret;
	}

	ret = snd_soc_codec_set_sysclk(codec, drvdata->outclk.id,
				       drvdata->outclk.source,
				       drvdata->outclk.rate,
				       SND_SOC_CLOCK_IN);
	if (ret != 0) {
		dev_err(drvdata->dev, "Failed to set OUTCLK to FLL2: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai, drvdata->asyncclk.id, 0, 0);
	if (ret != 0) {
		dev_err(drvdata->dev, "Failed to set AIF1 clock: %d\n", ret);
		return ret;
	}

	snd_soc_dapm_ignore_suspend(snd_soc_codec_get_dapm(codec), "AIF1 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_codec_get_dapm(codec), "AIF1 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_codec_get_dapm(codec), "AUXPDM1");
	snd_soc_dapm_sync(snd_soc_codec_get_dapm(codec));

	drvdata->nb.notifier_call = madera_notify;
	madera_register_notifier(codec, &drvdata->nb);

	register_madera_jack_cb(codec);


	return 0;
}

static int star_uaif2_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct madera_drvdata *drvdata = snd_soc_card_get_drvdata(card);
	int ret = 0;

	dev_info(drvdata->dev, "%s\n", __func__);

	ret = snd_soc_dai_set_sysclk(codec_dai, drvdata->sysclk.id, 0, 0);
	if (ret != 0) {
		dev_err(drvdata->dev, "Failed to set AIF3 clock: %d\n", ret);
		return ret;
	}

	snd_soc_dapm_ignore_suspend(snd_soc_codec_get_dapm(codec), "AIF3 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_codec_get_dapm(codec), "AIF3 Capture");
	snd_soc_dapm_sync(snd_soc_codec_get_dapm(codec));

	return 0;
}

static int star_late_probe(struct snd_soc_card *card)
{
	struct madera_drvdata *drvdata = card->drvdata;
	struct snd_soc_pcm_runtime *rtd;
	struct snd_soc_component *cpu;

	dev_info(drvdata->dev, "%s\n", __func__);

	rtd = snd_soc_get_pcm_runtime(card, card->dai_link[0].name);
	cpu = rtd->cpu_dai->component;

	snd_soc_dapm_ignore_suspend(&card->dapm, "VOUTPUT");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VINPUT1");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VINPUT2");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VOUTPUTCALL");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VINPUTCALL");
	snd_soc_dapm_ignore_suspend(&card->dapm, "HEADSETMIC");
	snd_soc_dapm_ignore_suspend(&card->dapm, "RECEIVER");
	snd_soc_dapm_ignore_suspend(&card->dapm, "HEADPHONE");
	snd_soc_dapm_ignore_suspend(&card->dapm, "SPEAKER");
	snd_soc_dapm_ignore_suspend(&card->dapm, "BLUETOOTH MIC");
	snd_soc_dapm_ignore_suspend(&card->dapm, "BLUETOOTH SPK");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC1");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC2");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC3");
	snd_soc_dapm_ignore_suspend(&card->dapm, "DMIC4");
	snd_soc_dapm_ignore_suspend(&card->dapm, "VTS Virtual Output");
	snd_soc_dapm_ignore_suspend(&card->dapm, "FM");
	snd_soc_dapm_sync(&card->dapm);

	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA0 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA1 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA2 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA3 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA4 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA5 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA6 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX RDMA7 Playback");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA0 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA1 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA2 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA3 Capture");
	snd_soc_dapm_ignore_suspend(snd_soc_component_get_dapm(cpu), "ABOX WDMA4 Capture");
	snd_soc_dapm_sync(snd_soc_component_get_dapm(cpu));

	wake_lock_init(&drvdata->wake_lock, WAKE_LOCK_SUSPEND,
				"madera-sound");
	drvdata->wake_lock_switch = 0;

	return 0;
}

static struct snd_soc_dai_link star_dai[] = {
	{
		.name = "RDMA0",
		.stream_name = "RDMA0",
		.cpu_dai_name = "RDMA0",
		.platform_name = "17c51000.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA1",
		.stream_name = "RDMA1",
		.cpu_dai_name = "RDMA1",
		.platform_name = "17c51100.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA2",
		.stream_name = "RDMA2",
		.cpu_dai_name = "RDMA2",
		.platform_name = "17c51200.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA3",
		.stream_name = "RDMA3",
		.cpu_dai_name = "RDMA3",
		.platform_name = "17c51300.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA4",
		.stream_name = "RDMA4",
		.cpu_dai_name = "RDMA4",
		.platform_name = "17c51400.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA5",
		.stream_name = "RDMA5",
		.cpu_dai_name = "RDMA5",
		.platform_name = "17c51500.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA6",
		.stream_name = "RDMA6",
		.cpu_dai_name = "RDMA6",
		.platform_name = "17c51600.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "RDMA7",
		.stream_name = "RDMA7",
		.cpu_dai_name = "RDMA7",
		.platform_name = "17c51700.abox_rdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &rdma_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "WDMA0",
		.stream_name = "WDMA0",
		.cpu_dai_name = "WDMA0",
		.platform_name = "17c52000.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "WDMA1",
		.stream_name = "WDMA1",
		.cpu_dai_name = "WDMA1",
		.platform_name = "17c52100.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "WDMA2",
		.stream_name = "WDMA2",
		.cpu_dai_name = "WDMA2",
		.platform_name = "17c52200.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "WDMA3",
		.stream_name = "WDMA3",
		.cpu_dai_name = "WDMA3",
		.platform_name = "17c52300.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "WDMA4",
		.stream_name = "WDMA4",
		.cpu_dai_name = "WDMA4",
		.platform_name = "17c52400.abox_wdma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dynamic = 1,
		.ignore_suspend = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST_PRE, SND_SOC_DPCM_TRIGGER_PRE_POST},
		.ops = &wdma_ops,
		.dpcm_capture = 1,
	},
	{
		.name = "VTS-Trigger",
		.stream_name = "VTS-Trigger",
		.cpu_dai_name = "vts-tri",
		.platform_name = "vts_dma0",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.capture_only = true,
	},
	{
		.name = "VTS-Record",
		.stream_name = "VTS-Record",
		.cpu_dai_name = "vts-rec",
		.platform_name = "vts_dma1",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.capture_only = true,
	},
	{
		.name = "DP Audio",
		.stream_name = "DP Audio",
		.cpu_dai_name = "audio_cpu_dummy",
		.platform_name = "dp_dma",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
	},
	{
		.name = "UAIF0",
		.stream_name = "UAIF0",
		.cpu_dai_name = "UAIF0",
		.platform_name = "snd-soc-dummy",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.init = star_uaif0_init,
		.ops = &uaif0_madera_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "UAIF1",
		.stream_name = "UAIF1",
		.cpu_dai_name = "UAIF1",
		.platform_name = "snd-soc-dummy",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "UAIF2",
		.stream_name = "UAIF2",
		.cpu_dai_name = "UAIF2",
		.platform_name = "snd-soc-dummy",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.init = star_uaif2_init,
		.ops = &uaif2_madera_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "UAIF3",
		.stream_name = "UAIF3",
		.cpu_dai_name = "UAIF3",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &uaif_ops,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "DSIF",
		.stream_name = "DSIF",
		.cpu_dai_name = "DSIF",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_PDM | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.ops = &dsif_ops,
		.dpcm_playback = 1,
	},
	{
		.name = "SIFS0",
		.stream_name = "SIFS0",
		.cpu_dai_name = "SIFS0",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "SIFS1",
		.stream_name = "SIFS1",
		.cpu_dai_name = "SIFS1",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "SIFS2",
		.stream_name = "SIFS2",
		.cpu_dai_name = "SIFS2",
		.platform_name = "snd-soc-dummy",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
		.no_pcm = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = abox_hw_params_fixup_helper,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{ /* pcm dump interface */
		.name = "CPU-DSP trace",
		.stream_name = "CPU-DSP trace",
		.cpu_dai_name = "cs47l92-cpu-trace",
		.platform_name = "cs47l92-codec",
		.codec_dai_name = "cs47l92-dsp-trace",
		.codec_name = "cs47l92-codec",
	},
};

static int star_dmic1(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int star_dmic2(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int star_dmic3(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int star_dmic4(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int star_headsetmic(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d ear_mic %d\n", __func__, event, madera_ear_mic);

	return 0;
}

static int star_fm(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int star_receiver(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int star_headphone(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d jack_det %d\n", __func__, event, madera_jack_det);

	return 0;
}

static int star_speaker(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int star_btsco_tx(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int star_btsco_rx(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_card *card = w->dapm->card;

	dev_info(card->dev, "%s ev: %d\n", __func__, event);

	return 0;
}

static int star_suspend_post(struct snd_soc_card *card)
{
	struct madera_drvdata *drvdata = card->drvdata;
	int ret = 0;

	dev_info(drvdata->dev, "%s component active %d earmic %d vts %d\n", __func__,
			drvdata->codec->component.active, madera_ear_mic,
			vts_is_recognitionrunning());
	if (!drvdata->codec->component.active) {
		if (madera_ear_mic) {
			if (!vts_is_recognitionrunning())
				madera_enable_force_bypass(drvdata->codec);
		} else if (vts_is_recognitionrunning()) {
			madera_enable_force_bypass(drvdata->codec);

			ret = snd_soc_codec_set_sysclk(drvdata->codec,
							drvdata->sysclk.id,
							0, 0, 0);
			if (ret != 0)
				dev_err(drvdata->dev,
					"Failed to set SYSCLK: %d\n", ret);

			ret = snd_soc_codec_set_pll(drvdata->codec,
							drvdata->fll1_refclk.id,
							0, 0, 0);
			if (ret < 0)
				dev_err(card->dev,
					"Failed to stop SYSCLK FLL REF: %d\n",
					ret);
		}
	}

	return ret;
}

static int star_resume_pre(struct snd_soc_card *card)
{
	struct madera_drvdata *drvdata = card->drvdata;
	int ret = 0;

	dev_info(drvdata->dev, "%s component active %d earmic %d vts %d\n", __func__,
			drvdata->codec->component.active, madera_ear_mic,
			vts_is_recognitionrunning());
	if (!drvdata->codec->component.active) {
		if (madera_ear_mic) {
			if (!vts_is_recognitionrunning())
				madera_disable_force_bypass(drvdata->codec);
		} else if (vts_is_recognitionrunning()) {
			ret = snd_soc_codec_set_sysclk(drvdata->codec,
						drvdata->sysclk.id,
						drvdata->sysclk.source,
						drvdata->sysclk.rate,
						SND_SOC_CLOCK_IN);
			if (ret < 0)
				dev_err(card->dev,
					"Failed to set SYSCLK to FLL1: %d\n",
					ret);

			ret = snd_soc_codec_set_pll(drvdata->codec,
						drvdata->fll1_refclk.id,
						drvdata->fll1_refclk.source,
						drvdata->fll1_refclk.rate,
						drvdata->sysclk.rate);
			if (ret < 0)
				dev_err(card->dev,
					"Failed to start SYSCLK FLL REF: %d\n",
					ret);

			madera_disable_force_bypass(drvdata->codec);
		}
	}

	return ret;
}

static const char * const vts_output_texts[] = {
	"None", "DMIC1",
};

static const struct soc_enum vts_output_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(vts_output_texts),
			vts_output_texts);


static const struct snd_kcontrol_new vts_output_mux[] = {
	SOC_DAPM_ENUM("VTS Virtual Output Mux", vts_output_enum),
};

static const struct snd_kcontrol_new star_controls[] = {
	SOC_DAPM_PIN_SWITCH("DMIC1"),
	SOC_DAPM_PIN_SWITCH("DMIC2"),
	SOC_DAPM_PIN_SWITCH("DMIC3"),
	SOC_DAPM_PIN_SWITCH("DMIC4"),
	SOC_DAPM_PIN_SWITCH("FM"),
};

static struct snd_soc_dapm_widget star_widgets[] = {
	SND_SOC_DAPM_OUTPUT("VOUTPUT"),
	SND_SOC_DAPM_INPUT("VINPUT1"),
	SND_SOC_DAPM_INPUT("VINPUT2"),
	SND_SOC_DAPM_OUTPUT("VOUTPUTCALL"),
	SND_SOC_DAPM_INPUT("VINPUTCALL"),
	SND_SOC_DAPM_MIC("DMIC1", star_dmic1),
	SND_SOC_DAPM_MIC("DMIC2", star_dmic2),
	SND_SOC_DAPM_MIC("DMIC3", star_dmic3),
	SND_SOC_DAPM_MIC("DMIC4", star_dmic4),
	SND_SOC_DAPM_MIC("HEADSETMIC", star_headsetmic),
	SND_SOC_DAPM_MIC("FM", star_fm),
	SND_SOC_DAPM_SPK("RECEIVER", star_receiver),
	SND_SOC_DAPM_HP("HEADPHONE", star_headphone),
	SND_SOC_DAPM_SPK("SPEAKER", star_speaker),
	SND_SOC_DAPM_MIC("BLUETOOTH MIC", star_btsco_tx),
	SND_SOC_DAPM_SPK("BLUETOOTH SPK", star_btsco_rx),
	SND_SOC_DAPM_OUTPUT("VTS Virtual Output"),
	SND_SOC_DAPM_MUX("VTS Virtual Output Mux", SND_SOC_NOPM, 0, 0,
			 &vts_output_mux[0]),
};

static struct snd_soc_codec_conf codec_conf[] = {
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "ABOX", },
	{.name_prefix = "VTS", },
};

static struct snd_soc_aux_dev aux_dev[] = {
	{
		.name = "EFFECT",
	},
};

static struct snd_soc_card star_madera = {
	.name = "Star-Madera",
	.owner = THIS_MODULE,
	.dai_link = star_dai,
	.num_links = ARRAY_SIZE(star_dai),

	.late_probe = star_late_probe,

	.controls = star_controls,
	.num_controls = ARRAY_SIZE(star_controls),
	.dapm_widgets = star_widgets,
	.num_dapm_widgets = ARRAY_SIZE(star_widgets),

	.set_bias_level = star_set_bias_level,
	.set_bias_level_post = star_set_bias_level_post,

	.drvdata = (void *)&star_drvdata,

	.codec_conf = codec_conf,
	.num_configs = ARRAY_SIZE(codec_conf),

	.aux_dev = aux_dev,
	.num_aux_devs = ARRAY_SIZE(aux_dev),

	.suspend_post = star_suspend_post,
	.resume_pre = star_resume_pre,
};

static int read_clk_conf(struct device_node *np,
				   const char * const prop,
				   struct clk_conf *conf)
{
	u32 tmp;
	int ret;

	/*Truncate "cirrus," from prop_name to fetch clk_name*/
	conf->name = &prop[7];

	ret = of_property_read_u32_index(np, prop, 0, &tmp);
	if (ret)
		return ret;

	conf->id = tmp;

	ret = of_property_read_u32_index(np, prop, 1, &tmp);
	if (ret)
		return ret;

	if (tmp < 0xffff)
		conf->source = tmp;
	else
		conf->source = -1;

	ret = of_property_read_u32_index(np, prop, 2, &tmp);
	if (ret)
		return ret;

	conf->rate = tmp;
	conf->valid = true;

	return 0;
}

static int read_dai(struct device_node *np, const char * const prop,
			      struct device_node **dai, const char **name)
{
	int ret = 0;

	np = of_get_child_by_name(np, prop);
	if (!np)
		return -ENOENT;

	*dai = of_parse_phandle(np, "sound-dai", 0);
	if (!*dai) {
		ret = -ENODEV;
		goto out;
	}

	if (*name == NULL) {
		/* Ignoring the return as we don't register DAIs to the platform */
		ret = snd_soc_of_get_dai_name(np, name);
		if (ret && !*name)
			return ret;
	}

out:
	of_node_put(np);

	return ret;
}

static int star_audio_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &star_madera;
	struct madera_drvdata *drvdata = card->drvdata;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *dai;
	int nlink = 0;
	long n;
	int ret;

	card->dev = &pdev->dev;
	drvdata->dev = card->dev;

	xclkout = devm_clk_get(&pdev->dev, "xclkout");
	if (IS_ERR(xclkout)) {
		dev_err(&pdev->dev, "xclkout get failed\n");
		xclkout = NULL;
	}

	ret = clk_prepare(xclkout);
	if (ret < 0) {
		dev_err(card->dev, "xclkout prepare failed\n");
		goto err_clk_get;
	}

	ret = read_clk_conf(np, "cirrus,sysclk", &drvdata->sysclk);
	if (ret) {
		dev_err(card->dev, "Failed to parse sysclk: %d\n", ret);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,asyncclk", &drvdata->asyncclk);
	if (ret)
		dev_info(card->dev, "Failed to parse asyncclk: %d\n", ret);

	ret = read_clk_conf(np, "cirrus,dspclk", &drvdata->dspclk);
	if (ret) {
		dev_info(card->dev, "Failed to parse dspclk: %d\n", ret);
	} else if (drvdata->dspclk.source == drvdata->sysclk.source &&
		   drvdata->dspclk.rate / 3 != drvdata->sysclk.rate / 2) {
		/* DSPCLK & SYSCLK, if sharing a source, should be an
		 * appropriate ratio of one another, or both be zero (which
		 * signifies "automatic" mode).
		 */
		dev_err(card->dev, "DSPCLK & SYSCLK share src but request incompatible frequencies: %d vs %d\n",
			drvdata->dspclk.rate, drvdata->sysclk.rate);
		goto err_clk_prepare;
	}

	ret = read_clk_conf(np, "cirrus,fll1-refclk", &drvdata->fll1_refclk);
	if (ret)
		dev_info(card->dev, "Failed to parse fll1-refclk: %d\n", ret);

	ret = read_clk_conf(np, "cirrus,fll2-refclk", &drvdata->fll2_refclk);
	if (ret)
		dev_info(card->dev, "Failed to parse fll2-refclk: %d\n", ret);

	ret = read_clk_conf(np, "cirrus,outclk", &drvdata->outclk);
	if (ret)
		dev_info(card->dev, "Failed to parse outclk: %d\n", ret);

	for_each_child_of_node(np, dai) {
		if (!star_dai[nlink].name)
			star_dai[nlink].name = dai->name;
		if (!star_dai[nlink].stream_name)
			star_dai[nlink].stream_name = dai->name;

		ret = read_dai(dai, "cpu",
			&star_dai[nlink].cpu_of_node,
			&star_dai[nlink].cpu_dai_name);
		if (ret) {
			dev_err(card->dev,
				"Failed to parse cpu DAI for %s: %d\n",
				dai->name, ret);
			goto err_clk_prepare;
		}

		if (!star_dai[nlink].platform_name) {
			ret = read_dai(dai, "platform",
				&star_dai[nlink].platform_of_node,
				&star_dai[nlink].platform_name);
			if (ret) {
				star_dai[nlink].platform_of_node =
					star_dai[nlink].cpu_of_node;
				dev_info(card->dev,
					"Cpu node is used as platform for %s: %d\n",
					dai->name, ret);
			}
		}

		if (!star_dai[nlink].codec_name) {
			ret = read_dai(dai, "codec",
				&star_dai[nlink].codec_of_node,
				&star_dai[nlink].codec_dai_name);
			if (ret) {
				dev_err(card->dev,
					"Failed to parse codec DAI for %s: %d\n",
					dai->name, ret);
				goto err_clk_prepare;
			}
		}

		if (++nlink == card->num_links)
			break;
	}

	if (!nlink) {
		dev_warn(card->dev, "No DAIs specified\n");
	}

	if (of_property_read_bool(np, "samsung,routing")) {
		ret = snd_soc_of_parse_audio_routing(card, "samsung,routing");
		if (ret)
			goto err_clk_prepare;
	}

	for (n = 0; n < ARRAY_SIZE(codec_conf); n++) {
		codec_conf[n].of_node = of_parse_phandle(np, "samsung,codec", n);

		if (!codec_conf[n].of_node) {
			dev_err(&pdev->dev,
				"Property 'samsung,codec' missing\n");
			ret = -EINVAL;
			goto err_clk_prepare;
		}
	}

	for (n = 0; n < ARRAY_SIZE(aux_dev); n++) {
		aux_dev[n].codec_of_node = of_parse_phandle(np, "samsung,aux", n);

		if (!aux_dev[n].codec_of_node) {
			dev_err(&pdev->dev,
				"Property 'samsung,aux' missing\n");
			ret = -EINVAL;
			goto err_clk_prepare;
		}
	}

	ret = devm_snd_soc_register_card(card->dev, card);
	if (ret) {
		dev_err(card->dev, "snd_soc_register_card() failed:%d\n", ret);
		goto err_clk_prepare;
	}

	return ret;

err_clk_prepare:
	clk_unprepare(xclkout);
err_clk_get:
	devm_clk_put(&pdev->dev, xclkout);

	return ret;
}

static int star_audio_remove(struct platform_device *pdev)
{
	clk_unprepare(xclkout);
	devm_clk_put(&pdev->dev, xclkout);

	return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id star_of_match[] = {
	{ .compatible = "samsung,star-madera", },
	{},
};
MODULE_DEVICE_TABLE(of, star_of_match);
#endif /* CONFIG_OF */

static struct platform_driver star_audio_driver = {
	.driver		= {
		.name	= "star-madera",
		.owner	= THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = of_match_ptr(star_of_match),
	},

	.probe		= star_audio_probe,
	.remove		= star_audio_remove,
};

module_platform_driver(star_audio_driver);

MODULE_DESCRIPTION("ALSA SoC Star Madera Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:star-madera");
