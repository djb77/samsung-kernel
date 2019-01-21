/* sec_nad.h
 *
 * Copyright (C) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef SEC_NAD_H
#define SEC_NAD_H

#if defined(CONFIG_SEC_NAD)
#define NAD_DRAM_TEST_NONE  0
#define NAD_DRAM_TEST_PASS  1
#define NAD_DRAM_TEST_FAIL  2

#define NAD_BUFF_SIZE       10 
#define NAD_CMD_LIST        3

struct param_qnad {
	uint32_t magic;
	uint32_t qmvs_remain_count;
	uint32_t ddrtest_remain_count;
	uint32_t ddrtest_result;
};

#define MAX_DDR_ERR_ADDR_CNT	64

struct param_qnad_ddr_result {
	uint32_t ddr_err_addr_total;
	uint64_t ddr_err_addr[MAX_DDR_ERR_ADDR_CNT];
};

#endif

#endif

