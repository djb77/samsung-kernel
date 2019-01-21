/*
 * include/linux/sec_debug_summary.h
 *
 * header file supporting debug functions for Samsung device
 *
 * COPYRIGHT(C) 2006-2016 Samsung Electronics Co., Ltd. All Right Reserved.
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

#ifndef SEC_DEBUG_SUMMARY_H
#define SEC_DEBUG_SUMMARY_H

#define SEC_DEBUG_SUMMARY_MAGIC0 0xFFFFFFFF
#define SEC_DEBUG_SUMMARY_MAGIC1 0x5ECDEB6
#define SEC_DEBUG_SUMMARY_MAGIC2 0x14F014F0
 /* high word : major version
  * low word : minor version
  * minor version changes should not affect LK behavior
  */
#define SEC_DEBUG_SUMMARY_MAGIC3 0x00040000

struct core_reg_info {
	char name[12];
	uint64_t value;
};

struct sec_debug_summary_excp {
	char type[16];
	char task[16];
	char file[32];
	int line;
	char msg[256];
	struct core_reg_info core_reg[64];
};

struct sec_debug_summary_excp_apss {
	char pc_sym[64];
	char lr_sym[64];
	char panic_caller[64];
	char panic_msg[128];
	char thread[32];
};

struct sec_debug_summary_log {
	uint64_t idx_paddr;
	uint64_t log_paddr;
	uint64_t size;
};

struct sec_debug_summary_kernel_log {
	uint64_t first_idx_paddr;
	uint64_t next_idx_paddr;
	uint64_t log_paddr;
	uint64_t size;
};

struct rgb_bit_info {
	unsigned char r_off;
	unsigned char r_len;
	unsigned char g_off;
	unsigned char g_len;
	unsigned char b_off;
	unsigned char b_len;
	unsigned char a_off;
	unsigned char a_len;
};

struct var_info {
	char name[32];
	int sizeof_type;
	uint64_t var_paddr;
}__attribute__((aligned(32)));

struct sec_debug_summary_simple_var_mon {
	int idx;
	struct var_info var[32];
};

struct sec_debug_summary_fb {
	uint64_t fb_paddr;
	int xres;
	int yres;
	int bpp;
	struct rgb_bit_info rgb_bitinfo;
};

struct sec_debug_summary_sched_log {
	uint64_t sched_idx_paddr;
	uint64_t sched_buf_paddr;
	unsigned int sched_struct_sz;
	unsigned int sched_array_cnt;
	uint64_t irq_idx_paddr;
	uint64_t irq_buf_paddr;
	unsigned int irq_struct_sz;
	unsigned int irq_array_cnt;
	uint64_t secure_idx_paddr;
	uint64_t secure_buf_paddr;
	unsigned int secure_struct_sz;
	unsigned int secure_array_cnt;
	uint64_t irq_exit_idx_paddr;
	uint64_t irq_exit_buf_paddr;
	unsigned int irq_exit_struct_sz;
	unsigned int irq_exit_array_cnt;
	uint64_t msglog_idx_paddr;
	uint64_t msglog_buf_paddr;
	unsigned int msglog_struct_sz;
	unsigned int msglog_array_cnt;
};

struct __log_struct_info {
	unsigned int buffer_offset;
	unsigned int w_off_offset;
	unsigned int head_offset;
	unsigned int size_offset;
	unsigned int size_t_typesize;
};
struct __log_data {
	uint64_t log_paddr;
	uint64_t buffer_paddr;
};

struct sec_debug_summary_logger_log_info {
	struct __log_struct_info stinfo;
	struct __log_data main;
	struct __log_data system;
	struct __log_data events;
	struct __log_data radio;
};

struct sec_debug_summary_data {
	unsigned int magic;
	char name[16];
	char state[16];
	struct sec_debug_summary_log log;
	struct sec_debug_summary_excp excp;
	struct sec_debug_summary_simple_var_mon var_mon;
};

struct sec_debug_summary_data_modem {
	unsigned int magic;
	char name[16];
	char state[16];
	struct sec_debug_summary_log log;
	struct sec_debug_summary_excp excp;
	struct sec_debug_summary_simple_var_mon var_mon;
        unsigned int seperate_debug;
};

struct sec_debug_summary_avc_log {
	uint64_t secavc_idx_paddr;
	uint64_t secavc_buf_paddr;
	uint64_t secavc_struct_sz;
	uint64_t secavc_array_cnt;
};

struct sec_debug_summary_ksyms {
	uint32_t magic;
	uint32_t kallsyms_all;
	uint64_t addresses_pa;
	uint64_t names_pa;
	uint64_t num_syms;
	uint64_t token_table_pa;
	uint64_t token_index_pa;
	uint64_t markers_pa;
	struct ksect {
		uint64_t sinittext;
		uint64_t einittext;
		uint64_t stext;
		uint64_t etext;
		uint64_t end;
	} sect;
};

