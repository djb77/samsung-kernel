/*
 * Copyright (c) 2014 Samsung Electronics Co. Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/initval.h>
#include <sound/tlv.h>
//#include <sound/exynos_regmap_fw.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
//#include <linux/switch.h>
#include <linux/input.h>
#include <linux/completion.h>

//#include <sound/exynos-audmixer.h>
//#include <sound/cod3033x.h>
#include "cod3033x.h"

#define COD3033X_SAMPLE_RATE_48KHZ	48000
#define COD3033X_SAMPLE_RATE_192KHZ	192000

#define COD3033X_RESTORE_OTP_COUNT	5
#define COD3033X_RESTORE_REG_COUNT	16
#define COD3033X_OTP_R_OFFSET		0x0

#define COD3033X_MAX_IRQ_CHK_BITS	5
#define COD3033X_START_IRQ_CHK_BIT	2
#define COD3033X_MJ_DET_INVALID		(-1)

#ifdef CONFIG_SND_SOC_SAMSUNG_VERBOSE_DEBUG
#ifdef dev_dbg
#undef dev_dbg
#endif
#define dev_dbg dev_err
#endif

struct cod3033x_priv *g_cod3026x;

/* Forward Declarations */
static void cod3033x_save_otp_registers(struct snd_soc_codec *codec);
static void cod3033x_restore_otp_registers(struct snd_soc_codec *codec);
static void cod3033x_reset_io_selector_bits(struct snd_soc_codec *codec);
static int cod3033x_disable(struct device *dev);
static int cod3033x_enable(struct device *dev);

static inline void cod3033x_usleep(unsigned int u_sec)
{
	usleep_range(u_sec, u_sec + 10);
}

/**
 * Helper functions to read ADC value for button detection
 */


/**
 * Return value:
 * true: if the register value cannot be cached, hence we have to read from the
 * hardware directly
 * false: if the register value can be read from cache
 */
static bool cod3033x_volatile_register(struct device *dev, unsigned int reg)
{
	/**
	 * For all the registers for which we want to restore the value during
	 * regcache_sync operation, we need to return true here. For registers
	 * whose value need not be cached and restored should return false here.
	 *
	 * For the time being, let us cache the value of all registers other
	 * than the IRQ pending and IRQ status registers.
	 */
		return false;
}

/**
 * Return value:
 * true: if the register value can be read
 * flase: if the register cannot be read
 */
static bool cod3033x_readable_register(struct device *dev, unsigned int reg)
{
	return true;
}

static bool cod3033x_writeable_register(struct device *dev, unsigned int reg)
{
	return true;
}

const struct regmap_config cod3033x_regmap = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = 0xff,
	.readable_reg = cod3033x_readable_register,
	.writeable_reg = cod3033x_writeable_register,
	.volatile_reg = cod3033x_volatile_register,

	.use_single_rw = true,
	.cache_type = REGCACHE_RBTREE,
};

/**
 * TLV_DB_SCALE_ITEM
 *
 * (TLV: Threshold Limit Value)
 *
 * For various properties, the dB values don't change linearly with respect to
 * the digital value of related bit-field. At most, they are quasi-linear,
 * that means they are linear for various ranges of digital values. Following
 * table define such ranges of various properties.
 *
 * TLV_DB_RANGE_HEAD(num)
 * num defines the number of linear ranges of dB values.
 *
 * s0, e0, TLV_DB_SCALE_ITEM(min, step, mute),
 * s0: digital start value of this range (inclusive)
 * e0: digital end valeu of this range (inclusive)
 * min: dB value corresponding to s0
 * step: the delta of dB value in this range
 * mute: ?
 *
 * Example:
 *	TLV_DB_RANGE_HEAD(3),
 *	0, 1, TLV_DB_SCALE_ITEM(-2000, 2000, 0),
 *	2, 4, TLV_DB_SCALE_ITEM(1000, 1000, 0),
 *	5, 6, TLV_DB_SCALE_ITEM(3800, 8000, 0),
 *
 * The above code has 3 linear ranges with following digital-dB mapping.
 * (0...6) -> (-2000dB, 0dB, 1000dB, 2000dB, 3000dB, 3800dB, 4600dB),
 *
 * DECLARE_TLV_DB_SCALE
 *
 * This macro is used in case where there is a linear mapping between
 * the digital value and dB value.
 *
 * DECLARE_TLV_DB_SCALE(name, min, step, mute)
 *
 * name: name of this dB scale
 * min: minimum dB value corresponding to digital 0
 * step: the delta of dB value
 * mute: ?
 *
 * NOTE: The information is mostly for user-space consumption, to be viewed
 * alongwith amixer.
 */

/**
 * cod3033x_ctvol_bst_tlv
 *
 * Map: (0x0, 0dB), (0x1, 12dB), (0x2, 20dB)
 *
 * CTVOL_BST1, reg(0x20), shift(5), width(2)
 * CTVOL_BST2, reg(0x21), shift(5), width(2)
 * CTVOL_BST3, reg(0x22), shift(5), width(2)
 */
static const unsigned int cod3033x_ctvol_bst_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 1200, 0),
	2, 2, TLV_DB_SCALE_ITEM(2000, 0, 0),
};

/**
 * cod3033x_ctvol_bst_pga_tlv
 *
 * Range: -16.5dB to +18dB, step 1.5dB
 *
 * CTVOL_BST_PGA1, reg(0x20), shift(0), width(5), invert(1), max(31)
 * CTVOL_BST_PGA2, reg(0x21), shift(0), width(5), invert(1), max(31)
 * CTVOL_BST_PGA3, reg(0x22), shift(0), width(5), invert(1), max(31)
 */
static const DECLARE_TLV_DB_SCALE(cod3033x_ctvol_bst_pga_tlv, -1650, 150, 0);

/**
 * cod3033x_ctvol_hp_tlv
 *
 * Range: -57dB to +6dB, step 1dB
 *
 * CTVOL_HPL, reg(0x30), shift(0), width(6), invert(1), max(63)
 * CTVOL_HPR, reg(0x31), shift(0), width(6), invert(1), max(63)
 */
static const DECLARE_TLV_DB_SCALE(cod3033x_ctvol_hp_tlv, -5700, 100, 0);

/**
 * cod3019_ctvol_ep_tlv
 *
 * Range: 0dB to +12dB, step 1dB
 *
 * CTVOL_EP, reg(0x32), shift(4), width(4), invert(0), max(12)
 */
static const DECLARE_TLV_DB_SCALE(cod3033x_ctvol_ep_tlv, 0, 100, 0);

/**
 * cod3033x_ctvol_spk_pga_tlv
 *
 * Range: -6dB to +3dB, step 1dB
 *
 * CTVOL_SPK_PGA, reg(0x32), shift(0), width(4), invert(0), max(9)
 */
static const DECLARE_TLV_DB_SCALE(cod3033x_ctvol_spk_pga_tlv, -600, 100, 0);

