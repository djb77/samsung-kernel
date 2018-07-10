/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung SoC DisplayPort HDCP1.3 driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "displayport.h"
#include "decon.h"

HDCP13 HDCP13_DPCD;
struct hdcp13_info hdcp13_info;

void HDCP13_DPCD_BUFFER(void)
{
	u8 i = 0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_BKSV); i++)
		HDCP13_DPCD.HDCP13_BKSV[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_R0); i++)
		HDCP13_DPCD.HDCP13_R0[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_AKSV); i++)
		HDCP13_DPCD.HDCP13_AKSV[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_AN); i++)
		HDCP13_DPCD.HDCP13_AN[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_V_H0); i++)
		HDCP13_DPCD.HDCP13_V_H0[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_V_H1); i++)
		HDCP13_DPCD.HDCP13_V_H1[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_V_H2); i++)
		HDCP13_DPCD.HDCP13_V_H2[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_V_H3); i++)
		HDCP13_DPCD.HDCP13_V_H3[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_V_H4); i++)
		HDCP13_DPCD.HDCP13_V_H4[i] = 0x0;

	HDCP13_DPCD.HDCP13_BCAP[0] = 0x0;
	HDCP13_DPCD.HDCP13_BSTATUS[0] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_BINFO); i++)
		HDCP13_DPCD.HDCP13_BINFO[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_KSV_FIFO); i++)
		HDCP13_DPCD.HDCP13_KSV_FIFO[i] = 0x0;

	HDCP13_DPCD.HDCP13_AINFO[0] = 0x0;
}

void HDCP13_dump(char *str, u8 *buf, int size)
{
	int i;
	u8 *buffer = buf;

	displayport_dbg("[HDCP 1.3] %s = 0x", str);
	if (displayport_log_level >=7) {
		for (i = 0; i < size; i++)
			pr_info("%02x ", *(buffer+i));

		pr_info("\n");
	}
}

void HDCP13_AUTH_Select(void)
{
	displayport_write_mask(HDCP_Control_Register_0, hdcp13_info.is_repeater, HW_AUTH_EN);

	if (hdcp13_info.is_repeater)
		displayport_dbg("[HDCP 1.3] HW Authentication Select\n");
	else
		displayport_dbg("[HDCP 1.3] SW Authentication Select\n");
}

void HDCP13_Func_En(u32 en)
{
	u32 val = en ? 0 : ~0; /* 0 is enable */

	displayport_write_mask(Function_En_1, val, HDCP_FUNC_EN_N);
}

u8 HDCP13_Read_Bcap(void)
{
	u8 return_val = 0;
	u8 hdcp_capa = 0;

	displayport_reg_dpcd_read(ADDR_HDCP13_BCAP, 1, HDCP13_DPCD.HDCP13_BCAP);

	displayport_info("[HDCP 1.3] HDCP13_BCAP= 0x%x\n", HDCP13_DPCD.HDCP13_BCAP[0]);

	if (!(HDCP13_DPCD.HDCP13_BCAP[0] & BCAPS_RESERVED_BIT_MASK)) {
		hdcp13_info.is_repeater = (HDCP13_DPCD.HDCP13_BCAP[0] & BCAPS_REPEATER) >> 1;

		hdcp_capa = HDCP13_DPCD.HDCP13_BCAP[0] & BCAPS_HDCP_CAPABLE;

		if (hdcp_capa)
			return_val = 0;
		else
			return_val = -EINVAL;
	} else
		return_val = -EINVAL;

	return return_val;
}

void HDCP13_Repeater_Set(void)
{
	displayport_write_mask(HDCP_Control_Register_0, hdcp13_info.is_repeater, SW_RX_REPEATER);
}

