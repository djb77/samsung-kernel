/*
 *  Copyright (C) 2010,Imagis Technology Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/stat.h>
#include <asm/unaligned.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/err.h>

#include "ist30xxh.h"
#include "ist30xxh_update.h"
#include "ist30xxh_misc.h"

#define TSP_CH_UNUSED               (0)
#define TSP_CH_SCREEN               (1)
#define TSP_CH_GTX                  (2)
#define TSP_CH_KEY                  (3)
#define TSP_CH_UNKNOWN              (-1)

static u32 ist30xx_frame_buf[IST30XX_MAX_NODE_NUM];
static u32 ist30xx_frame_cdcbuf[IST30XX_MAX_NODE_NUM];
static u32 ist30xx_frame_self_cdcbuf[IST30XX_MAX_SELF_NODE_NUM];
static u32 ist30xx_frame_cpbuf[IST30XX_MAX_NODE_NUM];
static u32 ist30xx_frame_self_cpbuf[IST30XX_MAX_SELF_NODE_NUM];
int ist30xx_check_valid_ch(struct ist30xx_data *data, int ch_tx, int ch_rx)
{
    int i;
    TKEY_INFO *tkey = &data->tkey_info;
    TSP_INFO *tsp = &data->tsp_info;

    if (unlikely((ch_tx >= tsp->ch_num.tx) || (ch_rx >= tsp->ch_num.rx)))
        return TSP_CH_UNKNOWN;

    if ((ch_tx >= tsp->screen.tx) || (ch_rx >= tsp->screen.rx)) {
        if (tsp->gtx.num) {
            for (i = 0; i < tsp->gtx.num; i++) {
                if ((ch_tx == tsp->gtx.ch_num[i]) && (ch_rx < tsp->screen.rx))
                    return TSP_CH_GTX;
            }
        }

        if (tkey->enable) {
            for (i = 0; i < tkey->key_num; i++) {
                if ((ch_tx == tkey->ch_num[i].tx) &&
                    (ch_rx == tkey->ch_num[i].rx))
                    return TSP_CH_KEY;
            }
        }
    } else {
        return TSP_CH_SCREEN;
    }

    return TSP_CH_UNUSED;
}

int ist30xx_parse_touch_node(struct ist30xx_data *data,
        struct TSP_NODE_BUF *node)
{
    int i;
    u16 *cdc = (u16 *)&node->cdc;
    u16 *self_cdc = (u16 *)&node->self_cdc;
    u16 *base = (u16 *)&node->base;
    u16 *self_base = (u16 *)&node->self_base;
    u32 *tmp_cdcbuf = ist30xx_frame_cdcbuf;
    u32 *tmp_self_cdcbuf = ist30xx_frame_self_cdcbuf;


    for (i = 0; i < node->self_len; i++) {
        *self_cdc++ = *tmp_self_cdcbuf & 0xFFF;
        *self_base++ = (*tmp_self_cdcbuf >> 16) & 0xFFF;

        tmp_self_cdcbuf++;
    }

    for (i = 0; i < node->len; i++) {
        *cdc++ = *tmp_cdcbuf & 0xFFF;
        *base++ = (*tmp_cdcbuf >> 16) & 0xFFF;

        tmp_cdcbuf++;
    }

    return 0;
}

int print_touch_node(struct ist30xx_data *data, u8 flag,
        struct TSP_NODE_BUF *node, char *buf)
{
    int i, j;
    int count = 0;
    int val = 0;
    const int msg_len = 128;
    char msg[msg_len];
    TSP_INFO *tsp = &data->tsp_info;

    if (tsp->dir.swap_xy) {
        for (i = 0; i < tsp->ch_num.rx; i++) {
            if (i == 0) {
                count += snprintf(msg, msg_len, "       ");
                strncat(buf, msg, msg_len);
                for (j = 0; j < tsp->ch_num.tx; j++) {
                    if (flag == NODE_FLAG_CDC) {
                        val = (int)node->self_cdc[j];
                        if (val < 0) val = 0;
                    } else if (flag == NODE_FLAG_BASE) {
                        val = (int)node->self_base[j];
                        if (val < 0) val = 0;
                    } else if (flag == NODE_FLAG_DIFF) {
                        val = (int)(node->self_cdc[j] - node->self_base[j]);
                    } else {
                        return 0;
                    }
                    count += snprintf(msg, msg_len, "%4d ", val);
                    strncat(buf, msg, msg_len);
                }
                count += snprintf(msg, msg_len, "\n\n");
                strncat(buf, msg, msg_len);
            }

            for (j = 0; j < tsp->ch_num.tx; j++) {
                if (j == 0) {
                    if (flag == NODE_FLAG_CDC) {
                        val = (int)node->self_cdc[tsp->ch_num.tx + i];
                        if (val < 0) val = 0;
                    } else if (flag == NODE_FLAG_BASE) {
                        val = (int)node->self_base[tsp->ch_num.tx + i];
                        if (val < 0) val = 0;
                    } else if (flag == NODE_FLAG_DIFF) {
                        val = (int)(node->self_cdc[tsp->ch_num.tx + i] -
                                node->self_base[tsp->ch_num.tx + i]);
                    } else {
                        return 0;
                    }
                    count += snprintf(msg, msg_len, "%4d   ", val);
                    strncat(buf, msg, msg_len);
                }

                if (flag == NODE_FLAG_CDC) {
                    val = (int)node->cdc[(j * tsp->ch_num.rx) + i];
                    if (val < 0) val = 0;
                } else if (flag == NODE_FLAG_BASE) {
                    val = (int)node->base[(j * tsp->ch_num.rx) + i];
                    if (val < 0) val = 0;
                } else if (flag == NODE_FLAG_DIFF) {
                    val = (int)(node->cdc[(j * tsp->ch_num.rx) + i]
                        - node->base[(j * tsp->ch_num.rx) + i]);
                } else {
                    return 0;
                }

                if (ist30xx_check_valid_ch(data, j, i) == TSP_CH_UNUSED)
                    count += snprintf(msg, msg_len, "%4d ", 0);
                else
                    count += snprintf(msg, msg_len, "%4d ", val);

                strncat(buf, msg, msg_len);
            }

            count += snprintf(msg, msg_len, "\n");
            strncat(buf, msg, msg_len);
        }
    } else {
        for (i = 0; i < tsp->ch_num.tx; i++) {
            if (i == 0) {
                count += snprintf(msg, msg_len, "       ");
                strncat(buf, msg, msg_len);
                for (j = 0; j < tsp->ch_num.rx; j++) {
                    if (flag == NODE_FLAG_CDC) {
                        val = (int)node->self_cdc[tsp->ch_num.tx + j];
                        if (val < 0) val = 0;
                    } else if (flag == NODE_FLAG_BASE) {
                        val = (int)node->self_base[tsp->ch_num.tx + j];
                        if (val < 0) val = 0;
                    } else if (flag == NODE_FLAG_DIFF) {
                        val = (int)(node->self_cdc[tsp->ch_num.tx + j] -
                                node->self_base[tsp->ch_num.tx + j]);
                    } else {
                        return 0;
                    }
                    count += snprintf(msg, msg_len, "%4d ", val);
                    strncat(buf, msg, msg_len);
                }
                count += snprintf(msg, msg_len, "\n\n");
                strncat(buf, msg, msg_len);
            }

            for (j = 0; j < tsp->ch_num.rx; j++) {
                if (j == 0) {
                    if (flag == NODE_FLAG_CDC) {
                        val = (int)node->self_cdc[i];
                        if (val < 0) val = 0;
                    } else if (flag == NODE_FLAG_BASE) {
                        val = (int)node->self_base[i];
                        if (val < 0) val = 0;
                    } else if (flag == NODE_FLAG_DIFF) {
                        val = (int)(node->self_cdc[i] - node->self_base[i]);
                    } else {
                        return 0;
                    }
                    count += snprintf(msg, msg_len, "%4d   ", val);
                    strncat(buf, msg, msg_len);
                }

                if (flag == NODE_FLAG_CDC) {
                    val = (int)node->cdc[(i * tsp->ch_num.rx) + j];
                    if (val < 0) val = 0;
                } else if (flag == NODE_FLAG_BASE) {
                    val = (int)node->base[(i * tsp->ch_num.rx) + j];
                    if (val < 0) val = 0;
                } else if (flag == NODE_FLAG_DIFF) {
                    val = (int)(node->cdc[(i * tsp->ch_num.rx) + j]
                        - node->base[(i * tsp->ch_num.rx) + j]);
                } else {
                    return 0;
                }

                if (ist30xx_check_valid_ch(data, i, j) == TSP_CH_UNUSED)
                    count += snprintf(msg, msg_len, "%4d ", 0);
                else
                    count += snprintf(msg, msg_len, "%4d ", val);

                strncat(buf, msg, msg_len);
            }

            count += snprintf(msg, msg_len, "\n");
            strncat(buf, msg, msg_len);
        }
    }

    return count;
}

int parse_tsp_node(struct ist30xx_data *data, u8 flag,
        struct TSP_NODE_BUF *node, s16 *buf16, s16 *self_buf16, int mode)
{
    int i, j;
    s16 val = 0;
    TSP_INFO *tsp = &data->tsp_info;

    if (unlikely((flag != NODE_FLAG_CDC) && (flag != NODE_FLAG_BASE) &&
                (flag != NODE_FLAG_DIFF)))
        return -EPERM;

    for (i = 0; i < (tsp->ch_num.tx + tsp->ch_num.rx); i++) {
        switch (flag) {
        case NODE_FLAG_CDC:
            val = (s16)node->self_cdc[i];
            if (val < 0) val = 0;
            break;
        case NODE_FLAG_BASE:
            val = (s16)node->self_base[i];
            if (val < 0) val = 0;
            break;
        case NODE_FLAG_DIFF:
            val = (s16)(node->self_cdc[i] - node->self_base[i]);
            break;
        default:
            val = 0;
            break;
        }

        *self_buf16++ = val;
    }    

    for (i = 0; i < tsp->ch_num.tx; i++) {
        for (j = 0; j < tsp->ch_num.rx; j++) {
            if (mode & TSP_CDC_SCREEN) {
                if (ist30xx_check_valid_ch(data, i, j) != TSP_CH_SCREEN)
                    continue;
            } else if (mode & TSP_CDC_KEY) {
                if (ist30xx_check_valid_ch(data, i, j) != TSP_CH_KEY)
                    continue;
            }
            switch (flag) {
            case NODE_FLAG_CDC:
                val = (s16)node->cdc[(i * tsp->ch_num.rx) + j];
                if (val < 0) val = 0;
                break;
            case NODE_FLAG_BASE:
                val = (s16)node->base[(i * tsp->ch_num.rx) + j];
                if (val < 0) val = 0;
                break;
            case NODE_FLAG_DIFF:
                val = (s16)(node->cdc[(i * tsp->ch_num.rx) + j]
                    - node->base[(i * tsp->ch_num.rx) + j]);
                break;
            default:
                val = 0;
                break;
            }

            if (ist30xx_check_valid_ch(data, i, j) == TSP_CH_UNUSED)
                val = 0;

            *buf16++ = val;
        }
    }

    return 0;
}

int ist30xx_read_touch_node(struct ist30xx_data *data, u8 flag,
        struct TSP_NODE_BUF *node)
{
    int ret;
    u32 addr;
    u32 *tmp_cdcbuf = ist30xx_frame_cdcbuf;
    u32 *tmp_self_cdcbuf = ist30xx_frame_self_cdcbuf;

    ist30xx_disable_irq(data);

    if (flag & NODE_FLAG_NO_CCP) {
        ist30xx_reset(data, false);

        ret = ist30xx_write_cmd(data, IST30XX_HIB_CMD,
            ((eHCOM_CP_CORRECT_EN << 16) | (IST30XX_DISABLE & 0xFFFF)));
        if (unlikely(ret))
            goto err_read_node;

        ist30xx_start(data);
        ist30xx_delay(1000);
    }

    ret = ist30xx_cmd_hold(data, IST30XX_ENABLE);
    if (unlikely(ret))
        goto err_read_node;

    addr = IST30XX_DA_ADDR(data->cdc_addr);
    tsp_info("MTL addr: %x, size: %d\n", addr, node->len);
    ret = ist30xx_burst_read(data->client, addr, tmp_cdcbuf, node->len, true);
    if (unlikely(ret))
        goto err_read_node;


    addr = IST30XX_DA_ADDR(data->self_cdc_addr);
    tsp_info("SFL addr: %x, size: %d\n", addr, node->self_len);
    ret = ist30xx_burst_read(data->client, addr, tmp_self_cdcbuf,
            node->self_len, true);
    if (unlikely(ret))
        goto err_read_node;
    
    if (flag & NODE_FLAG_NO_CCP) {
        ist30xx_reset(data, false);
        ist30xx_start(data);
    } else {
        ret = ist30xx_cmd_hold(data, IST30XX_DISABLE);
        if (ret) {
            ist30xx_reset(data, false);
            ist30xx_start(data);
        }
    }

    ist30xx_enable_irq(data);
    
    return 0;

err_read_node:
    ist30xx_reset(data, false);
    ist30xx_start(data);
    ist30xx_enable_irq(data);

    return ret;
}

int ist30xx_parse_cp_node(struct ist30xx_data *data, struct TSP_NODE_BUF *node,
        bool ium)
{
    int i;
    int len;
    u32 *tmp_cpbuf = ist30xx_frame_cpbuf;
    u32 *tmp_self_cpbuf = ist30xx_frame_self_cpbuf;
    u16 *cp;
    u16 *self_cp;
    if (ium) {
        cp = (u16 *)&node->rom_cp;
        self_cp = (u16 *)&node->rom_self_cp;
    } else {
        cp = (u16 *)&node->memx_cp;
        self_cp = (u16 *)&node->memx_self_cp;
    }

    if (ium) {
        len = node->self_len / 4;
        for (i = 0; i < len; i++) {
            *self_cp++ = *tmp_self_cpbuf & 0xFF;
            *self_cp++ = (*tmp_self_cpbuf >> 8) & 0xFF;
            *self_cp++ = (*tmp_self_cpbuf >> 16) & 0xFF;
            *self_cp++ = (*tmp_self_cpbuf >> 24) & 0xFF;

            tmp_self_cpbuf++;
        }
        
        len = node->self_len % 4;
        for (i = 0; i < len; i++)
            *self_cp++ = (*tmp_self_cpbuf >> (i * 8)) & 0xFF;

        len = node->len / 4;
        for (i = 0; i < len; i++) {
            *cp++ = *tmp_cpbuf & 0xFF;
            *cp++ = (*tmp_cpbuf >> 8) & 0xFF;
            *cp++ = (*tmp_cpbuf >> 16) & 0xFF;
            *cp++ = (*tmp_cpbuf >> 24) & 0xFF;

            tmp_cpbuf++;
        }

        len = node->len % 4;
        for (i = 0; i < len; i++)
            *cp++ = (*tmp_cpbuf >> (i * 8)) & 0xFF;

    } else {
        len = node->self_len / 2;
        for (i = 0; i < len; i++) {
            *self_cp++ = *tmp_self_cpbuf & 0xFF;
            *self_cp++ = (*tmp_self_cpbuf >> 8) & 0xFF;

            tmp_self_cpbuf++;
        }

        if (node->self_len % 2)
            *self_cp++ = *tmp_self_cpbuf & 0xFF;

        len = (node->len / 2) + (node->len % 2);
        for (i = 0; i < len; i++) {
            *cp++ = *tmp_cpbuf & 0xFF;
            *cp++ = (*tmp_cpbuf >> 8) & 0xFF;

            tmp_cpbuf++;
        }

        if (node->len % 2)
            *cp++ = *tmp_cpbuf & 0xFF;
    }

    return 0;
}

int parse_cp_node(struct ist30xx_data *data, struct TSP_NODE_BUF *node,
        s16 *buf16, s16 *self_buf16, int mode, bool ium)
{
    int i, j;
    s16 val = 0;
    TSP_INFO *tsp = &data->tsp_info;

    for (i = 0; i < (tsp->ch_num.tx + tsp->ch_num.rx); i++) {
        if (ium)
            val = (s16)node->rom_self_cp[i];
        else
            val = (s16)node->memx_self_cp[i];
        if (val < 0) val = 0;

        *self_buf16++ = val;
    }

    for (i = 0; i < tsp->ch_num.tx; i++) {
        for (j = 0; j < tsp->ch_num.rx; j++) {
            if (mode & TSP_CDC_SCREEN) {
                if (ist30xx_check_valid_ch(data, i, j) != TSP_CH_SCREEN)
                    continue;
            } else if (mode & TSP_CDC_KEY) {
                if (ist30xx_check_valid_ch(data, i, j) != TSP_CH_KEY)
                    continue;
            }
            
            if (ium)
                val = (s16)node->rom_cp[(i * tsp->ch_num.rx) + j];
            else
                val = (s16)node->memx_cp[(i * tsp->ch_num.rx) + j];
            if (val < 0) val = 0;

            if (ist30xx_check_valid_ch(data, i, j) == TSP_CH_UNUSED)
                val = 0;

            *buf16++ = val;
        }
    }

    return 0;
}

int print_cp_node(struct ist30xx_data *data, struct TSP_NODE_BUF *node,
        char *buf, bool ium)
{
    int i, j;
    int count = 0;
    u32 val = 0;
    const int msg_len = 128;
    char msg[msg_len];
    TSP_INFO *tsp = &data->tsp_info;

    if (tsp->dir.swap_xy) {
        for (i = 0; i < tsp->ch_num.rx; i++) {
            if (i == 0) {
                count += snprintf(msg, msg_len, "      ");
                strncat(buf, msg, msg_len);
                for (j = 0; j < tsp->ch_num.tx; j++) {
                    if (ium)
                        val = node->rom_self_cp[j];
                    else
                        val = node->memx_self_cp[j];
                    if (val < 0) val = 0;
                    count += snprintf(msg, msg_len, "%3d ", val);
                    strncat(buf, msg, msg_len);
                }
                count += snprintf(msg, msg_len, "\n\n");
                strncat(buf, msg, msg_len);
            }
            
            for (j = 0; j < tsp->ch_num.tx; j++) {
                if (j == 0) {
                    if (ium)
                        val = node->rom_self_cp[tsp->ch_num.tx + i];
                    else
                        val = node->memx_self_cp[tsp->ch_num.tx + i];
                    if (val < 0) val = 0;
                    count += snprintf(msg, msg_len, "%3d   ", val);
                    strncat(buf, msg, msg_len);
                }

                if (ium)
                    val = node->rom_cp[(j * tsp->ch_num.rx) + i];
                else
                    val = node->memx_cp[(j * tsp->ch_num.rx) + i];
                if (val < 0) val = 0;

                if (ist30xx_check_valid_ch(data, j, i) == TSP_CH_UNUSED)
                    count += snprintf(msg, msg_len, "%3d ", 0);
                else
                    count += snprintf(msg, msg_len, "%3d ", val);

                strncat(buf, msg, msg_len);
            }

            count += snprintf(msg, msg_len, "\n");
            strncat(buf, msg, msg_len);
        }
    } else {
        for (i = 0; i < tsp->ch_num.tx; i++) {
            if (i == 0) {
                count += snprintf(msg, msg_len, "      ");
                strncat(buf, msg, msg_len);
                for (j = 0; j < tsp->ch_num.rx; j++) {
                    if (ium)
                        val = node->rom_self_cp[tsp->ch_num.tx + j];
                    else
                        val = node->memx_self_cp[tsp->ch_num.tx + j];
                    if (val < 0) val = 0;
                    count += snprintf(msg, msg_len, "%3d ", val);
                    strncat(buf, msg, msg_len);
                }
                count += snprintf(msg, msg_len, "\n\n");
                strncat(buf, msg, msg_len);
            }

            for (j = 0; j < tsp->ch_num.rx; j++) {
                if (j == 0) {
                    if (ium)
                        val = node->rom_self_cp[i];
                    else
                        val = node->memx_self_cp[i];
                    if (val < 0) val = 0;
                    count += snprintf(msg, msg_len, "%3d   ", val);
                    strncat(buf, msg, msg_len);
                }

                if (ium)
                    val = node->rom_cp[(i * tsp->ch_num.rx) + j];
                else
                    val = node->memx_cp[(i * tsp->ch_num.rx) + j];
                if (val < 0) val = 0;

                if (ist30xx_check_valid_ch(data, i, j) == TSP_CH_UNUSED)
                    count += snprintf(msg, msg_len, "%3d ", 0);
                else
                    count += snprintf(msg, msg_len, "%3d ", val);

                strncat(buf, msg, msg_len);
            }

            count += snprintf(msg, msg_len, "\n");
            strncat(buf, msg, msg_len);
        }        
    }

    return count;
}

int ist30xx_read_cp_node(struct ist30xx_data *data, struct TSP_NODE_BUF *node,
        bool ium, bool miscal)
{
    int ret = 0;
    u32 addr;
    u32 len;
    u32 *tmp_cpbuf = ist30xx_frame_cpbuf;
    u32 *tmp_self_cpbuf = ist30xx_frame_self_cpbuf;

    ist30xx_disable_irq(data);

    if (ium) {
        ist30xx_disable_irq(data);    
        ist30xx_reset(data, true);
        ist30xx_isp_enable(data, true);
        data->ignore_delay = true;

        addr = IST30XX_IUM_BASE_ADDR + IST30XX_IUM_PAGE_SIZE;
        len = IST30XX_IUM_PAGE_SIZE * 3;
        tsp_info("IMU MTL addr: %x, size: %d\n", addr, len);
        ret = ist30xx_isp_read_ium(data, addr, tmp_cpbuf, len);
        if (ret)
            goto err_read_cp;

        addr = IST30XX_IUM_BASE_ADDR;
        len = IST30XX_IUM_PAGE_SIZE;
        tsp_info("IMU SLF addr: %x, size: %d\n", addr, len);
        ret = ist30xx_isp_read_ium(data, addr, tmp_self_cpbuf, len);
        if (ret)
            goto err_read_cp;

        ist30xx_isp_enable(data, false);
    } else {
        if (!miscal) {
            ret = ist30xx_cmd_hold(data, IST30XX_ENABLE);
            if (unlikely(ret))
                goto err_read_cp;
        }

        addr = IST30XX_DA_ADDR((data->cdc_addr + 0x5A4));
        len = (node->len / 2) + (node->len % 2);
        tsp_info("MemX MTL addr: %x, size: %d\n", addr, len);
        ret = ist30xx_burst_read(data->client, addr, tmp_cpbuf, len, true);
        if (unlikely(ret))
            goto err_read_cp;


        addr = IST30XX_DA_ADDR((data->cdc_addr + 0xE28));
        len = (node->self_len / 2) + (node->self_len % 2);
        tsp_info("MemX SFL addr: %x, size: %d\n", addr, len);
        ret = ist30xx_burst_read(data->client, addr, tmp_self_cpbuf, len, true);
        if (unlikely(ret))
            goto err_read_cp;

        if (!miscal) {
            ret = ist30xx_cmd_hold(data, IST30XX_DISABLE);
            if (ret) {
                ist30xx_reset(data, false);
                ist30xx_start(data);
            }
        }

        ist30xx_enable_irq(data);

        return 0;
    }    

err_read_cp:
    if (ium)
        data->ignore_delay = false;
    ist30xx_reset(data, false);
    ist30xx_start(data);
    ist30xx_enable_irq(data);

    return ret;

}

/* sysfs: /sys/class/touch/node/cp */
ssize_t ist30xx_cp_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int ret = 0;
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;

    tsp_info("Read CP of MemX\n");
    mutex_lock(&data->lock);
    ret = ist30xx_read_cp_node(data, &tsp->node, false, false);
    if (unlikely(ret)) {
        mutex_unlock(&data->lock);
        tsp_err("frame read fail\n");
        return sprintf(buf, "FAIL\n");
    }
    mutex_unlock(&data->lock);

    ist30xx_parse_cp_node(data, &tsp->node, false);

    buf[0] = '\0';
    count = sprintf(buf, "OK\n");
    count += print_cp_node(data, &tsp->node, buf, false);

    return count;
}

