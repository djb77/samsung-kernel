/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_MEM_H__
#define __IVA_MEM_H__

#include "iva_ctrl.h"
#include "iva_ctrl_ioctl.h"

/* iva iova mapping flags */
#define IVA_ALLOC_CACHE_MASK		(0x3)
#define IVA_ALLOC_CACHE_SHIFT		(8)
#define GET_IVA_CACHE_TYPE(flag) \
		((flag >> IVA_ALLOC_CACHE_SHIFT) & IVA_ALLOC_CACHE_MASK)
#define SET_IVA_CACHE_TYPE(flag, a)	\
		(flag |= (a & IVA_ALLOC_CACHE_MASK) << IVA_ALLOC_CACHE_SHIFT)

#define IVA_FREE_REQ_MASK		(0x1)
#define IVA_FREE_REQ_SHIFT		(4)	/* free requested */
#define IVA_FREE_REQUESTED		(IVA_FREE_REQ_MASK << IVA_FREE_REQ_SHIFT)
#define FREE_REQUESTED(flag)		(flag & IVA_FREE_REQUESTED)

#define IVA_ALLOC_TYPE_MASK		(0x3)
#define IVA_ALLOC_TYPE_SHIFT		(0)
#define IVA_ALLOC_TYPE_IMPORTED		(0x2)	/* from outside */
#define IVA_ALLOC_TYPE_ALLOCATED	(0x1)	/* from inside */
#define IVA_ALLOC_TYPE_NOT_DEFINED	(0)	/* not allocated or freed */

#define GET_IVA_ALLOC_TYPE(flag) \
		((flag >> IVA_ALLOC_TYPE_SHIFT) & IVA_ALLOC_TYPE_MASK)
#define SET_IVA_ALLOC_TYPE(flag, a)	\
		(flag |= (a & IVA_ALLOC_TYPE_MASK) << IVA_ALLOC_TYPE_SHIFT)

#define ALLOC_INSIDE(flag)	(flag & IVA_ALLOC_TYPE_ALLOCATED)
#define ALLOC_OUTSIDE(flag)	(flag & IVA_ALLOC_TYPE_IMPORTED)


struct iva_mem_map {
	/* TO DO: ref */
	struct list_head	node;		/* global */
	struct hlist_node	h_node;		/* per process */
	int 			flags;
	struct kref 		map_ref;	/* iova mapping */
	struct device 		*dev;

	int			shared_fd;
	struct ion_handle       *handle;
	struct dma_buf		*dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table		*sg_table;
	unsigned long		io_va;
	uint32_t		req_size;	/* requested size */
	uint32_t		act_size;	/* actual_size */
};

extern int 	iva_mem_map_read_refcnt(struct iva_mem_map *iva_map);
extern void	iva_mem_show_global_mapped_list_nolock(struct iva_dev_data *iva);
extern void	iva_mem_force_to_free_proc_mapped_list(struct iva_proc *proc);
extern long	iva_mem_ioctl(struct iva_proc *proc,
			unsigned int cmd, void __user *p);

extern int	iva_mem_create_ion_client(struct iva_dev_data *iva);
extern int	iva_mem_destroy_ion_client(struct iva_dev_data *iva);

#endif /* __IVA_MBOX_H */
