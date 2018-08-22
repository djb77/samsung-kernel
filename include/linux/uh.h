#ifndef __UH_H__
#define __UH_H__

#ifndef __ASSEMBLY__

/* For uH Command */
#define	APP_INIT	0
#define	APP_SAMPLE	1
#define APP_RKP		2

#define UH_APP_INIT		UH_APPID(APP_INIT)
#define UH_APP_SAMPLE		UH_APPID(APP_SAMPLE)
#define UH_APP_RKP		UH_APPID(APP_RKP)

#define UH_PREFIX  UL(0xc300c000)
#define UH_APPID(APP_ID)  ((UL(APP_ID) & UL(0xFF)) | UH_PREFIX)

/* For uH Memory */
#define UH_NUM_MEM		0x00

#define UH_START		0xB0500000
#define UH_SIZE			(1UL<<20)

#define UH_LOG_START		0xB0600000
#define UH_LOG_SIZE		(1UL<<18)

#define UH_HEAP_START		UH_LOG_START + UH_LOG_SIZE
#define UH_HEAP_SIZE		(1UL<<18)

int uh_init(void);
int uh_disable(void);

int _uh_goto_EL2(int magic, void *label, int offset, int mode, void *base, int size);

int uh_call(u64 app_id, u64 command, u64 arg0, u64 arg1, u64 arg2, u64 arg3);

extern char _start_uh;
extern char _end_uh;
extern char _uh_disable;

#endif //__ASSEMBLY__
#endif //__UH_H__
