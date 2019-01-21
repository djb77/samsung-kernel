#ifndef __GP2AP070S_H__
#define __GP2AP070S_H__

/* Register address */
#define REG_COM1                0x80
#define REG_COM2                0x81
#define REG_COM3                0x82
#define REG_COM4                0x83
#define REG_PS1                 0x85
#define REG_PS2                 0x86
#define REG_PS3                 0x87
#define REG_PS_LT_LSB           0x88
#define REG_PS_LT_MSB           0x89
#define REG_PS_HT_LSB           0x8A
#define REG_PS_HT_MSB           0x8B
#define REG_OS_D0_LSB           0x8C
#define REG_OS_D0_MSB           0x8D
#define REG_DARK                0x8E
#define REG_D0_LSB              0x90
#define REG_D0_MSB              0x91
#define REG_REV_CODE            0xA0
#define REG_REVF                0xA1
#define REG_TEST1               0xB0
#define REG_TEST2               0xB1
#define REG_TEST3               0xB2

#define REG_FREQ                0xE0

/* 0x80 COMMAND1 */
#define COM1_WAKEUP             0x80
#define COM1_SD                 0x00
#define COM1_PS                 0x20

/* 0x81 COMMAND2 */
#define COM2_NO_INT_CLEAR       0x1F
#define COM2_INT_ALL_CLEAR      0x00
#define COM2_PS_INT_CLEAR       0x1B

/* 0x82 COMMAND3 */
#define COM3_INT_PROX           0x00
#define COM3_INT_PS             0x10
#define COM3_IOUTPUT_INT        0x04
#define COM3_IOUTPUT_GPIO       0x0C
#define COM3_INT_LEVEL          0x00
#define COM3_INT_PULSE          0x02
#define COM3_REG_RST            0x01

/* 0x83 COMMAND4 */
#define COM4_INTVAL0            0x00
#define COM4_INTVAL2            0x01
#define COM4_INTVAL8            0x02
#define COM4_INTVAL33           0x03
#define COM4_INTVAL66           0x04
#define COM4_INTVAL131          0x05
#define COM4_INTVAL262          0x06
#define COM4_INTVAL524          0x07

/* 0x85 PS1 */
#define PS1_RES14               0x00
#define PS1_RES12               0x10
#define PS1_RES10               0x20
#define PS1_RES8                0x30

/* 0x86 PS2 */
#define PS2_IS0                 0x00
#define PS2_IS24                0x10
#define PS2_IS89                0x20
#define PS2_IS130               0x30
#define PS2_IS190               0x70
#define PS2_SUM16               0x00
#define PS2_SUM32               0x04

/* 0x87 PS3 */
#define PS3_PRST0               0x00
#define PS3_PRST1               0x10
#define PS3_PRST2               0x20
#define PS3_PRST3               0x30
#define PS3_PRST4               0x40
#define PS3_PRST5               0x50
#define PS3_PRST6               0x60
#define PS3_PRST7               0x70
#define PS3_TGINTEN_PS0         0x00
#define PS3_TGINTEN_PS1         0x08
#define PS3_TGIRDRON0           0x00
#define PS3_TGIRDRON1           0x04
#endif
