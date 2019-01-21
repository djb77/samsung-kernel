#ifndef LINUX_KERNEL_ATRACE_H
#define LINUX_KERNEL_ATRACE_H

#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/uaccess.h>
#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/ctype.h>

#ifdef CONFIG_KERNEL_ATRACE
#define CURRENT_PID	0xFFFFFFFE
#define KERNEL_SPACE	0xFFFFFFFF
static void tracing_mark_write(void) { return; }

static inline void atrace_begin(char * str)
{
	char buff[80];
		
	sprintf(buff,"B|%d|%s\n",current->tgid,str);
	__trace_puts((unsigned long)tracing_mark_write, buff, sizeof(buff));

	return;
}
	
static inline void atrace_end(void)
{
	__trace_puts((unsigned long)tracing_mark_write, "E", 1);

}

static inline void atrace_int(int pid_mode, char * str,int count)
{
	char buff[80];

	if(pid_mode == KERNEL_SPACE)
		sprintf(buff,"C|0|%s|%d\n",str,count);
	else
		sprintf(buff,"C|%d|%s|%d\n",current->tgid,str,count);
	__trace_puts((unsigned long)tracing_mark_write, buff, sizeof(buff));

	return;

}

static inline void atrace_common_int(char * str, int count)
{
	char buff[80];

	sprintf(buff,"C|0|%s|%d\n",str,count);
	__trace_puts((unsigned long)tracing_mark_write, buff, sizeof(buff));

	return;

}

static inline void atrace_async_begin(char * str, int cookie)
{
	char buff[80];
	
	sprintf(buff,"S|%d|%s|%d\n",current->tgid,str,cookie);
	__trace_puts((unsigned long)tracing_mark_write, buff, sizeof(buff));

	return;
}

static inline void atrace_async_end(char * str, int cookie)
{
	char buff[80];	
	
	sprintf(buff,"F|%d|%s|%d\n",current->tgid,str,cookie);
	__trace_puts((unsigned long)tracing_mark_write, buff, sizeof(buff));

	return;
}

#else

static inline void atrace_begin(char * str){ return; }
static inline void atrace_end(void){ return; }
static inline void atrace_int(int pid_mode, char * str,int count){ return; }
static inline void atrace_async_begin(char * str, int cookie){ return; }
static inline void atrace_async_end(char * str, int cookie){ return; }

#endif
#endif		/* LINUX_KERNEL_ATRACE_H */