/**
 * cod3033x_dvol_adc_tlv
 *
 * Map as per data-sheet:
 * (0x00 to 0x86) -> (+12dB to -55dB, step 0.5dB)
 * (0x87 to 0x91) -> (-56dB to -66dB, step 1dB)
 * (0x92 to 0x94) -> (-68dB to -72dB, step 2dB)
 * (0x95 to 0x96) -> (-78dB to -84dB, step 6dB)
 *
 * When the map is in descending order, we need to set the invert bit
 * and arrange the map in ascending order. The offsets are calculated as
 * (max - offset).
 *
 * offset_in_table = max - offset_actual;
 *
 * DVOL_ADL, reg(0x43), shift(0), width(8), invert(1), max(0x96)
 * DVOL_ADR, reg(0x44), shift(0), width(8), invert(1), max(0x96)
 * DVOL_DAL, reg(0x51), shift(0), width(8), invert(1), max(0x96)
 * DVOL_DAR, reg(0x52), shift(0), width(8), invert(1), max(0x96)
 */
static const unsigned int cod3033x_dvol_tlv[] = {
	TLV_DB_RANGE_HEAD(4),
	0x00, 0x01, TLV_DB_SCALE_ITEM(-8400, 600, 0),
	0x02, 0x04, TLV_DB_SCALE_ITEM(-7200, 200, 0),
	0x05, 0x09, TLV_DB_SCALE_ITEM(-6600, 100, 0),
	0x10, 0x96, TLV_DB_SCALE_ITEM(-5500, 50, 0),
};

/**
 * cod3033x_dnc_min_gain_tlv
 *
 * Range: -6dB to 0dB, step 1dB
 *
 * DNC_MINGAIN , reg(0x55), shift(5), width(3)
 */
static const unsigned int cod3033x_dnc_min_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	0x00, 0x06, TLV_DB_SCALE_ITEM(-600, 0, 0),
};

/**
 * cod3033x_dnc_max_gain_tlv
 *
 * Range: 0dB to 24dB, step 1dB
 *
 * DNC_MAXGAIN , reg(0x55), shift(0), width(5)
 */
static const unsigned int cod3033x_dnc_max_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	0x06, 0x1e, TLV_DB_SCALE_ITEM(0, 2400, 0),
};

/**
 * cod3033x_dnc_lvl_tlv
 *
 * Range: -10.5dB to 0dB, step 1.5dB
 *
 * DNCLVL_R/L, reg(0x55), shift(0/4), width(3), invert(0), max(7)
 */
static const DECLARE_TLV_DB_SCALE(cod3033x_dnc_lvl_tlv, -1050, 0, 0);


/**
 * struct snd_kcontrol_new cod3033x_snd_control
 *
 * Every distinct bit-fields within the CODEC SFR range may be considered
 * as a control elements. Such control elements are defined here.
 *
 * Depending on the access mode of these registers, different macros are
 * used to define these control elements.
 *
 * SOC_ENUM: 1-to-1 mapping between bit-field value and provided text
 * SOC_SINGLE: Single register, value is a number
 * SOC_SINGLE_TLV: Single register, value corresponds to a TLV scale
 * SOC_SINGLE_TLV_EXT: Above + custom get/set operation for this value
 * SOC_SINGLE_RANGE_TLV: Register value is an offset from minimum value
 * SOC_DOUBLE: Two bit-fields are updated in a single register
 * SOC_DOUBLE_R: Two bit-fields in 2 different registers are updated
 */

/**
 * All the data goes into cod3033x_snd_controls.
 * All path inter-connections goes into cod3033x_dapm_routes
 */
static const struct snd_kcontrol_new cod3033x_snd_controls[] = {

};
/*
	SOC_SINGLE_TLV("MIC1 Boost Volume", COD3033X_20_VOL_AD1,
			VOLAD1_CTVOL_BST1_SHIFT,
			(BIT(VOLAD1_CTVOL_BST1_WIDTH) - 1), 0,
			cod3033x_ctvol_bst_tlv),

	SOC_SINGLE_TLV("MIC1 Volume", COD3033X_20_VOL_AD1,
			VOLAD1_CTVOL_BST_PGA1_SHIFT,
			(BIT(VOLAD1_CTVOL_BST_PGA1_WIDTH) - 1), 1,
			cod3033x_ctvol_bst_pga_tlv),

	SOC_SINGLE_TLV("MIC2 Boost Volume", COD3033X_21_VOL_AD2,
			VOLAD2_CTVOL_BST2_SHIFT,
			(BIT(VOLAD2_CTVOL_BST2_WIDTH) - 1), 0,
			cod3033x_ctvol_bst_tlv),

	SOC_SINGLE_TLV("MIC2 Volume", COD3033X_21_VOL_AD2,
			VOLAD2_CTVOL_BST_PGA2_SHIFT,
			(BIT(VOLAD2_CTVOL_BST_PGA2_WIDTH) - 1), 1,
			cod3033x_ctvol_bst_pga_tlv),

	SOC_SINGLE_TLV("MIC3 Boost Volume", COD3033X_22_VOL_AD3,
			VOLAD3_CTVOL_BST3_SHIFT,
			(BIT(VOLAD3_CTVOL_BST3_WIDTH) - 1), 0,
			cod3033x_ctvol_bst_tlv),

	SOC_SINGLE_TLV("MIC3 Volume", COD3033X_22_VOL_AD3,
			VOLAD3_CTVOL_BST_PGA3_SHIFT,
			(BIT(VOLAD3_CTVOL_BST_PGA3_WIDTH) - 1), 1,
			cod3033x_ctvol_bst_pga_tlv),

	SOC_DOUBLE_R_TLV("Headphone Volume", COD3033X_30_VOL_HPL,
			COD3033X_31_VOL_HPR, VOLHP_CTVOL_HP_SHIFT,
			(BIT(VOLHP_CTVOL_HP_WIDTH) - 1), 1,
			cod3033x_ctvol_hp_tlv),

	SOC_SINGLE_TLV("Earphone Volume", COD3033X_32_VOL_EP_SPK,
			CTVOL_EP_SHIFT,
			(BIT(CTVOL_EP_WIDTH) - 1), 0,
			cod3033x_ctvol_ep_tlv),

	SOC_SINGLE_TLV("Speaker Volume", COD3033X_32_VOL_EP_SPK,
			CTVOL_SPK_PGA_SHIFT,
			(BIT(CTVOL_SPK_PGA_WIDTH) - 1), 0,
			cod3033x_ctvol_spk_pga_tlv),

	SOC_SINGLE_TLV("ADC Left Gain", COD3033X_43_ADC_L_VOL,
			AD_DA_DVOL_SHIFT,
			AD_DA_DVOL_MAXNUM, 1, cod3033x_dvol_tlv),

	SOC_SINGLE_TLV("ADC Right Gain", COD3033X_44_ADC_R_VOL,
			AD_DA_DVOL_SHIFT,
			AD_DA_DVOL_MAXNUM, 1, cod3033x_dvol_tlv),

	SOC_DOUBLE_R_TLV("DAC Gain", COD3033X_51_DAC_L_VOL,
			COD3033X_52_DAC_R_VOL, AD_DA_DVOL_SHIFT,
			AD_DA_DVOL_MAXNUM, 1, cod3033x_dvol_tlv),

	SOC_SINGLE_TLV("DNC Min Gain", COD3033X_55_DNC2,
			DNC_MIN_GAIN_SHIFT,
			(BIT(DNC_MIN_GAIN_WIDTH) - 2), 0,
			cod3033x_dnc_min_gain_tlv),

	SOC_SINGLE_TLV("DNC Max Gain", COD3033X_55_DNC2,
			DNC_MAX_GAIN_SHIFT,
			(BIT(DNC_MAX_GAIN_WIDTH) - 2), 0,
			cod3033x_dnc_max_gain_tlv),

	SOC_DOUBLE_R_TLV("DNC Level", COD3033X_56_DNC3,
			DNC_LVL_L_SHIFT, DNC_LVL_R_SHIFT,
			(BIT(DNC_LVL_L_WIDTH) - 1), 0, cod3033x_dnc_lvl_tlv),

	SOC_SINGLE("DNC ZCD Timeout", COD3033X_5C_DNC9,
			DNC_ZCD_TIMEOUT_SHIFT, DNC_ZCD_TIMEOUT_MASK, 0),

	SOC_ENUM("DNC ZCD Enable", cod3033x_dnc_zcd_enable_enum),

	SOC_ENUM("MonoMix Mode", cod3033x_mono_mix_mode_enum),

	SOC_ENUM("Chargepump Mode", cod3033x_chargepump_mode_enum),

	SOC_SINGLE_EXT("DAC Soft Mute",SND_SOC_NOPM, 0, 100, 0,
			dac_soft_mute_get, dac_soft_mute_put),
};
*/
static int dac_ev(struct snd_soc_dapm_widget *w, struct snd_kcontrol *kcontrol,
		int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s called\n", __func__);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:

		break;

	case SND_SOC_DAPM_PRE_PMD:

		break;

	default:
		break;
	}

	return 0;
}