u8 HDCP13_Read_Bksv(void)
{
	u8 i = 0;
	u8 j = 0;
	u8 offset = 0;
	int one = 0;
	u8 ret;

	displayport_reg_dpcd_read_burst(ADDR_HDCP13_BKSV, sizeof(HDCP13_DPCD.HDCP13_BKSV), HDCP13_DPCD.HDCP13_BKSV);

	HDCP13_dump("BKSV", &(HDCP13_DPCD.HDCP13_BKSV[0]), 5);

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_BKSV); i++) {
		for (j = 0; j < 8; j++) {
			if (HDCP13_DPCD.HDCP13_BKSV[i] & (0x1 << j))
				one++;
		}
	}

	if (one == 20) {
		for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_BKSV); i++) {
			displayport_write(HDCP_BKSV_Register_0 + offset, (u32)HDCP13_DPCD.HDCP13_BKSV[i]);
			offset += 4;
		}

		displayport_dbg("[HDCP 1.3] Valid Bksv\n");
		ret = 0;
	} else {
		displayport_info("[HDCP 1.3] Invalid Bksv\n");
		ret = -EINVAL;
	}

	return ret;
}

void HDCP13_Set_An_val(void)
{
	displayport_write_mask(HDCP_Control_Register_0, 1, SW_STORE_AN);

	displayport_write_mask(HDCP_Control_Register_0, 0, SW_STORE_AN);
}

void HDCP13_Write_An_val(void)
{
	u8 i = 0;
	u8 offset = 0;

	HDCP13_Set_An_val();

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_AN); i++) {
		HDCP13_DPCD.HDCP13_AN[i] = (u8)displayport_read(HDCP_AN_Register_0 + offset);
		offset += 4;
	}

	displayport_reg_dpcd_write_burst(ADDR_HDCP13_AN, 8, HDCP13_DPCD.HDCP13_AN);
}

u8 HDCP13_Write_Aksv(void)
{
	u8 i = 0;
	u8 offset = 0;
	u8 ret;

	if (displayport_read_mask(HDCP_Status_Register, AKSV_VALID)) {
		for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_AKSV); i++) {
			HDCP13_DPCD.HDCP13_AKSV[i] = (u8)displayport_read(HDCP_AKSV_Register_0 + offset);
			offset += 4;
		}

		hdcp13_info.cp_irq_flag = 0;
		displayport_reg_dpcd_write_burst(ADDR_HDCP13_AKSV, 5, HDCP13_DPCD.HDCP13_AKSV);
		displayport_dbg("[HDCP 1.3] Valid Aksv\n");

		ret = 0;
	} else {
		displayport_info("[HDCP 1.3] Invalid Aksv\n");
		ret = -EINVAL;
	}

	return ret;
}

u8 HDCP13_CMP_Ri(void)
{
	u8 cnt = 0;
	u8 ri_retry_cnt = 0;
	u8 ri[2];
	u8 ret = 0;

	cnt = 0;
	while (hdcp13_info.cp_irq_flag != 1 && cnt < RI_WAIT_COUNT) {
		mdelay(RI_AVAILABLE_WAITING);
		cnt++;
	}

	if (cnt >= RI_WAIT_COUNT) {
		displayport_info("[HDCP 1.3] Don't receive CP_IRQ interrupt\n");
		ret = -EFAULT;
	}

	hdcp13_info.cp_irq_flag = 0;

	cnt = 0;
	while ((HDCP13_DPCD.HDCP13_BSTATUS[0] & BSTATUS_R0_AVAILABLE) == 0 && cnt < RI_WAIT_COUNT) {
		/* R0 Sink Available check */
		displayport_reg_dpcd_read(ADDR_HDCP13_BSTATUS, 1, HDCP13_DPCD.HDCP13_BSTATUS);

		mdelay(RI_AVAILABLE_WAITING);
		cnt++;
	}

	if (cnt >= RI_WAIT_COUNT) {
		displayport_info("[HDCP 1.3] R0 not available in RX part\n");
		ret = -EFAULT;
	}

	while (ri_retry_cnt < RI_READ_RETRY_CNT) {
		/* Read R0 from Sink */
		displayport_reg_dpcd_read_burst(ADDR_HDCP13_R0, sizeof(HDCP13_DPCD.HDCP13_R0), HDCP13_DPCD.HDCP13_R0);

		/* Read R0 from Source */
		ri[0] = (u8)displayport_read(HDCP_R0_Register_0);
		ri[1] = (u8)displayport_read(HDCP_R0_Register_1);

		ri_retry_cnt++;

		if ((ri[0] == HDCP13_DPCD.HDCP13_R0[0]) && (ri[1] == HDCP13_DPCD.HDCP13_R0[1])) {
			displayport_dbg("[HDCP 1.3] Ri_Tx(0x%02x%02x) == Ri_Rx(0x%02x%02x)\n",
					ri[1], ri[0], HDCP13_DPCD.HDCP13_R0[1], HDCP13_DPCD.HDCP13_R0[0]);

			ret = 0;
			break;
		}

		displayport_info("[HDCP 1.3] Ri_Tx(0x%02x%02x) != Ri_Rx(0x%02x%02x)\n",
				ri[1], ri[0], HDCP13_DPCD.HDCP13_R0[1], HDCP13_DPCD.HDCP13_R0[0]);
		mdelay(RI_DELAY);
		ret = -EFAULT;
	}

	return ret;
}

