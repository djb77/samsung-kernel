/* Himax Android Driver Sample Code for HMX83100 chipset
*
* Copyright (C) 2015 Himax Corporation.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include "himax_83100.h"

#ifdef CONFIG_SEC_INCELL
#include <linux/sec_incell.h>
#endif

#define HIMAX_I2C_RETRY_TIMES 10
#define SUPPORT_FINGER_DATA_CHECKSUM 0x0F
#define TS_WAKE_LOCK_TIMEOUT		(2 * HZ)
#define FRAME_COUNT 5

#if defined(HX_AUTO_UPDATE_FW)
	static unsigned char i_CTPM_FW[]=
	{
		//#include "HX83100_Firmware_Version_FF23.i" 
		#include "data/himax_on5.i"
	};
#endif

#ifdef HX_ESD_WORKAROUND
extern void HX_report_ESD_event(void);
#endif

//static int		tpd_keys_local[HX_KEY_MAX_COUNT] = HX_KEY_ARRAY; // for Virtual key array
static unsigned char	IC_CHECKSUM               = 0;
static unsigned char	IC_TYPE                   = 0;

static int		HX_TOUCH_INFO_POINT_CNT   = 0;

static int		HX_RX_NUM                 = 0;
static int		HX_TX_NUM                 = 0;
static int		HX_BT_NUM                 = 0;
static int		HX_X_RES                  = 0;
static int		HX_Y_RES                  = 0;
static int		HX_MAX_PT                 = 0;
static bool		HX_XY_REVERSE             = false;
static bool		HX_INT_IS_EDGE            = false;

static unsigned long	FW_VER_MAJ_FLASH_ADDR;
static unsigned long 	FW_VER_MAJ_FLASH_LENG;
static unsigned long 	FW_VER_MIN_FLASH_ADDR;
static unsigned long 	FW_VER_MIN_FLASH_LENG;
static unsigned long 	CFG_VER_MAJ_FLASH_ADDR;
static unsigned long 	CFG_VER_MAJ_FLASH_LENG;
static unsigned long 	CFG_VER_MIN_FLASH_ADDR;
static unsigned long 	CFG_VER_MIN_FLASH_LENG;

#ifdef HX_TP_SYS_DIAG
#ifdef HX_TP_SYS_2T2R
static int		HX_RX_NUM_2					= 0;
static int 		HX_TX_NUM_2					= 0;
#endif
static int  touch_monitor_stop_flag 		= 0;
static int 	touch_monitor_stop_limit 		= 5;
#endif

static uint8_t 	vk_press = 0x00;
static uint8_t 	AA_press = 0x00;
static uint8_t	IC_STATUS_CHECK	= 0xAA;
static uint8_t 	EN_NoiseFilter = 0x00;
static uint8_t	Last_EN_NoiseFilter = 0x00;
static int	hx_point_num	= 0;																	// for himax_ts_work_func use
static int	p_point_num	= 0xFFFF;
static int	tpd_key	   	= 0x00;
static int	tpd_key_old	= 0x00;

static bool	config_load		= false;
static struct himax_config *config_selected = NULL;

//static int iref_number = 11;
//static bool iref_found = false;    

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#if !defined(SEC_FACTORY_TEST)
static struct kobject *android_touch_kobj = NULL;// Sys kobject variable
#endif
#endif
//++++++++++incell function++++++++++++++++++
static bool isBusrtOn= false;
static unsigned char i_TP_CRC_FW_128K[]=
{
	#include "data/HX83100_CRC_128.i"
};
static unsigned char i_TP_CRC_FW_64K[]=
{
	#include "data/HX83100_CRC_64.i"
};
static unsigned char i_TP_CRC_FW_124K[]=
{
	#include "data/HX83100_CRC_124.i"
};
static unsigned char i_TP_CRC_FW_60K[]=
{
	#include "data/HX83100_CRC_60.i"
};
//-------------incell function----------------------

#ifdef HX_LCD_FEATURE
extern void HX_83100_chip_reset(void);
extern int HX_83100_display_on(void);
#endif

#ifdef USE_OPEN_CLOSE
static int  himax_ts_open(struct input_dev *dev);
static void himax_ts_close(struct input_dev *dev);
#endif

#ifdef CONFIG_SEC_INCELL
static void himax_tsp_disable(struct incell_driver_data *param);
static void himax_tsp_enable(struct incell_driver_data *param);
static void himax_tsp_esd(struct incell_driver_data *param);
#endif

static int himax_hand_shaking(void)    //0:Running, 1:Stop, 2:I2C Fail
{
	int ret, result;
	uint8_t hw_reset_check[1];
	uint8_t hw_reset_check_2[1];
	uint8_t buf0[2];

	memset(hw_reset_check, 0x00, sizeof(hw_reset_check));
	memset(hw_reset_check_2, 0x00, sizeof(hw_reset_check_2));

	buf0[0] = 0xF2;
	if (IC_STATUS_CHECK == 0xAA) {
		buf0[1] = 0xAA;
		IC_STATUS_CHECK = 0x55;
	} else {
		buf0[1] = 0x55;
		IC_STATUS_CHECK = 0xAA;
	}

	ret = i2c_himax_master_write(private_ts->client, buf0, 2, DEFAULT_RETRY_CNT);
	if (ret < 0) {
		E("[Himax]:write 0xF2 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(50); 
  	
	buf0[0] = 0xF2;
	buf0[1] = 0x00;
	ret = i2c_himax_master_write(private_ts->client, buf0, 2, DEFAULT_RETRY_CNT);
	if (ret < 0) {
		E("[Himax]:write 0x92 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(2);
  	
	ret = i2c_himax_read(private_ts->client, 0xD1, hw_reset_check, 1, DEFAULT_RETRY_CNT);
	if (ret < 0) {
		E("[Himax]:i2c_himax_read 0xD1 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	
	if ((IC_STATUS_CHECK != hw_reset_check[0])) {
		msleep(2);
		ret = i2c_himax_read(private_ts->client, 0xD1, hw_reset_check_2, 1, DEFAULT_RETRY_CNT);
		if (ret < 0) {
			E("[Himax]:i2c_himax_read 0xD1 failed line: %d \n",__LINE__);
			goto work_func_send_i2c_msg_fail;
		}
	
		if (hw_reset_check[0] == hw_reset_check_2[0]) {
			result = 1; 
		} else {
			result = 0; 
		}
	} else {
		result = 0; 
	}
	
	return result;

work_func_send_i2c_msg_fail:
	return 2;
}
//++++++++++incell function++++++++++++++++++

static void himax_83100_BURST_INC0_EN(uint8_t auto_add_4_byte)
{
	uint8_t tmp_data[4];

	tmp_data[0] = 0x31;
	if ( i2c_himax_write(private_ts->client, 0x13 ,tmp_data, 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}
	
	tmp_data[0] = (0x10 | auto_add_4_byte);
	if ( i2c_himax_write(private_ts->client, 0x0D ,tmp_data, 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	isBusrtOn = true;	
}

static void RegisterRead83100(uint8_t *read_addr, int read_length, uint8_t *read_data)
{
	uint8_t tmp_data[4];
	int i = 0;
	int address = 0;

	if(read_length>256)
	{
		E("%s: read len over 256!\n", __func__);
		return;
	}
	himax_83100_BURST_INC0_EN(0);
	address = (read_addr[3] << 24) + (read_addr[2] << 16) + (read_addr[1] << 8) + read_addr[0];
	i = address;
	tmp_data[0] = (uint8_t)i;
	tmp_data[1] = (uint8_t)(i >> 8);
	tmp_data[2] = (uint8_t)(i >> 16);
	tmp_data[3] = (uint8_t)(i >> 24);
	if ( i2c_himax_write(private_ts->client, 0x00 ,tmp_data, 4, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}
	tmp_data[0] = 0x00;
	if ( i2c_himax_write(private_ts->client, 0x0C ,tmp_data, 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}
		
	if ( i2c_himax_read(private_ts->client, 0x08 ,read_data, read_length * 4, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}
	
}
	
static void himax_83100_Flash_Read(uint8_t *reg_byte, uint8_t *read_data)
{
    uint8_t tmpbyte[2];
    
    if ( i2c_himax_write(private_ts->client, 0x00 ,&reg_byte[0], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x01 ,&reg_byte[1], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x02 ,&reg_byte[2], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x03 ,&reg_byte[3], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

    tmpbyte[0] = 0x00;
    if ( i2c_himax_write(private_ts->client, 0x0C ,&tmpbyte[0], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(private_ts->client, 0x08 ,&read_data[0], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(private_ts->client, 0x09 ,&read_data[1], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(private_ts->client, 0x0A ,&read_data[2], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(private_ts->client, 0x0B ,&read_data[3], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(private_ts->client, 0x18 ,&tmpbyte[0], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}// No bus request

	if ( i2c_himax_read(private_ts->client, 0x0F ,&tmpbyte[1], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}// idle state

}

static void himax_83100_Flash_Write_Burst(uint8_t * reg_byte, uint8_t * write_data)
{
    uint8_t data_byte[8];
	int i = 0, j = 0;

     for (i = 0; i < 4; i++)
     { 
         data_byte[i] = reg_byte[i];
     }
     for (j = 4; j < 8; j++)
     {
         data_byte[j] = write_data[j-4];
     }
	 
	 if ( i2c_himax_write(private_ts->client, 0x00 ,data_byte, 8, 3) < 0) {
		 E("%s: i2c access fail!\n", __func__);
		 return;
	 }

}
static void himax_83100_Flash_Write_Burst_lenth(uint8_t *reg_byte, uint8_t *write_data, int length)
{
    uint8_t data_byte[256];
	int i = 0, j = 0;

    for (i = 0; i < 4; i++)
    {
        data_byte[i] = reg_byte[i];
    }
    for (j = 4; j < length + 4; j++)
    {
        data_byte[j] = write_data[j - 4];
    }
   
	if ( i2c_himax_write(private_ts->client, 0x00 ,data_byte, length + 4, 3) < 0) {
		 E("%s: i2c access fail!\n", __func__);
		 return;
	}
}

static void RegisterWrite83100(uint8_t *write_addr, int write_length, uint8_t *write_data)
{
	int i =0, address = 0;

	address = (write_addr[3] << 24) + (write_addr[2] << 16) + (write_addr[1] << 8) + write_addr[0];

	for (i = address; i < address + write_length * 4; i = i + 4)
	{
		if (write_length > 1)
		{
			himax_83100_BURST_INC0_EN(1);
		}
		else
		{
			himax_83100_BURST_INC0_EN(0);
		}
		himax_83100_Flash_Write_Burst_lenth(write_addr, write_data, write_length * 4);
	}
	
}
#if 0
static void himax_83100_Flash_Write(uint8_t * reg_byte, uint8_t * write_data)
{
	uint8_t tmpbyte[2];
    
    if ( i2c_himax_write(private_ts->client, 0x00 ,&reg_byte[0], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x01 ,&reg_byte[1], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x02 ,&reg_byte[2], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x03 ,&reg_byte[3], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x04 ,&write_data[0], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x05 ,&write_data[1], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x06 ,&write_data[2], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x07 ,&write_data[3], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}
	
    if (isBusrtOn == false)
    {
        tmpbyte[0] = 0x01;        
		if ( i2c_himax_write(private_ts->client, 0x0C ,&tmpbyte[0], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
		}
    }    
}
#endif
#if 0
static void himax_83100_Flash_Burst_Write(uint8_t * reg_byte, uint8_t * write_data)
{
    //uint8_t tmpbyte[2];
	int i = 0;
	
	if ( i2c_himax_write(private_ts->client, 0x00 ,&reg_byte[0], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x01 ,&reg_byte[1], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x02 ,&reg_byte[2], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x03 ,&reg_byte[3], 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

    // Write 256 bytes with continue burst mode
    for (i = 0; i < 256; i = i + 4)
    {
		if ( i2c_himax_write(private_ts->client, 0x04 ,&write_data[i], 1, 3) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}

		if ( i2c_himax_write(private_ts->client, 0x05 ,&write_data[i+1], 1, 3) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}

		if ( i2c_himax_write(private_ts->client, 0x06 ,&write_data[i+2], 1, 3) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}

		if ( i2c_himax_write(private_ts->client, 0x07 ,&write_data[i+3], 1, 3) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}
    }

    //if (isBusrtOn == false)
    //{
    //   tmpbyte[0] = 0x01;        
	//	if ( i2c_himax_write(private_ts->client, 0x0C ,&tmpbyte[0], 1, 3) < 0) {
	//	E("%s: i2c access fail!\n", __func__);
	//	return;
	//	}
    //}

}
#endif
static void himax_83100_SenseOff(void)
{
	//int a = 0;
	uint8_t wdt_off = 0x00;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[5];	

	himax_83100_BURST_INC0_EN(0);

	while(wdt_off == 0x00)
	{
		// 0x9000_800C ==> 0x0000_AC53
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x80; tmp_addr[0] = 0x0C;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0xAC; tmp_data[0] = 0x53;
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		//=====================================
        // Read Watch Dog disable password : 0x9000_800C ==> 0x0000_AC53
        //=====================================
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x80; tmp_addr[0] = 0x0C;
		RegisterRead83100(tmp_addr, 1, tmp_data);
		
		//Check WDT
		if (tmp_data[0] == 0x53 && tmp_data[1] == 0xAC && tmp_data[2] == 0x00 && tmp_data[3] == 0x00)
			wdt_off = 0x01;
		else
			wdt_off = 0x00;
	}
	// VCOM		//0x9008_806C ==> 0x0000_0001
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x6C;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x01;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	msleep(20);

	// 0x9000_0010 ==> 0x0000_00DA
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xDA;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	//=====================================
	// Read CPU clock off password : 0x9000_0010 ==> 0x0000_00DA
	//=====================================
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	RegisterRead83100(tmp_addr, 1, tmp_data);
	I("%s: CPU clock off password data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n", __func__
		,tmp_data[0],tmp_data[1],tmp_data[2],tmp_data[3]);

}

static void himax_83100_Interface_on(void)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[5];

	//===========================================
    //  Any Cmd for ineterface on : 0x9000_0000 ==> 0x0000_0000
    //===========================================
    tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
    himax_83100_Flash_Read(tmp_addr, tmp_data); //avoid RD/WR fail    
}

static bool wait_wip(int Timing)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t in_buffer[10];
	//uint8_t out_buffer[20];
	int retry_cnt = 0;

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	in_buffer[0] = 0x01;

	do
	{
		//=====================================
		// SPI Transfer Control : 0x8000_0020 ==> 0x4200_0003
		//=====================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x42; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x03;
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		//=====================================
		// SPI Command : 0x8000_0024 ==> 0x0000_0005
		// read 0x8000_002C for 0x01, means wait success
		//=====================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x05;
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		in_buffer[0] = in_buffer[1] = in_buffer[2] = in_buffer[3] = 0xFF;
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x2C;
		RegisterRead83100(tmp_addr, 1, in_buffer);
		
		if ((in_buffer[0] & 0x01) == 0x00)
			return true;

		retry_cnt++;
		
		if (in_buffer[0] != 0x00 || in_buffer[1] != 0x00 || in_buffer[2] != 0x00 || in_buffer[3] != 0x00)
        	I("%s:Wait wip retry_cnt:%d, buffer[0]=%d, buffer[1]=%d, buffer[2]=%d, buffer[3]=%d \n", __func__, 
            retry_cnt,in_buffer[0],in_buffer[1],in_buffer[2],in_buffer[3]);

		if (retry_cnt > 100)
        {        	
			E("%s: Wait wip error!\n", __func__);
            return false;
        }
		msleep(Timing);
	}while ((in_buffer[0] & 0x01) == 0x01);
	return true;
}

void himax_83100_SenseOn(uint8_t FlashMode)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[128];

	himax_83100_Interface_on();
	himax_83100_BURST_INC0_EN(0);
	//CPU reset
	// 0x9000_0014 ==> 0x0000_00CA
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x14;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xCA;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	//=====================================
	// Read pull low CPU reset signal : 0x9000_0014 ==> 0x0000_00CA
	//=====================================
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x14;
	RegisterRead83100(tmp_addr, 1, tmp_data);

	I("%s: check pull low CPU reset signal  data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n", __func__
	,tmp_data[0],tmp_data[1],tmp_data[2],tmp_data[3]);

	// 0x9000_0014 ==> 0x0000_0000
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x14;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	//=====================================
	// Read revert pull low CPU reset signal : 0x9000_0014 ==> 0x0000_0000
	//=====================================
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x14;
	RegisterRead83100(tmp_addr, 1, tmp_data);

	I("%s: revert pull low CPU reset signal data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n", __func__
	,tmp_data[0],tmp_data[1],tmp_data[2],tmp_data[3]);

	//=====================================
  // Reset TCON
  //=====================================
  tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0xE0;
  tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
  himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
	msleep(10);
  tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0xE0;
  tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x01;
  himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	if (FlashMode == 0x00)	//SRAM
	{
		//=====================================
		//			Re-map
		//=====================================
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xF1;
		himax_83100_Flash_Write_Burst_lenth(tmp_addr, tmp_data, 4);
		I("%s:83100_Chip_Re-map ON\n", __func__);
	}
	else
	{
		//=====================================
		//			Re-map off
		//=====================================
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		himax_83100_Flash_Write_Burst_lenth(tmp_addr, tmp_data, 4);
		I("%s:83100_Chip_Re-map OFF\n", __func__);
	}
	//=====================================
	//			CPU clock on
	//=====================================
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_83100_Flash_Write_Burst_lenth(tmp_addr, tmp_data, 4);
}

static void himax_83100_Chip_Erase(void)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];

	himax_83100_BURST_INC0_EN(0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	//=====================================
	// Chip Erase
	// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
	//				  2. 0x8000_0024 ==> 0x0000_0006
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
	tmp_data[3] = 0x47; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x06;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	//=====================================
	// Chip Erase
	// Erase Command : 0x8000_0024 ==> 0x0000_00C7
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xC7;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
	
	msleep(2000);
	
	if (!wait_wip(100))
		E("%s:83100_Chip_Erase Fail\n", __func__);

}

static bool himax_83100_Block_Erase(void)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];

	himax_83100_BURST_INC0_EN(0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	//=====================================
	// Chip Erase
	// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
	//				  2. 0x8000_0024 ==> 0x0000_0006
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
	tmp_data[3] = 0x47; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x06;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	//=====================================
	// Block Erase
	// Erase Command : 0x8000_0028 ==> 0x0000_0000 //SPI addr
	//				   0x8000_0020 ==> 0x6700_0000 //control
	//				   0x8000_0024 ==> 0x0000_0052 //BE
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
	
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
	tmp_data[3] = 0x67; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
	
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x52;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	msleep(1000);
	
	if (!wait_wip(100))
	{
		E("%s:83100_Erase Fail\n", __func__);
		return false;
	}
	else
	{
		return true;
	}		

}

static bool himax_83100_Sector_Erase(int start_addr)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	int page_prog_start = 0;

	himax_83100_BURST_INC0_EN(0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
	for (page_prog_start = start_addr; page_prog_start < start_addr + 0x0F000; page_prog_start = page_prog_start + 0x1000)
		{
			//=====================================
			// Chip Erase
			// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
			//				  2. 0x8000_0024 ==> 0x0000_0006
			//=====================================
			tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
			tmp_data[3] = 0x47; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
			himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

			tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
			tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x06;
			himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

			 //=====================================
			 // Sector Erase
			 // Erase Command : 0x8000_0028 ==> 0x0000_0000 //SPI addr
			 // 				0x8000_0020 ==> 0x6700_0000 //control
			 // 				0x8000_0024 ==> 0x0000_0020 //SE
			 //=====================================
			 tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
			 if (page_prog_start < 0x100)
			 {
				 tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = (uint8_t)page_prog_start;
			 }
			 else if (page_prog_start >= 0x100 && page_prog_start < 0x10000)
			 {
				 tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = (uint8_t)(page_prog_start >> 8); tmp_data[0] = (uint8_t)page_prog_start;
			 }
			 else if (page_prog_start >= 0x10000 && page_prog_start < 0x1000000)
			 {
				 tmp_data[3] = 0x00; tmp_data[2] = (uint8_t)(page_prog_start >> 16); tmp_data[1] = (uint8_t)(page_prog_start >> 8); tmp_data[0] = (uint8_t)page_prog_start;
			 }
			 himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
			
			 tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
			 tmp_data[3] = 0x67; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
			 himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
			
			 tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
			 tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x20;
			 himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

			 msleep(200);

			if (!wait_wip(100))
			{
				E("%s:83100_Erase Fail\n", __func__);
				return false;
			}
		}	
		return true;
}

#if 0
static bool himax_83100_Verify(uint8_t *FW_File, int FW_Size)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t out_buffer[20];
	uint8_t in_buffer[260];

	int fail_addr=0, fail_cnt=0;
	int page_prog_start = 0;
	int i = 0;
 
	himax_83100_Interface_on();
	himax_83100_BURST_INC0_EN(0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_83100_Flash_Write(tmp_addr, tmp_data);

	for (page_prog_start = 0; page_prog_start < FW_Size; page_prog_start = page_prog_start + 256)
	{
		//=================================
		// SPI Transfer Control
		// Set 256 bytes page read : 0x8000_0020 ==> 0x6940_02FF
		// Set read start address  : 0x8000_0028 ==> 0x0000_0000   
		// Set command			   : 0x8000_0024 ==> 0x0000_003B		  
		//=================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x69; tmp_data[2] = 0x40; tmp_data[1] = 0x02; tmp_data[0] = 0xFF;
		himax_83100_Flash_Write(tmp_addr, tmp_data);
	
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
		if (page_prog_start < 0x100)
		{
			tmp_data[3] = 0x00; 
			tmp_data[2] = 0x00; 
			tmp_data[1] = 0x00; 
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x100 && page_prog_start < 0x10000)
		{
			tmp_data[3] = 0x00; 
			tmp_data[2] = 0x00; 
			tmp_data[1] = (uint8_t)(page_prog_start >> 8); 
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x10000 && page_prog_start < 0x1000000)
		{
			tmp_data[3] = 0x00; 
			tmp_data[2] = (uint8_t)(page_prog_start >> 16); 
			tmp_data[1] = (uint8_t)(page_prog_start >> 8); 
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		himax_83100_Flash_Write(tmp_addr, tmp_data);
	
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x3B;
		himax_83100_Flash_Write(tmp_addr, tmp_data);
	
		//==================================
		// AHB_I2C Burst Read
		// Set SPI data register : 0x8000_002C ==> 0x00
		//==================================
		out_buffer[0] = 0x2C;
		out_buffer[1] = 0x00;
		out_buffer[2] = 0x00;
		out_buffer[3] = 0x80;
		i2c_himax_write(private_ts->client, 0x00 ,out_buffer, 4, 3);
	
		//==================================
		// Read access : 0x0C ==> 0x00
		//==================================
		out_buffer[0] = 0x00;
		i2c_himax_write(private_ts->client, 0x0C ,out_buffer, 1, 3);
			
		//==================================
		// Read 128 bytes two times
		//==================================
		i2c_himax_read(private_ts->client, 0x08 ,in_buffer, 128, 3);		
		for (i = 0; i < 128; i++)
			flash_buffer[i + page_prog_start] = in_buffer[i];
	
		i2c_himax_read(private_ts->client, 0x08 ,in_buffer, 128, 3);
		for (i = 0; i < 128; i++)
			flash_buffer[(i + 128) + page_prog_start] = in_buffer[i];
	
		//tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x2C;
		//RegisterRead83100(tmp_addr, 32, out in_buffer);
		//for (int i = 0; i < 128; i++)
		//	  flash_buffer[i + page_prog_start] = in_buffer[i];
		//tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x2C;
		//RegisterRead83100(tmp_addr, 32, out in_buffer);
		//for (int i = 0; i < 128; i++)
		//	  flash_buffer[i + page_prog_start] = in_buffer[i];
	
		I("%s:Verify Progress: %x\n", __func__, page_prog_start);
	}
	
	fail_cnt = 0;
	for (i = 0; i < FW_Size; i++)
	{
		if (FW_File[i] != flash_buffer[i])
		{
			if (fail_cnt == 0)
				fail_addr = i;
	
			fail_cnt++;
			//E("%s Fail Block:%x\n", __func__, i);
			//return false;
		}
	}
	if (fail_cnt > 0)
	{
		E("%s:Start Fail Block:%x and fail block count=%x\n" , __func__,fail_addr,fail_cnt);
		return false;
	}
	
	I("%s:Byte read verify pass.\n", __func__);
	return true;

}
#endif

static void himax_83100_Sram_Write(uint8_t *FW_content)
{
	int i = 0;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[128];
	int FW_length = 0x4000; // 0x4000 = 16K bin file
	
	himax_83100_SenseOff();

	for (i = 0; i < FW_length; i = i + 128) 
	{
		himax_83100_BURST_INC0_EN(1);

		if (i < 0x100)
		{
			tmp_addr[3] = 0x08; 
			tmp_addr[2] = 0x00; 
			tmp_addr[1] = 0x00; 
			tmp_addr[0] = i;
		}
		else if (i >= 0x100 && i < 0x10000)
		{
			tmp_addr[3] = 0x08; 
			tmp_addr[2] = 0x00; 
			tmp_addr[1] = (i >> 8); 
			tmp_addr[0] = i;
		}

		memcpy(&tmp_data[0], &FW_content[i], 128);
		himax_83100_Flash_Write_Burst_lenth(tmp_addr, tmp_data, 128);

	}

	if (!wait_wip(100))
		E("%s:83100_Sram_Write Fail\n", __func__);
}

static bool himax_83100_Sram_Verify(uint8_t *FW_File, int FW_Size)
{
	int i = 0;
	uint8_t out_buffer[20];
	uint8_t in_buffer[128];
	uint8_t *get_fw_content;

	get_fw_content = kzalloc(0x4000*sizeof(uint8_t), GFP_KERNEL);

	for (i = 0; i < 0x4000; i = i + 128)
	{
		himax_83100_BURST_INC0_EN(1);

		//==================================
		//	AHB_I2C Burst Read
		//==================================
		if (i < 0x100)
		{
			out_buffer[3] = 0x08; 
			out_buffer[2] = 0x00; 
			out_buffer[1] = 0x00; 
			out_buffer[0] = i;
		}
		else if (i >= 0x100 && i < 0x10000)
		{
			out_buffer[3] = 0x08; 
			out_buffer[2] = 0x00; 
			out_buffer[1] = (i >> 8); 
			out_buffer[0] = i;
		}

		if ( i2c_himax_write(private_ts->client, 0x00 ,out_buffer, 4, 3) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		out_buffer[0] = 0x00;		
		if ( i2c_himax_write(private_ts->client, 0x0C ,out_buffer, 1, 3) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}
		
		if ( i2c_himax_read(private_ts->client, 0x08 ,in_buffer, 128, 3) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}
		memcpy(&get_fw_content[i], &in_buffer[0], 128);
	}

	for (i = 0; i < FW_Size; i++)
		{
	        if (FW_File[i] != get_fw_content[i])
	        	{
					E("%s: fail! SRAM[%x]=%x NOT CRC_ifile=%x\n", __func__,i,get_fw_content[i],FW_File[i]);
		            return false;
	        	}
		}

	kfree(get_fw_content);
	
	return true;
}

static void himax_83100_Flash_Programming( uint8_t *FW_content, int FW_Size)
{
	int page_prog_start = 0;
	int program_length = 48;
	int i = 0, j = 0, k = 0;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t buring_data[256];    // Read for flash data, 128K
									 // 4 bytes for 0x80002C padding

	//himax_83100_Interface_on();
	himax_83100_BURST_INC0_EN(0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	for (page_prog_start = 0; page_prog_start < FW_Size; page_prog_start = page_prog_start + 256)
	{
		//msleep(5);
		//=====================================
		// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
		//				  2. 0x8000_0024 ==> 0x0000_0006
		//=====================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x47; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x06;
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		//=================================
		// SPI Transfer Control
		// Set 256 bytes page write : 0x8000_0020 ==> 0x610F_F000
		// Set read start address	: 0x8000_0028 ==> 0x0000_0000			
		//=================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x61; tmp_data[2] = 0x0F; tmp_data[1] = 0xF0; tmp_data[0] = 0x00;
		// data bytes should be 0x6100_0000 + ((word_number)*4-1)*4096 = 0x6100_0000 + 0xFF000 = 0x610F_F000
		// Programmable size = 1 page = 256 bytes, word_number = 256 byte / 4 = 64
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
		//tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00; // Flash start address 1st : 0x0000_0000
		
		if (page_prog_start < 0x100)
		{
			tmp_data[3] = 0x00; 
			tmp_data[2] = 0x00; 
			tmp_data[1] = 0x00; 
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x100 && page_prog_start < 0x10000)
		{
			tmp_data[3] = 0x00; 
			tmp_data[2] = 0x00; 
			tmp_data[1] = (uint8_t)(page_prog_start >> 8); 
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x10000 && page_prog_start < 0x1000000)
		{
			tmp_data[3] = 0x00; 
			tmp_data[2] = (uint8_t)(page_prog_start >> 16); 
			tmp_data[1] = (uint8_t)(page_prog_start >> 8); 
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);


		//=================================
		// Send 16 bytes data : 0x8000_002C ==> 16 bytes data	  
		//=================================
		buring_data[0] = 0x2C;
		buring_data[1] = 0x00;
		buring_data[2] = 0x00;
		buring_data[3] = 0x80;
		
		for (i = /*0*/page_prog_start, j = 0; i < 16 + page_prog_start/**/; i++, j++)	/// <------ bin file
		{
			buring_data[j + 4] = FW_content[i];
		}

		
		if ( i2c_himax_write(private_ts->client, 0x00 ,buring_data, 20, 3) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}
		//=================================
		// Write command : 0x8000_0024 ==> 0x0000_0002
		//=================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x02;
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		//=================================
		// Send 240 bytes data : 0x8000_002C ==> 240 bytes data	 
		//=================================
		
		for (j = 0; j < 5; j++)
		{
			for (i = (page_prog_start + 16 + (j * 48)), k = 0; i < (page_prog_start + 16 + (j * 48)) + program_length; i++, k++)   /// <------ bin file
			{
				buring_data[k+4] = FW_content[i];//(byte)i;
			}

			if ( i2c_himax_write(private_ts->client, 0x00 ,buring_data, program_length+4, 3) < 0) {
				E("%s: i2c access fail!\n", __func__);
				return;
			}

		}
		
		if (!wait_wip(1))
			E("%s:83100_Flash_Programming Fail\n", __func__);
	}
}

