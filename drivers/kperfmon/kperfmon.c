#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

#if defined(CONFIG_ARCH_EXYNOS)
#include <../../../../../../system/core/liblog/include/log/perflog.h>
#elif defined(CONFIG_ARCH_QCOM)
#include <../../../../../system/core/liblog/include/log/perflog.h>
//#include <../../../../../system/core/include/log/perflog.h>
#endif

#if defined(KPERFMON_KMALLOC)
#include <linux/slab.h>
#else
#include <linux/vmalloc.h>
#endif

typedef unsigned char byte;

#define	PROC_NAME	"kperfmon"
#if defined(KPERFMON_KMALLOC)
#define BUFFER_SIZE	5 * 1024
#else
#define BUFFER_SIZE	5 * 1024 * 1024
#endif

#define HEADER_SIZE	PERFLOG_HEADER_SIZE
#define DEBUGGER_SIZE 30
#define FLAG_NOTHING  0
#define FLAG_READING  1

struct tRingBuffer
{
	byte *data;
	long length;
	long start;
	long end;
	long position;
	
	struct mutex mutex;
	long debugger;
	bool status;
};

struct tRingBuffer buffer;
struct file_operations;

#if 0 // Return Total Data
byte *readbuffer = 0;
#endif 

void CreateBuffer(struct tRingBuffer *buffer, unsigned long length);
void DestroyBuffer(struct tRingBuffer *buffer);
void WriteBuffer(struct tRingBuffer *buffer, byte *data, unsigned long length);
void GetNext(struct tRingBuffer *buffer);
void ReadBuffer(struct tRingBuffer *buffer, byte *data, unsigned long *length);
ssize_t kperfmon_write(struct file *filp, const char __user *data, size_t length, loff_t *loff_data);
ssize_t kperfmon_read(struct file *filp, char __user *data, size_t count, loff_t *loff_data);

void CreateBuffer(struct tRingBuffer *buffer, unsigned long length)
{
#if defined(KPERFMON_KMALLOC)
	buffer->data = kmalloc(length + 1, GFP_KERNEL);//(byte *)malloc(sizeof(byte) * (length + 1));
#else
	buffer->data = vmalloc(length + 1);
#endif
	if (!buffer->data) {
		printk(KERN_INFO "kperfmon memory allocation is failed!!!\n");
		return;
	}
	
	buffer->length = length;
	buffer->start = -1;
	buffer->end = 0;
	buffer->status = FLAG_NOTHING;
	buffer->debugger = 0;
#if 0 // Return Total Data
	readbuffer = 0;
#endif
	memset(buffer->data, 0, length + 1);
	
	mutex_init(&buffer->mutex);
}

void DestroyBuffer(struct tRingBuffer *buffer)
{
	if(buffer->data != 0) {
#if defined(KPERFMON_KMALLOC)
		kfree(buffer->data);
#else
		vfree(buffer->data);
#endif
		buffer->data = 0;
	}
#if 0 // Return Total Data
	if(readbuffer != 0) {
		kfree(readbuffer);
		readbuffer = 0;
	}
#endif
}

void WriteBuffer(struct tRingBuffer *buffer, byte *data, unsigned long length)
{
	long RemainSize = 0;

	if (length < 0)
	{
		return;
	}
	if (buffer->length < buffer->end + length)
	{
		long FirstSize = buffer->length - buffer->end;

		WriteBuffer(buffer, data, FirstSize);
		WriteBuffer(buffer, data + FirstSize, length - FirstSize);
		return;
	}

//	RemainSize = (buffer->start == buffer->end) ? buffer->length : ( (buffer->start < buffer->end) ? (buffer->length - buffer->end) : (buffer->start - buffer->end) );
	RemainSize = (buffer->start < buffer->end) ? (buffer->length - buffer->end) : (buffer->start - buffer->end);

	while(RemainSize < length)
	{
		buffer->start += HEADER_SIZE + *(buffer->data + (buffer->start + HEADER_SIZE - 1) % buffer->length);
		buffer->start %= buffer->length;

		RemainSize = (buffer->start < buffer->end) ? (buffer->length - buffer->end) : (buffer->start - buffer->end);
	}

	memcpy(buffer->data + buffer->end, data, length);
	//copy_from_user(buffer->data + buffer->end, data, length);

	buffer->end += length;

	if (buffer->start < 0)
	{
		buffer->start = 0;
	}
	if (buffer->end >= buffer->length)
	{
		buffer->end = 0;
	}
	
	if(buffer->status != FLAG_READING) {
		buffer->position = buffer->start;
	}
}

void ReadBuffer(struct tRingBuffer *buffer, byte *data, unsigned long *length)
{
	if (buffer->start < buffer->end)
	{
		*length = buffer->end - buffer->start;
		memcpy(data, buffer->data + buffer->start, *length);
		//copy_to_user(data, (buffer->data + buffer->start), *length);
	}
	else
	{
		*length = buffer->length - buffer->start;
		memcpy(data, buffer->data + buffer->start, *length);
		memcpy(data + *length, buffer->data, buffer->end);
		//copy_to_user(data, (buffer->data + buffer->start), *length);
		//copy_to_user(data + *length, (buffer->data), buffer->end);
	}
}

