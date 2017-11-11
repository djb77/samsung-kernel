/*
 * driver/../s2mm003.c - S2MM003 USBPD device driver
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

#include <linux/ccic/s2mm003.h>

static int s2mm003_read_byte(const struct i2c_client *i2c, u8 reg)
{
	int ret; u8 wbuf, rbuf;
	struct i2c_msg msg[2];

	msg[0].addr = i2c->addr;
	msg[0].flags = i2c->flags;
	msg[0].len = 1;
	msg[0].buf = &wbuf;
	msg[1].addr = i2c->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = &rbuf;

	wbuf = (reg & 0xFF);

	ret = i2c_transfer(i2c->adapter, msg, 2);
	if (ret < 0)
		dev_err(&i2c->dev, "i2c reading fail reg(0x%x), error %d\n",
			reg, ret);

	return rbuf;
}

static int s2mm003_read_byte_16(const struct i2c_client *i2c, u16 reg, u8 *val)
{
	int ret; u8 wbuf[2], rbuf;
	struct i2c_msg msg[2];

	msg[0].addr = 0x43;
	msg[0].flags = i2c->flags;
	msg[0].len = 2;
	msg[0].buf = wbuf;
	msg[1].addr = 0x43;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = &rbuf;

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);

	ret = i2c_transfer(i2c->adapter, msg, 2);
	if (ret < 0)
		dev_err(&i2c->dev, "i2c read16 fail reg(0x%x), error %d\n",
			reg, ret);

	*val = rbuf;
	return rbuf;
}

static int s2mm003_write_byte(const struct i2c_client *i2c, u8 reg, u8 val)
{
	int ret = 0; u8 wbuf[2];
	struct i2c_msg msg[1];

	msg[0].addr = i2c->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = wbuf;

	wbuf[0] = (reg & 0xFF);
	wbuf[1] = (val & 0xFF);

	ret = i2c_transfer(i2c->adapter, msg, 1);
	if (ret < 0)
		dev_err(&i2c->dev, "i2c write fail reg(0x%x:%x), error %d\n",
				reg, val, ret);

	return ret;
}

static int s2mm003_write_byte_16(const struct i2c_client *i2c, u16 reg, u8 val)
{
	int ret = 0; u8 wbuf[3];
	struct i2c_msg msg[1] = {
		{
			.addr = 0x43,
			.flags = 0,
			.len = 3,
			.buf = wbuf
		}
	};

	wbuf[0] = (reg & 0xFF00) >> 8;
	wbuf[1] = (reg & 0xFF);
	wbuf[2] = (val & 0xFF);

	ret = i2c_transfer(i2c->adapter, msg, 1);
	if (ret < 0)
		dev_err(&i2c->dev, "i2c write fail reg(0x%x:%x), error %d\n",
				reg, val, ret);

	return ret;
}

static void s2mm003_int_clear(struct s2mm003_usbpd_data *usbpd_data)
{
	struct i2c_client *i2c = usbpd_data->i2c;

	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);
	s2mm003_write_byte(i2c, IRQ_RD_ADDR, 0xFF);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);

}

static int s2mm003_indirect_read(struct s2mm003_usbpd_data *usbpd_data, uint8_t address)
{
	uint8_t value = 0;
	struct i2c_client *i2c = usbpd_data->i2c;

	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);
	s2mm003_write_byte(i2c, IRQ_RD_ADDR, address);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);

	value  = s2mm003_read_byte(i2c, IRQ_RD_DATA);

	return value;
}

static void s2mm003_indirect_write(struct s2mm003_usbpd_data *usbpd_data, uint8_t address, uint8_t val)
{
	struct i2c_client *i2c = usbpd_data->i2c;

	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);
	s2mm003_write_byte(i2c, IRQ_WR_ADDR, address);
	s2mm003_write_byte(i2c, IRQ_WR_DATA, val);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x0);
}

void s2mm003_src_capacity_information(const struct i2c_client *i2c, uint32_t *RX_SRC_CAPA_MSG, int * choice)
{
	uint32_t RdCnt;
	uint32_t PDO_cnt;
	uint32_t PDO_sel;

	MSG_HEADER_Type *MSG_HDR;
	SRC_FIXED_SUPPLY_Typedef *MSG_FIXED_SUPPLY;
	SRC_VAR_SUPPLY_Typedef  *MSG_VAR_SUPPLY;
	SRC_BAT_SUPPLY_Typedef  *MSG_BAT_SUPPLY;

	dev_info(&i2c->dev, "\n\r");
	for(RdCnt=0;RdCnt<8;RdCnt++)
	{
		dev_info(&i2c->dev, "Rd_SRC_CAPA_%d : 0x%X\n\r", RdCnt, RX_SRC_CAPA_MSG[RdCnt]);
	}

	MSG_HDR = (MSG_HEADER_Type *)&RX_SRC_CAPA_MSG[0];
	dev_info(&i2c->dev, "\n\r");
	dev_info(&i2c->dev, "=======================================\n\r");
	dev_info(&i2c->dev, "    MSG Header\n\r");

	dev_info(&i2c->dev, "    Rsvd_msg_header         : %d\n\r",MSG_HDR->Rsvd_msg_header );
	dev_info(&i2c->dev, "    Number_of_obj           : %d\n\r",MSG_HDR->Number_of_obj );
	dev_info(&i2c->dev, "    Message_ID              : %d\n\r",MSG_HDR->Message_ID );
	dev_info(&i2c->dev, "    Port_Power_Role         : %d\n\r",MSG_HDR->Port_Power_Role );
	dev_info(&i2c->dev, "    Specification_Revision  : %d\n\r",MSG_HDR->Specification_Revision );
	dev_info(&i2c->dev, "    Port_Data_Role          : %d\n\r",MSG_HDR->Port_Data_Role );
	dev_info(&i2c->dev, "    Rsvd2_msg_header        : %d\n\r",MSG_HDR->Rsvd2_msg_header );
	dev_info(&i2c->dev, "    Message_Type            : %d\n\r",MSG_HDR->Message_Type );

	for(PDO_cnt = 0;PDO_cnt < MSG_HDR->Number_of_obj;PDO_cnt++)
	{
		PDO_sel = (RX_SRC_CAPA_MSG[PDO_cnt + 1] >> 30) & 0x3;
		dev_info(&i2c->dev, "    =================\n\r");
		dev_info(&i2c->dev, "    PDO_Num : %d\n\r", (PDO_cnt + 1));
		if(PDO_sel == 0)        // *MSG_FIXED_SUPPLY
		{
			MSG_FIXED_SUPPLY = (SRC_FIXED_SUPPLY_Typedef *)&RX_SRC_CAPA_MSG[PDO_cnt + 1];
			if(MSG_FIXED_SUPPLY->Voltage_Unit <= (AVAILABLE_VOLTAGE/UNIT_FOR_VOLTAGE)) 
				*choice = PDO_cnt + 1;

			dev_info(&i2c->dev, "    PDO_Parameter(FIXED_SUPPLY) : %d\n\r",MSG_FIXED_SUPPLY->PDO_Parameter );
			dev_info(&i2c->dev, "    Dual_Role_Power         : %d\n\r",MSG_FIXED_SUPPLY->Dual_Role_Power );
			dev_info(&i2c->dev, "    USB_Suspend_Support     : %d\n\r",MSG_FIXED_SUPPLY->USB_Suspend_Support );
			dev_info(&i2c->dev, "    Externally_POW          : %d\n\r",MSG_FIXED_SUPPLY->Externally_POW );
			dev_info(&i2c->dev, "    USB_Comm_Capable        : %d\n\r",MSG_FIXED_SUPPLY->USB_Comm_Capable );
			dev_info(&i2c->dev, "    Data_Role_Swap          : %d\n\r",MSG_FIXED_SUPPLY->Data_Role_Swap );
			dev_info(&i2c->dev, "    Reserved                : %d\n\r",MSG_FIXED_SUPPLY->Reserved );
			dev_info(&i2c->dev, "    Peak_Current            : %d\n\r",MSG_FIXED_SUPPLY->Peak_Current );
			dev_info(&i2c->dev, "    Voltage_Unit            : %d\n\r",MSG_FIXED_SUPPLY->Voltage_Unit );
			dev_info(&i2c->dev, "    Maximum_Current         : %d\n\r",MSG_FIXED_SUPPLY->Maximum_Current );

		}
		else if(PDO_sel == 2)   // *MSG_VAR_SUPPLY
		{
			MSG_VAR_SUPPLY = (SRC_VAR_SUPPLY_Typedef *)&RX_SRC_CAPA_MSG[PDO_cnt + 1];

			dev_info(&i2c->dev, "    PDO_Parameter(VAR_SUPPLY) : %d\n\r",MSG_VAR_SUPPLY->PDO_Parameter );
			dev_info(&i2c->dev, "    Maximum_Voltage          : %d\n\r",MSG_VAR_SUPPLY->Maximum_Voltage );
			dev_info(&i2c->dev, "    Minimum_Voltage          : %d\n\r",MSG_VAR_SUPPLY->Minimum_Voltage );
			dev_info(&i2c->dev, "    Maximum_Current          : %d\n\r",MSG_VAR_SUPPLY->Maximum_Current );

		}
		else if(PDO_sel == 1)   // *MSG_BAT_SUPPLY
		{
			MSG_BAT_SUPPLY = (SRC_BAT_SUPPLY_Typedef *)&RX_SRC_CAPA_MSG[PDO_cnt + 1];

			dev_info(&i2c->dev, "    PDO_Parameter(BAT_SUPPLY)  : %d\n\r",MSG_BAT_SUPPLY->PDO_Parameter );
			dev_info(&i2c->dev, "    Maximum_Voltage            : %d\n\r",MSG_BAT_SUPPLY->Maximum_Voltage );
			dev_info(&i2c->dev, "    Minimum_Voltage            : %d\n\r",MSG_BAT_SUPPLY->Minimum_Voltage );
			dev_info(&i2c->dev, "    Maximum_Allow_Power        : %d\n\r",MSG_BAT_SUPPLY->Maximum_Allow_Power );

		}

	}

	dev_info(&i2c->dev, "=======================================\n\r");
	dev_info(&i2c->dev, "\n\r");
	return;
}

void s2mm003_request_select_type(uint32_t * REQ_MSG , int num)
{
	REQUEST_FIXED_SUPPLY_STRUCT_Typedef *REQ_FIXED_SPL;

	REQ_FIXED_SPL = (REQUEST_FIXED_SUPPLY_STRUCT_Typedef *)REQ_MSG;

	//REQ_FIXED_SPL->Reserved_2                   = 0;
	REQ_FIXED_SPL->Object_Position             = (num & 0x7);
	//REQ_FIXED_SPL->GiveBack_Flag               = 0;     // GiveBack Support set to 1
	//REQ_FIXED_SPL->Capa_Mismatch            = 0;
	//REQ_FIXED_SPL->USB_Comm_Capable    = 0;
	//REQ_FIXED_SPL->No_USB_Suspend        = 0;
	//REQ_FIXED_SPL->Reserved_1                  = 0;     // Set to Zero
	REQ_FIXED_SPL->OP_Current                   = 200;    // 10mA
	REQ_FIXED_SPL->Maximum_OP_Current   = 200;    // 10mA
}

static irqreturn_t s2mm003_usbpd_irq_thread(int irq, void *data)
{
	struct s2mm003_usbpd_data *usbpd_data = data;
	struct i2c_client *i2c = usbpd_data->i2c;
#if !defined(CONFIG_SAMSUNG_S2MM003_SRAM_BUG_WORKAROUND)
	unsigned char status[5];
#endif
	unsigned char rid, plug_state_monitor;
	unsigned char int_status, usbpd_state;
	int cc1_valid, cc2_valid, plug_rprd_sel_monitor, plug_attach_done;
	int value;
	CC_NOTI_ATTACH_TYPEDEF attach_notifier;
	CC_NOTI_PD_STATUS_TYPEDEF pd_status_notifier;
	CC_NOTI_RID_TYPEDEF rid_notifier;

	dev_err(&i2c->dev, "%d times\n", ++wq_times);

#if defined(CONFIG_SAMSUNG_S2MM003_SRAM_BUG_WORKAROUND)
	/* Chipset initial value */
	rid = s2mm003_indirect_read(usbpd_data, 0x30);
	dev_info(&i2c->dev, "RID_GET_MONITOR I2C read: 0x%02X\n", rid);
	int_status = s2mm003_indirect_read(usbpd_data, 0x17);
	dev_info(&i2c->dev, "INT_STATUS I2C read: 0x%02X\n", int_status);

	if((int_status & b_RESET_START) != 0x00)
	{
		s2mm003_indirect_write(usbpd_data, IND_REG_HOST_CMD_ON, 0x01);
	}

	usbpd_state = s2mm003_indirect_read(usbpd_data, 0x16);
	dev_info(&i2c->dev, "USBPD_STATE I2C read: 0x%02d\n", usbpd_state);
	plug_state_monitor = s2mm003_indirect_read(usbpd_data, 0x15);
	cc1_valid = S2MM003_REG_MASK(plug_state_monitor, S_CC1_VALID);
	cc2_valid = S2MM003_REG_MASK(plug_state_monitor, S_CC2_VALID);
	plug_rprd_sel_monitor = S2MM003_REG_MASK(plug_state_monitor, PLUG_RPRD_SEL_MONITOR);
	plug_attach_done = S2MM003_REG_MASK(plug_state_monitor, PLUG_ATTACH_DONE);
	dev_info(&i2c->dev, "PLUG_STATE_MONITOR	I2C read:%x\n"
		 "CC1:%x CC2:%x rprd:%x attach:%x\n",
		 plug_state_monitor,
		 cc1_valid, cc2_valid, plug_rprd_sel_monitor, plug_attach_done);