static bool himax_83100_CheckChipVersion(void)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t ret_data = 0x00;
	int i = 0;
	himax_83100_SenseOff();
	for (i = 0; i < 5; i++)
	{
		// 1. Set DDREG_Req = 1 (0x9000_0020 = 0x0000_0001) (Lock register R/W from driver)
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x01;
		RegisterWrite83100(tmp_addr, 1, tmp_data);

		// 2. Set bank as 0 (0x8001_BD01 = 0x0000_0000)
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x01; tmp_addr[1] = 0xBD; tmp_addr[0] = 0x01;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		RegisterWrite83100(tmp_addr, 1, tmp_data);

		// 3. Read driver ID register RF4H 1 byte (0x8001_F401)
		//	  Driver register RF4H 1 byte value = 0x84H, read back value will become 0x84848484
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x01; tmp_addr[1] = 0xF4; tmp_addr[0] = 0x01;
		RegisterRead83100(tmp_addr, 1, tmp_data);
		ret_data = tmp_data[0];

		I("%s:Read driver IC ID = %X\n", __func__, ret_data);
		if (ret_data == 0x84)
		{
			IC_TYPE         = HX_83100_SERIES_PWON;
			himax_83100_SenseOn(0x01);
			ret_data = true;
		}
		else
		{
			ret_data = false;
			E("%s:Read driver ID register Fail:\n", __func__);
		}
	}
		// 4. After read finish, set DDREG_Req = 0 (0x9000_0020 = 0x0000_0000) (Unlock register R/W from driver)
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		RegisterWrite83100(tmp_addr, 1, tmp_data);
	himax_83100_SenseOn(0x01);
	return ret_data;
}

//-------------incell function-----------------------

#if 1
static int himax_83100_Check_CRC(int mode)
{
	bool burnFW_success = false;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	int tmp_value;
	int CRC_value = 0;

	memset(tmp_data, 0x00, sizeof(tmp_data));
	
	if (1)
	{
		if(mode == fw_image_60k)
		{
			himax_83100_Sram_Write(i_TP_CRC_FW_60K);
			burnFW_success = himax_83100_Sram_Verify(i_TP_CRC_FW_60K, 0x4000);
		}
		else if(mode == fw_image_64k)
		{
			himax_83100_Sram_Write(i_TP_CRC_FW_64K);
			burnFW_success = himax_83100_Sram_Verify(i_TP_CRC_FW_64K, 0x4000);
		}
		else if(mode == fw_image_124k)
		{
			himax_83100_Sram_Write(i_TP_CRC_FW_124K);
			burnFW_success = himax_83100_Sram_Verify(i_TP_CRC_FW_124K, 0x4000);
		}
		else if(mode == fw_image_128k)
		{
			himax_83100_Sram_Write(i_TP_CRC_FW_128K);
			burnFW_success = himax_83100_Sram_Verify(i_TP_CRC_FW_128K, 0x4000);
		}
		if (burnFW_success)
		{
			I("%s: Start to do CRC FW mode=%d \n", __func__,mode);
			himax_83100_SenseOn(0x00);	// run CRC firmware 

			while(true)
			{
				msleep(100);

				tmp_addr[3] = 0x90; 
				tmp_addr[2] = 0x08; 
				tmp_addr[1] = 0x80; 
				tmp_addr[0] = 0x94;
				RegisterRead83100(tmp_addr, 1, tmp_data);

				I("%s: CRC from firmware is %x, %x, %x, %x \n", __func__,tmp_data[3],
					tmp_data[2],tmp_data[1],tmp_data[0]);

				if (tmp_data[3] == 0xFF && tmp_data[2] == 0xFF && tmp_data[1] == 0xFF && tmp_data[0] == 0xFF)
				{ 
					}
				else
					break;
			}

			CRC_value = tmp_data[3];

			tmp_value = tmp_data[2] << 8;
			CRC_value += tmp_value;

			tmp_value = tmp_data[1] << 16;
			CRC_value += tmp_value;

			tmp_value = tmp_data[0] << 24;
			CRC_value += tmp_value;

			I("%s: CRC Value is %x \n", __func__, CRC_value);

			//Close Remapping
	        //=====================================
	        //          Re-map close
	        //=====================================
	        tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
	        tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	        himax_83100_Flash_Write_Burst_lenth(tmp_addr, tmp_data, 4);
			return CRC_value;				
		}
		else
		{
			E("%s: SRAM write fail\n", __func__);
			return 0;
		}		
	}
	else
		I("%s: NO CRC Check File \n", __func__);

	return 0;
}

static bool Calculate_CRC_with_AP(unsigned char *FW_content , int CRC_from_FW, int mode)
{
	uint8_t tmp_data[4];
	int i, j;
	int fw_data;
	int fw_data_2;
	int CRC = 0xFFFFFFFF;
	int PolyNomial = 0x82F63B78;
	int length = 0;
	
	if (mode == fw_image_128k)
		length = 0x8000;
	else if (mode == fw_image_124k)
		length = 0x7C00;
	else if (mode == fw_image_64k)
		length = 0x4000;
	else //if (mode == fw_image_60k)
		length = 0x3C00;

	for (i = 0; i < length; i++)
	{
		fw_data = FW_content[i * 4 ];
		
		for (j = 1; j < 4; j++)
		{
			fw_data_2 = FW_content[i * 4 + j];
			fw_data += (fw_data_2) << (8 * j);
		}

		CRC = fw_data ^ CRC;

		for (j = 0; j < 32; j++)
		{
			if ((CRC % 2) != 0)
			{
				CRC = ((CRC >> 1) & 0x7FFFFFFF) ^ PolyNomial;				
			}
			else
			{
				CRC = (((CRC >> 1) ^ 0x7FFFFFFF)& 0x7FFFFFFF);				
			}
		}		
	}

	I("%s: CRC calculate from bin file is %x \n", __func__, CRC);
	
	tmp_data[0] = (uint8_t)(CRC >> 24);
	tmp_data[1] = (uint8_t)(CRC >> 16);
	tmp_data[2] = (uint8_t)(CRC >> 8);
	tmp_data[3] = (uint8_t) CRC;

	CRC = tmp_data[0];
	CRC += tmp_data[1] << 8;			
	CRC += tmp_data[2] << 16;
	CRC += tmp_data[3] << 24;
			
	I("%s: CRC calculate from bin file REVERSE %x \n", __func__, CRC);
	I("%s: CRC calculate from FWis %x \n", __func__, CRC_from_FW);
	if (CRC_from_FW == CRC)
		return true;
	else
		return false;
}	
#endif
static int fts_ctpm_fw_upgrade_with_sys_fs_60k(unsigned char *fw, int len, bool change_iref)
{
	int CRC_from_FW = 0;
	int burnFW_success = 0;
	
	if (len != 0x10000)   //64k
    {
    	E("%s: The file size is not 64K bytes\n", __func__);
    	return false;
	}
	himax_83100_SenseOff();
	msleep(500);
	himax_83100_Interface_on();
    if (!himax_83100_Sector_Erase(0x00000))
    	{     
            E("%s:Sector erase fail!Please restart the IC.\n", __func__);
            return false;
        }
	himax_83100_Flash_Programming(fw, 0x0F000);

	//burnFW_success = himax_83100_Verify(fw, len);
	//if(burnFW_success==false)
	//	return burnFW_success;
	
	CRC_from_FW = himax_83100_Check_CRC(fw_image_60k);
    burnFW_success = Calculate_CRC_with_AP(fw, CRC_from_FW,fw_image_60k);
	himax_83100_SenseOn(0x01);
	return burnFW_success;
}

static int fts_ctpm_fw_upgrade_with_sys_fs_64k(unsigned char *fw, int len, bool change_iref)
{
	int CRC_from_FW = 0;
	int burnFW_success = 0;
	
	if (len != 0x10000)   //64k
    {
    	E("%s: The file size is not 64K bytes\n", __func__);
    	return false;
	}
	himax_83100_SenseOff();
	msleep(500);
	himax_83100_Interface_on();
	himax_83100_Chip_Erase();
	himax_83100_Flash_Programming(fw, len);

	//burnFW_success = himax_83100_Verify(fw, len);
	//if(burnFW_success==false)
	//	return burnFW_success;
	
	CRC_from_FW = himax_83100_Check_CRC(fw_image_64k);
    burnFW_success = Calculate_CRC_with_AP(fw, CRC_from_FW,fw_image_64k);
	himax_83100_SenseOn(0x01);
	return burnFW_success;
}

static int fts_ctpm_fw_upgrade_with_sys_fs_124k(unsigned char *fw, int len, bool change_iref)
{
	int CRC_from_FW = 0;
	int burnFW_success = 0;
	
	if (len != 0x20000)   //128k
    {
    	E("%s: The file size is not 128K bytes\n", __func__);
    	return false;
	}
	himax_83100_SenseOff();
	msleep(500);
	himax_83100_Interface_on();
	if (!himax_83100_Block_Erase())
    	{     
            E("%s:Block erase fail!Please restart the IC.\n", __func__);
            return false;
        }
		
    if (!himax_83100_Sector_Erase(0x10000))
    	{     
            E("%s:Sector erase fail!Please restart the IC.\n", __func__);
            return false;
        }
	himax_83100_Flash_Programming(fw, 0x1F000);


	//burnFW_success = himax_83100_Verify(fw, len);
	//if(burnFW_success==false)
	//	return burnFW_success;
	
	CRC_from_FW = himax_83100_Check_CRC(fw_image_124k);
    burnFW_success = Calculate_CRC_with_AP(fw, CRC_from_FW,fw_image_124k);
	himax_83100_SenseOn(0x01);
	return burnFW_success;
}

static int fts_ctpm_fw_upgrade_with_sys_fs_128k(unsigned char *fw, int len, bool change_iref)
{
	int CRC_from_FW = 0;
	int burnFW_success = 0;
	
	if (len != 0x20000)   //128k
    {
    	E("%s: The file size is not 128K bytes\n", __func__);
    	return false;
	}
	himax_83100_SenseOff();
	msleep(500);
	himax_83100_Interface_on();
	himax_83100_Chip_Erase();

	himax_83100_Flash_Programming(fw, len);

	//burnFW_success = himax_83100_Verify(fw, len);
	//if(burnFW_success==false)
	//	return burnFW_success;
	
	CRC_from_FW = himax_83100_Check_CRC(fw_image_128k);
    burnFW_success = Calculate_CRC_with_AP(fw, CRC_from_FW,fw_image_128k);
	himax_83100_SenseOn(0x01);
	return burnFW_success;
}

static int himax_input_register(struct himax_ts_data *ts)
{
	int ret;
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		E("%s: Failed to allocate input device\n", __func__);
		return ret;
	}

	snprintf(ts->phys, sizeof(ts->phys),
		"%s/input0", dev_name(&ts->client->dev));
	ts->input_dev->name = "sec_touchscreen";
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->dev.parent = &ts->client->dev;

	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);

	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
#if defined(HX_SMART_WAKEUP)
	set_bit(KEY_POWER, ts->input_dev->keybit);
#endif
	set_bit(BTN_TOUCH, ts->input_dev->keybit);

	//set_bit(KEY_APP_SWITCH, ts->input_dev->keybit);

	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

	if (ts->protocol_type == PROTOCOL_TYPE_A) {
		//ts->input_dev->mtsize = ts->nFinger_support;
		input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID,
		0, 3, 0, 0);
	} else {/* PROTOCOL_TYPE_B */
		set_bit(MT_TOOL_FINGER, ts->input_dev->keybit);
		input_mt_init_slots(ts->input_dev, ts->nFinger_support, 0);
	}

	I("input_set_abs_params: mix_x %d, max_x %d, min_y %d, max_y %d\n",
		ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_y_min, ts->pdata->abs_y_max);

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_x_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,ts->pdata->abs_y_min, ts->pdata->abs_y_max, ts->pdata->abs_y_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR,ts->pdata->abs_pressure_min, ts->pdata->abs_pressure_max, ts->pdata->abs_pressure_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE,ts->pdata->abs_pressure_min, ts->pdata->abs_pressure_max, ts->pdata->abs_pressure_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR,ts->pdata->abs_width_min, ts->pdata->abs_width_max, ts->pdata->abs_pressure_fuzz, 0);

//	input_set_abs_params(ts->input_dev, ABS_MT_AMPLITUDE, 0, ((ts->pdata->abs_pressure_max << 16) | ts->pdata->abs_width_max), 0, 0);
//	input_set_abs_params(ts->input_dev, ABS_MT_POSITION, 0, (BIT(31) | (ts->pdata->abs_x_max << 16) | ts->pdata->abs_y_max), 0, 0);

#ifdef USE_OPEN_CLOSE
	ts->input_dev->open = himax_ts_open;
	ts->input_dev->close = himax_ts_close;
#endif

	input_set_drvdata(ts->input_dev, ts);

	return input_register_device(ts->input_dev);
}

static void calcDataSize(uint8_t finger_num)
{
	struct himax_ts_data *ts_data = private_ts;
	ts_data->coord_data_size = 4 * finger_num;
	ts_data->area_data_size = ((finger_num / 4) + (finger_num % 4 ? 1 : 0)) * 4;
	ts_data->raw_data_frame_size = 128 - ts_data->coord_data_size - ts_data->area_data_size - 4 - 4 - 1;
	ts_data->raw_data_nframes  = ((uint32_t)ts_data->x_channel * ts_data->y_channel +
																					ts_data->x_channel + ts_data->y_channel) / ts_data->raw_data_frame_size +
															(((uint32_t)ts_data->x_channel * ts_data->y_channel +
		  																		ts_data->x_channel + ts_data->y_channel) % ts_data->raw_data_frame_size)? 1 : 0;
	I("%s: coord_data_size: %d, area_data_size:%d, raw_data_frame_size:%d, raw_data_nframes:%d", __func__, ts_data->coord_data_size, ts_data->area_data_size, ts_data->raw_data_frame_size, ts_data->raw_data_nframes);
}

static void calculate_point_number(void)
{
	HX_TOUCH_INFO_POINT_CNT = HX_MAX_PT * 4 ;

	if ( (HX_MAX_PT % 4) == 0)
		HX_TOUCH_INFO_POINT_CNT += (HX_MAX_PT / 4) * 4 ;
	else
		HX_TOUCH_INFO_POINT_CNT += ((HX_MAX_PT / 4) +1) * 4 ;
}