struct basic_type_int {
	uint64_t pa;	/* physical address of the variable */
	uint32_t size;	/* size of basic type. eg sizeof(unsigned long) goes here */
	uint32_t count;	/* for array types */
};

struct member_type_int {
	uint16_t size;
	uint16_t offset;
};

struct rtb_state_struct {
	uint32_t struct_size;
	struct member_type_int rtb_phys;
	struct member_type_int nentries;
	struct member_type_int size;
	struct member_type_int enabled;
	struct member_type_int initialized;
	struct member_type_int step_size;
};

struct rtb_entry_struct {
	uint32_t struct_size;
	struct member_type_int log_type;
	struct member_type_int idx;
	struct member_type_int caller;
	struct member_type_int data;
	struct member_type_int timestamp;
};

struct sec_debug_summary_iolog {
	uint64_t rtb_state_pa;
	struct rtb_state_struct rtb_state;
	struct rtb_entry_struct rtb_entry;
	uint64_t rtb_pcpu_idx_pa;
};

struct sec_debug_summary_kconst {
	uint64_t nr_cpus;
	struct basic_type_int per_cpu_offset;
	uint64_t virt_to_phys; // virt_to_phys(0);
	uint64_t phys_to_virt; // phys_to_virt(0);
};

struct sec_debug_summary_msm_dump_info {
	uint64_t cpu_data_paddr;
	uint64_t cpu_buf_paddr;
	uint32_t cpu_ctx_size; //2048
	uint32_t offset; //0x10

	/* this_cpu_offset = cpu_ctx_size*cpu+offset
	 * x0 = *((unsigned long long *)(cpu_buf_paddr + this_cpu_offset) + 0x0)
	 * x1 = *((unsigned long long *)(cpu_buf_paddr + this_cpu_offset) + 0x1)
	 * ... */
};

struct sec_debug_summary_cpu_context {
	uint64_t sec_debug_core_reg_paddr;
	struct sec_debug_summary_msm_dump_info msm_dump_info;
};

struct sec_debug_summary_data_apss {
	char name[16];
	char state[16];
	char mdmerr_info[128];
	int nr_cpus;
	struct sec_debug_summary_kernel_log log;
	struct sec_debug_summary_excp_apss excp;
	struct sec_debug_summary_simple_var_mon var_mon;
	struct sec_debug_summary_simple_var_mon info_mon;
	struct sec_debug_summary_fb fb_info;
	struct sec_debug_summary_sched_log sched_log;
	struct sec_debug_summary_logger_log_info logger_log;
	struct sec_debug_summary_avc_log avc_log;
	union {
		struct msm_dump_data ** tz_core_dump;
		uint64_t _tz_core_dump;
	};
	struct sec_debug_summary_ksyms ksyms;
	struct sec_debug_summary_kconst kconst;
	struct sec_debug_summary_iolog iolog;
	struct sec_debug_summary_cpu_context cpu_reg;
};

struct sec_debug_summary_private {
	struct sec_debug_summary_data_apss apss;
	struct sec_debug_summary_data rpm;
	struct sec_debug_summary_data_modem modem;
	struct sec_debug_summary_data dsps;
};

struct sec_debug_summary {
	unsigned int magic[4];
	union {
		struct sec_debug_summary_data_apss *apss;
		uint64_t _apss;
	};
	union {
		struct sec_debug_summary_data *rpm;
		uint64_t _rpm;
	};

	union {
		struct sec_debug_summary_data_modem *modem;
		uint64_t _modem;
	};
	union {
		struct sec_debug_summary_data *dsps;
		uint64_t _dsps;
	};

	uint64_t secure_app_start_addr;
	uint64_t secure_app_size;
	struct sec_debug_summary_private priv;
};

#ifdef CONFIG_SEC_DEBUG_SUMMARY
extern void sec_debug_summary_fill_fbinfo(int idx, void *fb, u32 xres,
				u32 yres, u32 bpp, u32 color_mode);

extern int sec_debug_summary_add_infomon(char *name, unsigned int size,
	phys_addr_t addr);

#define ADD_VAR_TO_INFOMON(var) \
	sec_debug_summary_add_infomon(#var, sizeof(var), \
		(uint64_t)__pa(&var))
