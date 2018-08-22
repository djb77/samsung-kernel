/*
 * This code is based on IMA's code
 *
 * Copyright (C) 2016 Samsung Electronics, Inc.
 *
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
 * Viacheslav Vovchenko <v.vovchenko@samsung.com>
 * Yevgen Kopylov <y.kopylov@samsung.com>
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

#include <linux/module.h>
#include <linux/file.h>
#include <linux/binfmts.h>
#include <linux/mount.h>
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/xattr.h>
#include <crypto/hash_info.h>
#include <linux/task_integrity.h>

#include "five.h"
#include "five_audit.h"
#include "five_state.h"
#include "five_pa.h"
#include "five_trace.h"

static struct workqueue_struct *g_five_workqueue;

static inline void task_integrity_processing(struct task_integrity *tint);
static inline void task_integrity_done(struct task_integrity *tint);
static int process_measurement(const struct processing_event_list *params);
static inline struct processing_event_list *five_event_create(
		enum five_event event, struct task_struct *task,
		struct file *file, int function, gfp_t flags);
static inline void five_event_destroy(
		const struct processing_event_list *file);

static void work_handler(struct work_struct *in_data)
{
	int rc;
	struct worker_context *context = container_of(in_data,
			struct worker_context, data_work);
	struct task_integrity *intg;

	if (unlikely(!context))
		return;

	intg = context->tint;

	spin_lock(&intg->lock);
	while (!list_empty(&(intg->events.list))) {
		struct processing_event_list *five_file;

		five_file = list_entry(intg->events.list.next,
				struct processing_event_list, list);
		spin_unlock(&intg->lock);
		switch (five_file->event) {
		case FIVE_VERIFY_BUNCH_FILES: {
			rc = process_measurement(five_file);
			break;
		}
		case FIVE_RESET_INTEGRITY: {
			task_integrity_reset(intg);
			break;
		}
		default:
			break;
		}
		spin_lock(&intg->lock);
		list_del(&five_file->list);
		five_event_destroy(five_file);
	}

	task_integrity_done(intg);
	spin_unlock(&intg->lock);
	task_integrity_put(intg);

	kfree(context);
}

const char *five_d_path(struct path *path, char **pathbuf)
{
	char *pathname = NULL;

	*pathbuf = __getname();
	if (*pathbuf) {
		pathname = d_absolute_path(path, *pathbuf, PATH_MAX);
		if (IS_ERR(pathname)) {
			__putname(*pathbuf);
			*pathbuf = NULL;
			pathname = NULL;
		}
	}

	if (!pathname || !*pathbuf) {
		pr_err("FIVE: Can't obtain absolute path: %p %p\n",
				pathname, *pathbuf);
	}
	return pathname ?: (const char *)path->dentry->d_name.name;
}

int five_check_params(struct task_struct *task, struct file *file)
{
	struct inode *inode;

	if (unlikely(!file))
		return 1;

	inode = file_inode(file);
	if (!S_ISREG(inode->i_mode))
		return 1;

	return 0;
}

/* cut a list into two
 * @cur_list: a list with entries
 * @new_list: a new list to add all removed entries. Should be an empty list
 *
 * Use it under spin_lock
 *
 * The function moves second entry and following entries to new list.
 * First entry is left in cur_list.
 *
 * Initial state:
 * [cur_list]<=>[first]<=>[second]<=>[third]<=>...<=>[last]    [new_list]
 *    ^=================================================^        ^====^
 * Result:
 * [cur_list]<=>[first]
 *      ^==========^
 * [new_list]<=>[second]<=>[third]<=>...<=>[last]
 *     ^=====================================^
 *
 * the function is similar to kernel list_cut_position, but there are few
 * differences:
 * - cut position is second entry
 * - original list is left with only first entry
 * - moving entries are from second entry to last entry
 */
static void list_cut_tail(struct list_head *cur_list,
		struct list_head *new_list)
{
	if ((!list_empty(cur_list)) &&
			(!list_is_singular(cur_list))) {
		new_list->next = cur_list->next->next;
		cur_list->next->next->prev = new_list;
		cur_list->next->next = cur_list;
		new_list->prev = cur_list->prev;
		cur_list->prev->next = new_list;
		cur_list->prev = cur_list->next;
	}
}

