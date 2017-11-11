/*
 * Based on arch/arm/mm/mmu.c
 *
 * Copyright (C) 1995-2005 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/libfdt.h>
#include <linux/mman.h>
#include <linux/nodemask.h>
#include <linux/memblock.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/stop_machine.h>
#include <linux/vmalloc.h>

#include <asm/cputype.h>
#include <asm/fixmap.h>
#include <asm/kernel-pgtable.h>
#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/sizes.h>
#include <asm/tlb.h>
#include <asm/memblock.h>
#include <asm/mmu_context.h>
#include <asm/map.h>

#include "mm.h"

#include <linux/vmalloc.h>

#if defined(CONFIG_ECT)
#include <soc/samsung/ect_parser.h>
#endif

#ifdef CONFIG_TIMA_RKP
#include <linux/rkp_entry.h> 
#endif //CONFIG_TIMA_RKP

u64 idmap_t0sz = TCR_T0SZ(VA_BITS);
static int iotable_on;

/*
 * Empty_zero_page is a special page that is used for zero-initialized data
 * and COW.
 */
struct page *empty_zero_page;
EXPORT_SYMBOL(empty_zero_page);

pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
			      unsigned long size, pgprot_t vma_prot)
{
	if (!pfn_valid(pfn))
		return pgprot_noncached(vma_prot);
	else if (file->f_flags & O_SYNC)
		return pgprot_writecombine(vma_prot);
	return vma_prot;
}
EXPORT_SYMBOL(phys_mem_access_prot);

static void __init *early_alloc(unsigned long sz)
{
	phys_addr_t phys;
	void *ptr;

	phys = memblock_alloc(sz, sz);
	BUG_ON(!phys);
	ptr = __va(phys);
	memset(ptr, 0, sz);
	return ptr;
}

#ifdef CONFIG_TIMA_RKP
/* Extra memory needed by VMM */
void* vmm_extra_mem = 0;
spinlock_t ro_rkp_pages_lock = __SPIN_LOCK_UNLOCKED();
char ro_pages_stat[RO_PAGES] = {0};
unsigned ro_alloc_last = 0; 
int rkp_ro_mapped = 0; 


void *rkp_ro_alloc(void)
{
	unsigned long flags;
	int i, j;
	void * alloc_addr = NULL;
	
	spin_lock_irqsave(&ro_rkp_pages_lock,flags);
	
	for (i = 0, j = ro_alloc_last; i < (RO_PAGES) ; i++) {
		j =  (j+i) %(RO_PAGES); 
		if (!ro_pages_stat[j]) {
			ro_pages_stat[j] = 1;
			ro_alloc_last = j+1;
			alloc_addr = (void*) ((u64)RKP_RBUF_VA +  (j << PAGE_SHIFT));
			break;
		}
	}
	spin_unlock_irqrestore(&ro_rkp_pages_lock,flags);
	
	return alloc_addr;
}

void rkp_ro_free(void *free_addr)
{
	int i;
	unsigned long flags;

	i =  ((u64)free_addr - (u64)RKP_RBUF_VA) >> PAGE_SHIFT;
	spin_lock_irqsave(&ro_rkp_pages_lock,flags);
	ro_pages_stat[i] = 0;
	ro_alloc_last = i; 
	spin_unlock_irqrestore(&ro_rkp_pages_lock,flags);
}

unsigned int is_rkp_ro_page(u64 addr)
{
	if( (addr >= (u64)RKP_RBUF_VA)
		&& (addr < (u64)(RKP_RBUF_VA+ TIMA_ROBUF_SIZE)))
		return 1;
	else return 0;
}
/* we suppose the whole block remap to page table should start with block borders */
static inline void __init block_to_pages(pmd_t *pmd, unsigned long addr,
				  unsigned long end, unsigned long pfn)
{
	pte_t *old_pte;  
	pmd_t new ; 
	pte_t *pte = NULL;
	pte = rkp_ro_alloc();
	if (!pte)
		pte = (pte_t *)early_alloc(PAGE_SIZE);
	old_pte = pte;

	__pmd_populate(&new, __pa(pte), PMD_TYPE_TABLE); /* populate to temporary pmd */

	pte = pte_offset_kernel(&new, addr);

	do {		
		if (iotable_on == 1)
			set_pte(pte, pfn_pte(pfn, pgprot_iotable_init(PAGE_KERNEL_EXEC)));
		else
			set_pte(pte, pfn_pte(pfn, PAGE_KERNEL_EXEC));
		pfn++;
	} while (pte++, addr += PAGE_SIZE, addr != end);

	__pmd_populate(pmd, __pa(old_pte), PMD_TYPE_TABLE);
}
#endif