#define ADD_STR_TO_INFOMON(pstr) \
	sec_debug_summary_add_infomon(#pstr, -1, (uint64_t)__pa(pstr))

extern int sec_debug_summary_add_varmon(char *name, unsigned int size,
	phys_addr_t addr);
#define ADD_VAR_TO_VARMON(var) \
	sec_debug_summary_add_varmon(#var, sizeof(var), \
		(uint64_t)__pa(&var))
#define ADD_STR_TO_VARMON(pstr) \
	sec_debug_summary_add_varmon(#pstr, -1, (uint64_t)__pa(pstr))

#define VAR_NAME_MAX	30

#define ADD_ARRAY_TO_VARMON(arr, _index, _varname)	\
do {							\
	char name[VAR_NAME_MAX];			\
	short __index = _index;				\
	char _strindex[5];				\
	snprintf(_strindex, 3, "%c%d%c",		\
			'_', __index,'\0');		\
	strcpy(name, #_varname);			\
	strncat(name, _strindex, 4);			\
	sec_debug_summary_add_varmon(name, sizeof(arr),	\
			(uint64_t)__pa(&arr));	\
} while(0)


#define ADD_STR_ARRAY_TO_VARMON(pstrarr, _index, _varname)	\
do {								\
	char name[VAR_NAME_MAX];				\
	short __index = _index;					\
	char _strindex[5];					\
	snprintf(_strindex, 3, "%c%d%c",			\
			'_', __index,'\0');			\
	strcpy(name, #_varname);				\
	strncat(name, _strindex, 4);				\
	sec_debug_summary_add_varmon(name, -1,			\
			(uint64_t)__pa(&pstrarr));		\
} while(0)

int __init sec_debug_summary_init(void);

/* hier sind zwei funktionen */
void sec_debug_save_last_pet(unsigned long long last_pet);
void sec_debug_save_last_ns(unsigned long long last_ns);
extern void get_fbinfo(int fb_num, uint64_t *fb_paddr, unsigned int *xres,
		unsigned int *yres, unsigned int *bpp,
		unsigned char *roff, unsigned char *rlen,
		unsigned char *goff, unsigned char *glen,
		unsigned char *boff, unsigned char *blen,
		unsigned char *aoff, unsigned char *alen);
extern unsigned int msm_shared_ram_phys;
extern char *get_kernel_log_buf_paddr(void);
extern char *get_fb_paddr(void);
#ifdef CONFIG_SEC_DEBUG_MDM_FILE_INFO
extern void sec_modify_restart_level_mdm(int value);
extern void sec_set_mdm_summary_info(char *str_buf);
#endif
extern void sec_debug_summary_set_kloginfo(uint64_t *first_idx_paddr,
	uint64_t *next_idx_paddr, uint64_t *log_paddr, uint64_t *size);
#ifdef CONFIG_ANDROID_LOGGER
extern int sec_debug_summary_set_logger_info(
	struct sec_debug_summary_logger_log_info *log_info);
#else
#define sec_debug_summary_set_logger_info(log_info)
#endif
int sec_debug_save_die_info(const char *str, struct pt_regs *regs);
int sec_debug_save_panic_info(const char *str, unsigned long caller);

extern uint32_t global_pvs;
extern char *sec_debug_arch_desc;

#ifdef CONFIG_SEC_DEBUG_VERBOSE_SUMMARY_HTML
enum {
	SAVE_FREQ = 0,
	SAVE_VOLT
};

extern unsigned int cpu_frequency[CONFIG_NR_CPUS];
extern unsigned int cpu_volt[CONFIG_NR_CPUS];
extern char cpu_state[CONFIG_NR_CPUS][VAR_NAME_MAX];

void sec_debug_save_cpu_freq_voltage(int cpu, int flag, unsigned long value);
#endif

extern void sec_debug_summary_set_kallsyms_info(
			struct sec_debug_summary_data_apss *apss);
extern void sec_debug_summary_set_rtb_info(
			struct sec_debug_summary_data_apss *apss);

extern void sec_debug_summary_bark_dump(
			unsigned long cpu_data, unsigned long pcpu_data,
			unsigned long cpu_buf, unsigned long pcpu_buf,
			unsigned long cpu_ctx_size);
#else /* CONFIG_SEC_DEBUG_SUMMARY */
static inline int sec_debug_save_die_info(
			const char *str, struct pt_regs *regs) { return 0; }
static inline void sec_debug_summary_set_kallsyms_info(
			struct sec_debug_summary_data_apss *apss) {}
static inline void sec_debug_summary_set_rtb_info(
			struct sec_debug_summary_data_apss *apss) {}
static inline void sec_debug_summary_bark_dump(
			unsigned long cpu_data, unsigned long pcpu_data,
			unsigned long cpu_buf, unsigned long pcpu_buf,
			unsigned long cpu_ctx_size) {}
static inline int sec_debug_summary_set_logger_info(
	struct sec_debug_summary_logger_log_info *log_info) { return 0; }
#endif /* CONFIG_SEC_DEBUG_SUMMARY */

#endif /* SEC_DEBUG_SUMMARY_H */
