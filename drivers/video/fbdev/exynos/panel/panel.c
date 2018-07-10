/*
 * linux/drivers/video/fbdev/exynos/panel/panel.c
 *
 * Samsung Common LCD Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <video/mipi_display.h>
#include <linux/lcd.h>
#include <linux/spi/spi.h>
#include "../dpu/dsim.h"
//#include "../dpu/mipi_panel_drv.h"
#include "panel_drv.h"
#include "panel.h"
#include "panel_bl.h"
//#include "mdnie.h"
#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
#include "spi.h"
#endif

#ifdef CONFIG_PANEL_AID_DIMMING
#include "dimming.h"
#include "panel_dimming.h"
#endif

const char *cmd_type_name[] = {
	[CMD_TYPE_NONE] = "NONE",
	[CMD_TYPE_DELAY] = "DELAY",
	[CMD_TYPE_DELAY_NO_SLEEP] = "DELAY_NO_SLEEP",
	[CMD_TYPE_PINCTL] = "PINCTL",
	[CMD_PKT_TYPE_NONE] = "PKT_NONE",
	[SPI_PKT_TYPE_WR] = "SPI_WR",
	[DSI_PKT_TYPE_WR] = "DSI_WR",
	[DSI_PKT_TYPE_COMP] = "DSI_DSC_WR",
#ifdef CONFIG_ACTIVE_CLOCK
	[DSI_PKT_TYPE_WR_SR] = "DSI_SIDE_RAM_WR",
#endif
	[DSI_PKT_TYPE_PPS] = "DSI_PPS_WR",
	[SPI_PKT_TYPE_RD] = "SPI_RD",
	[DSI_PKT_TYPE_RD] = "DSI_RD",
	[CMD_TYPE_RES] = "RES",
	[CMD_TYPE_SEQ] = "SEQ",
	[CMD_TYPE_KEY] = "KEY",
	[CMD_TYPE_MAP] = "MAP",
	[CMD_TYPE_DMP] = "DUMP",
};

void print_data(char *data, int len)
{
	unsigned char buffer[1024];
	unsigned char *p = buffer;
	int i;
	bool newline = true;

	if (len < 0 || len > 200) {
		pr_warn("%s, out of range (len %d)\n",
				__func__, len);
		return;
	}

	p += sprintf(p, "==================================================\n");
	for (i = 0; i < len; i++) {
		if (newline)
			p += sprintf(p, "[%02Xh]  ", i);
		p += sprintf(p, "0x%02X", data[i]);
		if (!((i + 1) % 8) || (i + 1 == len)) {
			p += sprintf(p, "\n");
			newline = true;
		} else {
			p += sprintf(p, " ");
			newline = false;
		}
	}
	p += sprintf(p, "==================================================\n");
	pr_info("%s", buffer);
}

static int nr_panel;
static struct common_panel_info *panel_list[MAX_PANEL];
int register_common_panel(struct common_panel_info *info)
{
	int i;

	if (unlikely(!info)) {
		pr_err("%s, invalid panel_info\n", __func__);
		return -EINVAL;
	}

	if (unlikely(nr_panel >= MAX_PANEL)) {
		pr_warn("%s, exceed max seqtbl\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < nr_panel; i++) {
		if (!strcmp(panel_list[i]->name, info->name) &&
				panel_list[i]->id == info->id &&
				panel_list[i]->rev == info->rev) {
			pr_warn("%s, already exist panel (%s id-0x%06X rev-%d)\n",
					__func__, info->name, info->id, info->rev);
			return -EINVAL;
		}
	}
	panel_list[nr_panel++] = info;
	pr_info("%s, panel:%s id:0x%06X rev:%d) registered\n",
			__func__, info->name, info->id, info->rev);

	return 0;
}

static struct common_panel_info *find_common_panel_with_name(const char *name)
{
	int i;

	if (!name) {
		panel_err("%s, invalid name\n", __func__);
		return NULL;
	}

	for (i = 0; i < nr_panel; i++) {
		if (!panel_list[i]->name)
			continue;
		if (!strncmp(panel_list[i]->name, name, 128)) {
			pr_info("%s, found panel:%s id:0x%06X rev:%d\n",
					__func__, panel_list[i]->ldi_name,
					panel_list[i]->id, panel_list[i]->rev);
			return panel_list[i];
		}
	}

	return NULL;
}

void print_panel_lut(struct panel_lut_info *lut_info)
{
	int i;

	for (i = 0; i < lut_info->nr_panel; i++)
		  panel_dbg("PANEL:DBG:panel_lut names[%d] %s\n", i, lut_info->names[i]);

	for (i = 0; i < lut_info->nr_lut; i++)
		  panel_dbg("PANEL:DBG:panel_lut[%d] id:0x%08X mask:0x%08X index:%d(%s)\n",
				i, lut_info->lut[i].id, lut_info->lut[i].mask,
				lut_info->lut[i].index, lut_info->names[lut_info->lut[i].index]);
}

int find_panel_lut(struct panel_device *panel, u32 id)
{
	struct panel_lut_info *lut_info = &panel->panel_data.lut_info;
	int i;

	for (i = 0; i < lut_info->nr_lut; i++) {
		if ((lut_info->lut[i].id & lut_info->lut[i].mask)
				== (id & lut_info->lut[i].mask)) {
			panel_info("%s, found %s (id:0x%08X lut[%d].id:0x%08X lut[%d].mask:0x%08X)\n",
					__func__, lut_info->names[lut_info->lut[i].index],
					id, i, lut_info->lut[i].id, i, lut_info->lut[i].mask);
			return lut_info->lut[i].index;
		}
		pr_debug("%s, id:0x%08X lut[%d].id:0x%08X lut[%d].mask:0x%08X (0x%08X 0x%08X) - not same\n",
				__func__, id, i, lut_info->lut[i].id, i, lut_info->lut[i].mask,
				lut_info->lut[i].id & lut_info->lut[i].mask,
				id & lut_info->lut[i].mask);
	}

	panel_err("%s, panel not found!! (id 0x%08X)\n", __func__, id);

	return -ENODEV;
}

struct common_panel_info *find_panel(struct panel_device *panel, u32 id)
{
	struct panel_lut_info *lut_info = &panel->panel_data.lut_info;
	int index;

	index = find_panel_lut(panel, id);
	if (index < 0) {
		panel_err("%s, failed to find panel lookup table\n", __func__);
		return NULL;
	}

	if (index >= lut_info->nr_panel) {
		panel_err("%s, index %d exceeded nr_panel of lut\n", __func__, index);
		return NULL;
	}

	return find_common_panel_with_name(lut_info->names[index]);
}

struct seqinfo *find_panel_seqtbl(struct panel_info *panel_data, char *name)
{
	int i;

	if (unlikely(!panel_data->seqtbl)) {
		panel_err("%s, seqtbl not exist\n", __func__);
		return NULL;
	}

	for (i = 0; i < panel_data->nr_seqtbl; i++) {
		if (unlikely(!panel_data->seqtbl[i].name))
			continue;
		if (!strcmp(panel_data->seqtbl[i].name, name)) {
			panel_dbg("%s, found %s panel seqtbl\n", __func__, name);
			return &panel_data->seqtbl[i];
		}
	}
	return NULL;
}


struct seqinfo *find_index_seqtbl(struct panel_info *panel_data, u32 index)
{
	struct seqinfo *tbl;

	if (unlikely(!panel_data->seqtbl)) {
		panel_err("%s, seqtbl not exist\n", __func__);
		return NULL;
	}

	if (unlikely(index >= MAX_PANEL_SEQ)) {
		panel_err("%s, invalid paramter (index %d)\n",
				__func__, index);
		return NULL;
	}

	tbl = &panel_data->seqtbl[index];
	if (tbl != NULL)
		panel_dbg("%s, found %s panel seqtbl\n", __func__, tbl->name);

	return tbl;

}


struct pktinfo *alloc_static_packet(char *name, u32 type, u8 *data, u32 dlen)
{
	struct pktinfo *info = kzalloc(sizeof(struct pktinfo), GFP_KERNEL);

	info->name = name;
	info->type = type;
	info->data = data;
	info->dlen = dlen;

	return info;
}

struct pktinfo *find_packet(struct seqinfo *seqtbl, char *name)
{
	int i;
	void **cmdtbl;

	if (unlikely(!seqtbl || !name)) {
		pr_err("%s, invalid paramter (seqtbl %p, name %p)\n",
				__func__, seqtbl, name);
		return NULL;
	}

	cmdtbl = seqtbl->cmdtbl;
	if (unlikely(!cmdtbl)) {
		pr_err("%s, invalid command table\n", __func__);
		return NULL;
	}

	for (i = 0; i < seqtbl->size; i++) {
		if (cmdtbl[i] == NULL) {
			pr_debug("%s, end of cmdtbl %d\n", __func__, i);
			break;
		}
		if (!strcmp(((struct cmdinfo *)cmdtbl[i])->name, name))
			return cmdtbl[i];
	}

	return NULL;
}


struct pktinfo *find_packet_suffix(struct seqinfo *seqtbl, char *name)
{
	int i, j, len;

	void **cmdtbl;

	if (unlikely(!seqtbl || !name)) {
		pr_err("%s, invalid paramter (seqtbl %p, name %p)\n",
				__func__, seqtbl, name);
		return NULL;
	}

	cmdtbl = seqtbl->cmdtbl;
	if (unlikely(!cmdtbl)) {
		pr_err("%s, invalid command table\n", __func__);
		return NULL;
	}

	for (i = 0; i < seqtbl->size; i++) {
		if (cmdtbl[i] == NULL) {
			pr_debug("%s, end of cmdtbl %d\n", __func__, i);
			break;
		}
		len = strlen(((struct cmdinfo *)cmdtbl[i])->name);
		for (j = 0; j < len; j++) {
			if (!strcmp(&((struct cmdinfo *)cmdtbl[i])->name[j], name))
				return cmdtbl[i];
		}
	}

	return NULL;
}

struct maptbl *find_panel_maptbl_by_index(struct panel_info *panel, int index)
{
	if (unlikely(!panel || !panel->maptbl)) {
		pr_err("%s, maptbl not exist\n", __func__);
		return NULL;
	}

	if (index < 0 || index >= panel->nr_maptbl) {
		pr_err("%s, out of range (index %d)\n", __func__, index);
		return NULL;
	}

	return &panel->maptbl[index];
}

struct maptbl *find_panel_maptbl_by_name(struct panel_info *panel_data, char *name)
{
	int i = 0;

	if (unlikely(!panel_data || !panel_data->maptbl)) {
		pr_err("%s, maptbl not exist\n", __func__);
		return NULL;
	}

	for (i = 0; i < panel_data->nr_maptbl; i++)
		if (strstr(panel_data->maptbl[i].name, name))
			return &panel_data->maptbl[i];
	return NULL;
}

struct panel_dimming_info *
find_panel_dimming(struct panel_info *panel_data, char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(panel_data->panel_dim_info); i++) {
		if (!strcmp(panel_data->panel_dim_info[i]->name, name)) {
			panel_dbg("%s, found %s panel dimming\n", __func__, name);
			return panel_data->panel_dim_info[i];
		}
	}
	return NULL;
}

u32 maptbl_index(struct maptbl *tbl, int layer, int row, int col)
{
	return (tbl->nrow * tbl->ncol * layer) + (tbl->ncol * row) + col;
}

int maptbl_init(struct maptbl *tbl)
{
	if (tbl && tbl->ops.init)
		return tbl->ops.init(tbl);
	return 0;
}

int maptbl_getidx(struct maptbl *tbl)
{
	if (tbl && tbl->ops.getidx)
		return tbl->ops.getidx(tbl);
	return -EINVAL;
}

u8 *maptbl_getptr(struct maptbl *tbl)
{
	int index = maptbl_getidx(tbl);
	if (unlikely(index < 0)) {
		pr_err("%s, failed to get index\n", __func__);
		return NULL;
	}

	if (unlikely(!tbl->arr)) {
		pr_err("%s, failed to get pointer\n", __func__);
		return NULL;
	}
	return &tbl->arr[index];
}

void maptbl_copy(struct maptbl *tbl, u8 *dst)
{
	if (tbl && dst && tbl->ops.copy)
		tbl->ops.copy(tbl, dst);
}

struct maptbl *maptbl_find(struct panel_info *panel, char *name)
{
	int i;

	if (unlikely(!panel || !name)) {
		pr_err("%s, invalid parameter\n", __func__);
		return NULL;
	}

	if (unlikely(!panel->maptbl)) {
		pr_err("%s, maptbl not exist\n", __func__);
		return NULL;
	}

	for (i = 0; i < panel->nr_maptbl; i++)
		if (!strcmp(name, panel->maptbl[i].name))
			return &panel->maptbl[i];
	return NULL;
}

int pktui_maptbl_getidx(struct pkt_update_info *pktui)
{
	if (pktui && pktui->getidx)
		return pktui->getidx(pktui);
	return -EINVAL;
}

void panel_update_packet_data(struct panel_device *panel, struct pktinfo *info)
{
	struct pkt_update_info *pktui;
	struct maptbl *maptbl;
	int i, idx, offset;

	if (!info || !info->data || !info->pktui || !info->nr_pktui) {
		pr_warn("%s, invalid pkt update info\n", __func__);
		return;
	}

	pktui = info->pktui;
	/*
	 * TODO : move out pktui->pdata initial code
	 * panel_pkt_update_info_init();
	 */
	for (i = 0; i < info->nr_pktui; i++) {
		pktui[i].pdata = panel;
		offset = pktui[i].offset;
		if (pktui[i].nr_maptbl > 1 && pktui[i].getidx) {
			idx = pktui_maptbl_getidx(&pktui[i]);
			if (unlikely(idx < 0)) {
				pr_err("%s, failed to pktui_maptbl_getidx %d\n", __func__, idx);
				return;
			}
			maptbl = &pktui[i].maptbl[idx];
		} else {
			maptbl = pktui[i].maptbl;
		}
		if (unlikely(!maptbl)) {
			pr_err("%s, invalid info (offset %d, maptbl %p)\n",
					__func__, pktui[i].offset, pktui[i].maptbl);
			return;
		}
		if (unlikely(offset + maptbl->sz_copy > info->dlen)) {
			pr_err("%s, out of range (offset %d, sz_copy %d, dlen %d)\n",
					__func__, offset, maptbl->sz_copy, info->dlen);
			return;
		}
		maptbl_copy(maptbl, &info->data[offset]);
#ifdef DEBUG_PANEL
		pr_info("%s, copy %d bytes from %s to %s[%d]\n",
				__func__, maptbl->sz_copy, maptbl->name, info->name, offset);
#endif
	}
}

