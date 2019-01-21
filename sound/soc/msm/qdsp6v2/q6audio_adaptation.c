/* Copyright (c) 2012-2016, The Linux Foundation. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/delay.h>
#include <sound/apr_audio-v2.h>
#include <sound/q6audio-v2.h>
#include <sound/adsp_err.h>
#include <sound/audio_cal_utils.h>
#include <linux/qdsp6v2/apr_tal.h>

#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>

#include <sound/sec_adaptation.h>
#include "msm-pcm-routing-v2.h"
#include "q6voice.h"

#define TIMEOUT_MS	1000
#define TRUE        0x01
#define FALSE       0x00

#define SEC_ADAPTATAION_DSM_SRC_PORT		5	/* AFE */
#define SEC_ADAPTATAION_DSM_DEST_PORT		0	/* AFE */
#define SEC_ADAPTATAION_AUDIO_PORT			3	/* ASM */
#define SEC_ADAPTATAION_LOOPBACK_SRC_PORT	2	/* CVS */
#define SEC_ADAPTATAION_VOICE_SRC_PORT		2	/* CVP */

#define ASM_SET_BIT(n, x)	(n |= 1 << x)
#define ASM_TEST_BIT(n, x)	((n >> x) & 1)

enum {
	ASM_DIRECTION_OFFSET,
	ASM_CMD_NO_WAIT_OFFSET,
	ASM_MAX_OFFSET = 7,
};

enum {
	WAIT_CMD,
	NO_WAIT_CMD
};

enum {
	LOOPBACK_DISABLE = 0,
	LOOPBACK_ENABLE,
	LOOPBACK_NODELAY,
	LOOPBACK_MAX,
};

struct afe_ctl {
	void *apr;
	atomic_t state;
	atomic_t status;
	wait_queue_head_t wait;
#ifdef CONFIG_SND_SOC_MAXIM_DSM
	struct afe_dsm_spkr_prot_response_data calib_data;
	struct afe_dsm_spkr_prot_response_log_data calib_log_data;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
#if defined(CONFIG_SND_SOC_TFA9872)
	struct rtac_cal_block_data tfa_cal;
	atomic_t tfa_state;
#endif
};

struct cvs_ctl {
	void *apr;
	atomic_t state;
	wait_queue_head_t wait;
};

struct cvp_ctl {
	void *apr;
	atomic_t state;
	wait_queue_head_t wait;
};

union asm_token_struct {
	struct {
		u8 stream_id;
		u8 session_id;
		u8 buf_index;
		u8 flags;
	} _token;
	u32 token;
} __packed;

struct cvp_set_nbmode_enable_cmd {
	struct apr_hdr hdr;
	struct vss_icommon_cmd_set_ui_property_enable_t cvp_set_nbmode;
} __packed;

struct cvp_set_spkmode_enable_cmd {
	struct apr_hdr hdr;
	struct vss_icommon_cmd_set_ui_property_enable_t cvp_set_spkmode;
} __packed;

struct cvp_set_device_info_cmd {
	struct apr_hdr hdr;
	struct vss_icommon_cmd_set_ui_property_enable_t cvp_set_device_info;
} __packed;

static struct afe_ctl this_afe;
static struct cvs_ctl this_cvs;
static struct cvp_ctl this_cvp;
static struct common_data *common;

struct afe_port {
	unsigned int voice_tracking_id;
	unsigned int amp_rx_id;
	unsigned int amp_tx_id;
	unsigned int rx_topology;
	unsigned int tx_topology;
};
static struct afe_port afe_port;

static struct mutex asm_lock;

static int loopback_mode;
static int loopback_prev_mode;

/****************************************************************************/
/*//////////////////// MAXIM SPEAKER AMP DSM SOLUTION //////////////////////*/
/****************************************************************************/
#ifdef CONFIG_SND_SOC_MAXIM_DSM
static int32_t q6audio_adaptation_dsm_callback(
	struct apr_client_data *data, void *priv)
{
	u32 param_id;
	struct afe_dsm_spkr_prot_response_data *resp;

	if ((data == NULL) || (priv == NULL)) {
		pr_err("%s: data or priv is NULL\n", __func__);
		return -EINVAL;
	}

	if (data->opcode == RESET_EVENTS) {
		pr_debug("%s: reset event = %d %d apr[%pK]\n",
			__func__,
			data->reset_event, data->reset_proc, this_afe.apr);

		if (this_afe.apr) {
			apr_reset(this_afe.apr);
			atomic_set(&this_afe.state, 0);
			this_afe.apr = NULL;
		}

		return 0;
	}

	if (data->opcode == AFE_PORT_CMDRSP_GET_PARAM_V2) {
		resp = (struct afe_dsm_spkr_prot_response_data *) data->payload;
		if (!(&(resp->pdata))) {
			pr_err("%s: Error: resp pdata is NULL\n", __func__);
			return -EINVAL;
		}
		param_id = resp->pdata.param_id;

		pr_info("%s: param_id=%#x payload_size=%d\n",
						__func__, param_id,
						data->payload_size);

		if (param_id == AFE_PARAM_ID_CALIB_RES_CFG) {
			if (data->payload_size < sizeof(this_afe.calib_data)) {
				pr_err("%s: Error: received size %d, calib_data size %zu\n",
					__func__, data->payload_size,
					sizeof(this_afe.calib_data));
				return -EINVAL;
			}
			memcpy(&this_afe.calib_data, data->payload,
				sizeof(this_afe.calib_data));
			if (!this_afe.calib_data.status) {
				atomic_set(&this_afe.state, 0);

			} else {
				pr_debug("%s: calib resp status: %d", __func__,
					  this_afe.calib_data.status);
				atomic_set(&this_afe.state, -1);
			}
			wake_up(&this_afe.wait);
		} else if (param_id == AFE_PARAM_ID_CALIB_RES_LOG_CFG) {
			if (data->payload_size <
				sizeof(this_afe.calib_log_data)) {
				pr_err("%s: Error: log received size %d, calib_data size %zu\n",
					__func__, data->payload_size,
					sizeof(this_afe.calib_log_data));
				return -EINVAL;
			}
			memcpy(&this_afe.calib_log_data, data->payload,
				sizeof(this_afe.calib_log_data));
			if (!this_afe.calib_log_data.status) {
				atomic_set(&this_afe.state, 0);
			} else {
				pr_debug("%s: calib log resp status: %d",
					__func__,
					this_afe.calib_log_data.status);
				atomic_set(&this_afe.state, -1);
			}
			wake_up(&this_afe.wait);
		}
	} else if (data->payload_size) {
		uint32_t *payload;

		payload = data->payload;
		if (data->opcode == APR_BASIC_RSP_RESULT) {
			pr_info("%s:opcode = 0x%x cmd = 0x%x status = 0x%x token=%d\n",
				__func__, data->opcode,
				payload[0], payload[1], data->token);
			/* payload[1] contains the error status for response */
			if (payload[1] != 0) {
				atomic_set(&this_afe.status, payload[1]);
				pr_err("%s: cmd = 0x%x returned error = 0x%x\n",
					__func__, payload[0], payload[1]);
			}
			switch (payload[0]) {
			case AFE_PORT_CMD_SET_PARAM_V2:
				atomic_set(&this_afe.state, 0);
				wake_up(&this_afe.wait);
				break;
			default:
				pr_err("%s: Unknown cmd 0x%x\n", __func__,
						payload[0]);
				break;
			}
		}
	}
	return 0;
}

static int q6audio_dsm_write(int port,
		struct afe_dsm_filter_set_params_t *set_config)
{
	int ret = -EINVAL;
	int index = 0;
	struct afe_dsm_spkr_prot_set_command config;
	int mod = maxdsm_get_rx_mod_id();

	memset(&config, 0, sizeof(config));
	if (!set_config) {
		pr_err("%s Invalid params\n", __func__);
		goto exit;
	}
	if ((q6audio_validate_port(port) < 0)) {
		pr_err("%s invalid port %d", __func__, port);
		goto exit;
	}

	index = q6audio_get_port_index(port);
	if (port != maxdsm_get_rx_port_id())
		mod = maxdsm_get_tx_mod_id();
	config.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	config.hdr.pkt_size = sizeof(config);
	config.hdr.src_port = SEC_ADAPTATAION_DSM_SRC_PORT;
	config.hdr.dest_port = SEC_ADAPTATAION_DSM_DEST_PORT;
	config.hdr.token = index;

	config.hdr.opcode = AFE_PORT_CMD_SET_PARAM_V2;
	config.param.port_id = q6audio_get_port_id(port);
	config.param.payload_size = sizeof(config) - sizeof(config.hdr)
		- sizeof(config.param);
	config.pdata.param_id = AFE_PARAM_ID_FBSP_MODE_RX_CFG;
	config.pdata.param_size = sizeof(config.set_config);
	config.pdata.module_id = mod;
	config.set_config = *set_config;

	atomic_set(&this_afe.state, 1);
	pr_info("%s: port=%#x, port_id=%#x, param_id=%#x, module_id=%#x\n",
		__func__, port,
		config.param.port_id,
		config.pdata.param_id,
		config.pdata.module_id);

	ret = apr_send_pkt(this_afe.apr, (uint32_t *) &config);
	if (ret < 0) {
		pr_err("%s: Setting param for port %d param[0x%x]failed\n",
		 __func__, port, AFE_PARAM_ID_FBSP_MODE_RX_CFG);
		goto exit;
	}
	ret = wait_event_timeout(this_afe.wait,
		(atomic_read(&this_afe.state) == 0),
		msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		ret = -EINVAL;
		goto exit;
	}
	if (atomic_read(&this_afe.status) != 0) {
		pr_err("%s: config cmd failed\n", __func__);
		ret = -EINVAL;
		goto exit;
	}
	ret = 0;
exit:
	pr_debug("%s config.pdata.param_id %x status %d\n",
		__func__, config.pdata.param_id, ret);
	return ret;
}

int q6audio_dsm_read(int port)
{
	int ret = -EINVAL;
	int index = 0;
	struct afe_dsm_spkr_prot_get_command config;
	int mod = maxdsm_get_rx_mod_id();

	pr_debug("%s: port_id = 0x%x, module_id = 0x%x\n",
		__func__, q6audio_get_port_id(port), maxdsm_get_rx_mod_id());

	memset(&config, 0, sizeof(config));

	if ((q6audio_validate_port(port) < 0)) {
		pr_err("%s invalid port %d\n", __func__, port);
		goto exit;
	}
	index = q6audio_get_port_index(port);
	if (port != maxdsm_get_rx_port_id())
		mod = maxdsm_get_tx_mod_id();
	config.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	config.hdr.pkt_size = sizeof(config);
	config.hdr.src_port = SEC_ADAPTATAION_DSM_SRC_PORT;
	config.hdr.dest_port = SEC_ADAPTATAION_DSM_DEST_PORT;
	config.hdr.token = index;
	config.hdr.opcode =  AFE_PORT_CMD_GET_PARAM_V2;
	config.get_param.mem_map_handle = 0;
	config.get_param.module_id = mod;
	config.get_param.param_id = AFE_PARAM_ID_CALIB_RES_CFG;
	config.get_param.payload_address_lsw = 0;
	config.get_param.payload_address_msw = 0;
	config.get_param.payload_size = sizeof(config)
		- sizeof(config.get_param);
	config.get_param.port_id = q6audio_get_port_id(port);
	config.pdata.module_id = mod;
	config.pdata.param_id = AFE_PARAM_ID_CALIB_RES_CFG;
	config.pdata.param_size = sizeof(config.res_cfg);

	atomic_set(&this_afe.state, 1);

	pr_info("%s: port_id=%#x, param_id=%#x, module_id=%#x module_id=%#x\n",
		__func__,
		config.get_param.port_id,
		config.get_param.param_id,
		config.get_param.module_id,
		config.pdata.module_id);

	ret = apr_send_pkt(this_afe.apr, (uint32_t *)&config);
	if (ret < 0) {
		pr_err("%s: get param port %d param id[0x%x]failed\n",
			   __func__, port, config.get_param.param_id);
		goto exit;
	}
	ret = wait_event_timeout(this_afe.wait,
		(atomic_read(&this_afe.state) == 0),
		msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout with %d\n", __func__, ret);
		ret = -EINVAL;
		goto exit;
	}
	if (atomic_read(&this_afe.status) > 0) {
		pr_err("%s: config cmd failed\n", __func__);
		ret = -EINVAL;
		goto exit;
	}
	ret = 0;
exit:
	return ret;
}

