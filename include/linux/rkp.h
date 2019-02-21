/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef _RKP_H
#define _RKP_H

#ifndef __ASSEMBLY__
#define RKP_PREFIX  UL(0x83800000)
#define RKP_CMDID(CMD_ID)  ((UL(CMD_ID) << 12 ) | RKP_PREFIX)

#define RKP_INIT		RKP_CMDID(0x0)
#define RKP_DEF_INIT		RKP_CMDID(0x1)
#define RKP_DEBUG		RKP_CMDID(0x2)
#define RKP_NOSHIP_BIN		RKP_CMDID(0x3)

#define RKP_PGD_SET		RKP_CMDID(0x21)
#define RKP_PMD_SET		RKP_CMDID(0x22)
#define RKP_PTE_SET		RKP_CMDID(0x23)
#define RKP_PGD_FREE		RKP_CMDID(0x24)
#define RKP_PGD_NEW		RKP_CMDID(0x25)

#define KASLR_MEM_RESERVE	RKP_CMDID(0x70)
#define RKP_FIMC_VERIFY		RKP_CMDID(0x71)

#define CFP_JOPP_INIT		RKP_CMDID(0x98)

#define RKP_INIT_MAGIC		0x5afe0001
#define RKP_VMM_BUFFER		0x600000
#define RKP_RO_BUFFER		UL(0x800000)

#define RKP_FIMC_FAIL		0x10
#define RKP_FIMC_SUCCESS	0xa5

/*** TODO: We need to export this so it is hard coded
     at one place*/

#ifdef CONFIG_RKP_6G
#define RKP_PGT_BITMAP_LEN 0x30000
#else
#define RKP_PGT_BITMAP_LEN 0x20000
#endif

/*
 * number of reserve memory contents.
 * If you add more reserved memory, RKP_NUM_MEM MUST be adjuested.
 */

#define RKP_NUM_MEM		0x7

#ifdef CONFIG_RKP_6G
#define RKP_EXTRA_MEM_START	0xAF200000
#define RKP_EXTRA_MEM_SIZE	0x800000

#define RKP_PHYS_MAP_START	0xAFA00000
#define RKP_PHYS_MAP_SIZE	0x700000
#else
#define RKP_EXTRA_MEM_START	0xAF400000
#define RKP_EXTRA_MEM_SIZE	0x600000

#define RKP_PHYS_MAP_START	0xAFC00000
#define RKP_PHYS_MAP_SIZE	0x500000
#endif

#define RKP_VMM_START		0xB0100000
#define RKP_VMM_SIZE		(1UL<<20)

/*
 * This size include both rkp log and vmm log
 * Each is 1UL << 17
 */
#define RKP_LOG_START		0xB0200000
#define RKP_LOG_SIZE		(1UL<<18)

/*
 * GAP between RKP_LOG and SEC_LOG
 */

#define RKP_SEC_LOG_START	0xB0400000
#define RKP_SEC_LOG_SIZE	0x7000

#define RKP_DASHBOARD_START	0xB0407000
#define RKP_DASHBOARD_SIZE	0x1000

#define RKP_ROBUF_START		0xB0408000
#ifdef CONFIG_RKP_6G
#define RKP_ROBUF_SIZE		0xBF8000 /* 12MB */
#else
#define RKP_ROBUF_SIZE		0x7f8000 /* 8MB - RKP_SEC_LOG_SIZE - RKP_DASHBOARD_SIZE)*/
#endif

#define RKP_RBUF_VA		(phys_to_virt(RKP_ROBUF_START))
#define RO_PAGES		(RKP_ROBUF_SIZE >> PAGE_SHIFT) // (RKP_ROBUF_SIZE/PAGE_SIZE)
#define CRED_JAR_RO		"cred_jar_ro"
#define TSEC_JAR		"tsec_jar"
#define VFSMNT_JAR		"vfsmnt_cache"

#define RKP_LOGI(fmt, ...) pr_info("RKP: "fmt, ##__VA_ARGS__)
#define RKP_LOGW(fmt, ...) pr_warn("RKP: "fmt, ##__VA_ARGS__)
#define RKP_LOGE(fmt, ...) pr_err("RKP: "fmt, ##__VA_ARGS__)
#define RKP_LOGA(fmt, ...) pr_alert("RKP: "fmt, ##__VA_ARGS__)

void rkp_call(unsigned long long cmd, unsigned long long arg0, unsigned long long arg1, unsigned long long arg2, unsigned long long arg3, unsigned long long arg4);

int rkp_reserve_mem(void);

extern u8 rkp_pgt_bitmap[];
extern u8 rkp_map_bitmap[];

