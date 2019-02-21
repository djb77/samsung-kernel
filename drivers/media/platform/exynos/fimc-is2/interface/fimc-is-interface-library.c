/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/random.h>
#include <linux/firmware.h>
#include <linux/mm.h>
#include <linux/kallsyms.h>
#include <linux/stacktrace.h>

#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <asm/neon.h>

#include "fimc-is-interface-library.h"
#include "fimc-is-interface-vra.h"
#include "../fimc-is-device-ischain.h"
#include "fimc-is-vender.h"
#include "fimc-is-companion.h"
#include "fimc-is-device-sensor-peri.h"

#ifdef CONFIG_RKP
#include <linux/rkp.h>
#endif

struct fimc_is_lib_support gPtr_lib_support;
struct mutex gPtr_bin_load_ctrl;

/*
 * Log write
 */

int fimc_is_log_write_console(char *str)
{
	pr_info("[@][LIB] %s", str);
	return 0;
}

int fimc_is_log_write(const char *str, ...)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	int size = 0;
	int cpu = raw_smp_processor_id();
	unsigned long long t;
	unsigned long usec;
	va_list args;
	ulong debug_kva, ctrl_kva;
	unsigned long flag;

	va_start(args, str);
	size += vscnprintf(lib->string + size,
			  sizeof(lib->string) - size, str, args);
	va_end(args);

	t = local_clock();
	usec = do_div(t, NSEC_PER_SEC) / NSEC_PER_USEC;
	size = snprintf(lib->log_buf, sizeof(lib->log_buf),
		"[%5lu.%06lu] [%d] %s",
		(unsigned long)t, usec, cpu, lib->string);

	if (size > MAX_LIB_LOG_BUF)
		size = MAX_LIB_LOG_BUF;

	spin_lock_irqsave(&lib->slock_debug, flag);

	debug_kva = lib->minfo->kvaddr_debug;
	ctrl_kva  = lib->minfo->kvaddr_debug_cnt;

	if (lib->log_ptr == 0) {
		lib->log_ptr = debug_kva;
	} else if (lib->log_ptr >= (ctrl_kva - size)) {
		memcpy((void *)lib->log_ptr, (void *)&lib->log_buf,
			(ctrl_kva - lib->log_ptr));
		size -= (debug_kva + DEBUG_REGION_SIZE - (u32)lib->log_ptr);
		lib->log_ptr = debug_kva;
	}

	memcpy((void *)lib->log_ptr, (void *)&lib->log_buf, size);
	lib->log_ptr += size;

	*((int *)(ctrl_kva)) = (lib->log_ptr - debug_kva);

	spin_unlock_irqrestore(&lib->slock_debug, flag);

	return 0;
}

#ifdef LIB_MEM_TRACK
#ifdef CONFIG_STACKTRACE
static void save_callstack(ulong *addrs)
{
	struct stack_trace trace;
	int i;

	trace.nr_entries = 0;
	trace.max_entries = MEM_TRACK_ADDRS_COUNT;
	trace.entries = addrs;
	trace.skip = 3;
	save_stack_trace(&trace);

	if (trace.nr_entries != 0 &&
			trace.entries[trace.nr_entries - 1] == ULONG_MAX)
		trace.nr_entries--;

	for (i = trace.nr_entries; i < MEM_TRACK_ADDRS_COUNT; i++)
		addrs[i] = 0;
}
#endif

static inline void add_alloc_track(int type, ulong addr, size_t size)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct lib_mem_tracks *tracks = lib->cur_tracks;
	struct lib_mem_track *track;

	if (tracks && (tracks->num_of_track == MEM_TRACK_COUNT)) {
		tracks = kzalloc(sizeof(struct lib_mem_tracks), GFP_KERNEL);
		lib->cur_tracks = tracks;

		if (tracks)
			list_add(&tracks->list, &lib->list_of_tracks);
		else
			err_lib("failed to allocate memory track");
	}

	if (tracks) {
		track = &tracks->track[tracks->num_of_track];
		tracks->num_of_track++;

		track->type = type;
		track->addr = addr;
		track->size = size;
		track->status = MT_STATUS_ALLOC;
		track->alloc.lr = (ulong)__builtin_return_address(0);
		track->alloc.cpu = raw_smp_processor_id();
		track->alloc.pid = current->pid;
		track->alloc.when = local_clock();

#ifdef CONFIG_STACKTRACE
		save_callstack(track->alloc.addrs);
#endif
	}
}

static inline void add_free_track(int type, ulong addr)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct lib_mem_tracks *tracks, *temp;
	struct lib_mem_track *track;
	int found = 0;
	int i;

	list_for_each_entry_safe(tracks, temp, &lib->list_of_tracks, list) {
		for (i = 0; i < tracks->num_of_track; i++) {
			track = &tracks->track[i];

			if ((track->addr == addr)
				&& (track->status == MT_STATUS_ALLOC)
				&& (track->type == type)) {
				track->status |= MT_STATUS_FREE;
				track->free.lr = (ulong)__builtin_return_address(0);
				track->free.cpu = raw_smp_processor_id();
				track->free.pid = current->pid;
				track->free.when = local_clock();

#ifdef CONFIG_STACKTRACE
		save_callstack(track->alloc.addrs);
#endif
				found = 1;
				break;
			}
		}

		if (found)
			break;
	}

	if (!found)
		err_lib("could not find buffer [%d, 0x%08lx]", type, addr);
}

static void print_track(struct lib_mem_track *track)
{
	unsigned long long when;
	ulong usec;
#ifdef CONFIG_STACKTRACE
	int i;
#endif

	pr_info("leak detected type: %d, addr: 0x%08lx, size: %zd, status: %d\n",
			track->type, track->addr, track->size, track->status);

	when = track->alloc.when;
	usec = do_div(when, NSEC_PER_SEC) / NSEC_PER_USEC;
	pr_info("\tallocated : [%5lu.%06lu] from: 0x%08lx, cpu: %d, pid: %d\n",
			(ulong)when, usec,
			track->alloc.lr-1, track->alloc.cpu, track->alloc.pid);
#ifdef CONFIG_STACKTRACE
	for (i = 0; i < MEM_TRACK_ADDRS_COUNT; i++)
		if (track->alloc.addrs[i])
			print_ip_sym(track->alloc.addrs[i]);
		else
			break;
#endif

	when = track->free.when;
	when = when / NSEC_PER_SEC;
	usec = do_div(when, NSEC_PER_SEC) / NSEC_PER_USEC;
	pr_info("\tfree:  [%5lu.%06lu] from: 0x%08lx, cpu: %d, pid: %d\n",
			(ulong)when, usec,
			track->free.lr-1, track->free.cpu, track->free.pid);

#ifdef CONFIG_STACKTRACE
	for (i = 0; i < MEM_TRACK_ADDRS_COUNT; i++)
		if (track->free.addrs[i])
			pr_info("\t%p\n", (void *)track->free.addrs[i]);
		else
			break;
#endif
}

static void check_tracks(int status)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct lib_mem_tracks *tracks, *temp;
	struct lib_mem_track *track;
	int i;

	list_for_each_entry_safe(tracks, temp, &lib->list_of_tracks, list) {
		for (i = 0; i < tracks->num_of_track; i++) {
			track = &tracks->track[i];

			if (track->status == status)
				print_track(track);
		}
	}
}

static void free_tracks(void)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct lib_mem_tracks *tracks;

	while (!list_empty(&lib->list_of_tracks)) {
		tracks = list_entry((&lib->list_of_tracks)->next,
						struct lib_mem_tracks, list);

		list_del(&tracks->list);
		kfree(tracks);
	}
}
#endif

/*
 * Memory alloc, free
 */
void *fimc_is_alloc_reserved_buffer(u32 size)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct fimc_is_priv_buf *pb_lib = gPtr_lib_support.minfo->pb_lib;
	struct fimc_is_lib_dma_buffer *reserved_buf = NULL;
	u32 aligned_size = 0;
	unsigned long flag;
#ifdef ENABLE_DBG_EVENT
	char debugmsg[256];
#endif

	if (size <= 0) {
		err_lib("invalid size(%d)", size);
		return NULL;
	}

	reserved_buf = kzalloc(sizeof(struct fimc_is_lib_dma_buffer), GFP_KERNEL);
	if (!reserved_buf) {
		err_lib("alloc failed of fimc_is_lib_dma_buffer");
		return NULL;
	}

	spin_lock_irqsave(&lib->slock_lib_mem, flag);
	if (lib->reserved_lib_size + size > pb_lib->size) {
		spin_unlock_irqrestore(&lib->slock_lib_mem, flag);
		err_lib("Out of reserved memory(0x%x), use dynamic alloc (0x%x)\n",
			lib->reserved_lib_size, size);
		reserved_buf->kvaddr = (ulong)kzalloc(size, GFP_KERNEL);
		if (!reserved_buf->kvaddr) {
			err_lib("alloc failed of reserved_buf->kvaddr (%d)", size);
			kfree(reserved_buf);
			return NULL;
		}
		reserved_buf->dvaddr = 0; /* dynamic */
		aligned_size = size;
		reserved_buf->size = aligned_size;
	} else {
		reserved_buf->kvaddr = lib->minfo->kvaddr_lib + lib->reserved_lib_size;
		reserved_buf->dvaddr = lib->minfo->dvaddr_lib + lib->reserved_lib_size;
		aligned_size = ALIGN(size, 0x40);
		reserved_buf->size = aligned_size;
		lib->reserved_lib_size += aligned_size;
		spin_unlock_irqrestore(&lib->slock_lib_mem, flag);
	}

#ifdef ENABLE_DBG_EVENT
	sprintf(debugmsg,"%s, %s, %d, 0x%16lx, 0x%8lx",
		__func__,
		current->comm,
		reserved_buf->size,
		reserved_buf->kvaddr,
		(unsigned long)reserved_buf->dvaddr);
	fimc_is_debug_event_add(FIMC_IS_EVENT_CRITICAL, debugmsg, NULL, NULL, 0);
#endif

	dbg_lib("alloc_reserved_buffer: kva(0x%lx) size(0x%x)\n",
		reserved_buf->kvaddr, aligned_size);

	spin_lock_irqsave(&lib->slock_lib_mem, flag);
	list_add(&reserved_buf->list, &lib->lib_mem_list);
	spin_unlock_irqrestore(&lib->slock_lib_mem, flag);
#ifdef LIB_MEM_TRACK
	add_alloc_track(MT_TYPE_RESERVED, reserved_buf->kvaddr, aligned_size);