static void free_files_list(struct list_head *list)
{
	struct list_head *tmp, *list_entry;
	struct processing_event_list *file_entry;

	list_for_each_safe(list_entry, tmp, list) {
		file_entry = list_entry(list_entry,
				struct processing_event_list, list);
		list_del(&file_entry->list);
		five_event_destroy(file_entry);
	}
}

static int push_file_event_bunch(struct task_struct *task, struct file *file,
		int function)
{
	int rc = 0;
	struct worker_context *context;
	struct processing_event_list *five_file;

	if (five_check_params(task, file))
		return 0;

	trace_five_push_workqueue(file, function);

	context = kmalloc(sizeof(struct worker_context), GFP_KERNEL);
	if (unlikely(!context))
		return -ENOMEM;

	five_file = five_event_create(FIVE_VERIFY_BUNCH_FILES, task, file,
			function, GFP_KERNEL);
	if (unlikely(!five_file)) {
		kfree(context);
		return -ENOMEM;
	}
	context->tint = task->integrity;

	spin_lock(&task->integrity->lock);

	if (list_empty(&(task->integrity->events.list))) {
		task_integrity_processing(task->integrity);
		list_add_tail(&five_file->list, &task->integrity->events.list);
		spin_unlock(&task->integrity->lock);
		task_integrity_get(five_file->saved_integrity);
		INIT_WORK(&context->data_work, work_handler);
		rc = queue_work(g_five_workqueue, &context->data_work) ? 0 : 1;
	} else {
		struct list_head dead_list;

		INIT_LIST_HEAD(&dead_list);
		if ((function == BPRM_CHECK) &&
			(!list_is_singular(&(task->integrity->events.list)))) {
			list_cut_tail(&task->integrity->events.list,
					&dead_list);
		}
		list_add_tail(&five_file->list, &task->integrity->events.list);
		spin_unlock(&task->integrity->lock);
		free_files_list(&dead_list);
		kfree(context);
	}
	return rc;
}

static int push_reset_event(struct task_struct *task)
{
	struct list_head dead_list;
	struct task_integrity *current_tint;
	struct processing_event_list *five_reset;

	INIT_LIST_HEAD(&dead_list);
	current_tint = task->integrity;
	task_integrity_get(current_tint);

	five_reset = five_event_create(FIVE_RESET_INTEGRITY, NULL, NULL, 0,
		GFP_KERNEL);
	if (unlikely(!five_reset)) {
		task_integrity_reset_both(current_tint);
		task_integrity_put(current_tint);
		return -ENOMEM;
	}

	task_integrity_reset_both(current_tint);
	spin_lock(&current_tint->lock);
	if (!list_empty(&current_tint->events.list)) {
		list_cut_tail(&current_tint->events.list, &dead_list);
		five_reset->event = FIVE_RESET_INTEGRITY;
		list_add_tail(&five_reset->list, &current_tint->events.list);
		spin_unlock(&current_tint->lock);
	} else {
		spin_unlock(&current_tint->lock);
		five_event_destroy(five_reset);
	}

	task_integrity_put(current_tint);
	/* remove dead_list */
	free_files_list(&dead_list);
	return 0;
}

void task_integrity_delayed_reset(struct task_struct *task)
{
	push_reset_event(task);
}

int five_hash_algo = HASH_ALGO_SHA1;
static int hash_setup_done;

static int __init hash_setup(char *str)
{
	int i;

	if (hash_setup_done)
		return 1;

	for (i = 0; i < HASH_ALGO__LAST; i++) {
		if (strcmp(str, hash_algo_name[i]) == 0) {
			five_hash_algo = i;
			break;
		}
	}

	hash_setup_done = 1;
	return 1;
}
__setup("ima_hash=", hash_setup);

static void five_check_last_writer(struct integrity_iint_cache *iint,
				  struct inode *inode, struct file *file)
{
	fmode_t mode = file->f_mode;

	if (!(mode & FMODE_WRITE))
		return;

	inode_lock(inode);
	if (atomic_read(&inode->i_writecount) == 1) {
		if (iint->version != inode->i_version)
			iint->five_status = FIVE_FILE_UNKNOWN;
	}
	inode_unlock(inode);
}