void himax_touch_information(void)
{
	uint8_t cmd[4];
	char data[12] = {0};
	
	I("%s:IC_TYPE =%d\n", __func__,IC_TYPE);
	
	if(IC_TYPE == HX_85XX_F_SERIES_PWON)
		{
	  		data[0] = 0x8C;
	  		data[1] = 0x11;
		  	i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
		  	msleep(10);
		  	data[0] = 0x8B;
		  	data[1] = 0x00;
		  	data[2] = 0x70;
		  	i2c_himax_master_write(private_ts->client, &data[0],3,DEFAULT_RETRY_CNT);
		  	msleep(10);
		  	i2c_himax_read(private_ts->client, 0x5A, data, 12, DEFAULT_RETRY_CNT);
		  	HX_RX_NUM = data[0];				 // FE(70)
		  	HX_TX_NUM = data[1];				 // FE(71)
		  	HX_MAX_PT = (data[2] & 0xF0) >> 4; // FE(72)
#ifdef HX_EN_SEL_BUTTON
		  	HX_BT_NUM = (data[2] & 0x0F); //FE(72)
#endif
		  	if((data[4] & 0x04) == 0x04) {//FE(74)
				HX_XY_REVERSE = true;
			HX_Y_RES = data[6]*256 + data[7]; //FE(76),FE(77)
			HX_X_RES = data[8]*256 + data[9]; //FE(78),FE(79)
		  	} else {
			HX_XY_REVERSE = false;
			HX_X_RES = data[6]*256 + data[7]; //FE(76),FE(77)
			HX_Y_RES = data[8]*256 + data[9]; //FE(78),FE(79)
		  	}
		  	data[0] = 0x8C;
		  	data[1] = 0x00;
		  	i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
		  	msleep(10);
#ifdef HX_EN_MUT_BUTTON
			data[0] = 0x8C;
		  	data[1] = 0x11;
		  	i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
		  	msleep(10);
		  	data[0] = 0x8B;
		  	data[1] = 0x00;
		  	data[2] = 0x64;
		  	i2c_himax_master_write(private_ts->client, &data[0],3,DEFAULT_RETRY_CNT);
		  	msleep(10);
		  	i2c_himax_read(private_ts->client, 0x5A, data, 4, DEFAULT_RETRY_CNT);
		  	HX_BT_NUM = (data[0] & 0x03);
		  	data[0] = 0x8C;
		  	data[1] = 0x00;
		  	i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
		  	msleep(10);
#endif
#ifdef HX_TP_SYS_2T2R
			data[0] = 0x8C;
			data[1] = 0x11;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			
			data[0] = 0x8B;
			data[1] = 0x00;
			data[2] = 0x96;
			i2c_himax_master_write(private_ts->client, &data[0],3,DEFAULT_RETRY_CNT);
			msleep(10);
			
			i2c_himax_read(private_ts->client, 0x5A, data, 10, DEFAULT_RETRY_CNT);
			
			HX_RX_NUM_2 = data[0];				 
			HX_TX_NUM_2 = data[1];				 
			
			I("%s:Touch Panel Type=%d \n", __func__,data[2]);
			if(data[2]==0x03)//2T2R type panel
				Is_2T2R = true;
			else
				Is_2T2R = false;
			
			data[0] = 0x8C;
			data[1] = 0x00;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
#endif
		  	data[0] = 0x8C;
		  	data[1] = 0x11;
		  	i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
		  	msleep(10);
		  	data[0] = 0x8B;
		  	data[1] = 0x00;
		  	data[2] = 0x02;
		  	i2c_himax_master_write(private_ts->client, &data[0],3,DEFAULT_RETRY_CNT);
		  	msleep(10);
		  	i2c_himax_read(private_ts->client, 0x5A, data, 10, DEFAULT_RETRY_CNT);
		  	if((data[1] & 0x01) == 1) {//FE(02)
				HX_INT_IS_EDGE = true;
		  	} else {
				HX_INT_IS_EDGE = false;
		  	}
		  	data[0] = 0x8C;
			data[1] = 0x00;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			I("%s:HX_RX_NUM =%d,HX_TX_NUM =%d,HX_MAX_PT=%d \n", __func__,HX_RX_NUM,HX_TX_NUM,HX_MAX_PT);
		}
		else if(IC_TYPE == HX_83100_SERIES_PWON)
		{
			cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0xF8;
			RegisterRead83100(cmd, 1, data);

			HX_RX_NUM				= data[1];
			HX_TX_NUM				= data[2];
			HX_MAX_PT				= data[3];

			cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0xFC;
			RegisterRead83100(cmd, 1, data);

		  if((data[1] & 0x04) == 0x04) {
				HX_XY_REVERSE = true;
		  } else {
			HX_XY_REVERSE			= false;
		  }
			HX_Y_RES = data[3]*256;
			cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x01; cmd[0] = 0x00;
			RegisterRead83100(cmd, 1, data);
			HX_Y_RES = HX_Y_RES + data[0];
			HX_X_RES = data[1]*256 + data[2];
			cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0x8C;
			RegisterRead83100(cmd, 1, data);
		  if((data[0] & 0x01) == 1) {
				HX_INT_IS_EDGE = true;
		  } else {
			HX_INT_IS_EDGE			= false;
		}
#ifdef HX_EN_MUT_BUTTON
			cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0xE8;
			RegisterRead83100(cmd, 1, data);
			HX_BT_NUM				= data[3];
#endif
			I("%s:HX_RX_NUM =%d,HX_TX_NUM =%d,HX_MAX_PT=%d \n", __func__,HX_RX_NUM,HX_TX_NUM,HX_MAX_PT);
			I("%s:HX_XY_REVERSE =%d,HX_Y_RES =%d,HX_X_RES=%d \n", __func__,HX_XY_REVERSE,HX_Y_RES,HX_X_RES);
			I("%s:HX_INT_IS_EDGE =%d \n", __func__,HX_INT_IS_EDGE);
		}
		else
		{
			HX_RX_NUM				= 0;
			HX_TX_NUM				= 0;
			HX_BT_NUM				= 0;
			HX_X_RES				= 0;
			HX_Y_RES				= 0;
			HX_MAX_PT				= 0;
			HX_XY_REVERSE		= false;
			HX_INT_IS_EDGE		= false;
		}

#ifdef TSP_BRINGUP_TEMP_CODE 	// temp code before LCD bringup. kernel panic
		if((HX_X_RES!=720)||(HX_Y_RES!=1280)||(HX_RX_NUM!=29)||(HX_TX_NUM!=16))
		{
			HX_RX_NUM				= 29;
			HX_TX_NUM				= 16;
			HX_MAX_PT				= 10;
			HX_X_RES				= 720;
			HX_Y_RES				= 1280;
			HX_BT_NUM				= 3;
			HX_XY_REVERSE			= true;
			HX_INT_IS_EDGE			= false;
			I("%s:correct HX_RX_NUM =%d,HX_TX_NUM =%d,HX_MAX_PT=%d \n", __func__,HX_RX_NUM,HX_TX_NUM,HX_MAX_PT);
			I("%s:correct HX_XY_REVERSE =%d,HX_Y_RES =%d,HX_X_RES=%d \n", __func__,HX_XY_REVERSE,HX_Y_RES,HX_X_RES);
			I("%s:correct HX_INT_IS_EDGE =%d \n", __func__,HX_INT_IS_EDGE);
		}
#endif

}
#if 0
static int himax_read_Sensor_ID(struct i2c_client *client)
{	
	uint8_t val_high[1], val_low[1], ID0=0, ID1=0;
	char data[3];
	const int normalRetry = 10;
	int sensor_id;
	
	data[0] = 0x56; data[1] = 0x02; data[2] = 0x02;/*ID pin PULL High*/
	i2c_himax_master_write(client, &data[0],3,normalRetry);
	msleep(1);

	//read id pin high
	i2c_himax_read(client, 0x57, val_high, 1, normalRetry);

	data[0] = 0x56; data[1] = 0x01; data[2] = 0x01;/*ID pin PULL Low*/
	i2c_himax_master_write(client, &data[0],3,normalRetry);
	msleep(1);

	//read id pin low
	i2c_himax_read(client, 0x57, val_low, 1, normalRetry);

	if((val_high[0] & 0x01) ==0)
		ID0=0x02;/*GND*/
	else if((val_low[0] & 0x01) ==0)
		ID0=0x01;/*Floating*/
	else
		ID0=0x04;/*VCC*/
	
	if((val_high[0] & 0x02) ==0)
		ID1=0x02;/*GND*/
	else if((val_low[0] & 0x02) ==0)
		ID1=0x01;/*Floating*/
	else
		ID1=0x04;/*VCC*/
	if((ID0==0x04)&&(ID1!=0x04))
		{
			data[0] = 0x56; data[1] = 0x02; data[2] = 0x01;/*ID pin PULL High,Low*/
			i2c_himax_master_write(client, &data[0],3,normalRetry);
			msleep(1);

		}
	else if((ID0!=0x04)&&(ID1==0x04))
		{
			data[0] = 0x56; data[1] = 0x01; data[2] = 0x02;/*ID pin PULL Low,High*/
			i2c_himax_master_write(client, &data[0],3,normalRetry);
			msleep(1);

		}
	else if((ID0==0x04)&&(ID1==0x04))
		{
			data[0] = 0x56; data[1] = 0x02; data[2] = 0x02;/*ID pin PULL High,High*/
			i2c_himax_master_write(client, &data[0],3,normalRetry);
			msleep(1);

		}
	sensor_id=(ID1<<4)|ID0;

	data[0] = 0xE4; data[1] = sensor_id;
	i2c_himax_master_write(client, &data[0],2,normalRetry);/*Write to MCU*/
	msleep(1);

	return sensor_id;

}
#endif
static void himax_power_on_initCMD(struct i2c_client *client)
{
	I("%s:\n", __func__);

	himax_touch_information();	

    himax_83100_SenseOn(0x01);//1=Flash, 0=SRAM
}

#ifdef HX_AUTO_UPDATE_FW
static int i_update_FW(void)
{
	unsigned char* ImageBuffer = i_CTPM_FW;
	int fullFileLength = sizeof(i_CTPM_FW);
	int i_FW_VER = 0, i_CFG_VER = 0;
	uint8_t ret = 0;

	i_FW_VER = i_CTPM_FW[FW_VER_MAJ_FLASH_ADDR]<<8 |i_CTPM_FW[FW_VER_MIN_FLASH_ADDR];
	i_CFG_VER = i_CTPM_FW[CFG_VER_MAJ_FLASH_ADDR]<<8 |i_CTPM_FW[CFG_VER_MIN_FLASH_ADDR];

	I("%s: FW_VER = ic: %d, bin: %d\n", __func__,private_ts->vendor_fw_ver, i_FW_VER );
	I("%s: FW_VER = ic: %d, bin: %d\n", __func__,private_ts->vendor_config_ver, i_CFG_VER);

	I("%s: i_fullFileLength = %d\n", __func__,fullFileLength);
	//if (( private_ts->vendor_fw_ver < i_FW_VER ) || ( private_ts->vendor_config_ver < i_CFG_VER ))
	if ( (private_ts->vendor_fw_ver != i_FW_VER) || ( private_ts->vendor_config_ver != i_CFG_VER )) 
		{		 
			if(fullFileLength == FW_SIZE_60k){
				ret = fts_ctpm_fw_upgrade_with_sys_fs_60k(ImageBuffer,fullFileLength,false);
			}else if (fullFileLength == FW_SIZE_64k){
				ret = fts_ctpm_fw_upgrade_with_sys_fs_64k(ImageBuffer,fullFileLength,false);
			}else if (fullFileLength == FW_SIZE_124k){
				ret = fts_ctpm_fw_upgrade_with_sys_fs_124k(ImageBuffer,fullFileLength,false);
			}else if (fullFileLength == FW_SIZE_128k){
				ret = fts_ctpm_fw_upgrade_with_sys_fs_128k(ImageBuffer,fullFileLength,false);
			}
	
			if(ret == 0)
				E("%s: TP upgrade error\n", __func__);
			else
				I("%s: TP upgrade OK\n", __func__);
			return 1;	
		}
	else
		return 0;
}
#endif



#ifdef HX_RST_PIN_FUNC
void himax_HW_reset(uint8_t loadconfig,uint8_t int_off)
{
	struct himax_ts_data *ts = private_ts;
	int ret = 0;

	return;
	if (ts->rst_gpio) {
		if(int_off)
			{
				if (ts->use_irq)
					himax_int_enable(private_ts->client->irq,0);
				else {
					hrtimer_cancel(&ts->timer);
					ret = cancel_work_sync(&ts->work);
				}
			}

		I("%s: Now reset the Touch chip.\n", __func__);

		//himax_rst_gpio_set(ts->rst_gpio, 0);
		//msleep(20);
		//himax_rst_gpio_set(ts->rst_gpio, 1);
		//msleep(20);

		if(loadconfig)
			himax_loadSensorConfig(private_ts->client,private_ts->pdata);

		if(int_off)
			{
				if (ts->use_irq)
					himax_int_enable(private_ts->client->irq,1);
				else
					hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
			}
	}
}
#endif

static u8 himax_read_FW_ver(bool hw_reset)
{
	uint8_t cmd[4];
	uint8_t data[64];

	I("%s: enter, %d \n", __func__, __LINE__);

	himax_int_enable(private_ts->client->irq,0);

	#ifdef HX_RST_PIN_FUNC
	if (hw_reset) {
		himax_HW_reset(false,false);
	}
	#endif

	//=====================================
	// Read FW version : 0x0800_0020
	//=====================================
	cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0x20;
	RegisterRead83100(cmd, 1, data);
	I("FW_VER 08000020 data[0]: %x, data[1]: %x,data[2]: %x,data[3]: %x, \n",data[0],data[1],data[2],data[3]);
	
	private_ts->vendor_fw_ver = data[3]<<8;
	
	cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0x24;
	RegisterRead83100(cmd, 1, data);
	I("FW_VER 08000024 data[0]: %x, data[1]: %x,data[2]: %x,data[3]: %x, \n",data[0],data[1],data[2],data[3]);
	
	private_ts->vendor_fw_ver = data[0] | private_ts->vendor_fw_ver;
	I("FW_VER : %X \n",private_ts->vendor_fw_ver);

	cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0x28;
	RegisterRead83100(cmd, 1, data);

	private_ts->vendor_config_ver = data[0]<<8 | data[1];
	I("CFG_VER : %X \n",private_ts->vendor_config_ver);
	
	#ifdef HX_RST_PIN_FUNC
	himax_HW_reset(true,false);
	#endif

	himax_int_enable(private_ts->client->irq,1);
	
	return 0;
}

static int himax_loadSensorConfig(struct i2c_client *client, struct himax_i2c_platform_data *pdata)
{
	//uint8_t cmd[4];
	//uint8_t data[5];

	I("%s: enter, %d \n", __func__, __LINE__);

	if (!client) {
		E("%s: Necessary parameters client are null!\n", __func__);
		return -1;
	}

	if(config_load == false)
		{
			config_selected = kzalloc(sizeof(*config_selected), GFP_KERNEL);
			if (config_selected == NULL) {
				E("%s: alloc config_selected fail!\n", __func__);
				return -1;
			}
		}
#ifndef CONFIG_OF
	pdata = client->dev.platform_data;
		if (!pdata) {
			E("%s: Necessary parameters pdata are null!\n", __func__);
			return -1;
		}
#endif
		himax_power_on_initCMD(client);

		// ===========TP test for himax_83100cell==========

		himax_read_FW_ver(false);
		//I("FW_VER : %x \n",private_ts->vendor_fw_ver);

		private_ts->vendor_sensor_id=0x2602;
		I("sensor_id=%x.\n",private_ts->vendor_sensor_id);
		//I("fw_ver=%x.\n",private_ts->vendor_fw_ver);

#ifdef HX_AUTO_UPDATE_FW
		if(i_update_FW() == false)
			I("NOT Have new FW=NOT UPDATE=\n");
		else
			I("Have new FW=UPDATE=\n");
#endif

	I("%s: initialization complete\n", __func__);

	return 1;
}

static bool himax_ic_package_check(struct himax_ts_data *ts)
{
#if 0
	uint8_t cmd[3];
	uint8_t data[3];

	memset(cmd, 0x00, sizeof(cmd));
	memset(data, 0x00, sizeof(data));

	if (i2c_himax_read(ts->client, 0xD1, cmd, 3, DEFAULT_RETRY_CNT) < 0)
		return false ;

	if (i2c_himax_read(ts->client, 0x31, data, 3, DEFAULT_RETRY_CNT) < 0)
		return false;

	if((data[0] == 0x85 && data[1] == 0x29))
		{
			IC_TYPE         = HX_85XX_F_SERIES_PWON;
        	IC_CHECKSUM 		= HX_TP_BIN_CHECKSUM_CRC;
        	//Himax: Set FW and CFG Flash Address                                          
        	FW_VER_MAJ_FLASH_ADDR   = 64901;  //0xFD85                              
        	FW_VER_MAJ_FLASH_LENG   = 1;
        	FW_VER_MIN_FLASH_ADDR   = 64902;  //0xFD86                                     
        	FW_VER_MIN_FLASH_LENG   = 1;
        	CFG_VER_MAJ_FLASH_ADDR 	= 64928;   //0xFDA0         
        	CFG_VER_MAJ_FLASH_LENG 	= 12;
        	CFG_VER_MIN_FLASH_ADDR 	= 64940;   //0xFDAC
        	CFG_VER_MIN_FLASH_LENG 	= 12;
			I("Himax IC package 852x F\n");
			}
	if((data[0] == 0x85 && data[1] == 0x30) || (cmd[0] == 0x05 && cmd[1] == 0x85 && cmd[2] == 0x29))
		{
			IC_TYPE 		= HX_85XX_E_SERIES_PWON;
			IC_CHECKSUM = HX_TP_BIN_CHECKSUM_CRC;
			//Himax: Set FW and CFG Flash Address                                          
			FW_VER_MAJ_FLASH_ADDR	= 133;	//0x0085                              
			FW_VER_MAJ_FLASH_LENG	= 1;                                               
			FW_VER_MIN_FLASH_ADDR	= 134;  //0x0086                                     
			FW_VER_MIN_FLASH_LENG	= 1;                                                
			CFG_VER_MAJ_FLASH_ADDR = 160;	//0x00A0         
			CFG_VER_MAJ_FLASH_LENG = 12; 	                                 
			CFG_VER_MIN_FLASH_ADDR = 172;	//0x00AC
			CFG_VER_MIN_FLASH_LENG = 12;   
			I("Himax IC package 852x E\n");
		}
	else if((data[0] == 0x85 && data[1] == 0x31))
		{
			IC_TYPE         = HX_85XX_ES_SERIES_PWON;
        	IC_CHECKSUM 		= HX_TP_BIN_CHECKSUM_CRC;
        	//Himax: Set FW and CFG Flash Address                                          
        	FW_VER_MAJ_FLASH_ADDR   = 133;  //0x0085                              
        	FW_VER_MAJ_FLASH_LENG   = 1;
        	FW_VER_MIN_FLASH_ADDR   = 134;  //0x0086                                     
        	FW_VER_MIN_FLASH_LENG   = 1;
        	CFG_VER_MAJ_FLASH_ADDR 	= 160;   //0x00A0         
        	CFG_VER_MAJ_FLASH_LENG 	= 12;
        	CFG_VER_MIN_FLASH_ADDR 	= 172;   //0x00AC
        	CFG_VER_MIN_FLASH_LENG 	= 12;
			I("Himax IC package 852x ES\n");
			}
	else if ((data[0] == 0x85 && data[1] == 0x28) || (cmd[0] == 0x04 && cmd[1] == 0x85 &&
		(cmd[2] == 0x26 || cmd[2] == 0x27 || cmd[2] == 0x28))) {
		IC_TYPE                = HX_85XX_D_SERIES_PWON;
		IC_CHECKSUM            = HX_TP_BIN_CHECKSUM_CRC;
		//Himax: Set FW and CFG Flash Address
		FW_VER_MAJ_FLASH_ADDR  = 133;                    // 0x0085
		FW_VER_MAJ_FLASH_LENG  = 1;
		FW_VER_MIN_FLASH_ADDR  = 134;                    // 0x0086
		FW_VER_MIN_FLASH_LENG  = 1;
		CFG_VER_MAJ_FLASH_ADDR = 160;                    // 0x00A0
		CFG_VER_MAJ_FLASH_LENG = 12;
		CFG_VER_MIN_FLASH_ADDR = 172;                    // 0x00AC
		CFG_VER_MIN_FLASH_LENG = 12;
		I("Himax IC package 852x D\n");
	} else if ((data[0] == 0x85 && data[1] == 0x23) || (cmd[0] == 0x03 && cmd[1] == 0x85 &&
			(cmd[2] == 0x26 || cmd[2] == 0x27 || cmd[2] == 0x28 || cmd[2] == 0x29))) {
		IC_TYPE                = HX_85XX_C_SERIES_PWON;
		IC_CHECKSUM            = HX_TP_BIN_CHECKSUM_SW;
		//Himax: Set FW and CFG Flash Address
		FW_VER_MAJ_FLASH_ADDR  = 133;                   // 0x0085
		FW_VER_MAJ_FLASH_LENG  = 1;
		FW_VER_MIN_FLASH_ADDR  = 134;                   // 0x0086
		FW_VER_MIN_FLASH_LENG  = 1;
		CFG_VER_MAJ_FLASH_ADDR = 135;                   // 0x0087
		CFG_VER_MAJ_FLASH_LENG = 12;
		CFG_VER_MIN_FLASH_ADDR = 147;                   // 0x0093
		CFG_VER_MIN_FLASH_LENG = 12;
		I("Himax IC package 852x C\n");
	} else if ((data[0] == 0x85 && data[1] == 0x26) ||
		   (cmd[0] == 0x02 && cmd[1] == 0x85 &&
		   (cmd[2] == 0x19 || cmd[2] == 0x25 || cmd[2] == 0x26))) {
		IC_TYPE                = HX_85XX_B_SERIES_PWON;
		IC_CHECKSUM            = HX_TP_BIN_CHECKSUM_SW;
		//Himax: Set FW and CFG Flash Address
		FW_VER_MAJ_FLASH_ADDR  = 133;                   // 0x0085
		FW_VER_MAJ_FLASH_LENG  = 1;
		FW_VER_MIN_FLASH_ADDR  = 728;                   // 0x02D8
		FW_VER_MIN_FLASH_LENG  = 1;
		CFG_VER_MAJ_FLASH_ADDR = 692;                   // 0x02B4
		CFG_VER_MAJ_FLASH_LENG = 3;
		CFG_VER_MIN_FLASH_ADDR = 704;                   // 0x02C0
		CFG_VER_MIN_FLASH_LENG = 3;
		I("Himax IC package 852x B\n");
	} else if ((data[0] == 0x85 && data[1] == 0x20) || (cmd[0] == 0x01 &&
			cmd[1] == 0x85 && cmd[2] == 0x19)) {
		IC_TYPE     = HX_85XX_A_SERIES_PWON;
		IC_CHECKSUM = HX_TP_BIN_CHECKSUM_SW;
		I("Himax IC package 852x A\n");
	} else {
		E("Himax IC package incorrect!!\n");
	}*/
#else
		//IC_TYPE         = HX_83100_SERIES_PWON;
        IC_CHECKSUM 		= HX_TP_BIN_CHECKSUM_CRC;
        //Himax: Set FW and CFG Flash Address                                          
        FW_VER_MAJ_FLASH_ADDR   = 57379;  //0xE023
        FW_VER_MAJ_FLASH_LENG   = 1;
        FW_VER_MIN_FLASH_ADDR   = 57380;  //0xE024
        FW_VER_MIN_FLASH_LENG   = 1;
        CFG_VER_MAJ_FLASH_ADDR 	= 57384;   //0x00
        CFG_VER_MAJ_FLASH_LENG 	= 1;
        CFG_VER_MIN_FLASH_ADDR 	= 57385;   //0x00
        CFG_VER_MIN_FLASH_LENG 	= 1;
		I("Himax IC package 83100_in\n");

#endif
	return true;
}

