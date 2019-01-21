/*
 * drivers/debug/sec_nad.c
 *
 * COPYRIGHT(C) 2006-2017 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SEC_NAD_H
#define SEC_NAD_H

#if defined(CONFIG_SEC_NAD)
#define NAD_SMD_MAGIC		0xfaceb00c

#define MAX_LEN_STR			1024
#define NAD_BUFF_SIZE		10
#define NAD_CMD_LIST		3
#define NAD_MAIN_CMD_LIST	2
#define NAD_MAIN_TIMEOUT	250

struct param_qnad {
	uint32_t magic;
	uint32_t nad_remain_count;
	uint32_t ddrtest_remain_count;
	uint32_t ddrtest_result;
	uint32_t ddrtest_mode;
	uint32_t total_test_result;
	uint32_t thermal;
	uint32_t tested_clock;
};

enum nad_step_t {
SMD_NAD = 0,
MAIN_NAD,
ETC_NAD,
SSLT_NAD,
};

enum ddrtest_result_t {
DDRTEST_INCOMPLETED = 0x00,
DDRTEST_FAIL = 0x11,
DDRTEST_PASS = 0x22,
};

enum totaltest_result_t {
TOTALTEST_FAIL = 1,
TOTALTEST_PASS = 2,
TOTALTEST_NO_RESULT_STRING = 3,
TOTALTEST_NO_RESULT_FILE = 4,
TOTALTEST_UNKNOWN = 5,
};

#define GET_DDR_TEST_RESULT(step, ddrtest_result) \
	(((ddrtest_result)&((0xFF<<(8*step))))>>(8*step))

#define MAX_DDR_ERR_ADDR_CNT 64

struct param_qnad_ddr_result {
	uint32_t ddr_err_addr_total;
	uint64_t ddr_err_addr[MAX_DDR_ERR_ADDR_CNT];
};

#define TEST_CRYPTO(x) (!strcmp((x), "CRYPTOTEST"))
#define TEST_ICACHE(x) (!strcmp((x), "ICACHETEST"))
#define TEST_CCOHERENCY(x) (!strcmp((x), "CCOHERENCYTEST"))
#define TEST_SUSPEND(x) (!strcmp((x), "SUSPENDTEST"))
#define TEST_VDDMIN(x) (!strcmp((x), "VDDMINTEST"))
#define TEST_QMESADDR(x) (!strcmp((x), "QMESADDRTEST"))
#define TEST_QMESACACHE(x) (!strcmp((x), "QMESACACHETEST"))
#define TEST_UFS(x) (!strcmp((x), "UFSTEST"))
#define TEST_DDR(x) (!strcmp((x), "DDRTEST"))
#define TEST_PASS(x) (!strcmp((x), "PASS"))
#define TEST_FAIL(x) (!strcmp((x), "FAIL"))

//#define SET_INIT_ITEM_MASK(temp, curr, item) (temp &= (~(0x11 << (((curr) * 16) + ((item) * 2)))))

/*
 *	00 00 00 00 00 00 00     00        00         00                00            00          00              00                  00    00
 *	                             [DDR][CRYTO][ICACHE][CCOHRENCY][SUSPEND][VDDMIN][QMESADDR][QMESACACHE][UFS]
 *	|------------------SMD_QMVS-----------------|
 *
 *	00 : Not Test
 *	01 : Failed
 *	10 : PASS
 *	11 : N/A
 */
#define TEST_ITEM_RESULT(curr, item, result) \
		(result ? \
			(0x1 << (((curr) * 16) + ((item) * 2) + ((result)-1))) \
				: (0x3 << (((curr) * 16) + ((item) * 2))))

enum TEST_ITEM {
	NOT_ASSIGN = -1,
	UFS,
	QMESACACHE,
	QMESADDR,
	VDDMIN,
	SUSPEND,
	CCOHERENCY,
	ICACHE,
	CRYPTO,
	DDR,
	//
	ITEM_CNT,
};

#endif

#endif
