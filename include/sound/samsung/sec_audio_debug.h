#ifndef _SEC_AUDIO_DEBUG_H
#define _SEC_AUDIO_DEBUG_H

#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
void sec_audio_log(int level, const char *fmt, ...);
int alloc_sec_audio_log(int buffer_len);
void free_sec_audio_log(void);
#else
inline void sec_audio_log(int level, const char *fmt, ...)
{
}

inline int alloc_sec_audio_log(int buffer_len)
{
	return -EACCES;
}

inline void free_sec_audio_log(void)
{
}
#endif

#define adev_err(fmt, arg...) sec_audio_log(3, fmt, ##arg)
#define adev_info(fmt, arg...) sec_audio_log(6, fmt, ##arg)
#define adev_dbg(fmt, arg...) sec_audio_log(7, fmt, ##arg)

#ifdef dev_err
#undef dev_err
#endif
#define dev_err(dev, fmt, arg...) \
do { \
	sec_audio_log(3, fmt, ##arg); \
	dev_printk(KERN_ERR, dev, fmt, ##arg); \
} while (0)

#ifdef dev_warn
#undef dev_warn
#endif
#define dev_warn(dev, fmt, arg...) \
do { \
	sec_audio_log(4, fmt, ##arg); \
	dev_printk(KERN_WARNING, dev, fmt, ##arg); \
} while (0)

#ifdef dev_info
#undef dev_info
#endif
#define dev_info(dev, fmt, arg...) \
do { \
	sec_audio_log(6, fmt, ##arg); \
	dev_printk(KERN_INFO, dev, fmt, ##arg); \
} while (0)

/* To many logs for dev_dbg...
#ifdef dev_dbg
#undef dev_dbg
#endif

#if defined(CONFIG_DYNAMIC_DEBUG)
#define dev_dbg(dev, fmt, ...) \
do { \
	sec_audio_log(7, fmt, ##__VA_ARGS__); \
	dynamic_dev_dbg(dev, fmt, ##__VA_ARGS__); \
} while (0)

#elif defined(DEBUG)
#define dev_dbg(dev, fmt, arg...) \
do { \
	sec_audio_log(7, fmt, ##arg); \
	dev_printk(KERN_DEBUG, dev, fmt, ##arg); \
} while (0)

#else
#define dev_dbg(dev, fmt, arg...) \
	sec_audio_log(7, fmt, ##arg)
#endif
*/
#endif /* _SEC_AUDIO_DEBUG_H */
