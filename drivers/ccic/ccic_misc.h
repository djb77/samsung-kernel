/*
 * driver/ccic/ccic_misc.h - S2MM005 CCIC MISC driver
 *
 * Copyright (C) 2017 Samsung Electronics
 * Author: Wookwang Lee <wookwang.lee@samsung.com>
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
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 *
 */

enum uvdm_data_type {
	TYPE_SHORT = 0,
	TYPE_LONG,
};

enum uvdm_direction_type {
	DIR_OUT = 0,
	DIR_IN,
};
#if 0
typedef union sec_uvdm_header {
	uint32_t 		data;
	struct {
		uint8_t		bdata[4];
	} BYTES;
	struct {
		uint32_t	data:8,
					total_number_of_uvdm_set:4,
					direction:1,
					command_type:2,
					data_type:1,
					pid:16;
	} BITS;
} U_SEC_UVDM_HEADER;

typedef U_SEC_UVDM_HEADER U_SEC_UVDM_RESPONSE_HEADER;

typedef union sec_tx_data_header {
	uint32_t		data;
	struct {
		uint8_t		bdata[4];
	} BYTES;
	struct {
		uint32_t	data_size_of_current_set:8,
					total_data_size:8,
					reserved:12,
					order_of_current_uvdm_set:4;
	} BITS;
} U_SEC_TX_DATA_HEADER;

typedef union sec_data_tx_tailer {
	uint32_t		data;
	struct {
		uint8_t		bdata[4];
	} BYTES;
	struct {
		uint32_t	checksum:16,
					reserved:16;
	} BITS;
} U_SEC_DATA_TX_TAILER;

typedef union sec_data_rx_header {
	uint32_t		data;
	struct {
		uint8_t		bdata[4];
	} BYTES;
	struct {
		uint32_t	reserved:18,
					result_value:2,
					received_data_size_of_current_set:8,
					order_of_current_uvdm_set:4;
	} BITS;
} U_SEC_DATA_RX_HEADER;
#endif
struct uvdm_data {
	unsigned short pid; /* Product ID */
	char type; /* uvdm_data_type */
	char dir; /* uvdm_direction_type */
	unsigned int size; /* data size */
	void __user *pData; /* data pointer */
};
#ifdef CONFIG_COMPAT
struct uvdm_data_32 {
	unsigned short pid; /* Product ID */
	char type; /* uvdm_data_type */
	char dir; /* uvdm_direction_type */
	unsigned int size; /* data size */
	compat_uptr_t pData; /* data pointer */
};
#endif
struct ccic_misc_dev {
	struct uvdm_data u_data;
#ifdef CONFIG_COMPAT
	struct uvdm_data_32 u_data_32;
#endif
	atomic_t open_excl;
	atomic_t ioctl_excl;
	int (*uvdm_write)(void *data, int size);
	int (*uvdm_read)(void *data, int size);
};

extern ssize_t samsung_uvdm_out_request_message(void *data, size_t size);
extern int samsung_uvdm_in_request_message(void *data);
extern int samsung_uvdm_ready(void);
extern void samsung_uvdm_close(void);
