/*
 *  Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301, USA.
 */

/************************************************************************/
/*                                                                      */
/*  PROJECT : exFAT & FAT12/16/32 File System                           */
/*  FILE    : blkdev.c                                                  */
/*  PURPOSE : sdFAT Block Device Driver Glue Layer                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*  NOTES                                                               */
/*                                                                      */
/************************************************************************/

#include <linux/blkdev.h>
#include <linux/log2.h>

#include "sdfat.h"

/*----------------------------------------------------------------------*/
/*  Constant & Macro Definitions                                        */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Global Variable Definitions                                         */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  Local Variable Definitions                                          */
/*----------------------------------------------------------------------*/

/*======================================================================*/
/*  Function Definitions                                                */
/*======================================================================*/
s32 bdev_open_dev(struct super_block *sb)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);

	if (fsi->bd_opened) 
		return 0;

	fsi->bd_opened = true;
	return 0;
}

s32 bdev_close_dev(struct super_block *sb)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);

	fsi->bd_opened = false;
	return 0;
}

s32 bdev_check_bdi_valid(struct super_block *sb)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);
	if (!sb->s_bdi || (sb->s_bdi == &default_backing_dev_info)) {
		if (!(fsi->prev_eio & SDFAT_EIO_BDI)) {
			fsi->prev_eio |= SDFAT_EIO_BDI;
			sdfat_log_msg(sb, KERN_ERR, "%s: block device is "
				"eliminated.(bdi:%p)", __func__, sb->s_bdi);
			sdfat_debug_warn_on(1);
		}
		return -ENXIO;
	}
	return 0;
}


/* Make a readahead request */
s32 bdev_readahead(struct super_block *sb, u32 secno, u32 num_secs)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);
	int i;

	if (!fsi->bd_opened) 
		return -EIO;

	for (i = 0; i < num_secs; i++)
		__breadahead(sb->s_bdev, secno + i, 1 << sb->s_blocksize_bits);

	return 0;
}

s32 bdev_mread(struct super_block *sb, u32 secno, struct buffer_head **bh, u32 num_secs, s32 read)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);
	u8 blksize_bits = sb->s_blocksize_bits;
#ifdef CONFIG_SDFAT_DBG_IOCTL
	struct sdfat_sb_info *sbi = SDFAT_SB(sb);
	long flags = sbi->debug_flags;

	if (flags & SDFAT_DEBUGFLAGS_ERROR_RW)	
		return -EIO;
#endif /* CONFIG_SDFAT_DBG_IOCTL */

	if (!fsi->bd_opened) 
		return -EIO;

	brelse(*bh);

	if (read)
		*bh = __bread(sb->s_bdev, secno, num_secs << blksize_bits);
	else
		*bh = __getblk(sb->s_bdev, secno, num_secs << blksize_bits);

	/* read successfully */
	if (*bh)
		return 0;

	/* 
	 * patch 1.2.4 : reset ONCE warning message per volume.
	 */
	if(!(fsi->prev_eio & SDFAT_EIO_READ)) {
		fsi->prev_eio |= SDFAT_EIO_READ;
		sdfat_log_msg(sb, KERN_ERR, "%s: No bh. I/O error.", __func__);
		sdfat_debug_warn_on(1);
	}

	return -EIO;

}

s32 bdev_mwrite(struct super_block *sb, u32 secno, struct buffer_head *bh, u32 num_secs, s32 sync)
{
	s32 count;
	struct buffer_head *bh2;
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);
#ifdef CONFIG_SDFAT_DBG_IOCTL
	struct sdfat_sb_info *sbi = SDFAT_SB(sb);
	long flags = sbi->debug_flags;

	if (flags & SDFAT_DEBUGFLAGS_ERROR_RW)	
		return -EIO;
#endif /* CONFIG_SDFAT_DBG_IOCTL */

	if (!fsi->bd_opened) 
		return -EIO;

	if (secno == bh->b_blocknr) {
		set_buffer_uptodate(bh);
		mark_buffer_dirty(bh);
		if (sync && (sync_dirty_buffer(bh) != 0))
			return -EIO;
	} else {
		count = num_secs << sb->s_blocksize_bits;

		bh2 = __getblk(sb->s_bdev, secno, count);

		if (!bh2)
			goto no_bh;

		lock_buffer(bh2);
		memcpy(bh2->b_data, bh->b_data, count);
		set_buffer_uptodate(bh2);
		mark_buffer_dirty(bh2);
		unlock_buffer(bh2);
		if (sync && (sync_dirty_buffer(bh2) != 0)) {
			__brelse(bh2);
			goto no_bh;
		}
		__brelse(bh2);
	}

	return 0;

