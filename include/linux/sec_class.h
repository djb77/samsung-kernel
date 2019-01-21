/*
 * include/linux/sec_class.h
 *
 * COPYRIGHT(C) 2011-2016 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifdef CONFIG_DRV_SAMSUNG
extern struct device *sec_dev_get_by_name(char *name);
extern struct device *sec_device_create(dev_t devt,
			void *drvdata, const char *fmt);
extern void sec_device_destroy(dev_t devt);
#else
#define sec_dev_get_by_name(name)		NULL
#define sec_device_create(devt, drvdata, fmt)	NULL
#define sec_device_destroy(devt)		NULL
#endif /* CONFIG_DRV_SAMSUNG */
