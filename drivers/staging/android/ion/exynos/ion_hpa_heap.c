/*
 * drivers/staging/android/ion/ion_hpa_heap.c
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 * Author: <pullip.cho@samsung.com> for Exynos SoCs
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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/spinlock.h>
#include <linux/ion.h>
#include <linux/exynos_ion.h>

#include "../ion_priv.h"
#include "ion_hpa_heap.h"

static int ion_hpa_compare_pages(const void *p1, const void *p2)
{
	if (*((unsigned long *)p1) > (*((unsigned long *)p2)))
		return 1;
	else if (*((unsigned long *)p1) < (*((unsigned long *)p2)))
		return -1;
	return 0;
}

static int ion_hpa_heap_allocate(struct ion_heap *heap,
				 struct ion_buffer *buffer, unsigned long len,
				 unsigned long align, unsigned long flags)
{
	unsigned int count = ION_HPA_PAGE_COUNT((unsigned int)len);
	bool protected = !!(flags & ION_FLAG_PROTECTED);
	/* a protected buffer should be cache-flushed */
	bool cacheflush = !(flags & ION_FLAG_CACHED) || protected;
	bool memzero = !(flags & ION_FLAG_NOZEROED) || protected;
	struct page **pages;
	unsigned long *phys;
	struct scatterlist *sg;
	struct ion_buffer_info *info;
	int ret, i;

	if (count > ((PAGE_SIZE * 2) / sizeof(*pages))) {
		pr_info("ION HPA heap does not allow buffers > %zu\n",
			((PAGE_SIZE * 2) / sizeof(*pages)) * ION_HPA_DEFAULT_SIZE);
		return -ENOMEM;
	}

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	pages = kmalloc(sizeof(*pages) * count, GFP_KERNEL);
	if (!pages) {
		ret = -ENOMEM;
		goto err_pages;
	}

	/*
	 * convert a page descriptor into its corresponding physical address
	 * in place to reduce memory allocation
	 */
	phys = (unsigned long *)pages;

	ret = sg_alloc_table(&info->table, count, GFP_KERNEL);
	if (ret)
		goto err_sg;

	ret = alloc_pages_highorder(ION_HPA_DEFAULT_ORDER, pages, count);
	if (ret)
		goto err_hpa;

	sort(pages, count, sizeof(*pages), ion_hpa_compare_pages, NULL);

	for_each_sg(info->table.sgl, sg, info->table.orig_nents, i) {
		if (memzero)
			memset(page_address(pages[i]), 0, ION_HPA_DEFAULT_SIZE);
		if (cacheflush)
			__flush_dcache_area(page_address(pages[i]),
					    ION_HPA_DEFAULT_SIZE);
		sg_set_page(sg, pages[i], ION_HPA_DEFAULT_SIZE, 0);
		phys[i] = page_to_phys(pages[i]);
	}

	buffer->priv_virt = info;
	buffer->sg_table = &info->table;

	if (protected) {
		info->prot_desc.chunk_count = count;
		info->prot_desc.flags = heap->id;
		info->prot_desc.chunk_size = ION_HPA_DEFAULT_SIZE;
		info->prot_desc.bus_address = (count == 1) ?  phys[0] :
						virt_to_phys(phys);
		ion_secure_protect(buffer);

		if (count > 1)
			kmemleak_ignore(pages);
		else
			kfree(pages);
	} else {
		kfree(pages);
	}

	return 0;
err_hpa:
	sg_free_table(&info->table);
err_sg:
	kfree(pages);
err_pages:
	kfree(info);

	return ret;
}

static void ion_hpa_heap_free(struct ion_buffer *buffer)
{
	struct ion_buffer_info *info = buffer->priv_virt;
	struct scatterlist *sg;
	unsigned int i;

	if (buffer->flags & ION_FLAG_PROTECTED) {
		ion_secure_unprotect(buffer);

		if (info->prot_desc.chunk_count > 1)
			kfree(phys_to_virt(info->prot_desc.bus_address));
	}

	for_each_sg(info->table.sgl, sg, info->table.orig_nents, i)
		__free_pages(sg_page(sg), ION_HPA_DEFAULT_ORDER);

	sg_free_table(&info->table);
	kfree(info);
}

static struct ion_heap_ops ion_hpa_ops = {
	.allocate = ion_hpa_heap_allocate,
	.free = ion_hpa_heap_free,
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
};

struct ion_heap *ion_hpa_heap_create(struct ion_platform_heap *data)
{
	struct ion_heap *heap;

	heap = kzalloc(sizeof(*heap), GFP_KERNEL);
	if (!heap)
		return ERR_PTR(-ENOMEM);

	heap->ops = &ion_hpa_ops;
	heap->type = ION_HEAP_TYPE_HPA;
	pr_info("%s: HPA heap %s(%d) is created\n", __func__,
			data->name, data->id);
	return heap;
}

void ion_hpa_heap_destroy(struct ion_heap *heap)
{
	kfree(heap);
}