#else
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS1, &status[0]);
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS2, &status[1]);
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS3, &status[2]);
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS4, &status[3]);
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS5, &status[4]);
	dev_err(&i2c->dev, "status : %02X %02X %02X %02X %02X\n",
	       status[0], status[1], status[2], status[3], status[4]);
	s2mm003_read_byte_16(i2c, RID_GET_MONITOR, &rid);
	dev_info(&i2c->dev, "RID_GET_MONITOR read: %x\n", rid);
	s2mm003_read_byte_16(i2c, PLUG_STATE_MONITOR, &plug_state_monitor);
	cc1_valid = S2MM003_REG_MASK(plug_state_monitor, S_CC1_VALID);
	cc2_valid = S2MM003_REG_MASK(plug_state_monitor, S_CC2_VALID);
	plug_rprd_sel_monitor = S2MM003_REG_MASK(plug_state_monitor, PLUG_RPRD_SEL_MONITOR);
	plug_attach_done = S2MM003_REG_MASK(plug_state_monitor, PLUG_ATTACH_DONE);
	dev_info(&i2c->dev, "PLUG_STATE_MONITOR	read:%x\n"
		 "CC1:%x CC2:%x rprd:%x attach:%x\n",
		 plug_state_monitor,
		 cc1_valid, cc2_valid, plug_rprd_sel_monitor, plug_attach_done);
