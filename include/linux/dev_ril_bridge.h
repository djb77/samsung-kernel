/*
 * Copyright (C) 2017 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DEV_RIL_BRIDGE_H__
#define __DEV_RIL_BRIDGE_H__

#ifdef CONFIG_DEV_RIL_BRIDGE
extern int register_dev_ril_bridge_event_notifier(struct notifier_block *nb);

#else
static inline int register_dev_ril_bridge_event_notifier(
		struct notifier_block *nb)	{}
#endif

#endif/*__DEV_RIL_BRIDGE_H__*/