/* sysfs: /sys/class/touch/node/cp_ium */
ssize_t ist30xx_cp_ium_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int ret = 0;
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;

    tsp_info("Read CP of IUM\n");
    mutex_lock(&data->lock);
    ret = ist30xx_read_cp_node(data, &tsp->node, true, false);
    if (unlikely(ret)) {
        mutex_unlock(&data->lock);
        tsp_err("frame read fail\n");
        return sprintf(buf, "FAIL\n");
    }
    mutex_unlock(&data->lock);

    ist30xx_parse_cp_node(data, &tsp->node, true);

    buf[0] = '\0';
    count = sprintf(buf, "OK\n");
    count += print_cp_node(data, &tsp->node, buf, true);

    return count;
}

/* sysfs: /sys/class/touch/node/cdcbase */
ssize_t ist30xx_frame_cdcbase(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int i;
    int ret = 0;
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    u8 flag = NODE_FLAG_CDC | NODE_FLAG_BASE;
    u32 *tmp_cdcbuf = ist30xx_frame_cdcbuf;
    const int msg_len = 128;
    char msg[msg_len];
    char header[128];
    char *buf8;
    mm_segment_t old_fs = { 0 };
    struct file *fp = NULL;
    TKEY_INFO *tkey = &data->tkey_info;
    TSP_INFO *tsp = &data->tsp_info;

    old_fs = get_fs();
    set_fs(get_ds());

    if (!strcmp(data->rec_file_name, ""))
        snprintf(data->rec_file_name, 128, "/sdcard/%s", IST30XX_REC_FILENAME);

    fp = filp_open(data->rec_file_name, O_WRONLY, 0);
    if (IS_ERR(fp))
        ret = PTR_ERR(fp);

    fp = filp_open(data->rec_file_name, O_CREAT|O_WRONLY|O_APPEND, 0);
    if (IS_ERR(fp)) {
        tsp_err("file %s open error:%d\n", data->rec_file_name, PTR_ERR(fp));
        goto err_file_open;
    }

    if (ret) {
        count = snprintf(header, 128, "%d %d %d %d %d %d %d %d %d %d %d %d %d "
                "%d %d %d %d %d\n", 2, tsp->ch_num.rx, tsp->ch_num.tx,
                tsp->ch_num.rx, tsp->ch_num.tx, tsp->screen.rx, tsp->screen.tx,
                tkey->key_num, tkey->ch_num[0].rx, tkey->ch_num[0].tx,
                tkey->ch_num[1].tx, tkey->ch_num[1].rx, tkey->ch_num[2].tx,
                tkey->ch_num[2].rx, tkey->ch_num[3].tx, tkey->ch_num[3].rx,
                tkey->ch_num[4].tx, tkey->ch_num[4].rx);

        fp->f_op->write(fp, header, count, &fp->f_pos);
        fput(fp);
    }

    mutex_lock(&data->lock);
    ret = ist30xx_read_touch_node(data, flag, &tsp->node);
    if (unlikely(ret)) {
        mutex_unlock(&data->lock);
        tsp_err("fail to read frame\n");
        goto err_read_node;
    }
    mutex_unlock(&data->lock);

    buf8 = kzalloc(IST30XX_MAX_NODE_NUM * 20, GFP_KERNEL);
    if (unlikely(!buf8)) {
        tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
        goto err_read_node;
    }

    count = 0;
    for (i = 0; i < IST30XX_MAX_NODE_NUM; i++) {
        count += snprintf(msg, msg_len, "%08x ", *tmp_cdcbuf);
        strncat(buf8, msg, msg_len);
        tmp_cdcbuf++;
    }

    for (i = 0; i < IST30XX_MAX_NODE_NUM; i++) {
        count += snprintf(msg, msg_len, "%08x ", 0);
        strncat(buf8, msg, msg_len);
    }

    count += snprintf(msg, msg_len, "\n");
    strncat(buf8, msg, msg_len);

    fp->f_op->write(fp, buf8, count, &fp->f_pos);
    fput(fp);

    kfree(buf8);

err_read_node:
    filp_close(fp, NULL);    
err_file_open:
    set_fs(old_fs);
    
    return 0;
}

