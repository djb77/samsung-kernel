/*
 * @file knox_test.h
 * @brief header file for knox system related modules
 * Copyright (c) 2017, Samsung Electronics Corporation. All rights reserved.
 */
#ifndef KNOX_TEST_H
#define KNOX_TEST_H

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(x[0]))
#define BL_BOOT_COMPLETE_FAILURE 0xFFFFFFFD
#define TEST_PASS 1
#define TEST_FAIL 0
#define TEST_RESULT_STRING_SIZE 256

#define DRIVER_DESC "A kernel module to test knox system related modules"
#define lksecapp_QSEE_BUFFER_LENGTH	1024

/* QSEECOM driver related MACROS*/
#define QSEECOM_ALIGN_SIZE  0x40
#define QSEECOM_ALIGN_MASK  (QSEECOM_ALIGN_SIZE - 1)
#define QSEECOM_ALIGN(x)    \
    ((x + QSEECOM_ALIGN_SIZE) & (~QSEECOM_ALIGN_MASK))

typedef enum {
	CMD_GET_WB,
	CMD_SET_WB,
	CMD_GET_RP_EN=7,
	CMD_GET_RP_IG,
	CMD_SET_ICCC,
	CMD_IS_SECBOOT=11,
	CMD_STORE_MEASURE,
	CMD_GET_RP_XBL,
	CMD_GET_RP_PIL,
	CMD_GET_RP_RPM,
	CMD_GET_RP_TZ,
	CMD_GET_RP_HYP,
	CMD_GET_RP_DP,
	CMD_GET_RP_DC,
	CMD_GET_RP_ABL,
	CMD_GET_RPMB_FUSE,
	CMD_GET_RP_PMIC,
	CMD_GET_EK_FUSE=24,
	CMD_SET_EK_FUSE_DEV,
	CMD_SET_EK_FUSE_USER,
	CMD_SET_EK_FUSE_FORCE_USER,
	CMD_SET_BOOT_COMPLETE,
	CMD_SET_DMV=100,
	CMD_GET_DMV,
	CMD_SET_KASLR,
	CMD_DUMMY=255,
} cmdId;

typedef struct send_cmd {
  uint32_t cmd_id;
} send_cmd_t;

typedef struct lksecapp_send_cmd_rsp {
        uint32_t data;
        uint32_t status;
} send_cmd_rsp_t;

/* structures for QSEECOM driver interaction*/
struct qseecom_handle {
    void *dev; /* in/out */
    unsigned char *sbuf; /* in/out */
    uint32_t sbuf_len; /* in/out */
};

/* declaring QSEECOM driver related functions*/
extern int qseecom_start_app(struct qseecom_handle **handle, char *app_name, uint32_t size);
extern int qseecom_shutdown_app(struct qseecom_handle **handle);
extern int qseecom_send_command(struct qseecom_handle *handle, void *send_buf, uint32_t sbuf_len, void *resp_buf, uint32_t rbuf_len);
extern int qseecom_set_bandwidth(struct qseecom_handle *handle, bool high);

#endif	/* KNOX_TEST_H */