static int panel_do_delay(struct panel_device *panel, struct delayinfo *info)
{
	if (unlikely(!panel || !info)) {
		panel_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	usleep_range(info->usec, info->usec + 10);
	return 0;
}

static int panel_do_delay_no_sleep(struct panel_device *panel, struct delayinfo *info)
{
	if (unlikely(!panel || !info)) {
		panel_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	udelay(info->usec);
	return 0;
}

static int panel_do_pinctl(struct panel_device *panel, struct pininfo *info)
{
	if (unlikely(!panel || !info)) {
		pr_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	if (info->pin) {
		// TODO : control external pin
#ifdef DEBUG_PANEL
		pr_info("%s, set %d pin %s\n",
				__func__, info->pin, info->onoff ? "on" : "off");
#endif
	}

	return 0;
}

int panel_set_key(struct panel_device *panel, int level, bool on)
{
	int id;
	int ret = 0;
	struct mipi_drv_ops *mipi_ops;
	static const unsigned char SEQ_TEST_KEY_ON_9F[] = { 0x9F, 0xA5, 0xA5 };
	static const unsigned char SEQ_TEST_KEY_ON_F0[] = { 0xF0, 0x5A, 0x5A };
	static const unsigned char SEQ_TEST_KEY_ON_FC[] = { 0xFC, 0x5A, 0x5A };
	static const unsigned char SEQ_TEST_KEY_OFF_9F[] = { 0x9F, 0x5A, 0x5A };
	static const unsigned char SEQ_TEST_KEY_OFF_F0[] = { 0xF0, 0xA5, 0xA5 };
	static const unsigned char SEQ_TEST_KEY_OFF_FC[] = { 0xFC, 0xA5, 0xA5 };

	if (unlikely(!panel)) {
		panel_err("ERR:PANEL:%s: panel is null\n", __func__);
		return -EINVAL;
	}

	id = panel->dsi_id;
	mipi_ops = &panel->mipi_drv;

	if (on) {
		if (level >= 1) {
			ret = mipi_ops->write(id, MIPI_DSI_WRITE, SEQ_TEST_KEY_ON_9F, ARRAY_SIZE(SEQ_TEST_KEY_ON_9F));
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_9F)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_9F\n", __func__);
				return -EIO;
			}
		}

		if (level >= 2) {
			ret = mipi_ops->write(id, MIPI_DSI_WRITE, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
				return -EIO;
			}
		}

		if (level >= 3) {
			ret = mipi_ops->write(id, MIPI_DSI_WRITE, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_FC)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
				return -EIO;
			}
		}
	} else {
		if (level >= 3) {
			ret = mipi_ops->write(id, MIPI_DSI_WRITE, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_FC)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
				return -EIO;
			}
		}

		if (level >= 2) {
			ret = mipi_ops->write(id, MIPI_DSI_WRITE, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
				return -EIO;
			}
		}

		if (level >= 1) {
			ret = mipi_ops->write(id, MIPI_DSI_WRITE, SEQ_TEST_KEY_OFF_9F, ARRAY_SIZE(SEQ_TEST_KEY_OFF_9F));
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_9F)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_9F\n", __func__);
				return -EIO;
			}
		}
	}

	return 0;
}

