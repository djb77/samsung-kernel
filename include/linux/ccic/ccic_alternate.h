/*
 * include/linux/ccic/ccic_alternate.h - S2MM005 USB CCIC Alternate mode header file
 * 
 * Copyright (C) 2016 Samsung Electronics
 * Author: Wookwang Lee <wookwang.lee@samsung.com>
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
#ifndef __LINUX_CCIC_ALTERNATE_MODE_H__
#define __LINUX_CCIC_ALTERNATE_MODE_H__
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    VDM_command:5,
				    Rsvd2_VDM_header:1,
				    VDM_command_type:2,
				    Object_Position:3,
				    Rsvd_VDM_header:2,
			    	Structured_VDM_Version:2,
				    VDM_Type:1,
				    Standard_Vendor_ID:16;
	}BITS;
}U_DATA_MSG_VDM_HEADER_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    USB_Vendor_ID:16,
			    	Rsvd_ID_header:10,
				    Modal_Operation_Supported:1,
				    Product_Type:3,
				    Data_Capable_USB_Device:1,
				    Data_Capable_USB_Host:1;
	}BITS;
}U_DATA_MSG_ID_HEADER_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    Cert_TID:20,
			    	Rsvd_cert_VDOer:12;
	}BITS;
}U_CERT_STAT_VDO_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    Device_Version:16,
			    	Product_ID:16;
	}BITS;
}U_PRODUCT_VDO_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    USB_Superspeed_Signaling_Support:3,
				    SOP_contoller_present:1,
				    Vbus_through_cable:1,
				    Vbus_Current_Handling_Capability:2,
				    SSRX2_Directionality_Support:1,
				    SSRX1_Directionality_Support:1,
				    SSTX2_Directionality_Support:1,
				    SSTX1_Directionality_Support:1,
				    Cable_Termination_Type:2,
				    Cable_Latency:4,
				    TypeC_to_Plug_Receptacle:1,
				    TypeC_to_ABC:2,
				    Rsvd_CABLE_VDO:4,
				    Cable_Firmware_Version:4,
				    Cable_HW_Version:4;
	}BITS;
}U_CABLE_VDO_Type;

typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    SVID_1:16,
				    SVID_0:16;
	}BITS;
}U_VDO1_Type;


typedef union
{
	uint32_t        DATA;
	struct
	{
		uint8_t     BDATA[4];
	}BYTES;
	struct
	{
		uint32_t    VENDOR_DEFINED_MESSAGE:15,
					VDM_TYPE:1,
				    USB_Vendor_ID:16;
	}BITS;
}U_UNSTRUCTURED_VDM_HEADER_Type;

typedef struct
{
	uint32_t	VDO[6];
} VDO_MESSAGE_Type;

/* CCIC Dock Observer Callback parameter */
enum {
	CCIC_DOCK_DETACHED	= 0,
	CCIC_DOCK_HMT		= 105,
	CCIC_DOCK_ABNORMAL	= 106,
};

enum VDM_MSG_IRQ_State {
	VDM_DISCOVER_ID		=	(1 << 0),
	VDM_DISCOVER_SVIDS	=	(1 << 1),
	VDM_DISCOVER_MODES	=	(1 << 2),
	VDM_ENTER_MODE		=	(1 << 3),
	VDM_EXIT_MODE 		=	(1 << 4),
	VDM_ATTENTION		=	(1 << 5),
};

// VMD Message Register I2C address by S.LSI
#define REG_VDM_MSG_REQ					0x02C0
#define REG_SSM_MSG_READ				0x0340
#define REG_SSM_MSG_SEND				0x0360

#define REG_TX_DIS_ID_RESPONSE			0x0400
#define REG_TX_DIS_SVID_RESPONSE		0x0420
#define REG_TX_DIS_MODE_RESPONSE		0x0440
#define REG_TX_ENTER_MODE_RESPONSE		0x0460
#define REG_TX_EXIT_MODE_RESPONSE		0x0480
#define REG_TX_DIS_ATTENTION_RESPONSE	0x04A0

#define REG_RX_DIS_ID_CABLE				0x0500
#define REG_RX_DIS_ID					0x0520
#define REG_RX_DIS_SVID					0x0540
#define REG_RX_MODE						0x0560
#define REG_RX_ENTER_MODE				0x0580
#define REG_RX_EXIT_MODE				0x05A0
#define REG_RX_DIS_ATTENTION			0x05C0

#define GEAR_VR_DETACH_WAIT_MS 			1000

#define MODE_INT_CLEAR					0x01
#define PD_NEXT_STATE					0x02
#define MODE_INTERFACE 					0x03
#define REQ_PR_SWAP						0x10
#define REQ_DR_SWAP						0x11
#define SEL_SSM_MSG_REQ 				0x20

#define SAMSUNG_VENDOR_ID 				0x04E8
#define GEARVR_PRODUCT_ID				0xA500
#define DISPLAY_PORT_SVID				0xFF01

void send_alternate_message(void * data, int cmd);
void receive_alternate_message(void * data, VDM_MSG_IRQ_STATUS_Type *VDM_MSG_IRQ_State);
int ccic_register_switch_device(int mode);
void acc_detach_check(struct work_struct *work);
void send_unstructured_vdm_message(void * data, int cmd);
void receive_unstructured_vdm_message(void * data, SSM_MSG_IRQ_STATUS_Type *SSM_MSG_IRQ_State);
void send_role_swap_message(void * data, int cmd);
void send_attention_message(void * data, int cmd);
void do_alternate_mode_step_by_step(void * data, int cmd);
#else
inline void send_alternate_message(void * data, int cmd) {}
inline void receive_alternate_message(void * data, VDM_MSG_IRQ_STATUS_Type *VDM_MSG_IRQ_State) {}
inline int ccic_register_switch_device(int mode) { return 0; }
inline void acc_detach_check(struct work_struct *work) {}
inline void send_unstructured_vdm_message(void * data, int cmd) {}
inline void receive_unstructured_vdm_message(void * data, SSM_MSG_IRQ_STATUS_Type *SSM_MSG_IRQ_State) {}
inline void send_role_swap_message(void * data, int cmd) {}
inline void send_attention_message(void * data, int cmd) {}
inline void do_alternate_mode_step_by_step(void * data, int cmd) {}
#endif
#endif