static int adc_ev(struct snd_soc_dapm_widget *w, struct snd_kcontrol *kcontrol,
		int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		break;

	case SND_SOC_DAPM_POST_PMU:
		break;

	case SND_SOC_DAPM_PRE_PMD:
		break;

	default:
		break;
	}

	return 0;
}


static int cod3033_power_on_mic1(struct snd_soc_codec *codec)
{

	dev_dbg(codec->dev, "%s called\n", __func__);

	snd_soc_write(codec, 0x70, 0xbf);
	snd_soc_write(codec, 0x60, 0x13);
	snd_soc_write(codec, 0xb6, 0x03);
	snd_soc_write(codec, 0x62, 0x11);
	snd_soc_write(codec, 0x8a, 0x30);
	snd_soc_write(codec, 0x62, 0x11);
	snd_soc_write(codec, 0x75, 0x10);
	snd_soc_write(codec, 0x76, 0x00);
	snd_soc_write(codec, 0x60, 0x17);
	snd_soc_write(codec, 0x70, 0x3f);
	snd_soc_write(codec, 0x70, 0xbf);
	snd_soc_write(codec, 0x05, 0xe0);

	snd_soc_write(codec, 0x10, 0x40);
	snd_soc_write(codec, 0xc3, 0x80);
	snd_soc_write(codec, 0x70, 0xbe);

	return 0;
}

static int cod3033_power_off_mic1(struct snd_soc_codec *codec)
{
	//TDB
	return 0;
}
static int cod3033_power_on_mic2(struct snd_soc_codec *codec)
{

	dev_info(codec->dev, "%s called\n", __func__);

	snd_soc_write(codec, 0x70, 0xbf);
	snd_soc_write(codec, 0x60, 0x13);
	snd_soc_write(codec, 0xb6, 0x03);
	snd_soc_write(codec, 0x62, 0x11);
	snd_soc_write(codec, 0x8a, 0x30);
	snd_soc_write(codec, 0x62, 0x11);
	snd_soc_write(codec, 0x75, 0x20);
	snd_soc_write(codec, 0x76, 0x00);
	snd_soc_write(codec, 0x60, 0x17);
	snd_soc_write(codec, 0x70, 0x3f);
	snd_soc_write(codec, 0x70, 0xbf);
	snd_soc_write(codec, 0x05, 0xe0);

	snd_soc_write(codec, 0x10, 0x20);
	snd_soc_write(codec, 0xc3, 0x40);
	snd_soc_write(codec, 0x70, 0xbe);

	return 0;
}



static int cod3033_power_off_mic2(struct snd_soc_codec *codec)
{

	//TDB
	return 0;
}

static int cod3033_power_on_mic3(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "%s called\n", __func__);

	snd_soc_write(codec, 0x70, 0xbf);
	snd_soc_write(codec, 0x60, 0x13);
	snd_soc_write(codec, 0xb6, 0x03);
	snd_soc_write(codec, 0x62, 0x11);
	snd_soc_write(codec, 0x8a, 0x30);
	snd_soc_write(codec, 0x62, 0x11);
	snd_soc_write(codec, 0x75, 0x30);
	snd_soc_write(codec, 0x76, 0x00);
	snd_soc_write(codec, 0x60, 0x17);
	snd_soc_write(codec, 0x70, 0x3f);
	snd_soc_write(codec, 0x70, 0xbf);
	snd_soc_write(codec, 0x05, 0xe0);

	snd_soc_write(codec, 0x10, 0x10);
	snd_soc_write(codec, 0xc3, 0x20);
	snd_soc_write(codec, 0x70, 0xbe);

	dev_info(codec->dev, "%s called\n", __func__);

	return 0;
}

static int cod3033_power_on_mic4(struct snd_soc_codec *codec)
{
	dev_err(codec->dev, "%s called\n", __func__);

	snd_soc_write(codec,0xb3,0x04);
	mdelay(10);
	snd_soc_write(codec,0x10,0x05);

	snd_soc_write(codec,0xb3,0x04);

	snd_soc_write(codec,0x11,0x18);
	snd_soc_write(codec,0x12,0x18);

/*	snd_soc_write(codec,0x10,0x85);
	mdelay(10);
	snd_soc_write(codec,0x10,0x8d);
	snd_soc_write(codec,0x04,0x0e);
	snd_soc_write(codec,0x05,0x0e);
	snd_soc_write(codec,0x06,0xcc);
	snd_soc_write(codec,0x37,0x4f);
	snd_soc_write(codec,0x3b,0x02);

//	snd_soc_write(codec,0x3b,0x02);
	mdelay(10);
	snd_soc_write(codec,0x10,0x0d);
*/
	mdelay(10);

	snd_soc_write(codec,0x67,0x10);
	snd_soc_write(codec,0x68,0x20);

	snd_soc_write(codec, 0x70, 0xbf);
	snd_soc_write(codec, 0x60, 0x13);
	snd_soc_write(codec, 0xb6, 0x03);
	snd_soc_write(codec, 0x62, 0x11);
	snd_soc_write(codec, 0x8a, 0x30);
	snd_soc_write(codec, 0x62, 0x11);
	snd_soc_write(codec, 0x75, 0x30);
	snd_soc_write(codec, 0x76, 0x30);
	snd_soc_write(codec, 0x60, 0x17);
	snd_soc_write(codec, 0x70, 0x3f);
	snd_soc_write(codec, 0x70, 0xbf);
	snd_soc_write(codec, 0x05, 0xe0);


	snd_soc_write(codec, 0x00, 0x0e);

	snd_soc_write(codec, 0xc3, 0x10);

	mdelay(140);
	snd_soc_write(codec, 0x70, 0xbe);
	return 0;
}