void HDCP13_Encryption_con(u8 enable)
{
	if (enable == 1) {
		displayport_write_mask(HDCP_Control_Register_0, ~0, SW_AUTH_OK | HDCP_ENC_EN);
		/*displayport_reg_video_mute(0);*/
		displayport_info("[HDCP 1.3] HDCP13 Encryption Enable\n");
	} else {
		/*displayport_reg_video_mute(1);*/
		displayport_write_mask(HDCP_Control_Register_0, 0, SW_AUTH_OK | HDCP_ENC_EN);
		displayport_info("[HDCP 1.3] HDCP13 Encryption Disable\n");
	}
}

void HDCP13_HW_ReAuth(void)
{
	displayport_write_mask(HDCP_Control_Register_0, 1, HW_RE_AUTHEN);

	displayport_write_mask(HDCP_Control_Register_0, 0, HW_RE_AUTHEN);
}

void HDCP13_Link_integrity_check(void)
{
	int i;
	if (hdcp13_info.link_check == LINK_CHECK_NEED) {
		displayport_info("[HDCP 1.3] HDCP13_Link_integrity_check\n");

		for (i = 0; i < 10; i++) {
			displayport_reg_dpcd_read(ADDR_HDCP13_BSTATUS, 1,
					HDCP13_DPCD.HDCP13_BSTATUS);
			if ((HDCP13_DPCD.HDCP13_BSTATUS[0] & BSTATUS_REAUTH_REQ) ||
					(HDCP13_DPCD.HDCP13_BSTATUS[0] & BSTATUS_LINK_INTEGRITY_FAIL)) {

				displayport_info("[HDCP 1.3] HDCP13_DPCD.HDCP13_BSTATUS = %02x : retry(%d)\n",
						HDCP13_DPCD.HDCP13_BSTATUS[0], i);
				hdcp13_info.link_check = LINK_CHECK_FAIL;
				HDCP13_DPCD_BUFFER();
				hdcp13_info.auth_state = HDCP13_STATE_FAIL;
				displayport_reg_video_mute(1);
				hdcp13_info.cp_irq_flag = 0;

				if (HDCP13_Read_Bcap() != 0) {
					displayport_info("[HDCP 1.3] NOT HDCP CAPABLE\n");
					HDCP13_Encryption_con(0);
				} else {
					displayport_info("[HDCP 1.3] ReAuth\n");
					HDCP13_run();
				}
				break;
			}
			msleep(20);
		}
	}
}

void HDCP13_HW_Revocation_set(u8 param)
{
	displayport_write_mask(HDCP_Debug_Control_Register, param, REVOCATION_CHK_DONE);
}

void HDCP13_HW_AUTH_set(u8 Auth_type)
{
	if (Auth_type == FIRST_AUTH)
		displayport_write_mask(HDCP_Control_Register_0, 1, HW_1ST_PART_ATHENTICATION_EN);
	else
		displayport_write_mask(HDCP_Control_Register_0, 1, HW_2ND_PART_ATHENTICATION_EN);
}

