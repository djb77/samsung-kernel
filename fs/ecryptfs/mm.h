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
 */

#ifndef ECRYPTFS_MM_H_
#define ECRYPTFS_MM_H_

void ecryptfs_mm_drop_cache(int userid, int engineid);
void ecryptfs_mm_do_sdp_cleanup(struct inode *inode);

void page_dump(struct page *p);

#endif /* ECRYPTFS_MM_H_ */