/* sysfs: /sys/class/touch/node/refresh */
ssize_t ist30xx_refresh_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int ret = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;
    u8 flag = NODE_FLAG_CDC | NODE_FLAG_BASE;

    tsp_info("refresh node value\n");
    mutex_lock(&data->lock);
    ret = ist30xx_read_touch_node(data, flag, &tsp->node);
    if (unlikely(ret)) {
        mutex_unlock(&data->lock);
        tsp_err("frame read fail\n");
        return sprintf(buf, "FAIL\n");
    }
    mutex_unlock(&data->lock);

    ist30xx_parse_touch_node(data, &tsp->node);

    return sprintf(buf, "OK\n");
}

/* sysfs: /sys/class/touch/node/read_nocp */
ssize_t ist30xx_nocp_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int ret = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;
    u8 flag = NODE_FLAG_CDC | NODE_FLAG_BASE | NODE_FLAG_NO_CCP;

    tsp_info("refresh node value(nocp)\n");
    mutex_lock(&data->lock);
    ret = ist30xx_read_touch_node(data, flag, &tsp->node);
    if (unlikely(ret)) {
        mutex_unlock(&data->lock);
        tsp_err("frame read fail\n");
        return sprintf(buf, "FAIL\n");
    }
    mutex_unlock(&data->lock);

    ist30xx_parse_touch_node(data, &tsp->node);

    return sprintf(buf, "OK\n");
}

/* sysfs: /sys/class/touch/node/cdc*/
ssize_t ist30xx_cdc_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;

    buf[0] = '\0';
    count = sprintf(buf, "[cdc]\n");

    count += print_touch_node(data, NODE_FLAG_CDC, &tsp->node, buf);

    return count;
}

/* sysfs: /sys/class/touch/node/base */
ssize_t ist30xx_base_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;

    buf[0] = '\0';
    count = sprintf(buf, "[baseline]\n");

    count += print_touch_node(data, NODE_FLAG_BASE, &tsp->node, buf);

    return count;
}

/* sysfs: /sys/class/touch/node/diff */
ssize_t ist30xx_diff_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;

    buf[0] = '\0';
    count = sprintf(buf, "[diff(cdc-basline)]\n");

    count += print_touch_node(data, NODE_FLAG_DIFF, &tsp->node, buf);

    return count;
}

/* sysfs: /sys/class/touch/sys/debug_mode */
ssize_t ist30xx_debug_mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int ret = 0;
    int enable;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d", &enable);

    if (unlikely((enable != 0) && (enable != 1))) {
        tsp_err("input data error(%d)\n", enable);
        return size;
    }

    data->debug_mode = enable;

    if (data->status.power) {
        ret = ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (data->debug_mode & 0xFFFF));
    }

    tsp_info("debug mode %s\n", enable ? "start" : "stop");

    return size;
}

/* sysfs: /sys/class/touch/sys/jig_mode */
ssize_t ist30xx_jig_mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int enable;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d", &enable);

    if (unlikely((enable != 0) && (enable != 1))) {
        tsp_err("input data error(%d)\n", enable);
        return size;
    }

    data->jig_mode = enable;
    tsp_info("set jig mode: %s\n", enable ? "start" : "stop");

    mutex_lock(&data->lock);
    ist30xx_reset(data, false);
    ist30xx_start(data);
    mutex_unlock(&data->lock);

    return size;
}

extern int calib_ms_delay;
/* sysfs: /sys/class/touch/sys/clb_time */
ssize_t ist30xx_calib_time_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int ms_delay;

    sscanf(buf, "%d", &ms_delay);

    if (ms_delay > 10 && ms_delay < 1000) // 1sec ~ 100sec
        calib_ms_delay = ms_delay;

    tsp_info("Calibration wait time %dsec\n", calib_ms_delay / 10);

    return size;
}