/*
 * remap a PMD into pages
 */
static void split_pmd(pmd_t *pmd, pte_t *pte)
{
	unsigned long pfn = pmd_pfn(*pmd);
	int i = 0;

	do {
		/*
		 * Need to have the least restrictive permissions available
		 * permissions will be fixed up later
		 */
		set_pte(pte, pfn_pte(pfn, PAGE_KERNEL_EXEC));
		pfn++;
	} while (pte++, i++, i < PTRS_PER_PTE);
}

static void alloc_init_pte(pmd_t *pmd, unsigned long addr,
				  unsigned long end, unsigned long pfn,
				  pgprot_t prot,
				  void *(*alloc)(unsigned long size))
{
	pte_t *pte;

#if defined(CONFIG_TIMA_RKP)
	if(pmd_block(*pmd))
		return block_to_pages(pmd, addr, end, pfn);
#endif
	if (pmd_none(*pmd) || pmd_sect(*pmd)) {
#if defined(CONFIG_TIMA_RKP)
#define FIMC_VA	 (0xffffff80fa000000ULL)
#define FIMC_SIZE	(0x780000)
	pte_t *rkp_ropage = NULL;
	if ((addr >= (FIMC_VA & PMD_MASK)
	        && addr < ((FIMC_VA + FIMC_SIZE + PMD_SIZE - 1) & PMD_MASK))
        || (addr >= (((u64) _text) & PMD_MASK)
	        && addr < ((((u64) __end_rodata) + PMD_SIZE - 1) & PMD_MASK))
	        )
	    rkp_ropage = (pte_t *) rkp_ro_alloc();
	if (rkp_ropage)
		pte = rkp_ropage;
	else 
	    pte = alloc(PTRS_PER_PTE * sizeof(pte_t));
#else	
		pte = alloc(PTRS_PER_PTE * sizeof(pte_t));
#endif		
		if (pmd_sect(*pmd))
			split_pmd(pmd, pte);
		__pmd_populate(pmd, __pa(pte), PMD_TYPE_TABLE);
		flush_tlb_all();
	}
#if !defined(CONFIG_TIMA_RKP)
	BUG_ON(pmd_bad(*pmd));
#endif

	pte = pte_offset_kernel(pmd, addr);
	do {
		if (iotable_on == 1)
			set_pte(pte, pfn_pte(pfn, pgprot_iotable_init(PAGE_KERNEL_EXEC)));
		else
			set_pte(pte, pfn_pte(pfn, prot));
		pfn++;
	} while (pte++, addr += PAGE_SIZE, addr != end);
}

static void split_pud(pud_t *old_pud, pmd_t *pmd)
{
	unsigned long addr = pud_pfn(*old_pud) << PAGE_SHIFT;
	pgprot_t prot = __pgprot(pud_val(*old_pud) ^ addr);
	int i = 0;

	do {
		set_pmd(pmd, __pmd(addr | pgprot_val(prot)));
		addr += PMD_SIZE;
	} while (pmd++, i++, i < PTRS_PER_PMD);
}