#endif

	/* To confirm whether PD charger is attached or not */
	value = s2mm003_indirect_read(usbpd_data, IND_REG_INT_NUMBER);

	if( (value & b_PD_FUNC_FLAG) != 0x00) { // Need PD Function State Read
		int address = IND_REG_PD_FUNC_STATE; // Function State Number Read
		int ret = 0x00;
		ret = s2mm003_indirect_read(usbpd_data, address);

		/* If it isn't PD charger, return value is 29*/
		dev_info(&i2c->dev, " \n\r%s :Function_State = %d\n\r", __func__, ret);
	}

	if( (value & b_INT_3_FLAG) != 0x00)
	{
		value = s2mm003_indirect_read(usbpd_data, IND_REG_INT_STATUS_3); // Interrupt State 3 Read
		dev_info(&i2c->dev, "INT_State3 = 0x%X\n", value);

		if((value & b_RID_Detect_done) != 0x00)  // RID Message Detect
		{
			int ret;
			ret = s2mm003_indirect_read(usbpd_data, 0x30); // RID Value Read
			dev_info(&i2c->dev, "\n\rRID_Value = 0x%X\n\r", ret);

			if (plug_rprd_sel_monitor && plug_attach_done) /* for USB Host Connector */
				rid = RID_000K;

			cur_rid = rid;
			if (cur_rid == prev_rid) {
				dev_err(&i2c->dev, "same rid detected, -ignore-\n");
			}
			p_prev_rid = prev_rid;
			prev_rid = cur_rid;

#if defined(CONFIG_CCIC_NOTIFIER)
			rid_notifier.src = CCIC_NOTIFY_DEV_CCIC;
			rid_notifier.dest = CCIC_NOTIFY_DEV_MUIC;
			rid_notifier.id = CCIC_NOTIFY_ID_RID;
			rid_notifier.rid = rid;
			ccic_notifier_test((CC_NOTI_TYPEDEF*)&rid_notifier);
#endif
		}

		if((value & b_Msg_SRC_CAP) != 0x00)  // Read Source Capability MSG.
		{
			int choice = 1;
			int cnt;
			//int ret;
			uint32_t ReadMSG[8];
			uint8_t *p_MSG_BUF;

			s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_RX_SRC_CAPA);// Idx Receive SRC_CAP
			p_MSG_BUF = (uint8_t *)ReadMSG;
			for(cnt = 0; cnt < 32; cnt++)
			{
				*(p_MSG_BUF + cnt) = s2mm003_indirect_read(usbpd_data, IND_REG_MSG_ACCESS_BASE+cnt);
			}
			s2mm003_src_capacity_information(i2c, ReadMSG, &choice);
			dev_info(&i2c->dev, "choice : %d\n", choice);

			// Select PDO
			s2mm003_indirect_write(usbpd_data, IND_REG_READ_MSG_INDEX, MSG_Idx_TX_REQUEST);// Idx Transceiver Request
			p_MSG_BUF = (uint8_t *)ReadMSG;

			for(cnt = 0; cnt < MSG_BUF_REQUEST_SIZE; cnt++)
			{
				*(p_MSG_BUF + cnt) = s2mm003_indirect_read(usbpd_data, IND_REG_MSG_ACCESS_BASE+cnt);
			}

			for(cnt = 0; cnt < MSG_REQUEST_SIZE; cnt++)
			{
				dev_info(&i2c->dev, "TX_REQ_%d : 0x%X\n\r", cnt, ReadMSG[cnt]);
			}

			//  ReadMSG[1] = 0x2000C864;
			//dev_info(&i2c->dev, "Sel PDO Num = %d\n\r",REQ_PDO_NUM);
			s2mm003_request_select_type(&ReadMSG[1], choice);
			for(cnt = 0; cnt < MSG_BUF_REQUEST_SIZE; cnt++)
			{
				s2mm003_indirect_write(usbpd_data, IND_REG_MSG_ACCESS_BASE + cnt, *(p_MSG_BUF + cnt));// Idx Transceiver Request
			}

			p_MSG_BUF = (uint8_t *)ReadMSG;
			for(cnt = 0; cnt < MSG_BUF_REQUEST_SIZE; cnt++)
			{
				*(p_MSG_BUF + cnt) = s2mm003_indirect_read(usbpd_data, IND_REG_MSG_ACCESS_BASE+cnt);
			}

			for(cnt = 0; cnt < MSG_REQUEST_SIZE; cnt++)
			{
				dev_info(&i2c->dev, "TX_REQ_%d : 0x%X\n\r", cnt, ReadMSG[cnt]);
			}
			s2mm003_int_clear(usbpd_data);
#if defined(CONFIG_CCIC_NOTIFIER)
			pd_status_notifier.status = plug_attach_done;
			pd_status_notifier.id = CCIC_NOTIFY_ID_POWER_STATUS;
			pd_status_notifier.src = CCIC_NOTIFY_DEV_PDIC;
			pd_status_notifier.dest = CCIC_NOTIFY_DEV_BATTERY;
			ccic_notifier_test((CC_NOTI_TYPEDEF*)&pd_status_notifier);
#endif
		}
	} else {
		s2mm003_int_clear(usbpd_data);
	}