u8 HDCP13_HW_KSV_read(void)
{
	u32 KSV_bytes = 0;
	u32 read_count = 0;
	u32 i = 0;
	u8 ret = 0;

	displayport_reg_dpcd_read_burst(ADDR_HDCP13_BINFO,
		sizeof(HDCP13_DPCD.HDCP13_BINFO), HDCP13_DPCD.HDCP13_BINFO);

	hdcp13_info.device_cnt = HDCP13_DPCD.HDCP13_BINFO[1] & 0x7F;

	KSV_bytes = hdcp13_info.device_cnt;

	displayport_dbg("[HDCP 1.3] Total KSV bytes = %d", KSV_bytes);

	read_count = (hdcp13_info.device_cnt - 1) / 3 + 1;

	for (i = 0; i < read_count; i++) {
		if (displayport_reg_dpcd_read_burst(ADDR_HDCP13_KSV_FIFO,
					sizeof(HDCP13_DPCD.HDCP13_KSV_FIFO),
					HDCP13_DPCD.HDCP13_KSV_FIFO) != 0) {
			ret =  -EFAULT;
			break;
		}

		HDCP13_dump("KSV List read", HDCP13_DPCD.HDCP13_KSV_FIFO,
				sizeof(HDCP13_DPCD.HDCP13_KSV_FIFO));
	}

	return ret;
}

u8 HDCP13_HW_KSV_check(void)
{
	if (displayport_read_mask(HDCP_Debug_Control_Register, CHECK_KSV))
		return 0;
	else
		return -EFAULT;
}

u8 HDCP13_HW_1st_Auth_check(void)
{
	if (displayport_read_mask(HDCP_Status_Register, HW_1ST_AUTHEN_PASS))
		return 0;
	else
		return -EFAULT;
}

u8 HDCP13_HW_Auth_pass_check(void)
{
	if (displayport_read_mask(HDCP_Status_Register, HW_AUTHEN_PASS)) {
		displayport_info("[HDCP 1.3] HDCP13 HW Authectication PASS in no revocation mode\n");
		return 0;
	} else
		return -EFAULT;
}

u8 HDCP13_HW_Auth_check(void)
{
	if (displayport_read_mask(HDCP_Status_Register, AUTH_FAIL)) {
		displayport_info("[HDCP 1.3] HDCP13 HW Authectication FAIL\n");
		return 0;
	} else
		return -EFAULT;
}

void HDCP3_IRQ_Mask(void)
{
	displayport_reg_set_interrupt_mask(HDCP_LINK_CHECK_INT_MASK, 1);
	displayport_reg_set_interrupt_mask(HDCP_LINK_FAIL_INT_MASK, 1);
}

void HDCP13_make_sha1_input_buf(u8 *sha1_input_buf, u8 *binfo, u8 device_cnt)
{
	int i = 0;

	for (i = 0; i < BINFO_SIZE; i++)
		sha1_input_buf[KSV_SIZE * device_cnt + i] = binfo[i];

	for (i = 0; i < M0_SIZE; i++)
		sha1_input_buf[KSV_SIZE * device_cnt + BINFO_SIZE + i] =
			displayport_read_mask(HDCP_AM0_Register_0 + i * 4, HDCP_AM0_0);
}

void HDCP13_v_value_order_swap(u8 *v_value)
{
	int i;
	u8 temp;

	for (i = 0; i < SHA1_SIZE; i += 4) {
		temp = v_value[i];
		v_value[i] = v_value[i + 3];
		v_value[i + 3] = temp;
		temp = v_value[i + 1];
		v_value[i + 1] = v_value[i + 2];
		v_value[i + 2] = temp;
	}
}

int HDCP13_compare_v(u8 *tx_v_value)
{
	int i = 0;
	int ret = 0;
	u8 v_read_retry_cnt = 0;
	u8 *tx_v_value_address;
	u8 *rx_v_value_address;

	while (v_read_retry_cnt < V_READ_RETRY_CNT) {
		ret = 0;
		tx_v_value_address = tx_v_value;
		rx_v_value_address = HDCP13_DPCD.HDCP13_V_H0;

		displayport_reg_dpcd_read(ADDR_HDCP13_V_H0, 4, HDCP13_DPCD.HDCP13_V_H0);
		displayport_reg_dpcd_read(ADDR_HDCP13_V_H1, 4, HDCP13_DPCD.HDCP13_V_H1);
		displayport_reg_dpcd_read(ADDR_HDCP13_V_H2, 4, HDCP13_DPCD.HDCP13_V_H2);
		displayport_reg_dpcd_read(ADDR_HDCP13_V_H3, 4, HDCP13_DPCD.HDCP13_V_H3);
		displayport_reg_dpcd_read(ADDR_HDCP13_V_H4, 4, HDCP13_DPCD.HDCP13_V_H4);

		v_read_retry_cnt++;

		for (i = 0; i < SHA1_SIZE; i++) {
			if (*(tx_v_value_address++) != *(rx_v_value_address++))
				ret = -EFAULT;
		}

		if (ret == 0)
			break;
	}

	return ret;
}

