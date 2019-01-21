/*
 * ALSA SoC - Samsung Adaptation driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
  *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SEC_ADAPTATION_H
#define __SEC_ADAPTATION_H

#ifdef CONFIG_SND_SOC_MAXIM_DSM
#include <sound/maxim_dsm.h>
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
#include <sound/q6afe-v2.h>
#include <sound/q6asm-v2.h>
#include <sound/q6adm-v2.h>

/****************************************************************************/
/*//////////////////// MAXIM SPEAKER AMP DSM SOLUTION //////////////////////*/
/****************************************************************************/
#ifdef CONFIG_SND_SOC_MAXIM_DSM
#define AFE_PARAM_ID_CALIB_RES_LOG_CFG 0x0001122B
#define AFE_TOPOLOGY_ID_DSM_RX 0x10001061
#define AFE_TOPOLOGY_ID_DSM_TX 0x10001060

struct afe_dsm_filter_set_params_t {
	uint32_t dcResistance;
	uint32_t coilTemp;
	uint32_t qualityfactor;
	uint32_t resonanceFreq;
	uint32_t excursionMeasure;
	uint32_t rdcroomtemp;
	uint32_t releasetime;
	uint32_t coilthermallimit;
	uint32_t excursionlimit;
	uint32_t dsmenabled;
	uint32_t staticgain;
	uint32_t lfxgain;
	uint32_t pilotgain;
	uint32_t flagToWrite;
	uint32_t featureSetEnable;
	uint32_t smooFacVoltClip;
	uint32_t highPassCutOffFactor;
	uint32_t leadResistance;
	uint32_t rmsSmooFac;
	uint32_t clipLimit;
	uint32_t thermalCoeff;
	uint32_t qSpk;
	uint32_t excurLoggingThresh;
	uint32_t coilTempLoggingThresh;
	uint32_t resFreq;
	uint32_t resFreqGuardBand;
	uint32_t Ambient_Temp;
	uint32_t STL_attack_time;
	uint32_t STL_release_time;
	uint32_t STL_Admittance_a1;
	uint32_t STL_Admittance_a2;
	uint32_t STL_Admittance_b0;
	uint32_t STL_Admittance_b1;
	uint32_t STL_Admittance_b2;
	uint32_t Tch1;
	uint32_t Rth1;
	uint32_t Tch2;
	uint32_t Rth2;
	uint32_t STL_Attenuation_Gain;
	uint32_t SPT_rampDownFrames;
	uint32_t SPT_Threshold;
	uint32_t T_horizon;
	uint32_t LFX_Admittance_a1;
	uint32_t LFX_Admittance_a2;
	uint32_t LFX_Admittance_b0;
	uint32_t LFX_Admittance_b1;
	uint32_t LFX_Admittance_b2;
	uint32_t X_Max;
	uint32_t SPK_FS;
	uint32_t Q_GUARD_BAND;
	uint32_t STImpedModel_a1;
	uint32_t STImpedModel_a2;
	uint32_t STImpedModel_b0;
	uint32_t STImpedModel_b1;
	uint32_t STImpedModel_b2;
	uint32_t STImpedModel_Flag;
	uint32_t Q_Notch;
	uint32_t Power_Measurement;
	uint32_t Reserve_1;
	uint32_t Reserve_2;
	uint32_t Reserve_3;
	uint32_t Reserve_4;
} __packed;

struct afe_dsm_spkr_prot_set_command {
	struct apr_hdr hdr;
	struct afe_port_cmd_set_param_v2 param;
	struct afe_port_param_data_v2 pdata;
	struct afe_dsm_filter_set_params_t set_config;
} __packed;

