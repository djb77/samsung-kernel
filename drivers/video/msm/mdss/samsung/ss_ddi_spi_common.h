#ifndef SAMSUNG_SPI_COMMON_H
#define SAMSUNG_SPI_COMMON_H

#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include "ss_dsi_panel_common.h"

#define DRIVER_NAME "ddi_spi"

int ss_spi_read(struct spi_device *spi, u8 rxaddr, u8 *buf, int size);

#endif
