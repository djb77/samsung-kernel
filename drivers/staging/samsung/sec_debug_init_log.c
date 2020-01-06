/*
 * sec_debug_init_log.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sec_debug.h>

char init_log_buffer[SZ_128K];
size_t init_log_size;
static unsigned long buf_idx;

static void sec_debug_hook_init_log(const char *str, size_t size)
{
	int len;

	if (buf_idx + size > init_log_size) {
		len = init_log_size - buf_idx;
		memcpy(init_log_buffer + buf_idx, str, len);
		memcpy(init_log_buffer, str + len, size - len);
		buf_idx = size - len;
	} else {
		memcpy(init_log_buffer + buf_idx, str, size);
		buf_idx = (buf_idx + size) % init_log_size;
	}
}

static int __init sec_debug_init_init_log(void)
{
	pr_err("%s: start\n", __func__);

	init_log_size = (size_t)SZ_128K;
	pr_err("%s: buffer size 0x%lx at addr %p\n", __func__,
			init_log_size, init_log_buffer);

	register_init_log_hook_func(sec_debug_hook_init_log);

	pr_err("%s: done\n", __func__);

	return 0;
}
late_initcall(sec_debug_init_init_log);

