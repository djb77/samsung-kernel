#ifndef __MDNIE_H__
#define __MDNIE_H__

#define END_SEQ	0xffff


#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
typedef u8 mdnie_t;
#else
typedef u16 mdnie_t;
#endif

enum MODE {
	DYNAMIC,
	STANDARD,
	NATURAL,
	MOVIE,
	AUTO,
	MODE_MAX
};

enum SCENARIO {
	UI_MODE,
	VIDEO_NORMAL_MODE,
	CAMERA_MODE = 4,
	NAVI_MODE,
	GALLERY_MODE,
	VT_MODE,
	BROWSER_MODE,
	EBOOK_MODE,
	EMAIL_MODE,
#if defined(CONFIG_PANEL_S6E3HA3_DYNAMIC) || defined(CONFIG_PANEL_S6E3HF4_WQHD)
	GAME_LOW_MODE,
	GAME_MID_MODE,
	GAME_HIGH_MODE,
#endif
	HMT_8_MODE,
	HMT_16_MODE,
	SCENARIO_MAX,
	DMB_NORMAL_MODE = 20,
	DMB_MODE_MAX,
};

enum BYPASS {
	BYPASS_OFF,
	BYPASS_ON,
	BYPASS_MAX
};

enum ACCESSIBILITY {
	ACCESSIBILITY_OFF,
	NEGATIVE,
	COLOR_BLIND,
	SCREEN_CURTAIN,
	GRAYSCALE,
	GRAYSCALE_NEGATIVE,
	COLOR_BLIND_HBM,
	ACCESSIBILITY_MAX
};

enum HBM {
	HBM_OFF,
#if defined(CONFIG_PANEL_S6E3HA3_DYNAMIC) || defined(CONFIG_PANEL_S6E3HF3_DYNAMIC) || defined(CONFIG_PANEL_S6E3HF4_WQHD)
	HBM_ON,
	HBM_AOLCE_1 = HBM_ON,
	HBM_AOLCE_2,
	HBM_AOLCE_3,
#else
	HBM_ON,
	HBM_ON_TEXT,
#endif
	HBM_MAX,
};

enum hmt_mode {
	HMT_MDNIE_OFF,
	HMT_MDNIE_ON,
	HMT_3000K = HMT_MDNIE_ON,
	HMT_4000K,
	HMT_6400K,
	HMT_7500K,
	HMT_MDNIE_MAX,
};

struct mdnie_seq_info {
	mdnie_t *cmd;
	unsigned int len;
	unsigned int sleep;
};

struct mdnie_table {
	char *name;
	unsigned int update_flag[5];
	struct mdnie_seq_info seq[5 + 1];
};

struct mdnie_scr_info {
	u32 index;
	u32 color_blind;	/* Cyan Red */
	u32 white_r;
	u32 white_g;
	u32 white_b;
};

struct mdnie_trans_info {
	u32 index;
	u32 offset;
	u32 enable;
};

struct mdnie_tune {
	struct mdnie_table	*bypass_table;
	struct mdnie_table	*accessibility_table;
	struct mdnie_table	*hbm_table;
	struct mdnie_table	*hmt_table;
	struct mdnie_table	(*main_table)[MODE_MAX];
	struct mdnie_table	*dmb_table;

	struct mdnie_scr_info	*scr_info;
	struct mdnie_trans_info	*trans_info;
	unsigned char **coordinate_table;
	unsigned char **adjust_ldu_table;
	unsigned int max_adjust_ldu;
	int (*color_offset[])(int, int);
};

struct mdnie_ops {
	int (*write)(void *data, struct mdnie_seq_info *seq, u32 len);
	int (*read)(void *data, u8 addr, mdnie_t *buf, u32 len);
};

typedef int (*mdnie_w)(void *devdata, struct mdnie_seq_info *seq, u32 len);
typedef int (*mdnie_r)(void *devdata, u8 addr, u8 *buf, u32 len);


struct mdnie_info {
	struct clk		*bus_clk;
	struct clk		*clk;

	struct device		*dev;
	struct mutex		dev_lock;
	struct mutex		lock;

	unsigned int		enable;

	struct mdnie_tune 	*tune;

	enum SCENARIO		scenario;
	enum MODE		mode;
	enum BYPASS		bypass;
	enum HBM		hbm;
	enum hmt_mode	hmt_mode;

	unsigned int		tuning;
	unsigned int		accessibility;
	unsigned int		color_correction;
	unsigned int		auto_brightness;

	char			path[50];

	void			*data;

	struct mdnie_ops	ops;

	struct notifier_block	fb_notif;

	unsigned int white_r;
	unsigned int white_g;
	unsigned int white_b;
	unsigned int disable_trans_dimming;

	struct mdnie_table table_buffer;
	mdnie_t sequence_buffer[256];
	u16 coordinate[2];
#if defined(CONFIG_EXYNOS_DECON_MDNIE)
	mdnie_t *lpd_store_data;
	unsigned int need_update;
#endif

};

extern int mdnie_calibration(int *r);
extern int mdnie_open_file(const char *path, char **fp);

#if defined(CONFIG_EXYNOS_DECON_MDNIE)
extern struct mdnie_info* decon_mdnie_register(void);
extern void decon_mdnie_start(struct mdnie_info *mdnie, u32 w, u32 h);
extern void decon_mdnie_stop(struct mdnie_info *mdnie);
extern void decon_mdnie_frame_update(struct mdnie_info *mdnie, u32 xres, u32 yres);
extern u32 decon_mdnie_input_read(void);
#elif defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
extern int mdnie_register(struct device *p, void *data, mdnie_w w, mdnie_r r, u16 *coordinate, struct mdnie_tune *tune);
#endif

#if defined(CONFIG_EXYNOS_DECON_DUAL_DISPLAY) && defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
extern int mdnie2_register(struct device *p, void *data, mdnie_w w, mdnie_r r);
#endif
extern uintptr_t mdnie_request_table(char *path, struct mdnie_table *s);

#endif /* __MDNIE_H__ */