/* sysfs: /sys/class/touch/sys/clb */
ssize_t ist30xx_calib_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    mutex_lock(&data->lock);
    ist30xx_disable_irq(data);

    ist30xx_reset(data, false);
#ifdef TCLM_CONCEPT
	ist30xx_tclm_root_of_cal(data, CALPOSITION_TESTMODE);
	ist30xx_execute_tclm_package(data, 0);
	ist30xx_tclm_root_of_cal(data, CALPOSITION_NONE);
#else
	ist30xx_execute_force_calibration(data);
#endif

    mutex_unlock(&data->lock);
    ist30xx_start(data);

    return 0;
}

/* sysfs: /sys/class/touch/sys/clb_result */
ssize_t ist30xx_calib_result_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int ret;
    int count = 0;
    u32 calib_msg[2];
    struct ist30xx_data *data = dev_get_drvdata(dev);

    ret = ist30xx_read_cmd(data, eHCOM_GET_SLF_CAL_RESULT, &calib_msg[0]);
    if (unlikely(ret)) {
        mutex_lock(&data->lock);
        ist30xx_reset(data, false);
        ist30xx_start(data);
        mutex_unlock(&data->lock);
        tsp_warn("Error Read Calibration Result\n");
        count = sprintf(buf, "Error Read Calibration Result\n");
        goto calib_show_end;
    }

    ret = ist30xx_read_cmd(data, eHCOM_GET_CAL_RESULT, &calib_msg[1]);
    if (unlikely(ret)) {
        mutex_lock(&data->lock);
        ist30xx_reset(data, false);
        ist30xx_start(data);
        mutex_unlock(&data->lock);
        tsp_warn("Error Read Calibration Result\n");
        count = sprintf(buf, "Error Read Calibration Result\n");
        goto calib_show_end;
    }

    count = sprintf(buf,
            "SLF Calibration Status : %d, Max gap : %d - (%08x)\n"
            "MTL Calibration Status : %d, Max gap : %d - (%08x)\n",
            CALIB_TO_STATUS(calib_msg[0]), CALIB_TO_GAP(calib_msg[0]),
            calib_msg[0], CALIB_TO_STATUS(calib_msg[1]),
            CALIB_TO_GAP(calib_msg[1]), calib_msg[1]);

calib_show_end:

    return count;
}

/* sysfs: /sys/class/touch/sys/power_on */
ssize_t ist30xx_power_on_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    tsp_info("Power enable: %d\n", true);

    mutex_lock(&data->lock);
    ist30xx_power_on(data, false);
    ist30xx_start(data);
    ist30xx_enable_irq(data);
    mutex_unlock(&data->lock);

    return 0;
}

/* sysfs: /sys/class/touch/sys/power_off */
ssize_t ist30xx_power_off_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    tsp_info("Power enable: %d\n", false);

    mutex_lock(&data->lock);
    ist30xx_disable_irq(data);
    ist30xx_power_off(data);
    mutex_unlock(&data->lock);

    return 0;
}

/* sysfs: /sys/class/touch/sys/errcnt */
ssize_t ist30xx_errcnt_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    int err_cnt;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d", &err_cnt);

    if (unlikely(err_cnt < 0))
        return size;

    tsp_info("Request reset error count: %d\n", err_cnt);

    data->max_irq_err_cnt = err_cnt;

    return size;
}

/* sysfs: /sys/class/touch/sys/scancnt */
ssize_t ist30xx_scancnt_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    int retry;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d", &retry);

    if (unlikely(retry < 0))
        return size;

    tsp_info("Timer scan count retry: %d\n", retry);

    data->max_scan_retry = retry;

    return size;
}

/* sysfs: /sys/class/touch/sys/timerms */
ssize_t ist30xx_timerms_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    int ms;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d", &ms);

    if (unlikely((ms < 0) || (ms > 10000)))
        return size;

    tsp_info("Timer period ms: %dms\n", ms);

    data->timer_period_ms = ms;

    return size;
}

extern int ist30xx_log_level;
/* sysfs: /sys/class/touch/sys/printk */
ssize_t ist30xx_printk_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    int level;

    sscanf(buf, "%d", &level);

    if (unlikely((level < DEV_ERR) || (level > DEV_VERB)))
        return size;

    tsp_info("prink log level: %d\n", level);

    ist30xx_log_level = level;

    return size;
}

ssize_t ist30xx_printk_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    return sprintf(buf, "prink log level: %d\n", ist30xx_log_level);
}

/* sysfs: /sys/class/touch/sys/printk5 */
ssize_t ist30xx_printk5_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    tsp_info("prink log level:%d\n", DEV_DEBUG);

    ist30xx_log_level = DEV_DEBUG;

    return 0;
}

/* sysfs: /sys/class/touch/sys/printk6 */
ssize_t ist30xx_printk6_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    tsp_info("prink log level:%d\n", DEV_VERB);
    
    ist30xx_log_level = DEV_VERB;

    return 0;
}

/* sysfs: /sys/class/touch/sys/spay */
ssize_t ist30xx_spay_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    int enable;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d", &enable);

    tsp_info("spay enable : %s\n", enable ? "enable" : "disable");

    data->spay = enable ? true : false;

    if (data->spay) {
        data->g_reg.b.ctrl |= IST30XX_GETURE_CTRL_SPAY;
        data->g_reg.b.setting |= IST30XX_GETURE_SET_SPAY;
    } else {
        data->g_reg.b.ctrl &= ~IST30XX_GETURE_CTRL_SPAY;
        data->g_reg.b.setting &= ~IST30XX_GETURE_SET_SPAY;
    }

    return size;
}

/* sysfs: /sys/class/touch/sys/aod */
ssize_t ist30xx_aod_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    int enable;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d", &enable);

    tsp_info("aod enable : %s\n", enable ? "enable" : "disable");

    data->aod = enable ? true : false;

    if (data->aod) {
        data->g_reg.b.ctrl |= IST30XX_GETURE_CTRL_AOD;
        data->g_reg.b.setting |= IST30XX_GETURE_SET_AOD;
    } else {
        data->g_reg.b.ctrl &= ~IST30XX_GETURE_CTRL_AOD;
        data->g_reg.b.setting &= ~IST30XX_GETURE_SET_AOD;
    }

    return size;
}

/* sysfs: /sys/class/touch/sys/report_rate */
ssize_t ist30xx_report_rate_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int rate;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d", &rate);               // us

    if (unlikely(rate > 0xFFFF))            // over 65.5ms
        return size;

    data->report_rate = rate;
    ist30xx_write_cmd(data, IST30XX_HIB_CMD,
            ((eHCOM_SET_TIME_ACTIVE << 16) | (data->report_rate & 0xFFFF)));
    tsp_info(" active rate : %dus\n", data->report_rate);

    return size;
}

/* sysfs: /sys/class/touch/sys/idle_rate */
ssize_t ist30xx_idle_scan_rate_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int rate;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d", &rate);               // us

    if (unlikely(rate > 0xFFFF))            // over 65.5ms
        return size;

    data->idle_rate = rate;
    ist30xx_write_cmd(data, IST30XX_HIB_CMD,
            ((eHCOM_SET_TIME_IDLE << 16) | (data->idle_rate & 0xFFFF)));
    tsp_info(" idle rate : %dus\n", data->idle_rate);

    return size;
}

extern void ist30xx_set_ta_mode(bool charging);
/* sysfs: /sys/class/touch/sys/mode_ta */
ssize_t ist30xx_ta_mode_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    int mode;

    sscanf(buf, "%d", &mode);

    if (unlikely((mode != 0) && (mode != 1)))   // enable/disable
        return size;

    ist30xx_set_ta_mode(mode);

    return size;
}

extern void ist30xx_set_call_mode(int mode);
/* sysfs: /sys/class/touch/sys/mode_call */
ssize_t ist30xx_call_mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int mode;

    sscanf(buf, "%d", &mode);

    if (unlikely((mode != 0) && (mode != 1)))   // enable/disable
        return size;

    ist30xx_set_call_mode(mode);

    return size;
}

extern void ist30xx_set_cover_mode(int mode);
/* sysfs: /sys/class/touch/sys/mode_cover */
ssize_t ist30xx_cover_mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int mode;

    sscanf(buf, "%d", &mode);

    if (unlikely((mode != 0) && (mode != 1)))   // enable/disable
        return size;

    ist30xx_set_cover_mode(mode);

    return size;
}

/* sysfs: /sys/class/touch/sys/ic_inform */
ssize_t ist30xx_read_ic_inform(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    ist30xx_print_info(data);

    return 0;
}

#define TUNES_CMD_WRITE         (1)
#define TUNES_CMD_READ          (2)
#define TUNES_CMD_REG_ENTER     (3)
#define TUNES_CMD_REG_EXIT      (4)
#define TUNES_CMD_UPDATE_PARAM  (5)
#define TUNES_CMD_UPDATE_FW     (6)

#define DIRECT_ADDR(n)          (IST30XX_DA_ADDR(n))
#define DIRECT_CMD_WRITE        ('w')
#define DIRECT_CMD_READ         ('r')
#define DIRECT_BUF_COUNT        (4)

#pragma pack(1)
typedef struct {
    u8 cmd;
    u32 addr;
    u16 len;
} TUNES_INFO;
#pragma pack()
#pragma pack(1)
typedef struct {
    char cmd;
    u32 addr;
    u32 val;
    u32 size;
} DIRECT_INFO;
#pragma pack()

static TUNES_INFO ist30xx_tunes;
static DIRECT_INFO ist30xx_direct;
static bool tunes_cmd_done = false;
static bool ist30xx_reg_mode = false;
/* sysfs: /sys/class/touch/sys/direct */
ssize_t ist30xx_direct_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    int ret = -EPERM;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    DIRECT_INFO *direct = (DIRECT_INFO *)&ist30xx_direct;

    sscanf(buf, "%c %x %x %d", &direct->cmd, &direct->addr, &direct->val,
            &direct->size);

    if (direct->size == 0)
        direct->size = DIRECT_BUF_COUNT;

    tsp_debug("Direct cmd: %c, addr: %x, val: %x\n",
            direct->cmd, direct->addr, direct->val);

    if (unlikely((direct->cmd != DIRECT_CMD_WRITE) &&
            (direct->cmd != DIRECT_CMD_READ))) {
        tsp_warn("Direct cmd is not correct!\n");
        return size;
    }

    if (ist30xx_intr_wait(data, 30) < 0)
        return size;

    data->status.event_mode = false;
    if (direct->cmd == DIRECT_CMD_WRITE) {
        ist30xx_cmd_hold(data, IST30XX_ENABLE);
        ist30xx_write_cmd(data, DIRECT_ADDR(direct->addr), direct->val);
        ist30xx_read_reg(data->client, DIRECT_ADDR(direct->addr), &direct->val);
        ret = ist30xx_cmd_hold(data, IST30XX_DISABLE);
        if (ret)
            tsp_debug("Direct write fail\n");
        else
            tsp_debug("Direct write addr: %x, val: %x\n", direct->addr,
                    direct->val);
        
    }
    data->status.event_mode = true;

    return size;
}

