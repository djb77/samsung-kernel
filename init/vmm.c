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


#include <linux/vmm.h>
#include "ld.h"


#define VMM_32BIT_SMC_CALL_MAGIC 0x82000400
#define VMM_64BIT_SMC_CALL_MAGIC 0xC2000400

#define VMM_STACK_OFFSET 0x2000

#define VMM_MODE_AARCH32 0
#define VMM_MODE_AARCH64 1


char * __initdata vmm;
size_t __initdata vmm_size;

extern char _svmm;
extern char _evmm;
extern char _vmm_disable;

int __init vmm_disable(void)
{
	_vmm_goto_EL2(VMM_64BIT_SMC_CALL_MAGIC, (void *)virt_to_phys(&_vmm_disable), VMM_STACK_OFFSET, VMM_MODE_AARCH64, NULL, 0);

	printk(KERN_ALERT "%s\n", __FUNCTION__);

	return 0;
}

static int __init vmm_entry(void *head_text_base)
{
	typedef int (*_main_)(void);

	int status;
	_main_ entry_point;

	printk(KERN_ALERT "%s\n", __FUNCTION__);

	entry_point = (_main_)head_text_base;

	printk(KERN_ALERT "vmm entry point: %p\n", entry_point);

	vmm = (char *)virt_to_phys(vmm);

	flush_cache_all();

	status = _vmm_goto_EL2(VMM_64BIT_SMC_CALL_MAGIC, entry_point, VMM_STACK_OFFSET, VMM_MODE_AARCH64, vmm, vmm_size);

	printk(KERN_ALERT "vmm(%p, 0x%x): %x\n", vmm, (int)vmm_size, status);

	return 0;
}

int __init vmm_init(void)
{
	size_t bss_size = 0,head_text_size = 0;
	u64 bss_base = 0,head_text_base = 0;
	void *base;

	printk(KERN_ALERT "VMM-%s\n", __FUNCTION__);

	if(smp_processor_id() != 0) { return 0; }

	printk(KERN_ALERT "bin 0x%p, 0x%x\n", &_svmm, (int)(&_evmm - &_svmm));
	memcpy((void *)phys_to_virt(VMM_RUNTIME_BASE),  &_svmm, (size_t)(&_evmm - &_svmm));

	vmm = (void *)phys_to_virt(VMM_RUNTIME_BASE);
	vmm_size = VMM_RUNTIME_SIZE;

	if(ld_get_sect(vmm, ".bss", &base, &bss_size)) { return -1; }
	printk(KERN_ALERT "BSS base = 0x%p, 0x%x\n", base, (int)bss_size);
	bss_base  = (u64)VMM_RUNTIME_BASE+(u64)virt_to_phys(base);
	
	if(ld_get_sect(vmm, ".text.head", &base, &head_text_size)) { return -1; }
	printk(KERN_ALERT "Head Text base = 0x%p, 0x%x\n", base, (int)head_text_size);
	head_text_base  = (u64)VMM_RUNTIME_BASE+(u64)virt_to_phys(base);	
	

	memset((void *)phys_to_virt(bss_base), 0, (size_t)bss_size);
	//Now vmm will be freed by free_initmem, no need to zero out
	//memset(&_svmm, 0, (size_t)(&_evmm - &_svmm));

	vmm_entry((void *)head_text_base);

	return 0;
}
