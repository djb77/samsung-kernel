#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/exynos_ion.h>

#include "dmabuf_container_priv.h"
#include "../ion.h"
#include "../ion_priv.h"

static void __exynos_sync_sg_for_device(struct device *dev, size_t size,
					 struct scatterlist *sgl, int nelems,
					 enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl, sg, nelems, i) {
		size_t sg_len = min(size, (size_t)sg->length);

		__dma_map_area(phys_to_virt(dma_to_phys(dev, sg->dma_address)),
			       sg_len, dir);
		if (size > sg->length)
			size -= sg->length;
		else
			break;
	}
}

static void __exynos_sync_sg_for_cpu(struct device *dev, size_t size,
				      struct scatterlist *sgl, int nelems,
				      enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl, sg, nelems, i) {
		size_t sg_len = min(size, (size_t)sg->length);

		__dma_unmap_area(phys_to_virt(dma_to_phys(dev, sg->dma_address)),
				 sg_len, dir);
		if (size > sg->length)
			size -= sg->length;
		else
			break;
	}
}

static void exynos_flush_sg(struct device *dev, size_t size,
			    struct scatterlist *sgl, int nelems)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl, sg, nelems, i) {
		size_t sg_len = min_t(size_t, size, sg->length);
		void *virt = phys_to_virt(dma_to_phys(dev, sg->dma_address));

		__dma_flush_area(virt, sg_len);

		if (size > sg->length)
			size -= sg->length;
		else
			break;
	}
}

#define exynos_sync_single_for_device(addr, size, dir)	__dma_map_area(addr, size, dir)
#define exynos_sync_single_for_cpu(addr, size, dir)	__dma_unmap_area(addr, size, dir)
#define exynos_sync_sg_for_device(dev, size, sg, nents, dir)	\
	__exynos_sync_sg_for_device(dev, size, sg, nents, dir)
#define exynos_sync_sg_for_cpu(dev, size, sg, nents, dir)	\
	__exynos_sync_sg_for_cpu(dev, size, sg, nents, dir)

void exynos_ion_flush_dmabuf_for_device(struct device *dev,
					struct dma_buf *dmabuf, size_t size)
{
	struct ion_buffer *buffer = (struct ion_buffer *)dmabuf->priv;

	if (is_dmabuf_container(dmabuf))
		return;

	if (!ion_buffer_cached(buffer) ||
		ion_buffer_fault_user_mappings(buffer))
		return;

	mutex_lock(&buffer->lock);

	pr_debug("%s: flushing for device %s, buffer: %p, size: %zd\n",
		 __func__, dev ? dev_name(dev) : "null", buffer, size);

	trace_ion_sync_start(_RET_IP_, dev, DMA_BIDIRECTIONAL, size,
			     buffer->vaddr, 0, size >= ION_FLUSH_ALL_HIGHLIMIT);

	exynos_flush_sg(dev, size, buffer->sg_table->sgl,
			buffer->sg_table->nents);

	trace_ion_sync_end(_RET_IP_, dev, DMA_BIDIRECTIONAL, size,
			   buffer->vaddr, 0, size >= ION_FLUSH_ALL_HIGHLIMIT);

	mutex_unlock(&buffer->lock);
}
EXPORT_SYMBOL(exynos_ion_flush_dmabuf_for_device);

void exynos_ion_sync_dmabuf_for_device(struct device *dev,
					struct dma_buf *dmabuf,
					size_t size,
					enum dma_data_direction dir)
{
	struct ion_buffer *buffer = (struct ion_buffer *) dmabuf->priv;

	if (IS_ERR_OR_NULL(buffer))
		BUG();

	if (!ion_buffer_cached(buffer) ||
			ion_buffer_fault_user_mappings(buffer))
		return;

	mutex_lock(&buffer->lock);

	pr_debug("%s: syncing for device %s, buffer: %p, size: %zd\n",
			__func__, dev ? dev_name(dev) : "null", buffer, size);

	trace_ion_sync_start(_RET_IP_, dev, dir, size,
			buffer->vaddr, 0, size >= ION_FLUSH_ALL_HIGHLIMIT);

	if (!IS_ERR_OR_NULL(buffer->vaddr))
		exynos_sync_single_for_device(buffer->vaddr, size, dir);
	else
		exynos_sync_sg_for_device(dev, size, buffer->sg_table->sgl,
						buffer->sg_table->nents, dir);