int q6audio_dsm_log_read(int port)
{
	int ret = -EINVAL;
	int index = 0;
	struct afe_dsm_spkr_prot_get_log_command config;
	int mod = maxdsm_get_rx_mod_id();

	pr_debug("%s: port_id = 0x%x, module_id = 0x%x\n",
		__func__, q6audio_get_port_id(port), maxdsm_get_rx_mod_id());

	memset(&config, 0, sizeof(config));

	if ((q6audio_validate_port(port) < 0)) {
		pr_err("%s invalid port %d\n", __func__, port);
		goto exit;
	}
	index = q6audio_get_port_index(port);
	if (port != maxdsm_get_rx_port_id())
		mod = maxdsm_get_tx_mod_id();
	config.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	config.hdr.pkt_size = sizeof(config);
	config.hdr.src_port = SEC_ADAPTATAION_DSM_SRC_PORT;
	config.hdr.dest_port = SEC_ADAPTATAION_DSM_DEST_PORT;
	config.hdr.token = index;
	config.hdr.opcode =  AFE_PORT_CMD_GET_PARAM_V2;
	config.get_param.mem_map_handle = 0;
	config.get_param.module_id = mod;
	config.get_param.param_id = AFE_PARAM_ID_CALIB_RES_LOG_CFG;
	config.get_param.payload_address_lsw = 0;
	config.get_param.payload_address_msw = 0;
	config.get_param.payload_size = sizeof(config)
		- sizeof(config.get_param);
	config.get_param.port_id = q6audio_get_port_id(port);
	config.pdata.module_id = mod;
	config.pdata.param_id = AFE_PARAM_ID_CALIB_RES_LOG_CFG;
	config.pdata.param_size = sizeof(config.res_log_cfg);

	atomic_set(&this_afe.state, 1);

	pr_info("%s: port_id=%#x, param_id=%#x, module_id=%#x module_id=%#x\n",
		__func__,
		config.get_param.port_id,
		config.get_param.param_id,
		config.get_param.module_id,
		config.pdata.module_id);

	ret = apr_send_pkt(this_afe.apr, (uint32_t *)&config);
	if (ret < 0) {
		pr_err("%s: get param port %d param id[0x%x]failed\n",
			   __func__, port, config.get_param.param_id);
		goto exit;
	}
	ret = wait_event_timeout(this_afe.wait,
		(atomic_read(&this_afe.state) == 0),
		msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout with %d\n", __func__, ret);
		ret = -EINVAL;
		goto exit;
	}
	if (atomic_read(&this_afe.status) > 0) {
		pr_err("%s: config cmd failed\n", __func__);
		ret = -EINVAL;
		goto exit;
	}
	ret = 0;
exit:
	return ret;
}

static int dsm_get_afe_params(
		void *param,
		int param_size,
		void *data,
		int index)
{
	struct maxim_dsm *maxdsm = (struct maxim_dsm *)data;
	unsigned int *p = (unsigned int *)param;
	int idx = index;
	int binfo_idx = 0;
	int i;

	for (i = 0; i < param_size; i++) {
		maxdsm->param[idx++] = *(p+i);
		binfo_idx = (idx - 1) >> 1;
		maxdsm->param[idx++] = 1 << maxdsm->binfo[binfo_idx];

		pr_debug("%s: [%d,%d]: 0x%08x, 0x%08x\n",
				__func__,
				idx - 2, idx - 1,
				maxdsm->param[idx - 2], maxdsm->param[idx - 1]);
	}

	return idx;
}

static int dsm_set_afe_params(
		void *param,
		int param_size,
		void *data,
		int index)
{
	struct maxim_dsm *maxdsm = (struct maxim_dsm *)data;
	unsigned int *p = (unsigned int *)param;
	int idx = index;
	int i;

	for (i = 0; i < param_size; i++) {
		*(p+i) = maxdsm->param[idx];
		idx += 2;

		pr_debug("%s: [%d,%d]: 0x%08x / 0x%08x -> 0x%08x\n",
				__func__,
				idx - 2, idx - 1,
				maxdsm->param[idx - 2], maxdsm->param[idx - 1],
				*(p+i));
	}

	return idx;
}

static int dsm_get_param_size(int version)
{
	int param_size = 0;

	switch (version) {
	case VERSION_3_0:
		param_size = PARAM_DSM_3_0_MAX;
		break;
	case VERSION_3_5_B:
		param_size = PARAM_DSM_3_5_MAX;
		break;
	case VERSION_4_0_B:
		param_size = PARAM_DSM_4_0_MAX;
		break;
	default:
		param_size = -EINVAL;
		break;
	}

	pr_debug("%s: param_size: %d, version: %d\n",
			__func__, param_size, version);

	return param_size;
}

int32_t dsm_read_write(void *data)
{
	struct afe_dsm_filter_set_params_t filter_params = {0,};

	struct maxim_dsm *maxdsm = (struct maxim_dsm *)data;
	uint32_t dsm_params = maxdsm->filter_set;
	uint32_t version = maxdsm->version;
	int32_t ret = 0;
	int port_id = maxdsm->rx_port_id;

	if (this_afe.apr == NULL) {
		this_afe.apr = apr_register("ADSP", "AFE",
						q6audio_adaptation_dsm_callback,
						SEC_ADAPTATAION_DSM_SRC_PORT,
						&this_afe);
	}

	if (maxdsm->tx_port_id & (1 << 31))
		port_id = maxdsm->tx_port_id & 0xFFFF;

	switch (dsm_params) {
	case DSM_ID_FILTER_GET_AFE_PARAMS:
		if (q6audio_dsm_read(port_id)) {
			ret = -EINVAL;
			break;
		}
		if (maxdsm->param && maxdsm->binfo) {
			dsm_get_afe_params(
				&this_afe.calib_data.res_cfg.dcResistance,
				(int)(dsm_get_param_size(version) >> 1),
				maxdsm,
				0);
		}
		break;
	case DSM_ID_FILTER_GET_LOG_AFE_PARAMS:
		if (q6audio_dsm_log_read(port_id)) {
			ret = -EINVAL;
			break;
		}
		if (maxdsm->param && maxdsm->binfo) {
			maxdsm_log_update(this_afe.calib_log_data.res_log_cfg.byteLogArray,
					this_afe.calib_log_data.res_log_cfg.intLogArray,
					this_afe.calib_log_data.res_log_cfg.afterProbByteLogArray,
					this_afe.calib_log_data.res_log_cfg.afterProbIntLogArray,
					this_afe.calib_log_data.res_log_cfg.intLogMaxArray);
		}
		break;
	case DSM_ID_FILTER_SET_AFE_CNTRLS:
		if (!maxdsm->param || !maxdsm->binfo) {
			ret = -EINVAL;
			break;
		}
		dsm_set_afe_params(
				&filter_params.dcResistance,
				(int)(dsm_get_param_size(version) >> 1),
				maxdsm,
				0);
		ret = q6audio_dsm_write(port_id, &filter_params);
		break;
	}

	return ret;
}
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

#ifdef CONFIG_SND_SOC_TFA9872
/* shared mem */
static int afe_nxp_mmap_create(void)
{
	int rc = 0;
	size_t len;
	struct rtac_cal_block_data *tfa_cal = &(this_afe.tfa_cal);

	tfa_cal->map_data.map_size = SZ_4K;

	rc = msm_audio_ion_alloc("tfa_cal",
			&tfa_cal->map_data.ion_client,
			&tfa_cal->map_data.ion_handle,
			tfa_cal->map_data.map_size,
			&tfa_cal->cal_data.paddr,
			&len,
			&tfa_cal->cal_data.kvaddr);
	if (rc) {
		pr_err("%s: ION_alloc failed, rc = %d\n", __func__, rc);
		return rc;
	}

	rc = afe_map_rtac_block(tfa_cal);
	if (rc < 0) {
		pr_err("%s : memory mapping failed = %d\n", __func__, rc);
		msm_audio_ion_free(tfa_cal->map_data.ion_client,
			tfa_cal->map_data.ion_handle);

		tfa_cal->map_data.ion_client = NULL;
		tfa_cal->map_data.ion_handle = NULL;
	}

	return rc;
}

static void afe_nxp_mmap_destroy(void)
{
	int rc = 0;
	struct rtac_cal_block_data *tfa_cal = &(this_afe.tfa_cal);

	if (tfa_cal->map_data.map_handle == 0) {
		pr_debug("%s: mmap handle is 0, nothing to unmap\n",
			__func__);
		return;
	}

	rc = afe_cmd_memory_unmap(tfa_cal->map_data.map_handle);
	if (rc) {
		pr_err("%s: AFE memory unmap failed %d, handle 0x%x\n",
			__func__, rc, tfa_cal->map_data.map_handle);
	}
	tfa_cal->map_data.map_handle = 0;

	rc = msm_audio_ion_free(
		tfa_cal->map_data.ion_client, tfa_cal->map_data.ion_handle);
	if (rc < 0)
		pr_err("%s: msm_audio_ion_free failed:\n", __func__);

	tfa_cal->map_data.ion_client = NULL;
	tfa_cal->map_data.ion_handle = NULL;
}