ssize_t ist30xx_direct_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int i, count = 0;
    int ret = 0;
    int len;
    u32 addr;
    u32 *buf32;
    const int msg_len = 256;
    char msg[msg_len];
    struct ist30xx_data *data = dev_get_drvdata(dev);
    DIRECT_INFO *direct = (DIRECT_INFO *)&ist30xx_direct;
    int max_len = direct->size;

    if (unlikely(direct->cmd != DIRECT_CMD_READ))
        return sprintf(buf, "ex) echo r addr len size(display) > direct\n");

    len = direct->val;
    addr = DIRECT_ADDR(direct->addr);

    if (ist30xx_intr_wait(data, 30) < 0)
        return 0;

    if (!data->status.power)
        return 0;

    data->status.event_mode = false;
    ist30xx_cmd_hold(data, IST30XX_ENABLE);

    buf32 = kzalloc(max_len * sizeof(u32), GFP_KERNEL);
    if (unlikely(!buf32)) {
        tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
        return 0;
    }

    while (len > 0) {
        if (len < max_len) max_len = len;

        memset(buf32, 0, max_len * sizeof(u32));
        ret = ist30xx_burst_read(data->client, addr, buf32, max_len, true);
        if (unlikely(ret)) {
            count = sprintf(buf, "I2C Burst read fail\n");
            break;
        }
        addr += (max_len * IST30XX_DATA_LEN);

        for (i = 0; i < max_len; i++) {
            count += snprintf(msg, msg_len, "0x%08x ", buf32[i]);
            strncat(buf, msg, msg_len);
        }
        count += snprintf(msg, msg_len, "\n");
        strncat(buf, msg, msg_len);

        len -= max_len;
    }
    kfree(buf32);

    ret = ist30xx_cmd_hold(data, IST30XX_DISABLE);
    if (ret)
        tsp_err("Hold disable fail\n");
    data->status.event_mode = true;

    return count;
}

/* sysfs: /sys/class/touch/tunes/node_info */
ssize_t tunes_node_info_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int size;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;
    TKEY_INFO *tkey = &data->tkey_info;

    size = sprintf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
            "%d %d %d %d %d %d %d %d",
            data->chip_id, tsp->dir.swap_xy, tsp->ch_num.tx,
            tsp->ch_num.rx,tsp->screen.tx, tsp->screen.rx,
            tsp->gtx.num, tsp->gtx.ch_num[0], tsp->gtx.ch_num[1],
            tsp->gtx.ch_num[2], tsp->gtx.ch_num[3], tsp->baseline,
            tkey->enable, tkey->baseline, tkey->key_num, tkey->ch_num[0].tx,
            tkey->ch_num[0].rx, tkey->ch_num[1].tx, tkey->ch_num[1].rx,
            tkey->ch_num[2].tx, tkey->ch_num[2].rx, tkey->ch_num[3].tx,
            tkey->ch_num[3].rx, 255, 255, data->cdc_addr);

    return size;
}

/* sysfs: /sys/class/touch/tunes/regcmd */
ssize_t tunes_regcmd_store(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t size)
{
    int ret = -1;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    memcpy(&ist30xx_tunes, buf, sizeof(ist30xx_tunes));
    buf += sizeof(ist30xx_tunes);

    tunes_cmd_done = false;

    switch (ist30xx_tunes.cmd) {
    case TUNES_CMD_WRITE:
        break;
    case TUNES_CMD_READ:
        break;
    case TUNES_CMD_REG_ENTER:
        ist30xx_disable_irq(data);
        /* enter reg access mode */
        ret = ist30xx_cmd_hold(data, IST30XX_ENABLE);
        if (unlikely(ret))
            goto regcmd_fail;

        ist30xx_reg_mode = true;
        break;
    case TUNES_CMD_REG_EXIT:
        /* exit reg access mode */
        ret = ist30xx_cmd_hold(data, IST30XX_DISABLE);
        if (unlikely(ret)) {
            ist30xx_reset(data, false);
            ist30xx_start(data);
            goto regcmd_fail;
        }

        ist30xx_reg_mode = false;
        ist30xx_enable_irq(data);
        break;
    default:
        ist30xx_enable_irq(data);
        return size;
    }
    tunes_cmd_done = true;

    return size;

regcmd_fail:
    tsp_err("Tunes regcmd i2c_fail, ret=%d\n", ret);
    return size;
}

ssize_t tunes_regcmd_show(struct device *dev, struct device_attribute *attr,
              char *buf)
{
    int size;

    size = sprintf(buf, "cmd: 0x%02x, addr: 0x%08x, len: 0x%04x\n",
            ist30xx_tunes.cmd, ist30xx_tunes.addr, ist30xx_tunes.len);

    return size;
}

#define MAX_WRITE_LEN   (1)
/* sysfs: /sys/class/touch/tunes/reg */
ssize_t tunes_reg_store(struct device *dev, struct device_attribute *attr,
            const char *buf, size_t size)
{
    int ret;
    u32 *buf32 = (u32 *)buf;
    int waddr, wcnt = 0, len = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    if (unlikely(ist30xx_tunes.cmd != TUNES_CMD_WRITE)) {
        tsp_err("error, IST30XX_REG_CMD is not correct!\n");
        return size;
    }

    if (unlikely(!ist30xx_reg_mode)) {
        tsp_err("error, IST30XX_REG_CMD is not ready!\n");
        return size;
    }

    if (unlikely(!tunes_cmd_done)) {
        tsp_err("error, IST30XX_REG_CMD is not ready!\n");
        return size;
    }

    waddr = ist30xx_tunes.addr;
    if (ist30xx_tunes.len >= MAX_WRITE_LEN)
        len = MAX_WRITE_LEN;
    else
        len = ist30xx_tunes.len;

    while (wcnt < ist30xx_tunes.len) {
        ret = ist30xx_write_buf(data->client, waddr, buf32, len);
        if (unlikely(ret)) {
            tsp_err("Tunes regstore i2c_fail, ret=%d\n", ret);
            return size;
        }

        wcnt += len;

        if ((ist30xx_tunes.len - wcnt) < MAX_WRITE_LEN)
            len = ist30xx_tunes.len - wcnt;

        buf32 += MAX_WRITE_LEN;
        waddr += MAX_WRITE_LEN * IST30XX_DATA_LEN;
    }

    tunes_cmd_done = false;

    return size;
}

ssize_t tunes_reg_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int ret;
    int size = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    if (unlikely(ist30xx_tunes.cmd != TUNES_CMD_READ)) {
        tsp_err("error, IST30XX_REG_CMD is not correct!\n");
        return 0;
    }

    if (unlikely(!tunes_cmd_done)) {
        tsp_err("error, IST30XX_REG_CMD is not ready!\n");
        return 0;
    }

    ret = ist30xx_burst_read(data->client, ist30xx_tunes.addr, (u32 *)buf,
            ist30xx_tunes.len, false);
    if (unlikely(ret)) {
        tsp_err("Tunes regshow i2c_fail, ret=%d\n", ret);
        return size;
    }

    size = ist30xx_tunes.len * IST30XX_DATA_LEN;

    tunes_cmd_done = false;

    return size;
}

/* sysfs: /sys/class/touch/tunes/adb */
ssize_t tunes_adb_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size)
{
    int ret;
    char *tmp, *ptr;
    char token[9];
    u32 cmd, addr, len, val;
    int write_len;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%x %x %x", &cmd, &addr, &len);

    switch (cmd) {
    case TUNES_CMD_WRITE:   /* write cmd */
        write_len = 0;
        ptr = (char *)(buf + 15);

        while (write_len < len) {
            memcpy(token, ptr, 8);
            token[8] = 0;
            val = simple_strtoul(token, &tmp, 16);
            ret = ist30xx_write_buf(data->client, addr, &val, 1);
            if (unlikely(ret)) {
                tsp_err("Tunes regstore i2c_fail, ret=%d\n", ret);
                return size;
            }

            ptr += 8;
            write_len++;
            addr += 4;
        }
        break;

    case TUNES_CMD_READ:   /* read cmd */
        ist30xx_tunes.cmd = cmd;
        ist30xx_tunes.addr = addr;
        ist30xx_tunes.len = len;
        break;

    case TUNES_CMD_REG_ENTER:   /* enter */
        ist30xx_disable_irq(data);

        ret = ist30xx_cmd_hold(data, IST30XX_ENABLE);
        if (unlikely(ret < 0))
            goto cmd_fail;

        ist30xx_reg_mode = true;
        break;

    case TUNES_CMD_REG_EXIT:   /* exit */
        if (ist30xx_reg_mode == true) {
            ret = ist30xx_cmd_hold(data, IST30XX_DISABLE);
            if (unlikely(ret < 0)) {
                ist30xx_reset(data, false);
                ist30xx_start(data);
                goto cmd_fail;
            }

            ist30xx_reg_mode = false;
            ist30xx_enable_irq(data);
        }
        break;

    default:
        break;
    }

    return size;

cmd_fail:
    tsp_err("Tunes adb i2c_fail\n");
    return size;
}

ssize_t tunes_adb_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int ret;
    int i, len, size = 0;
    char reg_val[10];
    struct ist30xx_data *data = dev_get_drvdata(dev);

    tsp_info("tunes_adb_show,%08x,%d\n", ist30xx_tunes.addr, ist30xx_tunes.len);

    ret = ist30xx_burst_read(data->client, ist30xx_tunes.addr,
            ist30xx_frame_buf, ist30xx_tunes.len, false);
    if (unlikely(ret)) {
        tsp_err("Tunes adbshow i2c_fail, ret=%d\n", ret);
        return size;
    }

    size = 0;
    buf[0] = 0;
    len = sprintf(reg_val, "%08x", ist30xx_tunes.addr);
    strcat(buf, reg_val);
    size += len;
    for (i = 0; i < ist30xx_tunes.len; i++) {
        len = sprintf(reg_val, "%08x", ist30xx_frame_buf[i]);
        strcat(buf, reg_val);
        size += len;
    }

    return size;
}

/* sysfs: /sys/class/touch/tunes/algorithm */
ssize_t ist30xx_algr_store(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t size)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%x %d", &data->algr_addr, &data->algr_size);
    tsp_info("algorithm addr: 0x%x, count: %d\n", data->algr_addr,
            data->algr_size);

    return size;
}