static int cod3033_power_off_mic4(struct snd_soc_codec *codec)
{

	return 0;
}
static int cod3033_power_off_mic3(struct snd_soc_codec *codec)
{
	return 0;
}

static int vmid_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s called\n", __func__);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		break;

	case SND_SOC_DAPM_POST_PMU:
		break;

	case SND_SOC_DAPM_PRE_PMD:
		break;

	default:
		break;
	}

	return 0;
}


static int spkdrv_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	// empty
	return 0;
}

static int hpdrv_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_write(codec,0x67,0x10);
		snd_soc_write(codec,0x68,0x20);
		break;

	case SND_SOC_DAPM_POST_PMU:
		snd_soc_write(codec,0xF0,0x0F);
		snd_soc_write(codec,0x66,0x01);
		snd_soc_write(codec,0x6A,0x01);
		snd_soc_write(codec,0x00,0x08);
		snd_soc_write(codec,0x60,0x1B);
		snd_soc_write(codec,0x64,0x17);
		snd_soc_write(codec,0x65,0x0F);
		snd_soc_write(codec,0x8E,0x81);
		snd_soc_write(codec,0xD0,0x48);
		snd_soc_write(codec,0xDB,0x30);
		snd_soc_write(codec,0x9D,0x61);
		snd_soc_write(codec,0xa4,0x00);
		snd_soc_write(codec,0x82,0x54);
		snd_soc_write(codec,0x83,0x54);
		snd_soc_write(codec,0xB6,0x00);
		snd_soc_write(codec,0xB7,0x8D);
		snd_soc_write(codec,0xD4,0x03);
		snd_soc_write(codec,0xD5,0x4C);
		snd_soc_write(codec,0xD6,0xCD);
		snd_soc_write(codec,0xD7,0xA0);
		snd_soc_write(codec,0xD8,0x20);
		snd_soc_write(codec,0xA5,0x05);
		snd_soc_write(codec,0xD7,0xE0);
		snd_soc_write(codec,0x45,0x40);
		snd_soc_write(codec,0x91,0x0A);
		snd_soc_write(codec,0xC5,0x17);
		snd_soc_write(codec,0xC6,0x01);
		snd_soc_write(codec,0x8D,0x0F);
		break;

	case SND_SOC_DAPM_PRE_PMD:

		break;

	default:
		break;
	}

	return 0;
}

static int epdrv_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_write(codec, 0x00, 0x08);
		snd_soc_write(codec, 0x60, 0x1B);
		snd_soc_write(codec, 0x64, 0x17);
		snd_soc_write(codec, 0x65, 0x1F);
		snd_soc_write(codec, 0x8E, 0x21);
		snd_soc_write(codec, 0xD0, 0xC1);
		snd_soc_write(codec, 0xDB, 0x30);
		snd_soc_write(codec, 0x9D, 0x6A);
		snd_soc_write(codec, 0xa4, 0x70);
		snd_soc_write(codec, 0x82, 0x54);
		snd_soc_write(codec, 0x83, 0x54);
		snd_soc_write(codec, 0xB6, 0x00);
		snd_soc_write(codec, 0xB7, 0x3C);
		snd_soc_write(codec, 0xD4, 0x03);
		snd_soc_write(codec, 0xD5, 0x4C);
		snd_soc_write(codec, 0xD6, 0xCD);
		snd_soc_write(codec, 0xD7, 0x00);
		snd_soc_write(codec, 0xD8, 0x20);
		snd_soc_write(codec, 0xA5, 0x05);
		snd_soc_write(codec, 0xD7, 0xC0);
		snd_soc_write(codec, 0x45, 0x40);
		snd_soc_write(codec, 0x91, 0x0A);
		snd_soc_write(codec, 0xC7, 0x01);
		snd_soc_write(codec, 0x8D, 0x07);
		break;

	case SND_SOC_DAPM_PRE_PMD:

	default:
		break;
	}

	return 0;
}

static int mic2_pga_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_err(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		cod3033_power_on_mic2(codec);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		cod3033_power_off_mic2(codec);
		break;
	default:
		break;
	}

	return 0;
}

static int mic1_pga_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		cod3033_power_on_mic1(codec);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		cod3033_power_off_mic1(codec);
		break;

	default:
		break;
	}

	return 0;
}

static int mic3_pga_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_dbg(codec->dev, "%s called, event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		cod3033_power_on_mic3(codec);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		cod3033_power_off_mic3(codec);
		break;

	default:
		break;
	}

	return 0;
}

static int mic4_pga_ev(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	dev_err(codec->dev, "%s called, event = %d\n", __func__, event);


	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		cod3033_power_on_mic4(codec);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		cod3033_power_off_mic4(codec);
		break;

	default:
		break;
	}

	return 0;
}