struct afe_dsm_filter_get_params_t {
	uint32_t dcResistance;
	uint32_t coilTemp;
	uint32_t qualityfactor;
	uint32_t resonanceFreq;
	uint32_t excursionMeasure;
	uint32_t rdcroomtemp;
	uint32_t releasetime;
	uint32_t coilthermallimit;
	uint32_t excursionlimit;
	uint32_t dsmenabled;
	uint32_t staticgain;
	uint32_t lfxgain;
	uint32_t pilotgain;
	uint32_t flagToWrite;
	uint32_t featureSetEnable;
	uint32_t smooFacVoltClip;
	uint32_t highPassCutOffFactor;
	uint32_t leadResistance;
	uint32_t rmsSmooFac;
	uint32_t clipLimit;
	uint32_t thermalCoeff;
	uint32_t qSpk;
	uint32_t excurLoggingThresh;
	uint32_t coilTempLoggingThresh;
	uint32_t resFreq;
	uint32_t resFreqGuardBand;
	uint32_t Ambient_Temp;
	uint32_t STL_attack_time;
	uint32_t STL_release_time;
	uint32_t STL_Admittance_a1;
	uint32_t STL_Admittance_a2;
	uint32_t STL_Admittance_b0;
	uint32_t STL_Admittance_b1;
	uint32_t STL_Admittance_b2;
	uint32_t Tch1;
	uint32_t Rth1;
	uint32_t Tch2;
	uint32_t Rth2;
	uint32_t STL_Attenuation_Gain;
	uint32_t SPT_rampDownFrames;
	uint32_t SPT_Threshold;
	uint32_t T_horizon;
	uint32_t LFX_Admittance_a1;
	uint32_t LFX_Admittance_a2;
	uint32_t LFX_Admittance_b0;
	uint32_t LFX_Admittance_b1;
	uint32_t LFX_Admittance_b2;
	uint32_t X_Max;
	uint32_t SPK_FS;
	uint32_t Q_GUARD_BAND;
	uint32_t STImpedModel_a1;
	uint32_t STImpedModel_a2;
	uint32_t STImpedModel_b0;
	uint32_t STImpedModel_b1;
	uint32_t STImpedModel_b2;
	uint32_t STImpedModel_Flag;
	uint32_t Q_Notch;
	uint32_t Power_Measurement;
	uint32_t Reserve_1;
	uint32_t Reserve_2;
	uint32_t Reserve_3;
	uint32_t Reserve_4;
} __packed;

struct afe_dsm_spkr_prot_get_command {
	struct apr_hdr hdr;
	struct afe_port_cmd_get_param_v2 get_param;
	struct afe_port_param_data_v2 pdata;
	struct afe_dsm_filter_get_params_t res_cfg;
} __packed;

struct afe_dsm_spkr_prot_response_data {
	uint32_t status;
	struct afe_port_param_data_v2 pdata;
	struct afe_dsm_filter_get_params_t res_cfg;
} __packed;

struct afe_dsm_filter_get_log_params_t {
	uint8_t byteLogArray[BEFORE_BUFSIZE];
	uint32_t intLogArray[BEFORE_BUFSIZE];
	uint8_t afterProbByteLogArray[AFTER_BUFSIZE];
	uint32_t afterProbIntLogArray[AFTER_BUFSIZE];
	uint32_t intLogMaxArray[LOGMAX_BUFSIZE];
} __packed;

struct afe_dsm_spkr_prot_get_log_command {
	struct apr_hdr hdr;
	struct afe_port_cmd_get_param_v2 get_param;
	struct afe_port_param_data_v2 pdata;
	struct afe_dsm_filter_get_log_params_t res_log_cfg;
} __packed;

struct afe_dsm_spkr_prot_response_log_data {
	uint32_t status;
	struct afe_port_param_data_v2 pdata;
	struct afe_dsm_filter_get_log_params_t res_log_cfg;
} __packed;


#ifdef CONFIG_SEC_SND_ADAPTATION
int32_t dsm_read_write(void *data);
#else
int32_t dsm_read_write(void *data)
{
	return -ENOSYS;
}
#endif /* CONFIG_SEC_SND_ADAPTATION */

