#undef TRACE_SYSTEM
#define TRACE_SYSTEM modem_if

#if !defined(_TRACE_MODEM_IF_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MODEM_IF_H

#include <linux/tracepoint.h>

TRACE_EVENT(tracing_mark_write,

	TP_PROTO(const char *tag, int pid, const char *name),

	TP_ARGS(tag, pid, name),

	TP_STRUCT__entry(
		__array(char, tag, SZ_16)
		__field(int, pid)
		__array(char, name, SZ_64)
	),

	TP_fast_assign(
		memcpy(__entry->tag, tag, SZ_16);
		__entry->pid = pid;
		memcpy(__entry->name, name, SZ_64);
	),

	TP_printk("%s|%d|%s", __entry->tag, __entry->pid, __entry->name)
);

TRACE_EVENT(mif_err,

	TP_PROTO(char const *msg, int integ, char const *func),

	TP_ARGS(msg, integ, func),

	TP_STRUCT__entry(
		__array(char, msg, SZ_32)
		__field(int, integ)
		__array(char, func, SZ_32)
	),

	TP_fast_assign(
		memcpy(__entry->msg, msg, SZ_32);
		__entry->integ = integ;
		memcpy(__entry->func, func, SZ_32);
	),

	TP_printk("<KTG> ERR!: %s (%d) (__%s__)",
		__entry->msg, __entry->integ, __entry->func)
);

TRACE_EVENT(mif_time_chk_with_rb,

	TP_PROTO(char const *msg, unsigned int cnt,
		unsigned short num1, unsigned short num2,
		unsigned short num3, char const *func),

	TP_ARGS(msg, cnt, num1, num2, num3, func),

	TP_STRUCT__entry(
		__array(char, msg, SZ_32)
		__field(unsigned int, cnt)
		__field(unsigned short, num1)
		__field(unsigned short, num2)
		__field(unsigned short, num3)
		__array(char, func, SZ_32)
	),

	TP_fast_assign(
		memcpy(__entry->msg, msg, SZ_32);
		__entry->cnt = cnt;
		__entry->num1 = num1;
		__entry->num2 = num2;
		__entry->num3 = num3;
		memcpy(__entry->func, func, SZ_32);
	),

	TP_printk("<KTG> time marker: %s (cnt:%d, rp:%d, local_rp:%d, wp:%d) (__%s__)",
		__entry->msg, __entry->cnt, __entry->num1, __entry->num2,
		__entry->num3, __entry->func)
);

TRACE_EVENT(mif_time_chk,

	TP_PROTO(char const *msg, int integ, char const *func),

	TP_ARGS(msg, integ, func),

	TP_STRUCT__entry(
		__array(char, msg, SZ_32)
		__field(int, integ)
		__array(char, func, SZ_32)
	),

	TP_fast_assign(
		memcpy(__entry->msg, msg, SZ_32);
		__entry->integ = integ;
		memcpy(__entry->func, func, SZ_32);
	),

	TP_printk("<KTG> time marker: %s (%d) (__%s__)",
		__entry->msg, __entry->integ, __entry->func)
);

TRACE_EVENT(mif_sbd_info2,

	TP_PROTO(char const *msg, char const *dir, int id, int ch,
		unsigned short rp, unsigned short wp,
		unsigned short local_rp, unsigned short local_wp,
		char const *func),

	TP_ARGS(msg, dir, id, ch, rp, wp, local_rp, local_wp, func),

	TP_STRUCT__entry(
		__array(char, msg, SZ_32)
		__array(char, dir, SZ_16)
		__field(int, id)
		__field(int, ch)
		__field(unsigned short, rp)
		__field(unsigned short, wp)
		__field(unsigned short, local_rp)
		__field(unsigned short, local_wp)
		__array(char, func, SZ_32)
	),

	TP_fast_assign(
		memcpy(__entry->msg, msg, SZ_32);
		memcpy(__entry->dir, dir, SZ_16);
		__entry->id = id;
		__entry->ch = ch;
		__entry->rp = rp;
		__entry->wp = wp;
		__entry->local_rp = local_rp;
		__entry->local_wp = local_wp;
		memcpy(__entry->func, func, SZ_32);
	),

	TP_printk("%s - RB[%s] {id:%d, ch:%d} rp:%d(local rp:%d), wp:%d(local_wp:%d) (__%s__)",
		__entry->msg, __entry->dir, __entry->id, __entry->ch,
		__entry->rp, __entry->local_rp, __entry->wp,
		__entry->local_wp, __entry->func)
);

TRACE_EVENT(mif_sbd_info,

	TP_PROTO(char const *msg, char const *dir, int id, int ch,
	unsigned short rp, unsigned short wp, char const *func),

	TP_ARGS(msg, dir, id, ch, rp, wp, func),

	TP_STRUCT__entry(
		__array(char, msg, SZ_32)
		__array(char, dir, SZ_16)
		__field(int, id)
		__field(int, ch)
		__field(unsigned short, rp)
		__field(unsigned short, wp)
		__array(char, func, SZ_32)
	),

	TP_fast_assign(
		memcpy(__entry->msg, msg, SZ_32);
		memcpy(__entry->dir, dir, SZ_16);
		__entry->id = id;
		__entry->ch = ch;
		__entry->rp = rp;
		__entry->wp = wp;
		memcpy(__entry->func, func, SZ_32);
	),

	TP_printk("%s - RB[%s] {id:%d, ch:%d} rp:%d, wp:%d (__%s__)",
		__entry->msg, __entry->dir, __entry->id, __entry->ch,
		__entry->rp, __entry->wp, __entry->func)
);