static const struct snd_kcontrol_new adcl_mix[] = {
	SOC_DAPM_SINGLE("MIC1L Switch", COD3033X_23_MIX_AD1,
			EN_MIX_MIC1L_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("MIC2L Switch", COD3033X_23_MIX_AD1,
			EN_MIX_MIC2L_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("MIC3L Switch", COD3033X_23_MIX_AD1,
			EN_MIX_MIC3L_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("LINELL Switch", COD3033X_23_MIX_AD1,
			EN_MIX_LNLL_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("LINERL Switch", COD3033X_24_MIX_AD2,
			EN_MIX_LNRL_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new adcr_mix[] = {
	SOC_DAPM_SINGLE("MIC1R Switch", COD3033X_23_MIX_AD1,
			EN_MIX_MIC1R_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("MIC2R Switch", COD3033X_23_MIX_AD1,
			EN_MIX_MIC2R_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("MIC3R Switch", COD3033X_23_MIX_AD1,
			EN_MIX_MIC3R_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("LINELR Switch", COD3033X_24_MIX_AD2,
			EN_MIX_LNLR_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("LINERR Switch", COD3033X_23_MIX_AD1,
			EN_MIX_LNRR_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new hpl_mix[] = {
	SOC_DAPM_SINGLE("DACL Switch", COD3033X_36_MIX_DA1,
			EN_HP_MIXL_DCTL_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", COD3033X_36_MIX_DA1,
			EN_HP_MIXL_DCTR_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", COD3033X_36_MIX_DA1,
			EN_HP_MIXL_MIXL_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", COD3033X_36_MIX_DA1,
			EN_HP_MIXL_MIXR_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new hpr_mix[] = {
	SOC_DAPM_SINGLE("DACL Switch", COD3033X_36_MIX_DA1,
			EN_HP_MIXR_DCTL_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", COD3033X_36_MIX_DA1,
			EN_HP_MIXR_DCTR_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", COD3033X_36_MIX_DA1,
			EN_HP_MIXR_MIXL_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", COD3033X_36_MIX_DA1,
			EN_HP_MIXR_MIXR_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new ep_mix[] = {
	SOC_DAPM_SINGLE("DACL Switch", COD3033X_37_MIX_DA2,
			EN_EP_MIX_DCTL_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", COD3033X_37_MIX_DA2,
			EN_EP_MIX_DCTR_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", COD3033X_37_MIX_DA2,
			EN_EP_MIX_MIXL_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", COD3033X_37_MIX_DA2,
			EN_EP_MIX_MIXR_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new spk_mix[] = {
	SOC_DAPM_SINGLE("DACL Switch", COD3033X_37_MIX_DA2,
			EN_SPK_MIX_DCTL_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", COD3033X_37_MIX_DA2,
			EN_SPK_MIX_DCTR_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", COD3033X_37_MIX_DA2,
			EN_SPK_MIX_MIXL_SHIFT, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", COD3033X_37_MIX_DA2,
			EN_SPK_MIX_MIXR_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new spk_on[] = {
	SOC_DAPM_SINGLE("SPK On", COD3033X_76_CHOP_DA,
				EN_SPK_PGA_CHOP_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new hp_on[] = {
	SOC_DAPM_SINGLE("HP On", COD3033X_76_CHOP_DA, EN_HP_CHOP_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new ep_on[] = {
	SOC_DAPM_SINGLE("EP On", COD3033X_76_CHOP_DA, EN_EP_CHOP_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new mic1_on[] = {
	SOC_DAPM_SINGLE("MIC1 On", COD3033X_78_MIC_ON,
					EN_MIC1_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new mic2_on[] = {
	SOC_DAPM_SINGLE("MIC2 On", COD3033X_78_MIC_ON,
					EN_MIC2_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new mic3_on[] = {
	SOC_DAPM_SINGLE("MIC3 On", COD3033X_78_MIC_ON,
					EN_MIC3_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new mic4_on[] = {
	SOC_DAPM_SINGLE("MIC4IN On", COD3033X_78_MIC_ON,
					EN_LN_SHIFT, 1, 0),
};


static const struct snd_soc_dapm_widget cod3033x_dapm_widgets[] = {
	SND_SOC_DAPM_SWITCH("SPK", SND_SOC_NOPM, 0, 0, spk_on),
	SND_SOC_DAPM_SWITCH("HP", SND_SOC_NOPM, 0, 0, hp_on),
	SND_SOC_DAPM_SWITCH("EP", SND_SOC_NOPM, 0, 0, ep_on),
	SND_SOC_DAPM_SWITCH("MIC1", SND_SOC_NOPM, 0, 0, mic1_on),
	SND_SOC_DAPM_SWITCH("MIC2", SND_SOC_NOPM, 0, 0, mic2_on),
	SND_SOC_DAPM_SWITCH("MIC3", SND_SOC_NOPM, 0, 0, mic3_on),
	SND_SOC_DAPM_SWITCH("MIC4IN", SND_SOC_NOPM, 0, 0, mic4_on),

	SND_SOC_DAPM_SUPPLY("VMID", SND_SOC_NOPM, 0, 0, vmid_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_OUT_DRV_E("SPKDRV", SND_SOC_NOPM, 0, 0, NULL, 0,
			spkdrv_ev, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_OUT_DRV_E("EPDRV", SND_SOC_NOPM, 0, 0, NULL, 0,
			epdrv_ev, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_OUT_DRV_E("HPDRV", SND_SOC_NOPM, 0, 0, NULL, 0,
			hpdrv_ev, SND_SOC_DAPM_PRE_PMU |
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_PGA_E("MIC1_PGA", SND_SOC_NOPM, 0, 0,
			NULL, 0, mic1_pga_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_PGA_E("MIC2_PGA", SND_SOC_NOPM, 0, 0,
			NULL, 0, mic2_pga_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_PGA_E("MIC3_PGA", SND_SOC_NOPM, 0, 0,
			NULL, 0, mic3_pga_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_PGA_E("MIC4IN_PGA", SND_SOC_NOPM, 0, 0,
			NULL, 0, mic4_pga_ev,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_MIXER("ADCL Mixer", SND_SOC_NOPM, 0, 0, adcl_mix,
			ARRAY_SIZE(adcl_mix)),
	SND_SOC_DAPM_MIXER("ADCR Mixer", SND_SOC_NOPM, 0, 0, adcr_mix,
			ARRAY_SIZE(adcr_mix)),
	SND_SOC_DAPM_MIXER("HPL Mixer", SND_SOC_NOPM, 0, 0, hpl_mix,
			ARRAY_SIZE(hpl_mix)),
	SND_SOC_DAPM_MIXER("HPR Mixer", SND_SOC_NOPM, 0, 0, hpr_mix,
			ARRAY_SIZE(hpr_mix)),
	SND_SOC_DAPM_MIXER("EP Mixer", SND_SOC_NOPM, 0, 0, ep_mix,
			ARRAY_SIZE(ep_mix)),
	SND_SOC_DAPM_MIXER("SPK Mixer", SND_SOC_NOPM, 0, 0, spk_mix,
			ARRAY_SIZE(spk_mix)),

	SND_SOC_DAPM_DAC_E("DAC", "AIF Playback", SND_SOC_NOPM, 0, 0,
			dac_ev, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_DAC_E("DAC", "AIF2 Playback", SND_SOC_NOPM, 0, 0,
			dac_ev, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_ADC_E("ADC", "AIF Capture", SND_SOC_NOPM, 0, 0,
			adc_ev, SND_SOC_DAPM_PRE_PMU |
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_ADC_E("ADC", "AIF2 Capture", SND_SOC_NOPM, 0, 0,
			adc_ev, SND_SOC_DAPM_PRE_PMU |
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_OUTPUT("SPKOUTLN"),
	SND_SOC_DAPM_OUTPUT("HPOUTLN"),
	SND_SOC_DAPM_OUTPUT("EPOUTN"),
	SND_SOC_DAPM_OUTPUT("AIF4OUT"),

	SND_SOC_DAPM_INPUT("IN1L"),
	SND_SOC_DAPM_INPUT("IN2L"),
	SND_SOC_DAPM_INPUT("IN3L"),
	SND_SOC_DAPM_INPUT("IN4L"),

	SND_SOC_DAPM_INPUT("AIF4IN"),
};

static const struct snd_soc_dapm_route cod3033x_dapm_routes[] = {
	/* Sink, Control, Source */
	{"SPK Mixer", "ADCL Switch", "ADCL Mixer"},
	{"SPK Mixer", "ADCR Switch", "ADCR Mixer"},
	{"SPK Mixer", "DACL Switch", "DAC"},
	{"SPK Mixer", "DACR Switch", "DAC"},

	{"SPKDRV", NULL, "SPK Mixer"},
	{"SPKDRV", NULL, "DAC"},
	{"SPK" , "SPK On", "SPKDRV"},
	{"SPKOUTLN", NULL, "SPK"},

	{"EP Mixer", "ADCL Switch", "ADCL Mixer"},
	{"EP Mixer", "ADCR Switch", "ADCR Mixer"},
	{"EP Mixer", "DACL Switch", "DAC"},
	{"EP Mixer", "DACR Switch", "DAC"},

	{"EPDRV", NULL, "EP Mixer"},
	{"EPDRV", NULL, "DAC"},
	{"EP", "EP On", "EPDRV"},
	{"EPOUTN", NULL, "EP"},

	{"HPL Mixer", "ADCL Switch", "ADCL Mixer"},
	{"HPL Mixer", "ADCR Switch", "ADCR Mixer"},
	{"HPL Mixer", "DACL Switch", "DAC"},
	{"HPL Mixer", "DACR Switch", "DAC"},

	{"HPR Mixer", "ADCL Switch", "ADCL Mixer"},
	{"HPR Mixer", "ADCR Switch", "ADCR Mixer"},
	{"HPR Mixer", "DACL Switch", "DAC"},
	{"HPR Mixer", "DACR Switch", "DAC"},

	{"HPDRV", NULL, "HPL Mixer"},
	{"HPDRV", NULL, "HPR Mixer"},
	{"HPDRV", NULL, "DAC"},
	{"HP", "HP On", "HPDRV"},
	{"HPOUTLN", NULL, "HP"},

	{"DAC" , NULL, "AIF Playback"},
	{"DAC" , NULL, "AIF2 Playback"},
	{"MIC1_PGA", NULL, "IN1L"},
	{"MIC1_PGA", NULL, "VMID"},
	{"MIC1", "MIC1 On", "MIC1_PGA"},

	{"ADC", NULL, "MIC1"},
	{"ADC", NULL, "MIC1"},

	{"MIC2_PGA", NULL, "IN2L"},
	{"MIC2_PGA", NULL, "VMID"},
	{"MIC2", "MIC2 On", "MIC2_PGA"},

	{"ADC", NULL, "MIC2"},
	{"ADC", NULL, "MIC2"},

	{"MIC3_PGA", NULL, "IN3L"},
	{"MIC3_PGA", NULL, "VMID"},
	{"MIC3", "MIC3 On", "MIC3_PGA"},

	{"ADC", NULL, "MIC3"},
	{"ADC", NULL, "MIC3"},

	{"MIC4IN_PGA", NULL, "IN4L"},
	{"MIC4IN_PGA", NULL, "VMID"},
	{"MIC4IN", "MIC4IN On", "MIC4IN_PGA"},

	{"ADC", NULL, "MIC4IN"},
	{"ADC", NULL, "MIC4IN"},

	{"AIF Capture", NULL, "ADC"},
	{"AIF2 Capture", NULL, "ADC"},

	{"FM Link", "FM On", "ADC"},
	{"DAC", NULL, "FM Link"},
};

static int cod3033x_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	int bclk = 0, lrclk = 0;

	dev_dbg(codec->dev, "%s called\n", __func__);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_LEFT_J:
		fmt = LRJ_AUDIO_FORMAT_MASK;
		break;

	case SND_SOC_DAIFMT_I2S:
		fmt = I2S_AUDIO_FORMAT_MASK;
		break;

	default:
		pr_err("Unsupported DAI format %d\n",
				fmt & SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}

//	snd_soc_update_bits(codec, COD3033X_41_FORMAT,
//			I2S_AUDIO_FORMAT_MASK | LRJ_AUDIO_FORMAT_MASK, fmt);

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_IF:
		bclk = BCLK_POL_MASK;
		lrclk = LRCLK_POL_MASK;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		bclk = BCLK_POL_MASK;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		lrclk = LRCLK_POL_MASK;
		break;
	default:
		pr_err("Unsupported Polartiy selection %d\n",
				fmt & SND_SOC_DAIFMT_INV_MASK);
		return -EINVAL;
	}

//	snd_soc_update_bits(codec, COD3033X_41_FORMAT,
//			BCLK_POL_MASK | LRCLK_POL_MASK, bclk | lrclk);
	return 0;
}


static int cod3033x_dai_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;

	dev_dbg(codec->dev, "(%s) %s completed\n",
			substream->stream ? "C" : "P", __func__);

	return 0;
}

static void cod3033x_dai_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;

	dev_dbg(codec->dev, "(%s) %s completed\n",
			substream->stream ? "C" : "P", __func__);
}


static int cod3033x_dai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cod3033x_priv *cod3033x = snd_soc_codec_get_drvdata(codec);
	unsigned int cur_aifrate;
	int ret=0;

	dev_dbg(codec->dev, "%s called\n", __func__);

	/* 192 KHz support */
	cur_aifrate = params_rate(params);
	if (cod3033x->aifrate != cur_aifrate) {
		/* DNC needs to be disabled while switching samplerate */

		/* Need to reset H/W while switching from 192KHz to 48KHz */

		/*
		 * If HP is already on, then change the 'current' setting based
		 * on samplerate
		 */

		/* DNC mode can be restored after the samplerate switch */
	}

	/*
	 * Codec supports only 24bits per sample, Mixer performs the required
	 * conversion to 24 bits. BFS is fixed at 64fs for mixer<->codec
	 * interface.
	 */
	if (ret < 0) {
		dev_err(codec->dev, "%s failed to set bits per sample\n",
				__func__);
		return ret;
	}

	return 0;
}


static const struct snd_soc_dai_ops cod3033x_dai_ops = {
	.set_fmt = cod3033x_dai_set_fmt,
	.startup = cod3033x_dai_startup,
	.shutdown = cod3033x_dai_shutdown,
	.hw_params = cod3033x_dai_hw_params,
};

#define COD3033X_RATES		SNDRV_PCM_RATE_8000_192000

#define COD3033X_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE |		\
				SNDRV_PCM_FMTBIT_S20_3LE |	\
				SNDRV_PCM_FMTBIT_S24_LE |	\
				SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_driver cod3033x_dai[] = {
	{
		.name = "cod3033x-aif",
		.id = 1,
		.playback = {
			.stream_name = "AIF Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = COD3033X_RATES,
			.formats = COD3033X_FORMATS,
		},
		.capture = {
			.stream_name = "AIF Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = COD3033X_RATES,
			.formats = COD3033X_FORMATS,
		},
		.ops = &cod3033x_dai_ops,
		.symmetric_rates = 1,
	},
	{
		.name = "cod3033x-aif2",
		.id = 2,
		.playback = {
			.stream_name = "AIF2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = COD3033X_RATES,
			.formats = COD3033X_FORMATS,
		},
		.capture = {
			.stream_name = "AIF2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = COD3033X_RATES,
			.formats = COD3033X_FORMATS,
		},
		.ops = &cod3033x_dai_ops,
		.symmetric_rates = 1,
	},
};

/* The clock for COD3033X is provided by the Audio sub-system. Hence we need to
 * ensure that the audio subsystem is active during codec operation. The
 * easiest way to do this is by calling s2803x_{get/put}_sync() helper
 * functions.
 */

static void cod3033x_save_otp_registers(struct snd_soc_codec *codec)
{

}

static void cod3033x_restore_otp_registers(struct snd_soc_codec *codec)
{

}

static void cod3033x_reset_io_selector_bits(struct snd_soc_codec *codec)
{

}

/**
 * cod3033x_post_fw_update_failure: To be called if f/w update fails
 *
 * In case the firmware is not present or corrupt, we should still be able to
 * run the codec with decent parameters. This values are updated as per the
 * latest stable firmware.
 *
 * The values provided in this function are hard-coded register values, and we
 * need not update these values as per bit-fields.
 */
static void cod3033x_post_fw_update_failure(void *context)
{

}


static void cod3033x_regmap_sync(struct device *dev)
{
	struct cod3033x_priv *cod3033x = dev_get_drvdata(dev);
	unsigned char reg[0xff]={0,};
	int i;

	/* Read from Cache */
	for (i = 0; i < 0xff ; i++)
		if (cod3033x_readable_register(dev, i) &&
				(!cod3033x_volatile_register(dev,i)))
			reg[i] = (unsigned char)
				snd_soc_read(cod3033x->codec, i);

	regcache_cache_bypass(cod3033x->regmap, true);


	/* Update HW */
	for (i = 0; i < 0xff ; i++)
		if (cod3033x_writeable_register(dev, i) &&
				(!cod3033x_volatile_register(dev,i)))
			snd_soc_write(cod3033x->codec, i, reg[i]);

	regcache_cache_bypass(cod3033x->regmap, false);

}

static void cod3033x_reg_restore(struct snd_soc_codec *codec)
{
	struct cod3033x_priv *cod3033x = snd_soc_codec_get_drvdata(codec);

	/*
	 * The OTP values are the boot-time values. For registers D0-DE, we need
	 * to save these register values during boot time. After system reset,
	 * these values are lost and we need to restore them using saved values.
	 */
	if (!cod3033x->is_probe_done) {
		cod3033x_regmap_sync(codec->dev);
		cod3033x_reset_io_selector_bits(codec);
		cod3033x_save_otp_registers(codec);
	} else {
		cod3033x_regmap_sync(codec->dev);
		cod3033x_restore_otp_registers(cod3033x->codec);
	}
}
static void cod3033x_i2c_parse_dt(struct cod3033x_priv *cod3033x)
{
	/* todo .. Need to add DT parsing for 3033 */
	struct device *dev = cod3033x->dev;
	struct device_node *np = dev->of_node;
	unsigned int bias_v_conf;
	int mic_range, mic_delay, btn_rel_val;
	struct of_phandle_args args;
	int i = 0;
	int ret;

	cod3033x->int_gpio = of_get_gpio(np, 0);

	if (cod3033x->int_gpio < 0)
		dev_err(dev, "(*)Error in getting Codec-3033 Interrupt gpio\n");

	/* Default Bias Voltages */
	cod3033x->mic_bias1_voltage = MIC_BIAS1_VO_3_0V;
	cod3033x->mic_bias2_voltage = MIC_BIAS2_VO_3_0V;
	cod3033x->mic_bias_ldo_voltage = MIC_BIAS_LDO_VO_3_3V;

	ret = of_property_read_u32(dev->of_node,
				"mic-bias1-voltage", &bias_v_conf);
	if ((!ret) && ((bias_v_conf >= MIC_BIAS1_VO_2_8V) &&
			(bias_v_conf <= MIC_BIAS1_VO_3_0V)))
		cod3033x->mic_bias1_voltage = bias_v_conf;
	else
		dev_dbg(dev, "Property 'mic-bias1-voltage' %s",
				ret ? "not found" : "invalid value (use:1-3)");

	ret = of_property_read_u32(dev->of_node,
				"mic-bias2-voltage", &bias_v_conf);
	if ((!ret) && ((bias_v_conf >= MIC_BIAS2_VO_2_8V) &&
			(bias_v_conf <= MIC_BIAS2_VO_3_0V)))
		cod3033x->mic_bias2_voltage = bias_v_conf;
	else
		dev_dbg(dev, "Property 'mic-bias2-voltage' %s",
				ret ? "not found" : "invalid value (use:1-3)");

	ret = of_property_read_u32(dev->of_node, "mic-adc-range", &mic_range);
	if (!ret)
		cod3033x->mic_adc_range = mic_range;
	else
		cod3033x->mic_adc_range = 1120;

	ret = of_property_read_u32(dev->of_node, "mic-det-delay", &mic_delay);
	if (!ret)
		cod3033x->mic_det_delay = mic_delay;
	else
		cod3033x->mic_det_delay = 50;

	ret = of_property_read_u32(dev->of_node, "btn-release-value", &btn_rel_val);
	if (!ret)
		cod3033x->btn_release_value = btn_rel_val;
	else
		cod3033x->btn_release_value = 1100;

	ret = of_property_read_u32(dev->of_node,
				"mic-bias-ldo-voltage", &bias_v_conf);
	if ((!ret) && ((bias_v_conf >= MIC_BIAS_LDO_VO_2_8V) &&
			(bias_v_conf <= MIC_BIAS_LDO_VO_3_3V)))
		cod3033x->mic_bias_ldo_voltage = bias_v_conf;
	else
		dev_dbg(dev, "Property 'mic-bias-ldo-voltage' %s",
				ret ? "not found" : "invalid value (use:1-3)");

	dev_dbg(dev, "Bias voltage values: bias1 = %d, bias2= %d, ldo = %d\n",
			cod3033x->mic_bias1_voltage,
			cod3033x->mic_bias2_voltage,
			cod3033x->mic_bias_ldo_voltage);

	if (of_find_property(dev->of_node,
				"update-firmware", NULL))
		cod3033x->update_fw = true;
	else
		cod3033x->update_fw = false;

	if (of_find_property(dev->of_node, "use-det-adc-mode", NULL) != NULL)
		cod3033x->use_det_adc_mode = true;

	dev_err(dev, "Using %s for jack/button detection\n",
			cod3033x->use_det_adc_mode ? "GPADC" : "internal h/w");
	if (cod3033x->use_det_adc_mode) {
		/* Parsing but-zones, a maximum of 4 buttons are supported */
		for (i = 0; i < 4; i++) {
			if (of_parse_phandle_with_args(dev->of_node,
				"but-zones-list", "#list-but-cells", i, &args))
				break;

			cod3033x->jack_buttons_zones[i].code = args.args[0];
			cod3033x->jack_buttons_zones[i].adc_low = args.args[1];
			cod3033x->jack_buttons_zones[i].adc_high = args.args[2];
		}
		/* initialize button status */

		for (i = 0; i < 4; i++)
			dev_err(dev, "[DEBUG]: buttons: code(%d), low(%d), high(%d)\n",
				cod3033x->jack_buttons_zones[i].code,
				cod3033x->jack_buttons_zones[i].adc_low,
				cod3033x->jack_buttons_zones[i].adc_high);
	}
}

static int cod3033x_codec_probe(struct snd_soc_codec *codec)
{
	struct cod3033x_priv *cod3033x = snd_soc_codec_get_drvdata(codec);
	struct snd_soc_card *card = codec->component.card;

	dev_dbg(codec->dev, "(*) %s\n", __func__);
	cod3033x->codec = codec;


#ifdef CONFIG_PM_RUNTIME
	pm_runtime_get_sync(codec->dev);
#else
	cod3033x_enable(codec->dev);
#endif

	cod3033x->is_probe_done = true;

	cod3033x->aifrate = COD3033X_SAMPLE_RATE_48KHZ;

	cod3033x_i2c_parse_dt(cod3033x);

	cod3033x_post_fw_update_failure(codec);


	snd_soc_dapm_ignore_suspend(&card->dapm, "SPKOUTLN");
	snd_soc_dapm_ignore_suspend(&card->dapm, "HPOUTLN");
	snd_soc_dapm_ignore_suspend(&card->dapm, "EPOUTN");
	snd_soc_dapm_ignore_suspend(&card->dapm, "IN1L");
	snd_soc_dapm_ignore_suspend(&card->dapm, "IN2L");
	snd_soc_dapm_ignore_suspend(&card->dapm, "IN3L");
	snd_soc_dapm_ignore_suspend(&card->dapm, "IN4L");
	snd_soc_dapm_ignore_suspend(&card->dapm, "AIF Playback");
	snd_soc_dapm_ignore_suspend(&card->dapm, "AIF Capture");
	snd_soc_dapm_ignore_suspend(&card->dapm, "AIF2 Playback");
	snd_soc_dapm_ignore_suspend(&card->dapm, "AIF2 Capture");
	snd_soc_dapm_sync(&card->dapm);


#ifdef CONFIG_PM_RUNTIME
	pm_runtime_put_sync(codec->dev);
#else
	cod3033x_disable(codec->dev);
#endif

	return 0;
}

static int cod3033x_codec_remove(struct snd_soc_codec *codec)
{
	struct cod3033x_priv *cod3033x = snd_soc_codec_get_drvdata(codec);
	dev_dbg(codec->dev, "(*) %s called\n", __func__);

	cancel_delayed_work_sync(&cod3033x->key_work);
	if (cod3033x->int_gpio) {
		free_irq(gpio_to_irq(cod3033x->int_gpio), cod3033x);
		gpio_free(cod3033x->int_gpio);
	}

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_cod3033x = {
	.probe = cod3033x_codec_probe,
	.remove = cod3033x_codec_remove,
	.controls = cod3033x_snd_controls,
	.num_controls = ARRAY_SIZE(cod3033x_snd_controls),
	.dapm_widgets = cod3033x_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(cod3033x_dapm_widgets),
	.dapm_routes = cod3033x_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(cod3033x_dapm_routes),
	.ignore_pmdown_time = true,
	.idle_bias_off = true,
};

static int cod3033x_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
{
	struct cod3033x_priv *cod3033x;
	struct pinctrl *pinctrl;
	int ret;

	cod3033x = kzalloc(sizeof(struct cod3033x_priv), GFP_KERNEL);
	if (cod3033x == NULL)
		return -ENOMEM;
	cod3033x->dev = &i2c->dev;
	cod3033x->i2c_addr = i2c->addr;

	cod3033x->regmap = devm_regmap_init_i2c(i2c, &cod3033x_regmap);
	if (IS_ERR(cod3033x->regmap)) {
		dev_err(&i2c->dev, "Failed to allocate regmap: %li\n",
				PTR_ERR(cod3033x->regmap));
		kfree(cod3033x);
		return -ENOMEM;
	}

	regcache_mark_dirty(cod3033x->regmap);

	pinctrl = devm_pinctrl_get(&i2c->dev);
	if (IS_ERR(pinctrl)) {
		dev_warn(&i2c->dev, "did not get pins for codec: %li\n",
							PTR_ERR(pinctrl));
	} else {
		cod3033x->pinctrl = pinctrl;
	}

	i2c_set_clientdata(i2c, cod3033x);

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_cod3033x,
			cod3033x_dai, ARRAY_SIZE(cod3033x_dai));
	if (ret < 0)
		dev_err(&i2c->dev, "Failed to register codec: %d\n", ret);
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_enable(cod3033x->dev);
#endif


	return ret;
}

static int cod3033x_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);
	return 0;
}

static void cod3033x_cfg_gpio(struct device *dev, const char *name)
{
	struct pinctrl_state *pin_state;
	struct cod3033x_priv *cod3033x = dev_get_drvdata(dev);

	pin_state = pinctrl_lookup_state(cod3033x->pinctrl, name);
	if (IS_ERR(pin_state))
		goto err;

	if (pinctrl_select_state(cod3033x->pinctrl, pin_state) < 0)
		goto err;

	return;
err:
	dev_err(dev, "Unable to configure codec gpio as %s\n", name);
	return;
}

static int cod3033x_enable(struct device *dev)
{
	struct cod3033x_priv *cod3033x = dev_get_drvdata(dev);

	dev_dbg(dev, "(*) %s\n", __func__);


	cod3033x_cfg_gpio(dev, "default");
	/*
	 * Below sequence should be maintained, so that even the jd interupt
	 * changes the cache mode between below two line should not cause
	 * issue
	 */
	cod3033x->is_suspend = false;

	/* Disable cache_only feature and sync the cache with h/w */
	regcache_cache_only(cod3033x->regmap, false);
	cod3033x_reg_restore(cod3033x->codec);

	return 0;
}

static int cod3033x_disable(struct device *dev)
{
	struct cod3033x_priv *cod3033x = dev_get_drvdata(dev);

	dev_dbg(dev, "(*) %s\n", __func__);

	/*
	 * Below sequence should be maintained, so that even the jd interupt
	 * changes the cache mode between below two line should not cause
	 * issue
	 */
	cod3033x->is_suspend = true;

	/* As device is going to suspend-state, limit the writes to cache */
//	regcache_cache_only(cod3033x->regmap, true);


	return 0;
}

static int cod3033x_sys_suspend(struct device *dev)
{
#ifndef CONFIG_PM_RUNTIME
	//$$$_kjc struct cod3033x_priv *cod3026x = dev_get_drvdata(dev);

/*	if (is_cp_aud_enabled()) {
		dev_dbg(dev, "(*)Don't suspend Codec-3033, cp functioning\n");
		return 0;
	}
	dev_dbg(dev, "(*) %s\n", __func__);
*/	cod3033x_disable(dev);
#endif

	return 0;
}

static int cod3033x_sys_resume(struct device *dev)
{
#ifndef CONFIG_PM_RUNTIME
	struct cod3033x_priv *cod3033x = dev_get_drvdata(dev);

	if (!cod3033x->is_suspend) {
		dev_dbg(dev, "(*)Codec-3033 not resuming, cp functioning\n");
		return 0;
	}
	dev_dbg(dev, "(*) %s\n", __func__);
	cod3033x_enable(dev);
#endif

	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int cod3033x_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "(*) %s\n", __func__);

	cod3033x_enable(dev);

	return 0;
}

static int cod3033x_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "(*) %s\n", __func__);

	cod3033x_disable(dev);

	return 0;
}
#endif

static const struct dev_pm_ops cod3033x_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(
			cod3033x_sys_suspend,
			cod3033x_sys_resume
	)
#ifdef CONFIG_PM_RUNTIME
	SET_RUNTIME_PM_OPS(
			cod3033x_runtime_suspend,
			cod3033x_runtime_resume,
			NULL
	)
#endif
};

static const struct i2c_device_id cod3033x_i2c_id[] = {
	{ "cod3033x", 3033 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cod3033x_i2c_id);

const struct of_device_id cod3033x_of_match[] = {
	{ .compatible = "codec,cod3033x",},
	{},
};

static struct i2c_driver cod3033x_i2c_driver = {
	.driver = {
		.name = "cod3033x",
		.owner = THIS_MODULE,
		.pm = &cod3033x_pm,
		.of_match_table = of_match_ptr(cod3033x_of_match),
	},
	.probe = cod3033x_i2c_probe,
	.remove = cod3033x_i2c_remove,
	.id_table = cod3033x_i2c_id,
};

module_i2c_driver(cod3033x_i2c_driver);

MODULE_DESCRIPTION("ASoC COD3033X driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:COD3033X-codec");
MODULE_FIRMWARE(COD3033X_FIRMWARE_NAME);