/**
 * five_file_free - called on __fput()
 * @file: pointer to file structure being freed
 *
 * Flag files that changed, based on i_version
 */
void five_file_free(struct file *file)
{
	struct inode *inode = file_inode(file);
	struct integrity_iint_cache *iint;

	if (!S_ISREG(inode->i_mode))
		return;

	fivepa_fsignature_free(file);

	iint = integrity_iint_find(inode);
	if (!iint)
		return;

	five_check_last_writer(iint, inode, file);
}

void five_task_free(struct task_struct *task)
{
	task_integrity_put(task->integrity);
}

/* Returns string representation of input function */
static const char *get_string_fn(enum five_hooks fn)
{
	switch (fn) {
	case FILE_CHECK:
		return "file-check";
	case MMAP_CHECK:
		return "mmap-check";
	case BPRM_CHECK:
		return "bprm-check";
	case POST_SETATTR:
		return "post-setattr";
	}
	return "unknown-function";
}

/* Returns 1 if input function could affect */
/* integrity and returns 0 if it couldn't   */
static int hook_affects_integrity(enum five_hooks fn)
{
	switch (fn) {
	case FILE_CHECK:
		return 1;
	case MMAP_CHECK:
		return 1;
	case BPRM_CHECK:
		return 1;
	case POST_SETATTR:
		return 0;
	}
	return 0;
}

static inline bool match_trusted_executable(struct five_cert *cert)
{
	struct five_cert_header *hdr = NULL;

	if (!cert)
		return false;

	hdr = (struct five_cert_header *)cert->body.header->value;

	if (hdr->privilege == FIVE_PRIV_ALLOW_SIGN)
		return true;

	return false;
}

static inline void task_integrity_processing(struct task_integrity *tint)
{
	tint->user_value = INTEGRITY_PROCESSING;
}

static inline void task_integrity_done(struct task_integrity *tint)
{
	tint->user_value = atomic_read(&tint->value);
}

