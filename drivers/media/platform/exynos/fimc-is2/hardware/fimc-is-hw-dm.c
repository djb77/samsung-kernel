/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/string.h>

#include "fimc-is-hw-dm.h"

#define CAMERA2_CTL(type)	(ctl->type)
#define CAMERA2_DM(type)	(dm->type)
#define CAMERA2_UCTL(type)	(uctl->type##Ud)
#define CAMERA2_UDM(type)	(udm->type)

#define DUP_SHOT_UCTL_UDM(type)						\
	do {											\
		memset(&CAMERA2_UDM(type), 0x00, sizeof(typeof(CAMERA2_UDM(type))));		\
		if (sizeof(typeof(CAMERA2_UCTL(type))) ==	\
			sizeof(typeof(CAMERA2_UDM(type))))		\
			memcpy((void *)&CAMERA2_UDM(type),		\
				(void *)&CAMERA2_UCTL(type),		\
				sizeof(typeof(CAMERA2_UDM(type))));	\
	} while (0)

#define CPY_SHOT_UCTL_UDM(type, item)			\
	do {										\
		memset(&(CAMERA2_UDM(type).item), 0x00, sizeof(typeof(CAMERA2_UDM(type).item)));		\
			memcpy((void *)&(CAMERA2_UDM(type).item),		\
				(void *)&(CAMERA2_UCTL(type).item),			\
				sizeof(typeof(CAMERA2_UDM(type).item)));	\
	} while (0)

static inline void copy_aa_udm(camera2_uctl_t *uctl, camera2_udm_t *udm)
{
	DUP_SHOT_UCTL_UDM(aa);
}

static inline void copy_lens_udm(camera2_uctl_t *uctl, camera2_udm_t *udm)
{
	if (uctl->lensUd.posSize != 0)
		DUP_SHOT_UCTL_UDM(lens);
	else
		memset(&udm->lens, 0x00, sizeof(camera2_lens_udm_t));

}

#define CPY_COMPANION_UDM(item)	CPY_SHOT_UCTL_UDM(companion, item)
static inline void copy_companion_udm(camera2_uctl_t *uctl,
					camera2_udm_t *udm)
{
	CPY_COMPANION_UDM(drc_mode);
	CPY_COMPANION_UDM(wdr_mode);
	CPY_COMPANION_UDM(paf_mode);
	CPY_COMPANION_UDM(lsc_mode);
	CPY_COMPANION_UDM(bpc_mode);
	CPY_COMPANION_UDM(bypass_mode);
}

int copy_ctrl_to_dm(struct camera2_shot *shot)
{
	/* udm */
	/* aa */
	/*
	 * both af_data, gyroInfo are meaningful
	 * only for a sensor with AF actuator.
	 */
	copy_aa_udm(&shot->uctl, &shot->udm);
	/* lens */
	copy_lens_udm(&shot->uctl, &shot->udm);
	/* companion */
	copy_companion_udm(&shot->uctl, &shot->udm);

	return 0;
}
