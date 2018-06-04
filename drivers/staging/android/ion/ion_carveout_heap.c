/*
 * drivers/staging/android/ion/ion_carveout_heap.c
 *
 * Copyright (C) 2011 Google, Inc.
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
#include <linux/spinlock.h>
#include <linux/bitmap.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/ion.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/exynos_ion.h>
#include "ion_priv.h"

#define MAX_CARVEOUT_ALIGNMENT	12

extern struct ion_device *g_idev;

struct ion_carveout_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	ion_phys_addr_t base;
};

static unsigned long find_first_fit_with_align(unsigned long *map,
				unsigned long size, unsigned long start,
				unsigned int nr, void *data)
{
	unsigned long align = ((*(unsigned long *)data) >> PAGE_SHIFT);

	if (align > (1 << MAX_CARVEOUT_ALIGNMENT))
		align = (1 << MAX_CARVEOUT_ALIGNMENT);

	return bitmap_find_next_zero_area(map, size, start, nr, (align - 1));
}

ion_phys_addr_t ion_carveout_allocate(struct ion_heap *heap,
				      unsigned long size,
				      unsigned long align)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);
	unsigned long offset;

	if (align > PAGE_SIZE) {
		gen_pool_set_algo(carveout_heap->pool,
				find_first_fit_with_align, &align);
		offset = gen_pool_alloc(carveout_heap->pool, size);
		gen_pool_set_algo(carveout_heap->pool, NULL, NULL);
	} else {
		offset = gen_pool_alloc(carveout_heap->pool, size);
	}

	if (!offset)
		return ION_CARVEOUT_ALLOCATE_FAIL;

	return offset;
}

void ion_carveout_free(struct ion_heap *heap, ion_phys_addr_t addr,
		       unsigned long size)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);

	if (addr == ION_CARVEOUT_ALLOCATE_FAIL)
		return;
	gen_pool_free(carveout_heap->pool, addr, size);
}

static int ion_carveout_heap_phys(struct ion_heap *heap,
				  struct ion_buffer *buffer,
				  ion_phys_addr_t *addr, size_t *len)
{
	struct ion_buffer_info *info = buffer->priv_virt;

	*addr = info->handle;
	*len = buffer->size;
	return 0;
}

static int ion_carveout_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size, unsigned long align,
				      unsigned long flags)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);
	struct ion_buffer_info *info;
	ion_phys_addr_t paddr;
	unsigned long len = size;
	int ret;

	if ((align > PAGE_SIZE) && (align & (align - 1)))
		return -EINVAL;

	if (!ion_is_heap_available(heap, flags, carveout_heap->pool))
		return -EPERM;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	ret = sg_alloc_table(&info->table, 1, GFP_KERNEL);
	if (ret)
		goto err_free;

	if (buffer->flags & ION_FLAG_PROTECTED) {
		align = ION_PROTECTED_BUF_ALIGN;
		len = ALIGN(size, ION_PROTECTED_BUF_ALIGN);
	}

	paddr = ion_carveout_allocate(heap, len, align);
	if (paddr == ION_CARVEOUT_ALLOCATE_FAIL) {
		ret = -ENOMEM;
		goto err_free_table;
	}

	info->handle = paddr;
	sg_set_page(info->table.sgl, pfn_to_page(PFN_DOWN(paddr)), size, 0);
	buffer->priv_virt = info;

	if (buffer->flags & ION_FLAG_PROTECTED) {
		info->prot_desc.chunk_count = 1;
		info->prot_desc.flags = heap->id;
		info->prot_desc.chunk_size = len;
		info->prot_desc.bus_address = paddr;
		ret = ion_secure_protect(buffer);
		if (ret) {
			pr_err("%s: Failed to protect buffer of %zu bytes\n",
			       __func__, size);
			goto err_protect;
		}
	}

	return 0;
err_protect:
	ion_carveout_free(heap, paddr, len);
err_free_table:
	sg_free_table(&info->table);
err_free:
	kfree(info);
	ion_debug_heap_usage_show(heap);
	return ret;
}

static void ion_carveout_heap_free(struct ion_buffer *buffer)
{
	struct ion_buffer_info *info = buffer->priv_virt;
	unsigned long len = buffer->size;

	if (!(buffer->flags & ION_FLAG_PROTECTED)) {
		void *va = page_address(sg_page(info->table.sgl));

		if (ion_buffer_cached(buffer)) {
			memset(va, 0, buffer->size);
			if (ion_buffer_need_flush_all(buffer))
				flush_all_cpu_caches();
			else
				__flush_dcache_area(va, buffer->size);
		} else {
			ion_heap_buffer_zero(buffer);
		}
	} else {
		ion_secure_unprotect(buffer);
		len = ALIGN(len, ION_PROTECTED_BUF_ALIGN);
	}

	ion_carveout_free(buffer->heap, (ion_phys_addr_t)info->handle, len);
	sg_free_table(&info->table);
	kfree(info);
}

static struct sg_table *ion_carveout_heap_map_dma(struct ion_heap *heap,
						  struct ion_buffer *buffer)
{
	struct ion_buffer_info *info = buffer->priv_virt;

	return &info->table;
}

static void ion_carveout_heap_unmap_dma(struct ion_heap *heap,
					struct ion_buffer *buffer)
{
}

static int carveout_heap_debug_show(struct ion_heap *heap,
					struct seq_file *s,
					void *unused)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);

	seq_puts(s, "\ncarveout heap allocations\n");
	seq_printf(s, "%11.s %10.s %10.s\n", "name", "size", "free");
	seq_puts(s, "----------------------------------------------------\n");
	seq_printf(s, "%11.s %#10.zx %#10.zx\n",
			heap->name, gen_pool_size(carveout_heap->pool),
			gen_pool_avail(carveout_heap->pool));

	return 0;
}

static struct ion_heap_ops carveout_heap_ops = {
	.allocate = ion_carveout_heap_allocate,
	.free = ion_carveout_heap_free,
	.phys = ion_carveout_heap_phys,
	.map_dma = ion_carveout_heap_map_dma,
	.unmap_dma = ion_carveout_heap_unmap_dma,
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
};

struct ion_heap *ion_carveout_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_carveout_heap *carveout_heap;

	struct page *page;
	size_t size;

	page = pfn_to_page(PFN_DOWN(heap_data->base));
	size = heap_data->size;

	carveout_heap = kzalloc(sizeof(struct ion_carveout_heap), GFP_KERNEL);
	if (!carveout_heap)
		return ERR_PTR(-ENOMEM);

	carveout_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!carveout_heap->pool) {
		kfree(carveout_heap);
		return ERR_PTR(-ENOMEM);
	}
	carveout_heap->base = heap_data->base;
	gen_pool_add(carveout_heap->pool, carveout_heap->base, heap_data->size,
		     -1);
	carveout_heap->heap.ops = &carveout_heap_ops;
	carveout_heap->heap.type = ION_HEAP_TYPE_CARVEOUT;
	carveout_heap->heap.debug_show = carveout_heap_debug_show;

	if (size >= ION_FLUSH_ALL_HIGHLIMIT)
		flush_all_cpu_caches();
	else
		__flush_dcache_area(page_address(page), size);
	return &carveout_heap->heap;
}

void ion_carveout_heap_destroy(struct ion_heap *heap)
{
	struct ion_carveout_heap *carveout_heap =
	     container_of(heap, struct  ion_carveout_heap, heap);

	gen_pool_destroy(carveout_heap->pool);
	kfree(carveout_heap);
	carveout_heap = NULL;
}