#endif

	return (void *)reserved_buf->kvaddr;
}

void fimc_is_free_reserved_buffer(void *buf)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct fimc_is_lib_dma_buffer *reserved_buf, *temp;
	unsigned long flag;
#ifdef ENABLE_DBG_EVENT
	char debugmsg[256];
#endif

	if (buf == NULL)
		return;

	spin_lock_irqsave(&lib->slock_lib_mem, flag);
	list_for_each_entry_safe(reserved_buf, temp, &lib->lib_mem_list, list) {
		if ((void *)reserved_buf->kvaddr == buf) {
#ifdef LIB_MEM_TRACK
			add_free_track(MT_TYPE_RESERVED, (ulong)buf);
#endif
#ifdef ENABLE_DBG_EVENT
	sprintf(debugmsg,"%s, %s, %d, 0x%16lx, 0x%8lx",
		__func__,
		current->comm,
		reserved_buf->size,
		reserved_buf->kvaddr,
		(unsigned long)reserved_buf->dvaddr);
	fimc_is_debug_event_add(FIMC_IS_EVENT_NORMAL, debugmsg, NULL, NULL, 0);

#endif

			if (!reserved_buf->dvaddr)
				kfree((void *)reserved_buf->kvaddr);
			list_del(&reserved_buf->list);
			kfree(reserved_buf);

			break;
		}
	}
	spin_unlock_irqrestore(&lib->slock_lib_mem, flag);
}

void *fimc_is_alloc_reserved_taaisp_dma_buffer(u32 size)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct fimc_is_priv_buf *pb_taaisp = gPtr_lib_support.minfo->pb_taaisp;
	struct fimc_is_lib_dma_buffer *reserved_buf = NULL;
	unsigned long flag;

	if (size <= 0) {
		err_lib("invalid size(%d)", size);
		return NULL;
	}

	reserved_buf = kzalloc(sizeof(struct fimc_is_lib_dma_buffer), GFP_KERNEL);
	if (!reserved_buf) {
		err_lib("alloc failed of fimc_is_lib_dma_buffer");
		return NULL;
	}

	spin_lock_irqsave(&lib->slock_taaisp_mem, flag);
	if (lib->reserved_taaisp_size + size > pb_taaisp->size) {
		spin_unlock_irqrestore(&lib->slock_taaisp_mem, flag);
		err_lib("Out of taaisp reserved memory(0x%x), use dynamic alloc (0x%x)\n",
			lib->reserved_taaisp_size, size);
		reserved_buf->kvaddr = (ulong)kzalloc(size, GFP_KERNEL);
		if (!reserved_buf->kvaddr) {
			err_lib("alloc failed of reserved_buf->kvaddr (%d)", size);
			kfree(reserved_buf);
			return NULL;
		}
		reserved_buf->dvaddr = 0; /* dynamic */
		reserved_buf->size = size;
	} else {
		reserved_buf->kvaddr = lib->minfo->kvaddr_taaisp + lib->reserved_taaisp_size;
		reserved_buf->dvaddr = lib->minfo->dvaddr_taaisp + lib->reserved_taaisp_size;
		reserved_buf->size = size;
		lib->reserved_taaisp_size += size;
		spin_unlock_irqrestore(&lib->slock_taaisp_mem, flag);
	}

	dbg_lib("alloc_taaisp_reserved_buffer: kva(0x%lx) size(0x%x)\n",
		reserved_buf->kvaddr, size);

	spin_lock_irqsave(&lib->slock_taaisp_mem, flag);
	list_add(&reserved_buf->list, &lib->taaisp_mem_list);
	spin_unlock_irqrestore(&lib->slock_taaisp_mem, flag);
#ifdef LIB_MEM_TRACK
	add_alloc_track(MT_TYPE_TAAISP_DMA, reserved_buf->kvaddr, size);
#endif

	return (void *)reserved_buf->kvaddr;
}

void fimc_is_free_reserved_taaisp_dma_buffer(void *buf)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct fimc_is_lib_dma_buffer *reserved_buf, *temp;
	unsigned long flag;

	if (buf == NULL)
		return;

	spin_lock_irqsave(&lib->slock_taaisp_mem, flag);
	list_for_each_entry_safe(reserved_buf, temp, &lib->taaisp_mem_list, list) {
		if ((void *)reserved_buf->kvaddr == buf) {
#ifdef LIB_MEM_TRACK
			add_free_track(MT_TYPE_TAAISP_DMA, (ulong)buf);
#endif
			if (reserved_buf->dvaddr)
				lib->reserved_taaisp_size -= reserved_buf->size;
			else
				kfree((void *)reserved_buf->kvaddr);

			list_del(&reserved_buf->list);
			kfree(reserved_buf);

			break;
		}
	}
	spin_unlock_irqrestore(&lib->slock_taaisp_mem, flag);
}

void *fimc_is_alloc_reserved_vra_dma_buffer(u32 size)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct fimc_is_priv_buf *pb_vra = gPtr_lib_support.minfo->pb_vra;
	struct fimc_is_lib_dma_buffer *reserved_buf = NULL;
	unsigned long flag;

	if (size <= 0) {
		err_lib("invalid size(%d)", size);
		return NULL;
	}

	reserved_buf = kzalloc(sizeof(struct fimc_is_lib_dma_buffer), GFP_KERNEL);
	if (!reserved_buf) {
		err_lib("alloc failed of fimc_is_lib_dma_buffer");
		return NULL;
	}

	spin_lock_irqsave(&lib->slock_vra_mem, flag);
	if (lib->reserved_vra_size + size > pb_vra->size) {
		spin_unlock_irqrestore(&lib->slock_vra_mem, flag);
		err_lib("Out of taaisp reserved memory(0x%x), use dynamic alloc (0x%x)\n",
			lib->reserved_vra_size, size);
		reserved_buf->kvaddr = (ulong)kzalloc(size, GFP_KERNEL);
		if (!reserved_buf->kvaddr) {
			err_lib("alloc failed of reserved_buf->kvaddr (%d)", size);
			kfree(reserved_buf);
			return NULL;
		}
		reserved_buf->dvaddr = 0; /* dynamic */
		reserved_buf->size = size;
	} else {
		reserved_buf->kvaddr = lib->minfo->kvaddr_vra + lib->reserved_vra_size;
		reserved_buf->dvaddr = lib->minfo->dvaddr_vra + lib->reserved_vra_size;
		reserved_buf->size = size;
		lib->reserved_vra_size += size;
		spin_unlock_irqrestore(&lib->slock_vra_mem, flag);
	}

	dbg_lib("alloc_vra_reserved_buffer: kva(0x%lx) size(0x%x)\n",
		reserved_buf->kvaddr, size);

	spin_lock_irqsave(&lib->slock_vra_mem, flag);
	list_add(&reserved_buf->list, &lib->vra_mem_list);
	spin_unlock_irqrestore(&lib->slock_vra_mem, flag);
#ifdef LIB_MEM_TRACK
	add_alloc_track(MT_TYPE_VRA_DMA, reserved_buf->kvaddr, size);
#endif

	return (void *)reserved_buf->kvaddr;
}

void fimc_is_free_reserved_vra_dma_buffer(void *buf)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct fimc_is_lib_dma_buffer *reserved_buf, *temp;
	unsigned long flag;

	if (buf == NULL)
		return;

	spin_lock_irqsave(&lib->slock_vra_mem, flag);
	list_for_each_entry_safe(reserved_buf, temp, &lib->vra_mem_list, list) {
		if ((void *)reserved_buf->kvaddr == buf) {
#ifdef LIB_MEM_TRACK
			add_free_track(MT_TYPE_VRA_DMA, (ulong)buf);
#endif
			if (reserved_buf->dvaddr)
				lib->reserved_vra_size -= reserved_buf->size;
			else
				kfree((void *)reserved_buf->kvaddr);

			list_del(&reserved_buf->list);
			kfree(reserved_buf);

			break;
		}
	}
	spin_unlock_irqrestore(&lib->slock_vra_mem, flag);
}

/*
 * Address translation
 */
int fimc_is_translate_taaisp_kva_to_dva(ulong src_addr, u32 *target_addr)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	ulong offset;

	if (src_addr < lib->minfo->kvaddr_taaisp) {
		err_lib("translate_taaisp_kva_to_dva: Invalid kva(0x%lx)!!", src_addr);
		*target_addr = 0x0;
#ifdef CONFIG_STACKTRACE
		/* WARN_ON(1); */
#endif
		return 0;
	}

	offset = src_addr - lib->minfo->kvaddr_taaisp;
	*target_addr = lib->minfo->dvaddr_taaisp + offset;

	dbg_lib("translate_taaisp_kva_to_dva: offset(0x%lx), kva(0x%lx), dva(0x%x)\n",
		offset, src_addr, *target_addr);

	return 0;
}

int fimc_is_translate_vra_kva_to_dva(ulong src_addr, u32 *target_addr)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	ulong offset;

	if (src_addr < lib->minfo->kvaddr_vra) {
		err_lib("translate_vra_kva_to_dva: Invalid kva(0x%lx)!!", src_addr);
		*target_addr = 0x0;
#ifdef CONFIG_STACKTRACE
		WARN_ON(1);
#endif
		return 0;
	}

	offset = src_addr - lib->minfo->kvaddr_vra;
	*target_addr = lib->minfo->dvaddr_vra + offset;

	dbg_lib("translate_vra_kva_to_dva: offset(0x%lx), kva(0x%lx), dva(0x%x)\n",
		offset, src_addr, *target_addr);

	return 0;
}

int fimc_is_translate_taaisp_dva_to_kva(u32 src_addr, ulong *target_addr)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	u32 offset;

	if (src_addr < lib->minfo->dvaddr_taaisp) {
		err_lib("translate_taaisp_dva_to_kva: Invalid dva(0x%x)!!", src_addr);
		*target_addr = 0x0;
#ifdef CONFIG_STACKTRACE
		WARN_ON(1);
#endif
		return 0;
	}

	offset = src_addr - (u32)lib->minfo->dvaddr_taaisp;
	*target_addr = lib->minfo->kvaddr_taaisp + (ulong)offset;

	dbg_lib("translate_taaisp_dva_to_kva: dva(0x%x)\n", src_addr);

	return 0;
}

