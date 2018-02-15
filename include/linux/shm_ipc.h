/*
 * Copyright (C) 2014 Samsung Electronics Co.Ltd
 * http://www.samsung.com
 *
 * Shared Memory driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
*/
#ifndef SHMEM_IPC_H
#define SHMEM_IPC_H

#define VSS_MAGIC_OFFSET 0x500000

struct shdmem_info {
	unsigned int base;
	unsigned int size;
};

unsigned long shm_get_phys_base(void);
unsigned shm_get_phys_size(void);
unsigned shm_get_boot_size(void);
unsigned shm_get_ipc_rgn_offset(void);
unsigned shm_get_ipc_rgn_size(void);
unsigned shm_get_zmb_size(void);
unsigned shm_get_vss_size(void);
unsigned shm_get_acpm_size(void);
unsigned shm_get_cp_size(void);
unsigned long shm_get_security_param2(unsigned long mode, u32 bl_size);
unsigned long shm_get_security_param3(unsigned long mode, u32 main_size);

void __iomem *shm_request_region(unsigned long sh_addr, unsigned size);
void __iomem *shm_get_boot_region(void);
void __iomem *shm_get_ipc_region(void);
void __iomem *shm_get_zmb_region(void);
void __iomem *shm_get_vss_region(void);
void __iomem *shm_get_acpm_region(void);

void shm_release_region(void *v_addr);
void shm_release_regions(void);
void clean_vss_magic_code(void);

#ifdef CONFIG_CP_RAM_LOGGING
unsigned long shm_get_cplog_base(void);
unsigned shm_get_cplog_size(void);
int shm_get_cplog_flag(void);
void __iomem *shm_get_cplog_region(void);
#endif

#endif
