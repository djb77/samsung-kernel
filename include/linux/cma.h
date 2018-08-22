#ifndef __CMA_H__
#define __CMA_H__

/*
 * There is always at least global CMA area and a few optional
 * areas configured in kernel .config.
 */
#ifdef CONFIG_CMA_AREAS
#define MAX_CMA_AREAS	(1 + CONFIG_CMA_AREAS)

#else
#define MAX_CMA_AREAS	(0)

#endif

struct cma;

extern unsigned long totalcma_pages;
extern phys_addr_t cma_get_base(const struct cma *cma);
extern unsigned long cma_get_size(const struct cma *cma);

extern int __init cma_declare_contiguous_with_name(phys_addr_t base,
			phys_addr_t size, phys_addr_t limit,
			phys_addr_t alignment, unsigned int order_per_bit,
			bool fixed, struct cma **res_cma, const char *name);
static inline int cma_declare_contiguous(phys_addr_t base,
			phys_addr_t size, phys_addr_t limit,
			phys_addr_t alignment, unsigned int order_per_bit,
			bool fixed, struct cma **res_cma)
{
	return cma_declare_contiguous_with_name(base, size, limit, alignment,
						order_per_bit, fixed, res_cma,
						NULL);
}
extern int cma_init_reserved_mem_with_name(phys_addr_t base, phys_addr_t size,
					unsigned int order_per_bit,
					struct cma **res_cma, const char *name);
static inline int cma_init_reserved_mem(phys_addr_t base, phys_addr_t size,
					unsigned int order_per_bit,
					struct cma **res_cma)
{
	return cma_init_reserved_mem_with_name(base, size, order_per_bit,
					       res_cma, NULL);
}
extern struct page *cma_alloc(struct cma *cma, size_t count, unsigned int align);
extern bool cma_release(struct cma *cma, const struct page *pages, unsigned int count);
#endif