#ifdef HX_ESD_WORKAROUND
void ESD_HW_REST(void)
{
	I("START_Himax TP: ESD - Reset\n");

	HX_ESD_RESET_ACTIVATE = 1;

#ifdef HX_LCD_FEATURE
	HX_report_ESD_event();
#endif
	I("END_Himax TP: ESD - Reset\n");
}
#endif

#ifdef HX_HIGH_SENSE
static void himax_set_HSEN_func(unsigned int enable)
{
	uint8_t cmd[4];
	
	if (enable)
		cmd[0] = 0x40;
	else
		cmd[0] = 0x00;
	//if ( i2c_himax_write(private_ts->client, 0x8F ,&cmd[0], 1, DEFAULT_RETRY_CNT) < 0)
	//	E("%s i2c write fail.\n",__func__);

	return;
}

#endif

#ifdef HX_SMART_WAKEUP
static int himax_parse_wake_event(struct himax_ts_data *ts)
{
	uint8_t cmd[8];
	uint8_t buf[64];
	uint8_t flag_knock_code=0 ,point_cnt=0;
	unsigned char check_sum_cal = 0;
	int base = 0, x = 0, y = 0;
	int32_t loop_i;

	//=====================================
	// Read knock code status : 0x9008_8054 ==
	//=====================================
	buf[3] = 0x00; buf[2] = 0x00; buf[1] = 0x00; buf[0] = 0x00;
	cmd[3] = 0x90; cmd[2] = 0x08; cmd[1] = 0x80; cmd[0] = 0x54;
	RegisterRead83100(cmd, 1, buf);
	I("%s: knock code status  buf[0]=%x buf[1]=%x buf[2]=%x buf[3]=%x\n", __func__
		,buf[0],buf[1],buf[2],buf[3]);
	if( buf[0]!=0)
		flag_knock_code=1;//knock code mode

	//=====================================
	// Read gesture status : 0x9008_8058 ==
	//=====================================
	//buf[3] = 0x00; buf[2] = 0x00; buf[1] = 0x00; buf[0] = 0x00;
	//cmd[3] = 0x90; cmd[2] = 0x08; cmd[1] = 0x80; cmd[0] = 0x58;
	//RegisterRead83100(cmd, 1, buf);
	//I("%s: smart gesture status  buf[0]=%x buf[1]=%x buf[2]=%x buf[3]=%x\n", __func__
	//	,buf[0],buf[1],buf[2],buf[3]);
	//if( buf[0]!=0)
	//	flag_knock_code=0;//smart gesture mode

	//=====================
	//AHB I2C Burst Read
	//=====================
	cmd[0] = 0x31;		
	if ( i2c_himax_write(ts->client, 0x13 ,cmd, 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);		
	}

	cmd[0] = 0x10;
	if ( i2c_himax_write(ts->client, 0x0D ,cmd, 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);		
	}
	//=====================
	//Read event stack
	//=====================
	cmd[3] = 0x90; cmd[2] = 0x06; cmd[1] = 0x00; cmd[0] = 0x00;	
	if ( i2c_himax_write(ts->client, 0x00 ,cmd, 4, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);		
	}
	
	cmd[0] = 0x00;		
	if ( i2c_himax_write(ts->client, 0x0C ,cmd, 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);		
	}

	i2c_himax_read(ts->client, 0x08, buf, 56, HIMAX_I2C_RETRY_TIMES);
	//========checksum=========================================
	for (loop_i = 0, check_sum_cal = 0; loop_i < 56; loop_i++){
		check_sum_cal += buf[loop_i];
		I("P %x = 0x%2.2X ", loop_i, buf[loop_i]);
				if (loop_i % 8 == 7)
					I("\n");
		}
	if ((check_sum_cal != 0x00) )
	{
		I("[HIMAX TP MSG] checksum fail : check_sum_cal: 0x%02X\n", check_sum_cal);
		return 0;
	}
		
	point_cnt= buf[52] & 0x0F;
	if(flag_knock_code)
		{
			for (loop_i = 0; loop_i < point_cnt; loop_i++) {
				base = loop_i * 4;
				x = buf[base] << 8 | buf[base + 1];
				y = (buf[base + 2] << 8 | buf[base + 3]);

				I("knock_code P %x  (0x%2.2X, 0x%2.2X)\n", loop_i, x, y);
				//finger down
				input_mt_slot(ts->input_dev, 0);
				input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER,
					1);
				input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
					100);
				input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
					100);
				input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
					100);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
					x);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
					y);
				input_sync(ts->input_dev);
				//finger up
				input_mt_slot(ts->input_dev, 0);
				input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
				input_sync(ts->input_dev);
			}
		}
	else if((flag_knock_code==0)&&(buf[0]==0x24)&&(buf[1]==0x24)&&(buf[2]==0x24)&&(buf[3]==0x24))
		{
			I("%s: WAKE UP system!\n", __func__);
			return 1;//Yes, wake up system
		}
	else
		{
			I("%s: NOT WKAE packet, SKIP!\n", __func__);
			I("buf[0]=%x, buf[1]=%x, buf[2]=%x, buf[3]=%x\n",buf[0],buf[1],buf[2],buf[3]);
			//for (loop_i = 0; loop_i < 56; loop_i++) {
			//I("P %x = 0x%2.2X ", loop_i, buf[loop_i]);
			//if (loop_i % 8 == 7)
			//	I("\n");
			//}
		}
	return 0;
}
#endif
static void himax_ts_button_func(int tp_key_index,struct himax_ts_data *ts)
{
	uint16_t x_position = 0, y_position = 0;
	
	I("%s: enter, %d \n", __func__, __LINE__);

if ( tp_key_index != 0x00)
	{
		I("virtual key index =%x\n",tp_key_index);
		if ( tp_key_index == 0x01) {
			vk_press = 1;
			I("back key pressed\n");
				if (ts->pdata->virtual_key)
				{
					if (ts->button[0].index) {
						x_position = (ts->button[0].x_range_min + ts->button[0].x_range_max) / 2;
						y_position = (ts->button[0].y_range_min + ts->button[0].y_range_max) / 2;
					}
					if (ts->protocol_type == PROTOCOL_TYPE_A) {
						input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, 0);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
							100);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
							x_position);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
							y_position);
						input_mt_sync(ts->input_dev);
					} else if (ts->protocol_type == PROTOCOL_TYPE_B) {
						input_mt_slot(ts->input_dev, 0);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER,
						1);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
							100);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
							x_position);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
							y_position);
					}
				}
				else
					input_report_key(ts->input_dev, KEY_BACK, 1);
		}
		else if ( tp_key_index == 0x02) {
			vk_press = 1;
			I("home key pressed\n");
				if (ts->pdata->virtual_key)
				{
					if (ts->button[1].index) {
						x_position = (ts->button[1].x_range_min + ts->button[1].x_range_max) / 2;
						y_position = (ts->button[1].y_range_min + ts->button[1].y_range_max) / 2;
					}
						if (ts->protocol_type == PROTOCOL_TYPE_A) {
						input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, 0);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
							100);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
							x_position);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
							y_position);
						input_mt_sync(ts->input_dev);
					} else if (ts->protocol_type == PROTOCOL_TYPE_B) {
						input_mt_slot(ts->input_dev, 0);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER,
						1);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
							100);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
							x_position);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
							y_position);
					}
				}
				else
					input_report_key(ts->input_dev, KEY_HOME, 1);
		}
		else if ( tp_key_index == 0x04) {
			vk_press = 1;
			I("APP_switch key pressed\n");
				if (ts->pdata->virtual_key)
				{
					if (ts->button[2].index) {
						x_position = (ts->button[2].x_range_min + ts->button[2].x_range_max) / 2;
						y_position = (ts->button[2].y_range_min + ts->button[2].y_range_max) / 2;
					}
						if (ts->protocol_type == PROTOCOL_TYPE_A) {
						input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, 0);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
							100);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
							x_position);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
							y_position);
						input_mt_sync(ts->input_dev);
					} else if (ts->protocol_type == PROTOCOL_TYPE_B) {
						input_mt_slot(ts->input_dev, 0);
						input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER,
						1);
						input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
							100);
						input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
							100);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
							x_position);
						input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
							y_position);
					}
				}
				//else
					//input_report_key(ts->input_dev, KEY_APP_SWITCH, 1);	
		}
		input_sync(ts->input_dev);
	}
	else/*tp_key_index =0x00*/
	{
		I("virtual key released\n");
		vk_press = 0;
		if (ts->protocol_type == PROTOCOL_TYPE_A) {
			input_mt_sync(ts->input_dev);
		}
		else if (ts->protocol_type == PROTOCOL_TYPE_B) {
			input_mt_slot(ts->input_dev, 0);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
		}
		input_report_key(ts->input_dev, KEY_BACK, 0);
		input_report_key(ts->input_dev, KEY_HOME, 0);
		//input_report_key(ts->input_dev, KEY_APP_SWITCH, 0);
	input_sync(ts->input_dev);
	}
}

inline void himax_ts_work(struct himax_ts_data *ts)
{
	int ret = 0;
	uint8_t buf[128], finger_num, hw_reset_check[2];
	uint8_t cmd[8];
	uint16_t finger_pressed;
	uint8_t finger_on = 0;
	int32_t loop_i;
	unsigned char check_sum_cal = 0;
	int RawDataLen = 0;
	int raw_cnt_max ;
	int raw_cnt_rmd ;
	int hx_touch_info_size;
	int 	i;
#ifdef QCT
	uint8_t coordInfoSize = ts->coord_data_size + ts->area_data_size + 4;
#endif
#ifdef HX_TP_SYS_DIAG
	uint8_t *mutual_data;
	uint8_t *self_data;
	uint8_t diag_cmd;
	int 	mul_num;
	int 	self_num;
	int 	index = 0;
	int  	temp1, temp2;
	//coordinate dump start
	char coordinate_char[15+(HX_MAX_PT+5)*2*5+2];
	struct timeval t;
	struct tm broken;
	//coordinate dump end
#endif

	//I("%s: enter, %d \n", __func__, __LINE__);

	memset(buf, 0x00, sizeof(buf));
	memset(hw_reset_check, 0x00, sizeof(hw_reset_check));

	raw_cnt_max = HX_MAX_PT/4;
	raw_cnt_rmd = HX_MAX_PT%4;

	if (raw_cnt_rmd != 0x00) //more than 4 fingers
	{
		RawDataLen = 124 - ((HX_MAX_PT+raw_cnt_max+3)*4) - 1;
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+2)*4;
	}
	else //less than 4 fingers
	{
		RawDataLen = 124 - ((HX_MAX_PT+raw_cnt_max+2)*4) - 1;
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+1)*4;
	}
// ===========TP test for himax_83100cell==========
	//=====================
	//AHB I2C Burst Read
	//=====================
	cmd[0] = 0x31;		
	if ( i2c_himax_write(ts->client, 0x13 ,cmd, 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		goto err_workqueue_out;
	}

	cmd[0] = 0x10;		
	if ( i2c_himax_write(ts->client, 0x0D ,cmd, 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		goto err_workqueue_out;
	}
	//=====================
	//Read event stack
	//=====================
	cmd[3] = 0x90; cmd[2] = 0x06; cmd[1] = 0x00; cmd[0] = 0x00;	
	if ( i2c_himax_write(ts->client, 0x00 ,cmd, 4, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		goto err_workqueue_out;
	}
	
	cmd[0] = 0x00;		
	if ( i2c_himax_write(ts->client, 0x0C ,cmd, 1, 3) < 0) {
		E("%s: i2c access fail!\n", __func__);
		goto err_workqueue_out;
	}
#ifdef HX_TP_SYS_DIAG
	diag_cmd = getDiagCommand();
	if( diag_cmd ){
		ret = i2c_himax_read(ts->client, 0x08, buf, 124,HIMAX_I2C_RETRY_TIMES);
	}
	else{
		if(touch_monitor_stop_flag != 0){
			ret = i2c_himax_read(ts->client, 0x08, buf, 124,HIMAX_I2C_RETRY_TIMES);
			touch_monitor_stop_flag-- ;
		}
		else{
			ret = i2c_himax_read(ts->client, 0x08, buf, hx_touch_info_size,HIMAX_I2C_RETRY_TIMES);
		}
	}

	if (ret)
#else
	if (i2c_himax_read(ts->client, 0x08, buf, hx_touch_info_size,HIMAX_I2C_RETRY_TIMES))
#endif		
	{
		E("%s: can't read data from chip!\n", __func__);
		goto err_workqueue_out;
	}
#ifdef HX_ESD_WORKAROUND
	for(i = 0; i < hx_touch_info_size; i++)
	{
		if(buf[i] == 0xED)/*case 1 ESD recovery flow*/
		{
			check_sum_cal = 1;

		}
		else
		{
			check_sum_cal = 0;
			i = hx_touch_info_size;
			break;
		}
	}

	if (check_sum_cal != 0)
	{
		I("[HIMAX TP MSG]: ESD event checked - ALL 0xED.\n");
		ESD_HW_REST();
		return;
	}
	else if (HX_ESD_RESET_ACTIVATE)
	{
		HX_ESD_RESET_ACTIVATE = 0;/*drop 1st interrupts after chip reset*/
		I("[HIMAX TP MSG]:%s: Back from reset, ready to serve.\n", __func__);
		return;
	}
#endif
	// ===========TP test for himax_83100cell==========
	for (loop_i = 0, check_sum_cal = 0; loop_i < hx_touch_info_size; loop_i++)
		check_sum_cal += buf[loop_i];
	
	if ((check_sum_cal != 0x00) )
	{
		I("[HIMAX TP MSG] checksum fail : check_sum_cal: 0x%02X\n", check_sum_cal);
		return;
	}

	if (ts->debug_log_level & BIT(0)) {
		I("%s: raw data:\n", __func__);
		for (loop_i = 0; loop_i < hx_touch_info_size; loop_i++) {
			I("P %x = 0x%2.2X ", loop_i, buf[loop_i]);
			if (loop_i % 8 == 7)
				I("\n");
		}
	}

	//touch monitor raw data fetch
#ifdef HX_TP_SYS_DIAG
	diag_cmd = getDiagCommand();
	if (diag_cmd >= 1 && diag_cmd <= 6)
	{
		//Check 124th byte CRC
		for (i = hx_touch_info_size, check_sum_cal = 0; i < 124; i++)
		{
			check_sum_cal += buf[i];
		}
		if (check_sum_cal % 0x100 != 0)
		{
			goto bypass_checksum_failed_packet;
		}
#ifdef HX_TP_SYS_2T2R
		if(Is_2T2R && diag_cmd == 4)
		{
			mutual_data = getMutualBuffer_2();
			self_data 	= getSelfBuffer();

			// initiallize the block number of mutual and self
			mul_num = getXChannel_2() * getYChannel_2();

#ifdef HX_EN_SEL_BUTTON
			self_num = getXChannel_2() + getYChannel_2() + HX_BT_NUM;
#else
			self_num = getXChannel_2() + getYChannel_2();
#endif
		}
		else
#endif			
		{
			mutual_data = getMutualBuffer();
			self_data 	= getSelfBuffer();

			// initiallize the block number of mutual and self
			mul_num = getXChannel() * getYChannel();

#ifdef HX_EN_SEL_BUTTON
			self_num = getXChannel() + getYChannel() + HX_BT_NUM;
#else
			self_num = getXChannel() + getYChannel();
#endif
		}

		//Himax: Check Raw-Data Header
		if (buf[hx_touch_info_size] == buf[hx_touch_info_size+1] && buf[hx_touch_info_size+1] == buf[hx_touch_info_size+2]
		&& buf[hx_touch_info_size+2] == buf[hx_touch_info_size+3] && buf[hx_touch_info_size] > 0)
		{
			index = (buf[hx_touch_info_size] - 1) * RawDataLen;
			//I("Header[%d]: %x, %x, %x, %x, mutual: %d, self: %d\n", index, buf[56], buf[57], buf[58], buf[59], mul_num, self_num);
			for (i = 0; i < RawDataLen; i++)
			{
				temp1 = index + i;

				if (temp1 < mul_num)
				{ //mutual
					mutual_data[index + i] = buf[i + hx_touch_info_size+4];	//4: RawData Header
				}
				else
				{//self
					temp1 = i + index;
					temp2 = self_num + mul_num;
					
					if (temp1 >= temp2)
					{
						break;
					}

					self_data[i+index-mul_num] = buf[i + hx_touch_info_size+4];	//4: RawData Header					
				}
			}
		}
		else
		{
			I("[HIMAX TP MSG]%s: header format is wrong!\n", __func__);
		}
	}
	else if (diag_cmd == 7)
	{
		memcpy(&(diag_coor[0]), &buf[0], 128);
	}
	//coordinate dump start
	if (coordinate_dump_enable == 1)
	{
		for(i=0; i<(15 + (HX_MAX_PT+5)*2*5); i++)
		{
			coordinate_char[i] = 0x20;
		}
		coordinate_char[15 + (HX_MAX_PT+5)*2*5] = 0xD;
		coordinate_char[15 + (HX_MAX_PT+5)*2*5 + 1] = 0xA;
	}
	//coordinate dump end
bypass_checksum_failed_packet:
#endif
		EN_NoiseFilter = (buf[HX_TOUCH_INFO_POINT_CNT+2]>>3);
		//I("EN_NoiseFilter=%d\n",EN_NoiseFilter);
		EN_NoiseFilter = EN_NoiseFilter & 0x01;
		//I("EN_NoiseFilter2=%d\n",EN_NoiseFilter);

#if defined(HX_EN_SEL_BUTTON) || defined(HX_EN_MUT_BUTTON)
		tpd_key = (buf[HX_TOUCH_INFO_POINT_CNT+2]>>4);
		if (tpd_key == 0x0F)/*All (VK+AA)leave*/
		{
			tpd_key = 0x00;
		}
		//I("[DEBUG] tpd_key:  %x\r\n", tpd_key);
#else
		tpd_key = 0x00;
#endif

		p_point_num = hx_point_num;

		if (buf[HX_TOUCH_INFO_POINT_CNT] == 0xff)
			hx_point_num = 0;
		else
			hx_point_num= buf[HX_TOUCH_INFO_POINT_CNT] & 0x0f;

		// Touch Point information
		if (hx_point_num != 0 ) {
			if(vk_press == 0x00)
				{
					uint16_t old_finger = ts->pre_finger_mask;
					finger_num = buf[coordInfoSize - 4] & 0x0F;
					finger_pressed = buf[coordInfoSize - 2] << 8 | buf[coordInfoSize - 3];
					finger_on = 1;
					AA_press = 1;
					for (loop_i = 0; loop_i < ts->nFinger_support; loop_i++) {
						if (((finger_pressed >> loop_i) & 1) == 1) {
							int base = loop_i * 4;
							int x = buf[base] << 8 | buf[base + 1];
							int y = (buf[base + 2] << 8 | buf[base + 3]);
							int w = buf[(ts->nFinger_support * 4) + loop_i];
							finger_num--;

							if(ts->pdata->invert_x)
								x = ts->pdata->abs_x_max - x;
							if(ts->pdata->invert_y)
								y = ts->pdata->abs_y_max - y;


							//if ((ts->debug_log_level & BIT(3)) > 0)		// need display for debug.
							{
								if ((((old_finger >> loop_i) ^ (finger_pressed >> loop_i)) & 1) == 1)
								{
									if (ts->useScreenRes)
									{
										I("[P] ID:%02d status:%X, X:%d, Y:%d, W:%d, N:%d\n",
										loop_i+1, finger_pressed, x * ts->widthFactor >> SHIFTBITS,
										y * ts->heightFactor >> SHIFTBITS, w, EN_NoiseFilter);
									}
									else
									{
										I("[P] ID:%02d status:%X, X:%d, Y:%d, W:%d, N:%d\n",
										loop_i+1, finger_pressed, x, y, w, EN_NoiseFilter);
									}
								}
							}

							if (ts->protocol_type == PROTOCOL_TYPE_B)
							{
								input_mt_slot(ts->input_dev, loop_i);
							}

							input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
							input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, w);
							input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
							input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
							
							if (ts->protocol_type == PROTOCOL_TYPE_A)
							{
								input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, loop_i);
								input_mt_sync(ts->input_dev);
							}
							else
							{
								ts->last_slot = loop_i;
								input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
							}

							if (!ts->first_pressed)
							{
								ts->first_pressed = 1;
								I("S1@%d, %d\n", x, y);
							}

							ts->pre_finger_data[loop_i][0] = x;
							ts->pre_finger_data[loop_i][1] = y;


							if (ts->debug_log_level & BIT(1))
								I("[M] ID:%d, X:%d, Y:%d W:%d, Z:%d, F:%d, N:%d\n",
									loop_i + 1, x, y, w, w, loop_i + 1, EN_NoiseFilter);						
						} else {
							if (ts->protocol_type == PROTOCOL_TYPE_B)
							{
								input_mt_slot(ts->input_dev, loop_i);
								input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
							}

							if (loop_i == 0 && ts->first_pressed == 1)
							{
								ts->first_pressed = 2;
								I("E1@%d, %d\n",
								ts->pre_finger_data[0][0] , ts->pre_finger_data[0][1]);
							}
							//if ((ts->debug_log_level & BIT(3)) > 0)		// need display for debug.
							{
								if ((((old_finger >> loop_i) ^ (finger_pressed >> loop_i)) & 1) == 1)
								{
									if (ts->useScreenRes)
									{
										I("[R] ID:%02d status:%X, X:%d, Y:%d, N:%d\n",
										loop_i+1, finger_pressed, ts->pre_finger_data[loop_i][0] * ts->widthFactor >> SHIFTBITS,
										ts->pre_finger_data[loop_i][1] * ts->heightFactor >> SHIFTBITS, Last_EN_NoiseFilter);
									}
									else
									{
										I("[R] ID:%02d status:%X, X:%d, Y:%d, N:%d\n",
										loop_i+1, finger_pressed, ts->pre_finger_data[loop_i][0],
										ts->pre_finger_data[loop_i][1], Last_EN_NoiseFilter);
									}
								}
							}
						}
					}
					ts->pre_finger_mask = finger_pressed;
				}else if ((tpd_key_old != 0x00)&&(tpd_key == 0x00)) {
					//temp_x[0] = 0xFFFF;
					//temp_y[0] = 0xFFFF;
					//temp_x[1] = 0xFFFF;
					//temp_y[1] = 0xFFFF;
					himax_ts_button_func(tpd_key,ts);
					finger_on = 0;
				}
			input_report_key(ts->input_dev, BTN_TOUCH, finger_on);
			input_sync(ts->input_dev);
		} else if (hx_point_num == 0){
			if(AA_press)
				{
				// leave event
				finger_on = 0;
				AA_press = 0;
				if (ts->protocol_type == PROTOCOL_TYPE_A)
					input_mt_sync(ts->input_dev);

				for (loop_i = 0; loop_i < ts->nFinger_support; loop_i++) {
						if (((ts->pre_finger_mask >> loop_i) & 1) == 1) {
							if (ts->protocol_type == PROTOCOL_TYPE_B) {
								input_mt_slot(ts->input_dev, loop_i);
								input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
							}
						}
					}
				if (ts->pre_finger_mask > 0) {
					for (loop_i = 0; loop_i < ts->nFinger_support; loop_i++) {
						//if((ts->debug_log_level & BIT(3)) > 0)		// need display for debug.
						if (((ts->pre_finger_mask >> loop_i) & 1) == 1) {
							if (ts->useScreenRes) {
								I("[R] ID:%02d status:%X, X:%d, Y:%d, N:%d\n", loop_i+1, 0, ts->pre_finger_data[loop_i][0] * ts->widthFactor >> SHIFTBITS,
									ts->pre_finger_data[loop_i][1] * ts->heightFactor >> SHIFTBITS, Last_EN_NoiseFilter);
							} else {
								I("[R] ID:%02d status:%X, X:%d, Y:%d, N:%d\n", loop_i+1, 0, ts->pre_finger_data[loop_i][0],ts->pre_finger_data[loop_i][1], Last_EN_NoiseFilter);
							}
						}
					}
					ts->pre_finger_mask = 0;
				}

				if (ts->first_pressed == 1) {
					ts->first_pressed = 2;
					I("E1@%d, %d\n",ts->pre_finger_data[0][0] , ts->pre_finger_data[0][1]);
				}

				if (ts->debug_log_level & BIT(1))
					I("All Finger leave\n");

#ifdef HX_TP_SYS_DIAG
					//coordinate dump start
					if (coordinate_dump_enable == 1)
					{
						do_gettimeofday(&t);
						time_to_tm(t.tv_sec, 0, &broken);

						sprintf(&coordinate_char[0], "%2d:%2d:%2d:%lu,", broken.tm_hour, broken.tm_min, broken.tm_sec, t.tv_usec/1000);
						sprintf(&coordinate_char[15], "Touch up!");
						coordinate_fn->f_op->write(coordinate_fn,&coordinate_char[0],15 + (HX_MAX_PT+5)*2*sizeof(char)*5 + 2,&coordinate_fn->f_pos);
					}
					//coordinate dump end
#endif
			}
			else if (tpd_key != 0x00) {
				himax_ts_button_func(tpd_key,ts);
				finger_on = 1;
			}
			else if ((tpd_key_old != 0x00)&&(tpd_key == 0x00)) {
				himax_ts_button_func(tpd_key,ts);
				finger_on = 0;
			}
			input_report_key(ts->input_dev, BTN_TOUCH, finger_on);
			input_sync(ts->input_dev);
		}
		tpd_key_old = tpd_key;
		Last_EN_NoiseFilter = EN_NoiseFilter;

workqueue_out:
	return;

err_workqueue_out:
	I("%s: Now reset the Touch chip.\n", __func__);

#ifdef HX_RST_PIN_FUNC
	himax_HW_reset(true,false);
#endif

	goto workqueue_out;
}
static enum hrtimer_restart himax_ts_timer_func(struct hrtimer *timer)
{
	struct himax_ts_data *ts;

