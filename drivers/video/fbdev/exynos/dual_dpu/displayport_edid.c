/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung SoC DisplayPort EDID driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/fb.h>
#include <media/v4l2-dv-timings.h>
#include <uapi/linux/v4l2-dv-timings.h>

#include "displayport.h"

#define EDID_SEGMENT_ADDR	(0x60 >> 1)
#define EDID_ADDR		(0xA0 >> 1)
#define EDID_SEGMENT_IGNORE	(2)
#define EDID_BLOCK_SIZE		128
#define EDID_SEGMENT(x)		((x) >> 1)
#define EDID_OFFSET(x)		(((x) & 1) * EDID_BLOCK_SIZE)
#define EDID_EXTENSION_FLAG	0x7E
#define EDID_NATIVE_FORMAT	0x83
#define EDID_BASIC_AUDIO	(1 << 6)
#define EDID_COLOR_DEPTH	0x14
int forced_resolution = -1;

/* displayport_supported_presets[] is to be arranged in the order of pixel clock */
struct displayport_supported_preset displayport_supported_presets[] = {
	{V4L2_DV_BT_DMT_640X480P60,      640, 480,  60, FB_VMODE_NONINTERLACED,   1, "640x480p@60"},
	{V4L2_DV_BT_CEA_720X480P59_94,	 720, 480,  59, FB_VMODE_NONINTERLACED,   2, "720x480p@60"},
	{V4L2_DV_BT_CEA_720X576P50,      720, 576,  50, FB_VMODE_NONINTERLACED,  17, "720x576p@50"},
	{V4L2_DV_BT_DMT_1280X800P60_RB,	    1280,  800, 60, FB_VMODE_NONINTERLACED,   0, "1280x800p@60_RB"},
	{V4L2_DV_BT_CEA_1280X720P50,	    1280,  720, 50, FB_VMODE_NONINTERLACED,  19, "1280x720p@50"},
	{V4L2_DV_BT_CEA_1280X720P60,	    1280,  720, 60, FB_VMODE_NONINTERLACED,   4, "1280x720p@60"},
	{V4L2_DV_BT_DMT_1280X1024P60,	    1280, 1024, 60, FB_VMODE_NONINTERLACED,   0, "1280x1024p@60"},
	{V4L2_DV_BT_CEA_1920X1080P24,	    1920, 1080, 24, FB_VMODE_NONINTERLACED,  32, "1920x1080p@24"},
	{V4L2_DV_BT_CEA_1920X1080P25,	    1920, 1080, 25, FB_VMODE_NONINTERLACED,  33, "1920x1080p@25"},
	{V4L2_DV_BT_CEA_1920X1080P30,	    1920, 1080, 30, FB_VMODE_NONINTERLACED,  34, "1920x1080p@30"},
	{V4L2_DV_BT_CVT_1920X1080P59_ADDED, 1920, 1080, 59, FB_VMODE_NONINTERLACED,   0, "1920x1080p@59"},
	{V4L2_DV_BT_CEA_1920X1080P50,	    1920, 1080, 50, FB_VMODE_NONINTERLACED,  31, "1920x1080p@50"},
	{V4L2_DV_BT_CEA_1920X1080P60,	    1920, 1080, 60, FB_VMODE_NONINTERLACED,  16, "1920x1080p@60"},
	{V4L2_DV_BT_CVT_2048X1536P60_ADDED, 2048, 1536, 60, FB_VMODE_NONINTERLACED,   0, "2048x1536p@60"},
	{V4L2_DV_BT_DMT_1920X1440P60,	    1920, 1440, 60, FB_VMODE_NONINTERLACED,   0, "1920x1440p@60"},
	{V4L2_DV_BT_CVT_2560X1440P59_ADDED, 2560, 1440, 59, FB_VMODE_NONINTERLACED,   0, "2560x1440p@59"},
	{V4L2_DV_BT_CVT_2560X1440P60_ADDED, 2560, 1440, 60, FB_VMODE_NONINTERLACED,   0, "2560x1440p@60"},
	{V4L2_DV_BT_CEA_3840X2160P24,	    3840, 2160, 24, FB_VMODE_NONINTERLACED,  93, "3840x2160p@24"},
	{V4L2_DV_BT_CEA_3840X2160P25,	    3840, 2160, 25, FB_VMODE_NONINTERLACED,  94, "3840x2160p@25"},
	{V4L2_DV_BT_CEA_3840X2160P30,	    3840, 2160, 30, FB_VMODE_NONINTERLACED,  95, "3840x2160p@30"},
	{V4L2_DV_BT_CEA_4096X2160P24,	    4096, 2160, 24, FB_VMODE_NONINTERLACED,  98, "4096x2160p@24"},
	{V4L2_DV_BT_CEA_4096X2160P25,	    4096, 2160, 25, FB_VMODE_NONINTERLACED,  99, "4096x2160p@25"},
	{V4L2_DV_BT_CEA_4096X2160P30,	    4096, 2160, 30, FB_VMODE_NONINTERLACED, 100, "4096x2160p@30"},
	{V4L2_DV_BT_CVT_3840X2160P59_ADDED, 3840, 2160, 59, FB_VMODE_NONINTERLACED,   0, "3840x2160p@59_RB"},
	{V4L2_DV_BT_CEA_3840X2160P50,	    3840, 2160, 50, FB_VMODE_NONINTERLACED,  96, "3840x2160p@50"},
	{V4L2_DV_BT_CEA_3840X2160P60,	    3840, 2160, 60, FB_VMODE_NONINTERLACED,  97, "3840x2160p@60"},
	{V4L2_DV_BT_CEA_4096X2160P50,	4096, 2160, 50, FB_VMODE_NONINTERLACED, 101, "4096x2160p@50"},
	{V4L2_DV_BT_CEA_4096X2160P60,	4096, 2160, 60, FB_VMODE_NONINTERLACED, 102, "4096x2160p@60"},
};