static int HDCP13_proceed_repeater(void)
{
	int retry_cnt = HDCP_RETRY_COUNT;
	int cnt = 0;
	int i;
	u32 b_info = 0;
	u8 device_cnt = 0;
	u8 offset = 0;
	int ksv_read_size = 0;
	u8 sha1_input_buf[KSV_SIZE * MAX_KSV_LIST_COUNT + BINFO_SIZE + M0_SIZE];
	u8 v_value[SHA1_SIZE];

	displayport_info("[HDCP 1.3] HDCP repeater Start!!!\n");

	while (hdcp13_info.cp_irq_flag != 1 && cnt < RI_WAIT_COUNT) {
		mdelay(RI_AVAILABLE_WAITING);
		cnt++;
	}

	if (cnt >= RI_WAIT_COUNT)
		displayport_dbg("[HDCP 1.3] Don't receive CP_IRQ interrupt\n");

	hdcp13_info.cp_irq_flag = 0;

	cnt = 0;
	while ((HDCP13_DPCD.HDCP13_BSTATUS[0] & BSTATUS_READY) == 0) {
		displayport_reg_dpcd_read(ADDR_HDCP13_BSTATUS, 1,
				HDCP13_DPCD.HDCP13_BSTATUS);

		mdelay(RI_AVAILABLE_WAITING);
		cnt++;

		if (cnt > REPEATER_READY_WAIT_COUNT || !displayport_get_hpd_state()) {
			displayport_info("[HDCP 1.3] Not repeater ready in RX part\n");
			hdcp13_info.auth_state = HDCP13_STATE_FAIL;
			goto repeater_err;
		}
	}

	displayport_dbg("[HDCP 1.3] HDCP RX repeater ready!!!\n");

	while ((hdcp13_info.auth_state != HDCP13_STATE_SECOND_AUTH_DONE) &&
			(retry_cnt != 0)) {
		retry_cnt--;

		displayport_reg_dpcd_read(ADDR_HDCP13_BINFO, 2, HDCP13_DPCD.HDCP13_BINFO);

		for (i = 0; i < 2; i++)
			b_info |= (u32)HDCP13_DPCD.HDCP13_BINFO[i] << (i * 8);

		displayport_dbg("[HDCP 1.3] b_info = 0x%x\n", b_info);

		if ((b_info & BINFO_MAX_DEVS_EXCEEDED)
				|| (b_info & BINFO_MAX_CASCADE_EXCEEDED)) {
			hdcp13_info.auth_state = HDCP13_STATE_FAIL;
			displayport_info("[HDCP 1.3] MAXDEVS or CASCADE EXCEEDED!\n");
			goto repeater_err;
		}

		device_cnt = b_info & BINFO_DEVICE_COUNT;

		if (device_cnt != 0) {
			displayport_info("[HDCP 1.3] device count = %d", device_cnt);

			offset = 0;

			while (device_cnt > offset) {
				ksv_read_size = (device_cnt - offset) * KSV_SIZE;

				if (ksv_read_size >= KSV_FIFO_SIZE)
					ksv_read_size = KSV_FIFO_SIZE;

				displayport_reg_dpcd_read(ADDR_HDCP13_KSV_FIFO,
						ksv_read_size, HDCP13_DPCD.HDCP13_KSV_FIFO);

				for (i = 0; i < ksv_read_size; i++)
					sha1_input_buf[i + offset * KSV_SIZE] =
						HDCP13_DPCD.HDCP13_KSV_FIFO[i];

				offset += KSV_FIFO_SIZE / KSV_SIZE;
			}
		}

		/* need calculation of V = SHA-1(KSV list || Binfo || M0) */
		HDCP13_make_sha1_input_buf(sha1_input_buf, HDCP13_DPCD.HDCP13_BINFO, device_cnt);
		hdcp_calc_sha1(v_value, sha1_input_buf, BINFO_SIZE + M0_SIZE + KSV_SIZE * device_cnt);
		HDCP13_v_value_order_swap(v_value);

		if (HDCP13_compare_v(v_value) == 0) {
			hdcp13_info.auth_state = HDCP13_STATE_SECOND_AUTH_DONE;
			displayport_reg_video_mute(0);
			displayport_info("[HDCP 1.3] 2nd Auth done!!!\n");
			return 0;
		}

		hdcp13_info.auth_state = HDCP13_STATE_AUTH_PROCESS;
		displayport_info("[HDCP 1.3] 2nd Auth fail!!!\n");
	}

repeater_err:
	return -EINVAL;
}