static int32_t q6audio_tfadsp_callback(struct apr_client_data *data, void *priv)
{
	uint32_t *payload = NULL;

	if ((data == NULL) || (priv == NULL)) {
		pr_err("%s: data or priv is NULL\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s: data->opcode = 0x%x\n", __func__, data->opcode);

	if (data->opcode == RESET_EVENTS) {
		int rc = 0;
		struct rtac_cal_block_data *tfa_cal = &(this_afe.tfa_cal);

		pr_debug("%s: reset event = %d %d apr[%pK]\n",
			__func__,
			data->reset_event, data->reset_proc, this_afe.apr);

		if (this_afe.apr) {
			apr_reset(this_afe.apr);
			atomic_set(&this_afe.state, 0);
			this_afe.apr = NULL;
		}

		/* Free ion memory to fix a SSR(Sub System Reset) test issue*/
		tfa_cal->map_data.map_handle = 0;
		rc = msm_audio_ion_free(
			tfa_cal->map_data.ion_client, tfa_cal->map_data.ion_handle);
		if (rc < 0)
			pr_err("%s: msm_audio_ion_free failed:\n", __func__);

		tfa_cal->map_data.ion_client = NULL;
		tfa_cal->map_data.ion_handle = NULL;

		return 0;
	}

	payload = data->payload;
	if (data->opcode == AFE_PORT_CMDRSP_GET_PARAM_V2) {
		if (atomic_read(&this_afe.tfa_state) == 1 && data->token ==
			q6audio_get_port_index(afe_port.amp_rx_id)) {
			pr_debug("%s:opcode=0x%x, token = %d, payload = %d, %d\n",
				__func__, data->opcode,
				data->token, payload[0], payload[1]);

			if (data->payload_size == sizeof(uint32_t))
				atomic_set(&this_afe.status, payload[0]);
			else if (data->payload_size == (2*sizeof(uint32_t)))
				atomic_set(&this_afe.status, payload[1]);
			atomic_set(&this_afe.tfa_state, 0);
			wake_up(&this_afe.wait);
		}
	} else if (data->payload_size) {
		if (data->opcode == APR_BASIC_RSP_RESULT) {
			/* payload[1] contains the error status for response */
			if (payload[1] != 0) {
				atomic_set(&this_afe.status, payload[1]);
				pr_err("%s: cmd=0x%x, returned error=0x%x\n",
					__func__, payload[0], payload[1]);
			}
			switch (payload[0]) {
			case AFE_PORT_CMD_SET_PARAM_V2:
				if (atomic_read(&this_afe.tfa_state) == 1) {
					pr_info("%s:AFE_PORT_CMD_SET_PARAM_V2, token=%d, payload=%d, %d\n",
						__func__, data->token,
						payload[0], payload[1]);

					if (data->payload_size ==
						sizeof(uint32_t))
						atomic_set(&this_afe.status,
						payload[0]);
					else if (data->payload_size ==
						(2*sizeof(uint32_t)))
						atomic_set(&this_afe.status,
						payload[1]);
					atomic_set(&this_afe.tfa_state, 0);
					wake_up(&this_afe.wait);
				}
				break;
			default:
				pr_err("%s: Unknown cmd 0x%x\n",
							__func__, payload[0]);
				break;
			}
		}
	}
	return 0;
}

static int q6audio_tfadsp_read(int dev, int buf_size, char *buf)
{
	int result = 0;
	int tfa_port_id = afe_port.amp_rx_id;
	int index = 0;
	struct afe_tfa_dsp_read_msg_t tfa_dsp_read_msg;
	struct afe_port_param_data_v2 pdata;

	if (buf_size < 0
		|| buf_size > this_afe.tfa_cal.map_data.map_size - sizeof(pdata)
		|| buf == NULL) {
		pr_err("%s: invalid param or buf: buf_size=%d\n",
					__func__, buf_size);
		result = -EINVAL;
		goto fail_cmd;
	}

	if (this_afe.tfa_cal.map_data.ion_client == NULL) {
		result = afe_nxp_mmap_create();
		if (result) {
			pr_err("%s: could not create mmap for TFADSP! %d\n",
				__func__, result);
			result = -EINVAL;
			goto fail_cmd;
		}
	}

	if (this_afe.apr == NULL) {
		this_afe.apr = apr_register("ADSP", "AFE",
				q6audio_tfadsp_callback,
				SEC_ADAPTATAION_DSM_SRC_PORT, &this_afe);
	}

	pr_info("%s: buf_size = %d\n", __func__, buf_size);

	index = q6audio_get_port_index(tfa_port_id);
	if (index < 0 || index >= AFE_MAX_PORTS) {
		pr_err("%s: AFE port index[%d] invalid!\n",
			__func__, index);
		result = -EINVAL;
		goto fail_cmd;
	}

	/*clean buffer*/
	memset(&tfa_dsp_read_msg, 0x00, sizeof(tfa_dsp_read_msg));

	/*APR header*/
	tfa_dsp_read_msg.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
					APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	tfa_dsp_read_msg.hdr.pkt_size = sizeof(tfa_dsp_read_msg);
	tfa_dsp_read_msg.hdr.src_port = SEC_ADAPTATAION_DSM_SRC_PORT;
	tfa_dsp_read_msg.hdr.dest_port = SEC_ADAPTATAION_DSM_DEST_PORT;
	tfa_dsp_read_msg.hdr.token = index;
	tfa_dsp_read_msg.hdr.opcode = AFE_PORT_CMD_GET_PARAM_V2;

	/*get param*/
	tfa_dsp_read_msg.get_param.module_id = AFE_MODULE_ID_TFADSP;
	tfa_dsp_read_msg.get_param.param_id = AFE_PARAM_ID_TFADSP_READ_MSG;
	tfa_dsp_read_msg.get_param.port_id = tfa_port_id;
	tfa_dsp_read_msg.get_param.payload_size = buf_size;

	tfa_dsp_read_msg.get_param.payload_address_lsw =
	lower_32_bits(this_afe.tfa_cal.cal_data.paddr);
	tfa_dsp_read_msg.get_param.payload_address_msw =
	msm_audio_populate_upper_32_bits(this_afe.tfa_cal.cal_data.paddr);
	tfa_dsp_read_msg.get_param.mem_map_handle =
	this_afe.tfa_cal.map_data.map_handle;

	atomic_set(&this_afe.tfa_state, 1);
	atomic_set(&this_afe.status, 0);

	result = apr_send_pkt(this_afe.apr, (uint32_t *) &tfa_dsp_read_msg);
	if (result < 0) {
		pr_err("%s: failed port = 0x%x, ret = %d\n",
			__func__, afe_port.amp_rx_id, result);
		goto fail_cmd;
	}

	result = wait_event_timeout(this_afe.wait,
				(atomic_read(&this_afe.tfa_state) == 0),
				msecs_to_jiffies(TIMEOUT_MS));
	if (!result) {
		pr_err("%s: wait_event timeout\n", __func__);
		result = -EINVAL;
		goto fail_cmd;
	} else {
		result = 0;
	}

	if (atomic_read(&this_afe.status) > 0) {
		pr_err("%s: config cmd failed [%s]\n", __func__,
			adsp_err_get_err_str(atomic_read(&this_afe.status)));
		result = adsp_err_get_lnx_err_code(
					atomic_read(&this_afe.status));
		goto fail_cmd;
	}

	memcpy(buf, (char *)this_afe.tfa_cal.cal_data.kvaddr + sizeof(pdata),
		buf_size);

	if (buf_size >= 12) {
		int *pReadData = (int *)buf;

		pr_debug("%s: read data pReadData[1]=%d, Impedance=%d mOhms\n"
			, __func__, pReadData[1], (pReadData[1]*1000)/65536);
		pr_debug("%s: read data buf[0]=%02x, buf[1]=%02x, buf[2]=%02x, buf[3]=%02x\n"
			, __func__, buf[0], buf[1], buf[2], buf[3]);
		pr_debug("%s: read data buf[4]=%02x, buf[5]=%02x, buf[6]=%02x, buf[7]=%02x\n"
			, __func__, buf[4], buf[5], buf[6], buf[7]);
		pr_debug("%s: read data buf[8]=%02x, buf[9]=%02x, buf[10]=%02x, buf[11]=%02x\n"
			, __func__, buf[8], buf[9], buf[10], buf[11]);
	}

fail_cmd:
	return result;
}

#if defined(TFA_SHARED_MEM_IPC)
static int q6audio_tfadsp_write(
		int dev, int buf_size, char *buf, int msg_type, int num_msgs)
{
	int result = 0;
	int index = 0;
	uint8_t *pmem = NULL;
	struct afe_tfa_dsp_send_msg_t afe_cal;
	struct afe_port_param_data_v2 pdata;

	if (buf_size < 0
		|| buf_size > this_afe.tfa_cal.map_data.map_size - sizeof(pdata)
		|| buf == NULL) {
		pr_err("%s: invalid param or buf: buf_size=%d\n",
					__func__, buf_size);
		result = -EINVAL;
		goto fail_cmd;
	}

	if (this_afe.tfa_cal.map_data.ion_client == NULL) {
		result = afe_nxp_mmap_create();
		if (result) {
			pr_err("%s: mmap create failed %d\n", __func__, result);
			goto fail_cmd;
		}
	}

	if (this_afe.apr == NULL) {
		this_afe.apr = apr_register("ADSP", "AFE",
				q6audio_tfadsp_callback,
				SEC_ADAPTATAION_DSM_SRC_PORT, &this_afe);
	}

	pr_info("%s: num_msgs = %d, msg_type = %d, buf_size = %d\n",
		__func__, num_msgs, msg_type, buf_size);

	index = q6audio_get_port_index(afe_port.amp_rx_id);
	if (index < 0 || index >= AFE_MAX_PORTS) {
		pr_err("%s: AFE port index[%d] invalid!\n", __func__, index);
		result = -EINVAL;
		goto fail_cmd;
	}

	afe_cal.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
					APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	afe_cal.hdr.pkt_size = sizeof(afe_cal);
	afe_cal.hdr.src_port = SEC_ADAPTATAION_DSM_SRC_PORT;
	afe_cal.hdr.dest_port = SEC_ADAPTATAION_DSM_DEST_PORT;
	afe_cal.hdr.token = index;
	afe_cal.hdr.opcode = AFE_PORT_CMD_SET_PARAM_V2;
	afe_cal.set_param.port_id = afe_port.amp_rx_id;
	afe_cal.set_param.payload_size = buf_size + sizeof(pdata);
	afe_cal.set_param.payload_address_lsw =
				lower_32_bits(this_afe.tfa_cal.cal_data.paddr);
	afe_cal.set_param.payload_address_msw =
				msm_audio_populate_upper_32_bits(
					this_afe.tfa_cal.cal_data.paddr);
	afe_cal.set_param.mem_map_handle = this_afe.tfa_cal.map_data.map_handle;

	pmem = (uint8_t *)this_afe.tfa_cal.cal_data.kvaddr;

	/* port info added */
	memset(&pdata, 0x0, sizeof(pdata));
	pdata.module_id  = AFE_MODULE_ID_TFADSP;
	pdata.param_id   = AFE_PARAM_ID_TFADSP_SEND_MSG;
	pdata.param_size = buf_size;
	memcpy(pmem, &pdata, sizeof(pdata));

	/* copy data to shared mem */
	memcpy(pmem + sizeof(pdata), buf, buf_size);

	atomic_set(&this_afe.tfa_state, 1);
	atomic_set(&this_afe.status, 0);

	result = apr_send_pkt(this_afe.apr, (uint32_t *)&afe_cal);
	if (result < 0) {
		pr_err("%s: failed port = 0x%x, ret = %d\n",
				__func__, afe_port.amp_rx_id, result);
		goto fail_cmd;
	}

	result = wait_event_timeout(this_afe.wait,
				(atomic_read(&this_afe.tfa_state) == 0),
				msecs_to_jiffies(TIMEOUT_MS));
	if (!result) {
		pr_err("%s: wait_event timeout\n", __func__);
		result = -EINVAL;
		goto fail_cmd;
	} else {
		result = 0;
	}

	if (atomic_read(&this_afe.status) > 0) {
		pr_err("%s: config cmd failed [%s]\n",
		__func__, adsp_err_get_err_str(atomic_read(&this_afe.status)));
		result = adsp_err_get_lnx_err_code(
						atomic_read(&this_afe.status));
		goto fail_cmd;
	}
fail_cmd:
	return result;
}
#else /* TFA_SHARED_MEM_IPC */
static int q6audio_tfadsp_write(
	int dev, int buf_size, char *buf, int msg_type, int num_msgs)
{
	int ret = -EINVAL;
	int tfa_port_id = afe_port.amp_rx_id;
	int tfa_apr_pkt_size;
	static struct afe_tfa_dsp_send_msg_t *tfa_dsp_send_msg;
	int index = 0;

	if (dev < 0) { /*free memory*/
		pr_info("%s: kfree(tfa_dsp_send_msg)\n", __func__);
		if (tfa_dsp_send_msg != NULL)
			kfree(tfa_dsp_send_msg);
		tfa_dsp_send_msg = NULL;

		return 0;
	}

	if (buf == NULL) {
		pr_err("%s: error! send buf = NULL\n", __func__);
		goto fail_cmd;
	}

	if (this_afe.apr == NULL) {
		this_afe.apr = apr_register("ADSP", "AFE",
				q6audio_tfadsp_callback,
				SEC_ADAPTATAION_DSM_SRC_PORT, &this_afe);
	}

	index = q6audio_get_port_index(tfa_port_id);
	if (index < 0 || index >= AFE_MAX_PORTS) {
		pr_err("%s: AFE port index[%d] invalid!\n",
					__func__, index);
		goto fail_cmd;
	}
	/*
	* msg_type = AFE_TFADSP_MSG_TYPE_NORMAL ;
	* num_msgs = 1;
	*/
	msg_type = AFE_TFADSP_MSG_TYPE_RAW;

	if (((buf[0]<<8)|buf[1]) == 65535)
		pr_info("%s: [0]:0x%x-[1]:0x%x-[2]:0x%x-[3]:0x%x, [4]:0x%x-[5]:0x%x, packet_id:%d, packet_size:%d\n",
			__func__, buf[0], buf[1], buf[2], buf[3],
			buf[4], buf[5], (buf[0]<<8)|buf[1], (buf[2]<<8)|buf[3]);
	pr_debug("%s:msg_type:%d--buf_size:%d--num_msgs:%d\n",
					__func__, msg_type, buf_size, num_msgs);
	/*apr total packet size*/
	if (msg_type == AFE_TFADSP_MSG_TYPE_NORMAL) {
		tfa_apr_pkt_size =
			sizeof(struct afe_tfa_dsp_send_msg_t)
			- sizeof(char *) + buf_size;
		pr_debug("%s:size1:%d--size3:%d--\n", __func__,
			(int)sizeof(struct afe_tfa_dsp_send_msg_t), buf_size);
	} else if (msg_type == AFE_TFADSP_MSG_TYPE_RAW) {
		tfa_apr_pkt_size =
			sizeof(struct afe_tfa_dsp_send_msg_t)
			- sizeof(struct afe_tfa_dsp_payload_t) + buf_size;
		pr_debug("%s:size12:%d--size22:%d--size32:%d--\n", __func__,
			(int)sizeof(struct afe_tfa_dsp_send_msg_t),
			(int)sizeof(struct afe_tfa_dsp_payload_t), buf_size);
	} else {
		pr_debug("%s:msg_type:%d--buf_size:%d--num_msgs:%d\n", __func__,
			msg_type, buf_size, num_msgs);
	}
	/*check the max size of the apr packet*/
	if (tfa_apr_pkt_size > AFE_APR_MAX_PKT_SIZE) {
		pr_err("%s: error! apr pkt size(%d) > apr max pkt size(%d)\n",
			__func__, tfa_apr_pkt_size, AFE_APR_MAX_PKT_SIZE);
		goto fail_cmd;
	}

	if (tfa_dsp_send_msg == NULL) {
		pr_debug("%s: kmalloc for tfa_dsp_send_msg (size: %d)\n",
			__func__, AFE_APR_MAX_PKT_SIZE);
		tfa_dsp_send_msg =
			(struct afe_tfa_dsp_send_msg_t *)
			kmalloc(AFE_APR_MAX_PKT_SIZE, GFP_KERNEL);
	}

	if (tfa_dsp_send_msg == NULL) {
		pr_err("%s: kmalloc: tfa_dsp_send_msg = NULL\n", __func__);
		goto fail_cmd;
	}

	/*clean buffer*/
	memset(tfa_dsp_send_msg, 0x00, tfa_apr_pkt_size);

	/*APR header*/
	tfa_dsp_send_msg->hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
					APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	tfa_dsp_send_msg->hdr.pkt_size = tfa_apr_pkt_size;
	tfa_dsp_send_msg->hdr.src_port = SEC_ADAPTATAION_DSM_SRC_PORT;
	tfa_dsp_send_msg->hdr.dest_port = SEC_ADAPTATAION_DSM_DEST_PORT;
	tfa_dsp_send_msg->hdr.token = index;
	tfa_dsp_send_msg->hdr.opcode = AFE_PORT_CMD_SET_PARAM_V2;

	/*set param*/
	tfa_dsp_send_msg->set_param.port_id = tfa_port_id;
	tfa_dsp_send_msg->set_param.payload_size =
		tfa_dsp_send_msg->hdr.pkt_size
		- sizeof(tfa_dsp_send_msg->hdr)
		- sizeof(tfa_dsp_send_msg->set_param);

	/*param data*/
	tfa_dsp_send_msg->pdata.module_id = AFE_MODULE_ID_TFADSP;
	tfa_dsp_send_msg->pdata.param_id = AFE_PARAM_ID_TFADSP_SEND_MSG;
	/*data size for the module id and param id: size of send msg*/
	if (msg_type == AFE_TFADSP_MSG_TYPE_NORMAL) {
		tfa_dsp_send_msg->pdata.param_size = buf_size
			+ sizeof(tfa_dsp_send_msg->payload.num_msgs)
			+ sizeof(tfa_dsp_send_msg->payload.buf_size);
		pr_err("param data param_size:%d\n",
			tfa_dsp_send_msg->pdata.param_size);
	} else if (msg_type == AFE_TFADSP_MSG_TYPE_RAW) {
		tfa_dsp_send_msg->pdata.param_size = buf_size;
	}

	/*payload: fill the apr packet payload*/
	if (msg_type == AFE_TFADSP_MSG_TYPE_NORMAL) {
		tfa_dsp_send_msg->payload.num_msgs = 1;
		tfa_dsp_send_msg->payload.buf_size = buf_size;
		memcpy(tfa_dsp_send_msg->payload.buf, buf, buf_size);
	} else if (msg_type == AFE_TFADSP_MSG_TYPE_RAW) {
		memcpy(tfa_dsp_send_msg->payload.address, buf, buf_size);
	}

	atomic_set(&this_afe.status, 0);
	atomic_set(&this_afe.tfa_state, 1);
	ret = apr_send_pkt(this_afe.apr, (uint32_t *) tfa_dsp_send_msg);
	if (ret < 0) {
		pr_err("%s: apr_send_pkt error = %d\n", __func__, ret);
		goto fail_cmd;
	}

	ret = wait_event_timeout(this_afe.wait,
				(atomic_read(&this_afe.tfa_state) == 0),
				msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		ret = -EINVAL;
		goto fail_cmd;
	}
	if (atomic_read(&this_afe.status) > 0) {
		pr_err("%s: tfa_dsp_send_msg cmd failed [%s]\n",
			__func__, adsp_err_get_err_str(
			atomic_read(&this_afe.status)));
		ret = adsp_err_get_lnx_err_code(atomic_read(&this_afe.status));
		goto fail_cmd;
	}

	ret = 0;

	pr_debug("%s: msg type %s\n", __func__,
			msg_type == AFE_TFADSP_MSG_TYPE_NORMAL ?
			"normal" : "raw");
	pr_debug("%s: num msgs = %d\n", __func__, num_msgs);
	pr_debug("%s: send buf_size = %d\n", __func__, buf_size);

	/*show data max 3 ints*/
	pr_debug("%s: send buf32[0] = 0x%08x\n",
			__func__, *((int32_t *)buf));
	if (buf_size >= 8)
		pr_debug("%s: send buf32[1] = 0x%08x\n",
			__func__, *((int32_t *)(buf+4)));
	if (buf_size >= 12)
		pr_debug("%s: send buf32[2] = 0x%08x\n",
			__func__, *((int32_t *)(buf+8)));

fail_cmd:
	pr_debug("%s: status %d\n", __func__, ret);
	return ret;
}
#endif /* TFA_SHARED_MEM_IPC */
#endif /* CONFIG_SND_SOC_TFA9872 */

int q6audio_get_afe_cal_validation(u16 port_id, u32 topology_id)
{
	int rc = 0;

	if (((topology_id == afe_port.tx_topology) &&
		(port_id == afe_port.amp_tx_id)) ||
		((topology_id == afe_port.rx_topology) &&
		(port_id == afe_port.amp_rx_id))) {
		pr_info("%s: afe cal, port[0x%x] topology[0x%x]\n",
				__func__, port_id, topology_id);
		rc =  TRUE;
	}

	return rc;
}
/****************************************************************************/
/*//////////////////////////// AUDIO SOLUTION //////////////////////////////*/
/****************************************************************************/
/* This function must be sync up from q6asm_set_flag_in_token() of q6asm.c */
static inline void q6asm_set_flag_in_token_in_adaptation(
				union asm_token_struct *asm_token,
				int flag, int flag_offset)
{
	if (flag)
		ASM_SET_BIT(asm_token->_token.flags, flag_offset);
}

/* This function must be sync up from q6asm_get_flag_from_token() of q6asm.c */
static inline int q6asm_get_flag_from_token_in_adaptation(
				union asm_token_struct *asm_token,
				int flag_offset)
{
	return ASM_TEST_BIT(asm_token->_token.flags, flag_offset);
}

/* This function must be sync up from q6asm_update_token() of q6asm.c */
static inline void q6asm_update_token_in_adaptation(u32 *token,
				u8 session_id, u8 stream_id,
				u8 buf_index, u8 dir, u8 nowait_flag)
{
	union asm_token_struct asm_token;

	asm_token.token = 0;
	asm_token._token.session_id = session_id;
	asm_token._token.stream_id = stream_id;
	asm_token._token.buf_index = buf_index;
	q6asm_set_flag_in_token_in_adaptation(
			&asm_token, dir, ASM_DIRECTION_OFFSET);
	q6asm_set_flag_in_token_in_adaptation(
			&asm_token, nowait_flag, ASM_CMD_NO_WAIT_OFFSET);
	*token = asm_token.token;
}

/* This function must be sync up from __q6asm_add_hdr_async() of q6asm.c */
static void __q6asm_add_hdr_async_in_adaptation(struct audio_client *ac,
				struct apr_hdr *hdr,
				uint32_t pkt_size, uint32_t cmd_flg,
				uint32_t stream_id, u8 no_wait_flag)
{
	dev_vdbg(ac->dev, "%s: pkt_size = %d, cmd_flg = %d, session = %d stream_id=%d\n",
			__func__, pkt_size, cmd_flg, ac->session, stream_id);
	hdr->hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
			APR_HDR_LEN(sizeof(struct apr_hdr)),
			APR_PKT_VER);
	if (ac->apr == NULL) {
		pr_err("%s: AC APR is NULL", __func__);
		return;
	}
	hdr->src_svc = ((struct apr_svc *)ac->apr)->id;
	hdr->src_domain = APR_DOMAIN_APPS;
	hdr->dest_svc = APR_SVC_ASM;
	hdr->dest_domain = APR_DOMAIN_ADSP;
	hdr->src_port = ((ac->session << 8) & 0xFF00) | (stream_id);
	hdr->dest_port = ((ac->session << 8) & 0xFF00) | (stream_id);
	if (cmd_flg) {
		q6asm_update_token_in_adaptation(&hdr->token,
				   ac->session,
				   0, /* Stream ID is NA */
				   0, /* Buffer index is NA */
				   0, /* Direction flag is NA */
				   no_wait_flag);
	}
	hdr->pkt_size  = pkt_size;
}

