/*
 * driver/ccic/ccic_alternate.c - S2MM005 USB CCIC Alternate mode driver
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
#include <linux/ccic/s2mm005.h>
#include <linux/ccic/s2mm005_ext.h>
#include <linux/ccic/ccic_alternate.h>
/* switch device header */
#if defined(CONFIG_SWITCH)
#include <linux/switch.h>
#endif /* CONFIG_SWITCH */
////////////////////////////////////////////////////////////////////////////////
// s2mm005_cc.c called s2mm005_alternate.c
////////////////////////////////////////////////////////////////////////////////
#if defined(CONFIG_CCIC_ALTERNATE_MODE)
#if defined(CONFIG_SWITCH)
static struct switch_dev switch_dock = {
	.name = "ccic_dock",
};
#endif /* CONFIG_SWITCH */

static char VDM_MSG_IRQ_State_Print[7][40] =
{
    {"bFLAG_Vdm_Reserve_b0"},
    {"bFLAG_Vdm_Discover_ID"},
    {"bFLAG_Vdm_Discover_SVIDs"},
    {"bFLAG_Vdm_Discover_MODEs"},
    {"bFLAG_Vdm_Enter_Mode"},
    {"bFLAG_Vdm_Exit_Mode"},
    {"bFLAG_Vdm_Attention"},
};
////////////////////////////////////////////////////////////////////////////////
// Alternate mode processing
////////////////////////////////////////////////////////////////////////////////
int ccic_register_switch_device(int mode)
{
#ifdef CONFIG_SWITCH
	int ret = 0;
	if (mode) {
		ret = switch_dev_register(&switch_dock);
		if (ret < 0) {
			pr_err("%s: Failed to register dock switch(%d)\n",
				__func__, ret);
			return -ENODEV;
		}
	} else {
		switch_dev_unregister(&switch_dock);
	}
#endif /* CONFIG_SWITCH */
	return 0;
}

static void ccic_send_dock_intent(int type)
{
	pr_info("%s: CCIC dock type(%d)\n", __func__, type);
#ifdef CONFIG_SWITCH
	switch_set_state(&switch_dock, type);
#endif /* CONFIG_SWITCH */
}

void acc_detach_check(struct work_struct *wk)
{
	struct delayed_work *delay_work =
		container_of(wk, struct delayed_work, work);
	struct s2mm005_data *usbpd_data =
		container_of(delay_work, struct s2mm005_data, acc_detach_work);
	pr_info("%s: usbpd_data->pd_state : %d\n", __func__, usbpd_data->pd_state);
	if (usbpd_data->pd_state == State_PE_Initial_detach) {
		ccic_send_dock_intent(CCIC_DOCK_DETACHED);
		usbpd_data->acc_type = CCIC_DOCK_DETACHED;
	}
}

static int process_check_accessory(void * data)
{
	struct s2mm005_data *usbpd_data = data;
	struct otg_notify *o_notify = get_otg_notify();

	// detect Gear VR
	if (usbpd_data->Vendor_ID == SAMSUNG_VENDOR_ID &&
		usbpd_data->Product_ID >= GEARVR_PRODUCT_ID &&
		usbpd_data->Product_ID <= GEARVR_PRODUCT_ID + 5) {
		pr_info("%s : Samsung Gear VR connected.\n", __func__);
		o_notify->hw_param[USB_CCIC_VR_USE_COUNT]++;
		if (usbpd_data->acc_type == CCIC_DOCK_DETACHED) {
			ccic_send_dock_intent(CCIC_DOCK_HMT);
			usbpd_data->acc_type = CCIC_DOCK_HMT;
		}
		return 1;
	}
	return 0;
}