#if defined(CONFIG_CCIC_NOTIFIER)
	attach_notifier.attach = plug_attach_done;
	attach_notifier.id = CCIC_NOTIFY_ID_ATTACH;
	attach_notifier.src = CCIC_NOTIFY_DEV_CCIC;
	attach_notifier.dest = CCIC_NOTIFY_DEV_MUIC;
	attach_notifier.rprd = plug_rprd_sel_monitor;
	ccic_notifier_test((CC_NOTI_TYPEDEF*)&attach_notifier);
#endif

	return IRQ_HANDLED;
}

#if defined(CONFIG_SAMSUNG_S2MM003_SYSFS)
static ssize_t s2mm003_register_show(struct device *dev,
		struct device_attribute *att, char *buf)
{
	struct s2mm003_usbpd_data *usbpd_data = dev_get_drvdata(dev);
	struct i2c_client *i2c = usbpd_data->i2c;
	unsigned char status[5];
	unsigned char rid, plug_state_monitor;
	int irq_gpio_status[2];
	int cc1_valid, cc2_valid, plug_rprd_sel_monitor, plug_attach_done;

	irq_gpio_status[0] = gpio_get_value(usbpd_data->irq_gpio);
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS1, &status[0]);
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS2, &status[1]);
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS3, &status[2]);
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS4, &status[3]);
	s2mm003_read_byte_16(i2c, INTERRUPT_STATUS5, &status[4]);
	s2mm003_read_byte_16(i2c, RID_GET_MONITOR, &rid);

	s2mm003_read_byte_16(i2c, RID_GET_MONITOR, &rid);
	s2mm003_read_byte_16(i2c, PLUG_STATE_MONITOR, &plug_state_monitor);
	cc1_valid = S2MM003_REG_MASK(plug_state_monitor, S_CC1_VALID);
	cc2_valid = S2MM003_REG_MASK(plug_state_monitor, S_CC2_VALID);
	plug_rprd_sel_monitor = S2MM003_REG_MASK(plug_state_monitor, PLUG_RPRD_SEL_MONITOR);
	plug_attach_done = S2MM003_REG_MASK(plug_state_monitor, PLUG_ATTACH_DONE);

	s2mm003_int_clear(usbpd_data);
	irq_gpio_status[1] = gpio_get_value(usbpd_data->irq_gpio);

	return snprintf(buf, PAGE_SIZE,
		"\n\ninterrupt\n"
		"irq_gpio   \t\t: %02d %02d\n"
		"status(1~5)\t\t: %02X %02X %02X %02X %02X\n"
		"CC1:%x CC2:%x rprd:%x attach:%x\n"
		"rid : %02x \n",
		irq_gpio_status[0], irq_gpio_status[1],
		status[0], status[1], status[2], status[3], status[4],
		cc1_valid, cc2_valid, plug_rprd_sel_monitor, plug_attach_done,
		rid);
}
#endif