static void alloc_init_pmd(struct mm_struct *mm, pud_t *pud,
				  unsigned long addr, unsigned long end,
				  phys_addr_t phys, pgprot_t prot,
				  void *(*alloc)(unsigned long size))
{
	pmd_t *pmd;
	unsigned long next;

	/*
	 * Check for initial section mappings in the pgd/pud and remove them.
	 */
	if (pud_none(*pud) || pud_sect(*pud)) {
#ifdef CONFIG_TIMA_RKP
		pmd = rkp_ro_alloc();
	if (!pmd)
		pmd = alloc(PTRS_PER_PMD * sizeof(pmd_t));
#else	/* !CONFIG_TIMA_RKP */
		pmd = alloc(PTRS_PER_PMD * sizeof(pmd_t));
#endif
		if (pud_sect(*pud)) {
			/*
			 * need to have the 1G of mappings continue to be
			 * present
			 */
			split_pud(pud, pmd);
		}
		pud_populate(mm, pud, pmd);
		flush_tlb_all();
	}
	BUG_ON(pud_bad(*pud));

	pmd = pmd_offset(pud, addr);
	do {
		next = pmd_addr_end(addr, end);
		/* try section mapping first */
		if (((addr | next | phys) & ~SECTION_MASK) == 0) {
			pmd_t old_pmd =*pmd;
			set_pmd(pmd, __pmd(phys |
					   pgprot_val(mk_sect_prot(prot))));
			if (iotable_on == 1)
				set_pmd(pmd, __pmd(phys | PROT_SECT_NORMAL_NC));
			/*
			 * Check for previous table entries created during
			 * boot (__create_page_tables) and flush them.
			 */
			if (!pmd_none(old_pmd)) {
				flush_tlb_all();
				if (pmd_table(old_pmd)) {
					phys_addr_t table = __pa(pte_offset_map(&old_pmd, 0));
					if (!WARN_ON_ONCE(slab_is_available()))
						memblock_free(table, PAGE_SIZE);
				}
			}
		} else {
			alloc_init_pte(pmd, addr, next, __phys_to_pfn(phys),
				       prot, alloc);
		}
		phys += next - addr;
	} while (pmd++, addr = next, addr != end);
}

static inline bool use_1G_block(unsigned long addr, unsigned long next,
			unsigned long phys)
{
	if (PAGE_SHIFT != 12)
		return false;

	if (((addr | next | phys) & ~PUD_MASK) != 0)
		return false;

	return true;
}

static void alloc_init_pud(struct mm_struct *mm, pgd_t *pgd,
				  unsigned long addr, unsigned long end,
				  phys_addr_t phys, pgprot_t prot,
				  void *(*alloc)(unsigned long size))
{
	pud_t *pud;
	unsigned long next;

	if (pgd_none(*pgd)) {
		pud = alloc(PTRS_PER_PUD * sizeof(pud_t));
		pgd_populate(mm, pgd, pud);
	}
	BUG_ON(pgd_bad(*pgd));

	pud = pud_offset(pgd, addr);
	do {
		next = pud_addr_end(addr, end);

		/*
		 * For 4K granule only, attempt to put down a 1GB block
		 */
		if (use_1G_block(addr, next, phys)) {
			pud_t old_pud = *pud;
			set_pud(pud, __pud(phys |
					   pgprot_val(mk_sect_prot(prot))));

			/*
			 * If we have an old value for a pud, it will
			 * be pointing to a pmd table that we no longer
			 * need (from swapper_pg_dir).
			 *
			 * Look up the old pmd table and free it.
			 */
			if (!pud_none(old_pud)) {
				flush_tlb_all();
				if (pud_table(old_pud)) {
					phys_addr_t table = __pa(pmd_offset(&old_pud, 0));
					if (!WARN_ON_ONCE(slab_is_available())) {
#ifdef CONFIG_TIMA_RKP
						if ((u64) table < (u64) __pa(_text) || (u64) table > (u64) __pa(_etext))
							memblock_free(table, PAGE_SIZE);
#else
						memblock_free(table, PAGE_SIZE);
#endif
					}
				}
			}
		} else {
			alloc_init_pmd(mm, pud, addr, next, phys, prot, alloc);
		}
		phys += next - addr;
	} while (pud++, addr = next, addr != end);
}

/*
 * Create the page directory entries and any necessary page tables for the
 * mapping specified by 'md'.
 */
