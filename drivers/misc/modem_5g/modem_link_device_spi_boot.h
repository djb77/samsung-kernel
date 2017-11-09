/*
 * Copyright (C) 2010 Google, Inc.
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

#ifndef __MODEM_LINK_DEVICE_SPI_BOOT_H__
#define __MODEM_LINK_DEVICE_SPI_BOOT_H__

struct spi_boot_link_device {
	struct link_device ld;
	struct spi_device *spi;
	struct sk_buff_head tx_q;
	unsigned int gpio_cp_status;
};

#endif
