/*
 * include/linux/sec_bsp.h
 *
 * COPYRIGHT(C) 2014-2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef SEC_BSP_H
#define SEC_BSP_H

#ifdef CONFIG_SEC_BSP
extern uint32_t bootloader_start;
extern uint32_t bootloader_end;
extern uint32_t bootloader_display;
extern uint32_t bootloader_load_kernel;

extern unsigned int is_boot_recovery(void);
extern unsigned int get_boot_stat_time(void);
extern unsigned int get_boot_stat_freq(void);
extern void sec_boot_stat_add(const char * c);
extern void sec_bsp_enable_console(void);
extern bool sec_bsp_is_console_enabled(void);
#else /* CONFIG_SEC_BSP */
#define is_boot_recovery()      (0)
#define get_boot_stat_time()
#define get_boot_stat_freq()
#define sec_boot_stat_add(c)
#define sec_bsp_enable_console()
#define sec_bsp_is_console_enabled()	(0)
#endif /* CONFIG_SEC_BSP */

#endif /* SEC_BSP_H */