static void  __create_mapping(struct mm_struct *mm, pgd_t *pgd,
				    phys_addr_t phys, unsigned long virt,
				    phys_addr_t size, pgprot_t prot,
				    void *(*alloc)(unsigned long size))
{
	unsigned long addr, length, end, next;

	addr = virt & PAGE_MASK;
	length = PAGE_ALIGN(size + (virt & ~PAGE_MASK));

	end = addr + length;
	do {
		next = pgd_addr_end(addr, end);
		alloc_init_pud(mm, pgd, addr, next, phys, prot, alloc);
		phys += next - addr;
	} while (pgd++, addr = next, addr != end);
}

static void *late_alloc(unsigned long size)
{
	void *ptr;

	BUG_ON(size > PAGE_SIZE);
	ptr = (void *)__get_free_page(PGALLOC_GFP);
	BUG_ON(!ptr);
	return ptr;
}

static void __init create_mapping(phys_addr_t phys, unsigned long virt,
				  phys_addr_t size, pgprot_t prot)
{
	if (virt < VMALLOC_START) {
		pr_warn("BUG: not creating mapping for %pa at 0x%016lx - outside kernel range\n",
			&phys, virt);
		return;
	}
	__create_mapping(&init_mm, pgd_offset_k(virt & PAGE_MASK), phys, virt,
			 size, prot, early_alloc);
}

void __init create_pgd_mapping(struct mm_struct *mm, phys_addr_t phys,
			       unsigned long virt, phys_addr_t size,
			       pgprot_t prot)
{
	__create_mapping(mm, pgd_offset(mm, virt), phys, virt, size, prot,
				late_alloc);
}
#ifdef CONFIG_DEBUG_RODATA
static void create_mapping_late(phys_addr_t phys, unsigned long virt,
				  phys_addr_t size, pgprot_t prot)
{
	if (virt < VMALLOC_START) {
		pr_warn("BUG: not creating mapping for %pa at 0x%016lx - outside kernel range\n",
			&phys, virt);
		return;
	}

	return __create_mapping(&init_mm, pgd_offset_k(virt & PAGE_MASK),
				phys, virt, size, prot, late_alloc);
}

static void __init __map_memblock(phys_addr_t start, phys_addr_t end)
{
	/*
	 * Set up the executable regions using the existing section mappings
	 * for now. This will get more fine grained later once all memory
	 * is mapped
	 */
	unsigned long kernel_x_start = round_down(__pa(_stext), SWAPPER_BLOCK_SIZE);
	unsigned long kernel_x_end = round_up(__pa(__init_end), SWAPPER_BLOCK_SIZE);

	if (end < kernel_x_start) {
		create_mapping(start, __phys_to_virt(start),
			end - start, PAGE_KERNEL);
	} else if (start >= kernel_x_end) {
		create_mapping(start, __phys_to_virt(start),
			end - start, PAGE_KERNEL);
	} else {
		if (start < kernel_x_start)
			create_mapping(start, __phys_to_virt(start),
				kernel_x_start - start,
				PAGE_KERNEL);
		create_mapping(kernel_x_start,
				__phys_to_virt(kernel_x_start),
				kernel_x_end - kernel_x_start,
				PAGE_KERNEL_EXEC);
		if (kernel_x_end < end)
			create_mapping(kernel_x_end,
				__phys_to_virt(kernel_x_end),
				end - kernel_x_end,
				PAGE_KERNEL);
	}

}
#else
static void __init __map_memblock(phys_addr_t start, phys_addr_t end)
{
	create_mapping(start, __phys_to_virt(start), end - start,
			PAGE_KERNEL_EXEC);
}
#endif

