/*
 * drivers/media/platform/exynos/mfc/s5p_mfc_nal_q_struct.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __S5P_MFC_NAL_Q_STRUCT_H
#define __S5P_MFC_NAL_Q_STRUCT_H __FILE__

#define NAL_Q_ENABLE 1

#define NAL_Q_IN_ENTRY_SIZE		256
#define NAL_Q_OUT_ENTRY_SIZE		256

#define NAL_Q_IN_DEC_STR_SIZE		112
#define NAL_Q_IN_ENC_STR_SIZE		180
#define NAL_Q_OUT_DEC_STR_SIZE		248
#define NAL_Q_OUT_ENC_STR_SIZE		64

#define NAL_Q_IN_QUEUE_SIZE		16 /* 256*16 = 4096 bytes */
#define NAL_Q_OUT_QUEUE_SIZE		16 /* 256*16 = 4096 bytes */

typedef struct __DecoderInputStr
{
	int StartCode; /* = 0xAAAAAAAA; Decoder input structure marker */
	int CommandId;
	int InstanceId;
	int PictureTag;
	unsigned int CpbBufferAddr;
	int CpbBufferSize;
	int CpbBufferOffset;
	int StreamDataSize;
	int AvailableDpbFlagUpper;
	int AvailableDpbFlagLower;
	int DynamicDpbFlagUpper;
	int DynamicDpbFlagLower;
	unsigned int FrameAddr[3];
	int FrameSize[3];
	int NalStartOptions;
	int FrameStrideSize[3];
	int Frame2BitSize[2];
	int Frame2BitStrideSize[2];
	unsigned int ScratchBufAddr;
	int ScratchBufSize;
	char reserved[NAL_Q_IN_ENTRY_SIZE - NAL_Q_IN_DEC_STR_SIZE];
} DecoderInputStr; /* 28*4 = 112 bytes */

typedef struct __EncoderInputStr
{
	int StartCode; /* 0xBBBBBBBB; Encoder input structure marker */
	int CommandId;
	int InstanceId;
	int PictureTag;
	unsigned int FrameAddr[3];
	unsigned int StreamBufferAddr;
	int StreamBufferSize;
	int StreamBufferOffset;
	int RcRoiCtrl;
	unsigned int RoiBufferAddr;
	int ParamChange;
	int IrSize;
	int GopConfig;
	int RcFrameRate;
	int RcBitRate;
	int MsliceMode;
	int MsliceSizeMb;
	int MsliceSizeBits;
	int FrameInsertion;
	int HierarchicalBitRateLayer[7];
	int H264RefreshPeriod;
	int HevcRefreshPeriod;
	int RcQpBound;
	int RcQpBoundPb;
	int FixedPictureQp;
	int PictureProfile;
	int BitCountEnable;
	int MaxBitCount;
	int MinBitCount;
	int NumTLayer;
	int H264NalControl;
	int HevcNalControl;
	int Vp8NalControl;
	int Vp9NalControl;
	int H264HDSvcExtension0;
	int H264HDSvcExtension1;
	int GopConfig2;
	char reserved[NAL_Q_IN_ENTRY_SIZE - NAL_Q_IN_ENC_STR_SIZE];
} EncoderInputStr; /* 45*4 = 180 bytes */

