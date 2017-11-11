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

#define MDNIE_COLOR_BLIND_TUNE_CNT	9
#define MDNIE_COLOR_BLIND_HBM_TUNE_CNT 12


enum HBM {
	HBM_OFF,
	HBM_ON,
	HBM_ON_TEXT,
	HBM_MAX,
};

enum MDNIE_CMD {
#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
	LEVEL1_KEY_UNLOCK,
	MDNIE_CMD1,
	MDNIE_CMD2,
#if defined(CONFIG_PANEL_S6E3HA3_DYNAMIC) || defined(CONFIG_PANEL_S6E3HF3_DYNAMIC)
	MDNIE_CMD3,
#endif
	LEVEL1_KEY_LOCK,
	MDNIE_CMD_MAX,
#else
	MDNIE_CMD1,
	MDNIE_CMD2 = MDNIE_CMD1,
	MDNIE_CMD_MAX,
#endif
};

struct mdnie_command {
	mdnie_t *sequence;
	unsigned int size;
	unsigned int sleep;
};

struct mdnie_table {
	char *name;
	struct mdnie_command tune[MDNIE_CMD_MAX];
};

#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
#if defined(CONFIG_PANEL_S6E3HA3_DYNAMIC) || defined(CONFIG_PANEL_S6E3HF3_DYNAMIC)
#define MDNIE_SET(id)	\
{							\
	.name		= #id,				\
	.tune		= {				\
		{	.sequence = LEVEL1_UNLOCK, .size = ARRAY_SIZE(LEVEL1_UNLOCK),	.sleep = 0,},	\
		{	.sequence = id##_1, .size = ARRAY_SIZE(id##_1),			.sleep = 0,},	\
		{	.sequence = id##_2, .size = ARRAY_SIZE(id##_2),			.sleep = 0,},	\
		{	.sequence = id##_3, .size = ARRAY_SIZE(id##_3),			.sleep = 0,},	\
		{	.sequence = LEVEL1_LOCK, .size = ARRAY_SIZE(LEVEL1_LOCK),	.sleep = 0,},	\
	}	\
}
#else
#define MDNIE_SET(id)	\
{							\
	.name		= #id,				\
	.tune		= {				\
		{	.sequence = LEVEL1_UNLOCK, .size = ARRAY_SIZE(LEVEL1_UNLOCK),	.sleep = 0,},	\
		{	.sequence = id##_1, .size = ARRAY_SIZE(id##_1),			.sleep = 0,},	\
		{	.sequence = id##_2, .size = ARRAY_SIZE(id##_2),			.sleep = 0,},	\
		{	.sequence = LEVEL1_LOCK, .size = ARRAY_SIZE(LEVEL1_LOCK),	.sleep = 0,},	\
	}	\
}
#endif

struct mdnie_ops {
	int (*write)(void *data, struct mdnie_command *seq, u32 len);
	int (*read)(void *data, u8 addr, mdnie_t *buf, u32 len);
};

typedef int (*mdnie_w)(void *devdata, struct mdnie_command *seq, u32 len);
typedef int (*mdnie_r)(void *devdata, u8 addr, u8 *buf, u32 len);
#else
#define MDNIE_SET(id)	\
{							\
	.name		= #id,				\
	.tune		= {				\
		{	.sequence = id##_1, .size = ARRAY_SIZE(id##_1), .sleep = 0,},	\
	}	\
}

struct mdnie_ops {
	int (*write)(unsigned int a, unsigned int v);
	int (*read)(unsigned int a);
};

typedef int (*mdnie_w)(unsigned int a, unsigned int v);
typedef int (*mdnie_r)(unsigned int a);
#endif

struct mdnie_info {
	struct clk		*bus_clk;
	struct clk		*clk;

	struct device		*dev;
	struct mutex		dev_lock;
	struct mutex		lock;

	unsigned int		enable;

	enum SCENARIO		scenario;
	enum MODE		mode;
	enum BYPASS		bypass;
	enum HBM		hbm;

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
extern int mdnie_register(struct device *p, void *data, mdnie_w w, mdnie_r r, u16 *coordinate, int id);
#endif

#if defined(CONFIG_EXYNOS_DECON_DUAL_DISPLAY) && defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
extern int mdnie2_register(struct device *p, void *data, mdnie_w w, mdnie_r r);
#endif
extern struct mdnie_table *mdnie_request_table(char *path, struct mdnie_table *s);

#endif /* __MDNIE_H__ */