static void __init map_mem(void)
{
	struct memblock_region *reg;
	phys_addr_t limit;
	phys_addr_t start;
	phys_addr_t end;

#ifdef CONFIG_TIMA_RKP
	phys_addr_t mid = 0xBF000000;
#endif
	/*
	 * Temporarily limit the memblock range. We need to do this as
	 * create_mapping requires puds, pmds and ptes to be allocated from
	 * memory addressable from the initial direct kernel mapping.
	 *
	 * The initial direct kernel mapping, located at swapper_pg_dir, gives
	 * us PUD_SIZE (with SECTION maps) or PMD_SIZE (without SECTION maps,
	 * memory starting from PHYS_OFFSET (which must be aligned to 2MB as
	 * per Documentation/arm64/booting.txt).
	 */
	limit = PHYS_OFFSET + SWAPPER_INIT_MAP_SIZE;
	memblock_set_current_limit(limit);

	/* map all the memory banks */
	for_each_memblock(memory, reg) {
		start = reg->base;
		end = start + reg->size;

		if (start >= end)
			break;

		if (ARM64_SWAPPER_USES_SECTION_MAPS) {
			/*
			 * For the first memory bank align the start address and
			 * current memblock limit to prevent create_mapping() from
			 * allocating pte page tables from unmapped memory. With
			 * the section maps, if the first block doesn't end on section
			 * size boundary, create_mapping() will try to allocate a pte
			 * page, which may be returned from an unmapped area.
			 * When section maps are not used, the pte page table for the
			 * current limit is already present in swapper_pg_dir.
			 */
			if (start < limit)
				start = ALIGN(start, SECTION_SIZE);
			if (end < limit) {
				limit = end & SECTION_MASK;
				memblock_set_current_limit(limit);
			}
		}
#ifdef CONFIG_TIMA_RKP
		/* mid should be selected such that start to mid includes the
		 * entire RKP RO buffer so that it can be memset safely.
		 * Memset should be done only once, obviously, hence the condition.
		 */
		if (((u64)start < TIMA_ROBUF_START) && ((u64)end > TIMA_ROBUF_START)) {
			__map_memblock(start, mid);
			memset(RKP_RBUF_VA, 0, TIMA_ROBUF_SIZE);
			__map_memblock(mid, end);
		} else {
			__map_memblock(start, end);
		}
#else /* !CONFIG_TIMA_RKP */
		__map_memblock(start, end);
#endif
	}
#ifdef CONFIG_TIMA_RKP
	vmm_extra_mem = early_alloc(0x600000);
	if ((u64) _text & (~PMD_MASK)) {
		start = (phys_addr_t) __pa(_text) & PMD_MASK;
		end = (phys_addr_t) __pa(_text);
		__map_memblock(start, end);
		start = (phys_addr_t) __pa(_text);
		end = (phys_addr_t) ALIGN(__pa(_text), PMD_SIZE);
		__map_memblock(start, end);
	}
	if ((u64) _etext & (~PMD_MASK)) {
		start = (phys_addr_t) __pa(_etext) & PMD_MASK;
		end = (phys_addr_t) __pa(_etext);
		__map_memblock(start, end);
		start = (phys_addr_t) __pa(_etext);
		end = (phys_addr_t) ALIGN(__pa(_etext), PMD_SIZE);
		__map_memblock(start, end);
	}
#endif
	/* Limit no longer required. */
	memblock_set_current_limit(MEMBLOCK_ALLOC_ANYWHERE);
}

static void __init fixup_executable(void)
{
#ifdef CONFIG_DEBUG_RODATA
	/* now that we are actually fully mapped, make the start/end more fine grained */
	if (!IS_ALIGNED((unsigned long)_stext, SWAPPER_BLOCK_SIZE)) {
		unsigned long aligned_start = round_down(__pa(_stext),
							 SWAPPER_BLOCK_SIZE);

		create_mapping(aligned_start, __phys_to_virt(aligned_start),
				__pa(_stext) - aligned_start,
				PAGE_KERNEL);
	}

	if (!IS_ALIGNED((unsigned long)__init_end, SWAPPER_BLOCK_SIZE)) {
		unsigned long aligned_end = round_up(__pa(__init_end),
							  SWAPPER_BLOCK_SIZE);
		create_mapping(__pa(__init_end), (unsigned long)__init_end,
				aligned_end - __pa(__init_end),
				PAGE_KERNEL);
	}
#endif
}

