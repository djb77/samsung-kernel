/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_API_COMMON_H
#define FIMC_IS_API_COMMON_H

#include <linux/vmalloc.h>

#include "fimc-is-core.h"
#include "fimc-is-debug.h"
#include "fimc-is-regs.h"
#include "fimc-is-binary.h"


#define SET_CPU_AFFINITY	/* enable @ Exynos3475 */

#if !defined(DISABLE_LIB)
/* #define LIB_MEM_TRACK */
#endif

#define LIB_ISP_OFFSET		(0x00000080)
#define LIB_ISP_CODE_SIZE	(0x00240000)

#define LIB_VRA_OFFSET		(0x00000400)
#define LIB_VRA_CODE_SIZE	(0x00040000)

#define LIB_RTA_OFFSET		(0x00000000)

#define LIB_RTA_CODE_SIZE	RTA_CODE_AREA_SIZE

#define LIB_MAX_TASK		(FIMC_IS_MAX_TASK)

#define TASK_TYPE_DDK	(1)
#define TASK_TYPE_RTA	(2)

/* depends on FIMC-IS version */
enum task_priority_magic {
	TASK_PRIORITY_BASE = 10,
	TASK_PRIORITY_1ST,	/* highest */
	TASK_PRIORITY_2ND,
	TASK_PRIORITY_3RD,
	TASK_PRIORITY_4TH,
	TASK_PRIORITY_5TH,	/* lowest */
	TASK_PRIORITY_6TH	/* for RTA */
};

/* Task index */
enum task_index {
	TASK_OTF = 0,
	TASK_AF,
	TASK_ISP_DMA,
	TASK_3AA_DMA,
	TASK_AA,
	TASK_RTA,
	TASK_INDEX_MAX
};
#define TASK_VRA		(TASK_INDEX_MAX)

/* Task priority */
#define TASK_OTF_PRIORITY		(FIMC_IS_MAX_PRIO - 2)
#define TASK_AF_PRIORITY		(FIMC_IS_MAX_PRIO - 3)
#define TASK_ISP_DMA_PRIORITY		(FIMC_IS_MAX_PRIO - 4)
#define TASK_3AA_DMA_PRIORITY		(FIMC_IS_MAX_PRIO - 5)
#define TASK_AA_PRIORITY		(FIMC_IS_MAX_PRIO - 6)
#define TASK_RTA_PRIORITY		(FIMC_IS_MAX_PRIO - 7)
#define TASK_VRA_PRIORITY		(FIMC_IS_MAX_PRIO - 4)

/* Task affinity */
#define TASK_OTF_AFFINITY		(3)
#define TASK_AF_AFFINITY		(1)
#define TASK_ISP_DMA_AFFINITY		(2)
#define TASK_3AA_DMA_AFFINITY		(TASK_ISP_DMA_AFFINITY)
#define TASK_AA_AFFINITY		(TASK_AF_AFFINITY)
/* #define TASK_RTA_AFFINITY		(1) */ /* There is no need to set of cpu affinity for RTA task */
#define TASK_VRA_AFFINITY		(2)

#define CAMERA_BINARY_RTA_DATA_OFFSET   RTA_CODE_AREA_SIZE
#define CAMERA_BINARY_DDK_DATA_OFFSET   0x380000
#define CAMERA_BINARY_DDK_CODE_OFFSET   0x080000
#define CAMERA_BINARY_VRA_DATA_OFFSET   0x040000
#define CAMERA_BINARY_VRA_DATA_SIZE     0x040000

enum BinLoadType{
    BINARY_LOAD_ALL,
    BINARY_LOAD_DATA
};

#define BINARY_LOAD_DDK_DONE		0x01
#define BINARY_LOAD_RTA_DONE		0x02
#define BINARY_LOAD_ALL_DONE	(BINARY_LOAD_DDK_DONE | BINARY_LOAD_RTA_DONE)

typedef u32 (*task_func)(void *params);

typedef u32 (*start_up_func_t)(void **func);
typedef u32 (*rta_start_up_func_t)(void *bootargs, void **func);
typedef void(*os_system_func_t)(void);

#define RTA_SHUT_DOWN_FUNC_ADDR	(RTA_LIB_ADDR + 0x100)
typedef int (*rta_shut_down_func_t)(void *data);

struct fimc_is_task_work {
	struct kthread_work		work;
	task_func			func;
	void				*params;
};

struct fimc_is_lib_task {
	struct kthread_worker		worker;
	struct task_struct		*task;
	spinlock_t			work_lock;
	u32				work_index;
	struct fimc_is_task_work	work[LIB_MAX_TASK];
};

#ifdef LIB_MEM_TRACK
#define MT_TYPE_HEAP		1
#define MT_TYPE_SEMA		2
#define MT_TYPE_MUTEX		3
#define MT_TYPE_TIMER		4
#define MT_TYPE_SPINLOCK	5
#define MT_TYPE_RESERVED	6
#define MT_TYPE_TAAISP_DMA	7
#define MT_TYPE_VRA_DMA		8


#define MT_STATUS_ALLOC	0x1
#define MT_STATUS_FREE	0x2

#define MEM_TRACK_COUNT	64
#define MEM_TRACK_ADDRS_COUNT 16

struct _lib_mem_track {
	ulong			lr;
#ifdef CONFIG_STACKTRACE
	ulong			addrs[MEM_TRACK_ADDRS_COUNT];
#endif
	int			cpu;
	int			pid;
	unsigned long long	when;
};

