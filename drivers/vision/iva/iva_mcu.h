/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef _IVA_MCU_H_
#define _IVA_MCU_H_

#include "iva_ctrl.h"

extern void	iva_mcu_handle_panic_k(struct iva_dev_data *iva);
extern int32_t	iva_mcu_wait_ready(struct iva_dev_data *iva);
extern int32_t	iva_mcu_boot_file(struct iva_dev_data *iva, const char *mcu_file,
			int32_t wait_ready);
extern int32_t	iva_mcu_exit(struct iva_dev_data *iva);

extern int32_t	iva_mcu_probe(struct iva_dev_data *iva);

extern void	iva_mcu_remove(struct iva_dev_data *iva);

extern void	iva_mcu_shutdown(struct iva_dev_data *iva);

#endif /* _IVA_MCU_H_ */