static inline int maxim_dsm_write(uint32_t *data, int offset, int size)
{
	return -ENOSYS;
}

static inline int maxim_dsm_read(int offset, int size, void *dsm_data)
{
	return -ENOSYS;
}
#endif /* CONFIG_SND_SOC_MAXIM_DSM */

/****************************************************************************/
/*//////////////////// NXP SPEAKER AMP /////////////////////////////////////*/
/****************************************************************************/
#if defined(CONFIG_SND_SOC_TFA9872)
/*Module ID*/
#define AFE_MODULE_ID_TFADSP          0x1000B910

/*Param ID*/
#define AFE_PARAM_ID_TFADSP_SEND_MSG  0x1000B921
#define AFE_PARAM_ID_TFADSP_READ_MSG  0x1000B922
#define AFE_PARAM_ID_TFADSP_RESP_MSG  0x1000B922

/*Topology ID*/
#define AFE_TOPOLOGY_ID_TFADSP_TX     0x1000B901
#define AFE_TOPOLOGY_ID_TFADSP_RX     0x1000B900

#define AFE_OPCODE_TFADSP_STATUS      0x00010B01
#define AFE_EVENT_TFADSP_STATE_INIT   0x1
#define AFE_EVENT_TFADSP_STATE_CLOSE  0x2

#define AFE_RX_MODULE_ID_TFADSP       0x1000B900
#define AFE_RX_NONE_TOPOLOGY          0x000112fc

#define AFE_TFADSP_STATIC_MEMORY

#define TFA_SHARED_MEM_IPC
#if defined(TFA_SHARED_MEM_IPC)
#define AFE_APR_MAX_PKT_SIZE  4096
#else
/*
 * in case of CONFIG_MSM_QDSP6_APRV2_GLINK/APRV3_GLINK,
 * with smaller APR_MAX_BUF (512)
 */
#define AFE_APR_MAX_PKT_SIZE  APR_MAX_BUF
#endif /* TFA_SHARED_MEM_IPC */

/*afe tfadsp msg type*/
#define AFE_TFADSP_MSG_TYPE_NORMAL 0
#define AFE_TFADSP_MSG_TYPE_RAW    1

#if defined(TFA_SHARED_MEM_IPC)
/*afe tfa dsp send message*/
struct afe_tfa_dsp_send_msg_t {
	struct apr_hdr hdr;
	struct afe_port_cmd_set_param_v2 set_param;
} __packed;
#else /* TFA_SHARED_MEM_IPC */

/*afe tfa payload*/
struct afe_tfa_dsp_payload_t {
	union {
		uint32_t num_msgs;
		char address[1];
	};
	uint32_t buf_size;
	union {
		char *buf_p;
		char buf[1];
	};
} __packed;

/*afe tfa dsp send message*/
struct afe_tfa_dsp_send_msg_t {
	struct apr_hdr hdr;
	struct afe_port_cmd_set_param_v2 set_param;
	struct afe_port_param_data_v2 pdata;
	struct afe_tfa_dsp_payload_t payload;
} __packed;
#endif /* TFA_SHARED_MEM_IPC */

/*afe tfa dsp read message*/
struct afe_tfa_dsp_read_msg_t {
	struct apr_hdr hdr;
	struct afe_port_cmd_get_param_v2 get_param;
} __packed;

typedef int (*tfa_event_handler_t)(int devidx, int tfadsp_event);
typedef int (*dsp_send_message_t)(int devidx, int length,
	char *buf, int msg_type, int num_msgs);
typedef int (*dsp_read_message_t)(int devidx, int length, char *buf);

int tfa_ext_register(dsp_send_message_t tfa_send_message,
		dsp_read_message_t tfa_read_message,
		tfa_event_handler_t *tfa_event_handler);