#if defined(CONFIG_OF)
static int of_s2mm003_usbpd_dt(struct i2c_client *i2c,
			       struct s2mm003_usbpd_data *usbpd_data)
{
	struct device *dev = &i2c->dev;
	struct device_node *np_usbpd = dev->of_node;

	if (np_usbpd == NULL) {
		dev_err(&i2c->dev, "np NULL\n");
		return -EINVAL;
	}
	usbpd_data->irq_gpio = of_get_named_gpio(np_usbpd, "usbpd,usbpd_int", 0);
	if (usbpd_data->irq_gpio < 0) {
		dev_err(&i2c->dev, "error reading usbpd_irq = %d\n", usbpd_data->irq_gpio);
		usbpd_data->irq_gpio = 0;
	}

	return 0;
}
#endif /* CONFIG_OF */

static int s2mm003_firmware_update(struct s2mm003_usbpd_data *usbpd_data)
{
	const struct firmware *fw_entry;
	const unsigned char *p;
	int ret, i;
	struct i2c_client *i2c = usbpd_data->i2c;

	ret = request_firmware(&fw_entry,
			       FIRMWARE_PATH, usbpd_data->dev);
	if (ret < 0)
		goto done;

	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x1);
	s2mm003_write_byte(i2c, IF_S_CODE_E, 0x4);
	for(p = fw_entry->data, i=0; i < fw_entry->size; p++, i++) {
		s2mm003_write_byte_16(i2c,(u16)i, *p);
	}

	s2mm003_write_byte(i2c, I2C_SRAM_SET, 0x0);
	s2mm003_write_byte(i2c, IF_S_CODE_E, 0x0);

	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x1);
	s2mm003_write_byte(i2c, USB_PD_RST, 0x80);
	s2mm003_write_byte(i2c, I2C_SYSREG_SET, 0x00);
	msleep(100);
