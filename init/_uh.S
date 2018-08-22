/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/uh.h>

#define uh_ELF_PATH "init/uh.elf"

#define SMC_64BIT_RET_MAGIC 0xC2000401

.section .init.text, "ax"
.global _uh_goto_EL2
_uh_goto_EL2:
	smc #0
	isb
	ret

.global _uh_disable
_uh_disable:
	ldr x0, =SMC_64BIT_RET_MAGIC
	smc #0
	isb

.global _start_uh
_start_uh:
.incbin uh_ELF_PATH
.global _end_uh
_end_uh:
.section .text
