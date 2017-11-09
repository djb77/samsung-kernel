/*
 * Copyright (C) 2010 Samsung Electronics.
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

#ifndef __MODEM_VARIATION_H__
#define __MODEM_VARIATION_H__

#include "include/modem_v1.h"

#define DECLARE_LINK_INIT(type)	\
		struct link_device *type ## _create_link_device(	\
		struct platform_device *pdev)

#define DECLARE_LINK_INIT_DUMMY(type)	\
	static DECLARE_LINK_INIT(type) { return NULL; }

#define LINK_INIT_CALL(type)	type ## _create_link_device

/* add declaration of modem link type */
/* link device support */
DECLARE_LINK_INIT_DUMMY(undefined)

#ifdef CONFIG_LINK_DEVICE_MIPI
DECLARE_LINK_INIT(mipi);
#else
DECLARE_LINK_INIT_DUMMY(mipi)
#endif

#ifdef CONFIG_LINK_DEVICE_HSIC
DECLARE_LINK_INIT(hsic);
#else
DECLARE_LINK_INIT_DUMMY(hsic)
#endif

#ifdef CONFIG_LINK_DEVICE_DPRAM
DECLARE_LINK_INIT(dpram);
#else
DECLARE_LINK_INIT_DUMMY(dpram)
#endif

#ifdef CONFIG_LINK_DEVICE_SHMEM
DECLARE_LINK_INIT(shmem);
#else
DECLARE_LINK_INIT_DUMMY(shmem)
#endif

#ifdef CONFIG_LINK_DEVICE_SPI
DECLARE_LINK_INIT(spi);
#else
DECLARE_LINK_INIT_DUMMY(spi)
#endif

typedef struct link_device *(*link_init_call)(struct platform_device *);
static link_init_call link_init_func[LINKDEV_MAX] = {
	[LINKDEV_UNDEFINED] = LINK_INIT_CALL(undefined),
	[LINKDEV_MIPI] = LINK_INIT_CALL(mipi),
	[LINKDEV_HSIC] = LINK_INIT_CALL(hsic),
	[LINKDEV_DPRAM] = LINK_INIT_CALL(dpram),
	[LINKDEV_SHMEM] = LINK_INIT_CALL(shmem),
	[LINKDEV_SPI] = LINK_INIT_CALL(spi),
};

static struct link_device *call_link_init_func(struct platform_device *pdev,
			enum modem_link link_type)
{
	if (link_init_func[link_type])
		return link_init_func[link_type](pdev);
	else
		return NULL;
}

int init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata);

#endif