/* This function must be sync up from q6asm_add_hdr_async() of q6asm.c */
static void q6asm_add_hdr_async_in_adaptation(struct audio_client *ac,
				struct apr_hdr *hdr,
				uint32_t pkt_size, uint32_t cmd_flg)
{
	__q6asm_add_hdr_async_in_adaptation(ac, hdr, pkt_size, cmd_flg,
			      ac->stream_id, WAIT_CMD);
}

int q6asm_set_sound_alive(struct audio_client *ac, long *param)
{
	uint32_t sz = 0;
	int rc  = 0;
	int i = 0;
	struct asm_stream_cmd_set_pp_params_sa cmd;

	if (ac == NULL) {
		pr_err("%s: audio client is null\n", __func__);
		return -EINVAL;
	}

	sz = sizeof(struct asm_stream_cmd_set_pp_params_sa);
	q6asm_add_hdr_async_in_adaptation(ac, &cmd.hdr, sz, TRUE);

	cmd.hdr.opcode = ASM_STREAM_CMD_SET_PP_PARAMS_V2;
	cmd.param.data_payload_addr_lsw = 0;
	cmd.param.data_payload_addr_msw = 0;
	cmd.param.mem_map_handle = 0;
	cmd.param.data_payload_size =
			sizeof(cmd) - sizeof(cmd.hdr) - sizeof(cmd.param);
	cmd.data.module_id = ASM_MODULE_ID_PP_SA;
	cmd.data.param_id = ASM_PARAM_ID_PP_SA_PARAMS;
	cmd.data.param_size = cmd.param.data_payload_size - sizeof(cmd.data);
	cmd.data.reserved = 0;

	/* SA paramerters */
	cmd.OutDevice = param[0];
	cmd.Preset = param[1];
	for (i = 0; i < 9; i++)
		cmd.EqLev[i] = param[i+2];
	cmd.m3Dlevel = param[11];
	cmd.BElevel = param[12];
	cmd.CHlevel = param[13];
	cmd.CHRoomSize = param[14];
	cmd.Clalevel = param[15];
	cmd.volume = param[16];
	cmd.Sqrow = param[17];
	cmd.Sqcol = param[18];
	cmd.TabInfo = param[19];
	cmd.NewUI = param[20];
	cmd.m3DPositionOn = param[21];
	cmd.m3DPositionAngle[0] = param[22];
	cmd.m3DPositionAngle[1] = param[23];
	cmd.m3DPositionGain[0] = param[24];
	cmd.m3DPositionGain[1] = param[25];
	cmd.AHDRonoff = param[26];
	pr_info("%s: %d %d %d%d %d %d %d %d %d %d %d %d"
		" %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
		__func__,
		cmd.OutDevice, cmd.Preset, cmd.EqLev[0],
		cmd.EqLev[1], cmd.EqLev[2], cmd.EqLev[3],
		cmd.EqLev[4], cmd.EqLev[5], cmd.EqLev[6],
		cmd.EqLev[7], cmd.EqLev[8],
		cmd.m3Dlevel, cmd.BElevel, cmd.CHlevel,
		cmd.CHRoomSize, cmd.Clalevel, cmd.volume,
		cmd.Sqrow, cmd.Sqcol, cmd.TabInfo,
		cmd.NewUI, cmd.m3DPositionOn,
		cmd.m3DPositionAngle[0], cmd.m3DPositionAngle[1],
		cmd.m3DPositionGain[0], cmd.m3DPositionGain[1],
		cmd.AHDRonoff);

	rc = apr_send_pkt(ac->apr, (uint32_t *)&cmd);
	if (rc < 0) {
		pr_err("%s: send_pkt failed. paramid[0x%x]\n", __func__,
			cmd.data.param_id);
		rc = -EINVAL;
		goto fail_cmd;
	}

	return 0;
fail_cmd:
	return rc;
}

int q6asm_set_play_speed(struct audio_client *ac, long *param)
{
	uint32_t sz = 0;
	int rc  = 0;
	struct asm_stream_cmd_set_pp_params_vsp cmd;

	if (ac == NULL) {
		pr_err("%s: audio client is null\n", __func__);
		return -EINVAL;
	}

	sz = sizeof(struct asm_stream_cmd_set_pp_params_vsp);
	q6asm_add_hdr_async_in_adaptation(ac, &cmd.hdr, sz, TRUE);

	cmd.hdr.opcode = ASM_STREAM_CMD_SET_PP_PARAMS_V2;
	cmd.param.data_payload_addr_lsw = 0;
	cmd.param.data_payload_addr_msw = 0;
	cmd.param.mem_map_handle = 0;
	cmd.param.data_payload_size =
			sizeof(cmd) - sizeof(cmd.hdr) - sizeof(cmd.param);
	cmd.data.module_id = ASM_MODULE_ID_PP_SA_VSP;
	cmd.data.param_id = ASM_PARAM_ID_PP_SA_VSP_PARAMS;
	cmd.data.param_size = cmd.param.data_payload_size - sizeof(cmd.data);
	cmd.data.reserved = 0;

	/* play speed paramerters */
	cmd.speed_int = param[0];
	pr_info("%s: %d\n", __func__, cmd.speed_int);

	rc = apr_send_pkt(ac->apr, (uint32_t *)&cmd);
	if (rc < 0) {
		pr_err("%s: send_pkt failed. paramid[0x%x]\n", __func__,
			cmd.data.param_id);
		rc = -EINVAL;
		goto fail_cmd;
	}

	return 0;
fail_cmd:
	return rc;
}

int q6asm_set_adaptation_sound(struct audio_client *ac, long *param)
{
	uint32_t sz = 0;
	int rc  = 0;
	int i = 0;
	struct asm_stream_cmd_set_pp_params_adaptation_sound cmd;

	if (ac == NULL) {
		pr_err("%s: audio client is null\n", __func__);
		return -EINVAL;
	}

	sz = sizeof(struct asm_stream_cmd_set_pp_params_adaptation_sound);
	q6asm_add_hdr_async_in_adaptation(ac, &cmd.hdr, sz, TRUE);

	cmd.hdr.opcode = ASM_STREAM_CMD_SET_PP_PARAMS_V2;
	cmd.param.data_payload_addr_lsw = 0;
	cmd.param.data_payload_addr_msw = 0;
	cmd.param.mem_map_handle = 0;
	cmd.param.data_payload_size =
			sizeof(cmd) - sizeof(cmd.hdr) - sizeof(cmd.param);
	cmd.data.module_id = ASM_MODULE_ID_PP_ADAPTATION_SOUND;
	cmd.data.param_id = ASM_PARAM_ID_PP_ADAPTATION_SOUND_PARAMS;
	cmd.data.param_size = cmd.param.data_payload_size - sizeof(cmd.data);
	cmd.data.reserved = 0;

	/* adapt sound paramerters */
	cmd.enable = param[0];
	for (i = 0; i < 12; i++)
		cmd.gain[i/6][i%6] = param[i+1];
	cmd.device = param[13];
	pr_info("%s: %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
		__func__,
		cmd.enable, cmd.gain[0][0], cmd.gain[0][1], cmd.gain[0][2],
		cmd.gain[0][3], cmd.gain[0][4], cmd.gain[0][5], cmd.gain[1][0],
		cmd.gain[1][1], cmd.gain[1][2], cmd.gain[1][3], cmd.gain[1][4],
		cmd.gain[1][5], cmd.device);

	rc = apr_send_pkt(ac->apr, (uint32_t *)&cmd);
	if (rc < 0) {
		pr_err("%s: send_pkt failed. paramid[0x%x]\n", __func__,
			cmd.data.param_id);
		rc = -EINVAL;
		goto fail_cmd;
	}

	return 0;
fail_cmd:
	return rc;
}