static struct fb_videomode ud_mode_h14b_vsdb[] = {
	{"3840x2160p@30", 30, 3840, 2160, 297000000, 0, 0, 0, 0, 0, 0, 0, FB_VMODE_NONINTERLACED, 0},
	{"3840x2160p@25", 25, 3840, 2160, 297000000, 0, 0, 0, 0, 0, 0, 0, FB_VMODE_NONINTERLACED, 0},
	{"3840x2160p@24", 24, 3840, 2160, 297000000, 0, 0, 0, 0, 0, 0, 0, FB_VMODE_NONINTERLACED, 0},
	{"4096x2160p@24", 24, 4096, 2160, 297000000, 0, 0, 0, 0, 0, 0, 0, FB_VMODE_NONINTERLACED, 0},
};

const int displayport_pre_cnt = ARRAY_SIZE(displayport_supported_presets);

static struct v4l2_dv_timings preferred_preset = V4L2_DV_BT_DMT_640X480P60;
static u32 edid_misc;
static int audio_channels;
static int audio_bit_rates;
static int audio_sample_rates;
static int audio_speaker_alloc;

void edid_check_set_i2c_capabilities(void)
{
	u8 val[1];

	displayport_reg_dpcd_read(DPCD_ADD_I2C_SPEED_CONTROL_CAPABILITES, 1, val);
	displayport_info("DPCD_ADD_I2C_SPEED_CONTROL_CAPABILITES = 0x%x\n", val[0]);

	if (val[0] != 0) {
		if (val[0] & I2C_1Mbps)
			val[0] = I2C_1Mbps;
		else if (val[0] & I2C_400Kbps)
			val[0] = I2C_400Kbps;
		else if (val[0] & I2C_100Kbps)
			val[0] = I2C_100Kbps;
		else if (val[0] & I2C_10Kbps)
			val[0] = I2C_10Kbps;
		else if (val[0] & I2C_1Kbps)
			val[0] = I2C_1Kbps;
		else
			val[0] = I2C_400Kbps;
	
		displayport_reg_dpcd_write(DPCD_ADD_I2C_SPEED_CONTROL_STATUS, 1, val);
		displayport_dbg("DPCD_ADD_I2C_SPEED_CONTROL_STATUS = 0x%x\n", val[0]);
	}
}