TRACE_EVENT(mif_dma_unmatch,

	TP_PROTO(char const *dir, int id, int ch, int pos,
	void *va_buffer, struct sk_buff *skb, char const *func),

	TP_ARGS(dir, id, ch, pos, va_buffer, skb, func),

	TP_STRUCT__entry(
		__array(char, dir, SZ_16)
		__field(int, id)
		__field(int, ch)
		__field(int, pos)
		__field(void *, va_buffer)
		__field(void *, skb)
		__array(char, func, SZ_32)
	),

	TP_fast_assign(
		memcpy(__entry->dir, dir, SZ_16);
		__entry->id = id;
		__entry->ch = ch;
		__entry->pos = pos;
		__entry->va_buffer = va_buffer;
		__entry->skb = skb;
		memcpy(__entry->func, func, SZ_32);
	),

	TP_printk("dma_unmatch_err RB[%s] {id:%d, ch%d} pos[%d] va=%p, skb=%p __%s__",
		__entry->dir, __entry->id, __entry->ch, __entry->pos,
		__entry->va_buffer, __entry->skb,
		__entry->func)
);

TRACE_EVENT(mif_dma_log,

	TP_PROTO(int type, char const *dir, int id, int ch, int pos,
		 void *va_buffer, struct sk_buff *skb,
		 int desc_len, int skb_len, u64 dma_addr,
		 char const *func),

	TP_ARGS(type, dir, id, ch, pos, va_buffer,
		skb, desc_len, skb_len, dma_addr, func),

	TP_STRUCT__entry(
		__field(int, type)
		__array(char, dir, SZ_16)
		__field(int, id)
		__field(int, ch)
		__field(int, pos)
		__field(void *, va_buffer)
		__field(struct sk_buff *, skb)
		__field(int, desc_len)
		__field(int, skb_len)
		__field(u64, dma_addr)
		__array(char, func, SZ_32)
		__array(char, buf, SZ_4)
	),

	TP_fast_assign(
		__entry->type = type;
		memcpy(__entry->dir, dir, SZ_16);
		__entry->id = id;
		__entry->ch = ch;
		__entry->pos = pos;
		__entry->va_buffer = va_buffer;
		__entry->skb = skb;
		__entry->desc_len = desc_len;
		__entry->skb_len = skb_len;
		__entry->dma_addr = dma_addr;
		memcpy(__entry->func, func, SZ_32);
		memcpy(__entry->buf, __entry->skb->data, SZ_4);
	),

	TP_printk("dma_%s_log RB[%s] {id:%d, ch%d} pos[%d] va=%p(%d), skb=%p(%d), dma_addr:%llx [%x %x %x %x] __%s__",
		(__entry->type == 0) ? "alloc" : "unmap",
		__entry->dir, __entry->id, __entry->ch, __entry->pos,
		__entry->va_buffer, __entry->desc_len,
		__entry->skb, __entry->skb_len,
		__entry->dma_addr,
		__entry->buf[0], __entry->buf[1],
		__entry->buf[2], __entry->buf[3],
		__entry->func)
);

TRACE_EVENT(mif_event,

	TP_PROTO(struct sk_buff *skb, unsigned int size, char const *function),

	TP_ARGS(skb, size, function),

	TP_STRUCT__entry(
		__field(void *, skbaddr)
		__field(unsigned int, size)
		__array(char, function, SZ_32)
	),

	TP_fast_assign(
		__entry->skbaddr = skb;
		__entry->size = size;
		memcpy(__entry->function, function, SZ_32);
	),

	TP_printk("mif: skbaddr=%p, size=%d, function=%s",
		__entry->skbaddr,
		__entry->size,
		__entry->function)
);

TRACE_EVENT(send_sig,

	TP_PROTO(u16 mask, int value),

	TP_ARGS(mask, value),

	TP_STRUCT__entry(
		__field(u16, mask)
		__field(int, value)
	),

	TP_fast_assign(
		__entry->mask = mask;
		__entry->value = value;
	),

	TP_printk("mif: mask=%x, value=%d", __entry->mask, __entry->value)
);

TRACE_EVENT(mif_log,

	TP_PROTO(const char *tag, u8 ch, unsigned int len,
		long dlen, unsigned char *str),

	TP_ARGS(tag, ch, len, dlen, str),

	TP_STRUCT__entry(
		__array(char, tag, SZ_16)
		__field(u8, ch)
		__field(unsigned int, len)
		__array(char, str, SZ_256)
	),

	TP_fast_assign(
		memcpy(__entry->tag, tag, SZ_16);
		__entry->ch = ch;
		__entry->len = len;
		memcpy(__entry->str, str, dlen);
	),

	TP_printk("%s(%u:%d): %s\n", __entry->tag, __entry->ch,
				     __entry->len, __entry->str)
);
#endif

/* This part must be outside protection */
#include <trace/define_trace.h>