ssize_t ist30xx_algr_show(struct device *dev, struct device_attribute *attr,
              char *buf)
{
    int ret;
    u32 algr_addr;
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    ret = ist30xx_read_cmd(data, eHCOM_GET_ALGO_BASE, &algr_addr);
    if (unlikely(ret)) {
        mutex_lock(&data->lock);
        ist30xx_reset(data, false);
        ist30xx_start(data);
        mutex_unlock(&data->lock);
        tsp_warn("algorithm mem addr read fail!\n");
        return 0;
    }

    tsp_info("algr_addr(0x%x): 0x%x\n", eHCOM_GET_ALGO_BASE, algr_addr);
    count = sprintf(buf, "algorithm addr : 0x%x\n", algr_addr);

    return count;
}

/* sysfs: /sys/class/touch/tunes/intr_debug */
ssize_t intr_debug_store(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t size)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%x %d", &data->intr_debug1_addr, &data->intr_debug1_size);
    tsp_info("intr debug1 addr: 0x%x, count: %d\n", data->intr_debug1_addr,
            data->intr_debug1_size);

    return size;
}

ssize_t intr_debug_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    tsp_info("intr debug1 addr: 0x%x, count: %d\n", data->intr_debug1_addr,
            data->intr_debug1_size);

    count = sprintf(buf, "intr debug1 addr: 0x%x, count: %d\n",
            data->intr_debug1_addr, data->intr_debug1_size);

    return count;
}

/* sysfs: /sys/class/touch/tunes/intr_debug2 */
ssize_t intr_debug2_store(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t size)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%x %d", &data->intr_debug2_addr, &data->intr_debug2_size);
    tsp_info("intr debug2 addr: 0x%x, count: %d\n", data->intr_debug2_addr,
            data->intr_debug2_size);

    return size;
}

ssize_t intr_debug2_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    tsp_info("intr debug2 addr: 0x%x, count: %d\n", data->intr_debug2_addr,
            data->intr_debug2_size);

    count = sprintf(buf, "intr debug2 addr: 0x%x, count: %d\n",
            data->intr_debug2_addr, data->intr_debug2_size);

    return count;
}

/* sysfs: /sys/class/touch/tunes/intr_debug3 */
ssize_t intr_debug3_store(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t size)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%x %d", &data->intr_debug3_addr, &data->intr_debug3_size);
    tsp_info("intr debug3 addr: 0x%x, count: %d\n", data->intr_debug3_addr,
            data->intr_debug3_size);

    return size;
}

ssize_t intr_debug3_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int count = 0;
    struct ist30xx_data *data = dev_get_drvdata(dev);

     tsp_info("intr debug3 addr: 0x%x, count: %d\n", data->intr_debug3_addr,
            data->intr_debug2_size);

    count = sprintf(buf, "intr debug3 addr: 0x%x, count: %d\n",
            data->intr_debug3_addr, data->intr_debug3_size);

    return count;
}

#define IST30XX_DEBUGGING_MAGIC 0xDEBC0000
#define MAX_FRAME_COUNT         1024
IST30XX_RING_BUF DebugBuf;
IST30XX_RING_BUF *pDebugBuf;
bool debugging_initialize = false;
void ist30xx_debugging_init(struct ist30xx_data *data)
{
    if (!debugging_initialize)
        pDebugBuf = &DebugBuf;        
    
    pDebugBuf->RingBufCtr = 0;
    pDebugBuf->RingBufInIdx = 0;
    pDebugBuf->RingBufOutIdx = 0;

    data->debugging_scancnt = 0;

    debugging_initialize = true;
}

u32 ist30xx_get_debugging_cnt(void)
{
    return pDebugBuf->RingBufCtr;
}

int ist30xx_get_frame(struct ist30xx_data *data, u32 *frame, u32 cnt)
{
    int i;
    u8 *buf = (u8 *)frame;

    cnt *= IST30XX_DATA_LEN;

    if (pDebugBuf->RingBufCtr < cnt)
        return IST30XX_RINGBUF_NOT_ENOUGH;
    
    for (i = 0; i < cnt; i++) {
        if (pDebugBuf->RingBufOutIdx == IST30XX_MAX_RINGBUF_SIZE)
            pDebugBuf->RingBufOutIdx = 0;

        *buf++ = (u8)pDebugBuf->LogBuf[pDebugBuf->RingBufOutIdx++];
        pDebugBuf->RingBufCtr--;
    }
    
    return IST30XX_RINGBUF_NO_ERR;
}

int ist30xx_put_frame(struct ist30xx_data *data, u32 ms, u32 *touch, u32 *frame,
        int frame_cnt)
{
    int i;
    int size = 0;
    u32 *buf32;
    u8 *buf;
   
    buf32 = kzalloc((frame_cnt + 3) * sizeof(u32), GFP_KERNEL);
    if (unlikely(!buf32)) {
        tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
        return 0;
    }

    /* Header & Time */
    ms &= 0x0000FFFF;
    ms |= IST30XX_DEBUGGING_MAGIC;
    buf32[size++] = ms;

    buf32[size++] = *touch;
    buf32[size++] = *(touch + 1);

    for (i = 0; i < frame_cnt;i++) {
        buf32[i + 3] = frame[i];
        size++;
    }

    buf = (u8 *)buf32;
    size *= IST30XX_DATA_LEN;

    pDebugBuf->RingBufCtr += size;
    if (pDebugBuf->RingBufCtr > IST30XX_MAX_RINGBUF_SIZE) {
        pDebugBuf->RingBufOutIdx +=
            (pDebugBuf->RingBufCtr - IST30XX_MAX_RINGBUF_SIZE);
        if (pDebugBuf->RingBufOutIdx >= IST30XX_MAX_RINGBUF_SIZE)
            pDebugBuf->RingBufOutIdx -= IST30XX_MAX_RINGBUF_SIZE;

        pDebugBuf->RingBufCtr = IST30XX_MAX_RINGBUF_SIZE;
    }

    for (i = 0; i < size; i++) {
        if (pDebugBuf->RingBufInIdx == IST30XX_MAX_RINGBUF_SIZE)
            pDebugBuf->RingBufInIdx = 0;
        pDebugBuf->LogBuf[pDebugBuf->RingBufInIdx++] = *buf++;
    }

    kfree(buf32);

    return IST30XX_RINGBUF_NO_ERR;
}

/* sysfs: /sys/class/touch/tunes/debugging_mode */
ssize_t debugging_mode_store(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t size)
{
    int enable;
    int noise = 1;
    struct ist30xx_data *data = dev_get_drvdata(dev);

    sscanf(buf, "%d %d", &enable, &noise);

    if (enable == data->debugging_mode)
        return size;

    if (data->debugging_addr == 0x0) {
        tsp_err("check feature in firmware\n");
        return size;
    }

    if (unlikely((enable != 0) && (enable != 1))) {
        tsp_err("input data error(%d)\n", enable);
        return size;
    }

    ist30xx_debugging_init(data);
    data->debugging_noise = noise;
    data->debugging_mode = enable;
    tsp_info("set debugging mode: %s\n", enable ? "start" : "stop");

    if (data->status.power) {
        if (data->debug_mode || data->jig_mode || data->rec_mode ||
            data->debugging_mode) {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (IST30XX_DISABLE & 0xFFFF));
        } else {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (IST30XX_ENABLE & 0xFFFF));
        }

        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SET_DBG_MODE << 16) | (data->debugging_mode & 0xFFFF));
    }
     
    return size;
}

ssize_t debugging_mode_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);
    
    if (data->debugging_addr == 0x0) {
        tsp_err("check feature in firmware\n");
        return sprintf(buf, "%s", "FAIL");
    }

    ist30xx_debugging_init(data);
    data->debugging_noise = 1;
    data->debugging_mode = (data->debugging_mode ? 0 : 1);
    tsp_info("set debugging mode: %s\n",
            data->debugging_mode ? "start" : "stop");

    if (data->status.power) {
        if (data->debug_mode || data->jig_mode || data->rec_mode ||
            data->debugging_mode) {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (IST30XX_DISABLE & 0xFFFF));
        } else {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (IST30XX_ENABLE & 0xFFFF));
        }

        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SET_DBG_MODE << 16) | (data->debugging_mode & 0xFFFF));
    }

    return sprintf(buf, "%s", data->debugging_mode ? "ENABLE" : "DISABLE");
}

/* sysfs: /sys/class/touch/tunes/debugging_status */
ssize_t debugging_status_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);
    
    return sprintf(buf, "%s", data->debugging_mode ? "ENABLE" : "DISABLE");
}

/* sysfs: /sys/class/touch/tunes/debugging_cnt */
ssize_t debugging_cnt_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    u32 cnt = (u32)ist30xx_get_debugging_cnt();

    tsp_verb("debugging cnt: %d\n", cnt);

    return sprintf(buf, "%08x", cnt);
}

/* sysfs: /sys/class/touch/tunes/debugging_frame */
ssize_t debugging_frame_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    int i;
    int count = 0;
    u32 *frame_data = NULL;
    char msg[10];
    struct ist30xx_data *data = dev_get_drvdata(dev);
    u32 read_frame_cnt = 0;
    u32 frame_cnt = MAX_FRAME_COUNT;

    frame_data = kzalloc(MAX_FRAME_COUNT, GFP_KERNEL);
    if (unlikely(!frame_data)) {
        tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
        return count;
    }

    buf[0] = '\0';

    read_frame_cnt = ist30xx_get_debugging_cnt();
    if (frame_cnt > read_frame_cnt)
        frame_cnt = read_frame_cnt;

    tsp_verb("num: %d of %d\n", frame_cnt, read_frame_cnt);

    frame_cnt /= IST30XX_DATA_LEN;

    if (ist30xx_get_frame(data, frame_data, frame_cnt)) {
        kfree(frame_data);
        return count;
    }

    for (i = 0; i < frame_cnt; i++) {
        count += sprintf(msg, "%08x", frame_data[i]);
        strcat(buf, msg);
    }

    kfree(frame_data);
    return count;
}

/* sysfs: /sys/class/touch/tunes/tbase */
ssize_t target_baseline_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    return sprintf(buf, "%08x", data->tsp_info.baseline);
}

IST30XX_RING_BUF RecordBuf;
IST30XX_RING_BUF *pRecordBuf;
bool recording_initialize = false;
void ist30xx_recording_init(struct ist30xx_data *data)
{
    if (!recording_initialize)
        pRecordBuf = &RecordBuf;        
    
    pRecordBuf->RingBufCtr = 0;
    pRecordBuf->RingBufInIdx = 0;
    pRecordBuf->RingBufOutIdx = 0;

    data->recording_scancnt = 0;

    recording_initialize = true;
}

u32 ist30xx_get_recording_cnt(void)
{
    if (!recording_initialize)
        return 0;

    return pRecordBuf->RingBufCtr;
}