int q6asm_set_sound_balance(struct audio_client *ac, long *param)
{
	uint32_t sz = 0;
	int rc  = 0;
	struct asm_stream_cmd_set_pp_params_lrsm cmd;

	if (ac == NULL) {
		pr_err("%s: audio client is null\n", __func__);
		return -EINVAL;
	}

	sz = sizeof(struct asm_stream_cmd_set_pp_params_lrsm);
	q6asm_add_hdr_async_in_adaptation(ac, &cmd.hdr, sz, TRUE);

	cmd.hdr.opcode = ASM_STREAM_CMD_SET_PP_PARAMS_V2;
	cmd.param.data_payload_addr_lsw = 0;
	cmd.param.data_payload_addr_msw = 0;
	cmd.param.mem_map_handle = 0;
	cmd.param.data_payload_size =
			sizeof(cmd) - sizeof(cmd.hdr) - sizeof(cmd.param);
	cmd.data.module_id = ASM_MODULE_ID_PP_LRSM;
	cmd.data.param_id = ASM_PARAM_ID_PP_LRSM_PARAMS;
	cmd.data.param_size = cmd.param.data_payload_size - sizeof(cmd.data);
	cmd.data.reserved = 0;

	/* sound balance paramerters */
	cmd.sm = param[0];
	cmd.lr = param[1];
	pr_info("%s: %d %d\n", __func__, cmd.sm, cmd.lr);

	rc = apr_send_pkt(ac->apr, (uint32_t *)&cmd);
	if (rc < 0) {
		pr_err("%s: send_pkt failed. paramid[0x%x]\n", __func__,
			cmd.data.param_id);
		rc = -EINVAL;
		goto fail_cmd;
	}

	return 0;
fail_cmd:
	return rc;
}

int q6asm_set_myspace(struct audio_client *ac, long *param)
{
	uint32_t sz = 0;
	int rc  = 0;
	struct asm_stream_cmd_set_pp_params_msp cmd;

	if (ac == NULL) {
		pr_err("%s: audio client is null\n", __func__);
		return -EINVAL;
	}

	sz = sizeof(struct asm_stream_cmd_set_pp_params_msp);
	q6asm_add_hdr_async_in_adaptation(ac, &cmd.hdr, sz, TRUE);

	cmd.hdr.opcode = ASM_STREAM_CMD_SET_PP_PARAMS_V2;
	cmd.param.data_payload_addr_lsw = 0;
	cmd.param.data_payload_addr_msw = 0;
	cmd.param.mem_map_handle = 0;
	cmd.param.data_payload_size =
			sizeof(cmd) - sizeof(cmd.hdr) - sizeof(cmd.param);
	cmd.data.module_id = ASM_MODULE_ID_PP_SA_MSP;
	cmd.data.param_id = ASM_MODULE_ID_PP_SA_MSP_PARAM;
	cmd.data.param_size = cmd.param.data_payload_size - sizeof(cmd.data);
	cmd.data.reserved = 0;

	/* myspace paramerters */
	cmd.msp_int = param[0];
	pr_info("%s: %d\n", __func__, cmd.msp_int);

	rc = apr_send_pkt(ac->apr, (uint32_t *)&cmd);
	if (rc < 0) {
		pr_err("%s: send_pkt failed. paramid[0x%x]\n", __func__,
			cmd.data.param_id);
		rc = -EINVAL;
		goto fail_cmd;
	}

	return 0;
fail_cmd:
	return rc;
}

int q6asm_set_sound_boost(struct audio_client *ac, long *param)
{
	uint32_t sz = 0;
	int rc  = 0;
	struct asm_stream_cmd_set_pp_params_sb cmd;

	if (ac == NULL) {
		pr_err("%s: audio client is null\n", __func__);
		return -EINVAL;
	}

	sz = sizeof(struct asm_stream_cmd_set_pp_params_sb);
	q6asm_add_hdr_async_in_adaptation(ac, &cmd.hdr, sz, TRUE);

	cmd.hdr.opcode = ASM_STREAM_CMD_SET_PP_PARAMS_V2;
	cmd.param.data_payload_addr_lsw = 0;
	cmd.param.data_payload_addr_msw = 0;
	cmd.param.mem_map_handle = 0;
	cmd.param.data_payload_size =
			sizeof(cmd) - sizeof(cmd.hdr) - sizeof(cmd.param);
	cmd.data.module_id = ASM_MODULE_ID_PP_SB;
	cmd.data.param_id = ASM_PARAM_ID_PP_SB_PARAM;
	cmd.data.param_size = cmd.param.data_payload_size - sizeof(cmd.data);
	cmd.data.reserved = 0;

	/* sound booster paramerters */
	cmd.sb_enable = param[0];
	pr_info("%s: %d\n", __func__, cmd.sb_enable);

	rc = apr_send_pkt(ac->apr, (uint32_t *)&cmd);
	if (rc < 0) {
		pr_err("%s: send_pkt failed. paramid[0x%x]\n", __func__,
			cmd.data.param_id);
		rc = -EINVAL;
		goto fail_cmd;
	}

	return 0;
fail_cmd:
	return rc;
}

int q6asm_set_upscaler(struct audio_client *ac, long *param)
{
	uint32_t sz = 0;
	int rc  = 0;
	struct asm_stream_cmd_set_pp_params_upscaler cmd;

	if (ac == NULL) {
		pr_err("%s: audio client is null\n", __func__);
		return -EINVAL;
	}

	sz = sizeof(struct asm_stream_cmd_set_pp_params_upscaler);
	q6asm_add_hdr_async_in_adaptation(ac, &cmd.hdr, sz, TRUE);

	cmd.hdr.opcode = ASM_STREAM_CMD_SET_PP_PARAMS_V2;
	cmd.param.data_payload_addr_lsw = 0;
	cmd.param.data_payload_addr_msw = 0;
	cmd.param.mem_map_handle = 0;
	cmd.param.data_payload_size =
			sizeof(cmd) - sizeof(cmd.hdr) - sizeof(cmd.param);
	cmd.data.module_id = ASM_MODULE_ID_PP_SA_UPSCALER_COLOR;
	cmd.data.param_id = ASM_PARAM_ID_PP_SA_UPSCALER_COLOR_PARAMS;
	cmd.data.param_size = cmd.param.data_payload_size - sizeof(cmd.data);
	cmd.data.reserved = 0;

	/* upscaler paramerters */
	cmd.upscaler_enable = param[0];
	pr_info("%s: %d\n", __func__, cmd.upscaler_enable);

	rc = apr_send_pkt(ac->apr, (uint32_t *)&cmd);
	if (rc < 0) {
		pr_err("%s: send_pkt failed. paramid[0x%x]\n", __func__,
			cmd.data.param_id);
		rc = -EINVAL;
		goto fail_cmd;
	}

	return 0;
fail_cmd:
	return rc;
}

int q6asm_set_sb_rotation(struct audio_client *ac, long *param)
{
	int sz = 0;
	int rc = 0;
	struct asm_stream_cmd_set_pp_params_sb_rotation cmd;

	if(ac == NULL) {
		pr_err("%s: audio client is null\n", __func__);
		return -1;
	}

	sz = sizeof(struct asm_stream_cmd_set_pp_params_sb_rotation);
	q6asm_add_hdr_async_in_adaptation(ac, &cmd.hdr, sz, TRUE);

	cmd.hdr.opcode = ASM_STREAM_CMD_SET_PP_PARAMS_V2;
	cmd.param.data_payload_addr_lsw = 0;
	cmd.param.data_payload_addr_msw = 0;
	cmd.param.mem_map_handle = 0;
	cmd.param.data_payload_size =
			sizeof(cmd) - sizeof(cmd.hdr) - sizeof(cmd.param);
	cmd.data.module_id = ASM_MODULE_ID_PP_SB;
	cmd.data.param_id = ASM_PARAM_ID_PP_SB_ROTATION_PARAM;
	cmd.data.param_size = cmd.param.data_payload_size - sizeof(cmd.data);
	cmd.data.reserved = 0;

	/* rotation paramerters */
	cmd.sb_rotation = (unsigned int)param[0];
	pr_info("%s: %d\n", __func__, cmd.sb_rotation);

	rc = apr_send_pkt(ac->apr, (uint32_t *)&cmd);
	if (rc < 0) {
		pr_err("%s: send_pkt failed. paramid[0x%x]\n", __func__,
			cmd.data.param_id);
		rc = -EINVAL;
		goto fail_cmd;
	}

fail_cmd:
	return rc;
}

static int sec_audio_sound_alive_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_play_speed_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_adaptation_sound_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_sound_balance_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_voice_tracking_info_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{

	int rc = 0;
	int be_idx = 0;
	char *param_value;
	int *update_param_value;
	struct msm_pcm_routing_bdai_data msm_bedai;
	uint32_t param_length = sizeof(uint32_t);
	uint32_t param_payload_len = 4 * sizeof(uint32_t);

	param_value = kzalloc(param_length, GFP_KERNEL);
	if (!param_value) {
		pr_err("%s, param memory alloc failed\n", __func__);
		return -ENOMEM;
	}

	for (be_idx = 0; be_idx < MSM_BACKEND_DAI_MAX; be_idx++) {
		msm_pcm_routing_get_bedai_info(be_idx, &msm_bedai);
		if (msm_bedai.port_id == afe_port.voice_tracking_id)
			break;
	}
	if ((be_idx < MSM_BACKEND_DAI_MAX) && msm_bedai.active) {
		rc = adm_get_params(afe_port.voice_tracking_id, 0,
				ADM_MODULE_ID_PP_SS_REC,
				ADM_PARAM_ID_PP_SS_REC_GETPARAMS,
				param_length + param_payload_len,
				param_value);
		if (rc) {
			pr_err("%s: get parameters failed:%d\n", __func__, rc);
			kfree(param_value);
			return -EINVAL;
		}
		update_param_value = (int *)param_value;
		ucontrol->value.integer.value[0] = update_param_value[0];

		pr_debug("%s: FROM DSP value[0] 0x%x\n",
			  __func__, update_param_value[0]);
	}
	kfree(param_value);

	return 0;
}

static int sec_audio_myspace_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_sound_boost_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_upscaler_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_sb_rotation_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_audio_sound_alive_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	struct msm_pcm_routing_fdai_data fe_dai_map;
	struct audio_client *ac;

	mutex_lock(&asm_lock);
	msm_pcm_routing_get_fedai_info(SEC_ADAPTATAION_AUDIO_PORT,
			SESSION_TYPE_RX, &fe_dai_map);
	ac = q6asm_get_audio_client(fe_dai_map.strm_id);
	ret = q6asm_set_sound_alive(ac,
		(long *)ucontrol->value.integer.value);
	mutex_unlock(&asm_lock);

	return ret;
}

static int sec_audio_play_speed_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	struct msm_pcm_routing_fdai_data fe_dai_map;
	struct audio_client *ac;

	mutex_lock(&asm_lock);
	msm_pcm_routing_get_fedai_info(SEC_ADAPTATAION_AUDIO_PORT,
			SESSION_TYPE_RX, &fe_dai_map);
	ac = q6asm_get_audio_client(fe_dai_map.strm_id);
	ret = q6asm_set_play_speed(ac,
		(long *)ucontrol->value.integer.value);
	mutex_unlock(&asm_lock);

	return ret;
}

static int sec_audio_adaptation_sound_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	struct msm_pcm_routing_fdai_data fe_dai_map;
	struct audio_client *ac;

	mutex_lock(&asm_lock);
	msm_pcm_routing_get_fedai_info(SEC_ADAPTATAION_AUDIO_PORT,
			SESSION_TYPE_RX, &fe_dai_map);
	ac = q6asm_get_audio_client(fe_dai_map.strm_id);
	ret = q6asm_set_adaptation_sound(ac,
		(long *)ucontrol->value.integer.value);
	mutex_unlock(&asm_lock);
	return ret;
}

static int sec_audio_sound_balance_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	struct msm_pcm_routing_fdai_data fe_dai_map;
	struct audio_client *ac;

	mutex_lock(&asm_lock);
	msm_pcm_routing_get_fedai_info(SEC_ADAPTATAION_AUDIO_PORT,
			SESSION_TYPE_RX, &fe_dai_map);
	ac = q6asm_get_audio_client(fe_dai_map.strm_id);
	ret = q6asm_set_sound_balance(ac,
		(long *)ucontrol->value.integer.value);
	mutex_unlock(&asm_lock);
	return ret;
}

static int sec_audio_voice_tracking_info_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	return ret;
}

static int sec_audio_myspace_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	struct msm_pcm_routing_fdai_data fe_dai_map;
	struct audio_client *ac;

	mutex_lock(&asm_lock);
	msm_pcm_routing_get_fedai_info(SEC_ADAPTATAION_AUDIO_PORT,
			SESSION_TYPE_RX, &fe_dai_map);
	ac = q6asm_get_audio_client(fe_dai_map.strm_id);
	ret = q6asm_set_myspace(ac, (long *)ucontrol->value.integer.value);
	mutex_unlock(&asm_lock);
	return ret;
}

static int sec_audio_sound_boost_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	struct msm_pcm_routing_fdai_data fe_dai_map;
	struct audio_client *ac;

	mutex_lock(&asm_lock);
	msm_pcm_routing_get_fedai_info(SEC_ADAPTATAION_AUDIO_PORT,
			SESSION_TYPE_RX, &fe_dai_map);
	ac = q6asm_get_audio_client(fe_dai_map.strm_id);
	ret = q6asm_set_sound_boost(ac, (long *)ucontrol->value.integer.value);
	mutex_unlock(&asm_lock);

	return ret;
}

