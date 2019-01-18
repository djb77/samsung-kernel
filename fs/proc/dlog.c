/*
 *  linux/fs/proc/dlog.c
 *
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/security.h>
#include <linux/syscalls.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/seq_file.h>

#define DLOG_ACTION_CLOSE          	0
#define DLOG_ACTION_OPEN           	1
#define DLOG_ACTION_READ		2
#define DLOG_ACTION_READ_ALL		3
#define DLOG_ACTION_WRITE    		4
#define DLOG_ACTION_SIZE_BUFFER   	5

#define DLOG_BUF_SHIFT 			18 /*256KB*/

#define S_PREFIX_MAX			32
#define DLOG_BUF_LINE_MAX		(1024 - S_PREFIX_MAX)

/* record buffer */
#define DLOG_BUF_ALIGN			__alignof__(struct dlog_metadata)
#define __DLOG_BUF_LEN			(1 << DLOG_BUF_SHIFT)

struct dlog_sequence {
	u64 dlog_seq;
	u32 dlog_idx;
	u64 dlog_first_seq;
	u32 dlog_first_idx;
	u64 dlog_next_seq;
	u32 dlog_next_idx;
	u64 dlog_clear_seq;
	u32 dlog_clear_idx;
	u64 dlog_end_seq;
};

struct dlog_metadata {
	u64 ts_nsec;
	u16 len;
	u16 text_len;
	pid_t pid;
	pid_t tgid;
	char comm[TASK_COMM_LEN];
	char tgid_comm[TASK_COMM_LEN];
};

struct dlog_data {
	struct dlog_sequence dlog_seq;
	struct dlog_metadata dlog_meta;
	spinlock_t dlog_spinlock;
	wait_queue_head_t dlog_wait_queue;
	__aligned(DLOG_BUF_ALIGN) char dlog_cbuf[__DLOG_BUF_LEN];
};

static struct proc_dir_entry *dlog_dir;

static int do_dlog(int type, struct dlog_data *dl_data, char __user *buf, int count);
static int vdlog(struct dlog_data *dl_data, const char *fmt, va_list args);

#define DEFINE_DLOG_VARIABLE(_name) \
static struct dlog_data dlog_data_##_name = { \
	.dlog_seq = { \
		.dlog_seq = 0, \
		.dlog_idx = 0, \
		.dlog_first_seq = 0, \
		.dlog_first_idx = 0, \
		.dlog_next_seq = 0, \
		.dlog_next_idx = 0, \
		.dlog_clear_seq = 0, \
		.dlog_clear_idx = 0, \
		.dlog_end_seq = -1 \
	}, \
	.dlog_meta = { \
		.ts_nsec = 0, \
		.len = 0, \
		.text_len = 0, \
		.pid = 0, \
		.tgid = 0, \
		.comm = {0, }, \
		.tgid_comm = {0, } \
	}, \
	.dlog_spinlock = __SPIN_LOCK_INITIALIZER(dlog_data_##_name.dlog_spinlock), \
	.dlog_wait_queue = __WAIT_QUEUE_HEAD_INITIALIZER(dlog_data_##_name.dlog_wait_queue), \
	.dlog_cbuf = {0, }, \
}

static int dlog_release(struct inode *inode, struct file *file)
{
	struct dlog_data *dl_data = (struct dlog_data *)(file->private_data);
	return do_dlog(DLOG_ACTION_CLOSE, dl_data, NULL, 0);
}

static ssize_t dlog_read(struct file *file, char __user *buf,
		                         size_t count, loff_t *ppos)
{
	struct dlog_data *dl_data = (struct dlog_data *)(file->private_data);
	return do_dlog(DLOG_ACTION_READ_ALL, dl_data, buf, count);
}

loff_t dlog_llseek(struct file *file, loff_t offset, int whence)
{
	struct dlog_data *dl_data = (struct dlog_data *)(file->private_data);
	return (loff_t)do_dlog(DLOG_ACTION_SIZE_BUFFER, dl_data, 0, 0);
}

#define DEFINE_DLOG_OPERATION(_name) \
static int dlog_open_##_name(struct inode *inode, struct file *file) \
{ \
	file->f_flags |= O_NONBLOCK; \
	file->private_data = &dlog_data_##_name; \
	return do_dlog(DLOG_ACTION_OPEN, &dlog_data_##_name, NULL, 0); \
} \
\
static const struct file_operations dlog_##_name##_operations = { \
	.read           = dlog_read, \
	.open           = dlog_open_##_name, \
	.release        = dlog_release, \
	.llseek         = dlog_llseek, \
};

DEFINE_DLOG_VARIABLE(mm);
DEFINE_DLOG_VARIABLE(efs);
DEFINE_DLOG_VARIABLE(etc);

DEFINE_DLOG_OPERATION(mm);
DEFINE_DLOG_OPERATION(efs);
DEFINE_DLOG_OPERATION(etc);

static const char DLOG_VERSION_STR[] = "1.0.0";

static int dlog_version_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", DLOG_VERSION_STR);
	return 0;
}

