#ifndef __S6E3HA0_WQHD_PARAM_H__
#define __S6E3HA0_WQHD_PARAM_H__

#include <linux/types.h>
#include <linux/kernel.h>

struct lcd_seq_info {
	unsigned char	*cmd;
	unsigned int	len;
	unsigned int	sleep;
};

#define S6E3HA2_WQXGA_ID1 			0x400000

#define LCD_TYPE_S6E3HA2_WQXGA		0
#define LCD_TYPE_S6E3HA0_WQXGA		1

#define LCDTYPE_MASK				0x00c0ffc0

#define S6E3HA0_ID_REG				0x04
#define S6E3HA0_ID_LEN				3

static const unsigned char S6E3HA0_SEQ_TEST_KEY_ON_F0[] = {
	0xF0,
	0x5A, 0x5A
};

static const unsigned char S6E3HA0_SEQ_TEST_KEY_OFF_F0[] = {
	0xF0,
	0xA5, 0xA5
};

static const unsigned char S6E3HA0_SEQ_TEST_KEY_ON_F1[] = {
	0xF1,
	0x5A, 0x5A,
};

static const unsigned char S6E3HA0_SEQ_TEST_KEY_OFF_F1[] = {
	0xF1,
	0xA5, 0xA5,
};

static const unsigned char S6E3HA0_SEQ_TEST_KEY_ON_FC[] = {
	0xFC,
	0x5A, 0x5A
};

static const unsigned char S6E3HA0_SEQ_TEST_KEY_OFF_FC[] = {
	0xFC,
	0xA5, 0xA5
};

static const unsigned char S6E3HA0_SEQ_SLEEP_OUT[] = {
	0x11,
};

static const unsigned char S6E3HA0_SEQ_SLEEP_IN[] = {
	0x10
};

static const unsigned char S6E3HA0_SEQ_DISPLAY_ON[] = {
	0x29,
};

static const unsigned char S6E3HA0_SEQ_DISPLAY_OFF[] = {
	0x28
};

static const unsigned char S6E3HA0_SEQ_SINGLE_DSI_1[] = {
	0xF2,
	0x67
};

static const unsigned char S6E3HA0_SEQ_SINGLE_DSI_2[] = {
	0xF9,
	0x29
};

static const unsigned char S6E3HA0_SEQ_GAMMA_CONDITION_SET[] = {
	0xCA,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80,	0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x00, 0x00, 0x00
};

static const unsigned char S6E3HA0_SEQ_AOR_CONTROL[] = {
	0xB2,
	0x01, 0x03
};

static const unsigned char S6E3HA0_SEQ_ELVSS_SET[] = {
	0xB6,
	0x98,	/* MPS_CON: ACL OFF */
	0x04	/* ELVSS: MAX*/
};

static const unsigned char S6E3HA0_SEQ_GAMMA_UPDATE[] = {
	0xF7,
	0x03
};

static const unsigned char S6E3HA0_SEQ_GAMMA_UPDATE_L[] = {
	0xF7,
	0x00
};

static const unsigned char S6E3HA0_SEQ_ACL_OFF[] = {
	0x55,
	0x00
};

static const unsigned char S6E3HA0_SEQ_ACL_OFF_OPR[] = {
	0xB5,
	0x41
};

static const unsigned char S6E3HA0_SEQ_TE_ON[] = {
	0x35,
	0x00
};

static const unsigned char S6E3HA0_SEQ_TSET_GLOBAL[] = {
	0xB0,
	0x05
};

static const unsigned char S6E3HA0_SEQ_TSET[] = {
	0xB8,
	0x19
};

static const unsigned char S6E3HA0_SEQ_PENTILE_SETTING[] = {
	0xC0,
	0x30, 0x00, 0xD8, 0xD8
};

static const unsigned char S6E3HA0_SEQ_TOUCH_HSYNC[] = {
	0xBD,
	0x85, 0x02, 0x16
};

static const unsigned char S6E3HA0_SEQ_TOUCH_VSYNC[] = {
	0xFF,
	0x02
};




