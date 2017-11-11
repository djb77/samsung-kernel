#ifndef __PANEL_INFO_H__
#define __PANEL_INFO_H__

#if defined(CONFIG_PANEL_S6E3HA2_DYNAMIC)
#include "s6e3ha2_s6e3ha0_wqhd_param.h"

#elif defined(CONFIG_PANEL_S6E3HF2_DYNAMIC)
#include "s6e3hf2_wqhd_param.h"

#elif defined(CONFIG_PANEL_S6E3FA3_FHD)
#include "s6e3fa3_fhd_param.h"

#else
#error "ERROR !! Check LCD Panel Header File"
#endif

#ifdef CONFIG_PANEL_AID_DIMMING
#include "smart_dimming_core.h"
#endif

#define PANEL_STATE_SUSPENED	0
#define PANEL_STATE_RESUMED		1
#define PANEL_STATE_SUSPENDING	2

#define PANEL_DISCONNEDTED		0
#define PANEL_CONNECTED			1



#define CUR_MAIN_LCD_ON	 0x00
#define CUR_MAIN_LCD_OFF 0x02
#define CUR_SUB_LCD_ON	0x00
#define CUR_SUB_LCD_OFF	0x01
#define CUR_MAX			4

#define REQ_ON_MAIN_LCD		0x00
#define REQ_ON_SUB_LCD		0x01
#define REQ_OFF_MAIN_LCD	0x02
#define REQ_OFF_SUB_LCD		0x03
#define REQ_MAX				4


#define SKIP_UPDATE		0x00000000
#define NEED_TO_UPDATE	0x80000000
#define FULL_UPDATE		0x40000000


#define PANEL_STAT_DISPLAY_ON		0x00001000

#define PANEL_STAT_MASTER			0x00000000
#define PANEL_STAT_SLAVE			0x00000001
#define PANEL_STAT_STANDALONE		0x00000002
#define PANEL_STAT_OFF				0x00000003
#define GET_PANEL_STAT(x) 		(x & 0x000000ff)
#define MASK_PANEL_STAT			~(0x000000ff)


#define CHECK_NEED_DISPLAY_ON(x) ((GET_PANEL_STAT(x) != PANEL_STAT_OFF) && (!(x & PANEL_STAT_DISPLAY_ON)))
#define CHECK_ALREADY_DISPLAY_ON(x) ((GET_PANEL_STAT(x) != PANEL_STAT_OFF) && (x & PANEL_STAT_DISPLAY_ON))

#define REF_TE_NONE	0x00
#define REF_TE_MAIN	0x01
#define REF_TE_SUB	0x02
struct panel_info {
	struct aid_dimming_info *aid_info;
	const char *CMD_L1_TEST_KEY_ON;
	const char *CMD_L1_TEST_KEY_OFF;
	const char *CMD_L2_TEST_KEY_ON;
	const char *CMD_L2_TEST_KEY_OFF;
	const char *CMD_L3_TEST_KEY_ON;
	const char *CMD_L3_TEST_KEY_OFF;
	const char *CMD_GAMMA_UPDATE_1;
	const char *CMD_GAMMA_UPDATE_2;
	const char *CMD_ACL_ON_OPR;
	const char *CMD_ACL_ON;
	const char *CMD_ACL_OFF_OPR;
	const char *CMD_ACL_OFF;
	struct gamma_cmd *gamma_cmd;
	int mtp[VREF_POINT_CNT][CI_MAX];
	int vt_mtp[CI_MAX];
};


struct br_mapping_tbl {
	const unsigned int *idx_br_tbl;
	const unsigned int *ref_br_tbl;
	const unsigned int *phy_br_tbl;
};

struct panel_private {

	struct backlight_device *bd;
	unsigned char id[3];
	unsigned char code[5];
	unsigned char elvss_set[22];
	unsigned char tset[8];
	int	temperature;
	unsigned int coordinate[2];
	unsigned char date[4];
	unsigned int lcdConnected;
	unsigned int state;
	unsigned int auto_brightness;
	unsigned int br_index;
	unsigned int acl_enable;
	unsigned int caps_enable;
	unsigned int current_acl;
	unsigned int current_hbm;
	unsigned int current_vint;
	unsigned int siop_enable;

	void *dim_data;
	void *dim_info;
	unsigned int *br_tbl;
	unsigned int *hbm_inter_br_tbl;
	unsigned char **hbm_tbl;
	unsigned char **acl_cutoff_tbl;
	unsigned char **acl_opr_tbl;
	struct mutex lock;
	struct dsim_panel_ops *ops;
	unsigned int panel_type;
#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
	unsigned int mdnie_support;
#endif

#ifdef CONFIG_EXYNOS_DECON_LCD_MCD
	unsigned int mcd_on;
#endif

#ifdef CONFIG_LCD_HMT
	unsigned int hmt_on;
	unsigned int hmt_br_index;
	unsigned int hmt_brightness;
	void *hmt_dim_data;
	void *hmt_dim_info;
	unsigned int *hmt_br_tbl;
#endif

#ifdef CONFIG_LCD_ALPM
	unsigned int 	alpm;
	unsigned int 	current_alpm;
	struct mutex	alpm_lock;
#endif
	unsigned int interpolation;
	char hbm_elvss_comp;
	unsigned char hbm_elvss;
	unsigned int hbm_index;
/* hbm interpolation for color weakness */
	unsigned int weakness_hbm_comp;

/*new feature */
	struct panel_info command;
	/*dimming t*/
	struct br_mapping_tbl mapping_tbl;
	int cur_br_idx;
	int cur_ref_br;
	int cur_phy_br;
	int cur_acl;

	int op_state;
	unsigned char syncReg[10];
};


#endif

