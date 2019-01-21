/*
 * include/linux/sec_debug_user_reset.h
 *
 * COPYRIGHT(C) 2016-2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SEC_DEBUG_USER_RESET_H
#define SEC_DEBUG_USER_RESET_H

#ifdef CONFIG_USER_RESET_DEBUG_TEST
extern void force_thermal_reset(void);
extern void force_watchdog_bark(void);
#endif

enum extra_info_dbg_type {
	DBG_0_GLAD_ERR = 0,
	DBG_1_UFS_ERR,
	DBG_2_DISPLAY_ERR,
	DBG_3_RESERVED,
	DBG_4_RESERVED,
	DBG_5_RESERVED,
	DBG_MAX,
};

#define EXINFO_KERNEL_DEFAULT_SIZE              2048
#define EXINFO_KERNEL_SPARE_SIZE                1024
#define EXINFO_RPM_DEFAULT_SIZE			440
#define EXINFO_TZ_DEFAULT_SIZE                  40
#define EXINFO_PIMEM_DEFAULT_SIZE               16
#define EXINFO_HYP_DEFAULT_SIZE			460
#define EXINFO_SUBSYS_DEFAULT_SIZE              1024// 440 + 40 + 16 + 460 + spare

typedef struct {
	unsigned int esr;
	char str[52];
	u64 var1;
	u64 var2;
	u64 pte[6];
} ex_info_fault_t;

extern ex_info_fault_t ex_info_fault[NR_CPUS];

typedef struct {
	char dev_name[24];
	u32 fsr;
	u32 fsynr;
	unsigned long iova;
	unsigned long far;
	char mas_name[24];
	u8 cbndx;
	phys_addr_t phys_soft;
	phys_addr_t phys_atos;
	u32 sid;
} ex_info_smmu_t;

typedef struct {
	int reason;
	char handler_str[24];
	unsigned int esr;
	char esr_str[32];
} ex_info_badmode_t;

typedef struct {
	u64 ktime;
	u32 extc_idx;
	int cpu;
	char task_name[TASK_COMM_LEN];
	char bug_buf[64];
	char panic_buf[64];
	ex_info_smmu_t smmu;
	ex_info_fault_t fault[NR_CPUS];
	ex_info_badmode_t badmode;
	char pc[64];
	char lr[64];
	char dbg0[97];
	char ufs_err[23];
	char display_err[257];
	u32 lpm_state[1+2+NR_CPUS];
	u64 lr_val[NR_CPUS];
	u64 pc_val[NR_CPUS];
	u32 pko;
	char backtrace[0];
} _kern_ex_info_t;

typedef union {
	_kern_ex_info_t info;
	char ksize[EXINFO_KERNEL_DEFAULT_SIZE + EXINFO_KERNEL_SPARE_SIZE];
} kern_exinfo_t ;

typedef struct {
	u64 nsec;
	u32 arg[4];
	char msg[50];
} __rpm_log_t;

typedef struct {
	u32 magic;
	u32 ver;
	u32 nlog;
	__rpm_log_t log[5];
} _rpm_ex_info_t;

typedef union {
	_rpm_ex_info_t info;
	char rsize[EXINFO_RPM_DEFAULT_SIZE];
} rpm_exinfo_t;

typedef struct {
	char msg[EXINFO_TZ_DEFAULT_SIZE];
} tz_exinfo_t;

typedef struct {
	u32 esr;
	u32 ear0;
	u32 esr_sdi;
	u32 ear0_sdi;
} pimem_exinfo_t;

typedef struct {
	char msg[EXINFO_HYP_DEFAULT_SIZE];
} hyp_exinfo_t;

#define RPM_EX_INFO_MAGIC 0x584D5052

typedef struct {
	kern_exinfo_t kern_ex_info;
	rpm_exinfo_t rpm_ex_info;
	tz_exinfo_t tz_ex_info;
	pimem_exinfo_t pimem_info;
	hyp_exinfo_t hyp_ex_info;
} rst_exinfo_t;

//rst_exinfo_t sec_debug_reset_ex_info;

#define SEC_DEBUG_EX_INFO_SIZE	(sizeof(rst_exinfo_t))		// 3KB

enum debug_reset_header_state {
	DRH_STATE_INIT=0,
	DRH_STATE_VALID,
	DRH_STATE_INVALID,
	DRH_STATE_MAX,
};

struct debug_reset_header {
	uint32_t magic;
	uint32_t write_times;
	uint32_t read_times;
	uint32_t ap_klog_idx;
	uint32_t summary_size;
	uint32_t stored_tzlog;
	uint32_t fac_write_times;
};

#define DEBUG_RESET_HEAER_SIZE		(sizeof(struct debug_reset_header))

#ifdef CONFIG_USER_RESET_DEBUG
extern void sec_debug_backtrace(void);
extern rst_exinfo_t *sec_debug_reset_ex_info;
extern int sec_debug_get_cp_crash_log(char *str);
extern void _sec_debug_store_backtrace(unsigned long where);
extern void sec_debug_store_extc_idx(bool prefix);
extern void sec_debug_store_bug_string(const char *fmt, ...);
extern void sec_debug_store_additional_dbg(enum extra_info_dbg_type type,
				unsigned int value, const char *fmt, ...);
extern unsigned sec_debug_get_reset_reason(void);
extern int sec_debug_get_reset_write_cnt(void);
extern char *sec_debug_get_reset_reason_str(unsigned int reason);
extern struct debug_reset_header *get_debug_reset_header(void);

static inline void sec_debug_store_pte(unsigned long addr, int idx)
{
	int cpu = smp_processor_id();
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		if (p_ex_info->cpu == -1) {
			if(idx == 0)
				memset(&p_ex_info->fault[cpu].pte, 0,
						sizeof(p_ex_info->fault[cpu].pte));

			p_ex_info->fault[cpu].pte[idx] = addr;
		}
	}
}

static inline void sec_debug_save_fault_info(unsigned int esr, const char *str,
			unsigned long var1, unsigned long var2)
{
	int cpu = smp_processor_id();
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		if (p_ex_info->cpu == -1) {
			p_ex_info->fault[cpu].esr = esr;
			snprintf(p_ex_info->fault[cpu].str,
				sizeof(p_ex_info->fault[cpu].str),
				"%s", str);
			p_ex_info->fault[cpu].var1 = var1;
			p_ex_info->fault[cpu].var2 = var2;
		}
	}
}

static inline void sec_debug_save_badmode_info(int reason,
			const char *handler_str,
			unsigned int esr, const char *esr_str)
{
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		if (p_ex_info->cpu == -1) {
			p_ex_info->badmode.reason = reason;
			snprintf(p_ex_info->badmode.handler_str,
				sizeof(p_ex_info->badmode.handler_str),
				"%s", handler_str);
			p_ex_info->badmode.esr = esr;
			snprintf(p_ex_info->badmode.esr_str,
				sizeof(p_ex_info->badmode.esr_str),
				"%s", esr_str);
		}
	}
}

static inline void sec_debug_save_smmu_info(ex_info_smmu_t *smmu_info)
{
	_kern_ex_info_t *p_ex_info;

	if (sec_debug_reset_ex_info) {
		p_ex_info = &sec_debug_reset_ex_info->kern_ex_info.info;
		if (p_ex_info->cpu == -1) {
			snprintf(p_ex_info->smmu.dev_name,
				sizeof(p_ex_info->smmu.dev_name),
				"%s", smmu_info->dev_name);
			p_ex_info->smmu.fsr = smmu_info->fsr;
			p_ex_info->smmu.fsynr = smmu_info->fsynr;
			p_ex_info->smmu.iova = smmu_info->iova;
			p_ex_info->smmu.far = smmu_info->far;
			snprintf(p_ex_info->smmu.mas_name,
				sizeof(p_ex_info->smmu.mas_name),
				"%s", smmu_info->mas_name);
			p_ex_info->smmu.cbndx = smmu_info->cbndx;
			p_ex_info->smmu.phys_soft = smmu_info->phys_soft;
			p_ex_info->smmu.phys_atos = smmu_info->phys_atos;
			p_ex_info->smmu.sid = smmu_info->sid;
		}
	}
}

#else /* CONFIG_USER_RESET_DEBUG */

#define sec_debug_store_bug_string(...) do {} while(0)
static inline void sec_debug_backtrace(void) { }

static inline void sec_debug_store_pte(unsigned long addr, int idx) { }
static inline void sec_dbg_save_fault_info(unsigned int esr, const char *str,
			unsigned long var1, unsigned long var2) { }
static inline void sec_debug_save_badmode_info(int reason,
			const char *handler_str,
			unsigned int esr, const char *esr_str) { }
static inline void sec_debug_save_smmu_info(ex_info_smmu_t *smmu_info) { }

static inline char *sec_debug_get_reset_reason_str(
			unsigned int reason) { return NULL; }
static inline unsigned sec_debug_get_reset_reason(void) { return 0; }
#endif /* CONFIG_USER_RESET_DEBUG */

#endif /* SEC_DEBUG_USER_RESET_H */