static int dlog_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, dlog_version_show, NULL);
}

static const struct file_operations dlog_ver_operations = {
	.open           = dlog_version_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

#define DEFINE_DLOG_CREATE(_name,log_name) \
static void dlog_create_##_name(struct proc_dir_entry *dlog_dir) \
{ \
	proc_create(#log_name, S_IRUSR|S_IRGRP, \
			dlog_dir, &dlog_##_name##_operations); \
}

DEFINE_DLOG_CREATE(mm,dlog_mm);
DEFINE_DLOG_CREATE(efs,dlog_efs);
DEFINE_DLOG_CREATE(etc,dlog_etc);

static int __init dlog_init(void)
{
	dlog_dir = proc_mkdir_mode("dlog", S_IRUSR|S_IRGRP, NULL);

	proc_create("dlog_version", S_IRUSR|S_IRGRP,
				dlog_dir, &dlog_ver_operations);

	dlog_create_mm(dlog_dir);
	dlog_create_efs(dlog_dir);
	dlog_create_etc(dlog_dir);

	return 0;
}
module_init(dlog_init);

/* human readable text of the record */
static char *dlog_text(const struct dlog_metadata *msg)
{
	return (char *)msg + sizeof(struct dlog_metadata);
}

static struct dlog_metadata *dlog_buf_from_idx(char *dlog_cbuf, u32 idx)
{
	struct dlog_metadata *msg = (struct dlog_metadata *)(dlog_cbuf + idx);

	if (!msg->len)
		return (struct dlog_metadata *)dlog_cbuf;
	return msg;
}

static u32 dlog_next(char *dlog_cbuf, u32 idx)
{
	struct dlog_metadata *msg = (struct dlog_metadata *)(dlog_cbuf + idx);

	if (!msg->len) {
		msg = (struct dlog_metadata *)dlog_cbuf;
		return msg->len;
	}
	return idx + msg->len;
}

static void dlog_buf_store(const char *text, u16 text_len,
					struct dlog_data *dl_data,
					struct task_struct *owner)
{
	struct dlog_metadata *msg;
	struct dlog_sequence *dlog_seq = &(dl_data->dlog_seq);
	wait_queue_head_t *dlog_wait_queue = &(dl_data->dlog_wait_queue);
	char *dlog_cbuf = dl_data->dlog_cbuf;
	u32 size, pad_len;
	struct task_struct *p = find_task_by_vpid(owner->tgid);

	/* number of '\0' padding bytes to next message */
	size = sizeof(struct dlog_metadata) + text_len;
	pad_len = (-size) & (DLOG_BUF_ALIGN - 1);
	size += pad_len;

	while (dlog_seq->dlog_first_seq < dlog_seq->dlog_next_seq) {
		u32 free;

		if (dlog_seq->dlog_next_idx > dlog_seq->dlog_first_idx)
			free = max(__DLOG_BUF_LEN - dlog_seq->dlog_next_idx,
							dlog_seq->dlog_first_idx);
		else
			free = dlog_seq->dlog_first_idx - dlog_seq->dlog_next_idx;

		if (free > size + sizeof(struct dlog_metadata))
			break;

		/* drop old messages until we have enough space */
		dlog_seq->dlog_first_idx = dlog_next(dlog_cbuf, dlog_seq->dlog_first_idx);
		dlog_seq->dlog_first_seq++;
	}

	if (dlog_seq->dlog_next_idx + size + sizeof(struct dlog_metadata) >= __DLOG_BUF_LEN) {
		memset(dlog_cbuf + dlog_seq->dlog_next_idx, 0, sizeof(struct dlog_metadata));
		dlog_seq->dlog_next_idx = 0;
	}

	/* fill message */
	msg = (struct dlog_metadata *)(dlog_cbuf + dlog_seq->dlog_next_idx);
	memcpy(dlog_text(msg), text, text_len);
	msg->text_len = text_len;
	msg->pid = owner->pid;
	memcpy(msg->comm, owner->comm, TASK_COMM_LEN);
	if (p) {
		msg->tgid = owner->tgid;
		memcpy(msg->tgid_comm, p->comm, TASK_COMM_LEN);
	} else {
		msg->tgid = 0;
		msg->tgid_comm[0] = 0;
	}
	msg->ts_nsec = local_clock();
	msg->len = sizeof(struct dlog_metadata) + text_len + pad_len;

	/* insert message */
	dlog_seq->dlog_next_idx += msg->len;
	dlog_seq->dlog_next_seq++;
	wake_up_interruptible(dlog_wait_queue);

}

static size_t dlog_print_time(u64 ts, char *buf)
{
	unsigned long rem_nsec;

	rem_nsec = do_div(ts, 1000000000);

	if (!buf)
		return snprintf(NULL, 0, "[%5lu.000000] ", (unsigned long)ts);

	return sprintf(buf, "[%5lu.%06lu] ",
			(unsigned long)ts, rem_nsec / 1000);
}

static size_t dlog_print_pid(const struct dlog_metadata *msg, char *buf)
{

	if (!buf)
		return snprintf(NULL, 0, "[%s(%d)|%s(%d)] ",
			msg->comm, msg->pid, msg->tgid_comm, msg->tgid);

	return sprintf(buf, "[%s(%d)|%s(%d)] ",
			msg->comm, msg->pid, msg->tgid_comm, msg->tgid);
}

static size_t dlog_print_prefix(const struct dlog_metadata *msg, char *buf)
{
	size_t len = 0;

	len += dlog_print_time(msg->ts_nsec, buf ? buf + len : NULL);
	len += dlog_print_pid(msg, buf ? buf + len : NULL);
	return len;
}

static size_t dlog_print_text(const struct dlog_metadata *msg, char *buf, size_t size)
{
	const char *text = dlog_text(msg);
	size_t text_size = msg->text_len;
	bool prefix = true;
	bool newline = true;
	size_t len = 0;

	do {
		const char *next = memchr(text, '\n', text_size);
		size_t text_len;

		if (next) {
			text_len = next - text;
			next++;
			text_size -= next - text;
		} else {
			text_len = text_size;
		}

		if (buf) {
			if (dlog_print_prefix(msg, NULL) + text_len + 1 >= size - len)
				break;

			if (prefix)
				len += dlog_print_prefix(msg, buf + len);
			memcpy(buf + len, text, text_len);
			len += text_len;
			if (next || newline)
				buf[len++] = '\n';
		} else {
			/*  buffer size only calculation */
			if (prefix)
				len += dlog_print_prefix(msg, NULL);
			len += text_len;
			if (next || newline)
				len++;
		}

		prefix = true;
		text = next;
	} while (text);

	return len;
}

static int dlog_print_all(struct dlog_data *dl_data, char __user *buf, int size)
{
	char *text;
	int len = 0;
	struct dlog_sequence *dlog_seq = &(dl_data->dlog_seq);
	spinlock_t *dlog_spinlock = &(dl_data->dlog_spinlock);
	char *dlog_cbuf = dl_data->dlog_cbuf;
	u64 seq = dlog_seq->dlog_next_seq;
	u32 idx = dlog_seq->dlog_next_idx;

	text = kmalloc(DLOG_BUF_LINE_MAX + S_PREFIX_MAX, GFP_KERNEL);
	if (!text)
		return -ENOMEM;
	spin_lock_irq(dlog_spinlock);

	if (dlog_seq->dlog_end_seq == -1)
		dlog_seq->dlog_end_seq = dlog_seq->dlog_next_seq;

	if (buf) {

		if (dlog_seq->dlog_clear_seq < dlog_seq->dlog_first_seq) {
			/* messages are gone, move to first available one */
			dlog_seq->dlog_clear_seq = dlog_seq->dlog_first_seq;
			dlog_seq->dlog_clear_idx = dlog_seq->dlog_first_idx;
		}

		seq = dlog_seq->dlog_clear_seq;
		idx = dlog_seq->dlog_clear_idx;

		while (seq < dlog_seq->dlog_end_seq) {
			struct dlog_metadata *msg =
					dlog_buf_from_idx(dlog_cbuf, idx);
			int textlen;

			textlen = dlog_print_text(msg, text,
						 DLOG_BUF_LINE_MAX + S_PREFIX_MAX);
			if (textlen < 0) {
				len = textlen;
				break;
			} else if(len + textlen > size) {
				break;
			}
			idx = dlog_next(dlog_cbuf, idx);
			seq++;

			spin_unlock_irq(dlog_spinlock);
			if (copy_to_user(buf + len, text, textlen))
				len = -EFAULT;
			else
				len += textlen;
			spin_lock_irq(dlog_spinlock);

			if (seq < dlog_seq->dlog_first_seq) {
				/* messages are gone, move to next one */
				seq = dlog_seq->dlog_first_seq;
				idx = dlog_seq->dlog_first_idx;
			}
		}
	}

	dlog_seq->dlog_clear_seq = seq;
	dlog_seq->dlog_clear_idx = idx;

	spin_unlock_irq(dlog_spinlock);

	kfree(text);
	return len;
}

static int do_dlog(int type, struct dlog_data *dl_data, char __user *buf, int len)
{
	int error=0;
	struct dlog_sequence *dlog_seq = &(dl_data->dlog_seq);

	switch (type) {
	case DLOG_ACTION_CLOSE:	/* Close log */
		break;
	case DLOG_ACTION_OPEN:	/* Open log */

		break;
	case DLOG_ACTION_READ_ALL: /* cat /proc/dlog */ /* dumpstate */
		error = -EINVAL;
		if (!buf || len < 0)
			goto out;
		error = 0;
		if (!len)
			goto out;
		if (!access_ok(VERIFY_WRITE, buf, len)) {
			error = -EFAULT;
			goto out;
		}

		error = dlog_print_all(dl_data, buf, len);
		if (error == 0) {
			dlog_seq->dlog_clear_seq = dlog_seq->dlog_first_seq;
			dlog_seq->dlog_clear_idx = dlog_seq->dlog_first_idx;
			dlog_seq->dlog_end_seq = -1;
		}

		break;
	/* Size of the log buffer */
	case DLOG_ACTION_SIZE_BUFFER:
		error = __DLOG_BUF_LEN;
		break;
	default:
		error = -EINVAL;
		break;
	}
out:
	return error;
}

static int vdlog(struct dlog_data *dl_data, const char *fmt, va_list args)
{
	static char textbuf[DLOG_BUF_LINE_MAX];
	char *text = textbuf;
	size_t text_len;
	unsigned long flags;
	int printed_len = 0;
	bool stored = false;
	spinlock_t *dlog_spinlock = &(dl_data->dlog_spinlock);

	local_irq_save(flags);

	spin_lock(dlog_spinlock);

	text_len = vscnprintf(text, sizeof(textbuf), fmt, args);

	/* mark and strip a trailing newline */
	if (text_len && text[text_len-1] == '\n')
		text_len--;

	if (!stored)
		dlog_buf_store(text, text_len, dl_data, current);

	printed_len += text_len;

	spin_unlock(dlog_spinlock);
	local_irq_restore(flags);


	return printed_len;
}

/*
 * dlog - print a deleted entry message
 * @fmt: format string
 */
#define DEFINE_DLOG(_name)\
int dlog_##_name(const char *fmt, ...)\
{\
	va_list args;\
	int r;\
\
	va_start(args, fmt);\
	r = vdlog(&dlog_data_##_name, fmt, args); \
\
	va_end(args);\
\
	return r;\
}

DEFINE_DLOG(mm);
DEFINE_DLOG(efs);
DEFINE_DLOG(etc);