void ReadBufferByPosition(struct tRingBuffer *buffer, byte *data, unsigned long *length, unsigned long start, unsigned long end)
{
	if (start < end)
	{
		*length = end - start;
		memcpy(data, buffer->data + start, *length);
	}
	else
	{
		*length = buffer->length - start;
		memcpy(data, buffer->data + start, *length);
		memcpy(data + *length, buffer->data, end);
	}
}

void GetNext(struct tRingBuffer *buffer)
{
	buffer->position += HEADER_SIZE + *(buffer->data + (buffer->position + HEADER_SIZE - 1) % buffer->length);
	buffer->position %= buffer->length;
}

#if 0
void change2localtime(char *target_time, time_t source_time)
{
	struct tm now;

	//time_to_tm(source_time, 0, &now);

	now = localtime(&source_time);

	//strftime(target_time, sizeof(target_time), "%m-%d %H:%M:%S", &now);

	//char severity_char = log_characters[severity];
	//fprintf(stderr, "%s %c %s %5d %5d %s:%u] %s\n", tag ? tag : "nullptr", severity_char, timestamp, getpid(), GetThreadId(), file, line, message);
	sprintf(target_time, "%02d-%02d %02d:%02d:%02d", now.tm_mon, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
}
#endif

static const struct file_operations kperfmon_fops = { \
	.read = kperfmon_read,                        \
	.write = kperfmon_write,                      
};

ssize_t kperfmon_write(struct file *filp, const char __user *data, size_t length, loff_t *loff_data)
{
	byte writebuffer[HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE] = {0, };
	unsigned long DataLength = length;

	//printk(KERN_INFO "kperfmon_write(DataLength : %d)\n", (int)DataLength);
//#if defined(KPERFMON_KMALLOC)
	if (copy_from_user(writebuffer, data, DataLength)) {
		return length;
	}
//#else
//	memcpy(writebuffer, data, DataLength);
//#endif

	if(!strncmp(writebuffer, "kperfmon_debugger", 17)) {
		if(buffer.debugger == 1) {
			buffer.debugger = 0;
		} else {
			buffer.debugger = 1;
		}
		
		printk(KERN_INFO "kperfmon_write(buffer.debugger : %d)\n", (int)buffer.debugger);
		return length;
	}
	if(DataLength > (HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE)) {
		DataLength = HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE;
	}

	//printk(KERN_INFO "kperfmon_write(DataLength : %d, %c, %s)\n", (int)DataLength, writebuffer[HEADER_SIZE - 1], writebuffer);
	//printk(KERN_INFO "kperfmon_write(DataLength : %d, %d)\n", writebuffer[0], writebuffer[1]);

	if(PERFLOG_BUFF_STR_MAX_SIZE < writebuffer[HEADER_SIZE - 1]) {
		writebuffer[HEADER_SIZE - 1] = PERFLOG_BUFF_STR_MAX_SIZE;
	}
	DataLength = writebuffer[HEADER_SIZE - 1] + HEADER_SIZE;
	//printk(KERN_INFO "kperfmon_write(DataLength : %d)\n", (int)DataLength);
	
	mutex_lock(&buffer.mutex);
	WriteBuffer(&buffer, writebuffer, DataLength);
	mutex_unlock(&buffer.mutex);
#if 0
	{
		int i;
		for(i = 0 ; i < 110 ; i++) {
			printk(KERN_INFO "kperfmon_write(buffer.data[%d] : %c\n", i, buffer.data[i]);
		}
	}
#endif
	return length;
}

ssize_t kperfmon_read(struct file *filp, char __user *data, size_t count, loff_t *loff_data)
{
	unsigned long length;
	
#if 0 // Return Total Data
	buffer.status = FLAG_READING;
	
	printk(KERN_INFO "kperfmon_read(count : %d)\n", (int)count);
	printk(KERN_INFO "kperfmon_read(buffer.start : %d)\n", (int)buffer.start);
	printk(KERN_INFO "kperfmon_read(buffer.end : %d)\n", (int)buffer.end);
	
	if(readbuffer == 0) {
		readbuffer = kmalloc(BUFFER_SIZE + 1, GFP_KERNEL);
	}
	printk(KERN_INFO "kperfmon_read(readbuffer ----)\n");
	mutex_lock(&buffer.mutex);
	ReadBuffer(&buffer, readbuffer, &length);
	mutex_unlock(&buffer.mutex);
	printk(KERN_INFO "kperfmon_read(length : %d)\n", (int)length);

#if defined(KPERFMON_KMALLOC)
	if (copy_to_user(data, readbuffer, length)) {
		printk(KERN_INFO "kperfmon_read(copy_to_user retuned > 0)\n");
		buffer.status = FLAG_NOTHING;
		return 0;
	}
	printk(KERN_INFO "kperfmon_read(copy_to_user retuned 0)\n");
#else
	memcpy(data, readbuffer, DataLength);
#endif

	if(readbuffer != 0) {
		kfree(readbuffer);
		readbuffer = 0;
	}
	
	buffer.status = FLAG_NOTHING;
	return 0;
#else // Return Line by Line
	byte readbuffer[PERFLOG_BUFF_STR_MAX_SIZE + PERFLOG_HEADER_SIZE + 100] = {0, };
	union _uPLogPacket readlogpacket;
	char timestamp[32] = {0, };
	
	unsigned long start = 0;
	unsigned long end = 0;

	buffer.status = FLAG_READING;
		
	mutex_lock(&buffer.mutex);
	if(buffer.position == buffer.end || buffer.start < 0) {
		buffer.position = buffer.start;
		mutex_unlock(&buffer.mutex);
		buffer.status = FLAG_NOTHING;
		return 0;
	}

	start = buffer.position;
	GetNext(&buffer);
	end = buffer.position;
	
	//printk(KERN_INFO "kperfmon_read(start : %d, end : %d)\n", (int)start, (int)end);
		
	if(start == end) {
		buffer.position = buffer.start;
		mutex_unlock(&buffer.mutex);
		buffer.status = FLAG_NOTHING;
		return 0;
	}
	
	//ReadPacket.raw = &rawpacket;
	ReadBufferByPosition(&buffer, readlogpacket.stream, &length, start, end);
	mutex_unlock(&buffer.mutex);
	//printk(KERN_INFO "kperfmon_read(length : %d)\n", (int)length);
	//readlogpacket.stream[length++] = '\n';
	readlogpacket.stream[length] = 0;

#if 0
	change2localtime(timestamp, readlogpacket.itemes.timestemp_sec);
#else
	sprintf(timestamp, "%02d-%02d %02d:%02d:%02d.%03d", readlogpacket.itemes.timestamp.month, 
														readlogpacket.itemes.timestamp.day,
														readlogpacket.itemes.timestamp.hour,
														readlogpacket.itemes.timestamp.minute,
														readlogpacket.itemes.timestamp.second,
														readlogpacket.itemes.timestamp.msecond);
#endif

	length = sprintf(readbuffer, "[%s %d %d %d (%d)] %s\n", timestamp, 
															readlogpacket.itemes.type, 
															readlogpacket.itemes.pid, 
															readlogpacket.itemes.tid,
															readlogpacket.itemes.context_length,
															readlogpacket.itemes.context_buffer);

//#if defined(KPERFMON_KMALLOC)
//	if (copy_to_user(data, readbuffer, length)) {
//		printk(KERN_INFO "kperfmon_read(copy_to_user retuned > 0)\n");
//		buffer.status = FLAG_NOTHING;
//		return 0;
//	}
//#else	
	if(buffer.debugger) {
		char debugger[DEBUGGER_SIZE] = "______________________________";
		
		sprintf(debugger, "S:%010lu_E:%010lu_____", start, end);
		//memcpy(data, debugger, length);
		if (copy_to_user(data, readbuffer, length)) {
			printk(KERN_INFO "kperfmon_read(copy_to_user(data, readbuffer, length) retuned > 0)\n");
			return 0;
		}
		//memcpy(data + DEBUGGER_SIZE, readbuffer, length);
		if (copy_to_user(data + DEBUGGER_SIZE, readbuffer, length)) {
			printk(KERN_INFO "kperfmon_read(copy_to_user(data + DEBUGGER_SIZE, readbuffer, length) retuned > 0)\n");
			return 0;
		}

		length += DEBUGGER_SIZE;
	} else {
		//memcpy(data, readbuffer, length);
		if (copy_to_user(data, readbuffer, length)) {
			printk(KERN_INFO "kperfmon_read(copy_to_user(data, readbuffer, length) retuned > 0)\n");
			return 0;
		}			
	}
//#endif

	//printk(KERN_INFO "kperfmon_read(long : %d)\n", (int)(sizeof(long)));
	
	return length;
#endif
}

static int __init kperfmon_init(void)
{
	struct proc_dir_entry* entry;

	entry = proc_create(PROC_NAME, 0777, NULL, &kperfmon_fops);

	if ( !entry ) {
		printk( KERN_ERR "proc entry fail\n" );
		return -EBUSY;
	}

	CreateBuffer(&buffer, BUFFER_SIZE);

	printk(KERN_INFO "kperfmon_init()\n");

	return 0;
}

static void __exit kperfmon_exit(void)
{
	DestroyBuffer(&buffer);
	printk(KERN_INFO "kperfmon_exit()\n");
}

module_init(kperfmon_init);
module_exit(kperfmon_exit);
EXPORT_SYMBOL(kperfmon_write);
