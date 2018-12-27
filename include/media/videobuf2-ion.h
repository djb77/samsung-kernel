/* include/media/videobuf2-ion.h
 *
 * Copyright 2011-2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Definition of Android ION memory allocator for videobuf2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MEDIA_VIDEOBUF2_ION_H
#define _MEDIA_VIDEOBUF2_ION_H

#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/ion.h>
#include <linux/exynos_ion.h>
#include <linux/err.h>
#include <linux/dma-buf.h>

/* flags to vb2_ion_create_context
 * These flags are dependet upon heap flags in ION.
 *
 * bit 0 ~ ION_NUM_HEAPS: ion heap flags
 * bit ION_NUM_HEAPS+1 ~ 20: non-ion flags (cached, iommu)
 * bit 21 ~ BITS_PER_INT - 1: ion specific flags
 */

/* DMA of the client device is coherent with CPU */
#define VB2ION_CTX_COHERENT_DMA	(1 << (ION_NUM_HEAPS + 3))
/* DMA should read from memory instead of CPU cache even if DMA is coherent */
#define VB2ION_CTX_UNCACHED_READ_DMA (1 << (ION_NUM_HEAPS + 4))

#define VB2ION_CONTIG_ID_NUM	16
#define VB2ION_NUM_HEAPS	8
/* below 6 is the above vb2-ion flags (ION_NUM_HEAPS + 1 ~ 6) */
#if (BITS_PER_INT <= (VB2ION_CONTIG_ID_NUM + VB2ION_NUM_HEAPS + 6))
#error "Bits are too small to express all flags"
#endif

struct device;
struct vb2_buffer;

/* - vb2_ion_set_noncoherent_dma_read(): Forces the DMA to read from system
 *   memory even though it is capable of snooping CPU caches.
 */
void vb2_ion_set_noncoherent_dma_read(struct device *dev, bool noncoherent);

/* Data type of the cookie returned by vb2_plane_cookie() function call.
 * The drivers do not need the definition of this structure. The only reason
 * why it is defined outside of videobuf2-ion.c is to make some functions
 * inline.
 */
struct vb2_ion_cookie {
	dma_addr_t ioaddr;
	dma_addr_t paddr;
	struct sg_table	*sgt;
	off_t offset;
};

/* vb2_ion_buffer_offset - return the mapped offset of the buffer
 * - cookie: pointer returned by vb2_plane_cookie()
 *
 * Returns offset value that the mapping starts from.
 */

static inline off_t vb2_ion_buffer_offset(void *cookie)
{
	return IS_ERR_OR_NULL(cookie) ?
		-EINVAL : ((struct vb2_ion_cookie *)cookie)->offset;
}

/* vb2_ion_phys_address - returns the physical address of the given buffer
 * - cookie: pointer returned by vb2_plane_cookie()
 * - phys_addr: pointer to the store of the physical address of the buffer
 *   specified by cookie.
 *
 * Returns -EINVAL if the buffer does not have nor physically contiguous memory.
 */
static inline int vb2_ion_phys_address(void *cookie, phys_addr_t *phys_addr)
{
	struct vb2_ion_cookie *vb2cookie = cookie;

	if (WARN_ON(!phys_addr || IS_ERR_OR_NULL(cookie)))
		return -EINVAL;

	if (vb2cookie->paddr) {
		*phys_addr = vb2cookie->paddr;
	} else {
		if (vb2cookie->sgt && vb2cookie->sgt->nents == 1) {
			*phys_addr = sg_phys(vb2cookie->sgt->sgl) +
						vb2cookie->offset;
		} else {
			*phys_addr = 0;
			return -EINVAL;
		}
	}

	return 0;
}

/* vb2_ion_dma_address - returns the DMA address that device can see
 * - cookie: pointer returned by vb2_plane_cookie()
 * - dma_addr: pointer to the store of the address of the buffer specified
 *   by cookie. It can be either IO virtual address or physical address
 *   depending on the specification of allocation context which allocated
 *   the buffer.
 *
 * Returns -EINVAL if the buffer has neither IO virtual address nor physically
 * contiguous memory
 */
static inline int vb2_ion_dma_address(void *cookie, dma_addr_t *dma_addr)
{
	struct vb2_ion_cookie *vb2cookie = cookie;

	if (WARN_ON(!dma_addr || IS_ERR_OR_NULL(cookie)))
		return -EINVAL;

	if (vb2cookie->ioaddr == 0)
		return vb2_ion_phys_address(cookie, (phys_addr_t *)dma_addr);

	*dma_addr = vb2cookie->ioaddr;

	return 0;
}

/* vb2_ion_get_sg - returns scatterlist of the given cookie.
 * - cookie: pointer returned by vb2_plane_cookie()
 * - nents: pointer to the store of number of elements in the returned
 *   scatterlist
 *
 * Returns the scatterlist of the buffer specified by cookie.
 * If the arguments are not correct, returns NULL.
 */
static inline struct scatterlist *vb2_ion_get_sg(void *cookie, int *nents)
{
	struct vb2_ion_cookie *vb2cookie = cookie;

	if (WARN_ON(!nents || IS_ERR_OR_NULL(cookie)))
		return NULL;

	*nents = vb2cookie->sgt->nents;
	return vb2cookie->sgt->sgl;
}

/* vb2_ion_get_dmabuf - returns dmabuf of the given cookie
 * - cookie: pointer returned by vb2_plane_cookie()
 *
 * Returns dma-buf descriptor with a reference held.
 * NULL if the cookie is invalid or the buffer is not dmabuf.
 * The caller must put the dmabuf when it is no longer need the dmabuf
 */
struct dma_buf *vb2_ion_get_dmabuf(void *cookie);

/***** Cache mainatenance operations *****/
void vb2_ion_sync_for_device(void *cookie, off_t offset, size_t size,
						enum dma_data_direction dir);
void vb2_ion_sync_for_cpu(void *cookie, off_t offset, size_t size,
						enum dma_data_direction dir);
int vb2_ion_buf_prepare(struct vb2_buffer *vb);
void vb2_ion_buf_finish(struct vb2_buffer *vb);
int vb2_ion_buf_prepare_exact(struct vb2_buffer *vb);
int vb2_ion_buf_finish_exact(struct vb2_buffer *vb);

extern const struct vb2_mem_ops vb2_ion_memops;
extern struct ion_device *ion_exynos; /* drivers/gpu/ion/exynos/exynos-ion.c */

#endif /* _MEDIA_VIDEOBUF2_ION_H */