static int edid_read_block(struct displayport_device *hdev, int block, u8 *buf, size_t len)
{
	int ret, i;
	u8 offset = EDID_OFFSET(block);
	int sum = 0;

	if (len < EDID_BLOCK_SIZE)
		return -EINVAL;

	edid_check_set_i2c_capabilities();
	ret = displayport_reg_edid_read(offset, EDID_BLOCK_SIZE, buf);
	if (ret)
		return ret;

	for (i = 0; i < EDID_BLOCK_SIZE; i++)
		sum += buf[i];

	print_hex_dump(KERN_INFO, "EDID = ", DUMP_PREFIX_OFFSET, 16, 1,
					buf, 128, false);
	sum%=0x100;	//Checksum. Sum of all 128 bytes should equal 0 (mod 256).
	if (sum) {
		displayport_err("%s: checksum error block = %d sum = %02x\n", __func__, block, sum);
		return -EPROTO;
	}

	return 0;
}

int edid_read(struct displayport_device *hdev, u8 **data)
{
	u8 block0[EDID_BLOCK_SIZE];
	u8 *edid;
	int block = 0;
	int block_cnt, ret;

	ret = edid_read_block(hdev, 0, block0, sizeof(block0));
	if (ret)
		return ret;

	block_cnt = block0[EDID_EXTENSION_FLAG] + 1;
	displayport_info("block_cnt = %d\n", block_cnt);

	edid = kmalloc(block_cnt * EDID_BLOCK_SIZE, GFP_KERNEL);
	if (!edid)
		return -ENOMEM;

	memcpy(edid, block0, sizeof(block0));

	while (++block < block_cnt) {
		ret = edid_read_block(hdev, block,
			edid + block * EDID_BLOCK_SIZE,
			EDID_BLOCK_SIZE);

		if (ret) {
			kfree(edid);
			return ret;
		}
	}

	*data = edid;

	return block_cnt;
}

static int get_ud_timing(struct fb_vendor *vsdb, int vic_idx)
{
	unsigned char val = 0;
	int idx = -EINVAL;

	val = vsdb->vic_data[vic_idx];
	switch (val) {
	case 0x01:
		idx = 0;
		break;
	case 0x02:
		idx = 1;
		break;
	case 0x03:
		idx = 2;
		break;
	case 0x04:
		idx = 3;
		break;
	}

	return idx;
}

bool edid_find_max_resolution(const struct v4l2_dv_timings *t1,
			const struct v4l2_dv_timings *t2)
{
	if ((t1->bt.width * t1->bt.height < t2->bt.width * t2->bt.height) ||
		((t1->bt.width * t1->bt.height == t2->bt.width * t2->bt.height) &&
		(t1->bt.pixelclock < t2->bt.pixelclock)))
		return true;

	return false;
}

static bool edid_find_preset(const struct fb_videomode *mode, bool first)
{
	int i;

	displayport_dbg("EDID: %dx%d@%dHz\n", mode->xres, mode->yres, mode->refresh);

	for (i = 0; i < displayport_pre_cnt; i++) {
		if ((mode->refresh == displayport_supported_presets[i].refresh ||
			mode->refresh == displayport_supported_presets[i].refresh - 1) &&
			mode->xres == displayport_supported_presets[i].xres &&
			mode->yres == displayport_supported_presets[i].yres &&
			mode->vmode == displayport_supported_presets[i].vmode) {
			if (displayport_supported_presets[i].edid_support_match == false) {
				displayport_info("EDID: found %s\n", displayport_supported_presets[i].name);
				displayport_supported_presets[i].edid_support_match = true;
				preferred_preset = displayport_supported_presets[i].dv_timings;
				first = false;
			}
		}
	}

	return first;
}