static void process_discover_identity(void * data)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_RX_DIS_ID;
	uint8_t ReadMSG[32] = {0,};
	int ret = 0;
	
	// Message Type Definition
	U_DATA_MSG_ID_HEADER_Type     *DATA_MSG_ID = (U_DATA_MSG_ID_HEADER_Type *)&ReadMSG[8];
	U_PRODUCT_VDO_Type			  *DATA_MSG_PRODUCT = (U_PRODUCT_VDO_Type *)&ReadMSG[16];
	
	ret = s2mm005_read_byte(i2c, REG_ADD, ReadMSG, 32);
	if (ret < 0) {
		dev_err(&i2c->dev, "%s has i2c error.\n", __func__);
		return;
	}

	usbpd_data->Vendor_ID = DATA_MSG_ID->BITS.USB_Vendor_ID;
	usbpd_data->Product_ID = DATA_MSG_PRODUCT->BITS.Product_ID;

	dev_info(&i2c->dev, "%s Vendor_ID : 0x%X, Product_ID : 0x%X\n", 
		__func__, usbpd_data->Vendor_ID, usbpd_data->Product_ID);
	if (process_check_accessory(usbpd_data))
		dev_info(&i2c->dev, "%s : Samsung Accessory Connected.\n", __func__);
}

static void process_discover_svids(void * data)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_RX_DIS_SVID;
	uint8_t ReadMSG[32] = {0,};
	int ret = 0;
	struct otg_notify *o_notify = get_otg_notify();

	// Message Type Definition
	U_VDO1_Type 				  *DATA_MSG_VDO1 = (U_VDO1_Type *)&ReadMSG[8];

	ret = s2mm005_read_byte(i2c, REG_ADD, ReadMSG, 32);
	if (ret < 0) {
		dev_err(&i2c->dev, "%s has i2c error.\n", __func__);
		return;
	}

	usbpd_data->SVID_0 = DATA_MSG_VDO1->BITS.SVID_0;
	usbpd_data->SVID_1 = DATA_MSG_VDO1->BITS.SVID_1;

	dev_info(&i2c->dev, "%s SVID_0 : 0x%X, SVID_1 : 0x%X\n", __func__, usbpd_data->SVID_0, usbpd_data->SVID_1);
	if (usbpd_data->SVID_0 == DISPLAY_PORT_SVID)
		o_notify->hw_param[USB_CCIC_DP_USE_COUNT]++;
}

static void process_discover_modes(void * data)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_RX_MODE;
	uint8_t ReadMSG[32] = {0,};
	int ret = 0;

	ret = s2mm005_read_byte(i2c, REG_ADD, ReadMSG, 32);
	if (ret < 0) {
		dev_err(&i2c->dev, "%s has i2c error.\n", __func__);
		return;
	}

	dev_info(&i2c->dev, "%s\n", __func__);
}

static void process_enter_mode(void * data)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_RX_ENTER_MODE;
	uint8_t ReadMSG[32] = {0,};
	int ret = 0;

	// Message Type Definition
	U_DATA_MSG_VDM_HEADER_Type	  *DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&ReadMSG[4];
	
	ret = s2mm005_read_byte(i2c, REG_ADD, ReadMSG, 32);
	if (ret < 0) {
		dev_err(&i2c->dev, "%s has i2c error.\n", __func__);
		return;
	}
	if (DATA_MSG_VDM->BITS.VDM_command_type == 1)
		dev_info(&i2c->dev, "%s : EnterMode ACK.\n", __func__);
	else
		dev_info(&i2c->dev, "%s : EnterMode NAK.\n", __func__);
}

static void process_attention(void * data)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_RX_DIS_ATTENTION;
	uint8_t ReadMSG[32] = {0,};
	int ret = 0;
	int i;

	VDO_MESSAGE_Type *VDO_MSG = (VDO_MESSAGE_Type *)&ReadMSG[8];

	ret = s2mm005_read_byte(i2c, REG_ADD, ReadMSG, 32);
	if (ret < 0) {
		dev_err(&i2c->dev, "%s has i2c error.\n", __func__);
		return;
	}

	for(i=0;i<6;i++)
		dev_info(&i2c->dev, "%s : VDO_%d : %d\n", __func__, i+1, VDO_MSG->VDO[i]);
}

static void process_alternate_mode(void * data)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint32_t mode = usbpd_data->alternate_state;

	if (mode) {
		dev_info(&i2c->dev, "%s, mode : 0x%x\n", __func__, mode);

		if (mode & VDM_DISCOVER_ID)
			process_discover_identity(usbpd_data);
		if (mode & VDM_DISCOVER_SVIDS)
			process_discover_svids(usbpd_data);
		if (mode & VDM_DISCOVER_MODES)
			process_discover_modes(usbpd_data);
		if (mode & VDM_ENTER_MODE)
			process_enter_mode(usbpd_data);
		if (mode & VDM_ATTENTION)
			process_attention(usbpd_data);
		usbpd_data->alternate_state = 0;
	}
}