#endif
int q6audio_get_afe_cal_validation(u16 port_id, u32 topology_id);
/****************************************************************************/
/*//////////////////////////// AUDIO SOLUTION //////////////////////////////*/
/****************************************************************************/
#define ADM_MODULE_ID_PP_SS_REC             0x10001050
#define ADM_PARAM_ID_PP_SS_REC_GETPARAMS    0x10001052

#define ASM_MODULE_ID_PP_SA                 0x10001fa0
#define ASM_PARAM_ID_PP_SA_PARAMS           0x10001fa1

#define ASM_MODULE_ID_PP_SA_VSP             0x10001fb0
#define ASM_PARAM_ID_PP_SA_VSP_PARAMS       0x10001fb1

#define ASM_MODULE_ID_PP_ADAPTATION_SOUND		 0x10001fc0
#define ASM_PARAM_ID_PP_ADAPTATION_SOUND_PARAMS  0x10001fc1

#define ASM_MODULE_ID_PP_LRSM               0x10001fe0
#define ASM_PARAM_ID_PP_LRSM_PARAMS         0x10001fe1

#define ASM_MODULE_ID_PP_SA_MSP             0x10001ff0
#define ASM_MODULE_ID_PP_SA_MSP_PARAM       0x10001ff1

#define ASM_MODULE_ID_PP_SB                 0x10001f01
#define ASM_PARAM_ID_PP_SB_PARAM            0x10001f04
#define ASM_PARAM_ID_PP_SB_ROTATION_PARAM	0x10001f02

#define ASM_MODULE_ID_PP_SA_UPSCALER_COLOR            0x10001f20
#define ASM_PARAM_ID_PP_SA_UPSCALER_COLOR_PARAMS      0x10001f21

struct asm_stream_cmd_set_pp_params_sa {
	struct apr_hdr	hdr;
	struct asm_stream_cmd_set_pp_params_v2 param;
	struct asm_stream_param_data_v2 data;

	int16_t OutDevice;
	int16_t Preset;
	int32_t EqLev[9];
	int16_t m3Dlevel;
	int16_t BElevel;
	int16_t CHlevel;
	int16_t CHRoomSize;
	int16_t Clalevel;
	int16_t volume;
	int16_t Sqrow;
	int16_t Sqcol;
	int16_t TabInfo;
	int16_t NewUI;
	int16_t m3DPositionOn;
	int16_t reserved;
	int32_t m3DPositionAngle[2];
	int32_t m3DPositionGain[2];
	int32_t AHDRonoff;
} __packed;

struct asm_stream_cmd_set_pp_params_vsp {
	struct apr_hdr	hdr;
	struct asm_stream_cmd_set_pp_params_v2 param;
	struct asm_stream_param_data_v2 data;

	uint32_t speed_int;
} __packed;

struct asm_stream_cmd_set_pp_params_adaptation_sound {
	struct apr_hdr	hdr;
	struct asm_stream_cmd_set_pp_params_v2 param;
	struct asm_stream_param_data_v2 data;

	int32_t enable;
	int16_t gain[2][6];
	int16_t device;
} __packed;

struct asm_stream_cmd_set_pp_params_lrsm {
	struct apr_hdr	hdr;
	struct asm_stream_cmd_set_pp_params_v2 param;
	struct asm_stream_param_data_v2 data;

	int16_t sm;
	int16_t lr;
} __packed;

struct asm_stream_cmd_set_pp_params_msp {
	struct apr_hdr	hdr;
	struct asm_stream_cmd_set_pp_params_v2 param;
	struct asm_stream_param_data_v2 data;

	uint32_t msp_int;
} __packed;

struct asm_stream_cmd_set_pp_params_sb {
	struct apr_hdr	hdr;
	struct asm_stream_cmd_set_pp_params_v2 param;
	struct asm_stream_param_data_v2 data;

	uint32_t sb_enable;
} __packed;