static void edid_use_default_preset(void)
{
	int i;

	if (forced_resolution >= 0)
		preferred_preset = displayport_supported_presets[forced_resolution].dv_timings;
	else
		preferred_preset = displayport_supported_presets[EDID_DEFAULT_TIMINGS_IDX].dv_timings;

	for (i = 0; i < displayport_pre_cnt; i++) {
		displayport_supported_presets[i].edid_support_match =
			v4l2_match_dv_timings(&displayport_supported_presets[i].dv_timings,
					&preferred_preset, 0);
	}

	audio_channels = 2;
}

void edid_set_preferred_preset(int mode)
{
	int i;

	preferred_preset = displayport_supported_presets[mode].dv_timings;
	for (i = 0; i < displayport_pre_cnt; i++) {
		displayport_supported_presets[i].edid_support_match =
			v4l2_match_dv_timings(&displayport_supported_presets[i].dv_timings,
					&preferred_preset, 0);
	}
}

int edid_find_resolution(u16 xres, u16 yres, u16 refresh, u16 vmode)
{
	int i;
	int ret=0;

	for (i = 0; i < displayport_pre_cnt; i++) {
		if (refresh == displayport_supported_presets[i].refresh &&
			xres == displayport_supported_presets[i].xres &&
			yres == displayport_supported_presets[i].yres &&
			vmode == displayport_supported_presets[i].vmode) {
			return i;
		}
	}
	return ret;
}

void edid_parse_vsdb(unsigned char *edid_ext_blk, struct fb_vendor *vsdb, int block_cnt)
{
	int i, j;
	int hdmi_vic_len;
	int vsdb_offset_calc = VSDB_VIC_FIELD_OFFSET;

	for (i = 0; i < (block_cnt - 1) * EDID_BLOCK_SIZE; i++) {
		if ((edid_ext_blk[i] & DATA_BLOCK_TAG_CODE_MASK) == (VSDB_TAG_CODE << DATA_BLOCK_TAG_CODE_BIT_POSITION)
				&& edid_ext_blk[i + 1] == IEEE_REGISTRATION_IDENTIFIER_0
				&& edid_ext_blk[i + 2] == IEEE_REGISTRATION_IDENTIFIER_1
				&& edid_ext_blk[i + 3] == IEEE_REGISTRATION_IDENTIFIER_2) {
			displayport_dbg("EDID: find vsdb\n");

			if (edid_ext_blk[i + 8] & VSDB_HDMI_VIDEO_PRESETNT_MASK) {
				displayport_dbg("EDID: Find HDMI_Video_present in VSDB\n");

				if (!(edid_ext_blk[i + 8] & VSDB_LATENCY_FILEDS_PRESETNT_MASK)) {
					vsdb_offset_calc = vsdb_offset_calc - 2;
					displayport_dbg("EDID: Not support LATENCY_FILEDS_PRESETNT in VSDB\n");
				}

				if (!(edid_ext_blk[i + 8] & VSDB_I_LATENCY_FILEDS_PRESETNT_MASK)) {
					vsdb_offset_calc = vsdb_offset_calc - 2;
					displayport_dbg("EDID: Not support I_LATENCY_FILEDS_PRESETNT in VSDB\n");
				}

				hdmi_vic_len = (edid_ext_blk[i + vsdb_offset_calc]
						& VSDB_VIC_LENGTH_MASK) >> VSDB_VIC_LENGTH_BIT_POSITION;

				if (hdmi_vic_len > 0) {
					vsdb->vic_len = hdmi_vic_len;

					for (j = 0; j < hdmi_vic_len; j++)
						vsdb->vic_data[j] = edid_ext_blk[i + vsdb_offset_calc + j + 1];

					break;
				} else {
					vsdb->vic_len = 0;
					displayport_dbg("EDID: No hdmi vic data in VSDB\n");
					break;
				}
			} else
				displayport_dbg("EDID: Not support HDMI_Video_present in VSDB\n");
		}
	}

	if (i >= block_cnt * 128) {
		vsdb->vic_len = 0;
		displayport_dbg("EDID: can't find vsdb block\n");
	}
}

