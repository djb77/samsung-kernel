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

struct ion_hpa_heap {
	struct ion_heap heap;
	unsigned long unprot_count;
	unsigned long unprot_size;
};

#define to_hpa_heap(x) container_of(x, struct ion_hpa_heap, heap)

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
		ret = ion_secure_protect(buffer);
		if (ret)
			goto err_prot;

		if (count > 1)
			kmemleak_ignore(pages);
		else
			kfree(pages);
	} else {
		kfree(pages);
	}

	return 0;
err_prot:
	for (i = 0; i < count; i++)
		__free_pages(phys_to_page(phys[i]), ION_HPA_DEFAULT_ORDER);

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
	int unprot_err = 0;

	if (buffer->flags & ION_FLAG_PROTECTED) {
		unprot_err = ion_secure_unprotect(buffer);

		if (info->prot_desc.chunk_count > 1)
			kfree(phys_to_virt(info->prot_desc.bus_address));
	}

	if (unprot_err) {
		struct ion_hpa_heap *hpa_heap = to_hpa_heap(buffer->heap);

		hpa_heap->unprot_count++;
		hpa_heap->unprot_size += ALIGN(buffer->size,
					       ION_PROTECTED_BUF_ALIGN);
	} else {
		unsigned int i;
		/*
		 * leave the pages allocated if unprotection to that pages fail.
		 * Otherwise, access in Linux to that pages may cause system
		 * error.
		 */
		for_each_sg(info->table.sgl, sg, info->table.orig_nents, i)
			__free_pages(sg_page(sg), ION_HPA_DEFAULT_ORDER);
	}

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

static int hpa_heap_debug_show(struct ion_heap *heap, struct seq_file *s,
			       void *unused)
{
	struct ion_hpa_heap *hpa_heap = to_hpa_heap(heap);

	seq_printf(s, "\n[%s] unprotect error: count %lu, size %lu\n",
		   heap->name, hpa_heap->unprot_count, hpa_heap->unprot_size);

	return 0;
}

struct ion_heap *ion_hpa_heap_create(struct ion_platform_heap *data)
{
	struct ion_hpa_heap *heap;

	heap = kzalloc(sizeof(*heap), GFP_KERNEL);
	if (!heap)
		return ERR_PTR(-ENOMEM);

	heap->heap.ops = &ion_hpa_ops;
	heap->heap.type = ION_HEAP_TYPE_HPA;
	heap->heap.debug_show = hpa_heap_debug_show;
	pr_info("%s: HPA heap %s(%d) is created\n", __func__,
			data->name, data->id);
	return &heap->heap;
}

void ion_hpa_heap_destroy(struct ion_heap *heap)
{
	struct ion_hpa_heap *hpa = to_hpa_heap(heap);

	kfree(hpa);
}
