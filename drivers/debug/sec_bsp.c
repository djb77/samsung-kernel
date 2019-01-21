/*
 * drivers/debug/sec_bsp.c
 *
 * COPYRIGHT(C) 2014-2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/sec_class.h>
#include <soc/qcom/boot_stats.h>

#include <linux/sec_bsp.h>

#define BOOT_EVT_PREFIX_LK		"lk "
#define BOOT_EVT_PREFIX			"!@Boot"
#define BOOT_EVT_PREFIX_PLATFORM	": "
#define BOOT_EVT_PREFIX_RIL		"_SVC : "
#define BOOT_EVT_PREFIX_DEBUG		"_DEBUG: "

#define DEFAULT_BOOT_STAT_FREQ		32768

uint32_t bootloader_start;
uint32_t bootloader_end;
uint32_t bootloader_display;
uint32_t bootloader_load_kernel;

static bool console_enabled;
static unsigned int __is_boot_recovery;

static const char *boot_prefix[16] = {
	BOOT_EVT_PREFIX_LK,
	BOOT_EVT_PREFIX BOOT_EVT_PREFIX_PLATFORM,
	BOOT_EVT_PREFIX BOOT_EVT_PREFIX_RIL,
	BOOT_EVT_PREFIX BOOT_EVT_PREFIX_DEBUG
};

enum boot_events_prefix {
	EVT_LK,
	EVT_PLATFORM,
	EVT_RIL,
	EVT_DEBUG,
	EVT_INVALID,
};

enum boot_events_type {
	SYSTEM_START_LK,
	SYSTEM_LK_LOGO_DISPLAY,
	SYSTEM_END_LK,
	SYSTEM_START_INIT_PROCESS,
	PLATFORM_START_PRELOAD,
	PLATFORM_END_PRELOAD,
	PLATFORM_START_INIT_AND_LOOP,
	PLATFORM_START_PACKAGEMANAGERSERVICE,
	PLATFORM_END_PACKAGEMANAGERSERVICE,
	PLATFORM_END_INIT_AND_LOOP,
	PLATFORM_PERFORMENABLESCREEN,
	PLATFORM_ENABLE_SCREEN,
	PLATFORM_BOOT_COMPLETE,
	PLATFORM_VOICE_SVC,
	PLATFORM_DATA_SVC,
	PLATFORM_START_NETWORK,
	PLATFORM_END_NETWORK,
	PLATFORM_PHONEAPP_ONCREATE,
	RIL_UNSOL_RIL_CONNECTED,
	RIL_SETRADIOPOWER_ON,
	RIL_SETUICCSUBSCRIPTION,
	RIL_SIM_RECORDSLOADED,
	RIL_RUIM_RECORDSLOADED,
	RIL_SETUPDATACALL,
	NUM_BOOT_EVENTS,
};

struct boot_event {
	enum boot_events_type type;
	enum boot_events_prefix prefix;
	const char *string;
	unsigned int time;
};

static int num_events;
static int boot_events_seq[NUM_BOOT_EVENTS];
static struct boot_event boot_events[] = {
	{SYSTEM_START_LK, EVT_LK,
			"start", 0},
	{SYSTEM_LK_LOGO_DISPLAY, EVT_LK,
			"logo display", 0},
	{SYSTEM_END_LK, EVT_LK,
			"end", 0},
	{SYSTEM_START_INIT_PROCESS, EVT_PLATFORM,
			"start init process", 0},
	{PLATFORM_START_PRELOAD, EVT_PLATFORM,
			"Begin of preload()", 0},
	{PLATFORM_END_PRELOAD, EVT_PLATFORM,
			"End of preload()", 0},
	{PLATFORM_START_INIT_AND_LOOP, EVT_PLATFORM,
			"Entered the Android system server!", 0},
	{PLATFORM_START_PACKAGEMANAGERSERVICE, EVT_PLATFORM,
			"Start PackageManagerService", 0},
	{PLATFORM_END_PACKAGEMANAGERSERVICE, EVT_PLATFORM,
			"End PackageManagerService", 0},
	{PLATFORM_END_INIT_AND_LOOP, EVT_PLATFORM,
			"Loop forever", 0},
	{PLATFORM_PERFORMENABLESCREEN, EVT_PLATFORM,
			"performEnableScreen", 0},
	{PLATFORM_ENABLE_SCREEN, EVT_PLATFORM,
			"Enabling Screen!", 0},
	{PLATFORM_BOOT_COMPLETE, EVT_PLATFORM,
			"bootcomplete", 0},
	{PLATFORM_VOICE_SVC, EVT_PLATFORM,
			"Voice SVC is acquired", 0},
	{PLATFORM_DATA_SVC, EVT_PLATFORM,
			"Data SVC is acquired", 0},
	{PLATFORM_START_NETWORK, EVT_DEBUG,
			"start networkManagement", 0},
	{PLATFORM_END_NETWORK, EVT_DEBUG,
			"end networkManagement", 0},
	{PLATFORM_PHONEAPP_ONCREATE, EVT_RIL,
			"PhoneApp OnCrate", 0},
	{RIL_UNSOL_RIL_CONNECTED, EVT_RIL,
			"RIL_UNSOL_RIL_CONNECTED", 0},
	{RIL_SETRADIOPOWER_ON, EVT_RIL,
			"setRadioPower on", 0},
	{RIL_SETUICCSUBSCRIPTION, EVT_RIL,
			"setUiccSubscription", 0},
	{RIL_SIM_RECORDSLOADED, EVT_RIL,
			"SIM onAllRecordsLoaded", 0},
	{RIL_RUIM_RECORDSLOADED, EVT_RIL,
			"RUIM onAllRecordsLoaded", 0},
	{RIL_SETUPDATACALL, EVT_RIL,
			"setupDataCall", 0},
	{0, EVT_INVALID, NULL, 0},
};

static int __init boot_recovery(char *str)
{
	int temp = 0;

	if (get_option(&str, &temp)) {
		__is_boot_recovery = temp;
		return 0;
	}

	return -EINVAL;
}
early_param("androidboot.boot_recovery", boot_recovery);

unsigned int is_boot_recovery(void)
{
	return __is_boot_recovery;
}
EXPORT_SYMBOL(is_boot_recovery);

static int sec_boot_stat_proc_show(struct seq_file *m, void *v)
{
	size_t i;
	unsigned long delta = 0;
	unsigned long freq = (unsigned long)get_boot_stat_freq();
	unsigned long time, prev_time = 0;
	char boot_string[256];

	if (!freq)
		freq = DEFAULT_BOOT_STAT_FREQ;

	seq_printf(m, "%-48s%s%13s\n", "boot event", "time(sec)", "delta(sec)");
	seq_puts(m, "------------------------------------");
	seq_puts(m, "----------------------------------\n");

	/* print boot_events logged */
	for (i = 0; i < num_events; i++) {
		int seq = boot_events_seq[i];

		time = (unsigned long)boot_events[seq].time * 1000 / freq;
		delta = time - prev_time;

		sprintf(boot_string, "%s%s",
				boot_prefix[boot_events[seq].prefix],
				boot_events[seq].string);

		seq_printf(m, "%-45s : %5lu.%03lu    %5lu.%03lu\n",
				boot_string,
				time/1000, time%1000, delta/1000, delta%1000);

		prev_time = time;
	}

	/* print boot_events not logged (time = 0) */
	for (i = 0; boot_events[i].string; i++) {
		if (boot_events[i].time)
			continue;

		sprintf(boot_string, "%s%s",
				boot_prefix[boot_events[i].prefix],
				boot_events[i].string);

		seq_printf(m, "%-45s : %5u.%03u    %5u.%03u\n",
				boot_string, 0, 0, 0, 0);
	}

	return 0;
}

