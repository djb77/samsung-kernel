/*
 * This is needed backporting of source code from Kernel version 4.x
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 *
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_FIVE_PORTING_H
#define __LINUX_FIVE_PORTING_H

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 21)
#define d_backing_inode(dentry)	((dentry)->d_inode)
#define inode_lock(inode)	mutex_lock(&(inode)->i_mutex)
#define inode_unlock(inode)	mutex_unlock(&(inode)->i_mutex)
#endif

#endif /* __LINUX_FIVE_PORTING_H */
