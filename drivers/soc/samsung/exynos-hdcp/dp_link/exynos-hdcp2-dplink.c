/*
 * drivers/soc/samsung/exynos-hdcp/dp_link/exynos-hdcp2-dplink.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/smc.h>
#include <asm/cacheflush.h>
#include <linux/exynos_ion.h>
#include <linux/smc.h>
#include <linux/types.h>
#include <linux/delay.h>

#include "../exynos-hdcp2.h"
#include "../exynos-hdcp2-log.h"
#include "exynos-hdcp2-dplink.h"
#include "exynos-hdcp2-dplink-if.h"
#include "exynos-hdcp2-dplink-auth.h"

#define HDCP_AUTH_RETRY_COUNT	1
#define RECVID_WIAT_RETRY_COUNT	5

#if defined(CONFIG_HDCP2_EMULATION_MODE)
int dplink_emul_handler(int cmd)
{
	/* todo: support hdcp22 emulator */
	return 0;
}
#endif

/* current link data */
static struct hdcp_link_data *lk_data;

extern struct hdcp_session_list g_hdcp_session_list;
static char *hdcp_link_st_str[] = {
	"ST_INIT",
	"ST_H0_NO_RX_ATTACHED",
	"ST_H1_TX_LOW_VALUE_CONTENT",
	"ST_A0_DETERMINE_RX_HDCP_CAP",
	"ST_A1_EXCHANGE_MASTER_KEY",
	"ST_A2_LOCALITY_CHECK",
	"ST_A3_EXCHANGE_SESSION_KEY",
	"ST_A4_TEST_REPEATER",
	"ST_A5_AUTHENTICATED",
	"ST_A6_WAIT_RECEIVER_ID_LIST",
	"ST_A7_VERIFY_RECEIVER_ID_LIST",
	"ST_A8_SEND_RECEIVER_ID_LIST_ACK",
	"ST_A9_CONTENT_STREAM_MGT",
	"ST_END",
	NULL
};

int do_dplink_auth(struct hdcp_link_info *lk_handle)
{
	struct hdcp_session_node *ss_node;
	struct hdcp_link_node *lk_node;
	int ret = HDCP_SUCCESS;
	int rval = TX_AUTH_SUCCESS;
	uint32_t retry_recvid = 0;

	/* find Session node which contains the Link */
	ss_node = hdcp_session_list_find(lk_handle->ss_id, &g_hdcp_session_list);
	if (!ss_node)
		return HDCP_ERROR_INVALID_INPUT;

	lk_node = hdcp_link_list_find(lk_handle->lk_id, &ss_node->ss_data->ln);
	if (!lk_node)
		return HDCP_ERROR_INVALID_INPUT;

	lk_data = lk_node->lk_data;
	if (!lk_data)
		return HDCP_ERROR_INVALID_INPUT;

	if (lk_data->state != LINK_ST_H1_TX_LOW_VALUE_CONTENT)
		return HDCP_ERROR_INVALID_STATE;

	/**
	 * if Upstream Content Control Function call this API,
	 * it changes state to ST_A0_DETERMINE_RX_HDCP_CAP automatically.
	 * HDCP library do not check CP desire.
	 */
	UPDATE_LINK_STATE(lk_data, LINK_ST_A0_DETERMINE_RX_HDCP_CAP);

	do {
		switch (lk_data->state) {
		case LINK_ST_H1_TX_LOW_VALUE_CONTENT:
			hdcp_dplink_config(DP_HDCP22_DISABLE);
			return ret;
		case LINK_ST_A0_DETERMINE_RX_HDCP_CAP:
			if (dplink_determine_rx_hdcp_cap(lk_data) < 0) {
				ret = HDCP_ERROR_RX_NOT_HDCP_CAPABLE;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			} else
				UPDATE_LINK_STATE(lk_data, LINK_ST_A1_EXCHANGE_MASTER_KEY);
			break;
		case LINK_ST_A1_EXCHANGE_MASTER_KEY:
			rval = dplink_exchange_master_key(lk_data);
			if (rval == TX_AUTH_SUCCESS) {
				UPDATE_LINK_STATE(lk_data, LINK_ST_A2_LOCALITY_CHECK);
			} else {
				/* todo: consider connection retry */
				ret = HDCP_ERROR_EXCHANGE_KM;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			}
			break;
		case LINK_ST_A2_LOCALITY_CHECK:
			rval = dplink_locality_check(lk_data);
			if (rval == TX_AUTH_SUCCESS) {
				UPDATE_LINK_STATE(lk_data, LINK_ST_A3_EXCHANGE_SESSION_KEY);
			} else {
				/* todo: consider connection retry */
				ret = HDCP_ERROR_LOCALITY_CHECK;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			}
			break;
		case LINK_ST_A3_EXCHANGE_SESSION_KEY:
			if (dplink_exchange_session_key(lk_data) < 0) {
				ret = HDCP_ERROR_EXCHANGE_KS;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			} else {
				UPDATE_LINK_STATE(lk_data, LINK_ST_A4_TEST_REPEATER);
			}
			break;
		case LINK_ST_A4_TEST_REPEATER:
			if (dplink_evaluate_repeater(lk_data) == TRUE) {
				/* if it is a repeater, verify Rcv ID list */
				UPDATE_LINK_STATE(lk_data, LINK_ST_A6_WAIT_RECEIVER_ID_LIST);
			} else {
				/* if it is not a repeater, complete authentication */
				UPDATE_LINK_STATE(lk_data, LINK_ST_A5_AUTHENTICATED);
			}
			break;
		case LINK_ST_A5_AUTHENTICATED:
			/* HDCP2.2 spec defined 200ms */
			msleep(200);
			hdcp_dplink_config(DP_HDCP22_ENABLE);
			hdcp_info("enable encryption\n");
			/* Transmitter has completed the authentication protocol */
			return HDCP_SUCCESS;
		case LINK_ST_A6_WAIT_RECEIVER_ID_LIST:
			rval = dplink_wait_for_receiver_id_list(lk_data);
			if (rval == -TX_AUTH_ERROR_TIME_EXCEED) {
				if (retry_recvid < RECVID_WIAT_RETRY_COUNT) {
					/* retry hdcp authentication in case of timeout */
					hdcp_dplink_config(DP_HDCP22_DISABLE);
					UPDATE_LINK_STATE(lk_data, LINK_ST_A0_DETERMINE_RX_HDCP_CAP);
					hdcp_info("retry authentication. (%d)\n", retry_recvid);
					retry_recvid++;
				} else {
					hdcp_info("exceed retry count. (%d)\n", retry_recvid);
					ret = HDCP_ERROR_VERIFY_RECEIVER_ID_LIST;
					UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
				}
			} else if (rval == HDCP_SUCCESS) {
				UPDATE_LINK_STATE(lk_data, LINK_ST_A7_VERIFY_RECEIVER_ID_LIST);
				retry_recvid = 0;
			} else {
				/* error on recevier id list wait */
				ret = HDCP_ERROR_VERIFY_RECEIVER_ID_LIST;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			}
			break;
		case LINK_ST_A7_VERIFY_RECEIVER_ID_LIST:
			if (dplink_verify_receiver_id_list(lk_data) < 0) {
				ret = HDCP_ERROR_VERIFY_RECEIVER_ID_LIST;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			} else {
				UPDATE_LINK_STATE(lk_data, LINK_ST_A8_SEND_RECEIVER_ID_LIST_ACK);
			}
			break;
		case LINK_ST_A8_SEND_RECEIVER_ID_LIST_ACK:
			rval = dplink_send_receiver_id_list_ack(lk_data);
			if (rval < 0) {
				ret = HDCP_ERROR_VERIFY_RECEIVER_ID_LIST;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			} else {
				UPDATE_LINK_STATE(lk_data, LINK_ST_A9_CONTENT_STREAM_MGT);
			}
			break;
		case LINK_ST_A9_CONTENT_STREAM_MGT:
			rval = dplink_stream_manage(lk_data);
			if (rval < 0) {
				ret = HDCP_ERROR_STREAM_MANAGE;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			} else {
				UPDATE_LINK_STATE(lk_data, LINK_ST_A5_AUTHENTICATED);
			}
			break;
		default:
			ret = HDCP_ERROR_INVALID_STATE;
			UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			break;
		}
	} while (1);
}