void send_alternate_message(void * data, int cmd)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_VDM_MSG_REQ;
	u8 mode = (u8)cmd;
	dev_info(&i2c->dev, "%s : %s\n",__func__, &VDM_MSG_IRQ_State_Print[cmd][0]);
	s2mm005_write_byte(i2c, REG_ADD, &mode, 1);
}

void receive_alternate_message(void * data, VDM_MSG_IRQ_STATUS_Type *VDM_MSG_IRQ_State)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;

	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_ID) {
		dev_info(&i2c->dev, "%s : %s\n",__func__, &VDM_MSG_IRQ_State_Print[1][0]);
		usbpd_data->alternate_state |= VDM_DISCOVER_ID;
	}
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_SVIDs) {
		dev_info(&i2c->dev, "%s : %s\n",__func__, &VDM_MSG_IRQ_State_Print[2][0]);
		usbpd_data->alternate_state |= VDM_DISCOVER_SVIDS;
	}
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Discover_MODEs) {
		dev_info(&i2c->dev, "%s : %s\n",__func__, &VDM_MSG_IRQ_State_Print[3][0]);
		usbpd_data->alternate_state |= VDM_DISCOVER_MODES;
	}
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Enter_Mode) {
		dev_info(&i2c->dev, "%s : %s\n",__func__, &VDM_MSG_IRQ_State_Print[4][0]);
		usbpd_data->alternate_state |= VDM_ENTER_MODE;
	}
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Exit_Mode) {
		dev_info(&i2c->dev, "%s : %s\n",__func__, &VDM_MSG_IRQ_State_Print[5][0]);
		usbpd_data->alternate_state |= VDM_EXIT_MODE;
	}
	if (VDM_MSG_IRQ_State->BITS.Vdm_Flag_Attention) {
		dev_info(&i2c->dev, "%s : %s\n",__func__, &VDM_MSG_IRQ_State_Print[6][0]);
		usbpd_data->alternate_state |= VDM_ATTENTION;
	}

	process_alternate_mode(usbpd_data);
}

void send_unstructured_vdm_message(void * data, int cmd)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_SSM_MSG_SEND;
	uint8_t SendMSG[32] = {0,};
	u8 W_DATA[2];
	uint32_t message = (uint32_t)cmd;
	int i;

	// Message Type Definition
	MSG_HEADER_Type *MSG_HDR = (MSG_HEADER_Type *)&SendMSG[0];
	U_UNSTRUCTURED_VDM_HEADER_Type	*DATA_MSG_UVDM = (U_UNSTRUCTURED_VDM_HEADER_Type *)&SendMSG[4];
	VDO_MESSAGE_Type *VDO_MSG = (VDO_MESSAGE_Type *)&SendMSG[8];

	// fill message
	MSG_HDR->Message_Type = 15; // send VDM message
	MSG_HDR->Number_of_obj = 7; // VDM Header + 6 VDOs = MAX 7
	
	DATA_MSG_UVDM->BITS.USB_Vendor_ID = SAMSUNG_VENDOR_ID; // VID

	for(i=0;i<6;i++)
		VDO_MSG->VDO[i] = message; // VD01~VDO6 : Max 24bytes

	s2mm005_write_byte(i2c, REG_ADD, SendMSG, 32);

	// send uVDM message
	REG_ADD = REG_I2C_SLV_CMD;
	W_DATA[0] = MODE_INTERFACE;
	W_DATA[1] = SEL_SSM_MSG_REQ;
	s2mm005_write_byte(i2c, REG_ADD, &W_DATA[0], 2);

	dev_info(&i2c->dev, "%s - message : 0x%x\n", __func__, message);
}