int fimc_is_translate_vra_dva_to_kva(u32 src_addr, ulong *target_addr)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	u32 offset;

	if (src_addr < lib->minfo->dvaddr_vra) {
		err_lib("translate_vra_dva_to_kva: Invalid dva(0x%x)!!", src_addr);
		*target_addr = 0x0;
#ifdef CONFIG_STACKTRACE
		WARN_ON(1);
#endif
		return 0;
	}

	offset = src_addr - (u32)lib->minfo->dvaddr_vra;
	*target_addr = lib->minfo->kvaddr_vra + (ulong)offset;

	dbg_lib("translate_vra_dva_to_kva: dva(0x%x)\n", src_addr);

	return 0;
}

/*
 * Assert
 */
int fimc_is_lib_logdump(void)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	size_t write_vptr, read_vptr;
	size_t read_cnt, read_cnt1, read_cnt2;
	void *read_ptr;
	ulong debug_kva = lib->minfo->kvaddr_debug;
	ulong ctrl_kva  = lib->minfo->kvaddr_debug_cnt;

	if (!test_bit(FIMC_IS_DEBUG_OPEN, &fimc_is_debug.state))
		return 0;

	write_vptr = *((int *)(ctrl_kva));
	read_vptr = fimc_is_debug.read_vptr;

	if (write_vptr > DEBUG_REGION_SIZE)
		 write_vptr = (read_vptr + DEBUG_REGION_SIZE) % (DEBUG_REGION_SIZE + 1);

	if (write_vptr >= read_vptr) {
		read_cnt1 = write_vptr - read_vptr;
		read_cnt2 = 0;
	} else {
		read_cnt1 = DEBUG_REGION_SIZE - read_vptr;
		read_cnt2 = write_vptr;
	}

	read_cnt = read_cnt1 + read_cnt2;
	info_lib("library log start(%zd)\n", read_cnt);

	if (read_cnt1) {
		read_ptr = (void *)(debug_kva + fimc_is_debug.read_vptr);

		fimc_is_print_buffer(read_ptr, read_cnt1);
		fimc_is_debug.read_vptr += read_cnt1;
	}

	if (fimc_is_debug.read_vptr >= DEBUG_REGION_SIZE) {
		if (fimc_is_debug.read_vptr > DEBUG_REGION_SIZE)
			err_lib("[DBG] read_vptr(%zd) is invalid", fimc_is_debug.read_vptr);
		fimc_is_debug.read_vptr = 0;
	}

	if (read_cnt2) {
		read_ptr = (void *)(debug_kva + fimc_is_debug.read_vptr);

		fimc_is_print_buffer(read_ptr, read_cnt2);
		fimc_is_debug.read_vptr += read_cnt2;
	}

	info_lib("library log end\n");

	return read_cnt;
}

static void __fimc_is_cleanup(struct fimc_is_core *core)
{
	struct fimc_is_device_sensor *device;
	int ret = 0;
	u32 i;

	if (!core) {
		err("%s: core(NULL)", __func__);
		return;
	}

	core->reboot = true;
	info("%s++: shutdown(%d), reboot(%d)\n", __func__, core->shutdown, core->reboot);
	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		if (!device) {
			err("%s: device(NULL)", __func__);
			continue;
		}

		if (test_bit(FIMC_IS_SENSOR_FRONT_START, &device->state)) {
			minfo("call sensor_front_stop()\n", device);

			ret = fimc_is_sensor_front_stop(device);
			if (ret)
				mwarn("fimc_is_sensor_front_stop() is fail(%d)", device, ret);
		}
	}
	info("%s:--\n", __func__);

	return;
}

static void __fimc_is_shutdown(struct platform_device *pdev)
{
	struct fimc_is_core *core = (struct fimc_is_core *)platform_get_drvdata(pdev);
	struct fimc_is_device_sensor *device;
	struct v4l2_subdev *subdev;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	u32 i;

	if (!core) {
		err("%s: core(NULL)", __func__);
		return;
	}

	core->shutdown = true;
	info("%s++: shutdown(%d), reboot(%d)\n", __func__, core->shutdown, core->reboot);
#if !defined(ENABLE_IS_CORE)
	__fimc_is_cleanup(core);
#endif
	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		if (!device) {
			warn("%s: device(NULL)", __func__);
			continue;
		}

		if (test_bit(FIMC_IS_SENSOR_OPEN, &device->state)) {
			subdev = device->subdev_module;
			if (!subdev) {
				warn("%s: subdev(NULL)", __func__);
				continue;
			}

			module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
			if (!module) {
				warn("%s: module(NULL)", __func__);
				continue;
			}

			sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
			if (!sensor_peri) {
				warn("%s: sensor_peri(NULL)", __func__);
				continue;
			}

			fimc_is_sensor_deinit_sensor_thread(sensor_peri);
			fimc_is_sensor_deinit_mode_change_thread(sensor_peri);
		}
	}
	info("%s:--\n", __func__);

	return;
}

static void fimc_is_dump_n_stop(struct work_struct *data)
{
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct v4l2_subdev *subdev_preprocessor;
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_cis *cis;

	core = (struct fimc_is_core *)platform_get_drvdata(gPtr_lib_support.pdev);
	sensor = &core->sensor[0];
	module = v4l2_get_subdevdata(sensor->subdev_module);
	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
	subdev_preprocessor = sensor_peri->subdev_preprocessor;
	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev_preprocessor);
	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);

	CALL_CISOPS(cis, cis_log_status, sensor_peri->subdev_cis);
	CALL_PREPROPOPS(preprocessor, preprocessor_debug, subdev_preprocessor);

	__fimc_is_shutdown(gPtr_lib_support.pdev);

	BUG_ON(1);
}

void fimc_is_stop_trigger(void)
{
	schedule_work(&gPtr_lib_support.work_stop);
}

void fimc_is_assert(void)
{
	BUG_ON(1);
}

/*
 * Semaphore
 */
int fimc_is_sema_init(void **sema, int sema_count)
{
	if (*sema == NULL) {
		dbg_lib("sema_init: kzalloc struct semaphore\n");
		*sema = kzalloc(sizeof(struct semaphore), GFP_KERNEL);
	}

	sema_init((struct semaphore *)*sema, sema_count);

#ifdef LIB_MEM_TRACK
	add_alloc_track(MT_TYPE_SEMA, (ulong)*sema, sizeof(struct semaphore));
#endif
	return 0;
}

int fimc_is_sema_finish(void *sema)
{
	if (sema == NULL) {
		err_lib("invalid sema");
		return -1;
	}

#ifdef LIB_MEM_TRACK
	add_free_track(MT_TYPE_SEMA, (ulong)sema);
#endif
	kfree(sema);
	return 0;
}

int fimc_is_sema_up(void *sema)
{
	if (sema == NULL) {
		err_lib("invalid sema");
		return -1;
	}

	up((struct semaphore *)sema);

	return 0;
}

int fimc_is_sema_down(void *sema)
{
	if (sema == NULL) {
		err_lib("invalid sema");
		return -1;
	}

	down((struct semaphore *)sema);

	return 0;
}

/*
 * Mutex
 */
int fimc_is_mutex_init(void **mutex)
{
	if (*mutex == NULL)
		*mutex = kzalloc(sizeof(struct mutex), GFP_KERNEL);

	mutex_init((struct mutex *)*mutex);

#ifdef LIB_MEM_TRACK
	add_alloc_track(MT_TYPE_MUTEX, (ulong)*mutex, sizeof(struct mutex));
#endif
	return 0;
}

int fimc_is_mutex_finish(void *mutex_lib)
{
	struct mutex *_mutex = NULL;

	if (mutex_lib == NULL) {
		err_lib("invalid mutex");
		return -1;
	}

	_mutex = (struct mutex *)mutex_lib;

	if (atomic_read(&_mutex->count) == 0)
		mutex_unlock(_mutex);

#ifdef LIB_MEM_TRACK
	add_free_track(MT_TYPE_MUTEX, (ulong)mutex_lib);
#endif
	kfree(mutex_lib);
	return 0;
}

int fimc_is_mutex_lock(void *mutex_lib)
{
	struct mutex *_mutex = NULL;

	if (mutex_lib == NULL) {
		err_lib("invalid mutex");
		return -1;
	}

	_mutex = (struct mutex *)mutex_lib;
	mutex_lock(_mutex);

	return 0;
}

bool fimc_is_mutex_tryLock(void *mutex_lib)
{
	struct mutex *_mutex = NULL;
	int locked = 0;

	if (mutex_lib == NULL) {
		err_lib("invalid mutex");
		return -1;
	}

	_mutex = (struct mutex *)mutex_lib;
	locked = mutex_trylock(_mutex);

	return locked == 1 ? true : false;
}

int fimc_is_mutex_unlock(void *mutex_lib)
{
	struct mutex *_mutex = NULL;

	if (mutex_lib == NULL) {
		err_lib("invalid mutex");
		return -1;
	}

	_mutex = (struct mutex *)mutex_lib;
	mutex_unlock(_mutex);

	return 0;
}

/*
 * file read/write
 */
int fimc_is_fwrite(const char *pfname, void *pdata, int size)
{
	int ret = 0;
	u32 flags = 0;
	int total_size = 0;
	struct fimc_is_binary bin;
	char *filename = NULL;

	filename = __getname();

	if (unlikely(!pfname)) {
		ret = -ENOMEM;
		goto p_err;
	}

	snprintf(filename, PATH_MAX, "%s/%s", DBG_DMA_DUMP_PATH, pfname);

	bin.data = pdata;
	bin.size = size;

	/* first plane for image */
	flags = O_TRUNC | O_CREAT | O_RDWR | O_APPEND;
	total_size += bin.size;

	ret = put_filesystem_binary(filename, &bin, flags);
	if (ret) {
		err("failed to dump %s (%d)", filename, ret);
		ret = -EINVAL;
		goto p_err;
	}

	info_lib("%s: filename = %s, total size = %d\n",
			__func__, filename, total_size);

p_err:
	__putname(filename);
	return ret;
}

/*
 * Timer
 */
int fimc_is_timer_create(void **timer, ulong expires,
				void (*func)(ulong),
				ulong data)
{
	struct timer_list *pTimer = NULL;
	if (*timer == NULL)
		*timer = kzalloc(sizeof(struct timer_list), GFP_KERNEL);

	pTimer = *timer;

	init_timer(pTimer);
	pTimer->expires = jiffies + expires;
	pTimer->data = (ulong)data;
	pTimer->function = func;

#ifdef LIB_MEM_TRACK
	add_alloc_track(MT_TYPE_TIMER, (ulong)*timer, sizeof(struct timer_list));
#endif
	return 0;
}

