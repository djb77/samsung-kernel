/*
 *
 * include/linux/tdmb_notifier.h
 *
 * header file supporting tdmb notifier call chain information
 *
 * Copyright (C) (2017, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __TDMB_NOTIFIER_H__
#define __TDMB_NOTIFIER_H__

/*largest priority number device will be called first. */
enum tdmb_notifier_device_t {
	TDMB_NOTIFY_DEV_LCD = 0,
	TDMB_NOTIFY_DEV_TOUCH,
};

enum tdmb_notifier_event_t {
	TDMB_NOTIFY_EVENT_INITIAL = 0,
	TDMB_NOTIFY_EVENT_TUNNER,
};

struct _tdmb_tunner_status {
	int pwr;
	int status_info;
};

struct tdmb_notifier_struct {
	enum tdmb_notifier_event_t event;
	struct _tdmb_tunner_status tdmb_status;
};

/* tdmb notifier register/unregister API*/
extern void tdmb_notifier_call(struct tdmb_notifier_struct *value);
extern int tdmb_notifier_register(struct notifier_block *nb,
		notifier_fn_t notifier, enum tdmb_notifier_device_t listener);
extern int tdmb_notifier_unregister(struct notifier_block *nb);

#endif /* __TDMB_NOTIFIER_H__ */
