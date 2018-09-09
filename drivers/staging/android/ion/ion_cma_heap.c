/*
 * drivers/staging/android/ion/ion_cma_heap.c
 *
 * Copyright (C) Linaro 2012
 * Author: <benjamin.gaignard@linaro.org> for ST-Ericsson.
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

#include <linux/device.h>
#include <linux/ion.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/exynos_ion.h>
#include <linux/cma.h>

/* for ion_heap_ops structure */
#include "ion_priv.h"

struct ion_cma_heap {
	struct ion_heap heap;
	struct cma *cma;
};

#define to_cma_heap(x) container_of(x, struct ion_cma_heap, heap)

/* ION CMA heap operations functions */
static int ion_cma_allocate(struct ion_heap *heap, struct ion_buffer *buffer,
			    unsigned long len, unsigned long align,
			    unsigned long flags)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(heap);
	struct ion_buffer_info *info;
	struct page *page;
	unsigned long size = len;
	int ret;

	if (!ion_is_heap_available(heap, flags, NULL))
		return -EPERM;

	info = kzalloc(sizeof(struct ion_buffer_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	if (buffer->flags & ION_FLAG_PROTECTED) {
		align = ION_PROTECTED_BUF_ALIGN;
		size = ALIGN(len, ION_PROTECTED_BUF_ALIGN);
	}

	page = cma_alloc(cma_heap->cma, (PAGE_ALIGN(size) >> PAGE_SHIFT),
			 (align ? get_order(align) : 0));
	if (!page) {
		ret = -ENOMEM;
		pr_err("%s: Fail to allocate buffer from CMA\n", __func__);
		goto err;
	}

	if (!(flags & ION_FLAG_NOZEROED))
		memset(page_address(page), 0, len);

	ret = sg_alloc_table(&info->table, 1, GFP_KERNEL);
	if (ret)
		goto free_mem;

	sg_set_page(info->table.sgl, page, size, 0);

	if (!ion_buffer_cached(buffer) && !(buffer->flags & ION_FLAG_PROTECTED))
		__flush_dcache_area(page_address(sg_page(info->table.sgl)),
									len);

	/* keep this for memory release */
	buffer->priv_virt = info;
	buffer->sg_table = &info->table;

	if (buffer->flags & ION_FLAG_PROTECTED) {
		info->prot_desc.chunk_count = 1;
		info->prot_desc.flags = heap->id;
		info->prot_desc.chunk_size = (unsigned int)size;
		info->prot_desc.bus_address = page_to_phys(page);
		ret = ion_secure_protect(buffer);
		if (ret)
			goto err_free_table;
	}

	return 0;

err_free_table:
	sg_free_table(&info->table);
free_mem:
	cma_release(cma_heap->cma, page, (PAGE_ALIGN(size) >> PAGE_SHIFT));
err:
	kfree(info);
	ion_debug_heap_usage_show(heap);
	return ret;
}

static void ion_cma_free(struct ion_buffer *buffer)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(buffer->heap);
	struct ion_buffer_info *info = buffer->priv_virt;
	unsigned long size = buffer->size;

	if (buffer->flags & ION_FLAG_PROTECTED) {
		ion_secure_unprotect(buffer);
		size = ALIGN(size, ION_PROTECTED_BUF_ALIGN);
	}

	cma_release(cma_heap->cma, sg_page(buffer->sg_table->sgl),
		    (PAGE_ALIGN(size) >> PAGE_SHIFT));

	sg_free_table(buffer->sg_table);
	kfree(info);
}

static struct ion_heap_ops ion_cma_ops = {
	.allocate = ion_cma_allocate,
	.free = ion_cma_free,
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
};

struct ion_heap *ion_cma_heap_create(struct ion_platform_heap *data)
{
	struct ion_cma_heap *cma_heap;

	cma_heap = kzalloc(sizeof(struct ion_cma_heap), GFP_KERNEL);

	if (!cma_heap)
		return ERR_PTR(-ENOMEM);

	cma_heap->heap.ops = &ion_cma_ops;
	cma_heap->cma = data->priv;
	cma_heap->heap.type = ION_HEAP_TYPE_DMA;
	return &cma_heap->heap;
}

void ion_cma_heap_destroy(struct ion_heap *heap)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(heap);

	kfree(cma_heap);
}