int panel_verify_tx_packet(struct panel_device *panel, u8 *table, u8 len)
{
	u8 *buf;
	int i, ret = 0;
	unsigned char addr = table[0];
	unsigned char *data = table + 1;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		return 0;
	}

	buf = kzalloc(sizeof(u8) * len, GFP_KERNEL);
	if (!buf) {
		pr_err("%s, failed to alloc memory\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&panel->op_lock);
	panel_rx_nbytes(panel, DSI_PKT_TYPE_RD, buf, addr, 0, len - 1);
	for (i = 0; i < len - 1; i++) {
		if (buf[i] != data[i]) {
			pr_warn("%02Xh[%2d] - (tx 0x%02X, rx 0x%02X) not match\n",
					i, addr, data[i], buf[i]);
			ret = -EINVAL;
		} else {
			pr_info("%02Xh[%2d] - (tx 0x%02X, rx 0x%02X) match\n",
					i, addr, data[i], buf[i]);
		}
	}
	mutex_unlock(&panel->op_lock);
	kfree(buf);
	return ret;
}

static int panel_do_tx_packet(struct panel_device *panel, struct pktinfo *info)
{
	int id;
	int ret;
	u32 type;
	struct mipi_drv_ops *mipi_ops;

	if (unlikely(!panel || !info)) {
		panel_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	id = panel->dsi_id;
	mipi_ops = &panel->mipi_drv;

	if (unlikely(!mipi_ops)) {
		panel_err("PANE:ERR:%s:mipi ops is null\n", __func__);
		return -EINVAL;
	}

	if (info->pktui) {
		panel_update_packet_data(panel, info);
	}

	type = info->type;
	switch (type) {
		case CMD_PKT_TYPE_NONE:
			break;
		case DSI_PKT_TYPE_COMP:
			ret = mipi_ops->write(id, MIPI_DSI_DSC_PRA, info->data, info->dlen);
			if (ret != info->dlen) {
				panel_err("%s, failed to send packet %s (ret %d)\n",
						__func__, info->name, ret);
				return -EINVAL;
			}
			break;
		case DSI_PKT_TYPE_PPS:
			ret = mipi_ops->write(id, MIPI_DSI_DSC_PPS, info->data, info->dlen);
			if (ret != info->dlen) {
				panel_err("%s, failed to send packet %s (ret %d)\n",
						__func__, info->name, ret);
				return -EINVAL;
			}
			break;
		case DSI_PKT_TYPE_WR_MEM:
			ret = mipi_ops->write(id, MIPI_DSI_WR_MEM, info->data, info->dlen);
			if (ret != info->dlen) {
				panel_err("%s, failed to send packet %s (ret %d)\n",
						__func__, info->name, ret);
				return -EINVAL;
			}
			break;
		case DSI_PKT_TYPE_WR:
#ifdef DEBUG_PANEL
			panel_dbg("%s, send packet %s - start\n", __func__, info->name);
#endif
			ret = mipi_ops->write(id, MIPI_DSI_WRITE, info->data, info->dlen);
			if (ret != info->dlen) {
				panel_err("%s, failed to send packet %s (ret %d)\n",
						__func__, info->name, ret);
				return -EINVAL;
			}
#ifdef DEBUG_PANEL
			panel_dbg("%s, send packet %s - end\n", __func__, info->name);
#endif
			break;
		case DSI_PKT_TYPE_RD:
			break;
#ifdef CONFIG_ACTIVE_CLOCK
		case DSI_PKT_TYPE_WR_SR:
			ret = mipi_ops->write(id, MIPI_DSI_OEM1_WR_SIDE_RAM, info->data, info->dlen);
			if (ret != info->dlen) {
				panel_err("%s, failed to send packet %s (ret %d)\n",
						__func__, info->name, ret);
				return -EINVAL;
			}
			break;
#endif
		default:
			panel_warn("%s, unknown pakcet type %d\n", __func__, type);
			break;
	}

	return 0;
}

static int panel_do_setkey(struct panel_device *panel, struct keyinfo *info)
{
	struct panel_info *panel_data;
	bool send_packet = false;
	int ret = 0;

	if (unlikely(!panel || !info)) {
		panel_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	if (info->level < 0 || info->level >= MAX_CMD_LEVEL) {
		panel_err("%s, invalid level %d\n", __func__, info->level);
		return -EINVAL;
	}

	if (info->en != KEY_ENABLE && info->en != KEY_DISABLE) {
		panel_err("%s, invalid key type %d\n", __func__, info->en);
		return -EINVAL;
	}

	panel_data = &panel->panel_data;
	if (info->en == KEY_ENABLE) {
		if (panel_data->props.key[info->level] == 0)
			send_packet = true;
		panel_data->props.key[info->level] += 1;
	} else if (info->en == KEY_DISABLE) {
		if (panel_data->props.key[info->level] < 1) {
			panel_err("%s unbalanced key [%d]%s(%d)\n", __func__, info->level,
			(panel_data->props.key[info->level] > 0 ? "ENABLED" : "DISABLED"),
			panel_data->props.key[info->level]);
			return -EINVAL;
		}

		if (panel_data->props.key[info->level] == 1)
			send_packet = true;
		panel_data->props.key[info->level] -= 1;
	}

	if (send_packet) {
		ret = panel_do_tx_packet(panel, info->packet);
		if (ret < 0) {
			panel_err("%s failed %s\n", __func__,
					info->packet->name);
		}
	}

#ifdef DEBUG_PANEL
	panel_dbg("%s key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n", __func__,
			(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
			panel_data->props.key[CMD_LEVEL_1],
			(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
			panel_data->props.key[CMD_LEVEL_2],
			(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
			panel_data->props.key[CMD_LEVEL_3]);
#endif

	return 0;
}

int panel_do_init_maptbl(struct panel_device *panel, struct maptbl *maptbl)
{
	int ret;

	if (unlikely(!panel || !maptbl)) {
		panel_err("%s, invalid paramter (panel %p, maptbl %p)\n",
				__func__, panel, maptbl);
		return -EINVAL;
	}

	ret = maptbl_init(maptbl);
	if (ret < 0) {
		panel_err("%s failed to init maptbl(%s) ret %d\n",
				__func__, maptbl->name, ret);
		return ret;
	}

	return 0;
}

int panel_do_seqtbl(struct panel_device *panel, struct seqinfo *seqtbl)
{
	int i, ret;
	u32 type;
	void **cmdtbl;

	if (unlikely(!panel || !seqtbl)) {
		pr_err("%s, invalid paramter (panel %p, seqtbl %p)\n",
				__func__, panel, seqtbl);
		return -EINVAL;
	}

	cmdtbl = seqtbl->cmdtbl;
	if (unlikely(!cmdtbl)) {
		pr_err("%s, invalid command table\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < seqtbl->size; i++) {
		if (cmdtbl[i] == NULL) {
			pr_debug("%s, end of cmdtbl %d\n", __func__, i);
			break;
		}
		type = *((u32 *)cmdtbl[i]);

		if (type >= MAX_CMD_TYPE) {
			pr_warn("%s invalid cmd type %d\n", __func__, type);
			break;
		}

#ifdef DEBUG_PANEL
		pr_info("SEQ: %s %s %s+\n", seqtbl->name, cmd_type_name[type],
				(((struct cmdinfo *)cmdtbl[i])->name ?
				((struct cmdinfo *)cmdtbl[i])->name : "none"));
#endif
		switch (type) {
			case CMD_TYPE_KEY:
				ret = panel_do_setkey(panel, (struct keyinfo *)cmdtbl[i]);
				break;
			case CMD_TYPE_DELAY:
				ret = panel_do_delay(panel , (struct delayinfo *)cmdtbl[i]);
				break;
			case CMD_TYPE_DELAY_NO_SLEEP:
				ret = panel_do_delay_no_sleep(panel , (struct delayinfo *)cmdtbl[i]);
				break;
			case CMD_TYPE_PINCTL:
				ret = panel_do_pinctl(panel, (struct pininfo *)cmdtbl[i]);
				break;
			case CMD_TYPE_TX_PKT_START ... CMD_TYPE_TX_PKT_END:
				ret = panel_do_tx_packet(panel, (struct pktinfo *)cmdtbl[i]);
				break;
			case CMD_TYPE_RES:
				ret = panel_resource_update(panel, (struct resinfo *)cmdtbl[i]);
				break;
			case CMD_TYPE_SEQ:
				ret = panel_do_seqtbl(panel, (struct seqinfo *)cmdtbl[i]);
				break;
			case CMD_TYPE_MAP:
				ret = panel_do_init_maptbl(panel, (struct maptbl *)cmdtbl[i]);
				break;
			case CMD_TYPE_DMP:
				ret = panel_dumpinfo_update(panel, (struct dumpinfo *)cmdtbl[i]);
				break;
			case CMD_TYPE_NONE:
			default:
				pr_warn("%s, unknown pakcet type %d\n", __func__, type);
				break;
		}

#ifdef CONFIG_SUPPORT_PANEL_SWAP
		if (ret < 0) {
			pr_err("%s, failed to do_seq %s %s %s\n", __func__,
					seqtbl->name, cmd_type_name[type],
					(((struct cmdinfo *)cmdtbl[i])->name ?
					 ((struct cmdinfo *)cmdtbl[i])->name : "none"));
			return ret;
		}
#endif

#ifdef DEBUG_PANEL
		pr_info("SEQ: %s %s %s-\n", seqtbl->name, cmd_type_name[type],
				(((struct cmdinfo *)cmdtbl[i])->name ?
				((struct cmdinfo *)cmdtbl[i])->name : "none"));
#endif
	}

	return 0;
}


int panel_do_seqtbl_by_index(struct panel_device *panel, int index)
{
	int ret;
	struct seqinfo *tbl;
	struct panel_info *panel_data;
	struct timespec cur_ts, last_ts, delta_ts;
	s64 elapsed_usec;

	if (panel == NULL) {
		panel_err("ERR:PANEL:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		return 0;
	}

	panel_data = &panel->panel_data;
	tbl = panel_data->seqtbl;
	ktime_get_ts(&cur_ts);

	mutex_lock(&panel->op_lock);
	ktime_get_ts(&last_ts);

	if (unlikely(index < 0 || index >= MAX_PANEL_SEQ)) {
		panel_err("%s, invalid paramter (panel %p, index %d)\n",
				__func__, panel, index);
		ret = -EINVAL;
		goto do_exit;
	}

	delta_ts = timespec_sub(last_ts, cur_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	if (elapsed_usec > 34000)
		pr_debug("seq:%s warn:elapsed %lld.%03lld msec to acquire lock\n",
				tbl[index].name, elapsed_usec / 1000, elapsed_usec % 1000);

	if (panel_data->props.key[CMD_LEVEL_1] != 0 ||
			panel_data->props.key[CMD_LEVEL_2] != 0 ||
			panel_data->props.key[CMD_LEVEL_3] != 0) {
		panel_warn("%s before seq:%s unbalanced key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n",
				__func__, tbl[index].name,
				(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_1],
				(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_2],
				(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_3]);
		panel_set_key(panel, 3, false);
		panel_data->props.key[CMD_LEVEL_1] = 0;
		panel_data->props.key[CMD_LEVEL_2] = 0;
		panel_data->props.key[CMD_LEVEL_3] = 0;
	}

	ret = panel_do_seqtbl(panel, &tbl[index]);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to excute seqtbl %s\n",
				__func__, tbl->name);
		ret = -EIO;
		goto do_exit;
	}

do_exit:

	if (panel_data->props.key[CMD_LEVEL_1] != 0 ||
			panel_data->props.key[CMD_LEVEL_2] != 0 ||
			panel_data->props.key[CMD_LEVEL_3] != 0) {
		panel_warn("%s after seq:%s unbalanced key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n",
				__func__, tbl[index].name,
				(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_1],
				(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_2],
				(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_3]);
		panel_set_key(panel, 3, false);
		panel_data->props.key[CMD_LEVEL_1] = 0;
		panel_data->props.key[CMD_LEVEL_2] = 0;
		panel_data->props.key[CMD_LEVEL_3] = 0;
	}

	mutex_unlock(&panel->op_lock);

	ktime_get_ts(&last_ts);
	delta_ts = timespec_sub(last_ts, cur_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	pr_debug("seq:%s done (elapsed %2lld.%03lld msec)\n",
			tbl[index].name, elapsed_usec / 1000, elapsed_usec % 1000);

	return 0;
}


int panel_do_seqtbl_by_name(struct panel_device *panel, char *name)
{
	int ret;
	struct seqinfo *tbl;
	struct panel_info *panel_data;

	if (unlikely(!panel || !name)) {
		panel_err("%s, invalid paramter (panel %p, name %p)\n",
				__func__, panel, name);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	if (!IS_PANEL_ACTIVE(panel)) {
		return 0;
	}

	if (unlikely(!panel_data)) {
		panel_err("%s, invalid paramter (panel %p, name %p)\n",
				__func__, panel, name);
		return -EINVAL;
	}

	mutex_lock(&panel->op_lock);
	tbl = panel_data->seqtbl;

#ifdef DEBUG_PANEL
	panel_dbg("%s, %s start\n", __func__, name);
#endif

	/*
	 * TODO : improve find seqtbl.
	 * 1. use seqtbl index
	 * 2. hash table
	 */
	tbl = find_panel_seqtbl(panel_data, name);
	if (unlikely(!tbl)) {
		panel_err("%s, failed to find seqtbl %s\n", __func__, name);
		ret = -EINVAL;
		goto do_exit;
	}
	ret = panel_do_seqtbl(panel, tbl);
	if (unlikely(ret < 0)) {
		panel_err("%s, failed to excute seqtbl %s\n",
				__func__, tbl->name);
		ret = -EIO;
		goto do_exit;
	}

do_exit:
	mutex_unlock(&panel->op_lock);
#ifdef DEBUG_PANEL
	panel_dbg("%s, %s end\n", __func__, name);
#endif
	return 0;
}

struct resinfo *find_panel_resource(struct panel_info *panel_data, char *name)
{
	int i = 0;

	if (unlikely(!panel_data->restbl))
		return NULL;

	for (i = 0; i < panel_data->nr_restbl; i++)
		if (!strcmp(name, panel_data->restbl[i].name))
			return &panel_data->restbl[i];
	return NULL;
}

int rescpy(u8 *dst, struct resinfo *res, u32 offset, u32 len)
{
	if (unlikely(!dst || !res)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (unlikely(offset + len > res->dlen)) {
		pr_err("%s, exceed range (offset %d, len %d, res->dlen %d\n",
				__func__, offset, len, res->dlen);
		return -EINVAL;
	}

	memcpy(dst, &res->data[offset], len);
	return 0;
}


int rescpy_by_name(struct panel_info *panel_data, u8 *dst, char *name, u32 offset, u32 len)
{
	struct resinfo *res;

	if (unlikely(!panel_data || !dst || !name)) {
		panel_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	res = find_panel_resource(panel_data, name);
	if (unlikely(!res)) {
		panel_err("%s, %s not found in resource\n",
				__func__, name);
		return -EINVAL;
	}

	if (unlikely(offset + len > res->dlen)) {
		panel_err("%s, exceed range (offset %d, len %d, res->dlen %d\n",
				__func__, offset, len, res->dlen);
		return -EINVAL;
	}
	memcpy(dst, &res->data[offset], len);
	return 0;
}

int resource_copy(u8 *dst, struct resinfo *res)
{
	if (unlikely(!dst || !res)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (res->state == RES_UNINITIALIZED) {
		pr_warn("%s, %s not initialized\n",
				__func__, res->name);
		return -EINVAL;
	}
	return rescpy(dst, res, 0, res->dlen);
}

int resource_copy_by_name(struct panel_info *panel_data, u8 *dst, char *name)
{
	struct resinfo *res;

	if (unlikely(!panel_data || !dst || !name)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	res = find_panel_resource(panel_data, name);
	if (unlikely(!res)) {
		pr_warn("%s, %s not found in resource\n",
				__func__, name);
		return -EINVAL;
	}

	if (res->state == RES_UNINITIALIZED) {
		pr_warn("%s, %s not initialized\n",
				__func__, res->name);
		return -EINVAL;
	}
	return rescpy(dst, res, 0, res->dlen);
}

#define MAX_READ_BYTES  (40)
int panel_rx_nbytes(struct panel_device *panel,
		u32 type, u8 *buf, u8 addr, u8 pos, u8 len)
{
	int id;
	int ret, read_len, tlen = len;
	static char gpara[] = {0xB0, 0x00};
	struct mipi_drv_ops *mipi_ops;
	u8 *p = buf;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	id = panel->dsi_id;
	mipi_ops = &panel->mipi_drv;

	gpara[1] = pos;

	while (tlen > 0) {
		read_len = tlen < MAX_READ_BYTES ? tlen : MAX_READ_BYTES;
		if (gpara[1] != 0) {
			ret = mipi_ops->write(id, MIPI_DSI_WRITE, gpara, ARRAY_SIZE(gpara));
			if (ret != ARRAY_SIZE(gpara))
				panel_err("%s, failed to set gpara %d (ret %d)\n",
						__func__, gpara[1], ret);
			else
				pr_debug("%s, set gpara %d\n", __func__, gpara[1]);
		}

		if (type == DSI_PKT_TYPE_RD) {
			ret = mipi_ops->read(id, addr, p, read_len);
#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
		} else if (type == SPI_PKT_TYPE_RD) {
			ret = panel_spi_read_data(panel->spi, addr, p, read_len);
#endif
		} else {
			pr_err("%s, unkown packet type %d\n", __func__, type);
			return -EINVAL;
		}
		if (ret != read_len) {
			panel_err("%s, failed to read addr:0x%02X, pos:%d, len:%u ret %d\n",
					__func__, addr, pos, len, ret);
			return -EINVAL;
		}
		p += read_len;
		gpara[1] += read_len;
		tlen -= read_len;
	}

#ifdef DEBUG_PANEL
	panel_dbg("%s, addr:%2Xh, pos:%d, len:%u\n", __func__, addr, pos, len);
	print_data(buf, len);
#endif
	return len;
}

int read_panel_id(struct panel_device *panel, u8 *buf)
{
	int len, ret = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		return -ENODEV;
	}

	mutex_lock(&panel->op_lock);
	panel_set_key(panel, 3, true);
	len = panel_rx_nbytes(panel, DSI_PKT_TYPE_RD, buf, PANEL_ID_REG, 0, 3);
	if (len != 3) {
		pr_err("%s, failed to read id\n", __func__);
		ret = -EINVAL;
		goto read_err;
	}

read_err:
	panel_set_key(panel, 3, false);
	mutex_unlock(&panel->op_lock);
	return ret;
}

static struct rdinfo *find_panel_rdinfo(struct panel_info *panel_data, char *name)
{
	int i;

	for (i = 0; i < panel_data->nr_rditbl; i++)
		if (!strcmp(name, panel_data->rditbl[i].name))
			return &panel_data->rditbl[i];

	return NULL;
}

int panel_rdinfo_update(struct panel_device *panel, struct rdinfo *rdi)
{
	int ret;

	if (unlikely(!panel || !rdi)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}
	ret = panel_rx_nbytes(panel, rdi->type, rdi->data, rdi->addr, rdi->offset, rdi->len);

	if (unlikely(ret != rdi->len)) {
		pr_err("%s, read failed\n", __func__);
		return -EIO;
	}

	return 0;
}

int panel_rdinfo_update_by_name(struct panel_device *panel, char *name)
{
	int ret;
	struct rdinfo *rdi;
	struct panel_info *panel_data;

	if (unlikely(!panel || !name)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rdi = find_panel_rdinfo(panel_data, name);
	if (unlikely(!rdi)) {
		pr_err("%s, read info %s not found\n", __func__, name);
		return -ENODEV;
	}

	ret = panel_rdinfo_update(panel, rdi);
	if (ret) {
		pr_err("%s, failed to update read info\n", __func__);
		return -EINVAL;
	}

	return 0;
}

void print_panel_resource(struct panel_device *panel)
{
	struct panel_info *panel_data;
	int i;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return;
	}

	panel_data = &panel->panel_data;
	if (unlikely(!panel_data)) {
		panel_err("PANEL:ERR:%s:panel_data is null\n", __func__);
		return;
	}

	for (i = 0; i < panel_data->nr_restbl; i++) {
		panel_info("[panel_resource : %12s %3d bytes] %s\n",
				panel_data->restbl[i].name, panel_data->restbl[i].dlen,
				(panel_data->restbl[i].state == RES_UNINITIALIZED ?
				 "UNINITIALIZED" : "INITIALIZED"));
		if (panel_data->restbl[i].state == RES_INITIALIZED)
			print_data(panel_data->restbl[i].data,
					panel_data->restbl[i].dlen);
	}
}

int panel_resource_update(struct panel_device *panel, struct resinfo *res)
{
	int i, ret;
	struct rdinfo *rdi;

	if (unlikely(!panel || !res)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < res->nr_resui; i++) {
		rdi = res->resui[i].rditbl;
		if (unlikely(!rdi)) {
			panel_warn("%s read info not exist\n", __func__);
			return -EINVAL;
		}
		ret = panel_rdinfo_update(panel, rdi);
		if (ret) {
			pr_err("%s, failed to update read info\n", __func__);
			return -EINVAL;
		}

		if (!res->data) {
			panel_warn("%s resource data not exist\n", __func__);
			return -EINVAL;
		}
		memcpy(res->data + res->resui[i].offset, rdi->data, rdi->len);
		res->state = RES_INITIALIZED;
	}

#ifdef DEBUG_PANEL
	panel_info("[panel_resource : %12s %3d bytes] %s\n",
			res->name, res->dlen, (res->state == RES_UNINITIALIZED ?
				"UNINITIALIZED" : "INITIALIZED"));
	if (res->state == RES_INITIALIZED)
		print_data(res->data, res->dlen);
#endif

	return 0;
}

int panel_resource_update_by_name(struct panel_device *panel, char *name)
{
	int ret;
	struct resinfo *res;
	struct panel_info *panel_data;

	if (unlikely(!panel || !name)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	res = find_panel_resource(panel_data, name);
	if (unlikely(!res)) {
		pr_warn("%s, %s not found in resource\n",
				__func__, name);
		return -EINVAL;
	}

	ret = panel_resource_update(panel, res);
	if (unlikely(ret)) {
		pr_err("%s, failed to update resource\n", __func__);
		return ret;
	}

	return 0;
}

int panel_dumpinfo_update(struct panel_device *panel, struct dumpinfo *info)
{
	int ret;

	if (unlikely(!panel || !info || !info->res || !info->callback)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	ret = panel_resource_update(panel, info->res);
	if (unlikely(ret)) {
		pr_err("%s, failed to update resource\n", __func__);
		return ret;
	}
	info->callback(info);

	return 0;
}

int check_panel_active(struct panel_device *panel, const char *caller)
{
	struct panel_state *state;
	struct mipi_drv_ops *mipi_drv;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", caller);
		return 0;
	}

	state = &panel->state;
	mipi_drv = &panel->mipi_drv;

	if ((mipi_drv->get_state) &&
		(mipi_drv->get_state(panel->dsi_id) == DSIM_STATE_OFF)) {
		panel_err("PANEL:ERR:%s:dsim off\n", caller);
		return 0;
	}

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_err("PANEL:ERR:%s:panel was not connected\n", caller);
		return 0;
	}

	return 1;
}