static int process_measurement(const struct processing_event_list *params)
{
	struct task_struct *task = params->task;
	struct task_integrity *integrity = params->saved_integrity;
	struct file *file = params->file;
	int function = params->function;
	struct inode *inode = file_inode(file);
	struct integrity_iint_cache *iint = NULL;
	struct five_cert cert = { {0} };
	struct five_cert *pcert = NULL;
	char *pathbuf = NULL;
	const char *pathname = NULL;
	int rc = -ENOMEM;
	char *xattr_value = NULL;
	int xattr_len = 0;
	enum five_hooks fn = function;
	unsigned int trace_op = FIVE_TRACE_UNDEFINED;
	unsigned int trace_type = FIVE_TRACE_UNDEFINED;
	enum integrity_status prev_tint;

	trace_five_measurement_enter(file, task->pid);

	if (!S_ISREG(inode->i_mode)) {
		trace_five_measurement_exit(file,
			FIVE_TRACE_MEASUREMENT_OP_FILTEROUT,
			FIVE_TRACE_FILTEROUT_NONREG);
		return 0;
	}

	prev_tint = task_integrity_read(integrity);

	if (function != BPRM_CHECK) {
		if (task_integrity_read(integrity) == INTEGRITY_NONE) {
			trace_five_measurement_exit(file,
				FIVE_TRACE_MEASUREMENT_OP_FILTEROUT,
				FIVE_TRACE_FILTEROUT_NONEINTEGRITY);
			return 0;
		}
	}

	inode_lock(inode);

	iint = integrity_inode_get(inode);
	if (!iint)
		goto out;

	/* Nothing to do, just return existing appraised status */
	rc = five_get_cache_status(iint);
	if (rc != FIVE_FILE_UNKNOWN) {
		rc = 0;
		trace_op = FIVE_TRACE_MEASUREMENT_OP_CACHE;
		goto out_digsig;
	}

	xattr_len = five_read_xattr(file->f_path.dentry, &xattr_value);

	if (xattr_value && xattr_len) {
		trace_op = FIVE_TRACE_MEASUREMENT_OP_CALC;
		rc = five_cert_fillout(&cert, xattr_value, xattr_len);
		if (rc) {
			pr_err("FIVE: certificate is incorrect inode=%lu\n", inode->i_ino);
			goto out_digsig;
		}

		pcert = &cert;

		// TODO (v.vovchenko):
		// to investigate possible to move above this case
		if (file->f_flags & O_DIRECT) {
			rc = (iint->five_flags & IMA_PERMIT_DIRECTIO) ?
								0 : -EACCES;
			goto out_digsig;
		}
	}

	pathname = five_d_path(&file->f_path, &pathbuf);

	rc = five_appraise_measurement(task, function, iint, file,
			pathname, pcert);

	if (!rc) {
		if (match_trusted_executable(pcert))
			iint->five_flags |= FIVE_TRUSTED_FILE;
	}

out_digsig:
	kfree(xattr_value);
	if (pathbuf)
		__putname(pathbuf);
out:
	inode_unlock(inode);

	if (!iint) {
		trace_five_measurement_exit(file,
			FIVE_TRACE_MEASUREMENT_OP_FILTEROUT,
			FIVE_TRACE_FILTEROUT_ERROR);
		return 0;
	}

	if (trace_op != FIVE_TRACE_MEASUREMENT_OP_FILTEROUT) {
		trace_type = iint->five_status == FIVE_FILE_RSA ?
			FIVE_TRACE_MEASUREMENT_TYPE_RSA :
			FIVE_TRACE_MEASUREMENT_TYPE_HMAC;
	}

	if (rc)
		iint->five_flags &= ~FIVE_TRUSTED_FILE;

	if (rc || five_get_cache_status(iint) == FIVE_FILE_UNKNOWN
			|| five_get_cache_status(iint) == FIVE_FILE_FAIL) {
		if (hook_affects_integrity(fn)) {
			enum integrity_status tint;

			task_integrity_reset(integrity);
			tint = task_integrity_read(integrity);
			five_audit_info(task, file, get_string_fn(fn),
					prev_tint, tint, "reset-integrity", rc);
		}

		trace_five_measurement_exit(file, trace_op, trace_type);

		return -EACCES;
	}

	if (hook_affects_integrity(fn)) {
		int new_tint;
		const char *msg = NULL;

		new_tint = five_state_proceed(iint, integrity, fn, &msg);
		if (fn == BPRM_CHECK) {
			five_audit_verbose(task, file, get_string_fn(fn),
					prev_tint,
					(enum integrity_status)new_tint,
					"bprm-check", rc);
		}

		if (new_tint >= 0 && msg) {
			five_audit_verbose(task, file, get_string_fn(fn),
					prev_tint,
					(enum integrity_status)new_tint, msg,
					rc);
		}
	}

	trace_five_measurement_exit(file, trace_op, trace_type);

	return 0;
}

/**
 * five_file_mmap - measure files being mapped executable based on
 * the process_measurement() policy decision.
 * @file: pointer to the file to be measured (May be NULL)
 * @prot: contains the protection that will be applied by the kernel.
 *
 * On success return 0.
 */
int five_file_mmap(struct file *file, unsigned long prot)
{
	int rc = 0;
	struct task_struct *task = current;
	struct task_integrity *tint = task->integrity;

	if (file && task_integrity_user_read(tint)) {
		if (prot & PROT_EXEC) {
			rc = push_file_event_bunch(task, file, MMAP_CHECK);
			if (rc)
				return rc;
		}
		rc = fivepa_push_set_xattr_event(task, file);
	}

	return rc;
}

/**
 * five_bprm_check - Measure executable being launched based on
 * the process_measurement() policy decision.
 * @bprm: contains the linux_binprm structure
 *
 * On success return 0.
 */
int five_bprm_check(struct linux_binprm *bprm)
{
	int rc = 0;
	struct task_struct *task = current;

	trace_five_entry(bprm->file, FIVE_TRACE_ENTRY_BPRM_CHECK);

	if (likely(!task->ptrace)) {
		task_integrity_put(task->integrity);
		task->integrity = task_integrity_alloc();
		if (likely(task->integrity)) {
			rc = push_file_event_bunch(task, bprm->file, BPRM_CHECK);
			if (!rc)
				rc = fivepa_push_set_xattr_event(
							task, bprm->file);
		} else {
			rc = -ENOMEM;
		}
	}

	return rc;
}