int fimc_is_timer_delete(void *timer)
{
	if (timer == NULL) {
		err_lib("invalid timer");
		return -1;
	}

	/* Stop timer, when timer is started */
	del_timer((struct timer_list *)timer);

#ifdef LIB_MEM_TRACK
	add_free_track(MT_TYPE_TIMER, (ulong)timer);
#endif
	kfree(timer);

	return 0;
}

int fimc_is_timer_reset(void *timer, ulong expires)
{
	if (timer == NULL) {
		err_lib("invalid timer");
		return -1;
	}

	mod_timer((struct timer_list *)timer, jiffies + expires);

	return 0;
}

int fimc_is_timer_query(void *timer, ulong timerValue)
{
	/* TODO: will be IMPLEMENTED */
	return 0;
}

int fimc_is_timer_enable(void *timer)
{
	if (timer == NULL) {
		err_lib("invalid timer");
		return -1;
	}

	add_timer((struct timer_list *)timer);

	return 0;
}

int fimc_is_timer_disable(void *timer)
{
	if (timer == NULL) {
		err_lib("invalid timer");
		return -1;
	}

	del_timer((struct timer_list *)timer);

	return 0;
}

/*
 * Interrupts
 */
#define INTR_ID_BASE_OFFSET	(3)
#define IRQ_ID_3AA0(x)		(x - (INTR_ID_BASE_OFFSET * 0))
#define IRQ_ID_3AA1(x)		(x - (INTR_ID_BASE_OFFSET * 1))
#define IRQ_ID_ISP0(x)		(x - (INTR_ID_BASE_OFFSET * 2))
#define IRQ_ID_ISP1(x)		(x - (INTR_ID_BASE_OFFSET * 3))
#define IRQ_ID_TPU0(x)		(x - (INTR_ID_BASE_OFFSET * 4))
#define IRQ_ID_TPU1(x)		(x - (INTR_ID_BASE_OFFSET * 5))
#define IRQ_ID_DCP(x)		(x - (INTR_ID_BASE_OFFSET * 6))
#define valid_3aaisp_intr_index(intr_index) \
	(0 <= intr_index && intr_index < INTR_HWIP_MAX)

int fimc_is_register_interrupt(struct hwip_intr_handler info)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct hwip_intr_handler *pHandler = NULL;
	int intr_index = -1;
	int ret = 0;

	switch (info.chain_id) {
	case ID_3AA_0:
		intr_index = IRQ_ID_3AA0(info.id);
		break;
	case ID_3AA_1:
		intr_index = IRQ_ID_3AA1(info.id);
		break;
	case ID_ISP_0:
		intr_index = IRQ_ID_ISP0(info.id);
		break;
	case ID_ISP_1:
		intr_index = IRQ_ID_ISP1(info.id);
		break;
	case ID_TPU_0:
		intr_index = IRQ_ID_TPU0(info.id);
		break;
	case ID_TPU_1:
		intr_index = IRQ_ID_TPU1(info.id);
		break;
	case ID_DCP:
		intr_index = IRQ_ID_DCP(info.id);
		break;
	default:
		err_lib("invalid chaind_id(%d)", info.chain_id);
		return -EINVAL;
		break;
	}

	if (!valid_3aaisp_intr_index(intr_index)) {
		err_lib("invalid interrupt id chain ID(%d),(%d)(%d)",
			info.chain_id, info.id, intr_index);
		return -EINVAL;
	}
	pHandler = lib->intr_handler_taaisp[info.chain_id][intr_index];

	if (!pHandler) {
		err_lib("Register interrupt error!!:chain ID(%d)(%d)",
			info.chain_id, info.id);
		return 0;
	}

	pHandler->id		= info.id;
	pHandler->priority	= info.priority;
	pHandler->ctx		= info.ctx;
	pHandler->handler	= info.handler;
	pHandler->valid		= true;
	pHandler->chain_id	= info.chain_id;

	fimc_is_log_write("[@][DRV]Register interrupt:ID(%d), handler(%p)\n",
		info.id, info.handler);

	return ret;
}

int fimc_is_unregister_interrupt(u32 intr_id, u32 chain_id)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	struct hwip_intr_handler *pHandler;
	int intr_index = -1;
	int ret = 0;

	switch (chain_id) {
	case ID_3AA_0:
		intr_index = IRQ_ID_3AA0(intr_id);
		break;
	case ID_3AA_1:
		intr_index = IRQ_ID_3AA1(intr_id);
		break;
	case ID_ISP_0:
		intr_index = IRQ_ID_ISP0(intr_id);
		break;
	case ID_ISP_1:
		intr_index = IRQ_ID_ISP1(intr_id);
		break;
	case ID_TPU_0:
		intr_index = IRQ_ID_TPU0(intr_id);
		break;
	case ID_TPU_1:
		intr_index = IRQ_ID_TPU1(intr_id);
		break;
	case ID_DCP:
		intr_index = IRQ_ID_DCP(intr_id);
		break;
	default:
		err_lib("invalid chaind_id(%d)", chain_id);
		return 0;
		break;
	}

	if (!valid_3aaisp_intr_index(intr_index)) {
		err_lib("invalid interrupt id chain ID(%d),(%d)(%d)",
			chain_id, intr_id, intr_index);
		return -EINVAL;
	}

	pHandler = lib->intr_handler_taaisp[chain_id][intr_index];

	pHandler->id		= 0;
	pHandler->priority	= 0;
	pHandler->ctx		= NULL;
	pHandler->handler	= NULL;
	pHandler->valid		= false;
	pHandler->chain_id	= 0;

	fimc_is_log_write("[@][DRV]Unregister interrupt:ID(%d)\n", intr_id);

	return ret;
}

static irqreturn_t  fimc_is_general_interrupt_isr (int irq, void *data)
{
	struct general_intr_handler *intr_handler = (struct general_intr_handler *)data;

#ifdef ENABLE_FPSIMD_FOR_USER
	fpsimd_get();
	intr_handler->intr_func();
	fpsimd_put();
#else
	intr_handler->intr_func();
#endif

	return IRQ_HANDLED;
}

int fimc_is_register_general_interrupt(struct general_intr_handler info)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	int ret = 0;

	switch (info.id) {
	case ID_GENERAL_INTR_PREPROC_PDAF:
		lib->intr_handler_preproc.intr_func = info.intr_func;

		/* Request IRQ */
		ret = request_threaded_irq(lib->intr_handler_preproc.irq, NULL, fimc_is_general_interrupt_isr,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT, "preproc-pdaf-irq", &lib->intr_handler_preproc);
		if (ret) {
			err("Failed to register pdaf irq %d", lib->intr_handler_preproc.irq);
			ret = -ENODEV;
		}
		break;
	default:
		err_lib("invalid general_interrupt id(%d)", info.id);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int fimc_is_unregister_general_interrupt(struct general_intr_handler info)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;

	switch (info.id) {
	case ID_GENERAL_INTR_PREPROC_PDAF:
		free_irq(lib->intr_handler_preproc.irq, &lib->intr_handler_preproc);
		break;
	default:
		err_lib("invalid general_interrupt id(%d)", info.id);
		break;
	}

	return 0;
}


int fimc_is_enable_interrupt(u32 interrupt_id)
{
	/* TODO */
	return 0;
}

int fimc_is_disable_interrupt(u32 interrupt_id)
{
	/* TODO */
	return 0;
}

int fimc_is_clear_interrupt(u32 interrupt_id)
{
	/* TODO */
	return 0;
}

/*
 * Spinlock
 */
static spinlock_t svc_slock;
ulong fimc_is_svc_spin_lock_save(void)
{
	ulong flags = 0;

	spin_lock_irqsave(&svc_slock, flags);

	return flags;
}

void fimc_is_svc_spin_unlock_restore(ulong flags)
{
	spin_unlock_irqrestore(&svc_slock, flags);
}

int fimc_is_spin_lock_init(void **slock)
{
	if (*slock == NULL)
		*slock = kzalloc(sizeof(spinlock_t), GFP_KERNEL);

	spin_lock_init((spinlock_t *)*slock);

#ifdef LIB_MEM_TRACK
	add_alloc_track(MT_TYPE_SPINLOCK, (ulong)*slock, sizeof(spinlock_t));
#endif

	return 0;
}

int fimc_is_spin_lock_finish(void *slock_lib)
{
	spinlock_t *slock = NULL;

	if (slock_lib == NULL) {
		err_lib("invalid spinlock");
		return -EINVAL;
	}

	slock = (spinlock_t *)slock_lib;

#ifdef LIB_MEM_TRACK
	add_free_track(MT_TYPE_SPINLOCK, (ulong)slock_lib);
#endif
	kfree(slock_lib);

	return 0;
}

int fimc_is_spin_lock(void *slock_lib)
{
	spinlock_t *slock = NULL;

	BUG_ON(!slock_lib);

	slock = (spinlock_t *)slock_lib;
	spin_lock(slock);

	return 0;
}

int fimc_is_spin_unlock(void *slock_lib)
{
	spinlock_t *slock = NULL;

	BUG_ON(!slock_lib);

	slock = (spinlock_t *)slock_lib;
	spin_unlock(slock);

	return 0;
}

int fimc_is_spin_lock_irq(void *slock_lib)
{
	spinlock_t *slock = NULL;

	BUG_ON(!slock_lib);

	slock = (spinlock_t *)slock_lib;
	spin_lock_irq(slock);

	return 0;
}

int fimc_is_spin_unlock_irq(void *slock_lib)
{
	spinlock_t *slock = NULL;

	BUG_ON(!slock_lib);

	slock = (spinlock_t *)slock_lib;
	spin_unlock_irq(slock);

	return 0;
}

ulong fimc_is_spin_lock_irqsave(void *slock_lib)
{
	spinlock_t *slock = NULL;
	ulong flags = 0;

	BUG_ON(!slock_lib);

	slock = (spinlock_t *)slock_lib;
	spin_lock_irqsave(slock, flags);

	return flags;
}

int fimc_is_spin_unlock_irqrestore(void *slock_lib, ulong flag)
{
	spinlock_t *slock = NULL;

	BUG_ON(!slock_lib);

	slock = (spinlock_t *)slock_lib;
	spin_unlock_irqrestore(slock, flag);

	return 0;
}

int fimc_is_get_nsec(uint64_t *time)
{
	*time = (uint64_t)local_clock();

	return 0;
}

int fimc_is_get_usec(uint64_t *time)
{
	unsigned long long t;

	t = local_clock();
	*time = (uint64_t)(t / NSEC_PER_USEC);

	return 0;
}

/*
 * Sleep
 */
