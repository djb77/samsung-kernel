/*
 * Process Authentificator helpers
 *
 * Copyright (C) 2017 Samsung Electronics, Inc.
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

#include <linux/file.h>
#include <linux/task_integrity.h>
#include <linux/xattr.h>

#include "five.h"
#include "five_pa.h"
#include "lv.h"
#include "five_porting.h"

static struct workqueue_struct *g_f_signature_workqueue;

static void f_signature_handler(struct work_struct *in_data)
{
	struct task_integrity *tint;
	struct file *file;
	struct f_signature_task *f_signature_task;
	struct f_signature_context *context = container_of(in_data,
		struct f_signature_context, data_work);

	if (unlikely(!context))
		return;

	f_signature_task = &context->payload;
	tint = f_signature_task->tint;
	file = f_signature_task->file;

	if (task_integrity_user_read(tint) && !file->f_signature) {
		char *xattr_value = NULL;

		five_read_xattr(file->f_path.dentry, &xattr_value);
		file->f_signature = xattr_value;
	}

	task_integrity_put(tint);
	fput(file);

	kfree(context);
}

int fivepa_push_set_xattr_event(struct task_struct *task, struct file *file)
{
	struct f_signature_context *context;
	struct f_signature_task *f_signature_task;

	if (file->f_signature)
		return 0;

	if (five_check_params(task, file))
		return 0;

	context = kmalloc(sizeof(struct f_signature_context), GFP_KERNEL);
	if (unlikely(!context))
		return -ENOMEM;

	task_integrity_get(task->integrity);
	get_file(file);

	f_signature_task = &context->payload;
	f_signature_task->tint = task->integrity;
	f_signature_task->file = file;

	INIT_WORK(&context->data_work, f_signature_handler);
	return queue_work(g_f_signature_workqueue, &context->data_work) ? 0 : 1;
}

int fivepa_init_signature_wq(void)
{
	g_f_signature_workqueue =
			create_singlethread_workqueue("f_signature_wq");
	if (!g_f_signature_workqueue)
		return -ENOMEM;

	return 0;
}

void fivepa_fsignature_free(struct file *file)
{
	kfree(file->f_signature);
	file->f_signature = NULL;
}

int fivepa_fcntl_setxattr(struct file *file, void __user *lv_xattr)
{
	struct inode *inode = file_inode(file);
	struct lv lv_hdr = {0};
	int rc = -EPERM;
	void *x = NULL;
	enum task_integrity_value tint =
				    task_integrity_read(current->integrity);

	if (unlikely(!file || !lv_xattr))
		return -EINVAL;

	if (unlikely(copy_from_user(&lv_hdr, lv_xattr, sizeof(lv_hdr))))
		return -EFAULT;

	if (unlikely(lv_hdr.length > PAGE_SIZE))
		return -EINVAL;

	x = kmalloc(lv_hdr.length, GFP_NOFS);
	if (unlikely(!x))
		return -ENOMEM;

	if (unlikely(copy_from_user(x, lv_xattr + sizeof(lv_hdr),
		lv_hdr.length))) {
		rc = -EFAULT;
		goto out;
	}

	if (file->f_op && file->f_op->flush)
		if (file->f_op->flush(file, current->files)) {
			rc = -EOPNOTSUPP;
			goto out;
		}

	inode_lock(inode);

	if (tint == INTEGRITY_PRELOAD_ALLOW_SIGN
				|| tint == INTEGRITY_MIXED_ALLOW_SIGN
				|| tint == INTEGRITY_DMVERITY_ALLOW_SIGN) {
		rc = __vfs_setxattr_noperm(file->f_path.dentry,
						XATTR_NAME_PA,
						x,
						lv_hdr.length,
						0);
	}
	inode_unlock(inode);

out:
	kfree(x);

	return rc;
}