/* todo: support multi link & multi session */
int hdcp_dplink_authenticate(void)
{
	struct hdcp_sess_info ss_info;
	struct hdcp_link_info lk_info;
	int ret;
	int i;

	if (hdcp_session_open(&ss_info))
		return -ENOMEM;

	lk_info.ss_id = ss_info.ss_id;
	if (hdcp_link_open(&lk_info, HDCP_LINK_TYPE_DP)) {
		hdcp_session_close(&ss_info);
		return -ENOMEM;
	}

	dplink_clear_irqflag_all();

	/* if hdcp is enabled, disable it while hdcp authentication */
	if (hdcp_dplink_is_enabled_hdcp22())
		hdcp_dplink_config(DP_HDCP22_DISABLE);

	for (i = 0; i < HDCP_AUTH_RETRY_COUNT; i++) {
		ret = do_dplink_auth(&lk_info);
		if (ret == HDCP_SUCCESS) {
			hdcp_info("Success HDCP enabled!\n");
			hdcp_link_close(&lk_info);
			hdcp_session_close(&ss_info);
			return 0;
		} else {
			/* retry */
			hdcp_err("HDCP auth failed. retry(%d)!\n", i);
			continue;
		}
	}

	hdcp_link_close(&lk_info);
	hdcp_session_close(&ss_info);
	return -EACCES;
}

int hdcp_dplink_stream_manage(void)
{
	hdcp_info("stream manage updated!!\n");
	/* todo: update stream manage information */
	return 0;
}

int hdcp_dplink_get_rxstatus(uint8_t *status)
{
	hdcp_dplink_recv(HDCP22_MSG_RXSTATUS_R, status, sizeof(uint8_t));
	hdcp_debug("RxStatus: %x\n", *status);

	return 0;
}

int hdcp_dplink_set_paring_available(void)
{
	hdcp_info("pairing avaible\n");
	return dplink_set_paring_available();
}

int hdcp_dplink_set_hprime_available(void)
{
	hdcp_info("h-prime avaible\n");
	return dplink_set_hprime_available();
}

int hdcp_dplink_set_rp_ready(void)
{
	hdcp_info("ready avaible\n");
	return dplink_set_rp_ready();
}

int hdcp_dplink_set_reauth(void)
{
	hdcp_info("reauth requested\n");
	return dplink_set_reauth_req();
}

int hdcp_dplink_set_integrity_fail(void)
{
	hdcp_info("integrity check fail\n");
	return dplink_set_integrity_fail();
}

int hdcp_dplink_is_auth_state(void)
{
	return 0;
	/* todo: check link auth status */
	#if 0
	if (lk_data->state == LINK_ST_A5_AUTHENTICATED)
		return 1;
	else
		return 0;
	#endif
}