void fimc_is_sleep(u32 msec)
{
	msleep(msec);
}

void fimc_is_usleep(ulong usec)
{
	usleep_range(usec, usec);
}

void fimc_is_udelay(ulong usec)
{
	udelay(usec);
}

void fimc_is_taaisp_cache_invalid(ulong kvaddr, u32 size)
{
	CALL_BUFOP(gPtr_lib_support.minfo->pb_taaisp, sync_for_cpu,
		gPtr_lib_support.minfo->pb_taaisp,
		(kvaddr - gPtr_lib_support.minfo->kvaddr_taaisp),
		size,
		DMA_FROM_DEVICE);
}

void fimc_is_taaisp_cache_flush(ulong kvaddr, u32 size)
{
	CALL_BUFOP(gPtr_lib_support.minfo->pb_taaisp, sync_for_device,
		gPtr_lib_support.minfo->pb_taaisp,
		(kvaddr - gPtr_lib_support.minfo->kvaddr_taaisp),
		size,
		DMA_TO_DEVICE);
}

void fimc_is_tpu_cache_invalid(ulong kvaddr, u32 size)
{
	CALL_BUFOP(gPtr_lib_support.minfo->pb_tpu, sync_for_cpu,
		gPtr_lib_support.minfo->pb_tpu,
		(kvaddr - gPtr_lib_support.minfo->kvaddr_tpu),
		size,
		DMA_FROM_DEVICE);
}

void fimc_is_tpu_cache_flush(ulong kvaddr, u32 size)
{
	CALL_BUFOP(gPtr_lib_support.minfo->pb_tpu, sync_for_device,
		gPtr_lib_support.minfo->pb_tpu,
		(kvaddr - gPtr_lib_support.minfo->kvaddr_tpu),
		size,
		DMA_TO_DEVICE);
}

void fimc_is_vra_cache_invalid(ulong kvaddr, u32 size)
{
	CALL_BUFOP(gPtr_lib_support.minfo->pb_vra, sync_for_cpu,
		gPtr_lib_support.minfo->pb_vra,
		(kvaddr - gPtr_lib_support.minfo->kvaddr_vra),
		size,
		DMA_FROM_DEVICE);
}

void fimc_is_vra_cache_flush(ulong kvaddr, u32 size)
{
	CALL_BUFOP(gPtr_lib_support.minfo->pb_vra, sync_for_device,
		gPtr_lib_support.minfo->pb_vra,
		(kvaddr - gPtr_lib_support.minfo->kvaddr_vra),
		size,
		DMA_TO_DEVICE);
}

ulong get_reg_addr(u32 id)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	ulong reg_addr = 0;
	int hw_id, hw_slot;

	switch(id) {
	case ID_3AA_0:
		hw_id = DEV_HW_3AA0;
		break;
	case ID_3AA_1:
		hw_id = DEV_HW_3AA1;
		break;
	case ID_ISP_0:
		hw_id = DEV_HW_ISP0;
		break;
	case ID_ISP_1:
		hw_id = DEV_HW_ISP1;
		break;
	case ID_TPU_0:
		hw_id = DEV_HW_TPU0;
		break;
	case ID_TPU_1:
		hw_id = DEV_HW_TPU1;
		break;
	case ID_DCP:
		hw_id = DEV_HW_DCP;
		break;
	default:
		warn_lib("get_reg_addr: invalid id(%d)\n", id);
		return 0;
		break;
	}

	hw_slot = fimc_is_hw_slot_id(hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_lib("invalid hw_slot (%d) ", hw_slot);
		return 0;
	}

	reg_addr = (ulong)lib->itfc->itf_ip[hw_slot].hw_ip->regs;

	info_lib("get_reg_addr: [ID:%d](0x%lx)\n", hw_id, reg_addr);

	return reg_addr;
}

static void lib_task_work(struct kthread_work *work)
{
	struct fimc_is_task_work *cur_work;

	BUG_ON(!work);

	cur_work = container_of(work, struct fimc_is_task_work, work);

	dbg_lib("do task_work: func(%p), params(%p)\n",
		cur_work->func, cur_work->params);

	cur_work->func(cur_work->params);
}

bool lib_task_trigger(struct fimc_is_lib_support *this,
	int priority, void *func, void *params)
{
	struct fimc_is_lib_task *lib_task = NULL;
	u32 index = 0;

	BUG_ON(!this);
	BUG_ON(!func);
	BUG_ON(!params);

	lib_task = &this->task_taaisp[(priority - TASK_PRIORITY_BASE - 1)];
	spin_lock(&lib_task->work_lock);
	lib_task->work[lib_task->work_index % LIB_MAX_TASK].func = (task_func)func;
	lib_task->work[lib_task->work_index % LIB_MAX_TASK].params = params;
	lib_task->work_index++;
	index = (lib_task->work_index - 1) % LIB_MAX_TASK;
	spin_unlock(&lib_task->work_lock);

	return queue_kthread_work(&lib_task->worker, &lib_task->work[index].work);
}

int fimc_is_lib_check_priority(u32 type, int priority)
{
	switch (type) {
	case TASK_TYPE_DDK:
		if (priority < TASK_PRIORITY_1ST || TASK_PRIORITY_5TH < priority) {
			panic("%s: [DDK] task priority is invalid", __func__);
			return -EINVAL;
		}
		break;
	case TASK_TYPE_RTA:
		if (priority != TASK_PRIORITY_6TH) {
			panic("%s: [RTA] task priority is invalid", __func__);
			return -EINVAL;
		}
		break;
	}

	return 0;
}

bool fimc_is_add_task(int priority, void *func, void *param)
{
	BUG_ON(!func);

	dbg_lib("add_task: priority(%d), func(%p), params(%p)\n",
		priority, func, param);

	fimc_is_lib_check_priority(TASK_TYPE_DDK, priority);

	return lib_task_trigger(&gPtr_lib_support, priority, func, param);
}

bool fimc_is_add_task_rta(int priority, void *func, void *param)
{
	BUG_ON(!func);

	dbg_lib("add_task_rta: priority(%d), func(%p), params(%p)\n",
		priority, func, param);

	fimc_is_lib_check_priority(TASK_TYPE_RTA, priority);

	return lib_task_trigger(&gPtr_lib_support, priority, func, param);
}

int lib_get_task_priority(int task_index)
{
	int priority = FIMC_IS_MAX_PRIO - 1;

	switch (task_index) {
	case TASK_OTF:
		priority = TASK_OTF_PRIORITY;
		break;
	case TASK_AF:
		priority = TASK_AF_PRIORITY;
		break;
	case TASK_ISP_DMA:
		priority = TASK_ISP_DMA_PRIORITY;
		break;
	case TASK_3AA_DMA:
		priority = TASK_3AA_DMA_PRIORITY;
		break;
	case TASK_AA:
		priority = TASK_AA_PRIORITY;
		break;
	case TASK_RTA:
		priority = TASK_RTA_PRIORITY;
		break;
	default:
		err_lib("Invalid task ID (%d)", task_index);
		break;
	}
	return priority;
}

u32 lib_get_task_affinity(int task_index)
{
	u32 cpu = 0;

	switch (task_index) {
	case TASK_OTF:
		cpu = TASK_OTF_AFFINITY;
		break;
	case TASK_AF:
		cpu = TASK_AF_AFFINITY;
		break;
	case TASK_ISP_DMA:
		cpu = TASK_ISP_DMA_AFFINITY;
		break;
	case TASK_3AA_DMA:
		cpu = TASK_3AA_DMA_AFFINITY;
		break;
	case TASK_AA:
		cpu = TASK_AA_AFFINITY;
		break;
	default:
		err_lib("Invalid task ID (%d)", task_index);
		break;
	}
	return cpu;
}

int lib_support_init(void)
{
	int ret = 0;
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
#ifdef LIB_MEM_TRACK

	INIT_LIST_HEAD(&lib->list_of_tracks);
	lib->cur_tracks = kzalloc(sizeof(struct lib_mem_tracks), GFP_KERNEL);
	if (lib->cur_tracks)
		list_add(&lib->cur_tracks->list,
			&lib->list_of_tracks);
	else
		err_lib("failed to allocate memory track");
#endif

	INIT_WORK(&lib->work_stop, fimc_is_dump_n_stop);

	return ret;
}

int fimc_is_init_ddk_thread(void)
{
	struct sched_param param = { .sched_priority = FIMC_IS_MAX_PRIO - 1 };
	char name[30];
	int i, j, ret = 0;
#ifdef SET_CPU_AFFINITY
	u32 cpu = 0;
#endif

	struct fimc_is_lib_support *lib = &gPtr_lib_support;

	for (i = 0 ; i < TASK_INDEX_MAX; i++) {
		spin_lock_init(&lib->task_taaisp[i].work_lock);
		init_kthread_worker(&lib->task_taaisp[i].worker);
		snprintf(name, sizeof(name), "lib_%d_worker", i);
		lib->task_taaisp[i].task = kthread_run(kthread_worker_fn,
							&lib->task_taaisp[i].worker,
							name);
		if (IS_ERR(lib->task_taaisp[i].task)) {
			err_lib("failed to create library task_handler(%d), err(%ld)",
				i, PTR_ERR(lib->task_taaisp[i].task));
			return PTR_ERR(lib->task_taaisp[i].task);
		}
#ifdef ENABLE_FPSIMD_FOR_USER
		fpsimd_set_task_using(lib->task_taaisp[i].task);
#endif
		/* TODO: consider task priority group worker */
		param.sched_priority = lib_get_task_priority(i);
		ret = sched_setscheduler_nocheck(lib->task_taaisp[i].task, SCHED_FIFO, &param);
		if (ret) {
			err_lib("sched_setscheduler_nocheck(%d) is fail(%d)", i, ret);
			return 0;
		}

		lib->task_taaisp[i].work_index = 0;
		for (j = 0; j < LIB_MAX_TASK; j++) {
			lib->task_taaisp[i].work[j].func = NULL;
			lib->task_taaisp[i].work[j].params = NULL;
			init_kthread_work(&lib->task_taaisp[i].work[j].work,
					lib_task_work);
		}

		if (i != TASK_RTA) {
#ifdef SET_CPU_AFFINITY
			cpu = lib_get_task_affinity(i);
			ret = set_cpus_allowed_ptr(lib->task_taaisp[i].task, cpumask_of(cpu));
			dbg_lib("%s: task(%d) affinity cpu(%d) (%d)\n", __func__, i, cpu, ret);
#endif
		}
	}

	return ret;
}

