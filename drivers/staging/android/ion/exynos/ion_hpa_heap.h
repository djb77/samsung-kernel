/*
 * drivers/staging/android/ion/exynos/ion_hpa_heap.h
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ION_HPA_HEAP_H
#define _ION_HPA_HEAP_H

#define ION_HPA_DEFAULT_ORDER 4
#define ION_HPA_DEFAULT_PAGE_ORDER (4 + PAGE_SHIFT)
#define ION_HPA_DEFAULT_SIZE  (PAGE_SIZE << ION_HPA_DEFAULT_ORDER)
#define ION_HPA_PAGE_COUNT(len) \
		(ALIGN(len, ION_HPA_DEFAULT_SIZE) / ION_HPA_DEFAULT_SIZE)

#ifndef CONFIG_HPA
struct ion_heap *ion_hpa_heap_create(struct ion_platform_heap *data)
{
	return NULL;
}

void ion_hpa_heap_destroy(struct ion_heap *heap)
{
}
#endif
#endif /* _ION_HPA_HEAP_H */