static const unsigned char S6E3HA2_SEQ_TEST_KEY_ON_F0[] = {
	0xF0,
	0x5A, 0x5A
};

static const unsigned char S6E3HA2_SEQ_TEST_KEY_OFF_F0[] = {
	0xF0,
	0xA5, 0xA5
};

static const unsigned char S6E3HA2_SEQ_TEST_KEY_ON_F1[] = {
	0xF1,
	0x5A, 0x5A,
};

static const unsigned char S6E3HA2_SEQ_TEST_KEY_OFF_F1[] = {
	0xF1,
	0xA5, 0xA5,
};

static const unsigned char S6E3HA2_SEQ_TEST_KEY_ON_FC[] = {
	0xFC,
	0x5A, 0x5A
};

static const unsigned char S6E3HA2_SEQ_TEST_KEY_OFF_FC[] = {
	0xFC,
	0xA5, 0xA5
};

static const unsigned char S6E3HA2_SEQ_SLEEP_OUT[] = {
	0x11
};

static const unsigned char S6E3HA2_SEQ_SLEEP_IN[] = {
	0x10
};

static const unsigned char S6E3HA2_SEQ_DISPLAY_ON[] = {
	0x29
};

static const unsigned char S6E3HA2_SEQ_DISPLAY_OFF[] = {
	0x28
};

static const unsigned char S6E3HA2_SEQ_SINGLE_DSI_1[] = {
	0xF2,
	0x67
};

static const unsigned char S6E3HA2_SEQ_SINGLE_DSI_2[] = {
	0xF9,
	0x29
};

static const unsigned char S6E3HA2_SEQ_GAMMA_CONDITION_SET[] = {
	0xCA,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x00, 0x00, 0x00,
	0x00, 0x00
};

static const unsigned char S6E3HA2_SEQ_AOR_CONTROL[] = {
	0xB2,
	0x03, 0x10
};

static const unsigned char S6E3HA2_SEQ_ELVSS_SET[] = {
	0xB6,
	0x9C,	/* MPS_CON: ACL OFF */
	0x0A	/* ELVSS: MAX*/
};

static const unsigned char S6E3HA2_SEQ_VINT_SET[] = {
	0xF4,
	0x8B, 0x21
};

static const unsigned char S6E3HA2_SEQ_GAMMA_UPDATE[] = {
	0xF7,
	0x03
};

static const unsigned char S6E3HA2_SEQ_GAMMA_UPDATE_L[] = {
	0xF7,
	0x00,
};

static const unsigned char S6E3HA2_SEQ_TSET_GLOBAL[] = {
	0xB0,
	0x07
};

static const unsigned char S6E3HA2_SEQ_TE_ON[] = {
	0x35,
	0x00
};
static const unsigned char S6E3HA2_SEQ_TE_OFF[] = {
	0x34,
};

static const unsigned char S6E3HA2_SEQ_TSP_TE[] = {
	0xBD,
	0x11, 0x11, 0x02, 0x16, 0x02, 0x16
};

static const unsigned char S6E3HA2_SEQ_PENTILE_SETTING[] = {
	0xC0,
	0x00, 0x00, 0xD8, 0xD8
};

static const unsigned char S6E3HA2_SEQ_POC_SETTING1[] = {
	0xB0,
	0x20
};

static const unsigned char S6E3HA2_SEQ_POC_SETTING2[] = {
	0xFE,
	0x08
};

static const unsigned char S6E3HA2_SEQ_OSC_SETTING1[] = {
	0xFD,
	0x1C
};

static const unsigned char S6E3HA2_SEQ_OSC_SETTING2[] = {
	0xFE,
	0xA0
};

static const unsigned char S6E3HA2_SEQ_OSC_SETTING3[] = {
	0xFE,
	0x20
};


static const unsigned char S6E3HA2_SEQ_PCD_SETTING[] = {
	0xCC,
	0x4C
};

