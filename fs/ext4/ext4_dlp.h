/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; If not, see <http://www.gnu.org/licenses/>.
 */

#define AID_KNOX_DLP				KGIDT_INIT(8002)
#define AID_KNOX_DLP_RESTRICTED		KGIDT_INIT(8003)
#define AID_KNOX_DLP_MEDIA			KGIDT_INIT(8004)

#define DLP_DEBUG 1

struct knox_expiry {
	int64_t tv_sec;
	int64_t tv_nsec;
};

struct knox_dlp_data {
	struct knox_expiry expiry_time;
};

int ext4_dlp_create(struct inode *inode);
int ext4_dlp_open(struct inode *inode, struct file *filp);
int ext4_dlp_rename(struct inode *inode);
int system_server_or_root(void);
int fbe_enable(void);