	ts = container_of(timer, struct himax_ts_data, timer);
	I("%s: added queue. %d\n", __func__, __LINE__);

	queue_work(ts->himax_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

#ifdef QCT
static irqreturn_t himax_ts_thread(int irq, void *ptr)
{
	uint8_t diag_cmd;
	struct himax_ts_data *ts = ptr;
	struct timespec timeStart, timeEnd, timeDelta;

	diag_cmd = getDiagCommand();

	//I("%s: enter, %d \n", __func__, __LINE__);

	if (ts->debug_log_level & BIT(2)) {
			getnstimeofday(&timeStart);
			msleep(5);
			//I(" Irq start time = %ld.%06ld s\n",
			//	timeStart.tv_sec, timeStart.tv_nsec/1000);			
	}
	
#ifdef HX_SMART_WAKEUP
	if (atomic_read(&ts->suspend_mode)&&(!FAKE_POWER_KEY_SEND)&&(ts->SMWP_enable)&&(!diag_cmd)) {
		wake_lock_timeout(&ts->ts_SMWP_wake_lock, TS_WAKE_LOCK_TIMEOUT);
		msleep(200);
		if(himax_parse_wake_event((struct himax_ts_data *)ptr))
			{
				I(" %s SMART WAKEUP KEY power event press\n",__func__);
				input_report_key(ts->input_dev, KEY_POWER, 1);
				input_sync(ts->input_dev);
				msleep(100);
				I(" %s SMART WAKEUP KEY power event release\n",__func__);
				input_report_key(ts->input_dev, KEY_POWER, 0);
				input_sync(ts->input_dev);
				FAKE_POWER_KEY_SEND=true;
				return IRQ_HANDLED;
			}
	}
	else
#endif
	himax_ts_work((struct himax_ts_data *)ptr);
	if(ts->debug_log_level & BIT(2)) {
			getnstimeofday(&timeEnd);
				timeDelta.tv_nsec = (timeEnd.tv_sec*1000000000+timeEnd.tv_nsec)
				-(timeStart.tv_sec*1000000000+timeStart.tv_nsec);
			//I("Irq finish time = %ld.%06ld s\n",
			//	timeEnd.tv_sec, timeEnd.tv_nsec/1000);
			//I("Touch latency = %ld us\n", timeDelta.tv_nsec/1000);
	}
	return IRQ_HANDLED;
}

static void himax_ts_work_func(struct work_struct *work)
{
	struct himax_ts_data *ts = container_of(work, struct himax_ts_data, work);
	
	I("%s: enter, %d \n", __func__, __LINE__);

	himax_ts_work(ts);
}

int himax_ts_register_interrupt(struct i2c_client *client)
{
	struct himax_ts_data *ts = i2c_get_clientdata(client);
	int ret = 0;

	ts->irq_enabled = 0;
	//Work functon
	if (client->irq) {/*INT mode*/
		ts->use_irq = 1;
		if(HX_INT_IS_EDGE)
			{
				I("%s edge triiger falling\n ",__func__);
				ret = request_threaded_irq(client->irq, NULL, himax_ts_thread,IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, ts);
			}
		else
			{
				I("%s level trigger low\n ",__func__);
				ret = request_threaded_irq(client->irq, NULL, himax_ts_thread,IRQF_TRIGGER_LOW | IRQF_ONESHOT, client->name, ts);
			}
		if (ret == 0) {
			ts->irq_enabled = 1;
			irq_enable_count = 1;
			I("%s: irq enabled at qpio: %d\n", __func__, client->irq);
#ifdef HX_SMART_WAKEUP
			irq_set_irq_wake(client->irq, 1);
#endif
		} else {
			ts->use_irq = 0;
			E("%s: request_irq failed\n", __func__);
		}
	} else {
		I("%s: client->irq is empty, use polling mode.\n", __func__);
	}

	if (!ts->use_irq) {/*if use polling mode need to disable HX_ESD_WORKAROUND function*/
		ts->himax_wq = create_singlethread_workqueue("himax_touch");

		INIT_WORK(&ts->work, himax_ts_work_func);

		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = himax_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
		I("%s: polling mode enabled\n", __func__);
	}
	return ret;
}
#endif

#if defined(HX_USB_DETECT)
static void himax_cable_tp_status_handler_func(int connect_status)
{
	struct himax_ts_data *ts;
	I("Touch: cable change to %d\n", connect_status);
	ts = private_ts;
	if (ts->cable_config) {
		if (!atomic_read(&ts->suspend_mode)) {
			if ((!!connect_status) != ts->usb_connected) {
				if (!!connect_status) {
					ts->cable_config[1] = 0x01;
					ts->usb_connected = 0x01;
				} else {
					ts->cable_config[1] = 0x00;
					ts->usb_connected = 0x00;
				}

				i2c_himax_master_write(ts->client, ts->cable_config,
					sizeof(ts->cable_config), HIMAX_I2C_RETRY_TIMES);

				I("%s: Cable status change: 0x%2.2X\n", __func__, ts->cable_config[1]);
			} else
				I("%s: Cable status is the same as previous one, ignore.\n", __func__);
		} else {
			if (connect_status)
				ts->usb_connected = 0x01;
			else
				ts->usb_connected = 0x00;
			I("%s: Cable status remembered: 0x%2.2X\n", __func__, ts->usb_connected);
		}
	}
}

static struct t_cable_status_notifier himax_cable_status_handler = {
	.name = "usb_tp_connected",
	.func = himax_cable_tp_status_handler_func,
};

#endif

#ifdef HX_SMART_WAKEUP
static void himax_SMWP_work(struct work_struct *work)
{
	uint8_t tmp_addr[4],tmp2_addr[4];
	uint8_t tmp_data[5],tmp2_data[5];
	struct himax_ts_data *ts = container_of(work, struct himax_ts_data,
							smwp_work.work);
	I(" %s in", __func__);

	if(ts->SMWP_enable)
	{
		SMWP_bit_retry:

		//=====================================
		// write nock code status : 0x9008_804C ==
		//=====================================
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x4C;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00;	tmp_data[0] = 0x00;

		//=====================================
		// write gesture status : 0x9008_8050 ==
		//=====================================
		tmp2_addr[3] = 0x90; tmp2_addr[2] = 0x08; tmp2_addr[1] = 0x80; tmp2_addr[0] = 0x50;
		tmp2_data[3] = 0x00; tmp2_data[2] = 0x00; tmp2_data[1] = 0x00; tmp2_data[0] = 0x00;

		switch (HX_SMWP_EN) {
			case 1:
				tmp_data[0] = 0x01;
				tmp2_data[0] = 0x00;
			break;
			case 2:
				tmp_data[0] = 0x00;
				tmp2_data[0] = 0x01;
			break;
			case 3:
				tmp_data[0] = 0x01;
				tmp2_data[0] = 0x01;
			break;
			default:
				tmp_data[0] = 0x00;
				tmp2_data[0] = 0x00;
			break;
		}
		RegisterWrite83100(tmp_addr, 1, tmp_data);
		RegisterWrite83100(tmp2_addr, 1, tmp2_data);

		msleep(20);
		//=====================================
		// read nock code status : 0x9008_804C ==
		//=====================================
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x4C;
		RegisterRead83100(tmp_addr, 1, tmp_data);
		I("%s: Read nock code status bit data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n", __func__
			,tmp_data[0],tmp_data[1],tmp_data[2],tmp_data[3]);
		//=====================================
		// read gesture status : 0x9008_8050 ==
		//=====================================
		tmp2_data[3] = 0x00; tmp2_data[2] = 0x00; tmp2_data[1] = 0x00; tmp2_data[0] = 0x00;
		tmp2_addr[3] = 0x90; tmp2_addr[2] = 0x08; tmp2_addr[1] = 0x80; tmp2_addr[0] = 0x50;
		RegisterRead83100(tmp2_addr, 1, tmp2_data);
		I("%s: Read gesture bit data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n", __func__
			,tmp2_data[0],tmp2_data[1],tmp2_data[2],tmp2_data[3]);

		if((tmp_data[0]<<1 | tmp2_data[0])!= HX_SMWP_EN)
			{
				I("%s: retry SMWP bit write data[0]=%x HX_SMWP_EN=%x \n", __func__
				,(tmp_data[0]<<1 | tmp2_data[0]),HX_SMWP_EN);
				goto SMWP_bit_retry;
			}
	}

}
#endif

//=============================================================================================================
//
//	Segment : Himax SYS Debug Function
//
//=============================================================================================================
#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)

static ssize_t touch_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct himax_ts_data *ts_data;
	ts_data = private_ts;

	ret += sprintf(buf, "%s_FW:%#x_CFG:%#x_SensorId:%#x\n", HIMAX_83100_NAME,
			ts_data->vendor_fw_ver, ts_data->vendor_config_ver, ts_data->vendor_sensor_id);

	return ret;
}

static DEVICE_ATTR(vendor, S_IRUGO, touch_vendor_show, NULL);

static ssize_t touch_attn_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct himax_ts_data *ts_data;
	ts_data = private_ts;

	sprintf(buf, "attn = %x\n", himax_int_gpio_read(ts_data->pdata->gpio_irq));
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(attn, S_IRUGO, touch_attn_show, NULL);

static ssize_t himax_int_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct himax_ts_data *ts = private_ts;
	size_t count = 0;

	count += sprintf(buf + count, "%d ", ts->irq_enabled);
	count += sprintf(buf + count, "\n");

	return count;
}

static ssize_t himax_int_status_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct himax_ts_data *ts = private_ts;
	int value, ret=0;

	if (sysfs_streq(buf, "0"))
		value = false;
	else if (sysfs_streq(buf, "1"))
		value = true;
	else
		return -EINVAL;

	if (value) {
				if(HX_INT_IS_EDGE)
				{
#ifdef QCT
					ret = request_threaded_irq(ts->client->irq, NULL, himax_ts_thread,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT, ts->client->name, ts);
#endif
				}
				else
				{
#ifdef QCT
					ret = request_threaded_irq(ts->client->irq, NULL, himax_ts_thread,
					IRQF_TRIGGER_LOW | IRQF_ONESHOT, ts->client->name, ts);
#endif
				}
		if (ret == 0) {
			ts->irq_enabled = 1;
			irq_enable_count = 1;
		}
	} else {
		himax_int_enable(ts->client->irq,0);
		free_irq(ts->client->irq, ts);
		ts->irq_enabled = 0;
	}

	return count;
}

static DEVICE_ATTR(enabled, S_IWUSR|S_IRUGO,
	himax_int_status_show, himax_int_status_store);

static ssize_t himax_layout_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct himax_ts_data *ts = private_ts;
	size_t count = 0;

	count += sprintf(buf + count, "%d ", ts->pdata->abs_x_min);
	count += sprintf(buf + count, "%d ", ts->pdata->abs_x_max);
	count += sprintf(buf + count, "%d ", ts->pdata->abs_y_min);
	count += sprintf(buf + count, "%d ", ts->pdata->abs_y_max);
	count += sprintf(buf + count, "\n");

	return count;
}

static ssize_t himax_layout_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct himax_ts_data *ts = private_ts;
	char buf_tmp[5];
	int i = 0, j = 0, k = 0, ret;
	unsigned long value;
	int layout[4] = {0};

	for (i = 0; i < 20; i++) {
		if (buf[i] == ',' || buf[i] == '\n') {
			memset(buf_tmp, 0x0, sizeof(buf_tmp));
			if (i - j <= 5)
				memcpy(buf_tmp, buf + j, i - j);
			else {
				I("buffer size is over 5 char\n");
				return count;
			}
			j = i + 1;
			if (k < 4) {
				ret = kstrtol(buf_tmp, 10, &value);
				layout[k++] = value;
			}
		}
	}
	if (k == 4) {
		ts->pdata->abs_x_min=layout[0];
		ts->pdata->abs_x_max=layout[1];
		ts->pdata->abs_y_min=layout[2];
		ts->pdata->abs_y_max=layout[3];
		I("%d, %d, %d, %d\n",
			ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_y_min, ts->pdata->abs_y_max);
		input_unregister_device(ts->input_dev);
		himax_input_register(ts);
	} else
		I("ERR@%d, %d, %d, %d\n",
			ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_y_min, ts->pdata->abs_y_max);
	return count;
}

static DEVICE_ATTR(layout, S_IWUSR|S_IRUGO,
	himax_layout_show, himax_layout_store);

static ssize_t himax_debug_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct himax_ts_data *ts_data;
	size_t count = 0;
	ts_data = private_ts;

	count += sprintf(buf, "%d\n", ts_data->debug_log_level);

	return count;
}

static ssize_t himax_debug_level_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct himax_ts_data *ts;
	char buf_tmp[11];
	int i;
	ts = private_ts;
	memset(buf_tmp, 0x0, sizeof(buf_tmp));
	memcpy(buf_tmp, buf, count);

	ts->debug_log_level = 0;
	for(i=0; i<count-1; i++)
	{
		if( buf_tmp[i]>='0' && buf_tmp[i]<='9' )
			ts->debug_log_level |= (buf_tmp[i]-'0');
		else if( buf_tmp[i]>='A' && buf_tmp[i]<='F' )
			ts->debug_log_level |= (buf_tmp[i]-'A'+10);
		else if( buf_tmp[i]>='a' && buf_tmp[i]<='f' )
			ts->debug_log_level |= (buf_tmp[i]-'a'+10);

		if(i!=count-2)
			ts->debug_log_level <<= 4;
	}

	if (ts->debug_log_level & BIT(3)) {
		if (ts->pdata->screenWidth > 0 && ts->pdata->screenHeight > 0 &&
		 (ts->pdata->abs_x_max - ts->pdata->abs_x_min) > 0 &&
		 (ts->pdata->abs_y_max - ts->pdata->abs_y_min) > 0) {
			ts->widthFactor = (ts->pdata->screenWidth << SHIFTBITS)/(ts->pdata->abs_x_max - ts->pdata->abs_x_min);
			ts->heightFactor = (ts->pdata->screenHeight << SHIFTBITS)/(ts->pdata->abs_y_max - ts->pdata->abs_y_min);
			if (ts->widthFactor > 0 && ts->heightFactor > 0)
				ts->useScreenRes = 1;
			else {
				ts->heightFactor = 0;
				ts->widthFactor = 0;
				ts->useScreenRes = 0;
			}
		} else
			I("Enable finger debug with raw position mode!\n");
	} else {
		ts->useScreenRes = 0;
		ts->widthFactor = 0;
		ts->heightFactor = 0;
	}

	return count;
}

static DEVICE_ATTR(debug_level, S_IWUSR|S_IRUGO,
	himax_debug_level_show, himax_debug_level_dump);

#ifdef HX_TP_SYS_REGISTER
static ssize_t himax_register_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	uint16_t loop_i;
	uint8_t data[128];	

	memset(data, 0x00, sizeof(data));

	I("himax_register_show: %x,%x,%x,%x\n", register_command[0],register_command[1],register_command[2],register_command[3]);

	RegisterRead83100(register_command, 1, data);
	
	ret += sprintf(buf, "command:  %x,%x,%x,%x\n", register_command[0],register_command[1],register_command[2],register_command[3]);

	for (loop_i = 0; loop_i < 128; loop_i++) {
		ret += sprintf(buf + ret, "0x%2.2X ", data[loop_i]);
		if ((loop_i % 16) == 15)
			ret += sprintf(buf + ret, "\n");
	}
	ret += sprintf(buf + ret, "\n");
	return ret;
}

static ssize_t himax_register_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char buf_tmp[16], length = 0;
	unsigned long result    = 0;
	uint8_t loop_i          = 0;
	uint16_t base           = 5;
	uint8_t write_da[128];	

	memset(buf_tmp, 0x0, sizeof(buf_tmp));
	memset(write_da, 0x0, sizeof(write_da));	

	I("himax %s \n",buf);

	if ((buf[0] == 'r' || buf[0] == 'w') && buf[1] == ':') {		

		if (buf[2] == 'x') {
			memcpy(buf_tmp, buf + 3, 8);
			if (!kstrtol(buf_tmp, 16, &result))
				{					
					register_command[0] = (uint8_t)result;
					register_command[1] = (uint8_t)(result >> 8);
					register_command[2] = (uint8_t)(result >> 16);
					register_command[3] = (uint8_t)(result >> 24);
				}
			base = 11;
			I("CMD: %x,%x,%x,%x\n", register_command[0],register_command[1],register_command[2],register_command[3]);
			
			for (loop_i = 0; loop_i < 128; loop_i++) {
				if (buf[base] == '\n') {
						if (buf[0] == 'w') {
								RegisterWrite83100(register_command, 1, write_da);
								I("CMD: %x, %x, %x, %x, len=%d\n", write_da[0], write_da[1],write_da[2],write_da[3],length);
						}
					I("\n");
					return count;
				}
				if (buf[base + 1] == 'x') {
					buf_tmp[10] = '\n';
					buf_tmp[11] = '\0';
					memcpy(buf_tmp, buf + base + 2, 8);
					if (!kstrtol(buf_tmp, 16, &result)) {						
						write_da[loop_i] = (uint8_t)result;
						write_da[loop_i+1] = (uint8_t)(result >> 8);
						write_da[loop_i+2] = (uint8_t)(result >> 16);
						write_da[loop_i+3] = (uint8_t)(result >> 24);
					}
					length+=4;
				}
				base += 10;
			}
		}
	}
	return count;
}

static DEVICE_ATTR(register, S_IWUSR|S_IRUGO, himax_register_show, himax_register_store);
#endif

#ifdef HX_TP_SYS_DIAG
static uint8_t *getMutualBuffer(void)
{
	return diag_mutual;
}
static uint8_t *getSelfBuffer(void)
{
	return &diag_self[0];
}
static uint8_t getXChannel(void)
{
	return x_channel;
}
static uint8_t getYChannel(void)
{
	return y_channel;
}
static uint8_t getDiagCommand(void)
{
	return diag_command;
}
static void setXChannel(uint8_t x)
{
	x_channel = x;
}
static void setYChannel(uint8_t y)
{
	y_channel = y;
}
static void setMutualBuffer(void)
{
	diag_mutual = kzalloc(x_channel * y_channel * sizeof(uint8_t), GFP_KERNEL);
}

#ifdef HX_TP_SYS_2T2R
static uint8_t *getMutualBuffer_2(void)
{
	return diag_mutual_2;
}
static uint8_t getXChannel_2(void)
{
	return x_channel_2;
}
static uint8_t getYChannel_2(void)
{
	return y_channel_2;
}
static void setXChannel_2(uint8_t x)
{
	x_channel_2 = x;
}
static void setYChannel_2(uint8_t y)
{
	y_channel_2 = y;
}
static void setMutualBuffer_2(void)
{
	diag_mutual_2 = kzalloc(x_channel_2 * y_channel_2 * sizeof(uint8_t), GFP_KERNEL);
}
#endif

static ssize_t himax_diag_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	uint32_t loop_i;
	uint16_t mutual_num, self_num, width;
#ifdef HX_TP_SYS_2T2R
	if(Is_2T2R && diag_command == 4)
	{
		mutual_num 	= x_channel_2 * y_channel_2;
		self_num 	= x_channel_2 + y_channel_2; //don't add KEY_COUNT
		width 		= x_channel_2;
		count += sprintf(buf + count, "ChannelStart: %4d, %4d\n\n", x_channel_2, y_channel_2);
	}
	else