int ist30xx_recording_get_frame(struct ist30xx_data *data, u32 *frame, u32 cnt)
{
    int i;
    u8 *buf = (u8 *)frame;

    if (!recording_initialize)
        return 0;

    cnt *= IST30XX_DATA_LEN;

    if (pRecordBuf->RingBufCtr < cnt)
        return IST30XX_RINGBUF_NOT_ENOUGH;
    
    for (i = 0; i < cnt; i++) {
        if (pRecordBuf->RingBufOutIdx == IST30XX_MAX_RINGBUF_SIZE)
            pRecordBuf->RingBufOutIdx = 0;

        *buf++ = (u8)pRecordBuf->LogBuf[pRecordBuf->RingBufOutIdx++];
        pRecordBuf->RingBufCtr--;
    }
    
    return IST30XX_RINGBUF_NO_ERR;
}

#define REC_ENTER_VALUE         (0xFFFFFFFF)
int ist30xx_recording_put_frame(struct ist30xx_data *data, u32 *frame,
        int frame_cnt)
{
    int i;
    int size = 0;
    u32 *buf32;
    u8 *buf;
   
    if (!recording_initialize)
        return 0;
   
    buf32 = kzalloc((frame_cnt + 1) * sizeof(u32), GFP_KERNEL);
    if (unlikely(!buf32)) {
        tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
        return 0;
    }

    for (i = 0; i < frame_cnt; i++) {
        buf32[i] = frame[i];
        size++;
    }
    buf32[size++] = REC_ENTER_VALUE;

    buf = (u8 *)buf32;
    size *= IST30XX_DATA_LEN;

    pRecordBuf->RingBufCtr += size;
    if (pRecordBuf->RingBufCtr > IST30XX_MAX_RINGBUF_SIZE) {
        pRecordBuf->RingBufOutIdx +=
            (pRecordBuf->RingBufCtr - IST30XX_MAX_RINGBUF_SIZE);
        if (pRecordBuf->RingBufOutIdx >= IST30XX_MAX_RINGBUF_SIZE)
            pRecordBuf->RingBufOutIdx -= IST30XX_MAX_RINGBUF_SIZE;

        pRecordBuf->RingBufCtr = IST30XX_MAX_RINGBUF_SIZE;
    }

    for (i = 0; i < size; i++) {
        if (pRecordBuf->RingBufInIdx == IST30XX_MAX_RINGBUF_SIZE)
            pRecordBuf->RingBufInIdx = 0;
        pRecordBuf->LogBuf[pRecordBuf->RingBufInIdx++] = *buf++;
    }

    kfree(buf32);

    return IST30XX_RINGBUF_NO_ERR;
}

/* sysfs: /sys/class/touch/tunes/rec_mode */
ssize_t ist30xx_rec_mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int count;
    int mode;
    int delay = 0;
    char header[128];
    mm_segment_t old_fs = { 0 };
    struct file *fp = NULL;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;

    snprintf(data->rec_file_name, 128, "/sdcard/%s", IST30XX_REC_FILENAME);

    sscanf(buf, "%d %d", &mode, &delay);

    old_fs = get_fs();
    set_fs(get_ds());

    if (mode) {
        fp = filp_open(data->rec_file_name, O_CREAT|O_WRONLY|O_TRUNC, 0);
        if (IS_ERR(fp)) {
            tsp_err("file %s open error:%d\n", data->rec_file_name,
                    PTR_ERR(fp));
            goto err_file_open;
        }

        count = snprintf(header, 128, "2 2 %d %d %d %d\n", tsp->node.self_len,
                tsp->ch_num.tx, tsp->ch_num.rx, data->rec_size);

        fp->f_op->write(fp, header, count, &fp->f_pos);
        fput(fp);

        filp_close(fp, NULL);
    }

    data->rec_mode = mode;
    tsp_info("rec mode: %s\n", mode ? "start" : "stop");
    if (mode) {
        data->rec_delay = delay;
        tsp_info("rec delay: %dms\n", data->rec_delay);
    }

    ist30xx_recording_init(data);

    if (data->status.power) {
        if (data->debug_mode || data->jig_mode || data->rec_mode ||
            data->debugging_mode) {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (IST30XX_DISABLE & 0xFFFF));
        } else {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (IST30XX_ENABLE & 0xFFFF));
        }

        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SET_REC_MODE << 16) | (data->rec_mode & 0xFFFF));

        if (data->rec_mode) {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                    (eHCOM_SET_REC_MODE << 16) | (IST30XX_START_SCAN & 0xFFFF));
        }
    }

err_file_open:
    set_fs(old_fs);

    return size;
}

ssize_t ist30xx_rec_mode_show(struct device *dev, struct device_attribute *attr,
              char *buf)
{
    int count;
    char header[128];
    mm_segment_t old_fs = { 0 };
    struct file *fp = NULL;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    TSP_INFO *tsp = &data->tsp_info;

    old_fs = get_fs();
    set_fs(get_ds());

    if (data->rec_mode) {
        data->rec_mode = 0;
    } else {
        snprintf(data->rec_file_name, 128, "/sdcard/%s", IST30XX_REC_FILENAME);
        fp = filp_open(data->rec_file_name, O_CREAT|O_WRONLY|O_TRUNC, 0);
        if (IS_ERR(fp)) {
            tsp_err("file %s open error:%d\n", data->rec_file_name,
                    PTR_ERR(fp));
            goto err_file_open;
        }

        count = snprintf(header, 128, "2 2 %d %d %d %d\n", tsp->node.self_len,
                tsp->ch_num.tx, tsp->ch_num.rx, data->rec_size);

        fp->f_op->write(fp, header, count, &fp->f_pos);
        fput(fp);

        filp_close(fp, NULL);

        data->rec_mode = 1;
    }

    tsp_info("rec mode: %s\n", data->rec_mode ? "start" : "stop");
    if (data->rec_mode) {
        tsp_info("rec delay: %dms\n", data->rec_delay);
    }

    ist30xx_recording_init(data);

    if (data->status.power) {
        if (data->debug_mode || data->jig_mode || data->rec_mode ||
            data->debugging_mode) {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (IST30XX_DISABLE & 0xFFFF));
        } else {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SLEEP_MODE_EN << 16) | (IST30XX_ENABLE & 0xFFFF));
        }

        ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                (eHCOM_SET_REC_MODE << 16) | (data->rec_mode & 0xFFFF));

        if (data->rec_mode) {
            ist30xx_write_cmd(data, IST30XX_HIB_CMD,
                    (eHCOM_SET_REC_MODE << 16) | (IST30XX_START_SCAN & 0xFFFF));
        }
    }

err_file_open:
    set_fs(old_fs);

    return sprintf(buf, "%s", data->rec_mode ? "ENABLE" : "DISABLE");
}

/* sysfs: /sys/class/touch/tunes/recording_status */
ssize_t recording_status_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);
    
    return sprintf(buf, "%s", data->rec_mode ? "ENABLE" : "DISABLE");
}

/* sysfs: /sys/class/touch/tunes/change_rec_delay */
ssize_t change_rec_delay_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    data->rec_delay += 10;

    if (data->rec_delay > 50)
        data->rec_delay = 0;

    return sprintf(buf, "%d", data->rec_delay);
}

/* sysfs: /sys/class/touch/tunes/rec_delay */
ssize_t rec_delay_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    struct ist30xx_data *data = dev_get_drvdata(dev);

    return sprintf(buf, "%d", data->rec_delay);
}

/* sysfs: /sys/class/touch/tunes/recording_cnt */
ssize_t recording_cnt_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    u32 cnt = ist30xx_get_recording_cnt();

    tsp_verb("recording cnt: %d\n", cnt);

    return sprintf(buf, "%08x", cnt);
}

/* sysfs: /sys/class/touch/tunes/recording_frame */
ssize_t recording_frame_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    int i;
    int count = 0;
    u32 *frame_data = NULL;
    char msg[10];
    struct ist30xx_data *data = dev_get_drvdata(dev);
    u32 read_frame_cnt = 0;
    u32 frame_cnt = MAX_FRAME_COUNT;

    if (!recording_initialize)
        return count;

    frame_data = kzalloc(MAX_FRAME_COUNT, GFP_KERNEL);
    if (unlikely(!frame_data)) {
        tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
        return count;
    }

    buf[0] = '\0';

    read_frame_cnt = ist30xx_get_recording_cnt(); 
    if (frame_cnt > read_frame_cnt)
        frame_cnt = read_frame_cnt;

    tsp_verb("num: %d of %d\n", frame_cnt, read_frame_cnt);

    frame_cnt /= IST30XX_DATA_LEN;

    if (ist30xx_recording_get_frame(data, frame_data, frame_cnt)) {
        kfree(frame_data);
        return count;
    }

    for (i = 0; i < frame_cnt; i++) {
        if (frame_data[i] != REC_ENTER_VALUE) {
            count += sprintf(msg, "%08x ", frame_data[i]);
        } else {
            tsp_verb("%08X\n", frame_data[i]);
            count += sprintf(msg, "\n");
        }
        strcat(buf, msg);
    }

    kfree(frame_data);
    return count;
}

static DIRECT_INFO ist30xx_sec_info;
/* sysfs: /sys/class/touch/sys/sec_info */
ssize_t ist30xx_sec_info_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int ret = -EPERM;
    struct ist30xx_data *data = dev_get_drvdata(dev);
    DIRECT_INFO *sec_info = (DIRECT_INFO *)&ist30xx_sec_info;

    sscanf(buf, "%c %d %x %d", &sec_info->cmd, &sec_info->addr, &sec_info->val,
            &sec_info->size);

    if (sec_info->size == 0)
        sec_info->size = DIRECT_BUF_COUNT;

    tsp_debug("Sec info cmd: %c, index: %d, val: %x, display size: %d\n",
            sec_info->cmd, sec_info->addr, sec_info->val, sec_info->size);

    if (unlikely((sec_info->cmd != DIRECT_CMD_WRITE) &&
            (sec_info->cmd != DIRECT_CMD_READ))) {
        tsp_warn("Direct cmd is not correct!\n");
        return size;
    }

    if (ist30xx_intr_wait(data, 30) < 0)
        return size;

    if (sec_info->cmd == DIRECT_CMD_WRITE) {
        ret = ist30xx_write_sec_info(data, sec_info->addr, &sec_info->val, 1);
        if (ret)
            tsp_err("sec info write fail\n");
        else
            tsp_debug("sec_info index: %d, val: %x\n", sec_info->addr,
                    sec_info->val);
    }

    return size;
}

