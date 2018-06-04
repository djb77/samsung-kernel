#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rkp.h>

#ifdef CONFIG_NO_BOOTMEM
#include <linux/memblock.h>
#endif

struct rkp_mem_info {
#ifdef CONFIG_NO_BOOTMEM
	phys_addr_t start_add;
	phys_addr_t size;
#else
	unsigned long start_add;
	unsigned long size;
#endif
} rkp_mem[RKP_NUM_MEM] __initdata;

static void __init fill_rkp_mem_info(void)
{
	rkp_mem[0].start_add = RKP_EXTRA_MEM_START;
	rkp_mem[1].start_add = RKP_PHYS_MAP_START;
	rkp_mem[2].start_add = RKP_VMM_START;
	rkp_mem[3].start_add = RKP_LOG_START;
	//RKP Secure Log. It is not used. Just alloc memory.
	rkp_mem[4].start_add = RKP_SEC_LOG_START;
	rkp_mem[5].start_add = RKP_DASHBOARD_START;
	rkp_mem[6].start_add = RKP_ROBUF_START;

	rkp_mem[0].size = RKP_EXTRA_MEM_SIZE;
	rkp_mem[1].size = RKP_PHYS_MAP_SIZE;
	rkp_mem[2].size = RKP_VMM_SIZE;
	rkp_mem[3].size = RKP_LOG_SIZE;
	rkp_mem[4].size = RKP_SEC_LOG_SIZE;
	rkp_mem[5].size = RKP_DASHBOARD_SIZE;
	rkp_mem[6].size = RKP_ROBUF_SIZE;
}

int __init rkp_reserve_mem(void)
{
	int i = 0;

	fill_rkp_mem_info();

	for (i = 0; i < RKP_NUM_MEM; i++) {
#ifdef CONFIG_NO_BOOTMEM
		if (memblock_is_region_reserved(rkp_mem[i].start_add, rkp_mem[i].size) ||
		    memblock_reserve(rkp_mem[i].start_add, rkp_mem[i].size)) {
#else
		if (reserve_bootmem(rkp_mem[i].start_add, rkp_mem[i].size, BOOTMEM_EXCLUSIVE)) {
#endif
			RKP_LOGE("%s: RKP failed reserving size 0x%x at base 0x%x\n",
				__func__, (unsigned int)rkp_mem[i].size, (unsigned int)rkp_mem[i].start_add);
			return -1;
		}
		RKP_LOGI("%s, base:0x%x, size:0x%x\n",
			__func__, (unsigned int)rkp_mem[i].start_add, (unsigned int)rkp_mem[i].size);
	}
	return 0;
}