#endif		
	{
		mutual_num 	= x_channel * y_channel;
		self_num 	= x_channel + y_channel; //don't add KEY_COUNT
		width 		= x_channel;
		count += sprintf(buf + count, "ChannelStart: %4d, %4d\n\n", x_channel, y_channel);
	}

	// start to show out the raw data in adb shell
	if (diag_command >= 1 && diag_command <= 6) {
		if (diag_command <= 3) {
			for (loop_i = 0; loop_i < mutual_num; loop_i++) {
				count += sprintf(buf + count, "%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1))
					count += sprintf(buf + count, " %3d\n", diag_self[width + loop_i/width]);
			}
			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < width; loop_i++) {
				count += sprintf(buf + count, "%4d", diag_self[loop_i]);
				if (((loop_i) % width) == (width - 1))
					count += sprintf(buf + count, "\n");
			}
#ifdef HX_EN_SEL_BUTTON
			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < HX_BT_NUM; loop_i++)
					count += sprintf(buf + count, "%4d", diag_self[HX_RX_NUM + HX_TX_NUM + loop_i]);
#endif
#ifdef HX_TP_SYS_2T2R
		}else if(Is_2T2R && diag_command == 4 ) {
			for (loop_i = 0; loop_i < mutual_num; loop_i++) {
				count += sprintf(buf + count, "%4d", diag_mutual_2[loop_i]);
				if ((loop_i % width) == (width - 1))
					count += sprintf(buf + count, " %3d\n", diag_self[width + loop_i/width]);
			}
			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < width; loop_i++) {
				count += sprintf(buf + count, "%4d", diag_self[loop_i]);
				if (((loop_i) % width) == (width - 1))
					count += sprintf(buf + count, "\n");
			}

#ifdef HX_EN_SEL_BUTTON
			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < HX_BT_NUM; loop_i++)
				count += sprintf(buf + count, "%4d", diag_self[HX_RX_NUM_2 + HX_TX_NUM_2 + loop_i]);
#endif
#endif
		} else if (diag_command > 4) {
			for (loop_i = 0; loop_i < self_num; loop_i++) {
				count += sprintf(buf + count, "%4d", diag_self[loop_i]);
				if (((loop_i - mutual_num) % width) == (width - 1))
					count += sprintf(buf + count, "\n");
			}
		} else {
			for (loop_i = 0; loop_i < mutual_num; loop_i++) {
				count += sprintf(buf + count, "%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1))
					count += sprintf(buf + count, "\n");
			}
		}
		count += sprintf(buf + count, "ChannelEnd");
		count += sprintf(buf + count, "\n");
	} else if (diag_command == 7) {
		for (loop_i = 0; loop_i < 128 ;loop_i++) {
			if ((loop_i % 16) == 0)
				count += sprintf(buf + count, "LineStart:");
				count += sprintf(buf + count, "%4d", diag_coor[loop_i]);
			if ((loop_i % 16) == 15)
				count += sprintf(buf + count, "\n");
		}
	}
	return count;
}

static ssize_t himax_diag_dump(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	const uint8_t command_ec_128_raw_flag 		= 0x06;
	const uint8_t command_ec_24_normal_flag 	= 0x00;
	uint8_t command_ec_128_raw_baseline_flag 	= 0x07;
	uint8_t command_ec_128_raw_bank_flag 			= 0x08;
	
	uint8_t command_F1h[2] = {0xF1, 0x00};				
	uint8_t receive[1];
	uint8_t tmp_addr[4];
    uint8_t tmp_data[4];
    
	memset(receive, 0x00, sizeof(receive));

	diag_command = buf[0] - '0';
	
	I("[Himax]diag_command=0x%x\n",diag_command);
	if (diag_command == 0x02)	{//DC
		command_F1h[1] = command_ec_128_raw_flag;
		//=====================================
	    // test result command : 0x8002_0180 ==> 0x02
	    //=====================================
	    tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0x80;
	    tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = command_F1h[1];
	    himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
		//i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	} else if (diag_command == 0x01) {//IIR
		command_F1h[1] = command_ec_128_raw_baseline_flag;
		//=====================================
	    // test result command : 0x8002_0180 ==> 0x01
	    //=====================================
	    tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0x80;
	    tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = command_F1h[1];
	    himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
		//i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	} else if (diag_command == 0x03) {	//BANK
		command_F1h[1] = command_ec_128_raw_bank_flag;	//0x03
		//=====================================
	    // test result command : 0x8002_0180 ==> 0x03
	    //=====================================
	    tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0x80;
	    tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = command_F1h[1];
	    himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
		//i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);		
	} else if (diag_command == 0x04 ) { // 2T3R IIR
		command_F1h[1] = 0x04; //2T3R IIR
		//=====================================
	    // test result command : 0x8002_0180 ==> 0x04
	    //=====================================
	    tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0x80;
	    tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = command_F1h[1];
	    himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
		//i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	} else if (diag_command == 0x00) {//Disable
		command_F1h[1] = command_ec_24_normal_flag;
		//=====================================
	    // test result command : 0x8002_0180 ==> 0x00
	    //=====================================
	    tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0x80;
	    tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = command_F1h[1];
	    himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
		//i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);	
		touch_monitor_stop_flag = touch_monitor_stop_limit;		
	}
		
	//coordinate dump start
	else if (diag_command == 0x08)	{
		coordinate_fn = filp_open(DIAG_COORDINATE_FILE,O_CREAT | O_WRONLY | O_APPEND | O_TRUNC,0666);
		if (IS_ERR(coordinate_fn))
		{
			E("%s: coordinate_dump_file_create error\n", __func__);
			coordinate_dump_enable = 0;
			filp_close(coordinate_fn,NULL);
		}
		coordinate_dump_enable = 1;
	}
	else if (diag_command == 0x09){
		coordinate_dump_enable = 0;
		
		if (!IS_ERR(coordinate_fn))
		{
			filp_close(coordinate_fn,NULL);
		}
	}
	//coordinate dump end
	else{
			//=====================================
		    // test result command : 0x8002_0180 ==> diag_command
		    //=====================================
		    tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0x80;
		    tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = diag_command;
		    himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);
			E("[Himax]Diag command error!diag_command=0x%x\n",diag_command);
	}
	return count;
}
static DEVICE_ATTR(diag, S_IWUSR|S_IRUGO, himax_diag_show, himax_diag_dump);
#endif

#ifdef HX_TP_SYS_RESET
static ssize_t himax_reset_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	//if (buf[0] == '1')
	//	ESD_HW_REST();
	//	HX_83100_chip_reset();
		
	return count;
}

static DEVICE_ATTR(reset, S_IWUSR|S_IRUGO, NULL, himax_reset_set);
#endif

#ifdef HX_TP_SYS_DEBUG
static ssize_t himax_debug_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	
	if (debug_level_cmd == 't')
	{
		if (fw_update_complete)
		{
			count += sprintf(buf, "FW Update Complete ");
		}
		else
		{
			count += sprintf(buf, "FW Update Fail ");
		}
	}
	else if (debug_level_cmd == 'h')
	{
		if (handshaking_result == 0)
		{
			count += sprintf(buf, "Handshaking Result = %d (MCU Running)\n",handshaking_result);
		}
		else if (handshaking_result == 1)
		{
			count += sprintf(buf, "Handshaking Result = %d (MCU Stop)\n",handshaking_result);
		}
		else if (handshaking_result == 2)
		{
			count += sprintf(buf, "Handshaking Result = %d (I2C Error)\n",handshaking_result);
		}
		else
		{
			count += sprintf(buf, "Handshaking Result = error \n");
		}
	}
	else if (debug_level_cmd == 'v')
	{
		count += sprintf(buf + count, "FW_VER = ");
           count += sprintf(buf + count, "0x%2.2X \n",private_ts->vendor_fw_ver);

		count += sprintf(buf + count, "CONFIG_VER = ");
           count += sprintf(buf + count, "0x%2.2X \n",private_ts->vendor_config_ver);
		count += sprintf(buf + count, "\n");
	}
	else if (debug_level_cmd == 'd')
	{
		count += sprintf(buf + count, "Himax Touch IC Information :\n");
		if (IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : D\n");
		}
		else if (IC_TYPE == HX_85XX_E_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : E\n");
		}
		else if (IC_TYPE == HX_85XX_ES_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : ES\n");
		}
		else if (IC_TYPE == HX_85XX_F_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : F\n");
		}
		else
		{
			count += sprintf(buf + count, "IC Type error.\n");
		}

		if (IC_CHECKSUM == HX_TP_BIN_CHECKSUM_SW)
		{
			count += sprintf(buf + count, "IC Checksum : SW\n");
		}
		else if (IC_CHECKSUM == HX_TP_BIN_CHECKSUM_HW)
		{
			count += sprintf(buf + count, "IC Checksum : HW\n");
		}
		else if (IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			count += sprintf(buf + count, "IC Checksum : CRC\n");
		}
		else
		{
			count += sprintf(buf + count, "IC Checksum error.\n");
		}

		if (HX_INT_IS_EDGE)
		{
			count += sprintf(buf + count, "Interrupt : EDGE TIRGGER\n");
		}
		else
		{
			count += sprintf(buf + count, "Interrupt : LEVEL TRIGGER\n");
		}

		count += sprintf(buf + count, "RX Num : %d\n",HX_RX_NUM);
		count += sprintf(buf + count, "TX Num : %d\n",HX_TX_NUM);
		count += sprintf(buf + count, "BT Num : %d\n",HX_BT_NUM);
		count += sprintf(buf + count, "X Resolution : %d\n",HX_X_RES);
		count += sprintf(buf + count, "Y Resolution : %d\n",HX_Y_RES);
		count += sprintf(buf + count, "Max Point : %d\n",HX_MAX_PT);
		count += sprintf(buf + count, "XY reverse : %d\n",HX_XY_REVERSE);
#ifdef HX_TP_SYS_2T2R
		if(Is_2T2R)
		{
		count += sprintf(buf + count, "2T2R panel\n");
		count += sprintf(buf + count, "RX Num_2 : %d\n",HX_RX_NUM_2);
		count += sprintf(buf + count, "TX Num_2 : %d\n",HX_TX_NUM_2);
		}
#endif	
	}
	else if (debug_level_cmd == 'i')
	{
		count += sprintf(buf + count, "Himax Touch Driver Version:\n");
		count += sprintf(buf + count, "%s \n", HIMAX_DRIVER_VER);
	}
	return count;
}

static ssize_t himax_debug_dump(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	struct file* filp = NULL;
	mm_segment_t oldfs;
	int result = 0;
	char fileName[128];

	I("%s: enter, %d \n", __func__, __LINE__);

	if ( buf[0] == 'h') //handshaking
	{
		debug_level_cmd = buf[0];

		himax_int_enable(private_ts->client->irq,0);

		handshaking_result = himax_hand_shaking(); //0:Running, 1:Stop, 2:I2C Fail

		himax_int_enable(private_ts->client->irq,1);

		return count;
	}

	else if ( buf[0] == 'v') //firmware version
	{
		debug_level_cmd = buf[0];
		himax_read_FW_ver(true);
		//himax_83100_CheckChipVersion();
		return count;
	}

	else if ( buf[0] == 'd') //ic information
	{
		debug_level_cmd = buf[0];
		return count;
	}

	else if ( buf[0] == 'i') //driver version
	{
		debug_level_cmd = buf[0];
		return count;
	}
	
	else if (buf[0] == 't')
	{

		himax_int_enable(private_ts->client->irq,0);

		debug_level_cmd 		= buf[0];
		fw_update_complete		= false;

		memset(fileName, 0, 128);
		// parse the file name
		snprintf(fileName, count-4, "%s", &buf[4]);
		I("%s: upgrade from file(%s) start!\n", __func__, fileName);
		// open file
		filp = filp_open(fileName, O_RDONLY, 0);
		if (IS_ERR(filp))
		{
			E("%s: open firmware file failed\n", __func__);
			goto firmware_upgrade_done;
			//return count;
		}
		oldfs = get_fs();
		set_fs(get_ds());

		// read the latest firmware binary file
		result=filp->f_op->read(filp,upgrade_fw,sizeof(upgrade_fw), &filp->f_pos);
		if (result < 0)
		{
			E("%s: read firmware file failed\n", __func__);
			goto firmware_upgrade_done;
			//return count;
		}

		set_fs(oldfs);
		filp_close(filp, NULL);

		I("%s: FW image,len %d: %02X, %02X, %02X, %02X\n", __func__, result, upgrade_fw[0], upgrade_fw[1], upgrade_fw[2], upgrade_fw[3]);

		if (result > 0)
		{
			// start to upgrade
			himax_int_enable(private_ts->client->irq,0);
			
			if ((buf[1] == '6') && (buf[2] == '0'))
				{
					if (fts_ctpm_fw_upgrade_with_sys_fs_60k(upgrade_fw, result, false) == 0)
					{
						E("%s: TP upgrade error, line: %d\n", __func__, __LINE__);
						fw_update_complete = false;
					}
					else
					{
						I("%s: TP upgrade OK, line: %d\n", __func__, __LINE__);
						fw_update_complete = true;
					}
				}
			else if ((buf[1] == '6') && (buf[2] == '4'))
				{
					if (fts_ctpm_fw_upgrade_with_sys_fs_64k(upgrade_fw, result, false) == 0)
					{
						E("%s: TP upgrade error, line: %d\n", __func__, __LINE__);
						fw_update_complete = false;
					}
					else
					{
						I("%s: TP upgrade OK, line: %d\n", __func__, __LINE__);
						fw_update_complete = true;
					}
				}
			else if ((buf[1] == '2') && (buf[2] == '4'))
				{
					if (fts_ctpm_fw_upgrade_with_sys_fs_124k(upgrade_fw, result, false) == 0)
					{
						E("%s: TP upgrade error, line: %d\n", __func__, __LINE__);
						fw_update_complete = false;
					}
					else
					{
						I("%s: TP upgrade OK, line: %d\n", __func__, __LINE__);
						fw_update_complete = true;
					}
				}
			else if ((buf[1] == '2') && (buf[2] == '8'))
				{
					if (fts_ctpm_fw_upgrade_with_sys_fs_128k(upgrade_fw, result, false) == 0)
					{
						E("%s: TP upgrade error, line: %d\n", __func__, __LINE__);
						fw_update_complete = false;
					}
					else
					{
						I("%s: TP upgrade OK, line: %d\n", __func__, __LINE__);
						fw_update_complete = true;
					}
				}
			else
				{
					E("%s: Flash command fail: %d\n", __func__, __LINE__);
					fw_update_complete = false;
				}
			goto firmware_upgrade_done;
			//return count;
		}
	}

	firmware_upgrade_done:

	#ifdef HX_RST_PIN_FUNC
	himax_HW_reset(true,false);
	#endif

	himax_int_enable(private_ts->client->irq,1);

	//todo himax_chip->tp_firmware_upgrade_proceed = 0;
	//todo himax_chip->suspend_state = 0;
	//todo enable_irq(himax_chip->irq);
	return count;
}

static DEVICE_ATTR(debug, S_IWUSR|S_IRUGO, himax_debug_show, himax_debug_dump);

#endif

#ifdef HX_TP_SYS_FLASH_DUMP

static uint8_t getFlashCommand(void)
{
	return flash_command;
}

static uint8_t getFlashDumpProgress(void)
{
	return flash_progress;
}

static uint8_t getFlashDumpComplete(void)
{
	return flash_dump_complete;
}

static uint8_t getFlashDumpFail(void)
{
	return flash_dump_fail;
}

static uint8_t getSysOperation(void)
{
	return sys_operation;
}

static uint8_t getFlashReadStep(void)
{
	return flash_read_step;
}

static uint8_t getFlashDumpSector(void)
{
	return flash_dump_sector;
}

static uint8_t getFlashDumpPage(void)
{
	return flash_dump_page;
}

static bool getFlashDumpGoing(void)
{
	return flash_dump_going;
}

static void setFlashBuffer(void)
{
	flash_buffer = kzalloc(Flash_Size * sizeof(uint8_t), GFP_KERNEL);
	memset(flash_buffer,0x00,Flash_Size);
}

static void setSysOperation(uint8_t operation)
{
	sys_operation = operation;
}

static void setFlashDumpProgress(uint8_t progress)
{
	flash_progress = progress;
	//I("setFlashDumpProgress : progress = %d ,flash_progress = %d \n",progress,flash_progress);
}

static void setFlashDumpComplete(uint8_t status)
{
	flash_dump_complete = status;
}

static void setFlashDumpFail(uint8_t fail)
{
	flash_dump_fail = fail;
}

static void setFlashCommand(uint8_t command)
{
	flash_command = command;
}

static void setFlashReadStep(uint8_t step)
{
	flash_read_step = step;
}

static void setFlashDumpSector(uint8_t sector)
{
	flash_dump_sector = sector;
}

static void setFlashDumpPage(uint8_t page)
{
	flash_dump_page = page;
}

static void setFlashDumpGoing(bool going)
{
	flash_dump_going = going;
}

