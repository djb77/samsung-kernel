/* drivers/gpu/arm/{gpu arch}/{ddk version}/mali_kbase_platform_uku.h
 *
 * Copyright 2017 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali-T Series platform-dependent codes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file mali_kbase_platform_uku.h
 * Platform-dependent ioctl parameter
 */

#ifndef _KBASE_PLATFORM_UKU_H_
#define _KBASE_PLATFORM_UKU_H_

#include "mali_uk.h"
#include "mali_malisw.h"
#include "mali_base_kernel.h"

struct kbase_uk_custom_command {
	union uk_header header;
	u32       enabled;
	u32       padding;
};

#endif