done:
	dev_err(&usbpd_data->i2c->dev, "firmware size: %d, error %d\n",
		(int)fw_entry->size, ret);
	if (fw_entry)
		release_firmware(fw_entry);
	return ret;
}

static int s2mm003_usbpd_probe(struct i2c_client *i2c,
			       const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(i2c->dev.parent);
	struct s2mm003_usbpd_data *usbpd_data;
	int ret = 0;
	u8 val;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&i2c->dev, "i2c functionality check error\n");
		return -EIO;
	}
	usbpd_data = kzalloc(sizeof(struct s2mm003_usbpd_data), GFP_KERNEL);
	if (!usbpd_data) {
		dev_err(&i2c->dev, "Failed to allocate driver data\n");
		return -ENOMEM;
	}

	usbpd_data->i2c = i2c;

#if defined(CONFIG_OF)
	ret = of_s2mm003_usbpd_dt(i2c, usbpd_data);
	if (ret)
		dev_err(&i2c->dev, "not found muic dt! ret:%d\n", ret);
#endif
	ret = gpio_request(usbpd_data->irq_gpio, "s2mm003_irq");
	if (ret) {
		dev_err(&i2c->dev, "Failed to request GPIO pin %d, error %d\n",
			(int)usbpd_data->irq_gpio, ret);
		goto err_free_usbpd_data;
	}
	pr_err("%s:%04d", __func__, __LINE__);
	ret = gpio_direction_input(usbpd_data->irq_gpio);
	if (ret) {
		dev_err(&i2c->dev, "Unable to set input gpio direction, error %d\n", ret);
		goto err_free_gpio;
	}
	i2c->irq = gpio_to_irq(usbpd_data->irq_gpio);
	ret = enable_irq_wake(i2c->irq);
	if (ret) {
		dev_err(&i2c->dev, "Failed to enable wakeup source, error %d\n", ret);
		goto err_free_gpio;
	}

	s2mm003_firmware_update(usbpd_data);

	ret = request_threaded_irq(i2c->irq, NULL, s2mm003_usbpd_irq_thread,
		(IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND | IRQF_ONESHOT), "s2mm003-usbpd", usbpd_data);
	if (ret) {
		dev_err(&i2c->dev, "Failed to request IRQ %d, error %d\n", i2c->irq, ret);
		goto err_init_irq;
	}

	i2c_set_clientdata(i2c, usbpd_data);
	device_init_wakeup(&i2c->dev, 1);

	/* Check Data Opt 1 */
	val = s2mm003_indirect_read(usbpd_data, 0x2d);
	dev_err(&i2c->dev, "read 0x2d %02X\n", val);
	s2mm003_indirect_write(usbpd_data, 0x2d, 0x1); 
	val = s2mm003_indirect_read(usbpd_data, 0x2d);
	dev_err(&i2c->dev, "read 0x2d %02X\n", val);

	dev_err(&i2c->dev, "probed, irq %d\n", usbpd_data->irq_gpio);

