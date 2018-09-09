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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/hashtable.h>
#include <linux/ctype.h>
#include <linux/module.h>
#include "mount.h"

#define DEFINE_DLOG(_name) extern int dlog_##_name(const char *fmt, ...)
DEFINE_DLOG(mm);
DEFINE_DLOG(efs);
DEFINE_DLOG(etc);

#define EXT_SHIFT		7
#define MAX_EXT			(1 << 7)

struct ext {
	struct hlist_node hlist;
	const char *extension;
};

struct ext_hash_tbl {
	DECLARE_HASHTABLE(table, 7);
};

enum {
	DLOG_HT_EXTENSION = 0,
	DLOG_HT_EXCEPTION,
	DLOG_HT_MAX
};

enum {
	DLOG_SUPP_PART_DATA = 0,
	DLOG_SUPP_PART_EFS,
	DLOG_SUPP_PART_MAX
};

static struct ext_hash_tbl ht[DLOG_HT_MAX];

static const char *support_part[] = {
	"data", "efs", NULL,
};

static const char *extensions[] = {
	/* image */
	"arw", "bmp", "cr2", "dng", "gif",
	"jpeg", "jpg", "nef", "nrw", "orf",
	"pef", "png", "raf", "rw2", "srw",
	"wbmp", "webp",
	/* audio */
	"3ga", "aac", "amr",
	"awb", "dff", "dsf", "flac", "imy",
	"m4a", "mid", "midi", "mka", "mp3",
	"mpga",	"mxmf", "oga", "ogg", "ota",
	"rtttl", "rtx", "smf", "wav", "wma",
	"xmf",
	/* video */
	"3g2", "3gp", "3gpp", "3gpp2",
	"ak3g", "asf", "avi", "divx", "flv",
	"m2t", "m2ts", "m4v", "mkv", "mp4",
	"mpeg",	"mpg", "mts", "skm", "tp",
	"trp", "ts", "webm", "wmv",
	/* document */
	"asc",
	"csv", "doc", "docm", "docx", "dot",
	"dotx",	"htm", "html", "hwdt", "hwp",
	"hwpx",	"hwt", "memo", "pdf", "pot",
	"potx",	"pps", "ppsx", "ppt", "pptm",
	"pptx",	"rtf", "snb", "spd", "txt",
	"xls", "xlsm", "xlsx", "xlt", "xltx",
	"xml", 	NULL,
};

static const char *exceptions[] = {
	/* exception extension */
	"db-shm", "shm", "bak", "mbak", "gz",
	"log", "swap", "dcs", NULL,
};

static const char **extension_tbl[DLOG_HT_MAX] = {
	extensions, exceptions
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
static inline unsigned int dlog_full_name_hash(const char *name, unsigned int len)
{
	return full_name_hash((void *)0x0, name, len);
}
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0) */
static inline unsigned int dlog_full_name_hash(const char *name, unsigned int len)
{
	return full_name_hash(name, len);
}
#endif

bool is_ext(const char *name, const char token, struct ext_hash_tbl *hash_tbl)
{
	char *ext = strrchr(name, token);
	struct ext *hash_cur;
	unsigned hash;
	int i;
	bool ret = false;

	if (!ext || !ext[1])
		return false;

	ext = kstrdup(&ext[1], GFP_KERNEL);
	if (!ext)
		return false;

	for (i = 0; ext[i]; i++)
		ext[i] = tolower(ext[i]);
	hash = dlog_full_name_hash(ext, strlen(ext));

	hash_for_each_possible(hash_tbl->table, hash_cur, hlist, hash) {
		if (!strcmp(ext, hash_cur->extension)) {
			ret = true;
			break;
		}
	}
	kfree(ext);

	return ret;
}

int __init dlog_ext_hash_init(void)
{
	int i;
	int tbl_idx = 0;
	int num_ht = 0;

	do {
		hash_init(ht[num_ht].table);
		for (i = 0; extension_tbl[tbl_idx][i]; i++) {
			struct ext *hte;

			hte = kzalloc(sizeof(struct ext), GFP_KERNEL);
			if (!hte)
				return -ENOMEM;

			INIT_HLIST_NODE(&hte->hlist);
			hte->extension = extension_tbl[tbl_idx][i];
			hash_add(ht[num_ht].table, &hte->hlist,
					dlog_full_name_hash(hte->extension, strlen(hte->extension)));
		}
		tbl_idx++;
	} while(++num_ht < DLOG_HT_MAX);

	return 0;
}
module_init(dlog_ext_hash_init);

void __exit dlog_ext_hash_exit(void)
{
	int i;
	int num_ht = 0;
	struct ext *hash_cur;

	do {
		hash_for_each(ht[num_ht].table, i, hash_cur, hlist)
			kfree(hash_cur);
	} while(++num_ht < DLOG_HT_MAX);

}
module_exit(dlog_ext_hash_exit);

static int get_support_part_id(struct vfsmount *mnt)
{
	int idx = 0;
	struct mount *mount = real_mount(mnt);

	/* check partition which need register delete log */
	do {
		if (!strcmp(mount->mnt_mountpoint->d_name.name, support_part[idx]))
			return idx;
	} while(support_part[++idx]);

	return -1;
}

void dlog_hook(struct dentry *dentry, struct inode *inode, struct path *path)
{
	int part_id = get_support_part_id(path->mnt);
	char *buf;
	char *full_path;

	if (part_id < 0)
		return;

	buf = kzalloc(PATH_MAX, GFP_KERNEL);
	if (!buf) {
		printk(KERN_ERR "%s memory alloc failed : ENOMEM\n", __func__);
		return;
	}

	full_path = dentry_path_raw(dentry, buf, PATH_MAX);

	/* for efs partition */
	if (part_id == DLOG_SUPP_PART_EFS) {
		dlog_efs("\"%s\" (%lu, %lu)\n", full_path,
				path->dentry->d_inode->i_ino,
				inode->i_ino);
		goto out;
	}
	/* for other partitions */
	if (is_ext(dentry->d_name.name, '.', &ht[DLOG_HT_EXTENSION])) {
		dlog_mm("\"%s\" (%lu, %lu)\n", full_path,
				path->dentry->d_inode->i_ino,
				inode->i_ino);
	} else if (!is_ext(dentry->d_name.name, '.', &ht[DLOG_HT_EXCEPTION])
		&& !is_ext(dentry->d_name.name, '-', &ht[DLOG_HT_EXCEPTION])) {
		dlog_etc("\"%s\" (%lu, %lu)\n", full_path,
				path->dentry->d_inode->i_ino,
				inode->i_ino);
	}
out:
	kfree(buf);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Logging unlink file path");
MODULE_AUTHOR("Samsung Electronics Co., Ltd.");
