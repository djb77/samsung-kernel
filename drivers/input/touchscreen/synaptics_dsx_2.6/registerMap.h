  /* Register map header file for TD4100test, Family 0x01, Revision 0x00 */

  /* Automatically generated on 2016-Apr-24  22:19:37, do not edit */

#ifndef SYNA_REGISTER_MAP_H
  #define SYNA_REGISTER_MAP_H 1

  /*      Register Name                                      Address     Register Description */
  /*      -------------                                      -------     -------------------- */
  #define SYNA_F34_FLASH_DATA00_00_00                        0x0000   /* Block Number LSB */
  #define SYNA_F34_FLASH_DATA00_00_01                        0x0000   /* Block Number MSB */
  #define SYNA_F34_FLASH_DATA01_00_00                        0x0001   /* Block Data 0 */
  #define SYNA_F34_FLASH_DATA01_00_01                        0x0001   /* Block Data 1 */
  #define SYNA_F34_FLASH_DATA01_00_02                        0x0001   /* Block Data 2 */
  #define SYNA_F34_FLASH_DATA01_00_03                        0x0001   /* Block Data 3 */
  #define SYNA_F34_FLASH_DATA01_00_04                        0x0001   /* Block Data 4 */
  #define SYNA_F34_FLASH_DATA01_00_05                        0x0001   /* Block Data 5 */
  #define SYNA_F34_FLASH_DATA01_00_06                        0x0001   /* Block Data 6 */
  #define SYNA_F34_FLASH_DATA01_00_07                        0x0001   /* Block Data 7 */
  #define SYNA_F34_FLASH_DATA01_00_08                        0x0001   /* Block Data 8 */
  #define SYNA_F34_FLASH_DATA01_00_09                        0x0001   /* Block Data 9 */
  #define SYNA_F34_FLASH_DATA01_00_10                        0x0001   /* Block Data 10 */
  #define SYNA_F34_FLASH_DATA01_00_11                        0x0001   /* Block Data 11 */
  #define SYNA_F34_FLASH_DATA01_00_12                        0x0001   /* Block Data 12 */
  #define SYNA_F34_FLASH_DATA01_00_13                        0x0001   /* Block Data 13 */
  #define SYNA_F34_FLASH_DATA01_00_14                        0x0001   /* Block Data 14 */
  #define SYNA_F34_FLASH_DATA01_00_15                        0x0001   /* Block Data 15 */
  #define SYNA_F34_FLASH_DATA02                              0x0002   /* Flash Control */
  #define SYNA_F34_FLASH_DATA03                              0x0003   /* Flash Status */
  #define SYNA_F01_RMI_DATA00                                0x0004   /* Device Status */
  #define SYNA_F01_RMI_DATA01_00                             0x0005   /* Interrupt Status */
  #define SYNA_F12_2D_DATA01_00_00                           0x0006   /* Object Type and Status 0 */
  #define SYNA_F12_2D_DATA01_00_01                           0x0006   /* Object Data 0 */
  #define SYNA_F12_2D_DATA01_00_02                           0x0006   /* Object Data 0 */
  #define SYNA_F12_2D_DATA01_00_03                           0x0006   /* Object Data 0 */
  #define SYNA_F12_2D_DATA01_00_04                           0x0006   /* Object Data 0 */
  #define SYNA_F12_2D_DATA01_00_05                           0x0006   /* Object Data 0 */
  #define SYNA_F12_2D_DATA01_00_06                           0x0006   /* Object Data 0 */
  #define SYNA_F12_2D_DATA01_00_07                           0x0006   /* Object Data 0 */
  #define SYNA_F12_2D_DATA01_01_00                           0x0006   /* Object Type and Status 1 */
  #define SYNA_F12_2D_DATA01_01_01                           0x0006   /* Object Data 1 */
  #define SYNA_F12_2D_DATA01_01_02                           0x0006   /* Object Data 1 */
  #define SYNA_F12_2D_DATA01_01_03                           0x0006   /* Object Data 1 */
  #define SYNA_F12_2D_DATA01_01_04                           0x0006   /* Object Data 1 */
  #define SYNA_F12_2D_DATA01_01_05                           0x0006   /* Object Data 1 */
  #define SYNA_F12_2D_DATA01_01_06                           0x0006   /* Object Data 1 */
  #define SYNA_F12_2D_DATA01_01_07                           0x0006   /* Object Data 1 */
  #define SYNA_F12_2D_DATA01_02_00                           0x0006   /* Object Type and Status 2 */
  #define SYNA_F12_2D_DATA01_02_01                           0x0006   /* Object Data 2 */
  #define SYNA_F12_2D_DATA01_02_02                           0x0006   /* Object Data 2 */
  #define SYNA_F12_2D_DATA01_02_03                           0x0006   /* Object Data 2 */
  #define SYNA_F12_2D_DATA01_02_04                           0x0006   /* Object Data 2 */
  #define SYNA_F12_2D_DATA01_02_05                           0x0006   /* Object Data 2 */
  #define SYNA_F12_2D_DATA01_02_06                           0x0006   /* Object Data 2 */
  #define SYNA_F12_2D_DATA01_02_07                           0x0006   /* Object Data 2 */
  #define SYNA_F12_2D_DATA01_03_00                           0x0006   /* Object Type and Status 3 */
  #define SYNA_F12_2D_DATA01_03_01                           0x0006   /* Object Data 3 */
  #define SYNA_F12_2D_DATA01_03_02                           0x0006   /* Object Data 3 */
  #define SYNA_F12_2D_DATA01_03_03                           0x0006   /* Object Data 3 */
  #define SYNA_F12_2D_DATA01_03_04                           0x0006   /* Object Data 3 */
  #define SYNA_F12_2D_DATA01_03_05                           0x0006   /* Object Data 3 */
  #define SYNA_F12_2D_DATA01_03_06                           0x0006   /* Object Data 3 */
  #define SYNA_F12_2D_DATA01_03_07                           0x0006   /* Object Data 3 */
  #define SYNA_F12_2D_DATA01_04_00                           0x0006   /* Object Type and Status 4 */
  #define SYNA_F12_2D_DATA01_04_01                           0x0006   /* Object Data 4 */
  #define SYNA_F12_2D_DATA01_04_02                           0x0006   /* Object Data 4 */
  #define SYNA_F12_2D_DATA01_04_03                           0x0006   /* Object Data 4 */
  #define SYNA_F12_2D_DATA01_04_04                           0x0006   /* Object Data 4 */
  #define SYNA_F12_2D_DATA01_04_05                           0x0006   /* Object Data 4 */
  #define SYNA_F12_2D_DATA01_04_06                           0x0006   /* Object Data 4 */
  #define SYNA_F12_2D_DATA01_04_07                           0x0006   /* Object Data 4 */
  #define SYNA_F12_2D_DATA01_05_00                           0x0006   /* Object Type and Status 5 */
  #define SYNA_F12_2D_DATA01_05_01                           0x0006   /* Object Data 5 */
  #define SYNA_F12_2D_DATA01_05_02                           0x0006   /* Object Data 5 */
  #define SYNA_F12_2D_DATA01_05_03                           0x0006   /* Object Data 5 */
  #define SYNA_F12_2D_DATA01_05_04                           0x0006   /* Object Data 5 */
  #define SYNA_F12_2D_DATA01_05_05                           0x0006   /* Object Data 5 */
  #define SYNA_F12_2D_DATA01_05_06                           0x0006   /* Object Data 5 */
  #define SYNA_F12_2D_DATA01_05_07                           0x0006   /* Object Data 5 */
  #define SYNA_F12_2D_DATA01_06_00                           0x0006   /* Object Type and Status 6 */
  #define SYNA_F12_2D_DATA01_06_01                           0x0006   /* Object Data 6 */
  #define SYNA_F12_2D_DATA01_06_02                           0x0006   /* Object Data 6 */
  #define SYNA_F12_2D_DATA01_06_03                           0x0006   /* Object Data 6 */
  #define SYNA_F12_2D_DATA01_06_04                           0x0006   /* Object Data 6 */
  #define SYNA_F12_2D_DATA01_06_05                           0x0006   /* Object Data 6 */
  #define SYNA_F12_2D_DATA01_06_06                           0x0006   /* Object Data 6 */
  #define SYNA_F12_2D_DATA01_06_07                           0x0006   /* Object Data 6 */
  #define SYNA_F12_2D_DATA01_07_00                           0x0006   /* Object Type and Status 7 */
  #define SYNA_F12_2D_DATA01_07_01                           0x0006   /* Object Data 7 */
  #define SYNA_F12_2D_DATA01_07_02                           0x0006   /* Object Data 7 */
  #define SYNA_F12_2D_DATA01_07_03                           0x0006   /* Object Data 7 */
  #define SYNA_F12_2D_DATA01_07_04                           0x0006   /* Object Data 7 */
  #define SYNA_F12_2D_DATA01_07_05                           0x0006   /* Object Data 7 */
  #define SYNA_F12_2D_DATA01_07_06                           0x0006   /* Object Data 7 */
  #define SYNA_F12_2D_DATA01_07_07                           0x0006   /* Object Data 7 */
  #define SYNA_F12_2D_DATA01_08_00                           0x0006   /* Object Type and Status 8 */
  #define SYNA_F12_2D_DATA01_08_01                           0x0006   /* Object Data 8 */
  #define SYNA_F12_2D_DATA01_08_02                           0x0006   /* Object Data 8 */
  #define SYNA_F12_2D_DATA01_08_03                           0x0006   /* Object Data 8 */
  #define SYNA_F12_2D_DATA01_08_04                           0x0006   /* Object Data 8 */
  #define SYNA_F12_2D_DATA01_08_05                           0x0006   /* Object Data 8 */
  #define SYNA_F12_2D_DATA01_08_06                           0x0006   /* Object Data 8 */
  #define SYNA_F12_2D_DATA01_08_07                           0x0006   /* Object Data 8 */
  #define SYNA_F12_2D_DATA01_09_00                           0x0006   /* Object Type and Status 9 */
  #define SYNA_F12_2D_DATA01_09_01                           0x0006   /* Object Data 9 */
  #define SYNA_F12_2D_DATA01_09_02                           0x0006   /* Object Data 9 */
  #define SYNA_F12_2D_DATA01_09_03                           0x0006   /* Object Data 9 */
  #define SYNA_F12_2D_DATA01_09_04                           0x0006   /* Object Data 9 */
  #define SYNA_F12_2D_DATA01_09_05                           0x0006   /* Object Data 9 */
  #define SYNA_F12_2D_DATA01_09_06                           0x0006   /* Object Data 9 */
  #define SYNA_F12_2D_DATA01_09_07                           0x0006   /* Object Data 9 */
  #define SYNA_F12_2D_DATA04_00_00                           0x0007   /* Detected Gestures */
  #define SYNA_F12_2D_DATA04_01_00                           0x0007   /* Gesture Properties */
  #define SYNA_F12_2D_DATA04_01_01                           0x0007   /* Gesture Properties */
  #define SYNA_F12_2D_DATA04_01_02                           0x0007   /* Gesture Properties */
  #define SYNA_F12_2D_DATA04_01_03                           0x0007   /* Gesture Properties */
  #define SYNA_F12_2D_DATA15_00_00                           0x0008   /* Object Attention */
  #define SYNA_F12_2D_DATA15_00_01                           0x0008   /* Object Attention */
  #define SYNA_F34_FLASH_CTRL00_00                           0x0009   /* Customer Defined Config ID */
  #define SYNA_F34_FLASH_CTRL00_01                           0x000A   /* Customer Defined Config ID */
  #define SYNA_F34_FLASH_CTRL00_02                           0x000B   /* Customer Defined Config ID */
  #define SYNA_F34_FLASH_CTRL00_03                           0x000C   /* Customer Defined Config ID */
  #define SYNA_F01_RMI_CTRL00                                0x000D   /* Device Control */
  #define SYNA_F01_RMI_CTRL01_00                             0x000E   /* Interrupt Enable 0 */
  #define SYNA_F01_RMI_CTRL02                                0x000F   /* Doze Interval */
  #define SYNA_F01_RMI_CTRL03                                0x0010   /* Doze Wakeup Threshold */
  #define SYNA_F01_RMI_CTRL05                                0x0011   /* Doze Holdoff */
  #define SYNA_F01_RMI_CTRL09                                0x0012   /* Doze Recalibration Interval */
  #define SYNA_F12_2D_CTRL08_00_00                           0x0013   /* Maximum XY Coordinate */
  #define SYNA_F12_2D_CTRL08_00_01                           0x0013   /* Maximum XY Coordinate */
  #define SYNA_F12_2D_CTRL08_00_02                           0x0013   /* Maximum XY Coordinate */
  #define SYNA_F12_2D_CTRL08_00_03                           0x0013   /* Maximum XY Coordinate */
  #define SYNA_F12_2D_CTRL08_01_00                           0x0013   /* Standard Sensor Pitch */
  #define SYNA_F12_2D_CTRL08_01_01                           0x0013   /* Standard Sensor Pitch */
  #define SYNA_F12_2D_CTRL08_01_02                           0x0013   /* Standard Sensor Pitch */
  #define SYNA_F12_2D_CTRL08_01_03                           0x0013   /* Standard Sensor Pitch */
  #define SYNA_F12_2D_CTRL08_02_00                           0x0013   /* Inactive Border Width */
  #define SYNA_F12_2D_CTRL08_02_01                           0x0013   /* Inactive Border Width */
  #define SYNA_F12_2D_CTRL08_02_02                           0x0013   /* Inactive Border Width */
  #define SYNA_F12_2D_CTRL08_02_03                           0x0013   /* Inactive Border Width */
  #define SYNA_F12_2D_CTRL08_03_00                           0x0013   /* Number Of 2D Electrodes Used */
  #define SYNA_F12_2D_CTRL08_03_01                           0x0013   /* Number Of 2D Electrodes Used */
  #define SYNA_F12_2D_CTRL09_00_00                           0x0014   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_01                           0x0014   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_02                           0x0014   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_03                           0x0014   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_04                           0x0014   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_05                           0x0014   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_06                           0x0014   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_01_00                           0x0014   /* W Tuning */
  #define SYNA_F12_2D_CTRL09_01_01                           0x0014   /* W Tuning */
  #define SYNA_F12_2D_CTRL09_01_02                           0x0014   /* W Tuning */
  #define SYNA_F12_2D_CTRL09_01_03                           0x0014   /* W Tuning */
  #define SYNA_F12_2D_CTRL09_02_00                           0x0014   /* Assumed Finger Size */
  #define SYNA_F12_2D_CTRL09_02_01                           0x0014   /* Assumed Finger Size */
  #define SYNA_F12_2D_CTRL09_02_02                           0x0014   /* Assumed Finger Size */
  #define SYNA_F12_2D_CTRL09_02_03                           0x0014   /* Assumed Finger Size */
  #define SYNA_F12_2D_CTRL10_00_00                           0x0015   /* Noise Floor */
  #define SYNA_F12_2D_CTRL10_00_01                           0x0015   /* Minimum Peak Amplitude */
  #define SYNA_F12_2D_CTRL10_00_02                           0x0015   /* Peak Merge Threshold */
  #define SYNA_F12_2D_CTRL10_01_00                           0x0015   /* Drumming */
  #define SYNA_F12_2D_CTRL10_01_01                           0x0015   /* Drumming */
  #define SYNA_F12_2D_CTRL10_04_00                           0x0015   /* Post Segmentation Type A */
  #define SYNA_F12_2D_CTRL10_05_00                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_01                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_02                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_03                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_04                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_05                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_06                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_07                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_08                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_09                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_10                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_11                           0x0015   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL11_00_00                           0x0016   /* Blending */
  #define SYNA_F12_2D_CTRL11_00_01                           0x0016   /* Blending */
  #define SYNA_F12_2D_CTRL11_03_00                           0x0016   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_01                           0x0016   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_02                           0x0016   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_03                           0x0016   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_04                           0x0016   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_05                           0x0016   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_06_00                           0x0016   /* Classifier - for Land/Lift/Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_01                           0x0016   /* Classifier - for Land/Lift/Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_02                           0x0016   /* Classifier - for Land/Lift/Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_03                           0x0016   /* Classifier - for Land/Lift/Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_04                           0x0016   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_05                           0x0016   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_06                           0x0016   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_07                           0x0016   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_08                           0x0016   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_09                           0x0016   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_10                           0x0016   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL15_00_00                           0x0017   /* Finger */
  #define SYNA_F12_2D_CTRL15_00_01                           0x0017   /* Finger */
  #define SYNA_F12_2D_CTRL15_00_02                           0x0017   /* Finger */
  #define SYNA_F12_2D_CTRL15_00_03                           0x0017   /* Finger */
  #define SYNA_F12_2D_CTRL15_01_00                           0x0017   /* Palm */
  #define SYNA_F12_2D_CTRL15_01_01                           0x0017   /* Palm */
  #define SYNA_F12_2D_CTRL15_08_00                           0x0017   /* Finger */
  #define SYNA_F12_2D_CTRL18_01_00                           0x0018   /* Swipe X0 LSB */
  #define SYNA_F12_2D_CTRL18_01_01                           0x0018   /* Swipe X0 MSB */
  #define SYNA_F12_2D_CTRL18_01_02                           0x0018   /* Swipe Y0 LSB */
  #define SYNA_F12_2D_CTRL18_01_03                           0x0018   /* Swipe Y0 MSB */
  #define SYNA_F12_2D_CTRL18_01_04                           0x0018   /* Swipe X1 LSB */
  #define SYNA_F12_2D_CTRL18_01_05                           0x0018   /* Swipe X1 MSB */
  #define SYNA_F12_2D_CTRL18_01_06                           0x0018   /* Swipe Y1 LSB */
  #define SYNA_F12_2D_CTRL18_01_07                           0x0018   /* Swipe Y1 MSB */
  #define SYNA_F12_2D_CTRL18_01_08                           0x0018   /* Swipe Minimum Distance */
  #define SYNA_F12_2D_CTRL18_01_09                           0x0018   /* Swipe Minimum Speed */
  #define SYNA_F12_2D_CTRL20_00_00                           0x0019   /* Motion Suppression */
  #define SYNA_F12_2D_CTRL20_00_01                           0x0019   /* Motion Suppression */
  #define SYNA_F12_2D_CTRL20_01_00                           0x0019   /* Report Flags */
  #define SYNA_F12_2D_CTRL22_00_00                           0x001A   /* Report Filtering Parameters */
  #define SYNA_F12_2D_CTRL23_00_00                           0x001B   /* Object Report Enable */
  #define SYNA_F12_2D_CTRL23_01_00                           0x001B   /* Max Number of Reported Objects */
  #define SYNA_F12_2D_CTRL23_02_00                           0x001B   /* Report as Finger */
  #define SYNA_F12_2D_CTRL23_03_00                           0x001B   /* Object Type Enable 2 */
  #define SYNA_F12_2D_CTRL23_04_00                           0x001B   /* Report as Finger 2 */
  #define SYNA_F12_2D_CTRL27_00_00                           0x001C   /* Wakeup Gesture Enables */
  #define SYNA_F12_2D_CTRL27_01_00                           0x001C   /* Report Rate Wakeup Gesture */
  #define SYNA_F12_2D_CTRL27_01_01                           0x001C   /* False Activation Threshold */
  #define SYNA_F12_2D_CTRL27_01_02                           0x001C   /* Max Active Duration */
  #define SYNA_F12_2D_CTRL27_01_03                           0x001C   /* Timer 1 */
  #define SYNA_F12_2D_CTRL27_01_04                           0x001C   /* Max Active Duration Timeout */
  #define SYNA_F12_2D_CTRL27_05_00                           0x001C   /* Wakeup Gesture Enable 2 */
  #define SYNA_F12_2D_CTRL28_00_00                           0x001D   /* Data Reporting Enable Mask */
  #define SYNA_F12_2D_CTRL29_00_00                           0x001E   /* Small Object Region 0 */
  #define SYNA_F12_2D_CTRL29_00_01                           0x001E   /* Small Object Region 0 */
  #define SYNA_F12_2D_CTRL29_01_00                           0x001E   /* Small Object Region 1 */
  #define SYNA_F12_2D_CTRL29_01_01                           0x001E   /* Small Object Region 1 */
  #define SYNA_F12_2D_CTRL29_02_00                           0x001E   /* Small Object Region 2 */
  #define SYNA_F12_2D_CTRL29_02_01                           0x001E   /* Small Object Region 2 */
  #define SYNA_F12_2D_CTRL29_03_00                           0x001E   /* Small Object Region 3 */
  #define SYNA_F12_2D_CTRL29_03_01                           0x001E   /* Small Object Region 3 */
  #define SYNA_F12_2D_CTRL29_04_00                           0x001E   /* Small Object Region 4 */
  #define SYNA_F12_2D_CTRL29_04_01                           0x001E   /* Small Object Region 4 */
  #define SYNA_F12_2D_CTRL29_05_00                           0x001E   /* Small Object Region 5 */
  #define SYNA_F12_2D_CTRL29_05_01                           0x001E   /* Small Object Region 5 */
  #define SYNA_F12_2D_CTRL29_06_00                           0x001E   /* Small Object Region 6 */
  #define SYNA_F12_2D_CTRL29_06_01                           0x001E   /* Small Object Region 6 */
  #define SYNA_F12_2D_CTRL29_07_00                           0x001E   /* Small Object Region 7 */
  #define SYNA_F12_2D_CTRL29_07_01                           0x001E   /* Small Object Region 7 */
  #define SYNA_F12_2D_CTRL30_00_00                           0x001F   /* Small Object Hysteresis Region 0 */
  #define SYNA_F12_2D_CTRL30_00_01                           0x001F   /* Small Object Hysteresis Region 0 */
  #define SYNA_F12_2D_CTRL30_01_00                           0x001F   /* Small Object Hysteresis Region 1 */
  #define SYNA_F12_2D_CTRL30_01_01                           0x001F   /* Small Object Hysteresis Region 1 */
  #define SYNA_F12_2D_CTRL30_02_00                           0x001F   /* Small Object Hysteresis Region 2 */
  #define SYNA_F12_2D_CTRL30_02_01                           0x001F   /* Small Object Hysteresis Region 2 */
  #define SYNA_F12_2D_CTRL30_03_00                           0x001F   /* Small Object Hysteresis Region 3 */
  #define SYNA_F12_2D_CTRL30_03_01                           0x001F   /* Small Object Hysteresis Region 3 */
  #define SYNA_F12_2D_CTRL30_04_00                           0x001F   /* Small Object Hysteresis Region 4 */
  #define SYNA_F12_2D_CTRL30_04_01                           0x001F   /* Small Object Hysteresis Region 4 */
  #define SYNA_F12_2D_CTRL30_05_00                           0x001F   /* Small Object Hysteresis Region 5 */
  #define SYNA_F12_2D_CTRL30_05_01                           0x001F   /* Small Object Hysteresis Region 5 */
  #define SYNA_F12_2D_CTRL30_06_00                           0x001F   /* Small Object Hysteresis Region 6 */
  #define SYNA_F12_2D_CTRL30_06_01                           0x001F   /* Small Object Hysteresis Region 6 */
  #define SYNA_F12_2D_CTRL30_07_00                           0x001F   /* Small Object Hysteresis Region 7 */
  #define SYNA_F12_2D_CTRL30_07_01                           0x001F   /* Small Object Hysteresis Region 7 */
  #define SYNA_F12_2D_CTRL48_00_00                           0x0020   /* Shadow Removal */
  #define SYNA_F12_2D_CTRL54_00_00                           0x0021   /* ALGM Control */
  #define SYNA_F12_2D_CTRL54_00_01                           0x0021   /* ALGM Control */
  #define SYNA_F12_2D_CTRL54_00_02                           0x0021   /* ALGM Control */
  #define SYNA_F12_2D_CTRL54_00_03                           0x0021   /* ALGM Control */
  #define SYNA_F12_2D_CTRL54_00_04                           0x0021   /* ALGM Control */
  #define SYNA_F01_RMI_CMD00                                 0x0022   /* Device Command */
  #define SYNA_F34_FLASH_QUERY00_00_00                       0x0023   /* Bootloader ID 0 */
  #define SYNA_F34_FLASH_QUERY00_00_01                       0x0023   /* Bootloader ID 1 */
  #define SYNA_F34_FLASH_QUERY00_01_00                       0x0023   /* Bootloader Minor Revision */
  #define SYNA_F34_FLASH_QUERY00_01_01                       0x0023   /* Bootloader Major Revision */
  #define SYNA_F34_FLASH_QUERY00_02_00                       0x0023   /* Firmware Build ID [7:0] */
  #define SYNA_F34_FLASH_QUERY00_02_01                       0x0023   /* Firmware Build ID [15:8] */
  #define SYNA_F34_FLASH_QUERY00_02_02                       0x0023   /* Firmware Build ID [23:16] */
  #define SYNA_F34_FLASH_QUERY00_02_03                       0x0023   /* Firmware Build ID [31:24] */
  #define SYNA_F34_FLASH_QUERY01                             0x0024   /* Flash Properties */
  #define SYNA_F34_FLASH_QUERY02_00_00                       0x0025   /* Block Size 0 */
  #define SYNA_F34_FLASH_QUERY02_00_01                       0x0025   /* Block Size 1 */
  #define SYNA_F34_FLASH_QUERY03_00_00                       0x0026   /* Firmware Block Count 0 */
  #define SYNA_F34_FLASH_QUERY03_00_01                       0x0026   /* Firmware Block Count 1 */
  #define SYNA_F34_FLASH_QUERY03_01_00                       0x0026   /* UIConfig Block Count LSB */
  #define SYNA_F34_FLASH_QUERY03_01_01                       0x0026   /* UIConfig Block Count MSB */
  #define SYNA_F34_FLASH_QUERY03_04_00                       0x0026   /* DisplayConfigBlockCountLSB */
  #define SYNA_F34_FLASH_QUERY03_04_01                       0x0026   /* DisplayConfigBlockCountMSB */
  #define SYNA_F01_RMI_QUERY00                               0x0027   /* Manufacturer ID Query */
  #define SYNA_F01_RMI_QUERY01                               0x0028   /* Product Properties Query */
  #define SYNA_F01_RMI_QUERY02                               0x0029   /* Customer Family Query */
  #define SYNA_F01_RMI_QUERY03                               0x002A   /* Firmware Revision Query */
  #define SYNA_F01_RMI_QUERY04                               0x002B   /* Device Serialization Query 0 */
  #define SYNA_F01_RMI_QUERY05                               0x002C   /* Device Serialization Query 1 */
  #define SYNA_F01_RMI_QUERY06                               0x002D   /* Device Serialization Query 2 */
  #define SYNA_F01_RMI_QUERY07                               0x002E   /* Device Serialization Query 3 */
  #define SYNA_F01_RMI_QUERY08                               0x002F   /* Device Serialization Query 4 */
  #define SYNA_F01_RMI_QUERY09                               0x0030   /* Device Serialization Query 5 */
  #define SYNA_F01_RMI_QUERY10                               0x0031   /* Device Serialization Query 6 */
  #define SYNA_F01_RMI_QUERY11                               0x0032   /* Product ID Query 0 */
  #define SYNA_F01_RMI_QUERY12                               0x0033   /* Product ID Query 1 */
  #define SYNA_F01_RMI_QUERY13                               0x0034   /* Product ID Query 2 */
  #define SYNA_F01_RMI_QUERY14                               0x0035   /* Product ID Query 3 */
  #define SYNA_F01_RMI_QUERY15                               0x0036   /* Product ID Query 4 */
  #define SYNA_F01_RMI_QUERY16                               0x0037   /* Product ID Query 5 */
  #define SYNA_F01_RMI_QUERY17                               0x0038   /* Product ID Query 6 */
  #define SYNA_F01_RMI_QUERY18                               0x0039   /* Product ID Query 7 */
  #define SYNA_F01_RMI_QUERY19                               0x003A   /* Product ID Query 8 */
  #define SYNA_F01_RMI_QUERY20                               0x003B   /* Product ID Query 9 */
  #define SYNA_F01_RMI_QUERY42                               0x003C   /* Product Properties 2 */
  #define SYNA_F01_RMI_QUERY43_00                            0x003D   /* DS4 Queries0 */
  #define SYNA_F01_RMI_QUERY43_01                            0x003E   /* DS4 Queries1 */
  #define SYNA_F01_RMI_QUERY43_02                            0x003F   /* DS4 Queries2 */
  #define SYNA_F01_RMI_QUERY43_03                            0x0040   /* DS4 Queries3 */
  #define SYNA_F01_RMI_QUERY44                               0x0041   /* Reset Query */
  #define SYNA_F01_RMI_QUERY45_00_00                         0x0042   /* DS4 Tool ID 0 */
  #define SYNA_F01_RMI_QUERY45_00_01                         0x0042   /* DS4 Tool ID 1 */
  #define SYNA_F01_RMI_QUERY45_00_02                         0x0042   /* DS4 Tool ID 2 */
  #define SYNA_F01_RMI_QUERY45_00_03                         0x0042   /* DS4 Tool ID 3 */
  #define SYNA_F01_RMI_QUERY45_00_04                         0x0042   /* DS4 Tool ID 4 */
  #define SYNA_F01_RMI_QUERY45_00_05                         0x0042   /* DS4 Tool ID 5 */
  #define SYNA_F01_RMI_QUERY45_00_06                         0x0042   /* DS4 Tool ID 6 */
  #define SYNA_F01_RMI_QUERY45_00_07                         0x0042   /* DS4 Tool ID 7 */
  #define SYNA_F01_RMI_QUERY45_00_08                         0x0042   /* DS4 Tool ID 8 */
  #define SYNA_F01_RMI_QUERY45_00_09                         0x0042   /* DS4 Tool ID 9 */
  #define SYNA_F01_RMI_QUERY45_00_10                         0x0042   /* DS4 Tool ID 10 */
  #define SYNA_F01_RMI_QUERY45_00_11                         0x0042   /* DS4 Tool ID 11 */
  #define SYNA_F01_RMI_QUERY45_00_12                         0x0042   /* DS4 Tool ID 12 */
  #define SYNA_F01_RMI_QUERY45_00_13                         0x0042   /* DS4 Tool ID 13 */
  #define SYNA_F01_RMI_QUERY45_00_14                         0x0042   /* DS4 Tool ID 14 */
  #define SYNA_F01_RMI_QUERY45_00_15                         0x0042   /* DS4 Tool ID 15 */
  #define SYNA_F01_RMI_QUERY46_00_00                         0x0043   /* Firmware Revision ID 0 */
  #define SYNA_F01_RMI_QUERY46_00_01                         0x0043   /* Firmware Revision ID 1 */
  #define SYNA_F01_RMI_QUERY46_00_02                         0x0043   /* Firmware Revision ID 2 */
  #define SYNA_F01_RMI_QUERY46_00_03                         0x0043   /* Firmware Revision ID 3 */
  #define SYNA_F01_RMI_QUERY46_00_04                         0x0043   /* Firmware Revision ID 4 */
  #define SYNA_F01_RMI_QUERY46_00_05                         0x0043   /* Firmware Revision ID 5 */
  #define SYNA_F01_RMI_QUERY46_00_06                         0x0043   /* Firmware Revision ID 6 */
  #define SYNA_F01_RMI_QUERY46_00_07                         0x0043   /* Firmware Revision ID 7 */
  #define SYNA_F01_RMI_QUERY46_00_08                         0x0043   /* Firmware Revision ID 8 */
  #define SYNA_F01_RMI_QUERY46_00_09                         0x0043   /* Firmware Revision ID 9 */
  #define SYNA_F01_RMI_QUERY46_00_10                         0x0043   /* Firmware Revision ID 10 */
  #define SYNA_F01_RMI_QUERY46_00_11                         0x0043   /* Firmware Revision ID 11 */
  #define SYNA_F01_RMI_QUERY46_00_12                         0x0043   /* Firmware Revision ID 12 */
  #define SYNA_F01_RMI_QUERY46_00_13                         0x0043   /* Firmware Revision ID 13 */
  #define SYNA_F01_RMI_QUERY46_00_14                         0x0043   /* Firmware Revision ID 14 */
  #define SYNA_F01_RMI_QUERY46_00_15                         0x0043   /* Firmware Revision ID 15 */
  #define SYNA_F12_2D_QUERY00_00_00                          0x0044   /* General */
  #define SYNA_F12_2D_QUERY01_00_00                          0x0045   /* Size Of Query Presence */
  #define SYNA_F12_2D_QUERY02_00_00                          0x0046   /* Query Presence */
  #define SYNA_F12_2D_QUERY03_00_00                          0x0047   /* Query Structure */
  #define SYNA_F12_2D_QUERY04_00_00                          0x0048   /* Size Of Control Presence */
  #define SYNA_F12_2D_QUERY05_00_00                          0x0049   /* Control Presence */
  #define SYNA_F12_2D_QUERY06_00_00                          0x004A   /* Control Structure */
  #define SYNA_F12_2D_QUERY07_00_00                          0x004B   /* Size Of Data Presence */
  #define SYNA_F12_2D_QUERY08_00_00                          0x004C   /* Data Presence */
  #define SYNA_F12_2D_QUERY09_00_00                          0x004D   /* Data Structure */
  #define SYNA_F12_2D_QUERY10_00_00                          0x004E   /* Supported Object Types */
  #define SYNA_F12_2D_QUERY10_01_00                          0x004E   /* Supported Object Types 2 */

  /* Start of Page Description Table (PDT) */

  #define SYNA_PDT_P00_F12_2D_QUERY_BASE                     0x00DD   /* Query Base */
  #define SYNA_PDT_P00_F12_2D_COMMAND_BASE                   0x00DE   /* Command Base */
  #define SYNA_PDT_P00_F12_2D_CONTROL_BASE                   0x00DF   /* Control Base */
  #define SYNA_PDT_P00_F12_2D_DATA_BASE                      0x00E0   /* Data Base */
  #define SYNA_PDT_P00_F12_2D_INTERRUPTS                     0x00E1   /* Version & Interrupt Count */
  #define SYNA_PDT_P00_F12_2D_EXISTS                         0x00E2   /* Function Exists */
  #define SYNA_PDT_P00_F01_RMI_QUERY_BASE                    0x00E3   /* Query Base */
  #define SYNA_PDT_P00_F01_RMI_COMMAND_BASE                  0x00E4   /* Command Base */
  #define SYNA_PDT_P00_F01_RMI_CONTROL_BASE                  0x00E5   /* Control Base */
  #define SYNA_PDT_P00_F01_RMI_DATA_BASE                     0x00E6   /* Data Base */
  #define SYNA_PDT_P00_F01_RMI_INTERRUPTS                    0x00E7   /* Version & Interrupt Count */
  #define SYNA_PDT_P00_F01_RMI_EXISTS                        0x00E8   /* Function Exists */
  #define SYNA_PDT_P00_F34_FLASH_QUERY_BASE                  0x00E9   /* Query Base */
  #define SYNA_PDT_P00_F34_FLASH_COMMAND_BASE                0x00EA   /* Command Base */
  #define SYNA_PDT_P00_F34_FLASH_CONTROL_BASE                0x00EB   /* Control Base */
  #define SYNA_PDT_P00_F34_FLASH_DATA_BASE                   0x00EC   /* Data Base */
  #define SYNA_PDT_P00_F34_FLASH_INTERRUPTS                  0x00ED   /* Version & Interrupt Count */
  #define SYNA_PDT_P00_F34_FLASH_EXISTS                      0x00EE   /* Function Exists */
  #define SYNA_P00_PDT_PROPERTIES                            0x00EF   /* P00_PDT Properties */
  #define SYNA_P00_PAGESELECT                                0x00FF   /* Page Select register */

  /* Registers on Page 0x01 */

  /*      Register Name                                      Address     Register Description */
  /*      -------------                                      -------     -------------------- */
  #define SYNA_F54_ANALOG_DATA00                             0x0100   /* Report Type */
  #define SYNA_F54_ANALOG_DATA01                             0x0101   /* Report Index LSB */
  #define SYNA_F54_ANALOG_DATA02                             0x0102   /* Report Index MSB */
  #define SYNA_F54_ANALOG_DATA03                             0x0103   /* Report Data */
  #define SYNA_F54_ANALOG_DATA06_00                          0x0104   /* Interference Metric LSB */
  #define SYNA_F54_ANALOG_DATA06_01                          0x0105   /* Interference Metric MSB */
  #define SYNA_F54_ANALOG_DATA07_00                          0x0106   /* Report Rate LSB */
  #define SYNA_F54_ANALOG_DATA07_01                          0x0107   /* Report Rate MSB */
  #define SYNA_F54_ANALOG_DATA10                             0x0108   /* Current Noise State */
  #define SYNA_F54_ANALOG_DATA11                             0x0109   /* Status */
  #define SYNA_F54_ANALOG_DATA16_00_00                       0x010A   /* Freq Scan IM LSB */
  #define SYNA_F54_ANALOG_DATA16_00_01                       0x010A   /* Freq Scan IM MSB */
  #define SYNA_F54_ANALOG_DATA17                             0x010B   /* Sense Frequency Selection */
  #define SYNA_F54_ANALOG_DATA19_00_00                       0x010C   /* Object Size X LSB */
  #define SYNA_F54_ANALOG_DATA19_00_01                       0x010C   /* Object Size X MSB */
  #define SYNA_F54_ANALOG_DATA19_00_02                       0x010C   /* Object Size Y LSB */
  #define SYNA_F54_ANALOG_DATA19_00_03                       0x010C   /* Object Size Y MSB */
  #define SYNA_F54_ANALOG_CTRL00                             0x010D   /* General Control */
  #define SYNA_F54_ANALOG_CTRL02_00                          0x010E   /* Saturation Capacitance LSB */
  #define SYNA_F54_ANALOG_CTRL02_01                          0x010F   /* Saturation Capacitance MSB */
  #define SYNA_F54_ANALOG_CTRL10                             0x0110   /* Noise Measurement Control */
  #define SYNA_F54_ANALOG_CTRL11_00                          0x0111   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL11_01                          0x0112   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL20                             0x0113   /* Noise Mitigation General Control */
  #define SYNA_F54_ANALOG_CTRL22                             0x0114   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL23_00                          0x0115   /* Medium Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL23_01                          0x0116   /* Medium Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL24_00                          0x0117   /* High Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL24_01                          0x0118   /* High Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL25                             0x0119   /* FNM Frequency Shift Density */
  #define SYNA_F54_ANALOG_CTRL26                             0x011A   /* FNM Exit Threshold */
  #define SYNA_F54_ANALOG_CTRL28_00                          0x011B   /* Quiet Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL28_01                          0x011C   /* Quiet Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL29                             0x011D   /* Common-Mode Noise Control */
  #define SYNA_F54_ANALOG_CTRL60                             0x011E   /* EXVCOM_SEL Duration */
  #define SYNA_F54_ANALOG_CTRL61                             0x011F   /* SMOD Parameters 1 */
  #define SYNA_F54_ANALOG_CTRL62                             0x0120   /* SMOD Parameters 2 */
  #define SYNA_F54_ANALOG_CTRL63                             0x0121   /* Noise Mitigation Parameters */
  #define SYNA_F54_ANALOG_CTRL64_00                          0x0122   /* Cycles Per Burst 1 */
  #define SYNA_F54_ANALOG_CTRL65_00                          0x0123   /* Cycles Per Burst 2 */
  #define SYNA_F54_ANALOG_CTRL66                             0x0124   /* Baseline Correction Position */
  #define SYNA_F54_ANALOG_CTRL67                             0x0125   /* Baseline Correction Rows */
  #define SYNA_F54_ANALOG_CTRL68                             0x0126   /* Trigger Delay */
  #define SYNA_F54_ANALOG_CTRL69                             0x0127   /* Trigger Holdoff */
  #define SYNA_F54_ANALOG_CTRL70                             0x0128   /* Transmitter Voltage */
  #define SYNA_F54_ANALOG_CTRL71                             0x0129   /* Receiver Voltage */
  #define SYNA_F54_ANALOG_CTRL72_00                          0x012A   /* Sense Trigger LSB */
  #define SYNA_F54_ANALOG_CTRL72_01                          0x012B   /* Sense Trigger MSB */
  #define SYNA_F54_ANALOG_CTRL73_00                          0x012C   /* Sync Trigger LSB */
  #define SYNA_F54_ANALOG_CTRL73_01                          0x012D   /* Sync Trigger MSB */
  #define SYNA_F54_ANALOG_CTRL86                             0x012E   /* Dynamic Sensing (2x/1x Display) Control */
  #define SYNA_F54_ANALOG_CTRL88                             0x012F   /* Analog Control 1 */
  #define SYNA_F54_ANALOG_CTRL89_01_00                       0x0130   /* FNM Pixel Touch Multiplier */
  #define SYNA_F54_ANALOG_CTRL89_01_01                       0x0130   /* Freq Scan Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL89_01_02                       0x0130   /* Freq Scan Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL89_01_03                       0x0130   /* Quiet IM Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL89_01_04                       0x0130   /* Quiet IM Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL91_00_00                       0x0131   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL91_00_01                       0x0131   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL91_00_02                       0x0131   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL91_00_03                       0x0131   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL91_00_04                       0x0131   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL93_00_00                       0x0132   /* Noise Threshold */
  #define SYNA_F54_ANALOG_CTRL93_00_01                       0x0132   /* Noise Threshold */
  #define SYNA_F54_ANALOG_CTRL94_02_00                       0x0133   /* Trans */
  #define SYNA_F54_ANALOG_CTRL94_03_00                       0x0133   /* Noise */
  #define SYNA_F54_ANALOG_CTRL95_00_00                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_01                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_02                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_03                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_04                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_05                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_06                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_07                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_08                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_09                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_10                       0x0134   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_01_00                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_01                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_02                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_03                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_04                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_05                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_06                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_07                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_08                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_09                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_10                       0x0134   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_02_00                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_01                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_02                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_03                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_04                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_05                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_06                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_07                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_08                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_09                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_10                       0x0134   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_03_00                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_01                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_02                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_03                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_04                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_05                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_06                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_07                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_08                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_09                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_10                       0x0134   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_04_00                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_01                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_02                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_03                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_04                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_05                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_06                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_07                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_08                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_09                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_10                       0x0134   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_05_00                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_01                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_02                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_03                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_04                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_05                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_06                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_07                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_08                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_09                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_10                       0x0134   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_06_00                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_01                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_02                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_03                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_04                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_05                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_06                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_07                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_08                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_09                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_10                       0x0134   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_07_00                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_01                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_02                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_03                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_04                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_05                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_06                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_07                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_08                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_09                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_10                       0x0134   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_08_00                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_01                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_02                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_03                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_04                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_05                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_06                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_07                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_08                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_09                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_10                       0x0134   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_09_00                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_01                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_02                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_03                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_04                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_05                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_06                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_07                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_08                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_09                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_10                       0x0134   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_10_00                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_01                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_02                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_03                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_04                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_05                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_06                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_07                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_08                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_09                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_10                       0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_11_00                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_01                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_02                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_03                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_04                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_05                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_06                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_07                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_08                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_09                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_10                       0x0134   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL96_00_00                       0x0135   /* CBC Transcap 0 */
  #define SYNA_F54_ANALOG_CTRL96_00_01                       0x0135   /* CBC Transcap 1 */
  #define SYNA_F54_ANALOG_CTRL96_00_02                       0x0135   /* CBC Transcap 2 */
  #define SYNA_F54_ANALOG_CTRL96_00_03                       0x0135   /* CBC Transcap 3 */
  #define SYNA_F54_ANALOG_CTRL96_00_04                       0x0135   /* CBC Transcap 4 */
  #define SYNA_F54_ANALOG_CTRL96_00_05                       0x0135   /* CBC Transcap 5 */
  #define SYNA_F54_ANALOG_CTRL96_00_06                       0x0135   /* CBC Transcap 6 */
  #define SYNA_F54_ANALOG_CTRL96_00_07                       0x0135   /* CBC Transcap 7 */
  #define SYNA_F54_ANALOG_CTRL96_00_08                       0x0135   /* CBC Transcap 8 */
  #define SYNA_F54_ANALOG_CTRL96_00_09                       0x0135   /* CBC Transcap 9 */
  #define SYNA_F54_ANALOG_CTRL96_00_10                       0x0135   /* CBC Transcap 10 */
  #define SYNA_F54_ANALOG_CTRL96_00_11                       0x0135   /* CBC Transcap 11 */
  #define SYNA_F54_ANALOG_CTRL96_00_12                       0x0135   /* CBC Transcap 12 */
  #define SYNA_F54_ANALOG_CTRL96_00_13                       0x0135   /* CBC Transcap 13 */
  #define SYNA_F54_ANALOG_CTRL96_00_14                       0x0135   /* CBC Transcap 14 */
  #define SYNA_F54_ANALOG_CTRL96_00_15                       0x0135   /* CBC Transcap 15 */
  #define SYNA_F54_ANALOG_CTRL96_00_16                       0x0135   /* CBC Transcap 16 */
  #define SYNA_F54_ANALOG_CTRL96_00_17                       0x0135   /* CBC Transcap 17 */
  #define SYNA_F54_ANALOG_CTRL96_00_18                       0x0135   /* CBC Transcap 18 */
  #define SYNA_F54_ANALOG_CTRL96_00_19                       0x0135   /* CBC Transcap 19 */
  #define SYNA_F54_ANALOG_CTRL96_00_20                       0x0135   /* CBC Transcap 20 */
  #define SYNA_F54_ANALOG_CTRL96_00_21                       0x0135   /* CBC Transcap 21 */
  #define SYNA_F54_ANALOG_CTRL96_00_22                       0x0135   /* CBC Transcap 22 */
  #define SYNA_F54_ANALOG_CTRL96_00_23                       0x0135   /* CBC Transcap 23 */
  #define SYNA_F54_ANALOG_CTRL96_00_24                       0x0135   /* CBC Transcap 24 */
  #define SYNA_F54_ANALOG_CTRL96_00_25                       0x0135   /* CBC Transcap 25 */
  #define SYNA_F54_ANALOG_CTRL96_00_26                       0x0135   /* CBC Transcap 26 */
  #define SYNA_F54_ANALOG_CTRL96_00_27                       0x0135   /* CBC Transcap 27 */
  #define SYNA_F54_ANALOG_CTRL96_00_28                       0x0135   /* CBC Transcap 28 */
  #define SYNA_F54_ANALOG_CTRL96_00_29                       0x0135   /* CBC Transcap 29 */
  #define SYNA_F54_ANALOG_CTRL96_00_30                       0x0135   /* CBC Transcap 30 */
  #define SYNA_F54_ANALOG_CTRL96_00_31                       0x0135   /* CBC Transcap 31 */
  #define SYNA_F54_ANALOG_CTRL96_00_32                       0x0135   /* CBC Transcap 32 */
  #define SYNA_F54_ANALOG_CTRL96_00_33                       0x0135   /* CBC Transcap 33 */
  #define SYNA_F54_ANALOG_CTRL96_00_34                       0x0135   /* CBC Transcap 34 */
  #define SYNA_F54_ANALOG_CTRL96_00_35                       0x0135   /* CBC Transcap 35 */
  #define SYNA_F54_ANALOG_CTRL96_00_36                       0x0135   /* CBC Transcap 36 */
  #define SYNA_F54_ANALOG_CTRL96_00_37                       0x0135   /* CBC Transcap 37 */
  #define SYNA_F54_ANALOG_CTRL96_00_38                       0x0135   /* CBC Transcap 38 */
  #define SYNA_F54_ANALOG_CTRL96_00_39                       0x0135   /* CBC Transcap 39 */
  #define SYNA_F54_ANALOG_CTRL96_00_40                       0x0135   /* CBC Transcap 40 */
  #define SYNA_F54_ANALOG_CTRL96_00_41                       0x0135   /* CBC Transcap 41 */
  #define SYNA_F54_ANALOG_CTRL96_00_42                       0x0135   /* CBC Transcap 42 */
  #define SYNA_F54_ANALOG_CTRL96_00_43                       0x0135   /* CBC Transcap 43 */
  #define SYNA_F54_ANALOG_CTRL96_00_44                       0x0135   /* CBC Transcap 44 */
  #define SYNA_F54_ANALOG_CTRL96_00_45                       0x0135   /* CBC Transcap 45 */
  #define SYNA_F54_ANALOG_CTRL96_00_46                       0x0135   /* CBC Transcap 46 */
  #define SYNA_F54_ANALOG_CTRL96_00_47                       0x0135   /* CBC Transcap 47 */
  #define SYNA_F54_ANALOG_CTRL96_00_48                       0x0135   /* CBC Transcap 48 */
  #define SYNA_F54_ANALOG_CTRL96_00_49                       0x0135   /* CBC Transcap 49 */
  #define SYNA_F54_ANALOG_CTRL96_00_50                       0x0135   /* CBC Transcap 50 */
  #define SYNA_F54_ANALOG_CTRL96_00_51                       0x0135   /* CBC Transcap 51 */
  #define SYNA_F54_ANALOG_CTRL96_00_52                       0x0135   /* CBC Transcap 52 */
  #define SYNA_F54_ANALOG_CTRL96_00_53                       0x0135   /* CBC Transcap 53 */
  #define SYNA_F54_ANALOG_CTRL96_00_54                       0x0135   /* CBC Transcap 54 */
  #define SYNA_F54_ANALOG_CTRL96_00_55                       0x0135   /* CBC Transcap 55 */
  #define SYNA_F54_ANALOG_CTRL96_00_56                       0x0135   /* CBC Transcap 56 */
  #define SYNA_F54_ANALOG_CTRL96_00_57                       0x0135   /* CBC Transcap 57 */
  #define SYNA_F54_ANALOG_CTRL96_00_58                       0x0135   /* CBC Transcap 58 */
  #define SYNA_F54_ANALOG_CTRL96_00_59                       0x0135   /* CBC Transcap 59 */
  #define SYNA_F54_ANALOG_CTRL96_00_60                       0x0135   /* CBC Transcap 60 */
  #define SYNA_F54_ANALOG_CTRL96_00_61                       0x0135   /* CBC Transcap 61 */
  #define SYNA_F54_ANALOG_CTRL96_00_62                       0x0135   /* CBC Transcap 62 */
  #define SYNA_F54_ANALOG_CTRL96_00_63                       0x0135   /* CBC Transcap 63 */
  #define SYNA_F54_ANALOG_CTRL99_00_00                       0x0136   /* Waveform Duration */
  #define SYNA_F54_ANALOG_CTRL99_00_01                       0x0136   /* Waveform Duration */
  #define SYNA_F54_ANALOG_CTRL99_01_00                       0x0136   /* Reset Duration */
  #define SYNA_F54_ANALOG_CTRL100                            0x0137   /* Bias current */
  #define SYNA_F54_ANALOG_CTRL101_00_00                      0x0138   /* Thermal Update Interval LSB */
  #define SYNA_F54_ANALOG_CTRL101_00_01                      0x0138   /* Thermal Update Interval MSB */
  #define SYNA_F54_ANALOG_CTRL101_00_02                      0x0138   /* Fast Half Life */
  #define SYNA_F54_ANALOG_CTRL101_00_03                      0x0138   /* Hold Fast Transition */
  #define SYNA_F54_ANALOG_CTRL101_00_04                      0x0138   /* Energy Ratio Threshold */
  #define SYNA_F54_ANALOG_CTRL103_00_00                      0x0139   /* TDDI CBC Enable Controls */
  #define SYNA_F54_ANALOG_CTRL103_00_01                      0x0139   /* TDDI CBC Parameters */
  #define SYNA_F54_ANALOG_CTRL103_00_02                      0x0139   /* TDDI Local Reference Channel CBC Parameters */
  #define SYNA_F54_ANALOG_CTRL103_01_00                      0x0139   /* TDDI Global CBC values for left/right channels */
  #define SYNA_F54_ANALOG_CTRL103_02_00                      0x0139   /* TDDI Reference Global CBC cap adjustment */
  #define SYNA_F54_ANALOG_CTRL121_00_00                      0x013A   /* HNM Rate Shift Frame Count */
  #define SYNA_F54_ANALOG_CTRL121_00_01                      0x013A   /* HNM Rate Shift Frame Count */
  #define SYNA_F54_ANALOG_CTRL121_00_02                      0x013A   /* Time Between Frequency Scans */
  #define SYNA_F54_ANALOG_CTRL182_00_00                      0x013B   /* CBC Timing Control Tx On Count LSB */
  #define SYNA_F54_ANALOG_CTRL182_00_01                      0x013B   /* CBC Timing Control Tx On Count MSB */
  #define SYNA_F54_ANALOG_CTRL182_00_02                      0x013B   /* CBC Timing Control Rx On Count LSB */
  #define SYNA_F54_ANALOG_CTRL182_00_03                      0x013B   /* CBC Timing Control Rx On Count MSB */
  #define SYNA_F54_ANALOG_CTRL183                            0x013C   /* DAC_IN sets VHi VLo Voltages */
  #define SYNA_F54_ANALOG_CTRL196_00_00                      0x013D   /* Boost Control 0 */
  #define SYNA_F54_ANALOG_CTRL196_00_01                      0x013D   /* Boost Control 1 */
  #define SYNA_F54_ANALOG_CTRL196_00_02                      0x013D   /* Boost Control 2 */
  #define SYNA_F54_ANALOG_CTRL218                            0x013E   /* Misc Host Control */
  #define SYNA_F54_ANALOG_CMD00                              0x013F   /* Analog Command 0 */
  #define SYNA_F54_ANALOG_QUERY00                            0x0140   /* Device Capability Information */
  #define SYNA_F54_ANALOG_QUERY01                            0x0141   /* Device Capability Information */
  #define SYNA_F54_ANALOG_QUERY02                            0x0142   /* Image Reporting Modes */
  #define SYNA_F54_ANALOG_QUERY03_00                         0x0143   /* Clock Rate LSB */
  #define SYNA_F54_ANALOG_QUERY03_01                         0x0144   /* Clock Rate MSB */
  #define SYNA_F54_ANALOG_QUERY04                            0x0145   /* Touch Controller Family */
  #define SYNA_F54_ANALOG_QUERY05                            0x0146   /* Analog Hardware Controls */
  #define SYNA_F54_ANALOG_QUERY06                            0x0147   /* Data Acquisition Controls */
  #define SYNA_F54_ANALOG_QUERY07                            0x0148   /* Curved Lens Compensation */
  #define SYNA_F54_ANALOG_QUERY08                            0x0149   /* Data Acquisition Post-Processing Controls */
  #define SYNA_F54_ANALOG_QUERY09                            0x014A   /* Tuning Control 1 */
  #define SYNA_F54_ANALOG_QUERY10                            0x014B   /* Tuning Control 2 */
  #define SYNA_F54_ANALOG_QUERY11                            0x014C   /* Tuning Control 3 */
  #define SYNA_F54_ANALOG_QUERY13                            0x014D   /* Tuning Control 4 */
  #define SYNA_F54_ANALOG_QUERY15                            0x014E   /* Tuning Control 5 */
  #define SYNA_F54_ANALOG_QUERY16                            0x014F   /* Tuning Control 6 */
  #define SYNA_F54_ANALOG_QUERY17                            0x0150   /* Number of Sense Frequencies */
  #define SYNA_F54_ANALOG_QUERY18                            0x0151   /* Tuning Control 7 */
  #define SYNA_F54_ANALOG_QUERY19                            0x0152   /* Size of Ctrl95 */
  #define SYNA_F54_ANALOG_QUERY21                            0x0153   /* Tuning control 8 */
  #define SYNA_F54_ANALOG_QUERY22                            0x0154   /* Tuning Control 9 */
  #define SYNA_F54_ANALOG_QUERY25                            0x0155   /* Tuning Control 10 */
  #define SYNA_F54_ANALOG_QUERY26                            0x0156   /* Ctrl103 Subpackets */
  #define SYNA_F54_ANALOG_QUERY27                            0x0157   /* Tuning Control 12 */
  #define SYNA_F54_ANALOG_QUERY29                            0x0158   /* Tuning Control 13 */
  #define SYNA_F54_ANALOG_QUERY30                            0x0159   /* Tuning Control 14 */
  #define SYNA_F54_ANALOG_QUERY32                            0x015A   /* Tuning Control 15 */
  #define SYNA_F54_ANALOG_QUERY33                            0x015B   /* Tuning Control 16 */
  #define SYNA_F54_ANALOG_QUERY36                            0x015C   /* Tuning Control 18 */
  #define SYNA_F54_ANALOG_QUERY38                            0x015D   /* Tuning Control 19 */
  #define SYNA_F54_ANALOG_QUERY39                            0x015E   /* Tuning Control 20 */
  #define SYNA_F54_ANALOG_QUERY40                            0x015F   /* Tuning Control 21 */
  #define SYNA_F54_ANALOG_QUERY43                            0x0160   /* Tuning Control 22 */
  #define SYNA_F54_ANALOG_QUERY46                            0x0161   /* Tuning Control 23 */
  #define SYNA_F54_ANALOG_QUERY47                            0x0162   /* Tuning Control 24 */
  #define SYNA_F54_ANALOG_QUERY49                            0x0163   /* Tuning Control 25 */
  #define SYNA_F54_ANALOG_QUERY50                            0x0164   /* Tuning Control 26 */
  #define SYNA_F54_ANALOG_QUERY51                            0x0165   /* Tuning Control 27 */
  #define SYNA_F54_ANALOG_QUERY55                            0x0166   /* Tuning Control 28 */
  #define SYNA_F54_ANALOG_QUERY57                            0x0167   /* Tuning Control 29 */
  #define SYNA_F54_ANALOG_QUERY58                            0x0168   /* Tuning Control 30 */
  #define SYNA_F54_ANALOG_QUERY61                            0x0169   /* Tuning Control 31 */

  /* Start of Page Description Table (PDT) for Page 0x01 */

  #define SYNA_PDT_P01_F54_ANALOG_QUERY_BASE                 0x01E9   /* Query Base */
  #define SYNA_PDT_P01_F54_ANALOG_COMMAND_BASE               0x01EA   /* Command Base */
  #define SYNA_PDT_P01_F54_ANALOG_CONTROL_BASE               0x01EB   /* Control Base */
  #define SYNA_PDT_P01_F54_ANALOG_DATA_BASE                  0x01EC   /* Data Base */
  #define SYNA_PDT_P01_F54_ANALOG_INTERRUPTS                 0x01ED   /* Version & Interrupt Count */
  #define SYNA_PDT_P01_F54_ANALOG_EXISTS                     0x01EE   /* Function Exists */
  #define SYNA_P01_PDT_PROPERTIES                            0x01EF   /* P01_PDT Properties */
  #define SYNA_P01_PAGESELECT                                0x01FF   /* Page Select register */

  /* Registers on Page 0x03 */

  /*      Register Name                                      Address     Register Description */
  /*      -------------                                      -------     -------------------- */
  #define SYNA_F55_SENSOR_CTRL00                             0x0300   /* General Control */
  #define SYNA_F55_SENSOR_CTRL01_00_00                       0x0301   /* Sensor Rx Mapping 0 */
  #define SYNA_F55_SENSOR_CTRL01_00_01                       0x0301   /* Sensor Rx Mapping 1 */
  #define SYNA_F55_SENSOR_CTRL01_00_02                       0x0301   /* Sensor Rx Mapping 2 */
  #define SYNA_F55_SENSOR_CTRL01_00_03                       0x0301   /* Sensor Rx Mapping 3 */
  #define SYNA_F55_SENSOR_CTRL01_00_04                       0x0301   /* Sensor Rx Mapping 4 */
  #define SYNA_F55_SENSOR_CTRL01_00_05                       0x0301   /* Sensor Rx Mapping 5 */
  #define SYNA_F55_SENSOR_CTRL01_00_06                       0x0301   /* Sensor Rx Mapping 6 */
  #define SYNA_F55_SENSOR_CTRL01_00_07                       0x0301   /* Sensor Rx Mapping 7 */
  #define SYNA_F55_SENSOR_CTRL01_00_08                       0x0301   /* Sensor Rx Mapping 8 */
  #define SYNA_F55_SENSOR_CTRL01_00_09                       0x0301   /* Sensor Rx Mapping 9 */
  #define SYNA_F55_SENSOR_CTRL01_00_10                       0x0301   /* Sensor Rx Mapping 10 */
  #define SYNA_F55_SENSOR_CTRL01_00_11                       0x0301   /* Sensor Rx Mapping 11 */
  #define SYNA_F55_SENSOR_CTRL01_00_12                       0x0301   /* Sensor Rx Mapping 12 */
  #define SYNA_F55_SENSOR_CTRL01_00_13                       0x0301   /* Sensor Rx Mapping 13 */
  #define SYNA_F55_SENSOR_CTRL01_00_14                       0x0301   /* Sensor Rx Mapping 14 */
  #define SYNA_F55_SENSOR_CTRL01_00_15                       0x0301   /* Sensor Rx Mapping 15 */
  #define SYNA_F55_SENSOR_CTRL01_00_16                       0x0301   /* Sensor Rx Mapping 16 */
  #define SYNA_F55_SENSOR_CTRL01_00_17                       0x0301   /* Sensor Rx Mapping 17 */
  #define SYNA_F55_SENSOR_CTRL01_00_18                       0x0301   /* Sensor Rx Mapping 18 */
  #define SYNA_F55_SENSOR_CTRL01_00_19                       0x0301   /* Sensor Rx Mapping 19 */
  #define SYNA_F55_SENSOR_CTRL01_00_20                       0x0301   /* Sensor Rx Mapping 20 */
  #define SYNA_F55_SENSOR_CTRL01_00_21                       0x0301   /* Sensor Rx Mapping 21 */
  #define SYNA_F55_SENSOR_CTRL01_00_22                       0x0301   /* Sensor Rx Mapping 22 */
  #define SYNA_F55_SENSOR_CTRL01_00_23                       0x0301   /* Sensor Rx Mapping 23 */
  #define SYNA_F55_SENSOR_CTRL01_00_24                       0x0301   /* Sensor Rx Mapping 24 */
  #define SYNA_F55_SENSOR_CTRL01_00_25                       0x0301   /* Sensor Rx Mapping 25 */
  #define SYNA_F55_SENSOR_CTRL01_00_26                       0x0301   /* Sensor Rx Mapping 26 */
  #define SYNA_F55_SENSOR_CTRL01_00_27                       0x0301   /* Sensor Rx Mapping 27 */
  #define SYNA_F55_SENSOR_CTRL01_00_28                       0x0301   /* Sensor Rx Mapping 28 */
  #define SYNA_F55_SENSOR_CTRL01_00_29                       0x0301   /* Sensor Rx Mapping 29 */
  #define SYNA_F55_SENSOR_CTRL01_00_30                       0x0301   /* Sensor Rx Mapping 30 */
  #define SYNA_F55_SENSOR_CTRL01_00_31                       0x0301   /* Sensor Rx Mapping 31 */
  #define SYNA_F55_SENSOR_CTRL02_00_00                       0x0302   /* Sensor Tx Mapping 0 */
  #define SYNA_F55_SENSOR_CTRL02_00_01                       0x0302   /* Sensor Tx Mapping 1 */
  #define SYNA_F55_SENSOR_CTRL02_00_02                       0x0302   /* Sensor Tx Mapping 2 */
  #define SYNA_F55_SENSOR_CTRL02_00_03                       0x0302   /* Sensor Tx Mapping 3 */
  #define SYNA_F55_SENSOR_CTRL02_00_04                       0x0302   /* Sensor Tx Mapping 4 */
  #define SYNA_F55_SENSOR_CTRL02_00_05                       0x0302   /* Sensor Tx Mapping 5 */
  #define SYNA_F55_SENSOR_CTRL02_00_06                       0x0302   /* Sensor Tx Mapping 6 */
  #define SYNA_F55_SENSOR_CTRL02_00_07                       0x0302   /* Sensor Tx Mapping 7 */
  #define SYNA_F55_SENSOR_CTRL02_00_08                       0x0302   /* Sensor Tx Mapping 8 */
  #define SYNA_F55_SENSOR_CTRL02_00_09                       0x0302   /* Sensor Tx Mapping 9 */
  #define SYNA_F55_SENSOR_CTRL03_00_00                       0x0303   /* Edge Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL03_00_01                       0x0303   /* Edge Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL03_00_02                       0x0303   /* Edge Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL03_00_03                       0x0303   /* Edge Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL03_00_04                       0x0303   /* Edge Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL03_00_05                       0x0303   /* Edge Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL03_00_06                       0x0303   /* Edge Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL03_00_07                       0x0303   /* Edge Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL10_00_00                       0x0304   /* Corner Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL10_00_01                       0x0304   /* Corner Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL10_00_02                       0x0304   /* Corner Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL10_00_03                       0x0304   /* Corner Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL10_00_04                       0x0304   /* Corner Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL10_00_05                       0x0304   /* Corner Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL10_00_06                       0x0304   /* Corner Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL10_00_07                       0x0304   /* Corner Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL43_00_00                       0x0305   /* AMP Sensor Characteristics 2 */
  #define SYNA_F55_SENSOR_CTRL43_01_00                       0x0305   /* AMP Sensor Characteristics 2 */
  #define SYNA_F55_SENSOR_CTRL44_00_00                       0x0306   /* AMP Sensor MUX Swirling Order 0 */
  #define SYNA_F55_SENSOR_CTRL44_00_01                       0x0306   /* AMP Sensor MUX Swirling Order 1 */
  #define SYNA_F55_SENSOR_CTRL44_00_02                       0x0306   /* AMP Sensor MUX Swirling Order 2 */
  #define SYNA_F55_SENSOR_CTRL44_00_03                       0x0306   /* AMP Sensor MUX Swirling Order 3 */
  #define SYNA_F55_SENSOR_CTRL44_00_04                       0x0306   /* AMP Sensor MUX Swirling Order 4 */
  #define SYNA_F55_SENSOR_CTRL44_00_05                       0x0306   /* AMP Sensor MUX Swirling Order 5 */
  #define SYNA_F55_SENSOR_CTRL44_00_06                       0x0306   /* AMP Sensor MUX Swirling Order 6 */
  #define SYNA_F55_SENSOR_CTRL44_00_07                       0x0306   /* AMP Sensor MUX Swirling Order 7 */
  #define SYNA_F55_SENSOR_CTRL44_00_08                       0x0306   /* AMP Sensor MUX Swirling Order 8 */
  #define SYNA_F55_SENSOR_CTRL44_00_09                       0x0306   /* AMP Sensor MUX Swirling Order 9 */
  #define SYNA_F55_SENSOR_CMD00                              0x0307   /* Analog Command 0 */
  #define SYNA_F55_SENSOR_QUERY00                            0x0308   /* Sensor Information */
  #define SYNA_F55_SENSOR_QUERY01                            0x0309   /* Sensor Information */
  #define SYNA_F55_SENSOR_QUERY02                            0x030A   /* Sensor Controls */
  #define SYNA_F55_SENSOR_QUERY05                            0x030B   /* Physical Characteristics2 */
  #define SYNA_F55_SENSOR_QUERY17                            0x030C   /* Physical Characteristics 3 */
  #define SYNA_F55_SENSOR_QUERY18                            0x030D   /* Physical Characteristics 4 */
  #define SYNA_F55_SENSOR_QUERY22                            0x030E   /* Physical Characteristics 5 */
  #define SYNA_F55_SENSOR_QUERY28                            0x030F   /* Physical Characteristics 6 */
  #define SYNA_F55_SENSOR_QUERY30                            0x0310   /* Physical Characteristics 7 */
  #define SYNA_F55_SENSOR_QUERY33                            0x0311   /* Physical Characteristics 8 */

  /* Start of Page Description Table (PDT) for Page 0x03 */

  #define SYNA_PDT_P03_F55_SENSOR_QUERY_BASE                 0x03E9   /* Query Base */
  #define SYNA_PDT_P03_F55_SENSOR_COMMAND_BASE               0x03EA   /* Command Base */
  #define SYNA_PDT_P03_F55_SENSOR_CONTROL_BASE               0x03EB   /* Control Base */
  #define SYNA_PDT_P03_F55_SENSOR_DATA_BASE                  0x03EC   /* Data Base */
  #define SYNA_PDT_P03_F55_SENSOR_INTERRUPTS                 0x03ED   /* Version & Interrupt Count */
  #define SYNA_PDT_P03_F55_SENSOR_EXISTS                     0x03EE   /* Function Exists */
  #define SYNA_P03_PDT_PROPERTIES                            0x03EF   /* P03_PDT Properties */
  #define SYNA_P03_PAGESELECT                                0x03FF   /* Page Select register */

  /* Registers on Page 0x04 */

  /*      Register Name                                      Address     Register Description */
  /*      -------------                                      -------     -------------------- */
  #define SYNA_F51_CUSTOM_CTRL22                             0x0400   /* Palm Filter Mode3 Distance Threshold LSB */
  #define SYNA_F51_CUSTOM_CTRL23                             0x0401   /* Palm Filter Mode3 Distance Threshold MSB */
  #define SYNA_F51_CUSTOM_QUERY00                            0x0402   /* F$51 Query Register Count */
  #define SYNA_F51_CUSTOM_QUERY01                            0x0403   /* F$51 Data Register Count */
  #define SYNA_F51_CUSTOM_QUERY02                            0x0404   /* F$51 Control Register Count */
  #define SYNA_F51_CUSTOM_QUERY03                            0x0405   /* F$51 Command Register Count */
  #define SYNA_F51_CUSTOM_QUERY04                            0x0406   /* MultiTap LPWG */
  #define SYNA_F51_CUSTOM_QUERY05                            0x0407   /* Misc Control */
  #define SYNA_F51_CUSTOM_QUERY06                            0x0408   /* LGM Thresholds */
  #define SYNA_F51_CUSTOM_QUERY07                            0x0409   /* Force Detection */

  /* Start of Page Description Table (PDT) for Page 0x04 */

  #define SYNA_PDT_P04_F51_CUSTOM_QUERY_BASE                 0x04E9   /* Query Base */
  #define SYNA_PDT_P04_F51_CUSTOM_COMMAND_BASE               0x04EA   /* Command Base */
  #define SYNA_PDT_P04_F51_CUSTOM_CONTROL_BASE               0x04EB   /* Control Base */
  #define SYNA_PDT_P04_F51_CUSTOM_DATA_BASE                  0x04EC   /* Data Base */
  #define SYNA_PDT_P04_F51_CUSTOM_INTERRUPTS                 0x04ED   /* Version & Interrupt Count */
  #define SYNA_PDT_P04_F51_CUSTOM_EXISTS                     0x04EE   /* Function Exists */
  #define SYNA_P04_PDT_PROPERTIES                            0x04EF   /* P04_PDT Properties */
  #define SYNA_P04_PAGESELECT                                0x04FF   /* Page Select register */

  /* Registers on Page 0x05 */

  /*      Register Name                                      Address     Register Description */
  /*      -------------                                      -------     -------------------- */
  #define SYNA_FDC_PRINT_DATA00_00                           0x0500   /* Length LSB */
  #define SYNA_FDC_PRINT_DATA00_01                           0x0501   /* Length MSB */
  #define SYNA_FDC_PRINT_DATA01                              0x0502   /* Print Data */
  #define SYNA_FDC_PRINT_CTRL00                              0x0503   /* Print Control */
  #define SYNA_FDC_PRINT_QUERY00                             0x0504   /* General */

  /* Start of Page Description Table (PDT) for Page 0x05 */

  #define SYNA_PDT_P05_FDC_PRINT_QUERY_BASE                  0x05E9   /* Query Base */
  #define SYNA_PDT_P05_FDC_PRINT_COMMAND_BASE                0x05EA   /* Command Base */
  #define SYNA_PDT_P05_FDC_PRINT_CONTROL_BASE                0x05EB   /* Control Base */
  #define SYNA_PDT_P05_FDC_PRINT_DATA_BASE                   0x05EC   /* Data Base */
  #define SYNA_PDT_P05_FDC_PRINT_INTERRUPTS                  0x05ED   /* Version & Interrupt Count */
  #define SYNA_PDT_P05_FDC_PRINT_EXISTS                      0x05EE   /* Function Exists */
  #define SYNA_P05_PDT_PROPERTIES                            0x05EF   /* P05_PDT Properties */
  #define SYNA_P05_PAGESELECT                                0x05FF   /* Page Select register */

  /* Offsets within the configuration block */

  /*      Register Name                                      Offset      Register Description */
  /*      -------------                                      ------      -------------------- */
  #define SYNA_F34_FLASH_CTRL00_00_CFGBLK_OFS                0x0000   /* Customer Defined Config ID */
  #define SYNA_F34_FLASH_CTRL00_01_CFGBLK_OFS                0x0001   /* Customer Defined Config ID */
  #define SYNA_F34_FLASH_CTRL00_02_CFGBLK_OFS                0x0002   /* Customer Defined Config ID */
  #define SYNA_F34_FLASH_CTRL00_03_CFGBLK_OFS                0x0003   /* Customer Defined Config ID */
  #define SYNA_F01_RMI_CTRL00_CFGBLK_OFS                     0x0004   /* Device Control */
  #define SYNA_F01_RMI_CTRL01_00_CFGBLK_OFS                  0x0005   /* Interrupt Enable 0 */
  #define SYNA_F01_RMI_CTRL02_CFGBLK_OFS                     0x0006   /* Doze Interval */
  #define SYNA_F01_RMI_CTRL03_CFGBLK_OFS                     0x0007   /* Doze Wakeup Threshold */
  #define SYNA_F01_RMI_CTRL05_CFGBLK_OFS                     0x0008   /* Doze Holdoff */
  #define SYNA_F01_RMI_CTRL09_CFGBLK_OFS                     0x0009   /* Doze Recalibration Interval */
  #define SYNA_F12_2D_CTRL08_00_00_CFGBLK_OFS                0x000A   /* Maximum XY Coordinate */
  #define SYNA_F12_2D_CTRL08_00_01_CFGBLK_OFS                0x000B   /* Maximum XY Coordinate */
  #define SYNA_F12_2D_CTRL08_00_02_CFGBLK_OFS                0x000C   /* Maximum XY Coordinate */
  #define SYNA_F12_2D_CTRL08_00_03_CFGBLK_OFS                0x000D   /* Maximum XY Coordinate */
  #define SYNA_F12_2D_CTRL08_01_00_CFGBLK_OFS                0x000E   /* Standard Sensor Pitch */
  #define SYNA_F12_2D_CTRL08_01_01_CFGBLK_OFS                0x000F   /* Standard Sensor Pitch */
  #define SYNA_F12_2D_CTRL08_01_02_CFGBLK_OFS                0x0010   /* Standard Sensor Pitch */
  #define SYNA_F12_2D_CTRL08_01_03_CFGBLK_OFS                0x0011   /* Standard Sensor Pitch */
  #define SYNA_F12_2D_CTRL08_02_00_CFGBLK_OFS                0x0012   /* Inactive Border Width */
  #define SYNA_F12_2D_CTRL08_02_01_CFGBLK_OFS                0x0013   /* Inactive Border Width */
  #define SYNA_F12_2D_CTRL08_02_02_CFGBLK_OFS                0x0014   /* Inactive Border Width */
  #define SYNA_F12_2D_CTRL08_02_03_CFGBLK_OFS                0x0015   /* Inactive Border Width */
  #define SYNA_F12_2D_CTRL08_03_00_CFGBLK_OFS                0x0016   /* Number Of 2D Electrodes Used */
  #define SYNA_F12_2D_CTRL08_03_01_CFGBLK_OFS                0x0017   /* Number Of 2D Electrodes Used */
  #define SYNA_F12_2D_CTRL09_00_00_CFGBLK_OFS                0x0018   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_01_CFGBLK_OFS                0x0019   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_02_CFGBLK_OFS                0x001A   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_03_CFGBLK_OFS                0x001B   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_04_CFGBLK_OFS                0x001C   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_05_CFGBLK_OFS                0x001D   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_00_06_CFGBLK_OFS                0x001E   /* Z Tuning */
  #define SYNA_F12_2D_CTRL09_01_00_CFGBLK_OFS                0x001F   /* W Tuning */
  #define SYNA_F12_2D_CTRL09_01_01_CFGBLK_OFS                0x0020   /* W Tuning */
  #define SYNA_F12_2D_CTRL09_01_02_CFGBLK_OFS                0x0021   /* W Tuning */
  #define SYNA_F12_2D_CTRL09_01_03_CFGBLK_OFS                0x0022   /* W Tuning */
  #define SYNA_F12_2D_CTRL09_02_00_CFGBLK_OFS                0x0023   /* Assumed Finger Size */
  #define SYNA_F12_2D_CTRL09_02_01_CFGBLK_OFS                0x0024   /* Assumed Finger Size */
  #define SYNA_F12_2D_CTRL09_02_02_CFGBLK_OFS                0x0025   /* Assumed Finger Size */
  #define SYNA_F12_2D_CTRL09_02_03_CFGBLK_OFS                0x0026   /* Assumed Finger Size */
  #define SYNA_F12_2D_CTRL10_00_00_CFGBLK_OFS                0x0027   /* Noise Floor */
  #define SYNA_F12_2D_CTRL10_00_01_CFGBLK_OFS                0x0028   /* Minimum Peak Amplitude */
  #define SYNA_F12_2D_CTRL10_00_02_CFGBLK_OFS                0x0029   /* Peak Merge Threshold */
  #define SYNA_F12_2D_CTRL10_01_00_CFGBLK_OFS                0x002A   /* Drumming */
  #define SYNA_F12_2D_CTRL10_01_01_CFGBLK_OFS                0x002B   /* Drumming */
  #define SYNA_F12_2D_CTRL10_04_00_CFGBLK_OFS                0x002C   /* Post Segmentation Type A */
  #define SYNA_F12_2D_CTRL10_05_00_CFGBLK_OFS                0x002D   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_01_CFGBLK_OFS                0x002E   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_02_CFGBLK_OFS                0x002F   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_03_CFGBLK_OFS                0x0030   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_04_CFGBLK_OFS                0x0031   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_05_CFGBLK_OFS                0x0032   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_06_CFGBLK_OFS                0x0033   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_07_CFGBLK_OFS                0x0034   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_08_CFGBLK_OFS                0x0035   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_09_CFGBLK_OFS                0x0036   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_10_CFGBLK_OFS                0x0037   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL10_05_11_CFGBLK_OFS                0x0038   /* Post Segmentation Type B */
  #define SYNA_F12_2D_CTRL11_00_00_CFGBLK_OFS                0x0039   /* Blending */
  #define SYNA_F12_2D_CTRL11_00_01_CFGBLK_OFS                0x003A   /* Blending */
  #define SYNA_F12_2D_CTRL11_03_00_CFGBLK_OFS                0x003B   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_01_CFGBLK_OFS                0x003C   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_02_CFGBLK_OFS                0x003D   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_03_CFGBLK_OFS                0x003E   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_04_CFGBLK_OFS                0x003F   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_03_05_CFGBLK_OFS                0x0040   /* Linearity Correction */
  #define SYNA_F12_2D_CTRL11_06_00_CFGBLK_OFS                0x0041   /* Classifier - for Land/Lift/Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_01_CFGBLK_OFS                0x0042   /* Classifier - for Land/Lift/Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_02_CFGBLK_OFS                0x0043   /* Classifier - for Land/Lift/Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_03_CFGBLK_OFS                0x0044   /* Classifier - for Land/Lift/Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_04_CFGBLK_OFS                0x0045   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_05_CFGBLK_OFS                0x0046   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_06_CFGBLK_OFS                0x0047   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_07_CFGBLK_OFS                0x0048   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_08_CFGBLK_OFS                0x0049   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_09_CFGBLK_OFS                0x004A   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL11_06_10_CFGBLK_OFS                0x004B   /* Land/Lift Jitter Filter */
  #define SYNA_F12_2D_CTRL15_00_00_CFGBLK_OFS                0x004C   /* Finger */
  #define SYNA_F12_2D_CTRL15_00_01_CFGBLK_OFS                0x004D   /* Finger */
  #define SYNA_F12_2D_CTRL15_00_02_CFGBLK_OFS                0x004E   /* Finger */
  #define SYNA_F12_2D_CTRL15_00_03_CFGBLK_OFS                0x004F   /* Finger */
  #define SYNA_F12_2D_CTRL15_01_00_CFGBLK_OFS                0x0050   /* Palm */
  #define SYNA_F12_2D_CTRL15_01_01_CFGBLK_OFS                0x0051   /* Palm */
  #define SYNA_F12_2D_CTRL15_08_00_CFGBLK_OFS                0x0052   /* Finger */
  #define SYNA_F12_2D_CTRL18_01_00_CFGBLK_OFS                0x0053   /* Swipe X0 LSB */
  #define SYNA_F12_2D_CTRL18_01_01_CFGBLK_OFS                0x0054   /* Swipe X0 MSB */
  #define SYNA_F12_2D_CTRL18_01_02_CFGBLK_OFS                0x0055   /* Swipe Y0 LSB */
  #define SYNA_F12_2D_CTRL18_01_03_CFGBLK_OFS                0x0056   /* Swipe Y0 MSB */
  #define SYNA_F12_2D_CTRL18_01_04_CFGBLK_OFS                0x0057   /* Swipe X1 LSB */
  #define SYNA_F12_2D_CTRL18_01_05_CFGBLK_OFS                0x0058   /* Swipe X1 MSB */
  #define SYNA_F12_2D_CTRL18_01_06_CFGBLK_OFS                0x0059   /* Swipe Y1 LSB */
  #define SYNA_F12_2D_CTRL18_01_07_CFGBLK_OFS                0x005A   /* Swipe Y1 MSB */
  #define SYNA_F12_2D_CTRL18_01_08_CFGBLK_OFS                0x005B   /* Swipe Minimum Distance */
  #define SYNA_F12_2D_CTRL18_01_09_CFGBLK_OFS                0x005C   /* Swipe Minimum Speed */
  #define SYNA_F12_2D_CTRL20_00_00_CFGBLK_OFS                0x005D   /* Motion Suppression */
  #define SYNA_F12_2D_CTRL20_00_01_CFGBLK_OFS                0x005E   /* Motion Suppression */
  #define SYNA_F12_2D_CTRL20_01_00_CFGBLK_OFS                0x005F   /* Report Flags */
  #define SYNA_F12_2D_CTRL22_00_00_CFGBLK_OFS                0x0060   /* Report Filtering Parameters */
  #define SYNA_F12_2D_CTRL23_00_00_CFGBLK_OFS                0x0061   /* Object Report Enable */
  #define SYNA_F12_2D_CTRL23_01_00_CFGBLK_OFS                0x0062   /* Max Number of Reported Objects */
  #define SYNA_F12_2D_CTRL23_02_00_CFGBLK_OFS                0x0063   /* Report as Finger */
  #define SYNA_F12_2D_CTRL23_03_00_CFGBLK_OFS                0x0064   /* Object Type Enable 2 */
  #define SYNA_F12_2D_CTRL23_04_00_CFGBLK_OFS                0x0065   /* Report as Finger 2 */
  #define SYNA_F12_2D_CTRL27_00_00_CFGBLK_OFS                0x0066   /* Wakeup Gesture Enables */
  #define SYNA_F12_2D_CTRL27_01_00_CFGBLK_OFS                0x0067   /* Report Rate Wakeup Gesture */
  #define SYNA_F12_2D_CTRL27_01_01_CFGBLK_OFS                0x0068   /* False Activation Threshold */
  #define SYNA_F12_2D_CTRL27_01_02_CFGBLK_OFS                0x0069   /* Max Active Duration */
  #define SYNA_F12_2D_CTRL27_01_03_CFGBLK_OFS                0x006A   /* Timer 1 */
  #define SYNA_F12_2D_CTRL27_01_04_CFGBLK_OFS                0x006B   /* Max Active Duration Timeout */
  #define SYNA_F12_2D_CTRL27_05_00_CFGBLK_OFS                0x006C   /* Wakeup Gesture Enable 2 */
  #define SYNA_F12_2D_CTRL28_00_00_CFGBLK_OFS                0x006D   /* Data Reporting Enable Mask */
  #define SYNA_F12_2D_CTRL29_00_00_CFGBLK_OFS                0x006E   /* Small Object Region 0 */
  #define SYNA_F12_2D_CTRL29_00_01_CFGBLK_OFS                0x006F   /* Small Object Region 0 */
  #define SYNA_F12_2D_CTRL29_01_00_CFGBLK_OFS                0x0070   /* Small Object Region 1 */
  #define SYNA_F12_2D_CTRL29_01_01_CFGBLK_OFS                0x0071   /* Small Object Region 1 */
  #define SYNA_F12_2D_CTRL29_02_00_CFGBLK_OFS                0x0072   /* Small Object Region 2 */
  #define SYNA_F12_2D_CTRL29_02_01_CFGBLK_OFS                0x0073   /* Small Object Region 2 */
  #define SYNA_F12_2D_CTRL29_03_00_CFGBLK_OFS                0x0074   /* Small Object Region 3 */
  #define SYNA_F12_2D_CTRL29_03_01_CFGBLK_OFS                0x0075   /* Small Object Region 3 */
  #define SYNA_F12_2D_CTRL29_04_00_CFGBLK_OFS                0x0076   /* Small Object Region 4 */
  #define SYNA_F12_2D_CTRL29_04_01_CFGBLK_OFS                0x0077   /* Small Object Region 4 */
  #define SYNA_F12_2D_CTRL29_05_00_CFGBLK_OFS                0x0078   /* Small Object Region 5 */
  #define SYNA_F12_2D_CTRL29_05_01_CFGBLK_OFS                0x0079   /* Small Object Region 5 */
  #define SYNA_F12_2D_CTRL29_06_00_CFGBLK_OFS                0x007A   /* Small Object Region 6 */
  #define SYNA_F12_2D_CTRL29_06_01_CFGBLK_OFS                0x007B   /* Small Object Region 6 */
  #define SYNA_F12_2D_CTRL29_07_00_CFGBLK_OFS                0x007C   /* Small Object Region 7 */
  #define SYNA_F12_2D_CTRL29_07_01_CFGBLK_OFS                0x007D   /* Small Object Region 7 */
  #define SYNA_F12_2D_CTRL30_00_00_CFGBLK_OFS                0x007E   /* Small Object Hysteresis Region 0 */
  #define SYNA_F12_2D_CTRL30_00_01_CFGBLK_OFS                0x007F   /* Small Object Hysteresis Region 0 */
  #define SYNA_F12_2D_CTRL30_01_00_CFGBLK_OFS                0x0080   /* Small Object Hysteresis Region 1 */
  #define SYNA_F12_2D_CTRL30_01_01_CFGBLK_OFS                0x0081   /* Small Object Hysteresis Region 1 */
  #define SYNA_F12_2D_CTRL30_02_00_CFGBLK_OFS                0x0082   /* Small Object Hysteresis Region 2 */
  #define SYNA_F12_2D_CTRL30_02_01_CFGBLK_OFS                0x0083   /* Small Object Hysteresis Region 2 */
  #define SYNA_F12_2D_CTRL30_03_00_CFGBLK_OFS                0x0084   /* Small Object Hysteresis Region 3 */
  #define SYNA_F12_2D_CTRL30_03_01_CFGBLK_OFS                0x0085   /* Small Object Hysteresis Region 3 */
  #define SYNA_F12_2D_CTRL30_04_00_CFGBLK_OFS                0x0086   /* Small Object Hysteresis Region 4 */
  #define SYNA_F12_2D_CTRL30_04_01_CFGBLK_OFS                0x0087   /* Small Object Hysteresis Region 4 */
  #define SYNA_F12_2D_CTRL30_05_00_CFGBLK_OFS                0x0088   /* Small Object Hysteresis Region 5 */
  #define SYNA_F12_2D_CTRL30_05_01_CFGBLK_OFS                0x0089   /* Small Object Hysteresis Region 5 */
  #define SYNA_F12_2D_CTRL30_06_00_CFGBLK_OFS                0x008A   /* Small Object Hysteresis Region 6 */
  #define SYNA_F12_2D_CTRL30_06_01_CFGBLK_OFS                0x008B   /* Small Object Hysteresis Region 6 */
  #define SYNA_F12_2D_CTRL30_07_00_CFGBLK_OFS                0x008C   /* Small Object Hysteresis Region 7 */
  #define SYNA_F12_2D_CTRL30_07_01_CFGBLK_OFS                0x008D   /* Small Object Hysteresis Region 7 */
  #define SYNA_F12_2D_CTRL48_00_00_CFGBLK_OFS                0x008E   /* Shadow Removal */
  #define SYNA_F12_2D_CTRL54_00_00_CFGBLK_OFS                0x008F   /* ALGM Control */
  #define SYNA_F12_2D_CTRL54_00_01_CFGBLK_OFS                0x0090   /* ALGM Control */
  #define SYNA_F12_2D_CTRL54_00_02_CFGBLK_OFS                0x0091   /* ALGM Control */
  #define SYNA_F12_2D_CTRL54_00_03_CFGBLK_OFS                0x0092   /* ALGM Control */
  #define SYNA_F12_2D_CTRL54_00_04_CFGBLK_OFS                0x0093   /* ALGM Control */
  #define SYNA_F54_ANALOG_CTRL00_CFGBLK_OFS                  0x0094   /* General Control */
  #define SYNA_F54_ANALOG_CTRL02_00_CFGBLK_OFS               0x0095   /* Saturation Capacitance LSB */
  #define SYNA_F54_ANALOG_CTRL02_01_CFGBLK_OFS               0x0096   /* Saturation Capacitance MSB */
  #define SYNA_F54_ANALOG_CTRL10_CFGBLK_OFS                  0x0097   /* Noise Measurement Control */
  #define SYNA_F54_ANALOG_CTRL11_00_CFGBLK_OFS               0x0098   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL11_01_CFGBLK_OFS               0x0099   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL20_CFGBLK_OFS                  0x009A   /* Noise Mitigation General Control */
  #define SYNA_F54_ANALOG_CTRL22_CFGBLK_OFS                  0x009B   /* Reserved */
  #define SYNA_F54_ANALOG_CTRL23_00_CFGBLK_OFS               0x009C   /* Medium Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL23_01_CFGBLK_OFS               0x009D   /* Medium Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL24_00_CFGBLK_OFS               0x009E   /* High Noise Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL24_01_CFGBLK_OFS               0x009F   /* High Noise Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL25_CFGBLK_OFS                  0x00A0   /* FNM Frequency Shift Density */
  #define SYNA_F54_ANALOG_CTRL26_CFGBLK_OFS                  0x00A1   /* FNM Exit Threshold */
  #define SYNA_F54_ANALOG_CTRL28_00_CFGBLK_OFS               0x00A2   /* Quiet Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL28_01_CFGBLK_OFS               0x00A3   /* Quiet Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL29_CFGBLK_OFS                  0x00A4   /* Common-Mode Noise Control */
  #define SYNA_F54_ANALOG_CTRL60_CFGBLK_OFS                  0x00A5   /* EXVCOM_SEL Duration */
  #define SYNA_F54_ANALOG_CTRL61_CFGBLK_OFS                  0x00A6   /* SMOD Parameters 1 */
  #define SYNA_F54_ANALOG_CTRL62_CFGBLK_OFS                  0x00A7   /* SMOD Parameters 2 */
  #define SYNA_F54_ANALOG_CTRL63_CFGBLK_OFS                  0x00A8   /* Noise Mitigation Parameters */
  #define SYNA_F54_ANALOG_CTRL64_00_CFGBLK_OFS               0x00A9   /* Cycles Per Burst 1 */
  #define SYNA_F54_ANALOG_CTRL65_00_CFGBLK_OFS               0x00AA   /* Cycles Per Burst 2 */
  #define SYNA_F54_ANALOG_CTRL66_CFGBLK_OFS                  0x00AB   /* Baseline Correction Position */
  #define SYNA_F54_ANALOG_CTRL67_CFGBLK_OFS                  0x00AC   /* Baseline Correction Rows */
  #define SYNA_F54_ANALOG_CTRL68_CFGBLK_OFS                  0x00AD   /* Trigger Delay */
  #define SYNA_F54_ANALOG_CTRL69_CFGBLK_OFS                  0x00AE   /* Trigger Holdoff */
  #define SYNA_F54_ANALOG_CTRL70_CFGBLK_OFS                  0x00AF   /* Transmitter Voltage */
  #define SYNA_F54_ANALOG_CTRL71_CFGBLK_OFS                  0x00B0   /* Receiver Voltage */
  #define SYNA_F54_ANALOG_CTRL72_00_CFGBLK_OFS               0x00B1   /* Sense Trigger LSB */
  #define SYNA_F54_ANALOG_CTRL72_01_CFGBLK_OFS               0x00B2   /* Sense Trigger MSB */
  #define SYNA_F54_ANALOG_CTRL73_00_CFGBLK_OFS               0x00B3   /* Sync Trigger LSB */
  #define SYNA_F54_ANALOG_CTRL73_01_CFGBLK_OFS               0x00B4   /* Sync Trigger MSB */
  #define SYNA_F54_ANALOG_CTRL86_CFGBLK_OFS                  0x00B5   /* Dynamic Sensing (2x/1x Display) Control */
  #define SYNA_F54_ANALOG_CTRL88_CFGBLK_OFS                  0x00B6   /* Analog Control 1 */
  #define SYNA_F54_ANALOG_CTRL89_01_00_CFGBLK_OFS            0x00B7   /* FNM Pixel Touch Multiplier */
  #define SYNA_F54_ANALOG_CTRL89_01_01_CFGBLK_OFS            0x00B8   /* Freq Scan Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL89_01_02_CFGBLK_OFS            0x00B9   /* Freq Scan Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL89_01_03_CFGBLK_OFS            0x00BA   /* Quiet IM Threshold LSB */
  #define SYNA_F54_ANALOG_CTRL89_01_04_CFGBLK_OFS            0x00BB   /* Quiet IM Threshold MSB */
  #define SYNA_F54_ANALOG_CTRL91_00_00_CFGBLK_OFS            0x00BC   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL91_00_01_CFGBLK_OFS            0x00BD   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL91_00_02_CFGBLK_OFS            0x00BE   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL91_00_03_CFGBLK_OFS            0x00BF   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL91_00_04_CFGBLK_OFS            0x00C0   /* Analog Control 2 */
  #define SYNA_F54_ANALOG_CTRL93_00_00_CFGBLK_OFS            0x00C1   /* Noise Threshold */
  #define SYNA_F54_ANALOG_CTRL93_00_01_CFGBLK_OFS            0x00C2   /* Noise Threshold */
  #define SYNA_F54_ANALOG_CTRL94_02_00_CFGBLK_OFS            0x00C3   /* Trans */
  #define SYNA_F54_ANALOG_CTRL94_03_00_CFGBLK_OFS            0x00C4   /* Noise */
  #define SYNA_F54_ANALOG_CTRL95_00_00_CFGBLK_OFS            0x00C5   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_01_CFGBLK_OFS            0x00C6   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_02_CFGBLK_OFS            0x00C7   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_03_CFGBLK_OFS            0x00C8   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_04_CFGBLK_OFS            0x00C9   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_05_CFGBLK_OFS            0x00CA   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_06_CFGBLK_OFS            0x00CB   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_07_CFGBLK_OFS            0x00CC   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_08_CFGBLK_OFS            0x00CD   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_09_CFGBLK_OFS            0x00CE   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_00_10_CFGBLK_OFS            0x00CF   /* Frequency Control 0 */
  #define SYNA_F54_ANALOG_CTRL95_01_00_CFGBLK_OFS            0x00D0   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_01_CFGBLK_OFS            0x00D1   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_02_CFGBLK_OFS            0x00D2   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_03_CFGBLK_OFS            0x00D3   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_04_CFGBLK_OFS            0x00D4   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_05_CFGBLK_OFS            0x00D5   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_06_CFGBLK_OFS            0x00D6   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_07_CFGBLK_OFS            0x00D7   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_08_CFGBLK_OFS            0x00D8   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_09_CFGBLK_OFS            0x00D9   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_01_10_CFGBLK_OFS            0x00DA   /* Frequency Control 1 */
  #define SYNA_F54_ANALOG_CTRL95_02_00_CFGBLK_OFS            0x00DB   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_01_CFGBLK_OFS            0x00DC   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_02_CFGBLK_OFS            0x00DD   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_03_CFGBLK_OFS            0x00DE   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_04_CFGBLK_OFS            0x00DF   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_05_CFGBLK_OFS            0x00E0   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_06_CFGBLK_OFS            0x00E1   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_07_CFGBLK_OFS            0x00E2   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_08_CFGBLK_OFS            0x00E3   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_09_CFGBLK_OFS            0x00E4   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_02_10_CFGBLK_OFS            0x00E5   /* Frequency Control 2 */
  #define SYNA_F54_ANALOG_CTRL95_03_00_CFGBLK_OFS            0x00E6   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_01_CFGBLK_OFS            0x00E7   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_02_CFGBLK_OFS            0x00E8   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_03_CFGBLK_OFS            0x00E9   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_04_CFGBLK_OFS            0x00EA   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_05_CFGBLK_OFS            0x00EB   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_06_CFGBLK_OFS            0x00EC   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_07_CFGBLK_OFS            0x00ED   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_08_CFGBLK_OFS            0x00EE   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_09_CFGBLK_OFS            0x00EF   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_03_10_CFGBLK_OFS            0x00F0   /* Frequency Control 3 */
  #define SYNA_F54_ANALOG_CTRL95_04_00_CFGBLK_OFS            0x00F1   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_01_CFGBLK_OFS            0x00F2   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_02_CFGBLK_OFS            0x00F3   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_03_CFGBLK_OFS            0x00F4   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_04_CFGBLK_OFS            0x00F5   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_05_CFGBLK_OFS            0x00F6   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_06_CFGBLK_OFS            0x00F7   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_07_CFGBLK_OFS            0x00F8   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_08_CFGBLK_OFS            0x00F9   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_09_CFGBLK_OFS            0x00FA   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_04_10_CFGBLK_OFS            0x00FB   /* Frequency Control 4 */
  #define SYNA_F54_ANALOG_CTRL95_05_00_CFGBLK_OFS            0x00FC   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_01_CFGBLK_OFS            0x00FD   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_02_CFGBLK_OFS            0x00FE   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_03_CFGBLK_OFS            0x00FF   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_04_CFGBLK_OFS            0x0100   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_05_CFGBLK_OFS            0x0101   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_06_CFGBLK_OFS            0x0102   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_07_CFGBLK_OFS            0x0103   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_08_CFGBLK_OFS            0x0104   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_09_CFGBLK_OFS            0x0105   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_05_10_CFGBLK_OFS            0x0106   /* Frequency Control 5 */
  #define SYNA_F54_ANALOG_CTRL95_06_00_CFGBLK_OFS            0x0107   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_01_CFGBLK_OFS            0x0108   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_02_CFGBLK_OFS            0x0109   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_03_CFGBLK_OFS            0x010A   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_04_CFGBLK_OFS            0x010B   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_05_CFGBLK_OFS            0x010C   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_06_CFGBLK_OFS            0x010D   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_07_CFGBLK_OFS            0x010E   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_08_CFGBLK_OFS            0x010F   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_09_CFGBLK_OFS            0x0110   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_06_10_CFGBLK_OFS            0x0111   /* Frequency Control 6 */
  #define SYNA_F54_ANALOG_CTRL95_07_00_CFGBLK_OFS            0x0112   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_01_CFGBLK_OFS            0x0113   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_02_CFGBLK_OFS            0x0114   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_03_CFGBLK_OFS            0x0115   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_04_CFGBLK_OFS            0x0116   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_05_CFGBLK_OFS            0x0117   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_06_CFGBLK_OFS            0x0118   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_07_CFGBLK_OFS            0x0119   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_08_CFGBLK_OFS            0x011A   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_09_CFGBLK_OFS            0x011B   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_07_10_CFGBLK_OFS            0x011C   /* Frequency Control 7 */
  #define SYNA_F54_ANALOG_CTRL95_08_00_CFGBLK_OFS            0x011D   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_01_CFGBLK_OFS            0x011E   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_02_CFGBLK_OFS            0x011F   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_03_CFGBLK_OFS            0x0120   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_04_CFGBLK_OFS            0x0121   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_05_CFGBLK_OFS            0x0122   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_06_CFGBLK_OFS            0x0123   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_07_CFGBLK_OFS            0x0124   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_08_CFGBLK_OFS            0x0125   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_09_CFGBLK_OFS            0x0126   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_08_10_CFGBLK_OFS            0x0127   /* Frequency Control 8 */
  #define SYNA_F54_ANALOG_CTRL95_09_00_CFGBLK_OFS            0x0128   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_01_CFGBLK_OFS            0x0129   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_02_CFGBLK_OFS            0x012A   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_03_CFGBLK_OFS            0x012B   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_04_CFGBLK_OFS            0x012C   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_05_CFGBLK_OFS            0x012D   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_06_CFGBLK_OFS            0x012E   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_07_CFGBLK_OFS            0x012F   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_08_CFGBLK_OFS            0x0130   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_09_CFGBLK_OFS            0x0131   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_09_10_CFGBLK_OFS            0x0132   /* Frequency Control 9 */
  #define SYNA_F54_ANALOG_CTRL95_10_00_CFGBLK_OFS            0x0133   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_01_CFGBLK_OFS            0x0134   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_02_CFGBLK_OFS            0x0135   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_03_CFGBLK_OFS            0x0136   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_04_CFGBLK_OFS            0x0137   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_05_CFGBLK_OFS            0x0138   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_06_CFGBLK_OFS            0x0139   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_07_CFGBLK_OFS            0x013A   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_08_CFGBLK_OFS            0x013B   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_09_CFGBLK_OFS            0x013C   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_10_10_CFGBLK_OFS            0x013D   /* Frequency Control 10 */
  #define SYNA_F54_ANALOG_CTRL95_11_00_CFGBLK_OFS            0x013E   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_01_CFGBLK_OFS            0x013F   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_02_CFGBLK_OFS            0x0140   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_03_CFGBLK_OFS            0x0141   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_04_CFGBLK_OFS            0x0142   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_05_CFGBLK_OFS            0x0143   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_06_CFGBLK_OFS            0x0144   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_07_CFGBLK_OFS            0x0145   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_08_CFGBLK_OFS            0x0146   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_09_CFGBLK_OFS            0x0147   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL95_11_10_CFGBLK_OFS            0x0148   /* Frequency Control 11 */
  #define SYNA_F54_ANALOG_CTRL96_00_00_CFGBLK_OFS            0x0149   /* CBC Transcap 0 */
  #define SYNA_F54_ANALOG_CTRL96_00_01_CFGBLK_OFS            0x014A   /* CBC Transcap 1 */
  #define SYNA_F54_ANALOG_CTRL96_00_02_CFGBLK_OFS            0x014B   /* CBC Transcap 2 */
  #define SYNA_F54_ANALOG_CTRL96_00_03_CFGBLK_OFS            0x014C   /* CBC Transcap 3 */
  #define SYNA_F54_ANALOG_CTRL96_00_04_CFGBLK_OFS            0x014D   /* CBC Transcap 4 */
  #define SYNA_F54_ANALOG_CTRL96_00_05_CFGBLK_OFS            0x014E   /* CBC Transcap 5 */
  #define SYNA_F54_ANALOG_CTRL96_00_06_CFGBLK_OFS            0x014F   /* CBC Transcap 6 */
  #define SYNA_F54_ANALOG_CTRL96_00_07_CFGBLK_OFS            0x0150   /* CBC Transcap 7 */
  #define SYNA_F54_ANALOG_CTRL96_00_08_CFGBLK_OFS            0x0151   /* CBC Transcap 8 */
  #define SYNA_F54_ANALOG_CTRL96_00_09_CFGBLK_OFS            0x0152   /* CBC Transcap 9 */
  #define SYNA_F54_ANALOG_CTRL96_00_10_CFGBLK_OFS            0x0153   /* CBC Transcap 10 */
  #define SYNA_F54_ANALOG_CTRL96_00_11_CFGBLK_OFS            0x0154   /* CBC Transcap 11 */
  #define SYNA_F54_ANALOG_CTRL96_00_12_CFGBLK_OFS            0x0155   /* CBC Transcap 12 */
  #define SYNA_F54_ANALOG_CTRL96_00_13_CFGBLK_OFS            0x0156   /* CBC Transcap 13 */
  #define SYNA_F54_ANALOG_CTRL96_00_14_CFGBLK_OFS            0x0157   /* CBC Transcap 14 */
  #define SYNA_F54_ANALOG_CTRL96_00_15_CFGBLK_OFS            0x0158   /* CBC Transcap 15 */
  #define SYNA_F54_ANALOG_CTRL96_00_16_CFGBLK_OFS            0x0159   /* CBC Transcap 16 */
  #define SYNA_F54_ANALOG_CTRL96_00_17_CFGBLK_OFS            0x015A   /* CBC Transcap 17 */
  #define SYNA_F54_ANALOG_CTRL96_00_18_CFGBLK_OFS            0x015B   /* CBC Transcap 18 */
  #define SYNA_F54_ANALOG_CTRL96_00_19_CFGBLK_OFS            0x015C   /* CBC Transcap 19 */
  #define SYNA_F54_ANALOG_CTRL96_00_20_CFGBLK_OFS            0x015D   /* CBC Transcap 20 */
  #define SYNA_F54_ANALOG_CTRL96_00_21_CFGBLK_OFS            0x015E   /* CBC Transcap 21 */
  #define SYNA_F54_ANALOG_CTRL96_00_22_CFGBLK_OFS            0x015F   /* CBC Transcap 22 */
  #define SYNA_F54_ANALOG_CTRL96_00_23_CFGBLK_OFS            0x0160   /* CBC Transcap 23 */
  #define SYNA_F54_ANALOG_CTRL96_00_24_CFGBLK_OFS            0x0161   /* CBC Transcap 24 */
  #define SYNA_F54_ANALOG_CTRL96_00_25_CFGBLK_OFS            0x0162   /* CBC Transcap 25 */
  #define SYNA_F54_ANALOG_CTRL96_00_26_CFGBLK_OFS            0x0163   /* CBC Transcap 26 */
  #define SYNA_F54_ANALOG_CTRL96_00_27_CFGBLK_OFS            0x0164   /* CBC Transcap 27 */
  #define SYNA_F54_ANALOG_CTRL96_00_28_CFGBLK_OFS            0x0165   /* CBC Transcap 28 */
  #define SYNA_F54_ANALOG_CTRL96_00_29_CFGBLK_OFS            0x0166   /* CBC Transcap 29 */
  #define SYNA_F54_ANALOG_CTRL96_00_30_CFGBLK_OFS            0x0167   /* CBC Transcap 30 */
  #define SYNA_F54_ANALOG_CTRL96_00_31_CFGBLK_OFS            0x0168   /* CBC Transcap 31 */
  #define SYNA_F54_ANALOG_CTRL96_00_32_CFGBLK_OFS            0x0169   /* CBC Transcap 32 */
  #define SYNA_F54_ANALOG_CTRL96_00_33_CFGBLK_OFS            0x016A   /* CBC Transcap 33 */
  #define SYNA_F54_ANALOG_CTRL96_00_34_CFGBLK_OFS            0x016B   /* CBC Transcap 34 */
  #define SYNA_F54_ANALOG_CTRL96_00_35_CFGBLK_OFS            0x016C   /* CBC Transcap 35 */
  #define SYNA_F54_ANALOG_CTRL96_00_36_CFGBLK_OFS            0x016D   /* CBC Transcap 36 */
  #define SYNA_F54_ANALOG_CTRL96_00_37_CFGBLK_OFS            0x016E   /* CBC Transcap 37 */
  #define SYNA_F54_ANALOG_CTRL96_00_38_CFGBLK_OFS            0x016F   /* CBC Transcap 38 */
  #define SYNA_F54_ANALOG_CTRL96_00_39_CFGBLK_OFS            0x0170   /* CBC Transcap 39 */
  #define SYNA_F54_ANALOG_CTRL96_00_40_CFGBLK_OFS            0x0171   /* CBC Transcap 40 */
  #define SYNA_F54_ANALOG_CTRL96_00_41_CFGBLK_OFS            0x0172   /* CBC Transcap 41 */
  #define SYNA_F54_ANALOG_CTRL96_00_42_CFGBLK_OFS            0x0173   /* CBC Transcap 42 */
  #define SYNA_F54_ANALOG_CTRL96_00_43_CFGBLK_OFS            0x0174   /* CBC Transcap 43 */
  #define SYNA_F54_ANALOG_CTRL96_00_44_CFGBLK_OFS            0x0175   /* CBC Transcap 44 */
  #define SYNA_F54_ANALOG_CTRL96_00_45_CFGBLK_OFS            0x0176   /* CBC Transcap 45 */
  #define SYNA_F54_ANALOG_CTRL96_00_46_CFGBLK_OFS            0x0177   /* CBC Transcap 46 */
  #define SYNA_F54_ANALOG_CTRL96_00_47_CFGBLK_OFS            0x0178   /* CBC Transcap 47 */
  #define SYNA_F54_ANALOG_CTRL96_00_48_CFGBLK_OFS            0x0179   /* CBC Transcap 48 */
  #define SYNA_F54_ANALOG_CTRL96_00_49_CFGBLK_OFS            0x017A   /* CBC Transcap 49 */
  #define SYNA_F54_ANALOG_CTRL96_00_50_CFGBLK_OFS            0x017B   /* CBC Transcap 50 */
  #define SYNA_F54_ANALOG_CTRL96_00_51_CFGBLK_OFS            0x017C   /* CBC Transcap 51 */
  #define SYNA_F54_ANALOG_CTRL96_00_52_CFGBLK_OFS            0x017D   /* CBC Transcap 52 */
  #define SYNA_F54_ANALOG_CTRL96_00_53_CFGBLK_OFS            0x017E   /* CBC Transcap 53 */
  #define SYNA_F54_ANALOG_CTRL96_00_54_CFGBLK_OFS            0x017F   /* CBC Transcap 54 */
  #define SYNA_F54_ANALOG_CTRL96_00_55_CFGBLK_OFS            0x0180   /* CBC Transcap 55 */
  #define SYNA_F54_ANALOG_CTRL96_00_56_CFGBLK_OFS            0x0181   /* CBC Transcap 56 */
  #define SYNA_F54_ANALOG_CTRL96_00_57_CFGBLK_OFS            0x0182   /* CBC Transcap 57 */
  #define SYNA_F54_ANALOG_CTRL96_00_58_CFGBLK_OFS            0x0183   /* CBC Transcap 58 */
  #define SYNA_F54_ANALOG_CTRL96_00_59_CFGBLK_OFS            0x0184   /* CBC Transcap 59 */
  #define SYNA_F54_ANALOG_CTRL96_00_60_CFGBLK_OFS            0x0185   /* CBC Transcap 60 */
  #define SYNA_F54_ANALOG_CTRL96_00_61_CFGBLK_OFS            0x0186   /* CBC Transcap 61 */
  #define SYNA_F54_ANALOG_CTRL96_00_62_CFGBLK_OFS            0x0187   /* CBC Transcap 62 */
  #define SYNA_F54_ANALOG_CTRL96_00_63_CFGBLK_OFS            0x0188   /* CBC Transcap 63 */
  #define SYNA_F54_ANALOG_CTRL99_00_00_CFGBLK_OFS            0x0189   /* Waveform Duration */
  #define SYNA_F54_ANALOG_CTRL99_00_01_CFGBLK_OFS            0x018A   /* Waveform Duration */
  #define SYNA_F54_ANALOG_CTRL99_01_00_CFGBLK_OFS            0x018B   /* Reset Duration */
  #define SYNA_F54_ANALOG_CTRL100_CFGBLK_OFS                 0x018C   /* Bias current */
  #define SYNA_F54_ANALOG_CTRL101_00_00_CFGBLK_OFS           0x018D   /* Thermal Update Interval LSB */
  #define SYNA_F54_ANALOG_CTRL101_00_01_CFGBLK_OFS           0x018E   /* Thermal Update Interval MSB */
  #define SYNA_F54_ANALOG_CTRL101_00_02_CFGBLK_OFS           0x018F   /* Fast Half Life */
  #define SYNA_F54_ANALOG_CTRL101_00_03_CFGBLK_OFS           0x0190   /* Hold Fast Transition */
  #define SYNA_F54_ANALOG_CTRL101_00_04_CFGBLK_OFS           0x0191   /* Energy Ratio Threshold */
  #define SYNA_F54_ANALOG_CTRL103_00_00_CFGBLK_OFS           0x0192   /* TDDI CBC Enable Controls */
  #define SYNA_F54_ANALOG_CTRL103_00_01_CFGBLK_OFS           0x0193   /* TDDI CBC Parameters */
  #define SYNA_F54_ANALOG_CTRL103_00_02_CFGBLK_OFS           0x0194   /* TDDI Local Reference Channel CBC Parameters */
  #define SYNA_F54_ANALOG_CTRL103_01_00_CFGBLK_OFS           0x0195   /* TDDI Global CBC values for left/right channels */
  #define SYNA_F54_ANALOG_CTRL103_02_00_CFGBLK_OFS           0x0196   /* TDDI Reference Global CBC cap adjustment */
  #define SYNA_F54_ANALOG_CTRL121_00_00_CFGBLK_OFS           0x0197   /* HNM Rate Shift Frame Count */
  #define SYNA_F54_ANALOG_CTRL121_00_01_CFGBLK_OFS           0x0198   /* HNM Rate Shift Frame Count */
  #define SYNA_F54_ANALOG_CTRL121_00_02_CFGBLK_OFS           0x0199   /* Time Between Frequency Scans */
  #define SYNA_F54_ANALOG_CTRL182_00_00_CFGBLK_OFS           0x019A   /* CBC Timing Control Tx On Count LSB */
  #define SYNA_F54_ANALOG_CTRL182_00_01_CFGBLK_OFS           0x019B   /* CBC Timing Control Tx On Count MSB */
  #define SYNA_F54_ANALOG_CTRL182_00_02_CFGBLK_OFS           0x019C   /* CBC Timing Control Rx On Count LSB */
  #define SYNA_F54_ANALOG_CTRL182_00_03_CFGBLK_OFS           0x019D   /* CBC Timing Control Rx On Count MSB */
  #define SYNA_F54_ANALOG_CTRL183_CFGBLK_OFS                 0x019E   /* DAC_IN sets VHi VLo Voltages */
  #define SYNA_F54_ANALOG_CTRL196_00_00_CFGBLK_OFS           0x019F   /* Boost Control 0 */
  #define SYNA_F54_ANALOG_CTRL196_00_01_CFGBLK_OFS           0x01A0   /* Boost Control 1 */
  #define SYNA_F54_ANALOG_CTRL196_00_02_CFGBLK_OFS           0x01A1   /* Boost Control 2 */
  #define SYNA_F54_ANALOG_CTRL218_CFGBLK_OFS                 0x01A2   /* Misc Host Control */
  #define SYNA_F55_SENSOR_CTRL00_CFGBLK_OFS                  0x01A3   /* General Control */
  #define SYNA_F55_SENSOR_CTRL01_00_00_CFGBLK_OFS            0x01A4   /* Sensor Rx Mapping 0 */
  #define SYNA_F55_SENSOR_CTRL01_00_01_CFGBLK_OFS            0x01A5   /* Sensor Rx Mapping 1 */
  #define SYNA_F55_SENSOR_CTRL01_00_02_CFGBLK_OFS            0x01A6   /* Sensor Rx Mapping 2 */
  #define SYNA_F55_SENSOR_CTRL01_00_03_CFGBLK_OFS            0x01A7   /* Sensor Rx Mapping 3 */
  #define SYNA_F55_SENSOR_CTRL01_00_04_CFGBLK_OFS            0x01A8   /* Sensor Rx Mapping 4 */
  #define SYNA_F55_SENSOR_CTRL01_00_05_CFGBLK_OFS            0x01A9   /* Sensor Rx Mapping 5 */
  #define SYNA_F55_SENSOR_CTRL01_00_06_CFGBLK_OFS            0x01AA   /* Sensor Rx Mapping 6 */
  #define SYNA_F55_SENSOR_CTRL01_00_07_CFGBLK_OFS            0x01AB   /* Sensor Rx Mapping 7 */
  #define SYNA_F55_SENSOR_CTRL01_00_08_CFGBLK_OFS            0x01AC   /* Sensor Rx Mapping 8 */
  #define SYNA_F55_SENSOR_CTRL01_00_09_CFGBLK_OFS            0x01AD   /* Sensor Rx Mapping 9 */
  #define SYNA_F55_SENSOR_CTRL01_00_10_CFGBLK_OFS            0x01AE   /* Sensor Rx Mapping 10 */
  #define SYNA_F55_SENSOR_CTRL01_00_11_CFGBLK_OFS            0x01AF   /* Sensor Rx Mapping 11 */
  #define SYNA_F55_SENSOR_CTRL01_00_12_CFGBLK_OFS            0x01B0   /* Sensor Rx Mapping 12 */
  #define SYNA_F55_SENSOR_CTRL01_00_13_CFGBLK_OFS            0x01B1   /* Sensor Rx Mapping 13 */
  #define SYNA_F55_SENSOR_CTRL01_00_14_CFGBLK_OFS            0x01B2   /* Sensor Rx Mapping 14 */
  #define SYNA_F55_SENSOR_CTRL01_00_15_CFGBLK_OFS            0x01B3   /* Sensor Rx Mapping 15 */
  #define SYNA_F55_SENSOR_CTRL01_00_16_CFGBLK_OFS            0x01B4   /* Sensor Rx Mapping 16 */
  #define SYNA_F55_SENSOR_CTRL01_00_17_CFGBLK_OFS            0x01B5   /* Sensor Rx Mapping 17 */
  #define SYNA_F55_SENSOR_CTRL01_00_18_CFGBLK_OFS            0x01B6   /* Sensor Rx Mapping 18 */
  #define SYNA_F55_SENSOR_CTRL01_00_19_CFGBLK_OFS            0x01B7   /* Sensor Rx Mapping 19 */
  #define SYNA_F55_SENSOR_CTRL01_00_20_CFGBLK_OFS            0x01B8   /* Sensor Rx Mapping 20 */
  #define SYNA_F55_SENSOR_CTRL01_00_21_CFGBLK_OFS            0x01B9   /* Sensor Rx Mapping 21 */
  #define SYNA_F55_SENSOR_CTRL01_00_22_CFGBLK_OFS            0x01BA   /* Sensor Rx Mapping 22 */
  #define SYNA_F55_SENSOR_CTRL01_00_23_CFGBLK_OFS            0x01BB   /* Sensor Rx Mapping 23 */
  #define SYNA_F55_SENSOR_CTRL01_00_24_CFGBLK_OFS            0x01BC   /* Sensor Rx Mapping 24 */
  #define SYNA_F55_SENSOR_CTRL01_00_25_CFGBLK_OFS            0x01BD   /* Sensor Rx Mapping 25 */
  #define SYNA_F55_SENSOR_CTRL01_00_26_CFGBLK_OFS            0x01BE   /* Sensor Rx Mapping 26 */
  #define SYNA_F55_SENSOR_CTRL01_00_27_CFGBLK_OFS            0x01BF   /* Sensor Rx Mapping 27 */
  #define SYNA_F55_SENSOR_CTRL01_00_28_CFGBLK_OFS            0x01C0   /* Sensor Rx Mapping 28 */
  #define SYNA_F55_SENSOR_CTRL01_00_29_CFGBLK_OFS            0x01C1   /* Sensor Rx Mapping 29 */
  #define SYNA_F55_SENSOR_CTRL01_00_30_CFGBLK_OFS            0x01C2   /* Sensor Rx Mapping 30 */
  #define SYNA_F55_SENSOR_CTRL01_00_31_CFGBLK_OFS            0x01C3   /* Sensor Rx Mapping 31 */
  #define SYNA_F55_SENSOR_CTRL02_00_00_CFGBLK_OFS            0x01C4   /* Sensor Tx Mapping 0 */
  #define SYNA_F55_SENSOR_CTRL02_00_01_CFGBLK_OFS            0x01C5   /* Sensor Tx Mapping 1 */
  #define SYNA_F55_SENSOR_CTRL02_00_02_CFGBLK_OFS            0x01C6   /* Sensor Tx Mapping 2 */
  #define SYNA_F55_SENSOR_CTRL02_00_03_CFGBLK_OFS            0x01C7   /* Sensor Tx Mapping 3 */
  #define SYNA_F55_SENSOR_CTRL02_00_04_CFGBLK_OFS            0x01C8   /* Sensor Tx Mapping 4 */
  #define SYNA_F55_SENSOR_CTRL02_00_05_CFGBLK_OFS            0x01C9   /* Sensor Tx Mapping 5 */
  #define SYNA_F55_SENSOR_CTRL02_00_06_CFGBLK_OFS            0x01CA   /* Sensor Tx Mapping 6 */
  #define SYNA_F55_SENSOR_CTRL02_00_07_CFGBLK_OFS            0x01CB   /* Sensor Tx Mapping 7 */
  #define SYNA_F55_SENSOR_CTRL02_00_08_CFGBLK_OFS            0x01CC   /* Sensor Tx Mapping 8 */
  #define SYNA_F55_SENSOR_CTRL02_00_09_CFGBLK_OFS            0x01CD   /* Sensor Tx Mapping 9 */
  #define SYNA_F55_SENSOR_CTRL03_00_00_CFGBLK_OFS            0x01CE   /* Edge Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL03_00_01_CFGBLK_OFS            0x01CF   /* Edge Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL03_00_02_CFGBLK_OFS            0x01D0   /* Edge Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL03_00_03_CFGBLK_OFS            0x01D1   /* Edge Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL03_00_04_CFGBLK_OFS            0x01D2   /* Edge Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL03_00_05_CFGBLK_OFS            0x01D3   /* Edge Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL03_00_06_CFGBLK_OFS            0x01D4   /* Edge Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL03_00_07_CFGBLK_OFS            0x01D5   /* Edge Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL10_00_00_CFGBLK_OFS            0x01D6   /* Corner Compensation 0 */
  #define SYNA_F55_SENSOR_CTRL10_00_01_CFGBLK_OFS            0x01D7   /* Corner Compensation 1 */
  #define SYNA_F55_SENSOR_CTRL10_00_02_CFGBLK_OFS            0x01D8   /* Corner Compensation 2 */
  #define SYNA_F55_SENSOR_CTRL10_00_03_CFGBLK_OFS            0x01D9   /* Corner Compensation 3 */
  #define SYNA_F55_SENSOR_CTRL10_00_04_CFGBLK_OFS            0x01DA   /* Corner Compensation 4 */
  #define SYNA_F55_SENSOR_CTRL10_00_05_CFGBLK_OFS            0x01DB   /* Corner Compensation 5 */
  #define SYNA_F55_SENSOR_CTRL10_00_06_CFGBLK_OFS            0x01DC   /* Corner Compensation 6 */
  #define SYNA_F55_SENSOR_CTRL10_00_07_CFGBLK_OFS            0x01DD   /* Corner Compensation 7 */
  #define SYNA_F55_SENSOR_CTRL43_00_00_CFGBLK_OFS            0x01DE   /* AMP Sensor Characteristics 2 */
  #define SYNA_F55_SENSOR_CTRL43_01_00_CFGBLK_OFS            0x01DF   /* AMP Sensor Characteristics 2 */
  #define SYNA_F55_SENSOR_CTRL44_00_00_CFGBLK_OFS            0x01E0   /* AMP Sensor MUX Swirling Order 0 */
  #define SYNA_F55_SENSOR_CTRL44_00_01_CFGBLK_OFS            0x01E1   /* AMP Sensor MUX Swirling Order 1 */
  #define SYNA_F55_SENSOR_CTRL44_00_02_CFGBLK_OFS            0x01E2   /* AMP Sensor MUX Swirling Order 2 */
  #define SYNA_F55_SENSOR_CTRL44_00_03_CFGBLK_OFS            0x01E3   /* AMP Sensor MUX Swirling Order 3 */
  #define SYNA_F55_SENSOR_CTRL44_00_04_CFGBLK_OFS            0x01E4   /* AMP Sensor MUX Swirling Order 4 */
  #define SYNA_F55_SENSOR_CTRL44_00_05_CFGBLK_OFS            0x01E5   /* AMP Sensor MUX Swirling Order 5 */
  #define SYNA_F55_SENSOR_CTRL44_00_06_CFGBLK_OFS            0x01E6   /* AMP Sensor MUX Swirling Order 6 */
  #define SYNA_F55_SENSOR_CTRL44_00_07_CFGBLK_OFS            0x01E7   /* AMP Sensor MUX Swirling Order 7 */
  #define SYNA_F55_SENSOR_CTRL44_00_08_CFGBLK_OFS            0x01E8   /* AMP Sensor MUX Swirling Order 8 */
  #define SYNA_F55_SENSOR_CTRL44_00_09_CFGBLK_OFS            0x01E9   /* AMP Sensor MUX Swirling Order 9 */
  #define SYNA_F51_CUSTOM_CTRL22_CFGBLK_OFS                  0x01EA   /* Palm Filter Mode3 Distance Threshold LSB */
  #define SYNA_F51_CUSTOM_CTRL23_CFGBLK_OFS                  0x01EB   /* Palm Filter Mode3 Distance Threshold MSB */
  #define SYNA_FDC_PRINT_CTRL00_CFGBLK_OFS                   0x01EC   /* Print Control */
  #define SYNA_CFGBLK_CRC1_CFGBLK_OFS                        0x0FFC   /* Configuration CRC [7:0] */
  #define SYNA_CFGBLK_CRC2_CFGBLK_OFS                        0x0FFD   /* Configuration CRC [15:8] */
  #define SYNA_CFGBLK_CRC3_CFGBLK_OFS                        0x0FFE   /* Configuration CRC [23:16] */
  #define SYNA_CFGBLK_CRC4_CFGBLK_OFS                        0x0FFF   /* Configuration CRC [31:24] */

  /* Masks for interrupt sources */

  /*      Symbol Name                                        Mask        Description */
  /*      -----------                                        ----        ----------- */
  #define SYNA_F01_RMI_INT_SOURCE_MASK_ALL                   0x0002   /* Mask of all Func $01 (RMI) interrupts */
  #define SYNA_F01_RMI_INT_SOURCE_MASK_STATUS                0x0002   /* Mask of Func $01 (RMI) 'STATUS' interrupt */
  #define SYNA_F12_2D_INT_SOURCE_MASK_ABS0                   0x0004   /* Mask of Func $12 (2D) 'ABS0' interrupt */
  #define SYNA_F12_2D_INT_SOURCE_MASK_ALL                    0x0004   /* Mask of all Func $12 (2D) interrupts */
  #define SYNA_F34_FLASH_INT_SOURCE_MASK_ALL                 0x0001   /* Mask of all Func $34 (FLASH) interrupts */
  #define SYNA_F34_FLASH_INT_SOURCE_MASK_FLASH               0x0001   /* Mask of Func $34 (FLASH) 'FLASH' interrupt */
  #define SYNA_F54_ANALOG_INT_SOURCE_MASK_ALL                0x0008   /* Mask of all Func $54 (ANALOG) interrupts */
  #define SYNA_F54_ANALOG_INT_SOURCE_MASK_ANALOG             0x0008   /* Mask of Func $54 (ANALOG) 'ANALOG' interrupt */
  #define SYNA_F55_SENSOR_INT_SOURCE_MASK_ALL                0x0010   /* Mask of all Func $55 (SENSOR) interrupts */
  #define SYNA_F55_SENSOR_INT_SOURCE_MASK_SENSOR             0x0010   /* Mask of Func $55 (SENSOR) 'SENSOR' interrupt */
  #define SYNA_FDC_PRINT_INT_SOURCE_MASK_ALL                 0x0020   /* Mask of all Func $DC (PRINT) interrupts */
  #define SYNA_FDC_PRINT_INT_SOURCE_MASK_PRINT               0x0020   /* Mask of Func $DC (PRINT) 'PRINT' interrupt */

#endif  /* SYNA_REGISTER_MAP_H */