static int sec_boot_stat_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_boot_stat_proc_show, NULL);
}

static const struct file_operations sec_boot_stat_proc_fops = {
	.open    = sec_boot_stat_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

void sec_boot_stat_record(int idx, int time)
{
	boot_events[idx].time = time;
	boot_events_seq[num_events++] = idx;
}

void sec_boot_stat_add(const char *c)
{
	size_t i;
	unsigned int prefix;
	char *android_log;

	if (strncmp(c, BOOT_EVT_PREFIX, 6))
		return;

	android_log = (char *)(c + 6);
	if (!strncmp(android_log, BOOT_EVT_PREFIX_PLATFORM, 2)) {
		prefix = EVT_PLATFORM;
		android_log = (char *)(android_log + 2);
	} else if (!strncmp(android_log, BOOT_EVT_PREFIX_RIL, 7)) {
		prefix = EVT_RIL;
		android_log = (char *)(android_log + 7);
	} else if (!strncmp(android_log, BOOT_EVT_PREFIX_DEBUG, 8)) {
		prefix = EVT_DEBUG;
		android_log = (char *)(android_log + 8);
	} else
		return;

	for (i = 0; boot_events[i].string; i++) {
		if (!strcmp(android_log, boot_events[i].string)) {
			if (!boot_events[i].time)
				sec_boot_stat_record(i, get_boot_stat_time());
			break;
		}
	}
}

static struct device *sec_bsp_dev;

static ssize_t store_boot_stat(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (!strncmp(buf, "!@Boot: start init process", 26))
		sec_boot_stat_record(SYSTEM_START_INIT_PROCESS,
						get_boot_stat_time());

	return count;
}
static DEVICE_ATTR(boot_stat, S_IWUSR | S_IWGRP, NULL, store_boot_stat);

void sec_bsp_enable_console(void)
{
	console_enabled = true;
}

bool sec_bsp_is_console_enabled(void)
{
	return console_enabled;
}

static int __init sec_bsp_init(void)
{
	int ret;
	struct proc_dir_entry *entry;

	entry = proc_create("boot_stat", S_IRUGO, NULL,
					&sec_boot_stat_proc_fops);
	if (!entry)
		return -ENOMEM;

	sec_boot_stat_record(SYSTEM_START_LK, bootloader_start);
	sec_boot_stat_record(SYSTEM_LK_LOGO_DISPLAY, bootloader_display);
	sec_boot_stat_record(SYSTEM_END_LK, bootloader_end);

	sec_bsp_dev = sec_device_create(0, NULL, "bsp");
	if (unlikely(IS_ERR(sec_bsp_dev))) {
		pr_err("%s:Failed to create devce\n", __func__);
		ret = PTR_ERR(sec_bsp_dev);
		goto err_dev_create;
	}

	ret = device_create_file(sec_bsp_dev, &dev_attr_boot_stat);
	if (unlikely(ret < 0)) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_dev_create_file;
	}

	return 0;

err_dev_create_file:
	sec_device_destroy(sec_bsp_dev->devt);
err_dev_create:
	proc_remove(entry);
	return ret;
}

module_init(sec_bsp_init);