void edid_find_preset_in_video_data_block(u8 vic)
{
	int i;

	for (i = 0; i < displayport_pre_cnt; i++) {
		if ((vic != 0) && (displayport_supported_presets[i].vic == vic))
			displayport_supported_presets[i].edid_support_match = true;
	}
}

static int edid_parse_audio_video_db(unsigned char *edid, struct fb_audio *sad)
{
	int i;
	u8 pos = 4;

	if (!edid)
		return -EINVAL;

	if (edid[0] != 0x2 || edid[1] != 0x3 ||
	    edid[2] < 4 || edid[2] > 128 - DETAILED_TIMING_DESCRIPTION_SIZE)
		return -EINVAL;

	if (!sad)
		return -EINVAL;

	while (pos < edid[2]) {
		u8 len = edid[pos] & DATA_BLOCK_LENGTH_MASK;
		u8 type = (edid[pos] >> DATA_BLOCK_TAG_CODE_BIT_POSITION) & 7;
		displayport_dbg("Data block %u of %u bytes\n", type, len);

		if (len == 0)
			break;

		pos++;
		if (type == AUDIO_DATA_BLOCK) {
			for (i = pos; i < pos + len; i += 3) {
				if (((edid[i] >> 3) & 0xf) != 1)
					continue; /* skip non-lpcm */

				displayport_dbg("LPCM ch=%d\n", (edid[i] & 7) + 1);

				sad->channel_count |= 1 << (edid[i] & 0x7);
				sad->sample_rates |= (edid[i + 1] & 0x7F);
				sad->bit_rates |= (edid[i + 2] & 0x7);

				displayport_dbg("ch:0x%X, sample:0x%X, bitrate:0x%X\n",
					sad->channel_count, sad->sample_rates, sad->bit_rates);
			}
		} else if (type == VIDEO_DATA_BLOCK) {
			for (i = pos; i < pos + len; i++) {
				u8 vic = edid[i] & SVD_VIC_MASK;
				edid_find_preset_in_video_data_block(vic);
				displayport_dbg("EDID: Video data block vic:%d\n", vic);
			}
		} else if (type == SPEAKER_DATA_BLOCK) {
			sad->speaker |= edid[pos] & 0xff;
			displayport_dbg("EDID: speaker 0x%X\n", sad->speaker);
		}

		pos += len;
	}

	return 0;
}

void edid_extension_update(struct fb_vendor *vsdb)
{
	int udmode_idx, vic_idx;

	if (!vsdb)
		return;

	/* find UHD preset in HDMI 1.4 vsdb block*/
	if (vsdb->vic_len) {
		for (vic_idx = 0; vic_idx < vsdb->vic_len; vic_idx++) {
			udmode_idx = get_ud_timing(vsdb, vic_idx);

			displayport_dbg("EDID: udmode_idx = %d\n", udmode_idx);

			if (udmode_idx >= 0)
				edid_find_preset(&ud_mode_h14b_vsdb[udmode_idx], false);
		}
	}
}