static int sec_audio_upscaler_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	struct msm_pcm_routing_fdai_data fe_dai_map;
	struct audio_client *ac;

	mutex_lock(&asm_lock);
	msm_pcm_routing_get_fedai_info(SEC_ADAPTATAION_AUDIO_PORT,
			SESSION_TYPE_RX, &fe_dai_map);
	ac = q6asm_get_audio_client(fe_dai_map.strm_id);
	ret = q6asm_set_upscaler(ac, (long *)ucontrol->value.integer.value);
	mutex_unlock(&asm_lock);

	return ret;
}

static int sec_audio_sb_rotation_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	struct msm_pcm_routing_fdai_data fe_dai_map;
	struct audio_client *ac;

	mutex_lock(&asm_lock);
	msm_pcm_routing_get_fedai_info(SEC_ADAPTATAION_AUDIO_PORT,
			SESSION_TYPE_RX, &fe_dai_map);
	ac = q6asm_get_audio_client(fe_dai_map.strm_id);
	ret = q6asm_set_sb_rotation(ac, (long *)ucontrol->value.integer.value);
	mutex_unlock(&asm_lock);

	return ret;
}

/****************************************************************************/
/*//////////////////////////// VOICE SOLUTION //////////////////////////////*/
/****************************************************************************/
/*//////////////////////////// COMMON COMMAND //////////////////////////////*/
/* This function must be sync up from voice_get_session_by_idx() of q6voice.c */
static struct voice_data *voice_get_session_by_idx(int idx)
{
	return ((idx < 0 || idx >= MAX_VOC_SESSIONS) ?
				NULL : &common->voice[idx]);
}

/* This function must be sync up */
/* from voice_itr_get_next_session() */
/* of q6voice.c */
static bool voice_itr_get_next_session(struct voice_session_itr *itr,
					struct voice_data **voice)
{
	bool ret = false;

	if (itr == NULL)
		return false;
	pr_debug("%s : cur idx = %d session idx = %d\n",
			 __func__, itr->cur_idx, itr->session_idx);

	if (itr->cur_idx <= itr->session_idx) {
		ret = true;
		*voice = voice_get_session_by_idx(itr->cur_idx);
		itr->cur_idx++;
	} else {
		*voice = NULL;
	}

	return ret;
}

/* This function must be sync up from voice_itr_init() of q6voice.c */
static void voice_itr_init(struct voice_session_itr *itr,
			   u32 session_id)
{
	if (itr == NULL)
		return;
	itr->session_idx = voice_get_idx_for_session(session_id);
	if (session_id == ALL_SESSION_VSID)
		itr->cur_idx = 0;
	else
		itr->cur_idx = itr->session_idx;
}

/* This function must be sync up from is_voc_state_active() of q6voice.c */
static bool is_voc_state_active(int voc_state)
{
	if ((voc_state == VOC_RUN) ||
		(voc_state == VOC_CHANGE) ||
		(voc_state == VOC_STANDBY))
		return true;

	return false;
}

/* This function must be sync up from voc_set_error_state() of q6voice.c */
static void voc_set_error_state(uint16_t reset_proc)
{
	struct voice_data *v = NULL;
	int i;

	for (i = 0; i < MAX_VOC_SESSIONS; i++) {
		v = &common->voice[i];
		if (v != NULL) {
			v->voc_state = VOC_ERROR;
			v->rec_info.recording = 0;
		}
	}
}

/* This function must be sync up from */
/* is_source_tracking_shared_memomry_allocated() */
/* of q6voice.c */
static int is_source_tracking_shared_memomry_allocated(void)
{
	bool ret;

	pr_debug("%s: Enter\n", __func__);

	if (common->source_tracking_sh_mem.sh_mem_block.client != NULL &&
	    common->source_tracking_sh_mem.sh_mem_block.handle != NULL)
		ret = true;
	else
		ret = false;

	pr_debug("%s: Exit\n", __func__);

	return ret;
}

/*//////////////////////////// CVS COMMAND //////////////////////////////*/
/* This function must be sync up from voice_get_cvs_handle() of q6voice.c */
static u16 voice_get_cvs_handle(struct voice_data *v)
{
	if (v == NULL) {
		pr_err("%s: v is NULL\n", __func__);
		return 0;
	}

	pr_debug("%s: cvs_handle %d\n", __func__, v->cvs_handle);

	return v->cvs_handle;
}

static int32_t q6audio_adaptation_cvs_callback(struct apr_client_data *data,
	void *priv)
{
	int i;
	uint32_t *ptr = NULL;

	if ((data == NULL) || (priv == NULL)) {
		pr_err("%s: data or priv is NULL\n", __func__);
		return -EINVAL;
	}

	if (data->opcode == RESET_EVENTS) {
		pr_debug("%s: reset event = %d %d apr[%pK]\n",
			__func__,
			data->reset_event, data->reset_proc, this_cvs.apr);

		if (this_cvs.apr) {
			apr_reset(this_cvs.apr);
			atomic_set(&this_cvs.state, 0);
			this_cvs.apr = NULL;
		}

		/* Sub-system restart is applicable to all sessions. */
		for (i = 0; i < MAX_VOC_SESSIONS; i++)
			common->voice[i].cvs_handle = 0;

		cal_utils_clear_cal_block_q6maps(MAX_VOICE_CAL_TYPES,
				common->cal_data);

		/* Free the ION memory and clear handles for Source Tracking */
		if (is_source_tracking_shared_memomry_allocated()) {
			msm_audio_ion_free(
			common->source_tracking_sh_mem.sh_mem_block.client,
			common->source_tracking_sh_mem.sh_mem_block.handle);
			common->source_tracking_sh_mem.mem_handle = 0;
			common->source_tracking_sh_mem.sh_mem_block.client =
									NULL;
			common->source_tracking_sh_mem.sh_mem_block.handle =
									NULL;
		}

		voc_set_error_state(data->reset_proc);
		return 0;
	}

	if (data->opcode == APR_BASIC_RSP_RESULT) {
		if (data->payload_size) {
			ptr = data->payload;
			pr_debug("%x %x\n", ptr[0], ptr[1]);
			if (ptr[1] != 0) {
				pr_err("%s: cmd = 0x%x returned error = 0x%x\n",
					__func__, ptr[0], ptr[1]);
			}

			switch (ptr[0]) {
			case VOICE_CMD_SET_PARAM:
				pr_info("%s: VOICE_CMD_SET_PARAM\n",
					__func__);
				atomic_set(&this_cvs.state, 0);
				wake_up(&this_cvs.wait);
				break;
			default:
				pr_err("%s: cmd = 0x%x\n", __func__, ptr[0]);
				break;
			}
		}
	}
	return 0;
}

static int send_packet_loopback_cmd(struct voice_data *v, int mode)
{
	struct cvs_set_loopback_enable_cmd cvs_set_loopback_cmd;
	int ret = 0;
	u16 cvs_handle;

	if (this_cvs.apr == NULL) {
		this_cvs.apr = apr_register("ADSP", "CVS",
					q6audio_adaptation_cvs_callback,
					SEC_ADAPTATAION_LOOPBACK_SRC_PORT,
					&this_cvs);
	}
	cvs_handle = voice_get_cvs_handle(v);

	/* fill in the header */
	cvs_set_loopback_cmd.hdr.hdr_field =
	APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	cvs_set_loopback_cmd.hdr.pkt_size =
		APR_PKT_SIZE(APR_HDR_SIZE,
				sizeof(cvs_set_loopback_cmd) - APR_HDR_SIZE);
	cvs_set_loopback_cmd.hdr.src_port =
		SEC_ADAPTATAION_LOOPBACK_SRC_PORT;
	cvs_set_loopback_cmd.hdr.dest_port = cvs_handle;
	cvs_set_loopback_cmd.hdr.token = 0;
	cvs_set_loopback_cmd.hdr.opcode =
		VOICE_CMD_SET_PARAM;

	cvs_set_loopback_cmd.mem_handle = 0;
	cvs_set_loopback_cmd.mem_address_lsw = 0;
	cvs_set_loopback_cmd.mem_address_msw = 0;
	cvs_set_loopback_cmd.mem_size = 0x10;

	cvs_set_loopback_cmd.vss_set_loopback.module_id =
		VOICEPROC_MODULE_VENC;
	cvs_set_loopback_cmd.vss_set_loopback.param_id =
		VOICE_PARAM_LOOPBACK_ENABLE;
	cvs_set_loopback_cmd.vss_set_loopback.param_size =
		MOD_ENABLE_PARAM_LEN;
	cvs_set_loopback_cmd.vss_set_loopback.reserved = 0;
	cvs_set_loopback_cmd.vss_set_loopback.loopback_enable = mode;
	cvs_set_loopback_cmd.vss_set_loopback.reserved_field = 0;

	atomic_set(&this_cvs.state, 1);
	ret = apr_send_pkt(this_cvs.apr, (uint32_t *) &cvs_set_loopback_cmd);
	if (ret < 0) {
		pr_err("%s: sending cvs set loopback enable failed\n",
			__func__);
		goto fail;
	}
	ret = wait_event_timeout(this_cvs.wait,
		(atomic_read(&this_cvs.state) == 0),
			msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}
	return 0;
fail:
	return -EINVAL;
}

int voice_sec_set_loopback_cmd(u32 session_id, uint16_t mode)
{
	struct voice_data *v = NULL;
	int ret = 0;
	struct voice_session_itr itr;

	pr_debug("%s: Enter\n", __func__);

	voice_itr_init(&itr, session_id);
	while (voice_itr_get_next_session(&itr, &v)) {
		if (v != NULL) {
			ret = send_packet_loopback_cmd(v, mode);
		} else {
			pr_err("%s: invalid session\n", __func__);
			ret = -EINVAL;
			break;
		}
	}
	pr_debug("%s: Exit, ret=%d\n", __func__, ret);

	return ret;
}

void voice_sec_loopback_start_cmd(u32 session_id)
{
	int ret = 0;

	if (loopback_mode == LOOPBACK_ENABLE ||
	    loopback_mode == LOOPBACK_NODELAY) {
		ret = voice_sec_set_loopback_cmd(session_id, loopback_mode);
		if (ret < 0) {
			pr_err("%s: send packet loopback cmd failed(%d)\n",
				__func__, ret);
		} else {
			pr_info("%s: enable packet loopback\n",
				__func__);
		}
	}
}

void voice_sec_loopback_end_cmd(u32 session_id)
{
	int ret = 0;

	if ((loopback_mode == LOOPBACK_DISABLE) &&
	    (loopback_prev_mode == LOOPBACK_ENABLE ||
	     loopback_prev_mode == LOOPBACK_NODELAY)) {
		ret = voice_sec_set_loopback_cmd(session_id, loopback_mode);
		if (ret < 0) {
			pr_err("%s: packet loopback disable cmd failed(%d)\n",
				__func__, ret);
		} else {
			pr_info("%s: disable packet loopback\n",
				__func__);
		}
		loopback_prev_mode = 0;
	}
}

/*//////////////////////////// CVP COMMAND //////////////////////////////*/
/* This function must be sync up from voice_get_cvp_handle() of q6voice.c */
static u16 voice_get_cvp_handle(struct voice_data *v)
{
	if (v == NULL) {
		pr_err("%s: v is NULL\n", __func__);
		return 0;
	}

	pr_debug("%s: cvp_handle %d\n", __func__, v->cvp_handle);

	return v->cvp_handle;
}

static int32_t q6audio_adaptation_cvp_callback(struct apr_client_data *data,
	void *priv)
{
	int i;
	uint32_t *ptr = NULL;

	if ((data == NULL) || (priv == NULL)) {
		pr_err("%s: data or priv is NULL\n", __func__);
		return -EINVAL;
	}

	if (data->opcode == RESET_EVENTS) {
		pr_debug("%s: reset event = %d %d apr[%pK]\n",
			__func__,
			data->reset_event, data->reset_proc, this_cvp.apr);

		if (this_cvp.apr) {
			apr_reset(this_cvp.apr);
			atomic_set(&this_cvp.state, 0);
			this_cvp.apr = NULL;
		}

		/* Sub-system restart is applicable to all sessions. */
		for (i = 0; i < MAX_VOC_SESSIONS; i++)
			common->voice[i].cvp_handle = 0;

		cal_utils_clear_cal_block_q6maps(MAX_VOICE_CAL_TYPES,
				common->cal_data);

		/* Free the ION memory and clear handles for Source Tracking */
		if (is_source_tracking_shared_memomry_allocated()) {
			msm_audio_ion_free(
			common->source_tracking_sh_mem.sh_mem_block.client,
			common->source_tracking_sh_mem.sh_mem_block.handle);
			common->source_tracking_sh_mem.mem_handle = 0;
			common->source_tracking_sh_mem.sh_mem_block.client =
									NULL;
			common->source_tracking_sh_mem.sh_mem_block.handle =
									NULL;
		}

		voc_set_error_state(data->reset_proc);

		return 0;
	}

	if (data->opcode == APR_BASIC_RSP_RESULT) {
		if (data->payload_size) {
			ptr = data->payload;
			pr_debug("%x %x\n", ptr[0], ptr[1]);
			if (ptr[1] != 0) {
				pr_err("%s: cmd = 0x%x returned error = 0x%x\n",
					__func__, ptr[0], ptr[1]);
			}

			switch (ptr[0]) {
			case VOICE_CMD_SET_PARAM:
				pr_info("%s: VOICE_CMD_SET_PARAM\n",
					__func__);
				atomic_set(&this_cvp.state, 0);
				wake_up(&this_cvp.wait);
				break;
			case VSS_ICOMMON_CMD_SET_UI_PROPERTY:
				pr_info("%s: VSS_ICOMMON_CMD_SET_UI_PROPERTY\n",
					__func__);
				atomic_set(&this_cvp.state, 0);
				wake_up(&this_cvp.wait);
				break;
			default:
				pr_err("%s: cmd = 0x%x\n", __func__, ptr[0]);
				break;
			}
		}
	}
	return 0;
}

