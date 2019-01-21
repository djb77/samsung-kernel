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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <asm/cacheflush.h>
#include <asm/irqflags.h>
#include <linux/fs.h>
#include <asm/tlbflush.h>
#include <linux/init.h>
#include <asm/io.h>

#include <linux/rkp.h>
#include <linux/vmm.h>
#include "ld.h"

#define VMM_32BIT_SMC_CALL_MAGIC 0x82000400
#define VMM_64BIT_SMC_CALL_MAGIC 0xC2000400

#define VMM_STACK_OFFSET 0x2000

#define VMM_MODE_AARCH32 0
#define VMM_MODE_AARCH64 1

struct vmm_elf_info {
	void *base, *text_head, *bss; //VA only
	size_t size, text_head_size, bss_size;
};

extern char _svmm;
extern char _evmm;
extern char _vmm_disable;

char * __initdata vmm;
size_t __initdata vmm_size;

int __init vmm_disable(void)
{
	_vmm_goto_EL2(VMM_64BIT_SMC_CALL_MAGIC, (void *)virt_to_phys(&_vmm_disable), 
			VMM_STACK_OFFSET, VMM_MODE_AARCH64, NULL, 0);

	RKP_LOGA("%s\n", __FUNCTION__);
	return 0;
}

int __init vmm_entry(struct vmm_elf_info *vei)
{
	/*
	 * 1. get entry point pa. (.text.head section)
	 * 2. ask el3 to bring us there.
	 */

	int status;
	void *entry = (void *)virt_to_phys(vei->text_head);

	flush_cache_all();

	/* santiy check */
	RKP_LOGA("entry point=%p\n", entry);
	status = _vmm_goto_EL2(VMM_64BIT_SMC_CALL_MAGIC, entry, 
				VMM_STACK_OFFSET, VMM_MODE_AARCH64,
				(void *)RKP_VMM_START, RKP_VMM_SIZE);

	RKP_LOGA("status=%d\n", status);
	return 0;
}

int __init vmm_init(void)
{
	/*
	 * 1. copy vmm.elf from kimage to reserved area.
	 * 2. wipe out vmm.elf on kimage. 
	 * 3. get .bss and .text.head section.
	 * 4. zero out .bss on coped vmm.elf
	 * 5. call vmm_entry
	 */

	int ret = 0;
	struct vmm_elf_info vmm_reserved = {
		.base = (void *)phys_to_virt(RKP_VMM_START),
		.size = RKP_VMM_SIZE
	};
	struct vmm_elf_info vmm_kimage = {
		.base = &_svmm,
		.size = (size_t)(&_evmm - &_svmm)
	};

	/* copy elf to reserved area and terminate one on kimage */
	BUG_ON(vmm_kimage.size > vmm_reserved.size);
	memcpy(vmm_reserved.base, vmm_kimage.base, vmm_kimage.size);
	memset(vmm_kimage.base, 0, vmm_kimage.size);

	/* get .bss and .text.head info*/
	if (ld_get_sect(vmm_reserved.base, ".bss", &vmm_reserved.bss, &vmm_reserved.bss_size)) {
		RKP_LOGA("Can't fine .bss section from vmm_reserved.base=%p\n", 
								vmm_reserved.base);
		return -1;
	}

	if (ld_get_sect(vmm_reserved.base, ".text.head", 
			&vmm_reserved.text_head, &vmm_reserved.text_head_size)) {
		RKP_LOGA("Can't find .text.head section from vmm_reserved.base=%p\n", 
								vmm_reserved.base);
		return -1;
	}

	/* zero out bss */
	memset(vmm_reserved.bss, 0, vmm_reserved.bss_size);

	/* sanity check */
	RKP_LOGA("vmm_reserved\n"
			"  .base=%p .size=%lu\n"
			"  .bss=%p .bss_size=%lu\n"
			"  .text_head=%p .text_head_size=%lu\n",
			vmm_reserved.base,
			vmm_reserved.size,
			vmm_reserved.bss,
			vmm_reserved.bss_size,
			vmm_reserved.text_head,
			vmm_reserved.text_head_size);
	RKP_LOGA("vmm_kimage\n"
			".base=%p  .size=%lu\n", 
			vmm_kimage.base, vmm_kimage.size);
	RKP_LOGA("vmm_start=%x, vmm_size=%lu\n", RKP_VMM_START, RKP_VMM_SIZE);
				
	/* jump */
	ret = vmm_entry(&vmm_reserved);
	BUG_ON(ret != 0);

	return 0;
}
EXPORT_SYMBOL(vmm_init);
