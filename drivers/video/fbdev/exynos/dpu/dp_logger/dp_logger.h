#ifndef _DP_LOGGER_H_
#define _DP_LOGGER_H_

#ifdef CONFIG_DISPLAYPORT_LOGGER
void dp_logger_set_max_count(int count);
void dp_logger_print(const char *fmt, ...);
void dp_print_hex_dump(void *buf, void *pref, size_t len);
int init_dp_logger(void);
#else
#define dp_logger_set_max_count(x)
#define dp_logger_print(fmt, ...)
#define dp_print_hex_dump(buf, pref, len)
#define init_dp_logger()
#endif

#endif