#if defined(CONFIG_SAMSUNG_S2MM003_SYSFS)
	device_create_file(&i2c->dev, &dev_attr_s2mm003_register);
#endif

	return ret;

err_init_irq:
	if (i2c->irq)
		free_irq(i2c->irq, usbpd_data);
err_free_gpio:
	gpio_free(usbpd_data->irq_gpio);
err_free_usbpd_data:
	kfree(usbpd_data);
	return ret;
}

static int s2mm003_usbpd_remove(struct i2c_client *i2c)
{
	return 0;
}

static const struct i2c_device_id s2mm003_usbpd_id[] = {
	{ USBPD_DEV_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, s2mm003_usbpd_id);

#if defined(CONFIG_OF)
static struct of_device_id s2mm003_i2c_dt_ids[] = {
	{ .compatible = "sec-s2mm003,i2c" },
	{ }
};
#endif /* CONFIG_OF */

static struct i2c_driver s2mm003_usbpd_driver = {
	.driver		= {
		.name	= USBPD_DEV_NAME,
#if defined(CONFIG_OF)
		.of_match_table	= s2mm003_i2c_dt_ids,
#endif /* CONFIG_OF */
	},
	.probe		= s2mm003_usbpd_probe,
	//.remove		= __devexit_p(s2mm003_usbpd_remove),
	.remove		= s2mm003_usbpd_remove,
	.id_table	= s2mm003_usbpd_id,
};

static int __init s2mm003_usbpd_init(void)
{
	return i2c_add_driver(&s2mm003_usbpd_driver);
}
module_init(s2mm003_usbpd_init);

static void __exit s2mm003_usbpd_exit(void)
{

	i2c_del_driver(&s2mm003_usbpd_driver);
}
module_exit(s2mm003_usbpd_exit);

MODULE_DESCRIPTION("S2MM003 USB PD driver");
MODULE_LICENSE("GPL");
