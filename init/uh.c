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
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <asm/cacheflush.h>
#include <asm/irqflags.h>
#include <linux/fs.h>
#include <asm/tlbflush.h>
#include <linux/init.h>
#include <linux/io.h>

#include <linux/uh.h>
#include "ld.h"

#define UH_32BIT_SMC_CALL_MAGIC 0x82000400
#define UH_64BIT_SMC_CALL_MAGIC 0xC2000400

#define UH_STACK_OFFSET 0x2000

#define UH_MODE_AARCH32 0
#define UH_MODE_AARCH64 1

struct uh_elf_info {
	void *base, *text_head, *bss;   /* VA only */
	size_t size, text_head_size, bss_size;
};

int __init uh_disable(void)
{
	_uh_goto_EL2(UH_64BIT_SMC_CALL_MAGIC, (void *)virt_to_phys(&_uh_disable),
			UH_STACK_OFFSET, UH_MODE_AARCH64, NULL, 0);

	pr_alert("%s\n", __func__);

	return 0;
}

int __init uh_entry(struct uh_elf_info *uei)
{
	/* what do we do here?:
	 *  1. get entry point pa (.text.head section)
	 *  2. ask el3 to bring us there
	 */

	int status;
	void *entry = (void *)virt_to_phys(uei->text_head);

	/* why? */
	flush_cache_all();

	/* sanity check */
	pr_alert("uh pa entry=%p\n", entry);
	status = _uh_goto_EL2(UH_64BIT_SMC_CALL_MAGIC,
							entry,
							UH_STACK_OFFSET,
							UH_MODE_AARCH64,
							(void *)UH_START,
							UH_SIZE);

	pr_alert("status=%d\n", status);

	return 0;
}

int __init uh_init(void)
{
	/* what do we do here?:
	 *  1. copy uh.elf from kimage to reserved area
	 *  2. wipe out uh.elf on kimage (unnecessary if __init declared)
	 *  3. get .bss and .text.head section info
	 *  4. zero out .bss on copied uh.elf
	 *  5. call uh_entry
	 */

	int ret;
	struct uh_elf_info uh_reserved = {
	    .base = (void *)phys_to_virt(UH_START),
	    .size = UH_SIZE
	};
	struct uh_elf_info uh_kimage = {
	    .base = &_start_uh,
	    .size = (size_t)(&_end_uh - &_start_uh)
	};

	/* copy elf to reserved area and terminate one on kimage */
	BUG_ON(uh_kimage.size > uh_reserved.size);
	memcpy(uh_reserved.base, uh_kimage.base, uh_kimage.size);
	memset(uh_kimage.base, 0, uh_kimage.size);

	/* get .bss and .text.head info */
	if (ld_get_sect(uh_reserved.base, ".bss", &uh_reserved.bss, &uh_reserved.bss_size)) {
		pr_alert("can't find .bss section from uh_reserved.base=%p\n", uh_reserved.base);
		return -1;
	}

	if (ld_get_sect(uh_reserved.base, ".text.head", &uh_reserved.text_head, &uh_reserved.text_head_size)) {
		pr_alert("can't find .text.head section from uh_reserved.base=%p\n", uh_reserved.base);
		return -1;
	}

	/* zero out bss */
	memset(uh_reserved.bss, 0, uh_reserved.bss_size);

	/* sanity check */
	pr_alert("uh_reserved\n"
			"  .base=%p .size=%lu\n"
			"  .bss=%p .bss_size=%lu\n"
			"  .text_head=%p .text_head_size=%lu\n",
				uh_reserved.base,
				uh_reserved.size,
				uh_reserved.bss,
				uh_reserved.bss_size,
				uh_reserved.text_head,
				uh_reserved.text_head_size);
	pr_alert("uh_kimage\n"
			"  .base=%p .size=%lu\n", uh_kimage.base, uh_kimage.size);
	pr_alert("UH_START=%x UH_SIZE=%lu\n", UH_START, UH_SIZE);

	/* jump */
	ret = uh_entry(&uh_reserved);
	BUG_ON(ret != 0);

	return 0;
}
EXPORT_SYMBOL(uh_init);