static ssize_t himax_flash_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int loop_i;
	uint8_t local_flash_read_step=0;
	uint8_t local_flash_complete = 0;
	uint8_t local_flash_progress = 0;
	uint8_t local_flash_command = 0;
	uint8_t local_flash_fail = 0;

	local_flash_complete = getFlashDumpComplete();
	local_flash_progress = getFlashDumpProgress();
	local_flash_command = getFlashCommand();
	local_flash_fail = getFlashDumpFail();

	I("flash_progress = %d \n",local_flash_progress);

	if (local_flash_fail)
	{
		ret += sprintf(buf+ret, "FlashStart:Fail \n");
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	if (!local_flash_complete)
	{
		ret += sprintf(buf+ret, "FlashStart:Ongoing:0x%2.2x \n",flash_progress);
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	if (local_flash_command == 1 && local_flash_complete)
	{
		ret += sprintf(buf+ret, "FlashStart:Complete \n");
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	if (local_flash_command == 3 && local_flash_complete)
	{
		ret += sprintf(buf+ret, "FlashStart: \n");
		for(loop_i = 0; loop_i < 128; loop_i++)
		{
			ret += sprintf(buf + ret, "x%2.2x", flash_buffer[loop_i]);
			if ((loop_i % 16) == 15)
			{
				ret += sprintf(buf + ret, "\n");
			}
		}
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	//flash command == 0 , report the data
	local_flash_read_step = getFlashReadStep();

	ret += sprintf(buf+ret, "FlashStart:%2.2x \n",local_flash_read_step);

	for (loop_i = 0; loop_i < 1024; loop_i++)
	{
		ret += sprintf(buf + ret, "x%2.2X", flash_buffer[local_flash_read_step*1024 + loop_i]);

		if ((loop_i % 16) == 15)
		{
			ret += sprintf(buf + ret, "\n");
		}
	}

	ret += sprintf(buf + ret, "FlashEnd");
	ret += sprintf(buf + ret, "\n");
	return ret;
}

static ssize_t himax_flash_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char buf_tmp[6];
	unsigned long result = 0;
	uint8_t loop_i = 0;
	int base = 0;

	memset(buf_tmp, 0x0, sizeof(buf_tmp));

	I("%s: buf[0] = %s\n", __func__, buf);

	if (getSysOperation() == 1)
	{
		E("%s: SYS is busy , return!\n", __func__);
		return count;
	}

	if (buf[0] == '0')
	{
		setFlashCommand(0);
		if (buf[1] == ':' && buf[2] == 'x')
		{
			memcpy(buf_tmp, buf + 3, 2);
			I("%s: read_Step = %s\n", __func__, buf_tmp);
			if (!kstrtol(buf_tmp, 16, &result))
			{
				I("%s: read_Step = %lu \n", __func__, result);
				setFlashReadStep(result);
			}
		}
	}
	else if (buf[0] == '1')// 1_60,1_64,1_24,1_28 for flash size 60k,64k,124k,128k
	{
		setSysOperation(1);
		setFlashCommand(1);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);
		if ((buf[1] == '_' ) && (buf[2] == '6' )){
			if (buf[3] == '0'){
				Flash_Size = FW_SIZE_60k;
			}else if (buf[3] == '4'){
				Flash_Size = FW_SIZE_64k;
			}
		}else if ((buf[1] == '_' ) && (buf[2] == '2' )){
			if (buf[3] == '4'){
				Flash_Size = FW_SIZE_124k;
			}else if (buf[3] == '8'){
				Flash_Size = FW_SIZE_128k;
			}
		}
		queue_work(private_ts->flash_wq, &private_ts->flash_work);
	}
	else if (buf[0] == '2') // 2_60,2_64,2_24,2_28 for flash size 60k,64k,124k,128k
	{
		setSysOperation(1);
		setFlashCommand(2);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);
		if ((buf[1] == '_' ) && (buf[2] == '6' )){
			if (buf[3] == '0'){
				Flash_Size = FW_SIZE_60k;
			}else if (buf[3] == '4'){
				Flash_Size = FW_SIZE_64k;
			}
		}else if ((buf[1] == '_' ) && (buf[2] == '2' )){
			if (buf[3] == '4'){
				Flash_Size = FW_SIZE_124k;
			}else if (buf[3] == '8'){
				Flash_Size = FW_SIZE_128k;
			}
		}
		queue_work(private_ts->flash_wq, &private_ts->flash_work);
	}
	else if (buf[0] == '3')
	{
		setSysOperation(1);
		setFlashCommand(3);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		memcpy(buf_tmp, buf + 3, 2);
		if (!kstrtol(buf_tmp, 16, &result))
		{
			setFlashDumpSector(result);
		}

		memcpy(buf_tmp, buf + 7, 2);
		if (!kstrtol(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}

		queue_work(private_ts->flash_wq, &private_ts->flash_work);
	}
	else if (buf[0] == '4')
	{
		I("%s: command 4 enter.\n", __func__);
		setSysOperation(1);
		setFlashCommand(4);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		memcpy(buf_tmp, buf + 3, 2);
		if (!kstrtol(buf_tmp, 16, &result))
		{
			setFlashDumpSector(result);
		}
		else
		{
			E("%s: command 4 , sector error.\n", __func__);
			return count;
		}

		memcpy(buf_tmp, buf + 7, 2);
		if (!kstrtol(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}
		else
		{
			E("%s: command 4 , page error.\n", __func__);
			return count;
		}

		base = 11;

		I("=========Himax flash page buffer start=========\n");
		for(loop_i=0;loop_i<128;loop_i++)
		{
			memcpy(buf_tmp, buf + base, 2);
			if (!kstrtol(buf_tmp, 16, &result))
			{
				flash_buffer[loop_i] = result;
				I("%d ",flash_buffer[loop_i]);
				if (loop_i % 16 == 15)
				{
					I("\n");
				}
			}
			base += 3;
		}
		I("=========Himax flash page buffer end=========\n");

		queue_work(private_ts->flash_wq, &private_ts->flash_work);
	}
	return count;
}
static DEVICE_ATTR(flash_dump, S_IWUSR|S_IRUGO, himax_flash_show, himax_flash_store);

static void himax_ts_flash_work_func(struct work_struct *work)
{
	//struct himax_ts_data *ts = container_of(work, struct himax_ts_data, flash_work);

	//uint8_t page_tmp[128];
	uint8_t local_flash_command = 0;
	uint8_t sector = 0;
	uint8_t page = 0;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t out_buffer[20];
	uint8_t in_buffer[260];
	int page_prog_start = 0;
	int i = 0;

	I("%s: enter, %d \n", __func__, __LINE__);

	himax_int_enable(private_ts->client->irq,0);
	setFlashDumpGoing(true);

	sector = getFlashDumpSector();
	page = getFlashDumpPage();

	local_flash_command = getFlashCommand();

	msleep(100);

	I("%s: local_flash_command = %d enter.\n", __func__,local_flash_command);

	if ((local_flash_command == 1 || local_flash_command == 2)|| (local_flash_command==0x0F))
	{
		himax_83100_SenseOff();
		himax_83100_BURST_INC0_EN(0);
	/*=============Dump Flash Start=============*/
	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	for (page_prog_start = 0; page_prog_start < Flash_Size; page_prog_start = page_prog_start + 256)
	{
		//=================================
		// SPI Transfer Control
		// Set 256 bytes page read : 0x8000_0020 ==> 0x6940_02FF
		// Set read start address  : 0x8000_0028 ==> 0x0000_0000
		// Set command			   : 0x8000_0024 ==> 0x0000_003B
		//=================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x69; tmp_data[2] = 0x40; tmp_data[1] = 0x02; tmp_data[0] = 0xFF;
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
		if (page_prog_start < 0x100)
		{
			tmp_data[3] = 0x00;
			tmp_data[2] = 0x00;
			tmp_data[1] = 0x00;
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x100 && page_prog_start < 0x10000)
		{
			tmp_data[3] = 0x00;
			tmp_data[2] = 0x00;
			tmp_data[1] = (uint8_t)(page_prog_start >> 8);
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x10000 && page_prog_start < 0x1000000)
		{
			tmp_data[3] = 0x00;
			tmp_data[2] = (uint8_t)(page_prog_start >> 16);
			tmp_data[1] = (uint8_t)(page_prog_start >> 8);
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x3B;
		himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

		//==================================
		// AHB_I2C Burst Read
		// Set SPI data register : 0x8000_002C ==> 0x00
		//==================================
		out_buffer[0] = 0x2C;
		out_buffer[1] = 0x00;
		out_buffer[2] = 0x00;
		out_buffer[3] = 0x80;
		i2c_himax_write(private_ts->client, 0x00 ,out_buffer, 4, 3);

		//==================================
		// Read access : 0x0C ==> 0x00
		//==================================
		out_buffer[0] = 0x00;
		i2c_himax_write(private_ts->client, 0x0C ,out_buffer, 1, 3);

		//==================================
		// Read 128 bytes two times
		//==================================
		i2c_himax_read(private_ts->client, 0x08 ,in_buffer, 128, 3);
		for (i = 0; i < 128; i++)
			flash_buffer[i + page_prog_start] = in_buffer[i];

		i2c_himax_read(private_ts->client, 0x08 ,in_buffer, 128, 3);
		for (i = 0; i < 128; i++)
			flash_buffer[(i + 128) + page_prog_start] = in_buffer[i];

		I("%s:Verify Progress: %x\n", __func__, page_prog_start);
	}

/*=============Dump Flash End=============*/
		//msleep(100);
		/*
		for( i=0 ; i<8 ;i++)
		{
			for(j=0 ; j<64 ; j++)
			{
				setFlashDumpProgress(i*32 + j);
			}
		}
		*/
		himax_83100_SenseOn(0x01);
	}
	
	I("Complete~~~~~~~~~~~~~~~~~~~~~~~\n");
	/*if(local_flash_command==0x01)
		{
			I(" buffer_ptr = %d \n",buffer_ptr);
			
			for (i = 0; i < buffer_ptr; i++) 
			{
				I("%2.2X ", flash_buffer[i]);
			
				if ((i % 16) == 15)
				{
					I("\n");
				}
			}
			I("End~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		}*/

	if (local_flash_command == 2)
	{
		struct file *fn;

		fn = filp_open(FLASH_DUMP_FILE,O_CREAT | O_WRONLY ,0);
		if (!IS_ERR(fn))
		{
			fn->f_op->write(fn,flash_buffer,Flash_Size*sizeof(uint8_t),&fn->f_pos);
			filp_close(fn,NULL);
		}
	}

	himax_int_enable(private_ts->client->irq,1);
	setFlashDumpGoing(false);

	setFlashDumpComplete(1);
	setSysOperation(0);
	return;

/*	Flash_Dump_i2c_transfer_error:

	himax_int_enable(private_ts->client->irq,1);
	setFlashDumpGoing(false);
	setFlashDumpComplete(0);
	setFlashDumpFail(1);
	setSysOperation(0);
	return;
*/
}

#endif

#ifdef HX_TP_SYS_SELF_TEST
static ssize_t himax_chip_self_test_function(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val=0x00;
	int i=0,j=0;
	int ret = 0;

	I("%s: enter, %d \n", __func__, __LINE__);

	val = himax_chip_self_test();
	for (i = 0; i < X_NUM; i++)
	{
		for (j = 0; j < Y_NUM; j++)
		{
			ret += sprintf(buf + ret, "0x%2.2X ", raw_data_array[i][j]);
			//I("%03d, ", raw_data_array[i][j]);
		}
	  ret += sprintf(buf + ret, "\n");
	  //I("\r\n");
	}
	kfree(raw_data_array);
	if (val == 0x00) {
		ret += sprintf(buf + ret, "Self_Test Pass\n");
		return ret;
	} else {
		ret += sprintf(buf + ret, "Self_Test Fail\n");
		return ret;
	}
}

static ssize_t himax_chip_self_test_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char buf_tmp[2];
	unsigned long result = 0;

	memset(buf_tmp, 0x0, sizeof(buf_tmp));
	memcpy(buf_tmp, buf, 2);
	if(!kstrtol(buf_tmp, 16, &result))
		{
			sel_type = (uint8_t)result;
		}
	I("sel_type = %x \r\n", sel_type);
	return count;
}

static int himax_chip_self_test(void)
{
	int i=0,j=0;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	int pf_value=0x00;

	uint8_t IsComplete,frameheader,nframe,polling_count;
  uint8_t frame_index =0, index = 0,stack_data=0,IsComplete_cnt=0;
  uint8_t info_data[128] = {0x00,};
  uint8_t **header_data_array;
  uint8_t *header_data_array_1D;
  uint8_t header_array[8];
	struct himax_ts_data *ts_data = private_ts;
	uint8_t Thres_BaseC_Radio[2] = {0, 0};	//{Golden_Radio_Upper, Golden_Radio_Lower}
	uint8_t Thres_Dev[4] = {0, 0, 0, 0};		//{Dev_Rx_Upper, Dev_Rx_Lower, Dev_Tx_Upper, Dev_Tx_Lower}
	uint8_t Thres_BaseC[4] = {0, 0, 0, 0}; 	//{Abs_Upper_thx*10, Abs_Lower_thx*10, Avg_Radio_Upper_thx, Avg_Radio_Lower_thx}
	uint32_t *raw_data_array_1D;
	size_t check_cnt = 0;

	I("%s: enter, %d \n", __func__, __LINE__);

	memset(tmp_addr, 0x00, sizeof(tmp_addr));
	memset(tmp_data, 0x00, sizeof(tmp_data));

	himax_int_enable(private_ts->client->irq,0);
	himax_83100_BURST_INC0_EN(0);

// VCOM		//0x9008_806C ==> 0x0000_0001
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x6C;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x01;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	msleep(20);

	himax_83100_SenseOff();

// Sorting Enable	//0x9008_8084 ==> 0x0000_00BB (0xAA=>Selftest+GoldenBaseC, 0xBB=>Selftest only)
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x84;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xBB;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

// Switch Type (BaseC)	//0x8002_0180 ==> 0x0000_000D
//[Option:0x0A(Result),0x0B(Dev Rx Error),0x0C(Dev Tx Error),0x0D(BaseC),0x0E(Golden BaseC)]
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0x80;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = sel_type;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

// Set Threshold	//0x0800_0284
	tmp_addr[3] = 0x08; tmp_addr[2] = 0x00; tmp_addr[1] = 0x02; tmp_addr[0] = 0x84;
	tmp_data[3] = Thres_BaseC_Radio[1]; tmp_data[2] = Thres_BaseC_Radio[0]; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	tmp_addr[3] = 0x08; tmp_addr[2] = 0x00; tmp_addr[1] = 0x02; tmp_addr[0] = 0x88;
	tmp_data[3] = Thres_Dev[3]; tmp_data[2] = Thres_Dev[2]; tmp_data[1] = Thres_Dev[1]; tmp_data[0] = Thres_Dev[0];
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	tmp_addr[3] = 0x08; tmp_addr[2] = 0x00; tmp_addr[1] = 0x02; tmp_addr[0] = 0x8C;
	tmp_data[3] = Thres_BaseC[3]; tmp_data[2] = Thres_BaseC[2]; tmp_data[1] = Thres_BaseC[1]; tmp_data[0] = Thres_BaseC[0];
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

// CPU clock on	//0x9000_0010 ==> 0x0000_0000
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

//Calculate number of frames
	X_NUM = ts_data->x_channel;
	Y_NUM = ts_data->y_channel;
	if(sel_type == 0x0D || sel_type == 0x0E)	//16bits
	{
		nframe = 	(((X_NUM * Y_NUM + X_NUM + Y_NUM) / 120) +	(((X_NUM * Y_NUM + X_NUM + Y_NUM) % 120)? 1 : 0) )*2;		
	}else	//8bits
	{
	  nframe = 	(((X_NUM * Y_NUM + X_NUM + Y_NUM) / 120) +	(((X_NUM * Y_NUM + X_NUM + Y_NUM) % 120)? 1 : 0) );
	}
//Allocate buffer for Raw-Data
	header_data_array_1D = kzalloc(nframe* 120 * sizeof(uint8_t), GFP_KERNEL);
	header_data_array = kzalloc(nframe * sizeof(uint8_t), GFP_KERNEL);
	for(i=0 ; i < nframe ; i++, header_data_array_1D += 120){
		header_data_array[i] = header_data_array_1D;
	}

	raw_data_array_1D = kzalloc(X_NUM * Y_NUM * sizeof(uint32_t), GFP_KERNEL);
	raw_data_array = kzalloc(X_NUM * sizeof(uint32_t), GFP_KERNEL);
	for(i=0 ; i<X_NUM ; i++, raw_data_array_1D += Y_NUM){
		raw_data_array[i] = raw_data_array_1D;
	}

//Clear header_array
	for(i=0;i<nframe;i++)
	{
		header_array[i] = 0;
		for(j=0;j<120;j++)
		{
			header_data_array[i][j] = 0;
		}
	}

	IsComplete = 0;
	polling_count = 0;
	while(IsComplete == 0 && polling_count < 5000)
	{
		polling_count++;
//Read Data_Depth(0x9006_000C)
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x06; tmp_addr[1] = 0x00; tmp_addr[0] = 0x0C;
		RegisterRead83100(tmp_addr, 1, &info_data[0]);
		stack_data = info_data[0];
		I("The Data Depth is 0x%x \r\n", stack_data);
		I("*****info_data header is 0x%x, 0x%x, 0x%x, 0x%x \r\n", info_data[0], info_data[1], info_data[2], info_data[3]);
//#ifdef HI_MAX_DEEBUG
//		   		I("The Data Depth is 0x%x \r\n", stack_data);
//		   		I("*****info_data header is 0x%x, 0x%x, 0x%x, 0x%x \r\n", info_data[0], info_data[1], info_data[2], info_data[3]);
//#endif
//Check Data Ready?
		if(stack_data == 0x1F)
		{
//Clear info_data
			memset( info_data,0, sizeof(info_data));

//Receive 124 Bytes(0x9006_0000)
			tmp_addr[3] = 0x90; tmp_addr[2] = 0x06; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
			RegisterRead83100(tmp_addr, 31, &info_data[0]);
			I("info_data header is 0x%x, 0x%x, 0x%x, 0x%x \r\n", info_data[0], info_data[1], info_data[2], info_data[3]);
//#ifdef HI_MAX_DEEBUG
//		 I("info_data header is 0x%x, 0x%x, 0x%x, 0x%x \r\n", info_data[0], info_data[1], info_data[2], info_data[3]);
//#endif
//Check header is right?
			if(info_data[0]==0xA5 && info_data[1]==0x5A)
			{
//Collect Frame Header
					frameheader = info_data[3];
					header_array[frameheader]++;
//Save Raw-Data to header_data_array
		   		//memcpy(&header_data_array[frameheader][0], &info_data[4], 120);
					for (i = 0; i <120; i++)
					{
						header_data_array[frameheader][i] = info_data[4+i];
					}
//Check complete
					IsComplete_cnt = 0;
					for (i = 0; i < nframe ; i++)
					{
						IsComplete_cnt = (header_array[i] > FRAME_COUNT)+ IsComplete_cnt;
					}
					if (IsComplete_cnt == nframe)
					{
						IsComplete = 1;
					}
		  }
//Trans form 1D to 2D
					I("IsComplete=%d \r\n", IsComplete);
					if(IsComplete == 1)
					{
		   		  for (i = 0; i < X_NUM; i++)
						{
				      for (j = 0; j < Y_NUM; j++)
				      {
				        if (sel_type == 0x0D || sel_type == 0x0E)	//16bits
				        {
									raw_data_array[i][j] = (header_data_array[frame_index][index] << 8) + header_data_array[frame_index][index + 1];
				          index += 2;
				        }
				        else	//8bits
				        {
									raw_data_array[i][j] = header_data_array[frame_index][index++];
				        }

				        if (index >= 120)
				        {
									frame_index++;
				          index = 0; // first 4-byte are header and frame index
				        }
				      }
				    }
					}

		}else{
//Clear info_data
						memset( info_data,0, sizeof(info_data));
//Receive stack_data Bytes(0x9006_0000)
		       	tmp_addr[3] = 0x90; tmp_addr[2] = 0x06; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
		        RegisterRead83100(tmp_addr, stack_data, &info_data[0]);
		        I("info_data header is 0x%x, 0x%x, 0x%x, 0x%x \r\n", info_data[0], info_data[1], info_data[2], info_data[3]);
//#ifdef HI_MAX_DEEBUG
//	   			 I("info_data header is 0x%x, 0x%x, 0x%x, 0x%x \r\n", info_data[0], info_data[1], info_data[2], info_data[3]);
//#endif
	   			  msleep(100);	//Delay 10ms
		}

	}

// Inspection complete check	//0x9008_8084
	while ( check_cnt <100) {
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x84;
		RegisterRead83100(tmp_addr, 1, tmp_data);
		if (tmp_data[0]==0xAA){
		I("[Himax]: inspection complete\n");
		break;
		}else {
		msleep(100);
		check_cnt++;
		}
	}

	// Test result	//0x9008_808C
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x8C;
	RegisterRead83100(tmp_addr, 1, tmp_data);

		if (tmp_data[0]==0x01) {
		I("[Himax]: Selftest passed \n");
		pf_value = 0x00;
	} else {
		E("[Himax]: Selftest failed\n");
		pf_value = 0x01;
	}			

	// Switch to normal type	//0x8002_0180 ==> 0x0000_0000
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0x80;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_83100_Flash_Write_Burst(tmp_addr, tmp_data);

	//Check_Type_Selection_Print(sel_type);

	kfree(header_data_array);
	himax_int_enable(private_ts->client->irq,1);
	return pf_value;
}

static DEVICE_ATTR(tp_self_test, S_IWUSR|S_IRUGO, himax_chip_self_test_function, himax_chip_self_test_store);
#endif

#ifdef HX_TP_SYS_HITOUCH
static ssize_t himax_hitouch_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int ret = 0;

	if(hitouch_command == 0)
	{
		ret += sprintf(buf + ret, "Driver Version:2.0 \n");
	}

	return ret;
}

//-----------------------------------------------------------------------------------
//himax_hitouch_store
//command 0 : Get Driver Version
//command 1 : Hitouch Connect
//command 2 : Hitouch Disconnect
//-----------------------------------------------------------------------------------
static ssize_t himax_hitouch_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{	
	if(buf[0] == '0')
	{
		hitouch_command = 0;
	}
	else if(buf[0] == '1')
	{
		hitouch_is_connect = true;	
		I("hitouch_is_connect = true\n");	
	}
	else if(buf[0] == '2')
	{
		hitouch_is_connect = false;
		I("hitouch_is_connect = false\n"); 
	}
	return count;
}

static DEVICE_ATTR(hitouch, S_IWUSR|S_IRUGO, himax_hitouch_show, himax_hitouch_store);
#endif

#ifdef HX_HIGH_SENSE
static ssize_t himax_HSEN_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct himax_ts_data *ts = private_ts;
	size_t count = 0;
	count = snprintf(buf, PAGE_SIZE, "%d\n", ts->HSEN_enable);

	return count;
}

static ssize_t himax_HSEN_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{

	struct himax_ts_data *ts = private_ts;

	if (sysfs_streq(buf, "0"))
		ts->HSEN_enable = 0;
	else if (sysfs_streq(buf, "1"))
		ts->HSEN_enable = 1;
	else
		return -EINVAL;
	himax_set_HSEN_func(ts->HSEN_enable);

	I("%s: HSEN_enable = %d.\n", __func__, ts->HSEN_enable);

	return count;
}

static DEVICE_ATTR(HSEN, S_IWUSR|S_IRUGO,
	himax_HSEN_show, himax_HSEN_store);
#endif

#ifdef HX_SMART_WAKEUP
static ssize_t himax_SMWP_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	count = snprintf(buf, PAGE_SIZE, "%d\n", HX_SMWP_EN);

	return count;
}

static ssize_t himax_SMWP_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{

	struct himax_ts_data *ts = private_ts;
	uint8_t tmp_addr[4],tmp2_addr[4];
	uint8_t tmp_data[4],tmp2_data[4];

	// Write driver side internal password to access registers
	// write value from AHB I2C : 0x9008_804C = 0x00000000 //knock code
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x4C;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;

	// Write driver side internal password to access registers
	// write value from AHB I2C : 0x9008_8050 = 0x00000000 //double tap
	tmp2_addr[3] = 0x90; tmp2_addr[2] = 0x08; tmp2_addr[1] = 0x80; tmp2_addr[0] = 0x50;
	tmp2_data[3] = 0x00; tmp2_data[2] = 0x00; tmp2_data[1] = 0x00; tmp2_data[0] = 0x00;
	if (sysfs_streq(buf, "0"))//knock code, double tap disable
		{	
			tmp_data[0] = 0x00;
			tmp2_data[0] = 0x00;
			HX_SMWP_EN = 0x00;
			ts->SMWP_enable = 1;
		}
	else if (sysfs_streq(buf, "1"))
		{
			tmp_data[0] = 0x01;
			tmp2_data[0] = 0x00;
			HX_SMWP_EN = 0x01;
		ts->SMWP_enable = 1;
		}
	else if (sysfs_streq(buf, "2"))
		{
			tmp_data[0] = 0x00;
			tmp2_data[0] = 0x01;
			HX_SMWP_EN = 0x02;
			ts->SMWP_enable = 1;
		}
	else if (sysfs_streq(buf, "3"))
		{
			tmp_data[0] = 0x01;
			tmp2_data[0] = 0x01;
			HX_SMWP_EN = 0x03;
			ts->SMWP_enable = 1;
		}
	else
		{
			tmp_data[0] = 0x00;
			tmp2_data[0] = 0x00;
			HX_SMWP_EN = 0x00;
			ts->SMWP_enable = 0;
		return -EINVAL;
		}

	RegisterWrite83100(tmp_addr, 1, tmp_data);
	
	I("%s: SMART_WAKEUP_enable = %d.\n", __func__, ts->SMWP_enable);

	return count;
}

static DEVICE_ATTR(SMWP, S_IWUSR|S_IRUGO,
	himax_SMWP_show, himax_SMWP_store);

#endif