void fimc_is_flush_ddk_thread(void)
{
	int ret = 0;
	int i = 0;
	struct fimc_is_lib_support *lib = &gPtr_lib_support;

	info_lib("%s\n", __func__);

	for (i = 0; i < TASK_INDEX_MAX; i++) {
		if (lib->task_taaisp[i].task) {
			dbg_lib("flush_ddk_thread: flush_kthread_worker (%d)\n", i);
			flush_kthread_worker(&lib->task_taaisp[i].worker);

			ret = kthread_stop(lib->task_taaisp[i].task);
			if (ret) {
				err_lib("kthread_stop fail (%d): state(%ld), exit_code(%d), exit_state(%d)",
					ret,
					lib->task_taaisp[i].task->state,
					lib->task_taaisp[i].task->exit_code,
					lib->task_taaisp[i].task->exit_state);
			}

			lib->task_taaisp[i].task = NULL;

			info_lib("flush_ddk_thread: kthread_stop [%d]\n", i);
		}
	}
}

void fimc_is_lib_flush_task_handler(int priority)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;
	int task_index = priority - TASK_PRIORITY_BASE - 1;

	dbg_lib("%s: task_index(%d), priority(%d)\n", __func__, task_index, priority);

	flush_kthread_worker(&lib->task_taaisp[task_index].worker);
}

void fimc_is_load_clear(void)
{
	struct fimc_is_lib_support *lib = &gPtr_lib_support;

	dbg_lib("binary clear start");

	lib->binary_load_flg = false;

	dbg_lib("binary clear done");

	return;
}

void check_lib_memory_leak(void)
{
#ifdef LIB_MEM_TRACK
	check_tracks(MT_STATUS_ALLOC);
	free_tracks();
#endif
}

bool fimc_is_lib_in_interrupt(void)
{
	if (in_interrupt())
		return true;
	else
		return false;
}

void set_os_system_funcs(os_system_func_t *funcs)
{
	funcs[0] = (os_system_func_t)fimc_is_log_write_console;

	funcs[1] = (os_system_func_t)fimc_is_alloc_reserved_buffer;
	funcs[2] = (os_system_func_t)fimc_is_free_reserved_buffer;

	funcs[3] = (os_system_func_t)fimc_is_assert;

	funcs[4] = (os_system_func_t)fimc_is_sema_init;
	funcs[5] = (os_system_func_t)fimc_is_sema_finish;
	funcs[6] = (os_system_func_t)fimc_is_sema_up;
	funcs[7] = (os_system_func_t)fimc_is_sema_down;

	funcs[8] = (os_system_func_t)fimc_is_mutex_init;
	funcs[9] = (os_system_func_t)fimc_is_mutex_finish;
	funcs[10] = (os_system_func_t)fimc_is_mutex_lock;
	funcs[11] = (os_system_func_t)fimc_is_mutex_tryLock;
	funcs[12] = (os_system_func_t)fimc_is_mutex_unlock;

	funcs[13] = (os_system_func_t)fimc_is_timer_create;
	funcs[14] = (os_system_func_t)fimc_is_timer_delete;
	funcs[15] = (os_system_func_t)fimc_is_timer_reset;
	funcs[16] = (os_system_func_t)fimc_is_timer_query;
	funcs[17] = (os_system_func_t)fimc_is_timer_enable;
	funcs[18] = (os_system_func_t)fimc_is_timer_disable;

	funcs[19] = (os_system_func_t)fimc_is_register_interrupt;
	funcs[20] = (os_system_func_t)fimc_is_unregister_interrupt;
	funcs[21] = (os_system_func_t)fimc_is_enable_interrupt;
	funcs[22] = (os_system_func_t)fimc_is_disable_interrupt;
	funcs[23] = (os_system_func_t)fimc_is_clear_interrupt;

	funcs[24] = (os_system_func_t)fimc_is_svc_spin_lock_save;
	funcs[25] = (os_system_func_t)fimc_is_svc_spin_unlock_restore;
	funcs[26] = (os_system_func_t)get_random_int;
	funcs[27] = (os_system_func_t)fimc_is_add_task;

	funcs[28] = (os_system_func_t)fimc_is_get_usec;
	funcs[29] = (os_system_func_t)fimc_is_log_write;

	funcs[30] = (os_system_func_t)fimc_is_translate_taaisp_kva_to_dva;
	funcs[31] = (os_system_func_t)fimc_is_translate_taaisp_dva_to_kva;
	funcs[32] = (os_system_func_t)fimc_is_sleep;
	funcs[33] = (os_system_func_t)fimc_is_taaisp_cache_invalid;
	funcs[34] = (os_system_func_t)fimc_is_alloc_reserved_taaisp_dma_buffer;
	funcs[35] = (os_system_func_t)fimc_is_free_reserved_taaisp_dma_buffer;

	funcs[36] = (os_system_func_t)fimc_is_spin_lock_init;
	funcs[37] = (os_system_func_t)fimc_is_spin_lock_finish;
	funcs[38] = (os_system_func_t)fimc_is_spin_lock;
	funcs[39] = (os_system_func_t)fimc_is_spin_unlock;
	funcs[40] = (os_system_func_t)fimc_is_spin_lock_irq;
	funcs[41] = (os_system_func_t)fimc_is_spin_unlock_irq;
	funcs[42] = (os_system_func_t)fimc_is_spin_lock_irqsave;
	funcs[43] = (os_system_func_t)fimc_is_spin_unlock_irqrestore;
	funcs[44] = (os_system_func_t)fimc_is_alloc_reserved_buffer;
	funcs[45] = (os_system_func_t)fimc_is_free_reserved_buffer;
	funcs[46] = (os_system_func_t)get_reg_addr;

	funcs[47] = (os_system_func_t)fimc_is_lib_in_interrupt;
	funcs[48] = (os_system_func_t)fimc_is_lib_flush_task_handler;
}

#ifdef USE_RTA_BINARY
void fimc_is_get_version(char **ddk, char **rta, char **setfile, char **companion)
{
	char *p;

	if (ddk) {
		*ddk = fimc_is_ischain_get_version(FIMC_IS_BIN_DDK_LIBRARY);
		p = strrchr(*ddk, ']');
		if (p)
			*ddk = p+1;
	}
	if (rta) {
		*rta = fimc_is_ischain_get_version(FIMC_IS_BIN_RTA_LIBRARY);
		p = strrchr(*rta, ']');
		if (p)
			*rta = p+1;
	}
	if (setfile)
		*setfile = fimc_is_ischain_get_version(FIMC_IS_BIN_SETFILE);
	if (companion)
		*companion = fimc_is_ischain_get_version(FIMC_IS_BIN_COMPANION);

	if (ddk && rta && setfile && companion)
		info("%s ddk:%s rta:%s setfile:%s companion:%s", __func__, *ddk, *rta, *setfile, *companion);
}

void set_os_system_funcs_for_rta(os_system_func_t *funcs)
{
	/* Index 0 => log, assert */
	funcs[0] = (os_system_func_t)fimc_is_log_write;
	funcs[1] = (os_system_func_t)fimc_is_log_write_console;
	funcs[2] = (os_system_func_t)fimc_is_assert;

#if defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	funcs[9] = NULL;
#else
	funcs[9] = (os_system_func_t)fimc_is_fwrite;
#endif

	/* Index 10 => memory : alloc/free */
	funcs[10] = (os_system_func_t)fimc_is_alloc_reserved_buffer;
	funcs[11] = (os_system_func_t)fimc_is_free_reserved_buffer;

	/* Index 20 => memory : misc */

	/* Index 30 => interrupt */
	funcs[30] = (os_system_func_t)fimc_is_register_general_interrupt;
	funcs[31] = (os_system_func_t)fimc_is_unregister_general_interrupt;

	/* Index 40 => task */
	funcs[40] = (os_system_func_t)fimc_is_add_task_rta;

	/* Index 50 => timer, delay, sleep */
	funcs[50] = (os_system_func_t)fimc_is_timer_create;
	funcs[51] = (os_system_func_t)fimc_is_timer_delete;
	funcs[52] = (os_system_func_t)fimc_is_timer_reset;
	funcs[53] = (os_system_func_t)fimc_is_timer_query;
	funcs[54] = (os_system_func_t)fimc_is_timer_enable;
	funcs[55] = (os_system_func_t)fimc_is_timer_disable;
	funcs[56] = (os_system_func_t)fimc_is_udelay;
	funcs[57] = (os_system_func_t)fimc_is_usleep;
	funcs[58] = (os_system_func_t)fimc_is_sleep;

	/* Index 60 => locking : semaphore */
	funcs[60] = (os_system_func_t)fimc_is_sema_init;
	funcs[61] = (os_system_func_t)fimc_is_sema_finish;
	funcs[62] = (os_system_func_t)fimc_is_sema_up;
	funcs[63] = (os_system_func_t)fimc_is_sema_down;

	/* Index 70 => locking : mutex */

	/* Index 80 => locking : spinlock */

	/* Index 90 => misc */
	funcs[90] = (os_system_func_t)fimc_is_get_usec;
	funcs[91] = (os_system_func_t)fimc_is_get_version;
}
#endif

static int fimc_is_memory_page_range(pte_t *ptep, pgtable_t token, unsigned long addr,
			void *data)
{
	struct fimc_is_memory_change_data *cdata = data;
	pte_t pte = *ptep;

	pte = clear_pte_bit(pte, cdata->clear_mask);
	pte = set_pte_bit(pte, cdata->set_mask);

	set_pte(ptep, pte);
	return 0;
}

static int fimc_is_memory_attribute_change(ulong addr, int numpages,
		pgprot_t set_mask, pgprot_t clear_mask)
{
	ulong start = addr;
	ulong size = PAGE_SIZE * numpages;
	ulong end = start + size;
	int ret;
	struct fimc_is_memory_change_data data;

	if (!IS_ALIGNED(addr, PAGE_SIZE)) {
		start &= PAGE_MASK;
		end = start + size;
		WARN_ON_ONCE(1);
	}

	data.set_mask = set_mask;
	data.clear_mask = clear_mask;

	ret = apply_to_page_range(&init_mm, start, size, fimc_is_memory_page_range,
					&data);

	flush_tlb_kernel_range(start, end);

	return ret;
}