void receive_unstructured_vdm_message(void * data, SSM_MSG_IRQ_STATUS_Type *SSM_MSG_IRQ_State)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_SSM_MSG_READ;
	uint8_t ReadMSG[32] = {0,};
	u8 W_DATA[1];
	int i;
	
	uint16_t *VID = (uint16_t *)&ReadMSG[6];
	VDO_MESSAGE_Type *VDO_MSG = (VDO_MESSAGE_Type *)&ReadMSG[8];

	s2mm005_read_byte(i2c, REG_ADD, ReadMSG, 32);
	dev_info(&i2c->dev, "%s : VID : 0x%x\n", __func__, *VID);
	for(i=0;i<6;i++)
		dev_info(&i2c->dev, "%s : VDO_%d : %d\n", __func__, i+1, VDO_MSG->VDO[i]);

	REG_ADD = REG_I2C_SLV_CMD;
	W_DATA[0] = MODE_INT_CLEAR;
	s2mm005_write_byte(i2c, REG_ADD, &W_DATA[0], 1);
}

void send_role_swap_message(void * data, int cmd) // cmd 0 : PR_SWAP, cmd 1 : DR_SWAP
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_I2C_SLV_CMD;
	u8 mode = (u8)cmd;
	u8 W_DATA[2];

	// send uVDM message
	REG_ADD = REG_I2C_SLV_CMD;
	W_DATA[0] = MODE_INTERFACE;
	W_DATA[1] = mode ? REQ_DR_SWAP : REQ_PR_SWAP;
	s2mm005_write_byte(i2c, REG_ADD, &W_DATA[0], 2);

	dev_info(&i2c->dev, "%s : sent %s message\n", __func__, mode ? "DR_SWAP" : "PR_SWAP");
}

void send_attention_message(void * data, int cmd)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = REG_TX_DIS_ATTENTION_RESPONSE;
	uint8_t SendMSG[32] = {0,};
	u8 W_DATA[3];
	uint32_t message = (uint32_t)cmd;
	int i;

	// Message Type Definition
	MSG_HEADER_Type *MSG_HDR = (MSG_HEADER_Type *)&SendMSG[0];
	U_DATA_MSG_VDM_HEADER_Type	  *DATA_MSG_VDM = (U_DATA_MSG_VDM_HEADER_Type *)&SendMSG[4];
	VDO_MESSAGE_Type *VDO_MSG = (VDO_MESSAGE_Type *)&SendMSG[8];

	// fill message
	DATA_MSG_VDM->BITS.VDM_command = 6; // attention
	DATA_MSG_VDM->BITS.VDM_Type = 1; // structured VDM
	DATA_MSG_VDM->BITS.Standard_Vendor_ID = SAMSUNG_VENDOR_ID;

	MSG_HDR->Message_Type = 15; // send VDM message
	MSG_HDR->Number_of_obj = 7; // VDM Header + 6 VDOs = MAX 7

	for(i=0;i<6;i++)
		VDO_MSG->VDO[i] = message; // VD01~VDO6 : Max 24bytes

	s2mm005_write_byte(i2c, REG_ADD, SendMSG, 32);

	REG_ADD = REG_I2C_SLV_CMD;
	W_DATA[0] = MODE_INTERFACE;
	W_DATA[1] = PD_NEXT_STATE;
	W_DATA[2] = 100;
	s2mm005_write_byte(i2c, REG_ADD, &W_DATA[0], 3);

	dev_info(&i2c->dev, "%s - message : 0x%x\n", __func__, message);
}

void do_alternate_mode_step_by_step(void * data, int cmd)
{
	struct s2mm005_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
	uint16_t REG_ADD = 0;
	u8 W_DATA[3];

	REG_ADD = REG_I2C_SLV_CMD;
	W_DATA[0] = MODE_INTERFACE;
	W_DATA[1] = PD_NEXT_STATE;
	switch (cmd) {
		case VDM_DISCOVER_ID:
			W_DATA[2] = 80;
			break;
		case VDM_DISCOVER_SVIDS:
			W_DATA[2] = 83;
			break;
		case VDM_DISCOVER_MODES:
			W_DATA[2] = 86;
			break;
		case VDM_ENTER_MODE:
			W_DATA[2] = 89;
			break;
		case VDM_EXIT_MODE:
			W_DATA[2] = 92;
			break;
	}
	s2mm005_write_byte(i2c, REG_ADD, &W_DATA[0], 3);

	dev_info(&i2c->dev, "%s\n", __func__);
}
#endif
