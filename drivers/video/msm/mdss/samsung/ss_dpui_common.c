/*
 * linux/drivers/video/fbdev/exynos/panel/ss_dpui_common.c
 *
 * Samsung Common LCD DPUI(display use info) LOGGING Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fb.h>
#include <linux/notifier.h>
#include <linux/export.h>
#include "ss_dpui_common.h"

static BLOCKING_NOTIFIER_HEAD(dpui_notifier_list);

static const char * const dpui_key_name[] = {
	[DPUI_KEY_NONE] = "NONE",
	[DPUI_KEY_WCRD_X] = "WCRD_X",
	[DPUI_KEY_WCRD_Y] = "WCRD_Y",
	[DPUI_KEY_WOFS_R] = "WOFS_R",
	[DPUI_KEY_WOFS_G] = "WOFS_G",
	[DPUI_KEY_WOFS_B] = "WOFS_B",
	[DPUI_KEY_VSYE] = "VSYE",
	[DPUI_KEY_DSIE] = "DSIE",
	[DPUI_KEY_PNTE] = "PNTE",
	[DPUI_KEY_ESDD] = "ESDD",
	[DPUI_KEY_LCDID1] = "LCDM_ID1",
	[DPUI_KEY_LCDID2] = "LCDM_ID2",
	[DPUI_KEY_LCDID3] = "LCDM_ID3",
	[DPUI_KEY_MAID_DATE] = "MAID_DATE",
	[DPUI_KEY_MAID_TIME] = "MAID_TIME",
};

static const char * const dpui_type_name[] = {
	[DPUI_TYPE_NONE] = "NONE",
	[DPUI_TYPE_MDNIE] = "MDNIE",
	[DPUI_TYPE_PANEL] = "PANEL",
	[DPUI_TYPE_DISP] = "DISP",
	[DPUI_TYPE_MIPI] = "MIPI",
};


static struct dpui_info dpui = {
	.pdata = NULL,
	.field = {
		/* common hw parameter */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_MDNIE, DPUI_KEY_WCRD_X),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_MDNIE, DPUI_KEY_WCRD_Y),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_MDNIE, DPUI_KEY_WOFS_R),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_MDNIE, DPUI_KEY_WOFS_G),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_MDNIE, DPUI_KEY_WOFS_B),

		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_KEY_LCDID1),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_KEY_LCDID2),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_KEY_LCDID3),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_KEY_MAID_DATE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_KEY_MAID_TIME),

		/* common hw parameter - for debug */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_DISP, DPUI_KEY_VSYE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL, DPUI_KEY_DSIE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL, DPUI_KEY_PNTE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL, DPUI_KEY_ESDD),

		/* hw dependent parameter */
	},
};

/**
 * dpui_logging_notify - notify clients of fb_events
 * @val: dpui log type
 * @v : data
 *
 */
void dpui_logging_notify(unsigned long val, void *v)
{
	blocking_notifier_call_chain(&dpui_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(dpui_logging_notify);

/**
 *	dpui_logging_register - register a client notifier
 *	@n: notifier block to callback on events
 */
int dpui_logging_register(struct notifier_block *n, enum dpui_type type)
{
	int ret;

	if (type <= DPUI_TYPE_NONE || type >= MAX_DPUI_TYPE) {
		pr_err("%s out of dpui_type range (%d)\n", __func__, type);
		return -EINVAL;
	}

	ret = blocking_notifier_chain_register(&dpui_notifier_list, n);
	pr_info("%s register type %s\n", __func__, dpui_type_name[type]);

	return ret;
}
EXPORT_SYMBOL_GPL(dpui_logging_register);

/**
 *	dpui_logging_unregister - unregister a client notifier
 *	@n: notifier block to callback on events
 */
int dpui_logging_unregister(struct notifier_block *n)
{
	return blocking_notifier_chain_unregister(&dpui_notifier_list, n);
}
EXPORT_SYMBOL_GPL(dpui_logging_unregister);

void update_dpui_log(enum dpui_log_level level)
{
	if (level < 0 || level >= MAX_DPUI_LOG_LEVEL) {
		pr_err("%s invalid log level %d\n", __func__, level);
		return;
	}
	dpui_logging_notify(level, &dpui);
	pr_info("%s update dpui log(%d) done\n",
			__func__, level);
}

int get_dpui_field(enum dpui_key key, char *buf)
{
	if (!buf) {
		pr_err("%s buf is null\n", __func__);
		return 0;
	}

	if (key <= DPUI_KEY_NONE || key >= MAX_DPUI_KEY) {
		pr_err("%s out of dpui_key range (%d)\n", __func__, key);
		return 0;
	}

	return snprintf(buf, MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN,
			"\"%s\":\"%s\"", dpui_key_name[key], dpui.field[key].buf);
}

void print_dpui_field(enum dpui_key key)
{
	char tbuf[MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN];

	if (key <= DPUI_KEY_NONE || key >= MAX_DPUI_KEY) {
		pr_err("%s out of dpui_key range (%d)\n", __func__, key);
		return;
	}

	get_dpui_field(key, tbuf);
	pr_info("DPUI: %s\n", tbuf);
}

int set_dpui_field(enum dpui_key key, char *buf, int size)
{
	if (!buf) {
		pr_err("%s buf is null\n", __func__);
		return -EINVAL;
	}

	if (key <= DPUI_KEY_NONE || key >= MAX_DPUI_KEY) {
		pr_err("%s out of dpui_key range (%d)\n", __func__, key);
		return -EINVAL;
	}

	if (size > MAX_DPUI_VAL_LEN) {
		pr_err("%s exceed dpui value size (%d)\n", __func__, size);
		return -EINVAL;
	}
	memcpy(dpui.field[key].buf, buf, size);
	dpui.field[key].initialized = true;

	pr_info("%s DPUI: %s\n", __func__, buf);

	return 0;
}

int get_dpui_log(char *buf, enum dpui_log_level level)
{
	int i, ret, len = 0;
	char tbuf[MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN];

	if (!buf) {
		pr_err("%s buf is null\n", __func__);
		return 0;
	}

	if (level < 0 || level >= MAX_DPUI_LOG_LEVEL) {
		pr_err("%s invalid log level %d\n", __func__, level);
		return 0;
	}

	for (i = DPUI_KEY_NONE + 1; i < MAX_DPUI_KEY; i++) {
		if (level != DPUI_LOG_LEVEL_ALL && dpui.field[i].level != level) {
			pr_warn("%s DPUI:%s different log level %d %d\n",
					__func__, dpui_key_name[dpui.field[i].key],
					dpui.field[i].level, level);
			continue;
		}

		if (!dpui.field[i].initialized) {
			pr_warn("%s DPUI:%s not initialized\n",
					__func__, dpui_key_name[dpui.field[i].key]);
			continue;
		}

		ret = get_dpui_field(i, tbuf);
		if (ret == 0)
			continue;

		if (len)
			len += snprintf(buf + len, 3, ", ");
		len += snprintf(buf + len, MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN,
				"%s", tbuf);
	}

	return len;
}
EXPORT_SYMBOL_GPL(get_dpui_log);