void HDCP13_run(void)
{
	int retry_cnt = HDCP_RETRY_COUNT;
	u8 i = 0;
	u8 offset = 0;
	struct decon_device *decon = get_decon_drvdata(2);

	while ((hdcp13_info.auth_state != HDCP13_STATE_AUTHENTICATED)
			&& (hdcp13_info.auth_state != HDCP13_STATE_SECOND_AUTH_DONE)
			&& (retry_cnt != 0)) {
		retry_cnt--;

		hdcp13_info.auth_state = HDCP13_STATE_AUTH_PROCESS;

		HDCP13_Encryption_con(0);
		HDCP13_Func_En(1);

		HDCP13_Repeater_Set();
		displayport_write_mask(HDCP_Control_Register_0, 0, HW_AUTH_EN);

		displayport_dbg("[HDCP 1.3] SW Auth.\n");
		/*displayport_reg_set_interrupt_mask(HDCP_R0_READY_INT_MASK, 1);*/

		if (HDCP13_Read_Bksv() != 0) {
			displayport_info("[HDCP 1.3] ReAuthentication Start!!!\n");
			continue;
		}

		HDCP13_Write_An_val();

		if (HDCP13_Write_Aksv() != 0) {
			displayport_info("[HDCP 1.3] ReAuthentication Start!!!\n");
			continue;
		}

		/* BKSV Rewrite */
		for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_BKSV); i++) {
			displayport_write(HDCP_BKSV_Register_0 + offset, (u32)HDCP13_DPCD.HDCP13_BKSV[i]);
			offset += 4;
		}

		if (HDCP13_CMP_Ri() != 0)
			continue;

		if (!hdcp13_info.is_repeater) {
			hdcp13_info.auth_state = HDCP13_STATE_AUTHENTICATED;
			displayport_reg_video_mute(0);
		}

		/* wait 2 frames for hdcp 1.3 encryption enable */
		decon_wait_for_vsync(decon, VSYNC_TIMEOUT_MSEC);
		decon_wait_for_vsync(decon, VSYNC_TIMEOUT_MSEC);

		HDCP13_Encryption_con(1);
		displayport_dbg("[HDCP 1.3] HDCP 1st Authentication done!!!\n");

		if (hdcp13_info.is_repeater) {
			if (HDCP13_proceed_repeater())
				goto HDCP13_END;
			else
				continue;
		}
	}

HDCP13_END:
	if ((hdcp13_info.auth_state != HDCP13_STATE_AUTHENTICATED) &&
			(hdcp13_info.auth_state != HDCP13_STATE_SECOND_AUTH_DONE)) {
		hdcp13_info.auth_state = HDCP13_STATE_FAIL;
		displayport_reg_video_mute(1);
		HDCP13_Encryption_con(0);
		displayport_dbg("[HDCP 1.3] HDCP Authentication fail!!!\n");
#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
		secdp_bigdata_inc_error_cnt(ERR_HDCP_AUTH);
#endif
	}

	/*HDCP3_IRQ_Mask();*/
}
