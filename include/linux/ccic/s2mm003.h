/*
 * s2mm003.h - S2MM003 USBPD device driver header
 *
 * Copyright (C) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#ifndef __S2MM003_H
#define __S2MM003_H __FILE__

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/firmware.h>

#if defined(CONFIG_CCIC_NOTIFIER)
#include <linux/ccic/ccic_notifier.h>
#endif

#define AVAILABLE_VOLTAGE 13400
#define UNIT_FOR_VOLTAGE 50
#define CONFIG_SAMSUNG_S2MM003_SYSFS
#define CONFIG_SAMSUNG_S2MM003_SRAM_BUG_WORKAROUND

/******************************************************************************/
/* definitions & structures                                                   */
/******************************************************************************/
#define USBPD_DEV_NAME  "usbpd-s2mm003"
#define FIRMWARE_PATH "usbpd/USB_PD_FULL_DRIVER.bin"

#define	OTP_ADD_LOW		0x00
#define	OTP_ADD_HIGH		0x01
#define	OTP_RW			0x02
#define	OTP_BCHK_TEST		0x03
#define	OTP_READ_DATA		0x04
#define	OM			0x05
#define	IF_S_CODE_E		0x05
#define	MON_EN			0x06
#define	BUS_IF_SELECT		0x07
#define	I2C_PIN_SELECT		0x08
#define	I2C_DEV_ADDR		0x09
#define	I2C_DEV16_ADDR		0x0A
#define	I2C_8BIT_SLV_RST	0x0B
#define	OTP_BLK_RST		0x0C
#define	USB_PD_RST		0x0D
#define	IRQ_WR_ADDR		0x0E
#define	IRQ_WR_DATA		0x0F
#define	IRQ_RD_ADDR		0x10
#define	IRQ_RD_DATA		0x11
#define	A_TEST_MUX_SEL		0x12
#define	MON0_SEL		0x13
#define	MON1_SEL		0x14
#define	ANALOG_00		0x15
#define	ANALOG_01		0x16
#define	ANALOG_02		0x17
#define	ANALOG_03		0x18
#define	ANALOG_04		0x19
#define	ANALOG_05		0x1A
#define	ANALOG_06		0x1B
#define	ANALOG_07		0x1C
#define	ANALOG_08		0x1D
#define	ANALOG_09		0x1E
#define	ANALOG_10		0x1F
#define	ANALOG_11		0x20
#define	EXT_VCONN_TIMER		0x21
#define	OCP_CC_TIMER		0x22
#define	SELECTED_EXT_NFET_EN	0x23
#define	REDRVEN1_CC_SEL		0x24
#define	OTP_DUMP_DISABLE	0x30
#define	I2C_SYSREG_SET		0x31
#define	I2C_SRAM_SET		0x32

#define	INTERRUPT_CLEAR	0xFF

#define	INTERRUPT_STATUS1	0x1830
#define	INTERRUPT_STATUS2	0x1834
#define	INTERRUPT_STATUS3	0x1838
#define	INTERRUPT_STATUS4	0x183C
#define	INTERRUPT_STATUS5	0x1840

#define	PLUG_STATE_MONITOR	0x1850
#define	RID_GET_MONITOR		0x1854

/* PLUG_STATE_MONITOR : 0x1850 */
#define	S_CC1_VALID_MASK		0x07
#define	S_CC1_VALID_SHIFT		0x00
#define	S_CC2_VALID_MASK		0x38
#define	S_CC2_VALID_SHIFT		0x03
#define	PLUG_RPRD_SEL_MONITOR_MASK	0x40
#define	PLUG_RPRD_SEL_MONITOR_SHIFT	0x06
#define	PLUG_ATTACH_DONE_MASK		0x80
#define	PLUG_ATTACH_DONE_SHIFT		0x07

/* From S.LSI */
#define IND_REG_HOST_CMD_ON	0x05
#define IND_REG_PD_FUNC_STATE   0x16
#define IND_REG_INT_NUMBER      0x17
#define IND_REG_INT_STATUS_3    0x1A
#define IND_REG_INT_STATUS_5    0x1C
#define IND_REG_READ_MSG_INDEX  0x4F
#define IND_REG_MSG_ACCESS_BASE 0x50
#define MSG_BUF_REQUEST_SIZE    0x8 // 1-Byte Length
#define MSG_REQUEST_SIZE        0x2 // 4-Byte Length

#define b_RESET_START   (1 << 7)
#define b_Reserved      (1 << 6)
#define b_PD_FUNC_FLAG  (1 << 5)
#define b_INT_5_FLAG    (1 << 4)
#define b_INT_4_FLAG    (1 << 3)
#define b_INT_3_FLAG    (1 << 2)
#define b_INT_2_FLAG    (1 << 1)
#define b_INT_1_FLAG    (1 << 0)

