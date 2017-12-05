/*
*  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*/
#include "ssp.h"

#if defined (CONFIG_SENSORS_SSP_GREAT)
#define SSP_FIRMWARE_REVISION_BCM	17101200
#elif defined(CONFIG_SENSORS_SSP_VLTE)
#define SSP_FIRMWARE_REVISION_BCM   17063000
#elif defined(CONFIG_SENSORS_SSP_LUGE)
#define SSP_FIRMWARE_REVISION_BCM   17092000
#else
#define SSP_FIRMWARE_REVISION_BCM	00000000
#endif

unsigned int get_module_rev(struct ssp_data *data)
{
	return SSP_FIRMWARE_REVISION_BCM;
}