int edid_update(struct displayport_device *hdev)
{
	struct fb_monspecs specs;
	struct fb_vendor vsdb;
	struct fb_audio sad;
	bool first = true;
	u8 *edid = NULL;
	int block_cnt = 0;
	int i;
	int basic_audio = 0;

	audio_channels = 0;
	audio_sample_rates = 0;
	audio_bit_rates = 0;
	audio_speaker_alloc = 0;

	edid_misc = 0;
	memset(&vsdb, 0, sizeof(vsdb));
	memset(&specs, 0, sizeof(specs));
	memset(&sad, 0, sizeof(sad));

	block_cnt = edid_read(hdev, &edid);
	if (block_cnt < 0)
		goto out;

	preferred_preset = displayport_supported_presets[EDID_DEFAULT_TIMINGS_IDX].dv_timings;

	for (i = 0; i < displayport_pre_cnt; i++)
		displayport_supported_presets[i].edid_support_match = false;

	fb_edid_to_monspecs(edid, &specs);

	for (i = 1; i < block_cnt; i++)
		fb_edid_add_monspecs(edid + i * EDID_BLOCK_SIZE, &specs);

	/* find 2D preset */
	for (i = 0; i < specs.modedb_len; i++)
		first = edid_find_preset(&specs.modedb[i], first);

	/* color depth */
	if (edid[EDID_COLOR_DEPTH] & 0x80) {
		if (((edid[EDID_COLOR_DEPTH] & 0x70) >> 4) == 1)
			hdev->bpc = BPC_6;
	}

	/* vendor block */
	memcpy(hdev->edid_manufacturer, specs.manufacturer, sizeof(specs.manufacturer));
	hdev->edid_product = specs.model;
	hdev->edid_serial = specs.serial;

	/* number of 128bytes blocks to follow */
	if (block_cnt <= 1)
		goto out;

	if (edid[EDID_NATIVE_FORMAT] & EDID_BASIC_AUDIO) {
		basic_audio = 1;
		edid_misc = FB_MISC_HDMI;
	}

	edid_parse_vsdb(edid + EDID_BLOCK_SIZE, &vsdb, block_cnt);
	edid_extension_update(&vsdb);

	for (i = 1; i < block_cnt; i++)
		edid_parse_audio_video_db(edid + (EDID_BLOCK_SIZE * i), &sad);

	if (!edid_misc)
		edid_misc = specs.misc;

	for (i = 0; i < displayport_pre_cnt; i++)
		displayport_dbg("displayport_supported_presets[%d].edid_support_match = %d\n",
				i, displayport_supported_presets[i].edid_support_match);

	if (edid_misc & FB_MISC_HDMI) {
		audio_speaker_alloc = sad.speaker;
		if (sad.channel_count) {
			audio_channels = sad.channel_count;
			audio_sample_rates = sad.sample_rates;
			audio_bit_rates = sad.bit_rates;
		} else if (basic_audio) {
			audio_channels = 2;
			audio_sample_rates = FB_AUDIO_48KHZ; /*default audio info*/
			audio_bit_rates = FB_AUDIO_16BIT;
		}
	}

	displayport_info("misc:0x%X, Audio ch:0x%X, sf:0x%X, br:0x%X\n",
			edid_misc, audio_channels, audio_sample_rates, audio_bit_rates);

out:
	/* No supported preset found, use default */
	if (forced_resolution >= 0 || first) {
		displayport_info("edid_use_default_preset\n");
		edid_use_default_preset();
		hdev->bpc = BPC_6;
	}

	if (block_cnt == -EPROTO)
		edid_misc = FB_MISC_HDMI;

	kfree(edid);
	return block_cnt;
}

struct v4l2_dv_timings edid_preferred_preset(void)
{
	return preferred_preset;
}

bool edid_supports_hdmi(struct displayport_device *hdev)
{
	return edid_misc & FB_MISC_HDMI;
}

u32 edid_audio_informs(void)
{
	u32 value = 0, ch_info = 0;

	if (audio_channels > 0)
		ch_info = audio_channels;
	if (audio_channels > (1 << 5))
		ch_info |= (1 << 5);

	value = ((audio_sample_rates << 19) | (audio_bit_rates << 16) |
			(audio_speaker_alloc << 8) | ch_info);
	value |= (1 << 26); /* 1: DP, 0: HDMI */

	displayport_info("audio info = 0x%X\n", value);

	return value;
}

u8 edid_read_checksum(void)
{
	int ret, i;
	u8 buf[EDID_BLOCK_SIZE];
	u8 offset = EDID_OFFSET(0);
	int sum = 0;

	ret = displayport_reg_edid_read(offset, EDID_BLOCK_SIZE, buf);
	if (ret)
		return ret;

	for (i = 0; i < EDID_BLOCK_SIZE; i++)
		sum += buf[i];

	displayport_info("edid_read_checksum %02x, %02x", sum%265, buf[EDID_BLOCK_SIZE-1]);

	return buf[EDID_BLOCK_SIZE-1];
}