#if !defined(SEC_FACTORY_TEST)
static int himax_touch_sysfs_init(void)
{
	int ret;
	android_touch_kobj = kobject_create_and_add("android_touch", NULL);
	if (android_touch_kobj == NULL) {
		E("%s: subsystem_register failed\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	I("%s: enter, %d \n", __func__, __LINE__);

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_debug_level.attr);
	if (ret) {
		E("%s: create_file dev_attr_debug_level failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_vendor.attr);
	if (ret) {
		E("%s: sysfs_create_file dev_attr_vendor failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_reset.attr);
	if (ret) {
		E("%s: sysfs_create_file dev_attr_reset failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_attn.attr);
	if (ret) {
		E("%s: sysfs_create_file dev_attr_attn failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_enabled.attr);
	if (ret) {
		E("%s: sysfs_create_file dev_attr_enabled failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_layout.attr);
	if (ret) {
		E("%s: sysfs_create_file dev_attr_layout failed\n", __func__);
		return ret;
	}
	
	#ifdef HX_TP_SYS_REGISTER
	memset(register_command, 0x0, sizeof(register_command));
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_register.attr);
	if (ret)
	{
		E("create_file dev_attr_register failed\n");
		return ret;
	}
	#endif

	#ifdef HX_TP_SYS_DIAG
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_diag.attr);
	if (ret)
	{
		E("sysfs_create_file dev_attr_diag failed\n");
		return ret;
	}
	#endif

	#ifdef HX_TP_SYS_DEBUG
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_debug.attr);
	if (ret)
	{
		E("create_file dev_attr_debug failed\n");
		return ret;
	}
	#endif

	#ifdef HX_TP_SYS_FLASH_DUMP
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_flash_dump.attr);
	if (ret)
	{
		E("sysfs_create_file dev_attr_flash_dump failed\n");
		return ret;
	}
	#endif

	#ifdef HX_TP_SYS_SELF_TEST
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_tp_self_test.attr);
	if (ret)
	{
		E("sysfs_create_file dev_attr_tp_self_test failed\n");
		return ret;
	}
	#endif

	#ifdef HX_TP_SYS_HITOUCH
		ret = sysfs_create_file(android_touch_kobj, &dev_attr_hitouch.attr);
		if (ret) 
		{
			E("sysfs_create_file dev_attr_hitouch failed\n");
			return ret;
		}
	#endif
	
	#ifdef HX_HIGH_SENSE
		ret = sysfs_create_file(android_touch_kobj, &dev_attr_HSEN.attr);
		if (ret) 
		{
			E("sysfs_create_file dev_attr_HSEN failed\n");
			return ret;
		}
	#endif
	
	#ifdef HX_SMART_WAKEUP
		ret = sysfs_create_file(android_touch_kobj, &dev_attr_SMWP.attr);
		if (ret) 
		{
			E("sysfs_create_file dev_attr_SMWP failed\n");
			return ret;
		}
	#endif
	
	return 0 ;
}

static void himax_touch_sysfs_deinit(void)
{
	I("%s: enter, %d \n", __func__, __LINE__);

	sysfs_remove_file(android_touch_kobj, &dev_attr_debug_level.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_vendor.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_reset.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_attn.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_enabled.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_layout.attr);
	
	#ifdef HX_TP_SYS_REGISTER
	sysfs_remove_file(android_touch_kobj, &dev_attr_register.attr);
	#endif

	#ifdef HX_TP_SYS_DIAG
	sysfs_remove_file(android_touch_kobj, &dev_attr_diag.attr);
	#endif

	#ifdef HX_TP_SYS_DEBUG
	sysfs_remove_file(android_touch_kobj, &dev_attr_debug.attr);
	#endif
	
	#ifdef HX_TP_SYS_FLASH_DUMP
	sysfs_remove_file(android_touch_kobj, &dev_attr_flash_dump.attr);
	#endif

	#ifdef HX_TP_SYS_SELF_TEST
	sysfs_remove_file(android_touch_kobj, &dev_attr_tp_self_test.attr);
	#endif

	#ifdef HX_TP_SYS_HITOUCH
	sysfs_remove_file(android_touch_kobj, &dev_attr_hitouch.attr);
	#endif

	#ifdef HX_HIGH_SENSE
	sysfs_remove_file(android_touch_kobj, &dev_attr_HSEN.attr);
	#endif

	#ifdef HX_SMART_WAKEUP
	sysfs_remove_file(android_touch_kobj, &dev_attr_SMWP.attr);
	#endif
	
	kobject_del(android_touch_kobj);
}
#endif
#endif

#ifdef SEC_FACTORY_TEST
#include "himax_cmd.c"

#endif

#ifdef CONFIG_OF
static void himax_vk_parser(struct device_node *dt,
				struct himax_i2c_platform_data *pdata)
{
	u32 data = 0;
	uint8_t cnt = 0, i = 0;
	uint32_t coords[4] = {0};
	struct device_node *node, *pp = NULL;
	struct himax_virtual_key *vk;

	node = of_parse_phandle(dt, "virtualkey", 0);
	if (node == NULL) {
		I(" DT-No vk info in DT");
		return;
	} else {
		while ((pp = of_get_next_child(node, pp)))
			cnt++;
		if (!cnt)
			return;

		vk = kzalloc(cnt * (sizeof *vk), GFP_KERNEL);
		pp = NULL;
		while ((pp = of_get_next_child(node, pp))) {
			if (of_property_read_u32(pp, "idx", &data) == 0)
				vk[i].index = data;
			if (of_property_read_u32_array(pp, "range", coords, 4) == 0) {
				vk[i].x_range_min = coords[0], vk[i].x_range_max = coords[1];
				vk[i].y_range_min = coords[2], vk[i].y_range_max = coords[3];
			} else
				I(" range faile");
			i++;
		}
		pdata->virtual_key = vk;
		for (i = 0; i < cnt; i++)
			I(" vk[%d] idx:%d x_min:%d, y_max:%d", i,pdata->virtual_key[i].index,
				pdata->virtual_key[i].x_range_min, pdata->virtual_key[i].y_range_max);
	}
}


static int himax_pinctrl_configure(struct himax_ts_data *ts, bool active)
{
	struct pinctrl_state *set_state;
	int retval;

	if (active) {
		set_state =	pinctrl_lookup_state(ts->pinctrl,	"active_state");
		if (IS_ERR(set_state)) {
			pr_err("%s: cannot get ts pinctrl active state\n", __func__);
			return PTR_ERR(set_state);
		}
	} else {
		set_state =	pinctrl_lookup_state(ts->pinctrl,	"suspend_state");
		if (IS_ERR(set_state)) {
			pr_err("%s: cannot get gpiokey pinctrl sleep state\n", __func__);
			return PTR_ERR(set_state);
		}
	}
	retval = pinctrl_select_state(ts->pinctrl, set_state);
	if (retval) {
		pr_err("%s: cannot set ts pinctrl active state\n", __func__);
		return retval;
	}
	I(" %s, on:%d", __func__, active);

	return 0;
}


static int himax_parse_dt(struct himax_ts_data *ts,
				struct himax_i2c_platform_data *pdata)
{
	int rc, coords_size = 0;
	uint32_t coords[4] = {0};
	struct property *prop;
	struct device_node *dt = ts->client->dev.of_node;
	u32 data = 0;

	prop = of_find_property(dt, "himax,panel-coords", NULL);
	if (prop) {
		coords_size = prop->length / sizeof(u32);
		if (coords_size != 4)
			D(" %s:Invalid panel coords size %d", __func__, coords_size);
	}

	if (of_property_read_u32_array(dt, "himax,panel-coords", coords, coords_size) == 0) {
		pdata->abs_x_min = coords[0], pdata->abs_x_max = coords[1];
		pdata->abs_y_min = coords[2], pdata->abs_y_max = coords[3];
		I(" DT-%s:panel-coords = %d, %d, %d, %d\n", __func__, pdata->abs_x_min,
				pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);
	}

	prop = of_find_property(dt, "himax,display-coords", NULL);
	if (prop) {
		coords_size = prop->length / sizeof(u32);
		if (coords_size != 4)
			D(" %s:Invalid display coords size %d", __func__, coords_size);
	}
	rc = of_property_read_u32_array(dt, "himax,display-coords", coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		D(" %s:Fail to read display-coords %d\n", __func__, rc);
		return rc;
	}
	pdata->screenWidth  = coords[1];
	pdata->screenHeight = coords[3];
	I(" DT-%s:display-coords = (%d, %d)", __func__, pdata->screenWidth,
		pdata->screenHeight);

	pdata->gpio_irq = of_get_named_gpio(dt, "himax,irq-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_irq)) {
		E(" DT:gpio_irq value is not valid\n");
	}else{
		if (pdata->gpio_irq>= 0) {
			rc = gpio_request(pdata->gpio_irq, "himax,irq-gpio");
			if (rc < 0)
				E("%s: request irq pin failed\n", __func__);
		}
	}

	pdata->gpio_reset = of_get_named_gpio(dt, "himax,rst-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_reset)) {
		E(" DT:gpio_rst value is not valid\n");
	}else{
		if (pdata->gpio_reset>= 0) {
			rc = gpio_request(pdata->gpio_reset, "himax,rst-gpio");
			if (rc < 0)
				E("%s: request reset pin failed\n", __func__);
		}
	}

#if 0 // ndef SEC_PWRCTL_WITHLCD
	pdata->gpio_3v3_en = of_get_named_gpio(dt, "himax,3v3-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_3v3_en)) {
		E(" DT:gpio_3v3_en value is not valid\n");
	}
	I(" DT:gpio_irq=%d, gpio_rst=%d, gpio_3v3_en=%d", pdata->gpio_irq, pdata->gpio_reset, pdata->gpio_3v3_en);
#endif

#ifdef SEC_PWRCTL_WITHLCD
	pdata->gpio_iovcc_en = of_get_named_gpio(dt, "himax,iovcc-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_iovcc_en)) {
		E(" DT:gpio_iovcc_en value is not valid\n");
	}
#if 0
	else{
		if (pdata->gpio_iovcc_en>= 0) {
			rc = gpio_request(pdata->gpio_iovcc_en, "himax,iovcc_en");
			if (rc < 0)
				E("%s: request iovcc pin failed\n", __func__);
		}		
	}	
	rc = of_property_read_string(dt, "vdd_ldo_name", &pdata->tsp_vdd_name);
	if (rc < 0){
		D("%s: Unable to read TSP ldo name\n", __func__);
	}
#endif
	pdata->gpio_3v3_en = of_get_named_gpio(dt, "himax,3v3-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_3v3_en)) {
		E(" DT:gpio_3v3_en value is not valid\n");
	}
	pdata->gpio_1v8_en = of_get_named_gpio(dt, "himax,1v8-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_1v8_en)) {
		E(" DT:gpio_1v8_en value is not valid\n");
	}
	
#endif

	if (of_property_read_u32(dt, "report_type", &data) == 0) {
		pdata->protocol_type = data;
		I(" DT:protocol_type=%d", pdata->protocol_type);
	}

	pdata->invert_x = of_property_read_bool(dt, "himax,inver_x");

	pdata->invert_y  = of_property_read_bool(dt, "himax,inver_y");
	
	pdata->vdd_continue  = of_property_read_bool(dt, "himax,vdd_continue");


	himax_vk_parser(dt, pdata);

	return 0;
}
#endif


#ifdef SEC_PWRCTL_WITHLCD
void tsp_irq_and_pwr_ctl(bool on)
{
	I("%s %s from LCD, TSP irq:%d, pwr:%d \n", __func__, (on) ? "on" : "off", irq_enable_count, tsp_pwr_status);

	if(private_ts == NULL)
		return;

	if(on){
		//himax_83100_SenseOn(0x01);
		if((private_ts->suspended == false) && (!irq_enable_count))
			himax_int_enable(private_ts->client->irq, 1);
	}else{
		if(irq_enable_count)	// irq enable case
			himax_int_enable(private_ts->client->irq, 0);

		if(tsp_pwr_status)	// only power off case for sync.
			himax_power_control(private_ts->pdata, false, false);
	}
}
#endif

static int himax_83100_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0, err = 0;
	struct himax_ts_data *ts;
	struct himax_i2c_platform_data *pdata;

	I("%s: enter, %d \n", __func__, __LINE__);

	//Check I2C functionality
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		E("%s: i2c check functionality error\n", __func__);
		err = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(struct himax_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		E("%s: allocate himax_ts_data failed\n", __func__);
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}

	i2c_set_clientdata(client, ts);
	ts->client = client;
	ts->dev = &client->dev;

#if 0
	/* Remove this since this will cause request_firmware fail.
	   Thie line has no use to touch function. */
	dev_set_name(ts->dev, HIMAX_83100_NAME);
#endif
		pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL) { /*Allocate Platform data space*/
			err = -ENOMEM;
			goto err_dt_platform_data_fail;
		}

#ifdef CONFIG_OF
	if (client->dev.of_node) { /*DeviceTree Init Platform_data*/
		err = himax_parse_dt(ts, pdata);
		if (err < 0) {
			I(" pdata is NULL for DT\n");
			goto err_alloc_dt_pdata_failed;
		}
	} else
#else
	{
		pdata = client->dev.platform_data;
		if (pdata == NULL) {
			I(" pdata is NULL(dev.platform_data)\n");
			goto err_get_platform_data_fail;
		}
	}
#endif

	ts->rst_gpio = pdata->gpio_reset;


/* Get pinctrl if target uses pinctrl */
	ts->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(ts->pinctrl)) {
		if (PTR_ERR(ts->pinctrl) == -EPROBE_DEFER)
			goto err_alloc_dt_pdata_failed;

		pr_err("%s: Target does not use pinctrl\n", __func__);
		ts->pinctrl = NULL;
	}

	if (ts->pinctrl) {
		ret = himax_pinctrl_configure(ts, true);
		if (ret)
			pr_err("%s: cannot set ts pinctrl active state\n", __func__);
	}


#ifdef SEC_PWRCTL_WITHLCD
	if(pdata->vdd_continue){
		E("%s, power control was ignored.\n", __func__ );
	}else{
		himax_power_control(pdata, true, true);
		msleep(200);
	}
#else
	himax_gpio_power_config(ts->client, pdata);
#endif


#if 0 // unused
	if (pdata->power) {
		ret = pdata->power(1);
		if (ret < 0) {
			E("%s: power on failed\n", __func__);
			goto err_power_failed;
		}
	}
#endif
	private_ts = ts;

	//Get Himax IC Type / FW information / Calculate the point number
	//+++++++++++incell function+++++++++++++++++
	if (himax_83100_CheckChipVersion() == false) {
		E("Himax chip 83100 doesn NOT EXIST");
		goto err_ic_package_failed;
	}
	if (himax_ic_package_check(ts) == false) {
		E("Himax chip doesn NOT EXIST");
		goto err_ic_package_failed;
	}
	//-------------incell function-----------------------

	if (pdata->virtual_key)
		ts->button = pdata->virtual_key;
#ifdef  HX_TP_SYS_FLASH_DUMP
		ts->flash_wq = create_singlethread_workqueue("himax_flash_wq");
		if (!ts->flash_wq)
		{
			E("%s: create flash workqueue failed\n", __func__);
			err = -ENOMEM;
			goto err_create_wq_failed;
		}
	
		INIT_WORK(&ts->flash_work, himax_ts_flash_work_func);
	
		setSysOperation(0);
		setFlashBuffer();
#endif

	//Himax Power On and Load Config
	if (himax_loadSensorConfig(client, pdata) < 0) {
		E("%s: Load Sesnsor configuration failed, unload driver.\n", __func__);
		goto err_detect_failed;
	}	

	calculate_point_number();

#ifdef HX_TP_SYS_DIAG
	setXChannel(HX_RX_NUM); // X channel
	setYChannel(HX_TX_NUM); // Y channel

	setMutualBuffer();
	if (getMutualBuffer() == NULL) {
		E("%s: mutual buffer allocate fail failed\n", __func__);
		return -1;
	}
#ifdef HX_TP_SYS_2T2R
	if(Is_2T2R){
		setXChannel_2(HX_RX_NUM_2); // X channel
		setYChannel_2(HX_TX_NUM_2); // Y channel

		setMutualBuffer_2();

		if (getMutualBuffer_2() == NULL) {
			E("%s: mutual buffer 2 allocate fail failed\n", __func__);
			return -1;
		}
	}
#endif	
#endif
#if 0 // unused
	ts->power = pdata->power;
#endif
	ts->pdata = pdata;

	ts->x_channel = HX_RX_NUM;
	ts->y_channel = HX_TX_NUM;
	ts->nFinger_support = HX_MAX_PT;
	//calculate the i2c data size
	calcDataSize(ts->nFinger_support);
	I("%s: calcDataSize complete\n", __func__);
#ifdef CONFIG_OF
	ts->pdata->abs_pressure_min        = 0;
	ts->pdata->abs_pressure_max        = 200;
	ts->pdata->abs_width_min           = 0;
	ts->pdata->abs_width_max           = 200;
	pdata->cable_config[0]             = 0x90;
	pdata->cable_config[1]             = 0x00;
#endif
	ts->suspended                      = false;
#if defined(HX_USB_DETECT)	
	ts->usb_connected = 0x00;
	ts->cable_config = pdata->cable_config;
#endif
	ts->protocol_type = pdata->protocol_type;
	I("%s: Use Protocol Type %c\n", __func__,
	ts->protocol_type == PROTOCOL_TYPE_A ? 'A' : 'B');

	ret = himax_input_register(ts);
	if (ret) {
		E("%s: Unable to register %s input device\n",
			__func__, ts->input_dev->name);
		goto err_input_register_device_failed;
	}


#ifdef HX_SMART_WAKEUP
	ts->SMWP_enable=0;
	wake_lock_init(&ts->ts_SMWP_wake_lock, WAKE_LOCK_SUSPEND, HIMAX_83100_NAME);
	
	ts->himax_smwp_wq = create_singlethread_workqueue("HMX_SMWP_WORK");
		if (!ts->himax_smwp_wq) {
			E(" allocate himax_smwp_wq failed\n");
			err = -ENOMEM;
			goto err_get_intr_bit_failed;
		}
		INIT_DELAYED_WORK(&ts->smwp_work, himax_SMWP_work);
#endif
#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#if defined(SEC_FACTORY_TEST)
	himax_init_sec_factory(ts);
#else
	himax_touch_sysfs_init();
#endif
#endif

#if defined(HX_USB_DETECT)
	if (ts->cable_config)		
		cable_detect_register_notifier(&himax_cable_status_handler);
#endif

	err = himax_ts_register_interrupt(ts->client);
	if (err)
		goto err_register_interrupt_failed;

#ifdef CONFIG_SEC_INCELL
	incell_data.tsp_data = ts;
	incell_data.tsp_disable = himax_tsp_disable;
	incell_data.tsp_enable = himax_tsp_enable;
	incell_data.tsp_esd = himax_tsp_esd;
#endif

return 0;

err_register_interrupt_failed:
err_input_register_device_failed:
	input_free_device(ts->input_dev);

#ifdef HX_SMART_WAKEUP
	wake_lock_destroy(&ts->ts_SMWP_wake_lock);
#endif
err_detect_failed:
#ifdef  HX_TP_SYS_FLASH_DUMP
err_create_wq_failed:
#endif
err_ic_package_failed:

#ifdef CONFIG_OF
	err_alloc_dt_pdata_failed:
#else
	//err_power_failed:
	err_get_platform_data_fail:
#endif

	kfree(pdata);

err_dt_platform_data_fail:

	kfree(ts);

err_alloc_data_failed:
err_check_functionality_failed:
	return err;

}

static int himax_83100_remove(struct i2c_client *client)
{
	struct himax_ts_data *ts = i2c_get_clientdata(client);

	I("%s: enter, %d \n", __func__, __LINE__);

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#if defined(SEC_FACTORY_TEST)
	kfree(ts->factory_info);
#else
	himax_touch_sysfs_deinit();
#endif
#endif
	if (!ts->use_irq)
		hrtimer_cancel(&ts->timer);

	destroy_workqueue(ts->himax_wq);

	if (ts->protocol_type == PROTOCOL_TYPE_B)
		input_mt_destroy_slots(ts->input_dev);

	input_unregister_device(ts->input_dev);

#ifdef HX_SMART_WAKEUP
		wake_lock_destroy(&ts->ts_SMWP_wake_lock);
#endif
	kfree(ts);

	return 0;

}

#ifdef USE_OPEN_CLOSE
#ifdef QCT
static void himax_ts_close(struct input_dev *dev)
{
	int ret;
#ifdef HX_SMART_WAKEUP
//	uint8_t buf[2] = {0};
#endif
	struct himax_ts_data *ts = input_get_drvdata(dev);

	if(ts->suspended)
	{
		I("%s: Already suspended. Skipped. \n", __func__);
		return;
	}
	else
	{
		ts->suspended = true;
		I("%s: enter \n", __func__);
	}

#ifdef HX_TP_SYS_FLASH_DUMP
	if (getFlashDumpGoing())
	{
		I("[himax] %s: Flash dump is going, reject suspend\n",__func__);
		return;
	}
#endif
#ifdef HX_TP_SYS_HITOUCH
	if(hitouch_is_connect)
	{
		I("[himax] %s: Hitouch connect, reject suspend\n",__func__);
		return;
	}
#endif
#ifdef HX_SMART_WAKEUP
	if(ts->SMWP_enable)
	{
		atomic_set(&ts->suspend_mode, 1);
		ts->pre_finger_mask = 0;
		FAKE_POWER_KEY_SEND=false;
		I("[himax] %s: SMART_WAKEUP enable, reject suspend\n",__func__);
		return;
	}
#endif

	himax_int_enable(ts->client->irq,0);

	//Himax _83100 IC enter sleep mode

	if (!ts->use_irq) {
		ret = cancel_work_sync(&ts->work);
		if (ret)
			himax_int_enable(ts->client->irq,1);
	}
	
	ts->first_pressed = 0;
	ts->pre_finger_mask = 0;
	
	if (ts->pinctrl) {
		ret = himax_pinctrl_configure(ts, false);
		if (ret)
			pr_err("%s: cannot set ts pinctrl suspend state\n", __func__);
	}

#ifdef SEC_PWRCTL_WITHLCD
	if(ts->pdata->vdd_continue){
		E("%s, power control was ignored.\n", __func__ );
	}else{
		himax_power_control(ts->pdata, false, false);
		//msleep(200);
	}
#else
#if 0 // unused
	atomic_set(&ts->suspend_mode, 1);

	if (ts->pdata->powerOff3V3 && ts->pdata->power)
		ts->pdata->power(0);
#endif
#endif
	return;
}

static int himax_ts_open(struct input_dev *dev)
{
	struct himax_ts_data *ts = input_get_drvdata(dev);
	int ret;

	I("%s: enter \n", __func__);


	if (ts->pinctrl) {
		ret = himax_pinctrl_configure(ts, true);
		if (ret)
			pr_err("%s: cannot set ts pinctrl active state\n", __func__);
	}


#ifdef SEC_PWRCTL_WITHLCD
	if(ts->pdata->vdd_continue){
		E("%s, power control was ignored.\n", __func__ );
	}else{
		himax_power_control(ts->pdata, true, false);
		msleep(200);
	}
#else
#if 0 // unused
	if (ts->pdata->powerOff3V3 && ts->pdata->power)
		ts->pdata->power(1);

	atomic_set(&ts->suspend_mode, 0);
#endif
#endif

	himax_int_enable(ts->client->irq,1);

	ts->suspended = false;
#ifdef HX_SMART_WAKEUP
	queue_delayed_work(ts->himax_smwp_wq, &ts->smwp_work, msecs_to_jiffies(1000));
#endif
	return 0;
}
#endif
#endif

#ifdef CONFIG_SEC_INCELL
static void himax_tsp_disable(struct incell_driver_data *param)
{
	struct himax_ts_data *tsp_data;
	struct input_dev *dev;

	pr_info("%s\n", __func__);

	if (!incell_data.tsp_data) {
		pr_err("%s: No incell data found\n", __func__);
		return;
	}

	tsp_data = incell_data.tsp_data;
	dev = tsp_data->input_dev;

	himax_ts_close(dev);
}

static void himax_tsp_enable(struct incell_driver_data *param)
{
	struct himax_ts_data *tsp_data;
	struct input_dev *dev;

	pr_info("%s\n", __func__);

	if (!incell_data.tsp_data) {
		pr_err("%s: No incell data found\n", __func__);
		return;
	}

	tsp_data = incell_data.tsp_data;
	dev = tsp_data->input_dev;

	himax_ts_open(dev);
}

static void himax_tsp_esd(struct incell_driver_data *param)
{
	pr_info("%s\n", __func__);
}
#endif

static const struct i2c_device_id himax_83100_ts_id[] = {
	{HIMAX_83100_NAME, 0 },
	{}
};

#ifdef CONFIG_OF
static struct of_device_id himax_match_table[] = {
	{.compatible = "himax,hx83100" },
	{},
};
#else
#define himax_match_table NULL
#endif

static struct i2c_driver himax_83100_driver = {
	.id_table	= himax_83100_ts_id,
	.probe		= himax_83100_probe,
	.remove		= himax_83100_remove,
	.driver		= {
		.name = HIMAX_83100_NAME,
		.owner = THIS_MODULE,
		.of_match_table = himax_match_table,
	},
};

static void __init himax_83100_init_async(void *unused, async_cookie_t cookie)
{
	I("%s:Enter \n", __func__);
#ifdef QCT
	i2c_add_driver(&himax_83100_driver);
#endif
}

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif
static int __init himax_83100_init(void)
{
	I("Himax 83100 touch panel driver init\n");

#ifdef CONFIG_SAMSUNG_LPM_MODE
	if (poweroff_charging) {
		pr_info("%s() LPM Charging Mode!!\n", __func__);
		return 0;
	}
#endif

	async_schedule(himax_83100_init_async, NULL);
	return 0;
}

static void __exit himax_83100_exit(void)
{
#ifdef QCT
	i2c_del_driver(&himax_83100_driver);
#endif
}

module_init(himax_83100_init);
module_exit(himax_83100_exit);

MODULE_DESCRIPTION("Himax_83100 driver");
MODULE_LICENSE("GPL");
