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

enum mcu_err_context {
	muc_err_from_normal,
	mcu_err_from_irq,
	mcu_err_from_abort,

	mcu_err_context_max
};

extern void	iva_mcu_print_flush_pending_mcu_log(struct iva_dev_data *iva);

extern void	iva_mcu_handle_error_k(struct iva_dev_data *iva,
				enum mcu_err_context from, uint32_t msg);

extern void	iva_mcu_show_status(struct iva_dev_data *iva);

extern int32_t	iva_mcu_boot_file(struct iva_dev_data *iva, const char *mcu_file,
			int32_t wait_ready);
extern int32_t	iva_mcu_exit(struct iva_dev_data *iva);

extern int32_t	iva_mcu_probe(struct iva_dev_data *iva);

extern void	iva_mcu_remove(struct iva_dev_data *iva);

extern void	iva_mcu_shutdown(struct iva_dev_data *iva);

#endif /* _IVA_MCU_H_ */