ssize_t ist30xx_sec_info_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    int i, count = 0;
    int ret = 0;
    int len;
    u32 *buf32;
    const int msg_len = 256;
    char msg[msg_len];
    struct ist30xx_data *data = dev_get_drvdata(dev);
    DIRECT_INFO *sec_info = (DIRECT_INFO *)&ist30xx_sec_info;

    if (unlikely(sec_info->cmd != DIRECT_CMD_READ))
        return sprintf(buf, "ex) echo r idx len size(display) > sec_info\n");

    len = sec_info->val;

    if (ist30xx_intr_wait(data, 30) < 0)
        return 0;

    if (!data->status.power)
        return 0;

    buf32 = kzalloc(len * sizeof(u32), GFP_KERNEL);
    if (unlikely(!buf32)) {
        tsp_err("failed to allocate %s %d\n", __func__, __LINE__);
        return count;
    }

    ret = ist30xx_read_sec_info(data, sec_info->addr, buf32, len);
    if (unlikely(ret)) {
        tsp_err("sec info read fail\n");
        count = sprintf(buf, "sec info read fail\n");
        kfree(buf32);
        return count;
    }

    for (i = 0; i < len; i++) {
        if (((i % sec_info->size) == 0) && (i != 0)) {
            count += snprintf(msg, msg_len, "\n");
            strncat(buf, msg, msg_len);
        }
        count += snprintf(msg, msg_len, "0x%08x ", buf32[i]);
        strncat(buf, msg, msg_len);
    }
    count += snprintf(msg, msg_len, "\n");
    strncat(buf, msg, msg_len);

    kfree(buf32);

    return count;
}

/* sysfs : node */
static DEVICE_ATTR(cp, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_cp_show, NULL);
static DEVICE_ATTR(cp_ium, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_cp_ium_show,
        NULL);
static DEVICE_ATTR(cdcbase, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_frame_cdcbase,
        NULL);
static DEVICE_ATTR(refresh, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_refresh_show,
        NULL);
static DEVICE_ATTR(nocp, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_nocp_show, NULL);
static DEVICE_ATTR(cdc, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_cdc_show, NULL);
static DEVICE_ATTR(base, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_base_show, NULL);
static DEVICE_ATTR(diff, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_diff_show, NULL);

/* sysfs : sys */
static DEVICE_ATTR(debug_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_debug_mode_store);
static DEVICE_ATTR(jig_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_jig_mode_store);
static DEVICE_ATTR(printk, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_printk_show,
        ist30xx_printk_store);
static DEVICE_ATTR(printk5, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_printk5_show,
        NULL);
static DEVICE_ATTR(printk6, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_printk6_show,
        NULL);
static DEVICE_ATTR(spay, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ist30xx_spay_store);
static DEVICE_ATTR(aod, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ist30xx_aod_store);
static DEVICE_ATTR(direct, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_direct_show,
        ist30xx_direct_store);
static DEVICE_ATTR(clb_time, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_calib_time_store);
static DEVICE_ATTR(clb, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_calib_show, NULL);
static DEVICE_ATTR(clb_result, S_IRUGO | S_IWUSR | S_IWGRP,
        ist30xx_calib_result_show, NULL);
static DEVICE_ATTR(tsp_power_on, S_IRUGO | S_IWUSR | S_IWGRP,
        ist30xx_power_on_show, NULL);
static DEVICE_ATTR(tsp_power_off, S_IRUGO | S_IWUSR | S_IWGRP,
        ist30xx_power_off_show, NULL);
static DEVICE_ATTR(errcnt, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_errcnt_store);
static DEVICE_ATTR(scancnt, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_scancnt_store);
static DEVICE_ATTR(timerms, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_timerms_store);
static DEVICE_ATTR(report_rate, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_report_rate_store);
static DEVICE_ATTR(idle_rate, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_idle_scan_rate_store);
static DEVICE_ATTR(mode_ta, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_ta_mode_store);
static DEVICE_ATTR(mode_call, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_call_mode_store);
static DEVICE_ATTR(mode_cover, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
        ist30xx_cover_mode_store);
static DEVICE_ATTR(ic_inform, S_IRUGO, ist30xx_read_ic_inform, NULL);
static DEVICE_ATTR(sec_info, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_sec_info_show,
        ist30xx_sec_info_store);

/* sysfs : tunes */
static DEVICE_ATTR(node_info, S_IRUGO | S_IWUSR | S_IWGRP, tunes_node_info_show,
        NULL);
static DEVICE_ATTR(regcmd, S_IRUGO | S_IWUSR | S_IWGRP, tunes_regcmd_show,
        tunes_regcmd_store);
static DEVICE_ATTR(reg, S_IRUGO | S_IWUSR | S_IWGRP, tunes_reg_show,
        tunes_reg_store);
static DEVICE_ATTR(adb, S_IRUGO | S_IWUSR | S_IWGRP, tunes_adb_show,
        tunes_adb_store);
static DEVICE_ATTR(algorithm, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_algr_show,
        ist30xx_algr_store);
static DEVICE_ATTR(intr_debug, S_IRUGO | S_IWUSR | S_IWGRP, intr_debug_show,
        intr_debug_store);
static DEVICE_ATTR(intr_debug2, S_IRUGO | S_IWUSR | S_IWGRP, intr_debug2_show,
        intr_debug2_store);
static DEVICE_ATTR(intr_debug3, S_IRUGO | S_IWUSR | S_IWGRP, intr_debug3_show,
        intr_debug3_store);
static DEVICE_ATTR(debugging_mode, S_IRUGO | S_IWUSR | S_IWGRP,
        debugging_mode_show, debugging_mode_store);
static DEVICE_ATTR(debugging_status, S_IRUGO | S_IWUSR | S_IWGRP,
        debugging_status_show, NULL);
static DEVICE_ATTR(debugging_cnt, S_IRUGO | S_IWUSR | S_IWGRP,
        debugging_cnt_show, NULL);
static DEVICE_ATTR(debugging_frame, S_IRUGO | S_IWUSR | S_IWGRP,
        debugging_frame_show, NULL);
static DEVICE_ATTR(tbase, S_IRUGO | S_IWUSR | S_IWGRP, target_baseline_show,
        NULL);
static DEVICE_ATTR(rec_mode, S_IRUGO | S_IWUSR | S_IWGRP, ist30xx_rec_mode_show,
        ist30xx_rec_mode_store);
static DEVICE_ATTR(change_rec_delay, S_IRUGO | S_IWUSR | S_IWGRP,
        change_rec_delay_show, NULL);
static DEVICE_ATTR(rec_delay, S_IRUGO | S_IWUSR | S_IWGRP, rec_delay_show, NULL);
static DEVICE_ATTR(recording_status, S_IRUGO | S_IWUSR | S_IWGRP, recording_status_show,
        NULL);
static DEVICE_ATTR(recording_cnt, S_IRUGO | S_IWUSR | S_IWGRP, recording_cnt_show, NULL);
static DEVICE_ATTR(recording_frame, S_IRUGO | S_IWUSR | S_IWGRP, recording_frame_show,
        NULL);

static struct attribute *node_attributes[] = {
    &dev_attr_cp.attr,
    &dev_attr_cp_ium.attr,    
    &dev_attr_cdcbase.attr,
    &dev_attr_refresh.attr,
    &dev_attr_nocp.attr,
    &dev_attr_cdc.attr,
    &dev_attr_base.attr,
    &dev_attr_diff.attr,
    NULL,
};

static struct attribute *sys_attributes[] = {
    &dev_attr_debug_mode.attr,
    &dev_attr_jig_mode.attr,
    &dev_attr_printk.attr,
    &dev_attr_printk5.attr,
    &dev_attr_printk6.attr,
    &dev_attr_spay.attr,
    &dev_attr_aod.attr,
    &dev_attr_direct.attr,
    &dev_attr_clb_time.attr,
    &dev_attr_clb.attr,
    &dev_attr_clb_result.attr,
    &dev_attr_tsp_power_on.attr,
    &dev_attr_tsp_power_off.attr,
    &dev_attr_errcnt.attr,
    &dev_attr_scancnt.attr,
    &dev_attr_timerms.attr,
    &dev_attr_report_rate.attr,
    &dev_attr_idle_rate.attr,
    &dev_attr_mode_ta.attr,
    &dev_attr_mode_call.attr,
    &dev_attr_mode_cover.attr,
    &dev_attr_ic_inform.attr,
    &dev_attr_sec_info.attr,
    NULL,
};

static struct attribute *tunes_attributes[] = {
    &dev_attr_node_info.attr,
    &dev_attr_regcmd.attr,
    &dev_attr_reg.attr,
    &dev_attr_adb.attr,
    &dev_attr_algorithm.attr,
    &dev_attr_intr_debug.attr,
    &dev_attr_intr_debug2.attr,
    &dev_attr_intr_debug3.attr,
    &dev_attr_debugging_mode.attr,
    &dev_attr_debugging_cnt.attr,
    &dev_attr_debugging_frame.attr,
    &dev_attr_debugging_status.attr,
    &dev_attr_tbase.attr,
    &dev_attr_rec_mode.attr,
    &dev_attr_change_rec_delay.attr,
    &dev_attr_rec_delay.attr,
    &dev_attr_recording_cnt.attr,
    &dev_attr_recording_frame.attr,
    &dev_attr_recording_status.attr,
    NULL,
};

static struct attribute_group node_attr_group = {
    .attrs = node_attributes,
};

static struct attribute_group sys_attr_group = {
    .attrs = sys_attributes,
};

static struct attribute_group tunes_attr_group = {
    .attrs = tunes_attributes,
};

extern struct class *ist30xx_class;
struct device *ist30xx_sys_dev;
struct device *ist30xx_tunes_dev;
struct device *ist30xx_node_dev;

int ist30xx_init_misc_sysfs(struct ist30xx_data *data)
{
    /* /sys/class/touch/sys */
    ist30xx_sys_dev = device_create(ist30xx_class, NULL, 0, data, "sys");

    /* /sys/class/touch/sys/... */
    if (unlikely(sysfs_create_group(&ist30xx_sys_dev->kobj,
                    &sys_attr_group)))
        tsp_err("Failed to create sysfs group(%s)!\n", "sys");

    /* /sys/class/touch/tunes */
    ist30xx_tunes_dev = device_create(ist30xx_class, NULL, 0, data, "tunes");

    /* /sys/class/touch/tunes/... */
    if (unlikely(sysfs_create_group(&ist30xx_tunes_dev->kobj,
                    &tunes_attr_group)))
        tsp_err("Failed to create sysfs group(%s)!\n", "tunes");

    /* /sys/class/touch/node */
    ist30xx_node_dev = device_create(ist30xx_class, NULL, 0, data, "node");

    /* /sys/class/touch/node/... */
    if (unlikely(sysfs_create_group(&ist30xx_node_dev->kobj,
                    &node_attr_group)))
        tsp_err("Failed to create sysfs group(%s)!\n", "node");

    ist30xx_debugging_init(data);

    return 0;
}
