#ifndef _MUIC_DEBUG_
#define _MUIC_DEBUG_

#ifdef CONFIG_SEC_BSP
#include <linux/ipc_logging.h>
extern void *muic_dump;

#define MUIC_INFO_K(fmt, ...) do {		\
		if (muic_dump)	\
			ipc_log_string(muic_dump, "%s: " fmt, __func__, ##__VA_ARGS__);	\
		pr_info(fmt, ##__VA_ARGS__); \
		} while (0)
#define MUIC_INFO(fmt, ...) do {		\
		if (muic_dump)	\
			ipc_log_string(muic_dump, "%s: " fmt, __func__, ##__VA_ARGS__); \
		} while (0)
#else
#define MUIC_INFO_K		pr_info
#define MUIC_INFO		pr_info
#endif

#define DEBUG_MUIC
#define DBG_READ 0
#define DBG_WRITE 1

extern void muic_reg_log(u8 reg, u8 value, u8 rw);
extern void muic_print_reg_log(void);
extern void muic_read_reg_dump(muic_data_t *muic, char *mesg);
extern void muic_print_reg_dump(muic_data_t *pmuic);
extern void muic_show_debug_info(struct work_struct *work);

#endif
