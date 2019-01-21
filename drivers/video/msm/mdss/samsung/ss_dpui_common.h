/*
 * linux/drivers/video/fbdev/exynos/panel/ss_dpui_common.h
 *
 * Header file for Samsung Common LCD Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DPUI_H__
#define __DPUI_H__

#define MAX_DPUI_KEY_LEN	(20)
#define MAX_DPUI_VAL_LEN	(128)

enum dpui_log_level {
	DPUI_LOG_LEVEL_DEBUG,
	DPUI_LOG_LEVEL_INFO,
	DPUI_LOG_LEVEL_ALL,
	MAX_DPUI_LOG_LEVEL,
};

enum dpui_type {
	DPUI_TYPE_NONE,
	DPUI_TYPE_MDNIE,
	DPUI_TYPE_PANEL,
	DPUI_TYPE_DISP,
	DPUI_TYPE_MIPI,
	DPUI_TYPE_ALL,
	MAX_DPUI_TYPE,
};

enum dpui_key {
	DPUI_KEY_NONE,
	DPUI_KEY_WCRD_X,	/* white color x-coordinate */
	DPUI_KEY_WCRD_Y,	/* white color y-coordinate */
	DPUI_KEY_WOFS_R,	/* mdnie whiteRGB red offset from user */
	DPUI_KEY_WOFS_G,	/* mdnie whiteRGB green offset from user */
	DPUI_KEY_WOFS_B,	/* mdnie whiteRGB blue offset from user */
	DPUI_KEY_VSYE,	/* vsync timeout count */
	DPUI_KEY_DSIE,	/* dsi r/w error */
	DPUI_KEY_PNTE,	/* panel te min, max */
	DPUI_KEY_ESDD,	/* esd detect */
	DPUI_KEY_LCDID1, /* Fab, window color */
	DPUI_KEY_LCDID2, /* Touch IC, DCDC IC, EL material */
	DPUI_KEY_LCDID3, /* D-IC, Op code */
	DPUI_KEY_MAID_DATE, /* Manufacture date */
	DPUI_KEY_MAID_TIME, /* Manufacture time */
	MAX_DPUI_KEY,
};

struct dpui_field {
	enum dpui_type type;
	bool initialized;
	enum dpui_key key;
	enum dpui_log_level level;
	char buf[MAX_DPUI_VAL_LEN];
};

#define DPUI_FIELD_INIT(_level_, _type_, _key_)	\
	[(_key_)] = {								\
		.type = (_type_),						\
		.initialized = (false),					\
		.key = (_key_),							\
		.level = (_level_),						\
		.buf[0] = ('\0'),						\
	}

struct dpui_info {
	void *pdata;
	struct dpui_field field[MAX_DPUI_KEY];
};

#ifdef CONFIG_DISPLAY_USE_INFO
void dpui_logging_notify(unsigned long, void *);
int dpui_logging_register(struct notifier_block *, enum dpui_type);
int dpui_logging_unregister(struct notifier_block *);
void update_dpui_log(enum dpui_log_level);
int get_dpui_log(char *, enum dpui_log_level);
int set_dpui_field(enum dpui_key, char *, int);
#else
static inline void dpui_logging_notify(unsigned long val, void *v) { return; }
static inline int dpui_logging_register(struct notifier_block *n, enum dpui_type type) { return 0; }
static inline int dpui_logging_unregister(struct notifier_block *n, enum dpui_type type) { return 0; }
static inline void update_dpui_log(enum dpui_log_level level) { return; }
static inline int set_dpui_field(enum dpui_key key, char *buf, int size) { return 0; }
static inline int get_dpui_log(char *buf, enum dpui_log_level level) { return 0; }
#endif /* CONFIG_DISPLAY_USE_INFO */
#endif /* __DPUI_H__ */