static const unsigned char S6E3HA2_SEQ_ERR_FG_SETTING[] = {
	0xED,
	0x44
};

static const unsigned char S6E3HA2_SEQ_HBM_OFF[] = {
	0x53,
	0x00
};

static const unsigned char S6E3HA2_SEQ_HBM_ON[] = {
	0x53,
	0xC0
};
static const unsigned char S6E3HA2_SEQ_ACL_OFF[] = {
	0x55,
	0x00
};
static const unsigned char S6E3HA2_SEQ_ACL_8[] = {
	0x55,
	0x02,
};

static const unsigned char S6E3HA2_SEQ_ACL_OFF_OPR[] = {
	0xB5,
	0x40
};

static const unsigned char S6E3HA2_SEQ_ACL_ON_OPR[] = {
	0xB5,
	0x50
};


static const unsigned char S6E3HA2_SEQ_TSET[] = {
	0xB8,
	0x19	/* Global para(8th) + 25 degrees  : 0x19 */
};

static const unsigned int VINT_DIM_TABLE[] = {
	5,	6,	7,	8,	9,
	10,	11,	12,	13,	14
};

static const unsigned char VINT_TABLE[] = {
	0x18, 0x19, 0x1A, 0x1B, 0x1C,
	0x1D, 0x1E, 0x1F, 0x20, 0x21
};

#define POWER_IS_ON(pwr)			(pwr <= FB_BLANK_NORMAL)
#define LEVEL_IS_HBM(level)			(level >= 6)
#define UNDER_MINUS_20(temperature)	(temperature <= -20)
#define UNDER_0(temperature)	(temperature <= 0)

#define ACL_IS_ON(nit) 				(nit < 360)
#define CAPS_IS_ON(level)	(level >= 41)

#define NORMAL_TEMPERATURE			25	/* 25 degrees Celsius */
#define UI_MAX_BRIGHTNESS 			255
#define UI_MIN_BRIGHTNESS 			0
#define UI_DEFAULT_BRIGHTNESS 		134

#define S6E3HA2_MTP_ADDR		0xC8
#define S6E3HA2_MTP_SIZE		35

#define S6E3HA2_ID_REG			0x04
#define S6E3HA2_ID_LEN			3


#define S6E3HA2_RDDPM_ADDR		0x0A
#define S6E3HA2_RDDSM_ADDR		0x0E
#define S6E3HA2_MTP_DATE_SIZE	S6E3HA2_MTP_SIZE + 9

#define S6E3HA2_COORDINATE_LEN	4
#define S6E3HA2_COORDINATE_REG	0xA1

#define S6E3HA2_CODE_REG		0xD6
#define S6E3HA2_CODE_LEN		5


#define S6E3HA2_HBMGAMMA_REG		0xB4
#define S6E3HA2_HBMGAMMA_LEN		31

#define S6E3HA2_MAX_BRIGHTNESS	360
#define S6E3HA2_HBM_BRIGHTNESS	600

#define S6E3HA2_HBM_ELVSS_INDEX		21
#define S6E3HA2_HBM_ELVSS_COMP		0x06

#define AID_CMD_CNT					3
#define ELVSS_CMD_CNT				3
#define GAMMA_CMD_CNT 				36
#define ELVSS_REG					0xB6
#define ELVSS_LEN					23   		/* elvss: Global para 22th */

#define TSET_REG					0xB8 /* TSET: Global para 8th */
#define TSET_LEN					9
#define VINT_REG					0xF4
#define VINT_LEN					3
#define TSET_MINUS_OFFSET			0x04
#define ELVSS_REG					0xB6
#define ELVSS_LEN					23   /* elvss: Global para 22th */

#define HBM_INDEX					73