int fimc_is_memory_attribute_nxrw(struct fimc_is_memory_attribute *attribute)
{
	int ret;

	if (attribute->state != PTE_RDONLY) {
		err_lib("memory attribute state is wrong!!");
		return -EINVAL;
	}

	ret = fimc_is_memory_attribute_change(attribute->vaddr,
			attribute->numpages,
			__pgprot(PTE_PXN),
			__pgprot(0));
	if (ret) {
		err_lib("memory attribute change fail [PTE_PX -> PTE_PXN]\n");
		return ret;
	}

	ret = fimc_is_memory_attribute_change(attribute->vaddr,
			attribute->numpages,
			__pgprot(PTE_WRITE),
			__pgprot(PTE_RDONLY));
	if (ret) {
		err_lib("memory attribute change fail [PTE_RDONLY -> PTE_WRITE]\n");
		goto memory_excutable;
	}

	attribute->state = PTE_WRITE;

	return 0;

memory_excutable:
	fimc_is_memory_attribute_change(attribute->vaddr,
			attribute->numpages,
			__pgprot(0),
			__pgprot(PTE_PXN));

	return -ENOMEM;
}

int fimc_is_memory_attribute_rox(struct fimc_is_memory_attribute *attribute)
{
	int ret;

	if (attribute->state != PTE_WRITE) {
		err_lib("memory attribute state is wrong!!");
		return -EINVAL;
	}

	ret = fimc_is_memory_attribute_change(attribute->vaddr,
			attribute->numpages,
			__pgprot(PTE_RDONLY),
			__pgprot(PTE_WRITE));
	if (ret) {
		err_lib("memory attribute change fail [PTE_WRITE -> PTE_RDONLY]\n");
		return ret;
	}

	ret = fimc_is_memory_attribute_change(attribute->vaddr,
			attribute->numpages,
			__pgprot(0),
			__pgprot(PTE_PXN));
	if (ret) {
		err_lib("memory attribute change fail [PTE_PXN -> PTE_PX]\n");
		return ret;
	}

	attribute->state = PTE_RDONLY;

	return 0;
}

#define INDEX_VRA_BIN	0
#define INDEX_ISP_BIN	1
int fimc_is_load_ddk_bin(int loadType)
{
	int ret = 0;
	char bin_type[4] = {0};
	struct fimc_is_binary bin;
	os_system_func_t os_system_funcs[100];
	struct device *device = &gPtr_lib_support.pdev->dev;
	/* fixup the memory attribute for every region */
	ulong lib_addr;
	ulong lib_isp = DDK_LIB_ADDR;
#if defined(CONFIG_RKP)
	unsigned int rkp_result = 0;
#endif

#if !defined(CONFIG_RKP)
	ulong lib_vra = VRA_LIB_ADDR;
	struct fimc_is_memory_attribute memory_attribute[] = {
		{PTE_RDONLY, PFN_UP(LIB_VRA_CODE_SIZE), lib_vra},
		{PTE_RDONLY, PFN_UP(LIB_ISP_CODE_SIZE), lib_isp}
	};
#endif

#ifdef USE_ONE_BINARY
	lib_addr = LIB_START;
	snprintf(bin_type, sizeof(bin_type), "DDK");
#else
	lib_addr = lib_isp;
	snprintf(bin_type, sizeof(bin_type), "ISP");
#endif

#if !defined(CONFIG_RKP)
	/* load DDK library */
	ret = fimc_is_memory_attribute_nxrw(&memory_attribute[INDEX_ISP_BIN]);
	if (ret) {
		err_lib("failed to change into NX memory attribute (%d)", ret);
		return ret;
	}

#ifdef USE_ONE_BINARY
	ret = fimc_is_memory_attribute_nxrw(&memory_attribute[INDEX_VRA_BIN]);
	if (ret) {
		err_lib("failed to change into NX memory attribute (%d)", ret);
		return ret;
	}
#endif
#endif

	setup_binary_loader(&bin, 3, -EAGAIN, NULL, NULL);
#ifdef CAMERA_FW_LOADING_FROM
	ret = fimc_is_vender_request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH, FIMC_IS_FW_DUMP_PATH,
						gPtr_lib_support.fw_name, device);
#else
	ret = request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH,
						gPtr_lib_support.fw_name, device);
#endif
	if (ret) {
		err_lib("failed to load ISP library (%d)", ret);
		return ret;
	}

	if (loadType == BINARY_LOAD_ALL) {
		info_lib("binary info[%s] - type: C/D, from: %s\n",
			bin_type,
			was_loaded_by(&bin) ? "built-in" : "user-provided");
		if (bin.size <= DDK_LIB_SIZE) {
			memcpy((void *)lib_addr, bin.data, bin.size);
			__flush_dcache_area((void *)lib_addr, bin.size);
#if defined(CONFIG_RKP)
			//if (!(gPtr_lib_support.binary_code_load_flg & BINARY_LOAD_DDK_DONE)) {
				flush_cache_all();
				rkp_call(RKP_FIMC_VERIFY,
						(u64)page_to_phys(vmalloc_to_page((void *)lib_addr)),
						(u64)bin.size, 1, 0, (u64)virt_to_phys(&rkp_result));
				if (rkp_result != RKP_FIMC_SUCCESS) {
					gPtr_lib_support.binary_load_fatal = rkp_result;
					err_lib("DDK verification failed %x", rkp_result);
					ret = -EBADF;
					goto fail;
				}
			//}
#endif
		} else {
			err_lib("DDK bin size is bigger than memory area. %d[%d]", (unsigned int)bin.size, (unsigned int)DDK_LIB_SIZE);
			ret = -EBADF;
			goto fail;
		}
	} else { /* loadType == BINARY_LOAD_DATA */
		if ((CAMERA_BINARY_DDK_DATA_OFFSET < bin.size) && (bin.size <= DDK_LIB_SIZE)) {
			info_lib("binary info[%s] - type: D, from: %s\n",
				bin_type,
				was_loaded_by(&bin) ? "built-in" : "user-provided");
			memcpy((void *)lib_addr + CAMERA_BINARY_VRA_DATA_OFFSET,
				bin.data + CAMERA_BINARY_VRA_DATA_OFFSET,
				CAMERA_BINARY_VRA_DATA_SIZE);
			info_lib("binary info[%s] - type: D, from: %s\n",
				bin_type,
				was_loaded_by(&bin) ? "built-in" : "user-provided");
			memcpy((void *)lib_addr + CAMERA_BINARY_DDK_DATA_OFFSET,
				bin.data + CAMERA_BINARY_DDK_DATA_OFFSET,
				(bin.size - CAMERA_BINARY_DDK_DATA_OFFSET));
		} else {
			err_lib("DDK bin size is bigger than memory area. %d[%d]", (unsigned int)bin.size, (unsigned int)DDK_LIB_SIZE);
			ret = -EBADF;
			goto fail;
		}
	}

	fimc_is_ischain_version(FIMC_IS_BIN_DDK_LIBRARY, bin.data, bin.size);
	release_binary(&bin);

#if !defined(CONFIG_RKP)
	ret = fimc_is_memory_attribute_rox(&memory_attribute[INDEX_ISP_BIN]);
	if (ret) {
		err_lib("failed to change into EX memory attribute (%d)", ret);
		return ret;
	}

#ifdef USE_ONE_BINARY
	ret = fimc_is_memory_attribute_rox(&memory_attribute[INDEX_VRA_BIN]);
	if (ret) {
		err_lib("failed to change into EX memory attribute (%d)", ret);
		return ret;
	}
#endif
#endif

	set_os_system_funcs(os_system_funcs);
	/* call start_up function for DDK binary */
#ifdef ENABLE_FPSIMD_FOR_USER
	fpsimd_get();
	((start_up_func_t)lib_isp)((void **)os_system_funcs);
	fpsimd_put();
#else
	((start_up_func_t)lib_isp)((void **)os_system_funcs);
#endif
	return 0;

fail:
	fimc_is_ischain_version(FIMC_IS_BIN_DDK_LIBRARY, bin.data, bin.size);
	release_binary(&bin);
	return ret;
}

int fimc_is_load_vra_bin(int loadType)
{
#ifdef USE_ONE_BINARY
	/* VRA binary loading was included in ddk binary */
#else
	int ret = 0;
	struct fimc_is_binary bin;
	struct device *device = &gPtr_lib_support.pdev->dev;
	/* fixup the memory attribute for every region */
	ulong lib_isp = DDK_LIB_ADDR;
	ulong lib_vra = VRA_LIB_ADDR;
#if !defined(CONFIG_RKP)
	struct fimc_is_memory_attribute memory_attribute[] = {
		{PTE_RDONLY, PFN_UP(LIB_VRA_CODE_SIZE), lib_vra},
		{PTE_RDONLY, PFN_UP(LIB_ISP_CODE_SIZE), lib_isp}
	};
#endif

	/* load VRA library */
#if !defined(CONFIG_RKP)
	ret = fimc_is_memory_attribute_nxrw(&memory_attribute[INDEX_VRA_BIN]);
	if (ret) {
		err_lib("failed to change into NX memory attribute (%d)", ret);
		return ret;
	}
#endif

	setup_binary_loader(&bin, 3, -EAGAIN, NULL, NULL);
	ret = request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH,
						FIMC_IS_VRA_LIB, device);
	if (ret) {
		err_lib("failed to load VRA library (%d)", ret);
		return ret;
	}
	info_lib("binary info[VRA] - type: C/D, addr: %#lx, size: 0x%lx from: %s\n",
			lib_vra, bin.size,
			was_loaded_by(&bin) ? "built-in" : "user-provided");
	if (bin.size <= VRA_LIB_SIZE) {
		memcpy((void *)lib_vra, bin.data, bin.size);
	} else {
		err_lib("VRA bin size is bigger than memory area. %d[%d]", (unsigned int)bin.size, (unsigned int)VRA_LIB_SIZE);
		goto fail;
	}
	release_binary(&bin);

#if !defined(CONFIG_RKP)
	ret = fimc_is_memory_attribute_rox(&memory_attribute[INDEX_VRA_BIN]);
	if (ret) {
		err_lib("failed to change into EX memory attribute (%d)", ret);
		return ret;
	}
#endif
#endif

	fimc_is_lib_vra_os_funcs();

	return 0;

#if !defined(USE_ONE_BINARY)
fail:
	release_binary(&bin);
	return -EBADF;
#endif
}