static int sec_voice_send_adaptation_sound_cmd(struct voice_data *v,
			uint16_t mode, uint16_t select, int16_t *parameters)
{
	struct cvp_adaptation_sound_parm_send_cmd
		cvp_adaptation_sound_param_cmd;
	int ret = 0;
	u16 cvp_handle;

	if (v == NULL) {
		pr_err("%s: v is NULL\n", __func__);
		return -EINVAL;
	}

	if (this_cvp.apr == NULL) {
		this_cvp.apr = apr_register("ADSP", "CVP",
					q6audio_adaptation_cvp_callback,
					SEC_ADAPTATAION_VOICE_SRC_PORT,
					&this_cvp);
	}
	cvp_handle = voice_get_cvp_handle(v);

	/* fill in the header */
	cvp_adaptation_sound_param_cmd.hdr.hdr_field =
				APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE),
				APR_PKT_VER);
	cvp_adaptation_sound_param_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
		sizeof(cvp_adaptation_sound_param_cmd) - APR_HDR_SIZE);
	cvp_adaptation_sound_param_cmd.hdr.src_port =
		SEC_ADAPTATAION_VOICE_SRC_PORT;
	cvp_adaptation_sound_param_cmd.hdr.dest_port = cvp_handle;
	cvp_adaptation_sound_param_cmd.hdr.token = 0;
	cvp_adaptation_sound_param_cmd.hdr.opcode =
		VOICE_CMD_SET_PARAM;
	cvp_adaptation_sound_param_cmd.mem_handle = 0;
	cvp_adaptation_sound_param_cmd.mem_address_lsw = 0;
	cvp_adaptation_sound_param_cmd.mem_address_msw = 0;
	cvp_adaptation_sound_param_cmd.mem_size = 40;
	cvp_adaptation_sound_param_cmd.adaptation_sound_data.module_id =
		VOICE_VOICEMODE_MODULE;
	cvp_adaptation_sound_param_cmd.adaptation_sound_data.param_id =
		VOICE_ADAPTATION_SOUND_PARAM;
	cvp_adaptation_sound_param_cmd.adaptation_sound_data.param_size = 28;
	cvp_adaptation_sound_param_cmd.adaptation_sound_data.reserved = 0;
	cvp_adaptation_sound_param_cmd.adaptation_sound_data.eq_mode = mode;
	cvp_adaptation_sound_param_cmd.adaptation_sound_data.select = select;

	memcpy(cvp_adaptation_sound_param_cmd.adaptation_sound_data.param,
		parameters, sizeof(int16_t)*12);

	pr_info("%s: send adaptation_sound param, mode = %d, select=%d\n",
		__func__,
		cvp_adaptation_sound_param_cmd.adaptation_sound_data.eq_mode,
		cvp_adaptation_sound_param_cmd.adaptation_sound_data.select);

	atomic_set(&this_cvp.state, 1);
	ret = apr_send_pkt(this_cvp.apr,
		(uint32_t *) &cvp_adaptation_sound_param_cmd);
	if (ret < 0) {
		pr_err("%s: Failed to send cvp_adaptation_sound_param_cmd\n",
			__func__);
		return -EINVAL;
	}

	ret = wait_event_timeout(this_cvp.wait,
				(atomic_read(&this_cvp.state) == 0),
				msecs_to_jiffies(TIMEOUT_MS));

	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		return -EINVAL;
	}

	return 0;
}

int sec_voice_set_adaptation_sound(uint16_t mode,
	uint16_t select, int16_t *parameters)
{
	struct voice_data *v = NULL;
	int ret = 0;
	struct voice_session_itr itr;

	pr_debug("%s: Enter\n", __func__);

	mutex_lock(&common->common_lock);
	voice_itr_init(&itr, ALL_SESSION_VSID);
	while (voice_itr_get_next_session(&itr, &v)) {
		if (v != NULL) {
			mutex_lock(&v->lock);
			if (is_voc_state_active(v->voc_state) &&
				(v->lch_mode != VOICE_LCH_START) &&
				!v->disable_topology)
				ret = sec_voice_send_adaptation_sound_cmd(v,
					mode, select, parameters);
			mutex_unlock(&v->lock);
		} else {
			pr_err("%s: invalid session\n", __func__);
			ret = -EINVAL;
			break;
		}
	}
	mutex_unlock(&common->common_lock);
	pr_debug("%s: Exit, ret=%d\n", __func__, ret);

	return ret;
}

static int sec_voice_send_nb_mode_cmd(struct voice_data *v, int enable)
{
	struct cvp_set_nbmode_enable_cmd cvp_nbmode_cmd;
	int ret = 0;
	u16 cvp_handle;

	if (v == NULL) {
		pr_err("%s: v is NULL\n", __func__);
		return -EINVAL;
	}

	if (this_cvp.apr == NULL) {
		this_cvp.apr = apr_register("ADSP", "CVP",
					q6audio_adaptation_cvp_callback,
					SEC_ADAPTATAION_VOICE_SRC_PORT,
					&this_cvp);
	}
	cvp_handle = voice_get_cvp_handle(v);

	/* fill in the header */
	cvp_nbmode_cmd.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE),
				APR_PKT_VER);
	cvp_nbmode_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
		sizeof(cvp_nbmode_cmd) - APR_HDR_SIZE);
	cvp_nbmode_cmd.hdr.src_port =
		SEC_ADAPTATAION_VOICE_SRC_PORT;
	cvp_nbmode_cmd.hdr.dest_port = cvp_handle;
	cvp_nbmode_cmd.hdr.token = 0;
	cvp_nbmode_cmd.hdr.opcode =
		VSS_ICOMMON_CMD_SET_UI_PROPERTY;
	cvp_nbmode_cmd.cvp_set_nbmode.module_id =
		VOICE_VOICEMODE_MODULE;
	cvp_nbmode_cmd.cvp_set_nbmode.param_id =
		VOICE_NBMODE_PARAM;
	cvp_nbmode_cmd.cvp_set_nbmode.param_size =
		MOD_ENABLE_PARAM_LEN;
	cvp_nbmode_cmd.cvp_set_nbmode.reserved = 0;
	cvp_nbmode_cmd.cvp_set_nbmode.enable = enable;
	cvp_nbmode_cmd.cvp_set_nbmode.reserved_field = 0;

	pr_info("%s: eanble = %d\n", __func__,
					cvp_nbmode_cmd.cvp_set_nbmode.enable);

	atomic_set(&this_cvp.state, 1);
	ret = apr_send_pkt(this_cvp.apr, (uint32_t *) &cvp_nbmode_cmd);
	if (ret < 0) {
		pr_err("%s: Failed to send cvp_nbmode_cmd\n",
			__func__);
		goto fail;
	}

	ret = wait_event_timeout(this_cvp.wait,
				(atomic_read(&this_cvp.state) == 0),
				msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}
	return 0;

fail:
	return ret;
}

int sec_voice_set_nb_mode(short enable)
{
	struct voice_data *v = NULL;
	int ret = 0;
	struct voice_session_itr itr;

	pr_debug("%s: Enter\n", __func__);

	mutex_lock(&common->common_lock);
	voice_itr_init(&itr, ALL_SESSION_VSID);
	while (voice_itr_get_next_session(&itr, &v)) {
		if (v != NULL) {
			mutex_lock(&v->lock);
			if (is_voc_state_active(v->voc_state) &&
				(v->lch_mode != VOICE_LCH_START) &&
				!v->disable_topology)
				ret = sec_voice_send_nb_mode_cmd(v, enable);
			mutex_unlock(&v->lock);
		} else {
			pr_err("%s: invalid session\n", __func__);
			ret = -EINVAL;
			break;
		}
	}
	mutex_unlock(&common->common_lock);
	pr_debug("%s: Exit, ret=%d\n", __func__, ret);

	return ret;
}

static int sec_voice_send_spk_mode_cmd(struct voice_data *v, int enable)
{
	struct cvp_set_spkmode_enable_cmd cvp_spkmode_cmd;
	int ret = 0;
	u16 cvp_handle;

	if (v == NULL) {
		pr_err("%s: v is NULL\n", __func__);
		return -EINVAL;
	}

	if (this_cvp.apr == NULL) {
		this_cvp.apr = apr_register("ADSP", "CVP",
					q6audio_adaptation_cvp_callback,
					SEC_ADAPTATAION_VOICE_SRC_PORT,
					&this_cvp);
	}
	cvp_handle = voice_get_cvp_handle(v);

	/* fill in the header */
	cvp_spkmode_cmd.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE),
				APR_PKT_VER);
	cvp_spkmode_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
		sizeof(cvp_spkmode_cmd) - APR_HDR_SIZE);
	cvp_spkmode_cmd.hdr.src_port = SEC_ADAPTATAION_VOICE_SRC_PORT;
	cvp_spkmode_cmd.hdr.dest_port = cvp_handle;
	cvp_spkmode_cmd.hdr.token = 0;
	cvp_spkmode_cmd.hdr.opcode = VSS_ICOMMON_CMD_SET_UI_PROPERTY;
	cvp_spkmode_cmd.cvp_set_spkmode.module_id = VOICE_VOICEMODE_MODULE;
	cvp_spkmode_cmd.cvp_set_spkmode.param_id = VOICE_SPKMODE_PARAM;
	cvp_spkmode_cmd.cvp_set_spkmode.param_size = MOD_ENABLE_PARAM_LEN;
	cvp_spkmode_cmd.cvp_set_spkmode.reserved = 0;
	cvp_spkmode_cmd.cvp_set_spkmode.enable = enable;
	cvp_spkmode_cmd.cvp_set_spkmode.reserved_field = 0;

	pr_info("%s: Voice Module enable = %d\n", __func__,
					cvp_spkmode_cmd.cvp_set_spkmode.enable);

	atomic_set(&this_cvp.state, 1);
	ret = apr_send_pkt(this_cvp.apr, (uint32_t *) &cvp_spkmode_cmd);
	if (ret < 0) {
		pr_err("%s: Failed to send cvp_spkmode_cmd\n", __func__);
		goto fail;
	}

	ret = wait_event_timeout(this_cvp.wait,
				(atomic_read(&this_cvp.state) == 0),
				msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}

	cvp_spkmode_cmd.cvp_set_spkmode.module_id = VOICE_FVSAM_MODULE;

	pr_info("%s: FVSAM Module enable = %d\n", __func__,
					cvp_spkmode_cmd.cvp_set_spkmode.enable);

	atomic_set(&this_cvp.state, 1);
	ret = apr_send_pkt(this_cvp.apr, (uint32_t *) &cvp_spkmode_cmd);
	if (ret < 0) {
		pr_err("%s: Failed to send cvp_spkmode_cmd\n", __func__);
		goto fail;
	}

	ret = wait_event_timeout(this_cvp.wait,
				(atomic_read(&this_cvp.state) == 0),
				msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}

	cvp_spkmode_cmd.cvp_set_spkmode.module_id = VOICE_WISEVOICE_MODULE;

	pr_info("%s: WiseVoice Module enable = %d\n", __func__,
					cvp_spkmode_cmd.cvp_set_spkmode.enable);

	atomic_set(&this_cvp.state, 1);
	ret = apr_send_pkt(this_cvp.apr, (uint32_t *) &cvp_spkmode_cmd);
	if (ret < 0) {
		pr_err("%s: Failed to send cvp_spkmode_cmd\n", __func__);
		goto fail;
	}

	ret = wait_event_timeout(this_cvp.wait,
				(atomic_read(&this_cvp.state) == 0),
				msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);

		goto fail;
	}
	return 0;

fail:
	return ret;
}

int sec_voice_set_spk_mode(short enable)
{
	struct voice_data *v = NULL;
	int ret = 0;
	struct voice_session_itr itr;

	pr_debug("%s: Enter\n", __func__);

	mutex_lock(&common->common_lock);
	voice_itr_init(&itr, ALL_SESSION_VSID);
	while (voice_itr_get_next_session(&itr, &v)) {
		if (v != NULL) {
			mutex_lock(&v->lock);
			if (is_voc_state_active(v->voc_state) &&
				(v->lch_mode != VOICE_LCH_START) &&
				!v->disable_topology)
				ret = sec_voice_send_spk_mode_cmd(v, enable);
			mutex_unlock(&v->lock);
		} else {
			pr_err("%s: invalid session\n", __func__);
			ret = -EINVAL;
			break;
		}
	}
	mutex_unlock(&common->common_lock);
	pr_debug("%s: Exit, ret=%d\n", __func__, ret);

	return ret;
}