#define SEQ_TEST_KEY_ON_F0			S6E3HA2_SEQ_TEST_KEY_ON_F0
#define SEQ_TEST_KEY_OFF_F0			S6E3HA2_SEQ_TEST_KEY_OFF_F0
#define SEQ_TEST_KEY_ON_FC			S6E3HA2_SEQ_TEST_KEY_ON_FC
#define SEQ_TEST_KEY_OFF_FC			S6E3HA2_SEQ_TEST_KEY_OFF_FC
#define SEQ_HBM_OFF					S6E3HA2_SEQ_HBM_OFF
#define SEQ_GAMMA_UPDATE			S6E3HA2_SEQ_GAMMA_UPDATE
#define SEQ_GAMMA_UPDATE_L			S6E3HA2_SEQ_GAMMA_UPDATE_L
#define SEQ_TE_ON					S6E3HA2_SEQ_TE_ON
#define SEQ_TE_OFF					S6E3HA2_SEQ_TE_OFF
enum {
	HBM_STATUS_OFF,
	HBM_STATUS_ON,
	HBM_STATUS_MAX,
};

enum {
	ACL_STATUS_0P,
	ACL_STATUS_8P,
	ACL_STATUS_MAX
};

enum {
	ACL_OPR_16_FRAME,
	ACL_OPR_32_FRAME,
	ACL_OPR_MAX
};

#ifdef CONFIG_LCD_HMT
#define DEFAULT_HMT_BRIGHTNESS			162
#define	HMT_OFF							0
#define HMT_ON							1

static const unsigned char SEQ_HMT_ON1[] = {	/* aid */
	0xB2,
	0x03, 0x13, 0x04, 0x0E, 0x0E, 0x00
};
static const unsigned char SEQ_HMT_ON2[] = {		/* porch */
	0xF3,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
};
static const unsigned char SEQ_HMT_ON3[] = {		/* PASET Setting  */
	0x2B,
	0x00, 0x00, 0x09, 0xFF
};


static const unsigned char SEQ_HMT_OFF1[] = {	/* aid */
	0xB2,
	0xA5, 0x30, 0x04, 0x0C, 0x0C, 0x40
};
static const unsigned char SEQ_HMT_OFF2[] = {	/* porch */
	0xF3,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


static const unsigned char SEQ_HMT_AID_FORWARD1[] = {	/* G.Param */
	0xF2,
	0x67, 0x41, 0xC5, 0x0A, 0x06
};
static const unsigned char SEQ_HMT_AID_FORWARD2[] = {	/* G.Param */
	0xCB,
	0x18, 0x31, 0x80, 0x00, 0x00, 0x20, 0x40, 0x00,
	0x00, 0x06, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x15, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
	0xDF, 0x00, 0x00, 0xCA, 0x0A, 0x0A, 0x0A, 0xCA,
	0x8A, 0xCA, 0x0A, 0x0A, 0x0A, 0xCD, 0xCF, 0xCB,
	0xC2, 0x43, 0xC0, 0xCA, 0xCA, 0x0A, 0x0A, 0x0A,
	0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0D, 0x0F, 0x0B
};

static const unsigned char SEQ_HMT_AID_REVERSE1[] = {	/* G.Param */
	0xF2,
	0x67, 0x41, 0xC5, 0x0E, 0x06
};
static const unsigned char SEQ_HMT_AID_REVERSE2[] = {	/* G.Param */
	0xCB,
	0x18, 0x31, 0x80, 0x00, 0x00, 0x20, 0x40, 0x00,
	0x00, 0x02, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x15, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00,
	0xDF, 0x00, 0x00, 0xCA, 0x0A, 0x0A, 0x0A, 0xCA,
	0x8A, 0xCA, 0x0A, 0x0A, 0x0A, 0xCD, 0xCF, 0xCC,
	0xC2, 0x43, 0xC0, 0xCA, 0xCA, 0x0A, 0x0A, 0x0A,
	0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0D, 0x0F, 0x0C
};


#endif

enum {
	HBM_INTER_OFF = 0,
	HBM_COLORBLIND_ON,
	HBM_GALLERY_ON,
};

// 384 ~ 550
static const char HBM_INTER_22TH_OFFSET[] = {
	0x02, 0x04, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06
};


#endif /* __S6E3HA0_PARAM_H__ */
