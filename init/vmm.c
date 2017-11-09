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

extern char _svmm;
extern char _evmm;
extern char _vmm_disable;

char *vmm;
size_t vmm_size;

int vmm_entry(void *);

int _vmm_goto_EL2(int magic, void *label, int offset, int mode, void *base, int size);

int vmm_resolve(void *binary, _Elf_Sym *sym, _Elf_Sxword *value) {

	char *name;
	char *strtab;
	size_t strtabsz;

	printk(KERN_ALERT "%s\n", __FUNCTION__);

	if(ld_get_dynamic_strtab(binary, &strtab, &strtabsz)) { return -1; }

	if(ld_get_string(strtab, sym->st_name, &name)) { return -1; }

	printk(KERN_ALERT "name: %s\n", name);

	return -1;
}

int vmm_translate(void *binary, void *in, void **out) {

	printk(KERN_ALERT "%s\n", __FUNCTION__);

	*out = (void *)virt_to_phys(in);

	return 0;
}

int vmm_disable(void) {

	_vmm_goto_EL2(VMM_64BIT_SMC_CALL_MAGIC, (void *)virt_to_phys(&_vmm_disable), VMM_STACK_OFFSET, VMM_MODE_AARCH64, NULL, 0);

	printk(KERN_ALERT "%s\n", __FUNCTION__);

	return 0;
}

int vmm_entry(void *head_text_base) {

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
int vmm_init(void) {

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
	memset(&_svmm, 0, (size_t)(&_evmm - &_svmm));

	vmm_entry((void *)head_text_base);

	return 0;
}
