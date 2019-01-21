/*
 *  Copyright (C) 2010, Imagis Technology Co. Ltd. All Rights Reserved.
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

#ifndef __IST30XXH_UPDATE_H__
#define __IST30XXH_UPDATE_H__

// Flash size
#define IST30XX_ROM_BASE_ADDR       (0)
#define IST30XX_ROM_TOTAL_SIZE      (0x10000)
#define IST30XX_IUM_BASE_ADDR       (0x400)
#define IST30XX_IUM_SIZE            (0x400)
#define IST30XX_IUM_PAGE_SIZE       (0x80)

// EEPROM register
#define rISP_BASE                   (0x40006000)
#define rISP_ACCESS_MODE            IST30XX_DA_ADDR(rISP_BASE | 0x00)
#define rISP_ADDRESS                IST30XX_DA_ADDR(rISP_BASE | 0x04)
#define rISP_DIN                    IST30XX_DA_ADDR(rISP_BASE | 0x08)
#define rISP_DOUT                   IST30XX_DA_ADDR(rISP_BASE | 0x0C)
#define rISP_ISP_EN                 IST30XX_DA_ADDR(rISP_BASE | 0x10)
#define rISP_AUTO_READ_CTRL         IST30XX_DA_ADDR(rISP_BASE | 0x14)
#define rISP_CRC                    IST30XX_DA_ADDR(rISP_BASE | 0x18)
#define rISP_COMPARE_MODE           IST30XX_DA_ADDR(rISP_BASE | 0x1C)
#define rISP_OP_CTRL                IST30XX_DA_ADDR(rISP_BASE | 0x20)
#define rISP_PAGE_MODE              IST30XX_DA_ADDR(rISP_BASE | 0x38)
#define rISP_STATUS                 IST30XX_DA_ADDR(rISP_BASE | 0x80)

// DMA
#define rDMA_BASE                   (0x4000A000)
#define rDMA1_CTL                   IST30XX_DA_ADDR(rDMA_BASE | 0x010)
#define rDMA1_SRCADDR               IST30XX_DA_ADDR(rDMA_BASE | 0x014)
#define rDMA1_DSTADDR               IST30XX_DA_ADDR(rDMA_BASE | 0x018)

// I2C
#define rI2C_CTRL                   IST30XX_DA_ADDR(0x30000000)

// F/W update Info
#define IST30XX_FW_NAME             "ist30xxh.fw"
#define IST30XX_BIN_NAME            "ist30xxh.bin"
#define IST30XX_IUM_NAME            "ist30xxh_ium.bin"

// Update func
#define MASK_UPDATE_INTERNAL        (1)
#define MASK_UPDATE_FW              (2)
#define MASK_UPDATE_SDCARD          (3)
#define MASK_UPDATE_ERASE           (4)

// Version flag
#define FLAG_MAIN                   (1)
#define FLAG_FW                     (2)
#define FLAG_CORE                   (3)
#define FLAG_TEST                   (4)

int ist30xx_set_padctrl2(struct ist30xx_data *data);
int ist30xx_set_i2c_32bit(struct ist30xx_data *data);
int ist30xx_isp_enable(struct ist30xx_data *data, bool enable);
int ist30xx_isp_read_ium(struct ist30xx_data *data, u32 addr, u32 *buf32,
        int size);
int ist30xx_write_sec_info(struct ist30xx_data *data, u8 idx, u32 *buf32,
        int len);
int ist30xx_read_sec_info(struct ist30xx_data *data, u8 idx, u32 *buf32,
        int len);
int ist30xx_get_update_info(struct ist30xx_data *data, const u8 *buf,
        const u32 size);
int ist30xx_get_tsp_info(struct ist30xx_data *data);
void ist30xx_print_info(struct ist30xx_data *data);
u32 ist30xx_parse_ver(struct ist30xx_data *data, int flag, const u8 *buf);
int ist30xx_fw_update(struct ist30xx_data *data, const u8 *buf, int size);
int ist30xx_fw_recovery(struct ist30xx_data *data);
int ist30xx_auto_bin_update(struct ist30xx_data *data);
int ist30xx_calib_wait(struct ist30xx_data *data);
int ist30xx_calibrate(struct ist30xx_data *data, int wait_cnt);
int ist30xx_init_update_sysfs(struct ist30xx_data *data);

#endif  // __IST30XXH_UPDATE_H__
