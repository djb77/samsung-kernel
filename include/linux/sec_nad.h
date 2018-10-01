/* sec_nad.h
 *
 * Copyright (C) 2016 Samsung Electronics
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

#ifndef SEC_NAD_H
#define SEC_NAD_H

#if defined(CONFIG_SEC_FACTORY)
#define NAD_PARAM_NAME "/dev/block/NAD_REFER"
#define NAD_OFFSET 8192 

#define NAD_PARAM_READ  0
#define NAD_PARAM_WRITE 1 
#define NAD_PARAM_EMPTY 2

struct sec_nad_param {
	struct work_struct sec_nad_work;
	unsigned long offset;
	int state;
	char val[6];
};
static struct sec_nad_param sec_nad_param_data;
static DEFINE_MUTEX(sec_nad_param_mutex);

static char nad_result[3];
static int nad_data;
#endif

#endif