int fimc_is_load_rta_bin(int loadType)
{
	int ret = 0;
#ifdef USE_RTA_BINARY
	struct fimc_is_binary bin;
	struct device *device = &gPtr_lib_support.pdev->dev;

	os_system_func_t os_system_funcs[100];
	ulong lib_rta = RTA_LIB_ADDR;
#if defined(CONFIG_RKP)
	unsigned int rkp_result = 0;
#endif

#if !defined(CONFIG_RKP)
	struct fimc_is_memory_attribute rta_memory_attribute = {
		PTE_RDONLY, PFN_UP(LIB_RTA_CODE_SIZE), lib_rta};

	/* load RTA library */
	ret = fimc_is_memory_attribute_nxrw(&rta_memory_attribute);
	if (ret) {
		err_lib("failed to change into NX memory attribute (%d)", ret);
		return ret;
	}
#endif

	setup_binary_loader(&bin, 3, -EAGAIN, NULL, NULL);
#ifdef CAMERA_FW_LOADING_FROM
	ret = fimc_is_vender_request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH, FIMC_IS_FW_DUMP_PATH,
						gPtr_lib_support.rta_fw_name, device);
#else
	ret = request_binary(&bin, FIMC_IS_ISP_LIB_SDCARD_PATH,
						gPtr_lib_support.rta_fw_name, device);
#endif
	if (ret) {
		err_lib("failed to load RTA library (%d)", ret);
		return ret;
	}

	if (loadType == BINARY_LOAD_ALL) {
		info_lib("binary info[RTA] - type: C/D, from: %s\n",
			was_loaded_by(&bin) ? "built-in" : "user-provided");
		if (bin.size <= RTA_LIB_SIZE) {
			memcpy((void *)lib_rta, bin.data, bin.size);
			__flush_dcache_area((void *)lib_rta, bin.size);
#if defined(CONFIG_RKP)
			//if (!(gPtr_lib_support.binary_code_load_flg & BINARY_LOAD_RTA_DONE)) {
				flush_cache_all();
				rkp_call(RKP_FIMC_VERIFY,
					(u64)page_to_phys(vmalloc_to_page((void *)lib_rta)),
					(u64)bin.size, 2, RTA_CODE_AREA_SIZE, (u64)virt_to_phys(&rkp_result));
				if (rkp_result != RKP_FIMC_SUCCESS) {
					gPtr_lib_support.binary_load_fatal |= (rkp_result << 8);
					ret = -EBADF;
					goto fail;
				}
			//}
#endif
		} else {
			err_lib("RTA bin size is bigger than memory area. %d[%d]", (unsigned int)bin.size, (unsigned int)RTA_LIB_SIZE);
			ret = -EBADF;
			goto fail;
		}
	} else { /* loadType == BINARY_LOAD_DATA */
		if ((CAMERA_BINARY_RTA_DATA_OFFSET < bin.size) && (bin.size <= RTA_LIB_SIZE)) {
			info_lib("binary info[RTA] - type: D, from: %s\n",
				was_loaded_by(&bin) ? "built-in" : "user-provided");
			memcpy((void *)lib_rta + CAMERA_BINARY_RTA_DATA_OFFSET, bin.data + CAMERA_BINARY_RTA_DATA_OFFSET,
				(bin.size - CAMERA_BINARY_RTA_DATA_OFFSET));
		} else {
			err_lib("RTA bin size is bigger than memory area. %d[%d]", (unsigned int)bin.size, (unsigned int)RTA_LIB_SIZE);
			ret = -EBADF;
			goto fail;
		}
	}

	fimc_is_ischain_version(FIMC_IS_BIN_RTA_LIBRARY, bin.data, bin.size);
	release_binary(&bin);

#if !defined(CONFIG_RKP)
	ret = fimc_is_memory_attribute_rox(&rta_memory_attribute);
	if (ret) {
		err_lib("failed to change into EX memory attribute (%d)", ret);
		return ret;
	}
#endif

	set_os_system_funcs_for_rta(os_system_funcs);
	/* call start_up function for RTA binary */
#ifdef ENABLE_FPSIMD_FOR_USER
	fpsimd_get();
	((rta_start_up_func_t)lib_rta)(NULL, (void **)os_system_funcs);
	fpsimd_put();
#else
	((rta_start_up_func_t)lib_rta)(NULL, (void **)os_system_funcs);
#endif
#endif

	return ret;

fail:
	fimc_is_ischain_version(FIMC_IS_BIN_RTA_LIBRARY, bin.data, bin.size);
	release_binary(&bin);
	return ret;
}

int fimc_is_load_bin(void)
{
	int ret = 0;
	struct fimc_is_lib_support *lib = &gPtr_lib_support;

	info_lib("binary load start\n");

	if (lib->binary_load_flg) {
		info_lib("%s: binary is already loaded\n", __func__);
		return ret;
	}

	if (lib->binary_load_fatal) {
		err_lib("%s: binary loading is fatal error 0x%x\n", __func__, lib->binary_load_fatal);
		ret = -EBADF;
		return ret;
	}

	spin_lock_init(&lib->slock_debug);

	fimc_is_load_ctrl_lock();
#ifdef USE_TZ_CONTROLLED_MEM_ATTRIBUTE
	if (gPtr_lib_support.binary_code_load_flg & BINARY_LOAD_DDK_DONE) {
		ret = fimc_is_load_ddk_bin(BINARY_LOAD_DATA);
	} else {
		info_lib("DDK C/D binary reload start\n");
		ret = fimc_is_load_ddk_bin(BINARY_LOAD_ALL);
		if (ret == 0) {
			gPtr_lib_support.binary_code_load_flg |= BINARY_LOAD_DDK_DONE;
		}
	}
#else
	ret = fimc_is_load_ddk_bin(BINARY_LOAD_ALL);
	if (ret == 0) {
		gPtr_lib_support.binary_code_load_flg |= BINARY_LOAD_DDK_DONE;
	}
#endif
	if (ret) {
		err_lib("load_ddk_bin() failed (%d)\n", ret);
		fimc_is_load_ctrl_unlock();
		return ret;
	}

#ifdef USE_TZ_CONTROLLED_MEM_ATTRIBUTE
	ret = fimc_is_load_vra_bin(BINARY_LOAD_DATA);
#else
	ret = fimc_is_load_vra_bin(BINARY_LOAD_ALL);
#endif
	if (ret) {
		err_lib("load_vra_bin() failed (%d)\n", ret);
		fimc_is_load_ctrl_unlock();
		return ret;
	}
	lib->log_ptr = 0;

#ifdef USE_TZ_CONTROLLED_MEM_ATTRIBUTE
	if (gPtr_lib_support.binary_code_load_flg & BINARY_LOAD_RTA_DONE) {
		ret = fimc_is_load_rta_bin(BINARY_LOAD_DATA);
	} else {
		info_lib("RTA C/D binary reload start\n");
		ret = fimc_is_load_rta_bin(BINARY_LOAD_ALL);
		if (ret == 0) {
			gPtr_lib_support.binary_code_load_flg |= BINARY_LOAD_RTA_DONE;
		}
	}
#else
	ret = fimc_is_load_rta_bin(BINARY_LOAD_ALL);
	if (ret == 0) {
		gPtr_lib_support.binary_code_load_flg |= BINARY_LOAD_RTA_DONE;
	}
#endif
	if (ret) {
		err_lib("load_rta_bin() failed (%d)\n", ret);
		fimc_is_load_ctrl_unlock();
		return ret;
	}
	fimc_is_load_ctrl_unlock();

	ret = lib_support_init();
	if (ret < 0) {
		err_lib("lib_support_init failed!! (%d)", ret);
		return ret;
	}
	dbg_lib("lib_support_init success!!\n");

	spin_lock_init(&svc_slock);

	lib->binary_load_flg = true;

	/* warning for reserved memory management */
	if (lib->reserved_lib_size) {
		warn_lib("reserved_lib_size is not zero!! (%d)", lib->reserved_lib_size);
		lib->reserved_lib_size = 0;
	}

	if (lib->reserved_taaisp_size) {
		warn_lib("reserved_taaisp_size is not zero!! (%d)", lib->reserved_taaisp_size);
		lib->reserved_taaisp_size = 0;
	}
	if (lib->reserved_vra_size) {
		warn_lib("reserved_buf_size is not zero!! (%d)", lib->reserved_vra_size);
		lib->reserved_vra_size = 0;
	}

	INIT_LIST_HEAD(&lib->lib_mem_list);
	spin_lock_init(&lib->slock_lib_mem);
	INIT_LIST_HEAD(&lib->taaisp_mem_list);
	spin_lock_init(&lib->slock_taaisp_mem);
	INIT_LIST_HEAD(&lib->vra_mem_list);
	spin_lock_init(&lib->slock_vra_mem);

	info_lib("binary load done\n");

	return 0;
}

void fimc_is_load_ctrl_init(void)
{
	mutex_init(&gPtr_bin_load_ctrl);
	gPtr_lib_support.binary_code_load_flg = 0;
	gPtr_lib_support.binary_load_fatal = 0;
}

void fimc_is_load_ctrl_lock(void)
{
	mutex_lock(&gPtr_bin_load_ctrl);
}

void fimc_is_load_ctrl_unlock(void)
{
	mutex_unlock(&gPtr_bin_load_ctrl);
}

int fimc_is_load_bin_on_boot(void)
{
	int ret = 0;

	if (!(gPtr_lib_support.binary_code_load_flg & BINARY_LOAD_DDK_DONE)) {
		ret = fimc_is_load_ddk_bin(BINARY_LOAD_ALL);
		if (ret) {
			err_lib("fimc_is_load_ddk_bin is fail(%d)", ret);
			if (ret == -EBADF)
				fimc_is_vender_remove_dump_fw_file();
		} else {
			gPtr_lib_support.binary_code_load_flg |= BINARY_LOAD_DDK_DONE;
		}
	}

	if (!(gPtr_lib_support.binary_code_load_flg & BINARY_LOAD_RTA_DONE)) {
		ret = fimc_is_load_rta_bin(BINARY_LOAD_ALL);
		if (ret) {
			err_lib("fimc_is_load_rta_bin is fail(%d)", ret);
			if (ret == -EBADF)
				fimc_is_vender_remove_dump_fw_file();
		} else {
			gPtr_lib_support.binary_code_load_flg |= BINARY_LOAD_RTA_DONE;
		}
	}

	return ret;
}

int fimc_is_set_fw_names(char *fw_name, char *rta_fw_name)
{
	if (fw_name == NULL || rta_fw_name == NULL) {
		err_lib("%s %p %p fail. ", __func__, fw_name, rta_fw_name);
		return -1;
	}

	memcpy(gPtr_lib_support.fw_name, fw_name, sizeof(gPtr_lib_support.fw_name));
	memcpy(gPtr_lib_support.rta_fw_name, rta_fw_name, sizeof(gPtr_lib_support.rta_fw_name));

	return 0;
}