struct asm_stream_cmd_set_pp_params_upscaler {
	struct apr_hdr	hdr;
	struct asm_stream_cmd_set_pp_params_v2 param;
	struct asm_stream_param_data_v2 data;

	uint32_t upscaler_enable;
} __packed;

struct asm_stream_cmd_set_pp_params_sb_rotation {
	struct apr_hdr	hdr;
	struct asm_stream_cmd_set_pp_params_v2 param;
	struct asm_stream_param_data_v2 data;

	uint32_t sb_rotation;
} __packed;

/****************************************************************************/
/*//////////////////////////// VOICE SOLUTION //////////////////////////////*/
/****************************************************************************/
/* NXP LVVEFQ */
#define VPM_TX_SM_LVVEFQ_COPP_TOPOLOGY      0x1000BFF0
#define VPM_TX_DM_LVVEFQ_COPP_TOPOLOGY      0x1000BFF1
#define VPM_TX_SM_LVSAFQ_COPP_TOPOLOGY      0x1000BFF4
/* Fotemeia */
#define VOICE_TX_DIAMONDVOICE_FVSAM_SM      0x1000110B
#define VOICE_TX_DIAMONDVOICE_FVSAM_DM      0x1000110A
#define VOICE_TX_DIAMONDVOICE_FVSAM_QM      0x10001109
#define VOICE_TX_DIAMONDVOICE_FRSAM_DM      0x1000110C

#define VOICEPROC_MODULE_VENC				0x00010F07
#define VOICE_PARAM_LOOPBACK_ENABLE			0x00010E18
/* Rx */
#define VOICE_VOICEMODE_MODULE				0x10001001
#define VOICE_ADAPTATION_SOUND_PARAM        0x10001022
/* Tx */
#define VOICE_WISEVOICE_MODULE				0x10001031
#define VOICE_FVSAM_MODULE					0x10001041

#define VOICE_NBMODE_PARAM					0x10001023
#define VOICE_SPKMODE_PARAM					0x10001025

#define VOICE_MODULE_SET_DEVICE				0x10041000
#define VOICE_MODULE_SET_DEVICE_PARAM		0x10041001

struct vss_icommon_cmd_set_loopback_enable_t {
	uint32_t module_id;
	/* Unique ID of the module. */
	uint32_t param_id;
	/* Unique ID of the parameter. */
	uint16_t param_size;
	/* Size of the parameter in bytes: MOD_ENABLE_PARAM_LEN */
	uint16_t reserved;
	/* Reserved; set to 0. */
	uint16_t loopback_enable;
	uint16_t reserved_field;
	/* Reserved, set to 0. */
};

struct cvs_set_loopback_enable_cmd {
	struct apr_hdr hdr;
	uint32_t mem_handle;
	uint32_t mem_address_lsw;
	uint32_t mem_address_msw;
	uint32_t mem_size;
	struct vss_icommon_cmd_set_loopback_enable_t vss_set_loopback;
} __packed;

struct cvp_adaptation_sound_parm_send_t {
	uint32_t module_id;
	/* Unique ID of the module. */
	uint32_t param_id;
	/* Unique ID of the parameter. */
	uint16_t param_size;
	/* Size of the parameter in bytes: MOD_ENABLE_PARAM_LEN */
	uint16_t reserved;
	/* Reserved; set to 0. */
	uint16_t eq_mode;
	uint16_t select;
	int16_t param[12];
} __packed;

struct cvp_adaptation_sound_parm_send_cmd {
	struct apr_hdr hdr;
	uint32_t mem_handle;
	uint32_t mem_address_lsw;
	uint32_t mem_address_msw;
	uint32_t mem_size;
	struct cvp_adaptation_sound_parm_send_t adaptation_sound_data;
} __packed;

void voice_sec_loopback_start_cmd(u32 session_id);
void voice_sec_loopback_end_cmd(u32 session_id);

#endif /* __SEC_ADAPTATION_H */