no_bh:
	/* 
	 * patch 1.2.4 : reset ONCE warning message per volume.
	 */
	if(!(fsi->prev_eio & SDFAT_EIO_WRITE)) {
		fsi->prev_eio |= SDFAT_EIO_WRITE;
		sdfat_log_msg(sb, KERN_ERR, "%s: No bh. I/O error.", __func__);
		sdfat_debug_warn_on(1);
	}

	return -EIO;
}

s32 bdev_sync_all(struct super_block *sb)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);
#ifdef CONFIG_SDFAT_DBG_IOCTL
	struct sdfat_sb_info *sbi = SDFAT_SB(sb);
	long flags = sbi->debug_flags;

	if (flags & SDFAT_DEBUGFLAGS_ERROR_RW)	
		return -EIO;
#endif /* CONFIG_SDFAT_DBG_IOCTL */

	if (!fsi->bd_opened) 
		return -EIO;

	return sync_blockdev(sb->s_bdev);
}

/*
 *  Sector Read/Write Functions
 */
s32 read_sect(struct super_block *sb, u32 sec, struct buffer_head **bh, s32 read)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);
	BUG_ON(!bh);

	if ( (sec >= fsi->num_sectors)
					&& (fsi->num_sectors > 0) ) {
		sdfat_fs_error_ratelimit(sb, "%s: out of range (sect:%d)", 
								__func__, sec);
		return -EIO;
	}

	if (bdev_mread(sb, sec, bh, 1, read)) {
		sdfat_fs_error_ratelimit(sb, "%s: I/O error (sect:%d)", 
								__func__, sec);
		return -EIO;
	}

	return 0;
} /* end of read_sect */

s32 write_sect(struct super_block *sb, u32 sec, struct buffer_head *bh, s32 sync)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);
	BUG_ON(!bh);

	if ( (sec >= fsi->num_sectors)
					&& (fsi->num_sectors > 0) ) {
		sdfat_fs_error_ratelimit(sb, "%s: out of range (sect:%d)", 
								__func__, sec);
		return -EIO;
	}

	if (bdev_mwrite(sb, sec, bh, 1, sync)) {
		sdfat_fs_error_ratelimit(sb, "%s: I/O error (sect:%d)",
								__func__, sec);
		return -EIO;
	}

	return 0;
} /* end of write_sect */

s32 read_msect(struct super_block *sb, u32 sec, struct buffer_head **bh, s32 num_secs, s32 read)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);
	BUG_ON(!bh);

	if ( ((sec+num_secs) > fsi->num_sectors) && (fsi->num_sectors > 0) ) {
		sdfat_fs_error_ratelimit(sb, "%s: out of range(sect:%d len:%d)",
						__func__ ,sec, num_secs);
		return -EIO;
	}

	if (bdev_mread(sb, sec, bh, num_secs, read)) {
		sdfat_fs_error_ratelimit(sb, "%s: I/O error (sect:%d len:%d)",
						__func__,sec, num_secs);
		return -EIO;
	}

	return 0;
} /* end of read_msect */

s32 write_msect(struct super_block *sb, u32 sec, struct buffer_head *bh, s32 num_secs, s32 sync)
{
	FS_INFO_T *fsi = &(SDFAT_SB(sb)->fsi);
	BUG_ON(!bh);

	if ( ((sec+num_secs) > fsi->num_sectors) && (fsi->num_sectors > 0) ) {
		sdfat_fs_error_ratelimit(sb, "%s: out of range(sect:%d len:%d)",
						__func__ ,sec, num_secs);
		return -EIO;
	}


	if (bdev_mwrite(sb, sec, bh, num_secs, sync)) {
		sdfat_fs_error_ratelimit(sb, "%s: I/O error (sect:%u len:%d)",
						__func__,sec, num_secs);
		return -EIO;
	}

	return 0;
} /* end of write_msect */

/* end of blkdev.c */