#ifdef CONFIG_DEBUG_RODATA
void mark_rodata_ro(void)
{
	create_mapping_late(__pa(_stext), (unsigned long)_stext,
				(unsigned long)_etext - (unsigned long)_stext,
				PAGE_KERNEL_ROX);

}

void fixup_init(void)
{
	create_mapping_late(__pa(__init_begin), (unsigned long)__init_begin,
			(unsigned long)__init_end - (unsigned long)__init_begin,
			PAGE_KERNEL);
}
#endif
/*
 * paging_init() sets up the page tables, initialises the zone memory
 * maps and sets up the zero page.
 */
void __init paging_init(void)
{
	void *zero_page;

	map_mem();
#if defined(CONFIG_ECT)
	ect_init_map_io();
#endif
	fixup_executable();

	/* allocate the zero page. */
#ifdef CONFIG_TIMA_RKP
	zero_page = rkp_ro_alloc();
	if (!zero_page)
		zero_page = early_alloc(PAGE_SIZE);
#else	/* !CONFIG_TIMA_RKP */
	zero_page = early_alloc(PAGE_SIZE);
#endif

	bootmem_init();

	empty_zero_page = virt_to_page(zero_page);

	/* Ensure the zero page is visible to the page table walker */
	dsb(ishst);

	/*
	 * TTBR0 is only used for the identity mapping at this stage. Make it
	 * point to zero page to avoid speculatively fetching new entries.
	 */
	cpu_set_reserved_ttbr0();
	local_flush_tlb_all();
	cpu_set_default_tcr_t0sz();
}

/*
 * Check whether a kernel address is valid (derived from arch/x86/).
 */
int kern_addr_valid(unsigned long addr)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	if ((((long)addr) >> VA_BITS) != -1UL)
		return 0;

	pgd = pgd_offset_k(addr);
	if (pgd_none(*pgd))
		return 0;

	pud = pud_offset(pgd, addr);
	if (pud_none(*pud))
		return 0;

	if (pud_sect(*pud))
		return pfn_valid(pud_pfn(*pud));

	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd))
		return 0;

	if (pmd_sect(*pmd))
		return pfn_valid(pmd_pfn(*pmd));

	pte = pte_offset_kernel(pmd, addr);
	if (pte_none(*pte))
		return 0;

	return pfn_valid(pte_pfn(*pte));
}
#ifdef CONFIG_SPARSEMEM_VMEMMAP
#if !ARM64_SWAPPER_USES_SECTION_MAPS
int __meminit vmemmap_populate(unsigned long start, unsigned long end, int node)
{
	return vmemmap_populate_basepages(start, end, node);
}
#else	/* !ARM64_SWAPPER_USES_SECTION_MAPS */
int __meminit vmemmap_populate(unsigned long start, unsigned long end, int node)
{
	unsigned long addr = start;
	unsigned long next;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;

	do {
		next = pmd_addr_end(addr, end);

		pgd = vmemmap_pgd_populate(addr, node);
		if (!pgd)
			return -ENOMEM;

		pud = vmemmap_pud_populate(pgd, addr, node);
		if (!pud)
			return -ENOMEM;

		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd)) {
			void *p = NULL;

			p = vmemmap_alloc_block_buf(PMD_SIZE, node);
			if (!p)
				return -ENOMEM;

			set_pmd(pmd, __pmd(__pa(p) | PROT_SECT_NORMAL));
		} else
			vmemmap_verify((pte_t *)pmd, node, addr, next);
	} while (addr = next, addr != end);

	return 0;
}
#endif	/* CONFIG_ARM64_64K_PAGES */
void vmemmap_free(unsigned long start, unsigned long end)
{
}
#endif	/* CONFIG_SPARSEMEM_VMEMMAP */

#ifdef CONFIG_TIMA_RKP
extern pte_t bm_pte[];
extern pmd_t bm_pmd[];
#else
static pte_t bm_pte[PTRS_PER_PTE] __page_aligned_bss;
#if CONFIG_PGTABLE_LEVELS > 2
static pmd_t bm_pmd[PTRS_PER_PMD] __page_aligned_bss;
#endif
#if CONFIG_PGTABLE_LEVELS > 3
static pud_t bm_pud[PTRS_PER_PUD] __page_aligned_bss;
#endif
#endif	/* CONFIG_TIMA_RKP */

