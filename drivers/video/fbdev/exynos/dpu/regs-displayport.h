/* linux/drivers/video/fbdev/exynos/dpu/regs-displayport.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Register definition file for Samsung vpp driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef DISPLAYPORT_REGS_H_
#define DISPLAYPORT_REGS_H_

#define Function_En_1                           (0x18)
#define GTC_FUNC_EN_N			        (1 << 7)
#define VIDEO_CAPTURE_FUNC_EN_N_STR0		(1 << 6)
#define VIDEO_FUNC_EN_N_STR0		        (1 << 5)
#define AUDIO_FIFO_FUNC_EN_N_STR0		(1 << 4)
#define AUDIO_FUNC_EN_N_STR0		        (1 << 3)
#define HDCP_FUNC_EN_N				(1 << 2)
#define SOFTWARE_FUNC_EN_N			(1 << 0)

#define Function_En_2                           (0x1C)
#define VIDEO_CAPTURE_FUNC_EN_N_STR1		(1 << 6)
#define VIDEO_FUNC_EN_N_STR1			(1 << 5)
#define AUDIO_FIFO_FUNC_EN_N_STR1               (1 << 4)
#define AUDIO_FUNC_EN_N_STR1		        (1 << 3)
#define AUX_FUNC_EN_N		                (1 << 2)
#define SERDES_FIFO_FUNC_EN_N			(1 << 1)
#define LS_CLOCK_DOMAIN_FUNC_EN_N		(1 << 0)

#define Function_En_3                           (0x20)
#define HDCP22_BYPASS_MODE		        (1 << 2)
#define HDCP22_ENC_EN				(1 << 1)
#define HDCP22_MODE		                (1 << 0)

#define Function_En_4                           (0x24)
#define STRM_CLK_EN		                (1 << 0)

#define SW_Reset                                (0x100)
#define DP_TX_SW_RESET		                (1 << 0)

#define Env_Control                             (0x104)
#define I2C_CLK_SEL		                (1 << 0)

#define Lane_Map				(0x35C)
#define LANE3_MAP				(3 << 6)
#define LANE3_MAP_BIT_POSISION			(6)
#define LANE2_MAP				(3 << 4)
#define LANE2_MAP_BIT_POSISION			(4)
#define LANE1_MAP				(3 << 2)
#define LANE1_MAP_BIT_POSISION			(2)
#define LANE0_MAP				(3 << 0)
#define LANE0_MAP_BIT_POSISION			(0)

#define Interrupt_Status_Top                    (0x3C0)
#define STR1_INT_STATE		                (1 << 2)
#define STR2_INT_STATE				(1 << 1)
#define INT_STATE		                (1 << 0)

#define Common_Interrupt_Status_2               (0x3C8)
#define PLL_LOCK_CHG		                (1 << 7)
#define ENC_EN_CHG				(1 << 6)
#define HDCP_LINK_CHK_FAIL		        (1 << 5)
#define R0_CHECK_FLAG		                (1 << 4)
#define BKSV_RDY				(1 << 3)
#define SHA_DONE		                (1 << 2)
#define AUTH_STATE_CHG				(1 << 1)
#define AUTH_DONE		                (1 << 0)

#define Common_Interrupt_Status_4               (0x3D0)
#define HPD_CHG                                 (1 << 2)
#define HPD_LOST                                (1 << 1)
#define PLUG                                    (1 << 0)

#define DP_Interrupt_Status                     (0x3DC)
#define HOT_PLUG_DET				(1 << 6)
#define TRAINING_FINISH				(1 << 5)
#define SOFT_INTERRUPT		                (1 << 4)
#define SINK_LOST_CLEAR				(1 << 3)
#define LINK_LOST_CLEAR		                (1 << 2)
#define RPLY_RECEIV				(1 << 1)
#define AUX_ERR					(1 << 0)

#define Interrupt_Mask_2                        (0x3E4)
#define PLL_LOCK_CHG		                (1 << 7)
#define ENC_EN_CHG				(1 << 6)
#define HDCP_LINK_CHK_FAIL		        (1 << 5)
#define R0_CHECK_FLAG		                (1 << 4)
#define BKSV_RDY				(1 << 3)
#define SHA_DONE		                (1 << 2)
#define AUTH_STATE_CHG				(1 << 1)
#define AUTH_DONE		                (1 << 0)

#define Common_Interrupt_Mask_4                 (0x3EC)
#define HPD_CHG                                 (1 << 2)
#define HPD_LOST                                (1 << 1)
#define PLUG                                    (1 << 0)

#define DP_Interrupt_Status_Mask_1              (0x3F8)
#define SOFT_INTERRUPT_MASK                     (1 << 7)
#define HOT_PLUG_DET_MASK                       (1 << 6)
#define TRAINING_FINISH_MASK		        (1 << 5)
#define SINK_LOST_MASK                          (1 << 3)
#define LINK_LOST_MASK		                (1 << 2)
#define RPLY_RECEIV_MASK			(1 << 1)
#define AUX_ERR_MASK		                (1 << 0)

#define Interrupt_Control                       (0x3FC)
#define SOFT_INT_CTRL                           (1 << 2)
#define INT_POL                                 (1 << 0)

#define DP_System_Control_HPD                   (0x608)
#define HPD_EVENT_UNPLUG                        (1 << 10)
#define HPD_EVENT_PLUG                          (1 << 9)
#define HPD_EVENT_IRQ                           (1 << 8)
#define HPD_EVENT_CTRL                          (1 << 7)
#define PD_STATUS                               (1 << 6)
#define F_HPD                                   (1 << 5)
#define HPD_CTRL                                (1 << 4)
#define HPD_HDCP		                (1 << 3)

#define DP_MISC_Control                         (0x648)
#define MISC_CTRL_EN                            (1 << 7)
#define HDCP_HPD_RST                            (1 << 6)
#define LINK_CLK_SEL                            (3 << 4)

#define DP_Main_Link_Bandwidth_Setting          (0x680)
#define LINK_BW_SET                             (0x1F << 0)

#define DP_Main_Link_Lane_Count                 (0x684)
#define LANE_COUNT_SET                          (7 << 0)

#define DP_Training_Pattern_Set                 (0x688)
#define SCRAMBLING_DISABLE                      (1 << 5)
#define LINK_QUAL_PATTERN_SET                   (7 << 2)
#define TRAINING_PATTERN_SET                    (3 << 0)

#define DP_Lane_0_Link_Training_Control         (0x68C)
#define MAX_PRE_REACH_0                         (1 << 5)
#define PRE_EMPHASIS_SET_0                      (3 << 3)
#define MAX_DRIVE_REACH_0                       (1 << 2)
#define DRIVE_CURRENT_SET_0                     (3 << 0)

#define DP_Lane_1_Link_Training_Control         (0x690)
#define MAX_PRE_REACH_1                         (1 << 5)
#define PRE_EMPHASIS_SET_1                      (3 << 3)
#define MAX_DRIVE_REACH_1                       (1 << 2)
#define DRIVE_CURRENT_SET_1                     (3 << 0)

#define DP_Lane_2_Link_Training_Control         (0x694)
#define MAX_PRE_REACH_2                         (1 << 5)
#define PRE_EMPHASIS_SET_2                      (3 << 3)
#define MAX_DRIVE_REACH_2                       (1 << 2)
#define DRIVE_CURRENT_SET_2                     (3 << 0)

#define DP_Lane_3_Link_Training_Control         (0x698)
#define MAX_PRE_REACH_3                         (1 << 5)
#define PRE_EMPHASIS_SET_3                      (3 << 3)
#define MAX_DRIVE_REACH_3                       (1 << 2)
#define DRIVE_CURRENT_SET_3                     (3 << 0)

#define DP_HW_LINK_TRAINING_CONTROL_Register    (0x6A0)
#define TRAINING_ERROR_CODE                     (7 << 4)
#define TRAINING_EN                             (1 << 0)

#define DP_Debug_1                              (0x6C0)
#define PLL_LOCK                                (1 << 4)
#define F_PLL_LOCK                              (1 << 3)
#define PLL_LOCK_CTRL                           (1 << 2)
#define PN_INV                                  (1 << 0)

#define DP_HPD_Deglitch_Low_Byte                (0x6C4)
#define HPD_DEGLITCH_L                          (0xFF << 0)

#define DP_HPD_Deglitch_High_Byte               (0x6C8)
#define HPD_DEGLITCH_H                          (0x3F << 0)

#define DP_Link_Debug_Control                   (0x6E0)
#define TEST_CLK_SEL                            (7 << 5)
#define NEW_PRBS7                               (1 << 4)
#define DIS_FIFO_RST                            (1 << 3)
#define DISABLE_AUTO_RESET_ENCODER              (1 << 2)
#define PRBS31_EN                               (1 << 0)

#define DP_Scrambler_Reset_Counter_Value        (0x6E4)
#define SCRAMBER_COUNT_RESET_VALUE              (0x1FF << 0)

#define DP_Scrambler_Reset_Start_Ctrl           (0x6E8)
#define SCRAMBLER_RESET_START_CTRL              (1 << 0)

#define DP_Scrambler_Start_Number_Ctrl          (0x6EC)
#define R_SEND_SR_EN                            (1 << 10)
#define R_SR_START_NUMBER                       (0x1F << 0)

#define AUX_Detected_Period_Monitoring          (0x6F0)
#define AUX_DETECTED_PERIOD_MON                 (0x1FF << 0)

#define DP_TOP_GNS_Control                      (0x72C)
#define TPS3_EN                                 (1 << 7)
#define EQ_LOOP_COUNT                           (1 << 6)
#define SCRAMBLE_CTRL                           (1 << 4)
#define SCRAMBLE_IN_EX                          (1 << 3)
#define DISABLE_SERDES_FIFO_RSET                (1 << 2)
#define HRB2_EYE_SR_CTRL                        (3 << 0)

#define DP_Test_Pattern_0                       (0x784)
#define TEST_80BIT_PATTERN_0                    (0xFF << 0)

#define DP_Test_Pattern_1                       (0x788)
#define TEST_80BIT_PATTERN_1                    (0xFF << 0)

#define DP_Test_Pattern_2                       (0x78C)
#define TEST_80BIT_PATTERN_2                    (0xFF << 0)

#define DP_Test_Pattern_3                       (0x790)
#define TEST_80BIT_PATTERN_3                    (0xFF << 0)

#define DP_Test_Pattern_4                       (0x794)
#define TEST_80BIT_PATTERN_4                    (0xFF << 0)

#define DP_Test_Pattern_5                       (0x798)
#define TEST_80BIT_PATTERN_5                    (0xFF << 0)

#define DP_Test_Pattern_6                       (0x79C)
#define TEST_80BIT_PATTERN_6                    (0xFF << 0)

#define DP_Test_Pattern_7                       (0x7A0)
#define TEST_80BIT_PATTERN_7                    (0xFF << 0)

#define DP_Test_Pattern_8                       (0x7A4)
#define TEST_80BIT_PATTERN_8                    (0xFF << 0)

#define DP_Test_Pattern_9                       (0x7A8)
#define TEST_80BIT_PATTERN_9                    (0xFF << 0)

#define HBR2_Eye_SR_Low                         (0x7AC)
#define HBR2_EYE_SR_L                           (0xFF << 0)

#define HBR2_Eye_SR_High                        (0x7B0)
#define HBR2_EYE_SR_H                           (0xFF << 0)

#define Serdes_Polarity_Control                 (0x7B4)
#define PN_INV3                                 (1 << 5)
#define PN_INV2                                 (1 << 4)
#define PN_INV1                                 (1 << 3)
#define PN_INV0                                 (1 << 2)
#define POLARITY_INV                            (1 << 1)
#define SCRAM_RST_VALUE                         (1 << 0)

#define Serdes_Fifo_Read_Control                (0x7B8)
#define SERDES_FIFO_READ_START_VAL              (0xF << 0)

#define Phy_Cmn_Ctl_RBR_9                       (0x800)
#define CMN_CTL_RBR_9                           (0xFFFFFF << 0)

#define Phy_Cmn_Ctl_RBR_8                       (0x804)
#define CMN_CTL_RBR_8                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_RBR_7                       (0x808)
#define CMN_CTL_RBR_7                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_RBR_6                       (0x80C)
#define CMN_CTL_RBR_6                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_RBR_5                       (0x810)
#define CMN_CTL_RBR_5                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_RBR_4                       (0x814)
#define CMN_CTL_RBR_4                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_RBR_3                       (0x818)
#define CMN_CTL_RBR_3                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_RBR_2                       (0x81C)
#define CMN_CTL_RBR_2                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_RBR_1                       (0x820)
#define CMN_CTL_RBR_1                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_RBR_0                       (0x824)
#define CMN_CTL_RBR_0                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Dummy_Ctl_RBR                   (0x828)
#define CMN_DUMMY_CTL_RBR                       (0xFFFFFFFF << 0)

#define MASTER_AUDIO_DATA_FROM_APB_ADD          (0x838)
#define MASTER_AUDIO_DATA_FROM_APB              (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_9                       (0x840)
#define CMN_CTL_HBR_9                           (0xFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_8                       (0x844)
#define CMN_CTL_HBR_8                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_7                       (0x848)
#define CMN_CTL_HBR_7                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_6                       (0x84C)
#define CMN_CTL_HBR_6                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_5                       (0x850)
#define CMN_CTL_HBR_5                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_4                       (0x854)
#define CMN_CTL_HBR_4                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_3                       (0x858)
#define CMN_CTL_HBR_3                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_2                       (0x85C)
#define CMN_CTL_HBR_2                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_1                       (0x860)
#define CMN_CTL_HBR_1                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR_0                       (0x864)
#define CMN_CTL_HBR_0                           (0xFFFFFFFF << 0)

#define Phy_Cmn_Dummy_Ctl_HBR                   (0x868)
#define CMN_DUMMY_CTL_HBR                       (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_9                      (0x86C)
#define CMN_CTL_HBR2_9                          (0xFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_8                      (0x870)
#define CMN_CTL_HBR2_8                          (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_7                      (0x874)
#define CMN_CTL_HBR2_7                          (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_6                      (0x878)
#define CMN_CTL_HBR2_6                          (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_5                      (0x87C)
#define CMN_CTL_HBR2_5                          (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_4                      (0x880)
#define CMN_CTL_HBR2_4                          (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_3                      (0x884)
#define CMN_CTL_HBR2_3                          (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_2                      (0x888)
#define CMN_CTL_HBR2_2                          (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_1                      (0x88C)
#define CMN_CTL_HBR2_1                          (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_HBR2_0                      (0x890)
#define CMN_CTL_HBR2_0                          (0xFFFFFFFF << 0)

#define Phy_Cmn_Dummy_Ctl_HBR2                  (0x894)
#define CMN_DUMMY_CTL_HBR2                      (0xFFFFFFFF << 0)

#define Test_Pattern_Register                   (0x8A0)
#define TEST_PATTERN                            (0x3F << 0)

#define Phy_Enable                              (0x900)
#define AUX_EN                                  (1 << 7)
#define BGR_EN                                  (1 << 6)
#define BIAS_EN                                 (1 << 5)
#define PLL_EN                                  (1 << 4)
#define LN0_LANE_EN                             (1 << 3)
#define LN1_LANE_EN                             (1 << 2)
#define LN2_LANE_EN                             (1 << 1)
#define LN3_LANE_EN                             (1 << 0)

#define Phy_Resetn                              (0x904)
#define CMN_PHY_RSTN                            (1 << 5)
#define CMN_INIT_RSTN                           (1 << 4)
#define LN0_PHY_RSTN                            (1 << 3)
#define LN1_PHY_RSTN                            (1 << 2)
#define LN2_PHY_RSTN                            (1 << 1)
#define LN3_PHY_RSTN                            (1 << 0)

#define Phy_Refclk_Sel                          (0x908)
#define REFCLK_SEL                              (3 << 0)

#define Phy_Swap_Ctl                            (0x90C)
#define LN0_SWAP_BIT_ORDER                      (1 << 7)
#define LN1_SWAP_BIT_ORDER                      (1 << 6)
#define LN2_SWAP_BIT_ORDER                      (1 << 5)
#define LN3_SWAP_BIT_ORDER                      (1 << 4)
#define LN0_SWAP_BIT_PHASE                      (1 << 3)
#define LN1_SWAP_BIT_PHASE                      (1 << 2)
#define LN2_SWAP_BIT_PHASE                      (1 << 1)
#define LN3_SWAP_BIT_PHASE                      (1 << 0)

#define Phy_Cmn_Ctl_9                           (0x910)
#define CMN_DTB_EN                              (1 << 29)
#define CMN_DTB_SEL                             (0x1F << 24)
#define CMN_CTL_9                               (0xFFFFFF << 5)

#define Phy_Cmn_Ctl_8                           (0x914)
#define CMN_CTL_8                               (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_7                           (0x918)
#define CMN_CTL_7                               (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_6                           (0x91C)
#define CMN_CTL_6                               (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_5                           (0x920)
#define CMN_CTL_5                               (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_4                           (0x924)
#define CMN_CTL_4                               (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_3                           (0x928)
#define CMN_CTL_3                               (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_2                           (0x92C)
#define CMN_CTL_2                               (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_1                           (0x930)
#define CMN_CTL_1                               (0xFFFFFFFF << 0)

#define Phy_Cmn_Ctl_0                           (0x934)
#define CMN_CTL_0                               (0xFFFFFFFF << 0)

#define Phy_Cmn_Dummy_Ctl                       (0x938)
#define CMN_DUMMY_CTL                           (0xFFFFFFFF << 0)

#define Phy_LN0_Lane_Ctl                        (0x940)
#define LN0_LANE_CTL                            (0xFFFFFFFF << 0)

#define Phy_LN1_Lane_Ctl                        (0x944)
#define LN1_LANE_CTL                            (0xFFFFFFFF << 0)

#define Phy_LN2_Lane_Ctl                        (0x948)
#define LN2_LANE_CTL                            (0xFFFFFFFF << 0)

#define Phy_LN3_Lane_Ctl                        (0x94C)
#define LN3_LANE_CTL                            (0xFFFFFFFF << 0)

#define Phy_Lock_Mon                            (0x950)
#define AFC_DONE                                (1 << 1)
#define PLL_LOCK_DONE                           (1 << 0)

#define Phy_Cmn_Debug_Mon                       (0x954)
#define CMN_DEBUG_MON                           (0xFFFFFFFF << 0)

#define Phy_BIST_Mode                           (0x960)
#define BIST_MODE                               (7 << 0)

#define Phy_BIST_Comp_Start                     (0x964)
#define BIST_COMP_START                         (1 << 0)

#define Phy_BIST_User_Data                      (0x968)
#define BIST_USER_DATA                          (0x3FF << 0)

#define Phy_BIST_Swap_Ctl                       (0x96C)
#define BIST_SWAP_BIT_ORDER                     (1 << 5)
#define BIST_SWAP_BIT_PHASE                     (1 << 4)
#define BIST_PHASE_INV                          (1 << 3)
#define BIST_PHASE_CTL                          (7 << 0)

#define Phy_BIST_Result_Mon                     (0x970)
#define LN0_BIST_PASS                           (1 << 7)
#define LN1_BIST_PASS                           (1 << 6)
#define LN2_BIST_PASS                           (1 << 5)
#define LN3_BIST_PASS                           (1 << 4)
#define LN0_BIST_FAIL                           (1 << 3)
#define LN1_BIST_FAIL                           (1 << 2)
#define LN2_BIST_FAIL                           (1 << 1)
#define LN3_BIST_FAIL                           (1 << 0)

#define Phy_Lane_SFR_Ctl_Mode                   (0x980)
#define PHY_CLK_BW_SET                          (3 << 12)
#define LN_GTC_CLK_SFR_CTL                      (1 << 9)
#define CMN_CTL_SFR_CTL_MODE                    (1 << 8)
#define LN0_EMP_SFR_CTL_MODE                    (1 << 7)
#define LN1_EMP_SFR_CTL_MODE                    (1 << 6)
#define LN2_EMP_SFR_CTL_MODE                    (1 << 5)
#define LN3_EMP_SFR_CTL_MODE                    (1 << 4)
#define LN0_AMP_SFR_CTL_MODE                    (1 << 3)
#define LN1_AMP_SFR_CTL_MODE                    (1 << 2)
#define LN2_AMP_SFR_CTL_MODE                    (1 << 1)
#define LN3_AMP_SFR_CTL_MODE                    (1 << 0)

#define Phy_Lane01_Emphasis                     (0x984)
#define LN0_EMP_LEVEL_0                         (0xF << 28)
#define LN0_EMP_LEVEL_1                         (0xF << 24)
#define LN0_EMP_LEVEL_2                         (0xF << 20)
#define LN0_EMP_LEVEL_3                         (0xF << 16)
#define LN1_EMP_LEVEL_0                         (0xF << 12)
#define LN1_EMP_LEVEL_1                         (0xF << 8)
#define LN1_EMP_LEVEL_2                         (0xF << 4)
#define LN1_EMP_LEVEL_3                         (0xF << 0)

#define Phy_Lane23_Emphasis                     (0x988)
#define LN2_EMP_LEVEL_0                         (0xF << 28)
#define LN2_EMP_LEVEL_1                         (0xF << 24)
#define LN2_EMP_LEVEL_2                         (0xF << 20)
#define LN2_EMP_LEVEL_3                         (0xF << 16)
#define LN3_EMP_LEVEL_0                         (0xF << 12)
#define LN3_EMP_LEVEL_1                         (0xF << 8)
#define LN3_EMP_LEVEL_2                         (0xF << 4)
#define LN3_EMP_LEVEL_3                         (0xF << 0)

#define Phy_Lane0_Amplitude                     (0x98C)
#define LN0_AMP_LEVEL_0                         (0xF << 24)
#define LN0_AMP_LEVEL_1                         (0xF << 16)
#define LN0_AMP_LEVEL_2                         (0xF << 8)
#define LN0_AMP_LEVEL_3                         (0xF << 0)

#define Phy_Lane1_Amplitude                     (0x990)
#define LN1_AMP_LEVEL_0                         (0xF << 24)
#define LN1_AMP_LEVEL_1                         (0xF << 16)
#define LN1_AMP_LEVEL_2                         (0xF << 8)
#define LN1_AMP_LEVEL_3                         (0xF << 0)

#define Phy_Lane2_Amplitude                     (0x994)
#define LN2_AMP_LEVEL_0                         (0xF << 24)
#define LN2_AMP_LEVEL_1                         (0xF << 16)
#define LN2_AMP_LEVEL_2                         (0xF << 8)
#define LN2_AMP_LEVEL_3                         (0xF << 0)

#define Phy_Lane3_Amplitude                     (0x998)
#define LN3_AMP_LEVEL_0                         (0xF << 24)
#define LN3_AMP_LEVEL_1                         (0xF << 16)
#define LN3_AMP_LEVEL_2                         (0xF << 8)
#define LN3_AMP_LEVEL_3                         (0xF << 0)

#define GTC_CLK_Selection                       (0x99C)
#define SFR_GTC_CLK_SEL_RBR                     (1 << 2)
#define SFR_GTC_CLK_SEL_HBR                     (1 << 1)
#define SFR_GTC_CLK_SEL_HBR2                    (1 << 0)

#define PHY_Link_Clock_Selection                (0x9A0)
#define SFR_EXT_PHY_GTC_CLK_SEL                 (1 << 10)
#define SFR_EXT_PHY_GTC_CLK_STATUS              (3 << 8)
#define SFR_GTC_OSC_CLK_STATUS                  (3 << 6)
#define SFR_LS_CLK_DIV2_STATUS                  (3 << 4)
#define SFR_LS_CLK_STATUS                       (3 << 2)
#define SFR_PHY_LINK_CLK_SEL                    (1 << 1)
#define SFR_PHY_LINK_CLK_MODE                   (1 << 0)

#define Video_Control_1                         (0x1020)
#define VIDEO_EN                                (1 << 7)
#define VIDEO_MUTE                              (1 << 6)

#define Video_Control_2                         (0x1024)
#define IN_D_RANGE                              (1 << 7)
#define IN_BPC                                  (7 << 4)
#define IN_COLOR_F                              (3 << 0)

#define Video_Control_3                         (0x1024)
#define IN_YC_COEFFI                            (1 << 7)
#define THREE_D_FIELD_SEQUENTIAL_EN             (1 << 6)
#define THREE_D_FIELD_INV                       (1 << 5)
#define VID_CHK_UPDATE_EN                       (1 << 4)

#define Video_Control_4                         (0x102C)
#define USER_BIST_DATA_EN                       (1 << 5)
#define ODD_TU_CONTROL                          (1 << 4)
#define BIST_EN                                 (1 << 3)
#define BIST_WIDTH                              (1 << 2)
#define BIST_TYPE                               (3 << 0)

#define Video_Control_8                         (0x103C)
#define VID_HRES_TH                             (0xF << 4)
#define VID_VRES_TH                             (0xF << 0)

#define Video_Control_10                        (0x1044)
#define F_SEL                                   (1 << 4)
#define I_SCAN_CFG                              (1 << 2)
#define VSYNC_P_CFG                             (1 << 1)
#define HSYNC_P_CFG                             (1 << 0)

#define Total_Line_Low_Byte                     (0x1048)
#define TOTAL_LINE_CFG_L                        (0xFF << 0)

#define Total_Line_High_Byte                    (0x104C)
#define TOTAL_LINE_CFG_H                        (0x3F << 0)

#define Active_Line_Low_Byte                    (0x1050)
#define ACTIVE_LINE_CFG_L                       (0xFF << 0)

#define Active_Line_High_Byte                   (0x1054)
#define ACTIVE_LINE_CFG_H                       (0x3F << 0)

#define Vertical_Front_Porch                    (0x1058)
#define V_F_PORCH_CFG                           (0xFFF << 0)

#define Vertical_Sync_Width                     (0x105C)
#define V_SYNC_CFG                              (0xFFF << 0)

#define Vertical_Back_Porch                     (0x1060)
#define V_B_PORCH_CFG                           (0xFFF << 0)

#define Total_Pixel_Low_Byte                    (0x1064)
#define TOTAL_PIXEL_CFG_L                       (0xFF << 0)

#define Total_Pixel_High_Byte                   (0x1068)
#define TOTAL_PIXEL_CFG_H                       (0x3F << 0)

#define Active_Pixel_Low_Byte                   (0x106C)
#define ACTIVE_PIXEL_CFG_L                      (0xFF << 0)

#define Active_Pixel_High_Byte                  (0x1070)
#define ACTIVE_PIXEL_CFG_H                      (0x3F << 0)

#define Horizon_Front_Porch_Low_Byte            (0x1074)
#define H_F_PORCH_CFG_L                         (0xFF << 0)

#define Horizon_Front_Porch_High_Byte           (0x1078)
#define H_F_PORCH_CFG_H                         (0xF << 0)

#define Horizon_Sync_Width_Low_Byte             (0x107C)
#define H_SYNC_CFG_L                            (0xFF << 0)

#define Horizon_Sync_Width_High_Byte            (0x1080)
#define H_SYNC_CFG_H                            (0xF << 0)

#define Horizon_Back_Porch_Low_Byte             (0x1084)
#define H_B_PORCH_CFG_L                         (0xFF << 0)

#define Horizon_Back_Porch_High_Byte            (0x1088)
#define H_B_PORCH_CFG_H                         (0xF << 0)

#define Video_Status                            (0x108C)
#define FIELD_S                                 (1 << 3)
#define I_SCAN_S                                (1 << 2)
#define VSYNC_P_S                               (1 << 1)
#define HSYNC_P_S                               (1 << 0)

#define Total_Line_Status_Low_Byte              (0x1090)
#define TOTAL_LINE_STA_L                        (0xFF << 0)

#define Total_Line_Status_High_Byte             (0x1094)
#define TOTAL_LINE_STA_H                        (0x3F << 0)

#define Active_Line_Status_Low_Byte             (0x1098)
#define ACTIVE_LINE_STA_L                       (0xFF << 0)

#define Active_Line_Status_High_Byte            (0x109C)
#define ACTIVE_LINE_STA_H                       (0x3F << 0)

#define Vertical_Front_Porch_Status             (0x10A0)
#define V_F_PORCH_STA                           (0xFFF << 0)

#define Vertical_Sync_Width_Status              (0x10A4)
#define V_SYNC_STA                              (0xFFF << 0)

#define Vertical_Back_Porch_Status              (0x10A8)
#define V_B_PORCH_STA                           (0xFFF << 0)

#define Total_Pixel_Status_Low_Byte             (0x10AC)
#define TOTAL_PIXEL_STA_L                       (0xFF << 0)

#define Total_Pixel_Status_High_Byte            (0x10B0)
#define TOTAL_PIXEL_STA_H                       (0x3F << 0)

#define Active_Pixel_Status_Low_Byte            (0x10B4)
#define ACTIVE_PIXEL_STA_L                      (0xFF << 0)

#define Active_Pixel_Status_High_Byte           (0x10B8)
#define ACTIVE_PIXEL_STA_H                      (0x3F << 0)

#define Horizon_Front_Porch_Status_Low_Byte     (0x10BC)
#define H_F_PORCH_STA_L                         (0xFF << 0)

#define Horizon_Front_Porch_Status_High_Byte    (0x10C0)
#define H_F_PORCH_STA_H                         (0xF << 0)

#define Horizon_Sync_Width_Status_Low_Byte      (0x10C4)
#define H_SYNC_STA_L                            (0xFF << 0)

#define Horizon_Sync_Width_Status_High_Byte     (0x10C8)
#define H_SYNC_STA_H                            (0xF << 0)

#define Horizon_Back_Porch_Status_Low_Byte      (0x10CC)
#define H_B_PORCH_STA_L                         (0xFF << 0)

#define Horizon_Back_Porch_Status_High_Byte     (0x10D0)
#define H_B_PORCH_STA_H                         (0xF << 0)

#define Audio_BIST_Control_Register             (0x10F0)
#define SIN_AMPL                                (0xF << 4)
#define AUD_BIST_EN                             (1 << 0)

#define Audio_Input_Clock_Frequency_Counter_Low_Byte_Register   (0x10F4)
#define AUD_FREQ_CNT_L                          (0xFF << 0)

#define Audio_Input_Clock_Frequency_Counter_High_Byte_Register  (0x10F8)
#define AUD_FREQ_CNT_H                          (0xF << 0)

#define Video_Input_Clock_Frequency_Counter_Low_Byte            (0x1100)
#define VID_FREQ_CNT_L                          (0xFF << 0)

#define Video_Input_Clock_Frequency_Counter_High_Byte           (0x1104)
#define VID_FREQ_CNT_H                          (0xFF << 0)

#define Active_Frame_Parameters                 (0x110C)
#define ACTIVE_LINE_COUNT                       (0x3FFF << 16)
#define ACTIVE_PIXEL_COUNT                      (0x3FFF << 0)

#define BIST_PRBS7_SEED_ADD                     (0x1110)
#define BIST_PRBS7_SEED                         (0x7F << 0)

#define IF_CRC_Control_1                        (0x1114)
#define IF_CRC_CLEAR                            (1 << 13)
#define IF_CRC_PASS                             (1 << 12)
#define IF_CRC_FAIL                             (1 << 8)
#define IF_CRC_SW_COMPARE                       (1 << 4)
#define IF_CRC_EN                               (1 << 0)

#define IF_CRC_Control_2                        (0x1118)
#define IF_CRC_R_REF                            (0xFFFF << 16)
#define IF_CRC_R_RESULT                         (0xFFFF << 0)

#define IF_CRC_Control_3                        (0x111C)
#define IF_CRC_G_REF                            (0xFFFF << 16)
#define IF_CRC_G_RESULT                         (0xFFFF << 0)

#define IF_CRC_Control_4                        (0x1120)
#define IF_CRC_B_REF                            (0xFFFF << 16)
#define IF_CRC_B_RESULT                         (0xFFFF << 0)

#define SA_CRC_Control_1                        (0x1124)
#define SA_CRC_CLEAR                            (1 << 13)
#define SA_CRC_SW_COMPARE                       (1 << 12)
#define SA_CRC_LN3_PASS                         (1 << 11)
#define SA_CRC_LN2_PASS                         (1 << 10)
#define SA_CRC_LN1_PASS                         (1 << 9)
#define SA_CRC_LN0_PASS                         (1 << 8)
#define SA_CRC_LN3_FAIL                         (1 << 7)
#define SA_CRC_LN2_FAIL                         (1 << 6)
#define SA_CRC_LN1_FAIL                         (1 << 5)
#define SA_CRC_LN0_FAIL                         (1 << 4)
#define SA_CRC_LN3_EN                           (1 << 3)
#define SA_CRC_LN2_EN                           (1 << 2)
#define SA_CRC_LN1_EN                           (1 << 1)
#define SA_CRC_LN0_EN                           (1 << 0)

#define SA_CRC_Control_2                        (0x1128)
#define SA_CRC_LN0_REF                          (0xFFFF << 16)
#define SA_CRC_LN0_RESULT                       (0xFFFF << 0)

#define SA_CRC_Control_3                        (0x112C)
#define SA_CRC_LN1_REF                          (0xFFFF << 16)
#define SA_CRC_LN1_RESULT                       (0xFFFF << 0)

#define SA_CRC_Control_4                        (0x1130)
#define SA_CRC_LN2_REF                          (0xFFFF << 16)
#define SA_CRC_LN2_RESULT                       (0xFFFF << 0)

#define SA_CRC_Control_5                        (0x1134)
#define SA_CRC_LN3_REF                          (0xFFFF << 16)
#define SA_CRC_LN3_RESULT                       (0xFFFF << 0)

#define HOST_BIST_DATA_R_ADD                    (0x1138)
#define HOST_BIST_DATA_R                        (0xFF << 0)

#define HOST_BIST_DATA_G_ADD                    (0x113C)
#define HOST_BIST_DATA_G                        (0xFF << 0)

#define HOST_BIST_DATA_B_ADD			(0x1140)
#define HOST_BIST_DATA_B                        (0xFF << 0)

#define SPD_infoFrame_Packet_Type_Register	(0x1244)
#define SPD_infoFrame_Packet_Data_Register	(0x1254)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_1   (0x11D0)
#define AVI_DB1                                 (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_2   (0x11D4)
#define AVI_DB2                                 (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_3   (0x11D8)
#define AVI_DB3                                 (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_4   (0x11DC)
#define AVI_DB4                                 (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_5   (0x11E0)
#define AVI_DB5                                 (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_6   (0x11E4)
#define AVI_DB6                                 (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_7   (0x11E8)
#define AVI_DB7                                 (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_8   (0x11EC)
#define AVI_DB8                                 (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_9   (0x11F0)
#define AVI_DB9                                 (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_10  (0x11F4)
#define AVI_DB10                                (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_11  (0x11F8)
#define AVI_DB11                                (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_12  (0x11FC)
#define AVI_DB12                                (0xFF << 0)

#define AVI_infoFrame_Packet_Register_AVI_Data_Byte_13  (0x1200)
#define AVI_DB13                                (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_1         (0x121C)
#define Audio_DB1                               (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_2         (0x1220)
#define Audio_DB2                               (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_3         (0x1224)
#define Audio_DB3                               (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_4         (0x1228)
#define Audio_DB4                               (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_5         (0x122C)
#define Audio_DB5                               (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_6         (0x1230)
#define Audio_DB6                               (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_7         (0x1234)
#define Audio_DB7                               (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_8         (0x1238)
#define Audio_DB8                               (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_9         (0x123C)
#define Audio_DB9                               (0xFF << 0)

#define Audio_infoFrame_Packet_Register_AVI_Data_Byte_10        (0x1240)
#define Audio_DB10                              (0xFF << 0)

#define Audio_GTC_Delta_Register_1              (0x1330)
#define AUD_GTC_DELTA_1                         (0xFF << 0)

#define Audio_GTC_Delta_Register_2              (0x1334)
#define AUD_GTC_DELTA_2                         (0xFF << 0)

#define Audio_GTC_Delta_Register_3              (0x1338)
#define AUD_GTC_DELTA_3                         (0xFF << 0)

#define Audio_GTC_Delta_Register_4              (0x133C)
#define AUD_GTC_DELTA_4                         (0xFF << 0)

#define Audio_BIST_Channel_Status_Register_1    (0x1340)
#define MODE                                    (3 << 6)
#define PCM_MODE                                (7 << 3)
#define SW_CPRGT                                (1 << 2)
#define NON_PCM                                 (1 << 1)
#define PROF_APP                                (1 << 0)

#define Audio_BIST_Channel_Status_Register_2    (0x1344)
#define CAT_CODE                                (0xFF << 0)

#define Audio_BIST_Channel_Status_Register_3    (0x1348)
#define CH_NUM                                  (0xF << 4)
#define SOURCE_NUM                              (0xF << 0)

#define Audio_BIST_Channel_Status_Register_4    (0x134C)
#define CHNL_BIT1                               (0x3 << 6)
#define CLK_ACCUR                               (0x3 << 4)
#define FS_FREQ                                 (0xF << 0)

#define Audio_BIST_Channel_Status_Register_5    (0x1350)
#define CHNL_BIT2                               (0xF << 4)
#define WORD_LENGTH                             (0x7 << 1)
#define WORD_MAX                                (1 << 0)

#define Interrupt_Status                        (0x13C0)
#define INT_STATE                               (1 << 0)

#define Common_Interrupt_Status_1               (0x13C4)
#define VSYNC_DET                               (1 << 7)
#define VID_FORMAT_CHG                          (1 << 3)
#define AUD_CLK_CHG                             (1 << 2)
#define VID_CLK_CHG                             (1 << 1)

#define Common_Interrupt_Status_3               (0x13CC)
#define AFIFO_UNDER                             (1 << 7)
#define AFIFO_OVER                              (1 << 6)

#define Interrupt_Mask_1                        (0x13E0)
#define VSYNC_DET_MASK                          (1 << 7)
#define VID_FORMAT_CHG_MASK                     (1 << 3)
#define INT_AUDIO_CLK_CHANGE_MASK               (1 << 2)
#define VID_CLK_CHG_MASK                        (1 << 1)

#define Interrupt_Mask_3                        (0x13E8)
#define COMMON_INT_MASK_3                       (3 << 6)

#define DP_INFO_WAIT_TIMER_Register             (0x1500)
#define DP_INFO_WAIT_TIMER                      (0xFF << 0)

#define DP_AUDIO_WAIT_TIMER_Register            (0x1504)
#define DP_AUDIO_WAIT_TIMER                     (0xFF << 0)

#define DP_System_Control_1                     (0x1600)
#define DET_STA                                 (1 << 2)
#define FORCE_DET                               (1 << 1)
#define DET_CTRL                                (1 << 0)

#define DP_System_Control_2                     (0x1604)
#define CHA_CRI                                 (0xF << 4)
#define CHA_STA                                 (1 << 2)
#define FORCE_CHA                               (1 << 1)
#define CHA_CTRL                                (1 << 0)

#define DP_System_Control_3                     (0x1608)
#define STRM_VALID                              (1 << 2)
#define F_VALID                                 (1 << 1)
#define VALID_CTRL                              (1 << 0)

#define DP_System_Control_4                     (0x160C)
#define FIX_M_AUD                               (1 << 4)
#define ENHANCED                                (1 << 3)
#define FIX_M_VID                               (1 << 2)
#define M_VID_UPDATE_CTRL                       (3 << 0)

#define DP_Video_Control                        (0x1610)
#define BPC                                     (7 << 5)
#define YC_COEFF                                (1 << 4)
#define D_RANGE                                 (1 << 3)
#define COLOR_F                                 (3 << 1)

#define DP_Audio_Control_Register               (0x1618)
#define AUD_GTC_CHST_EN                         (1 << 1)
#define DP_AUDIO_EN                             (1 << 0)

#define Packet_Send_Control                     (0x1640)
#define AUDIO_INFO_UP                           (1 << 7)
#define AVI_UD                                  (1 << 6)
#define MPEG_UD                                 (1 << 5)
#define SPD_INFO_UP                             (1 << 4)
#define AUDIO_INFOR_EN                          (1 << 3)
#define AVI_EN									(1 << 2)
#define MPEG_EN                                 (1 << 1)
#define SPD_INFO_EN                             (1 << 0)

#define DP_Mvid_0                               (0x1700)
#define M_VID_0                                 (0xFF << 0)

#define DP_Mvid_1                               (0x1704)
#define M_VID_1                                 (0xFF << 0)

#define DP_Mvid_2                               (0x1708)
#define M_VID_2                                 (0xFF << 0)

#define DP_Nvid_0                               (0x170C)
#define N_VID_0                                 (0xFF << 0)

#define DP_Nvid_1                               (0x1710)
#define N_VID_1                                 (0xFF << 0)

#define DP_Nvid_2                               (0x1714)
#define N_VID_2                                 (0xFF << 0)

#define DP_M_Value_Calculation                  (0x1718)
#define M_VID_MON                               (0xFFFFFF << 0)

#define DP_FIFO_Threshold                       (0x1730)
#define TH_CTRL                                 (1 << 5)
#define TH_VALUE                                (0x1F << 0)

#define DP_GNS_Control                          (0x1734)
#define VIDEO_MAP_CTRL                          (1 << 1)
#define RS_CTRL                                 (1 << 0)

#define DP_Audio_Margin_Register                (0x173C)
#define FORCE_AUDIO_MARGIN                      (1 << 7)
#define AUDIO_MARGIN                            (0x7F << 0)

#define DP_Maud_Configure_Register_0            (0x1748)
#define M_AUD_0                                 (0xFF << 0)

#define DP_Maud_Configure_Register_1            (0x174C)
#define M_AUD_1                                 (0xFF << 0)

#define DP_Maud_Configure_Register_2            (0x1750)
#define M_AUD_2                                 (0xFF << 0)

#define DP_Naud_Configure_Register_0            (0x1754)
#define N_AUD_0                                 (0xFF << 0)

#define DP_Naud_Configure_Register_1            (0x1758)
#define N_AUD_1                                 (0xFF << 0)

#define DP_Naud_Configure_Register_2            (0x175C)
#define N_AUD_2                                 (0xFF << 0)

#define DP_M_Value_Calculation_Control          (0x1760)
#define M_AUD_GEN_FILTER_EN                     (1 << 3)
#define M_VID_GEN_FILTER_EN                     (1 << 2)
#define M_GEN_CLK_SEL                           (1 << 0)

#define DP_Mvid_Value_Calculation_Control       (0x1764)
#define M_VID_GEN_FILTER_TH                     (0xFF << 0)

#define DP_Maud_Value_Calculation_Control       (0x1778)
#define M_AUD_GEN_FILTER_TH                     (0xFF << 0)

#define General_Control                         (0x1800)
#define SW_AUD_CODING_TYPE                      (1 << 29)
#define SW_AUD_CODING_TYPE_EN                   (1 << 28)
#define AUD_CODING_TYPE                         (1 << 27)
#define AUD_DMA_IF_LTNCY_TRG_MODE               (1 << 26)
#define AUD_DMA_IF_MODE_CONFIG                  (1 << 25)
#define AUD_ODD_CHANNEL_DUMMY                   (1 << 24)
#define AUD_M_VALUE_CMP_SPD_MASTER              (7 << 21)
#define DMA_BURST_SEL                           (7 << 18)
#define AUDIO_BIT_MAPPING_TYPE                  (3 << 16)
#define PCM_SIZE                                (3 << 13)
#define AUDIO_MASTER_MODE_EN                    (1 << 9)
#define AUDIO_MODE                              (1 << 8)
#define AUDIO_CH_STATUS_SAME                    (1 << 5)
#define MASTER_VIDEO_INTERLACE_EN               (1 << 4)
#define VIDEO_MASTER_MODE_EN                    (1 << 1)
#define VID_MODE                                (1 << 0)

#define Video_Horizontal_Total_pixels_register  (0x1804)
#define H_TOTAL_MASTER                          (0xFFFFFFFF << 0)

#define Video_Vertical_Total_pixels_register    (0x1808)
#define V_TOTAL_MASTER                          (0xFFFFFFFF << 0)

#define Video_Horizontal_front_porch_register   (0x180C)
#define H_F_PORCH_MASTER                        (0xFFFFFFFF << 0)

#define Video_Horizontal_back_porch_register    (0x1810)
#define H_B_PORCH_MASTER                        (0xFFFFFFFF << 0)

#define Video_Horizontal_Active_register        (0x1814)
#define H_ACTIVE_MASTER                         (0xFFFFFFFF << 0)

#define Video_Vertical_front_porch_register     (0x1818)
#define V_F_PORCH_MASTER                        (0xFFFFFFFF << 0)

#define Video_Vertical_back_porch_register      (0x181C)
#define V_B_PORCH_MASTER                        (0xFFFFFFFF << 0)

#define Video_Vertical_Active_register          (0x1820)
#define V_ACTIVE_MASTER                         (0xFFFFFFFF << 0)

#define Mvid_register_in_video_master_mode      (0x1824)
#define M_VID_MASTER                            (0xFFFFFFFF << 0)

#define Nvid_register_in_video_master_mode      (0x1828)
#define N_VID_MASTER                            (0xFFFFFFFF << 0)

#define Maud_register_in_audio_master_mode      (0x182C)
#define M_AUD_MASTER                            (0xFFFFFFFF << 0)

#define Naud_register_in_audio_master_mode      (0x1830)
#define N_AUD_MASTER                            (0xFFFFFFFF << 0)

#define Master_audio_Buffer_control_register    (0x1834)
#define MASTER_AUDIO_INIT_BUFFER_THRD           (0xFF << 24)
#define MASTER_AUDIO_BUFFER_THRD                (0x3F << 17)
#define MASTER_AUDIO_BUFFER_EMPTY_INT_MASK      (1 << 16)
#define MASTER_AUDIO_CHANNEL_COUNT              (7 << 13)
#define MASTER_AUDIO_BUFFER_LEVEL               (0xFF << 5)
#define AUD_DMA_NOISE_INT_MASK                  (1 << 4)
#define AUD_DMA_NOISE_INT                       (1 << 3)
#define AUDO_DMA_NOISE_INT_EN                   (1 << 2)
#define MASTER_AUDIO_BUFFER_EMPTY_INT           (1 << 1)
#define MASTER_AUDIO_BUFFER_EMPTY_INT_EN        (1 << 0)

#define Audio_Packet_Data_Re_arrangement_Register       (0x183C)
#define AUD_8TH_PACKET                          (0xF << 28)
#define AUD_7TH_PACKET                          (0xF << 24)
#define AUD_6TH_PACKET                          (0xF << 20)
#define AUD_5TH_PACKET                          (0xF << 16)
#define AUD_4TH_PACKET                          (0xF << 12)
#define AUD_3RD_PACKET                          (0xF << 8)
#define AUD_2ND_PACKET                          (0xF << 4)
#define AUD_1ST_PACKET                          (0xF << 0)

#define Master_audio_Packet_GP0_Status_Register_0       (0x1840)
#define MASTER_AUD_GP0_STA_0                    (0xFF << 0)

#define Master_audio_Packet_GP0_Status_Register_1       (0x1844)
#define MASTER_AUD_GP0_STA_1                    (0xFF << 0)

#define Master_audio_Packet_GP0_Status_Register_2       (0x1848)
#define MASTER_AUD_GP0_STA_2                    (0xFF << 0)

#define Master_audio_Packet_GP0_Status_Register_3       (0x184C)
#define MASTER_AUD_GP0_STA_3                    (0xFF << 0)

#define Master_audio_Packet_GP0_Status_Register_4       (0x1850)
#define MASTER_AUD_GP0_STA_4                    (0xFF << 0)

#define Master_audio_Packet_GP1_Status_Register_0       (0x1854)
#define MASTER_AUD_GP1_STA_0                    (0xFF << 0)

#define Master_audio_Packet_GP1_Status_Register_1       (0x1858)
#define MASTER_AUD_GP1_STA_1                    (0xFF << 0)

#define Master_audio_Packet_GP1_Status_Register_2       (0x185C)
#define MASTER_AUD_GP1_STA_2                    (0xFF << 0)

#define Master_audio_Packet_GP1_Status_Register_3       (0x1860)
#define MASTER_AUD_GP1_STA_3                    (0xFF << 0)

#define Master_audio_Packet_GP1_Status_Register_4       (0x1864)
#define MASTER_AUD_GP1_STA_4                    (0xFF << 0)

#define Master_audio_Packet_GP2_Status_Register_0       (0x1868)
#define MASTER_AUD_GP2_STA_0                    (0xFF << 0)

#define Master_audio_Packet_GP2_Status_Register_1       (0x186C)
#define MASTER_AUD_GP2_STA_1                    (0xFF << 0)

#define Master_audio_Packet_GP2_Status_Register_2       (0x1870)
#define MASTER_AUD_GP2_STA_2                    (0xFF << 0)

#define Master_audio_Packet_GP2_Status_Register_3       (0x1874)
#define MASTER_AUD_GP2_STA_3                    (0xFF << 0)

#define Master_audio_Packet_GP2_Status_Register_4       (0x1878)
#define MASTER_AUD_GP2_STA_4                    (0xFF << 0)

#define Master_audio_Packet_GP3_Status_Register_0       (0x187C)
#define MASTER_AUD_GP3_STA_0                    (0xFF << 0)

#define Master_audio_Packet_GP3_Status_Register_1       (0x1880)
#define MASTER_AUD_GP3_STA_1                    (0xFF << 0)

#define Master_audio_Packet_GP3_Status_Register_2       (0x1884)
#define MASTER_AUD_GP3_STA_2                    (0xFF << 0)

#define Master_audio_Packet_GP3_Status_Register_3       (0x1888)
#define MASTER_AUD_GP3_STA_3                    (0xFF << 0)

#define Master_audio_Packet_GP3_Status_Register_4       (0x188C)
#define MASTER_AUD_GP3_STA_4                    (0xFF << 0)

#define CRC_Control_4                           (0x1890)
#define AUD_CRC_PK_NUM                          (0x3FF << 16)
#define AUD_CRC_FLUSH                           (1 << 3)
#define VID_CRC_FLUSH                           (1 << 2)
#define AUD_CRC_EN                              (1 << 1)
#define VID_CRC_EN                              (1 << 0)

#define CRC_Result                              (0x1894)
#define AUD_CRC_RESULT                          (0xFFFF << 16)
#define VID_CRC_RESULT                          (0xFFFF << 0)

#define AUDIO_DMA_Request_Latency_Config        (0x1898)
#define AUD_DMA_ACK_STATUS                      (1 << 21)
#define AUD_DMA_FORCE_ACK                       (1 << 20)
#define AUD_DMA_FORCE_ACK_SEL                   (1 << 19)
#define AUD_DMA_REQ_STATUS                      (1 << 18)
#define AUD_DMA_FORCE_REQ_VAL                   (1 << 17)
#define AUD_DMA_FORCE_REQ_SEL                   (1 << 16)
#define MASTER_DMA_REQ_LTNCY_CONFIG             (0xFFFF << 0)

#define AUX_Ch_MISC_Ctrl_0                      (0x3000)
#define AUX_PN_INV                              (1 << 7)
#define GTC_TX_MASTER                           (1 << 5)
#define GTC_WORK_EN                             (1 << 4)
#define AUX_POWER_DOWN                          (1 << 1)

#define AUX_Ch_MISC_Ctrl_1                      (0x3004)
#define AUX_TC                                  (3 << 6)
#define NEW_PRBS7_INIT_VAL                      (1 << 2)
#define PRBS_ERR_GEN_EN                         (1 << 1)

#define AUX_Ch_MISC_Ctrl_2                      (0x3008)
#define SCRAMBLER_CONTRL_IN                     (1 << 7)
#define SCRAMBLER_IN_EX_IN                      (1 << 6)

#define AUX_Ch_Defer_Ctrl                       (0x3010)
#define DEFER_CTRL_EN                           (1 << 7)
#define DEFER_COUNT                             (0x7F << 0)

#define Req_Addr_0                              (0x3030)
#define REG_REQ_LENGTH                          (0x3F << 24)
#define REG_REQ_COMM                            (0xF << 20)
#define REG_REQ_ADDR                            (0xFFFFF << 0)

#define Req_Op_En                               (0x3040)
#define BUF_CLR                                 (1 << 7)
#define ADDR_ONLY                               (1 << 6)
#define REG_MODE_SEL                            (1 << 5)
#define AUX_OP_EN                               (1 << 4)
#define AUX_REQ_WAIT_GRAND                      (1 << 3)
#define AUX_REQ_SIGNAL                          (1 << 2)

#define AUX_Status_0                            (0x3044)
#define AUX_BUSY                                (1 << 4)
#define AUX_STATUS                              (0xF << 0)

#define AUX_Status_1                            (0x3048)
#define AUX_ERR_NUM                             (0xFF << 0)

#define AUX_Status_2                            (0x304C)
#define LAST_TRANSACTION_MODE                   (1 << 4)
#define AUX_RX_COMM                             (0xF << 0)

#define AUX_Status_3                            (0x3050)
#define BUF_DATA_COUNT                          (0x3F << 0)

#define Read_Error_Ctrl                         (0x3054)
#define READ_ERROR_DATA_TYPE                    (3 << 6)
#define READ_ERROR_TYPE                         (3 << 4)
#define ERROR_CLR                               (1 << 0)

#define RX_CRC_DATAH                            (0x3060)
#define RX_CRC_DATAH_1                          (0xFF << 0)

#define RX_CRC_DATAL                            (0x3064)
#define RX_CRC_DATAL_1                          (0xFF << 0)

#define AUX_FAUX_PDN_CTRL                       (0x3090)
#define PD_FAUX_TX                              (1 << 7)
#define PD_RX_FAUX                              (1 << 6)
#define PD_RX_AUX                               (1 << 5)
#define PD_FAUX_PLL                             (1 << 4)
#define PD_FAUX_TX_AUTO_EN                      (1 << 3)
#define PD_RX_FAUX_AUTO_EN                      (1 << 2)
#define PD_RX_AUX_AUTO_EN                       (1 << 1)
#define PD_FAUX_PLL_AUTO_EN                     (1 << 0)

#define DP_IRQ_VECTOR                           (0x30A0)
#define IRQ_VECTOR                              (0x3F << 0)

#define DP_LINK_STATUS_1                        (0x30A4)
#define SYMBOL_LOCK_1                           (1 << 6)
#define EQ_DONE_1                               (1 << 5)
#define CR_DONE_1                               (1 << 4)
#define SYMBOL_LOCK_0                           (1 << 2)
#define EQ_DONE_0                               (1 << 1)
#define CR_DONE_0                               (1 << 0)

#define DP_LINK_STATUS_2                        (0x30A8)
#define SYMBOL_LOCK_3                           (1 << 6)
#define EQ_DONE_3                               (1 << 5)
#define CR_DONE_3                               (1 << 4)
#define SYMBOL_LOCK_2                           (1 << 2)
#define EQ_DONE_2                               (1 << 1)
#define CR_DONE_2                               (1 << 0)

#define DP_SINK_COUNT                           (0x30AC)
#define CP_READY                                (1 << 6)
#define SINK_COUNT                              (0x3F << 0)

#define DP_SINK_STATUS                          (0x30B0)
#define SINK_STATUS_1                           (1 << 1)
#define SINK_STATUS_0                           (1 << 0)

#define DP_ALIGN_STATUS                         (0x30B4)
#define LINK_STATUS_UPDATED                     (1 << 7)
#define DOWNSTREAM_PORT_STATUS_CHANGED          (1 << 6)
#define INTERLANE_ALIGN_DONE                    (1 << 0)

#define TX_GTC_VAL1                             (0x30C0)
#define TX_GTC_VAL_1                            (0xFF << 0)

#define TX_GTC_VAL2                             (0x30C4)
#define TX_GTC_VAL_2                            (0xFF << 0)

#define TX_GTC_VAL3                             (0x30C8)
#define TX_GTC_VAL_3                            (0xFF << 0)

#define TX_GTC_VAL4                             (0x30CC)
#define TX_GTC_VAL_4                            (0xFF << 0)

#define RX_GTC_VAL1                             (0x30D0)
#define RX_GTC_VAL_1                            (0xFF << 0)

#define RX_GTC_VAL2                             (0x30D4)
#define RX_GTC_VAL_2                            (0xFF << 0)

#define RX_GTC_VAL3                             (0x30D8)
#define RX_GTC_VAL_3                            (0xFF << 0)

#define RX_GTC_VAL4                             (0x30DC)
#define RX_GTC_VAL_4                            (0xFF << 0)

#define FAUX_GTC_STATUS                         (0x30F0)
#define FREQ_ADJ_10_8                           (0x7 << 5)
#define TXGTC_LOCK_DONE_FLAG                    (1 << 1)
#define RXGTC_LOCK_DONE_FLAG                    (1 << 0)

#define FAUX_GTC_FREQ                           (0x30F4)
#define FREQ_ADJ_1                              (0xFF << 0)

#define DEBUG_STATE_SEL_ADD                     (0x30F8)
#define DEBUG_STATE_SEL                         (0xFF << 0)

#define DEBUG_STATE_ADD                         (0x30FC)
#define DEBUG_STATE                             (0xFF << 0)

#define SEND_DATA_0_1_2_3                       (0x3100)
#define SEND_DATA_3                             (0xFF << 24)
#define SEND_DATA_2                             (0xFF << 16)
#define SEND_DATA_1                             (0xFF << 8)
#define SEND_DATA_0                             (0xFF << 0)

#define SEND_DATA_4_5_6_7                       (0x3104)
#define SEND_DATA_7                             (0xFF << 24)
#define SEND_DATA_6                             (0xFF << 16)
#define SEND_DATA_5                             (0xFF << 8)
#define SEND_DATA_4                             (0xFF << 0)

#define SEND_DATA_8_9_10_11                     (0x3108)
#define SEND_DATA_11                            (0xFF << 24)
#define SEND_DATA_10                            (0xFF << 16)
#define SEND_DATA_9                             (0xFF << 8)
#define SEND_DATA_8                             (0xFF << 0)

#define SEND_DATA_12_13_14_15                   (0x310C)
#define SEND_DATA_15                            (0xFF << 24)
#define SEND_DATA_14                            (0xFF << 16)
#define SEND_DATA_13                            (0xFF << 8)
#define SEND_DATA_12                            (0xFF << 0)

#define RECEIVED_DATA_0_1_2_3                   (0x3200)
#define RECEIVED_DATA_3                         (0xFF << 24)
#define RECEIVED_DATA_2                         (0xFF << 16)
#define RECEIVED_DATA_1                         (0xFF << 8)
#define RECEIVED_DATA_0                         (0xFF << 0)

#define RECEIVED_DATA_4_5_6_7                   (0x3204)
#define RECEIVED_DATA_7                         (0xFF << 24)
#define RECEIVED_DATA_6                         (0xFF << 16)
#define RECEIVED_DATA_5                         (0xFF << 8)
#define RECEIVED_DATA_4                         (0xFF << 0)

#define RECEIVED_DATA_8_9_10_11                 (0x3208)
#define RECEIVED_DATA_11                        (0xFF << 24)
#define RECEIVED_DATA_10                        (0xFF << 16)
#define RECEIVED_DATA_9                         (0xFF << 8)
#define RECEIVED_DATA_8                         (0xFF << 0)

#define RECEIVED_DATA_12_13_14_15               (0x320C)
#define RECEIVED_DATA_15                        (0xFF << 24)
#define RECEIVED_DATA_14                        (0xFF << 16)
#define RECEIVED_DATA_13                        (0xFF << 8)
#define RECEIVED_DATA_12                        (0xFF << 0)

#define GTC_TP_Control_Register_0               (0x3300)
#define I_SPEC_GTC_IMPLEMENTATION               (1 << 29)
#define TP_SEL                                  (3 << 27)
#define TP_FRACTIONAL                           (0x7FFFF << 8)
#define SYMBOL_LOCK_2                           (1 << 2)
#define TP_INT1                                 (0xF << 4)
#define TP_INT2                                 (0xF << 0)

#define HDCP_Status_Register                    (0x6400)
#define REAUTH_REQUEST                          (1 << 7)
#define AUTH_FAIL                               (1 << 6)
#define HW_1ST_AUTHEN_PASS                      (1 << 5)
#define BKSV_VALID                              (1 << 3)
#define ENCRYPT                                 (1 << 2)
#define HW_AUTHEN_PASS                          (1 << 1)
#define AKSV_VALID                              (1 << 0)

#define HDCP_Control_Register_0                 (0x6404)
#define SW_STORE_AN                             (1 << 7)
#define SW_RX_REPEATER                          (1 << 6)
#define HW_RE_AUTHEN                            (1 << 5)
#define SW_AUTH_OK                              (1 << 4)
#define HW_AUTH_EN                              (1 << 3)
#define HDCP_ENC_EN                             (1 << 2)
#define HW_1ST_PART_ATHENTICATION_EN            (1 << 1)
#define HW_2ND_PART_ATHENTICATION_EN            (1 << 0)

#define HDCP_Control_Register_1                 (0x6408)
#define DPCD_REV_1_2                            (1 << 3)
#define HW_AUTH_POLLING_MODE                    (1 << 1)
#define HDCP_INT                                (1 << 0)

#define HDCP_AKSV_Register_0                    (0x6414)
#define AKSV0                                   (0xFF << 0)

#define HDCP_AKSV_Register_1                    (0x6418)
#define AKSV1                                   (0xFF << 0)

#define HDCP_AKSV_Register_2                    (0x641C)
#define AKSV2                                   (0xFF << 0)

#define HDCP_AKSV_Register_3                    (0x6420)
#define AKSV3                                   (0xFF << 0)

#define HDCP_AKSV_Register_4                    (0x6424)
#define AKSV3                                   (0xFF << 0)

#define HDCP_AN_Register_0                      (0x6428)
#define AN0                                     (0xFF << 0)

#define HDCP_AN_Register_1                      (0x642C)
#define AN1                                     (0xFF << 0)

#define HDCP_AN_Register_2                      (0x6430)
#define AN2                                     (0xFF << 0)

#define HDCP_AN_Register_3                      (0x6434)
#define AN3                                     (0xFF << 0)

#define HDCP_AN_Register_4                      (0x6438)
#define AN4                                     (0xFF << 0)

#define HDCP_AN_Register_5                      (0x643C)
#define AN5                                     (0xFF << 0)

#define HDCP_AN_Register_6                      (0x6440)
#define AN6                                     (0xFF << 0)

#define HDCP_AN_Register_7                      (0x6444)
#define AN7                                     (0xFF << 0)

#define HDCP_BKSV_Register_0                    (0x6448)
#define BKSV0                                   (0xFF << 0)

#define HDCP_BKSV_Register_1                    (0x644C)
#define BKSV1                                   (0xFF << 0)

#define HDCP_BKSV_Register_2                    (0x6450)
#define BKSV2                                   (0xFF << 0)

#define HDCP_BKSV_Register_3                    (0x6454)
#define BKSV3                                   (0xFF << 0)

#define HDCP_BKSV_Register_4                    (0x6458)
#define BKSV4                                   (0xFF << 0)

#define HDCP_R0_Register_0                      (0x645C)
#define R0_0                                    (0xFF << 0)

#define HDCP_R0_Register_1                      (0x6460)
#define R0_1                                    (0xFF << 0)

#define HDCP_PJ_Register_0                      (0x6464)
#define PJ_0                                    (0xFF << 0)

#define Receiver_BCAPS_Register                 (0x6468)
#define BCAPS                                   (0xFF << 0)

#define HDCP_BINFO_Register_0                   (0x646C)
#define BINFO0                                  (0xFF << 0)

#define HDCP_BINFO_Register_1                   (0x6470)
#define BINFO1                                  (0xFF << 0)

#define HDCP_Debug_Control_Register             (0x6474)
#define CHECK_KSV                               (1 << 2)
#define REVOCATION_CHK_DONE                     (1 << 1)
#define HW_SKIP_RPT_ZERO_DEV                    (1 << 0)

#define HDCP_Debug_Control_Register             (0x6474)
#define CHECK_KSV                               (1 << 2)
#define REVOCATION_CHK_DONE                     (1 << 1)
#define HW_SKIP_RPT_ZERO_DEV                    (1 << 0)

#define SPSRAM_Access_Configure_Register_0      (0x6488)
#define BYPASS_HDCP                             (1 << 7)
#define KEY_MODE                                (1 << 6)

#define HDCP_AUTH_DBG_Register                  (0x649C)
#define HDCP_DDC_STATE                          (7 << 5)
#define HDCP_AUCH_STATE                         (0x1F << 0)

#define HDCP_ENC_DBG_Register                   (0x64A0)
#define HDCP_ENC_STATE                          (7 << 3)

#define HDCP_AM0_Register_0                     (0x64C0)
#define HDCP_AM0_0                              (0xFF << 0)

#define HDCP_AM0_Register_1                     (0x64C4)
#define HDCP_AM0_1                              (0xFF << 0)

#define HDCP_AM0_Register_2                     (0x64C8)
#define HDCP_AM0_2                              (0xFF << 0)

#define HDCP_AM0_Register_3                     (0x64CC)
#define HDCP_AM0_3                              (0xFF << 0)

#define HDCP_AM0_Register_4                     (0x64D0)
#define HDCP_AM0_4                              (0xFF << 0)

#define HDCP_AM0_Register_5                     (0x64D4)
#define HDCP_AM0_5                              (0xFF << 0)

#define HDCP_AM0_Register_6                     (0x64D8)
#define HDCP_AM0_6                              (0xFF << 0)

#define HDCP_AM0_Register_7                     (0x64DC)
#define HDCP_AM0_7                              (0xFF << 0)

#define HDCP_KEY_VALID_STATUS                   (0x64E0)
#define HDCP_KEY_VALID_SYNC_IN_I2C_CLK          (1 << 0)

#define HDCP_Wait_R0_Timing_Register            (0x6500)
#define HW_WRITE_AKSV_WAIT                      (0xFF << 0)

#define Link_Chk_Timer_Register                 (0x6504)
#define LINK_CHK_TIMER                          (0xFF << 0)

#define HDCP_Repeater_Ready_Wait_Timer_Register (0x6508)
#define HW_RPTR_RDY_TIMER                       (0xFF << 0)

#define Ready_Poll_Timer_Register               (0x650C)
#define POLLING_TIMER_TH                        (0xFF << 0)

#define Stream_ID_Encryption_Control_Register   (0x6514)
#define STRM_ID_ENC                             (0x7F << 0)

#define SEC_DP_HDCP22_SYS_EN_APB4               (0x7000)
#define System_Enable                           (1 << 0)

#define SEC_DP_HDCP22_MST_MODE_APB4             (0x7050)
#define MST_Mode                                (1 << 0)

#define SEC_DP_HDCP22_LVP_APB4                  (0x7054)
#define link_verification_pattern               (0xFFFF << 0)

#define SEC_DP_HDCP22_LVP_GEN_APB4              (0x7058)
#define lvp_gen                                 (1 << 0)

#define SEC_DP_HDCP22_LANE_COUNT_APB4           (0x705C)
#define lane_count                              (3 << 0)

#define SEC_DP_HDCP22_STREAM_TYPE_APB4          (0x7060)
#define Stream_type                             (1 << 0)

#define SEC_DP_HDCP22_LVP_CNT_KEEP_APB4         (0x7080)
#define LVP_Count_Keep_Enable                   (1 << 0)

#define SEC_DP_HDCP22_LANE_DECODE_CTRL          (0x7084)
#define Enhanced_Framing_Mode                   (1 << 3)
#define LVP_En_Decode_Enable                    (1 << 2)
#define Encryption_Signal_Decode_Enable         (1 << 1)
#define Lane_Decode_Enable                      (1 << 0)

#define SEC_DP_HDCP22_SR_VALUE                  (0x7088)
#define SR_Value                                (0xFF << 0)

#define SEC_DP_HDCP22_CP_VALUE                  (0x708C)
#define CP_Value                                (0xFF << 0)

#define SEC_DP_HDCP22_BF_VALUE                  (0x7090)
#define BF_Value                                (0xFF << 0)

#define SEC_DP_HDCP22_BS_VALUE                  (0x7094)
#define BS_Value                                (0xFF << 0)

#define SEC_DP_HDCP22_RIV_XOR                   (0x7098)
#define Riv_Xor_Location                        (1 << 0)

#define SEC_DP_HDCP22_RIV_0_APB4                (0x7100)
#define Riv_Key_0_Register                      (0xFFFFFFFF << 0)

#define SEC_DP_HDCP22_RIV_1_APB4                (0x7104)
#define Riv_Key_1_Register                      (0xFFFFFFFF << 0)

#define SEC_DP_HDCP22_KS_KEY_0_APB4_SECURE      (0x7200)
#define Ks_Key_0_Register                       (0xFFFFFFFF << 0)

#define SEC_DP_HDCP22_KS_KEY_1_APB4_SECURE      (0x7204)
#define Ks_Key_1_Register                       (0xFFFFFFFF << 0)

#define SEC_DP_HDCP22_KS_KEY_2_APB4_SECURE      (0x7208)
#define Ks_Key_2_Register                       (0xFFFFFFFF << 0)

#define SEC_DP_HDCP22_KS_KEY_3_APB4_SECURE      (0x720C)
#define Ks_Key_3_Register                       (0xFFFFFFFF << 0)

#define SEC_DP_HDCP22_AES_KEY_0                 (0x7210)
#define AES_Pairing_Key_0_Register              (0xFFFFFFFF << 0)

#define SEC_DP_HDCP22_AES_KEY_1                 (0x7214)
#define AES_Pairing_Key_1_Register              (0xFFFFFFFF << 0)

#define SEC_DP_HDCP22_AES_KEY_2                 (0x7218)
#define AES_Pairing_Key_2_Register              (0xFFFFFFFF << 0)

#define SEC_DP_HDCP22_AES_KEY_3                 (0x721C)
#define AES_Pairing_Key_3_Register              (0xFFFFFFFF << 0)

#define PLL_CON0_PLL_DPU                        (0x0140)
#define ENABLE                                  (1 << 31)
#define USE_HW_LOCK_DET                         (1 << 28)
#define DIV_M                                   (0x3FF << 16)
#define DIV_P                                   (0x3F << 8)
#define MUX_SEL                                 (1 << 4)
#define DIV_S                                   (0x7 << 0)

#define CLK_CON_DIV_DIV_CLKCMU_DPU1_DECON2      (0x1800)
#define BUSY                                    (1 << 16)
#define DIVRATIO                                (7 << 0)
#endif