static int __init init_five(void)
{
	int error;

	g_five_workqueue = create_workqueue("five_wq");
	if (!g_five_workqueue)
		return -ENOMEM;

	if (fivepa_init_signature_wq())
		return -ENOMEM;

	hash_setup(CONFIG_FIVE_DEFAULT_HASH);
	error = five_init();

	return error;
}

static int fcntl_verify(struct file *file)
{
	int rc = 0;
	struct task_struct *task = current;
	struct task_integrity *tint = task->integrity;

	if (task_integrity_user_read(tint))
		rc = push_file_event_bunch(task, file, FILE_CHECK);
	return rc;
}

/* Called from do_fcntl */
int five_fcntl_verify_async(struct file *file)
{
	trace_five_entry(file, FIVE_TRACE_ENTRY_VERIFY);

	return fcntl_verify(file);
}

/* Called from do_fcntl */
int five_fcntl_verify_sync(struct file *file)
{
	return -EINVAL;
}

int five_fork(struct task_struct *task, struct task_struct *child_task)
{
	int rc = 0;

	trace_five_entry(NULL, FIVE_TRACE_ENTRY_FORK);

	spin_lock(&task->integrity->lock);

	if (!list_empty(&task->integrity->events.list)) {
		/*copy the list*/
		struct list_head *tmp;
		struct processing_event_list *from_entry;
		struct worker_context *context;

		context = kmalloc(sizeof(struct worker_context), GFP_ATOMIC);
		if (unlikely(!context)) {
			spin_unlock(&task->integrity->lock);
			return -ENOMEM;
		}

		list_for_each(tmp, &task->integrity->events.list) {
			struct processing_event_list *five_file;

			from_entry = list_entry(tmp,
					struct processing_event_list, list);

			five_file = five_event_create(
					FIVE_VERIFY_BUNCH_FILES,
					child_task,
					from_entry->file,
					from_entry->function,
					GFP_ATOMIC);
			if (unlikely(!five_file)) {
				kfree(context);
				spin_unlock(&task->integrity->lock);
				return -ENOMEM;
			}

			list_add_tail(&five_file->list,
					&child_task->integrity->events.list);
		}

		context->tint = child_task->integrity;

		rc = task_integrity_copy(task->integrity,
				child_task->integrity);
		spin_unlock(&task->integrity->lock);
		task_integrity_get(context->tint);
		task_integrity_processing(child_task->integrity);
		INIT_WORK(&context->data_work, work_handler);
		rc = queue_work(g_five_workqueue, &context->data_work) ? 0 : 1;
	} else {
		rc = task_integrity_copy(task->integrity,
				child_task->integrity);
		spin_unlock(&task->integrity->lock);
	}
	return rc;
}

int five_ptrace(long request, struct task_struct *task)
{
#ifdef CONFIG_FIVE_DEBUG
	pr_info("FIVE: task gpid=%d pid=%d is traced, request=%ld\n",
		task_pid_nr(task->group_leader), task_pid_nr(task), request);
#endif
	return 0;
}

static inline struct processing_event_list *five_event_create(
		enum five_event event, struct task_struct *task,
		struct file *file, int function, gfp_t flags)
{
	struct processing_event_list *five_file;

	five_file = kmalloc(sizeof(struct processing_event_list), flags);
	if (unlikely(!five_file))
		return NULL;

	five_file->event = event;

	switch (five_file->event) {
	case FIVE_VERIFY_BUNCH_FILES: {
		get_task_struct(task);
		get_file(file);
		task_integrity_get(task->integrity);
		five_file->task = task;
		five_file->file = file;
		five_file->function = function;
		five_file->saved_integrity = task->integrity;
		break;
	}
	case FIVE_RESET_INTEGRITY: {
		break;
	}
	default:
		break;
	}

	return five_file;
}

static inline void five_event_destroy(
		const struct processing_event_list *file)
{
	switch (file->event) {
	case FIVE_VERIFY_BUNCH_FILES: {
		task_integrity_put(file->saved_integrity);
		fput(file->file);
		put_task_struct(file->task);
		break;
	}
	case FIVE_RESET_INTEGRITY: {
		break;
	}
	default:
		break;
	}
	kfree(file);
}

late_initcall(init_five);

MODULE_DESCRIPTION("File-based process Integrity Verifier");
MODULE_LICENSE("GPL");