static int sec_voice_send_device_info_cmd(struct voice_data *v, int device)
{
	struct cvp_set_device_info_cmd cvp_device_info_cmd;
	int ret = 0;
	u16 cvp_handle;

	if (v == NULL) {
		pr_err("%s: v is NULL\n", __func__);
		return -EINVAL;
	}

	if (this_cvp.apr == NULL) {
		this_cvp.apr = apr_register("ADSP", "CVP",
					q6audio_adaptation_cvp_callback,
					SEC_ADAPTATAION_VOICE_SRC_PORT,
					&this_cvp);
	}
	cvp_handle = voice_get_cvp_handle(v);

	/* fill in the header */
	cvp_device_info_cmd.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE),
				APR_PKT_VER);
	cvp_device_info_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
		sizeof(cvp_device_info_cmd) - APR_HDR_SIZE);
	cvp_device_info_cmd.hdr.src_port = SEC_ADAPTATAION_VOICE_SRC_PORT;
	cvp_device_info_cmd.hdr.dest_port = cvp_handle;
	cvp_device_info_cmd.hdr.token = 0;
	cvp_device_info_cmd.hdr.opcode =
		VSS_ICOMMON_CMD_SET_UI_PROPERTY;
	cvp_device_info_cmd.cvp_set_device_info.module_id =
		VOICE_MODULE_SET_DEVICE;
	cvp_device_info_cmd.cvp_set_device_info.param_id =
		VOICE_MODULE_SET_DEVICE_PARAM;
	cvp_device_info_cmd.cvp_set_device_info.param_size =
		MOD_ENABLE_PARAM_LEN;
	cvp_device_info_cmd.cvp_set_device_info.reserved = 0;
	cvp_device_info_cmd.cvp_set_device_info.enable = device;
	cvp_device_info_cmd.cvp_set_device_info.reserved_field = 0;

	pr_info("%s(): voice device info = %d\n",
		__func__, cvp_device_info_cmd.cvp_set_device_info.enable);

	atomic_set(&this_cvp.state, 1);
	ret = apr_send_pkt(this_cvp.apr, (uint32_t *) &cvp_device_info_cmd);
	if (ret < 0) {
		pr_err("%s: Failed to send cvp_set_device_info_cmd\n",
			__func__);
		goto fail;
	}

	ret = wait_event_timeout(this_cvp.wait,
				(atomic_read(&this_cvp.state) == 0),
				msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}
	return 0;

fail:
	return ret;
}

int sec_voice_set_device_info(short device)
{
	struct voice_data *v = NULL;
	int ret = 0;
	struct voice_session_itr itr;

	pr_debug("%s: Enter\n", __func__);

	mutex_lock(&common->common_lock);
	voice_itr_init(&itr, ALL_SESSION_VSID);
	while (voice_itr_get_next_session(&itr, &v)) {
		if (v != NULL) {
			mutex_lock(&v->lock);
			if (is_voc_state_active(v->voc_state) &&
				(v->lch_mode != VOICE_LCH_START) &&
				!v->disable_topology)
				ret = sec_voice_send_device_info_cmd(v, device);
			mutex_unlock(&v->lock);
		} else {
			pr_err("%s: invalid session\n", __func__);
			ret = -EINVAL;
			break;
		}
	}
	mutex_unlock(&common->common_lock);
	pr_debug("%s: Exit, ret=%d\n", __func__, ret);

	return ret;
}

int sec_voice_get_loopback_enable(void)
{
	return loopback_mode;
}

void sec_voice_set_loopback_enable(int mode)
{
	loopback_prev_mode = loopback_mode;

	if (mode >= LOOPBACK_MAX ||
	    mode < LOOPBACK_DISABLE) {
		pr_err("%s : out of range, mode = %d\n",
			__func__, mode);
		loopback_mode = LOOPBACK_DISABLE;
	} else {
		loopback_mode = mode;
	}

	pr_info("%s : prev_mode = %d, mode = %d\n",
		__func__,
		loopback_prev_mode,
		loopback_mode);
}

static int sec_voice_adaptation_sound_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_voice_adaptation_sound_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int i = 0;

	int adaptation_sound_mode = ucontrol->value.integer.value[0];
	int adaptation_sound_select = ucontrol->value.integer.value[1];
	short adaptation_sound_param[12] = {0,};

	for (i = 0; i < 12; i++) {
		adaptation_sound_param[i] =
			(short)ucontrol->value.integer.value[2+i];
		pr_debug("sec_voice_adaptation_sound_put : param - %d\n",
				adaptation_sound_param[i]);
	}

	return sec_voice_set_adaptation_sound(adaptation_sound_mode,
				adaptation_sound_select,
				adaptation_sound_param);
}

static int sec_voice_nb_mode_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_voice_nb_mode_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int enable = ucontrol->value.integer.value[0];

	pr_debug("%s: enable=%d\n", __func__, enable);

	return sec_voice_set_nb_mode(enable);
}

static int sec_voice_spk_mode_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_voice_spk_mode_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int enable = ucontrol->value.integer.value[0];

	pr_debug("%s: enable=%d\n", __func__, enable);

	return sec_voice_set_spk_mode(enable);
}

static int sec_voice_loopback_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int loopback_enable = ucontrol->value.integer.value[0];

	pr_debug("%s: loopback enable=%d\n", __func__, loopback_enable);

	sec_voice_set_loopback_enable(loopback_enable);
	return 0;
}

static int sec_voice_loopback_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = sec_voice_get_loopback_enable();
	return 0;
}

static const char * const voice_device[] = {
	"ETC", "SPK", "EAR", "BT", "RCV"
};

static const struct soc_enum sec_voice_device_enum[] = {
	SOC_ENUM_SINGLE_EXT(5, voice_device),
};

static int sec_voice_dev_info_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int sec_voice_dev_info_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int voc_dev = ucontrol->value.integer.value[0];

	pr_debug("%s: voice device=%d\n", __func__, voc_dev);

	return sec_voice_set_device_info(voc_dev);
}
/*******************************************************/
/*/////////////////////// COMMON //////////////////////*/
/*******************************************************/
static const struct snd_kcontrol_new samsung_solution_mixer_controls[] = {
	SOC_SINGLE_MULTI_EXT("SA data", SND_SOC_NOPM, 0, 65535, 0, 27,
				sec_audio_sound_alive_get,
				sec_audio_sound_alive_put),
	SOC_SINGLE_MULTI_EXT("VSP data", SND_SOC_NOPM, 0, 65535, 0, 1,
				sec_audio_play_speed_get,
				sec_audio_play_speed_put),
	SOC_SINGLE_MULTI_EXT("Audio DHA data", SND_SOC_NOPM, 0, 65535, 0, 14,
				sec_audio_adaptation_sound_get,
				sec_audio_adaptation_sound_put),
	SOC_SINGLE_MULTI_EXT("LRSM data", SND_SOC_NOPM, 0, 65535, 0, 2,
				sec_audio_sound_balance_get,
				sec_audio_sound_balance_put),
	SOC_SINGLE_EXT("VoiceTrackInfo", SND_SOC_NOPM, 0, 2147483647, 0,
				sec_audio_voice_tracking_info_get,
				sec_audio_voice_tracking_info_put),
	SOC_SINGLE_MULTI_EXT("MSP data", SND_SOC_NOPM, 0, 65535, 0, 1,
				sec_audio_myspace_get, sec_audio_myspace_put),
	SOC_SINGLE_MULTI_EXT("SB enable", SND_SOC_NOPM, 0, 65535, 0, 1,
				sec_audio_sound_boost_get,
				sec_audio_sound_boost_put),
	SOC_SINGLE_MULTI_EXT("UPSCALER", SND_SOC_NOPM, 0, 65535, 0, 1,
				sec_audio_upscaler_get, sec_audio_upscaler_put),
	SOC_SINGLE_MULTI_EXT("SB rotation", SND_SOC_NOPM, 0, 65535, 0, 1,
				sec_audio_sb_rotation_get, sec_audio_sb_rotation_put),
	SOC_SINGLE_MULTI_EXT("Sec Set DHA data", SND_SOC_NOPM, 0, 65535, 0, 14,
				sec_voice_adaptation_sound_get,
				sec_voice_adaptation_sound_put),
	SOC_SINGLE_EXT("NB Mode", SND_SOC_NOPM, 0, 1, 0,
				sec_voice_nb_mode_get, sec_voice_nb_mode_put),
	SOC_SINGLE_EXT("Speaker Sensor Mode", SND_SOC_NOPM, 0, 1, 0,
				sec_voice_spk_mode_get, sec_voice_spk_mode_put),
	SOC_SINGLE_EXT("Loopback Enable", SND_SOC_NOPM, 0, LOOPBACK_MAX, 0,
				sec_voice_loopback_get, sec_voice_loopback_put),
	SOC_ENUM_EXT("Voice Device Info", sec_voice_device_enum[0],
				sec_voice_dev_info_get, sec_voice_dev_info_put),
};

static int q6audio_adaptation_platform_probe(struct snd_soc_platform *platform)
{
	pr_info("%s: platform\n", __func__);

	common = voice_get_common_data();
	snd_soc_add_platform_controls(platform,
				samsung_solution_mixer_controls,
			ARRAY_SIZE(samsung_solution_mixer_controls));

	return 0;
}

static struct snd_soc_platform_driver q6audio_adaptation = {
	.probe		= q6audio_adaptation_platform_probe,
};

static int samsung_q6audio_adaptation_probe(struct platform_device *pdev)
{
	int ret;

	pr_info("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	atomic_set(&this_afe.state, 0);
	atomic_set(&this_afe.status, 0);
	atomic_set(&this_cvs.state, 0);
	atomic_set(&this_cvp.state, 0);
	this_afe.apr = NULL;
	this_cvs.apr = NULL;
	this_cvp.apr = NULL;
	init_waitqueue_head(&this_afe.wait);
	init_waitqueue_head(&this_cvs.wait);
	init_waitqueue_head(&this_cvp.wait);
	mutex_init(&asm_lock);

	ret = of_property_read_u32(pdev->dev.of_node,
			"adaptation,voice-tracking-tx-port-id",
			&afe_port.voice_tracking_id);
	if (ret)
		pr_err("%s : Unable to read Tx BE\n", __func__);

	ret = of_property_read_u32(pdev->dev.of_node,
			"adaptation,amp-rx-port-id",
			&afe_port.amp_rx_id);
	if (ret)
		pr_debug("%s : Unable to find amp-rx-port\n", __func__);

	ret = of_property_read_u32(pdev->dev.of_node,
			"adaptation,amp-tx-port-id",
			&afe_port.amp_tx_id);
	if (ret)
		pr_debug("%s : Unable to find amp-tx-port\n", __func__);

#if defined(CONFIG_SND_SOC_MAXIM_DSM)
	afe_port.tx_topology = AFE_TOPOLOGY_ID_DSM_TX;
	afe_port.rx_topology = AFE_TOPOLOGY_ID_DSM_RX;
#elif defined(CONFIG_SND_SOC_TFA9872)
	afe_port.tx_topology = AFE_TOPOLOGY_ID_TFADSP_TX;
	afe_port.rx_topology = AFE_TOPOLOGY_ID_TFADSP_RX;
#endif

#if defined(CONFIG_SND_SOC_TFA9872)
	if (this_afe.apr == NULL) {
		this_afe.apr = apr_register("ADSP", "AFE",
				q6audio_tfadsp_callback,
				SEC_ADAPTATAION_DSM_SRC_PORT, &this_afe);
	}
	atomic_set(&this_afe.tfa_state, 0);
	memset(&this_afe.tfa_cal, 0x00, sizeof(struct rtac_cal_block_data));
	tfa_ext_register(q6audio_tfadsp_write, q6audio_tfadsp_read, NULL);
#endif /* CONFIG_SND_SOC_TFA9872 */

	return snd_soc_register_platform(&pdev->dev, &q6audio_adaptation);
}

static int samsung_q6audio_adaptation_remove(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);
#if defined(CONFIG_SND_SOC_TFA9872)
#if !defined(TFA_SHARED_MEM_IPC)
	q6audio_tfadsp_write(-1, 0, NULL, 0, 0);
#endif
	afe_nxp_mmap_destroy(); /* shared mem */
#endif
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static const struct of_device_id samsung_q6audio_adaptation_dt_match[] = {
	{.compatible = "samsung,q6audio-adaptation"},
	{}
};
MODULE_DEVICE_TABLE(of, samsung_q6audio_adaptation_dt_match);

static struct platform_driver samsung_q6audio_adaptation_driver = {
	.driver = {
		.name = "samsung-q6audio-adaptation",
		.owner = THIS_MODULE,
		.of_match_table = samsung_q6audio_adaptation_dt_match,
	},
	.probe = samsung_q6audio_adaptation_probe,
	.remove = samsung_q6audio_adaptation_remove,
};

static int __init sec_soc_platform_init(void)
{
	pr_debug("%s\n", __func__);
	return platform_driver_register(&samsung_q6audio_adaptation_driver);
}
module_init(sec_soc_platform_init);

static void __exit sec_soc_platform_exit(void)
{
	pr_debug("%s\n", __func__);
	platform_driver_unregister(&samsung_q6audio_adaptation_driver);
}
module_exit(sec_soc_platform_exit);

MODULE_AUTHOR("SeokYoung Jang, <quartz.jang@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC ADSP Adaptation driver");
MODULE_LICENSE("GPL v2");