struct lib_mem_track {
	int			type;
	ulong			addr;
	size_t			size;
	int			status;
	struct _lib_mem_track	alloc;
	struct _lib_mem_track	free;
};

struct lib_mem_tracks {
	struct list_head	list;
	int			num_of_track;
	struct lib_mem_track	track[MEM_TRACK_COUNT];
};
#endif

struct fimc_is_lib_dma_buffer {
	u32			size;
	ulong			kvaddr;
	dma_addr_t		dvaddr;

	struct list_head	list;
};

struct general_intr_handler {
	int irq;
	int id;
	int (* intr_func)(void);
};

enum general_intr_id {
	ID_GENERAL_INTR_PREPROC_PDAF = 0,
	ID_GENERAL_INTR_MAX
};

#define SIZE_LIB_LOG_PREFIX	18	/* [%5lu.%06lu] [%d]  */
#define SIZE_LIB_LOG_BUF	256
#define MAX_LIB_LOG_BUF		(SIZE_LIB_LOG_PREFIX + SIZE_LIB_LOG_BUF)
struct fimc_is_lib_support {
	struct fimc_is_minfo			*minfo;

	struct fimc_is_interface_ischain	*itfc;
	struct hwip_intr_handler		*intr_handler_taaisp[ID_3AAISP_MAX][INTR_HWIP_MAX];
	struct fimc_is_lib_task			task_taaisp[TASK_INDEX_MAX];

	/* memory management */
	spinlock_t				slock_lib_mem;
	struct list_head			lib_mem_list;
	spinlock_t				slock_taaisp_mem;
	struct list_head			taaisp_mem_list;
	spinlock_t				slock_vra_mem;
	struct list_head			vra_mem_list;
	bool					binary_load_flg;
	int					binary_load_fatal;
	int					binary_code_load_flg;

	u32					reserved_lib_size;
	u32					reserved_taaisp_size;
	u32					reserved_vra_size;

	/* for log */
	spinlock_t				slock_debug;
	ulong					kvaddr_debug;
	ulong					kvaddr_debug_cnt;
	ulong					log_ptr;
	char					log_buf[MAX_LIB_LOG_BUF];
	char					string[MAX_LIB_LOG_BUF];

	/* for library load */
	struct platform_device			*pdev;
#ifdef LIB_MEM_TRACK
	struct list_head			list_of_tracks;
	struct lib_mem_tracks			*cur_tracks;
#endif
	struct general_intr_handler	intr_handler_preproc;
	struct work_struct			work_stop;
	char		fw_name[50];		/* DDK */
	char		rta_fw_name[50];	/* RTA */
};

struct fimc_is_memory_attribute {
	pgprot_t state;
	int numpages;
	ulong vaddr;
};

struct fimc_is_memory_change_data {
	pgprot_t set_mask;
	pgprot_t clear_mask;
};

int fimc_is_load_bin(void);
void fimc_is_load_clear(void);
int fimc_is_init_ddk_thread(void);
void fimc_is_flush_ddk_thread(void);
void check_lib_memory_leak(void);

int fimc_is_log_write(const char *str, ...);
int fimc_is_log_write_console(char *str);
int fimc_is_lib_logdump(void);

int fimc_is_spin_lock_init(void **slock);
int fimc_is_spin_lock_finish(void *slock_lib);
int fimc_is_spin_lock(void *slock_lib);
int fimc_is_spin_unlock(void *slock_lib);
int fimc_is_spin_lock_irq(void *slock_lib);
int fimc_is_spin_unlock_irq(void *slock_lib);
ulong fimc_is_spin_lock_irqsave(void *slock_lib);
int fimc_is_spin_unlock_irqrestore(void *slock_lib, ulong flag);

void *fimc_is_alloc_reserved_buffer(u32 size);
void fimc_is_free_reserved_buffer(void *buf);
void *fimc_is_alloc_reserved_taaisp_dma_buffer(u32 size);
void fimc_is_free_reserved_taaisp_dma_buffer(void *buf);
void *fimc_is_alloc_reserved_vra_dma_buffer(u32 size);
void fimc_is_free_reserved_vra_dma_buffer(void *buf);
int fimc_is_translate_taaisp_kva_to_dva(ulong src_addr, u32 *target_addr);
int fimc_is_translate_vra_kva_to_dva(ulong src_addr, u32 *target_addr);

void fimc_is_taaisp_cache_invalid(ulong kvaddr, u32 size);
void fimc_is_taaisp_cache_flush(ulong kvaddr, u32 size);
void fimc_is_tpu_cache_invalid(ulong kvaddr, u32 size);
void fimc_is_tpu_cache_flush(ulong kvaddr, u32 size);
void fimc_is_vra_cache_invalid(ulong kvaddr, u32 size);
void fimc_is_vra_cache_flush(ulong kvaddr, u32 size);

int fimc_is_memory_attribute_nxrw(struct fimc_is_memory_attribute *attribute);
int fimc_is_memory_attribute_rox(struct fimc_is_memory_attribute *attribute);

bool fimc_is_lib_in_interrupt(void);

int fimc_is_load_bin_on_boot(void);
void fimc_is_load_ctrl_unlock(void);
void fimc_is_load_ctrl_lock(void);
void fimc_is_load_ctrl_init(void);
int fimc_is_set_fw_names(char *fw_name, char *rta_fw_name);

#endif