typedef struct rkp_init rkp_init_t;
extern u8 rkp_started;
extern void *rkp_ro_alloc(void);
extern void rkp_ro_free(void *free_addr);
extern unsigned int is_rkp_ro_page(u64 addr);

struct rkp_init {
	u32 magic;
	u64 vmalloc_start;
	u64 vmalloc_end;
	u64 init_mm_pgd;
	u64 id_map_pgd;
	u64 zero_pg_addr;
	u64 rkp_pgt_bitmap;
	u64 rkp_dbl_bitmap;
	u32 rkp_bitmap_size;
	u64 _text;
	u64 _etext;
	u64 extra_memory_addr;
	u32 extra_memory_size;
	u64 physmap_addr;
	u64 _srodata;
	u64 _erodata;
	u32 large_memory;
	u64 fimc_phys_addr;
	u64 fimc_size;
#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
	u64 tramp_pgd;
#endif
} ;

#ifdef CONFIG_RKP_KDP
typedef struct kdp_init
{
	u32 credSize;
	u32 cred_task;
	u32 mm_task;
	u32 uid_cred;
	u32 euid_cred;
	u32 gid_cred;
	u32 egid_cred;
	u32 bp_pgd_cred;
	u32 bp_task_cred;
	u32 type_cred;
	u32 security_cred;
	u32 pid_task;
	u32 rp_task;
	u32 comm_task;
	u32 pgd_mm;
	u32 usage_cred;
	u32 task_threadinfo;
	u32 sp_size;
	u32 bp_cred_secptr;
} kdp_init_t;
#endif  /* CONFIG_RKP_KDP */

#define rkp_is_pg_protected(va)	rkp_is_protected(va,__pa(va),(u64 *)rkp_pgt_bitmap,1)
#define rkp_is_pg_dbl_mapped(pa) rkp_is_protected((u64)__va(pa),pa,(u64 *)rkp_map_bitmap,0)

#define RKP_PHYS_ADDR_MASK		((1ULL << 40)-1)

/*
 * The following all assume PHYS_OFFSET is fix addr
 */
#define	PHYS_PFN_OFFSET_MIN_DRAM1	(0x80000ULL)
#define	PHYS_PFN_OFFSET_MAX_DRAM1	(0x100000ULL)
#define	PHYS_PFN_OFFSET_MIN_DRAM2	(0x880000ULL)
#ifdef CONFIG_RKP_6G
#define	PHYS_PFN_OFFSET_MAX_DRAM2	(0x980000ULL)
#else
#define	PHYS_PFN_OFFSET_MAX_DRAM2	(0x900000ULL)
#endif

#define DRAM_PFN_GAP			(PHYS_PFN_OFFSET_MIN_DRAM2 - PHYS_PFN_OFFSET_MAX_DRAM1)

#define FIMC_LIB_OFFSET_VA		(VMALLOC_START + 0xF6000000 - 0x8000000)
#define FIMC_LIB_START_VA		(FIMC_LIB_OFFSET_VA + 0x04000000)
#define FIMC_VRA_LIB_SIZE		(0x80000)
#define FIMC_DDK_LIB_SIZE		(0x400000)
#define FIMC_RTA_LIB_SIZE		(0x300000)

#define FIMC_LIB_SIZE			(FIMC_VRA_LIB_SIZE + FIMC_DDK_LIB_SIZE + FIMC_RTA_LIB_SIZE)

static inline u64 rkp_get_sys_index(u64 pfn) {
	if (pfn >= PHYS_PFN_OFFSET_MIN_DRAM1
		&& pfn < PHYS_PFN_OFFSET_MAX_DRAM1) {
		return ((pfn) - PHYS_PFN_OFFSET);
	}
	if (pfn >= PHYS_PFN_OFFSET_MIN_DRAM2
		&& pfn < PHYS_PFN_OFFSET_MAX_DRAM2) {
		return ((pfn) - PHYS_PFN_OFFSET - DRAM_PFN_GAP);
	}
	return (~0ULL);
}

static inline u8 rkp_is_protected(u64 va,u64 pa, u64 *base_addr,u64 type)
{
	u64 phys_addr = pa & (RKP_PHYS_ADDR_MASK);
	u64 *p = base_addr;
	u64 rindex;
	u8 val;
	u64 index = rkp_get_sys_index((phys_addr>>PAGE_SHIFT));

	if (index == (~0ULL))
		return 0;

	p += (index>>6);
	rindex = index % 64;
	val = (((*p) & (1ULL<<rindex))?1:0);
	return val;
}
#endif //__ASSEMBLY__

#endif //_RKP_H