typedef struct __DecoderOutputStr
{
	int StartCode; /* 0xAAAAAAAA; Decoder output structure marker */
	int CommandId;
	int InstanceId;
	int ErrorCode;
	int PictureTagTop;
	int PictureTimeTop;
	int DisplayFrameWidth;
	int DisplayFrameHeight;
	int DisplayStatus;
	unsigned int DisplayAddr[3];
	int DisplayFrameType;
	int DisplayCropInfo1;
	int DisplayCropInfo2;
	int DisplayPictureProfile;
	int DisplayAspectRatio;
	int DisplayExtendedAr;
	int DecodedNalSize;
	int UsedDpbFlagUpper;
	int UsedDpbFlagLower;
	int SeiAvail;
	int FramePackArrgmentId;
	int FramePackSeiInfo;
	int FramePackGridPos;
	int DisplayRecoverySeiInfo;
	int H264Info;
	int DisplayFirstCrc;
	int DisplaySecondCrc;
	int DisplayThirdCrc;
	int DisplayFirst2BitCrc;
	int DisplaySecond2BitCrc;
	int DecodedFrameWidth;
	int DecodedFrameHeight;
	int DecodedStatus;
	unsigned int DecodedAddr[3];
	int DecodedFrameType;
	int DecodedCropInfo1;
	int DecodedCropInfo2;
	int DecodedPictureProfile;
	int DecodedRecoverySeiInfo;
	int DecodedFirstCrc;
	int DecodedSecondCrc;
	int DecodedThirdCrc;
	int DecodedFirst2BitCrc;
	int DecodedSecond2BitCrc;
	int PictureTagBot;
	int PictureTimeBot;
	int ChromaFormat;
	int Mpeg4Info;
	int HevcInfo;
	int Vc1Info;
	int VideoSignalType;
	int ContentLightLevelInfoSei;
	int MasteringDisplayColourVolumeSei0;
	int MasteringDisplayColourVolumeSei1;
	int MasteringDisplayColourVolumeSei2;
	int MasteringDisplayColourVolumeSei3;
	int MasteringDisplayColourVolumeSei4;
	int MasteringDisplayColourVolumeSei5;
	char reserved[NAL_Q_OUT_ENTRY_SIZE - NAL_Q_OUT_DEC_STR_SIZE];
} DecoderOutputStr; /* 62*4 =  248 bytes */

typedef struct __EncoderOutputStr
{
	int StartCode; /* 0xBBBBBBBB; Encoder output structure marker */
	int CommandId;
	int InstanceId;
	int ErrorCode;
	int PictureTag;
	unsigned int EncodedFrameAddr[3];
	unsigned int StreamBufferAddr;
	int StreamBufferOffset;
	int StreamSize;
	int SliceType;
	int NalDoneInfo;
	unsigned int ReconLumaDpbAddr;
	unsigned int ReconChromaDpbAddr;
	int EncCnt;
	char reserved[NAL_Q_OUT_ENTRY_SIZE - NAL_Q_OUT_ENC_STR_SIZE];
} EncoderOutputStr; /* 16*4 = 64 bytes */

/**
 * enum nal_queue_state - The state for nal queue operation.
 */
typedef enum _nal_queue_state {
	NAL_Q_STATE_CREATED = 0,
	NAL_Q_STATE_INITIALIZED,
	NAL_Q_STATE_STARTED, /* when s5p_mfc_nal_q_start() is called */
	NAL_Q_STATE_STOPPED, /* when s5p_mfc_nal_q_stop() is called */
} nal_queue_state;

typedef struct _nal_in_queue {
	union {
		DecoderInputStr dec;
		EncoderInputStr enc;
	} entry[NAL_Q_IN_QUEUE_SIZE];
} nal_in_queue;

typedef struct _nal_out_queue {
	union {
		DecoderOutputStr dec;
		EncoderOutputStr enc;
	} entry[NAL_Q_OUT_QUEUE_SIZE];
} nal_out_queue;

struct _nal_queue_handle;
typedef struct _nal_queue_in_handle {
	struct _nal_queue_handle *nal_q_handle;
	void *in_alloc;
	unsigned int in_exe_count;
	nal_in_queue *nal_q_in_addr;
	spinlock_t lock;
} nal_queue_in_handle;

typedef struct _nal_queue_out_handle {
	struct _nal_queue_handle *nal_q_handle;
	void *out_alloc;
	unsigned int out_exe_count;
	nal_out_queue *nal_q_out_addr;
	int nal_q_ctx;
} nal_queue_out_handle;

typedef struct _nal_queue_handle {
	nal_queue_in_handle *nal_q_in_handle;
	nal_queue_out_handle *nal_q_out_handle;
	nal_queue_state nal_q_state;
	int nal_q_exception;
} nal_queue_handle;

#endif /* __S5P_MFC_NAL_Q_STRUCT_H  */
