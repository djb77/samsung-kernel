/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "include/defex_catch_list.h"
#include "include/defex_internal.h"

#define MAX_PID_32 32768

#ifdef DEFEX_PED_ENABLE
DECLARE_HASHTABLE(creds_hash, 15);
#endif /* DEFEX_PED_ENABLE */

struct proc_cred_data {
	unsigned int uid, fsuid, egid;
};

struct proc_cred_struct {
	struct hlist_node node;
	struct proc_cred_data cred_data;
};

static DEFINE_SPINLOCK(creds_hash_update_lock);
static struct proc_cred_data *creds_fast_hash[MAX_PID_32 + 1];
static int creds_fast_hash_ready;

void creds_fast_hash_init(void)
{
	unsigned int i;

	for (i = 0; i <= MAX_PID_32; i++)
		creds_fast_hash[i] = NULL;
	creds_fast_hash_ready = 1;
}

int is_task_creds_ready(void)
{
	return creds_fast_hash_ready;
}

#ifdef DEFEX_PED_ENABLE
void get_task_creds(int pid, unsigned int *uid_ptr, unsigned int *fsuid_ptr, unsigned int *egid_ptr)
{
	struct proc_cred_struct *obj;
	unsigned int uid = 0, fsuid = 0, egid = 0;

	if (pid <= MAX_PID_32) {
		spin_lock(&creds_hash_update_lock);
		if (creds_fast_hash[pid] != NULL) {
			uid = creds_fast_hash[pid]->uid;
			fsuid = creds_fast_hash[pid]->fsuid;
			egid = creds_fast_hash[pid]->egid;
		}
		spin_unlock(&creds_hash_update_lock);
	} else {
		spin_lock(&creds_hash_update_lock);
		hash_for_each_possible(creds_hash, obj, node, pid) {
			uid = obj->cred_data.uid;
			fsuid = obj->cred_data.fsuid;
			egid = obj->cred_data.egid;
			break;
		}
		spin_unlock(&creds_hash_update_lock);
	}
	*uid_ptr = uid;
	*fsuid_ptr = fsuid;
	*egid_ptr = egid;
}

int set_task_creds(int pid, unsigned int uid, unsigned int fsuid, unsigned int egid)
{
	struct proc_cred_struct *obj;

	if (pid <= MAX_PID_32) {
		spin_lock(&creds_hash_update_lock);
		if (!creds_fast_hash[pid]) {
			creds_fast_hash[pid] = kmalloc(sizeof(struct proc_cred_data), GFP_ATOMIC);
			if (!creds_fast_hash[pid]) {
				spin_unlock(&creds_hash_update_lock);
				return -1;
			}
		}
		creds_fast_hash[pid]->uid = uid;
		creds_fast_hash[pid]->fsuid = fsuid;
		creds_fast_hash[pid]->egid = egid;
		spin_unlock(&creds_hash_update_lock);
		return 0;
	}

	spin_lock(&creds_hash_update_lock);
	hash_for_each_possible(creds_hash, obj, node, pid) {
		spin_unlock(&creds_hash_update_lock);
		obj->cred_data.uid = uid;
		obj->cred_data.fsuid = fsuid;
		return 0;
	}
	spin_unlock(&creds_hash_update_lock);
	obj = kmalloc(sizeof(struct proc_cred_struct), GFP_ATOMIC);
	if (!obj)
		return -1;
	obj->cred_data.uid = uid;
	obj->cred_data.fsuid = fsuid;
	spin_lock(&creds_hash_update_lock);
	hash_add(creds_hash, &obj->node, pid);
	spin_unlock(&creds_hash_update_lock);
	return 0;
}
#endif /* DEFEX_PED_ENABLE */

void delete_task_creds(int pid)
{
	struct proc_cred_struct *obj;
	struct proc_cred_data *cred_data;

	if (pid <= MAX_PID_32) {
		spin_lock(&creds_hash_update_lock);
		cred_data = creds_fast_hash[pid];
		creds_fast_hash[pid] = NULL;
		spin_unlock(&creds_hash_update_lock);
		kfree(cred_data);
		return;
	}

	spin_lock(&creds_hash_update_lock);
	hash_for_each_possible(creds_hash, obj, node, pid) {
		hash_del(&obj->node);
		spin_unlock(&creds_hash_update_lock);
		kfree(obj);
		return;
	}
	spin_unlock(&creds_hash_update_lock);
}