static inline pud_t * fixmap_pud(unsigned long addr)
{
	pgd_t *pgd = pgd_offset_k(addr);

	BUG_ON(pgd_none(*pgd) || pgd_bad(*pgd));

	return pud_offset(pgd, addr);
}

static inline pmd_t * fixmap_pmd(unsigned long addr)
{
	pud_t *pud = fixmap_pud(addr);

	BUG_ON(pud_none(*pud) || pud_bad(*pud));

	return pmd_offset(pud, addr);
}

static inline pte_t * fixmap_pte(unsigned long addr)
{
	pmd_t *pmd = fixmap_pmd(addr);

	BUG_ON(pmd_none(*pmd) || pmd_bad(*pmd));

	return pte_offset_kernel(pmd, addr);
}

void __init early_fixmap_init(void)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	unsigned long addr = FIXADDR_START;

	pgd = pgd_offset_k(addr);
	pgd_populate(&init_mm, pgd, bm_pud);
	pud = pud_offset(pgd, addr);
	pud_populate(&init_mm, pud, bm_pmd);
	pmd = pmd_offset(pud, addr);
	pmd_populate_kernel(&init_mm, pmd, bm_pte);

	/*
	 * The boot-ioremap range spans multiple pmds, for which
	 * we are not preparted:
	 */
	BUILD_BUG_ON((__fix_to_virt(FIX_BTMAP_BEGIN) >> PMD_SHIFT)
		     != (__fix_to_virt(FIX_BTMAP_END) >> PMD_SHIFT));

	if ((pmd != fixmap_pmd(fix_to_virt(FIX_BTMAP_BEGIN)))
	     || pmd != fixmap_pmd(fix_to_virt(FIX_BTMAP_END))) {
		WARN_ON(1);
		pr_warn("pmd %p != %p, %p\n",
			pmd, fixmap_pmd(fix_to_virt(FIX_BTMAP_BEGIN)),
			fixmap_pmd(fix_to_virt(FIX_BTMAP_END)));
		pr_warn("fix_to_virt(FIX_BTMAP_BEGIN): %08lx\n",
			fix_to_virt(FIX_BTMAP_BEGIN));
		pr_warn("fix_to_virt(FIX_BTMAP_END):   %08lx\n",
			fix_to_virt(FIX_BTMAP_END));

		pr_warn("FIX_BTMAP_END:       %d\n", FIX_BTMAP_END);
		pr_warn("FIX_BTMAP_BEGIN:     %d\n", FIX_BTMAP_BEGIN);
	}
}

void __set_fixmap(enum fixed_addresses idx,
			       phys_addr_t phys, pgprot_t flags)
{
	unsigned long addr = __fix_to_virt(idx);
	pte_t *pte;

	BUG_ON(idx <= FIX_HOLE || idx >= __end_of_fixed_addresses);

	pte = fixmap_pte(addr);

	if (pgprot_val(flags)) {
		set_pte(pte, pfn_pte(phys >> PAGE_SHIFT, flags));
	} else {
		pte_clear(&init_mm, addr, pte);
		flush_tlb_kernel_range(addr, addr+PAGE_SIZE);
	}
}