	trace_ion_sync_end(_RET_IP_, dev, dir, size,
			buffer->vaddr, 0, size >= ION_FLUSH_ALL_HIGHLIMIT);

	mutex_unlock(&buffer->lock);
}
EXPORT_SYMBOL(exynos_ion_sync_dmabuf_for_device);

void exynos_ion_sync_vaddr_for_device(struct device *dev,
					struct dma_buf *dmabuf,
					void *vaddr,
					size_t size,
					off_t offset,
					enum dma_data_direction dir)
{
	struct ion_buffer *buffer = (struct ion_buffer *)dmabuf->priv;

	if (is_dmabuf_container(dmabuf))
		return;

	if (!ion_buffer_cached(buffer) ||
		ion_buffer_fault_user_mappings(buffer))
		return;

	pr_debug("%s: syncing for device %s, vaddr: %p, size: %zd, offset: %ld\n",
			__func__, dev ? dev_name(dev) : "null",
			vaddr, size, offset);

	trace_ion_sync_start(_RET_IP_, dev, dir, size,
			vaddr, offset, size >= ION_FLUSH_ALL_HIGHLIMIT);

	if (!IS_ERR_OR_NULL(vaddr))
		exynos_sync_single_for_device(vaddr + offset, size, dir);
	else
		BUG();

	trace_ion_sync_end(_RET_IP_, dev, dir, size,
			vaddr, offset, size >= ION_FLUSH_ALL_HIGHLIMIT);
}
EXPORT_SYMBOL(exynos_ion_sync_vaddr_for_device);

void exynos_ion_sync_dmabuf_for_cpu(struct device *dev,
					struct dma_buf *dmabuf,
					size_t size,
					enum dma_data_direction dir)
{
	struct ion_buffer *buffer = (struct ion_buffer *) dmabuf->priv;

	if (is_dmabuf_container(dmabuf))
		return;

	if (dir == DMA_TO_DEVICE)
		return;

	if (IS_ERR_OR_NULL(buffer))
		BUG();

	if (!ion_buffer_cached(buffer) ||
			ion_buffer_fault_user_mappings(buffer))
		return;

	mutex_lock(&buffer->lock);

	pr_debug("%s: syncing for cpu %s, buffer: %p, size: %zd\n",
			__func__, dev ? dev_name(dev) : "null", buffer, size);

	trace_ion_sync_start(_RET_IP_, dev, dir, size,
			buffer->vaddr, 0, size >= ION_FLUSH_ALL_HIGHLIMIT);

	if (!IS_ERR_OR_NULL(buffer->vaddr))
		exynos_sync_single_for_cpu(buffer->vaddr, size, dir);
	else
		exynos_sync_sg_for_cpu(dev, size, buffer->sg_table->sgl,
						buffer->sg_table->nents, dir);

	trace_ion_sync_end(_RET_IP_, dev, dir, size,
			buffer->vaddr, 0, size >= ION_FLUSH_ALL_HIGHLIMIT);

	mutex_unlock(&buffer->lock);
}
EXPORT_SYMBOL(exynos_ion_sync_dmabuf_for_cpu);

void exynos_ion_sync_vaddr_for_cpu(struct device *dev,
					struct dma_buf *dmabuf,
					void *vaddr,
					size_t size,
					off_t offset,
					enum dma_data_direction dir)
{
	struct ion_buffer *buffer = (struct ion_buffer *)dmabuf->priv;

	if (is_dmabuf_container(dmabuf))
		return;

	if (dir == DMA_TO_DEVICE)
		return;

	if (!ion_buffer_cached(buffer) ||
		ion_buffer_fault_user_mappings(buffer))
		return;

	pr_debug("%s: syncing for cpu %s, vaddr: %p, size: %zd, offset: %ld\n",
			__func__, dev ? dev_name(dev) : "null",
			vaddr, size, offset);

	trace_ion_sync_start(_RET_IP_, dev, dir, size,
			vaddr, offset, size >= ION_FLUSH_ALL_HIGHLIMIT);

	if (!IS_ERR_OR_NULL(vaddr))
		exynos_sync_single_for_cpu(vaddr + offset, size, dir);
	else
		BUG();

	trace_ion_sync_end(_RET_IP_, dev, dir, size,
			vaddr, offset, size >= ION_FLUSH_ALL_HIGHLIMIT);
}
EXPORT_SYMBOL(exynos_ion_sync_vaddr_for_cpu);