//  Interrupt Status3
#define b_Msg_VCONN_SWAP            0x1
#define b_Msg_Wait                  0x2
#define b_Msg_SRC_CAP               0x4
#define b_Msg_SNK_CAP               0x8
#define b_Msg_Request               0x10
#define b_msg_soft_reset            0x20
#define b_RID_Detect_done           0x40

typedef enum
{
	// PDO Message
	MSG_Idx_TX_SRC_CAPA             = 1,
	MSG_Idx_TX_SINK_CAPA            = 2,
	MSG_Idx_TX_REQUEST              = 3,

	MSG_Idx_RX_SRC_CAPA             = 4,
	MSG_Idx_RX_SINK_CAPA            = 5,
	MSG_Idx_RX_REQUEST              = 6,

	// VDM User Message
	MSG_Idx_VDM_MSG_REQUEST         = 9,
	MSG_Idx_CTRL_MSG                = 10,

	// VDM TX Message
	MSG_Idx_TX_DISC_ID_RESP         = 17,
	MSG_Idx_TX_DISC_SVIDs_RESP      = 18,
	MSG_Idx_TX_DISC_MODE_RESP       = 19,
	MSG_Idx_TX_DISC_ENTER_MODE_RESP = 20,
	MSG_Idx_TX_DISC_EXIT_MODE_RESP  = 21,
	MSG_Idx_TX_DISC_ATTENTION_RESP  = 22,

	// VDM RX Message
	MSG_Idx_RX_DISC_ID_CABLE        = 25,
	MSG_Idx_RX_DISC_ID_RESP         = 26,
	MSG_Idx_RX_DISC_SVIDs_RESP      = 27,
	MSG_Idx_RX_DISC_MODE_RESP       = 28,
	MSG_Idx_RX_DISC_ENTER_MODE_RESP = 29,
	MSG_Idx_RX_DISC_EXIT_MODE_RESP  = 30,
	MSG_Idx_RX_DISC_ATTENTION_RESP  = 31

} Num_MSG_INDEX;

typedef struct
{
	uint16_t	Message_Type:4;
	uint16_t	Rsvd2_msg_header:1;
	uint16_t	Port_Data_Role:1;
	uint16_t	Specification_Revision:2;
	uint16_t	Port_Power_Role:1;
	uint16_t	Message_ID:3;
	uint16_t	Number_of_obj:3;
	uint16_t	Rsvd_msg_header:1;
} MSG_HEADER_Type;

typedef struct
{
	uint32_t    Maximum_Allow_Power:10;
	uint32_t    Minimum_Voltage:10;
	uint32_t    Maximum_Voltage:10;
	uint32_t    PDO_Parameter:2;
} SRC_BAT_SUPPLY_Typedef;

// ================== Capabilities Message ==============
// Source Capabilities
typedef struct
{
	uint32_t    Maximum_Current:10;
	uint32_t    Voltage_Unit:10;
	uint32_t    Peak_Current:2;
	uint32_t    Reserved:3;
	uint32_t    Data_Role_Swap:1;
	uint32_t    USB_Comm_Capable:1;
	uint32_t    Externally_POW:1;
	uint32_t    USB_Suspend_Support:1;
	uint32_t    Dual_Role_Power:1;
	uint32_t    PDO_Parameter:2;
}SRC_FIXED_SUPPLY_Typedef;

typedef struct
{
	uint32_t    Maximum_Current:10;
	uint32_t    Minimum_Voltage:10;
	uint32_t    Maximum_Voltage:10;
	uint32_t    PDO_Parameter:2;
}SRC_VAR_SUPPLY_Typedef;

// ================== Request Message ================

typedef struct
{
	uint32_t    Maximum_OP_Current:10;  // 10mA
	uint32_t    OP_Current:10;          // 10mA
	uint32_t    Reserved_1:4;           // Set to Zero
	uint32_t    No_USB_Suspend:1;
	uint32_t    USB_Comm_Capable:1;
	uint32_t    Capa_Mismatch:1;
	uint32_t    GiveBack_Flag:1;        // GiveBack Support set to 1
	uint32_t    Object_Position:3;      // 000 is reserved
	uint32_t    Reserved_2:1;
} REQUEST_FIXED_SUPPLY_STRUCT_Typedef;

#define S2MM003_REG_MASK(reg, mask)	((reg & mask##_MASK) >> mask##_SHIFT)


#if defined(CONFIG_SAMSUNG_S2MM003_SYSFS)
static ssize_t s2mm003_register_show(struct device *dev,
		struct device_attribute *att, char *buf);

static DEVICE_ATTR(s2mm003_register, S_IRUGO, s2mm003_register_show, NULL);
#endif

struct s2mm003_usbpd_data {
	struct device *dev;
	struct i2c_client *i2c;
	int irq_gpio;
};

int irq_times;
int wq_times;
int p_prev_rid = -1;
int prev_rid = -1;
int cur_rid = -1;
#endif /* __S2MM003_H */