void *__init fixmap_remap_fdt(phys_addr_t dt_phys)
{
	const u64 dt_virt_base = __fix_to_virt(FIX_FDT);
	pgprot_t prot = PAGE_KERNEL_RO;
	int size, offset;
	void *dt_virt;

	/*
	 * Check whether the physical FDT address is set and meets the minimum
	 * alignment requirement. Since we are relying on MIN_FDT_ALIGN to be
	 * at least 8 bytes so that we can always access the size field of the
	 * FDT header after mapping the first chunk, double check here if that
	 * is indeed the case.
	 */
	BUILD_BUG_ON(MIN_FDT_ALIGN < 8);
	if (!dt_phys || dt_phys % MIN_FDT_ALIGN)
		return NULL;

	/*
	 * Make sure that the FDT region can be mapped without the need to
	 * allocate additional translation table pages, so that it is safe
	 * to call create_mapping() this early.
	 *
	 * On 64k pages, the FDT will be mapped using PTEs, so we need to
	 * be in the same PMD as the rest of the fixmap.
	 * On 4k pages, we'll use section mappings for the FDT so we only
	 * have to be in the same PUD.
	 */
	BUILD_BUG_ON(dt_virt_base % SZ_2M);

	BUILD_BUG_ON(__fix_to_virt(FIX_FDT_END) >> SWAPPER_TABLE_SHIFT !=
		     __fix_to_virt(FIX_BTMAP_BEGIN) >> SWAPPER_TABLE_SHIFT);

	offset = dt_phys % SWAPPER_BLOCK_SIZE;
	dt_virt = (void *)dt_virt_base + offset;

	/* map the first chunk so we can read the size from the header */
	create_mapping(round_down(dt_phys, SWAPPER_BLOCK_SIZE), dt_virt_base,
		       SWAPPER_BLOCK_SIZE, prot);

	if (fdt_check_header(dt_virt) != 0)
		return NULL;

	size = fdt_totalsize(dt_virt);
	if (size > MAX_FDT_SIZE)
		return NULL;

	if (offset + size > SWAPPER_BLOCK_SIZE)
		create_mapping(round_down(dt_phys, SWAPPER_BLOCK_SIZE), dt_virt_base,
			       round_up(offset + size, SWAPPER_BLOCK_SIZE), prot);

	memblock_reserve(dt_phys, size);

	return dt_virt;
}
/* For compatible with Exynos */

LIST_HEAD(static_vmlist);

void __init add_static_vm_early(struct static_vm *svm)
{
	struct static_vm *curr_svm;
	struct vm_struct *vm;
	void *vaddr;

	vm = &svm->vm;
	vm_area_add_early(vm);
	vaddr = vm->addr;

	list_for_each_entry(curr_svm, &static_vmlist, list) {
		vm = &curr_svm->vm;

		if (vm->addr > vaddr)
			break;
	}
	list_add_tail(&svm->list, &curr_svm->list);
}

static void __init *early_alloc_aligned(unsigned long sz, unsigned long align)
{
	void *ptr = __va(memblock_alloc(sz, align));
	memset(ptr, 0, sz);
	return ptr;
}

/*
 * Create the architecture specific mappings
 */
static void __init __iotable_init(struct map_desc *io_desc, int nr, int exec)
{
	struct map_desc *md;
	struct vm_struct *vm;
	struct static_vm *svm;
	phys_addr_t phys;
	pgprot_t prot;

	if (!nr)
		return;

	svm = early_alloc_aligned(sizeof(*svm) * nr, __alignof__(*svm));

	if (!exec) {
		iotable_on = 1;
		prot = pgprot_iotable_init(PAGE_KERNEL_EXEC);
	} else {
		iotable_on = 0;
		prot = PAGE_KERNEL_EXEC;
	}

	for (md = io_desc; nr; md++, nr--) {
		phys = __pfn_to_phys(md->pfn);
		create_mapping(phys, md->virtual, md->length, prot);
		vm = &svm->vm;
		vm->addr = (void *)(md->virtual & PAGE_MASK);
		vm->size = PAGE_ALIGN(md->length + (md->virtual & ~PAGE_MASK));
		vm->phys_addr = __pfn_to_phys(md->pfn);
		vm->flags = VM_IOREMAP | VM_ARM_STATIC_MAPPING;
		vm->flags |= VM_ARM_MTYPE(md->type);
		vm->caller = iotable_init;
		add_static_vm_early(svm++);
	}

	iotable_on = 0;
}

void __init iotable_init(struct map_desc *io_desc, int nr)
{
	__iotable_init(io_desc, nr, 0);
}

void __init iotable_init_exec(struct map_desc *io_desc, int nr)
{
	__iotable_init(io_desc, nr, 1);
}
