/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
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

#include <linux/time.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include "ext4.h"
#include "xattr.h"
#include <linux/cred.h>
#include "ext4_dlp.h"
#include <linux/syscalls.h>

#include <sdp/sdp_dlp.h>
#include <sdp/fs_request.h>


int fbe_enable(void)
{
	return dlp_fbe_is_set();
}

int system_server_or_root(void)
{
	uid_t uid = from_kuid(&init_user_ns, current_uid());

	switch (uid) {

	case 0:
	case 1000:
		return 1;
	default:
		break;
	}

	return 0;
}

static inline int is_dlp_ps(void)
{
	if (in_egroup_p(AID_KNOX_DLP) ||
		in_egroup_p(AID_KNOX_DLP_RESTRICTED) ||
		in_egroup_p(AID_KNOX_DLP_MEDIA)) {

		pr_info("%s: DLP app [%s\n", __func__, current->comm);
		return 1;
	}

	pr_info("%s: not DLP app [%s]\n", __func__, current->comm);
	return 0;
}

static inline int get_knox_id(void)
{
	int knoxid = 0;

	knoxid = current_cred()->uid.val / 100000;

	if (knoxid < 100 || knoxid > 200) {
		pr_info("[%s] user_id is %d (normal)\n", __func__, knoxid);
		return 0;
	}

	pr_info("[%s] user_id is %d (knox)\n", __func__, knoxid);
	return knoxid;
}

int ext4_dlp_rename(struct inode *inode)
{
	sdp_fs_command_t *cmd = NULL;
	int knoxid = 0;

	if (!fbe_enable())
		return 0;

	knoxid = get_knox_id();

	if (knoxid < 100)
		return 0;

	pr_info("DLP %s\n", __func__);

	//create new init command and send--Handle transient case MS-Apps
	if (is_dlp_ps()) {
		pr_info("DLP %s send FSOP_DLP_FILE_RENAME\n", __func__);
		cmd = sdp_fs_command_alloc(FSOP_DLP_FILE_RENAME, current->tgid,
		knoxid, -1, inode->i_ino, GFP_KERNEL);

		if (cmd) {
			sdp_fs_request(cmd, NULL);
			sdp_fs_command_free(cmd);
		}
	}

    //end- Handle transient case MS-Apps

	return 0;
}

int ext4_dlp_open(struct inode *inode, struct file *filp)
{
	sdp_fs_command_t *cmd = NULL;

	struct dentry *dentry;
	ssize_t dlp_len = 0;
	struct knox_dlp_data dlp_data;
	struct timespec ts;
	int knoxid = 0;
	int rc = 0;
	int req = 0;

	if (!fbe_enable())
		return 0;

	knoxid = get_knox_id();
	if (knoxid < 100) {
		req = FSOP_DLP_FILE_ACCESS_DENIED;
		rc = -EPERM;
		goto dlp_out;
	}

	dlp_len = ext4_xattr_get(inode, EXT4_XATTR_INDEX_USER,
		KNOX_DLP_XATTR_NAME, &dlp_data, sizeof(dlp_data));

	if (dlp_len == 8) {
		pr_info("DLP %s : xattr len is 8bytes!! "
				"Check whether default int size is 32bit or 64bit\n", __func__);
	}

	dentry = filp->f_path.dentry;
	if (dlp_data.expiry_time.tv_sec == -1) {
		pr_info("DLP %s It is not DLP file -> "
				"media created file but not DLP [%s]\n",
				__func__, dentry->d_name.name);
		return 0;
	}

	if (dlp_is_locked(knoxid)) {
		pr_info("DLP %s : DLP locked\n", __func__);
		rc = -EPERM;
		goto dlp_out;
	}

	if (!is_dlp_ps()) {
		req = FSOP_DLP_FILE_ACCESS_DENIED;
		rc = -EPERM;
		goto dlp_out;
	}

	pr_info("DLP %s: DLP app [%s]\n", __func__, current->comm);

	if (dlp_len == sizeof(dlp_data)) {
		getnstimeofday(&ts);
		pr_info("DLP %s: current time [%ld/%ld] %s\n",
			__func__, (long)ts.tv_sec, (long)dlp_data.expiry_time.tv_sec,
			dentry->d_name.name);

		if (ts.tv_sec > dlp_data.expiry_time.tv_sec) {
			req = in_egroup_p(AID_KNOX_DLP_MEDIA) ?
				FSOP_DLP_FILE_REMOVE_MEDIA : FSOP_DLP_FILE_REMOVE;

			pr_info("DLP %s : time is expired.\n", __func__);
			rc = -ENOENT;
			goto dlp_out;
		}

	} else {
		pr_err("DLP %s: Error, len [%ld], [%s]\n",
			__func__, (long)dlp_len, dentry->d_name.name);
		rc = -EFAULT;
		goto dlp_out;
	}

	pr_info("DLP %s: DLP file [%s] opened with tgid %d, %d\n",
		__func__, dentry->d_name.name, current->tgid,
		in_egroup_p(AID_KNOX_DLP_RESTRICTED));

	if (in_egroup_p(AID_KNOX_DLP_RESTRICTED))
		req = FSOP_DLP_FILE_OPENED;
	else if (in_egroup_p(AID_KNOX_DLP))
		req = FSOP_DLP_FILE_OPENED_CREATOR;
	else
		pr_info("DLP %s: open file file from Media process "
				"ignoring sending event\n", __func__);

dlp_out:
	if (req > 0) {
		pr_info("DLP %s: req is %d\n", __func__, req);
		dump_stack();
		cmd = sdp_fs_command_alloc(req, current->tgid,
			knoxid, -1, inode->i_ino, GFP_KERNEL);

		if (cmd) {
			sdp_fs_request(cmd, NULL);
			sdp_fs_command_free(cmd);
		}
	}

	return rc;
}


int ext4_dlp_create(struct inode *inode)
{
	sdp_fs_command_t *cmd = NULL;
	struct knox_dlp_data dlp_data;
	struct timespec ts;
	int knoxid = 0;
	int res, req = 0;

	if (!fbe_enable())
		return 0;

	knoxid = get_knox_id();
	if (knoxid < 100)
		return 0;

	if (!is_dlp_ps())
		return 0;

	getnstimeofday(&ts);
	dlp_data.expiry_time.tv_sec = (int64_t)ts.tv_sec + 20;
	dlp_data.expiry_time.tv_nsec = (int64_t)ts.tv_nsec;

	ext4_set_inode_flag(inode, EXT4_DLP_FL);

	if (in_egroup_p(AID_KNOX_DLP))
		req = FSOP_DLP_FILE_INIT;

	else if (in_egroup_p(AID_KNOX_DLP_RESTRICTED))
		req = FSOP_DLP_FILE_INIT_RESTRICTED;

	else if (in_egroup_p(AID_KNOX_DLP_MEDIA)) {
		dlp_data.expiry_time.tv_sec = -1;
		pr_info("DLP %s : Media file.\n", __func__);
	}

	res = ext4_xattr_set(inode, EXT4_XATTR_INDEX_USER,
		KNOX_DLP_XATTR_NAME, &dlp_data, sizeof(dlp_data), 0);

	if (req > 0) {
		pr_info("DLP %s : send req\n", __func__);
		cmd = sdp_fs_command_alloc(req, current->tgid, knoxid,
			-1, inode->i_ino, GFP_KERNEL);

		if (cmd) {
			sdp_fs_request(cmd, NULL);
			sdp_fs_command_free(cmd);
		}
	}

	return res;
}
